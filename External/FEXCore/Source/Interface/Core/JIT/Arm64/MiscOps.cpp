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

DEF_OP(GetRoundingMode) {
  auto Op = IROp->C<IR::IROp_GetRoundingMode>();
  auto Dst = GetReg<RA_64>(Node);
  mrs(Dst, FPCR);
  lsr(Dst, Dst,  22);

  // FTZ is already in the correct location
  // Rounding mode is different
  and_(TMP1, Dst, 0b11);

  cmp(TMP1, 1);
  LoadConstant(TMP3, IR::ROUND_MODE_POSITIVE_INFINITY);
  csel(TMP2, TMP3, xzr, vixl::aarch64::Condition::eq);

  cmp(TMP1, 2);
  LoadConstant(TMP3, IR::ROUND_MODE_NEGATIVE_INFINITY);
  csel(TMP2, TMP3, TMP2, vixl::aarch64::Condition::eq);

  cmp(TMP1, 3);
  LoadConstant(TMP3, IR::ROUND_MODE_TOWARDS_ZERO);
  csel(TMP2, TMP3, TMP2, vixl::aarch64::Condition::eq);

  orr(Dst, Dst, TMP2);

  bfi(Dst, TMP2, 0, 2);
}

DEF_OP(SetRoundingMode) {
  auto Op = IROp->C<IR::IROp_SetRoundingMode>();
  auto Src = GetReg<RA_32>(Op->Header.Args[0].ID());

  // Setup the rounding flags correctly
  and_(TMP1, Src, 0b11);

  cmp(TMP1, IR::ROUND_MODE_POSITIVE_INFINITY);
  LoadConstant(TMP3, 1);
  csel(TMP2, TMP3, xzr, vixl::aarch64::Condition::eq);

  cmp(TMP1, IR::ROUND_MODE_NEGATIVE_INFINITY);
  LoadConstant(TMP3, 2);
  csel(TMP2, TMP3, TMP2, vixl::aarch64::Condition::eq);

  cmp(TMP1, IR::ROUND_MODE_TOWARDS_ZERO);
  LoadConstant(TMP3, 3);
  csel(TMP2, TMP3, TMP2, vixl::aarch64::Condition::eq);

  mrs(TMP1, FPCR);

  // Insert the rounding flags
  bfi(TMP1, TMP2, 22, 2);

  // Insert the FTZ flag
  lsr(TMP2, Src, 2);
  bfi(TMP1, TMP2, 24, 1);

  // Now save the new FPCR
  msr(FPCR, TMP1);
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
  REGISTER_OP(GETROUNDINGMODE, GetRoundingMode);
  REGISTER_OP(SETROUNDINGMODE, SetRoundingMode);
#undef REGISTER_OP
}
}

