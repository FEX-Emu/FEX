/*
$info$
category: glue ~ Logic that binds various parts together
meta: glue|driver ~ Emulation mainloop related glue logic
tags: glue|driver
desc: Glues Frontend, OpDispatcher and IR Opts & Compilation, LookupCache, Dispatcher and provides the Execution loop entrypoint
$end_info$
*/

#include "Interface/Context/Context.h"
#include "Interface/Core/LookupCache.h"
#include "Interface/Core/Core.h"
#include "Interface/Core/CPUID.h"
#include "Interface/Core/Frontend.h"
#include "Interface/Core/GdbServer.h"
#include "Interface/Core/ObjectCache/ObjectCacheService.h"
#include "Interface/Core/OpcodeDispatcher.h"
#include "Interface/Core/Interpreter/InterpreterCore.h"
#include "Interface/Core/JIT/JITCore.h"
#include "Interface/HLE/Thunks/Thunks.h"
#include "Interface/IR/Passes/RegisterAllocationPass.h"
#include "Interface/IR/Passes.h"
#include "Interface/IR/PassManager.h"

#include <FEXCore/Config/Config.h>
#include <FEXCore/Core/CodeLoader.h>
#include <FEXCore/Core/Context.h>
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Core/CPUBackend.h>
#include <FEXCore/Core/SignalDelegator.h>
#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/Debug/InternalThreadState.h>
#include <FEXCore/Debug/X86Tables.h>
#include <FEXCore/HLE/SyscallHandler.h>
#include <FEXCore/HLE/Linux/ThreadManagement.h>
#include <FEXCore/IR/IR.h>
#include <FEXCore/IR/IREmitter.h>
#include <FEXCore/IR/IntrusiveIRList.h>
#include <FEXCore/IR/RegisterAllocationData.h>
#include <FEXCore/Utils/Allocator.h>
#include <FEXCore/Utils/Event.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/Threads.h>
#include <FEXHeaderUtils/Syscalls.h>
#include <FEXHeaderUtils/TodoDefines.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <shared_mutex>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <string_view>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <type_traits>
#include <unistd.h>
#include <unordered_map>
#include <utility>
#include <vector>
#include <xxhash.h>

namespace FEXCore::CPU {
  bool CreateCPUCore(FEXCore::Context::Context *CTX) {
    // This should be used for generating things that are shared between threads
    CTX->CPUID.Init(CTX);
    return true;
  }
}

namespace FEXCore::Core {
struct ThreadLocalData {
  FEXCore::Core::InternalThreadState* Thread;
};

thread_local ThreadLocalData ThreadData{};

constexpr std::array<std::string_view const, 22> FlagNames = {
  "CF",
  "",
  "PF",
  "",
  "AF",
  "",
  "ZF",
  "SF",
  "TF",
  "IF",
  "DF",
  "OF",
  "IOPL",
  "",
  "NT",
  "",
  "RF",
  "VM",
  "AC",
  "VIF",
  "VIP",
  "ID",
};

std::string_view const& GetFlagName(unsigned Flag) {
  return FlagNames[Flag];
}

constexpr std::array<std::string_view const, 16> RegNames = {
  "rax",
  "rbx",
  "rcx",
  "rdx",
  "rsi",
  "rdi",
  "rbp",
  "rsp",
  "r8",
  "r9",
  "r10",
  "r11",
  "r12",
  "r13",
  "r14",
  "r15",
};

std::string_view const& GetGRegName(unsigned Reg) {
  return RegNames[Reg];
}
} // namespace FEXCore::Core

namespace FEXCore::Context {
  Context::Context()
  : IRCaptureCache {this} {
#ifdef BLOCKSTATS
    BlockData = std::make_unique<FEXCore::BlockSamplingData>();
#endif
    if (Config.CacheObjectCodeCompilation() != FEXCore::Config::ConfigObjectCodeHandler::CONFIG_NONE) {
      CodeObjectCacheService = std::make_unique<FEXCore::CodeSerialize::CodeObjectSerializeService>(this);
    }
  }

  Context::~Context() {
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

  static FEXCore::Core::CPUState CreateDefaultCPUState() {
    FEXCore::Core::CPUState NewThreadState{};

    // Initialize default CPU state
    NewThreadState.rip = ~0ULL;
    for (int i = 0; i < 16; ++i) {
      NewThreadState.gregs[i] = 0;
    }

    for (int i = 0; i < 16; ++i) {
      NewThreadState.xmm[i][0] = 0xDEADBEEFULL;
      NewThreadState.xmm[i][1] = 0xBAD0DAD1ULL;
    }
    memset(NewThreadState.flags, 0, 32);
    NewThreadState.flags[1] = 1;
    NewThreadState.flags[9] = 1;
    NewThreadState.FCW = 0x37F;
    NewThreadState.FTW = 0xFFFF;
    return NewThreadState;
  }

  FEXCore::Core::InternalThreadState* Context::InitCore(FEXCore::CodeLoader *Loader) {
    // Initialize the CPU core signal handlers
    switch (Config.Core) {
#ifdef INTERPRETER_ENABLED
    case FEXCore::Config::CONFIG_INTERPRETER:
      FEXCore::CPU::InitializeInterpreterSignalHandlers(this);
      break;
#endif
    case FEXCore::Config::CONFIG_IRJIT:
#if (_M_X86_64 && JIT_X86_64)
      FEXCore::CPU::InitializeX86JITSignalHandlers(this);
#elif (_M_ARM_64 && JIT_ARM64)
      FEXCore::CPU::InitializeArm64JITSignalHandlers(this);
#else
      ERROR_AND_DIE_FMT("FEXCore has been compiled without a viable JIT core");
#endif
      break;
    case FEXCore::Config::CONFIG_CUSTOM:
      // Do nothing
      break;
    default:
      ERROR_AND_DIE_FMT("Unknown core configuration");
      break;
    }

    // Initialize GDBServer after the signal handlers are installed
    // It may install its own handlers that need to be executed AFTER the CPU cores
    if (Config.GdbServer) {
      StartGdbServer();
    }
    else {
      StopGdbServer();
    }

    ThunkHandler.reset(FEXCore::ThunkHandler::Create());

    LocalLoader = Loader;
    using namespace FEXCore::Core;

    FEXCore::Core::CPUState NewThreadState = CreateDefaultCPUState();
    FEXCore::Core::InternalThreadState *Thread = CreateThread(&NewThreadState, 0);

    // We are the parent thread
    ParentThread = Thread;

    Thread->CurrentFrame->State.gregs[X86State::REG_RSP] = Loader->GetStackPointer();

    Thread->CurrentFrame->State.rip = StartingRIP = Loader->DefaultRIP();

    InitializeThreadData(Thread);
    return Thread;
  }

  void Context::StartGdbServer() {
    if (!DebugServer) {
      DebugServer = std::make_unique<GdbServer>(this);
      StartPaused = true;
    }
  }

  void Context::StopGdbServer() {
    DebugServer.reset();
  }

  void Context::HandleCallback(FEXCore::Core::InternalThreadState *Thread, uint64_t RIP) {
    Thread->CPUBackend->CallbackPtr(Thread->CurrentFrame, RIP);
  }

  void Context::RegisterHostSignalHandler(int Signal, HostSignalDelegatorFunction Func, bool Required) {
      SignalDelegation->RegisterHostSignalHandler(Signal, Func, Required);
  }

  void Context::RegisterFrontendHostSignalHandler(int Signal, HostSignalDelegatorFunction Func, bool Required) {
    SignalDelegation->RegisterFrontendHostSignalHandler(Signal, Func, Required);
  }

  void Context::WaitForIdle() {
    std::unique_lock<std::mutex> lk(IdleWaitMutex);
    IdleWaitCV.wait(lk, [this] {
      return IdleWaitRefCount.load() == 0;
    });

    Running = false;
  }

  void Context::WaitForIdleWithTimeout() {
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

  void Context::NotifyPause() {

    // Tell all the threads that they should pause
    std::lock_guard<std::mutex> lk(ThreadCreationMutex);
    for (auto &Thread : Threads) {
      Thread->SignalReason.store(FEXCore::Core::SignalEvent::Pause);
      if (Thread->RunningEvents.Running.load()) {
        // Only attempt to stop this thread if it is running
        FHU::Syscalls::tgkill(Thread->ThreadManager.PID, Thread->ThreadManager.TID, SignalDelegator::SIGNAL_FOR_PAUSE);
      }
    }
  }

  void Context::Pause() {
    // If we aren't running, WaitForIdle will never compete.
    if (Running) {
      NotifyPause();

      WaitForIdle();
    }
  }

  void Context::Run() {
    // Spin up all the threads
    std::lock_guard<std::mutex> lk(ThreadCreationMutex);
    for (auto &Thread : Threads) {
      Thread->SignalReason.store(FEXCore::Core::SignalEvent::Return);
    }

    for (auto &Thread : Threads) {
      Thread->StartRunning.NotifyAll();
    }
  }

  void Context::WaitForThreadsToRun() {
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

  void Context::Step() {
    {
      std::lock_guard<std::mutex> lk(ThreadCreationMutex);
      // Walk the threads and tell them to clear their caches
      // Useful when our block size is set to a large number and we need to step a single instruction
      for (auto &Thread : Threads) {
        // Wait for thread to be fully constructed
        // XXX: Look into thread partial construction issues
        while(Thread->RunningEvents.WaitingToStart.load()) ;
        ClearCodeCache(Thread, true);
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

  void Context::Stop(bool IgnoreCurrentThread) {
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

  void Context::StopThread(FEXCore::Core::InternalThreadState *Thread) {
    if (Thread->RunningEvents.Running.exchange(false)) {
      Thread->SignalReason.store(FEXCore::Core::SignalEvent::Stop);
      FHU::Syscalls::tgkill(Thread->ThreadManager.PID, Thread->ThreadManager.TID, SignalDelegator::SIGNAL_FOR_PAUSE);
    }
  }

  void Context::SignalThread(FEXCore::Core::InternalThreadState *Thread, FEXCore::Core::SignalEvent Event) {
    if (Thread->RunningEvents.Running.load()) {
      Thread->SignalReason.store(Event);
      FHU::Syscalls::tgkill(Thread->ThreadManager.PID, Thread->ThreadManager.TID, SignalDelegator::SIGNAL_FOR_PAUSE);
    }
  }

  FEXCore::Context::ExitReason Context::RunUntilExit() {
    if(!StartPaused) {
      // We will only have one thread at this point, but just in case run notify everything
      std::lock_guard lk(ThreadCreationMutex);
      for (auto &Thread : Threads) {
        Thread->StartRunning.NotifyAll();
      }
    }

    ExecutionThread(ParentThread);
    while(true) {
      this->WaitForIdle();
      auto reason = ParentThread->ExitReason;

      // Don't return if a custom exit handling the exit
      if (!CustomExitHandler || reason == ExitReason::EXIT_SHUTDOWN) {
        return reason;
      }
    }
  }

  int Context::GetProgramStatus() const {
    return ParentThread->StatusCode;
  }

  void Context::InitializeThreadData(FEXCore::Core::InternalThreadState *Thread) {
    Thread->CPUBackend->Initialize();

    auto IRHandler = [Thread](uint64_t Addr, IR::IREmitter *IR) -> void {
      // Run the passmanager over the IR from the dispatcher
      Thread->PassManager->Run(IR);
      Core::LocalIREntry Entry = {Addr, 0ULL,
        decltype(Entry.IR)(IR->CreateIRCopy()),
        decltype(Entry.RAData)(Thread->PassManager->HasPass("RA")
          ? Thread->PassManager->GetPass<IR::RegisterAllocationPass>("RA")->PullAllocationData()
          : nullptr),
        decltype(Entry.DebugData)(new Core::DebugData())
      };
      std::lock_guard<std::recursive_mutex> lk(Thread->LookupCache->WriteLock);
      Thread->LocalIRCache.insert({Addr, std::move(Entry)});
    };

    LocalLoader->AddIR(IRHandler);
  }

  struct ExecutionThreadHandler {
    FEXCore::Context::Context *This;
    FEXCore::Core::InternalThreadState *Thread;
  };

  static void *ThreadHandler(void* Data) {
    ExecutionThreadHandler *Handler = reinterpret_cast<ExecutionThreadHandler*>(Data);
    Handler->This->ExecutionThread(Handler->Thread);
    FEXCore::Allocator::free(Handler);
    return nullptr;
  }

  void Context::InitializeThread(FEXCore::Core::InternalThreadState *Thread) {
    // This will create the execution thread but it won't actually start executing
    ExecutionThreadHandler *Arg = reinterpret_cast<ExecutionThreadHandler*>(FEXCore::Allocator::malloc(sizeof(ExecutionThreadHandler)));
    Arg->This = this;
    Arg->Thread = Thread;
    Thread->ExecutionThread = FEXCore::Threads::Thread::Create(ThreadHandler, Arg);

    // Wait for the thread to have started
    Thread->ThreadWaiting.Wait();
  }

  void Context::InitializeThreadTLSData(FEXCore::Core::InternalThreadState *Thread) {
    // Let's do some initial bookkeeping here
    Thread->ThreadManager.TID = FHU::Syscalls::gettid();
    Thread->ThreadManager.PID = ::getpid();
    SignalDelegation->RegisterTLSState(Thread);
    ThunkHandler->RegisterTLSState(Thread);
  }

  void Context::RunThread(FEXCore::Core::InternalThreadState *Thread) {
    // Tell the thread to start executing
    Thread->StartRunning.NotifyAll();
  }

  void Context::InitializeCompiler(FEXCore::Core::InternalThreadState* State) {
    State->OpDispatcher = std::make_unique<FEXCore::IR::OpDispatchBuilder>(this);
    State->OpDispatcher->SetMultiblock(Config.Multiblock);
    State->LookupCache = std::make_unique<FEXCore::LookupCache>(this);
    State->FrontendDecoder = std::make_unique<FEXCore::Frontend::Decoder>(this);
    State->PassManager = std::make_unique<FEXCore::IR::PassManager>();
    State->PassManager->RegisterExitHandler([this]() {
        Stop(false /* Ignore current thread */);
    });

    State->CTX = this;

    #if _M_ARM_64
    bool DoSRA = State->CTX->Config.StaticRegisterAllocation;
    #else
    bool DoSRA = false;
    #endif

    State->PassManager->AddDefaultPasses(this, Config.Core == FEXCore::Config::CONFIG_IRJIT, DoSRA);
    State->PassManager->AddDefaultValidationPasses();

    State->PassManager->RegisterSyscallHandler(SyscallHandler);

    // Create CPU backend
    switch (Config.Core) {
#ifdef INTERPRETER_ENABLED
    case FEXCore::Config::CONFIG_INTERPRETER:
      State->CPUBackend = FEXCore::CPU::CreateInterpreterCore(this, State);
      break;
#endif
    case FEXCore::Config::CONFIG_IRJIT:
      State->PassManager->InsertRegisterAllocationPass(DoSRA);

#if (_M_X86_64 && JIT_X86_64)
      State->CPUBackend = FEXCore::CPU::CreateX86JITCore(this, State);
#elif (_M_ARM_64 && JIT_ARM64)
      State->CPUBackend = FEXCore::CPU::CreateArm64JITCore(this, State);
#else
      ERROR_AND_DIE_FMT("FEXCore has been compiled without a viable JIT core");
#endif

      break;
    case FEXCore::Config::CONFIG_CUSTOM:
      State->CPUBackend = CustomCPUFactory(this, State);
      break;
    default:
      ERROR_AND_DIE_FMT("Unknown core configuration");
      break;
    }
  }

  FEXCore::Core::InternalThreadState* Context::CreateThread(FEXCore::Core::CPUState *NewThreadState, uint64_t ParentTID) {
    FEXCore::Core::InternalThreadState *Thread{};

    // Grab the new thread object
    {
      std::lock_guard<std::mutex> lk(ThreadCreationMutex);
      Thread = Threads.emplace_back(new FEXCore::Core::InternalThreadState{});
      Thread->ThreadManager.TID = ++ThreadID;
    }

    // Copy over the new thread state to the new object
    memcpy(Thread->CurrentFrame, NewThreadState, sizeof(FEXCore::Core::CPUState));
    Thread->CurrentFrame->Thread = Thread;

    // Set up the thread manager state
    Thread->ThreadManager.parent_tid = ParentTID;

    InitializeCompiler(Thread);
    InitializeThreadData(Thread);

    return Thread;
  }

  void Context::DestroyThread(FEXCore::Core::InternalThreadState *Thread) {
    // remove new thread object
    {
      std::lock_guard<std::mutex> lk(ThreadCreationMutex);

      auto It = std::find(Threads.begin(), Threads.end(), Thread);
      LOGMAN_THROW_A_FMT(It != Threads.end(), "Thread wasn't in Threads");

      Threads.erase(It);
    }

    if (Thread->ExecutionThread &&
        Thread->ExecutionThread->IsSelf()) {
      // To be able to delete a thread from itself, we need to detached the std::thread object
      Thread->ExecutionThread->detach();
    }
    delete Thread;
  }

  void Context::CleanupAfterFork(FEXCore::Core::InternalThreadState *LiveThread) {
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

  void Context::AddBlockMapping(FEXCore::Core::InternalThreadState *Thread, uint64_t Address, void *Ptr, uint64_t Start, uint64_t Length) {
    // Only call MarkGuestExecutableRange if new pages are marked as containing code
    if (Thread->LookupCache->AddBlockMapping(Address, Ptr, Start, Length)) {
      Thread->CTX->SyscallHandler->MarkGuestExecutableRange(Start, Length);
    }
  }

  void Context::ClearCodeCache(FEXCore::Core::InternalThreadState *Thread, bool AlsoClearIRCache) {
    {
      // Ensure the Code Object Serialization service has fully serialized this thread's data before clearing the cache
      // Use the thread's object cache ref counter for this
      CodeSerialize::CodeObjectSerializeService::WaitForEmptyJobQueue(&Thread->ObjectCacheRefCounter);
    }
    std::lock_guard<std::recursive_mutex> lk(Thread->LookupCache->WriteLock);

    Thread->LookupCache->ClearCache();
    Thread->CPUBackend->ClearCache();

    if (AlsoClearIRCache) {
      Thread->LocalIRCache.clear();
    }
  }

  static void IRDumper(FEXCore::Core::InternalThreadState *Thread, uint64_t GuestRIP, IR::RegisterAllocationData* RA) {
    FILE* f = nullptr;
    bool CloseAfter = false;
    const auto DumpIRStr = Thread->CTX->Config.DumpIR();

    if (DumpIRStr =="stderr") {
      f = stderr;
    }
    else if (DumpIRStr =="stdout") {
      f = stdout;
    }
    else {
      const auto fileName = fmt::format("{}/{:x}{}", DumpIRStr, GuestRIP, RA ? "-post.ir" : "-pre.ir");
      f = fopen(fileName.c_str(), "w");
      CloseAfter = true;
    }

    if (f) {
      std::stringstream out;
      auto NewIR = Thread->OpDispatcher->ViewIR();
      FEXCore::IR::Dump(&out, &NewIR, RA);
      fmt::print(f,"IR-{} 0x{:x}:\n{}\n@@@@@\n", RA ? "post" : "pre", GuestRIP, out.str());

      if (CloseAfter) {
        fclose(f);
      }
    }
  };

  static void ValidateIR(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread) {
    // Convert to text, Parse, Convert to text again and make sure the texts match
    std::stringstream out;
    static auto compaction = IR::CreateIRCompaction(ctx->OpDispatcherAllocator);
    compaction->Run(Thread->OpDispatcher.get());
    auto NewIR = Thread->OpDispatcher->ViewIR();
    Dump(&out, &NewIR, nullptr);
    out.seekg(0);
    FEXCore::Utils::PooledAllocatorMalloc Allocator;
    auto reparsed = IR::Parse(Allocator, &out);
    if (reparsed == nullptr) {
      LOGMAN_MSG_A_FMT("Failed to parse IR\n");
    } else {
      std::stringstream out2;
      auto NewIR2 = reparsed->ViewIR();
      Dump(&out2, &NewIR2, nullptr);
      if (out.str() != out2.str()) {
        LogMan::Msg::IFmt("one:\n {}", out.str());
        LogMan::Msg::IFmt("two:\n {}", out2.str());
        LOGMAN_MSG_A_FMT("Parsed IR doesn't match\n");
      }
    }
  }

  Context::GenerateIRResult Context::GenerateIR(FEXCore::Core::InternalThreadState *Thread, uint64_t GuestRIP) {
    uint8_t const *GuestCode{};
    GuestCode = reinterpret_cast<uint8_t const*>(GuestRIP);

    bool HadDispatchError {false};

    uint64_t TotalInstructions {0};
    uint64_t TotalInstructionsLength {0};

    Thread->FrontendDecoder->DecodeInstructionsAtEntry(GuestCode, GuestRIP);

    auto CodeBlocks = Thread->FrontendDecoder->GetDecodedBlocks();

    Thread->OpDispatcher->ReownOrClaimBuffer();
    Thread->OpDispatcher->ResetWorkingList();
    Thread->OpDispatcher->BeginFunction(GuestRIP, CodeBlocks);

    const uint8_t GPRSize = GetGPRSize();

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

        if (Config.SMCChecks == FEXCore::Config::CONFIG_SMC_FULL) {
          auto ExistingCodePtr = reinterpret_cast<uint64_t*>(Block.Entry + BlockInstructionsLength);

          auto CodeChanged = Thread->OpDispatcher->_ValidateCode(ExistingCodePtr[0], ExistingCodePtr[1], (uintptr_t)ExistingCodePtr - GuestRIP, DecodedInfo->InstSize);

          auto InvalidateCodeCond = Thread->OpDispatcher->_CondJump(CodeChanged);

          auto CurrentBlock = Thread->OpDispatcher->GetCurrentBlock();
          auto CodeWasChangedBlock = Thread->OpDispatcher->CreateNewCodeBlockAtEnd();
          Thread->OpDispatcher->SetTrueJumpTarget(InvalidateCodeCond, CodeWasChangedBlock);

          Thread->OpDispatcher->SetCurrentCodeBlock(CodeWasChangedBlock);
          Thread->OpDispatcher->_RemoveThreadCodeEntry();
          Thread->OpDispatcher->_ExitFunction(Thread->OpDispatcher->_EntrypointOffset(Block.Entry + BlockInstructionsLength - GuestRIP, GPRSize));

          auto NextOpBlock = Thread->OpDispatcher->CreateNewCodeBlockAfter(CurrentBlock);

          Thread->OpDispatcher->SetFalseJumpTarget(InvalidateCodeCond, NextOpBlock);
          Thread->OpDispatcher->SetCurrentCodeBlock(NextOpBlock);
        }

        if (TableInfo && TableInfo->OpcodeDispatcher) {
          auto Fn = TableInfo->OpcodeDispatcher;
          Thread->OpDispatcher->HandledLock = false;
          Thread->OpDispatcher->ResetDecodeFailure();
          std::invoke(Fn, Thread->OpDispatcher, DecodedInfo);
          if (Thread->OpDispatcher->HadDecodeFailure()) {
            HadDispatchError = true;
          }
          else {
            if (Thread->OpDispatcher->HandledLock != IsLocked) {
              HadDispatchError = true;
              LogMan::Msg::EFmt("Missing LOCK HANDLER at 0x{:x}{{'{}'}}", Block.Entry + BlockInstructionsLength, TableInfo->Name ?: "UND");
            }
            BlockInstructionsLength += DecodedInfo->InstSize;
            TotalInstructionsLength += DecodedInfo->InstSize;
            ++TotalInstructions;
          }
        }
        else {
          // Invalid instruction
          Thread->OpDispatcher->InvalidOp(DecodedInfo);
          Thread->OpDispatcher->_ExitFunction(Thread->OpDispatcher->_EntrypointOffset(Block.Entry - GuestRIP, GPRSize));
        }

        // If we had a dispatch error then leave early
        if (HadDispatchError) {
          if (TotalInstructions == 0) {
            // Couldn't handle any instruction in op dispatcher
            Thread->OpDispatcher->ResetWorkingList();
            return { nullptr, nullptr, 0, 0, 0, 0 };
          }
          else {
            const uint8_t GPRSize = GetGPRSize();

            // We had some instructions. Early exit
            Thread->OpDispatcher->_ExitFunction(Thread->OpDispatcher->_EntrypointOffset(Block.Entry + BlockInstructionsLength - GuestRIP, GPRSize));
            break;
          }
        }

        if (Thread->OpDispatcher->FinishOp(DecodedInfo->PC + DecodedInfo->InstSize, i + 1 == InstsInBlock)) {
          break;
        }
      }
    }

    Thread->OpDispatcher->Finalize();

    // Debug
    {
      if (Thread->CTX->Config.DumpIR() != "no") {
        IRDumper(Thread, GuestRIP, nullptr);
      }

      if (Thread->CTX->Config.ValidateIRarser) {
        ValidateIR(this, Thread);
      }
    }

    // Run the passmanager over the IR from the dispatcher
    Thread->PassManager->Run(Thread->OpDispatcher.get());

    // Debug
    {
      if (Thread->CTX->Config.DumpIR() != "no") {
        IRDumper(Thread, GuestRIP, Thread->PassManager->HasPass("RA") ? Thread->PassManager->GetPass<IR::RegisterAllocationPass>("RA")->GetAllocationData() : nullptr);
      }

      if (Thread->OpDispatcher->ShouldDump) {
        std::stringstream out;
        auto NewIR = Thread->OpDispatcher->ViewIR();
        FEXCore::IR::Dump(&out, &NewIR, Thread->PassManager->HasPass("RA") ? Thread->PassManager->GetPass<IR::RegisterAllocationPass>("RA")->GetAllocationData() : nullptr);
        LogMan::Msg::IFmt("IR 0x{:x}:\n{}\n@@@@@\n", GuestRIP, out.str());
      }
    }

    auto RAData = Thread->PassManager->HasPass("RA") ? Thread->PassManager->GetPass<IR::RegisterAllocationPass>("RA")->PullAllocationData() : nullptr;
    auto IRList = Thread->OpDispatcher->CreateIRCopy();

    Thread->OpDispatcher->DelayedDisownBuffer();
    Thread->FrontendDecoder->DelayedDisownBuffer();

    return {
      .IRList = IRList,
      .RAData = RAData.release(),
      .TotalInstructions = TotalInstructions,
      .TotalInstructionsLength = TotalInstructionsLength,
      .StartAddr = Thread->FrontendDecoder->DecodedMinAddress,
      .Length = Thread->FrontendDecoder->DecodedMaxAddress - Thread->FrontendDecoder->DecodedMinAddress,
    };
  }

  Context::CompileCodeResult Context::CompileCode(FEXCore::Core::InternalThreadState *Thread, uint64_t GuestRIP) {
    FEXCore::IR::IRListView *IRList {};
    FEXCore::Core::DebugData *DebugData {};
    FEXCore::IR::RegisterAllocationData *RAData {};
    bool GeneratedIR {};
    uint64_t StartAddr {};
    uint64_t Length {};

    std::lock_guard<std::recursive_mutex> lk(Thread->LookupCache->WriteLock);
    // Do we already have this in the IR cache?
    auto LocalEntry = Thread->LocalIRCache.find(GuestRIP);

    if (LocalEntry != Thread->LocalIRCache.end()) {
      // Entry already exists
      // pull in the data
      IRList = LocalEntry->second.IR.get();
      DebugData = LocalEntry->second.DebugData.get();
      RAData = LocalEntry->second.RAData.get();
      StartAddr = LocalEntry->second.StartAddr;
      Length = LocalEntry->second.Length;

      GeneratedIR = false;
    }

    // JIT Code object cache lookup
    if (CodeObjectCacheService) {
      auto CodeCacheEntry = CodeObjectCacheService->FetchCodeObjectFromCache(GuestRIP);
      if (CodeCacheEntry) {
        auto CompiledCode = Thread->CPUBackend->RelocateJITObjectCode(GuestRIP, CodeCacheEntry);
        if (CompiledCode) {
          return {
            .CompiledCode = CompiledCode,
            .IRData      = nullptr, // No IR data generated
            .DebugData   = nullptr, // nullptr here ensures that code serialization doesn't occur on from cache read
            .RAData      = nullptr, // No RA data generated
            .GeneratedIR = false, // nullptr here ensures IR cache mechanisms won't run
            .StartAddr   = 0, // Unused
            .Length      = 0, // Unused
          };
        }
      }
    }

    // AOT IR bookkeeping and cache
    {
      auto [IRCopy, RACopy, DebugDataCopy, _StartAddr, _Length, _GeneratedIR] = IRCaptureCache.PreGenerateIRFetch(GuestRIP, IRList);
      if (_GeneratedIR) {
        // Setup pointers to internal structures
        IRList = IRCopy;
        RAData = RACopy;
        DebugData = DebugDataCopy;
        StartAddr = _StartAddr;
        Length = _Length;
        GeneratedIR = _GeneratedIR;
      }
    }

    if (IRList == nullptr) {
      // Generate IR + Meta Info
      auto [IRCopy, RACopy, TotalInstructions, TotalInstructionsLength, _StartAddr, _Length] = GenerateIR(Thread, GuestRIP);

      // Setup pointers to internal structures
      IRList = IRCopy;
      RAData = RACopy;
      DebugData = new FEXCore::Core::DebugData();
      StartAddr = _StartAddr;
      Length = _Length;

      // Increment stats
      Thread->Stats.BlocksCompiled.fetch_add(1);

      // These blocks aren't already in the cache
      GeneratedIR = true;
    }

    if (IRList == nullptr) {
      return {};
    }
    // Attempt to get the CPU backend to compile this code
    return {
      .CompiledCode = Thread->CPUBackend->CompileCode(GuestRIP, IRList, DebugData, RAData),
      .IRData = IRList,
      .DebugData = DebugData,
      .RAData = RAData,
      .GeneratedIR = GeneratedIR,
      .StartAddr = StartAddr,
      .Length = Length,
    };
  }

  void Context::CompileBlockJit(FEXCore::Core::CpuStateFrame *Frame, uint64_t GuestRIP) {
    auto NewBlock = CompileBlock(Frame, GuestRIP);

    if (NewBlock == 0) {
      LogMan::Msg::EFmt("CompileBlockJit: Failed to compile code {:X} - aborting process", GuestRIP);
      // Return similar behaviour of SIGILL abort
      Frame->Thread->StatusCode = 128 + SIGILL;
      Stop(false /* Ignore current thread */);
    }
  }

  uintptr_t Context::CompileBlock(FEXCore::Core::CpuStateFrame *Frame, uint64_t GuestRIP) {
    auto Thread = Frame->Thread;

    // Is the code in the cache?
    // The backends only check L1 and L2, not L3
    if (auto HostCode = Thread->LookupCache->FindBlock(GuestRIP)) {
      return HostCode;
    }

    void *CodePtr {};
    FEXCore::IR::IRListView *IRList {};
    FEXCore::Core::DebugData *DebugData {};
    FEXCore::IR::RegisterAllocationData *RAData {};

    bool GeneratedIR {};
    uint64_t StartAddr {}, Length {};

    auto [Code, IR, Data, RA, Generated, _StartAddr, _Length] = CompileCode(Thread, GuestRIP);
    CodePtr = Code;
    IRList = IR;
    DebugData = Data;
    RAData = RA;
    GeneratedIR = Generated;
    StartAddr = _StartAddr;
    Length = _Length;

    if (CodePtr == nullptr) {
      return 0;
    }

    // The core managed to compile the code.
    if (Config.BlockJITNaming()) {
      if (DebugData) {
        auto GuestRIPLookup = this->SyscallHandler->LookupAOTIRCacheEntry(GuestRIP);

        if (DebugData->Subblocks.size()) {
          for (auto& Subblock: DebugData->Subblocks) {
            if (GuestRIPLookup.Entry) {
              Symbols.Register(CodePtr, DebugData->HostCodeSize, GuestRIPLookup.Entry->Filename, GuestRIP - GuestRIPLookup.Offset);
            } else {
            Symbols.Register((void*)Subblock.HostCodeStart, GuestRIP, Subblock.HostCodeSize);
          }
          }
        } else {
          if (GuestRIPLookup.Entry) {
            Symbols.Register(CodePtr, DebugData->HostCodeSize, GuestRIPLookup.Entry->Filename, GuestRIP - GuestRIPLookup.Offset);
        } else {
          Symbols.Register(CodePtr, GuestRIP, DebugData->HostCodeSize);
          }
        }
      }
    }

    // Tell the object cache service to serialize the code if enabled
    if (CodeObjectCacheService &&
        Config.CacheObjectCodeCompilation == FEXCore::Config::ConfigObjectCodeHandler::CONFIG_READWRITE &&
        DebugData) {
      CodeObjectCacheService->AsyncAddSerializationJob(std::make_unique<CodeSerialize::AsyncJobHandler::SerializationJobData>(
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
        RAData,
        IRList,
        DebugData,
        GeneratedIR)) {
      // Early exit
      return (uintptr_t)CodePtr;
    }

    // Insert to lookup cache
    AddBlockMapping(Thread, GuestRIP, CodePtr, StartAddr, Length);

    return (uintptr_t)CodePtr;
  }

  void Context::ExecutionThread(FEXCore::Core::InternalThreadState *Thread) {
    Core::ThreadData.Thread = Thread;
    Thread->ExitReason = FEXCore::Context::ExitReason::EXIT_WAITING;

    InitializeThreadTLSData(Thread);

    ++IdleWaitRefCount;

    // Now notify the thread that we are initialized
    Thread->ThreadWaiting.NotifyAll();

    if (Thread != Thread->CTX->ParentThread || StartPaused) {
      // Parent thread doesn't need to wait to run
      Thread->StartRunning.Wait();
    }

    if (!Thread->RunningEvents.EarlyExit.load()) {
      Thread->RunningEvents.WaitingToStart = false;

      Thread->ExitReason = FEXCore::Context::ExitReason::EXIT_NONE;

      Thread->RunningEvents.Running = true;

      Thread->CPUBackend->ExecuteDispatch(Thread->CurrentFrame);

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

    SignalDelegation->UninstallTLSState(Thread);

    // If the parent thread is waiting to join, then we can't destroy our thread object
    if (!Thread->DestroyedByParent && Thread != Thread->CTX->ParentThread) {
      Thread->CTX->DestroyThread(Thread);
    }
  }

  static void InvalidateGuestThreadCodeRange(FEXCore::Core::InternalThreadState *Thread, uint64_t Start, uint64_t Length) {
    std::lock_guard<std::recursive_mutex> lk(Thread->LookupCache->WriteLock);

    auto lower = Thread->LookupCache->CodePages.lower_bound(Start >> 12);
    auto upper = Thread->LookupCache->CodePages.upper_bound((Start + Length - 1) >> 12);

    for (auto it = lower; it != upper; it++) {
      for (auto Address: it->second) {
        Context::RemoveThreadCodeEntry(Thread, Address);
      }
      it->second.clear();
    }
  }

  void InvalidateGuestCodeRange(FEXCore::Context::Context *CTX, uint64_t Start, uint64_t Length) {
    std::lock_guard<std::mutex> lk(CTX->ThreadCreationMutex);
    for (auto &Thread : CTX->Threads) {
      if (Thread->RunningEvents.Running.load()) {
        InvalidateGuestThreadCodeRange(Thread, Start, Length);
      }
    }
  }

  void Context::RemoveThreadCodeEntry(FEXCore::Core::InternalThreadState *Thread, uint64_t GuestRIP) {
    std::lock_guard<std::recursive_mutex> lk(Thread->LookupCache->WriteLock);

    Thread->LocalIRCache.erase(GuestRIP);
    Thread->LookupCache->Erase(GuestRIP);
  }

  // Debug interface
  void Context::CompileRIP(FEXCore::Core::InternalThreadState *Thread, uint64_t RIP) {
    uint64_t RIPBackup = Thread->CurrentFrame->State.rip;
    Thread->CurrentFrame->State.rip = RIP;

    // Erase the RIP from all the storage backings if it exists
    RemoveThreadCodeEntry(Thread, RIP);

    // We don't care if compilation passes or not
    CompileBlock(Thread->CurrentFrame, RIP);

    Thread->CurrentFrame->State.rip = RIPBackup;
  }

  uint64_t Context::GetThreadCount() const {
    return Threads.size();
  }

  FEXCore::Core::RuntimeStats *Context::GetRuntimeStatsForThread(uint64_t Thread) {
    return &Threads[Thread]->Stats;
  }

  bool Context::GetDebugDataForRIP(uint64_t RIP, FEXCore::Core::DebugData *Data) {
    std::lock_guard<std::recursive_mutex> lk(ParentThread->LookupCache->WriteLock);
    auto it = ParentThread->LocalIRCache.find(RIP);
    if (it == ParentThread->LocalIRCache.end()) {
      return false;
    }

    memcpy(Data, it->second.DebugData.get(), sizeof(FEXCore::Core::DebugData));
    return true;
  }

  bool Context::FindHostCodeForRIP(uint64_t RIP, uint8_t **Code) {
    uintptr_t HostCode = ParentThread->LookupCache->FindBlock(RIP);
    if (!HostCode) {
      return false;
    }

    *Code = reinterpret_cast<uint8_t*>(HostCode);
    return true;
  }

  uint64_t HandleSyscall(FEXCore::HLE::SyscallHandler *Handler, FEXCore::Core::CpuStateFrame *Frame, FEXCore::HLE::SyscallArguments *Args) {
    uint64_t Result{};
    Result = Handler->HandleSyscall(Frame, Args);
    return Result;
  }

  IR::AOTIRCacheEntry *Context::LoadAOTIRCacheEntry(const std::string &filename) {
    auto rv = IRCaptureCache.LoadAOTIRCacheEntry(filename);
    if (DebugServer) {
      DebugServer->AlertLibrariesChanged();
    }
    return rv;
  }

  void Context::UnloadAOTIRCacheEntry(IR::AOTIRCacheEntry *Entry) {
    IRCaptureCache.UnloadAOTIRCacheEntry(Entry);
    if (DebugServer) {
      DebugServer->AlertLibrariesChanged();
    }
  }

  void ConfigureAOTGen(FEXCore::Core::InternalThreadState *Thread, std::set<uint64_t> *ExternalBranches, uint64_t SectionMaxAddress) {
    Thread->FrontendDecoder->SetExternalBranches(ExternalBranches);
    Thread->FrontendDecoder->SetSectionMaxAddress(SectionMaxAddress);
  }
}
