// SPDX-License-Identifier: MIT
/*
$info$
tags: backend|arm64
$end_info$
*/

#include "FEXCore/IR/IR.h"
#include "Interface/Context/Context.h"
#include "Interface/Core/JIT/Arm64/JITClass.h"
#include "Interface/IR/Passes/RegisterAllocationPass.h"

namespace FEXCore::CPU {

#define GRD(Node) (IROp->Size <= 4 ? GetDst<RA_32>(Node) : GetDst<RA_64>(Node))
#define GRS(Node) (IROp->Size <= 4 ? GetReg<RA_32>(Node) : GetReg<RA_64>(Node))

#define DEF_OP(x) void Arm64JITCore::Op_##x(IR::IROp_Header const* IROp, IR::NodeID Node)

#define DEF_BINOP_WITH_CONSTANT(FEXOp, VarOp, ConstOp)                                         \
  DEF_OP(FEXOp) {                                                                              \
    auto Op = IROp->C<IR::IROp_##FEXOp>();                                                     \
                                                                                               \
    uint64_t Const;                                                                            \
    if (IsInlineConstant(Op->Src2, &Const)) {                                                  \
      ConstOp(ConvertSize(IROp), GetReg(Node), GetReg(Op->Src1.ID()), Const);                  \
    } else {                                                                                   \
      VarOp(ConvertSize(IROp), GetReg(Node), GetZeroableReg(Op->Src1), GetReg(Op->Src2.ID())); \
    }                                                                                          \
  }

DEF_BINOP_WITH_CONSTANT(Add, add, add)
DEF_BINOP_WITH_CONSTANT(Sub, sub, sub)
DEF_BINOP_WITH_CONSTANT(AddWithFlags, adds, adds)
DEF_BINOP_WITH_CONSTANT(SubWithFlags, subs, subs)
DEF_BINOP_WITH_CONSTANT(Or, orr, orr)
DEF_BINOP_WITH_CONSTANT(And, and_, and_)
DEF_BINOP_WITH_CONSTANT(Andn, bic, bic)
DEF_BINOP_WITH_CONSTANT(Xor, eor, eor)
DEF_BINOP_WITH_CONSTANT(Lshl, lslv, lsl)
DEF_BINOP_WITH_CONSTANT(Lshr, lsrv, lsr)
DEF_BINOP_WITH_CONSTANT(Ror, rorv, ror)

DEF_OP(TruncElementPair) {
  auto Op = IROp->C<IR::IROp_TruncElementPair>();

  switch (IROp->Size) {
  case 4: {
    auto Dst = GetRegPair(Node);
    auto Src = GetRegPair(Op->Pair.ID());
    mov(ARMEmitter::Size::i32Bit, Dst.first, Src.first);
    mov(ARMEmitter::Size::i32Bit, Dst.second, Src.second);
    break;
  }
  default: LOGMAN_MSG_A_FMT("Unhandled Truncation size: {}", IROp->Size); break;
  }
}

DEF_OP(Constant) {
  auto Op = IROp->C<IR::IROp_Constant>();
  auto Dst = GetReg(Node);
  LoadConstant(ARMEmitter::Size::i64Bit, Dst, Op->Constant);
}

DEF_OP(EntrypointOffset) {
  auto Op = IROp->C<IR::IROp_EntrypointOffset>();

  auto Constant = Entry + Op->Offset;
  auto Dst = GetReg(Node);
  uint64_t Mask = ~0ULL;
  uint8_t OpSize = IROp->Size;
  if (OpSize == 4) {
    Mask = 0xFFFF'FFFFULL;
  }

  LoadConstant(ARMEmitter::Size::i64Bit, Dst, Constant & Mask);
}

DEF_OP(InlineConstant) {
  // nop
}

DEF_OP(InlineEntrypointOffset) {
  // nop
}

DEF_OP(CycleCounter) {
#ifdef DEBUG_CYCLES
  movz(ARMEmitter::Size::i64Bit, GetReg(Node), 0);
#else
  mrs(GetReg(Node), ARMEmitter::SystemRegister::CNTVCT_EL0);
#endif
}

DEF_OP(AddShift) {
  auto Op = IROp->C<IR::IROp_AddShift>();

  add(ConvertSize48(IROp), GetReg(Node), GetReg(Op->Src1.ID()), GetReg(Op->Src2.ID()), ConvertIRShiftType(Op->Shift), Op->ShiftAmount);
}

DEF_OP(AddNZCV) {
  auto Op = IROp->C<IR::IROp_AddNZCV>();

  const auto EmitSize = ConvertSize(IROp);
  auto Src1 = GetReg(Op->Src1.ID());

  uint64_t Const;
  if (IsInlineConstant(Op->Src2, &Const)) {
    LOGMAN_THROW_AA_FMT(IROp->Size >= 4, "Constant not allowed here");
    cmn(EmitSize, Src1, Const);
  } else if (IROp->Size < 4) {
    unsigned Shift = 32 - (8 * IROp->Size);

    lsl(ARMEmitter::Size::i32Bit, TMP1, Src1, Shift);
    cmn(EmitSize, TMP1, GetReg(Op->Src2.ID()), ARMEmitter::ShiftType::LSL, Shift);
  } else {
    cmn(EmitSize, Src1, GetReg(Op->Src2.ID()));
  }
}

DEF_OP(AdcNZCV) {
  auto Op = IROp->C<IR::IROp_AdcNZCV>();

  adcs(ConvertSize48(IROp), ARMEmitter::Reg::zr, GetReg(Op->Src1.ID()), GetReg(Op->Src2.ID()));
}

DEF_OP(AdcWithFlags) {
  auto Op = IROp->C<IR::IROp_AdcWithFlags>();

  adcs(ConvertSize48(IROp), GetReg(Node), GetZeroableReg(Op->Src1), GetReg(Op->Src2.ID()));
}

DEF_OP(Adc) {
  auto Op = IROp->C<IR::IROp_Adc>();

  adc(ConvertSize48(IROp), GetReg(Node), GetZeroableReg(Op->Src1), GetReg(Op->Src2.ID()));
}

DEF_OP(SbbWithFlags) {
  auto Op = IROp->C<IR::IROp_SbbWithFlags>();

  sbcs(ConvertSize48(IROp), GetReg(Node), GetReg(Op->Src1.ID()), GetReg(Op->Src2.ID()));
}

DEF_OP(SbbNZCV) {
  auto Op = IROp->C<IR::IROp_SbbNZCV>();

  sbcs(ConvertSize48(IROp), ARMEmitter::Reg::zr, GetReg(Op->Src1.ID()), GetReg(Op->Src2.ID()));
}

DEF_OP(Sbb) {
  auto Op = IROp->C<IR::IROp_Sbb>();

  sbc(ConvertSize48(IROp), GetReg(Node), GetZeroableReg(Op->Src1), GetReg(Op->Src2.ID()));
}

DEF_OP(TestNZ) {
  auto Op = IROp->C<IR::IROp_TestNZ>();
  const auto EmitSize = ConvertSize(IROp);

  uint64_t Const;
  auto Src1 = GetReg(Op->Src1.ID());

  // Shift the sign bit into place, clearing out the garbage in upper bits.
  // Adding zero does an effective test, setting NZ according to the result and
  // zeroing CV.
  if (IROp->Size < 4) {
    // Cheaper to and+cmn than to lsl+lsl+tst, so do the and ourselves if
    // needed.
    if (Op->Src1 != Op->Src2) {
      if (IsInlineConstant(Op->Src2, &Const)) {
        and_(EmitSize, TMP1, Src1, Const);
      } else {
        auto Src2 = GetReg(Op->Src2.ID());
        and_(EmitSize, TMP1, Src1, Src2);
      }

      Src1 = TMP1;
    }

    unsigned Shift = 32 - (IROp->Size * 8);
    cmn(EmitSize, ARMEmitter::Reg::zr, Src1, ARMEmitter::ShiftType::LSL, Shift);
  } else {
    if (IsInlineConstant(Op->Src2, &Const)) {
      tst(EmitSize, Src1, Const);
    } else {
      const auto Src2 = GetReg(Op->Src2.ID());
      tst(EmitSize, Src1, Src2);
    }
  }
}

DEF_OP(SubShift) {
  auto Op = IROp->C<IR::IROp_SubShift>();

  sub(ConvertSize48(IROp), GetReg(Node), GetReg(Op->Src1.ID()), GetReg(Op->Src2.ID()), ConvertIRShiftType(Op->Shift), Op->ShiftAmount);
}

DEF_OP(SubNZCV) {
  auto Op = IROp->C<IR::IROp_SubNZCV>();
  const uint8_t OpSize = IROp->Size;
  const auto EmitSize = ConvertSize(IROp);

  uint64_t Const;
  if (IsInlineConstant(Op->Src2, &Const)) {
    LOGMAN_THROW_AA_FMT(OpSize >= 4, "Constant not allowed here");
    cmp(EmitSize, GetReg(Op->Src1.ID()), Const);
  } else {
    unsigned Shift = OpSize < 4 ? (32 - (8 * OpSize)) : 0;
    ARMEmitter::Register ShiftedSrc1 = GetZeroableReg(Op->Src1);

    // Shift to fix flags for <32-bit ops.
    // Any shift of zero is still zero so optimize out silly zero shifts.
    if (OpSize < 4 && ShiftedSrc1 != ARMEmitter::Reg::zr) {
      lsl(ARMEmitter::Size::i32Bit, TMP1, ShiftedSrc1, Shift);
      ShiftedSrc1 = TMP1;
    }

    if (OpSize < 4) {
      cmp(EmitSize, ShiftedSrc1, GetReg(Op->Src2.ID()), ARMEmitter::ShiftType::LSL, Shift);
    } else {
      cmp(EmitSize, ShiftedSrc1, GetReg(Op->Src2.ID()));
    }
  }
}

DEF_OP(CmpPairZ) {
  auto Op = IROp->C<IR::IROp_CmpPairZ>();
  const auto EmitSize = ConvertSize(IROp);

  // Save NZCV
  mrs(TMP1, ARMEmitter::SystemRegister::NZCV);

  // Compare, setting Z and clobbering NzCV
  const auto Src1 = GetRegPair(Op->Src1.ID());
  const auto Src2 = GetRegPair(Op->Src2.ID());
  cmp(EmitSize, Src1.first, Src2.first);
  ccmp(EmitSize, Src1.second, Src2.second, ARMEmitter::StatusFlags::None, ARMEmitter::Condition::CC_EQ);

  // Restore NzCV
  if (CTX->HostFeatures.SupportsFlagM) {
    rmif(TMP1, 0, 0xb /* NzCV */);
  } else {
    cset(ARMEmitter::Size::i32Bit, TMP2, ARMEmitter::Condition::CC_EQ);
    bfi(ARMEmitter::Size::i32Bit, TMP1, TMP2, 30 /* lsb: Z */, 1);
    msr(ARMEmitter::SystemRegister::NZCV, TMP1);
  }
}

DEF_OP(CarryInvert) {
  LOGMAN_THROW_A_FMT(CTX->HostFeatures.SupportsFlagM, "Unsupported flagm op");
  cfinv();
}

DEF_OP(RmifNZCV) {
  auto Op = IROp->C<IR::IROp_RmifNZCV>();
  LOGMAN_THROW_A_FMT(CTX->HostFeatures.SupportsFlagM, "Unsupported flagm op");

  rmif(GetZeroableReg(Op->Src).X(), Op->Rotate, Op->Mask);
}

DEF_OP(SetSmallNZV) {
  auto Op = IROp->C<IR::IROp_SetSmallNZV>();
  LOGMAN_THROW_A_FMT(CTX->HostFeatures.SupportsFlagM, "Unsupported flagm op");

  const uint8_t OpSize = IROp->Size;
  LOGMAN_THROW_AA_FMT(OpSize == 1 || OpSize == 2, "Unsupported {} size: {}", __func__, OpSize);

  if (OpSize == 1) {
    setf8(GetReg(Op->Src.ID()).W());
  } else {
    setf16(GetReg(Op->Src.ID()).W());
  }
}

DEF_OP(AXFlag) {
  LOGMAN_THROW_A_FMT(CTX->HostFeatures.SupportsFlagM2, "Unsupported flagm2 op");
  axflag();
}

DEF_OP(CondAddNZCV) {
  auto Op = IROp->C<IR::IROp_CondAddNZCV>();

  ARMEmitter::StatusFlags Flags = (ARMEmitter::StatusFlags)Op->FalseNZCV;
  uint64_t Const = 0;
  auto Src1 = GetZeroableReg(Op->Src1);

  if (IsInlineConstant(Op->Src2, &Const)) {
    ccmn(ConvertSize48(IROp), Src1, Const, Flags, MapCC(Op->Cond));
  } else {
    ccmn(ConvertSize48(IROp), Src1, GetReg(Op->Src2.ID()), Flags, MapCC(Op->Cond));
  }
}

DEF_OP(CondSubNZCV) {
  auto Op = IROp->C<IR::IROp_CondSubNZCV>();

  ARMEmitter::StatusFlags Flags = (ARMEmitter::StatusFlags)Op->FalseNZCV;
  uint64_t Const = 0;
  auto Src1 = GetZeroableReg(Op->Src1);

  if (IsInlineConstant(Op->Src2, &Const)) {
    ccmp(ConvertSize48(IROp), Src1, Const, Flags, MapCC(Op->Cond));
  } else {
    ccmp(ConvertSize48(IROp), Src1, GetReg(Op->Src2.ID()), Flags, MapCC(Op->Cond));
  }
}

DEF_OP(Neg) {
  auto Op = IROp->C<IR::IROp_Neg>();

  if (Op->Cond == FEXCore::IR::COND_AL) {
    neg(ConvertSize48(IROp), GetReg(Node), GetReg(Op->Src.ID()));
  } else {
    cneg(ConvertSize48(IROp), GetReg(Node), GetReg(Op->Src.ID()), MapCC(Op->Cond));
  }
}

DEF_OP(Mul) {
  auto Op = IROp->C<IR::IROp_Mul>();

  mul(ConvertSize48(IROp), GetReg(Node), GetReg(Op->Src1.ID()), GetReg(Op->Src2.ID()));
}

DEF_OP(UMul) {
  auto Op = IROp->C<IR::IROp_UMul>();

  mul(ConvertSize48(IROp), GetReg(Node), GetReg(Op->Src1.ID()), GetReg(Op->Src2.ID()));
}

DEF_OP(UMull) {
  auto Op = IROp->C<IR::IROp_UMull>();
  umull(GetReg(Node).X(), GetReg(Op->Src1.ID()).W(), GetReg(Op->Src2.ID()).W());
}

DEF_OP(SMull) {
  auto Op = IROp->C<IR::IROp_SMull>();
  smull(GetReg(Node).X(), GetReg(Op->Src1.ID()).W(), GetReg(Op->Src2.ID()).W());
}

DEF_OP(Div) {
  auto Op = IROp->C<IR::IROp_Div>();

  // Each source is OpSize in size
  // So you can have up to a 128bit divide from x86-64
  const uint8_t OpSize = IROp->Size;
  const auto EmitSize = ConvertSize(IROp);

  const auto Dst = GetReg(Node);
  auto Src1 = GetReg(Op->Src1.ID());
  auto Src2 = GetReg(Op->Src2.ID());

  if (OpSize == 1) {
    sxtb(EmitSize, TMP1, Src1);
    sxtb(EmitSize, TMP2, Src2);

    Src1 = TMP1;
    Src2 = TMP2;
  } else if (OpSize == 2) {
    sxth(EmitSize, TMP1, Src1);
    sxth(EmitSize, TMP2, Src2);

    Src1 = TMP1;
    Src2 = TMP2;
  }

  sdiv(EmitSize, Dst, Src1, Src2);
}

DEF_OP(UDiv) {
  auto Op = IROp->C<IR::IROp_UDiv>();

  // Each source is OpSize in size
  // So you can have up to a 128bit divide from x86-64
  const uint8_t OpSize = IROp->Size;
  const auto EmitSize = ConvertSize(IROp);

  const auto Dst = GetReg(Node);
  auto Src1 = GetReg(Op->Src1.ID());
  auto Src2 = GetReg(Op->Src2.ID());

  if (OpSize == 1) {
    uxtb(EmitSize, TMP1, Src1);
    uxtb(EmitSize, TMP2, Src2);

    Src1 = TMP1;
    Src2 = TMP2;
  } else if (OpSize == 2) {
    uxth(EmitSize, TMP1, Src1);
    uxth(EmitSize, TMP2, Src2);

    Src1 = TMP1;
    Src2 = TMP2;
  }

  udiv(EmitSize, Dst, Src1, Src2);
}

DEF_OP(Rem) {
  auto Op = IROp->C<IR::IROp_Rem>();
  // Each source is OpSize in size
  // So you can have up to a 128bit divide from x86-64
  const uint8_t OpSize = IROp->Size;
  const auto EmitSize = ConvertSize(IROp);

  const auto Dst = GetReg(Node);
  auto Src1 = GetReg(Op->Src1.ID());
  auto Src2 = GetReg(Op->Src2.ID());

  if (OpSize == 1) {
    sxtb(EmitSize, TMP1, Src1);
    sxtb(EmitSize, TMP2, Src2);

    Src1 = TMP1;
    Src2 = TMP2;
  } else if (OpSize == 2) {
    sxth(EmitSize, TMP1, Src1);
    sxth(EmitSize, TMP2, Src2);

    Src1 = TMP1;
    Src2 = TMP2;
  }

  sdiv(EmitSize, TMP1, Src1, Src2);
  msub(EmitSize, Dst, TMP1, Src2, Src1);
}

DEF_OP(URem) {
  auto Op = IROp->C<IR::IROp_URem>();
  // Each source is OpSize in size
  // So you can have up to a 128bit divide from x86-64
  const uint8_t OpSize = IROp->Size;
  const auto EmitSize = ConvertSize(IROp);

  const auto Dst = GetReg(Node);
  auto Src1 = GetReg(Op->Src1.ID());
  auto Src2 = GetReg(Op->Src2.ID());

  if (OpSize == 1) {
    uxtb(EmitSize, TMP1, Src1);
    uxtb(EmitSize, TMP2, Src2);

    Src1 = TMP1;
    Src2 = TMP2;
  } else if (OpSize == 2) {
    uxth(EmitSize, TMP1, Src1);
    uxth(EmitSize, TMP2, Src2);

    Src1 = TMP1;
    Src2 = TMP2;
  }

  udiv(EmitSize, TMP3, Src1, Src2);
  msub(EmitSize, Dst, TMP3, Src2, Src1);
}

DEF_OP(MulH) {
  auto Op = IROp->C<IR::IROp_MulH>();
  const uint8_t OpSize = IROp->Size;

  LOGMAN_THROW_AA_FMT(OpSize == 4 || OpSize == 8, "Unsupported {} size: {}", __func__, OpSize);

  const auto Dst = GetReg(Node);
  const auto Src1 = GetReg(Op->Src1.ID());
  const auto Src2 = GetReg(Op->Src2.ID());

  if (OpSize == 4) {
    sxtw(TMP1, Src1.W());
    sxtw(TMP2, Src2.W());
    mul(ARMEmitter::Size::i32Bit, Dst, TMP1, TMP2);
    ubfx(ARMEmitter::Size::i32Bit, Dst, Dst, 32, 32);
  } else {
    smulh(Dst.X(), Src1.X(), Src2.X());
  }
}

DEF_OP(UMulH) {
  auto Op = IROp->C<IR::IROp_UMulH>();
  const uint8_t OpSize = IROp->Size;

  LOGMAN_THROW_AA_FMT(OpSize == 4 || OpSize == 8, "Unsupported {} size: {}", __func__, OpSize);

  const auto Dst = GetReg(Node);
  const auto Src1 = GetReg(Op->Src1.ID());
  const auto Src2 = GetReg(Op->Src2.ID());

  if (OpSize == 4) {
    uxtw(ARMEmitter::Size::i64Bit, TMP1, Src1);
    uxtw(ARMEmitter::Size::i64Bit, TMP2, Src2);
    mul(ARMEmitter::Size::i64Bit, Dst, TMP1, TMP2);
    ubfx(ARMEmitter::Size::i64Bit, Dst, Dst, 32, 32);
  } else {
    umulh(Dst.X(), Src1.X(), Src2.X());
  }
}

DEF_OP(Orlshl) {
  auto Op = IROp->C<IR::IROp_Orlshl>();
  const auto Dst = GetReg(Node);
  const auto Src1 = GetReg(Op->Src1.ID());

  uint64_t Const;
  if (IsInlineConstant(Op->Src2, &Const)) {
    orr(ConvertSize(IROp), Dst, Src1, Const << Op->BitShift);
  } else {
    const auto Src2 = GetReg(Op->Src2.ID());
    orr(ConvertSize(IROp), Dst, Src1, Src2, ARMEmitter::ShiftType::LSL, Op->BitShift);
  }
}

DEF_OP(Orlshr) {
  auto Op = IROp->C<IR::IROp_Orlshr>();

  const auto Dst = GetReg(Node);
  const auto Src1 = GetReg(Op->Src1.ID());

  uint64_t Const;
  if (IsInlineConstant(Op->Src2, &Const)) {
    orr(ConvertSize(IROp), Dst, Src1, Const >> Op->BitShift);
  } else {
    const auto Src2 = GetReg(Op->Src2.ID());
    orr(ConvertSize(IROp), Dst, Src1, Src2, ARMEmitter::ShiftType::LSR, Op->BitShift);
  }
}

DEF_OP(Ornror) {
  auto Op = IROp->C<IR::IROp_Ornror>();

  const auto Dst = GetReg(Node);
  const auto Src1 = GetReg(Op->Src1.ID());

  const auto Src2 = GetReg(Op->Src2.ID());
  orn(ConvertSize(IROp), Dst, Src1, Src2, ARMEmitter::ShiftType::ROR, Op->BitShift);
}

DEF_OP(AndWithFlags) {
  auto Op = IROp->C<IR::IROp_AndWithFlags>();
  const uint8_t OpSize = IROp->Size;
  const auto EmitSize = ConvertSize(IROp);

  uint64_t Const;
  const auto Dst = GetReg(Node);
  auto Src1 = GetReg(Op->Src1.ID());

  // See TestNZ
  if (OpSize < 4) {
    if (IsInlineConstant(Op->Src2, &Const)) {
      and_(EmitSize, Dst, Src1, Const);
    } else {
      auto Src2 = GetReg(Op->Src2.ID());

      if (Src1 != Src2) {
        and_(EmitSize, Dst, Src1, Src2);
      } else if (Dst != Src1) {
        mov(ARMEmitter::Size::i64Bit, Dst, Src1);
      }
    }

    unsigned Shift = 32 - (OpSize * 8);
    cmn(EmitSize, ARMEmitter::Reg::zr, Dst, ARMEmitter::ShiftType::LSL, Shift);
  } else {
    if (IsInlineConstant(Op->Src2, &Const)) {
      ands(EmitSize, Dst, Src1, Const);
    } else {
      const auto Src2 = GetReg(Op->Src2.ID());
      ands(EmitSize, Dst, Src1, Src2);
    }
  }
}

DEF_OP(XorShift) {
  auto Op = IROp->C<IR::IROp_XorShift>();

  eor(ConvertSize48(IROp), GetReg(Node), GetReg(Op->Src1.ID()), GetReg(Op->Src2.ID()), ConvertIRShiftType(Op->Shift), Op->ShiftAmount);
}

DEF_OP(XornShift) {
  auto Op = IROp->C<IR::IROp_XornShift>();

  eon(ConvertSize48(IROp), GetReg(Node), GetReg(Op->Src1.ID()), GetReg(Op->Src2.ID()), ConvertIRShiftType(Op->Shift), Op->ShiftAmount);
}

DEF_OP(Ashr) {
  auto Op = IROp->C<IR::IROp_Ashr>();
  const uint8_t OpSize = IROp->Size;
  const auto EmitSize = ConvertSize(IROp);

  const auto Dst = GetReg(Node);
  const auto Src1 = GetReg(Op->Src1.ID());

  uint64_t Const;
  if (IsInlineConstant(Op->Src2, &Const)) {
    if (OpSize >= 4) {
      asr(EmitSize, Dst, Src1, (unsigned int)Const);
    } else {
      sbfx(EmitSize, TMP1, Src1, 0, OpSize * 8);
      asr(EmitSize, Dst, TMP1, (unsigned int)Const);
      ubfx(EmitSize, Dst, Dst, 0, OpSize * 8);
    }
  } else {
    const auto Src2 = GetReg(Op->Src2.ID());
    if (OpSize >= 4) {
      asrv(EmitSize, Dst, Src1, Src2);
    } else {
      sbfx(EmitSize, TMP1, Src1, 0, OpSize * 8);
      asrv(EmitSize, Dst, TMP1, Src2);
      ubfx(EmitSize, Dst, Dst, 0, OpSize * 8);
    }
  }
}

DEF_OP(ShiftFlags) {
  auto Op = IROp->C<IR::IROp_ShiftFlags>();
  const uint8_t OpSize = Op->Size;
  const auto EmitSize = OpSize == 8 ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;

  const auto PFOutput = GetReg(Node);
  const auto PFInput = GetReg(Op->PFInput.ID());
  const auto Dst = GetReg(Op->Result.ID());
  const auto Src1 = GetReg(Op->Src1.ID());
  const auto Src2 = GetReg(Op->Src2.ID());

  bool PFBlocked = (PFOutput == Dst) || (PFOutput == Src1) || (PFOutput == Src2);
  const auto PFTemp = PFBlocked ? TMP4 : PFOutput;

  // Set the output outside the branch to avoid needing an extra leg of the
  // branch. We specifically do not hardcode the PF register anywhere (relying
  // on a tied SRA register instead) to avoid fighting with RA.
  if (PFTemp != PFInput) {
    mov(ARMEmitter::Size::i64Bit, PFTemp, PFInput);
  }

  ARMEmitter::SingleUseForwardLabel Done;
  cbz(EmitSize, Src2, &Done);
  {
    // PF/SF/ZF/OF
    if (OpSize >= 4) {
      ands(EmitSize, PFTemp, Dst, Dst);
    } else {
      unsigned Shift = 32 - (OpSize * 8);
      cmn(EmitSize, ARMEmitter::Reg::zr, Dst, ARMEmitter::ShiftType::LSL, Shift);
      mov(ARMEmitter::Size::i64Bit, PFTemp, Dst);
    }

    // Extract the last bit shifted in to CF
    if (Op->Shift == IR::ShiftType::LSL) {
      if (OpSize >= 4) {
        neg(EmitSize, TMP1, Src2);
      } else {
        mov(EmitSize, TMP1, OpSize * 8);
        sub(EmitSize, TMP1, TMP1, Src2);
      }
    } else {
      sub(ARMEmitter::Size::i64Bit, TMP1, Src2, 1);
    }

    lsrv(EmitSize, TMP1, Src1, TMP1);

    bool SetOF = Op->Shift != IR::ShiftType::ASR;
    if (SetOF) {
      // Only defined when Shift is 1 else undefined
      // OF flag is set if a sign change occurred
      eor(EmitSize, TMP3, Src1, Dst);
    }

    if (CTX->HostFeatures.SupportsFlagM) {
      rmif(TMP1, 63, (1 << 1) /* C */);

      if (SetOF) {
        rmif(TMP3, OpSize * 8 - 1, (1 << 0) /* V */);
      }
    } else {
      mrs(TMP2, ARMEmitter::SystemRegister::NZCV);
      bfi(ARMEmitter::Size::i32Bit, TMP2, TMP1, 29 /* C */, 1);

      if (SetOF) {
        lsr(EmitSize, TMP3, TMP3, OpSize * 8 - 1);
        bfi(ARMEmitter::Size::i32Bit, TMP2, TMP3, 28 /* V */, 1);
      }

      msr(ARMEmitter::SystemRegister::NZCV, TMP2);
    }
  }
  Bind(&Done);

  // TODO: Make RA less dumb so this can't happen (e.g. with late-kill).
  if (PFOutput != PFTemp) {
    mov(ARMEmitter::Size::i64Bit, PFOutput, PFTemp);
  }
}

DEF_OP(Extr) {
  auto Op = IROp->C<IR::IROp_Extr>();
  const auto Dst = GetReg(Node);
  const auto Upper = GetReg(Op->Upper.ID());
  const auto Lower = GetReg(Op->Lower.ID());

  extr(ConvertSize48(IROp), Dst, Upper, Lower, Op->LSB);
}

DEF_OP(PDep) {
  auto Op = IROp->C<IR::IROp_PExt>();
  const auto EmitSize = ConvertSize48(IROp);

  const auto Dest = GetReg(Node);

  // PDep implementation follows the ideas from
  // http://0x80.pl/articles/pdep-soft-emu.html ... Basically, iterate the *set*
  // bits only, which will be faster than the naive implementation as long as
  // there are enough holes in the mask.
  //
  // The specific arm64 assembly used is based on the sequence that clang
  // generates for the C code, giving context to the scheduling yielding better
  // ILP than I would do by hand. The registers are allocated by hand however,
  // to fit within the tight constraints we have here withot spilling. Also, we
  // use cbz/cbnz for conditional branching to avoid clobbering NZCV.

  // We can't clobber these
  const auto OrigInput = GetReg(Op->Input.ID());
  const auto OrigMask = GetReg(Op->Mask.ID());

  // So we have shadow as temporaries
  const auto Input = TMP1.R();
  const auto Mask = TMP2.R();

  // these get used variously as scratch
  const auto T0 = TMP3.R();
  const auto T1 = TMP4.R();

  ARMEmitter::BackwardLabel NextBit;
  ARMEmitter::SingleUseForwardLabel Done;

  // First, copy the input/mask, since we'll be clobbering. Copy as 64-bit to
  // make this 0-uop on Firestorm.
  mov(ARMEmitter::Size::i64Bit, Input, OrigInput);
  mov(ARMEmitter::Size::i64Bit, Mask, OrigMask);

  // Now, they're copied, so we can start setting Dest (even if it overlaps with
  // one of them).  Handle early exit case
  mov(EmitSize, Dest, 0);
  cbz(EmitSize, OrigMask, &Done);

  // Setup for first iteration
  neg(EmitSize, T0, Mask);
  and_(EmitSize, T0, T0, Mask);

  // Main loop
  Bind(&NextBit);
  sbfx(EmitSize, T1, Input, 0, 1);
  eor(EmitSize, Mask, Mask, T0);
  and_(EmitSize, T0, T1, T0);
  neg(EmitSize, T1, Mask);
  orr(EmitSize, Dest, Dest, T0);
  lsr(EmitSize, Input, Input, 1);
  and_(EmitSize, T0, Mask, T1);
  cbnz(EmitSize, T0, &NextBit);

  // All done with nothing to do.
  Bind(&Done);
}

DEF_OP(PExt) {
  auto Op = IROp->C<IR::IROp_PExt>();
  const auto OpSize = IROp->Size;
  const auto OpSizeBitsM1 = (OpSize * 8) - 1;
  const auto EmitSize = ConvertSize48(IROp);

  const auto Input = GetReg(Op->Input.ID());
  const auto Mask = GetReg(Op->Mask.ID());
  const auto Dest = GetReg(Node);

  const auto MaskReg = TMP1;
  const auto BitReg = TMP2;
  const auto ValueReg = TMP3;

  ARMEmitter::SingleUseForwardLabel EarlyExit;
  ARMEmitter::BackwardLabel NextBit;
  ARMEmitter::SingleUseForwardLabel Done;

  cbz(EmitSize, Mask, &EarlyExit);
  mov(EmitSize, MaskReg, Mask);
  mov(EmitSize, ValueReg, Input);
  mov(EmitSize, Dest, ARMEmitter::Reg::zr);

  // Main loop
  Bind(&NextBit);
  cbz(EmitSize, MaskReg, &Done);
  clz(EmitSize, BitReg, MaskReg);
  lslv(EmitSize, ValueReg, ValueReg, BitReg);
  lslv(EmitSize, MaskReg, MaskReg, BitReg);
  extr(EmitSize, Dest, Dest, ValueReg, OpSizeBitsM1);
  bfc(EmitSize, MaskReg, OpSizeBitsM1, 1);
  b(&NextBit);

  // Early exit
  Bind(&EarlyExit);
  mov(EmitSize, Dest, ARMEmitter::Reg::zr);

  // All done with nothing to do.
  Bind(&Done);
}

DEF_OP(LDiv) {
  auto Op = IROp->C<IR::IROp_LDiv>();
  const uint8_t OpSize = IROp->Size;
  const auto EmitSize = OpSize >= 4 ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;

  const auto Dst = GetReg(Node);
  const auto Upper = GetReg(Op->Upper.ID());
  const auto Lower = GetReg(Op->Lower.ID());
  const auto Divisor = GetReg(Op->Divisor.ID());

  // Each source is OpSize in size
  // So you can have up to a 128bit divide from x86-64
  switch (OpSize) {
  case 2: {
    uxth(EmitSize, TMP1, Lower);
    bfi(EmitSize, TMP1, Upper, 16, 16);
    sxth(EmitSize, TMP2, Divisor);
    sdiv(EmitSize, Dst, TMP1, TMP2);
    break;
  }
  case 4: {
    // TODO: 32-bit operation should be guaranteed not to leave garbage in the upper bits.
    mov(EmitSize, TMP1, Lower);
    bfi(EmitSize, TMP1, Upper, 32, 32);
    sxtw(TMP2, Divisor.W());
    sdiv(EmitSize, Dst, TMP1, TMP2);
    break;
  }
  case 8: {
    ARMEmitter::SingleUseForwardLabel Only64Bit {};
    ARMEmitter::SingleUseForwardLabel LongDIVRet {};

    // Check if the upper bits match the top bit of the lower 64-bits
    // Sign extend the top bit of lower bits
    sbfx(EmitSize, TMP1, Lower, 63, 1);
    eor(EmitSize, TMP1, TMP1, Upper);

    // If the sign bit matches then the result is zero
    cbz(EmitSize, TMP1, &Only64Bit);

    // Long divide
    {
      mov(EmitSize, TMP1, Upper);
      mov(EmitSize, TMP2, Lower);
      mov(EmitSize, TMP3, Divisor);

      ldr(TMP4, STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.AArch64.LDIVHandler));

      str<ARMEmitter::IndexType::PRE>(ARMEmitter::XReg::lr, ARMEmitter::Reg::rsp, -16);
      blr(TMP4);
      ldr<ARMEmitter::IndexType::POST>(ARMEmitter::XReg::lr, ARMEmitter::Reg::rsp, 16);

      // Move result to its destination register
      mov(EmitSize, Dst, TMP1);

      // Skip 64-bit path
      b(&LongDIVRet);
    }

    Bind(&Only64Bit);
    // 64-Bit only
    { sdiv(EmitSize, Dst, Lower, Divisor); }

    Bind(&LongDIVRet);
    break;
  }
  default: LOGMAN_MSG_A_FMT("Unknown LDIV Size: {}", OpSize); break;
  }
}

DEF_OP(LUDiv) {
  auto Op = IROp->C<IR::IROp_LUDiv>();
  const uint8_t OpSize = IROp->Size;
  const auto EmitSize = OpSize >= 4 ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;

  const auto Dst = GetReg(Node);
  const auto Upper = GetReg(Op->Upper.ID());
  const auto Lower = GetReg(Op->Lower.ID());
  const auto Divisor = GetReg(Op->Divisor.ID());

  // Each source is OpSize in size
  // So you can have up to a 128bit divide from x86-64=
  switch (OpSize) {
  case 2: {
    uxth(EmitSize, TMP1, Lower);
    bfi(EmitSize, TMP1, Upper, 16, 16);
    udiv(EmitSize, Dst, TMP1, Divisor);
    break;
  }
  case 4: {
    // TODO: 32-bit operation should be guaranteed not to leave garbage in the upper bits.
    mov(EmitSize, TMP1, Lower);
    bfi(EmitSize, TMP1, Upper, 32, 32);
    udiv(EmitSize, Dst, TMP1, Divisor);
    break;
  }
  case 8: {
    ARMEmitter::SingleUseForwardLabel Only64Bit {};
    ARMEmitter::SingleUseForwardLabel LongDIVRet {};

    // Check the upper bits for zero
    // If the upper bits are zero then we can do a 64-bit divide
    cbz(EmitSize, Upper, &Only64Bit);

    // Long divide
    {
      mov(EmitSize, TMP1, Upper);
      mov(EmitSize, TMP2, Lower);
      mov(EmitSize, TMP3, Divisor);

      ldr(TMP4, STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.AArch64.LUDIVHandler));

      str<ARMEmitter::IndexType::PRE>(ARMEmitter::XReg::lr, ARMEmitter::Reg::rsp, -16);
      blr(TMP4);
      ldr<ARMEmitter::IndexType::POST>(ARMEmitter::XReg::lr, ARMEmitter::Reg::rsp, 16);

      // Move result to its destination register
      mov(EmitSize, Dst, TMP1);

      // Skip 64-bit path
      b(&LongDIVRet);
    }

    Bind(&Only64Bit);
    // 64-Bit only
    { udiv(EmitSize, Dst, Lower, Divisor); }

    Bind(&LongDIVRet);
    break;
  }
  default: LOGMAN_MSG_A_FMT("Unknown LUDIV Size: {}", OpSize); break;
  }
}

DEF_OP(LRem) {
  auto Op = IROp->C<IR::IROp_LRem>();
  const uint8_t OpSize = IROp->Size;
  const auto EmitSize = OpSize >= 4 ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;

  const auto Dst = GetReg(Node);
  const auto Upper = GetReg(Op->Upper.ID());
  const auto Lower = GetReg(Op->Lower.ID());
  const auto Divisor = GetReg(Op->Divisor.ID());

  // Each source is OpSize in size
  // So you can have up to a 128bit divide from x86-64
  switch (OpSize) {
  case 2: {
    uxth(EmitSize, TMP1, Lower);
    bfi(EmitSize, TMP1, Upper, 16, 16);
    sxth(EmitSize, TMP2, Divisor);
    sdiv(EmitSize, TMP3, TMP1, TMP2);
    msub(EmitSize, Dst, TMP3, TMP2, TMP1);
    break;
  }
  case 4: {
    // TODO: 32-bit operation should be guaranteed not to leave garbage in the upper bits.
    mov(EmitSize, TMP1, Lower);
    bfi(EmitSize, TMP1, Upper, 32, 32);
    sxtw(TMP3, Divisor.W());
    sdiv(EmitSize, TMP2, TMP1, TMP3);
    msub(EmitSize, Dst, TMP2, TMP3, TMP1);
    break;
  }
  case 8: {
    ARMEmitter::SingleUseForwardLabel Only64Bit {};
    ARMEmitter::SingleUseForwardLabel LongDIVRet {};

    // Check if the upper bits match the top bit of the lower 64-bits
    // Sign extend the top bit of lower bits
    sbfx(EmitSize, TMP1, Lower, 63, 1);
    eor(EmitSize, TMP1, TMP1, Upper);

    // If the sign bit matches then the result is zero
    cbz(EmitSize, TMP1, &Only64Bit);

    // Long divide
    {
      mov(EmitSize, TMP1, Upper);
      mov(EmitSize, TMP2, Lower);
      mov(EmitSize, TMP3, Divisor);

      ldr(TMP4, STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.AArch64.LREMHandler));

      str<ARMEmitter::IndexType::PRE>(ARMEmitter::XReg::lr, ARMEmitter::Reg::rsp, -16);
      blr(TMP4);
      ldr<ARMEmitter::IndexType::POST>(ARMEmitter::XReg::lr, ARMEmitter::Reg::rsp, 16);

      // Move result to its destination register
      mov(EmitSize, Dst, TMP1);

      // Skip 64-bit path
      b(&LongDIVRet);
    }

    Bind(&Only64Bit);
    // 64-Bit only
    {
      sdiv(EmitSize, TMP1, Lower, Divisor);
      msub(EmitSize, Dst, TMP1, Divisor, Lower);
    }
    Bind(&LongDIVRet);
    break;
  }
  default: LOGMAN_MSG_A_FMT("Unknown LREM Size: {}", OpSize); break;
  }
}

DEF_OP(LURem) {
  auto Op = IROp->C<IR::IROp_LURem>();
  const uint8_t OpSize = IROp->Size;
  const auto EmitSize = OpSize >= 4 ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;

  const auto Dst = GetReg(Node);
  const auto Upper = GetReg(Op->Upper.ID());
  const auto Lower = GetReg(Op->Lower.ID());
  const auto Divisor = GetReg(Op->Divisor.ID());

  // Each source is OpSize in size
  // So you can have up to a 128bit divide from x86-64
  switch (OpSize) {
  case 2: {
    uxth(EmitSize, TMP1, Lower);
    bfi(EmitSize, TMP1, Upper, 16, 16);
    udiv(EmitSize, TMP2, TMP1, Divisor);
    msub(EmitSize, Dst, TMP2, Divisor, TMP1);
    break;
  }
  case 4: {
    // TODO: 32-bit operation should be guaranteed not to leave garbage in the upper bits.
    mov(EmitSize, TMP1, Lower);
    bfi(EmitSize, TMP1, Upper, 32, 32);
    udiv(EmitSize, TMP2, TMP1, Divisor);
    msub(EmitSize, Dst, TMP2, Divisor, TMP1);
    break;
  }
  case 8: {
    ARMEmitter::SingleUseForwardLabel Only64Bit {};
    ARMEmitter::SingleUseForwardLabel LongDIVRet {};

    // Check the upper bits for zero
    // If the upper bits are zero then we can do a 64-bit divide
    cbz(EmitSize, Upper, &Only64Bit);

    // Long divide
    {
      mov(EmitSize, TMP1, Upper);
      mov(EmitSize, TMP2, Lower);
      mov(EmitSize, TMP3, Divisor);

      ldr(TMP4, STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.AArch64.LUREMHandler));

      str<ARMEmitter::IndexType::PRE>(ARMEmitter::XReg::lr, ARMEmitter::Reg::rsp, -16);
      blr(TMP4);
      ldr<ARMEmitter::IndexType::POST>(ARMEmitter::XReg::lr, ARMEmitter::Reg::rsp, 16);

      // Move result to its destination register
      mov(EmitSize, Dst, TMP1);

      // Skip 64-bit path
      b(&LongDIVRet);
    }

    Bind(&Only64Bit);
    // 64-Bit only
    {
      udiv(EmitSize, TMP1, Lower, Divisor);
      msub(EmitSize, Dst, TMP1, Divisor, Lower);
    }

    Bind(&LongDIVRet);
    break;
  }
  default: LOGMAN_MSG_A_FMT("Unknown LUREM Size: {}", OpSize); break;
  }
}

DEF_OP(Not) {
  auto Op = IROp->C<IR::IROp_Not>();

  const auto Dst = GetReg(Node);
  const auto Src = GetReg(Op->Src.ID());

  mvn(ConvertSize48(IROp), Dst, Src);
}

DEF_OP(Popcount) {
  auto Op = IROp->C<IR::IROp_Popcount>();
  const uint8_t OpSize = IROp->Size;

  const auto Dst = GetReg(Node);
  const auto Src = GetReg(Op->Src.ID());

  switch (OpSize) {
  case 0x1:
    fmov(ARMEmitter::Size::i32Bit, VTMP1.S(), Src);
    // only use lowest byte
    cnt(ARMEmitter::SubRegSize::i8Bit, VTMP1.D(), VTMP1.D());
    break;
  case 0x2:
    fmov(ARMEmitter::Size::i32Bit, VTMP1.S(), Src);
    cnt(ARMEmitter::SubRegSize::i8Bit, VTMP1.D(), VTMP1.D());
    // only count two lowest bytes
    addp(ARMEmitter::SubRegSize::i8Bit, VTMP1.D(), VTMP1.D(), VTMP1.D());
    break;
  case 0x4:
    fmov(ARMEmitter::Size::i32Bit, VTMP1.S(), Src);
    cnt(ARMEmitter::SubRegSize::i8Bit, VTMP1.D(), VTMP1.D());
    // fmov has zero extended, unused bytes are zero
    addv(ARMEmitter::SubRegSize::i8Bit, VTMP1.D(), VTMP1.D());
    break;
  case 0x8:
    fmov(ARMEmitter::Size::i64Bit, VTMP1.D(), Src);
    cnt(ARMEmitter::SubRegSize::i8Bit, VTMP1.D(), VTMP1.D());
    // fmov has zero extended, unused bytes are zero
    addv(ARMEmitter::SubRegSize::i8Bit, VTMP1.D(), VTMP1.D());
    break;
  default: LOGMAN_MSG_A_FMT("Unsupported Popcount size: {}", OpSize);
  }

  umov<ARMEmitter::SubRegSize::i8Bit>(Dst, VTMP1, 0);
}

DEF_OP(FindLSB) {
  auto Op = IROp->C<IR::IROp_FindLSB>();
  const auto EmitSize = ConvertSize(IROp);

  const auto Dst = GetReg(Node);
  const auto Src = GetReg(Op->Src.ID());

  if (IROp->Size != 8) {
    ubfx(EmitSize, TMP1, Src, 0, IROp->Size * 8);
    cmp(EmitSize, TMP1, 0);
    rbit(EmitSize, TMP1, TMP1);
  } else {
    rbit(EmitSize, TMP1, Src);
    cmp(EmitSize, Src, 0);
  }

  clz(EmitSize, Dst, TMP1);
  csinv(EmitSize, Dst, Dst, ARMEmitter::Reg::zr, ARMEmitter::Condition::CC_NE);
}

DEF_OP(FindMSB) {
  auto Op = IROp->C<IR::IROp_FindMSB>();
  const uint8_t OpSize = IROp->Size;

  LOGMAN_THROW_AA_FMT(OpSize == 2 || OpSize == 4 || OpSize == 8, "Unsupported {} size: {}", __func__, OpSize);
  const auto EmitSize = ConvertSize(IROp);

  const auto Dst = GetReg(Node);
  const auto Src = GetReg(Op->Src.ID());

  movz(ARMEmitter::Size::i64Bit, TMP1, OpSize * 8 - 1);

  if (OpSize == 2) {
    lsl(EmitSize, Dst, Src, 16);
    orr(EmitSize, Dst, Dst, 0x8000);
    clz(EmitSize, Dst, Dst);
  } else {
    clz(EmitSize, Dst, Src);
  }

  sub(ARMEmitter::Size::i64Bit, Dst, TMP1, Dst);
}

DEF_OP(FindTrailingZeroes) {
  auto Op = IROp->C<IR::IROp_FindTrailingZeroes>();
  const uint8_t OpSize = IROp->Size;

  LOGMAN_THROW_AA_FMT(OpSize == 2 || OpSize == 4 || OpSize == 8, "Unsupported {} size: {}", __func__, OpSize);
  const auto EmitSize = ConvertSize(IROp);

  const auto Dst = GetReg(Node);
  const auto Src = GetReg(Op->Src.ID());

  rbit(EmitSize, Dst, Src);

  if (OpSize == 2) {
    // This orr does two things. First, if the (masked) source is zero, it
    // reverses to zero in the top so it forces clz to return 16. Second, it
    // ensures garbage in the upper bits of the source don't affect clz, because
    // they'll rbit to garbage in the bottom below the 0x8000 and be ignored by
    // the clz. So we handle Src upper garbage without explicitly masking.
    orr(EmitSize, Dst, Dst, 0x8000);
  }

  clz(EmitSize, Dst, Dst);
}

DEF_OP(CountLeadingZeroes) {
  auto Op = IROp->C<IR::IROp_CountLeadingZeroes>();
  const uint8_t OpSize = IROp->Size;

  LOGMAN_THROW_AA_FMT(OpSize == 2 || OpSize == 4 || OpSize == 8, "Unsupported {} size: {}", __func__, OpSize);
  const auto EmitSize = ConvertSize(IROp);

  const auto Dst = GetReg(Node);
  const auto Src = GetReg(Op->Src.ID());

  if (OpSize == 2) {
    // Expressing as lsl+orr+clz clears away any garbage in the upper bits
    // (alternatively could do uxth+clz+sub.. equal cost in total).
    lsl(EmitSize, Dst, Src, 16);
    orr(EmitSize, Dst, Dst, 0x8000);
    clz(EmitSize, Dst, Dst);
  } else {
    clz(EmitSize, Dst, Src);
  }
}

DEF_OP(Rev) {
  auto Op = IROp->C<IR::IROp_Rev>();
  const uint8_t OpSize = IROp->Size;

  LOGMAN_THROW_AA_FMT(OpSize == 2 || OpSize == 4 || OpSize == 8, "Unsupported {} size: {}", __func__, OpSize);
  const auto EmitSize = ConvertSize(IROp);

  const auto Dst = GetReg(Node);
  const auto Src = GetReg(Op->Src.ID());

  rev(EmitSize, Dst, Src);
  if (OpSize == 2) {
    lsr(EmitSize, Dst, Dst, 16);
  }
}

DEF_OP(Bfi) {
  auto Op = IROp->C<IR::IROp_Bfi>();
  const auto EmitSize = ConvertSize(IROp);

  const auto Dst = GetReg(Node);
  const auto SrcDst = GetReg(Op->Dest.ID());
  const auto Src = GetReg(Op->Src.ID());

  if (Dst == SrcDst) {
    // If Dst and SrcDst match then this turns in to a simple BFI instruction.
    bfi(EmitSize, Dst, Src, Op->lsb, Op->Width);
  } else if (Dst != Src) {
    // If the destination isn't the source then we can move the DstSrc and insert directly.
    mov(EmitSize, Dst, SrcDst);
    bfi(EmitSize, Dst, Src, Op->lsb, Op->Width);
  } else {
    // Destination didn't match the dst source register.
    // TODO: Inefficient until FEX can have RA constraints here.
    mov(EmitSize, TMP1, SrcDst);
    bfi(EmitSize, TMP1, Src, Op->lsb, Op->Width);

    if (IROp->Size >= 4) {
      mov(EmitSize, Dst, TMP1.R());
    } else {
      ubfx(EmitSize, Dst, TMP1, 0, IROp->Size * 8);
    }
  }
}

DEF_OP(Bfxil) {
  auto Op = IROp->C<IR::IROp_Bfxil>();
  const auto EmitSize = ConvertSize(IROp);

  const auto Dst = GetReg(Node);
  const auto SrcDst = GetReg(Op->Dest.ID());
  const auto Src = GetReg(Op->Src.ID());

  if (Dst == SrcDst) {
    // If Dst and SrcDst match then this turns in to a single instruction.
    bfxil(EmitSize, Dst, Src, Op->lsb, Op->Width);
  } else if (Dst != Src) {
    // If the destination isn't the source then we can move the DstSrc and insert directly.
    mov(EmitSize, Dst, SrcDst);
    bfxil(EmitSize, Dst, Src, Op->lsb, Op->Width);
  } else {
    // Destination didn't match the dst source register.
    // TODO: Inefficient until FEX can have RA constraints here.
    mov(EmitSize, TMP1, SrcDst);
    bfxil(EmitSize, TMP1, Src, Op->lsb, Op->Width);
    mov(EmitSize, Dst, TMP1.R());
  }
}

DEF_OP(Bfe) {
  auto Op = IROp->C<IR::IROp_Bfe>();
  LOGMAN_THROW_AA_FMT(IROp->Size <= 8, "OpSize is too large for BFE: {}", IROp->Size);
  LOGMAN_THROW_AA_FMT(Op->Width != 0, "Invalid BFE width of 0");
  const auto EmitSize = ConvertSize(IROp);

  const auto Dst = GetReg(Node);
  const auto Src = GetReg(Op->Src.ID());

  if (Op->lsb == 0 && Op->Width == 32) {
    mov(ARMEmitter::Size::i32Bit, Dst, Src);
  } else if (Op->lsb == 0 && Op->Width == 64) {
    LOGMAN_THROW_AA_FMT(IROp->Size == 8, "Must be 64-bit wide register");
    mov(ARMEmitter::Size::i64Bit, Dst, Src);
  } else {
    ubfx(EmitSize, Dst, Src, Op->lsb, Op->Width);
  }
}

DEF_OP(Sbfe) {
  auto Op = IROp->C<IR::IROp_Sbfe>();
  const auto Dst = GetReg(Node);
  const auto Src = GetReg(Op->Src.ID());

  sbfx(ConvertSize(IROp), Dst, Src, Op->lsb, Op->Width);
}

DEF_OP(Select) {
  auto Op = IROp->C<IR::IROp_Select>();
  const uint8_t OpSize = IROp->Size;
  const auto EmitSize = ConvertSize(IROp);
  const auto CompareEmitSize = Op->CompareSize == 8 ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;

  uint64_t Const;
  auto cc = MapCC(Op->Cond);

  if (IsGPR(Op->Cmp1.ID())) {
    const auto Src1 = GetReg(Op->Cmp1.ID());

    if (IsInlineConstant(Op->Cmp2, &Const)) {
      cmp(CompareEmitSize, Src1, Const);
    } else {
      const auto Src2 = GetReg(Op->Cmp2.ID());
      cmp(CompareEmitSize, Src1, Src2);
    }
  } else if (IsGPRPair(Op->Cmp1.ID())) {
    const auto Src1 = GetRegPair(Op->Cmp1.ID());
    const auto Src2 = GetRegPair(Op->Cmp2.ID());
    cmp(EmitSize, Src1.first, Src2.first);
    ccmp(EmitSize, Src1.second, Src2.second, ARMEmitter::StatusFlags::None, cc);
  } else if (IsFPR(Op->Cmp1.ID())) {
    const auto Src1 = GetVReg(Op->Cmp1.ID());
    const auto Src2 = GetVReg(Op->Cmp2.ID());
    fcmp(Op->CompareSize == 8 ? ARMEmitter::ScalarRegSize::i64Bit : ARMEmitter::ScalarRegSize::i32Bit, Src1, Src2);
  } else {
    LOGMAN_MSG_A_FMT("Select: Expected GPR or FPR");
  }

  uint64_t const_true, const_false;
  bool is_const_true = IsInlineConstant(Op->TrueVal, &const_true);
  bool is_const_false = IsInlineConstant(Op->FalseVal, &const_false);

  uint64_t all_ones = OpSize == 8 ? 0xffff'ffff'ffff'ffffull : 0xffff'ffffull;

  ARMEmitter::Register Dst = GetReg(Node);

  if (is_const_true || is_const_false) {
    if (is_const_false != true || is_const_true != true || !(const_true == 1 || const_true == all_ones) || const_false != 0) {
      LOGMAN_MSG_A_FMT("Select: Unsupported compare inline parameters");
    }

    if (const_true == all_ones) {
      csetm(EmitSize, Dst, cc);
    } else {
      cset(EmitSize, Dst, cc);
    }
  } else {
    csel(EmitSize, Dst, GetReg(Op->TrueVal.ID()), GetReg(Op->FalseVal.ID()), cc);
  }
}

DEF_OP(NZCVSelect) {
  auto Op = IROp->C<IR::IROp_NZCVSelect>();
  const auto EmitSize = ConvertSize(IROp);

  auto cc = MapCC(Op->Cond);

  uint64_t const_true, const_false;
  bool is_const_true = IsInlineConstant(Op->TrueVal, &const_true);
  bool is_const_false = IsInlineConstant(Op->FalseVal, &const_false);

  uint64_t all_ones = IROp->Size == 8 ? 0xffff'ffff'ffff'ffffull : 0xffff'ffffull;

  ARMEmitter::Register Dst = GetReg(Node);

  if (is_const_true) {
    if (is_const_false != true || !(const_true == 1 || const_true == all_ones) || const_false != 0) {
      LOGMAN_MSG_A_FMT("NZCVSelect: Unsupported constant");
    }

    if (const_true == all_ones) {
      csetm(EmitSize, Dst, cc);
    } else {
      cset(EmitSize, Dst, cc);
    }
  } else {
    csel(EmitSize, Dst, GetReg(Op->TrueVal.ID()), GetZeroableReg(Op->FalseVal), cc);
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

  const auto Dst = GetReg(Node);
  const auto Vector = GetVReg(Op->Vector.ID());

  const auto PerformMove = [&](const ARMEmitter::VRegister reg, int index) {
    switch (OpSize) {
    case 1: umov<ARMEmitter::SubRegSize::i8Bit>(Dst, Vector, index); break;
    case 2: umov<ARMEmitter::SubRegSize::i16Bit>(Dst, Vector, index); break;
    case 4: umov<ARMEmitter::SubRegSize::i32Bit>(Dst, Vector, index); break;
    case 8: umov<ARMEmitter::SubRegSize::i64Bit>(Dst, Vector, index); break;
    default: LOGMAN_MSG_A_FMT("Unhandled ExtractElementSize: {}", OpSize); break;
    }
  };

  if (Offset < SSERegBitSize) {
    // Desired data lies within the lower 128-bit lane, so we
    // can treat the operation as a 128-bit operation, even
    // when acting on larger register sizes.
    PerformMove(Vector, Op->Index);
  } else {
    LOGMAN_THROW_AA_FMT(HostSupportsSVE256, "Host doesn't support SVE. Cannot perform 256-bit operation.");
    LOGMAN_THROW_AA_FMT(Is256Bit, "Can't perform 256-bit extraction with op side: {}", OpSize);
    LOGMAN_THROW_AA_FMT(Offset < AVXRegBitSize, "Trying to extract element outside bounds of register. Offset={}, Index={}", Offset, Op->Index);

    // We need to use the upper 128-bit lane, so lets move it down.
    // Inverting our dedicated predicate for 128-bit operations selects
    // all of the top lanes. We can then compact those into a temporary.
    const auto CompactPred = ARMEmitter::PReg::p0;
    not_(CompactPred, PRED_TMP_32B.Zeroing(), PRED_TMP_16B);
    compact(ARMEmitter::SubRegSize::i64Bit, VTMP1.Z(), CompactPred, Vector.Z());

    // Sanitize the zero-based index to work on the now-moved
    // upper half of the vector.
    const auto SanitizedIndex = [OpSize, Op] {
      switch (OpSize) {
      case 1: return Op->Index - 16;
      case 2: return Op->Index - 8;
      case 4: return Op->Index - 4;
      case 8: return Op->Index - 2;
      default: LOGMAN_MSG_A_FMT("Unhandled OpSize: {}", OpSize); return 0;
      }
    }();

    // Move the value from the now-low-lane data.
    PerformMove(VTMP1, SanitizedIndex);
  }
}

DEF_OP(Float_ToGPR_ZS) {
  auto Op = IROp->C<IR::IROp_Float_ToGPR_ZS>();

  ARMEmitter::Register Dst = GetReg(Node);
  ARMEmitter::VRegister Src = GetVReg(Op->Scalar.ID());

  if (Op->SrcElementSize == 8) {
    fcvtzs(ConvertSize(IROp), Dst, Src.D());
  } else {
    fcvtzs(ConvertSize(IROp), Dst, Src.S());
  }
}

DEF_OP(Float_ToGPR_S) {
  auto Op = IROp->C<IR::IROp_Float_ToGPR_S>();

  ARMEmitter::Register Dst = GetReg(Node);
  ARMEmitter::VRegister Src = GetVReg(Op->Scalar.ID());

  if (Op->SrcElementSize == 8) {
    frinti(VTMP1.D(), Src.D());
    fcvtzs(ConvertSize(IROp), Dst, VTMP1.D());
  } else {
    frinti(VTMP1.S(), Src.S());
    fcvtzs(ConvertSize(IROp), Dst, VTMP1.S());
  }
}

DEF_OP(FCmp) {
  auto Op = IROp->C<IR::IROp_FCmp>();
  const auto EmitSubSize = Op->ElementSize == 8 ? ARMEmitter::ScalarRegSize::i64Bit : ARMEmitter::ScalarRegSize::i32Bit;

  ARMEmitter::VRegister Scalar1 = GetVReg(Op->Scalar1.ID());
  ARMEmitter::VRegister Scalar2 = GetVReg(Op->Scalar2.ID());

  fcmp(EmitSubSize, Scalar1, Scalar2);
}

#undef DEF_OP

} // namespace FEXCore::CPU
