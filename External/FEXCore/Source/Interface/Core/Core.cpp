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
#include <filesystem>

#include "Interface/Core/GdbServer.h"

namespace {
  // Compression function for Merkle-Damgard construction.
  // This function is generated using the framework provided.
  #define mix(h) ({					\
        (h) ^= (h) >> 23;		\
        (h) *= 0x2127599bf4325c37ULL;	\
        (h) ^= (h) >> 47; })

  static uint64_t fasthash64(const void *buf, size_t len, uint64_t seed)
  {
    const uint64_t    m = 0x880355f21e6d1965ULL;
    const uint64_t *pos = (const uint64_t *)buf;
    const uint64_t *end = pos + (len / 8);
    const unsigned char *pos2;
    uint64_t h = seed ^ (len * m);
    uint64_t v;

    while (pos != end) {
      v  = *pos++;
      h ^= mix(v);
      h *= m;
    }

    pos2 = (const unsigned char*)pos;
    v = 0;

    switch (len & 7) {
    case 7: v ^= (uint64_t)pos2[6] << 48;
    case 6: v ^= (uint64_t)pos2[5] << 40;
    case 5: v ^= (uint64_t)pos2[4] << 32;
    case 4: v ^= (uint64_t)pos2[3] << 24;
    case 3: v ^= (uint64_t)pos2[2] << 16;
    case 2: v ^= (uint64_t)pos2[1] << 8;
    case 1: v ^= (uint64_t)pos2[0];
      h ^= mix(v);
      h *= m;
    }

    return mix(h);
  }
  #undef mix
}

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

    void *CompileCode(FEXCore::IR::IRListView const *IR, FEXCore::Core::DebugData *DebugData, FEXCore::IR::RegisterAllocationData *RAData) override {
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
    for (auto &IR : Thread->LocalIRCache) {
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

    // AOTIRCache needs manual clear
    for (auto &Mod: AOTIRCache) {
      for (auto &Entry: Mod.second) {
        delete Entry.second.IR;
        free(Entry.second.RAData);
      }
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
        if (Thread->State.RunningEvents.Running.load()) {
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
    if (Thread->State.RunningEvents.Running.exchange(false)) {
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

    auto IRHandler = [Thread](uint64_t Addr, IR::IREmitter *IR) -> void {
      // Run the passmanager over the IR from the dispatcher
      Thread->PassManager->Run(IR);
      Core::LocalIREntry Entry = {Addr, 0ULL, decltype(Entry.IR)(IR->CreateIRCopy()), decltype(Entry.RAData)(Thread->PassManager->GetRAPass() ? Thread->PassManager->GetRAPass()->PullAllocationData() : nullptr), decltype(Entry.DebugData)(new Core::DebugData())};
      Thread->LocalIRCache.insert({Addr, std::move(Entry)});
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
    case FEXCore::Config::CONFIG_CUSTOM:      State->CPUBackend.reset(CustomCPUFactory(this, &State->State)); break;
    default: ERROR_AND_DIE("Unknown core configuration");
    }
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

    return Thread;
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
      if (Config.BreakOnFrontendFailure) {
        LogMan::Msg::E("Had Frontend decoder error");
        Stop(false /* Ignore Current Thread */);
      }
      return { nullptr, nullptr, 0, 0, 0, 0 };
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

        if (Config.SMCChecks == FEXCore::Config::CONFIG_SMC_FULL) {
          auto ExistingCodePtr = reinterpret_cast<uint64_t*>(Block.Entry + BlockInstructionsLength);

          auto CodeChanged = Thread->OpDispatcher->_ValidateCode(ExistingCodePtr[0], ExistingCodePtr[1], (uintptr_t)ExistingCodePtr - GuestRIP, DecodedInfo->InstSize);

          auto InvalidateCodeCond = Thread->OpDispatcher->_CondJump(CodeChanged);

          auto CurrentBlock = Thread->OpDispatcher->GetCurrentBlock();
          auto CodeWasChangedBlock = Thread->OpDispatcher->CreateNewCodeBlockAtEnd();
          Thread->OpDispatcher->SetTrueJumpTarget(InvalidateCodeCond, CodeWasChangedBlock);

          Thread->OpDispatcher->SetCurrentCodeBlock(CodeWasChangedBlock);
          Thread->OpDispatcher->_RemoveCodeEntry();
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
            return { nullptr, nullptr, 0, 0, 0, 0};
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

    auto RAData = Thread->PassManager->GetRAPass() ? Thread->PassManager->GetRAPass()->PullAllocationData() : nullptr;
    auto IRList = Thread->OpDispatcher->CreateIRCopy();

    Thread->OpDispatcher->ResetWorkingList();

    return {IRList, RAData.release(), TotalInstructions, TotalInstructionsLength, Thread->FrontendDecoder->DecodedMinAddress, Thread->FrontendDecoder->DecodedMaxAddress - Thread->FrontendDecoder->DecodedMinAddress };
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

    if (IRList == nullptr && Config.AOTIRLoad) {
      std::lock_guard<std::mutex> lk(AOTIRCacheLock);
      auto file = AddrToFile.lower_bound(GuestRIP);
      if (file != AddrToFile.begin()) {
        --file;
        auto Mod = (decltype(AOTIRCache)::value_type::second_type*) file->second.CachedFileEntry;

        if (Mod == nullptr) {
          file->second.CachedFileEntry = Mod = &AOTIRCache[file->second.fileid];
        }

        auto AOTEntry = Mod->find(GuestRIP - file->second.Start + file->second.Offset);

        if (AOTEntry != Mod->end()) {
          // verify hash
          auto MappedStart = AOTEntry->second.start + file->second.Start  - file->second.Offset;
          auto hash = fasthash64((void*)MappedStart, AOTEntry->second.len, 0);
          if (hash == AOTEntry->second.crc) {
            IRList = AOTEntry->second.IR;
            //LogMan::Msg::D("using %s + %lx -> %lx\n", file->second.fileid.c_str(), AOTEntry->first, GuestRIP);
            // relocate
            IRList->GetHeader()->Entry = GuestRIP;

            RAData = AOTEntry->second.RAData;
            DebugData = new FEXCore::Core::DebugData();
            StartAddr = MappedStart;
            Length = AOTEntry->second.len;

            GeneratedIR = true;
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

    // Attempt to get the CPU backend to compile this code
    return { Thread->CPUBackend->CompileCode(IRList, DebugData, RAData), IRList, DebugData, RAData, GeneratedIR, StartAddr, Length};
  }

  bool Context::LoadAOTIRCache(std::istream &stream) {
    std::lock_guard<std::mutex> lk(AOTIRCacheLock);
    uint64_t tag;
    stream.read((char*)&tag, sizeof(tag));
    if (!stream || tag != 0xDEADBEEFC0D30002)
      return false;

    uint64_t ModCount;
    stream.read((char*)&ModCount, sizeof(ModCount));
    if (!stream)
      return false;

    for (int ModIndex = 0; ModIndex < ModCount; ModIndex++) {
      std::string Module;
      uint64_t ModSize;
      stream.read((char*)&ModSize, sizeof(ModSize));
      if (!stream)
        return false;

      Module.resize(ModSize);
      stream.read((char*)&Module[0], Module.size());
      if (!stream)
        return false;

      auto &Mod = AOTIRCache[Module];

      uint64_t FnCount;
      stream.read((char*)&FnCount, sizeof(FnCount));
      if (!stream)
        return false;

      LogMan::Msg::D("AOTIR: Module %s has %ld functions", Module.c_str(), FnCount);
      for (int FnIndex = 0; FnIndex < FnCount; FnIndex++) {
        uint64_t addr, start, crc, len;
        stream.read((char*)&addr, sizeof(addr));
        if (!stream)
          return false;

        stream.read((char*)&start, sizeof(start));
        if (!stream)
          return false;
        stream.read((char*)&len, sizeof(len));
        if (!stream)
          return false;
        stream.read((char*)&crc, sizeof(crc));
        if (!stream)
          return false;
        auto IR = new IR::IRListView(stream);
        if (!stream) {
          delete IR;
          return false;
        }
        uint64_t RASize;
        stream.read((char*)&RASize, sizeof(RASize));
        if (!stream) {
          delete IR;
          return false;
        }
        IR::RegisterAllocationData *RAData = (IR::RegisterAllocationData *)malloc(IR::RegisterAllocationData::Size(RASize));
        RAData->MapCount = RASize;

        stream.read((char*)&RAData->Map[0], sizeof(RAData->Map[0]) * RASize);

        if (!stream) {
          delete IR;
          return false;
        }
        stream.read((char*)&RAData->SpillSlotCount, sizeof(RAData->SpillSlotCount));
        if (!stream) {
          delete IR;
          return false;
        }

        IR->IsShared = true;
        RAData->IsShared = true;

        Mod.insert({addr, {start, len, crc, IR, RAData}});
      }
    }

    return true;
  }

  bool Context::WriteAOTIRCache(std::function<std::unique_ptr<std::ostream>(const std::string&)> CacheWriter) {
    std::lock_guard<std::mutex> lk(AOTIRCacheLock);

    bool rv = true;

    for (auto AOTModule: AOTIRCache) {
      if (AOTModule.second.size() == 0) {
        continue;
      }

      auto stream = CacheWriter(AOTModule.first);
      if (!*stream) {
        rv = false;
      }
      uint64_t tag = 0xDEADBEEFC0D30002;
      stream->write((char*)&tag, sizeof(tag));

      uint64_t ModCount = 1;
      stream->write((char*)&ModCount, sizeof(ModCount));
      auto ModSize = AOTModule.first.size();
      stream->write((char*)&ModSize, sizeof(ModSize));
      stream->write((char*)&AOTModule.first[0], ModSize);

      auto FnCount = AOTModule.second.size();
      stream->write((char*)&FnCount, sizeof(FnCount));

      for (auto entry: AOTModule.second) {
        stream->write((char*)&entry.first, sizeof(entry.first));
        stream->write((char*)&entry.second.start, sizeof(entry.second.start));
        stream->write((char*)&entry.second.len, sizeof(entry.second.len));
        stream->write((char*)&entry.second.crc, sizeof(entry.second.crc));
        entry.second.IR->Serialize(*stream);
        uint64_t RASize = entry.second.RAData->MapCount;
        stream->write((char*)&RASize, sizeof(RASize));
        stream->write((char*)&entry.second.RAData->Map[0], sizeof(entry.second.RAData->Map[0]) * RASize);
        stream->write((char*)&entry.second.RAData->SpillSlotCount, sizeof(entry.second.RAData->SpillSlotCount));
      }
    }

    return rv;
  }


  uintptr_t Context::CompileBlock(FEXCore::Core::InternalThreadState *Thread, uint64_t GuestRIP) {

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

    LogMan::Throw::A(CodePtr != nullptr, "Failed to compile code %lX", GuestRIP);

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
      if (Config.AOTIRCapture && RAData) {
        std::lock_guard<std::mutex> lk(AOTIRCacheLock);

        RAData->IsShared = true;
        IRList->IsShared = true;

        auto hash = fasthash64((void*)StartAddr, Length, 0);

        auto file = AddrToFile.lower_bound(StartAddr);
        if (file != AddrToFile.begin()) {
          --file;
          if (file->second.Start <= StartAddr && (file->second.Start + file->second.Len) >= (StartAddr + Length)) {
            AOTIRCache[file->second.fileid].insert({GuestRIP - file->second.Start + file->second.Offset, {StartAddr - file->second.Start + file->second.Offset, Length, hash, IRList, RAData}});
          }
        }
      }
    }

    if (DecrementRefCount)
      --Thread->CompileBlockReentrantRefCount;

    // Insert to lookup cache
    AddBlockMapping(Thread, GuestRIP, CodePtr, StartAddr, Length);

    return (uintptr_t)CodePtr;

    if (DecrementRefCount)
      --Thread->CompileBlockReentrantRefCount;

    return 0;
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
    uint64_t RIPBackup = Thread->State.State.rip;
    Thread->State.State.rip = RIP;

    // Erase the RIP from all the storage backings if it exists
    RemoveCodeEntry(Thread, RIP);

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

  FEXCore::Core::ThreadState *Context::GetThreadState() {
    return &ParentThread->State;
  }

  uint64_t HandleSyscall(FEXCore::HLE::SyscallHandler *Handler, FEXCore::Core::InternalThreadState *Thread, FEXCore::HLE::SyscallArguments *Args) {
    uint64_t Result{};
    Result = Handler->HandleSyscall(Thread, Args);
    return Result;
  }

  void Context::AddNamedRegion(uintptr_t Base, uintptr_t Size, uintptr_t Offset, const std::string &filename) {
    // TODO: Support overlapping maps and region splitting
    auto base_filename = std::filesystem::path(filename).filename().string();

    if (base_filename.size()) {
      auto filename_hash = fasthash64(filename.c_str(), filename.size(), 0xBAADF00D);

      auto fileid = base_filename + "-" + std::to_string(filename_hash) + "-";

      // append optimization flags to the fileid
      fileid += (Config.SMCChecks == FEXCore::Config::CONFIG_SMC_FULL) ? "S" : "s";
      fileid += Config.TSOEnabled ? "T" : "t";
      fileid += Config.ABILocalFlags ? "L" : "l";
      fileid += Config.ABINoPF ? "p" : "P";

      AddrToFile.insert({ Base, { Base, Size, Offset, fileid, nullptr } });

      if (Config.AOTIRLoad && !AOTIRCache.contains(fileid) && AOTIRLoader) {
        auto stream = AOTIRLoader(fileid);
        if (*stream) {
          LoadAOTIRCache(*stream);
        }
      }
    }
  }

  void Context::RemoveNamedRegion(uintptr_t Base, uintptr_t Size) {
    // TODO: Support partial removing
    AddrToFile.erase(Base);
  }
}
