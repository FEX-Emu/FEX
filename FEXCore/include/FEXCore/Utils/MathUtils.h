// SPDX-License-Identifier: MIT
#pragma once

#include <FEXCore/Utils/LogManager.h>

#include <bit>
#include <cstdint>
#include <type_traits>

namespace FEXCore {
[[nodiscard]]
constexpr uint64_t AlignUp(uint64_t value, uint64_t size) {
  return value + (size - value % size) % size;
}

[[nodiscard]]
constexpr uint64_t AlignDown(uint64_t value, uint64_t size) {
  return value - value % size;
}

// Returns the ilog2 of a power-of-2 integer.
// Asserts in the case that the passed in integer is not a power-of-2.
template<typename T>
requires (std::is_unsigned_v<T>)
[[nodiscard]]
constexpr T ilog2(T Value) {
  LOGMAN_THROW_A_FMT(std::has_single_bit(Value), "ilog2 requires popcount to be one");
  return std::countr_zero(Value);
}

// Divide a number by a power-of-2 by avoiding integer division.
// Can be a faster implementation than regular integer divide.
// Divisor requires to be power-of-2, is enforced in ilog2 helper.
template<typename T, typename TT>
requires (std::is_unsigned_v<T> && std::is_unsigned_v<TT>)
[[nodiscard]]
constexpr T DividePow2(T Dividend, TT Divisor) {
  return Dividend >> ilog2(Divisor);
}
} // namespace FEXCore
