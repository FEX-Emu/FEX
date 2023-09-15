/*
$info$
tags: backend|x86-64
$end_info$
*/

#include "Interface/Core/JIT/x86_64/JITClass.h"
#include "Interface/Core/Dispatcher/X86Dispatcher.h"

#include <FEXCore/IR/IR.h>
#include <FEXCore/Utils/LogManager.h>

#include <array>
#include <stdint.h>
#include <utility>

namespace FEXCore::CPU {

#define GRS(Node) (IROp->Size <= 4 ? GetSrc<RA_32>(Node) : GetSrc<RA_64>(Node))
#define GRD(Node) (IROp->Size <= 4 ? GetDst<RA_32>(Node) : GetDst<RA_64>(Node))
#define GRCMP(Node) (Op->CompareSize == 4 ? GetSrc<RA_32>(Node) : GetSrc<RA_64>(Node))

#define DEF_OP(x) void X86JITCore::Op_##x(IR::IROp_Header *IROp, IR::NodeID Node)
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
    default: LOGMAN_MSG_A_FMT("Unhandled Truncation size: {}", IROp->Size); break;
  }
}

DEF_OP(Constant) {
  auto Op = IROp->C<IR::IROp_Constant>();
  mov(GetDst<RA_64>(Node), Op->Constant);
}

DEF_OP(EntrypointOffset) {
  auto Op = IROp->C<IR::IROp_EntrypointOffset>();

  auto Constant = Entry + Op->Offset;
  uint64_t Mask = ~0ULL;
  uint8_t OpSize = IROp->Size;
  if (OpSize == 4) {
    Mask = 0xFFFF'FFFFULL;
  }

  mov(GetDst<RA_64>(Node), Constant & Mask);
}

DEF_OP(InlineConstant) {
  //nop
}

DEF_OP(InlineEntrypointOffset) {
  //nop
}

DEF_OP(CycleCounter) {
#ifdef DEBUG_CYCLES
  mov (GetDst<RA_64>(Node), 0);
#else
  rdtsc();
  shl(rdx, 32);
  or_(rax, rdx);
  mov (GetDst<RA_64>(Node), rax);
#endif
}

DEF_OP(Add) {
  auto Op = IROp->C<IR::IROp_Add>();
  const uint8_t OpSize = IROp->Size;

  mov(rax, GetSrc<RA_64>(Op->Src1.ID()));

  uint64_t Const;
  if (IsInlineConstant(Op->Src2, &Const)) {
    switch (OpSize) {
    case 4:
      add(eax, Const);
      break;
    case 8:
      add(rax, Const);
      break;
    default:  LOGMAN_MSG_A_FMT("Unhandled Add size: {}", OpSize);
      break;
    }
  } else {
    switch (OpSize) {
    case 4:
      add(eax, GetSrc<RA_32>(Op->Src2.ID()));
      break;
    case 8:
      add(rax, GetSrc<RA_64>(Op->Src2.ID()));
      break;
    default:  LOGMAN_MSG_A_FMT("Unhandled Add size: {}", OpSize);
      break;
    }
  }
  mov(GetDst<RA_64>(Node), rax);
}

DEF_OP(AddNZCV) {
  auto Op = IROp->C<IR::IROp_AddNZCV>();
  const uint8_t OpSize = Op->Size;

  // Results returned in Arm64 NZCV format
  // N = Sign bit
  // Z = Is Zero
  // C = Carry occured (Unsigned result can't fit within resulting register)
  // V = Overflow occured (Signed result can't fit in to resulting register)

  Xbyak::Reg Src2 = TMP2;
  uint64_t Const;
  if (IsInlineConstant(Op->Src2, &Const)) {
    mov(Src2, Const);
  }
  else {
    Src2 = GetSrc<RA_64>(Op->Src2.ID());
  }

  switch (OpSize) {
  case 4:
    mov(TMP1.cvt32(), GetSrc<RA_32>(Op->Src1.ID()));
    add(TMP1.cvt32(), Src2.cvt32());
    break;
  case 8:
    mov(TMP1.cvt64(), GetSrc<RA_64>(Op->Src1.ID()));
    add(TMP1.cvt64(), Src2.cvt64());
    break;
  default:  LOGMAN_MSG_A_FMT("Unhandled {} size: {}", __func__, OpSize);
    break;
  }

  mov(TMP1, 0);
  mov(TMP2, 0);
  mov(TMP3, 0);
  mov(TMP4, 0);
  sets(TMP1.cvt8());
  setz(TMP2.cvt8());
  setc(TMP3.cvt8());
  seto(TMP4.cvt8());
  // Flags NZCV in Tmps 1,2,3,4 respectively
  shl(TMP1, 31);
  shl(TMP2, 30);
  shl(TMP3, 29);
  shl(TMP4, 28);
  or_(TMP1, TMP2);
  or_(TMP1, TMP3);
  or_(TMP1, TMP4);
  mov(GetDst<RA_64>(Node), TMP1);
}

DEF_OP(TestNZ) {
  auto Op = IROp->C<IR::IROp_TestNZ>();
  const uint8_t OpSize = Op->Size;

  // Results returned in Arm64 NZCV format
  // N = Sign bit
  // Z = Is Zero
  // CV = 00
  switch (OpSize) {
  case 4:
    mov(TMP1.cvt32(), GetSrc<RA_32>(Op->Src1.ID()));
    shr(TMP1.cvt32(), OpSize * 8 - 1);
    shl(TMP1.cvt32(), 31);
    cmp(GetSrc<RA_32>(Op->Src1.ID()), 0);
    mov(GetDst<RA_32>(Node), 0);
    sete(GetDst<RA_32>(Node).cvt8());
    shl(GetDst<RA_32>(Node), 30);
    or_(GetDst<RA_32>(Node), TMP1.cvt32());
    break;
  case 8:
    mov(TMP1, GetSrc<RA_64>(Op->Src1.ID()));
    shr(TMP1, OpSize * 8 - 1);
    shl(TMP1, 31);
    cmp(GetSrc<RA_64>(Op->Src1.ID()), 0);
    mov(GetDst<RA_64>(Node), 0);
    sete(GetDst<RA_64>(Node).cvt8());
    shl(GetDst<RA_64>(Node), 30);
    or_(GetDst<RA_64>(Node), TMP1);
    break;
  default:  LOGMAN_MSG_A_FMT("Unhandled {} size: {}", __func__, OpSize);
    break;
  }
}

DEF_OP(Sub) {
  auto Op = IROp->C<IR::IROp_Sub>();
  const uint8_t OpSize = IROp->Size;

  mov(rax, GetSrc<RA_64>(Op->Src1.ID()));

  uint64_t Const;
  if (IsInlineConstant(Op->Src2, &Const)) {
    switch (OpSize) {
    case 4:
      sub(eax, Const);
      break;
    case 8:
      sub(rax, Const);
      break;
    default:  LOGMAN_MSG_A_FMT("Unhandled Sub size: {}", OpSize);
      break;
    }
  } else {
    switch (OpSize) {
    case 4:
      sub(eax, GetSrc<RA_32>(Op->Src2.ID()));
      break;
    case 8:
      sub(rax, GetSrc<RA_64>(Op->Src2.ID()));
      break;
    default:  LOGMAN_MSG_A_FMT("Unhandled Sub size: {}", OpSize);
      break;
    }
  }
  mov(GetDst<RA_64>(Node), rax);
}

DEF_OP(SubNZCV) {
  auto Op = IROp->C<IR::IROp_SubNZCV>();
  const uint8_t OpSize = Op->Size;

  // Results returned in Arm64 NZCV format
  // N = Sign bit
  // Z = Is Zero
  // C = Carry occured (Unsigned result can't fit within resulting register)
  // V = Overflow occured (Signed result can't fit in to resulting register)

  Xbyak::Reg Src1 = TMP1;
  Xbyak::Reg Src2 = TMP2;
  uint64_t Const;
  if (IsInlineConstant(Op->Src1, &Const)) {
    mov(Src1, Const);
  }
  else {
    Src1 = GetSrc<RA_64>(Op->Src1.ID());
  }

  if (IsInlineConstant(Op->Src2, &Const)) {
    mov(Src2, Const);
  }
  else {
    Src2 = GetSrc<RA_64>(Op->Src2.ID());
  }

  switch (OpSize) {
  case 4:
    cmp(Src1.cvt32(), Src2.cvt32());
    break;
  case 8:
    cmp(Src1.cvt64(), Src2.cvt64());
    break;
  default:  LOGMAN_MSG_A_FMT("Unhandled {} size: {}", __func__, OpSize);
    break;
  }

  if (!Op->InvertCarry) {
    cmc();
  }

  mov(TMP1, 0);
  mov(TMP2, 0);
  mov(TMP3, 0);
  mov(TMP4, 0);
  sets(TMP1.cvt8());
  setz(TMP2.cvt8());
  setc(TMP3.cvt8());
  seto(TMP4.cvt8());
  // Flags NZCV in Tmps 1,2,3,4 respectively
  shl(TMP1, 31);
  shl(TMP2, 30);
  shl(TMP3, 29);
  shl(TMP4, 28);
  or_(TMP1, TMP2);
  or_(TMP1, TMP3);
  or_(TMP1, TMP4);
  mov(GetDst<RA_64>(Node), TMP1);
}


DEF_OP(Neg) {
  auto Op = IROp->C<IR::IROp_Neg>();
  const uint8_t OpSize = IROp->Size;

  Xbyak::Reg Src;
  Xbyak::Reg Dst;
  switch (OpSize) {
  case 4:
    Src = GetSrc<RA_32>(Op->Src.ID());
    Dst = GetDst<RA_32>(Node);
    break;
  case 8:
    Src = GetSrc<RA_64>(Op->Src.ID());
    Dst = GetDst<RA_64>(Node);
    break;
  default:  LOGMAN_MSG_A_FMT("Unhandled Neg size: {}", OpSize);
    break;
  }
  mov(Dst, Src);
  neg(Dst);
}

DEF_OP(Abs) {
  auto Op = IROp->C<IR::IROp_Abs>();
  const uint8_t OpSize = IROp->Size;

  Xbyak::Reg Src;
  Xbyak::Reg Dst;
  switch (OpSize) {
  case 4:
    Src = GetSrc<RA_32>(Op->Src.ID());
    Dst = GetDst<RA_32>(Node);
    break;
  case 8:
    Src = GetSrc<RA_64>(Op->Src.ID());
    Dst = GetDst<RA_64>(Node);
    break;
  default:  LOGMAN_MSG_A_FMT("Unhandled Abs size: {}", OpSize);
    break;
  }
  mov(Dst, Src);
  mov(TMP1, Src);
  neg(Dst);
  cmovs(Dst, TMP1);
}

DEF_OP(Mul) {
  auto Op = IROp->C<IR::IROp_Mul>();
  const uint8_t OpSize = IROp->Size;

  auto Dst = GetDst<RA_64>(Node);

  switch (OpSize) {
  case 4:
    movsxd(rax, GetSrc<RA_32>(Op->Src1.ID()));
    imul(eax, GetSrc<RA_32>(Op->Src2.ID()));
    mov(Dst.cvt32(), eax);
  break;
  case 8:
    mov(rax, GetSrc<RA_64>(Op->Src1.ID()));
    imul(rax, GetSrc<RA_64>(Op->Src2.ID()));
    mov(Dst, rax);
  break;
  default: LOGMAN_MSG_A_FMT("Unknown Mul size: {}", OpSize);
  }
}

DEF_OP(UMul) {
  auto Op = IROp->C<IR::IROp_UMul>();
  const uint8_t OpSize = IROp->Size;

  switch (OpSize) {
  case 4:
    mov(rax, GetSrc<RA_32>(Op->Src1.ID()));
    mul(GetSrc<RA_32>(Op->Src2.ID()));
    mov(GetDst<RA_64>(Node), rax);
  break;
  case 8:
    mov(rax, GetSrc<RA_64>(Op->Src1.ID()));
    mul(GetSrc<RA_64>(Op->Src2.ID()));
    mov(GetDst<RA_64>(Node), rax);
  break;
  default: LOGMAN_MSG_A_FMT("Unknown UMul size: {}", OpSize);
  }
}

DEF_OP(Div) {
  auto Op = IROp->C<IR::IROp_Div>();
  const uint8_t OpSize = IROp->Size;

  // Each source is OpSize in size
  // So you can have up to a 128bit divide from x86-64
  auto Size = OpSize;
  switch (Size) {
  case 1: {
    movsx(ax, GetSrc<RA_8>(Op->Src1.ID()));
    idiv(GetSrc<RA_8>(Op->Src2.ID()));
    movsx(GetDst<RA_32>(Node), al);
  break;
  }
  case 2: {
    mov (ax, GetSrc<RA_16>(Op->Src1.ID()));
    cwd();
    idiv(GetSrc<RA_16>(Op->Src2.ID()));
    movsx(GetDst<RA_32>(Node), ax);
  break;
  }
  case 4: {
    mov (eax, GetSrc<RA_32>(Op->Src1.ID()));
    cdq();
    idiv(GetSrc<RA_32>(Op->Src2.ID()));
    movsxd(GetDst<RA_64>(Node).cvt64(), eax);
  break;
  }
  case 8: {
    mov (rax, GetSrc<RA_64>(Op->Src1.ID()));
    cqo();
    idiv(GetSrc<RA_64>(Op->Src2.ID()));
    mov(GetDst<RA_64>(Node), rax);
  break;
  }
  default: LOGMAN_MSG_A_FMT("Unknown DIV Size: {}", Size); break;
  }
}

DEF_OP(UDiv) {
  auto Op = IROp->C<IR::IROp_UDiv>();
  const uint8_t OpSize = IROp->Size;

  // Each source is OpSize in size
  // So you can have up to a 128bit divide from x86-64
  switch (OpSize) {
  case 1: {
    mov (al, GetSrc<RA_8>(Op->Src1.ID()));
    mov (edx, 0);
    mov (cl, GetSrc<RA_8>(Op->Src2.ID()));
    div(cl);
    movzx(GetDst<RA_32>(Node), al);
  break;
  }
  case 2: {
    mov (ax, GetSrc<RA_16>(Op->Src1.ID()));
    mov (edx, 0);
    mov (cx, GetSrc<RA_16>(Op->Src2.ID()));
    div(cx);
    movzx(GetDst<RA_32>(Node), ax);
  break;
  }
  case 4: {
    mov (eax, GetSrc<RA_32>(Op->Src1.ID()));
    mov (edx, 0);
    mov (ecx, GetSrc<RA_32>(Op->Src2.ID()));
    div(ecx);
    mov(GetDst<RA_32>(Node), eax);
  break;
  }
  case 8: {
    mov (rax, GetSrc<RA_64>(Op->Src1.ID()));
    mov (rdx, 0);
    mov (rcx, GetSrc<RA_64>(Op->Src2.ID()));
    div(rcx);
    mov(GetDst<RA_64>(Node), rax);
  break;
  }
  default: LOGMAN_MSG_A_FMT("Unknown UDIV OpSize: {}", OpSize); break;
  }
}

DEF_OP(Rem) {
  auto Op = IROp->C<IR::IROp_Rem>();
  const uint8_t OpSize = IROp->Size;

  switch (OpSize) {
  case 1: {
    movsx(ax, GetSrc<RA_8>(Op->Src1.ID()));
    idiv(GetSrc<RA_8>(Op->Src2.ID()));
    mov(al, ah);
    movsx(GetDst<RA_32>(Node), al);
  break;
  }
  case 2: {
    mov (ax, GetSrc<RA_16>(Op->Src1.ID()));
    cwd();
    idiv(GetSrc<RA_16>(Op->Src2.ID()));
    movsx(GetDst<RA_32>(Node), dx);
  break;
  }
  case 4: {
    mov (eax, GetSrc<RA_32>(Op->Src1.ID()));
    cdq();
    idiv(GetSrc<RA_32>(Op->Src2.ID()));
    movsxd(GetDst<RA_64>(Node).cvt64(), edx);
  break;
  }
  case 8: {
    mov (rax, GetSrc<RA_64>(Op->Src1.ID()));
    cqo();
    idiv(GetSrc<RA_64>(Op->Src2.ID()));
    mov(GetDst<RA_64>(Node), rdx);
  break;
  }
  default: LOGMAN_MSG_A_FMT("Unknown Rem Size: {}", OpSize); break;
  }
}

DEF_OP(URem) {
  auto Op = IROp->C<IR::IROp_URem>();
  const uint8_t OpSize = IROp->Size;

  // Each source is OpSize in size
  // So you can have up to a 128bit divide from x86-64
  switch (OpSize) {
  case 1: {
    mov (al, GetSrc<RA_8>(Op->Src1.ID()));
    mov (edx, 0);
    mov (cl, GetSrc<RA_8>(Op->Src2.ID()));
    div(cl);
    movzx(GetDst<RA_32>(Node), ah);
  break;
  }
  case 2: {
    mov (ax, GetSrc<RA_16>(Op->Src1.ID()));
    mov (edx, 0);
    mov (cx, GetSrc<RA_16>(Op->Src2.ID()));
    div(cx);
    movzx(GetDst<RA_32>(Node), dx);
  break;
  }
  case 4: {
    mov (eax, GetSrc<RA_32>(Op->Src1.ID()));
    mov (edx, 0);
    mov (ecx, GetSrc<RA_32>(Op->Src2.ID()));
    div(ecx);
    mov(GetDst<RA_32>(Node), edx);
  break;
  }
  case 8: {
    mov (rax, GetSrc<RA_64>(Op->Src1.ID()));
    mov (rdx, 0);
    mov (rcx, GetSrc<RA_64>(Op->Src2.ID()));
    div(rcx);
    mov(GetDst<RA_64>(Node), rdx);
  break;
  }
  default: LOGMAN_MSG_A_FMT("Unknown URem OpSize: {}", OpSize); break;
  }
}

DEF_OP(MulH) {
  auto Op = IROp->C<IR::IROp_MulH>();
  const uint8_t OpSize = IROp->Size;

  switch (OpSize) {
  case 4:
    movsxd(rax, GetSrc<RA_32>(Op->Src1.ID()));
    imul(GetSrc<RA_32>(Op->Src2.ID()));
    movsxd(GetDst<RA_64>(Node).cvt64(), edx);
  break;
  case 8:
    mov(rax, GetSrc<RA_64>(Op->Src1.ID()));
    imul(GetSrc<RA_64>(Op->Src2.ID()));
    mov(GetDst<RA_64>(Node), rdx);
  break;
  default: LOGMAN_MSG_A_FMT("Unknown MulH size: {}", OpSize);
  }
}

DEF_OP(UMulH) {
  auto Op = IROp->C<IR::IROp_UMulH>();
  const uint8_t OpSize = IROp->Size;

  switch (OpSize) {
  case 4:
    mov(rax, GetSrc<RA_32>(Op->Src1.ID()));
    mul(GetSrc<RA_32>(Op->Src2.ID()));
    mov(GetDst<RA_64>(Node), rdx);
  break;
  case 8:
    mov(rax, GetSrc<RA_64>(Op->Src1.ID()));
    mul(GetSrc<RA_64>(Op->Src2.ID()));
    mov(GetDst<RA_64>(Node), rdx);
  break;
  default: LOGMAN_MSG_A_FMT("Unknown UMulH size: {}", OpSize);
  }
}

DEF_OP(Or) {
  auto Op = IROp->C<IR::IROp_Or>();
  auto Dst = GetDst<RA_64>(Node);
  mov(rax, GetSrc<RA_64>(Op->Src1.ID()));

  uint64_t Const;
  if (IsInlineConstant(Op->Src2, &Const)) {
    or_(rax, Const);
  } else {
    or_(rax, GetSrc<RA_64>(Op->Src2.ID()));
  }
  mov(Dst, rax);
}

DEF_OP(Orlshl) {
  auto Op = IROp->C<IR::IROp_Orlshl>();
  auto Dst = GetDst<RA_64>(Node);
  const auto BitShift = Op->BitShift;

  uint64_t Const;
  if (IsInlineConstant(Op->Src2, &Const)) {
    if (IROp->Size == 8) {
      mov(Dst, GetSrc<RA_64>(Op->Src1.ID()));
      or_(Dst, Const << BitShift);
    }
    else {
      mov(Dst.cvt32(), GetSrc<RA_32>(Op->Src1.ID()));
      or_(Dst.cvt32(), Const << BitShift);
    }
  } else {
    if (IROp->Size == 8) {
      mov(TMP2, GetSrc<RA_64>(Op->Src2.ID()));
      mov(Dst, GetSrc<RA_64>(Op->Src1.ID()));
      shl(TMP2, BitShift);
      or_(Dst, TMP2);
    }
    else {
      mov(TMP2.cvt32(), GetSrc<RA_32>(Op->Src2.ID()));
      mov(Dst.cvt32(), GetSrc<RA_32>(Op->Src1.ID()));
      shl(TMP2.cvt32(), BitShift);
      or_(Dst.cvt32(), TMP2.cvt32());
    }
  }
}

DEF_OP(Orlshr) {
  auto Op = IROp->C<IR::IROp_Orlshr>();
  auto Dst = GetDst<RA_64>(Node);
  const auto BitShift = Op->BitShift;

  uint64_t Const;
  if (IsInlineConstant(Op->Src2, &Const)) {
    if (IROp->Size == 8) {
      mov(Dst, GetSrc<RA_64>(Op->Src1.ID()));
      or_(Dst, Const >> BitShift);
    }
    else {
      mov(Dst.cvt32(), GetSrc<RA_32>(Op->Src1.ID()));
      or_(Dst.cvt32(), Const >> BitShift);
    }
  } else {
    if (IROp->Size == 8) {
      mov(TMP2, GetSrc<RA_64>(Op->Src2.ID()));
      mov(Dst, GetSrc<RA_64>(Op->Src1.ID()));
      shr(TMP2, BitShift);
      or_(Dst, TMP2);
    }
    else {
      mov(TMP2.cvt32(), GetSrc<RA_32>(Op->Src2.ID()));
      mov(Dst.cvt32(), GetSrc<RA_32>(Op->Src1.ID()));
      shr(TMP2.cvt32(), BitShift);
      or_(Dst.cvt32(), TMP2.cvt32());
    }
  }
}

DEF_OP(And) {
  auto Op = IROp->C<IR::IROp_And>();
  auto Dst = GetDst<RA_64>(Node);
  mov(rax, GetSrc<RA_64>(Op->Src1.ID()));
  uint64_t Const;
  if (IsInlineConstant(Op->Src2, &Const)) {
    and_(rax, Const);
  } else {
    and_(rax, GetSrc<RA_64>(Op->Src2.ID()));
  }
  mov(Dst, rax);
}

DEF_OP(Andn) {
  auto Op = IROp->C<IR::IROp_Andn>();
  const auto& Lhs = Op->Src1;
  const auto& Rhs = Op->Src2;
  auto Dst = GRD(Node);

  uint64_t Const{};
  if (IsInlineConstant(Rhs, &Const)) {
    mov(Dst, GRS(Lhs.ID()));
    and_(Dst, ~Const);
  } else {
    const auto Temp = IROp->Size <= 4 ? Xbyak::Reg{rax.cvt32()} : Xbyak::Reg{rax};
    mov(Temp, GRS(Rhs.ID()));
    not_(Temp);
    and_(Temp, GRS(Lhs.ID()));
    mov(Dst, Temp);
  }
}

DEF_OP(Xor) {
  auto Op = IROp->C<IR::IROp_Xor>();
  auto Dst = GetDst<RA_64>(Node);
  mov(rax, GetSrc<RA_64>(Op->Src1.ID()));
  uint64_t Const;
  if (IsInlineConstant(Op->Src2, &Const)) {
    xor_(rax, Const);
  } else {
    xor_(rax, GetSrc<RA_64>(Op->Src2.ID()));
  }
  mov(Dst, rax);
}

DEF_OP(Lshl) {
  auto Op = IROp->C<IR::IROp_Lshl>();
  const uint8_t OpSize = IROp->Size;
  const uint8_t Mask = OpSize * 8 - 1;

  uint64_t Const;
  if (IsInlineConstant(Op->Src2, &Const)) {
    Const &= Mask;
    switch (OpSize) {
      case 4:
        mov(GetDst<RA_32>(Node), GetSrc<RA_32>(Op->Src1.ID()));
        shl(GetDst<RA_32>(Node), Const);
        break;
      case 8:
        mov(GetDst<RA_64>(Node), GetSrc<RA_64>(Op->Src1.ID()));
        shl(GetDst<RA_64>(Node), Const);
        break;
      default: LOGMAN_MSG_A_FMT("Unknown LSHL Size: {}\n", OpSize); break;
    };
  } else {
    mov(rcx, GetSrc<RA_64>(Op->Src2.ID()));
    and_(rcx, Mask);

    switch (OpSize) {
      case 4:
        mov(GetDst<RA_32>(Node), GetSrc<RA_32>(Op->Src1.ID()));
        shl(GetDst<RA_32>(Node), cl);
        break;
      case 8:
        mov(GetDst<RA_64>(Node), GetSrc<RA_64>(Op->Src1.ID()));
        shl(GetDst<RA_64>(Node), cl);
        break;
      default: LOGMAN_MSG_A_FMT("Unknown LSHL Size: {}\n", OpSize); break;
    };
  }
}

DEF_OP(Lshr) {
  auto Op = IROp->C<IR::IROp_Lshr>();
  const uint8_t OpSize = IROp->Size;
  const uint8_t Mask = OpSize * 8 - 1;

  uint64_t Const;
  if (IsInlineConstant(Op->Src2, &Const)) {
    Const &= Mask;

    switch (OpSize) {
      case 1:
        movzx(GetDst<RA_32>(Node), GetSrc<RA_8>(Op->Src1.ID()));
        shr(GetDst<RA_32>(Node).cvt8(), Const);
        break;
      case 2:
        movzx(GetDst<RA_32>(Node), GetSrc<RA_16>(Op->Src1.ID()));
        shr(GetDst<RA_32>(Node).cvt16(), Const);
        break;
      case 4:
        mov(GetDst<RA_32>(Node), GetSrc<RA_32>(Op->Src1.ID()));
        shr(GetDst<RA_32>(Node), Const);
        break;
      case 8:
        mov(GetDst<RA_64>(Node), GetSrc<RA_64>(Op->Src1.ID()));
        shr(GetDst<RA_64>(Node), Const);
        break;
      default: LOGMAN_MSG_A_FMT("Unknown Size: {}\n", OpSize); break;
    };

  } else {
    mov (rcx, GetSrc<RA_64>(Op->Src2.ID()));
    and_(rcx, Mask);

    switch (OpSize) {
      case 1:
        movzx(GetDst<RA_32>(Node), GetSrc<RA_8>(Op->Src1.ID()));
        shr(GetDst<RA_32>(Node).cvt8(), cl);
        break;
      case 2:
        movzx(GetDst<RA_32>(Node), GetSrc<RA_16>(Op->Src1.ID()));
        shr(GetDst<RA_32>(Node).cvt16(), cl);
        break;
      case 4:
        mov(GetDst<RA_32>(Node), GetSrc<RA_32>(Op->Src1.ID()));
        shr(GetDst<RA_32>(Node), cl);
        break;
      case 8:
        mov(GetDst<RA_64>(Node), GetSrc<RA_64>(Op->Src1.ID()));
        shr(GetDst<RA_64>(Node), cl);
        break;
      default: LOGMAN_MSG_A_FMT("Unknown Size: {}\n", OpSize); break;
    };
  }
}

DEF_OP(Ashr) {
  auto Op = IROp->C<IR::IROp_Ashr>();
  const uint8_t OpSize = IROp->Size;
  const uint8_t Mask = OpSize * 8 - 1;

  uint64_t Const;
  if (IsInlineConstant(Op->Src2, &Const)) {
    Const &= Mask;

    switch (OpSize) {
    case 1:
      movsx(rax, GetSrc<RA_8>(Op->Src1.ID()));
      sar(al, Const);
      movzx(GetDst<RA_64>(Node), al);
    break;
    case 2:
      movsx(rax, GetSrc<RA_16>(Op->Src1.ID()));
      sar(ax, Const);
      movzx(GetDst<RA_64>(Node), ax);
    break;
    case 4:
      mov(GetDst<RA_32>(Node), GetSrc<RA_32>(Op->Src1.ID()));
      sar(GetDst<RA_32>(Node), Const);
    break;
    case 8:
      mov(GetDst<RA_64>(Node), GetSrc<RA_64>(Op->Src1.ID()));
      sar(GetDst<RA_64>(Node), Const);
    break;
    default: LOGMAN_MSG_A_FMT("Unknown ASHR Size: {}\n", OpSize); break;
    };

  } else {
    mov (rcx, GetSrc<RA_64>(Op->Src2.ID()));
    and_(rcx, Mask);
    switch (OpSize) {
    case 1:
      movsx(rax, GetSrc<RA_8>(Op->Src1.ID()));
      sar(al, cl);
      movzx(GetDst<RA_64>(Node), al);
    break;
    case 2:
      movsx(rax, GetSrc<RA_16>(Op->Src1.ID()));
      sar(ax, cl);
      movzx(GetDst<RA_64>(Node), ax);
    break;
    case 4:
      mov(GetDst<RA_32>(Node), GetSrc<RA_32>(Op->Src1.ID()));
      sar(GetDst<RA_32>(Node), cl);
    break;
    case 8:
      mov(GetDst<RA_64>(Node), GetSrc<RA_64>(Op->Src1.ID()));
      sar(GetDst<RA_64>(Node), cl);
    break;
    default: LOGMAN_MSG_A_FMT("Unknown ASHR Size: {}\n", OpSize); break;
    };
  }
}

DEF_OP(Ror) {
  auto Op = IROp->C<IR::IROp_Ror>();
  const uint8_t OpSize = IROp->Size;
  const uint8_t Mask = OpSize * 8 - 1;

  uint64_t Const;
  if (IsInlineConstant(Op->Src2, &Const)) {
    Const &= Mask;
    switch (OpSize) {
      case 4: {
        mov(eax, GetSrc<RA_32>(Op->Src1.ID()));
        ror(eax, Const);
      break;
      }
      case 8: {
        mov(rax, GetSrc<RA_64>(Op->Src1.ID()));
        ror(rax, Const);
      break;
      }
      default: LOGMAN_MSG_A_FMT("Unknown ROR Size: {}\n", OpSize); break;
    }
  } else {
    mov (rcx, GetSrc<RA_64>(Op->Src2.ID()));
    and_(rcx, Mask);
    switch (OpSize) {
      case 4: {
        mov(eax, GetSrc<RA_32>(Op->Src1.ID()));
        ror(eax, cl);
      break;
      }
      case 8: {
        mov(rax, GetSrc<RA_64>(Op->Src1.ID()));
        ror(rax, cl);
      break;
      }
      default: LOGMAN_MSG_A_FMT("Unknown ROR Size: {}\n", OpSize); break;
    }
  }
  mov(GetDst<RA_64>(Node), rax);
}

DEF_OP(Extr) {
  auto Op = IROp->C<IR::IROp_Extr>();
  const uint8_t OpSize = IROp->Size;

  switch (OpSize) {
    case 4: {
      mov(eax, GetSrc<RA_32>(Op->Upper.ID()));
      mov(ecx, GetSrc<RA_32>(Op->Lower.ID()));
      shrd(ecx, eax, Op->LSB);
      mov(GetDst<RA_32>(Node), ecx);
      break;
    }
    case 8: {
      mov(rax, GetSrc<RA_64>(Op->Upper.ID()));
      mov(rcx, GetSrc<RA_64>(Op->Lower.ID()));
      shrd(rcx, rax, Op->LSB);
      mov(GetDst<RA_64>(Node), rcx);
      break;
    }
  }
}

DEF_OP(PDep) {
  const auto Op = IROp->C<IR::IROp_PExt>();
  const auto OpSize = IROp->Size;

  const auto Input = GRS(Op->Input.ID());
  const auto Mask  = GRS(Op->Mask.ID());
  const auto Dest  = GRD(Node);

  if (OpSize == 4) {
    pdep(Dest.cvt32(), Input.cvt32(), Mask.cvt32());
  } else {
    pdep(Dest.cvt64(), Input.cvt64(), Mask.cvt64());
  }
}

DEF_OP(PExt) {
  const auto Op = IROp->C<IR::IROp_PExt>();
  const auto OpSize = IROp->Size;

  const auto Input = GRS(Op->Input.ID());
  const auto Mask  = GRS(Op->Mask.ID());
  const auto Dest  = GRD(Node);

  if (OpSize == 4) {
    pext(Dest.cvt32(), Input.cvt32(), Mask.cvt32());
  } else {
    pext(Dest.cvt64(), Input.cvt64(), Mask.cvt64());
  }
}

DEF_OP(LDiv) {
  auto Op = IROp->C<IR::IROp_LDiv>();
  const uint8_t OpSize = IROp->Size;

  // Each source is OpSize in size
  // So you can have up to a 128bit divide from x86-64
  switch (OpSize) {
    case 2: {
      mov(eax, GetSrc<RA_16>(Op->Lower.ID()));
      mov(edx, GetSrc<RA_16>(Op->Upper.ID()));
      idiv(GetSrc<RA_16>(Op->Divisor.ID()));
      movsx(GetDst<RA_64>(Node), ax);
      break;
    }
    case 4: {
      mov(eax, GetSrc<RA_32>(Op->Lower.ID()));
      mov(edx, GetSrc<RA_32>(Op->Upper.ID()));
      idiv(GetSrc<RA_32>(Op->Divisor.ID()));
      movsxd(GetDst<RA_64>(Node).cvt64(), eax);
      break;
    }
    case 8: {
      mov(rax, GetSrc<RA_64>(Op->Lower.ID()));
      mov(rdx, GetSrc<RA_64>(Op->Upper.ID()));
      idiv(GetSrc<RA_64>(Op->Divisor.ID()));
      mov(GetDst<RA_64>(Node), rax);
      break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown LDIV OpSize: {}", OpSize); break;
  }
}

DEF_OP(LUDiv) {
  auto Op = IROp->C<IR::IROp_LUDiv>();
  const uint8_t OpSize = IROp->Size;

  // Each source is OpSize in size
  // So you can have up to a 128bit divide from x86-64
  switch (OpSize) {
    case 2: {
      mov (ax, GetSrc<RA_16>(Op->Lower.ID()));
      mov (dx, GetSrc<RA_16>(Op->Upper.ID()));
      div(GetSrc<RA_16>(Op->Divisor.ID()));
      movzx(GetDst<RA_32>(Node), ax);
      break;
    }
    case 4: {
      mov (eax, GetSrc<RA_32>(Op->Lower.ID()));
      mov (edx, GetSrc<RA_32>(Op->Upper.ID()));
      div(GetSrc<RA_32>(Op->Divisor.ID()));
      mov(GetDst<RA_64>(Node), rax);
      break;
    }
    case 8: {
      mov (rax, GetSrc<RA_64>(Op->Lower.ID()));
      mov (rdx, GetSrc<RA_64>(Op->Upper.ID()));
      div(GetSrc<RA_64>(Op->Divisor.ID()));
      mov(GetDst<RA_64>(Node), rax);
      break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown LUDIV OpSize: {}", OpSize); break;
  }
}

DEF_OP(LRem) {
  auto Op = IROp->C<IR::IROp_LRem>();
  const uint8_t OpSize = IROp->Size;

  // Each source is OpSize in size
  // So you can have up to a 128bit divide from x86-64
  switch (OpSize) {
    case 2: {
      mov(ax, GetSrc<RA_16>(Op->Lower.ID()));
      mov(dx, GetSrc<RA_16>(Op->Upper.ID()));
      idiv(GetSrc<RA_16>(Op->Divisor.ID()));
      movsx(GetDst<RA_64>(Node), dx);
      break;
    }
    case 4: {
      mov(eax, GetSrc<RA_32>(Op->Lower.ID()));
      mov(edx, GetSrc<RA_32>(Op->Upper.ID()));
      idiv(GetSrc<RA_32>(Op->Divisor.ID()));
      mov(GetDst<RA_64>(Node), rdx);
      break;
    }
    case 8: {
      mov(rax, GetSrc<RA_64>(Op->Lower.ID()));
      mov(rdx, GetSrc<RA_64>(Op->Upper.ID()));
      idiv(GetSrc<RA_64>(Op->Divisor.ID()));
      mov(GetDst<RA_64>(Node), rdx);
      break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown LREM OpSize: {}", OpSize); break;
  }
}

DEF_OP(LURem) {
  auto Op = IROp->C<IR::IROp_LURem>();
  const uint8_t OpSize = IROp->Size;

  // Each source is OpSize in size
  // So you can have up to a 128bit divide from x86-64
  switch (OpSize) {
    case 2: {
      mov (ax, GetSrc<RA_16>(Op->Lower.ID()));
      mov (dx, GetSrc<RA_16>(Op->Upper.ID()));
      div(GetSrc<RA_16>(Op->Divisor.ID()));
      movzx(GetDst<RA_64>(Node), dx);
      break;
    }
    case 4: {
      mov (eax, GetSrc<RA_32>(Op->Lower.ID()));
      mov (edx, GetSrc<RA_32>(Op->Upper.ID()));
      div(GetSrc<RA_32>(Op->Divisor.ID()));
      mov(GetDst<RA_64>(Node), rdx);
      break;
    }
    case 8: {
      mov (rax, GetSrc<RA_64>(Op->Lower.ID()));
      mov (rdx, GetSrc<RA_64>(Op->Upper.ID()));
      div(GetSrc<RA_64>(Op->Divisor.ID()));
      mov(GetDst<RA_64>(Node), rdx);
      break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown LUREM OpSize: {}", OpSize); break;
  }
}


DEF_OP(Not) {
  auto Op = IROp->C<IR::IROp_Not>();
  auto Dst = GetDst<RA_64>(Node);
  mov(Dst, GetSrc<RA_64>(Op->Src.ID()));
  not_(Dst);
}

DEF_OP(Popcount) {
  auto Op = IROp->C<IR::IROp_Popcount>();
  const uint8_t OpSize = IROp->Size;

  auto Dst64 = GetDst<RA_64>(Node);

  switch (OpSize) {
    case 1:
      movzx(GetDst<RA_32>(Node), GetSrc<RA_8>(Op->Src.ID()));
      popcnt(Dst64, Dst64);
      break;
    case 2: {
      movzx(GetDst<RA_32>(Node), GetSrc<RA_16>(Op->Src.ID()));
      popcnt(Dst64, Dst64);
      break;
    }
    case 4:
      popcnt(GetDst<RA_32>(Node), GetSrc<RA_32>(Op->Src.ID()));
      break;
    case 8:
      popcnt(GetDst<RA_64>(Node), GetSrc<RA_64>(Op->Src.ID()));
      break;
  }
}

DEF_OP(FindLSB) {
  auto Op = IROp->C<IR::IROp_FindLSB>();

  bsf(rcx, GetSrc<RA_64>(Op->Src.ID()));
  mov(rax, 0x40);
  cmovz(rcx, rax);
  xor_(rax, rax);
  cmp(GetSrc<RA_64>(Op->Src.ID()), 1);
  sbb(rax, rax);
  or_(rax, rcx);
  mov (GetDst<RA_64>(Node), rax);
}

DEF_OP(FindMSB) {
  auto Op = IROp->C<IR::IROp_FindMSB>();
  const uint8_t OpSize = IROp->Size;

  switch (OpSize) {
    case 2:
      bsr(GetDst<RA_16>(Node), GetSrc<RA_16>(Op->Src.ID()));
      movzx(GetDst<RA_32>(Node), GetDst<RA_16>(Node));
      break;
    case 4:
      bsr(GetDst<RA_32>(Node), GetSrc<RA_32>(Op->Src.ID()));
      break;
    case 8:
      bsr(GetDst<RA_64>(Node), GetSrc<RA_64>(Op->Src.ID()));
      break;
    default: LOGMAN_MSG_A_FMT("Unknown FindMSB OpSize: {}", OpSize);
  }
}

DEF_OP(FindTrailingZeroes) {
  auto Op = IROp->C<IR::IROp_FindTrailingZeroes>();
  const uint8_t OpSize = IROp->Size;

  switch (OpSize) {
    case 2:
      bsf(GetDst<RA_16>(Node), GetSrc<RA_16>(Op->Src.ID()));
      mov(ax, 0x10);
      cmovz(GetDst<RA_16>(Node), ax);
    break;
    case 4:
      bsf(GetDst<RA_32>(Node), GetSrc<RA_32>(Op->Src.ID()));
      mov(eax, 0x20);
      cmovz(GetDst<RA_32>(Node), eax);
      break;
    case 8:
      bsf(GetDst<RA_64>(Node), GetSrc<RA_64>(Op->Src.ID()));
      mov(rax, 0x40);
      cmovz(GetDst<RA_64>(Node), rax);
      break;
    default: LOGMAN_MSG_A_FMT("Unknown FindTrailingZeroes size: {}", OpSize); break;
  }
}

DEF_OP(CountLeadingZeroes) {
  auto Op = IROp->C<IR::IROp_CountLeadingZeroes>();
  const uint8_t OpSize = IROp->Size;

  if (Features.has(Xbyak::util::Cpu::tLZCNT)) {
    switch (OpSize) {
      case 2: {
        lzcnt(GetDst<RA_16>(Node), GetSrc<RA_16>(Op->Src.ID()));
        movzx(GetDst<RA_32>(Node), GetDst<RA_16>(Node));
        break;
      }
      case 4: {
        lzcnt(GetDst<RA_32>(Node), GetSrc<RA_32>(Op->Src.ID()));
        break;
      }
      case 8: {
        lzcnt(GetDst<RA_64>(Node), GetSrc<RA_64>(Op->Src.ID()));
        break;
      }
      default: LOGMAN_MSG_A_FMT("Unknown CountLeadingZeroes size: {}", OpSize); break;
    }
  }
  else {
    switch (OpSize) {
      case 2: {
        test(GetSrc<RA_16>(Op->Src.ID()), GetSrc<RA_16>(Op->Src.ID()));
        mov(eax, 0x10);
        Label Skip;
        je(Skip);
          bsr(ax, GetSrc<RA_16>(Op->Src.ID()));
          xor_(ax, 0xF);
          movzx(eax, ax);
        L(Skip);
        mov(GetDst<RA_32>(Node), eax);
        break;
      }
      case 4: {
        test(GetSrc<RA_32>(Op->Src.ID()), GetSrc<RA_32>(Op->Src.ID()));
        mov(eax, 0x20);
        Label Skip;
        je(Skip);
          bsr(eax, GetSrc<RA_32>(Op->Src.ID()));
          xor_(eax, 0x1F);
        L(Skip);
        mov(GetDst<RA_32>(Node), eax);
        break;
      }
      case 8: {
        test(GetSrc<RA_64>(Op->Src.ID()), GetSrc<RA_64>(Op->Src.ID()));
        mov(rax, 0x40);
        Label Skip;
        je(Skip);
          bsr(rax, GetSrc<RA_64>(Op->Src.ID()));
          xor_(rax, 0x3F);
        L(Skip);
        mov(GetDst<RA_64>(Node), rax);
        break;
      }
      default: LOGMAN_MSG_A_FMT("Unknown CountLeadingZeroes size: {}", OpSize); break;
    }
  }
}

DEF_OP(Rev) {
  auto Op = IROp->C<IR::IROp_Rev>();
  const uint8_t OpSize = IROp->Size;

  switch (OpSize) {
    case 2:
      mov (GetDst<RA_32>(Node), GetSrc<RA_32>(Op->Src.ID()));
      rol(GetDst<RA_16>(Node), 8);
    break;
    case 4:
      mov (GetDst<RA_32>(Node), GetSrc<RA_32>(Op->Src.ID()));
      bswap(GetDst<RA_32>(Node).cvt32());
      break;
    case 8:
      mov (GetDst<RA_64>(Node), GetSrc<RA_64>(Op->Src.ID()));
      bswap(GetDst<RA_64>(Node).cvt64());
      break;
    default: LOGMAN_MSG_A_FMT("Unknown REV size: {}", OpSize); break;
  }
}

DEF_OP(Bfi) {
  auto Op = IROp->C<IR::IROp_Bfi>();
  const uint8_t OpSize = IROp->Size;

  auto Dst = GetDst<RA_64>(Node);

  uint64_t SourceMask = (1ULL << Op->Width) - 1;
  if (Op->Width == 64) {
    SourceMask = ~0ULL;
  }
  const uint64_t DestMask = ~(SourceMask << Op->lsb);

  mov(TMP1, GetSrc<RA_64>(Op->Src.ID()));
  mov(Dst, GetSrc<RA_64>(Op->Dest.ID()));

  mov(TMP2, DestMask);
  and_(Dst, TMP2);
  mov(TMP2, SourceMask);
  and_(TMP1, TMP2);
  shl(TMP1, Op->lsb);
  or_(Dst, TMP1);

  if (OpSize != 8) {
    mov(rcx, uint64_t((1ULL << (OpSize * 8)) - 1));
    and_(Dst, rcx);
  }
}

DEF_OP(Bfxil) {
  auto Op = IROp->C<IR::IROp_Bfxil>();
  const uint8_t OpSize = IROp->Size;

  auto Dst = GetDst<RA_64>(Node);

  uint64_t SourceMask = (1ULL << Op->Width) - 1;
  if (Op->Width == 64) {
    SourceMask = ~0ULL;
  }
  const uint64_t DestMask = ~SourceMask;

  mov(TMP1, GetSrc<RA_64>(Op->Src.ID()));
  shr(TMP1, Op->lsb);
  mov(Dst, GetSrc<RA_64>(Op->Dest.ID()));

  mov(TMP2, DestMask);
  and_(Dst, TMP2);
  mov(TMP2, SourceMask);
  and_(TMP1, TMP2);
  or_(Dst, TMP1);

  if (OpSize != 8) {
    mov(rcx, uint64_t((1ULL << (OpSize * 8)) - 1));
    and_(Dst, rcx);
  }
}

DEF_OP(Bfe) {
  auto Op = IROp->C<IR::IROp_Bfe>();
  LOGMAN_THROW_AA_FMT(IROp->Size <= 8, "OpSize is too large for BFE: {}", IROp->Size);

  auto Dst = GetDst<RA_64>(Node);

  // Special cases for fast extends
  if (Op->lsb == 0) {
    switch (Op->Width / 8) {
    case 1:
      movzx(Dst, GetSrc<RA_8>(Op->Src.ID()));
      return;
    case 2:
      movzx(Dst, GetSrc<RA_16>(Op->Src.ID()));
      return;
    case 4:
      mov(Dst.cvt32(), GetSrc<RA_32>(Op->Src.ID()));
      return;
    case 8:
      mov(Dst, GetSrc<RA_64>(Op->Src.ID()));
      return;
    default:
      // Need to use slower general case
      break;
    }
  }

  mov(Dst, GetSrc<RA_64>(Op->Src.ID()));

  if (Op->lsb != 0)
    shr(Dst, Op->lsb);

  if (Op->Width != 64) {
    mov(rcx, uint64_t((1ULL << Op->Width) - 1));
    and_(Dst, rcx);
  }
}

DEF_OP(Sbfe) {
  auto Op = IROp->C<IR::IROp_Sbfe>();
  auto Dst = GetDst<RA_64>(Node);

  // Special cases for fast signed extends
  if (Op->lsb == 0) {
    switch (Op->Width / 8) {
    case 1:
      movsx(Dst, GetSrc<RA_8>(Op->Src.ID()));
      return;
    case 2:
      movsx(Dst, GetSrc<RA_16>(Op->Src.ID()));
      return;
    case 4:
      movsxd(Dst.cvt64(), GetSrc<RA_32>(Op->Src.ID()));
      return;
    case 8:
      mov(Dst, GetSrc<RA_64>(Op->Src.ID()));
      return;
    default:
      // Need to use slower general case
      break;
    }
  }

  // Slightly slower general case
  {
    uint64_t ShiftLeftAmount = (64 - (Op->Width + Op->lsb));
    uint64_t ShiftRightAmount = ShiftLeftAmount + Op->lsb;
    mov(Dst, GetSrc<RA_64>(Op->Src.ID()));
    shl(Dst, ShiftLeftAmount);
    sar(Dst, ShiftRightAmount);
  }
}

DEF_OP(Select) {
  auto Op = IROp->C<IR::IROp_Select>();
  auto Dst = GRD(Node);

  if (IsGPR(Op->Cmp1.ID())) {
    uint64_t Const;
    if (IsInlineConstant(Op->Cmp2, &Const)) {
      cmp(GRCMP(Op->Cmp1.ID()), Const);
    } else {
      cmp(GRCMP(Op->Cmp1.ID()), GRCMP(Op->Cmp2.ID()));
    }
  }
  else if (IsGPRPair(Op->Cmp1.ID())) {
    if (Op->CompareSize == 4) {
      const auto Src1 = GetSrcPair<RA_32>(Op->Cmp1.ID());
      const auto Src2 = GetSrcPair<RA_32>(Op->Cmp2.ID());
      mov (TMP1.cvt32(), Src1.first);
      mov (TMP2.cvt32(), Src1.second);
      xor_(TMP1.cvt32(), Src2.first);
      xor_(TMP2.cvt32(), Src2.second);
      or_(TMP1.cvt32(), TMP2.cvt32());
    }
    else {
      const auto Src1 = GetSrcPair<RA_64>(Op->Cmp1.ID());
      const auto Src2 = GetSrcPair<RA_64>(Op->Cmp2.ID());
      mov (TMP1, Src1.first);
      mov (TMP2, Src1.second);
      xor_(TMP1, Src2.first);
      xor_(TMP2, Src2.second);
      or_(TMP1, TMP2);
    }
  }
  else if (IsFPR(Op->Cmp1.ID())) {
    if (Op->CompareSize  == 4)
      ucomiss(GetSrc(Op->Cmp1.ID()), GetSrc(Op->Cmp2.ID()));
    else
      ucomisd(GetSrc(Op->Cmp1.ID()), GetSrc(Op->Cmp2.ID()));
  }

  uint64_t const_true, const_false;
  bool is_const_true = IsInlineConstant(Op->TrueVal, &const_true);
  bool is_const_false = IsInlineConstant(Op->FalseVal, &const_false);

  auto [SetCC, CMovCC, _] = GetCC(Op->Cond);

  if (is_const_true || is_const_false) {
    if (is_const_false != true || is_const_true != true || const_true != 1 || const_false != 0) {
      LOGMAN_MSG_A_FMT("Select: Unsupported compare inline parameters");
    }
    (this->*SetCC)(al);
    movzx(Dst, al);
  } else {
    mov(rax, GetSrc<RA_64>(Op->FalseVal.ID()));
    (this->*CMovCC)(rax, GetSrc<RA_64>(Op->TrueVal.ID()));
    mov (Dst, rax.changeBit(IROp->Size * 8));
  }
}

DEF_OP(VExtractToGPR) {
  const auto Op = IROp->C<IR::IROp_VExtractToGPR>();

  constexpr auto SSERegSize = Core::CPUState::XMM_SSE_REG_SIZE;
  constexpr auto SSEBitSize = SSERegSize * 8;

  const auto ElementSize = Op->Header.ElementSize;
  const auto ElementSizeBits = ElementSize * 8;
  const auto Offset = ElementSizeBits * Op->Index;

  const auto Is256Bit = Offset >= SSEBitSize;

  const auto Vector = GetSrc(Op->Vector.ID());

  switch (ElementSize) {
    case 1: {
      if (Is256Bit) {
        vextracti128(xmm15, ToYMM(Vector), 1);
        pextrb(GetDst<RA_32>(Node), xmm15, Op->Index - 16);
      } else {
        pextrb(GetDst<RA_32>(Node), Vector, Op->Index);
      }
      break;
    }
    case 2: {
      if (Is256Bit) {
        vextracti128(xmm15, ToYMM(Vector), 1);
        pextrw(GetDst<RA_32>(Node), xmm15, Op->Index - 8);
      } else {
        pextrw(GetDst<RA_32>(Node), Vector, Op->Index);
      }
      break;
    }
    case 4: {
      if (Is256Bit) {
        vextracti128(xmm15, ToYMM(Vector), 1);
        pextrd(GetDst<RA_32>(Node), xmm15, Op->Index - 4);
      } else {
        pextrd(GetDst<RA_32>(Node), Vector, Op->Index);
      }
      break;
    }
    case 8: {
      if (Is256Bit) {
        vextracti128(xmm15, ToYMM(Vector), 1);
        pextrq(GetDst<RA_64>(Node), xmm15, Op->Index - 2);
      } else {
        pextrq(GetDst<RA_64>(Node), Vector, Op->Index);
      }
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
}

DEF_OP(Float_ToGPR_ZS) {
  auto Op = IROp->C<IR::IROp_Float_ToGPR_ZS>();
  const uint16_t Conv = (IROp->Size << 8) | Op->SrcElementSize;

  switch (Conv) {
    case 0x0804: // int64_t <- float
      cvttss2si(GetDst<RA_64>(Node), GetSrc(Op->Scalar.ID()));
    break;
    case 0x0808: // int64_t <- double
      cvttsd2si(GetDst<RA_64>(Node), GetSrc(Op->Scalar.ID()));
    break;
    case 0x0404: // int32_t <- float
      cvttss2si(GetDst<RA_32>(Node), GetSrc(Op->Scalar.ID()));
    break;
    case 0x0408: // int32_t <- double
      cvttsd2si(GetDst<RA_32>(Node), GetSrc(Op->Scalar.ID()));
    break;
  }
}

DEF_OP(Float_ToGPR_S) {
  auto Op = IROp->C<IR::IROp_Float_ToGPR_S>();
  const uint16_t Conv = (IROp->Size << 8) | Op->SrcElementSize;

  switch (Conv) {
    case 0x0804: // int64_t <- float
      cvtss2si(GetDst<RA_64>(Node), GetSrc(Op->Scalar.ID()));
    break;
    case 0x0808: // int64_t <- double
      cvtsd2si(GetDst<RA_64>(Node), GetSrc(Op->Scalar.ID()));
    break;
    case 0x0404: // int32_t <- float
      cvtss2si(GetDst<RA_32>(Node), GetSrc(Op->Scalar.ID()));
    break;
    case 0x0408: // int32_t <- double
      cvtsd2si(GetDst<RA_32>(Node), GetSrc(Op->Scalar.ID()));
    break;
  }
}

DEF_OP(FCmp) {
  auto Op = IROp->C<IR::IROp_FCmp>();

  if (Op->Flags & (1 << IR::FCMP_FLAG_UNORDERED)) {
    if (Op->ElementSize == 4) {
      ucomiss(GetSrc(Op->Scalar1.ID()), GetSrc(Op->Scalar2.ID()));
    }
    else {
      ucomisd(GetSrc(Op->Scalar1.ID()), GetSrc(Op->Scalar2.ID()));
    }
  }
  else {
    if (Op->ElementSize == 4) {
      comiss(GetSrc(Op->Scalar1.ID()), GetSrc(Op->Scalar2.ID()));
    }
    else {
      comisd(GetSrc(Op->Scalar1.ID()), GetSrc(Op->Scalar2.ID()));
    }
  }
  mov (rdx, 0);

  lahf();
  if (Op->Flags & (1 << IR::FCMP_FLAG_LT)) {
    sahf();
    mov(rcx, 0);
    setb(cl);
    shl(rcx, IR::FCMP_FLAG_LT);
    or_(rdx, rcx);
  }
  if (Op->Flags & (1 << IR::FCMP_FLAG_UNORDERED)) {
    sahf();
    mov(rcx, 0);
    setp(cl);
    shl(rcx, IR::FCMP_FLAG_UNORDERED);
    or_(rdx, rcx);
  }
  if (Op->Flags & (1 << IR::FCMP_FLAG_EQ)) {
    sahf();
    mov(rcx, 0);
    setz(cl);
    shl(rcx, IR::FCMP_FLAG_EQ);
    or_(rdx, rcx);
  }
  mov (GetDst<RA_64>(Node), rdx);
}

#undef DEF_OP

void X86JITCore::RegisterALUHandlers() {
#define REGISTER_OP(op, x) OpHandlers[FEXCore::IR::IROps::OP_##op] = &X86JITCore::Op_##x
  REGISTER_OP(TRUNCELEMENTPAIR,  TruncElementPair);
  REGISTER_OP(CONSTANT,          Constant);
  REGISTER_OP(ENTRYPOINTOFFSET,  EntrypointOffset);
  REGISTER_OP(INLINECONSTANT,    InlineConstant);
  REGISTER_OP(INLINEENTRYPOINTOFFSET,  InlineEntrypointOffset);
  REGISTER_OP(CYCLECOUNTER,      CycleCounter);
  REGISTER_OP(ADD,               Add);
  REGISTER_OP(ADDNZCV,           AddNZCV);
  REGISTER_OP(TESTNZ,            TestNZ);
  REGISTER_OP(SUB,               Sub);
  REGISTER_OP(SUBNZCV,           SubNZCV);
  REGISTER_OP(NEG,               Neg);
  REGISTER_OP(ABS,               Abs);
  REGISTER_OP(MUL,               Mul);
  REGISTER_OP(UMUL,              UMul);
  REGISTER_OP(DIV,               Div);
  REGISTER_OP(UDIV,              UDiv);
  REGISTER_OP(REM,               Rem);
  REGISTER_OP(UREM,              URem);
  REGISTER_OP(MULH,              MulH);
  REGISTER_OP(UMULH,             UMulH);
  REGISTER_OP(OR,                Or);
  REGISTER_OP(ORLSHL,            Orlshl);
  REGISTER_OP(ORLSHR,            Orlshr);
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
  REGISTER_OP(FINDTRAILINGZEROES, FindTrailingZeroes);
  REGISTER_OP(COUNTLEADINGZEROES, CountLeadingZeroes);
  REGISTER_OP(REV,               Rev);
  REGISTER_OP(BFI,               Bfi);
  REGISTER_OP(BFXIL,             Bfxil);
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
