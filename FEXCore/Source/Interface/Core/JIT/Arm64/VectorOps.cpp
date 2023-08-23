/*
$info$
tags: backend|arm64
$end_info$
*/

#include "Interface/Core/ArchHelpers/CodeEmitter/Emitter.h"
#include "Interface/Core/ArchHelpers/CodeEmitter/Registers.h"
#include "Interface/Core/Dispatcher/Arm64Dispatcher.h"
#include "Interface/Core/JIT/Arm64/JITClass.h"

#include <FEXCore/Utils/MathUtils.h>

namespace FEXCore::CPU {
#define DEF_OP(x) void Arm64JITCore::Op_##x(IR::IROp_Header const *IROp, IR::NodeID Node)
DEF_OP(VectorZero) {
  const auto OpSize = IROp->Size;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);

  if (HostSupportsSVE256 && Is256Bit) {
    mov_imm(ARMEmitter::SubRegSize::i64Bit, Dst.Z(), 0);
  } else {
    switch (OpSize) {
      case 8: {
        movi(ARMEmitter::SubRegSize::i64Bit, Dst.D(), 0);
        break;
      }
      case 16: {
        movi(ARMEmitter::SubRegSize::i64Bit, Dst.Q(), 0);
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Op Size: {}", OpSize);
        break;
    }
  }
}

DEF_OP(VectorImm) {
  const auto Op = IROp->C<IR::IROp_VectorImm>();
  const auto OpSize = IROp->Size;

  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;
  const auto ElementSize = Op->Header.ElementSize;

  LOGMAN_THROW_AA_FMT(ElementSize == 1 || ElementSize == 2 || ElementSize == 4 || ElementSize == 8, "Invalid size");
  const auto SubRegSize =
    ElementSize == 1 ? ARMEmitter::SubRegSize::i8Bit :
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i8Bit;

  const auto Dst = GetVReg(Node);

  if (HostSupportsSVE256 && Is256Bit) {
    if (ElementSize > 1 && (Op->Immediate & 0x80)) {
      // SVE dup uses sign extension where VectorImm wants zext
      LoadConstant(ARMEmitter::Size::i64Bit, TMP1, Op->Immediate);
      dup(SubRegSize, Dst.Z(), TMP1);
    }
    else {
      dup_imm(SubRegSize, Dst.Z(), static_cast<int8_t>(Op->Immediate));
    }
  } else {
    if (ElementSize == 8) {
      // movi with 64bit element size doesn't do what we want here
      LoadConstant(ARMEmitter::Size::i64Bit, TMP1, Op->Immediate);
      dup(SubRegSize, Dst.Q(), TMP1.R());
    }
    else {
      movi(SubRegSize, Dst.Q(), Op->Immediate);
    }
  }
}

DEF_OP(LoadNamedVectorConstant) {
  const auto Op = IROp->C<IR::IROp_LoadNamedVectorConstant>();
  const auto OpSize = IROp->Size;

  const auto Dst = GetVReg(Node);

  // Load the pointer.
  ldr(TMP1, STATE_PTR(CpuStateFrame, Pointers.Common.NamedVectorConstantPointers[Op->Constant]));

  switch (OpSize) {
    case 1:
      ldrb(Dst, TMP1, 0);
      break;
    case 2:
      ldrh(Dst, TMP1, 0);
      break;
    case 4:
      ldr(Dst.S(), TMP1, 0);
      break;
    case 8:
      ldr(Dst.D(), TMP1, 0);
      break;
    case 16:
      ldr(Dst.Q(), TMP1, 0);
      break;
    case 32: {
      ld1b<ARMEmitter::SubRegSize::i8Bit>(Dst.Z(), PRED_TMP_32B.Zeroing(), TMP1, 0);
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unhandled {} size: {}", __func__, OpSize);
      break;
  }
}
DEF_OP(VMov) {
  const auto Op = IROp->C<IR::IROp_VMov>();
  const auto OpSize = IROp->Size;

  const auto Dst = GetVReg(Node);
  const auto Source = GetVReg(Op->Source.ID());

  switch (OpSize) {
    case 1: {
      movi(ARMEmitter::SubRegSize::i64Bit, VTMP1.Q(), 0);
      ins(ARMEmitter::SubRegSize::i8Bit, VTMP1, 0, Source, 0);
      mov(Dst.Q(), VTMP1.Q());
      break;
    }
    case 2: {
      movi(ARMEmitter::SubRegSize::i64Bit, VTMP1.Q(), 0);
      ins(ARMEmitter::SubRegSize::i16Bit, VTMP1, 0, Source, 0);
      mov(Dst.Q(), VTMP1.Q());
      break;
    }
    case 4: {
      movi(ARMEmitter::SubRegSize::i64Bit, VTMP1.Q(), 0);
      ins(ARMEmitter::SubRegSize::i32Bit, VTMP1, 0, Source, 0);
      mov(Dst.Q(), VTMP1.Q());
      break;
    }
    case 8: {
      mov(Dst.D(), Source.D());
      break;
    }
    case 16: {
      if (HostSupportsSVE256 || Dst.Idx() != Source.Idx()) {
        mov(Dst.Q(), Source.Q());
      }
      break;
    }
    case 32: {
      // NOTE: If, in the distant future we support larger moves, or registers
      //       (*cough* AVX-512 *cough*) make sure to change this to treat
      //       256-bit moves with zero extending behavior instead of doing only
      //       a regular SVE move into a 512-bit register.
      if (Dst.Idx() != Source.Idx()) {
        mov(Dst.Z(), Source.Z());
      }
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unknown Op Size: {}", OpSize);
      break;
  }
}

DEF_OP(VAnd) {
  const auto Op = IROp->C<IR::IROp_VAnd>();
  const auto OpSize = IROp->Size;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1.ID());
  const auto Vector2 = GetVReg(Op->Vector2.ID());

  if (HostSupportsSVE256 && Is256Bit) {
    and_(Dst.Z(), Vector1.Z(), Vector2.Z());
  } else {
    and_(Dst.Q(), Vector1.Q(), Vector2.Q());
  }
}

DEF_OP(VBic) {
  const auto Op = IROp->C<IR::IROp_VBic>();
  const auto OpSize = IROp->Size;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1.ID());
  const auto Vector2 = GetVReg(Op->Vector2.ID());

  if (HostSupportsSVE256 && Is256Bit) {
    bic(Dst.Z(), Vector1.Z(), Vector2.Z());
  } else {
    bic(Dst.Q(), Vector1.Q(), Vector2.Q());
  }
}

DEF_OP(VOr) {
  const auto Op = IROp->C<IR::IROp_VOr>();
  const auto OpSize = IROp->Size;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1.ID());
  const auto Vector2 = GetVReg(Op->Vector2.ID());

  if (HostSupportsSVE256 && Is256Bit) {
    orr(Dst.Z(), Vector1.Z(), Vector2.Z());
  } else {
    orr(Dst.Q(), Vector1.Q(), Vector2.Q());
  }
}

DEF_OP(VXor) {
  const auto Op = IROp->C<IR::IROp_VXor>();
  const auto OpSize = IROp->Size;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1.ID());
  const auto Vector2 = GetVReg(Op->Vector2.ID());

  if (HostSupportsSVE256 && Is256Bit) {
    eor(Dst.Z(), Vector1.Z(), Vector2.Z());
  } else {
    eor(Dst.Q(), Vector1.Q(), Vector2.Q());
  }
}

DEF_OP(VAdd) {
  const auto Op = IROp->C<IR::IROp_VAdd>();
  const auto OpSize = IROp->Size;

  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;
  const auto ElementSize = Op->Header.ElementSize;

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1.ID());
  const auto Vector2 = GetVReg(Op->Vector2.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 1 || ElementSize == 2 || ElementSize == 4 || ElementSize == 8, "Invalid size");
  const auto SubRegSize =
    ElementSize == 1 ? ARMEmitter::SubRegSize::i8Bit :
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i8Bit;

  if (HostSupportsSVE256 && Is256Bit) {
    add(SubRegSize, Dst.Z(), Vector1.Z(), Vector2.Z());
  } else {
    add(SubRegSize, Dst.Q(), Vector1.Q(), Vector2.Q());
  }
}

DEF_OP(VSub) {
  const auto Op = IROp->C<IR::IROp_VSub>();
  const auto OpSize = IROp->Size;

  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;
  const auto ElementSize = Op->Header.ElementSize;

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1.ID());
  const auto Vector2 = GetVReg(Op->Vector2.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 1 || ElementSize == 2 || ElementSize == 4 || ElementSize == 8, "Invalid size");
  const auto SubRegSize =
    ElementSize == 1 ? ARMEmitter::SubRegSize::i8Bit :
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i8Bit;

  if (HostSupportsSVE256 && Is256Bit) {
    sub(SubRegSize, Dst.Z(), Vector1.Z(), Vector2.Z());
  } else {
    sub(SubRegSize, Dst.Q(), Vector1.Q(), Vector2.Q());
  }
}

DEF_OP(VUQAdd) {
  const auto Op = IROp->C<IR::IROp_VUQAdd>();
  const auto OpSize = IROp->Size;

  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;
  const auto ElementSize = Op->Header.ElementSize;

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1.ID());
  const auto Vector2 = GetVReg(Op->Vector2.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 1 || ElementSize == 2 || ElementSize == 4 || ElementSize == 8, "Invalid size");
  const auto SubRegSize =
    ElementSize == 1 ? ARMEmitter::SubRegSize::i8Bit :
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i8Bit;

  if (HostSupportsSVE256 && Is256Bit) {
    uqadd(SubRegSize, Dst.Z(), Vector1.Z(), Vector2.Z());
  } else {
    uqadd(SubRegSize, Dst.Q(), Vector1.Q(), Vector2.Q());
  }
}

DEF_OP(VUQSub) {
  const auto Op = IROp->C<IR::IROp_VUQSub>();
  const auto OpSize = IROp->Size;

  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;
  const auto ElementSize = Op->Header.ElementSize;

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1.ID());
  const auto Vector2 = GetVReg(Op->Vector2.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 1 || ElementSize == 2 || ElementSize == 4 || ElementSize == 8, "Invalid size");
  const auto SubRegSize =
    ElementSize == 1 ? ARMEmitter::SubRegSize::i8Bit :
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i8Bit;

  if (HostSupportsSVE256 && Is256Bit) {
    uqsub(SubRegSize, Dst.Z(), Vector1.Z(), Vector2.Z());
  } else {
    uqsub(SubRegSize, Dst.Q(), Vector1.Q(), Vector2.Q());
  }
}

DEF_OP(VSQAdd) {
  const auto Op = IROp->C<IR::IROp_VSQAdd>();
  const auto OpSize = IROp->Size;

  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;
  const auto ElementSize = Op->Header.ElementSize;

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1.ID());
  const auto Vector2 = GetVReg(Op->Vector2.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 1 || ElementSize == 2 || ElementSize == 4 || ElementSize == 8, "Invalid size");
  const auto SubRegSize =
    ElementSize == 1 ? ARMEmitter::SubRegSize::i8Bit :
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i8Bit;

  if (HostSupportsSVE256 && Is256Bit) {
    sqadd(SubRegSize, Dst.Z(), Vector1.Z(), Vector2.Z());
  } else {
    sqadd(SubRegSize, Dst.Q(), Vector1.Q(), Vector2.Q());
  }
}

DEF_OP(VSQSub) {
  const auto Op = IROp->C<IR::IROp_VSQSub>();
  const auto OpSize = IROp->Size;

  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;
  const auto ElementSize = Op->Header.ElementSize;

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1.ID());
  const auto Vector2 = GetVReg(Op->Vector2.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 1 || ElementSize == 2 || ElementSize == 4 || ElementSize == 8, "Invalid size");
  const auto SubRegSize =
    ElementSize == 1 ? ARMEmitter::SubRegSize::i8Bit :
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i8Bit;

  if (HostSupportsSVE256 && Is256Bit) {
    sqsub(SubRegSize, Dst.Z(), Vector1.Z(), Vector2.Z());
  } else {
    sqsub(SubRegSize, Dst.Q(), Vector1.Q(), Vector2.Q());
  }
}

DEF_OP(VAddP) {
  const auto Op = IROp->C<IR::IROp_VAddP>();
  const auto OpSize = IROp->Size;
  const auto IsScalar = OpSize == 8;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto ElementSize = Op->Header.ElementSize;

  const auto Dst = GetVReg(Node);
  const auto VectorLower = GetVReg(Op->VectorLower.ID());
  const auto VectorUpper = GetVReg(Op->VectorUpper.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 1 || ElementSize == 2 || ElementSize == 4 || ElementSize == 8, "Invalid size");
  const auto SubRegSize =
    ElementSize == 1 ? ARMEmitter::SubRegSize::i8Bit :
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i8Bit;

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Pred = PRED_TMP_32B.Merging();

    // SVE ADDP is a destructive operation, so we need a temporary
    movprfx(VTMP1.Z(), VectorLower.Z());

    // Unlike Adv. SIMD's version of ADDP, which acts like it concats the
    // upper vector onto the end of the lower vector and then performs
    // pairwise addition, the SVE version actually interleaves the
    // results of the pairwise addition (gross!), so we need to undo that.
    addp(SubRegSize, VTMP1.Z(), Pred, VTMP1.Z(), VectorUpper.Z());
    uzp1(SubRegSize, Dst.Z(), VTMP1.Z(), VTMP1.Z());
    uzp2(SubRegSize, VTMP2.Z(), VTMP1.Z(), VTMP1.Z());

    // Merge upper half with lower half.
    splice<ARMEmitter::OpType::Destructive>(ARMEmitter::SubRegSize::i64Bit, Dst.Z(), PRED_TMP_16B, Dst.Z(), VTMP2.Z());
  } else {
    if (IsScalar) {
      addp(SubRegSize, Dst.D(), VectorLower.D(), VectorUpper.D());
    } else {
      addp(SubRegSize, Dst.Q(), VectorLower.Q(), VectorUpper.Q());
    }
  }
}

DEF_OP(VAddV) {
  const auto Op = IROp->C<IR::IROp_VAddV>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto Vector = GetVReg(Op->Vector.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 1 || ElementSize == 2 || ElementSize == 4 || ElementSize == 8, "Invalid size");
  const auto SubRegSize = ARMEmitter::ToVectorSizePair(
    ElementSize == 1 ? ARMEmitter::SubRegSize::i8Bit :
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i8Bit);

  if (HostSupportsSVE256 && Is256Bit) {
    // SVE doesn't have an equivalent ADDV instruction, so we make do
    // by performing two Adv. SIMD ADDV operations on the high and low
    // 128-bit lanes and then sum them up.

    const auto Mask = PRED_TMP_32B.Zeroing();
    const auto CompactPred = ARMEmitter::PReg::p0;

    // Select all our upper elements to run ADDV over them.
    not_(CompactPred, Mask, PRED_TMP_16B);
    compact(ARMEmitter::SubRegSize::i64Bit, VTMP1.Z(), CompactPred, Vector.Z());

    addv(SubRegSize.Vector, VTMP2.Q(), Vector.Q());
    addv(SubRegSize.Vector, VTMP1.Q(), VTMP1.Q());
    add(SubRegSize.Vector, Dst.Q(), VTMP1.Q(), VTMP2.Q());
  } else {
    if (ElementSize == 8) {
      addp(SubRegSize.Scalar, Dst, Vector);
    }
    else {
      addv(SubRegSize.Vector, Dst.Q(), Vector.Q());
    }
  }
}

DEF_OP(VUMinV) {
  const auto Op = IROp->C<IR::IROp_VUMinV>();
  const auto OpSize = IROp->Size;

  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;
  const auto ElementSize = Op->Header.ElementSize;

  const auto Dst = GetVReg(Node);
  const auto Vector = GetVReg(Op->Vector.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 1 || ElementSize == 2 || ElementSize == 4 || ElementSize == 8, "Invalid size");
  const auto SubRegSize =
    ElementSize == 1 ? ARMEmitter::SubRegSize::i8Bit :
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i8Bit;

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Pred = PRED_TMP_32B;
    uminv(SubRegSize, Dst, Pred, Vector.Z());
  } else {
    // Vector
    uminv(SubRegSize, Dst.Q(), Vector.Q());
  }
}

DEF_OP(VURAvg) {
  const auto Op = IROp->C<IR::IROp_VURAvg>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1.ID());
  const auto Vector2 = GetVReg(Op->Vector2.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 1 || ElementSize == 2 || ElementSize == 4 || ElementSize == 8, "Invalid size");
  const auto SubRegSize =
    ElementSize == 1 ? ARMEmitter::SubRegSize::i8Bit :
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i8Bit;

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Mask = PRED_TMP_32B.Merging();

    if (Dst == Vector1) {
      // Trivial case where we already have source data to be averaged in
      // the destination register. We can just do the operation in place.
      urhadd(SubRegSize, Dst.Z(), Mask, Dst.Z(), Vector2.Z());
    } else {
      // SVE URHADD is a destructive operation, so we need
      // a temporary for performing operations.
      movprfx(VTMP1.Z(), Vector1.Z());
      urhadd(SubRegSize, VTMP1.Z(), Mask, VTMP1.Z(), Vector2.Z());
      mov(Dst.Z(), VTMP1.Z());
    }
  } else {
    urhadd(SubRegSize, Dst.Q(), Vector1.Q(), Vector2.Q());
  }
}

DEF_OP(VAbs) {
  const auto Op = IROp->C<IR::IROp_VAbs>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto Src = GetVReg(Op->Vector.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 1 || ElementSize == 2 || ElementSize == 4 || ElementSize == 8, "Invalid size");
  const auto SubRegSize =
    ElementSize == 1 ? ARMEmitter::SubRegSize::i8Bit :
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i8Bit;

  if (HostSupportsSVE256 && Is256Bit) {
    abs(SubRegSize, Dst.Z(), PRED_TMP_32B.Merging(), Src.Z());
  } else {
    if (ElementSize == OpSize) {
      // Scalar
      abs(SubRegSize, Dst.D(), Src.D());
    } else {
      // Vector
      abs(SubRegSize, Dst.Q(), Src.Q());
    }
  }
}

DEF_OP(VPopcount) {
  const auto Op = IROp->C<IR::IROp_VPopcount>();
  const auto OpSize = IROp->Size;

  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;
  const auto IsScalar = OpSize == 8;

  const auto ElementSize = Op->Header.ElementSize;

  const auto Dst = GetVReg(Node);
  const auto Src = GetVReg(Op->Vector.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 1 || ElementSize == 2 || ElementSize == 4 || ElementSize == 8, "Invalid size");
  const auto SubRegSize =
    ElementSize == 1 ? ARMEmitter::SubRegSize::i8Bit :
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i8Bit;

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Pred = PRED_TMP_32B.Merging();
    cnt(SubRegSize, Dst.Z(), Pred, Src.Z());
  } else {
    if (IsScalar) {
      cnt(SubRegSize, Dst.D(), Src.D());
    } else {
      cnt(SubRegSize, Dst.Q(), Src.Q());
    }
  }
}

DEF_OP(VFAdd) {
  const auto Op = IROp->C<IR::IROp_VFAdd>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;
  const auto IsScalar = ElementSize == OpSize;

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1.ID());
  const auto Vector2 = GetVReg(Op->Vector2.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 2 || ElementSize == 4 || ElementSize == 8, "Invalid size");
  const auto SubRegSize =
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i8Bit;

  if (HostSupportsSVE256 && Is256Bit) {
    fadd(SubRegSize, Dst.Z(), Vector1.Z(), Vector2.Z());
  } else {
    if (IsScalar) {
      switch (ElementSize) {
        case 2: {
          fadd(Dst.H(), Vector1.H(), Vector2.H());
          break;
        }
        case 4: {
          fadd(Dst.S(), Vector1.S(), Vector2.S());
          break;
        }
        case 8: {
          fadd(Dst.D(), Vector1.D(), Vector2.D());
          break;
        }
        default:
          LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
          break;
      }
    } else {
      fadd(SubRegSize, Dst.Q(), Vector1.Q(), Vector2.Q());
    }
  }
}

DEF_OP(VFAddP) {
  const auto Op = IROp->C<IR::IROp_VFAddP>();
  const auto OpSize = IROp->Size;

  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;
  const auto ElementSize = Op->Header.ElementSize;

  const auto Dst = GetVReg(Node);
  const auto VectorLower = GetVReg(Op->VectorLower.ID());
  const auto VectorUpper = GetVReg(Op->VectorUpper.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 2 || ElementSize == 4 || ElementSize == 8, "Invalid size");
  const auto SubRegSize =
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i8Bit;

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Pred = PRED_TMP_32B.Merging();

    // SVE FADDP is a destructive operation, so we need a temporary
    movprfx(VTMP1.Z(), VectorLower.Z());

    // Unlike Adv. SIMD's version of FADDP, which acts like it concats the
    // upper vector onto the end of the lower vector and then performs
    // pairwise addition, the SVE version actually interleaves the
    // results of the pairwise addition (gross!), so we need to undo that.
    faddp(SubRegSize, VTMP1.Z(), Pred, VTMP1.Z(), VectorUpper.Z());
    uzp1(SubRegSize, Dst.Z(), VTMP1.Z(), VTMP1.Z());
    uzp2(SubRegSize, VTMP2.Z(), VTMP1.Z(), VTMP1.Z());

    // Merge upper half with lower half.
    splice<ARMEmitter::OpType::Destructive>(ARMEmitter::SubRegSize::i64Bit, Dst.Z(), PRED_TMP_16B, Dst.Z(), VTMP2.Z());
  } else {
    faddp(SubRegSize, Dst.Q(), VectorLower.Q(), VectorUpper.Q());
  }
}

DEF_OP(VFSub) {
  const auto Op = IROp->C<IR::IROp_VFSub>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;
  const auto IsScalar = ElementSize == OpSize;

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1.ID());
  const auto Vector2 = GetVReg(Op->Vector2.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 2 || ElementSize == 4 || ElementSize == 8, "Invalid size");
  const auto SubRegSize =
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i8Bit;

  if (HostSupportsSVE256 && Is256Bit) {
    fsub(SubRegSize, Dst.Z(), Vector1.Z(), Vector2.Z());
  } else {
    if (IsScalar) {
      switch (ElementSize) {
        case 2: {
          fsub(Dst.H(), Vector1.H(), Vector2.H());
          break;
        }
        case 4: {
          fsub(Dst.S(), Vector1.S(), Vector2.S());
          break;
        }
        case 8: {
          fsub(Dst.D(), Vector1.D(), Vector2.D());
          break;
        }
        default:
          LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
          break;
      }
    } else {
      fsub(SubRegSize, Dst.Q(), Vector1.Q(), Vector2.Q());
    }
  }
}

DEF_OP(VFMul) {
  const auto Op = IROp->C<IR::IROp_VFMul>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;
  const auto IsScalar = ElementSize == OpSize;

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1.ID());
  const auto Vector2 = GetVReg(Op->Vector2.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 2 || ElementSize == 4 || ElementSize == 8, "Invalid size");
  const auto SubRegSize =
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i8Bit;

  if (HostSupportsSVE256 && Is256Bit) {
    fmul(SubRegSize, Dst.Z(), Vector1.Z(), Vector2.Z());
  } else {
    if (IsScalar) {
      switch (ElementSize) {
        case 2: {
          fmul(Dst.H(), Vector1.H(), Vector2.H());
          break;
        }
        case 4: {
          fmul(Dst.S(), Vector1.S(), Vector2.S());
          break;
        }
        case 8: {
          fmul(Dst.D(), Vector1.D(), Vector2.D());
          break;
        }
        default:
          LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
          break;
      }
    } else {
      fmul(SubRegSize, Dst.Q(), Vector1.Q(), Vector2.Q());
    }
  }
}

DEF_OP(VFDiv) {
  const auto Op = IROp->C<IR::IROp_VFDiv>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto IsScalar = ElementSize == OpSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1.ID());
  const auto Vector2 = GetVReg(Op->Vector2.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 2 || ElementSize == 4 || ElementSize == 8, "Invalid size");
  const auto SubRegSize =
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i8Bit;

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Mask = PRED_TMP_32B.Merging();

    if (Dst == Vector1) {
      // Trivial case where we already have source data to be divided in the
      // destination register. We can just divide by Vector2 and be done with it.
      fdiv(SubRegSize, Dst.Z(), Mask, Dst.Z(), Vector2.Z());
    } else {
      // SVE FDIV is a destructive operation, so we need a temporary
      // in the event that Dst and Vector1 don't alias.
      movprfx(VTMP1.Z(), Vector1.Z());
      fdiv(SubRegSize, VTMP1.Z(), Mask, VTMP1.Z(), Vector2.Z());
      mov(Dst.Z(), VTMP1.Z());
    }
  } else {
    if (IsScalar) {
      switch (ElementSize) {
        case 2: {
          fdiv(Dst.H(), Vector1.H(), Vector2.H());
          break;
        }
        case 4: {
          fdiv(Dst.S(), Vector1.S(), Vector2.S());
          break;
        }
        case 8: {
          fdiv(Dst.D(), Vector1.D(), Vector2.D());
          break;
        }
        default:
          LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
          break;
      }
    } else {
      fdiv(SubRegSize, Dst.Q(), Vector1.Q(), Vector2.Q());
    }
  }
}

DEF_OP(VFMin) {
  const auto Op = IROp->C<IR::IROp_VFMin>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto IsScalar = ElementSize == OpSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1.ID());
  const auto Vector2 = GetVReg(Op->Vector2.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 2 || ElementSize == 4 || ElementSize == 8, "Invalid size");
  const auto SubRegSize =
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i8Bit;

  // NOTE: We don't directly use FMIN here for any of the implementations,
  //       because it has undesirable NaN handling behavior (it sets
  //       entries either to the incoming NaN value*, or the default NaN
  //       depending on FPCR flags set). We want behavior that sets NaN
  //       entries to zero for the comparison result.
  //
  // * - Not exactly (differs slightly with SNaNs), but close enough for the explanation

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Mask = PRED_TMP_32B;
    const auto ComparePred = ARMEmitter::PReg::p0;

    // General idea:
    // 1. Compare greater than against the two vectors
    // 2. Invert the resulting values in the predicate register.
    // 3. Move the first vector into a temporary
    // 4. Merge all the elements that correspond to the inverted
    //    predicate bits from the second vector into the
    //    same temporary.
    // 5. Move temporary into the destination register and we're done.
    fcmgt(SubRegSize, ComparePred, Mask.Zeroing(),
          Vector2.Z(), Vector1.Z());
    not_(ComparePred, Mask.Zeroing(), ComparePred);
    mov(VTMP1.Z(), Vector1.Z());
    mov(SubRegSize, VTMP1.Z(), ComparePred.Merging(), Vector2.Z());
    mov(Dst.Z(), VTMP1.Z());
  } else {
    if (IsScalar) {
      switch (ElementSize) {
        case 2: {
          fcmp(Vector1.H(), Vector2.H());
          fcsel(Dst.H(), Vector1.H(), Vector2.H(), ARMEmitter::Condition::CC_MI);
          break;
        }
        case 4: {
          fcmp(Vector1.S(), Vector2.S());
          fcsel(Dst.S(), Vector1.S(), Vector2.S(), ARMEmitter::Condition::CC_MI);
          break;
        }
        case 8: {
          fcmp(Vector1.D(), Vector2.D());
          fcsel(Dst.D(), Vector1.D(), Vector2.D(), ARMEmitter::Condition::CC_MI);
          break;
        }
        default:
          LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
          break;
      }
    } else {
      if (Dst == Vector1) {
        // Destination is already Vector1, need to insert Vector2 on false.
        fcmgt(SubRegSize, VTMP1.Q(), Vector2.Q(), Vector1.Q());
        bif(Dst.Q(), Vector2.Q(), VTMP1.Q());
      }
      else if (Dst == Vector2) {
        // Destination is already Vector2, Invert arguments and insert Vector1 on false.
        fcmgt(SubRegSize, VTMP1.Q(), Vector1.Q(), Vector2.Q());
        bif(Dst.Q(), Vector1.Q(), VTMP1.Q());
      }
      else {
        // Dst is not either source, need a move.
        fcmgt(SubRegSize, VTMP1.Q(), Vector2.Q(), Vector1.Q());
        mov(Dst.Q(), Vector1.Q());
        bif(Dst.Q(), Vector2.Q(), VTMP1.Q());
      }
    }
  }
}

DEF_OP(VFMax) {
  const auto Op = IROp->C<IR::IROp_VFMax>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto IsScalar = ElementSize == OpSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1.ID());
  const auto Vector2 = GetVReg(Op->Vector2.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 2 || ElementSize == 4 || ElementSize == 8, "Invalid size");
  const auto SubRegSize =
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i8Bit;

  // NOTE: See VFMin implementation for reasons why we
  //       don't just use FMAX/FMIN for these implementations.

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Mask = PRED_TMP_32B;
    const auto ComparePred = ARMEmitter::PReg::p0;

    fcmgt(SubRegSize, ComparePred, Mask.Zeroing(),
          Vector2.Z(), Vector1.Z());
    mov(VTMP1.Z(), Vector1.Z());
    mov(SubRegSize, VTMP1.Z(), ComparePred.Merging(), Vector2.Z());
    mov(Dst.Z(), VTMP1.Z());
  } else {
    if (IsScalar) {
      switch (ElementSize) {
        case 2: {
          fcmp(Vector1.H(), Vector2.H());
          fcsel(Dst.H(), Vector2.H(), Vector1.H(), ARMEmitter::Condition::CC_MI);
          break;
        }
        case 4: {
          fcmp(Vector1.S(), Vector2.S());
          fcsel(Dst.S(), Vector2.S(), Vector1.S(), ARMEmitter::Condition::CC_MI);
          break;
        }
        case 8: {
          fcmp(Vector1.D(), Vector2.D());
          fcsel(Dst.D(), Vector2.D(), Vector1.D(), ARMEmitter::Condition::CC_MI);
          break;
        }
        default:
          LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
          break;
      }
    } else {
      if (Dst == Vector1) {
        // Destination is already Vector1, need to insert Vector2 on true.
        fcmgt(SubRegSize, VTMP1.Q(), Vector2.Q(), Vector1.Q());
        bit(Dst.Q(), Vector2.Q(), VTMP1.Q());
      }
      else if (Dst == Vector2) {
        // Destination is already Vector2, Invert arguments and insert Vector1 on true.
        fcmgt(SubRegSize, VTMP1.Q(), Vector1.Q(), Vector2.Q());
        bit(Dst.Q(), Vector1.Q(), VTMP1.Q());
      }
      else {
        // Dst is not either source, need a move.
        fcmgt(SubRegSize, VTMP1.Q(), Vector2.Q(), Vector1.Q());
        mov(Dst.Q(), Vector1.Q());
        bit(Dst.Q(), Vector2.Q(), VTMP1.Q());
      }
    }
  }
}

DEF_OP(VFRecp) {
  const auto Op = IROp->C<IR::IROp_VFRecp>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto IsScalar = Op->Header.ElementSize == OpSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto Vector = GetVReg(Op->Vector.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 2 || ElementSize == 4 || ElementSize == 8, "Invalid size");
  const auto SubRegSize = ARMEmitter::ToVectorSizePair(
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i128Bit);

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Pred = PRED_TMP_32B.Merging();

    fmov(SubRegSize.Vector, VTMP1.Z(), 1.0);
    fdiv(SubRegSize.Vector, VTMP1.Z(), Pred, VTMP1.Z(), Vector.Z());
    mov(Dst.Z(), VTMP1.Z());
  } else {
    if (IsScalar) {
      fmov(SubRegSize.Scalar, VTMP1.Q(), 1.0f);
      switch (ElementSize) {
        case 2: {
          fdiv(Dst.H(), VTMP1.H(), Vector.H());
          break;
        }
        case 4: {
          fdiv(Dst.S(), VTMP1.S(), Vector.S());
          break;
        }
        case 8: {
          fdiv(Dst.D(), VTMP1.D(), Vector.D());
          break;
        }
        default:
          LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
          break;
      }
    } else {
      fmov(SubRegSize.Vector, VTMP1.Q(), 1.0f);
      fdiv(SubRegSize.Vector, Dst.Q(), VTMP1.Q(), Vector.Q());
    }
  }
}

DEF_OP(VFSqrt) {
  const auto Op = IROp->C<IR::IROp_VFRSqrt>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto IsScalar = ElementSize == OpSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto Vector = GetVReg(Op->Vector.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 2 || ElementSize == 4 || ElementSize == 8, "Invalid size");
  const auto SubRegSize =
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i8Bit;

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Pred = PRED_TMP_32B.Merging();

    fsqrt(SubRegSize, Dst.Z(), Pred, Vector.Z());
  } else {
    if (IsScalar) {
      switch (ElementSize) {
        case 2: {
          fsqrt(Dst.H(), Vector.H());
          break;
        }
        case 4: {
          fsqrt(Dst.S(), Vector.S());
          break;
        }
        case 8: {
          fsqrt(Dst.D(), Vector.D());
          break;
        }
        default:
          LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
          break;
      }
    } else {
      fsqrt(SubRegSize, Dst.Q(), Vector.Q());
    }
  }
}

DEF_OP(VFRSqrt) {
  const auto Op = IROp->C<IR::IROp_VFRSqrt>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto IsScalar = ElementSize == OpSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto Vector = GetVReg(Op->Vector.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 2 || ElementSize == 4 || ElementSize == 8, "Invalid size");
  const auto SubRegSize = ARMEmitter::ToVectorSizePair(
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i128Bit);

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Pred = PRED_TMP_32B.Merging();
    fsqrt(SubRegSize.Vector, VTMP1.Z(), Pred, Vector.Z());
    fmov(SubRegSize.Vector, Dst.Z(), 1.0);
    fdiv(SubRegSize.Vector, Dst.Z(), Pred, Dst.Z(), VTMP1.Z());
  } else {
    if (IsScalar) {
      fmov(SubRegSize.Scalar, VTMP1.Q(), 1.0);
      switch (ElementSize) {
        case 2: {
          fsqrt(VTMP2.H(), Vector.H());
          fdiv(Dst.H(), VTMP1.H(), VTMP2.H());
          break;
        }
        case 4: {
          fsqrt(VTMP2.S(), Vector.S());
          fdiv(Dst.S(), VTMP1.S(), VTMP2.S());
          break;
        }
        case 8: {
          fsqrt(VTMP2.D(), Vector.D());
          fdiv(Dst.D(), VTMP1.D(), VTMP2.D());
          break;
        }
        default:
          LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
          break;
      }
    } else {
      fmov(SubRegSize.Vector, VTMP1.Q(), 1.0);
      fsqrt(SubRegSize.Vector, VTMP2.Q(), Vector.Q());
      fdiv(SubRegSize.Vector, Dst.Q(), VTMP1.Q(), VTMP2.Q());
    }
  }
}

DEF_OP(VNeg) {
  const auto Op = IROp->C<IR::IROp_VNeg>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto Vector= GetVReg(Op->Vector.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 1 || ElementSize == 2 || ElementSize == 4 || ElementSize == 8, "Invalid size");
  const auto SubRegSize =
    ElementSize == 1 ? ARMEmitter::SubRegSize::i8Bit :
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i8Bit;

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Pred = PRED_TMP_32B.Merging();
    neg(SubRegSize, Dst.Z(), Pred, Vector.Z());
  } else {
    neg(SubRegSize, Dst.Q(), Vector.Q());
  }
}

DEF_OP(VFNeg) {
  const auto Op = IROp->C<IR::IROp_VFNeg>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto Vector = GetVReg(Op->Vector.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 2 || ElementSize == 4 || ElementSize == 8, "Invalid size");
  const auto SubRegSize =
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i8Bit;

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Pred = PRED_TMP_32B.Merging();

    fneg(SubRegSize, Dst.Z(), Pred, Vector.Z());
  } else {
    fneg(SubRegSize, Dst.Q(), Vector.Q());
  }
}

DEF_OP(VNot) {
  const auto Op = IROp->C<IR::IROp_VNot>();
  const auto OpSize = IROp->Size;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto Vector = GetVReg(Op->Vector.ID());

  if (HostSupportsSVE256 && Is256Bit) {
    not_(ARMEmitter::SubRegSize::i8Bit, Dst.Z(), PRED_TMP_32B.Merging(), Vector.Z());
  } else {
    mvn(ARMEmitter::SubRegSize::i8Bit, Dst.Q(), Vector.Q());
  }
}

DEF_OP(VUMin) {
  const auto Op = IROp->C<IR::IROp_VUMin>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1.ID());
  const auto Vector2 = GetVReg(Op->Vector2.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 1 || ElementSize == 2 || ElementSize == 4 || ElementSize == 8 || ElementSize == 16, "Invalid size");
  const auto SubRegSize =
    ElementSize == 1 ? ARMEmitter::SubRegSize::i8Bit :
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i128Bit;

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Pred = PRED_TMP_32B.Merging();

    if (Dst == Vector1) {
      // Trivial case where we can perform the operation in place.
      umin(SubRegSize, Dst.Z(), Pred, Dst.Z(), Vector2.Z());
    } else {
      // SVE UMIN is a destructive operation so we need a temporary.
      movprfx(VTMP1.Z(), Vector1.Z());
      umin(SubRegSize, VTMP1.Z(), Pred, VTMP1.Z(), Vector2.Z());
      mov(Dst.Z(), VTMP1.Z());
    }
  } else {
    switch (ElementSize) {
      case 1:
      case 2:
      case 4: {
        umin(SubRegSize, Dst.Q(), Vector1.Q(), Vector2.Q());
        break;
      }
      case 8: {
        cmhi(SubRegSize, VTMP1.Q(), Vector2.Q(), Vector1.Q());
        mov(VTMP2.Q(), Vector1.Q());
        bif(VTMP2.Q(), Vector2.Q(), VTMP1.Q());
        mov(Dst.Q(), VTMP2.Q());
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
    }
  }
}

DEF_OP(VSMin) {
  const auto Op = IROp->C<IR::IROp_VSMin>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1.ID());
  const auto Vector2 = GetVReg(Op->Vector2.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 1 || ElementSize == 2 || ElementSize == 4 || ElementSize == 8 || ElementSize == 16, "Invalid size");
  const auto SubRegSize =
    ElementSize == 1 ? ARMEmitter::SubRegSize::i8Bit :
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i128Bit;

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Pred = PRED_TMP_32B.Merging();

    if (Dst == Vector1) {
      // Trivial case where we can perform the operation in place.
      smin(SubRegSize, Dst.Z(), Pred, Dst.Z(), Vector2.Z());
    } else {
      // SVE SMIN is a destructive operation, so we need a temporary.
      movprfx(VTMP1.Z(), Vector1.Z());
      smin(SubRegSize, VTMP1.Z(), Pred, VTMP1.Z(), Vector2.Z());
      mov(Dst.Z(), VTMP1.Z());
    }
  } else {
    switch (ElementSize) {
      case 1:
      case 2:
      case 4: {
        smin(SubRegSize, Dst.Q(), Vector1.Q(), Vector2.Q());
        break;
      }
      case 8: {
        cmgt(SubRegSize, VTMP1.Q(), Vector1.Q(), Vector2.Q());
        mov(VTMP2.Q(), Vector1.Q());
        bif(VTMP2.Q(), Vector2.Q(), VTMP1.Q());
        mov(Dst.Q(), VTMP2.Q());
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
    }
  }
}

DEF_OP(VUMax) {
  const auto Op = IROp->C<IR::IROp_VUMax>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1.ID());
  const auto Vector2 = GetVReg(Op->Vector2.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 1 || ElementSize == 2 || ElementSize == 4 || ElementSize == 8 || ElementSize == 16, "Invalid size");
  const auto SubRegSize =
    ElementSize == 1 ? ARMEmitter::SubRegSize::i8Bit :
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i128Bit;

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Pred = PRED_TMP_32B.Merging();

    if (Dst == Vector1) {
      // Trivial case where we can perform the operation in place.
      umax(SubRegSize, Dst.Z(), Pred, Dst.Z(), Vector2.Z());
    } else {
      // SVE UMAX is a destructive operation, so we need a temporary.
      movprfx(VTMP1.Z(), Vector1.Z());
      umax(SubRegSize, VTMP1.Z(), Pred, VTMP1.Z(), Vector2.Z());
      mov(Dst.Z(), VTMP1.Z());
    }
  } else {
    switch (ElementSize) {
      case 1:
      case 2:
      case 4: {
        umax(SubRegSize, Dst.Q(), Vector1.Q(), Vector2.Q());
        break;
      }
      case 8: {
        cmhi(SubRegSize, VTMP1.Q(), Vector2.Q(), Vector1.Q());
        mov(VTMP2.Q(), Vector1.Q());
        bif(VTMP2.Q(), Vector2.Q(), VTMP1.Q());
        mov(Dst.Q(), VTMP2.Q());
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
    }
  }
}

DEF_OP(VSMax) {
  const auto Op = IROp->C<IR::IROp_VSMax>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1.ID());
  const auto Vector2 = GetVReg(Op->Vector2.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 1 || ElementSize == 2 || ElementSize == 4 || ElementSize == 8 || ElementSize == 16, "Invalid size");
  const auto SubRegSize =
    ElementSize == 1 ? ARMEmitter::SubRegSize::i8Bit :
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i128Bit;

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Pred = PRED_TMP_32B.Merging();

    if (Dst == Vector1) {
      // Trivial case where we can perform the operation in place.
      smax(SubRegSize, Dst.Z(), Pred, Dst.Z(), Vector2.Z());
    } else {
      // SVE SMAX is a destructive operation, so we need a temporary.
      movprfx(VTMP1.Z(), Vector1.Z());
      smax(SubRegSize, VTMP1.Z(), Pred, VTMP1.Z(), Vector2.Z());
      mov(Dst.Z(), VTMP1.Z());
    }
  } else {
    switch (ElementSize) {
      case 1:
      case 2:
      case 4: {
        smax(SubRegSize, Dst.Q(), Vector1.Q(), Vector2.Q());
        break;
      }
      case 8: {
        cmgt(SubRegSize, VTMP1.Q(), Vector2.Q(), Vector1.Q());
        mov(VTMP2.Q(), Vector1.Q());
        bif(VTMP2.Q(), Vector2.Q(), VTMP1.Q());
        mov(Dst.Q(), VTMP2.Q());
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
    }
  }
}

DEF_OP(VZip) {
  const auto Op = IROp->C<IR::IROp_VZip>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto VectorLower = GetVReg(Op->VectorLower.ID());
  const auto VectorUpper = GetVReg(Op->VectorUpper.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 1 || ElementSize == 2 || ElementSize == 4 || ElementSize == 8, "Invalid size");
  const auto SubRegSize =
    ElementSize == 1 ? ARMEmitter::SubRegSize::i8Bit :
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i8Bit;

  if (HostSupportsSVE256 && Is256Bit) {
    zip1(SubRegSize, Dst.Z(), VectorLower.Z(), VectorUpper.Z());
  } else {
    if (OpSize == 8) {
      zip1(SubRegSize, Dst.D(), VectorLower.D(), VectorUpper.D());
    } else {
      zip1(SubRegSize, Dst.Q(), VectorLower.Q(), VectorUpper.Q());
    }
  }
}

DEF_OP(VZip2) {
  const auto Op = IROp->C<IR::IROp_VZip2>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto VectorLower = GetVReg(Op->VectorLower.ID());
  const auto VectorUpper = GetVReg(Op->VectorUpper.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 1 || ElementSize == 2 || ElementSize == 4 || ElementSize == 8, "Invalid size");
  const auto SubRegSize =
    ElementSize == 1 ? ARMEmitter::SubRegSize::i8Bit :
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i8Bit;

  if (HostSupportsSVE256 && Is256Bit) {
    zip2(SubRegSize, Dst.Z(), VectorLower.Z(), VectorUpper.Z());
  } else {
    if (OpSize == 8) {
      zip2(SubRegSize, Dst.D(), VectorLower.D(), VectorUpper.D());
    } else {
      zip2(SubRegSize, Dst.Q(), VectorLower.Q(), VectorUpper.Q());
    }
  }
}

DEF_OP(VUnZip) {
  const auto Op = IROp->C<IR::IROp_VUnZip>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto VectorLower = GetVReg(Op->VectorLower.ID());
  const auto VectorUpper = GetVReg(Op->VectorUpper.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 1 || ElementSize == 2 || ElementSize == 4 || ElementSize == 8, "Invalid size");
  const auto SubRegSize =
    ElementSize == 1 ? ARMEmitter::SubRegSize::i8Bit :
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i8Bit;

  if (HostSupportsSVE256 && Is256Bit) {
    uzp1(SubRegSize, Dst.Z(), VectorLower.Z(), VectorUpper.Z());
  } else {
    if (OpSize == 8) {
      uzp1(SubRegSize, Dst.D(), VectorLower.D(), VectorUpper.D());
    } else {
      uzp1(SubRegSize, Dst.Q(), VectorLower.Q(), VectorUpper.Q());
    }
  }
}

DEF_OP(VUnZip2) {
  const auto Op = IROp->C<IR::IROp_VUnZip2>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto VectorLower = GetVReg(Op->VectorLower.ID());
  const auto VectorUpper = GetVReg(Op->VectorUpper.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 1 || ElementSize == 2 || ElementSize == 4 || ElementSize == 8, "Invalid size");
  const auto SubRegSize =
    ElementSize == 1 ? ARMEmitter::SubRegSize::i8Bit :
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i8Bit;

  if (HostSupportsSVE256 && Is256Bit) {
    uzp2(SubRegSize, Dst.Z(), VectorLower.Z(), VectorUpper.Z());
  } else {
    if (OpSize == 8) {
      uzp2(SubRegSize, Dst.D(), VectorLower.D(), VectorUpper.D());
    } else {
      uzp2(SubRegSize, Dst.Q(), VectorLower.Q(), VectorUpper.Q());
    }
  }
}

DEF_OP(VTrn) {
  const auto Op = IROp->C<IR::IROp_VTrn>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto VectorLower = GetVReg(Op->VectorLower.ID());
  const auto VectorUpper = GetVReg(Op->VectorUpper.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 1 || ElementSize == 2 || ElementSize == 4 || ElementSize == 8, "Invalid size");
  const auto SubRegSize =
    ElementSize == 1 ? ARMEmitter::SubRegSize::i8Bit :
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i8Bit;

  if (HostSupportsSVE256 && Is256Bit) {
    trn1(SubRegSize, Dst.Z(), VectorLower.Z(), VectorUpper.Z());
  } else {
    if (OpSize == 8) {
      trn1(SubRegSize, Dst.D(), VectorLower.D(), VectorUpper.D());
    } else {
      trn1(SubRegSize, Dst.Q(), VectorLower.Q(), VectorUpper.Q());
    }
  }
}

DEF_OP(VTrn2) {
  const auto Op = IROp->C<IR::IROp_VTrn2>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto VectorLower = GetVReg(Op->VectorLower.ID());
  const auto VectorUpper = GetVReg(Op->VectorUpper.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 1 || ElementSize == 2 || ElementSize == 4 || ElementSize == 8, "Invalid size");
  const auto SubRegSize =
    ElementSize == 1 ? ARMEmitter::SubRegSize::i8Bit :
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i8Bit;

  if (HostSupportsSVE256 && Is256Bit) {
    trn2(SubRegSize, Dst.Z(), VectorLower.Z(), VectorUpper.Z());
  } else {
    if (OpSize == 8) {
      trn2(SubRegSize, Dst.D(), VectorLower.D(), VectorUpper.D());
    } else {
      trn2(SubRegSize, Dst.Q(), VectorLower.Q(), VectorUpper.Q());
    }
  }
}

DEF_OP(VBSL) {
  const auto Op = IROp->C<IR::IROp_VBSL>();
  const auto OpSize = IROp->Size;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto VectorFalse = GetVReg(Op->VectorFalse.ID());
  const auto VectorTrue = GetVReg(Op->VectorTrue.ID());
  const auto VectorMask = GetVReg(Op->VectorMask.ID());

  if (HostSupportsSVE256 && Is256Bit) {
    // NOTE: Slight parameter difference from ASIMD
    //       ASIMD -> BSL Mask, True, False
    //       SVE   -> BSL True, True, False, Mask
    //       ASIMD -> BIT True, False, Mask
    //       ASIMD -> BIF False, True, Mask
    movprfx(VTMP1.Z(), VectorTrue.Z());
    bsl(VTMP1.Z(), VTMP1.Z(), VectorFalse.Z(), VectorMask.Z());
    mov(Dst.Z(), VTMP1.Z());
  } else {
    if (VectorMask == Dst) {
      // Can use BSL without any moves.
      if (OpSize == 8) {
        bsl(Dst.D(), VectorTrue.D(), VectorFalse.D());
      }
      else {
        bsl(Dst.Q(), VectorTrue.Q(), VectorFalse.Q());
      }
    }
    else if (VectorTrue == Dst) {
      // Can use BIF without any moves.
      if (OpSize == 8) {
        bif(Dst.D(), VectorFalse.D(), VectorMask.D());
      }
      else {
        bif(Dst.Q(), VectorFalse.Q(), VectorMask.Q());
      }
    }
    else if (VectorFalse == Dst) {
      // Can use BIT without any moves.
      if (OpSize == 8) {
        bit(Dst.D(), VectorTrue.D(), VectorMask.D());
      }
      else {
        bit(Dst.Q(), VectorTrue.Q(), VectorMask.Q());
      }
    }
    else {
      // Needs moves.
      if (OpSize == 8) {
        mov(Dst.D(), VectorMask.D());
        bsl(Dst.D(), VectorTrue.D(), VectorFalse.D());
      } else {
        mov(Dst.Q(), VectorMask.Q());
        bsl(Dst.Q(), VectorTrue.Q(), VectorFalse.Q());
      }
    }
  }
}

DEF_OP(VCMPEQ) {
  const auto Op = IROp->C<IR::IROp_VCMPEQ>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto IsScalar = ElementSize == OpSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1.ID());
  const auto Vector2 = GetVReg(Op->Vector2.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 1 || ElementSize == 2 || ElementSize == 4 || ElementSize == 8 || ElementSize == 16, "Invalid size");
  const auto SubRegSize = ARMEmitter::ToVectorSizePair(
    ElementSize == 1 ? ARMEmitter::SubRegSize::i8Bit :
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i128Bit);

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Mask = PRED_TMP_32B.Zeroing();
    const auto ComparePred = ARMEmitter::PReg::p0;

    // General idea is to compare for equality, not the equal vals
    // from one of the registers, then or both together to make the
    // relevant equal entries all 1s.
    cmpeq(SubRegSize.Vector, ComparePred, Mask, Vector1.Z(), Vector2.Z());
    not_(SubRegSize.Vector, VTMP1.Z(), ComparePred.Merging(), Vector1.Z());
    movprfx(SubRegSize.Vector, Dst.Z(), ComparePred.Zeroing(), Vector1.Z());
    orr(SubRegSize.Vector, Dst.Z(), ComparePred.Merging(), Dst.Z(), VTMP1.Z());
  } else {
    if (IsScalar) {
      cmeq(SubRegSize.Scalar, Dst, Vector1, Vector2);
    } else {
      cmeq(SubRegSize.Vector, Dst.Q(), Vector1.Q(), Vector2.Q());
    }
  }
}

DEF_OP(VCMPEQZ) {
  const auto Op = IROp->C<IR::IROp_VCMPEQZ>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto IsScalar = ElementSize == OpSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto Vector = GetVReg(Op->Vector.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 1 || ElementSize == 2 || ElementSize == 4 || ElementSize == 8 || ElementSize == 16, "Invalid size");
  const auto SubRegSize = ARMEmitter::ToVectorSizePair(
    ElementSize == 1 ? ARMEmitter::SubRegSize::i8Bit :
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i128Bit);

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Mask = PRED_TMP_32B.Zeroing();
    const auto ComparePred = ARMEmitter::PReg::p0;

    // Ensure no junk is in the temp (important for ensuring
    // non-equal entries remain as zero).
    mov_imm(ARMEmitter::SubRegSize::i64Bit, VTMP1.Z(), 0);
    // Unlike with VCMPEQ, we can skip needing to bitwise OR the
    // final results, since if our elements are equal to zero,
    // we just need to bitwise NOT them and they're already set
    // to all 1s.
    cmpeq(SubRegSize.Vector, ComparePred, Mask, Vector.Z(), 0);
    not_(SubRegSize.Vector, VTMP1.Z(), ComparePred.Merging(), Vector.Z());
    mov(Dst.Z(), VTMP1.Z());
  } else {
    if (IsScalar) {
      cmeq(SubRegSize.Scalar, Dst, Vector);
    } else {
      cmeq(SubRegSize.Vector, Dst.Q(), Vector.Q());
    }
  }
}

DEF_OP(VCMPGT) {
  const auto Op = IROp->C<IR::IROp_VCMPGT>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto IsScalar = ElementSize == OpSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1.ID());
  const auto Vector2 = GetVReg(Op->Vector2.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 1 || ElementSize == 2 || ElementSize == 4 || ElementSize == 8 || ElementSize == 16, "Invalid size");
  const auto SubRegSize = ARMEmitter::ToVectorSizePair(
    ElementSize == 1 ? ARMEmitter::SubRegSize::i8Bit :
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i128Bit);

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Mask = PRED_TMP_32B.Zeroing();
    const auto ComparePred = ARMEmitter::PReg::p0;

    // General idea is to compare for greater-than, bitwise NOT
    // the valid values, then ORR the NOTed values with the original
    // values to form entries that are all 1s.
    cmpgt(SubRegSize.Vector, ComparePred, Mask, Vector1.Z(), Vector2.Z());
    not_(SubRegSize.Vector, VTMP1.Z(), ComparePred.Merging(), Vector1.Z());
    movprfx(SubRegSize.Vector, Dst.Z(), ComparePred.Zeroing(), Vector1.Z());
    orr(SubRegSize.Vector, Dst.Z(), ComparePred.Merging(), Dst.Z(), VTMP1.Z());
  } else {
    if (IsScalar) {
      cmgt(SubRegSize.Scalar, Dst, Vector1, Vector2);
    } else {
      cmgt(SubRegSize.Vector, Dst.Q(), Vector1.Q(), Vector2.Q());
    }
  }
}

DEF_OP(VCMPGTZ) {
  const auto Op = IROp->C<IR::IROp_VCMPGTZ>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto IsScalar = ElementSize == OpSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto Vector = GetVReg(Op->Vector.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 1 || ElementSize == 2 || ElementSize == 4 || ElementSize == 8 || ElementSize == 16, "Invalid size");
  const auto SubRegSize = ARMEmitter::ToVectorSizePair(
    ElementSize == 1 ? ARMEmitter::SubRegSize::i8Bit :
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i128Bit);

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Mask = PRED_TMP_32B.Zeroing();
    const auto ComparePred = ARMEmitter::PReg::p0;

    // Ensure no junk is in the temp (important for ensuring
    // non greater-than values remain as zero).
    mov_imm(ARMEmitter::SubRegSize::i64Bit, VTMP1.Z(), 0);
    cmpgt(SubRegSize.Vector, ComparePred, Mask, Vector.Z(), 0);
    not_(SubRegSize.Vector, VTMP1.Z(), ComparePred.Merging(), Vector.Z());
    orr(SubRegSize.Vector, VTMP1.Z(), ComparePred.Merging(), VTMP1.Z(), Vector.Z());
    mov(Dst.Z(), VTMP1.Z());
  } else {
    if (IsScalar) {
      cmgt(SubRegSize.Scalar, Dst, Vector);
    } else {
      cmgt(SubRegSize.Vector, Dst.Q(), Vector.Q());
    }
  }
}

DEF_OP(VCMPLTZ) {
  const auto Op = IROp->C<IR::IROp_VCMPLTZ>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto IsScalar = ElementSize == OpSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto Vector = GetVReg(Op->Vector.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 1 || ElementSize == 2 || ElementSize == 4 || ElementSize == 8 || ElementSize == 16, "Invalid size");
  const auto SubRegSize = ARMEmitter::ToVectorSizePair(
    ElementSize == 1 ? ARMEmitter::SubRegSize::i8Bit :
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i128Bit);

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Mask = PRED_TMP_32B.Zeroing();
    const auto ComparePred = ARMEmitter::PReg::p0;

    // Ensure no junk is in the temp (important for ensuring
    // non less-than values remain as zero).
    mov_imm(ARMEmitter::SubRegSize::i64Bit, VTMP1.Z(), 0);
    cmplt(SubRegSize.Vector, ComparePred, Mask, Vector.Z(), 0);
    not_(SubRegSize.Vector, VTMP1.Z(), ComparePred.Merging(), Vector.Z());
    orr(SubRegSize.Vector, VTMP1.Z(), ComparePred.Merging(), VTMP1.Z(), Vector.Z());
    mov(Dst.Z(), VTMP1.Z());
  } else {
    if (IsScalar) {
      cmlt(SubRegSize.Scalar, Dst, Vector);
    } else {
      cmlt(SubRegSize.Vector, Dst.Q(), Vector.Q());
    }
  }
}

DEF_OP(VFCMPEQ) {
  const auto Op = IROp->C<IR::IROp_VFCMPEQ>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto IsScalar = ElementSize == OpSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1.ID());
  const auto Vector2 = GetVReg(Op->Vector2.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 2 || ElementSize == 4 || ElementSize == 8, "Invalid size");
  const auto SubRegSize = ARMEmitter::ToVectorSizePair(
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i8Bit);

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Mask = PRED_TMP_32B.Zeroing();
    const auto ComparePred = ARMEmitter::PReg::p0;

    fcmeq(SubRegSize.Vector, ComparePred, Mask, Vector1.Z(), Vector2.Z());
    not_(SubRegSize.Vector, VTMP1.Z(), ComparePred.Merging(), Vector1.Z());
    movprfx(SubRegSize.Vector, Dst.Z(), ComparePred.Zeroing(), Vector1.Z());
    orr(SubRegSize.Vector, Dst.Z(), ComparePred.Merging(), Dst.Z(), VTMP1.Z());
  } else {
    if (IsScalar) {
      switch (ElementSize) {
        case 2: {
          fcmeq(Dst.H(), Vector1.H(), Vector2.H());
          break;
        }
        case 4:
        case 8:
          fcmeq(SubRegSize.Scalar, Dst, Vector1, Vector2);
          break;
        default:
          LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
          break;
      }
    } else {
      fcmeq(SubRegSize.Vector, Dst.Q(), Vector1.Q(), Vector2.Q());
    }
  }
}

DEF_OP(VFCMPNEQ) {
  const auto Op = IROp->C<IR::IROp_VFCMPNEQ>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto IsScalar = ElementSize == OpSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1.ID());
  const auto Vector2 = GetVReg(Op->Vector2.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 2 || ElementSize == 4 || ElementSize == 8, "Invalid size");
  const auto SubRegSize = ARMEmitter::ToVectorSizePair(
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i8Bit);

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Mask = PRED_TMP_32B.Zeroing();
    const auto ComparePred = ARMEmitter::PReg::p0;

    fcmne(SubRegSize.Vector, ComparePred, Mask, Vector1.Z(), Vector2.Z());
    not_(SubRegSize.Vector, VTMP1.Z(), ComparePred.Merging(), Vector1.Z());
    movprfx(SubRegSize.Vector, Dst.Z(), ComparePred.Zeroing(), Vector1.Z());
    orr(SubRegSize.Vector, Dst.Z(), ComparePred.Merging(), Dst.Z(), VTMP1.Z());
  } else {
    if (IsScalar) {
      switch (ElementSize) {
        case 2: {
          fcmeq(Dst.H(), Vector1.H(), Vector2.H());
          break;
        }
        case 4:
        case 8:
          fcmeq(SubRegSize.Scalar, Dst, Vector1, Vector2);
          break;
        default:
          LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
          break;
      }
      mvn(ARMEmitter::SubRegSize::i8Bit, Dst.D(), Dst.D());
    } else {
      fcmeq(SubRegSize.Vector, Dst.Q(), Vector1.Q(), Vector2.Q());
      mvn(ARMEmitter::SubRegSize::i8Bit, Dst.Q(), Dst.Q());
    }
  }
}

DEF_OP(VFCMPLT) {
  const auto Op = IROp->C<IR::IROp_VFCMPLT>();
  const auto  OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto IsScalar = ElementSize == OpSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1.ID());
  const auto Vector2 = GetVReg(Op->Vector2.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 2 || ElementSize == 4 || ElementSize == 8, "Invalid size");
  const auto SubRegSize = ARMEmitter::ToVectorSizePair(
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i8Bit);

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Mask = PRED_TMP_32B.Zeroing();
    const auto ComparePred = ARMEmitter::PReg::p0;

    fcmgt(SubRegSize.Vector, ComparePred, Mask, Vector2.Z(), Vector1.Z());
    not_(SubRegSize.Vector, VTMP1.Z(), ComparePred.Merging(), Vector2.Z());
    movprfx(SubRegSize.Vector, Dst.Z(), ComparePred.Zeroing(), Vector2.Z());
    orr(SubRegSize.Vector, Dst.Z(), ComparePred.Merging(), Dst.Z(), VTMP1.Z());
  } else {
    if (IsScalar) {
      switch (ElementSize) {
        case 2: {
          fcmgt(Dst.H(), Vector2.H(), Vector1.H());
          break;
        }
        case 4:
        case 8:
          fcmgt(SubRegSize.Scalar, Dst, Vector2, Vector1);
          break;
        default:
          LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
          break;
      }
    } else {
      fcmgt(SubRegSize.Vector, Dst.Q(), Vector2.Q(), Vector1.Q());
    }
  }
}

DEF_OP(VFCMPGT) {
  const auto Op = IROp->C<IR::IROp_VFCMPGT>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto IsScalar = ElementSize == OpSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1.ID());
  const auto Vector2 = GetVReg(Op->Vector2.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 2 || ElementSize == 4 || ElementSize == 8, "Invalid size");
  const auto SubRegSize = ARMEmitter::ToVectorSizePair(
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i8Bit);

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Mask = PRED_TMP_32B.Zeroing();
    const auto ComparePred = ARMEmitter::PReg::p0;

    fcmgt(SubRegSize.Vector, ComparePred, Mask, Vector1.Z(), Vector2.Z());
    not_(SubRegSize.Vector, VTMP1.Z(), ComparePred.Merging(), Vector1.Z());
    movprfx(SubRegSize.Vector, Dst.Z(), ComparePred.Zeroing(), Vector1.Z());
    orr(SubRegSize.Vector, Dst.Z(), ComparePred.Merging(), Dst.Z(), VTMP1.Z());
  } else {
    if (IsScalar) {
      switch (ElementSize) {
        case 2: {
          fcmgt(Dst.H(), Vector1.H(), Vector2.H());
          break;
        }
        case 4:
        case 8:
          fcmgt(SubRegSize.Scalar, Dst, Vector1, Vector2);
          break;
        default:
          LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
          break;
      }
    } else {
      fcmgt(SubRegSize.Vector, Dst.Q(), Vector1.Q(), Vector2.Q());
    }
  }
}

DEF_OP(VFCMPLE) {
  const auto Op = IROp->C<IR::IROp_VFCMPLE>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto IsScalar = ElementSize == OpSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1.ID());
  const auto Vector2 = GetVReg(Op->Vector2.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 2 || ElementSize == 4 || ElementSize == 8, "Invalid size");
  const auto SubRegSize = ARMEmitter::ToVectorSizePair(
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i8Bit);

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Mask = PRED_TMP_32B.Zeroing();
    const auto ComparePred = ARMEmitter::PReg::p0;

    fcmge(SubRegSize.Vector, ComparePred, Mask, Vector2.Z(), Vector1.Z());
    not_(SubRegSize.Vector, VTMP1.Z(), ComparePred.Merging(), Vector2.Z());
    movprfx(SubRegSize.Vector, Dst.Z(), ComparePred.Zeroing(), Vector2.Z());
    orr(SubRegSize.Vector, Dst.Z(), ComparePred.Merging(), Dst.Z(), VTMP1.Z());
  } else {
    if (IsScalar) {
      switch (ElementSize) {
        case 2: {
          fcmge(Dst.H(), Vector2.H(), Vector1.H());
          break;
        }
        case 4:
        case 8:
          fcmge(SubRegSize.Scalar, Dst, Vector2, Vector1);
          break;
        default:
          LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
          break;
      }
    } else {
      fcmge(SubRegSize.Vector, Dst.Q(), Vector2.Q(), Vector1.Q());
    }
  }
}

DEF_OP(VFCMPORD) {
  const auto Op = IROp->C<IR::IROp_VFCMPORD>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto IsScalar = ElementSize == OpSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1.ID());
  const auto Vector2 = GetVReg(Op->Vector2.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 2 || ElementSize == 4 || ElementSize == 8, "Incorrect size");

  const auto SubRegSize = ARMEmitter::ToVectorSizePair(
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i8Bit);

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Mask = PRED_TMP_32B.Zeroing();
    const auto ComparePred = ARMEmitter::PReg::p0;

    // The idea is like comparing for unordered, but we just
    // invert the predicate from the comparison to instead
    // select all ordered elements in the vector.
    fcmuo(SubRegSize.Vector, ComparePred, Mask, Vector1.Z(), Vector2.Z());
    not_(ComparePred, Mask, ComparePred);
    not_(SubRegSize.Vector, VTMP1.Z(), ComparePred.Merging(), Vector1.Z());
    movprfx(SubRegSize.Vector, Dst.Z(), ComparePred.Zeroing(), Vector1.Z());
    orr(SubRegSize.Vector, Dst.Z(), ComparePred.Merging(), Dst.Z(), VTMP1.Z());
  } else {
    if (IsScalar) {
      switch (ElementSize) {
        case 2: {
          fcmge(VTMP1.H(), Vector1.H(), Vector2.H());
          fcmgt(VTMP2.H(), Vector2.H(), Vector1.H());
          orr(Dst.D(), VTMP1.D(), VTMP2.D());
          break;
        }
        case 4:
        case 8:
          fcmge(SubRegSize.Scalar, VTMP1, Vector1, Vector2);
          fcmgt(SubRegSize.Scalar, VTMP2, Vector2, Vector1);
          orr(Dst.D(), VTMP1.D(), VTMP2.D());
          break;
        default:
          LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
          break;
      }
    } else {
      fcmge(SubRegSize.Vector, VTMP1.Q(), Vector1.Q(), Vector2.Q());
      fcmgt(SubRegSize.Vector, VTMP2.Q(), Vector2.Q(), Vector1.Q());
      orr(Dst.Q(), VTMP1.Q(), VTMP2.Q());
    }
  }
}

DEF_OP(VFCMPUNO) {
  const auto Op = IROp->C<IR::IROp_VFCMPUNO>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto IsScalar = ElementSize == OpSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1.ID());
  const auto Vector2 = GetVReg(Op->Vector2.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 2 || ElementSize == 4 || ElementSize == 8, "Incorrect size");

  const auto SubRegSize = ARMEmitter::ToVectorSizePair(
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i8Bit);

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Mask = PRED_TMP_32B.Zeroing();
    const auto ComparePred = ARMEmitter::PReg::p0;

    fcmuo(SubRegSize.Vector, ComparePred, Mask, Vector1.Z(), Vector2.Z());
    not_(SubRegSize.Vector, VTMP1.Z(), ComparePred.Merging(), Vector1.Z());
    movprfx(SubRegSize.Vector, Dst.Z(), ComparePred.Zeroing(), Vector1.Z());
    orr(SubRegSize.Vector, Dst.Z(), ComparePred.Merging(), Dst.Z(), VTMP1.Z());
  } else {
    if (IsScalar) {
      switch (ElementSize) {
        case 2: {
          fcmge(VTMP1.H(), Vector1.H(), Vector2.H());
          fcmgt(VTMP2.H(), Vector2.H(), Vector1.H());
          orr(Dst.D(), VTMP1.D(), VTMP2.D());
          mvn(ARMEmitter::SubRegSize::i8Bit, Dst.D(), Dst.D());
          break;
        }
        case 4:
        case 8:
          fcmge(SubRegSize.Scalar, VTMP1, Vector1, Vector2);
          fcmgt(SubRegSize.Scalar, VTMP2, Vector2, Vector1);
          orr(Dst.D(), VTMP1.D(), VTMP2.D());
          mvn(ARMEmitter::SubRegSize::i8Bit, Dst.D(), Dst.D());
          break;
        default:
          LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
          break;
      }
    } else {
      fcmge(SubRegSize.Vector, VTMP1.Q(), Vector1.Q(), Vector2.Q());
      fcmgt(SubRegSize.Vector, VTMP2.Q(), Vector2.Q(), Vector1.Q());
      orr(Dst.Q(), VTMP1.Q(), VTMP2.Q());
      mvn(ARMEmitter::SubRegSize::i8Bit, Dst.Q(), Dst.Q());
    }
  }
}

DEF_OP(VUShl) {
  const auto Op = IROp->C<IR::IROp_VUShl>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = IROp->ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;
  const auto MaxShift = ElementSize * 8;

  const auto Dst = GetVReg(Node);
  const auto ShiftVector = GetVReg(Op->ShiftVector.ID());
  const auto Vector = GetVReg(Op->Vector.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 1 || ElementSize == 2 || ElementSize == 4 || ElementSize == 8, "Invalid size");
  const auto SubRegSize =
    ElementSize == 1 ? ARMEmitter::SubRegSize::i8Bit :
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i8Bit;

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Mask = PRED_TMP_32B.Merging();

    dup_imm(SubRegSize, VTMP2.Z(), MaxShift);
    umin(SubRegSize, VTMP2.Z(), Mask, VTMP2.Z(), ShiftVector.Z());

    movprfx(Dst.Z(), Vector.Z());
    lsl(SubRegSize, Dst.Z(), Mask, Dst.Z(), VTMP2.Z());
  } else {
    if (ElementSize < 8) {
      movi(SubRegSize, VTMP1.Q(), MaxShift);
      umin(SubRegSize, VTMP1.Q(), VTMP1.Q(), ShiftVector.Q());
    } else {
      LoadConstant(ARMEmitter::Size::i64Bit, TMP1, MaxShift);
      dup(SubRegSize, VTMP1.Q(), TMP1.R());

      // UMIN is silly on Adv.SIMD and doesn't have a variant that handles 64-bit elements
      cmhi(SubRegSize, VTMP2.Q(), ShiftVector.Q(), VTMP1.Q());
      bif(VTMP1.Q(), ShiftVector.Q(), VTMP2.Q());
    }

    ushl(SubRegSize, Dst.Q(), Vector.Q(), VTMP1.Q());
  }
}

DEF_OP(VUShr) {
  const auto Op = IROp->C<IR::IROp_VUShr>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = IROp->ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;
  const auto MaxShift = ElementSize * 8;

  const auto Dst = GetVReg(Node);
  const auto ShiftVector = GetVReg(Op->ShiftVector.ID());
  const auto Vector = GetVReg(Op->Vector.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 1 || ElementSize == 2 || ElementSize == 4 || ElementSize == 8, "Invalid size");
  const auto SubRegSize =
    ElementSize == 1 ? ARMEmitter::SubRegSize::i8Bit :
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i8Bit;

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Mask = PRED_TMP_32B.Merging();

    dup_imm(SubRegSize, VTMP2.Z(), MaxShift);
    umin(SubRegSize, VTMP2.Z(), Mask, VTMP2.Z(), ShiftVector.Z());

    movprfx(Dst.Z(), Vector.Z());
    lsr(SubRegSize, Dst.Z(), Mask, Dst.Z(), VTMP2.Z());
  } else {
    if (ElementSize < 8) {
      movi(SubRegSize, VTMP1.Q(), MaxShift);
      umin(SubRegSize, VTMP1.Q(), VTMP1.Q(), ShiftVector.Q());
    } else {
      LoadConstant(ARMEmitter::Size::i64Bit, TMP1, MaxShift);
      dup(SubRegSize, VTMP1.Q(), TMP1.R());

      // UMIN is silly on Adv.SIMD and doesn't have a variant that handles 64-bit elements
      cmhi(SubRegSize, VTMP2.Q(), ShiftVector.Q(), VTMP1.Q());
      bif(VTMP1.Q(), ShiftVector.Q(), VTMP2.Q());
    }

    // Need to invert shift values to perform a right shift with USHL
    // (USHR only has an immediate variant).
    neg(SubRegSize, VTMP1.Q(), VTMP1.Q());
    ushl(SubRegSize, Dst.Q(), Vector.Q(), VTMP1.Q());
  }
}

DEF_OP(VSShr) {
  const auto Op = IROp->C<IR::IROp_VSShr>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = IROp->ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;
  const auto MaxShift = (ElementSize * 8) - 1;

  const auto Dst = GetVReg(Node);
  const auto ShiftVector = GetVReg(Op->ShiftVector.ID());
  const auto Vector = GetVReg(Op->Vector.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 1 || ElementSize == 2 || ElementSize == 4 || ElementSize == 8, "Invalid size");
  const auto SubRegSize =
    ElementSize == 1 ? ARMEmitter::SubRegSize::i8Bit :
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i8Bit;

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Mask = PRED_TMP_32B.Merging();

    dup_imm(SubRegSize, VTMP1.Z(), MaxShift);
    umin(SubRegSize, VTMP1.Z(), Mask, VTMP1.Z(), ShiftVector.Z());

    movprfx(Dst.Z(), Vector.Z());
    asr(SubRegSize, Dst.Z(), Mask, Dst.Z(), VTMP1.Z());
  } else {
    if (ElementSize < 8) {
      movi(SubRegSize, VTMP1.Q(), MaxShift);
      umin(SubRegSize, VTMP1.Q(), VTMP1.Q(), ShiftVector.Q());
    } else {
      LoadConstant(ARMEmitter::Size::i64Bit, TMP1, MaxShift);
      dup(SubRegSize, VTMP1.Q(), TMP1.R());

      // UMIN is silly on Adv.SIMD and doesn't have a variant that handles 64-bit elements
      cmhi(SubRegSize, VTMP2.Q(), ShiftVector.Q(), VTMP1.Q());
      bif(VTMP1.Q(), ShiftVector.Q(), VTMP2.Q());
    }

    // Need to invert shift values to perform a right shift with SSHL
    // (SSHR only has an immediate variant).
    neg(SubRegSize, VTMP1.Q(), VTMP1.Q());
    sshl(SubRegSize, Dst.Q(), Vector.Q(), VTMP1.Q());
  }
}

DEF_OP(VUShlS) {
  const auto Op = IROp->C<IR::IROp_VUShlS>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto ShiftScalar = GetVReg(Op->ShiftScalar.ID());
  const auto Vector = GetVReg(Op->Vector.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 1 || ElementSize == 2 || ElementSize == 4 || ElementSize == 8 || ElementSize == 16, "Invalid size");
  const auto SubRegSize =
    ElementSize == 1 ? ARMEmitter::SubRegSize::i8Bit :
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i128Bit;

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Mask = PRED_TMP_32B.Merging();

    // NOTE: SVE LSL is a destructive operation.
    dup(SubRegSize, VTMP1.Z(), ShiftScalar.Z(), 0);
    movprfx(Dst.Z(), Vector.Z());
    lsl(SubRegSize, Dst.Z(), Mask, Dst.Z(), VTMP1.Z());
  } else {
    dup(SubRegSize, VTMP1.Q(), ShiftScalar.Q(), 0);
    ushl(SubRegSize, Dst.Q(), Vector.Q(), VTMP1.Q());
  }
}

DEF_OP(VUShrS) {
  const auto Op = IROp->C<IR::IROp_VUShrS>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto ShiftScalar = GetVReg(Op->ShiftScalar.ID());
  const auto Vector = GetVReg(Op->Vector.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 1 || ElementSize == 2 || ElementSize == 4 || ElementSize == 8 || ElementSize == 16, "Invalid size");
  const auto SubRegSize =
    ElementSize == 1 ? ARMEmitter::SubRegSize::i8Bit :
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i128Bit;

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Mask = PRED_TMP_32B.Merging();

    // NOTE: SVE LSR is a destructive operation.
    dup(SubRegSize, VTMP1.Z(), ShiftScalar.Z(), 0);
    movprfx(Dst.Z(), Vector.Z());
    lsr(SubRegSize, Dst.Z(), Mask, Dst.Z(), VTMP1.Z());
  } else {
    dup(SubRegSize, VTMP1.Q(), ShiftScalar.Q(), 0);
    neg(SubRegSize, VTMP1.Q(), VTMP1.Q());
    ushl(SubRegSize, Dst.Q(), Vector.Q(), VTMP1.Q());
  }
}

DEF_OP(VUShrSWide) {
  const auto Op = IROp->C<IR::IROp_VUShrSWide>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto ShiftScalar = GetVReg(Op->ShiftScalar.ID());
  const auto Vector = GetVReg(Op->Vector.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 1 || ElementSize == 2 || ElementSize == 4 || ElementSize == 8, "Invalid size");
  const auto SubRegSize =
    ElementSize == 1 ? ARMEmitter::SubRegSize::i8Bit :
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit : ARMEmitter::SubRegSize::i64Bit;

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Mask = PRED_TMP_32B.Merging();

    dup(ARMEmitter::SubRegSize::i64Bit, VTMP1.Z(), ShiftScalar.Z(), 0);
    if (Dst != Vector) {
      // NOTE: SVE LSR is a destructive operation.
      movprfx(Dst.Z(), Vector.Z());
    }
    if (ElementSize == 8) {
      lsr(SubRegSize, Dst.Z(), Mask, Dst.Z(), VTMP1.Z());
    }
    else {
      lsr_wide(SubRegSize, Dst.Z(), Mask, Dst.Z(), VTMP1.Z());
    }
  }
  else if (HostSupportsSVE128) {
    const auto Mask = PRED_TMP_16B.Merging();

    dup(ARMEmitter::SubRegSize::i64Bit, VTMP1.Z(), ShiftScalar.Z(), 0);
    if (Dst != Vector) {
      // NOTE: SVE LSR is a destructive operation.
      movprfx(Dst.Z(), Vector.Z());
    }
    if (ElementSize == 8) {
      lsr(SubRegSize, Dst.Z(), Mask, Dst.Z(), VTMP1.Z());
    }
    else {
      lsr_wide(SubRegSize, Dst.Z(), Mask, Dst.Z(), VTMP1.Z());
    }
  } else {
    // uqshl + ushr of 57-bits leaves 7-bits remaining.
    // This saturates the 64-bit shift value from an arbitrary 64-bit length
    // variable to maximum of 0x7F.
    // This allows the shift to fit within the width of the signed 8-bits
    // that ASIMD's vector shift requires.
    uqshl(ARMEmitter::ScalarRegSize::i64Bit, VTMP1, ShiftScalar, 57);
    ushr(ARMEmitter::ScalarRegSize::i64Bit, VTMP1, VTMP1, 57);
    dup(SubRegSize, VTMP1.Q(), VTMP1.Q(), 0);
    neg(SubRegSize, VTMP1.Q(), VTMP1.Q());
    ushl(SubRegSize, Dst.Q(), Vector.Q(), VTMP1.Q());
  }
}

DEF_OP(VSShrSWide) {
  const auto Op = IROp->C<IR::IROp_VSShrSWide>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto ShiftScalar = GetVReg(Op->ShiftScalar.ID());
  const auto Vector = GetVReg(Op->Vector.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 1 || ElementSize == 2 || ElementSize == 4 || ElementSize == 8, "Invalid size");
  const auto SubRegSize =
    ElementSize == 1 ? ARMEmitter::SubRegSize::i8Bit :
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit : ARMEmitter::SubRegSize::i64Bit;

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Mask = PRED_TMP_32B.Merging();

    dup(ARMEmitter::SubRegSize::i64Bit, VTMP1.Z(), ShiftScalar.Z(), 0);
    if (Dst != Vector) {
      // NOTE: SVE LSR is a destructive operation.
      movprfx(Dst.Z(), Vector.Z());
    }
    if (ElementSize == 8) {
      asr(SubRegSize, Dst.Z(), Mask, Dst.Z(), VTMP1.Z());
    }
    else {
      asr_wide(SubRegSize, Dst.Z(), Mask, Dst.Z(), VTMP1.Z());
    }
  }
  else if (HostSupportsSVE128) {
    const auto Mask = PRED_TMP_16B.Merging();

    dup(ARMEmitter::SubRegSize::i64Bit, VTMP1.Z(), ShiftScalar.Z(), 0);
    if (Dst != Vector) {
      // NOTE: SVE LSR is a destructive operation.
      movprfx(Dst.Z(), Vector.Z());
    }
    if (ElementSize == 8) {
      asr(SubRegSize, Dst.Z(), Mask, Dst.Z(), VTMP1.Z());
    }
    else {
      asr_wide(SubRegSize, Dst.Z(), Mask, Dst.Z(), VTMP1.Z());
    }
  } else {
    // uqshl + ushr of 57-bits leaves 7-bits remaining.
    // This saturates the 64-bit shift value from an arbitrary 64-bit length
    // variable to maximum of 0x7F.
    // This allows the shift to fit within the width of the signed 8-bits
    // that ASIMD's vector shift requires.
    uqshl(ARMEmitter::ScalarRegSize::i64Bit, VTMP1, ShiftScalar, 57);
    ushr(ARMEmitter::ScalarRegSize::i64Bit, VTMP1, VTMP1, 57);
    dup(SubRegSize, VTMP1.Q(), VTMP1.Q(), 0);
    neg(SubRegSize, VTMP1.Q(), VTMP1.Q());
    sshl(SubRegSize, Dst.Q(), Vector.Q(), VTMP1.Q());
  }
}

DEF_OP(VUShlSWide) {
  const auto Op = IROp->C<IR::IROp_VUShlSWide>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto ShiftScalar = GetVReg(Op->ShiftScalar.ID());
  const auto Vector = GetVReg(Op->Vector.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 1 || ElementSize == 2 || ElementSize == 4 || ElementSize == 8, "Invalid size");
  const auto SubRegSize =
    ElementSize == 1 ? ARMEmitter::SubRegSize::i8Bit :
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit : ARMEmitter::SubRegSize::i64Bit;

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Mask = PRED_TMP_32B.Merging();

    dup(ARMEmitter::SubRegSize::i64Bit, VTMP1.Z(), ShiftScalar.Z(), 0);
    if (Dst != Vector) {
      // NOTE: SVE LSR is a destructive operation.
      movprfx(Dst.Z(), Vector.Z());
    }
    if (ElementSize == 8) {
      lsl(SubRegSize, Dst.Z(), Mask, Dst.Z(), VTMP1.Z());
    }
    else {
      lsl_wide(SubRegSize, Dst.Z(), Mask, Dst.Z(), VTMP1.Z());
    }
  }
  else if (HostSupportsSVE128) {
    const auto Mask = PRED_TMP_16B.Merging();

    dup(ARMEmitter::SubRegSize::i64Bit, VTMP1.Z(), ShiftScalar.Z(), 0);
    if (Dst != Vector) {
      // NOTE: SVE LSR is a destructive operation.
      movprfx(Dst.Z(), Vector.Z());
    }
    if (ElementSize == 8) {
      lsl(SubRegSize, Dst.Z(), Mask, Dst.Z(), VTMP1.Z());
    }
    else {
      lsl_wide(SubRegSize, Dst.Z(), Mask, Dst.Z(), VTMP1.Z());
    }
  } else {
    // uqshl + ushr of 57-bits leaves 7-bits remaining.
    // This saturates the 64-bit shift value from an arbitrary 64-bit length
    // variable to maximum of 0x7F.
    // This allows the shift to fit within the width of the signed 8-bits
    // that ASIMD's vector shift requires.
    uqshl(ARMEmitter::ScalarRegSize::i64Bit, VTMP1, ShiftScalar, 57);
    ushr(ARMEmitter::ScalarRegSize::i64Bit, VTMP1, VTMP1, 57);
    dup(SubRegSize, VTMP1.Q(), VTMP1.Q(), 0);
    ushl(SubRegSize, Dst.Q(), Vector.Q(), VTMP1.Q());
  }
}

DEF_OP(VSShrS) {
  const auto Op = IROp->C<IR::IROp_VSShrS>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto ShiftScalar = GetVReg(Op->ShiftScalar.ID());
  const auto Vector = GetVReg(Op->Vector.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 1 || ElementSize == 2 || ElementSize == 4 || ElementSize == 8 || ElementSize == 16, "Invalid size");
  const auto SubRegSize =
    ElementSize == 1 ? ARMEmitter::SubRegSize::i8Bit :
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i128Bit;

   if (HostSupportsSVE256 && Is256Bit) {
    const auto Mask = PRED_TMP_32B.Merging();

    // NOTE: SVE ASR is a destructive operation.
    dup(SubRegSize, VTMP1.Z(), ShiftScalar.Z(), 0);
    movprfx(Dst.Z(), Vector.Z());
    asr(SubRegSize, Dst.Z(), Mask, Dst.Z(), VTMP1.Z());
  } else {
    dup(SubRegSize, VTMP1.Q(), ShiftScalar.Q(), 0);
    neg(SubRegSize, VTMP1.Q(), VTMP1.Q());
    sshl(SubRegSize, Dst.Q(), Vector.Q(), VTMP1.Q());
  }
}

DEF_OP(VInsElement) {
  const auto Op = IROp->C<IR::IROp_VInsElement>();
  const auto OpSize = IROp->Size;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const uint32_t ElementSize = Op->Header.ElementSize;

  const uint32_t DestIdx = Op->DestIdx;
  const uint32_t SrcIdx = Op->SrcIdx;

  const auto Dst = GetVReg(Node);
  const auto SrcVector = GetVReg(Op->SrcVector.ID());
  auto Reg = GetVReg(Op->DestVector.ID());

  if (HostSupportsSVE256 && Is256Bit) {
    LOGMAN_THROW_AA_FMT(ElementSize == 1 || ElementSize == 2 || ElementSize == 4 || ElementSize == 8 || ElementSize == 16, "Invalid size");
    const auto SubRegSize =
      ElementSize == 1 ? ARMEmitter::SubRegSize::i8Bit :
      ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
      ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
      ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i128Bit;

    // Broadcast our source value across a temporary,
    // then combine with the destination.
    dup(SubRegSize, VTMP2.Z(), SrcVector.Z(), SrcIdx);

    // We don't need to move the data unnecessarily if
    // DestVector just so happens to also be the IR op
    // destination.
    if (Dst != Reg) {
      mov(Dst.Z(), Reg.Z());
    }

    constexpr auto Predicate = ARMEmitter::PReg::p0;

    if (ElementSize == 16) {
      if (DestIdx == 0) {
        mov(ARMEmitter::SubRegSize::i8Bit, Dst.Z(), PRED_TMP_16B.Merging(), VTMP2.Z());
      } else {
        not_(Predicate, PRED_TMP_32B.Zeroing(), PRED_TMP_16B);
        mov(ARMEmitter::SubRegSize::i8Bit, Dst.Z(), Predicate.Merging(), VTMP2.Z());
      }
    } else {
      const auto UpperBound = 16 >> FEXCore::ilog2(ElementSize);
      const auto TargetElement = static_cast<int>(DestIdx) - UpperBound;
      index(SubRegSize, VTMP1.Z(), -UpperBound, 1);
      cmpeq(SubRegSize, Predicate, PRED_TMP_32B.Zeroing(), VTMP1.Z(), TargetElement);
      mov(SubRegSize, Dst.Z(), Predicate.Merging(), VTMP2.Z());
    }
  }
  else {
    LOGMAN_THROW_AA_FMT(ElementSize == 1 || ElementSize == 2 || ElementSize == 4 || ElementSize == 8, "Invalid size");
    const auto SubRegSize =
      ElementSize == 1 ? ARMEmitter::SubRegSize::i8Bit :
      ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
      ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
      ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i8Bit;

    if (Dst.Idx() != Reg.Idx()) {
      mov(VTMP1.Q(), Reg.Q());
      Reg = VTMP1;
    }

    ins(SubRegSize, Reg.Q(), DestIdx, SrcVector.Q(), SrcIdx);

    if (Dst.Idx() != Reg.Idx()) {
      mov(Dst.Q(), Reg.Q());
    }
  }
}

DEF_OP(VDupElement) {
  const auto Op = IROp->C<IR::IROp_VDupElement>();
  const auto OpSize = IROp->Size;

  const auto Index = Op->Index;
  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto Vector = GetVReg(Op->Vector.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 1 || ElementSize == 2 || ElementSize == 4 || ElementSize == 8 || ElementSize == 16, "Invalid size");
  const auto SubRegSize =
    ElementSize == 1 ? ARMEmitter::SubRegSize::i8Bit :
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i128Bit;

  if (HostSupportsSVE256 && Is256Bit) {
    dup(SubRegSize, Dst.Z(), Vector.Z(), Index);
  } else {
    dup(SubRegSize, Dst.Q(), Vector.Q(), Index);
  }
}

DEF_OP(VExtr) {
  const auto Op = IROp->C<IR::IROp_VExtr>();
  const auto OpSize = IROp->Size;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  // AArch64 ext op has bit arrangement as [Vm:Vn] so arguments need to be swapped
  const auto Dst = GetVReg(Node);
  auto UpperBits = GetVReg(Op->VectorLower.ID());
  auto LowerBits = GetVReg(Op->VectorUpper.ID());

  const auto ElementSize = Op->Header.ElementSize;
  auto Index = Op->Index;

  if (Index >= OpSize) {
    // Upper bits have moved in to the lower bits
    LowerBits = UpperBits;

    // Upper bits are all now zero
    UpperBits = VTMP1;
    movi(ARMEmitter::SubRegSize::i64Bit, VTMP1.Q(), 0);
    Index -= OpSize;
  }

  const auto CopyFromByte = Index * ElementSize;

  if (HostSupportsSVE256 && Is256Bit) {
    movprfx(VTMP2.Z(), LowerBits.Z());
    ext<FEXCore::ARMEmitter::OpType::Destructive>(VTMP2.Z(), VTMP2.Z(), UpperBits.Z(), CopyFromByte);
    mov(Dst.Z(), VTMP2.Z());
  } else {
    if (OpSize == 8) {
      ext(Dst.D(), LowerBits.D(), UpperBits.D(), CopyFromByte);
    } else {
      ext(Dst.Q(), LowerBits.Q(), UpperBits.Q(), CopyFromByte);
    }
  }
}

DEF_OP(VUShrI) {
  const auto Op = IROp->C<IR::IROp_VUShrI>();
  const auto OpSize = IROp->Size;

  const auto BitShift = Op->BitShift;
  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto Vector = GetVReg(Op->Vector.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 1 || ElementSize == 2 || ElementSize == 4 || ElementSize == 8, "Invalid size");
  const auto SubRegSize =
    ElementSize == 1 ? ARMEmitter::SubRegSize::i8Bit :
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i8Bit;

  if (BitShift >= (ElementSize * 8)) {
    movi(ARMEmitter::SubRegSize::i64Bit, Dst.Q(), 0);
  } else {
    if (HostSupportsSVE256 && Is256Bit) {
      const auto Mask = PRED_TMP_32B.Merging();

      if (BitShift == 0) {
        if (Dst.Idx() != Vector.Idx()) {
          mov(Dst.Z(), Vector.Z());
        }
      }
      else {
        // SVE LSR is destructive, so lets set up the destination.
        movprfx(Dst.Z(), Vector.Z());
        lsr(SubRegSize, Dst.Z(), Mask, Dst.Z(), BitShift);
      }
    } else {
      if (BitShift == 0) {
        if (Dst.Idx() != Vector.Idx()) {
          mov(Dst.Q(), Vector.Q());
        }
      }
      else {
        ushr(SubRegSize, Dst.Q(), Vector.Q(), BitShift);
      }
    }
  }
}

DEF_OP(VSShrI) {
  const auto Op = IROp->C<IR::IROp_VSShrI>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Shift = std::min(uint8_t(ElementSize * 8 - 1), Op->BitShift);
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto Vector = GetVReg(Op->Vector.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 1 || ElementSize == 2 || ElementSize == 4 || ElementSize == 8, "Invalid size");
  const auto SubRegSize =
    ElementSize == 1 ? ARMEmitter::SubRegSize::i8Bit :
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i8Bit;

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Mask = PRED_TMP_32B.Merging();

    if (Shift == 0) {
      if (Dst.Idx() != Vector.Idx()) {
        mov(Dst.Z(), Vector.Z());
      }
    }
    else {
      // SVE ASR is destructive, so lets set up the destination.
      movprfx(Dst.Z(), Vector.Z());
      asr(SubRegSize, Dst.Z(), Mask, Dst.Z(), Shift);
    }
  } else {
    if (Shift == 0) {
      if (Dst.Idx() != Vector.Idx()) {
        mov(Dst.Q(), Vector.Q());
      }
    }
    else {
      sshr(SubRegSize, Dst.Q(), Vector.Q(), Shift);
    }
  }
}

DEF_OP(VShlI) {
  const auto Op = IROp->C<IR::IROp_VShlI>();
  const auto OpSize = IROp->Size;

  const auto BitShift = Op->BitShift;
  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto Vector = GetVReg(Op->Vector.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 1 || ElementSize == 2 || ElementSize == 4 || ElementSize == 8, "Invalid size");
  const auto SubRegSize =
    ElementSize == 1 ? ARMEmitter::SubRegSize::i8Bit :
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i8Bit;


  if (BitShift >= (ElementSize * 8)) {
    movi(ARMEmitter::SubRegSize::i64Bit, Dst.Q(), 0);
  } else {
    if (HostSupportsSVE256 && Is256Bit) {
      const auto Mask = PRED_TMP_32B.Merging();

      if (BitShift == 0) {
        if (Dst.Idx() != Vector.Idx()) {
          mov(Dst.Z(), Vector.Z());
        }
      }
      else {
        // SVE LSL is destructive, so lets set up the destination.
        movprfx(Dst.Z(), Vector.Z());
        lsl(SubRegSize, Dst.Z(), Mask, Dst.Z(), BitShift);
      }
    } else {
      if (BitShift == 0) {
        if (Dst.Idx() != Vector.Idx()) {
          mov(Dst.Q(), Vector.Q());
        }
      }
      else {
        shl(SubRegSize, Dst.Q(), Vector.Q(), BitShift);
      }
    }
  }
}

DEF_OP(VUShrNI) {
  const auto Op = IROp->C<IR::IROp_VUShrNI>();
  const auto OpSize = IROp->Size;

  const auto BitShift = Op->BitShift;
  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto Vector = GetVReg(Op->Vector.ID());
  LOGMAN_THROW_AA_FMT(ElementSize == 1 || ElementSize == 2 || ElementSize == 4, "Incorrect size");

  const auto SubRegSize =
    ElementSize == 1 ? ARMEmitter::SubRegSize::i8Bit :
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit : ARMEmitter::SubRegSize::i8Bit;

  if (HostSupportsSVE256 && Is256Bit) {
    shrnb(SubRegSize, Dst.Z(), Vector.Z(), BitShift);
    uzp1(SubRegSize, Dst.Z(), Dst.Z(), Dst.Z());
  } else {
    shrn(SubRegSize, Dst.D(), Vector.D(), BitShift);
  }
}

DEF_OP(VUShrNI2) {
  const auto Op = IROp->C<IR::IROp_VUShrNI2>();
  const auto OpSize = IROp->Size;

  const auto BitShift = Op->BitShift;
  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto VectorLower = GetVReg(Op->VectorLower.ID());
  const auto VectorUpper = GetVReg(Op->VectorUpper.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 1 || ElementSize == 2 || ElementSize == 4 || ElementSize == 8, "Invalid size");
  const auto SubRegSize =
    ElementSize == 1 ? ARMEmitter::SubRegSize::i8Bit :
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit : ARMEmitter::SubRegSize::i8Bit;

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Mask = PRED_TMP_16B;

    shrnb(SubRegSize, VTMP2.Z(), VectorUpper.Z(), BitShift);
    uzp1(SubRegSize, VTMP2.Z(), VTMP2.Z(), VTMP2.Z());

    movprfx(Dst.Z(), VectorLower.Z());
    splice<ARMEmitter::OpType::Destructive>(SubRegSize, Dst.Z(), Mask, Dst.Z(), VTMP2.Z());
  } else {
    mov(VTMP1.Q(), VectorLower.Q());
    shrn2(SubRegSize, VTMP1.Q(), VectorUpper.Q(), BitShift);
    mov(Dst.Q(), VTMP1.Q());
  }
}

DEF_OP(VSXTL) {
  const auto Op = IROp->C<IR::IROp_VSXTL>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto Vector = GetVReg(Op->Vector.ID());
  LOGMAN_THROW_AA_FMT(ElementSize == 2 || ElementSize == 4 || ElementSize == 8, "Incorrect size");

  const auto SubRegSize =
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i8Bit;

  if ((HostSupportsSVE128 && !Is256Bit && !HostSupportsSVE256) ||
      (HostSupportsSVE256 && Is256Bit)) {
    sunpklo(SubRegSize, Dst.Z(), Vector.Z());
  } else {
    sxtl(SubRegSize, Dst.D(), Vector.D());
  }
}

DEF_OP(VSXTL2) {
  const auto Op = IROp->C<IR::IROp_VSXTL2>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto Vector = GetVReg(Op->Vector.ID());
  LOGMAN_THROW_AA_FMT(ElementSize == 2 || ElementSize == 4 || ElementSize == 8, "Incorrect size");

  const auto SubRegSize =
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i8Bit;

  if ((HostSupportsSVE128 && !Is256Bit && !HostSupportsSVE256) ||
      (HostSupportsSVE256 && Is256Bit)) {
    sunpkhi(SubRegSize, Dst.Z(), Vector.Z());
  } else {
    sxtl2(SubRegSize, Dst.Q(), Vector.Q());
  }
}

DEF_OP(VUXTL) {
  const auto Op = IROp->C<IR::IROp_VUXTL>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto Vector = GetVReg(Op->Vector.ID());
  LOGMAN_THROW_AA_FMT(ElementSize == 2 || ElementSize == 4 || ElementSize == 8, "Incorrect size");

  const auto SubRegSize =
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i8Bit;

  if ((HostSupportsSVE128 && !Is256Bit && !HostSupportsSVE256) ||
      (HostSupportsSVE256 && Is256Bit)) {
    uunpklo(SubRegSize, Dst.Z(), Vector.Z());
  } else {
    uxtl(SubRegSize, Dst.D(), Vector.D());
  }
}

DEF_OP(VUXTL2) {
  const auto Op = IROp->C<IR::IROp_VUXTL2>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto Vector = GetVReg(Op->Vector.ID());
  LOGMAN_THROW_AA_FMT(ElementSize == 2 || ElementSize == 4 || ElementSize == 8, "Incorrect size");

  const auto SubRegSize =
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i8Bit;

  if ((HostSupportsSVE128 && !Is256Bit && !HostSupportsSVE256) ||
      (HostSupportsSVE256 && Is256Bit)) {
    uunpkhi(SubRegSize, Dst.Z(), Vector.Z());
  } else {
    uxtl2(SubRegSize, Dst.Q(), Vector.Q());
  }
}

DEF_OP(VSQXTN) {
  const auto Op = IROp->C<IR::IROp_VSQXTN>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto Vector = GetVReg(Op->Vector.ID());
  LOGMAN_THROW_AA_FMT(ElementSize == 1 || ElementSize == 2 || ElementSize == 4, "Incorrect size");

  const auto SubRegSize =
    ElementSize == 1 ? ARMEmitter::SubRegSize::i8Bit :
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit : ARMEmitter::SubRegSize::i8Bit;

  if (HostSupportsSVE256 && Is256Bit) {
    // Note that SVE SQXTNB and SQXTNT are a tad different
    // in behavior compared to most other [name]B and [name]T
    // instructions.
    //
    // Most other bottom and top instructions operate
    // on even (bottom) or odd (top) elements and store each
    // result into the next subsequent element in the destination
    // vector
    //
    // SQXTNB and SQXTNT will operate on the same elements regardless
    // of which one is chosen, but will instead place results from
    // the operation into either each subsequent even (bottom) element
    // or odd (top) element. However the bottom instruction will zero the
    // odd elements out in the destination vector, while the top instruction
    // will leave the even elements alone (in a behavior similar to Adv.SIMD's
    // SQXTN/SQXTN2 instructions).
    //
    // e.g. consider this 64-bit (for brevity) vector with four 16-bit elements:
    //
    // ╔═══════════╗╔═══════════╗╔═══════════╗╔═══════════╗
    // ║  Value 3  ║║  Value 2  ║║  Value 1  ║║  Value 0  ║
    // ╚═══════════╝╚═══════════╝╚═══════════╝╚═══════════╝
    //
    // SQXTNB Dst.VnB, Src.VnH will result in:
    //
    // ╔═════╗╔═════╗╔═════╗╔═════╗╔═════╗╔═════╗╔═════╗╔═════╗
    // ║  0  ║║ V3  ║║  0  ║║ V2  ║║  0  ║║ V1  ║║  0  ║║ V0  ║
    // ╚═════╝╚═════╝╚═════╝╚═════╝╚═════╝╚═════╝╚═════╝╚═════╝
    //
    // This is kind of convenient, considering we only need
    // to use the bottom variant and then concatenate all the
    // even elements with SVE UZP1.

    sqxtnb(SubRegSize, Dst.Z(), Vector.Z());
    uzp1(SubRegSize, Dst.Z(), Dst.Z(), Dst.Z());
  } else {
    sqxtn(SubRegSize, Dst, Vector);
  }
}

DEF_OP(VSQXTN2) {
  const auto Op = IROp->C<IR::IROp_VSQXTN2>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto VectorLower = GetVReg(Op->VectorLower.ID());
  const auto VectorUpper = GetVReg(Op->VectorUpper.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 1 || ElementSize == 2 || ElementSize == 4, "Invalid size");
  const auto SubRegSize =
    ElementSize == 1 ? ARMEmitter::SubRegSize::i8Bit :
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit : ARMEmitter::SubRegSize::i8Bit;

  if (HostSupportsSVE256 && Is256Bit) {
    // We use the 16 byte mask due to how SPLICE works. We only
    // want to get at the first 16 bytes in the lower vector, so
    // that SPLICE will then begin copying the first 16 bytes
    // from the upper vector and begin placing them after the
    // previously copied lower 16 bytes.
    const auto Mask = PRED_TMP_16B;

    sqxtnb(SubRegSize, VTMP2.Z(), VectorUpper.Z());
    uzp1(SubRegSize, VTMP2.Z(), VTMP2.Z(), VTMP2.Z());

    // Need to use the destructive variant of SPLICE, since
    // the constructive variant requires a register list, and
    // we can't guarantee VectorLower and VectorUpper will always
    // have consecutive indexes with one another.
    movprfx(Dst.Z(), VectorLower.Z());
    splice<ARMEmitter::OpType::Destructive>(SubRegSize, Dst.Z(), Mask, Dst.Z(), VTMP2.Z());
  } else {
    if (OpSize == 8) {
      sqxtn(SubRegSize, VTMP2, VectorUpper);
      mov(Dst.Q(), VectorLower.Q());
      ins(ARMEmitter::SubRegSize::i32Bit, Dst, 1, VTMP2, 0);
    } else {
      mov(VTMP1.Q(), VectorLower.Q());
      sqxtn2(SubRegSize, VTMP1, VectorUpper);
      mov(Dst.Q(), VTMP1.Q());
    }
  }
}

DEF_OP(VSQXTUN) {
  const auto Op = IROp->C<IR::IROp_VSQXTUN>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto Vector = GetVReg(Op->Vector.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 1 || ElementSize == 2 || ElementSize == 4 || ElementSize == 8, "Invalid size");
  const auto SubRegSize =
    ElementSize == 1 ? ARMEmitter::SubRegSize::i8Bit :
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit : ARMEmitter::SubRegSize::i8Bit;

  if (HostSupportsSVE256 && Is256Bit) {
    sqxtunb(SubRegSize, Dst.Z(), Vector.Z());
    uzp1(SubRegSize, Dst.Z(), Dst.Z(), Dst.Z());
  } else {
    sqxtun(SubRegSize, Dst, Vector);
  }
}

DEF_OP(VSQXTUN2) {
  const auto Op = IROp->C<IR::IROp_VSQXTUN2>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto VectorLower = GetVReg(Op->VectorLower.ID());
  const auto VectorUpper = GetVReg(Op->VectorUpper.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 1 || ElementSize == 2 || ElementSize == 4, "Invalid size");
  const auto SubRegSize =
    ElementSize == 1 ? ARMEmitter::SubRegSize::i8Bit :
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit : ARMEmitter::SubRegSize::i8Bit;

  if (HostSupportsSVE256 && Is256Bit) {
    // NOTE: See VSQXTN2 implementation for an in-depth explanation
    //       of everything going on here.

    const auto Mask = PRED_TMP_16B;

    sqxtunb(SubRegSize, VTMP2.Z(), VectorUpper.Z());
    uzp1(SubRegSize, VTMP2.Z(), VTMP2.Z(), VTMP2.Z());

    movprfx(Dst.Z(), VectorLower.Z());
    splice<ARMEmitter::OpType::Destructive>(SubRegSize, Dst.Z(), Mask, Dst.Z(), VTMP2.Z());
  } else {
    if (OpSize == 8) {
      sqxtun(SubRegSize, VTMP2, VectorUpper);
      mov(Dst.Q(), VectorLower.Q());
      ins(ARMEmitter::SubRegSize::i32Bit, Dst, 1, VTMP2, 0);
    } else {
      mov(VTMP1.Q(), VectorLower.Q());
      sqxtun2(SubRegSize, VTMP1, VectorUpper);
      mov(Dst.Q(), VTMP1.Q());
    }
  }
}

DEF_OP(VSRSHR) {
  const auto Op = IROp->C<IR::IROp_VSRSHR>();
  const auto OpSize = IROp->Size;

  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;
  const auto ElementSize = Op->Header.ElementSize;

  const auto Dst = GetVReg(Node);
  const auto Vector = GetVReg(Op->Vector.ID());
  const auto BitShift = Op->BitShift;

  LOGMAN_THROW_AA_FMT(ElementSize == 1 || ElementSize == 2 || ElementSize == 4 || ElementSize == 8, "Invalid size");
  const auto SubRegSize =
    ElementSize == 1 ? ARMEmitter::SubRegSize::i8Bit :
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i8Bit;

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Mask = PRED_TMP_32B.Merging();
    // SVE SRSHR is destructive, so lets set up the destination.
    movprfx(Dst.Z(), Vector.Z());
    srshr(SubRegSize, Dst.Z(), Mask, Vector.Z(), BitShift);
  } else {
    if (OpSize == 8) {
      srshr(SubRegSize, Dst.D(), Vector.D(), BitShift);
    }
    else {
      srshr(SubRegSize, Dst.Q(), Vector.Q(), BitShift);
    }
  }
}

DEF_OP(VSQSHL) {
  const auto Op = IROp->C<IR::IROp_VSQSHL>();
  const auto OpSize = IROp->Size;

  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;
  const auto ElementSize = Op->Header.ElementSize;

  const auto Dst = GetVReg(Node);
  const auto Vector = GetVReg(Op->Vector.ID());
  const auto BitShift = Op->BitShift;

  LOGMAN_THROW_AA_FMT(ElementSize == 1 || ElementSize == 2 || ElementSize == 4 || ElementSize == 8, "Invalid size");
  const auto SubRegSize =
    ElementSize == 1 ? ARMEmitter::SubRegSize::i8Bit :
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i8Bit;

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Mask = PRED_TMP_32B.Merging();
    // SVE SQSHL is destructive, so lets set up the destination.
    movprfx(Dst.Z(), Vector.Z());
    sqshl(SubRegSize, Dst.Z(), Mask, Vector.Z(), BitShift);
  } else {
    if (OpSize == 8) {
      sqshl(SubRegSize, Dst.D(), Vector.D(), BitShift);
    }
    else {
      sqshl(SubRegSize, Dst.Q(), Vector.Q(), BitShift);
    }
  }
}

DEF_OP(VMul) {
  const auto Op = IROp->C<IR::IROp_VUMul>();
  const auto OpSize = IROp->Size;

  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;
  const auto ElementSize = Op->Header.ElementSize;

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1.ID());
  const auto Vector2 = GetVReg(Op->Vector2.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 1 || ElementSize == 2 || ElementSize == 4 || ElementSize == 8, "Invalid size");
  const auto SubRegSize =
    ElementSize == 1 ? ARMEmitter::SubRegSize::i8Bit :
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i8Bit;

  if (HostSupportsSVE256 && Is256Bit) {
    mul(SubRegSize, Dst.Z(), Vector1.Z(), Vector2.Z());
  } else {
    mul(SubRegSize, Dst.Q(), Vector1.Q(), Vector2.Q());
  }
}

DEF_OP(VUMull) {
  const auto Op = IROp->C<IR::IROp_VUMull>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1.ID());
  const auto Vector2 = GetVReg(Op->Vector2.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 2 || ElementSize == 4 || ElementSize == 8, "Invalid size");
  const auto SubRegSize =
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i8Bit;

  if (HostSupportsSVE256 && Is256Bit) {
    umullb(SubRegSize, VTMP1.Z(), Vector1.Z(), Vector2.Z());
    umullt(SubRegSize, VTMP2.Z(), Vector1.Z(), Vector2.Z());
    zip1(SubRegSize, Dst.Z(), VTMP1.Z(), VTMP2.Z());
  } else {
    umull(SubRegSize, Dst.D(), Vector1.D(), Vector2.D());
  }
}

DEF_OP(VSMull) {
  const auto Op = IROp->C<IR::IROp_VSMull>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1.ID());
  const auto Vector2 = GetVReg(Op->Vector2.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 2 || ElementSize == 4 || ElementSize == 8, "Invalid size");
  const auto SubRegSize =
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i8Bit;

  if (HostSupportsSVE256 && Is256Bit) {
    smullb(SubRegSize, VTMP1.Z(), Vector1.Z(), Vector2.Z());
    smullt(SubRegSize, VTMP2.Z(), Vector1.Z(), Vector2.Z());
    zip1(SubRegSize, Dst.Z(), VTMP1.Z(), VTMP2.Z());
  } else {
    smull(SubRegSize, Dst.D(), Vector1.D(), Vector2.D());
  }
}

DEF_OP(VUMull2) {
  const auto Op = IROp->C<IR::IROp_VUMull2>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1.ID());
  const auto Vector2 = GetVReg(Op->Vector2.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 2 || ElementSize == 4 || ElementSize == 8, "Invalid size");
  const auto SubRegSize =
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i8Bit;

  if (HostSupportsSVE256 && Is256Bit) {
    umullb(SubRegSize, VTMP1.Z(), Vector1.Z(), Vector2.Z());
    umullt(SubRegSize, VTMP2.Z(), Vector1.Z(), Vector2.Z());
    zip2(SubRegSize, Dst.Z(), VTMP1.Z(), VTMP2.Z());
  } else {
    umull2(SubRegSize, Dst.Q(), Vector1.Q(), Vector2.Q());
  }
}

DEF_OP(VSMull2) {
  const auto Op = IROp->C<IR::IROp_VSMull2>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1.ID());
  const auto Vector2 = GetVReg(Op->Vector2.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 2 || ElementSize == 4 || ElementSize == 8, "Invalid size");
  const auto SubRegSize =
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i8Bit;

  if (HostSupportsSVE256 && Is256Bit) {
    smullb(SubRegSize, VTMP1.Z(), Vector1.Z(), Vector2.Z());
    smullt(SubRegSize, VTMP2.Z(), Vector1.Z(), Vector2.Z());
    zip2(SubRegSize, Dst.Z(), VTMP1.Z(), VTMP2.Z());
  } else {
    smull2(SubRegSize, Dst.Q(), Vector1.Q(), Vector2.Q());
  }
}

DEF_OP(VUABDL) {
  const auto Op = IROp->C<IR::IROp_VUABDL>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1.ID());
  const auto Vector2 = GetVReg(Op->Vector2.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 2 || ElementSize == 4 || ElementSize == 8, "Invalid size");
  const auto SubRegSize =
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i8Bit;

  if (HostSupportsSVE256 && Is256Bit) {
    // To mimic the behavior of AdvSIMD UABDL, we need to get the
    // absolute difference of the even elements (UADBLB), get the
    // absolute difference of the odd elemenets (UABDLT), then
    // interleave the results in both vectors together.

    uabdlb(SubRegSize, VTMP1.Z(), Vector1.Z(), Vector2.Z());
    uabdlt(SubRegSize, VTMP2.Z(), Vector1.Z(), Vector2.Z());
    zip1(SubRegSize, Dst.Z(), VTMP1.Z(), VTMP2.Z());
  } else {
    uabdl(SubRegSize, Dst.D(), Vector1.D(), Vector2.D());
  }
}

DEF_OP(VUABDL2) {
  const auto Op = IROp->C<IR::IROp_VUABDL2>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1.ID());
  const auto Vector2 = GetVReg(Op->Vector2.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 2 || ElementSize == 4 || ElementSize == 8, "Invalid size");
  const auto SubRegSize =
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    ElementSize == 8 ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i8Bit;

  if (HostSupportsSVE256 && Is256Bit) {
    // To mimic the behavior of AdvSIMD UABDL, we need to get the
    // absolute difference of the even elements (UADBLB), get the
    // absolute difference of the odd elemenets (UABDLT), then
    // interleave the results in both vectors together.

    uabdlb(SubRegSize, VTMP1.Z(), Vector1.Z(), Vector2.Z());
    uabdlt(SubRegSize, VTMP2.Z(), Vector1.Z(), Vector2.Z());
    zip2(SubRegSize, Dst.Z(), VTMP1.Z(), VTMP2.Z());
  } else {
    uabdl2(SubRegSize, Dst.Q(), Vector1.Q(), Vector2.Q());
  }
}

DEF_OP(VTBL1) {
  const auto Op = IROp->C<IR::IROp_VTBL1>();
  const auto OpSize = IROp->Size;

  const auto Dst = GetVReg(Node);
  const auto VectorIndices = GetVReg(Op->VectorIndices.ID());
  const auto VectorTable = GetVReg(Op->VectorTable.ID());

  switch (OpSize) {
    case 8: {
      tbl(Dst.D(), VectorTable.Q(), VectorIndices.D());
      break;
    }
    case 16: {
      tbl(Dst.Q(), VectorTable.Q(), VectorIndices.Q());
      break;
    }
    case 32: {
      LOGMAN_THROW_AA_FMT(HostSupportsSVE256,
                          "Host does not support SVE. Cannot perform 256-bit table lookup");

      tbl(ARMEmitter::SubRegSize::i8Bit, Dst.Z(), VectorTable.Z(), VectorIndices.Z());
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unknown OpSize: {}", OpSize);
      break;
  }
}

DEF_OP(VRev32) {
  const auto Op = IROp->C<IR::IROp_VRev32>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto Vector = GetVReg(Op->Vector.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 1 || ElementSize == 2, "Invalid size");
  const auto SubRegSize =
    ElementSize == 1 ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i16Bit;

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Mask = PRED_TMP_32B.Merging();

    switch (ElementSize) {
      case 1: {
        revb(ARMEmitter::SubRegSize::i32Bit, Dst.Z(), Mask, Vector.Z());
        break;
      }
      case 2: {
        revh(ARMEmitter::SubRegSize::i32Bit, Dst.Z(), Mask, Vector.Z());
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Invalid Element Size: {}", ElementSize);
        break;
    }
  } else {
    if (OpSize == 8) {
      rev32(SubRegSize, Dst.D(), Vector.D());
    }
    else {
      rev32(SubRegSize, Dst.Q(), Vector.Q());
    }
  }
}


DEF_OP(VRev64) {
  const auto Op = IROp->C<IR::IROp_VRev64>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetVReg(Node);
  const auto Vector = GetVReg(Op->Vector.ID());

  LOGMAN_THROW_AA_FMT(ElementSize == 1 || ElementSize == 2 || ElementSize == 4, "Invalid size");
  const auto SubRegSize =
    ElementSize == 1 ? ARMEmitter::SubRegSize::i8Bit :
    ElementSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    ElementSize == 4 ? ARMEmitter::SubRegSize::i32Bit : ARMEmitter::SubRegSize::i8Bit;

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Mask = PRED_TMP_32B.Merging();

    switch (ElementSize) {
      case 1: {
        revb(ARMEmitter::SubRegSize::i64Bit, Dst.Z(), Mask, Vector.Z());
        break;
      }
      case 2: {
        revh(ARMEmitter::SubRegSize::i64Bit, Dst.Z(), Mask, Vector.Z());
        break;
      }
      case 4: {
        revw(ARMEmitter::SubRegSize::i64Bit, Dst.Z(), Mask, Vector.Z());
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Invalid Element Size: {}", ElementSize);
        break;
    }
  } else {
    if (OpSize == 8) {
      rev64(SubRegSize, Dst.D(), Vector.D());
    }
    else {
      rev64(SubRegSize, Dst.Q(), Vector.Q());
    }
  }
}

#undef DEF_OP
}

