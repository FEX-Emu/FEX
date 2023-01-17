/*
$info$
tags: backend|arm64
$end_info$
*/

#include "Interface/Context/Context.h"
#include "Interface/Core/ArchHelpers/CodeEmitter/Emitter.h"
#include "Interface/Core/ArchHelpers/CodeEmitter/Registers.h"
#include "Interface/Core/CPUID.h"
#include "Interface/Core/JIT/Arm64/JITClass.h"
#include <FEXCore/Utils/CompilerDefs.h>

namespace FEXCore::CPU {
#define DEF_OP(x) void Arm64JITCore::Op_##x(IR::IROp_Header const *IROp, IR::NodeID Node)

DEF_OP(LoadContext) {
  const auto Op = IROp->C<IR::IROp_LoadContext>();
  const auto OpSize = IROp->Size;

  if (Op->Class == FEXCore::IR::GPRClass) {
    auto Dst = GetReg(Node);

    switch (OpSize) {
    case 1:
      ldrb(Dst, STATE, Op->Offset);
      break;
    case 2:
      ldrh(Dst, STATE, Op->Offset);
      break;
    case 4:
      ldr(Dst.W(), STATE, Op->Offset);
      break;
    case 8:
      ldr(Dst.X(), STATE, Op->Offset);
      break;
    default:
      LOGMAN_MSG_A_FMT("Unhandled LoadContext size: {}", OpSize);
      break;
    }
  }
  else {
    auto Dst = GetVReg(Node);

    switch (OpSize) {
    case 1:
      ldrb(Dst, STATE, Op->Offset);
      break;
    case 2:
      ldrh(Dst, STATE, Op->Offset);
      break;
    case 4:
      ldr(Dst.S(), STATE, Op->Offset);
      break;
    case 8:
      ldr(Dst.D(), STATE, Op->Offset);
      break;
    case 16:
      ldr(Dst.Q(), STATE, Op->Offset);
      break;
    case 32:
      mov(TMP1, Op->Offset);
      ld1b<ARMEmitter::SubRegSize::i8Bit>(Dst.Z(), PRED_TMP_32B.Zeroing(), STATE, TMP1);
      break;
    default:
      LOGMAN_MSG_A_FMT("Unhandled LoadContext size: {}", OpSize);
      break;
    }
  }
}

DEF_OP(StoreContext) {
  const auto Op = IROp->C<IR::IROp_StoreContext>();
  const auto OpSize = IROp->Size;

  if (Op->Class == FEXCore::IR::GPRClass) {
    auto Src =  GetReg(Op->Value.ID());

    switch (OpSize) {
    case 1:
      strb(Src, STATE, Op->Offset);
      break;
    case 2:
      strh(Src, STATE, Op->Offset);
      break;
    case 4:
      str(Src.W(), STATE, Op->Offset);
      break;
    case 8:
      str(Src.X(), STATE, Op->Offset);
      break;
    default:
      LOGMAN_MSG_A_FMT("Unhandled StoreContext size: {}", OpSize);
      break;
    }
  }
  else {
    const auto Src = GetVReg(Op->Value.ID());

    switch (OpSize) {
    case 1:
      strb(Src, STATE, Op->Offset);
      break;
    case 2:
      strh(Src, STATE, Op->Offset);
      break;
    case 4:
      str(Src.S(), STATE, Op->Offset);
      break;
    case 8:
      str(Src.D(), STATE, Op->Offset);
      break;
    case 16:
      str(Src.Q(), STATE, Op->Offset);
      break;
    case 32:
      mov(TMP1, Op->Offset);
      st1b<ARMEmitter::SubRegSize::i8Bit>(Src.Z(), PRED_TMP_32B, STATE, TMP1);
      break;
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
    const auto regId = (Op->Offset - offsetof(Core::CpuStateFrame, State.gregs[0])) / Core::CPUState::GPR_REG_SIZE;
    const auto regOffs = Op->Offset & 7;

    LOGMAN_THROW_A_FMT(regId < SRA64.size(), "out of range regId");

    const auto reg = SRA64[regId];

    switch (OpSize) {
      case 1:
        LOGMAN_THROW_AA_FMT(regOffs == 0 || regOffs == 1, "unexpected regOffs");
        ubfx(ARMEmitter::Size::i64Bit, GetReg(Node), reg, regOffs * 8, 8);
        break;

      case 2:
        LOGMAN_THROW_AA_FMT(regOffs == 0, "unexpected regOffs");
        ubfx(ARMEmitter::Size::i64Bit, GetReg(Node), reg, 0, 16);
        break;

      case 4:
        LOGMAN_THROW_AA_FMT(regOffs == 0, "unexpected regOffs");
        if (GetReg(Node).Idx() != reg.Idx())
          mov(GetReg(Node).W(), reg.W());
        break;

      case 8:
        LOGMAN_THROW_AA_FMT(regOffs == 0, "unexpected regOffs");
        if (GetReg(Node).Idx() != reg.Idx()) {
          mov(GetReg(Node).X(), reg.X());
        }
        break;

      default:
        LOGMAN_MSG_A_FMT("Unhandled LoadRegister GPR size: {}", OpSize);
        break;
    }
  }
  else if (Op->Class == IR::FPRClass) {
    const auto regSize = HostSupportsSVE ? Core::CPUState::XMM_AVX_REG_SIZE
                                         : Core::CPUState::XMM_SSE_REG_SIZE;
    const auto regId = (Op->Offset - offsetof(Core::CpuStateFrame, State.xmm.avx.data[0][0])) / regSize;

    LOGMAN_THROW_A_FMT(regId < SRAFPR.size(), "out of range regId");

    const auto guest = SRAFPR[regId];
    const auto host = GetVReg(Node);

    if (HostSupportsSVE) {
      const auto regOffs = Op->Offset & 31;

      ARMEmitter::ForwardLabel DataLocation;
      const auto LoadPredicate = [this, &DataLocation] {
        const auto Predicate = ARMEmitter::PReg::p0;
        adr(TMP1, &DataLocation);
        ldr(Predicate, TMP1);
        return Predicate.Merging();
      };

      const auto EmitData = [this, &DataLocation](uint32_t Value) {
        ARMEmitter::ForwardLabel PastConstant;
        b(&PastConstant);
        Bind(&DataLocation);
        dc32(Value);
        Bind(&PastConstant);
      };

      switch (OpSize) {
        case 1: {
          LOGMAN_THROW_AA_FMT(regOffs == 0, "unexpected regOffs: {}", regOffs);
          dup(ARMEmitter::ScalarRegSize::i8Bit, host, guest, 0);
          break;
        }
        case 2: {
          LOGMAN_THROW_AA_FMT(regOffs == 0, "unexpected regOffs: {}", regOffs);
          fmov(host.H(), guest.H());
          break;
        }
        case 4: {
          LOGMAN_THROW_AA_FMT((regOffs & 3) == 0, "unexpected regOffs: {}", regOffs);
          if (regOffs == 0) {
            if (host.Idx() != guest.Idx()) {
              fmov(host.S(), guest.S());
            }
          } else {
            const auto Predicate = LoadPredicate();

            dup(FEXCore::ARMEmitter::SubRegSize::i32Bit, VTMP1.Z(), host.Z(), 0);
            mov(FEXCore::ARMEmitter::SubRegSize::i32Bit, guest.Z(), Predicate, VTMP1.Z());

            EmitData(1U << regOffs);
          }
          break;
        }

        case 8: {
          LOGMAN_THROW_AA_FMT((regOffs & 7) == 0, "unexpected regOffs: {}", regOffs);
          if (regOffs == 0) {
            if (host.Idx() != guest.Idx()) {
              dup(ARMEmitter::ScalarRegSize::i64Bit, host, guest, 0);
            }
          } else {
            const auto Predicate = LoadPredicate();

            dup(FEXCore::ARMEmitter::SubRegSize::i64Bit, VTMP1.Z(), host.Z(), 0);
            mov(FEXCore::ARMEmitter::SubRegSize::i64Bit, guest.Z(), Predicate, VTMP1.Z());

            EmitData(1U << regOffs);
          }
          break;
        }

        case 16: {
          LOGMAN_THROW_AA_FMT(regOffs == 0, "unexpected regOffs: {}", regOffs);
          if (host.Idx() != guest.Idx()) {
            mov(host.Q(), guest.Q());
          }
          break;
        }

        case 32: {
          LOGMAN_THROW_AA_FMT(regOffs == 0, "unexpected regOffs: {}", regOffs);
          if (host.Idx() != guest.Idx()) {
            mov(ARMEmitter::SubRegSize::i64Bit, host.Z(), PRED_TMP_32B.Merging(), guest.Z());
          }
          break;
        }

        default:
          LOGMAN_MSG_A_FMT("Unhandled LoadRegister FPR size: {}", OpSize);
          break;
      }
    } else {
      const auto regOffs = Op->Offset & 15;

      switch (OpSize) {
        case 1:
          LOGMAN_THROW_AA_FMT(regOffs == 0, "unexpected regOffs: {}", regOffs);
          dup(ARMEmitter::ScalarRegSize::i8Bit, host, guest, 0);
          break;

        case 2:
          LOGMAN_THROW_AA_FMT(regOffs == 0, "unexpected regOffs: {}", regOffs);
          fmov(host.H(), guest.H());
          break;

        case 4:
          LOGMAN_THROW_AA_FMT((regOffs & 3) == 0, "unexpected regOffs: {}", regOffs);
          if (regOffs == 0) {
            if (host.Idx() != guest.Idx()) {
              fmov(host.S(), guest.S());
            }
          } else {
            ins(ARMEmitter::SubRegSize::i32Bit, host, 0, guest, regOffs/4);
          }
          break;

        case 8:
          LOGMAN_THROW_AA_FMT((regOffs & 7) == 0, "unexpected regOffs: {}", regOffs);
          if (regOffs == 0) {
            if (host.Idx() != guest.Idx()) {
              dup(ARMEmitter::ScalarRegSize::i64Bit, host, guest, 0);
            }
          } else {
            ins(ARMEmitter::SubRegSize::i64Bit, host, 0, guest, regOffs/8);
          }
          break;

        case 16:
          LOGMAN_THROW_AA_FMT(regOffs == 0, "unexpected regOffs: {}", regOffs);
          if (host.Idx() != guest.Idx()) {
            mov(host.Q(), guest.Q());
          }
          break;

        default:
          LOGMAN_MSG_A_FMT("Unhandled LoadRegister FPR size: {}", OpSize);
          break;
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
    const auto regId = (Op->Offset / Core::CPUState::GPR_REG_SIZE) - 1;
    const auto regOffs = Op->Offset & 7;

    LOGMAN_THROW_A_FMT(regId < SRA64.size(), "out of range regId");

    const auto reg = SRA64[regId];
    const auto Src = GetReg(Op->Value.ID());

    switch (OpSize) {
      case 1:
        LOGMAN_THROW_AA_FMT(regOffs == 0 || regOffs == 1, "unexpected regOffs");
        bfi(ARMEmitter::Size::i64Bit, reg, Src, regOffs * 8, 8);
        break;

      case 2:
        LOGMAN_THROW_AA_FMT(regOffs == 0, "unexpected regOffs");
        bfi(ARMEmitter::Size::i64Bit, reg, Src, 0, 16);
        break;

      case 4:
        LOGMAN_THROW_AA_FMT(regOffs == 0, "unexpected regOffs");
        bfi(ARMEmitter::Size::i64Bit, reg, Src, 0, 32);
        break;
      case 8:
        LOGMAN_THROW_AA_FMT(regOffs == 0, "unexpected regOffs");
        if (Src.Idx() != reg.Idx()) {
          mov(ARMEmitter::Size::i64Bit, reg, Src);
        }
        break;

      default:
        LOGMAN_MSG_A_FMT("Unhandled StoreRegister GPR size: {}", OpSize);
        break;
    }
  } else if (Op->Class == IR::FPRClass) {
    const auto regSize = HostSupportsSVE ? Core::CPUState::XMM_AVX_REG_SIZE
                                         : Core::CPUState::XMM_SSE_REG_SIZE;
    const auto regId = (Op->Offset - offsetof(Core::CpuStateFrame, State.xmm.avx.data[0][0])) / regSize;

    LOGMAN_THROW_A_FMT(regId < SRAFPR.size(), "regId out of range");

    const auto guest = SRAFPR[regId];
    const auto host = GetVReg(Op->Value.ID());

    if (HostSupportsSVE) {
      // 256-bit capable hardware allows us to expand the allowed
      // offsets used, however we cannot use Adv. SIMD's INS instruction
      // at all, since it will zero out the upper lanes of the 256-bit SVE
      // vectors, so we'll need to set up a proper predicate for performing
      // the insert.

      const auto regOffs = Op->Offset & 31;

      // Compartmentalized setting up of the predicate for the cases that need it.
      ARMEmitter::ForwardLabel DataLocation;
      const auto LoadPredicate = [this, &DataLocation] {
        const auto Predicate = ARMEmitter::PReg::p0;
        adr(TMP1, &DataLocation);
        ldr(Predicate, TMP1);
        return Predicate.Merging();
      };

      // Emits the predicate data and provides the necessary jump to go around the
      // emitted data instead of trying to execute it. Place at end of necessary code.
      // It's helpful to treat LoadPredicate and EmitData as a prologue and epilogue
      // respectfully.
      const auto EmitData = [this, &DataLocation](uint32_t Data) {
        ARMEmitter::ForwardLabel PastConstant;
        b(&PastConstant);
        Bind(&DataLocation);
        dc32(Data);
        Bind(&PastConstant);
      };

      switch (OpSize) {
        case 1: {
          LOGMAN_THROW_AA_FMT(regOffs <= 31, "unexpected reg index: {}", regOffs);

          const auto Predicate = LoadPredicate();
          dup(ARMEmitter::SubRegSize::i8Bit, VTMP1.Z(), host.Z(), 0);
          mov(ARMEmitter::SubRegSize::i8Bit, guest.Z(), Predicate, VTMP1.Z());

          EmitData(1U << regOffs);
          break;
        }

        case 2: {
          LOGMAN_THROW_AA_FMT((regOffs / 2) <= 15, "unexpected reg index: {}", regOffs / 2);

          const auto Predicate = LoadPredicate();
          dup(ARMEmitter::SubRegSize::i16Bit, VTMP1.Z(), host.Z(), 0);
          mov(ARMEmitter::SubRegSize::i16Bit, guest.Z(), Predicate, VTMP1.Z());

          EmitData(1U << regOffs);
          break;
        }

        case 4: {
          LOGMAN_THROW_AA_FMT((regOffs / 4) <= 7, "unexpected reg index: {}", regOffs / 4);

          const auto Predicate = LoadPredicate();

          dup(ARMEmitter::SubRegSize::i32Bit, VTMP1.Z(), host.Z(), 0);
          mov(ARMEmitter::SubRegSize::i32Bit, guest.Z(), Predicate, VTMP1.Z());

          EmitData(1U << regOffs);
          break;
        }

        case 8: {
          LOGMAN_THROW_AA_FMT((regOffs / 8) <= 3, "unexpected reg index: {}", regOffs / 8);

          const auto Predicate = LoadPredicate();

          dup(ARMEmitter::SubRegSize::i64Bit, VTMP1.Z(), host.Z(), 0);
          mov(ARMEmitter::SubRegSize::i64Bit, guest.Z(), Predicate, VTMP1.Z());

          EmitData(1U << regOffs);
          break;
        }

        case 16: {
          LOGMAN_THROW_AA_FMT(regOffs == 0, "unexpected regOffs: {}", regOffs);
          if (guest.Idx() != host.Idx()) {
            mov(guest.Q(), host.Q());
          }
          break;
        }

        case 32: {
          LOGMAN_THROW_AA_FMT(regOffs == 0, "unexpected regOffs: {}", regOffs);
          if (guest.Idx() != host.Idx()) {
            mov(ARMEmitter::SubRegSize::i64Bit, guest.Z(), PRED_TMP_32B.Merging(), host.Z());
          }
          break;
        }

        default:
          LOGMAN_MSG_A_FMT("Unhandled StoreRegister FPR size: {}", OpSize);
          break;
      }
    } else {
      const auto regOffs = Op->Offset & 15;

      switch (OpSize) {
        case 1:
          ins(ARMEmitter::SubRegSize::i8Bit, guest, regOffs, host, 0);
          break;

        case 2:
          LOGMAN_THROW_AA_FMT((regOffs & 1) == 0, "unexpected regOffs: {}", regOffs);
          ins(ARMEmitter::SubRegSize::i16Bit, guest, regOffs / 2, host, 0);
          break;

        case 4:
          LOGMAN_THROW_AA_FMT((regOffs & 3) == 0, "unexpected regOffs: {}", regOffs);
          // XXX: This had a bug with insert of size 16bit
          ins(ARMEmitter::SubRegSize::i32Bit, guest, regOffs / 4, host, 0);
          break;

        case 8:
          LOGMAN_THROW_AA_FMT((regOffs & 7) == 0, "unexpected regOffs: {}", regOffs);
          // XXX: This had a bug with insert of size 16bit
          ins(ARMEmitter::SubRegSize::i64Bit, guest, regOffs / 8, host, 0);
          break;

        case 16:
          LOGMAN_THROW_AA_FMT(regOffs == 0, "unexpected regOffs: {}", regOffs);
          if (guest.Idx() != host.Idx()) {
            mov(guest.Q(), host.Q());
          }
          break;

        default:
          LOGMAN_MSG_A_FMT("Unhandled StoreRegister FPR size: {}", OpSize);
          break;
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
      LoadConstant(ARMEmitter::Size::i64Bit, TMP1, Op->Stride);
      mul(ARMEmitter::Size::i64Bit, TMP1, Index, TMP1);
      add(ARMEmitter::Size::i64Bit, TMP1, STATE, TMP1.R());

      const auto Dst = GetReg(Node);
      switch (OpSize) {
      case 1:
        ldrb(Dst, TMP1, Op->BaseOffset);
        break;
      case 2:
        ldrh(Dst, TMP1, Op->BaseOffset);
        break;
      case 4:
        ldr(Dst.W(), TMP1, Op->BaseOffset);
        break;
      case 8:
        ldr(Dst.X(), TMP1, Op->BaseOffset);
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
    case 8:
    case 16:
    case 32: {
      LoadConstant(ARMEmitter::Size::i64Bit, TMP1, Op->Stride);
      mul(ARMEmitter::Size::i64Bit, TMP1, Index, TMP1);
      add(ARMEmitter::Size::i64Bit, TMP1, STATE, TMP1.R());

      const auto Dst = GetVReg(Node);

      switch (OpSize) {
      case 1:
        ldrb(Dst, TMP1, Op->BaseOffset);
        break;
      case 2:
        ldrh(Dst, TMP1, Op->BaseOffset);
        break;
      case 4:
        ldr(Dst.S(), TMP1, Op->BaseOffset);
        break;
      case 8:
        ldr(Dst.D(), TMP1, Op->BaseOffset);
        break;
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

  const auto Index = GetReg(Op->Index.ID());

  if (Op->Class == FEXCore::IR::GPRClass) {
    const auto Value = GetReg(Op->Value.ID());

    switch (Op->Stride) {
    case 1:
    case 2:
    case 4:
    case 8: {
      LoadConstant(ARMEmitter::Size::i64Bit, TMP1, Op->Stride);
      mul(ARMEmitter::Size::i64Bit, TMP1, Index, TMP1);
      add(ARMEmitter::Size::i64Bit, TMP1, STATE, TMP1.R());

      switch (OpSize) {
      case 1:
        strb(Value, TMP1, Op->BaseOffset);
        break;
      case 2:
        strh(Value, TMP1, Op->BaseOffset);
        break;
      case 4:
        str(Value.W(), TMP1, Op->BaseOffset);
        break;
      case 8:
        str(Value.X(), TMP1, Op->BaseOffset);
        break;
      default:
        LOGMAN_MSG_A_FMT("Unhandled StoreContextIndexed size: {}", OpSize);
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
    const auto Value = GetVReg(Op->Value.ID());

    switch (Op->Stride) {
    case 1:
    case 2:
    case 4:
    case 8:
    case 16:
    case 32: {
      LoadConstant(ARMEmitter::Size::i64Bit, TMP1, Op->Stride);
      mul(ARMEmitter::Size::i64Bit, TMP1, Index, TMP1);
      add(ARMEmitter::Size::i64Bit, TMP1, STATE, TMP1.R());

      switch (OpSize) {
      case 1:
        strb(Value, TMP1, Op->BaseOffset);
        break;
      case 2:
        strh(Value, TMP1, Op->BaseOffset);
        break;
      case 4:
        str(Value.S(), TMP1, Op->BaseOffset);
        break;
      case 8:
        str(Value.D(), TMP1, Op->BaseOffset);
        break;
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
    const auto Src = GetReg(Op->Value.ID());
    switch (OpSize) {
    case 1: {
      if (SlotOffset > 4095) {
        LoadConstant(ARMEmitter::Size::i64Bit, TMP1, SlotOffset);
        strb(Src, ARMEmitter::Reg::rsp, TMP1.R(), ARMEmitter::ExtendedType::LSL_64, 0);
      }
      else {
        strb(Src, ARMEmitter::Reg::rsp, SlotOffset);
      }
      break;
    }
    case 2: {
      if (SlotOffset > 8190) {
        LoadConstant(ARMEmitter::Size::i64Bit, TMP1, SlotOffset);
        strh(Src, ARMEmitter::Reg::rsp, TMP1.R(), ARMEmitter::ExtendedType::LSL_64, 0);
      }
      else {
        strh(Src, ARMEmitter::Reg::rsp, SlotOffset);
      }
      break;
    }
    case 4: {
      if (SlotOffset > 16380) {
        LoadConstant(ARMEmitter::Size::i64Bit, TMP1, SlotOffset);
        str(Src.W(), ARMEmitter::Reg::rsp, TMP1.R(), ARMEmitter::ExtendedType::LSL_64, 0);
      }
      else {
        str(Src.W(), ARMEmitter::Reg::rsp, SlotOffset);
      }
      break;
    }
    case 8: {
      if (SlotOffset > 32760) {
        LoadConstant(ARMEmitter::Size::i64Bit, TMP1, SlotOffset);
        str(Src.X(), ARMEmitter::Reg::rsp, TMP1.R(), ARMEmitter::ExtendedType::LSL_64, 0);
      }
      else {
        str(Src.X(), ARMEmitter::Reg::rsp, SlotOffset);
      }
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unhandled SpillRegister size: {}", OpSize);
      break;
    }
  } else if (Op->Class == FEXCore::IR::FPRClass) {
    const auto Src = GetVReg(Op->Value.ID());

    switch (OpSize) {
    case 4: {
      if (SlotOffset > 16380) {
        LoadConstant(ARMEmitter::Size::i64Bit, TMP1, SlotOffset);
        str(Src.S(), ARMEmitter::Reg::rsp, TMP1.R(), ARMEmitter::ExtendedType::LSL_64, 0);
      }
      else {
        str(Src.S(), ARMEmitter::Reg::rsp, SlotOffset);
      }
      break;
    }
    case 8: {
      if (SlotOffset > 32760) {
        LoadConstant(ARMEmitter::Size::i64Bit, TMP1, SlotOffset);
        str(Src.D(), ARMEmitter::Reg::rsp, TMP1.R(), ARMEmitter::ExtendedType::LSL_64, 0);
      }
      else {
        str(Src.D(), ARMEmitter::Reg::rsp, SlotOffset);
      }
      break;
    }
    case 16: {
      if (SlotOffset > 65520) {
        LoadConstant(ARMEmitter::Size::i64Bit, TMP1, SlotOffset);
        str(Src.Q(), ARMEmitter::Reg::rsp, TMP1.R(), ARMEmitter::ExtendedType::LSL_64, 0);
      }
      else {
        str(Src.Q(), ARMEmitter::Reg::rsp, SlotOffset);
      }
      break;
    }
    case 32: {
      mov(TMP3, SlotOffset);
      st1b<ARMEmitter::SubRegSize::i8Bit>(Src.Z(), PRED_TMP_32B, ARMEmitter::Reg::rsp, TMP3);
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
    const auto Dst = GetReg(Node);
    switch (OpSize) {
    case 1: {
      if (SlotOffset > 4095) {
        LoadConstant(ARMEmitter::Size::i64Bit, TMP1, SlotOffset);
        ldrb(Dst, ARMEmitter::Reg::rsp, TMP1.R(), ARMEmitter::ExtendedType::LSL_64, 0);
      }
      else {
        ldrb(Dst, ARMEmitter::Reg::rsp, SlotOffset);
      }
      break;
    }
    case 2: {
      if (SlotOffset > 8190) {
        LoadConstant(ARMEmitter::Size::i64Bit, TMP1, SlotOffset);
        ldrh(Dst, ARMEmitter::Reg::rsp, TMP1.R(), ARMEmitter::ExtendedType::LSL_64, 0);
      }
      else {
        ldrh(Dst, ARMEmitter::Reg::rsp, SlotOffset);
      }
      break;
    }
    case 4: {
      if (SlotOffset > 16380) {
        LoadConstant(ARMEmitter::Size::i64Bit, TMP1, SlotOffset);
        ldr(Dst.W(), ARMEmitter::Reg::rsp, TMP1.R(), ARMEmitter::ExtendedType::LSL_64, 0);
      }
      else {
        ldr(Dst.W(), ARMEmitter::Reg::rsp, SlotOffset);
      }
      break;
    }
    case 8: {
      if (SlotOffset > 32760) {
        LoadConstant(ARMEmitter::Size::i64Bit, TMP1, SlotOffset);
        ldr(Dst.X(), ARMEmitter::Reg::rsp, TMP1.R(), ARMEmitter::ExtendedType::LSL_64, 0);
      }
      else {
        ldr(Dst.X(), ARMEmitter::Reg::rsp, SlotOffset);
      }
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unhandled FillRegister size: {}", OpSize);
      break;
    }
  } else if (Op->Class == FEXCore::IR::FPRClass) {
    const auto Dst = GetVReg(Node);

    switch (OpSize) {
    case 4: {
      if (SlotOffset > 16380) {
        LoadConstant(ARMEmitter::Size::i64Bit, TMP1, SlotOffset);
        ldr(Dst.S(), ARMEmitter::Reg::rsp, TMP1.R(), ARMEmitter::ExtendedType::LSL_64, 0);
      }
      else {
        ldr(Dst.S(), ARMEmitter::Reg::rsp, SlotOffset);
      }
      break;
    }
    case 8: {
      if (SlotOffset > 32760) {
        LoadConstant(ARMEmitter::Size::i64Bit, TMP1, SlotOffset);
        ldr(Dst.D(), ARMEmitter::Reg::rsp, TMP1.R(), ARMEmitter::ExtendedType::LSL_64, 0);
      }
      else {
        ldr(Dst.D(), ARMEmitter::Reg::rsp, SlotOffset);
      }
      break;
    }
    case 16: {
      if (SlotOffset > 65520) {
        LoadConstant(ARMEmitter::Size::i64Bit, TMP1, SlotOffset);
        ldr(Dst.Q(), ARMEmitter::Reg::rsp, TMP1.R(), ARMEmitter::ExtendedType::LSL_64, 0);
      }
      else {
        ldr(Dst.Q(), ARMEmitter::Reg::rsp, SlotOffset);
      }
      break;
    }
    case 32: {
      mov(TMP3, SlotOffset);
      ld1b<ARMEmitter::SubRegSize::i8Bit>(Dst.Z(), PRED_TMP_32B.Zeroing(), ARMEmitter::Reg::rsp, TMP3);
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
  auto Dst = GetReg(Node);
  ldrb(Dst, STATE, offsetof(FEXCore::Core::CPUState, flags[0]) + Op->Flag);
}

DEF_OP(StoreFlag) {
  auto Op = IROp->C<IR::IROp_StoreFlag>();
  strb(GetReg(Op->Value.ID()), STATE, offsetof(FEXCore::Core::CPUState, flags[0]) + Op->Flag);
}

FEXCore::ARMEmitter::ExtendedMemOperand Arm64JITCore::GenerateMemOperand(uint8_t AccessSize,
                                            FEXCore::ARMEmitter::Register Base,
                                            IR::OrderedNodeWrapper Offset,
                                            IR::MemOffsetType OffsetType,
                                            uint8_t OffsetScale) {
  if (Offset.IsInvalid()) {
    return FEXCore::ARMEmitter::ExtendedMemOperand(Base, ARMEmitter::IndexType::OFFSET, 0);
  } else {
    if (OffsetScale != 1 && OffsetScale != AccessSize) {
      LOGMAN_MSG_A_FMT("Unhandled GenerateMemOperand OffsetScale: {}", OffsetScale);
    }
    uint64_t Const;
    if (IsInlineConstant(Offset, &Const)) {
      return FEXCore::ARMEmitter::ExtendedMemOperand(Base, ARMEmitter::IndexType::OFFSET, Const);
    } else {
      auto RegOffset = GetReg(Offset.ID());
      switch(OffsetType.Val) {
        case IR::MEM_OFFSET_SXTX.Val: return FEXCore::ARMEmitter::ExtendedMemOperand(Base, RegOffset, FEXCore::ARMEmitter::ExtendedType::SXTX, (int)std::log2(OffsetScale) );
        case IR::MEM_OFFSET_UXTW.Val: return FEXCore::ARMEmitter::ExtendedMemOperand(Base, RegOffset, FEXCore::ARMEmitter::ExtendedType::UXTW, (int)std::log2(OffsetScale) );
        case IR::MEM_OFFSET_SXTW.Val: return FEXCore::ARMEmitter::ExtendedMemOperand(Base, RegOffset, FEXCore::ARMEmitter::ExtendedType::SXTW, (int)std::log2(OffsetScale) );
        default: LOGMAN_MSG_A_FMT("Unhandled GenerateMemOperand OffsetType: {}", OffsetType.Val); break;
      }
    }
  }

  FEX_UNREACHABLE;
}

static auto ConvertExtendedType(IR::MemOffsetType OffsetType) -> ARMEmitter::ExtendedType {
  switch (OffsetType.Val) {
    case IR::MEM_OFFSET_SXTX.Val: return ARMEmitter::ExtendedType::SXTX;
    case IR::MEM_OFFSET_UXTW.Val: return ARMEmitter::ExtendedType::UXTW;
    case IR::MEM_OFFSET_SXTW.Val: return ARMEmitter::ExtendedType::SXTW;

    default: LOGMAN_MSG_A_FMT("Unhandled GenerateMemOperand OffsetType: {}", OffsetType.Val); break;
  }
  FEX_UNREACHABLE;
}

FEXCore::ARMEmitter::SVEMemOperand Arm64JITCore::GenerateSVEMemOperand(uint8_t AccessSize,
                                                  FEXCore::ARMEmitter::Register Base,
                                                  IR::OrderedNodeWrapper Offset,
                                                  IR::MemOffsetType OffsetType,
                                                  [[maybe_unused]] uint8_t OffsetScale) {
  if (Offset.IsInvalid()) {
    return FEXCore::ARMEmitter::SVEMemOperand(Base.X(), 0);
  }

  uint64_t Const{};
  if (IsInlineConstant(Offset, &Const)) {
    if (Const == 0) {
      return FEXCore::ARMEmitter::SVEMemOperand(Base.X(), 0);
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
      return FEXCore::ARMEmitter::SVEMemOperand(Base.X(), static_cast<uint64_t>(Index));
    }

    // If we can't do that for whatever reason, then unfortunately, we need
    // to move it over to a temporary to use as an offset.
    mov(TMP1, Const);
    return FEXCore::ARMEmitter::SVEMemOperand(Base.X(), TMP1);
  }

  // Otherwise handle it like normal.
  // Note that we do nothing with the offset type and offset scale,
  // since SVE loads and stores don't have the ability to perform an
  // optional extension or shift as part of their behavior.
  LOGMAN_THROW_A_FMT(OffsetType.Val == IR::MEM_OFFSET_SXTX.Val,
                     "Currently only the default offset type (SXTX) is supported.");

  const auto RegOffset = GetReg(Offset.ID());
  return FEXCore::ARMEmitter::SVEMemOperand(Base.X(), RegOffset.X());
}

DEF_OP(LoadMem) {
  const auto Op = IROp->C<IR::IROp_LoadMem>();
  const auto OpSize = IROp->Size;

  const auto MemReg = GetReg(Op->Addr.ID());
  const auto MemSrc = GenerateMemOperand(OpSize, MemReg, Op->Offset, Op->OffsetType, Op->OffsetScale);

  if (Op->Class == FEXCore::IR::GPRClass) {
    const auto Dst = GetReg(Node);

    switch (OpSize) {
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
        ldr(Dst.X(), MemSrc);
        break;
      default:
        LOGMAN_MSG_A_FMT("Unhandled LoadMem size: {}", OpSize);
        break;
    }
  }
  else {
    const auto Dst = GetVReg(Node);

    switch (OpSize) {
      case 1:
        ldrb(Dst, MemSrc);
        break;
      case 2:
        ldrh(Dst, MemSrc);
        break;
      case 4:
        ldr(Dst.S(), MemSrc);
        break;
      case 8:
        ldr(Dst.D(), MemSrc);
        break;
      case 16:
        ldr(Dst.Q(), MemSrc);
        break;
      case 32: {
        const auto Operand = GenerateSVEMemOperand(OpSize, MemReg, Op->Offset, Op->OffsetType, Op->OffsetScale);
        ld1b<ARMEmitter::SubRegSize::i8Bit>(Dst.Z(), PRED_TMP_32B.Zeroing(), Operand);
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unhandled LoadMem size: {}", OpSize);
        break;
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
    }
    else {
      // Aligned
      nop();
      switch (OpSize) {
        case 2:
          ldapurh(Dst, MemReg, Offset);
          break;
        case 4:
          ldapur(Dst.W(), MemReg, Offset);
          break;
        case 8:
          ldapur(Dst.X(), MemReg, Offset);
          break;
        default:
          LOGMAN_MSG_A_FMT("Unhandled LoadMemTSO size: {}", OpSize);
          break;
      }
      nop();
    }
  }
  else if (CTX->HostFeatures.SupportsRCPC && Op->Class == FEXCore::IR::GPRClass) {

    const auto Dst = GetReg(Node);
    if (OpSize == 1) {
      // 8bit load is always aligned to natural alignment
      ldaprb(Dst, MemReg);
    }
    else {
      // Aligned
      nop();
      switch (OpSize) {
        case 2:
          ldaprh(Dst, MemReg);
          break;
        case 4:
          ldapr(Dst.W(), MemReg);
          break;
        case 8:
          ldapr(Dst.X(), MemReg);
          break;
        default:
          LOGMAN_MSG_A_FMT("Unhandled LoadMemTSO size: {}", OpSize);
          break;
      }
      nop();
    }
  }
  else if (Op->Class == FEXCore::IR::GPRClass) {
    const auto Dst = GetReg(Node);
    if (OpSize == 1) {
      // 8bit load is always aligned to natural alignment
      ldarb(Dst, MemReg);
    }
    else {
      // Aligned
      nop();
      switch (OpSize) {
        case 2:
          ldarh(Dst, MemReg);
          break;
        case 4:
          ldar(Dst.W(), MemReg);
          break;
        case 8:
          ldar(Dst.X(), MemReg);
          break;
        default:
          LOGMAN_MSG_A_FMT("Unhandled LoadMemTSO size: {}", OpSize);
          break;
      }
      nop();
    }
  }
  else {
    dmb(FEXCore::ARMEmitter::BarrierScope::ISH);
    const auto Dst = GetVReg(Node);
    const auto MemSrc = GenerateMemOperand(OpSize, MemReg, Op->Offset, Op->OffsetType, Op->OffsetScale);
    switch (OpSize) {
      case 1:
        ldrb(Dst, MemSrc);
        break;
      case 2:
        ldrh(Dst, MemSrc);
        break;
      case 4:
        ldr(Dst.S(), MemSrc);
        break;
      case 8:
        ldr(Dst.D(), MemSrc);
        break;
      case 16:
        ldr(Dst.Q(), MemSrc);
        break;
      case 32: {
        const auto MemSrc = GenerateSVEMemOperand(OpSize, MemReg, Op->Offset, Op->OffsetType, Op->OffsetScale);
        ld1b<ARMEmitter::SubRegSize::i8Bit>(Dst.Z(), PRED_TMP_32B.Zeroing(), MemSrc);
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unhandled LoadMemTSO size: {}", OpSize);
        break;
    }
    dmb(FEXCore::ARMEmitter::BarrierScope::ISH);
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
      case 1:
        strb(Src, MemSrc);
        break;
      case 2:
        strh(Src, MemSrc);
        break;
      case 4:
        str(Src.W(), MemSrc);
        break;
      case 8:
        str(Src.X(), MemSrc);
        break;
      default:
        LOGMAN_MSG_A_FMT("Unhandled StoreMem size: {}", OpSize);
        break;
    }
  }
  else {
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
      default:
        LOGMAN_MSG_A_FMT("Unhandled StoreMem size: {}", OpSize);
        break;
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
    }
    else {
      nop();
      switch (OpSize) {
        case 2:
          stlurh(Src, MemReg, Offset);
          break;
        case 4:
          stlur(Src.W(), MemReg, Offset);
          break;
        case 8:
          stlur(Src.X(), MemReg, Offset);
          break;
        default:
          LOGMAN_MSG_A_FMT("Unhandled StoreMemTSO size: {}", OpSize);
          break;
      }
      nop();
    }
  }
  else if (Op->Class == FEXCore::IR::GPRClass) {
    const auto Src = GetReg(Op->Value.ID());

    if (OpSize == 1) {
      // 8bit load is always aligned to natural alignment
      stlrb(Src, MemReg);
    }
    else {
      nop();
      switch (OpSize) {
        case 2:
          stlrh(Src, MemReg);
          break;
        case 4:
          stlr(Src.W(), MemReg);
          break;
        case 8:
          stlr(Src.X(), MemReg);
          break;
        default:
          LOGMAN_MSG_A_FMT("Unhandled StoreMemTSO size: {}", OpSize);
          break;
      }
      nop();
    }
  }
  else {
    dmb(FEXCore::ARMEmitter::BarrierScope::ISH);
    const auto Src = GetVReg(Op->Value.ID());
    const auto MemSrc = GenerateMemOperand(OpSize, MemReg, Op->Offset, Op->OffsetType, Op->OffsetScale);
    switch (OpSize) {
      case 1:
        strb(Src, MemSrc);
        break;
      case 2:
        strh(Src, MemSrc);
        break;
      case 4:
        str(Src.S(), MemSrc);
        break;
      case 8:
        str(Src.D(), MemSrc);
        break;
      case 16:
        str(Src.Q(), MemSrc);
        break;
      case 32: {
        const auto Operand = GenerateSVEMemOperand(OpSize, MemReg, Op->Offset, Op->OffsetType, Op->OffsetScale);
        st1b<ARMEmitter::SubRegSize::i8Bit>(Src.Z(), PRED_TMP_32B, Operand);
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unhandled StoreMemTSO size: {}", OpSize);
        break;
    }
    dmb(FEXCore::ARMEmitter::BarrierScope::ISH);
  }
}

DEF_OP(ParanoidLoadMemTSO) {
  const auto Op = IROp->C<IR::IROp_LoadMemTSO>();
  const auto OpSize = IROp->Size;

  const auto Addr = GetReg(Op->Addr.ID());

  if (!Op->Offset.IsInvalid()) {
    LOGMAN_MSG_A_FMT("ParanoidLoadMemTSO: No offset allowed");
  }

  if (Op->Class == FEXCore::IR::GPRClass) {
    const auto Dst = GetReg(Node);
    switch (OpSize) {
      case 1:
        ldarb(Dst, Addr);
        break;
      case 2:
        ldarh(Dst, Addr);
        break;
      case 4:
        ldar(Dst.W(), Addr);
        break;
      case 8:
        ldar(Dst.X(), Addr);
        break;
      default:
        LOGMAN_MSG_A_FMT("Unhandled ParanoidLoadMemTSO size: {}", OpSize);
        break;
    }
  }
  else {
    const auto Dst = GetVReg(Node);
    switch (OpSize) {
      case 1:
        ldarb(TMP1, Addr);
        ins(ARMEmitter::SubRegSize::i8Bit, Dst, 0, TMP1);
        break;
      case 2:
        ldarh(TMP1, Addr);
        ins(ARMEmitter::SubRegSize::i16Bit, Dst, 0, TMP1);
        break;
      case 4:
        ldar(TMP1.W(), Addr);
        ins(ARMEmitter::SubRegSize::i32Bit, Dst, 0, TMP1);
        break;
      case 8:
        ldar(TMP1, Addr);
        ins(ARMEmitter::SubRegSize::i64Bit, Dst, 0, TMP1);
        break;
      case 16:
        nop();
        ldaxp(ARMEmitter::Size::i64Bit, TMP1, TMP2, Addr);
        clrex();
        ins(ARMEmitter::SubRegSize::i64Bit, Dst, 0, TMP1);
        ins(ARMEmitter::SubRegSize::i64Bit, Dst, 1, TMP2);
        break;
      case 32:
        dmb(FEXCore::ARMEmitter::BarrierScope::ISH);
        ld1b<ARMEmitter::SubRegSize::i8Bit>(Dst.Z(), PRED_TMP_32B.Zeroing(), Addr);
        dmb(FEXCore::ARMEmitter::BarrierScope::ISH);
        break;
      default:
        LOGMAN_MSG_A_FMT("Unhandled ParanoidLoadMemTSO size: {}", OpSize);
        break;
    }
  }
}

DEF_OP(ParanoidStoreMemTSO) {
  const auto Op = IROp->C<IR::IROp_StoreMemTSO>();
  const auto OpSize = IROp->Size;

  const auto Addr = GetReg(Op->Addr.ID());

  if (!Op->Offset.IsInvalid()) {
    LOGMAN_MSG_A_FMT("ParanoidStoreMemTSO: No offset allowed");
  }

  if (Op->Class == FEXCore::IR::GPRClass) {
    const auto Src = GetReg(Op->Value.ID());
    switch (OpSize) {
      case 1:
        stlrb(Src, Addr);
        break;
      case 2:
        stlrh(Src, Addr);
        break;
      case 4:
        stlr(Src.W(), Addr);
        break;
      case 8:
        stlr(Src.X(), Addr);
        break;
      default:
        LOGMAN_MSG_A_FMT("Unhandled ParanoidStoreMemTSO size: {}", OpSize);
        break;
    }
  }
  else {
    const auto Src = GetVReg(Op->Value.ID());

    switch (OpSize) {
      case 1:
        umov<ARMEmitter::SubRegSize::i8Bit>(TMP1, Src, 0);
        stlrb(TMP1, Addr);
        break;
      case 2:
        umov<ARMEmitter::SubRegSize::i16Bit>(TMP1, Src, 0);
        stlrh(TMP1, Addr);
        break;
      case 4:
        umov<ARMEmitter::SubRegSize::i32Bit>(TMP1, Src, 0);
        stlr(TMP1.W(), Addr);
        break;
      case 8:
        umov<ARMEmitter::SubRegSize::i64Bit>(TMP1, Src, 0);
        stlr(TMP1, Addr);
        break;
      case 16: {
        // Move vector to GPRs
        umov<ARMEmitter::SubRegSize::i64Bit>(TMP1, Src, 0);
        umov<ARMEmitter::SubRegSize::i64Bit>(TMP2, Src, 1);
        ARMEmitter::BackwardLabel B;
        Bind(&B);

        // ldaxp must not have both the destination registers be the same
        ldaxp(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::zr, TMP3, Addr); // <- Can hit SIGBUS. Overwritten with DMB
        stlxp(ARMEmitter::Size::i64Bit, TMP3, TMP1, TMP2, Addr); // <- Can also hit SIGBUS
        cbnz(ARMEmitter::Size::i64Bit, TMP3, &B); // < Overwritten with DMB
        break;
      }
      case 32: {
        dmb(FEXCore::ARMEmitter::BarrierScope::ISH);
        st1b<ARMEmitter::SubRegSize::i8Bit>(Src, PRED_TMP_32B, Addr, 0);
        dmb(FEXCore::ARMEmitter::BarrierScope::ISH);
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unhandled ParanoidStoreMemTSO size: {}", OpSize);
        break;
    }
  }
}

DEF_OP(CacheLineClear) {
  auto Op = IROp->C<IR::IROp_CacheLineClear>();

  auto MemReg = GetReg(Op->Addr.ID());

  // Clear dcache only
  // icache doesn't matter here since the guest application shouldn't be calling clflush on JIT code.
  mov(TMP1, MemReg.X());
  for (size_t i = 0; i < std::max(1U, CTX->HostFeatures.DCacheLineSize / 64U); ++i) {
    dc(ARMEmitter::DataCacheOperation::CVAU, TMP1);
    add(ARMEmitter::Size::i64Bit, TMP1, TMP1, CTX->HostFeatures.DCacheLineSize);
  }
  dsb(FEXCore::ARMEmitter::BarrierScope::ISH);
}

DEF_OP(CacheLineZero) {
  auto Op = IROp->C<IR::IROp_CacheLineZero>();

  auto MemReg = GetReg(Op->Addr.ID());

  if (CTX->HostFeatures.SupportsCLZERO) {
    // We can use this instruction directly
    dc(ARMEmitter::DataCacheOperation::ZVA, MemReg);
  }
  else {
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
  REGISTER_OP(CACHELINECLEAR,      CacheLineClear);
  REGISTER_OP(CACHELINEZERO,       CacheLineZero);
#undef REGISTER_OP
}
}

