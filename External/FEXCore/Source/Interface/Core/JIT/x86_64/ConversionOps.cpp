/*
$info$
tags: backend|x86-64
$end_info$
*/

#include "Interface/Core/JIT/x86_64/JITClass.h"

#include <FEXCore/IR/IR.h>
#include <FEXCore/Utils/LogManager.h>

#include <array>
#include <stdint.h>
#include <xbyak/xbyak.h>

namespace FEXCore::CPU {

#define DEF_OP(x) void X86JITCore::Op_##x(IR::IROp_Header *IROp, IR::NodeID Node)
DEF_OP(VInsGPR) {
  const auto Op = IROp->C<IR::IROp_VInsGPR>();
  const auto OpSize = IROp->Size;

  const auto Dst = GetDst(Node);
  const auto DestVector = GetSrc(Op->DestVector.ID());

  const auto DestIdx = Op->DestIdx;
  const auto ElementSize = Op->Header.ElementSize;
  const auto ElementSizeBits = ElementSize * 8;
  const auto Offset = ElementSizeBits * DestIdx;

  constexpr auto SSEBitSize = Core::CPUState::XMM_SSE_REG_SIZE * 8;

  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;
  const auto InUpperLane = Offset >= SSEBitSize;

  if (InUpperLane && !Is256Bit) {
    LOGMAN_MSG_A_FMT("Attempt to access upper 128-bit lane in 128-bit operation! Offset={}",
                     Offset);
    return;
  }

  if (Is256Bit) {
    vmovapd(ToYMM(Dst), ToYMM(DestVector));
  } else {
    vmovapd(Dst, DestVector);
  }

  const auto Insert = [&](const Xbyak::Xmm& reg, int index) {
    switch (ElementSize) {
      case 1: {
        if (InUpperLane) {
          index -= 16;
        }
        pinsrb(reg, GetSrc<RA_32>(Op->Src.ID()), index);
        break;
      }
      case 2: {
        if (InUpperLane) {
          index -= 8;
        }
        pinsrw(reg, GetSrc<RA_32>(Op->Src.ID()), index);
        break;
      }
      case 4: {
        if (InUpperLane) {
          index -= 4;
        }
        pinsrd(reg, GetSrc<RA_32>(Op->Src.ID()), index);
        break;
      }
      case 8: {
        if (InUpperLane) {
          index -= 2;
        }
        pinsrq(reg, GetSrc<RA_64>(Op->Src.ID()), index);
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
    }
  };

  if (InUpperLane) {
    vextracti128(xmm15, ToYMM(Dst), 1);
    Insert(xmm15, DestIdx);
    vinserti128(ToYMM(Dst), ToYMM(Dst), xmm15, 1);
  } else {
    Insert(Dst, DestIdx);
  }
}

DEF_OP(VCastFromGPR) {
  auto Op = IROp->C<IR::IROp_VCastFromGPR>();
  switch (Op->Header.ElementSize) {
    case 1:
      movzx(rax, GetSrc<RA_8>(Op->Src.ID()));
      vmovq(GetDst(Node), rax);
      break;
    case 2:
      movzx(rax, GetSrc<RA_16>(Op->Src.ID()));
      vmovq(GetDst(Node), rax);
      break;
    case 4:
      vmovd(GetDst(Node), GetSrc<RA_32>(Op->Src.ID()).cvt32());
      break;
    case 8:
      vmovq(GetDst(Node), GetSrc<RA_64>(Op->Src.ID()).cvt64());
      break;
    default: LOGMAN_MSG_A_FMT("Unknown VCastFromGPR element size: {}", Op->Header.ElementSize);
  }
}

DEF_OP(Float_FromGPR_S) {
  const auto Op = IROp->C<IR::IROp_Float_FromGPR_S>();

  const uint16_t ElementSize = Op->Header.ElementSize;
  const uint16_t Conv = (ElementSize << 8) | Op->SrcElementSize;

  switch (Conv) {
    case 0x0404: { // Float <- int32_t
      cvtsi2ss(GetDst(Node), GetSrc<RA_32>(Op->Src.ID()));
      break;
    }
    case 0x0408: { // Float <- int64_t
      cvtsi2ss(GetDst(Node), GetSrc<RA_64>(Op->Src.ID()));
      break;
    }
    case 0x0804: { // Double <- int32_t
      cvtsi2sd(GetDst(Node), GetSrc<RA_32>(Op->Src.ID()));
      break;
    }
    case 0x0808: { // Double <- int64_t
      cvtsi2sd(GetDst(Node), GetSrc<RA_64>(Op->Src.ID()));
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unhandled conversion mask: Mask=0x{:04x}, ElementSize={}, SrcElementSize={}",
                       Conv, ElementSize, Op->SrcElementSize);
      break;
  }
}

DEF_OP(Float_FToF) {
  auto Op = IROp->C<IR::IROp_Float_FToF>();
  const uint16_t Conv = (Op->Header.ElementSize << 8) | Op->SrcElementSize;

  switch (Conv) {
    case 0x0804: { // Double <- Float
      cvtss2sd(GetDst(Node), GetSrc(Op->Scalar.ID()));
      break;
    }
    case 0x0408: { // Float <- Double
      cvtsd2ss(GetDst(Node), GetSrc(Op->Scalar.ID()));
      break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown Float_FToF sizes: 0x{:x}", Conv);
  }
}

DEF_OP(Vector_SToF) {
  const auto Op = IROp->C<IR::IROp_Vector_SToF>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto Vector = GetSrc(Op->Vector.ID());

  switch (ElementSize) {
    case 4:
      if (Is256Bit) {
        vcvtdq2ps(ToYMM(Dst), ToYMM(Vector));
      } else {
        vcvtdq2ps(Dst, Vector);
      }
      break;
    case 8:
      // This operation is a bit disgusting in x86
      // There is no vector form of this instruction until AVX512VL + AVX512DQ (vcvtqq2pd)
      // 1) First extract the top 64bits
      // 2) Do a scalar conversion on each
      // 3) Make sure to merge them together at the end
      pextrq(rax, Vector, 1);
      pextrq(rcx, Vector, 0);
      cvtsi2sd(Dst, rcx);
      cvtsi2sd(xmm15, rax);
      if (Is256Bit) {
        movlhps(Dst, xmm15);
        vextracti128(xmm15, ToYMM(Vector), 1);

        pextrq(rax, xmm15, 1);
        pextrq(rcx, xmm15, 0);
        cvtsi2sd(xmm15, rcx);
        cvtsi2sd(xmm14, rax);
        movlhps(xmm15, xmm14);

        vinserti128(ToYMM(Dst), ToYMM(Dst), xmm15, 1);
      } else {
        vmovlhps(Dst, Dst, xmm15);
      }
      break;
    default:
      LOGMAN_MSG_A_FMT("Unknown Vector_SToF element size: {}", ElementSize);
      break;
  }
}

DEF_OP(Vector_FToZS) {
  const auto Op = IROp->C<IR::IROp_Vector_FToZS>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto Vector = GetSrc(Op->Vector.ID());

  switch (ElementSize) {
    case 4:
      if (Is256Bit) {
        vcvttps2dq(ToYMM(Dst), ToYMM(Vector));
      } else {
        vcvttps2dq(Dst, Vector);
      }
      break;
    case 8:
      if (Is256Bit) {
        vcvttpd2dq(ToYMM(Dst), ToYMM(Vector));
      } else {
        vcvttpd2dq(Dst, Vector);
      }
      break;
    default:
      LOGMAN_MSG_A_FMT("Unknown Vector_FToZS element size: {}", ElementSize);
      break;
  }
}

DEF_OP(Vector_FToS) {
  const auto Op = IROp->C<IR::IROp_Vector_FToS>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto Vector = GetSrc(Op->Vector.ID());

  switch (ElementSize) {
    case 4:
      if (Is256Bit) {
        vcvtps2dq(ToYMM(Dst), ToYMM(Vector));
      } else {
        vcvtps2dq(Dst, Vector);
      }
      break;
    case 8:
      if (Is256Bit) {
        vcvtpd2dq(ToYMM(Dst), ToYMM(Vector));
      } else {
        vcvtpd2dq(Dst, Vector);
      }
      break;
    default:
      LOGMAN_MSG_A_FMT("Unknown Vector_FToS element size: {}", ElementSize);
      break;
  }
}

DEF_OP(Vector_FToF) {
  const auto Op = IROp->C<IR::IROp_Vector_FToF>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;
  const auto Conv = (ElementSize << 8) | Op->SrcElementSize;

  const auto Dst = GetDst(Node);
  const auto Vector = GetSrc(Op->Vector.ID());

  switch (Conv) {
    case 0x0804: { // Double <- Float
      if (Is256Bit) {
        vcvtps2pd(ToYMM(Dst), Vector);
      } else {
        vcvtps2pd(Dst, Vector);
      }
      break;
    }
    case 0x0408: { // Float <- Double
      if (Is256Bit) {
        vcvtpd2ps(Dst, ToYMM(Vector));
      } else {
        vcvtpd2ps(Dst, Vector);
      }
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unknown Vector_FToF conversion type : 0x{:04x}", Conv);
      break;
  }
}

DEF_OP(Vector_FToI) {
  const auto Op = IROp->C<IR::IROp_Vector_FToI>();
  const auto OpSize = IROp->Size;

  const uint8_t RoundMode = [Op] {
    switch (Op->Round) {
      case FEXCore::IR::Round_Nearest.Val:
        return 0b0000'0'0'00;
      case FEXCore::IR::Round_Negative_Infinity.Val:
        return 0b0000'0'0'01;
      case FEXCore::IR::Round_Positive_Infinity.Val:
        return 0b0000'0'0'10;
      case FEXCore::IR::Round_Towards_Zero.Val:
        return 0b0000'0'0'11;
      case FEXCore::IR::Round_Host.Val:
        return 0b0000'0'1'00;
      default:
        LOGMAN_MSG_A_FMT("Unhandled rounding mode");
        return 0;
    }
  }();

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto Vector = GetSrc(Op->Vector.ID());

  switch (ElementSize) {
    case 4:
      if (Is256Bit) {
        vroundps(ToYMM(Dst), ToYMM(Vector), RoundMode);
      } else {
        vroundps(Dst, Vector, RoundMode);
      }
      break;
    case 8:
      if (Is256Bit) {
        vroundpd(ToYMM(Dst), ToYMM(Vector), RoundMode);
      } else {
        vroundpd(Dst, Vector, RoundMode);
      }
      break;
    default:
      LOGMAN_MSG_A_FMT("Unhandled element size: {}", ElementSize);
      break;
  }
}

#undef DEF_OP
void X86JITCore::RegisterConversionHandlers() {
#define REGISTER_OP(op, x) OpHandlers[FEXCore::IR::IROps::OP_##op] = &X86JITCore::Op_##x
  REGISTER_OP(VINSGPR,         VInsGPR);
  REGISTER_OP(VCASTFROMGPR,    VCastFromGPR);
  REGISTER_OP(FLOAT_FROMGPR_S, Float_FromGPR_S);
  REGISTER_OP(FLOAT_FTOF,      Float_FToF);
  REGISTER_OP(VECTOR_STOF,     Vector_SToF);
  REGISTER_OP(VECTOR_FTOZS,    Vector_FToZS);
  REGISTER_OP(VECTOR_FTOS,     Vector_FToS);
  REGISTER_OP(VECTOR_FTOF,     Vector_FToF);
  REGISTER_OP(VECTOR_FTOI,     Vector_FToI);
#undef REGISTER_OP
}
}

