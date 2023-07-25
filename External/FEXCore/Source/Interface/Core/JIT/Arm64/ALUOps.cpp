/*
$info$
tags: backend|arm64
$end_info$
*/

#include "Interface/Context/Context.h"
#include "Interface/Core/ArchHelpers/CodeEmitter/Emitter.h"
#include "Interface/Core/ArchHelpers/CodeEmitter/Registers.h"
#include "Interface/Core/JIT/Arm64/JITClass.h"
#include "Interface/IR/Passes/RegisterAllocationPass.h"

namespace FEXCore::CPU {

#define GRD(Node) (IROp->Size <= 4 ? GetDst<RA_32>(Node) : GetDst<RA_64>(Node))
#define GRS(Node) (IROp->Size <= 4 ? GetReg<RA_32>(Node) : GetReg<RA_64>(Node))

#define DEF_OP(x) void Arm64JITCore::Op_##x(IR::IROp_Header const *IROp, IR::NodeID Node)
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
    default:
      LOGMAN_MSG_A_FMT("Unhandled Truncation size: {}", IROp->Size);
      break;
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
  //nop
}

DEF_OP(InlineEntrypointOffset) {
  //nop
}

DEF_OP(CycleCounter) {
#ifdef DEBUG_CYCLES
  movz(ARMEmitter::Size::i64Bit, GetReg(Node), 0);
#else
  mrs(GetReg(Node), ARMEmitter::SystemRegister::CNTVCT_EL0);
#endif
}

DEF_OP(Add) {
  auto Op = IROp->C<IR::IROp_Add>();
  const uint8_t OpSize = IROp->Size;

  LOGMAN_THROW_AA_FMT(OpSize == 4 || OpSize == 8, "Unsupported {} size: {}", __func__, OpSize);
  const auto EmitSize = OpSize == 8 ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;

  uint64_t Const;
  if (IsInlineConstant(Op->Src2, &Const)) {
    add(EmitSize, GetReg(Node), GetReg(Op->Src1.ID()), Const);
  } else {
    add(EmitSize, GetReg(Node), GetReg(Op->Src1.ID()), GetReg(Op->Src2.ID()));
  }
}

DEF_OP(Sub) {
  auto Op = IROp->C<IR::IROp_Sub>();
  const uint8_t OpSize = IROp->Size;

  LOGMAN_THROW_AA_FMT(OpSize == 4 || OpSize == 8, "Unsupported {} size: {}", __func__, OpSize);
  const auto EmitSize = OpSize == 8 ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;

  uint64_t Const;
  if (IsInlineConstant(Op->Src2, &Const)) {
    sub(EmitSize, GetReg(Node), GetReg(Op->Src1.ID()), Const);
  } else {
    sub(EmitSize, GetReg(Node), GetReg(Op->Src1.ID()), GetReg(Op->Src2.ID()));
  }
}

DEF_OP(Neg) {
  auto Op = IROp->C<IR::IROp_Neg>();
  const uint8_t OpSize = IROp->Size;

  LOGMAN_THROW_AA_FMT(OpSize == 4 || OpSize == 8, "Unsupported {} size: {}", __func__, OpSize);
  const auto EmitSize = OpSize == 8 ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;

  neg(EmitSize, GetReg(Node), GetReg(Op->Src.ID()));
}

DEF_OP(Abs) {
  auto Op = IROp->C<IR::IROp_Abs>();
  const uint8_t OpSize = IROp->Size;

  LOGMAN_THROW_AA_FMT(OpSize == 4 || OpSize == 8, "Unsupported {} size: {}", __func__, OpSize);
  const auto EmitSize = OpSize == 8 ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;

  const auto Dst = GetReg(Node);
  auto Src = GetReg(Op->Src.ID());

  if (CTX->HostFeatures.SupportsCSSC) {
    // On CSSC supporting processors, this turns in to one instruction and doesn't modify flags.
    abs(EmitSize, Dst, Src);
  }
  else {
    cmp(EmitSize, Src, 0);
    cneg(EmitSize, Dst, Src, ARMEmitter::Condition::CC_MI);
  }
}

DEF_OP(Mul) {
  auto Op = IROp->C<IR::IROp_Mul>();
  const uint8_t OpSize = IROp->Size;

  LOGMAN_THROW_AA_FMT(OpSize == 4 || OpSize == 8, "Unsupported {} size: {}", __func__, OpSize);
  const auto EmitSize = OpSize == 8 ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;

  mul(EmitSize, GetReg(Node), GetReg(Op->Src1.ID()), GetReg(Op->Src2.ID()));
}

DEF_OP(UMul) {
  auto Op = IROp->C<IR::IROp_UMul>();
  const uint8_t OpSize = IROp->Size;

  LOGMAN_THROW_AA_FMT(OpSize == 4 || OpSize == 8, "Unsupported {} size: {}", __func__, OpSize);
  const auto EmitSize = OpSize == 8 ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;

  mul(EmitSize, GetReg(Node), GetReg(Op->Src1.ID()), GetReg(Op->Src2.ID()));
}

DEF_OP(Div) {
  auto Op = IROp->C<IR::IROp_Div>();

  // Each source is OpSize in size
  // So you can have up to a 128bit divide from x86-64
  const uint8_t OpSize = IROp->Size;

  const auto Dst = GetReg(Node);
  auto Src1 = GetReg(Op->Src1.ID());
  auto Src2 = GetReg(Op->Src2.ID());

  const auto EmitSize = OpSize == 8 ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;

  if (OpSize == 1) {
    sxtb(EmitSize, TMP1, Src1);
    sxtb(EmitSize, TMP2, Src2);

    Src1 = TMP1;
    Src2 = TMP2;
  }
  else if (OpSize == 2) {
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

  const auto Dst = GetReg(Node);
  auto Src1 = GetReg(Op->Src1.ID());
  auto Src2 = GetReg(Op->Src2.ID());

  const auto EmitSize = OpSize == 8 ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;

  if (OpSize == 1) {
    uxtb(EmitSize, TMP1, Src1);
    uxtb(EmitSize, TMP2, Src2);

    Src1 = TMP1;
    Src2 = TMP2;
  }
  else if (OpSize == 2) {
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

  const auto Dst = GetReg(Node);
  auto Src1 = GetReg(Op->Src1.ID());
  auto Src2 = GetReg(Op->Src2.ID());

  const auto EmitSize = OpSize == 8 ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;

  if (OpSize == 1) {
    sxtb(EmitSize, TMP1, Src1);
    sxtb(EmitSize, TMP2, Src2);

    Src1 = TMP1;
    Src2 = TMP2;
  }
  else if (OpSize == 2) {
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

  const auto Dst = GetReg(Node);
  auto Src1 = GetReg(Op->Src1.ID());
  auto Src2 = GetReg(Op->Src2.ID());

  const auto EmitSize = OpSize == 8 ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;
  if (OpSize == 1) {
    uxtb(EmitSize, TMP1, Src1);
    uxtb(EmitSize, TMP2, Src2);

    Src1 = TMP1;
    Src2 = TMP2;
  }
  else if (OpSize == 2) {
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
  }
  else {
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
  }
  else {
    umulh(Dst.X(), Src1.X(), Src2.X());
  }
}

DEF_OP(Or) {
  auto Op = IROp->C<IR::IROp_Or>();
  const uint8_t OpSize = IROp->Size;
  const auto EmitSize = OpSize == 8 ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;

  const auto Dst = GetReg(Node);
  const auto Src1 = GetReg(Op->Src1.ID());

  uint64_t Const;
  if (IsInlineConstant(Op->Src2, &Const)) {
    orr(EmitSize, Dst, Src1, Const);
  } else {
    const auto Src2 = GetReg(Op->Src2.ID());
    orr(EmitSize, Dst, Src1, Src2);
  }
}

DEF_OP(Orlshl) {
  auto Op = IROp->C<IR::IROp_Orlshl>();
  const uint8_t OpSize = IROp->Size;
  const auto EmitSize = OpSize == 8 ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;

  const auto Dst = GetReg(Node);
  const auto Src1 = GetReg(Op->Src1.ID());

  uint64_t Const;
  if (IsInlineConstant(Op->Src2, &Const)) {
    orr(EmitSize, Dst, Src1, Const << Op->BitShift);
  } else {
    const auto Src2 = GetReg(Op->Src2.ID());
    orr(EmitSize, Dst, Src1, Src2, ARMEmitter::ShiftType::LSL, Op->BitShift);
  }
}

DEF_OP(And) {
  auto Op = IROp->C<IR::IROp_And>();
  const uint8_t OpSize = IROp->Size;
  const auto EmitSize = OpSize == 8 ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;

  const auto Dst = GetReg(Node);
  const auto Src1 = GetReg(Op->Src1.ID());

  uint64_t Const;
  if (IsInlineConstant(Op->Src2, &Const)) {
    and_(EmitSize, Dst, Src1, Const);
  } else {
    const auto Src2 = GetReg(Op->Src2.ID());
    and_(EmitSize, Dst, Src1, Src2);
  }
}

DEF_OP(Andn) {
  auto Op = IROp->C<IR::IROp_Andn>();
  const uint8_t OpSize = IROp->Size;
  const auto EmitSize = OpSize == 8 ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;

  const auto Dst = GetReg(Node);
  const auto Src1 = GetReg(Op->Src1.ID());

  uint64_t Const;
  if (IsInlineConstant(Op->Src2, &Const)) {
    bic(EmitSize, Dst, Src1, Const);
  } else {
    const auto Src2 = GetReg(Op->Src2.ID());
    bic(EmitSize, Dst, Src1, Src2);
  }
}

DEF_OP(Xor) {
  auto Op = IROp->C<IR::IROp_Xor>();
  const uint8_t OpSize = IROp->Size;
  const auto EmitSize = OpSize == 8 ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;

  const auto Dst = GetReg(Node);
  const auto Src1 = GetReg(Op->Src1.ID());

  uint64_t Const;
  if (IsInlineConstant(Op->Src2, &Const)) {
    eor(EmitSize, Dst, Src1, Const);
  } else {
    const auto Src2 = GetReg(Op->Src2.ID());
    eor(EmitSize, Dst, Src1, Src2);
  }
}

DEF_OP(Lshl) {
  auto Op = IROp->C<IR::IROp_Lshl>();
  const uint8_t OpSize = IROp->Size;
  const auto EmitSize = OpSize == 8 ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;

  const auto Dst = GetReg(Node);
  const auto Src1 = GetReg(Op->Src1.ID());

  uint64_t Const;
  if (IsInlineConstant(Op->Src2, &Const)) {
    lsl(EmitSize, Dst, Src1, Const);
  } else {
    const auto Src2 = GetReg(Op->Src2.ID());
    lslv(EmitSize, Dst, Src1, Src2);
  }
}

DEF_OP(Lshr) {
  auto Op = IROp->C<IR::IROp_Lshr>();

  const uint8_t OpSize = IROp->Size;
  const auto EmitSize = OpSize == 8 ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;

  const auto Dst = GetReg(Node);
  const auto Src1 = GetReg(Op->Src1.ID());

  uint64_t Const;
  if (IsInlineConstant(Op->Src2, &Const)) {
    lsr(EmitSize, Dst, Src1, Const);
  } else {
    const auto Src2 = GetReg(Op->Src2.ID());
    lsrv(EmitSize, Dst, Src1, Src2);
  }
}

DEF_OP(Ashr) {
  auto Op = IROp->C<IR::IROp_Ashr>();
  const uint8_t OpSize = IROp->Size;
  const auto EmitSize = OpSize == 8 ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;

  const auto Dst = GetReg(Node);
  const auto Src1 = GetReg(Op->Src1.ID());

  uint64_t Const;
  if (IsInlineConstant(Op->Src2, &Const)) {
    if (OpSize >= 4) {
      asr(EmitSize, Dst, Src1, (unsigned int)Const);
    }
    else {
      sbfx(EmitSize, TMP1, Src1, 0, OpSize * 8);
      asr(EmitSize, Dst, TMP1, (unsigned int)Const);
      ubfx(EmitSize, Dst, Dst, 0, OpSize * 8);
    }
  } else {
    const auto Src2 = GetReg(Op->Src2.ID());
    if (OpSize >= 4) {
      asrv(EmitSize, Dst, Src1, Src2);
    }
    else {
      sbfx(EmitSize, TMP1, Src1, 0, OpSize * 8);
      asrv(EmitSize, Dst, TMP1, Src2);
      ubfx(EmitSize, Dst, Dst, 0, OpSize * 8);
    }
  }
}

DEF_OP(Ror) {
  auto Op = IROp->C<IR::IROp_Ror>();
  const uint8_t OpSize = IROp->Size;

  LOGMAN_THROW_AA_FMT(OpSize == 4 || OpSize == 8, "Unsupported {} size: {}", __func__, OpSize);
  const auto EmitSize = OpSize == 8 ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;

  const auto Dst = GetReg(Node);
  const auto Src1 = GetReg(Op->Src1.ID());

  uint64_t Const;
  if (IsInlineConstant(Op->Src2, &Const)) {
    ror(EmitSize, Dst, Src1, Const);
  } else {
    const auto Src2 = GetReg(Op->Src2.ID());
    rorv(EmitSize, Dst, Src1, Src2);
  }
}

DEF_OP(Extr) {
  auto Op = IROp->C<IR::IROp_Extr>();
  const uint8_t OpSize = IROp->Size;

  LOGMAN_THROW_AA_FMT(OpSize == 4 || OpSize == 8, "Unsupported {} size: {}", __func__, OpSize);
  const auto EmitSize = OpSize == 8 ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;

  const auto Dst = GetReg(Node);
  const auto Upper = GetReg(Op->Upper.ID());
  const auto Lower = GetReg(Op->Lower.ID());

  extr(EmitSize, Dst, Upper, Lower, Op->LSB);
}

DEF_OP(PDep) {
  auto Op = IROp->C<IR::IROp_PExt>();
  const auto OpSize = IROp->Size;

  LOGMAN_THROW_AA_FMT(OpSize == 4 || OpSize == 8, "Unsupported {} size: {}", __func__, OpSize);
  const auto EmitSize = OpSize == 8 ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;

  const auto Input = GetReg(Op->Input.ID());
  const auto Mask = GetReg(Op->Mask.ID());
  const auto Dest = GetReg(Node);

  const auto ShiftedBitReg = TMP1.R();
  const auto BitReg        = TMP2.R();
  const auto SubMaskReg    = TMP3.R();
  const auto IndexReg      = TMP4.R();
  const auto ZeroReg       = ARMEmitter::Reg::zr;

  const auto InputReg = StaticRegisters[0];
  const auto MaskReg  = StaticRegisters[1];
  const auto DestReg  = StaticRegisters[2];

  const auto SpillCode = 1U << InputReg.Idx() |
                         1U << MaskReg.Idx() |
                         1U << DestReg.Idx();

  ARMEmitter::ForwardLabel EarlyExit;
  ARMEmitter::BackwardLabel NextBit;
  ARMEmitter::ForwardLabel Done;
  cbz(EmitSize, Mask, &EarlyExit);
  mov(EmitSize, IndexReg, ZeroReg);

  // We sadly need to spill regs for this for the time being
  // TODO: Remove when scratch registers can be allocated
  //       explicitly.
  SpillStaticRegs(TMP1, false, SpillCode);


  mov(EmitSize, InputReg, Input);
  mov(EmitSize, MaskReg, Mask);
  mov(EmitSize, DestReg, ZeroReg);

  // Main loop
  Bind(&NextBit);
  rbit(EmitSize, ShiftedBitReg, MaskReg);
  clz(EmitSize, ShiftedBitReg, ShiftedBitReg);
  lsrv(EmitSize, BitReg, InputReg, IndexReg);
  and_(EmitSize, BitReg, BitReg, 1);
  sub(EmitSize, SubMaskReg, MaskReg, 1);
  add(EmitSize, IndexReg, IndexReg, 1);
  ands(EmitSize, MaskReg, MaskReg, SubMaskReg);
  lslv(EmitSize, ShiftedBitReg, BitReg, ShiftedBitReg);
  orr(EmitSize, DestReg, DestReg, ShiftedBitReg);
  b(ARMEmitter::Condition::CC_NE, &NextBit);
  // Store result in a temp so it doesn't get clobbered.
  // and restore it after the re-fill below.
  mov(EmitSize, IndexReg, DestReg);
  // Restore our registers before leaving
  // TODO: Also remove along with above TODO.
  FillStaticRegs(false, SpillCode);
  mov(EmitSize, Dest, IndexReg);
  b(&Done);

  // Early exit
  Bind(&EarlyExit);
  mov(EmitSize, Dest, ZeroReg);

  // All done with nothing to do.
  Bind(&Done);
}

DEF_OP(PExt) {
  auto Op = IROp->C<IR::IROp_PExt>();
  const auto OpSize = IROp->Size;

  LOGMAN_THROW_AA_FMT(OpSize == 4 || OpSize == 8, "Unsupported {} size: {}", __func__, OpSize);
  const auto EmitSize = OpSize == 8 ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;

  const auto Input = GetReg(Op->Input.ID());
  const auto Mask = GetReg(Op->Mask.ID());
  const auto Dest = GetReg(Node);

  const auto MaskReg    = TMP1;
  const auto BitReg     = TMP2;
  const auto SubMaskReg = TMP3;
  const auto Offset     = TMP4;
  const auto ZeroReg    = ARMEmitter::Reg::zr;

  ARMEmitter::ForwardLabel EarlyExit;
  ARMEmitter::BackwardLabel NextBit;
  ARMEmitter::ForwardLabel Done;

  cbz(EmitSize, Mask, &EarlyExit);
  mov(EmitSize, MaskReg, Mask);
  mov(EmitSize, Offset, ZeroReg);

  // We sadly need to spill a reg for this for the time being
  // TODO: Remove when scratch registers can be allocated
  //       explicitly.
  SpillStaticRegs(TMP2, false, 1U << Mask.Idx());
  mov(EmitSize, Mask, ZeroReg);

  // Main loop
  Bind(&NextBit);
  rbit(EmitSize, BitReg, MaskReg);
  clz(EmitSize, BitReg, BitReg);
  sub(EmitSize, SubMaskReg, MaskReg, 1);
  ands(EmitSize, MaskReg.R(), SubMaskReg.R(), MaskReg.R());
  lsrv(EmitSize, BitReg, Input, BitReg);
  and_(EmitSize, BitReg, BitReg, 1);
  lslv(EmitSize, BitReg, BitReg, Offset);
  add(EmitSize, Offset, Offset, 1);
  orr(EmitSize, Mask, BitReg, Mask);
  b(ARMEmitter::Condition::CC_NE, &NextBit);
  mov(EmitSize, Dest, Mask);
  // Restore our mask register before leaving
  // TODO: Also remove along with above TODO.
  FillStaticRegs(false, 1U << Mask.Idx());
  b(&Done);

  // Early exit
  Bind(&EarlyExit);
  mov(EmitSize, Dest, ZeroReg);

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
      mov(EmitSize, TMP1, Lower);
      bfi(EmitSize, TMP1, Upper, 32, 32);
      sxtw(TMP2, Divisor.W());
      sdiv(EmitSize, Dst, TMP1, TMP2);
    break;
    }
    case 8: {
      ARMEmitter::ForwardLabel Only64Bit{};
      ARMEmitter::ForwardLabel LongDIVRet{};

      // Check if the upper bits match the top bit of the lower 64-bits
      // Sign extend the top bit of lower bits
      sbfx(EmitSize, TMP1, Lower, 63, 1);
      eor(EmitSize, TMP1, TMP1, Upper);

      // If the sign bit matches then the result is zero
      cbz(EmitSize, TMP1, &Only64Bit);

      // Long divide
      {
        mov(EmitSize, ARMEmitter::Reg::r0, Upper);
        mov(EmitSize, ARMEmitter::Reg::r1, Lower);
        mov(EmitSize, ARMEmitter::Reg::r2, Divisor);

        ldr(ARMEmitter::XReg::x3, STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.AArch64.LDIVHandler));

        str<ARMEmitter::IndexType::PRE>(ARMEmitter::XReg::lr, ARMEmitter::Reg::rsp, -16);
        blr(ARMEmitter::Reg::r3);
        ldr<ARMEmitter::IndexType::POST>(ARMEmitter::XReg::lr, ARMEmitter::Reg::rsp, 16);

        // Move result to its destination register
        mov(EmitSize, Dst, ARMEmitter::Reg::r0);

        // Skip 64-bit path
        b(&LongDIVRet);
      }

      Bind(&Only64Bit);
      // 64-Bit only
      {
        sdiv(EmitSize, Dst, Lower, Divisor);
      }

      Bind(&LongDIVRet);
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
      mov(EmitSize, TMP1, Lower);
      bfi(EmitSize, TMP1, Upper, 32, 32);
      udiv(EmitSize, Dst, TMP1, Divisor);
    break;
    }
    case 8: {
      ARMEmitter::ForwardLabel Only64Bit{};
      ARMEmitter::ForwardLabel LongDIVRet{};

      // Check the upper bits for zero
      // If the upper bits are zero then we can do a 64-bit divide
      cbz(EmitSize, Upper, &Only64Bit);

      // Long divide
      {
        mov(EmitSize, ARMEmitter::Reg::r0, Upper);
        mov(EmitSize, ARMEmitter::Reg::r1, Lower);
        mov(EmitSize, ARMEmitter::Reg::r2, Divisor);

        ldr(ARMEmitter::XReg::x3, STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.AArch64.LUDIVHandler));

        str<ARMEmitter::IndexType::PRE>(ARMEmitter::XReg::lr, ARMEmitter::Reg::rsp, -16);
        blr(ARMEmitter::Reg::r3);
        ldr<ARMEmitter::IndexType::POST>(ARMEmitter::XReg::lr, ARMEmitter::Reg::rsp, 16);

        // Move result to its destination register
        mov(EmitSize, Dst, ARMEmitter::Reg::r0);

        // Skip 64-bit path
        b(&LongDIVRet);
      }

      Bind(&Only64Bit);
      // 64-Bit only
      {
        udiv(EmitSize, Dst, Lower, Divisor);
      }

      Bind(&LongDIVRet);
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
      mov(EmitSize, TMP1, Lower);
      bfi(EmitSize, TMP1, Upper, 32, 32);
      sxtw(TMP3, Divisor.W());
      sdiv(EmitSize, TMP2, TMP1, TMP3);
      msub(EmitSize, Dst, TMP2, TMP3, TMP1);
    break;
    }
    case 8: {
      ARMEmitter::ForwardLabel Only64Bit{};
      ARMEmitter::ForwardLabel LongDIVRet{};

      // Check if the upper bits match the top bit of the lower 64-bits
      // Sign extend the top bit of lower bits
      sbfx(EmitSize, TMP1, Lower, 63, 1);
      eor(EmitSize, TMP1, TMP1, Upper);

      // If the sign bit matches then the result is zero
      cbz(EmitSize, TMP1, &Only64Bit);

      // Long divide
      {
        mov(EmitSize, ARMEmitter::Reg::r0, Upper);
        mov(EmitSize, ARMEmitter::Reg::r1, Lower);
        mov(EmitSize, ARMEmitter::Reg::r2, Divisor);

        ldr(ARMEmitter::XReg::x3, STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.AArch64.LREMHandler));

        str<ARMEmitter::IndexType::PRE>(ARMEmitter::XReg::lr, ARMEmitter::Reg::rsp, -16);
        blr(ARMEmitter::Reg::r3);
        ldr<ARMEmitter::IndexType::POST>(ARMEmitter::XReg::lr, ARMEmitter::Reg::rsp, 16);

        // Move result to its destination register
        mov(EmitSize, Dst, ARMEmitter::Reg::r0);

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
    default:
      LOGMAN_MSG_A_FMT("Unknown LREM Size: {}", OpSize);
      break;
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
      mov(EmitSize, TMP1, Lower);
      bfi(EmitSize, TMP1, Upper, 32, 32);
      udiv(EmitSize, TMP2, TMP1, Divisor);
      msub(EmitSize, Dst, TMP2, Divisor, TMP1);
    break;
    }
    case 8: {
      ARMEmitter::ForwardLabel Only64Bit{};
      ARMEmitter::ForwardLabel LongDIVRet{};

      // Check the upper bits for zero
      // If the upper bits are zero then we can do a 64-bit divide
      cbz(EmitSize, Upper, &Only64Bit);

      // Long divide
      {
        mov(EmitSize, ARMEmitter::Reg::r0, Upper);
        mov(EmitSize, ARMEmitter::Reg::r1, Lower);
        mov(EmitSize, ARMEmitter::Reg::r2, Divisor);

        ldr(ARMEmitter::XReg::x3, STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.AArch64.LUREMHandler));

        str<ARMEmitter::IndexType::PRE>(ARMEmitter::XReg::lr, ARMEmitter::Reg::rsp, -16);
        blr(ARMEmitter::Reg::r3);
        ldr<ARMEmitter::IndexType::POST>(ARMEmitter::XReg::lr, ARMEmitter::Reg::rsp, 16);

        // Move result to its destination register
        mov(EmitSize, Dst, ARMEmitter::Reg::r0);

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
    default:
      LOGMAN_MSG_A_FMT("Unknown LUREM Size: {}", OpSize);
      break;
  }
}

DEF_OP(Not) {
  auto Op = IROp->C<IR::IROp_Not>();
  const uint8_t OpSize = IROp->Size;

  LOGMAN_THROW_AA_FMT(OpSize == 4 || OpSize == 8, "Unsupported {} size: {}", __func__, OpSize);
  const auto EmitSize = OpSize == 8 ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;

  const auto Dst = GetReg(Node);
  const auto Src = GetReg(Op->Src.ID());

  mvn(EmitSize, Dst, Src);
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
      cnt(FEXCore::ARMEmitter::SubRegSize::i8Bit, VTMP1.D(), VTMP1.D());
      break;
    case 0x2:
      fmov(ARMEmitter::Size::i32Bit, VTMP1.S(), Src);
      cnt(FEXCore::ARMEmitter::SubRegSize::i8Bit, VTMP1.D(), VTMP1.D());
      // only count two lowest bytes
      addp(FEXCore::ARMEmitter::SubRegSize::i8Bit, VTMP1.D(), VTMP1.D(), VTMP1.D());
      break;
    case 0x4:
      fmov(ARMEmitter::Size::i32Bit, VTMP1.S(), Src);
      cnt(FEXCore::ARMEmitter::SubRegSize::i8Bit, VTMP1.D(), VTMP1.D());
      // fmov has zero extended, unused bytes are zero
      addv(ARMEmitter::SubRegSize::i8Bit, VTMP1.D(), VTMP1.D());
      break;
    case 0x8:
      fmov(ARMEmitter::Size::i64Bit, VTMP1.D(), Src);
      cnt(FEXCore::ARMEmitter::SubRegSize::i8Bit, VTMP1.D(), VTMP1.D());
      // fmov has zero extended, unused bytes are zero
      addv(ARMEmitter::SubRegSize::i8Bit, VTMP1.D(), VTMP1.D());
      break;
    default: LOGMAN_MSG_A_FMT("Unsupported Popcount size: {}", OpSize);
  }

  umov<ARMEmitter::SubRegSize::i8Bit>(Dst, VTMP1, 0);
}

DEF_OP(FindLSB) {
  auto Op = IROp->C<IR::IROp_FindLSB>();
  const uint8_t OpSize = IROp->Size;

  const auto EmitSize = OpSize == 8 ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;

  const auto Dst = GetReg(Node);
  const auto Src = GetReg(Op->Src.ID());

  if (OpSize != 8) {
    ubfx(EmitSize, TMP1, Src, 0, OpSize * 8);
    cmp(EmitSize, TMP1, 0);
    rbit(EmitSize, TMP1, TMP1);
  }
  else {
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
  const auto EmitSize = OpSize == 8 ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;

  const auto Dst = GetReg(Node);
  const auto Src = GetReg(Op->Src.ID());

  movz(ARMEmitter::Size::i64Bit, TMP1, OpSize * 8 - 1);

  if (OpSize == 2) {
    lsl(EmitSize, Dst, Src, 16);
    orr(EmitSize, Dst, Dst, 0x8000);
    clz(EmitSize, Dst, Dst);
  }
  else {
    clz(EmitSize, Dst, Src);
  }

  sub(ARMEmitter::Size::i64Bit, Dst, TMP1, Dst);
}

DEF_OP(FindTrailingZeros) {
  auto Op = IROp->C<IR::IROp_FindTrailingZeros>();
  const uint8_t OpSize = IROp->Size;

  LOGMAN_THROW_AA_FMT(OpSize == 2 || OpSize == 4 || OpSize == 8, "Unsupported {} size: {}", __func__, OpSize);
  const auto EmitSize = OpSize == 8 ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;

  const auto Dst = GetReg(Node);
  const auto Src = GetReg(Op->Src.ID());

  rbit(EmitSize, Dst, Src);

  if (OpSize == 2) {
    orr(EmitSize, Dst, Dst, 0x8000);
  }

  clz(EmitSize, Dst, Dst);
}

DEF_OP(CountLeadingZeroes) {
  auto Op = IROp->C<IR::IROp_CountLeadingZeroes>();
  const uint8_t OpSize = IROp->Size;

  LOGMAN_THROW_AA_FMT(OpSize == 2 || OpSize == 4 || OpSize == 8, "Unsupported {} size: {}", __func__, OpSize);
  const auto EmitSize = OpSize == 8 ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;

  const auto Dst = GetReg(Node);
  const auto Src = GetReg(Op->Src.ID());

  if (OpSize == 2) {
    lsl(EmitSize, Dst, Src, 16);
    orr(EmitSize, Dst, Dst, 0x8000);
    clz(EmitSize, Dst, Dst);
  }
  else {
    clz(EmitSize, Dst, Src);
  }
}

DEF_OP(Rev) {
  auto Op = IROp->C<IR::IROp_Rev>();
  const uint8_t OpSize = IROp->Size;

  LOGMAN_THROW_AA_FMT(OpSize == 2 || OpSize == 4 || OpSize == 8, "Unsupported {} size: {}", __func__, OpSize);
  const auto EmitSize = OpSize == 8 ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;

  const auto Dst = GetReg(Node);
  const auto Src = GetReg(Op->Src.ID());

  rev(EmitSize, Dst, Src);
  if (OpSize == 2) {
    lsr(EmitSize, Dst, Dst, 16);
  }
}

DEF_OP(Bfi) {
  auto Op = IROp->C<IR::IROp_Bfi>();
  const uint8_t OpSize = IROp->Size;

  const auto EmitSize = OpSize == 8 ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;

  const auto Dst = GetReg(Node);
  const auto SrcDst = GetReg(Op->Dest.ID());
  const auto Src = GetReg(Op->Src.ID());

  if (Dst == SrcDst) {
    // If Dst and SrcDst match then this turns in to a simple BFI instruction.
    bfi(EmitSize, Dst, Src, Op->lsb, Op->Width);
  }
  else {
    // Destination didn't match the dst source register.
    // TODO: Inefficient until FEX can have RA constraints here.
    mov(EmitSize, TMP1, SrcDst);
    bfi(EmitSize, TMP1, Src, Op->lsb, Op->Width);

    if (OpSize == 8) {
      mov(EmitSize, Dst, TMP1.R());
    }
    else {
      ubfx(EmitSize, Dst, TMP1, 0, OpSize * 8);
    }
  }
}

DEF_OP(Bfe) {
  auto Op = IROp->C<IR::IROp_Bfe>();
  LOGMAN_THROW_AA_FMT(IROp->Size <= 8, "OpSize is too large for BFE: {}", IROp->Size);
  LOGMAN_THROW_AA_FMT(Op->Width != 0, "Invalid BFE width of 0");

  const auto Dst = GetReg(Node);
  const auto Src = GetReg(Op->Src.ID());

  ubfx(ARMEmitter::Size::i64Bit, Dst, Src, Op->lsb, Op->Width);
}

DEF_OP(Sbfe) {
  auto Op = IROp->C<IR::IROp_Bfe>();
  const auto Dst = GetReg(Node);
  const auto Src = GetReg(Op->Src.ID());

  sbfx(ARMEmitter::Size::i64Bit, Dst, Src, Op->lsb, Op->Width);
}

ARMEmitter::Condition MapSelectCC(IR::CondClassType Cond) {
  switch (Cond.Val) {
  case FEXCore::IR::COND_EQ:  return ARMEmitter::Condition::CC_EQ;
  case FEXCore::IR::COND_NEQ: return ARMEmitter::Condition::CC_NE;
  case FEXCore::IR::COND_SGE: return ARMEmitter::Condition::CC_GE;
  case FEXCore::IR::COND_SLT: return ARMEmitter::Condition::CC_LT;
  case FEXCore::IR::COND_SGT: return ARMEmitter::Condition::CC_GT;
  case FEXCore::IR::COND_SLE: return ARMEmitter::Condition::CC_LE;
  case FEXCore::IR::COND_UGE: return ARMEmitter::Condition::CC_CS;
  case FEXCore::IR::COND_ULT: return ARMEmitter::Condition::CC_CC;
  case FEXCore::IR::COND_UGT: return ARMEmitter::Condition::CC_HI;
  case FEXCore::IR::COND_ULE: return ARMEmitter::Condition::CC_LS;
  case FEXCore::IR::COND_FLU: return ARMEmitter::Condition::CC_LT;
  case FEXCore::IR::COND_FGE: return ARMEmitter::Condition::CC_GE;
  case FEXCore::IR::COND_FLEU:return ARMEmitter::Condition::CC_LE;
  case FEXCore::IR::COND_FGT: return ARMEmitter::Condition::CC_GT;
  case FEXCore::IR::COND_FU:  return ARMEmitter::Condition::CC_VS;
  case FEXCore::IR::COND_FNU: return ARMEmitter::Condition::CC_VC;
  case FEXCore::IR::COND_VS:
  case FEXCore::IR::COND_VC:
  case FEXCore::IR::COND_MI:
  case FEXCore::IR::COND_PL:
  default:
  LOGMAN_MSG_A_FMT("Unsupported compare type");
  return ARMEmitter::Condition::CC_NV;
  }
}

DEF_OP(Select) {
  auto Op = IROp->C<IR::IROp_Select>();
  const uint8_t OpSize = IROp->Size;
  const auto EmitSize = OpSize == 8 ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;
  const auto CompareEmitSize = Op->CompareSize == 8 ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;

  uint64_t Const;

  if (IsGPR(Op->Cmp1.ID())) {
    const auto Src1 = GetReg(Op->Cmp1.ID());

    if (IsInlineConstant(Op->Cmp2, &Const))
      cmp(CompareEmitSize, Src1, Const);
    else {
      const auto Src2 = GetReg(Op->Cmp2.ID());
      cmp(CompareEmitSize, Src1, Src2);
    }
  } else if (IsFPR(Op->Cmp1.ID())) {
    const auto Src1 = GetVReg(Op->Cmp1.ID());
    const auto Src2 = GetVReg(Op->Cmp2.ID());
    fcmp(Op->CompareSize == 8 ? ARMEmitter::ScalarRegSize::i64Bit : ARMEmitter::ScalarRegSize::i32Bit, Src1, Src2);
  } else {
    LOGMAN_MSG_A_FMT("Select: Expected GPR or FPR");
  }

  auto cc = MapSelectCC(Op->Cond);

  uint64_t const_true, const_false;
  bool is_const_true = IsInlineConstant(Op->TrueVal, &const_true);
  bool is_const_false = IsInlineConstant(Op->FalseVal, &const_false);

  ARMEmitter::Register Dst = GetReg(Node);

  if (is_const_true || is_const_false) {
    if (is_const_false != true || is_const_true != true || const_true != 1 || const_false != 0) {
      LOGMAN_MSG_A_FMT("Select: Unsupported compare inline parameters");
    }
    cset(EmitSize, Dst, cc);
  } else {
    csel(EmitSize, Dst, GetReg(Op->TrueVal.ID()), GetReg(Op->FalseVal.ID()), cc);
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
      case 1:
        umov<ARMEmitter::SubRegSize::i8Bit>(Dst, Vector, index);
        break;
      case 2:
        umov<ARMEmitter::SubRegSize::i16Bit>(Dst, Vector, index);
        break;
      case 4:
        umov<ARMEmitter::SubRegSize::i32Bit>(Dst, Vector, index);
        break;
      case 8:
        umov<ARMEmitter::SubRegSize::i64Bit>(Dst, Vector, index);
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
    const auto CompactPred = ARMEmitter::PReg::p0;
    not_(CompactPred, PRED_TMP_32B.Zeroing(), PRED_TMP_16B);
    compact(ARMEmitter::SubRegSize::i64Bit, VTMP1.Z(), CompactPred, Vector.Z());

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

  ARMEmitter::Register Dst = GetReg(Node);
  ARMEmitter::VRegister Src = GetVReg(Op->Scalar.ID());
  const auto DestSize = IROp->Size == 8 ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;

  if (Op->SrcElementSize == 8) {
    fcvtzs(DestSize, Dst, Src.D());
  }
  else {
    fcvtzs(DestSize, Dst, Src.S());
  }
}

DEF_OP(Float_ToGPR_S) {
  auto Op = IROp->C<IR::IROp_Float_ToGPR_S>();

  ARMEmitter::Register Dst = GetReg(Node);
  ARMEmitter::VRegister Src = GetVReg(Op->Scalar.ID());
  const auto DestSize = IROp->Size == 8 ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;

  if (Op->SrcElementSize == 8) {
    frinti(VTMP1.D(), Src.D());
    fcvtzs(DestSize, Dst, VTMP1.D());
  }
  else {
    frinti(VTMP1.S(), Src.S());
    fcvtzs(DestSize, Dst, VTMP1.S());
  }
}

DEF_OP(FCmp) {
  auto Op = IROp->C<IR::IROp_FCmp>();
  const auto EmitSubSize = Op->ElementSize == 8 ? ARMEmitter::ScalarRegSize::i64Bit : ARMEmitter::ScalarRegSize::i32Bit;

  ARMEmitter::Register Dst = GetReg(Node);
  ARMEmitter::VRegister Scalar1 = GetVReg(Op->Scalar1.ID());
  ARMEmitter::VRegister Scalar2 = GetVReg(Op->Scalar2.ID());

  fcmp(EmitSubSize, Scalar1, Scalar2);
  bool set = false;

  if (Op->Flags & (1 << IR::FCMP_FLAG_EQ)) {
    LOGMAN_THROW_AA_FMT(IR::FCMP_FLAG_EQ == 0, "IR::FCMP_FLAG_EQ must equal 0");
    // EQ or unordered
    cset(ARMEmitter::Size::i64Bit, Dst, ARMEmitter::Condition::CC_EQ); // Z = 1
    csinc(ARMEmitter::Size::i64Bit, Dst, Dst, ARMEmitter::Reg::zr, ARMEmitter::Condition::CC_VC); // IF !V ? Z : 1
    set = true;
  }

  if (Op->Flags & (1 << IR::FCMP_FLAG_LT)) {
    // LT or unordered
    cset(ARMEmitter::Size::i64Bit, TMP2, ARMEmitter::Condition::CC_LT);
    if (!set) {
      lsl(ARMEmitter::Size::i64Bit, Dst, TMP2, IR::FCMP_FLAG_LT);
      set = true;
    } else {
      bfi(ARMEmitter::Size::i64Bit, Dst, TMP2, IR::FCMP_FLAG_LT, 1);
    }
  }

  if (Op->Flags & (1 << IR::FCMP_FLAG_UNORDERED)) {
    cset(ARMEmitter::Size::i64Bit, TMP2, ARMEmitter::Condition::CC_VS);
    if (!set) {
      lsl(ARMEmitter::Size::i64Bit, Dst, TMP2, IR::FCMP_FLAG_UNORDERED);
      set = true;
    } else {
      bfi(ARMEmitter::Size::i64Bit, Dst, TMP2, IR::FCMP_FLAG_UNORDERED, 1);
    }
  }
}

#undef DEF_OP

}
