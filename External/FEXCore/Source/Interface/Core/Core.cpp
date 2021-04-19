/*
$info$
category: glue ~ Logic that binds various parts together
meta: glue|driver ~ Emulation mainloop related glue logic
tags: glue|driver
desc: Glues Frontend, OpDispatcher and IR Opts & Compilation, LookupCache, Dispatcher and provides the Execution loop entrypoint
$end_info$
*/

#include "Common/MathUtils.h"
#include "Common/Paths.h"

#include "Interface/Context/Context.h"
#include "Interface/Core/LookupCache.h"
#include "Interface/Core/BlockSamplingData.h"
#include "Interface/Core/CompileService.h"
#include "Interface/Core/Core.h"
#include "Interface/Core/DebugData.h"
#include "Interface/Core/OpcodeDispatcher.h"
#include "Interface/Core/Interpreter/InterpreterCore.h"
#include "Interface/Core/JIT/JITCore.h"
#include "Interface/IR/Passes/RegisterAllocationPass.h"
#include "Interface/IR/Passes.h"

#include <FEXCore/Config/Config.h>
#include <FEXCore/Core/CodeLoader.h>
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Core/CPUBackend.h>
#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/HLE/SyscallHandler.h>

#include "Interface/HLE/Thunks/Thunks.h"
#include "FEXCore/Utils/Allocator.h"

#include <xxh3.h>
#include <fstream>
#include <unistd.h>
#include <filesystem>
#include <algorithm>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/stat.h>

#include "Interface/Core/GdbServer.h"

namespace FEXCore::CPU {
  bool CreateCPUCore(FEXCore::Context::Context *CTX) {
    // This should be used for generating things that are shared between threads
    CTX->CPUID.Init(CTX);
    return true;
  }
}

static std::mutex AOTIRCacheLock;

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

namespace DefaultFallbackCore {
  class DefaultFallbackCore final : public FEXCore::CPU::CPUBackend {
  public:
    explicit DefaultFallbackCore(FEXCore::Core::ThreadState *Thread)
      : ThreadState {reinterpret_cast<FEXCore::Core::InternalThreadState*>(Thread)} {
    }
    ~DefaultFallbackCore() override = default;

    std::string GetName() override { return "Default Fallback"; }

    void *MapRegion(void *HostPtr, uint64_t VirtualGuestPtr, uint64_t Size) override {
      return HostPtr;
    }

    void Initialize() override {}
    bool NeedsOpDispatch() override { return false; }

    void *CompileCode(uint64_t Entry, FEXCore::IR::IRListView const *IR, FEXCore::Core::DebugData *DebugData, FEXCore::IR::RegisterAllocationData *RAData) override {
      LogMan::Msg::E("Fell back to default code handler at RIP: 0x%lx", ThreadState->CurrentFrame->State.rip);
      return nullptr;
    }

  private:
    FEXCore::Core::InternalThreadState *ThreadState;
  };

  FEXCore::CPU::CPUBackend *CPUCreationFactory(FEXCore::Context::Context* CTX, FEXCore::Core::ThreadState *Thread) {
    return new DefaultFallbackCore(Thread);
  }
}

}

namespace FEXCore::Context {
  Context::Context() {
#ifdef BLOCKSTATS
    BlockData = std::make_unique<FEXCore::BlockSamplingData>();
#endif
    if (Config.GdbServer) {
      StartGdbServer();
    }
    else {
      StopGdbServer();
    }
  }

  Context::~Context() {
    {
      for (auto &Thread : Threads) {
        if (Thread->ExecutionThread->joinable()) {
          Thread->ExecutionThread->join(nullptr);
        }
      }

      for (auto &Thread : Threads) {

        if (Thread->CompileService) {
          Thread->CompileService->Shutdown();
        }
        delete Thread;
      }
      Threads.clear();
    }

    // AOTIRCaptureCache needs manual clear
    for (auto &Mod: AOTIRCaptureCache) {
      for (auto &Entry: Mod.second) {
        delete Entry.second.IR;
        free(Entry.second.RAData);
      }
    }

    for (auto &Mod: AOTIRCache) {
      FEXCore::Allocator::munmap(Mod.second.mapping, Mod.second.size);
    }
  }

  bool Context::InitCore(FEXCore::CodeLoader *Loader) {
    ThunkHandler.reset(FEXCore::ThunkHandler::Create());

    LocalLoader = Loader;
    using namespace FEXCore::Core;
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

    FEXCore::Core::InternalThreadState *Thread = CreateThread(&NewThreadState, 0);

    // We are the parent thread
    ParentThread = Thread;

    Thread->CurrentFrame->State.gregs[X86State::REG_RSP] = Loader->GetStackPointer();

    Thread->CurrentFrame->State.rip = StartingRIP = Loader->DefaultRIP();

    InitializeThreadData(Thread);

    return true;
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

  void Context::HandleCallback(uint64_t RIP) {
    auto Thread = Core::ThreadData.Thread;
    Thread->CPUBackend->CallbackPtr(Thread->CurrentFrame, RIP);
  }

  void Context::RegisterHostSignalHandler(int Signal, HostSignalDelegatorFunction Func) {
      SignalDelegation->RegisterHostSignalHandler(Signal, Func);
  }

  void Context::RegisterFrontendHostSignalHandler(int Signal, HostSignalDelegatorFunction Func) {
    SignalDelegation->RegisterFrontendHostSignalHandler(Signal, Func);
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
      Thread->SignalReason.store(FEXCore::Core::SignalEvent::SIGNALEVENT_PAUSE);
      if (Thread->RunningEvents.Running.load()) {
        // Only attempt to stop this thread if it is running
        tgkill(Thread->ThreadManager.PID, Thread->ThreadManager.TID, SignalDelegator::SIGNAL_FOR_PAUSE);
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
      Thread->SignalReason.store(FEXCore::Core::SignalEvent::SIGNALEVENT_RETURN);
      Thread->RunningEvents.WaitingToStart.store(true);
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
        ClearCodeCache(Thread, true);
      }
    }
    CoreRunningMode PreviousRunningMode = this->Config.RunningMode;
    int64_t PreviousMaxIntPerBlock = this->Config.MaxInstPerBlock;
    this->Config.RunningMode = FEXCore::Context::CoreRunningMode::MODE_SINGLESTEP;
    this->Config.MaxInstPerBlock = 1;
    Run();
    WaitForThreadsToRun();
    WaitForIdleWithTimeout();
    this->Config.RunningMode = PreviousRunningMode;
    this->Config.MaxInstPerBlock = PreviousMaxIntPerBlock;
  }

  void Context::Stop(bool IgnoreCurrentThread) {
    pid_t tid = gettid();
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
        } else {
          LogMan::Msg::D("Skipping thread %p: Already stopped", Thread);
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
      Thread->SignalReason.store(FEXCore::Core::SignalEvent::SIGNALEVENT_STOP);
      tgkill(Thread->ThreadManager.PID, Thread->ThreadManager.TID, SignalDelegator::SIGNAL_FOR_PAUSE);
    }
  }

  void Context::SignalThread(FEXCore::Core::InternalThreadState *Thread, FEXCore::Core::SignalEvent Event) {
    if (Thread->RunningEvents.Running.load()) {
      Thread->SignalReason.store(Event);
      tgkill(Thread->ThreadManager.PID, Thread->ThreadManager.TID, SignalDelegator::SIGNAL_FOR_PAUSE);
    }
  }

  FEXCore::Context::ExitReason Context::RunUntilExit() {
    if(!StartPaused)
      Run();

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

  int Context::GetProgramStatus() {
    return ParentThread->StatusCode;
  }

  void Context::InitializeThreadData(FEXCore::Core::InternalThreadState *Thread) {
    Thread->CPUBackend->Initialize();

    auto IRHandler = [Thread](uint64_t Addr, IR::IREmitter *IR) -> void {
      // Run the passmanager over the IR from the dispatcher
      Thread->PassManager->Run(IR);
      Core::LocalIREntry Entry = {Addr, 0ULL, decltype(Entry.IR)(IR->CreateIRCopy()), decltype(Entry.RAData)(Thread->PassManager->GetRAPass() ? Thread->PassManager->GetRAPass()->PullAllocationData() : nullptr), decltype(Entry.DebugData)(new Core::DebugData())};
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
    free(Handler);
    return nullptr;
  }

  void Context::InitializeThread(FEXCore::Core::InternalThreadState *Thread) {
    InitializeThreadData(Thread);

    // This will create the execution thread but it won't actually start executing
    ExecutionThreadHandler *Arg = reinterpret_cast<ExecutionThreadHandler*>(malloc(sizeof(ExecutionThreadHandler)));
    Arg->This = this;
    Arg->Thread = Thread;
    Thread->ExecutionThread = FEXCore::Threads::Thread::Create(ThreadHandler, Arg);

    // Wait for the thread to have started
    Thread->ThreadWaiting.Wait();
  }

  void Context::RunThread(FEXCore::Core::InternalThreadState *Thread) {
    // Tell the thread to start executing
    Thread->StartRunning.NotifyAll();
  }

  void Context::InitializeCompiler(FEXCore::Core::InternalThreadState* State, bool CompileThread) {
    State->OpDispatcher = std::make_unique<FEXCore::IR::OpDispatchBuilder>(this);
    State->OpDispatcher->SetMultiblock(Config.Multiblock);
    State->LookupCache = std::make_unique<FEXCore::LookupCache>(this);
    State->FrontendDecoder = std::make_unique<FEXCore::Frontend::Decoder>(this);
    State->PassManager = std::make_unique<FEXCore::IR::PassManager>();
    State->PassManager->RegisterExitHandler([this]() {
        Stop(false /* Ignore current thread */);
    });

    #if _M_ARM_64
    bool DoSRA = true;
    #else
    bool DoSRA = false;
    #endif

    State->PassManager->AddDefaultPasses(Config.Core == FEXCore::Config::CONFIG_IRJIT, DoSRA);
    State->PassManager->AddDefaultValidationPasses();

    State->PassManager->RegisterSyscallHandler(SyscallHandler);

    State->CTX = this;

    // Create CPU backend
    switch (Config.Core) {
    case FEXCore::Config::CONFIG_INTERPRETER:
      State->CPUBackend.reset(FEXCore::CPU::CreateInterpreterCore(this, State, CompileThread));
      break;
    case FEXCore::Config::CONFIG_IRJIT:
      State->PassManager->InsertRegisterAllocationPass(DoSRA);

#if (_M_X86_64 && JIT_X86_64)
      State->CPUBackend.reset(FEXCore::CPU::CreateX86JITCore(this, State, CompileThread));
#elif (_M_ARM_64 && JIT_ARM64)
      State->CPUBackend.reset(FEXCore::CPU::CreateArm64JITCore(this, State, CompileThread));
#else
      ERROR_AND_DIE("FEXCore has been compiled without a viable JIT core");
#endif

      break;
    case FEXCore::Config::CONFIG_CUSTOM:      State->CPUBackend.reset(CustomCPUFactory(this, State)); break;
    default: ERROR_AND_DIE("Unknown core configuration");
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

    InitializeCompiler(Thread, false);

    return Thread;
  }

  void Context::DestroyThread(FEXCore::Core::InternalThreadState *Thread) {
    // remove new thread object
    {
      std::lock_guard<std::mutex> lk(ThreadCreationMutex);

      auto It = std::find(Threads.begin(), Threads.end(), Thread);
      LOGMAN_THROW_A(It != Threads.end(), "Thread wasn't in Threads");

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
  }

  void Context::AddBlockMapping(FEXCore::Core::InternalThreadState *Thread, uint64_t Address, void *Ptr, uint64_t Start, uint64_t Length) {
    Thread->LookupCache->AddBlockMapping(Address, Ptr, Start, Length);
  }

  void Context::ClearCodeCache(FEXCore::Core::InternalThreadState *Thread, bool AlsoClearIRCache) {
    Thread->LookupCache->ClearCache();
    Thread->CPUBackend->ClearCache();
    if (Thread->CompileService) {
      Thread->CompileService->ClearCache(Thread);
    }

    if (AlsoClearIRCache) {
      Thread->LocalIRCache.clear();
    }
  }

  std::tuple<FEXCore::IR::IRListView *, FEXCore::IR::RegisterAllocationData *, uint64_t, uint64_t, uint64_t, uint64_t> Context::GenerateIR(FEXCore::Core::InternalThreadState *Thread, uint64_t GuestRIP) {
    uint8_t const *GuestCode{};
    GuestCode = reinterpret_cast<uint8_t const*>(GuestRIP);

    bool HadDispatchError {false};

    uint64_t TotalInstructions {0};
    uint64_t TotalInstructionsLength {0};

    if (!Thread->FrontendDecoder->DecodeInstructionsAtEntry(GuestCode, GuestRIP)) {
      return { nullptr, nullptr, 0, 0, 0, 0 };
    }

    auto CodeBlocks = Thread->FrontendDecoder->GetDecodedBlocks();

    Thread->OpDispatcher->BeginFunction(GuestRIP, CodeBlocks);

    uint8_t GPRSize = Config.Is64BitMode ? 8 : 4;

    for (size_t j = 0; j < CodeBlocks->size(); ++j) {
      FEXCore::Frontend::Decoder::DecodedBlocks const &Block = CodeBlocks->at(j);
      // Set the block entry point
      Thread->OpDispatcher->SetNewBlockIfChanged(Block.Entry);


      uint64_t BlockInstructionsLength {};

      // Reset any block-specific state
      Thread->OpDispatcher->StartNewBlock();

      uint64_t InstsInBlock = Block.NumInstructions;

      if (Block.HasInvalidInstruction) {
        Thread->OpDispatcher->_ExitFunction(Thread->OpDispatcher->_EntrypointOffset(Block.Entry - GuestRIP, GPRSize));
        break;
      }

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
          Thread->OpDispatcher->_RemoveCodeEntry();
          Thread->OpDispatcher->_ExitFunction(Thread->OpDispatcher->_EntrypointOffset(Block.Entry + BlockInstructionsLength - GuestRIP, GPRSize));

          auto NextOpBlock = Thread->OpDispatcher->CreateNewCodeBlockAfter(CurrentBlock);

          Thread->OpDispatcher->SetFalseJumpTarget(InvalidateCodeCond, NextOpBlock);
          Thread->OpDispatcher->SetCurrentCodeBlock(NextOpBlock);
        }

        if (TableInfo->OpcodeDispatcher) {
          auto Fn = TableInfo->OpcodeDispatcher;
          Thread->OpDispatcher->HandledLock = false;
          std::invoke(Fn, Thread->OpDispatcher, DecodedInfo);
          if (Thread->OpDispatcher->HadDecodeFailure()) {
            HadDispatchError = true;
          }
          else {
            if (Thread->OpDispatcher->HandledLock != IsLocked)
              HadDispatchError = true;
            //LOGMAN_THROW_A(Thread->OpDispatcher->HandledLock == IsLocked, "Missing LOCK HANDLER at 0x%lx{'%s'}\n", Block.Entry + BlockInstructionsLength, TableInfo->Name);
            BlockInstructionsLength += DecodedInfo->InstSize;
            TotalInstructionsLength += DecodedInfo->InstSize;
            ++TotalInstructions;
          }
        }
        else {
          LogMan::Msg::E("Missing OpDispatcher at 0x%lx{'%s'}", Block.Entry + BlockInstructionsLength, TableInfo->Name);
          HadDispatchError = true;
        }

        // If we had a dispatch error then leave early
        if (HadDispatchError) {
          if (TotalInstructions == 0) {
            // Couldn't handle any instruction in op dispatcher
            Thread->OpDispatcher->ResetWorkingList();
            return { nullptr, nullptr, 0, 0, 0, 0 };
          }
          else {
            uint8_t GPRSize = Config.Is64BitMode ? 8 : 4;

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

    auto IRDumper = [Thread, GuestRIP](IR::RegisterAllocationData* RA) {
      FILE* f = nullptr;
      bool CloseAfter = false;

      if (Thread->CTX->Config.DumpIR() =="stderr") {
        f = stderr;
      }
      else if (Thread->CTX->Config.DumpIR() =="stdout") {
        f = stdout;
      }
      else {
        std::stringstream fileName;
        fileName << Thread->CTX->Config.DumpIR()  << "/" << std::hex << GuestRIP << (RA ? "-post.ir" : "-pre.ir");

        f = fopen(fileName.str().c_str(), "w");
        CloseAfter = true;
      }

      if (f) {
        std::stringstream out;
        auto NewIR = Thread->OpDispatcher->ViewIR();
        FEXCore::IR::Dump(&out, &NewIR, RA);
        fprintf(f,"IR-%s 0x%lx:\n%s\n@@@@@\n", RA ? "post" : "pre", GuestRIP, out.str().c_str());

        if (CloseAfter) {
          fclose(f);
        }
      }
    };

    if (Thread->CTX->Config.DumpIR() != "no") {
      IRDumper(nullptr);
    }

    if (Thread->CTX->Config.ValidateIRarser) {
      // Convert to text, Parse, Convert to text again and make sure the texts match
      std::stringstream out;
      static auto compaction = IR::CreateIRCompaction();
      compaction->Run(Thread->OpDispatcher.get());
      auto NewIR = Thread->OpDispatcher->ViewIR();
      Dump(&out, &NewIR, nullptr);
      out.seekg(0);
      auto reparsed = IR::Parse(&out);
      if (reparsed == nullptr) {
        LOGMAN_MSG_A("Failed to parse ir\n");
      } else {
        std::stringstream out2;
        auto NewIR2 = reparsed->ViewIR();
        Dump(&out2, &NewIR2, nullptr);
        if (out.str() != out2.str()) {
          LogMan::Msg::I("one:\n %s", out.str().c_str());
          LogMan::Msg::I("two:\n %s", out2.str().c_str());
          LOGMAN_MSG_A("Parsed ir doesn't match\n");
        }
        delete reparsed;
      }
    }
    // Run the passmanager over the IR from the dispatcher
    Thread->PassManager->Run(Thread->OpDispatcher.get());

    if (Thread->CTX->Config.DumpIR() != "no") {
      IRDumper(Thread->PassManager->GetRAPass() ? Thread->PassManager->GetRAPass()->GetAllocationData() : nullptr);
    }

    if (Thread->OpDispatcher->ShouldDump) {
      std::stringstream out;
      auto NewIR = Thread->OpDispatcher->ViewIR();
      FEXCore::IR::Dump(&out, &NewIR, Thread->PassManager->GetRAPass() ? Thread->PassManager->GetRAPass()->GetAllocationData() : nullptr);
      LogMan::Msg::I("IR 0x%lx:\n%s\n@@@@@\n", GuestRIP, out.str().c_str());
    }

    auto RAData = Thread->PassManager->GetRAPass() ? Thread->PassManager->GetRAPass()->PullAllocationData() : nullptr;
    auto IRList = Thread->OpDispatcher->CreateIRCopy();

    Thread->OpDispatcher->ResetWorkingList();

    return {IRList, RAData.release(), TotalInstructions, TotalInstructionsLength, Thread->FrontendDecoder->DecodedMinAddress, Thread->FrontendDecoder->DecodedMaxAddress - Thread->FrontendDecoder->DecodedMinAddress };
  }

  AOTIRInlineEntry *AOTIRInlineIndex::GetInlineEntry(uint64_t DataOffset) {
    uintptr_t This = (uintptr_t)this;

    return (AOTIRInlineEntry*)(This + DataBase + DataOffset);
  }

  AOTIRInlineEntry *AOTIRInlineIndex::Find(uint64_t GuestStart) {
    ssize_t l = 0;
    ssize_t r = Count - 1;

    while (l <= r) {
      size_t m = l + (r - l) / 2;

      if (Entries[m].GuestStart == GuestStart)
        return GetInlineEntry(Entries[m].DataOffset);
      else if (Entries[m].GuestStart < GuestStart)
        l = m + 1;
      else
        r = m - 1;
    }

    return nullptr;
  }

  IR::RegisterAllocationData *AOTIRInlineEntry::GetRAData() {
    return (IR::RegisterAllocationData *)InlineData;
  }

  IR::IRListView *AOTIRInlineEntry::GetIRData() {
    auto RAData = GetRAData();
    auto Offset = RAData->Size(RAData->MapCount);

    return (IR::IRListView *)&InlineData[Offset];
  }

  std::tuple<void *, FEXCore::IR::IRListView *, FEXCore::Core::DebugData *, FEXCore::IR::RegisterAllocationData *, bool, uint64_t, uint64_t> Context::CompileCode(FEXCore::Core::InternalThreadState *Thread, uint64_t GuestRIP) {
    FEXCore::IR::IRListView *IRList {};
    FEXCore::Core::DebugData *DebugData {};
    FEXCore::IR::RegisterAllocationData *RAData {};
    bool GeneratedIR {};
    uint64_t StartAddr {};
    uint64_t Length {};

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

    {
      std::lock_guard<std::mutex> lk(AOTIRCacheLock);
      auto file = AddrToFile.lower_bound(GuestRIP);
      if (file != AddrToFile.begin()) {
        --file;
        if (!file->second.ContainsCode) {
          file->second.ContainsCode = true;
          FilesWithCode[file->second.fileid] = file->second.filename;
        }
      }
    }

    if (IRList == nullptr && Config.AOTIRLoad) {
      std::lock_guard<std::mutex> lk(AOTIRCacheLock);
      auto file = AddrToFile.lower_bound(GuestRIP);
      if (file != AddrToFile.begin()) {
        --file;
        auto Mod = (AOTIRInlineIndex*)file->second.CachedFileEntry;

        if (Mod == nullptr) {
          file->second.CachedFileEntry = Mod = AOTIRCache[file->second.fileid].Array;
        }

        if (Mod != nullptr)
        {
          auto AOTEntry = Mod->Find(GuestRIP - file->second.Start + file->second.Offset);

          if (AOTEntry) {
            // verify hash
            auto MappedStart = GuestRIP;
            auto hash = XXH3_64bits((void*)MappedStart, AOTEntry->GuestLength);
            if (hash == AOTEntry->GuestHash) {
              IRList = AOTEntry->GetIRData();
              //LogMan::Msg::D("using %s + %lx -> %lx\n", file->second.fileid.c_str(), AOTEntry->first, GuestRIP);


              RAData = AOTEntry->GetRAData();;
              DebugData = new FEXCore::Core::DebugData();
              StartAddr = MappedStart;
              Length = AOTEntry->GuestLength;

              GeneratedIR = true;
            } else {
              LogMan::Msg::I("AOTIR: hash check failed %lx\n", MappedStart);
            }
          } else {
            //LogMan::Msg::I("AOTIR: Failed to find %lx, %lx, %s\n", GuestRIP, GuestRIP - file->second.Start + file->second.Offset, file->second.fileid.c_str());
          }
        }
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

      // Initialize metadata
      DebugData->GuestCodeSize = TotalInstructionsLength;
      DebugData->GuestInstructionCount = TotalInstructions;

      // Increment stats
      Thread->Stats.BlocksCompiled.fetch_add(1);

      // These blocks aren't already in the cache
      GeneratedIR = true;
    }

    if (IRList == nullptr) {
      return { nullptr, nullptr, nullptr, nullptr, false, 0, 0 };
    }
    // Attempt to get the CPU backend to compile this code
    return { Thread->CPUBackend->CompileCode(GuestRIP, IRList, DebugData, RAData), IRList, DebugData, RAData, GeneratedIR, StartAddr, Length};
  }

  static bool readAll(int fd, void *data, size_t size) {
    int rv = read(fd, data, size);

    if (rv != size)
      return false;
    else
      return true;
  }

  bool Context::LoadAOTIRCache(int streamfd) {
    uint64_t tag;
    
    if (!readAll(streamfd, (char*)&tag, sizeof(tag)) || tag != 0xDEADBEEFC0D30003)
      return false;
    
    std::string Module;
    uint64_t ModSize;
    
    if (!readAll(streamfd,  (char*)&ModSize, sizeof(ModSize)))
      return false;

    Module.resize(ModSize);
    if (!readAll(streamfd,  (char*)&Module[0], Module.size()))
      return false;

    struct stat fileinfo;
    if (fstat(streamfd, &fileinfo) < 0)
      return false;
    size_t Size = (fileinfo.st_size + 4095) & ~4095;

    void *FilePtr = FEXCore::Allocator::mmap(nullptr, Size, PROT_READ, MAP_SHARED, streamfd, 0);

    if (FilePtr == MAP_FAILED)
      return false;

    auto Array = (AOTIRInlineIndex *)((char*)FilePtr + sizeof(tag) + sizeof(ModSize) + ((ModSize+31) & ~31));

    AOTIRCache.insert({Module, {Array, FilePtr, Size}});

    LogMan::Msg::D("AOTIR: Module %s has %ld functions", Module.c_str(), Array->Count);

    return true;

  }

  void Context::WriteFilesWithCode(std::function<void(const std::string& fileid, const std::string& filename)> Writer) {
    for( const auto &File: FilesWithCode) {
      Writer(File.first, File.second);
    }
  }

  bool Context::WriteAOTIRCache(std::function<std::unique_ptr<std::ostream>(const std::string&)> CacheWriter) {
    std::lock_guard<std::mutex> lk(AOTIRCacheLock);

    bool rv = true;

    for (auto AOTModule: AOTIRCaptureCache) {
      if (AOTModule.second.size() == 0) {
        continue;
      }

      auto stream = CacheWriter(AOTModule.first);
      if (!*stream) {
        rv = false;
      }
      uint64_t tag = 0xDEADBEEFC0D30003;
      stream->write((char*)&tag, sizeof(tag));

      auto ModSize = AOTModule.first.size();
      stream->write((char*)&ModSize, sizeof(ModSize));
      stream->write((char*)&AOTModule.first[0], ModSize);

      auto Skip = ((ModSize + 31) & ~31) - ModSize;
      char Zero = 0;
      for (int i = 0; i < Skip; i++)
        stream->write(&Zero, 1);

      // AOTIRInlineIndex
      

      auto FnCount = AOTModule.second.size();
      stream->write((char*)&FnCount, sizeof(FnCount));

      size_t DataBase = sizeof(FnCount) + sizeof(DataBase) + FnCount * sizeof(AOTIRInlineIndexEntry);
      stream->write((char*)&DataBase, sizeof(DataBase));

      size_t DataOffset = 0;
      for (auto entry: AOTModule.second) {
        //AOTIRInlineIndexEntry

        // GuestStart
        stream->write((char*)&entry.first, sizeof(entry.first));

        // DataOffset
        stream->write((char*)&DataOffset, sizeof(DataOffset));


        DataOffset += sizeof(entry.second.crc);
        DataOffset += sizeof(entry.second.len);

        DataOffset += entry.second.RAData->Size(entry.second.RAData->MapCount);

        DataOffset += entry.second.IR->GetInlineSize();
      }

      // AOTIRInlineEntry
      for (auto entry: AOTModule.second) {
        //GuestHash
        stream->write((char*)&entry.second.crc, sizeof(entry.second.crc));

        //GuestLength
        stream->write((char*)&entry.second.len, sizeof(entry.second.len));
        
        // RAData (inline)
        stream->write((char*)entry.second.RAData, entry.second.RAData->Size(entry.second.RAData->MapCount));
        
        // IRData (inline)
        entry.second.IR->Serialize(*stream);
      }
    }

    return rv;
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

    bool DecrementRefCount = false;
    bool GeneratedIR {};
    uint64_t StartAddr {}, Length {};

    if (Thread->CompileBlockReentrantRefCount != 0) {
      if (!Thread->CompileService) {
        Thread->CompileService = std::make_shared<FEXCore::CompileService>(this, Thread);
        Thread->CompileService->Initialize();
      }

      auto WorkItem = Thread->CompileService->CompileCode(GuestRIP);
      WorkItem->ServiceWorkDone.Wait();
      // Return here with the data in place
      CodePtr = WorkItem->CodePtr;
      IRList = WorkItem->IRList;
      DebugData = WorkItem->DebugData;
      RAData = WorkItem->RAData;
      StartAddr = WorkItem->StartAddr;
      Length = WorkItem->Length;
      WorkItem->SafeToClear = true;

      // The compile service will always generate IR + DebugData + RAData
      // Remove the entries here to make sure we don't fail to insert later on
      RemoveCodeEntry(Thread, GuestRIP);
      GeneratedIR = true;
    } else {
      ++Thread->CompileBlockReentrantRefCount;
      DecrementRefCount = true;
      auto [Code, IR, Data, RA, Generated, _StartAddr, _Length] = CompileCode(Thread, GuestRIP);
      CodePtr = Code;
      IRList = IR;
      DebugData = Data;
      RAData = RA;
      GeneratedIR = Generated;
      StartAddr = _StartAddr;
      Length = _Length;
    }

    if (CodePtr == nullptr) {
      if (DecrementRefCount)
        --Thread->CompileBlockReentrantRefCount;
      return 0;
    }

    LOGMAN_THROW_A(CodePtr != nullptr, "Failed to compile code %lX", GuestRIP);

    // The core managed to compile the code.
#if ENABLE_JITSYMBOLS
    if (DebugData) {
      if (DebugData->Subblocks.size()) {
        for (auto& Subblock: DebugData->Subblocks) {
          Symbols.Register((void*)Subblock.HostCodeStart, GuestRIP, Subblock.HostCodeSize);
        }
      } else {
        Symbols.Register(CodePtr, GuestRIP, DebugData->HostCodeSize);
      }
    }
#endif

    // Insert to caches if we generated IR
    if (GeneratedIR) {
      Core::LocalIREntry Entry = {StartAddr, Length, decltype(Entry.IR)(IRList), decltype(Entry.RAData)(RAData), decltype(Entry.DebugData)(DebugData)};
      Thread->LocalIRCache.insert({GuestRIP, std::move(Entry)});

      // Add to AOT cache if aot generation is enabled
      if ((Config.AOTIRCapture() || Config.AOTIRGenerate()) && RAData) {
        std::lock_guard<std::mutex> lk(AOTIRCacheLock);

        RAData->IsShared = true;
        IRList->SetShared(true);

        auto hash = XXH3_64bits((void*)StartAddr, Length);

        auto file = AddrToFile.lower_bound(StartAddr);
        if (file != AddrToFile.begin()) {
          --file;
          if (file->second.Start <= StartAddr && (file->second.Start + file->second.Len) >= (StartAddr + Length)) {
            AOTIRCaptureCache[file->second.fileid].insert({GuestRIP - file->second.Start + file->second.Offset, {StartAddr - file->second.Start + file->second.Offset, Length, hash, IRList, RAData}});
          }
        }
      }
    }

    if (DecrementRefCount)
      --Thread->CompileBlockReentrantRefCount;

    // Insert to lookup cache
    AddBlockMapping(Thread, GuestRIP, CodePtr, StartAddr, Length);

    return (uintptr_t)CodePtr;
  }

  void Context::ExecutionThread(FEXCore::Core::InternalThreadState *Thread) {
    Core::ThreadData.Thread = Thread;
    Thread->ExitReason = FEXCore::Context::ExitReason::EXIT_WAITING;

    // Let's do some initial bookkeeping here
    Thread->ThreadManager.TID = ::gettid();
    Thread->ThreadManager.PID = ::getpid();
    SignalDelegation->RegisterTLSState(Thread);
    ThunkHandler->RegisterTLSState(Thread);

    ++IdleWaitRefCount;

    LogMan::Msg::D("[%d] Waiting to run", Thread->ThreadManager.TID.load());

    // Now notify the thread that we are initialized
    Thread->ThreadWaiting.NotifyAll();

    if (Thread != Thread->CTX->ParentThread || StartPaused) {
      // Parent thread doesn't need to wait to run
      Thread->StartRunning.Wait();
    }

    LogMan::Msg::D("[%d] Running", Thread->ThreadManager.TID.load());

    Thread->ExitReason = FEXCore::Context::ExitReason::EXIT_NONE;

    Thread->RunningEvents.Running = true;

    Thread->CPUBackend->ExecuteDispatch(Thread->CurrentFrame);

    Thread->RunningEvents.WaitingToStart = false;
    Thread->RunningEvents.Running = false;

    // If it is the parent thread that died then just leave
    // XXX: This doesn't make sense when the parent thread doesn't outlive its children
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

  void FlushCodeRange(FEXCore::Core::InternalThreadState *Thread, uint64_t Start, uint64_t Length) {

    if (Thread->CTX->Config.SMCChecks == FEXCore::Config::CONFIG_SMC_MMAN) {
      auto lower = Thread->LookupCache->CodePages.lower_bound(Start >> 12);
      auto upper = Thread->LookupCache->CodePages.upper_bound((Start + Length) >> 12);

      for (auto it = lower; it != upper; it++) {
        for (auto Address: it->second)
          Context::RemoveCodeEntry(Thread, Address);
        it->second.clear();
      }
    }
  }

  void Context::RemoveCodeEntry(FEXCore::Core::InternalThreadState *Thread, uint64_t GuestRIP) {
    Thread->LocalIRCache.erase(GuestRIP);
    Thread->LookupCache->Erase(GuestRIP);
  }

  // Debug interface
  void Context::CompileRIP(FEXCore::Core::InternalThreadState *Thread, uint64_t RIP) {
    uint64_t RIPBackup = Thread->CurrentFrame->State.rip;
    Thread->CurrentFrame->State.rip = RIP;

    // Erase the RIP from all the storage backings if it exists
    RemoveCodeEntry(Thread, RIP);

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

  void Context::AddNamedRegion(uintptr_t Base, uintptr_t Size, uintptr_t Offset, const std::string &filename) {
    // TODO: Support overlapping maps and region splitting
    auto base_filename = std::filesystem::path(filename).filename().string();

    if (base_filename.size()) {
      auto filename_hash = XXH3_64bits(filename.c_str(), filename.size());

      auto fileid = base_filename + "-" + std::to_string(filename_hash) + "-";

      // append optimization flags to the fileid
      fileid += (Config.SMCChecks == FEXCore::Config::CONFIG_SMC_FULL) ? "S" : "s";
      fileid += Config.TSOEnabled ? "T" : "t";
      fileid += Config.ABILocalFlags ? "L" : "l";
      fileid += Config.ABINoPF ? "p" : "P";

      std::lock_guard<std::mutex> lk(AOTIRCacheLock);

      AddrToFile.insert({ Base, { Base, Size, Offset, fileid, filename, nullptr, false} });

      if (Config.AOTIRLoad && !AOTIRCache.contains(fileid) && AOTIRLoader) {
        auto streamfd = AOTIRLoader(fileid);
        if (streamfd != -1) {
          LoadAOTIRCache(streamfd);
          close(streamfd);
        }
      }
    }
  }

  void Context::RemoveNamedRegion(uintptr_t Base, uintptr_t Size) {
    std::lock_guard<std::mutex> lk(AOTIRCacheLock);
    // TODO: Support partial removing
    AddrToFile.erase(Base);
  }
}
