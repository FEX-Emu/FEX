#include "Interface/Core/JIT/Arm64/JITClass.h"

namespace FEXCore::CPU {

using namespace vixl;
using namespace vixl::aarch64;
#define DEF_OP(x) void JITCore::Op_##x(FEXCore::IR::IROp_Header *IROp, uint32_t Node)
DEF_OP(VInsGPR) {
  auto Op = IROp->C<IR::IROp_VInsGPR>();
  mov(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
  switch (Op->Header.ElementSize) {
    case 1: {
      ins(GetDst(Node).V16B(), Op->Index, GetReg<RA_32>(Op->Header.Args[1].ID()));
    break;
    }
    case 2: {
      ins(GetDst(Node).V8H(), Op->Index, GetReg<RA_32>(Op->Header.Args[1].ID()));
    break;
    }
    case 4: {
      ins(GetDst(Node).V4S(), Op->Index, GetReg<RA_32>(Op->Header.Args[1].ID()));
    break;
    }
    case 8: {
      ins(GetDst(Node).V2D(), Op->Index, GetReg<RA_64>(Op->Header.Args[1].ID()));
    break;
    }
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
  }
}

DEF_OP(VCastFromGPR) {
  auto Op = IROp->C<IR::IROp_VCastFromGPR>();
  switch (Op->Header.ElementSize) {
    case 1:
      uxtb(TMP1.W(), GetReg<RA_32>(Op->Header.Args[0].ID()).W());
      fmov(GetDst(Node).S(), TMP1.W());
      break;
    case 2:
      uxth(TMP1.W(), GetReg<RA_32>(Op->Header.Args[0].ID()).W());
      fmov(GetDst(Node).S(), TMP1.W());
      break;
    case 4:
      fmov(GetDst(Node).S(), GetReg<RA_32>(Op->Header.Args[0].ID()).W());
      break;
    case 8:
      fmov(GetDst(Node).D(), GetReg<RA_64>(Op->Header.Args[0].ID()).X());
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
    scvtf(GetDst(Node).D(), GetReg<RA_64>(Op->Header.Args[0].ID()));
  }
  else {
    scvtf(GetDst(Node).S(), GetReg<RA_32>(Op->Header.Args[0].ID()));
  }
}

DEF_OP(Float_FToF) {
  auto Op = IROp->C<IR::IROp_Float_FToF>();
  uint16_t Conv = (Op->Header.ElementSize << 8) | Op->SrcElementSize;
  switch (Conv) {
    case 0x0804: { // Double <- Float
      fcvt(GetDst(Node).D(), GetSrc(Op->Header.Args[0].ID()).S());
      break;
    }
    case 0x0408: { // Float <- Double
      fcvt(GetDst(Node).S(), GetSrc(Op->Header.Args[0].ID()).D());
      break;
    }
    default: LogMan::Msg::A("Unknown FCVT sizes: 0x%x", Conv);
  }
}

DEF_OP(Vector_UToF) {
  auto Op = IROp->C<IR::IROp_Vector_UToF>();
  switch (Op->Header.ElementSize) {
    case 4:
      ucvtf(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S());
    break;
    case 8:
      ucvtf(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D());
    break;
    default: LogMan::Msg::A("Unknown castGPR element size: %d", Op->Header.ElementSize);
  }
}

DEF_OP(Vector_SToF) {
  auto Op = IROp->C<IR::IROp_Vector_SToF>();
  switch (Op->Header.ElementSize) {
    case 4:
      scvtf(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S());
    break;
    case 8:
      scvtf(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D());
    break;
    default: LogMan::Msg::A("Unknown castGPR element size: %d", Op->Header.ElementSize);
  }
}

DEF_OP(Vector_FToZU) {
  auto Op = IROp->C<IR::IROp_Vector_FToZU>();
  switch (Op->Header.ElementSize) {
    case 4:
      fcvtzu(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S());
    break;
    case 8:
      fcvtzu(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D());
    break;
    default: LogMan::Msg::A("Unknown castGPR element size: %d", Op->Header.ElementSize);
  }
}

DEF_OP(Vector_FToZS) {
  auto Op = IROp->C<IR::IROp_Vector_FToZS>();
  switch (Op->Header.ElementSize) {
    case 4:
      fcvtzs(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S());
    break;
    case 8:
      fcvtzs(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D());
    break;
    default: LogMan::Msg::A("Unknown castGPR element size: %d", Op->Header.ElementSize);
  }
}

DEF_OP(Vector_FToU) {
  auto Op = IROp->C<IR::IROp_Vector_FToU>();
  switch (Op->Header.ElementSize) {
    case 4:
      frinti(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S());
      fcvtzu(GetDst(Node).V4S(), GetDst(Node).V4S());
    break;
    case 8:
      frinti(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D());
      fcvtzu(GetDst(Node).V2D(), GetDst(Node).V2D());
    break;
    default: LogMan::Msg::A("Unknown castGPR element size: %d", Op->Header.ElementSize);
  }
}

DEF_OP(Vector_FToS) {
  auto Op = IROp->C<IR::IROp_Vector_FToS>();
  switch (Op->Header.ElementSize) {
    case 4:
      frinti(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S());
      fcvtzs(GetDst(Node).V4S(), GetDst(Node).V4S());
    break;
    case 8:
      frinti(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D());
      fcvtzs(GetDst(Node).V2D(), GetDst(Node).V2D());
    break;
    default: LogMan::Msg::A("Unknown castGPR element size: %d", Op->Header.ElementSize);
  }
}

DEF_OP(Vector_FToF) {
  auto Op = IROp->C<IR::IROp_Vector_FToF>();
  uint16_t Conv = (Op->Header.ElementSize << 8) | Op->SrcElementSize;

  switch (Conv) {
    case 0x0804: { // Double <- Float
      fcvtl(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2S());
      break;
    }
    case 0x0408: { // Float <- Double
      fcvtn(GetDst(Node).V2S(), GetSrc(Op->Header.Args[0].ID()).V2D());
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

