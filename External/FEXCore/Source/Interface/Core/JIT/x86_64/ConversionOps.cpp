#include "Interface/Core/JIT/x86_64/JITClass.h"
#include "Interface/IR/Passes/RegisterAllocationPass.h"

namespace FEXCore::CPU {

#define DEF_OP(x) void JITCore::Op_##x(FEXCore::IR::IROp_Header *IROp, uint32_t Node)
DEF_OP(VInsGPR) {
  auto Op = IROp->C<IR::IROp_VInsGPR>();
  movapd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));

  switch (Op->Header.ElementSize) {
    case 1: {
      pinsrb(GetDst(Node), GetSrc<RA_32>(Op->Header.Args[1].ID()), Op->Index);
      break;
    }
    case 2: {
      pinsrw(GetDst(Node), GetSrc<RA_32>(Op->Header.Args[1].ID()), Op->Index);
      break;
    }
    case 4: {
      pinsrd(GetDst(Node), GetSrc<RA_32>(Op->Header.Args[1].ID()), Op->Index);
      break;
    }
    case 8: {
      pinsrq(GetDst(Node), GetSrc<RA_64>(Op->Header.Args[1].ID()), Op->Index);
      break;
    }
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
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
    default: LogMan::Msg::A("Unknown castGPR element size: %d", Op->Header.ElementSize);
  }
}

DEF_OP(Float_FromGPR_U) {
  LogMan::Msg::A("Unimplemented");
}

DEF_OP(Float_FromGPR_S) {
  auto Op = IROp->C<IR::IROp_Float_FromGPR_S>();
  if (Op->Header.ElementSize == 8) {
    cvtsi2sd(GetDst(Node), GetSrc<RA_64>(Op->Header.Args[0].ID()));
  }
  else {
    cvtsi2ss(GetDst(Node), GetSrc<RA_32>(Op->Header.Args[0].ID()));
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
    default: LogMan::Msg::A("Unknown FCVT sizes: 0x%x", Conv);
  }
}

DEF_OP(Vector_UToF) {
  LogMan::Msg::A("Unimplemented");
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
    default: LogMan::Msg::A("Unknown castGPR element size: %d", Op->Header.ElementSize);
  }
}

DEF_OP(Vector_FToZU) {
  LogMan::Msg::A("Unimplemented");
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
    default: LogMan::Msg::A("Unknown castGPR element size: %d", Op->Header.ElementSize);
  }
}

DEF_OP(Vector_FToU) {
  LogMan::Msg::A("Unimplemented");
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
    default: LogMan::Msg::A("Unknown castGPR element size: %d", Op->Header.ElementSize);
  }
}

DEF_OP(Vector_FToF) {
  auto Op = IROp->C<IR::IROp_Vector_FToF>();
  uint16_t Conv = (Op->Header.ElementSize << 8) | Op->SrcElementSize;

  switch (Conv) {
    case 0x0804: { // Double <- Float
      cvtps2pd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
      break;
    }
    case 0x0408: { // Float <- Double
      cvtpd2ps(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
      break;
    }
    default: LogMan::Msg::A("Unknown Conversion Type : 0%04x", Conv); break;
  }
}

#undef DEF_OP
void JITCore::RegisterConversionHandlers() {
#define REGISTER_OP(op, x) OpHandlers[FEXCore::IR::IROps::OP_##op] = &JITCore::Op_##x
  REGISTER_OP(VINSGPR,         VInsGPR);
  REGISTER_OP(VCASTFROMGPR,    VCastFromGPR);
  REGISTER_OP(FLOAT_FROMGPR_U, Float_FromGPR_U);
  REGISTER_OP(FLOAT_FROMGPR_S, Float_FromGPR_S);
  REGISTER_OP(FLOAT_FTOF,      Float_FToF);
  REGISTER_OP(VECTOR_UTOF,     Vector_UToF);
  REGISTER_OP(VECTOR_STOF,     Vector_SToF);
  REGISTER_OP(VECTOR_FTOZU,    Vector_FToZU);
  REGISTER_OP(VECTOR_FTOZS,    Vector_FToZS);
  REGISTER_OP(VECTOR_FTOU,     Vector_FToU);
  REGISTER_OP(VECTOR_FTOS,     Vector_FToS);
  REGISTER_OP(VECTOR_FTOF,     Vector_FToF);
#undef REGISTER_OP
}
}

