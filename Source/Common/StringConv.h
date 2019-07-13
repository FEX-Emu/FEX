#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <optional>

namespace FEX::StrConv {
  [[maybe_unused]] static bool Conv(std::string_view Value, bool *Result) {
    *Result = std::stoi(std::string(Value), nullptr, 0);
    return true;
  }

  [[maybe_unused]] static bool Conv(std::string_view Value, uint8_t *Result) {
    *Result = std::stoi(std::string(Value), nullptr, 0);
    return true;
  }

  [[maybe_unused]] static bool Conv(std::string_view Value, uint16_t *Result) {
    *Result = std::stoi(std::string(Value), nullptr, 0);
    return true;
  }

  [[maybe_unused]] static bool Conv(std::string_view Value, uint32_t *Result) {
    *Result = std::stoi(std::string(Value), nullptr, 0);
    return true;
  }

  [[maybe_unused]] static bool Conv(std::string_view Value, int32_t *Result) {
    *Result = std::stoi(std::string(Value), nullptr, 0);
    return true;
  }

  [[maybe_unused]] static bool Conv(std::string_view Value, uint64_t *Result) {
    *Result = std::stoull(std::string(Value), nullptr, 0);
    return true;
  }
  [[maybe_unused]] static bool Conv(std::string_view Value, std::string *Result) {
    *Result = Value;
    return true;
  }

}
