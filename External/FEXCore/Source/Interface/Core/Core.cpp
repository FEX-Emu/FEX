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
#include "Interface/Core/LookupCache.h"
#include "Interface/Core/Core.h"
#include "Interface/Core/CPUID.h"
#include "Interface/Core/Frontend.h"
#include "Interface/Core/GdbServer.h"
#include "Interface/Core/ObjectCache/ObjectCacheService.h"
#include "Interface/Core/OpcodeDispatcher.h"
#include "Interface/Core/Interpreter/InterpreterCore.h"
#include "Interface/Core/JIT/JITCore.h"
#include "Interface/Core/Dispatcher/Dispatcher.h"
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
#include <FEXCore/HLE/SourcecodeResolver.h>
#include <FEXCore/HLE/Linux/ThreadManagement.h>
#include <FEXCore/IR/IR.h>
#include <FEXCore/IR/IREmitter.h>
#include <FEXCore/IR/IntrusiveIRList.h>
#include <FEXCore/IR/RegisterAllocationData.h>
#include <FEXCore/Utils/Allocator.h>
#include <FEXCore/Utils/Event.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/Threads.h>
#include <FEXCore/Utils/Profiler.h>
#include <FEXHeaderUtils/Syscalls.h>
#include <FEXHeaderUtils/TodoDefines.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
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
    if (!Config.EnableAVX) {
      HostFeatures.SupportsAVX = false;
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
    for (auto& greg : NewThreadState.gregs) {
      greg = 0;
    }

    for (auto& xmm : NewThreadState.xmm.avx.data) {
      xmm[0] = 0xDEADBEEFULL;
      xmm[1] = 0xBAD0DAD1ULL;
      xmm[2] = 0xDEADCAFEULL;
      xmm[3] = 0xBAD2CAD3ULL;
    }
    memset(NewThreadState.flags, 0, Core::CPUState::NUM_EFLAG_BITS);
    NewThreadState.flags[1] = 1;
    NewThreadState.flags[9] = 1;
    NewThreadState.FCW = 0x37F;
    NewThreadState.FTW = 0xFFFF;
    return NewThreadState;
  }

  FEXCore::Core::InternalThreadState* Context::InitCore(uint64_t InitialRIP, uint64_t StackPointer) {
    // Initialize the CPU core signal handlers & DispatcherConfig
    switch (Config.Core) {
#ifdef INTERPRETER_ENABLED
    case FEXCore::Config::CONFIG_INTERPRETER:
      FEXCore::CPU::InitializeInterpreterSignalHandlers(this);
      BackendFeatures = FEXCore::CPU::GetInterpreterBackendFeatures();
      break;
#endif
    case FEXCore::Config::CONFIG_IRJIT:
#if (_M_X86_64 && JIT_X86_64)
      FEXCore::CPU::InitializeX86JITSignalHandlers(this);
      BackendFeatures = FEXCore::CPU::GetX86JITBackendFeatures();
#elif (_M_ARM_64 && JIT_ARM64) || defined(VIXL_SIMULATOR)
      FEXCore::CPU::InitializeArm64JITSignalHandlers(this);
      BackendFeatures = FEXCore::CPU::GetArm64JITBackendFeatures();
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

    DispatcherConfig.StaticRegisterAllocation = Config.StaticRegisterAllocation && BackendFeatures.SupportsStaticRegisterAllocation;

#if JIT_ARM64
    Dispatcher = FEXCore::CPU::Dispatcher::CreateArm64(this, DispatcherConfig);
#elif JIT_X86_64
    Dispatcher = FEXCore::CPU::Dispatcher::CreateX86(this, DispatcherConfig);
#else
    ERROR_AND_DIE_FMT("FEXCore has been compiled with an unknown target");
#endif

    // Initialize common signal handlers

    auto PauseHandler = [](FEXCore::Core::InternalThreadState *Thread, int Signal, void *info, void *ucontext) -> bool {
      return Thread->CTX->Dispatcher->HandleSignalPause(Thread, Signal, info, ucontext);
    };

    SignalDelegation->RegisterHostSignalHandler(SignalDelegator::SIGNAL_FOR_PAUSE, PauseHandler, true);

    auto GuestSignalHandler = [](FEXCore::Core::InternalThreadState *Thread, int Signal, void *info, void *ucontext, GuestSigAction *GuestAction, stack_t *GuestStack) -> bool {
      return Thread->CTX->Dispatcher->HandleGuestSignal(Thread, Signal, info, ucontext, GuestAction, GuestStack);
    };

    for (uint32_t Signal = 0; Signal <= SignalDelegator::MAX_SIGNALS; ++Signal) {
      SignalDelegation->RegisterHostSignalHandlerForGuest(Signal, GuestSignalHandler);
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

    using namespace FEXCore::Core;

    FEXCore::Core::CPUState NewThreadState = CreateDefaultCPUState();
    FEXCore::Core::InternalThreadState *Thread = CreateThread(&NewThreadState, 0);

    // We are the parent thread
    ParentThread = Thread;

    Thread->CurrentFrame->State.gregs[X86State::REG_RSP] = StackPointer;

    Thread->CurrentFrame->State.rip = InitialRIP;

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
    Thread->CTX->Dispatcher->ExecuteJITCallback(Thread->CurrentFrame, RIP);
  }

  void Context::HandleSignalHandlerReturn(bool RT) {
    using SignalHandlerReturnFunc = void(*)();

    SignalHandlerReturnFunc SignalHandlerReturn{};
    if (RT) {
      SignalHandlerReturn = reinterpret_cast<SignalHandlerReturnFunc>(Dispatcher->SignalHandlerReturnAddressRT);
    }
    else {
      SignalHandlerReturn = reinterpret_cast<SignalHandlerReturnFunc>(Dispatcher->SignalHandlerReturnAddress);
    }

    SignalHandlerReturn();
    FEX_UNREACHABLE;
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
    Thread->StartPaused = NeedToCheckXID;
    Thread->ExecutionThread = FEXCore::Threads::Thread::Create(ThreadHandler, Arg);

    // Wait for the thread to have started
    Thread->ThreadWaiting.Wait();

    if (NeedToCheckXID) {
      // The first time an application creates a thread, GLIBC installs their SETXID signal handler.
      // FEX needs to capture all signals and defer them to the guest.
      // Once FEX creates its first guest thread, overwrite the GLIBC SETXID handler *again* to ensure
      // FEX maintains control of the signal handler on this signal.
      NeedToCheckXID = false;
      SignalDelegation->CheckXIDHandler();
      Thread->StartRunning.NotifyAll();
    }
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

  void Context::InitializeCompiler(FEXCore::Core::InternalThreadState* Thread) {
    Thread->OpDispatcher = std::make_unique<FEXCore::IR::OpDispatchBuilder>(this);
    Thread->OpDispatcher->SetMultiblock(Config.Multiblock);
    Thread->LookupCache = std::make_unique<FEXCore::LookupCache>(this);
    Thread->FrontendDecoder = std::make_unique<FEXCore::Frontend::Decoder>(this);
    Thread->PassManager = std::make_unique<FEXCore::IR::PassManager>();
    Thread->PassManager->RegisterExitHandler([this]() {
        Stop(false /* Ignore current thread */);
    });

    Thread->CurrentFrame->Pointers.Common.L1Pointer = Thread->LookupCache->GetL1Pointer();
    Thread->CurrentFrame->Pointers.Common.L2Pointer = Thread->LookupCache->GetPagePointer();

    Dispatcher->InitThreadPointers(Thread);

    Thread->CTX = this;

    bool DoSRA = DispatcherConfig.StaticRegisterAllocation;

    Thread->PassManager->AddDefaultPasses(this, Config.Core == FEXCore::Config::CONFIG_IRJIT, DoSRA);
    Thread->PassManager->AddDefaultValidationPasses();

    Thread->PassManager->RegisterSyscallHandler(SyscallHandler);

    // Create CPU backend
    switch (Config.Core) {
#ifdef INTERPRETER_ENABLED
    case FEXCore::Config::CONFIG_INTERPRETER:
      Thread->CPUBackend = FEXCore::CPU::CreateInterpreterCore(this, Thread);
      break;
#endif
    case FEXCore::Config::CONFIG_IRJIT:
      Thread->PassManager->InsertRegisterAllocationPass(DoSRA, HostFeatures.SupportsAVX);

#if (_M_X86_64 && JIT_X86_64)
      Thread->CPUBackend = FEXCore::CPU::CreateX86JITCore(this, Thread);
#elif (_M_ARM_64 && JIT_ARM64) || defined(VIXL_SIMULATOR)
      Thread->CPUBackend = FEXCore::CPU::CreateArm64JITCore(this, Thread);
#else
      ERROR_AND_DIE_FMT("FEXCore has been compiled without a viable JIT core");
#endif

      break;
    case FEXCore::Config::CONFIG_CUSTOM:
      Thread->CPUBackend = CustomCPUFactory(this, Thread);
      break;
    default:
      ERROR_AND_DIE_FMT("Unknown core configuration");
      break;
    }
  }

  FEXCore::Core::InternalThreadState* Context::CreateThread(FEXCore::Core::CPUState *NewThreadState, uint64_t ParentTID) {
    FEXCore::Core::InternalThreadState *Thread = new FEXCore::Core::InternalThreadState{};

    // Copy over the new thread state to the new object
    memcpy(Thread->CurrentFrame, NewThreadState, sizeof(FEXCore::Core::CPUState));
    Thread->CurrentFrame->Thread = Thread;

    // Set up the thread manager state
    Thread->ThreadManager.parent_tid = ParentTID;

    InitializeCompiler(Thread);
    InitializeThreadData(Thread);

    // Insert after the Thread object has been fully initialized
    {
      std::lock_guard lk(ThreadCreationMutex);
      Threads.push_back(Thread);
    }

    return Thread;
  }

  void Context::DestroyThread(FEXCore::Core::InternalThreadState *Thread) {
    // remove new thread object
    {
      std::lock_guard lk(ThreadCreationMutex);

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

  void Context::AddBlockMapping(FEXCore::Core::InternalThreadState *Thread, uint64_t Address, void *Ptr) {
    Thread->LookupCache->AddBlockMapping(Address, Ptr);
  }

  void Context::ClearCodeCache(FEXCore::Core::InternalThreadState *Thread) {
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
    FILE* f = nullptr;
    bool CloseAfter = false;
    const auto DumpIRStr = Thread->CTX->Config.DumpIR();

    // DumpIRStr might be no if not dumping but ShouldDump is set in OpDisp
    if (DumpIRStr =="stderr" || DumpIRStr =="no") {
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
      auto NewIR = IREmitter->ViewIR();
      FEXCore::IR::Dump(&out, &NewIR, RA);
      fmt::print(f,"IR-{} 0x{:x}:\n{}\n@@@@@\n", RA ? "post" : "pre", GuestRIP, out.str());

      if (CloseAfter) {
        fclose(f);
      }
    }
  };

  static void ValidateIR(FEXCore::Context::Context *ctx, IR::IREmitter *IREmitter) {
    // Convert to text, Parse, Convert to text again and make sure the texts match
    std::stringstream out;
    static auto compaction = IR::CreateIRCompaction(ctx->OpDispatcherAllocator);
    compaction->Run(IREmitter);
    auto NewIR = IREmitter->ViewIR();
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

  Context::GenerateIRResult Context::GenerateIR(FEXCore::Core::InternalThreadState *Thread, uint64_t GuestRIP, bool ExtendedDebugInfo) {
    FEXCORE_PROFILE_SCOPED("GenerateIR");

    Thread->OpDispatcher->ReownOrClaimBuffer();
    Thread->OpDispatcher->ResetWorkingList();

    uint64_t TotalInstructions {0};
    uint64_t TotalInstructionsLength {0};


    std::shared_lock lk(CustomIRMutex);

    auto Handler = CustomIRHandlers.find(GuestRIP);
    if (Handler != CustomIRHandlers.end()) {
      TotalInstructions = 1;
      TotalInstructionsLength = 1;
      std::get<0>(Handler->second)(GuestRIP, Thread->OpDispatcher.get());
      lk.unlock();
    } else {
      lk.unlock();
      uint8_t const *GuestCode{};
      GuestCode = reinterpret_cast<uint8_t const*>(GuestRIP);

      bool HadDispatchError {false};

      Thread->FrontendDecoder->DecodeInstructionsAtEntry(GuestCode, GuestRIP, [Thread](uint64_t BlockEntry, uint64_t Start, uint64_t Length) {
        if (Thread->LookupCache->AddBlockExecutableRange(BlockEntry, Start, Length)) {
          Thread->CTX->SyscallHandler->MarkGuestExecutableRange(Start, Length);
        }
      });

      auto CodeBlocks = Thread->FrontendDecoder->GetDecodedBlocks();

      Thread->OpDispatcher->BeginFunction(GuestRIP, CodeBlocks);

      const uint8_t GPRSize = GetGPRSize();

      for (size_t j = 0; j < CodeBlocks->size(); ++j) {
        FEXCore::Frontend::Decoder::DecodedBlocks const &Block = CodeBlocks->at(j);
        // Set the block entry point
        Thread->OpDispatcher->SetNewBlockIfChanged(Block.Entry);

        uint64_t BlockInstructionsLength {};

        // Reset any block-specific state
        Thread->OpDispatcher->StartNewBlock();

        if (Config.x86dec_SynchronizeRIPOnAllBlocks) {
          // Ensure the RIP is synchronized to the context on block entry.
          // In the case of block linking, the RIP may not have synchronized.
          auto NewRIP = Thread->OpDispatcher->_EntrypointOffset(Block.Entry - GuestRIP, GPRSize);
          Thread->OpDispatcher->_StoreContext(GPRSize, IR::GPRClass, NewRIP, offsetof(FEXCore::Core::CPUState, rip));
        }

        uint64_t InstsInBlock = Block.NumInstructions;

        for (size_t i = 0; i < InstsInBlock; ++i) {
          FEXCore::X86Tables::X86InstInfo const* TableInfo {nullptr};
          FEXCore::X86Tables::DecodedInst const* DecodedInfo {nullptr};

          TableInfo = Block.DecodedInstructions[i].TableInfo;
          DecodedInfo = &Block.DecodedInstructions[i];
          bool IsLocked = DecodedInfo->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_LOCK;

          if (ExtendedDebugInfo) {
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

          const bool NeedsBlockEnd = (HadDispatchError && TotalInstructions > 0) ||
            (Thread->OpDispatcher->NeedsBlockEnder() && i + 1 == InstsInBlock);

          // If we had a dispatch error then leave early
          if (HadDispatchError && TotalInstructions == 0) {
            // Couldn't handle any instruction in op dispatcher
            Thread->OpDispatcher->ResetWorkingList();
            return { nullptr, nullptr, 0, 0, 0, 0 };
          }

          if (NeedsBlockEnd) {
            const uint8_t GPRSize = GetGPRSize();

            // We had some instructions. Early exit
            Thread->OpDispatcher->_ExitFunction(Thread->OpDispatcher->_EntrypointOffset(Block.Entry + BlockInstructionsLength - GuestRIP, GPRSize));
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

    auto ShouldDump = Thread->CTX->Config.DumpIR() != "no" || Thread->OpDispatcher->ShouldDump;
    // Debug
    {
      if (ShouldDump) {
        IRDumper(Thread, IREmitter, GuestRIP, nullptr);
      }

      if (Thread->CTX->Config.ValidateIRarser) {
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

  Context::CompileCodeResult Context::CompileCode(FEXCore::Core::InternalThreadState *Thread, uint64_t GuestRIP) {
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
      auto AOTIRCacheEntry = SyscallHandler->LookupAOTIRCacheEntry(GuestRIP);
      if (AOTIRCacheEntry.Entry && !AOTIRCacheEntry.Entry->ContainsCode) {
        AOTIRCacheEntry.Entry->SourcecodeMap =
            SourcecodeResolver->GenerateMap(AOTIRCacheEntry.Entry->Filename, AOTIRCacheEntry.Entry->FileId);
      }
    }

    // AOT IR bookkeeping and cache
    {
      auto [IRCopy, RACopy, DebugDataCopy, _StartAddr, _Length, _GeneratedIR] = IRCaptureCache.PreGenerateIRFetch(GuestRIP, IRList);
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
      auto [IRCopy, RACopy, TotalInstructions, TotalInstructionsLength, _StartAddr, _Length] = GenerateIR(Thread, GuestRIP, Config.GDBSymbols());

      // Setup pointers to internal structures
      IRList = IRCopy;
      RAData = std::move(RACopy);
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
      .CompiledCode = Thread->CPUBackend->CompileCode(GuestRIP, IRList, DebugData, RAData.get(), GetGdbServerStatus()),
      .IRData = IRList,
      .DebugData = DebugData,
      .RAData = std::move(RAData),
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
    FEXCORE_PROFILE_SCOPED("CompileBlock");
    auto Thread = Frame->Thread;

    // Invalidate might take a unique lock on this, to guarantee that during invalidation no code gets compiled
    std::shared_lock lk(CodeInvalidationMutex);

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

    auto [Code, IR, Data, RAData, Generated, _StartAddr, _Length] = CompileCode(Thread, GuestRIP);
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
        auto GuestRIPLookup = SyscallHandler->LookupAOTIRCacheEntry(GuestRIP);

        if (DebugData->Subblocks.size()) {
          for (auto& Subblock: DebugData->Subblocks) {
            auto BlockBasePtr = FragmentBasePtr + Subblock.HostCodeOffset;
            if (GuestRIPLookup.Entry) {
              Symbols.Register(BlockBasePtr, DebugData->HostCodeSize, GuestRIPLookup.Entry->Filename, GuestRIP - GuestRIPLookup.VAFileStart);
            } else {
              Symbols.Register(BlockBasePtr, GuestRIP, Subblock.HostCodeSize);
            }
          }
        } else {
          if (GuestRIPLookup.Entry) {
            Symbols.Register(FragmentBasePtr, DebugData->HostCodeSize, GuestRIPLookup.Entry->Filename, GuestRIP - GuestRIPLookup.VAFileStart);
        } else {
          Symbols.Register(FragmentBasePtr, GuestRIP, DebugData->HostCodeSize);
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

  void Context::ExecutionThread(FEXCore::Core::InternalThreadState *Thread) {
    Core::ThreadData.Thread = Thread;
    Thread->ExitReason = FEXCore::Context::ExitReason::EXIT_WAITING;

    InitializeThreadTLSData(Thread);

    ++IdleWaitRefCount;

    // Now notify the thread that we are initialized
    Thread->ThreadWaiting.NotifyAll();

    if (Thread != Thread->CTX->ParentThread || StartPaused || Thread->StartPaused) {
      // Parent thread doesn't need to wait to run
      Thread->StartRunning.Wait();
    }

    if (!Thread->RunningEvents.EarlyExit.load()) {
      Thread->RunningEvents.WaitingToStart = false;

      Thread->ExitReason = FEXCore::Context::ExitReason::EXIT_NONE;

      Thread->RunningEvents.Running = true;

      Thread->CTX->Dispatcher->ExecuteDispatch(Thread->CurrentFrame);

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
        Context::ThreadRemoveCodeEntry(Thread, Address);
      }
      it->second.clear();
    }
  }

  static void InvalidateGuestCodeRangeInternal(FEXCore::Context::Context *CTX, uint64_t Start, uint64_t Length) {
    std::lock_guard lk(CTX->ThreadCreationMutex);

    for (auto &Thread : CTX->Threads) {
      InvalidateGuestThreadCodeRange(Thread, Start, Length);
    }
  }

  void InvalidateGuestCodeRange(FEXCore::Context::Context *CTX, uint64_t Start, uint64_t Length) {
    FHU::ScopedSignalMaskWithUniqueLock CodeInvalidationLock(CTX->CodeInvalidationMutex);

    InvalidateGuestCodeRangeInternal(CTX, Start, Length);
  }

  void InvalidateGuestCodeRange(FEXCore::Context::Context *CTX, uint64_t Start, uint64_t Length, std::function<void(uint64_t start, uint64_t Length)> CallAfter) {
    FHU::ScopedSignalMaskWithUniqueLock CodeInvalidationLock(CTX->CodeInvalidationMutex);

    InvalidateGuestCodeRangeInternal(CTX, Start, Length);
    CallAfter(Start, Length);
  }

  void Context::MarkMemoryShared() {
    if (!IsMemoryShared) {
      IsMemoryShared = true;

      if (Config.TSOAutoMigration) {
        std::lock_guard<std::mutex> lkThreads(ThreadCreationMutex);
        LogMan::Throw::AFmt(Threads.size() == 1, "First MarkMemoryShared called must be before creating any threads");

        auto Thread = Threads[0];

        // Only the lookup cache is cleared here, so that old code can keep running until next compilation
        std::lock_guard<std::recursive_mutex> lkLookupCache(Thread->LookupCache->WriteLock);
        Thread->LookupCache->ClearCache();

        // DebugStore also needs to be cleared
        Thread->DebugStore.clear();
      }
    }
  }

  void MarkMemoryShared(FEXCore::Context::Context *CTX) {
    CTX->MarkMemoryShared();
  }

  void Context::ThreadAddBlockLink(FEXCore::Core::InternalThreadState *Thread, uint64_t GuestDestination, uintptr_t HostLink, const std::function<void()> &delinker) {
    std::shared_lock lk(Thread->CTX->CodeInvalidationMutex);

    Thread->LookupCache->AddBlockLink(GuestDestination, HostLink, delinker);
  }

  void Context::ThreadRemoveCodeEntry(FEXCore::Core::InternalThreadState *Thread, uint64_t GuestRIP) {
    LogMan::Throw::AFmt(Thread->CTX->CodeInvalidationMutex.try_lock() == false, "CodeInvalidationMutex needs to be unique_locked here");

    std::lock_guard<std::recursive_mutex> lk(Thread->LookupCache->WriteLock);

    Thread->DebugStore.erase(GuestRIP);
    Thread->LookupCache->Erase(GuestRIP);
  }

  CustomIRResult Context::AddCustomIREntrypoint(uintptr_t Entrypoint, std::function<void(uintptr_t Entrypoint, FEXCore::IR::IREmitter *)> Handler, void *Creator, void *Data) {
    LOGMAN_THROW_A_FMT(Config.Is64BitMode || !(Entrypoint >> 32), "64-bit Entrypoint in 32-bit mode {:x}", Entrypoint);

    std::unique_lock lk(CustomIRMutex);

    auto InsertedIterator = CustomIRHandlers.emplace(Entrypoint, std::tuple(Handler, Creator, Data));

    if (!InsertedIterator.second) {
      const auto &[fn, Creator, Data] = InsertedIterator.first->second;
      return CustomIRResult(std::move(lk), Creator, Data);
    } else {
      lk.unlock();
      return CustomIRResult(std::move(lk), 0, 0);
    }
  }

  void Context::RemoveCustomIREntrypoint(uintptr_t Entrypoint) {
    LOGMAN_THROW_A_FMT(Config.Is64BitMode || !(Entrypoint >> 32), "64-bit Entrypoint in 32-bit mode {:x}", Entrypoint);

    std::scoped_lock lk(CustomIRMutex);

    InvalidateGuestCodeRange(this, Entrypoint, 1, [this](uint64_t Entrypoint, uint64_t) {
      CustomIRHandlers.erase(Entrypoint);
    });
  }

  // Debug interface
  void Context::CompileRIP(FEXCore::Core::InternalThreadState *Thread, uint64_t RIP) {
    uint64_t RIPBackup = Thread->CurrentFrame->State.rip;
    Thread->CurrentFrame->State.rip = RIP;

    // Erase the RIP from all the storage backings if it exists
    ThreadRemoveCodeEntry(Thread, RIP);

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
    auto it = ParentThread->DebugStore.find(RIP);
    if (it == ParentThread->DebugStore.end()) {
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


  void Context::AppendThunkDefinitions(std::vector<FEXCore::IR::ThunkDefinition> const& Definitions) {
    ThunkHandler->AppendThunkDefinitions(Definitions);
  }

  void ConfigureAOTGen(FEXCore::Core::InternalThreadState *Thread, std::set<uint64_t> *ExternalBranches, uint64_t SectionMaxAddress) {
    Thread->FrontendDecoder->SetExternalBranches(ExternalBranches);
    Thread->FrontendDecoder->SetSectionMaxAddress(SectionMaxAddress);
  }
}
