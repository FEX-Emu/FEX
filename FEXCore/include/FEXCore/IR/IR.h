// SPDX-License-Identifier: MIT
#pragma once

#include <FEXCore/Utils/EnumOperators.h>

#include <cstdint>
#include <cstring>

namespace FEXCore::IR {

// This enum of named vector constants are linked to an array in CPUBackend.cpp.
// This is used with the IROp `LoadNamedVectorConstant` to load a vector constant
// that would otherwise be costly to materialize.
enum NamedVectorConstant : uint8_t {
  NAMED_VECTOR_INCREMENTAL_U16_INDEX = 0,
  NAMED_VECTOR_INCREMENTAL_U16_INDEX_UPPER,
  NAMED_VECTOR_PADDSUBPS_INVERT,
  NAMED_VECTOR_PADDSUBPS_INVERT_UPPER,
  NAMED_VECTOR_PADDSUBPD_INVERT,
  NAMED_VECTOR_PADDSUBPD_INVERT_UPPER,
  NAMED_VECTOR_PSUBADDPS_INVERT,
  NAMED_VECTOR_PSUBADDPS_INVERT_UPPER,
  NAMED_VECTOR_PSUBADDPD_INVERT,
  NAMED_VECTOR_PSUBADDPD_INVERT_UPPER,
  NAMED_VECTOR_MOVMSKPS_SHIFT,
  NAMED_VECTOR_AESKEYGENASSIST_SWIZZLE,
  NAMED_VECTOR_BLENDPS_0110B,
  NAMED_VECTOR_BLENDPS_0111B,
  NAMED_VECTOR_BLENDPS_1001B,
  NAMED_VECTOR_BLENDPS_1011B,
  NAMED_VECTOR_BLENDPS_1101B,
  NAMED_VECTOR_BLENDPS_1110B,
  NAMED_VECTOR_MOVMASKB,
  NAMED_VECTOR_MOVMASKB_UPPER,

  NAMED_VECTOR_X87_ONE,
  NAMED_VECTOR_X87_LOG2_10,
  NAMED_VECTOR_X87_LOG2_E,
  NAMED_VECTOR_X87_PI,
  NAMED_VECTOR_X87_LOG10_2,
  NAMED_VECTOR_X87_LOG_2,

  NAMED_VECTOR_CVTMAX_F32_I32,
  NAMED_VECTOR_CVTMAX_F32_I32_UPPER,
  NAMED_VECTOR_CVTMAX_F32_I64,
  NAMED_VECTOR_CVTMAX_F64_I32,
  NAMED_VECTOR_CVTMAX_F64_I32_UPPER,
  NAMED_VECTOR_CVTMAX_F64_I64,
  NAMED_VECTOR_CVTMAX_I32,
  NAMED_VECTOR_CVTMAX_I64,
  NAMED_VECTOR_F80_SIGN_MASK,
  NAMED_VECTOR_SHA1RNDS_K0,
  NAMED_VECTOR_SHA1RNDS_K1,
  NAMED_VECTOR_SHA1RNDS_K2,
  NAMED_VECTOR_SHA1RNDS_K3,

  NAMED_VECTOR_CONST_POOL_MAX,
  // Beginning of named constants that don't have a constant pool backing.
  NAMED_VECTOR_ZERO = NAMED_VECTOR_CONST_POOL_MAX,
  NAMED_VECTOR_MAX,
};

// This enum of named vector constants are linked to an array in CPUBackend.cpp.
// This is used with the IROp `LoadNamedVectorIndexedConstant` to load a vector constant
// that would otherwise be costly to materialize.
enum IndexNamedVectorConstant : uint8_t {
  INDEXED_NAMED_VECTOR_PSHUFLW = 0,
  INDEXED_NAMED_VECTOR_PSHUFHW,
  INDEXED_NAMED_VECTOR_PSHUFD,
  INDEXED_NAMED_VECTOR_SHUFPS,
  INDEXED_NAMED_VECTOR_DPPS_MASK,
  INDEXED_NAMED_VECTOR_DPPD_MASK,
  INDEXED_NAMED_VECTOR_PBLENDW,
  INDEXED_NAMED_VECTOR_MAX,
};

struct SHA256Sum final {
  uint8_t data[32];
  [[nodiscard]]
  bool operator<(const SHA256Sum& rhs) const {
    return memcmp(data, rhs.data, sizeof(data)) < 0;
  }

  [[nodiscard]]
  bool operator==(const SHA256Sum& rhs) const {
    return memcmp(data, rhs.data, sizeof(data)) == 0;
  }
};

typedef void ThunkedFunction(void* ArgsRv);

struct ThunkDefinition final {
  SHA256Sum Sum;
  ThunkedFunction* ThunkFunction;
};

} // namespace FEXCore::IR
