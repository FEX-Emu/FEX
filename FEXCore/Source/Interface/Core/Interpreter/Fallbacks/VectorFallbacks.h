#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "Interface/Core/Interpreter/Fallbacks/FallbackOpHandler.h"
#include "Interface/IR/IR.h"
#include "Common/VectorRegType.h"

namespace FEXCore::CPU {

template<>
struct OpHandlers<IR::OP_VPCMPESTRX> {
  enum class AggregationOp {
    EqualAny = 0b00,
    Ranges = 0b01,
    EqualEach = 0b10,
    EqualOrdered = 0b11,
  };

  enum class SourceData {
    U8,
    U16,
    S8,
    S16,
  };

  enum class Polarity {
    Positive,
    Negative,
    PositiveMasked,
    NegativeMasked,
  };

  FEXCORE_PRESERVE_ALL_ATTR static uint32_t handle(uint64_t RAX, uint64_t RDX, __uint128_t lhs, __uint128_t rhs, uint16_t control) {
    // Subtract by 1 in order to make validity limits 0-based
    const auto valid_lhs = GetExplicitLength(RAX, control) - 1;
    const auto valid_rhs = GetExplicitLength(RDX, control) - 1;

    return MainBody(lhs, valid_lhs, rhs, valid_rhs, control);
  }

  // Main PCMPXSTRX algorithm body. Allows for reuse with both implicit and explicit length variants.
  FEXCORE_PRESERVE_ALL_ATTR static uint32_t MainBody(const __uint128_t& lhs, int valid_lhs, const __uint128_t& rhs, int valid_rhs, uint16_t control) {
    const uint32_t aggregation = PerformAggregation(lhs, valid_lhs, rhs, valid_rhs, control);
    const int32_t upper_limit = (16 >> (control & 1)) - 1;

    // Bits are arranged as:
    // Bit #:   3    2    1    0
    //         [SF | ZF | CF | OF]
    uint32_t flags = 0;
    flags |= (valid_rhs < upper_limit) ? 0b0100 : 0b0000;
    flags |= (valid_lhs < upper_limit) ? 0b1000 : 0b0000;

    const uint32_t result = HandlePolarity(aggregation, control, upper_limit, valid_rhs);
    if (result != 0) {
      flags |= 0b0010;
    }
    if ((result & 1) != 0) {
      flags |= 0b0001;
    }

    // We track the flags in the usual NZCV bit position so we can msr them
    // later. Avoids handling flags natively in JIT.
    return result | (flags << 28);
  }

  FEXCORE_PRESERVE_ALL_ATTR static int32_t GetExplicitLength(uint64_t reg, uint16_t control) {
    // Bit 8 controls whether or not the reg value is 64-bit or 32-bit.
    int64_t value = 0;
    if (((control >> 8) & 1) != 0) {
      value = static_cast<int64_t>(reg);
    } else {
      // We need a sign extend in this case.
      value = static_cast<int32_t>(reg);
    }

    // If control[0] is set, then we're dealing with words instead of bytes
    const int64_t limit = (control & 1) != 0 ? 8 : 16;

    // Length needs to saturate to 16 (if bytes) or 8 (if words)
    // when the length value is greater than 16 (if bytes)/8 (if words)
    // or if the length value is less than -16 (if bytes)/-8 (if words).
    if (value < -limit || value > limit) {
      return limit;
    }

    return std::abs(static_cast<int>(value));
  }

  FEXCORE_PRESERVE_ALL_ATTR static int32_t GetElement(const __uint128_t& vec, int32_t index, uint16_t control) {
    const auto* vec_ptr = reinterpret_cast<const uint8_t*>(&vec);

    // Control bits [1:0] define the data type being dealt with.
    switch (static_cast<SourceData>(control & 0b11)) {
    case SourceData::U8: return static_cast<int32_t>(vec_ptr[index]);
    case SourceData::U16: {
      uint16_t value {};
      std::memcpy(&value, vec_ptr + (sizeof(uint16_t) * static_cast<size_t>(index)), sizeof(value));
      return value;
    }
    case SourceData::S8: return static_cast<int8_t>(vec_ptr[index]);
    case SourceData::S16:
    default: {
      int16_t value {};
      std::memcpy(&value, vec_ptr + (sizeof(int16_t) * static_cast<size_t>(index)), sizeof(value));
      return value;
    }
    }
  }

  FEXCORE_PRESERVE_ALL_ATTR static uint32_t
  PerformAggregation(const __uint128_t& lhs, int32_t valid_lhs, const __uint128_t& rhs, int32_t valid_rhs, uint16_t control) {
    switch (static_cast<AggregationOp>((control >> 2) & 0b11)) {
    case AggregationOp::EqualAny: return HandleEqualAny(lhs, valid_lhs, rhs, valid_rhs, control);
    case AggregationOp::Ranges: return HandleRanges(lhs, valid_lhs, rhs, valid_rhs, control);
    case AggregationOp::EqualEach: return HandleEqualEach(lhs, valid_lhs, rhs, valid_rhs, control);
    case AggregationOp::EqualOrdered:
    default: return HandleEqualOrdered(lhs, valid_lhs, rhs, valid_rhs, control);
    }
  }

  FEXCORE_PRESERVE_ALL_ATTR static uint32_t HandlePolarity(uint32_t value, uint16_t control, int upper_limit, int valid_rhs) {
    switch (static_cast<Polarity>((control >> 4) & 0b11)) {
    case Polarity::Negative: return value ^ ((2U << upper_limit) - 1);
    case Polarity::NegativeMasked: return value ^ ((1U << (valid_rhs + 1)) - 1);
    case Polarity::Positive:
    case Polarity::PositiveMasked:
    default:
      // Both positive masking and positive polarity are documented
      // as both being equivalent to "IntRes2 = IntRes1", where IntRes1
      // is our 'value' parameter, so we don't need to do anything in
      // these cases except return the same value.
      return value;
    }
  }

  // Finds characters from an overall character set.
  //
  // Scans through RHS trying to find any characters contained in LHS.
  // Think of this as a sort of vectorized version of strspn (kind of).
  //
  // e.g. Assume operating on two character vectors as unsigned words
  //
  //         0  1  2  3  4  5  6  7
  // LHS -> [a, b, c, d, e, f, g, n]
  // RHS -> [z, k, v, c, d, o, p, n]
  //
  // With both explicit lengths for each string being 8 (the max length for words),
  // this would result in an intermediate result like:
  //
  //            0b1001'1000
  //              │  │ │
  // 'n' match ───┘  │ │
  //                 │ │
  // 'd' match ──────┘ │
  //                   │
  // 'c' match ────────┘
  //
  FEXCORE_PRESERVE_ALL_ATTR static uint32_t
  HandleEqualAny(const __uint128_t& lhs, int32_t valid_lhs, const __uint128_t& rhs, int32_t valid_rhs, uint16_t control) {
    uint32_t result = 0;

    for (int j = valid_rhs; j >= 0; j--) {
      result <<= 1;

      const int rhs_value = GetElement(rhs, j, control);
      for (int i = valid_lhs; i >= 0; i--) {
        const int lhs_value = GetElement(lhs, i, control);
        result |= static_cast<uint32_t>(rhs_value == lhs_value);
      }
    }

    return result;
  }

  // Determines if a character falls within a limited range
  //
  // Scans through rhs using a range denoted by two elements
  // in lhs and determines if the respective character in rhs
  // falls within its range.
  //
  // i.e.
  //      lhs_upper_bound >= rhs_value && lhs_lower_bound <= rhs_value
  //
  // e.g. Assume operating on two character vectors as unsigned words
  //
  //         0  1  2  3  4  5  6  7
  // LHS -> [a, z, A, Z, 0, 0, 0, 0]
  // RHS -> [z, k, ., C, M, ;, \, ']
  //
  // With LHS's length being 4 and RHS's lenth being 8,
  // this would result in an intermediate result like:
  //
  //                          0b0001'1011
  //                               │ │ ││
  // 'z' >= 'M' && 'a' <= 'M' ─────┘ │ ││
  //                                 │ ││
  // 'z' >= 'C' && 'a' <= 'C' ───────┘ ││
  //                                   ││
  // 'Z' >= 'k' && 'A' <= 'k' ─────────┘│
  //                                    │
  // 'Z' >= 'z' && 'A' <= 'z' ──────────┘
  //
  FEXCORE_PRESERVE_ALL_ATTR static uint32_t
  HandleRanges(const __uint128_t& lhs, int32_t valid_lhs, const __uint128_t& rhs, int32_t valid_rhs, uint16_t control) {
    uint32_t result = 0;

    for (int j = valid_rhs; j >= 0; j--) {
      result <<= 1;

      const int element = GetElement(rhs, j, control);
      for (int i = (valid_lhs - 1) | 1; i >= 0; i -= 2) {
        const int upper_bound = GetElement(lhs, i - 0, control);
        const int lower_bound = GetElement(lhs, i - 1, control);

        const bool ge = upper_bound >= element;
        const bool le = lower_bound <= element;

        result |= static_cast<uint32_t>(ge && le);
      }
    }

    return result;
  }

  // Determines if each character is equal to one another (string compare)
  //
  // Essentially the PCMPXSTRX variant of memcmp/strcmp. Sets the bit of the
  // resulting mask if both elements are equal to one another. Otherwise
  // sets it to false.
  //
  // e.g. Assume operating on two character vectors as unsigned words
  //
  //         0  1  2  3  4  5  6  7
  // LHS -> [a, b, c, d, e, f, g, n]
  // RHS -> [a, b, c, d, e, f, e, x]
  //
  // With both explicit lengths for each string being 8 (the max length for words),
  // this would result in an intermediate result like:
  //
  //            0b0011'1111
  //                ││ ││││
  // 'f' == 'f' ────┘│ ││││
  //                 │ ││││
  // 'e' == 'e' ─────┘ ││││
  //                   ││││
  // 'd' == 'd' ───────┘│││
  //                    │││
  // 'c' == 'c' ────────┘││
  //                     ││
  // 'b' == 'b' ─────────┘│
  //                      │
  // 'a' == 'a' ──────────┘
  //
  FEXCORE_PRESERVE_ALL_ATTR static uint32_t
  HandleEqualEach(const __uint128_t& lhs, int32_t valid_lhs, const __uint128_t& rhs, int32_t valid_rhs, uint16_t control) {
    const auto upper_limit = (16 >> (control & 1)) - 1;
    const auto max_valid = std::max(valid_lhs, valid_rhs);
    const auto min_valid = std::min(valid_lhs, valid_rhs);

    // All values past the end of string must be forced to true.
    // (See 4.1.6 Valid/Invalid Override of Comparisons in the Intel Software Development Manual)
    // So we can calculate this part of the mask ahead of time and set all those to-be bits to true
    // and then progressively shift them into place over the course of execution.
    uint32_t result = (1U << (upper_limit - max_valid)) - 1;
    result <<= (max_valid - min_valid);

    for (int i = min_valid; i >= 0; i--) {
      const int lhs_element = GetElement(lhs, i, control);
      const int rhs_element = GetElement(rhs, i, control);

      result <<= 1;
      result |= static_cast<uint32_t>(lhs_element == rhs_element);
    }

    return result;
  }

  // Determines if a substring exists within an overall string
  //
  // Somewhat equivalent to the behavior of strstr.
  //
  // Sets the corresponding index in the result where a substring is found.
  //
  // e.g. Assume operating on two character vectors as unsigned words
  //
  //         0  1  2  3  4  5  6  7
  // LHS -> [b, a, x, z, y, v, o, m]
  // RHS -> [b, a, d, b, a, n, k, s]
  //
  // With the length of LHS being 2 and the length of RHS being 8, we have a composition like:
  //
  //      Substring to look for
  //       ┌──┴──┐
  // LHS -> [b, a, x, z, y, v, o, m]
  // RHS -> [b, a, d, b, a, n, k, s]
  //       └───────────┬────────────┘
  //         Entire string to search
  //
  // And we end up with a result like:
  //
  //            0b0000'1001
  //                   │  │
  // At index 3 ───────┘  │
  //                      │
  // At index 0 ──────────┘
  //
  FEXCORE_PRESERVE_ALL_ATTR static uint32_t
  HandleEqualOrdered(const __uint128_t& lhs, int32_t valid_lhs, const __uint128_t& rhs, int32_t valid_rhs, uint16_t control) {
    const auto upper_limit = (16 >> (control & 1)) - 1;

    // Edge case!
    // If we have *no* valid characters in our inner string, then
    // we need to return the intermediate result as
    // 0xFF (if operating on words) or 0xFFFF (if operating on bytes)
    if (valid_lhs == -1) {
      return (2U << upper_limit) - 1;
    }

    uint32_t result = 0;
    const int initial = valid_rhs == upper_limit ? valid_rhs : valid_rhs - valid_lhs;
    for (int j = initial; j >= 0; j--) {
      result <<= 1;

      uint32_t value = 1;
      const int start = std::min(valid_rhs - j, valid_lhs);
      for (int i = start; i >= 0; i--) {
        const int lhs_value = GetElement(lhs, i + 0, control);
        const int rhs_value = GetElement(rhs, i + j, control);

        value &= static_cast<uint32_t>(lhs_value == rhs_value);
      }

      result |= value;
    }

    return result;
  }
};

template<>
struct OpHandlers<IR::OP_VPCMPISTRX> {
  FEXCORE_PRESERVE_ALL_ATTR static uint32_t handle(VectorRegType lhs, VectorRegType rhs, uint16_t control);
};

} // namespace FEXCore::CPU
