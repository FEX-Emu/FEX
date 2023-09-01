#pragma once

#include <cstdint>

namespace FEXCore::FPState {
  enum class X87Tag : uint8_t {
    Valid   = 0b00,
    Zero    = 0b01,
    Special = 0b10,
    Empty   = 0b11
  };
}
