/*
$info$
tags: backend|interpreter
$end_info$
*/

#include "Interface/Core/Interpreter/InterpreterClass.h"
#include "Interface/Core/Interpreter/InterpreterOps.h"
#include "Interface/Core/Interpreter/InterpreterDefines.h"

#include "F80Ops.h"

#include <cstdint>

namespace FEXCore::CPU {
#define DEF_OP(x) void InterpreterOps::Op_##x(IR::IROp_Header *IROp, IROpData *Data, IR::NodeID Node)
DEF_OP(F80LOADFCW) {
  FEXCore::CPU::OpHandlers<IR::OP_F80LOADFCW>::handle(*GetSrc<uint16_t*>(Data->SSAData, IROp->Args[0]));
}

DEF_OP(F80ADD) {
  auto Op = IROp->C<IR::IROp_F80Add>();
  X80SoftFloat Src1 = *GetSrc<X80SoftFloat*>(Data->SSAData, Op->Header.Args[0]);
  X80SoftFloat Src2 = *GetSrc<X80SoftFloat*>(Data->SSAData, Op->Header.Args[1]);
  X80SoftFloat Tmp;
  Tmp = X80SoftFloat::FADD(Src1, Src2);

  memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
}

DEF_OP(F80SUB) {
  auto Op = IROp->C<IR::IROp_F80Sub>();
  X80SoftFloat Src1 = *GetSrc<X80SoftFloat*>(Data->SSAData, Op->Header.Args[0]);
  X80SoftFloat Src2 = *GetSrc<X80SoftFloat*>(Data->SSAData, Op->Header.Args[1]);
  X80SoftFloat Tmp;
  Tmp = X80SoftFloat::FSUB(Src1, Src2);

  memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
}

DEF_OP(F80MUL) {
  auto Op = IROp->C<IR::IROp_F80Mul>();
  X80SoftFloat Src1 = *GetSrc<X80SoftFloat*>(Data->SSAData, Op->Header.Args[0]);
  X80SoftFloat Src2 = *GetSrc<X80SoftFloat*>(Data->SSAData, Op->Header.Args[1]);
  X80SoftFloat Tmp;
  Tmp = X80SoftFloat::FMUL(Src1, Src2);

  memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
}

DEF_OP(F80DIV) {
  auto Op = IROp->C<IR::IROp_F80Div>();
  X80SoftFloat Src1 = *GetSrc<X80SoftFloat*>(Data->SSAData, Op->Header.Args[0]);
  X80SoftFloat Src2 = *GetSrc<X80SoftFloat*>(Data->SSAData, Op->Header.Args[1]);
  X80SoftFloat Tmp;
  Tmp = X80SoftFloat::FDIV(Src1, Src2);

  memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
}

DEF_OP(F80FYL2X) {
  auto Op = IROp->C<IR::IROp_F80FYL2X>();
  X80SoftFloat Src1 = *GetSrc<X80SoftFloat*>(Data->SSAData, Op->Header.Args[0]);
  X80SoftFloat Src2 = *GetSrc<X80SoftFloat*>(Data->SSAData, Op->Header.Args[1]);
  X80SoftFloat Tmp;
  Tmp = X80SoftFloat::FYL2X(Src1, Src2);

  memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
}

DEF_OP(F80ATAN) {
  auto Op = IROp->C<IR::IROp_F80ATAN>();
  X80SoftFloat Src1 = *GetSrc<X80SoftFloat*>(Data->SSAData, Op->Header.Args[0]);
  X80SoftFloat Src2 = *GetSrc<X80SoftFloat*>(Data->SSAData, Op->Header.Args[1]);
  X80SoftFloat Tmp;
  Tmp = X80SoftFloat::FATAN(Src1, Src2);

  memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
}

DEF_OP(F80FPREM1) {
  auto Op = IROp->C<IR::IROp_F80FPREM1>();
  X80SoftFloat Src1 = *GetSrc<X80SoftFloat*>(Data->SSAData, Op->Header.Args[0]);
  X80SoftFloat Src2 = *GetSrc<X80SoftFloat*>(Data->SSAData, Op->Header.Args[1]);
  X80SoftFloat Tmp;
  Tmp = X80SoftFloat::FREM1(Src1, Src2);

  memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
}

DEF_OP(F80FPREM) {
  auto Op = IROp->C<IR::IROp_F80FPREM>();
  X80SoftFloat Src1 = *GetSrc<X80SoftFloat*>(Data->SSAData, Op->Header.Args[0]);
  X80SoftFloat Src2 = *GetSrc<X80SoftFloat*>(Data->SSAData, Op->Header.Args[1]);
  X80SoftFloat Tmp;
  Tmp = X80SoftFloat::FREM(Src1, Src2);

  memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
}

DEF_OP(F80SCALE) {
  auto Op = IROp->C<IR::IROp_F80SCALE>();
  X80SoftFloat Src1 = *GetSrc<X80SoftFloat*>(Data->SSAData, Op->Header.Args[0]);
  X80SoftFloat Src2 = *GetSrc<X80SoftFloat*>(Data->SSAData, Op->Header.Args[1]);
  X80SoftFloat Tmp;
  Tmp = X80SoftFloat::FSCALE(Src1, Src2);

  memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
}

DEF_OP(F80CVT) {
  auto Op = IROp->C<IR::IROp_F80CVT>();
  uint8_t OpSize = IROp->Size;

  X80SoftFloat Src = *GetSrc<X80SoftFloat*>(Data->SSAData, Op->Header.Args[0]);

  switch (OpSize) {
    case 4: {
      float Tmp = Src;
      memcpy(GDP, &Tmp, OpSize);
      break;
    }
    case 8: {
      double Tmp = Src;
      memcpy(GDP, &Tmp, OpSize);
      break;
    }
    default: LogMan::Msg::DFmt("Unhandled size: {}", OpSize);
  }
}

DEF_OP(F80CVTINT) {
  auto Op = IROp->C<IR::IROp_F80CVTInt>();
  uint8_t OpSize = IROp->Size;

  X80SoftFloat Src = *GetSrc<X80SoftFloat*>(Data->SSAData, Op->Header.Args[0]);

  switch (OpSize) {
    case 2: {
      int16_t Tmp = (Op->Truncate? FEXCore::CPU::OpHandlers<IR::OP_F80CVTINT>::handle2t : FEXCore::CPU::OpHandlers<IR::OP_F80CVTINT>::handle2)(Src);
      memcpy(GDP, &Tmp, sizeof(Tmp));
      break;
    }
    case 4: {
      int32_t Tmp = (Op->Truncate? FEXCore::CPU::OpHandlers<IR::OP_F80CVTINT>::handle4t : FEXCore::CPU::OpHandlers<IR::OP_F80CVTINT>::handle4)(Src);
      memcpy(GDP, &Tmp, sizeof(Tmp));
      break;
    }
    case 8: {
      int64_t Tmp = (Op->Truncate? FEXCore::CPU::OpHandlers<IR::OP_F80CVTINT>::handle8t : FEXCore::CPU::OpHandlers<IR::OP_F80CVTINT>::handle8)(Src);
      memcpy(GDP, &Tmp, sizeof(Tmp));
      break;
    }
    default: LogMan::Msg::DFmt("Unhandled size: {}", OpSize);
  }
}

DEF_OP(F80CVTTO) {
  auto Op = IROp->C<IR::IROp_F80CVTTo>();

  switch (Op->Size) {
    case 4: {
      float Src = *GetSrc<float *>(Data->SSAData, Op->Header.Args[0]);
      X80SoftFloat Tmp = Src;
      memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
      break;
    }
    case 8: {
      double Src = *GetSrc<double *>(Data->SSAData, Op->Header.Args[0]);
      X80SoftFloat Tmp = Src;
      memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
      break;
    }
    default: LogMan::Msg::DFmt("Unhandled size: {}", Op->Size);
  }
}

DEF_OP(F80CVTTOINT) {
  auto Op = IROp->C<IR::IROp_F80CVTToInt>();

  switch (Op->Size) {
    case 2: {
      int16_t Src = *GetSrc<int16_t*>(Data->SSAData, Op->Header.Args[0]);
      X80SoftFloat Tmp = Src;
      memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
      break;
    }
    case 4: {
      int32_t Src = *GetSrc<int32_t*>(Data->SSAData, Op->Header.Args[0]);
      X80SoftFloat Tmp = Src;
      memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
      break;
    }
    default: LogMan::Msg::DFmt("Unhandled size: {}", Op->Size);
  }
}

DEF_OP(F80ROUND) {
  auto Op = IROp->C<IR::IROp_F80Round>();
  X80SoftFloat Src = *GetSrc<X80SoftFloat*>(Data->SSAData, Op->Header.Args[0]);
  X80SoftFloat Tmp;
  Tmp = X80SoftFloat::FRNDINT(Src);

  memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
}

DEF_OP(F80F2XM1) {
  auto Op = IROp->C<IR::IROp_F80F2XM1>();
  X80SoftFloat Src = *GetSrc<X80SoftFloat*>(Data->SSAData, Op->Header.Args[0]);
  X80SoftFloat Tmp;
  Tmp = X80SoftFloat::F2XM1(Src);

  memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
}

DEF_OP(F80TAN) {
  auto Op = IROp->C<IR::IROp_F80TAN>();
  X80SoftFloat Src = *GetSrc<X80SoftFloat*>(Data->SSAData, Op->Header.Args[0]);
  X80SoftFloat Tmp;
  Tmp = X80SoftFloat::FTAN(Src);

  memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
}

DEF_OP(F80SQRT) {
  auto Op = IROp->C<IR::IROp_F80SQRT>();
  X80SoftFloat Src = *GetSrc<X80SoftFloat*>(Data->SSAData, Op->Header.Args[0]);
  X80SoftFloat Tmp;
  Tmp = X80SoftFloat::FSQRT(Src);

  memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
}

DEF_OP(F80SIN) {
  auto Op = IROp->C<IR::IROp_F80SIN>();
  X80SoftFloat Src = *GetSrc<X80SoftFloat*>(Data->SSAData, Op->Header.Args[0]);
  X80SoftFloat Tmp;
  Tmp = X80SoftFloat::FSIN(Src);
  memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
}

DEF_OP(F80COS) {
  auto Op = IROp->C<IR::IROp_F80COS>();
  X80SoftFloat Src = *GetSrc<X80SoftFloat*>(Data->SSAData, Op->Header.Args[0]);
  X80SoftFloat Tmp;
  Tmp = X80SoftFloat::FCOS(Src);
  memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
}

DEF_OP(F80XTRACT_EXP) {
  auto Op = IROp->C<IR::IROp_F80XTRACT_EXP>();
  X80SoftFloat Src = *GetSrc<X80SoftFloat*>(Data->SSAData, Op->Header.Args[0]);
  X80SoftFloat Tmp;
  Tmp = X80SoftFloat::FXTRACT_EXP(Src);
  memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
}

DEF_OP(F80XTRACT_SIG) {
  auto Op = IROp->C<IR::IROp_F80XTRACT_SIG>();
  X80SoftFloat Src = *GetSrc<X80SoftFloat*>(Data->SSAData, Op->Header.Args[0]);
  X80SoftFloat Tmp;
  Tmp = X80SoftFloat::FXTRACT_SIG(Src);
  memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
}

DEF_OP(F80CMP) {
  auto Op = IROp->C<IR::IROp_F80Cmp>();
  uint32_t ResultFlags{};
  X80SoftFloat Src1 = *GetSrc<X80SoftFloat*>(Data->SSAData, Op->Header.Args[0]);
  X80SoftFloat Src2 = *GetSrc<X80SoftFloat*>(Data->SSAData, Op->Header.Args[1]);
  bool eq, lt, nan;
  X80SoftFloat::FCMP(Src1, Src2, &eq, &lt, &nan);
  if (Op->Flags & (1 << IR::FCMP_FLAG_LT) &&
      lt) {
    ResultFlags |= (1 << IR::FCMP_FLAG_LT);
  }
  if (Op->Flags & (1 << IR::FCMP_FLAG_UNORDERED) &&
      nan) {
    ResultFlags |= (1 << IR::FCMP_FLAG_UNORDERED);
  }
  if (Op->Flags & (1 << IR::FCMP_FLAG_EQ) &&
      eq) {
    ResultFlags |= (1 << IR::FCMP_FLAG_EQ);
  }

  GD = ResultFlags;
}

DEF_OP(F80BCDLOAD) {
  auto Op = IROp->C<IR::IROp_F80BCDLoad>();
  uint8_t *Src1 = GetSrc<uint8_t*>(Data->SSAData, Op->Header.Args[0]);
  uint64_t BCD{};
  // We walk through each uint8_t and pull out the BCD encoding
  // Each 4bit split is a digit
  // Only 0-9 is supported, A-F results in undefined data
  // | 4 bit     | 4 bit    |
  // | 10s place | 1s place |
  // EG 0x48 = 48
  // EG 0x4847 = 4847
  // This gives us an 18digit value encoded in BCD
  // The last byte lets us know if it negative or not
  for (size_t i = 0; i < 9; ++i) {
    uint8_t Digit = Src1[8 - i];
    // First shift our last value over
    BCD *= 100;

    // Add the tens place digit
    BCD += (Digit >> 4) * 10;

    // Add the ones place digit
    BCD += Digit & 0xF;
  }

  // Set negative flag once converted to x87
  bool Negative = Src1[9] & 0x80;
  X80SoftFloat Tmp;

  Tmp = BCD;
  Tmp.Sign = Negative;

  memcpy(GDP, &Tmp, sizeof(X80SoftFloat));
}

DEF_OP(F80BCDSTORE) {
  auto Op = IROp->C<IR::IROp_F80BCDStore>();
  X80SoftFloat Src1 = *GetSrc<X80SoftFloat*>(Data->SSAData, Op->Header.Args[0]);
  bool Negative = Src1.Sign;

  // Clear the Sign bit
  Src1.Sign = 0;

  uint64_t Tmp = Src1;
  uint8_t BCD[10]{};

  for (size_t i = 0; i < 9; ++i) {
    if (Tmp == 0) {
      // Nothing left? Just leave
      break;
    }
    // Extract the lower 100 values
    uint8_t Digit = Tmp % 100;

    // Now divide it for the next iteration
    Tmp /= 100;

    uint8_t UpperNibble = Digit / 10;
    uint8_t LowerNibble = Digit % 10;

    // Now store the BCD
    BCD[i] = (UpperNibble << 4) | LowerNibble;
  }

  // Set negative flag once converted to x87
  BCD[9] = Negative ? 0x80 : 0;

  memcpy(GDP, BCD, 10);
}

#undef DEF_OP
void InterpreterOps::RegisterF80Handlers() {
#define REGISTER_OP(op, x) OpHandlers[FEXCore::IR::IROps::OP_##op] = &InterpreterOps::Op_##x
  REGISTER_OP(F80LOADFCW, F80LOADFCW);
  REGISTER_OP(F80ADD, F80ADD);
  REGISTER_OP(F80SUB, F80SUB);
  REGISTER_OP(F80MUL, F80MUL);
  REGISTER_OP(F80DIV, F80DIV);
  REGISTER_OP(F80FYL2X, F80FYL2X);
  REGISTER_OP(F80ATAN, F80ATAN);
  REGISTER_OP(F80FPREM1, F80FPREM1);
  REGISTER_OP(F80FPREM, F80FPREM);
  REGISTER_OP(F80SCALE, F80SCALE);
  REGISTER_OP(F80CVT, F80CVT);
  REGISTER_OP(F80CVTINT, F80CVTINT);
  REGISTER_OP(F80CVTTO, F80CVTTO);
  REGISTER_OP(F80CVTTOINT, F80CVTTOINT);
  REGISTER_OP(F80ROUND, F80ROUND);
  REGISTER_OP(F80F2XM1, F80F2XM1);
  REGISTER_OP(F80TAN, F80TAN);
  REGISTER_OP(F80SQRT, F80SQRT);
  REGISTER_OP(F80SIN, F80SIN);
  REGISTER_OP(F80COS, F80COS);
  REGISTER_OP(F80XTRACT_EXP, F80XTRACT_EXP);
  REGISTER_OP(F80XTRACT_SIG, F80XTRACT_SIG);
  REGISTER_OP(F80CMP, F80CMP);
  REGISTER_OP(F80BCDLOAD, F80BCDLOAD);
  REGISTER_OP(F80BCDSTORE, F80BCDSTORE);
#undef REGISTER_OP
}
}
