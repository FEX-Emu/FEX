#include "Interface/Core/JIT/Arm64/JITClass.h"

namespace FEXCore::CPU {

using namespace vixl;
using namespace vixl::aarch64;
#define DEF_OP(x) void JITCore::Op_##x(FEXCore::IR::IROp_Header *IROp, uint32_t Node)

DEF_OP(EndBlock) {
  auto Op = IROp->C<IR::IROp_EndBlock>();
  if (Op->RIPIncrement) {
    ldr(TMP1, MemOperand(STATE, offsetof(FEXCore::Core::CPUState, rip)));
    add(TMP1, TMP1, Operand(Op->RIPIncrement));
    str(TMP1,  MemOperand(STATE, offsetof(FEXCore::Core::CPUState, rip)));
  }
}

DEF_OP(Fence) {
  auto Op = IROp->C<IR::IROp_Fence>();
  switch (Op->Fence) {
    case IR::Fence_Load.Val:
      dmb(FullSystem, BarrierReads);
      break;
    case IR::Fence_LoadStore.Val:
      dmb(FullSystem, BarrierAll);
      break;
    case IR::Fence_Store.Val:
      dmb(FullSystem, BarrierWrites);
      break;
    default: LogMan::Msg::A("Unknown Fence: %d", Op->Fence); break;
  }
}

DEF_OP(Break) {
  auto Op = IROp->C<IR::IROp_Break>();
  switch (Op->Reason) {
    case 0: // Hard fault
    case 5: // Guest ud2
      hlt(4);
      break;
    case 4: // HLT
    case 6: { // INT3
      LoadConstant(TMP1, 1);
      size_t offset = Op->Reason == 4 ?
          offsetof(FEXCore::Core::ThreadState, RunningEvents.ShouldStop) // HLT
        : offsetof(FEXCore::Core::ThreadState, RunningEvents.ShouldPause); // INT3

      add(TMP2, STATE, offset);

      stlrb(TMP1, MemOperand(TMP2));

      if (SpillSlots) {
        add(sp, sp, SpillSlots * 16);
      }
      ret();
      break;
    }
    default: LogMan::Msg::A("Unknown Break reason: %d", Op->Reason);
  }
}

#undef DEF_OP
void JITCore::RegisterMiscHandlers() {
#define REGISTER_OP(op, x) OpHandlers[FEXCore::IR::IROps::OP_##op] = &JITCore::Op_##x
  REGISTER_OP(DUMMY,      NoOp);
  REGISTER_OP(IRHEADER,   NoOp);
  REGISTER_OP(CODEBLOCK,  NoOp);
  REGISTER_OP(BEGINBLOCK, NoOp);
  REGISTER_OP(ENDBLOCK,   EndBlock);
  REGISTER_OP(FENCE,      Fence);
  REGISTER_OP(BREAK,      Break);
  REGISTER_OP(PHI,        NoOp);
  REGISTER_OP(PHIVALUE,   NoOp);
  REGISTER_OP(PRINT,      Unhandled);
#undef REGISTER_OP
}
}

