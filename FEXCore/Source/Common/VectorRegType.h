// SPDX-License-Identifier: MIT
#pragma once

#ifdef _M_X86_64
#include <xmmintrin.h>
#endif

namespace FEXCore {
#ifdef _M_ARM_64
// Can't use uint8x16_t directly from arm_neon.h here.
// Overrides softfloat-3e's defines which causes problems.
using VectorRegType = __attribute__((neon_vector_type(16))) uint8_t;
#elif defined(_M_X86_64)
using VectorRegType = __m128i;
#endif
} // namespace FEXCore
