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
#include "Interface/Core/CPUBackend.h"
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
#include "Interface/IR/RegisterAllocationData.h"
#include "Utils/Allocator.h"
#include "Utils/Allocator/HostAllocator.h"

#include <FEXCore/Config/Config.h>
#include <FEXCore/Core/Context.h>
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Core/SignalDelegator.h>
#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/Debug/InternalThreadState.h>
#include <FEXCore/HLE/SyscallHandler.h>
#include <FEXCore/HLE/SourcecodeResolver.h>
#include <FEXCore/HLE/Linux/ThreadManagement.h>
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

  if (Config.BlockJITNaming() || Config.GlobalJITNaming() || Config.LibraryJITNaming()) {
    // Only initialize symbols file if enabled. Ensures we don't pollute /tmp with empty files.
    Symbols.InitFile();
  }

  if (FEXCore::GetCycleCounterFrequency() >= FEXCore::Context::TSC_SCALE_MAXIMUM) {
    Config.SmallTSCScale = false;
  }

  // Track atomic TSO emulation configuration.
  UpdateAtomicTSOEmulationConfig();
}

ContextImpl::~ContextImpl() {
  {
    if (CodeObjectCacheService) {
      CodeObjectCacheService->Shutdown();
    }
  }
}

uint64_t ContextImpl::RestoreRIPFromHostPC(FEXCore::Core::InternalThreadState* Thread, uint64_t HostPC) {
  const auto Frame = Thread->CurrentFrame;
  const uint64_t BlockBegin = Frame->State.InlineJITBlockHeader;
  auto InlineHeader = reinterpret_cast<const CPU::CPUBackend::JITCodeHeader*>(BlockBegin);

  if (InlineHeader) {
    auto InlineTail = reinterpret_cast<const CPU::CPUBackend::JITCodeTail*>(Frame->State.InlineJITBlockHeader + InlineHeader->OffsetToBlockTail);
    auto RIPEntries = reinterpret_cast<const CPU::CPUBackend::JITRIPReconstructEntries*>(
      Frame->State.InlineJITBlockHeader + InlineHeader->OffsetToBlockTail + InlineTail->OffsetToRIPEntries);

    // Check if the host PC is currently within a code block.
    // If it is then RIP can be reconstructed from the beginning of the code block.
    // This is currently as close as FEX can get RIP reconstructions.
    if (HostPC >= reinterpret_cast<uint64_t>(BlockBegin) && HostPC < reinterpret_cast<uint64_t>(BlockBegin + InlineTail->Size)) {

      // Reconstruct RIP from JIT entries for this block.
      uint64_t StartingHostPC = BlockBegin;
      uint64_t StartingGuestRIP = InlineTail->RIP;

      for (uint32_t i = 0; i < InlineTail->NumberOfRIPEntries; ++i) {
        const auto& RIPEntry = RIPEntries[i];
        if (HostPC >= (StartingHostPC + RIPEntry.HostPCOffset)) {
          // We are beyond this entry, keep going forward.
          StartingHostPC += RIPEntry.HostPCOffset;
          StartingGuestRIP += RIPEntry.GuestRIPOffset;
        } else {
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

uint32_t ContextImpl::ReconstructCompactedEFLAGS(FEXCore::Core::InternalThreadState* Thread, bool WasInJIT, uint64_t* HostGPRs, uint64_t PSTATE) {
  const auto Frame = Thread->CurrentFrame;
  uint32_t EFLAGS {};

  // Currently these flags just map 1:1 inside of the resulting value.
  for (size_t i = 0; i < FEXCore::Core::CPUState::NUM_EFLAG_BITS; ++i) {
    switch (i) {
    case X86State::RFLAG_CF_RAW_LOC:
    case X86State::RFLAG_PF_RAW_LOC:
    case X86State::RFLAG_AF_RAW_LOC:
    case X86State::RFLAG_ZF_RAW_LOC:
    case X86State::RFLAG_SF_RAW_LOC:
    case X86State::RFLAG_OF_RAW_LOC:
    case X86State::RFLAG_DF_RAW_LOC:
      // Intentionally do nothing.
      // These contain multiple bits which can corrupt other members when compacted.
      break;
    default: EFLAGS |= uint32_t {Frame->State.flags[i]} << i; break;
    }
  }

  uint32_t Packed_NZCV {};
  if (WasInJIT) {
    // If we were in the JIT then NZCV is in the CPU's PSTATE object.
    // Packed in to the same bit locations as RFLAG_NZCV_LOC.
    Packed_NZCV = PSTATE;

    // If we were in the JIT then PF and AF are in registers.
    // Move them to the CPUState frame now.
    Frame->State.pf_raw = HostGPRs[CPU::REG_PF.Idx()];
    Frame->State.af_raw = HostGPRs[CPU::REG_AF.Idx()];
  } else {
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

  // DF is pretransformed, undo the transform from 1/-1 back to 0/1
  uint8_t DFByte = Frame->State.flags[X86State::RFLAG_DF_RAW_LOC];
  if (DFByte & 0x80) {
    EFLAGS |= 1 << X86State::RFLAG_DF_RAW_LOC;
  }

  return EFLAGS;
}

void ContextImpl::ReconstructXMMRegisters(const FEXCore::Core::InternalThreadState* Thread, __uint128_t* XMM_Low, __uint128_t* YMM_High) {
  const size_t MaximumRegisters = Config.Is64BitMode ? FEXCore::Core::CPUState::NUM_XMMS : 8;

  if (YMM_High != nullptr && HostFeatures.SupportsAVX) {
    const bool SupportsConvergedRegisters = HostFeatures.SupportsSVE256;

    if (SupportsConvergedRegisters) {
      ///< Output wants to de-interleave
      for (size_t i = 0; i < MaximumRegisters; ++i) {
        memcpy(&XMM_Low[i], &Thread->CurrentFrame->State.xmm.avx.data[i][0], sizeof(__uint128_t));
        memcpy(&YMM_High[i], &Thread->CurrentFrame->State.xmm.avx.data[i][2], sizeof(__uint128_t));
      }
    } else {
      ///< Matches what FEX wants with non-converged registers
      for (size_t i = 0; i < MaximumRegisters; ++i) {
        memcpy(&XMM_Low[i], &Thread->CurrentFrame->State.xmm.sse.data[i][0], sizeof(__uint128_t));
        memcpy(&YMM_High[i], &Thread->CurrentFrame->State.avx_high[i][0], sizeof(__uint128_t));
      }
    }
  } else {
    // Only support SSE, no AVX here, even if requested.
    memcpy(XMM_Low, Thread->CurrentFrame->State.xmm.sse.data, MaximumRegisters * sizeof(__uint128_t));
  }
}

void ContextImpl::SetXMMRegistersFromState(FEXCore::Core::InternalThreadState* Thread, const __uint128_t* XMM_Low, const __uint128_t* YMM_High) {
  const size_t MaximumRegisters = Config.Is64BitMode ? FEXCore::Core::CPUState::NUM_XMMS : 8;
  if (YMM_High != nullptr && HostFeatures.SupportsAVX) {
    const bool SupportsConvergedRegisters = HostFeatures.SupportsSVE256;

    if (SupportsConvergedRegisters) {
      ///< Output wants to de-interleave
      for (size_t i = 0; i < MaximumRegisters; ++i) {
        memcpy(&Thread->CurrentFrame->State.xmm.avx.data[i][0], &XMM_Low[i], sizeof(__uint128_t));
        memcpy(&Thread->CurrentFrame->State.xmm.avx.data[i][2], &YMM_High[i], sizeof(__uint128_t));
      }
    } else {
      ///< Matches what FEX wants with non-converged registers
      for (size_t i = 0; i < MaximumRegisters; ++i) {
        memcpy(&Thread->CurrentFrame->State.xmm.sse.data[i][0], &XMM_Low[i], sizeof(__uint128_t));
        memcpy(&Thread->CurrentFrame->State.avx_high[i][0], &YMM_High[i], sizeof(__uint128_t));
      }
    }
  } else {
    // Only support SSE, no AVX here, even if requested.
    memcpy(Thread->CurrentFrame->State.xmm.sse.data, XMM_Low, MaximumRegisters * sizeof(__uint128_t));
  }
}

void ContextImpl::SetFlagsFromCompactedEFLAGS(FEXCore::Core::InternalThreadState* Thread, uint32_t EFLAGS) {
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
    case X86State::RFLAG_DF_RAW_LOC:
      // DF is encoded as 1/-1
      Frame->State.flags[i] = (EFLAGS & (1U << i)) ? 0xff : 1;
      break;
    default: Frame->State.flags[i] = (EFLAGS & (1U << i)) ? 1 : 0; break;
    }
  }

  // Calculate packed NZCV
  uint32_t Packed_NZCV {};
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
  case FEXCore::Config::CONFIG_IRJIT: BackendFeatures = FEXCore::CPU::GetArm64JITBackendFeatures(); break;
  case FEXCore::Config::CONFIG_CUSTOM:
    // Do nothing
    break;
  default: LogMan::Msg::EFmt("Unknown core configuration"); return false;
  }

  Dispatcher = FEXCore::CPU::Dispatcher::Create(this);

  // Set up the SignalDelegator config since core is initialized.
  FEXCore::SignalDelegator::SignalDelegatorConfig SignalConfig {
    .SupportsAVX = HostFeatures.SupportsAVX,

    .DispatcherBegin = Dispatcher->Start,
    .DispatcherEnd = Dispatcher->End,

    .AbsoluteLoopTopAddress = Dispatcher->AbsoluteLoopTopAddress,
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

void ContextImpl::HandleCallback(FEXCore::Core::InternalThreadState* Thread, uint64_t RIP) {
  static_cast<ContextImpl*>(Thread->CTX)->Dispatcher->ExecuteJITCallback(Thread->CurrentFrame, RIP);
}

FEXCore::Context::ExitReason ContextImpl::RunUntilExit(FEXCore::Core::InternalThreadState* Thread) {
  ExecutionThread(Thread);
  while (true) {
    auto reason = Thread->ExitReason;

    // Don't return if a custom exit handling the exit
    if (!CustomExitHandler || reason == ExitReason::EXIT_SHUTDOWN) {
      return reason;
    }
  }
}

void ContextImpl::ExecuteThread(FEXCore::Core::InternalThreadState* Thread) {
  Dispatcher->ExecuteDispatch(Thread->CurrentFrame);
}


void ContextImpl::InitializeThreadTLSData(FEXCore::Core::InternalThreadState* Thread) {
  // Let's do some initial bookkeeping here
  Thread->ThreadManager.TID = FHU::Syscalls::gettid();
  Thread->ThreadManager.PID = ::getpid();

  if (ThunkHandler) {
    ThunkHandler->RegisterTLSState(Thread);
  }
#ifndef _WIN32
  Alloc::OSAllocator::RegisterTLSData(Thread);
#endif
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

  Thread->PassManager->AddDefaultPasses(this);
  Thread->PassManager->AddDefaultValidationPasses();

  Thread->PassManager->RegisterSyscallHandler(SyscallHandler);

  // Create CPU backend
  switch (Config.Core) {
  case FEXCore::Config::CONFIG_IRJIT:
    Thread->PassManager->InsertRegisterAllocationPass();
    Thread->CPUBackend = FEXCore::CPU::CreateArm64JITCore(this, Thread);
    break;
  case FEXCore::Config::CONFIG_CUSTOM: Thread->CPUBackend = CustomCPUFactory(this, Thread); break;
  default: ERROR_AND_DIE_FMT("Unknown core configuration"); break;
  }

  Thread->PassManager->Finalize();
}

FEXCore::Core::InternalThreadState*
ContextImpl::CreateThread(uint64_t InitialRIP, uint64_t StackPointer, FEXCore::Core::CPUState* NewThreadState, uint64_t ParentTID) {
  FEXCore::Core::InternalThreadState* Thread = new FEXCore::Core::InternalThreadState {};

  Thread->CurrentFrame->State.gregs[X86State::REG_RSP] = StackPointer;
  Thread->CurrentFrame->State.rip = InitialRIP;

  // Copy over the new thread state to the new object
  if (NewThreadState) {
    memcpy(&Thread->CurrentFrame->State, NewThreadState, sizeof(FEXCore::Core::CPUState));
  }

  // Set up the thread manager state
  Thread->ThreadManager.parent_tid = ParentTID;
  Thread->CurrentFrame->Thread = Thread;

  InitializeCompiler(Thread);

  Thread->CurrentFrame->State.DeferredSignalRefCount.Store(0);

  if (Config.BlockJITNaming() || Config.GlobalJITNaming() || Config.LibraryJITNaming()) {
    // Allocate a JIT symbol buffer only if enabled.
    Thread->SymbolBuffer = JITSymbols::AllocateBuffer();
  }

  return Thread;
}

void ContextImpl::DestroyThread(FEXCore::Core::InternalThreadState* Thread, bool NeedsTLSUninstall) {
  if (NeedsTLSUninstall) {
#ifndef _WIN32
    Alloc::OSAllocator::UninstallTLSData(Thread);
#endif
  }

  FEXCore::Allocator::VirtualProtect(&Thread->InterruptFaultPage, sizeof(Thread->InterruptFaultPage),
                                     Allocator::ProtectOptions::Read | Allocator::ProtectOptions::Write);
  delete Thread;
}

#ifndef _WIN32
void ContextImpl::UnlockAfterFork(FEXCore::Core::InternalThreadState* LiveThread, bool Child) {
  Allocator::UnlockAfterFork(LiveThread, Child);

  if (Child) {
    CodeInvalidationMutex.StealAndDropActiveLocks();
  } else {
    CodeInvalidationMutex.unlock();
    return;
  }
}

void ContextImpl::LockBeforeFork(FEXCore::Core::InternalThreadState* Thread) {
  CodeInvalidationMutex.lock();
  Allocator::LockBeforeFork(Thread);
}
#endif

void ContextImpl::AddBlockMapping(FEXCore::Core::InternalThreadState* Thread, uint64_t Address, void* Ptr) {
  Thread->LookupCache->AddBlockMapping(Address, Ptr);
}

void ContextImpl::ClearCodeCache(FEXCore::Core::InternalThreadState* Thread) {
  FEXCORE_PROFILE_INSTANT("ClearCodeCache");

  {
    // Ensure the Code Object Serialization service has fully serialized this thread's data before clearing the cache
    // Use the thread's object cache ref counter for this
    CodeSerialize::CodeObjectSerializeService::WaitForEmptyJobQueue(&Thread->ObjectCacheRefCounter);
  }
  std::lock_guard<std::recursive_mutex> lk(Thread->LookupCache->WriteLock);

  Thread->LookupCache->ClearCache();
  Thread->CPUBackend->ClearCache();
}

static void IRDumper(FEXCore::Core::InternalThreadState* Thread, IR::IREmitter* IREmitter, uint64_t GuestRIP, IR::RegisterAllocationData* RA) {
  FEXCore::File::File FD = FEXCore::File::File::GetStdERR();
  fextl::stringstream out;
  auto NewIR = IREmitter->ViewIR();
  FEXCore::IR::Dump(&out, &NewIR, RA);
  fextl::fmt::print(FD, "IR-ShouldDump-{} 0x{:x}:\n{}\n@@@@@\n", RA ? "post" : "pre", GuestRIP, out.str());
};

// IRStorageBase with fully owned memory
struct IRListCopy : public IR::IRStorageBase {
  std::span<std::byte> IRData;
  std::span<std::byte> ListData;

  // TODO: Consider defaulting to empty RAData instead?
  IR::RegisterAllocationData::UniquePtr RADataInternal;

  IRListCopy(const IR::IRListView& view, IR::RegisterAllocationData::UniquePtr RAData)
    : RADataInternal(std::move(RAData)) {
    std::byte* Storage = reinterpret_cast<std::byte*>(FEXCore::Allocator::malloc(view.GetDataSize() + view.GetListSize()));

    IRData = {Storage, Storage + view.GetDataSize()};
    ListData = {Storage + view.GetDataSize(), Storage + view.GetDataSize() + view.GetListSize()};
    memcpy(IRData.data(), (char*)view.GetData(), IRData.size());
    memcpy(ListData.data(), (char*)view.GetListData(), ListData.size());
  }

  IRListCopy(const IRListCopy& other) = delete;
  IRListCopy(IRListCopy&& other) = delete;

  ~IRListCopy() {
    FEXCore::Allocator::free(IRData.data());
  }

  const IR::RegisterAllocationData* RAData() override {
    return RADataInternal.get();
  }
  IR::IRListView GetIRView() override {
    return IR::IRListView {IRData.data(), ListData.data(), IRData.size(), ListData.size()};
  }
};


ContextImpl::GenerateIRResult
ContextImpl::GenerateIR(FEXCore::Core::InternalThreadState* Thread, uint64_t GuestRIP, bool ExtendedDebugInfo, uint64_t MaxInst) {
  FEXCORE_PROFILE_SCOPED("GenerateIR");

  Thread->OpDispatcher->ReownOrClaimBuffer();
  Thread->OpDispatcher->ResetWorkingList();

  uint64_t TotalInstructions {0};
  uint64_t TotalInstructionsLength {0};

  bool HasCustomIR {};

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
    const uint8_t* GuestCode {};
    GuestCode = reinterpret_cast<const uint8_t*>(GuestRIP);

    bool HadDispatchError {false};

    Thread->FrontendDecoder->DecodeInstructionsAtEntry(GuestCode, GuestRIP, MaxInst,
                                                       [Thread](uint64_t BlockEntry, uint64_t Start, uint64_t Length) {
      if (Thread->LookupCache->AddBlockExecutableRange(BlockEntry, Start, Length)) {
        static_cast<ContextImpl*>(Thread->CTX)->SyscallHandler->MarkGuestExecutableRange(Thread, Start, Length);
      }
    });

    auto BlockInfo = Thread->FrontendDecoder->GetDecodedBlockInfo();
    auto CodeBlocks = &BlockInfo->Blocks;

    Thread->OpDispatcher->BeginFunction(GuestRIP, CodeBlocks, BlockInfo->TotalInstructionCount);

    const uint8_t GPRSize = GetGPRSize();

    for (size_t j = 0; j < CodeBlocks->size(); ++j) {
      const FEXCore::Frontend::Decoder::DecodedBlocks& Block = CodeBlocks->at(j);
      // Set the block entry point
      Thread->OpDispatcher->SetNewBlockIfChanged(Block.Entry);

      uint64_t BlockInstructionsLength {};

      // Reset any block-specific state
      Thread->OpDispatcher->StartNewBlock();

      uint64_t InstsInBlock = Block.NumInstructions;

      for (size_t i = 0; i < InstsInBlock; ++i) {
        const FEXCore::X86Tables::X86InstInfo* TableInfo {nullptr};
        const FEXCore::X86Tables::DecodedInst* DecodedInfo {nullptr};

        TableInfo = Block.DecodedInstructions[i].TableInfo;
        DecodedInfo = &Block.DecodedInstructions[i];
        bool IsLocked = DecodedInfo->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_LOCK;

        // Do a partial register cache flush before every instruction. This
        // prevents cross-instruction static register caching, while allowing
        // context load/stores to be optimized within a block. Theoretically,
        // this flush is not required for correctness, all mandatory flushes are
        // included in instruction-specific handlers. Instead, this is a blunt
        // heuristic to make the register cache less aggressive, as the current
        // RA generates bad code in common cases with tied registers otherwise.
        //
        // However, it makes our exception handling behaviour more predictable.
        // It is potentially correctness bearing in that sense, but that is a
        // side effect here and (if that behaviour is required) we should handle
        // that more explicitly later.
        Thread->OpDispatcher->FlushRegisterCache(true);

        if (ExtendedDebugInfo || Thread->OpDispatcher->CanHaveSideEffects(TableInfo, DecodedInfo)) {
          Thread->OpDispatcher->_GuestOpcode(Block.Entry + BlockInstructionsLength - GuestRIP);
        }

        if (Config.SMCChecks == FEXCore::Config::CONFIG_SMC_FULL) {
          auto ExistingCodePtr = reinterpret_cast<uint64_t*>(Block.Entry + BlockInstructionsLength);

          auto CodeChanged = Thread->OpDispatcher->_ValidateCode(ExistingCodePtr[0], ExistingCodePtr[1],
                                                                 (uintptr_t)ExistingCodePtr - GuestRIP, DecodedInfo->InstSize);

          auto InvalidateCodeCond = Thread->OpDispatcher->CondJump(CodeChanged);

          auto CurrentBlock = Thread->OpDispatcher->GetCurrentBlock();
          auto CodeWasChangedBlock = Thread->OpDispatcher->CreateNewCodeBlockAtEnd();
          Thread->OpDispatcher->SetTrueJumpTarget(InvalidateCodeCond, CodeWasChangedBlock);

          Thread->OpDispatcher->SetCurrentCodeBlock(CodeWasChangedBlock);
          Thread->OpDispatcher->_ThreadRemoveCodeEntry();
          Thread->OpDispatcher->ExitFunction(
            Thread->OpDispatcher->_EntrypointOffset(IR::SizeToOpSize(GPRSize), Block.Entry + BlockInstructionsLength - GuestRIP));

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
          } else {
            if (Thread->OpDispatcher->HasHandledLock() != IsLocked) {
              HadDispatchError = true;
              LogMan::Msg::EFmt("Missing LOCK HANDLER at 0x{:x}{{'{}'}}", Block.Entry + BlockInstructionsLength, TableInfo->Name ?: "UND");
            }
            BlockInstructionsLength += DecodedInfo->InstSize;
            TotalInstructionsLength += DecodedInfo->InstSize;
            ++TotalInstructions;
          }
        } else {
          if (TableInfo) {
            LogMan::Msg::EFmt("Invalid or Unknown instruction: {} 0x{:x}", TableInfo->Name ?: "UND", Block.Entry - GuestRIP);
          }
          // Invalid instruction
          Thread->OpDispatcher->InvalidOp(DecodedInfo);
          Thread->OpDispatcher->ExitFunction(Thread->OpDispatcher->_EntrypointOffset(IR::SizeToOpSize(GPRSize), Block.Entry - GuestRIP));
        }

        const bool NeedsBlockEnd =
          (HadDispatchError && TotalInstructions > 0) || (Thread->OpDispatcher->NeedsBlockEnder() && i + 1 == InstsInBlock);

        // If we had a dispatch error then leave early
        if (HadDispatchError && TotalInstructions == 0) {
          // Couldn't handle any instruction in op dispatcher
          Thread->OpDispatcher->ResetWorkingList();
          return {nullptr, 0, 0, 0, 0};
        }

        if (NeedsBlockEnd) {
          const uint8_t GPRSize = GetGPRSize();

          // We had some instructions. Early exit
          Thread->OpDispatcher->ExitFunction(
            Thread->OpDispatcher->_EntrypointOffset(IR::SizeToOpSize(GPRSize), Block.Entry + BlockInstructionsLength - GuestRIP));
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

  IR::IREmitter* IREmitter = Thread->OpDispatcher.get();

  auto ShouldDump = Thread->OpDispatcher->ShouldDumpIR();
  // Debug
  if (ShouldDump) {
    IRDumper(Thread, IREmitter, GuestRIP, nullptr);
  }

  // Run the passmanager over the IR from the dispatcher
  Thread->PassManager->Run(IREmitter);

  // Debug
  if (ShouldDump) {
    IRDumper(Thread, IREmitter, GuestRIP,
             Thread->PassManager->HasPass("RA") ? Thread->PassManager->GetPass<IR::RegisterAllocationPass>("RA")->GetAllocationData() : nullptr);
  }

  auto RAData = Thread->PassManager->HasPass("RA") ? Thread->PassManager->GetPass<IR::RegisterAllocationPass>("RA")->PullAllocationData() : nullptr;
  auto IRList = fextl::make_unique<IRListCopy>(IREmitter->ViewIR(), std::move(RAData));

  IREmitter->DelayedDisownBuffer();

  return {
    .IR = std::move(IRList),
    .TotalInstructions = TotalInstructions,
    .TotalInstructionsLength = TotalInstructionsLength,
    .StartAddr = Thread->FrontendDecoder->DecodedMinAddress,
    .Length = Thread->FrontendDecoder->DecodedMaxAddress - Thread->FrontendDecoder->DecodedMinAddress,
  };
}

ContextImpl::CompileCodeResult ContextImpl::CompileCode(FEXCore::Core::InternalThreadState* Thread, uint64_t GuestRIP, uint64_t MaxInst) {
  // JIT Code object cache lookup
  if (CodeObjectCacheService) {
    auto CodeCacheEntry = CodeObjectCacheService->FetchCodeObjectFromCache(GuestRIP);
    if (CodeCacheEntry) {
      auto CompiledCode = Thread->CPUBackend->RelocateJITObjectCode(GuestRIP, CodeCacheEntry);
      if (CompiledCode) {
        return {
          .CompiledCode = CompiledCode,
          .IR = nullptr,        // No IR/RA data generated
          .DebugData = nullptr, // nullptr here ensures that code serialization doesn't occur on from cache read
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
      AOTIRCacheEntry.Entry->SourcecodeMap = SourcecodeResolver->GenerateMap(AOTIRCacheEntry.Entry->Filename, AOTIRCacheEntry.Entry->FileId);
    }
  }

  fextl::unique_ptr<FEXCore::IR::IRStorageBase> IR;
  FEXCore::Core::DebugData* DebugData {};
  uint64_t StartAddr {};
  uint64_t Length {};

  // AOT IR bookkeeping and cache
  {
    auto IRFromAOT = IRCaptureCache.PreGenerateIRFetch(Thread, GuestRIP);
    if (IRFromAOT) {
      // Setup pointers to internal structures
      IR = std::move(IRFromAOT->IR);
      DebugData = IRFromAOT->DebugData;
      StartAddr = IRFromAOT->StartAddr;
      Length = IRFromAOT->Length;
    }
  }

  if (!IR) {
    // Generate IR + Meta Info
    auto [IRCopy, TotalInstructions, TotalInstructionsLength, _StartAddr, _Length] = GenerateIR(Thread, GuestRIP, Config.GDBSymbols(), MaxInst);

    // Setup pointers to internal structures
    IR = std::move(IRCopy);
    DebugData = new FEXCore::Core::DebugData();
    StartAddr = _StartAddr;
    Length = _Length;
  }

  if (!IR) {
    return {};
  }
  // Attempt to get the CPU backend to compile this code
  auto IRView = IR->GetIRView();
  return {
    // FEX currently throws away the CPUBackend::CompiledCode object other than the entrypoint
    // In the future with code caching getting wired up, we will pass the rest of the data forward.
    // TODO: Pass the data forward when code caching is wired up to this.
    .CompiledCode = Thread->CPUBackend->CompileCode(GuestRIP, &IRView, DebugData, IR->RAData()).BlockEntry,
    .IR = std::move(IR),
    .DebugData = DebugData,
    .GeneratedIR = true,
    .StartAddr = StartAddr,
    .Length = Length,
  };
}

uintptr_t ContextImpl::CompileBlock(FEXCore::Core::CpuStateFrame* Frame, uint64_t GuestRIP, uint64_t MaxInst) {
  FEXCORE_PROFILE_SCOPED("CompileBlock");
  auto Thread = Frame->Thread;

#ifdef _M_ARM_64EC
  // If the target PC is EC code, mark it in the L2 and return straight to the dispatcher
  // so it can handle the call/return.
  if (Thread->LookupCache->CheckPageEC(GuestRIP)) {
    return GuestRIP;
  }
#endif

  // Invalidate might take a unique lock on this, to guarantee that during invalidation no code gets compiled
  auto lk = GuardSignalDeferringSection<std::shared_lock>(CodeInvalidationMutex, Thread);

  // Is the code in the cache?
  // The backends only check L1 and L2, not L3
  if (auto HostCode = Thread->LookupCache->FindBlock(GuestRIP)) {
    return HostCode;
  }

  auto [CodePtr, IR, DebugData, GeneratedIR, StartAddr, Length] = CompileCode(Thread, GuestRIP, MaxInst);
  if (CodePtr == nullptr) {
    return 0;
  }

  // The core managed to compile the code.
  if (Config.BlockJITNaming()) {
    auto FragmentBasePtr = reinterpret_cast<uint8_t*>(CodePtr);

    if (DebugData) {
      auto GuestRIPLookup = SyscallHandler->LookupAOTIRCacheEntry(Thread, GuestRIP);

      if (DebugData->Subblocks.size()) {
        for (auto& Subblock : DebugData->Subblocks) {
          auto BlockBasePtr = FragmentBasePtr + Subblock.HostCodeOffset;
          if (GuestRIPLookup.Entry) {
            Symbols.Register(Thread->SymbolBuffer.get(), BlockBasePtr, DebugData->HostCodeSize, GuestRIPLookup.Entry->Filename,
                             GuestRIP - GuestRIPLookup.VAFileStart);
          } else {
            Symbols.Register(Thread->SymbolBuffer.get(), BlockBasePtr, GuestRIP, Subblock.HostCodeSize);
          }
        }
      } else {
        if (GuestRIPLookup.Entry) {
          Symbols.Register(Thread->SymbolBuffer.get(), FragmentBasePtr, DebugData->HostCodeSize, GuestRIPLookup.Entry->Filename,
                           GuestRIP - GuestRIPLookup.VAFileStart);
        } else {
          Symbols.Register(Thread->SymbolBuffer.get(), FragmentBasePtr, GuestRIP, DebugData->HostCodeSize);
        }
      }
    }
  }

  // Tell the object cache service to serialize the code if enabled
  if (CodeObjectCacheService && Config.CacheObjectCodeCompilation == FEXCore::Config::ConfigObjectCodeHandler::CONFIG_READWRITE && DebugData) {
    CodeObjectCacheService->AsyncAddSerializationJob(
      fextl::make_unique<CodeSerialize::AsyncJobHandler::SerializationJobData>(CodeSerialize::AsyncJobHandler::SerializationJobData {
        .GuestRIP = GuestRIP,
        .GuestCodeLength = Length,
        .GuestCodeHash = 0,
        .HostCodeBegin = CodePtr,
        .HostCodeLength = DebugData->HostCodeSize,
        .HostCodeHash = 0,
        .ThreadJobRefCount = &Thread->ObjectCacheRefCounter,
        .Relocations = std::move(*DebugData->Relocations),
      }));
  }

  // Clear any relocations that might have been generated
  Thread->CPUBackend->ClearRelocations();

  if (IRCaptureCache.PostCompileCode(Thread, CodePtr, GuestRIP, StartAddr, Length, std::move(IR), DebugData, GeneratedIR)) {
    // Early exit
    return (uintptr_t)CodePtr;
  }

  // Insert to lookup cache
  // Pages containing this block are added via AddBlockExecutableRange before each page gets accessed in the frontend
  AddBlockMapping(Thread, GuestRIP, CodePtr);

  return (uintptr_t)CodePtr;
}

void ContextImpl::ExecutionThread(FEXCore::Core::InternalThreadState* Thread) {
  Thread->ExitReason = FEXCore::Context::ExitReason::EXIT_WAITING;

  InitializeThreadTLSData(Thread);

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

#ifndef _WIN32
  Alloc::OSAllocator::UninstallTLSData(Thread);
#endif
}

static void InvalidateGuestThreadCodeRange(FEXCore::Core::InternalThreadState* Thread, uint64_t Start, uint64_t Length) {
  std::lock_guard<std::recursive_mutex> lk(Thread->LookupCache->WriteLock);

  auto lower = Thread->LookupCache->CodePages.lower_bound(Start >> 12);
  auto upper = Thread->LookupCache->CodePages.upper_bound((Start + Length - 1) >> 12);

  for (auto it = lower; it != upper; it++) {
    for (auto Address : it->second) {
      ContextImpl::ThreadRemoveCodeEntry(Thread, Address);
    }
    it->second.clear();
  }
}

void ContextImpl::InvalidateGuestCodeRange(FEXCore::Core::InternalThreadState* Thread, uint64_t Start, uint64_t Length) {
  InvalidateGuestThreadCodeRange(Thread, Start, Length);
}

void ContextImpl::InvalidateGuestCodeRange(FEXCore::Core::InternalThreadState* Thread, uint64_t Start, uint64_t Length,
                                           CodeRangeInvalidationFn CallAfter) {
  InvalidateGuestThreadCodeRange(Thread, Start, Length);
  CallAfter(Start, Length);
}

void ContextImpl::MarkMemoryShared(FEXCore::Core::InternalThreadState* Thread) {
  if (!IsMemoryShared) {
    IsMemoryShared = true;
    UpdateAtomicTSOEmulationConfig();

    if (Config.TSOAutoMigration) {
      // Only the lookup cache is cleared here, so that old code can keep running until next compilation
      std::lock_guard<std::recursive_mutex> lkLookupCache(Thread->LookupCache->WriteLock);
      Thread->LookupCache->ClearCache();
    }
  }
}

void ContextImpl::ThreadAddBlockLink(FEXCore::Core::InternalThreadState* Thread, uint64_t GuestDestination,
                                     FEXCore::Context::ExitFunctionLinkData* HostLink, const FEXCore::Context::BlockDelinkerFunc& delinker) {
  auto lk = GuardSignalDeferringSection<std::shared_lock>(static_cast<ContextImpl*>(Thread->CTX)->CodeInvalidationMutex, Thread);

  Thread->LookupCache->AddBlockLink(GuestDestination, HostLink, delinker);
}

void ContextImpl::ThreadRemoveCodeEntry(FEXCore::Core::InternalThreadState* Thread, uint64_t GuestRIP) {
  LogMan::Throw::AFmt(static_cast<ContextImpl*>(Thread->CTX)->CodeInvalidationMutex.try_lock() == false, "CodeInvalidationMutex needs to "
                                                                                                         "be unique_locked here");

  std::lock_guard<std::recursive_mutex> lk(Thread->LookupCache->WriteLock);

  Thread->LookupCache->Erase(Thread->CurrentFrame, GuestRIP);
}

CustomIRResult ContextImpl::AddCustomIREntrypoint(uintptr_t Entrypoint, CustomIREntrypointHandler Handler, void* Creator, void* Data) {
  LOGMAN_THROW_A_FMT(Config.Is64BitMode || !(Entrypoint >> 32), "64-bit Entrypoint in 32-bit mode {:x}", Entrypoint);

  std::unique_lock lk(CustomIRMutex);

  auto InsertedIterator = CustomIRHandlers.emplace(Entrypoint, std::tuple(Handler, Creator, Data));
  HasCustomIRHandlers = true;

  if (!InsertedIterator.second) {
    const auto& [fn, Creator, Data] = InsertedIterator.first->second;
    return CustomIRResult(std::move(lk), Creator, Data);
  } else {
    lk.unlock();
    return CustomIRResult(std::move(lk), 0, 0);
  }
}

void ContextImpl::RemoveCustomIREntrypoint(uintptr_t Entrypoint) {
  LOGMAN_THROW_A_FMT(Config.Is64BitMode || !(Entrypoint >> 32), "64-bit Entrypoint in 32-bit mode {:x}", Entrypoint);

  std::scoped_lock lk(CustomIRMutex);

  InvalidateGuestCodeRange(nullptr, Entrypoint, 1, [this](uint64_t Entrypoint, uint64_t) { CustomIRHandlers.erase(Entrypoint); });

  HasCustomIRHandlers = !CustomIRHandlers.empty();
}

IR::AOTIRCacheEntry* ContextImpl::LoadAOTIRCacheEntry(const fextl::string& filename) {
  auto rv = IRCaptureCache.LoadAOTIRCacheEntry(filename);
  return rv;
}

void ContextImpl::UnloadAOTIRCacheEntry(IR::AOTIRCacheEntry* Entry) {
  IRCaptureCache.UnloadAOTIRCacheEntry(Entry);
}

void ContextImpl::AppendThunkDefinitions(const fextl::vector<FEXCore::IR::ThunkDefinition>& Definitions) {
  if (ThunkHandler) {
    ThunkHandler->AppendThunkDefinitions(Definitions);
  }
}

void ContextImpl::ConfigureAOTGen(FEXCore::Core::InternalThreadState* Thread, fextl::set<uint64_t>* ExternalBranches, uint64_t SectionMaxAddress) {
  Thread->FrontendDecoder->SetExternalBranches(ExternalBranches);
  Thread->FrontendDecoder->SetSectionMaxAddress(SectionMaxAddress);
}
} // namespace FEXCore::Context
