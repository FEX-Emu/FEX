// SPDX-License-Identifier: MIT
#pragma once

#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/fextl/sstream.h>
#include <FEXCore/fextl/string.h>
#include <FEXHeaderUtils/BitUtils.h>

#include <cmath>
#include <cstring>
#include <stdint.h>

extern "C" {
#include "SoftFloat-3e/platform.h"
#include "SoftFloat-3e/softfloat.h"
}

struct FEX_PACKED X80SoftFloat {
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
#elif defined(_M_ARM_64)
#define BIGFLOAT long double
#define BIGFLOATSIZE 16
#else
#error No 128bit float for this target!
#endif

#ifndef _WIN32
#define LIBRARY_PRECISION BIGFLOAT
#else
// Mingw Win32 libraries don't have `__float128` helpers. Needs to use a lower precision.
#define LIBRARY_PRECISION double
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
  FEXCORE_PRESERVE_ALL_ATTR static X80SoftFloat FADD(const X80SoftFloat& lhs, const X80SoftFloat& rhs) {
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
    return extF80_add(lhs, rhs);
#endif
  }

  FEXCORE_PRESERVE_ALL_ATTR static X80SoftFloat FSUB(const X80SoftFloat& lhs, const X80SoftFloat& rhs) {
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
    return extF80_sub(lhs, rhs);
#endif
  }

  FEXCORE_PRESERVE_ALL_ATTR static X80SoftFloat FMUL(const X80SoftFloat& lhs, const X80SoftFloat& rhs) {
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
    return extF80_mul(lhs, rhs);
#endif
  }

  FEXCORE_PRESERVE_ALL_ATTR static X80SoftFloat FDIV(const X80SoftFloat& lhs, const X80SoftFloat& rhs) {
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
    return extF80_div(lhs, rhs);
#endif
  }

  FEXCORE_PRESERVE_ALL_ATTR static X80SoftFloat FREM(const X80SoftFloat& lhs, const X80SoftFloat& rhs) {
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
    return extF80_rem(lhs, rhs);
#endif
  }

  FEXCORE_PRESERVE_ALL_ATTR static X80SoftFloat FREM1(const X80SoftFloat& lhs, const X80SoftFloat& rhs) {
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
    return extF80_rem(lhs, rhs);
#endif
  }

  FEXCORE_PRESERVE_ALL_ATTR static X80SoftFloat FRNDINT(const X80SoftFloat& lhs) {
    return extF80_roundToInt(lhs, softfloat_roundingMode, false);
  }

  FEXCORE_PRESERVE_ALL_ATTR static X80SoftFloat FRNDINT(const X80SoftFloat& lhs, uint_fast8_t RoundMode) {
    return extF80_roundToInt(lhs, RoundMode, false);
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
    int32_t TrueExp = lhs.Exponent - ExponentBias;
    return i32_to_extF80(TrueExp);
#endif
  }

  FEXCORE_PRESERVE_ALL_ATTR static void FCMP(const X80SoftFloat& lhs, const X80SoftFloat& rhs, bool* eq, bool* lt, bool* nan) {
    *eq = extF80_eq(lhs, rhs);
    *lt = extF80_lt(lhs, rhs);
    *nan = IsNan(lhs) || IsNan(rhs);
  }

  FEXCORE_PRESERVE_ALL_ATTR static X80SoftFloat FSCALE(const X80SoftFloat& lhs, const X80SoftFloat& rhs) {
    WARN_ONCE_FMT("x87: Application used FSCALE which may have accuracy problems");
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
    X80SoftFloat Int = FRNDINT(rhs, softfloat_round_minMag);
    LIBRARY_PRECISION Src2_d = Int;
    Src2_d = exp2l(Src2_d);
    X80SoftFloat Src2_X80 = Src2_d;
    X80SoftFloat Result = extF80_mul(lhs, Src2_X80);
    return Result;
#endif
  }

  FEXCORE_PRESERVE_ALL_ATTR static X80SoftFloat F2XM1(const X80SoftFloat& lhs) {
    WARN_ONCE_FMT("x87: Application used F2XM1 which may have accuracy problems");
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
    LIBRARY_PRECISION Src1_d = lhs;
    LIBRARY_PRECISION Result = exp2l(Src1_d);
    Result -= 1.0;
    return Result;
#endif
  }

  FEXCORE_PRESERVE_ALL_ATTR static X80SoftFloat FYL2X(const X80SoftFloat& lhs, const X80SoftFloat& rhs) {
    WARN_ONCE_FMT("x87: Application used FYL2X which may have accuracy problems");
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
    LIBRARY_PRECISION Src1_d = lhs;
    LIBRARY_PRECISION Src2_d = rhs;
    LIBRARY_PRECISION Tmp = Src2_d * log2l(Src1_d);
    return Tmp;
#endif
  }

  FEXCORE_PRESERVE_ALL_ATTR static X80SoftFloat FATAN(const X80SoftFloat& lhs, const X80SoftFloat& rhs) {
    WARN_ONCE_FMT("x87: Application used FATAN which may have accuracy problems");
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
    LIBRARY_PRECISION Src1_d = lhs;
    LIBRARY_PRECISION Src2_d = rhs;
    LIBRARY_PRECISION Tmp = atan2l(Src1_d, Src2_d);
    return Tmp;
#endif
  }

  FEXCORE_PRESERVE_ALL_ATTR static X80SoftFloat FTAN(const X80SoftFloat& lhs) {
    WARN_ONCE_FMT("x87: Application used FTAN which may have accuracy problems");
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
    LIBRARY_PRECISION Src_d = lhs;
    Src_d = tanl(Src_d);
    return Src_d;
#endif
  }

  FEXCORE_PRESERVE_ALL_ATTR static X80SoftFloat FSIN(const X80SoftFloat& lhs) {
    WARN_ONCE_FMT("x87: Application used FSIN which may have accuracy problems");
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
    LIBRARY_PRECISION Src_d = lhs;
    Src_d = sinl(Src_d);
    return Src_d;
#endif
  }

  FEXCORE_PRESERVE_ALL_ATTR static X80SoftFloat FCOS(const X80SoftFloat& lhs) {
    WARN_ONCE_FMT("x87: Application used FCOS which may have accuracy problems");
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
    LIBRARY_PRECISION Src_d = lhs;
    Src_d = cosl(Src_d);
    return Src_d;
#endif
  }

  FEXCORE_PRESERVE_ALL_ATTR static X80SoftFloat FSQRT(const X80SoftFloat& lhs) {
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

#ifndef _WIN32
  operator BIGFLOAT() const {
#if BIGFLOATSIZE == 16
    const float128_t Result = extF80_to_f128(*this);
    return FEXCore::BitCast<BIGFLOAT>(Result);
#else
    BIGFLOAT result {};
    memcpy(&result, this, sizeof(result));
    return result;
#endif
  }
#endif

  operator int16_t() const {
    auto rv = extF80_to_i32(*this, softfloat_roundingMode, false);
    if (rv > INT16_MAX || rv < INT16_MIN) {
      ///< Indefinite value for 16-bit conversions.
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

#ifndef _WIN32
  X80SoftFloat(BIGFLOAT rhs) {
#if BIGFLOATSIZE == 16
    *this = f128_to_extF80(FEXCore::BitCast<float128_t>(rhs));
#else
    *this = FEXCore::BitCast<long double>(rhs);
#endif
  }
#endif

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
    extFloat80_t Result {};
    Result.signif = Significand;
    Result.signExp = Exponent | (Sign << 15);
    return Result;
  }

  static bool IsNan(const X80SoftFloat& lhs) {
    return (lhs.Exponent == 0x7FFF) && (lhs.Significand & Bottom62Significand);
  }

  static bool SignBit(const X80SoftFloat& lhs) {
    return lhs.Sign;
  }

private:
  static constexpr uint64_t IntegerBit = (1ULL << 63);
  static constexpr uint64_t Bottom62Significand = ((1ULL << 62) - 1);
  static constexpr uint32_t ExponentBias = 16383;
};

#ifndef _WIN32
static_assert(sizeof(X80SoftFloat) == 10, "tword must be 10bytes in size");
#else
// Padding on this extends to 16-bytes rather than 10-bytes on WIN32.
static_assert(sizeof(X80SoftFloat) == 16, "tword must be 16bytes in size");
#endif
