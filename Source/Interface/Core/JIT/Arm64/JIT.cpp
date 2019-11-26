#include "Interface/Context/Context.h"

#include "Interface/Core/BlockCache.h"
#include "Interface/Core/InternalThreadState.h"

#include "Interface/HLE/Syscalls.h"

#include "Interface/IR/Passes/RegisterAllocationPass.h"

#if _M_X86_64
#define VIXL_INCLUDE_SIMULATOR_AARCH64
#include "aarch64/simulator-aarch64.h"
#endif
#include "aarch64/assembler-aarch64.h"
#include "aarch64/cpu-aarch64.h"
#include "aarch64/disasm-aarch64.h"
#include "aarch64/macro-assembler-aarch64.h"

#include <FEXCore/Core/CPUBackend.h>
#include <FEXCore/IR/IR.h>
#include <FEXCore/IR/IntrusiveIRList.h>

#include <sys/mman.h>

namespace FEXCore::CPU {
using namespace vixl;
using namespace vixl::aarch64;

#define MEM_BASE x28
#define STATE x27
#define TMP1 x1
#define TMP2 x2

#define VTMP1 v1
#define VTMP2 v2
#define VTMP3 v3

const std::array<aarch64::Register, 13> RA64 = {
  x4, x5, x6, x7, x8, x9,
  x10, x11, x12, x13, x14, x15,
  /*x16, x17,*/ // We can't use these until we move away from the MacroAssembler
  x18};
const std::array<aarch64::Register, 13> RA32 = {
  w4, w5, w6, w7, w8, w9,
  w10, w11, w12, w13, w14, w15,
  /*w16, w17,*/
  w18};

//  v8..v15 = (lower 64bits) Callee saved
const std::array<aarch64::VRegister, 22> RAFPR = {
  v3, v4, v5, v6, v7, v8, v16,
  v17, v18, v19, v20, v21, v22,
  v23, v24, v25, v26, v27, v28,
  v29, v30, v31};

static uint64_t SyscallThunk(FEXCore::SyscallHandler *Handler, FEXCore::Core::InternalThreadState *Thread, FEXCore::HLE::SyscallArguments *Args) {
  return Handler->HandleSyscall(Thread, Args);
}

static void CPUIDThunk(FEXCore::CPUIDEmu *CPUID, uint64_t Function, FEXCore::CPUIDEmu::FunctionResults *Results) {
  FEXCore::CPUIDEmu::FunctionResults Res = CPUID->RunFunction(Function);
  memcpy(Results, &Res, sizeof(FEXCore::CPUIDEmu::FunctionResults));
}

static uint64_t CompileBlockThunk(FEXCore::Context::Context* CTX, FEXCore::Core::InternalThreadState *Thread, uint64_t RIP) {
  uint64_t Result = CTX->CompileBlock(Thread, RIP);
  return Result;
}

static uint64_t CompileFallbackBlockThunk(FEXCore::Context::Context* CTX, FEXCore::Core::InternalThreadState *Thread, uint64_t RIP) {
  uint64_t Result = CTX->CompileFallbackBlock(Thread, RIP);
  return Result;
}

// XXX: Switch from MacroAssembler to Assembler once we drop the simulator
class JITCore final : public CPUBackend, public vixl::aarch64::MacroAssembler  {
public:
  explicit JITCore(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread);
  ~JITCore() override;
  std::string GetName() override { return "JIT"; }
  void *CompileCode(FEXCore::IR::IRListView<true> const *IR, FEXCore::Core::DebugData *DebugData) override;

  void *MapRegion(void* HostPtr, uint64_t, uint64_t) override { return HostPtr; }

  bool NeedsOpDispatch() override { return true; }

#if _M_X86_64
  void SimulationExecution(FEXCore::Core::InternalThreadState *Thread);
#endif

  bool HasCustomDispatch() const override { return CustomDispatchGenerated; }

#if _M_X86_64
  void ExecuteCustomDispatch(FEXCore::Core::ThreadState *Thread) override;
#else
  void ExecuteCustomDispatch(FEXCore::Core::ThreadState *Thread) override {
    DispatchPtr(reinterpret_cast<FEXCore::Core::InternalThreadState*>(Thread));
  }
#endif

private:
  FEXCore::Context::Context *CTX;
  FEXCore::Core::InternalThreadState *State;
  FEXCore::IR::IRListView<true> const *CurrentIR;

  std::map<IR::OrderedNodeWrapper::NodeOffsetType, aarch64::Label> JumpTargets;

  /**
   * @name Register Allocation
   * @{ */
  constexpr static uint32_t NumGPRs = RA64.size();
  constexpr static uint32_t NumFPRs = RAFPR.size();
  constexpr static uint32_t RegisterCount = NumGPRs + NumFPRs;
  constexpr static uint32_t RegisterClasses = 2;

  constexpr static uint32_t GPRBase = 0;
  constexpr static uint32_t GPRClass = IR::RegisterAllocationPass::GPRClass;
  constexpr static uint32_t FPRBase = NumGPRs;
  constexpr static uint32_t FPRClass = IR::RegisterAllocationPass::FPRClass;

  IR::RegisterAllocationPass::RegisterSet *RASet;
  /**  @} */

  constexpr static uint8_t RA_32 = 0;
  constexpr static uint8_t RA_64 = 1;
  constexpr static uint8_t RA_FPR = 2;

  IR::RegisterAllocationPass::RegisterGraph *Graph;
  uint32_t GetPhys(uint32_t Node);

  template<uint8_t RAType>
  aarch64::Register GetSrc(uint32_t Node);

  template<uint8_t RAType>
  aarch64::Register GetDst(uint32_t Node);

  aarch64::VRegister GetSrc(uint32_t Node);
  aarch64::VRegister GetDst(uint32_t Node);

  struct LiveRange {
    uint32_t Begin;
    uint32_t End;
  };

  std::vector<LiveRange> LiveRanges;
#if DEBUG || _M_X86_64
  vixl::aarch64::Decoder Decoder;
#endif
  vixl::aarch64::CPU CPU;

#if DEBUG
  vixl::aarch64::Disassembler Disasm;
#endif

#if _M_X86_64
  vixl::aarch64::Simulator Sim;
  std::unordered_map<uint64_t, std::pair<uint64_t, uint64_t>> HostToGuest;
#endif
  void LoadConstant(vixl::aarch64::Register Reg, uint64_t Constant);

  void CreateCustomDispatch(FEXCore::Core::InternalThreadState *Thread);
  bool CustomDispatchGenerated {false};
  using CustomDispatch = void(*)(FEXCore::Core::InternalThreadState *Thread);
  CustomDispatch DispatchPtr{};
  IR::RegisterAllocationPass *RAPass;

#if _M_X86_64
  uint64_t CustomDispatchEnd;
#endif
};

#if _M_X86_64
void JITCore::ExecuteCustomDispatch(FEXCore::Core::ThreadState *Thread) {
  PrintDisassembler PrintDisasm(stdout);
  PrintDisasm.DisassembleBuffer(vixl::aarch64::Instruction::Cast(DispatchPtr), vixl::aarch64::Instruction::Cast(CustomDispatchEnd));

  Sim.WriteXRegister(0, reinterpret_cast<uint64_t>(Thread));
  Sim.RunFrom(vixl::aarch64::Instruction::Cast(DispatchPtr));
}

static void SimulatorExecution(FEXCore::Core::InternalThreadState *Thread) {
  JITCore *Core = reinterpret_cast<JITCore*>(Thread->CPUBackend.get());
  Core->SimulationExecution(Thread);
}

void JITCore::SimulationExecution(FEXCore::Core::InternalThreadState *Thread) {
  using namespace vixl::aarch64;
  auto SimulatorAddress = HostToGuest[Thread->State.State.rip];
  // PrintDisassembler PrintDisasm(stdout);
  // PrintDisasm.DisassembleBuffer(vixl::aarch64::Instruction::Cast(SimulatorAddress.first), vixl::aarch64::Instruction::Cast(SimulatorAddress.second));

  Sim.WriteXRegister(0, reinterpret_cast<uint64_t>(Thread));
  Sim.RunFrom(vixl::aarch64::Instruction::Cast(SimulatorAddress.first));
}

#endif

JITCore::JITCore(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread)
  : vixl::aarch64::MacroAssembler(1024 * 1024 * 128, vixl::aarch64::PositionDependentCode)
  , CTX {ctx}
  , State {Thread}
#if _M_X86_64
  , Sim {&Decoder}
#endif
{
  // XXX: Set this to a real minimum feature set in the future
  SetCPUFeatures(vixl::CPUFeatures::All());

  RAPass = CTX->GetRegisterAllocatorPass();
  RAPass->SetSupportsSpills(true);

  RASet = RAPass->AllocateRegisterSet(RegisterCount, RegisterClasses);
  RAPass->AddRegisters(RASet, GPRClass, GPRBase, NumGPRs);
  RAPass->AddRegisters(RASet, FPRClass, FPRBase, NumFPRs);

  Graph = RAPass->AllocateRegisterGraph(RASet, 9000);
	LiveRanges.resize(9000);

  // Just set the entire range as executable
  auto Buffer = GetBuffer();
  mprotect(Buffer->GetOffsetAddress<void*>(0), Buffer->GetCapacity(), PROT_READ | PROT_WRITE | PROT_EXEC);
#if DEBUG
  Decoder.AppendVisitor(&Disasm)
#endif
#if _M_X86_64
  Sim.SetCPUFeatures(vixl::CPUFeatures::All());
#endif
  CPU.SetUp();
  SetAllowAssembler(true);
  CreateCustomDispatch(Thread);
}

JITCore::~JITCore() {
  RAPass->FreeRegisterGraph();
  RAPass->FreeRegisterSet(RASet);
}

void JITCore::LoadConstant(vixl::aarch64::Register Reg, uint64_t Constant) {
  bool Is64Bit = Reg.IsX();
  int Segments = Is64Bit ? 4 : 2;

  movz(Reg, (Constant) & 0xFFFF, 0);
  for (int i = 1; i < Segments; ++i) {
    uint16_t Part = (Constant >> (i * 16)) & 0xFFFF;
    if (Part) {
      movk(Reg, Part, i * 16);
    }
  }
}

uint32_t JITCore::GetPhys(uint32_t Node) {
  uint32_t Reg = RAPass->GetNodeRegister(Node);

  if (Reg < FPRBase)
    return Reg;
  else if (Reg != ~0U)
    return Reg - FPRBase;
  else
    LogMan::Msg::A("Couldn't Allocate register for node: ssa%d", Node);

  return ~0U;
}

template<uint8_t RAType>
aarch64::Register JITCore::GetSrc(uint32_t Node) {
  uint32_t Reg = GetPhys(Node);
  if (RAType == RA_64)
    return RA64[Reg];
  else if (RAType == RA_32)
    return RA32[Reg];
}

template<uint8_t RAType>
aarch64::Register JITCore::GetDst(uint32_t Node) {
  uint32_t Reg = GetPhys(Node);
  if (RAType == RA_64)
    return RA64[Reg];
  else if (RAType == RA_32)
    return RA32[Reg];
}

aarch64::VRegister JITCore::GetSrc(uint32_t Node) {
  uint32_t Reg = GetPhys(Node);
  return RAFPR[Reg];
}

aarch64::VRegister JITCore::GetDst(uint32_t Node) {
  uint32_t Reg = GetPhys(Node);
  return RAFPR[Reg];
}

void *JITCore::CompileCode([[maybe_unused]] FEXCore::IR::IRListView<true> const *IR, [[maybe_unused]] FEXCore::Core::DebugData *DebugData) {
  using namespace aarch64;
  JumpTargets.clear();
  CurrentIR = IR;

  uintptr_t ListBegin = CurrentIR->GetListData();
  uintptr_t DataBegin = CurrentIR->GetData();

  LogMan::Throw::A(RAPass->HasFullRA(), "Arm64 JIT only works with RA");

  uint32_t SpillSlots = RAPass->SpillSlots();

  // AAPCS64
  // r30      = LR
  // r29      = FP
  // r19..r28 = Callee saved
  // r18      = Platform Register (Matters if we target Windows or iOS)
  // r16..r17 = Inter-procedure scratch
  //  r9..r15 = Temp
  //  r8      = Indirect Result
  //  r0...r7 = Parameter/Results
  //
  //  FPRS:
  //  v8..v15 = (lower 64bits) Callee saved

  // Our allocation:
  // X0 = ThreadState
  // X1 = MemBase
  //
  // X1-X3 = Temp
  // X4-r18 = RA

  auto Buffer = GetBuffer();
  auto Entry = Buffer->GetOffsetAddress<uint64_t>(GetCursorOffset());

  if (!CustomDispatchGenerated) {
    void *Memory = CTX->MemoryMapper.GetMemoryBase();
    LoadConstant(MEM_BASE, (uint64_t)Memory);
    mov(STATE, x0);
  }

  if (SpillSlots) {
    sub(sp, sp, SpillSlots * 16);
  }

  auto HeaderIterator = CurrentIR->begin();
  IR::OrderedNodeWrapper *HeaderNodeWrapper = HeaderIterator();
  IR::OrderedNode *HeaderNode = HeaderNodeWrapper->GetNode(ListBegin);
  auto HeaderOp = HeaderNode->Op(DataBegin)->CW<FEXCore::IR::IROp_IRHeader>();
  LogMan::Throw::A(HeaderOp->Header.Op == IR::OP_IRHEADER, "First op wasn't IRHeader");

  IR::OrderedNode *BlockNode = HeaderOp->Blocks.GetNode(ListBegin);

  while (1) {
    using namespace FEXCore::IR;
    auto BlockIROp = BlockNode->Op(DataBegin)->CW<FEXCore::IR::IROp_CodeBlock>();
    LogMan::Throw::A(BlockIROp->Header.Op == IR::OP_CODEBLOCK, "IR type failed to be a code block");

    // We grab these nodes this way so we can iterate easily
    auto CodeBegin = CurrentIR->at(BlockIROp->Begin);
    auto CodeLast = CurrentIR->at(BlockIROp->Last);

    {
      uint32_t Node = BlockNode->Wrapped(ListBegin).ID();
      auto IsTarget = JumpTargets.find(Node);
      if (IsTarget == JumpTargets.end()) {
        IsTarget = JumpTargets.try_emplace(Node).first;
      }

      bind(&IsTarget->second);
    }

    while (1) {
      OrderedNodeWrapper *WrapperOp = CodeBegin();
      OrderedNode *RealNode = WrapperOp->GetNode(ListBegin);
      FEXCore::IR::IROp_Header *IROp = RealNode->Op(DataBegin);
      uint8_t OpSize = IROp->Size;
      uint32_t Node = WrapperOp->ID();

      if (0) {
        std::stringstream Inst;
        auto Name = FEXCore::IR::GetName(IROp->Op);

        if (IROp->HasDest) {
          uint32_t PhysReg = RAPass->GetNodeRegister(Node);
          if (PhysReg >= FPRBase)
            Inst << "\tFPR" << GetPhys(Node) << " = " << Name << " ";
          else
            Inst << "\tReg" << GetPhys(Node) << " = " << Name << " ";
        }
        else {
          Inst << "\t" << Name << " ";
        }

        uint8_t NumArgs = IR::GetArgs(IROp->Op);
        for (uint8_t i = 0; i < NumArgs; ++i) {
          uint32_t ArgNode = IROp->Args[i].ID();
          uint32_t PhysReg = RAPass->GetNodeRegister(ArgNode);
          if (PhysReg >= FPRBase)
            Inst << "FPR" << GetPhys(ArgNode) << (i + 1 == NumArgs ? "" : ", ");
          else
            Inst << "Reg" << GetPhys(ArgNode) << (i + 1 == NumArgs ? "" : ", ");
        }
      }

      switch (IROp->Op) {
      case IR::OP_BEGINBLOCK: {
        auto IsTarget = JumpTargets.find(Node);
        if (IsTarget == JumpTargets.end()) {
          IsTarget = JumpTargets.try_emplace(Node).first;
        }
        else {
        }

        bind(&IsTarget->second);
        break;
      }
      case IR::OP_ENDBLOCK: {
        auto Op = IROp->C<IR::IROp_EndBlock>();
        if (Op->RIPIncrement) {
          ldr(TMP1, MemOperand(STATE, offsetof(FEXCore::Core::CPUState, rip)));
          add(TMP1, TMP1, Operand(Op->RIPIncrement));
          str(TMP1,  MemOperand(STATE, offsetof(FEXCore::Core::CPUState, rip)));
        }
        break;
      }
      case IR::OP_EXITFUNCTION: {
        if (SpillSlots) {
          add(sp, sp, SpillSlots * 16);
        }

        ret();
        break;
      }
      case IR::OP_SYSCALL: {
        auto Op = IROp->C<IR::IROp_Syscall>();
        // Arguments are passed as follows:
        // X0: SyscallHandler
        // X1: ThreadState
        // X2: Pointer to SyscallArguments

        uint64_t SPOffset = AlignUp((RA64.size() + 7 + 1) * 8, 16);

        sub(sp, sp, SPOffset);
        for (uint32_t i = 0; i < 7; ++i)
          str(GetSrc<RA_64>(Op->Header.Args[i].ID()), MemOperand(sp, 0 + i * 8));

        int i = 0;
        for (auto RA : RA64) {
          str(RA, MemOperand(sp, 7 * 8 + i * 8));
          i++;
        }
        str(lr,       MemOperand(sp, 7 * 8 + RA64.size() * 8 + 0 * 8));

        LoadConstant(x0, reinterpret_cast<uint64_t>(&CTX->SyscallHandler));
        mov(x1, STATE);
        mov(x2, sp);

#if _M_X86_64
        CallRuntime(SyscallThunk);
#else
        using ClassPtrType = uint64_t (FEXCore::SyscallHandler::*)(FEXCore::Core::InternalThreadState *, FEXCore::HLE::SyscallArguments *);
        union PtrCast {
          ClassPtrType ClassPtr;
          uintptr_t Data;
        };

        PtrCast Ptr;
        Ptr.ClassPtr = &FEXCore::SyscallHandler::HandleSyscall;
        LoadConstant(x3, Ptr.Data);
        blr(x3);
#endif

        // Result is now in x0
        // Fix the stack and any values that were stepped on
        i = 0;
        for (auto RA : RA64) {
          ldr(RA, MemOperand(sp, 7 * 8 + i * 8));
          i++;
        }

        // Move result to its destination register
        mov(GetDst<RA_64>(Node), x0);

        ldr(lr,       MemOperand(sp, 7 * 8 + RA64.size() * 8 + 0 * 8));

        add(sp, sp, SPOffset);
        break;
      }
      case IR::OP_CPUID: {
        auto Op = IROp->C<IR::IROp_CPUID>();

        uint64_t SPOffset = AlignUp((RA64.size() + 2 + 2) * 8 + sizeof(FEXCore::CPUIDEmu::FunctionResults), 16);
        sub(sp, sp, SPOffset);

        int i = 0;
        for (auto RA : RA64) {
          str(RA, MemOperand(sp, 0 + i * 8));
          i++;
        }

        str(lr,       MemOperand(sp, RA64.size() * 8 + 0 * 8));

        // x0 = CPUID Handler
        // x1 = CPUID Function
        // x2 = Result location
        LoadConstant(x0, reinterpret_cast<uint64_t>(&CTX->CPUID));
        mov(x1, GetSrc<RA_64>(Op->Header.Args[0].ID()));
        add(x2, sp, RA64.size() * 8 + 3 * 8);
        CallRuntime(CPUIDThunk);

        i = 0;
        for (auto RA : RA64) {
          ldr(RA, MemOperand(sp, 0 + i * 8));
          i++;
        }

        // Results are in x0, x1
        // Results want to be in a i32v4 vector
        auto Dst = GetDst(Node);
        ldr(Dst, MemOperand(sp, RA64.size() * 8 + 3 * 8));

        ldr(lr,       MemOperand(sp, RA64.size() * 8 + 0 * 8));

        add(sp, sp, SPOffset);

        break;
      }
      case IR::OP_EXTRACTELEMENT: {
        auto Op = IROp->C<IR::IROp_ExtractElement>();

        uint32_t PhysReg = RAPass->GetNodeRegister(Op->Header.Args[0].ID());
        if (PhysReg >= FPRBase) {
          switch (OpSize) {
          case 4:
            umov(GetDst<RA_32>(Node), GetSrc(Op->Header.Args[0].ID()).V4S(), Op->Idx);
          break;
          case 8:
            umov(GetDst<RA_64>(Node), GetSrc(Op->Header.Args[0].ID()).V2D(), Op->Idx);
          break;
          default:  LogMan::Msg::A("Unhandled ExtractElementSize: %d", OpSize);
          }
        }
        else {
          LogMan::Msg::A("Can't handle extract from GPR yet");
        }
        break;
      }
      case IR::OP_JUMP: {
        auto Op = IROp->C<IR::IROp_Jump>();

        Label *TargetLabel;
        auto IsTarget = JumpTargets.find(Op->Header.Args[0].ID());
        if (IsTarget == JumpTargets.end()) {
          TargetLabel = &JumpTargets.try_emplace(Op->Header.Args[0].ID()).first->second;
        }
        else {
          TargetLabel = &IsTarget->second;
        }

        b(TargetLabel);
        break;
      }
      case IR::OP_CONDJUMP: {
        auto Op = IROp->C<IR::IROp_CondJump>();

        Label *TrueTargetLabel;
        Label *FalseTargetLabel;

        auto TrueIter = JumpTargets.find(Op->Header.Args[1].ID());
        auto FalseIter = JumpTargets.find(Op->Header.Args[2].ID());

        if (TrueIter == JumpTargets.end()) {
          TrueTargetLabel = &JumpTargets.try_emplace(Op->Header.Args[1].ID()).first->second;
        }
        else {
          TrueTargetLabel = &TrueIter->second;
        }

        if (FalseIter == JumpTargets.end()) {
          FalseTargetLabel = &JumpTargets.try_emplace(Op->Header.Args[2].ID()).first->second;
        }
        else {
          FalseTargetLabel = &FalseIter->second;
        }

        cbnz(GetSrc<RA_64>(Op->Header.Args[0].ID()), TrueTargetLabel);
        b(FalseTargetLabel);
        break;
      }
      case IR::OP_LOADCONTEXT: {
        auto Op = IROp->C<IR::IROp_LoadContext>();
        switch (Op->Size) {
        case 1:
          ldrb(GetDst<RA_32>(Node), MemOperand(STATE, Op->Offset));
        break;
        case 2:
          ldrh(GetDst<RA_32>(Node), MemOperand(STATE, Op->Offset));
        break;
        case 4:
          ldr(GetDst<RA_32>(Node), MemOperand(STATE, Op->Offset));
        break;
        case 8:
          ldr(GetDst<RA_64>(Node), MemOperand(STATE, Op->Offset));
        break;
        case 16:
          ldr(GetDst(Node), MemOperand(STATE, Op->Offset));
        break;
        default:  LogMan::Msg::A("Unhandled LoadContext size: %d", Op->Size);
        }
        break;
      }
      case IR::OP_STORECONTEXT: {
        auto Op = IROp->C<IR::IROp_StoreContext>();
        switch (Op->Size) {
        case 1:
          strb(GetSrc<RA_32>(Op->Header.Args[0].ID()), MemOperand(STATE, Op->Offset));
        break;
        case 2:
          strh(GetSrc<RA_32>(Op->Header.Args[0].ID()), MemOperand(STATE, Op->Offset));
        break;
        case 4:
          str(GetSrc<RA_32>(Op->Header.Args[0].ID()), MemOperand(STATE, Op->Offset));
        break;
        case 8:
          str(GetSrc<RA_64>(Op->Header.Args[0].ID()), MemOperand(STATE, Op->Offset));
        break;
        case 16:
          str(GetSrc(Op->Header.Args[0].ID()), MemOperand(STATE, Op->Offset));
        break;
        default:  LogMan::Msg::A("Unhandled LoadContext size: %d", Op->Size);
        }
        break;
      }
      case IR::OP_STOREFLAG: {
        auto Op = IROp->C<IR::IROp_StoreFlag>();
        and_(TMP1, GetSrc<RA_64>(Op->Header.Args[0].ID()), 1);
        strb(TMP1, MemOperand(STATE, offsetof(FEXCore::Core::CPUState, flags[0]) + Op->Flag));
        break;
      }
      case IR::OP_LOADFLAG: {
        auto Op = IROp->C<IR::IROp_LoadFlag>();
        auto Dst = GetDst<RA_64>(Node);
        ldrb(Dst, MemOperand(STATE, offsetof(FEXCore::Core::CPUState, flags[0]) + Op->Flag));
        and_(Dst, Dst, 1);
        break;
      }
      case IR::OP_FILLREGISTER: {
        auto Op = IROp->C<IR::IROp_FillRegister>();
        uint32_t SlotOffset = Op->Slot * 16;
        switch (OpSize) {
        case 1: {
          ldrb(GetDst<RA_64>(Node), MemOperand(sp, SlotOffset));
          break;
        }
        case 2: {
          ldrh(GetDst<RA_64>(Node), MemOperand(sp, SlotOffset));
          break;
        }
        case 4: {
          ldr(GetDst<RA_32>(Node), MemOperand(sp, SlotOffset));
          break;
        }
        case 8: {
          ldr(GetDst<RA_64>(Node), MemOperand(sp, SlotOffset));
          break;
        }
        case 16: {
          ldr(GetDst(Node), MemOperand(sp, SlotOffset));
          break;
        }
        default:  LogMan::Msg::A("Unhandled SpillRegister size: %d", OpSize);
        }
        break;
      }

      case IR::OP_SPILLREGISTER: {
        auto Op = IROp->C<IR::IROp_SpillRegister>();
        uint32_t SlotOffset = Op->Slot * 16;
        switch (OpSize) {
        case 1: {
          strb(GetSrc<RA_64>(Op->Header.Args[0].ID()), MemOperand(sp, SlotOffset));
          break;
        }
        case 2: {
          strh(GetSrc<RA_64>(Op->Header.Args[0].ID()), MemOperand(sp, SlotOffset));
          break;
        }
        case 4: {
          str(GetSrc<RA_32>(Op->Header.Args[0].ID()), MemOperand(sp, SlotOffset));
          break;
        }
        case 8: {
          str(GetSrc<RA_64>(Op->Header.Args[0].ID()), MemOperand(sp, SlotOffset));
          break;
        }
        case 16: {
          str(GetSrc(Op->Header.Args[0].ID()), MemOperand(sp, SlotOffset));
          break;
        }
        default:  LogMan::Msg::A("Unhandled SpillRegister size: %d", OpSize);
        }
        break;
      }
      case IR::OP_BREAK: {
        auto Op = IROp->C<IR::IROp_Break>();
        switch (Op->Reason) {
          case 0: // Hard fault
          case 5: // Guest ud2
            hlt(4);
            break;
          case 4: // HLT
            LoadConstant(TMP1, 1);
            stlrb(TMP1, MemOperand(STATE, offsetof(FEXCore::Core::ThreadState, RunningEvents.ShouldStop)));

            if (SpillSlots) {
              add(sp, sp, SpillSlots * 16);
            }
            ret();
          break;
          default: LogMan::Msg::A("Unknown Break reason: %d", Op->Reason);
        }
        break;
      }
      case IR::OP_CONSTANT: {
        auto Op = IROp->C<IR::IROp_Constant>();
        auto Dst = GetDst<RA_64>(Node);
        LoadConstant(Dst, Op->Constant);
        break;
      }
      case IR::OP_ADD: {
        auto Op = IROp->C<IR::IROp_Add>();
        add(GetDst<RA_64>(Node), GetSrc<RA_64>(Op->Header.Args[0].ID()), GetSrc<RA_64>(Op->Header.Args[1].ID()));
        break;
      }
      case IR::OP_SUB: {
        auto Op = IROp->C<IR::IROp_Sub>();
        sub(GetDst<RA_64>(Node), GetSrc<RA_64>(Op->Header.Args[0].ID()), GetSrc<RA_64>(Op->Header.Args[1].ID()));
        break;
      }
      case IR::OP_AND: {
        auto Op = IROp->C<IR::IROp_And>();
        and_(GetDst<RA_64>(Node), GetSrc<RA_64>(Op->Header.Args[0].ID()), GetSrc<RA_64>(Op->Header.Args[1].ID()));
        break;
      }
      case IR::OP_XOR: {
        auto Op = IROp->C<IR::IROp_Xor>();
        eor(GetDst<RA_64>(Node), GetSrc<RA_64>(Op->Header.Args[0].ID()), GetSrc<RA_64>(Op->Header.Args[1].ID()));
        break;
      }
      case IR::OP_OR: {
        auto Op = IROp->C<IR::IROp_Or>();
        orr(GetDst<RA_64>(Node), GetSrc<RA_64>(Op->Header.Args[0].ID()), GetSrc<RA_64>(Op->Header.Args[1].ID()));
        break;
      }
      case IR::OP_MOV: {
        auto Op = IROp->C<IR::IROp_Mov>();
        mov(GetDst<RA_64>(Node), GetSrc<RA_64>(Op->Header.Args[0].ID()));
        break;
      }
      case IR::OP_LSHR: {
        auto Op = IROp->C<IR::IROp_Lshr>();
        if (OpSize == 8)
          lsrv(GetDst<RA_64>(Node), GetSrc<RA_64>(Op->Header.Args[0].ID()), GetSrc<RA_64>(Op->Header.Args[1].ID()));
        else
          lsrv(GetDst<RA_32>(Node), GetSrc<RA_32>(Op->Header.Args[0].ID()), GetSrc<RA_32>(Op->Header.Args[1].ID()));
        break;
      }
      case IR::OP_ASHR: {
        auto Op = IROp->C<IR::IROp_Ashr>();
        if (OpSize == 8)
          asrv(GetDst<RA_64>(Node), GetSrc<RA_64>(Op->Header.Args[0].ID()), GetSrc<RA_64>(Op->Header.Args[1].ID()));
        else
          asrv(GetDst<RA_32>(Node), GetSrc<RA_32>(Op->Header.Args[0].ID()), GetSrc<RA_32>(Op->Header.Args[1].ID()));
        break;
      }
      case IR::OP_LSHL: {
        auto Op = IROp->C<IR::IROp_Lshl>();
        if (OpSize == 8)
          lslv(GetDst<RA_64>(Node), GetSrc<RA_64>(Op->Header.Args[0].ID()), GetSrc<RA_64>(Op->Header.Args[1].ID()));
        else
          lslv(GetDst<RA_32>(Node), GetSrc<RA_32>(Op->Header.Args[0].ID()), GetSrc<RA_32>(Op->Header.Args[1].ID()));
        break;
      }
      case IR::OP_ROR: {
        auto Op = IROp->C<IR::IROp_Ror>();
        uint8_t Mask = OpSize * 8 - 1;

        switch (OpSize) {
        case 4: {
          rorv(GetDst<RA_32>(Node), GetSrc<RA_32>(Op->Header.Args[0].ID()), GetSrc<RA_32>(Op->Header.Args[1].ID()));
        break;
        }
        case 8: {
          rorv(GetDst<RA_64>(Node), GetSrc<RA_64>(Op->Header.Args[0].ID()), GetSrc<RA_64>(Op->Header.Args[1].ID()));
        break;
        }

        default: LogMan::Msg::A("Unhandled ROR size: %d", OpSize);
        }
        break;
      }
      case IR::OP_ROL: {
        auto Op = IROp->C<IR::IROp_Rol>();
        uint8_t Mask = OpSize * 8 - 1;

        switch (OpSize) {
        case 4: {
          movz(TMP1, 32);
          sub(TMP1.W(), TMP1.W(), GetSrc<RA_32>(Op->Header.Args[1].ID()));
          rorv(GetDst<RA_32>(Node), GetSrc<RA_32>(Op->Header.Args[0].ID()), TMP1.W());
        break;
        }
        case 8: {
          movz(TMP1, 64);
          sub(TMP1, TMP1, GetSrc<RA_64>(Op->Header.Args[1].ID()));
          rorv(GetDst<RA_64>(Node), GetSrc<RA_64>(Op->Header.Args[0].ID()), TMP1);
        break;
        }

        default: LogMan::Msg::A("Unhandled ROL size: %d", OpSize);
        }
        break;
      }
      case IR::OP_SEXT: {
        auto Op = IROp->C<IR::IROp_Sext>();
        LogMan::Throw::A(Op->SrcSize <= 64, "Can't support Zext of size: %ld", Op->SrcSize);
        auto Dst = GetDst<RA_64>(Node);

        switch (Op->SrcSize / 8) {
        case 1:
          sxtb(Dst, GetSrc<RA_64>(Op->Header.Args[0].ID()));
        break;
        case 2:
          sxth(Dst, GetSrc<RA_64>(Op->Header.Args[0].ID()));
        break;
        case 4:
          sxtw(Dst, GetSrc<RA_64>(Op->Header.Args[0].ID()));
        break;
        case 8:
          mov(Dst, GetSrc<RA_64>(Op->Header.Args[0].ID()));
        break;
        default: LogMan::Msg::A("Unknown Sext size: %d", Op->SrcSize / 8);
        }
        break;
      }
      case IR::OP_ZEXT: {
        auto Op = IROp->C<IR::IROp_Zext>();
        LogMan::Throw::A(Op->SrcSize <= 64, "Can't support Zext of size: %ld", Op->SrcSize);
        uint32_t PhysReg = RAPass->GetNodeRegister(Op->Header.Args[0].ID());
        if (PhysReg >= FPRBase) {
          // FPR -> GPR transfer with free truncation
          switch (Op->SrcSize) {
          case 8:
            mov(GetDst<RA_64>(Node), GetSrc(Op->Header.Args[0].ID()).V16B(), 0);
          break;
          case 16:
            mov(GetDst<RA_64>(Node), GetSrc(Op->Header.Args[0].ID()).V8H(), 0);
          break;
          case 32:
            mov(GetDst<RA_64>(Node), GetSrc(Op->Header.Args[0].ID()).V4S(), 0);
          break;
          case 64:
            mov(GetDst<RA_64>(Node), GetSrc(Op->Header.Args[0].ID()).V2D(), 0);
          break;
          default: LogMan::Msg::A("Unhandled Zext size: %d", Op->SrcSize); break;
          }
        }
        else {
          if (Op->SrcSize == 64) {
            // GPR->FPR transfer
            auto Dst = GetDst(Node);
            eor(Dst.V16B(), Dst.V16B(), Dst.V16B());
            ins(Dst.V2D(), 0, GetSrc<RA_64>(Op->Header.Args[0].ID()));
          }
          else {
            and_(GetDst<RA_64>(Node), GetSrc<RA_64>(Op->Header.Args[0].ID()), ((1ULL << Op->SrcSize) - 1));
          }
        }
        break;
      }
      case IR::OP_MUL: {
        auto Op = IROp->C<IR::IROp_Mul>();
        auto Dst = GetDst<RA_64>(Node);

        switch (OpSize) {
        case 1:
          sxtb(TMP1, GetSrc<RA_64>(Op->Header.Args[0].ID()));
          sxtb(TMP2, GetSrc<RA_64>(Op->Header.Args[1].ID()));
          mul(Dst, TMP1, TMP2);
          sxtb(Dst, Dst);
        break;
        case 2:
          sxth(TMP1, GetSrc<RA_64>(Op->Header.Args[0].ID()));
          sxth(TMP2, GetSrc<RA_64>(Op->Header.Args[1].ID()));
          mul(Dst, TMP1, TMP2);
          sxth(Dst, Dst);
        break;
        case 4:
          sxtw(TMP1, GetSrc<RA_64>(Op->Header.Args[0].ID()));
          sxtw(TMP2, GetSrc<RA_64>(Op->Header.Args[1].ID()));
          mul(Dst.W(), TMP1.W(), TMP2.W());
          sxtw(Dst, Dst);
        break;
        case 8:
          mul(Dst, GetSrc<RA_64>(Op->Header.Args[0].ID()), GetSrc<RA_64>(Op->Header.Args[1].ID()));
        break;
        default: LogMan::Msg::A("Unknown Sext size: %d", OpSize);
        }
        break;
      }
      case IR::OP_UMUL: {
        auto Op = IROp->C<IR::IROp_UMul>();
        auto Dst = GetDst<RA_64>(Node);

        switch (OpSize) {
        case 1:
          uxtb(TMP1, GetSrc<RA_64>(Op->Header.Args[0].ID()));
          uxtb(TMP2, GetSrc<RA_64>(Op->Header.Args[1].ID()));
          mul(Dst, TMP1, TMP2);
          uxtb(Dst, Dst);
        break;
        case 2:
          uxth(TMP1, GetSrc<RA_64>(Op->Header.Args[0].ID()));
          uxth(TMP2, GetSrc<RA_64>(Op->Header.Args[1].ID()));
          mul(Dst, TMP1, TMP2);
          uxth(Dst, Dst);
        break;
        case 4:
          uxtw(TMP1, GetSrc<RA_64>(Op->Header.Args[0].ID()));
          uxtw(TMP2, GetSrc<RA_64>(Op->Header.Args[1].ID()));
          mul(Dst.W(), TMP1.W(), TMP2.W());
          uxtw(Dst, Dst);
        break;
        case 8:
          mul(Dst, GetSrc<RA_64>(Op->Header.Args[0].ID()), GetSrc<RA_64>(Op->Header.Args[1].ID()));
        break;
        default: LogMan::Msg::A("Unknown Sext size: %d", OpSize);
        }
        break;
      }
      case IR::OP_BFE: {
        auto Op = IROp->C<IR::IROp_Bfe>();
        LogMan::Throw::A(OpSize <= 16, "OpSize is too large for BFE: %d", OpSize);
        LogMan::Throw::A(Op->Width != 0, "Invalid BFE width of 0");

        auto Dst = GetDst<RA_64>(Node);
        if (OpSize == 16) {
          LogMan::Throw::A(!(Op->lsb < 64 && (Op->lsb + Op->Width > 64)), "Trying to BFE an XMM across the 64bit split: Beginning at %d, ending at %d", Op->lsb, Op->lsb + Op->Width);
          uint8_t Offset = Op->lsb;
          if (Offset < 64) {
            mov(Dst, GetSrc(Op->Header.Args[0].ID()).D(), 0);
          }
          else {
            mov(Dst, GetSrc(Op->Header.Args[0].ID()).D(), 1);
            Offset -= 64;
          }

          if (Offset) {
            lsr(Dst, Dst, Offset);
          }

          if (Op->Width != 64) {
            ubfx(Dst, Dst, 0, Op->Width);
          }
        }
        else {
          lsr(Dst, GetSrc<RA_64>(Op->Header.Args[0].ID()), Op->lsb);
          if (Op->Width != 64) {
            and_(Dst, Dst, ((1ULL << Op->Width) - 1));
          }
        }
        break;
      }
      case IR::OP_POPCOUNT: {
        auto Op = IROp->C<IR::IROp_Popcount>();
        auto Dst = GetDst<RA_64>(Node);
        fmov(VTMP1.D(), GetSrc<RA_64>(Op->Header.Args[0].ID()));
        cnt(VTMP1.V8B(), VTMP1.V8B());
        addv(VTMP1.B(), VTMP1.V8B());
        umov(Dst.W(), VTMP1.B(), 0);
        break;
      }
      case IR::OP_FINDLSB: {
        auto Op = IROp->C<IR::IROp_FindLSB>();
        auto Dst = GetDst<RA_64>(Node);
        auto Src = GetSrc<RA_64>(Op->Header.Args[0].ID());
        if (OpSize != 8) {
          ubfx(TMP1, Src, 0, OpSize * 8);
          cmp(TMP1, 0);
          rbit(TMP1, TMP1);
          clz(Dst, TMP1);
          csinv(Dst, Dst, xzr, ne);
        }
        else {
          rbit(TMP1, Src);
          cmp(Src, 0);
          clz(Dst, TMP1);
          csinv(Dst, Dst, xzr, ne);
        }

        break;
      }
      case IR::OP_FINDMSB: {
        auto Op = IROp->C<IR::IROp_FindMSB>();
        auto Dst = GetDst<RA_64>(Node);
        movz(TMP1, OpSize * 8);
        clz(Dst, GetSrc<RA_64>(Op->Header.Args[0].ID()));
        sub(Dst, TMP1, Dst);
        break;
      }
      case IR::OP_REV: {
        auto Op = IROp->C<IR::IROp_Rev>();
        switch (OpSize) {
          case 2:
            rev16(GetDst<RA_32>(Node), GetSrc<RA_32>(Op->Header.Args[0].ID()));
          break;
          case 4:
            rev(GetDst<RA_32>(Node), GetSrc<RA_32>(Op->Header.Args[0].ID()));
            break;
          case 8:
            rev(GetDst<RA_64>(Node), GetSrc<RA_64>(Op->Header.Args[0].ID()));
            break;
          default: LogMan::Msg::A("Unknown REV size: %d", OpSize); break;
        }
        break;
      }
      case IR::OP_CAS: {
        auto Op = IROp->C<IR::IROp_CAS>();
        // Args[0]: Expected
        // Args[1]: Desired
        // Args[2]: Pointer
        // DataSrc = *Src1
        // if (DataSrc == Src3) { *Src1 == Src2; } Src2 = DataSrc
        // This will write to memory! Careful!

        auto Expected = GetSrc<RA_64>(Op->Header.Args[0].ID());
        auto Desired = GetSrc<RA_64>(Op->Header.Args[1].ID());
        auto MemSrc = GetSrc<RA_64>(Op->Header.Args[2].ID());

        add(TMP1, MEM_BASE, MemSrc);
        mov(TMP2, Expected);

        switch (OpSize) {
        case 1: casalb(TMP2.W(), Desired.W(), MemOperand(TMP1)); break;
        case 2: casalh(TMP2.W(), Desired.W(), MemOperand(TMP1)); break;
        case 4: casal(TMP2.W(), Desired.W(), MemOperand(TMP1)); break;
        case 8: casal(TMP2.X(), Desired.X(), MemOperand(TMP1)); break;
        default: LogMan::Msg::A("Unsupported: %d", OpSize);
        }

        mov(GetDst<RA_64>(Node), TMP2);
        break;
      }
      case IR::OP_SELECT: {
        auto Op = IROp->C<IR::IROp_Select>();

        cmp(GetSrc<RA_64>(Op->Header.Args[0].ID()), GetSrc<RA_64>(Op->Header.Args[1].ID()));

        switch (Op->Cond.Val) {
        case FEXCore::IR::COND_EQ:
          csel(GetDst<RA_64>(Node), GetSrc<RA_64>(Op->Header.Args[2].ID()), GetSrc<RA_64>(Op->Header.Args[3].ID()), Condition::eq);
        break;
        case FEXCore::IR::COND_NEQ:
          csel(GetDst<RA_64>(Node), GetSrc<RA_64>(Op->Header.Args[2].ID()), GetSrc<RA_64>(Op->Header.Args[3].ID()), Condition::ne);
        break;
        case FEXCore::IR::COND_GE:
          csel(GetDst<RA_64>(Node), GetSrc<RA_64>(Op->Header.Args[2].ID()), GetSrc<RA_64>(Op->Header.Args[3].ID()), Condition::ge);
        break;
        case FEXCore::IR::COND_LT:
          csel(GetDst<RA_64>(Node), GetSrc<RA_64>(Op->Header.Args[2].ID()), GetSrc<RA_64>(Op->Header.Args[3].ID()), Condition::lo);
        break;
        case FEXCore::IR::COND_GT:
          csel(GetDst<RA_64>(Node), GetSrc<RA_64>(Op->Header.Args[2].ID()), GetSrc<RA_64>(Op->Header.Args[3].ID()), Condition::gt);
        break;
        case FEXCore::IR::COND_LE:
          csel(GetDst<RA_64>(Node), GetSrc<RA_64>(Op->Header.Args[2].ID()), GetSrc<RA_64>(Op->Header.Args[3].ID()), Condition::le);
        break;
        case FEXCore::IR::COND_CS:
        case FEXCore::IR::COND_CC:
        case FEXCore::IR::COND_MI:
        case FEXCore::IR::COND_PL:
        case FEXCore::IR::COND_VS:
        case FEXCore::IR::COND_VC:
        case FEXCore::IR::COND_HI:
        case FEXCore::IR::COND_LS:
        default:
        LogMan::Msg::A("Unsupported compare type");
        break;
        }

        break;
      }
      case IR::OP_LOADMEM: {
        auto Op = IROp->C<IR::IROp_LoadMem>();

        auto Dst = GetDst<RA_64>(Node);
        switch (Op->Size) {
        case 1:
          ldrb(Dst, MemOperand(MEM_BASE, GetSrc<RA_64>(Op->Header.Args[0].ID())));
        break;
        case 2:
          ldrh(Dst, MemOperand(MEM_BASE, GetSrc<RA_64>(Op->Header.Args[0].ID())));
        break;
        case 4:
          ldr(Dst.W(), MemOperand(MEM_BASE, GetSrc<RA_64>(Op->Header.Args[0].ID())));
        break;
        case 8:
          ldr(Dst, MemOperand(MEM_BASE, GetSrc<RA_64>(Op->Header.Args[0].ID())));
        break;
        case 16:
          ldr(GetDst(Node), MemOperand(MEM_BASE, GetSrc<RA_64>(Op->Header.Args[0].ID())));
        break;
        default:  LogMan::Msg::A("Unhandled LoadMem size: %d", Op->Size);
        }
        break;
      }
      case IR::OP_STOREMEM: {
        auto Op = IROp->C<IR::IROp_StoreMem>();
        switch (Op->Size) {
        case 1:
          strb(GetSrc<RA_64>(Op->Header.Args[1].ID()), MemOperand(MEM_BASE, GetSrc<RA_64>(Op->Header.Args[0].ID())));
        break;
        case 2:
          strh(GetSrc<RA_64>(Op->Header.Args[1].ID()), MemOperand(MEM_BASE, GetSrc<RA_64>(Op->Header.Args[0].ID())));
        break;
        case 4:
          str(GetSrc<RA_32>(Op->Header.Args[1].ID()), MemOperand(MEM_BASE, GetSrc<RA_64>(Op->Header.Args[0].ID())));
        break;
        case 8:
          str(GetSrc<RA_64>(Op->Header.Args[1].ID()), MemOperand(MEM_BASE, GetSrc<RA_64>(Op->Header.Args[0].ID())));
        break;
        case 16:
          str(GetSrc(Op->Header.Args[1].ID()), MemOperand(MEM_BASE, GetSrc<RA_64>(Op->Header.Args[0].ID())));
        break;
        default:  LogMan::Msg::A("Unhandled StoreMem size: %d", Op->Size);
        }
        break;
      }
      case IR::OP_MULH: {
        auto Op = IROp->C<IR::IROp_MulH>();
        switch (OpSize) {
        case 1:
          sxtb(TMP1, GetSrc<RA_64>(Op->Header.Args[0].ID()));
          sxtb(TMP2, GetSrc<RA_64>(Op->Header.Args[1].ID()));
          mul(TMP1, TMP1, TMP2);
          sbfx(GetDst<RA_64>(Node), TMP1, 8, 8);
        break;
        case 2:
          sxth(TMP1, GetSrc<RA_64>(Op->Header.Args[0].ID()));
          sxth(TMP2, GetSrc<RA_64>(Op->Header.Args[1].ID()));
          mul(TMP1, TMP1, TMP2);
          sbfx(GetDst<RA_64>(Node), TMP1, 16, 16);
        break;
        case 4:
          sxtw(TMP1, GetSrc<RA_64>(Op->Header.Args[0].ID()));
          sxtw(TMP2, GetSrc<RA_64>(Op->Header.Args[1].ID()));
          mul(TMP1, TMP1, TMP2);
          sbfx(GetDst<RA_64>(Node), TMP1, 32, 32);
        break;
        case 8:
          smulh(GetDst<RA_64>(Node), GetSrc<RA_64>(Op->Header.Args[0].ID()), GetSrc<RA_64>(Op->Header.Args[1].ID()));
        break;
        default: LogMan::Msg::A("Unknown Sext size: %d", OpSize);
        }
        break;
      }
      case IR::OP_UMULH: {
        auto Op = IROp->C<IR::IROp_UMulH>();
        switch (OpSize) {
        case 1:
          uxtb(TMP1, GetSrc<RA_64>(Op->Header.Args[0].ID()));
          uxtb(TMP2, GetSrc<RA_64>(Op->Header.Args[1].ID()));
          mul(TMP1, TMP1, TMP2);
          ubfx(GetDst<RA_64>(Node), TMP1, 8, 8);
        break;
        case 2:
          uxth(TMP1, GetSrc<RA_64>(Op->Header.Args[0].ID()));
          uxth(TMP2, GetSrc<RA_64>(Op->Header.Args[1].ID()));
          mul(TMP1, TMP1, TMP2);
          ubfx(GetDst<RA_64>(Node), TMP1, 16, 16);
        break;
        case 4:
          uxtw(TMP1, GetSrc<RA_64>(Op->Header.Args[0].ID()));
          uxtw(TMP2, GetSrc<RA_64>(Op->Header.Args[1].ID()));
          mul(TMP1, TMP1, TMP2);
          ubfx(GetDst<RA_64>(Node), TMP1, 32, 32);
        break;
        case 8:
          umulh(GetDst<RA_64>(Node), GetSrc<RA_64>(Op->Header.Args[0].ID()), GetSrc<RA_64>(Op->Header.Args[1].ID()));
        break;
        default: LogMan::Msg::A("Unknown Sext size: %d", OpSize);
        }
        break;
      }
      case IR::OP_LUDIV: {
        auto Op = IROp->C<IR::IROp_LUDiv>();
        // Each source is OpSize in size
        // So you can have up to a 128bit divide from x86-64
        auto Size = OpSize;
        switch (Size) {
        case 4: {
          mov(TMP1, GetSrc<RA_64>(Op->Header.Args[0].ID()));
          bfi(TMP1, GetSrc<RA_64>(Op->Header.Args[1].ID()), 32, 32);
          udiv(GetDst<RA_64>(Node), TMP1, GetSrc<RA_64>(Op->Header.Args[2].ID()));
        break;
        }
        case 8: {
          udiv(GetDst<RA_64>(Node), GetSrc<RA_64>(Op->Header.Args[0].ID()), GetSrc<RA_64>(Op->Header.Args[2].ID()));
        break;
        }
        default: LogMan::Msg::A("Unknown LUDIV Size: %d", Size); break;
        }
        break;
      }
      case IR::OP_LDIV: {
        auto Op = IROp->C<IR::IROp_LDiv>();
        // Each source is OpSize in size
        // So you can have up to a 128bit divide from x86-64
        auto Size = OpSize;
        switch (Size) {
        case 4: {
          mov(TMP1, GetSrc<RA_64>(Op->Header.Args[0].ID()));
          bfi(TMP1, GetSrc<RA_64>(Op->Header.Args[1].ID()), 32, 32);
          sdiv(GetDst<RA_64>(Node), TMP1, GetSrc<RA_64>(Op->Header.Args[2].ID()));
        break;
        }
        case 8: {
          sdiv(GetDst<RA_64>(Node), GetSrc<RA_64>(Op->Header.Args[0].ID()), GetSrc<RA_64>(Op->Header.Args[2].ID()));
        break;
        }
        default: LogMan::Msg::A("Unknown LUDIV Size: %d", Size); break;
        }
        break;
      }
      case IR::OP_LUREM: {
        auto Op = IROp->C<IR::IROp_LURem>();
        // Each source is OpSize in size
        // So you can have up to a 128bit divide from x86-64
        auto Size = OpSize;
        switch (Size) {
        case 4: {
          auto Divisor = GetSrc<RA_64>(Op->Header.Args[2].ID());

          mov(TMP1, GetSrc<RA_64>(Op->Header.Args[0].ID()));
          bfi(TMP1, GetSrc<RA_64>(Op->Header.Args[1].ID()), 32, 32);
          udiv(TMP2, TMP1, Divisor);

          msub(GetDst<RA_64>(Node), TMP2, Divisor, TMP1);
        break;
        }
        case 8: {
          auto Dividend = GetSrc<RA_64>(Op->Header.Args[0].ID());
          auto Divisor = GetSrc<RA_64>(Op->Header.Args[2].ID());

          udiv(TMP1, Dividend, Divisor);
          msub(GetDst<RA_64>(Node), TMP1, Divisor, Dividend);
        break;
        }
        default: LogMan::Msg::A("Unknown LUDIV Size: %d", Size); break;
        }
        break;
      }
      case IR::OP_LREM: {
        auto Op = IROp->C<IR::IROp_LRem>();
        // Each source is OpSize in size
        // So you can have up to a 128bit divide from x86-64
        auto Size = OpSize;
        switch (Size) {
        case 4: {
          auto Divisor = GetSrc<RA_64>(Op->Header.Args[2].ID());

          mov(TMP1, GetSrc<RA_64>(Op->Header.Args[0].ID()));
          bfi(TMP1, GetSrc<RA_64>(Op->Header.Args[1].ID()), 32, 32);
          sdiv(TMP2, TMP1, Divisor);

          msub(GetDst<RA_64>(Node), TMP2, Divisor, TMP1);
        break;
        }
        case 8: {
          auto Dividend = GetSrc<RA_64>(Op->Header.Args[0].ID());
          auto Divisor = GetSrc<RA_64>(Op->Header.Args[2].ID());

          sdiv(TMP1, Dividend, Divisor);
          msub(GetDst<RA_64>(Node), TMP1, Divisor, Dividend);
        break;
        }
        default: LogMan::Msg::A("Unknown LUDIV Size: %d", Size); break;
        }
        break;
      }
      case IR::OP_VINSELEMENT: {
        auto Op = IROp->C<IR::IROp_VInsElement>();
        mov(VTMP1, GetSrc(Op->Header.Args[0].ID()));
        switch (Op->ElementSize) {
        case 1: {
          mov(VTMP1.V16B(), Op->DestIdx, GetSrc(Op->Header.Args[1].ID()).V16B(), Op->SrcIdx);
        break;
        }
        case 2: {
          mov(VTMP1.V8H(), Op->DestIdx, GetSrc(Op->Header.Args[1].ID()).V8H(), Op->SrcIdx);
        break;
        }
        case 4: {
          mov(VTMP1.V4S(), Op->DestIdx, GetSrc(Op->Header.Args[1].ID()).V4S(), Op->SrcIdx);
        break;
        }
        case 8: {
          mov(VTMP1.V2D(), Op->DestIdx, GetSrc(Op->Header.Args[1].ID()).V2D(), Op->SrcIdx);
        break;
        }
        default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
        }
        mov(GetDst(Node), VTMP1);
        break;
      }
      case IR::OP_VXOR: {
        auto Op = IROp->C<IR::IROp_VXor>();
        eor(GetDst(Node).V16B(), GetSrc(Op->Header.Args[0].ID()).V16B(), GetSrc(Op->Header.Args[1].ID()).V16B());
        break;
      }
      case IR::OP_VOR: {
        auto Op = IROp->C<IR::IROp_VOr>();
        orr(GetDst(Node).V16B(), GetSrc(Op->Header.Args[0].ID()).V16B(), GetSrc(Op->Header.Args[1].ID()).V16B());
        break;
      }
      case IR::OP_VAND: {
        auto Op = IROp->C<IR::IROp_VAnd>();
        and_(GetDst(Node).V16B(), GetSrc(Op->Header.Args[0].ID()).V16B(), GetSrc(Op->Header.Args[1].ID()).V16B());
        break;
      }
      case IR::OP_VADD: {
        auto Op = IROp->C<IR::IROp_VAdd>();
        switch (Op->ElementSize) {
        case 1: {
          add(GetDst(Node).V16B(), GetSrc(Op->Header.Args[0].ID()).V16B(), GetSrc(Op->Header.Args[1].ID()).V16B());
        break;
        }
        case 2: {
          add(GetDst(Node).V8H(), GetSrc(Op->Header.Args[0].ID()).V8H(), GetSrc(Op->Header.Args[1].ID()).V8H());
        break;
        }
        case 4: {
          add(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S(), GetSrc(Op->Header.Args[1].ID()).V4S());
        break;
        }
        case 8: {
          add(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D(), GetSrc(Op->Header.Args[1].ID()).V2D());
        break;
        }
        default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
        }
        break;
      }
      case IR::OP_VSUB: {
        auto Op = IROp->C<IR::IROp_VSub>();
        switch (Op->ElementSize) {
        case 1: {
          sub(GetDst(Node).V16B(), GetSrc(Op->Header.Args[0].ID()).V16B(), GetSrc(Op->Header.Args[1].ID()).V16B());
        break;
        }
        case 2: {
          sub(GetDst(Node).V8H(), GetSrc(Op->Header.Args[0].ID()).V8H(), GetSrc(Op->Header.Args[1].ID()).V8H());
        break;
        }
        case 4: {
          sub(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S(), GetSrc(Op->Header.Args[1].ID()).V4S());
        break;
        }
        case 8: {
          sub(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D(), GetSrc(Op->Header.Args[1].ID()).V2D());
        break;
        }
        default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
        }
        break;
      }
      case IR::OP_VFADD: {
        auto Op = IROp->C<IR::IROp_VFAdd>();
        if (Op->ElementSize == Op->RegisterSize) {
          // Scalar
          switch (Op->ElementSize) {
            case 4: {
              fadd(GetDst(Node).S(), GetSrc(Op->Header.Args[0].ID()).S(), GetSrc(Op->Header.Args[1].ID()).S());
            break;
            }
            case 8: {
              fadd(GetDst(Node).D(), GetSrc(Op->Header.Args[0].ID()).D(), GetSrc(Op->Header.Args[1].ID()).D());
            break;
            }
            default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
          }
        }
        else {
          // Vector
          switch (Op->ElementSize) {
            case 4: {
              fadd(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S(), GetSrc(Op->Header.Args[1].ID()).V4S());
            break;
            }
            case 8: {
              fadd(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D(), GetSrc(Op->Header.Args[1].ID()).V2D());
            break;
            }
            default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
          }
        }
        break;
      }
      case IR::OP_VFSUB: {
        auto Op = IROp->C<IR::IROp_VFSub>();
        if (Op->ElementSize == Op->RegisterSize) {
          // Scalar
          switch (Op->ElementSize) {
            case 4: {
              fsub(GetDst(Node).S(), GetSrc(Op->Header.Args[0].ID()).S(), GetSrc(Op->Header.Args[1].ID()).S());
            break;
            }
            case 8: {
              fsub(GetDst(Node).D(), GetSrc(Op->Header.Args[0].ID()).D(), GetSrc(Op->Header.Args[1].ID()).D());
            break;
            }
            default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
          }
        }
        else {
          // Vector
          switch (Op->ElementSize) {
            case 4: {
              fsub(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S(), GetSrc(Op->Header.Args[1].ID()).V4S());
            break;
            }
            case 8: {
              fsub(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D(), GetSrc(Op->Header.Args[1].ID()).V2D());
            break;
            }
            default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
          }
        }
        break;
      }
      case IR::OP_VFMUL: {
        auto Op = IROp->C<IR::IROp_VFMul>();
        if (Op->ElementSize == Op->RegisterSize) {
          // Scalar
          switch (Op->ElementSize) {
            case 4: {
              fmul(GetDst(Node).S(), GetSrc(Op->Header.Args[0].ID()).S(), GetSrc(Op->Header.Args[1].ID()).S());
            break;
            }
            case 8: {
              fmul(GetDst(Node).D(), GetSrc(Op->Header.Args[0].ID()).D(), GetSrc(Op->Header.Args[1].ID()).D());
            break;
            }
            default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
          }
        }
        else {
          // Vector
          switch (Op->ElementSize) {
            case 4: {
              fmul(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S(), GetSrc(Op->Header.Args[1].ID()).V4S());
            break;
            }
            case 8: {
              fmul(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D(), GetSrc(Op->Header.Args[1].ID()).V2D());
            break;
            }
            default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
          }
        }
        break;
      }
      case IR::OP_VFDIV: {
        auto Op = IROp->C<IR::IROp_VFDiv>();
        if (Op->ElementSize == Op->RegisterSize) {
          // Scalar
          switch (Op->ElementSize) {
            case 4: {
              fdiv(GetDst(Node).S(), GetSrc(Op->Header.Args[0].ID()).S(), GetSrc(Op->Header.Args[1].ID()).S());
            break;
            }
            case 8: {
              fdiv(GetDst(Node).D(), GetSrc(Op->Header.Args[0].ID()).D(), GetSrc(Op->Header.Args[1].ID()).D());
            break;
            }
            default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
          }
        }
        else {
          // Vector
          switch (Op->ElementSize) {
            case 4: {
              fdiv(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S(), GetSrc(Op->Header.Args[1].ID()).V4S());
            break;
            }
            case 8: {
              fdiv(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D(), GetSrc(Op->Header.Args[1].ID()).V2D());
            break;
            }
            default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
          }
        }
        break;
      }
      case IR::OP_VFMAX: {
        auto Op = IROp->C<IR::IROp_VFMax>();
        if (Op->ElementSize == Op->RegisterSize) {
          // Scalar
          switch (Op->ElementSize) {
            case 4: {
              fmax(GetDst(Node).S(), GetSrc(Op->Header.Args[0].ID()).S(), GetSrc(Op->Header.Args[1].ID()).S());
            break;
            }
            case 8: {
              fmax(GetDst(Node).D(), GetSrc(Op->Header.Args[0].ID()).D(), GetSrc(Op->Header.Args[1].ID()).D());
            break;
            }
            default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
          }
        }
        else {
          // Vector
          switch (Op->ElementSize) {
            case 4: {
              fmax(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S(), GetSrc(Op->Header.Args[1].ID()).V4S());
            break;
            }
            case 8: {
              fmax(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D(), GetSrc(Op->Header.Args[1].ID()).V2D());
            break;
            }
            default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
          }
        }
        break;
      }
      case IR::OP_VFMIN: {
        auto Op = IROp->C<IR::IROp_VFMin>();
        if (Op->ElementSize == Op->RegisterSize) {
          // Scalar
          switch (Op->ElementSize) {
            case 4: {
              fmin(GetDst(Node).S(), GetSrc(Op->Header.Args[0].ID()).S(), GetSrc(Op->Header.Args[1].ID()).S());
            break;
            }
            case 8: {
              fmin(GetDst(Node).D(), GetSrc(Op->Header.Args[0].ID()).D(), GetSrc(Op->Header.Args[1].ID()).D());
            break;
            }
            default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
          }
        }
        else {
          // Vector
          switch (Op->ElementSize) {
            case 4: {
              fmin(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S(), GetSrc(Op->Header.Args[1].ID()).V4S());
            break;
            }
            case 8: {
              fmin(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D(), GetSrc(Op->Header.Args[1].ID()).V2D());
            break;
            }
            default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
          }
        }
        break;
      }
      case IR::OP_VFRECP: {
        auto Op = IROp->C<IR::IROp_VFRecp>();
        if (Op->ElementSize == Op->RegisterSize) {
          // Scalar
          switch (Op->ElementSize) {
            case 4: {
              fmov(VTMP1.S(), 1.0);
              fdiv(GetDst(Node).S(), VTMP1.S(), GetSrc(Op->Header.Args[0].ID()).S());
            break;
            }
            case 8: {
              fmov(VTMP1.D(), 1.0);
              fdiv(GetDst(Node).D(), VTMP1.D(), GetSrc(Op->Header.Args[0].ID()).D());
            break;
            }
            default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
          }
        }
        else {
          // Vector
          switch (Op->ElementSize) {
            case 4: {
              fmov(VTMP1.V4S(), 1.0);
              fdiv(GetDst(Node).V4S(), VTMP1.V4S(), GetSrc(Op->Header.Args[0].ID()).V4S());
            break;
            }
            case 8: {
              fmov(VTMP1.V2D(), 1.0);
              fdiv(GetDst(Node).V2D(), VTMP1.V2D(), GetSrc(Op->Header.Args[0].ID()).V2D());
            break;
            }
            default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
          }
        }
        break;
      }
      case IR::OP_VFSQRT: {
        auto Op = IROp->C<IR::IROp_VFRSqrt>();
        if (Op->ElementSize == Op->RegisterSize) {
          // Scalar
          switch (Op->ElementSize) {
            case 4: {
              fsqrt(GetDst(Node).S(), GetSrc(Op->Header.Args[0].ID()).S());
              fdiv(GetDst(Node).S(), VTMP1.S(), VTMP2.S());
            break;
            }
            case 8: {
              fsqrt(GetDst(Node).D(), GetSrc(Op->Header.Args[0].ID()).D());
              fdiv(GetDst(Node).D(), VTMP1.D(), VTMP2.D());
            break;
            }
            default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
          }
        }
        else {
          // Vector
          switch (Op->ElementSize) {
            case 4: {
              fsqrt(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S());
              fdiv(GetDst(Node).V4S(), VTMP1.V4S(), VTMP2.V4S());
            break;
            }
            case 8: {
              fsqrt(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D());
              fdiv(GetDst(Node).V2D(), VTMP1.V2D(), VTMP2.V2D());
            break;
            }
            default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
          }
        }
        break;
      }
      case IR::OP_VFRSQRT: {
        auto Op = IROp->C<IR::IROp_VFRSqrt>();
        if (Op->ElementSize == Op->RegisterSize) {
          // Scalar
          switch (Op->ElementSize) {
            case 4: {
              fmov(VTMP1.S(), 1.0);
              fsqrt(VTMP2.S(), GetSrc(Op->Header.Args[0].ID()).S());
              fdiv(GetDst(Node).S(), VTMP1.S(), VTMP2.S());
            break;
            }
            case 8: {
              fmov(VTMP1.D(), 1.0);
              fsqrt(VTMP2.D(), GetSrc(Op->Header.Args[0].ID()).D());
              fdiv(GetDst(Node).D(), VTMP1.D(), VTMP2.D());
            break;
            }
            default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
          }
        }
        else {
          // Vector
          switch (Op->ElementSize) {
            case 4: {
              fmov(VTMP1.V4S(), 1.0);
              fsqrt(VTMP2.V4S(), GetSrc(Op->Header.Args[0].ID()).V4S());
              fdiv(GetDst(Node).V4S(), VTMP1.V4S(), VTMP2.V4S());
            break;
            }
            case 8: {
              fmov(VTMP1.V2D(), 1.0);
              fsqrt(VTMP2.V2D(), GetSrc(Op->Header.Args[0].ID()).V2D());
              fdiv(GetDst(Node).V2D(), VTMP1.V2D(), VTMP2.V2D());
            break;
            }
            default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
          }
        }
        break;
      }
      case IR::OP_VCMPEQ: {
        auto Op = IROp->C<IR::IROp_VCMPEQ>();
        LogMan::Throw::A(Op->RegisterSize == 16, "Can't handle register size of: %d", Op->RegisterSize);
        switch (Op->ElementSize) {
        case 1: {
          cmeq(GetDst(Node).V16B(), GetSrc(Op->Header.Args[0].ID()).V16B(), GetSrc(Op->Header.Args[1].ID()).V16B());
        break;
        }
        case 2: {
          cmeq(GetDst(Node).V8H(), GetSrc(Op->Header.Args[0].ID()).V8H(), GetSrc(Op->Header.Args[1].ID()).V8H());
        break;
        }
        case 4: {
          cmeq(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S(), GetSrc(Op->Header.Args[1].ID()).V4S());
        break;
        }
        case 8: {
          cmeq(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D(), GetSrc(Op->Header.Args[1].ID()).V2D());
        break;
        }
        default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
        }
        break;
      }
      case IR::OP_VCMPGT: {
        auto Op = IROp->C<IR::IROp_VCMPGT>();
        LogMan::Throw::A(Op->RegisterSize == 16, "Can't handle register size of: %d", Op->RegisterSize);
        switch (Op->ElementSize) {
        case 1: {
          cmgt(GetDst(Node).V16B(), GetSrc(Op->Header.Args[0].ID()).V16B(), GetSrc(Op->Header.Args[1].ID()).V16B());
        break;
        }
        case 2: {
          cmgt(GetDst(Node).V8H(), GetSrc(Op->Header.Args[0].ID()).V8H(), GetSrc(Op->Header.Args[1].ID()).V8H());
        break;
        }
        case 4: {
          cmgt(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S(), GetSrc(Op->Header.Args[1].ID()).V4S());
        break;
        }
        case 8: {
          cmgt(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D(), GetSrc(Op->Header.Args[1].ID()).V2D());
        break;
        }
        default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
        }
        break;
      }
      case IR::OP_VZIP: {
        auto Op = IROp->C<IR::IROp_VZip>();
        LogMan::Throw::A(Op->RegisterSize == 16, "Can't handle register size of: %d", Op->RegisterSize);
        switch (Op->ElementSize) {
        case 1: {
          zip1(GetDst(Node).V16B(), GetSrc(Op->Header.Args[0].ID()).V16B(), GetSrc(Op->Header.Args[1].ID()).V16B());
        break;
        }
        case 2: {
          zip1(GetDst(Node).V8H(), GetSrc(Op->Header.Args[0].ID()).V8H(), GetSrc(Op->Header.Args[1].ID()).V8H());
        break;
        }
        case 4: {
          zip1(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S(), GetSrc(Op->Header.Args[1].ID()).V4S());
        break;
        }
        case 8: {
          zip1(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D(), GetSrc(Op->Header.Args[1].ID()).V2D());
        break;
        }
        default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
        }
        break;
      }
      case IR::OP_VZIP2: {
        auto Op = IROp->C<IR::IROp_VZip2>();
        LogMan::Throw::A(Op->RegisterSize == 16, "Can't handle register size of: %d", Op->RegisterSize);
        switch (Op->ElementSize) {
        case 1: {
          zip2(GetDst(Node).V16B(), GetSrc(Op->Header.Args[0].ID()).V16B(), GetSrc(Op->Header.Args[1].ID()).V16B());
        break;
        }
        case 2: {
          zip2(GetDst(Node).V8H(), GetSrc(Op->Header.Args[0].ID()).V8H(), GetSrc(Op->Header.Args[1].ID()).V8H());
        break;
        }
        case 4: {
          zip2(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S(), GetSrc(Op->Header.Args[1].ID()).V4S());
        break;
        }
        case 8: {
          zip2(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D(), GetSrc(Op->Header.Args[1].ID()).V2D());
        break;
        }
        default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
        }
        break;
      }
      case IR::OP_VBITCAST: {
        auto Op = IROp->C<IR::IROp_VBitcast>();
        mov(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
      break;
      }
      case IR::OP_VEXTR: {
        auto Op = IROp->C<IR::IROp_VExtr>();
        // AArch64 ext op has bit arrangement as [Vm:Vn] so arguments need to be swapped
        ext(GetDst(Node).V16B(), GetSrc(Op->Header.Args[1].ID()).V16B(), GetSrc(Op->Header.Args[0].ID()).V16B(), Op->Index);
        break;
      }
      case IR::OP_VUSHLS: {
        auto Op = IROp->C<IR::IROp_VUShlS>();

        switch (Op->ElementSize) {
        case 1: {
          dup(VTMP1.V16B(), GetSrc<RA_32>(Op->Header.Args[1].ID()));
          ushl(GetDst(Node).V16B(), GetSrc(Op->Header.Args[0].ID()).V16B(), VTMP1.V16B());
        break;
        }
        case 2: {
          dup(VTMP1.V8H(), GetSrc<RA_32>(Op->Header.Args[1].ID()));
          ushl(GetDst(Node).V8H(), GetSrc(Op->Header.Args[0].ID()).V8H(), VTMP1.V8H());
        break;
        }
        case 4: {
          dup(VTMP1.V4S(), GetSrc<RA_32>(Op->Header.Args[1].ID()));
          ushl(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S(), VTMP1.V4S());
        break;
        }
        case 8: {
          dup(VTMP1.V2D(), GetSrc<RA_64>(Op->Header.Args[1].ID()));
          ushl(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D(), VTMP1.V2D());
        break;
        }
        default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
        }
        break;
      }
      case IR::OP_VSLI: {
        auto Op = IROp->C<IR::IROp_VSLI>();
        eor(VTMP1.V16B(), VTMP1.V16B(), VTMP1.V16B());
        sli(VTMP1.V16B(), GetSrc(Op->Header.Args[0].ID()).V16B(), Op->ByteShift);
        mov(GetDst(Node).V16B(), VTMP1.V16B());
        break;
      }
      case IR::OP_VSRI: {
        auto Op = IROp->C<IR::IROp_VSRI>();
        eor(VTMP1.V16B(), VTMP1.V16B(), VTMP1.V16B());
        sri(VTMP1.V16B(), GetSrc(Op->Header.Args[0].ID()).V16B(), Op->ByteShift);
        mov(GetDst(Node).V16B(), VTMP1.V16B());
        break;
      }
      case IR::OP_VUMIN: {
        auto Op = IROp->C<IR::IROp_VUMin>();
        switch (Op->ElementSize) {
        case 1: {
          umin(GetDst(Node).V16B(), GetSrc(Op->Header.Args[0].ID()).V16B(), GetSrc(Op->Header.Args[1].ID()).V16B());
        break;
        }
        case 2: {
          umin(GetDst(Node).V8H(), GetSrc(Op->Header.Args[0].ID()).V8H(), GetSrc(Op->Header.Args[1].ID()).V8H());
        break;
        }
        case 4: {
          umin(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S(), GetSrc(Op->Header.Args[1].ID()).V4S());
        break;
        }
        case 8: {
          umin(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D(), GetSrc(Op->Header.Args[1].ID()).V2D());
        break;
        }
        default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
        }
        break;
      }
      case IR::OP_VSMIN: {
        auto Op = IROp->C<IR::IROp_VSMin>();
        switch (Op->ElementSize) {
        case 1: {
          smin(GetDst(Node).V16B(), GetSrc(Op->Header.Args[0].ID()).V16B(), GetSrc(Op->Header.Args[1].ID()).V16B());
        break;
        }
        case 2: {
          smin(GetDst(Node).V8H(), GetSrc(Op->Header.Args[0].ID()).V8H(), GetSrc(Op->Header.Args[1].ID()).V8H());
        break;
        }
        case 4: {
          smin(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S(), GetSrc(Op->Header.Args[1].ID()).V4S());
        break;
        }
        case 8: {
          smin(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D(), GetSrc(Op->Header.Args[1].ID()).V2D());
        break;
        }
        default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
        }
        break;
      }
      case IR::OP_CYCLECOUNTER: {
#ifdef DEBUG_CYCLES
          movz(GetDst<RA_64>(Node), 0);
#else
          mrs(GetDst<RA_64>(Node), CNTVCT_EL0);
#endif
        break;
      }
      case IR::OP_DUMMY:
        break;
      default:
        LogMan::Msg::A("Unknown IR Op: %d(%s)", IROp->Op, FEXCore::IR::GetName(IROp->Op).data());
        break;
      }

      // CodeLast is inclusive. So we still need to dump the CodeLast op as well
      if (CodeBegin == CodeLast) {
        break;
      }
      ++CodeBegin;
    }

    if (BlockIROp->Next.ID() == 0) {
      break;
    } else {
      BlockNode = BlockIROp->Next.GetNode(ListBegin);
    }
  }

  FinalizeCode();

  CPU.EnsureIAndDCacheCoherency(reinterpret_cast<void*>(Entry), Buffer->GetOffsetAddress<uint64_t>(GetCursorOffset()) - reinterpret_cast<uint64_t>(Entry));
#if _M_X86_64
  if (!CustomDispatchGenerated) {
    auto CodeEnd = Buffer->GetOffsetAddress<uint64_t>(GetCursorOffset());
    HostToGuest[State->State.State.rip] = std::make_pair(Entry, CodeEnd);
    return (void*)SimulatorExecution;
  }
#endif

  return reinterpret_cast<void*>(Entry);
}

void JITCore::CreateCustomDispatch(FEXCore::Core::InternalThreadState *Thread) {
  auto Buffer = GetBuffer();
  DispatchPtr = Buffer->GetOffsetAddress<CustomDispatch>(GetCursorOffset());
  EmissionCheckScope(this, 0);

  // while (!Thread->State.RunningEvents.ShouldStop.load()) {
  //    Ptr = FindBlock(RIP)
  //    if (!Ptr)
  //      Ptr = CTX->CompileBlock(RIP);
  //
  //    if (Ptr)
  //      Ptr();
  //    else
  //    {
  //      Ptr = FallbackCore->CompileBlock()
  //      if (Ptr)
  //        Ptr()
  //      else {
  //        ShouldStop = true;
  //      }
  //    }
  // }

  // Push all the register we need to save
  PushCalleeSavedRegisters();

  // Push our memory base to the correct register
  void *Memory = CTX->MemoryMapper.GetMemoryBase();
  LoadConstant(MEM_BASE, (uint64_t)Memory);
  // Move our thread pointer to the correct register
  // This is passed in to parameter 0 (x0)
  mov(STATE, x0);

  aarch64::Label LoopTop;
  bind(&LoopTop);

  // Load in our RIP
  // Don't modify x2 since it contains our RIP once the block doesn't exist
  ldr(x2, MemOperand(STATE, offsetof(FEXCore::Core::ThreadState, State.rip)));
  LoadConstant(x0, Thread->BlockCache->GetPagePointer());

  // Offset the address and add to our page pointer
  lsr(x1, x2, 12);

  // Load the pointer from the offset
  ldr(x0, MemOperand(x0, x1, Shift::LSL, 3));
  aarch64::Label NoBlock;

  // If page pointer is zero then we have no block
  cbz(x0, &NoBlock);

  // Steal the page offset
  and_(x1, x2, 0x0FFF);

  // Now load from that pointer offset by the page offset to get our real block
  ldr(x0, MemOperand(x0, x1, Shift::LSL, 3));
  cbz(x0, &NoBlock);

  // If we've made it here then we have a real compiled block
  {
    blr(x0);
  }

  aarch64::Label ExitCheck;
  bind(&ExitCheck);

  constexpr uint64_t ShouldStopOffset = offsetof(FEXCore::Core::ThreadState, RunningEvents.ShouldStop);
  // If we don't need to stop then keep going
  add(x1, STATE, ShouldStopOffset);
  ldarb(x0, MemOperand(x1));
  cbz(x0, &LoopTop);

  PopCalleeSavedRegisters();

  // Return from the function
  // LR is set to the correct return location now
  ret();

  aarch64::Label FallbackCore;
  // Need to create the block
  {
    bind(&NoBlock);

    LoadConstant(x0, reinterpret_cast<uintptr_t>(CTX));
    mov(x1, STATE);

#if _M_X86_64
    CallRuntime(CompileBlockThunk);
#else
    using ClassPtrType = uintptr_t (FEXCore::Context::Context::*)(FEXCore::Core::InternalThreadState *, uint64_t);
    union PtrCast {
      ClassPtrType ClassPtr;
      uintptr_t Data;
    };

    PtrCast Ptr;
    Ptr.ClassPtr = &FEXCore::Context::Context::CompileBlock;
    LoadConstant(x3, Ptr.Data);

    stp(STATE, MEM_BASE, MemOperand(sp, -16, PreIndex));
    // X2 contains our guest RIP
    blr(x3); // { CTX, ThreadState, RIP}
    ldp(STATE, MEM_BASE, MemOperand(sp, 16, PostIndex));
#endif
    // X0 now contains either nullptr or block pointer
    cbz(x0, &FallbackCore);
    blr(x0);

    b(&ExitCheck);
  }

  aarch64::Label ExitError;
  // We need to fallback to our fallback core
  {
    bind(&FallbackCore);

#if _M_X86_64
    // XXX: Fallback core doesn't work on x86-64
    // We can't tell the difference between simulator entry points and not
    b(&ExitError);
#else
    LoadConstant(x0, reinterpret_cast<uintptr_t>(CTX));
    mov(x1, STATE);

    using ClassPtrType = uintptr_t (FEXCore::Context::Context::*)(FEXCore::Core::InternalThreadState *, uint64_t);
    union PtrCast {
      ClassPtrType ClassPtr;
      uintptr_t Data;
    };

    PtrCast Ptr;
    Ptr.ClassPtr = &FEXCore::Context::Context::CompileFallbackBlock;
    LoadConstant(x3, Ptr.Data);

    stp(STATE, MEM_BASE, MemOperand(sp, -16, PreIndex));
    // X2 contains our guest RIP
    blr(x3); // {ThreadState, RIP}
    ldp(STATE, MEM_BASE, MemOperand(sp, 16, PostIndex));
#endif
    // X0 now contains either nullptr or block pointer
    cbz(x0, &ExitError);
    blr(x0);

    b(&ExitCheck);
  }

  // Exit error
  {
    bind(&ExitError);
    LoadConstant(x0, 1);
    add(x1, STATE, ShouldStopOffset);
    stlrb(x0, MemOperand(x1));
    b(&ExitCheck);
  }

#if _M_X86_64
  CustomDispatchEnd = Buffer->GetOffsetAddress<uint64_t>(GetCursorOffset());
#endif

  FinalizeCode();
  CPU.EnsureIAndDCacheCoherency(reinterpret_cast<void*>(DispatchPtr), Buffer->GetOffsetAddress<uint64_t>(GetCursorOffset()) - reinterpret_cast<uint64_t>(DispatchPtr));
  // XXX: Crashes currently.
  // Disabling will be useful for debugging ThreadState
  CustomDispatchGenerated = true;
}

FEXCore::CPU::CPUBackend *CreateJITCore(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread) {
  return new JITCore(ctx, Thread);
}
}
