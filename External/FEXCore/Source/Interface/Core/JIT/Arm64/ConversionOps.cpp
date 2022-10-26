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
DEF_OP(VInsGPR) {
  auto Op = IROp->C<IR::IROp_VInsGPR>();
  mov(GetDst(Node), GetSrc(Op->DestVector.ID()));
  switch (Op->Header.ElementSize) {
    case 1: {
      ins(GetDst(Node).V16B(), Op->DestIdx, GetReg<RA_32>(Op->Src.ID()));
    break;
    }
    case 2: {
      ins(GetDst(Node).V8H(), Op->DestIdx, GetReg<RA_32>(Op->Src.ID()));
    break;
    }
    case 4: {
      ins(GetDst(Node).V4S(), Op->DestIdx, GetReg<RA_32>(Op->Src.ID()));
    break;
    }
    case 8: {
      ins(GetDst(Node).V2D(), Op->DestIdx, GetReg<RA_64>(Op->Src.ID()));
    break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
}

DEF_OP(VCastFromGPR) {
  auto Op = IROp->C<IR::IROp_VCastFromGPR>();
  switch (Op->Header.ElementSize) {
    case 1:
      uxtb(TMP1.W(), GetReg<RA_32>(Op->Src.ID()));
      fmov(GetDst(Node).S(), TMP1.W());
      break;
    case 2:
      uxth(TMP1.W(), GetReg<RA_32>(Op->Src.ID()));
      fmov(GetDst(Node).S(), TMP1.W());
      break;
    case 4:
      fmov(GetDst(Node).S(), GetReg<RA_32>(Op->Src.ID()).W());
      break;
    case 8:
      fmov(GetDst(Node).D(), GetReg<RA_64>(Op->Src.ID()).X());
      break;
    default: LOGMAN_MSG_A_FMT("Unknown castGPR element size: {}", Op->Header.ElementSize);
  }
}

DEF_OP(Float_FromGPR_S) {
  auto Op = IROp->C<IR::IROp_Float_FromGPR_S>();
  const uint16_t Conv = (Op->Header.ElementSize << 8) | Op->SrcElementSize;
  switch (Conv) {
    case 0x0404: { // Float <- int32_t
      scvtf(GetDst(Node).S(), GetReg<RA_32>(Op->Src.ID()));
      break;
    }
    case 0x0408: { // Float <- int64_t
      scvtf(GetDst(Node).S(), GetReg<RA_64>(Op->Src.ID()));
      break;
    }
    case 0x0804: { // Double <- int32_t
      scvtf(GetDst(Node).D(), GetReg<RA_32>(Op->Src.ID()));
      break;
    }
    case 0x0808: { // Double <- int64_t
      scvtf(GetDst(Node).D(), GetReg<RA_64>(Op->Src.ID()));
      break;
    }
  }
}

DEF_OP(Float_FToF) {
  auto Op = IROp->C<IR::IROp_Float_FToF>();
  const uint16_t Conv = (Op->Header.ElementSize << 8) | Op->SrcElementSize;
  switch (Conv) {
    case 0x0804: { // Double <- Float
      fcvt(GetDst(Node).D(), GetSrc(Op->Scalar.ID()).S());
      break;
    }
    case 0x0408: { // Float <- Double
      fcvt(GetDst(Node).S(), GetSrc(Op->Scalar.ID()).D());
      break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown FCVT sizes: 0x{:x}", Conv);
  }
}

DEF_OP(Vector_SToF) {
  const auto Op = IROp->C<IR::IROp_Vector_SToF>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto Vector = GetSrc(Op->Vector.ID());

  if (HostSupportsSVE && Is256Bit) {
    const auto Mask = PRED_TMP_32B.Merging();

    switch (ElementSize) {
      case 2:
        scvtf(Dst.Z().VnH(), Mask, Vector.Z().VnH());
        break;
      case 4:
        scvtf(Dst.Z().VnS(), Mask, Vector.Z().VnS());
        break;
      case 8:
        scvtf(Dst.Z().VnD(), Mask, Vector.Z().VnD());
        break;
      default:
        LOGMAN_MSG_A_FMT("Unknown Vector_SToF element size: {}", ElementSize);
        break;
    }
  } else {
    switch (ElementSize) {
      case 2:
        scvtf(Dst.V8H(), Vector.V8H());
        break;
      case 4:
        scvtf(Dst.V4S(), Vector.V4S());
        break;
      case 8:
        scvtf(Dst.V2D(), Vector.V2D());
        break;
      default:
        LOGMAN_MSG_A_FMT("Unknown Vector_SToF element size: {}", ElementSize);
        break;
    }
  }
}

DEF_OP(Vector_FToZS) {
  const auto Op = IROp->C<IR::IROp_Vector_FToZS>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto Vector = GetSrc(Op->Vector.ID());

  if (HostSupportsSVE && Is256Bit) {
    const auto Mask = PRED_TMP_32B.Merging();

    switch (ElementSize) {
      case 2:
        fcvtzs(Dst.Z().VnH(), Mask, Vector.Z().VnH());
        break;
      case 4:
        fcvtzs(Dst.Z().VnS(), Mask, Vector.Z().VnS());
        break;
      case 8:
        fcvtzs(Dst.Z().VnD(), Mask, Vector.Z().VnD());
        break;
      default:
        LOGMAN_MSG_A_FMT("Unknown Vector_FToZS element size: {}", ElementSize);
        break;
    }
  } else {
    switch (ElementSize) {
      case 2:
        fcvtzs(Dst.V8H(), Vector.V8H());
        break;
      case 4:
        fcvtzs(Dst.V4S(), Vector.V4S());
        break;
      case 8:
        fcvtzs(Dst.V2D(), Vector.V2D());
        break;
      default:
        LOGMAN_MSG_A_FMT("Unknown Vector_FToZS element size: {}", ElementSize);
        break;
    }
  }
}

DEF_OP(Vector_FToS) {
  const auto Op = IROp->C<IR::IROp_Vector_FToS>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto Vector = GetSrc(Op->Vector.ID());

  if (HostSupportsSVE && Is256Bit) {
    const auto Mask = PRED_TMP_32B.Merging();

    switch (ElementSize) {
      case 2:
        frinti(Dst.Z().VnH(), Mask, Vector.Z().VnH());
        fcvtzs(Dst.Z().VnH(), Mask, Dst.Z().VnH());
        break;
      case 4:
        frinti(Dst.Z().VnS(), Mask, Vector.Z().VnS());
        fcvtzs(Dst.Z().VnS(), Mask, Dst.Z().VnS());
        break;
      case 8:
        frinti(Dst.Z().VnD(), Mask, Vector.Z().VnD());
        fcvtzs(Dst.Z().VnD(), Mask, Dst.Z().VnD());
        break;
      default:
        LOGMAN_MSG_A_FMT("Unknown Vector_FToS element size: {}", ElementSize);
        break;
    }
  } else {
    switch (ElementSize) {
      case 2:
        frinti(Dst.V8H(), Vector.V8H());
        fcvtzs(Dst.V8H(), Dst.V8H());
        break;
      case 4:
        frinti(Dst.V4S(), Vector.V4S());
        fcvtzs(Dst.V4S(), Dst.V4S());
        break;
      case 8:
        frinti(Dst.V2D(), Vector.V2D());
        fcvtzs(Dst.V2D(), Dst.V2D());
        break;
      default:
        LOGMAN_MSG_A_FMT("Unknown Vector_FToS element size: {}", ElementSize);
        break;
    }
  }
}

DEF_OP(Vector_FToF) {
  auto Op = IROp->C<IR::IROp_Vector_FToF>();
  uint16_t Conv = (Op->Header.ElementSize << 8) | Op->SrcElementSize;

  switch (Conv) {
    case 0x0804: { // Double <- Float
      fcvtl(GetDst(Node).V2D(), GetSrc(Op->Vector.ID()).V2S());
      break;
    }
    case 0x0408: { // Float <- Double
      fcvtn(GetDst(Node).V2S(), GetSrc(Op->Vector.ID()).V2D());
      break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown Vector_FToF Type : 0x{:04x}", Conv); break;
  }
}

DEF_OP(Vector_FToI) {
  const auto Op = IROp->C<IR::IROp_Vector_FToI>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto Vector = GetSrc(Op->Vector.ID());

  if (HostSupportsSVE && Is256Bit) {
    const auto Mask = PRED_TMP_32B.Merging();
    
    switch (Op->Round) {
      case FEXCore::IR::Round_Nearest.Val:
        switch (ElementSize) {
          case 2:
            frintn(Dst.Z().VnH(), Mask, Vector.Z().VnH());
            break;
          case 4:
            frintn(Dst.Z().VnS(), Mask, Vector.Z().VnS());
            break;
          case 8:
            frintn(Dst.Z().VnD(), Mask, Vector.Z().VnD());
            break;
        }
        break;

      case FEXCore::IR::Round_Negative_Infinity.Val:
        switch (ElementSize) {
          case 2:
            frintm(Dst.Z().VnH(), Mask, Vector.Z().VnH());
            break;
          case 4:
            frintm(Dst.Z().VnS(), Mask, Vector.Z().VnS());
            break;
          case 8:
            frintm(Dst.Z().VnD(), Mask, Vector.Z().VnD());
            break;
        }
        break;

      case FEXCore::IR::Round_Positive_Infinity.Val:
        switch (ElementSize) {
          case 2:
            frintp(Dst.Z().VnH(), Mask, Vector.Z().VnH());
            break;
          case 4:
            frintp(Dst.Z().VnS(), Mask, Vector.Z().VnS());
            break;
          case 8:
            frintp(Dst.Z().VnD(), Mask, Vector.Z().VnD());
            break;
        }
        break;

      case FEXCore::IR::Round_Towards_Zero.Val:
        switch (ElementSize) {
          case 2:
            frintz(Dst.Z().VnH(), Mask, Vector.Z().VnH());
            break;
          case 4:
            frintz(Dst.Z().VnS(), Mask, Vector.Z().VnS());
            break;
          case 8:
            frintz(Dst.Z().VnD(), Mask, Vector.Z().VnD());
            break;
        }
        break;

      case FEXCore::IR::Round_Host.Val:
        switch (ElementSize) {
          case 2:
            frinti(Dst.Z().VnH(), Mask, Vector.Z().VnH());
            break;
          case 4:
            frinti(Dst.Z().VnS(), Mask, Vector.Z().VnS());
            break;
          case 8:
            frinti(Dst.Z().VnD(), Mask, Vector.Z().VnD());
            break;
        }
        break;
    }
  } else {
    switch (Op->Round) {
      case FEXCore::IR::Round_Nearest.Val:
        switch (ElementSize) {
          case 2:
            frintn(Dst.V8H(), Vector.V8H());
            break;
          case 4:
            frintn(Dst.V4S(), Vector.V4S());
            break;
          case 8:
            frintn(Dst.V2D(), Vector.V2D());
            break;
        }
        break;

      case FEXCore::IR::Round_Negative_Infinity.Val:
        switch (ElementSize) {
          case 2:
            frintm(Dst.V8H(), Vector.V8H());
            break;
          case 4:
            frintm(Dst.V4S(), Vector.V4S());
            break;
          case 8:
            frintm(Dst.V2D(), Vector.V2D());
            break;
        }
        break;

      case FEXCore::IR::Round_Positive_Infinity.Val:
        switch (ElementSize) {
          case 2:
            frintp(Dst.V8H(), Vector.V8H());
            break;
          case 4:
            frintp(Dst.V4S(), Vector.V4S());
            break;
          case 8:
            frintp(Dst.V2D(), Vector.V2D());
            break;
        }
        break;

      case FEXCore::IR::Round_Towards_Zero.Val:
        switch (ElementSize) {
          case 2:
            frintz(Dst.V8H(), Vector.V8H());
            break;
          case 4:
            frintz(Dst.V4S(), Vector.V4S());
            break;
          case 8:
            frintz(Dst.V2D(), Vector.V2D());
            break;
        }
        break;

      case FEXCore::IR::Round_Host.Val:
        switch (ElementSize) {
          case 2:
            frinti(Dst.V8H(), Vector.V8H());
            break;
          case 4:
            frinti(Dst.V4S(), Vector.V4S());
            break;
          case 8:
            frinti(Dst.V2D(), Vector.V2D());
            break;
        }
        break;
    }
  }
}

#undef DEF_OP
void Arm64JITCore::RegisterConversionHandlers() {
#define REGISTER_OP(op, x) OpHandlers[FEXCore::IR::IROps::OP_##op] = &Arm64JITCore::Op_##x
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

