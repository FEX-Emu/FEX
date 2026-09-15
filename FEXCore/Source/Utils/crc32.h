// SPDX-License-Identifier: MIT
#pragma once
#include <cstddef>
#include <cstdint>
#if defined(ARCHITECTURE_arm64)
#include <arm_acle.h>
#elif defined(ARCHITECTURE_x86_64)
#include <nmmintrin.h>
#endif

namespace FEXCore::Utils {
template<typename T>
static inline uint32_t crc32(const T* Ptr, size_t Size) {
  uint32_t Result {};
#if defined(ARCHITECTURE_arm64)
#define do_crc(type, suffix)                                               \
  while (Size >= sizeof(type)) {                                           \
    Result = __crc32##suffix(Result, *reinterpret_cast<const type*>(Ptr)); \
    Ptr += sizeof(type);                                                   \
    Size -= sizeof(type);                                                  \
  }
  do_crc(uint64_t, d);
  do_crc(uint32_t, w);
  do_crc(uint16_t, h);
  do_crc(uint8_t, b);

#elif defined(ARCHITECTURE_x86_64)
#define do_crc(type, intrinsic)                                      \
  while (Size >= sizeof(type)) {                                     \
    Result = intrinsic(Result, *reinterpret_cast<const type*>(Ptr)); \
    Ptr += sizeof(type);                                             \
    Size -= sizeof(type);                                            \
  }

  do_crc(uint64_t, _mm_crc32_u64);
  do_crc(uint32_t, _mm_crc32_u32);
  do_crc(uint16_t, _mm_crc32_u16);
  do_crc(uint8_t, _mm_crc32_u8);
#else
  // Unsupported on non-arm.
  return 0;
#endif
#undef do_crc
  return Result;
}
} // namespace FEXCore::Utils
