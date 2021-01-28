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

#include <fstream>
#include <unistd.h>

#include "Interface/Core/GdbServer.h"

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

    void *CompileCode(FEXCore::IR::IRListView<true> const *IR, FEXCore::Core::DebugData *DebugData, FEXCore::IR::RegisterAllocationData *RAData) override {
      LogMan::Msg::E("Fell back to default code handler at RIP: 0x%lx", ThreadState->State.State.rip);
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
    FallbackCPUFactory = FEXCore::Core::DefaultFallbackCore::CPUCreationFactory;
#ifdef BLOCKSTATS
    BlockData = std::make_unique<FEXCore::BlockSamplingData>();
#endif
  }

  bool Context::GetFilenameHash(std::string const &Filename, std::string &Hash) {
    // Calculate a hash for the input file
    std::ifstream Input (Filename.c_str(), std::ios::in | std::ios::binary | std::ios::ate);
    if (Input.is_open()) {
      std::streampos Size;
      Size = Input.tellg();
      Input.seekg(0, std::ios::beg);
      std::string Data;
      Data.resize(Size);
      Input.read(&Data.at(0), Size);
      Input.close();

      std::hash<std::string> string_hash;
      Hash = std::to_string(string_hash(Data));
      return true;
    }
    return false;
  }

  void Context::AddThreadRIPsToEntryList(FEXCore::Core::InternalThreadState *Thread) {
    for (auto &IR : Thread->IRLists) {
      EntryList.insert(IR.first);
    }
  }

  void Context::SaveEntryList() {
    std::string const &Filename = AppFilename();
    std::string hash_string;

    if (GetFilenameHash(Filename, hash_string)) {
      auto DataPath = FEXCore::Paths::GetEntryCachePath();
      DataPath += "Entries_" + hash_string;

      std::ofstream Output (DataPath.c_str(), std::ios::out | std::ios::binary);
      if (Output.is_open()) {
        for (auto Entry : EntryList) {
          Output.write(reinterpret_cast<char const*>(&Entry), sizeof(Entry));
        }
        Output.close();
      }
    }
  }

  void Context::LoadEntryList() {
    std::string const &Filename = AppFilename();
    std::string hash_string;

    if (GetFilenameHash(Filename, hash_string)) {
      auto DataPath = FEXCore::Paths::GetEntryCachePath();
      DataPath += "Entries_" + hash_string;

      std::ifstream Input (DataPath.c_str(), std::ios::in | std::ios::binary | std::ios::ate);
      if (Input.is_open()) {
        std::streampos Size;
        Size = Input.tellg();
        Input.seekg(0, std::ios::beg);
        std::string Data;
        Data.resize(Size);
        Input.read(&Data.at(0), Size);
        Input.close();
        size_t EntryCount = Size / sizeof(uint64_t);
        uint64_t *Entries = reinterpret_cast<uint64_t*>(&Data.at(0));

        for (size_t i = 0; i < EntryCount; ++i) {
          EntryList.insert(Entries[i]);
        }
      }
    }
  }

  Context::~Context() {
    {
      for (auto &Thread : Threads) {
        if (Thread->ExecutionThread.joinable()) {
          Thread->ExecutionThread.join();
        }
      }

      for (auto &Thread : Threads) {
        AddThreadRIPsToEntryList(Thread);
      }

      for (auto &Thread : Threads) {

        if (Thread->CompileService) {
          Thread->CompileService->Shutdown();
        }
        delete Thread;
      }
      Threads.clear();
    }

    SaveEntryList();
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

    FEXCore::Core::InternalThreadState *Thread = CreateThread(&NewThreadState, 0);

    // We are the parent thread
    ParentThread = Thread;

    Loader->MapMemoryRegion();

    Thread->State.State.gregs[X86State::REG_RSP] = Loader->SetupStack();

    Loader->LoadMemory();
    Loader->GetInitLocations(&InitLocations);

    Thread->State.State.rip = StartingRIP = Loader->DefaultRIP();

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
    Thread->CPUBackend->CallbackPtr(Thread, RIP);
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
      if (Thread->State.RunningEvents.Running.load()) {
        // Only attempt to stop this thread if it is running
        tgkill(Thread->State.ThreadManager.PID, Thread->State.ThreadManager.TID, SignalDelegator::SIGNAL_FOR_PAUSE);
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
      Thread->State.RunningEvents.WaitingToStart.store(true);
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
        ClearCodeCache(Thread, 0);
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
            Thread->State.ThreadManager.TID == tid) {
          // If we are callign stop from the current thread then we can ignore sending signals to this thread
          // This means that this thread is already gone
          continue;
        }
        else if (Thread->State.ThreadManager.TID == tid) {
          // We need to save the current thread for last to ensure all threads receive their stop signals
          CurrentThread = Thread;
          continue;
        }
        StopThread(Thread);
      }
    }

    // Stop the current thread now if we aren't ignoring it
    if (CurrentThread) {
      StopThread(CurrentThread);
    }
  }

  void Context::StopThread(FEXCore::Core::InternalThreadState *Thread) {
    if (Thread->State.RunningEvents.Running.load()) {
      Thread->SignalReason.store(FEXCore::Core::SignalEvent::SIGNALEVENT_STOP);
      tgkill(Thread->State.ThreadManager.PID, Thread->State.ThreadManager.TID, SignalDelegator::SIGNAL_FOR_PAUSE);
    }
  }

  void Context::SignalThread(FEXCore::Core::InternalThreadState *Thread, FEXCore::Core::SignalEvent Event) {
    if (Thread->State.RunningEvents.Running.load()) {
      Thread->SignalReason.store(Event);
      tgkill(Thread->State.ThreadManager.PID, Thread->State.ThreadManager.TID, SignalDelegator::SIGNAL_FOR_PAUSE);
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
    Thread->IntBackend->Initialize();
    Thread->FallbackBackend->Initialize();

    auto IRHandler = [Thread](uint64_t Addr, IR::IREmitter *IR) -> void {
      // Run the passmanager over the IR from the dispatcher
      Thread->PassManager->Run(IR);
      Thread->IRLists.try_emplace(Addr, IR->CreateIRCopy());
    };

    LocalLoader->AddIR(IRHandler);

    // Compile all of our cached entries
    LogMan::Msg::D("Precompiling: %ld blocks...", EntryList.size());
    for (auto Entry : EntryList) {
      CompileRIP(Thread, Entry);
    }
    LogMan::Msg::D("Done", EntryList.size());
  }

  void Context::InitializeThread(FEXCore::Core::InternalThreadState *Thread) {
    InitializeThreadData(Thread);

    // This will create the execution thread but it won't actually start executing
    Thread->ExecutionThread = std::thread(&Context::ExecutionThread, this, Thread);

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
      State->IntBackend = State->CPUBackend;
      break;
    case FEXCore::Config::CONFIG_IRJIT:
      State->PassManager->InsertRegisterAllocationPass(DoSRA);
      // Initialization order matters here, the IR JIT may want to have the interpreter created first to get a pointer to its execution function
      // This is useful for JIT to interpreter fallback support
      State->IntBackend.reset(FEXCore::CPU::CreateInterpreterCore(this, State, CompileThread));
      State->CPUBackend.reset(FEXCore::CPU::CreateJITCore(this, State, CompileThread));
      break;
    case FEXCore::Config::CONFIG_CUSTOM:      State->CPUBackend.reset(CustomCPUFactory(this, &State->State)); break;
    default: LogMan::Msg::A("Unknown core configuration");
    }

    if (!State->IntBackend) {
      State->IntBackend.reset(FEXCore::CPU::CreateInterpreterCore(this, State, CompileThread));
    }
    State->FallbackBackend.reset(FallbackCPUFactory(this, &State->State));
  }

  FEXCore::Core::InternalThreadState* Context::CreateThread(FEXCore::Core::CPUState *NewThreadState, uint64_t ParentTID) {
    FEXCore::Core::InternalThreadState *Thread{};

    // Grab the new thread object
    {
      std::lock_guard<std::mutex> lk(ThreadCreationMutex);
      Thread = Threads.emplace_back(new FEXCore::Core::InternalThreadState{});
      Thread->State.ThreadManager.TID = ++ThreadID;
    }

    // Copy over the new thread state to the new object
    memcpy(&Thread->State.State, NewThreadState, sizeof(FEXCore::Core::CPUState));

    // Set up the thread manager state
    Thread->State.ThreadManager.parent_tid = ParentTID;

    InitializeCompiler(Thread, false);

    LogMan::Throw::A(!Thread->FallbackBackend->NeedsOpDispatch(), "Fallback CPU backend must not require OpDispatch");
    return Thread;
  }

  void Context::AddBlockMapping(FEXCore::Core::InternalThreadState *Thread, uint64_t Address, void *Ptr) {
    Thread->LookupCache->AddBlockMapping(Address, Ptr);
  }

  void Context::ClearCodeCache(FEXCore::Core::InternalThreadState *Thread, uint64_t GuestRIP) {
    Thread->LookupCache->ClearCache();
    Thread->CPUBackend->ClearCache();
    Thread->IntBackend->ClearCache();

    if (Thread->CompileService) {
      Thread->CompileService->ClearCache(Thread, GuestRIP);
    }

    if (GuestRIP == 0) {
      Thread->IRLists.clear();
    }
    else {
      auto IR = Thread->IRLists.find(GuestRIP)->second.release();
      Thread->IRLists.clear();
      Thread->IRLists.try_emplace(GuestRIP, IR);
    }
  }

  std::tuple<void *, FEXCore::Core::DebugData *> Context::CompileCode(FEXCore::Core::InternalThreadState *Thread, uint64_t GuestRIP) {
    uint8_t const *GuestCode{};
      GuestCode = reinterpret_cast<uint8_t const*>(GuestRIP);

    // Do we already have this in the IR cache?
    auto IR = Thread->IRLists.find(GuestRIP);
    FEXCore::IR::IRListView<true> *IRList {};
    FEXCore::Core::DebugData *DebugData {};
    FEXCore::IR::RegisterAllocationData *RAData {};

    if (IR == Thread->IRLists.end()) {
      bool HadDispatchError {false};

      uint64_t TotalInstructions {0};
      uint64_t TotalInstructionsLength {0};

      if (!Thread->FrontendDecoder->DecodeInstructionsAtEntry(GuestCode, GuestRIP)) {
        if (Config.BreakOnFrontendFailure) {
          LogMan::Msg::E("Had Frontend decoder error");
          Stop(false /* Ignore Current Thread */);
        }
        return { nullptr, nullptr };
      }

      auto CodeBlocks = Thread->FrontendDecoder->GetDecodedBlocks();

      Thread->OpDispatcher->BeginFunction(GuestRIP, CodeBlocks);

      for (size_t j = 0; j < CodeBlocks->size(); ++j) {
        FEXCore::Frontend::Decoder::DecodedBlocks const &Block = CodeBlocks->at(j);
        // Set the block entry point
        Thread->OpDispatcher->SetNewBlockIfChanged(Block.Entry);


        uint64_t BlockInstructionsLength {};

        // Reset any block-specific state
        Thread->OpDispatcher->StartNewBlock();

        uint64_t InstsInBlock = Block.NumInstructions;

        if (Block.HasInvalidInstruction) {
          uint8_t GPRSize = Config.Is64BitMode ? 8 : 4;
          Thread->OpDispatcher->_ExitFunction(Thread->OpDispatcher->_Constant(GPRSize * 8, Block.Entry));
          break;
        }

        for (size_t i = 0; i < InstsInBlock; ++i) {
          FEXCore::X86Tables::X86InstInfo const* TableInfo {nullptr};
          FEXCore::X86Tables::DecodedInst const* DecodedInfo {nullptr};

          TableInfo = Block.DecodedInstructions[i].TableInfo;
          DecodedInfo = &Block.DecodedInstructions[i];

          if (Config.SMCChecks) {
            auto ExistingCodePtr = reinterpret_cast<uint64_t*>(Block.Entry + BlockInstructionsLength);

            auto CodeChanged = Thread->OpDispatcher->_ValidateCode(ExistingCodePtr[0], ExistingCodePtr[1], (uintptr_t)ExistingCodePtr, DecodedInfo->InstSize);

            auto InvalidateCodeCond = Thread->OpDispatcher->_CondJump(CodeChanged);

            auto CurrentBlock = Thread->OpDispatcher->GetCurrentBlock();
            auto CodeWasChangedBlock = Thread->OpDispatcher->CreateNewCodeBlockAtEnd();
            Thread->OpDispatcher->SetTrueJumpTarget(InvalidateCodeCond, CodeWasChangedBlock);

            Thread->OpDispatcher->SetCurrentCodeBlock(CodeWasChangedBlock);
            Thread->OpDispatcher->_RemoveCodeEntry(GuestRIP);
            Thread->OpDispatcher->_ExitFunction(Thread->OpDispatcher->_Constant(Block.Entry + BlockInstructionsLength));
            
            auto NextOpBlock = Thread->OpDispatcher->CreateNewCodeBlockAfter(CurrentBlock);

            Thread->OpDispatcher->SetFalseJumpTarget(InvalidateCodeCond, NextOpBlock);
            Thread->OpDispatcher->SetCurrentCodeBlock(NextOpBlock);
          }

          if (TableInfo->OpcodeDispatcher) {
            auto Fn = TableInfo->OpcodeDispatcher;
            std::invoke(Fn, Thread->OpDispatcher, DecodedInfo);
            if (Thread->OpDispatcher->HadDecodeFailure()) {
              if (Config.BreakOnFrontendFailure) {
                LogMan::Msg::E("Had OpDispatcher error at 0x%lx", GuestRIP);
                Stop(false /* Ignore Current Thread */);
              }
              HadDispatchError = true;
            }
            else {
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
              return { nullptr, nullptr };
            }
            else {
              uint8_t GPRSize = Config.Is64BitMode ? 8 : 4;

              // We had some instructions. Early exit
              Thread->OpDispatcher->_ExitFunction(Thread->OpDispatcher->_Constant(GPRSize * 8, Block.Entry + BlockInstructionsLength));
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

        if (Thread->CTX->Config.DumpIR=="stderr") {
          f = stderr;
        }
        else if (Thread->CTX->Config.DumpIR=="stdout") {
          f = stdout;
        }
        else {
          std::stringstream fileName;
          fileName << Thread->CTX->Config.DumpIR  << "/" << std::hex << GuestRIP << (RA ? "-post.ir" : "-pre.ir");

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

      if (Thread->CTX->Config.DumpIR != "no") {
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
          LogMan::Msg::A("Failed to parse ir\n");
        } else {
          std::stringstream out2;
          auto NewIR2 = reparsed->ViewIR();
          Dump(&out2, &NewIR2, nullptr);
          if (out.str() != out2.str()) {
            printf("one:\n %s\n", out.str().c_str());
            printf("two:\n %s\n", out2.str().c_str());
            LogMan::Msg::A("Parsed ir doesn't match\n");
          }
          delete reparsed;
        }
      }
      // Run the passmanager over the IR from the dispatcher
      Thread->PassManager->Run(Thread->OpDispatcher.get());

      if (Thread->CTX->Config.DumpIR != "no") {
        IRDumper(Thread->PassManager->GetRAPass() ? Thread->PassManager->GetRAPass()->GetAllocationData() : nullptr);
      }

      if (Thread->OpDispatcher->ShouldDump) {
        std::stringstream out;
        auto NewIR = Thread->OpDispatcher->ViewIR();
        FEXCore::IR::Dump(&out, &NewIR, Thread->PassManager->GetRAPass() ? Thread->PassManager->GetRAPass()->GetAllocationData() : nullptr);
        printf("IR 0x%lx:\n%s\n@@@@@\n", GuestRIP, out.str().c_str());
      }

      RAData = Thread->PassManager->GetRAPass() ? Thread->PassManager->GetRAPass()->GetAllocationData() : nullptr;

      // Create a copy of the IR and place it in this thread's IR cache
      auto AddedIR = Thread->IRLists.try_emplace(GuestRIP, Thread->OpDispatcher->CreateIRCopy());
      Thread->OpDispatcher->ResetWorkingList();

      auto Debugit = &Thread->DebugData.try_emplace(GuestRIP).first->second;
      Debugit->GuestCodeSize = TotalInstructionsLength;
      Debugit->GuestInstructionCount = TotalInstructions;

      IRList = AddedIR.first->second.get();
      DebugData = Debugit;
      Thread->Stats.BlocksCompiled.fetch_add(1);
    }
    else {
      IRList = IR->second.get();
      auto Debugit = Thread->DebugData.find(GuestRIP);
      if (Debugit != Thread->DebugData.end()) {
        DebugData = &Debugit->second;
      }
    }

    // Attempt to get the CPU backend to compile this code
    return { Thread->CPUBackend->CompileCode(IRList, DebugData, RAData), DebugData };
  }

  uintptr_t Context::CompileBlock(FEXCore::Core::InternalThreadState *Thread, uint64_t GuestRIP) {
    
    // Is the code in the cache?
    // The backends only check L1 and L2, not L3
    if (auto HostCode = Thread->LookupCache->FindBlock(GuestRIP)) {
      return HostCode;
    }

    void *CodePtr;
    FEXCore::Core::DebugData *DebugData;
    bool DecrementRefCount = false;

    if (Thread->CompileBlockReentrantRefCount != 0) {
      if (!Thread->CompileService) {
        Thread->CompileService = std::make_shared<FEXCore::CompileService>(this, Thread);
        Thread->CompileService->Initialize();
      }
      auto WorkItem = Thread->CompileService->CompileCode(GuestRIP);
      WorkItem->ServiceWorkDone.Wait();
      // Return here with the data in place
      Thread->IRLists.try_emplace(GuestRIP, WorkItem->IRList->CreateCopy());
      CodePtr = WorkItem->CodePtr;
      DebugData = WorkItem->DebugData;
      WorkItem->SafeToClear = true;
    } else {
      ++Thread->CompileBlockReentrantRefCount;
      DecrementRefCount = true;
      auto [Code, Data] = CompileCode(Thread, GuestRIP);
      CodePtr = Code;
      DebugData = Data;
    }

    if (CodePtr != nullptr) {
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

      if (DecrementRefCount)
        --Thread->CompileBlockReentrantRefCount;
      AddBlockMapping(Thread, GuestRIP, CodePtr);

      return (uintptr_t)CodePtr;
    }

    if (DecrementRefCount)
      --Thread->CompileBlockReentrantRefCount;

    return 0;
  }

  uintptr_t Context::CompileFallbackBlock(FEXCore::Core::InternalThreadState *Thread, uint64_t GuestRIP) {
    // We have ONE more chance to try and fallback to the fallback CPU backend
    // This will most likely fail since regular code use won't be using a fallback core.
    // It's mainly for testing new instruction encodings
    void *CodePtr = Thread->FallbackBackend->CompileCode(nullptr, nullptr, nullptr);
    AddBlockMapping(Thread, GuestRIP, CodePtr);
    return (uintptr_t)CodePtr;
  }

  void Context::ExecutionThread(FEXCore::Core::InternalThreadState *Thread) {
    Core::ThreadData.Thread = Thread;
    Thread->ExitReason = FEXCore::Context::ExitReason::EXIT_WAITING;

    // Let's do some initial bookkeeping here
    Thread->State.ThreadManager.TID = ::gettid();
    Thread->State.ThreadManager.PID = ::getpid();
    SignalDelegation->RegisterTLSState(Thread);
    ThunkHandler->RegisterTLSState(Thread);

    ++IdleWaitRefCount;

    LogMan::Msg::D("[%d] Waiting to run", Thread->State.ThreadManager.TID.load());

    // Now notify the thread that we are initialized
    Thread->ThreadWaiting.NotifyAll();

    if (Thread != Thread->CTX->ParentThread || StartPaused) {
      // Parent thread doesn't need to wait to run
      Thread->StartRunning.Wait();
    }

    LogMan::Msg::D("[%d] Running", Thread->State.ThreadManager.TID.load());

    Thread->ExitReason = FEXCore::Context::ExitReason::EXIT_NONE;

    Thread->State.RunningEvents.Running = true;

    Thread->CPUBackend->ExecuteDispatch(Thread);

    Thread->State.RunningEvents.WaitingToStart = false;
    Thread->State.RunningEvents.Running = false;

    // If it is the parent thread that died then just leave
    // XXX: This doesn't make sense when the parent thread doesn't outlive its children
    if (Thread->State.ThreadManager.parent_tid == 0) {
      CoreShuttingDown.store(true);
      Thread->ExitReason = FEXCore::Context::ExitReason::EXIT_SHUTDOWN;

      if (CustomExitHandler) {
        CustomExitHandler(Thread->State.ThreadManager.TID, Thread->ExitReason);
      }
    }

    --IdleWaitRefCount;
    IdleWaitCV.notify_all();

    SignalDelegation->UninstallTLSState(Thread);
  }

  void Context::RemoveCodeEntry(FEXCore::Core::InternalThreadState *Thread, uint64_t GuestRIP) {
    Thread->IRLists.erase(GuestRIP);
    Thread->DebugData.erase(GuestRIP);
    Thread->LookupCache->Erase(GuestRIP);

    if (Thread->CompileService) {
      Thread->CompileService->RemoveCodeEntry(GuestRIP);
    }
  }

  // Debug interface
  void Context::CompileRIP(FEXCore::Core::InternalThreadState *Thread, uint64_t RIP) {
    uint64_t RIPBackup = Thread->State.State.rip;
    Thread->State.State.rip = RIP;

    // Erase the RIP from all the storage backings if it exists
    Thread->IRLists.erase(RIP);
    Thread->DebugData.erase(RIP);
    Thread->LookupCache->Erase(RIP);

    // We don't care if compilation passes or not
    CompileBlock(Thread, RIP);

    Thread->State.State.rip = RIPBackup;
  }

  uint64_t Context::GetThreadCount() const {
    return Threads.size();
  }

  FEXCore::Core::RuntimeStats *Context::GetRuntimeStatsForThread(uint64_t Thread) {
    return &Threads[Thread]->Stats;
  }

  FEXCore::Core::CPUState Context::GetCPUState() {
    return ParentThread->State.State;
  }

  bool Context::GetDebugDataForRIP(uint64_t RIP, FEXCore::Core::DebugData *Data) {
    auto it = ParentThread->DebugData.find(RIP);
    if (it == ParentThread->DebugData.end()) {
      return false;
    }

    memcpy(Data, &it->second, sizeof(FEXCore::Core::DebugData));
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

	// XXX:
  // bool Context::FindIRForRIP(uint64_t RIP, FEXCore::IR::IntrusiveIRList **ir) {
  //   auto IR = ParentThread->IRLists.find(RIP);
  //   if (IR == ParentThread->IRLists.end()) {
  //     return false;
  //   }

  //   //*ir = &IR->second;
  //   return true;
  // }

  // void Context::SetIRForRIP(uint64_t RIP, FEXCore::IR::IntrusiveIRList *const ir) {
  //   //ParentThread->IRLists.try_emplace(RIP, *ir);
  // }

  FEXCore::Core::ThreadState *Context::GetThreadState() {
    return &ParentThread->State;
  }

  uint64_t HandleSyscall(FEXCore::HLE::SyscallHandler *Handler, FEXCore::Core::InternalThreadState *Thread, FEXCore::HLE::SyscallArguments *Args) {
    uint64_t Result{};
    Result = Handler->HandleSyscall(Thread, Args);
    return Result;
  }

}
