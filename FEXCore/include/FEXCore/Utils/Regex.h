// SPDX-License-Identifier: MIT

#pragma once
#include <string_view>

namespace FEXCore::Utils {
class Regex {
public:
  static bool matches(std::string_view pattern, std::string_view text);
};
} // namespace FEXCore::Utils
