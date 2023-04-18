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

template<>
struct OpHandlers<IR::OP_VPCMPESTRX> {
  enum class AggregationOp {
    EqualAny     = 0b00,
    Ranges       = 0b01,
    EqualEach    = 0b10,
    EqualOrdered = 0b11,
  };

  enum class SourceData {
    U8,
    U16,
    S8,
    S16,
  };

  enum class Polarity {
    Positive,
    Negative,
    PositiveMasked,
    NegativeMasked,
  };

  static uint32_t handle(uint64_t RAX, uint64_t RDX, __uint128_t lhs, __uint128_t rhs, uint16_t control) {
    // Subtract by 1 in order to make validity limits 0-based
    const auto valid_lhs = GetExplicitLength(RAX, control) - 1;
    const auto valid_rhs = GetExplicitLength(RDX, control) - 1;

    return MainBody(lhs, valid_lhs, rhs, valid_rhs, control);
  }

  // Main PCMPXSTRX algorithm body. Allows for reuse with both implicit and explicit length variants.
  static uint32_t MainBody(const __uint128_t& lhs, int valid_lhs, const __uint128_t& rhs, int valid_rhs, uint16_t control) {
    const uint32_t aggregation = PerformAggregation(lhs, valid_lhs, rhs, valid_rhs, control);
    const uint32_t upper_limit = (16U >> (control & 1)) - 1;

    // Bits are arranged as:
    // Bit #:   3    2    1    0
    //         [OF | CF | SF | ZF]
    uint32_t flags = 0;
    flags |= (valid_rhs < upper_limit) ? 0b01 : 0b00;
    flags |= (valid_lhs < upper_limit) ? 0b10 : 0b00;

    const uint32_t result = HandlePolarity(aggregation, control, upper_limit, valid_rhs);
    if (result != 0) {
      flags |= 0b0100;
    }
    if ((result & 1) != 0) {
      flags |= 0b1000;
    }

    // We tack the flags on top of the result to avoid needing to handle
    // multiple return values in the JITs.
    return result | (flags << 16);
  }

  static int32_t GetExplicitLength(uint64_t reg, uint16_t control) {
    // Bit 8 controls whether or not the reg value is 64-bit or 32-bit.
    int64_t value = 0;
    if (((control >> 8) & 1) != 0) {
      value = static_cast<int64_t>(reg);
    } else {
      // We need a sign extend in this case.
      value = static_cast<int32_t>(reg);
    }

    // If control[0] is set, then we're dealing with words instead of bytes
    const int64_t limit = (control & 1) != 0 ? 8 : 16;

    // Length needs to saturate to 16 (if bytes) or 8 (if words)
    // when the length value is greater than 16 (if bytes)/8 (if words)
    // or if the length value is less than -16 (if bytes)/-8 (if words).
    if (value < -limit || value > limit) {
      return limit;
    }

    return std::abs(static_cast<int>(value));
  }

  static int32_t GetElement(const __uint128_t& vec, int32_t index, uint16_t control) {
    const auto* vec_ptr = reinterpret_cast<const uint8_t*>(&vec);

    // Control bits [1:0] define the data type being dealt with.
    switch (static_cast<SourceData>(control & 0b11)) {
    case SourceData::U8:
      return static_cast<int32_t>(vec_ptr[index]);
    case SourceData::U16: {
      uint16_t value{};
      std::memcpy(&value, vec_ptr + (sizeof(uint16_t) * static_cast<size_t>(index)), sizeof(value));
      return value;
    }
    case SourceData::S8:
      return static_cast<int8_t>(vec_ptr[index]);
    case SourceData::S16:
    default: {
      int16_t value{};
      std::memcpy(&value, vec_ptr + (sizeof(int16_t) * static_cast<size_t>(index)), sizeof(value));
      return value;
    }
    }
  }

  static uint32_t PerformAggregation(const __uint128_t& lhs, int32_t valid_lhs,
                                     const __uint128_t& rhs, int32_t valid_rhs,
                                     uint16_t control) {
    switch (static_cast<AggregationOp>((control >> 2) & 0b11)) {
    case AggregationOp::EqualAny:
      return HandleEqualAny(lhs, valid_lhs, rhs, valid_rhs, control);
    case AggregationOp::Ranges:
      return HandleRanges(lhs, valid_lhs, rhs, valid_rhs, control);
    case AggregationOp::EqualEach:
      return HandleEqualEach(lhs, valid_lhs, rhs, valid_rhs, control);
    case AggregationOp::EqualOrdered:
    default:
      return HandleEqualOrdered(lhs, valid_lhs, rhs, valid_rhs, control);
    }
  }

  static uint32_t HandlePolarity(uint32_t value, uint16_t control, int upper_limit, int valid_rhs) {
    switch (static_cast<Polarity>((control >> 4) & 0b11)) {
      case Polarity::Negative:
        return value ^ ((2U << upper_limit) - 1);
      case Polarity::NegativeMasked:
        return value ^ ((1U << (valid_rhs + 1)) - 1);
      case Polarity::Positive:
      case Polarity::PositiveMasked:
      default:
        // Both positive masking and positive polarity are documented
        // as both being equivalent to "IntRes2 = IntRes1", where IntRes1
        // is our 'value' parameter, so we don't need to do anything in
        // these cases except return the same value.
        return value;
    }
  }

  // Finds characters from an overall character set.
  //
  // Scans through RHS trying to find any characters contained in LHS.
  // Think of this as a sort of vectorized version of strspn (kind of).
  //
  // e.g. Assume operating on two character vectors as unsigned words
  //
  //         0  1  2  3  4  5  6  7
  // LHS -> [a, b, c, d, e, f, g, n]
  // RHS -> [z, k, v, c, d, o, p, n]
  //
  // With both explicit lengths for each string being 8 (the max length for words),
  // this would result in an intermediate result like:
  //
  //            0b1001'1000
  //              │  │ │
  // 'n' match ───┘  │ │
  //                 │ │
  // 'd' match ──────┘ │
  //                   │
  // 'c' match ────────┘
  //
  static uint32_t HandleEqualAny(const __uint128_t& lhs, int32_t valid_lhs,
                                 const __uint128_t& rhs, int32_t valid_rhs,
                                 uint16_t control) {
    uint32_t result = 0;

    for (int j = valid_rhs; j >= 0; j--) {
      result <<= 1;

      const int rhs_value = GetElement(rhs, j, control);
      for (int i = valid_lhs; i >= 0; i--) {
        const int lhs_value = GetElement(lhs, i, control);
        result |= static_cast<uint32_t>(rhs_value == lhs_value);
      }
    }

    return result;
  }

  // Determines if a character falls within a limited range
  //
  // Scans through rhs using a range denoted by two elements
  // in lhs and determines if the respective character in rhs
  // falls within its range.
  //
  // i.e.
  //      lhs_upper_bound >= rhs_value && lhs_lower_bound <= rhs_value
  //
  // e.g. Assume operating on two character vectors as unsigned words
  //
  //         0  1  2  3  4  5  6  7
  // LHS -> [a, z, A, Z, 0, 0, 0, 0]
  // RHS -> [z, k, ., C, M, ;, \, ']
  //
  // With LHS's length being 4 and RHS's lenth being 8,
  // this would result in an intermediate result like:
  //
  //                          0b0001'1011
  //                               │ │ ││
  // 'z' >= 'M' && 'a' <= 'M' ─────┘ │ ││
  //                                 │ ││
  // 'z' >= 'C' && 'a' <= 'C' ───────┘ ││
  //                                   ││
  // 'Z' >= 'k' && 'A' <= 'k' ─────────┘│
  //                                    │
  // 'Z' >= 'z' && 'A' <= 'z' ──────────┘
  //
  static uint32_t HandleRanges(const __uint128_t& lhs, int32_t valid_lhs,
                               const __uint128_t& rhs, int32_t valid_rhs,
                               uint16_t control) {
    uint32_t result = 0;

    for (int j = valid_rhs; j >= 0; j--) {
      result <<= 1;

      const int element = GetElement(rhs, j, control);
      for (int i = (valid_lhs - 1) | 1; i >= 0; i -= 2) {
        const int upper_bound = GetElement(lhs, i - 0, control);
        const int lower_bound = GetElement(lhs, i - 1, control);

        const bool ge = upper_bound >= element;
        const bool le = lower_bound <= element;

        result |= static_cast<uint32_t>(ge && le);
      }
    }

    return result;
  }

  // Determines if each character is equal to one another (string compare)
  //
  // Essentially the PCMPXSTRX variant of memcmp/strcmp. Sets the bit of the
  // resulting mask if both elements are equal to one another. Otherwise
  // sets it to false.
  //
  // e.g. Assume operating on two character vectors as unsigned words
  //
  //         0  1  2  3  4  5  6  7
  // LHS -> [a, b, c, d, e, f, g, n]
  // RHS -> [a, b, c, d, e, f, e, x]
  //
  // With both explicit lengths for each string being 8 (the max length for words),
  // this would result in an intermediate result like:
  //
  //            0b0011'1111
  //                ││ ││││
  // 'f' == 'f' ────┘│ ││││
  //                 │ ││││
  // 'e' == 'e' ─────┘ ││││
  //                   ││││
  // 'd' == 'd' ───────┘│││
  //                    │││
  // 'c' == 'c' ────────┘││
  //                     ││
  // 'b' == 'b' ─────────┘│
  //                      │
  // 'a' == 'a' ──────────┘
  //
  static uint32_t HandleEqualEach(const __uint128_t& lhs, int32_t valid_lhs,
                                  const __uint128_t& rhs, int32_t valid_rhs,
                                  uint16_t control) {
    const auto upper_limit = (16 >> (control & 1)) - 1;
    const auto max_valid = std::max(valid_lhs, valid_rhs);
    const auto min_valid = std::min(valid_lhs, valid_rhs);

    // All values past the end of string must be forced to true.
    // (See 4.1.6 Valid/Invalid Override of Comparisons in the Intel Software Development Manual)
    // So we can calculate this part of the mask ahead of time and set all those to-be bits to true
    // and then progressively shift them into place over the course of execution.
    uint32_t result = (1U << (upper_limit - max_valid)) - 1;
    result <<= (max_valid - min_valid);

    for (int i = min_valid; i >= 0; i--) {
      const int lhs_element = GetElement(lhs, i, control);
      const int rhs_element = GetElement(rhs, i, control);

      result <<= 1;
      result |= static_cast<uint32_t>(lhs_element == rhs_element);
    }

    return result;
  }

  // Determines if a substring exists within an overall string
  //
  // Somewhat equivalent to the behavior of strstr.
  //
  // Sets the corresponding index in the result where a substring is found.
  //
  // e.g. Assume operating on two character vectors as unsigned words
  //
  //         0  1  2  3  4  5  6  7
  // LHS -> [b, a, x, z, y, v, o, m]
  // RHS -> [b, a, d, b, a, n, k, s]
  //
  // With the length of LHS being 2 and the length of RHS being 8, we have a composition like:
  //
  //      Substring to look for
  //       ┌──┴──┐
  // LHS -> [b, a, x, z, y, v, o, m]
  // RHS -> [b, a, d, b, a, n, k, s]
  //       └───────────┬────────────┘
  //         Entire string to search
  //
  // And we end up with a result like:
  //
  //            0b0000'1001
  //                   │  │
  // At index 3 ───────┘  │
  //                      │
  // At index 0 ──────────┘
  //
  static uint32_t HandleEqualOrdered(const __uint128_t& lhs, int32_t valid_lhs,
                                     const __uint128_t& rhs, int32_t valid_rhs,
                                     uint16_t control) {
    const auto upper_limit = (16 >> (control & 1)) - 1;

    // Edge case!
    // If we have *no* valid characters in our inner string, then
    // we need to return the intermediate result as
    // 0xFF (if operating on words) or 0xFFFF (if operating on bytes)
    if (valid_lhs == -1) {
      return (2U << upper_limit) - 1;
    }

    uint32_t result = 0;
    const int initial = valid_rhs == upper_limit ? valid_rhs
                                                 : valid_rhs - valid_lhs;
    for (int j = initial; j >= 0; j--) {
      result <<= 1;

      uint32_t value = 1;
      const int start = std::min(valid_rhs - j, valid_lhs);
      for (int i = start; i >= 0; i--) {
        const int lhs_value = GetElement(lhs, i + 0, control);
        const int rhs_value = GetElement(rhs, i + j, control);

        value &= static_cast<uint32_t>(lhs_value == rhs_value);
      }

      result |= value;
    }

    return result;
  }
};

}
