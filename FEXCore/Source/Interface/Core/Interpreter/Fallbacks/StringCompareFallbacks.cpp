// SPDX-License-Identifier: MIT
#include "Interface/Core/Interpreter/Fallbacks/FallbackOpHandler.h"
#include "Interface/Core/Interpreter/Fallbacks/VectorFallbacks.h"
#include "Interface/IR/IR.h"

#ifdef _M_ARM_64
#include <arm_neon.h>
#endif

#include <cstring>

namespace FEXCore::CPU {
#ifdef _M_ARM_64
FEXCORE_PRESERVE_ALL_ATTR static int32_t GetImplicitLength(const __uint128_t& data, uint16_t control) {
  const auto is_using_words = (control & 1) != 0;

  if (is_using_words) {
    uint16x8_t a {};
    memcpy(&a, &data, sizeof(a));
    uint16x8_t VIndexes {};
    const uint16x8_t VIndex16 = vdupq_n_u16(8);
    uint16_t Indexes[8] = {
      0, 1, 2, 3, 4, 5, 6, 7,
    };
    memcpy(&VIndexes, Indexes, sizeof(VIndexes));
    auto MaskResult = vceqzq_u16(a);
    auto SelectResult = vbslq_u16(MaskResult, VIndexes, VIndex16);
    return vminvq_u16(SelectResult);
  } else {
    uint8x16_t a {};
    memcpy(&a, &data, sizeof(a));
    uint8x16_t VIndexes {};
    const uint8x16_t VIndex16 = vdupq_n_u8(16);
    uint8_t Indexes[16] = {
      0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
    };
    memcpy(&VIndexes, Indexes, sizeof(VIndexes));
    auto MaskResult = vceqzq_u8(a);
    auto SelectResult = vbslq_u8(MaskResult, VIndexes, VIndex16);
    return vminvq_u8(SelectResult);
  }
}
#else
FEXCORE_PRESERVE_ALL_ATTR static int32_t GetImplicitLength(const __uint128_t& data, uint16_t control) {
  const auto* data_u8 = reinterpret_cast<const uint8_t*>(&data);
  const auto is_using_words = (control & 1) != 0;

  int32_t length = 0;

  if (is_using_words) {
    const auto get_word = [data_u8](int32_t index) {
      const auto* src = data_u8 + (index * sizeof(uint16_t));

      uint16_t element {};
      std::memcpy(&element, src, sizeof(uint16_t));
      return element;
    };

    while (length < 8 && get_word(length) != 0) {
      length++;
    }
  } else {
    while (length < 16 && data_u8[length] != 0) {
      length++;
    }
  }

  return length;
}
#endif

// Essentially the same in terms of behavior with VPCMPESTRX instructions,
// with the only difference being that the length of the string is encoded
// as part of the data vectors passed in.
//
// i.e. Length is determined by the presence of a NUL (all-zero) character
//      within the data.
//
//      If no NUL character exists, then the length of the strings are assumed
//      to be the max length possible for the given character size specified
//      in the control flags (16 characters for 8-bit, and 8 characters for 16-bit).
//
FEXCORE_PRESERVE_ALL_ATTR uint32_t OpHandlers<IR::OP_VPCMPISTRX>::handle(__uint128_t lhs, __uint128_t rhs, uint16_t control) {
  // Subtract by 1 in order to make validity limits 0-based
  const auto valid_lhs = GetImplicitLength(lhs, control) - 1;
  const auto valid_rhs = GetImplicitLength(rhs, control) - 1;

  return OpHandlers<IR::OP_VPCMPESTRX>::MainBody(lhs, valid_lhs, rhs, valid_rhs, control);
}

} // namespace FEXCore::CPU
