/*
$info$
tags: backend|x86-64
$end_info$
*/

#include "Interface/Core/CPUID.h"
#include "Interface/Core/JIT/x86_64/JITClass.h"
#include "Interface/Core/Dispatcher/X86Dispatcher.h"
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/IR/IR.h>
#include <FEXCore/Utils/LogManager.h>

#include <array>
#include <stddef.h>
#include <stdint.h>

namespace FEXCore::CPU {

#define DEF_OP(x) void X86JITCore::Op_##x(IR::IROp_Header *IROp, IR::NodeID Node)

DEF_OP(LoadContext) {
  const auto Op = IROp->C<IR::IROp_LoadContext>();
  const auto OpSize = IROp->Size;

  if (Op->Class == IR::GPRClass) {
    switch (OpSize) {
    case 1: {
      movzx(GetDst<RA_32>(Node), byte [STATE + Op->Offset]);
      break;
    }
    case 2: {
      movzx(GetDst<RA_32>(Node), word [STATE + Op->Offset]);
      break;
    }
    case 4: {
      mov(GetDst<RA_32>(Node), dword [STATE + Op->Offset]);
      break;
    }
    case 8: {
      mov(GetDst<RA_64>(Node), qword [STATE + Op->Offset]);
      break;
    }
    case 16: {
      LOGMAN_MSG_A_FMT("Invalid GPR load of size 16");
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unhandled LoadContext size: {}", OpSize);
      break;
    }
  }
  else {
    const auto Dst = GetDst(Node);

    switch (OpSize) {
    case 1: {
      movzx(rax, byte [STATE + Op->Offset]);
      vmovq(Dst, rax);
      break;
    }
    case 2: {
      movzx(rax, word [STATE + Op->Offset]);
      vmovq(Dst, rax);
      break;
    }
    case 4: {
      vmovd(Dst, dword [STATE + Op->Offset]);
      break;
    }
    case 8: {
      vmovq(Dst, qword [STATE + Op->Offset]);
      break;
    }
    case 16: {
      if (Op->Offset % 16 == 0) {
        vmovaps(Dst, xword [STATE + Op->Offset]);
      } else {
        vmovups(Dst, xword [STATE + Op->Offset]);
      }
      break;
    }
    case 32: {
      vmovups(ToYMM(Dst), yword [STATE + Op->Offset]);
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unhandled LoadContext size: {}", OpSize);
      break;
    }
  }
}

DEF_OP(StoreContext) {
  const auto Op = IROp->C<IR::IROp_StoreContext>();
  const auto OpSize = IROp->Size;

  if (Op->Class == IR::GPRClass) {
    switch (OpSize) {
    case 1: {
      mov(byte [STATE + Op->Offset], GetSrc<RA_8>(Op->Value.ID()));
      break;
    }
    case 2: {
      mov(word [STATE + Op->Offset], GetSrc<RA_16>(Op->Value.ID()));
      break;
    }
    case 4: {
      mov(dword [STATE + Op->Offset], GetSrc<RA_32>(Op->Value.ID()));
      break;
    }
    case 8: {
      mov(qword [STATE + Op->Offset], GetSrc<RA_64>(Op->Value.ID()));
      break;
    }
    case 16: {
      LOGMAN_MSG_A_FMT("Invalid store size of 16");
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unhandled StoreContext size: {}", OpSize);
      break;
    }
  }
  else {
    const auto Value = GetSrc(Op->Value.ID());

    switch (OpSize) {
    case 1: {
      pextrb(byte [STATE + Op->Offset], Value, 0);
      break;
    }
    case 2: {
      pextrw(word [STATE + Op->Offset], Value, 0);
      break;
    }
    case 4: {
      vmovd(dword [STATE + Op->Offset], Value);
      break;
    }
    case 8: {
      vmovq(qword [STATE + Op->Offset], Value);
      break;
    }
    case 16: {
      if (Op->Offset % 16 == 0) {
        vmovaps(xword [STATE + Op->Offset], Value);
      } else {
        vmovups(xword [STATE + Op->Offset], Value);
      }
      break;
    }
    case 32: {
      vmovups(yword [STATE + Op->Offset], ToYMM(Value));
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unhandled StoreContext size: {}", OpSize);
      break;
    }
  }
}

DEF_OP(LoadRegister) {
  const auto Op = IROp->C<IR::IROp_LoadRegister>();
  const auto OpSize = IROp->Size;

  if (Op->Class == IR::GPRClass) {
    switch (OpSize) {
    case 1: {
      movzx(GetSrc<RA_32>(Node), byte [STATE + Op->Offset]);
      break;
    }
    case 2: {
      movzx(GetSrc<RA_32>(Node), word [STATE + Op->Offset]);
      break;
    }
    case 4: {
      mov(GetSrc<RA_32>(Node), dword [STATE + Op->Offset]);
      break;
    }
    case 8: {
      mov(GetSrc<RA_64>(Node), qword [STATE + Op->Offset]);
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unhandled LoadRegister size: {}", OpSize);
      break;
    }
  }
  else {
    const auto Dst = GetSrc(Node);

    switch (OpSize) {
    case 1: {
      movzx(rax, byte [STATE + Op->Offset]);
      vmovq(Dst, rax);
      break;
    }
    case 2: {
      movzx(rax, word [STATE + Op->Offset]);
      vmovq(Dst, rax);
      break;
    }
    case 4: {
      vmovd(Dst, dword [STATE + Op->Offset]);
      break;
    }
    case 8: {
      vmovq(Dst, qword [STATE + Op->Offset]);
      break;
    }
    case 16: {
      if (Op->Offset % 16 == 0) {
        vmovaps(Dst, xword [STATE + Op->Offset]);
      } else {
        vmovups(Dst, xword [STATE + Op->Offset]);
      }
      break;
    }
    case 32: {
      vmovups(ToYMM(Dst), yword [STATE + Op->Offset]);
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unhandled LoadRegister size: {}", OpSize);
      break;
    }
  }
}

DEF_OP(StoreRegister) {
  const auto Op = IROp->C<IR::IROp_StoreRegister>();
  const auto OpSize = IROp->Size;

  if (Op->Class == IR::GPRClass) {
    const auto regOffs = Op->Offset & 7;

    switch (OpSize) {
      case 4:
        LOGMAN_THROW_AA_FMT(regOffs == 0, "unexpected regOffs");
        mov(dword [STATE + Op->Offset], GetSrc<RA_32>(Op->Value.ID()));
        break;

      case 8:
        LOGMAN_THROW_AA_FMT(regOffs == 0, "unexpected regOffs");
        mov(qword [STATE + Op->Offset], GetSrc<RA_64>(Op->Value.ID()));
        break;

      default:
        LOGMAN_MSG_A_FMT("Unhandled StoreRegister GPR size: {}", OpSize);
        break;
    }
  } else if (Op->Class == IR::FPRClass) {
    const auto Value = GetSrc(Op->Value.ID());
    switch (OpSize) {
      case 16: {
        if (Op->Offset % 16 == 0) {
          vmovaps(xword [STATE + Op->Offset], Value);
        } else {
          vmovups(xword [STATE + Op->Offset], Value);
        }
        break;
      }
      case 32: {
        vmovups(yword [STATE + Op->Offset], ToYMM(Value));
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unhandled StoreContext size: {}", OpSize);
        break;
    }
  } else {
    LOGMAN_THROW_AA_FMT(false, "Unhandled Op->Class {}", Op->Class);
  }
}

DEF_OP(LoadContextIndexed) {
  const auto Op = IROp->C<IR::IROp_LoadContextIndexed>();
  const auto OpSize = IROp->Size;

  const Reg Index = GetSrc<RA_64>(Op->Index.ID());

  if (Op->Class == IR::GPRClass) {
    switch (Op->Stride) {
    case 1:
    case 2:
    case 4:
    case 8: {
      lea(rax, dword [STATE + Op->BaseOffset]);
      switch (OpSize) {
      case 1:
        movzx(GetDst<RA_32>(Node), byte [rax + Index * Op->Stride]);
        break;
      case 2:
        movzx(GetDst<RA_32>(Node), word [rax + Index * Op->Stride]);
        break;
      case 4:
        mov(GetDst<RA_32>(Node),  dword [rax + Index * Op->Stride]);
        break;
      case 8:
        mov(GetDst<RA_64>(Node),  qword [rax + Index * Op->Stride]);
        break;
      default:
        LOGMAN_MSG_A_FMT("Unhandled LoadContextIndexed size: {}", OpSize);
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
    case 8: {
      const auto Dst = GetDst(Node);

      lea(rax, dword [STATE + Op->BaseOffset]);
      switch (OpSize) {
      case 1:
        movzx(eax, byte [rax + Index * Op->Stride]);
        vmovd(Dst, eax);
        break;
      case 2:
        movzx(eax, word [rax + Index * Op->Stride]);
        vmovd(Dst, eax);
        break;
      case 4:
        vmovd(Dst,  dword [rax + Index * Op->Stride]);
        break;
      case 8:
        vmovq(Dst,  qword [rax + Index * Op->Stride]);
        break;
      default:
        LOGMAN_MSG_A_FMT("Unhandled LoadContextIndexed size: {}", OpSize);
        break;
      }
      break;
    }
    case 16:
    case 32: {
      const auto Dst = GetDst(Node);
      const auto Shift = Op->Stride == 16 ? 4 : 5;

      mov(rax, Index);
      shl(rax, Shift);
      lea(rax, dword [rax + Op->BaseOffset]);
      switch (OpSize) {
      case 1:
        pinsrb(Dst, byte [STATE + rax], 0);
        break;
      case 2:
        pinsrw(Dst, word [STATE + rax], 0);
        break;
      case 4:
        vmovd(Dst, dword [STATE + rax]);
        break;
      case 8:
        vmovq(Dst, qword [STATE + rax]);
        break;
      case 16:
        if (Op->BaseOffset % 16 == 0) {
          vmovaps(Dst, xword [STATE + rax]);
        } else {
          vmovups(Dst, xword [STATE + rax]);
        }
        break;
      case 32:
        vmovups(ToYMM(Dst), yword [STATE + rax]);
        break;
      default:
        LOGMAN_MSG_A_FMT("Unhandled LoadContextIndexed size: {}", OpSize);
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
  const auto Op = IROp->C<IR::IROp_StoreContextIndexed>();
  const auto OpSize = IROp->Size;

  const Reg Index = GetSrc<RA_64>(Op->Index.ID());

  if (Op->Class == IR::GPRClass) {
    const auto Value = GetSrc<RA_64>(Op->Value.ID());
    lea(rax, dword [STATE + Op->BaseOffset]);

    switch (Op->Stride) {
    case 1:
    case 2:
    case 4:
    case 8: {
      if (!(OpSize == 1 || OpSize == 2 || OpSize == 4 || OpSize == 8)) {
        LOGMAN_MSG_A_FMT("Unhandled StoreContextIndexed size: {}", OpSize);
      }
      mov(AddressFrame(OpSize * 8) [rax + Index * Op->Stride], Value);
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unhandled StoreContextIndexed stride: {}", Op->Stride);
      break;
    }
  }
  else {
    const auto Value = GetSrc(Op->Value.ID());
    switch (Op->Stride) {
    case 1:
    case 2:
    case 4:
    case 8: {
      lea(rax, dword [STATE + Op->BaseOffset]);
      switch (OpSize) {
      case 1:
        pextrb(AddressFrame(OpSize * 8) [rax + Index * Op->Stride], Value, 0);
        break;
      case 2:
        pextrw(AddressFrame(OpSize * 8) [rax + Index * Op->Stride], Value, 0);
        break;
      case 4:
        vmovd(AddressFrame(OpSize * 8) [rax + Index * Op->Stride], Value);
        break;
      case 8:
        vmovq(AddressFrame(OpSize * 8) [rax + Index * Op->Stride], Value);
        break;
      default:
        LOGMAN_MSG_A_FMT("Unhandled StoreContextIndexed size: {}", OpSize);
        break;
      }
      break;
    }
    case 16:
    case 32: {
      const auto Shift = Op->Stride == 16 ? 4 : 5;

      mov(rax, Index);
      shl(rax, Shift);
      lea(rax, dword [rax + Op->BaseOffset]);
      switch (OpSize) {
      case 1:
        pextrb(AddressFrame(OpSize * 8) [STATE + rax], Value, 0);
        break;
      case 2:
        pextrw(AddressFrame(OpSize * 8) [STATE + rax], Value, 0);
        break;
      case 4:
        vmovd(AddressFrame(OpSize * 8) [STATE + rax], Value);
        break;
      case 8:
        vmovq(AddressFrame(OpSize * 8) [STATE + rax], Value);
        break;
      case 16:
        if (Op->BaseOffset % 16 == 0) {
          vmovaps(xword [STATE + rax], Value);
        } else {
          vmovups(xword [STATE + rax], Value);
        }
        break;
      case 32:
        vmovups(yword [STATE + rax], ToYMM(Value));
        break;
      default:
        LOGMAN_MSG_A_FMT("Unhandled StoreContextIndexed size: {}", OpSize);
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
  const auto Op = IROp->C<IR::IROp_SpillRegister>();
  const uint8_t OpSize = IROp->Size;
  const uint32_t SlotOffset = Op->Slot * MaxSpillSlotSize;

  if (Op->Class == FEXCore::IR::GPRClass) {
    switch (OpSize) {
      case 1: {
        mov(byte [rsp + SlotOffset], GetSrc<RA_8>(Op->Value.ID()));
        break;
      }
      case 2: {
        mov(word [rsp + SlotOffset], GetSrc<RA_16>(Op->Value.ID()));
        break;
      }
      case 4: {
        mov(dword [rsp + SlotOffset], GetSrc<RA_32>(Op->Value.ID()));
        break;
      }
      case 8: {
        mov(qword [rsp + SlotOffset], GetSrc<RA_64>(Op->Value.ID()));
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unhandled SpillRegister size: {}", OpSize);
        break;
    }
  } else if (Op->Class == FEXCore::IR::FPRClass) {
    const auto Src = GetSrc(Op->Value.ID());

    switch (OpSize) {
      case 4: {
        movss(dword [rsp + SlotOffset], Src);
        break;
      }
      case 8: {
        movsd(qword [rsp + SlotOffset], Src);
        break;
      }
      case 16: {
        movaps(xword [rsp + SlotOffset], Src);
        break;
      }
      case 32: {
        vmovups(yword [rsp + SlotOffset], ToYMM(Src));
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unhandled SpillRegister size: {}", OpSize);
        break;
    }
  } else {
    LOGMAN_MSG_A_FMT("Unhandled SpillRegister class: {}", Op->Class.Val);
  }
}

DEF_OP(FillRegister) {
  const auto Op = IROp->C<IR::IROp_FillRegister>();
  const uint8_t OpSize = IROp->Size;
  const uint32_t SlotOffset = Op->Slot * MaxSpillSlotSize;

  if (Op->Class == FEXCore::IR::GPRClass) {
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
      default:
        LOGMAN_MSG_A_FMT("Unhandled FillRegister size: {}", OpSize);
        break;
    }
  } else if (Op->Class == FEXCore::IR::FPRClass) {
    const auto Dst = GetDst(Node);

    switch (OpSize) {
      case 4: {
        vmovss(Dst, dword [rsp + SlotOffset]);
        break;
      }
      case 8: {
        vmovsd(Dst, qword [rsp + SlotOffset]);
        break;
      }
      case 16: {
        vmovaps(Dst, xword [rsp + SlotOffset]);
        break;
      }
      case 32: {
        vmovups(ToYMM(Dst), yword [rsp + SlotOffset]);
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unhandled FillRegister size: {}", OpSize);
        break;
    }
  } else {
    LOGMAN_MSG_A_FMT("Unhandled FillRegister class: {}", Op->Class.Val);
  }
}

DEF_OP(LoadFlag) {
  auto Op = IROp->C<IR::IROp_LoadFlag>();

  auto Dst = GetDst<RA_64>(Node);

  if (Op->Flag == 24 /* NZCV */)
    mov(Dst.cvt32(), dword [STATE + (offsetof(FEXCore::Core::CPUState, flags[0]) + Op->Flag)]);
  else
    movzx(Dst, byte [STATE + (offsetof(FEXCore::Core::CPUState, flags[0]) + Op->Flag)]);
}

DEF_OP(StoreFlag) {
  auto Op = IROp->C<IR::IROp_StoreFlag>();

  mov (rax, GetSrc<RA_64>(Op->Value.ID()));

  if (Op->Flag == 24 /* NZCV */)
    mov(dword [STATE + (offsetof(FEXCore::Core::CPUState, flags[0]) + Op->Flag)], eax);
  else
    mov(byte [STATE + (offsetof(FEXCore::Core::CPUState, flags[0]) + Op->Flag)], al);
}

Xbyak::RegExp X86JITCore::GenerateModRM(Xbyak::Reg Base, IR::OrderedNodeWrapper Offset, IR::MemOffsetType OffsetType, uint8_t OffsetScale) const {
  if (Offset.IsInvalid()) {
    return Base;
  } else {
    if (OffsetScale != 1 && OffsetScale != 2 && OffsetScale != 4 && OffsetScale != 8) {
      LOGMAN_MSG_A_FMT("Unhandled GenerateModRM OffsetScale: {}", OffsetScale);
    }

    if (OffsetType != IR::MEM_OFFSET_SXTX) {
      LOGMAN_MSG_A_FMT("Unhandled GenerateModRM OffsetType: {}", OffsetType.Val);
    }

    uint64_t Const;
    if (IsInlineConstant(Offset, &Const)) {
      return Base + Const;
    } else {
      auto MemOffset = GetSrc<RA_64>(Offset.ID());

      return Base + MemOffset * OffsetScale;
    }
  }
}

DEF_OP(LoadMem) {
  const auto Op = IROp->C<IR::IROp_LoadMem>();
  const auto OpSize = IROp->Size;

  const Xbyak::Reg MemReg = GetSrc<RA_64>(Op->Addr.ID());
  const auto MemPtr = GenerateModRM(MemReg, Op->Offset, Op->OffsetType, Op->OffsetScale);

  if (Op->Class == IR::GPRClass) {
    const auto Dst = GetDst<RA_64>(Node);

    switch (OpSize) {
      case 1: {
        movzx(Dst, byte [MemPtr]);
        break;
      }
      case 2: {
        movzx(Dst, word [MemPtr]);
        break;
      }
      case 4: {
        mov(Dst.cvt32(), dword [MemPtr]);
        break;
      }
      case 8: {
        mov(Dst, qword [MemPtr]);
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unhandled LoadMem size: {}", OpSize);
        break;
    }
  }
  else
  {
    const auto Dst = GetDst(Node);

    switch (OpSize) {
      case 1: {
        movzx(eax, byte [MemPtr]);
        vmovd(Dst, eax);
        break;
      }
      case 2: {
        movzx(eax, word [MemPtr]);
        vmovd(Dst, eax);
        break;
      }
      case 4: {
        vmovd(Dst, dword [MemPtr]);
        break;
      }
      case 8: {
        vmovq(Dst, qword [MemPtr]);
        break;
      }
      case 16: {
        vmovups(Dst, xword [MemPtr]);
        if (MemoryDebug) {
          movq(rcx, Dst);
        }
        break;
      }
      case 32: {
        vmovups(ToYMM(Dst), yword [MemPtr]);
        if (MemoryDebug) {
          movq(rcx, Dst);
        }
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unhandled LoadMem size: {}", OpSize);
        break;
    }
  }
}

DEF_OP(StoreMem) {
  const auto Op = IROp->C<IR::IROp_StoreMem>();
  const auto OpSize = IROp->Size;

  const Xbyak::Reg MemReg = GetSrc<RA_64>(Op->Addr.ID());
  const auto MemPtr = GenerateModRM(MemReg, Op->Offset, Op->OffsetType, Op->OffsetScale);

  if (Op->Class == IR::GPRClass) {
    switch (OpSize) {
    case 1:
      mov(byte [MemPtr], GetSrc<RA_8>(Op->Value.ID()));
      break;
    case 2:
      mov(word [MemPtr], GetSrc<RA_16>(Op->Value.ID()));
      break;
    case 4:
      mov(dword [MemPtr], GetSrc<RA_32>(Op->Value.ID()));
      break;
    case 8:
      mov(qword [MemPtr], GetSrc<RA_64>(Op->Value.ID()));
      break;
    default:
      LOGMAN_MSG_A_FMT("Unhandled StoreMem size: {}", OpSize);
      break;
    }
  }
  else {
    const auto Value = GetSrc(Op->Value.ID());

    switch (OpSize) {
    case 1:
      pextrb(byte [MemPtr], Value, 0);
      break;
    case 2:
      pextrw(word [MemPtr], Value, 0);
      break;
    case 4:
      vmovd(dword [MemPtr], Value);
      break;
    case 8:
      vmovq(qword [MemPtr], Value);
      break;
    case 16:
      vmovups(xword [MemPtr], Value);
      break;
    case 32:
      vmovups(yword [MemPtr], ToYMM(Value));
      break;
    default:
      LOGMAN_MSG_A_FMT("Unhandled StoreMem size: {}", OpSize);
      break;
    }
  }
}

DEF_OP(VLoadVectorMasked) {
  const auto Op = IROp->C<IR::IROp_VLoadVectorMasked>();
  const auto OpSize = IROp->Size;

  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;
  const auto ElementSize = IROp->ElementSize;

  const auto Dst = GetDst(Node);
  const auto Mask = GetSrc(Op->Mask.ID());

  const Xbyak::Reg MemReg = GetSrc<RA_64>(Op->Addr.ID());
  const auto MemPtr = GenerateModRM(MemReg, Op->Offset, Op->OffsetType, Op->OffsetScale);

  switch (ElementSize) {
    case 4: {
      if (Is256Bit) {
        vmaskmovps(ToYMM(Dst), ToYMM(Mask), yword [MemPtr]);
      } else {
        vmaskmovps(Dst, Mask, xword [MemPtr]);
      }
      return;
    }
    case 8: {
      if (Is256Bit) {
        vmaskmovpd(ToYMM(Dst), ToYMM(Mask), yword [MemPtr]);
      } else {
        vmaskmovpd(Dst, Mask, xword [MemPtr]);
      }
      return;
    }
    default:
      LOGMAN_MSG_A_FMT("Unhandled VLoadVectorMasked element size: {}", ElementSize);
      return;
  }
}
DEF_OP(VStoreVectorMasked) {
  const auto Op = IROp->C<IR::IROp_VStoreVectorMasked>();
  const auto OpSize = IROp->Size;

  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;
  const auto ElementSize = IROp->ElementSize;

  const auto Data = GetDst(Op->Data.ID());
  const auto Mask = GetSrc(Op->Mask.ID());

  const Xbyak::Reg MemReg = GetSrc<RA_64>(Op->Addr.ID());
  const auto MemPtr = GenerateModRM(MemReg, Op->Offset, Op->OffsetType, Op->OffsetScale);

  switch (ElementSize) {
    case 4: {
      if (Is256Bit) {
        vmaskmovps(yword [MemPtr], ToYMM(Mask), ToYMM(Data));
      } else {
        vmaskmovps(xword [MemPtr], Mask, Data);
      }
      return;
    }
    case 8: {
      if (Is256Bit) {
        vmaskmovpd(yword [MemPtr], ToYMM(Mask), ToYMM(Data));
      } else {
        vmaskmovpd(xword [MemPtr], Mask, Data);
      }
      return;
    }
    default:
      LOGMAN_MSG_A_FMT("Unhandled VStoreVectorMasked element size: {}", ElementSize);
      return;
  }
}

DEF_OP(VBroadcastFromMem) {
  const auto Op = IROp->C<IR::IROp_VBroadcastFromMem>();
  const auto OpSize = IROp->Size;

  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;
  const auto ElementSize = IROp->ElementSize;

  const auto Dst = GetDst(Node);

  const Xbyak::Reg MemReg = GetSrc<RA_64>(Op->Address.ID());

  if (Is256Bit) {
    const auto DstYMM = ToYMM(Dst);

    switch (ElementSize) {
    case 1:
      vpbroadcastb(DstYMM, byte [MemReg]);
      break;
    case 2:
      vpbroadcastw(DstYMM, word [MemReg]);
      break;
    case 4:
      vpbroadcastd(DstYMM, dword [MemReg]);
      break;
    case 8:
      vpbroadcastq(DstYMM, qword [MemReg]);
      break;
    case 16:
      vbroadcasti128(DstYMM, xword [MemReg]);
      break;
    default:
      LOGMAN_MSG_A_FMT("Unhandled VBroadcastFromMem element size: {}", ElementSize);
      return;
    }
  } else {
    switch (ElementSize) {
    case 1:
      vpbroadcastb(Dst, byte [MemReg]);
      break;
    case 2:
      vpbroadcastw(Dst, word [MemReg]);
      break;
    case 4:
      vpbroadcastd(Dst, dword [MemReg]);
      break;
    case 8:
      vpbroadcastq(Dst, qword [MemReg]);
      break;
    case 16:
      vmovaps(Dst, xword [MemReg]);
      break;
    default:
      LOGMAN_MSG_A_FMT("Unhandled VBroadcastFromMem element size: {}", ElementSize);
      return;
    }
  }
}

DEF_OP(Push) {
  const auto Op = IROp->C<IR::IROp_Push>();
  const auto ValueSize = Op->ValueSize;

  const Xbyak::Reg Value = GetSrc<RA_64>(Op->Value.ID());
  const Xbyak::Reg AddrSrc = GetSrc<RA_64>(Op->Addr.ID());
  const auto Dst = GetSrc<RA_64>(Node);

  switch (ValueSize) {
  case 1:
    mov(byte [AddrSrc - ValueSize], Value.cvt8());
    break;
  case 2:
    mov(word [AddrSrc - ValueSize], Value.cvt16());
    break;
  case 4:
    mov(dword [AddrSrc - ValueSize], Value.cvt32());
    break;
  case 8:
    mov(qword [AddrSrc - ValueSize], Value);
    break;
  default:
    LOGMAN_MSG_A_FMT("Unhandled {} size: {}", __func__, ValueSize);
    break;
  }

  lea(Dst, qword [AddrSrc - ValueSize]);
}

DEF_OP(MemSet) {
  const auto Op = IROp->C<IR::IROp_MemSet>();

  const int32_t Size = Op->Size;
  const auto MemReg = GetSrc<RA_64>(Op->Addr.ID());
  const auto Value = GetSrc<RA_64>(Op->Value.ID());
  const auto Length = GetSrc<RA_64>(Op->Length.ID());
  const auto Direction = GetSrc<RA_64>(Op->Direction.ID());
  const auto Dst = GetSrc<RA_64>(Node);

  // If Direction == 0 then:
  //   MemReg is incremented (by size)
  // else:
  //   MemReg is decremented (by size)
  //
  // Counter is decremented regardless.

  // TMP1 = rax
  // TMP2 = rcx
  // TMP4 = rdi
  // That leaves us with TMP3 and TMP5
  mov(rax, Value);
  mov(rcx, Length);
  mov(rdi, MemReg);

  if (!Op->Prefix.IsInvalid()) {
    add(rdi, GetSrc<RA_64>(Op->Prefix.ID()));
  }

  {
    mov(TMP3, Length);
    auto CalculateDest = [&]() {
      mov(Dst, MemReg);
      switch (Size) {
        case 1:
          break;
        case 2:
          shl(TMP3, 1);
          break;
        case 4:
          shl(TMP3, 2);
          break;
        case 8:
          shl(TMP3, 3);
          break;
        default:
          LOGMAN_MSG_A_FMT("Unhandled {} size: {}", __func__, Size);
          break;
      }
    };

    Label AfterDir;
    Label BackwardDir;

    cmp(Direction, 0);
    jne(BackwardDir);
    // Incrementing DF flag.
    cld();
    CalculateDest();
    add(Dst, TMP3);
    jmp(AfterDir);

    L(BackwardDir);
    // Decrementing DF flag.
    std();
    CalculateDest();
    sub(Dst, TMP3);

    L(AfterDir);
  }

  switch (Size) {
    case 1:
      rep(); stosb();
      break;
    case 2:
      rep(); stosw();
      break;
    case 4:
      rep(); stosd();
      break;
    case 8:
      rep(); stosq();
      break;
    default:
      LOGMAN_MSG_A_FMT("Unhandled {} size: {}", __func__, Size);
      break;
  }
  // Ensure we set DF back to zero. Required by the ABI.
  cld();
}

DEF_OP(MemCpy) {
  const auto Op = IROp->C<IR::IROp_MemCpy>();

  const int32_t Size = Op->Size;
  const auto MemRegDest = GetSrc<RA_64>(Op->AddrDest.ID());
  const auto MemRegSrc = GetSrc<RA_64>(Op->AddrSrc.ID());

  const auto Length = GetSrc<RA_64>(Op->Length.ID());
  const auto Direction = GetSrc<RA_64>(Op->Direction.ID());

  // If Direction == 0 then:
  //   MemRegDest is incremented (by size)
  //   MemRegSrc is incremented (by size)
  // else:
  //   MemRegDest is decremented (by size)
  //   MemRegSrc is decremented (by size)
  //
  // Counter is decremented regardless.

  // TMP1 = Length
  // TMP2 = Dest
  // TMP3 = Src
  // TMP4 = Temp value
  mov(TMP1, Length);
  mov(TMP2, MemRegDest);
  mov(TMP3, MemRegSrc);
  if (!Op->PrefixDest.IsInvalid()) {
    add(TMP2, GetSrc<RA_64>(Op->PrefixDest.ID()));
  }
  if (!Op->PrefixSrc.IsInvalid()) {
    add(TMP3, GetSrc<RA_64>(Op->PrefixSrc.ID()));
  }

  auto Dst = GetSrcPair<RA_64>(Node);
  Label Done;
  Label BackwardImpl;
  cmp(Direction, 0);
  jne(BackwardImpl);

  // Emit forward direction memcpy then backward direction memcpy.
  for (int32_t Direction :  { 1, -1 }) {
    Label DoneInternal;
    Label AgainInternal;

    L(AgainInternal);
    cmp(TMP1, 0);
    je(DoneInternal);

    {
      switch (Size) {
        case 1:
          movzx(TMP4, byte [TMP3]);
          mov(byte [TMP2], TMP4.cvt8());
          break;
        case 2:
          movzx(TMP4, word [TMP3]);
          mov(word [TMP2], TMP4.cvt16());
          break;
        case 4:
          mov(TMP4.cvt32(), dword [TMP3]);
          mov(dword [TMP2], TMP4.cvt32());
          break;
        case 8:
          mov(TMP4, qword [TMP3]);
          mov(qword [TMP2], TMP4);
          break;
        default:
          LOGMAN_MSG_A_FMT("Unhandled {} size: {}", __func__, Size);
          break;
      }
    }

    if (Direction == 1) {
      // Incrementing pointers
      add(TMP2, Size);
      add(TMP3, Size);
    }
    else {
      // Decrementing pointers
      sub(TMP2, Size);
      sub(TMP3, Size);
    }

    // Decrement counter by one
    sub(TMP1, 1);

    jmp(AgainInternal);
    L(DoneInternal);

    // Pointer math using source pointers and length.
    mov(TMP3, Length);
    switch (Size) {
      case 1:
        break;
      case 2:
        shl(TMP3, 1);
        break;
      case 4:
        shl(TMP3, 2);
        break;
      case 8:
        shl(TMP3, 3);
        break;
      default:
        LOGMAN_MSG_A_FMT("Unhandled {} size: {}", __func__, Size);
        break;
    }

    // Needs to use temporaries just in case of overwrite
    mov(TMP1, MemRegDest);
    mov(TMP2, MemRegSrc);

    mov(Dst.first, TMP1);
    mov(Dst.second, TMP2);

    if (Direction == 1) {
      // Incrementing pointers
      add(Dst.first, TMP3);
      add(Dst.second, TMP3);

      jmp(Done);
      L(BackwardImpl);
    }
    else {
      // Decrementing pointers
      sub(Dst.first, TMP3);
      sub(Dst.second, TMP3);
    }
  }

  L(Done);
}

DEF_OP(CacheLineClear) {
  auto Op = IROp->C<IR::IROp_CacheLineClear>();

  Xbyak::Reg MemReg = GetSrc<RA_64>(Op->Addr.ID());

  if (Op->Serialize) {
    clflush(ptr [MemReg]);
  }
  else {
    clflushopt(ptr [MemReg]);
  }
}

DEF_OP(CacheLineClean) {
  auto Op = IROp->C<IR::IROp_CacheLineClean>();

  Xbyak::Reg MemReg = GetSrc<RA_64>(Op->Addr.ID());
  clwb(ptr [MemReg]);
}

DEF_OP(CacheLineZero) {
  auto Op = IROp->C<IR::IROp_CacheLineZero>();

  Xbyak::Reg MemReg = GetSrc<RA_64>(Op->Addr.ID());

  // Align by cacheline
  mov (TMP1, CPUIDEmu::CACHELINE_SIZE - 1);
  andn(TMP1, TMP1, MemReg.cvt64());
  xor_(TMP2, TMP2);

  using DataType = uint64_t;
  // 64-byte cache line zero
  for (size_t i = 0; i < CPUIDEmu::CACHELINE_SIZE; i += sizeof(DataType)) {
    mov (qword [TMP1 + i], TMP2);
  }
}

#undef DEF_OP
void X86JITCore::RegisterMemoryHandlers() {
#define REGISTER_OP(op, x) OpHandlers[FEXCore::IR::IROps::OP_##op] = &X86JITCore::Op_##x
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
  REGISTER_OP(LOADMEMTSO,          LoadMem);
  REGISTER_OP(STOREMEMTSO,         StoreMem);
  REGISTER_OP(VLOADVECTORMASKED,   VLoadVectorMasked);
  REGISTER_OP(VSTOREVECTORMASKED,  VStoreVectorMasked);
  REGISTER_OP(VBROADCASTFROMMEM,   VBroadcastFromMem);
  REGISTER_OP(PUSH,                Push);
  REGISTER_OP(MEMSET,              MemSet);
  REGISTER_OP(MEMCPY,              MemCpy);
  REGISTER_OP(CACHELINECLEAR,      CacheLineClear);
  REGISTER_OP(CACHELINECLEAN,      CacheLineClean);
  REGISTER_OP(CACHELINEZERO,       CacheLineZero);
#undef REGISTER_OP
}
}

