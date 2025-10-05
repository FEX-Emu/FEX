// SPDX-License-Identifier: MIT
#pragma once
#include <FEXCore/fextl/string.h>

#include <concepts>
#include <string_view>

namespace FEXCore::StrConv {
template<std::integral T>
bool Conv(std::string_view Value, T* Result) {
  if constexpr (std::is_signed_v<T>) {
    *Result = static_cast<T>(std::strtoll(Value.data(), nullptr, 0));
  } else {
    *Result = static_cast<T>(std::strtoull(Value.data(), nullptr, 0));
  }
  return true;
}

template<typename T, typename = std::enable_if_t<std::is_enum_v<T>, T>>
bool Conv(std::string_view Value, T* Result) {
  *Result = static_cast<T>(std::strtoull(Value.data(), nullptr, 0));
  return true;
}

inline bool Conv(std::string_view Value, fextl::string* Result) {
  *Result = Value;
  return true;
}
} // namespace FEXCore::StrConv
