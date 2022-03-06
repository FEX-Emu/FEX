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
  auto Op = IROp->C<IR::IROp_VInsGPR>();
  movapd(GetDst(Node), GetSrc(Op->DestVector.ID()));

  switch (Op->Header.ElementSize) {
    case 1: {
      pinsrb(GetDst(Node), GetSrc<RA_32>(Op->Src.ID()), Op->DestIdx);
      break;
    }
    case 2: {
      pinsrw(GetDst(Node), GetSrc<RA_32>(Op->Src.ID()), Op->DestIdx);
      break;
    }
    case 4: {
      pinsrd(GetDst(Node), GetSrc<RA_32>(Op->Src.ID()), Op->DestIdx);
      break;
    }
    case 8: {
      pinsrq(GetDst(Node), GetSrc<RA_64>(Op->Src.ID()), Op->DestIdx);
      break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
}

DEF_OP(VCastFromGPR) {
  auto Op = IROp->C<IR::IROp_VCastFromGPR>();
  switch (Op->Header.ElementSize) {
    case 1:
      movzx(rax, GetSrc<RA_8>(Op->Header.Args[0].ID()));
      vmovq(GetDst(Node), rax);
      break;
    case 2:
      movzx(rax, GetSrc<RA_16>(Op->Header.Args[0].ID()));
      vmovq(GetDst(Node), rax);
      break;
    case 4:
      vmovd(GetDst(Node), GetSrc<RA_32>(Op->Header.Args[0].ID()).cvt32());
      break;
    case 8:
      vmovq(GetDst(Node), GetSrc<RA_64>(Op->Header.Args[0].ID()).cvt64());
      break;
    default: LOGMAN_MSG_A_FMT("Unknown VCastFromGPR element size: {}", Op->Header.ElementSize);
  }
}

DEF_OP(Float_FromGPR_S) {
  auto Op = IROp->C<IR::IROp_Float_FromGPR_S>();
  uint16_t Conv = (Op->Header.ElementSize << 8) | Op->SrcElementSize;
  switch (Conv) {
    case 0x0404: { // Float <- int32_t
      cvtsi2ss(GetDst(Node), GetSrc<RA_32>(Op->Header.Args[0].ID()));
      break;
    }
    case 0x0408: { // Float <- int64_t
      cvtsi2ss(GetDst(Node), GetSrc<RA_64>(Op->Header.Args[0].ID()));
      break;
    }
    case 0x0804: { // Double <- int32_t
      cvtsi2sd(GetDst(Node), GetSrc<RA_32>(Op->Header.Args[0].ID()));
      break;
    }
    case 0x0808: { // Double <- int64_t
      cvtsi2sd(GetDst(Node), GetSrc<RA_64>(Op->Header.Args[0].ID()));
      break;
    }
  }
}

DEF_OP(Float_FToF) {
  auto Op = IROp->C<IR::IROp_Float_FToF>();
  uint16_t Conv = (Op->Header.ElementSize << 8) | Op->SrcElementSize;
  switch (Conv) {
    case 0x0804: { // Double <- Float
      cvtss2sd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
      break;
    }
    case 0x0408: { // Float <- Double
      cvtsd2ss(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
      break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown Float_FToF sizes: 0x{:x}", Conv);
  }
}

DEF_OP(Vector_SToF) {
  auto Op = IROp->C<IR::IROp_Vector_SToF>();
  switch (Op->Header.ElementSize) {
    case 4:
      cvtdq2ps(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
    break;
    case 8:
      // This operation is a bit disgusting in x86
      // There is no vector form of this instruction until AVX512VL + AVX512DQ (vcvtqq2pd)
      // 1) First extract the top 64bits
      // 2) Do a scalar conversion on each
      // 3) Make sure to merge them together at the end
      pextrq(rax, GetSrc(Op->Header.Args[0].ID()), 1);
      pextrq(rcx, GetSrc(Op->Header.Args[0].ID()), 0);
      cvtsi2sd(GetDst(Node), rcx);
      cvtsi2sd(xmm15, rax);
      movlhps(GetDst(Node), xmm15);
    break;
    default: LOGMAN_MSG_A_FMT("Unknown Vector_SToF element size: {}", Op->Header.ElementSize);
  }
}

DEF_OP(Vector_FToZS) {
  auto Op = IROp->C<IR::IROp_Vector_FToZS>();
  switch (Op->Header.ElementSize) {
    case 4:
      cvttps2dq(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
    break;
    case 8:
      cvttpd2dq(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
    break;
    default: LOGMAN_MSG_A_FMT("Unknown Vector_FToZS element size: {}", Op->Header.ElementSize);
  }
}

DEF_OP(Vector_FToS) {
  auto Op = IROp->C<IR::IROp_Vector_FToS>();
  switch (Op->Header.ElementSize) {
    case 4:
      cvtps2dq(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
    break;
    case 8:
      cvtpd2dq(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
    break;
    default: LOGMAN_MSG_A_FMT("Unknown Vector_FToS element size: {}", Op->Header.ElementSize);
  }
}

DEF_OP(Vector_FToF) {
  auto Op = IROp->C<IR::IROp_Vector_FToF>();
  uint16_t Conv = (Op->Header.ElementSize << 8) | Op->SrcElementSize;

  switch (Conv) {
    case 0x0402: { // Float <- Float16
      vcvtph2ps(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
      break;
    }
    case 0x0804: { // Double <- Float
      cvtps2pd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
      break;
    }
    case 0x0408: { // Float <- Double
      cvtpd2ps(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
      break;
    }
    case 0x0204: { // Float16 <-- Float
      // Host rounding mode
      vcvtps2ph(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), 0b100);
      break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown Vector_FToF conversion type : 0x{:04x}", Conv); break;
  }
}

DEF_OP(Vector_FToI) {
  auto Op = IROp->C<IR::IROp_Vector_FToI>();
  uint8_t RoundMode{};

  switch (Op->Round) {
    case FEXCore::IR::Round_Nearest.Val:
      RoundMode = 0b0000'0'0'00;
    break;
    case FEXCore::IR::Round_Negative_Infinity.Val:
      RoundMode = 0b0000'0'0'01;
    break;
    case FEXCore::IR::Round_Positive_Infinity.Val:
      RoundMode = 0b0000'0'0'10;
    break;
    case FEXCore::IR::Round_Towards_Zero.Val:
      RoundMode = 0b0000'0'0'11;
    break;
    case FEXCore::IR::Round_Host.Val:
      RoundMode = 0b0000'0'1'00;
    break;
  }

  switch (Op->Header.ElementSize) {
    case 4:
      roundps(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), RoundMode);
    break;
    case 8:
      roundpd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), RoundMode);
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

