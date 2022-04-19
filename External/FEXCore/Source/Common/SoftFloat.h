#pragma once

#include <FEXCore/Utils/BitUtils.h>
#include <FEXCore/Utils/LogManager.h>

#include <cmath>
#include <cstring>
#include <stdint.h>
#include <string>
#include <sstream>

extern "C" {
#include "SoftFloat-3e/platform.h"
#include "SoftFloat-3e/softfloat.h"
}

struct X80SoftFloat {
#ifdef _M_X86_64
// Define this to push some operations to x87
// Only useful to see if precision loss is killing something
// #define DEBUG_X86_FLOAT
#ifdef DEBUG_X86_FLOAT
#define BIGFLOAT long double
#define BIGFLOATSIZE 10
#else
#define BIGFLOAT __float128
#define BIGFLOATSIZE 16
#endif
#elif defined(_M_ARM_64) || defined(_M_RISCV_64)
#define BIGFLOAT long double
#define BIGFLOATSIZE 16
#else
#error No 128bit float for this target!
#endif
  struct __attribute__((packed)) {
    uint64_t Significand : 64;
    uint16_t Exponent    : 15;
    unsigned Sign        : 1;
  };

  X80SoftFloat() { memset(this, 0, sizeof(*this)); }
  X80SoftFloat(unsigned _Sign, uint16_t _Exponent, uint64_t _Significand)
    : Significand {_Significand}
    , Exponent {_Exponent}
    , Sign {_Sign}
    {
  }

  std::string str() const {
    std::ostringstream string;
    string << std::hex << Sign;
    string << "_" << Exponent;
    string << "_" << (Significand >> 63);
    string << "_" << (Significand & ((1ULL << 63) - 1));
    return string.str();
  }

  // Ops
  static X80SoftFloat FADD(X80SoftFloat const &lhs, X80SoftFloat const &rhs) {
#ifdef DEBUG_X86_FLOAT
    BIGFLOAT Result;
    asm (R"(
    fninit;
    fldt %[rhs]; # st1
    fldt %[lhs]; # st0
    faddp;
    fstpt %[result];
    )"
    : [result] "=m" (Result)
    : [lhs] "m" (lhs)
    , [rhs] "m" (rhs)
    : "st", "st(1)");

    return Result;
#else
    return extF80_add(lhs, rhs);
#endif
  }

  static X80SoftFloat FSUB(X80SoftFloat const &lhs, X80SoftFloat const &rhs) {
#ifdef DEBUG_X86_FLOAT
    BIGFLOAT Result;
    asm (R"(
    fninit;
    fldt %[rhs]; # st1
    fldt %[lhs]; # st0
    fsubp;
    fstpt %[result];
    )"
    : [result] "=m" (Result)
    : [lhs] "m" (lhs)
    , [rhs] "m" (rhs)
    : "st", "st(1)");

    return Result;
#else
    return extF80_sub(lhs, rhs);
#endif
  }

  static X80SoftFloat FMUL(X80SoftFloat const &lhs, X80SoftFloat const &rhs) {
#ifdef DEBUG_X86_FLOAT
    BIGFLOAT Result;
    asm (R"(
    fninit;
    fldt %[rhs]; # st1
    fldt %[lhs]; # st0
    fmulp;
    fstpt %[result];
    )"
    : [result] "=m" (Result)
    : [lhs] "m" (lhs)
    , [rhs] "m" (rhs)
    : "st", "st(1)");

    return Result;
#else
    return extF80_mul(lhs, rhs);
#endif
  }

  static X80SoftFloat FDIV(X80SoftFloat const &lhs, X80SoftFloat const &rhs) {
#ifdef DEBUG_X86_FLOAT
    BIGFLOAT Result;
    asm (R"(
    fninit;
    fldt %[rhs]; # st1
    fldt %[lhs]; # st0
    fdivp;
    fstpt %[result];
    )"
    : [result] "=m" (Result)
    : [lhs] "m" (lhs)
    , [rhs] "m" (rhs)
    : "st", "st(1)");

    return Result;
#else
    return extF80_div(lhs, rhs);
#endif
  }

  static X80SoftFloat FREM(X80SoftFloat const &lhs, X80SoftFloat const &rhs) {
#if defined(DEBUG_X86_FLOAT)
    BIGFLOAT Result;
    asm (R"(
    fninit;
    fldt %[rhs]; # st1
    fldt %[lhs]; # st0
    fprem;
    fstpt %[result];
    ffreep %%st(0);
    )"
    : [result] "=m" (Result)
    : [lhs] "m" (lhs)
    , [rhs] "m" (rhs)
    : "st", "st(1)");

    return Result;
#else
    return extF80_rem(lhs, rhs);
#endif
  }

  static X80SoftFloat FREM1(X80SoftFloat const &lhs, X80SoftFloat const &rhs) {
#if defined(DEBUG_X86_FLOAT)
    BIGFLOAT Result;
    asm (R"(
    fninit;
    fldt %[rhs]; # st1
    fldt %[lhs]; # st0
    fprem1;
    fstpt %[result];
    ffreep %%st(0);
    )"
    : [result] "=m" (Result)
    : [lhs] "m" (lhs)
    , [rhs] "m" (rhs)
    : "st", "st(1)");

    return Result;
#else
    return extF80_rem(lhs, rhs);
#endif
  }

  static X80SoftFloat FRNDINT(X80SoftFloat const &lhs) {
    return extF80_roundToInt(lhs, softfloat_roundingMode, false);
  }

  static X80SoftFloat FRNDINT(X80SoftFloat const &lhs, uint_fast8_t RoundMode) {
    return extF80_roundToInt(lhs, RoundMode, false);
  }

  static X80SoftFloat FXTRACT_SIG(X80SoftFloat const &lhs) {
#if defined(DEBUG_X86_FLOAT)
    BIGFLOAT Result;
    asm (R"(
    fninit;
    fldt %[lhs]; # st0
    fxtract;
    fstpt %[result];
    ffreep %%st(0);
    )"
    : [result] "=m" (Result)
    : [lhs] "m" (lhs)
    : "st", "st(1)");

    return Result;
#else
    X80SoftFloat Tmp = lhs;
    Tmp.Exponent = 0x3FFF;
    Tmp.Sign = lhs.Sign;
    return Tmp;
#endif
  }

  static X80SoftFloat FXTRACT_EXP(X80SoftFloat const &lhs) {
#if defined(DEBUG_X86_FLOAT)
    BIGFLOAT Result;
    asm (R"(
    fninit;
    fldt %[lhs]; # st0
    fxtract;
    ffreep %%st(0);
    fstpt %[result];
    )"
    : [result] "=m" (Result)
    : [lhs] "m" (lhs)
    : "st", "st(1)");

    return Result;
#else
    int32_t TrueExp = lhs.Exponent - ExponentBias;
    return i32_to_extF80(TrueExp);
#endif
  }

  static void FCMP(X80SoftFloat const &lhs, X80SoftFloat const &rhs, bool *eq, bool *lt, bool *nan) {
    *eq = extF80_eq(lhs, rhs);
    *lt = extF80_lt(lhs, rhs);
    *nan = IsNan(lhs) || IsNan(rhs);
  }

  static X80SoftFloat FSCALE(X80SoftFloat const &lhs, X80SoftFloat const &rhs) {
    WARN_ONCE_FMT("x87: Application used FSCALE which may have accuracy problems");
#ifdef DEBUG_X86_FLOAT
    BIGFLOAT Result;
    asm (R"(
    fninit;
    fldt %[rhs]; # st1
    fldt %[lhs]; # st0
    fscale; # st0 = st0 * 2^(rdint(st1))
    fstpt %[result];
    ffreep %%st(0);
    )"
    : [result] "=m" (Result)
    : [lhs] "m" (lhs)
    , [rhs] "m" (rhs)
    : "st", "st(1)");

    return Result;
#else
    X80SoftFloat Int = FRNDINT(rhs, softfloat_round_minMag);
    BIGFLOAT Src2_d = Int;
    Src2_d = exp2l(Src2_d);
    X80SoftFloat Src2_X80 = Src2_d;
    X80SoftFloat Result = extF80_mul(lhs, Src2_X80);
    return Result;
#endif
  }

  static X80SoftFloat F2XM1(X80SoftFloat const &lhs) {
    WARN_ONCE_FMT("x87: Application used F2XM1 which may have accuracy problems");
#ifdef DEBUG_X86_FLOAT
    BIGFLOAT Result;
    asm (R"(
    fninit;
    fldt %[lhs]; # st0
    f2xm1; # st0 = 2^st(0) - 1
    fstpt %[result];
    )"
    : [result] "=m" (Result)
    : [lhs] "m" (lhs)
    : "st");

    return Result;
#else
    BIGFLOAT Src1_d = lhs;
    BIGFLOAT Result = exp2l(Src1_d);
    Result -= 1.0;
    return Result;
#endif
  }

  static X80SoftFloat FYL2X(X80SoftFloat const &lhs, X80SoftFloat const &rhs) {
    WARN_ONCE_FMT("x87: Application used FYL2X which may have accuracy problems");
#ifdef DEBUG_X86_FLOAT
    BIGFLOAT Result;
    asm (R"(
    fninit;
    fldt %[rhs]; # st(1)
    fldt %[lhs]; # st(0)
    fyl2x; # st(1) * log2l(st(0))
    fstpt %[result];
    )"
    : [result] "=m" (Result)
    : [lhs] "m" (lhs)
    , [rhs] "m" (rhs)
    : "st", "st(1)");

    return Result;
#else
    BIGFLOAT Src1_d = lhs;
    BIGFLOAT Src2_d = rhs;
    BIGFLOAT Tmp = Src2_d * log2l(Src1_d);
    return Tmp;
#endif
  }

  static X80SoftFloat FATAN(X80SoftFloat const &lhs, X80SoftFloat const &rhs) {
    WARN_ONCE_FMT("x87: Application used FATAN which may have accuracy problems");
#ifdef DEBUG_X86_FLOAT
    BIGFLOAT Result;
    asm (R"(
    fninit;
    fldt %[lhs];
    fldt %[rhs];
    fpatan;
    fstpt %[result];
    )"
    : [result] "=m" (Result)
    : [lhs] "m" (lhs)
    , [rhs] "m" (rhs)
    : "st", "st(1)");

    return Result;
#else
    BIGFLOAT Src1_d = lhs;
    BIGFLOAT Src2_d = rhs;
    BIGFLOAT Tmp = atan2l(Src1_d, Src2_d);
    return Tmp;
#endif
  }

  static X80SoftFloat FTAN(X80SoftFloat const &lhs) {
    WARN_ONCE_FMT("x87: Application used FTAN which may have accuracy problems");
#ifdef DEBUG_X86_FLOAT
    BIGFLOAT Result;
    asm (R"(
    fninit;
    fldt %[lhs]; # st0
    fptan;
    ffreep %%st(0);
    fstpt %[result];
    )"
    : [result] "=m" (Result)
    : [lhs] "m" (lhs)
    : "st");

    return Result;
#else
    BIGFLOAT Src_d = lhs;
    Src_d = tanl(Src_d);
    return Src_d;
#endif
  }

  static X80SoftFloat FSIN(X80SoftFloat const &lhs) {
    WARN_ONCE_FMT("x87: Application used FSIN which may have accuracy problems");
#ifdef DEBUG_X86_FLOAT
    BIGFLOAT Result;
    asm (R"(
    fninit;
    fldt %[lhs]; # st0
    fsin;
    fstpt %[result];
    )"
    : [result] "=m" (Result)
    : [lhs] "m" (lhs)
    : "st");

    return Result;
#else
    BIGFLOAT Src_d = lhs;
    Src_d = sinl(Src_d);
    return Src_d;
#endif
  }

  static X80SoftFloat FCOS(X80SoftFloat const &lhs) {
    WARN_ONCE_FMT("x87: Application used FCOS which may have accuracy problems");
#ifdef DEBUG_X86_FLOAT
    BIGFLOAT Result;
    asm (R"(
    fninit;
    fldt %[lhs]; # st0
    fcos;
    fstpt %[result];
    )"
    : [result] "=m" (Result)
    : [lhs] "m" (lhs)
    : "st");

    return Result;
#else
    BIGFLOAT Src_d = lhs;
    Src_d = cosl(Src_d);
    return Src_d;
#endif
  }

  static X80SoftFloat FSQRT(X80SoftFloat const &lhs) {
#ifdef DEBUG_X86_FLOAT
    BIGFLOAT Result;
    asm (R"(
    fninit;
    fldt %[lhs]; # st0
    fsqrt;
    fstpt %[result];
    )"
    : [result] "=m" (Result)
    : [lhs] "m" (lhs)
    : "st");

    return Result;
#else
    return extF80_sqrt(lhs);
#endif
  }

  operator float() const {
    const float32_t Result = extF80_to_f32(*this);
    return FEXCore::BitCast<float>(Result);
  }

  operator double() const {
    const float64_t Result = extF80_to_f64(*this);
    return FEXCore::BitCast<double>(Result);
  }

  operator BIGFLOAT() const {
#if BIGFLOATSIZE == 16
    const float128_t Result = extF80_to_f128(*this);
    return FEXCore::BitCast<BIGFLOAT>(Result);
#else
    BIGFLOAT result{};
    memcpy(&result, this, sizeof(result));
    return result;
#endif
  }

  operator int16_t() const {
    auto rv = extF80_to_i32(*this, softfloat_roundingMode, false);
    if (rv > INT16_MAX) {
      return INT16_MAX;
    } else if (rv < INT16_MIN) {
      return INT16_MIN;
    } else {
      return rv;
    }
  }

  operator int32_t() const {
    return extF80_to_i32(*this, softfloat_roundingMode, false);
  }

  operator int64_t() const {
    return extF80_to_i64(*this, softfloat_roundingMode, false);
  }

  operator uint64_t() const {
    return extF80_to_ui64(*this, softfloat_roundingMode, false);
  }

  void operator=(const float rhs) {
    *this = f32_to_extF80(FEXCore::BitCast<float32_t>(rhs));
  }

  void operator=(const double rhs) {
    *this = f64_to_extF80(FEXCore::BitCast<float64_t>(rhs));
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

  X80SoftFloat(const float rhs) {
    *this = f32_to_extF80(FEXCore::BitCast<float32_t>(rhs));
  }

  X80SoftFloat(const double rhs) {
    *this = f64_to_extF80(FEXCore::BitCast<float64_t>(rhs));
  }

  X80SoftFloat(BIGFLOAT rhs) {
#if BIGFLOATSIZE == 16
    *this = f128_to_extF80(FEXCore::BitCast<float128_t>(rhs));
#else
    *this = FEXCore::BitCast<long double>(rhs);
#endif
  }

  X80SoftFloat(const int16_t rhs) {
    *this = i32_to_extF80(rhs);
  }

  X80SoftFloat(const int32_t rhs) {
    *this = i32_to_extF80(rhs);
  }

  void operator=(extFloat80_t rhs) {
    Significand = rhs.signif;
    Exponent = rhs.signExp & 0x7FFF;
    Sign = rhs.signExp >> 15;
  }

  operator extFloat80_t() const {
    extFloat80_t Result{};
    Result.signif = Significand;
    Result.signExp = Exponent | (Sign << 15);
    return Result;
  }

  static bool IsNan(X80SoftFloat const &lhs) {
    return (lhs.Exponent == 0x7FFF) &&
      (lhs.Significand & IntegerBit) &&
      (lhs.Significand & Bottom62Significand);
  }

  static bool SignBit(X80SoftFloat const &lhs) {
    return lhs.Sign;
  }

private:
  static constexpr uint64_t IntegerBit = (1ULL << 63);
  static constexpr uint64_t Bottom62Significand = ((1ULL << 62) - 1);
  static constexpr uint32_t ExponentBias = 16383;
};

static_assert(sizeof(X80SoftFloat) == 10, "tword must be 10bytes in size");
