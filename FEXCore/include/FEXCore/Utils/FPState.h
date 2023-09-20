// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>

namespace FEXCore::FPState {
  enum class X87Tag : uint8_t {
    Valid   = 0b00,
    Zero    = 0b01,
    Special = 0b10,
    Empty   = 0b11
  };

  static inline X87Tag GetX87Tag(uint64_t (&Reg)[2], bool Valid) {
    if (!Valid) {
      return X87Tag::Empty;
    }

    const uint64_t Exponent = Reg[1] & 0x7fff;
    if (Exponent == 0x7fff) {
      // (Pseudo) NaN / Inf
      return X87Tag::Special;
    }

    const bool JBit = Reg[0] & (1ULL << 63);
    if (Exponent == 0) {
      const uint64_t Fraction = Reg[0] & ((1ULL << 63) - 1);
      if (!JBit && !Fraction) {
        return X87Tag::Zero;
      } else {
        // (Pseudo) Subnormal
        return X87Tag::Special;
      }
    }

    if (JBit) {
      // Normal
      return X87Tag::Valid;
    } else {
      // Invalid
      return X87Tag::Special;
    }
  }

  static inline uint16_t ConvertFromAbridgedFTW(uint16_t FSW, uint64_t (&MM)[8][2], uint8_t AbridgedFTW) {
    const uint32_t StackTop = (FSW >> 11) & 0b111;

    uint16_t FTW = 0;
    for (uint32_t i = 0; i < 8; i++) {
      // The AMD manually incorrectly states there is a direct mapping here, only the intel-manual correctly states
      // the stack-relative behaviour
      const uint16_t StackIndex = (i - StackTop) & 0b111;
      const X87Tag Tag = GetX87Tag(MM[StackIndex], AbridgedFTW & (1 << i));
      FTW |= static_cast<uint8_t>(Tag) << (2 * i);
    }

    return FTW;
  }

  static inline uint8_t ConvertToAbridgedFTW(uint16_t FTW) {
    uint8_t AbridgedFTW = 0;

    for (uint32_t i = 0; i < 8; i++) {
      const X87Tag Tag = static_cast<X87Tag>((FTW >> (2 * i)) & 3);
      AbridgedFTW |= ((Tag == X87Tag::Empty) ? 0 : 1) << i;
    }

    return AbridgedFTW;
  }
}
