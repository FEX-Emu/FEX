#include "VM.h"
#include "LogManager.h"

#include <FEXCore/Config/Config.h>
#include <FEXCore/Core/CodeLoader.h>
#include <FEXCore/Core/Context.h>
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Core/CPUBackend.h>
#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/HLE/SyscallHandler.h>
#include <FEXCore/Debug/InternalThreadState.h>
#include <FEXCore/Memory/SharedMem.h>

namespace VMFactory {
  class VMCore final : public FEXCore::CPU::CPUBackend {
  public:
    explicit VMCore(FEXCore::Context::Context* CTX, FEXCore::Core::ThreadState *Thread, bool Fallback);
    ~VMCore() override;
    std::string GetName() override { return "VM Core"; }
    void* CompileCode(FEXCore::IR::IRListView<true> const *IR, FEXCore::Core::DebugData *DebugData) override;

    void *MapRegion(void *HostPtr, uint64_t VirtualGuestPtr, uint64_t Size) override {
      MemoryRegions.emplace_back(MemoryRegion{HostPtr, VirtualGuestPtr, Size});
      LogMan::Throw::A(!IsInitialized, "Tried mapping a new VM region post initialization");
      return HostPtr;
    }

    void Initialize() override;
    void ExecuteCode(FEXCore::Core::ThreadState *Thread);
    bool NeedsOpDispatch() override { return false; }

  private:
    FEXCore::Context::Context* CTX;
    FEXCore::Core::ThreadState *ThreadState;
    bool IsFallback;

    void CopyHostStateToGuest();
    void CopyGuestCPUToHost();
    void CopyHostMemoryToGuest();
    void CopyGuestMemoryToHost();

    void RecalculatePML4();

    struct MemoryRegion {
      void *HostPtr;
      uint64_t VirtualGuestPtr;
      uint64_t Size;
    };
    std::vector<MemoryRegion> MemoryRegions;

    struct PhysicalToVirtual {
      uint64_t PhysicalPtr;
      uint64_t GuestVirtualPtr;
      void *HostPtr;
      uint64_t Size;
    };
    std::vector<PhysicalToVirtual> PhysToVirt;

    SU::VM::VMInstance *VM;
    bool IsInitialized{false};
  };

  VMCore::~VMCore() {
    delete VM;
  }

  VMCore::VMCore(FEXCore::Context::Context* CTX, FEXCore::Core::ThreadState *Thread, bool Fallback)
    : CTX {CTX}
    , ThreadState {Thread}
    , IsFallback {Fallback} {
    VM = SU::VM::VMInstance::Create();
  }

  void VMCore::Initialize() {
    // Scan the mapped memory regions and see how much memory backing we need
    uint64_t PhysicalMemoryNeeded {};

    PhysToVirt.reserve(MemoryRegions.size());

    for (auto &Region : MemoryRegions) {
      PhysToVirt.emplace_back(PhysicalToVirtual{PhysicalMemoryNeeded, Region.VirtualGuestPtr, Region.HostPtr, Region.Size});
      PhysicalMemoryNeeded += Region.Size;
    }
    LogMan::Msg::D("We need 0x%lx physical memory", PhysicalMemoryNeeded);
    VM->SetPhysicalMemorySize(PhysicalMemoryNeeded);

    // Map these regions in the VM
    for (auto &Region : PhysToVirt) {
      if (!Region.Size) continue;
      VM->AddMemoryMapping(Region.GuestVirtualPtr, Region.PhysicalPtr, Region.Size);
    }

    // Initialize the VM with the memory mapping
    VM->Initialize();

    CopyHostMemoryToGuest();

    // Set initial register states
    CopyHostStateToGuest();

    IsInitialized = true;
  }

  void VMCore::CopyHostMemoryToGuest() {
    void *PhysBase = VM->GetPhysicalMemoryPointer();
    for (auto &Region : PhysToVirt) {
      void *GuestPtr = reinterpret_cast<void*>((reinterpret_cast<uintptr_t>(PhysBase) + Region.PhysicalPtr));
      memcpy(GuestPtr, Region.HostPtr, Region.Size);
    }
  }

  void VMCore::CopyGuestMemoryToHost() {
    void *PhysBase = VM->GetPhysicalMemoryPointer();
    for (auto &Region : PhysToVirt) {
      void *GuestPtr = reinterpret_cast<void*>((reinterpret_cast<uintptr_t>(PhysBase) + Region.PhysicalPtr));
      memcpy(Region.HostPtr, GuestPtr, Region.Size);
    }
  }

  void VMCore::CopyHostStateToGuest() {
    auto CompactRFlags = [](auto Arg) -> uint32_t {
      uint32_t Res = 2;
      for (int i = 0; i < 32; ++i) {
        Res |= Arg->flags[i] << i;
      }
      return Res;
    };

    SU::VM::VMInstance::RegisterState State;
    SU::VM::VMInstance::SpecialRegisterState SpecialState;

    State.rax    = ThreadState->State.gregs[FEXCore::X86State::REG_RAX];
    State.rbx    = ThreadState->State.gregs[FEXCore::X86State::REG_RBX];
    State.rcx    = ThreadState->State.gregs[FEXCore::X86State::REG_RCX];
    State.rdx    = ThreadState->State.gregs[FEXCore::X86State::REG_RDX];
    State.rsi    = ThreadState->State.gregs[FEXCore::X86State::REG_RSI];
    State.rdi    = ThreadState->State.gregs[FEXCore::X86State::REG_RDI];
    State.rsp    = ThreadState->State.gregs[FEXCore::X86State::REG_RSP];
    State.rbp    = ThreadState->State.gregs[FEXCore::X86State::REG_RBP];
    State.r8     = ThreadState->State.gregs[FEXCore::X86State::REG_R8];
    State.r9     = ThreadState->State.gregs[FEXCore::X86State::REG_R9];
    State.r10    = ThreadState->State.gregs[FEXCore::X86State::REG_R10];
    State.r11    = ThreadState->State.gregs[FEXCore::X86State::REG_R11];
    State.r12    = ThreadState->State.gregs[FEXCore::X86State::REG_R12];
    State.r13    = ThreadState->State.gregs[FEXCore::X86State::REG_R13];
    State.r14    = ThreadState->State.gregs[FEXCore::X86State::REG_R14];
    State.r15    = ThreadState->State.gregs[FEXCore::X86State::REG_R15];
    State.rip    = ThreadState->State.rip;
    State.rflags = CompactRFlags(&ThreadState->State);
    VM->SetRegisterState(State);

    SpecialState.fs.base = ThreadState->State.fs;
    SpecialState.gs.base = ThreadState->State.gs;

    VM->SetSpecialRegisterState(SpecialState);
  }

  void VMCore::CopyGuestCPUToHost() {
    auto UnpackRFLAGS = [](auto ThreadState, uint64_t GuestFlags) {
      for (int i = 0; i < 32; ++i) {
        ThreadState->flags[i] = (GuestFlags >> i) & 1;
      }
    };

    SU::VM::VMInstance::RegisterState State;
    SU::VM::VMInstance::SpecialRegisterState SpecialState;

    State = VM->GetRegisterState();
    SpecialState = VM->GetSpecialRegisterState();

    // Copy the VM's register state to our host context
    ThreadState->State.gregs[FEXCore::X86State::REG_RAX] = State.rax;
    ThreadState->State.gregs[FEXCore::X86State::REG_RBX] = State.rbx;
    ThreadState->State.gregs[FEXCore::X86State::REG_RCX] = State.rcx;
    ThreadState->State.gregs[FEXCore::X86State::REG_RDX] = State.rdx;
    ThreadState->State.gregs[FEXCore::X86State::REG_RSI] = State.rsi;
    ThreadState->State.gregs[FEXCore::X86State::REG_RDI] = State.rdi;
    ThreadState->State.gregs[FEXCore::X86State::REG_RSP] = State.rsp;
    ThreadState->State.gregs[FEXCore::X86State::REG_RBP] = State.rbp;
    ThreadState->State.gregs[FEXCore::X86State::REG_R8]  = State.r8;
    ThreadState->State.gregs[FEXCore::X86State::REG_R9]  = State.r9;
    ThreadState->State.gregs[FEXCore::X86State::REG_R10] = State.r10;
    ThreadState->State.gregs[FEXCore::X86State::REG_R11] = State.r11;
    ThreadState->State.gregs[FEXCore::X86State::REG_R12] = State.r12;
    ThreadState->State.gregs[FEXCore::X86State::REG_R13] = State.r13;
    ThreadState->State.gregs[FEXCore::X86State::REG_R14] = State.r14;
    ThreadState->State.gregs[FEXCore::X86State::REG_R15] = State.r15;
    ThreadState->State.rip = State.rip;
    UnpackRFLAGS(&ThreadState->State, State.rflags);

    ThreadState->State.fs = SpecialState.fs.base;
    ThreadState->State.gs = SpecialState.gs.base;
  }


  static void VMExecution(FEXCore::Core::ThreadState *Thread) {
    auto InternalThread = reinterpret_cast<FEXCore::Core::InternalThreadState*>(Thread);
    VMCore *Core = reinterpret_cast<VMCore*>(InternalThread->CPUBackend.get());
    Core->ExecuteCode(Thread);
  }

  static void VMExecutionFallback(FEXCore::Core::ThreadState *Thread) {
    auto InternalThread = reinterpret_cast<FEXCore::Core::InternalThreadState*>(Thread);
    VMCore *Core = reinterpret_cast<VMCore*>(InternalThread->FallbackBackend.get());
    Core->ExecuteCode(Thread);
  }

  void* VMCore::CompileCode([[maybe_unused]] FEXCore::IR::IRListView<true> const *IR, FEXCore::Core::DebugData *DebugData) {
    if (IsFallback)
      return reinterpret_cast<void*>(VMExecutionFallback);
    else
      return reinterpret_cast<void*>(VMExecution);
  }

  void VMCore::ExecuteCode(FEXCore::Core::ThreadState *Thread) {

#if 0
    auto DumpState = [this]() {
      LogMan::Msg::D("RIP: 0x%016lx", ThreadState->State.rip);
      LogMan::Msg::D("RAX              RBX             RCX            RDX");
      LogMan::Msg::D("0x%016lx 0x%016lx 0x%016lx 0x%016lx",
        ThreadState->State.gregs[FEXCore::X86State::REG_RAX],
        ThreadState->State.gregs[FEXCore::X86State::REG_RBX],
        ThreadState->State.gregs[FEXCore::X86State::REG_RCX],
        ThreadState->State.gregs[FEXCore::X86State::REG_RDX]);
      LogMan::Msg::D("RSI              RDI             RSP            RBP");
      LogMan::Msg::D("0x%016lx 0x%016lx 0x%016lx 0x%016lx",
        ThreadState->State.gregs[FEXCore::X86State::REG_RSI],
        ThreadState->State.gregs[FEXCore::X86State::REG_RDI],
        ThreadState->State.gregs[FEXCore::X86State::REG_RSP],
        ThreadState->State.gregs[FEXCore::X86State::REG_RBP]);
      LogMan::Msg::D("R8               R9              R10            R11");
      LogMan::Msg::D("0x%016lx 0x%016lx 0x%016lx 0x%016lx",
        ThreadState->State.gregs[FEXCore::X86State::REG_R8],
        ThreadState->State.gregs[FEXCore::X86State::REG_R9],
        ThreadState->State.gregs[FEXCore::X86State::REG_R10],
        ThreadState->State.gregs[FEXCore::X86State::REG_R11]);
      LogMan::Msg::D("R12              R13             R14            R15");
      LogMan::Msg::D("0x%016lx 0x%016lx 0x%016lx 0x%016lx",
        ThreadState->State.gregs[FEXCore::X86State::REG_R12],
        ThreadState->State.gregs[FEXCore::X86State::REG_R13],
        ThreadState->State.gregs[FEXCore::X86State::REG_R14],
        ThreadState->State.gregs[FEXCore::X86State::REG_R15]);
    };
#endif

    CopyHostMemoryToGuest();
    CopyHostStateToGuest();
    VM->SetStepping(true);

    if (VM->Run()) {
      int ExitReason = VM->ExitReason();

      // 4 = DEBUG
      if (ExitReason == 4 || ExitReason == 5) {
        CopyGuestCPUToHost();
        CopyGuestMemoryToHost();
      }

      // 5 = HLT
      if (ExitReason == 5) {
      }

      // 8 = Shutdown. Due to an unhandled error
      if (ExitReason == 8) {
        LogMan::Msg::E("Unhandled VM Fault");
        VM->Debug();
        CopyGuestCPUToHost();
      }

      // 9 = Failed Entry
      if (ExitReason == 9) {
        LogMan::Msg::E("Failed to enter VM due to: 0x%lx", VM->GetFailEntryReason());
      }
    }
    else {
      LogMan::Msg::E("VM failed to run");
    }
  }

  FEXCore::CPU::CPUBackend *CPUCreationFactory(FEXCore::Context::Context* CTX, FEXCore::Core::ThreadState *Thread) {
    return new VMCore(CTX, Thread, false);
  }

  FEXCore::CPU::CPUBackend *CPUCreationFactoryFallback(FEXCore::Context::Context* CTX, FEXCore::Core::ThreadState *Thread) {
    return new VMCore(CTX, Thread, true);
  }

}

