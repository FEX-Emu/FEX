// SPDX-License-Identifier: MIT
#pragma once
#include <FEXCore/fextl/string.h>

namespace FEXCore::StringUtils {
  // Trim the left side of the string of whitespace and new lines
  [[maybe_unused]] static fextl::string LeftTrim(fextl::string String, std::string_view TrimTokens = " \t\n\r") {
    size_t pos = fextl::string::npos;
    if ((pos = String.find_first_not_of(TrimTokens)) != fextl::string::npos) {
      String.erase(0, pos);
    }

    return String;
  }

  // Trim the right side of the string of whitespace and new lines
  [[maybe_unused]] static fextl::string RightTrim(fextl::string String, std::string_view TrimTokens = " \t\n\r") {
    size_t pos = fextl::string::npos;
    if ((pos = String.find_last_not_of(TrimTokens)) != fextl::string::npos) {
      String.erase(String.begin() + pos + 1, String.end());
    }

    return String;
  }

  // Trim both the left and right of the string of whitespace and new lines
  [[maybe_unused]] static fextl::string Trim(fextl::string String, std::string_view TrimTokens = " \t\n\r") {
    return RightTrim(LeftTrim(std::move(String), TrimTokens), TrimTokens);
  }

}
