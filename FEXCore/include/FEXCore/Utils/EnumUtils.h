// SPDX-License-Identifier: MIT
#pragma once

// Header for various utilities related to operating with enums

#include <FEXCore/fextl/fmt.h>
#include <type_traits>

namespace FEXCore {

// Macro that defines all of the built in operators for conveniently using
// enum classes as flag types without needing to define all of the basic
// boilerplate.
#define FEX_DECLARE_ENUM_FLAG_OPERATORS(type)                        \
  [[nodiscard]]                                                      \
  constexpr type operator|(type a, type b) noexcept {                \
    using T = std::underlying_type_t<type>;                          \
    return static_cast<type>(static_cast<T>(a) | static_cast<T>(b)); \
  }                                                                  \
  [[nodiscard]]                                                      \
  constexpr type operator&(type a, type b) noexcept {                \
    using T = std::underlying_type_t<type>;                          \
    return static_cast<type>(static_cast<T>(a) & static_cast<T>(b)); \
  }                                                                  \
  [[nodiscard]]                                                      \
  constexpr type operator^(type a, type b) noexcept {                \
    using T = std::underlying_type_t<type>;                          \
    return static_cast<type>(static_cast<T>(a) ^ static_cast<T>(b)); \
  }                                                                  \
  constexpr type& operator|=(type& a, type b) noexcept {             \
    a = a | b;                                                       \
    return a;                                                        \
  }                                                                  \
  constexpr type& operator&=(type& a, type b) noexcept {             \
    a = a & b;                                                       \
    return a;                                                        \
  }                                                                  \
  constexpr type& operator^=(type& a, type b) noexcept {             \
    a = a ^ b;                                                       \
    return a;                                                        \
  }                                                                  \
  [[nodiscard]]                                                      \
  constexpr type operator~(type key) noexcept {                      \
    using T = std::underlying_type_t<type>;                          \
    return static_cast<type>(~static_cast<T>(key));                  \
  }                                                                  \
  [[nodiscard]]                                                      \
  constexpr bool True(type key) noexcept {                           \
    using T = std::underlying_type_t<type>;                          \
    return static_cast<T>(key) != 0;                                 \
  }                                                                  \
  [[nodiscard]]                                                      \
  constexpr bool False(type key) noexcept {                          \
    using T = std::underlying_type_t<type>;                          \
    return static_cast<T>(key) == 0;                                 \
  }

// Macro that defines a fmt formatter for a reasonable case where an enum
// is formatted as a purely integral type based on its underlying type.
#define FEX_DEFINE_ENUM_FMT_PASSTHROUGH(type)                                  \
  template<>                                                                   \
  struct fmt::formatter<type> : fmt::formatter<std::underlying_type_t<type>> { \
    using Base = fmt::formatter<std::underlying_type_t<type>>;                 \
                                                                               \
    template<typename FormatContext>                                           \
    auto format(const type& Value, FormatContext& ctx) const {                 \
      return Base::format(FEXCore::ToUnderlying(Value), ctx);                  \
    }                                                                          \
  }

// Equivalent to C++23's std::to_underlying.
template<typename Enum>
[[nodiscard]]
constexpr std::underlying_type_t<Enum> ToUnderlying(Enum e) noexcept {
  return static_cast<std::underlying_type_t<Enum>>(e);
}

} // namespace FEXCore
