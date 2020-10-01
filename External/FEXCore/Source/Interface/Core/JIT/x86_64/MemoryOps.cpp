#include "Interface/Core/JIT/x86_64/JITClass.h"
#include "Interface/IR/Passes/RegisterAllocationPass.h"

namespace FEXCore::CPU {

#define DEF_OP(x) void JITCore::Op_##x(FEXCore::IR::IROp_Header *IROp, uint32_t Node)
DEF_OP(LoadContextPair) {
  auto Op = IROp->C<IR::IROp_LoadContextPair>();
  switch (Op->Size) {
    case 4: {
      auto Dst = GetSrcPair<RA_32>(Node);
      mov(Dst.first,  dword [STATE + Op->Offset]);
      mov(Dst.second, dword [STATE + Op->Offset + Op->Size]);
      break;
    }
    case 8: {
      auto Dst = GetSrcPair<RA_64>(Node);
      mov(Dst.first,  qword [STATE + Op->Offset]);
      mov(Dst.second, qword [STATE + Op->Offset + Op->Size]);
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
      mov(dword [STATE + Op->Offset], Src.first);
      mov(dword [STATE + Op->Offset + Op->Size], Src.first);
      break;
    }
    case 8: {
      auto Src = GetSrcPair<RA_64>(Op->Header.Args[0].ID());
      mov(qword [STATE + Op->Offset], Src.first);
      mov(qword [STATE + Op->Offset + Op->Size], Src.first);
      break;
    }
  }
}

DEF_OP(LoadContext) {
  auto Op = IROp->C<IR::IROp_LoadContext>();
  uint8_t OpSize = IROp->Size;

  if (Op->Class.Val == 0) {
    switch (OpSize) {
    case 1: {
      movzx(GetDst<RA_32>(Node), byte [STATE + Op->Offset]);
    }
    break;
    case 2: {
      movzx(GetDst<RA_32>(Node), word [STATE + Op->Offset]);
    }
    break;
    case 4: {
      mov(GetDst<RA_32>(Node), dword [STATE + Op->Offset]);
    }
    break;
    case 8: {
      mov(GetDst<RA_64>(Node), qword [STATE + Op->Offset]);
    }
    break;
    case 16: {
      LogMan::Msg::A("Invalid GPR load of size 16");
    }
    break;
    default:  LogMan::Msg::A("Unhandled LoadContext size: %d", OpSize);
    }
  }
  else {
    switch (OpSize) {
    case 1: {
      movzx(rax, byte [STATE + Op->Offset]);
      vmovq(GetDst(Node), rax);
    }
    break;
    case 2: {
      movzx(rax, word [STATE + Op->Offset]);
      vmovq(GetDst(Node), rax);
    }
    break;
    case 4: {
      vmovd(GetDst(Node), dword [STATE + Op->Offset]);
    }
    break;
    case 8: {
      vmovq(GetDst(Node), qword [STATE + Op->Offset]);
    }
    break;
    case 16: {
      if (Op->Offset % 16 == 0)
        movaps(GetDst(Node), xword [STATE + Op->Offset]);
      else
        movups(GetDst(Node), xword [STATE + Op->Offset]);
    }
    break;
    default:  LogMan::Msg::A("Unhandled LoadContext size: %d", OpSize);
    }
  }
}

DEF_OP(StoreContext) {
  auto Op = IROp->C<IR::IROp_StoreContext>();
  uint8_t OpSize = IROp->Size;

  if (Op->Class.Val == 0) {
    switch (OpSize) {
    case 1: {
      mov(byte [STATE + Op->Offset], GetSrc<RA_8>(Op->Header.Args[0].ID()));
    }
    break;

    case 2: {
      mov(word [STATE + Op->Offset], GetSrc<RA_16>(Op->Header.Args[0].ID()));
    }
    break;
    case 4: {
      mov(dword [STATE + Op->Offset], GetSrc<RA_32>(Op->Header.Args[0].ID()));
    }
    break;
    case 8: {
      mov(qword [STATE + Op->Offset], GetSrc<RA_64>(Op->Header.Args[0].ID()));
    }
    break;
    case 16:
      LogMan::Msg::D("Invalid store size of 16");
    break;
    default:  LogMan::Msg::A("Unhandled StoreContext size: %d", OpSize);
    }
  }
  else {
    switch (OpSize) {
    case 1: {
      pextrb(byte [STATE + Op->Offset], GetSrc(Op->Header.Args[0].ID()), 0);
    }
    break;

    case 2: {
      pextrw(word [STATE + Op->Offset], GetSrc(Op->Header.Args[0].ID()), 0);
    }
    break;
    case 4: {
      vmovd(dword [STATE + Op->Offset], GetSrc(Op->Header.Args[0].ID()));
    }
    break;
    case 8: {
      vmovq(qword [STATE + Op->Offset], GetSrc(Op->Header.Args[0].ID()));
    }
    break;
    case 16: {
      if (Op->Offset % 16 == 0)
        movaps(xword [STATE + Op->Offset], GetSrc(Op->Header.Args[0].ID()));
      else
        movups(xword [STATE + Op->Offset], GetSrc(Op->Header.Args[0].ID()));
    }
    break;
    default:  LogMan::Msg::A("Unhandled StoreContext size: %d", OpSize);
    }
  }
}

DEF_OP(LoadContextIndexed) {
  auto Op = IROp->C<IR::IROp_LoadContextIndexed>();
  size_t size = Op->Size;
  Reg index = GetSrc<RA_64>(Op->Header.Args[0].ID());

  if (Op->Class.Val == 0) {
    switch (Op->Stride) {
    case 1:
    case 2:
    case 4:
    case 8: {
      lea(rax, dword [STATE + Op->BaseOffset]);
      switch (size) {
      case 1:
        movzx(GetDst<RA_32>(Node), byte [rax + index * Op->Stride]);
        break;
      case 2:
        movzx(GetDst<RA_32>(Node), word [rax + index * Op->Stride]);
        break;
      case 4:
        mov(GetDst<RA_32>(Node),  dword [rax + index * Op->Stride]);
        break;
      case 8:
        mov(GetDst<RA_64>(Node),  qword [rax + index * Op->Stride]);
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
    case 8: {
      lea(rax, dword [STATE + Op->BaseOffset]);
      switch (size) {
      case 1:
        movzx(eax, byte [rax + index * Op->Stride]);
        vmovd(GetDst(Node), eax);
        break;
      case 2:
        movzx(eax, word [rax + index * Op->Stride]);
        vmovd(GetDst(Node), eax);
        break;
      case 4:
        vmovd(GetDst(Node),  dword [rax + index * Op->Stride]);
        break;
      case 8:
        vmovq(GetDst(Node),  qword [rax + index * Op->Stride]);
        break;
      default:
        LogMan::Msg::A("Unhandled LoadContextIndexed size: %d", Op->Size);
      }
      break;
    }
    case 16: {
      mov(rax, index);
      shl(rax, 4);
      lea(rax, dword [rax + Op->BaseOffset]);
      switch (size) {
      case 1:
        pinsrb(GetDst(Node), byte [STATE + rax], 0);
        break;
      case 2:
        pinsrw(GetDst(Node), word [STATE + rax], 0);
        break;
      case 4:
        vmovd(GetDst(Node), dword [STATE + rax]);
        break;
      case 8:
        vmovq(GetDst(Node), qword [STATE + rax]);
        break;
      case 16:
        if (Op->BaseOffset % 16 == 0)
          movaps(GetDst(Node), xword [STATE + rax]);
        else
          movups(GetDst(Node), xword [STATE + rax]);
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
  Reg index = GetSrc<RA_64>(Op->Header.Args[1].ID());
  size_t size = Op->Size;

  if (Op->Class.Val == 0) {
    auto value = GetSrc<RA_64>(Op->Header.Args[0].ID());
    lea(rax, dword [STATE + Op->BaseOffset]);

    switch (Op->Stride) {
    case 1:
    case 2:
    case 4:
    case 8: {
      if (!(size == 1 || size == 2 || size == 4 || size == 8)) {
        LogMan::Msg::A("Unhandled StoreContextIndexed size: %d", Op->Size);
      }
      mov(AddressFrame(Op->Size * 8) [rax + index * Op->Stride], value);
      break;
    }
    default:
      LogMan::Msg::A("Unhandled StoreContextIndexed stride: %d", Op->Stride);
    }
  }
  else {
    auto value = GetSrc(Op->Header.Args[0].ID());
    switch (Op->Stride) {
    case 1:
    case 2:
    case 4:
    case 8: {
      lea(rax, dword [STATE + Op->BaseOffset]);
      switch (size) {
      case 1:
        pextrb(AddressFrame(Op->Size * 8) [rax + index * Op->Stride], value, 0);
        break;
      case 2:
        pextrw(AddressFrame(Op->Size * 8) [rax + index * Op->Stride], value, 0);
        break;
      case 4:
        vmovd(AddressFrame(Op->Size * 8) [rax + index * Op->Stride], value);
        break;
      case 8:
        vmovq(AddressFrame(Op->Size * 8) [rax + index * Op->Stride], value);
        break;
      default:
        LogMan::Msg::A("Unhandled StoreContextIndexed size: %d", size);
      }
      break;
    }
    case 16: {
      mov(rax, index);
      shl(rax, 4);
      lea(rax, dword [rax + Op->BaseOffset]);
      switch (size) {
      case 1:
        pextrb(AddressFrame(Op->Size * 8) [STATE + rax], value, 0);
        break;
      case 2:
        pextrw(AddressFrame(Op->Size * 8) [STATE + rax], value, 0);
        break;
      case 4:
        vmovd(AddressFrame(Op->Size * 8) [STATE + rax], value);
        break;
      case 8:
        vmovq(AddressFrame(Op->Size * 8) [STATE + rax], value);
        break;
      case 16:
        if (Op->BaseOffset % 16 == 0)
          movaps(xword [STATE + rax], value);
        else
          movups(xword [STATE + rax], value);
        break;
      default:
        LogMan::Msg::A("Unhandled StoreContextIndexed size: %d", size);
      }
      break;
    }
    default:
      LogMan::Msg::A("Unhandled StoreContextIndexed stride: %d", Op->Stride);
    }
  }
}

DEF_OP(SpillRegister) {
  auto Op = IROp->C<IR::IROp_SpillRegister>();
  uint8_t OpSize = IROp->Size;

  uint32_t SlotOffset = Op->Slot * 16;
  switch (OpSize) {
    case 1: {
      mov(byte [rsp + SlotOffset], GetSrc<RA_8>(Op->Header.Args[0].ID()));
      break;
    }
    case 2: {
      mov(word [rsp + SlotOffset], GetSrc<RA_16>(Op->Header.Args[0].ID()));
      break;
    }
    case 4: {
      mov(dword [rsp + SlotOffset], GetSrc<RA_32>(Op->Header.Args[0].ID()));
      break;
    }
    case 8: {
      mov(qword [rsp + SlotOffset], GetSrc<RA_64>(Op->Header.Args[0].ID()));
      break;
    }
    case 16: {
      movaps(xword [rsp + SlotOffset], GetSrc(Op->Header.Args[0].ID()));
      break;
    }
    default:  LogMan::Msg::A("Unhandled SpillRegister size: %d", OpSize);
  }
}

DEF_OP(FillRegister) {
  auto Op = IROp->C<IR::IROp_FillRegister>();
  uint8_t OpSize = IROp->Size;

  uint32_t SlotOffset = Op->Slot * 16;
  switch (OpSize) {
    case 1: {
      movzx(GetDst<RA_32>(Node), byte [rsp + SlotOffset]);
      break;
    }
    case 2: {
      movzx(GetDst<RA_32>(Node), word [rsp + SlotOffset]);
      break;
    }
    case 4: {
      mov(GetDst<RA_32>(Node), dword [rsp + SlotOffset]);
      break;
    }
    case 8: {
      mov(GetDst<RA_64>(Node), qword [rsp + SlotOffset]);
      break;
    }
    case 16: {
      movaps(GetDst(Node), xword [rsp + SlotOffset]);
      break;
    }
    default:  LogMan::Msg::A("Unhandled FillRegister size: %d", OpSize);
  }
}

DEF_OP(LoadFlag) {
  auto Op = IROp->C<IR::IROp_LoadFlag>();

  auto Dst = GetDst<RA_64>(Node);
  movzx(Dst, byte [STATE + (offsetof(FEXCore::Core::CPUState, flags[0]) + Op->Flag)]);
  and(Dst, 1);
}

DEF_OP(StoreFlag) {
  auto Op = IROp->C<IR::IROp_StoreFlag>();

  mov (rax, GetSrc<RA_64>(Op->Header.Args[0].ID()));
  and(rax, 1);
  mov(byte [STATE + (offsetof(FEXCore::Core::CPUState, flags[0]) + Op->Flag)], al);
}

DEF_OP(LoadMem) {
  auto Op = IROp->C<IR::IROp_LoadMem>();
  uint64_t Memory = CTX->MemoryMapper.GetBaseOffset<uint64_t>(0);

  Xbyak::Reg MemReg = rax;
  if (CTX->Config.UnifiedMemory) {
    MemReg = GetSrc<RA_64>(Op->Header.Args[0].ID());
  }
  else {
    mov(MemReg, Memory);
    add(MemReg, GetSrc<RA_64>(Op->Header.Args[0].ID()));
  }
  if (Op->Class.Val == 0) {
    auto Dst = GetDst<RA_64>(Node);

    switch (Op->Size) {
      case 1: {
        movzx (Dst, byte [MemReg]);
      }
      break;
      case 2: {
        movzx (Dst, word [MemReg]);
      }
      break;
      case 4: {
        mov(Dst.cvt32(), dword [MemReg]);
      }
      break;
      case 8: {
        mov(Dst, qword [MemReg]);
      }
      break;
      default:  LogMan::Msg::A("Unhandled LoadMem size: %d", Op->Size);
    }
  }
  else
  {
    auto Dst = GetDst(Node);

    switch (Op->Size) {
      case 1: {
        movzx(eax, byte [MemReg]);
        vmovd(Dst, eax);
      }
      break;
      case 2: {
        movzx(eax, byte [MemReg]);
        vmovd(Dst, eax);
      }
      break;
      case 4: {
        vmovd(Dst, dword [MemReg]);
      }
      break;
      case 8: {
        vmovq(Dst, qword [MemReg]);
      }
      break;
      case 16: {
         if (Op->Size == Op->Align)
           movups(GetDst(Node), xword [MemReg]);
         else
           movups(GetDst(Node), xword [MemReg]);
         if (MemoryDebug) {
           movq(rcx, GetDst(Node));
         }
       }
       break;
      default:  LogMan::Msg::A("Unhandled LoadMem size: %d", Op->Size);
    }
  }
}

DEF_OP(StoreMem) {
  auto Op = IROp->C<IR::IROp_StoreMem>();
  uint64_t Memory = CTX->MemoryMapper.GetBaseOffset<uint64_t>(0);

  Xbyak::Reg MemReg = rax;
  if (CTX->Config.UnifiedMemory) {
    MemReg = GetSrc<RA_64>(Op->Header.Args[0].ID());
  }
  else {
    mov(MemReg, Memory);
    add(MemReg, GetSrc<RA_64>(Op->Header.Args[0].ID()));
  }

  if (Op->Class.Val == 0) {
    switch (Op->Size) {
    case 1:
      mov(byte [MemReg], GetSrc<RA_8>(Op->Header.Args[1].ID()));
    break;
    case 2:
      mov(word [MemReg], GetSrc<RA_16>(Op->Header.Args[1].ID()));
    break;
    case 4:
      mov(dword [MemReg], GetSrc<RA_32>(Op->Header.Args[1].ID()));
    break;
    case 8:
      mov(qword [MemReg], GetSrc<RA_64>(Op->Header.Args[1].ID()));
    break;
    default:  LogMan::Msg::A("Unhandled StoreMem size: %d", Op->Size);
    }
  }
  else {
    switch (Op->Size) {
    case 1:
      pextrb(byte [MemReg], GetSrc(Op->Header.Args[1].ID()), 0);
    break;
    case 2:
      pextrw(word [MemReg], GetSrc(Op->Header.Args[1].ID()), 0);
    break;
    case 4:
      vmovd(dword [MemReg], GetSrc(Op->Header.Args[1].ID()));
    break;
    case 8:
      vmovq(qword [MemReg], GetSrc(Op->Header.Args[1].ID()));
    break;
    case 16:
      if (Op->Size == Op->Align)
        movups(xword [MemReg], GetSrc(Op->Header.Args[1].ID()));
      else
        movups(xword [MemReg], GetSrc(Op->Header.Args[1].ID()));
    break;
    default:  LogMan::Msg::A("Unhandled StoreMem size: %d", Op->Size);
    }
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
  REGISTER_OP(LOADMEMTSO,          LoadMem);
  REGISTER_OP(STOREMEMTSO,         StoreMem);
  REGISTER_OP(VLOADMEMELEMENT,     VLoadMemElement);
  REGISTER_OP(VSTOREMEMELEMENT,    VStoreMemElement);
#undef REGISTER_OP
}
}

