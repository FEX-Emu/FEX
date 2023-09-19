// SPDX-License-Identifier: MIT
/*
$info$
tags: backend|interpreter
$end_info$
*/

#include "Interface/Core/Interpreter/InterpreterClass.h"
#include "Interface/Core/Interpreter/InterpreterOps.h"
#include "Interface/Core/Interpreter/InterpreterDefines.h"

#include "Interface/Core/Interpreter/Fallbacks/F80Fallbacks.h"

#include <cstdint>

namespace FEXCore::CPU {
#define DEF_OP(x) void InterpreterOps::Op_##x(IR::IROp_Header *IROp, IROpData *Data, IR::NodeID Node)
DEF_OP(F80ADD) {
  auto Op = IROp->C<IR::IROp_F80Add>();
  const auto Src1 = *GetSrc<X80SoftFloat*>(Data->SSAData, Op->X80Src1);
  const auto Src2 = *GetSrc<X80SoftFloat*>(Data->SSAData, Op->X80Src2);
  const auto Tmp = CPU::OpHandlers<IR::OP_F80ADD>::handle(Data->State->CurrentFrame->State.FCW, Src1, Src2);

  memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
}

DEF_OP(F80SUB) {
  auto Op = IROp->C<IR::IROp_F80Sub>();
  const auto Src1 = *GetSrc<X80SoftFloat*>(Data->SSAData, Op->X80Src1);
  const auto Src2 = *GetSrc<X80SoftFloat*>(Data->SSAData, Op->X80Src2);
  const auto Tmp = CPU::OpHandlers<IR::OP_F80SUB>::handle(Data->State->CurrentFrame->State.FCW, Src1, Src2);

  memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
}

DEF_OP(F80MUL) {
  auto Op = IROp->C<IR::IROp_F80Mul>();
  const auto Src1 = *GetSrc<X80SoftFloat*>(Data->SSAData, Op->X80Src1);
  const auto Src2 = *GetSrc<X80SoftFloat*>(Data->SSAData, Op->X80Src2);
  const auto Tmp = CPU::OpHandlers<IR::OP_F80MUL>::handle(Data->State->CurrentFrame->State.FCW, Src1, Src2);

  memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
}

DEF_OP(F80DIV) {
  auto Op = IROp->C<IR::IROp_F80Div>();
  const auto Src1 = *GetSrc<X80SoftFloat*>(Data->SSAData, Op->X80Src1);
  const auto Src2 = *GetSrc<X80SoftFloat*>(Data->SSAData, Op->X80Src2);
  const auto Tmp = CPU::OpHandlers<IR::OP_F80DIV>::handle(Data->State->CurrentFrame->State.FCW, Src1, Src2);

  memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
}

DEF_OP(F80FYL2X) {
  auto Op = IROp->C<IR::IROp_F80FYL2X>();
  const auto Src1 = *GetSrc<X80SoftFloat*>(Data->SSAData, Op->X80Src1);
  const auto Src2 = *GetSrc<X80SoftFloat*>(Data->SSAData, Op->X80Src2);
  const auto Tmp = CPU::OpHandlers<IR::OP_F80FYL2X>::handle(Data->State->CurrentFrame->State.FCW, Src1, Src2);

  memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
}

DEF_OP(F80ATAN) {
  auto Op = IROp->C<IR::IROp_F80ATAN>();
  const auto Src1 = *GetSrc<X80SoftFloat*>(Data->SSAData, Op->X80Src1);
  const auto Src2 = *GetSrc<X80SoftFloat*>(Data->SSAData, Op->X80Src2);
  const auto Tmp = CPU::OpHandlers<IR::OP_F80ATAN>::handle(Data->State->CurrentFrame->State.FCW, Src1, Src2);

  memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
}

DEF_OP(F80FPREM1) {
  auto Op = IROp->C<IR::IROp_F80FPREM1>();
  const auto Src1 = *GetSrc<X80SoftFloat*>(Data->SSAData, Op->X80Src1);
  const auto Src2 = *GetSrc<X80SoftFloat*>(Data->SSAData, Op->X80Src2);
  const auto Tmp = CPU::OpHandlers<IR::OP_F80FPREM1>::handle(Data->State->CurrentFrame->State.FCW, Src1, Src2);

  memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
}

DEF_OP(F80FPREM) {
  auto Op = IROp->C<IR::IROp_F80FPREM>();
  const auto Src1 = *GetSrc<X80SoftFloat*>(Data->SSAData, Op->X80Src1);
  const auto Src2 = *GetSrc<X80SoftFloat*>(Data->SSAData, Op->X80Src2);
  const auto Tmp = CPU::OpHandlers<IR::OP_F80FPREM>::handle(Data->State->CurrentFrame->State.FCW, Src1, Src2);

  memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
}

DEF_OP(F80SCALE) {
  auto Op = IROp->C<IR::IROp_F80SCALE>();
  const auto Src1 = *GetSrc<X80SoftFloat*>(Data->SSAData, Op->X80Src1);
  const auto Src2 = *GetSrc<X80SoftFloat*>(Data->SSAData, Op->X80Src2);
  const auto Tmp = CPU::OpHandlers<IR::OP_F80SCALE>::handle(Data->State->CurrentFrame->State.FCW, Src1, Src2);

  memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
}

DEF_OP(F80CVT) {
  auto Op = IROp->C<IR::IROp_F80CVT>();
  const uint8_t OpSize = IROp->Size;

  const auto Src = *GetSrc<X80SoftFloat*>(Data->SSAData, Op->X80Src);

  switch (OpSize) {
    case 4: {
      const auto Tmp = CPU::OpHandlers<IR::OP_F80CVT>::handle4(Data->State->CurrentFrame->State.FCW, Src);
      memcpy(GDP, &Tmp, OpSize);
      break;
    }
    case 8: {
      const auto Tmp = CPU::OpHandlers<IR::OP_F80CVT>::handle8(Data->State->CurrentFrame->State.FCW, Src);
      memcpy(GDP, &Tmp, OpSize);
      break;
    }
    default: LogMan::Msg::DFmt("Unhandled size: {}", OpSize);
  }
}

DEF_OP(F80CVTINT) {
  auto Op = IROp->C<IR::IROp_F80CVTInt>();
  const uint8_t OpSize = IROp->Size;

  const auto Src = *GetSrc<X80SoftFloat*>(Data->SSAData, Op->X80Src);

  switch (OpSize) {
    case 2: {
      int16_t Tmp = (Op->Truncate? FEXCore::CPU::OpHandlers<IR::OP_F80CVTINT>::handle2t : FEXCore::CPU::OpHandlers<IR::OP_F80CVTINT>::handle2)(Data->State->CurrentFrame->State.FCW, Src);
      memcpy(GDP, &Tmp, sizeof(Tmp));
      break;
    }
    case 4: {
      int32_t Tmp = (Op->Truncate? FEXCore::CPU::OpHandlers<IR::OP_F80CVTINT>::handle4t : FEXCore::CPU::OpHandlers<IR::OP_F80CVTINT>::handle4)(Data->State->CurrentFrame->State.FCW, Src);
      memcpy(GDP, &Tmp, sizeof(Tmp));
      break;
    }
    case 8: {
      int64_t Tmp = (Op->Truncate? FEXCore::CPU::OpHandlers<IR::OP_F80CVTINT>::handle8t : FEXCore::CPU::OpHandlers<IR::OP_F80CVTINT>::handle8)(Data->State->CurrentFrame->State.FCW, Src);
      memcpy(GDP, &Tmp, sizeof(Tmp));
      break;
    }
    default: LogMan::Msg::DFmt("Unhandled size: {}", OpSize);
  }
}

DEF_OP(F80CVTTO) {
  auto Op = IROp->C<IR::IROp_F80CVTTo>();

  switch (Op->SrcSize) {
    case 4: {
      float Src = *GetSrc<float *>(Data->SSAData, Op->X80Src);
      const auto Tmp = CPU::OpHandlers<IR::OP_F80CVTTO>::handle4(Data->State->CurrentFrame->State.FCW, Src);
      memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
      break;
    }
    case 8: {
      double Src = *GetSrc<double *>(Data->SSAData, Op->X80Src);
      const auto Tmp = CPU::OpHandlers<IR::OP_F80CVTTO>::handle8(Data->State->CurrentFrame->State.FCW, Src);
      memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
      break;
    }
    default: LogMan::Msg::DFmt("Unhandled size: {}", Op->SrcSize);
  }
}

DEF_OP(F80CVTTOINT) {
  auto Op = IROp->C<IR::IROp_F80CVTToInt>();

  switch (Op->SrcSize) {
    case 2: {
      int16_t Src = *GetSrc<int16_t*>(Data->SSAData, Op->Src);
      const auto Tmp = CPU::OpHandlers<IR::OP_F80CVTTOINT>::handle2(Data->State->CurrentFrame->State.FCW, Src);
      memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
      break;
    }
    case 4: {
      int32_t Src = *GetSrc<int32_t*>(Data->SSAData, Op->Src);
      const auto Tmp = CPU::OpHandlers<IR::OP_F80CVTTOINT>::handle4(Data->State->CurrentFrame->State.FCW, Src);
      memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
      break;
    }
    default: LogMan::Msg::DFmt("Unhandled size: {}", Op->SrcSize);
  }
}

DEF_OP(F80ROUND) {
  auto Op = IROp->C<IR::IROp_F80Round>();
  const auto Src = *GetSrc<X80SoftFloat*>(Data->SSAData, Op->X80Src);
  const auto Tmp = CPU::OpHandlers<IR::OP_F80ROUND>::handle(Data->State->CurrentFrame->State.FCW, Src);

  memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
}

DEF_OP(F80F2XM1) {
  auto Op = IROp->C<IR::IROp_F80F2XM1>();
  const auto Src = *GetSrc<X80SoftFloat*>(Data->SSAData, Op->X80Src);
  const auto Tmp = CPU::OpHandlers<IR::OP_F80F2XM1>::handle(Data->State->CurrentFrame->State.FCW, Src);

  memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
}

DEF_OP(F80TAN) {
  auto Op = IROp->C<IR::IROp_F80TAN>();
  const auto Src = *GetSrc<X80SoftFloat*>(Data->SSAData, Op->X80Src);
  const auto Tmp = CPU::OpHandlers<IR::OP_F80TAN>::handle(Data->State->CurrentFrame->State.FCW, Src);

  memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
}

DEF_OP(F80SQRT) {
  auto Op = IROp->C<IR::IROp_F80SQRT>();
  const auto Src = *GetSrc<X80SoftFloat*>(Data->SSAData, Op->X80Src);
  const auto Tmp = CPU::OpHandlers<IR::OP_F80SQRT>::handle(Data->State->CurrentFrame->State.FCW, Src);

  memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
}

DEF_OP(F80SIN) {
  auto Op = IROp->C<IR::IROp_F80SIN>();
  const auto Src = *GetSrc<X80SoftFloat*>(Data->SSAData, Op->X80Src);
  const auto Tmp = CPU::OpHandlers<IR::OP_F80SIN>::handle(Data->State->CurrentFrame->State.FCW, Src);

  memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
}

DEF_OP(F80COS) {
  auto Op = IROp->C<IR::IROp_F80COS>();
  const auto Src = *GetSrc<X80SoftFloat*>(Data->SSAData, Op->X80Src);
  const auto Tmp = CPU::OpHandlers<IR::OP_F80COS>::handle(Data->State->CurrentFrame->State.FCW, Src);

  memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
}

DEF_OP(F80XTRACT_EXP) {
  auto Op = IROp->C<IR::IROp_F80XTRACT_EXP>();
  const auto Src = *GetSrc<X80SoftFloat*>(Data->SSAData, Op->X80Src);
  const auto Tmp = CPU::OpHandlers<IR::OP_F80XTRACT_EXP>::handle(Data->State->CurrentFrame->State.FCW, Src);

  memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
}

DEF_OP(F80XTRACT_SIG) {
  auto Op = IROp->C<IR::IROp_F80XTRACT_SIG>();
  const auto Src = *GetSrc<X80SoftFloat*>(Data->SSAData, Op->X80Src);
  const auto Tmp = CPU::OpHandlers<IR::OP_F80XTRACT_SIG>::handle(Data->State->CurrentFrame->State.FCW, Src);

  memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
}

DEF_OP(F80CMP) {
  auto Op = IROp->C<IR::IROp_F80Cmp>();
  const auto Src1 = *GetSrc<X80SoftFloat*>(Data->SSAData, Op->X80Src1);
  const auto Src2 = *GetSrc<X80SoftFloat*>(Data->SSAData, Op->X80Src2);
  const auto ResultFlags = CPU::OpHandlers<IR::OP_F80CMP>::handle<IR::FCMP_FLAG_LT | IR::FCMP_FLAG_UNORDERED | IR::FCMP_FLAG_EQ>(Data->State->CurrentFrame->State.FCW, Src1, Src2);

  GD = ResultFlags;
}

DEF_OP(F80BCDLOAD) {
  auto Op = IROp->C<IR::IROp_F80BCDLoad>();
  const auto Src = *GetSrc<X80SoftFloat*>(Data->SSAData, Op->X80Src);
  const auto Tmp = CPU::OpHandlers<IR::OP_F80BCDLOAD>::handle(Data->State->CurrentFrame->State.FCW, Src);
  memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
}

DEF_OP(F80BCDSTORE) {
  auto Op = IROp->C<IR::IROp_F80BCDStore>();
  const auto Src = *GetSrc<X80SoftFloat*>(Data->SSAData, Op->X80Src);
  const auto Tmp = CPU::OpHandlers<IR::OP_F80BCDSTORE>::handle(Data->State->CurrentFrame->State.FCW, Src);
  memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
}

DEF_OP(F64SIN) {
  auto Op = IROp->C<IR::IROp_F64SIN>();
  const double Src = *GetSrc<double*>(Data->SSAData, Op->Src);
  const double Tmp = sin(Src);
  memcpy(GDP, &Tmp, sizeof(double));
}

DEF_OP(F64COS) {
  auto Op = IROp->C<IR::IROp_F64COS>();
  const double Src = *GetSrc<double*>(Data->SSAData, Op->Src);
  const double Tmp = cos(Src);
  memcpy(GDP, &Tmp, sizeof(double));
}

DEF_OP(F64TAN) {
  auto Op = IROp->C<IR::IROp_F64TAN>();
  const double Src = *GetSrc<double*>(Data->SSAData, Op->Src);
  const double Tmp = tan(Src);
  memcpy(GDP, &Tmp, sizeof(double));
}

DEF_OP(F64F2XM1) {
  auto Op = IROp->C<IR::IROp_F64F2XM1>();
  const double Src = *GetSrc<double*>(Data->SSAData, Op->Src);
  const double Tmp = exp2(Src) - 1.0;
  memcpy(GDP, &Tmp, sizeof(double));
}

DEF_OP(F64ATAN) {
  auto Op = IROp->C<IR::IROp_F64ATAN>();
  const double Src1 = *GetSrc<double*>(Data->SSAData, Op->Src1);
  const double Src2 = *GetSrc<double*>(Data->SSAData, Op->Src2);
  const double Tmp = atan2(Src1, Src2);

  memcpy(GDP, &Tmp, sizeof(double));
}

DEF_OP(F64FPREM) {
  auto Op = IROp->C<IR::IROp_F64FPREM>();
  const double Src1 = *GetSrc<double*>(Data->SSAData, Op->Src1);
  const double Src2 = *GetSrc<double*>(Data->SSAData, Op->Src2);
  const double Tmp = fmod(Src1, Src2);

  memcpy(GDP, &Tmp, sizeof(double));
}

DEF_OP(F64FPREM1) {
  auto Op = IROp->C<IR::IROp_F64FPREM1>();
  const double Src1 = *GetSrc<double*>(Data->SSAData, Op->Src1);
  const double Src2 = *GetSrc<double*>(Data->SSAData, Op->Src2);
  const double Tmp = remainder(Src1, Src2);

  memcpy(GDP, &Tmp, sizeof(double));
}

DEF_OP(F64FYL2X) {
  auto Op = IROp->C<IR::IROp_F64FYL2X>();
  const double Src1 = *GetSrc<double*>(Data->SSAData, Op->Src);
  const double Src2 = *GetSrc<double*>(Data->SSAData, Op->Src2);
  const double Tmp = Src2 * log2(Src1);

  memcpy(GDP, &Tmp, sizeof(double));
}

DEF_OP(F64SCALE) {
  auto Op = IROp->C<IR::IROp_F64SCALE>();
  const double Src1 = *GetSrc<double*>(Data->SSAData, Op->Src1);
  const double Src2 = *GetSrc<double*>(Data->SSAData, Op->Src2);
  const double trunc = (double)(int64_t)(Src2); //truncate
  const double Tmp = Src1 * exp2(trunc);

  memcpy(GDP, &Tmp, sizeof(double));
}

#undef DEF_OP

} // namespace FEXCore::CPU
