#pragma once
#include "Common/SoftFloat.h"
#include "Common/SoftFloat-3e/softfloat.h"

#include <FEXCore/IR/IR.h>

#include "Interface/Core/Interpreter/Fallbacks/FallbackOpHandler.h"

namespace FEXCore::CPU {
template<>
struct OpHandlers<IR::OP_F80CVTTO> {
  static X80SoftFloat handle4(float src) {
    return src;
  }

  static X80SoftFloat handle8(double src) {
    return src;
  }
};

template<>
struct OpHandlers<IR::OP_F80CMP> {
  template<uint32_t Flags>
  static uint64_t handle(X80SoftFloat Src1, X80SoftFloat Src2) {
    bool eq, lt, nan;
    uint64_t ResultFlags = 0;

    X80SoftFloat::FCMP(Src1, Src2, &eq, &lt, &nan);
    if (Flags & (1 << IR::FCMP_FLAG_LT) &&
        lt) {
      ResultFlags |= (1 << IR::FCMP_FLAG_LT);
    }
    if (Flags & (1 << IR::FCMP_FLAG_UNORDERED) &&
        nan) {
      ResultFlags |= (1 << IR::FCMP_FLAG_UNORDERED);
    }
    if (Flags & (1 << IR::FCMP_FLAG_EQ) &&
        eq) {
      ResultFlags |= (1 << IR::FCMP_FLAG_EQ);
    }
    return ResultFlags;
  }
};

template<>
struct OpHandlers<IR::OP_F80CVT> {
  static float handle4(X80SoftFloat src) {
    return src;
  }

  static double handle8(X80SoftFloat src) {
    return src;
  }
};

template<>
struct OpHandlers<IR::OP_F80CVTINT> {
  static  int16_t handle2(X80SoftFloat src) {
    return src;
  }

  static int32_t handle4(X80SoftFloat src) {
    return src;
  }

  static int64_t handle8(X80SoftFloat src) {
    return src;
  }

  static  int16_t handle2t(X80SoftFloat src) {
    auto rv = extF80_to_i32(src, softfloat_round_minMag, false);

    if (rv > INT16_MAX) {
      return INT16_MAX;
    } else if (rv < INT16_MIN) {
      return INT16_MIN;
    } else {
      return rv;
    }
  }

  static int32_t handle4t(X80SoftFloat src) {
    return extF80_to_i32(src, softfloat_round_minMag, false);
  }

  static int64_t handle8t(X80SoftFloat src) {
    return extF80_to_i64(src, softfloat_round_minMag, false);
  }
};

template<>
struct OpHandlers<IR::OP_F80CVTTOINT> {
  static X80SoftFloat handle2(int16_t src) {
    return src;
  }

  static X80SoftFloat handle4(int32_t src) {
    return src;
  }
};

template<>
struct OpHandlers<IR::OP_F80ROUND> {
  static X80SoftFloat handle(X80SoftFloat Src1) {
    return X80SoftFloat::FRNDINT(Src1);
  }
};

template<>
struct OpHandlers<IR::OP_F80F2XM1> {
  static X80SoftFloat handle(X80SoftFloat Src1) {
    return X80SoftFloat::F2XM1(Src1);
  }
};

template<>
struct OpHandlers<IR::OP_F80TAN> {
  static X80SoftFloat handle(X80SoftFloat Src1) {
    return X80SoftFloat::FTAN(Src1);
  }
};

template<>
struct OpHandlers<IR::OP_F80SQRT> {
  static X80SoftFloat handle(X80SoftFloat Src1) {
    return X80SoftFloat::FSQRT(Src1);
  }
};

template<>
struct OpHandlers<IR::OP_F80SIN> {
  static X80SoftFloat handle(X80SoftFloat Src1) {
    return X80SoftFloat::FSIN(Src1);
  }
};

template<>
struct OpHandlers<IR::OP_F80COS> {
  static X80SoftFloat handle(X80SoftFloat Src1) {
    return X80SoftFloat::FCOS(Src1);
  }
};

template<>
struct OpHandlers<IR::OP_F80XTRACT_EXP> {
  static X80SoftFloat handle(X80SoftFloat Src1) {
    return X80SoftFloat::FXTRACT_EXP(Src1);
  }
};

template<>
struct OpHandlers<IR::OP_F80XTRACT_SIG> {
  static X80SoftFloat handle(X80SoftFloat Src1) {
    return X80SoftFloat::FXTRACT_SIG(Src1);
  }
};

template<>
struct OpHandlers<IR::OP_F80ADD> {
  static X80SoftFloat handle(X80SoftFloat Src1, X80SoftFloat Src2) {
    return X80SoftFloat::FADD(Src1, Src2);
  }
};

template<>
struct OpHandlers<IR::OP_F80SUB> {
  static X80SoftFloat handle(X80SoftFloat Src1, X80SoftFloat Src2) {
    return X80SoftFloat::FSUB(Src1, Src2);
  }
};

template<>
struct OpHandlers<IR::OP_F80MUL> {
  static X80SoftFloat handle(X80SoftFloat Src1, X80SoftFloat Src2) {
    return X80SoftFloat::FMUL(Src1, Src2);
  }
};

template<>
struct OpHandlers<IR::OP_F80DIV> {
  static X80SoftFloat handle(X80SoftFloat Src1, X80SoftFloat Src2) {
    return X80SoftFloat::FDIV(Src1, Src2);
  }
};


template<>
struct OpHandlers<IR::OP_F80FYL2X> {
  static X80SoftFloat handle(X80SoftFloat Src1, X80SoftFloat Src2) {
    return X80SoftFloat::FYL2X(Src1, Src2);
  }
};

template<>
struct OpHandlers<IR::OP_F80ATAN> {
  static X80SoftFloat handle(X80SoftFloat Src1, X80SoftFloat Src2) {
    return X80SoftFloat::FATAN(Src1, Src2);
  }
};

template<>
struct OpHandlers<IR::OP_F80FPREM1> {
  static X80SoftFloat handle(X80SoftFloat Src1, X80SoftFloat Src2) {
    return X80SoftFloat::FREM1(Src1, Src2);
  }
};

template<>
struct OpHandlers<IR::OP_F80FPREM> {
  static X80SoftFloat handle(X80SoftFloat Src1, X80SoftFloat Src2) {
    return X80SoftFloat::FREM(Src1, Src2);
  }
};

template<>
struct OpHandlers<IR::OP_F80SCALE> {
  static X80SoftFloat handle(X80SoftFloat Src1, X80SoftFloat Src2) {
    return X80SoftFloat::FSCALE(Src1, Src2);
  }
};

template<>
struct OpHandlers<IR::OP_F64SIN> {
  static double handle(double src) {
    return sin(src);
  }
};

template<>
struct OpHandlers<IR::OP_F64COS> {
  static double handle(double src) {
    return cos(src);
  }
};

template<>
struct OpHandlers<IR::OP_F64TAN> {
  static double handle(double src) {
    return tan(src);
  }
};

template<>
struct OpHandlers<IR::OP_F64F2XM1> {
  static double handle(double src) {
    return exp2(src) - 1.0;
  }
};

template<>
struct OpHandlers<IR::OP_F64ATAN> {
  static double handle(double src1, double src2) {
    return atan2(src1, src2);
  }
};

template<>
struct OpHandlers<IR::OP_F64FPREM> {
  static double handle(double src1, double src2) {
    return fmod(src1, src2);
  }
};

template<>
struct OpHandlers<IR::OP_F64FPREM1> {
  static double handle(double src1, double src2) {
    return remainder(src1, src2);
  }
};


template<>
struct OpHandlers<IR::OP_F64FYL2X> {
  static double handle(double src1, double src2) {
    return src2 * log2(src1);
  }
};

template<>
struct OpHandlers<IR::OP_F64SCALE> {
  static double handle(double src1, double src2) {
    double trunc = (double)(int64_t)(src2); //truncate
    return src1 * exp2(trunc);
  }
};



template<>
struct OpHandlers<IR::OP_F80BCDSTORE> {
  static X80SoftFloat handle(X80SoftFloat Src1) {
    bool Negative = Src1.Sign;

    Src1 = X80SoftFloat::FRNDINT(Src1);

    // Clear the Sign bit
    Src1.Sign = 0;

    uint64_t Tmp = Src1;
    X80SoftFloat Rv;
    uint8_t *BCD = reinterpret_cast<uint8_t*>(&Rv);
    memset(BCD, 0, 10);

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

    return Rv;
  }
};

template<>
struct OpHandlers<IR::OP_F80BCDLOAD> {
  static X80SoftFloat handle(X80SoftFloat Src) {
    uint8_t *Src1 = reinterpret_cast<uint8_t *>(&Src);
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
    return Tmp;
  }
};

template<>
struct OpHandlers<IR::OP_F80LOADFCW> {
  static void handle(uint16_t NewFCW) {

    auto PC = (NewFCW >> 8) & 3;
    switch(PC) {
      case 0: extF80_roundingPrecision = 32; break;
      case 2: extF80_roundingPrecision = 64; break;
      case 3: extF80_roundingPrecision = 80; break;
      case 1: LOGMAN_MSG_A_FMT("Invalid x87 precision mode, {}", PC);
    }

    auto RC = (NewFCW >> 10) & 3;
    switch(RC) {
      case 0:
        softfloat_roundingMode = softfloat_round_near_even;
        break;
      case 1:
        softfloat_roundingMode = softfloat_round_min;
        break;
      case 2:
        softfloat_roundingMode = softfloat_round_max;
        break;
      case 3:
        softfloat_roundingMode = softfloat_round_minMag;
      break;
    }
  }
};

} // namespace FEXCore::CPU
