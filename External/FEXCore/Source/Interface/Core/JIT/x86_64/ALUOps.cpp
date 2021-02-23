#include "Interface/Core/JIT/x86_64/JITClass.h"
#include "Interface/IR/Passes/RegisterAllocationPass.h"

namespace FEXCore::CPU {
#define DEF_OP(x) void JITCore::Op_##x(FEXCore::IR::IROp_Header *IROp, uint32_t Node)
DEF_OP(TruncElementPair) {
  auto Op = IROp->C<IR::IROp_TruncElementPair>();

  switch (Op->Size) {
    case 4: {
      auto Dst = GetSrcPair<RA_32>(Node);
      auto Src = GetSrcPair<RA_32>(Op->Header.Args[0].ID());
      mov(Dst.first, Src.first);
      mov(Dst.second, Src.second);
      break;
    }
    default: LogMan::Msg::A("Unhandled Truncation size: %d", Op->Size); break;
  }
}

DEF_OP(Constant) {
  auto Op = IROp->C<IR::IROp_Constant>();
  mov(GetDst<RA_64>(Node), Op->Constant);
}

DEF_OP(EntrypointOffset) {
  auto Op = IROp->C<IR::IROp_EntrypointOffset>();

  auto Constant = IR->GetHeader()->Entry + Op->Offset;
  mov(GetDst<RA_64>(Node), Constant);
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
  or(rax, rdx);
  mov (GetDst<RA_64>(Node), rax);
#endif
}

DEF_OP(Add) {
  auto Op = IROp->C<IR::IROp_Add>();
  uint8_t OpSize = IROp->Size;

  mov(rax, GetSrc<RA_64>(Op->Header.Args[0].ID()));

  uint64_t Const;
  if (IsInlineConstant(Op->Header.Args[1], &Const)) {
    switch (OpSize) {
    case 4:
      add(eax, Const);
      break;
    case 8:
      add(rax, Const);
      break;
    default:  LogMan::Msg::A("Unhandled Add size: %d", OpSize);
      break;
    }
  } else {
    switch (OpSize) {
    case 4:
      add(eax, GetSrc<RA_32>(Op->Header.Args[1].ID()));
      break;
    case 8:
      add(rax, GetSrc<RA_64>(Op->Header.Args[1].ID()));
      break;
    default:  LogMan::Msg::A("Unhandled Add size: %d", OpSize);
      break;
    }
  }
  mov(GetDst<RA_64>(Node), rax);
}

DEF_OP(Sub) {
  auto Op = IROp->C<IR::IROp_Sub>();
  uint8_t OpSize = IROp->Size;

  mov(rax, GetSrc<RA_64>(Op->Header.Args[0].ID()));

  uint64_t Const;
  if (IsInlineConstant(Op->Header.Args[1], &Const)) {
    switch (OpSize) {
    case 4:
      sub(eax, Const);
      break;
    case 8:
      sub(rax, Const);
      break;
    default:  LogMan::Msg::A("Unhandled Sub size: %d", OpSize);
      break;
    }
  } else {
    switch (OpSize) {
    case 4:
      sub(eax, GetSrc<RA_32>(Op->Header.Args[1].ID()));
      break;
    case 8:
      sub(rax, GetSrc<RA_64>(Op->Header.Args[1].ID()));
      break;
    default:  LogMan::Msg::A("Unhandled Sub size: %d", OpSize);
      break;
    }
  }
  mov(GetDst<RA_64>(Node), rax);
}

DEF_OP(Neg) {
  auto Op = IROp->C<IR::IROp_Neg>();
  uint8_t OpSize = IROp->Size;

  Xbyak::Reg Src;
  Xbyak::Reg Dst;
  switch (OpSize) {
  case 4:
    Src = GetSrc<RA_32>(Op->Header.Args[0].ID());
    Dst = GetDst<RA_32>(Node);
    break;
  case 8:
    Src = GetSrc<RA_64>(Op->Header.Args[0].ID());
    Dst = GetDst<RA_64>(Node);
    break;
  default:  LogMan::Msg::A("Unhandled Neg size: %d", OpSize);
    break;
  }
  mov(Dst, Src);
  neg(Dst);
}

DEF_OP(Mul) {
  auto Op = IROp->C<IR::IROp_Mul>();
  uint8_t OpSize = IROp->Size;

  auto Dst = GetDst<RA_64>(Node);

  switch (OpSize) {
  case 4:
    movsxd(rax, GetSrc<RA_32>(Op->Header.Args[0].ID()));
    imul(eax, GetSrc<RA_32>(Op->Header.Args[1].ID()));
    mov(Dst.cvt32(), eax);
  break;
  case 8:
    mov(rax, GetSrc<RA_64>(Op->Header.Args[0].ID()));
    imul(rax, GetSrc<RA_64>(Op->Header.Args[1].ID()));
    mov(Dst, rax);
  break;
  default: LogMan::Msg::A("Unknown Sext size: %d", OpSize);
  }
}

DEF_OP(UMul) {
  auto Op = IROp->C<IR::IROp_UMul>();
  uint8_t OpSize = IROp->Size;

  switch (OpSize) {
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
}

DEF_OP(Div) {
  auto Op = IROp->C<IR::IROp_Div>();
  uint8_t OpSize = IROp->Size;

  // Each source is OpSize in size
  // So you can have up to a 128bit divide from x86-64
  auto Size = OpSize;
  switch (Size) {
  case 1: {
    movsx(ax, GetSrc<RA_8>(Op->Header.Args[0].ID()));
    idiv(GetSrc<RA_8>(Op->Header.Args[1].ID()));
    movsx(GetDst<RA_32>(Node), al);
  break;
  }
  case 2: {
    mov (ax, GetSrc<RA_16>(Op->Header.Args[0].ID()));
    cwd();
    idiv(GetSrc<RA_16>(Op->Header.Args[1].ID()));
    movsx(GetDst<RA_32>(Node), ax);
  break;
  }
  case 4: {
    mov (eax, GetSrc<RA_32>(Op->Header.Args[0].ID()));
    cdq();
    idiv(GetSrc<RA_32>(Op->Header.Args[1].ID()));
    movsxd(GetDst<RA_64>(Node).cvt64(), eax);
  break;
  }
  case 8: {
    mov (rax, GetSrc<RA_64>(Op->Header.Args[0].ID()));
    cqo();
    idiv(GetSrc<RA_64>(Op->Header.Args[1].ID()));
    mov(GetDst<RA_64>(Node), rax);
  break;
  }
  default: LogMan::Msg::A("Unknown UDIV Size: %d", Size); break;
  }
}

DEF_OP(UDiv) {
  auto Op = IROp->C<IR::IROp_UDiv>();
  uint8_t OpSize = IROp->Size;

  // Each source is OpSize in size
  // So you can have up to a 128bit divide from x86-64
  switch (OpSize) {
  case 1: {
    mov (al, GetSrc<RA_8>(Op->Header.Args[0].ID()));
    mov (edx, 0);
    mov (cl, GetSrc<RA_8>(Op->Header.Args[1].ID()));
    div(cl);
    movzx(GetDst<RA_32>(Node), al);
  break;
  }
  case 2: {
    mov (ax, GetSrc<RA_16>(Op->Header.Args[0].ID()));
    mov (edx, 0);
    mov (cx, GetSrc<RA_16>(Op->Header.Args[1].ID()));
    div(cx);
    movzx(GetDst<RA_32>(Node), ax);
  break;
  }
  case 4: {
    mov (eax, GetSrc<RA_32>(Op->Header.Args[0].ID()));
    mov (edx, 0);
    mov (ecx, GetSrc<RA_32>(Op->Header.Args[1].ID()));
    div(ecx);
    mov(GetDst<RA_32>(Node), eax);
  break;
  }
  case 8: {
    mov (rax, GetSrc<RA_64>(Op->Header.Args[0].ID()));
    mov (rdx, 0);
    mov (rcx, GetSrc<RA_64>(Op->Header.Args[1].ID()));
    div(rcx);
    mov(GetDst<RA_64>(Node), rax);
  break;
  }
  default: LogMan::Msg::A("Unknown UDIV OpSize: %d", OpSize); break;
  }
}

DEF_OP(Rem) {
  auto Op = IROp->C<IR::IROp_Rem>();
  uint8_t OpSize = IROp->Size;

  switch (OpSize) {
  case 1: {
    movsx(ax, GetSrc<RA_8>(Op->Header.Args[0].ID()));
    idiv(GetSrc<RA_8>(Op->Header.Args[1].ID()));
    mov(al, ah);
    movsx(GetDst<RA_32>(Node), al);
  break;
  }
  case 2: {
    mov (ax, GetSrc<RA_16>(Op->Header.Args[0].ID()));
    cwd();
    idiv(GetSrc<RA_16>(Op->Header.Args[1].ID()));
    movsx(GetDst<RA_32>(Node), dx);
  break;
  }
  case 4: {
    mov (eax, GetSrc<RA_32>(Op->Header.Args[0].ID()));
    cdq();
    idiv(GetSrc<RA_32>(Op->Header.Args[1].ID()));
    movsxd(GetDst<RA_64>(Node).cvt64(), edx);
  break;
  }
  case 8: {
    mov (rax, GetSrc<RA_64>(Op->Header.Args[0].ID()));
    cqo();
    idiv(GetSrc<RA_64>(Op->Header.Args[1].ID()));
    mov(GetDst<RA_64>(Node), rdx);
  break;
  }
  default: LogMan::Msg::A("Unknown UDIV Size: %d", OpSize); break;
  }
}

DEF_OP(URem) {
  auto Op = IROp->C<IR::IROp_URem>();
  uint8_t OpSize = IROp->Size;

  // Each source is OpSize in size
  // So you can have up to a 128bit divide from x86-64
  switch (OpSize) {
  case 1: {
    mov (al, GetSrc<RA_8>(Op->Header.Args[0].ID()));
    mov (edx, 0);
    mov (cl, GetSrc<RA_8>(Op->Header.Args[1].ID()));
    div(cl);
    movzx(GetDst<RA_32>(Node), ah);
  break;
  }
  case 2: {
    mov (ax, GetSrc<RA_16>(Op->Header.Args[0].ID()));
    mov (edx, 0);
    mov (cx, GetSrc<RA_16>(Op->Header.Args[1].ID()));
    div(cx);
    movzx(GetDst<RA_32>(Node), dx);
  break;
  }
  case 4: {
    mov (eax, GetSrc<RA_32>(Op->Header.Args[0].ID()));
    mov (edx, 0);
    mov (ecx, GetSrc<RA_32>(Op->Header.Args[1].ID()));
    div(ecx);
    mov(GetDst<RA_32>(Node), edx);
  break;
  }
  case 8: {
    mov (rax, GetSrc<RA_64>(Op->Header.Args[0].ID()));
    mov (rdx, 0);
    mov (rcx, GetSrc<RA_64>(Op->Header.Args[1].ID()));
    div(rcx);
    mov(GetDst<RA_64>(Node), rdx);
  break;
  }
  default: LogMan::Msg::A("Unknown UDIV OpSize: %d", OpSize); break;
  }
}

DEF_OP(MulH) {
  auto Op = IROp->C<IR::IROp_MulH>();
  uint8_t OpSize = IROp->Size;

  switch (OpSize) {
  case 4:
    movsxd(rax, GetSrc<RA_32>(Op->Header.Args[0].ID()));
    imul(GetSrc<RA_32>(Op->Header.Args[1].ID()));
    movsxd(GetDst<RA_64>(Node).cvt64(), edx);
  break;
  case 8:
    mov(rax, GetSrc<RA_64>(Op->Header.Args[0].ID()));
    imul(GetSrc<RA_64>(Op->Header.Args[1].ID()));
    mov(GetDst<RA_64>(Node), rdx);
  break;
  default: LogMan::Msg::A("Unknown Sext size: %d", OpSize);
  }
}

DEF_OP(UMulH) {
  auto Op = IROp->C<IR::IROp_UMulH>();
  uint8_t OpSize = IROp->Size;

  switch (OpSize) {
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
}

DEF_OP(Or) {
  auto Op = IROp->C<IR::IROp_Or>();
  auto Dst = GetDst<RA_64>(Node);
  mov(rax, GetSrc<RA_64>(Op->Header.Args[0].ID()));

  uint64_t Const;
  if (IsInlineConstant(Op->Header.Args[1], &Const)) {
    or (rax, Const);
  } else {
    or (rax, GetSrc<RA_64>(Op->Header.Args[1].ID()));
  }
  mov(Dst, rax);
}

DEF_OP(And) {
  auto Op = IROp->C<IR::IROp_And>();
  auto Dst = GetDst<RA_64>(Node);
  mov(rax, GetSrc<RA_64>(Op->Header.Args[0].ID()));
  uint64_t Const;
  if (IsInlineConstant(Op->Header.Args[1], &Const)) {
    and (rax, Const);
  } else {
    and(rax, GetSrc<RA_64>(Op->Header.Args[1].ID()));
  }
  mov(Dst, rax);
}

DEF_OP(Xor) {
  auto Op = IROp->C<IR::IROp_Xor>();
  auto Dst = GetDst<RA_64>(Node);
  mov(rax, GetSrc<RA_64>(Op->Header.Args[0].ID()));
  uint64_t Const;
  if (IsInlineConstant(Op->Header.Args[1], &Const)) {
    xor(rax, Const);
  } else {
    xor(rax, GetSrc<RA_64>(Op->Header.Args[1].ID()));
  }
  mov(Dst, rax);
}

DEF_OP(Lshl) {
  auto Op = IROp->C<IR::IROp_Lshl>();
  uint8_t OpSize = IROp->Size;

  uint8_t Mask = OpSize * 8 - 1;

  uint64_t Const;
  if (IsInlineConstant(Op->Header.Args[1], &Const)) {
    Const &= Mask;
    switch (OpSize) {
      case 4:
        mov(GetDst<RA_32>(Node), GetSrc<RA_32>(Op->Header.Args[0].ID()));
        shl(GetDst<RA_32>(Node), Const);
        break;
      case 8:
        mov(GetDst<RA_64>(Node), GetSrc<RA_64>(Op->Header.Args[0].ID()));
        shl(GetDst<RA_64>(Node), Const);
        break;
      default: LogMan::Msg::A("Unknown LSHL Size: %d\n", OpSize); break;
    };
  } else {
    mov(rcx, GetSrc<RA_64>(Op->Header.Args[1].ID()));
    and(rcx, Mask);

    switch (OpSize) {
      case 4:
        mov(GetDst<RA_32>(Node), GetSrc<RA_32>(Op->Header.Args[0].ID()));
        shl(GetDst<RA_32>(Node), cl);
        break;
      case 8:
        mov(GetDst<RA_64>(Node), GetSrc<RA_64>(Op->Header.Args[0].ID()));
        shl(GetDst<RA_64>(Node), cl);
        break;
      default: LogMan::Msg::A("Unknown LSHL Size: %d\n", OpSize); break;
    };
  }
}

DEF_OP(Lshr) {
  auto Op = IROp->C<IR::IROp_Lshr>();
  uint8_t OpSize = IROp->Size;

  uint8_t Mask = OpSize * 8 - 1;

  uint64_t Const;
  if (IsInlineConstant(Op->Header.Args[1], &Const)) {
    Const &= Mask;

    switch (OpSize) {
      case 1:
        movzx(GetDst<RA_32>(Node), GetSrc<RA_8>(Op->Header.Args[0].ID()));
        shr(GetDst<RA_32>(Node).cvt8(), Const);
        break;
      case 2:
        movzx(GetDst<RA_32>(Node), GetSrc<RA_16>(Op->Header.Args[0].ID()));
        shr(GetDst<RA_32>(Node).cvt16(), Const);
        break;
      case 4:
        mov(GetDst<RA_32>(Node), GetSrc<RA_32>(Op->Header.Args[0].ID()));
        shr(GetDst<RA_32>(Node), Const);
        break;
      case 8:
        mov(GetDst<RA_64>(Node), GetSrc<RA_64>(Op->Header.Args[0].ID()));
        shr(GetDst<RA_64>(Node), Const);
        break;
      default: LogMan::Msg::A("Unknown Size: %d\n", OpSize); break;
    };

  } else {
    mov (rcx, GetSrc<RA_64>(Op->Header.Args[1].ID()));
    and(rcx, Mask);

    switch (OpSize) {
      case 1:
        movzx(GetDst<RA_32>(Node), GetSrc<RA_8>(Op->Header.Args[0].ID()));
        shr(GetDst<RA_32>(Node).cvt8(), cl);
        break;
      case 2:
        movzx(GetDst<RA_32>(Node), GetSrc<RA_16>(Op->Header.Args[0].ID()));
        shr(GetDst<RA_32>(Node).cvt16(), cl);
        break;
      case 4:
        mov(GetDst<RA_32>(Node), GetSrc<RA_32>(Op->Header.Args[0].ID()));
        shr(GetDst<RA_32>(Node), cl);
        break;
      case 8:
        mov(GetDst<RA_64>(Node), GetSrc<RA_64>(Op->Header.Args[0].ID()));
        shr(GetDst<RA_64>(Node), cl);
        break;
      default: LogMan::Msg::A("Unknown Size: %d\n", OpSize); break;
    };
  }
}

DEF_OP(Ashr) {
  auto Op = IROp->C<IR::IROp_Ashr>();
  uint8_t OpSize = IROp->Size;

  uint8_t Mask = OpSize * 8 - 1;

  uint64_t Const;
  if (IsInlineConstant(Op->Header.Args[1], &Const)) {
    Const &= Mask;

    switch (OpSize) {
    case 1:
      movsx(rax, GetSrc<RA_8>(Op->Header.Args[0].ID()));
      sar(al, Const);
      movzx(GetDst<RA_64>(Node), al);
    break;
    case 2:
      movsx(rax, GetSrc<RA_16>(Op->Header.Args[0].ID()));
      sar(ax, Const);
      movzx(GetDst<RA_64>(Node), ax);
    break;
    case 4:
      mov(GetDst<RA_32>(Node), GetSrc<RA_32>(Op->Header.Args[0].ID()));
      sar(GetDst<RA_32>(Node), Const);
    break;
    case 8:
      mov(GetDst<RA_64>(Node), GetSrc<RA_64>(Op->Header.Args[0].ID()));
      sar(GetDst<RA_64>(Node), Const);
    break;
    default: LogMan::Msg::A("Unknown ASHR Size: %d\n", OpSize); break;
    };

  } else {
    mov (rcx, GetSrc<RA_64>(Op->Header.Args[1].ID()));
    and(rcx, Mask);
    switch (OpSize) {
    case 1:
      movsx(rax, GetSrc<RA_8>(Op->Header.Args[0].ID()));
      sar(al, cl);
      movzx(GetDst<RA_64>(Node), al);
    break;
    case 2:
      movsx(rax, GetSrc<RA_16>(Op->Header.Args[0].ID()));
      sar(ax, cl);
      movzx(GetDst<RA_64>(Node), ax);
    break;
    case 4:
      mov(GetDst<RA_32>(Node), GetSrc<RA_32>(Op->Header.Args[0].ID()));
      sar(GetDst<RA_32>(Node), cl);
    break;
    case 8:
      mov(GetDst<RA_64>(Node), GetSrc<RA_64>(Op->Header.Args[0].ID()));
      sar(GetDst<RA_64>(Node), cl);
    break;
    default: LogMan::Msg::A("Unknown ASHR Size: %d\n", OpSize); break;
    };
  }
}

DEF_OP(Ror) {
  auto Op = IROp->C<IR::IROp_Ror>();
  uint8_t OpSize = IROp->Size;

  uint8_t Mask = OpSize * 8 - 1;

  uint64_t Const;
  if (IsInlineConstant(Op->Header.Args[1], &Const)) {
    Const &= Mask;
    switch (OpSize) {
      case 4: {
        mov(eax, GetSrc<RA_32>(Op->Header.Args[0].ID()));
        ror(eax, Const);
      break;
      }
      case 8: {
        mov(rax, GetSrc<RA_64>(Op->Header.Args[0].ID()));
        ror(rax, Const);
      break;
      }
      default: LogMan::Msg::A("Unknown ROR Size: %d\n", OpSize); break;
    }
  } else {
    mov (rcx, GetSrc<RA_64>(Op->Header.Args[1].ID()));
    and(rcx, Mask);
    switch (OpSize) {
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
      default: LogMan::Msg::A("Unknown ROR Size: %d\n", OpSize); break;
    }
  }
  mov(GetDst<RA_64>(Node), rax);
}

DEF_OP(Extr) {
  auto Op = IROp->C<IR::IROp_Extr>();
  uint8_t OpSize = IROp->Size;

  switch (OpSize) {
    case 4: {
      mov(eax, GetSrc<RA_32>(Op->Header.Args[0].ID()));
      mov(ecx, GetSrc<RA_32>(Op->Header.Args[1].ID()));
      shrd(ecx, eax, Op->LSB);
      mov(GetDst<RA_32>(Node), ecx);
      break;
    }
    case 8: {
      mov(rax, GetSrc<RA_64>(Op->Header.Args[0].ID()));
      mov(rcx, GetSrc<RA_64>(Op->Header.Args[1].ID()));
      shrd(rcx, rax, Op->LSB);
      mov(GetDst<RA_64>(Node), rcx);
      break;
    }
  }
}

DEF_OP(LDiv) {
  auto Op = IROp->C<IR::IROp_LDiv>();
  uint8_t OpSize = IROp->Size;

  // Each source is OpSize in size
  // So you can have up to a 128bit divide from x86-64
  switch (OpSize) {
    case 2: {
      mov(eax, GetSrc<RA_16>(Op->Header.Args[0].ID()));
      mov(edx, GetSrc<RA_16>(Op->Header.Args[1].ID()));
      idiv(GetSrc<RA_16>(Op->Header.Args[2].ID()));
      movsx(GetDst<RA_64>(Node), ax);
      break;
    }
    case 4: {
      mov(eax, GetSrc<RA_32>(Op->Header.Args[0].ID()));
      mov(edx, GetSrc<RA_32>(Op->Header.Args[1].ID()));
      idiv(GetSrc<RA_32>(Op->Header.Args[2].ID()));
      movsxd(GetDst<RA_64>(Node).cvt64(), eax);
      break;
    }
    case 8: {
      mov(rax, GetSrc<RA_64>(Op->Header.Args[0].ID()));
      mov(rdx, GetSrc<RA_64>(Op->Header.Args[1].ID()));
      idiv(GetSrc<RA_64>(Op->Header.Args[2].ID()));
      mov(GetDst<RA_64>(Node), rax);
      break;
    }
    default: LogMan::Msg::A("Unknown LDIV OpSize: %d", OpSize); break;
  }
}

DEF_OP(LUDiv) {
  auto Op = IROp->C<IR::IROp_LUDiv>();
  uint8_t OpSize = IROp->Size;

  // Each source is OpSize in size
  // So you can have up to a 128bit divide from x86-64
  switch (OpSize) {
    case 2: {
      mov (ax, GetSrc<RA_16>(Op->Header.Args[0].ID()));
      mov (dx, GetSrc<RA_16>(Op->Header.Args[1].ID()));
      div(GetSrc<RA_16>(Op->Header.Args[2].ID()));
      movzx(GetDst<RA_32>(Node), ax);
      break;
    }
    case 4: {
      mov (eax, GetSrc<RA_32>(Op->Header.Args[0].ID()));
      mov (edx, GetSrc<RA_32>(Op->Header.Args[1].ID()));
      div(GetSrc<RA_32>(Op->Header.Args[2].ID()));
      mov(GetDst<RA_64>(Node), rax);
      break;
    }
    case 8: {
      mov (rax, GetSrc<RA_64>(Op->Header.Args[0].ID()));
      mov (rdx, GetSrc<RA_64>(Op->Header.Args[1].ID()));
      div(GetSrc<RA_64>(Op->Header.Args[2].ID()));
      mov(GetDst<RA_64>(Node), rax);
      break;
    }
    default: LogMan::Msg::A("Unknown LUDIV OpSize: %d", OpSize); break;
  }
}

DEF_OP(LRem) {
  auto Op = IROp->C<IR::IROp_LRem>();
  uint8_t OpSize = IROp->Size;

  // Each source is OpSize in size
  // So you can have up to a 128bit divide from x86-64
  switch (OpSize) {
    case 2: {
      mov(ax, GetSrc<RA_16>(Op->Header.Args[0].ID()));
      mov(dx, GetSrc<RA_16>(Op->Header.Args[1].ID()));
      idiv(GetSrc<RA_16>(Op->Header.Args[2].ID()));
      movsx(GetDst<RA_64>(Node), dx);
      break;
    }
    case 4: {
      mov(eax, GetSrc<RA_32>(Op->Header.Args[0].ID()));
      mov(edx, GetSrc<RA_32>(Op->Header.Args[1].ID()));
      idiv(GetSrc<RA_32>(Op->Header.Args[2].ID()));
      mov(GetDst<RA_64>(Node), rdx);
      break;
    }
    case 8: {
      mov(rax, GetSrc<RA_64>(Op->Header.Args[0].ID()));
      mov(rdx, GetSrc<RA_64>(Op->Header.Args[1].ID()));
      idiv(GetSrc<RA_64>(Op->Header.Args[2].ID()));
      mov(GetDst<RA_64>(Node), rdx);
      break;
    }
    default: LogMan::Msg::A("Unknown LREM OpSize: %d", OpSize); break;
  }
}

DEF_OP(LURem) {
  auto Op = IROp->C<IR::IROp_LURem>();
  uint8_t OpSize = IROp->Size;

  // Each source is OpSize in size
  // So you can have up to a 128bit divide from x86-64
  switch (OpSize) {
    case 2: {
      mov (ax, GetSrc<RA_16>(Op->Header.Args[0].ID()));
      mov (dx, GetSrc<RA_16>(Op->Header.Args[1].ID()));
      div(GetSrc<RA_16>(Op->Header.Args[2].ID()));
      movzx(GetDst<RA_64>(Node), dx);
      break;
    }
    case 4: {
      mov (eax, GetSrc<RA_32>(Op->Header.Args[0].ID()));
      mov (edx, GetSrc<RA_32>(Op->Header.Args[1].ID()));
      div(GetSrc<RA_32>(Op->Header.Args[2].ID()));
      mov(GetDst<RA_64>(Node), rdx);
      break;
    }
    case 8: {
      mov (rax, GetSrc<RA_64>(Op->Header.Args[0].ID()));
      mov (rdx, GetSrc<RA_64>(Op->Header.Args[1].ID()));
      div(GetSrc<RA_64>(Op->Header.Args[2].ID()));
      mov(GetDst<RA_64>(Node), rdx);
      break;
    }
    default: LogMan::Msg::A("Unknown LUDIV OpSize: %d", OpSize); break;
  }
}


DEF_OP(Not) {
  auto Op = IROp->C<IR::IROp_Not>();
  auto Dst = GetDst<RA_64>(Node);
  mov(Dst, GetSrc<RA_64>(Op->Header.Args[0].ID()));
  not_(Dst);
}

DEF_OP(Popcount) {
  auto Op = IROp->C<IR::IROp_Popcount>();
  uint8_t OpSize = IROp->Size;

  auto Dst64 = GetDst<RA_64>(Node);

  switch (OpSize) {
    case 1:
      movzx(GetDst<RA_32>(Node), GetSrc<RA_8>(Op->Header.Args[0].ID()));
      popcnt(Dst64, Dst64);
      break;
    case 2: {
      movzx(GetDst<RA_32>(Node), GetSrc<RA_16>(Op->Header.Args[0].ID()));
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
}

DEF_OP(FindLSB) {
  auto Op = IROp->C<IR::IROp_FindLSB>();

  bsf(rcx, GetSrc<RA_64>(Op->Header.Args[0].ID()));
  mov(rax, 0x40);
  cmovz(rcx, rax);
  xor(rax, rax);
  cmp(GetSrc<RA_64>(Op->Header.Args[0].ID()), 1);
  sbb(rax, rax);
  or(rax, rcx);
  mov (GetDst<RA_64>(Node), rax);
}

DEF_OP(FindMSB) {
  auto Op = IROp->C<IR::IROp_FindMSB>();
  uint8_t OpSize = IROp->Size;

  switch (OpSize) {
    case 2:
      bsr(GetDst<RA_16>(Node), GetSrc<RA_16>(Op->Header.Args[0].ID()));
      movzx(GetDst<RA_32>(Node), GetDst<RA_16>(Node));
      break;
    case 4:
      bsr(GetDst<RA_32>(Node), GetSrc<RA_32>(Op->Header.Args[0].ID()));
      break;
    case 8:
      bsr(GetDst<RA_64>(Node), GetSrc<RA_64>(Op->Header.Args[0].ID()));
      break;
    default: LogMan::Msg::A("Unknown OpSize: %d", OpSize);
  }
}

DEF_OP(FindTrailingZeros) {
  auto Op = IROp->C<IR::IROp_FindTrailingZeros>();
  uint8_t OpSize = IROp->Size;

  switch (OpSize) {
    case 2:
      bsf(GetDst<RA_16>(Node), GetSrc<RA_16>(Op->Header.Args[0].ID()));
      mov(ax, 0x10);
      cmovz(GetDst<RA_16>(Node), ax);
    break;
    case 4:
      bsf(GetDst<RA_32>(Node), GetSrc<RA_32>(Op->Header.Args[0].ID()));
      mov(eax, 0x20);
      cmovz(GetDst<RA_32>(Node), eax);
      break;
    case 8:
      bsf(GetDst<RA_64>(Node), GetSrc<RA_64>(Op->Header.Args[0].ID()));
      mov(rax, 0x40);
      cmovz(GetDst<RA_64>(Node), rax);
      break;
    default: LogMan::Msg::A("Unknown size: %d", OpSize); break;
  }
}

DEF_OP(CountLeadingZeroes) {
  auto Op = IROp->C<IR::IROp_CountLeadingZeroes>();
  uint8_t OpSize = IROp->Size;

  if (Features.has(Xbyak::util::Cpu::tLZCNT)) {
    switch (OpSize) {
      case 2: {
        lzcnt(GetDst<RA_16>(Node), GetSrc<RA_16>(Op->Header.Args[0].ID()));
        movzx(GetDst<RA_32>(Node), GetDst<RA_16>(Node));
        break;
      }
      case 4: {
        lzcnt(GetDst<RA_32>(Node), GetSrc<RA_32>(Op->Header.Args[0].ID()));
        break;
      }
      case 8: {
        lzcnt(GetDst<RA_64>(Node), GetSrc<RA_64>(Op->Header.Args[0].ID()));
        break;
      }
      default: LogMan::Msg::A("Unknown size: %d", OpSize); break;
    }
  }
  else {
    switch (OpSize) {
      case 2: {
        test(GetSrc<RA_16>(Op->Header.Args[0].ID()), GetSrc<RA_16>(Op->Header.Args[0].ID()));
        mov(eax, 0x10);
        Label Skip;
        je(Skip);
          bsr(ax, GetSrc<RA_16>(Op->Header.Args[0].ID()));
          xor(ax, 0xF);
          movzx(eax, ax);
        L(Skip);
        mov(GetDst<RA_32>(Node), eax);
        break;
      }
      case 4: {
        test(GetSrc<RA_32>(Op->Header.Args[0].ID()), GetSrc<RA_32>(Op->Header.Args[0].ID()));
        mov(eax, 0x20);
        Label Skip;
        je(Skip);
          bsr(eax, GetSrc<RA_32>(Op->Header.Args[0].ID()));
          xor(eax, 0x1F);
        L(Skip);
        mov(GetDst<RA_32>(Node), eax);
        break;
      }
      case 8: {
        test(GetSrc<RA_64>(Op->Header.Args[0].ID()), GetSrc<RA_64>(Op->Header.Args[0].ID()));
        mov(rax, 0x40);
        Label Skip;
        je(Skip);
          bsr(rax, GetSrc<RA_64>(Op->Header.Args[0].ID()));
          xor(rax, 0x3F);
        L(Skip);
        mov(GetDst<RA_64>(Node), rax);
        break;
      }
      default: LogMan::Msg::A("Unknown size: %d", OpSize); break;
    }
  }
}

DEF_OP(Rev) {
  auto Op = IROp->C<IR::IROp_Rev>();
  uint8_t OpSize = IROp->Size;

  switch (OpSize) {
    case 2:
      mov (GetDst<RA_32>(Node), GetSrc<RA_32>(Op->Header.Args[0].ID()));
      rol(GetDst<RA_16>(Node), 8);
    break;
    case 4:
      mov (GetDst<RA_32>(Node), GetSrc<RA_32>(Op->Header.Args[0].ID()));
      bswap(GetDst<RA_32>(Node).cvt32());
      break;
    case 8:
      mov (GetDst<RA_64>(Node), GetSrc<RA_64>(Op->Header.Args[0].ID()));
      bswap(GetDst<RA_64>(Node).cvt64());
      break;
    default: LogMan::Msg::A("Unknown REV size: %d", OpSize); break;
  }
}

DEF_OP(Bfi) {
  auto Op = IROp->C<IR::IROp_Bfi>();
  uint8_t OpSize = IROp->Size;

  auto Dst = GetDst<RA_64>(Node);

  uint64_t SourceMask = (1ULL << Op->Width) - 1;
  if (Op->Width == 64)
    SourceMask = ~0ULL;
  uint64_t DestMask = ~(SourceMask << Op->lsb);

  mov(TMP1, GetSrc<RA_64>(Op->Header.Args[1].ID()));
  mov(Dst, GetSrc<RA_64>(Op->Header.Args[0].ID()));

  mov(TMP2, DestMask);
  and(Dst, TMP2);
  mov(TMP2, SourceMask);
  and(TMP1, TMP2);
  shl(TMP1, Op->lsb);
  or_(Dst, TMP1);

  if (OpSize != 8) {
    mov(rcx, uint64_t((1ULL << (OpSize * 8)) - 1));
    and(Dst, rcx);
  }
}

DEF_OP(Bfe) {
  auto Op = IROp->C<IR::IROp_Bfe>();
  uint8_t OpSize = IROp->Size;

  LogMan::Throw::A(OpSize <= 8, "OpSize is too large for BFE: %d", OpSize);

  auto Dst = GetDst<RA_64>(Node);

  // Special cases for fast extends
  if (Op->lsb == 0) {
    switch (Op->Width / 8) {
    case 1:
      movzx(Dst, GetSrc<RA_8>(Op->Header.Args[0].ID()));
      return;
    case 2:
      movzx(Dst, GetSrc<RA_16>(Op->Header.Args[0].ID()));
      return;
    case 4:
      mov(Dst.cvt32(), GetSrc<RA_32>(Op->Header.Args[0].ID()));
      return;
    case 8:
      mov(Dst, GetSrc<RA_64>(Op->Header.Args[0].ID()));
      return;
    default:
      // Need to use slower general case
      break;
    }
  }

  mov(Dst, GetSrc<RA_64>(Op->Header.Args[0].ID()));

  if (Op->lsb != 0)
    shr(Dst, Op->lsb);

  if (Op->Width != 64) {
    mov(rcx, uint64_t((1ULL << Op->Width) - 1));
    and(Dst, rcx);
  }
}

DEF_OP(Sbfe) {
  auto Op = IROp->C<IR::IROp_Sbfe>();
  auto Dst = GetDst<RA_64>(Node);

  // Special cases for fast signed extends
  if (Op->lsb == 0) {
    switch (Op->Width / 8) {
    case 1:
      movsx(Dst, GetSrc<RA_8>(Op->Header.Args[0].ID()));
      return;
    case 2:
      movsx(Dst, GetSrc<RA_16>(Op->Header.Args[0].ID()));
      return;
    case 4:
      movsxd(Dst.cvt64(), GetSrc<RA_32>(Op->Header.Args[0].ID()));
      return;
    case 8:
      mov(Dst, GetSrc<RA_64>(Op->Header.Args[0].ID()));
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
    mov(Dst, GetSrc<RA_64>(Op->Header.Args[0].ID()));
    shl(Dst, ShiftLeftAmount);
    sar(Dst, ShiftRightAmount);
  }
}

#define GRS(Node) (IROp->Size <= 4 ? GetSrc<RA_32>(Node) : GetSrc<RA_64>(Node))
#define GRD(Node) (IROp->Size <= 4 ? GetDst<RA_32>(Node) : GetDst<RA_64>(Node))
#define GRCMP(Node) (Op->CompareSize == 4 ? GetSrc<RA_32>(Node) : GetSrc<RA_64>(Node))

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
  } else if (IsFPR(Op->Cmp1.ID())) {
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
      LogMan::Msg::A("Select: Unsupported compare inline parameters");
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
  auto Op = IROp->C<IR::IROp_VExtractToGPR>();

  switch (Op->Header.ElementSize) {
    case 1: {
      pextrb(GetDst<RA_32>(Node), GetSrc(Op->Header.Args[0].ID()), Op->Idx);
    break;
    }
    case 2: {
      pextrw(GetDst<RA_32>(Node), GetSrc(Op->Header.Args[0].ID()), Op->Idx);
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
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
  }
}

DEF_OP(Float_ToGPR_ZU) {
  LogMan::Msg::D("Unimplemented");
}

DEF_OP(Float_ToGPR_ZS) {
  auto Op = IROp->C<IR::IROp_Float_ToGPR_ZS>();
  if (Op->Header.ElementSize == 8) {
    cvttsd2si(GetDst<RA_64>(Node), GetSrc(Op->Header.Args[0].ID()));
  }
  else {
    cvttss2si(GetDst<RA_32>(Node), GetSrc(Op->Header.Args[0].ID()));
  }
}

DEF_OP(Float_ToGPR_U) {
  LogMan::Msg::D("Unimplemented");
}

DEF_OP(Float_ToGPR_S) {
  auto Op = IROp->C<IR::IROp_Float_ToGPR_S>();
  if (Op->Header.ElementSize == 8) {
    cvtsd2si(GetDst<RA_64>(Node), GetSrc(Op->Header.Args[0].ID()));
  }
  else {
    cvtss2si(GetDst<RA_32>(Node), GetSrc(Op->Header.Args[0].ID()));
  }
}

DEF_OP(FCmp) {
  auto Op = IROp->C<IR::IROp_FCmp>();

  if (Op->ElementSize == 4) {
    ucomiss(GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
  }
  else {
    ucomisd(GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
  }
  mov (rdx, 0);

  lahf();
  if (Op->Flags & (1 << IR::FCMP_FLAG_LT)) {
    sahf();
    mov(rcx, 0);
    setb(cl);
    shl(rcx, IR::FCMP_FLAG_LT);
    or(rdx, rcx);
  }
  if (Op->Flags & (1 << IR::FCMP_FLAG_UNORDERED)) {
    sahf();
    mov(rcx, 0);
    setp(cl);
    shl(rcx, IR::FCMP_FLAG_UNORDERED);
    or(rdx, rcx);
  }
  if (Op->Flags & (1 << IR::FCMP_FLAG_EQ)) {
    sahf();
    mov(rcx, 0);
    setz(cl);
    shl(rcx, IR::FCMP_FLAG_EQ);
    or(rdx, rcx);
  }
  mov (GetDst<RA_64>(Node), rdx);
}

#undef DEF_OP

void JITCore::RegisterALUHandlers() {
#define REGISTER_OP(op, x) OpHandlers[FEXCore::IR::IROps::OP_##op] = &JITCore::Op_##x
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
  REGISTER_OP(XOR,               Xor);
  REGISTER_OP(LSHL,              Lshl);
  REGISTER_OP(LSHR,              Lshr);
  REGISTER_OP(ASHR,              Ashr);
  REGISTER_OP(ROR,               Ror);
  REGISTER_OP(EXTR,              Extr);
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
  REGISTER_OP(FLOAT_TOGPR_ZU,    Float_ToGPR_ZU);
  REGISTER_OP(FLOAT_TOGPR_ZS,    Float_ToGPR_ZS);
  REGISTER_OP(FLOAT_TOGPR_U,     Float_ToGPR_U);
  REGISTER_OP(FLOAT_TOGPR_S,     Float_ToGPR_S);
  REGISTER_OP(FCMP,              FCmp);
#undef REGISTER_OP
}


}
