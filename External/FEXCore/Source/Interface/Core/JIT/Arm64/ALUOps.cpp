/*
$info$
tags: backend|arm64
$end_info$
*/

#include "Interface/Core/JIT/Arm64/JITClass.h"
#include "Interface/IR/Passes/RegisterAllocationPass.h"

namespace FEXCore::CPU {

#define GRD(Node) (IROp->Size <= 4 ? GetDst<RA_32>(Node) : GetDst<RA_64>(Node))
#define GRS(Node) (IROp->Size <= 4 ? GetReg<RA_32>(Node) : GetReg<RA_64>(Node))

using namespace vixl;
using namespace vixl::aarch64;
#define DEF_OP(x) void Arm64JITCore::Op_##x(IR::IROp_Header const *IROp, IR::NodeID Node)
DEF_OP(TruncElementPair) {
  auto Op = IROp->C<IR::IROp_TruncElementPair>();

  switch (IROp->Size) {
    case 4: {
      auto Dst = GetSrcPair<RA_32>(Node);
      auto Src = GetSrcPair<RA_32>(Op->Pair.ID());
      mov(Dst.first, Src.first);
      mov(Dst.second, Src.second);
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unhandled Truncation size: {}", IROp->Size);
      break;
  }
}

DEF_OP(Constant) {
  auto Op = IROp->C<IR::IROp_Constant>();
  auto Dst = GetReg<RA_64>(Node);
  LoadConstant(Dst, Op->Constant);
}

DEF_OP(EntrypointOffset) {
  auto Op = IROp->C<IR::IROp_EntrypointOffset>();

  auto Constant = Entry + Op->Offset;
  auto Dst = GetReg<RA_64>(Node);
  uint64_t Mask = ~0ULL;
  uint8_t OpSize = IROp->Size;
  if (OpSize == 4) {
    Mask = 0xFFFF'FFFFULL;
  }

  LoadConstant(Dst, Constant & Mask);
}

DEF_OP(InlineConstant) {
  //nop
}

DEF_OP(InlineEntrypointOffset) {
  //nop
}

DEF_OP(CycleCounter) {
#ifdef DEBUG_CYCLES
  movz(GetReg<RA_64>(Node), 0);
#else
  mrs(GetReg<RA_64>(Node), CNTVCT_EL0);
#endif
}

DEF_OP(Add) {
  auto Op = IROp->C<IR::IROp_Add>();
  const uint8_t OpSize = IROp->Size;

  uint64_t Const;
  if (IsInlineConstant(Op->Src2, &Const)) {
    switch (OpSize) {
      case 4:
        add(GetReg<RA_32>(Node), GetReg<RA_32>(Op->Src1.ID()), Const);
        break;
      case 8:
        add(GetReg<RA_64>(Node), GetReg<RA_64>(Op->Src1.ID()), Const);
        break;
      default: LOGMAN_MSG_A_FMT("Unsupported Add size: {}", OpSize);
    }
  } else {
    switch (OpSize) {
      case 4:
        add(GetReg<RA_32>(Node), GetReg<RA_32>(Op->Src1.ID()), GetReg<RA_32>(Op->Src2.ID()));
        break;
      case 8:
        add(GetReg<RA_64>(Node), GetReg<RA_64>(Op->Src1.ID()), GetReg<RA_64>(Op->Src2.ID()));
        break;
      default: LOGMAN_MSG_A_FMT("Unsupported Add size: {}", OpSize);
    }
  }
}

DEF_OP(Sub) {
  auto Op = IROp->C<IR::IROp_Sub>();
  const uint8_t OpSize = IROp->Size;

  uint64_t Const;
  if (IsInlineConstant(Op->Src2, &Const)) {
    switch (OpSize) {
      case 4:
      case 8:
        sub(GRS(Node), GRS(Op->Src1.ID()), Const);
        break;
      default: LOGMAN_MSG_A_FMT("Unsupported Sub size: {}", OpSize);
    }
  } else {
    switch (OpSize) {
      case 4:
        sub(GetReg<RA_32>(Node), GetReg<RA_32>(Op->Src1.ID()), GetReg<RA_32>(Op->Src2.ID()));
        break;
      case 8:
        sub(GetReg<RA_64>(Node), GetReg<RA_64>(Op->Src1.ID()), GetReg<RA_64>(Op->Src2.ID()));
        break;
      default: LOGMAN_MSG_A_FMT("Unsupported Sub size: {}", OpSize);
    }
  }

}

DEF_OP(Neg) {
  auto Op = IROp->C<IR::IROp_Neg>();
  const uint8_t OpSize = IROp->Size;
  switch (OpSize) {
    case 4:
      neg(GetReg<RA_32>(Node), GetReg<RA_32>(Op->Src.ID()));
      break;
    case 8:
      neg(GetReg<RA_64>(Node), GetReg<RA_64>(Op->Src.ID()));
      break;
    default: LOGMAN_MSG_A_FMT("Unsupported Neg size: {}", OpSize);
  }
}

DEF_OP(Mul) {
  auto Op = IROp->C<IR::IROp_Mul>();
  const uint8_t OpSize = IROp->Size;
  auto Dst = GetReg<RA_64>(Node);

  switch (OpSize) {
    case 4:
      mul(Dst.W(), GetReg<RA_32>(Op->Src1.ID()), GetReg<RA_32>(Op->Src2.ID()));
    break;
    case 8:
      mul(Dst, GetReg<RA_64>(Op->Src1.ID()), GetReg<RA_64>(Op->Src2.ID()));
    break;
    default: LOGMAN_MSG_A_FMT("Unknown Mul size: {}", OpSize);
  }
}

DEF_OP(UMul) {
  auto Op = IROp->C<IR::IROp_UMul>();
  const uint8_t OpSize = IROp->Size;
  auto Dst = GetReg<RA_64>(Node);

  switch (OpSize) {
    case 4:
      mul(Dst.W(), GetReg<RA_32>(Op->Src1.ID()), GetReg<RA_32>(Op->Src2.ID()));
    break;
    case 8:
      mul(Dst, GetReg<RA_64>(Op->Src1.ID()), GetReg<RA_64>(Op->Src2.ID()));
    break;
    default: LOGMAN_MSG_A_FMT("Unknown UMul size: {}", OpSize);
  }
}

DEF_OP(Div) {
  auto Op = IROp->C<IR::IROp_Div>();

  // Each source is OpSize in size
  // So you can have up to a 128bit divide from x86-64
  const uint8_t OpSize = IROp->Size;
  switch (OpSize) {
    case 1: {
      auto Dividend = GetReg<RA_32>(Op->Src1.ID());
      auto Divisor = GetReg<RA_32>(Op->Src2.ID());
      sxtb(w2, Dividend);
      sxtb(w3, Divisor);

      sdiv(GetReg<RA_32>(Node), w2, w3);
    break;
    }
    case 2: {
      auto Dividend = GetReg<RA_32>(Op->Src1.ID());
      auto Divisor = GetReg<RA_32>(Op->Src2.ID());
      sxth(w2, Dividend);
      sxth(w3, Divisor);

      sdiv(GetReg<RA_32>(Node), w2, w3);
    break;
    }
    case 4: {
      sdiv(GetReg<RA_32>(Node), GetReg<RA_32>(Op->Src1.ID()), GetReg<RA_32>(Op->Src2.ID()));
    break;
    }
    case 8: {
      sdiv(GetReg<RA_64>(Node), GetReg<RA_64>(Op->Src1.ID()), GetReg<RA_64>(Op->Src2.ID()));
    break;
    }
    default: {
      LOGMAN_MSG_A_FMT("Unknown DIV Size: {}", OpSize);
    break;
    }
  }
}

DEF_OP(UDiv) {
  auto Op = IROp->C<IR::IROp_UDiv>();

  // Each source is OpSize in size
  // So you can have up to a 128bit divide from x86-64
  const uint8_t OpSize = IROp->Size;
  switch (OpSize) {
    case 1: {
      udiv(GetReg<RA_32>(Node), GetReg<RA_32>(Op->Src1.ID()), GetReg<RA_32>(Op->Src2.ID()));
      break;
    }
    case 2: {
      udiv(GetReg<RA_32>(Node), GetReg<RA_32>(Op->Src1.ID()), GetReg<RA_32>(Op->Src2.ID()));
      break;
    }
    case 4: {
      udiv(GetReg<RA_32>(Node), GetReg<RA_32>(Op->Src1.ID()), GetReg<RA_32>(Op->Src2.ID()));
      break;
    }
    case 8: {
      udiv(GetReg<RA_64>(Node), GetReg<RA_64>(Op->Src1.ID()), GetReg<RA_64>(Op->Src2.ID()));
      break;
    }
    default: {
      LOGMAN_MSG_A_FMT("Unknown UDIV Size: {}", OpSize);
      break;
    }
  }
}

DEF_OP(Rem) {
  auto Op = IROp->C<IR::IROp_Rem>();
  const uint8_t OpSize = IROp->Size;
  // Each source is OpSize in size
  // So you can have up to a 128bit divide from x86-64
  switch (OpSize) {
    case 1: {
      auto Dividend = GetReg<RA_32>(Op->Src1.ID());
      auto Divisor = GetReg<RA_32>(Op->Src2.ID());
      sxtb(w2, Dividend);
      sxtb(w3, Divisor);

      sdiv(TMP1.W(), w2, w3);
      msub(GetReg<RA_32>(Node), TMP1.W(), w3, w2);
    break;
    }
    case 2: {
      auto Dividend = GetReg<RA_32>(Op->Src1.ID());
      auto Divisor = GetReg<RA_32>(Op->Src2.ID());

      sxth(w2, Dividend);
      sxth(w3, Divisor);

      sdiv(TMP1.W(), w2, w3);
      msub(GetReg<RA_32>(Node), TMP1.W(), w3, w2);
    break;
    }
    case 4: {
      auto Dividend = GetReg<RA_32>(Op->Src1.ID());
      auto Divisor = GetReg<RA_32>(Op->Src2.ID());

      sdiv(TMP1.W(), Dividend, Divisor);
      msub(GetReg<RA_32>(Node), TMP1, Divisor, Dividend);
    break;
    }
    case 8: {
      auto Dividend = GetReg<RA_64>(Op->Src1.ID());
      auto Divisor = GetReg<RA_64>(Op->Src2.ID());

      sdiv(TMP1, Dividend, Divisor);
      msub(GetReg<RA_64>(Node), TMP1, Divisor, Dividend);
    break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown REM Size: {}", OpSize); break;
  }
}

DEF_OP(URem) {
  auto Op = IROp->C<IR::IROp_URem>();
  const uint8_t OpSize = IROp->Size;
  // Each source is OpSize in size
  // So you can have up to a 128bit divide from x86-64
  switch (OpSize) {
    case 1: {
      auto Dividend = GetReg<RA_32>(Op->Src1.ID());
      auto Divisor = GetReg<RA_32>(Op->Src2.ID());

      udiv(TMP1.W(), Dividend, Divisor);
      msub(GetReg<RA_32>(Node), TMP1, Divisor, Dividend);
    break;
    }
    case 2: {
      auto Dividend = GetReg<RA_32>(Op->Src1.ID());
      auto Divisor = GetReg<RA_32>(Op->Src2.ID());

      udiv(TMP1.W(), Dividend, Divisor);
      msub(GetReg<RA_32>(Node), TMP1, Divisor, Dividend);
    break;
    }
    case 4: {
      auto Dividend = GetReg<RA_32>(Op->Src1.ID());
      auto Divisor = GetReg<RA_32>(Op->Src2.ID());

      udiv(TMP1.W(), Dividend, Divisor);
      msub(GetReg<RA_32>(Node), TMP1, Divisor, Dividend);
    break;
    }
    case 8: {
      auto Dividend = GetReg<RA_64>(Op->Src1.ID());
      auto Divisor = GetReg<RA_64>(Op->Src2.ID());

      udiv(TMP1, Dividend, Divisor);
      msub(GetReg<RA_64>(Node), TMP1, Divisor, Dividend);
    break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown UREM Size: {}", OpSize); break;
  }
}

DEF_OP(MulH) {
  auto Op = IROp->C<IR::IROp_MulH>();
  const uint8_t OpSize = IROp->Size;
  switch (OpSize) {
    case 4:
      sxtw(TMP1, GetReg<RA_64>(Op->Src1.ID()));
      sxtw(TMP2, GetReg<RA_64>(Op->Src2.ID()));
      mul(TMP1, TMP1, TMP2);
      ubfx(GetReg<RA_64>(Node), TMP1, 32, 32);
    break;
    case 8:
      smulh(GetReg<RA_64>(Node), GetReg<RA_64>(Op->Src1.ID()), GetReg<RA_64>(Op->Src2.ID()));
    break;
    default: LOGMAN_MSG_A_FMT("Unknown Sext size: {}", OpSize);
  }
}

DEF_OP(UMulH) {
  auto Op = IROp->C<IR::IROp_UMulH>();
  const uint8_t OpSize = IROp->Size;
  switch (OpSize) {
    case 4:
      uxtw(TMP1, GetReg<RA_64>(Op->Src1.ID()));
      uxtw(TMP2, GetReg<RA_64>(Op->Src2.ID()));
      mul(TMP1, TMP1, TMP2);
      ubfx(GetReg<RA_64>(Node), TMP1, 32, 32);
    break;
    case 8:
      umulh(GetReg<RA_64>(Node), GetReg<RA_64>(Op->Src1.ID()), GetReg<RA_64>(Op->Src2.ID()));
    break;
    default: LOGMAN_MSG_A_FMT("Unknown Sext size: {}", OpSize);
  }
}

DEF_OP(Or) {
  auto Op = IROp->C<IR::IROp_Or>();
  uint64_t Const;
  if (IsInlineConstant(Op->Src2, &Const)) {
    orr(GRS(Node), GRS(Op->Src1.ID()), Const);
  } else {
    orr(GRS(Node), GRS(Op->Src1.ID()), GRS(Op->Src2.ID()));
  }
}

DEF_OP(And) {
  auto Op = IROp->C<IR::IROp_And>();
  uint64_t Const;
  if (IsInlineConstant(Op->Src2, &Const)) {
    and_(GRS(Node), GRS(Op->Src1.ID()), Const);
  } else {
    and_(GRS(Node), GRS(Op->Src1.ID()), GRS(Op->Src2.ID()));
  }
}

DEF_OP(Andn) {
  auto Op = IROp->C<IR::IROp_Andn>();
  const auto& Lhs = Op->Src1;
  const auto& Rhs = Op->Src2;
  uint64_t Const{};

  if (IsInlineConstant(Rhs, &Const)) {
    bic(GRS(Node), GRS(Lhs.ID()), Const);
  } else {
    bic(GRS(Node), GRS(Lhs.ID()), GRS(Rhs.ID()));
  }
}

DEF_OP(Xor) {
  auto Op = IROp->C<IR::IROp_Xor>();
  uint64_t Const;
  if (IsInlineConstant(Op->Src2, &Const)) {
    eor(GRS(Node), GRS(Op->Src1.ID()), Const);
  } else {
    eor(GRS(Node), GRS(Op->Src1.ID()), GRS(Op->Src2.ID()));
  }
}

DEF_OP(Lshl) {
  auto Op = IROp->C<IR::IROp_Lshl>();
  uint64_t Const;
  if (IsInlineConstant(Op->Src2, &Const)) {
    lsl(GRS(Node), GRS(Op->Src1.ID()), (unsigned int)Const);
  } else {
    lslv(GRS(Node), GRS(Op->Src1.ID()), GRS(Op->Src2.ID()));
  }
}

DEF_OP(Lshr) {
  auto Op = IROp->C<IR::IROp_Lshr>();
  uint64_t Const;
  if (IsInlineConstant(Op->Src2, &Const)) {
    lsr(GRS(Node), GRS(Op->Src1.ID()), (unsigned int)Const);
  } else {
    lsrv(GRS(Node), GRS(Op->Src1.ID()), GRS(Op->Src2.ID()));
  }
}

DEF_OP(Ashr) {
  auto Op = IROp->C<IR::IROp_Ashr>();
  const uint8_t OpSize = IROp->Size;

  uint64_t Const;
  if (IsInlineConstant(Op->Src2, &Const)) {
    if (OpSize >= 4) {
      asr(GRS(Node), GRS(Op->Src1.ID()), (unsigned int)Const);
    }
    else {
      sbfx(TMP1.X(), GetReg<RA_64>(Op->Src1.ID()), 0, OpSize * 8);
      asr(GetReg<RA_64>(Node), TMP1.X(), (unsigned int)Const);
      ubfx(GetReg<RA_64>(Node),GetReg<RA_64>(Node), 0, OpSize * 8);
    }
  } else {
    if (OpSize >= 4) {
      asrv(GRS(Node), GRS(Op->Src1.ID()), GRS(Op->Src2.ID()));
    }
    else {
      sbfx(TMP1.X(), GetReg<RA_64>(Op->Src1.ID()), 0, OpSize * 8);
      asrv(GetReg<RA_64>(Node), TMP1.X(), GetReg<RA_64>(Op->Src2.ID()));
      ubfx(GetReg<RA_64>(Node),GetReg<RA_64>(Node), 0, OpSize * 8);
    }
  }
}

DEF_OP(Ror) {
  auto Op = IROp->C<IR::IROp_Ror>();
  const uint8_t OpSize = IROp->Size;

  uint64_t Const;
  if (IsInlineConstant(Op->Src2, &Const)) {
    switch (OpSize) {
      case 4: {
        ror(GetReg<RA_32>(Node), GetReg<RA_32>(Op->Src1.ID()), (unsigned int)Const);
      break;
      }
      case 8: {
        ror(GetReg<RA_64>(Node), GetReg<RA_64>(Op->Src1.ID()), (unsigned int)Const);
      break;
      }

      default: LOGMAN_MSG_A_FMT("Unhandled ROR size: {}", OpSize);
    }
  } else {
    switch (OpSize) {
      case 4: {
        rorv(GetReg<RA_32>(Node), GetReg<RA_32>(Op->Src1.ID()), GetReg<RA_32>(Op->Src2.ID()));
      break;
      }
      case 8: {
        rorv(GetReg<RA_64>(Node), GetReg<RA_64>(Op->Src1.ID()), GetReg<RA_64>(Op->Src2.ID()));
      break;
      }

      default: LOGMAN_MSG_A_FMT("Unhandled ROR size: {}", OpSize);
    }
  }
}

DEF_OP(Extr) {
  auto Op = IROp->C<IR::IROp_Extr>();
  const uint8_t OpSize = IROp->Size;

  switch (OpSize) {
    case 4: {
      extr(GetReg<RA_32>(Node), GetReg<RA_32>(Op->Upper.ID()), GetReg<RA_32>(Op->Lower.ID()), Op->LSB);
    break;
    }
    case 8: {
      extr(GetReg<RA_64>(Node), GetReg<RA_64>(Op->Upper.ID()), GetReg<RA_64>(Op->Lower.ID()), Op->LSB);
    break;
    }

    default: LOGMAN_MSG_A_FMT("Unhandled EXTR size: {}", OpSize);
  }
}

DEF_OP(PDep) {
  auto Op = IROp->C<IR::IROp_PExt>();
  const auto OpSize = IROp->Size;

  const Register Input = GRS(Op->Input.ID());
  const Register Mask = GRS(Op->Mask.ID());
  const Register Dest = GRS(Node);

  const Register ShiftedBitReg = OpSize <= 4 ? TMP1.W() : TMP1;
  const Register BitReg        = OpSize <= 4 ? TMP2.W() : TMP2;
  const Register SubMaskReg    = OpSize <= 4 ? TMP3.W() : TMP3;
  const Register IndexReg      = OpSize <= 4 ? TMP4.W() : TMP4;
  const Register SizedZero     = OpSize <= 4 ? Register{wzr} : Register{xzr};

  const Register InputReg = OpSize <= 4 ? SRA64[0].W() : SRA64[0];
  const Register MaskReg  = OpSize <= 4 ? SRA64[1].W() : SRA64[1];
  const Register DestReg  = OpSize <= 4 ? SRA64[2].W() : SRA64[2];
  const auto SpillCode    = 1U << InputReg.GetCode() |
                            1U << MaskReg.GetCode() |
                            1U << DestReg.GetCode();

  aarch64::Label EarlyExit;
  aarch64::Label NextBit;
  aarch64::Label Done;

  cbz(Mask, &EarlyExit);
  mov(IndexReg, SizedZero);

  // We sadly need to spill regs for this for the time being
  // TODO: Remove when scratch registers can be allocated
  //       explicitly.
  SpillStaticRegs(false, SpillCode);
  mov(InputReg, Input);
  mov(MaskReg, Mask);
  mov(DestReg, SizedZero);

  // Main loop
  bind(&NextBit);
  rbit(ShiftedBitReg, MaskReg);
  clz(ShiftedBitReg, ShiftedBitReg);
  lsrv(BitReg, InputReg, IndexReg);
  and_(BitReg, BitReg, 1);
  sub(SubMaskReg, MaskReg, 1);
  add(IndexReg, IndexReg, 1);
  ands(MaskReg, MaskReg, SubMaskReg);
  lslv(ShiftedBitReg, BitReg, ShiftedBitReg);
  orr(DestReg, DestReg, ShiftedBitReg);
  b(&NextBit, Condition::ne);
  // Store result in a temp so it doesn't get clobbered.
  // and restore it after the re-fill below.
  mov(IndexReg, DestReg);
  // Restore our registers before leaving
  // TODO: Also remove along with above TODO.
  FillStaticRegs(false, SpillCode);
  mov(Dest, IndexReg);
  b(&Done);

  // Early exit
  bind(&EarlyExit);
  mov(Dest, SizedZero);

  // All done with nothing to do.
  bind(&Done);
}

DEF_OP(PExt) {
  auto Op = IROp->C<IR::IROp_PExt>();
  const auto OpSize = IROp->Size;

  const Register Input = GRS(Op->Input.ID());
  const Register Mask = GRS(Op->Mask.ID());
  const Register Dest = GRS(Node);

  const Register MaskReg    = OpSize <= 4 ? TMP1.W() : TMP1;
  const Register BitReg     = OpSize <= 4 ? TMP2.W() : TMP2;
  const Register SubMaskReg = OpSize <= 4 ? TMP3.W() : TMP3;
  const Register Offset     = OpSize <= 4 ? TMP4.W() : TMP4;
  const Register SizedZero  = OpSize <= 4 ? Register{wzr} : Register{xzr};

  aarch64::Label EarlyExit;
  aarch64::Label NextBit;
  aarch64::Label Done;

  cbz(Mask, &EarlyExit);
  mov(MaskReg, Mask);
  mov(Offset, SizedZero);

  // We sadly need to spill a reg for this for the time being
  // TODO: Remove when scratch registers can be allocated
  //       explicitly.
  SpillStaticRegs(false, 1U << Mask.GetCode());
  mov(Mask, SizedZero);

  // Main loop
  bind(&NextBit);
  rbit(BitReg, MaskReg);
  clz(BitReg, BitReg);
  sub(SubMaskReg, MaskReg, 1);
  ands(MaskReg, SubMaskReg, MaskReg);
  lsrv(BitReg, Input, BitReg);
  and_(BitReg, BitReg, 1);
  lslv(BitReg, BitReg, Offset);
  add(Offset, Offset, 1);
  orr(Mask, BitReg, Mask);
  b(&NextBit, Condition::ne);
  mov(Dest, Mask);
  // Restore our mask register before leaving
  // TODO: Also remove along with above TODO.
  FillStaticRegs(false, 1U << Mask.GetCode());
  b(&Done);

  // Early exit
  bind(&EarlyExit);
  mov(Dest, SizedZero);

  // All done with nothing to do.
  bind(&Done);
}

DEF_OP(LDiv) {
  auto Op = IROp->C<IR::IROp_LDiv>();
  const uint8_t OpSize = IROp->Size;

  // Each source is OpSize in size
  // So you can have up to a 128bit divide from x86-64
  switch (OpSize) {
    case 2: {
      uxth(TMP1.W(), GetReg<RA_32>(Op->Lower.ID()));
      bfi(TMP1.W(), GetReg<RA_32>(Op->Upper.ID()), 16, 16);
      sxth(TMP2.W(), GetReg<RA_32>(Op->Divisor.ID()));
      sdiv(GetReg<RA_32>(Node), TMP1.W(), TMP2.W());
    break;
    }
    case 4: {
      mov(TMP1, GetReg<RA_64>(Op->Lower.ID()));
      bfi(TMP1, GetReg<RA_64>(Op->Upper.ID()), 32, 32);
      sxtw(TMP2, GetReg<RA_32>(Op->Divisor.ID()));
      sdiv(GetReg<RA_64>(Node), TMP1, TMP2);
    break;
    }
    case 8: {
      auto Upper64Bit = GetReg<RA_64>(Op->Upper.ID());
      auto Lower64Bit = GetReg<RA_64>(Op->Lower.ID());
      auto Divisor = GetReg<RA_64>(Op->Divisor.ID());
      Label Only64Bit{};
      Label LongDIVRet{};

      // Check if the upper bits match the top bit of the lower 64-bits
      // Sign extend the top bit of lower bits
      sbfx(TMP1, Lower64Bit, 63, 1);
      eor(TMP1, TMP1, Upper64Bit);

      // If the sign bit matches then the result is zero
      cbz(TMP1, &Only64Bit);

      // Long divide
      {
        mov(x0, Upper64Bit);
        mov(x1, Lower64Bit);
        mov(x2, Divisor);

        ldr(x3, MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.AArch64.LDIVHandler)));
        blr(x3);
        // Move result to its destination register
        mov(GetReg<RA_64>(Node), x0);

        // Skip 64-bit path
        b(&LongDIVRet);
      }

      bind(&Only64Bit);
      // 64-Bit only
      {
        sdiv(GetReg<RA_64>(Node), Lower64Bit, Divisor);
      }

      bind(&LongDIVRet);
    break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unknown LDIV Size: {}", OpSize);
      break;
  }
}

DEF_OP(LUDiv) {
  auto Op = IROp->C<IR::IROp_LUDiv>();
  const uint8_t OpSize = IROp->Size;

  // Each source is OpSize in size
  // So you can have up to a 128bit divide from x86-64=
  switch (OpSize) {
    case 2: {
      uxth(TMP1.W(), GetReg<RA_32>(Op->Lower.ID()));
      bfi(TMP1.W(), GetReg<RA_32>(Op->Upper.ID()), 16, 16);
      udiv(GetReg<RA_32>(Node), TMP1.W(), GetReg<RA_32>(Op->Divisor.ID()));
    break;
    }
    case 4: {
      mov(TMP1, GetReg<RA_64>(Op->Lower.ID()));
      bfi(TMP1, GetReg<RA_64>(Op->Upper.ID()), 32, 32);
      udiv(GetReg<RA_64>(Node), TMP1, GetReg<RA_64>(Op->Divisor.ID()));
    break;
    }
    case 8: {
      auto Upper64Bit = GetReg<RA_64>(Op->Upper.ID());
      auto Lower64Bit = GetReg<RA_64>(Op->Lower.ID());
      auto Divisor = GetReg<RA_64>(Op->Divisor.ID());
      Label Only64Bit{};
      Label LongDIVRet{};

      // Check the upper bits for zero
      // If the upper bits are zero then we can do a 64-bit divide
      cbz(Upper64Bit, &Only64Bit);

      // Long divide
      {
        mov(x0, Upper64Bit);
        mov(x1, Lower64Bit);
        mov(x2, Divisor);

        ldr(x3, MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.AArch64.LUDIVHandler)));
        blr(x3);
        // Move result to its destination register
        mov(GetReg<RA_64>(Node), x0);

        // Skip 64-bit path
        b(&LongDIVRet);
      }

      bind(&Only64Bit);
      // 64-Bit only
      {
        udiv(GetReg<RA_64>(Node), Lower64Bit, Divisor);
      }

      bind(&LongDIVRet);
    break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unknown LUDIV Size: {}", OpSize);
      break;
  }
}

DEF_OP(LRem) {
  auto Op = IROp->C<IR::IROp_LRem>();
  const uint8_t OpSize = IROp->Size;

  // Each source is OpSize in size
  // So you can have up to a 128bit divide from x86-64
  switch (OpSize) {
    case 2: {
      auto Divisor = GetReg<RA_32>(Op->Divisor.ID());

      uxth(TMP1.W(), GetReg<RA_32>(Op->Lower.ID()));
      bfi(TMP1.W(), GetReg<RA_32>(Op->Upper.ID()), 16, 16);
      sxth(w3, Divisor);
      sdiv(TMP2.W(), TMP1.W(), w3);

      msub(GetReg<RA_32>(Node), TMP2.W(), w3, TMP1.W());
    break;
    }
    case 4: {
      auto Divisor = GetReg<RA_64>(Op->Divisor.ID());

      mov(TMP1, GetReg<RA_64>(Op->Lower.ID()));
      bfi(TMP1, GetReg<RA_64>(Op->Upper.ID()), 32, 32);
      sxtw(x3, Divisor);
      sdiv(TMP2, TMP1, x3);

      msub(GetReg<RA_32>(Node), TMP2.W(), w3, TMP1.W());
    break;
    }
    case 8: {
      auto Upper64Bit = GetReg<RA_64>(Op->Upper.ID());
      auto Lower64Bit = GetReg<RA_64>(Op->Lower.ID());
      auto Divisor = GetReg<RA_64>(Op->Divisor.ID());
      Label Only64Bit{};
      Label LongDIVRet{};

      // Check if the upper bits match the top bit of the lower 64-bits
      // Sign extend the top bit of lower bits
      sbfx(TMP1, Lower64Bit, 63, 1);
      eor(TMP1, TMP1, Upper64Bit);

      // If the sign bit matches then the result is zero
      cbz(TMP1, &Only64Bit);

      // Long divide
      {
        mov(x0, Upper64Bit);
        mov(x1, Lower64Bit);
        mov(x2, Divisor);

        ldr(x3, MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.AArch64.LREMHandler)));
        blr(x3);
        // Move result to its destination register
        mov(GetReg<RA_64>(Node), x0);

        // Skip 64-bit path
        b(&LongDIVRet);
      }

      bind(&Only64Bit);
      // 64-Bit only
      {
        sdiv(TMP1, Lower64Bit, Divisor);
        msub(GetReg<RA_64>(Node), TMP1, Divisor, Lower64Bit);
      }
      bind(&LongDIVRet);
    break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unknown LREM Size: {}", OpSize);
      break;
  }
}

DEF_OP(LURem) {
  auto Op = IROp->C<IR::IROp_LURem>();
  const uint8_t OpSize = IROp->Size;

  // Each source is OpSize in size
  // So you can have up to a 128bit divide from x86-64
  switch (OpSize) {
    case 2: {
      auto Divisor = GetReg<RA_32>(Op->Divisor.ID());

      uxth(TMP1.W(), GetReg<RA_32>(Op->Lower.ID()));
      bfi(TMP1.W(), GetReg<RA_32>(Op->Upper.ID()), 16, 16);
      udiv(TMP2.W(), TMP1.W(), Divisor);
      msub(GetReg<RA_32>(Node), TMP2.W(), Divisor, TMP1.W());
    break;
    }
    case 4: {
      auto Divisor = GetReg<RA_64>(Op->Divisor.ID());

      mov(TMP1, GetReg<RA_64>(Op->Lower.ID()));
      bfi(TMP1, GetReg<RA_64>(Op->Upper.ID()), 32, 32);
      udiv(TMP2, TMP1, Divisor);

      msub(GetReg<RA_64>(Node), TMP2, Divisor, TMP1);
    break;
    }
    case 8: {
      auto Upper64Bit = GetReg<RA_64>(Op->Upper.ID());
      auto Lower64Bit = GetReg<RA_64>(Op->Lower.ID());
      auto Divisor = GetReg<RA_64>(Op->Divisor.ID());
      Label Only64Bit{};
      Label LongDIVRet{};

      // Check the upper bits for zero
      // If the upper bits are zero then we can do a 64-bit divide
      cbz(Upper64Bit, &Only64Bit);

      // Long divide
      {
        mov(x0, Upper64Bit);
        mov(x1, Lower64Bit);
        mov(x2, Divisor);

        ldr(x3, MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.AArch64.LUREMHandler)));
        blr(x3);
        // Move result to its destination register
        mov(GetReg<RA_64>(Node), x0);

        // Skip 64-bit path
        b(&LongDIVRet);
      }

      bind(&Only64Bit);
      // 64-Bit only
      {
        udiv(TMP1, Lower64Bit, Divisor);
        msub(GetReg<RA_64>(Node), TMP1, Divisor, Lower64Bit);
      }

      bind(&LongDIVRet);
    break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unknown LUREM Size: {}", OpSize);
      break;
  }
}

DEF_OP(Not) {
  auto Op = IROp->C<IR::IROp_Not>();
  const uint8_t OpSize = IROp->Size;

  switch (OpSize) {
    case 4:
      mvn(GetReg<RA_32>(Node), GetReg<RA_32>(Op->Src.ID()));
      break;
    case 8:
      mvn(GetReg<RA_64>(Node), GetReg<RA_64>(Op->Src.ID()));
      break;
    default: LOGMAN_MSG_A_FMT("Unsupported Not size: {}", OpSize);
  }
}

DEF_OP(Popcount) {
  auto Op = IROp->C<IR::IROp_Popcount>();
  const uint8_t OpSize = IROp->Size;

  switch (OpSize) {
    case 0x1:
      fmov(VTMP1.S(), GetReg<RA_32>(Op->Src.ID()));
      // only use lowest byte
      cnt(VTMP1.V8B(), VTMP1.V8B());
      break;
    case 0x2:
      fmov(VTMP1.S(), GetReg<RA_32>(Op->Src.ID()));
      cnt(VTMP1.V8B(), VTMP1.V8B());
      // only count two lowest bytes
      addp(VTMP1.V8B(), VTMP1.V8B(), VTMP1.V8B());
      break;
    case 0x4:
      fmov(VTMP1.S(), GetReg<RA_32>(Op->Src.ID()));
      cnt(VTMP1.V8B(), VTMP1.V8B());
      // fmov has zero extended, unused bytes are zero
      addv(VTMP1.B(), VTMP1.V8B());
      break;
    case 0x8:
      fmov(VTMP1.D(), GetReg<RA_64>(Op->Src.ID()));
      cnt(VTMP1.V8B(), VTMP1.V8B());
      // fmov has zero extended, unused bytes are zero
      addv(VTMP1.B(), VTMP1.V8B());
      break;
    default: LOGMAN_MSG_A_FMT("Unsupported Popcount size: {}", OpSize);
  }

  auto Dst = GetReg<RA_32>(Node);
  umov(Dst.W(), VTMP1.B(), 0);
}

DEF_OP(FindLSB) {
  auto Op = IROp->C<IR::IROp_FindLSB>();
  const uint8_t OpSize = IROp->Size;

  auto Dst = GetReg<RA_64>(Node);
  auto Src = GetReg<RA_64>(Op->Src.ID());
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
}

DEF_OP(FindMSB) {
  auto Op = IROp->C<IR::IROp_FindMSB>();
  const uint8_t OpSize = IROp->Size;

  auto Dst = GetReg<RA_64>(Node);
  switch (OpSize) {
    case 2:
      movz(TMP1, OpSize * 8 - 1);
      lsl(Dst.W(), GetReg<RA_32>(Op->Src.ID()), 16);
      orr(Dst.W(), Dst.W(), 0x8000);
      clz(Dst.W(), Dst.W());
      sub(Dst, TMP1, Dst);
    break;
    case 4:
      movz(TMP1, OpSize * 8 - 1);
      clz(Dst.W(), GetReg<RA_32>(Op->Src.ID()));
      sub(Dst, TMP1, Dst);
      break;
    case 8:
      movz(TMP1, OpSize * 8 - 1);
      clz(Dst, GetReg<RA_64>(Op->Src.ID()));
      sub(Dst, TMP1, Dst);
      break;
    default:
      LOGMAN_MSG_A_FMT("Unknown FindMSB size: {}", OpSize);
      break;
  }
}

DEF_OP(FindTrailingZeros) {
  auto Op = IROp->C<IR::IROp_FindTrailingZeros>();
  const uint8_t OpSize = IROp->Size;

  switch (OpSize) {
    case 2:
      rbit(GetReg<RA_32>(Node), GetReg<RA_32>(Op->Src.ID()));
      orr(GetReg<RA_32>(Node), GetReg<RA_32>(Node), 0x8000);
      clz(GetReg<RA_32>(Node), GetReg<RA_32>(Node));
    break;
    case 4:
      rbit(GetReg<RA_32>(Node), GetReg<RA_32>(Op->Src.ID()));
      clz(GetReg<RA_32>(Node), GetReg<RA_32>(Node));
      break;
    case 8:
      rbit(GetReg<RA_64>(Node), GetReg<RA_64>(Op->Src.ID()));
      clz(GetReg<RA_64>(Node), GetReg<RA_64>(Node));
      break;
    default:
      LOGMAN_MSG_A_FMT("Unknown FindTrailingZeros size: {}", OpSize);
      break;
  }
}

DEF_OP(CountLeadingZeroes) {
  auto Op = IROp->C<IR::IROp_CountLeadingZeroes>();
  const uint8_t OpSize = IROp->Size;

  switch (OpSize) {
    case 2:
      lsl(GetReg<RA_32>(Node), GetReg<RA_32>(Op->Src.ID()), 16);
      orr(GetReg<RA_32>(Node), GetReg<RA_32>(Node), 0x8000);
      clz(GetReg<RA_32>(Node), GetReg<RA_32>(Node));
    break;
    case 4:
      clz(GetReg<RA_32>(Node), GetReg<RA_32>(Op->Src.ID()));
      break;
    case 8:
      clz(GetReg<RA_64>(Node), GetReg<RA_64>(Op->Src.ID()));
      break;
    default:
      LOGMAN_MSG_A_FMT("Unknown CountLeadingZeroes size: {}", OpSize);
      break;
  }
}

DEF_OP(Rev) {
  auto Op = IROp->C<IR::IROp_Rev>();
  const uint8_t OpSize = IROp->Size;

  switch (OpSize) {
    case 2:
      rev(GetReg<RA_32>(Node), GetReg<RA_32>(Op->Src.ID()));
      lsr(GetReg<RA_32>(Node), GetReg<RA_32>(Node), 16);
    break;
    case 4:
      rev(GetReg<RA_32>(Node), GetReg<RA_32>(Op->Src.ID()));
      break;
    case 8:
      rev(GetReg<RA_64>(Node), GetReg<RA_64>(Op->Src.ID()));
      break;
    default:
      LOGMAN_MSG_A_FMT("Unknown REV size: {}", OpSize);
      break;
  }
}

DEF_OP(Bfi) {
  auto Op = IROp->C<IR::IROp_Bfi>();
  const uint8_t OpSize = IROp->Size;

  switch (OpSize) {
    case 1:
    case 2:
    case 4: {
      auto Dst = GetReg<RA_32>(Node);
      mov(TMP1.W(), GetReg<RA_32>(Op->Dest.ID()));
      bfi(TMP1.W(), GetReg<RA_32>(Op->Src.ID()), Op->lsb, Op->Width);
      ubfx(Dst, TMP1.W(), 0, OpSize * 8);
      break;
    }
    case 8:
      mov(TMP1, GetReg<RA_64>(Op->Dest.ID()));
      bfi(TMP1, GetReg<RA_64>(Op->Src.ID()), Op->lsb, Op->Width);
      mov(GetReg<RA_64>(Node), TMP1);
      break;
    default:
      LOGMAN_MSG_A_FMT("Unknown BFI size: {}", OpSize);
      break;
  }
}

DEF_OP(Bfe) {
  auto Op = IROp->C<IR::IROp_Bfe>();
  LOGMAN_THROW_AA_FMT(IROp->Size <= 8, "OpSize is too large for BFE: {}", IROp->Size);
  LOGMAN_THROW_AA_FMT(Op->Width != 0, "Invalid BFE width of 0");

  auto Dst = GetReg<RA_64>(Node);
  ubfx(Dst, GetReg<RA_64>(Op->Src.ID()), Op->lsb, Op->Width);
}

DEF_OP(Sbfe) {
  auto Op = IROp->C<IR::IROp_Bfe>();
  uint8_t OpSize = IROp->Size;

  auto Dst = GetReg<RA_64>(Node);
  if (OpSize == 8) {
    sbfx(Dst, GetReg<RA_64>(Op->Src.ID()), Op->lsb, Op->Width);
  } else {
    LogMan::Msg::DFmt("Unimplemented Sbfe size");
  }
}

#define GRCMP(Node) (Op->CompareSize == 4 ? GetReg<RA_32>(Node) : GetReg<RA_64>(Node))

#define GRFCMP(Node) (Op->CompareSize == 4 ? GetDst(Node).S() : GetDst(Node).D())

Condition MapSelectCC(IR::CondClassType Cond) {
  switch (Cond.Val) {
  case FEXCore::IR::COND_EQ: return Condition::eq;
  case FEXCore::IR::COND_NEQ: return Condition::ne;
  case FEXCore::IR::COND_SGE: return Condition::ge;
  case FEXCore::IR::COND_SLT: return Condition::lt;
  case FEXCore::IR::COND_SGT: return Condition::gt;
  case FEXCore::IR::COND_SLE: return Condition::le;
  case FEXCore::IR::COND_UGE: return Condition::cs;
  case FEXCore::IR::COND_ULT: return Condition::cc;
  case FEXCore::IR::COND_UGT: return Condition::hi;
  case FEXCore::IR::COND_ULE: return Condition::ls;
  case FEXCore::IR::COND_FLU: return Condition::lt;
  case FEXCore::IR::COND_FGE: return Condition::ge;
  case FEXCore::IR::COND_FLEU:return Condition::le;
  case FEXCore::IR::COND_FGT: return Condition::gt;
  case FEXCore::IR::COND_FU:  return Condition::vs;
  case FEXCore::IR::COND_FNU: return Condition::vc;
  case FEXCore::IR::COND_VS:
  case FEXCore::IR::COND_VC:
  case FEXCore::IR::COND_MI:
  case FEXCore::IR::COND_PL:
  default:
  LOGMAN_MSG_A_FMT("Unsupported compare type");
  return Condition::nv;
  }
}

DEF_OP(Select) {
  auto Op = IROp->C<IR::IROp_Select>();

  uint64_t Const;

  if (IsGPR(Op->Cmp1.ID())) {
    if (IsInlineConstant(Op->Cmp2, &Const))
      cmp(GRCMP(Op->Cmp1.ID()), Const);
    else
      cmp(GRCMP(Op->Cmp1.ID()), GRCMP(Op->Cmp2.ID()));
  } else if (IsFPR(Op->Cmp1.ID())) {
    fcmp(GRFCMP(Op->Cmp1.ID()), GRFCMP(Op->Cmp2.ID()));
  } else {
    LOGMAN_MSG_A_FMT("Select: Expected GPR or FPR");
  }

  auto cc = MapSelectCC(Op->Cond);

  uint64_t const_true, const_false;
  bool is_const_true = IsInlineConstant(Op->TrueVal, &const_true);
  bool is_const_false = IsInlineConstant(Op->FalseVal, &const_false);

  if (is_const_true || is_const_false) {
    if (is_const_false != true || is_const_true != true || const_true != 1 || const_false != 0) {
      LOGMAN_MSG_A_FMT("Select: Unsupported compare inline parameters");
    }
    cset(GRS(Node), cc);
  } else {
    csel(GRS(Node), GRS(Op->TrueVal.ID()), GRS(Op->FalseVal.ID()), cc);
  }
}

DEF_OP(VExtractToGPR) {
  const auto Op = IROp->C<IR::IROp_VExtractToGPR>();
  const auto OpSize = IROp->Size;

  constexpr auto AVXRegBitSize = Core::CPUState::XMM_AVX_REG_SIZE * 8;
  constexpr auto SSERegBitSize = Core::CPUState::XMM_SSE_REG_SIZE * 8;
  const auto ElementSizeBits = Op->Header.ElementSize * 8;

  const auto Offset = ElementSizeBits * Op->Index;
  const auto Is256Bit = Offset >= SSERegBitSize;

  const auto Vector = GetSrc(Op->Vector.ID());

  const auto PerformMove = [&](const aarch64::VRegister& reg, int index) {
    switch (OpSize) {
      case 1:
        umov(GetReg<RA_32>(Node), reg.V16B(), index);
        break;
      case 2:
        umov(GetReg<RA_32>(Node), reg.V8H(), index);
        break;
      case 4:
        umov(GetReg<RA_32>(Node), reg.V4S(), index);
        break;
      case 8:
        umov(GetReg<RA_64>(Node), reg.V2D(), index);
        break;
      default:
        LOGMAN_MSG_A_FMT("Unhandled ExtractElementSize: {}", OpSize);
        break;
    }
  };

  if (Offset < SSERegBitSize) {
    // Desired data lies within the lower 128-bit lane, so we
    // can treat the operation as a 128-bit operation, even
    // when acting on larger register sizes.
    PerformMove(Vector, Op->Index);
  } else {
    LOGMAN_THROW_AA_FMT(HostSupportsSVE,
                        "Host doesn't support SVE. Cannot perform 256-bit operation.");
    LOGMAN_THROW_AA_FMT(Is256Bit,
                        "Can't perform 256-bit extraction with op side: {}", OpSize);
    LOGMAN_THROW_AA_FMT(Offset < AVXRegBitSize,
                        "Trying to extract element outside bounds of register. Offset={}, Index={}",
                        Offset, Op->Index);

    // We need to use the upper 128-bit lane, so lets move it down.
    // Inverting our dedicated predicate for 128-bit operations selects
    // all of the top lanes. We can then compact those into a temporary.
    const auto CompactPred = p0;
    not_(CompactPred.VnB(), PRED_TMP_32B.Zeroing(), PRED_TMP_16B.VnB());
    compact(VTMP1.Z().VnD(), CompactPred, Vector.Z().VnD());

    // Sanitize the zero-based index to work on the now-moved
    // upper half of the vector.
    const auto SanitizedIndex = [OpSize, Op] {
      switch (OpSize) {
        case 1:
          return Op->Index - 16;
        case 2:
          return Op->Index - 8;
        case 4:
          return Op->Index - 4;
        case 8:
          return Op->Index - 2;
        default:
          LOGMAN_MSG_A_FMT("Unhandled OpSize: {}", OpSize);
          return 0;
      }
    }();

    // Move the value from the now-low-lane data.
    PerformMove(VTMP1, SanitizedIndex);
  }
}

DEF_OP(Float_ToGPR_ZS) {
  auto Op = IROp->C<IR::IROp_Float_ToGPR_ZS>();
  aarch64::Register Dst{};
  aarch64::VRegister Src{};
  if (Op->SrcElementSize == 8) {
    Src = GetSrc(Op->Scalar.ID()).D();
  }
  else {
    Src = GetSrc(Op->Scalar.ID()).S();
  }

  if (IROp->Size == 8) {
    Dst = GetReg<RA_64>(Node);
  }
  else {
    Dst = GetReg<RA_32>(Node);
  }

  fcvtzs(Dst, Src);
}

DEF_OP(Float_ToGPR_S) {
  auto Op = IROp->C<IR::IROp_Float_ToGPR_S>();

  aarch64::Register Dst{};
  aarch64::VRegister Src{};
  if (Op->SrcElementSize == 8) {
    frinti(VTMP1.D(), GetSrc(Op->Scalar.ID()).D());
    Src = VTMP1.D();
  }
  else {
    frinti(VTMP1.S(), GetSrc(Op->Scalar.ID()).S());
    Src = VTMP1.S();
  }

  if (IROp->Size == 8) {
    Dst = GetReg<RA_64>(Node);
  }
  else {
    Dst = GetReg<RA_32>(Node);
  }

  fcvtzs(Dst, Src);
}

DEF_OP(FCmp) {
  auto Op = IROp->C<IR::IROp_FCmp>();

  if (Op->ElementSize == 4) {
    fcmp(GetSrc(Op->Scalar1.ID()).S(), GetSrc(Op->Scalar2.ID()).S());
  }
  else {
    fcmp(GetSrc(Op->Scalar1.ID()).D(), GetSrc(Op->Scalar2.ID()).D());
  }
  auto Dst = GetReg<RA_64>(Node);

  bool set = false;

  if (Op->Flags & (1 << IR::FCMP_FLAG_EQ)) {
    LOGMAN_THROW_AA_FMT(IR::FCMP_FLAG_EQ == 0, "IR::FCMP_FLAG_EQ must equal 0");
    // EQ or unordered
    cset(Dst, Condition::eq); // Z = 1
    csinc(Dst, Dst, xzr, Condition::vc); // IF !V ? Z : 1
    set = true;
  }

  if (Op->Flags & (1 << IR::FCMP_FLAG_LT)) {
    // LT or unordered
    cset(TMP2, Condition::lt);
    if (!set) {
      lsl(Dst, TMP2, IR::FCMP_FLAG_LT);
      set = true;
    } else {
      bfi(Dst, TMP2, IR::FCMP_FLAG_LT, 1);
    }
  }

  if (Op->Flags & (1 << IR::FCMP_FLAG_UNORDERED)) {
    cset(TMP2, Condition::vs);
    if (!set) {
      lsl(Dst, TMP2, IR::FCMP_FLAG_UNORDERED);
      set = true;
    } else {
      bfi(Dst, TMP2, IR::FCMP_FLAG_UNORDERED, 1);
    }
  }
}

#undef DEF_OP

void Arm64JITCore::RegisterALUHandlers() {
#define REGISTER_OP(op, x) OpHandlers[FEXCore::IR::IROps::OP_##op] = &Arm64JITCore::Op_##x
  REGISTER_OP(TRUNCELEMENTPAIR,  TruncElementPair);
  REGISTER_OP(CONSTANT,          Constant);
  REGISTER_OP(ENTRYPOINTOFFSET,  EntrypointOffset);
  REGISTER_OP(INLINECONSTANT,    InlineConstant);
  REGISTER_OP(INLINEENTRYPOINTOFFSET,  InlineEntrypointOffset);
  REGISTER_OP(CYCLECOUNTER,      CycleCounter);
  REGISTER_OP(ADD,               Add);
  REGISTER_OP(SUB,               Sub);
  REGISTER_OP(NEG,               Neg);
  REGISTER_OP(MUL,               Mul);
  REGISTER_OP(UMUL,              UMul);
  REGISTER_OP(DIV,               Div);
  REGISTER_OP(UDIV,              UDiv);
  REGISTER_OP(REM,               Rem);
  REGISTER_OP(UREM,              URem);
  REGISTER_OP(MULH,              MulH);
  REGISTER_OP(UMULH,             UMulH);
  REGISTER_OP(OR,                Or);
  REGISTER_OP(AND,               And);
  REGISTER_OP(ANDN,              Andn);
  REGISTER_OP(XOR,               Xor);
  REGISTER_OP(LSHL,              Lshl);
  REGISTER_OP(LSHR,              Lshr);
  REGISTER_OP(ASHR,              Ashr);
  REGISTER_OP(ROR,               Ror);
  REGISTER_OP(EXTR,              Extr);
  REGISTER_OP(PDEP,              PDep);
  REGISTER_OP(PEXT,              PExt);
  REGISTER_OP(LDIV,              LDiv);
  REGISTER_OP(LUDIV,             LUDiv);
  REGISTER_OP(LREM,              LRem);
  REGISTER_OP(LUREM,             LURem);
  REGISTER_OP(NOT,               Not);
  REGISTER_OP(POPCOUNT,          Popcount);
  REGISTER_OP(FINDLSB,           FindLSB);
  REGISTER_OP(FINDMSB,           FindMSB);
  REGISTER_OP(FINDTRAILINGZEROS, FindTrailingZeros);
  REGISTER_OP(COUNTLEADINGZEROES, CountLeadingZeroes);
  REGISTER_OP(REV,               Rev);
  REGISTER_OP(BFI,               Bfi);
  REGISTER_OP(BFE,               Bfe);
  REGISTER_OP(SBFE,              Sbfe);
  REGISTER_OP(SELECT,            Select);
  REGISTER_OP(VEXTRACTTOGPR,     VExtractToGPR);
  REGISTER_OP(FLOAT_TOGPR_ZS,    Float_ToGPR_ZS);
  REGISTER_OP(FLOAT_TOGPR_S,     Float_ToGPR_S);
  REGISTER_OP(FCMP,              FCmp);

#undef REGISTER_OP
}

}
