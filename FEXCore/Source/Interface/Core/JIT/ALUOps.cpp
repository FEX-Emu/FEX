// SPDX-License-Identifier: MIT
/*
$info$
tags: backend|arm64
$end_info$
*/

#include "CodeEmitter/Emitter.h"
#include "FEXCore/IR/IR.h"
#include "Interface/Context/Context.h"
#include "Interface/Core/JIT/JITClass.h"
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
  const auto OpSize = IROp->Size;
  if (OpSize == IR::OpSize::i32Bit) {
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
    LOGMAN_THROW_A_FMT(IROp->Size >= IR::OpSize::i32Bit, "Constant not allowed here");
    cmn(EmitSize, Src1, Const);
  } else if (IROp->Size < IR::OpSize::i32Bit) {
    unsigned Shift = 32 - IR::OpSizeAsBits(IROp->Size);

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

DEF_OP(AdcZeroWithFlags) {
  auto Op = IROp->C<IR::IROp_AdcZeroWithFlags>();
  auto Size = ConvertSize48(IROp);

  cset(Size, TMP1, ARMEmitter::Condition::CC_CC);
  adds(Size, GetReg(Node), GetReg(Op->Src1.ID()), TMP1);
}

DEF_OP(AdcZero) {
  auto Op = IROp->C<IR::IROp_AdcZero>();
  auto Size = ConvertSize48(IROp);

  cinc(Size, GetReg(Node), GetReg(Op->Src1.ID()), ARMEmitter::Condition::CC_CC);
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
  if (IROp->Size < IR::OpSize::i32Bit) {
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

    unsigned Shift = 32 - IR::OpSizeAsBits(IROp->Size);
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

DEF_OP(TestZ) {
  auto Op = IROp->C<IR::IROp_TestZ>();
  LOGMAN_THROW_A_FMT(IROp->Size < IR::OpSize::i32Bit, "TestNZ used at higher sizes");
  const auto EmitSize = ARMEmitter::Size::i32Bit;

  uint64_t Const;
  uint64_t Mask = IROp->Size == IR::OpSize::i64Bit ? ~0ULL : ((1ull << IR::OpSizeAsBits(IROp->Size)) - 1);
  auto Src1 = GetReg(Op->Src1.ID());

  if (IsInlineConstant(Op->Src2, &Const)) {
    // We can promote 8/16-bit tests to 32-bit since the constant is masked.
    LOGMAN_THROW_A_FMT(!(Const & ~Mask), "constant is already masked");
    tst(EmitSize, Src1, Const);
  } else {
    const auto Src2 = GetReg(Op->Src2.ID());
    if (Src1 == Src2) {
      tst(EmitSize, Src1 /* Src2 */, Mask);
    } else {
      and_(EmitSize, TMP1, Src1, Src2);
      tst(EmitSize, TMP1, Mask);
    }
  }
}

DEF_OP(SubShift) {
  auto Op = IROp->C<IR::IROp_SubShift>();

  sub(ConvertSize48(IROp), GetReg(Node), GetReg(Op->Src1.ID()), GetReg(Op->Src2.ID()), ConvertIRShiftType(Op->Shift), Op->ShiftAmount);
}

DEF_OP(SubNZCV) {
  auto Op = IROp->C<IR::IROp_SubNZCV>();
  const auto OpSize = IROp->Size;
  const auto EmitSize = ConvertSize(IROp);

  uint64_t Const;
  if (IsInlineConstant(Op->Src2, &Const)) {
    LOGMAN_THROW_A_FMT(OpSize >= IR::OpSize::i32Bit, "Constant not allowed here");
    cmp(EmitSize, GetReg(Op->Src1.ID()), Const);
  } else {
    unsigned Shift = OpSize < IR::OpSize::i32Bit ? (32 - IR::OpSizeAsBits(OpSize)) : 0;
    ARMEmitter::Register ShiftedSrc1 = GetZeroableReg(Op->Src1);

    // Shift to fix flags for <32-bit ops.
    // Any shift of zero is still zero so optimize out silly zero shifts.
    if (OpSize < IR::OpSize::i32Bit && ShiftedSrc1 != ARMEmitter::Reg::zr) {
      lsl(ARMEmitter::Size::i32Bit, TMP1, ShiftedSrc1, Shift);
      ShiftedSrc1 = TMP1;
    }

    if (OpSize < IR::OpSize::i32Bit) {
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
  cmp(EmitSize, GetReg(Op->Src1Lo.ID()), GetReg(Op->Src2Lo.ID()));
  ccmp(EmitSize, GetReg(Op->Src1Hi.ID()), GetReg(Op->Src2Hi.ID()), ARMEmitter::StatusFlags::None, ARMEmitter::Condition::CC_EQ);

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

  const auto OpSize = IROp->Size;
  LOGMAN_THROW_A_FMT(OpSize == IR::OpSize::i8Bit || OpSize == IR::OpSize::i16Bit, "Unsupported {} size: {}", __func__, OpSize);

  if (OpSize == IR::OpSize::i8Bit) {
    setf8(GetReg(Op->Src.ID()).W());
  } else {
    setf16(GetReg(Op->Src.ID()).W());
  }
}

DEF_OP(AXFlag) {
  if (CTX->HostFeatures.SupportsFlagM2) {
    axflag();
  } else {
    // AXFLAG is defined in the Arm spec as
    //
    //   gt: nzCv -> nzCv
    //   lt: Nzcv -> nzcv  <==>  1 + 0
    //   eq: nZCv -> nZCv  <==>  1 + (~0)
    //   un: nzCV -> nZcv  <==>  0 + 0
    //
    // For the latter 3 cases, we therefore get the right NZCV by adding V_inv
    // to (eq ? ~0 : 0). The remaining case is forced with ccmn.
    auto V_inv = GetReg(IROp->Args[0].ID());
    csetm(ARMEmitter::Size::i64Bit, TMP1, ARMEmitter::Condition::CC_EQ);
    ccmn(ARMEmitter::Size::i64Bit, V_inv, TMP1, ARMEmitter::StatusFlags {0x2} /* nzCv */, ARMEmitter::Condition::CC_LE);
  }
}

DEF_OP(Parity) {
  auto Op = IROp->C<IR::IROp_Parity>();
  auto Raw = GetReg(Op->Raw.ID());
  auto Dest = GetReg(Node);

  // Cascade to calculate parity of bottom 8-bits to bottom bit.
  eor(ARMEmitter::Size::i32Bit, TMP1, Raw, Raw, ARMEmitter::ShiftType::LSR, 4);
  eor(ARMEmitter::Size::i32Bit, TMP1, TMP1, TMP1, ARMEmitter::ShiftType::LSR, 2);

  if (Op->Invert) {
    eon(ARMEmitter::Size::i32Bit, Dest, TMP1, TMP1, ARMEmitter::ShiftType::LSR, 1);
  } else {
    eor(ARMEmitter::Size::i32Bit, Dest, TMP1, TMP1, ARMEmitter::ShiftType::LSR, 1);
  }

  // The above sequence leaves garbage in the upper bits.
  if (Op->Mask) {
    and_(ARMEmitter::Size::i32Bit, Dest, Dest, 1);
  }
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
  const auto OpSize = IROp->Size;
  const auto EmitSize = ConvertSize(IROp);

  const auto Dst = GetReg(Node);
  auto Src1 = GetReg(Op->Src1.ID());
  auto Src2 = GetReg(Op->Src2.ID());

  if (OpSize == IR::OpSize::i8Bit) {
    sxtb(EmitSize, TMP1, Src1);
    sxtb(EmitSize, TMP2, Src2);

    Src1 = TMP1;
    Src2 = TMP2;
  } else if (OpSize == IR::OpSize::i16Bit) {
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
  const auto OpSize = IROp->Size;
  const auto EmitSize = ConvertSize(IROp);

  const auto Dst = GetReg(Node);
  auto Src1 = GetReg(Op->Src1.ID());
  auto Src2 = GetReg(Op->Src2.ID());

  if (OpSize == IR::OpSize::i8Bit) {
    uxtb(EmitSize, TMP1, Src1);
    uxtb(EmitSize, TMP2, Src2);

    Src1 = TMP1;
    Src2 = TMP2;
  } else if (OpSize == IR::OpSize::i16Bit) {
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
  const auto OpSize = IROp->Size;
  const auto EmitSize = ConvertSize(IROp);

  const auto Dst = GetReg(Node);
  auto Src1 = GetReg(Op->Src1.ID());
  auto Src2 = GetReg(Op->Src2.ID());

  if (OpSize == IR::OpSize::i8Bit) {
    sxtb(EmitSize, TMP1, Src1);
    sxtb(EmitSize, TMP2, Src2);

    Src1 = TMP1;
    Src2 = TMP2;
  } else if (OpSize == IR::OpSize::i16Bit) {
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
  const auto OpSize = IROp->Size;
  const auto EmitSize = ConvertSize(IROp);

  const auto Dst = GetReg(Node);
  auto Src1 = GetReg(Op->Src1.ID());
  auto Src2 = GetReg(Op->Src2.ID());

  if (OpSize == IR::OpSize::i8Bit) {
    uxtb(EmitSize, TMP1, Src1);
    uxtb(EmitSize, TMP2, Src2);

    Src1 = TMP1;
    Src2 = TMP2;
  } else if (OpSize == IR::OpSize::i16Bit) {
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
  const auto OpSize = IROp->Size;

  LOGMAN_THROW_A_FMT(OpSize == IR::OpSize::i32Bit || OpSize == IR::OpSize::i64Bit, "Unsupported {} size: {}", __func__, OpSize);

  const auto Dst = GetReg(Node);
  const auto Src1 = GetReg(Op->Src1.ID());
  const auto Src2 = GetReg(Op->Src2.ID());

  if (OpSize == IR::OpSize::i32Bit) {
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
  const auto OpSize = IROp->Size;

  LOGMAN_THROW_A_FMT(OpSize == IR::OpSize::i32Bit || OpSize == IR::OpSize::i64Bit, "Unsupported {} size: {}", __func__, OpSize);

  const auto Dst = GetReg(Node);
  const auto Src1 = GetReg(Op->Src1.ID());
  const auto Src2 = GetReg(Op->Src2.ID());

  if (OpSize == IR::OpSize::i32Bit) {
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
  const auto OpSize = IROp->Size;
  const auto EmitSize = ConvertSize(IROp);

  uint64_t Const;
  const auto Dst = GetReg(Node);
  auto Src1 = GetReg(Op->Src1.ID());

  // See TestNZ
  if (OpSize < IR::OpSize::i32Bit) {
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

    unsigned Shift = 32 - IR::OpSizeAsBits(OpSize);
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
  const auto OpSize = IROp->Size;
  const auto EmitSize = ConvertSize(IROp);

  const auto Dst = GetReg(Node);
  const auto Src1 = GetReg(Op->Src1.ID());

  uint64_t Const;
  if (IsInlineConstant(Op->Src2, &Const)) {
    if (OpSize >= IR::OpSize::i32Bit) {
      asr(EmitSize, Dst, Src1, (unsigned int)Const);
    } else {
      sbfx(EmitSize, TMP1, Src1, 0, IR::OpSizeAsBits(OpSize));
      asr(EmitSize, Dst, TMP1, (unsigned int)Const);
      ubfx(EmitSize, Dst, Dst, 0, IR::OpSizeAsBits(OpSize));
    }
  } else {
    const auto Src2 = GetReg(Op->Src2.ID());
    if (OpSize >= IR::OpSize::i32Bit) {
      asrv(EmitSize, Dst, Src1, Src2);
    } else {
      sbfx(EmitSize, TMP1, Src1, 0, IR::OpSizeAsBits(OpSize));
      asrv(EmitSize, Dst, TMP1, Src2);
      ubfx(EmitSize, Dst, Dst, 0, IR::OpSizeAsBits(OpSize));
    }
  }
}

DEF_OP(ShiftFlags) {
  auto Op = IROp->C<IR::IROp_ShiftFlags>();
  const auto OpSize = Op->Size;
  const auto EmitSize = OpSize == IR::OpSize::i64Bit ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;

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

  // We need to mask the source before comparing it. We don't just skip flag
  // updates for Src2=0 but anything that masks to zero.
  and_(ARMEmitter::Size::i32Bit, TMP1, Src2, OpSize == IR::OpSize::i64Bit ? 0x3f : 0x1f);

  ARMEmitter::SingleUseForwardLabel Done;
  cbz(EmitSize, TMP1, &Done);
  {
    // PF/SF/ZF/OF
    if (OpSize >= IR::OpSize::i32Bit) {
      ands(EmitSize, PFTemp, Dst, Dst);
    } else {
      unsigned Shift = 32 - (IR::OpSizeToSize(OpSize) * 8);
      cmn(EmitSize, ARMEmitter::Reg::zr, Dst, ARMEmitter::ShiftType::LSL, Shift);
      mov(ARMEmitter::Size::i64Bit, PFTemp, Dst);
    }

    auto CFWord = TMP1;
    unsigned CFBit = 0;

    // Extract the last bit shifted in to CF
    if (Op->Shift == IR::ShiftType::LSL) {
      if (OpSize >= IR::OpSize::i32Bit) {
        neg(EmitSize, CFWord, Src2);
        lsrv(EmitSize, CFWord, Src1, CFWord);
      } else {
        CFWord = Dst.X();
        CFBit = IR::OpSizeToSize(OpSize) * 8;
      }
    } else {
      sub(ARMEmitter::Size::i64Bit, CFWord, Src2, 1);
      lsrv(EmitSize, CFWord, Src1, CFWord);
    }

    if (Op->InvertCF) {
      mvn(ARMEmitter::Size::i64Bit, TMP1, CFWord);
      CFWord = TMP1;
    }

    bool SetOF = Op->Shift != IR::ShiftType::ASR;
    if (SetOF) {
      // Only defined when Shift is 1 else undefined
      // OF flag is set if a sign change occurred
      eor(EmitSize, TMP3, Src1, Dst);
    }

    if (CTX->HostFeatures.SupportsFlagM) {
      rmif(CFWord, (CFBit - 1) % 64, (1 << 1) /* C */);

      if (SetOF) {
        rmif(TMP3, IR::OpSizeToSize(OpSize) * 8 - 1, (1 << 0) /* V */);
      }
    } else {
      mrs(TMP2, ARMEmitter::SystemRegister::NZCV);

      if (CFBit != 0) {
        lsr(ARMEmitter::Size::i64Bit, TMP1, CFWord, CFBit);
        CFWord = TMP1;
      }

      bfi(ARMEmitter::Size::i32Bit, TMP2, CFWord, 29 /* C */, 1);

      if (SetOF) {
        lsr(EmitSize, TMP3, TMP3, IR::OpSizeToSize(OpSize) * 8 - 1);
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

DEF_OP(RotateFlags) {
  auto Op = IROp->C<IR::IROp_RotateFlags>();
  const auto Result = GetReg(Op->Result.ID());
  const auto Shift = GetReg(Op->Shift.ID());
  const bool Left = Op->Left;
  const auto EmitSize = Op->Size == IR::OpSize::i64Bit ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;

  // If shift=0, flags are unaffected. Wrap the whole implementation in a cbz.
  ARMEmitter::SingleUseForwardLabel Done;
  cbz(EmitSize, Shift, &Done);
  {
    // Extract the last bit shifted in to CF
    const auto BitSize = IR::OpSizeToSize(Op->Size) * 8;
    unsigned CFBit = Left ? 0 : BitSize - 1;

    // For ROR, OF is the XOR of the new CF bit and the most significant bit of the result.
    // For ROL, OF is the LSB and MSB XOR'd together.
    // OF is architecturally only defined for 1-bit rotate.
    eor(ARMEmitter::Size::i64Bit, TMP1, Result, Result, ARMEmitter::ShiftType::LSR, Left ? BitSize - 1 : 1);
    unsigned OFBit = Left ? 0 : BitSize - 2;

    // Invert result so we get inverted carry.
    mvn(ARMEmitter::Size::i64Bit, TMP2, Result);

    if (CTX->HostFeatures.SupportsFlagM) {
      rmif(TMP2, (CFBit - 1) % 64, 1 << 1 /* nzCv */);
      rmif(TMP1, OFBit, 1 << 0 /* nzcV */);
    } else {
      if (OFBit != 0) {
        lsr(EmitSize, TMP1, TMP1, OFBit);
      }
      if (CFBit != 0) {
        lsr(EmitSize, TMP2, TMP2, CFBit);
      }

      mrs(TMP3, ARMEmitter::SystemRegister::NZCV);
      bfi(ARMEmitter::Size::i32Bit, TMP3, TMP1, 28 /* V */, 1);
      bfi(ARMEmitter::Size::i32Bit, TMP3, TMP2, 29 /* C */, 1);
      msr(ARMEmitter::SystemRegister::NZCV, TMP3);
    }
  }
  Bind(&Done);
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

  // We can't clobber these
  const auto OrigInput = GetReg(Op->Input.ID());
  const auto OrigMask = GetReg(Op->Mask.ID());

  if (CTX->HostFeatures.SupportsSVEBitPerm) {
    // SVE added support for PDEP but it needs to be done in a vector register.
    if (EmitSize == ARMEmitter::Size::i32Bit) {
      fmov(ARMEmitter::Size::i32Bit, VTMP1.S(), OrigInput.W());
      fmov(ARMEmitter::Size::i32Bit, VTMP2.S(), OrigMask.W());
      bdep(ARMEmitter::SubRegSize::i32Bit, VTMP1.Z(), VTMP1.Z(), VTMP2.Z());
      umov<ARMEmitter::SubRegSize::i32Bit>(Dest, VTMP1, 0);
    } else {
      fmov(ARMEmitter::Size::i64Bit, VTMP1.D(), OrigInput.X());
      fmov(ARMEmitter::Size::i64Bit, VTMP2.D(), OrigMask.X());
      bdep(ARMEmitter::SubRegSize::i64Bit, VTMP1.Z(), VTMP1.Z(), VTMP2.Z());
      umov<ARMEmitter::SubRegSize::i64Bit>(Dest, VTMP1, 0);
    }
  } else {
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
}

DEF_OP(PExt) {
  auto Op = IROp->C<IR::IROp_PExt>();
  const auto OpSize = IROp->Size;
  const auto OpSizeBitsM1 = IR::OpSizeAsBits(OpSize) - 1;
  const auto EmitSize = ConvertSize48(IROp);

  const auto Input = GetReg(Op->Input.ID());
  const auto Mask = GetReg(Op->Mask.ID());
  const auto Dest = GetReg(Node);

  if (CTX->HostFeatures.SupportsSVEBitPerm) {
    // SVE added support for PEXT but it needs to be done in a vector register.
    if (EmitSize == ARMEmitter::Size::i32Bit) {
      fmov(ARMEmitter::Size::i32Bit, VTMP1.S(), Input.W());
      fmov(ARMEmitter::Size::i32Bit, VTMP2.S(), Mask.W());
      bext(ARMEmitter::SubRegSize::i32Bit, VTMP1.Z(), VTMP1.Z(), VTMP2.Z());
      umov<ARMEmitter::SubRegSize::i32Bit>(Dest, VTMP1, 0);
    } else {
      fmov(ARMEmitter::Size::i64Bit, VTMP1.D(), Input.X());
      fmov(ARMEmitter::Size::i64Bit, VTMP2.D(), Mask.X());
      bext(ARMEmitter::SubRegSize::i64Bit, VTMP1.Z(), VTMP1.Z(), VTMP2.Z());
      umov<ARMEmitter::SubRegSize::i64Bit>(Dest, VTMP1, 0);
    }
  } else {
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
}

DEF_OP(LDiv) {
  auto Op = IROp->C<IR::IROp_LDiv>();
  const auto OpSize = IROp->Size;
  const auto EmitSize = OpSize >= IR::OpSize::i32Bit ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;

  const auto Dst = GetReg(Node);
  const auto Upper = GetReg(Op->Upper.ID());
  const auto Lower = GetReg(Op->Lower.ID());
  const auto Divisor = GetReg(Op->Divisor.ID());

  // Each source is OpSize in size
  // So you can have up to a 128bit divide from x86-64
  switch (OpSize) {
  case IR::OpSize::i16Bit: {
    uxth(EmitSize, TMP1, Lower);
    bfi(EmitSize, TMP1, Upper, 16, 16);
    sxth(EmitSize, TMP2, Divisor);
    sdiv(EmitSize, Dst, TMP1, TMP2);
    break;
  }
  case IR::OpSize::i32Bit: {
    // TODO: 32-bit operation should be guaranteed not to leave garbage in the upper bits.
    mov(EmitSize, TMP1, Lower);
    bfi(EmitSize, TMP1, Upper, 32, 32);
    sxtw(TMP2, Divisor.W());
    sdiv(EmitSize, Dst, TMP1, TMP2);
    break;
  }
  case IR::OpSize::i64Bit: {
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
  const auto OpSize = IROp->Size;
  const auto EmitSize = OpSize >= IR::OpSize::i32Bit ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;

  const auto Dst = GetReg(Node);
  const auto Upper = GetReg(Op->Upper.ID());
  const auto Lower = GetReg(Op->Lower.ID());
  const auto Divisor = GetReg(Op->Divisor.ID());

  // Each source is OpSize in size
  // So you can have up to a 128bit divide from x86-64=
  switch (OpSize) {
  case IR::OpSize::i16Bit: {
    uxth(EmitSize, TMP1, Lower);
    bfi(EmitSize, TMP1, Upper, 16, 16);
    udiv(EmitSize, Dst, TMP1, Divisor);
    break;
  }
  case IR::OpSize::i32Bit: {
    // TODO: 32-bit operation should be guaranteed not to leave garbage in the upper bits.
    mov(EmitSize, TMP1, Lower);
    bfi(EmitSize, TMP1, Upper, 32, 32);
    udiv(EmitSize, Dst, TMP1, Divisor);
    break;
  }
  case IR::OpSize::i64Bit: {
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
  const auto OpSize = IROp->Size;
  const auto EmitSize = OpSize >= IR::OpSize::i32Bit ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;

  const auto Dst = GetReg(Node);
  const auto Upper = GetReg(Op->Upper.ID());
  const auto Lower = GetReg(Op->Lower.ID());
  const auto Divisor = GetReg(Op->Divisor.ID());

  // Each source is OpSize in size
  // So you can have up to a 128bit divide from x86-64
  switch (OpSize) {
  case IR::OpSize::i16Bit: {
    uxth(EmitSize, TMP1, Lower);
    bfi(EmitSize, TMP1, Upper, 16, 16);
    sxth(EmitSize, TMP2, Divisor);
    sdiv(EmitSize, TMP3, TMP1, TMP2);
    msub(EmitSize, Dst, TMP3, TMP2, TMP1);
    break;
  }
  case IR::OpSize::i32Bit: {
    // TODO: 32-bit operation should be guaranteed not to leave garbage in the upper bits.
    mov(EmitSize, TMP1, Lower);
    bfi(EmitSize, TMP1, Upper, 32, 32);
    sxtw(TMP3, Divisor.W());
    sdiv(EmitSize, TMP2, TMP1, TMP3);
    msub(EmitSize, Dst, TMP2, TMP3, TMP1);
    break;
  }
  case IR::OpSize::i64Bit: {
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
  const auto OpSize = IROp->Size;
  const auto EmitSize = OpSize >= IR::OpSize::i32Bit ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;

  const auto Dst = GetReg(Node);
  const auto Upper = GetReg(Op->Upper.ID());
  const auto Lower = GetReg(Op->Lower.ID());
  const auto Divisor = GetReg(Op->Divisor.ID());

  // Each source is OpSize in size
  // So you can have up to a 128bit divide from x86-64
  switch (OpSize) {
  case IR::OpSize::i16Bit: {
    uxth(EmitSize, TMP1, Lower);
    bfi(EmitSize, TMP1, Upper, 16, 16);
    udiv(EmitSize, TMP2, TMP1, Divisor);
    msub(EmitSize, Dst, TMP2, Divisor, TMP1);
    break;
  }
  case IR::OpSize::i32Bit: {
    // TODO: 32-bit operation should be guaranteed not to leave garbage in the upper bits.
    mov(EmitSize, TMP1, Lower);
    bfi(EmitSize, TMP1, Upper, 32, 32);
    udiv(EmitSize, TMP2, TMP1, Divisor);
    msub(EmitSize, Dst, TMP2, Divisor, TMP1);
    break;
  }
  case IR::OpSize::i64Bit: {
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
  const auto OpSize = IROp->Size;

  const auto Dst = GetReg(Node);
  const auto Src = GetReg(Op->Src.ID());

  switch (OpSize) {
  case IR::OpSize::i8Bit:
    fmov(ARMEmitter::Size::i32Bit, VTMP1.S(), Src);
    // only use lowest byte
    cnt(ARMEmitter::SubRegSize::i8Bit, VTMP1.D(), VTMP1.D());
    break;
  case IR::OpSize::i16Bit:
    fmov(ARMEmitter::Size::i32Bit, VTMP1.S(), Src);
    cnt(ARMEmitter::SubRegSize::i8Bit, VTMP1.D(), VTMP1.D());
    // only count two lowest bytes
    addp(ARMEmitter::SubRegSize::i8Bit, VTMP1.D(), VTMP1.D(), VTMP1.D());
    break;
  case IR::OpSize::i32Bit:
    fmov(ARMEmitter::Size::i32Bit, VTMP1.S(), Src);
    cnt(ARMEmitter::SubRegSize::i8Bit, VTMP1.D(), VTMP1.D());
    // fmov has zero extended, unused bytes are zero
    addv(ARMEmitter::SubRegSize::i8Bit, VTMP1.D(), VTMP1.D());
    break;
  case IR::OpSize::i64Bit:
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

  // We assume the source is nonzero, so we can just rbit+clz without worrying
  // about upper garbage for smaller types.
  rbit(EmitSize, TMP1, Src);
  clz(EmitSize, Dst, TMP1);
}

DEF_OP(FindMSB) {
  auto Op = IROp->C<IR::IROp_FindMSB>();
  const auto OpSize = IROp->Size;

  LOGMAN_THROW_A_FMT(OpSize == IR::OpSize::i16Bit || OpSize == IR::OpSize::i32Bit || OpSize == IR::OpSize::i64Bit,
                     "Unsupported {} size: {}", __func__, OpSize);
  const auto EmitSize = ConvertSize(IROp);

  const auto Dst = GetReg(Node);
  const auto Src = GetReg(Op->Src.ID());

  movz(ARMEmitter::Size::i64Bit, TMP1, IR::OpSizeAsBits(OpSize) - 1);

  if (OpSize == IR::OpSize::i16Bit) {
    lsl(EmitSize, Dst, Src, 16);
    clz(EmitSize, Dst, Dst);
  } else {
    clz(EmitSize, Dst, Src);
  }

  sub(ARMEmitter::Size::i64Bit, Dst, TMP1, Dst);
}

DEF_OP(FindTrailingZeroes) {
  auto Op = IROp->C<IR::IROp_FindTrailingZeroes>();
  const auto OpSize = IROp->Size;

  LOGMAN_THROW_A_FMT(OpSize == IR::OpSize::i16Bit || OpSize == IR::OpSize::i32Bit || OpSize == IR::OpSize::i64Bit,
                     "Unsupported {} size: {}", __func__, OpSize);
  const auto EmitSize = ConvertSize(IROp);

  const auto Dst = GetReg(Node);
  const auto Src = GetReg(Op->Src.ID());

  rbit(EmitSize, Dst, Src);

  if (OpSize == IR::OpSize::i16Bit) {
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
  const auto OpSize = IROp->Size;

  LOGMAN_THROW_A_FMT(OpSize == IR::OpSize::i16Bit || OpSize == IR::OpSize::i32Bit || OpSize == IR::OpSize::i64Bit,
                     "Unsupported {} size: {}", __func__, OpSize);
  const auto EmitSize = ConvertSize(IROp);

  const auto Dst = GetReg(Node);
  const auto Src = GetReg(Op->Src.ID());

  if (OpSize == IR::OpSize::i16Bit) {
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
  const auto OpSize = IROp->Size;

  LOGMAN_THROW_A_FMT(OpSize == IR::OpSize::i16Bit || OpSize == IR::OpSize::i32Bit || OpSize == IR::OpSize::i64Bit,
                     "Unsupported {} size: {}", __func__, OpSize);
  const auto EmitSize = ConvertSize(IROp);

  const auto Dst = GetReg(Node);
  const auto Src = GetReg(Op->Src.ID());

  rev(EmitSize, Dst, Src);
  if (OpSize == IR::OpSize::i16Bit) {
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

    if (IROp->Size >= IR::OpSize::i32Bit) {
      mov(EmitSize, Dst, TMP1.R());
    } else {
      ubfx(EmitSize, Dst, TMP1, 0, IR::OpSizeAsBits(IROp->Size));
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
  LOGMAN_THROW_A_FMT(IROp->Size <= IR::OpSize::i64Bit, "OpSize is too large for BFE: {}", IROp->Size);
  LOGMAN_THROW_A_FMT(Op->Width != 0, "Invalid BFE width of 0");
  const auto EmitSize = ConvertSize(IROp);

  const auto Dst = GetReg(Node);
  const auto Src = GetReg(Op->Src.ID());

  if (Op->lsb == 0 && Op->Width == 32) {
    mov(ARMEmitter::Size::i32Bit, Dst, Src);
  } else if (Op->lsb == 0 && Op->Width == 64) {
    LOGMAN_THROW_A_FMT(IROp->Size == IR::OpSize::i64Bit, "Must be 64-bit wide register");
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
  const auto OpSize = IROp->Size;
  const auto EmitSize = ConvertSize(IROp);
  const auto CompareEmitSize = Op->CompareSize == IR::OpSize::i64Bit ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;

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
  } else if (IsFPR(Op->Cmp1.ID())) {
    const auto Src1 = GetVReg(Op->Cmp1.ID());
    const auto Src2 = GetVReg(Op->Cmp2.ID());
    fcmp(Op->CompareSize == IR::OpSize::i64Bit ? ARMEmitter::ScalarRegSize::i64Bit : ARMEmitter::ScalarRegSize::i32Bit, Src1, Src2);
  } else {
    LOGMAN_MSG_A_FMT("Select: Expected GPR or FPR");
  }

  uint64_t const_true, const_false;
  bool is_const_true = IsInlineConstant(Op->TrueVal, &const_true);
  bool is_const_false = IsInlineConstant(Op->FalseVal, &const_false);

  uint64_t all_ones = OpSize == IR::OpSize::i64Bit ? 0xffff'ffff'ffff'ffffull : 0xffff'ffffull;

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

  uint64_t all_ones = IROp->Size == IR::OpSize::i64Bit ? 0xffff'ffff'ffff'ffffull : 0xffff'ffffull;

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

DEF_OP(NZCVSelectV) {
  auto Op = IROp->C<IR::IROp_NZCVSelectV>();

  auto cc = MapCC(Op->Cond);
  const auto SubRegSize = ConvertSubRegSizePair248(IROp);
  fcsel(SubRegSize.Scalar, GetVReg(Node), GetVReg(Op->TrueVal.ID()), GetVReg(Op->FalseVal.ID()), cc);
}

DEF_OP(NZCVSelectIncrement) {
  auto Op = IROp->C<IR::IROp_NZCVSelectIncrement>();

  csinc(ConvertSize(IROp), GetReg(Node), GetReg(Op->TrueVal.ID()), GetZeroableReg(Op->FalseVal), MapCC(Op->Cond));
}

DEF_OP(VExtractToGPR) {
  const auto Op = IROp->C<IR::IROp_VExtractToGPR>();
  const auto OpSize = IROp->Size;

  constexpr auto AVXRegBitSize = Core::CPUState::XMM_AVX_REG_SIZE * 8;
  constexpr auto SSERegBitSize = Core::CPUState::XMM_SSE_REG_SIZE * 8;
  const auto ElementSizeBits = IR::OpSizeAsBits(Op->Header.ElementSize);

  const auto Offset = ElementSizeBits * Op->Index;
  const auto Is256Bit = Offset >= SSERegBitSize;
  LOGMAN_THROW_A_FMT(!Is256Bit || (Is256Bit && HostSupportsSVE256), "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetReg(Node);
  const auto Vector = GetVReg(Op->Vector.ID());

  const auto PerformMove = [&](const ARMEmitter::VRegister reg, int index) {
    switch (OpSize) {
    case IR::OpSize::i8Bit: umov<ARMEmitter::SubRegSize::i8Bit>(Dst, Vector, index); break;
    case IR::OpSize::i16Bit: umov<ARMEmitter::SubRegSize::i16Bit>(Dst, Vector, index); break;
    case IR::OpSize::i32Bit: umov<ARMEmitter::SubRegSize::i32Bit>(Dst, Vector, index); break;
    case IR::OpSize::i64Bit: umov<ARMEmitter::SubRegSize::i64Bit>(Dst, Vector, index); break;
    default: LOGMAN_MSG_A_FMT("Unhandled ExtractElementSize: {}", OpSize); break;
    }
  };

  if (Offset < SSERegBitSize) {
    // Desired data lies within the lower 128-bit lane, so we
    // can treat the operation as a 128-bit operation, even
    // when acting on larger register sizes.
    PerformMove(Vector, Op->Index);
  } else {
    LOGMAN_THROW_A_FMT(Is256Bit, "Can't perform 256-bit extraction with op side: {}", OpSize);
    LOGMAN_THROW_A_FMT(Offset < AVXRegBitSize, "Trying to extract element outside bounds of register. Offset={}, Index={}", Offset, Op->Index);

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
      case IR::OpSize::i8Bit: return Op->Index - 16;
      case IR::OpSize::i16Bit: return Op->Index - 8;
      case IR::OpSize::i32Bit: return Op->Index - 4;
      case IR::OpSize::i64Bit: return Op->Index - 2;
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

  if (Op->SrcElementSize == IR::OpSize::i64Bit) {
    fcvtzs(ConvertSize(IROp), Dst, Src.D());
  } else {
    fcvtzs(ConvertSize(IROp), Dst, Src.S());
  }
}

DEF_OP(Float_ToGPR_S) {
  auto Op = IROp->C<IR::IROp_Float_ToGPR_S>();

  ARMEmitter::Register Dst = GetReg(Node);
  ARMEmitter::VRegister Src = GetVReg(Op->Scalar.ID());

  if (Op->SrcElementSize == IR::OpSize::i64Bit) {
    frinti(VTMP1.D(), Src.D());
    fcvtzs(ConvertSize(IROp), Dst, VTMP1.D());
  } else {
    frinti(VTMP1.S(), Src.S());
    fcvtzs(ConvertSize(IROp), Dst, VTMP1.S());
  }
}

DEF_OP(FCmp) {
  auto Op = IROp->C<IR::IROp_FCmp>();
  const auto EmitSubSize = Op->ElementSize == IR::OpSize::i64Bit ? ARMEmitter::ScalarRegSize::i64Bit : ARMEmitter::ScalarRegSize::i32Bit;

  ARMEmitter::VRegister Scalar1 = GetVReg(Op->Scalar1.ID());
  ARMEmitter::VRegister Scalar2 = GetVReg(Op->Scalar2.ID());

  fcmp(EmitSubSize, Scalar1, Scalar2);
}

#undef DEF_OP

} // namespace FEXCore::CPU
