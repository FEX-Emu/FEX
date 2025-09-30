// SPDX-License-Identifier: MIT
#pragma once
#include <FEXCore/fextl/string.h>

#include <cstdint>
#include <string_view>
#include <optional>

namespace FEXCore::StrConv {
inline bool Conv(std::string_view Value, bool* Result) {
  *Result = std::strtoull(Value.data(), nullptr, 0);
  return true;
}

inline bool Conv(std::string_view Value, uint8_t* Result) {
  *Result = std::strtoul(Value.data(), nullptr, 0);
  return true;
}

inline bool Conv(std::string_view Value, int8_t* Result) {
  *Result = std::strtol(Value.data(), nullptr, 0);
  return true;
}

inline bool Conv(std::string_view Value, uint16_t* Result) {
  *Result = std::strtoul(Value.data(), nullptr, 0);
  return true;
}

inline bool Conv(std::string_view Value, int16_t* Result) {
  *Result = std::strtol(Value.data(), nullptr, 0);
  return true;
}

inline bool Conv(std::string_view Value, uint32_t* Result) {
  *Result = std::strtoul(Value.data(), nullptr, 0);
  return true;
}

inline bool Conv(std::string_view Value, int32_t* Result) {
  *Result = std::strtol(Value.data(), nullptr, 0);
  return true;
}

inline bool Conv(std::string_view Value, uint64_t* Result) {
  *Result = std::strtoull(Value.data(), nullptr, 0);
  return true;
}

inline bool Conv(std::string_view Value, int64_t* Result) {
  *Result = std::strtoll(Value.data(), nullptr, 0);
  return true;
}

template<typename T, typename = std::enable_if_t<std::is_enum_v<T>, T>>
inline bool Conv(std::string_view Value, T* Result) {
  *Result = static_cast<T>(std::stoull(Value.data(), nullptr, 0));
  return true;
}

inline bool Conv(std::string_view Value, fextl::string* Result) {
  *Result = Value;
  return true;
}
} // namespace FEXCore::StrConv
