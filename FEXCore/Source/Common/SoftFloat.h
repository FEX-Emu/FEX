// SPDX-License-Identifier: MIT
#pragma once

#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/fextl/sstream.h>
#include <FEXCore/fextl/string.h>
#include "cephes_128bit.h"

#include <bit>
#include <cmath>
#include <cstring>
#include <stdint.h>

#include "Common/VectorRegType.h"

extern "C" {
#include "SoftFloat-3e/platform.h"
#include "SoftFloat-3e/softfloat.h"
}

struct FEX_PACKED X80SoftFloat {
#ifdef ARCHITECTURE_x86_64
// Define this to push some operations to x87
// Only useful to see if precision loss is killing something
// #define DEBUG_X86_FLOAT
#ifdef DEBUG_X86_FLOAT
#define BIGFLOAT long double
#define BIGFLOATSIZE 10
#else
#define BIGFLOAT float128_t
#define BIGFLOATSIZE 16
#endif
#elif defined(ARCHITECTURE_arm64)
#define BIGFLOAT float128_t
#define BIGFLOATSIZE 16
#else
#error No 128bit float for this target!
#endif

  uint64_t Significand : 64;
  uint16_t Exponent    : 15;
  uint16_t Sign        : 1;

  X80SoftFloat() {
    memset(this, 0, sizeof(*this));
  }
  X80SoftFloat(uint16_t _Sign, uint16_t _Exponent, uint64_t _Significand)
    : Significand {_Significand}
    , Exponent {_Exponent}
    , Sign {_Sign} {}

  fextl::string str() const {
    fextl::ostringstream string;
    string << std::hex << Sign;
    string << "_" << Exponent;
    string << "_" << (Significand >> 63);
    string << "_" << (Significand & ((1ULL << 63) - 1));
    return string.str();
  }

  // Ops
  FEXCORE_PRESERVE_ALL_ATTR static X80SoftFloat FADD(softfloat_state* state, const X80SoftFloat& lhs, const X80SoftFloat& rhs) {
#ifdef DEBUG_X86_FLOAT
    BIGFLOAT Result;
    asm(R"(
    fninit;
    fldt %[rhs]; # st1
    fldt %[lhs]; # st0
    faddp;
    fstpt %[result];
    )"
        : [result] "=m"(Result)
        : [lhs] "m"(lhs), [rhs] "m"(rhs)
        : "st", "st(1)");

    return Result;
#else
    return extF80_add(state, lhs, rhs);
#endif
  }

  FEXCORE_PRESERVE_ALL_ATTR static X80SoftFloat FSUB(softfloat_state* state, const X80SoftFloat& lhs, const X80SoftFloat& rhs) {
#ifdef DEBUG_X86_FLOAT
    BIGFLOAT Result;
    asm(R"(
    fninit;
    fldt %[rhs]; # st1
    fldt %[lhs]; # st0
    fsubp;
    fstpt %[result];
    )"
        : [result] "=m"(Result)
        : [lhs] "m"(lhs), [rhs] "m"(rhs)
        : "st", "st(1)");

    return Result;
#else
    return extF80_sub(state, lhs, rhs);
#endif
  }

  FEXCORE_PRESERVE_ALL_ATTR static X80SoftFloat FMUL(softfloat_state* state, const X80SoftFloat& lhs, const X80SoftFloat& rhs) {
#ifdef DEBUG_X86_FLOAT
    BIGFLOAT Result;
    asm(R"(
    fninit;
    fldt %[rhs]; # st1
    fldt %[lhs]; # st0
    fmulp;
    fstpt %[result];
    )"
        : [result] "=m"(Result)
        : [lhs] "m"(lhs), [rhs] "m"(rhs)
        : "st", "st(1)");

    return Result;
#else
    return extF80_mul(state, lhs, rhs);
#endif
  }

  FEXCORE_PRESERVE_ALL_ATTR static X80SoftFloat FDIV(softfloat_state* state, const X80SoftFloat& lhs, const X80SoftFloat& rhs) {
#ifdef DEBUG_X86_FLOAT
    BIGFLOAT Result;
    asm(R"(
    fninit;
    fldt %[rhs]; # st1
    fldt %[lhs]; # st0
    fdivp;
    fstpt %[result];
    )"
        : [result] "=m"(Result)
        : [lhs] "m"(lhs), [rhs] "m"(rhs)
        : "st", "st(1)");

    return Result;
#else
    return extF80_div(state, lhs, rhs);
#endif
  }

  FEXCORE_PRESERVE_ALL_ATTR static X80SoftFloat FREM(softfloat_state* state, const X80SoftFloat& lhs, const X80SoftFloat& rhs) {
#if defined(DEBUG_X86_FLOAT)
    BIGFLOAT Result;
    asm(R"(
    fninit;
    fldt %[rhs]; # st1
    fldt %[lhs]; # st0
    fprem;
    fstpt %[result];
    ffreep %%st(0);
    )"
        : [result] "=m"(Result)
        : [lhs] "m"(lhs), [rhs] "m"(rhs)
        : "st", "st(1)");

    return Result;
#else
    /*
     * Check for invalid operation cases first - Intel FPREM sets Invalid Operation
     * for several cases including infinity dividend and zero divisor.
     */
    X80SoftFloat result = 0;
    if (HandleInfinityOp(state, lhs, result)) {
      return result;
    } else if (lhs.Exponent == 0x7FFF && (lhs.Significand & 0x7FFFFFFFFFFFFFFFULL)) { // NaN
      // propagate NaN
      state->exceptionFlags |= softfloat_flag_invalid;
      return lhs;
    }

    // Check for zero divisor - fprem(x, 0) is invalid operation
    if (rhs.Exponent == 0 && rhs.Significand == 0) {
      state->exceptionFlags |= softfloat_flag_invalid;
      // Return QNaN
      result.Sign = 0;
      result.Exponent = 0x7FFF;
      result.Significand = 0xC000000000000000ULL;
      return result;
    }

    /*
     * FPREM is not an IEEE-754 remainder.  From the Intel spec:
     *
     *    Computes the remainder obtained from dividing the value in the ST(0)
     *    register (the dividend) by the value in the ST(1) register (the divisor
     *    or modulus), and stores the result in ST(0). The remainder represents the
     *    following value:
     *
     *    Remainder := ST(0) − (Q * ST(1))
     *
     *    Here, Q is an integer value that is obtained by truncating the
     *    floating-point number quotient of [ST(0) / ST(1)] toward zero.
     *
     * We implement this sequence literally. softfloat_round_minMag means
     * "truncate towards zero".
     */
    extFloat80_t quotient = extF80_div(state, lhs, rhs);
    extFloat80_t Q = extF80_roundToInt(state, quotient, softfloat_round_minMag, true);
    bool Q_zero = Q.signif == 0 && (Q.signExp & ~(1 << 15)) == 0;

    if (Q_zero) {
      return lhs;
    } else {
      return extF80_sub(state, lhs, extF80_mul(state, Q, rhs));
    }
#endif
  }

  FEXCORE_PRESERVE_ALL_ATTR static X80SoftFloat FREM1(softfloat_state* state, const X80SoftFloat& lhs, const X80SoftFloat& rhs) {
#if defined(DEBUG_X86_FLOAT)
    BIGFLOAT Result;
    asm(R"(
    fninit;
    fldt %[rhs]; # st1
    fldt %[lhs]; # st0
    fprem1;
    fstpt %[result];
    ffreep %%st(0);
    )"
        : [result] "=m"(Result)
        : [lhs] "m"(lhs), [rhs] "m"(rhs)
        : "st", "st(1)");

    return Result;
#else
    return extF80_rem(state, lhs, rhs);
#endif
  }

  FEXCORE_PRESERVE_ALL_ATTR static X80SoftFloat FRNDINT(softfloat_state* state, const X80SoftFloat& lhs) {
    return extF80_roundToInt(state, lhs, state->roundingMode, false);
  }

  FEXCORE_PRESERVE_ALL_ATTR static X80SoftFloat FRNDINT(softfloat_state* state, const X80SoftFloat& lhs, uint_fast8_t RoundMode) {
    return extF80_roundToInt(state, lhs, RoundMode, false);
  }

  FEXCORE_PRESERVE_ALL_ATTR static X80SoftFloat FXTRACT_SIG(const X80SoftFloat& lhs) {
#if defined(DEBUG_X86_FLOAT)
    BIGFLOAT Result;
    asm(R"(
    fninit;
    fldt %[lhs]; # st0
    fxtract;
    fstpt %[result];
    ffreep %%st(0);
    )"
        : [result] "=m"(Result)
        : [lhs] "m"(lhs)
        : "st", "st(1)");

    return Result;
#else
    // Zero is a special case, the significand for +/- 0 is +/- zero.
    if (lhs.Exponent == 0x0 && lhs.Significand == 0x0) {
      return lhs;
    }
    X80SoftFloat Tmp = lhs;
    Tmp.Exponent = 0x3FFF;
    Tmp.Sign = lhs.Sign;
    return Tmp;
#endif
  }

  FEXCORE_PRESERVE_ALL_ATTR static X80SoftFloat FXTRACT_EXP(const X80SoftFloat& lhs) {
#if defined(DEBUG_X86_FLOAT)
    BIGFLOAT Result;
    asm(R"(
    fninit;
    fldt %[lhs]; # st0
    fxtract;
    ffreep %%st(0);
    fstpt %[result];
    )"
        : [result] "=m"(Result)
        : [lhs] "m"(lhs)
        : "st", "st(1)");

    return Result;
#else
    // Zero is a special case, the exponent is always -inf
    if (lhs.Exponent == 0x0 && lhs.Significand == 0x0) {
      X80SoftFloat Result(1, 0x7FFFUL, 0x8000'0000'0000'0000UL);
      return Result;
    }

    int32_t TrueExp = lhs.Exponent - ExponentBias;
    return i32_to_extF80(TrueExp);
#endif
  }

  FEXCORE_PRESERVE_ALL_ATTR static void
  FCMP(softfloat_state* state, const X80SoftFloat& lhs, const X80SoftFloat& rhs, bool* eq, bool* lt, bool* nan) {
    *eq = extF80_eq(state, lhs, rhs);
    *lt = extF80_lt(state, lhs, rhs);

    // Use IEEE 754 semantics: unordered if neither <, =, nor > is true
    // This is more reliable than custom NaN detection
    bool gt = !(*eq) && !(*lt) && extF80_le(state, rhs, lhs);
    *nan = !(*eq) && !(*lt) && !gt;
  }

  FEXCORE_PRESERVE_ALL_ATTR static X80SoftFloat FSCALE(softfloat_state* state, const X80SoftFloat& lhs, const X80SoftFloat& rhs) {
#ifdef DEBUG_X86_FLOAT
    BIGFLOAT Result;
    asm(R"(
    fninit;
    fldt %[rhs]; # st1
    fldt %[lhs]; # st0
    fscale; # st0 = st0 * 2^(rdint(st1))
    fstpt %[result];
    ffreep %%st(0);
    )"
        : [result] "=m"(Result)
        : [lhs] "m"(lhs), [rhs] "m"(rhs)
        : "st", "st(1)");

    return Result;
#else
    extFloat80_t Zero {0, 0};
    if (extF80_eq(state, lhs, Zero)) {
      return lhs;
    }
    X80SoftFloat Int = FRNDINT(state, rhs, softfloat_round_minMag);
    BIGFLOAT Src2_d = Int.ToFMax(state);
    Src2_d = FEXCore::cephes_128bit::exp2l(Src2_d);
    X80SoftFloat Src2_X80(state, Src2_d);
    X80SoftFloat Result = extF80_mul(state, lhs, Src2_X80);
    return Result;
#endif
  }

  FEXCORE_PRESERVE_ALL_ATTR static X80SoftFloat F2XM1(softfloat_state* state, const X80SoftFloat& lhs) {
#ifdef DEBUG_X86_FLOAT
    BIGFLOAT Result;
    asm(R"(
    fninit;
    fldt %[lhs]; # st0
    f2xm1; # st0 = 2^st(0) - 1
    fstpt %[result];
    )"
        : [result] "=m"(Result)
        : [lhs] "m"(lhs)
        : "st");

    return Result;
#else
    auto Src1_d = lhs.ToFMax(state);
    auto Result = FEXCore::cephes_128bit::exp2l(Src1_d);

    static const float128_t one {0x0ULL, 0x3fff000000000000ULL};
    return X80SoftFloat(state, f128_sub(state, Result, one));
#endif
  }

  FEXCORE_PRESERVE_ALL_ATTR static X80SoftFloat FYL2X(softfloat_state* state, const X80SoftFloat& lhs, const X80SoftFloat& rhs) {
#ifdef DEBUG_X86_FLOAT
    BIGFLOAT Result;
    asm(R"(
    fninit;
    fldt %[rhs]; # st(1)
    fldt %[lhs]; # st(0)
    fyl2x; # st(1) * log2l(st(0))
    fstpt %[result];
    )"
        : [result] "=m"(Result)
        : [lhs] "m"(lhs), [rhs] "m"(rhs)
        : "st", "st(1)");

    return Result;
#else
    auto Src1_d = lhs.ToFMax(state);
    auto Src2_d = rhs.ToFMax(state);

    auto Tmp = f128_mul(state, Src2_d, FEXCore::cephes_128bit::log2l(Src1_d));
    return X80SoftFloat(state, Tmp);
#endif
  }

  FEXCORE_PRESERVE_ALL_ATTR static X80SoftFloat FATAN(softfloat_state* state, const X80SoftFloat& lhs, const X80SoftFloat& rhs) {
#ifdef DEBUG_X86_FLOAT
    BIGFLOAT Result;
    asm(R"(
    fninit;
    fldt %[lhs];
    fldt %[rhs];
    fpatan;
    fstpt %[result];
    )"
        : [result] "=m"(Result)
        : [lhs] "m"(lhs), [rhs] "m"(rhs)
        : "st", "st(1)");

    return Result;
#else
    BIGFLOAT Src1_d = lhs.ToFMax(state);
    BIGFLOAT Src2_d = rhs.ToFMax(state);
    BIGFLOAT Tmp = FEXCore::cephes_128bit::atan2l(Src1_d, Src2_d);
    return X80SoftFloat(state, Tmp);
#endif
  }

  FEXCORE_PRESERVE_ALL_ATTR static X80SoftFloat FTAN(softfloat_state* state, const X80SoftFloat& lhs) {
#ifdef DEBUG_X86_FLOAT
    BIGFLOAT Result;
    asm(R"(
    fninit;
    fldt %[lhs]; # st0
    fptan;
    ffreep %%st(0);
    fstpt %[result];
    )"
        : [result] "=m"(Result)
        : [lhs] "m"(lhs)
        : "st");

    return Result;
#else
    X80SoftFloat result;
    if (HandleInfinityOp(state, lhs, result)) {
      return result;
    }

    BIGFLOAT Src_d = lhs.ToFMax(state);
    Src_d = FEXCore::cephes_128bit::tanl(Src_d);
    return X80SoftFloat(state, Src_d);
#endif
  }

  FEXCORE_PRESERVE_ALL_ATTR static X80SoftFloat FSIN(softfloat_state* state, const X80SoftFloat& lhs) {
#ifdef DEBUG_X86_FLOAT
    BIGFLOAT Result;
    asm(R"(
    fninit;
    fldt %[lhs]; # st0
    fsin;
    fstpt %[result];
    )"
        : [result] "=m"(Result)
        : [lhs] "m"(lhs)
        : "st");

    return Result;
#else
    X80SoftFloat result;
    if (HandleInfinityOp(state, lhs, result)) {
      return result;
    }

    BIGFLOAT Src_d = lhs.ToFMax(state);
    Src_d = FEXCore::cephes_128bit::sinl(Src_d);
    return X80SoftFloat(state, Src_d);
#endif
  }

  FEXCORE_PRESERVE_ALL_ATTR static X80SoftFloat FCOS(softfloat_state* state, const X80SoftFloat& lhs) {
#ifdef DEBUG_X86_FLOAT
    BIGFLOAT Result;
    asm(R"(
    fninit;
    fldt %[lhs]; # st0
    fcos;
    fstpt %[result];
    )"
        : [result] "=m"(Result)
        : [lhs] "m"(lhs)
        : "st");

    return Result;
#else
    X80SoftFloat result;
    if (HandleInfinityOp(state, lhs, result)) {
      return result;
    }

    BIGFLOAT Src_d = lhs.ToFMax(state);
    Src_d = FEXCore::cephes_128bit::cosl(Src_d);
    return X80SoftFloat(state, Src_d);
#endif
  }

  FEXCORE_PRESERVE_ALL_ATTR static X80SoftFloat FSQRT(softfloat_state* state, const X80SoftFloat& lhs) {
#ifdef DEBUG_X86_FLOAT
    BIGFLOAT Result;
    asm(R"(
    fninit;
    fldt %[lhs]; # st0
    fsqrt;
    fstpt %[result];
    )"
        : [result] "=m"(Result)
        : [lhs] "m"(lhs)
        : "st");

    return Result;
#else
    return extF80_sqrt(state, lhs);
#endif
  }

  float ToF32(softfloat_state* state) const {
    const float32_t Result = extF80_to_f32(state, *this);
    return std::bit_cast<float>(Result);
  }

  double ToF64(softfloat_state* state) const {
    const float64_t Result = extF80_to_f64(state, *this);
    return std::bit_cast<double>(Result);
  }

  FEXCore::VectorRegType ToVector() const {
    FEXCore::VectorRegType Ret {};
    memcpy(&Ret, this, sizeof(*this));
    return Ret;
  }

  BIGFLOAT ToFMax(softfloat_state* state) const {
#if BIGFLOATSIZE == 16
    const float128_t Result = extF80_to_f128(state, *this);
    return std::bit_cast<BIGFLOAT>(Result);
#else
    BIGFLOAT result {};
    memcpy(&result, this, sizeof(result));
    return result;
#endif
  }

  int16_t ToI16(softfloat_state* state) const {
    auto rv = extF80_to_i32(state, *this, state->roundingMode, false);
    if (rv > INT16_MAX || rv < INT16_MIN) {
      ///< Indefinite value for 16-bit conversions.
      return INT16_MIN;
    } else {
      return rv;
    }
  }

  int32_t ToI32(softfloat_state* state) const {
    return extF80_to_i32(state, *this, state->roundingMode, false);
  }

  int64_t ToI64(softfloat_state* state) const {
    return extF80_to_i64(state, *this, state->roundingMode, false);
  }

  uint64_t ToUI64(softfloat_state* state) const {
    return extF80_to_ui64(state, *this, state->roundingMode, false);
  }

  void operator=(const int16_t rhs) {
    *this = i32_to_extF80(rhs);
  }

  void operator=(const int32_t rhs) {
    *this = i32_to_extF80(rhs);
  }

  void operator=(const uint64_t rhs) {
    *this = ui64_to_extF80(rhs);
  }

#if BIGFLOATSIZE == 10
  void operator=(const long double rhs) {
    memcpy(this, &rhs, sizeof(rhs));
  }
#endif

  operator void*() {
    return reinterpret_cast<void*>(this);
  }

  X80SoftFloat(extFloat80_t rhs) {
    Significand = rhs.signif;
    Exponent = rhs.signExp & 0x7FFF;
    Sign = rhs.signExp >> 15;
  }

  X80SoftFloat(softfloat_state* state, const float rhs) {
    *this = f32_to_extF80(state, std::bit_cast<float32_t>(rhs));
  }

  X80SoftFloat(softfloat_state* state, const double rhs) {
    *this = f64_to_extF80(state, std::bit_cast<float64_t>(rhs));
  }

  X80SoftFloat(softfloat_state* state, BIGFLOAT rhs) {
#if BIGFLOATSIZE == 16
    *this = f128_to_extF80(state, std::bit_cast<float128_t>(rhs));
#else
    *this = std::bit_cast<long double>(rhs);
#endif
  }

  X80SoftFloat(const int16_t rhs) {
    *this = i32_to_extF80(rhs);
  }

  X80SoftFloat(const int32_t rhs) {
    *this = i32_to_extF80(rhs);
  }

  X80SoftFloat(const FEXCore::VectorRegType rhs) {
    memcpy(this, &rhs, sizeof(*this));
  }

  void operator=(extFloat80_t rhs) {
    Significand = rhs.signif;
    Exponent = rhs.signExp & 0x7FFF;
    Sign = rhs.signExp >> 15;
  }

  operator FEXCore::VectorRegType() const {
    return ToVector();
  }

  operator extFloat80_t() const {
    extFloat80_t Result {};
    Result.signif = Significand;
    Result.signExp = Exponent | (Sign << 15);
    return Result;
  }

  static bool IsNan(const X80SoftFloat& lhs) {
    return (lhs.Exponent == 0x7FFF) && (lhs.Significand & IntegerBit) && (lhs.Significand & Bottom62Significand);
  }

  static bool SignBit(const X80SoftFloat& lhs) {
    return lhs.Sign;
  }

private:
  static constexpr uint64_t IntegerBit = (1ULL << 63);
  static constexpr uint64_t Bottom62Significand = ((1ULL << 62) - 1);
  static constexpr uint32_t ExponentBias = 16383;

  // Helper function to check for infinity and set invalid operation flag.
  // Returns true if infinity is dealt with, false otherwise.
  FEXCORE_PRESERVE_ALL_ATTR static bool HandleInfinityOp(softfloat_state* state, const X80SoftFloat& arg, X80SoftFloat& result) {
    if (arg.Exponent == 0x7FFF && arg.Significand == 0x8000000000000000ULL) {
      state->exceptionFlags |= softfloat_flag_invalid;
      // Return QNaN.
      result.Sign = 0;
      result.Exponent = 0x7FFF;
      result.Significand = 0xC000000000000000ULL;
      return true;
    }
    return false;
  }
};

#ifndef _WIN32
static_assert(sizeof(X80SoftFloat) == 10, "tword must be 10bytes in size");
#else
// Padding on this extends to 16-bytes rather than 10-bytes on WIN32.
static_assert(sizeof(X80SoftFloat) == 16, "tword must be 16bytes in size");
#endif
