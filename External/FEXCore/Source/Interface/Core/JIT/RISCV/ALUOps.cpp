/*
$info$
tags: backend|riscv64
$end_info$
*/

#include "Interface/Core/JIT/RISCV/JITClass.h"
#include "Interface/IR/Passes/RegisterAllocationPass.h"

#include <biscuit/assembler.hpp>

namespace FEXCore::CPU {
using namespace biscuit;

#define DEF_OP(x) void RISCVJITCore::Op_##x(IR::IROp_Header *IROp, IR::NodeID Node)
DEF_OP(TruncElementPair) {
  auto Op = IROp->C<IR::IROp_TruncElementPair>();
  switch (IROp->Size) {
    case 4: {
      auto Dst = GetSrcPair(Node);
      auto Src = GetSrcPair(Op->Header.Args[0].ID());
      UXTW(Dst.first, Src.first);
      UXTW(Dst.second, Src.second);
      break;
    }
    default: LOGMAN_MSG_A_FMT("Unhandled Truncation size: {}", IROp->Size); break;
  }
}

DEF_OP(Constant) {
  auto Op = IROp->C<IR::IROp_Constant>();
  auto Dst = GetReg(Node);
  LoadConstant(Dst, Op->Constant);
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

  LoadConstant(Dst, Constant & Mask);
}

DEF_OP(InlineConstant) {
  //nop
}

DEF_OP(InlineEntrypointOffset) {
  //nop
}

DEF_OP(CycleCounter) {
  RDTIME(GetReg(Node));
}

DEF_OP(Add) {
  auto Op = IROp->C<IR::IROp_Add>();
  uint8_t OpSize = IROp->Size;

  switch (OpSize) {
    case 4:
      ADDUW(GetReg(Node), GetReg(Op->Header.Args[0].ID()), GetReg(Op->Header.Args[1].ID()));
      break;
    case 8:
      ADD(GetReg(Node), GetReg(Op->Header.Args[0].ID()), GetReg(Op->Header.Args[1].ID()));
      break;
    default: LOGMAN_MSG_A_FMT("Unsupported Add size: {}", OpSize);
  }
}

DEF_OP(Sub) {
  auto Op = IROp->C<IR::IROp_Sub>();
  uint8_t OpSize = IROp->Size;

  switch (OpSize) {
    case 4:
      SUBW(GetReg(Node), GetReg(Op->Header.Args[0].ID()), GetReg(Op->Header.Args[1].ID()));
      UXTW(GetReg(Node), GetReg(Node));
      break;
    case 8:
      SUB(GetReg(Node), GetReg(Op->Header.Args[0].ID()), GetReg(Op->Header.Args[1].ID()));
      break;
    default: LOGMAN_MSG_A_FMT("Unsupported Sub size: {}", OpSize);
  }
}

DEF_OP(Neg) {
  auto Op = IROp->C<IR::IROp_Neg>();
  uint8_t OpSize = IROp->Size;
  switch (OpSize) {
    case 4:
      NEG(GetReg(Node), GetReg(Op->Header.Args[0].ID()));
      UXTW(GetReg(Node), GetReg(Node));
      break;
    case 8:
      NEG(GetReg(Node), GetReg(Op->Header.Args[0].ID()));
      break;
    default: LOGMAN_MSG_A_FMT("Unsupported Neg size: {}", OpSize);
  }
}

DEF_OP(Mul) {
  auto Op = IROp->C<IR::IROp_Mul>();
  uint8_t OpSize = IROp->Size;
  auto Dst = GetReg(Node);

  switch (OpSize) {
    case 4:
      MULW(Dst, GetReg(Op->Header.Args[0].ID()), GetReg(Op->Header.Args[1].ID()));
      UXTW(Dst, Dst);
    break;
    case 8:
      MUL(Dst, GetReg(Op->Header.Args[0].ID()), GetReg(Op->Header.Args[1].ID()));
    break;
    default: LOGMAN_MSG_A_FMT("Unknown Mul size: {}", OpSize);
  }
}

DEF_OP(UMul) {
  auto Op = IROp->C<IR::IROp_UMul>();
  uint8_t OpSize = IROp->Size;
  auto Dst = GetReg(Node);

  switch (OpSize) {
    case 4:
      MULW(Dst, GetReg(Op->Header.Args[0].ID()), GetReg(Op->Header.Args[1].ID()));
      UXTW(Dst, Dst);
    break;
    case 8:
      MUL(Dst, GetReg(Op->Header.Args[0].ID()), GetReg(Op->Header.Args[1].ID()));
    break;
    default: LOGMAN_MSG_A_FMT("Unknown UMul size: {}", OpSize);
  }
}

DEF_OP(Div) {
  auto Op = IROp->C<IR::IROp_Div>();
  uint8_t OpSize = IROp->Size;
  // Each source is OpSize in size
  // So you can have up to a 128bit divide from x86-64
  auto Size = OpSize;
  switch (Size) {
    case 1: {
      auto Dividend = GetReg(Op->Header.Args[0].ID());
      auto Divisor = GetReg(Op->Header.Args[1].ID());
      SLLIW(TMP1, Dividend, 24);
      SRAIW(TMP1, TMP1, 24);
      SLLIW(TMP2, Divisor, 24);
      SRAIW(TMP2, TMP1, 24);

      DIVW(GetReg(Node), TMP1, TMP2);
      UXTW(GetReg(Node), GetReg(Node));
    break;
    }
    case 2: {
      auto Dividend = GetReg(Op->Header.Args[0].ID());
      auto Divisor = GetReg(Op->Header.Args[1].ID());
      SLLIW(TMP1, Dividend, 16);
      SRAIW(TMP1, TMP1, 16);
      SLLIW(TMP2, Divisor, 16);
      SRAIW(TMP2, TMP1, 16);

      DIVW(GetReg(Node), TMP1, TMP2);
      UXTW(GetReg(Node), GetReg(Node));
    break;
    }
    case 4: {
      DIVW(GetReg(Node), GetReg(Op->Header.Args[0].ID()), GetReg(Op->Header.Args[1].ID()));
      UXTW(GetReg(Node), GetReg(Node));
    break;
    }
    case 8: {
      DIV(GetReg(Node), GetReg(Op->Header.Args[0].ID()), GetReg(Op->Header.Args[1].ID()));
    break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown DIV Size: {}", Size); break;
  }
}

DEF_OP(UDiv) {
  auto Op = IROp->C<IR::IROp_UDiv>();
  uint8_t OpSize = IROp->Size;
  // Each source is OpSize in size
  // So you can have up to a 128bit divide from x86-64
  auto Size = OpSize;
  switch (Size) {
    case 1: {
      auto Dividend = GetReg(Op->Header.Args[0].ID());
      auto Divisor = GetReg(Op->Header.Args[1].ID());
      SLLIW(TMP1, Dividend, 24);
      SRLIW(TMP1, TMP1, 24);
      SLLIW(TMP2, Divisor, 24);
      SRLIW(TMP2, TMP1, 24);

      DIVUW(GetReg(Node), TMP1, TMP2);
      UXTW(GetReg(Node), GetReg(Node));
    break;
    }
    case 2: {
      auto Dividend = GetReg(Op->Header.Args[0].ID());
      auto Divisor = GetReg(Op->Header.Args[1].ID());
      SLLIW(TMP1, Dividend, 16);
      SRLIW(TMP1, TMP1, 16);
      SLLIW(TMP2, Divisor, 16);
      SRLIW(TMP2, TMP1, 16);

      DIVUW(GetReg(Node), TMP1, TMP2);
      UXTW(GetReg(Node), GetReg(Node));
    break;
    }
    case 4: {
      DIVUW(GetReg(Node), GetReg(Op->Header.Args[0].ID()), GetReg(Op->Header.Args[1].ID()));
      UXTW(GetReg(Node), GetReg(Node));
    break;
    }
    case 8: {
      DIVU(GetReg(Node), GetReg(Op->Header.Args[0].ID()), GetReg(Op->Header.Args[1].ID()));
    break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown DIV Size: {}", Size); break;
  }
}

DEF_OP(Rem) {
  auto Op = IROp->C<IR::IROp_Rem>();
  uint8_t OpSize = IROp->Size;
  // Each source is OpSize in size
  // So you can have up to a 128bit divide from x86-64
  auto Size = OpSize;
  switch (Size) {
    case 1: {
      auto Dividend = GetReg(Op->Header.Args[0].ID());
      auto Divisor = GetReg(Op->Header.Args[1].ID());
      SLLIW(TMP1, Dividend, 24);
      SRAIW(TMP1, TMP1, 24);
      SLLIW(TMP2, Divisor, 24);
      SRAIW(TMP2, TMP1, 24);

      REMW(GetReg(Node), TMP1, TMP2);
      UXTW(GetReg(Node), GetReg(Node));
    break;
    }
    case 2: {
      auto Dividend = GetReg(Op->Header.Args[0].ID());
      auto Divisor = GetReg(Op->Header.Args[1].ID());
      SLLIW(TMP1, Dividend, 16);
      SRAIW(TMP1, TMP1, 16);
      SLLIW(TMP2, Divisor, 16);
      SRAIW(TMP2, TMP1, 16);

      REMW(GetReg(Node), TMP1, TMP2);
      UXTW(GetReg(Node), GetReg(Node));
    break;
    }
    case 4: {
      REMW(GetReg(Node), GetReg(Op->Header.Args[0].ID()), GetReg(Op->Header.Args[1].ID()));
      UXTW(GetReg(Node), GetReg(Node));
    break;
    }
    case 8: {
      REM(GetReg(Node), GetReg(Op->Header.Args[0].ID()), GetReg(Op->Header.Args[1].ID()));
    break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown DIV Size: {}", Size); break;
  }
}

DEF_OP(URem) {
  auto Op = IROp->C<IR::IROp_URem>();
  uint8_t OpSize = IROp->Size;
  // Each source is OpSize in size
  // So you can have up to a 128bit divide from x86-64
  auto Size = OpSize;
  switch (Size) {
    case 1: {
      auto Dividend = GetReg(Op->Header.Args[0].ID());
      auto Divisor = GetReg(Op->Header.Args[1].ID());
      SLLIW(TMP1, Dividend, 24);
      SRLIW(TMP1, TMP1, 24);
      SLLIW(TMP2, Divisor, 24);
      SRLIW(TMP2, TMP1, 24);

      REMUW(GetReg(Node), TMP1, TMP2);
      UXTW(GetReg(Node), GetReg(Node));
    break;
    }
    case 2: {
      auto Dividend = GetReg(Op->Header.Args[0].ID());
      auto Divisor = GetReg(Op->Header.Args[1].ID());
      SLLIW(TMP1, Dividend, 16);
      SRLIW(TMP1, TMP1, 16);
      SLLIW(TMP2, Divisor, 16);
      SRLIW(TMP2, TMP1, 16);

      REMUW(GetReg(Node), TMP1, TMP2);
      UXTW(GetReg(Node), GetReg(Node));
    break;
    }
    case 4: {
      REMUW(GetReg(Node), GetReg(Op->Header.Args[0].ID()), GetReg(Op->Header.Args[1].ID()));
      UXTW(GetReg(Node), GetReg(Node));
    break;
    }
    case 8: {
      REMU(GetReg(Node), GetReg(Op->Header.Args[0].ID()), GetReg(Op->Header.Args[1].ID()));
    break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown DIV Size: {}", Size); break;
  }
}

DEF_OP(MulH) {
  auto Op = IROp->C<IR::IROp_MulH>();
  uint8_t OpSize = IROp->Size;
  switch (OpSize) {
    case 4:
      SXTW(TMP1, GetReg(Op->Header.Args[0].ID()));
      SXTW(TMP2, GetReg(Op->Header.Args[1].ID()));
      MULH(TMP1, TMP1, TMP2);
      UXTW(GetReg(Node), TMP1);
    break;
    case 8:
      MULH(GetReg(Node), GetReg(Op->Header.Args[0].ID()), GetReg(Op->Header.Args[1].ID()));
    break;
    default: LOGMAN_MSG_A_FMT("Unknown Sext size: {}", OpSize);
  }
}

DEF_OP(UMulH) {
  auto Op = IROp->C<IR::IROp_UMulH>();
  uint8_t OpSize = IROp->Size;
  switch (OpSize) {
    case 4:
      UXTW(TMP1, GetReg(Op->Header.Args[0].ID()));
      UXTW(TMP2, GetReg(Op->Header.Args[1].ID()));
      MULHU(TMP1, TMP1, TMP2);
      UXTW(GetReg(Node), TMP1);
    break;
    case 8:
      MULHU(GetReg(Node), GetReg(Op->Header.Args[0].ID()), GetReg(Op->Header.Args[1].ID()));
    break;
    default: LOGMAN_MSG_A_FMT("Unknown Sext size: {}", OpSize);
  }
}

DEF_OP(Or) {
  auto Op = IROp->C<IR::IROp_And>();
  uint64_t Const;
  if (IsInlineConstant(Op->Header.Args[1], &Const)) {
    ORI(GetReg(Node), GetReg(Op->Header.Args[0].ID()), (unsigned int)Const);
  } else {
    OR(GetReg(Node), GetReg(Op->Header.Args[0].ID()), GetReg(Op->Header.Args[1].ID()));
  }
  if (IROp->Size == 4) {
    UXTW(GetReg(Node), GetReg(Node));
  }
}

DEF_OP(And) {
  auto Op = IROp->C<IR::IROp_And>();
  uint64_t Const;
  if (IsInlineConstant(Op->Header.Args[1], &Const)) {
    ANDI(GetReg(Node), GetReg(Op->Header.Args[0].ID()), (unsigned int)Const);
  } else {
    AND(GetReg(Node), GetReg(Op->Header.Args[0].ID()), GetReg(Op->Header.Args[1].ID()));
  }
  if (IROp->Size == 4) {
    UXTW(GetReg(Node), GetReg(Node));
  }
}

DEF_OP(Andn) {
  auto Op = IROp->C<IR::IROp_Andn>();
  uint64_t Const{};

  if (IsInlineConstant(Op->Header.Args[1], &Const)) {
    ANDI(GetReg(Node), GetReg(Op->Header.Args[0].ID()), (unsigned int)~Const);
  } else {
    ANDN(GetReg(Node), GetReg(Op->Header.Args[0].ID()), GetReg(Op->Header.Args[1].ID()));
  }
  if (IROp->Size == 4) {
    UXTW(GetReg(Node), GetReg(Node));
  }
}

DEF_OP(Xor) {
  auto Op = IROp->C<IR::IROp_Xor>();
  uint64_t Const;
  if (IsInlineConstant(Op->Header.Args[1], &Const)) {
    XORI(GetReg(Node), GetReg(Op->Header.Args[0].ID()), Const);
  } else {
    XOR(GetReg(Node), GetReg(Op->Header.Args[0].ID()), GetReg(Op->Header.Args[1].ID()));
  }
  if (IROp->Size == 4) {
    UXTW(GetReg(Node), GetReg(Node));
  }
}

DEF_OP(Lshl) {
  auto Op = IROp->C<IR::IROp_Lshl>();
  uint64_t Const;
  if (IsInlineConstant(Op->Header.Args[1], &Const)) {
    SLLI64(GetReg(Node), GetReg(Op->Header.Args[0].ID()), (unsigned int)Const);
  } else {
    SLL(GetReg(Node), GetReg(Op->Header.Args[0].ID()), GetReg(Op->Header.Args[1].ID()));
  }
  if (IROp->Size == 4) {
    UXTW(GetReg(Node), GetReg(Node));
  }
}

DEF_OP(Lshr) {
  auto Op = IROp->C<IR::IROp_Lshr>();
  uint64_t Const;
  if (IsInlineConstant(Op->Header.Args[1], &Const)) {
    SRLI64(GetReg(Node), GetReg(Op->Header.Args[0].ID()), (unsigned int)Const);
  } else {
    SRL(GetReg(Node), GetReg(Op->Header.Args[0].ID()), GetReg(Op->Header.Args[1].ID()));
  }
  if (IROp->Size == 4) {
    UXTW(GetReg(Node), GetReg(Node));
  }
}

DEF_OP(Ashr) {
  auto Op = IROp->C<IR::IROp_Ashr>();
  uint8_t OpSize = IROp->Size;

  uint64_t Const;
  if (IsInlineConstant(Op->Header.Args[1], &Const)) {
    if (OpSize >= 4) {
      SRAI64(GetReg(Node), GetReg(Op->Header.Args[0].ID()), (unsigned int)Const);
      if (OpSize == 4) {
        UXTW(GetReg(Node), GetReg(Node));
      }
    }
    else {
      SBFX(TMP1, GetReg(Op->Header.Args[0].ID()), 0, OpSize * 8);
      SRAI(GetReg(Node), TMP1, (unsigned int)Const);
      UBFX(GetReg(Node),GetReg(Node), 0, OpSize * 8);
    }
  } else {
    if (OpSize >= 4) {
      SRA(GetReg(Node), GetReg(Op->Header.Args[0].ID()), GetReg(Op->Header.Args[1].ID()));
      if (OpSize == 4) {
        UXTW(GetReg(Node), GetReg(Node));
      }
    }
    else {
      SBFX(TMP1, GetReg(Op->Header.Args[0].ID()), 0, OpSize * 8);
      SRA(GetReg(Node), TMP1, GetReg(Op->Header.Args[1].ID()));
      UBFX(GetReg(Node), GetReg(Node), 0, OpSize * 8);
    }
  }
}

DEF_OP(Ror) {
  auto Op = IROp->C<IR::IROp_Ror>();
  uint8_t OpSize = IROp->Size;

  uint64_t Const;
  if (IsInlineConstant(Op->Header.Args[1], &Const)) {
    switch (OpSize) {
      case 4: {
        RORIW(GetReg(Node), GetReg(Op->Header.Args[0].ID()), (unsigned int)Const);
        UXTW(GetReg(Node), GetReg(Node));
      break;
      }
      case 8: {
        RORI(GetReg(Node), GetReg(Op->Header.Args[0].ID()), (unsigned int)Const);
      break;
      }

      default: LOGMAN_MSG_A_FMT("Unhandled ROR size: {}", OpSize);
    }
  } else {
    switch (OpSize) {
      case 4: {
        RORW(GetReg(Node), GetReg(Op->Header.Args[0].ID()), GetReg(Op->Header.Args[1].ID()));
        UXTW(GetReg(Node), GetReg(Node));
      break;
      }
      case 8: {
        ROR(GetReg(Node), GetReg(Op->Header.Args[0].ID()), GetReg(Op->Header.Args[1].ID()));
      break;
      }

      default: LOGMAN_MSG_A_FMT("Unhandled ROR size: {}", OpSize);
    }
  }
}

DEF_OP(Extr) {
  auto Op = IROp->C<IR::IROp_Extr>();
  uint8_t OpSize = IROp->Size;

  switch (OpSize) {
    case 4: {
      SLLI64(TMP1, GetReg(Op->Upper.ID()), 32);
      UXTW(TMP2, GetReg(Op->Lower.ID()));
      OR(TMP1, TMP2, TMP1);
      SRLI64(TMP1, TMP1, Op->LSB);
      UXTW(GetReg(Node), TMP1);
    break;
    }
    case 8: {
      SLLI64(TMP1, GetReg(Op->Upper.ID()), 64 - Op->LSB);
      SRLI64(TMP2, GetReg(Op->Lower.ID()), Op->LSB);
      OR(GetReg(Node), TMP1, TMP2);
    break;
    }

    default: LOGMAN_MSG_A_FMT("Unhandled EXTR size: {}", OpSize);
  }
}

//DEF_OP(PDep) {
//  auto Op = IROp->C<IR::IROp_PExt>();
//  const auto OpSize = IROp->Size;
//
//  const Register Input = GRS(Op->Args(0).ID());
//  const Register Mask = GRS(Op->Args(1).ID());
//  const Register Dest = GRS(Node);
//
//  const Register ShiftedBitReg = OpSize <= 4 ? TMP1.W() : TMP1;
//  const Register BitReg        = OpSize <= 4 ? TMP2.W() : TMP2;
//  const Register SubMaskReg    = OpSize <= 4 ? TMP3.W() : TMP3;
//  const Register IndexReg      = OpSize <= 4 ? TMP4.W() : TMP4;
//  const Register SizedZero     = OpSize <= 4 ? Register{wzr} : Register{xzr};
//
//  const Register InputReg = OpSize <= 4 ? SRA64[0].W() : SRA64[0];
//  const Register MaskReg  = OpSize <= 4 ? SRA64[1].W() : SRA64[1];
//  const Register DestReg  = OpSize <= 4 ? SRA64[2].W() : SRA64[2];
//  const auto SpillCode    = 1U << InputReg.GetCode() |
//                            1U << MaskReg.GetCode() |
//                            1U << DestReg.GetCode();
//
//  aarch64::Label EarlyExit;
//  aarch64::Label NextBit;
//  aarch64::Label Done;
//
//  cbz(Mask, &EarlyExit);
//  mov(IndexReg, SizedZero);
//
//  // We sadly need to spill regs for this for the time being
//  // TODO: Remove when scratch registers can be allocated
//  //       explicitly.
//  SpillStaticRegs(false, SpillCode);
//  mov(InputReg, Input);
//  mov(MaskReg, Mask);
//  mov(DestReg, SizedZero);
//
//  // Main loop
//  bind(&NextBit);
//  rbit(ShiftedBitReg, MaskReg);
//  clz(ShiftedBitReg, ShiftedBitReg);
//  lsrv(BitReg, InputReg, IndexReg);
//  and_(BitReg, BitReg, 1);
//  sub(SubMaskReg, MaskReg, 1);
//  add(IndexReg, IndexReg, 1);
//  ands(MaskReg, MaskReg, SubMaskReg);
//  lslv(ShiftedBitReg, BitReg, ShiftedBitReg);
//  orr(DestReg, DestReg, ShiftedBitReg);
//  b(&NextBit, Condition::ne);
//  // Store result in a temp so it doesn't get clobbered.
//  // and restore it after the re-fill below.
//  mov(IndexReg, DestReg);
//  // Restore our registers before leaving
//  // TODO: Also remove along with above TODO.
//  FillStaticRegs(false, SpillCode);
//  mov(Dest, IndexReg);
//  b(&Done);
//
//  // Early exit
//  bind(&EarlyExit);
//  mov(Dest, SizedZero);
//
//  // All done with nothing to do.
//  bind(&Done);
//}
//
//DEF_OP(PExt) {
//  auto Op = IROp->C<IR::IROp_PExt>();
//  const auto OpSize = IROp->Size;
//
//  const Register Input = GRS(Op->Args(0).ID());
//  const Register Mask = GRS(Op->Args(1).ID());
//  const Register Dest = GRS(Node);
//
//  const Register MaskReg    = OpSize <= 4 ? TMP1.W() : TMP1;
//  const Register BitReg     = OpSize <= 4 ? TMP2.W() : TMP2;
//  const Register SubMaskReg = OpSize <= 4 ? TMP3.W() : TMP3;
//  const Register Offset     = OpSize <= 4 ? TMP4.W() : TMP4;
//  const Register SizedZero  = OpSize <= 4 ? Register{wzr} : Register{xzr};
//
//  aarch64::Label EarlyExit;
//  aarch64::Label NextBit;
//  aarch64::Label Done;
//
//  cbz(Mask, &EarlyExit);
//  mov(MaskReg, Mask);
//  mov(Offset, SizedZero);
//
//  // We sadly need to spill a reg for this for the time being
//  // TODO: Remove when scratch registers can be allocated
//  //       explicitly.
//  SpillStaticRegs(false, 1U << Mask.GetCode());
//  mov(Mask, SizedZero);
//
//  // Main loop
//  bind(&NextBit);
//  rbit(BitReg, MaskReg);
//  clz(BitReg, BitReg);
//  sub(SubMaskReg, MaskReg, 1);
//  ands(MaskReg, SubMaskReg, MaskReg);
//  lsrv(BitReg, Input, BitReg);
//  and_(BitReg, BitReg, 1);
//  lslv(BitReg, BitReg, Offset);
//  add(Offset, Offset, 1);
//  orr(Mask, BitReg, Mask);
//  b(&NextBit, Condition::ne);
//  mov(Dest, Mask);
//  // Restore our mask register before leaving
//  // TODO: Also remove along with above TODO.
//  FillStaticRegs(false, 1U << Mask.GetCode());
//  b(&Done);
//
//  // Early exit
//  bind(&EarlyExit);
//  mov(Dest, SizedZero);
//
//  // All done with nothing to do.
//  bind(&Done);
//}
//
DEF_OP(LDiv) {
  auto Op = IROp->C<IR::IROp_LDiv>();
  uint8_t OpSize = IROp->Size;
  // Each source is OpSize in size
  // So you can have up to a 128bit divide from x86-64
  auto Size = OpSize;
  switch (Size) {
    case 2: {
      SLLI(TMP1, GetReg(Op->Upper.ID()), 16);
      UXTH(TMP2, GetReg(Op->Lower.ID()));
      OR(TMP1, TMP1, TMP2);
      SXTH(TMP2, GetReg(Op->Divisor.ID()));
      DIVW(GetReg(Node), TMP1, TMP2);
      UXTH(GetReg(Node), GetReg(Node));
    break;
    }
    case 4: {
      SLLI64(TMP1, GetReg(Op->Upper.ID()), 32);
      UXTW(TMP2, GetReg(Op->Lower.ID()));
      OR(TMP1, TMP1, TMP2);
      SXTW(TMP2, GetReg(Op->Divisor.ID()));
      DIV(GetReg(Node), TMP1, TMP2);
      UXTW(GetReg(Node), GetReg(Node));
    break;
    }
    case 8: {
      PushDynamicRegsAndLR();

      MV(a0, GetReg(Op->Upper.ID()));
      MV(a1, GetReg(Op->Lower.ID()));
      MV(a2, GetReg(Op->Divisor.ID()));

      SpillStaticRegs();
      LD(ra, offsetof(FEXCore::Core::CpuStateFrame, Pointers.RISCV.LDIV), STATE);
      JALR(ra);
      MV(TMP1, a0);
      FillStaticRegs();

      // Result is now in x0
      // Fix the stack and any values that were stepped on
      PopDynamicRegsAndLR();

      // Move result to its destination register
      MV(GetReg(Node), TMP1);
    break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown LDIV Size: {}", Size); break;
  }
}

DEF_OP(LUDiv) {
  auto Op = IROp->C<IR::IROp_LUDiv>();
  uint8_t OpSize = IROp->Size;
  // Each source is OpSize in size
  // So you can have up to a 128bit divide from x86-64
  auto Size = OpSize;
  switch (Size) {
    case 2: {
      SLLI(TMP1, GetReg(Op->Upper.ID()), 16);
      UXTH(TMP2, GetReg(Op->Lower.ID()));
      OR(TMP1, TMP1, TMP2);
      UXTH(TMP2, GetReg(Op->Divisor.ID()));
      DIVUW(GetReg(Node), TMP1, TMP2);
      UXTH(GetReg(Node), GetReg(Node));
    break;
    }
    case 4: {
      SLLI64(TMP1, GetReg(Op->Upper.ID()), 32);
      UXTW(TMP2, GetReg(Op->Lower.ID()));
      OR(TMP1, TMP1, TMP2);
      UXTW(TMP2, GetReg(Op->Divisor.ID()));
      DIVU(GetReg(Node), TMP1, TMP2);
      UXTW(GetReg(Node), GetReg(Node));
    break;
    }
    case 8: {
      PushDynamicRegsAndLR();

      MV(a0, GetReg(Op->Upper.ID()));
      MV(a1, GetReg(Op->Lower.ID()));
      MV(a2, GetReg(Op->Divisor.ID()));

      LD(ra, offsetof(FEXCore::Core::CpuStateFrame, Pointers.RISCV.LUDIV), STATE);

      SpillStaticRegs();
      JALR(ra);
      MV(TMP1, a0);
      FillStaticRegs();

      // Result is now in x0
      // Fix the stack and any values that were stepped on
      PopDynamicRegsAndLR();

      // Move result to its destination register
      MV(GetReg(Node), TMP1);
    break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown LUDIV Size: {}", Size); break;
  }
}

DEF_OP(LRem) {
  auto Op = IROp->C<IR::IROp_LRem>();
  uint8_t OpSize = IROp->Size;
  // Each source is OpSize in size
  // So you can have up to a 128bit divide from x86-64
  auto Size = OpSize;
  switch (Size) {
    case 2: {
      SLLI(TMP1, GetReg(Op->Upper.ID()), 16);
      UXTH(TMP2, GetReg(Op->Lower.ID()));
      OR(TMP1, TMP1, TMP2);
      SXTH(TMP2, GetReg(Op->Divisor.ID()));
      REMW(GetReg(Node), TMP1, TMP2);
      UXTH(GetReg(Node), GetReg(Node));
    break;
    }
    case 4: {
      SLLI64(TMP1, GetReg(Op->Upper.ID()), 32);
      UXTW(TMP2, GetReg(Op->Lower.ID()));
      OR(TMP1, TMP1, TMP2);
      SXTW(TMP2, GetReg(Op->Divisor.ID()));
      REM(GetReg(Node), TMP1, TMP2);
      UXTW(GetReg(Node), GetReg(Node));
    break;
    }
    case 8: {
      PushDynamicRegsAndLR();

      MV(a0, GetReg(Op->Upper.ID()));
      MV(a1, GetReg(Op->Lower.ID()));
      MV(a2, GetReg(Op->Divisor.ID()));

      LD(ra, offsetof(FEXCore::Core::CpuStateFrame, Pointers.RISCV.LREM), STATE);

      SpillStaticRegs();
      JALR(ra);
      MV(TMP1, a0);
      FillStaticRegs();

      // Result is now in x0
      // Fix the stack and any values that were stepped on
      PopDynamicRegsAndLR();

      // Move result to its destination register
      MV(GetReg(Node), TMP1);
    break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown LREM Size: {}", Size); break;
  }
}

DEF_OP(LURem) {
  auto Op = IROp->C<IR::IROp_LURem>();
  uint8_t OpSize = IROp->Size;
  // Each source is OpSize in size
  // So you can have up to a 128bit divide from x86-64
  switch (OpSize) {
    case 2: {
      SLLI(TMP1, GetReg(Op->Upper.ID()), 16);
      UXTH(TMP2, GetReg(Op->Lower.ID()));
      OR(TMP1, TMP1, TMP2);
      UXTH(TMP2, GetReg(Op->Divisor.ID()));
      REMUW(GetReg(Node), TMP1, TMP2);
      UXTH(GetReg(Node), GetReg(Node));
    break;
    }
    case 4: {
      SLLI64(TMP1, GetReg(Op->Upper.ID()), 32);
      UXTW(TMP2, GetReg(Op->Lower.ID()));
      OR(TMP1, TMP1, TMP2);
      UXTW(TMP2, GetReg(Op->Divisor.ID()));
      REMU(GetReg(Node), TMP1, TMP2);
      UXTW(GetReg(Node), GetReg(Node));
    break;
    }
    case 8: {
      PushDynamicRegsAndLR();

      MV(a0, GetReg(Op->Upper.ID()));
      MV(a1, GetReg(Op->Lower.ID()));
      MV(a2, GetReg(Op->Divisor.ID()));

      LD(ra, offsetof(FEXCore::Core::CpuStateFrame, Pointers.RISCV.LUREM), STATE);

      SpillStaticRegs();
      JALR(ra);
      MV(TMP1, a0);
      FillStaticRegs();

      // Result is now in x0
      // Fix the stack and any values that were stepped on
      PopDynamicRegsAndLR();

      // Move result to its destination register
      MV(GetReg(Node), TMP1);
    break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown LUREM Size: {}", OpSize); break;
  }
}

DEF_OP(Not) {
  auto Op = IROp->C<IR::IROp_Not>();
  uint8_t OpSize = IROp->Size;
  switch (OpSize) {
    case 4:
      NOT(GetReg(Node), GetReg(Op->Header.Args[0].ID()));
      UXTW(GetReg(Node), GetReg(Node));
      break;
    case 8:
      NOT(GetReg(Node), GetReg(Op->Header.Args[0].ID()));
      break;
    default: LOGMAN_MSG_A_FMT("Unsupported Not size: {}", OpSize);
  }
}

DEF_OP(Popcount) {
  auto Op = IROp->C<IR::IROp_Popcount>();
  uint8_t OpSize = IROp->Size;
  // XXX:
  LoadConstant(GetReg(Node), 0);
  //switch (OpSize) {
  //  case 0x1:
  //    fmov(VTMP1.S(), GetReg<RA_32>(Op->Header.Args[0].ID()));
  //    // only use lowest byte
  //    cnt(VTMP1.V8B(), VTMP1.V8B());
  //    break;
  //  case 0x2:
  //    fmov(VTMP1.S(), GetReg<RA_32>(Op->Header.Args[0].ID()));
  //    cnt(VTMP1.V8B(), VTMP1.V8B());
  //    // only count two lowest bytes
  //    addp(VTMP1.V8B(), VTMP1.V8B(), VTMP1.V8B());
  //    break;
  //  case 0x4:
  //    fmov(VTMP1.S(), GetReg<RA_32>(Op->Header.Args[0].ID()));
  //    cnt(VTMP1.V8B(), VTMP1.V8B());
  //    // fmov has zero extended, unused bytes are zero
  //    addv(VTMP1.B(), VTMP1.V8B());
  //    break;
  //  case 0x8:
  //    fmov(VTMP1.D(), GetReg<RA_64>(Op->Header.Args[0].ID()));
  //    cnt(VTMP1.V8B(), VTMP1.V8B());
  //    // fmov has zero extended, unused bytes are zero
  //    addv(VTMP1.B(), VTMP1.V8B());
  //    break;
  //  default: LOGMAN_MSG_A_FMT("Unsupported Popcount size: {}", OpSize);
  //}

  //auto Dst = GetReg<RA_32>(Node);
  //umov(Dst.W(), VTMP1.B(), 0);
}

//DEF_OP(FindLSB) {
//  auto Op = IROp->C<IR::IROp_FindLSB>();
//  uint8_t OpSize = IROp->Size;
//  auto Dst = GetReg<RA_64>(Node);
//  auto Src = GetReg<RA_64>(Op->Header.Args[0].ID());
//  if (OpSize != 8) {
//    ubfx(TMP1, Src, 0, OpSize * 8);
//    cmp(TMP1, 0);
//    rbit(TMP1, TMP1);
//    clz(Dst, TMP1);
//    csinv(Dst, Dst, xzr, ne);
//  }
//  else {
//    rbit(TMP1, Src);
//    cmp(Src, 0);
//    clz(Dst, TMP1);
//    csinv(Dst, Dst, xzr, ne);
//  }
//}
//
//DEF_OP(FindMSB) {
//  auto Op = IROp->C<IR::IROp_FindMSB>();
//  uint8_t OpSize = IROp->Size;
//  auto Dst = GetReg<RA_64>(Node);
//  switch (OpSize) {
//    case 2:
//      movz(TMP1, OpSize * 8 - 1);
//      lsl(Dst.W(), GetReg<RA_32>(Op->Header.Args[0].ID()), 16);
//      orr(Dst.W(), Dst.W(), 0x8000);
//      clz(Dst.W(), Dst.W());
//      sub(Dst, TMP1, Dst);
//    break;
//    case 4:
//      movz(TMP1, OpSize * 8 - 1);
//      clz(Dst.W(), GetReg<RA_32>(Op->Header.Args[0].ID()));
//      sub(Dst, TMP1, Dst);
//      break;
//    case 8:
//      movz(TMP1, OpSize * 8 - 1);
//      clz(Dst, GetReg<RA_64>(Op->Header.Args[0].ID()));
//      sub(Dst, TMP1, Dst);
//      break;
//    default: LOGMAN_MSG_A_FMT("Unknown FindMSB size: {}", OpSize); break;
//  }
//}
//
DEF_OP(FindTrailingZeros) {
  auto Op = IROp->C<IR::IROp_FindTrailingZeros>();
  uint8_t OpSize = IROp->Size;
  switch (OpSize) {
    case 2:
      LoadConstant(TMP1, 0x1'0000ULL);
      OR(GetReg(Node), GetReg(Op->Header.Args[0].ID()), TMP1);
      CTZW(GetReg(Node), GetReg(Op->Header.Args[0].ID()));
    break;
    case 4:
      CTZW(GetReg(Node), GetReg(Op->Header.Args[0].ID()));
      break;
    case 8:
      CTZ(GetReg(Node), GetReg(Op->Header.Args[0].ID()));
      break;
    default: LOGMAN_MSG_A_FMT("Unknown FindTrailingZeros size: {}", OpSize); break;
  }
}

DEF_OP(CountLeadingZeroes) {
  auto Op = IROp->C<IR::IROp_CountLeadingZeroes>();
  uint8_t OpSize = IROp->Size;
  switch (OpSize) {
    case 2:
      SLLI(GetReg(Node), GetReg(Op->Header.Args[0].ID()), 1);
      ORI(GetReg(Node), GetReg(Node), 1);
      SLLI(GetReg(Node), GetReg(Node), 15);
      CLZW(GetReg(Node), GetReg(Node));
      break;
    case 4:
      CLZW(GetReg(Node), GetReg(Op->Header.Args[0].ID()));
      break;
    case 8:
      CLZ(GetReg(Node), GetReg(Op->Header.Args[0].ID()));
      break;
    default: LOGMAN_MSG_A_FMT("Unknown CountLeadingZeroes size: {}", OpSize); break;
  }
}

DEF_OP(Rev) {
  auto Op = IROp->C<IR::IROp_Rev>();
  uint8_t OpSize = IROp->Size;
  switch (OpSize) {
    case 2:
      REV8_64(GetReg(Node), GetReg(Op->Header.Args[0].ID()));
      SRLI64(GetReg(Node), GetReg(Node), 48);
    break;
    case 4:
      REV8_64(GetReg(Node), GetReg(Op->Header.Args[0].ID()));
      SRLI64(GetReg(Node), GetReg(Node), 32);
      break;
    case 8:
      REV8_64(GetReg(Node), GetReg(Op->Header.Args[0].ID()));
      break;
    default: LOGMAN_MSG_A_FMT("Unknown REV size: {}", OpSize); break;
  }
}

//DEF_OP(Bfi) {
//  auto Op = IROp->C<IR::IROp_Bfi>();
//  uint8_t OpSize = IROp->Size;
//  switch (OpSize) {
//    case 1:
//    case 2:
//    case 4: {
//      auto Dst = GetReg<RA_32>(Node);
//      mov(TMP1.W(), GetReg<RA_32>(Op->Header.Args[0].ID()));
//      bfi(TMP1.W(), GetReg<RA_32>(Op->Header.Args[1].ID()), Op->lsb, Op->Width);
//      ubfx(Dst, TMP1.W(), 0, OpSize * 8);
//      break;
//    }
//    case 8:
//      mov(TMP1, GetReg<RA_64>(Op->Header.Args[0].ID()));
//      bfi(TMP1, GetReg<RA_64>(Op->Header.Args[1].ID()), Op->lsb, Op->Width);
//      mov(GetReg<RA_64>(Node), TMP1);
//      break;
//    default: LOGMAN_MSG_A_FMT("Unknown BFI size: {}", OpSize); break;
//  }
//}
//
DEF_OP(Bfe) {
  auto Op = IROp->C<IR::IROp_Bfe>();
  LOGMAN_THROW_A_FMT(IROp->Size <= 8, "OpSize is too large for BFE: {}", IROp->Size);
  LOGMAN_THROW_A_FMT(Op->Width != 0, "Invalid BFE width of 0");

  UBFX(GetReg(Node), GetReg(Op->Header.Args[0].ID()), Op->lsb, Op->Width);
}

DEF_OP(Sbfe) {
  auto Op = IROp->C<IR::IROp_Bfe>();
  SBFX(GetReg(Node), GetReg(Op->Header.Args[0].ID()), Op->lsb, Op->Width);
}

//#define GRCMP(Node) (Op->CompareSize == 4 ? GetReg<RA_32>(Node) : GetReg<RA_64>(Node))
//
//#define GRFCMP(Node) (Op->CompareSize == 4 ? GetDst(Node).S() : GetDst(Node).D())
//
//Condition MapSelectCC(IR::CondClassType Cond) {
//  switch (Cond.Val) {
//  case FEXCore::IR::COND_EQ: return Condition::eq;
//  case FEXCore::IR::COND_NEQ: return Condition::ne;
//  case FEXCore::IR::COND_SGE: return Condition::ge;
//  case FEXCore::IR::COND_SLT: return Condition::lt;
//  case FEXCore::IR::COND_SGT: return Condition::gt;
//  case FEXCore::IR::COND_SLE: return Condition::le;
//  case FEXCore::IR::COND_UGE: return Condition::cs;
//  case FEXCore::IR::COND_ULT: return Condition::cc;
//  case FEXCore::IR::COND_UGT: return Condition::hi;
//  case FEXCore::IR::COND_ULE: return Condition::ls;
//  case FEXCore::IR::COND_FLU: return Condition::lt;
//  case FEXCore::IR::COND_FGE: return Condition::ge;
//  case FEXCore::IR::COND_FLEU:return Condition::le;
//  case FEXCore::IR::COND_FGT: return Condition::gt;
//  case FEXCore::IR::COND_FU:  return Condition::vs;
//  case FEXCore::IR::COND_FNU: return Condition::vc;
//  case FEXCore::IR::COND_VS:
//  case FEXCore::IR::COND_VC:
//  case FEXCore::IR::COND_MI:
//  case FEXCore::IR::COND_PL:
//  default:
//  LOGMAN_MSG_A_FMT("Unsupported compare type");
//  return Condition::nv;
//  }
//}
//
DEF_OP(Select) {
  auto Op = IROp->C<IR::IROp_Select>();

  uint64_t Const;

  // XXX:
  LoadConstant(GetReg(Node), 0);

  //if (IsGPR(Op->Cmp1.ID())) {
  //  if (IsInlineConstant(Op->Cmp2, &Const))
  //    cmp(GRCMP(Op->Cmp1.ID()), Const);
  //  else
  //    cmp(GRCMP(Op->Cmp1.ID()), GRCMP(Op->Cmp2.ID()));
  //} else if (IsFPR(Op->Cmp1.ID())) {
  //  fcmp(GRFCMP(Op->Cmp1.ID()), GRFCMP(Op->Cmp2.ID()));
  //} else {
  //  LOGMAN_MSG_A_FMT("Select: Expected GPR or FPR");
  //}

  //auto cc = MapSelectCC(Op->Cond);

  //uint64_t const_true, const_false;
  //bool is_const_true = IsInlineConstant(Op->TrueVal, &const_true);
  //bool is_const_false = IsInlineConstant(Op->FalseVal, &const_false);

  //if (is_const_true || is_const_false) {
  //  if (is_const_false != true || is_const_true != true || const_true != 1 || const_false != 0) {
  //    LOGMAN_MSG_A_FMT("Select: Unsupported compare inline parameters");
  //  }
  //  cset(GRS(Node), cc);
  //} else {
  //  csel(GRS(Node), GRS(Op->TrueVal.ID()), GRS(Op->FalseVal.ID()), cc);
  //}
}

DEF_OP(VExtractToGPR) {
  auto Op = IROp->C<IR::IROp_VExtractToGPR>();
  auto Src1 = GetVIndex(Op->Header.Args[0].ID());
  LDUSize(Op->Header.ElementSize, GetReg(Node), Src1 * 16 + (Op->Index * IROp->Size), FPRSTATE);
}

//DEF_OP(Float_ToGPR_ZS) {
//  auto Op = IROp->C<IR::IROp_Float_ToGPR_ZS>();
//  aarch64::Register Dst{};
//  aarch64::VRegister Src{};
//  if (Op->SrcElementSize == 8) {
//    Src = GetSrc(Op->Header.Args[0].ID()).D();
//  }
//  else {
//    Src = GetSrc(Op->Header.Args[0].ID()).S();
//  }
//
//  if (IROp->Size == 8) {
//    Dst = GetReg<RA_64>(Node);
//  }
//  else {
//    Dst = GetReg<RA_32>(Node);
//  }
//
//  fcvtzs(Dst, Src);
//}
//
//DEF_OP(Float_ToGPR_S) {
//  auto Op = IROp->C<IR::IROp_Float_ToGPR_S>();
//
//  aarch64::Register Dst{};
//  aarch64::VRegister Src{};
//  if (Op->SrcElementSize == 8) {
//    frinti(VTMP1.D(), GetSrc(Op->Header.Args[0].ID()).D());
//    Src = VTMP1.D();
//  }
//  else {
//    frinti(VTMP1.S(), GetSrc(Op->Header.Args[0].ID()).S());
//    Src = VTMP1.S();
//  }
//
//  if (IROp->Size == 8) {
//    Dst = GetReg<RA_64>(Node);
//  }
//  else {
//    Dst = GetReg<RA_32>(Node);
//  }
//
//  fcvtzs(Dst, Src);
//}
//
//DEF_OP(FCmp) {
//  auto Op = IROp->C<IR::IROp_FCmp>();
//
//  if (Op->ElementSize == 4) {
//    fcmp(GetSrc(Op->Header.Args[0].ID()).S(), GetSrc(Op->Header.Args[1].ID()).S());
//  }
//  else {
//    fcmp(GetSrc(Op->Header.Args[0].ID()).D(), GetSrc(Op->Header.Args[1].ID()).D());
//  }
//  auto Dst = GetReg<RA_64>(Node);
//
//  bool set = false;
//
//  if (Op->Flags & (1 << IR::FCMP_FLAG_EQ)) {
//    LOGMAN_THROW_A_FMT(IR::FCMP_FLAG_EQ == 0, "IR::FCMP_FLAG_EQ must equal 0");
//    // EQ or unordered
//    cset(Dst, Condition::eq); // Z = 1
//    csinc(Dst, Dst, xzr, Condition::vc); // IF !V ? Z : 1
//    set = true;
//  }
//
//  if (Op->Flags & (1 << IR::FCMP_FLAG_LT)) {
//    // LT or unordered
//    cset(TMP2, Condition::lt);
//    if (!set) {
//      lsl(Dst, TMP2, IR::FCMP_FLAG_LT);
//      set = true;
//    } else {
//      bfi(Dst, TMP2, IR::FCMP_FLAG_LT, 1);
//    }
//  }
//
//  if (Op->Flags & (1 << IR::FCMP_FLAG_UNORDERED)) {
//    cset(TMP2, Condition::vs);
//    if (!set) {
//      lsl(Dst, TMP2, IR::FCMP_FLAG_UNORDERED);
//      set = true;
//    } else {
//      bfi(Dst, TMP2, IR::FCMP_FLAG_UNORDERED, 1);
//    }
//  }
//}

#undef DEF_OP

void RISCVJITCore::RegisterALUHandlers() {
#define REGISTER_OP(op, x) OpHandlers[FEXCore::IR::IROps::OP_##op] = &RISCVJITCore::Op_##x
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
  //REGISTER_OP(PDEP,              PDep);
  //REGISTER_OP(PEXT,              PExt);
  REGISTER_OP(LDIV,              LDiv);
  REGISTER_OP(LUDIV,             LUDiv);
  REGISTER_OP(LREM,              LRem);
  REGISTER_OP(LUREM,             LURem);
  REGISTER_OP(NOT,               Not);
  REGISTER_OP(POPCOUNT,          Popcount);
  //REGISTER_OP(FINDLSB,           FindLSB);
  //REGISTER_OP(FINDMSB,           FindMSB);
  REGISTER_OP(FINDTRAILINGZEROS, FindTrailingZeros);
  REGISTER_OP(COUNTLEADINGZEROES, CountLeadingZeroes);
  REGISTER_OP(REV,               Rev);
  //REGISTER_OP(BFI,               Bfi);
  REGISTER_OP(BFE,               Bfe);
  REGISTER_OP(SBFE,              Sbfe);
  REGISTER_OP(SELECT,            Select);
  REGISTER_OP(VEXTRACTTOGPR,     VExtractToGPR);
  //REGISTER_OP(FLOAT_TOGPR_ZS,    Float_ToGPR_ZS);
  //REGISTER_OP(FLOAT_TOGPR_S,     Float_ToGPR_S);
  //REGISTER_OP(FCMP,              FCmp);

#undef REGISTER_OP
}

}
