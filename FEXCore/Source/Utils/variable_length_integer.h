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

// Variable length pair that optimizes around FEXCore's JITRIPReconstruction.
//
// 8-bit:
// bit[7]   = 0 - 8-bit
// bit[6:4] = 3-bit unsigned - 1. [1 - 8] range.
// bit[3:0] = 4-bit unsigned divided by 4 - 1. [4 - 64 byte] range.
//
// 16-bit:
// byte1[7:6] = 0b10 - 16-bit
// byte1[5:0] = 6-bit signed value [-32 - 31] range
// byte2[7:0] = 8-bit signed value divided by 4. [-512 - 508] byte range.
//
// 32-bit and 64-bit don't attempt to do any compression beyond range checks.
// 32-bit
// byte1[7:5] = 0b110 - 32-bit
// byte1[4:0] = <reserved>
// word1[31:0] = signed word
// word2[31:0] = signed word
//
// 64-bit
// byte1[7:5] = 0b111 - 64-bit
// byte1[4:0] = <reserved>
// dword1[63:0] = signed dword
// dword2[63:0] = signed dword

struct vl64pair final {
public:
  static size_t EncodedSize(uint64_t data_arm, uint64_t data_rip) {
    if (can_encode_vl8(data_arm, data_rip)) {
      return sizeof(vl8_enc);
    } else if (can_encode_vl16(data_arm, data_rip)) {
      return sizeof(vl16_enc);
    } else if (can_encode_vl32(data_arm, data_rip)) {
      return sizeof(vl32_enc);
    }
    return sizeof(vl64_enc);
  }

  struct Decoded {
    uint64_t IntegerARMPC;
    uint64_t IntegerX86RIP;
    size_t Size;
  };

  static Decoded Decode(const uint8_t* data) {
    auto vl8_type = reinterpret_cast<const vl8_enc*>(data);
    auto vl16_type = reinterpret_cast<const vl16_enc*>(data);
    auto vl32_type = reinterpret_cast<const vl32_enc*>(data);
    auto vl64_type = reinterpret_cast<const vl64_enc*>(data);

    if (vl8_type->Type == vl8_type_header) {
      return Decode(vl8_type);
    } else if (vl16_type->HighBits.Type == vl16_type_header) {
      return Decode(vl16_type);
    } else if (vl32_type->Type == vl32_type_header) {
      return Decode(vl32_type);
    }
    return {vl64_type->IntegerARMPC, vl64_type->IntegerX86RIP, sizeof(vl64_enc)};
  }

  static size_t Encode(uint8_t* dst, uint64_t data_arm, uint64_t data_rip) {
    auto vl8_type = reinterpret_cast<vl8_enc*>(dst);
    auto vl16_type = reinterpret_cast<vl16_enc*>(dst);
    auto vl32_type = reinterpret_cast<vl32_enc*>(dst);
    auto vl64_type = reinterpret_cast<vl64_enc*>(dst);

    if (can_encode_vl8(data_arm, data_rip)) {
      *vl8_type = {
        .IntegerARMPC = static_cast<uint8_t>((data_arm - 1) >> vl8_arm_align_bits),
        .IntegerX86RIP = static_cast<uint8_t>(data_rip - 1),
        .Type = vl8_type_header,
      };
      return sizeof(vl8_enc);
    } else if (can_encode_vl16(data_arm, data_rip)) {
      *vl16_type = {
        .HighBits {
          .IntegerX86RIP = static_cast<int8_t>(static_cast<int64_t>(data_rip)),
          .Type = vl16_type_header,
        },
        .IntegerARMPC = static_cast<int8_t>(static_cast<int64_t>(data_arm) >> vl8_arm_align_bits),
      };
      return sizeof(vl16_enc);
    } else if (can_encode_vl32(data_arm, data_rip)) {
      *vl32_type = {
        .Type = vl32_type_header,
        .IntegerARMPC = static_cast<int32_t>(data_arm),
        .IntegerX86RIP = static_cast<int32_t>(data_rip),
      };
      return sizeof(vl32_enc);
    }

    *vl64_type = {
      .Type = vl64_type_header,
      .IntegerARMPC = data_arm,
      .IntegerX86RIP = data_rip,
    };
    return sizeof(vl64_enc);
  }

private:
  struct vl8_enc {
    uint8_t IntegerARMPC  : 4;
    uint8_t IntegerX86RIP : 3;
    uint8_t Type          : 1;
  };
  static_assert(sizeof(vl8_enc) == 1);

  static inline Decoded Decode(const vl8_enc* enc) {
    const uint64_t data_arm = enc->IntegerARMPC;
    const uint64_t data_rip = enc->IntegerX86RIP;
    return {(data_arm + 1) << vl8_arm_align_bits, data_rip + 1, sizeof(vl8_enc)};
  }

  struct vl16_enc {
    struct {
      int8_t IntegerX86RIP : 6;
      uint8_t Type         : 2;
    } HighBits;
    int8_t IntegerARMPC;
  };
  static_assert(sizeof(vl16_enc) == 2);

  static inline Decoded Decode(const vl16_enc* enc) {
    int64_t arm_pc = enc->IntegerARMPC << vl8_arm_align_bits;
    int64_t x86_rip = enc->HighBits.IntegerX86RIP;
    return {static_cast<uint64_t>(arm_pc), static_cast<uint64_t>(x86_rip), sizeof(vl16_enc)};
  }

  struct FEX_PACKED vl32_enc {
    uint8_t Type;
    int32_t IntegerARMPC;
    int32_t IntegerX86RIP;
  };
  static_assert(sizeof(vl32_enc) == 9);

  static inline Decoded Decode(const vl32_enc* enc) {
    int64_t arm_pc = enc->IntegerARMPC;
    int64_t x86_rip = enc->IntegerX86RIP;
    return {static_cast<uint64_t>(arm_pc), static_cast<uint64_t>(x86_rip), sizeof(vl32_enc)};
  }

  struct FEX_PACKED vl64_enc {
    uint8_t Type;
    uint64_t IntegerARMPC;
    uint64_t IntegerX86RIP;
  };
  static_assert(sizeof(vl64_enc) == 17);

  // vl8 can hold a two small unsigned integers.
  // Encoded in 8-bit.
  constexpr static int64_t vl8_type_header = 0;
  constexpr static int64_t vl8_arm_min = 1;
  constexpr static int64_t vl8_arm_max = 16;
  constexpr static int64_t vl8_arm_align_bits = 2;
  constexpr static int64_t vl8_arm_shift_mask = (1U << vl8_arm_align_bits) - 1;
  constexpr static int64_t vl8_pc_min = 1;
  constexpr static int64_t vl8_pc_max = 8;
  static bool can_encode_vl8(uint64_t data_arm, uint64_t data_rip) {
    // GuestPC can only be [1,8] bytes.
    if (data_rip < vl8_pc_min || data_rip > vl8_pc_max) {
      return false;
    }
    // Unaligned doesn't fit at all.
    if (data_arm & vl8_arm_shift_mask) {
      return false;
    }

    // HostPC can only be [1,16] instructions.
    int64_t ShiftedHostPC = data_arm >> vl8_arm_align_bits;
    if (ShiftedHostPC < vl8_arm_min || ShiftedHostPC > vl8_arm_max) {
      return false;
    }

    return true;
  }

  // vl16 can hold a two small signed integers
  // Encoded in one 16-bit value.
  constexpr static int64_t vl16_type_header = 0b10;
  constexpr static int64_t vl16_arm_min = -128;
  constexpr static int64_t vl16_arm_max = 127;
  constexpr static int64_t vl16_arm_align_bits = 2;
  constexpr static int64_t vl16_arm_shift_mask = (1U << vl16_arm_align_bits) - 1;
  constexpr static int64_t vl16_pc_min = -32;
  constexpr static int64_t vl16_pc_max = 31;
  static bool can_encode_vl16(int64_t data_arm, int64_t data_rip) {
    // GuestPC can only be [-32,31] bytes.
    if (data_rip < vl16_pc_min || data_rip > vl16_pc_max) {
      return false;
    }

    // Unaligned doesn't fit at all.
    if (data_arm & vl16_arm_shift_mask) {
      return false;
    }

    // HostPC can only be [-128,127] instructions.
    int64_t ShiftedHostPC = data_arm >> vl16_arm_align_bits;
    if (ShiftedHostPC < vl16_arm_min || ShiftedHostPC > vl16_arm_max) {
      return false;
    }

    return true;
  }

  // vl32 can hold a two 32-bit integers.
  // Encoded in 8-bit and two 32-bit values.
  constexpr static int64_t vl32_type_header = 0b1100'0000;
  constexpr static int64_t vl32_min = std::numeric_limits<int32_t>::min();
  constexpr static int64_t vl32_max = std::numeric_limits<int32_t>::max();
  static bool can_encode_vl32(int64_t data_arm, int64_t data_rip) {
    if (data_rip < vl32_min || data_rip > vl32_max) {
      return false;
    }
    if (data_arm < vl32_min || data_arm > vl32_max) {
      return false;
    }

    return true;
  }

  // vl64 can hold a two 64-bit integers.
  // Encoded in 8-bit and two 64-bit values.
  constexpr static int64_t vl64_type_header = 0b1110'0000;
};

} // namespace FEXCore::Utils
