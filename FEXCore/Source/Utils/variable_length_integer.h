// SPDX-License-Identifier: MIT
#pragma once

#include <FEXCore/Utils/CompilerDefs.h>

#include <cstdio>
#include <cstdint>
#include <cstddef>
#include <limits>

namespace FEXCore::Utils {
// Variable length signed integer
// The most common encoded size is 8-bit positive, but other values can occur
//
// 8-bit:
// bit[7] = 0 - 8-bit
// bit[6:0] = 7-bit encoding
//
// 16-bit:
// byte1[7:6] = 0b10 - 16-bit
// byte1[5:0] = top 6-bits
// byte2[7:0] = Bottom 8-bits bits
//
// 32-bit
// byte1[7:5] = 0b110 - 32-bit
// byte1[4:0] = <reserved>
// word[31:0] = signed word
//
// 64-bit
// byte1[7:5] = 0b111 - 64-bit
// byte1[4:0] = <reserved>
// dword[63:0] = signed dword
struct vl64 final {
  static size_t EncodedSize(int64_t Data) {
    if (Data >= vl8_min && Data <= vl8_max) {
      return sizeof(vl8_enc);
    } else if (Data >= vl16_min && Data <= vl16_max) {
      return sizeof(vl16_enc);
    } else if (Data >= vl32_min && Data <= vl32_max) {
      return sizeof(vl32_enc);
    }
    return sizeof(vl64_enc);
  }

  struct Decoded {
    int64_t Integer;
    size_t Size;
  };

  static Decoded Decode(const uint8_t* data) {
    auto vl8_type = reinterpret_cast<const vl8_enc*>(data);
    auto vl16_type = reinterpret_cast<const vl16_enc*>(data);
    auto vl32_type = reinterpret_cast<const vl32_enc*>(data);
    auto vl64_type = reinterpret_cast<const vl64_enc*>(data);

    if (vl8_type->Type == vl8_type_header) {
      return {vl8_type->Integer, sizeof(vl8_enc)};
    } else if (vl16_type->HighBits.Type == vl16_type_header) {
      return {vl16_type->Integer(), sizeof(vl16_enc)};
    } else if (vl32_type->Type == vl32_type_header) {
      return {vl32_type->Integer, sizeof(vl32_enc)};
    }
    return {vl64_type->Integer, sizeof(vl64_enc)};
  }

  static size_t Encode(uint8_t* dst, int64_t Data) {
    auto vl8_type = reinterpret_cast<vl8_enc*>(dst);
    auto vl16_type = reinterpret_cast<vl16_enc*>(dst);
    auto vl32_type = reinterpret_cast<vl32_enc*>(dst);
    auto vl64_type = reinterpret_cast<vl64_enc*>(dst);

    if (Data >= vl8_min && Data <= vl8_max) {
      *vl8_type = {
        .Integer = static_cast<int8_t>(Data),
        .Type = vl8_type_header,
      };
      return sizeof(vl8_enc);
    } else if (Data >= vl16_min && Data <= vl16_max) {
      *vl16_type = {
        .HighBits {
          .Top = static_cast<int8_t>((Data >> 8) & 0xFF),
          .Type = vl16_type_header,
        },
        .LowBits = static_cast<uint8_t>(Data & 0xFF),
      };
      return sizeof(vl16_enc);
    } else if (Data >= vl32_min && Data <= vl32_max) {
      *vl32_type = {
        .Type = vl32_type_header,
        .Integer = static_cast<int32_t>(Data),
      };
      return sizeof(vl32_enc);
    }

    *vl64_type = {
      .Type = vl64_type_header,
      .Integer = Data,
    };
    return sizeof(vl64_enc);
  }

private:

  struct vl8_enc {
    int8_t Integer : 7;
    uint8_t Type   : 1;
  };
  static_assert(sizeof(vl8_enc) == 1);

  struct vl16_enc {
    struct {
      int8_t Top   : 6;
      uint8_t Type : 2;
    } HighBits;
    uint8_t LowBits;

    int64_t Integer() const {
      int16_t Value {};
      Value |= (HighBits.Top << 8);
      Value |= LowBits;
      return (Value << 2) >> 2;
    }
  };
  static_assert(sizeof(vl16_enc) == 2);

  struct FEX_PACKED vl32_enc {
    uint8_t Type;
    int32_t Integer;
  };
  static_assert(sizeof(vl32_enc) == 5);

  struct FEX_PACKED vl64_enc {
    uint8_t Type;
    int64_t Integer;
  };
  static_assert(sizeof(vl64_enc) == 9);

  // Maximum ranges for encodings.

  // vl8 can hold a signed 7-bit integer.
  // Encoded in one 8-bit value.
  constexpr static int64_t vl8_encoded_bits = 7;
  constexpr static int64_t vl8_type_header = 0;
  constexpr static int64_t vl8_min = std::numeric_limits<int64_t>::min() >> ((sizeof(int64_t) * 8) - vl8_encoded_bits);
  constexpr static int64_t vl8_max = std::numeric_limits<int64_t>::max() >> ((sizeof(int64_t) * 8) - vl8_encoded_bits);

  // vl16 can hold a signed 14-bit integer.
  // Encoded in one 16-bit value.
  constexpr static int64_t vl16_encoded_bits = 14;
  constexpr static int64_t vl16_type_header = 0b10;
  constexpr static int64_t vl16_min = std::numeric_limits<int64_t>::min() >> ((sizeof(int64_t) * 8) - vl16_encoded_bits);
  constexpr static int64_t vl16_max = std::numeric_limits<int64_t>::max() >> ((sizeof(int64_t) * 8) - vl16_encoded_bits);

  // vl32 can hold a signed 32-bit integer.
  // Encoded in 8-bit and 32-bit value;
  constexpr static int64_t vl32_encoded_bits = 32;
  constexpr static int64_t vl32_type_header = 0b1100'0000;
  constexpr static int64_t vl32_min = std::numeric_limits<int32_t>::min();
  constexpr static int64_t vl32_max = std::numeric_limits<int32_t>::max();

  // vl64 can hold a signed 32-bit integer.
  // Encoded in 8-bit and 64-bit value.
  constexpr static int64_t vl64_encoded_bits = 64;
  constexpr static int64_t vl64_type_header = 0b1110'0000;
  constexpr static int64_t vl64_min = std::numeric_limits<int64_t>::min();
  constexpr static int64_t vl64_max = std::numeric_limits<int64_t>::max();
};

} // namespace FEXCore::Utils
