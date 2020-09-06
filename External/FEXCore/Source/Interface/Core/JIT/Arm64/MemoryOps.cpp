#include "Interface/Core/JIT/Arm64/JITClass.h"

namespace FEXCore::CPU {

using namespace vixl;
using namespace vixl::aarch64;
#define DEF_OP(x) void JITCore::Op_##x(FEXCore::IR::IROp_Header *IROp, uint32_t Node)
DEF_OP(LoadContextPair) {
  auto Op = IROp->C<IR::IROp_LoadContextPair>();
  switch (Op->Size) {
    case 4: {
      auto Dst = GetSrcPair<RA_32>(Node);
      ldp(Dst.first, Dst.second, MemOperand(STATE, Op->Offset));
      break;
    }
    case 8: {
      auto Dst = GetSrcPair<RA_64>(Node);
      ldp(Dst.first, Dst.second, MemOperand(STATE, Op->Offset));
      break;
    }
    default: LogMan::Msg::A("Unknown Size"); break;
  }
}

DEF_OP(StoreContextPair) {
  auto Op = IROp->C<IR::IROp_StoreContextPair>();
  switch (Op->Size) {
    case 4: {
      auto Src = GetSrcPair<RA_32>(Op->Header.Args[0].ID());
      stp(Src.first, Src.second, MemOperand(STATE, Op->Offset));
      break;
    }
    case 8: {
      auto Src = GetSrcPair<RA_64>(Op->Header.Args[0].ID());
      stp(Src.first, Src.second, MemOperand(STATE, Op->Offset));
      break;
    }
  }
}

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
  case 16: {
    str(GetSrc(Op->Header.Args[0].ID()), MemOperand(sp, SlotOffset));
    break;
  }
  default:  LogMan::Msg::A("Unhandled SpillRegister size: %d", OpSize);
  }
}

DEF_OP(FillRegister) {
  auto Op = IROp->C<IR::IROp_FillRegister>();
  uint8_t OpSize = IROp->Size;
  uint32_t SlotOffset = Op->Slot * 16 + 16;
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
  case 16: {
    ldr(GetDst(Node), MemOperand(sp, SlotOffset));
    break;
  }
  default:  LogMan::Msg::A("Unhandled SpillRegister size: %d", OpSize);
  }
}

DEF_OP(LoadFlag) {
  auto Op = IROp->C<IR::IROp_LoadFlag>();
  auto Dst = GetReg<RA_64>(Node);
  ldrb(Dst, MemOperand(STATE, offsetof(FEXCore::Core::CPUState, flags[0]) + Op->Flag));
  and_(Dst, Dst, 1);
}

DEF_OP(StoreFlag) {
  auto Op = IROp->C<IR::IROp_StoreFlag>();
  and_(TMP1, GetReg<RA_64>(Op->Header.Args[0].ID()), 1);
  strb(TMP1, MemOperand(STATE, offsetof(FEXCore::Core::CPUState, flags[0]) + Op->Flag));
}

DEF_OP(LoadMem) {
  auto Op = IROp->C<IR::IROp_LoadMem>();

  auto MemSrc = MemOperand(GetReg<RA_64>(Op->Header.Args[0].ID()));
  if (!CTX->Config.UnifiedMemory) {
    LoadConstant(TMP1, (uint64_t)CTX->MemoryMapper.GetMemoryBase());
    MemSrc = MemOperand(TMP1, GetReg<RA_64>(Op->Header.Args[0].ID()));
  }

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
  if (!CTX->Config.UnifiedMemory) {
    LoadConstant(TMP1, (uint64_t)CTX->MemoryMapper.GetMemoryBase());
    add(TMP1, TMP1, GetReg<RA_64>(Op->Header.Args[0].ID()));
    MemSrc = MemOperand(TMP1);
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
  auto MemSrc = MemOperand(GetReg<RA_64>(Op->Header.Args[0].ID()));
  if (!CTX->Config.UnifiedMemory) {
    LoadConstant(TMP1, (uint64_t)CTX->MemoryMapper.GetMemoryBase());
    MemSrc = MemOperand(TMP1, GetReg<RA_64>(Op->Header.Args[0].ID()));
  }

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
  if (!CTX->Config.UnifiedMemory) {
    LoadConstant(TMP1, (uint64_t)CTX->MemoryMapper.GetMemoryBase());
    add(TMP1, TMP1, GetReg<RA_64>(Op->Header.Args[0].ID()));
    MemSrc = MemOperand(TMP1);
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
void JITCore::RegisterMemoryHandlers() {
#define REGISTER_OP(op, x) OpHandlers[FEXCore::IR::IROps::OP_##op] = &JITCore::Op_##x
  REGISTER_OP(LOADCONTEXTPAIR,     LoadContextPair);
  REGISTER_OP(STORECONTEXTPAIR,    StoreContextPair);
  REGISTER_OP(LOADCONTEXT,         LoadContext);
  REGISTER_OP(STORECONTEXT,        StoreContext);
  REGISTER_OP(LOADCONTEXTINDEXED,  LoadContextIndexed);
  REGISTER_OP(STORECONTEXTINDEXED, StoreContextIndexed);
  REGISTER_OP(SPILLREGISTER,       SpillRegister);
  REGISTER_OP(FILLREGISTER,        FillRegister);
  REGISTER_OP(LOADFLAG,            LoadFlag);
  REGISTER_OP(STOREFLAG,           StoreFlag);
  REGISTER_OP(LOADMEM,             LoadMem);
  REGISTER_OP(STOREMEM,            StoreMem);
  if (CTX->Config.TSOEnabled) {
    REGISTER_OP(LOADMEMTSO,          LoadMemTSO);
    REGISTER_OP(STOREMEMTSO,         StoreMemTSO);
  }
  else {
    REGISTER_OP(LOADMEMTSO,          LoadMem);
    REGISTER_OP(STOREMEMTSO,         StoreMem);
  }
  REGISTER_OP(VLOADMEMELEMENT,     VLoadMemElement);
  REGISTER_OP(VSTOREMEMELEMENT,    VStoreMemElement);
#undef REGISTER_OP
}
}

