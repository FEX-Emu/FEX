#pragma once
#include "Common/SoftFloat.h"
#include "Common/SoftFloat-3e/softfloat.h"

#include "Interface/Core/Interpreter/Fallbacks/FallbackOpHandler.h"
#include "Interface/IR/IR.h"

namespace FEXCore::CPU {
FEXCORE_PRESERVE_ALL_ATTR static void LoadDeferredFCW(uint16_t NewFCW) {
  auto PC = (NewFCW >> 8) & 3;
  switch (PC) {
  case 0: extF80_roundingPrecision = 32; break;
  case 2: extF80_roundingPrecision = 64; break;
  case 3: extF80_roundingPrecision = 80; break;
  case 1: LOGMAN_MSG_A_FMT("Invalid x87 precision mode, {}", PC);
  }

  auto RC = (NewFCW >> 10) & 3;
  switch (RC) {
  case 0: softfloat_roundingMode = softfloat_round_near_even; break;
  case 1: softfloat_roundingMode = softfloat_round_min; break;
  case 2: softfloat_roundingMode = softfloat_round_max; break;
  case 3: softfloat_roundingMode = softfloat_round_minMag; break;
  }
}

template<>
struct OpHandlers<IR::OP_F80CVTTO> {
  FEXCORE_PRESERVE_ALL_ATTR static X80SoftFloat handle4(uint16_t NewFCW, float src) {
    LoadDeferredFCW(NewFCW);
    return src;
  }

  FEXCORE_PRESERVE_ALL_ATTR static X80SoftFloat handle8(uint16_t NewFCW, double src) {
    LoadDeferredFCW(NewFCW);
    return src;
  }
};

template<>
struct OpHandlers<IR::OP_F80CMP> {
  FEXCORE_PRESERVE_ALL_ATTR static uint64_t handle(uint16_t NewFCW, X80SoftFloat Src1, X80SoftFloat Src2) {
    LoadDeferredFCW(NewFCW);

    bool eq, lt, nan;
    uint64_t ResultFlags = 0;

    X80SoftFloat::FCMP(Src1, Src2, &eq, &lt, &nan);
    if (lt) {
      ResultFlags |= (1 << IR::FCMP_FLAG_LT);
    }
    if (nan) {
      ResultFlags |= (1 << IR::FCMP_FLAG_UNORDERED);
    }
    if (eq) {
      ResultFlags |= (1 << IR::FCMP_FLAG_EQ);
    }
    return ResultFlags;
  }
};

template<>
struct OpHandlers<IR::OP_F80CVT> {
  FEXCORE_PRESERVE_ALL_ATTR static float handle4(uint16_t NewFCW, X80SoftFloat src) {
    LoadDeferredFCW(NewFCW);
    return src;
  }

  FEXCORE_PRESERVE_ALL_ATTR static double handle8(uint16_t NewFCW, X80SoftFloat src) {
    LoadDeferredFCW(NewFCW);
    return src;
  }
};

template<>
struct OpHandlers<IR::OP_F80CVTINT> {
  FEXCORE_PRESERVE_ALL_ATTR static int16_t handle2(uint16_t NewFCW, X80SoftFloat src) {
    LoadDeferredFCW(NewFCW);
    return src;
  }

  FEXCORE_PRESERVE_ALL_ATTR static int32_t handle4(uint16_t NewFCW, X80SoftFloat src) {
    LoadDeferredFCW(NewFCW);
    return src;
  }

  FEXCORE_PRESERVE_ALL_ATTR static int64_t handle8(uint16_t NewFCW, X80SoftFloat src) {
    LoadDeferredFCW(NewFCW);
    return src;
  }

  FEXCORE_PRESERVE_ALL_ATTR static int16_t handle2t(uint16_t NewFCW, X80SoftFloat src) {
    LoadDeferredFCW(NewFCW);
    auto rv = extF80_to_i32(src, softfloat_round_minMag, false);

    if (rv > INT16_MAX || rv < INT16_MIN) {
      ///< Indefinite value for 16-bit conversions.
      return INT16_MIN;
    } else {
      return rv;
    }
  }

  FEXCORE_PRESERVE_ALL_ATTR static int32_t handle4t(uint16_t NewFCW, X80SoftFloat src) {
    LoadDeferredFCW(NewFCW);
    return extF80_to_i32(src, softfloat_round_minMag, false);
  }

  FEXCORE_PRESERVE_ALL_ATTR static int64_t handle8t(uint16_t NewFCW, X80SoftFloat src) {
    LoadDeferredFCW(NewFCW);
    return extF80_to_i64(src, softfloat_round_minMag, false);
  }
};

template<>
struct OpHandlers<IR::OP_F80CVTTOINT> {
  FEXCORE_PRESERVE_ALL_ATTR static X80SoftFloat handle2(uint16_t NewFCW, int16_t src) {
    LoadDeferredFCW(NewFCW);
    return src;
  }

  FEXCORE_PRESERVE_ALL_ATTR static X80SoftFloat handle4(uint16_t NewFCW, int32_t src) {
    LoadDeferredFCW(NewFCW);
    return src;
  }
};

template<>
struct OpHandlers<IR::OP_F80ROUND> {
  FEXCORE_PRESERVE_ALL_ATTR static X80SoftFloat handle(uint16_t NewFCW, X80SoftFloat Src1) {
    LoadDeferredFCW(NewFCW);
    return X80SoftFloat::FRNDINT(Src1);
  }
};

template<>
struct OpHandlers<IR::OP_F80F2XM1> {
  FEXCORE_PRESERVE_ALL_ATTR static X80SoftFloat handle(uint16_t NewFCW, X80SoftFloat Src1) {
    LoadDeferredFCW(NewFCW);
    return X80SoftFloat::F2XM1(Src1);
  }
};

template<>
struct OpHandlers<IR::OP_F80TAN> {
  FEXCORE_PRESERVE_ALL_ATTR static X80SoftFloat handle(uint16_t NewFCW, X80SoftFloat Src1) {
    LoadDeferredFCW(NewFCW);
    return X80SoftFloat::FTAN(Src1);
  }
};

template<>
struct OpHandlers<IR::OP_F80SQRT> {
  FEXCORE_PRESERVE_ALL_ATTR static X80SoftFloat handle(uint16_t NewFCW, X80SoftFloat Src1) {
    LoadDeferredFCW(NewFCW);
    return X80SoftFloat::FSQRT(Src1);
  }
};

template<>
struct OpHandlers<IR::OP_F80SIN> {
  FEXCORE_PRESERVE_ALL_ATTR static X80SoftFloat handle(uint16_t NewFCW, X80SoftFloat Src1) {
    LoadDeferredFCW(NewFCW);
    return X80SoftFloat::FSIN(Src1);
  }
};

template<>
struct OpHandlers<IR::OP_F80COS> {
  FEXCORE_PRESERVE_ALL_ATTR static X80SoftFloat handle(uint16_t NewFCW, X80SoftFloat Src1) {
    LoadDeferredFCW(NewFCW);
    return X80SoftFloat::FCOS(Src1);
  }
};

template<>
struct OpHandlers<IR::OP_F80XTRACT_EXP> {
  FEXCORE_PRESERVE_ALL_ATTR static X80SoftFloat handle(uint16_t NewFCW, X80SoftFloat Src1) {
    LoadDeferredFCW(NewFCW);
    return X80SoftFloat::FXTRACT_EXP(Src1);
  }
};

template<>
struct OpHandlers<IR::OP_F80XTRACT_SIG> {
  FEXCORE_PRESERVE_ALL_ATTR static X80SoftFloat handle(uint16_t NewFCW, X80SoftFloat Src1) {
    LoadDeferredFCW(NewFCW);
    return X80SoftFloat::FXTRACT_SIG(Src1);
  }
};

template<>
struct OpHandlers<IR::OP_F80ADD> {
  FEXCORE_PRESERVE_ALL_ATTR static X80SoftFloat handle(uint16_t NewFCW, X80SoftFloat Src1, X80SoftFloat Src2) {
    LoadDeferredFCW(NewFCW);
    return X80SoftFloat::FADD(Src1, Src2);
  }
};

template<>
struct OpHandlers<IR::OP_F80SUB> {
  FEXCORE_PRESERVE_ALL_ATTR static X80SoftFloat handle(uint16_t NewFCW, X80SoftFloat Src1, X80SoftFloat Src2) {
    LoadDeferredFCW(NewFCW);
    return X80SoftFloat::FSUB(Src1, Src2);
  }
};

template<>
struct OpHandlers<IR::OP_F80MUL> {
  FEXCORE_PRESERVE_ALL_ATTR static X80SoftFloat handle(uint16_t NewFCW, X80SoftFloat Src1, X80SoftFloat Src2) {
    LoadDeferredFCW(NewFCW);
    return X80SoftFloat::FMUL(Src1, Src2);
  }
};

template<>
struct OpHandlers<IR::OP_F80DIV> {
  FEXCORE_PRESERVE_ALL_ATTR static X80SoftFloat handle(uint16_t NewFCW, X80SoftFloat Src1, X80SoftFloat Src2) {
    LoadDeferredFCW(NewFCW);
    return X80SoftFloat::FDIV(Src1, Src2);
  }
};

template<>
struct OpHandlers<IR::OP_F80FYL2X> {
  FEXCORE_PRESERVE_ALL_ATTR static X80SoftFloat handle(uint16_t NewFCW, X80SoftFloat Src1, X80SoftFloat Src2) {
    LoadDeferredFCW(NewFCW);
    return X80SoftFloat::FYL2X(Src1, Src2);
  }
};

template<>
struct OpHandlers<IR::OP_F80ATAN> {
  FEXCORE_PRESERVE_ALL_ATTR static X80SoftFloat handle(uint16_t NewFCW, X80SoftFloat Src1, X80SoftFloat Src2) {
    LoadDeferredFCW(NewFCW);
    return X80SoftFloat::FATAN(Src1, Src2);
  }
};

template<>
struct OpHandlers<IR::OP_F80FPREM1> {
  FEXCORE_PRESERVE_ALL_ATTR static X80SoftFloat handle(uint16_t NewFCW, X80SoftFloat Src1, X80SoftFloat Src2) {
    LoadDeferredFCW(NewFCW);
    return X80SoftFloat::FREM1(Src1, Src2);
  }
};

template<>
struct OpHandlers<IR::OP_F80FPREM> {
  FEXCORE_PRESERVE_ALL_ATTR static X80SoftFloat handle(uint16_t NewFCW, X80SoftFloat Src1, X80SoftFloat Src2) {
    LoadDeferredFCW(NewFCW);
    return X80SoftFloat::FREM(Src1, Src2);
  }
};

template<>
struct OpHandlers<IR::OP_F80SCALE> {
  FEXCORE_PRESERVE_ALL_ATTR static X80SoftFloat handle(uint16_t NewFCW, X80SoftFloat Src1, X80SoftFloat Src2) {
    LoadDeferredFCW(NewFCW);
    return X80SoftFloat::FSCALE(Src1, Src2);
  }
};

template<>
struct OpHandlers<IR::OP_F64SIN> {
  static double handle(uint16_t NewFCW, double src) {
    LoadDeferredFCW(NewFCW);
    return sin(src);
  }
};

template<>
struct OpHandlers<IR::OP_F64COS> {
  static double handle(uint16_t NewFCW, double src) {
    LoadDeferredFCW(NewFCW);
    return cos(src);
  }
};

template<>
struct OpHandlers<IR::OP_F64TAN> {
  static double handle(uint16_t NewFCW, double src) {
    LoadDeferredFCW(NewFCW);
    return tan(src);
  }
};

template<>
struct OpHandlers<IR::OP_F64F2XM1> {
  static double handle(uint16_t NewFCW, double src) {
    LoadDeferredFCW(NewFCW);
    return exp2(src) - 1.0;
  }
};

template<>
struct OpHandlers<IR::OP_F64ATAN> {
  static double handle(uint16_t NewFCW, double src1, double src2) {
    LoadDeferredFCW(NewFCW);
    return atan2(src1, src2);
  }
};

template<>
struct OpHandlers<IR::OP_F64FPREM> {
  static double handle(uint16_t NewFCW, double src1, double src2) {
    LoadDeferredFCW(NewFCW);
    return fmod(src1, src2);
  }
};

template<>
struct OpHandlers<IR::OP_F64FPREM1> {
  static double handle(uint16_t NewFCW, double src1, double src2) {
    LoadDeferredFCW(NewFCW);
    return remainder(src1, src2);
  }
};

template<>
struct OpHandlers<IR::OP_F64FYL2X> {
  static double handle(uint16_t NewFCW, double src1, double src2) {
    LoadDeferredFCW(NewFCW);
    return src2 * log2(src1);
  }
};

template<>
struct OpHandlers<IR::OP_F64SCALE> {
  static double handle(uint16_t NewFCW, double src1, double src2) {
    LoadDeferredFCW(NewFCW);
    double trunc = (double)(int64_t)(src2); // truncate
    return src1 * exp2(trunc);
  }
};

template<>
struct OpHandlers<IR::OP_F80BCDSTORE> {
  FEXCORE_PRESERVE_ALL_ATTR static X80SoftFloat handle(uint16_t NewFCW, X80SoftFloat Src1) {
    LoadDeferredFCW(NewFCW);
    bool Negative = Src1.Sign;

    Src1 = X80SoftFloat::FRNDINT(Src1);

    // Clear the Sign bit
    Src1.Sign = 0;

    uint64_t Tmp = Src1;
    X80SoftFloat Rv;
    uint8_t* BCD = reinterpret_cast<uint8_t*>(&Rv);
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
  FEXCORE_PRESERVE_ALL_ATTR static X80SoftFloat handle(uint16_t NewFCW, X80SoftFloat Src) {
    LoadDeferredFCW(NewFCW);
    uint8_t* Src1 = reinterpret_cast<uint8_t*>(&Src);
    uint64_t BCD {};
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
struct OpHandlers<IR::OP_F80XAM> {
  FEXCORE_PRESERVE_ALL_ATTR static uint64_t handle(uint32_t TopValid, X80SoftFloat Src) {
    // Return flag results in the order of C3:C2:C1:C0
    // ✅0b0000: +Unsupported format
    // ✅0b0001: +NaN
    // ✅0b0010: -Unsupported format
    // ✅0b0011: -NaN
    // ✅0b0100: +Normal
    // ✅0b0101: +Infinity
    // ✅0b0110: -Normal
    // ✅0b0111: -Infinity
    // ✅0b1000: +0
    // ✅0b1001: +empty
    // ✅0b1010: -0
    // ✅0b1011: -empty
    // ✅0b1100: +denormal
    // ✅0b1110: -denormal
    // 0b1111: ?????????

    ///< Sign of what is in the register is always passed through.
    uint32_t Sign = X80SoftFloat::SignBit(Src);

    uint32_t Result {};
    if (!TopValid) {
      ///< Top invalid takes priority.
      Result = (1 << 3) | 1;
    } else if (Src.Exponent == 0x7FFF) {
      if (Src.Significand == (1ULL << 63)) {
        ///< Inf
        Result = (1 << 2) | 1;
      } else if (Src.Significand & (1ULL << 63)) {
        ///< NaN
        Result = 1;
      }
    } else if (Src.Exponent == 0) {
      if (Src.Significand == 0) {
        ///< Zero
        Result = (1 << 3);
      } else {
        ///< Denormal
        Result = (1 << 3) | (1 << 2);
      }
    } else if (Src.Significand & (1ULL << 63)) {
      ///< Normal
      Result = 1 << 2;
    }
    return Result | (Sign << 1);
  }
};

} // namespace FEXCore::CPU
