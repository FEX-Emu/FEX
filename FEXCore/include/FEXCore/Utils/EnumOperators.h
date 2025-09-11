// SPDX-License-Identifier: MIT
#pragma once
#include <type_traits>

#define FEX_DEF_ENUM_CLASS_BIN_OP(Enum, Op)                       \
  inline constexpr Enum operator Op(Enum lhs, Enum rhs) {         \
    using Type = std::underlying_type_t<Enum>;                    \
    Type _lhs = static_cast<Type>(lhs);                           \
    Type _rhs = static_cast<Type>(rhs);                           \
    return static_cast<Enum>(_lhs Op _rhs);                       \
  }                                                               \
  inline constexpr uint64_t operator Op(uint64_t lhs, Enum rhs) { \
    using Type = std::underlying_type_t<Enum>;                    \
    Type _rhs = static_cast<Type>(rhs);                           \
    return lhs Op _rhs;                                           \
  }

#define FEX_DEF_ENUM_CLASS_UNARY_OP(Enum, Op)   \
  inline constexpr Enum operator Op(Enum rhs) { \
    using Type = std::underlying_type_t<Enum>;  \
    Type _rhs = static_cast<Type>(rhs);         \
    return static_cast<Enum>(Op _rhs);          \
  }

#define FEX_DEF_NUM_OPS(Enum)        \
  FEX_DEF_ENUM_CLASS_BIN_OP(Enum, |) \
  FEX_DEF_ENUM_CLASS_BIN_OP(Enum, &) \
  FEX_DEF_ENUM_CLASS_BIN_OP(Enum, ^) \
  FEX_DEF_ENUM_CLASS_UNARY_OP(Enum, ~)
