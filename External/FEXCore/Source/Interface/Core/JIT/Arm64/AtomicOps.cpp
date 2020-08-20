#include "Interface/Core/JIT/Arm64/JITClass.h"

namespace FEXCore::CPU {
using namespace vixl;
using namespace vixl::aarch64;
#define DEF_OP(x) void JITCore::Op_##x(FEXCore::IR::IROp_Header *IROp, uint32_t Node)
DEF_OP(CASPair) {
  auto Op = IROp->C<IR::IROp_CASPair>();
  uint8_t OpSize = IROp->Size;
  // Size is the size of each pair element
  auto Dst = GetSrcPair<RA_64>(Node);
  auto Expected = GetSrcPair<RA_64>(Op->Header.Args[0].ID());
  auto Desired = GetSrcPair<RA_64>(Op->Header.Args[1].ID());
  auto MemSrc = GetReg<RA_64>(Op->Header.Args[2].ID());

  if (!CTX->Config.UnifiedMemory) {
    LoadConstant(TMP1, (uint64_t)CTX->MemoryMapper.GetMemoryBase());
    add(TMP1, TMP1, MemSrc);
    MemSrc = TMP1;
  }

  if (SupportsAtomics) {
    mov(TMP3, Expected.first);
    mov(TMP4, Expected.second);

    switch (OpSize) {
    case 4:
      caspal(TMP3.W(), TMP4.W(), Desired.first.W(), Desired.second.W(), MemOperand(MemSrc));
      mov(Dst.first.W(), TMP3.W());
      mov(Dst.second.W(), TMP4.W());
      break;
    case 8:
      caspal(TMP3.X(), TMP4.X(), Desired.first.X(), Desired.second.X(), MemOperand(MemSrc));
      mov(Dst.first, TMP3);
      mov(Dst.second, TMP4);
      break;
    default: LogMan::Msg::A("Unsupported: %d", OpSize);
    }
  }
  else {
    switch (OpSize) {
      case 4: {
        aarch64::Label LoopTop;
        aarch64::Label LoopNotExpected;
        aarch64::Label LoopExpected;
        bind(&LoopTop);
        ldaxp(TMP2.W(), TMP3.W(), MemOperand(MemSrc));
        cmp(TMP2.W(), Expected.first.W());
        ccmp(TMP3.W(), Expected.second.W(), NoFlag, Condition::eq);
        b(&LoopNotExpected, Condition::ne);
        stlxp(TMP2.W(), Desired.first.W(), Desired.second.W(), MemOperand(MemSrc));
        cbnz(TMP2.W(), &LoopTop);
        mov(Dst.first.W(), Expected.first.W());
        mov(Dst.second.W(), Expected.second.W());

        b(&LoopExpected);

          bind(&LoopNotExpected);
          mov(Dst.first.W(), TMP2.W());
          mov(Dst.second.W(), TMP3.W());
          // exclusive monitor needs to be cleared here
          // Might have hit the case where ldaxr was hit but stlxr wasn't
          clrex();
        bind(&LoopExpected);
        break;
      }
      case 8: {
        aarch64::Label LoopTop;
        aarch64::Label LoopNotExpected;
        aarch64::Label LoopExpected;
        bind(&LoopTop);
        ldaxp(TMP2.X(), TMP3.X(), MemOperand(MemSrc));
        cmp(TMP2.X(), Expected.first.X());
        ccmp(TMP3.X(), Expected.second.X(), NoFlag, Condition::eq);
        b(&LoopNotExpected, Condition::ne);
        stlxp(TMP2.X(), Desired.first.X(), Desired.second.X(), MemOperand(MemSrc));
        cbnz(TMP2.X(), &LoopTop);
        mov(Dst.first.X(), Expected.first.X());
        mov(Dst.second.X(), Expected.second.X());

        b(&LoopExpected);

          bind(&LoopNotExpected);
          mov(Dst.first.X(), TMP2.X());
          mov(Dst.second.X(), TMP3.X());
          // exclusive monitor needs to be cleared here
          // Might have hit the case where ldaxr was hit but stlxr wasn't
          clrex();
        bind(&LoopExpected);
        break;
      }
      default: LogMan::Msg::A("Unsupported: %d", OpSize);
    }
  }
}

DEF_OP(CAS) {
  auto Op = IROp->C<IR::IROp_CAS>();
  uint8_t OpSize = IROp->Size;
  // Args[0]: Expected
  // Args[1]: Desired
  // Args[2]: Pointer
  // DataSrc = *Src1
  // if (DataSrc == Src3) { *Src1 == Src2; } Src2 = DataSrc
  // This will write to memory! Careful!

  auto Expected = GetReg<RA_64>(Op->Header.Args[0].ID());
  auto Desired = GetReg<RA_64>(Op->Header.Args[1].ID());
  auto MemSrc = GetReg<RA_64>(Op->Header.Args[2].ID());

  if (!CTX->Config.UnifiedMemory) {
    LoadConstant(TMP1, (uint64_t)CTX->MemoryMapper.GetMemoryBase());
    add(TMP1, TMP1, MemSrc);
    MemSrc = TMP1;
  }

  if (SupportsAtomics) {
    mov(TMP2, Expected);
    switch (OpSize) {
    case 1: casalb(TMP2.W(), Desired.W(), MemOperand(MemSrc)); break;
    case 2: casalh(TMP2.W(), Desired.W(), MemOperand(MemSrc)); break;
    case 4: casal(TMP2.W(), Desired.W(), MemOperand(MemSrc)); break;
    case 8: casal(TMP2.X(), Desired.X(), MemOperand(MemSrc)); break;
    default: LogMan::Msg::A("Unsupported: %d", OpSize);
    }
    mov(GetReg<RA_64>(Node), TMP2);
  }
  else {
    switch (OpSize) {
      case 1: {
        aarch64::Label LoopTop;
        aarch64::Label LoopNotExpected;
        aarch64::Label LoopExpected;
        bind(&LoopTop);
        ldaxrb(TMP2.W(), MemOperand(MemSrc));
        cmp(TMP2.W(), Operand(Expected.W(), Extend::UXTB));
        b(&LoopNotExpected, Condition::ne);
        stlxrb(TMP3.W(), Desired.W(), MemOperand(MemSrc));
        cbnz(TMP3.W(), &LoopTop);
        mov(GetReg<RA_32>(Node), Expected.W());
        b(&LoopExpected);

          bind(&LoopNotExpected);
          mov(GetReg<RA_32>(Node), TMP2.W());
          // exclusive monitor needs to be cleared here
          // Might have hit the case where ldaxr was hit but stlxr wasn't
          clrex();
        bind(&LoopExpected);
        break;
      }
      case 2: {
        aarch64::Label LoopTop;
        aarch64::Label LoopNotExpected;
        aarch64::Label LoopExpected;
        bind(&LoopTop);
        ldaxrh(TMP2.W(), MemOperand(MemSrc));
        cmp(TMP2.W(), Operand(Expected.W(), Extend::UXTH));
        b(&LoopNotExpected, Condition::ne);
        stlxrh(TMP3.W(), Desired.W(), MemOperand(MemSrc));
        cbnz(TMP3.W(), &LoopTop);
        mov(GetReg<RA_32>(Node), Expected.W());
        b(&LoopExpected);

          bind(&LoopNotExpected);
          mov(GetReg<RA_32>(Node), TMP2.W());
          // exclusive monitor needs to be cleared here
          // Might have hit the case where ldaxr was hit but stlxr wasn't
          clrex();
        bind(&LoopExpected);
        break;
      }
      case 4: {
        aarch64::Label LoopTop;
        aarch64::Label LoopNotExpected;
        aarch64::Label LoopExpected;
        bind(&LoopTop);
        ldaxr(TMP2.W(), MemOperand(MemSrc));
        cmp(TMP2.W(), Expected.W());
        b(&LoopNotExpected, Condition::ne);
        stlxr(TMP3.W(), Desired.W(), MemOperand(MemSrc));
        cbnz(TMP3.W(), &LoopTop);
        mov(GetReg<RA_32>(Node), Expected.W());
        b(&LoopExpected);

          bind(&LoopNotExpected);
          mov(GetReg<RA_32>(Node), TMP2.W());
          // exclusive monitor needs to be cleared here
          // Might have hit the case where ldaxr was hit but stlxr wasn't
          clrex();
        bind(&LoopExpected);
        break;
      }
      case 8: {
        aarch64::Label LoopTop;
        aarch64::Label LoopNotExpected;
        aarch64::Label LoopExpected;
        bind(&LoopTop);
        ldaxr(TMP2, MemOperand(MemSrc));
        cmp(TMP2, Expected);
        b(&LoopNotExpected, Condition::ne);
        stlxr(TMP2, Desired, MemOperand(MemSrc));
        cbnz(TMP2, &LoopTop);
        mov(GetReg<RA_64>(Node), Expected);
        b(&LoopExpected);

          bind(&LoopNotExpected);
          mov(GetReg<RA_64>(Node), TMP2);
          // exclusive monitor needs to be cleared here
          // Might have hit the case where ldaxr was hit but stlxr wasn't
          clrex();
        bind(&LoopExpected);

        break;
      }
      default:  LogMan::Msg::A("Unhandled Atomic size: %d", OpSize);
    }
  }
}

DEF_OP(AtomicAdd) {
  auto Op = IROp->C<IR::IROp_AtomicAdd>();

  auto MemSrc = GetReg<RA_64>(Op->Header.Args[0].ID());
  if (!CTX->Config.UnifiedMemory) {
    LoadConstant(TMP1, (uint64_t)CTX->MemoryMapper.GetMemoryBase());
    add(TMP1, TMP1, MemSrc);
    MemSrc = TMP1;
  }

  if (SupportsAtomics) {
    switch (Op->Size) {
    case 1: staddlb(GetReg<RA_32>(Op->Header.Args[1].ID()), MemOperand(MemSrc)); break;
    case 2: staddlh(GetReg<RA_32>(Op->Header.Args[1].ID()), MemOperand(MemSrc)); break;
    case 4: staddl(GetReg<RA_32>(Op->Header.Args[1].ID()), MemOperand(MemSrc)); break;
    case 8: staddl(GetReg<RA_64>(Op->Header.Args[1].ID()), MemOperand(MemSrc)); break;
    default:  LogMan::Msg::A("Unhandled Atomic size: %d", Op->Size);
    }
  }
  else {
    // TMP2-TMP3
    switch (Op->Size) {
      case 1: {
        aarch64::Label LoopTop;
        bind(&LoopTop);
        ldaxrb(TMP2.W(), MemOperand(MemSrc));
        add(TMP2.W(), TMP2.W(), GetReg<RA_32>(Op->Header.Args[1].ID()));
        stlxrb(TMP2.W(), TMP2.W(), MemOperand(MemSrc));
        cbnz(TMP2.W(), &LoopTop);
        break;
      }
      case 2: {
        aarch64::Label LoopTop;
        bind(&LoopTop);
        ldaxrh(TMP2.W(), MemOperand(MemSrc));
        add(TMP2.W(), TMP2.W(), GetReg<RA_32>(Op->Header.Args[1].ID()));
        stlxrh(TMP2.W(), TMP2.W(), MemOperand(MemSrc));
        cbnz(TMP2.W(), &LoopTop);
        break;
      }
      case 4: {
        aarch64::Label LoopTop;
        bind(&LoopTop);
        ldaxr(TMP2.W(), MemOperand(MemSrc));
        add(TMP2.W(), TMP2.W(), GetReg<RA_32>(Op->Header.Args[1].ID()));
        stlxr(TMP2.W(), TMP2.W(), MemOperand(MemSrc));
        cbnz(TMP2.W(), &LoopTop);
        break;
      }
      case 8: {
        aarch64::Label LoopTop;
        bind(&LoopTop);
        ldaxr(TMP2, MemOperand(MemSrc));
        add(TMP2, TMP2, GetReg<RA_64>(Op->Header.Args[1].ID()));
        stlxr(TMP2, TMP2, MemOperand(MemSrc));
        cbnz(TMP2, &LoopTop);
        break;
      }
      default:  LogMan::Msg::A("Unhandled Atomic size: %d", Op->Size);
    }
  }
}

DEF_OP(AtomicSub) {
  auto Op = IROp->C<IR::IROp_AtomicSub>();

  auto MemSrc = GetReg<RA_64>(Op->Header.Args[0].ID());
  if (!CTX->Config.UnifiedMemory) {
    LoadConstant(TMP1, (uint64_t)CTX->MemoryMapper.GetMemoryBase());
    add(TMP1, TMP1, MemSrc);
    MemSrc = TMP1;
  }

  if (SupportsAtomics) {
    neg(TMP2, GetReg<RA_64>(Op->Header.Args[1].ID()));
    switch (Op->Size) {
    case 1: staddlb(TMP2.W(), MemOperand(MemSrc)); break;
    case 2: staddlh(TMP2.W(), MemOperand(MemSrc)); break;
    case 4: staddl(TMP2.W(), MemOperand(MemSrc)); break;
    case 8: staddl(TMP2.X(), MemOperand(MemSrc)); break;
    default:  LogMan::Msg::A("Unhandled Atomic size: %d", Op->Size);
    }
  }
  else {
    // TMP2-TMP3
    switch (Op->Size) {
      case 1: {
        aarch64::Label LoopTop;
        bind(&LoopTop);
        ldaxrb(TMP2.W(), MemOperand(MemSrc));
        sub(TMP2.W(), TMP2.W(), GetReg<RA_32>(Op->Header.Args[1].ID()));
        stlxrb(TMP2.W(), TMP2.W(), MemOperand(MemSrc));
        cbnz(TMP2.W(), &LoopTop);
        break;
      }
      case 2: {
        aarch64::Label LoopTop;
        bind(&LoopTop);
        ldaxrh(TMP2.W(), MemOperand(MemSrc));
        sub(TMP2.W(), TMP2.W(), GetReg<RA_32>(Op->Header.Args[1].ID()));
        stlxrh(TMP2.W(), TMP2.W(), MemOperand(MemSrc));
        cbnz(TMP2.W(), &LoopTop);
        break;
      }
      case 4: {
        aarch64::Label LoopTop;
        bind(&LoopTop);
        ldaxr(TMP2.W(), MemOperand(MemSrc));
        sub(TMP2.W(), TMP2.W(), GetReg<RA_32>(Op->Header.Args[1].ID()));
        stlxr(TMP2.W(), TMP2.W(), MemOperand(MemSrc));
        cbnz(TMP2.W(), &LoopTop);
        break;
      }
      case 8: {
        aarch64::Label LoopTop;
        bind(&LoopTop);
        ldaxr(TMP2, MemOperand(MemSrc));
        sub(TMP2, TMP2, GetReg<RA_64>(Op->Header.Args[1].ID()));
        stlxr(TMP2, TMP2, MemOperand(MemSrc));
        cbnz(TMP2, &LoopTop);
        break;
      }
      default:  LogMan::Msg::A("Unhandled Atomic size: %d", Op->Size);
    }
  }
}

DEF_OP(AtomicAnd) {
  auto Op = IROp->C<IR::IROp_AtomicAnd>();

  auto MemSrc = GetReg<RA_64>(Op->Header.Args[0].ID());
  if (!CTX->Config.UnifiedMemory) {
    LoadConstant(TMP1, (uint64_t)CTX->MemoryMapper.GetMemoryBase());
    add(TMP1, TMP1, MemSrc);
    MemSrc = TMP1;
  }

  if (SupportsAtomics) {
    mvn(TMP2, GetReg<RA_64>(Op->Header.Args[1].ID()));
    switch (Op->Size) {
    case 1: stclrlb(TMP2.W(), MemOperand(MemSrc)); break;
    case 2: stclrlh(TMP2.W(), MemOperand(MemSrc)); break;
    case 4: stclrl(TMP2.W(), MemOperand(MemSrc)); break;
    case 8: stclrl(TMP2.X(), MemOperand(MemSrc)); break;
    default:  LogMan::Msg::A("Unhandled Atomic size: %d", Op->Size);
    }
  }
  else {
    // TMP2-TMP3
    switch (Op->Size) {
      case 1: {
        aarch64::Label LoopTop;
        bind(&LoopTop);
        ldaxrb(TMP2.W(), MemOperand(MemSrc));
        and_(TMP2.W(), TMP2.W(), GetReg<RA_32>(Op->Header.Args[1].ID()));
        stlxrb(TMP2.W(), TMP2.W(), MemOperand(MemSrc));
        cbnz(TMP2.W(), &LoopTop);
        break;
      }
      case 2: {
        aarch64::Label LoopTop;
        bind(&LoopTop);
        ldaxrh(TMP2.W(), MemOperand(MemSrc));
        and_(TMP2.W(), TMP2.W(), GetReg<RA_32>(Op->Header.Args[1].ID()));
        stlxrh(TMP2.W(), TMP2.W(), MemOperand(MemSrc));
        cbnz(TMP2.W(), &LoopTop);
        break;
      }
      case 4: {
        aarch64::Label LoopTop;
        bind(&LoopTop);
        ldaxr(TMP2.W(), MemOperand(MemSrc));
        and_(TMP2.W(), TMP2.W(), GetReg<RA_32>(Op->Header.Args[1].ID()));
        stlxr(TMP2.W(), TMP2.W(), MemOperand(MemSrc));
        cbnz(TMP2.W(), &LoopTop);
        break;
      }
      case 8: {
        aarch64::Label LoopTop;
        bind(&LoopTop);
        ldaxr(TMP2, MemOperand(MemSrc));
        and_(TMP2, TMP2, GetReg<RA_64>(Op->Header.Args[1].ID()));
        stlxr(TMP2, TMP2, MemOperand(MemSrc));
        cbnz(TMP2, &LoopTop);
        break;
      }
      default:  LogMan::Msg::A("Unhandled Atomic size: %d", Op->Size);
    }
  }
}

DEF_OP(AtomicOr) {
  auto Op = IROp->C<IR::IROp_AtomicOr>();

  auto MemSrc = GetReg<RA_64>(Op->Header.Args[0].ID());
  if (!CTX->Config.UnifiedMemory) {
    LoadConstant(TMP1, (uint64_t)CTX->MemoryMapper.GetMemoryBase());
    add(TMP1, TMP1, MemSrc);
    MemSrc = TMP1;
  }

  if (SupportsAtomics) {
    switch (Op->Size) {
    case 1: stsetlb(GetReg<RA_32>(Op->Header.Args[1].ID()), MemOperand(MemSrc)); break;
    case 2: stsetlh(GetReg<RA_32>(Op->Header.Args[1].ID()), MemOperand(MemSrc)); break;
    case 4: stsetl(GetReg<RA_32>(Op->Header.Args[1].ID()), MemOperand(MemSrc)); break;
    case 8: stsetl(GetReg<RA_64>(Op->Header.Args[1].ID()), MemOperand(MemSrc)); break;
    default:  LogMan::Msg::A("Unhandled Atomic size: %d", Op->Size);
    }
  }
  else {
    // TMP2-TMP3
    switch (Op->Size) {
      case 1: {
        aarch64::Label LoopTop;
        bind(&LoopTop);
        ldaxrb(TMP2.W(), MemOperand(MemSrc));
        orr(TMP2.W(), TMP2.W(), GetReg<RA_32>(Op->Header.Args[1].ID()));
        stlxrb(TMP2.W(), TMP2.W(), MemOperand(MemSrc));
        cbnz(TMP2.W(), &LoopTop);
        break;
      }
      case 2: {
        aarch64::Label LoopTop;
        bind(&LoopTop);
        ldaxrh(TMP2.W(), MemOperand(MemSrc));
        orr(TMP2.W(), TMP2.W(), GetReg<RA_32>(Op->Header.Args[1].ID()));
        stlxrh(TMP2.W(), TMP2.W(), MemOperand(MemSrc));
        cbnz(TMP2.W(), &LoopTop);
        break;
      }
      case 4: {
        aarch64::Label LoopTop;
        bind(&LoopTop);
        ldaxr(TMP2.W(), MemOperand(MemSrc));
        orr(TMP2.W(), TMP2.W(), GetReg<RA_32>(Op->Header.Args[1].ID()));
        stlxr(TMP2.W(), TMP2.W(), MemOperand(MemSrc));
        cbnz(TMP2.W(), &LoopTop);
        break;
      }
      case 8: {
        aarch64::Label LoopTop;
        bind(&LoopTop);
        ldaxr(TMP2, MemOperand(MemSrc));
        orr(TMP2, TMP2, GetReg<RA_64>(Op->Header.Args[1].ID()));
        stlxr(TMP2, TMP2, MemOperand(MemSrc));
        cbnz(TMP2, &LoopTop);
        break;
      }
      default:  LogMan::Msg::A("Unhandled Atomic size: %d", Op->Size);
    }
  }
}

DEF_OP(AtomicXor) {
  auto Op = IROp->C<IR::IROp_AtomicXor>();

  auto MemSrc = GetReg<RA_64>(Op->Header.Args[0].ID());
  if (!CTX->Config.UnifiedMemory) {
    LoadConstant(TMP1, (uint64_t)CTX->MemoryMapper.GetMemoryBase());
    add(TMP1, TMP1, MemSrc);
    MemSrc = TMP1;
  }

  if (SupportsAtomics) {
    switch (Op->Size) {
    case 1: steorlb(GetReg<RA_32>(Op->Header.Args[1].ID()), MemOperand(MemSrc)); break;
    case 2: steorlh(GetReg<RA_32>(Op->Header.Args[1].ID()), MemOperand(MemSrc)); break;
    case 4: steorl(GetReg<RA_32>(Op->Header.Args[1].ID()), MemOperand(MemSrc)); break;
    case 8: steorl(GetReg<RA_64>(Op->Header.Args[1].ID()), MemOperand(MemSrc)); break;
    default:  LogMan::Msg::A("Unhandled Atomic size: %d", Op->Size);
    }
  }
  else {
    // TMP2-TMP3
    switch (Op->Size) {
      case 1: {
        aarch64::Label LoopTop;
        bind(&LoopTop);
        ldaxrb(TMP2.W(), MemOperand(MemSrc));
        eor(TMP2.W(), TMP2.W(), GetReg<RA_32>(Op->Header.Args[1].ID()));
        stlxrb(TMP2.W(), TMP2.W(), MemOperand(MemSrc));
        cbnz(TMP2.W(), &LoopTop);
        break;
      }
      case 2: {
        aarch64::Label LoopTop;
        bind(&LoopTop);
        ldaxrh(TMP2.W(), MemOperand(MemSrc));
        eor(TMP2.W(), TMP2.W(), GetReg<RA_32>(Op->Header.Args[1].ID()));
        stlxrh(TMP2.W(), TMP2.W(), MemOperand(MemSrc));
        cbnz(TMP2.W(), &LoopTop);
        break;
      }
      case 4: {
        aarch64::Label LoopTop;
        bind(&LoopTop);
        ldaxr(TMP2.W(), MemOperand(MemSrc));
        eor(TMP2.W(), TMP2.W(), GetReg<RA_32>(Op->Header.Args[1].ID()));
        stlxr(TMP2.W(), TMP2.W(), MemOperand(MemSrc));
        cbnz(TMP2.W(), &LoopTop);
        break;
      }
      case 8: {
        aarch64::Label LoopTop;
        bind(&LoopTop);
        ldaxr(TMP2, MemOperand(MemSrc));
        eor(TMP2, TMP2, GetReg<RA_64>(Op->Header.Args[1].ID()));
        stlxr(TMP2, TMP2, MemOperand(MemSrc));
        cbnz(TMP2, &LoopTop);
        break;
      }
      default:  LogMan::Msg::A("Unhandled Atomic size: %d", Op->Size);
    }
  }
}

DEF_OP(AtomicSwap) {
  auto Op = IROp->C<IR::IROp_AtomicSwap>();

  auto MemSrc = GetReg<RA_64>(Op->Header.Args[0].ID());
  if (!CTX->Config.UnifiedMemory) {
    LoadConstant(TMP1, (uint64_t)CTX->MemoryMapper.GetMemoryBase());
    add(TMP1, TMP1, MemSrc);
    MemSrc = TMP1;
  }

  if (SupportsAtomics) {
    mov(TMP2, GetReg<RA_64>(Op->Header.Args[1].ID()));
    switch (Op->Size) {
    case 1: swplb(TMP2.W(), GetReg<RA_32>(Node), MemOperand(MemSrc)); break;
    case 2: swplh(TMP2.W(), GetReg<RA_32>(Node), MemOperand(MemSrc)); break;
    case 4: swpl(TMP2.W(), GetReg<RA_32>(Node), MemOperand(MemSrc)); break;
    case 8: swpl(TMP2.X(), GetReg<RA_64>(Node), MemOperand(MemSrc)); break;
    default:  LogMan::Msg::A("Unhandled Atomic size: %d", Op->Size);
    }
  }
  else {
    // TMP2-TMP3
    mov(TMP3, GetReg<RA_64>(Op->Header.Args[1].ID()));
    switch (Op->Size) {
      case 1: {
        aarch64::Label LoopTop;
        bind(&LoopTop);
        ldaxrb(TMP2.W(), MemOperand(MemSrc));
        stlxrb(TMP4.W(), TMP3.W(), MemOperand(MemSrc));
        cbnz(TMP4.W(), &LoopTop);
        uxtb(GetReg<RA_64>(Node), TMP2.W());
        break;
      }
      case 2: {
        aarch64::Label LoopTop;
        bind(&LoopTop);
        ldaxrh(TMP2.W(), MemOperand(MemSrc));
        stlxrh(TMP4.W(), TMP3.W(), MemOperand(MemSrc));
        cbnz(TMP4.W(), &LoopTop);
        uxtw(GetReg<RA_64>(Node), TMP2.W());
        break;
      }
      case 4: {
        aarch64::Label LoopTop;
        bind(&LoopTop);
        ldaxr(TMP2.W(), MemOperand(MemSrc));
        stlxr(TMP4.W(), TMP3.W(), MemOperand(MemSrc));
        cbnz(TMP4.W(), &LoopTop);
        mov(GetReg<RA_32>(Node), TMP2.W());
        break;
      }
      case 8: {
        aarch64::Label LoopTop;
        bind(&LoopTop);
        ldaxr(TMP2, MemOperand(MemSrc));
        stlxr(TMP4, TMP3.X(), MemOperand(MemSrc));
        cbnz(TMP4, &LoopTop);
        mov(GetReg<RA_64>(Node), TMP2.X());
        break;
      }
      default:  LogMan::Msg::A("Unhandled Atomic size: %d", Op->Size);
    }
  }
}

DEF_OP(AtomicFetchAdd) {
  auto Op = IROp->C<IR::IROp_AtomicFetchAdd>();
  auto MemSrc = GetReg<RA_64>(Op->Header.Args[0].ID());
  if (!CTX->Config.UnifiedMemory) {
    LoadConstant(TMP1, (uint64_t)CTX->MemoryMapper.GetMemoryBase());
    add(TMP1, TMP1, MemSrc);
    MemSrc = TMP1;
  }

  if (SupportsAtomics) {
    switch (Op->Size) {
    case 1: ldaddalb(GetReg<RA_32>(Op->Header.Args[1].ID()), GetReg<RA_32>(Node), MemOperand(MemSrc)); break;
    case 2: ldaddalh(GetReg<RA_32>(Op->Header.Args[1].ID()), GetReg<RA_32>(Node), MemOperand(MemSrc)); break;
    case 4: ldaddal(GetReg<RA_32>(Op->Header.Args[1].ID()), GetReg<RA_32>(Node), MemOperand(MemSrc)); break;
    case 8: ldaddal(GetReg<RA_64>(Op->Header.Args[1].ID()), GetReg<RA_64>(Node), MemOperand(MemSrc)); break;
    default:  LogMan::Msg::A("Unhandled Atomic size: %d", Op->Size);
    }
  }
  else {
    // TMP2-TMP3
    switch (Op->Size) {
      case 1: {
        aarch64::Label LoopTop;
        bind(&LoopTop);
        ldaxrb(TMP2.W(), MemOperand(MemSrc));
        add(TMP3.W(), TMP2.W(), GetReg<RA_32>(Op->Header.Args[1].ID()));
        stlxrb(TMP4.W(), TMP3.W(), MemOperand(MemSrc));
        cbnz(TMP4.W(), &LoopTop);
        mov(GetReg<RA_32>(Node), TMP2.W());
        break;
      }
      case 2: {
        aarch64::Label LoopTop;
        bind(&LoopTop);
        ldaxrh(TMP2.W(), MemOperand(MemSrc));
        add(TMP3.W(), TMP2.W(), GetReg<RA_32>(Op->Header.Args[1].ID()));
        stlxrh(TMP4.W(), TMP3.W(), MemOperand(MemSrc));
        cbnz(TMP4.W(), &LoopTop);
        mov(GetReg<RA_32>(Node), TMP2.W());
        break;
      }
      case 4: {
        aarch64::Label LoopTop;
        bind(&LoopTop);
        ldaxr(TMP2.W(), MemOperand(MemSrc));
        add(TMP3.W(), TMP2.W(), GetReg<RA_32>(Op->Header.Args[1].ID()));
        stlxr(TMP4.W(), TMP3.W(), MemOperand(MemSrc));
        cbnz(TMP4.W(), &LoopTop);
        mov(GetReg<RA_32>(Node), TMP2.W());
        break;
      }
      case 8: {
        aarch64::Label LoopTop;
        bind(&LoopTop);
        ldaxr(TMP2, MemOperand(MemSrc));
        add(TMP3, TMP2, GetReg<RA_64>(Op->Header.Args[1].ID()));
        stlxr(TMP4, TMP3, MemOperand(MemSrc));
        cbnz(TMP4, &LoopTop);
        mov(GetReg<RA_64>(Node), TMP2);
        break;
      }
      default:  LogMan::Msg::A("Unhandled Atomic size: %d", Op->Size);
    }
  }
}

DEF_OP(AtomicFetchSub) {
  auto Op = IROp->C<IR::IROp_AtomicFetchSub>();
  auto MemSrc = GetReg<RA_64>(Op->Header.Args[0].ID());
  if (!CTX->Config.UnifiedMemory) {
    LoadConstant(TMP1, (uint64_t)CTX->MemoryMapper.GetMemoryBase());
    add(TMP1, TMP1, MemSrc);
    MemSrc = TMP1;
  }

  if (SupportsAtomics) {
    neg(TMP2, GetReg<RA_64>(Op->Header.Args[1].ID()));
    switch (Op->Size) {
    case 1: ldaddalb(TMP2.W(), GetReg<RA_32>(Node), MemOperand(MemSrc)); break;
    case 2: ldaddalh(TMP2.W(), GetReg<RA_32>(Node), MemOperand(MemSrc)); break;
    case 4: ldaddal(TMP2.W(), GetReg<RA_32>(Node), MemOperand(MemSrc)); break;
    case 8: ldaddal(TMP2.X(), GetReg<RA_64>(Node), MemOperand(MemSrc)); break;
    default:  LogMan::Msg::A("Unhandled Atomic size: %d", Op->Size);
    }
  }
  else {
    // TMP2-TMP3
    switch (Op->Size) {
      case 1: {
        aarch64::Label LoopTop;
        bind(&LoopTop);
        ldaxrb(TMP2.W(), MemOperand(MemSrc));
        sub(TMP3.W(), TMP2.W(), GetReg<RA_32>(Op->Header.Args[1].ID()));
        stlxrb(TMP4.W(), TMP3.W(), MemOperand(MemSrc));
        cbnz(TMP4.W(), &LoopTop);
        mov(GetReg<RA_32>(Node), TMP2.W());
        break;
      }
      case 2: {
        aarch64::Label LoopTop;
        bind(&LoopTop);
        ldaxrh(TMP2.W(), MemOperand(MemSrc));
        sub(TMP3.W(), TMP2.W(), GetReg<RA_32>(Op->Header.Args[1].ID()));
        stlxrh(TMP4.W(), TMP3.W(), MemOperand(MemSrc));
        cbnz(TMP4.W(), &LoopTop);
        mov(GetReg<RA_32>(Node), TMP2.W());
        break;
      }
      case 4: {
        aarch64::Label LoopTop;
        bind(&LoopTop);
        ldaxr(TMP2.W(), MemOperand(MemSrc));
        sub(TMP3.W(), TMP2.W(), GetReg<RA_32>(Op->Header.Args[1].ID()));
        stlxr(TMP4.W(), TMP3.W(), MemOperand(MemSrc));
        cbnz(TMP4.W(), &LoopTop);
        mov(GetReg<RA_32>(Node), TMP2.W());
        break;
      }
      case 8: {
        aarch64::Label LoopTop;
        bind(&LoopTop);
        ldaxr(TMP2, MemOperand(MemSrc));
        sub(TMP3, TMP2, GetReg<RA_64>(Op->Header.Args[1].ID()));
        stlxr(TMP4, TMP3, MemOperand(MemSrc));
        cbnz(TMP4, &LoopTop);
        mov(GetReg<RA_64>(Node), TMP2);
        break;
      }
      default:  LogMan::Msg::A("Unhandled Atomic size: %d", Op->Size);
    }
  }
}

DEF_OP(AtomicFetchAnd) {
  auto Op = IROp->C<IR::IROp_AtomicFetchAnd>();
  auto MemSrc = GetReg<RA_64>(Op->Header.Args[0].ID());
  if (!CTX->Config.UnifiedMemory) {
    LoadConstant(TMP1, (uint64_t)CTX->MemoryMapper.GetMemoryBase());
    add(TMP1, TMP1, MemSrc);
    MemSrc = TMP1;
  }

  if (SupportsAtomics) {
    mvn(TMP2, GetReg<RA_64>(Op->Header.Args[1].ID()));
    switch (Op->Size) {
    case 1: ldclralb(TMP2.W(), GetReg<RA_32>(Node), MemOperand(MemSrc)); break;
    case 2: ldclralh(TMP2.W(), GetReg<RA_32>(Node), MemOperand(MemSrc)); break;
    case 4: ldclral(TMP2.W(), GetReg<RA_32>(Node), MemOperand(MemSrc)); break;
    case 8: ldclral(TMP2.X(), GetReg<RA_64>(Node), MemOperand(MemSrc)); break;
    default:  LogMan::Msg::A("Unhandled Atomic size: %d", Op->Size);
    }
  }
  else {
    // TMP2-TMP3
    switch (Op->Size) {
      case 1: {
        aarch64::Label LoopTop;
        bind(&LoopTop);
        ldaxrb(TMP2.W(), MemOperand(MemSrc));
        and_(TMP3.W(), TMP2.W(), GetReg<RA_32>(Op->Header.Args[1].ID()));
        stlxrb(TMP4.W(), TMP3.W(), MemOperand(MemSrc));
        cbnz(TMP4.W(), &LoopTop);
        mov(GetReg<RA_32>(Node), TMP2.W());
        break;
      }
      case 2: {
        aarch64::Label LoopTop;
        bind(&LoopTop);
        ldaxrh(TMP2.W(), MemOperand(MemSrc));
        and_(TMP3.W(), TMP2.W(), GetReg<RA_32>(Op->Header.Args[1].ID()));
        stlxrh(TMP4.W(), TMP3.W(), MemOperand(MemSrc));
        cbnz(TMP4.W(), &LoopTop);
        mov(GetReg<RA_32>(Node), TMP2.W());
        break;
      }
      case 4: {
        aarch64::Label LoopTop;
        bind(&LoopTop);
        ldaxr(TMP2.W(), MemOperand(MemSrc));
        and_(TMP3.W(), TMP2.W(), GetReg<RA_32>(Op->Header.Args[1].ID()));
        stlxr(TMP4.W(), TMP3.W(), MemOperand(MemSrc));
        cbnz(TMP4.W(), &LoopTop);
        mov(GetReg<RA_32>(Node), TMP2.W());
        break;
      }
      case 8: {
        aarch64::Label LoopTop;
        bind(&LoopTop);
        ldaxr(TMP2, MemOperand(MemSrc));
        and_(TMP3, TMP2, GetReg<RA_64>(Op->Header.Args[1].ID()));
        stlxr(TMP4, TMP3, MemOperand(MemSrc));
        cbnz(TMP4, &LoopTop);
        mov(GetReg<RA_64>(Node), TMP2);
        break;
      }
      default:  LogMan::Msg::A("Unhandled Atomic size: %d", Op->Size);
    }
  }
}

DEF_OP(AtomicFetchOr) {
  auto Op = IROp->C<IR::IROp_AtomicFetchOr>();
  auto MemSrc = GetReg<RA_64>(Op->Header.Args[0].ID());
  if (!CTX->Config.UnifiedMemory) {
    LoadConstant(TMP1, (uint64_t)CTX->MemoryMapper.GetMemoryBase());
    add(TMP1, TMP1, MemSrc);
    MemSrc = TMP1;
  }

  if (SupportsAtomics) {
    switch (Op->Size) {
    case 1: ldsetalb(GetReg<RA_32>(Op->Header.Args[1].ID()), GetReg<RA_32>(Node), MemOperand(MemSrc)); break;
    case 2: ldsetalh(GetReg<RA_32>(Op->Header.Args[1].ID()), GetReg<RA_32>(Node), MemOperand(MemSrc)); break;
    case 4: ldsetal(GetReg<RA_32>(Op->Header.Args[1].ID()), GetReg<RA_32>(Node), MemOperand(MemSrc)); break;
    case 8: ldsetal(GetReg<RA_64>(Op->Header.Args[1].ID()), GetReg<RA_64>(Node), MemOperand(MemSrc)); break;
    default:  LogMan::Msg::A("Unhandled Atomic size: %d", Op->Size);
    }
  }
  else {
    // TMP2-TMP3
    switch (Op->Size) {
      case 1: {
        aarch64::Label LoopTop;
        bind(&LoopTop);
        ldaxrb(TMP2.W(), MemOperand(MemSrc));
        orr(TMP3.W(), TMP2.W(), GetReg<RA_32>(Op->Header.Args[1].ID()));
        stlxrb(TMP4.W(), TMP3.W(), MemOperand(MemSrc));
        cbnz(TMP4.W(), &LoopTop);
        mov(GetReg<RA_32>(Node), TMP2.W());
        break;
      }
      case 2: {
        aarch64::Label LoopTop;
        bind(&LoopTop);
        ldaxrh(TMP2.W(), MemOperand(MemSrc));
        orr(TMP3.W(), TMP2.W(), GetReg<RA_32>(Op->Header.Args[1].ID()));
        stlxrh(TMP4.W(), TMP3.W(), MemOperand(MemSrc));
        cbnz(TMP4.W(), &LoopTop);
        mov(GetReg<RA_32>(Node), TMP2.W());
        break;
      }
      case 4: {
        aarch64::Label LoopTop;
        bind(&LoopTop);
        ldaxr(TMP2.W(), MemOperand(MemSrc));
        orr(TMP3.W(), TMP2.W(), GetReg<RA_32>(Op->Header.Args[1].ID()));
        stlxr(TMP4.W(), TMP3.W(), MemOperand(MemSrc));
        cbnz(TMP4.W(), &LoopTop);
        mov(GetReg<RA_32>(Node), TMP2.W());
        break;
      }
      case 8: {
        aarch64::Label LoopTop;
        bind(&LoopTop);
        ldaxr(TMP2, MemOperand(MemSrc));
        orr(TMP3, TMP2, GetReg<RA_64>(Op->Header.Args[1].ID()));
        stlxr(TMP4, TMP3, MemOperand(MemSrc));
        cbnz(TMP4, &LoopTop);
        mov(GetReg<RA_64>(Node), TMP2);
        break;
      }
      default:  LogMan::Msg::A("Unhandled Atomic size: %d", Op->Size);
    }
  }
}

DEF_OP(AtomicFetchXor) {
  auto Op = IROp->C<IR::IROp_AtomicFetchXor>();
  auto MemSrc = GetReg<RA_64>(Op->Header.Args[0].ID());
  if (!CTX->Config.UnifiedMemory) {
    LoadConstant(TMP1, (uint64_t)CTX->MemoryMapper.GetMemoryBase());
    add(TMP1, TMP1, MemSrc);
    MemSrc = TMP1;
  }

  if (SupportsAtomics) {
    switch (Op->Size) {
    case 1: ldeoralb(GetReg<RA_32>(Op->Header.Args[1].ID()), GetReg<RA_32>(Node), MemOperand(MemSrc)); break;
    case 2: ldeoralh(GetReg<RA_32>(Op->Header.Args[1].ID()), GetReg<RA_32>(Node), MemOperand(MemSrc)); break;
    case 4: ldeoral(GetReg<RA_32>(Op->Header.Args[1].ID()), GetReg<RA_32>(Node), MemOperand(MemSrc)); break;
    case 8: ldeoral(GetReg<RA_64>(Op->Header.Args[1].ID()), GetReg<RA_64>(Node), MemOperand(MemSrc)); break;
    default:  LogMan::Msg::A("Unhandled Atomic size: %d", Op->Size);
    }
  }
  else {
    // TMP2-TMP3
    switch (Op->Size) {
      case 1: {
        aarch64::Label LoopTop;
        bind(&LoopTop);
        ldaxrb(TMP2.W(), MemOperand(MemSrc));
        eor(TMP3.W(), TMP2.W(), GetReg<RA_32>(Op->Header.Args[1].ID()));
        stlxrb(TMP4.W(), TMP3.W(), MemOperand(MemSrc));
        cbnz(TMP4.W(), &LoopTop);
        mov(GetReg<RA_32>(Node), TMP2.W());
        break;
      }
      case 2: {
        aarch64::Label LoopTop;
        bind(&LoopTop);
        ldaxrh(TMP2.W(), MemOperand(MemSrc));
        eor(TMP3.W(), TMP2.W(), GetReg<RA_32>(Op->Header.Args[1].ID()));
        stlxrh(TMP4.W(), TMP3.W(), MemOperand(MemSrc));
        cbnz(TMP4.W(), &LoopTop);
        mov(GetReg<RA_32>(Node), TMP2.W());
        break;
      }
      case 4: {
        aarch64::Label LoopTop;
        bind(&LoopTop);
        ldaxr(TMP2.W(), MemOperand(MemSrc));
        eor(TMP3.W(), TMP2.W(), GetReg<RA_32>(Op->Header.Args[1].ID()));
        stlxr(TMP4.W(), TMP3.W(), MemOperand(MemSrc));
        cbnz(TMP4.W(), &LoopTop);
        mov(GetReg<RA_32>(Node), TMP2.W());
        break;
      }
      case 8: {
        aarch64::Label LoopTop;
        bind(&LoopTop);
        ldaxr(TMP2, MemOperand(MemSrc));
        eor(TMP3, TMP2, GetReg<RA_64>(Op->Header.Args[1].ID()));
        stlxr(TMP4, TMP3, MemOperand(MemSrc));
        cbnz(TMP4, &LoopTop);
        mov(GetReg<RA_64>(Node), TMP2);
        break;
      }
      default:  LogMan::Msg::A("Unhandled Atomic size: %d", Op->Size);
    }
  }
}

#undef DEF_OP
void JITCore::RegisterAtomicHandlers() {
#define REGISTER_OP(op, x) OpHandlers[FEXCore::IR::IROps::OP_##op] = &JITCore::Op_##x
  REGISTER_OP(CASPAIR,        CASPair);
  REGISTER_OP(CAS,            CAS);
  REGISTER_OP(ATOMICADD,      AtomicAdd);
  REGISTER_OP(ATOMICSUB,      AtomicSub);
  REGISTER_OP(ATOMICAND,      AtomicAnd);
  REGISTER_OP(ATOMICOR,       AtomicOr);
  REGISTER_OP(ATOMICXOR,      AtomicXor);
  REGISTER_OP(ATOMICSWAP,     AtomicSwap);
  REGISTER_OP(ATOMICFETCHADD, AtomicFetchAdd);
  REGISTER_OP(ATOMICFETCHSUB, AtomicFetchSub);
  REGISTER_OP(ATOMICFETCHAND, AtomicFetchAnd);
  REGISTER_OP(ATOMICFETCHOR,  AtomicFetchOr);
  REGISTER_OP(ATOMICFETCHXOR, AtomicFetchXor);
#undef REGISTER_OP
}
}

