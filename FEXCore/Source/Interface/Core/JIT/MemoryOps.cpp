// SPDX-License-Identifier: MIT
/*
$info$
tags: backend|arm64
$end_info$
*/

#include "FEXCore/Core/X86Enums.h"
#include "FEXCore/Utils/LogManager.h"
#include "Interface/Context/Context.h"
#include "Interface/Core/ArchHelpers/Arm64Emitter.h"
#include "Interface/Core/CPUID.h"
#include "Interface/Core/JIT/JITClass.h"
#include "Interface/IR/RegisterAllocationData.h"
#include <FEXCore/Utils/CompilerDefs.h>
#include <FEXCore/Utils/MathUtils.h>

namespace FEXCore::CPU {

DEF_OP(LoadContext) {
  const auto Op = IROp->C<IR::IROp_LoadContext>();
  const auto OpSize = IROp->Size;

  if (Op->Class == IR::RegClass::GPR) {
    auto Dst = GetReg(Node);

    switch (OpSize) {
    case IR::OpSize::i8Bit: ldrb(Dst, STATE, Op->Offset); break;
    case IR::OpSize::i16Bit: ldrh(Dst, STATE, Op->Offset); break;
    case IR::OpSize::i32Bit: ldr(Dst.W(), STATE, Op->Offset); break;
    case IR::OpSize::i64Bit: ldr(Dst.X(), STATE, Op->Offset); break;
    default: LOGMAN_MSG_A_FMT("Unhandled LoadContext size: {}", OpSize); break;
    }
  } else {
    auto Dst = GetVReg(Node);

    switch (OpSize) {
    case IR::OpSize::i8Bit: ldrb(Dst, STATE, Op->Offset); break;
    case IR::OpSize::i16Bit: ldrh(Dst, STATE, Op->Offset); break;
    case IR::OpSize::i32Bit: ldr(Dst.S(), STATE, Op->Offset); break;
    case IR::OpSize::i64Bit: ldr(Dst.D(), STATE, Op->Offset); break;
    case IR::OpSize::i128Bit: ldr(Dst.Q(), STATE, Op->Offset); break;
    case IR::OpSize::i256Bit:
      mov(TMP1, Op->Offset);
      ld1b<ARMEmitter::SubRegSize::i8Bit>(Dst.Z(), PRED_TMP_32B.Zeroing(), STATE, TMP1);
      break;
    default: LOGMAN_MSG_A_FMT("Unhandled LoadContext size: {}", OpSize); break;
    }
  }
}

DEF_OP(LoadContextPair) {
  const auto Op = IROp->C<IR::IROp_LoadContextPair>();

  if (Op->Class == IR::RegClass::GPR) {
    const auto Dst1 = GetReg(Op->OutValue1);
    const auto Dst2 = GetReg(Op->OutValue2);

    switch (IROp->Size) {
    case IR::OpSize::i32Bit: ldp<ARMEmitter::IndexType::OFFSET>(Dst1.W(), Dst2.W(), STATE, Op->Offset); break;
    case IR::OpSize::i64Bit: ldp<ARMEmitter::IndexType::OFFSET>(Dst1.X(), Dst2.X(), STATE, Op->Offset); break;
    default: LOGMAN_MSG_A_FMT("Unhandled LoadMemPair size: {}", IROp->Size); break;
    }
  } else {
    const auto Dst1 = GetVReg(Op->OutValue1);
    const auto Dst2 = GetVReg(Op->OutValue2);

    switch (IROp->Size) {
    case IR::OpSize::i32Bit: ldp<ARMEmitter::IndexType::OFFSET>(Dst1.S(), Dst2.S(), STATE, Op->Offset); break;
    case IR::OpSize::i64Bit: ldp<ARMEmitter::IndexType::OFFSET>(Dst1.D(), Dst2.D(), STATE, Op->Offset); break;
    case IR::OpSize::i128Bit: ldp<ARMEmitter::IndexType::OFFSET>(Dst1.Q(), Dst2.Q(), STATE, Op->Offset); break;
    default: LOGMAN_MSG_A_FMT("Unhandled LoadMemPair size: {}", IROp->Size); break;
    }
  }
}

DEF_OP(StoreContext) {
  const auto Op = IROp->C<IR::IROp_StoreContext>();
  const auto OpSize = IROp->Size;

  if (Op->Class == IR::RegClass::GPR) {
    auto Src = GetZeroableReg(Op->Value);

    switch (OpSize) {
    case IR::OpSize::i8Bit: strb(Src, STATE, Op->Offset); break;
    case IR::OpSize::i16Bit: strh(Src, STATE, Op->Offset); break;
    case IR::OpSize::i32Bit: str(Src.W(), STATE, Op->Offset); break;
    case IR::OpSize::i64Bit: str(Src.X(), STATE, Op->Offset); break;
    default: LOGMAN_MSG_A_FMT("Unhandled StoreContext size: {}", OpSize); break;
    }
  } else {
    const auto Src = GetVReg(Op->Value);

    switch (OpSize) {
    case IR::OpSize::i8Bit: strb(Src, STATE, Op->Offset); break;
    case IR::OpSize::i16Bit: strh(Src, STATE, Op->Offset); break;
    case IR::OpSize::i32Bit: str(Src.S(), STATE, Op->Offset); break;
    case IR::OpSize::i64Bit: str(Src.D(), STATE, Op->Offset); break;
    case IR::OpSize::i128Bit: str(Src.Q(), STATE, Op->Offset); break;
    case IR::OpSize::i256Bit:
      mov(TMP1, Op->Offset);
      st1b<ARMEmitter::SubRegSize::i8Bit>(Src.Z(), PRED_TMP_32B, STATE, TMP1);
      break;
    default: LOGMAN_MSG_A_FMT("Unhandled StoreContext size: {}", OpSize); break;
    }
  }
}

DEF_OP(StoreContextPair) {
  const auto Op = IROp->C<IR::IROp_StoreContextPair>();
  const auto OpSize = IROp->Size;

  if (Op->Class == IR::RegClass::GPR) {
    auto Src1 = GetZeroableReg(Op->Value1);
    auto Src2 = GetZeroableReg(Op->Value2);

    switch (OpSize) {
    case IR::OpSize::i32Bit: stp<ARMEmitter::IndexType::OFFSET>(Src1.W(), Src2.W(), STATE, Op->Offset); break;
    case IR::OpSize::i64Bit: stp<ARMEmitter::IndexType::OFFSET>(Src1.X(), Src2.X(), STATE, Op->Offset); break;
    default: LOGMAN_MSG_A_FMT("Unhandled StoreContext size: {}", OpSize); break;
    }
  } else {
    const auto Src1 = GetVReg(Op->Value1);
    const auto Src2 = GetVReg(Op->Value2);

    switch (OpSize) {
    case IR::OpSize::i32Bit: stp<ARMEmitter::IndexType::OFFSET>(Src1.S(), Src2.S(), STATE, Op->Offset); break;
    case IR::OpSize::i64Bit: stp<ARMEmitter::IndexType::OFFSET>(Src1.D(), Src2.D(), STATE, Op->Offset); break;
    case IR::OpSize::i128Bit: stp<ARMEmitter::IndexType::OFFSET>(Src1.Q(), Src2.Q(), STATE, Op->Offset); break;
    default: LOGMAN_MSG_A_FMT("Unhandled StoreContextPair size: {}", OpSize); break;
    }
  }
}

DEF_OP(LoadRegister) {
  const auto Op = IROp->C<IR::IROp_LoadRegister>();

  if (Op->Class == IR::RegClass::GPR) {
    LOGMAN_THROW_A_FMT(Op->Reg < StaticRegisters.size(), "out of range reg");

    mov(GetReg(Node).X(), StaticRegisters[Op->Reg].X());
  } else if (Op->Class == IR::RegClass::FPR) {
    const auto regSize = HostSupportsAVX256 ? IR::OpSize::i256Bit : IR::OpSize::i128Bit;
    LOGMAN_THROW_A_FMT(Op->Reg < StaticFPRegisters.size(), "out of range reg");
    LOGMAN_THROW_A_FMT(IROp->Size == regSize, "expected sized");

    const auto guest = StaticFPRegisters[Op->Reg];
    const auto host = GetVReg(Node);

    if (HostSupportsAVX256) {
      mov(ARMEmitter::SubRegSize::i64Bit, host.Z(), PRED_TMP_32B.Merging(), guest.Z());
    } else {
      mov(host.Q(), guest.Q());
    }
  } else {
    LOGMAN_THROW_A_FMT(false, "Unhandled Op->Class {}", Op->Class);
  }
}

DEF_OP(LoadPF) {
  const auto reg = StaticRegisters[StaticRegisters.size() - 2];

  if (GetReg(Node).Idx() != reg.Idx()) {
    mov(GetReg(Node).X(), reg.X());
  }
}

DEF_OP(LoadAF) {
  const auto reg = StaticRegisters[StaticRegisters.size() - 1];

  if (GetReg(Node).Idx() != reg.Idx()) {
    mov(GetReg(Node).X(), reg.X());
  }
}

DEF_OP(StoreRegister) {
  const auto Op = IROp->C<IR::IROp_StoreRegister>();
  const auto Reg = IR::PhysicalRegister(Node);
  const auto RegClass = Reg.AsRegClass();

  if (RegClass == IR::RegClass::GPRFixed) {
    // Always use 64-bit, it's faster. Upper bits ignored for 32-bit mode.
    mov(ARMEmitter::Size::i64Bit, GetReg(Reg), GetReg(Op->Value));
  } else if (RegClass == IR::RegClass::FPRFixed) {
    const auto regSize = HostSupportsAVX256 ? IR::OpSize::i256Bit : IR::OpSize::i128Bit;
    LOGMAN_THROW_A_FMT(IROp->Size == regSize, "expected sized");

    const auto guest = GetVReg(Reg);
    const auto host = GetVReg(Op->Value);

    if (HostSupportsAVX256) {
      mov(ARMEmitter::SubRegSize::i64Bit, guest.Z(), PRED_TMP_32B.Merging(), host.Z());
    } else {
      mov(guest.Q(), host.Q());
    }
  } else {
    LOGMAN_THROW_A_FMT(false, "Unhandled Op->Class {}", RegClass);
  }
}

DEF_OP(StorePF) {
  const auto Op = IROp->C<IR::IROp_StorePF>();
  const auto reg = StaticRegisters[StaticRegisters.size() - 2];
  const auto Src = GetReg(Op->Value);

  if (Src.Idx() != reg.Idx()) {
    // Always use 64-bit, it's faster. Upper bits ignored for 32-bit mode.
    mov(ARMEmitter::Size::i64Bit, reg, Src);
  }
}

DEF_OP(StoreAF) {
  const auto Op = IROp->C<IR::IROp_StoreAF>();
  const auto reg = StaticRegisters[StaticRegisters.size() - 1];
  const auto Src = GetReg(Op->Value);

  if (Src.Idx() != reg.Idx()) {
    // Always use 64-bit, it's faster. Upper bits ignored for 32-bit mode.
    mov(ARMEmitter::Size::i64Bit, reg, Src);
  }
}

DEF_OP(LoadContextIndexed) {
  const auto Op = IROp->C<IR::IROp_LoadContextIndexed>();
  const auto OpSize = IROp->Size;

  const auto Index = GetReg(Op->Index);

  if (Op->Class == IR::RegClass::GPR) {
    switch (Op->Stride) {
    case 1:
    case 2:
    case 4:
    case 8: {
      add(ARMEmitter::Size::i64Bit, TMP1, STATE, Index, ARMEmitter::ShiftType::LSL, FEXCore::ilog2(Op->Stride));
      const auto Dst = GetReg(Node);
      switch (OpSize) {
      case IR::OpSize::i8Bit: ldrb(Dst, TMP1, Op->BaseOffset); break;
      case IR::OpSize::i16Bit: ldrh(Dst, TMP1, Op->BaseOffset); break;
      case IR::OpSize::i32Bit: ldr(Dst.W(), TMP1, Op->BaseOffset); break;
      case IR::OpSize::i64Bit: ldr(Dst.X(), TMP1, Op->BaseOffset); break;
      default: LOGMAN_MSG_A_FMT("Unhandled LoadContextIndexed size: {}", OpSize); break;
      }
      break;
    }
    case 16: LOGMAN_MSG_A_FMT("Invalid Class load of size 16"); break;
    default: LOGMAN_MSG_A_FMT("Unhandled LoadContextIndexed stride: {}", Op->Stride); break;
    }
  } else {
    switch (Op->Stride) {
    case 1:
    case 2:
    case 4:
    case 8:
    case 16:
    case 32: {
      add(ARMEmitter::Size::i64Bit, TMP1, STATE, Index, ARMEmitter::ShiftType::LSL, FEXCore::ilog2(Op->Stride));
      const auto Dst = GetVReg(Node);

      switch (OpSize) {
      case IR::OpSize::i8Bit: ldrb(Dst, TMP1, Op->BaseOffset); break;
      case IR::OpSize::i16Bit: ldrh(Dst, TMP1, Op->BaseOffset); break;
      case IR::OpSize::i32Bit: ldr(Dst.S(), TMP1, Op->BaseOffset); break;
      case IR::OpSize::i64Bit: ldr(Dst.D(), TMP1, Op->BaseOffset); break;
      case IR::OpSize::i128Bit:
        if (Op->BaseOffset % 16 == 0) {
          ldr(Dst.Q(), TMP1, Op->BaseOffset);
        } else {
          add(ARMEmitter::Size::i64Bit, TMP1, TMP1, Op->BaseOffset);
          ldur(Dst.Q(), TMP1, Op->BaseOffset);
        }
        break;
      case IR::OpSize::i256Bit:
        mov(TMP2, Op->BaseOffset);
        ld1b<ARMEmitter::SubRegSize::i8Bit>(Dst.Z(), PRED_TMP_32B.Zeroing(), TMP1, TMP2);
        break;
      default: LOGMAN_MSG_A_FMT("Unhandled LoadContextIndexed size: {}", OpSize); break;
      }
      break;
    }
    default: LOGMAN_MSG_A_FMT("Unhandled LoadContextIndexed stride: {}", Op->Stride); break;
    }
  }
}

DEF_OP(StoreContextIndexed) {
  const auto Op = IROp->C<IR::IROp_StoreContextIndexed>();
  const auto OpSize = IROp->Size;

  const auto Index = GetReg(Op->Index);

  if (Op->Class == IR::RegClass::GPR) {
    const auto Value = GetReg(Op->Value);

    switch (Op->Stride) {
    case 1:
    case 2:
    case 4:
    case 8: {
      add(ARMEmitter::Size::i64Bit, TMP1, STATE, Index, ARMEmitter::ShiftType::LSL, FEXCore::ilog2(Op->Stride));

      switch (OpSize) {
      case IR::OpSize::i8Bit: strb(Value, TMP1, Op->BaseOffset); break;
      case IR::OpSize::i16Bit: strh(Value, TMP1, Op->BaseOffset); break;
      case IR::OpSize::i32Bit: str(Value.W(), TMP1, Op->BaseOffset); break;
      case IR::OpSize::i64Bit: str(Value.X(), TMP1, Op->BaseOffset); break;
      default: LOGMAN_MSG_A_FMT("Unhandled StoreContextIndexed size: {}", OpSize); break;
      }
      break;
    }
    case 16: LOGMAN_MSG_A_FMT("Invalid Class store of size 16"); break;
    default: LOGMAN_MSG_A_FMT("Unhandled StoreContextIndexed stride: {}", Op->Stride); break;
    }
  } else {
    const auto Value = GetVReg(Op->Value);

    switch (Op->Stride) {
    case 1:
    case 2:
    case 4:
    case 8:
    case 16:
    case 32: {
      add(ARMEmitter::Size::i64Bit, TMP1, STATE, Index, ARMEmitter::ShiftType::LSL, FEXCore::ilog2(Op->Stride));

      switch (OpSize) {
      case IR::OpSize::i8Bit: strb(Value, TMP1, Op->BaseOffset); break;
      case IR::OpSize::i16Bit: strh(Value, TMP1, Op->BaseOffset); break;
      case IR::OpSize::i32Bit: str(Value.S(), TMP1, Op->BaseOffset); break;
      case IR::OpSize::i64Bit: str(Value.D(), TMP1, Op->BaseOffset); break;
      case IR::OpSize::i128Bit:
        if (Op->BaseOffset % 16 == 0) {
          str(Value.Q(), TMP1, Op->BaseOffset);
        } else {
          add(ARMEmitter::Size::i64Bit, TMP1, TMP1, Op->BaseOffset);
          stur(Value.Q(), TMP1, Op->BaseOffset);
        }
        break;
      case IR::OpSize::i256Bit:
        mov(TMP2, Op->BaseOffset);
        st1b<ARMEmitter::SubRegSize::i8Bit>(Value.Z(), PRED_TMP_32B, TMP1, TMP2);
        break;
      default: LOGMAN_MSG_A_FMT("Unhandled StoreContextIndexed size: {}", OpSize); break;
      }
      break;
    }
    default: LOGMAN_MSG_A_FMT("Unhandled StoreContextIndexed stride: {}", Op->Stride); break;
    }
  }
}

DEF_OP(FormContextAddress) {
  const auto Op = IROp->C<IR::IROp_FormContextAddress>();
  const auto Index = GetReg(Op->Index);
  const auto Dst = GetReg(Node);

  switch (Op->Stride) {
  case 1:
  case 2:
  case 4:
  case 8:
  case 16:
  case 32: {
    add(ARMEmitter::Size::i64Bit, Dst, STATE, Index, ARMEmitter::ShiftType::LSL, FEXCore::ilog2(Op->Stride));
    break;
  }
  default: LOGMAN_MSG_A_FMT("Unhandled FormContextAddress stride: {}", Op->Stride); break;
  }
}

DEF_OP(SpillRegister) {
  const auto Op = IROp->C<IR::IROp_SpillRegister>();
  const auto OpSize = IROp->Size;
  const uint32_t SlotOffset = Op->Slot * MaxSpillSlotSize;

  if (Op->Class == IR::RegClass::GPR) {
    const auto Src = GetReg(Op->Value);
    switch (OpSize) {
    case IR::OpSize::i8Bit: {
      if (SlotOffset > LSByteMaxUnsignedOffset) {
        LoadConstant(ARMEmitter::Size::i64Bit, TMP1, SlotOffset);
        strb(Src, ARMEmitter::Reg::rsp, TMP1.R(), ARMEmitter::ExtendedType::LSL_64, 0);
      } else {
        strb(Src, ARMEmitter::Reg::rsp, SlotOffset);
      }
      break;
    }
    case IR::OpSize::i16Bit: {
      if (SlotOffset > LSHalfMaxUnsignedOffset) {
        LoadConstant(ARMEmitter::Size::i64Bit, TMP1, SlotOffset);
        strh(Src, ARMEmitter::Reg::rsp, TMP1.R(), ARMEmitter::ExtendedType::LSL_64, 0);
      } else {
        strh(Src, ARMEmitter::Reg::rsp, SlotOffset);
      }
      break;
    }
    case IR::OpSize::i32Bit: {
      if (SlotOffset > LSWordMaxUnsignedOffset) {
        LoadConstant(ARMEmitter::Size::i64Bit, TMP1, SlotOffset);
        str(Src.W(), ARMEmitter::Reg::rsp, TMP1.R(), ARMEmitter::ExtendedType::LSL_64, 0);
      } else {
        str(Src.W(), ARMEmitter::Reg::rsp, SlotOffset);
      }
      break;
    }
    case IR::OpSize::i64Bit: {
      if (SlotOffset > LSDWordMaxUnsignedOffset) {
        LoadConstant(ARMEmitter::Size::i64Bit, TMP1, SlotOffset);
        str(Src.X(), ARMEmitter::Reg::rsp, TMP1.R(), ARMEmitter::ExtendedType::LSL_64, 0);
      } else {
        str(Src.X(), ARMEmitter::Reg::rsp, SlotOffset);
      }
      break;
    }
    default: LOGMAN_MSG_A_FMT("Unhandled SpillRegister size: {}", OpSize); break;
    }
  } else if (Op->Class == FEXCore::IR::RegClass::FPR) {
    const auto Src = GetVReg(Op->Value);

    switch (OpSize) {
    case IR::OpSize::i32Bit: {
      if (SlotOffset > LSWordMaxUnsignedOffset) {
        LoadConstant(ARMEmitter::Size::i64Bit, TMP1, SlotOffset);
        str(Src.S(), ARMEmitter::Reg::rsp, TMP1.R(), ARMEmitter::ExtendedType::LSL_64, 0);
      } else {
        str(Src.S(), ARMEmitter::Reg::rsp, SlotOffset);
      }
      break;
    }
    case IR::OpSize::i64Bit: {
      if (SlotOffset > LSDWordMaxUnsignedOffset) {
        LoadConstant(ARMEmitter::Size::i64Bit, TMP1, SlotOffset);
        str(Src.D(), ARMEmitter::Reg::rsp, TMP1.R(), ARMEmitter::ExtendedType::LSL_64, 0);
      } else {
        str(Src.D(), ARMEmitter::Reg::rsp, SlotOffset);
      }
      break;
    }
    case IR::OpSize::i128Bit: {
      if (SlotOffset > LSQWordMaxUnsignedOffset) {
        LoadConstant(ARMEmitter::Size::i64Bit, TMP1, SlotOffset);
        str(Src.Q(), ARMEmitter::Reg::rsp, TMP1.R(), ARMEmitter::ExtendedType::LSL_64, 0);
      } else {
        str(Src.Q(), ARMEmitter::Reg::rsp, SlotOffset);
      }
      break;
    }
    case IR::OpSize::i256Bit: {
      mov(TMP3, SlotOffset);
      st1b<ARMEmitter::SubRegSize::i8Bit>(Src.Z(), PRED_TMP_32B, ARMEmitter::Reg::rsp, TMP3);
      break;
    }
    default: LOGMAN_MSG_A_FMT("Unhandled SpillRegister size: {}", OpSize); break;
    }
  } else {
    LOGMAN_MSG_A_FMT("Unhandled SpillRegister class: {}", Op->Class);
  }
}

DEF_OP(FillRegister) {
  const auto Op = IROp->C<IR::IROp_FillRegister>();
  const auto OpSize = IROp->Size;
  const uint32_t SlotOffset = Op->Slot * MaxSpillSlotSize;

  if (Op->Class == IR::RegClass::GPR) {
    const auto Dst = GetReg(Node);
    switch (OpSize) {
    case IR::OpSize::i8Bit: {
      if (SlotOffset > LSByteMaxUnsignedOffset) {
        LoadConstant(ARMEmitter::Size::i64Bit, TMP1, SlotOffset);
        ldrb(Dst, ARMEmitter::Reg::rsp, TMP1.R(), ARMEmitter::ExtendedType::LSL_64, 0);
      } else {
        ldrb(Dst, ARMEmitter::Reg::rsp, SlotOffset);
      }
      break;
    }
    case IR::OpSize::i16Bit: {
      if (SlotOffset > LSHalfMaxUnsignedOffset) {
        LoadConstant(ARMEmitter::Size::i64Bit, TMP1, SlotOffset);
        ldrh(Dst, ARMEmitter::Reg::rsp, TMP1.R(), ARMEmitter::ExtendedType::LSL_64, 0);
      } else {
        ldrh(Dst, ARMEmitter::Reg::rsp, SlotOffset);
      }
      break;
    }
    case IR::OpSize::i32Bit: {
      if (SlotOffset > LSWordMaxUnsignedOffset) {
        LoadConstant(ARMEmitter::Size::i64Bit, TMP1, SlotOffset);
        ldr(Dst.W(), ARMEmitter::Reg::rsp, TMP1.R(), ARMEmitter::ExtendedType::LSL_64, 0);
      } else {
        ldr(Dst.W(), ARMEmitter::Reg::rsp, SlotOffset);
      }
      break;
    }
    case IR::OpSize::i64Bit: {
      if (SlotOffset > LSDWordMaxUnsignedOffset) {
        LoadConstant(ARMEmitter::Size::i64Bit, TMP1, SlotOffset);
        ldr(Dst.X(), ARMEmitter::Reg::rsp, TMP1.R(), ARMEmitter::ExtendedType::LSL_64, 0);
      } else {
        ldr(Dst.X(), ARMEmitter::Reg::rsp, SlotOffset);
      }
      break;
    }
    default: LOGMAN_MSG_A_FMT("Unhandled FillRegister size: {}", OpSize); break;
    }
  } else if (Op->Class == FEXCore::IR::RegClass::FPR) {
    const auto Dst = GetVReg(Node);

    switch (OpSize) {
    case IR::OpSize::i32Bit: {
      if (SlotOffset > LSWordMaxUnsignedOffset) {
        LoadConstant(ARMEmitter::Size::i64Bit, TMP1, SlotOffset);
        ldr(Dst.S(), ARMEmitter::Reg::rsp, TMP1.R(), ARMEmitter::ExtendedType::LSL_64, 0);
      } else {
        ldr(Dst.S(), ARMEmitter::Reg::rsp, SlotOffset);
      }
      break;
    }
    case IR::OpSize::i64Bit: {
      if (SlotOffset > LSDWordMaxUnsignedOffset) {
        LoadConstant(ARMEmitter::Size::i64Bit, TMP1, SlotOffset);
        ldr(Dst.D(), ARMEmitter::Reg::rsp, TMP1.R(), ARMEmitter::ExtendedType::LSL_64, 0);
      } else {
        ldr(Dst.D(), ARMEmitter::Reg::rsp, SlotOffset);
      }
      break;
    }
    case IR::OpSize::i128Bit: {
      if (SlotOffset > LSQWordMaxUnsignedOffset) {
        LoadConstant(ARMEmitter::Size::i64Bit, TMP1, SlotOffset);
        ldr(Dst.Q(), ARMEmitter::Reg::rsp, TMP1.R(), ARMEmitter::ExtendedType::LSL_64, 0);
      } else {
        ldr(Dst.Q(), ARMEmitter::Reg::rsp, SlotOffset);
      }
      break;
    }
    case IR::OpSize::i256Bit: {
      mov(TMP3, SlotOffset);
      ld1b<ARMEmitter::SubRegSize::i8Bit>(Dst.Z(), PRED_TMP_32B.Zeroing(), ARMEmitter::Reg::rsp, TMP3);
      break;
    }
    default: LOGMAN_MSG_A_FMT("Unhandled FillRegister size: {}", OpSize); break;
    }
  } else {
    LOGMAN_MSG_A_FMT("Unhandled FillRegister class: {}", Op->Class);
  }
}

DEF_OP(LoadNZCV) {
  auto Dst = GetReg(Node);

  mrs(Dst, ARMEmitter::SystemRegister::NZCV);
}

DEF_OP(StoreNZCV) {
  auto Op = IROp->C<IR::IROp_StoreNZCV>();

  msr(ARMEmitter::SystemRegister::NZCV, GetReg(Op->Value));
}

DEF_OP(LoadDF) {
  auto Dst = GetReg(Node);
  auto Flag = X86State::RFLAG_DF_RAW_LOC;

  // DF needs sign extension to turn 0x1/0xFF into 1/-1
  ldrsb(Dst.X(), STATE, offsetof(FEXCore::Core::CPUState, flags[Flag]));
}

DEF_OP(ContextClear) {
  auto Op = IROp->C<IR::IROp_ContextClear>();
  if (CTX->HostFeatures.SupportsCLZERO) {
    // We can use CLZero directly when hardware supports it.
    // Provides a fairly generous speed-up on Ampere1A hardware.
    // TODO: When FEAT_MOPS hardware ships, test memset using MOPS.
    for (size_t i = 0; i < Op->Size; i += 64) {
      add(ARMEmitter::Size::i64Bit, TMP1, STATE.R(), Op->Offset + i);
      dc(ARMEmitter::DataCacheOperation::ZVA, TMP1);
    }
  } else {
    movi(ARMEmitter::SubRegSize::i64Bit, VTMP1.Q(), 0);
    for (size_t i = 0; i < Op->Size; i += 32) {
      stp<ARMEmitter::IndexType::OFFSET>(VTMP1.Q(), VTMP1.Q(), STATE.R(), Op->Offset + i);
    }
  }
}

ARMEmitter::ExtendedMemOperand Arm64JITCore::GenerateMemOperand(
  IR::OpSize AccessSize, ARMEmitter::Register Base, IR::OrderedNodeWrapper Offset, IR::MemOffsetType OffsetType, uint8_t OffsetScale) {
  if (Offset.IsInvalid()) {
    return ARMEmitter::ExtendedMemOperand(Base.X(), ARMEmitter::IndexType::OFFSET, 0);
  } else {
    if (OffsetScale != 1 && OffsetScale != IR::OpSizeToSize(AccessSize)) {
      LOGMAN_MSG_A_FMT("Unhandled GenerateMemOperand OffsetScale: {}", OffsetScale);
    }
    uint64_t Const;
    if (IsInlineConstant(Offset, &Const)) {
      return ARMEmitter::ExtendedMemOperand(Base.X(), ARMEmitter::IndexType::OFFSET, Const);
    } else {
      auto RegOffset = GetReg(Offset);
      switch (OffsetType) {
      case IR::MemOffsetType::SXTX:
        return ARMEmitter::ExtendedMemOperand(Base.X(), RegOffset.X(), ARMEmitter::ExtendedType::SXTX, FEXCore::ilog2(OffsetScale));
      case IR::MemOffsetType::UXTW:
        return ARMEmitter::ExtendedMemOperand(Base.X(), RegOffset.X(), ARMEmitter::ExtendedType::UXTW, FEXCore::ilog2(OffsetScale));
      case IR::MemOffsetType::SXTW:
        return ARMEmitter::ExtendedMemOperand(Base.X(), RegOffset.X(), ARMEmitter::ExtendedType::SXTW, FEXCore::ilog2(OffsetScale));
      default: LOGMAN_MSG_A_FMT("Unhandled GenerateMemOperand OffsetType: {}", OffsetType); break;
      }
    }
  }

  FEX_UNREACHABLE;
}

ARMEmitter::Register Arm64JITCore::ApplyMemOperand(IR::OpSize AccessSize, ARMEmitter::Register Base, ARMEmitter::Register Tmp,
                                                   IR::OrderedNodeWrapper Offset, IR::MemOffsetType OffsetType, uint8_t OffsetScale) {
  if (Offset.IsInvalid()) {
    return Base;
  }

  if (OffsetScale != 1 && OffsetScale != IR::OpSizeToSize(AccessSize)) {
    LOGMAN_MSG_A_FMT("Unhandled OffsetScale: {}", OffsetScale);
  }

  uint64_t Const;
  if (IsInlineConstant(Offset, &Const)) {
    if (Const == 0) {
      return Base;
    }
    LoadConstant(ARMEmitter::Size::i64Bit, Tmp, Const);
    add(ARMEmitter::Size::i64Bit, Tmp, Base, Tmp, ARMEmitter::ShiftType::LSL, FEXCore::ilog2(OffsetScale));
  } else {
    auto RegOffset = GetReg(Offset);
    switch (OffsetType) {
    case IR::MemOffsetType::SXTX:
      add(ARMEmitter::Size::i64Bit, Tmp, Base, RegOffset, ARMEmitter::ExtendedType::SXTX, FEXCore::ilog2(OffsetScale));
      break;

    case IR::MemOffsetType::UXTW:
      add(ARMEmitter::Size::i64Bit, Tmp, Base, RegOffset, ARMEmitter::ExtendedType::UXTW, FEXCore::ilog2(OffsetScale));
      break;

    case IR::MemOffsetType::SXTW:
      add(ARMEmitter::Size::i64Bit, Tmp, Base, RegOffset, ARMEmitter::ExtendedType::SXTW, FEXCore::ilog2(OffsetScale));
      break;

    default: LOGMAN_MSG_A_FMT("Unhandled OffsetType: {}", OffsetType); break;
    }
  }
  return Tmp;
}

ARMEmitter::SVEMemOperand Arm64JITCore::GenerateSVEMemOperand(IR::OpSize AccessSize, ARMEmitter::Register Base, IR::OrderedNodeWrapper Offset,
                                                              IR::MemOffsetType OffsetType, [[maybe_unused]] uint8_t OffsetScale) {
  if (Offset.IsInvalid()) {
    return ARMEmitter::SVEMemOperand(Base.X(), 0);
  }

  uint64_t Const {};
  if (IsInlineConstant(Offset, &Const)) {
    if (Const == 0) {
      return ARMEmitter::SVEMemOperand(Base.X(), 0);
    }

    const auto SignedConst = static_cast<int64_t>(Const);
    const auto SignedSVESize = static_cast<int64_t>(HostSupportsSVE256 ? Core::CPUState::XMM_AVX_REG_SIZE : Core::CPUState::XMM_SSE_REG_SIZE);

    const auto IsCleanlyDivisible = (SignedConst % SignedSVESize) == 0;
    const auto Index = SignedConst / SignedSVESize;

    // SVE's immediate variants of load stores are quite limited in terms
    // of immediate range. They also operate on a by-vector-length basis.
    //
    // e.g. On a 256-bit SVE capable system:
    //
    //      LD1B Dst.B, Predicate/Z, [Reg, #1, MUL VL]
    //
    //      Will add 32 to the base register as the offset
    //
    // So if we have a constant that cleanly lies along a 256-bit offset
    // and is also within the limitations of the immediate of -8 to 7
    // then we can encode it as an immediate offset.
    //
    if (IsCleanlyDivisible && Index >= -8 && Index <= 7) {
      return ARMEmitter::SVEMemOperand(Base.X(), static_cast<uint64_t>(Index));
    }

    // If we can't do that for whatever reason, then unfortunately, we need
    // to move it over to a temporary to use as an offset.
    mov(TMP1, Const);
    return ARMEmitter::SVEMemOperand(Base.X(), TMP1);
  }

  // Otherwise handle it like normal.
  // Note that we do nothing with the offset type and offset scale,
  // since SVE loads and stores don't have the ability to perform an
  // optional extension or shift as part of their behavior.
  LOGMAN_THROW_A_FMT(OffsetType == IR::MemOffsetType::SXTX, "Currently only the default offset type (SXTX) is supported.");

  const auto RegOffset = GetReg(Offset);
  return ARMEmitter::SVEMemOperand(Base.X(), RegOffset.X());
}

DEF_OP(LoadMem) {
  const auto Op = IROp->C<IR::IROp_LoadMem>();
  const auto OpSize = IROp->Size;

  const auto MemReg = GetReg(Op->Addr);
  const auto MemSrc = GenerateMemOperand(OpSize, MemReg, Op->Offset, Op->OffsetType, Op->OffsetScale);

  if (Op->Class == IR::RegClass::GPR) {
    const auto Dst = GetReg(Node);

    switch (OpSize) {
    case IR::OpSize::i8Bit: ldrb(Dst, MemSrc); break;
    case IR::OpSize::i16Bit: ldrh(Dst, MemSrc); break;
    case IR::OpSize::i32Bit: ldr(Dst.W(), MemSrc); break;
    case IR::OpSize::i64Bit: ldr(Dst.X(), MemSrc); break;
    default: LOGMAN_MSG_A_FMT("Unhandled LoadMem size: {}", OpSize); break;
    }
  } else {
    const auto Dst = GetVReg(Node);

    switch (OpSize) {
    case IR::OpSize::i8Bit: ldrb(Dst, MemSrc); break;
    case IR::OpSize::i16Bit: ldrh(Dst, MemSrc); break;
    case IR::OpSize::i32Bit: ldr(Dst.S(), MemSrc); break;
    case IR::OpSize::i64Bit: ldr(Dst.D(), MemSrc); break;
    case IR::OpSize::i128Bit: ldr(Dst.Q(), MemSrc); break;
    case IR::OpSize::i256Bit: {
      LOGMAN_THROW_A_FMT(HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);
      const auto Operand = GenerateSVEMemOperand(OpSize, MemReg, Op->Offset, Op->OffsetType, Op->OffsetScale);
      ld1b<ARMEmitter::SubRegSize::i8Bit>(Dst.Z(), PRED_TMP_32B.Zeroing(), Operand);
      break;
    }
    default: LOGMAN_MSG_A_FMT("Unhandled LoadMem size: {}", OpSize); break;
    }
  }
}

DEF_OP(LoadMemPair) {
  const auto Op = IROp->C<IR::IROp_LoadMemPair>();
  const auto Addr = GetReg(Op->Addr);

  if (Op->Class == IR::RegClass::GPR) {
    const auto Dst1 = GetReg(Op->OutValue1);
    const auto Dst2 = GetReg(Op->OutValue2);

    switch (IROp->Size) {
    case IR::OpSize::i32Bit: ldp<ARMEmitter::IndexType::OFFSET>(Dst1.W(), Dst2.W(), Addr, Op->Offset); break;
    case IR::OpSize::i64Bit: ldp<ARMEmitter::IndexType::OFFSET>(Dst1.X(), Dst2.X(), Addr, Op->Offset); break;
    default: LOGMAN_MSG_A_FMT("Unhandled LoadMemPair size: {}", IROp->Size); break;
    }
  } else {
    const auto Dst1 = GetVReg(Op->OutValue1);
    const auto Dst2 = GetVReg(Op->OutValue2);

    switch (IROp->Size) {
    case IR::OpSize::i32Bit: ldp<ARMEmitter::IndexType::OFFSET>(Dst1.S(), Dst2.S(), Addr, Op->Offset); break;
    case IR::OpSize::i64Bit: ldp<ARMEmitter::IndexType::OFFSET>(Dst1.D(), Dst2.D(), Addr, Op->Offset); break;
    case IR::OpSize::i128Bit: ldp<ARMEmitter::IndexType::OFFSET>(Dst1.Q(), Dst2.Q(), Addr, Op->Offset); break;
    default: LOGMAN_MSG_A_FMT("Unhandled LoadMemPair size: {}", IROp->Size); break;
    }
  }
}

DEF_OP(LoadMemTSO) {
  const auto Op = IROp->C<IR::IROp_LoadMemTSO>();
  const auto OpSize = IROp->Size;

  const auto MemReg = GetReg(Op->Addr);

  if (Op->Class == IR::RegClass::GPR) {
    LOGMAN_THROW_A_FMT(Op->Offset.IsInvalid() || CTX->HostFeatures.SupportsTSOImm9, "unexpected offset");
    LOGMAN_THROW_A_FMT(Op->OffsetScale == 1, "unexpected offset scale");
    LOGMAN_THROW_A_FMT(Op->OffsetType == IR::MemOffsetType::SXTX, "unexpected offset type");
  }

  if (CTX->HostFeatures.SupportsTSOImm9 && Op->Class == IR::RegClass::GPR) {
    const auto Dst = GetReg(Node);
    uint64_t Offset = 0;
    if (!Op->Offset.IsInvalid()) {
      bool IsInline = IsInlineConstant(Op->Offset, &Offset);
      LOGMAN_THROW_A_FMT(IsInline, "expected immediate");
    }

    if (OpSize == IR::OpSize::i8Bit) {
      // 8bit load is always aligned to natural alignment
      const auto Dst = GetReg(Node);
      ldapurb(Dst, MemReg, Offset);
    } else {
      switch (OpSize) {
      case IR::OpSize::i16Bit: ldapurh(Dst, MemReg, Offset); break;
      case IR::OpSize::i32Bit: ldapur(Dst.W(), MemReg, Offset); break;
      case IR::OpSize::i64Bit: ldapur(Dst.X(), MemReg, Offset); break;
      default: LOGMAN_MSG_A_FMT("Unhandled LoadMemTSO size: {}", OpSize); break;
      }
      // Half-barrier once back-patched.
      nop();
    }
  } else if (CTX->HostFeatures.SupportsRCPC && Op->Class == IR::RegClass::GPR) {
    const auto Dst = GetReg(Node);
    if (OpSize == IR::OpSize::i8Bit) {
      // 8bit load is always aligned to natural alignment
      ldaprb(Dst.W(), MemReg);
    } else {
      switch (OpSize) {
      case IR::OpSize::i16Bit: ldaprh(Dst.W(), MemReg); break;
      case IR::OpSize::i32Bit: ldapr(Dst.W(), MemReg); break;
      case IR::OpSize::i64Bit: ldapr(Dst.X(), MemReg); break;
      default: LOGMAN_MSG_A_FMT("Unhandled LoadMemTSO size: {}", OpSize); break;
      }
      // Half-barrier once back-patched.
      nop();
    }
  } else if (Op->Class == IR::RegClass::GPR) {
    const auto Dst = GetReg(Node);
    if (OpSize == IR::OpSize::i8Bit) {
      // 8bit load is always aligned to natural alignment
      ldarb(Dst, MemReg);
    } else {
      switch (OpSize) {
      case IR::OpSize::i16Bit: ldarh(Dst, MemReg); break;
      case IR::OpSize::i32Bit: ldar(Dst.W(), MemReg); break;
      case IR::OpSize::i64Bit: ldar(Dst.X(), MemReg); break;
      default: LOGMAN_MSG_A_FMT("Unhandled LoadMemTSO size: {}", OpSize); break;
      }
      // Half-barrier once back-patched.
      nop();
    }
  } else {
    const auto Dst = GetVReg(Node);
    const auto MemSrc = GenerateMemOperand(OpSize, MemReg, Op->Offset, Op->OffsetType, Op->OffsetScale);
    switch (OpSize) {
    case IR::OpSize::i8Bit: ldrb(Dst, MemSrc); break;
    case IR::OpSize::i16Bit: ldrh(Dst, MemSrc); break;
    case IR::OpSize::i32Bit: ldr(Dst.S(), MemSrc); break;
    case IR::OpSize::i64Bit: ldr(Dst.D(), MemSrc); break;
    case IR::OpSize::i128Bit: ldr(Dst.Q(), MemSrc); break;
    case IR::OpSize::i256Bit: {
      LOGMAN_THROW_A_FMT(HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);
      const auto MemSrc = GenerateSVEMemOperand(OpSize, MemReg, Op->Offset, Op->OffsetType, Op->OffsetScale);
      ld1b<ARMEmitter::SubRegSize::i8Bit>(Dst.Z(), PRED_TMP_32B.Zeroing(), MemSrc);
      break;
    }
    default: LOGMAN_MSG_A_FMT("Unhandled LoadMemTSO size: {}", OpSize); break;
    }
    if (CTX->IsVectorAtomicTSOEnabled()) {
      // Half-barrier.
      dmb(ARMEmitter::BarrierScope::ISHLD);
    }
  }
}

DEF_OP(VLoadVectorMasked) {

  const auto Op = IROp->C<IR::IROp_VLoadVectorMasked>();
  const auto OpSize = IROp->Size;

  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);
  const auto SubRegSize = ConvertSubRegSize8(IROp);

  const auto CMPPredicate = ARMEmitter::PReg::p0;
  const auto GoverningPredicate = Is256Bit ? PRED_TMP_32B : PRED_TMP_16B;

  const auto Dst = GetVReg(Node);
  const auto MaskReg = GetVReg(Op->Mask);
  const auto MemReg = GetReg(Op->Addr);

  if (HostSupportsSVE128 || HostSupportsSVE256) {
    const auto MemSrc = GenerateSVEMemOperand(OpSize, MemReg, Op->Offset, Op->OffsetType, Op->OffsetScale);

    // Check if the sign bit is set for the given element size.
    cmplt(SubRegSize, CMPPredicate, GoverningPredicate.Zeroing(), MaskReg.Z(), 0);

    switch (IROp->ElementSize) {
    case IR::OpSize::i8Bit: {
      ld1b<ARMEmitter::SubRegSize::i8Bit>(Dst.Z(), CMPPredicate.Zeroing(), MemSrc);
      break;
    }
    case IR::OpSize::i16Bit: {
      ld1h<ARMEmitter::SubRegSize::i16Bit>(Dst.Z(), CMPPredicate.Zeroing(), MemSrc);
      break;
    }
    case IR::OpSize::i32Bit: {
      ld1w<ARMEmitter::SubRegSize::i32Bit>(Dst.Z(), CMPPredicate.Zeroing(), MemSrc);
      break;
    }
    case IR::OpSize::i64Bit: {
      ld1d(Dst.Z(), CMPPredicate.Zeroing(), MemSrc);
      break;
    }
    default: break;
    }
  } else {
    const auto PerformMove = [this](IR::OpSize ElementSize, const ARMEmitter::Register Dst, const ARMEmitter::VRegister Vector, int index) {
      switch (ElementSize) {
      case IR::OpSize::i8Bit: umov<ARMEmitter::SubRegSize::i8Bit>(Dst, Vector, index); break;
      case IR::OpSize::i16Bit: umov<ARMEmitter::SubRegSize::i16Bit>(Dst, Vector, index); break;
      case IR::OpSize::i32Bit: umov<ARMEmitter::SubRegSize::i32Bit>(Dst, Vector, index); break;
      case IR::OpSize::i64Bit: umov<ARMEmitter::SubRegSize::i64Bit>(Dst, Vector, index); break;
      default: LOGMAN_MSG_A_FMT("Unhandled ExtractElementSize: {}", ElementSize); break;
      }
    };

    // Prepare yourself adventurer. For a masked load without instructions that implement it.
    LOGMAN_THROW_A_FMT(OpSize == IR::OpSize::i128Bit, "Only supports 128-bit without SVE256");
    size_t NumElements = IR::NumElements(IROp->Size, IROp->ElementSize);

    // Use VTMP1 as the temporary destination
    auto TempDst = VTMP1;
    auto WorkingReg = TMP1;
    auto TempMemReg = MemReg;
    movi(ARMEmitter::SubRegSize::i64Bit, TempDst.Q(), 0);
    uint64_t Const {};
    if (Op->Offset.IsInvalid()) {
      // Intentional no-op.
    } else if (IsInlineConstant(Op->Offset, &Const)) {
      TempMemReg = TMP2;
      add(ARMEmitter::Size::i64Bit, TMP2, MemReg, Const);
    } else {
      LOGMAN_MSG_A_FMT("Complex addressing requested and not supported!");
    }

    const uint64_t ElementSizeInBits = IR::OpSizeAsBits(IROp->ElementSize);
    for (size_t i = 0; i < NumElements; ++i) {
      // Extract the mask element.
      PerformMove(IROp->ElementSize, WorkingReg, MaskReg, i);

      // If the sign bit is zero then skip the load
      ARMEmitter::ForwardLabel Skip {};
      (void)tbz(WorkingReg, ElementSizeInBits - 1, &Skip);
      // Do the gather load for this element into the destination
      switch (IROp->ElementSize) {
      case IR::OpSize::i8Bit: ld1<ARMEmitter::SubRegSize::i8Bit>(TempDst.Q(), i, TempMemReg); break;
      case IR::OpSize::i16Bit: ld1<ARMEmitter::SubRegSize::i16Bit>(TempDst.Q(), i, TempMemReg); break;
      case IR::OpSize::i32Bit: ld1<ARMEmitter::SubRegSize::i32Bit>(TempDst.Q(), i, TempMemReg); break;
      case IR::OpSize::i64Bit: ld1<ARMEmitter::SubRegSize::i64Bit>(TempDst.Q(), i, TempMemReg); break;
      case IR::OpSize::i128Bit: ldr(TempDst.Q(), TempMemReg, 0); break;
      default: LOGMAN_MSG_A_FMT("Unhandled {} size: {}", __func__, IROp->ElementSize); return;
      }

      (void)Bind(&Skip);

      if ((i + 1) != NumElements) {
        // Handle register rename to save a move.
        auto WorkingReg = TempMemReg;
        TempMemReg = TMP2;
        add(ARMEmitter::Size::i64Bit, TempMemReg, WorkingReg, IR::OpSizeToSize(IROp->ElementSize));
      }
    }

    // Move result.
    mov(Dst.Q(), TempDst.Q());
  }
}

DEF_OP(VStoreVectorMasked) {
  const auto Op = IROp->C<IR::IROp_VStoreVectorMasked>();
  const auto OpSize = IROp->Size;

  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);
  const auto SubRegSize = ConvertSubRegSize8(IROp);

  const auto CMPPredicate = ARMEmitter::PReg::p0;
  const auto GoverningPredicate = Is256Bit ? PRED_TMP_32B : PRED_TMP_16B;

  const auto RegData = GetVReg(Op->Data);
  const auto MaskReg = GetVReg(Op->Mask);
  const auto MemReg = GetReg(Op->Addr);
  if (HostSupportsSVE128 || HostSupportsSVE256) {
    const auto MemDst = GenerateSVEMemOperand(OpSize, MemReg, Op->Offset, Op->OffsetType, Op->OffsetScale);

    // Check if the sign bit is set for the given element size.
    cmplt(SubRegSize, CMPPredicate, GoverningPredicate.Zeroing(), MaskReg.Z(), 0);

    switch (IROp->ElementSize) {
    case IR::OpSize::i8Bit: {
      st1b<ARMEmitter::SubRegSize::i8Bit>(RegData.Z(), CMPPredicate.Zeroing(), MemDst);
      break;
    }
    case IR::OpSize::i16Bit: {
      st1h<ARMEmitter::SubRegSize::i16Bit>(RegData.Z(), CMPPredicate.Zeroing(), MemDst);
      break;
    }
    case IR::OpSize::i32Bit: {
      st1w<ARMEmitter::SubRegSize::i32Bit>(RegData.Z(), CMPPredicate.Zeroing(), MemDst);
      break;
    }
    case IR::OpSize::i64Bit: {
      st1d(RegData.Z(), CMPPredicate.Zeroing(), MemDst);
      break;
    }
    default: break;
    }
  } else {
    const auto PerformMove = [this](IR::OpSize ElementSize, const ARMEmitter::Register Dst, const ARMEmitter::VRegister Vector, int index) {
      switch (ElementSize) {
      case IR::OpSize::i8Bit: umov<ARMEmitter::SubRegSize::i8Bit>(Dst, Vector, index); break;
      case IR::OpSize::i16Bit: umov<ARMEmitter::SubRegSize::i16Bit>(Dst, Vector, index); break;
      case IR::OpSize::i32Bit: umov<ARMEmitter::SubRegSize::i32Bit>(Dst, Vector, index); break;
      case IR::OpSize::i64Bit: umov<ARMEmitter::SubRegSize::i64Bit>(Dst, Vector, index); break;
      default: LOGMAN_MSG_A_FMT("Unhandled ExtractElementSize: {}", ElementSize); break;
      }
    };

    // Prepare yourself adventurer. For a masked store without instructions that implement it.
    LOGMAN_THROW_A_FMT(OpSize == IR::OpSize::i128Bit, "Only supports 128-bit without SVE256");
    size_t NumElements = IR::NumElements(IROp->Size, IROp->ElementSize);

    // Use VTMP1 as the temporary destination
    auto WorkingReg = TMP1;
    auto TempMemReg = MemReg;

    uint64_t Const {};
    if (Op->Offset.IsInvalid()) {
      // Intentional no-op.
    } else if (IsInlineConstant(Op->Offset, &Const)) {
      TempMemReg = TMP2;
      add(ARMEmitter::Size::i64Bit, TMP2, MemReg, Const);
    } else {
      LOGMAN_MSG_A_FMT("Complex addressing requested and not supported!");
    }

    const uint64_t ElementSizeInBits = IR::OpSizeAsBits(IROp->ElementSize);
    for (size_t i = 0; i < NumElements; ++i) {
      // Extract the mask element.
      PerformMove(IROp->ElementSize, WorkingReg, MaskReg, i);

      // If the sign bit is zero then skip the load
      ARMEmitter::ForwardLabel Skip {};
      (void)tbz(WorkingReg, ElementSizeInBits - 1, &Skip);
      // Do the gather load for this element into the destination
      switch (IROp->ElementSize) {
      case IR::OpSize::i8Bit: st1<ARMEmitter::SubRegSize::i8Bit>(RegData.Q(), i, TempMemReg); break;
      case IR::OpSize::i16Bit: st1<ARMEmitter::SubRegSize::i16Bit>(RegData.Q(), i, TempMemReg); break;
      case IR::OpSize::i32Bit: st1<ARMEmitter::SubRegSize::i32Bit>(RegData.Q(), i, TempMemReg); break;
      case IR::OpSize::i64Bit: st1<ARMEmitter::SubRegSize::i64Bit>(RegData.Q(), i, TempMemReg); break;
      case IR::OpSize::i128Bit: str(RegData.Q(), TempMemReg, 0); break;
      default: LOGMAN_MSG_A_FMT("Unhandled {} size: {}", __func__, IROp->ElementSize); return;
      }

      (void)Bind(&Skip);

      if ((i + 1) != NumElements) {
        // Handle register rename to save a move.
        auto WorkingReg = TempMemReg;
        TempMemReg = TMP2;
        add(ARMEmitter::Size::i64Bit, TempMemReg, WorkingReg, IR::OpSizeToSize(IROp->ElementSize));
      }
    }
  }
}

void Arm64JITCore::Emulate128BitGather(IR::OpSize Size, IR::OpSize ElementSize, ARMEmitter::VRegister Dst,
                                       ARMEmitter::VRegister IncomingDst, std::optional<ARMEmitter::Register> BaseAddr,
                                       ARMEmitter::VRegister VectorIndexLow, std::optional<ARMEmitter::VRegister> VectorIndexHigh,
                                       ARMEmitter::VRegister MaskReg, IR::OpSize VectorIndexSize, size_t DataElementOffsetStart,
                                       size_t IndexElementOffsetStart, uint8_t OffsetScale, IR::OpSize AddrSize) {
  LOGMAN_THROW_A_FMT(ElementSize >= IR::OpSize::i8Bit && ElementSize <= IR::OpSize::i64Bit, "Invalid element size");

  const auto PerformSMove = [this](IR::OpSize ElementSize, const ARMEmitter::Register Dst, const ARMEmitter::VRegister Vector, int index) {
    switch (ElementSize) {
    case IR::OpSize::i8Bit: smov<ARMEmitter::SubRegSize::i8Bit>(Dst.X(), Vector, index); break;
    case IR::OpSize::i16Bit: smov<ARMEmitter::SubRegSize::i16Bit>(Dst.X(), Vector, index); break;
    case IR::OpSize::i32Bit: smov<ARMEmitter::SubRegSize::i32Bit>(Dst.X(), Vector, index); break;
    case IR::OpSize::i64Bit: umov<ARMEmitter::SubRegSize::i64Bit>(Dst.X(), Vector, index); break;
    default: LOGMAN_MSG_A_FMT("Unhandled ExtractElementSize: {}", ElementSize); break;
    }
  };

  const auto PerformMove = [this](IR::OpSize ElementSize, const ARMEmitter::Register Dst, const ARMEmitter::VRegister Vector, int index) {
    switch (ElementSize) {
    case IR::OpSize::i8Bit: umov<ARMEmitter::SubRegSize::i8Bit>(Dst, Vector, index); break;
    case IR::OpSize::i16Bit: umov<ARMEmitter::SubRegSize::i16Bit>(Dst, Vector, index); break;
    case IR::OpSize::i32Bit: umov<ARMEmitter::SubRegSize::i32Bit>(Dst, Vector, index); break;
    case IR::OpSize::i64Bit: umov<ARMEmitter::SubRegSize::i64Bit>(Dst, Vector, index); break;
    default: LOGMAN_MSG_A_FMT("Unhandled ExtractElementSize: {}", ElementSize); break;
    }
  };

  // FEX needs to use a temporary destination vector register in a couple of instances.
  // When Dst overlaps MaskReg, VectorIndexLow, or VectorIndexHigh
  // Due to x86 gather instruction limitations, it is highly likely that a destination temporary isn't required.
  const bool NeedsDestTmp = Dst == MaskReg || Dst == VectorIndexLow || (VectorIndexHigh.has_value() && Dst == *VectorIndexHigh);

  // If the incoming destination isn't the destination then we need to move.
  const bool NeedsIncomingDestMove = Dst != IncomingDst || NeedsDestTmp;

  ///< Adventurers beware, emulated ASIMD style gather masked load operation.
  // Number of elements to load is calculated by the number of index elements available.
  size_t NumAddrElements = (VectorIndexHigh.has_value() ? 32 : 16) / IR::OpSizeToSize(VectorIndexSize);
  // The number of elements is clamped by the resulting register size.
  size_t NumDataElements = std::min<size_t>(IR::OpSizeToSize(Size) / IR::OpSizeToSize(ElementSize), NumAddrElements);

  size_t IndexElementsSizeBytes = NumAddrElements * IR::OpSizeToSize(VectorIndexSize);
  if (IndexElementsSizeBytes > 16) {
    // We must have a high register in this case.
    LOGMAN_THROW_A_FMT(VectorIndexHigh.has_value(), "Need High vector index register!");
  }

  auto ResultReg = Dst;
  if (NeedsDestTmp) {
    // Use VTMP1 as the temporary destination
    ResultReg = VTMP1;
  }
  auto WorkingReg = TMP1;
  auto TempMemReg = TMP2;
  const uint64_t ElementSizeInBits = IR::OpSizeToSize(ElementSize) * 8;

  if (NeedsIncomingDestMove) {
    mov(ResultReg.Q(), IncomingDst.Q());
  }

  for (size_t i = DataElementOffsetStart, IndexElement = IndexElementOffsetStart; i < NumDataElements; ++i, ++IndexElement) {
    ARMEmitter::ForwardLabel Skip {};
    // Extract mask element
    PerformMove(ElementSize, WorkingReg, MaskReg, i);

    // Skip if the mask's sign bit isn't set
    (void)tbz(WorkingReg, ElementSizeInBits - 1, &Skip);

    // Extract Index Element
    if ((IndexElement * IR::OpSizeToSize(VectorIndexSize)) >= 16) {
      // Fetch from the high index register.
      PerformSMove(VectorIndexSize, WorkingReg, *VectorIndexHigh, IndexElement - (16 / IR::OpSizeToSize(VectorIndexSize)));
    } else {
      // Fetch from the low index register.
      PerformSMove(VectorIndexSize, WorkingReg, VectorIndexLow, IndexElement);
    }

    // Calculate memory position for this gather load
    if (BaseAddr.has_value()) {
      if (VectorIndexSize == IR::OpSize::i32Bit) {
        add(ConvertSize(AddrSize), TempMemReg, *BaseAddr, WorkingReg, ARMEmitter::ExtendedType::SXTW, FEXCore::ilog2(OffsetScale));
      } else {
        add(ConvertSize(AddrSize), TempMemReg, *BaseAddr, WorkingReg, ARMEmitter::ShiftType::LSL, FEXCore::ilog2(OffsetScale));
      }
    } else {
      ///< In this case we have no base address, All addresses come from the vector register itself
      if (VectorIndexSize == IR::OpSize::i32Bit) {
        // Sign extend and shift in to the 64-bit register
        sbfiz(ConvertSize(AddrSize), TempMemReg, WorkingReg, FEXCore::ilog2(OffsetScale), 32);
      } else {
        lsl(ConvertSize(AddrSize), TempMemReg, WorkingReg, FEXCore::ilog2(OffsetScale));
      }
    }

    // Now that the address is calculated. Do the load.
    switch (ElementSize) {
    case IR::OpSize::i8Bit: ld1<ARMEmitter::SubRegSize::i8Bit>(ResultReg.Q(), i, TempMemReg); break;
    case IR::OpSize::i16Bit: ld1<ARMEmitter::SubRegSize::i16Bit>(ResultReg.Q(), i, TempMemReg); break;
    case IR::OpSize::i32Bit: ld1<ARMEmitter::SubRegSize::i32Bit>(ResultReg.Q(), i, TempMemReg); break;
    case IR::OpSize::i64Bit: ld1<ARMEmitter::SubRegSize::i64Bit>(ResultReg.Q(), i, TempMemReg); break;
    case IR::OpSize::i128Bit: ldr(ResultReg.Q(), TempMemReg, 0); break;
    default: LOGMAN_MSG_A_FMT("Unhandled {} size: {}", __func__, ElementSize); FEX_UNREACHABLE;
    }

    (void)Bind(&Skip);
  }

  if (NeedsDestTmp) {
    // Move result.
    mov(Dst.Q(), ResultReg.Q());
  }
}

DEF_OP(VLoadVectorGatherMasked) {
  const auto Op = IROp->C<IR::IROp_VLoadVectorGatherMasked>();
  const auto OpSize = IROp->Size;

  const auto VectorIndexSize = Op->VectorIndexElementSize;
  const auto OffsetScale = Op->OffsetScale;
  const auto DataElementOffsetStart = Op->DataElementOffsetStart;
  const auto IndexElementOffsetStart = Op->IndexElementOffsetStart;

  ///< This IR operation handles discontiguous masked gather loadstore instructions. Some things to note about its behaviour.
  ///  - VSIB behaviour is mostly entirely exposed in the IR operation directly.
  ///    - Displacement is the only value missing as that can be added directly to AddrBase.
  ///  - VectorIndex{Low,High} contains the index offsets for each element getting loaded.
  ///     - These element sizes are decoupled from the resulting element size. These can be 32-bit or 64-bit.
  ///     - When the element size is 32-bit then the value is zero-extended to the full 64-bit address calculation
  ///     - When loading a 128-bit result with 64-bit VectorIndex Elements, this requires the use of both VectorIndexLow and VectorIndexHigh
  ///     to get enough pointers.
  ///  - When VectorIndexElementSize and OffsetScale matches Arm64 SVE behaviour then the operation becomes more optimal
  ///     - When the behaviour doesn't match then it gets decomposed to ASIMD style masked load.
  ///  - AddrBase also doesn't need to exist
  ///     - If the instruction is using 64-bit vector indexing or 32-bit addresses where the top-bit isn't set then this is valid!
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetVReg(Node);
  const auto IncomingDst = GetVReg(Op->Incoming);

  const auto MaskReg = GetVReg(Op->Mask);
  std::optional<ARMEmitter::Register> BaseAddr = !Op->AddrBase.IsInvalid() ? std::make_optional(GetReg(Op->AddrBase)) : std::nullopt;
  const auto VectorIndexLow = GetVReg(Op->VectorIndexLow);
  std::optional<ARMEmitter::VRegister> VectorIndexHigh =
    !Op->VectorIndexHigh.IsInvalid() ? std::make_optional(GetVReg(Op->VectorIndexHigh)) : std::nullopt;

  ///< If the host supports SVE and the offset scale matches SVE limitations then it can do an SVE style load.
  const bool SupportsSVELoad = (HostSupportsSVE128 || HostSupportsSVE256) &&
                               (OffsetScale == 1 || OffsetScale == IR::OpSizeToSize(VectorIndexSize)) &&
                               VectorIndexSize == IROp->ElementSize && Op->AddrSize == IR::OpSize::i64Bit;

  if (SupportsSVELoad) {
    uint8_t SVEScale = FEXCore::ilog2(OffsetScale);
    ARMEmitter::SVEModType ModType = ARMEmitter::SVEModType::MOD_NONE;
    if (VectorIndexSize == IR::OpSize::i32Bit) {
      ModType = ARMEmitter::SVEModType::MOD_SXTW;
    } else if (VectorIndexSize == IR::OpSize::i64Bit && OffsetScale != 1) {
      ModType = ARMEmitter::SVEModType::MOD_LSL;
    }

    const auto SubRegSize = ConvertSubRegSize8(IROp);

    const auto CMPPredicate = ARMEmitter::PReg::p0;
    const auto GoverningPredicate = Is256Bit ? PRED_TMP_32B : PRED_TMP_16B;

    // Check if the sign bit is set for the given element size.
    cmplt(SubRegSize, CMPPredicate, GoverningPredicate.Zeroing(), MaskReg.Z(), 0);
    auto TempDst = VTMP1;

    // No need to load a temporary register in the case that we weren't provided a base address and there is no scaling.
    ARMEmitter::SVEMemOperand MemDst {ARMEmitter::SVEMemOperand(VectorIndexLow.Z(), 0)};
    if (BaseAddr.has_value() || OffsetScale != 1) {
      ARMEmitter::Register AddrReg = TMP1;
      if (BaseAddr.has_value()) {
        AddrReg = GetReg(Op->AddrBase);
      } else {
        ///< OpcodeDispatcher didn't provide a Base address while SVE requires one.
        LoadConstant(ARMEmitter::Size::i64Bit, AddrReg, 0);
      }
      MemDst = ARMEmitter::SVEMemOperand(AddrReg.X(), VectorIndexLow.Z(), ModType, SVEScale);
    }

    switch (IROp->ElementSize) {
    case IR::OpSize::i8Bit: {
      ld1b<ARMEmitter::SubRegSize::i8Bit>(TempDst.Z(), CMPPredicate.Zeroing(), MemDst);
      break;
    }
    case IR::OpSize::i16Bit: {
      ld1h<ARMEmitter::SubRegSize::i16Bit>(TempDst.Z(), CMPPredicate.Zeroing(), MemDst);
      break;
    }
    case IR::OpSize::i32Bit: {
      ld1w<ARMEmitter::SubRegSize::i32Bit>(TempDst.Z(), CMPPredicate.Zeroing(), MemDst);
      break;
    }
    case IR::OpSize::i64Bit: {
      ld1d(TempDst.Z(), CMPPredicate.Zeroing(), MemDst);
      break;
    }
    default: break;
    }

    ///< Merge elements based on predicate.
    sel(SubRegSize, Dst.Z(), CMPPredicate, TempDst.Z(), IncomingDst.Z());
  } else {
    LOGMAN_THROW_A_FMT(!Is256Bit, "Can't emulate this gather load in the backend! Programming error!");
    Emulate128BitGather(IROp->Size, IROp->ElementSize, Dst, IncomingDst, BaseAddr, VectorIndexLow, VectorIndexHigh, MaskReg,
                        VectorIndexSize, DataElementOffsetStart, IndexElementOffsetStart, OffsetScale, Op->AddrSize);
  }
}

DEF_OP(VLoadVectorGatherMaskedQPS) {
  const auto Op = IROp->C<IR::IROp_VLoadVectorGatherMaskedQPS>();

  /// This instruction behaves similarly to the non-QPS version except for some STRICT limitations
  /// - Only supports 32-bit element data size!
  /// - Only supports 64-bit element address size!
  /// - Only masks elements based on 32-bit element data size! (NOT ADDR SIZE!)
  /// - Optimally uses SVE's `ld1w {zt.D}` variant instruction!
  /// - Only outputs a single 128-bit result, while consuming 128-bit or 256-bit of address indexes!
  /// - Matches VGATHERQPS/VPGATHERQD behaviour!
  const auto OffsetScale = Op->OffsetScale;
  const auto Dst = GetVReg(Node);
  const auto IncomingDst = GetVReg(Op->Incoming);

  const auto MaskReg = GetVReg(Op->MaskReg);
  std::optional<ARMEmitter::Register> BaseAddr = !Op->AddrBase.IsInvalid() ? std::make_optional(GetReg(Op->AddrBase)) : std::nullopt;
  const auto VectorIndexLow = GetVReg(Op->VectorIndexLow);
  std::optional<ARMEmitter::VRegister> VectorIndexHigh =
    !Op->VectorIndexHigh.IsInvalid() ? std::make_optional(GetVReg(Op->VectorIndexHigh)) : std::nullopt;

  ///< If the host supports SVE and the offset scale matches SVE limitations then it can do an SVE style load.
  const bool SupportsSVELoad = HostSupportsSVE128 && (OffsetScale == 1 || OffsetScale == 4) && Op->AddrSize == IR::OpSize::i64Bit;

  if (SupportsSVELoad) {
    ARMEmitter::SVEModType ModType = ARMEmitter::SVEModType::MOD_NONE;
    if (OffsetScale != 1) {
      ModType = ARMEmitter::SVEModType::MOD_LSL;
    }

    const auto CMPPredicate = ARMEmitter::PReg::p0;
    const auto CMPPredicate2 = ARMEmitter::PReg::p1;

    const auto GoverningPredicate = PRED_TMP_16B;

    // Check if the sign bit is set for the given element size.
    // This will set the predicate bits for elements [0, 1, 2, 3]
    // We then use punpklo to extend the low results to be for 64-bit elements.
    cmplt(ARMEmitter::SubRegSize::i32Bit, CMPPredicate, GoverningPredicate.Zeroing(), MaskReg.Z(), 0);
    punpklo(CMPPredicate2, CMPPredicate);
    auto TempDst = VTMP1;

    auto GatherExtend = [this](ARMEmitter::VRegister Dst, std::optional<ARMEmitter::Register> BaseAddr, ARMEmitter::VRegister VectorIndex,
                               ARMEmitter::PRegister CMPPredicate, ARMEmitter::SVEModType ModType, uint8_t OffsetScale) {
      // No need to load a temporary register in the case that we weren't provided a base address and there is no scaling.
      uint8_t SVEScale = FEXCore::ilog2(OffsetScale);
      ARMEmitter::SVEMemOperand MemDst {ARMEmitter::SVEMemOperand(VectorIndex.Z(), 0)};
      if (BaseAddr.has_value() || OffsetScale != 1) {
        ARMEmitter::Register AddrReg = TMP1;
        if (BaseAddr.has_value()) {
          AddrReg = *BaseAddr;
        } else {
          ///< OpcodeDispatcher didn't provide a Base address while SVE requires one.
          LoadConstant(ARMEmitter::Size::i64Bit, AddrReg, 0);
        }
        MemDst = ARMEmitter::SVEMemOperand(AddrReg.X(), VectorIndex.Z(), ModType, SVEScale);
      }

      ld1w<ARMEmitter::SubRegSize::i64Bit>(Dst.Z(), CMPPredicate.Zeroing(), MemDst);
    };

    GatherExtend(TempDst, BaseAddr, VectorIndexLow, CMPPredicate2, ModType, OffsetScale);

    if (VectorIndexHigh.has_value()) {
      punpkhi(CMPPredicate2, CMPPredicate);
      GatherExtend(VTMP2, BaseAddr, *VectorIndexHigh, CMPPredicate2, ModType, OffsetScale);
      // Move elements to the lower half.
      uzp1(ARMEmitter::SubRegSize::i32Bit, TempDst.Q(), TempDst.Q(), VTMP2.Q());
      ///< Merge elements based on predicate.
      sel(ARMEmitter::SubRegSize::i32Bit, Dst.Z(), CMPPredicate, TempDst.Z(), IncomingDst.Z());
    } else {
      // Move elements to the lower half.
      xtn(ARMEmitter::SubRegSize::i32Bit, TempDst.Q(), TempDst.Q());
      ///< Merge elements based on predicate.
      sel(ARMEmitter::SubRegSize::i32Bit, Dst.Z(), CMPPredicate, TempDst.Z(), IncomingDst.Z());
    }
  } else {
    Emulate128BitGather(IR::OpSize::i128Bit, IR::OpSize::i32Bit, Dst, IncomingDst, BaseAddr, VectorIndexLow, VectorIndexHigh, MaskReg,
                        IR::OpSize::i64Bit, 0, 0, OffsetScale, Op->AddrSize);
  }
}

DEF_OP(VLoadVectorElement) {
  const auto Op = IROp->C<IR::IROp_VLoadVectorElement>();
  const auto OpSize = IROp->Size;

  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  const auto ElementSize = IROp->ElementSize;

  const auto Dst = GetVReg(Node);
  const auto DstSrc = GetVReg(Op->DstSrc);
  const auto MemReg = GetReg(Op->Addr);

  LOGMAN_THROW_A_FMT(ElementSize == IR::OpSize::i8Bit || ElementSize == IR::OpSize::i16Bit || ElementSize == IR::OpSize::i32Bit ||
                       ElementSize == IR::OpSize::i64Bit || ElementSize == IR::OpSize::i128Bit,
                     "Invalid element "
                     "size");

  if (Is256Bit) {
    LOGMAN_MSG_A_FMT("Unsupported 256-bit VLoadVectorElement");
  } else {
    if (Dst != DstSrc && ElementSize != IR::OpSize::i128Bit) {
      mov(Dst.Q(), DstSrc.Q());
    }
    switch (ElementSize) {
    case IR::OpSize::i8Bit: ld1<ARMEmitter::SubRegSize::i8Bit>(Dst.Q(), Op->Index, MemReg); break;
    case IR::OpSize::i16Bit: ld1<ARMEmitter::SubRegSize::i16Bit>(Dst.Q(), Op->Index, MemReg); break;
    case IR::OpSize::i32Bit: ld1<ARMEmitter::SubRegSize::i32Bit>(Dst.Q(), Op->Index, MemReg); break;
    case IR::OpSize::i64Bit: ld1<ARMEmitter::SubRegSize::i64Bit>(Dst.Q(), Op->Index, MemReg); break;
    case IR::OpSize::i128Bit: ldr(Dst.Q(), MemReg); break;
    default: LOGMAN_MSG_A_FMT("Unhandled {} size: {}", __func__, ElementSize); return;
    }
  }

  // Emit a half-barrier if TSO is enabled.
  if (CTX->IsVectorAtomicTSOEnabled()) {
    dmb(ARMEmitter::BarrierScope::ISHLD);
  }
}

DEF_OP(VStoreVectorElement) {
  const auto Op = IROp->C<IR::IROp_VStoreVectorElement>();
  const auto OpSize = IROp->Size;

  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  const auto ElementSize = IROp->ElementSize;

  const auto Value = GetVReg(Op->Value);
  const auto MemReg = GetReg(Op->Addr);

  LOGMAN_THROW_A_FMT(ElementSize == IR::OpSize::i8Bit || ElementSize == IR::OpSize::i16Bit || ElementSize == IR::OpSize::i32Bit ||
                       ElementSize == IR::OpSize::i64Bit || ElementSize == IR::OpSize::i128Bit,
                     "Invalid element "
                     "size");

  // Emit a half-barrier if TSO is enabled.
  if (CTX->IsVectorAtomicTSOEnabled()) {
    dmb(ARMEmitter::BarrierScope::ISH);
  }

  if (Is256Bit) {
    LOGMAN_MSG_A_FMT("Unsupported 256-bit {}", __func__);
  } else {
    switch (ElementSize) {
    case IR::OpSize::i8Bit: st1<ARMEmitter::SubRegSize::i8Bit>(Value.Q(), Op->Index, MemReg); break;
    case IR::OpSize::i16Bit: st1<ARMEmitter::SubRegSize::i16Bit>(Value.Q(), Op->Index, MemReg); break;
    case IR::OpSize::i32Bit: st1<ARMEmitter::SubRegSize::i32Bit>(Value.Q(), Op->Index, MemReg); break;
    case IR::OpSize::i64Bit: st1<ARMEmitter::SubRegSize::i64Bit>(Value.Q(), Op->Index, MemReg); break;
    case IR::OpSize::i128Bit: str(Value.Q(), MemReg); break;
    default: LOGMAN_MSG_A_FMT("Unhandled {} size: {}", __func__, ElementSize); return;
    }
  }
}

DEF_OP(VBroadcastFromMem) {
  const auto Op = IROp->C<IR::IROp_VBroadcastFromMem>();
  const auto OpSize = IROp->Size;

  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);
  const auto ElementSize = IROp->ElementSize;

  const auto Dst = GetVReg(Node);
  const auto MemReg = GetReg(Op->Address);

  LOGMAN_THROW_A_FMT(ElementSize == IR::OpSize::i8Bit || ElementSize == IR::OpSize::i16Bit || ElementSize == IR::OpSize::i32Bit ||
                       ElementSize == IR::OpSize::i64Bit || ElementSize == IR::OpSize::i128Bit,
                     "Invalid element "
                     "size");

  if (Is256Bit && HostSupportsSVE256) {
    const auto GoverningPredicate = PRED_TMP_32B.Zeroing();

    switch (ElementSize) {
    case IR::OpSize::i8Bit: ld1rb(ARMEmitter::SubRegSize::i8Bit, Dst.Z(), GoverningPredicate, MemReg); break;
    case IR::OpSize::i16Bit: ld1rh(ARMEmitter::SubRegSize::i16Bit, Dst.Z(), GoverningPredicate, MemReg); break;
    case IR::OpSize::i32Bit: ld1rw(ARMEmitter::SubRegSize::i32Bit, Dst.Z(), GoverningPredicate, MemReg); break;
    case IR::OpSize::i64Bit: ld1rd(Dst.Z(), GoverningPredicate, MemReg); break;
    case IR::OpSize::i128Bit: ld1rqb(Dst.Z(), GoverningPredicate, MemReg); break;
    default: LOGMAN_MSG_A_FMT("Unhandled VBroadcastFromMem size: {}", ElementSize); return;
    }
  } else {
    switch (ElementSize) {
    case IR::OpSize::i8Bit: ld1r<ARMEmitter::SubRegSize::i8Bit>(Dst.Q(), MemReg); break;
    case IR::OpSize::i16Bit: ld1r<ARMEmitter::SubRegSize::i16Bit>(Dst.Q(), MemReg); break;
    case IR::OpSize::i32Bit: ld1r<ARMEmitter::SubRegSize::i32Bit>(Dst.Q(), MemReg); break;
    case IR::OpSize::i64Bit: ld1r<ARMEmitter::SubRegSize::i64Bit>(Dst.Q(), MemReg); break;
    case IR::OpSize::i128Bit:
      // Normal load, like ld1rqb with 128-bit regs.
      ldr(Dst.Q(), MemReg);
      break;
    default: LOGMAN_MSG_A_FMT("Unhandled VBroadcastFromMem size: {}", ElementSize); return;
    }
  }

  // Emit a half-barrier if TSO is enabled.
  if (CTX->IsVectorAtomicTSOEnabled()) {
    dmb(ARMEmitter::BarrierScope::ISHLD);
  }
}

DEF_OP(Push) {
  const auto Op = IROp->C<IR::IROp_Push>();
  const auto ValueSize = IR::OpSizeToSize(Op->ValueSize);
  auto Src = GetReg(Op->Value);
  const auto AddrSrc = GetReg(Op->Addr);
  const auto Dst = GetReg(Node);

  bool NeedsMoveAfterwards = false;
  if (Dst != AddrSrc) {
    if (Dst == Src) {
      NeedsMoveAfterwards = true;
      // Need to be careful here, incoming source might be reused afterwards.
    } else {
      // RA constraints would let this always be true.
      mov(IROp->Size == IR::OpSize::i64Bit ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit, Dst, AddrSrc);
    }
  }

  if (Src == AddrSrc) {
    // If the data source is the address source then we need to do some additional work.
    // This is because it is undefined behaviour to do a writeback on store operation where dest == src.
    // In the case of writeback where the source is the address there are multiple behaviours.
    // - SIGILL - Apple Silicon Behaviour
    // - Stores original value - Cortex behaviour
    // - Stores value after pre-index adjust adjust - Vixl simulator behaviour.
    // - Undefined value stored
    // - Undefined behaviour(!)

    // In this path Src can end up overlapping both AddrSrc and Dst.
    // Move the data to a temporary and store from there instead.
    mov(TMP1, Src.X());
    Src = TMP1;
  }

  if (NeedsMoveAfterwards) {
    switch (ValueSize) {
    case 1: {
      sturb(Src.W(), AddrSrc, -ValueSize);
      break;
    }
    case 2: {
      sturh(Src.W(), AddrSrc, -ValueSize);
      break;
    }
    case 4: {
      stur(Src.W(), AddrSrc, -ValueSize);
      break;
    }
    case 8: {
      stur(Src.X(), AddrSrc, -ValueSize);
      break;
    }
    default: {
      LOGMAN_MSG_A_FMT("Unhandled {} size: {}", __func__, ValueSize);
      break;
    }
    }

    sub(IROp->Size == IR::OpSize::i64Bit ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit, Dst, AddrSrc, ValueSize);
  } else {
    switch (ValueSize) {
    case 1: {
      strb<ARMEmitter::IndexType::PRE>(Src.W(), Dst, -ValueSize);
      break;
    }
    case 2: {
      strh<ARMEmitter::IndexType::PRE>(Src.W(), Dst, -ValueSize);
      break;
    }
    case 4: {
      str<ARMEmitter::IndexType::PRE>(Src.W(), Dst, -ValueSize);
      break;
    }
    case 8: {
      str<ARMEmitter::IndexType::PRE>(Src.X(), Dst, -ValueSize);
      break;
    }
    default: {
      LOGMAN_MSG_A_FMT("Unhandled {} size: {}", __func__, ValueSize);
      break;
    }
    }
  }
}

DEF_OP(PushTwo) {
  const auto Op = IROp->C<IR::IROp_PushTwo>();
  const auto ValueSize = IR::OpSizeToSize(Op->ValueSize);
  auto Src1 = GetReg(Op->Value1);
  auto Src2 = GetReg(Op->Value2);
  const auto Dst = GetReg(Op->Addr);

  switch (ValueSize) {
  case 4: {
    stp<ARMEmitter::IndexType::PRE>(Src1.W(), Src2.W(), Dst, -2 * ValueSize);
    break;
  }
  case 8: {
    stp<ARMEmitter::IndexType::PRE>(Src1.X(), Src2.X(), Dst, -2 * ValueSize);
    break;
  }
  default: {
    LOGMAN_MSG_A_FMT("Unhandled {} size: {}", __func__, ValueSize);
    break;
  }
  }
}

DEF_OP(Pop) {
  const auto Op = IROp->C<IR::IROp_Pop>();
  const auto Size = IR::OpSizeToSize(Op->Size);
  const auto Addr = GetReg(Op->InoutAddr);
  const auto Dst = GetReg(Op->OutValue);

  LOGMAN_THROW_A_FMT(Dst != Addr, "Invalid");

  switch (Size) {
  case 1: {
    ldrb<ARMEmitter::IndexType::POST>(Dst.W(), Addr, Size);
    break;
  }
  case 2: {
    ldrh<ARMEmitter::IndexType::POST>(Dst.W(), Addr, Size);
    break;
  }
  case 4: {
    ldr<ARMEmitter::IndexType::POST>(Dst.W(), Addr, Size);
    break;
  }
  case 8: {
    ldr<ARMEmitter::IndexType::POST>(Dst.X(), Addr, Size);
    break;
  }
  default: {
    LOGMAN_MSG_A_FMT("Unhandled {} size: {}", __func__, Op->Size);
    break;
  }
  }
}

DEF_OP(PopTwo) {
  const auto Op = IROp->C<IR::IROp_PopTwo>();
  const auto Size = IR::OpSizeToSize(Op->Size);
  const auto Addr = GetReg(Op->InoutAddr);
  auto Dst1 = GetReg(Op->OutValue1);
  const auto Dst2 = GetReg(Op->OutValue2);

  // ldp x, x is invalid. Explicitly discard the first destination to encode.
  if (Dst1 == Dst2) {
    Dst1 = ARMEmitter::Reg::zr;
  }

  LOGMAN_THROW_A_FMT(Dst1 != Addr && Dst2 != Addr, "Invalid");
  LOGMAN_THROW_A_FMT(Dst1 != Dst2, "Invalid");

  switch (Size) {
  case 4: {
    ldp<ARMEmitter::IndexType::POST>(Dst1.W(), Dst2.W(), Addr, 2 * Size);
    break;
  }
  case 8: {
    ldp<ARMEmitter::IndexType::POST>(Dst1.X(), Dst2.X(), Addr, 2 * Size);
    break;
  }
  default: {
    LOGMAN_MSG_A_FMT("Unhandled {} size: {}", __func__, Op->Size);
    break;
  }
  }
}

DEF_OP(StoreMem) {
  const auto Op = IROp->C<IR::IROp_StoreMem>();
  const auto OpSize = IROp->Size;

  const auto MemReg = GetReg(Op->Addr);
  const auto MemSrc = GenerateMemOperand(OpSize, MemReg, Op->Offset, Op->OffsetType, Op->OffsetScale);

  if (Op->Class == IR::RegClass::GPR) {
    const auto Src = GetZeroableReg(Op->Value);
    switch (OpSize) {
    case IR::OpSize::i8Bit: strb(Src, MemSrc); break;
    case IR::OpSize::i16Bit: strh(Src, MemSrc); break;
    case IR::OpSize::i32Bit: str(Src.W(), MemSrc); break;
    case IR::OpSize::i64Bit: str(Src.X(), MemSrc); break;
    default: LOGMAN_MSG_A_FMT("Unhandled StoreMem size: {}", OpSize); break;
    }
  } else {
    const auto Src = GetVReg(Op->Value);

    switch (OpSize) {
    case IR::OpSize::i8Bit: {
      strb(Src, MemSrc);
      break;
    }
    case IR::OpSize::i16Bit: {
      strh(Src, MemSrc);
      break;
    }
    case IR::OpSize::i32Bit: {
      str(Src.S(), MemSrc);
      break;
    }
    case IR::OpSize::i64Bit: {
      str(Src.D(), MemSrc);
      break;
    }
    case IR::OpSize::i128Bit: {
      str(Src.Q(), MemSrc);
      break;
    }
    case IR::OpSize::i256Bit: {
      LOGMAN_THROW_A_FMT(HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);
      const auto MemSrc = GenerateSVEMemOperand(OpSize, MemReg, Op->Offset, Op->OffsetType, Op->OffsetScale);
      st1b<ARMEmitter::SubRegSize::i8Bit>(Src.Z(), PRED_TMP_32B, MemSrc);
      break;
    }
    default: LOGMAN_MSG_A_FMT("Unhandled StoreMem size: {}", OpSize); break;
    }
  }
}

DEF_OP(StoreMemX87SVEOptPredicate) {
  const auto Op = IROp->C<IR::IROp_StoreMemX87SVEOptPredicate>();
  const auto Predicate = PRED_X87_SVEOPT;

  LOGMAN_THROW_A_FMT(HostSupportsSVE128 || HostSupportsSVE256, "StoreMemX87SVEOptPredicate needs SVE support");

  const auto RegData = GetVReg(Op->Value);
  const auto MemReg = GetReg(Op->Addr);
  const auto MemDst = ARMEmitter::SVEMemOperand(MemReg.X(), 0);

  switch (IROp->ElementSize) {
  case IR::OpSize::i8Bit: {
    st1b<ARMEmitter::SubRegSize::i8Bit>(RegData.Z(), Predicate, MemDst);
    break;
  }
  case IR::OpSize::i16Bit: {
    st1h<ARMEmitter::SubRegSize::i16Bit>(RegData.Z(), Predicate, MemDst);
    break;
  }
  case IR::OpSize::i32Bit: {
    st1w<ARMEmitter::SubRegSize::i32Bit>(RegData.Z(), Predicate, MemDst);
    break;
  }
  case IR::OpSize::i64Bit: {
    st1d(RegData.Z(), Predicate, MemDst);
    break;
  }
  default: LOGMAN_MSG_A_FMT("Unhandled {} element size: {}", __func__, IROp->ElementSize); break;
  }
}

DEF_OP(LoadMemX87SVEOptPredicate) {
  const auto Op = IROp->C<IR::IROp_LoadMemX87SVEOptPredicate>();
  const auto Dst = GetVReg(Node);
  const auto Predicate = PRED_X87_SVEOPT;
  const auto MemReg = GetReg(Op->Addr);

  LOGMAN_THROW_A_FMT(HostSupportsSVE128 || HostSupportsSVE256, "LoadMemX87SVEOptPredicate needs SVE support");

  const auto MemDst = ARMEmitter::SVEMemOperand(MemReg.X(), 0);

  switch (IROp->ElementSize) {
  case IR::OpSize::i8Bit: {
    ld1b<ARMEmitter::SubRegSize::i8Bit>(Dst.Z(), Predicate.Zeroing(), MemDst);
    break;
  }
  case IR::OpSize::i16Bit: {
    ld1h<ARMEmitter::SubRegSize::i16Bit>(Dst.Z(), Predicate.Zeroing(), MemDst);
    break;
  }
  case IR::OpSize::i32Bit: {
    ld1w<ARMEmitter::SubRegSize::i32Bit>(Dst.Z(), Predicate.Zeroing(), MemDst);
    break;
  }
  case IR::OpSize::i64Bit: {
    ld1d(Dst.Z(), Predicate.Zeroing(), MemDst);
    break;
  }
  default: LOGMAN_MSG_A_FMT("Unhandled {} element size: {}", __func__, IROp->ElementSize); break;
  }
}

DEF_OP(StoreMemPair) {
  const auto Op = IROp->C<IR::IROp_StoreMemPair>();
  const auto OpSize = IROp->Size;
  const auto Addr = GetReg(Op->Addr);

  if (Op->Class == IR::RegClass::GPR) {
    const auto Src1 = GetZeroableReg(Op->Value1);
    const auto Src2 = GetZeroableReg(Op->Value2);
    switch (OpSize) {
    case IR::OpSize::i32Bit: stp<ARMEmitter::IndexType::OFFSET>(Src1.W(), Src2.W(), Addr, Op->Offset); break;
    case IR::OpSize::i64Bit: stp<ARMEmitter::IndexType::OFFSET>(Src1.X(), Src2.X(), Addr, Op->Offset); break;
    default: LOGMAN_MSG_A_FMT("Unhandled StoreMem size: {}", OpSize); break;
    }
  } else {
    const auto Src1 = GetVReg(Op->Value1);
    const auto Src2 = GetVReg(Op->Value2);

    switch (OpSize) {
    case IR::OpSize::i32Bit: stp<ARMEmitter::IndexType::OFFSET>(Src1.S(), Src2.S(), Addr, Op->Offset); break;
    case IR::OpSize::i64Bit: stp<ARMEmitter::IndexType::OFFSET>(Src1.D(), Src2.D(), Addr, Op->Offset); break;
    case IR::OpSize::i128Bit: stp<ARMEmitter::IndexType::OFFSET>(Src1.Q(), Src2.Q(), Addr, Op->Offset); break;
    default: LOGMAN_MSG_A_FMT("Unhandled StoreMemPair size: {}", OpSize); break;
    }
  }
}

DEF_OP(StoreMemTSO) {
  const auto Op = IROp->C<IR::IROp_StoreMemTSO>();
  const auto OpSize = IROp->Size;

  const auto MemReg = GetReg(Op->Addr);

  if (Op->Class == IR::RegClass::GPR) {
    LOGMAN_THROW_A_FMT(Op->Offset.IsInvalid() || CTX->HostFeatures.SupportsTSOImm9, "unexpected offset");
    LOGMAN_THROW_A_FMT(Op->OffsetScale == 1, "unexpected offset scale");
    LOGMAN_THROW_A_FMT(Op->OffsetType == IR::MemOffsetType::SXTX, "unexpected offset type");
  }

  if (CTX->HostFeatures.SupportsTSOImm9 && Op->Class == IR::RegClass::GPR) {
    const auto Src = GetZeroableReg(Op->Value);
    uint64_t Offset = 0;
    if (!Op->Offset.IsInvalid()) {
      bool IsInline = IsInlineConstant(Op->Offset, &Offset);
      LOGMAN_THROW_A_FMT(IsInline, "expected immediate");
    }

    if (OpSize == IR::OpSize::i8Bit) {
      // 8bit load is always aligned to natural alignment
      stlurb(Src, MemReg, Offset);
    } else {
      // Half-barrier once back-patched.
      nop();
      switch (OpSize) {
      case IR::OpSize::i16Bit: stlurh(Src, MemReg, Offset); break;
      case IR::OpSize::i32Bit: stlur(Src.W(), MemReg, Offset); break;
      case IR::OpSize::i64Bit: stlur(Src.X(), MemReg, Offset); break;
      default: LOGMAN_MSG_A_FMT("Unhandled StoreMemTSO size: {}", OpSize); break;
      }
    }
  } else if (Op->Class == IR::RegClass::GPR) {
    const auto Src = GetZeroableReg(Op->Value);

    if (OpSize == IR::OpSize::i8Bit) {
      // 8bit load is always aligned to natural alignment
      stlrb(Src, MemReg);
    } else {
      // Half-barrier once back-patched.
      nop();
      switch (OpSize) {
      case IR::OpSize::i16Bit: stlrh(Src, MemReg); break;
      case IR::OpSize::i32Bit: stlr(Src.W(), MemReg); break;
      case IR::OpSize::i64Bit: stlr(Src.X(), MemReg); break;
      default: LOGMAN_MSG_A_FMT("Unhandled StoreMemTSO size: {}", OpSize); break;
      }
    }
  } else {
    if (CTX->IsVectorAtomicTSOEnabled()) {
      // Half-Barrier.
      dmb(ARMEmitter::BarrierScope::ISH);
    }
    const auto Src = GetVReg(Op->Value);
    const auto MemSrc = GenerateMemOperand(OpSize, MemReg, Op->Offset, Op->OffsetType, Op->OffsetScale);
    switch (OpSize) {
    case IR::OpSize::i8Bit: strb(Src, MemSrc); break;
    case IR::OpSize::i16Bit: strh(Src, MemSrc); break;
    case IR::OpSize::i32Bit: str(Src.S(), MemSrc); break;
    case IR::OpSize::i64Bit: str(Src.D(), MemSrc); break;
    case IR::OpSize::i128Bit: str(Src.Q(), MemSrc); break;
    case IR::OpSize::i256Bit: {
      LOGMAN_THROW_A_FMT(HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);
      const auto Operand = GenerateSVEMemOperand(OpSize, MemReg, Op->Offset, Op->OffsetType, Op->OffsetScale);
      st1b<ARMEmitter::SubRegSize::i8Bit>(Src.Z(), PRED_TMP_32B, Operand);
      break;
    }
    default: LOGMAN_MSG_A_FMT("Unhandled StoreMemTSO size: {}", OpSize); break;
    }
  }
}

DEF_OP(MemSet) {
  // TODO: A future looking task would be to support this with ARM's MOPS instructions.
  // The 8-bit non-atomic forward path directly matches ARM's SETP/SETM/SETE instruction,
  // while the backward version needs some fixup to convert it to a forward direction.
  //
  // Assuming non-atomicity and non-faulting behaviour, this can accelerate this implementation.
  // Additionally: This is commonly used as a memset to zero. If we know up-front with an inline constant
  // that the value is zero, we can optimize any operation larger than 8-bit down to 8-bit to use the MOPS implementation.
  const auto Op = IROp->C<IR::IROp_MemSet>();

  const bool IsAtomic = CTX->IsMemcpyAtomicTSOEnabled();
  const auto Size = IR::OpSizeToSize(Op->Size);
  const auto MemReg = GetReg(Op->Addr);
  const auto Value = GetZeroableReg(Op->Value);
  const auto Length = GetReg(Op->Length);
  const auto Dst = GetReg(Node);

  uint64_t DirectionConstant;
  bool DirectionIsInline = IsInlineConstant(Op->Direction, &DirectionConstant);
  ARMEmitter::Register DirectionReg = ARMEmitter::Reg::r0;
  if (!DirectionIsInline) {
    DirectionReg = GetReg(Op->Direction);
  }

  // If Direction > 0 then:
  //   MemReg is incremented (by size)
  // else:
  //   MemReg is decremented (by size)
  //
  // Counter is decremented regardless.

  ARMEmitter::ForwardLabel BackwardImpl {};
  ARMEmitter::ForwardLabel Done {};

  mov(TMP1, Length.X());
  if (Op->Prefix.IsInvalid()) {
    mov(TMP2, MemReg.X());
  } else {
    const auto Prefix = GetReg(Op->Prefix);
    add(TMP2, Prefix.X(), MemReg.X());
  }

  if (!DirectionIsInline) {
    // Backward or forwards implementation depends on flag
    (void)tbnz(DirectionReg, 1, &BackwardImpl);
  }

  auto MemStore = [this](auto Value, uint32_t OpSize, int32_t Size) {
    switch (OpSize) {
    case 1: strb<ARMEmitter::IndexType::POST>(Value.W(), TMP2, Size); break;
    case 2: strh<ARMEmitter::IndexType::POST>(Value.W(), TMP2, Size); break;
    case 4: str<ARMEmitter::IndexType::POST>(Value.W(), TMP2, Size); break;
    case 8: str<ARMEmitter::IndexType::POST>(Value.X(), TMP2, Size); break;
    default: LOGMAN_MSG_A_FMT("Unhandled {} size: {}", __func__, Size); break;
    }
  };

  auto MemStoreTSO = [this](auto Value, uint32_t OpSize, int32_t Size) {
    if (OpSize == 1) {
      // 8bit load is always aligned to natural alignment
      stlrb(Value.W(), TMP2);
    } else {
      nop();
      switch (OpSize) {
      case 2: stlrh(Value.W(), TMP2); break;
      case 4: stlr(Value.W(), TMP2); break;
      case 8: stlr(Value.X(), TMP2); break;
      default: LOGMAN_MSG_A_FMT("Unhandled {} size: {}", __func__, Size); break;
      }
    }

    if (Size >= 0) {
      add(ARMEmitter::Size::i64Bit, TMP2, TMP2, OpSize);
    } else {
      sub(ARMEmitter::Size::i64Bit, TMP2, TMP2, OpSize);
    }
  };

  const auto SubRegSize = Size == 1 ? ARMEmitter::SubRegSize::i8Bit :
                          Size == 2 ? ARMEmitter::SubRegSize::i16Bit :
                          Size == 4 ? ARMEmitter::SubRegSize::i32Bit :
                          Size == 8 ? ARMEmitter::SubRegSize::i64Bit :
                                      ARMEmitter::SubRegSize::i8Bit;

  auto EmitMemset = [&](int32_t Direction) {
    const int32_t OpSize = Size;
    const int32_t SizeDirection = Size * Direction;

    ARMEmitter::BiDirectionalLabel AgainInternal {};
    ARMEmitter::ForwardLabel DoneInternal {};

    // Early exit if zero count.
    (void)cbz(ARMEmitter::Size::i64Bit, TMP1, &DoneInternal);

    if (!IsAtomic) {
      ARMEmitter::ForwardLabel AgainInternal256Exit {};
      ARMEmitter::BackwardLabel AgainInternal256 {};
      ARMEmitter::ForwardLabel AgainInternal128Exit {};
      ARMEmitter::BackwardLabel AgainInternal128 {};

      if (Direction == -1) {
        sub(ARMEmitter::Size::i64Bit, TMP2, TMP2, 32 - Size);
      }

      // Keep the counter one copy ahead, so that underflow can be used to detect when to fallback
      // to the copy unit size copy loop for the last chunk.
      // Do this in two parts, to fallback to the byte by byte loop if size < 32, and to the
      // single copy loop if size < 64.
      sub(ARMEmitter::Size::i64Bit, TMP1, TMP1, 32 / Size);
      (void)tbnz(TMP1, 63, &AgainInternal128Exit);

      // Fill VTMP2 with the set pattern
      dup(SubRegSize, VTMP2.Q(), Value);

      sub(ARMEmitter::Size::i64Bit, TMP1, TMP1, 32 / Size);
      (void)tbnz(TMP1, 63, &AgainInternal256Exit);

      (void)Bind(&AgainInternal256);
      stp<ARMEmitter::IndexType::POST>(VTMP2.Q(), VTMP2.Q(), TMP2, 32 * Direction);
      stp<ARMEmitter::IndexType::POST>(VTMP2.Q(), VTMP2.Q(), TMP2, 32 * Direction);
      sub(ARMEmitter::Size::i64Bit, TMP1, TMP1, 64 / Size);
      (void)tbz(TMP1, 63, &AgainInternal256);

      (void)Bind(&AgainInternal256Exit);
      add(ARMEmitter::Size::i64Bit, TMP1, TMP1, 64 / Size);
      (void)cbz(ARMEmitter::Size::i64Bit, TMP1, &DoneInternal);

      sub(ARMEmitter::Size::i64Bit, TMP1, TMP1, 32 / Size);
      (void)tbnz(TMP1, 63, &AgainInternal128Exit);
      (void)Bind(&AgainInternal128);
      stp<ARMEmitter::IndexType::POST>(VTMP2.Q(), VTMP2.Q(), TMP2, 32 * Direction);
      sub(ARMEmitter::Size::i64Bit, TMP1, TMP1, 32 / Size);
      (void)tbz(TMP1, 63, &AgainInternal128);

      (void)Bind(&AgainInternal128Exit);
      add(ARMEmitter::Size::i64Bit, TMP1, TMP1, 32 / Size);
      (void)cbz(ARMEmitter::Size::i64Bit, TMP1, &DoneInternal);

      if (Direction == -1) {
        add(ARMEmitter::Size::i64Bit, TMP2, TMP2, 32 - Size);
      }
    }

    (void)Bind(&AgainInternal);
    if (IsAtomic) {
      MemStoreTSO(Value, OpSize, SizeDirection);
    } else {
      MemStore(Value, OpSize, SizeDirection);
    }
    sub(ARMEmitter::Size::i64Bit, TMP1, TMP1, 1);
    (void)cbnz(ARMEmitter::Size::i64Bit, TMP1, &AgainInternal);

    (void)Bind(&DoneInternal);

    if (SizeDirection >= 0) {
      switch (OpSize) {
      case 1: add(Dst.X(), MemReg.X(), Length.X()); break;
      case 2: add(Dst.X(), MemReg.X(), Length.X(), ARMEmitter::ShiftType::LSL, 1); break;
      case 4: add(Dst.X(), MemReg.X(), Length.X(), ARMEmitter::ShiftType::LSL, 2); break;
      case 8: add(Dst.X(), MemReg.X(), Length.X(), ARMEmitter::ShiftType::LSL, 3); break;
      default: LOGMAN_MSG_A_FMT("Unhandled {} size: {}", __func__, OpSize); break;
      }
    } else {
      switch (OpSize) {
      case 1: sub(Dst.X(), MemReg.X(), Length.X()); break;
      case 2: sub(Dst.X(), MemReg.X(), Length.X(), ARMEmitter::ShiftType::LSL, 1); break;
      case 4: sub(Dst.X(), MemReg.X(), Length.X(), ARMEmitter::ShiftType::LSL, 2); break;
      case 8: sub(Dst.X(), MemReg.X(), Length.X(), ARMEmitter::ShiftType::LSL, 3); break;
      default: LOGMAN_MSG_A_FMT("Unhandled {} size: {}", __func__, OpSize); break;
      }
    }
  };

  if (DirectionIsInline) {
    LOGMAN_THROW_A_FMT(DirectionConstant == 1 || DirectionConstant == -1, "unexpected direction");
    EmitMemset(DirectionConstant);
  } else {
    // Emit forward direction memset then backward direction memset.
    for (int32_t Direction : {1, -1}) {
      EmitMemset(Direction);

      if (Direction == 1) {
        (void)b(&Done);
        (void)Bind(&BackwardImpl);
      }
    }

    (void)Bind(&Done);
    // Destination already set to the final pointer.
  }
}

DEF_OP(MemCpy) {
  // TODO: A future looking task would be to support this with ARM's MOPS instructions.
  // The 8-bit non-atomic path directly matches ARM's CPYP/CPYM/CPYE instruction,
  //
  // Assuming non-atomicity and non-faulting behaviour, this can accelerate this implementation.
  const auto Op = IROp->C<IR::IROp_MemCpy>();

  const bool IsAtomic = CTX->IsMemcpyAtomicTSOEnabled();
  const auto Size = IR::OpSizeToSize(Op->Size);
  const auto MemRegDest = GetReg(Op->Dest);
  const auto MemRegSrc = GetReg(Op->Src);

  const auto Length = GetReg(Op->Length);
  uint64_t DirectionConstant;
  bool DirectionIsInline = IsInlineConstant(Op->Direction, &DirectionConstant);
  ARMEmitter::Register DirectionReg = ARMEmitter::Reg::r0;
  if (!DirectionIsInline) {
    DirectionReg = GetReg(Op->Direction);
  }

  auto Dst0 = GetReg(Op->OutDstAddress);
  auto Dst1 = GetReg(Op->OutSrcAddress);
  // If Direction > 0 then:
  //   MemRegDest is incremented (by size)
  //   MemRegSrc is incremented (by size)
  // else:
  //   MemRegDest is decremented (by size)
  //   MemRegSrc is decremented (by size)
  //
  // Counter is decremented regardless.

  ARMEmitter::ForwardLabel BackwardImpl {};
  ARMEmitter::ForwardLabel Done {};

  mov(TMP1, Length.X());
  mov(TMP2, MemRegDest.X());
  mov(TMP3, MemRegSrc.X());

  // TMP1 = Length
  // TMP2 = Dest
  // TMP3 = Src
  // TMP4 = load+store temp value

  if (!DirectionIsInline) {
    // Backward or forwards implementation depends on flag
    (void)tbnz(DirectionReg, 1, &BackwardImpl);
  }

  auto MemCpy = [this](uint32_t OpSize, int32_t Size) {
    switch (OpSize) {
    case 1:
      ldrb<ARMEmitter::IndexType::POST>(TMP4.W(), TMP3, Size);
      strb<ARMEmitter::IndexType::POST>(TMP4.W(), TMP2, Size);
      break;
    case 2:
      ldrh<ARMEmitter::IndexType::POST>(TMP4.W(), TMP3, Size);
      strh<ARMEmitter::IndexType::POST>(TMP4.W(), TMP2, Size);
      break;
    case 4:
      ldr<ARMEmitter::IndexType::POST>(TMP4.W(), TMP3, Size);
      str<ARMEmitter::IndexType::POST>(TMP4.W(), TMP2, Size);
      break;
    case 8:
      ldr<ARMEmitter::IndexType::POST>(TMP4, TMP3, Size);
      str<ARMEmitter::IndexType::POST>(TMP4, TMP2, Size);
      break;
    case 32:
      ldp<ARMEmitter::IndexType::POST>(VTMP1.Q(), VTMP2.Q(), TMP3, Size);
      stp<ARMEmitter::IndexType::POST>(VTMP1.Q(), VTMP2.Q(), TMP2, Size);
      break;
    default: LOGMAN_MSG_A_FMT("Unhandled {} size: {}", __func__, Size); break;
    }
  };

  auto MemCpyTSO = [this](uint32_t OpSize, int32_t Size) {
    if (CTX->HostFeatures.SupportsRCPC) {
      if (OpSize == 1) {
        // 8bit load is always aligned to natural alignment
        ldaprb(TMP4.W(), TMP3);
        stlrb(TMP4.W(), TMP2);
      } else {
        switch (OpSize) {
        case 2: ldaprh(TMP4.W(), TMP3); break;
        case 4: ldapr(TMP4.W(), TMP3); break;
        case 8: ldapr(TMP4, TMP3); break;
        default: LOGMAN_MSG_A_FMT("Unhandled {} size: {}", __func__, Size); break;
        }

        // Placeholders for backpatching barriers (one per load/store)
        nop();
        nop();

        switch (OpSize) {
        case 2: stlrh(TMP4.W(), TMP2); break;
        case 4: stlr(TMP4.W(), TMP2); break;
        case 8: stlr(TMP4, TMP2); break;
        default: LOGMAN_MSG_A_FMT("Unhandled {} size: {}", __func__, Size); break;
        }
      }
    } else {
      if (OpSize == 1) {
        // 8bit load is always aligned to natural alignment
        ldarb(TMP4.W(), TMP3);
        stlrb(TMP4.W(), TMP2);
      } else {
        switch (OpSize) {
        case 2: ldarh(TMP4.W(), TMP3); break;
        case 4: ldar(TMP4.W(), TMP3); break;
        case 8: ldar(TMP4, TMP3); break;
        default: LOGMAN_MSG_A_FMT("Unhandled {} size: {}", __func__, Size); break;
        }

        // Placeholders for backpatching barriers (one per load/store)
        nop();
        nop();

        switch (OpSize) {
        case 2: stlrh(TMP4.W(), TMP2); break;
        case 4: stlr(TMP4.W(), TMP2); break;
        case 8: stlr(TMP4, TMP2); break;
        default: LOGMAN_MSG_A_FMT("Unhandled {} size: {}", __func__, Size); break;
        }
      }
    }

    if (Size >= 0) {
      add(ARMEmitter::Size::i64Bit, TMP2, TMP2, OpSize);
      add(ARMEmitter::Size::i64Bit, TMP3, TMP3, OpSize);
    } else {
      sub(ARMEmitter::Size::i64Bit, TMP2, TMP2, OpSize);
      sub(ARMEmitter::Size::i64Bit, TMP3, TMP3, OpSize);
    }
  };

  auto EmitMemcpy = [&](int32_t Direction) {
    const int32_t OpSize = Size;
    const int32_t SizeDirection = Size * Direction;

    ARMEmitter::BiDirectionalLabel AgainInternal {};
    ARMEmitter::ForwardLabel DoneInternal {};

    // Early exit if zero count.
    (void)cbz(ARMEmitter::Size::i64Bit, TMP1, &DoneInternal);

    if (!IsAtomic) {
      ARMEmitter::ForwardLabel AbsPos {};
      ARMEmitter::ForwardLabel AgainInternal256Exit {};
      ARMEmitter::ForwardLabel AgainInternal128Exit {};
      ARMEmitter::BackwardLabel AgainInternal128 {};
      ARMEmitter::BackwardLabel AgainInternal256 {};

      sub(ARMEmitter::Size::i64Bit, TMP4, TMP2, TMP3);
      (void)tbz(TMP4, 63, &AbsPos);
      neg(ARMEmitter::Size::i64Bit, TMP4, TMP4);
      (void)Bind(&AbsPos);
      sub(ARMEmitter::Size::i64Bit, TMP4, TMP4, 32);
      (void)tbnz(TMP4, 63, &AgainInternal);

      if (Direction == -1) {
        sub(ARMEmitter::Size::i64Bit, TMP2, TMP2, 32 - Size);
        sub(ARMEmitter::Size::i64Bit, TMP3, TMP3, 32 - Size);
      }

      // Keep the counter one copy ahead, so that underflow can be used to detect when to fallback
      // to the copy unit size copy loop for the last chunk.
      // Do this in two parts, to fallback to the byte by byte loop if size < 32, and to the
      // single copy loop if size < 64.
      sub(ARMEmitter::Size::i64Bit, TMP1, TMP1, 32 / Size);
      (void)tbnz(TMP1, 63, &AgainInternal128Exit);
      sub(ARMEmitter::Size::i64Bit, TMP1, TMP1, 32 / Size);
      (void)tbnz(TMP1, 63, &AgainInternal256Exit);

      (void)Bind(&AgainInternal256);
      MemCpy(32, 32 * Direction);
      MemCpy(32, 32 * Direction);
      sub(ARMEmitter::Size::i64Bit, TMP1, TMP1, 64 / Size);
      (void)tbz(TMP1, 63, &AgainInternal256);

      (void)Bind(&AgainInternal256Exit);
      add(ARMEmitter::Size::i64Bit, TMP1, TMP1, 64 / Size);
      (void)cbz(ARMEmitter::Size::i64Bit, TMP1, &DoneInternal);

      sub(ARMEmitter::Size::i64Bit, TMP1, TMP1, 32 / Size);
      (void)tbnz(TMP1, 63, &AgainInternal128Exit);
      (void)Bind(&AgainInternal128);
      MemCpy(32, 32 * Direction);
      sub(ARMEmitter::Size::i64Bit, TMP1, TMP1, 32 / Size);
      (void)tbz(TMP1, 63, &AgainInternal128);

      (void)Bind(&AgainInternal128Exit);
      add(ARMEmitter::Size::i64Bit, TMP1, TMP1, 32 / Size);
      (void)cbz(ARMEmitter::Size::i64Bit, TMP1, &DoneInternal);

      if (Direction == -1) {
        add(ARMEmitter::Size::i64Bit, TMP2, TMP2, 32 - Size);
        add(ARMEmitter::Size::i64Bit, TMP3, TMP3, 32 - Size);
      }
    }

    (void)Bind(&AgainInternal);
    if (IsAtomic) {
      MemCpyTSO(OpSize, SizeDirection);
    } else {
      MemCpy(OpSize, SizeDirection);
    }
    sub(ARMEmitter::Size::i64Bit, TMP1, TMP1, 1);
    (void)cbnz(ARMEmitter::Size::i64Bit, TMP1, &AgainInternal);

    (void)Bind(&DoneInternal);

    // Needs to use temporaries just in case of overwrite
    mov(TMP1, MemRegDest.X());
    mov(TMP2, MemRegSrc.X());
    mov(TMP3, Length.X());

    if (SizeDirection >= 0) {
      switch (OpSize) {
      case 1:
        add(Dst0.X(), TMP1, TMP3);
        add(Dst1.X(), TMP2, TMP3);
        break;
      case 2:
        add(Dst0.X(), TMP1, TMP3, ARMEmitter::ShiftType::LSL, 1);
        add(Dst1.X(), TMP2, TMP3, ARMEmitter::ShiftType::LSL, 1);
        break;
      case 4:
        add(Dst0.X(), TMP1, TMP3, ARMEmitter::ShiftType::LSL, 2);
        add(Dst1.X(), TMP2, TMP3, ARMEmitter::ShiftType::LSL, 2);
        break;
      case 8:
        add(Dst0.X(), TMP1, TMP3, ARMEmitter::ShiftType::LSL, 3);
        add(Dst1.X(), TMP2, TMP3, ARMEmitter::ShiftType::LSL, 3);
        break;
      default: LOGMAN_MSG_A_FMT("Unhandled {} size: {}", __func__, OpSize); break;
      }
    } else {
      switch (OpSize) {
      case 1:
        sub(Dst0.X(), TMP1, TMP3);
        sub(Dst1.X(), TMP2, TMP3);
        break;
      case 2:
        sub(Dst0.X(), TMP1, TMP3, ARMEmitter::ShiftType::LSL, 1);
        sub(Dst1.X(), TMP2, TMP3, ARMEmitter::ShiftType::LSL, 1);
        break;
      case 4:
        sub(Dst0.X(), TMP1, TMP3, ARMEmitter::ShiftType::LSL, 2);
        sub(Dst1.X(), TMP2, TMP3, ARMEmitter::ShiftType::LSL, 2);
        break;
      case 8:
        sub(Dst0.X(), TMP1, TMP3, ARMEmitter::ShiftType::LSL, 3);
        sub(Dst1.X(), TMP2, TMP3, ARMEmitter::ShiftType::LSL, 3);
        break;
      default: LOGMAN_MSG_A_FMT("Unhandled {} size: {}", __func__, OpSize); break;
      }
    }
  };

  if (DirectionIsInline) {
    LOGMAN_THROW_A_FMT(DirectionConstant == 1 || DirectionConstant == -1, "unexpected direction");
    EmitMemcpy(DirectionConstant);
  } else {
    // Emit forward direction memset then backward direction memset.
    for (int32_t Direction : {1, -1}) {
      EmitMemcpy(Direction);
      if (Direction == 1) {
        (void)b(&Done);
        (void)Bind(&BackwardImpl);
      }
    }
    (void)Bind(&Done);
    // Destination already set to the final pointer.
  }
}

DEF_OP(CacheLineClear) {
  if (!CTX->HostFeatures.SupportsCacheMaintenanceOps) {
    dmb(ARMEmitter::BarrierScope::SY);
    return;
  }

  auto Op = IROp->C<IR::IROp_CacheLineClear>();

  auto MemReg = GetReg(Op->Addr);

  // Clear dcache only
  // icache doesn't matter here since the guest application shouldn't be calling clflush on JIT code.
  if (CTX->HostFeatures.DCacheLineSize >= 64U) {
    dc(ARMEmitter::DataCacheOperation::CIVAC, MemReg);
  } else {
    auto CurrentWorkingReg = MemReg.X();
    for (size_t i = 0; i < std::max(1U, CTX->HostFeatures.DCacheLineSize / 64U); ++i) {
      dc(ARMEmitter::DataCacheOperation::CIVAC, TMP1);
      add(ARMEmitter::Size::i64Bit, TMP1, CurrentWorkingReg, CTX->HostFeatures.DCacheLineSize);
      CurrentWorkingReg = TMP1;
    }
  }

  if (Op->Serialize) {
    // If requested, serialized all of the data cache operations.
    dsb(ARMEmitter::BarrierScope::ISH);
  }
}

DEF_OP(CacheLineClean) {
  if (!CTX->HostFeatures.SupportsCacheMaintenanceOps) {
    dmb(ARMEmitter::BarrierScope::ST);
    return;
  }

  auto Op = IROp->C<IR::IROp_CacheLineClean>();

  auto MemReg = GetReg(Op->Addr);

  // Clean dcache only
  if (CTX->HostFeatures.DCacheLineSize >= 64U) {
    dc(ARMEmitter::DataCacheOperation::CVAC, MemReg);
  } else {
    auto CurrentWorkingReg = MemReg.X();
    for (size_t i = 0; i < std::max(1U, CTX->HostFeatures.DCacheLineSize / 64U); ++i) {
      dc(ARMEmitter::DataCacheOperation::CVAC, TMP1);
      add(ARMEmitter::Size::i64Bit, TMP1, CurrentWorkingReg, CTX->HostFeatures.DCacheLineSize);
      CurrentWorkingReg = TMP1;
    }
  }
}

DEF_OP(CacheLineZero) {
  auto Op = IROp->C<IR::IROp_CacheLineZero>();

  auto MemReg = GetReg(Op->Addr);

  if (CTX->HostFeatures.SupportsCLZERO) {
    // We can use this instruction directly
    dc(ARMEmitter::DataCacheOperation::ZVA, MemReg);
  } else {
    // We must walk the cacheline ourselves
    // Force cacheline alignment
    and_(ARMEmitter::Size::i64Bit, TMP1, MemReg, ~(CPUIDEmu::CACHELINE_SIZE - 1));
    // This will end up being four STPs
    // Depending on uarch it could be slightly more efficient in instructions emitted
    // and uops to use vector pair STP, but we want the non-temporal bit specifically here
    for (size_t i = 0; i < CPUIDEmu::CACHELINE_SIZE; i += 16) {
      stnp(ARMEmitter::XReg::zr, ARMEmitter::XReg::zr, TMP1, i);
    }
  }
}

DEF_OP(Prefetch) {
  auto Op = IROp->C<IR::IROp_Prefetch>();
  const auto MemReg = GetReg(Op->Addr);

  // Access size is only ever handled as 8-byte. Even though it is accesssed as a cacheline.
  const auto MemSrc = GenerateMemOperand(IR::OpSize::i64Bit, MemReg, Op->Offset, Op->OffsetType, Op->OffsetScale);

  size_t LUT = (Op->Stream ? 1 : 0) | ((Op->CacheLevel - 1) << 1) | (Op->ForStore ? 1U << 3 : 0);

  constexpr static std::array<ARMEmitter::Prefetch, 14> PrefetchType = {
    ARMEmitter::Prefetch::PLDL1KEEP,
    ARMEmitter::Prefetch::PLDL1STRM,

    ARMEmitter::Prefetch::PLDL2KEEP,
    ARMEmitter::Prefetch::PLDL2STRM,

    ARMEmitter::Prefetch::PLDL3KEEP,
    ARMEmitter::Prefetch::PLDL3STRM,

    // Gap of two.
    // 0b0'11'0
    ARMEmitter::Prefetch::PLDL1STRM,
    // 0b0'11'1
    ARMEmitter::Prefetch::PLDL1STRM,

    ARMEmitter::Prefetch::PSTL1KEEP,
    ARMEmitter::Prefetch::PSTL1STRM,

    ARMEmitter::Prefetch::PSTL2KEEP,
    ARMEmitter::Prefetch::PSTL2STRM,

    ARMEmitter::Prefetch::PSTL3KEEP,
    ARMEmitter::Prefetch::PSTL3STRM,
  };

  prfm(PrefetchType[LUT], MemSrc);
}

DEF_OP(VStoreNonTemporal) {
  const auto Op = IROp->C<IR::IROp_VStoreNonTemporal>();
  const auto OpSize = IROp->Size;

  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);
  const auto Is128Bit = OpSize == IR::OpSize::i128Bit;

  const auto Value = GetVReg(Op->Value);
  const auto MemReg = GetReg(Op->Addr);
  const auto Offset = Op->Offset;

  if (Is256Bit) {
    const auto GoverningPredicate = PRED_TMP_32B.Zeroing();
    const auto OffsetScaled = Offset / 32;
    stnt1b(Value.Z(), GoverningPredicate, MemReg, OffsetScaled);
  } else if (Is128Bit && HostSupportsSVE128) {
    const auto GoverningPredicate = PRED_TMP_16B.Zeroing();
    const auto OffsetScaled = Offset / 16;
    stnt1b(Value.Z(), GoverningPredicate, MemReg, OffsetScaled);
  } else {
    // Treat the non-temporal store as a regular vector store in this case for compatibility
    str(Value.Q(), MemReg, Offset);
  }
}

DEF_OP(VStoreNonTemporalPair) {
  const auto Op = IROp->C<IR::IROp_VStoreNonTemporalPair>();
  const auto OpSize = IROp->Size;

  const auto Is128Bit = OpSize == IR::OpSize::i128Bit;
  LOGMAN_THROW_A_FMT(Is128Bit, "This IR operation only operates at 128-bit wide");

  const auto ValueLow = GetVReg(Op->ValueLow);
  const auto ValueHigh = GetVReg(Op->ValueHigh);

  const auto MemReg = GetReg(Op->Addr);
  const auto Offset = Op->Offset;

  stnp(ValueLow.Q(), ValueHigh.Q(), MemReg, Offset);
}

DEF_OP(VLoadNonTemporal) {
  const auto Op = IROp->C<IR::IROp_VLoadNonTemporal>();
  const auto OpSize = IROp->Size;

  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);
  const auto Is128Bit = OpSize == IR::OpSize::i128Bit;

  const auto Dst = GetVReg(Node);
  const auto MemReg = GetReg(Op->Addr);
  const auto Offset = Op->Offset;

  if (Is256Bit) {
    const auto GoverningPredicate = PRED_TMP_32B.Zeroing();
    const auto OffsetScaled = Offset / 32;
    ldnt1b(Dst.Z(), GoverningPredicate, MemReg, OffsetScaled);
  } else if (Is128Bit && HostSupportsSVE128) {
    const auto GoverningPredicate = PRED_TMP_16B.Zeroing();
    const auto OffsetScaled = Offset / 16;
    ldnt1b(Dst.Z(), GoverningPredicate, MemReg, OffsetScaled);
  } else {
    // Treat the non-temporal store as a regular vector store in this case for compatibility
    ldr(Dst.Q(), MemReg, Offset);
  }
}

} // namespace FEXCore::CPU
