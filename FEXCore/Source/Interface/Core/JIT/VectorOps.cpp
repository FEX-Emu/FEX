// SPDX-License-Identifier: MIT
/*
$info$
tags: backend|arm64
$end_info$
*/

#include "Interface/Core/Dispatcher/Dispatcher.h"
#include "Interface/Core/JIT/JITClass.h"

#include <FEXCore/Utils/MathUtils.h>

namespace FEXCore::CPU {

#define DEF_UNOP(FEXOp, ARMOp, ScalarCase)                                                                                          \
  DEF_OP(FEXOp) {                                                                                                                   \
    const auto Op = IROp->C<IR::IROp_##FEXOp>();                                                                                    \
    const auto OpSize = IROp->Size;                                                                                                 \
                                                                                                                                    \
    const auto ElementSize = Op->Header.ElementSize;                                                                                \
    const auto Is256Bit = OpSize == IR::OpSize::i256Bit;                                                                            \
    LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__); \
    const auto SubRegSize = ConvertSubRegSize8(IROp);                                                                               \
                                                                                                                                    \
    const auto Dst = GetVReg(Node);                                                                                                 \
    const auto Src = GetVReg(Op->Vector);                                                                                           \
                                                                                                                                    \
    if (HostSupportsSVE256 && Is256Bit) {                                                                                           \
      ARMOp(SubRegSize, Dst.Z(), PRED_TMP_32B.Merging(), Src.Z());                                                                  \
    } else {                                                                                                                        \
      if (ElementSize == OpSize && ScalarCase) {                                                                                    \
        ARMOp(SubRegSize, Dst.D(), Src.D());                                                                                        \
      } else {                                                                                                                      \
        ARMOp(SubRegSize, Dst.Q(), Src.Q());                                                                                        \
      }                                                                                                                             \
    }                                                                                                                               \
  }

#define DEF_BITOP(FEXOp, ARMOp)                                                                                                     \
  DEF_OP(FEXOp) {                                                                                                                   \
    const auto Op = IROp->C<IR::IROp_##FEXOp>();                                                                                    \
    const auto OpSize = IROp->Size;                                                                                                 \
    const auto Is256Bit = OpSize == IR::OpSize::i256Bit;                                                                            \
    LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__); \
                                                                                                                                    \
    const auto Dst = GetVReg(Node);                                                                                                 \
    const auto Vector1 = GetVReg(Op->Vector1);                                                                                      \
    const auto Vector2 = GetVReg(Op->Vector2);                                                                                      \
                                                                                                                                    \
    if (HostSupportsSVE256 && Is256Bit) {                                                                                           \
      ARMOp(Dst.Z(), Vector1.Z(), Vector2.Z());                                                                                     \
    } else {                                                                                                                        \
      ARMOp(Dst.Q(), Vector1.Q(), Vector2.Q());                                                                                     \
    }                                                                                                                               \
  }

#define DEF_BINOP(FEXOp, ARMOp)                                                                                                     \
  DEF_OP(FEXOp) {                                                                                                                   \
    const auto Op = IROp->C<IR::IROp_##FEXOp>();                                                                                    \
    const auto OpSize = IROp->Size;                                                                                                 \
                                                                                                                                    \
    const auto Is256Bit = OpSize == IR::OpSize::i256Bit;                                                                            \
    LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__); \
    const auto SubRegSize = ConvertSubRegSize8(IROp);                                                                               \
                                                                                                                                    \
    const auto Dst = GetVReg(Node);                                                                                                 \
    const auto Vector1 = GetVReg(Op->Vector1);                                                                                      \
    const auto Vector2 = GetVReg(Op->Vector2);                                                                                      \
                                                                                                                                    \
    if (HostSupportsSVE256 && Is256Bit) {                                                                                           \
      ARMOp(SubRegSize, Dst.Z(), Vector1.Z(), Vector2.Z());                                                                         \
    } else {                                                                                                                        \
      ARMOp(SubRegSize, Dst.Q(), Vector1.Q(), Vector2.Q());                                                                         \
    }                                                                                                                               \
  }

#define DEF_ZIPOP(FEXOp, ARMOp)                                                                                                     \
  DEF_OP(FEXOp) {                                                                                                                   \
    const auto Op = IROp->C<IR::IROp_##FEXOp>();                                                                                    \
    const auto OpSize = IROp->Size;                                                                                                 \
                                                                                                                                    \
    const auto SubRegSize = ConvertSubRegSize8(IROp);                                                                               \
    const auto Is256Bit = OpSize == IR::OpSize::i256Bit;                                                                            \
    LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__); \
                                                                                                                                    \
    const auto Dst = GetVReg(Node);                                                                                                 \
    const auto VectorLower = GetVReg(Op->VectorLower);                                                                              \
    const auto VectorUpper = GetVReg(Op->VectorUpper);                                                                              \
                                                                                                                                    \
    if (HostSupportsSVE256 && Is256Bit) {                                                                                           \
      ARMOp(SubRegSize, Dst.Z(), VectorLower.Z(), VectorUpper.Z());                                                                 \
    } else {                                                                                                                        \
      if (OpSize == IR::OpSize::i64Bit) {                                                                                           \
        ARMOp(SubRegSize, Dst.D(), VectorLower.D(), VectorUpper.D());                                                               \
      } else {                                                                                                                      \
        ARMOp(SubRegSize, Dst.Q(), VectorLower.Q(), VectorUpper.Q());                                                               \
      }                                                                                                                             \
    }                                                                                                                               \
  }

#define DEF_FUNOP(FEXOp, ARMOp)                                                                                                     \
  DEF_OP(FEXOp) {                                                                                                                   \
    const auto Op = IROp->C<IR::IROp_##FEXOp>();                                                                                    \
    const auto OpSize = IROp->Size;                                                                                                 \
                                                                                                                                    \
    const auto ElementSize = Op->Header.ElementSize;                                                                                \
    const auto SubRegSize = ConvertSubRegSize248(IROp);                                                                             \
    const auto Is256Bit = OpSize == IR::OpSize::i256Bit;                                                                            \
    LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__); \
                                                                                                                                    \
    const auto Dst = GetVReg(Node);                                                                                                 \
    const auto Src = GetVReg(Op->Vector);                                                                                           \
                                                                                                                                    \
    if (HostSupportsSVE256 && Is256Bit) {                                                                                           \
      ARMOp(SubRegSize, Dst.Z(), PRED_TMP_32B.Merging(), Src.Z());                                                                  \
    } else {                                                                                                                        \
      if (ElementSize == OpSize) {                                                                                                  \
        switch (ElementSize) {                                                                                                      \
        case IR::OpSize::i16Bit: {                                                                                                  \
          ARMOp(Dst.H(), Src.H());                                                                                                  \
          break;                                                                                                                    \
        }                                                                                                                           \
        case IR::OpSize::i32Bit: {                                                                                                  \
          ARMOp(Dst.S(), Src.S());                                                                                                  \
          break;                                                                                                                    \
        }                                                                                                                           \
        case IR::OpSize::i64Bit: {                                                                                                  \
          ARMOp(Dst.D(), Src.D());                                                                                                  \
          break;                                                                                                                    \
        }                                                                                                                           \
        default: break;                                                                                                             \
        }                                                                                                                           \
      } else {                                                                                                                      \
        ARMOp(SubRegSize, Dst.Q(), Src.Q());                                                                                        \
      }                                                                                                                             \
    }                                                                                                                               \
  }

#define DEF_FBINOP(FEXOp, ARMOp)                                                                                                    \
  DEF_OP(FEXOp) {                                                                                                                   \
    const auto Op = IROp->C<IR::IROp_##FEXOp>();                                                                                    \
    const auto OpSize = IROp->Size;                                                                                                 \
                                                                                                                                    \
    const auto ElementSize = Op->Header.ElementSize;                                                                                \
    const auto SubRegSize = ConvertSubRegSize248(IROp);                                                                             \
    const auto Is256Bit = OpSize == IR::OpSize::i256Bit;                                                                            \
    LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__); \
    const auto IsScalar = ElementSize == OpSize;                                                                                    \
                                                                                                                                    \
    const auto Dst = GetVReg(Node);                                                                                                 \
    const auto Vector1 = GetVReg(Op->Vector1);                                                                                      \
    const auto Vector2 = GetVReg(Op->Vector2);                                                                                      \
                                                                                                                                    \
    if (HostSupportsSVE256 && Is256Bit) {                                                                                           \
      ARMOp(SubRegSize, Dst.Z(), Vector1.Z(), Vector2.Z());                                                                         \
    } else {                                                                                                                        \
      if (IsScalar) {                                                                                                               \
        switch (ElementSize) {                                                                                                      \
        case IR::OpSize::i16Bit: {                                                                                                  \
          ARMOp(Dst.H(), Vector1.H(), Vector2.H());                                                                                 \
          break;                                                                                                                    \
        }                                                                                                                           \
        case IR::OpSize::i32Bit: {                                                                                                  \
          ARMOp(Dst.S(), Vector1.S(), Vector2.S());                                                                                 \
          break;                                                                                                                    \
        }                                                                                                                           \
        case IR::OpSize::i64Bit: {                                                                                                  \
          ARMOp(Dst.D(), Vector1.D(), Vector2.D());                                                                                 \
          break;                                                                                                                    \
        }                                                                                                                           \
        default: break;                                                                                                             \
        }                                                                                                                           \
      } else {                                                                                                                      \
        ARMOp(SubRegSize, Dst.Q(), Vector1.Q(), Vector2.Q());                                                                       \
      }                                                                                                                             \
    }                                                                                                                               \
  }

#define DEF_FBINOP_SCALAR_INSERT(FEXOp, ARMOp)                                                                                \
  DEF_OP(FEXOp) {                                                                                                             \
    const auto Op = IROp->C<IR::IROp_##FEXOp>();                                                                              \
    const auto ElementSize = Op->Header.ElementSize;                                                                          \
    const auto SubRegSize = ConvertSubRegSizePair248(IROp);                                                                   \
                                                                                                                              \
    auto ScalarEmit = [this, SubRegSize](ARMEmitter::VRegister Dst, ARMEmitter::VRegister Src1, ARMEmitter::VRegister Src2) { \
      ARMOp(SubRegSize.Scalar, Dst, Src1, Src2);                                                                              \
    };                                                                                                                        \
                                                                                                                              \
    const auto Dst = GetVReg(Node);                                                                                           \
    const auto Vector1 = GetVReg(Op->Vector1);                                                                                \
    const auto Vector2 = GetVReg(Op->Vector2);                                                                                \
                                                                                                                              \
    VFScalarOperation(IROp->Size, ElementSize, Op->ZeroUpperBits, ScalarEmit, Dst, Vector1, Vector2);                         \
  }

#define DEF_FMAOP_SCALAR_INSERT(FEXOp, ARMOp)                                                                                \
  DEF_OP(FEXOp) {                                                                                                            \
    const auto Op = IROp->C<IR::IROp_##FEXOp>();                                                                             \
    const auto ElementSize = Op->Header.ElementSize;                                                                         \
                                                                                                                             \
    auto ScalarEmit = [this, ElementSize](ARMEmitter::VRegister Dst, ARMEmitter::VRegister Src1, ARMEmitter::VRegister Src2, \
                                          ARMEmitter::VRegister Src3) {                                                      \
      if (ElementSize == IR::OpSize::i16Bit) {                                                                               \
        ARMOp(Dst.H(), Src1.H(), Src2.H(), Src3.H());                                                                        \
      } else if (ElementSize == IR::OpSize::i32Bit) {                                                                        \
        ARMOp(Dst.S(), Src1.S(), Src2.S(), Src3.S());                                                                        \
      } else if (ElementSize == IR::OpSize::i64Bit) {                                                                        \
        ARMOp(Dst.D(), Src1.D(), Src2.D(), Src3.D());                                                                        \
      }                                                                                                                      \
    };                                                                                                                       \
                                                                                                                             \
    const auto Dst = GetVReg(Node);                                                                                          \
    const auto Upper = GetVReg(Op->Upper);                                                                                   \
    const auto Vector1 = GetVReg(Op->Vector1);                                                                               \
    const auto Vector2 = GetVReg(Op->Vector2);                                                                               \
    const auto Addend = GetVReg(Op->Addend);                                                                                 \
                                                                                                                             \
    VFScalarFMAOperation(IROp->Size, ElementSize, ScalarEmit, Dst, Upper, Vector1, Vector2, Addend);                         \
  }

DEF_UNOP(VAbs, abs, true)
DEF_UNOP(VPopcount, cnt, true)
DEF_UNOP(VNeg, neg, false)
DEF_UNOP(VFNeg, fneg, false)

DEF_BITOP(VAnd, and_)
DEF_BITOP(VAndn, bic)
DEF_BITOP(VOr, orr)
DEF_BITOP(VXor, eor)

DEF_BINOP(VAdd, add)
DEF_BINOP(VSub, sub)
DEF_BINOP(VUQAdd, uqadd)
DEF_BINOP(VUQSub, uqsub)
DEF_BINOP(VSQAdd, sqadd)
DEF_BINOP(VSQSub, sqsub)

DEF_ZIPOP(VZip, zip1)
DEF_ZIPOP(VZip2, zip2)
DEF_ZIPOP(VUnZip, uzp1)
DEF_ZIPOP(VUnZip2, uzp2)
DEF_ZIPOP(VTrn, trn1)
DEF_ZIPOP(VTrn2, trn2)

DEF_FUNOP(VFSqrt, fsqrt)
DEF_FUNOP(VFAbs, fabs)

DEF_FBINOP(VFAdd, fadd)
DEF_FBINOP(VFSub, fsub)
DEF_FBINOP(VFMul, fmul)

DEF_FBINOP_SCALAR_INSERT(VFAddScalarInsert, fadd)
DEF_FBINOP_SCALAR_INSERT(VFSubScalarInsert, fsub)
DEF_FBINOP_SCALAR_INSERT(VFMulScalarInsert, fmul)
DEF_FBINOP_SCALAR_INSERT(VFDivScalarInsert, fdiv)

DEF_FMAOP_SCALAR_INSERT(VFMLAScalarInsert, fmadd)
DEF_FMAOP_SCALAR_INSERT(VFMLSScalarInsert, fnmsub)
DEF_FMAOP_SCALAR_INSERT(VFNMLAScalarInsert, fmsub)
DEF_FMAOP_SCALAR_INSERT(VFNMLSScalarInsert, fnmadd)

void Arm64JITCore::VFScalarFMAOperation(IR::OpSize OpSize, IR::OpSize ElementSize, ScalarFMAOpCaller ScalarEmit, ARMEmitter::VRegister Dst,
                                        ARMEmitter::VRegister Upper, ARMEmitter::VRegister Vector1, ARMEmitter::VRegister Vector2,
                                        ARMEmitter::VRegister Addend) {
  LOGMAN_THROW_A_FMT(OpSize == IR::OpSize::i128Bit, "256-bit unsupported", __func__);

  LOGMAN_THROW_A_FMT(ElementSize == IR::OpSize::i16Bit || ElementSize == IR::OpSize::i32Bit || ElementSize == IR::OpSize::i64Bit, "Invalid "
                                                                                                                                  "size");
  const auto SubRegSize = ARMEmitter::ToVectorSizePair(ElementSize == IR::OpSize::i16Bit ? ARMEmitter::SubRegSize::i16Bit :
                                                       ElementSize == IR::OpSize::i32Bit ? ARMEmitter::SubRegSize::i32Bit :
                                                                                           ARMEmitter::SubRegSize::i64Bit);

  if (Dst != Upper) {
    // If destination is not tied, move the upper bits to the destination first.
    mov(Dst.Q(), Upper.Q());
  }

  if (HostSupportsAFP && Dst == Addend) {
    ///< Exactly matches ARM scalar FMA semantics
    // If the host CPU supports AFP then scalar does an insert without modifying upper bits.
    ScalarEmit(Dst, Vector1, Vector2, Addend);
  } else {
    // Host doesn't support AFP, need to emit in to a temporary then insert.
    ScalarEmit(VTMP1, Vector1, Vector2, Addend);
    ins(SubRegSize.Vector, Dst.Q(), 0, VTMP1.Q(), 0);
  }
}

// VFScalarOperation performs the operation described through ScalarEmit between Vector1 and Vector2,
// storing it into Dst. This is a scalar operation, so the only lowest element of each vector is used for the operation.
// The result is stored into the destination. The untouched bits of the destination come from Vector1, unless it's a 256 vector
// and ZeroUpperBits is true, in which case the upper bits are zero.
void Arm64JITCore::VFScalarOperation(IR::OpSize OpSize, IR::OpSize ElementSize, bool ZeroUpperBits, ScalarBinaryOpCaller ScalarEmit,
                                     ARMEmitter::VRegister Dst, ARMEmitter::VRegister Vector1, ARMEmitter::VRegister Vector2) {
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);
  LOGMAN_THROW_A_FMT(Is256Bit || !ZeroUpperBits, "128-bit operation doesn't support ZeroUpperBits in {}", __func__);

  // Bit of a tricky detail.
  // The upper bits of the destination comes from Vector1.
  LOGMAN_THROW_A_FMT(ElementSize == IR::OpSize::i16Bit || ElementSize == IR::OpSize::i32Bit || ElementSize == IR::OpSize::i64Bit, "Invalid "
                                                                                                                                  "size");
  const auto SubRegSize = ARMEmitter::ToVectorSizePair(ElementSize == IR::OpSize::i16Bit ? ARMEmitter::SubRegSize::i16Bit :
                                                       ElementSize == IR::OpSize::i32Bit ? ARMEmitter::SubRegSize::i32Bit :
                                                                                           ARMEmitter::SubRegSize::i64Bit);

  constexpr auto Predicate = ARMEmitter::PReg::p0;

  if (Dst == Vector1) {
    if (ZeroUpperBits) {
      // When zeroing the upper 128-bits we just use an ASIMD move.
      mov(Dst.Q(), Vector1.Q());
    }

    if (HostSupportsAFP) {
      // If the host CPU supports AFP then scalar does an insert without modifying upper bits.
      ScalarEmit(Dst, Vector1, Vector2);
    } else {
      // If AFP is unsupported then the operation result goes in to a temporary.
      // and then it gets inserted.
      ScalarEmit(VTMP1, Vector1, Vector2);
      if (!ZeroUpperBits && Is256Bit) {
        ptrue(SubRegSize.Vector, Predicate, ARMEmitter::PredicatePattern::SVE_VL1);
        mov(SubRegSize.Vector, Dst.Z(), Predicate.Merging(), VTMP1.Z());
      } else {
        ins(SubRegSize.Vector, Dst.Q(), 0, VTMP1.Q(), 0);
      }
    }
  } else if (Dst != Vector2) { // Dst different from both Vector1 and Vector2
    if (Is256Bit && !ZeroUpperBits) {
      mov(Dst.Z(), Vector1.Z());
    } else {
      mov(Dst.Q(), Vector1.Q());
    }

    if (HostSupportsAFP) {
      ScalarEmit(Dst, Vector1, Vector2);
    } else {
      ScalarEmit(VTMP1, Vector1, Vector2);
      if (!ZeroUpperBits && Is256Bit) {
        ptrue(SubRegSize.Vector, Predicate, ARMEmitter::PredicatePattern::SVE_VL1);
        mov(SubRegSize.Vector, Dst.Z(), Predicate.Merging(), VTMP1.Z());
      } else {
        ins(SubRegSize.Vector, Dst.Q(), 0, VTMP1.Q(), 0);
      }
    }
  } else { // Dst same as Vector2

    ScalarEmit(VTMP1, Vector1, Vector2);

    if (!ZeroUpperBits && Is256Bit) {
      mov(Dst.Z(), Vector1.Z());
      ptrue(SubRegSize.Vector, Predicate, ARMEmitter::PredicatePattern::SVE_VL1);
      mov(SubRegSize.Vector, Dst.Z(), Predicate.Merging(), VTMP1.Z());
    } else {
      mov(Dst.Q(), Vector1.Q());
      ins(SubRegSize.Vector, Dst.Q(), 0, VTMP1.Q(), 0);
    }
  }
}

// Similarly to VFScalarOperation it performs the operation described through ScalarEmit operating on Vector2.
// However the result of the scalar operation is inserted into Vector1 and moved to Destination.
// The untouched bits of the destination come from Vector1, unless it's a 256 vector
// and ZeroUpperBits is true, in which case the upper bits are zero.
void Arm64JITCore::VFScalarUnaryOperation(IR::OpSize OpSize, IR::OpSize ElementSize, bool ZeroUpperBits, ScalarUnaryOpCaller ScalarEmit,
                                          ARMEmitter::VRegister Dst, ARMEmitter::VRegister Vector1,
                                          std::variant<ARMEmitter::VRegister, ARMEmitter::Register> Vector2) {
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);
  LOGMAN_THROW_A_FMT(Is256Bit || !ZeroUpperBits, "128-bit operation doesn't support ZeroUpperBits in {}", __func__);

  LOGMAN_THROW_A_FMT(ElementSize == IR::OpSize::i16Bit || ElementSize == IR::OpSize::i32Bit || ElementSize == IR::OpSize::i64Bit, "Invalid "
                                                                                                                                  "size");
  const auto SubRegSize = ARMEmitter::ToVectorSizePair(ElementSize == IR::OpSize::i16Bit ? ARMEmitter::SubRegSize::i16Bit :
                                                       ElementSize == IR::OpSize::i32Bit ? ARMEmitter::SubRegSize::i32Bit :
                                                                                           ARMEmitter::SubRegSize::i64Bit);

  constexpr auto Predicate = ARMEmitter::PReg::p0;
  bool DstOverlapsVector2 = false;
  if (const auto* Vector2Reg = std::get_if<ARMEmitter::VRegister>(&Vector2)) {
    DstOverlapsVector2 = Dst == *Vector2Reg;
  }

  if (Dst == Vector1) {
    if (ZeroUpperBits) {
      // When zeroing the upper 128-bits we just use an ASIMD move.
      mov(Dst.Q(), Vector1.Q());
    }

    if (HostSupportsAFP) { // or Dst (here Dst == Vector1)
      // If the host CPU supports AFP then scalar does an insert without modifying upper bits.
      ScalarEmit(Dst, Vector2);
    } else {
      // If AFP is unsupported then the operation result goes in to a temporary.
      // and then it gets inserted.
      ScalarEmit(VTMP1, Vector2);
      if (!ZeroUpperBits && Is256Bit) {
        ptrue(SubRegSize.Vector, Predicate, ARMEmitter::PredicatePattern::SVE_VL1);
        mov(SubRegSize.Vector, Dst.Z(), Predicate.Merging(), VTMP1.Z());
      } else {
        ins(SubRegSize.Vector, Dst.Q(), 0, VTMP1.Q(), 0);
      }
    }
  } else if (!DstOverlapsVector2) {
    if (!ZeroUpperBits && Is256Bit) {
      mov(Dst.Z(), Vector1.Z());
    } else {
      mov(Dst.Q(), Vector1.Q());
    }

    if (HostSupportsAFP) {
      ScalarEmit(Dst, Vector2);
    } else {
      ScalarEmit(VTMP1, Vector2);
      if (!ZeroUpperBits && Is256Bit) {
        ptrue(SubRegSize.Vector, Predicate, ARMEmitter::PredicatePattern::SVE_VL1);
        mov(SubRegSize.Vector, Dst.Z(), Predicate.Merging(), VTMP1.Z());
      } else {
        ins(SubRegSize.Vector, Dst.Q(), 0, VTMP1.Q(), 0);
      }
    }
  } else {
    // Destination intersects Vector2, can't do anything optimal in this case.
    // Do the scalar operation first and then move and insert.
    ScalarEmit(VTMP1, Vector2);

    if (!ZeroUpperBits && Is256Bit) {
      mov(Dst.Z(), Vector1.Z());
      ptrue(SubRegSize.Vector, Predicate, ARMEmitter::PredicatePattern::SVE_VL1);
      mov(SubRegSize.Vector, Dst.Z(), Predicate.Merging(), VTMP1.Z());
    } else {
      mov(Dst.Q(), Vector1.Q());
      ins(SubRegSize.Vector, Dst.Q(), 0, VTMP1.Q(), 0);
    }
  }
}

DEF_OP(VFMinScalarInsert) {
  const auto Op = IROp->C<IR::IROp_VFMinScalarInsert>();
  const auto ElementSize = Op->Header.ElementSize;
  const auto SubRegSize = ConvertSubRegSizePair248(IROp);

  auto ScalarEmit = [this, SubRegSize](ARMEmitter::VRegister Dst, ARMEmitter::VRegister Src1, ARMEmitter::VRegister Src2) {
    if (HostSupportsAFP) {
      // AFP.AH lets fmin behave like x86 min
      fmin(SubRegSize.Scalar, Dst, Src1, Src2);
    } else {
      // Only take the first operand if it is strictly less. Otherwise take
      // the second. This emulates all the weird x86 rules for signed zero and
      // NaNs. No, they're not IEEE-754 semantics.
      fcmp(SubRegSize.Scalar, Src1, Src2);
      fcsel(SubRegSize.Scalar, Dst, Src1, Src2, ARMEmitter::Condition::CC_MI);
    }
  };

  // Bit of a tricky detail.
  // The upper bits of the destination comes from the first source.
  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1);
  const auto Vector2 = GetVReg(Op->Vector2);

  VFScalarOperation(IROp->Size, ElementSize, Op->ZeroUpperBits, ScalarEmit, Dst, Vector1, Vector2);
}

DEF_OP(VFMaxScalarInsert) {
  const auto Op = IROp->C<IR::IROp_VFMaxScalarInsert>();
  const auto ElementSize = Op->Header.ElementSize;
  const auto SubRegSize = ConvertSubRegSizePair248(IROp);

  // AFP can make this more optimal.
  auto ScalarEmit = [this, SubRegSize](ARMEmitter::VRegister Dst, ARMEmitter::VRegister Src1, ARMEmitter::VRegister Src2) {
    if (HostSupportsAFP) {
      // AFP.AH lets fmax behave like x86 max
      fmax(SubRegSize.Scalar, Dst, Src1, Src2);
    } else {
      // Only take the first operand if it is strictly greater. See fmin.
      fcmp(SubRegSize.Scalar, Src1, Src2);
      fcsel(SubRegSize.Scalar, Dst, Src1, Src2, ARMEmitter::Condition::CC_GT);
    }
  };

  // Bit of a tricky detail.
  // The upper bits of the destination comes from the first source.
  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1);
  const auto Vector2 = GetVReg(Op->Vector2);

  VFScalarOperation(IROp->Size, ElementSize, Op->ZeroUpperBits, ScalarEmit, Dst, Vector1, Vector2);
}

DEF_OP(VFSqrtScalarInsert) {
  const auto Op = IROp->C<IR::IROp_VFSqrtScalarInsert>();
  const auto ElementSize = Op->Header.ElementSize;
  const auto SubRegSize = ConvertSubRegSizePair248(IROp);

  auto ScalarEmit = [this, SubRegSize](ARMEmitter::VRegister Dst, std::variant<ARMEmitter::VRegister, ARMEmitter::Register> SrcVar) {
    auto Src = *std::get_if<ARMEmitter::VRegister>(&SrcVar);
    fsqrt(SubRegSize.Scalar, Dst, Src);
  };

  // Bit of a tricky detail.
  // The upper bits of the destination comes from the first source.
  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1);
  const auto Vector2 = GetVReg(Op->Vector2);

  VFScalarUnaryOperation(IROp->Size, ElementSize, Op->ZeroUpperBits, ScalarEmit, Dst, Vector1, Vector2);
}

DEF_OP(VFRSqrtScalarInsert) {
  const auto Op = IROp->C<IR::IROp_VFRSqrtScalarInsert>();
  const auto ElementSize = Op->Header.ElementSize;
  const auto SubRegSize = ConvertSubRegSizePair248(IROp);

  auto ScalarEmit = [this, SubRegSize](ARMEmitter::VRegister Dst, std::variant<ARMEmitter::VRegister, ARMEmitter::Register> SrcVar) {
    auto Src = *std::get_if<ARMEmitter::VRegister>(&SrcVar);

    fmov(SubRegSize.Scalar, VTMP1.Q(), 1.0f);
    fsqrt(SubRegSize.Scalar, VTMP2, Src);
    if (HostSupportsAFP) {
      fdiv(SubRegSize.Scalar, VTMP1, VTMP1, VTMP2);
      ins(SubRegSize.Vector, Dst, 0, VTMP1, 0);
    } else {
      fdiv(SubRegSize.Scalar, Dst, VTMP1, VTMP2);
    }
  };

  auto ScalarEmitRPRES = [this, SubRegSize](ARMEmitter::VRegister Dst, std::variant<ARMEmitter::VRegister, ARMEmitter::Register> SrcVar) {
    auto Src = *std::get_if<ARMEmitter::VRegister>(&SrcVar);
    frsqrte(SubRegSize.Scalar, Dst.D(), Src.D());
  };

  std::array<ScalarUnaryOpCaller, 2> Handlers = {
    ScalarEmit,
    ScalarEmitRPRES,
  };
  const auto HandlerIndex = ElementSize == IR::OpSize::i32Bit && HostSupportsRPRES ? 1 : 0;

  // Bit of a tricky detail.
  // The upper bits of the destination comes from the first source.
  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1);
  const auto Vector2 = GetVReg(Op->Vector2);

  VFScalarUnaryOperation(IROp->Size, ElementSize, Op->ZeroUpperBits, Handlers[HandlerIndex], Dst, Vector1, Vector2);
}

DEF_OP(VFRecpScalarInsert) {
  const auto Op = IROp->C<IR::IROp_VFRecpScalarInsert>();
  const auto ElementSize = Op->Header.ElementSize;
  const auto SubRegSize = ConvertSubRegSizePair248(IROp);

  auto ScalarEmit = [this, SubRegSize](ARMEmitter::VRegister Dst, std::variant<ARMEmitter::VRegister, ARMEmitter::Register> SrcVar) {
    auto Src = *std::get_if<ARMEmitter::VRegister>(&SrcVar);

    fmov(SubRegSize.Scalar, VTMP1.Q(), 1.0f);
    if (HostSupportsAFP) {
      fdiv(SubRegSize.Scalar, VTMP1, VTMP1, Src);
      ins(SubRegSize.Vector, Dst, 0, VTMP1, 0);
    } else {
      fdiv(SubRegSize.Scalar, Dst, VTMP1, Src);
    }
  };

  auto ScalarEmitRPRES = [this, SubRegSize](ARMEmitter::VRegister Dst, std::variant<ARMEmitter::VRegister, ARMEmitter::Register> SrcVar) {
    auto Src = *std::get_if<ARMEmitter::VRegister>(&SrcVar);
    frecpe(SubRegSize.Scalar, Dst, Src);
  };

  std::array<ScalarUnaryOpCaller, 2> Handlers = {
    ScalarEmit,
    ScalarEmitRPRES,
  };
  const auto HandlerIndex = ElementSize == IR::OpSize::i32Bit && HostSupportsRPRES ? 1 : 0;

  // Bit of a tricky detail.
  // The upper bits of the destination comes from the first source.
  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1);
  const auto Vector2 = GetVReg(Op->Vector2);

  VFScalarUnaryOperation(IROp->Size, ElementSize, Op->ZeroUpperBits, Handlers[HandlerIndex], Dst, Vector1, Vector2);
}

DEF_OP(VFToFScalarInsert) {
  const auto Op = IROp->C<IR::IROp_VFToFScalarInsert>();
  const auto ElementSize = Op->Header.ElementSize;
  const uint16_t Conv = (IR::OpSizeToSize(Op->Header.ElementSize) << 8) | IR::OpSizeToSize(Op->SrcElementSize);

  auto ScalarEmit = [this, Conv](ARMEmitter::VRegister Dst, std::variant<ARMEmitter::VRegister, ARMEmitter::Register> SrcVar) {
    auto Src = *std::get_if<ARMEmitter::VRegister>(&SrcVar);

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
  };

  // Bit of a tricky detail.
  // The upper bits of the destination comes from the first source.
  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1);
  const auto Vector2 = GetVReg(Op->Vector2);

  VFScalarUnaryOperation(IROp->Size, ElementSize, Op->ZeroUpperBits, ScalarEmit, Dst, Vector1, Vector2);
}

DEF_OP(VSToFVectorInsert) {
  const auto Op = IROp->C<IR::IROp_VSToFVectorInsert>();
  const auto ElementSize = Op->Header.ElementSize;
  const auto HasTwoElements = Op->HasTwoElements;

  LOGMAN_THROW_A_FMT(ElementSize == IR::OpSize::i32Bit || ElementSize == IR::OpSize::i64Bit, "Invalid size");
  if (HasTwoElements) {
    LOGMAN_THROW_A_FMT(ElementSize == IR::OpSize::i32Bit, "Can't have two elements for 8-byte size");
  }

  auto ScalarEmit = [this, ElementSize, HasTwoElements](ARMEmitter::VRegister Dst, std::variant<ARMEmitter::VRegister, ARMEmitter::Register> SrcVar) {
    auto Src = *std::get_if<ARMEmitter::VRegister>(&SrcVar);
    if (ElementSize == IR::OpSize::i32Bit) {
      if (HasTwoElements) {
        scvtf(ARMEmitter::SubRegSize::i32Bit, Dst.D(), Src.D());
      } else {
        scvtf(ARMEmitter::ScalarRegSize::i32Bit, Dst.S(), Src.S());
      }
    } else {
      scvtf(ARMEmitter::ScalarRegSize::i64Bit, Dst.D(), Src.D());
    }
  };

  // Bit of a tricky detail.
  // The upper bits of the destination comes from the first source.
  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1);
  const auto Vector2 = GetVReg(Op->Vector2);

  // Claim the element size is 8-bytes.
  // Might be scalar 8-byte (cvtsi2ss xmm0, rax)
  // Might be vector i32v2 (cvtpi2ps xmm0, mm0)
  if (!HasTwoElements) {
    VFScalarUnaryOperation(IROp->Size, ElementSize, Op->ZeroUpperBits, ScalarEmit, Dst, Vector1, Vector2);
    return;
  }

  // Dealing with the odd case of this being actually a vector operation rather than scalar.
  const auto Is256Bit = IROp->Size == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);
  constexpr auto Predicate = ARMEmitter::PReg::p0;

  ScalarEmit(VTMP1, Vector2);
  if (!Op->ZeroUpperBits && Is256Bit) {
    if (Dst != Vector1) {
      mov(Dst.Z(), Vector1.Z());
    }
    ptrue(ARMEmitter::SubRegSize::i64Bit, Predicate, ARMEmitter::PredicatePattern::SVE_VL1);
    mov(ARMEmitter::SubRegSize::i64Bit, Dst.Z(), Predicate.Merging(), VTMP1.Z());
  } else {
    if (Dst != Vector1) {
      mov(Dst.Q(), Vector1.Q());
    }
    ins(ARMEmitter::SubRegSize::i64Bit, Dst.Q(), 0, VTMP1.Q(), 0);
  }
}

DEF_OP(VSToFGPRInsert) {
  const auto Op = IROp->C<IR::IROp_VSToFGPRInsert>();

  const auto ElementSize = Op->Header.ElementSize;
  const uint16_t Conv = (IR::OpSizeToSize(ElementSize) << 8) | IR::OpSizeToSize(Op->SrcElementSize);

  auto ScalarEmit = [this, Conv](ARMEmitter::VRegister Dst, std::variant<ARMEmitter::VRegister, ARMEmitter::Register> SrcVar) {
    auto Src = *std::get_if<ARMEmitter::Register>(&SrcVar);

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
    default: LOGMAN_MSG_A_FMT("Unhandled conversion mask: Mask=0x{:04x}", Conv); break;
    }
  };


  // Bit of a tricky detail.
  // The upper bits of the destination comes from the first source.
  const auto Dst = GetVReg(Node);
  const auto Vector = GetVReg(Op->Vector);
  const auto GPR = GetReg(Op->Src);

  VFScalarUnaryOperation(IROp->Size, ElementSize, Op->ZeroUpperBits, ScalarEmit, Dst, Vector, GPR);
}

DEF_OP(VFToIScalarInsert) {
  const auto Op = IROp->C<IR::IROp_VFToIScalarInsert>();
  const auto ElementSize = Op->Header.ElementSize;
  const auto SubRegSize = ConvertSubRegSizePair248(IROp);
  const auto RoundMode = Op->Round;

  auto ScalarEmit = [this, SubRegSize, RoundMode](ARMEmitter::VRegister Dst, std::variant<ARMEmitter::VRegister, ARMEmitter::Register> SrcVar) {
    auto Src = *std::get_if<ARMEmitter::VRegister>(&SrcVar);

    switch (RoundMode) {
    case IR::Round_Nearest: frintn(SubRegSize.Scalar, Dst, Src); break;
    case IR::Round_Negative_Infinity: frintm(SubRegSize.Scalar, Dst, Src); break;
    case IR::Round_Positive_Infinity: frintp(SubRegSize.Scalar, Dst, Src); break;
    case IR::Round_Towards_Zero: frintz(SubRegSize.Scalar, Dst, Src); break;
    case IR::Round_Host: frinti(SubRegSize.Scalar, Dst, Src); break;
    }
  };

  // Bit of a tricky detail.
  // The upper bits of the destination comes from the first source.
  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1);
  const auto Vector2 = GetVReg(Op->Vector2);

  VFScalarUnaryOperation(IROp->Size, ElementSize, Op->ZeroUpperBits, ScalarEmit, Dst, Vector1, Vector2);
}

DEF_OP(VFCMPScalarInsert) {
  const auto Op = IROp->C<IR::IROp_VFCMPScalarInsert>();
  const auto ElementSize = Op->Header.ElementSize;
  const auto SubRegSize = ConvertSubRegSizePair248(IROp);

  const auto ZeroUpperBits = Op->ZeroUpperBits;
  const auto Is256Bit = IROp->Size == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  auto ScalarEmitEQ = [this, SubRegSize](ARMEmitter::VRegister Dst, ARMEmitter::VRegister Src1, ARMEmitter::VRegister Src2) {
    switch (SubRegSize.Scalar) {
    case ARMEmitter::ScalarRegSize::i16Bit: {
      fcmeq(Dst.H(), Src2.H(), Src1.H());
      break;
    }
    case ARMEmitter::ScalarRegSize::i32Bit:
    case ARMEmitter::ScalarRegSize::i64Bit: fcmeq(SubRegSize.Scalar, Dst, Src2, Src1); break;
    default: break;
    }
  };
  auto ScalarEmitLT = [this, SubRegSize](ARMEmitter::VRegister Dst, ARMEmitter::VRegister Src1, ARMEmitter::VRegister Src2) {
    switch (SubRegSize.Scalar) {
    case ARMEmitter::ScalarRegSize::i16Bit: {
      fcmgt(Dst.H(), Src2.H(), Src1.H());
      break;
    }
    case ARMEmitter::ScalarRegSize::i32Bit:
    case ARMEmitter::ScalarRegSize::i64Bit: fcmgt(SubRegSize.Scalar, Dst, Src2, Src1); break;
    default: break;
    }
  };
  auto ScalarEmitLE = [this, SubRegSize](ARMEmitter::VRegister Dst, ARMEmitter::VRegister Src1, ARMEmitter::VRegister Src2) {
    switch (SubRegSize.Scalar) {
    case ARMEmitter::ScalarRegSize::i16Bit: {
      fcmge(Dst.H(), Src2.H(), Src1.H());
      break;
    }
    case ARMEmitter::ScalarRegSize::i32Bit:
    case ARMEmitter::ScalarRegSize::i64Bit: fcmge(SubRegSize.Scalar, Dst, Src2, Src1); break;
    default: break;
    }
  };
  auto ScalarEmitUNO = [this, SubRegSize, ZeroUpperBits, Is256Bit](ARMEmitter::VRegister Dst, ARMEmitter::VRegister Src1,
                                                                   ARMEmitter::VRegister Src2) {
    switch (SubRegSize.Scalar) {
    case ARMEmitter::ScalarRegSize::i16Bit: {
      fcmge(VTMP1.H(), Src1.H(), Src2.H());
      fcmgt(VTMP2.H(), Src2.H(), Src1.H());
      break;
    }
    case ARMEmitter::ScalarRegSize::i32Bit:
    case ARMEmitter::ScalarRegSize::i64Bit:
      fcmge(SubRegSize.Scalar, VTMP1, Src1, Src2);
      fcmgt(SubRegSize.Scalar, VTMP2, Src2, Src1);
      break;
    default: break;
    }
    // If the destination is a temporary then it is going to do an insert after the operation.
    // This means this operation can avoid a redundant insert in this case.
    const bool DstIsTemp = Dst == VTMP1;

    // Combine results and invert directly in VTMP1.
    orr(VTMP1.D(), VTMP1.D(), VTMP2.D());
    mvn(ARMEmitter::SubRegSize::i8Bit, VTMP1.D(), VTMP1.D());

    if (!DstIsTemp) {
      // If the destination doesn't overlap VTMP1, then we need to insert the final result.
      // This only happens in the case that the host supports AFP.
      if (!ZeroUpperBits && Is256Bit) {
        constexpr auto Predicate = ARMEmitter::PReg::p0;
        ptrue(SubRegSize.Vector, Predicate, ARMEmitter::PredicatePattern::SVE_VL1);
        mov(SubRegSize.Vector, Dst.Z(), Predicate.Merging(), VTMP1.Z());
      } else {
        ins(SubRegSize.Vector, Dst.Q(), 0, VTMP1.Q(), 0);
      }
    }
  };
  auto ScalarEmitNEQ = [this, SubRegSize, ZeroUpperBits, Is256Bit](ARMEmitter::VRegister Dst, ARMEmitter::VRegister Src1,
                                                                   ARMEmitter::VRegister Src2) {
    switch (SubRegSize.Scalar) {
    case ARMEmitter::ScalarRegSize::i16Bit: {
      fcmeq(VTMP1.H(), Src2.H(), Src1.H());
      break;
    }
    case ARMEmitter::ScalarRegSize::i32Bit:
    case ARMEmitter::ScalarRegSize::i64Bit: fcmeq(SubRegSize.Scalar, VTMP1, Src2, Src1); break;
    default: break;
    }
    // If the destination is a temporary then it is going to do an insert after the operation.
    // This means this operation can avoid a redundant insert in this case.
    const bool DstIsTemp = Dst == VTMP1;

    // Invert directly in VTMP1.
    mvn(ARMEmitter::SubRegSize::i8Bit, VTMP1.D(), VTMP1.D());

    if (!DstIsTemp) {
      // If the destination doesn't overlap VTMP1, then we need to insert the final result.
      // This only happens in the case that the host supports AFP.
      if (!ZeroUpperBits && Is256Bit) {
        constexpr auto Predicate = ARMEmitter::PReg::p0;
        ptrue(SubRegSize.Vector, Predicate, ARMEmitter::PredicatePattern::SVE_VL1);
        mov(SubRegSize.Vector, Dst.Z(), Predicate.Merging(), VTMP1.Z());
      } else {
        ins(SubRegSize.Vector, Dst.Q(), 0, VTMP1.Q(), 0);
      }
    }
  };
  auto ScalarEmitORD = [this, SubRegSize, ZeroUpperBits, Is256Bit](ARMEmitter::VRegister Dst, ARMEmitter::VRegister Src1,
                                                                   ARMEmitter::VRegister Src2) {
    switch (SubRegSize.Scalar) {
    case ARMEmitter::ScalarRegSize::i16Bit: {
      fcmge(VTMP1.H(), Src1.H(), Src2.H());
      fcmgt(VTMP2.H(), Src2.H(), Src1.H());
      break;
    }
    case ARMEmitter::ScalarRegSize::i32Bit:
    case ARMEmitter::ScalarRegSize::i64Bit:
      fcmge(SubRegSize.Scalar, VTMP1, Src1, Src2);
      fcmgt(SubRegSize.Scalar, VTMP2, Src2, Src1);
      break;
    default: break;
    }
    // If the destination is a temporary then it is going to do an insert after the operation.
    // This means this operation can avoid a redundant insert in this case.
    const bool DstIsTemp = Dst == VTMP1;

    // Combine results directly in VTMP1.
    orr(VTMP1.D(), VTMP1.D(), VTMP2.D());

    if (!DstIsTemp) {
      // If the destination doesn't overlap VTMP1, then we need to insert the final result.
      // This only happens in the case that the host supports AFP.
      if (!ZeroUpperBits && Is256Bit) {
        constexpr auto Predicate = ARMEmitter::PReg::p0;
        ptrue(SubRegSize.Vector, Predicate, ARMEmitter::PredicatePattern::SVE_VL1);
        mov(SubRegSize.Vector, Dst.Z(), Predicate.Merging(), VTMP1.Z());
      } else {
        ins(SubRegSize.Vector, Dst.Q(), 0, VTMP1.Q(), 0);
      }
    }
  };

  std::array<ScalarBinaryOpCaller, 6> Funcs = {{
    ScalarEmitEQ,
    ScalarEmitLT,
    ScalarEmitLE,
    ScalarEmitUNO,
    ScalarEmitNEQ,
    ScalarEmitORD,
  }};

  // Bit of a tricky detail.
  // The upper bits of the destination comes from the first source.
  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1);
  const auto Vector2 = GetVReg(Op->Vector2);

  VFScalarOperation(IROp->Size, ElementSize, Op->ZeroUpperBits, Funcs[FEXCore::ToUnderlying(Op->Op)], Dst, Vector1, Vector2);
}

DEF_OP(VectorImm) {
  const auto Op = IROp->C<IR::IROp_VectorImm>();
  const auto OpSize = IROp->Size;

  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);
  const auto ElementSize = Op->Header.ElementSize;
  const auto SubRegSize = ConvertSubRegSize8(IROp);

  const auto Dst = GetVReg(Node);

  if (HostSupportsSVE256 && Is256Bit) {
    LOGMAN_THROW_A_FMT(Op->ShiftAmount == 0, "SVE VectorImm doesn't support a shift");
    if (ElementSize > IR::OpSize::i8Bit && (Op->Immediate & 0x80)) {
      // SVE dup uses sign extension where VectorImm wants zext
      LoadConstant(ARMEmitter::Size::i64Bit, TMP1, Op->Immediate);
      dup(SubRegSize, Dst.Z(), TMP1);
    } else {
      dup_imm(SubRegSize, Dst.Z(), static_cast<int8_t>(Op->Immediate));
    }
  } else {
    if (ElementSize == IR::OpSize::i64Bit) {
      // movi with 64bit element size doesn't do what we want here
      LoadConstant(ARMEmitter::Size::i64Bit, TMP1, static_cast<uint64_t>(Op->Immediate) << Op->ShiftAmount);
      dup(SubRegSize, Dst.Q(), TMP1.R());
    } else {
      movi(SubRegSize, Dst.Q(), Op->Immediate, Op->ShiftAmount);
    }
  }
}

DEF_OP(LoadNamedVectorConstant) {
  const auto Op = IROp->C<IR::IROp_LoadNamedVectorConstant>();
  const auto OpSize = IROp->Size;

  const auto Dst = GetVReg(Node);
  switch (Op->Constant) {
  case FEXCore::IR::NamedVectorConstant::NAMED_VECTOR_ZERO: movi(ARMEmitter::SubRegSize::i64Bit, Dst.Q(), 0); return;
  default:
    // Intentionally doing nothing.
    break;
  }

  if (HostSupportsSVE128) {
    switch (Op->Constant) {
    case FEXCore::IR::NamedVectorConstant::NAMED_VECTOR_MOVMSKPS_SHIFT: index(ARMEmitter::SubRegSize::i32Bit, Dst.Z(), 0, 1); return;
    default:
      // Intentionally doing nothing.
      break;
    }
  }
  // Load the pointer.
  auto GenerateMemOperand = [this](IR::OpSize OpSize, uint32_t NamedConstant, ARMEmitter::Register Base) {
    const auto ConstantOffset = offsetof(FEXCore::Core::CpuStateFrame, Pointers.Common.NamedVectorConstants[NamedConstant]);

    if (ConstantOffset <= 255 || // Unscaled 9-bit signed
        ((ConstantOffset & (IR::OpSizeToSize(OpSize) - 1)) == 0 &&
         FEXCore::DividePow2(ConstantOffset, IR::OpSizeToSize(OpSize)) <= 4095)) /* 12-bit unsigned scaled */ {
      return ARMEmitter::ExtendedMemOperand(Base.X(), ARMEmitter::IndexType::OFFSET, ConstantOffset);
    }

    ldr(TMP1, STATE_PTR(CpuStateFrame, Pointers.Common.NamedVectorConstantPointers[NamedConstant]));
    return ARMEmitter::ExtendedMemOperand(TMP1, ARMEmitter::IndexType::OFFSET, 0);
  };

  if (OpSize == IR::OpSize::i256Bit) {
    // Handle SVE 32-byte variant upfront.
    ldr(TMP1, STATE_PTR(CpuStateFrame, Pointers.Common.NamedVectorConstantPointers[Op->Constant]));
    ld1b<ARMEmitter::SubRegSize::i8Bit>(Dst.Z(), PRED_TMP_32B.Zeroing(), TMP1, 0);
    return;
  }

  auto MemOperand = GenerateMemOperand(OpSize, Op->Constant, STATE);
  switch (OpSize) {
  case IR::OpSize::i8Bit: ldrb(Dst, MemOperand); break;
  case IR::OpSize::i16Bit: ldrh(Dst, MemOperand); break;
  case IR::OpSize::i32Bit: ldr(Dst.S(), MemOperand); break;
  case IR::OpSize::i64Bit: ldr(Dst.D(), MemOperand); break;
  case IR::OpSize::i128Bit: ldr(Dst.Q(), MemOperand); break;
  default: LOGMAN_MSG_A_FMT("Unhandled {} size: {}", __func__, OpSize); break;
  }
}
DEF_OP(LoadNamedVectorIndexedConstant) {
  const auto Op = IROp->C<IR::IROp_LoadNamedVectorIndexedConstant>();
  const auto OpSize = IROp->Size;

  const auto Dst = GetVReg(Node);

  // Load the pointer.
  ldr(TMP1, STATE_PTR(CpuStateFrame, Pointers.Common.IndexedNamedVectorConstantPointers[Op->Constant]));

  switch (OpSize) {
  case IR::OpSize::i8Bit: ldrb(Dst, TMP1, Op->Index); break;
  case IR::OpSize::i16Bit: ldrh(Dst, TMP1, Op->Index); break;
  case IR::OpSize::i32Bit: ldr(Dst.S(), TMP1, Op->Index); break;
  case IR::OpSize::i64Bit: ldr(Dst.D(), TMP1, Op->Index); break;
  case IR::OpSize::i128Bit: ldr(Dst.Q(), TMP1, Op->Index); break;
  case IR::OpSize::i256Bit: {
    add(ARMEmitter::Size::i64Bit, TMP1, TMP1, Op->Index);
    ld1b<ARMEmitter::SubRegSize::i8Bit>(Dst.Z(), PRED_TMP_32B.Zeroing(), TMP1, 0);
    break;
  }
  default: LOGMAN_MSG_A_FMT("Unhandled {} size: {}", __func__, OpSize); break;
  }
}

DEF_OP(VMov) {
  const auto Op = IROp->C<IR::IROp_VMov>();
  const auto OpSize = IROp->Size;

  const auto Dst = GetVReg(Node);
  const auto Source = GetVReg(Op->Source);

  switch (OpSize) {
  case IR::OpSize::i8Bit: {
    movi(ARMEmitter::SubRegSize::i64Bit, VTMP1.Q(), 0);
    ins(ARMEmitter::SubRegSize::i8Bit, VTMP1, 0, Source, 0);
    mov(Dst.Q(), VTMP1.Q());
    break;
  }
  case IR::OpSize::i16Bit: {
    movi(ARMEmitter::SubRegSize::i64Bit, VTMP1.Q(), 0);
    ins(ARMEmitter::SubRegSize::i16Bit, VTMP1, 0, Source, 0);
    mov(Dst.Q(), VTMP1.Q());
    break;
  }
  case IR::OpSize::i32Bit: {
    movi(ARMEmitter::SubRegSize::i64Bit, VTMP1.Q(), 0);
    ins(ARMEmitter::SubRegSize::i32Bit, VTMP1, 0, Source, 0);
    mov(Dst.Q(), VTMP1.Q());
    break;
  }
  case IR::OpSize::i64Bit: {
    mov(Dst.D(), Source.D());
    break;
  }
  case IR::OpSize::i128Bit: {
    if (HostSupportsSVE256 || Dst.Idx() != Source.Idx()) {
      mov(Dst.Q(), Source.Q());
    }
    break;
  }
  case IR::OpSize::i256Bit: {
    // NOTE: If, in the distant future we support larger moves, or registers
    //       (*cough* AVX-512 *cough*) make sure to change this to treat
    //       256-bit moves with zero extending behavior instead of doing only
    //       a regular SVE move into a 512-bit register.
    if (Dst.Idx() != Source.Idx()) {
      mov(Dst.Z(), Source.Z());
    }
    break;
  }
  default: LOGMAN_MSG_A_FMT("Unknown Op Size: {}", OpSize); break;
  }
}

DEF_OP(VAddP) {
  const auto Op = IROp->C<IR::IROp_VAddP>();
  const auto OpSize = IROp->Size;
  const auto IsScalar = OpSize == IR::OpSize::i64Bit;
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);
  const auto SubRegSize = ConvertSubRegSize8(IROp);

  const auto Dst = GetVReg(Node);
  const auto VectorLower = GetVReg(Op->VectorLower);
  const auto VectorUpper = GetVReg(Op->VectorUpper);

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

DEF_OP(VFAddV) {
  const auto Op = IROp->C<IR::IROp_VFAddV>();
  const auto OpSize = IROp->Size;
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto ElementSize = Op->Header.ElementSize;
  const auto SubRegSize = ConvertSubRegSizePair248(IROp);

  const auto Dst = GetVReg(Node);
  const auto Vector = GetVReg(Op->Vector);

  LOGMAN_THROW_A_FMT(OpSize == IR::OpSize::i128Bit || OpSize == IR::OpSize::i256Bit, "Only AVX and SSE size supported");
  if (HostSupportsSVE256 && Is256Bit) {
    const auto Pred = PRED_TMP_32B.Merging();
    faddv(SubRegSize.Vector, Dst, Pred, Vector.Z());
  }
  if (HostSupportsSVE128) {
    const auto Pred = PRED_TMP_16B.Merging();
    faddv(SubRegSize.Vector, Dst, Pred, Vector.Z());
  } else {
    // ASIMD doesn't support faddv, need to use multiple faddp to match behaviour.
    if (ElementSize == IR::OpSize::i32Bit) {
      faddp(SubRegSize.Vector, Dst.Q(), Vector.Q(), Vector.Q());
      faddp(SubRegSize.Scalar, Dst, Dst);
    } else {
      faddp(SubRegSize.Scalar, Dst, Vector);
    }
  }
}

DEF_OP(VAddV) {
  const auto Op = IROp->C<IR::IROp_VAddV>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto SubRegSize = ConvertSubRegSizePair8(IROp);
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetVReg(Node);
  const auto Vector = GetVReg(Op->Vector);

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
    if (ElementSize == IR::OpSize::i64Bit) {
      addp(SubRegSize.Scalar, Dst, Vector);
    } else {
      addv(SubRegSize.Vector, Dst.Q(), Vector.Q());
    }
  }
}

DEF_OP(VUMinV) {
  const auto Op = IROp->C<IR::IROp_VUMinV>();
  const auto OpSize = IROp->Size;

  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto SubRegSize = ConvertSubRegSize8(IROp);

  const auto Dst = GetVReg(Node);
  const auto Vector = GetVReg(Op->Vector);

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Pred = PRED_TMP_32B;
    uminv(SubRegSize, Dst, Pred, Vector.Z());
  } else {
    // Vector
    uminv(SubRegSize, Dst.Q(), Vector.Q());
  }
}

DEF_OP(VUMaxV) {
  const auto Op = IROp->C<IR::IROp_VUMaxV>();
  const auto OpSize = IROp->Size;

  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto SubRegSize = ConvertSubRegSize8(IROp);

  const auto Dst = GetVReg(Node);
  const auto Vector = GetVReg(Op->Vector);

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Pred = PRED_TMP_32B;
    umaxv(SubRegSize, Dst, Pred, Vector.Z());
  } else {
    // Vector
    umaxv(SubRegSize, Dst.Q(), Vector.Q());
  }
}

DEF_OP(VURAvg) {
  const auto Op = IROp->C<IR::IROp_VURAvg>();
  const auto OpSize = IROp->Size;

  const auto SubRegSize = ConvertSubRegSize8(IROp);
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1);
  const auto Vector2 = GetVReg(Op->Vector2);

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Mask = PRED_TMP_32B.Merging();

    // Trivial cases where we already have source data to be averaged in
    // the destination register. We can just do the operation in place.
    if (Dst == Vector1) {
      urhadd(SubRegSize, Dst.Z(), Mask, Dst.Z(), Vector2.Z());
    } else if (Dst == Vector2) {
      urhadd(SubRegSize, Dst.Z(), Mask, Dst.Z(), Vector1.Z());
    } else {
      // SVE URHADD is a destructive operation, but we know that
      // we don't have any source/destination aliasing happening here
      // so we can safely move one of the source operands into the destination.
      movprfx(Dst.Z(), Vector1.Z());
      urhadd(SubRegSize, Dst.Z(), Mask, Dst.Z(), Vector2.Z());
    }
  } else {
    urhadd(SubRegSize, Dst.Q(), Vector1.Q(), Vector2.Q());
  }
}

DEF_OP(VFAddP) {
  const auto Op = IROp->C<IR::IROp_VFAddP>();
  const auto OpSize = IROp->Size;

  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto SubRegSize = ConvertSubRegSize248(IROp);

  const auto Dst = GetVReg(Node);
  const auto VectorLower = GetVReg(Op->VectorLower);
  const auto VectorUpper = GetVReg(Op->VectorUpper);

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

DEF_OP(VFDiv) {
  const auto Op = IROp->C<IR::IROp_VFDiv>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto SubRegSize = ConvertSubRegSize248(IROp);
  const auto IsScalar = ElementSize == OpSize;
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1);
  const auto Vector2 = GetVReg(Op->Vector2);

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Mask = PRED_TMP_32B.Merging();

    if (Dst == Vector1) {
      // Trivial case where we already have source data to be divided in the
      // destination register. We can just divide by Vector2 and be done with it.
      fdiv(SubRegSize, Dst.Z(), Mask, Dst.Z(), Vector2.Z());
    } else if (Dst == Vector2) {
      // If the destination aliases the second vector, then we need
      // to use a temp.
      movprfx(VTMP1.Z(), Vector1.Z());
      fdiv(SubRegSize, VTMP1.Z(), Mask, VTMP1.Z(), Vector2.Z());
      mov(Dst.Z(), VTMP1.Z());
    } else {
      // If no registers alias the destination, then we can move directly
      // into the destination and then divide.
      movprfx(Dst.Z(), Vector1.Z());
      fdiv(SubRegSize, Dst.Z(), Mask, Dst.Z(), Vector2.Z());
    }
  } else {
    if (IsScalar) {
      switch (ElementSize) {
      case IR::OpSize::i16Bit: {
        fdiv(Dst.H(), Vector1.H(), Vector2.H());
        break;
      }
      case IR::OpSize::i32Bit: {
        fdiv(Dst.S(), Vector1.S(), Vector2.S());
        break;
      }
      case IR::OpSize::i64Bit: {
        fdiv(Dst.D(), Vector1.D(), Vector2.D());
        break;
      }
      default: break;
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
  const auto SubRegSize = ConvertSubRegSize248(IROp);
  const auto IsScalar = ElementSize == OpSize;
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1);
  const auto Vector2 = GetVReg(Op->Vector2);

  // NOTE: We don't directly use FMIN** here for any of the implementations,
  //       because it has undesirable NaN handling behavior (it sets
  //       entries either to the incoming NaN value*, or the default NaN
  //       depending on FPCR flags set). We want behavior that sets NaN
  //       entries to zero for the comparison result.
  //
  // * - Not exactly (differs slightly with SNaNs), but close enough for the explanation
  // ** - Unless the host supports AFP.AH, which allows FMIN/FMAX to select the second source element as expected of x86.

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
    fcmgt(SubRegSize, ComparePred, Mask.Zeroing(), Vector2.Z(), Vector1.Z());
    not_(ComparePred, Mask.Zeroing(), ComparePred);

    if (Dst == Vector1) {
      // Trivial case where Vector1 is also the destination.
      // We don't need to move any data around in this case (aside from the merge).
      mov(SubRegSize, Dst.Z(), ComparePred.Merging(), Vector2.Z());
    } else {
      mov(VTMP1.Z(), Vector1.Z());
      mov(SubRegSize, VTMP1.Z(), ComparePred.Merging(), Vector2.Z());
      mov(Dst.Z(), VTMP1.Z());
    }
  } else {
    LOGMAN_THROW_A_FMT(!IsScalar, "should use VFMinScalarInsert instead");

    if (HostSupportsAFP) {
      // AFP.AH lets fmin behave like x86 min
      fmin(SubRegSize, Dst.Q(), Vector1.Q(), Vector2.Q());
      return;
    }

    if (Dst == Vector1) {
      // Destination is already Vector1, need to insert Vector2 on false.
      fcmgt(SubRegSize, VTMP1.Q(), Vector2.Q(), Vector1.Q());
      bif(Dst.Q(), Vector2.Q(), VTMP1.Q());
    } else if (Dst == Vector2) {
      // Destination is already Vector2, Invert arguments and insert Vector1 on false.
      fcmgt(SubRegSize, VTMP1.Q(), Vector1.Q(), Vector2.Q());
      bif(Dst.Q(), Vector1.Q(), VTMP1.Q());
    } else {
      // Dst is not either source, need a move.
      fcmgt(SubRegSize, VTMP1.Q(), Vector2.Q(), Vector1.Q());
      mov(Dst.Q(), Vector1.Q());
      bif(Dst.Q(), Vector2.Q(), VTMP1.Q());
    }
  }
}

DEF_OP(VFMax) {
  const auto Op = IROp->C<IR::IROp_VFMax>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto SubRegSize = ConvertSubRegSize248(IROp);
  const auto IsScalar = ElementSize == OpSize;
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1);
  const auto Vector2 = GetVReg(Op->Vector2);

  // NOTE: See VFMin implementation for reasons why we
  //       don't just use FMAX/FMIN for these implementations.

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Mask = PRED_TMP_32B;
    const auto ComparePred = ARMEmitter::PReg::p0;

    fcmgt(SubRegSize, ComparePred, Mask.Zeroing(), Vector2.Z(), Vector1.Z());

    if (Dst == Vector1) {
      // Trivial case where Vector1 is also the destination.
      // We don't need to move any data around in this case (aside from the merge).
      mov(SubRegSize, Dst.Z(), ComparePred.Merging(), Vector2.Z());
    } else {
      mov(VTMP1.Z(), Vector1.Z());
      mov(SubRegSize, VTMP1.Z(), ComparePred.Merging(), Vector2.Z());
      mov(Dst.Z(), VTMP1.Z());
    }
  } else {
    LOGMAN_THROW_A_FMT(!IsScalar, "should use VFMaxScalarInsert instead");

    if (HostSupportsAFP) {
      // AFP.AH lets fmax behave like x86 max
      fmax(SubRegSize, Dst.Q(), Vector1.Q(), Vector2.Q());
      return;
    }

    if (Dst == Vector1) {
      // Destination is already Vector1, need to insert Vector2 on true.
      fcmgt(SubRegSize, VTMP1.Q(), Vector2.Q(), Vector1.Q());
      bit(Dst.Q(), Vector2.Q(), VTMP1.Q());
    } else if (Dst == Vector2) {
      // Destination is already Vector2, Invert arguments and insert Vector1 on true.
      fcmgt(SubRegSize, VTMP1.Q(), Vector1.Q(), Vector2.Q());
      bit(Dst.Q(), Vector1.Q(), VTMP1.Q());
    } else {
      // Dst is not either source, need a move.
      fcmgt(SubRegSize, VTMP1.Q(), Vector2.Q(), Vector1.Q());
      mov(Dst.Q(), Vector1.Q());
      bit(Dst.Q(), Vector2.Q(), VTMP1.Q());
    }
  }
}

DEF_OP(VFRecp) {
  const auto Op = IROp->C<IR::IROp_VFRecp>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto SubRegSize = ConvertSubRegSizePair16(IROp);
  const auto IsScalar = Op->Header.ElementSize == OpSize;
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetVReg(Node);
  const auto Vector = GetVReg(Op->Vector);

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Pred = PRED_TMP_32B.Merging();

    if (ElementSize == IR::OpSize::i32Bit && HostSupportsRPRES) {
      // RPRES gives enough precision for this.
      frecpe(SubRegSize.Vector, Dst.Z(), Vector.Z());
      return;
    }

    fmov(SubRegSize.Vector, VTMP1.Z(), 1.0);
    fdiv(SubRegSize.Vector, VTMP1.Z(), Pred, VTMP1.Z(), Vector.Z());
    mov(Dst.Z(), VTMP1.Z());
  } else {
    if (IsScalar) {
      if (ElementSize == IR::OpSize::i32Bit && HostSupportsRPRES) {
        // RPRES gives enough precision for this.
        frecpe(SubRegSize.Scalar, Dst.S(), Vector.S());
        return;
      }

      fmov(SubRegSize.Scalar, VTMP1.Q(), 1.0f);
      switch (ElementSize) {
      case IR::OpSize::i16Bit: {
        fdiv(Dst.H(), VTMP1.H(), Vector.H());
        break;
      }
      case IR::OpSize::i32Bit: {
        fdiv(Dst.S(), VTMP1.S(), Vector.S());
        break;
      }
      case IR::OpSize::i64Bit: {
        fdiv(Dst.D(), VTMP1.D(), Vector.D());
        break;
      }
      default: {
        LOGMAN_MSG_A_FMT("Unexpected ElementSize for {}", __func__);
        FEX_UNREACHABLE;
      }
      }
    } else {
      if (ElementSize == IR::OpSize::i32Bit && HostSupportsRPRES) {
        // RPRES gives enough precision for this.
        if (OpSize == IR::OpSize::i64Bit) {
          frecpe(SubRegSize.Vector, Dst.D(), Vector.D());
        } else {
          frecpe(SubRegSize.Vector, Dst.Q(), Vector.Q());
        }
        return;
      }

      fmov(SubRegSize.Vector, VTMP1.Q(), 1.0f);
      fdiv(SubRegSize.Vector, Dst.Q(), VTMP1.Q(), Vector.Q());
    }
  }
}

DEF_OP(VFRecpPrecision) {
  const auto Op = IROp->C<IR::IROp_VFRecpPrecision>();
  const auto OpSize = IROp->Size;
  const auto ElementSize = Op->Header.ElementSize;

  LOGMAN_THROW_A_FMT((OpSize == IR::OpSize::i64Bit || OpSize == IR::OpSize::i32Bit) && ElementSize == IR::OpSize::i32Bit,
                     "Unexpected sizes for operation.", __func__);

  const auto SubRegSize = ConvertSubRegSizePair16(IROp);
  const auto IsScalar = OpSize == ElementSize;

  const auto Dst = GetVReg(Node);
  const auto Vector = GetVReg(Op->Vector);

  if (IsScalar) {
    if (ElementSize == IR::OpSize::i32Bit && HostSupportsRPRES) {
      // Not enough precision so we need to improve it with frecps
      frecpe(SubRegSize.Scalar, VTMP1.S(), Vector.S());
      frecps(SubRegSize.Scalar, VTMP2.S(), VTMP1.S(), Vector.S());
      fmul(SubRegSize.Scalar, Dst.S(), VTMP1.S(), VTMP2.S());
      return;
    }

    fmov(SubRegSize.Scalar, VTMP1.Q(), 1.0f);
    // Element size is known to be 32bits
    fdiv(Dst.S(), VTMP1.S(), Vector.S());
  } else { // Vector operation - Opsize 64bits, elementsize 32bits
    if (HostSupportsRPRES) {
      frecpe(SubRegSize.Vector, VTMP1.D(), Vector.D());
      frecps(SubRegSize.Vector, VTMP2.D(), VTMP1.D(), Vector.D());
      fmul(SubRegSize.Vector, Dst.D(), VTMP1.D(), VTMP2.D());
      return;
    }

    // No RPRES, so normal division
    fmov(SubRegSize.Vector, VTMP1.Q(), 1.0f);
    fdiv(SubRegSize.Vector, Dst.Q(), VTMP1.Q(), Vector.Q());
  }
}

DEF_OP(VFRSqrt) {
  const auto Op = IROp->C<IR::IROp_VFRSqrt>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto SubRegSize = ConvertSubRegSizePair16(IROp);
  const auto IsScalar = ElementSize == OpSize;
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetVReg(Node);
  const auto Vector = GetVReg(Op->Vector);

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Pred = PRED_TMP_32B.Merging();
    if (ElementSize == IR::OpSize::i32Bit && HostSupportsRPRES) {
      // RPRES gives enough precision for this.
      frsqrte(SubRegSize.Vector, Dst.Z(), Vector.Z());
      return;
    }

    fsqrt(SubRegSize.Vector, VTMP1.Z(), Pred, Vector.Z());
    fmov(SubRegSize.Vector, Dst.Z(), 1.0);
    fdiv(SubRegSize.Vector, Dst.Z(), Pred, Dst.Z(), VTMP1.Z());
  } else {
    if (IsScalar) {
      if (ElementSize == IR::OpSize::i32Bit && HostSupportsRPRES) {
        // RPRES gives enough precision for this.
        frsqrte(SubRegSize.Scalar, Dst.S(), Vector.S());
        return;
      }

      fmov(SubRegSize.Scalar, VTMP1.Q(), 1.0);
      switch (ElementSize) {
      case IR::OpSize::i16Bit: {
        fsqrt(VTMP2.H(), Vector.H());
        fdiv(Dst.H(), VTMP1.H(), VTMP2.H());
        break;
      }
      case IR::OpSize::i32Bit: {
        fsqrt(VTMP2.S(), Vector.S());
        fdiv(Dst.S(), VTMP1.S(), VTMP2.S());
        break;
      }
      case IR::OpSize::i64Bit: {
        fsqrt(VTMP2.D(), Vector.D());
        fdiv(Dst.D(), VTMP1.D(), VTMP2.D());
        break;
      }
      default: break;
      }
    } else {
      if (ElementSize == IR::OpSize::i32Bit && HostSupportsRPRES) {
        // RPRES gives enough precision for this.
        if (OpSize == IR::OpSize::i64Bit) {
          frsqrte(SubRegSize.Vector, Dst.D(), Vector.D());
        } else {
          frsqrte(SubRegSize.Vector, Dst.Q(), Vector.Q());
        }
        return;
      }

      fmov(SubRegSize.Vector, VTMP1.Q(), 1.0);
      fsqrt(SubRegSize.Vector, VTMP2.Q(), Vector.Q());
      fdiv(SubRegSize.Vector, Dst.Q(), VTMP1.Q(), VTMP2.Q());
    }
  }
}

DEF_OP(VFRSqrtPrecision) {
  const auto Op = IROp->C<IR::IROp_VFRSqrtPrecision>();
  const auto OpSize = IROp->Size;
  const auto ElementSize = Op->Header.ElementSize;


  LOGMAN_THROW_A_FMT((OpSize == IR::OpSize::i64Bit || OpSize == IR::OpSize::i32Bit) && ElementSize == IR::OpSize::i32Bit,
                     "Unexpected sizes for operation.", __func__);

  const auto SubRegSize = ConvertSubRegSizePair16(IROp);
  const auto IsScalar = ElementSize == OpSize;

  const auto Dst = GetVReg(Node);
  const auto Vector = GetVReg(Op->Vector);

  if (IsScalar) {
    if (HostSupportsRPRES) {
      frsqrte(SubRegSize.Scalar, VTMP1.S(), Vector.S());
      // Improve initial estimate which is not good enough.
      fmul(SubRegSize.Scalar, VTMP2.S(), VTMP1.S(), VTMP1.S());
      frsqrts(SubRegSize.Scalar, VTMP2.S(), VTMP2.S(), Vector.S());
      fmul(SubRegSize.Scalar, Dst.S(), VTMP1.S(), VTMP2.S());
      return;
    }

    fmov(SubRegSize.Scalar, VTMP1.Q(), 1.0);
    // element size is known to be 32bits
    fsqrt(VTMP2.S(), Vector.S());
    fdiv(Dst.S(), VTMP1.S(), VTMP2.S());
  } else {
    if (HostSupportsRPRES) {
      frsqrte(SubRegSize.Vector, VTMP1.D(), Vector.D());
      // Improve initial estimate which is not good enough.
      fmul(SubRegSize.Vector, VTMP2.D(), VTMP1.D(), VTMP1.D());
      frsqrts(SubRegSize.Vector, VTMP2.D(), VTMP2.D(), Vector.D());
      fmul(SubRegSize.Vector, Dst.D(), VTMP1.D(), VTMP2.D());
      return;
    }
    fmov(SubRegSize.Vector, VTMP1.Q(), 1.0);
    fsqrt(SubRegSize.Vector, VTMP2.Q(), Vector.Q());
    fdiv(SubRegSize.Vector, Dst.Q(), VTMP1.Q(), VTMP2.Q());
  }
}

DEF_OP(VNot) {
  const auto Op = IROp->C<IR::IROp_VNot>();
  const auto OpSize = IROp->Size;
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetVReg(Node);
  const auto Vector = GetVReg(Op->Vector);

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
  const auto SubRegSize = ConvertSubRegSize16(IROp);
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1);
  const auto Vector2 = GetVReg(Op->Vector2);

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Pred = PRED_TMP_32B.Merging();

    // In any case where the destination aliases one of the source vectors
    // then we can just perform the UMIN in place.
    if (Dst == Vector1) {
      umin(SubRegSize, Dst.Z(), Pred, Dst.Z(), Vector2.Z());
    } else if (Dst == Vector2) {
      umin(SubRegSize, Dst.Z(), Pred, Dst.Z(), Vector1.Z());
    } else {
      // SVE UMIN is a destructive operation, but we know nothing is
      // aliasing the destination by this point, so we can move into
      // the destination without needing a temporary.
      movprfx(Dst.Z(), Vector1.Z());
      umin(SubRegSize, Dst.Z(), Pred, Dst.Z(), Vector2.Z());
    }
  } else {
    switch (ElementSize) {
    case IR::OpSize::i8Bit:
    case IR::OpSize::i16Bit:
    case IR::OpSize::i32Bit: {
      umin(SubRegSize, Dst.Q(), Vector1.Q(), Vector2.Q());
      break;
    }
    case IR::OpSize::i64Bit: {
      cmhi(SubRegSize, VTMP1.Q(), Vector2.Q(), Vector1.Q());
      mov(VTMP2.Q(), Vector1.Q());
      bif(VTMP2.Q(), Vector2.Q(), VTMP1.Q());
      mov(Dst.Q(), VTMP2.Q());
      break;
    }
    default: break;
    }
  }
}

DEF_OP(VSMin) {
  const auto Op = IROp->C<IR::IROp_VSMin>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto SubRegSize = ConvertSubRegSize16(IROp);
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1);
  const auto Vector2 = GetVReg(Op->Vector2);

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Pred = PRED_TMP_32B.Merging();

    // In any case where the destination aliases one of the source vectors
    // then we can just perform the SMIN in place.
    if (Dst == Vector1) {
      smin(SubRegSize, Dst.Z(), Pred, Dst.Z(), Vector2.Z());
    } else if (Dst == Vector2) {
      smin(SubRegSize, Dst.Z(), Pred, Dst.Z(), Vector1.Z());
    } else {
      // SVE SMIN is a destructive operation, but we know nothing is
      // aliasing the destination by this point, so we can move into
      // the destination without needing a temporary.
      movprfx(Dst.Z(), Vector1.Z());
      smin(SubRegSize, Dst.Z(), Pred, Dst.Z(), Vector2.Z());
    }
  } else {
    switch (ElementSize) {
    case IR::OpSize::i8Bit:
    case IR::OpSize::i16Bit:
    case IR::OpSize::i32Bit: {
      smin(SubRegSize, Dst.Q(), Vector1.Q(), Vector2.Q());
      break;
    }
    case IR::OpSize::i64Bit: {
      cmgt(SubRegSize, VTMP1.Q(), Vector1.Q(), Vector2.Q());
      mov(VTMP2.Q(), Vector1.Q());
      bif(VTMP2.Q(), Vector2.Q(), VTMP1.Q());
      mov(Dst.Q(), VTMP2.Q());
      break;
    }
    default: break;
    }
  }
}

DEF_OP(VUMax) {
  const auto Op = IROp->C<IR::IROp_VUMax>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto SubRegSize = ConvertSubRegSize16(IROp);
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1);
  const auto Vector2 = GetVReg(Op->Vector2);

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Pred = PRED_TMP_32B.Merging();

    // In any case where the destination aliases one of the source vectors
    // then we can just perform the UMAX in place.
    if (Dst == Vector1) {
      umax(SubRegSize, Dst.Z(), Pred, Dst.Z(), Vector2.Z());
    } else if (Dst == Vector2) {
      umax(SubRegSize, Dst.Z(), Pred, Dst.Z(), Vector1.Z());
    } else {
      // SVE UMAX is a destructive operation, but we know nothing is
      // aliasing the destination by this point, so we can move into
      // the destination without needing a temporary.
      movprfx(Dst.Z(), Vector1.Z());
      umax(SubRegSize, Dst.Z(), Pred, Dst.Z(), Vector2.Z());
    }
  } else {
    switch (ElementSize) {
    case IR::OpSize::i8Bit:
    case IR::OpSize::i16Bit:
    case IR::OpSize::i32Bit: {
      umax(SubRegSize, Dst.Q(), Vector1.Q(), Vector2.Q());
      break;
    }
    case IR::OpSize::i64Bit: {
      cmhi(SubRegSize, VTMP1.Q(), Vector2.Q(), Vector1.Q());
      mov(VTMP2.Q(), Vector1.Q());
      bif(VTMP2.Q(), Vector2.Q(), VTMP1.Q());
      mov(Dst.Q(), VTMP2.Q());
      break;
    }
    default: break;
    }
  }
}

DEF_OP(VSMax) {
  const auto Op = IROp->C<IR::IROp_VSMax>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto SubRegSize = ConvertSubRegSize16(IROp);
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1);
  const auto Vector2 = GetVReg(Op->Vector2);

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Pred = PRED_TMP_32B.Merging();

    // In any case where the destination aliases one of the source vectors
    // then we can just perform the SMAX in place.
    if (Dst == Vector1) {
      smax(SubRegSize, Dst.Z(), Pred, Dst.Z(), Vector2.Z());
    } else if (Dst == Vector2) {
      smax(SubRegSize, Dst.Z(), Pred, Dst.Z(), Vector1.Z());
    } else {
      // SVE SMAX is a destructive operation, but we know nothing is
      // aliasing the destination by this point, so we can move into
      // the destination without needing a temporary.
      movprfx(Dst.Z(), Vector1.Z());
      smax(SubRegSize, Dst.Z(), Pred, Dst.Z(), Vector2.Z());
    }
  } else {
    switch (ElementSize) {
    case IR::OpSize::i8Bit:
    case IR::OpSize::i16Bit:
    case IR::OpSize::i32Bit: {
      smax(SubRegSize, Dst.Q(), Vector1.Q(), Vector2.Q());
      break;
    }
    case IR::OpSize::i64Bit: {
      cmgt(SubRegSize, VTMP1.Q(), Vector2.Q(), Vector1.Q());
      mov(VTMP2.Q(), Vector1.Q());
      bif(VTMP2.Q(), Vector2.Q(), VTMP1.Q());
      mov(Dst.Q(), VTMP2.Q());
      break;
    }
    default: break;
    }
  }
}

DEF_OP(VBSL) {
  const auto Op = IROp->C<IR::IROp_VBSL>();
  const auto OpSize = IROp->Size;
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Is128Bit = OpSize == IR::OpSize::i128Bit;

  const auto Dst = GetVReg(Node);
  const auto VectorFalse = GetVReg(Op->VectorFalse);
  const auto VectorTrue = GetVReg(Op->VectorTrue);
  const auto VectorMask = GetVReg(Op->VectorMask);

  if (HostSupportsSVE256 && Is256Bit) {
    // NOTE: Slight parameter difference from ASIMD
    //       ASIMD -> BSL Mask, True, False
    //       SVE   -> BSL True, True, False, Mask
    //       ASIMD -> BIT True, False, Mask
    //       ASIMD -> BIF False, True, Mask
    if (Dst == VectorTrue) {
      // Trivial case where we can perform the operation in place.
      bsl(Dst.Z(), Dst.Z(), VectorFalse.Z(), VectorMask.Z());
    } else {
      movprfx(VTMP1.Z(), VectorTrue.Z());
      bsl(VTMP1.Z(), VTMP1.Z(), VectorFalse.Z(), VectorMask.Z());
      mov(Dst.Z(), VTMP1.Z());
    }
  } else if (!HostSupportsSVE256 && HostSupportsSVE128 && Is128Bit && Dst != VectorFalse && Dst != VectorTrue && Dst != VectorMask) {
    // Needs to move but SVE movprfx+bsl is slightly more efficient than ASIMD mov+bsl on CPUs that support
    // movprfx fusion and NOT zero-cycle vector register moves.
    movprfx(Dst.Z(), VectorTrue.Z());
    bsl(Dst.Z(), Dst.Z(), VectorFalse.Z(), VectorMask.Z());
  } else {
    if (VectorMask == Dst) {
      // Can use BSL without any moves.
      if (OpSize == IR::OpSize::i64Bit) {
        bsl(Dst.D(), VectorTrue.D(), VectorFalse.D());
      } else {
        bsl(Dst.Q(), VectorTrue.Q(), VectorFalse.Q());
      }
    } else if (VectorTrue == Dst) {
      // Can use BIF without any moves.
      if (OpSize == IR::OpSize::i64Bit) {
        bif(Dst.D(), VectorFalse.D(), VectorMask.D());
      } else {
        bif(Dst.Q(), VectorFalse.Q(), VectorMask.Q());
      }
    } else if (VectorFalse == Dst) {
      // Can use BIT without any moves.
      if (OpSize == IR::OpSize::i64Bit) {
        bit(Dst.D(), VectorTrue.D(), VectorMask.D());
      } else {
        bit(Dst.Q(), VectorTrue.Q(), VectorMask.Q());
      }
    } else {
      // Needs moves.
      if (OpSize == IR::OpSize::i64Bit) {
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
  const auto SubRegSize = ConvertSubRegSizePair16(IROp);
  const auto IsScalar = ElementSize == OpSize;
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1);
  const auto Vector2 = GetVReg(Op->Vector2);

  if (HostSupportsSVE256 && Is256Bit) {
    // FIXME: We should rework this op to avoid the NZCV spill/fill dance.
    mrs(TMP1, ARMEmitter::SystemRegister::NZCV);

    const auto Mask = PRED_TMP_32B.Zeroing();
    const auto ComparePred = ARMEmitter::PReg::p0;

    // General idea is to compare for equality, not the equal vals
    // from one of the registers, then or both together to make the
    // relevant equal entries all 1s.
    cmpeq(SubRegSize.Vector, ComparePred, Mask, Vector1.Z(), Vector2.Z());
    not_(SubRegSize.Vector, VTMP1.Z(), ComparePred.Merging(), Vector1.Z());
    movprfx(SubRegSize.Vector, Dst.Z(), ComparePred.Zeroing(), Vector1.Z());
    orr(SubRegSize.Vector, Dst.Z(), ComparePred.Merging(), Dst.Z(), VTMP1.Z());

    // Restore NZCV
    msr(ARMEmitter::SystemRegister::NZCV, TMP1);
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
  const auto SubRegSize = ConvertSubRegSizePair16(IROp);
  const auto IsScalar = ElementSize == OpSize;
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetVReg(Node);
  const auto Vector = GetVReg(Op->Vector);

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Mask = PRED_TMP_32B.Zeroing();
    const auto ComparePred = ARMEmitter::PReg::p0;

    // FIXME: We should rework this op to avoid the NZCV spill/fill dance.
    mrs(TMP1, ARMEmitter::SystemRegister::NZCV);

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

    // Restore NZCV
    msr(ARMEmitter::SystemRegister::NZCV, TMP1);
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
  const auto SubRegSize = ConvertSubRegSizePair16(IROp);
  const auto IsScalar = ElementSize == OpSize;
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1);
  const auto Vector2 = GetVReg(Op->Vector2);

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Mask = PRED_TMP_32B.Zeroing();
    const auto ComparePred = ARMEmitter::PReg::p0;

    // FIXME: We should rework this op to avoid the NZCV spill/fill dance.
    mrs(TMP1, ARMEmitter::SystemRegister::NZCV);

    // General idea is to compare for greater-than, bitwise NOT
    // the valid values, then ORR the NOTed values with the original
    // values to form entries that are all 1s.
    cmpgt(SubRegSize.Vector, ComparePred, Mask, Vector1.Z(), Vector2.Z());
    not_(SubRegSize.Vector, VTMP1.Z(), ComparePred.Merging(), Vector1.Z());
    movprfx(SubRegSize.Vector, Dst.Z(), ComparePred.Zeroing(), Vector1.Z());
    orr(SubRegSize.Vector, Dst.Z(), ComparePred.Merging(), Dst.Z(), VTMP1.Z());

    // Restore NZCV
    msr(ARMEmitter::SystemRegister::NZCV, TMP1);
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
  const auto SubRegSize = ConvertSubRegSizePair16(IROp);
  const auto IsScalar = ElementSize == OpSize;
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetVReg(Node);
  const auto Vector = GetVReg(Op->Vector);

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Mask = PRED_TMP_32B.Zeroing();
    const auto ComparePred = ARMEmitter::PReg::p0;

    // FIXME: We should rework this op to avoid the NZCV spill/fill dance.
    mrs(TMP1, ARMEmitter::SystemRegister::NZCV);

    // Ensure no junk is in the temp (important for ensuring
    // non greater-than values remain as zero).
    mov_imm(ARMEmitter::SubRegSize::i64Bit, VTMP1.Z(), 0);
    cmpgt(SubRegSize.Vector, ComparePred, Mask, Vector.Z(), 0);
    not_(SubRegSize.Vector, VTMP1.Z(), ComparePred.Merging(), Vector.Z());
    orr(SubRegSize.Vector, VTMP1.Z(), ComparePred.Merging(), VTMP1.Z(), Vector.Z());
    mov(Dst.Z(), VTMP1.Z());

    // Restore NZCV
    msr(ARMEmitter::SystemRegister::NZCV, TMP1);
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
  const auto SubRegSize = ConvertSubRegSizePair16(IROp);
  const auto IsScalar = ElementSize == OpSize;
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetVReg(Node);
  const auto Vector = GetVReg(Op->Vector);

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Mask = PRED_TMP_32B.Zeroing();
    const auto ComparePred = ARMEmitter::PReg::p0;

    // FIXME: We should rework this op to avoid the NZCV spill/fill dance.
    mrs(TMP1, ARMEmitter::SystemRegister::NZCV);

    // Ensure no junk is in the temp (important for ensuring
    // non less-than values remain as zero).
    mov_imm(ARMEmitter::SubRegSize::i64Bit, VTMP1.Z(), 0);
    cmplt(SubRegSize.Vector, ComparePred, Mask, Vector.Z(), 0);
    not_(SubRegSize.Vector, VTMP1.Z(), ComparePred.Merging(), Vector.Z());
    orr(SubRegSize.Vector, VTMP1.Z(), ComparePred.Merging(), VTMP1.Z(), Vector.Z());
    mov(Dst.Z(), VTMP1.Z());

    // Restore NZCV
    msr(ARMEmitter::SystemRegister::NZCV, TMP1);
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
  const auto SubRegSize = ConvertSubRegSizePair248(IROp);
  const auto IsScalar = ElementSize == OpSize;
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1);
  const auto Vector2 = GetVReg(Op->Vector2);

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
      case IR::OpSize::i16Bit: {
        fcmeq(Dst.H(), Vector1.H(), Vector2.H());
        break;
      }
      case IR::OpSize::i32Bit:
      case IR::OpSize::i64Bit: fcmeq(SubRegSize.Scalar, Dst, Vector1, Vector2); break;
      default: break;
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
  const auto SubRegSize = ConvertSubRegSizePair248(IROp);
  const auto IsScalar = ElementSize == OpSize;
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1);
  const auto Vector2 = GetVReg(Op->Vector2);

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
      case IR::OpSize::i16Bit: {
        fcmeq(Dst.H(), Vector1.H(), Vector2.H());
        break;
      }
      case IR::OpSize::i32Bit:
      case IR::OpSize::i64Bit: fcmeq(SubRegSize.Scalar, Dst, Vector1, Vector2); break;
      default: break;
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
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto SubRegSize = ConvertSubRegSizePair248(IROp);
  const auto IsScalar = ElementSize == OpSize;
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1);
  const auto Vector2 = GetVReg(Op->Vector2);

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
      case IR::OpSize::i16Bit: {
        fcmgt(Dst.H(), Vector2.H(), Vector1.H());
        break;
      }
      case IR::OpSize::i32Bit:
      case IR::OpSize::i64Bit: fcmgt(SubRegSize.Scalar, Dst, Vector2, Vector1); break;
      default: break;
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
  const auto SubRegSize = ConvertSubRegSizePair248(IROp);
  const auto IsScalar = ElementSize == OpSize;
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1);
  const auto Vector2 = GetVReg(Op->Vector2);

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
      case IR::OpSize::i16Bit: {
        fcmgt(Dst.H(), Vector1.H(), Vector2.H());
        break;
      }
      case IR::OpSize::i32Bit:
      case IR::OpSize::i64Bit: fcmgt(SubRegSize.Scalar, Dst, Vector1, Vector2); break;
      default: break;
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
  const auto SubRegSize = ConvertSubRegSizePair248(IROp);
  const auto IsScalar = ElementSize == OpSize;
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1);
  const auto Vector2 = GetVReg(Op->Vector2);

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
      case IR::OpSize::i16Bit: {
        fcmge(Dst.H(), Vector2.H(), Vector1.H());
        break;
      }
      case IR::OpSize::i32Bit:
      case IR::OpSize::i64Bit: fcmge(SubRegSize.Scalar, Dst, Vector2, Vector1); break;
      default: break;
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
  const auto SubRegSize = ConvertSubRegSizePair248(IROp);
  const auto IsScalar = ElementSize == OpSize;
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1);
  const auto Vector2 = GetVReg(Op->Vector2);

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
      case IR::OpSize::i16Bit: {
        fcmge(VTMP1.H(), Vector1.H(), Vector2.H());
        fcmgt(VTMP2.H(), Vector2.H(), Vector1.H());
        orr(Dst.D(), VTMP1.D(), VTMP2.D());
        break;
      }
      case IR::OpSize::i32Bit:
      case IR::OpSize::i64Bit:
        fcmge(SubRegSize.Scalar, VTMP1, Vector1, Vector2);
        fcmgt(SubRegSize.Scalar, VTMP2, Vector2, Vector1);
        orr(Dst.D(), VTMP1.D(), VTMP2.D());
        break;
      default: break;
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
  const auto SubRegSize = ConvertSubRegSizePair248(IROp);
  const auto IsScalar = ElementSize == OpSize;
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1);
  const auto Vector2 = GetVReg(Op->Vector2);

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
      case IR::OpSize::i16Bit: {
        fcmge(VTMP1.H(), Vector1.H(), Vector2.H());
        fcmgt(VTMP2.H(), Vector2.H(), Vector1.H());
        orr(Dst.D(), VTMP1.D(), VTMP2.D());
        mvn(ARMEmitter::SubRegSize::i8Bit, Dst.D(), Dst.D());
        break;
      }
      case IR::OpSize::i32Bit:
      case IR::OpSize::i64Bit:
        fcmge(SubRegSize.Scalar, VTMP1, Vector1, Vector2);
        fcmgt(SubRegSize.Scalar, VTMP2, Vector2, Vector1);
        orr(Dst.D(), VTMP1.D(), VTMP2.D());
        mvn(ARMEmitter::SubRegSize::i8Bit, Dst.D(), Dst.D());
        break;
      default: break;
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
  const auto SubRegSize = ConvertSubRegSize8(IROp);
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto MaxShift = IR::OpSizeAsBits(ElementSize);

  const auto Dst = GetVReg(Node);
  auto ShiftVector = GetVReg(Op->ShiftVector);
  const auto Vector = GetVReg(Op->Vector);
  const auto RangeCheck = Op->RangeCheck;

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Mask = PRED_TMP_32B.Merging();

    if (RangeCheck) {
      dup_imm(SubRegSize, VTMP2.Z(), MaxShift);
      umin(SubRegSize, VTMP2.Z(), Mask, VTMP2.Z(), ShiftVector.Z());
      ShiftVector = VTMP2;
    }

    if (Dst == ShiftVector) {
      // If destination aliases the shift vector then we need to move it temporarily.
      mov(VTMP2.Z(), ShiftVector.Z());
      ShiftVector = VTMP2;
    }

    // If Dst aliases Vector, then we can skip the move.
    if (Dst != Vector) {
      movprfx(Dst.Z(), Vector.Z());
    }
    lsl(SubRegSize, Dst.Z(), Mask, Dst.Z(), ShiftVector.Z());
  } else {
    if (RangeCheck) {
      if (ElementSize < IR::OpSize::i64Bit) {
        movi(SubRegSize, VTMP1.Q(), MaxShift);
        umin(SubRegSize, VTMP1.Q(), VTMP1.Q(), ShiftVector.Q());
      } else {
        LoadConstant(ARMEmitter::Size::i64Bit, TMP1, MaxShift);
        dup(SubRegSize, VTMP1.Q(), TMP1.R());

        // UMIN is silly on Adv.SIMD and doesn't have a variant that handles 64-bit elements
        cmhi(SubRegSize, VTMP2.Q(), ShiftVector.Q(), VTMP1.Q());
        bif(VTMP1.Q(), ShiftVector.Q(), VTMP2.Q());
      }
      ShiftVector = VTMP1;
    }

    ushl(SubRegSize, Dst.Q(), Vector.Q(), ShiftVector.Q());
  }
}

DEF_OP(VUShr) {
  const auto Op = IROp->C<IR::IROp_VUShr>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = IROp->ElementSize;
  const auto SubRegSize = ConvertSubRegSize8(IROp);
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto MaxShift = IR::OpSizeAsBits(ElementSize);

  const auto Dst = GetVReg(Node);
  auto ShiftVector = GetVReg(Op->ShiftVector);
  const auto Vector = GetVReg(Op->Vector);
  const auto RangeCheck = Op->RangeCheck;

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Mask = PRED_TMP_32B.Merging();

    if (RangeCheck) {
      dup_imm(SubRegSize, VTMP2.Z(), MaxShift);
      umin(SubRegSize, VTMP2.Z(), Mask, VTMP2.Z(), ShiftVector.Z());
      ShiftVector = VTMP2;
    }

    if (Dst == ShiftVector) {
      // If destination aliases the shift vector then we need to move it temporarily.
      mov(VTMP2.Z(), ShiftVector.Z());
      ShiftVector = VTMP2;
    }

    // If Dst aliases Vector, then we can skip the move.
    if (Dst != Vector) {
      movprfx(Dst.Z(), Vector.Z());
    }
    lsr(SubRegSize, Dst.Z(), Mask, Dst.Z(), ShiftVector.Z());
  } else {
    if (RangeCheck) {
      if (ElementSize < IR::OpSize::i64Bit) {
        movi(SubRegSize, VTMP1.Q(), MaxShift);
        umin(SubRegSize, VTMP1.Q(), VTMP1.Q(), ShiftVector.Q());
      } else {
        LoadConstant(ARMEmitter::Size::i64Bit, TMP1, MaxShift);
        dup(SubRegSize, VTMP1.Q(), TMP1.R());

        // UMIN is silly on Adv.SIMD and doesn't have a variant that handles 64-bit elements
        cmhi(SubRegSize, VTMP2.Q(), ShiftVector.Q(), VTMP1.Q());
        bif(VTMP1.Q(), ShiftVector.Q(), VTMP2.Q());
      }
      ShiftVector = VTMP1;
    }

    // Need to invert shift values to perform a right shift with USHL
    // (USHR only has an immediate variant).
    neg(SubRegSize, VTMP1.Q(), ShiftVector.Q());
    ushl(SubRegSize, Dst.Q(), Vector.Q(), VTMP1.Q());
  }
}

DEF_OP(VSShr) {
  const auto Op = IROp->C<IR::IROp_VSShr>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = IROp->ElementSize;
  const auto SubRegSize = ConvertSubRegSize8(IROp);
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto MaxShift = IR::OpSizeAsBits(ElementSize) - 1;
  const auto RangeCheck = Op->RangeCheck;

  const auto Dst = GetVReg(Node);
  auto ShiftVector = GetVReg(Op->ShiftVector);
  const auto Vector = GetVReg(Op->Vector);

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Mask = PRED_TMP_32B.Merging();

    if (RangeCheck) {
      dup_imm(SubRegSize, VTMP1.Z(), MaxShift);
      umin(SubRegSize, VTMP1.Z(), Mask, VTMP1.Z(), ShiftVector.Z());
      ShiftVector = VTMP1;
    }

    if (Dst == ShiftVector) {
      // If destination aliases the shift vector then we need to move it temporarily.
      mov(VTMP1.Z(), ShiftVector.Z());
      ShiftVector = VTMP1;
    }

    // If Dst aliases Vector, then we can skip the move.
    if (Dst != Vector) {
      movprfx(Dst.Z(), Vector.Z());
    }
    asr(SubRegSize, Dst.Z(), Mask, Dst.Z(), ShiftVector.Z());
  } else {
    if (RangeCheck) {
      if (ElementSize < IR::OpSize::i64Bit) {
        movi(SubRegSize, VTMP1.Q(), MaxShift);
        umin(SubRegSize, VTMP1.Q(), VTMP1.Q(), ShiftVector.Q());
      } else {
        LoadConstant(ARMEmitter::Size::i64Bit, TMP1, MaxShift);
        dup(SubRegSize, VTMP1.Q(), TMP1.R());

        // UMIN is silly on Adv.SIMD and doesn't have a variant that handles 64-bit elements
        cmhi(SubRegSize, VTMP2.Q(), ShiftVector.Q(), VTMP1.Q());
        bif(VTMP1.Q(), ShiftVector.Q(), VTMP2.Q());
      }
      ShiftVector = VTMP1;
    }

    // Need to invert shift values to perform a right shift with SSHL
    // (SSHR only has an immediate variant).
    neg(SubRegSize, VTMP1.Q(), ShiftVector.Q());
    sshl(SubRegSize, Dst.Q(), Vector.Q(), VTMP1.Q());
  }
}

DEF_OP(VUShlS) {
  const auto Op = IROp->C<IR::IROp_VUShlS>();
  const auto OpSize = IROp->Size;

  const auto SubRegSize = ConvertSubRegSize16(IROp);
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetVReg(Node);
  const auto ShiftScalar = GetVReg(Op->ShiftScalar);
  const auto Vector = GetVReg(Op->Vector);

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Mask = PRED_TMP_32B.Merging();

    // NOTE: SVE LSL is a destructive operation, so we need to
    //       move the vector into the destination if they don't
    //       already alias.
    dup(SubRegSize, VTMP1.Z(), ShiftScalar.Z(), 0);
    if (Dst != Vector) {
      movprfx(Dst.Z(), Vector.Z());
    }
    lsl(SubRegSize, Dst.Z(), Mask, Dst.Z(), VTMP1.Z());
  } else {
    dup(SubRegSize, VTMP1.Q(), ShiftScalar.Q(), 0);
    ushl(SubRegSize, Dst.Q(), Vector.Q(), VTMP1.Q());
  }
}

DEF_OP(VUShrS) {
  const auto Op = IROp->C<IR::IROp_VUShrS>();
  const auto OpSize = IROp->Size;

  const auto SubRegSize = ConvertSubRegSize16(IROp);
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetVReg(Node);
  const auto ShiftScalar = GetVReg(Op->ShiftScalar);
  const auto Vector = GetVReg(Op->Vector);

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Mask = PRED_TMP_32B.Merging();

    // NOTE: SVE LSR is a destructive operation, so we need to
    //       move the vector into the destination if they don't
    //       already alias.
    dup(SubRegSize, VTMP1.Z(), ShiftScalar.Z(), 0);
    if (Dst != Vector) {
      movprfx(Dst.Z(), Vector.Z());
    }
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
  const auto SubRegSize = ConvertSubRegSize8(IROp);
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetVReg(Node);
  const auto ShiftScalar = GetVReg(Op->ShiftScalar);
  const auto Vector = GetVReg(Op->Vector);

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Mask = PRED_TMP_32B.Merging();

    dup(ARMEmitter::SubRegSize::i64Bit, VTMP1.Z(), ShiftScalar.Z(), 0);
    if (Dst != Vector) {
      // NOTE: SVE LSR is a destructive operation.
      movprfx(Dst.Z(), Vector.Z());
    }
    if (ElementSize == IR::OpSize::i64Bit) {
      lsr(SubRegSize, Dst.Z(), Mask, Dst.Z(), VTMP1.Z());
    } else {
      lsr_wide(SubRegSize, Dst.Z(), Mask, Dst.Z(), VTMP1.Z());
    }
  } else if (HostSupportsSVE128) {
    const auto Mask = PRED_TMP_16B.Merging();

    auto ShiftRegister = ShiftScalar;
    if (OpSize > IR::OpSize::i64Bit) {
      // SVE wide shifts don't need to duplicate the low bits unless the OpSize is 16-bytes
      // Slightly more optimal for 8-byte opsize.
      dup(ARMEmitter::SubRegSize::i64Bit, VTMP1.Z(), ShiftScalar.Z(), 0);
      ShiftRegister = VTMP1;
    }

    if (Dst == ShiftRegister) {
      // If destination aliases the shift vector then we need to move it temporarily.
      mov(VTMP1.Z(), ShiftRegister.Z());
      ShiftRegister = VTMP1;
    }

    if (Dst != Vector) {
      // NOTE: SVE LSR is a destructive operation.
      movprfx(Dst.Z(), Vector.Z());
    }
    if (ElementSize == IR::OpSize::i64Bit) {
      lsr(SubRegSize, Dst.Z(), Mask, Dst.Z(), ShiftRegister.Z());
    } else {
      lsr_wide(SubRegSize, Dst.Z(), Mask, Dst.Z(), ShiftRegister.Z());
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
  const auto SubRegSize = ConvertSubRegSize8(IROp);
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetVReg(Node);
  const auto ShiftScalar = GetVReg(Op->ShiftScalar);
  const auto Vector = GetVReg(Op->Vector);

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Mask = PRED_TMP_32B.Merging();

    dup(ARMEmitter::SubRegSize::i64Bit, VTMP1.Z(), ShiftScalar.Z(), 0);
    if (Dst != Vector) {
      // NOTE: SVE LSR is a destructive operation.
      movprfx(Dst.Z(), Vector.Z());
    }
    if (ElementSize == IR::OpSize::i64Bit) {
      asr(SubRegSize, Dst.Z(), Mask, Dst.Z(), VTMP1.Z());
    } else {
      asr_wide(SubRegSize, Dst.Z(), Mask, Dst.Z(), VTMP1.Z());
    }
  } else if (HostSupportsSVE128) {
    const auto Mask = PRED_TMP_16B.Merging();

    auto ShiftRegister = ShiftScalar;
    if (OpSize > IR::OpSize::i64Bit) {
      // SVE wide shifts don't need to duplicate the low bits unless the OpSize is 16-bytes
      // Slightly more optimal for 8-byte opsize.
      dup(ARMEmitter::SubRegSize::i64Bit, VTMP1.Z(), ShiftScalar.Z(), 0);
      ShiftRegister = VTMP1;
    }

    if (Dst == ShiftRegister) {
      // If destination aliases the shift vector then we need to move it temporarily.
      mov(VTMP1.Z(), ShiftRegister.Z());
      ShiftRegister = VTMP1;
    }

    if (Dst != Vector) {
      // NOTE: SVE LSR is a destructive operation.
      movprfx(Dst.Z(), Vector.Z());
    }
    if (ElementSize == IR::OpSize::i64Bit) {
      asr(SubRegSize, Dst.Z(), Mask, Dst.Z(), ShiftRegister.Z());
    } else {
      asr_wide(SubRegSize, Dst.Z(), Mask, Dst.Z(), ShiftRegister.Z());
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
  const auto SubRegSize = ConvertSubRegSize8(IROp);
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetVReg(Node);
  const auto ShiftScalar = GetVReg(Op->ShiftScalar);
  const auto Vector = GetVReg(Op->Vector);

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Mask = PRED_TMP_32B.Merging();

    dup(ARMEmitter::SubRegSize::i64Bit, VTMP1.Z(), ShiftScalar.Z(), 0);
    if (Dst != Vector) {
      // NOTE: SVE LSR is a destructive operation.
      movprfx(Dst.Z(), Vector.Z());
    }
    if (ElementSize == IR::OpSize::i64Bit) {
      lsl(SubRegSize, Dst.Z(), Mask, Dst.Z(), VTMP1.Z());
    } else {
      lsl_wide(SubRegSize, Dst.Z(), Mask, Dst.Z(), VTMP1.Z());
    }
  } else if (HostSupportsSVE128) {
    const auto Mask = PRED_TMP_16B.Merging();

    auto ShiftRegister = ShiftScalar;
    if (OpSize > IR::OpSize::i64Bit) {
      // SVE wide shifts don't need to duplicate the low bits unless the OpSize is 16-bytes
      // Slightly more optimal for 8-byte opsize.
      dup(ARMEmitter::SubRegSize::i64Bit, VTMP1.Z(), ShiftScalar.Z(), 0);
      ShiftRegister = VTMP1;
    }

    if (Dst == ShiftRegister) {
      // If destination aliases the shift vector then we need to move it temporarily.
      mov(VTMP1.Z(), ShiftRegister.Z());
      ShiftRegister = VTMP1;
    }

    if (Dst != Vector) {
      // NOTE: SVE LSR is a destructive operation.
      movprfx(Dst.Z(), Vector.Z());
    }
    if (ElementSize == IR::OpSize::i64Bit) {
      lsl(SubRegSize, Dst.Z(), Mask, Dst.Z(), ShiftRegister.Z());
    } else {
      lsl_wide(SubRegSize, Dst.Z(), Mask, Dst.Z(), ShiftRegister.Z());
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

  const auto SubRegSize = ConvertSubRegSize16(IROp);
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetVReg(Node);
  const auto ShiftScalar = GetVReg(Op->ShiftScalar);
  const auto Vector = GetVReg(Op->Vector);

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Mask = PRED_TMP_32B.Merging();

    // NOTE: SVE ASR is a destructive operation, so we need to
    //       move the vector into the destination if they don't
    //       already alias.
    dup(SubRegSize, VTMP1.Z(), ShiftScalar.Z(), 0);
    if (Dst != Vector) {
      movprfx(Dst.Z(), Vector.Z());
    }
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
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto ElementSize = Op->Header.ElementSize;
  const auto SubRegSize = ConvertSubRegSize16(IROp);

  const uint32_t DestIdx = Op->DestIdx;
  const uint32_t SrcIdx = Op->SrcIdx;

  const auto Dst = GetVReg(Node);
  const auto SrcVector = GetVReg(Op->SrcVector);
  auto Reg = GetVReg(Op->DestVector);

  if (HostSupportsSVE256 && Is256Bit) {
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

    if (ElementSize == IR::OpSize::i128Bit) {
      if (DestIdx == 0) {
        mov(ARMEmitter::SubRegSize::i8Bit, Dst.Z(), PRED_TMP_16B.Merging(), VTMP2.Z());
      } else {
        not_(Predicate, PRED_TMP_32B.Zeroing(), PRED_TMP_16B);
        mov(ARMEmitter::SubRegSize::i8Bit, Dst.Z(), Predicate.Merging(), VTMP2.Z());
      }
    } else {
      const auto UpperBound = 16 >> FEXCore::ilog2(IR::OpSizeToSize(ElementSize));
      const auto TargetElement = static_cast<int>(DestIdx) - UpperBound;

      // FIXME: We should rework this op to avoid the NZCV spill/fill dance.
      mrs(TMP1, ARMEmitter::SystemRegister::NZCV);

      index(SubRegSize, VTMP1.Z(), -UpperBound, 1);
      cmpeq(SubRegSize, Predicate, PRED_TMP_32B.Zeroing(), VTMP1.Z(), TargetElement);
      mov(SubRegSize, Dst.Z(), Predicate.Merging(), VTMP2.Z());

      // Restore NZCV
      msr(ARMEmitter::SystemRegister::NZCV, TMP1);
    }
  } else {
    // If nothing aliases the destination, then we can just
    // move the DestVector over and directly insert.
    if (Dst != Reg && Dst != SrcVector) {
      mov(Dst.Q(), Reg.Q());
      ins(SubRegSize, Dst.Q(), DestIdx, SrcVector.Q(), SrcIdx);
      return;
    }

    // If our vector data to insert into is within a register
    // that aliases the destination, then we can avoid using a
    // temporary and just perform the insert.
    //
    // Otherwise, if the source vector to select from aliases
    // the destination, then we hit the worst case where we
    // need to use a temporary to avoid clobbering data.
    if (Dst != Reg) {
      mov(VTMP1.Q(), Reg.Q());
      Reg = VTMP1;
    }

    ins(SubRegSize, Reg.Q(), DestIdx, SrcVector.Q(), SrcIdx);

    if (Dst != Reg) {
      mov(Dst.Q(), Reg.Q());
    }
  }
}

DEF_OP(VDupElement) {
  const auto Op = IROp->C<IR::IROp_VDupElement>();
  const auto OpSize = IROp->Size;

  const auto Index = Op->Index;
  const auto SubRegSize = ConvertSubRegSize16(IROp);
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Is128Bit = OpSize == IR::OpSize::i128Bit;

  const auto Dst = GetVReg(Node);
  const auto Vector = GetVReg(Op->Vector);

  if (HostSupportsSVE256 && Is256Bit) {
    dup(SubRegSize, Dst.Z(), Vector.Z(), Index);
  } else {
    if (Is128Bit) {
      dup(SubRegSize, Dst.Q(), Vector.Q(), Index);
    } else {
      dup(SubRegSize, Dst.D(), Vector.D(), Index);
    }
  }
}

DEF_OP(VExtr) {
  const auto Op = IROp->C<IR::IROp_VExtr>();
  const auto OpSize = IROp->Size;
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  // AArch64 ext op has bit arrangement as [Vm:Vn] so arguments need to be swapped
  const auto Dst = GetVReg(Node);
  auto UpperBits = GetVReg(Op->VectorLower);
  auto LowerBits = GetVReg(Op->VectorUpper);

  const auto ElementSize = Op->Header.ElementSize;
  auto Index = Op->Index;

  if (Index >= IR::OpSizeToSize(OpSize)) {
    // Upper bits have moved in to the lower bits
    LowerBits = UpperBits;

    // Upper bits are all now zero
    UpperBits = VTMP1;
    movi(ARMEmitter::SubRegSize::i64Bit, VTMP1.Q(), 0);
    Index -= IR::OpSizeToSize(OpSize);
  }

  const auto CopyFromByte = Index * IR::OpSizeToSize(ElementSize);

  if (HostSupportsSVE256 && Is256Bit) {
    if (Dst == LowerBits) {
      // Trivial case where we don't need to do any moves
      ext<ARMEmitter::OpType::Destructive>(Dst.Z(), Dst.Z(), UpperBits.Z(), CopyFromByte);
    } else if (Dst == UpperBits) {
      movprfx(VTMP2.Z(), LowerBits.Z());
      ext<ARMEmitter::OpType::Destructive>(VTMP2.Z(), VTMP2.Z(), UpperBits.Z(), CopyFromByte);
      mov(Dst.Z(), VTMP2.Z());
    } else {
      // No registers alias the destination, so we can safely move into it.
      movprfx(Dst.Z(), LowerBits.Z());
      ext<ARMEmitter::OpType::Destructive>(Dst.Z(), Dst.Z(), UpperBits.Z(), CopyFromByte);
    }
  } else {
    if (OpSize == IR::OpSize::i64Bit) {
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
  const auto SubRegSize = ConvertSubRegSize8(IROp);
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetVReg(Node);
  const auto Vector = GetVReg(Op->Vector);

  if (BitShift >= IR::OpSizeAsBits(ElementSize)) {
    movi(ARMEmitter::SubRegSize::i64Bit, Dst.Q(), 0);
  } else {
    if (HostSupportsSVE256 && Is256Bit) {
      const auto Mask = PRED_TMP_32B.Merging();

      if (BitShift == 0) {
        if (Dst != Vector) {
          mov(Dst.Z(), Vector.Z());
        }
      } else {
        // SVE LSR is destructive, so lets set up the destination if
        // Vector doesn't already alias it.
        if (Dst != Vector) {
          movprfx(Dst.Z(), Vector.Z());
        }
        lsr(SubRegSize, Dst.Z(), Mask, Dst.Z(), BitShift);
      }
    } else {
      if (BitShift == 0) {
        if (Dst != Vector) {
          mov(Dst.Q(), Vector.Q());
        }
      } else {
        ushr(SubRegSize, Dst.Q(), Vector.Q(), BitShift);
      }
    }
  }
}

DEF_OP(VUShraI) {
  const auto Op = IROp->C<IR::IROp_VUShraI>();
  const auto OpSize = IROp->Size;

  const auto BitShift = Op->BitShift;
  const auto SubRegSize = ConvertSubRegSize8(IROp);
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetVReg(Node);
  const auto DestVector = GetVReg(Op->DestVector);
  const auto Vector = GetVReg(Op->Vector);

  if (HostSupportsSVE256 && Is256Bit) {
    if (Dst == DestVector) {
      usra(SubRegSize, Dst.Z(), Vector.Z(), BitShift);
    } else {
      if (Dst != Vector) {
        mov(Dst.Z(), DestVector.Z());
        usra(SubRegSize, Dst.Z(), Vector.Z(), BitShift);
      } else {
        mov(VTMP1.Z(), DestVector.Z());
        usra(SubRegSize, Dst.Z(), Vector.Z(), BitShift);
        mov(Dst.Z(), VTMP1.Z());
      }
    }
  } else {
    if (Dst == DestVector) {
      usra(SubRegSize, Dst.Q(), Vector.Q(), BitShift);
    } else {
      if (Dst != Vector) {
        mov(Dst.Q(), DestVector.Q());
        usra(SubRegSize, Dst.Q(), Vector.Q(), BitShift);
      } else {
        mov(VTMP1.Q(), DestVector.Q());
        usra(SubRegSize, VTMP1.Q(), Vector.Q(), BitShift);
        mov(Dst.Q(), VTMP1.Q());
      }
    }
  }
}

DEF_OP(VSShrI) {
  const auto Op = IROp->C<IR::IROp_VSShrI>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  LOGMAN_THROW_A_FMT(ElementSize >= IR::OpSize::i8Bit && ElementSize <= IR::OpSize::i64Bit, "Invalid element size");
  const auto SubRegSize = ConvertSubRegSize8(IROp);
  const auto Shift = std::min<uint8_t>(IR::OpSizeAsBits(ElementSize) - 1, Op->BitShift);
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetVReg(Node);
  const auto Vector = GetVReg(Op->Vector);

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Mask = PRED_TMP_32B.Merging();

    if (Shift == 0) {
      if (Dst != Vector) {
        mov(Dst.Z(), Vector.Z());
      }
    } else {
      // SVE ASR is destructive, so lets set up the destination if
      // Vector doesn't already alias it.
      if (Dst != Vector) {
        movprfx(Dst.Z(), Vector.Z());
      }
      asr(SubRegSize, Dst.Z(), Mask, Dst.Z(), Shift);
    }
  } else {
    if (Shift == 0) {
      if (Dst != Vector) {
        mov(Dst.Q(), Vector.Q());
      }
    } else {
      sshr(SubRegSize, Dst.Q(), Vector.Q(), Shift);
    }
  }
}

DEF_OP(VShlI) {
  const auto Op = IROp->C<IR::IROp_VShlI>();
  const auto OpSize = IROp->Size;

  const auto BitShift = Op->BitShift;
  const auto ElementSize = Op->Header.ElementSize;
  const auto SubRegSize = ConvertSubRegSize8(IROp);
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetVReg(Node);
  const auto Vector = GetVReg(Op->Vector);

  if (BitShift >= IR::OpSizeAsBits(ElementSize)) {
    movi(ARMEmitter::SubRegSize::i64Bit, Dst.Q(), 0);
  } else {
    if (HostSupportsSVE256 && Is256Bit) {
      const auto Mask = PRED_TMP_32B.Merging();

      if (BitShift == 0) {
        if (Dst != Vector) {
          mov(Dst.Z(), Vector.Z());
        }
      } else {
        // SVE LSL is destructive, so lets set up the destination if
        // Vector doesn't already alias it.
        if (Dst != Vector) {
          movprfx(Dst.Z(), Vector.Z());
        }
        lsl(SubRegSize, Dst.Z(), Mask, Dst.Z(), BitShift);
      }
    } else {
      if (BitShift == 0) {
        if (Dst != Vector) {
          mov(Dst.Q(), Vector.Q());
        }
      } else {
        shl(SubRegSize, Dst.Q(), Vector.Q(), BitShift);
      }
    }
  }
}

DEF_OP(VUShrNI) {
  const auto Op = IROp->C<IR::IROp_VUShrNI>();
  const auto OpSize = IROp->Size;

  const auto BitShift = Op->BitShift;
  const auto SubRegSize = ConvertSubRegSize4(IROp);
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetVReg(Node);
  const auto Vector = GetVReg(Op->Vector);

  if (HostSupportsSVE256 && Is256Bit) {
    shrnb(SubRegSize, Dst.Z(), Vector.Z(), BitShift);
    uzp1(SubRegSize, Dst.Z(), Dst.Z(), Dst.Z());
  } else {
    if (BitShift == 0) {
      xtn(SubRegSize, Dst.D(), Vector.D());
    } else {
      shrn(SubRegSize, Dst.D(), Vector.D(), BitShift);
    }
  }
}

DEF_OP(VUShrNI2) {
  const auto Op = IROp->C<IR::IROp_VUShrNI2>();
  const auto OpSize = IROp->Size;

  const auto BitShift = Op->BitShift;
  const auto SubRegSize = ConvertSubRegSize8(IROp);
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetVReg(Node);
  const auto VectorLower = GetVReg(Op->VectorLower);
  const auto VectorUpper = GetVReg(Op->VectorUpper);

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Mask = PRED_TMP_16B;

    shrnb(SubRegSize, VTMP2.Z(), VectorUpper.Z(), BitShift);
    uzp1(SubRegSize, VTMP2.Z(), VTMP2.Z(), VTMP2.Z());

    if (Dst != VectorLower) {
      movprfx(Dst.Z(), VectorLower.Z());
    }
    splice<ARMEmitter::OpType::Destructive>(SubRegSize, Dst.Z(), Mask, Dst.Z(), VTMP2.Z());
  } else {
    auto Lower = VectorLower;
    if (Dst != VectorLower) {
      mov(VTMP1.Q(), VectorLower.Q());
      Lower = VTMP1;
    }

    shrn2(SubRegSize, Lower.Q(), VectorUpper.Q(), BitShift);

    if (Dst != VectorLower) {
      mov(Dst.Q(), Lower.Q());
    }
  }
}

DEF_OP(VSXTL) {
  const auto Op = IROp->C<IR::IROp_VSXTL>();
  const auto OpSize = IROp->Size;

  const auto SubRegSize = ConvertSubRegSize248(IROp);
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetVReg(Node);
  const auto Vector = GetVReg(Op->Vector);

  if ((HostSupportsSVE128 && !Is256Bit && !HostSupportsSVE256) || (HostSupportsSVE256 && Is256Bit)) {
    sunpklo(SubRegSize, Dst.Z(), Vector.Z());
  } else {
    sxtl(SubRegSize, Dst.D(), Vector.D());
  }
}

DEF_OP(VSXTL2) {
  const auto Op = IROp->C<IR::IROp_VSXTL2>();
  const auto OpSize = IROp->Size;

  const auto SubRegSize = ConvertSubRegSize248(IROp);
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetVReg(Node);
  const auto Vector = GetVReg(Op->Vector);

  if ((HostSupportsSVE128 && !Is256Bit && !HostSupportsSVE256) || (HostSupportsSVE256 && Is256Bit)) {
    sunpkhi(SubRegSize, Dst.Z(), Vector.Z());
  } else {
    sxtl2(SubRegSize, Dst.Q(), Vector.Q());
  }
}

DEF_OP(VSSHLL) {
  const auto Op = IROp->C<IR::IROp_VSSHLL>();
  const auto OpSize = IROp->Size;

  const auto SubRegSize = ConvertSubRegSize248(IROp);
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetVReg(Node);
  const auto Vector = GetVReg(Op->Vector);
  const auto BitShift = Op->BitShift;
  LOGMAN_THROW_A_FMT(BitShift < IR::OpSizeAsBits(IROp->ElementSize / 2), "Bitshift size too large for source element size: {} < {}",
                     BitShift, IR::OpSizeAsBits(IROp->ElementSize / 2));

  if (Is256Bit) {
    sunpklo(SubRegSize, Dst.Z(), Vector.Z());
    lsl(SubRegSize, Dst.Z(), Dst.Z(), BitShift);
  } else {
    sshll(SubRegSize, Dst.D(), Vector.D(), BitShift);
  }
}

DEF_OP(VSSHLL2) {
  const auto Op = IROp->C<IR::IROp_VSSHLL2>();
  const auto OpSize = IROp->Size;

  const auto SubRegSize = ConvertSubRegSize248(IROp);
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetVReg(Node);
  const auto Vector = GetVReg(Op->Vector);
  const auto BitShift = Op->BitShift;
  LOGMAN_THROW_A_FMT(BitShift < IR::OpSizeAsBits(IROp->ElementSize / 2), "Bitshift size too large for source element size: {} < {}",
                     BitShift, IR::OpSizeAsBits(IROp->ElementSize / 2));

  if (Is256Bit) {
    sunpkhi(SubRegSize, Dst.Z(), Vector.Z());
    lsl(SubRegSize, Dst.Z(), Dst.Z(), BitShift);
  } else {
    sshll2(SubRegSize, Dst.Q(), Vector.Q(), BitShift);
  }
}

DEF_OP(VUXTL) {
  const auto Op = IROp->C<IR::IROp_VUXTL>();
  const auto OpSize = IROp->Size;

  const auto SubRegSize = ConvertSubRegSize248(IROp);
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetVReg(Node);
  const auto Vector = GetVReg(Op->Vector);

  if ((HostSupportsSVE128 && !Is256Bit && !HostSupportsSVE256) || (HostSupportsSVE256 && Is256Bit)) {
    uunpklo(SubRegSize, Dst.Z(), Vector.Z());
  } else {
    uxtl(SubRegSize, Dst.D(), Vector.D());
  }
}

DEF_OP(VUXTL2) {
  const auto Op = IROp->C<IR::IROp_VUXTL2>();
  const auto OpSize = IROp->Size;

  const auto SubRegSize = ConvertSubRegSize248(IROp);
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetVReg(Node);
  const auto Vector = GetVReg(Op->Vector);

  if ((HostSupportsSVE128 && !Is256Bit && !HostSupportsSVE256) || (HostSupportsSVE256 && Is256Bit)) {
    uunpkhi(SubRegSize, Dst.Z(), Vector.Z());
  } else {
    uxtl2(SubRegSize, Dst.Q(), Vector.Q());
  }
}

DEF_OP(VSQXTN) {
  const auto Op = IROp->C<IR::IROp_VSQXTN>();
  const auto OpSize = IROp->Size;

  const auto SubRegSize = ConvertSubRegSize4(IROp);
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetVReg(Node);
  const auto Vector = GetVReg(Op->Vector);

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

  const auto SubRegSize = ConvertSubRegSize4(IROp);
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetVReg(Node);
  const auto VectorLower = GetVReg(Op->VectorLower);
  const auto VectorUpper = GetVReg(Op->VectorUpper);

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
    if (Dst != VectorLower) {
      movprfx(Dst.Z(), VectorLower.Z());
    }
    splice<ARMEmitter::OpType::Destructive>(SubRegSize, Dst.Z(), Mask, Dst.Z(), VTMP2.Z());
  } else {
    if (OpSize == IR::OpSize::i64Bit) {
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

DEF_OP(VSQXTNPair) {
  const auto Op = IROp->C<IR::IROp_VSQXTNPair>();
  const auto OpSize = IROp->Size;

  const auto SubRegSize = ConvertSubRegSize4(IROp);
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetVReg(Node);
  const auto VectorLower = GetVReg(Op->VectorLower);
  auto VectorUpper = GetVReg(Op->VectorUpper);

  if (HostSupportsSVE256 && Is256Bit) {
    // This combines the SVE versions of VSQXTN/VSQXTN2.
    // Upper VSQXTN2 handling.
    // Doing upper first to ensure it doesn't get overwritten by lower calculation.
    const auto Mask = PRED_TMP_16B;

    sqxtnb(SubRegSize, VTMP2.Z(), VectorUpper.Z());
    uzp1(SubRegSize, VTMP2.Z(), VTMP2.Z(), VTMP2.Z());

    // Look at those implementations for details about this.
    // Lower VSQXTN handling.
    sqxtnb(SubRegSize, Dst.Z(), VectorLower.Z());
    uzp1(SubRegSize, Dst.Z(), Dst.Z(), Dst.Z());

    // Merge.
    splice<ARMEmitter::OpType::Destructive>(SubRegSize, Dst.Z(), Mask, Dst.Z(), VTMP2.Z());
  } else {
    if (OpSize == IR::OpSize::i64Bit) {
      zip1(ARMEmitter::SubRegSize::i64Bit, Dst.Q(), VectorLower.Q(), VectorUpper.Q());
      sqxtn(SubRegSize, Dst, Dst);
    } else {
      if (Dst == VectorUpper) {
        // If the destination overlaps the upper then we need to move it temporarily.
        mov(VTMP1.Q(), VectorUpper.Q());
        VectorUpper = VTMP1;
      }
      sqxtn(SubRegSize, Dst, VectorLower);
      sqxtn2(SubRegSize, Dst, VectorUpper);
    }
  }
}

DEF_OP(VSQXTUN) {
  const auto Op = IROp->C<IR::IROp_VSQXTUN>();
  const auto OpSize = IROp->Size;

  const auto SubRegSize = ConvertSubRegSize8(IROp);
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetVReg(Node);
  const auto Vector = GetVReg(Op->Vector);

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

  const auto SubRegSize = ConvertSubRegSize8(IROp);
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetVReg(Node);
  const auto VectorLower = GetVReg(Op->VectorLower);
  const auto VectorUpper = GetVReg(Op->VectorUpper);

  if (HostSupportsSVE256 && Is256Bit) {
    // NOTE: See VSQXTN2 implementation for an in-depth explanation
    //       of everything going on here.

    const auto Mask = PRED_TMP_16B;

    sqxtunb(SubRegSize, VTMP2.Z(), VectorUpper.Z());
    uzp1(SubRegSize, VTMP2.Z(), VTMP2.Z(), VTMP2.Z());

    if (Dst != VectorLower) {
      movprfx(Dst.Z(), VectorLower.Z());
    }
    splice<ARMEmitter::OpType::Destructive>(SubRegSize, Dst.Z(), Mask, Dst.Z(), VTMP2.Z());
  } else {
    if (OpSize == IR::OpSize::i64Bit) {
      sqxtun(SubRegSize, VTMP2, VectorUpper);
      mov(Dst.Q(), VectorLower.Q());
      ins(ARMEmitter::SubRegSize::i32Bit, Dst, 1, VTMP2, 0);
    } else {
      auto Lower = VectorLower;
      if (Dst != VectorLower) {
        mov(VTMP1.Q(), VectorLower.Q());
        Lower = VTMP1;
      }

      sqxtun2(SubRegSize, Lower, VectorUpper);

      if (Dst != VectorLower) {
        mov(Dst.Q(), Lower.Q());
      }
    }
  }
}

DEF_OP(VSQXTUNPair) {
  const auto Op = IROp->C<IR::IROp_VSQXTUNPair>();
  const auto OpSize = IROp->Size;

  const auto SubRegSize = ConvertSubRegSize4(IROp);
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetVReg(Node);
  const auto VectorLower = GetVReg(Op->VectorLower);
  auto VectorUpper = GetVReg(Op->VectorUpper);

  if (HostSupportsSVE256 && Is256Bit) {
    // This combines the SVE versions of VSQXTUN/VSQXTUN2.
    // Upper VSQXTUN2 handling.
    // Doing upper first to ensure it doesn't get overwritten by lower calculation.
    const auto Mask = PRED_TMP_16B;

    sqxtunb(SubRegSize, VTMP2.Z(), VectorUpper.Z());
    uzp1(SubRegSize, VTMP2.Z(), VTMP2.Z(), VTMP2.Z());

    // Look at those implementations for details about this.
    // Lower VSQXTUN handling.
    sqxtunb(SubRegSize, Dst.Z(), VectorLower.Z());
    uzp1(SubRegSize, Dst.Z(), Dst.Z(), Dst.Z());

    // Merge.
    splice<ARMEmitter::OpType::Destructive>(SubRegSize, Dst.Z(), Mask, Dst.Z(), VTMP2.Z());
  } else {
    if (OpSize == IR::OpSize::i64Bit) {
      zip1(ARMEmitter::SubRegSize::i64Bit, Dst.Q(), VectorLower.Q(), VectorUpper.Q());
      sqxtun(SubRegSize, Dst, Dst);
    } else {
      if (Dst == VectorUpper) {
        // If the destination overlaps the upper then we need to move it temporarily.
        mov(VTMP1.Q(), VectorUpper.Q());
        VectorUpper = VTMP1;
      }
      sqxtun(SubRegSize, Dst, VectorLower);
      sqxtun2(SubRegSize, Dst, VectorUpper);
    }
  }
}

DEF_OP(VSRSHR) {
  const auto Op = IROp->C<IR::IROp_VSRSHR>();
  const auto OpSize = IROp->Size;

  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto SubRegSize = ConvertSubRegSize16(IROp);

  const auto Dst = GetVReg(Node);
  const auto Vector = GetVReg(Op->Vector);
  const auto BitShift = Op->BitShift;

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Mask = PRED_TMP_32B.Merging();
    // SVE SRSHR is destructive, so lets set up the destination
    // in the event we Dst and Vector don't alias.
    if (Dst != Vector) {
      movprfx(Dst.Z(), Vector.Z());
    }
    srshr(SubRegSize, Dst.Z(), Mask, Dst.Z(), BitShift);
  } else {
    if (OpSize == IR::OpSize::i64Bit) {
      srshr(SubRegSize, Dst.D(), Vector.D(), BitShift);
    } else {
      srshr(SubRegSize, Dst.Q(), Vector.Q(), BitShift);
    }
  }
}

DEF_OP(VSQSHL) {
  const auto Op = IROp->C<IR::IROp_VSQSHL>();
  const auto OpSize = IROp->Size;

  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto SubRegSize = ConvertSubRegSize8(IROp);

  const auto Dst = GetVReg(Node);
  const auto Vector = GetVReg(Op->Vector);
  const auto BitShift = Op->BitShift;

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Mask = PRED_TMP_32B.Merging();
    // SVE SQSHL is destructive, so lets set up the destination
    // in the event Dst and Vector don't alias
    if (Dst != Vector) {
      movprfx(Dst.Z(), Vector.Z());
    }
    sqshl(SubRegSize, Dst.Z(), Mask, Dst.Z(), BitShift);
  } else {
    if (OpSize == IR::OpSize::i64Bit) {
      sqshl(SubRegSize, Dst.D(), Vector.D(), BitShift);
    } else {
      sqshl(SubRegSize, Dst.Q(), Vector.Q(), BitShift);
    }
  }
}

DEF_OP(VMul) {
  const auto Op = IROp->C<IR::IROp_VMul>();
  const auto OpSize = IROp->Size;

  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto SubRegSize = ConvertSubRegSize16(IROp);

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1);
  const auto Vector2 = GetVReg(Op->Vector2);

  if (HostSupportsSVE256 && Is256Bit) {
    mul(SubRegSize, Dst.Z(), Vector1.Z(), Vector2.Z());
  } else {
    mul(SubRegSize, Dst.Q(), Vector1.Q(), Vector2.Q());
  }
}

DEF_OP(VUMull) {
  const auto Op = IROp->C<IR::IROp_VUMull>();
  const auto OpSize = IROp->Size;

  const auto SubRegSize = ConvertSubRegSize248(IROp);
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1);
  const auto Vector2 = GetVReg(Op->Vector2);

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

  const auto SubRegSize = ConvertSubRegSize248(IROp);
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1);
  const auto Vector2 = GetVReg(Op->Vector2);

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

  const auto SubRegSize = ConvertSubRegSize248(IROp);
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1);
  const auto Vector2 = GetVReg(Op->Vector2);

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

  const auto SubRegSize = ConvertSubRegSize248(IROp);
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1);
  const auto Vector2 = GetVReg(Op->Vector2);

  if (HostSupportsSVE256 && Is256Bit) {
    smullb(SubRegSize, VTMP1.Z(), Vector1.Z(), Vector2.Z());
    smullt(SubRegSize, VTMP2.Z(), Vector1.Z(), Vector2.Z());
    zip2(SubRegSize, Dst.Z(), VTMP1.Z(), VTMP2.Z());
  } else {
    smull2(SubRegSize, Dst.Q(), Vector1.Q(), Vector2.Q());
  }
}

DEF_OP(VUMulH) {
  const auto Op = IROp->C<IR::IROp_VUMulH>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto SubRegSize = ConvertSubRegSize8(IROp);
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Is128Bit = OpSize == IR::OpSize::i128Bit;

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1);
  const auto Vector2 = GetVReg(Op->Vector2);

  const auto SubRegSizeLarger = ElementSize == IR::OpSize::i8Bit  ? ARMEmitter::SubRegSize::i16Bit :
                                ElementSize == IR::OpSize::i16Bit ? ARMEmitter::SubRegSize::i32Bit :
                                ElementSize == IR::OpSize::i32Bit ? ARMEmitter::SubRegSize::i64Bit :
                                                                    ARMEmitter::SubRegSize::i8Bit;

  if (HostSupportsSVE256 && Is256Bit) {
    umulh(SubRegSize, Dst.Z(), Vector1.Z(), Vector2.Z());
  } else if (HostSupportsSVE128 && Is128Bit) {
    if (HostSupportsSVE256) {
      // Do predicated to ensure upper-bits get zero as expected
      const auto Mask = PRED_TMP_16B.Merging();

      if (Dst == Vector1) {
        umulh(SubRegSize, Dst.Z(), Mask, Dst.Z(), Vector2.Z());
      } else if (Dst == Vector2) {
        umulh(SubRegSize, Dst.Z(), Mask, Dst.Z(), Vector1.Z());
      } else {
        // Destination register doesn't overlap either source.
        // NOTE: SVE umulh (predicated) is a destructive operation.
        movprfx(Dst.Z(), Vector1.Z());
        umulh(SubRegSize, Dst.Z(), Mask, Dst.Z(), Vector2.Z());
      }
    } else {
      umulh(SubRegSize, Dst.Z(), Vector1.Z(), Vector2.Z());
    }
  } else if (OpSize == IR::OpSize::i64Bit) {
    umull(SubRegSizeLarger, Dst.D(), Vector1.D(), Vector2.D());
    shrn(SubRegSize, Dst.D(), Dst.D(), IR::OpSizeAsBits(ElementSize));
  } else {
    // ASIMD doesn't have a umulh. Need to emulate.
    umull2(SubRegSizeLarger, VTMP1.Q(), Vector1.Q(), Vector2.Q());
    umull(SubRegSizeLarger, Dst.D(), Vector1.D(), Vector2.D());
    uzp2(SubRegSize, Dst.Q(), Dst.Q(), VTMP1.Q());
  }
}

DEF_OP(VSMulH) {
  const auto Op = IROp->C<IR::IROp_VSMulH>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto SubRegSize = ConvertSubRegSize8(IROp);
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Is128Bit = OpSize == IR::OpSize::i128Bit;

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1);
  const auto Vector2 = GetVReg(Op->Vector2);

  const auto SubRegSizeLarger = ElementSize == IR::OpSize::i8Bit  ? ARMEmitter::SubRegSize::i16Bit :
                                ElementSize == IR::OpSize::i16Bit ? ARMEmitter::SubRegSize::i32Bit :
                                ElementSize == IR::OpSize::i32Bit ? ARMEmitter::SubRegSize::i64Bit :
                                                                    ARMEmitter::SubRegSize::i8Bit;

  if (HostSupportsSVE256 && Is256Bit) {
    smulh(SubRegSize, Dst.Z(), Vector1.Z(), Vector2.Z());
  } else if (HostSupportsSVE128 && Is128Bit) {
    if (HostSupportsSVE256) {
      // Do predicated to ensure upper-bits get zero as expected
      const auto Mask = PRED_TMP_16B.Merging();

      if (Dst == Vector1) {
        smulh(SubRegSize, Dst.Z(), Mask, Dst.Z(), Vector2.Z());
      } else if (Dst == Vector2) {
        smulh(SubRegSize, Dst.Z(), Mask, Dst.Z(), Vector1.Z());
      } else {
        // Destination register doesn't overlap either source.
        // NOTE: SVE umulh (predicated) is a destructive operation.
        movprfx(Dst.Z(), Vector1.Z());
        smulh(SubRegSize, Dst.Z(), Mask, Dst.Z(), Vector2.Z());
      }
    } else {
      smulh(SubRegSize, Dst.Z(), Vector1.Z(), Vector2.Z());
    }
  } else if (OpSize == IR::OpSize::i64Bit) {
    smull(SubRegSizeLarger, Dst.D(), Vector1.D(), Vector2.D());
    shrn(SubRegSize, Dst.D(), Dst.D(), IR::OpSizeAsBits(ElementSize));
  } else {
    // ASIMD doesn't have a umulh. Need to emulate.
    smull2(SubRegSizeLarger, VTMP1.Q(), Vector1.Q(), Vector2.Q());
    smull(SubRegSizeLarger, Dst.D(), Vector1.D(), Vector2.D());
    uzp2(SubRegSize, Dst.Q(), Dst.Q(), VTMP1.Q());
  }
}

DEF_OP(VUABDL) {
  const auto Op = IROp->C<IR::IROp_VUABDL>();
  const auto OpSize = IROp->Size;

  const auto SubRegSize = ConvertSubRegSize248(IROp);
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1);
  const auto Vector2 = GetVReg(Op->Vector2);

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

  const auto SubRegSize = ConvertSubRegSize248(IROp);
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1);
  const auto Vector2 = GetVReg(Op->Vector2);

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
  const auto VectorIndices = GetVReg(Op->VectorIndices);
  const auto VectorTable = GetVReg(Op->VectorTable);

  switch (OpSize) {
  case IR::OpSize::i64Bit: {
    tbl(Dst.D(), VectorTable.Q(), VectorIndices.D());
    break;
  }
  case IR::OpSize::i128Bit: {
    tbl(Dst.Q(), VectorTable.Q(), VectorIndices.Q());
    break;
  }
  case IR::OpSize::i256Bit: {
    LOGMAN_THROW_A_FMT(HostSupportsSVE256, "Host does not support SVE. Cannot perform 256-bit table lookup");

    tbl(ARMEmitter::SubRegSize::i8Bit, Dst.Z(), VectorTable.Z(), VectorIndices.Z());
    break;
  }
  default: LOGMAN_MSG_A_FMT("Unknown OpSize: {}", OpSize); break;
  }
}

DEF_OP(VTBL2) {
  const auto Op = IROp->C<IR::IROp_VTBL2>();
  const auto OpSize = IROp->Size;

  const auto Dst = GetVReg(Node);
  const auto VectorIndices = GetVReg(Op->VectorIndices);
  auto VectorTable1 = GetVReg(Op->VectorTable1);
  auto VectorTable2 = GetVReg(Op->VectorTable2);

  if (!ARMEmitter::AreVectorsSequential(VectorTable1, VectorTable2)) {
    // Vector registers aren't sequential, need to move to temporaries.
    if (OpSize == IR::OpSize::i256Bit) {
      mov(VTMP1.Z(), VectorTable1.Z());
      mov(VTMP2.Z(), VectorTable2.Z());
    } else {
      mov(VTMP1.Q(), VectorTable1.Q());
      mov(VTMP2.Q(), VectorTable2.Q());
    }

    static_assert(ARMEmitter::AreVectorsSequential(VTMP1, VTMP2), "VTMP1 and VTMP2 must be sequential in order to use double-table "
                                                                  "TBL");
    VectorTable1 = VTMP1;
    VectorTable2 = VTMP2;
  }

  switch (OpSize) {
  case IR::OpSize::i64Bit: {
    tbl(Dst.D(), VectorTable1.Q(), VectorTable2.Q(), VectorIndices.D());
    break;
  }
  case IR::OpSize::i128Bit: {
    tbl(Dst.Q(), VectorTable1.Q(), VectorTable2.Q(), VectorIndices.Q());
    break;
  }
  case IR::OpSize::i256Bit: {
    LOGMAN_THROW_A_FMT(HostSupportsSVE256, "Host does not support SVE. Cannot perform 256-bit table lookup");

    tbl(ARMEmitter::SubRegSize::i8Bit, Dst.Z(), VectorTable1.Z(), VectorTable2.Z(), VectorIndices.Z());
    break;
  }
  default: LOGMAN_MSG_A_FMT("Unknown OpSize: {}", OpSize); break;
  }
}

DEF_OP(VTBX1) {
  const auto Op = IROp->C<IR::IROp_VTBX1>();
  const auto OpSize = IROp->Size;

  const auto Dst = GetVReg(Node);
  const auto VectorSrcDst = GetVReg(Op->VectorSrcDst);
  const auto VectorIndices = GetVReg(Op->VectorIndices);
  const auto VectorTable = GetVReg(Op->VectorTable);

  if (Dst != VectorSrcDst) {
    switch (OpSize) {
    case IR::OpSize::i64Bit: {
      mov(VTMP1.D(), VectorSrcDst.D());
      tbx(VTMP1.D(), VectorTable.Q(), VectorIndices.D());
      mov(Dst.D(), VTMP1.D());
      break;
    }
    case IR::OpSize::i128Bit: {
      mov(VTMP1.Q(), VectorSrcDst.Q());
      tbx(VTMP1.Q(), VectorTable.Q(), VectorIndices.Q());
      mov(Dst.Q(), VTMP1.Q());
      break;
    }
    case IR::OpSize::i256Bit: {
      LOGMAN_THROW_A_FMT(HostSupportsSVE256, "Host does not support SVE. Cannot perform 256-bit table lookup");
      mov(VTMP1.Z(), VectorSrcDst.Z());
      tbx(ARMEmitter::SubRegSize::i8Bit, VTMP1.Z(), VectorTable.Z(), VectorIndices.Z());
      mov(Dst.Z(), VTMP1.Z());
      break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown OpSize: {}", OpSize); break;
    }
  } else {
    switch (OpSize) {
    case IR::OpSize::i64Bit: {
      tbx(VectorSrcDst.D(), VectorTable.Q(), VectorIndices.D());
      break;
    }
    case IR::OpSize::i128Bit: {
      tbx(VectorSrcDst.Q(), VectorTable.Q(), VectorIndices.Q());
      break;
    }
    case IR::OpSize::i256Bit: {
      LOGMAN_THROW_A_FMT(HostSupportsSVE256, "Host does not support SVE. Cannot perform 256-bit table lookup");

      tbx(ARMEmitter::SubRegSize::i8Bit, VectorSrcDst.Z(), VectorTable.Z(), VectorIndices.Z());
      break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown OpSize: {}", OpSize); break;
    }
  }
}

DEF_OP(VRev32) {
  const auto Op = IROp->C<IR::IROp_VRev32>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetVReg(Node);
  const auto Vector = GetVReg(Op->Vector);

  LOGMAN_THROW_A_FMT(ElementSize == IR::OpSize::i8Bit || ElementSize == IR::OpSize::i16Bit, "Invalid size");
  const auto SubRegSize = ElementSize == IR::OpSize::i8Bit ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i16Bit;

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Mask = PRED_TMP_32B.Merging();

    switch (ElementSize) {
    case IR::OpSize::i8Bit: {
      revb(ARMEmitter::SubRegSize::i32Bit, Dst.Z(), Mask, Vector.Z());
      break;
    }
    case IR::OpSize::i16Bit: {
      revh(ARMEmitter::SubRegSize::i32Bit, Dst.Z(), Mask, Vector.Z());
      break;
    }
    default: LOGMAN_MSG_A_FMT("Invalid Element Size: {}", ElementSize); break;
    }
  } else {
    if (OpSize == IR::OpSize::i64Bit) {
      rev32(SubRegSize, Dst.D(), Vector.D());
    } else {
      rev32(SubRegSize, Dst.Q(), Vector.Q());
    }
  }
}


DEF_OP(VRev64) {
  const auto Op = IROp->C<IR::IROp_VRev64>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto SubRegSize = ConvertSubRegSize4(IROp);
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetVReg(Node);
  const auto Vector = GetVReg(Op->Vector);

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Mask = PRED_TMP_32B.Merging();

    switch (ElementSize) {
    case IR::OpSize::i8Bit: {
      revb(ARMEmitter::SubRegSize::i64Bit, Dst.Z(), Mask, Vector.Z());
      break;
    }
    case IR::OpSize::i16Bit: {
      revh(ARMEmitter::SubRegSize::i64Bit, Dst.Z(), Mask, Vector.Z());
      break;
    }
    case IR::OpSize::i32Bit: {
      revw(ARMEmitter::SubRegSize::i64Bit, Dst.Z(), Mask, Vector.Z());
      break;
    }
    default: LOGMAN_MSG_A_FMT("Invalid Element Size: {}", ElementSize); break;
    }
  } else {
    if (OpSize == IR::OpSize::i64Bit) {
      rev64(SubRegSize, Dst.D(), Vector.D());
    } else {
      rev64(SubRegSize, Dst.Q(), Vector.Q());
    }
  }
}

DEF_OP(VFCADD) {
  const auto Op = IROp->C<IR::IROp_VFCADD>();
  const auto OpSize = IROp->Size;

  const auto SubRegSize = ConvertSubRegSize248(IROp);
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1);
  const auto Vector2 = GetVReg(Op->Vector2);

  LOGMAN_THROW_A_FMT(Op->Rotate == 90 || Op->Rotate == 270, "Invalidate Rotate");
  const auto Rotate = Op->Rotate == 90 ? ARMEmitter::Rotation::ROTATE_90 : ARMEmitter::Rotation::ROTATE_270;

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Mask = PRED_TMP_32B.Merging();

    if (Dst == Vector1) {
      // Trivial case where we already have first vector in the destination
      // register. We can just do the operation in place.
      fcadd(SubRegSize, Dst.Z(), Mask, Vector1.Z(), Vector2.Z(), Rotate);
    } else if (Dst == Vector2) {
      // SVE FCADD is a destructive operation, so we need
      // a temporary for performing operations.
      movprfx(VTMP1.Z(), Vector1.Z());
      fcadd(SubRegSize, VTMP1.Z(), Mask, VTMP1.Z(), Vector2.Z(), Rotate);
      mov(Dst.Z(), VTMP1.Z());
    } else {
      // We have no source/dest aliasing, so we can move into the destination.
      movprfx(Dst.Z(), Vector1.Z());
      fcadd(SubRegSize, Dst.Z(), Mask, Dst.Z(), Vector2.Z(), Rotate);
    }
  } else {
    if (OpSize == IR::OpSize::i64Bit) {
      fcadd(SubRegSize, Dst.D(), Vector1.D(), Vector2.D(), Rotate);
    } else {
      fcadd(SubRegSize, Dst.Q(), Vector1.Q(), Vector2.Q(), Rotate);
    }
  }
}

DEF_OP(VFMLA) {
  ///< Dest = (Vector1 * Vector2) + Addend
  // Matches:
  // - SVE    - FMLA
  // - ASIMD  - FMLA
  // - Scalar - FMADD
  const auto Op = IROp->C<IR::IROp_VFMLA>();
  const auto OpSize = IROp->Size;

  const auto SubRegSize = ConvertSubRegSize248(IROp);
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1);
  const auto Vector2 = GetVReg(Op->Vector2);
  const auto VectorAddend = GetVReg(Op->Addend);

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Mask = PRED_TMP_32B.Merging();
    ARMEmitter::VRegister DestTmp = Dst;
    if (Dst != VectorAddend) {
      if (Dst != Vector1 && Dst != Vector2) {
        DestTmp = Dst;
      } else {
        DestTmp = VTMP1;
      }
      mov(DestTmp.Z(), VectorAddend.Z());
    }

    fmla(SubRegSize, DestTmp.Z(), Mask, Vector1.Z(), Vector2.Z());
    if (Dst != DestTmp) {
      mov(Dst.Z(), DestTmp.Z());
    }
  } else {
    if (IROp->ElementSize == OpSize) {
      if (IROp->ElementSize == IR::OpSize::i16Bit) {
        fmadd(Dst.H(), Vector1.H(), Vector2.H(), VectorAddend.H());
      } else if (IROp->ElementSize == IR::OpSize::i32Bit) {
        fmadd(Dst.S(), Vector1.S(), Vector2.S(), VectorAddend.S());
      } else if (IROp->ElementSize == IR::OpSize::i64Bit) {
        fmadd(Dst.D(), Vector1.D(), Vector2.D(), VectorAddend.D());
      }
      return;
    }
    ARMEmitter::VRegister DestTmp = Dst;
    if (Dst != VectorAddend) {
      if (Dst != Vector1 && Dst != Vector2) {
        DestTmp = Dst;
      } else {
        DestTmp = VTMP1;
      }
      mov(DestTmp.Q(), VectorAddend.Q());
    }
    if (OpSize == IR::OpSize::i128Bit) {
      fmla(SubRegSize, DestTmp.Q(), Vector1.Q(), Vector2.Q());
    } else {
      fmla(SubRegSize, DestTmp.D(), Vector1.D(), Vector2.D());
    }

    if (Dst != DestTmp) {
      mov(Dst.Q(), DestTmp.Q());
    }
  }
}

DEF_OP(VFMLS) {
  ///< Dest = (Vector1 * Vector2) - Addend
  // Matches:
  // - SVE    - FNMLS
  // - ASIMD  - FMLA (With negated addend)
  // - Scalar - FNMSUB
  const auto Op = IROp->C<IR::IROp_VFMLS>();
  const auto OpSize = IROp->Size;

  const auto SubRegSize = ConvertSubRegSize248(IROp);
  const auto Is128Bit = OpSize == IR::OpSize::i128Bit;
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1);
  const auto Vector2 = GetVReg(Op->Vector2);
  const auto VectorAddend = GetVReg(Op->Addend);

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Mask = PRED_TMP_32B.Merging();
    ARMEmitter::VRegister DestTmp = Dst;
    if (Dst != VectorAddend) {
      if (Dst != Vector1 && Dst != Vector2) {
        DestTmp = Dst;
      } else {
        DestTmp = VTMP1;
      }
      mov(DestTmp.Z(), VectorAddend.Z());
    }

    fnmls(SubRegSize, DestTmp.Z(), Mask, Vector1.Z(), Vector2.Z());
    if (Dst != DestTmp) {
      mov(Dst.Z(), DestTmp.Z());
    }
  } else if (HostSupportsSVE128 && Is128Bit) {
    const auto Mask = PRED_TMP_16B.Merging();
    ARMEmitter::VRegister DestTmp = Dst;
    if (Dst != VectorAddend) {
      if (Dst != Vector1 && Dst != Vector2) {
        DestTmp = Dst;
      } else {
        DestTmp = VTMP1;
      }
      mov(DestTmp.Z(), VectorAddend.Z());
    }

    fnmls(SubRegSize, DestTmp.Z(), Mask, Vector1.Z(), Vector2.Z());
    if (Dst != DestTmp) {
      mov(Dst.Z(), DestTmp.Z());
    }
  } else {
    if (IROp->ElementSize == OpSize) {
      if (IROp->ElementSize == IR::OpSize::i16Bit) {
        fnmsub(Dst.H(), Vector1.H(), Vector2.H(), VectorAddend.H());
      } else if (IROp->ElementSize == IR::OpSize::i32Bit) {
        fnmsub(Dst.S(), Vector1.S(), Vector2.S(), VectorAddend.S());
      } else if (IROp->ElementSize == IR::OpSize::i64Bit) {
        fnmsub(Dst.D(), Vector1.D(), Vector2.D(), VectorAddend.D());
      }
      return;
    }

    // Addend needs to get negated to match correct behaviour here.
    ARMEmitter::VRegister DestTmp = Dst;
    if (Dst == Vector1 || Dst == Vector2) {
      DestTmp = VTMP1;
    }

    if (Is128Bit) {
      fneg(SubRegSize, DestTmp.Q(), VectorAddend.Q());
    } else {
      fneg(SubRegSize, DestTmp.D(), VectorAddend.D());
    }

    if (Is128Bit) {
      fmla(SubRegSize, DestTmp.Q(), Vector1.Q(), Vector2.Q());
    } else {
      fmla(SubRegSize, DestTmp.D(), Vector1.D(), Vector2.D());
    }

    if (DestTmp != Dst) {
      if (Is128Bit) {
        mov(Dst.Q(), DestTmp.Q());
      } else {
        mov(Dst.D(), DestTmp.D());
      }
    }
  }
}

DEF_OP(VFNMLA) {
  ///< Dest = (-Vector1 * Vector2) + Addend
  // Matches:
  // - SVE    - FMLS
  // - ASIMD  - FMLS
  // - Scalar - FMSUB
  const auto Op = IROp->C<IR::IROp_VFMLA>();
  const auto OpSize = IROp->Size;

  const auto SubRegSize = ConvertSubRegSize248(IROp);
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1);
  const auto Vector2 = GetVReg(Op->Vector2);
  const auto VectorAddend = GetVReg(Op->Addend);

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Mask = PRED_TMP_32B.Merging();
    ARMEmitter::VRegister DestTmp = Dst;
    if (Dst != VectorAddend) {
      if (Dst != Vector1 && Dst != Vector2) {
        DestTmp = Dst;
      } else {
        DestTmp = VTMP1;
      }
      mov(DestTmp.Z(), VectorAddend.Z());
    }

    fmls(SubRegSize, DestTmp.Z(), Mask, Vector1.Z(), Vector2.Z());
    if (Dst != DestTmp) {
      mov(Dst.Z(), DestTmp.Z());
    }
  } else {
    if (IROp->ElementSize == OpSize) {
      if (IROp->ElementSize == IR::OpSize::i16Bit) {
        fmsub(Dst.H(), Vector1.H(), Vector2.H(), VectorAddend.H());
      } else if (IROp->ElementSize == IR::OpSize::i32Bit) {
        fmsub(Dst.S(), Vector1.S(), Vector2.S(), VectorAddend.S());
      } else if (IROp->ElementSize == IR::OpSize::i64Bit) {
        fmsub(Dst.D(), Vector1.D(), Vector2.D(), VectorAddend.D());
      }
      return;
    }

    ARMEmitter::VRegister DestTmp = Dst;
    if (Dst != VectorAddend) {
      if (Dst != Vector1 && Dst != Vector2) {
        DestTmp = Dst;
      } else {
        DestTmp = VTMP1;
      }
      mov(DestTmp.Q(), VectorAddend.Q());
    }
    if (OpSize == IR::OpSize::i128Bit) {
      fmls(SubRegSize, DestTmp.Q(), Vector1.Q(), Vector2.Q());
    } else {
      fmls(SubRegSize, DestTmp.D(), Vector1.D(), Vector2.D());
    }

    if (Dst != DestTmp) {
      mov(Dst.Q(), DestTmp.Q());
    }
  }
}

DEF_OP(VFNMLS) {
  ///< Dest = (-Vector1 * Vector2) - Addend
  // Matches:
  // - SVE    - FNMLA
  // - ASIMD  - FMLS (With Negated addend)
  // - Scalar - FNMADD

  const auto Op = IROp->C<IR::IROp_VFMLS>();
  const auto OpSize = IROp->Size;

  const auto SubRegSize = ConvertSubRegSize248(IROp);
  const auto Is256Bit = OpSize == IR::OpSize::i256Bit;
  LOGMAN_THROW_A_FMT(!Is256Bit || HostSupportsSVE256, "Need SVE256 support in order to use {} with 256-bit operation", __func__);

  const auto Is128Bit = OpSize == IR::OpSize::i128Bit;

  const auto Dst = GetVReg(Node);
  const auto Vector1 = GetVReg(Op->Vector1);
  const auto Vector2 = GetVReg(Op->Vector2);
  const auto VectorAddend = GetVReg(Op->Addend);

  if (HostSupportsSVE256 && Is256Bit) {
    const auto Mask = PRED_TMP_32B.Merging();
    ARMEmitter::VRegister DestTmp = Dst;
    if (Dst != VectorAddend) {
      if (Dst != Vector1 && Dst != Vector2) {
        DestTmp = Dst;
      } else {
        DestTmp = VTMP1;
      }
      mov(DestTmp.Z(), VectorAddend.Z());
    }

    fnmla(SubRegSize, DestTmp.Z(), Mask, Vector1.Z(), Vector2.Z());
    if (Dst != DestTmp) {
      mov(Dst.Z(), DestTmp.Z());
    }
  } else if (HostSupportsSVE128 && Is128Bit) {
    const auto Mask = PRED_TMP_16B.Merging();
    ARMEmitter::VRegister DestTmp = Dst;
    if (Dst != VectorAddend) {
      if (Dst != Vector1 && Dst != Vector2) {
        DestTmp = Dst;
      } else {
        DestTmp = VTMP1;
      }
      mov(DestTmp.Z(), VectorAddend.Z());
    }

    fnmla(SubRegSize, DestTmp.Z(), Mask, Vector1.Z(), Vector2.Z());
    if (Dst != DestTmp) {
      mov(Dst.Z(), DestTmp.Z());
    }
  } else {
    if (IROp->ElementSize == OpSize) {
      if (IROp->ElementSize == IR::OpSize::i16Bit) {
        fnmadd(Dst.H(), Vector1.H(), Vector2.H(), VectorAddend.H());
      } else if (IROp->ElementSize == IR::OpSize::i32Bit) {
        fnmadd(Dst.S(), Vector1.S(), Vector2.S(), VectorAddend.S());
      } else if (IROp->ElementSize == IR::OpSize::i64Bit) {
        fnmadd(Dst.D(), Vector1.D(), Vector2.D(), VectorAddend.D());
      }
      return;
    }

    // Addend needs to get negated to match correct behaviour here.
    ARMEmitter::VRegister DestTmp = Dst;
    if (Dst == Vector1 || Dst == Vector2) {
      DestTmp = VTMP1;
    }

    if (Is128Bit) {
      fneg(SubRegSize, DestTmp.Q(), VectorAddend.Q());
    } else {
      fneg(SubRegSize, DestTmp.D(), VectorAddend.D());
    }

    if (Is128Bit) {
      fmls(SubRegSize, DestTmp.Q(), Vector1.Q(), Vector2.Q());
    } else {
      fmls(SubRegSize, DestTmp.D(), Vector1.D(), Vector2.D());
    }

    if (DestTmp != Dst) {
      if (Is128Bit) {
        mov(Dst.Q(), DestTmp.Q());
      } else {
        mov(Dst.D(), DestTmp.D());
      }
    }
  }
}

DEF_OP(VFCopySign) {
  auto Op = IROp->C<IR::IROp_VFCopySign>();
  const auto OpSize = IROp->Size;
  const auto SubRegSize = ConvertSubRegSize248(IROp);

  ARMEmitter::VRegister Magnitude = GetVReg(Op->Vector1);
  ARMEmitter::VRegister Sign = GetVReg(Op->Vector2);

  //  We don't assign explicity to Dst but Dst and Magniture are tied to the same register.
  //  Similar in semantics to C's copysignf.
  switch (OpSize) {
  case IR::OpSize::i64Bit:
    movi(SubRegSize, VTMP1.D(), 0x80, 24);
    bit(Magnitude.D(), Sign.D(), VTMP1.D());
    break;
  case IR::OpSize::i128Bit:
    movi(SubRegSize, VTMP1.Q(), 0x80, 24);
    bit(Magnitude.Q(), Sign.Q(), VTMP1.Q());
    break;
  default: LOGMAN_MSG_A_FMT("Unsupported element size for operation {}", __func__); FEX_UNREACHABLE;
  }
}

} // namespace FEXCore::CPU
