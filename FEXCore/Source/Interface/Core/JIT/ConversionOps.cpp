// SPDX-License-Identifier: MIT
/*
$info$
tags: backend|arm64
$end_info$
*/

#include "Interface/Core/JIT/JITClass.h"

namespace FEXCore::CPU {
#define DEF_OP(x) void Arm64JITCore::Op_##x(IR::IROp_Header const* IROp, IR::NodeID Node)
DEF_OP(VInsGPR) {
  const auto Op = IROp->C<IR::IROp_VInsGPR>();
  const auto OpSize = IROp->Size;

  const auto DestIdx = Op->DestIdx;
  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;
  LOGMAN_THROW_A_FMT(!Is256Bit || (Is256Bit && HostSupportsSVE256), "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto SubEmitSize = ConvertSubRegSize8(IROp);
  const auto ElementsPer128Bit = 16 / ElementSize;

  const auto Dst = GetVReg(Node);
  const auto DestVector = GetVReg(Op->DestVector.ID());
  const auto Src = GetReg(Op->Src.ID());

  if (HostSupportsSVE256 && Is256Bit) {
    const auto ElementSizeBits = ElementSize * 8;
    const auto Offset = ElementSizeBits * DestIdx;

    const auto SSEBitSize = Core::CPUState::XMM_SSE_REG_SIZE * 8;
    const auto InUpperLane = Offset >= SSEBitSize;

    // This is going to be a little gross. Pls forgive me.
    // Since SVE has the whole vector length agnostic programming
    // thing going on, we can't exactly freely insert entries into
    // arbitrary locations in the vector.
    //
    // SVE *does* have INSR, however this only shifts the entire
    // vector to the left by an element size and inserts a value
    // at the beginning of the vector. Not *quite* what we need.
    // (though INSR *is* very useful for other things).
    //
    // The idea is (in the case of the upper lane), move the upper
    // lane down, insert into it and recombine with the lower lane.
    //
    // In the case of the lower lane, insert and then recombine with
    // the upper lane.

    if (InUpperLane) {
      // Move the upper lane down for the insertion.
      const auto CompactPred = ARMEmitter::PReg::p0;
      not_(CompactPred, PRED_TMP_32B.Zeroing(), PRED_TMP_16B);
      compact(ARMEmitter::SubRegSize::i64Bit, VTMP1.Z(), CompactPred, DestVector.Z());
    }

    // Put data in place for destructive SPLICE below.
    mov(Dst.Z(), DestVector.Z());

    // Inserts the GPR value into the given V register.
    // Also automatically adjusts the index in the case of using the
    // moved upper lane.
    const auto Insert = [&](const ARMEmitter::VRegister& reg, int index) {
      if (InUpperLane) {
        index -= ElementsPer128Bit;
      }
      ins(SubEmitSize, reg, index, Src);
    };

    if (InUpperLane) {
      Insert(VTMP1, DestIdx);
      splice<ARMEmitter::OpType::Destructive>(ARMEmitter::SubRegSize::i64Bit, Dst.Z(), PRED_TMP_16B, Dst.Z(), VTMP1.Z());
    } else {
      Insert(Dst, DestIdx);
      splice<ARMEmitter::OpType::Destructive>(ARMEmitter::SubRegSize::i64Bit, Dst.Z(), PRED_TMP_16B, Dst.Z(), DestVector.Z());
    }
  } else {
    // No need to move if Dst and DestVector alias one another.
    if (Dst != DestVector) {
      mov(Dst.Q(), DestVector.Q());
    }
    ins(SubEmitSize, Dst, DestIdx, Src);
  }
}

DEF_OP(VCastFromGPR) {
  auto Op = IROp->C<IR::IROp_VCastFromGPR>();
  auto Dst = GetVReg(Node);
  auto Src = GetReg(Op->Src.ID());

  switch (Op->Header.ElementSize) {
  case 1:
    uxtb(ARMEmitter::Size::i32Bit, TMP1, Src);
    fmov(ARMEmitter::Size::i32Bit, Dst.S(), TMP1);
    break;
  case 2:
    uxth(ARMEmitter::Size::i32Bit, TMP1, Src);
    fmov(ARMEmitter::Size::i32Bit, Dst.S(), TMP1);
    break;
  case 4: fmov(ARMEmitter::Size::i32Bit, Dst.S(), Src); break;
  case 8: fmov(ARMEmitter::Size::i64Bit, Dst.D(), Src); break;
  default: LOGMAN_MSG_A_FMT("Unknown castGPR element size: {}", Op->Header.ElementSize);
  }
}

DEF_OP(VDupFromGPR) {
  const auto Op = IROp->C<IR::IROp_VDupFromGPR>();
  const auto OpSize = IROp->Size;

  const auto Dst = GetVReg(Node);
  const auto Src = GetReg(Op->Src.ID());

  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;
  LOGMAN_THROW_A_FMT(!Is256Bit || (Is256Bit && HostSupportsSVE256), "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto SubEmitSize = ConvertSubRegSize8(IROp);

  if (HostSupportsSVE256 && Is256Bit) {
    dup(SubEmitSize, Dst.Z(), Src);
  } else {
    dup(SubEmitSize, Dst.Q(), Src);
  }
}

DEF_OP(Float_FromGPR_S) {
  const auto Op = IROp->C<IR::IROp_Float_FromGPR_S>();

  const uint16_t ElementSize = Op->Header.ElementSize;
  const uint16_t Conv = (ElementSize << 8) | Op->SrcElementSize;

  auto Dst = GetVReg(Node);
  auto Src = GetReg(Op->Src.ID());

  switch (Conv) {
  case 0x0204: { // Half <- int32_t
    scvtf(ARMEmitter::Size::i32Bit, Dst.H(), Src);
    break;
  }
  case 0x0208: { // Half <- int64_t
    scvtf(ARMEmitter::Size::i64Bit, Dst.H(), Src);
    break;
  }
  case 0x0404: { // Float <- int32_t
    scvtf(ARMEmitter::Size::i32Bit, Dst.S(), Src);
    break;
  }
  case 0x0408: { // Float <- int64_t
    scvtf(ARMEmitter::Size::i64Bit, Dst.S(), Src);
    break;
  }
  case 0x0804: { // Double <- int32_t
    scvtf(ARMEmitter::Size::i32Bit, Dst.D(), Src);
    break;
  }
  case 0x0808: { // Double <- int64_t
    scvtf(ARMEmitter::Size::i64Bit, Dst.D(), Src);
    break;
  }
  default:
    LOGMAN_MSG_A_FMT("Unhandled conversion mask: Mask=0x{:04x}, ElementSize={}, SrcElementSize={}", Conv, ElementSize, Op->SrcElementSize);
    break;
  }
}

DEF_OP(Float_FToF) {
  auto Op = IROp->C<IR::IROp_Float_FToF>();
  const uint16_t Conv = (Op->Header.ElementSize << 8) | Op->SrcElementSize;

  auto Dst = GetVReg(Node);
  auto Src = GetVReg(Op->Scalar.ID());

  switch (Conv) {
  case 0x0204: { // Half <- Float
    fcvt(Dst.H(), Src.S());
    break;
  }
  case 0x0208: { // Half <- Double
    fcvt(Dst.H(), Src.D());
    break;
  }
  case 0x0402: { // Float <- Half
    fcvt(Dst.S(), Src.H());
    break;
  }
  case 0x0802: { // Double <- Half
    fcvt(Dst.D(), Src.H());
    break;
  }
  case 0x0804: { // Double <- Float
    fcvt(Dst.D(), Src.S());
    break;
  }
  case 0x0408: { // Float <- Double
    fcvt(Dst.S(), Src.D());
    break;
  }
  default: LOGMAN_MSG_A_FMT("Unknown FCVT sizes: 0x{:x}", Conv);
  }
}

DEF_OP(Vector_SToF) {
  const auto Op = IROp->C<IR::IROp_Vector_SToF>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto SubEmitSize = ConvertSubRegSize248(IROp);
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;
  LOGMAN_THROW_A_FMT(!Is256Bit || (Is256Bit && HostSupportsSVE256), "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetVReg(Node);
  const auto Vector = GetVReg(Op->Vector.ID());
  if (HostSupportsSVE256 && Is256Bit) {
    const auto Mask = PRED_TMP_32B;
    scvtf(Dst.Z(), SubEmitSize, Mask.Merging(), Vector.Z(), SubEmitSize);
  } else {
    if (OpSize == ElementSize) {
      if (ElementSize == 8) {
        scvtf(ARMEmitter::ScalarRegSize::i64Bit, Dst.D(), Vector.D());
      } else if (ElementSize == 4) {
        scvtf(ARMEmitter::ScalarRegSize::i32Bit, Dst.S(), Vector.S());
      } else {
        scvtf(ARMEmitter::ScalarRegSize::i16Bit, Dst.H(), Vector.H());
      }
    } else {
      if (OpSize == 8) {
        scvtf(SubEmitSize, Dst.D(), Vector.D());
      } else {
        scvtf(SubEmitSize, Dst.Q(), Vector.Q());
      }
    }
  }
}

DEF_OP(Vector_FToZS) {
  const auto Op = IROp->C<IR::IROp_Vector_FToZS>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto SubEmitSize = ConvertSubRegSize248(IROp);
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;
  LOGMAN_THROW_A_FMT(!Is256Bit || (Is256Bit && HostSupportsSVE256), "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetVReg(Node);
  const auto Vector = GetVReg(Op->Vector.ID());
  if (HostSupportsSVE256 && Is256Bit) {
    const auto Mask = PRED_TMP_32B;
    fcvtzs(Dst.Z(), SubEmitSize, Mask.Merging(), Vector.Z(), SubEmitSize);
  } else {
    if (OpSize == ElementSize) {
      if (ElementSize == 8) {
        fcvtzs(ARMEmitter::ScalarRegSize::i64Bit, Dst.D(), Vector.D());
      } else if (ElementSize == 4) {
        fcvtzs(ARMEmitter::ScalarRegSize::i32Bit, Dst.S(), Vector.S());
      } else {
        fcvtzs(ARMEmitter::ScalarRegSize::i16Bit, Dst.H(), Vector.H());
      }
    } else {
      if (OpSize == 8) {
        fcvtzs(SubEmitSize, Dst.D(), Vector.D());
      } else {
        fcvtzs(SubEmitSize, Dst.Q(), Vector.Q());
      }
    }
  }
}

DEF_OP(Vector_FToS) {
  const auto Op = IROp->C<IR::IROp_Vector_FToS>();
  const auto OpSize = IROp->Size;

  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;
  LOGMAN_THROW_A_FMT(!Is256Bit || (Is256Bit && HostSupportsSVE256), "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto SubEmitSize = ConvertSubRegSize248(IROp);

  const auto Dst = GetVReg(Node);
  const auto Vector = GetVReg(Op->Vector.ID());

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Mask = PRED_TMP_32B;
    frinti(SubEmitSize, Dst.Z(), Mask.Merging(), Vector.Z());
    fcvtzs(Dst.Z(), SubEmitSize, Mask.Merging(), Dst.Z(), SubEmitSize);
  } else {
    const auto Dst = GetVReg(Node);
    const auto Vector = GetVReg(Op->Vector.ID());
    if (OpSize == 8) {
      frinti(SubEmitSize, Dst.D(), Vector.D());
      fcvtzs(SubEmitSize, Dst.D(), Dst.D());
    } else {
      frinti(SubEmitSize, Dst.Q(), Vector.Q());
      fcvtzs(SubEmitSize, Dst.Q(), Dst.Q());
    }
  }
}

DEF_OP(Vector_FToF) {
  const auto Op = IROp->C<IR::IROp_Vector_FToF>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto SubEmitSize = ConvertSubRegSize248(IROp);
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;
  LOGMAN_THROW_A_FMT(!Is256Bit || (Is256Bit && HostSupportsSVE256), "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Conv = (ElementSize << 8) | Op->SrcElementSize;

  const auto Dst = GetVReg(Node);
  const auto Vector = GetVReg(Op->Vector.ID());

  if (HostSupportsSVE256 && Is256Bit) {
    // Curiously, FCVTLT and FCVTNT have no bottom variants,
    // and also interesting is that FCVTLT will iterate the
    // source vector by accessing each odd element and storing
    // them consecutively in the destination.
    //
    // FCVTNT is somewhat like the opposite. It will read each
    // consecutive element, but store each result into every odd
    // element in the destination vector.
    //
    // We need to undo the behavior of FCVTNT with UZP2. In the case
    // of FCVTLT, we instead need to set the vector up with ZIP1, so
    // that the elements will be processed correctly.

    const auto Mask = PRED_TMP_32B.Merging();

    switch (Conv) {
    case 0x0402: { // Float <- Half
      zip1(ARMEmitter::SubRegSize::i16Bit, Dst.Z(), Vector.Z(), Vector.Z());
      fcvtlt(ARMEmitter::SubRegSize::i32Bit, Dst.Z(), Mask, Dst.Z());
      break;
    }
    case 0x0804: { // Double <- Float
      zip1(ARMEmitter::SubRegSize::i32Bit, Dst.Z(), Vector.Z(), Vector.Z());
      fcvtlt(ARMEmitter::SubRegSize::i64Bit, Dst.Z(), Mask, Dst.Z());
      break;
    }
    case 0x0204: { // Half <- Float
      fcvtnt(ARMEmitter::SubRegSize::i16Bit, Dst.Z(), Mask, Vector.Z());
      uzp2(ARMEmitter::SubRegSize::i16Bit, Dst.Z(), Dst.Z(), Dst.Z());
      break;
    }
    case 0x0408: { // Float <- Double
      fcvtnt(ARMEmitter::SubRegSize::i32Bit, Dst.Z(), Mask, Vector.Z());
      uzp2(ARMEmitter::SubRegSize::i32Bit, Dst.Z(), Dst.Z(), Dst.Z());
      break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown Vector_FToF Type : 0x{:04x}", Conv); break;
    }
  } else {
    switch (Conv) {
    case 0x0402:   // Float <- Half
    case 0x0804: { // Double <- Float
      fcvtl(SubEmitSize, Dst.D(), Vector.D());
      break;
    }
    case 0x0204:   // Half <- Float
    case 0x0408: { // Float <- Double
      fcvtn(SubEmitSize, Dst.D(), Vector.D());
      break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown Vector_FToF Type : 0x{:04x}", Conv); break;
    }
  }
}

DEF_OP(VFCVTL2) {
  const auto Op = IROp->C<IR::IROp_VFCVTL2>();

  const auto SubEmitSize = ConvertSubRegSize248(IROp);

  const auto Dst = GetVReg(Node);
  const auto Vector = GetVReg(Op->Vector.ID());

  fcvtl2(SubEmitSize, Dst.D(), Vector.D());
}

DEF_OP(VFCVTN2) {
  const auto Op = IROp->C<IR::IROp_VFCVTN2>();

  const auto SubEmitSize = ConvertSubRegSize248(IROp);

  const auto Dst = GetVReg(Node);
  const auto VectorLower = GetVReg(Op->VectorLower.ID());
  const auto VectorUpper = GetVReg(Op->VectorUpper.ID());

  auto Lower = VectorLower;
  if (Dst != VectorLower) {
    mov(VTMP1.Q(), VectorLower.Q());
    Lower = VTMP1;
  }

  fcvtn2(SubEmitSize, Lower.Q(), VectorUpper.Q());

  if (Dst != VectorLower) {
    mov(Dst.Q(), Lower.Q());
  }
}

DEF_OP(Vector_FToI) {
  const auto Op = IROp->C<IR::IROp_Vector_FToI>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto SubEmitSize = ConvertSubRegSize248(IROp);
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;
  LOGMAN_THROW_A_FMT(!Is256Bit || (Is256Bit && HostSupportsSVE256), "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetVReg(Node);
  const auto Vector = GetVReg(Op->Vector.ID());

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Mask = PRED_TMP_32B.Merging();

    switch (Op->Round) {
    case FEXCore::IR::Round_Nearest.Val: frintn(SubEmitSize, Dst.Z(), Mask, Vector.Z()); break;
    case FEXCore::IR::Round_Negative_Infinity.Val: frintm(SubEmitSize, Dst.Z(), Mask, Vector.Z()); break;
    case FEXCore::IR::Round_Positive_Infinity.Val: frintp(SubEmitSize, Dst.Z(), Mask, Vector.Z()); break;
    case FEXCore::IR::Round_Towards_Zero.Val: frintz(SubEmitSize, Dst.Z(), Mask, Vector.Z()); break;
    case FEXCore::IR::Round_Host.Val: frinti(SubEmitSize, Dst.Z(), Mask, Vector.Z()); break;
    }
  } else {
    const auto IsScalar = ElementSize == OpSize;

    if (IsScalar) {
// Since we have multiple overloads of the same name (e.g.
// frinti having AdvSIMD, AdvSIMD scalar, and an SVE version),
// we can't just use a lambda without some seriously ugly casting.
// This is fairly self-contained otherwise.
#define ROUNDING_FN(name)        \
  if (ElementSize == 2) {        \
    name(Dst.H(), Vector.H());   \
  } else if (ElementSize == 4) { \
    name(Dst.S(), Vector.S());   \
  } else if (ElementSize == 8) { \
    name(Dst.D(), Vector.D());   \
  } else {                       \
    FEX_UNREACHABLE;             \
  }

      switch (Op->Round) {
      case IR::Round_Nearest.Val: ROUNDING_FN(frintn); break;
      case IR::Round_Negative_Infinity.Val: ROUNDING_FN(frintm); break;
      case IR::Round_Positive_Infinity.Val: ROUNDING_FN(frintp); break;
      case IR::Round_Towards_Zero.Val: ROUNDING_FN(frintz); break;
      case IR::Round_Host.Val: ROUNDING_FN(frinti); break;
      }

#undef ROUNDING_FN
    } else {
      switch (Op->Round) {
      case FEXCore::IR::Round_Nearest.Val: frintn(SubEmitSize, Dst.Q(), Vector.Q()); break;
      case FEXCore::IR::Round_Negative_Infinity.Val: frintm(SubEmitSize, Dst.Q(), Vector.Q()); break;
      case FEXCore::IR::Round_Positive_Infinity.Val: frintp(SubEmitSize, Dst.Q(), Vector.Q()); break;
      case FEXCore::IR::Round_Towards_Zero.Val: frintz(SubEmitSize, Dst.Q(), Vector.Q()); break;
      case FEXCore::IR::Round_Host.Val: frinti(SubEmitSize, Dst.Q(), Vector.Q()); break;
      }
    }
  }
}

DEF_OP(Vector_F64ToI32) {
  const auto Op = IROp->C<IR::IROp_Vector_F64ToI32>();
  const auto OpSize = IROp->Size;
  const auto Round = Op->Round;

  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;
  LOGMAN_THROW_A_FMT(!Is256Bit || (Is256Bit && HostSupportsSVE256), "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetVReg(Node);
  const auto Vector = GetVReg(Op->Vector.ID());
  if (HostSupportsSVE128 || HostSupportsSVE256) {
    const auto Mask = Is256Bit ? PRED_TMP_32B.Merging() : PRED_TMP_16B.Merging();
    // First step is to round the f64 values to integrals (frint*)
    // Then convert to integers using fcvtzs.
    auto CVTReg = Dst.Z();
    switch (Round) {
    case IR::Round_Nearest.Val: frintn(ARMEmitter::SubRegSize::i64Bit, Dst.Z(), Mask, Vector.Z()); break;
    case IR::Round_Negative_Infinity.Val: frintm(ARMEmitter::SubRegSize::i64Bit, Dst.Z(), Mask, Vector.Z()); break;
    case IR::Round_Positive_Infinity.Val: frintp(ARMEmitter::SubRegSize::i64Bit, Dst.Z(), Mask, Vector.Z()); break;
    case IR::Round_Towards_Zero.Val: CVTReg = Vector.Z(); break;
    case IR::Round_Host.Val: frinti(ARMEmitter::SubRegSize::i64Bit, Dst.Z(), Mask, Vector.Z()); break;
    }

    fcvtzs(Dst.Z(), ARMEmitter::SubRegSize::i32Bit, Mask, CVTReg, ARMEmitter::SubRegSize::i64Bit);

    ///< Fixup format of register that fcvtzs returns.
    uzp1(ARMEmitter::SubRegSize::i32Bit, Dst.Z(), Dst.Z(), Dst.Z());
    if (Op->EnsureZeroUpperHalf) {
      ///< Match CVTPD2DQ/CVTTPD2DQ behaviour if necessary by zeroing the upper bits here.
      if (Is256Bit) {
        mov(Dst.Q(), Dst.Q());
      } else {
        mov(Dst.D(), Dst.D());
      }
    }
  } else {
    // This has a known precision issue that isn't easily resolvable without throwing away performance.
    // Doing the conversion in multi-stage steps has an issue that you can lose precision in the f32->i32 step if your source was f64.
    // To get around this with ASIMD FEX needs to use fcvtzs (Scalar, Integer, to GPR) for each F64 to be directly converted to i32.
    // This is a very costly transform that the SVE path doesn't need to do since it supports f64->i32 directly.
    // If this precision issue is necessary then we can add an option for it in the future.

    ///< Round float to integral depending on rounding mode.
    switch (Round) {
    case FEXCore::IR::Round_Nearest.Val: frintn(ARMEmitter::SubRegSize::i64Bit, Dst.Q(), Vector.Q()); break;
    case FEXCore::IR::Round_Negative_Infinity.Val: frintm(ARMEmitter::SubRegSize::i64Bit, Dst.Q(), Vector.Q()); break;
    case FEXCore::IR::Round_Positive_Infinity.Val: frintp(ARMEmitter::SubRegSize::i64Bit, Dst.Q(), Vector.Q()); break;
    case FEXCore::IR::Round_Towards_Zero.Val: frintz(ARMEmitter::SubRegSize::i64Bit, Dst.Q(), Vector.Q()); break;
    case FEXCore::IR::Round_Host.Val: frinti(ARMEmitter::SubRegSize::i64Bit, Dst.Q(), Vector.Q()); break;
    }

    // Now narrow from f64 to f32.
    fcvtn(ARMEmitter::SubRegSize::i32Bit, Dst.Q(), Dst.Q());

    ///< Convert the two F32 integrals to real integers.
    fcvtzs(ARMEmitter::SubRegSize::i32Bit, Dst.D(), Dst.D());
  }
}

#undef DEF_OP
} // namespace FEXCore::CPU
