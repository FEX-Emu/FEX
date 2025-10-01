// SPDX-License-Identifier: MIT
#pragma once

// Header for various utilities that operate on bits and bytes.

#include <bit>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace FEXCore {

// Determines the number of bits inside of a given type.
template<typename T>
[[nodiscard]]
constexpr size_t BitSize() noexcept {
  return sizeof(T) * CHAR_BIT;
}

// Swaps the bytes of a 16-bit unsigned value.
[[nodiscard]]
inline uint16_t BSwap16(uint16_t value) noexcept {
#ifdef __GNUC__
  return __builtin_bswap16(value);
#else
  return (value >> 8) | (value << 8);
#endif
}

// Swaps the bytes of a 32-bit unsigned value.
[[nodiscard]]
inline uint32_t BSwap32(uint32_t value) noexcept {
#ifdef __GNUC__
  return __builtin_bswap32(value);
#else
  return ((value & 0xFF000000U) >> 24) | ((value & 0x00FF0000U) >> 8) | ((value & 0x0000FF00U) << 8) | ((value & 0x000000FFU) << 24);
#endif
}

// Swaps the bytes of a 64-bit unsigned value.
[[nodiscard]]
inline uint64_t BSwap64(uint64_t value) noexcept {
#ifdef __GNUC__
  return __builtin_bswap64(value);
#else
  return ((value & 0xFF00000000000000ULL) >> 56) | ((value & 0x00FF000000000000ULL) >> 40) | ((value & 0x0000FF0000000000ULL) >> 24) |
         ((value & 0x000000FF00000000ULL) >> 8) | ((value & 0x00000000FF000000ULL) << 8) | ((value & 0x0000000000FF0000ULL) << 24) |
         ((value & 0x000000000000FF00ULL) << 40) | ((value & 0x00000000000000FFULL) << 56);
#endif
}

// Finds the first least-significant set bit within a given value.
// Note that all returned indices are 1-based, not 0-based.
template<typename T>
[[nodiscard]]
constexpr int FindFirstSetBit(T value) noexcept {
  static_assert(std::is_unsigned_v<T>, "Type must be unsigned.");

  if (value == 0) {
    return 0;
  }

  const int trailing_zeroes = std::countr_zero(value);
  return trailing_zeroes + 1;
}

} // namespace FEXCore
