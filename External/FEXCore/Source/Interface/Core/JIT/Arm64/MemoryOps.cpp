/*
$info$
tags: backend|arm64
$end_info$
*/

#include "Interface/Core/JIT/Arm64/JITClass.h"

namespace FEXCore::CPU {

using namespace vixl;
using namespace vixl::aarch64;
#define DEF_OP(x) void Arm64JITCore::Op_##x(FEXCore::IR::IROp_Header *IROp, uint32_t Node)

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
    default:  LogMan::Msg::A("Unhandled LoadContext size: %d", OpSize);
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
    default:  LogMan::Msg::A("Unhandled LoadContext size: %d", OpSize);
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
    default:  LogMan::Msg::A("Unhandled StoreContext size: %d", OpSize);
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
    default:  LogMan::Msg::A("Unhandled LoadContext size: %d", OpSize);
    }
  }
}


DEF_OP(LoadRegister) {
  auto Op = IROp->C<IR::IROp_LoadRegister>();

  if (Op->Class == IR::GPRClass) {
    auto regId = (Op->Offset - offsetof(FEXCore::Core::CpuStateFrame, State.gregs[0])) / 8;
    auto regOffs = Op->Offset & 7;

    LogMan::Throw::A(regId < SRA64.size(), "out of range regId");

    auto reg = SRA64[regId];

    switch(Op->Header.Size) {
      case 1:
        LogMan::Throw::A(regOffs == 0 || regOffs == 1, "unexpected regOffs");
        ubfx(GetReg<RA_64>(Node), reg, regOffs * 8, 8);
        break;

      case 2:
        LogMan::Throw::A(regOffs == 0, "unexpected regOffs");
        ubfx(GetReg<RA_64>(Node), reg, 0, 16);
        break;

      case 4:
        LogMan::Throw::A(regOffs == 0, "unexpected regOffs");
        if (GetReg<RA_64>(Node).GetCode() != reg.GetCode())
          mov(GetReg<RA_32>(Node), reg.W());
        break;

      case 8:
        LogMan::Throw::A(regOffs == 0, "unexpected regOffs");
        if (GetReg<RA_64>(Node).GetCode() != reg.GetCode())
          mov(GetReg<RA_64>(Node), reg);
        break;
    }
  } else if (Op->Class == IR::FPRClass) {
    auto regId = (Op->Offset - offsetof(FEXCore::Core::CpuStateFrame, State.xmm[0][0])) / 16;
    auto regOffs = Op->Offset & 15;

    LogMan::Throw::A(regId < SRAFPR.size(), "out of range regId");

    auto guest = SRAFPR[regId];
    auto host = GetSrc(Node);

    switch(Op->Header.Size) {
      case 1:
        LogMan::Throw::A(regOffs == 0, "unexpected regOffs");
        mov(host.B(), guest.B());
        break;

      case 2:
        LogMan::Throw::A(regOffs == 0, "unexpected regOffs");
        fmov(host.H(), guest.H());
        break;

      case 4:
        LogMan::Throw::A((regOffs & 3) == 0, "unexpected regOffs");
        if (regOffs == 0) {
          if (host.GetCode() != guest.GetCode())
            fmov(host.S(), guest.S());
        } else {
          ins(host.V4S(), 0, guest.V4S(), regOffs/4);
        }
        break;

      case 8:
        LogMan::Throw::A((regOffs & 7) == 0, "unexpected regOffs");
        if (regOffs == 0) {
          if (host.GetCode() != guest.GetCode())
            mov(host.D(), guest.D());
        } else {
          ins(host.V2D(), 0, guest.V2D(), regOffs/8);
        }
        break;

      case 16:
        LogMan::Throw::A(regOffs == 0, "unexpected regOffs");
        if (host.GetCode() != guest.GetCode())
          mov(host.Q(), guest.Q());
        break;
    }
  } else {
    LogMan::Throw::A(false, "Unhandled Op->Class %d", Op->Class);
  }
}

DEF_OP(StoreRegister) {
  auto Op = IROp->C<IR::IROp_StoreRegister>();

  if (Op->Class == IR::GPRClass) {
    auto regId = Op->Offset / 8 - 1;
    auto regOffs = Op->Offset & 7;

    LogMan::Throw::A(regId < SRA64.size(), "out of range regId");

    auto reg = SRA64[regId];

    switch(Op->Header.Size) {
      case 1:
        LogMan::Throw::A(regOffs == 0 || regOffs == 1, "unexpected regOffs");
        bfi(reg, GetReg<RA_64>(Op->Value.ID()), regOffs * 8, 8);
        break;

      case 2:
        LogMan::Throw::A(regOffs == 0, "unexpected regOffs");
        bfi(reg, GetReg<RA_64>(Op->Value.ID()), 0, 16);
        break;

      case 4:
        LogMan::Throw::A(regOffs == 0, "unexpected regOffs");
        bfi(reg, GetReg<RA_64>(Op->Value.ID()), 0, 32);
        break;

      case 8:
        LogMan::Throw::A(regOffs == 0, "unexpected regOffs");
        if (GetReg<RA_64>(Op->Value.ID()).GetCode() != reg.GetCode())
          mov(reg, GetReg<RA_64>(Op->Value.ID()));
        break;
    }
  } else if (Op->Class == IR::FPRClass) {
    auto regId = (Op->Offset - offsetof(FEXCore::Core::CpuStateFrame, State.xmm[0][0])) / 16;
    auto regOffs = Op->Offset & 15;

    LogMan::Throw::A(regId < SRAFPR.size(), "regId out of range");

    auto guest = SRAFPR[regId];
    auto host = GetSrc(Op->Value.ID());

    switch(Op->Header.Size) {
      case 1:
        ins(guest.V16B(), regOffs, host.V16B(), 0);
        break;

      case 2:
        LogMan::Throw::A((regOffs & 1) == 0, "unexpected regOffs");
        ins(guest.V8H(), regOffs/2, host.V8H(), 0);
        break;

      case 4:
        LogMan::Throw::A((regOffs & 3) == 0, "unexpected regOffs");
        ins(guest.V4S(), regOffs/4, host.V4S(), 0);
        break;

      case 8:
        LogMan::Throw::A((regOffs & 7) == 0, "unexpected regOffs");
        ins(guest.V2D(), regOffs / 8, host.V2D(), 0);
        break;

      case 16:
        LogMan::Throw::A(regOffs == 0, "unexpected regOffs");
        if (guest.GetCode() != host.GetCode())
          mov(guest.Q(), host.Q());
        break;
    }
  } else {
    LogMan::Throw::A(false, "Unhandled Op->Class %d", Op->Class);
  }
}


DEF_OP(LoadContextIndexed) {
  auto Op = IROp->C<IR::IROp_LoadContextIndexed>();
  size_t size = Op->Size;
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
        LogMan::Msg::A("Unhandled LoadContextIndexed size: %d", Op->Size);
      }
      break;
    }
    case 16:
      LogMan::Msg::A("Invalid Class load of size 16");
      break;
    default:
      LogMan::Msg::A("Unhandled LoadContextIndexed stride: %d", Op->Stride);
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
        LogMan::Msg::A("Unhandled LoadContextIndexed size: %d", Op->Size);
      }
      break;
    }
    default:
      LogMan::Msg::A("Unhandled LoadContextIndexed stride: %d", Op->Stride);
    }
  }
}

DEF_OP(StoreContextIndexed) {
  auto Op = IROp->C<IR::IROp_StoreContextIndexed>();
  size_t size = Op->Size;
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
        LogMan::Msg::A("Unhandled LoadContextIndexed size: %d", Op->Size);
      }
      break;
    }
    case 16:
      LogMan::Msg::A("Invalid Class load of size 16");
      break;
    default:
      LogMan::Msg::A("Unhandled LoadContextIndexed stride: %d", Op->Stride);
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
        LogMan::Msg::A("Unhandled LoadContextIndexed size: %d", Op->Size);
      }
      break;
    }
    default:
      LogMan::Msg::A("Unhandled LoadContextIndexed stride: %d", Op->Stride);
    }
  }
}

DEF_OP(SpillRegister) {
  auto Op = IROp->C<IR::IROp_SpillRegister>();
  uint8_t OpSize = IROp->Size;
  uint32_t SlotOffset = Op->Slot * 16 + 16;

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
    default:  LogMan::Msg::A("Unhandled SpillRegister size: %d", OpSize);
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
    default:  LogMan::Msg::A("Unhandled SpillRegister size: %d", OpSize);
    }
  } else {
    LogMan::Msg::A("Unhandled SpillRegister class: %d", Op->Class.Val);
  }
}

DEF_OP(FillRegister) {
  auto Op = IROp->C<IR::IROp_FillRegister>();
  uint8_t OpSize = IROp->Size;
  uint32_t SlotOffset = Op->Slot * 16 + 16;

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
    default:  LogMan::Msg::A("Unhandled SpillRegister size: %d", OpSize);
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
    default:  LogMan::Msg::A("Unhandled SpillRegister size: %d", OpSize);
    }
  } else {
    LogMan::Msg::A("Unhandled FillRegister class: %d", Op->Class.Val);
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
        LogMan::Msg::A("Unhandled GenerateMemOperand OffsetScale: %d", OffsetScale);
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

        default: LogMan::Msg::A("Unhandled GenerateMemOperand OffsetType: %d", OffsetType.Val); break;
      }
    }
  }
  __builtin_unreachable();
}

DEF_OP(LoadMem) {
  auto Op = IROp->C<IR::IROp_LoadMem>();

  auto MemReg = GetReg<RA_64>(Op->Header.Args[0].ID());
  auto MemSrc = GenerateMemOperand(Op->Size, MemReg, Op->Offset, Op->OffsetType, Op->OffsetScale);

  if (Op->Class == FEXCore::IR::GPRClass) {
    auto Dst = GetReg<RA_64>(Node);
    switch (Op->Size) {
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
      default:  LogMan::Msg::A("Unhandled LoadMem size: %d", Op->Size);
    }
  }
  else {
    auto Dst = GetDst(Node);
    switch (Op->Size) {
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
      default:  LogMan::Msg::A("Unhandled LoadMem size: %d", Op->Size);
    }
  }
}

DEF_OP(LoadMemTSO) {
  auto Op = IROp->C<IR::IROp_LoadMemTSO>();

  auto MemSrc = MemOperand(GetReg<RA_64>(Op->Header.Args[0].ID()));

  if (!Op->Offset.IsInvalid()) {
    LogMan::Msg::A("LoadMemTSO: No offset allowed");
  }

  if (SupportsRCPC && Op->Class == FEXCore::IR::GPRClass) {
    if (Op->Size == 1) {
      // 8bit load is always aligned to natural alignment
      auto Dst = GetReg<RA_64>(Node);
      ldaprb(Dst, MemSrc);
    }
    else {
      // Aligned
      auto Dst = GetReg<RA_64>(Node);
      nop();
      switch (Op->Size) {
        case 2:
          ldaprh(Dst, MemSrc);
          break;
        case 4:
          ldapr(Dst.W(), MemSrc);
          break;
        case 8:
          ldapr(Dst, MemSrc);
          break;
        default:  LogMan::Msg::A("Unhandled LoadMem size: %d", Op->Size);
      }
      nop();
    }
  }
  else if (Op->Class == FEXCore::IR::GPRClass) {
    if (Op->Size == 1) {
      // 8bit load is always aligned to natural alignment
      auto Dst = GetReg<RA_64>(Node);
      ldarb(Dst, MemSrc);
    }
    else {
      // Aligned
      auto Dst = GetReg<RA_64>(Node);
      nop();
      switch (Op->Size) {
        case 2:
          ldarh(Dst, MemSrc);
          break;
        case 4:
          ldar(Dst.W(), MemSrc);
          break;
        case 8:
          ldar(Dst, MemSrc);
          break;
        default:  LogMan::Msg::A("Unhandled LoadMem size: %d", Op->Size);
      }
      nop();
    }
  }
  else {
    dmb(InnerShareable, BarrierAll);
    auto Dst = GetDst(Node);
    switch (Op->Size) {
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
      default:  LogMan::Msg::A("Unhandled LoadMem size: %d", Op->Size);
    }
    dmb(InnerShareable, BarrierAll);
  }
}

DEF_OP(StoreMem) {
  auto Op = IROp->C<IR::IROp_StoreMem>();

  auto MemReg = GetReg<RA_64>(Op->Header.Args[0].ID());

  auto MemSrc = GenerateMemOperand(Op->Size, MemReg, Op->Offset, Op->OffsetType, Op->OffsetScale);

  if (Op->Class == FEXCore::IR::GPRClass) {
    switch (Op->Size) {
      case 1:
        strb(GetReg<RA_64>(Op->Header.Args[1].ID()), MemSrc);
        break;
      case 2:
        strh(GetReg<RA_64>(Op->Header.Args[1].ID()), MemSrc);
        break;
      case 4:
        str(GetReg<RA_32>(Op->Header.Args[1].ID()), MemSrc);
        break;
      case 8:
        str(GetReg<RA_64>(Op->Header.Args[1].ID()), MemSrc);
        break;
      default:  LogMan::Msg::A("Unhandled StoreMem size: %d", Op->Size);
    }
  }
  else {
    auto Src = GetSrc(Op->Header.Args[1].ID());
    switch (Op->Size) {
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
      default:  LogMan::Msg::A("Unhandled StoreMem size: %d", Op->Size);
    }
  }
}

DEF_OP(StoreMemTSO) {
  auto Op = IROp->C<IR::IROp_StoreMemTSO>();
  auto MemSrc = MemOperand(GetReg<RA_64>(Op->Header.Args[0].ID()));

  if (!Op->Offset.IsInvalid()) {
    LogMan::Msg::A("StoreMemTSO: No offset allowed");
  }

  if (Op->Class == FEXCore::IR::GPRClass) {
    if (Op->Size == 1) {
      // 8bit load is always aligned to natural alignment
      stlrb(GetReg<RA_64>(Op->Header.Args[1].ID()), MemSrc);
    }
    else {
      nop();
      switch (Op->Size) {
        case 2:
          stlrh(GetReg<RA_64>(Op->Header.Args[1].ID()), MemSrc);
          break;
        case 4:
          stlr(GetReg<RA_32>(Op->Header.Args[1].ID()), MemSrc);
          break;
        case 8:
          stlr(GetReg<RA_64>(Op->Header.Args[1].ID()), MemSrc);
          break;
        default:  LogMan::Msg::A("Unhandled StoreMem size: %d", Op->Size);
      }
      nop();
    }
  }
  else {
    dmb(InnerShareable, BarrierAll);
    auto Src = GetSrc(Op->Header.Args[1].ID());
    switch (Op->Size) {
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
      default:  LogMan::Msg::A("Unhandled StoreMem size: %d", Op->Size);
    }
    dmb(InnerShareable, BarrierAll);
  }
}

DEF_OP(VLoadMemElement) {
  LogMan::Msg::A("Unimplemented");
}

DEF_OP(VStoreMemElement) {
  LogMan::Msg::A("Unimplemented");
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
  REGISTER_OP(LOADMEMTSO,          LoadMemTSO);
  REGISTER_OP(STOREMEMTSO,         StoreMemTSO);
  REGISTER_OP(VLOADMEMELEMENT,     VLoadMemElement);
  REGISTER_OP(VSTOREMEMELEMENT,    VStoreMemElement);
#undef REGISTER_OP
}
}

