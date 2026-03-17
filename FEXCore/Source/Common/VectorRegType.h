// SPDX-License-Identifier: MIT
#pragma once

#ifdef ARCHITECTURE_x86_64
#include <xmmintrin.h>
#include <immintrin.h>
#else
#include <cstdint>
#endif

namespace FEXCore {
struct VectorScalarF64Pair {
  double val[2];
};

#ifdef ARCHITECTURE_arm64
// Can't use uint8x16_t directly from arm_neon.h here.
// Overrides softfloat-3e's defines which causes problems.
#ifdef __clang__
using VectorRegType = __attribute__((neon_vector_type(16))) uint8_t;
#else
using VectorRegType = __attribute__((vector_size(16))) uint8_t;
#endif
struct VectorRegPairType {
  VectorRegType val[2];
};

static inline VectorRegPairType MakeVectorRegPair(VectorRegType low, VectorRegType high) {
  return VectorRegPairType {low, high};
}

#elif defined(ARCHITECTURE_x86_64)
using VectorRegType = __m128i;
using VectorRegPairType = __m256i;

static inline VectorRegPairType MakeVectorRegPair(VectorRegType low, VectorRegType high) {
  return _mm256_set_m128i(high, low);
}
#endif
} // namespace FEXCore
