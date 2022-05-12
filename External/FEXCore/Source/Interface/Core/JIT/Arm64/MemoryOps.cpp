/*
$info$
tags: backend|arm64
$end_info$
*/

#include "Interface/Core/JIT/Arm64/JITClass.h"
#include <FEXCore/Utils/CompilerDefs.h>

namespace FEXCore::CPU {

using namespace vixl;
using namespace vixl::aarch64;
#define DEF_OP(x) void Arm64JITCore::Op_##x(IR::IROp_Header *IROp, IR::NodeID Node)

DEF_OP(LoadContext) {
  auto Op = IROp->C<IR::IROp_LoadContext>();
  uint8_t OpSize = IROp->Size;
  if (Op->Class == FEXCore::IR::GPRClass) {
    switch (OpSize) {
    case 1:
      ldrb(GetReg<RA_32>(Node), MemOperand(STATE, Op->Offset));
    break;
    case 2:
      ldrh(GetReg<RA_32>(Node), MemOperand(STATE, Op->Offset));
    break;
    case 4:
      ldr(GetReg<RA_32>(Node), MemOperand(STATE, Op->Offset));
    break;
    case 8:
      ldr(GetReg<RA_64>(Node), MemOperand(STATE, Op->Offset));
    break;
    default:  LOGMAN_MSG_A_FMT("Unhandled LoadContext size: {}", OpSize);
    }
  }
  else {
    auto Dst = GetDst(Node);
    switch (OpSize) {
    case 1:
      ldr(Dst.B(), MemOperand(STATE, Op->Offset));
    break;
    case 2:
      ldr(Dst.H(), MemOperand(STATE, Op->Offset));
    break;
    case 4:
      ldr(Dst.S(), MemOperand(STATE, Op->Offset));
    break;
    case 8:
      ldr(Dst.D(), MemOperand(STATE, Op->Offset));
    break;
    case 16:
      ldr(Dst, MemOperand(STATE, Op->Offset));
    break;
    default:  LOGMAN_MSG_A_FMT("Unhandled LoadContext size: {}", OpSize);
    }
  }
}

DEF_OP(StoreContext) {
  auto Op = IROp->C<IR::IROp_StoreContext>();
  uint8_t OpSize = IROp->Size;
  if (Op->Class == FEXCore::IR::GPRClass) {
    switch (OpSize) {
    case 1:
      strb(GetReg<RA_32>(Op->Header.Args[0].ID()), MemOperand(STATE, Op->Offset));
    break;
    case 2:
      strh(GetReg<RA_32>(Op->Header.Args[0].ID()), MemOperand(STATE, Op->Offset));
    break;
    case 4:
      str(GetReg<RA_32>(Op->Header.Args[0].ID()), MemOperand(STATE, Op->Offset));
    break;
    case 8:
      str(GetReg<RA_64>(Op->Header.Args[0].ID()), MemOperand(STATE, Op->Offset));
    break;
    default:  LOGMAN_MSG_A_FMT("Unhandled StoreContext size: {}", OpSize);
    }
  }
  else {
    auto Src =  GetSrc(Op->Header.Args[0].ID());
    switch (OpSize) {
    case 1:
      str(Src.B(), MemOperand(STATE, Op->Offset));
    break;
    case 2:
      str(Src.H(), MemOperand(STATE, Op->Offset));
    break;
    case 4:
      str(Src.S(), MemOperand(STATE, Op->Offset));
    break;
    case 8:
      str(Src.D(), MemOperand(STATE, Op->Offset));
    break;
    case 16:
      str(Src, MemOperand(STATE, Op->Offset));
    break;
    default:  LOGMAN_MSG_A_FMT("Unhandled StoreContext size: {}", OpSize);
    }
  }
}


DEF_OP(LoadRegister) {
  auto Op = IROp->C<IR::IROp_LoadRegister>();

  if (Op->Class == IR::GPRClass) {
    auto regId = (Op->Offset - offsetof(FEXCore::Core::CpuStateFrame, State.gregs[0])) / 8;
    auto regOffs = Op->Offset & 7;

    LOGMAN_THROW_A_FMT(regId < SRA64.size(), "out of range regId");

    auto reg = SRA64[regId];

    switch(Op->Header.Size) {
      case 1:
        LOGMAN_THROW_A_FMT(regOffs == 0 || regOffs == 1, "unexpected regOffs");
        ubfx(GetReg<RA_64>(Node), reg, regOffs * 8, 8);
        break;

      case 2:
        LOGMAN_THROW_A_FMT(regOffs == 0, "unexpected regOffs");
        ubfx(GetReg<RA_64>(Node), reg, 0, 16);
        break;

      case 4:
        LOGMAN_THROW_A_FMT(regOffs == 0, "unexpected regOffs");
        if (GetReg<RA_64>(Node).GetCode() != reg.GetCode())
          mov(GetReg<RA_32>(Node), reg.W());
        break;

      case 8:
        LOGMAN_THROW_A_FMT(regOffs == 0, "unexpected regOffs");
        if (GetReg<RA_64>(Node).GetCode() != reg.GetCode())
          mov(GetReg<RA_64>(Node), reg);
        break;
    }
  } else if (Op->Class == IR::FPRClass) {
    auto regId = (Op->Offset - offsetof(FEXCore::Core::CpuStateFrame, State.xmm[0][0])) / 16;
    auto regOffs = Op->Offset & 15;

    LOGMAN_THROW_A_FMT(regId < SRAFPR.size(), "out of range regId");

    auto guest = SRAFPR[regId];
    auto host = GetSrc(Node);

    switch(Op->Header.Size) {
      case 1:
        LOGMAN_THROW_A_FMT(regOffs == 0, "unexpected regOffs");
        mov(host.B(), guest.B());
        break;

      case 2:
        LOGMAN_THROW_A_FMT(regOffs == 0, "unexpected regOffs");
        fmov(host.H(), guest.H());
        break;

      case 4:
        LOGMAN_THROW_A_FMT((regOffs & 3) == 0, "unexpected regOffs");
        if (regOffs == 0) {
          if (host.GetCode() != guest.GetCode())
            fmov(host.S(), guest.S());
        } else {
          ins(host.V4S(), 0, guest.V4S(), regOffs/4);
        }
        break;

      case 8:
        LOGMAN_THROW_A_FMT((regOffs & 7) == 0, "unexpected regOffs");
        if (regOffs == 0) {
          if (host.GetCode() != guest.GetCode())
            mov(host.D(), guest.D());
        } else {
          ins(host.V2D(), 0, guest.V2D(), regOffs/8);
        }
        break;

      case 16:
        LOGMAN_THROW_A_FMT(regOffs == 0, "unexpected regOffs");
        if (host.GetCode() != guest.GetCode())
          mov(host.Q(), guest.Q());
        break;
    }
  } else {
    LOGMAN_THROW_A_FMT(false, "Unhandled Op->Class {}", Op->Class);
  }
}

DEF_OP(StoreRegister) {
  auto Op = IROp->C<IR::IROp_StoreRegister>();

  if (Op->Class == IR::GPRClass) {
    auto regId = Op->Offset / 8 - 1;
    auto regOffs = Op->Offset & 7;

    LOGMAN_THROW_A_FMT(regId < SRA64.size(), "out of range regId");

    auto reg = SRA64[regId];

    switch(Op->Header.Size) {
      case 1:
        LOGMAN_THROW_A_FMT(regOffs == 0 || regOffs == 1, "unexpected regOffs");
        bfi(reg, GetReg<RA_64>(Op->Value.ID()), regOffs * 8, 8);
        break;

      case 2:
        LOGMAN_THROW_A_FMT(regOffs == 0, "unexpected regOffs");
        bfi(reg, GetReg<RA_64>(Op->Value.ID()), 0, 16);
        break;

      case 4:
        LOGMAN_THROW_A_FMT(regOffs == 0, "unexpected regOffs");
        bfi(reg, GetReg<RA_64>(Op->Value.ID()), 0, 32);
        break;

      case 8:
        LOGMAN_THROW_A_FMT(regOffs == 0, "unexpected regOffs");
        if (GetReg<RA_64>(Op->Value.ID()).GetCode() != reg.GetCode())
          mov(reg, GetReg<RA_64>(Op->Value.ID()));
        break;
    }
  } else if (Op->Class == IR::FPRClass) {
    auto regId = (Op->Offset - offsetof(FEXCore::Core::CpuStateFrame, State.xmm[0][0])) / 16;
    auto regOffs = Op->Offset & 15;

    LOGMAN_THROW_A_FMT(regId < SRAFPR.size(), "regId out of range");

    auto guest = SRAFPR[regId];
    auto host = GetSrc(Op->Value.ID());

    switch(Op->Header.Size) {
      case 1:
        ins(guest.V16B(), regOffs, host.V16B(), 0);
        break;

      case 2:
        LOGMAN_THROW_A_FMT((regOffs & 1) == 0, "unexpected regOffs");
        ins(guest.V8H(), regOffs/2, host.V8H(), 0);
        break;

      case 4:
        LOGMAN_THROW_A_FMT((regOffs & 3) == 0, "unexpected regOffs");
        ins(guest.V4S(), regOffs/4, host.V4S(), 0);
        break;

      case 8:
        LOGMAN_THROW_A_FMT((regOffs & 7) == 0, "unexpected regOffs");
        ins(guest.V2D(), regOffs / 8, host.V2D(), 0);
        break;

      case 16:
        LOGMAN_THROW_A_FMT(regOffs == 0, "unexpected regOffs");
        if (guest.GetCode() != host.GetCode())
          mov(guest.Q(), host.Q());
        break;
    }
  } else {
    LOGMAN_THROW_A_FMT(false, "Unhandled Op->Class {}", Op->Class);
  }
}


DEF_OP(LoadContextIndexed) {
  auto Op = IROp->C<IR::IROp_LoadContextIndexed>();
  size_t size = IROp->Size;
  auto index = GetReg<RA_64>(Op->Header.Args[0].ID());

  if (Op->Class == FEXCore::IR::GPRClass) {
    switch (Op->Stride) {
    case 1:
    case 2:
    case 4:
    case 8: {
      LoadConstant(TMP1, Op->Stride);
      mul(TMP1, index, TMP1);
      add(TMP1, STATE, TMP1);

      switch (size) {
      case 1:
        ldrb(GetReg<RA_32>(Node), MemOperand(TMP1, Op->BaseOffset));
        break;
      case 2:
        ldrh(GetReg<RA_32>(Node), MemOperand(TMP1, Op->BaseOffset));
        break;
      case 4:
        ldr(GetReg<RA_32>(Node), MemOperand(TMP1, Op->BaseOffset));
        break;
      case 8:
        ldr(GetReg<RA_64>(Node), MemOperand(TMP1, Op->BaseOffset));
        break;
      default:
        LOGMAN_MSG_A_FMT("Unhandled LoadContextIndexed size: {}", IROp->Size);
        break;
      }
      break;
    }
    case 16:
      LOGMAN_MSG_A_FMT("Invalid Class load of size 16");
      break;
    default:
      LOGMAN_MSG_A_FMT("Unhandled LoadContextIndexed stride: {}", Op->Stride);
      break;
    }
  }
  else {
    switch (Op->Stride) {
    case 1:
    case 2:
    case 4:
    case 8:
    case 16: {
      LoadConstant(TMP1, Op->Stride);
      mul(TMP1, index, TMP1);
      add(TMP1, STATE, TMP1);

      switch (size) {
      case 1:
        ldr(GetDst(Node).B(), MemOperand(TMP1, Op->BaseOffset));
        break;
      case 2:
        ldr(GetDst(Node).H(), MemOperand(TMP1, Op->BaseOffset));
        break;
      case 4:
        ldr(GetDst(Node).S(), MemOperand(TMP1, Op->BaseOffset));
        break;
      case 8:
        ldr(GetDst(Node).D(), MemOperand(TMP1, Op->BaseOffset));
        break;
      case 16:
        if (Op->BaseOffset % 16 == 0) {
          ldr(GetDst(Node), MemOperand(TMP1, Op->BaseOffset));
        }
        else {
          add(TMP1, TMP1, Op->BaseOffset);
          ldur(GetDst(Node), MemOperand(TMP1, Op->BaseOffset));
        }
        break;
      default:
        LOGMAN_MSG_A_FMT("Unhandled LoadContextIndexed size: {}", IROp->Size);
        break;
      }
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unhandled LoadContextIndexed stride: {}", Op->Stride);
      break;
    }
  }
}

DEF_OP(StoreContextIndexed) {
  auto Op = IROp->C<IR::IROp_StoreContextIndexed>();
  size_t size = IROp->Size;
  auto index = GetReg<RA_64>(Op->Header.Args[1].ID());

  if (Op->Class == FEXCore::IR::GPRClass) {
    auto value = GetReg<RA_64>(Op->Header.Args[0].ID());

    switch (Op->Stride) {
    case 1:
    case 2:
    case 4:
    case 8: {
      LoadConstant(TMP1, Op->Stride);
      mul(TMP1, index, TMP1);
      add(TMP1, STATE, TMP1);

      switch (size) {
      case 1:
        strb(value, MemOperand(TMP1, Op->BaseOffset));
        break;
      case 2:
        strh(value, MemOperand(TMP1, Op->BaseOffset));
        break;
      case 4:
        str(value.W(), MemOperand(TMP1, Op->BaseOffset));
        break;
      case 8:
        str(value, MemOperand(TMP1, Op->BaseOffset));
        break;
      default:
        LOGMAN_MSG_A_FMT("Unhandled StoreContextIndexed size: {}", IROp->Size);
        break;
      }
      break;
    }
    case 16:
      LOGMAN_MSG_A_FMT("Invalid Class store of size 16");
      break;
    default:
      LOGMAN_MSG_A_FMT("Unhandled StoreContextIndexed stride: {}", Op->Stride);
      break;
    }
  }
  else {
    auto value = GetSrc(Op->Header.Args[0].ID());

    switch (Op->Stride) {
    case 1:
    case 2:
    case 4:
    case 8:
    case 16: {
      LoadConstant(TMP1, Op->Stride);
      mul(TMP1, index, TMP1);
      add(TMP1, STATE, TMP1);

      switch (size) {
      case 1:
        str(value.B(), MemOperand(TMP1, Op->BaseOffset));
        break;
      case 2:
        str(value.H(), MemOperand(TMP1, Op->BaseOffset));
        break;
      case 4:
        str(value.S(), MemOperand(TMP1, Op->BaseOffset));
        break;
      case 8:
        str(value.D(), MemOperand(TMP1, Op->BaseOffset));
        break;
      case 16:
        if (Op->BaseOffset % 16 == 0) {
          str(value, MemOperand(TMP1, Op->BaseOffset));
        }
        else {
          add(TMP1, TMP1, Op->BaseOffset);
          stur(value, MemOperand(TMP1, Op->BaseOffset));
        }
        break;
      default:
        LOGMAN_MSG_A_FMT("Unhandled StoreContextIndexed size: {}", IROp->Size);
        break;
      }
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unhandled StoreContextIndexed stride: {}", Op->Stride);
      break;
    }
  }
}

DEF_OP(SpillRegister) {
  auto Op = IROp->C<IR::IROp_SpillRegister>();
  uint8_t OpSize = IROp->Size;
  uint32_t SlotOffset = Op->Slot * 16;

  if (Op->Class == FEXCore::IR::GPRClass) {
    switch (OpSize) {
    case 1: {
      strb(GetReg<RA_64>(Op->Header.Args[0].ID()), MemOperand(sp, SlotOffset));
      break;
    }
    case 2: {
      strh(GetReg<RA_64>(Op->Header.Args[0].ID()), MemOperand(sp, SlotOffset));
      break;
    }
    case 4: {
      str(GetReg<RA_32>(Op->Header.Args[0].ID()), MemOperand(sp, SlotOffset));
      break;
    }
    case 8: {
      str(GetReg<RA_64>(Op->Header.Args[0].ID()), MemOperand(sp, SlotOffset));
      break;
    }
    default:  LOGMAN_MSG_A_FMT("Unhandled SpillRegister size: {}", OpSize);
    }
  } else if (Op->Class == FEXCore::IR::FPRClass) {
    switch (OpSize) {
    case 4: {
      str(GetSrc(Op->Header.Args[0].ID()).S(), MemOperand(sp, SlotOffset));
      break;
    }
    case 8: {
      str(GetSrc(Op->Header.Args[0].ID()).D(), MemOperand(sp, SlotOffset));
      break;
    }
    case 16: {
      str(GetSrc(Op->Header.Args[0].ID()), MemOperand(sp, SlotOffset));
      break;
    }
    default:  LOGMAN_MSG_A_FMT("Unhandled SpillRegister size: {}", OpSize);
    }
  } else {
    LOGMAN_MSG_A_FMT("Unhandled SpillRegister class: {}", Op->Class.Val);
  }
}

DEF_OP(FillRegister) {
  auto Op = IROp->C<IR::IROp_FillRegister>();
  uint8_t OpSize = IROp->Size;
  uint32_t SlotOffset = Op->Slot * 16;

  if (Op->Class == FEXCore::IR::GPRClass) {
    switch (OpSize) {
    case 1: {
      ldrb(GetReg<RA_64>(Node), MemOperand(sp, SlotOffset));
      break;
    }
    case 2: {
      ldrh(GetReg<RA_64>(Node), MemOperand(sp, SlotOffset));
      break;
    }
    case 4: {
      ldr(GetReg<RA_32>(Node), MemOperand(sp, SlotOffset));
      break;
    }
    case 8: {
      ldr(GetReg<RA_64>(Node), MemOperand(sp, SlotOffset));
      break;
    }
    default:  LOGMAN_MSG_A_FMT("Unhandled FillRegister size: {}", OpSize);
    }
  } else if (Op->Class == FEXCore::IR::FPRClass) {
    switch (OpSize) {
    case 4: {
      ldr(GetDst(Node).S(), MemOperand(sp, SlotOffset));
      break;
    }
    case 8: {
      ldr(GetDst(Node).D(), MemOperand(sp, SlotOffset));
      break;
    }
    case 16: {
      ldr(GetDst(Node), MemOperand(sp, SlotOffset));
      break;
    }
    default:  LOGMAN_MSG_A_FMT("Unhandled FillRegister size: {}", OpSize);
    }
  } else {
    LOGMAN_MSG_A_FMT("Unhandled FillRegister class: {}", Op->Class.Val);
  }
}

DEF_OP(LoadFlag) {
  auto Op = IROp->C<IR::IROp_LoadFlag>();
  auto Dst = GetReg<RA_64>(Node);
  ldrb(Dst, MemOperand(STATE, offsetof(FEXCore::Core::CPUState, flags[0]) + Op->Flag));
}

DEF_OP(StoreFlag) {
  auto Op = IROp->C<IR::IROp_StoreFlag>();
  strb(GetReg<RA_64>(Op->Header.Args[0].ID()), MemOperand(STATE, offsetof(FEXCore::Core::CPUState, flags[0]) + Op->Flag));
}

MemOperand Arm64JITCore::GenerateMemOperand(uint8_t AccessSize, aarch64::Register Base, IR::OrderedNodeWrapper Offset, IR::MemOffsetType OffsetType, uint8_t OffsetScale) {
  if (Offset.IsInvalid()) {
    return MemOperand(Base);
  } else {
    if (OffsetScale != 1 && OffsetScale != AccessSize) {
        LOGMAN_MSG_A_FMT("Unhandled GenerateMemOperand OffsetScale: {}", OffsetScale);
    }
    uint64_t Const;
    if (IsInlineConstant(Offset, &Const)) {
        return MemOperand(Base, Const);
    } else {
      auto RegOffset = GetReg<RA_64>(Offset.ID());
      switch(OffsetType.Val) {
        case IR::MEM_OFFSET_SXTX.Val: return MemOperand(Base, RegOffset, Extend::SXTX, (int)std::log2(OffsetScale) );
        case IR::MEM_OFFSET_UXTW.Val: return MemOperand(Base, RegOffset.W(), Extend::UXTW, (int)std::log2(OffsetScale) );
        case IR::MEM_OFFSET_SXTW.Val: return MemOperand(Base, RegOffset.W(), Extend::SXTW, (int)std::log2(OffsetScale) );

        default: LOGMAN_MSG_A_FMT("Unhandled GenerateMemOperand OffsetType: {}", OffsetType.Val); break;
      }
    }
  }

  FEX_UNREACHABLE;
}

DEF_OP(LoadMem) {
  auto Op = IROp->C<IR::IROp_LoadMem>();

  auto MemReg = GetReg<RA_64>(Op->Header.Args[0].ID());
  auto MemSrc = GenerateMemOperand(IROp->Size, MemReg, Op->Offset, Op->OffsetType, Op->OffsetScale);

  if (Op->Class == FEXCore::IR::GPRClass) {
    auto Dst = GetReg<RA_64>(Node);
    switch (IROp->Size) {
      case 1:
        ldrb(Dst, MemSrc);
        break;
      case 2:
        ldrh(Dst, MemSrc);
        break;
      case 4:
        ldr(Dst.W(), MemSrc);
        break;
      case 8:
        ldr(Dst, MemSrc);
        break;
      default:  LOGMAN_MSG_A_FMT("Unhandled LoadMem size: {}", IROp->Size);
    }
  }
  else {
    auto Dst = GetDst(Node);
    switch (IROp->Size) {
      case 1:
        ldr(Dst.B(), MemSrc);
        break;
      case 2:
        ldr(Dst.H(), MemSrc);
        break;
      case 4:
        ldr(Dst.S(), MemSrc);
        break;
      case 8:
        ldr(Dst.D(), MemSrc);
        break;
      case 16:
        ldr(Dst, MemSrc);
        break;
      default:  LOGMAN_MSG_A_FMT("Unhandled LoadMem size: {}", IROp->Size);
    }
  }
}

DEF_OP(LoadMemTSO) {
  auto Op = IROp->C<IR::IROp_LoadMemTSO>();

  auto MemReg = GetReg<RA_64>(Op->Addr.ID());
  auto MemSrc = GenerateMemOperand(IROp->Size, MemReg, Op->Offset, Op->OffsetType, Op->OffsetScale);

  if (CTX->HostFeatures.SupportsTSOImm9) {
    // RCPC2 means that the offset must be an inline constant
    LOGMAN_THROW_A_FMT(MemSrc.IsRegisterOffset() == false, "RCPC2 doesn't support register offset. Only Immediate offset");
  }
  else {
    LOGMAN_THROW_A_FMT(Op->Offset.IsInvalid(), "LoadMemTSO: No offset allowed");
  }

  if (CTX->HostFeatures.SupportsTSOImm9 && Op->Class == FEXCore::IR::GPRClass) {
    if (IROp->Size == 1) {
      // 8bit load is always aligned to natural alignment
      auto Dst = GetReg<RA_64>(Node);
      ldapurb(Dst, MemSrc);
    }
    else {
      // Aligned
      auto Dst = GetReg<RA_64>(Node);
      switch (IROp->Size) {
        case 2:
          ldapurh(Dst, MemSrc);
          break;
        case 4:
          ldapur(Dst.W(), MemSrc);
          break;
        case 8:
          ldapur(Dst, MemSrc);
          break;
        default:  LOGMAN_MSG_A_FMT("Unhandled LoadMemTSO size: {}", IROp->Size);
      }
      nop();
    }
  }
  else if (CTX->HostFeatures.SupportsRCPC && Op->Class == FEXCore::IR::GPRClass) {
    if (IROp->Size == 1) {
      // 8bit load is always aligned to natural alignment
      auto Dst = GetReg<RA_64>(Node);
      ldaprb(Dst, MemSrc);
    }
    else {
      // Aligned
      auto Dst = GetReg<RA_64>(Node);
      switch (IROp->Size) {
        case 2:
          ldaprh(Dst, MemSrc);
          break;
        case 4:
          ldapr(Dst.W(), MemSrc);
          break;
        case 8:
          ldapr(Dst, MemSrc);
          break;
        default:  LOGMAN_MSG_A_FMT("Unhandled LoadMemTSO size: {}", IROp->Size);
      }
      nop();
    }
  }
  else if (Op->Class == FEXCore::IR::GPRClass) {
    if (IROp->Size == 1) {
      // 8bit load is always aligned to natural alignment
      auto Dst = GetReg<RA_64>(Node);
      ldarb(Dst, MemSrc);
    }
    else {
      // Aligned
      auto Dst = GetReg<RA_64>(Node);
      switch (IROp->Size) {
        case 2:
          ldarh(Dst, MemSrc);
          break;
        case 4:
          ldar(Dst.W(), MemSrc);
          break;
        case 8:
          ldar(Dst, MemSrc);
          break;
        default:  LOGMAN_MSG_A_FMT("Unhandled LoadMemTSO size: {}", IROp->Size);
      }
      nop();
    }
  }
  else {
    dmb(InnerShareable, BarrierWrites);
    auto Dst = GetDst(Node);
    switch (IROp->Size) {
      case 2:
        ldr(Dst.H(), MemSrc);
        break;
      case 4:
        ldr(Dst.S(), MemSrc);
        break;
      case 8:
        ldr(Dst.D(), MemSrc);
        break;
      case 16:
        ldr(Dst, MemSrc);
        break;
      default:  LOGMAN_MSG_A_FMT("Unhandled LoadMemTSO size: {}", IROp->Size);
    }
  }
}

DEF_OP(StoreMem) {
  auto Op = IROp->C<IR::IROp_StoreMem>();

  auto MemReg = GetReg<RA_64>(Op->Addr.ID());

  auto MemSrc = GenerateMemOperand(IROp->Size, MemReg, Op->Offset, Op->OffsetType, Op->OffsetScale);

  if (Op->Class == FEXCore::IR::GPRClass) {
    switch (IROp->Size) {
      case 1:
        strb(GetReg<RA_64>(Op->Value.ID()), MemSrc);
        break;
      case 2:
        strh(GetReg<RA_64>(Op->Value.ID()), MemSrc);
        break;
      case 4:
        str(GetReg<RA_32>(Op->Value.ID()), MemSrc);
        break;
      case 8:
        str(GetReg<RA_64>(Op->Value.ID()), MemSrc);
        break;
      default:  LOGMAN_MSG_A_FMT("Unhandled StoreMem size: {}", IROp->Size);
    }
  }
  else {
    auto Src = GetSrc(Op->Value.ID());
    switch (IROp->Size) {
      case 1:
        str(Src.B(), MemSrc);
        break;
      case 2:
        str(Src.H(), MemSrc);
        break;
      case 4:
        str(Src.S(), MemSrc);
        break;
      case 8:
        str(Src.D(), MemSrc);
        break;
      case 16:
        str(Src, MemSrc);
        break;
      default:  LOGMAN_MSG_A_FMT("Unhandled StoreMem size: {}", IROp->Size);
    }
  }
}

DEF_OP(StoreMemTSO) {
  auto Op = IROp->C<IR::IROp_StoreMemTSO>();

  auto MemReg = GetReg<RA_64>(Op->Addr.ID());
  auto MemSrc = GenerateMemOperand(IROp->Size, MemReg, Op->Offset, Op->OffsetType, Op->OffsetScale);

  if (CTX->HostFeatures.SupportsTSOImm9) {
    // RCPC2 means that the offset must be an inline constant
    LOGMAN_THROW_A_FMT(MemSrc.IsRegisterOffset() == false, "RCPC2 doesn't support register offset. Only Immediate offset");
  }
  else {
    LOGMAN_THROW_A_FMT(Op->Offset.IsInvalid(), "StoreMemTSO: No offset allowed");
  }

  if (CTX->HostFeatures.SupportsTSOImm9 && Op->Class == FEXCore::IR::GPRClass) {
    if (IROp->Size == 1) {
      // 8bit load is always aligned to natural alignment
      stlurb(GetReg<RA_64>(Op->Value.ID()), MemSrc);
    }
    else {
      nop();
      switch (IROp->Size) {
        case 2:
          stlurh(GetReg<RA_64>(Op->Value.ID()), MemSrc);
          break;
        case 4:
          stlur(GetReg<RA_32>(Op->Value.ID()), MemSrc);
          break;
        case 8:
          stlur(GetReg<RA_64>(Op->Value.ID()), MemSrc);
          break;
        default:  LOGMAN_MSG_A_FMT("Unhandled StoreMemTSO size: {}", IROp->Size);
      }
    }
  }
  else if (Op->Class == FEXCore::IR::GPRClass) {
    if (IROp->Size == 1) {
      // 8bit load is always aligned to natural alignment
      stlrb(GetReg<RA_64>(Op->Value.ID()), MemSrc);
    }
    else {
      nop();
      switch (IROp->Size) {
        case 2:
          stlrh(GetReg<RA_64>(Op->Value.ID()), MemSrc);
          break;
        case 4:
          stlr(GetReg<RA_32>(Op->Value.ID()), MemSrc);
          break;
        case 8:
          stlr(GetReg<RA_64>(Op->Value.ID()), MemSrc);
          break;
        default:  LOGMAN_MSG_A_FMT("Unhandled StoreMemTSO size: {}", IROp->Size);
      }
    }
  }
  else {
    dmb(InnerShareable, BarrierReads);
    auto Src = GetSrc(Op->Value.ID());
    switch (IROp->Size) {
      case 1:
        str(Src.B(), MemSrc);
        break;
      case 2:
        str(Src.H(), MemSrc);
        break;
      case 4:
        str(Src.S(), MemSrc);
        break;
      case 8:
        str(Src.D(), MemSrc);
        break;
      case 16:
        str(Src, MemSrc);
        break;
      default:  LOGMAN_MSG_A_FMT("Unhandled StoreMemTSO size: {}", IROp->Size);
    }
  }
}

DEF_OP(ParanoidLoadMemTSO) {
  auto Op = IROp->C<IR::IROp_LoadMemTSO>();

  auto MemSrc = MemOperand(GetReg<RA_64>(Op->Addr.ID()));

  if (!Op->Offset.IsInvalid()) {
    LOGMAN_MSG_A_FMT("ParanoidLoadMemTSO: No offset allowed");
  }

  if (Op->Class == FEXCore::IR::GPRClass) {
    if (IROp->Size == 1) {
      // 8bit load is always aligned to natural alignment
      auto Dst = GetReg<RA_64>(Node);
      ldarb(Dst, MemSrc);
    }
    else {
      auto Dst = GetReg<RA_64>(Node);
      switch (IROp->Size) {
        case 2:
          ldarh(Dst, MemSrc);
          break;
        case 4:
          ldar(Dst.W(), MemSrc);
          break;
        case 8:
          ldar(Dst, MemSrc);
          break;
        default:  LOGMAN_MSG_A_FMT("Unhandled ParanoidLoadMemTSO size: {}", IROp->Size);
      }
    }
  }
  else {
    auto Dst = GetDst(Node);
    switch (IROp->Size) {
      case 2:
        ldarh(TMP1.W(), MemSrc);
        fmov(Dst.H(), TMP1.W());
        break;
      case 4:
        ldar(TMP1.W(), MemSrc);
        fmov(Dst.S(), TMP1.W());
        break;
      case 8:
        ldar(TMP1, MemSrc);
        fmov(Dst.D(), TMP1);
        break;
      case 16:
        nop();
        ldaxp(TMP1, TMP2, MemSrc);
        clrex();
        mov(Dst.V2D(), 0, TMP1);
        mov(Dst.V2D(), 1, TMP2);
        break;
      default:  LOGMAN_MSG_A_FMT("Unhandled ParanoidLoadMemTSO size: {}", IROp->Size);
    }
  }
}

DEF_OP(ParanoidStoreMemTSO) {
  auto Op = IROp->C<IR::IROp_StoreMemTSO>();
  auto MemSrc = MemOperand(GetReg<RA_64>(Op->Addr.ID()));

  if (!Op->Offset.IsInvalid()) {
    LOGMAN_MSG_A_FMT("ParanoidStoreMemTSO: No offset allowed");
  }

  if (Op->Class == FEXCore::IR::GPRClass) {
    if (IROp->Size == 1) {
      // 8bit load is always aligned to natural alignment
      stlrb(GetReg<RA_64>(Op->Value.ID()), MemSrc);
    }
    else {
      switch (IROp->Size) {
        case 2:
          stlrh(GetReg<RA_64>(Op->Value.ID()), MemSrc);
          break;
        case 4:
          stlr(GetReg<RA_32>(Op->Value.ID()), MemSrc);
          break;
        case 8:
          stlr(GetReg<RA_64>(Op->Value.ID()), MemSrc);
          break;
        default:  LOGMAN_MSG_A_FMT("Unhandled ParanoidStoreMemTSO size: {}", IROp->Size);
      }
    }
  }
  else {
    auto Src = GetSrc(Op->Value.ID());
    if (IROp->Size == 1) {
      // 8bit load is always aligned to natural alignment
      mov(TMP1.W(), Src.V16B(), 0);
      stlrb(TMP1, MemSrc);
    }
    else {
      switch (IROp->Size) {
        case 2:
          mov(TMP1.W(), Src.V8H(), 0);
          stlrh(TMP1, MemSrc);
          break;
        case 4:
          mov(TMP1.W(), Src.V4S(), 0);
          stlr(TMP1.W(), MemSrc);
          break;
        case 8:
          mov(TMP1, Src.V2D(), 0);
          stlr(TMP1, MemSrc);
          break;
        case 16: {
          // Move vector to GPRs
          mov(TMP1, Src.V2D(), 0);
          mov(TMP2, Src.V2D(), 1);
          Label B;
          bind(&B);

          // ldaxp must not have both the destination registers be the same
          ldaxp(xzr, TMP3, MemSrc); // <- Can hit SIGBUS. Overwritten with DMB
          stlxp(TMP3, TMP1, TMP2, MemSrc); // <- Can also hit SIGBUS
          cbnz(TMP3, &B); // < Overwritten with NOP
          break;
        }
        default:  LOGMAN_MSG_A_FMT("Unhandled ParanoidStoreMemTSO size: {}", IROp->Size);
      }
    }
  }
}

DEF_OP(VLoadMemElement) {
  LOGMAN_MSG_A_FMT("Unimplemented");
}

DEF_OP(VStoreMemElement) {
  LOGMAN_MSG_A_FMT("Unimplemented");
}

DEF_OP(CacheLineClear) {
  auto Op = IROp->C<IR::IROp_CacheLineClear>();

  auto MemReg = GetReg<RA_64>(Op->Header.Args[0].ID());

  // Clear dcache only
  // icache doesn't matter here since the guest application shouldn't be calling clflush on JIT code.
  mov(TMP1, MemReg);
  for (size_t i = 0; i < std::max(1U, CTX->HostFeatures.DCacheLineSize / 64U); ++i) {
    dc(DataCacheOp::CVAU, TMP1);
    add(TMP1, TMP1, CTX->HostFeatures.DCacheLineSize);
  }
  dsb(InnerShareable, BarrierAll);
}

DEF_OP(CacheLineZero) {
  auto Op = IROp->C<IR::IROp_CacheLineZero>();

  auto MemReg = GetReg<RA_64>(Op->Header.Args[0].ID());

  if (CTX->HostFeatures.SupportsCLZERO) {
    // We can use this instruction directly
    dc(DataCacheOp::ZVA, MemReg);
  }
  else {
    // We must walk the cacheline ourselves
    // Force cacheline alignment
    and_(TMP1, MemReg, ~(CPUIDEmu::CACHELINE_SIZE - 1));
    // This will end up being four STPs
    // Depending on uarch it could be slightly more efficient in instructions emitted
    // and uops to use vector pair STP, but we want the non-temporal bit specifically here
    for (size_t i = 0; i < CPUIDEmu::CACHELINE_SIZE; i += 16) {
      stnp(xzr, xzr, MemOperand(TMP1, i, Offset));
    }
  }
}

#undef DEF_OP
void Arm64JITCore::RegisterMemoryHandlers() {
#define REGISTER_OP(op, x) OpHandlers[FEXCore::IR::IROps::OP_##op] = &Arm64JITCore::Op_##x
  REGISTER_OP(LOADCONTEXT,         LoadContext);
  REGISTER_OP(STORECONTEXT,        StoreContext);
  REGISTER_OP(LOADREGISTER,        LoadRegister);
  REGISTER_OP(STOREREGISTER,       StoreRegister);
  REGISTER_OP(LOADCONTEXTINDEXED,  LoadContextIndexed);
  REGISTER_OP(STORECONTEXTINDEXED, StoreContextIndexed);
  REGISTER_OP(SPILLREGISTER,       SpillRegister);
  REGISTER_OP(FILLREGISTER,        FillRegister);
  REGISTER_OP(LOADFLAG,            LoadFlag);
  REGISTER_OP(STOREFLAG,           StoreFlag);
  REGISTER_OP(LOADMEM,             LoadMem);
  REGISTER_OP(STOREMEM,            StoreMem);
  if (ParanoidTSO()) {
    REGISTER_OP(LOADMEMTSO,          ParanoidLoadMemTSO);
    REGISTER_OP(STOREMEMTSO,         ParanoidStoreMemTSO);
  }
  else {
    REGISTER_OP(LOADMEMTSO,          LoadMemTSO);
    REGISTER_OP(STOREMEMTSO,         StoreMemTSO);
  }
  REGISTER_OP(VLOADMEMELEMENT,     VLoadMemElement);
  REGISTER_OP(VSTOREMEMELEMENT,    VStoreMemElement);
  REGISTER_OP(CACHELINECLEAR,      CacheLineClear);
  REGISTER_OP(CACHELINEZERO,       CacheLineZero);
#undef REGISTER_OP
}
}

