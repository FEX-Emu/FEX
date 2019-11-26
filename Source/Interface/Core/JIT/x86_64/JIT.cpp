#include "Interface/Context/Context.h"
#include "Interface/Core/BlockSamplingData.h"
#include "Interface/Core/InternalThreadState.h"
#include "Interface/IR/Passes/RegisterAllocationPass.h"

#include "Interface/Core/JIT/x86_64/JIT.h"
#include <xbyak/xbyak.h>
using namespace Xbyak;

#include <FEXCore/Core/CPUBackend.h>
#include <FEXCore/IR/IR.h>
#include <FEXCore/IR/IntrusiveIRList.h>
// #define DEBUG_RA 1
// #define DEBUG_CYCLES

namespace FEXCore::CPU {
static void PrintValue(uint64_t Value) {
  LogMan::Msg::D("Value: 0x%lx", Value);
}
// Temp registers
// rax, rcx, rdx, rsi, r8, r9,
// r10, r11
//
// Callee Saved
// rbx, rbp, r12, r13, r14, r15
//
// 1St Argument: rdi <ThreadState>
// XMM:
// All temp
#define STATE rdi
using namespace Xbyak::util;
const std::array<Xbyak::Reg, 11> RA64 = { rsi, r8, r9, r10, r11, rbx, rbp, r12, r13, r14, r15 };
const std::array<Xbyak::Reg, 11> RA32 = { esi, r8d, r9d, r10d, r11d, ebx, ebp, r12d, r13d, r14d, r15d };
const std::array<Xbyak::Reg, 11> RA16 = { si, r8w, r9w, r10w, r11w, bx, bp, r12w, r13w, r14w, r15w };
const std::array<Xbyak::Reg, 11> RA8 = { sil, r8b, r9b, r10b, r11b, bl, bpl, r12b, r13b, r14b, r15b };
const std::array<Xbyak::Reg, 11> RAXMM = { xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm8, xmm9, xmm10 };
const std::array<Xbyak::Xmm, 11> RAXMM_x = { xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm8, xmm9, xmm10 };

class JITCore final : public CPUBackend, public Xbyak::CodeGenerator {
public:
  explicit JITCore(FEXCore::Context::Context *ctx);
  ~JITCore() override;
  std::string GetName() override { return "JIT"; }
  void *CompileCode(FEXCore::IR::IRListView<true> const *IR, FEXCore::Core::DebugData *DebugData) override;

  void *MapRegion(void* HostPtr, uint64_t, uint64_t) override { return HostPtr; }

  bool NeedsOpDispatch() override { return true; }

  bool HasCustomDispatch() const override { return CustomDispatchGenerated; }

  void ExecuteCustomDispatch(FEXCore::Core::ThreadState *Thread) override {
    DispatchPtr(reinterpret_cast<FEXCore::Core::InternalThreadState*>(Thread));
  }

private:
  FEXCore::Context::Context *CTX;
  FEXCore::IR::IRListView<true> const *CurrentIR;
  std::unordered_map<IR::OrderedNodeWrapper::NodeOffsetType, Label> JumpTargets;

  std::vector<uint8_t> Stack;
  bool MemoryDebug = false;

  /**
   * @name Register Allocation
   * @{ */
  constexpr static uint32_t NumGPRs = RA64.size(); // 4 is the minimum required for GPR ops
  constexpr static uint32_t NumXMMs = RAXMM.size();
  constexpr static uint32_t RegisterCount = NumGPRs + NumXMMs;
  constexpr static uint32_t RegisterClasses = 2;

  constexpr static uint32_t GPRBase = 0;
  constexpr static uint32_t GPRClass = IR::RegisterAllocationPass::GPRClass;
  constexpr static uint32_t XMMBase = NumGPRs;
  constexpr static uint32_t XMMClass = IR::RegisterAllocationPass::FPRClass;

  IR::RegisterAllocationPass::RegisterSet *RASet;
  /**  @} */

  constexpr static uint8_t RA_8 = 0;
  constexpr static uint8_t RA_16 = 1;
  constexpr static uint8_t RA_32 = 2;
  constexpr static uint8_t RA_64 = 3;
  constexpr static uint8_t RA_XMM = 4;

  IR::RegisterAllocationPass::RegisterGraph *Graph;
  uint32_t GetPhys(uint32_t Node);

  template<uint8_t RAType>
  Xbyak::Reg GetSrc(uint32_t Node);

  template<uint8_t RAType>
  Xbyak::Reg GetDst(uint32_t Node);

  Xbyak::Xmm GetSrc(uint32_t Node);
  Xbyak::Xmm GetDst(uint32_t Node);

  void CreateCustomDispatch();
  bool CustomDispatchGenerated {false};
  using CustomDispatch = void(*)(FEXCore::Core::InternalThreadState *Thread);
  CustomDispatch DispatchPtr{};
  IR::RegisterAllocationPass *RAPass;

#ifdef BLOCKSTATS
  bool GetSamplingData {true};
#endif
};

JITCore::JITCore(FEXCore::Context::Context *ctx)
  : CodeGenerator(1024 * 1024 * 32)
  , CTX {ctx} {
  Stack.resize(9000 * 16 * 64);

  RAPass = CTX->GetRegisterAllocatorPass();
  RAPass->SetSupportsSpills(true);

  RASet = RAPass->AllocateRegisterSet(RegisterCount, RegisterClasses);
  RAPass->AddRegisters(RASet, GPRClass, GPRBase, NumGPRs);
  RAPass->AddRegisters(RASet, XMMClass, XMMBase, NumXMMs);

  Graph = RAPass->AllocateRegisterGraph(RASet, 9000);
  CreateCustomDispatch();
}

JITCore::~JITCore() {
  printf("Used %ld bytes for compiling\n", getCurr<uintptr_t>() - getCode<uintptr_t>());
	RAPass->FreeRegisterSet(RASet);
	RAPass->FreeRegisterGraph();
}

static void LoadMem(uint64_t Addr, uint64_t Data, uint8_t Size) {
  LogMan::Msg::D("Loading from guestmem: 0x%lx (%d)", Addr, Size);
  LogMan::Msg::D("\tLoading: 0x%016lx", Data);
}

static void StoreMem(uint64_t Addr, uint64_t Data, uint8_t Size) {
  LogMan::Msg::D("Storing guestmem: 0x%lx (%d)", Addr, Size);
  LogMan::Msg::D("\tStoring: 0x%016lx", Data);
}

uint32_t JITCore::GetPhys(uint32_t Node) {
  uint32_t Reg = RAPass->GetNodeRegister(Node);

  if (Reg < XMMBase)
    return Reg;
  else if (Reg != ~0U)
    return Reg - XMMBase;
  else
    LogMan::Msg::A("Couldn't Allocate register for node: ssa%d", Node);

  return ~0U;
}

template<uint8_t RAType>
Xbyak::Reg JITCore::GetSrc(uint32_t Node) {
  // rax, rcx, rdx, rsi, r8, r9,
  // r10
  // Callee Saved
  // rbx, rbp, r12, r13, r14, r15
  uint32_t Reg = GetPhys(Node);
  if (RAType == RA_64)
    return RA64[Reg];
  else if (RAType == RA_XMM)
    return RAXMM[Reg];
  else if (RAType == RA_32)
    return RA32[Reg];
  else if (RAType == RA_16)
    return RA16[Reg];
  else if (RAType == RA_8)
    return RA8[Reg];
}

Xbyak::Xmm JITCore::GetSrc(uint32_t Node) {
  uint32_t Reg = GetPhys(Node);
  return RAXMM_x[Reg];
}

template<uint8_t RAType>
Xbyak::Reg JITCore::GetDst(uint32_t Node) {
  uint32_t Reg = GetPhys(Node);
  if (RAType == RA_64)
    return RA64[Reg];
  else if (RAType == RA_XMM)
    return RAXMM[Reg];
  else if (RAType == RA_32)
    return RA32[Reg];
  else if (RAType == RA_16)
    return RA16[Reg];
  else if (RAType == RA_8)
    return RA8[Reg];
}

Xbyak::Xmm JITCore::GetDst(uint32_t Node) {
  uint32_t Reg = GetPhys(Node);
  return RAXMM_x[Reg];
}

void *JITCore::CompileCode([[maybe_unused]] FEXCore::IR::IRListView<true> const *IR, [[maybe_unused]] FEXCore::Core::DebugData *DebugData) {
  JumpTargets.clear();
  CurrentIR = IR;

  uintptr_t ListBegin = CurrentIR->GetListData();
  uintptr_t DataBegin = CurrentIR->GetData();

  uint32_t SSACount = CurrentIR->GetSSACount();
  uint64_t ListStackSize = SSACount * 16;
  if (ListStackSize > Stack.size()) {
    Stack.resize(ListStackSize);
  }

	void *Entry = getCurr<void*>();

  LogMan::Throw::A(RAPass->HasFullRA(), "Needs RA");

  uint32_t SpillSlots = RAPass->SpillSlots();

  push(rbx);
  push(rbp);
  push(r12);
  push(r13);
  push(r14);
  push(r15);

  if (SpillSlots) {
    sub(rsp, SpillSlots * 16);
  }

  auto HeaderIterator = CurrentIR->begin();
  IR::OrderedNodeWrapper *HeaderNodeWrapper = HeaderIterator();
  IR::OrderedNode *HeaderNode = HeaderNodeWrapper->GetNode(ListBegin);
  auto HeaderOp = HeaderNode->Op(DataBegin)->CW<FEXCore::IR::IROp_IRHeader>();
  LogMan::Throw::A(HeaderOp->Header.Op == IR::OP_IRHEADER, "First op wasn't IRHeader");

#ifdef BLOCKSTATS
  BlockSamplingData::BlockData *SamplingData = CTX->BlockData->GetBlockData(HeaderOp->Entry);
  if (GetSamplingData) {
    mov(rcx, reinterpret_cast<uintptr_t>(SamplingData));
    rdtsc();
    shl(rdx, 32);
    or(rax, rdx);
    mov(qword [rcx + offsetof(BlockSamplingData::BlockData, Start)], rax);
  }

  auto ExitBlock = [&]() {
    if (GetSamplingData) {
      mov(rcx, reinterpret_cast<uintptr_t>(SamplingData));
      // Get time
      rdtsc();
      shl(rdx, 32);
      or(rax, rdx);

      // Calculate time spent in block
      mov(rdx, qword [rcx + offsetof(BlockSamplingData::BlockData, Start)]);
      sub(rax, rdx);

      // Add time to total time
      add(qword [rcx + offsetof(BlockSamplingData::BlockData, TotalTime)], rax);

      // Increment call count
      inc(qword [rcx + offsetof(BlockSamplingData::BlockData, TotalCalls)]);

      // Calculate min
      mov(rdx, qword [rcx + offsetof(BlockSamplingData::BlockData, Min)]);
      cmp(rdx, rax);
      cmova(rdx, rax);
      mov(qword [rcx + offsetof(BlockSamplingData::BlockData, Min)], rdx);

      // Calculate max
      mov(rdx, qword [rcx + offsetof(BlockSamplingData::BlockData, Max)]);
      cmp(rdx, rax);
      cmovb(rdx, rax);
      mov(qword [rcx + offsetof(BlockSamplingData::BlockData, Max)], rdx);
    }
  };
#endif

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

      L(IsTarget->second);
    }

    while (1) {
      OrderedNodeWrapper *WrapperOp = CodeBegin();
      OrderedNode *RealNode = WrapperOp->GetNode(ListBegin);
      FEXCore::IR::IROp_Header *IROp = RealNode->Op(DataBegin);
      uint8_t OpSize = IROp->Size;
      uint32_t Node = WrapperOp->ID();

      #ifdef DEBUG_RA
      if (IROp->Op != IR::OP_BEGINBLOCK &&
          IROp->Op != IR::OP_CONDJUMP &&
          IROp->Op != IR::OP_JUMP) {
        std::stringstream Inst;
        auto Name = FEXCore::IR::GetName(IROp->Op);

        if (IROp->HasDest) {
          uint32_t PhysReg = RAPass->GetNodeRegister(Node);
          if (PhysReg >= XMMBase)
            Inst << "\tXMM" << GetPhys(Node) << " = " << Name << " ";
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
          if (PhysReg >= XMMBase)
            Inst << "XMM" << GetPhys(ArgNode) << (i + 1 == NumArgs ? "" : ", ");
          else
            Inst << "Reg" << GetPhys(ArgNode) << (i + 1 == NumArgs ? "" : ", ");
        }

        LogMan::Msg::D("%s", Inst.str().c_str());
      }
      #endif

      switch (IROp->Op) {
        case IR::OP_BEGINBLOCK: {
          auto IsTarget = JumpTargets.find(Node);
          if (IsTarget == JumpTargets.end()) {
            IsTarget = JumpTargets.try_emplace(Node).first;
          }
          else {
          }

          L(IsTarget->second);
          break;
        }
        case IR::OP_ENDBLOCK: {
          auto Op = IROp->C<IR::IROp_EndBlock>();
          if (Op->RIPIncrement) {
            add(qword [STATE + offsetof(FEXCore::Core::CPUState, rip)], Op->RIPIncrement);
          }
          break;
        }
        case IR::OP_EXITFUNCTION: {
          if (SpillSlots) {
            add(rsp, SpillSlots * 16);
          }

          pop(r15);
          pop(r14);
          pop(r13);
          pop(r12);
          pop(rbp);
          pop(rbx);

#ifdef BLOCKSTATS
          ExitBlock();
#endif
          ret();
          break;
        }
        case IR::OP_BREAK: {
          auto Op = IROp->C<IR::IROp_Break>();
          switch (Op->Reason) {
            case 0: // Hard fault
            case 5: // Guest ud2
              ud2();
            break;
            case 4: // HLT
            case 6: // INT3
            {
              mov(al, 1);

              auto offset = Op->Reason == 4 ?
                  offsetof(FEXCore::Core::ThreadState, RunningEvents.ShouldStop) // HLT
                : offsetof(FEXCore::Core::ThreadState, RunningEvents.ShouldPause); // INT3
              xchg(byte [STATE + offset], al);

              // This code matches what is in EXITFUNCTION
              if (SpillSlots) {
                add(rsp, SpillSlots * 16);
              }

              pop(r15);
              pop(r14);
              pop(r13);
              pop(r12);
              pop(rbp);
              pop(rbx);

#ifdef BLOCKSTATS
              ExitBlock();
#endif
              ret();
            break;
            }
            default: LogMan::Msg::A("Unknown Break reason: %d", Op->Reason);
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

          jmp(*TargetLabel, T_NEAR);

          break;
        }
        default: break;
      }

      switch (IROp->Op) {
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

          // Take branch if (src != 0)
          cmp(GetSrc<RA_64>(Op->Header.Args[0].ID()), 0);
          jne(*TrueTargetLabel, T_NEAR);
          jmp(*FalseTargetLabel, T_NEAR);
          break;
        }
        case IR::OP_LOADCONTEXT: {
          auto Op = IROp->C<IR::IROp_LoadContext>();
          switch (Op->Size) {
          case 1: {
            mov(GetDst<RA_8>(Node), byte [STATE + Op->Offset]);
          }
          break;
          case 2: {
            mov(GetDst<RA_16>(Node), word [STATE + Op->Offset]);
          }
          break;
          case 4: {
            mov(GetDst<RA_32>(Node), dword [STATE + Op->Offset]);
          }
          break;
          case 8: {
            mov(GetDst<RA_64>(Node), qword [STATE + Op->Offset]);
          }
          break;
          case 16: {
            if (Op->Offset % 16 == 0)
              movaps(GetDst(Node), xword [STATE + Op->Offset]);
            else
              movups(GetDst(Node), xword [STATE + Op->Offset]);
          }
          break;
          default:  LogMan::Msg::A("Unhandled LoadContext size: %d", Op->Size);
          }
          break;
        }
        case IR::OP_STORECONTEXT: {
          auto Op = IROp->C<IR::IROp_StoreContext>();

          switch (Op->Size) {
          case 1: {
            mov(byte [STATE + Op->Offset], GetSrc<RA_8>(Op->Header.Args[0].ID()));
          }
          break;

          case 2: {
            mov(word [STATE + Op->Offset], GetSrc<RA_16>(Op->Header.Args[0].ID()));
          }
          break;
          case 4: {
            mov(dword [STATE + Op->Offset], GetSrc<RA_32>(Op->Header.Args[0].ID()));
          }
          break;
          case 8: {
            mov(qword [STATE + Op->Offset], GetSrc<RA_64>(Op->Header.Args[0].ID()));
          }
          break;
          case 16: {
            if (Op->Offset % 16 == 0)
              movaps(xword [STATE + Op->Offset], GetSrc(Op->Header.Args[0].ID()));
            else
              movups(xword [STATE + Op->Offset], GetSrc(Op->Header.Args[0].ID()));
          }
          break;
          default:  LogMan::Msg::A("Unhandled StoreContext size: %d", Op->Size);
          }
          break;
        }
        case IR::OP_FILLREGISTER: {
          auto Op = IROp->C<IR::IROp_FillRegister>();
          uint32_t SlotOffset = Op->Slot * 16;
          switch (OpSize) {
          case 1: {
            mov(GetDst<RA_8>(Node), byte [rsp + SlotOffset]);
          }
          break;
          case 2: {
            mov(GetDst<RA_16>(Node), word [rsp + SlotOffset]);
          }
          break;
          case 4: {
            mov(GetDst<RA_32>(Node), dword [rsp + SlotOffset]);
          }
          break;
          case 8: {
            mov(GetDst<RA_64>(Node), qword [rsp + SlotOffset]);
          }
          break;
          case 16: {
            movaps(GetDst(Node), xword [rsp + SlotOffset]);
          }
          break;
          default:  LogMan::Msg::A("Unhandled FillRegister size: %d", OpSize);
          }
          break;
        }
        case IR::OP_SPILLREGISTER: {
          auto Op = IROp->C<IR::IROp_SpillRegister>();
          uint32_t SlotOffset = Op->Slot * 16;
          switch (OpSize) {
          case 1: {
            mov(byte [rsp + SlotOffset], GetSrc<RA_8>(Op->Header.Args[0].ID()));
          }
          break;
          case 2: {
            mov(word [rsp + SlotOffset], GetSrc<RA_16>(Op->Header.Args[0].ID()));
          }
          break;
          case 4: {
            mov(dword [rsp + SlotOffset], GetSrc<RA_32>(Op->Header.Args[0].ID()));
          }
          break;
          case 8: {
            mov(qword [rsp + SlotOffset], GetSrc<RA_64>(Op->Header.Args[0].ID()));
          }
          break;
          case 16: {
            movaps(xword [rsp + SlotOffset], GetSrc(Op->Header.Args[0].ID()));
          }
          break;
          default:  LogMan::Msg::A("Unhandled SpillRegister size: %d", OpSize);
          }
          break;
        }
        case IR::OP_ADD: {
          auto Op = IROp->C<IR::IROp_Add>();
          auto Dst = GetDst<RA_64>(Node);
          mov(rax, GetSrc<RA_64>(Op->Header.Args[1].ID()));
          add(rax, GetSrc<RA_64>(Op->Header.Args[0].ID()));
          mov(Dst, rax);
          break;
        }
        case IR::OP_NEG: {
          auto Op = IROp->C<IR::IROp_Neg>();
          auto Dst = GetDst<RA_64>(Node);
          mov(Dst, GetSrc<RA_64>(Op->Header.Args[0].ID()));
          neg(Dst);
          break;
        }
        case IR::OP_SUB: {
          auto Op = IROp->C<IR::IROp_Sub>();
          auto Dst = GetDst<RA_64>(Node);
          mov(rax, GetSrc<RA_64>(Op->Header.Args[0].ID()));
          sub(rax, GetSrc<RA_64>(Op->Header.Args[1].ID()));
          mov(Dst, rax);
          break;
        }
        case IR::OP_XOR: {
          auto Op = IROp->C<IR::IROp_Xor>();
          auto Dst = GetDst<RA_64>(Node);
          mov(rax, GetSrc<RA_64>(Op->Header.Args[1].ID()));
          xor(rax, GetSrc<RA_64>(Op->Header.Args[0].ID()));
          mov(Dst, rax);
          break;
        }
        case IR::OP_AND: {
          auto Op = IROp->C<IR::IROp_And>();
          auto Dst = GetDst<RA_64>(Node);
          mov(rax, GetSrc<RA_64>(Op->Header.Args[1].ID()));
          and(rax, GetSrc<RA_64>(Op->Header.Args[0].ID()));
          mov(Dst, rax);
          break;
        }
        case IR::OP_OR: {
          auto Op = IROp->C<IR::IROp_Or>();
          auto Dst = GetDst<RA_64>(Node);
          mov(rax, GetSrc<RA_64>(Op->Header.Args[1].ID()));
          or (rax, GetSrc<RA_64>(Op->Header.Args[0].ID()));
          mov(Dst, rax);
          break;
        }
        case IR::OP_MOV: {
          auto Op = IROp->C<IR::IROp_Mov>();
          mov (GetDst<RA_64>(Node), GetSrc<RA_64>(Op->Header.Args[0].ID()));
          break;
        }
        case IR::OP_CONSTANT: {
          auto Op = IROp->C<IR::IROp_Constant>();
          mov(GetDst<RA_64>(Node), Op->Constant);
          break;
        }
        case IR::OP_POPCOUNT: {
          auto Op = IROp->C<IR::IROp_Popcount>();
          auto Dst64 = GetDst<RA_64>(Node);

          switch (OpSize) {
          case 1:
            movzx(GetDst<RA_8>(Node), GetSrc<RA_8>(Op->Header.Args[0].ID()));
            popcnt(Dst64, Dst64);
          break;
          case 2: {
            movzx(GetDst<RA_16>(Node), GetSrc<RA_16>(Op->Header.Args[0].ID()));
            popcnt(Dst64, Dst64);
            break;
          }
          case 4:
            popcnt(GetDst<RA_32>(Node), GetSrc<RA_32>(Op->Header.Args[0].ID()));
          break;
          case 8:
            popcnt(GetDst<RA_64>(Node), GetSrc<RA_64>(Op->Header.Args[0].ID()));
          break;
          }
          break;
        }
        case IR::OP_ZEXT: {
          auto Op = IROp->C<IR::IROp_Zext>();
          LogMan::Throw::A(Op->SrcSize <= 64, "Can't support Zext of size: %ld", Op->SrcSize);

          uint32_t PhysReg = RAPass->GetNodeRegister(Op->Header.Args[0].ID());
          if (PhysReg >= XMMBase) {
            // XMM -> GPR transfer with free truncation
            switch (Op->SrcSize) {
            case 8:
              pextrb(al, GetSrc(Op->Header.Args[0].ID()), 0);
            break;
            case 16:
              pextrw(ax, GetSrc(Op->Header.Args[0].ID()), 0);
            break;
            case 32:
              pextrd(eax, GetSrc(Op->Header.Args[0].ID()), 0);
            break;
            case 64:
              pextrw(rax, GetSrc(Op->Header.Args[0].ID()), 0);
            break;
            default: LogMan::Msg::A("Unhandled Zext size: %d", Op->SrcSize); break;
            }
            auto Dst = GetDst<RA_64>(Node);
            mov(Dst, rax);
          }
          else {
            if (Op->SrcSize == 64) {
              vmovq(xmm15, Reg64(GetSrc<RA_64>(Op->Header.Args[0].ID()).getIdx()));
              movapd(GetDst(Node), xmm15);
            }
            else {
              auto Dst = GetDst<RA_64>(Node);
              mov(rax, uint64_t((1ULL << Op->SrcSize) - 1));
              and(rax, GetSrc<RA_64>(Op->Header.Args[0].ID()));
              mov(Dst, rax);
            }
          }
          break;
        }
        case IR::OP_SEXT: {
          auto Op = IROp->C<IR::IROp_Sext>();
          LogMan::Throw::A(Op->SrcSize <= 64, "Can't support Zext of size: %ld", Op->SrcSize);
          auto Dst = GetDst<RA_64>(Node);

          switch (Op->SrcSize / 8) {
          case 1:
            movsx(Dst, GetSrc<RA_8>(Op->Header.Args[0].ID()));
          break;
          case 2:
            movsx(Dst, GetSrc<RA_16>(Op->Header.Args[0].ID()));
          break;
          case 4:
            movsxd(Dst.cvt64(), GetSrc<RA_32>(Op->Header.Args[0].ID()));
          break;
          case 8:
            mov(Dst, GetSrc<RA_64>(Op->Header.Args[0].ID()));
          break;
          default: LogMan::Msg::A("Unknown Sext size: %d", Op->SrcSize / 8);
          }
          break;
        }
        case IR::OP_BFE: {
          auto Op = IROp->C<IR::IROp_Bfe>();
          LogMan::Throw::A(OpSize <= 16, "OpSize is too large for BFE: %d", OpSize);
          if (OpSize == 16) {
            LogMan::Throw::A(!(Op->lsb < 64 && (Op->lsb + Op->Width > 64)), "Trying to BFE an XMM across the 64bit split: Beginning at %d, ending at %d", Op->lsb, Op->lsb + Op->Width);
            movups(xmm15, GetSrc(Op->Header.Args[0].ID()));
            uint8_t Offset = Op->lsb;
            if (Offset < 64) {
              pextrq(rax, xmm15, 0);
            }
            else {
              pextrq(rax, xmm15, 1);
              Offset -= 64;
            }

            if (Offset) {
              shr(rax, Offset);
            }

            if (Op->Width != 64) {
              mov(rcx, uint64_t((1ULL << Op->Width) - 1));
              and(rax, rcx);
            }

            mov (GetDst<RA_64>(Node), rax);
          }
          else {
            auto Dst = GetDst<RA_64>(Node);
            mov(rax, GetSrc<RA_64>(Op->Header.Args[0].ID()));

            if (Op->lsb != 0)
              shr(rax, Op->lsb);

            if (Op->Width != 64) {
              mov(rcx, uint64_t((1ULL << Op->Width) - 1));
              and(rax, rcx);
            }
            mov(Dst, rax);
          }
          break;
        }
        case IR::OP_LSHR: {
          auto Op = IROp->C<IR::IROp_Lshr>();
          uint8_t Mask = OpSize * 8 - 1;

          auto Dst = GetDst<RA_64>(Node);
          mov (rcx, GetSrc<RA_64>(Op->Header.Args[1].ID()));
          and(rcx, Mask);

          shrx(Reg32e(Dst.getIdx(), 64), GetSrc<RA_64>(Op->Header.Args[0].ID()), rcx);
          break;
        }
        case IR::OP_LSHL: {
          auto Op = IROp->C<IR::IROp_Lshl>();
          uint8_t Mask = OpSize * 8 - 1;

          auto Dst = GetDst<RA_64>(Node);
          mov (rcx, GetSrc<RA_64>(Op->Header.Args[1].ID()));
          and(rcx, Mask);

          shlx(Reg32e(Dst.getIdx(), 64), GetSrc<RA_64>(Op->Header.Args[0].ID()), rcx);
          break;
        }
        case IR::OP_ASHR: {
          auto Op = IROp->C<IR::IROp_Ashr>();
          uint8_t Mask = OpSize * 8 - 1;

          mov (rcx, GetSrc<RA_64>(Op->Header.Args[1].ID()));
          and(rcx, Mask);
          switch (OpSize) {
          case 1:
            movsx(rax, GetSrc<RA_8>(Op->Header.Args[0].ID()));
            sar(al, cl);
            movsx(GetDst<RA_64>(Node), al);
          break;
          case 2:
            movsx(rax, GetSrc<RA_16>(Op->Header.Args[0].ID()));
            sar(ax, cl);
            movsx(GetDst<RA_64>(Node), ax);
          break;
          case 4:
            sarx(Reg32e(GetDst<RA_32>(Node).getIdx(), 32), GetSrc<RA_32>(Op->Header.Args[0].ID()), ecx);
          break;
          case 8:
            sarx(Reg32e(GetDst<RA_64>(Node).getIdx(), 64), GetSrc<RA_64>(Op->Header.Args[0].ID()), rcx);
          break;
          default: LogMan::Msg::A("Unknown ASHR Size: %d\n", OpSize); break;
          };
          break;
        }
        case IR::OP_ROL: {
          auto Op = IROp->C<IR::IROp_Rol>();
          uint8_t Mask = OpSize * 8 - 1;

          mov (rcx, GetSrc<RA_64>(Op->Header.Args[1].ID()));
          and(rcx, Mask);
          switch (OpSize) {
          case 1: {
            movzx(rax, GetSrc<RA_8>(Op->Header.Args[0].ID()));
            rol(al, cl);
          break;
          }
          case 2: {
            movzx(rax, GetSrc<RA_16>(Op->Header.Args[0].ID()));
            rol(ax, cl);
          break;
          }
          case 4: {
            mov(eax, GetSrc<RA_32>(Op->Header.Args[0].ID()));
            rol(eax, cl);
          break;
          }
          case 8: {
            mov(rax, GetSrc<RA_64>(Op->Header.Args[0].ID()));
            rol(rax, cl);
          break;
          }
          }
          mov(GetDst<RA_64>(Node), rax);
          break;
        }
        case IR::OP_ROR: {
          auto Op = IROp->C<IR::IROp_Ror>();
          uint8_t Mask = OpSize * 8 - 1;

          mov (rcx, GetSrc<RA_64>(Op->Header.Args[1].ID()));
          and(rcx, Mask);
          switch (OpSize) {
          case 1: {
            movzx(rax, GetSrc<RA_8>(Op->Header.Args[0].ID()));
            ror(al, cl);
          break;
          }
          case 2: {
            movzx(rax, GetSrc<RA_16>(Op->Header.Args[0].ID()));
            ror(ax, cl);
          break;
          }
          case 4: {
            mov(eax, GetSrc<RA_32>(Op->Header.Args[0].ID()));
            ror(eax, cl);
          break;
          }
          case 8: {
            mov(rax, GetSrc<RA_64>(Op->Header.Args[0].ID()));
            ror(rax, cl);
          break;
          }
          }
          mov(GetDst<RA_64>(Node), rax);
          break;
        }
        case IR::OP_MUL: {
          auto Op = IROp->C<IR::IROp_Mul>();
          auto Dst = GetDst<RA_64>(Node);

          switch (OpSize) {
          case 1:
            movsx(rax, GetSrc<RA_8>(Op->Header.Args[0].ID()));
            movsx(rcx, GetSrc<RA_8>(Op->Header.Args[1].ID()));
            imul(cl);
            movsx(Dst, al);
          break;
          case 2:
            movsx(rax, GetSrc<RA_16>(Op->Header.Args[0].ID()));
            movsx(rcx, GetSrc<RA_16>(Op->Header.Args[1].ID()));
            imul(cx);
            movsx(Dst, ax);
          break;
          case 4:
            movsxd(rax, GetSrc<RA_32>(Op->Header.Args[0].ID()));
            imul(eax, GetSrc<RA_32>(Op->Header.Args[1].ID()));
            movsxd(Dst.cvt64(), eax);
          break;
          case 8:
            mov(rax, GetSrc<RA_64>(Op->Header.Args[0].ID()));
            imul(rax, GetSrc<RA_64>(Op->Header.Args[1].ID()));
            mov(Dst, rax);
          break;
          default: LogMan::Msg::A("Unknown Sext size: %d", OpSize);
          }
          break;
        }
        case IR::OP_MULH: {
          auto Op = IROp->C<IR::IROp_MulH>();
          switch (OpSize) {
          case 1:
            movsx(rax, GetSrc<RA_8>(Op->Header.Args[0].ID()));
            movsx(rcx, GetSrc<RA_8>(Op->Header.Args[1].ID()));
            imul(cl);
            movsx(rax, ax);
            mov(GetDst<RA_64>(Node), rax);
          break;
          case 2:
            movsx(rax, GetSrc<RA_16>(Op->Header.Args[0].ID()));
            movsx(rcx, GetSrc<RA_16>(Op->Header.Args[1].ID()));
            imul(cx);
            movsx(rax, dx);
            mov(GetDst<RA_64>(Node), rax);
          break;
          case 4:
            movsxd(rax, GetSrc<RA_32>(Op->Header.Args[0].ID()));
            imul(GetSrc<RA_32>(Op->Header.Args[1].ID()));
            movsxd(rax, edx);
            mov(GetDst<RA_64>(Node), rdx);
          break;
          case 8:
            mov(rax, GetSrc<RA_64>(Op->Header.Args[0].ID()));
            imul(GetSrc<RA_64>(Op->Header.Args[1].ID()));
            mov(GetDst<RA_64>(Node), rdx);
          break;
          default: LogMan::Msg::A("Unknown Sext size: %d", OpSize);
          }
          break;
        }
        case IR::OP_UMUL: {
          auto Op = IROp->C<IR::IROp_UMul>();
          switch (OpSize) {
          case 1:
            movzx(rax, GetSrc<RA_8>(Op->Header.Args[0].ID()));
            movzx(rcx, GetSrc<RA_8>(Op->Header.Args[1].ID()));
            mul(cl);
            movzx(rax, al);
            mov(GetDst<RA_64>(Node), rax);
          break;
          case 2:
            movzx(rax, GetSrc<RA_16>(Op->Header.Args[0].ID()));
            movzx(rcx, GetSrc<RA_16>(Op->Header.Args[1].ID()));
            mul(cx);
            movzx(rax, ax);
            mov(GetDst<RA_64>(Node), rax);
          break;
          case 4:
            mov(rax, GetSrc<RA_32>(Op->Header.Args[0].ID()));
            mul(GetSrc<RA_32>(Op->Header.Args[1].ID()));
            mov(GetDst<RA_64>(Node), rax);
          break;
          case 8:
            mov(rax, GetSrc<RA_64>(Op->Header.Args[0].ID()));
            mul(GetSrc<RA_64>(Op->Header.Args[1].ID()));
            mov(GetDst<RA_64>(Node), rax);
          break;
          default: LogMan::Msg::A("Unknown Sext size: %d", OpSize);
          }
          break;
        }
        case IR::OP_UMULH: {
          auto Op = IROp->C<IR::IROp_UMulH>();
          switch (OpSize) {
          case 1:
            movzx(rax, GetSrc<RA_8>(Op->Header.Args[0].ID()));
            movzx(rcx, GetSrc<RA_8>(Op->Header.Args[1].ID()));
            mul(cl);
            movzx(rax, ax);
            mov(GetDst<RA_64>(Node), rax);
          break;
          case 2:
            movzx(rax, GetSrc<RA_16>(Op->Header.Args[0].ID()));
            movzx(rcx, GetSrc<RA_16>(Op->Header.Args[1].ID()));
            mul(cx);
            movzx(rax, dx);
            mov(GetDst<RA_64>(Node), rax);
          break;
          case 4:
            mov(rax, GetSrc<RA_32>(Op->Header.Args[0].ID()));
            mul(GetSrc<RA_32>(Op->Header.Args[1].ID()));
            mov(GetDst<RA_64>(Node), rdx);
          break;
          case 8:
            mov(rax, GetSrc<RA_64>(Op->Header.Args[0].ID()));
            mul(GetSrc<RA_64>(Op->Header.Args[1].ID()));
            mov(GetDst<RA_64>(Node), rdx);
          break;
          default: LogMan::Msg::A("Unknown Sext size: %d", OpSize);
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
            mov(eax, GetSrc<RA_32>(Op->Header.Args[0].ID()));
            mov(edx, GetSrc<RA_32>(Op->Header.Args[1].ID()));
            mov(ecx, GetSrc<RA_32>(Op->Header.Args[2].ID()));
            idiv(ecx);
            mov(GetDst<RA_64>(Node), rax);
          break;
          }
          case 8: {
            mov(rax, GetSrc<RA_64>(Op->Header.Args[0].ID()));
            mov(rdx, GetSrc<RA_64>(Op->Header.Args[1].ID()));
            mov(rcx, GetSrc<RA_64>(Op->Header.Args[2].ID()));
            idiv(rcx);
            mov(GetDst<RA_64>(Node), rax);
          break;
          }
          default: LogMan::Msg::A("Unknown LDIV Size: %d", Size); break;
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
            mov(eax, GetSrc<RA_32>(Op->Header.Args[0].ID()));
            mov(edx, GetSrc<RA_32>(Op->Header.Args[1].ID()));
            mov(ecx, GetSrc<RA_32>(Op->Header.Args[2].ID()));
            idiv(ecx);
            mov(GetDst<RA_64>(Node), rdx);
          break;
          }

          case 8: {
            mov(rax, GetSrc<RA_64>(Op->Header.Args[0].ID()));
            mov(rdx, GetSrc<RA_64>(Op->Header.Args[1].ID()));
            mov(rcx, GetSrc<RA_64>(Op->Header.Args[2].ID()));
            idiv(rcx);
            mov(GetDst<RA_64>(Node), rdx);
          break;
          }
          default: LogMan::Msg::A("Unknown LREM Size: %d", Size); break;
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
            mov (eax, GetSrc<RA_32>(Op->Header.Args[0].ID()));
            mov (edx, GetSrc<RA_32>(Op->Header.Args[1].ID()));
            mov (ecx, GetSrc<RA_32>(Op->Header.Args[2].ID()));
            div(ecx);
            mov(GetDst<RA_64>(Node), rax);
          break;
          }
          case 8: {
            mov (rax, GetSrc<RA_64>(Op->Header.Args[0].ID()));
            mov (rdx, GetSrc<RA_64>(Op->Header.Args[1].ID()));
            mov (rcx, GetSrc<RA_64>(Op->Header.Args[2].ID()));
            div(rcx);
            mov(GetDst<RA_64>(Node), rax);
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
            mov (eax, GetSrc<RA_32>(Op->Header.Args[0].ID()));
            mov (edx, GetSrc<RA_32>(Op->Header.Args[1].ID()));
            mov (ecx, GetSrc<RA_32>(Op->Header.Args[2].ID()));
            div(ecx);
            mov(GetDst<RA_64>(Node), rdx);
          break;
          }

          case 8: {
            mov (rax, GetSrc<RA_64>(Op->Header.Args[0].ID()));
            mov (rdx, GetSrc<RA_64>(Op->Header.Args[1].ID()));
            mov (rcx, GetSrc<RA_64>(Op->Header.Args[2].ID()));
            div(rcx);
            mov(GetDst<RA_64>(Node), rdx);
          break;
          }
          default: LogMan::Msg::A("Unknown LUDIV Size: %d", Size); break;
          }
          break;
        }
        case IR::OP_LOADFLAG: {
          auto Op = IROp->C<IR::IROp_LoadFlag>();

          auto Dst = GetDst<RA_64>(Node);
          movzx(Dst, byte [STATE + (offsetof(FEXCore::Core::CPUState, flags[0]) + Op->Flag)]);
          and(Dst, 1);
          break;
        }
        case IR::OP_STOREFLAG: {
          auto Op = IROp->C<IR::IROp_StoreFlag>();

          mov (rax, GetSrc<RA_64>(Op->Header.Args[0].ID()));
          and(rax, 1);
          mov(byte [STATE + (offsetof(FEXCore::Core::CPUState, flags[0]) + Op->Flag)], al);
          break;
        }
        case IR::OP_SELECT: {
          auto Op = IROp->C<IR::IROp_Select>();
          auto Dst = GetDst<RA_64>(Node);

          mov(rax, GetSrc<RA_64>(Op->Header.Args[0].ID()));
          cmp(rax, GetSrc<RA_64>(Op->Header.Args[1].ID()));

          switch (Op->Cond.Val) {
          case FEXCore::IR::COND_EQ:
            mov(rax, GetSrc<RA_64>(Op->Header.Args[3].ID()));
            cmove(rax, GetSrc<RA_64>(Op->Header.Args[2].ID()));
          break;
          case FEXCore::IR::COND_NEQ:
            mov(rax, GetSrc<RA_64>(Op->Header.Args[3].ID()));
            cmovne(rax, GetSrc<RA_64>(Op->Header.Args[2].ID()));
          break;
          case FEXCore::IR::COND_GE:
            mov(rax, GetSrc<RA_64>(Op->Header.Args[3].ID()));
            cmovge(rax, GetSrc<RA_64>(Op->Header.Args[2].ID()));
          break;
          case FEXCore::IR::COND_LT:
            mov(rax, GetSrc<RA_64>(Op->Header.Args[2].ID()));
            cmovae(rax, GetSrc<RA_64>(Op->Header.Args[3].ID()));
          break;
          case FEXCore::IR::COND_GT:
            mov(rax, GetSrc<RA_64>(Op->Header.Args[3].ID()));
            cmovg(rax, GetSrc<RA_64>(Op->Header.Args[2].ID()));
          break;
          case FEXCore::IR::COND_LE:
            mov(rax, GetSrc<RA_64>(Op->Header.Args[3].ID()));
            cmovle(rax, GetSrc<RA_64>(Op->Header.Args[2].ID()));
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
          mov (Dst, rax);
          break;
        }
        case IR::OP_LOADMEM: {
          auto Op = IROp->C<IR::IROp_LoadMem>();
          uint64_t Memory = CTX->MemoryMapper.GetBaseOffset<uint64_t>(0);

          auto Dst = GetDst<RA_64>(Node);
          mov(rax, Memory);
          add(rax, GetSrc<RA_64>(Op->Header.Args[0].ID()));
          switch (Op->Size) {
            case 1: {
              movzx (Dst, byte [rax]);
            }
            break;
            case 2: {
              movzx (Dst, word [rax]);
            }
            break;
            case 4: {
              mov(Dst, dword [rax]);
            }
            break;
            case 8: {
              mov(Dst, qword [rax]);
            }
            break;
            case 16: {
               if (Op->Size == Op->Align)
                 movaps(GetDst(Node), xword [rax]);
               else
                 movups(GetDst(Node), xword [rax]);
               if (MemoryDebug) {
                 movq(rcx, GetDst(Node));
               }
             }
             break;
            default:  LogMan::Msg::A("Unhandled LoadMem size: %d", Op->Size);
          }
          break;
        }
        case IR::OP_STOREMEM: {
          auto Op = IROp->C<IR::IROp_StoreMem>();
          uint64_t Memory = CTX->MemoryMapper.GetBaseOffset<uint64_t>(0);

          mov(rax, Memory);
          add(rax, GetSrc<RA_64>(Op->Header.Args[0].ID()));
          switch (Op->Size) {
          case 1:
            mov(byte [rax], GetSrc<RA_8>(Op->Header.Args[1].ID()));
          break;
          case 2:
            mov(word [rax], GetSrc<RA_16>(Op->Header.Args[1].ID()));
          break;
          case 4:
            mov(dword [rax], GetSrc<RA_32>(Op->Header.Args[1].ID()));
          break;
          case 8:
            mov(qword [rax], GetSrc<RA_64>(Op->Header.Args[1].ID()));
          break;
          case 16:
            if (Op->Size == Op->Align)
              movaps(xword [rax], GetSrc(Op->Header.Args[1].ID()));
            else
              movups(xword [rax], GetSrc(Op->Header.Args[1].ID()));
          break;
          default:  LogMan::Msg::A("Unhandled StoreMem size: %d", Op->Size);
          }
          break;
        }
        case IR::OP_SYSCALL: {
          auto Op = IROp->C<IR::IROp_Syscall>();
          // XXX: This is very terrible, but I don't care for right now

          push(rdi);

          for (auto &Reg : RA64)
            push(Reg);

          // Syscall ABI for x86-64
          // this: rdi
          // Thread: rsi
          // ArgPointer: rdx (Stack)
          //
          // Result: RAX

          // These are pushed in reverse order because stacks
          for (uint32_t i = 7; i > 0; --i)
            push(GetSrc<RA_64>(Op->Header.Args[i - 1].ID()));

          mov(rsi, rdi); // Move thread in to rsi
          mov(rdi, reinterpret_cast<uint64_t>(&CTX->SyscallHandler));
          mov(rdx, rsp);

          using PtrType = uint64_t (FEXCore::SyscallHandler::*)(FEXCore::Core::InternalThreadState *Thread, FEXCore::HLE::SyscallArguments *Args);
          union {
            PtrType ptr;
            uint64_t Raw;
          } PtrCast;
          PtrCast.ptr = &FEXCore::SyscallHandler::HandleSyscall;
          mov(rax, PtrCast.Raw);
          call(rax);

          // Reload arguments just in case they are sill live after the fact
          for (uint32_t i = 0; i < 7; ++i)
            pop(GetSrc<RA_64>(Op->Header.Args[i].ID()));

          for (uint32_t i = RA64.size(); i > 0; --i)
            pop(RA64[i - 1]);

          pop(rdi);

          mov (GetDst<RA_64>(Node), rax);
          break;
        }

        case IR::OP_VEXTRACTTOGPR: {
          auto Op = IROp->C<IR::IROp_VExtractToGPR>();
          movapd(xmm15, GetSrc(Op->Header.Args[0].ID()));

          // Dst_d[Op->DestIdx] = Src2_d[Op->SrcIdx];

          // pextrq reg64/mem64, xmm, imm
          // pinsrq xmm, reg64/mem64, imm8
          switch (Op->ElementSize) {
          case 1: {
            pextrb(GetDst<RA_8>(Node), GetSrc(Op->Header.Args[0].ID()), Op->Idx);
          break;
          }
          case 2: {
            pextrw(GetDst<RA_16>(Node), GetSrc(Op->Header.Args[0].ID()), Op->Idx);
          break;
          }
          case 4: {
            pextrd(GetDst<RA_32>(Node), GetSrc(Op->Header.Args[0].ID()), Op->Idx);
          break;
          }
          case 8: {
            pextrq(GetDst<RA_64>(Node), GetSrc(Op->Header.Args[0].ID()), Op->Idx);
          break;
          }
          default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
          }

          break;
        }

        case IR::OP_PRINT: {
          auto Op = IROp->C<IR::IROp_Print>();

          push(rdi);
          push(rdi);

          for (auto &Reg : RA64)
            push(Reg);

          mov (rdi, GetSrc<RA_64>(Op->Header.Args[0].ID()));

          mov(rax, reinterpret_cast<uintptr_t>(PrintValue));
          call(rax);

          for (uint32_t i = RA64.size(); i > 0; --i)
            pop(RA64[i - 1]);

          pop(rdi);
          pop(rdi);

          break;
        }

        case IR::OP_CPUID: {
          auto Op = IROp->C<IR::IROp_CPUID>();
          using ClassPtrType = FEXCore::CPUIDEmu::FunctionResults (FEXCore::CPUIDEmu::*)(uint32_t Function);
          union {
            ClassPtrType ClassPtr;
            uint64_t Raw;
          } Ptr;
          Ptr.ClassPtr = &CPUIDEmu::RunFunction;

          for (auto &Reg : RA64)
            push(Reg);

          // CPUID ABI
          // this: rdi
          // Function: rsi
          //
          // Result: RAX, RDX. 4xi32
          push(rdi);
          mov (rsi, GetSrc<RA_64>(Op->Header.Args[0].ID()));
          mov (rdi, reinterpret_cast<uint64_t>(&CTX->CPUID));

          sub(rsp, 8); // Align

          mov(rax, Ptr.Raw);
          call(rax);

          add(rsp, 8); // Align

          pop(rdi);

          for (uint32_t i = RA64.size(); i > 0; --i)
            pop(RA64[i - 1]);

          auto Dst = GetDst(Node);
          pinsrq(Dst, rax, 0);
          pinsrq(Dst, rdx, 1);
          break;
        }
        case IR::OP_EXTRACTELEMENT: {
          auto Op = IROp->C<IR::IROp_ExtractElement>();

          uint32_t PhysReg = RAPass->GetNodeRegister(Op->Header.Args[0].ID());
          if (PhysReg >= XMMBase) {
            switch (OpSize) {
            case 1:
              pextrb(GetDst<RA_8>(Node), GetSrc(Op->Header.Args[0].ID()), Op->Idx);
            break;
            case 2:
              pextrw(GetDst<RA_16>(Node), GetSrc(Op->Header.Args[0].ID()), Op->Idx);
            break;
            case 4:
              pextrd(GetDst<RA_32>(Node), GetSrc(Op->Header.Args[0].ID()), Op->Idx);
            break;
            case 8:
              pextrq(GetDst<RA_64>(Node), GetSrc(Op->Header.Args[0].ID()), Op->Idx);
            break;
            default:  LogMan::Msg::A("Unhandled ExtractElementSize: %d", OpSize);
            }
          }
          else {
            LogMan::Msg::A("Can't handle extract from GPR yet");
          }
          break;
        }
        case IR::OP_VINSELEMENT: {
          auto Op = IROp->C<IR::IROp_VInsElement>();
          movapd(xmm15, GetSrc(Op->Header.Args[0].ID()));

          // Dst_d[Op->DestIdx] = Src2_d[Op->SrcIdx];

          // pextrq reg64/mem64, xmm, imm
          // pinsrq xmm, reg64/mem64, imm8
          switch (Op->ElementSize) {
          case 1: {
            pextrb(al, GetSrc(Op->Header.Args[1].ID()), Op->SrcIdx);
            pinsrb(xmm15, al, Op->DestIdx);
          break;
          }
          case 2: {
            pextrw(ax, GetSrc(Op->Header.Args[1].ID()), Op->SrcIdx);
            pinsrw(xmm15, ax, Op->DestIdx);
          break;
          }
          case 4: {
            pextrd(eax, GetSrc(Op->Header.Args[1].ID()), Op->SrcIdx);
            pinsrd(xmm15, eax, Op->DestIdx);
          break;
          }
          case 8: {
            pextrq(rax, GetSrc(Op->Header.Args[1].ID()), Op->SrcIdx);
            pinsrq(xmm15, rax, Op->DestIdx);
          break;
          }
          default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
          }

          movapd(GetDst(Node), xmm15);
          break;
        }
        case IR::OP_VADD: {
          auto Op = IROp->C<IR::IROp_VAdd>();
          switch (Op->ElementSize) {
          case 1: {
            vpaddb(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          case 2: {
            vpaddw(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          case 4: {
            vpaddd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          case 8: {
            vpaddq(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
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
            vpsubb(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          case 2: {
            vpsubw(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          case 4: {
            vpsubd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          case 8: {
            vpsubq(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
          }
          break;
        }
        case IR::OP_VAND: {
          auto Op = IROp->C<IR::IROp_VAnd>();
          vpand(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
        }
        case IR::OP_VXOR: {
          auto Op = IROp->C<IR::IROp_VXor>();
          vpxor(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
        }
        case IR::OP_VOR: {
          auto Op = IROp->C<IR::IROp_VOr>();
          vpor(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
        }
        case IR::OP_VFADD: {
          auto Op = IROp->C<IR::IROp_VFAdd>();
          if (Op->ElementSize == Op->RegisterSize) {
            // Scalar
            switch (Op->ElementSize) {
              case 4: {
                vaddss(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
              break;
              }
              case 8: {
                vaddsd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
              break;
              }
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
            }
          }
          else {
            // Vector
            switch (Op->ElementSize) {
              case 4: {
                vaddps(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
              break;
              }
              case 8: {
                vaddpd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
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
                vsubss(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
              break;
              }
              case 8: {
                vsubsd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
              break;
              }
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
            }
          }
          else {
            // Vector
            switch (Op->ElementSize) {
              case 4: {
                vsubps(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
              break;
              }
              case 8: {
                vsubpd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
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
                vmulss(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
              break;
              }
              case 8: {
                vmulsd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
              break;
              }
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
            }
          }
          else {
            // Vector
            switch (Op->ElementSize) {
              case 4: {
                vmulps(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
              break;
              }
              case 8: {
                vmulpd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
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
                vdivss(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
              break;
              }
              case 8: {
                vdivsd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
              break;
              }
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
            }
          }
          else {
            // Vector
            switch (Op->ElementSize) {
              case 4: {
                vdivps(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
              break;
              }
              case 8: {
                vdivpd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
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
                vrcpss(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[0].ID()));
              break;
              }
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
            }
          }
          else {
            // Vector
            switch (Op->ElementSize) {
              case 4: {
                vrcpps(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
              break;
              }
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
            }
          }
          break;
        }
        case IR::OP_VFSQRT: {
          auto Op = IROp->C<IR::IROp_VFSqrt>();
          if (Op->ElementSize == Op->RegisterSize) {
            // Scalar
            switch (Op->ElementSize) {
              case 4: {
                vsqrtss(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[0].ID()));
              break;
              }
              case 8: {
                vsqrtsd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[0].ID()));
              break;
              }
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
            }
          }
          else {
            // Vector
            switch (Op->ElementSize) {
              case 4: {
                vsqrtps(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
              break;
              }
              case 8: {
                vsqrtpd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
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
                vrsqrtss(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[0].ID()));
              break;
              }
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
            }
          }
          else {
            // Vector
            switch (Op->ElementSize) {
              case 4: {
                vrsqrtps(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
              break;
              }
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
            }
          }
          break;
        }
        case IR::OP_VBITCAST: {
          auto Op = IROp->C<IR::IROp_VBitcast>();
          movaps(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
        break;
        }
        case IR::OP_VCASTFROMGPR: {
          auto Op = IROp->C<IR::IROp_VCastFromGPR>();
          switch (Op->ElementSize) {
            case 1:
              movzx(rax, GetSrc<RA_8>(Op->Header.Args[0].ID()));
              vmovq(GetDst(Node), rax);
            break;
            case 2:
              movzx(rax, GetSrc<RA_16>(Op->Header.Args[0].ID()));
              vmovq(GetDst(Node), rax);
            break;
            case 4:
              vmovd(GetDst(Node), GetSrc<RA_32>(Op->Header.Args[0].ID()).cvt32());
            break;
            case 8:
              vmovq(GetDst(Node), GetSrc<RA_64>(Op->Header.Args[0].ID()).cvt64());
            break;
            default: LogMan::Msg::A("Unknown castGPR element size: %d", Op->ElementSize);
          }
          break;
        }
        case IR::OP_VCMPEQ: {
          auto Op = IROp->C<IR::IROp_VCMPEQ>();
          LogMan::Throw::A(Op->RegisterSize == 16, "Can't handle register size of: %d", Op->RegisterSize);

          switch (Op->ElementSize) {
          case 1:
            vpcmpeqb(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          case 2:
            vpcmpeqw(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          case 4:
            vpcmpeqd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          case 8:
            vpcmpeqq(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          default: LogMan::Msg::A("Unsupported elementSize: %d", Op->ElementSize);
          }
          break;
        }
        case IR::OP_VCMPGT: {
          auto Op = IROp->C<IR::IROp_VCMPGT>();
          LogMan::Throw::A(Op->RegisterSize == 16, "Can't handle register size of: %d", Op->RegisterSize);

          switch (Op->ElementSize) {
          case 1:
            vpcmpgtb(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          case 2:
            vpcmpgtw(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          case 4:
            vpcmpgtd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          case 8:
            vpcmpgtq(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          default: LogMan::Msg::A("Unsupported elementSize: %d", Op->ElementSize);
          }
          break;
        }
        case IR::OP_VZIP: {
          auto Op = IROp->C<IR::IROp_VZip>();
          movapd(xmm15, GetSrc(Op->Header.Args[0].ID()));

          switch (Op->ElementSize) {
          case 1: {
            punpcklbw(xmm15, GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          case 2: {
            punpcklwd(xmm15, GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          case 4: {
            punpckldq(xmm15, GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          case 8: {
            punpcklqdq(xmm15, GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
          }
          movapd(GetDst(Node), xmm15);
          break;
        }
        case IR::OP_VZIP2: {
          auto Op = IROp->C<IR::IROp_VZip2>();
          movapd(xmm15, GetSrc(Op->Header.Args[0].ID()));

          switch (Op->ElementSize) {
          case 1: {
            punpckhbw(xmm15, GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          case 2: {
            punpckhwd(xmm15, GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          case 4: {
            punpckhdq(xmm15, GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          case 8: {
            punpckhqdq(xmm15, GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
          }
          movapd(GetDst(Node), xmm15);
          break;
        }
        case IR::OP_VUSHLS: {
          auto Op = IROp->C<IR::IROp_VUShlS>();
          movapd(xmm15, GetSrc(Op->Header.Args[0].ID()));
          vmovq(xmm14, Reg64(GetSrc<RA_64>(Op->Header.Args[1].ID()).getIdx()));

          switch (Op->ElementSize) {
          case 2: {
            psllw(xmm15, xmm14);
          break;
          }
          case 4: {
            pslld(xmm15, xmm14);
          break;
          }
          case 8: {
            psllq(xmm15, xmm14);
          break;
          }
          default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
          }
          movapd(GetDst(Node), xmm15);

          break;
        }
        case IR::OP_VSLI: {
          auto Op = IROp->C<IR::IROp_VSLI>();
          movapd(xmm15, GetSrc(Op->Header.Args[0].ID()));
          pslldq(xmm15, Op->ByteShift);
          movapd(GetDst(Node), xmm15);
          break;
        }
        case IR::OP_VSRI: {
          auto Op = IROp->C<IR::IROp_VSRI>();
          movapd(xmm15, GetSrc(Op->Header.Args[0].ID()));
          psrldq(xmm15, Op->ByteShift);
          movapd(GetDst(Node), xmm15);
          break;
        }
        case IR::OP_VEXTR: {
          auto Op = IROp->C<IR::IROp_VExtr>();
          vpalignr(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()), Op->Index);
          break;
        }
        case IR::OP_VUMIN: {
          auto Op = IROp->C<IR::IROp_VUMin>();
          switch (Op->ElementSize) {
          case 1: {
            vpminub(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          case 2: {
            vpminuw(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          case 4: {
            vpminud(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
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
            vpminsb(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          case 2: {
            vpminsw(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          case 4: {
            vpminsd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
          }
          break;
        }
        case IR::OP_VUMAX: {
          auto Op = IROp->C<IR::IROp_VUMax>();
          switch (Op->ElementSize) {
          case 1: {
            vpmaxub(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          case 2: {
            vpmaxuw(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          case 4: {
            vpmaxud(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
          }
          break;
        }
        case IR::OP_VSMAX: {
          auto Op = IROp->C<IR::IROp_VSMax>();
          switch (Op->ElementSize) {
          case 1: {
            vpmaxsb(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          case 2: {
            vpmaxsw(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          case 4: {
            vpmaxsd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
          }
          break;
        }
        case IR::OP_SCVTF: {
          auto Op = IROp->C<IR::IROp_SCVTF>();
          if (Op->ElementSize == 8) {
            cvtsi2sd(GetDst(Node), GetSrc<RA_64>(Op->Header.Args[0].ID()));
          }
          else
            cvtsi2ss(GetDst(Node), GetSrc<RA_64>(Op->Header.Args[0].ID()));
          break;
        }
        case IR::OP_FCVTZS: {
          auto Op = IROp->C<IR::IROp_SCVTF>();
          if (Op->ElementSize == 8) {
            cvttsd2si(GetDst<RA_64>(Node), GetSrc(Op->Header.Args[0].ID()));
          }
          else
            cvttss2si(GetDst<RA_32>(Node), GetSrc(Op->Header.Args[0].ID()));
          break;
        }
        case IR::OP_FCVTF: {
          auto Op = IROp->C<IR::IROp_FCVTF>();
          uint16_t Conv = (Op->DstElementSize << 8) | Op->SrcElementSize;
          switch (Conv) {
            case 0x0804: { // Double <- Float
              cvtss2sd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
              break;
            }
            case 0x0408: { // Float <- Double
              cvtsd2ss(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
              break;
            }
            default: LogMan::Msg::A("Unknown FCVT sizes: 0x%x", Conv);
          }
          break;
        }
        case IR::OP_CAS: {
          auto Op = IROp->C<IR::IROp_CAS>();
          // Args[0]: Desired
          // Args[1]: Expected
          // Args[2]: Pointer
          // DataSrc = *Src1
          // if (DataSrc == Src3) { *Src1 == Src2; } Src2 = DataSrc
          // This will write to memory! Careful!
          // Third operand must be a calculated guest memory address
          //OrderedNode *CASResult = _CAS(Src3, Src2, Src1);
          uint64_t Memory = CTX->MemoryMapper.GetBaseOffset<uint64_t>(0);

          mov(rcx, Memory);
          add(rcx, GetSrc<RA_64>(Op->Header.Args[2].ID()));
          mov(rdx, GetSrc<RA_64>(Op->Header.Args[1].ID()));
          mov(rax, GetSrc<RA_64>(Op->Header.Args[0].ID()));

          // RCX now contains pointer
          // RAX contains our expected value
          // RDX contains our desired

          lock();

          switch (OpSize) {
          case 1: {
            cmpxchg(byte [rcx], dl);
            movzx(rax, al);
          break;
          }
          case 2: {
            cmpxchg(word [rcx], dx);
            movzx(rax, ax);
          break;
          }
          case 4: {
            cmpxchg(dword [rcx], edx);
          break;
          }
          case 8: {
            cmpxchg(qword [rcx], rdx);
          break;
          }
          default: LogMan::Msg::A("Unsupported: %d", OpSize);
          }

          // RAX now contains the result
          mov (GetDst<RA_64>(Node), rax);
          break;
        }
        case IR::OP_CYCLECOUNTER: {
          #ifdef DEBUG_CYCLES
          mov (GetDst<RA_64>(Node), 0);
          #else
          rdtsc();
          shl(rdx, 32);
          or(rax, rdx);
          mov (GetDst<RA_64>(Node), rax);
          #endif
          break;
        }
        case IR::OP_FINDLSB: {
          auto Op = IROp->C<IR::IROp_FindLSB>();
          tzcnt(rcx, GetSrc<RA_64>(Op->Header.Args[0].ID()));
          xor(rax, rax);
          cmp(GetSrc<RA_64>(Op->Header.Args[0].ID()), 1);
          sbb(rax, rax);
          or(rax, rcx);
          mov (GetDst<RA_64>(Node), rax);
          break;
        }
        case IR::OP_FINDMSB: {
          auto Op = IROp->C<IR::IROp_FindMSB>();
          mov(rax, OpSize * 8);
          lzcnt(rcx, GetSrc<RA_64>(Op->Header.Args[0].ID()));
          sub(rax, rcx);
          mov (GetDst<RA_64>(Node), rax);
          break;
        }
        case IR::OP_FINDTRAILINGZEROS: {
          auto Op = IROp->C<IR::IROp_FindTrailingZeros>();
          tzcnt(GetDst<RA_64>(Node), GetSrc<RA_64>(Op->Header.Args[0].ID()));
          break;
        }
        case IR::OP_DUMMY:
        case IR::OP_CODEBLOCK:
        case IR::OP_IRHEADER:
        case IR::OP_BEGINBLOCK:
        case IR::OP_ENDBLOCK:
        case IR::OP_EXITFUNCTION:
        case IR::OP_BREAK:
        case IR::OP_JUMP:
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

  void *Exit = getCurr<void*>();

  ready();

  DebugData->HostCodeSize = reinterpret_cast<uintptr_t>(Exit) - reinterpret_cast<uintptr_t>(Entry);

  return Entry;
}

void JITCore::CreateCustomDispatch() {
// Temp registers
// rax, rcx, rdx, rsi, r8, r9,
// r10, r11
//
// Callee Saved
// rbx, rbp, r12, r13, r14, r15
//
// 1St Argument: rdi <ThreadState>
// XMM:
// All temp
	void *Entry = getCurr<void*>();

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
  // Bunch of exit state stuff
  ready();
  // CustomDispatchGenerated = true;
}

FEXCore::CPU::CPUBackend *CreateJITCore(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread) {
  return new JITCore(ctx);
}
}
