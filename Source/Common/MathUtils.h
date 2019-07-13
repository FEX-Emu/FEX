#pragma once

#include <stdint.h>

[[maybe_unused]] inline uint64_t AlignUp(uint64_t value, uint64_t size) {
  return value + (size - value % size) % size;
};

[[maybe_unused]] inline uint64_t AlignDown(uint64_t value, uint64_t size) {
  return value - value % size;
};


