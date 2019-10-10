#include "Common/MathUtils.h"
#include "Common/Paths.h"
#include "Interface/Context/Context.h"
#include "Interface/Core/BlockCache.h"
#include "Interface/Core/Core.h"
#include "Interface/Core/DebugData.h"
#include "Interface/Core/OpcodeDispatcher.h"
#include "Interface/Core/Interpreter/InterpreterCore.h"
#include "Interface/Core/JIT/JITCore.h"
#include "Interface/Core/LLVMJIT/LLVMCore.h"
#include "Interface/IR/Passes/RegisterAllocationPass.h"

#include <FEXCore/Config/Config.h>
#include <FEXCore/Core/CodeLoader.h>
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Core/CPUBackend.h>
#include <FEXCore/Core/X86Enums.h>

#include <fstream>

constexpr uint64_t STACK_OFFSET = 0xc000'0000;

constexpr uint64_t FS_OFFSET = 0xb000'0000;
constexpr uint64_t FS_SIZE = 0x1000'0000;

namespace FEXCore::CPU {
  bool CreateCPUCore(FEXCore::Context::Context *CTX) {
    // This should be used for generating things that are shared between threads
    CTX->CPUID.Init();
    return true;
  }
}

namespace FEXCore::Core {
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

    void *CompileCode(FEXCore::IR::IRListView<true> const *IR, FEXCore::Core::DebugData *DebugData) override {
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
  Context::Context()
    : FrontendDecoder {this}
    , SyscallHandler {this} {
    FallbackCPUFactory = FEXCore::Core::DefaultFallbackCore::CPUCreationFactory;
    PassManager.AddDefaultPasses();
    PassManager.AddDefaultValidationPasses();
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
    std::string const &Filename = SyscallHandler.GetFilename();
    std::string hash_string;

    if (GetFilenameHash(Filename, hash_string)) {
      auto DataPath = FEXCore::Paths::GetDataPath();
      DataPath += "/EntryCache/Entries_" + hash_string;

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
    std::string const &Filename = SyscallHandler.GetFilename();
    std::string hash_string;

    if (GetFilenameHash(Filename, hash_string)) {
      auto DataPath = FEXCore::Paths::GetDataPath();
      DataPath += "/EntryCache/Entries_" + hash_string;

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
    ShouldStop.store(true);

    Pause();
    {
      std::lock_guard<std::mutex> lk(ThreadCreationMutex);
      for (auto &Thread : Threads) {
        Thread->ExecutionThread.join();
      }

      for (auto &Thread : Threads) {
        AddThreadRIPsToEntryList(Thread);
      }

      for (auto &Thread : Threads) {
        delete Thread;
      }
      Threads.clear();
    }

    SaveEntryList();
  }

  bool Context::InitCore(FEXCore::CodeLoader *Loader) {
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
    NewThreadState.gs = 0;
    NewThreadState.fs = FS_OFFSET;
    NewThreadState.flags[1] = 1;

    FEXCore::Core::InternalThreadState *Thread = CreateThread(&NewThreadState, 0, 0);

    // We are the parent thread
    ParentThread = Thread;

    uintptr_t MemoryBase = MemoryMapper.GetBaseOffset<uintptr_t>(0);

    auto MemoryMapperFunction = [&](uint64_t Base, uint64_t Size) -> void* {
      Thread->BlockCache->HintUsedRange(Base, Base);
      return MapRegion(Thread, Base, Size, true);
    };

    Loader->MapMemoryRegion(MemoryMapperFunction);

    // Set up all of our memory mappings
    MapRegion(Thread, FS_OFFSET, FS_SIZE, true);

    void *StackPointer = MapRegion(Thread, STACK_OFFSET, Loader->StackSize(), true);
    Thread->State.State.gregs[X86State::REG_RSP] = Loader->SetupStack(StackPointer, STACK_OFFSET);

    // Now let the code loader setup memory
    auto MemoryWriterFunction = [&](void const *Data, uint64_t Addr, uint64_t Size) -> void {
      // Writes the machine code to be emulated in to memory
      memcpy(reinterpret_cast<void*>(MemoryBase + Addr), Data, Size);
    };

    Loader->LoadMemory(MemoryWriterFunction);
    Loader->GetInitLocations(&InitLocations);

    auto TLSSlotWriter = [&](void const *Data, uint64_t Size) -> void {
      memcpy(reinterpret_cast<void*>(MemoryBase + FS_OFFSET), Data, Size);
    };

    // Offset next thread's FS_OFFSET by slot size
    uint64_t SlotSize = Loader->InitializeThreadSlot(TLSSlotWriter);
    StartingRIP = Loader->DefaultRIP();

    InitializeThread(Thread);

    return true;
  }

  void Context::WaitForIdle() {
    do {
      bool AllPaused = true;
      {
        // Grab the mutex lock so a thread doesn't try and spin up while we are waiting
        for (size_t i = 0; i < Threads.size(); ++i) {
          if (Threads[i]->State.RunningEvents.Running.load() || Threads[i]->State.RunningEvents.WaitingToStart.load()) {
            AllPaused = false;
            break;
          }
        }
      }

      if (AllPaused)
        break;

      PauseWait.WaitFor(std::chrono::milliseconds(10));
    } while (true);
  }

  void Context::Pause() {
    // Tell all the threads that they should pause
    {
      std::lock_guard<std::mutex> lk(ThreadCreationMutex);
      for (auto &Thread : Threads) {
        Thread->State.RunningEvents.ShouldPause.store(true);
      }

      for (auto &Thread : Threads) {
        Thread->StartRunning.NotifyAll();
      }
      Running = true;
    }

    WaitForIdle();
    Running = false;
  }

  FEXCore::Context::ExitReason Context::RunLoop(bool WaitForIdle) {
    {
      // Spin up all the threads
      std::lock_guard<std::mutex> lk(ThreadCreationMutex);
      for (auto &Thread : Threads) {
        Thread->State.RunningEvents.ShouldPause.store(false);
        Thread->State.RunningEvents.WaitingToStart.store(true);
      }

      for (auto &Thread : Threads) {
        Thread->StartRunning.NotifyAll();
      }
      Running = true;
    }

    if (WaitForIdle) {
      this->WaitForIdle();
      return ParentThread->ExitReason;
    }

    return FEXCore::Context::ExitReason::EXIT_ASYNC_RUN;
  }

  void Context::InitializeThread(FEXCore::Core::InternalThreadState *Thread) {
    Thread->CPUBackend->Initialize();
    Thread->FallbackBackend->Initialize();

    // Compile all of our cached entries
    LogMan::Msg::D("Precompiling: %ld blocks...", EntryList.size());
    for (auto Entry : EntryList) {
      CompileRIP(Thread, Entry);
    }
    LogMan::Msg::D("Done", EntryList.size());

    // This will create the execution thread but it won't actually start executing
    Thread->ExecutionThread = std::thread(&Context::ExecutionThread, this, Thread);

    // Wait for the thread to have started
    Thread->ThreadWaiting.Wait();
  }

  void Context::RunThread(FEXCore::Core::InternalThreadState *Thread) {
    // Tell the thread to start executing
    Thread->StartRunning.NotifyAll();
  }

  FEXCore::Core::InternalThreadState* Context::CreateThread(FEXCore::Core::CPUState *NewThreadState, uint64_t ParentTID, uint64_t ChildTID) {
    FEXCore::Core::InternalThreadState *Thread{};

    // Grab the new thread object
    {
      std::lock_guard<std::mutex> lk(ThreadCreationMutex);
      Thread = Threads.emplace_back(new FEXCore::Core::InternalThreadState{});
      Thread->State.ThreadManager.TID = ++ThreadID;
    }

    Thread->OpDispatcher = std::make_unique<FEXCore::IR::OpDispatchBuilder>();
    Thread->BlockCache = std::make_unique<FEXCore::BlockCache>(this);
    Thread->CTX = this;

    // Copy over the new thread state to the new object
    memcpy(&Thread->State.State, NewThreadState, sizeof(FEXCore::Core::CPUState));

    // Set up the thread manager state
    Thread->State.ThreadManager.parent_tid = ParentTID;
    Thread->State.ThreadManager.child_tid = ChildTID;

    // Create CPU backend
    switch (Config.Core) {
    case FEXCore::Config::CONFIG_INTERPRETER: Thread->CPUBackend.reset(FEXCore::CPU::CreateInterpreterCore(this)); break;
    case FEXCore::Config::CONFIG_IRJIT:       Thread->CPUBackend.reset(FEXCore::CPU::CreateJITCore(this, Thread)); break;
    case FEXCore::Config::CONFIG_LLVMJIT:     Thread->CPUBackend.reset(FEXCore::CPU::CreateLLVMCore(Thread)); break;
    case FEXCore::Config::CONFIG_CUSTOM:      Thread->CPUBackend.reset(CustomCPUFactory(this, &Thread->State)); break;
    default: LogMan::Msg::A("Unknown core configuration");
    }

    Thread->FallbackBackend.reset(FallbackCPUFactory(this, &Thread->State));

    LogMan::Throw::A(!Thread->FallbackBackend->NeedsOpDispatch(), "Fallback CPU backend must not require OpDispatch");

    return Thread;
  }

  IR::RegisterAllocationPass *Context::GetRegisterAllocatorPass() {
    if (!RAPass) {
      RAPass = new IR::RegisterAllocationPass();
      PassManager.InsertPass(RAPass);
    }

    return RAPass;
  }

  uintptr_t Context::AddBlockMapping(FEXCore::Core::InternalThreadState *Thread, uint64_t Address, void *Ptr) {
    auto BlockMapPtr = Thread->BlockCache->AddBlockMapping(Address, Ptr);
    if (BlockMapPtr == 0) {
      Thread->BlockCache->ClearCache();
      BlockMapPtr = Thread->BlockCache->AddBlockMapping(Address, Ptr);
      LogMan::Throw::A(BlockMapPtr, "Couldn't add mapping after clearing mapping cache");
    }

    return BlockMapPtr;
  }

  uintptr_t Context::CompileBlock(FEXCore::Core::InternalThreadState *Thread, uint64_t GuestRIP) {
    void *CodePtr {nullptr};
    uint8_t const *GuestCode = MemoryMapper.GetPointer<uint8_t const*>(GuestRIP);

    uint64_t TotalInstructions {0};
    uint64_t TotalInstructionsLength {0};

    // Do we already have this in the IR cache?
    auto IR = Thread->IRLists.find(GuestRIP);
    FEXCore::IR::IRListView<true> *IRList {};
    FEXCore::Core::DebugData *DebugData {};

    if (IR == Thread->IRLists.end()) {
      bool HadDispatchError {false};
      [[maybe_unused]] bool HadRIPSetter {false};

      if (!FrontendDecoder.DecodeInstructionsInBlock(&GuestCode[TotalInstructionsLength], GuestRIP + TotalInstructionsLength)) {
        if (Config.BreakOnFrontendFailure) {
           LogMan::Msg::E("Had Frontend decoder error");
           ShouldStop = true;
        }
        return 0;
      }

      Thread->OpDispatcher->BeginFunction(GuestRIP);
      auto DecodedOps = FrontendDecoder.GetDecodedInsts();
      for (size_t i = 0; i < DecodedOps.second; ++i) {
        FEXCore::X86Tables::X86InstInfo const* TableInfo {nullptr};
        FEXCore::X86Tables::DecodedInst const* DecodedInfo {nullptr};

        TableInfo = DecodedOps.first->at(i).TableInfo;
        DecodedInfo = &DecodedOps.first->at(i);
        // if (FrontendDecoder.JumpTargets.find(DecodedInfo->PC) != FrontendDecoder.JumpTargets.end()) {
        //   Thread->OpDispatcher->_EndBlock(0);
        //   auto JumpTarget = Thread->OpDispatcher->_BeginBlock();
        //   Thread->OpDispatcher->Arguments.JumpTargets.emplace(DecodedInfo->PC, JumpTarget.location);
        // }

        // // Check our fixups to see if they still are necessary
        // auto fixup = Thread->OpDispatcher->Arguments.Fixups.find(DecodedInfo->PC);
        // if (fixup != Thread->OpDispatcher->Arguments.Fixups.end()) {
        //   IR::AlignmentType JumpTarget;
        //   auto it = Thread->OpDispatcher->Arguments.JumpTargets.find(DecodedInfo->PC);
        //   if (it != Thread->OpDispatcher->Arguments.JumpTargets.end()) {
        //     JumpTarget = it->second;
        //   }

        //   for (auto it : fixup->second) {
        //     switch (it.SourceCondJump->Op) {
        //     case FEXCore::IR::OP_CONDJUMP: {
        //       if (JumpTarget.IsInvalid()) {
        //         Thread->OpDispatcher->_EndBlock(0);
        //         JumpTarget = Thread->OpDispatcher->_BeginBlock().location;
        //       }
        //       auto CondJumpOp = it.SourceCondJump->CW<IR::IROp_CondJump>();
        //       CondJumpOp->Location = JumpTarget;
        //     break;
        //     }
        //     case FEXCore::IR::OP_JUMP: {
        //       if (JumpTarget.IsInvalid()) {
        //         Thread->OpDispatcher->_EndBlock(0);
        //         JumpTarget = Thread->OpDispatcher->_BeginBlock().location;
        //       }
        //       auto JumpOp = it.SourceCondJump->CW<IR::IROp_Jump>();
        //       JumpOp->Location = JumpTarget;
        //     break;
        //     }

        //     default:
        //       LogMan::Msg::A("Unknown fixup kind");
        //     break;
        //     }
        //   }

        //   // No longer need this fixup
        //   Thread->OpDispatcher->Arguments.Fixups.erase(fixup);
        // }

        if (TableInfo->OpcodeDispatcher) {
          auto Fn = TableInfo->OpcodeDispatcher;
          std::invoke(Fn, Thread->OpDispatcher, DecodedInfo);
          if (Thread->OpDispatcher->HadDecodeFailure()) {
            if (Config.BreakOnFrontendFailure) {
              LogMan::Msg::E("Had OpDispatcher error at 0x%lx", GuestRIP);
              ShouldStop = true;
            }
            HadDispatchError = true;
          }
          else {
            TotalInstructionsLength += DecodedInfo->InstSize;
            TotalInstructions++;
          }
        }
        else {
          // LogMan::Msg::E("Missing OpDispatcher at 0x%lx", GuestRIP);
          HadDispatchError = true;
        }

        // If we had a dispatch error then leave early
        if (HadDispatchError) {
          if (TotalInstructions == 0) {
            // Couldn't handle any instruction in op dispatcher
            Thread->OpDispatcher->ResetWorkingList();
            return 0;
          }
          else {
            // We had some instructions. Early exit
            break;
          }
        }

        // This is to make sure if we are stepping or make a block that is too large that we will still set the block
        if (!HadDispatchError && (TableInfo->Flags & X86Tables::InstFlags::FLAGS_SETS_RIP)) {
          HadRIPSetter = true;
        }

        if (TotalInstructions >= Config.MaxInstPerBlock) {
          break;
        }
      }

      //LogMan::Throw::A(Thread->OpDispatcher->Arguments.Fixups.empty(), "Still had fixups that weren't fixed!");

      if (!Thread->OpDispatcher->Information.HadUnconditionalExit)
      {
        Thread->OpDispatcher->CreateNewEndBlock(TotalInstructionsLength);
        Thread->OpDispatcher->CreateNewBeginBlock();
        Thread->OpDispatcher->ExitFunction();
        Thread->OpDispatcher->CreateNewEndBlock(0);
      }
      Thread->OpDispatcher->Finalize();

      // Run the passmanager over the IR from the dispatcher
      PassManager.Run(Thread->OpDispatcher.get());

      if (Thread->OpDispatcher->ShouldDump)
      {
        std::stringstream out;
        auto NewIR = Thread->OpDispatcher->ViewIR();
        FEXCore::IR::Dump(&out, &NewIR);
        printf("IR 0x%lx:\n%s\n@@@@@\n", GuestRIP, out.str().c_str());
      }

      // Create a copy of the IR and place it in this thread's IR cache
      auto AddedIR = Thread->IRLists.try_emplace(GuestRIP, Thread->OpDispatcher->CreateIRCopy());
      Thread->OpDispatcher->ResetWorkingList();

      auto Debugit = Thread->DebugData.try_emplace(GuestRIP);
      Debugit.first->second.GuestCodeSize = TotalInstructionsLength;
      Debugit.first->second.GuestInstructionCount = TotalInstructions;

      IRList = AddedIR.first->second.get();
      DebugData = &Debugit.first->second;
      Thread->Stats.BlocksCompiled.fetch_add(1);
    }
    else {
      IRList = IR->second.get();
    }

    // Attempt to get the CPU backend to compile this code
    CodePtr = Thread->CPUBackend->CompileCode(IRList, DebugData);

    if (CodePtr != nullptr) {
      // The core managed to compile the code.
      return AddBlockMapping(Thread, GuestRIP, CodePtr);
    }

    return 0;
  }

  using BlockFn = void (*)(FEXCore::Core::InternalThreadState *Thread);
  uintptr_t Context::CompileFallbackBlock(FEXCore::Core::InternalThreadState *Thread, uint64_t GuestRIP) {
    // We have ONE more chance to try and fallback to the fallback CPU backend
    // This will most likely fail since regular code use won't be using a fallback core.
    // It's mainly for testing new instruction encodings
    void *CodePtr = Thread->FallbackBackend->CompileCode(nullptr, nullptr);
    if (CodePtr) {
     uintptr_t Ptr = reinterpret_cast<uintptr_t >(AddBlockMapping(Thread, GuestRIP, CodePtr));
     return Ptr;
    }

    return 0;
  }

  void Context::ExecutionThread(FEXCore::Core::InternalThreadState *Thread) {
    Thread->ExitReason = FEXCore::Context::ExitReason::EXIT_WAITING;

    Thread->ThreadWaiting.NotifyAll();
    Thread->StartRunning.Wait();

    if (ShouldStop.load() || Thread->State.RunningEvents.ShouldStop.load()) {
      ShouldStop = true;
      Thread->State.RunningEvents.ShouldStop.store(true);
      Thread->State.RunningEvents.Running.store(false);
      Thread->ExitReason = FEXCore::Context::ExitReason::EXIT_SHUTDOWN;
      return;
    }

    Thread->ExitReason = FEXCore::Context::ExitReason::EXIT_NONE;

    Thread->State.RunningEvents.Running = true;
    Thread->State.RunningEvents.ShouldPause = false;
    constexpr uint32_t CoreDebugLevel = 0;

    bool Initializing = false;

    uint64_t InitializationStep = 0;
    if (Initializing) {
      Thread->State.State.rip = ~0ULL;
    }
    else {
      Thread->State.State.rip = StartingRIP;
    }

    if (Thread->CPUBackend->HasCustomDispatch()) {
      Thread->CPUBackend->ExecuteCustomDispatch(&Thread->State);
    }
    else {
      while (!ShouldStop.load() && !Thread->State.RunningEvents.ShouldStop.load()) {
        if (Initializing) {
          if (Thread->State.State.rip == ~0ULL) {
            if (InitializationStep < InitLocations.size()) {
              Thread->State.State.gregs[X86State::REG_RSP] -= 8;
              *MemoryMapper.GetPointer<uint64_t*>(Thread->State.State.gregs[X86State::REG_RSP]) = ~0ULL;
              LogMan::Msg::D("Going down init path: 0x%lx", InitLocations[InitializationStep]);
              Thread->State.State.rip = InitLocations[InitializationStep++];
            }
            else {
              Initializing = false;
              Thread->State.State.rip = StartingRIP;
            }
          }
        }
        uint64_t GuestRIP = Thread->State.State.rip;

        if (CoreDebugLevel >= 1) {
          char const *Name = LocalLoader->FindSymbolNameInRange(GuestRIP);
          LogMan::Msg::D(">>>>RIP: 0x%lx: '%s'", GuestRIP, Name ? Name : "<Unknown>");
        }

        if (!Thread->CPUBackend->NeedsOpDispatch()) {
          BlockFn Ptr = reinterpret_cast<BlockFn>(Thread->CPUBackend->CompileCode(nullptr, nullptr));
          Ptr(Thread);
        }
        else {
          // Do have have this block compiled?
          auto it = Thread->BlockCache->FindBlock(GuestRIP);
          if (it == 0) {
            // If not compile it
            it = CompileBlock(Thread, GuestRIP);
          }

          // Did we successfully compile this block?
          if (it != 0) {
            // Block is compiled, run it
            BlockFn Ptr = reinterpret_cast<BlockFn>(it);
            Ptr(Thread);
          }
          else {
            // We have ONE more chance to try and fallback to the fallback CPU backend
            // This will most likely fail since regular code use won't be using a fallback core.
            // It's mainly for testing new instruction encodings
            uintptr_t CodePtr = CompileFallbackBlock(Thread, GuestRIP);
            if (CodePtr) {
              BlockFn Ptr = reinterpret_cast<BlockFn>(CodePtr);
              Ptr(Thread);
            }
            else {
              // Let the frontend know that something has happened that is unhandled
              Thread->State.RunningEvents.ShouldPause = true;
              Thread->ExitReason = FEXCore::Context::ExitReason::EXIT_UNKNOWNERROR;
            }
          }
        }

        if (CoreDebugLevel >= 2) {
          int i = 0;
          LogMan::Msg::D("\tGPR[%d]: %016lx %016lx %016lx %016lx", i, Thread->State.State.gregs[i + 0], Thread->State.State.gregs[i + 1], Thread->State.State.gregs[i + 2], Thread->State.State.gregs[i + 3]);
          i += 4;
          LogMan::Msg::D("\tGPR[%d]: %016lx %016lx %016lx %016lx", i, Thread->State.State.gregs[i + 0], Thread->State.State.gregs[i + 1], Thread->State.State.gregs[i + 2], Thread->State.State.gregs[i + 3]);
          i += 4;
          LogMan::Msg::D("\tGPR[%d]: %016lx %016lx %016lx %016lx", i, Thread->State.State.gregs[i + 0], Thread->State.State.gregs[i + 1], Thread->State.State.gregs[i + 2], Thread->State.State.gregs[i + 3]);
          i += 4;
          LogMan::Msg::D("\tGPR[%d]: %016lx %016lx %016lx %016lx", i, Thread->State.State.gregs[i + 0], Thread->State.State.gregs[i + 1], Thread->State.State.gregs[i + 2], Thread->State.State.gregs[i + 3]);
          uint64_t PackedFlags{};
          for (i = 0; i < 32; ++i) {
            PackedFlags |= static_cast<uint64_t>(Thread->State.State.flags[i]) << i;
          }
          LogMan::Msg::D("\tFlags: %016lx", PackedFlags);
        }

        if (CoreDebugLevel >= 3) {
          int i = 0;
          LogMan::Msg::D("\tXMM[%d][0]: %016lx %016lx %016lx %016lx", i, Thread->State.State.xmm[i + 0][0], Thread->State.State.xmm[i + 1][0], Thread->State.State.xmm[i + 2][0], Thread->State.State.xmm[i + 3][0]);
          LogMan::Msg::D("\tXMM[%d][1]: %016lx %016lx %016lx %016lx", i, Thread->State.State.xmm[i + 0][1], Thread->State.State.xmm[i + 1][1], Thread->State.State.xmm[i + 2][1], Thread->State.State.xmm[i + 3][1]);

          i += 4;
          LogMan::Msg::D("\tXMM[%d][0]: %016lx %016lx %016lx %016lx", i, Thread->State.State.xmm[i + 0][0], Thread->State.State.xmm[i + 1][0], Thread->State.State.xmm[i + 2][0], Thread->State.State.xmm[i + 3][0]);
          LogMan::Msg::D("\tXMM[%d][1]: %016lx %016lx %016lx %016lx", i, Thread->State.State.xmm[i + 0][1], Thread->State.State.xmm[i + 1][1], Thread->State.State.xmm[i + 2][1], Thread->State.State.xmm[i + 3][1]);
          i += 4;
          LogMan::Msg::D("\tXMM[%d][0]: %016lx %016lx %016lx %016lx", i, Thread->State.State.xmm[i + 0][0], Thread->State.State.xmm[i + 1][0], Thread->State.State.xmm[i + 2][0], Thread->State.State.xmm[i + 3][0]);
          LogMan::Msg::D("\tXMM[%d][1]: %016lx %016lx %016lx %016lx", i, Thread->State.State.xmm[i + 0][1], Thread->State.State.xmm[i + 1][1], Thread->State.State.xmm[i + 2][1], Thread->State.State.xmm[i + 3][1]);
          i += 4;
          LogMan::Msg::D("\tXMM[%d][0]: %016lx %016lx %016lx %016lx", i, Thread->State.State.xmm[i + 0][0], Thread->State.State.xmm[i + 1][0], Thread->State.State.xmm[i + 2][0], Thread->State.State.xmm[i + 3][0]);
          LogMan::Msg::D("\tXMM[%d][1]: %016lx %016lx %016lx %016lx", i, Thread->State.State.xmm[i + 0][1], Thread->State.State.xmm[i + 1][1], Thread->State.State.xmm[i + 2][1], Thread->State.State.xmm[i + 3][1]);
          uint64_t PackedFlags{};
          for (i = 0; i < 32; ++i) {
            PackedFlags |= static_cast<uint64_t>(Thread->State.State.flags[i]) << i;
          }
          LogMan::Msg::D("\tFlags: %016lx", PackedFlags);
        }

        if (Thread->State.RunningEvents.ShouldStop.load()) {
          // If it is the parent thread that died then just leave
          // XXX: This doesn't make sense when the parent thread doesn't outlive its children
          if (Thread->State.ThreadManager.GetTID() == 1) {
            ShouldStop = true;
            Thread->ExitReason = FEXCore::Context::ExitReason::EXIT_SHUTDOWN;
          }
          break;
        }

        if (RunningMode == FEXCore::Context::CoreRunningMode::MODE_SINGLESTEP || Thread->State.RunningEvents.ShouldPause) {
          Thread->State.RunningEvents.Running = false;
          Thread->State.RunningEvents.WaitingToStart = false;

          // If something previously hasn't set the exit state then set it now
          if (Thread->ExitReason == FEXCore::Context::ExitReason::EXIT_NONE)
            Thread->ExitReason = FEXCore::Context::ExitReason::EXIT_DEBUG;

          PauseWait.NotifyAll();
          Thread->StartRunning.Wait();

          // If we set it to debug then set it back to none after this
          // We want to retain the state if the frontend decides to leave
          if (Thread->ExitReason == FEXCore::Context::ExitReason::EXIT_DEBUG)
            Thread->ExitReason = FEXCore::Context::ExitReason::EXIT_NONE;

          Thread->State.RunningEvents.Running = true;
        }
      }
    }

    Thread->State.RunningEvents.WaitingToStart = false;
    Thread->State.RunningEvents.Running = false;
  }

  // Debug interface
  void Context::CompileRIP(FEXCore::Core::InternalThreadState *Thread, uint64_t RIP) {
    uint64_t RIPBackup = Thread->State.State.rip;
    Thread->State.State.rip = RIP;

    // Erase the RIP from all the storage backings if it exists
    Thread->IRLists.erase(RIP);
    Thread->DebugData.erase(RIP);
    Thread->BlockCache->Erase(RIP);

    // We don't care if compilation passes or not
    CompileBlock(Thread, RIP);

    Thread->State.State.rip = RIPBackup;
  }

  void *Context::MapRegion(FEXCore::Core::InternalThreadState *Thread, uint64_t Offset, uint64_t Size, bool Fixed) {
    void *Ptr = MemoryMapper.MapRegion(Offset, Size, Fixed);
    Thread->CPUBackend->MapRegion(Ptr, Offset, Size);
    Thread->FallbackBackend->MapRegion(Ptr, Offset, Size);
    return Ptr;
  }

  void Context::MirrorRegion(FEXCore::Core::InternalThreadState *Thread, void *HostPtr, uint64_t Offset, uint64_t Size) {
    Thread->CPUBackend->MapRegion(HostPtr, Offset, Size);
    Thread->FallbackBackend->MapRegion(HostPtr, Offset, Size);
  }

  void *Context::ShmBase() {
    return MemoryMapper.GetMemoryBase();
  }

  void Context::CopyMemoryMapping([[maybe_unused]] FEXCore::Core::InternalThreadState*, FEXCore::Core::InternalThreadState *ChildThread) {
    auto Regions = MemoryMapper.MappedRegions;
    for (auto const& Region : Regions) {
      ChildThread->CPUBackend->MapRegion(Region.Ptr, Region.Offset, Region.Size);
      ChildThread->FallbackBackend->MapRegion(Region.Ptr, Region.Offset, Region.Size);
    }
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

  void Context::GetMemoryRegions(std::vector<FEXCore::Memory::MemRegion> *Regions) {
    Regions->clear();
    Regions->resize(MemoryMapper.MappedRegions.size());
    memcpy(&Regions->at(0), &MemoryMapper.MappedRegions.at(0), sizeof(FEXCore::Memory::MemRegion) * MemoryMapper.MappedRegions.size());
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
    uintptr_t HostCode = ParentThread->BlockCache->FindBlock(RIP);
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

}
