// SPDX-License-Identifier: MIT
/*
$info$
category: glue ~ Logic that binds various parts together
meta: glue|driver ~ Emulation mainloop related glue logic
tags: glue|driver
desc: Glues Frontend, OpDispatcher and IR Opts & Compilation, LookupCache, Dispatcher and provides the Execution loop entrypoint
$end_info$
*/

#include <cstdint>
#include "Interface/Context/Context.h"
#include "Interface/Core/ArchHelpers//Arm64Emitter.h"
#include "Interface/Core/LookupCache.h"
#include "Interface/Core/CPUID.h"
#include "Interface/Core/Frontend.h"
#include "Interface/Core/ObjectCache/ObjectCacheService.h"
#include "Interface/Core/OpcodeDispatcher.h"
#include "Interface/Core/JIT/JITCore.h"
#include "Interface/Core/Dispatcher/Dispatcher.h"
#include "Interface/Core/X86Tables/X86Tables.h"
#include "Interface/HLE/Thunks/Thunks.h"
#include "Interface/IR/IR.h"
#include "Interface/IR/IREmitter.h"
#include "Interface/IR/Passes/RegisterAllocationPass.h"
#include "Interface/IR/Passes.h"
#include "Interface/IR/PassManager.h"
#include "Utils/Allocator.h"
#include "Utils/Allocator/HostAllocator.h"

#include <FEXCore/Config/Config.h>
#include <FEXCore/Core/CodeLoader.h>
#include <FEXCore/Core/Context.h>
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Core/CPUBackend.h>
#include <FEXCore/Core/SignalDelegator.h>
#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/Debug/InternalThreadState.h>
#include <FEXCore/HLE/SyscallHandler.h>
#include <FEXCore/HLE/SourcecodeResolver.h>
#include <FEXCore/HLE/Linux/ThreadManagement.h>
#include <FEXCore/IR/IR.h>
#include <FEXCore/IR/IntrusiveIRList.h>
#include <FEXCore/IR/RegisterAllocationData.h>
#include <FEXCore/Utils/Allocator.h>
#include <FEXCore/Utils/Event.h>
#include <FEXCore/Utils/File.h>
#include <FEXCore/Utils/LogManager.h>
#include "FEXCore/Utils/SignalScopeGuards.h"
#include <FEXCore/Utils/Threads.h>
#include <FEXCore/Utils/Profiler.h>
#include <FEXCore/fextl/fmt.h>
#include <FEXCore/fextl/memory.h>
#include <FEXCore/fextl/set.h>
#include <FEXCore/fextl/sstream.h>
#include <FEXCore/fextl/vector.h>
#include <FEXHeaderUtils/Syscalls.h>
#include <FEXHeaderUtils/TodoDefines.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <fcntl.h>
#include <functional>
#include <mutex>
#include <queue>
#include <shared_mutex>
#include <signal.h>
#include <stdio.h>
#include <string_view>
#include <sys/stat.h>
#include <type_traits>
#include <unistd.h>
#include <unordered_map>
#include <utility>
#include <xxhash.h>

namespace FEXCore::Context {
  ContextImpl::ContextImpl()
  : CPUID {this}
  , IRCaptureCache {this} {
#ifdef BLOCKSTATS
    BlockData = std::make_unique<FEXCore::BlockSamplingData>();
#endif
    if (Config.CacheObjectCodeCompilation() != FEXCore::Config::ConfigObjectCodeHandler::CONFIG_NONE) {
      CodeObjectCacheService = fextl::make_unique<FEXCore::CodeSerialize::CodeObjectSerializeService>(this);
    }
    if (!Config.Is64BitMode()) {
      // When operating in 32-bit mode, the virtual memory we care about is only the lower 32-bits.
      Config.VirtualMemSize = 1ULL << 32;
    }

    if (Config.BlockJITNaming() ||
        Config.GlobalJITNaming() ||
        Config.LibraryJITNaming()) {
      // Only initialize symbols file if enabled. Ensures we don't pollute /tmp with empty files.
      Symbols.InitFile();
    }

    // Track atomic TSO emulation configuration.
    UpdateAtomicTSOEmulationConfig();
  }

  ContextImpl::~ContextImpl() {
    {
      if (CodeObjectCacheService) {
        CodeObjectCacheService->Shutdown();
      }

      for (auto &Thread : Threads) {
        if (Thread->ExecutionThread->joinable()) {
          Thread->ExecutionThread->join(nullptr);
        }
      }

      for (auto &Thread : Threads) {
        delete Thread;
      }
      Threads.clear();
    }
  }

  uint64_t ContextImpl::RestoreRIPFromHostPC(FEXCore::Core::InternalThreadState *Thread, uint64_t HostPC) {
    const auto Frame = Thread->CurrentFrame;
    const uint64_t BlockBegin = Frame->State.InlineJITBlockHeader;
    auto InlineHeader = reinterpret_cast<const CPU::CPUBackend::JITCodeHeader *>(BlockBegin);

    if (InlineHeader) {
      auto InlineTail = reinterpret_cast<const CPU::CPUBackend::JITCodeTail *>(Frame->State.InlineJITBlockHeader + InlineHeader->OffsetToBlockTail);
      auto RIPEntries = reinterpret_cast<const CPU::CPUBackend::JITRIPReconstructEntries *>(Frame->State.InlineJITBlockHeader + InlineHeader->OffsetToBlockTail + InlineTail->OffsetToRIPEntries);

      // Check if the host PC is currently within a code block.
      // If it is then RIP can be reconstructed from the beginning of the code block.
      // This is currently as close as FEX can get RIP reconstructions.
      if (HostPC >= reinterpret_cast<uint64_t>(BlockBegin) &&
          HostPC < reinterpret_cast<uint64_t>(BlockBegin + InlineTail->Size)) {

        // Reconstruct RIP from JIT entries for this block.
        uint64_t StartingHostPC = BlockBegin;
        uint64_t StartingGuestRIP = InlineTail->RIP;

        for (uint32_t i = 0; i < InlineTail->NumberOfRIPEntries; ++i) {
          const auto &RIPEntry = RIPEntries[i];
          if (HostPC >= (StartingHostPC + RIPEntry.HostPCOffset)) {
            // We are beyond this entry, keep going forward.
            StartingHostPC += RIPEntry.HostPCOffset;
            StartingGuestRIP += RIPEntry.GuestRIPOffset;
          }
          else {
            // Passed where the Host PC is at. Break now.
            break;
          }
        }
        return StartingGuestRIP;
      }
    }

    // Fallback to what is stored in the RIP currently.
    return Frame->State.rip;
  }

  uint32_t ContextImpl::ReconstructCompactedEFLAGS(FEXCore::Core::InternalThreadState *Thread, bool WasInJIT, uint64_t *HostGPRs, uint64_t PSTATE) {
    const auto Frame = Thread->CurrentFrame;
    uint32_t EFLAGS{};

    // Currently these flags just map 1:1 inside of the resulting value.
    for (size_t i = 0; i < FEXCore::Core::CPUState::NUM_EFLAG_BITS; ++i) {
      switch (i) {
        case X86State::RFLAG_CF_RAW_LOC:
        case X86State::RFLAG_PF_RAW_LOC:
        case X86State::RFLAG_AF_RAW_LOC:
        case X86State::RFLAG_ZF_RAW_LOC:
        case X86State::RFLAG_SF_RAW_LOC:
        case X86State::RFLAG_OF_RAW_LOC:
          // Intentionally do nothing.
          // These contain multiple bits which can corrupt other members when compacted.
          break;
        default:
          EFLAGS |= uint32_t{Frame->State.flags[i]} << i;
          break;
      }
    }

    uint32_t Packed_NZCV{};
    if (WasInJIT) {
      // If we were in the JIT then NZCV is in the CPU's PSTATE object.
      // Packed in to the same bit locations as RFLAG_NZCV_LOC.
      Packed_NZCV = PSTATE;

      // If we were in the JIT then PF and AF are in registers.
      // Move them to the CPUState frame now.
      Frame->State.pf_raw = HostGPRs[CPU::REG_PF.Idx()];
      Frame->State.af_raw = HostGPRs[CPU::REG_AF.Idx()];
    }
    else {
      // If we were not in the JIT then the NZCV state is stored in the CPUState RFLAG_NZCV_LOC.
      // SF/ZF/CF/OF are packed in a 32-bit value in RFLAG_NZCV_LOC.
      memcpy(&Packed_NZCV, &Frame->State.flags[X86State::RFLAG_NZCV_LOC], sizeof(Packed_NZCV));
    }

    uint32_t OF = (Packed_NZCV >> IR::OpDispatchBuilder::IndexNZCV(X86State::RFLAG_OF_RAW_LOC)) & 1;
    uint32_t CF = (Packed_NZCV >> IR::OpDispatchBuilder::IndexNZCV(X86State::RFLAG_CF_RAW_LOC)) & 1;
    uint32_t ZF = (Packed_NZCV >> IR::OpDispatchBuilder::IndexNZCV(X86State::RFLAG_ZF_RAW_LOC)) & 1;
    uint32_t SF = (Packed_NZCV >> IR::OpDispatchBuilder::IndexNZCV(X86State::RFLAG_SF_RAW_LOC)) & 1;

    // Pack in to EFLAGS
    EFLAGS |= OF << X86State::RFLAG_OF_RAW_LOC;
    EFLAGS |= CF << X86State::RFLAG_CF_RAW_LOC;
    EFLAGS |= ZF << X86State::RFLAG_ZF_RAW_LOC;
    EFLAGS |= SF << X86State::RFLAG_SF_RAW_LOC;

    // PF calculation is deferred, calculate it now.
    // Popcount the 8-bit flag and then extract the lower bit.
    uint32_t PFByte = Frame->State.pf_raw & 0xff;
    uint32_t PF = std::popcount(PFByte ^ 1) & 1;
    EFLAGS |= PF << X86State::RFLAG_PF_RAW_LOC;

    // AF calculation is deferred, calculate it now.
    // XOR with PF byte and extract bit 4.
    uint32_t AF = ((Frame->State.af_raw ^ PFByte) & (1 << 4)) ? 1 : 0;
    EFLAGS |= AF << X86State::RFLAG_AF_RAW_LOC;

    return EFLAGS;
  }

  void ContextImpl::SetFlagsFromCompactedEFLAGS(FEXCore::Core::InternalThreadState *Thread, uint32_t EFLAGS) {
    const auto Frame = Thread->CurrentFrame;
    for (size_t i = 0; i < FEXCore::Core::CPUState::NUM_EFLAG_BITS; ++i) {
      switch (i) {
        case X86State::RFLAG_OF_RAW_LOC:
        case X86State::RFLAG_CF_RAW_LOC:
        case X86State::RFLAG_ZF_RAW_LOC:
        case X86State::RFLAG_SF_RAW_LOC:
          // Intentionally do nothing.
        break;
        case X86State::RFLAG_AF_RAW_LOC:
          // AF stored in bit 4 in our internal representation. It is also
          // XORed with byte 4 of the PF byte, but we write that as zero here so
          // we don't need any special handling for that.
          Frame->State.af_raw = (EFLAGS & (1U << i)) ? (1 << 4) : 0;
          break;
        case X86State::RFLAG_PF_RAW_LOC:
          // PF is inverted in our internal representation.
          Frame->State.pf_raw = (EFLAGS & (1U << i)) ? 0 : 1;
          break;
        default:
          Frame->State.flags[i] = (EFLAGS & (1U << i)) ? 1 : 0;
        break;
      }
    }

    // Calculate packed NZCV
    uint32_t Packed_NZCV{};
    Packed_NZCV |= (EFLAGS & (1U << X86State::RFLAG_OF_RAW_LOC)) ? 1U << IR::OpDispatchBuilder::IndexNZCV(X86State::RFLAG_OF_RAW_LOC) : 0;
    Packed_NZCV |= (EFLAGS & (1U << X86State::RFLAG_CF_RAW_LOC)) ? 1U << IR::OpDispatchBuilder::IndexNZCV(X86State::RFLAG_CF_RAW_LOC) : 0;
    Packed_NZCV |= (EFLAGS & (1U << X86State::RFLAG_ZF_RAW_LOC)) ? 1U << IR::OpDispatchBuilder::IndexNZCV(X86State::RFLAG_ZF_RAW_LOC) : 0;
    Packed_NZCV |= (EFLAGS & (1U << X86State::RFLAG_SF_RAW_LOC)) ? 1U << IR::OpDispatchBuilder::IndexNZCV(X86State::RFLAG_SF_RAW_LOC) : 0;
    memcpy(&Frame->State.flags[X86State::RFLAG_NZCV_LOC], &Packed_NZCV, sizeof(Packed_NZCV));

    // Reserved, Read-As-1, Write-as-1
    Frame->State.flags[X86State::RFLAG_RESERVED_LOC] = 1;
    // Interrupt Flag. Can't be written by CPL-3 userland.
    Frame->State.flags[X86State::RFLAG_IF_LOC] = 1;
  }

  bool ContextImpl::InitCore() {
    // Initialize the CPU core signal handlers & DispatcherConfig
    switch (Config.Core) {
    case FEXCore::Config::CONFIG_IRJIT:
      BackendFeatures = FEXCore::CPU::GetArm64JITBackendFeatures();
      break;
    case FEXCore::Config::CONFIG_CUSTOM:
      // Do nothing
      break;
    default:
      LogMan::Msg::EFmt("Unknown core configuration");
      return false;
    }

    Dispatcher = FEXCore::CPU::Dispatcher::Create(this);

    // Set up the SignalDelegator config since core is initialized.
    FEXCore::SignalDelegator::SignalDelegatorConfig SignalConfig {
      .SupportsAVX = HostFeatures.SupportsAVX,

      .DispatcherBegin = Dispatcher->Start,
      .DispatcherEnd = Dispatcher->End,

      .AbsoluteLoopTopAddressFillSRA = Dispatcher->AbsoluteLoopTopAddressFillSRA,
      .SignalHandlerReturnAddress = Dispatcher->SignalHandlerReturnAddress,
      .SignalHandlerReturnAddressRT = Dispatcher->SignalHandlerReturnAddressRT,

      .PauseReturnInstruction = Dispatcher->PauseReturnInstruction,
      .ThreadPauseHandlerAddressSpillSRA = Dispatcher->ThreadPauseHandlerAddressSpillSRA,
      .ThreadPauseHandlerAddress = Dispatcher->ThreadPauseHandlerAddress,

      // Stop handlers.
      .ThreadStopHandlerAddressSpillSRA = Dispatcher->ThreadStopHandlerAddressSpillSRA,
      .ThreadStopHandlerAddress = Dispatcher->ThreadStopHandlerAddress,

      // SRA information.
      .SRAGPRCount = Dispatcher->GetSRAGPRCount(),
      .SRAFPRCount = Dispatcher->GetSRAFPRCount(),
    };

    Dispatcher->GetSRAGPRMapping(SignalConfig.SRAGPRMapping);
    Dispatcher->GetSRAFPRMapping(SignalConfig.SRAFPRMapping);

    // Give this configuration to the SignalDelegator.
    SignalDelegation->SetConfig(SignalConfig);

#ifndef _WIN32
    ThunkHandler = FEXCore::ThunkHandler::Create();
#else
    // WIN32 always needs the interrupt fault check to be enabled.
    Config.NeedsPendingInterruptFaultCheck = true;
#endif

    if (Config.GdbServer) {
      // If gdbserver is enabled then this needs to be enabled.
      Config.NeedsPendingInterruptFaultCheck = true;
      // FEX needs to start paused when gdb is enabled.
      StartPaused = true;
    }

    return true;
  }

  void ContextImpl::HandleCallback(FEXCore::Core::InternalThreadState *Thread, uint64_t RIP) {
    static_cast<ContextImpl*>(Thread->CTX)->Dispatcher->ExecuteJITCallback(Thread->CurrentFrame, RIP);
  }

  void ContextImpl::WaitForIdle() {
    std::unique_lock<std::mutex> lk(IdleWaitMutex);
    IdleWaitCV.wait(lk, [this] {
      return IdleWaitRefCount.load() == 0;
    });

    Running = false;
  }

  void ContextImpl::WaitForIdleWithTimeout() {
    std::unique_lock<std::mutex> lk(IdleWaitMutex);
    bool WaitResult = IdleWaitCV.wait_for(lk, std::chrono::milliseconds(1500),
      [this] {
        return IdleWaitRefCount.load() == 0;
    });

    if (!WaitResult) {
      // The wait failed, this will occur if we stepped in to a syscall
      // That's okay, we just need to pause the threads manually
      NotifyPause();
    }

    // We have sent every thread a pause signal
    // Now wait again because they /will/ be going to sleep
    WaitForIdle();
  }

  void ContextImpl::NotifyPause() {

    // Tell all the threads that they should pause
    std::lock_guard<std::mutex> lk(ThreadCreationMutex);
    for (auto &Thread : Threads) {
      SignalDelegation->SignalThread(Thread, FEXCore::Core::SignalEvent::Pause);
    }
  }

  void ContextImpl::Pause() {
    // If we aren't running, WaitForIdle will never compete.
    if (Running) {
      NotifyPause();

      WaitForIdle();
    }
  }

  void ContextImpl::Run() {
    // Spin up all the threads
    std::lock_guard<std::mutex> lk(ThreadCreationMutex);
    for (auto &Thread : Threads) {
      Thread->SignalReason.store(FEXCore::Core::SignalEvent::Return);
    }

    for (auto &Thread : Threads) {
      Thread->StartRunning.NotifyAll();
    }
  }

  void ContextImpl::WaitForThreadsToRun() {
    size_t NumThreads{};
    {
      std::lock_guard<std::mutex> lk(ThreadCreationMutex);
      NumThreads = Threads.size();
    }

    // Spin while waiting for the threads to start up
    std::unique_lock<std::mutex> lk(IdleWaitMutex);
    IdleWaitCV.wait(lk, [this, NumThreads] {
      return IdleWaitRefCount.load() >= NumThreads;
    });

    Running = true;
  }

  void ContextImpl::Step() {
    {
      std::lock_guard<std::mutex> lk(ThreadCreationMutex);
      // Walk the threads and tell them to clear their caches
      // Useful when our block size is set to a large number and we need to step a single instruction
      for (auto &Thread : Threads) {
        ClearCodeCache(Thread);
      }
    }
    CoreRunningMode PreviousRunningMode = this->Config.RunningMode;
    int64_t PreviousMaxIntPerBlock = this->Config.MaxInstPerBlock;
    this->Config.RunningMode = FEXCore::Context::CoreRunningMode::MODE_SINGLESTEP;
    this->Config.MaxInstPerBlock = 1;
    Run();
    WaitForThreadsToRun();
    WaitForIdle();
    this->Config.RunningMode = PreviousRunningMode;
    this->Config.MaxInstPerBlock = PreviousMaxIntPerBlock;
  }

  void ContextImpl::Stop(bool IgnoreCurrentThread) {
    pid_t tid = FHU::Syscalls::gettid();
    FEXCore::Core::InternalThreadState* CurrentThread{};

    // Tell all the threads that they should stop
    {
      std::lock_guard<std::mutex> lk(ThreadCreationMutex);
      for (auto &Thread : Threads) {
        if (IgnoreCurrentThread &&
            Thread->ThreadManager.TID == tid) {
          // If we are callign stop from the current thread then we can ignore sending signals to this thread
          // This means that this thread is already gone
          continue;
        }
        else if (Thread->ThreadManager.TID == tid) {
          // We need to save the current thread for last to ensure all threads receive their stop signals
          CurrentThread = Thread;
          continue;
        }
        if (Thread->RunningEvents.Running.load()) {
          StopThread(Thread);
        }

        // If the thread is waiting to start but immediately killed then there can be a hang
        // This occurs in the case of gdb attach with immediate kill
        if (Thread->RunningEvents.WaitingToStart.load()) {
          Thread->RunningEvents.EarlyExit = true;
          Thread->StartRunning.NotifyAll();
        }
      }
    }

    // Stop the current thread now if we aren't ignoring it
    if (CurrentThread) {
      StopThread(CurrentThread);
    }
  }

  void ContextImpl::StopThread(FEXCore::Core::InternalThreadState *Thread) {
    if (Thread->RunningEvents.Running.exchange(false)) {
      SignalDelegation->SignalThread(Thread, FEXCore::Core::SignalEvent::Stop);
    }
  }

  void ContextImpl::SignalThread(FEXCore::Core::InternalThreadState *Thread, FEXCore::Core::SignalEvent Event) {
    if (Thread->RunningEvents.Running.load()) {
      SignalDelegation->SignalThread(Thread, Event);
    }
  }

  FEXCore::Context::ExitReason ContextImpl::RunUntilExit(FEXCore::Core::InternalThreadState *Thread) {
    if(!StartPaused) {
      // We will only have one thread at this point, but just in case run notify everything
      std::lock_guard lk(ThreadCreationMutex);
      for (auto &Thread : Threads) {
        Thread->StartRunning.NotifyAll();
      }
    }

    ExecutionThread(Thread);
    while(true) {
      this->WaitForIdle();
      auto reason = Thread->ExitReason;

      // Don't return if a custom exit handling the exit
      if (!CustomExitHandler || reason == ExitReason::EXIT_SHUTDOWN) {
        return reason;
      }
    }
  }

  void ContextImpl::ExecuteThread(FEXCore::Core::InternalThreadState *Thread) {
    Dispatcher->ExecuteDispatch(Thread->CurrentFrame);
  }


  void ContextImpl::InitializeThreadTLSData(FEXCore::Core::InternalThreadState *Thread) {
    // Let's do some initial bookkeeping here
    Thread->ThreadManager.TID = FHU::Syscalls::gettid();
    Thread->ThreadManager.PID = ::getpid();

    if (Config.BlockJITNaming() ||
        Config.GlobalJITNaming() ||
        Config.LibraryJITNaming()) {
      // Allocate a TLS JIT symbol buffer only if enabled.
      Thread->SymbolBuffer = JITSymbols::AllocateBuffer();
    }

    SignalDelegation->RegisterTLSState(Thread);
    if (ThunkHandler) {
      ThunkHandler->RegisterTLSState(Thread);
    }
#ifndef _WIN32
    Alloc::OSAllocator::RegisterTLSData(Thread);
#endif
  }

  void ContextImpl::RunThread(FEXCore::Core::InternalThreadState *Thread) {
    // Tell the thread to start executing
    Thread->StartRunning.NotifyAll();
  }

  void ContextImpl::InitializeCompiler(FEXCore::Core::InternalThreadState* Thread) {
    Thread->OpDispatcher = fextl::make_unique<FEXCore::IR::OpDispatchBuilder>(this);
    Thread->OpDispatcher->SetMultiblock(Config.Multiblock);
    Thread->LookupCache = fextl::make_unique<FEXCore::LookupCache>(this);
    Thread->FrontendDecoder = fextl::make_unique<FEXCore::Frontend::Decoder>(this);
    Thread->PassManager = fextl::make_unique<FEXCore::IR::PassManager>();

    Thread->CurrentFrame->Pointers.Common.L1Pointer = Thread->LookupCache->GetL1Pointer();
    Thread->CurrentFrame->Pointers.Common.L2Pointer = Thread->LookupCache->GetPagePointer();

    Dispatcher->InitThreadPointers(Thread);

    Thread->CTX = this;

    Thread->PassManager->AddDefaultPasses(this, Config.Core == FEXCore::Config::CONFIG_IRJIT);
    Thread->PassManager->AddDefaultValidationPasses();

    Thread->PassManager->RegisterSyscallHandler(SyscallHandler);

    // Create CPU backend
    switch (Config.Core) {
    case FEXCore::Config::CONFIG_IRJIT:
      Thread->PassManager->InsertRegisterAllocationPass(HostFeatures.SupportsAVX);
      Thread->CPUBackend = FEXCore::CPU::CreateArm64JITCore(this, Thread);
      break;
    case FEXCore::Config::CONFIG_CUSTOM:
      Thread->CPUBackend = CustomCPUFactory(this, Thread);
      break;
    default:
      ERROR_AND_DIE_FMT("Unknown core configuration");
      break;
    }

    Thread->PassManager->Finalize();
  }

  FEXCore::Core::InternalThreadState* ContextImpl::CreateThread(uint64_t InitialRIP, uint64_t StackPointer, ManagedBy WhoManages, FEXCore::Core::CPUState *NewThreadState, uint64_t ParentTID) {
    FEXCore::Core::InternalThreadState *Thread = new FEXCore::Core::InternalThreadState{};

    Thread->CurrentFrame->State.gregs[X86State::REG_RSP] = StackPointer;
    Thread->CurrentFrame->State.rip = InitialRIP;

    // Set up default code segment.
    // Default code segment indexes match the numbers that the Linux kernel uses.
    Thread->CurrentFrame->State.cs_idx = 6 << 3;
    auto &GDT = Thread->CurrentFrame->State.gdt[Thread->CurrentFrame->State.cs_idx >> 3];
    Thread->CurrentFrame->State.SetGDTBase(&GDT, 0);
    Thread->CurrentFrame->State.SetGDTLimit(&GDT, 0xF'FFFFU);

    if (Config.Is64BitMode) {
      GDT.L = 1; // L = Long Mode = 64-bit
      GDT.D = 0; // D = Default Operand SIze = Reserved
    }
    else {
      GDT.L = 0; // L = Long Mode = 32-bit
      GDT.D = 1; // D = Default Operand Size = 32-bit
    }

    // Copy over the new thread state to the new object
    if (NewThreadState) {
      memcpy(&Thread->CurrentFrame->State, NewThreadState, sizeof(FEXCore::Core::CPUState));
    }

    // Set up the thread manager state
    Thread->ThreadManager.parent_tid = ParentTID;
    Thread->CurrentFrame->Thread = Thread;

    InitializeCompiler(Thread);

    Thread->CurrentFrame->State.DeferredSignalRefCount.Store(0);
    Thread->CurrentFrame->State.DeferredSignalFaultAddress = reinterpret_cast<Core::NonAtomicRefCounter<uint64_t>*>(FEXCore::Allocator::VirtualAlloc(4096));
    Thread->DestroyedByParent = WhoManages == ManagedBy::FRONTEND;

    // Insert after the Thread object has been fully initialized
    if (WhoManages == ManagedBy::CORE) {
      std::lock_guard lk(ThreadCreationMutex);
      Threads.push_back(Thread);
    }

    return Thread;
  }

  void ContextImpl::DestroyThread(FEXCore::Core::InternalThreadState *Thread, bool NeedsTLSUninstall) {
    // remove new thread object
    {
      std::lock_guard lk(ThreadCreationMutex);

      auto It = std::find(Threads.begin(), Threads.end(), Thread);
      // TODO: Some threads aren't currently tracked in FEXCore.
      // Re-enable once tracking is in frontend.
      // LOGMAN_THROW_A_FMT(It != Threads.end(), "Thread wasn't in Threads");

      if (It != Threads.end()) {
        Threads.erase(It);
      }
    }

    if (Thread->ExecutionThread &&
        Thread->ExecutionThread->IsSelf()) {
      // To be able to delete a thread from itself, we need to detached the std::thread object
      Thread->ExecutionThread->detach();
    }

    // TODO: This is temporary until the frontend has full ownership of threads.
    if (NeedsTLSUninstall) {
#ifndef _WIN32
      Alloc::OSAllocator::UninstallTLSData(Thread);
#endif
      SignalDelegation->UninstallTLSState(Thread);
    }

    FEXCore::Allocator::VirtualFree(reinterpret_cast<void*>(Thread->CurrentFrame->State.DeferredSignalFaultAddress), 4096);
    delete Thread;
  }

#ifndef _WIN32
  void ContextImpl::UnlockAfterFork(FEXCore::Core::InternalThreadState *LiveThread, bool Child) {
    Allocator::UnlockAfterFork(LiveThread, Child);

    if (Child) {
      CodeInvalidationMutex.StealAndDropActiveLocks();
    }
    else {
      CodeInvalidationMutex.unlock();
      return;
    }

    // This function is called after fork
    // We need to cleanup some of the thread data that is dead
    for (auto &DeadThread : Threads) {
      if (DeadThread == LiveThread) {
        continue;
      }

      // Setting running to false ensures that when they are shutdown we won't send signals to kill them
      DeadThread->RunningEvents.Running = false;

      // Despite what google searches may susgest, glibc actually has special code to handle forks
      // with multiple active threads.
      // It cleans up the stacks of dead threads and marks them as terminated.
      // It also cleans up a bunch of internal mutexes.

      // FIXME: TLS is probally still alive. Investigate

      // Deconstructing the Interneal thread state should clean up most of the state.
      // But if anything on the now deleted stack is holding a refrence to the heap, it will be leaked
      delete DeadThread;

      // FIXME: Make sure sure nothing gets leaked via the heap. Ideas:
      //         * Make sure nothing is allocated on the heap without ref in InternalThreadState
      //         * Surround any code that heap allocates with a per-thread mutex.
      //           Before forking, the the forking thread can lock all thread mutexes.
    }

    // Remove all threads but the live thread from Threads
    Threads.clear();
    Threads.push_back(LiveThread);

    // We now only have one thread
    IdleWaitRefCount = 1;

    // Clean up dead stacks
    FEXCore::Threads::Thread::CleanupAfterFork();
  }

  void ContextImpl::LockBeforeFork(FEXCore::Core::InternalThreadState *Thread) {
    CodeInvalidationMutex.lock();
    Allocator::LockBeforeFork(Thread);
  }
#endif

  void ContextImpl::AddBlockMapping(FEXCore::Core::InternalThreadState *Thread, uint64_t Address, void *Ptr) {
    Thread->LookupCache->AddBlockMapping(Address, Ptr);
  }

  void ContextImpl::ClearCodeCache(FEXCore::Core::InternalThreadState *Thread) {
    FEXCORE_PROFILE_INSTANT("ClearCodeCache");

    {
      // Ensure the Code Object Serialization service has fully serialized this thread's data before clearing the cache
      // Use the thread's object cache ref counter for this
      CodeSerialize::CodeObjectSerializeService::WaitForEmptyJobQueue(&Thread->ObjectCacheRefCounter);
    }
    std::lock_guard<std::recursive_mutex> lk(Thread->LookupCache->WriteLock);

    Thread->LookupCache->ClearCache();
    Thread->CPUBackend->ClearCache();
    Thread->DebugStore.clear();
  }

  static void IRDumper(FEXCore::Core::InternalThreadState *Thread, IR::IREmitter *IREmitter, uint64_t GuestRIP, IR::RegisterAllocationData* RA) {
    FEXCore::File::File FD = FEXCore::File::File::GetStdERR();
    fextl::stringstream out;
    auto NewIR = IREmitter->ViewIR();
    FEXCore::IR::Dump(&out, &NewIR, RA);
    fextl::fmt::print(FD, "IR-ShouldDump-{} 0x{:x}:\n{}\n@@@@@\n", RA ? "post" : "pre", GuestRIP, out.str());
  };

  static void ValidateIR(ContextImpl *ctx, IR::IREmitter *IREmitter) {
    // Convert to text, Parse, Convert to text again and make sure the texts match
    fextl::stringstream out;
    static auto compaction = IR::CreateIRCompaction(ctx->OpDispatcherAllocator);
    compaction->Run(IREmitter);
    auto NewIR = IREmitter->ViewIR();
    Dump(&out, &NewIR, nullptr);
    out.seekg(0);
    FEXCore::Utils::PooledAllocatorMalloc Allocator;
    auto reparsed = IR::Parse(Allocator, out);
    if (reparsed == nullptr) {
      LOGMAN_MSG_A_FMT("Failed to parse IR\n");
    } else {
      fextl::stringstream out2;
      auto NewIR2 = reparsed->ViewIR();
      Dump(&out2, &NewIR2, nullptr);
      if (out.str() != out2.str()) {
        LogMan::Msg::IFmt("one:\n {}", out.str());
        LogMan::Msg::IFmt("two:\n {}", out2.str());
        LOGMAN_MSG_A_FMT("Parsed IR doesn't match\n");
      }
    }
  }

  ContextImpl::GenerateIRResult ContextImpl::GenerateIR(FEXCore::Core::InternalThreadState *Thread, uint64_t GuestRIP, bool ExtendedDebugInfo, uint64_t MaxInst) {
    FEXCORE_PROFILE_SCOPED("GenerateIR");

    Thread->OpDispatcher->ReownOrClaimBuffer();
    Thread->OpDispatcher->ResetWorkingList();

    uint64_t TotalInstructions {0};
    uint64_t TotalInstructionsLength {0};

    bool HasCustomIR{};

    if (HasCustomIRHandlers.load(std::memory_order_relaxed)) {
      std::shared_lock lk(CustomIRMutex);
      auto Handler = CustomIRHandlers.find(GuestRIP);
      if (Handler != CustomIRHandlers.end()) {
        TotalInstructions = 1;
        TotalInstructionsLength = 1;
        std::get<0>(Handler->second)(GuestRIP, Thread->OpDispatcher.get());
        HasCustomIR = true;
      }
    }

    if (!HasCustomIR) {
      uint8_t const *GuestCode{};
      GuestCode = reinterpret_cast<uint8_t const*>(GuestRIP);

      bool HadDispatchError {false};

      Thread->FrontendDecoder->DecodeInstructionsAtEntry(Thread, GuestCode, GuestRIP, MaxInst, [Thread](uint64_t BlockEntry, uint64_t Start, uint64_t Length) {
        if (Thread->LookupCache->AddBlockExecutableRange(BlockEntry, Start, Length)) {
          static_cast<ContextImpl*>(Thread->CTX)->SyscallHandler->MarkGuestExecutableRange(Thread, Start, Length);
        }
      });

      auto BlockInfo = Thread->FrontendDecoder->GetDecodedBlockInfo();
      auto CodeBlocks = &BlockInfo->Blocks;

      Thread->OpDispatcher->BeginFunction(GuestRIP, CodeBlocks, BlockInfo->TotalInstructionCount, BlockInfo->Is64BitMode);

      const uint8_t GPRSize = Thread->OpDispatcher->GetGPRSize();

      for (size_t j = 0; j < CodeBlocks->size(); ++j) {
        FEXCore::Frontend::Decoder::DecodedBlocks const &Block = CodeBlocks->at(j);
        // Set the block entry point
        Thread->OpDispatcher->SetNewBlockIfChanged(Block.Entry);

        uint64_t BlockInstructionsLength {};

        // Reset any block-specific state
        Thread->OpDispatcher->StartNewBlock();

        uint64_t InstsInBlock = Block.NumInstructions;

        for (size_t i = 0; i < InstsInBlock; ++i) {
          FEXCore::X86Tables::X86InstInfo const* TableInfo {nullptr};
          FEXCore::X86Tables::DecodedInst const* DecodedInfo {nullptr};

          TableInfo = Block.DecodedInstructions[i].TableInfo;
          DecodedInfo = &Block.DecodedInstructions[i];
          bool IsLocked = DecodedInfo->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_LOCK;

          if (ExtendedDebugInfo || Thread->OpDispatcher->CanHaveSideEffects(TableInfo, DecodedInfo)) {
            Thread->OpDispatcher->_GuestOpcode(Block.Entry + BlockInstructionsLength - GuestRIP);
          }

          if (Config.SMCChecks == FEXCore::Config::CONFIG_SMC_FULL) {
            auto ExistingCodePtr = reinterpret_cast<uint64_t*>(Block.Entry + BlockInstructionsLength);

            auto CodeChanged = Thread->OpDispatcher->_ValidateCode(ExistingCodePtr[0], ExistingCodePtr[1], (uintptr_t)ExistingCodePtr - GuestRIP, DecodedInfo->InstSize);

            auto InvalidateCodeCond = Thread->OpDispatcher->_CondJump(CodeChanged);

            auto CurrentBlock = Thread->OpDispatcher->GetCurrentBlock();
            auto CodeWasChangedBlock = Thread->OpDispatcher->CreateNewCodeBlockAtEnd();
            Thread->OpDispatcher->SetTrueJumpTarget(InvalidateCodeCond, CodeWasChangedBlock);

            Thread->OpDispatcher->SetCurrentCodeBlock(CodeWasChangedBlock);
            Thread->OpDispatcher->_ThreadRemoveCodeEntry();
            Thread->OpDispatcher->_ExitFunction(Thread->OpDispatcher->_EntrypointOffset(IR::SizeToOpSize(GPRSize), Block.Entry + BlockInstructionsLength - GuestRIP));

            auto NextOpBlock = Thread->OpDispatcher->CreateNewCodeBlockAfter(CurrentBlock);

            Thread->OpDispatcher->SetFalseJumpTarget(InvalidateCodeCond, NextOpBlock);
            Thread->OpDispatcher->SetCurrentCodeBlock(NextOpBlock);
          }

          if (TableInfo && TableInfo->OpcodeDispatcher) {
            auto Fn = TableInfo->OpcodeDispatcher;
            Thread->OpDispatcher->ResetHandledLock();
            Thread->OpDispatcher->ResetDecodeFailure();
            std::invoke(Fn, Thread->OpDispatcher, DecodedInfo);
            if (Thread->OpDispatcher->HadDecodeFailure()) {
              HadDispatchError = true;
            }
            else {
              if (Thread->OpDispatcher->HasHandledLock() != IsLocked) {
                HadDispatchError = true;
                LogMan::Msg::EFmt("Missing LOCK HANDLER at 0x{:x}{{'{}'}}", Block.Entry + BlockInstructionsLength, TableInfo->Name ?: "UND");
              }
              BlockInstructionsLength += DecodedInfo->InstSize;
              TotalInstructionsLength += DecodedInfo->InstSize;
              ++TotalInstructions;
            }
          }
          else {
            if (TableInfo) {
              LogMan::Msg::EFmt("Invalid or Unknown instruction: {} 0x{:x}", TableInfo->Name ?: "UND", Block.Entry - GuestRIP);
            }
            // Invalid instruction
            Thread->OpDispatcher->InvalidOp(DecodedInfo);
            Thread->OpDispatcher->_ExitFunction(Thread->OpDispatcher->_EntrypointOffset(IR::SizeToOpSize(GPRSize), Block.Entry - GuestRIP));
          }

          const bool NeedsBlockEnd = (HadDispatchError && TotalInstructions > 0) ||
            (Thread->OpDispatcher->NeedsBlockEnder() && i + 1 == InstsInBlock);

          // If we had a dispatch error then leave early
          if (HadDispatchError && TotalInstructions == 0) {
            // Couldn't handle any instruction in op dispatcher
            Thread->OpDispatcher->ResetWorkingList();
            return { nullptr, nullptr, 0, 0, 0, 0 };
          }

          if (NeedsBlockEnd) {
            // We had some instructions. Early exit
            Thread->OpDispatcher->_ExitFunction(Thread->OpDispatcher->_EntrypointOffset(IR::SizeToOpSize(GPRSize), Block.Entry + BlockInstructionsLength - GuestRIP));
            break;
          }


          if (Thread->OpDispatcher->FinishOp(DecodedInfo->PC + DecodedInfo->InstSize, i + 1 == InstsInBlock)) {
            break;
          }
        }
      }

      Thread->OpDispatcher->Finalize();

      Thread->FrontendDecoder->DelayedDisownBuffer();
    }

    IR::IREmitter *IREmitter = Thread->OpDispatcher.get();

    auto ShouldDump = Thread->OpDispatcher->ShouldDumpIR();
    // Debug
    {
      if (ShouldDump) {
        IRDumper(Thread, IREmitter, GuestRIP, nullptr);
      }

      if (static_cast<ContextImpl*>(Thread->CTX)->Config.ValidateIRarser) {
        ValidateIR(this, IREmitter);
      }
    }

    // Run the passmanager over the IR from the dispatcher
    Thread->PassManager->Run(IREmitter);

    // Debug
    {
      if (ShouldDump) {
        IRDumper(Thread, IREmitter, GuestRIP, Thread->PassManager->HasPass("RA") ? Thread->PassManager->GetPass<IR::RegisterAllocationPass>("RA")->GetAllocationData() : nullptr);
      }
    }

    auto RAData = Thread->PassManager->HasPass("RA") ? Thread->PassManager->GetPass<IR::RegisterAllocationPass>("RA")->PullAllocationData() : nullptr;
    auto IRList = IREmitter->CreateIRCopy();

    IREmitter->DelayedDisownBuffer();

    return {
      .IRList = IRList,
      .RAData = std::move(RAData),
      .TotalInstructions = TotalInstructions,
      .TotalInstructionsLength = TotalInstructionsLength,
      .StartAddr = Thread->FrontendDecoder->DecodedMinAddress,
      .Length = Thread->FrontendDecoder->DecodedMaxAddress - Thread->FrontendDecoder->DecodedMinAddress,
    };
  }

  ContextImpl::CompileCodeResult ContextImpl::CompileCode(FEXCore::Core::InternalThreadState *Thread, uint64_t GuestRIP, uint64_t MaxInst) {
    FEXCore::IR::IRListView *IRList {};
    FEXCore::Core::DebugData *DebugData {};
    FEXCore::IR::RegisterAllocationData::UniquePtr RAData {};
    bool GeneratedIR {};
    uint64_t StartAddr {};
    uint64_t Length {};

    // JIT Code object cache lookup
    if (CodeObjectCacheService) {
      auto CodeCacheEntry = CodeObjectCacheService->FetchCodeObjectFromCache(GuestRIP);
      if (CodeCacheEntry) {
        auto CompiledCode = Thread->CPUBackend->RelocateJITObjectCode(GuestRIP, CodeCacheEntry);
        if (CompiledCode) {
          return {
              .CompiledCode = CompiledCode,
              .IRData = nullptr,    // No IR data generated
              .DebugData = nullptr, // nullptr here ensures that code serialization doesn't occur on from cache read
              .RAData = nullptr,    // No RA data generated
              .GeneratedIR = false, // nullptr here ensures IR cache mechanisms won't run
              .StartAddr = 0,       // Unused
              .Length = 0,          // Unused
          };
        }
      }
    }

    if (SourcecodeResolver && Config.GDBSymbols()) {
      auto AOTIRCacheEntry = SyscallHandler->LookupAOTIRCacheEntry(Thread, GuestRIP);
      if (AOTIRCacheEntry.Entry && !AOTIRCacheEntry.Entry->ContainsCode) {
        AOTIRCacheEntry.Entry->SourcecodeMap =
            SourcecodeResolver->GenerateMap(AOTIRCacheEntry.Entry->Filename, AOTIRCacheEntry.Entry->FileId);
      }
    }

    // AOT IR bookkeeping and cache
    {
      auto [IRCopy, RACopy, DebugDataCopy, _StartAddr, _Length, _GeneratedIR] = IRCaptureCache.PreGenerateIRFetch(Thread, GuestRIP, IRList);
      if (_GeneratedIR) {
        // Setup pointers to internal structures
        IRList = IRCopy;
        RAData = std::move(RACopy);
        DebugData = DebugDataCopy;
        StartAddr = _StartAddr;
        Length = _Length;
        GeneratedIR = _GeneratedIR;
      }
    }

    if (IRList == nullptr) {
      // Generate IR + Meta Info
      auto [IRCopy, RACopy, TotalInstructions, TotalInstructionsLength, _StartAddr, _Length] = GenerateIR(Thread, GuestRIP, Config.GDBSymbols(), MaxInst);

      // Setup pointers to internal structures
      IRList = IRCopy;
      RAData = std::move(RACopy);
      DebugData = new FEXCore::Core::DebugData();
      StartAddr = _StartAddr;
      Length = _Length;

      // These blocks aren't already in the cache
      GeneratedIR = true;
    }

    if (IRList == nullptr) {
      return {};
    }
    // Attempt to get the CPU backend to compile this code
    return {
      // FEX currently throws away the CPUBackend::CompiledCode object other than the entrypoint
      // In the future with code caching getting wired up, we will pass the rest of the data forward.
      // TODO: Pass the data forward when code caching is wired up to this.
      .CompiledCode = Thread->CPUBackend->CompileCode(GuestRIP, IRList, DebugData, RAData.get()).BlockEntry,
      .IRData = IRList,
      .DebugData = DebugData,
      .RAData = std::move(RAData),
      .GeneratedIR = GeneratedIR,
      .StartAddr = StartAddr,
      .Length = Length,
    };
  }

  uintptr_t ContextImpl::CompileBlock(FEXCore::Core::CpuStateFrame *Frame, uint64_t GuestRIP, uint64_t MaxInst) {
    FEXCORE_PROFILE_SCOPED("CompileBlock");
    auto Thread = Frame->Thread;

    // Invalidate might take a unique lock on this, to guarantee that during invalidation no code gets compiled
    auto lk = GuardSignalDeferringSection<std::shared_lock>(CodeInvalidationMutex, Thread);

    // Is the code in the cache?
    // The backends only check L1 and L2, not L3
    if (auto HostCode = Thread->LookupCache->FindBlock(GuestRIP)) {
      return HostCode;
    }

    void *CodePtr {};
    FEXCore::IR::IRListView *IRList {};
    FEXCore::Core::DebugData *DebugData {};

    bool GeneratedIR {};
    uint64_t StartAddr {}, Length {};

    auto [Code, IR, Data, RAData, Generated, _StartAddr, _Length] = CompileCode(Thread, GuestRIP, MaxInst);
    CodePtr = Code;
    IRList = IR;
    DebugData = Data;
    GeneratedIR = Generated;
    StartAddr = _StartAddr;
    Length = _Length;

    if (CodePtr == nullptr) {
      return 0;
    }

    // The core managed to compile the code.
    if (Config.BlockJITNaming()) {
      auto FragmentBasePtr = reinterpret_cast<uint8_t *>(CodePtr);

      if (DebugData) {
        auto GuestRIPLookup = SyscallHandler->LookupAOTIRCacheEntry(Thread, GuestRIP);

        if (DebugData->Subblocks.size()) {
          for (auto& Subblock: DebugData->Subblocks) {
            auto BlockBasePtr = FragmentBasePtr + Subblock.HostCodeOffset;
            if (GuestRIPLookup.Entry) {
              Symbols.Register(Thread->SymbolBuffer.get(), BlockBasePtr, DebugData->HostCodeSize, GuestRIPLookup.Entry->Filename, GuestRIP - GuestRIPLookup.VAFileStart);
            } else {
              Symbols.Register(Thread->SymbolBuffer.get(), BlockBasePtr, GuestRIP, Subblock.HostCodeSize);
            }
          }
        } else {
          if (GuestRIPLookup.Entry) {
            Symbols.Register(Thread->SymbolBuffer.get(), FragmentBasePtr, DebugData->HostCodeSize, GuestRIPLookup.Entry->Filename, GuestRIP - GuestRIPLookup.VAFileStart);
        } else {
          Symbols.Register(Thread->SymbolBuffer.get(), FragmentBasePtr, GuestRIP, DebugData->HostCodeSize);
          }
        }
      }
    }

    // Tell the object cache service to serialize the code if enabled
    if (CodeObjectCacheService &&
        Config.CacheObjectCodeCompilation == FEXCore::Config::ConfigObjectCodeHandler::CONFIG_READWRITE &&
        DebugData) {
      CodeObjectCacheService->AsyncAddSerializationJob(fextl::make_unique<CodeSerialize::AsyncJobHandler::SerializationJobData>(
        CodeSerialize::AsyncJobHandler::SerializationJobData {
          .GuestRIP = GuestRIP,
          .GuestCodeLength = Length,
          .GuestCodeHash = 0,
          .HostCodeBegin = CodePtr,
          .HostCodeLength = DebugData->HostCodeSize,
          .HostCodeHash = 0,
          .ThreadJobRefCount = &Thread->ObjectCacheRefCounter,
          .Relocations = std::move(*DebugData->Relocations),
        }
      ));
    }

    // Clear any relocations that might have been generated
    Thread->CPUBackend->ClearRelocations();

    if (IRCaptureCache.PostCompileCode(
        Thread,
        CodePtr,
        GuestRIP,
        StartAddr,
        Length,
        std::move(RAData),
        IRList,
        DebugData,
        GeneratedIR)) {
      // Early exit
      return (uintptr_t)CodePtr;
    }

    // Insert to lookup cache
    // Pages containing this block are added via AddBlockExecutableRange before each page gets accessed in the frontend
    AddBlockMapping(Thread, GuestRIP, CodePtr);

    return (uintptr_t)CodePtr;
  }

  void ContextImpl::ExecutionThread(FEXCore::Core::InternalThreadState *Thread) {
    Thread->ExitReason = FEXCore::Context::ExitReason::EXIT_WAITING;

    InitializeThreadTLSData(Thread);
    ++IdleWaitRefCount;

    // Now notify the thread that we are initialized
    Thread->ThreadWaiting.NotifyAll();

    if (StartPaused || Thread->StartPaused) {
      // Parent thread doesn't need to wait to run
      Thread->StartRunning.Wait();
    }

    if (!Thread->RunningEvents.EarlyExit.load()) {
      Thread->RunningEvents.WaitingToStart = false;

      Thread->ExitReason = FEXCore::Context::ExitReason::EXIT_NONE;

      Thread->RunningEvents.Running = true;

      static_cast<ContextImpl*>(Thread->CTX)->Dispatcher->ExecuteDispatch(Thread->CurrentFrame);

      Thread->RunningEvents.Running = false;
    }

    {
      // Ensure the Code Object Serialization service has fully serialized this thread's data before clearing the cache
      // Use the thread's object cache ref counter for this
      CodeSerialize::CodeObjectSerializeService::WaitForEmptyJobQueue(&Thread->ObjectCacheRefCounter);
    }

    // If it is the parent thread that died then just leave
    FEX_TODO("This doesn't make sense when the parent thread doesn't outlive its children");

    if (Thread->ThreadManager.parent_tid == 0) {
      CoreShuttingDown.store(true);
      Thread->ExitReason = FEXCore::Context::ExitReason::EXIT_SHUTDOWN;

      if (CustomExitHandler) {
        CustomExitHandler(Thread->ThreadManager.TID, Thread->ExitReason);
      }
    }

    --IdleWaitRefCount;
    IdleWaitCV.notify_all();

#ifndef _WIN32
    Alloc::OSAllocator::UninstallTLSData(Thread);
#endif
    SignalDelegation->UninstallTLSState(Thread);

    // If the parent thread is waiting to join, then we can't destroy our thread object
    if (!Thread->DestroyedByParent) {
      Thread->CTX->DestroyThread(Thread);
    }
  }

  static void InvalidateGuestThreadCodeRange(FEXCore::Core::InternalThreadState *Thread, uint64_t Start, uint64_t Length) {
    std::lock_guard<std::recursive_mutex> lk(Thread->LookupCache->WriteLock);

    auto lower = Thread->LookupCache->CodePages.lower_bound(Start >> 12);
    auto upper = Thread->LookupCache->CodePages.upper_bound((Start + Length - 1) >> 12);

    for (auto it = lower; it != upper; it++) {
      for (auto Address: it->second) {
        ContextImpl::ThreadRemoveCodeEntry(Thread, Address);
      }
      it->second.clear();
    }
  }

  static void InvalidateGuestCodeRangeInternal(FEXCore::Core::InternalThreadState *CallingThread, ContextImpl *CTX, uint64_t Start, uint64_t Length) {
    std::lock_guard lk(static_cast<ContextImpl*>(CTX)->ThreadCreationMutex);

    for (auto &Thread : static_cast<ContextImpl*>(CTX)->Threads) {

      // TODO: Skip calling thread.
      // Remove once frontend has thread ownership.
      if (CallingThread == Thread) continue;
      InvalidateGuestThreadCodeRange(Thread, Start, Length);
    }

    // Now invalidate calling thread's code.
    if (CallingThread) {
      InvalidateGuestThreadCodeRange(CallingThread, Start, Length);
    }
  }

  void ContextImpl::InvalidateGuestCodeRange(FEXCore::Core::InternalThreadState *Thread, uint64_t Start, uint64_t Length) {
    // Potential deferred since Thread might not be valid.
    // Thread object isn't valid very early in frontend's initialization.
    // To be more optimal the frontend should provide this code with a valid Thread object earlier.
    auto lk = GuardSignalDeferringSectionWithFallback(CodeInvalidationMutex, Thread);

    InvalidateGuestCodeRangeInternal(Thread, this, Start, Length);
  }

  void ContextImpl::InvalidateGuestCodeRange(FEXCore::Core::InternalThreadState *Thread, uint64_t Start, uint64_t Length, CodeRangeInvalidationFn CallAfter) {
    // Potential deferred since Thread might not be valid.
    // Thread object isn't valid very early in frontend's initialization.
    // To be more optimal the frontend should provide this code with a valid Thread object earlier.
    auto lk = GuardSignalDeferringSectionWithFallback(CodeInvalidationMutex, Thread);

    InvalidateGuestCodeRangeInternal(Thread, this, Start, Length);
    CallAfter(Start, Length);
  }

  void ContextImpl::MarkMemoryShared(FEXCore::Core::InternalThreadState *Thread) {
    if (!IsMemoryShared) {
      IsMemoryShared = true;
      UpdateAtomicTSOEmulationConfig();

      if (Config.TSOAutoMigration) {
        std::lock_guard<std::mutex> lkThreads(ThreadCreationMutex);

        // Only the lookup cache is cleared here, so that old code can keep running until next compilation
        std::lock_guard<std::recursive_mutex> lkLookupCache(Thread->LookupCache->WriteLock);
        Thread->LookupCache->ClearCache();

        // DebugStore also needs to be cleared
        Thread->DebugStore.clear();
      }
    }
  }

  void ContextImpl::ThreadAddBlockLink(FEXCore::Core::InternalThreadState *Thread, uint64_t GuestDestination, FEXCore::Context::ExitFunctionLinkData *HostLink, const FEXCore::Context::BlockDelinkerFunc &delinker) {
    auto lk = GuardSignalDeferringSection<std::shared_lock>(static_cast<ContextImpl*>(Thread->CTX)->CodeInvalidationMutex, Thread);

    Thread->LookupCache->AddBlockLink(GuestDestination, HostLink, delinker);
  }

  void ContextImpl::ThreadRemoveCodeEntry(FEXCore::Core::InternalThreadState *Thread, uint64_t GuestRIP) {
    LogMan::Throw::AFmt(static_cast<ContextImpl*>(Thread->CTX)->CodeInvalidationMutex.try_lock() == false, "CodeInvalidationMutex needs to be unique_locked here");

    std::lock_guard<std::recursive_mutex> lk(Thread->LookupCache->WriteLock);

    Thread->DebugStore.erase(GuestRIP);
    Thread->LookupCache->Erase(Thread->CurrentFrame, GuestRIP);
  }

  CustomIRResult ContextImpl::AddCustomIREntrypoint(uintptr_t Entrypoint, CustomIREntrypointHandler Handler, void *Creator, void *Data) {
    LOGMAN_THROW_A_FMT(Config.Is64BitMode || !(Entrypoint >> 32), "64-bit Entrypoint in 32-bit mode {:x}", Entrypoint);

    std::unique_lock lk(CustomIRMutex);

    auto InsertedIterator = CustomIRHandlers.emplace(Entrypoint, std::tuple(Handler, Creator, Data));
    HasCustomIRHandlers = true;

    if (!InsertedIterator.second) {
      const auto &[fn, Creator, Data] = InsertedIterator.first->second;
      return CustomIRResult(std::move(lk), Creator, Data);
    } else {
      lk.unlock();
      return CustomIRResult(std::move(lk), 0, 0);
    }
  }

  void ContextImpl::RemoveCustomIREntrypoint(uintptr_t Entrypoint) {
    LOGMAN_THROW_A_FMT(Config.Is64BitMode || !(Entrypoint >> 32), "64-bit Entrypoint in 32-bit mode {:x}", Entrypoint);

    std::scoped_lock lk(CustomIRMutex);

    InvalidateGuestCodeRange(nullptr, Entrypoint, 1, [this](uint64_t Entrypoint, uint64_t) {
      CustomIRHandlers.erase(Entrypoint);
    });

    HasCustomIRHandlers = !CustomIRHandlers.empty();
  }

  IR::AOTIRCacheEntry *ContextImpl::LoadAOTIRCacheEntry(const fextl::string &filename) {
    auto rv = IRCaptureCache.LoadAOTIRCacheEntry(filename);
    return rv;
  }

  void ContextImpl::UnloadAOTIRCacheEntry(IR::AOTIRCacheEntry *Entry) {
    IRCaptureCache.UnloadAOTIRCacheEntry(Entry);
  }

  void ContextImpl::AppendThunkDefinitions(fextl::vector<FEXCore::IR::ThunkDefinition> const& Definitions) {
    if (ThunkHandler) {
      ThunkHandler->AppendThunkDefinitions(Definitions);
    }
  }

  void ContextImpl::ConfigureAOTGen(FEXCore::Core::InternalThreadState *Thread, fextl::set<uint64_t> *ExternalBranches, uint64_t SectionMaxAddress) {
    Thread->FrontendDecoder->SetExternalBranches(ExternalBranches);
    Thread->FrontendDecoder->SetSectionMaxAddress(SectionMaxAddress);
  }
}
