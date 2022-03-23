/*
$info$
tags: backend|riscv64
$end_info$
*/

#include "Interface/Core/JIT/RISCV/JITClass.h"
#include <FEXCore/Utils/CompilerDefs.h>

namespace FEXCore::CPU {
using namespace biscuit;
#define DEF_OP(x) void RISCVJITCore::Op_##x(IR::IROp_Header *IROp, IR::NodeID Node)
DEF_OP(VInsGPR) {
  auto Op = IROp->C<IR::IROp_VInsGPR>();
  auto Dst  = GetVIndex(Node);
  auto Src1 = GetVIndex(Op->DestVector.ID());
  auto Src2 = GetReg(Op->Src.ID());

  LD(TMP1, Src1 * 16 + 0, FPRSTATE);
  SD(TMP1, Dst * 16 + 0, FPRSTATE);
  LD(TMP1, Src1 * 16 + 8, FPRSTATE);
  SD(zero, Dst * 16 + 8, FPRSTATE);

  STSize(Op->Header.ElementSize, Src2, Dst * 16 + (Op->DestIdx * Op->Header.ElementSize), FPRSTATE);
}

DEF_OP(VCastFromGPR) {
  auto Op = IROp->C<IR::IROp_VCastFromGPR>();
  auto Dst  = GetVIndex(Node);
  auto Src = GetReg(Op->Src.ID());

  SD(zero, Dst * 16 + 0, FPRSTATE);
  SD(zero, Dst * 16 + 8, FPRSTATE);

  STSize(Op->Header.ElementSize, Src, Dst * 16 + 0, FPRSTATE);
}

DEF_OP(Float_FromGPR_S) {
  auto Op = IROp->C<IR::IROp_Float_FromGPR_S>();
  auto Dst  = GetVIndex(Node);
  uint16_t Conv = (Op->Header.ElementSize << 8) | Op->SrcElementSize;

  SD(zero, Dst * 16 + 0, FPRSTATE);
  SD(zero, Dst * 16 + 8, FPRSTATE);

  switch (Conv) {
    case 0x0404: { // Float <- int32_t
      FCVT_S_W(VTMP1, GetReg(Op->Src.ID()));
      break;
    }
    case 0x0408: { // Float <- int64_t
      FCVT_S_L(VTMP1, GetReg(Op->Src.ID()));
      break;
    }
    case 0x0804: { // Double <- int32_t
      FCVT_D_W(VTMP1, GetReg(Op->Src.ID()));
      break;
    }
    case 0x0808: { // Double <- int64_t
      FCVT_D_L(VTMP1, GetReg(Op->Src.ID()));
      break;
    }
  }

  STSize(Op->Header.ElementSize, VTMP1, Dst * 16 + 0, FPRSTATE);
}


DEF_OP(Float_FToF) {
  auto Op = IROp->C<IR::IROp_Float_FToF>();
  auto Dst  = GetVIndex(Node);
  auto Src1 = GetVIndex(Op->Header.Args[0].ID());

  LDSize(Op->SrcElementSize, VTMP1, Src1 * 16 + 0, FPRSTATE);
  SD(zero, Dst * 16 + 0, FPRSTATE);
  SD(zero, Dst * 16 + 8, FPRSTATE);
  uint16_t Conv = (Op->Header.ElementSize << 8) | Op->SrcElementSize;
  if (Conv == 0x0804) { // Double <- Float
    FCVT_D_S(VTMP1, VTMP1);
  }
  else { // Float <- Double
    FCVT_S_D(VTMP1, VTMP1);
  }
  STSize(Op->Header.ElementSize, VTMP1, Src1 * 16 + 0, FPRSTATE);
}

DEF_OP(Vector_SToF) {
  auto Op = IROp->C<IR::IROp_Vector_SToF>();
  auto Dst  = GetVIndex(Node);
  auto Src1 = GetVIndex(Op->Header.Args[0].ID());

  uint8_t Elements = IROp->Size / Op->Header.ElementSize;

  for (size_t i = 0; i < Elements; ++i) {
    LDUSize(Op->Header.ElementSize, TMP1, Src1 * 16 + (i * Op->Header.ElementSize), FPRSTATE);
    // Doesn't do size conversion
    if (Op->Header.ElementSize == 4) {
      FCVT_S_W(VTMP1, TMP1);
    }
    else {
      FCVT_D_L(VTMP1, TMP1);
    }
    STSize(Op->Header.ElementSize, VTMP1, Dst * 16 + (i * Op->Header.ElementSize), FPRSTATE);
  }
}


#undef DEF_OP
void RISCVJITCore::RegisterConversionHandlers() {
#define REGISTER_OP(op, x) OpHandlers[FEXCore::IR::IROps::OP_##op] = &RISCVJITCore::Op_##x
  REGISTER_OP(VINSGPR,         VInsGPR);
  REGISTER_OP(VCASTFROMGPR,    VCastFromGPR);
  REGISTER_OP(FLOAT_FROMGPR_S, Float_FromGPR_S);
  REGISTER_OP(FLOAT_FTOF,      Float_FToF);
  REGISTER_OP(VECTOR_STOF,     Vector_SToF);
  //REGISTER_OP(VECTOR_FTOZS,    Vector_FToZS);
  //REGISTER_OP(VECTOR_FTOS,     Vector_FToS);
  //REGISTER_OP(VECTOR_FTOF,     Vector_FToF);
  //REGISTER_OP(VECTOR_FTOI,     Vector_FToI);
#undef REGISTER_OP
}
}

