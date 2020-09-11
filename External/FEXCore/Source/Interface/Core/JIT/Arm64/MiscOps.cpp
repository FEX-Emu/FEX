#include "Interface/Core/JIT/Arm64/JITClass.h"

namespace FEXCore::CPU {

using namespace vixl;
using namespace vixl::aarch64;
#define DEF_OP(x) void JITCore::Op_##x(FEXCore::IR::IROp_Header *IROp, uint32_t Node)

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
    case 4: { // HLT
      // Time to quit
      // Set our stack to the starting stack location
      ldr(TMP1, MemOperand(STATE, offsetof(FEXCore::Core::ThreadState, ReturningStackLocation)));
      add(sp, TMP1, 0);

      // Now we need to jump to the thread stop handler
      LoadConstant(TMP1, ThreadStopHandlerAddress);
      br(TMP1);
      break;
    }
    case 6: { // INT3
      ldp(TMP1, lr, MemOperand(sp, 16, PostIndex));
      add(sp, TMP1, 0); // Move that supports SP

      LoadConstant(TMP1, ThreadPauseHandlerAddress);
      br(TMP1);
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
  REGISTER_OP(ENDBLOCK,   NoOp);
  REGISTER_OP(FENCE,      Fence);
  REGISTER_OP(BREAK,      Break);
  REGISTER_OP(PHI,        NoOp);
  REGISTER_OP(PHIVALUE,   NoOp);
  REGISTER_OP(PRINT,      Unhandled);
#undef REGISTER_OP
}
}

