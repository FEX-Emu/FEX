#pragma once

#include <stdint.h>

static inline uint64_t AlignUp(uint64_t value, uint64_t size) {
  return value + (size - value % size) % size;
};

static inline uint64_t AlignDown(uint64_t value, uint64_t size) {
  return value - value % size;
};


