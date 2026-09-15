// SPDX-License-Identifier: MIT
#pragma once
#include <cstddef>
#include <cstdint>
#if defined(ARCHITECTURE_arm64)
#include <arm_acle.h>
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
#undef do_crc
#else
  // Emulate arm64 crc32.
  // This is basically just the pseudo-code for crc32b.
  // Doesn't need to be fast, just needs to match.
  auto reverse_bits = [](auto bits) {
    decltype(bits) Result {};
    for (size_t i = 0; i < (sizeof(decltype(bits)) * 8); ++i) {
      Result = (Result << 1) | ((bits >> i) & 1);
    }
    return Result;
  };

  auto Poly32Mod2 = [](uint64_t data) -> uint32_t {
    constexpr static size_t bits = 40;
    constexpr static uint64_t poly = 0x04C11DB7U;
    for (size_t i = (bits - 1); i >= 32; --i) {
      if (((data >> i) & 1) != 0) {
        const uint64_t poly_shift = poly << (i - 32);
        const uint64_t data_mask = (1ULL << i) - 1;
        data = (data & data_mask) ^ poly_shift;
      }
    }

    return data;
  };

  for (size_t i = 0; i < Size; ++i) {
    uint64_t TempAcc = static_cast<uint64_t>(reverse_bits(Result)) << 8;
    uint64_t TempVal = static_cast<uint64_t>(reverse_bits(reinterpret_cast<const uint8_t*>(Ptr)[i])) << 32;
    Result = reverse_bits(Poly32Mod2(TempAcc ^ TempVal));
  }
#endif
  return Result;
}
} // namespace FEXCore::Utils
