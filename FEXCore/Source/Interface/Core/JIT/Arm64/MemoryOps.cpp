// SPDX-License-Identifier: MIT
/*
$info$
tags: backend|arm64
$end_info$
*/

#include "FEXCore/Core/X86Enums.h"
#include "Interface/Context/Context.h"
#include "Interface/Core/CPUID.h"
#include "Interface/Core/JIT/Arm64/JITClass.h"
#include <FEXCore/Utils/CompilerDefs.h>
#include <FEXCore/Utils/MathUtils.h>

namespace FEXCore::CPU {
#define DEF_OP(x) void Arm64JITCore::Op_##x(IR::IROp_Header const* IROp, IR::NodeID Node)

DEF_OP(LoadContext) {
  const auto Op = IROp->C<IR::IROp_LoadContext>();
  const auto OpSize = IROp->Size;

  if (Op->Class == FEXCore::IR::GPRClass) {
    auto Dst = GetReg(Node);

    switch (OpSize) {
    case 1: ldrb(Dst, STATE, Op->Offset); break;
    case 2: ldrh(Dst, STATE, Op->Offset); break;
    case 4: ldr(Dst.W(), STATE, Op->Offset); break;
    case 8: ldr(Dst.X(), STATE, Op->Offset); break;
    default: LOGMAN_MSG_A_FMT("Unhandled LoadContext size: {}", OpSize); break;
    }
  } else {
    auto Dst = GetVReg(Node);

    switch (OpSize) {
    case 1: ldrb(Dst, STATE, Op->Offset); break;
    case 2: ldrh(Dst, STATE, Op->Offset); break;
    case 4: ldr(Dst.S(), STATE, Op->Offset); break;
    case 8: ldr(Dst.D(), STATE, Op->Offset); break;
    case 16: ldr(Dst.Q(), STATE, Op->Offset); break;
    case 32:
      mov(TMP1, Op->Offset);
      ld1b<ARMEmitter::SubRegSize::i8Bit>(Dst.Z(), PRED_TMP_32B.Zeroing(), STATE, TMP1);
      break;
    default: LOGMAN_MSG_A_FMT("Unhandled LoadContext size: {}", OpSize); break;
    }
  }
}

DEF_OP(StoreContext) {
  const auto Op = IROp->C<IR::IROp_StoreContext>();
  const auto OpSize = IROp->Size;

  if (Op->Class == FEXCore::IR::GPRClass) {
    auto Src = GetReg(Op->Value.ID());

    switch (OpSize) {
    case 1: strb(Src, STATE, Op->Offset); break;
    case 2: strh(Src, STATE, Op->Offset); break;
    case 4: str(Src.W(), STATE, Op->Offset); break;
    case 8: str(Src.X(), STATE, Op->Offset); break;
    default: LOGMAN_MSG_A_FMT("Unhandled StoreContext size: {}", OpSize); break;
    }
  } else {
    const auto Src = GetVReg(Op->Value.ID());

    switch (OpSize) {
    case 1: strb(Src, STATE, Op->Offset); break;
    case 2: strh(Src, STATE, Op->Offset); break;
    case 4: str(Src.S(), STATE, Op->Offset); break;
    case 8: str(Src.D(), STATE, Op->Offset); break;
    case 16: str(Src.Q(), STATE, Op->Offset); break;
    case 32:
      mov(TMP1, Op->Offset);
      st1b<ARMEmitter::SubRegSize::i8Bit>(Src.Z(), PRED_TMP_32B, STATE, TMP1);
      break;
    default: LOGMAN_MSG_A_FMT("Unhandled StoreContext size: {}", OpSize); break;
    }
  }
}

DEF_OP(LoadRegister) {
  const auto Op = IROp->C<IR::IROp_LoadRegister>();
  const auto OpSize = IROp->Size;

  if (Op->Class == IR::GPRClass) {
    unsigned Reg = Op->Reg == Core::CPUState::PF_AS_GREG ? (StaticRegisters.size() - 2) :
                   Op->Reg == Core::CPUState::AF_AS_GREG ? (StaticRegisters.size() - 1) :
                                                           Op->Reg;

    LOGMAN_THROW_A_FMT(Reg < StaticRegisters.size(), "out of range reg");
    const auto reg = StaticRegisters[Reg];

    if (GetReg(Node).Idx() != reg.Idx()) {
      if (OpSize == 4) {
        mov(GetReg(Node).W(), reg.W());
      } else {
        mov(GetReg(Node).X(), reg.X());
      }
    }
  } else if (Op->Class == IR::FPRClass) {
    const auto regSize = HostSupportsSVE256 ? Core::CPUState::XMM_AVX_REG_SIZE : Core::CPUState::XMM_SSE_REG_SIZE;
    LOGMAN_THROW_A_FMT(Op->Reg < StaticFPRegisters.size(), "out of range reg");
    LOGMAN_THROW_A_FMT(OpSize == regSize, "expected sized");

    const auto guest = StaticFPRegisters[Op->Reg];
    const auto host = GetVReg(Node);

    if (host.Idx() != guest.Idx()) {
      if (HostSupportsSVE256) {
        mov(ARMEmitter::SubRegSize::i64Bit, host.Z(), PRED_TMP_32B.Merging(), guest.Z());
      } else {
        mov(host.Q(), guest.Q());
      }
    }
  } else {
    LOGMAN_THROW_AA_FMT(false, "Unhandled Op->Class {}", Op->Class);
  }
}

DEF_OP(StoreRegister) {
  const auto Op = IROp->C<IR::IROp_StoreRegister>();
  const auto OpSize = IROp->Size;


  if (Op->Class == IR::GPRClass) {
    unsigned Reg = Op->Reg == Core::CPUState::PF_AS_GREG ? (StaticRegisters.size() - 2) :
                   Op->Reg == Core::CPUState::AF_AS_GREG ? (StaticRegisters.size() - 1) :
                                                           Op->Reg;

    LOGMAN_THROW_A_FMT(Reg < StaticRegisters.size(), "out of range reg");
    const auto reg = StaticRegisters[Reg];
    const auto Src = GetReg(Op->Value.ID());

    if (Src.Idx() != reg.Idx()) {
      // Always use 64-bit, it's faster. Upper bits ignored for 32-bit mode.
      mov(ARMEmitter::Size::i64Bit, reg, Src);
    }
  } else if (Op->Class == IR::FPRClass) {
    const auto regSize = HostSupportsSVE256 ? Core::CPUState::XMM_AVX_REG_SIZE : Core::CPUState::XMM_SSE_REG_SIZE;
    LOGMAN_THROW_A_FMT(Op->Reg < StaticFPRegisters.size(), "reg out of range");
    LOGMAN_THROW_A_FMT(OpSize == regSize, "expected sized");

    const auto guest = StaticFPRegisters[Op->Reg];
    const auto host = GetVReg(Op->Value.ID());

    if (guest.Idx() != host.Idx()) {
      if (HostSupportsSVE256) {
        mov(ARMEmitter::SubRegSize::i64Bit, guest.Z(), PRED_TMP_32B.Merging(), host.Z());
      } else {
        mov(guest.Q(), host.Q());
      }
    }
  } else {
    LOGMAN_THROW_AA_FMT(false, "Unhandled Op->Class {}", Op->Class);
  }
}

DEF_OP(LoadContextIndexed) {
  const auto Op = IROp->C<IR::IROp_LoadContextIndexed>();
  const auto OpSize = IROp->Size;

  const auto Index = GetReg(Op->Index.ID());

  if (Op->Class == FEXCore::IR::GPRClass) {
    switch (Op->Stride) {
    case 1:
    case 2:
    case 4:
    case 8: {
      add(ARMEmitter::Size::i64Bit, TMP1, STATE, Index, ARMEmitter::ShiftType::LSL, FEXCore::ilog2(Op->Stride));
      const auto Dst = GetReg(Node);
      switch (OpSize) {
      case 1: ldrb(Dst, TMP1, Op->BaseOffset); break;
      case 2: ldrh(Dst, TMP1, Op->BaseOffset); break;
      case 4: ldr(Dst.W(), TMP1, Op->BaseOffset); break;
      case 8: ldr(Dst.X(), TMP1, Op->BaseOffset); break;
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
      case 1: ldrb(Dst, TMP1, Op->BaseOffset); break;
      case 2: ldrh(Dst, TMP1, Op->BaseOffset); break;
      case 4: ldr(Dst.S(), TMP1, Op->BaseOffset); break;
      case 8: ldr(Dst.D(), TMP1, Op->BaseOffset); break;
      case 16:
        if (Op->BaseOffset % 16 == 0) {
          ldr(Dst.Q(), TMP1, Op->BaseOffset);
        } else {
          add(ARMEmitter::Size::i64Bit, TMP1, TMP1, Op->BaseOffset);
          ldur(Dst.Q(), TMP1, Op->BaseOffset);
        }
        break;
      case 32:
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

  const auto Index = GetReg(Op->Index.ID());

  if (Op->Class == FEXCore::IR::GPRClass) {
    const auto Value = GetReg(Op->Value.ID());

    switch (Op->Stride) {
    case 1:
    case 2:
    case 4:
    case 8: {
      add(ARMEmitter::Size::i64Bit, TMP1, STATE, Index, ARMEmitter::ShiftType::LSL, FEXCore::ilog2(Op->Stride));

      switch (OpSize) {
      case 1: strb(Value, TMP1, Op->BaseOffset); break;
      case 2: strh(Value, TMP1, Op->BaseOffset); break;
      case 4: str(Value.W(), TMP1, Op->BaseOffset); break;
      case 8: str(Value.X(), TMP1, Op->BaseOffset); break;
      default: LOGMAN_MSG_A_FMT("Unhandled StoreContextIndexed size: {}", OpSize); break;
      }
      break;
    }
    case 16: LOGMAN_MSG_A_FMT("Invalid Class store of size 16"); break;
    default: LOGMAN_MSG_A_FMT("Unhandled StoreContextIndexed stride: {}", Op->Stride); break;
    }
  } else {
    const auto Value = GetVReg(Op->Value.ID());

    switch (Op->Stride) {
    case 1:
    case 2:
    case 4:
    case 8:
    case 16:
    case 32: {
      add(ARMEmitter::Size::i64Bit, TMP1, STATE, Index, ARMEmitter::ShiftType::LSL, FEXCore::ilog2(Op->Stride));

      switch (OpSize) {
      case 1: strb(Value, TMP1, Op->BaseOffset); break;
      case 2: strh(Value, TMP1, Op->BaseOffset); break;
      case 4: str(Value.S(), TMP1, Op->BaseOffset); break;
      case 8: str(Value.D(), TMP1, Op->BaseOffset); break;
      case 16:
        if (Op->BaseOffset % 16 == 0) {
          str(Value.Q(), TMP1, Op->BaseOffset);
        } else {
          add(ARMEmitter::Size::i64Bit, TMP1, TMP1, Op->BaseOffset);
          stur(Value.Q(), TMP1, Op->BaseOffset);
        }
        break;
      case 32:
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

DEF_OP(SpillRegister) {
  const auto Op = IROp->C<IR::IROp_SpillRegister>();
  const uint8_t OpSize = IROp->Size;
  const uint32_t SlotOffset = Op->Slot * MaxSpillSlotSize;

  if (Op->Class == FEXCore::IR::GPRClass) {
    const auto Src = GetReg(Op->Value.ID());
    switch (OpSize) {
    case 1: {
      if (SlotOffset > LSByteMaxUnsignedOffset) {
        LoadConstant(ARMEmitter::Size::i64Bit, TMP1, SlotOffset);
        strb(Src, ARMEmitter::Reg::rsp, TMP1.R(), ARMEmitter::ExtendedType::LSL_64, 0);
      } else {
        strb(Src, ARMEmitter::Reg::rsp, SlotOffset);
      }
      break;
    }
    case 2: {
      if (SlotOffset > LSHalfMaxUnsignedOffset) {
        LoadConstant(ARMEmitter::Size::i64Bit, TMP1, SlotOffset);
        strh(Src, ARMEmitter::Reg::rsp, TMP1.R(), ARMEmitter::ExtendedType::LSL_64, 0);
      } else {
        strh(Src, ARMEmitter::Reg::rsp, SlotOffset);
      }
      break;
    }
    case 4: {
      if (SlotOffset > LSWordMaxUnsignedOffset) {
        LoadConstant(ARMEmitter::Size::i64Bit, TMP1, SlotOffset);
        str(Src.W(), ARMEmitter::Reg::rsp, TMP1.R(), ARMEmitter::ExtendedType::LSL_64, 0);
      } else {
        str(Src.W(), ARMEmitter::Reg::rsp, SlotOffset);
      }
      break;
    }
    case 8: {
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
  } else if (Op->Class == FEXCore::IR::FPRClass) {
    const auto Src = GetVReg(Op->Value.ID());

    switch (OpSize) {
    case 4: {
      if (SlotOffset > LSWordMaxUnsignedOffset) {
        LoadConstant(ARMEmitter::Size::i64Bit, TMP1, SlotOffset);
        str(Src.S(), ARMEmitter::Reg::rsp, TMP1.R(), ARMEmitter::ExtendedType::LSL_64, 0);
      } else {
        str(Src.S(), ARMEmitter::Reg::rsp, SlotOffset);
      }
      break;
    }
    case 8: {
      if (SlotOffset > LSDWordMaxUnsignedOffset) {
        LoadConstant(ARMEmitter::Size::i64Bit, TMP1, SlotOffset);
        str(Src.D(), ARMEmitter::Reg::rsp, TMP1.R(), ARMEmitter::ExtendedType::LSL_64, 0);
      } else {
        str(Src.D(), ARMEmitter::Reg::rsp, SlotOffset);
      }
      break;
    }
    case 16: {
      if (SlotOffset > LSQWordMaxUnsignedOffset) {
        LoadConstant(ARMEmitter::Size::i64Bit, TMP1, SlotOffset);
        str(Src.Q(), ARMEmitter::Reg::rsp, TMP1.R(), ARMEmitter::ExtendedType::LSL_64, 0);
      } else {
        str(Src.Q(), ARMEmitter::Reg::rsp, SlotOffset);
      }
      break;
    }
    case 32: {
      mov(TMP3, SlotOffset);
      st1b<ARMEmitter::SubRegSize::i8Bit>(Src.Z(), PRED_TMP_32B, ARMEmitter::Reg::rsp, TMP3);
      break;
    }
    default: LOGMAN_MSG_A_FMT("Unhandled SpillRegister size: {}", OpSize); break;
    }
  } else if (Op->Class == FEXCore::IR::GPRPairClass) {
    const auto Src = GetRegPair(Op->Value.ID());
    switch (OpSize) {
    case 8: {
      if (SlotOffset <= 252 && (SlotOffset & 0b11) == 0) {
        stp<ARMEmitter::IndexType::OFFSET>(Src.first.W(), Src.second.W(), ARMEmitter::Reg::rsp, SlotOffset);
      } else {
        add(ARMEmitter::Size::i64Bit, TMP1, ARMEmitter::Reg::rsp, SlotOffset);
        stp<ARMEmitter::IndexType::OFFSET>(Src.first.W(), Src.second.W(), TMP1, 0);
      }
      break;
    }

    case 16: {
      if (SlotOffset <= 504 && (SlotOffset & 0b111) == 0) {
        stp<ARMEmitter::IndexType::OFFSET>(Src.first.X(), Src.second.X(), ARMEmitter::Reg::rsp, SlotOffset);
      } else {
        add(ARMEmitter::Size::i64Bit, TMP1, ARMEmitter::Reg::rsp, SlotOffset);
        stp<ARMEmitter::IndexType::OFFSET>(Src.first.X(), Src.second.X(), TMP1, 0);
      }
      break;
    }
    default: LOGMAN_MSG_A_FMT("Unhandled SpillRegister(GPRPair) size: {}", OpSize); break;
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
    const auto Dst = GetReg(Node);
    switch (OpSize) {
    case 1: {
      if (SlotOffset > LSByteMaxUnsignedOffset) {
        LoadConstant(ARMEmitter::Size::i64Bit, TMP1, SlotOffset);
        ldrb(Dst, ARMEmitter::Reg::rsp, TMP1.R(), ARMEmitter::ExtendedType::LSL_64, 0);
      } else {
        ldrb(Dst, ARMEmitter::Reg::rsp, SlotOffset);
      }
      break;
    }
    case 2: {
      if (SlotOffset > LSHalfMaxUnsignedOffset) {
        LoadConstant(ARMEmitter::Size::i64Bit, TMP1, SlotOffset);
        ldrh(Dst, ARMEmitter::Reg::rsp, TMP1.R(), ARMEmitter::ExtendedType::LSL_64, 0);
      } else {
        ldrh(Dst, ARMEmitter::Reg::rsp, SlotOffset);
      }
      break;
    }
    case 4: {
      if (SlotOffset > LSWordMaxUnsignedOffset) {
        LoadConstant(ARMEmitter::Size::i64Bit, TMP1, SlotOffset);
        ldr(Dst.W(), ARMEmitter::Reg::rsp, TMP1.R(), ARMEmitter::ExtendedType::LSL_64, 0);
      } else {
        ldr(Dst.W(), ARMEmitter::Reg::rsp, SlotOffset);
      }
      break;
    }
    case 8: {
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
  } else if (Op->Class == FEXCore::IR::FPRClass) {
    const auto Dst = GetVReg(Node);

    switch (OpSize) {
    case 4: {
      if (SlotOffset > LSWordMaxUnsignedOffset) {
        LoadConstant(ARMEmitter::Size::i64Bit, TMP1, SlotOffset);
        ldr(Dst.S(), ARMEmitter::Reg::rsp, TMP1.R(), ARMEmitter::ExtendedType::LSL_64, 0);
      } else {
        ldr(Dst.S(), ARMEmitter::Reg::rsp, SlotOffset);
      }
      break;
    }
    case 8: {
      if (SlotOffset > LSDWordMaxUnsignedOffset) {
        LoadConstant(ARMEmitter::Size::i64Bit, TMP1, SlotOffset);
        ldr(Dst.D(), ARMEmitter::Reg::rsp, TMP1.R(), ARMEmitter::ExtendedType::LSL_64, 0);
      } else {
        ldr(Dst.D(), ARMEmitter::Reg::rsp, SlotOffset);
      }
      break;
    }
    case 16: {
      if (SlotOffset > LSQWordMaxUnsignedOffset) {
        LoadConstant(ARMEmitter::Size::i64Bit, TMP1, SlotOffset);
        ldr(Dst.Q(), ARMEmitter::Reg::rsp, TMP1.R(), ARMEmitter::ExtendedType::LSL_64, 0);
      } else {
        ldr(Dst.Q(), ARMEmitter::Reg::rsp, SlotOffset);
      }
      break;
    }
    case 32: {
      mov(TMP3, SlotOffset);
      ld1b<ARMEmitter::SubRegSize::i8Bit>(Dst.Z(), PRED_TMP_32B.Zeroing(), ARMEmitter::Reg::rsp, TMP3);
      break;
    }
    default: LOGMAN_MSG_A_FMT("Unhandled FillRegister size: {}", OpSize); break;
    }
  } else if (Op->Class == FEXCore::IR::GPRPairClass) {
    const auto Src = GetRegPair(Node);
    switch (OpSize) {
    case 8: {
      if (SlotOffset <= 252 && (SlotOffset & 0b11) == 0) {
        ldp<ARMEmitter::IndexType::OFFSET>(Src.first.W(), Src.second.W(), ARMEmitter::Reg::rsp, SlotOffset);
      } else {
        add(ARMEmitter::Size::i64Bit, TMP1, ARMEmitter::Reg::rsp, SlotOffset);
        ldp<ARMEmitter::IndexType::OFFSET>(Src.first.W(), Src.second.W(), TMP1, 0);
      }
      break;
    }

    case 16: {
      if (SlotOffset <= 504 && (SlotOffset & 0b111) == 0) {
        ldp<ARMEmitter::IndexType::OFFSET>(Src.first.X(), Src.second.X(), ARMEmitter::Reg::rsp, SlotOffset);
      } else {
        add(ARMEmitter::Size::i64Bit, TMP1, ARMEmitter::Reg::rsp, SlotOffset);
        ldp<ARMEmitter::IndexType::OFFSET>(Src.first.X(), Src.second.X(), TMP1, 0);
      }
      break;
    }
    default: LOGMAN_MSG_A_FMT("Unhandled FillRegister(GPRPair) size: {}", OpSize); break;
    }
  } else {
    LOGMAN_MSG_A_FMT("Unhandled FillRegister class: {}", Op->Class.Val);
  }
}

DEF_OP(LoadNZCV) {
  auto Dst = GetReg(Node);

  mrs(Dst, ARMEmitter::SystemRegister::NZCV);
}

DEF_OP(StoreNZCV) {
  auto Op = IROp->C<IR::IROp_StoreNZCV>();

  msr(ARMEmitter::SystemRegister::NZCV, GetReg(Op->Value.ID()));
}

DEF_OP(LoadDF) {
  auto Dst = GetReg(Node);
  auto Flag = X86State::RFLAG_DF_RAW_LOC;

  // DF needs sign extension to turn 0x1/0xFF into 1/-1
  ldrsb(Dst.X(), STATE, offsetof(FEXCore::Core::CPUState, flags[Flag]));
}

DEF_OP(LoadFlag) {
  auto Op = IROp->C<IR::IROp_LoadFlag>();
  auto Dst = GetReg(Node);

  LOGMAN_THROW_A_FMT(Op->Flag != X86State::RFLAG_PF_RAW_LOC && Op->Flag != X86State::RFLAG_AF_RAW_LOC, "PF/AF must be accessed as "
                                                                                                       "registers");

  ldrb(Dst, STATE, offsetof(FEXCore::Core::CPUState, flags[0]) + Op->Flag);
}

DEF_OP(StoreFlag) {
  auto Op = IROp->C<IR::IROp_StoreFlag>();

  LOGMAN_THROW_A_FMT(Op->Flag != X86State::RFLAG_PF_RAW_LOC && Op->Flag != X86State::RFLAG_AF_RAW_LOC, "PF/AF must be accessed as "
                                                                                                       "registers");

  strb(GetReg(Op->Value.ID()), STATE, offsetof(FEXCore::Core::CPUState, flags[0]) + Op->Flag);
}

ARMEmitter::ExtendedMemOperand Arm64JITCore::GenerateMemOperand(
  uint8_t AccessSize, ARMEmitter::Register Base, IR::OrderedNodeWrapper Offset, IR::MemOffsetType OffsetType, uint8_t OffsetScale) {
  if (Offset.IsInvalid()) {
    return ARMEmitter::ExtendedMemOperand(Base.X(), ARMEmitter::IndexType::OFFSET, 0);
  } else {
    if (OffsetScale != 1 && OffsetScale != AccessSize) {
      LOGMAN_MSG_A_FMT("Unhandled GenerateMemOperand OffsetScale: {}", OffsetScale);
    }
    uint64_t Const;
    if (IsInlineConstant(Offset, &Const)) {
      return ARMEmitter::ExtendedMemOperand(Base.X(), ARMEmitter::IndexType::OFFSET, Const);
    } else {
      auto RegOffset = GetReg(Offset.ID());
      switch (OffsetType.Val) {
      case IR::MEM_OFFSET_SXTX.Val:
        return ARMEmitter::ExtendedMemOperand(Base.X(), RegOffset.X(), ARMEmitter::ExtendedType::SXTX, FEXCore::ilog2(OffsetScale));
      case IR::MEM_OFFSET_UXTW.Val:
        return ARMEmitter::ExtendedMemOperand(Base.X(), RegOffset.X(), ARMEmitter::ExtendedType::UXTW, FEXCore::ilog2(OffsetScale));
      case IR::MEM_OFFSET_SXTW.Val:
        return ARMEmitter::ExtendedMemOperand(Base.X(), RegOffset.X(), ARMEmitter::ExtendedType::SXTW, FEXCore::ilog2(OffsetScale));
      default: LOGMAN_MSG_A_FMT("Unhandled GenerateMemOperand OffsetType: {}", OffsetType.Val); break;
      }
    }
  }

  FEX_UNREACHABLE;
}

ARMEmitter::SVEMemOperand Arm64JITCore::GenerateSVEMemOperand(uint8_t AccessSize, ARMEmitter::Register Base, IR::OrderedNodeWrapper Offset,
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
    const auto SignedAVXSize = static_cast<int64_t>(Core::CPUState::XMM_AVX_REG_SIZE);

    const auto IsCleanlyDivisible = (SignedConst % SignedAVXSize) == 0;
    const auto Index = SignedConst / SignedAVXSize;

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
  LOGMAN_THROW_A_FMT(OffsetType.Val == IR::MEM_OFFSET_SXTX.Val, "Currently only the default offset type (SXTX) is supported.");

  const auto RegOffset = GetReg(Offset.ID());
  return ARMEmitter::SVEMemOperand(Base.X(), RegOffset.X());
}

DEF_OP(LoadMem) {
  const auto Op = IROp->C<IR::IROp_LoadMem>();
  const auto OpSize = IROp->Size;

  const auto MemReg = GetReg(Op->Addr.ID());
  const auto MemSrc = GenerateMemOperand(OpSize, MemReg, Op->Offset, Op->OffsetType, Op->OffsetScale);

  if (Op->Class == FEXCore::IR::GPRClass) {
    const auto Dst = GetReg(Node);

    switch (OpSize) {
    case 1: ldrb(Dst, MemSrc); break;
    case 2: ldrh(Dst, MemSrc); break;
    case 4: ldr(Dst.W(), MemSrc); break;
    case 8: ldr(Dst.X(), MemSrc); break;
    default: LOGMAN_MSG_A_FMT("Unhandled LoadMem size: {}", OpSize); break;
    }
  } else {
    const auto Dst = GetVReg(Node);

    switch (OpSize) {
    case 1: ldrb(Dst, MemSrc); break;
    case 2: ldrh(Dst, MemSrc); break;
    case 4: ldr(Dst.S(), MemSrc); break;
    case 8: ldr(Dst.D(), MemSrc); break;
    case 16: ldr(Dst.Q(), MemSrc); break;
    case 32: {
      const auto Operand = GenerateSVEMemOperand(OpSize, MemReg, Op->Offset, Op->OffsetType, Op->OffsetScale);
      ld1b<ARMEmitter::SubRegSize::i8Bit>(Dst.Z(), PRED_TMP_32B.Zeroing(), Operand);
      break;
    }
    default: LOGMAN_MSG_A_FMT("Unhandled LoadMem size: {}", OpSize); break;
    }
  }
}

DEF_OP(LoadMemTSO) {
  const auto Op = IROp->C<IR::IROp_LoadMemTSO>();
  const auto OpSize = IROp->Size;

  const auto MemReg = GetReg(Op->Addr.ID());

  if (CTX->HostFeatures.SupportsTSOImm9 && Op->Class == FEXCore::IR::GPRClass) {
    const auto Dst = GetReg(Node);
    uint64_t Offset = 0;
    if (!Op->Offset.IsInvalid()) {
      (void)IsInlineConstant(Op->Offset, &Offset);
    }

    if (OpSize == 1) {
      // 8bit load is always aligned to natural alignment
      const auto Dst = GetReg(Node);
      ldapurb(Dst, MemReg, Offset);
    } else {
      switch (OpSize) {
      case 2: ldapurh(Dst, MemReg, Offset); break;
      case 4: ldapur(Dst.W(), MemReg, Offset); break;
      case 8: ldapur(Dst.X(), MemReg, Offset); break;
      default: LOGMAN_MSG_A_FMT("Unhandled LoadMemTSO size: {}", OpSize); break;
      }
      // Half-barrier once back-patched.
      nop();
    }
  } else if (CTX->HostFeatures.SupportsRCPC && Op->Class == FEXCore::IR::GPRClass) {
    const auto Dst = GetReg(Node);
    if (OpSize == 1) {
      // 8bit load is always aligned to natural alignment
      ldaprb(Dst.W(), MemReg);
    } else {
      switch (OpSize) {
      case 2: ldaprh(Dst.W(), MemReg); break;
      case 4: ldapr(Dst.W(), MemReg); break;
      case 8: ldapr(Dst.X(), MemReg); break;
      default: LOGMAN_MSG_A_FMT("Unhandled LoadMemTSO size: {}", OpSize); break;
      }
      // Half-barrier once back-patched.
      nop();
    }
  } else if (Op->Class == FEXCore::IR::GPRClass) {
    const auto Dst = GetReg(Node);
    if (OpSize == 1) {
      // 8bit load is always aligned to natural alignment
      ldarb(Dst, MemReg);
    } else {
      switch (OpSize) {
      case 2: ldarh(Dst, MemReg); break;
      case 4: ldar(Dst.W(), MemReg); break;
      case 8: ldar(Dst.X(), MemReg); break;
      default: LOGMAN_MSG_A_FMT("Unhandled LoadMemTSO size: {}", OpSize); break;
      }
      // Half-barrier once back-patched.
      nop();
    }
  } else {
    const auto Dst = GetVReg(Node);
    const auto MemSrc = GenerateMemOperand(OpSize, MemReg, Op->Offset, Op->OffsetType, Op->OffsetScale);
    switch (OpSize) {
    case 1: ldrb(Dst, MemSrc); break;
    case 2: ldrh(Dst, MemSrc); break;
    case 4: ldr(Dst.S(), MemSrc); break;
    case 8: ldr(Dst.D(), MemSrc); break;
    case 16: ldr(Dst.Q(), MemSrc); break;
    case 32: {
      const auto MemSrc = GenerateSVEMemOperand(OpSize, MemReg, Op->Offset, Op->OffsetType, Op->OffsetScale);
      ld1b<ARMEmitter::SubRegSize::i8Bit>(Dst.Z(), PRED_TMP_32B.Zeroing(), MemSrc);
      break;
    }
    default: LOGMAN_MSG_A_FMT("Unhandled LoadMemTSO size: {}", OpSize); break;
    }
    if (VectorTSOEnabled()) {
      // Half-barrier.
      dmb(ARMEmitter::BarrierScope::ISHLD);
    }
  }
}

DEF_OP(VLoadVectorMasked) {
  LOGMAN_THROW_A_FMT(HostSupportsSVE256, "Need SVE support in order to use VLoadVectorMasked");

  const auto Op = IROp->C<IR::IROp_VLoadVectorMasked>();
  const auto OpSize = IROp->Size;

  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;
  const auto SubRegSize = ConvertSubRegSize8(IROp);

  const auto CMPPredicate = ARMEmitter::PReg::p0;
  const auto GoverningPredicate = Is256Bit ? PRED_TMP_32B : PRED_TMP_16B;

  const auto Dst = GetVReg(Node);
  const auto MaskReg = GetVReg(Op->Mask.ID());
  const auto MemReg = GetReg(Op->Addr.ID());
  const auto MemSrc = GenerateSVEMemOperand(OpSize, MemReg, Op->Offset, Op->OffsetType, Op->OffsetScale);

  // Check if the sign bit is set for the given element size.
  cmplt(SubRegSize, CMPPredicate, GoverningPredicate.Zeroing(), MaskReg.Z(), 0);

  switch (IROp->ElementSize) {
  case 1: {
    ld1b<ARMEmitter::SubRegSize::i8Bit>(Dst.Z(), CMPPredicate.Zeroing(), MemSrc);
    break;
  }
  case 2: {
    ld1h<ARMEmitter::SubRegSize::i16Bit>(Dst.Z(), CMPPredicate.Zeroing(), MemSrc);
    break;
  }
  case 4: {
    ld1w<ARMEmitter::SubRegSize::i32Bit>(Dst.Z(), CMPPredicate.Zeroing(), MemSrc);
    break;
  }
  case 8: {
    ld1d(Dst.Z(), CMPPredicate.Zeroing(), MemSrc);
    break;
  }
  default: break;
  }
}

DEF_OP(VStoreVectorMasked) {
  LOGMAN_THROW_A_FMT(HostSupportsSVE256, "Need SVE support in order to use VStoreVectorMasked");

  const auto Op = IROp->C<IR::IROp_VStoreVectorMasked>();
  const auto OpSize = IROp->Size;

  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;
  const auto SubRegSize = ConvertSubRegSize8(IROp);

  const auto CMPPredicate = ARMEmitter::PReg::p0;
  const auto GoverningPredicate = Is256Bit ? PRED_TMP_32B : PRED_TMP_16B;

  const auto RegData = GetVReg(Op->Data.ID());
  const auto MaskReg = GetVReg(Op->Mask.ID());
  const auto MemReg = GetReg(Op->Addr.ID());
  const auto MemDst = GenerateSVEMemOperand(OpSize, MemReg, Op->Offset, Op->OffsetType, Op->OffsetScale);

  // Check if the sign bit is set for the given element size.
  cmplt(SubRegSize, CMPPredicate, GoverningPredicate.Zeroing(), MaskReg.Z(), 0);

  switch (IROp->ElementSize) {
  case 1: {
    st1b<ARMEmitter::SubRegSize::i8Bit>(RegData.Z(), CMPPredicate.Zeroing(), MemDst);
    break;
  }
  case 2: {
    st1h<ARMEmitter::SubRegSize::i16Bit>(RegData.Z(), CMPPredicate.Zeroing(), MemDst);
    break;
  }
  case 4: {
    st1w<ARMEmitter::SubRegSize::i32Bit>(RegData.Z(), CMPPredicate.Zeroing(), MemDst);
    break;
  }
  case 8: {
    st1d(RegData.Z(), CMPPredicate.Zeroing(), MemDst);
    break;
  }
  default: break;
  }
}

DEF_OP(VLoadVectorElement) {
  const auto Op = IROp->C<IR::IROp_VLoadVectorElement>();
  const auto OpSize = IROp->Size;

  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;
  const auto ElementSize = IROp->ElementSize;

  const auto Dst = GetVReg(Node);
  const auto DstSrc = GetVReg(Op->DstSrc.ID());
  const auto MemReg = GetReg(Op->Addr.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 1 || ElementSize == 2 || ElementSize == 4 || ElementSize == 8 || ElementSize == 16, "Invalid element "
                                                                                                                         "size");

  if (Is256Bit) {
    LOGMAN_MSG_A_FMT("Unsupported 256-bit VLoadVectorElement");
  } else {
    if (Dst != DstSrc && ElementSize != 16) {
      mov(Dst.Q(), DstSrc.Q());
    }
    switch (ElementSize) {
    case 1: ld1<ARMEmitter::SubRegSize::i8Bit>(Dst.Q(), Op->Index, MemReg); break;
    case 2: ld1<ARMEmitter::SubRegSize::i16Bit>(Dst.Q(), Op->Index, MemReg); break;
    case 4: ld1<ARMEmitter::SubRegSize::i32Bit>(Dst.Q(), Op->Index, MemReg); break;
    case 8: ld1<ARMEmitter::SubRegSize::i64Bit>(Dst.Q(), Op->Index, MemReg); break;
    case 16: ldr(Dst.Q(), MemReg); break;
    default: LOGMAN_MSG_A_FMT("Unhandled {} size: {}", __func__, ElementSize); return;
    }
  }

  // Emit a half-barrier if TSO is enabled.
  if (CTX->IsAtomicTSOEnabled() && VectorTSOEnabled()) {
    dmb(ARMEmitter::BarrierScope::ISHLD);
  }
}

DEF_OP(VStoreVectorElement) {
  const auto Op = IROp->C<IR::IROp_VStoreVectorElement>();
  const auto OpSize = IROp->Size;

  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;
  const auto ElementSize = IROp->ElementSize;

  const auto Value = GetVReg(Op->Value.ID());
  const auto MemReg = GetReg(Op->Addr.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 1 || ElementSize == 2 || ElementSize == 4 || ElementSize == 8 || ElementSize == 16, "Invalid element "
                                                                                                                         "size");

  // Emit a half-barrier if TSO is enabled.
  if (CTX->IsAtomicTSOEnabled() && VectorTSOEnabled()) {
    dmb(ARMEmitter::BarrierScope::ISH);
  }

  if (Is256Bit) {
    LOGMAN_MSG_A_FMT("Unsupported 256-bit {}", __func__);
  } else {
    switch (ElementSize) {
    case 1: st1<ARMEmitter::SubRegSize::i8Bit>(Value.Q(), Op->Index, MemReg); break;
    case 2: st1<ARMEmitter::SubRegSize::i16Bit>(Value.Q(), Op->Index, MemReg); break;
    case 4: st1<ARMEmitter::SubRegSize::i32Bit>(Value.Q(), Op->Index, MemReg); break;
    case 8: st1<ARMEmitter::SubRegSize::i64Bit>(Value.Q(), Op->Index, MemReg); break;
    case 16: str(Value.Q(), MemReg); break;
    default: LOGMAN_MSG_A_FMT("Unhandled {} size: {}", __func__, ElementSize); return;
    }
  }
}

DEF_OP(VBroadcastFromMem) {
  const auto Op = IROp->C<IR::IROp_VBroadcastFromMem>();
  const auto OpSize = IROp->Size;

  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;
  const auto ElementSize = IROp->ElementSize;

  const auto Dst = GetVReg(Node);
  const auto MemReg = GetReg(Op->Address.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 1 || ElementSize == 2 || ElementSize == 4 || ElementSize == 8 || ElementSize == 16, "Invalid element "
                                                                                                                         "size");

  if (Is256Bit && !HostSupportsSVE256) {
    LOGMAN_MSG_A_FMT("{}: 256-bit vectors must support SVE256", __func__);
    return;
  }

  if (Is256Bit && HostSupportsSVE256) {
    const auto GoverningPredicate = PRED_TMP_32B.Zeroing();

    switch (ElementSize) {
    case 1: ld1rb(ARMEmitter::SubRegSize::i8Bit, Dst.Z(), GoverningPredicate, MemReg); break;
    case 2: ld1rh(ARMEmitter::SubRegSize::i16Bit, Dst.Z(), GoverningPredicate, MemReg); break;
    case 4: ld1rw(ARMEmitter::SubRegSize::i32Bit, Dst.Z(), GoverningPredicate, MemReg); break;
    case 8: ld1rd(Dst.Z(), GoverningPredicate, MemReg); break;
    case 16: ld1rqb(Dst.Z(), GoverningPredicate, MemReg); break;
    default: LOGMAN_MSG_A_FMT("Unhandled VBroadcastFromMem size: {}", ElementSize); return;
    }
  } else {
    switch (ElementSize) {
    case 1: ld1r<ARMEmitter::SubRegSize::i8Bit>(Dst.Q(), MemReg); break;
    case 2: ld1r<ARMEmitter::SubRegSize::i16Bit>(Dst.Q(), MemReg); break;
    case 4: ld1r<ARMEmitter::SubRegSize::i32Bit>(Dst.Q(), MemReg); break;
    case 8: ld1r<ARMEmitter::SubRegSize::i64Bit>(Dst.Q(), MemReg); break;
    case 16:
      // Normal load, like ld1rqb with 128-bit regs.
      ldr(Dst.Q(), MemReg);
      break;
    default: LOGMAN_MSG_A_FMT("Unhandled VBroadcastFromMem size: {}", ElementSize); return;
    }
  }

  // Emit a half-barrier if TSO is enabled.
  if (CTX->IsAtomicTSOEnabled() && VectorTSOEnabled()) {
    dmb(ARMEmitter::BarrierScope::ISHLD);
  }
}

DEF_OP(Push) {
  const auto Op = IROp->C<IR::IROp_Push>();
  const auto ValueSize = Op->ValueSize;
  auto Src = GetReg(Op->Value.ID());
  const auto AddrSrc = GetReg(Op->Addr.ID());
  const auto Dst = GetReg(Node);

  bool NeedsMoveAfterwards = false;
  if (Dst != AddrSrc) {
    if (Dst == Src) {
      NeedsMoveAfterwards = true;
      // Need to be careful here, incoming source might be reused afterwards.
    } else {
      // RA constraints would let this always be true.
      mov(IROp->Size == 8 ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit, Dst, AddrSrc);
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

    sub(IROp->Size == 8 ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit, Dst, AddrSrc, ValueSize);
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

DEF_OP(StoreMem) {
  const auto Op = IROp->C<IR::IROp_StoreMem>();
  const auto OpSize = IROp->Size;

  const auto MemReg = GetReg(Op->Addr.ID());
  const auto MemSrc = GenerateMemOperand(OpSize, MemReg, Op->Offset, Op->OffsetType, Op->OffsetScale);

  if (Op->Class == FEXCore::IR::GPRClass) {
    const auto Src = GetReg(Op->Value.ID());
    switch (OpSize) {
    case 1: strb(Src, MemSrc); break;
    case 2: strh(Src, MemSrc); break;
    case 4: str(Src.W(), MemSrc); break;
    case 8: str(Src.X(), MemSrc); break;
    default: LOGMAN_MSG_A_FMT("Unhandled StoreMem size: {}", OpSize); break;
    }
  } else {
    const auto Src = GetVReg(Op->Value.ID());

    switch (OpSize) {
    case 1: {
      strb(Src, MemSrc);
      break;
    }
    case 2: {
      strh(Src, MemSrc);
      break;
    }
    case 4: {
      str(Src.S(), MemSrc);
      break;
    }
    case 8: {
      str(Src.D(), MemSrc);
      break;
    }
    case 16: {
      str(Src.Q(), MemSrc);
      break;
    }
    case 32: {
      const auto MemSrc = GenerateSVEMemOperand(OpSize, MemReg, Op->Offset, Op->OffsetType, Op->OffsetScale);
      st1b<ARMEmitter::SubRegSize::i8Bit>(Src.Z(), PRED_TMP_32B, MemSrc);
      break;
    }
    default: LOGMAN_MSG_A_FMT("Unhandled StoreMem size: {}", OpSize); break;
    }
  }
}

DEF_OP(StoreMemTSO) {
  const auto Op = IROp->C<IR::IROp_StoreMemTSO>();
  const auto OpSize = IROp->Size;

  const auto MemReg = GetReg(Op->Addr.ID());

  if (CTX->HostFeatures.SupportsTSOImm9 && Op->Class == FEXCore::IR::GPRClass) {
    const auto Src = GetReg(Op->Value.ID());
    uint64_t Offset = 0;
    if (!Op->Offset.IsInvalid()) {
      (void)IsInlineConstant(Op->Offset, &Offset);
    }

    if (OpSize == 1) {
      // 8bit load is always aligned to natural alignment
      stlurb(Src, MemReg, Offset);
    } else {
      // Half-barrier once back-patched.
      nop();
      switch (OpSize) {
      case 2: stlurh(Src, MemReg, Offset); break;
      case 4: stlur(Src.W(), MemReg, Offset); break;
      case 8: stlur(Src.X(), MemReg, Offset); break;
      default: LOGMAN_MSG_A_FMT("Unhandled StoreMemTSO size: {}", OpSize); break;
      }
    }
  } else if (Op->Class == FEXCore::IR::GPRClass) {
    const auto Src = GetReg(Op->Value.ID());

    if (OpSize == 1) {
      // 8bit load is always aligned to natural alignment
      stlrb(Src, MemReg);
    } else {
      // Half-barrier once back-patched.
      nop();
      switch (OpSize) {
      case 2: stlrh(Src, MemReg); break;
      case 4: stlr(Src.W(), MemReg); break;
      case 8: stlr(Src.X(), MemReg); break;
      default: LOGMAN_MSG_A_FMT("Unhandled StoreMemTSO size: {}", OpSize); break;
      }
    }
  } else {
    if (VectorTSOEnabled()) {
      // Half-Barrier.
      dmb(ARMEmitter::BarrierScope::ISH);
    }
    const auto Src = GetVReg(Op->Value.ID());
    const auto MemSrc = GenerateMemOperand(OpSize, MemReg, Op->Offset, Op->OffsetType, Op->OffsetScale);
    switch (OpSize) {
    case 1: strb(Src, MemSrc); break;
    case 2: strh(Src, MemSrc); break;
    case 4: str(Src.S(), MemSrc); break;
    case 8: str(Src.D(), MemSrc); break;
    case 16: str(Src.Q(), MemSrc); break;
    case 32: {
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

  const bool IsAtomic = Op->IsAtomic && MemcpySetTSOEnabled();
  const int32_t Size = Op->Size;
  const auto MemReg = GetReg(Op->Addr.ID());
  const auto Value = GetReg(Op->Value.ID());
  const auto Length = GetReg(Op->Length.ID());
  const auto Dst = GetReg(Node);

  uint64_t DirectionConstant;
  bool DirectionIsInline = IsInlineConstant(Op->Direction, &DirectionConstant);
  ARMEmitter::Register DirectionReg = ARMEmitter::Reg::r0;
  if (!DirectionIsInline) {
    DirectionReg = GetReg(Op->Direction.ID());
  }

  // If Direction > 0 then:
  //   MemReg is incremented (by size)
  // else:
  //   MemReg is decremented (by size)
  //
  // Counter is decremented regardless.

  ARMEmitter::SingleUseForwardLabel BackwardImpl {};
  ARMEmitter::SingleUseForwardLabel Done {};

  mov(TMP1, Length.X());
  if (Op->Prefix.IsInvalid()) {
    mov(TMP2, MemReg.X());
  } else {
    const auto Prefix = GetReg(Op->Prefix.ID());
    add(TMP2, Prefix.X(), MemReg.X());
  }

  if (!DirectionIsInline) {
    // Backward or forwards implementation depends on flag
    tbnz(DirectionReg, 1, &BackwardImpl);
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
      nop();
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
    cbz(ARMEmitter::Size::i64Bit, TMP1, &DoneInternal);

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
      tbnz(TMP1, 63, &AgainInternal128Exit);

      // Fill VTMP2 with the set pattern
      dup(SubRegSize, VTMP2.Q(), Value);

      sub(ARMEmitter::Size::i64Bit, TMP1, TMP1, 32 / Size);
      tbnz(TMP1, 63, &AgainInternal256Exit);

      Bind(&AgainInternal256);
      stp<ARMEmitter::IndexType::POST>(VTMP2.Q(), VTMP2.Q(), TMP2, 32 * Direction);
      stp<ARMEmitter::IndexType::POST>(VTMP2.Q(), VTMP2.Q(), TMP2, 32 * Direction);
      sub(ARMEmitter::Size::i64Bit, TMP1, TMP1, 64 / Size);
      tbz(TMP1, 63, &AgainInternal256);

      Bind(&AgainInternal256Exit);
      add(ARMEmitter::Size::i64Bit, TMP1, TMP1, 64 / Size);
      cbz(ARMEmitter::Size::i64Bit, TMP1, &DoneInternal);

      sub(ARMEmitter::Size::i64Bit, TMP1, TMP1, 32 / Size);
      tbnz(TMP1, 63, &AgainInternal128Exit);
      Bind(&AgainInternal128);
      stp<ARMEmitter::IndexType::POST>(VTMP2.Q(), VTMP2.Q(), TMP2, 32 * Direction);
      sub(ARMEmitter::Size::i64Bit, TMP1, TMP1, 32 / Size);
      tbz(TMP1, 63, &AgainInternal128);

      Bind(&AgainInternal128Exit);
      add(ARMEmitter::Size::i64Bit, TMP1, TMP1, 32 / Size);
      cbz(ARMEmitter::Size::i64Bit, TMP1, &DoneInternal);

      if (Direction == -1) {
        add(ARMEmitter::Size::i64Bit, TMP2, TMP2, 32 - Size);
      }
    }

    Bind(&AgainInternal);
    if (IsAtomic) {
      MemStoreTSO(Value, OpSize, SizeDirection);
    } else {
      MemStore(Value, OpSize, SizeDirection);
    }
    sub(ARMEmitter::Size::i64Bit, TMP1, TMP1, 1);
    cbnz(ARMEmitter::Size::i64Bit, TMP1, &AgainInternal);

    Bind(&DoneInternal);

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
    LOGMAN_THROW_AA_FMT(DirectionConstant == 1 || DirectionConstant == -1, "unexpected direction");
    EmitMemset(DirectionConstant);
  } else {
    // Emit forward direction memset then backward direction memset.
    for (int32_t Direction : {1, -1}) {
      EmitMemset(Direction);

      if (Direction == 1) {
        b(&Done);
        Bind(&BackwardImpl);
      }
    }

    Bind(&Done);
    // Destination already set to the final pointer.
  }
}

DEF_OP(MemCpy) {
  // TODO: A future looking task would be to support this with ARM's MOPS instructions.
  // The 8-bit non-atomic path directly matches ARM's CPYP/CPYM/CPYE instruction,
  //
  // Assuming non-atomicity and non-faulting behaviour, this can accelerate this implementation.
  const auto Op = IROp->C<IR::IROp_MemCpy>();

  const bool IsAtomic = Op->IsAtomic && MemcpySetTSOEnabled();
  const int32_t Size = Op->Size;
  const auto MemRegDest = GetReg(Op->Dest.ID());
  const auto MemRegSrc = GetReg(Op->Src.ID());

  const auto Length = GetReg(Op->Length.ID());
  uint64_t DirectionConstant;
  bool DirectionIsInline = IsInlineConstant(Op->Direction, &DirectionConstant);
  ARMEmitter::Register DirectionReg = ARMEmitter::Reg::r0;
  if (!DirectionIsInline) {
    DirectionReg = GetReg(Op->Direction.ID());
  }

  auto Dst = GetRegPair(Node);
  // If Direction > 0 then:
  //   MemRegDest is incremented (by size)
  //   MemRegSrc is incremented (by size)
  // else:
  //   MemRegDest is decremented (by size)
  //   MemRegSrc is decremented (by size)
  //
  // Counter is decremented regardless.

  ARMEmitter::SingleUseForwardLabel BackwardImpl {};
  ARMEmitter::SingleUseForwardLabel Done {};

  mov(TMP1, Length.X());
  mov(TMP2, MemRegDest.X());
  mov(TMP3, MemRegSrc.X());

  // TMP1 = Length
  // TMP2 = Dest
  // TMP3 = Src
  // TMP4 = load+store temp value

  if (!DirectionIsInline) {
    // Backward or forwards implementation depends on flag
    tbnz(DirectionReg, 1, &BackwardImpl);
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
        nop();
        switch (OpSize) {
        case 2: ldaprh(TMP4.W(), TMP3); break;
        case 4: ldapr(TMP4.W(), TMP3); break;
        case 8: ldapr(TMP4, TMP3); break;
        default: LOGMAN_MSG_A_FMT("Unhandled {} size: {}", __func__, Size); break;
        }
        nop();

        nop();
        switch (OpSize) {
        case 2: stlrh(TMP4.W(), TMP2); break;
        case 4: stlr(TMP4.W(), TMP2); break;
        case 8: stlr(TMP4, TMP2); break;
        default: LOGMAN_MSG_A_FMT("Unhandled {} size: {}", __func__, Size); break;
        }
        nop();
      }
    } else {
      if (OpSize == 1) {
        // 8bit load is always aligned to natural alignment
        ldarb(TMP4.W(), TMP3);
        stlrb(TMP4.W(), TMP2);
      } else {
        nop();
        switch (OpSize) {
        case 2: ldarh(TMP4.W(), TMP3); break;
        case 4: ldar(TMP4.W(), TMP3); break;
        case 8: ldar(TMP4, TMP3); break;
        default: LOGMAN_MSG_A_FMT("Unhandled {} size: {}", __func__, Size); break;
        }
        nop();

        nop();
        switch (OpSize) {
        case 2: stlrh(TMP4.W(), TMP2); break;
        case 4: stlr(TMP4.W(), TMP2); break;
        case 8: stlr(TMP4, TMP2); break;
        default: LOGMAN_MSG_A_FMT("Unhandled {} size: {}", __func__, Size); break;
        }
        nop();
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
    cbz(ARMEmitter::Size::i64Bit, TMP1, &DoneInternal);

    if (!IsAtomic) {
      ARMEmitter::ForwardLabel AbsPos {};
      ARMEmitter::ForwardLabel AgainInternal256Exit {};
      ARMEmitter::ForwardLabel AgainInternal128Exit {};
      ARMEmitter::BackwardLabel AgainInternal128 {};
      ARMEmitter::BackwardLabel AgainInternal256 {};

      sub(ARMEmitter::Size::i64Bit, TMP4, TMP2, TMP3);
      tbz(TMP4, 63, &AbsPos);
      neg(ARMEmitter::Size::i64Bit, TMP4, TMP4);
      Bind(&AbsPos);
      sub(ARMEmitter::Size::i64Bit, TMP4, TMP4, 32);
      tbnz(TMP4, 63, &AgainInternal);

      if (Direction == -1) {
        sub(ARMEmitter::Size::i64Bit, TMP2, TMP2, 32 - Size);
        sub(ARMEmitter::Size::i64Bit, TMP3, TMP3, 32 - Size);
      }

      // Keep the counter one copy ahead, so that underflow can be used to detect when to fallback
      // to the copy unit size copy loop for the last chunk.
      // Do this in two parts, to fallback to the byte by byte loop if size < 32, and to the
      // single copy loop if size < 64.
      sub(ARMEmitter::Size::i64Bit, TMP1, TMP1, 32 / Size);
      tbnz(TMP1, 63, &AgainInternal128Exit);
      sub(ARMEmitter::Size::i64Bit, TMP1, TMP1, 32 / Size);
      tbnz(TMP1, 63, &AgainInternal256Exit);

      Bind(&AgainInternal256);
      MemCpy(32, 32 * Direction);
      MemCpy(32, 32 * Direction);
      sub(ARMEmitter::Size::i64Bit, TMP1, TMP1, 64 / Size);
      tbz(TMP1, 63, &AgainInternal256);

      Bind(&AgainInternal256Exit);
      add(ARMEmitter::Size::i64Bit, TMP1, TMP1, 64 / Size);
      cbz(ARMEmitter::Size::i64Bit, TMP1, &DoneInternal);

      sub(ARMEmitter::Size::i64Bit, TMP1, TMP1, 32 / Size);
      tbnz(TMP1, 63, &AgainInternal128Exit);
      Bind(&AgainInternal128);
      MemCpy(32, 32 * Direction);
      sub(ARMEmitter::Size::i64Bit, TMP1, TMP1, 32 / Size);
      tbz(TMP1, 63, &AgainInternal128);

      Bind(&AgainInternal128Exit);
      add(ARMEmitter::Size::i64Bit, TMP1, TMP1, 32 / Size);
      cbz(ARMEmitter::Size::i64Bit, TMP1, &DoneInternal);

      if (Direction == -1) {
        add(ARMEmitter::Size::i64Bit, TMP2, TMP2, 32 - Size);
        add(ARMEmitter::Size::i64Bit, TMP3, TMP3, 32 - Size);
      }
    }

    Bind(&AgainInternal);
    if (IsAtomic) {
      MemCpyTSO(OpSize, SizeDirection);
    } else {
      MemCpy(OpSize, SizeDirection);
    }
    sub(ARMEmitter::Size::i64Bit, TMP1, TMP1, 1);
    cbnz(ARMEmitter::Size::i64Bit, TMP1, &AgainInternal);

    Bind(&DoneInternal);

    // Needs to use temporaries just in case of overwrite
    mov(TMP1, MemRegDest.X());
    mov(TMP2, MemRegSrc.X());
    mov(TMP3, Length.X());

    if (SizeDirection >= 0) {
      switch (OpSize) {
      case 1:
        add(Dst.first.X(), TMP1, TMP3);
        add(Dst.second.X(), TMP2, TMP3);
        break;
      case 2:
        add(Dst.first.X(), TMP1, TMP3, ARMEmitter::ShiftType::LSL, 1);
        add(Dst.second.X(), TMP2, TMP3, ARMEmitter::ShiftType::LSL, 1);
        break;
      case 4:
        add(Dst.first.X(), TMP1, TMP3, ARMEmitter::ShiftType::LSL, 2);
        add(Dst.second.X(), TMP2, TMP3, ARMEmitter::ShiftType::LSL, 2);
        break;
      case 8:
        add(Dst.first.X(), TMP1, TMP3, ARMEmitter::ShiftType::LSL, 3);
        add(Dst.second.X(), TMP2, TMP3, ARMEmitter::ShiftType::LSL, 3);
        break;
      default: LOGMAN_MSG_A_FMT("Unhandled {} size: {}", __func__, OpSize); break;
      }
    } else {
      switch (OpSize) {
      case 1:
        sub(Dst.first.X(), TMP1, TMP3);
        sub(Dst.second.X(), TMP2, TMP3);
        break;
      case 2:
        sub(Dst.first.X(), TMP1, TMP3, ARMEmitter::ShiftType::LSL, 1);
        sub(Dst.second.X(), TMP2, TMP3, ARMEmitter::ShiftType::LSL, 1);
        break;
      case 4:
        sub(Dst.first.X(), TMP1, TMP3, ARMEmitter::ShiftType::LSL, 2);
        sub(Dst.second.X(), TMP2, TMP3, ARMEmitter::ShiftType::LSL, 2);
        break;
      case 8:
        sub(Dst.first.X(), TMP1, TMP3, ARMEmitter::ShiftType::LSL, 3);
        sub(Dst.second.X(), TMP2, TMP3, ARMEmitter::ShiftType::LSL, 3);
        break;
      default: LOGMAN_MSG_A_FMT("Unhandled {} size: {}", __func__, OpSize); break;
      }
    }
  };

  if (DirectionIsInline) {
    LOGMAN_THROW_AA_FMT(DirectionConstant == 1 || DirectionConstant == -1, "unexpected direction");
    EmitMemcpy(DirectionConstant);
  } else {
    // Emit forward direction memset then backward direction memset.
    for (int32_t Direction : {1, -1}) {
      EmitMemcpy(Direction);
      if (Direction == 1) {
        b(&Done);
        Bind(&BackwardImpl);
      }
    }
    Bind(&Done);
    // Destination already set to the final pointer.
  }
}

DEF_OP(ParanoidLoadMemTSO) {
  const auto Op = IROp->C<IR::IROp_LoadMemTSO>();
  const auto OpSize = IROp->Size;

  const auto MemReg = GetReg(Op->Addr.ID());

  if (CTX->HostFeatures.SupportsTSOImm9 && Op->Class == FEXCore::IR::GPRClass) {
    const auto Dst = GetReg(Node);
    uint64_t Offset = 0;
    if (!Op->Offset.IsInvalid()) {
      (void)IsInlineConstant(Op->Offset, &Offset);
    }

    if (OpSize == 1) {
      // 8bit load is always aligned to natural alignment
      const auto Dst = GetReg(Node);
      ldapurb(Dst, MemReg, Offset);
    } else {
      switch (OpSize) {
      case 2: ldapurh(Dst, MemReg, Offset); break;
      case 4: ldapur(Dst.W(), MemReg, Offset); break;
      case 8: ldapur(Dst.X(), MemReg, Offset); break;
      default: LOGMAN_MSG_A_FMT("Unhandled ParanoidLoadMemTSO size: {}", OpSize); break;
      }
    }
  } else if (CTX->HostFeatures.SupportsRCPC && Op->Class == FEXCore::IR::GPRClass) {
    const auto Dst = GetReg(Node);
    if (OpSize == 1) {
      // 8bit load is always aligned to natural alignment
      ldaprb(Dst.W(), MemReg);
    } else {
      switch (OpSize) {
      case 2: ldaprh(Dst.W(), MemReg); break;
      case 4: ldapr(Dst.W(), MemReg); break;
      case 8: ldapr(Dst.X(), MemReg); break;
      default: LOGMAN_MSG_A_FMT("Unhandled ParanoidLoadMemTSO size: {}", OpSize); break;
      }
    }
  } else if (Op->Class == FEXCore::IR::GPRClass) {
    const auto Dst = GetReg(Node);
    switch (OpSize) {
    case 1: ldarb(Dst, MemReg); break;
    case 2: ldarh(Dst, MemReg); break;
    case 4: ldar(Dst.W(), MemReg); break;
    case 8: ldar(Dst.X(), MemReg); break;
    default: LOGMAN_MSG_A_FMT("Unhandled ParanoidLoadMemTSO size: {}", OpSize); break;
    }
  } else {
    const auto Dst = GetVReg(Node);
    switch (OpSize) {
    case 1:
      ldarb(TMP1, MemReg);
      fmov(ARMEmitter::Size::i32Bit, Dst.S(), TMP1.W());
      break;
    case 2:
      ldarh(TMP1, MemReg);
      fmov(ARMEmitter::Size::i32Bit, Dst.S(), TMP1.W());
      break;
    case 4:
      ldar(TMP1.W(), MemReg);
      fmov(ARMEmitter::Size::i32Bit, Dst.S(), TMP1.W());
      break;
    case 8:
      ldar(TMP1, MemReg);
      fmov(ARMEmitter::Size::i64Bit, Dst.D(), TMP1);
      break;
    case 16:
      ldaxp(ARMEmitter::Size::i64Bit, TMP1, TMP2, MemReg);
      clrex();
      ins(ARMEmitter::SubRegSize::i64Bit, Dst, 0, TMP1);
      ins(ARMEmitter::SubRegSize::i64Bit, Dst, 1, TMP2);
      break;
    case 32:
      dmb(ARMEmitter::BarrierScope::ISH);
      ld1b<ARMEmitter::SubRegSize::i8Bit>(Dst.Z(), PRED_TMP_32B.Zeroing(), MemReg);
      dmb(ARMEmitter::BarrierScope::ISH);
      break;
    default: LOGMAN_MSG_A_FMT("Unhandled ParanoidLoadMemTSO size: {}", OpSize); break;
    }
  }
}

DEF_OP(ParanoidStoreMemTSO) {
  const auto Op = IROp->C<IR::IROp_StoreMemTSO>();
  const auto OpSize = IROp->Size;

  const auto MemReg = GetReg(Op->Addr.ID());

  if (CTX->HostFeatures.SupportsTSOImm9 && Op->Class == FEXCore::IR::GPRClass) {
    const auto Src = GetReg(Op->Value.ID());
    uint64_t Offset = 0;
    if (!Op->Offset.IsInvalid()) {
      (void)IsInlineConstant(Op->Offset, &Offset);
    }

    if (OpSize == 1) {
      // 8bit load is always aligned to natural alignment
      stlurb(Src, MemReg, Offset);
    } else {
      switch (OpSize) {
      case 2: stlurh(Src, MemReg, Offset); break;
      case 4: stlur(Src.W(), MemReg, Offset); break;
      case 8: stlur(Src.X(), MemReg, Offset); break;
      default: LOGMAN_MSG_A_FMT("Unhandled ParanoidStoreMemTSO size: {}", OpSize); break;
      }
    }
  } else if (Op->Class == FEXCore::IR::GPRClass) {
    const auto Src = GetReg(Op->Value.ID());
    switch (OpSize) {
    case 1: stlrb(Src, MemReg); break;
    case 2: stlrh(Src, MemReg); break;
    case 4: stlr(Src.W(), MemReg); break;
    case 8: stlr(Src.X(), MemReg); break;
    default: LOGMAN_MSG_A_FMT("Unhandled ParanoidStoreMemTSO size: {}", OpSize); break;
    }
  } else {
    const auto Src = GetVReg(Op->Value.ID());

    switch (OpSize) {
    case 1:
      umov<ARMEmitter::SubRegSize::i8Bit>(TMP1, Src, 0);
      stlrb(TMP1, MemReg);
      break;
    case 2:
      umov<ARMEmitter::SubRegSize::i16Bit>(TMP1, Src, 0);
      stlrh(TMP1, MemReg);
      break;
    case 4:
      umov<ARMEmitter::SubRegSize::i32Bit>(TMP1, Src, 0);
      stlr(TMP1.W(), MemReg);
      break;
    case 8:
      umov<ARMEmitter::SubRegSize::i64Bit>(TMP1, Src, 0);
      stlr(TMP1, MemReg);
      break;
    case 16: {
      // Move vector to GPRs
      umov<ARMEmitter::SubRegSize::i64Bit>(TMP1, Src, 0);
      umov<ARMEmitter::SubRegSize::i64Bit>(TMP2, Src, 1);
      ARMEmitter::BackwardLabel B;
      Bind(&B);

      // ldaxp must not have both the destination registers be the same
      ldaxp(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::zr, TMP3, MemReg); // <- Can hit SIGBUS. Overwritten with DMB
      stlxp(ARMEmitter::Size::i64Bit, TMP3, TMP1, TMP2, MemReg);          // <- Can also hit SIGBUS
      cbnz(ARMEmitter::Size::i64Bit, TMP3, &B);                           // < Overwritten with DMB
      break;
    }
    case 32: {
      dmb(ARMEmitter::BarrierScope::ISH);
      st1b<ARMEmitter::SubRegSize::i8Bit>(Src.Z(), PRED_TMP_32B, MemReg, 0);
      dmb(ARMEmitter::BarrierScope::ISH);
      break;
    }
    default: LOGMAN_MSG_A_FMT("Unhandled ParanoidStoreMemTSO size: {}", OpSize); break;
    }
  }
}

DEF_OP(CacheLineClear) {
  auto Op = IROp->C<IR::IROp_CacheLineClear>();

  auto MemReg = GetReg(Op->Addr.ID());

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
  auto Op = IROp->C<IR::IROp_CacheLineClean>();

  auto MemReg = GetReg(Op->Addr.ID());

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

  auto MemReg = GetReg(Op->Addr.ID());

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
  const auto MemReg = GetReg(Op->Addr.ID());

  // Access size is only ever handled as 8-byte. Even though it is accesssed as a cacheline.
  const auto MemSrc = GenerateMemOperand(8, MemReg, Op->Offset, Op->OffsetType, Op->OffsetScale);

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

#undef DEF_OP
} // namespace FEXCore::CPU
