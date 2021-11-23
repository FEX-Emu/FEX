#pragma once

#include <cstdint>

namespace FEXCore {
[[nodiscard]] constexpr uint64_t AlignUp(uint64_t value, uint64_t size) {
  return value + (size - value % size) % size;
}

[[nodiscard]] constexpr uint64_t AlignDown(uint64_t value, uint64_t size) {
  return value - value % size;
}
} // namespace FEXCore
