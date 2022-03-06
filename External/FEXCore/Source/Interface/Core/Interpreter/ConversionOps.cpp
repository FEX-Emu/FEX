/*
$info$
tags: backend|interpreter
$end_info$
*/

#include "Interface/Core/Interpreter/InterpreterClass.h"
#include "Interface/Core/Interpreter/InterpreterOps.h"
#include "Interface/Core/Interpreter/InterpreterDefines.h"

#include <cstdint>

namespace FEXCore::CPU {
#define DEF_OP(x) void InterpreterOps::Op_##x(IR::IROp_Header *IROp, IROpData *Data, IR::NodeID Node)
DEF_OP(VInsGPR) {
  auto Op = IROp->C<IR::IROp_VInsGPR>();
  uint8_t OpSize = IROp->Size;

  __uint128_t Src1 = *GetSrc<__uint128_t*>(Data->SSAData, Op->Header.Args[0]);
  __uint128_t Src2 = *GetSrc<__uint128_t*>(Data->SSAData, Op->Header.Args[1]);

  uint64_t Offset = Op->DestIdx * Op->Header.ElementSize * 8;
  __uint128_t Mask = (1ULL << (Op->Header.ElementSize * 8)) - 1;
  if (Op->Header.ElementSize == 8) {
    Mask = ~0ULL;
  }
  Src2 = Src2 & Mask;
  Mask <<= Offset;
  Mask = ~Mask;
  __uint128_t Dst = Src1 & Mask;
  Dst |= Src2 << Offset;

  memcpy(GDP, &Dst, OpSize);
}

DEF_OP(VCastFromGPR) {
  auto Op = IROp->C<IR::IROp_VCastFromGPR>();
  memcpy(GDP, GetSrc<void*>(Data->SSAData, Op->Header.Args[0]), Op->Header.ElementSize);
}

DEF_OP(Float_FromGPR_S) {
  auto Op = IROp->C<IR::IROp_Float_FromGPR_S>();

  uint16_t Conv = (Op->Header.ElementSize << 8) | Op->SrcElementSize;
  switch (Conv) {
    case 0x0404: { // Float <- int32_t
      float Dst = (float)*GetSrc<int32_t*>(Data->SSAData, Op->Header.Args[0]);
      memcpy(GDP, &Dst, Op->Header.ElementSize);
      break;
    }
    case 0x0408: { // Float <- int64_t
      float Dst = (float)*GetSrc<int64_t*>(Data->SSAData, Op->Header.Args[0]);
      memcpy(GDP, &Dst, Op->Header.ElementSize);
      break;
    }
    case 0x0804: { // Double <- int32_t
      double Dst = (double)*GetSrc<int32_t*>(Data->SSAData, Op->Header.Args[0]);
      memcpy(GDP, &Dst, Op->Header.ElementSize);
      break;
    }
    case 0x0808: { // Double <- int64_t
      double Dst = (double)*GetSrc<int64_t*>(Data->SSAData, Op->Header.Args[0]);
      memcpy(GDP, &Dst, Op->Header.ElementSize);
      break;
    }
  }
}

DEF_OP(Float_FToF) {
  auto Op = IROp->C<IR::IROp_Float_FToF>();
  uint16_t Conv = (Op->Header.ElementSize << 8) | Op->SrcElementSize;
  switch (Conv) {
    case 0x0804: { // Double <- Float
      double Dst = (double)*GetSrc<float*>(Data->SSAData, Op->Header.Args[0]);
      memcpy(GDP, &Dst, 8);
      break;
    }
    case 0x0408: { // Float <- Double
      float Dst = (float)*GetSrc<double*>(Data->SSAData, Op->Header.Args[0]);
      memcpy(GDP, &Dst, 4);
      break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown FCVT sizes: 0x{:x}", Conv);
  }
}

DEF_OP(Vector_SToF) {
  auto Op = IROp->C<IR::IROp_Vector_SToF>();
  uint8_t OpSize = IROp->Size;

  void *Src = GetSrc<void*>(Data->SSAData, Op->Header.Args[0]);
  uint8_t Tmp[16]{};

  uint8_t Elements = OpSize / Op->Header.ElementSize;

  auto Func = [](auto a, auto min, auto max) { return a; };
  switch (Op->Header.ElementSize) {
    DO_VECTOR_1SRC_2TYPE_OP(4, float, int32_t, Func, 0, 0)
    DO_VECTOR_1SRC_2TYPE_OP(8, double, int64_t, Func, 0, 0)
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(Vector_FToZS) {
  auto Op = IROp->C<IR::IROp_Vector_FToZS>();
  uint8_t OpSize = IROp->Size;

  void *Src = GetSrc<void*>(Data->SSAData, Op->Header.Args[0]);
  uint8_t Tmp[16]{};

  uint8_t Elements = OpSize / Op->Header.ElementSize;

  auto Func = [](auto a, auto min, auto max) { return std::trunc(a); };
  switch (Op->Header.ElementSize) {
    DO_VECTOR_1SRC_2TYPE_OP(4, int32_t, float, Func, 0, 0)
    DO_VECTOR_1SRC_2TYPE_OP(8, int64_t, double, Func, 0, 0)
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(Vector_FToS) {
  auto Op = IROp->C<IR::IROp_Vector_FToS>();
  uint8_t OpSize = IROp->Size;

  void *Src = GetSrc<void*>(Data->SSAData, Op->Header.Args[0]);
  uint8_t Tmp[16]{};

  uint8_t Elements = OpSize / Op->Header.ElementSize;

  auto Func = [](auto a, auto min, auto max) { return std::nearbyint(a); };
  switch (Op->Header.ElementSize) {
    DO_VECTOR_1SRC_2TYPE_OP(4, int32_t, float, Func, 0, 0)
    DO_VECTOR_1SRC_2TYPE_OP(8, int64_t, double, Func, 0, 0)
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(Vector_FToF) {
  auto Op = IROp->C<IR::IROp_Vector_FToF>();
  uint8_t OpSize = IROp->Size;

  void *Src = GetSrc<void*>(Data->SSAData, Op->Header.Args[0]);
  uint8_t Tmp[16]{};

  uint16_t Conv = (Op->Header.ElementSize << 8) | Op->SrcElementSize;

  auto Func = [](auto a, auto min, auto max) { return a; };
  switch (Conv) {
    case 0x0804: { // Double <- float
      // Only the lower elements from the source
      // This uses half the source elements
      uint8_t Elements = OpSize / 8;
      DO_VECTOR_1SRC_2TYPE_OP_NOSIZE(double, float, Func, 0, 0)
      break;
    }
    case 0x0408: { // Float <- Double
      // Little bit tricky here
      // Sometimes is used to convert from a 128bit vector register
      // in to a 64bit vector register with different sized elements
      // eg: %ssa5 i32v2 = Vector_FToF %ssa4 i128, #0x8
      uint8_t Elements = (OpSize << 1) / Op->SrcElementSize;
      DO_VECTOR_1SRC_2TYPE_OP_NOSIZE(float, double, Func, 0, 0)
      break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown Conversion Type : 0x{:04x}", Conv); break;
  }
  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(Vector_FToI) {
  auto Op = IROp->C<IR::IROp_Vector_FToI>();
  uint8_t OpSize = IROp->Size;

  void *Src = GetSrc<void*>(Data->SSAData, Op->Header.Args[0]);
  uint8_t Tmp[16]{};

  uint8_t Elements = OpSize / Op->Header.ElementSize;
  auto Func_Nearest = [](auto a) { return std::rint(a); };
  auto Func_Neg = [](auto a) { return std::floor(a); };
  auto Func_Pos = [](auto a) { return std::ceil(a); };
  auto Func_Trunc = [](auto a) { return std::trunc(a); };
  auto Func_Host = [](auto a) { return std::rint(a); };

  switch (Op->Round) {
    case FEXCore::IR::Round_Nearest.Val:
      switch (Op->Header.ElementSize) {
        DO_VECTOR_1SRC_OP(4, float, Func_Nearest)
        DO_VECTOR_1SRC_OP(8, double, Func_Nearest)
      }
    break;
    case FEXCore::IR::Round_Negative_Infinity.Val:
      switch (Op->Header.ElementSize) {
        DO_VECTOR_1SRC_OP(4, float, Func_Neg)
        DO_VECTOR_1SRC_OP(8, double, Func_Neg)
      }
    break;
    case FEXCore::IR::Round_Positive_Infinity.Val:
      switch (Op->Header.ElementSize) {
        DO_VECTOR_1SRC_OP(4, float, Func_Pos)
        DO_VECTOR_1SRC_OP(8, double, Func_Pos)
      }
    break;
    case FEXCore::IR::Round_Towards_Zero.Val:
      switch (Op->Header.ElementSize) {
        DO_VECTOR_1SRC_OP(4, float, Func_Trunc)
        DO_VECTOR_1SRC_OP(8, double, Func_Trunc)
      }
    break;
    case FEXCore::IR::Round_Host.Val:
      switch (Op->Header.ElementSize) {
        DO_VECTOR_1SRC_OP(4, float, Func_Host)
        DO_VECTOR_1SRC_OP(8, double, Func_Host)
      }
    break;
  }
  memcpy(GDP, Tmp, OpSize);
}

#undef DEF_OP

} // namespace FEXCore::CPU
