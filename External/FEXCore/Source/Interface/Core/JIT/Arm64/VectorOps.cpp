/*
$info$
tags: backend|arm64
$end_info$
*/

#include "Interface/Core/JIT/Arm64/JITClass.h"

namespace FEXCore::CPU {

using namespace vixl;
using namespace vixl::aarch64;
#define DEF_OP(x) void Arm64JITCore::Op_##x(IR::IROp_Header *IROp, IR::NodeID Node)
DEF_OP(VectorZero) {
  if (HostSupportsSVE) {
    const auto Dst = GetDst(Node).Z().VnD();
    eor(Dst, Dst, Dst);
  } else {
    const uint8_t OpSize = IROp->Size;

    switch (OpSize) {
      case 8: {
        const auto Dst = GetDst(Node).V8B();
        eor(Dst, Dst, Dst);
        break;
      }
      case 16: {
        const auto Dst = GetDst(Node).V16B();
        eor(Dst, Dst, Dst);
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Op Size: {}", OpSize);
        break;
    }
  }
}

DEF_OP(VectorImm) {
  auto Op = IROp->C<IR::IROp_VectorImm>();

  const uint8_t OpSize = IROp->Size;
  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;

  if (HostSupportsSVE) {
    const auto Dst = [&] {
      const auto Tmp = GetDst(Node).Z();
      switch (ElementSize) {
      case 1:
        return Tmp.VnB();
      case 2:
        return Tmp.VnH();
      case 4:
        return Tmp.VnS();
      case 8:
        return Tmp.VnD();
      default:
        LOGMAN_MSG_A_FMT("Unhandled element size: {}", ElementSize);
        return Tmp;
      }
    }();

    if (ElementSize > 1 && (Op->Immediate & 0x80)) {
      // SVE dup uses sign extension where VectorImm wants zext
      LoadConstant(TMP1.X(), Op->Immediate);
      dup(Dst, TMP1.X());
    }
    else {
      dup(Dst, static_cast<int8_t>(Op->Immediate));
    }
  } else {
    if (ElementSize == 8) {
      // movi with 64bit element size doesn't do what we want here
      LoadConstant(TMP1.X(), Op->Immediate);
      dup(GetDst(Node).V2D(), TMP1.X());
    }
    else {
      movi(GetDst(Node).VCast(OpSize * 8, Elements), Op->Immediate);
    }
  }
}

DEF_OP(VMov) {
  auto Op = IROp->C<IR::IROp_VMov>();
  const uint8_t OpSize = IROp->Size;

  const auto Dst = GetDst(Node);
  const auto Source = GetSrc(Op->Source.ID());

  switch (OpSize) {
    case 1: {
      if (HostSupportsSVE) {
        eor(VTMP1.Z().VnD(), VTMP1.Z().VnD(), VTMP1.Z().VnD());
      } else {
        eor(VTMP1.V16B(), VTMP1.V16B(), VTMP1.V16B());
      }

      mov(VTMP1.V16B(), 0, Source.V16B(), 0);

      if (HostSupportsSVE) {
        mov(Dst.Z().VnD(), VTMP1.Z().VnD());
      } else {
        mov(Dst, VTMP1);
      }
      break;
    }
    case 2: {
      if (HostSupportsSVE) {
        eor(VTMP1.Z().VnD(), VTMP1.Z().VnD(), VTMP1.Z().VnD());
      } else {
        eor(VTMP1.V16B(), VTMP1.V16B(), VTMP1.V16B());
      }

      mov(VTMP1.V8H(), 0, Source.V8H(), 0);

      if (HostSupportsSVE) {
        mov(Dst.Z().VnD(), VTMP1.Z().VnD());
      } else {
        mov(Dst, VTMP1);
      }
      break;
    }
    case 4: {
      if (HostSupportsSVE) {
        eor(VTMP1.Z().VnD(), VTMP1.Z().VnD(), VTMP1.Z().VnD());
      } else {
        eor(VTMP1.V16B(), VTMP1.V16B(), VTMP1.V16B());
      }

      mov(VTMP1.V4S(), 0, Source.V4S(), 0);

      if (HostSupportsSVE) {
        mov(Dst.Z().VnD(), VTMP1.Z().VnD());
      } else {
        mov(Dst, VTMP1);
      }
      break;
    }
    case 8: {
      if (HostSupportsSVE) {
        eor(VTMP1.Z().VnD(), VTMP1.Z().VnD(), VTMP1.Z().VnD());
        mov(VTMP1.V8B(), Source.V8B());
        mov(Dst.Z().VnB(), VTMP1.Z().VnB());
      } else {
        mov(Dst.V8B(), Source.V8B());
      }
      break;
    }
    case 16: {
      if (HostSupportsSVE) {
        eor(VTMP1.Z().VnD(), VTMP1.Z().VnD(), VTMP1.Z().VnD());
        mov(VTMP1.V16B(), Source.V16B());
        mov(Dst.Z().VnB(), VTMP1.Z().VnB());
      } else {
        if (Dst.GetCode() != Source.GetCode()) {
          mov(Dst.V16B(), Source.V16B());
        }
      }
      break;
    }
    case 32: {
      if (Dst.GetCode() != Source.GetCode()) {
        mov(Dst.Z().VnD(), Source.Z().VnD());
      }
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unknown Op Size: {}", OpSize);
      break;
  }
}

DEF_OP(VAnd) {
  auto Op = IROp->C<IR::IROp_VAnd>();

  const auto Dst = GetDst(Node);
  const auto Vector1 = GetSrc(Op->Vector1.ID());
  const auto Vector2 = GetSrc(Op->Vector2.ID());

  if (HostSupportsSVE) {
    and_(Dst.Z().VnD(), Vector1.Z().VnD(), Vector2.Z().VnD());
  } else {
    and_(Dst.V16B(), Vector1.V16B(), Vector2.V16B());
  }
}

DEF_OP(VBic) {
  auto Op = IROp->C<IR::IROp_VBic>();

  const auto Dst = GetDst(Node);
  const auto Vector1 = GetSrc(Op->Vector1.ID());
  const auto Vector2 = GetSrc(Op->Vector2.ID());

  if (HostSupportsSVE) {
    bic(Dst.Z().VnD(), Vector1.Z().VnD(), Vector2.Z().VnD());
  } else {
    bic(Dst.V16B(), Vector1.V16B(), Vector2.V16B());
  }
}

DEF_OP(VOr) {
  auto Op = IROp->C<IR::IROp_VOr>();

  const auto Dst = GetDst(Node);
  const auto Vector1 = GetSrc(Op->Vector1.ID());
  const auto Vector2 = GetSrc(Op->Vector2.ID());

  if (HostSupportsSVE) {
    orr(Dst.Z().VnD(), Vector1.Z().VnD(), Vector2.Z().VnD());
  } else {
    orr(Dst.V16B(), Vector1.V16B(), Vector2.V16B());
  }
}

DEF_OP(VXor) {
  auto Op = IROp->C<IR::IROp_VXor>();

  const auto Dst = GetDst(Node);
  const auto Vector1 = GetSrc(Op->Vector1.ID());
  const auto Vector2 = GetSrc(Op->Vector2.ID());

  if (HostSupportsSVE) {
    eor(Dst.Z().VnD(), Vector1.Z().VnD(), Vector2.Z().VnD());
  } else {
    eor(Dst.V16B(), Vector1.V16B(), Vector2.V16B());
  }
}

DEF_OP(VAdd) {
  auto Op = IROp->C<IR::IROp_VAdd>();

  const auto ElementSize = Op->Header.ElementSize;

  const auto Dst = GetDst(Node);
  const auto Vector1 = GetSrc(Op->Vector1.ID());
  const auto Vector2 = GetSrc(Op->Vector2.ID());

  switch (ElementSize) {
    case 1: {
      if (HostSupportsSVE) {
        add(Dst.Z().VnB(), Vector1.Z().VnB(), Vector2.Z().VnB());
      } else {
        add(Dst.V16B(), Vector1.V16B(), Vector2.V16B());
      }
      break;
    }
    case 2: {
      if (HostSupportsSVE) {
        add(Dst.Z().VnH(), Vector1.Z().VnH(), Vector2.Z().VnH());
      } else {
        add(Dst.V8H(), Vector1.V8H(), Vector2.V8H());
      }
      break;
    }
    case 4: {
      if (HostSupportsSVE) {
        add(Dst.Z().VnS(), Vector1.Z().VnS(), Vector2.Z().VnS());
      } else {
        add(Dst.V4S(), Vector1.V4S(), Vector2.V4S());
      }
      break;
    }
    case 8: {
      if (HostSupportsSVE) {
        add(Dst.Z().VnD(), Vector1.Z().VnD(), Vector2.Z().VnD());
      } else {
        add(Dst.V2D(), Vector1.V2D(), Vector2.V2D());
      }
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
}

DEF_OP(VSub) {
  auto Op = IROp->C<IR::IROp_VSub>();

  const auto ElementSize = Op->Header.ElementSize;

  const auto Dst = GetDst(Node);
  const auto Vector1 = GetSrc(Op->Vector1.ID());
  const auto Vector2 = GetSrc(Op->Vector2.ID());

  switch (ElementSize) {
    case 1: {
      if (HostSupportsSVE) {
        sub(Dst.Z().VnB(), Vector1.Z().VnB(), Vector2.Z().VnB());
      } else {
        sub(Dst.V16B(), Vector1.V16B(), Vector2.V16B());
      }
      break;
    }
    case 2: {
      if (HostSupportsSVE) {
        sub(Dst.Z().VnH(), Vector1.Z().VnH(), Vector2.Z().VnH());
      } else {
        sub(Dst.V8H(), Vector1.V8H(), Vector2.V8H());
      }
      break;
    }
    case 4: {
      if (HostSupportsSVE) {
        sub(Dst.Z().VnS(), Vector1.Z().VnS(), Vector2.Z().VnS());
      } else {
        sub(Dst.V4S(), Vector1.V4S(), Vector2.V4S());
      }
      break;
    }
    case 8: {
      if (HostSupportsSVE) {
        sub(Dst.Z().VnD(), Vector1.Z().VnD(), Vector2.Z().VnD());
      } else {
        sub(Dst.V2D(), Vector1.V2D(), Vector2.V2D());
      }
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
}

DEF_OP(VUQAdd) {
  auto Op = IROp->C<IR::IROp_VUQAdd>();

  const auto ElementSize = Op->Header.ElementSize;

  const auto Dst = GetDst(Node);
  const auto Vector1 = GetSrc(Op->Vector1.ID());
  const auto Vector2 = GetSrc(Op->Vector2.ID());

  switch (ElementSize) {
    case 1: {
      if (HostSupportsSVE) {
        uqadd(Dst.Z().VnB(), Vector1.Z().VnB(), Vector2.Z().VnB());
      } else {
        uqadd(Dst.V16B(), Vector1.V16B(), Vector2.V16B());
      }
      break;
    }
    case 2: {
      if (HostSupportsSVE) {
        uqadd(Dst.Z().VnH(), Vector1.Z().VnH(), Vector2.Z().VnH());
      } else {
        uqadd(Dst.V8H(), Vector1.V8H(), Vector2.V8H());
      }
      break;
    }
    case 4: {
      if (HostSupportsSVE) {
        uqadd(Dst.Z().VnS(), Vector1.Z().VnS(), Vector2.Z().VnS());
      } else {
        uqadd(Dst.V4S(), Vector1.V4S(), Vector2.V4S());
      }
      break;
    }
    case 8: {
      if (HostSupportsSVE) {
        uqadd(Dst.Z().VnD(), Vector1.Z().VnD(), Vector2.Z().VnD());
      } else {
        uqadd(Dst.V2D(), Vector1.V2D(), Vector2.V2D());
      }
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
}

DEF_OP(VUQSub) {
  auto Op = IROp->C<IR::IROp_VUQSub>();

  const auto ElementSize = Op->Header.ElementSize;

  const auto Dst = GetDst(Node);
  const auto Vector1 = GetSrc(Op->Vector1.ID());
  const auto Vector2 = GetSrc(Op->Vector2.ID());

  switch (ElementSize) {
    case 1: {
      if (HostSupportsSVE) {
        uqsub(Dst.Z().VnB(), Vector1.Z().VnB(), Vector2.Z().VnB());
      } else {
        uqsub(Dst.V16B(), Vector1.V16B(), Vector2.V16B());
      }
      break;
    }
    case 2: {
      if (HostSupportsSVE) {
        uqsub(Dst.Z().VnH(), Vector1.Z().VnH(), Vector2.Z().VnH());
      } else {
        uqsub(Dst.V8H(), Vector1.V8H(), Vector2.V8H());
      }
      break;
    }
    case 4: {
      if (HostSupportsSVE) {
        uqsub(Dst.Z().VnS(), Vector1.Z().VnS(), Vector2.Z().VnS());
      } else {
        uqsub(Dst.V4S(), Vector1.V4S(), Vector2.V4S());
      }
      break;
    }
    case 8: {
      if (HostSupportsSVE) {
        uqsub(Dst.Z().VnD(), Vector1.Z().VnD(), Vector2.Z().VnD());
      } else {
        uqsub(Dst.V2D(), Vector1.V2D(), Vector2.V2D());
      }
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
}

DEF_OP(VSQAdd) {
  auto Op = IROp->C<IR::IROp_VSQAdd>();

  const auto ElementSize = Op->Header.ElementSize;

  const auto Dst = GetDst(Node);
  const auto Vector1 = GetSrc(Op->Vector1.ID());
  const auto Vector2 = GetSrc(Op->Vector2.ID());

  switch (ElementSize) {
    case 1: {
      if (HostSupportsSVE) {
        sqadd(Dst.Z().VnB(), Vector1.Z().VnB(), Vector2.Z().VnB());
      } else {
        sqadd(Dst.V16B(), Vector1.V16B(), Vector2.V16B());
      }
      break;
    }
    case 2: {
      if (HostSupportsSVE) {
        sqadd(Dst.Z().VnH(), Vector1.Z().VnH(), Vector2.Z().VnH());
      } else {
        sqadd(Dst.V8H(), Vector1.V8H(), Vector2.V8H());
      }
      break;
    }
    case 4: {
      if (HostSupportsSVE) {
        sqadd(Dst.Z().VnS(), Vector1.Z().VnS(), Vector2.Z().VnS());
      } else {
        sqadd(Dst.V4S(), Vector1.V4S(), Vector2.V4S());
      }
      break;
    }
    case 8: {
      if (HostSupportsSVE) {
        sqadd(Dst.Z().VnD(), Vector1.Z().VnD(), Vector2.Z().VnD());
      } else {
        sqadd(Dst.V2D(), Vector1.V2D(), Vector2.V2D());
      }
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
}

DEF_OP(VSQSub) {
  auto Op = IROp->C<IR::IROp_VSQSub>();

  const auto ElementSize = Op->Header.ElementSize;

  const auto Dst = GetDst(Node);
  const auto Vector1 = GetSrc(Op->Vector1.ID());
  const auto Vector2 = GetSrc(Op->Vector2.ID());

  switch (ElementSize) {
    case 1: {
      if (HostSupportsSVE) {
        sqsub(Dst.Z().VnB(), Vector1.Z().VnB(), Vector2.Z().VnB());
      } else {
        sqsub(Dst.V16B(), Vector1.V16B(), Vector2.V16B());
      }
      break;
    }
    case 2: {
      if (HostSupportsSVE) {
        sqsub(Dst.Z().VnH(), Vector1.Z().VnH(), Vector2.Z().VnH());
      } else {
        sqsub(Dst.V8H(), Vector1.V8H(), Vector2.V8H());
      }
      break;
    }
    case 4: {
      if (HostSupportsSVE) {
        sqsub(Dst.Z().VnS(), Vector1.Z().VnS(), Vector2.Z().VnS());
      } else {
        sqsub(Dst.V4S(), Vector1.V4S(), Vector2.V4S());
      }
      break;
    }
    case 8: {
      if (HostSupportsSVE) {
        sqsub(Dst.Z().VnD(), Vector1.Z().VnD(), Vector2.Z().VnD());
      } else {
        sqsub(Dst.V2D(), Vector1.V2D(), Vector2.V2D());
      }
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
}

DEF_OP(VAddP) {
  const auto Op = IROp->C<IR::IROp_VAddP>();
  const auto OpSize = IROp->Size;
  const auto IsScalar = OpSize == 8;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto ElementSize = Op->Header.ElementSize;

  const auto Dst = GetDst(Node);
  const auto VectorLower = GetSrc(Op->VectorLower.ID());
  const auto VectorUpper = GetSrc(Op->VectorUpper.ID());

  if (HostSupportsSVE && Is256Bit && !IsScalar) {
    const auto Pred = PRED_TMP_32B.Merging();

    // SVE ADDP is a destructive operation, so we need a temporary
    eor(VTMP1.Z().VnD(), VTMP1.Z().VnD(), VTMP1.Z().VnD());
    mov(VTMP1.Z().VnD(), Pred, VectorLower.Z().VnD());

    // Unlike Adv. SIMD's version of ADDP, which acts like it concats the
    // upper vector onto the end of the lower vector and then performs
    // pairwise addition, the SVE version actually interleaves the
    // results of the pairwise addition (gross!), so we need to undo that.
    switch (ElementSize) {
      case 1: {
        addp(VTMP1.Z().VnB(), Pred, VTMP1.Z().VnB(), VectorUpper.Z().VnB());
        uzp1(VTMP2.Z().VnB(), VTMP1.Z().VnB(), VTMP1.Z().VnB());
        uzp2(VTMP3.Z().VnB(), VTMP1.Z().VnB(), VTMP1.Z().VnB());
        break;
      }
      case 2: {
        addp(VTMP1.Z().VnH(), Pred, VTMP1.Z().VnH(), VectorUpper.Z().VnH());
        uzp1(VTMP2.Z().VnH(), VTMP1.Z().VnH(), VTMP1.Z().VnH());
        uzp2(VTMP3.Z().VnH(), VTMP1.Z().VnH(), VTMP1.Z().VnH());
        break;
      }
      case 4: {
        addp(VTMP1.Z().VnS(), Pred, VTMP1.Z().VnS(), VectorUpper.Z().VnS());
        uzp1(VTMP2.Z().VnS(), VTMP1.Z().VnS(), VTMP1.Z().VnS());
        uzp2(VTMP3.Z().VnS(), VTMP1.Z().VnS(), VTMP1.Z().VnS());
        break;
      }
      case 8: {
        addp(VTMP1.Z().VnD(), Pred, VTMP1.Z().VnD(), VectorUpper.Z().VnD());
        uzp1(VTMP2.Z().VnD(), VTMP1.Z().VnD(), VTMP1.Z().VnD());
        uzp2(VTMP3.Z().VnD(), VTMP1.Z().VnD(), VTMP1.Z().VnD());
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        return;
    }

    // Shift the entire vector over by 128 bits.
    mov(TMP1, 0);
    insr(VTMP3.Z().VnD(), TMP1.X());
    insr(VTMP3.Z().VnD(), TMP1.X());

    // Now combine the lower and upper halves.
    orr(Dst.Z().VnD(), VTMP2.Z().VnD(), VTMP3.Z().VnD());
  } else {
    if (IsScalar) {
      switch (ElementSize) {
        case 1: {
          addp(Dst.V8B(), VectorLower.V8B(), VectorUpper.V8B());
          break;
        }
        case 2: {
          addp(Dst.V4H(), VectorLower.V4H(), VectorUpper.V4H());
          break;
        }
        case 4: {
          addp(Dst.V2S(), VectorLower.V2S(), VectorUpper.V2S());
          break;
        }
        default:
          LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
          break;
      }
    } else {
      switch (ElementSize) {
        case 1: {
          addp(Dst.V16B(), VectorLower.V16B(), VectorUpper.V16B());
          break;
        }
        case 2: {
          addp(Dst.V8H(), VectorLower.V8H(), VectorUpper.V8H());
          break;
        }
        case 4: {
          addp(Dst.V4S(), VectorLower.V4S(), VectorUpper.V4S());
          break;
        }
        case 8: {
          addp(Dst.V2D(), VectorLower.V2D(), VectorUpper.V2D());
          break;
        }
        default:
          LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
          break;
      }
    }
  }
}

DEF_OP(VAddV) {
  auto Op = IROp->C<IR::IROp_VAddV>();
  const uint8_t OpSize = IROp->Size;
  const uint8_t Elements = OpSize / Op->Header.ElementSize;
  // Vector
  switch (Op->Header.ElementSize) {
    case 1:
    case 2:
    case 4:
      addv(GetDst(Node).VCast(Op->Header.ElementSize * 8, 1), GetSrc(Op->Vector.ID()).VCast(OpSize * 8, Elements));
      break;
    case 8:
      addp(GetDst(Node).VCast(OpSize * 8, 1), GetSrc(Op->Vector.ID()).VCast(OpSize * 8, Elements));
      break;
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
}

DEF_OP(VUMinV) {
  auto Op = IROp->C<IR::IROp_VUMinV>();

  const auto OpSize = IROp->Size;
  const auto ElementSize = Op->Header.ElementSize;
  const auto Elements = OpSize / ElementSize;

  const auto Dst = GetDst(Node);
  const auto Vector = GetSrc(Op->Vector.ID());

  if (HostSupportsSVE) {
    LOGMAN_THROW_AA_FMT(OpSize == 16 || OpSize == 32,
                        "Unsupported vector length: {}", OpSize);

    const auto Pred = OpSize == 16 ? PRED_TMP_16B
                                   : PRED_TMP_32B;

    switch (ElementSize) {
      case 1:
        uminv(Dst.B(), Pred, Vector.Z().VnB());
        break;
      case 2:
        uminv(Dst.H(), Pred, Vector.Z().VnH());
        break;
      case 4:
        uminv(Dst.S(), Pred, Vector.Z().VnS());
        break;
      case 8:
        uminv(Dst.D(), Pred, Vector.Z().VnD());
        break;
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        return;
    }
  } else {
    // Vector
    switch (ElementSize) {
      case 1:
      case 2:
      case 4:
        uminv(Dst.VCast(ElementSize * 8, 1), Vector.VCast(OpSize * 8, Elements));
        break;
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
    }
  }
}

DEF_OP(VURAvg) {
  const auto Op = IROp->C<IR::IROp_VURAvg>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;

  const auto Dst = GetDst(Node);
  const auto Vector1 = GetSrc(Op->Vector1.ID());
  const auto Vector2 = GetSrc(Op->Vector2.ID());

  if (HostSupportsSVE && OpSize == 32) {
    // SVE URHADD is a destructive operation, so we need
    // a temporary for performing operations.
    mov(VTMP1.Z().VnD(), Vector1.Z().VnD());

    switch (ElementSize) {
      case 1: {
        urhadd(VTMP1.Z().VnB(), PRED_TMP_32B.Merging(),
               VTMP1.Z().VnB(), Vector2.Z().VnB());
        break;
      }
      case 2: {
        urhadd(VTMP1.Z().VnH(), PRED_TMP_32B.Merging(),
               VTMP1.Z().VnH(), Vector2.Z().VnH());
        break;
      }
      case 4: {
        urhadd(VTMP1.Z().VnS(), PRED_TMP_32B.Merging(),
               VTMP1.Z().VnS(), Vector2.Z().VnS());
        break;
      }
      case 8: {
        urhadd(VTMP1.Z().VnD(), PRED_TMP_32B.Merging(),
               VTMP1.Z().VnD(), Vector2.Z().VnD());
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        return;
    }

    mov(Dst.Z().VnD(), VTMP1.Z().VnD());
  } else {
    switch (ElementSize) {
      case 1: {
        urhadd(Dst.V16B(), Vector1.V16B(), Vector2.V16B());
        break;
      }
      case 2: {
        urhadd(Dst.V8H(), Vector1.V8H(), Vector2.V8H());
        break;
      }
      case 4: {
        urhadd(Dst.V4S(), Vector1.V4S(), Vector2.V4S());
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
    }
  }
}

DEF_OP(VAbs) {
  const auto Op = IROp->C<IR::IROp_VAbs>();
  const auto OpSize = IROp->Size;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / Op->Header.ElementSize;

  const auto Dst = GetDst(Node);
  const auto Src = GetSrc(Op->Vector.ID());

  if (HostSupportsSVE && OpSize == 32) {
    switch (ElementSize) {
      case 1: {
        abs(Dst.Z().VnB(), PRED_TMP_32B.Merging(), Src.Z().VnB());
        break;
      }
      case 2: {
        abs(Dst.Z().VnH(), PRED_TMP_32B.Merging(), Src.Z().VnH());
        break;
      }
      case 4: {
        abs(Dst.Z().VnS(), PRED_TMP_32B.Merging(), Src.Z().VnS());
        break;
      }
      case 8: {
        abs(Dst.Z().VnD(), PRED_TMP_32B.Merging(), Src.Z().VnD());
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
    }
  } else {
    if (ElementSize == OpSize) {
      // Scalar
      switch (ElementSize) {
        case 8: {
          abs(Dst.D(), Src.D());
          break;
        }
        default:
          LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
          break;
      }
    } else {
      // Vector
      switch (ElementSize) {
        case 1:
        case 2:
        case 4:
        case 8:
          abs(Dst.VCast(OpSize * 8, Elements), Src.VCast(OpSize * 8, Elements));
          break;
        default:
          LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
          break;
      }
    }
  }
}

DEF_OP(VPopcount) {
  const auto Op = IROp->C<IR::IROp_VPopcount>();
  const auto OpSize = IROp->Size;
  const bool IsScalar = OpSize == 8;

  const auto ElementSize = Op->Header.ElementSize;

  const auto Dst = GetDst(Node);
  const auto Src = GetSrc(Op->Vector.ID());

  if (HostSupportsSVE && !IsScalar) {
    const auto Pred = OpSize == 16 ? PRED_TMP_16B.Merging()
                                   : PRED_TMP_32B.Merging();

    switch (ElementSize) {
      case 1:
        cnt(Dst.Z().VnB(), Pred, Src.Z().VnB());
        break;
      case 2:
        cnt(Dst.Z().VnH(), Pred, Src.Z().VnH());
        break;
      case 4:
        cnt(Dst.Z().VnS(), Pred, Src.Z().VnS());
        break;
      case 8:
        cnt(Dst.Z().VnD(), Pred, Src.Z().VnD());
        break;
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
    }
  } else {
    if (IsScalar) {
      // Scalar
      switch (ElementSize) {
        case 1: {
          cnt(Dst.V8B(), Src.V8B());
          break;
        }
        default:
          LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
          break;
      }
    } else {
      // Vector
      switch (ElementSize) {
        case 1:
          cnt(Dst.V16B(), Src.V16B());
          break;
        default:
          LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
          break;
      }
    }
  }
}

DEF_OP(VFAdd) {
  const auto Op = IROp->C<IR::IROp_VFAdd>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto IsScalar = ElementSize == OpSize;

  const auto Dst = GetDst(Node);
  const auto Vector1 = GetSrc(Op->Vector1.ID());
  const auto Vector2 = GetSrc(Op->Vector2.ID());

  if (HostSupportsSVE && !IsScalar) {
    switch (ElementSize) {
      case 2: {
        fadd(Dst.Z().VnH(), Vector1.Z().VnH(), Vector2.Z().VnH());
        break;
      }
      case 4: {
        fadd(Dst.Z().VnS(), Vector1.Z().VnS(), Vector2.Z().VnS());
        break;
      }
      case 8: {
        fadd(Dst.Z().VnD(), Vector1.Z().VnD(), Vector2.Z().VnD());
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
    }
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
      switch (ElementSize) {
        case 2: {
          fadd(Dst.V8H(), Vector1.V8H(), Vector2.V8H());
          break;
        }
        case 4: {
          fadd(Dst.V4S(), Vector1.V4S(), Vector2.V4S());
          break;
        }
        case 8: {
          fadd(Dst.V2D(), Vector1.V2D(), Vector2.V2D());
          break;
        }
        default:
          LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
          break;
      }
    }
  }
}

DEF_OP(VFAddP) {
  const auto Op = IROp->C<IR::IROp_VFAddP>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;

  const auto Dst = GetDst(Node);
  const auto VectorLower = GetSrc(Op->VectorLower.ID());
  const auto VectorUpper = GetSrc(Op->VectorUpper.ID());

  const bool Is256Bit = OpSize == 32;

  if (HostSupportsSVE && Is256Bit) {
    const auto Pred = PRED_TMP_32B.Merging();

    // SVE FADDP is a destructive operation, so we need a temporary
    eor(VTMP1.Z().VnD(), VTMP1.Z().VnD(), VTMP1.Z().VnD());
    mov(VTMP1.Z().VnD(), Pred, VectorLower.Z().VnD());

    // Unlike Adv. SIMD's version of FADDP, which acts like it concats the
    // upper vector onto the end of the lower vector and then performs
    // pairwise addition, the SVE version actually interleaves the
    // results of the pairwise addition (gross!), so we need to undo that.
    switch (ElementSize) {
      case 2: {
        faddp(VTMP1.Z().VnH(), Pred, VTMP1.Z().VnH(), VectorUpper.Z().VnH());
        uzp1(VTMP2.Z().VnH(), VTMP1.Z().VnH(), VTMP1.Z().VnH());
        uzp2(VTMP3.Z().VnH(), VTMP1.Z().VnH(), VTMP1.Z().VnH());
        break;
      }
      case 4: {
        faddp(VTMP1.Z().VnS(), Pred, VTMP1.Z().VnS(), VectorUpper.Z().VnS());
        uzp1(VTMP2.Z().VnS(), VTMP1.Z().VnS(), VTMP1.Z().VnS());
        uzp2(VTMP3.Z().VnS(), VTMP1.Z().VnS(), VTMP1.Z().VnS());
        break;
      }
      case 8: {
        faddp(VTMP1.Z().VnD(), Pred, VTMP1.Z().VnD(), VectorUpper.Z().VnD());
        uzp1(VTMP2.Z().VnD(), VTMP1.Z().VnD(), VTMP1.Z().VnD());
        uzp2(VTMP3.Z().VnD(), VTMP1.Z().VnD(), VTMP1.Z().VnD());
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        return;
    }

    // Shift the entire vector over by 64 bits (128-bit vector case)
    // or by 128 bits (256-bit vector case).
    mov(TMP1, 0);
    insr(VTMP3.Z().VnD(), TMP1);
    insr(VTMP3.Z().VnD(), TMP1);

    // Now combine the lower and upper halves.
    orr(Dst.Z().VnD(), VTMP2.Z().VnD(), VTMP3.Z().VnD());
  } else {
    switch (ElementSize) {
      case 2: {
        faddp(Dst.V8H(), VectorLower.V8H(), VectorUpper.V8H());
        break;
      }
      case 4: {
        faddp(Dst.V4S(), VectorLower.V4S(), VectorUpper.V4S());
        break;
      }
      case 8: {
        faddp(Dst.V2D(), VectorLower.V2D(), VectorUpper.V2D());
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
    }
  }
}

DEF_OP(VFSub) {
  const auto Op = IROp->C<IR::IROp_VFSub>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto IsScalar = ElementSize == OpSize;

  const auto Dst = GetDst(Node);
  const auto Vector1 = GetSrc(Op->Vector1.ID());
  const auto Vector2 = GetSrc(Op->Vector2.ID());

  if (HostSupportsSVE && !IsScalar) {
    switch (ElementSize) {
      case 2: {
        fsub(Dst.Z().VnH(), Vector1.Z().VnH(), Vector2.Z().VnH());
        break;
      }
      case 4: {
        fsub(Dst.Z().VnS(), Vector1.Z().VnS(), Vector2.Z().VnS());
        break;
      }
      case 8: {
        fsub(Dst.Z().VnD(), Vector1.Z().VnD(), Vector2.Z().VnD());
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
    }
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
      switch (ElementSize) {
        case 2: {
          fsub(Dst.V8H(), Vector1.V8H(), Vector2.V8H());
          break;
        }
        case 4: {
          fsub(Dst.V4S(), Vector1.V4S(), Vector2.V4S());
          break;
        }
        case 8: {
          fsub(Dst.V2D(), Vector1.V2D(), Vector2.V2D());
          break;
        }
        default:
          LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
          break;
      }
    }
  }
}

DEF_OP(VFMul) {
  const auto Op = IROp->C<IR::IROp_VFMul>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto IsScalar = ElementSize == OpSize;

  const auto Dst = GetDst(Node);
  const auto Vector1 = GetSrc(Op->Vector1.ID());
  const auto Vector2 = GetSrc(Op->Vector2.ID());

  if (HostSupportsSVE && !IsScalar) {
    switch (ElementSize) {
      case 2: {
        fmul(Dst.Z().VnH(), Vector1.Z().VnH(), Vector2.Z().VnH());
        break;
      }
      case 4: {
        fmul(Dst.Z().VnS(), Vector1.Z().VnS(), Vector2.Z().VnS());
        break;
      }
      case 8: {
        fmul(Dst.Z().VnD(), Vector1.Z().VnD(), Vector2.Z().VnD());
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
    }
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
      switch (ElementSize) {
        case 2: {
          fmul(Dst.V8H(), Vector1.V8H(), Vector2.V8H());
          break;
        }
        case 4: {
          fmul(Dst.V4S(), Vector1.V4S(), Vector2.V4S());
          break;
        }
        case 8: {
          fmul(Dst.V2D(), Vector1.V2D(), Vector2.V2D());
          break;
        }
        default:
          LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
          break;
      }
    }
  }
}

DEF_OP(VFDiv) {
  const auto Op = IROp->C<IR::IROp_VFDiv>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto IsScalar = ElementSize == OpSize;
  const auto Is256Bit = OpSize == 32;

  const auto Dst = GetDst(Node);
  const auto Vector1 = GetSrc(Op->Vector1.ID());
  const auto Vector2 = GetSrc(Op->Vector2.ID());

  if (HostSupportsSVE && Is256Bit && !IsScalar) {
    // SVE VDIV is a destructive operation, so we need a temporary.
    mov(VTMP1.Z().VnD(), Vector1.Z().VnD());

    switch (ElementSize) {
      case 2: {
        fdiv(VTMP1.Z().VnH(), PRED_TMP_32B.Merging(),
             VTMP1.Z().VnH(), Vector2.Z().VnH());
        break;
      }
      case 4: {
        fdiv(VTMP1.Z().VnS(), PRED_TMP_32B.Merging(),
             VTMP1.Z().VnS(), Vector2.Z().VnS());
        break;
      }
      case 8: {
        fdiv(VTMP1.Z().VnD(), PRED_TMP_32B.Merging(),
             VTMP1.Z().VnD(), Vector2.Z().VnD());
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        return;
    }

    mov(Dst.Z().VnD(), VTMP1.Z().VnD());
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
      switch (ElementSize) {
        case 2: {
          fdiv(Dst.V8H(), Vector1.V8H(), Vector2.V8H());
          break;
        }
        case 4: {
          fdiv(Dst.V4S(), Vector1.V4S(), Vector2.V4S());
          break;
        }
        case 8: {
          fdiv(Dst.V2D(), Vector1.V2D(), Vector2.V2D());
          break;
        }
        default:
          LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
          break;
      }
    }
  }
}

DEF_OP(VFMin) {
  const auto Op = IROp->C<IR::IROp_VFMin>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto IsScalar = ElementSize == OpSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto Vector1 = GetSrc(Op->Vector1.ID());
  const auto Vector2 = GetSrc(Op->Vector2.ID());

  // NOTE: We don't directly use FMIN here for any of the implementations,
  //       because it has undesirable NaN handling behavior (it sets
  //       entries either to the incoming NaN value*, or the default NaN
  //       depending on FPCR flags set). We want behavior that sets NaN
  //       entries to zero for the comparison result.
  //
  // * - Not exactly (differs slightly with SNaNs), but close enough for the explanation

  if (HostSupportsSVE && !IsScalar) {
    const auto Mask = Is256Bit ? PRED_TMP_32B
                               : PRED_TMP_16B;
    const auto ComparePred = p0;

    // General idea:
    // 1. Compare greater than against the two vectors
    // 2. Invert the resulting values in the predicate register.
    // 3. Move the first vector into a temporary
    // 4. Merge all the elements that correspond to the inverted
    //    predicate bits from the second vector into the
    //    same temporary.
    // 5. Move temporary into the destination register and we're done.

    switch (ElementSize) {
      case 2: {
        fcmgt(ComparePred.VnH(), Mask.Zeroing(),
              Vector2.Z().VnH(), Vector1.Z().VnH());
        not_(ComparePred.VnB(), Mask.Zeroing(), ComparePred.VnB());
        mov(VTMP1.Z().VnD(), Vector1.Z().VnD());
        mov(VTMP1.Z().VnH(), ComparePred.Merging(), Vector2.Z().VnH());
        break;
      }
      case 4: {
        fcmgt(ComparePred.VnS(), Mask.Zeroing(),
              Vector2.Z().VnS(), Vector1.Z().VnS());
        not_(ComparePred.VnB(), Mask.Zeroing(), ComparePred.VnB());
        mov(VTMP1.Z().VnD(), Vector1.Z().VnD());
        mov(VTMP1.Z().VnS(), ComparePred.Merging(), Vector2.Z().VnS());
        break;
      }
      case 8: {
        fcmgt(ComparePred.VnD(), Mask.Zeroing(),
              Vector2.Z().VnD(), Vector1.Z().VnD());
        not_(ComparePred.VnB(), Mask.Zeroing(), ComparePred.VnB());
        mov(VTMP1.Z().VnD(), Vector1.Z().VnD());
        mov(VTMP1.Z().VnD(), ComparePred.Merging(), Vector2.Z().VnD());
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        return;
    }

    mov(Dst.Z().VnD(), VTMP1.Z().VnD());
  } else {
    if (IsScalar) {
      switch (ElementSize) {
        case 2: {
          fcmp(Vector1.H(), Vector2.H());
          fcsel(Dst.H(), Vector1.H(), Vector2.H(), Condition::mi);
          break;
        }
        case 4: {
          fcmp(Vector1.S(), Vector2.S());
          fcsel(Dst.S(), Vector1.S(), Vector2.S(), Condition::mi);
          break;
        }
        case 8: {
          fcmp(Vector1.D(), Vector2.D());
          fcsel(Dst.D(), Vector1.D(), Vector2.D(), Condition::mi);
          break;
        }
        default:
          LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
          break;
      }
    } else {
      switch (ElementSize) {
        case 2: {
          fcmgt(VTMP1.V8H(), Vector2.V8H(), Vector1.V8H());
          mov(VTMP2.V8H(), Vector1.V8H());
          bif(VTMP2.V16B(), Vector2.V16B(), VTMP1.V16B());
          mov(Dst.V8H(), VTMP2.V8H());
          break;
        }
        case 4: {
          fcmgt(VTMP1.V4S(), Vector2.V4S(), Vector1.V4S());
          mov(VTMP2.V4S(), Vector1.V4S());
          bif(VTMP2.V16B(), Vector2.V16B(), VTMP1.V16B());
          mov(Dst.V4S(), VTMP2.V4S());
          break;
        }
        case 8: {
          fcmgt(VTMP1.V2D(), Vector2.V2D(), Vector1.V2D());
          mov(VTMP2.V2D(), Vector1.V2D());
          bif(VTMP2.V16B(), Vector2.V16B(), VTMP1.V16B());
          mov(Dst.V2D(), VTMP2.V2D());
          break;
        }
        default:
          LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
          break;
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

  const auto Dst = GetDst(Node);
  const auto Vector1 = GetSrc(Op->Vector1.ID());
  const auto Vector2 = GetSrc(Op->Vector2.ID());

  // NOTE: See VFMin implementation for reasons why we
  //       don't just use FMAX/FMIN for these implementations.

  if (HostSupportsSVE && !IsScalar) {
    const auto Mask = Is256Bit ? PRED_TMP_32B
                               : PRED_TMP_16B;
    const auto ComparePred = p0;

    switch (ElementSize) {
      case 2: {
        fcmgt(ComparePred.VnH(), Mask.Zeroing(),
              Vector2.Z().VnH(), Vector1.Z().VnH());
        mov(VTMP1.Z().VnD(), Vector1.Z().VnD());
        mov(VTMP1.Z().VnH(), ComparePred.Merging(), Vector2.Z().VnH());
        break;
      }
      case 4: {
        fcmgt(ComparePred.VnS(), Mask.Zeroing(),
              Vector2.Z().VnS(), Vector1.Z().VnS());
        mov(VTMP1.Z().VnD(), Vector1.Z().VnD());
        mov(VTMP1.Z().VnS(), ComparePred.Merging(), Vector2.Z().VnS());
        break;
      }
      case 8: {
        fcmgt(ComparePred.VnD(), Mask.Zeroing(),
              Vector2.Z().VnD(), Vector1.Z().VnD());
        mov(VTMP1.Z().VnD(), Vector1.Z().VnD());
        mov(VTMP1.Z().VnD(), ComparePred.Merging(), Vector2.Z().VnD());
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        return;
    }

    mov(Dst.Z().VnD(), VTMP1.Z().VnD());
  } else {
    if (IsScalar) {
      switch (ElementSize) {
        case 2: {
          fcmp(Vector1.H(), Vector2.H());
          fcsel(Dst.H(), Vector2.H(), Vector1.H(), Condition::mi);
          break;
        }
        case 4: {
          fcmp(Vector1.S(), Vector2.S());
          fcsel(Dst.S(), Vector2.S(), Vector1.S(), Condition::mi);
          break;
        }
        case 8: {
          fcmp(Vector1.D(), Vector2.D());
          fcsel(Dst.D(), Vector2.D(), Vector1.D(), Condition::mi);
          break;
        }
        default:
          LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
          break;
      }
    } else {
      switch (ElementSize) {
        case 2: {
          fcmgt(VTMP1.V8H(), Vector2.V8H(), Vector1.V8H());
          mov(VTMP2.V8H(), Vector1.V8H());
          bit(VTMP2.V16B(), Vector2.V16B(), VTMP1.V16B());
          mov(Dst.V8H(), VTMP2.V8H());
          break;
        }
        case 4: {
          fcmgt(VTMP1.V4S(), Vector2.V4S(), Vector1.V4S());
          mov(VTMP2.V4S(), Vector1.V4S());
          bit(VTMP2.V16B(), Vector2.V16B(), VTMP1.V16B());
          mov(Dst.V4S(), VTMP2.V4S());
          break;
        }
        case 8: {
          fcmgt(VTMP1.V2D(), Vector2.V2D(), Vector1.V2D());
          mov(VTMP2.V2D(), Vector1.V2D());
          bit(VTMP2.V16B(), Vector2.V16B(), VTMP1.V16B());
          mov(Dst.V2D(), VTMP2.V2D());
          break;
        }
        default:
          LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
          break;
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

  const auto Dst = GetDst(Node);
  const auto Vector = GetSrc(Op->Vector.ID());

  if (HostSupportsSVE && !IsScalar) {
    const auto Pred = Is256Bit ? PRED_TMP_32B.Merging()
                               : PRED_TMP_16B.Merging();

    switch (ElementSize) {
      case 2: {
        fmov(VTMP1.Z().VnH(), 1.0);
        fdiv(VTMP1.Z().VnH(), Pred, VTMP1.Z().VnH(), Vector.Z().VnH());
        break;
      }
      case 4: {
        fmov(VTMP1.Z().VnS(), 1.0);
        fdiv(VTMP1.Z().VnS(), Pred, VTMP1.Z().VnS(), Vector.Z().VnS());
        break;
      }
      case 8: {
        fmov(VTMP1.Z().VnD(), 1.0);
        fdiv(VTMP1.Z().VnD(), Pred, VTMP1.Z().VnD(), Vector.Z().VnD());
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        return;
    }

    mov(Dst.Z().VnD(), VTMP1.Z().VnD());
  } else {
    if (IsScalar) {
      switch (ElementSize) {
        case 2: {
          fmov(VTMP1.H(), Float16{1.0});
          fdiv(Dst.H(), VTMP1.H(), Vector.H());
          break;
        }
        case 4: {
          fmov(VTMP1.S(), 1.0f);
          fdiv(Dst.S(), VTMP1.S(), Vector.S());
          break;
        }
        case 8: {
          fmov(VTMP1.D(), 1.0);
          fdiv(Dst.D(), VTMP1.D(), Vector.D());
          break;
        }
        default:
          LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
          break;
      }
    } else {
      switch (ElementSize) {
        case 2: {
          fmov(VTMP1.V8H(), Float16{1.0});
          fdiv(Dst.V8H(), VTMP1.V8H(), Vector.V8H());
          break;
        }
        case 4: {
          fmov(VTMP1.V4S(), 1.0f);
          fdiv(Dst.V4S(), VTMP1.V4S(), Vector.V4S());
          break;
        }
        case 8: {
          fmov(VTMP1.V2D(), 1.0);
          fdiv(Dst.V2D(), VTMP1.V2D(), Vector.V2D());
          break;
        }
        default:
          LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
          break;
      }
    }
  }
}

DEF_OP(VFSqrt) {
  const auto Op = IROp->C<IR::IROp_VFRSqrt>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto IsScalar = ElementSize == OpSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto Vector = GetSrc(Op->Vector.ID());

  if (HostSupportsSVE && !IsScalar) {
    const auto Pred = Is256Bit ? PRED_TMP_32B.Merging()
                               : PRED_TMP_16B.Merging();

    switch (ElementSize) {
      case 2: {
        fsqrt(Dst.Z().VnH(), Pred, Vector.Z().VnH());
        break;
      }
      case 4: {
        fsqrt(Dst.Z().VnS(), Pred, Vector.Z().VnS());
        break;
      }
      case 8: {
        fsqrt(Dst.Z().VnD(), Pred, Vector.Z().VnD());
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
    }
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
      switch (ElementSize) {
        case 2: {
          fsqrt(Dst.V8H(), Vector.V8H());
          break;
        }
        case 4: {
          fsqrt(Dst.V4S(), Vector.V4S());
          break;
        }
        case 8: {
          fsqrt(Dst.V2D(), Vector.V2D());
          break;
        }
        default:
          LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
          break;
      }
    }
  }
}

DEF_OP(VFRSqrt) {
  const auto Op = IROp->C<IR::IROp_VFRSqrt>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto IsScalar = ElementSize == OpSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto Vector = GetSrc(Op->Vector.ID());

  if (HostSupportsSVE && Is256Bit && !IsScalar) {
    const auto Pred = PRED_TMP_32B.Merging();

    switch (ElementSize) {
      case 2: {
        fmov(VTMP1.Z().VnH(), 1.0);
        fsqrt(VTMP2.Z().VnH(), Pred, Vector.Z().VnH());
        fdiv(VTMP1.Z().VnH(), Pred, VTMP1.Z().VnH(), VTMP2.Z().VnH());
        break;
      }
      case 4: {
        fmov(VTMP1.Z().VnS(), 1.0);
        fsqrt(VTMP2.Z().VnS(), Pred, Vector.Z().VnS());
        fdiv(VTMP1.Z().VnS(), Pred, VTMP1.Z().VnS(), VTMP2.Z().VnS());
        break;
      }
      case 8: {
        fmov(VTMP1.Z().VnD(), 1.0);
        fsqrt(VTMP2.Z().VnD(), Pred, Vector.Z().VnD());
        fdiv(VTMP1.Z().VnD(), Pred, VTMP1.Z().VnD(), VTMP2.Z().VnD());
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        return;
    }

    mov(Dst.Z().VnD(), VTMP1.Z().VnD());
  } else {
    if (IsScalar) {
      switch (ElementSize) {
        case 2: {
          fmov(VTMP1.H(), Float16{1.0});
          fsqrt(VTMP2.H(), Vector.H());
          fdiv(Dst.H(), VTMP1.H(), VTMP2.H());
          break;
        }
        case 4: {
          fmov(VTMP1.S(), 1.0f);
          fsqrt(VTMP2.S(), Vector.S());
          fdiv(Dst.S(), VTMP1.S(), VTMP2.S());
          break;
        }
        case 8: {
          fmov(VTMP1.D(), 1.0);
          fsqrt(VTMP2.D(), Vector.D());
          fdiv(Dst.D(), VTMP1.D(), VTMP2.D());
          break;
        }
        default:
          LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
          break;
      }
    } else {
      switch (ElementSize) {
        case 2: {
          fmov(VTMP1.V8H(), Float16{1.0});
          fsqrt(VTMP2.V8H(), Vector.V8H());
          fdiv(Dst.V8H(), VTMP1.V8H(), VTMP2.V8H());
          break;
        }
        case 4: {
          fmov(VTMP1.V4S(), 1.0f);
          fsqrt(VTMP2.V4S(), Vector.V4S());
          fdiv(Dst.V4S(), VTMP1.V4S(), VTMP2.V4S());
          break;
        }
        case 8: {
          fmov(VTMP1.V2D(), 1.0);
          fsqrt(VTMP2.V2D(), Vector.V2D());
          fdiv(Dst.V2D(), VTMP1.V2D(), VTMP2.V2D());
          break;
        }
        default:
          LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
          break;
      }
    }
  }
}

DEF_OP(VNeg) {
  const auto Op = IROp->C<IR::IROp_VNeg>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto Vector = GetSrc(Op->Vector.ID());

  if (HostSupportsSVE) {
    const auto Pred = Is256Bit ? PRED_TMP_32B.Merging()
                               : PRED_TMP_16B.Merging();

    switch (ElementSize) {
      case 1:
        neg(Dst.Z().VnB(), Pred, Vector.Z().VnB());
        break;
      case 2:
        neg(Dst.Z().VnH(), Pred, Vector.Z().VnH());
        break;
      case 4:
        neg(Dst.Z().VnS(), Pred, Vector.Z().VnS());
        break;
      case 8:
        neg(Dst.Z().VnD(), Pred, Vector.Z().VnD());
        break;
      default:
        LOGMAN_MSG_A_FMT("Unsupported VNeg size: {}", ElementSize);
        break;
    }
  } else {
    switch (ElementSize) {
    case 1:
      neg(Dst.V16B(), Vector.V16B());
      break;
    case 2:
      neg(Dst.V8H(), Vector.V8H());
      break;
    case 4:
      neg(Dst.V4S(), Vector.V4S());
      break;
    case 8:
      neg(Dst.V2D(), Vector.V2D());
      break;
    default:
      LOGMAN_MSG_A_FMT("Unsupported VNeg size: {}", ElementSize);
      break;
    }
  }
}

DEF_OP(VFNeg) {
  const auto Op = IROp->C<IR::IROp_VFNeg>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto Vector = GetSrc(Op->Vector.ID());

  if (HostSupportsSVE) {
    const auto Pred = Is256Bit ? PRED_TMP_32B.Merging()
                               : PRED_TMP_16B.Merging();

    switch (ElementSize) {
      case 2:
        fneg(Dst.Z().VnH(), Pred, Vector.Z().VnH());
        break;
      case 4:
        fneg(Dst.Z().VnS(), Pred, Vector.Z().VnS());
        break;
      case 8:
        fneg(Dst.Z().VnD(), Pred, Vector.Z().VnD());
        break;
      default:
        LOGMAN_MSG_A_FMT("Unsupported VFNeg element size: {}", ElementSize);
        break;
    }
  } else {
    switch (ElementSize) {
      case 2:
        fneg(Dst.V8H(), Vector.V8H());
        break;
      case 4:
        fneg(Dst.V4S(), Vector.V4S());
        break;
      case 8:
        fneg(Dst.V2D(), Vector.V2D());
        break;
      default:
        LOGMAN_MSG_A_FMT("Unsupported VFNeg element size: {}", ElementSize);
        break;
    }
  }
}

DEF_OP(VNot) {
  const auto Op = IROp->C<IR::IROp_VNot>();
  const auto OpSize = IROp->Size;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto Vector = GetSrc(Op->Vector.ID());

  if (HostSupportsSVE && Is256Bit) {
    not_(Dst.Z().VnB(), PRED_TMP_32B.Merging(), Vector.Z().VnB());
  } else {
    mvn(Dst.V16B(), Vector.V16B());
  }
}

DEF_OP(VUMin) {
  const auto Op = IROp->C<IR::IROp_VUMin>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto IsScalar = ElementSize == OpSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto Vector1 = GetSrc(Op->Vector1.ID());
  const auto Vector2 = GetSrc(Op->Vector2.ID());

  if (HostSupportsSVE && Is256Bit && !IsScalar) {
    const auto Pred = PRED_TMP_32B.Merging();

    // SVE UMIN is a destructive operation so we need a temporary.
    mov(VTMP1.Z().VnD(), Vector1.Z().VnD());

    switch (ElementSize) {
      case 1: {
        umin(VTMP1.Z().VnB(), Pred, VTMP1.Z().VnB(), Vector2.Z().VnB());
        break;
      }
      case 2: {
        umin(VTMP1.Z().VnH(), Pred, VTMP1.Z().VnH(), Vector2.Z().VnH());
        break;
      }
      case 4: {
        umin(VTMP1.Z().VnS(), Pred, VTMP1.Z().VnS(), Vector2.Z().VnS());
        break;
      }
      case 8: {
        umin(VTMP1.Z().VnD(), Pred, VTMP1.Z().VnD(), Vector2.Z().VnD());
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        return;
    }

    mov(Dst.Z().VnD(), VTMP1.Z().VnD());
  } else {
    switch (ElementSize) {
      case 1: {
        umin(Dst.V16B(), Vector1.V16B(), Vector2.V16B());
        break;
      }
      case 2: {
        umin(Dst.V8H(), Vector1.V8H(), Vector2.V8H());
        break;
      }
      case 4: {
        umin(Dst.V4S(), Vector1.V4S(), Vector2.V4S());
        break;
      }
      case 8: {
        cmhi(VTMP1.V2D(), Vector2.V2D(), Vector1.V2D());
        mov(VTMP2.V2D(), Vector1.V2D());
        bif(VTMP2.V16B(), Vector2.V16B(), VTMP1.V16B());
        mov(Dst.V2D(), VTMP2.V2D());
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
  const auto IsScalar = ElementSize == OpSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto Vector1 = GetSrc(Op->Vector1.ID());
  const auto Vector2 = GetSrc(Op->Vector2.ID());

  if (HostSupportsSVE && Is256Bit && !IsScalar) {
    const auto Pred = PRED_TMP_32B.Merging();

    // SVE SMIN is a destructive operation, so we need a temporary.
    mov(VTMP1.Z().VnD(), Vector1.Z().VnD());

    switch (ElementSize) {
      case 1: {
        smin(VTMP1.Z().VnB(), Pred, VTMP1.Z().VnB(), Vector2.Z().VnB());
        break;
      }
      case 2: {
        smin(VTMP1.Z().VnH(), Pred, VTMP1.Z().VnH(), Vector2.Z().VnH());
        break;
      }
      case 4: {
        smin(VTMP1.Z().VnS(), Pred, VTMP1.Z().VnS(), Vector2.Z().VnS());
        break;
      }
      case 8: {
        smin(VTMP1.Z().VnD(), Pred, VTMP1.Z().VnD(), Vector2.Z().VnD());
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        return;
    }

    mov(Dst.Z().VnD(), VTMP1.Z().VnD());
  } else {
    switch (ElementSize) {
      case 1: {
        smin(Dst.V16B(), Vector1.V16B(), Vector2.V16B());
        break;
      }
      case 2: {
        smin(Dst.V8H(), Vector1.V8H(), Vector2.V8H());
        break;
      }
      case 4: {
        smin(Dst.V4S(), Vector1.V4S(), Vector2.V4S());
        break;
      }
      case 8: {
        cmgt(VTMP1.V2D(), Vector2.V2D(), Vector1.V2D());
        mov(VTMP2.V2D(), Vector1.V2D());
        bif(VTMP2.V16B(), Vector2.V16B(), VTMP1.V16B());
        mov(Dst.V2D(), VTMP2.V2D());
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
  const auto IsScalar = ElementSize == OpSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto Vector1 = GetSrc(Op->Vector1.ID());
  const auto Vector2 = GetSrc(Op->Vector2.ID());

  if (HostSupportsSVE && Is256Bit && !IsScalar) {
    const auto Pred = PRED_TMP_32B.Merging();

    // SVE UMAX is a destructive operation, so we need a temporary.
    mov(VTMP1.Z().VnD(), Vector1.Z().VnD());

    switch (ElementSize) {
      case 1: {
        umax(VTMP1.Z().VnB(), Pred, VTMP1.Z().VnB(), Vector2.Z().VnB());
        break;
      }
      case 2: {
        umax(VTMP1.Z().VnH(), Pred, VTMP1.Z().VnH(), Vector2.Z().VnH());
        break;
      }
      case 4: {
        umax(VTMP1.Z().VnS(), Pred, VTMP1.Z().VnS(), Vector2.Z().VnS());
        break;
      }
      case 8: {
        umax(VTMP1.Z().VnD(), Pred, VTMP1.Z().VnD(), Vector2.Z().VnD());
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        return;
    }

    mov(Dst.Z().VnD(), VTMP1.Z().VnD());
  } else {
    switch (ElementSize) {
      case 1: {
        umax(Dst.V16B(), Vector1.V16B(), Vector2.V16B());
        break;
      }
      case 2: {
        umax(Dst.V8H(), Vector1.V8H(), Vector2.V8H());
        break;
      }
      case 4: {
        umax(Dst.V4S(), Vector1.V4S(), Vector2.V4S());
        break;
      }
      case 8: {
        cmhi(VTMP1.V2D(), Vector2.V2D(), Vector1.V2D());
        mov(VTMP2.V2D(), Vector1.V2D());
        bit(VTMP2.V16B(), Vector2.V16B(), VTMP1.V16B());
        mov(Dst.V2D(), VTMP2.V2D());
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
  const auto IsScalar = ElementSize == OpSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto Vector1 = GetSrc(Op->Vector1.ID());
  const auto Vector2 = GetSrc(Op->Vector2.ID());

  if (HostSupportsSVE && Is256Bit && !IsScalar) {
    const auto Pred = PRED_TMP_32B.Merging();

    // SVE SMAX is a destructive operation, so we need a temporary.
    mov(VTMP1.Z().VnD(), Vector1.Z().VnD());

    switch (ElementSize) {
      case 1: {
        smax(VTMP1.Z().VnB(), Pred, VTMP1.Z().VnB(), Vector2.Z().VnB());
        break;
      }
      case 2: {
        smax(VTMP1.Z().VnH(), Pred, VTMP1.Z().VnH(), Vector2.Z().VnH());
        break;
      }
      case 4: {
        smax(VTMP1.Z().VnS(), Pred, VTMP1.Z().VnS(), Vector2.Z().VnS());
        break;
      }
      case 8: {
        smax(VTMP1.Z().VnD(), Pred, VTMP1.Z().VnD(), Vector2.Z().VnD());
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        return;
    }

    mov(Dst.Z().VnD(), VTMP1.Z().VnD());
  } else {
    switch (ElementSize) {
      case 1: {
        smax(Dst.V16B(), Vector1.V16B(), Vector2.V16B());
        break;
      }
      case 2: {
        smax(Dst.V8H(), Vector1.V8H(), Vector2.V8H());
        break;
      }
      case 4: {
        smax(Dst.V4S(), Vector1.V4S(), Vector2.V4S());
        break;
      }
      case 8: {
        cmgt(VTMP1.V2D(), Vector2.V2D(), Vector1.V2D());
        mov(VTMP2.V2D(), Vector1.V2D());
        bit(VTMP2.V16B(), Vector2.V16B(), VTMP1.V16B());
        mov(Dst.V2D(), VTMP2.V2D());
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
    }
  }
}

DEF_OP(VZip) {
  auto Op = IROp->C<IR::IROp_VZip>();
  const uint8_t OpSize = IROp->Size;
  if (OpSize == 8) {
    switch (Op->Header.ElementSize) {
      case 1: {
        zip1(GetDst(Node).V8B(), GetSrc(Op->VectorLower.ID()).V8B(), GetSrc(Op->VectorUpper.ID()).V8B());
      break;
      }
      case 2: {
        zip1(GetDst(Node).V4H(), GetSrc(Op->VectorLower.ID()).V4H(), GetSrc(Op->VectorUpper.ID()).V4H());
      break;
      }
      case 4: {
        zip1(GetDst(Node).V2S(), GetSrc(Op->VectorLower.ID()).V2S(), GetSrc(Op->VectorUpper.ID()).V2S());
      break;
      }
      default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
    }
  }
  else {
    switch (Op->Header.ElementSize) {
      case 1: {
        zip1(GetDst(Node).V16B(), GetSrc(Op->VectorLower.ID()).V16B(), GetSrc(Op->VectorUpper.ID()).V16B());
      break;
      }
      case 2: {
        zip1(GetDst(Node).V8H(), GetSrc(Op->VectorLower.ID()).V8H(), GetSrc(Op->VectorUpper.ID()).V8H());
      break;
      }
      case 4: {
        zip1(GetDst(Node).V4S(), GetSrc(Op->VectorLower.ID()).V4S(), GetSrc(Op->VectorUpper.ID()).V4S());
      break;
      }
      case 8: {
        zip1(GetDst(Node).V2D(), GetSrc(Op->VectorLower.ID()).V2D(), GetSrc(Op->VectorUpper.ID()).V2D());
      break;
      }
      default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
    }
  }
}

DEF_OP(VZip2) {
  auto Op = IROp->C<IR::IROp_VZip2>();
  const uint8_t OpSize = IROp->Size;
  if (OpSize == 8) {
    switch (Op->Header.ElementSize) {
    case 1: {
      zip2(GetDst(Node).V8B(), GetSrc(Op->VectorLower.ID()).V8B(), GetSrc(Op->VectorUpper.ID()).V8B());
    break;
    }
    case 2: {
      zip2(GetDst(Node).V4H(), GetSrc(Op->VectorLower.ID()).V4H(), GetSrc(Op->VectorUpper.ID()).V4H());
    break;
    }
    case 4: {
      zip2(GetDst(Node).V2S(), GetSrc(Op->VectorLower.ID()).V2S(), GetSrc(Op->VectorUpper.ID()).V2S());
    break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
    }
  }
  else {
    switch (Op->Header.ElementSize) {
    case 1: {
      zip2(GetDst(Node).V16B(), GetSrc(Op->VectorLower.ID()).V16B(), GetSrc(Op->VectorUpper.ID()).V16B());
    break;
    }
    case 2: {
      zip2(GetDst(Node).V8H(), GetSrc(Op->VectorLower.ID()).V8H(), GetSrc(Op->VectorUpper.ID()).V8H());
    break;
    }
    case 4: {
      zip2(GetDst(Node).V4S(), GetSrc(Op->VectorLower.ID()).V4S(), GetSrc(Op->VectorUpper.ID()).V4S());
    break;
    }
    case 8: {
      zip2(GetDst(Node).V2D(), GetSrc(Op->VectorLower.ID()).V2D(), GetSrc(Op->VectorUpper.ID()).V2D());
    break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
    }
  }
}

DEF_OP(VUnZip) {
  auto Op = IROp->C<IR::IROp_VUnZip>();
  const uint8_t OpSize = IROp->Size;
  if (OpSize == 8) {
    switch (Op->Header.ElementSize) {
      case 1: {
        uzp1(GetDst(Node).V8B(), GetSrc(Op->VectorLower.ID()).V8B(), GetSrc(Op->VectorUpper.ID()).V8B());
      break;
      }
      case 2: {
        uzp1(GetDst(Node).V4H(), GetSrc(Op->VectorLower.ID()).V4H(), GetSrc(Op->VectorUpper.ID()).V4H());
      break;
      }
      case 4: {
        uzp1(GetDst(Node).V2S(), GetSrc(Op->VectorLower.ID()).V2S(), GetSrc(Op->VectorUpper.ID()).V2S());
      break;
      }
      default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
    }
  }
  else {
    switch (Op->Header.ElementSize) {
      case 1: {
        uzp1(GetDst(Node).V16B(), GetSrc(Op->VectorLower.ID()).V16B(), GetSrc(Op->VectorUpper.ID()).V16B());
      break;
      }
      case 2: {
        uzp1(GetDst(Node).V8H(), GetSrc(Op->VectorLower.ID()).V8H(), GetSrc(Op->VectorUpper.ID()).V8H());
      break;
      }
      case 4: {
        uzp1(GetDst(Node).V4S(), GetSrc(Op->VectorLower.ID()).V4S(), GetSrc(Op->VectorUpper.ID()).V4S());
      break;
      }
      case 8: {
        uzp1(GetDst(Node).V2D(), GetSrc(Op->VectorLower.ID()).V2D(), GetSrc(Op->VectorUpper.ID()).V2D());
      break;
      }
      default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
    }
  }
}

DEF_OP(VUnZip2) {
  auto Op = IROp->C<IR::IROp_VUnZip2>();
  uint8_t OpSize = IROp->Size;
  if (OpSize == 8) {
    switch (Op->Header.ElementSize) {
    case 1: {
      uzp2(GetDst(Node).V8B(), GetSrc(Op->VectorLower.ID()).V8B(), GetSrc(Op->VectorUpper.ID()).V8B());
    break;
    }
    case 2: {
      uzp2(GetDst(Node).V4H(), GetSrc(Op->VectorLower.ID()).V4H(), GetSrc(Op->VectorUpper.ID()).V4H());
    break;
    }
    case 4: {
      uzp2(GetDst(Node).V2S(), GetSrc(Op->VectorLower.ID()).V2S(), GetSrc(Op->VectorUpper.ID()).V2S());
    break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
    }
  }
  else {
    switch (Op->Header.ElementSize) {
    case 1: {
      uzp2(GetDst(Node).V16B(), GetSrc(Op->VectorLower.ID()).V16B(), GetSrc(Op->VectorUpper.ID()).V16B());
    break;
    }
    case 2: {
      uzp2(GetDst(Node).V8H(), GetSrc(Op->VectorLower.ID()).V8H(), GetSrc(Op->VectorUpper.ID()).V8H());
    break;
    }
    case 4: {
      uzp2(GetDst(Node).V4S(), GetSrc(Op->VectorLower.ID()).V4S(), GetSrc(Op->VectorUpper.ID()).V4S());
    break;
    }
    case 8: {
      uzp2(GetDst(Node).V2D(), GetSrc(Op->VectorLower.ID()).V2D(), GetSrc(Op->VectorUpper.ID()).V2D());
    break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
    }
  }
}

DEF_OP(VBSL) {
  const auto Op = IROp->C<IR::IROp_VBSL>();
  const auto OpSize = IROp->Size;

  const auto Dst = GetDst(Node);
  const auto VectorFalse = GetSrc(Op->VectorFalse.ID());
  const auto VectorTrue = GetSrc(Op->VectorTrue.ID());
  const auto VectorMask = GetSrc(Op->VectorMask.ID());

  if (HostSupportsSVE) {
    // NOTE: Slight parameter difference from ASIMD
    //       ASIMD -> BSL Mask, True, False
    //       SVE   -> BSL True, True, False, Mask
    mov(VTMP1.Z().VnD(), VectorTrue.Z().VnD());
    bsl(VTMP1.Z().VnD(), VTMP1.Z().VnD(), VectorFalse.Z().VnD(), VectorMask.Z().VnD());
    mov(Dst.Z().VnD(), VTMP1.Z().VnD());
  } else {
    if (OpSize == 8) {
      mov(VTMP1.V8B(), VectorMask.V8B());
      bsl(VTMP1.V8B(), VectorTrue.V8B(), VectorFalse.V8B());
      mov(Dst.V8B(), VTMP1.V8B());
    } else {
      mov(VTMP1.V16B(), VectorMask.V16B());
      bsl(VTMP1.V16B(), VectorTrue.V16B(), VectorFalse.V16B());
      mov(Dst.V16B(), VTMP1.V16B());
    }
  }
}

DEF_OP(VCMPEQ) {
  const auto Op = IROp->C<IR::IROp_VCMPEQ>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto IsScalar = ElementSize == OpSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto Vector1 = GetSrc(Op->Vector1.ID());
  const auto Vector2 = GetSrc(Op->Vector2.ID());

  if (HostSupportsSVE && Is256Bit && !IsScalar) {
    const auto Mask = PRED_TMP_32B.Zeroing();
    const auto ComparePred = p0;

    // Ensure no junk is in the temp (important for ensuring
    // non-equal entries remain as zero during the final bitwise OR). 
    eor(VTMP1.Z().VnD(), VTMP1.Z().VnD(), VTMP1.Z().VnD());

    // General idea is to compare for equality, not the equal vals
    // from one of the registers, then or both together to make the
    // relevant equal entries all 1s.

    switch (ElementSize) {
      case 1: {
        cmpeq(ComparePred.VnB(), Mask, Vector1.Z().VnB(), Vector2.Z().VnB());
        not_(VTMP1.Z().VnB(), ComparePred.Merging(), Vector1.Z().VnB());
        orr(VTMP1.Z().VnB(), ComparePred.Merging(), VTMP1.Z().VnB(), Vector1.Z().VnB());
        break;
      }
      case 2: {
        cmpeq(ComparePred.VnH(), Mask, Vector1.Z().VnH(), Vector2.Z().VnH());
        not_(VTMP1.Z().VnH(), ComparePred.Merging(), Vector1.Z().VnH());
        orr(VTMP1.Z().VnH(), ComparePred.Merging(), VTMP1.Z().VnH(), Vector1.Z().VnH());
        break;
      }
      case 4: {
        cmpeq(ComparePred.VnS(), Mask, Vector1.Z().VnS(), Vector2.Z().VnS());
        not_(VTMP1.Z().VnS(), ComparePred.Merging(), Vector1.Z().VnS());
        orr(VTMP1.Z().VnS(), ComparePred.Merging(), VTMP1.Z().VnS(), Vector1.Z().VnS());
        break;
      }
      case 8: {
        cmpeq(ComparePred.VnD(), Mask, Vector1.Z().VnD(), Vector2.Z().VnD());
        not_(VTMP1.Z().VnD(), ComparePred.Merging(), Vector1.Z().VnD());
        orr(VTMP1.Z().VnD(), ComparePred.Merging(), VTMP1.Z().VnD(), Vector1.Z().VnD());
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        return;
    }

    mov(Dst.Z().VnD(), VTMP1.Z().VnD());
  } else {
    if (IsScalar) {
      switch (ElementSize) {
        case 4: {
          cmeq(Dst.S(), Vector1.S(), Vector2.S());
          break;
        }
        case 8: {
          cmeq(Dst.D(), Vector1.D(), Vector2.D());
          break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
      }
    } else {
      switch (ElementSize) {
        case 1: {
          cmeq(Dst.V16B(), Vector1.V16B(), Vector2.V16B());
          break;
        }
        case 2: {
          cmeq(Dst.V8H(), Vector1.V8H(), Vector2.V8H());
          break;
        }
        case 4: {
          cmeq(Dst.V4S(), Vector1.V4S(), Vector2.V4S());
          break;
        }
        case 8: {
          cmeq(Dst.V2D(), Vector1.V2D(), Vector2.V2D());
          break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
      }
    }
  }
}

DEF_OP(VCMPEQZ) {
  const auto Op = IROp->C<IR::IROp_VCMPEQZ>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto IsScalar = ElementSize == OpSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto Vector = GetSrc(Op->Vector.ID());

  if (HostSupportsSVE && Is256Bit && !IsScalar) {
    const auto Mask = PRED_TMP_32B.Zeroing();
    const auto ComparePred = p0;

    // Ensure no junk is in the temp (important for ensuring
    // non-equal entries remain as zero).
    eor(VTMP1.Z().VnD(), VTMP1.Z().VnD(), VTMP1.Z().VnD());

    // Unlike with VCMPEQ, we can skip needing to bitwise OR the
    // final results, since if our elements are equal to zero,
    // we just need to bitwise NOT them and they're already set
    // to all 1s.
    switch (ElementSize) {
      case 1: {
        cmpeq(ComparePred.VnB(), Mask, Vector.Z().VnB(), 0);
        not_(VTMP1.Z().VnB(), ComparePred.Merging(), Vector.Z().VnB());
        break;
      }
      case 2: {
        cmpeq(ComparePred.VnH(), Mask, Vector.Z().VnH(), 0);
        not_(VTMP1.Z().VnH(), ComparePred.Merging(), Vector.Z().VnH());
        break;
      }
      case 4: {
        cmpeq(ComparePred.VnS(), Mask, Vector.Z().VnS(), 0);
        not_(VTMP1.Z().VnS(), ComparePred.Merging(), Vector.Z().VnS());
        break;
      }
      case 8: {
        cmpeq(ComparePred.VnD(), Mask, Vector.Z().VnD(), 0);
        not_(VTMP1.Z().VnD(), ComparePred.Merging(), Vector.Z().VnD());
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        return;
    }

    mov(Dst.Z().VnD(), VTMP1.Z().VnD());
  } else {
    if (IsScalar) {
      switch (ElementSize) {
        case 4: {
          cmeq(Dst.S(), Vector.S(), 0);
          break;
        }
        case 8: {
          cmeq(Dst.D(), Vector.D(), 0);
          break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
      }
    } else {
      switch (ElementSize) {
        case 1: {
          cmeq(Dst.V16B(), Vector.V16B(), 0);
          break;
        }
        case 2: {
          cmeq(Dst.V8H(), Vector.V8H(), 0);
          break;
        }
        case 4: {
          cmeq(Dst.V4S(), Vector.V4S(), 0);
          break;
        }
        case 8: {
          cmeq(Dst.V2D(), Vector.V2D(), 0);
          break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
      }
    }
  }
}

DEF_OP(VCMPGT) {
  const auto Op = IROp->C<IR::IROp_VCMPGT>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto IsScalar = ElementSize == OpSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto Vector1 = GetSrc(Op->Vector1.ID());
  const auto Vector2 = GetSrc(Op->Vector2.ID());

  if (HostSupportsSVE && Is256Bit && !IsScalar) {
    const auto Mask = PRED_TMP_32B.Zeroing();
    const auto ComparePred = p0;

    // Ensure no junk is in the temp (important for ensuring
    // non greater-than values remain as zero).
    eor(VTMP1.Z().VnD(), VTMP1.Z().VnD(), VTMP1.Z().VnD());

    // General idea is to compare for greater-than, bitwise NOT
    // the valid values, then ORR the NOTed values with the original
    // values to form entries that are all 1s.

    switch (ElementSize) {
      case 1: {
        cmpgt(ComparePred.VnB(), Mask, Vector1.Z().VnB(), Vector2.Z().VnB());
        not_(VTMP1.Z().VnB(), ComparePred.Merging(), Vector1.Z().VnB());
        orr(VTMP1.Z().VnB(), ComparePred.Merging(), VTMP1.Z().VnB(), Vector1.Z().VnB());
        break;
      }
      case 2: {
        cmpgt(ComparePred.VnH(), Mask, Vector1.Z().VnH(), Vector2.Z().VnH());
        not_(VTMP1.Z().VnH(), ComparePred.Merging(), Vector1.Z().VnH());
        orr(VTMP1.Z().VnH(), ComparePred.Merging(), VTMP1.Z().VnH(), Vector1.Z().VnH());
        break;
      }
      case 4: {
        cmpgt(ComparePred.VnS(), Mask, Vector1.Z().VnS(), Vector2.Z().VnS());
        not_(VTMP1.Z().VnS(), ComparePred.Merging(), Vector1.Z().VnS());
        orr(VTMP1.Z().VnS(), ComparePred.Merging(), VTMP1.Z().VnS(), Vector1.Z().VnS());
        break;
      }
      case 8: {
        cmpgt(ComparePred.VnD(), Mask, Vector1.Z().VnD(), Vector2.Z().VnD());
        not_(VTMP1.Z().VnD(), ComparePred.Merging(), Vector1.Z().VnD());
        orr(VTMP1.Z().VnD(), ComparePred.Merging(), VTMP1.Z().VnD(), Vector1.Z().VnD());
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        return;
    }

    mov(Dst.Z().VnD(), VTMP1.Z().VnD());
  } else {
    if (IsScalar) {
      switch (ElementSize) {
        case 4: {
          cmgt(Dst.S(), Vector1.S(), Vector2.S());
          break;
        }
        case 8: {
          cmgt(Dst.D(), Vector1.D(), Vector2.D());
          break;
        }
        default:
          LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
          break;
      }
    } else {
      switch (ElementSize) {
        case 1: {
          cmgt(Dst.V16B(), Vector1.V16B(), Vector2.V16B());
          break;
        }
        case 2: {
          cmgt(Dst.V8H(), Vector1.V8H(), Vector2.V8H());
          break;
        }
        case 4: {
          cmgt(Dst.V4S(), Vector1.V4S(), Vector2.V4S());
          break;
        }
        case 8: {
          cmgt(Dst.V2D(), Vector1.V2D(), Vector2.V2D());
          break;
        }
        default:
          LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
          break;
      }
    }
  }
}

DEF_OP(VCMPGTZ) {
  const auto Op = IROp->C<IR::IROp_VCMPGTZ>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto IsScalar = ElementSize == OpSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto Vector = GetSrc(Op->Vector.ID());

  if (HostSupportsSVE && Is256Bit && !IsScalar) {
    const auto Mask = PRED_TMP_32B.Zeroing();
    const auto ComparePred = p0;

    // Ensure no junk is in the temp (important for ensuring
    // non greater-than values remain as zero).
    eor(VTMP1.Z().VnD(), VTMP1.Z().VnD(), VTMP1.Z().VnD());

    switch (ElementSize) {
      case 1: {
        cmpgt(ComparePred.VnB(), Mask, Vector.Z().VnB(), 0);
        not_(VTMP1.Z().VnB(), ComparePred.Merging(), Vector.Z().VnB());
        orr(VTMP1.Z().VnB(), ComparePred.Merging(), VTMP1.Z().VnB(), Vector.Z().VnB());
        break;
      }
      case 2: {
        cmpgt(ComparePred.VnH(), Mask, Vector.Z().VnH(), 0);
        not_(VTMP1.Z().VnH(), ComparePred.Merging(), Vector.Z().VnH());
        orr(VTMP1.Z().VnH(), ComparePred.Merging(), VTMP1.Z().VnH(), Vector.Z().VnH());
        break;
      }
      case 4: {
        cmpgt(ComparePred.VnS(), Mask, Vector.Z().VnS(), 0);
        not_(VTMP1.Z().VnS(), ComparePred.Merging(), Vector.Z().VnS());
        orr(VTMP1.Z().VnS(), ComparePred.Merging(), VTMP1.Z().VnS(), Vector.Z().VnS());
        break;
      }
      case 8: {
        cmpgt(ComparePred.VnD(), Mask, Vector.Z().VnD(), 0);
        not_(VTMP1.Z().VnD(), ComparePred.Merging(), Vector.Z().VnD());
        orr(VTMP1.Z().VnD(), ComparePred.Merging(), VTMP1.Z().VnD(), Vector.Z().VnD());
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        return;
    }

    mov(Dst.Z().VnD(), VTMP1.Z().VnD());
  } else {
    if (IsScalar) {
      switch (ElementSize) {
        case 4: {
          cmgt(Dst.S(), Vector.S(), 0);
          break;
        }
        case 8: {
          cmgt(Dst.D(), Vector.D(), 0);
          break;
        }
        default:
          LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
          break;
      }
    } else {
      switch (ElementSize) {
        case 1: {
          cmgt(Dst.V16B(), Vector.V16B(), 0);
          break;
        }
        case 2: {
          cmgt(Dst.V8H(), Vector.V8H(), 0);
          break;
        }
        case 4: {
          cmgt(Dst.V4S(), Vector.V4S(), 0);
          break;
        }
        case 8: {
          cmgt(Dst.V2D(), Vector.V2D(), 0);
          break;
        }
        default:
          LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
          break;
      }
    }
  }
}

DEF_OP(VCMPLTZ) {
  const auto Op = IROp->C<IR::IROp_VCMPLTZ>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto IsScalar = ElementSize == OpSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto Vector = GetSrc(Op->Vector.ID());

  if (HostSupportsSVE && Is256Bit && !IsScalar) {
    const auto Mask = PRED_TMP_32B.Zeroing();
    const auto ComparePred = p0;

    // Ensure no junk is in the temp (important for ensuring
    // non less-than values remain as zero).
    eor(VTMP1.Z().VnD(), VTMP1.Z().VnD(), VTMP1.Z().VnD());

    switch (ElementSize) {
      case 1: {
        cmplt(ComparePred.VnB(), Mask, Vector.Z().VnB(), 0);
        not_(VTMP1.Z().VnB(), ComparePred.Merging(), Vector.Z().VnB());
        orr(VTMP1.Z().VnB(), ComparePred.Merging(), VTMP1.Z().VnB(), Vector.Z().VnB());
        break;
      }
      case 2: {
        cmplt(ComparePred.VnH(), Mask, Vector.Z().VnH(), 0);
        not_(VTMP1.Z().VnH(), ComparePred.Merging(), Vector.Z().VnH());
        orr(VTMP1.Z().VnH(), ComparePred.Merging(), VTMP1.Z().VnH(), Vector.Z().VnH());
        break;
      }
      case 4: {
        cmplt(ComparePred.VnS(), Mask, Vector.Z().VnS(), 0);
        not_(VTMP1.Z().VnS(), ComparePred.Merging(), Vector.Z().VnS());
        orr(VTMP1.Z().VnS(), ComparePred.Merging(), VTMP1.Z().VnS(), Vector.Z().VnS());
        break;
      }
      case 8: {
        cmplt(ComparePred.VnD(), Mask, Vector.Z().VnD(), 0);
        not_(VTMP1.Z().VnD(), ComparePred.Merging(), Vector.Z().VnD());
        orr(VTMP1.Z().VnD(), ComparePred.Merging(), VTMP1.Z().VnD(), Vector.Z().VnD());
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        return;
    }

    mov(Dst.Z().VnD(), VTMP1.Z().VnD());
  } else {
    if (IsScalar) {
      switch (ElementSize) {
        case 4: {
          cmlt(Dst.S(), Vector.S(), 0);
          break;
        }
        case 8: {
          cmlt(Dst.D(), Vector.D(), 0);
          break;
        }
        default:
          LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
          break;
      }
    } else {
      switch (ElementSize) {
        case 1: {
          cmlt(Dst.V16B(), Vector.V16B(), 0);
          break;
        }
        case 2: {
          cmlt(Dst.V8H(), Vector.V8H(), 0);
          break;
        }
        case 4: {
          cmlt(Dst.V4S(), Vector.V4S(), 0);
          break;
        }
        case 8: {
          cmlt(Dst.V2D(), Vector.V2D(), 0);
          break;
        }
        default:
          LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
          break;
      }
    }
  }
}

DEF_OP(VFCMPEQ) {
  const auto Op = IROp->C<IR::IROp_VFCMPEQ>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto IsScalar = ElementSize == OpSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto Vector1 = GetSrc(Op->Vector1.ID());
  const auto Vector2 = GetSrc(Op->Vector2.ID());

  if (HostSupportsSVE && Is256Bit && !IsScalar) {
    const auto Mask = PRED_TMP_32B.Zeroing();
    const auto ComparePred = p0;

    // Ensure we have no junk in the temporary.
    eor(VTMP1.Z().VnD(), VTMP1.Z().VnD(), VTMP1.Z().VnD());

    switch (ElementSize) {
      case 2: {
        fcmeq(ComparePred.VnH(), Mask, Vector1.Z().VnH(), Vector2.Z().VnH());
        not_(VTMP1.Z().VnH(), ComparePred.Merging(), Vector1.Z().VnH());
        orr(VTMP1.Z().VnH(), ComparePred.Merging(), VTMP1.Z().VnH(), Vector1.Z().VnH());
        break;
      }
      case 4: {
        fcmeq(ComparePred.VnS(), Mask, Vector1.Z().VnS(), Vector2.Z().VnS());
        not_(VTMP1.Z().VnS(), ComparePred.Merging(), Vector1.Z().VnS());
        orr(VTMP1.Z().VnS(), ComparePred.Merging(), VTMP1.Z().VnS(), Vector1.Z().VnS());
        break;
      }
      case 8: {
        fcmeq(ComparePred.VnD(), Mask, Vector1.Z().VnD(), Vector2.Z().VnD());
        not_(VTMP1.Z().VnD(), ComparePred.Merging(), Vector1.Z().VnD());
        orr(VTMP1.Z().VnD(), ComparePred.Merging(), VTMP1.Z().VnD(), Vector1.Z().VnD());
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        return;
    }

    mov(Dst.Z().VnD(), VTMP1.Z().VnD());
  } else {
    if (IsScalar) {
      switch (ElementSize) {
        case 2: {
          fcmeq(Dst.H(), Vector1.H(), Vector2.H());
          break;
        }
        case 4: {
          fcmeq(Dst.S(), Vector1.S(), Vector2.S());
          break;
        }
        case 8: {
          fcmeq(Dst.D(), Vector1.D(), Vector2.D());
          break;
        }
        default:
          LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
          break;
      }
    } else {
      switch (ElementSize) {
        case 2: {
          fcmeq(Dst.V8H(), Vector1.V8H(), Vector2.V8H());
          break;
        }
        case 4: {
          fcmeq(Dst.V4S(), Vector1.V4S(), Vector2.V4S());
          break;
        }
        case 8: {
          fcmeq(Dst.V2D(), Vector1.V2D(), Vector2.V2D());
          break;
        }
        default:
          LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
          break;
      }
    }
  }
}

DEF_OP(VFCMPNEQ) {
  const auto Op = IROp->C<IR::IROp_VFCMPNEQ>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto IsScalar = ElementSize == OpSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto Vector1 = GetSrc(Op->Vector1.ID());
  const auto Vector2 = GetSrc(Op->Vector2.ID());

  if (HostSupportsSVE && Is256Bit && !IsScalar) {
    const auto Mask = PRED_TMP_32B.Zeroing();
    const auto ComparePred = p0;

    // Ensure we have no junk in the temporary.
    eor(VTMP1.Z().VnD(), VTMP1.Z().VnD(), VTMP1.Z().VnD());

    switch (ElementSize) {
      case 2: {
        fcmne(ComparePred.VnH(), Mask, Vector1.Z().VnH(), Vector2.Z().VnH());
        not_(VTMP1.Z().VnH(), ComparePred.Merging(), Vector1.Z().VnH());
        orr(VTMP1.Z().VnH(), ComparePred.Merging(), VTMP1.Z().VnH(), Vector1.Z().VnH());
        break;
      }
      case 4: {
        fcmne(ComparePred.VnS(), Mask, Vector1.Z().VnS(), Vector2.Z().VnS());
        not_(VTMP1.Z().VnS(), ComparePred.Merging(), Vector1.Z().VnS());
        orr(VTMP1.Z().VnS(), ComparePred.Merging(), VTMP1.Z().VnS(), Vector1.Z().VnS());
        break;
      }
      case 8: {
        fcmne(ComparePred.VnD(), Mask, Vector1.Z().VnD(), Vector2.Z().VnD());
        not_(VTMP1.Z().VnD(), ComparePred.Merging(), Vector1.Z().VnD());
        orr(VTMP1.Z().VnD(), ComparePred.Merging(), VTMP1.Z().VnD(), Vector1.Z().VnD());
        break;
      }
      default:
        return;
    }

    mov(Dst.Z().VnD(), VTMP1.Z().VnD());
  } else {
    if (IsScalar) {
      switch (ElementSize) {
        case 2: {
          fcmeq(Dst.H(), Vector1.H(), Vector2.H());
          break;
        }
        case 4: {
          fcmeq(Dst.S(), Vector1.S(), Vector2.S());
          break;
        }
        case 8: {
          fcmeq(Dst.D(), Vector1.D(), Vector2.D());
          break;
        }
        default:
          LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
          break;
      }
      mvn(Dst.V8B(), Dst.V8B());
    } else {
      switch (ElementSize) {
        case 2: {
          fcmeq(Dst.V8H(), Vector1.V8H(), Vector2.V8H());
          break;
        }
        case 4: {
          fcmeq(Dst.V4S(), Vector1.V4S(), Vector2.V4S());
          break;
        }
        case 8: {
          fcmeq(Dst.V2D(), Vector1.V2D(), Vector2.V2D());
          break;
        }
        default:
          LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
          break;
      }
      mvn(Dst.V16B(), Dst.V16B());
    }
  }
}

DEF_OP(VFCMPLT) {
  const auto Op = IROp->C<IR::IROp_VFCMPLT>();
  const auto  OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto IsScalar = ElementSize == OpSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto Vector1 = GetSrc(Op->Vector1.ID());
  const auto Vector2 = GetSrc(Op->Vector2.ID());

  if (HostSupportsSVE && Is256Bit && !IsScalar) {
    const auto Mask = PRED_TMP_32B.Zeroing();
    const auto ComparePred = p0;

    // Ensure we have no junk in the temporary.
    eor(VTMP1.Z().VnD(), VTMP1.Z().VnD(), VTMP1.Z().VnD());

    switch (ElementSize) {
      case 2: {
        fcmgt(ComparePred.VnH(), Mask, Vector2.Z().VnH(), Vector1.Z().VnH());
        not_(VTMP1.Z().VnH(), ComparePred.Merging(), Vector2.Z().VnH());
        orr(VTMP1.Z().VnH(), ComparePred.Merging(), VTMP1.Z().VnH(), Vector2.Z().VnH());
        break;
      }
      case 4: {
        fcmgt(ComparePred.VnS(), Mask, Vector2.Z().VnS(), Vector1.Z().VnS());
        not_(VTMP1.Z().VnS(), ComparePred.Merging(), Vector2.Z().VnS());
        orr(VTMP1.Z().VnS(), ComparePred.Merging(), VTMP1.Z().VnS(), Vector2.Z().VnS());
        break;
      }
      case 8: {
        fcmgt(ComparePred.VnD(), Mask, Vector2.Z().VnD(), Vector1.Z().VnD());
        not_(VTMP1.Z().VnD(), ComparePred.Merging(), Vector2.Z().VnD());
        orr(VTMP1.Z().VnD(), ComparePred.Merging(), VTMP1.Z().VnD(), Vector2.Z().VnD());
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        return;
    }

    mov(Dst.Z().VnD(), VTMP1.Z().VnD());
  } else {
    if (IsScalar) {
      switch (ElementSize) {
        case 2: {
          fcmgt(Dst.H(), Vector2.H(), Vector1.H());
          break;
        }
        case 4: {
          fcmgt(Dst.S(), Vector2.S(), Vector1.S());
          break;
        }
        case 8: {
          fcmgt(Dst.D(), Vector2.D(), Vector1.D());
          break;
        }
        default:
          LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
          break;
      }
    } else {
      switch (ElementSize) {
        case 2: {
          fcmgt(Dst.V8H(), Vector2.V8H(), Vector1.V8H());
          break;
        }
        case 4: {
          fcmgt(Dst.V4S(), Vector2.V4S(), Vector1.V4S());
          break;
        }
        case 8: {
          fcmgt(Dst.V2D(), Vector2.V2D(), Vector1.V2D());
          break;
        }
        default:
          LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
          break;
      }
    }
  }
}

DEF_OP(VFCMPGT) {
  const auto Op = IROp->C<IR::IROp_VFCMPGT>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto IsScalar = ElementSize == OpSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto Vector1 = GetSrc(Op->Vector1.ID());
  const auto Vector2 = GetSrc(Op->Vector2.ID());

  if (HostSupportsSVE && Is256Bit && !IsScalar) {
    const auto Mask = PRED_TMP_32B.Zeroing();
    const auto ComparePred = p0;

    // Ensure there's no junk in the temporary.
    eor(VTMP1.Z().VnD(), VTMP1.Z().VnD(), VTMP1.Z().VnD());

    switch (ElementSize) {
      case 2: {
        fcmgt(ComparePred.VnH(), Mask, Vector1.Z().VnH(), Vector2.Z().VnH());
        not_(VTMP1.Z().VnH(), ComparePred.Merging(), Vector1.Z().VnH());
        orr(VTMP1.Z().VnH(), ComparePred.Merging(), VTMP1.Z().VnH(), Vector1.Z().VnH());
        break;
      }
      case 4: {
        fcmgt(ComparePred.VnS(), Mask, Vector1.Z().VnS(), Vector2.Z().VnS());
        not_(VTMP1.Z().VnS(), ComparePred.Merging(), Vector1.Z().VnS());
        orr(VTMP1.Z().VnS(), ComparePred.Merging(), VTMP1.Z().VnS(), Vector1.Z().VnS());
        break;
      }
      case 8: {
        fcmgt(ComparePred.VnD(), Mask, Vector1.Z().VnD(), Vector2.Z().VnD());
        not_(VTMP1.Z().VnD(), ComparePred.Merging(), Vector1.Z().VnD());
        orr(VTMP1.Z().VnD(), ComparePred.Merging(), VTMP1.Z().VnD(), Vector1.Z().VnD());
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        return;
    }

    mov(Dst.Z().VnD(), VTMP1.Z().VnD());
  } else {
    if (IsScalar) {
      switch (ElementSize) {
        case 2: {
          fcmgt(Dst.H(), Vector1.H(), Vector2.H());
          break;
        }
        case 4: {
          fcmgt(Dst.S(), Vector1.S(), Vector2.S());
          break;
        }
        case 8: {
          fcmgt(Dst.D(), Vector1.D(), Vector2.D());
          break;
        }
        default:
          LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
          break;
      }
    } else {
      switch (ElementSize) {
        case 2: {
          fcmgt(Dst.V8H(), Vector1.V8H(), Vector2.V8H());
          break;
        }
        case 4: {
          fcmgt(Dst.V4S(), Vector1.V4S(), Vector2.V4S());
          break;
        }
        case 8: {
          fcmgt(Dst.V2D(), Vector1.V2D(), Vector2.V2D());
          break;
        }
        default:
          LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
          break;
      }
    }
  }
}

DEF_OP(VFCMPLE) {
  const auto Op = IROp->C<IR::IROp_VFCMPLE>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto IsScalar = ElementSize == OpSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto Vector1 = GetSrc(Op->Vector1.ID());
  const auto Vector2 = GetSrc(Op->Vector2.ID());

  if (HostSupportsSVE && Is256Bit && !IsScalar) {
    const auto Mask = PRED_TMP_32B.Zeroing();
    const auto ComparePred = p0;

    // Ensure there's no junk in the temporary.
    eor(VTMP1.Z().VnD(), VTMP1.Z().VnD(), VTMP1.Z().VnD());

    switch (ElementSize) {
      case 2: {
        fcmge(ComparePred.VnH(), Mask, Vector2.Z().VnH(), Vector1.Z().VnH());
        not_(VTMP1.Z().VnH(), ComparePred.Merging(), Vector2.Z().VnH());
        orr(VTMP1.Z().VnH(), ComparePred.Merging(), VTMP1.Z().VnH(), Vector2.Z().VnH());
        break;
      }
      case 4: {
        fcmge(ComparePred.VnS(), Mask, Vector2.Z().VnS(), Vector1.Z().VnS());
        not_(VTMP1.Z().VnS(), ComparePred.Merging(), Vector2.Z().VnS());
        orr(VTMP1.Z().VnS(), ComparePred.Merging(), VTMP1.Z().VnS(), Vector2.Z().VnS());
        break;
      }
      case 8: {
        fcmge(ComparePred.VnD(), Mask, Vector2.Z().VnD(), Vector1.Z().VnD());
        not_(VTMP1.Z().VnD(), ComparePred.Merging(), Vector2.Z().VnD());
        orr(VTMP1.Z().VnD(), ComparePred.Merging(), VTMP1.Z().VnD(), Vector2.Z().VnD());
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        return;
    }

    mov(Dst.Z().VnD(), VTMP1.Z().VnD());
  } else {
    if (IsScalar) {
      switch (ElementSize) {
        case 2: {
          fcmge(Dst.H(), Vector2.H(), Vector1.H());
          break;
        }
        case 4: {
          fcmge(Dst.S(), Vector2.S(), Vector1.S());
          break;
        }
        case 8: {
          fcmge(Dst.D(), Vector2.D(), Vector1.D());
          break;
        }
        default:
          LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
          break;
      }
    } else {
      switch (ElementSize) {
        case 2: {
          fcmge(Dst.V8H(), Vector2.V8H(), Vector1.V8H());
          break;
        }
        case 4: {
          fcmge(Dst.V4S(), Vector2.V4S(), Vector1.V4S());
          break;
        }
        case 8: {
          fcmge(Dst.V2D(), Vector2.V2D(), Vector1.V2D());
          break;
        }
        default:
          LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
          break;
      }
    }
  }
}

DEF_OP(VFCMPORD) {
  const auto Op = IROp->C<IR::IROp_VFCMPORD>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto IsScalar = ElementSize == OpSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto Vector1 = GetSrc(Op->Vector1.ID());
  const auto Vector2 = GetSrc(Op->Vector2.ID());

  if (HostSupportsSVE && Is256Bit && !IsScalar) {
    const auto Mask = PRED_TMP_32B.Zeroing();
    const auto ComparePred = p0;

    // Ensure there's no junk in the temporary.
    eor(VTMP1.Z().VnD(), VTMP1.Z().VnD(), VTMP1.Z().VnD());

    // The idea is like comparing for unordered, but we just
    // invert the predicate from the comparison to instead
    // select all ordered elements in the vector.

    switch (ElementSize) {
      case 2: {
        fcmuo(ComparePred.VnH(), Mask, Vector1.Z().VnH(), Vector2.Z().VnH());
        not_(ComparePred.VnB(), Mask, ComparePred.VnB());
        not_(VTMP1.Z().VnH(), ComparePred.Merging(), Vector1.Z().VnH());
        orr(VTMP1.Z().VnH(), ComparePred.Merging(), VTMP1.Z().VnH(), Vector1.Z().VnH());
        break;
      }
      case 4: {
        fcmuo(ComparePred.VnS(), Mask, Vector1.Z().VnS(), Vector2.Z().VnS());
        not_(ComparePred.VnB(), Mask, ComparePred.VnB());
        not_(VTMP1.Z().VnS(), ComparePred.Merging(), Vector1.Z().VnS());
        orr(VTMP1.Z().VnS(), ComparePred.Merging(), VTMP1.Z().VnS(), Vector1.Z().VnS());
        break;
      }
      case 8: {
        fcmuo(ComparePred.VnD(), Mask, Vector1.Z().VnD(), Vector2.Z().VnD());
        not_(ComparePred.VnB(), Mask, ComparePred.VnB());
        not_(VTMP1.Z().VnD(), ComparePred.Merging(), Vector1.Z().VnD());
        orr(VTMP1.Z().VnD(), ComparePred.Merging(), VTMP1.Z().VnD(), Vector1.Z().VnD());
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        return;
    }

    mov(Dst.Z().VnD(), VTMP1.Z().VnD());
  } else {
    if (IsScalar) {
      switch (ElementSize) {
        case 2: {
          fcmge(VTMP1.H(), Vector1.H(), Vector2.H());
          fcmgt(VTMP2.H(), Vector2.H(), Vector1.H());
          orr(Dst.V8B(), VTMP1.V8B(), VTMP2.V8B());
          break;
        }
        case 4: {
          fcmge(VTMP1.S(), Vector1.S(), Vector2.S());
          fcmgt(VTMP2.S(), Vector2.S(), Vector1.S());
          orr(Dst.V8B(), VTMP1.V8B(), VTMP2.V8B());
          break;
        }
        case 8: {
          fcmge(VTMP1.D(), Vector1.D(), Vector2.D());
          fcmgt(VTMP2.D(), Vector2.D(), Vector1.D());
          orr(Dst.V8B(), VTMP1.V8B(), VTMP2.V8B());
          break;
        }
        default:
          LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
          break;
      }
    } else {
      switch (ElementSize) {
        case 2: {
          fcmge(VTMP1.V8H(), Vector1.V8H(), Vector2.V8H());
          fcmgt(VTMP2.V8H(), Vector2.V8H(), Vector1.V8H());
          orr(Dst.V16B(), VTMP1.V16B(), VTMP2.V16B());
          break;
        }
        case 4: {
          fcmge(VTMP1.V4S(), Vector1.V4S(), Vector2.V4S());
          fcmgt(VTMP2.V4S(), Vector2.V4S(), Vector1.V4S());
          orr(Dst.V16B(), VTMP1.V16B(), VTMP2.V16B());
          break;
        }
        case 8: {
          fcmge(VTMP1.V2D(), Vector1.V2D(), Vector2.V2D());
          fcmgt(VTMP2.V2D(), Vector2.V2D(), Vector1.V2D());
          orr(Dst.V16B(), VTMP1.V16B(), VTMP2.V16B());
          break;
        }
        default:
          LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
          break;
      }
    }
  }
}

DEF_OP(VFCMPUNO) {
  const auto Op = IROp->C<IR::IROp_VFCMPUNO>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto IsScalar = ElementSize == OpSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto Vector1 = GetSrc(Op->Vector1.ID());
  const auto Vector2 = GetSrc(Op->Vector2.ID());

  if (HostSupportsSVE && Is256Bit && !IsScalar) {
    const auto Mask = PRED_TMP_32B.Zeroing();
    const auto ComparePred = p0;

    // Ensure there's no junk in the temporary.
    eor(VTMP1.Z().VnD(), VTMP1.Z().VnD(), VTMP1.Z().VnD());

    switch (ElementSize) {
      case 2: {
        fcmuo(ComparePred.VnH(), Mask, Vector1.Z().VnH(), Vector2.Z().VnH());
        not_(VTMP1.Z().VnH(), ComparePred.Merging(), Vector1.Z().VnH());
        orr(VTMP1.Z().VnH(), ComparePred.Merging(), VTMP1.Z().VnH(), Vector1.Z().VnH());
        break;
      }
      case 4: {
        fcmuo(ComparePred.VnS(), Mask, Vector1.Z().VnS(), Vector2.Z().VnS());
        not_(VTMP1.Z().VnS(), ComparePred.Merging(), Vector1.Z().VnS());
        orr(VTMP1.Z().VnS(), ComparePred.Merging(), VTMP1.Z().VnS(), Vector1.Z().VnS());
        break;
      }
      case 8: {
        fcmuo(ComparePred.VnD(), Mask, Vector1.Z().VnD(), Vector2.Z().VnD());
        not_(VTMP1.Z().VnD(), ComparePred.Merging(), Vector1.Z().VnD());
        orr(VTMP1.Z().VnD(), ComparePred.Merging(), VTMP1.Z().VnD(), Vector1.Z().VnD());
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        return;
    }

    mov(Dst.Z().VnD(), VTMP1.Z().VnD());
  } else {
    if (IsScalar) {
      switch (ElementSize) {
        case 2: {
          fcmge(VTMP1.H(), Vector1.H(), Vector2.H());
          fcmgt(VTMP2.H(), Vector2.H(), Vector1.H());
          orr(Dst.V8B(), VTMP1.V8B(), VTMP2.V8B());
          mvn(Dst.V8B(), Dst.V8B());
          break;
        }
        case 4: {
          fcmge(VTMP1.S(), Vector1.S(), Vector2.S());
          fcmgt(VTMP2.S(), Vector2.S(), Vector1.S());
          orr(Dst.V8B(), VTMP1.V8B(), VTMP2.V8B());
          mvn(Dst.V8B(), Dst.V8B());
          break;
        }
        case 8: {
          fcmge(VTMP1.D(), Vector1.D(), Vector2.D());
          fcmgt(VTMP2.D(), Vector2.D(), Vector1.D());
          orr(Dst.V8B(), VTMP1.V8B(), VTMP2.V8B());
          mvn(Dst.V8B(), Dst.V8B());
          break;
        }
        default:
          LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
          break;
      }
    } else {
      switch (ElementSize) {
        case 2: {
          fcmge(VTMP1.V8H(), Vector1.V8H(), Vector2.V8H());
          fcmgt(VTMP2.V8H(), Vector2.V8H(), Vector1.V8H());
          orr(Dst.V16B(), VTMP1.V16B(), VTMP2.V16B());
          mvn(Dst.V16B(), Dst.V16B());
          break;
        }
        case 4: {
          fcmge(VTMP1.V4S(), Vector1.V4S(), Vector2.V4S());
          fcmgt(VTMP2.V4S(), Vector2.V4S(), Vector1.V4S());
          orr(Dst.V16B(), VTMP1.V16B(), VTMP2.V16B());
          mvn(Dst.V16B(), Dst.V16B());
          break;
        }
        case 8: {
          fcmge(VTMP1.V2D(), Vector1.V2D(), Vector2.V2D());
          fcmgt(VTMP2.V2D(), Vector2.V2D(), Vector1.V2D());
          orr(Dst.V16B(), VTMP1.V16B(), VTMP2.V16B());
          mvn(Dst.V16B(), Dst.V16B());
          break;
        }
        default:
          LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
          break;
      }
    }
  }
}

DEF_OP(VUShl) {
  LOGMAN_MSG_A_FMT("Unimplemented");
}

DEF_OP(VUShr) {
  LOGMAN_MSG_A_FMT("Unimplemented");
}

DEF_OP(VSShr) {
  LOGMAN_MSG_A_FMT("Unimplemented");
}

DEF_OP(VUShlS) {
  const auto Op = IROp->C<IR::IROp_VUShlS>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto ShiftScalar = GetSrc(Op->ShiftScalar.ID());
  const auto Vector = GetSrc(Op->Vector.ID());

  if (HostSupportsSVE && Is256Bit) {
    const auto Mask = PRED_TMP_32B.Merging();

    // NOTE: SVE LSL is a destructive operation.

    switch (ElementSize) {
      case 1: {
        dup(VTMP1.Z().VnB(), ShiftScalar.Z().VnB(), 0);
        mov(Dst.Z().VnD(), Vector.Z().VnD());
        lsl(Dst.Z().VnB(), Mask, Dst.Z().VnB(), VTMP1.Z().VnB());
        break;
      }
      case 2: {
        dup(VTMP1.Z().VnH(), ShiftScalar.Z().VnH(), 0);
        mov(Dst.Z().VnD(), Vector.Z().VnD());
        lsl(Dst.Z().VnH(), Mask, Dst.Z().VnH(), VTMP1.Z().VnH());
        break;
      }
      case 4: {
        dup(VTMP1.Z().VnS(), ShiftScalar.Z().VnS(), 0);
        mov(Dst.Z().VnD(), Vector.Z().VnD());
        lsl(Dst.Z().VnS(), Mask, Dst.Z().VnS(), VTMP1.Z().VnS());
        break;
      }
      case 8: {
        dup(VTMP1.Z().VnD(), ShiftScalar.Z().VnD(), 0);
        mov(Dst.Z().VnD(), Vector.Z().VnD());
        lsl(Dst.Z().VnD(), Mask, Dst.Z().VnD(), VTMP1.Z().VnD());
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
    }
  } else {
    switch (ElementSize) {
      case 1: {
        dup(VTMP1.V16B(), ShiftScalar.V16B(), 0);
        ushl(Dst.V16B(), Vector.V16B(), VTMP1.V16B());
        break;
      }
      case 2: {
        dup(VTMP1.V8H(), ShiftScalar.V8H(), 0);
        ushl(Dst.V8H(), Vector.V8H(), VTMP1.V8H());
        break;
      }
      case 4: {
        dup(VTMP1.V4S(), ShiftScalar.V4S(), 0);
        ushl(Dst.V4S(), Vector.V4S(), VTMP1.V4S());
        break;
      }
      case 8: {
        dup(VTMP1.V2D(), ShiftScalar.V2D(), 0);
        ushl(Dst.V2D(), Vector.V2D(), VTMP1.V2D());
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
    }
  }
}

DEF_OP(VUShrS) {
  const auto Op = IROp->C<IR::IROp_VUShrS>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto ShiftScalar = GetSrc(Op->ShiftScalar.ID());
  const auto Vector = GetSrc(Op->Vector.ID());

  if (HostSupportsSVE && Is256Bit) {
    const auto Mask = PRED_TMP_32B.Merging();

    // NOTE: SVE LSR is a destructive operation.

    switch (ElementSize) {
      case 1: {
        dup(VTMP1.Z().VnB(), ShiftScalar.Z().VnB(), 0);
        mov(Dst.Z().VnD(), Vector.Z().VnD());
        lsr(Dst.Z().VnB(), Mask, Dst.Z().VnB(), VTMP1.Z().VnB());
        break;
      }
      case 2: {
        dup(VTMP1.Z().VnH(), ShiftScalar.Z().VnH(), 0);
        mov(Dst.Z().VnD(), Vector.Z().VnD());
        lsr(Dst.Z().VnH(), Mask, Dst.Z().VnH(), VTMP1.Z().VnH());
        break;
      }
      case 4: {
        dup(VTMP1.Z().VnS(), ShiftScalar.Z().VnS(), 0);
        mov(Dst.Z().VnD(), Vector.Z().VnD());
        lsr(Dst.Z().VnS(), Mask, Dst.Z().VnS(), VTMP1.Z().VnS());
        break;
      }
      case 8: {
        dup(VTMP1.Z().VnD(), ShiftScalar.Z().VnD(), 0);
        mov(Dst.Z().VnD(), Vector.Z().VnD());
        lsr(Dst.Z().VnD(), Mask, Dst.Z().VnD(), VTMP1.Z().VnD());
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
    }
  } else {
    switch (ElementSize) {
      case 1: {
        dup(VTMP1.V16B(), ShiftScalar.V16B(), 0);
        neg(VTMP1.V16B(), VTMP1.V16B());
        ushl(Dst.V16B(), Vector.V16B(), VTMP1.V16B());
        break;
      }
      case 2: {
        dup(VTMP1.V8H(), ShiftScalar.V8H(), 0);
        neg(VTMP1.V8H(), VTMP1.V8H());
        ushl(Dst.V8H(), Vector.V8H(), VTMP1.V8H());
        break;
      }
      case 4: {
        dup(VTMP1.V4S(), ShiftScalar.V4S(), 0);
        neg(VTMP1.V4S(), VTMP1.V4S());
        ushl(Dst.V4S(), Vector.V4S(), VTMP1.V4S());
        break;
      }
      case 8: {
        dup(VTMP1.V2D(), ShiftScalar.V2D(), 0);
        neg(VTMP1.V2D(), VTMP1.V2D());
        ushl(Dst.V2D(), Vector.V2D(), VTMP1.V2D());
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
    }
  }
}

DEF_OP(VSShrS) {
  const auto Op = IROp->C<IR::IROp_VSShrS>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto ShiftScalar = GetSrc(Op->ShiftScalar.ID());
  const auto Vector = GetSrc(Op->Vector.ID());

  if (HostSupportsSVE && Is256Bit) {
    const auto Mask = PRED_TMP_32B.Merging();

    // NOTE: SVE ASR is a destructive operation.

    switch (ElementSize) {
      case 1: {
        dup(VTMP1.Z().VnB(), ShiftScalar.Z().VnB(), 0);
        mov(Dst.Z().VnD(), Vector.Z().VnD());
        asr(Dst.Z().VnB(), Mask, Dst.Z().VnB(), VTMP1.Z().VnB());
        break;
      }
      case 2: {
        dup(VTMP1.Z().VnH(), ShiftScalar.Z().VnH(), 0);
        mov(Dst.Z().VnD(), Vector.Z().VnD());
        asr(Dst.Z().VnH(), Mask, Dst.Z().VnH(), VTMP1.Z().VnH());
        break;
      }
      case 4: {
        dup(VTMP1.Z().VnS(), ShiftScalar.Z().VnS(), 0);
        mov(Dst.Z().VnD(), Vector.Z().VnD());
        asr(Dst.Z().VnS(), Mask, Dst.Z().VnS(), VTMP1.Z().VnS());
        break;
      }
      case 8: {
        dup(VTMP1.Z().VnD(), ShiftScalar.Z().VnD(), 0);
        mov(Dst.Z().VnD(), Vector.Z().VnD());
        asr(Dst.Z().VnD(), Mask, Dst.Z().VnD(), VTMP1.Z().VnD());
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
    }
  } else {
    switch (ElementSize) {
      case 1: {
        dup(VTMP1.V16B(), ShiftScalar.V16B(), 0);
        neg(VTMP1.V16B(), VTMP1.V16B());
        sshl(Dst.V16B(), Vector.V16B(), VTMP1.V16B());
        break;
      }
      case 2: {
        dup(VTMP1.V8H(), ShiftScalar.V8H(), 0);
        neg(VTMP1.V8H(), VTMP1.V8H());
        sshl(Dst.V8H(), Vector.V8H(), VTMP1.V8H());
        break;
      }
      case 4: {
        dup(VTMP1.V4S(), ShiftScalar.V4S(), 0);
        neg(VTMP1.V4S(), VTMP1.V4S());
        sshl(Dst.V4S(), Vector.V4S(), VTMP1.V4S());
        break;
      }
      case 8: {
        dup(VTMP1.V2D(), ShiftScalar.V2D(), 0);
        neg(VTMP1.V2D(), VTMP1.V2D());
        sshl(Dst.V2D(), Vector.V2D(), VTMP1.V2D());
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
    }
  }
}

DEF_OP(VInsElement) {
  auto Op = IROp->C<IR::IROp_VInsElement>();

  auto reg = GetSrc(Op->DestVector.ID());

  if (GetDst(Node).GetCode() != reg.GetCode()) {
    mov(VTMP1, reg);
    reg = VTMP1;
  }

  switch (Op->Header.ElementSize) {
    case 1: {
      mov(reg.V16B(), Op->DestIdx, GetSrc(Op->SrcVector.ID()).V16B(), Op->SrcIdx);
    break;
    }
    case 2: {
      mov(reg.V8H(), Op->DestIdx, GetSrc(Op->SrcVector.ID()).V8H(), Op->SrcIdx);
    break;
    }
    case 4: {
      mov(reg.V4S(), Op->DestIdx, GetSrc(Op->SrcVector.ID()).V4S(), Op->SrcIdx);
    break;
    }
    case 8: {
      mov(reg.V2D(), Op->DestIdx, GetSrc(Op->SrcVector.ID()).V2D(), Op->SrcIdx);
    break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }

  if (GetDst(Node).GetCode() != reg.GetCode()) {
    mov(GetDst(Node), reg);
  }
}

DEF_OP(VDupElement) {
  auto Op = IROp->C<IR::IROp_VDupElement>();
  switch (Op->Header.ElementSize) {
    case 1:
      dup(GetDst(Node).V16B(), GetSrc(Op->Vector.ID()).V16B(), Op->Index);
    break;
    case 2:
      dup(GetDst(Node).V8H(), GetSrc(Op->Vector.ID()).V8H(), Op->Index);
    break;
    case 4:
      dup(GetDst(Node).V4S(), GetSrc(Op->Vector.ID()).V4S(), Op->Index);
    break;
    case 8:
      dup(GetDst(Node).V2D(), GetSrc(Op->Vector.ID()).V2D(), Op->Index);
    break;
    default:  LOGMAN_MSG_A_FMT("Unhandled VDupElement element size: {}", Op->Header.ElementSize);
  }
}

DEF_OP(VExtr) {
  auto Op = IROp->C<IR::IROp_VExtr>();
  const uint8_t OpSize = IROp->Size;

  // AArch64 ext op has bit arrangement as [Vm:Vn] so arguments need to be swapped
  auto UpperBits = GetSrc(Op->VectorLower.ID());
  auto LowerBits = GetSrc(Op->VectorUpper.ID());
  auto Index = Op->Index;

  if (Index >= OpSize) {
    // Upper bits have moved in to the lower bits
    LowerBits = UpperBits;

    // Upper bits are all now zero
    UpperBits = VTMP1;
    eor(VTMP1.V16B(), VTMP1.V16B(), VTMP1.V16B());
    Index -= OpSize;
  }

  if (OpSize == 8) {
    ext(GetDst(Node).V8B(), LowerBits.V8B(), UpperBits.V8B(), Index * Op->Header.ElementSize);
  }
  else {
    ext(GetDst(Node).V16B(), LowerBits.V16B(), UpperBits.V16B(), Index * Op->Header.ElementSize);
  }
}

DEF_OP(VSLI) {
  auto Op = IROp->C<IR::IROp_VSLI>();
  const uint8_t OpSize = IROp->Size;
  const uint8_t BitShift = Op->ByteShift * 8;
  if (BitShift < 64) {
    // Move to Pair [TMP2:TMP1]
    mov(TMP1, GetSrc(Op->Vector.ID()).V2D(), 0);
    mov(TMP2, GetSrc(Op->Vector.ID()).V2D(), 1);
    // Left shift low 64bits
    lsl(TMP3, TMP1, BitShift);

    // Extract high 64bits from [TMP2:TMP1]
    extr(TMP1, TMP2, TMP1, 64 - BitShift);

    mov(GetDst(Node).V2D(), 0, TMP3);
    mov(GetDst(Node).V2D(), 1, TMP1);
  }
  else {
    if (Op->ByteShift >= OpSize) {
      eor(GetDst(Node).V16B(), GetDst(Node).V16B(), GetDst(Node).V16B());
    }
    else {
      mov(TMP1, GetSrc(Op->Vector.ID()).V2D(), 0);
      lsl(TMP1, TMP1, BitShift - 64);
      mov(GetDst(Node).V2D(), 0, xzr);
      mov(GetDst(Node).V2D(), 1, TMP1);
    }
  }
}

DEF_OP(VSRI) {
  auto Op = IROp->C<IR::IROp_VSRI>();
  const uint8_t OpSize = IROp->Size;
  const uint8_t BitShift = Op->ByteShift * 8;
  if (BitShift < 64) {
    // Move to Pair [TMP2:TMP1]
    mov(TMP1, GetSrc(Op->Vector.ID()).V2D(), 0);
    mov(TMP2, GetSrc(Op->Vector.ID()).V2D(), 1);

    // Extract Low 64bits [TMP2:TMP2] >> BitShift
    extr(TMP1, TMP2, TMP1, BitShift);
    // Right shift high bits
    lsr(TMP2, TMP2, BitShift);

    mov(GetDst(Node).V2D(), 0, TMP1);
    mov(GetDst(Node).V2D(), 1, TMP2);
  }
  else {
    if (Op->ByteShift >= OpSize) {
      eor(GetDst(Node).V16B(), GetDst(Node).V16B(), GetDst(Node).V16B());
    }
    else {
      mov(TMP1, GetSrc(Op->Vector.ID()).V2D(), 1);
      lsr(TMP1, TMP1, BitShift - 64);
      mov(GetDst(Node).V2D(), 0, TMP1);
      mov(GetDst(Node).V2D(), 1, xzr);
    }
  }
}

DEF_OP(VUShrI) {
  const auto Op = IROp->C<IR::IROp_VUShrI>();
  const auto OpSize = IROp->Size;

  const auto BitShift = Op->BitShift;
  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto Vector = GetSrc(Op->Vector.ID());

  if (BitShift >= (ElementSize * 8)) {
    eor(Dst.V16B(), Dst.V16B(), Dst.V16B());
  } else {
    if (HostSupportsSVE && Is256Bit) {
      const auto Mask = PRED_TMP_32B.Merging();

      // SVE LSR is destructive, so lets set up the destination.
      mov(Dst.Z().VnD(), Vector.Z().VnD());

      switch (ElementSize) {
        case 1: {
          lsr(Dst.Z().VnB(), Mask, Dst.Z().VnB(), BitShift);
          break;
        }
        case 2: {
          lsr(Dst.Z().VnH(), Mask, Dst.Z().VnH(), BitShift);
          break;
        }
        case 4: {
          lsr(Dst.Z().VnS(), Mask, Dst.Z().VnS(), BitShift);
          break;
        }
        case 8: {
          lsr(Dst.Z().VnD(), Mask, Dst.Z().VnD(), BitShift);
          break;
        }
        default:
          LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
          break;
      }
    } else {
      switch (ElementSize) {
        case 1: {
          ushr(Dst.V16B(), Vector.V16B(), BitShift);
          break;
        }
        case 2: {
          ushr(Dst.V8H(), Vector.V8H(), BitShift);
          break;
        }
        case 4: {
          ushr(Dst.V4S(), Vector.V4S(), BitShift);
          break;
        }
        case 8: {
          ushr(Dst.V2D(), Vector.V2D(), BitShift);
          break;
        }
        default:
          LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
          break;
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

  const auto Dst = GetDst(Node);
  const auto Vector = GetSrc(Op->Vector.ID());

  if (HostSupportsSVE && Is256Bit) {
    const auto Mask = PRED_TMP_32B.Merging();

    // SVE ASR is destructive, so lets set up the destination.
    mov(Dst.Z().VnD(), Vector.Z().VnD());

    switch (ElementSize) {
      case 1: {
        asr(Dst.Z().VnB(), Mask, Dst.Z().VnB(), Shift);
        break;
      }
      case 2: {
        asr(Dst.Z().VnH(), Mask, Dst.Z().VnH(), Shift);
        break;
      }
      case 4: {
        asr(Dst.Z().VnS(), Mask, Dst.Z().VnS(), Shift);
        break;
      }
      case 8: {
        asr(Dst.Z().VnD(), Mask, Dst.Z().VnD(), Shift);
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
    }
  } else {
    switch (ElementSize) {
      case 1: {
        sshr(Dst.V16B(), Vector.V16B(), Shift);
        break;
      }
      case 2: {
        sshr(Dst.V8H(), Vector.V8H(), Shift);
        break;
      }
      case 4: {
        sshr(Dst.V4S(), Vector.V4S(), Shift);
        break;
      }
      case 8: {
        sshr(Dst.V2D(), Vector.V2D(), Shift);
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
    }
  }
}

DEF_OP(VShlI) {
  const auto Op = IROp->C<IR::IROp_VShlI>();
  const auto OpSize = IROp->Size;

  const auto BitShift = Op->BitShift;
  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto Vector = GetSrc(Op->Vector.ID());

  if (BitShift >= (ElementSize * 8)) {
    eor(Dst.V16B(), Dst.V16B(), Dst.V16B());
  } else {
    if (HostSupportsSVE && Is256Bit) {
      const auto Mask = PRED_TMP_32B.Merging();

      // SVE LSL is destructive, so lets set up the destination.
      mov(Dst.Z().VnD(), Vector.Z().VnD());

      switch (ElementSize) {
        case 1: {
          lsl(Dst.Z().VnB(), Mask, Dst.Z().VnB(), BitShift);
          break;
        }
        case 2: {
          lsl(Dst.Z().VnH(), Mask, Dst.Z().VnH(), BitShift);
          break;
        }
        case 4: {
          lsl(Dst.Z().VnS(), Mask, Dst.Z().VnS(), BitShift);
          break;
        }
        case 8: {
          lsl(Dst.Z().VnD(), Mask, Dst.Z().VnD(), BitShift);
          break;
        }
        default:
          LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
          break;
      }
    } else {
      switch (ElementSize) {
        case 1: {
          shl(Dst.V16B(), Vector.V16B(), BitShift);
          break;
        }
        case 2: {
          shl(Dst.V8H(), Vector.V8H(), BitShift);
          break;
        }
        case 4: {
          shl(Dst.V4S(), Vector.V4S(), BitShift);
          break;
        }
        case 8: {
          shl(Dst.V2D(), Vector.V2D(), BitShift);
          break;
        }
        default:
          LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
          break;
      }
    }
  }
}

DEF_OP(VUShrNI) {
  auto Op = IROp->C<IR::IROp_VUShrNI>();

  switch (Op->Header.ElementSize) {
    case 1: {
      shrn(GetDst(Node).V8B(), GetSrc(Op->Vector.ID()).V8H(), Op->BitShift);
    break;
    }
    case 2: {
      shrn(GetDst(Node).V4H(), GetSrc(Op->Vector.ID()).V4S(), Op->BitShift);
    break;
    }
    case 4: {
      shrn(GetDst(Node).V2S(), GetSrc(Op->Vector.ID()).V2D(), Op->BitShift);
    break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
}

DEF_OP(VUShrNI2) {
  auto Op = IROp->C<IR::IROp_VUShrNI2>();
  mov(VTMP1, GetSrc(Op->VectorLower.ID()));
  switch (Op->Header.ElementSize) {
    case 1: {
      shrn2(VTMP1.V16B(), GetSrc(Op->VectorUpper.ID()).V8H(), Op->BitShift);
    break;
    }
    case 2: {
      shrn2(VTMP1.V8H(), GetSrc(Op->VectorUpper.ID()).V4S(), Op->BitShift);
    break;
    }
    case 4: {
      shrn2(VTMP1.V4S(), GetSrc(Op->VectorUpper.ID()).V2D(), Op->BitShift);
    break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }

  mov(GetDst(Node), VTMP1);
}

DEF_OP(VSXTL) {
  const auto Op = IROp->C<IR::IROp_VSXTL>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto Vector = GetSrc(Op->Vector.ID());

  if (HostSupportsSVE && Is256Bit) {
    // A little gross, but SVE SXTB/SXTH/SXTW would be a little
    // more cumbersome to use here, since those instructions
    // use the supplied element size to determine indexing across
    // the vector.
    //
    // So for example if we were sign-extending a byte to a halfword
    // with SXTB, assume the vector is like so:
    // 
    // ╔═════════╗╔═════════╗╔═════════╗╔═════════╗
    // ║ Value 3 ║║ Value 2 ║║ Value 1 ║║ Value 0 ║ 
    // ╚═════════╝╚═════════╝╚═════════╝╚═════════╝
    //
    // (Each element is 8 bits in size, and for brevity assume a vector
    //  that's only 32 bits wide).
    //
    // The operation
    //
    // SXTB Dst.VnH, Src.VnB
    //
    // Will sign-extend bytes based off the element size and also index
    // the source vector on a by-element-size basis.
    //
    // The problem is, since we've specified halfwords as the element size
    // (via Dst.VnH), the instruction will skip over Value 1 and sign-extend
    // Value 2, place it into the Dst vector, and so on. So we'd be ignoring
    // values and end up with something like:
    //
    // ╔════════════════════╗╔════════════════════╗
    // ║      Value 2       ║║      Value 0       ║
    // ╚════════════════════╝╚════════════════════╝
    //
    // Uh oh!
    //
    // What we want is:
    //
    // ╔════════════════════╗╔════════════════════╗
    // ║      Value 1       ║║      Value 0       ║
    // ╚════════════════════╝╚════════════════════╝
    //
    // We want the extending operation to handle each individual value from
    // the source vector and not overlap or ignore them.

    switch (ElementSize) {
      case 2:
        sshllb(VTMP1.Z().VnH(), Vector.Z().VnB(), 0);
        sshllt(VTMP2.Z().VnH(), Vector.Z().VnB(), 0);
        zip1(Dst.Z().VnH(), VTMP1.Z().VnH(), VTMP2.Z().VnH());
        break;
      case 4:
        sshllb(VTMP1.Z().VnS(), Vector.Z().VnH(), 0);
        sshllt(VTMP2.Z().VnS(), Vector.Z().VnH(), 0);
        zip1(Dst.Z().VnS(), VTMP1.Z().VnS(), VTMP2.Z().VnS());
        break;
      case 8:
        sshllb(VTMP1.Z().VnD(), Vector.Z().VnS(), 0);
        sshllt(VTMP2.Z().VnD(), Vector.Z().VnS(), 0);
        zip1(Dst.Z().VnD(), VTMP1.Z().VnD(), VTMP2.Z().VnD());
        break;
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
    }
  } else {
    switch (ElementSize) {
      case 2:
        sxtl(Dst.V8H(), Vector.V8B());
        break;
      case 4:
        sxtl(Dst.V4S(), Vector.V4H());
        break;
      case 8:
        sxtl(Dst.V2D(), Vector.V2S());
        break;
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
    }
  }
}

DEF_OP(VSXTL2) {
  const auto Op = IROp->C<IR::IROp_VSXTL2>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto Vector = GetSrc(Op->Vector.ID());

  if (HostSupportsSVE && Is256Bit) {
    // See VSXTL implementation for in depth explanation
    // of all the instructions below.

    switch (ElementSize) {
      case 2:
        sshllb(VTMP1.Z().VnH(), Vector.Z().VnB(), 0);
        sshllt(VTMP2.Z().VnH(), Vector.Z().VnB(), 0);
        zip2(Dst.Z().VnH(), VTMP1.Z().VnH(), VTMP2.Z().VnH());
        break;
      case 4:
        sshllb(VTMP1.Z().VnS(), Vector.Z().VnH(), 0);
        sshllt(VTMP2.Z().VnS(), Vector.Z().VnH(), 0);
        zip2(Dst.Z().VnS(), VTMP1.Z().VnS(), VTMP2.Z().VnS());
        break;
      case 8:
        sshllb(VTMP1.Z().VnD(), Vector.Z().VnS(), 0);
        sshllt(VTMP2.Z().VnD(), Vector.Z().VnS(), 0);
        zip2(Dst.Z().VnD(), VTMP1.Z().VnD(), VTMP2.Z().VnD());
        break;
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
    }
  } else {
    switch (ElementSize) {
      case 2:
        sxtl2(Dst.V8H(), Vector.V16B());
        break;
      case 4:
        sxtl2(Dst.V4S(), Vector.V8H());
        break;
      case 8:
        sxtl2(Dst.V2D(), Vector.V4S());
        break;
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
    }
  }
}

DEF_OP(VUXTL) {
  const auto Op = IROp->C<IR::IROp_VUXTL>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto Vector = GetSrc(Op->Vector.ID());

  if (HostSupportsSVE && Is256Bit) {
    // NOTE: See VSXTL implementation for an explanation on why
    //       UXTB/UXTH/UXTW aren't used, since the same behavior
    //       concerns applies here, but with zero-extension
    //       instead of sign-extension.

    switch (ElementSize) {
      case 2:
        ushllb(VTMP1.Z().VnH(), Vector.Z().VnB(), 0);
        ushllt(VTMP2.Z().VnH(), Vector.Z().VnB(), 0);
        zip1(Dst.Z().VnH(), VTMP1.Z().VnH(), VTMP2.Z().VnH());
        break;
      case 4:
        ushllb(VTMP1.Z().VnS(), Vector.Z().VnH(), 0);
        ushllt(VTMP2.Z().VnS(), Vector.Z().VnH(), 0);
        zip1(Dst.Z().VnS(), VTMP1.Z().VnS(), VTMP2.Z().VnS());
        break;
      case 8:
        ushllb(VTMP1.Z().VnD(), Vector.Z().VnS(), 0);
        ushllt(VTMP2.Z().VnD(), Vector.Z().VnS(), 0);
        zip1(Dst.Z().VnD(), VTMP1.Z().VnD(), VTMP2.Z().VnD());
        break;
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
    }
  } else {
    switch (ElementSize) {
      case 2:
        uxtl(Dst.V8H(), Vector.V8B());
        break;
      case 4:
        uxtl(Dst.V4S(), Vector.V4H());
        break;
      case 8:
        uxtl(Dst.V2D(), Vector.V2S());
        break;
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
    }
  }
}

DEF_OP(VUXTL2) {
  const auto Op = IROp->C<IR::IROp_VUXTL2>();

  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto Vector = GetSrc(Op->Vector.ID());

  if (HostSupportsSVE && Is256Bit) {
    // NOTE: See VSXTL implementation for an explanation on why
    //       UXTB/UXTH/UXTW aren't used, since the same behavior
    //       concerns applies here, but with zero-extension
    //       instead of sign-extension.

    switch (ElementSize) {
      case 2:
        ushllb(VTMP1.Z().VnH(), Vector.Z().VnB(), 0);
        ushllt(VTMP2.Z().VnH(), Vector.Z().VnB(), 0);
        zip2(Dst.Z().VnH(), VTMP1.Z().VnH(), VTMP2.Z().VnH());
        break;
      case 4:
        ushllb(VTMP1.Z().VnS(), Vector.Z().VnH(), 0);
        ushllt(VTMP2.Z().VnS(), Vector.Z().VnH(), 0);
        zip2(Dst.Z().VnS(), VTMP1.Z().VnS(), VTMP2.Z().VnS());
        break;
      case 8:
        ushllb(VTMP1.Z().VnD(), Vector.Z().VnS(), 0);
        ushllt(VTMP2.Z().VnD(), Vector.Z().VnS(), 0);
        zip2(Dst.Z().VnD(), VTMP1.Z().VnD(), VTMP2.Z().VnD());
        break;
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
    }
  } else {
    switch (ElementSize) {
      case 2:
        uxtl2(Dst.V8H(), Vector.V16B());
        break;
      case 4:
        uxtl2(Dst.V4S(), Vector.V8H());
        break;
      case 8:
        uxtl2(Dst.V2D(), Vector.V4S());
        break;
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
    }
  }
}

DEF_OP(VSQXTN) {
  auto Op = IROp->C<IR::IROp_VSQXTN>();
  switch (Op->Header.ElementSize) {
    case 1:
      sqxtn(GetDst(Node).V8B(), GetSrc(Op->Vector.ID()).V8H());
    break;
    case 2:
      sqxtn(GetDst(Node).V4H(), GetSrc(Op->Vector.ID()).V4S());
    break;
    case 4:
      sqxtn(GetDst(Node).V2S(), GetSrc(Op->Vector.ID()).V2D());
    break;
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize);
  }
}

DEF_OP(VSQXTN2) {
  auto Op = IROp->C<IR::IROp_VSQXTN2>();
  uint8_t OpSize = IROp->Size;
  mov(VTMP1, GetSrc(Op->VectorLower.ID()));
  if (OpSize == 8) {
    switch (Op->Header.ElementSize) {
      case 1:
        sqxtn(VTMP2.V8B(), GetSrc(Op->VectorUpper.ID()).V8H());
        ins(VTMP1.V4S(), 1, VTMP2.V4S(), 0);
      break;
      case 2:
        sqxtn(VTMP2.V4H(), GetSrc(Op->VectorUpper.ID()).V4S());
        ins(VTMP1.V4S(), 1, VTMP2.V4S(), 0);
      break;
      case 4:
        sqxtn(VTMP2.V2S(), GetSrc(Op->VectorUpper.ID()).V2D());
        ins(VTMP1.V4S(), 1, VTMP2.V4S(), 0);
      break;
      default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize);
    }
  }
  else {
    switch (Op->Header.ElementSize) {
      case 1:
        sqxtn2(VTMP1.V16B(), GetSrc(Op->VectorUpper.ID()).V8H());
      break;
      case 2:
        sqxtn2(VTMP1.V8H(), GetSrc(Op->VectorUpper.ID()).V4S());
      break;
      case 4:
        sqxtn2(VTMP1.V4S(), GetSrc(Op->VectorUpper.ID()).V2D());
      break;
      default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize);
    }
  }
  mov(GetDst(Node), VTMP1);
}

DEF_OP(VSQXTUN) {
  auto Op = IROp->C<IR::IROp_VSQXTUN>();
  switch (Op->Header.ElementSize) {
    case 1:
      sqxtun(GetDst(Node).V8B(), GetSrc(Op->Vector.ID()).V8H());
    break;
    case 2:
      sqxtun(GetDst(Node).V4H(), GetSrc(Op->Vector.ID()).V4S());
    break;
    case 4:
      sqxtun(GetDst(Node).V2S(), GetSrc(Op->Vector.ID()).V2D());
    break;
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize);
  }
}

DEF_OP(VSQXTUN2) {
  auto Op = IROp->C<IR::IROp_VSQXTUN2>();
  uint8_t OpSize = IROp->Size;
  mov(VTMP1, GetSrc(Op->VectorLower.ID()));
  if (OpSize == 8) {
    switch (Op->Header.ElementSize) {
      case 1:
        sqxtun(VTMP2.V8B(), GetSrc(Op->VectorUpper.ID()).V8H());
        ins(VTMP1.V4S(), 1, VTMP2.V4S(), 0);
      break;
      case 2:
        sqxtun(VTMP2.V4H(), GetSrc(Op->VectorUpper.ID()).V4S());
        ins(VTMP1.V4S(), 1, VTMP2.V4S(), 0);
      break;
      case 4:
        sqxtun(VTMP2.V2S(), GetSrc(Op->VectorUpper.ID()).V2D());
        ins(VTMP1.V4S(), 1, VTMP2.V4S(), 0);
      break;
      default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize);
    }
  }
  else {
    switch (Op->Header.ElementSize) {
      case 1:
        sqxtun2(VTMP1.V16B(), GetSrc(Op->VectorUpper.ID()).V8H());
      break;
      case 2:
        sqxtun2(VTMP1.V8H(), GetSrc(Op->VectorUpper.ID()).V4S());
      break;
      case 4:
        sqxtun2(VTMP1.V4S(), GetSrc(Op->VectorUpper.ID()).V2D());
      break;
      default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize);
    }
  }
  mov(GetDst(Node), VTMP1);
}

DEF_OP(VMul) {
  const auto Op = IROp->C<IR::IROp_VUMul>();

  const auto ElementSize = Op->Header.ElementSize;

  const auto Dst = GetDst(Node);
  const auto Vector1 = GetSrc(Op->Vector1.ID());
  const auto Vector2 = GetSrc(Op->Vector2.ID());

  if (HostSupportsSVE) {
    switch (ElementSize) {
      case 1: {
        mul(Dst.Z().VnB(), Vector1.Z().VnB(), Vector2.Z().VnB());
        break;
      }
      case 2: {
        mul(Dst.Z().VnH(), Vector1.Z().VnH(), Vector2.Z().VnH());
        break;
      }
      case 4: {
        mul(Dst.Z().VnS(), Vector1.Z().VnS(), Vector2.Z().VnS());
        break;
      }
      case 8: {
        mul(Dst.Z().VnD(), Vector1.Z().VnD(), Vector2.Z().VnD());
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
    }
  } else {
    switch (ElementSize) {
      case 1: {
        mul(Dst.V16B(), Vector1.V16B(), Vector2.V16B());
        break;
      }
      case 2: {
        mul(Dst.V8H(), Vector1.V8H(), Vector2.V8H());
        break;
      }
      case 4: {
        mul(Dst.V4S(), Vector1.V4S(), Vector2.V4S());
        break;
      }
      case 8: {
        mul(Dst.V2D(), Vector1.V2D(), Vector2.V2D());
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
    }
  }
}

DEF_OP(VUMull) {
  const auto Op = IROp->C<IR::IROp_VUMull>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto Vector1 = GetSrc(Op->Vector1.ID());
  const auto Vector2 = GetSrc(Op->Vector2.ID());

  if (HostSupportsSVE && Is256Bit) {
    switch (ElementSize) {
      case 2: {
        umullb(VTMP1.Z().VnH(), Vector1.Z().VnB(), Vector2.Z().VnB());
        umullt(VTMP2.Z().VnH(), Vector1.Z().VnB(), Vector2.Z().VnB());
        zip1(Dst.Z().VnH(), VTMP1.Z().VnH(), VTMP2.Z().VnH());
        break;
      }
      case 4: {
        umullb(VTMP1.Z().VnS(), Vector1.Z().VnH(), Vector2.Z().VnH());
        umullt(VTMP2.Z().VnS(), Vector1.Z().VnH(), Vector2.Z().VnH());
        zip1(Dst.Z().VnS(), VTMP1.Z().VnS(), VTMP2.Z().VnS());
        break;
      }
      case 8: {
        umullb(VTMP1.Z().VnD(), Vector1.Z().VnS(), Vector2.Z().VnS());
        umullt(VTMP2.Z().VnD(), Vector1.Z().VnS(), Vector2.Z().VnS());
        zip1(Dst.Z().VnD(), VTMP1.Z().VnD(), VTMP2.Z().VnD());
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize >> 1);
        break;
    }
  } else {
    switch (ElementSize) {
      case 2: {
        umull(Dst.V8H(), Vector1.V8B(), Vector2.V8B());
        break;
      }
      case 4: {
        umull(Dst.V4S(), Vector1.V4H(), Vector2.V4H());
        break;
      }
      case 8: {
        umull(Dst.V2D(), Vector1.V2S(), Vector2.V2S());
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize >> 1);
        break;
    }
  }
}

DEF_OP(VSMull) {
  auto Op = IROp->C<IR::IROp_VSMull>();
  switch (Op->Header.ElementSize) {
    case 2: {
      smull(GetDst(Node).V8H(), GetSrc(Op->Vector1.ID()).V8B(), GetSrc(Op->Vector2.ID()).V8B());
    break;
    }
    case 4: {
      smull(GetDst(Node).V4S(), GetSrc(Op->Vector1.ID()).V4H(), GetSrc(Op->Vector2.ID()).V4H());
    break;
    }
    case 8: {
      smull(GetDst(Node).V2D(), GetSrc(Op->Vector1.ID()).V2S(), GetSrc(Op->Vector2.ID()).V2S());
    break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize >> 1); break;
  }
}

DEF_OP(VUMull2) {
  auto Op = IROp->C<IR::IROp_VUMull2>();
  switch (Op->Header.ElementSize) {
    case 2: {
      umull2(GetDst(Node).V8H(), GetSrc(Op->Vector1.ID()).V16B(), GetSrc(Op->Vector2.ID()).V16B());
    break;
    }
    case 4: {
      umull2(GetDst(Node).V4S(), GetSrc(Op->Vector1.ID()).V8H(), GetSrc(Op->Vector2.ID()).V8H());
    break;
    }
    case 8: {
      umull2(GetDst(Node).V2D(), GetSrc(Op->Vector1.ID()).V4S(), GetSrc(Op->Vector2.ID()).V4S());
    break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize >> 1); break;
  }
}

DEF_OP(VSMull2) {
  auto Op = IROp->C<IR::IROp_VSMull2>();
  switch (Op->Header.ElementSize) {
    case 2: {
      smull2(GetDst(Node).V8H(), GetSrc(Op->Vector1.ID()).V16B(), GetSrc(Op->Vector2.ID()).V16B());
    break;
    }
    case 4: {
      smull2(GetDst(Node).V4S(), GetSrc(Op->Vector1.ID()).V8H(), GetSrc(Op->Vector2.ID()).V8H());
    break;
    }
    case 8: {
      smull2(GetDst(Node).V2D(), GetSrc(Op->Vector1.ID()).V4S(), GetSrc(Op->Vector2.ID()).V4S());
    break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize >> 1); break;
  }
}

DEF_OP(VUABDL) {
  const auto Op = IROp->C<IR::IROp_VUABDL>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto Vector1 = GetSrc(Op->Vector1.ID());
  const auto Vector2 = GetSrc(Op->Vector2.ID());

  if (HostSupportsSVE && Is256Bit) {
    // To mimic the behavior of AdvSIMD UABDL, we need to get the
    // absolute difference of the even elements (UADBLB), get the
    // absolute difference of the odd elemenets (UABDLT), then
    // interleave the results in both vectors together.
    
    switch (ElementSize) {
      case 2: {
        uabdlb(VTMP1.Z().VnH(), Vector1.Z().VnB(), Vector2.Z().VnB());
        uabdlt(VTMP2.Z().VnH(), Vector1.Z().VnB(), Vector2.Z().VnB());
        zip1(Dst.Z().VnH(), VTMP1.Z().VnH(), VTMP2.Z().VnH());
        break;
      }
      case 4: {
        uabdlb(VTMP1.Z().VnS(), Vector1.Z().VnH(), Vector2.Z().VnH());
        uabdlt(VTMP2.Z().VnS(), Vector1.Z().VnH(), Vector2.Z().VnH());
        zip1(Dst.Z().VnS(), VTMP1.Z().VnS(), VTMP2.Z().VnS());
        break;
      }
      case 8: {
        uabdlb(VTMP1.Z().VnD(), Vector1.Z().VnS(), Vector2.Z().VnS());
        uabdlt(VTMP2.Z().VnD(), Vector1.Z().VnS(), Vector2.Z().VnS());
        zip1(Dst.Z().VnD(), VTMP1.Z().VnD(), VTMP2.Z().VnD());
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize >> 1);
        return;
    }
  } else {
    switch (ElementSize) {
      case 2: {
        uabdl(Dst.V8H(), Vector1.V8B(), Vector2.V8B());
        break;
      }
      case 4: {
        uabdl(Dst.V4S(), Vector1.V4H(), Vector2.V4H());
        break;
      }
      case 8: {
        uabdl(Dst.V2D(), Vector1.V2S(), Vector2.V2S());
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize >> 1);
        break;
    }
  }
}

DEF_OP(VTBL1) {
  auto Op = IROp->C<IR::IROp_VTBL1>();
  const uint8_t OpSize = IROp->Size;

  switch (OpSize) {
    case 8: {
      tbl(GetDst(Node).V8B(), GetSrc(Op->VectorTable.ID()).V16B(), GetSrc(Op->VectorIndices.ID()).V8B());
    break;
    }
    case 16: {
      tbl(GetDst(Node).V16B(), GetSrc(Op->VectorTable.ID()).V16B(), GetSrc(Op->VectorIndices.ID()).V16B());
    break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown OpSize: {}", OpSize); break;
  }
}

DEF_OP(VRev64) {
  const auto Op = IROp->C<IR::IROp_VRev64>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Elements = OpSize / ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto Vector = GetSrc(Op->Vector.ID());

  if (HostSupportsSVE && Is256Bit) {
    const auto Mask = PRED_TMP_32B.Merging();

    switch (ElementSize) {
      case 1: {
        revb(Dst.Z().VnD(), Mask, Vector.Z().VnD());
        break;
      }
      case 2: {
        revh(Dst.Z().VnD(), Mask, Vector.Z().VnD());
        break;
      }
      case 4: {
        revw(Dst.Z().VnD(), Mask, Vector.Z().VnD());
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Invalid Element Size: {}", ElementSize);
        break;
    }
  } else {
    switch (ElementSize) {
      case 1:
      case 2:
      case 4:
        rev64(Dst.VCast(OpSize * 8, Elements), Vector.VCast(OpSize * 8, Elements));
        break;
      default:
        LOGMAN_MSG_A_FMT("Invalid Element Size: {}", ElementSize);
        break;
    }
  }
}

#undef DEF_OP
void Arm64JITCore::RegisterVectorHandlers() {
#define REGISTER_OP(op, x) OpHandlers[FEXCore::IR::IROps::OP_##op] = &Arm64JITCore::Op_##x
  REGISTER_OP(VECTORZERO,        VectorZero);
  REGISTER_OP(VECTORIMM,         VectorImm);
  REGISTER_OP(VMOV,              VMov);
  REGISTER_OP(VAND,              VAnd);
  REGISTER_OP(VBIC,              VBic);
  REGISTER_OP(VOR,               VOr);
  REGISTER_OP(VXOR,              VXor);
  REGISTER_OP(VADD,              VAdd);
  REGISTER_OP(VSUB,              VSub);
  REGISTER_OP(VUQADD,            VUQAdd);
  REGISTER_OP(VUQSUB,            VUQSub);
  REGISTER_OP(VSQADD,            VSQAdd);
  REGISTER_OP(VSQSUB,            VSQSub);
  REGISTER_OP(VADDP,             VAddP);
  REGISTER_OP(VADDV,             VAddV);
  REGISTER_OP(VUMINV,            VUMinV);
  REGISTER_OP(VURAVG,            VURAvg);
  REGISTER_OP(VABS,              VAbs);
  REGISTER_OP(VPOPCOUNT,         VPopcount);
  REGISTER_OP(VFADD,             VFAdd);
  REGISTER_OP(VFADDP,            VFAddP);
  REGISTER_OP(VFSUB,             VFSub);
  REGISTER_OP(VFMUL,             VFMul);
  REGISTER_OP(VFDIV,             VFDiv);
  REGISTER_OP(VFMIN,             VFMin);
  REGISTER_OP(VFMAX,             VFMax);
  REGISTER_OP(VFRECP,            VFRecp);
  REGISTER_OP(VFSQRT,            VFSqrt);
  REGISTER_OP(VFRSQRT,           VFRSqrt);
  REGISTER_OP(VNEG,              VNeg);
  REGISTER_OP(VFNEG,             VFNeg);
  REGISTER_OP(VNOT,              VNot);
  REGISTER_OP(VUMIN,             VUMin);
  REGISTER_OP(VSMIN,             VSMin);
  REGISTER_OP(VUMAX,             VUMax);
  REGISTER_OP(VSMAX,             VSMax);
  REGISTER_OP(VZIP,              VZip);
  REGISTER_OP(VZIP2,             VZip2);
  REGISTER_OP(VUNZIP,            VUnZip);
  REGISTER_OP(VUNZIP2,           VUnZip2);
  REGISTER_OP(VBSL,              VBSL);
  REGISTER_OP(VCMPEQ,            VCMPEQ);
  REGISTER_OP(VCMPEQZ,           VCMPEQZ);
  REGISTER_OP(VCMPGT,            VCMPGT);
  REGISTER_OP(VCMPGTZ,           VCMPGTZ);
  REGISTER_OP(VCMPLTZ,           VCMPLTZ);
  REGISTER_OP(VFCMPEQ,           VFCMPEQ);
  REGISTER_OP(VFCMPNEQ,          VFCMPNEQ);
  REGISTER_OP(VFCMPLT,           VFCMPLT);
  REGISTER_OP(VFCMPGT,           VFCMPGT);
  REGISTER_OP(VFCMPLE,           VFCMPLE);
  REGISTER_OP(VFCMPORD,          VFCMPORD);
  REGISTER_OP(VFCMPUNO,          VFCMPUNO);
  REGISTER_OP(VUSHL,             VUShl);
  REGISTER_OP(VUSHR,             VUShr);
  REGISTER_OP(VSSHR,             VSShr);
  REGISTER_OP(VUSHLS,            VUShlS);
  REGISTER_OP(VUSHRS,            VUShrS);
  REGISTER_OP(VSSHRS,            VSShrS);
  REGISTER_OP(VINSELEMENT,       VInsElement);
  REGISTER_OP(VDUPELEMENT,       VDupElement);
  REGISTER_OP(VEXTR,             VExtr);
  REGISTER_OP(VSLI,              VSLI);
  REGISTER_OP(VSRI,              VSRI);
  REGISTER_OP(VUSHRI,            VUShrI);
  REGISTER_OP(VSSHRI,            VSShrI);
  REGISTER_OP(VSHLI,             VShlI);
  REGISTER_OP(VUSHRNI,           VUShrNI);
  REGISTER_OP(VUSHRNI2,          VUShrNI2);
  REGISTER_OP(VSXTL,             VSXTL);
  REGISTER_OP(VSXTL2,            VSXTL2);
  REGISTER_OP(VUXTL,             VUXTL);
  REGISTER_OP(VUXTL2,            VUXTL2);
  REGISTER_OP(VSQXTN,            VSQXTN);
  REGISTER_OP(VSQXTN2,           VSQXTN2);
  REGISTER_OP(VSQXTUN,           VSQXTUN);
  REGISTER_OP(VSQXTUN2,          VSQXTUN2);
  REGISTER_OP(VUMUL,             VMul);
  REGISTER_OP(VSMUL,             VMul);
  REGISTER_OP(VUMULL,            VUMull);
  REGISTER_OP(VSMULL,            VSMull);
  REGISTER_OP(VUMULL2,           VUMull2);
  REGISTER_OP(VSMULL2,           VSMull2);
  REGISTER_OP(VUABDL,            VUABDL);
  REGISTER_OP(VTBL1,             VTBL1);
  REGISTER_OP(VREV64,            VRev64);
#undef REGISTER_OP
}
}

