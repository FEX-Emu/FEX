#pragma once
#include <FEXCore/fextl/string.h>

#include <cstdint>
#include <string_view>
#include <optional>

namespace FEXCore::StrConv {
  [[maybe_unused]] static bool Conv(std::string_view Value, bool *Result) {
    *Result = std::stoi(fextl::string(Value).c_str(), nullptr, 0);
    return true;
  }

  [[maybe_unused]] static bool Conv(std::string_view Value, uint8_t *Result) {
    *Result = std::stoi(fextl::string(Value).c_str(), nullptr, 0);
    return true;
  }

  [[maybe_unused]] static bool Conv(std::string_view Value, uint16_t *Result) {
    *Result = std::stoi(fextl::string(Value).c_str(), nullptr, 0);
    return true;
  }

  [[maybe_unused]] static bool Conv(std::string_view Value, uint32_t *Result) {
    *Result = std::stoi(fextl::string(Value).c_str(), nullptr, 0);
    return true;
  }

  [[maybe_unused]] static bool Conv(std::string_view Value, int32_t *Result) {
    *Result = std::stoi(fextl::string(Value).c_str(), nullptr, 0);
    return true;
  }

  [[maybe_unused]] static bool Conv(std::string_view Value, uint64_t *Result) {
    *Result = std::stoull(fextl::string(Value).c_str(), nullptr, 0);
    return true;
  }
  template <typename T,
    typename = std::enable_if<std::is_enum<T>::value, T>>
  [[maybe_unused]] static bool Conv(std::string_view Value, T *Result) {
    *Result = static_cast<T>(std::stoull(fextl::string(Value).c_str(), nullptr, 0));
    return true;
  }

  [[maybe_unused]] static bool Conv(std::string_view Value, fextl::string *Result) {
    *Result = Value;
    return true;
  }
}
