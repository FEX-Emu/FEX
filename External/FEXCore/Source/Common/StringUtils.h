#pragma once
#include <FEXCore/fextl/string.h>
#include <string>

namespace FEXCore::StringUtils {
  // TODO: Remove std::string helpers once moved to fextl entirely.
  // Trim the left side of the string of whitespace and new lines
  [[maybe_unused]] static std::string LeftTrim(std::string String, std::string TrimTokens = " \t\n\r") {
    size_t pos = std::string::npos;
    if ((pos = String.find_first_not_of(TrimTokens)) != std::string::npos) {
      String.erase(0, pos);
    }

    return String;
  }

  [[maybe_unused]] static fextl::string LeftTrim(fextl::string String, fextl::string TrimTokens = " \t\n\r") {
    size_t pos = fextl::string::npos;
    if ((pos = String.find_first_not_of(TrimTokens)) != fextl::string::npos) {
      String.erase(0, pos);
    }

    return String;
  }

  // Trim the right side of the string of whitespace and new lines
  [[maybe_unused]] static std::string RightTrim(std::string String, std::string TrimTokens = " \t\n\r") {
    size_t pos = std::string::npos;
    if ((pos = String.find_last_not_of(TrimTokens)) != std::string::npos) {
      String.erase(String.begin() + pos + 1, String.end());
    }

    return String;
  }

  [[maybe_unused]] static fextl::string RightTrim(fextl::string String, fextl::string TrimTokens = " \t\n\r") {
    size_t pos = fextl::string::npos;
    if ((pos = String.find_last_not_of(TrimTokens)) != fextl::string::npos) {
      String.erase(String.begin() + pos + 1, String.end());
    }

    return String;
  }

  // Trim both the left and right of the string of whitespace and new lines
  [[maybe_unused]] static std::string Trim(std::string String, std::string TrimTokens = " \t\n\r") {
    return RightTrim(LeftTrim(String, TrimTokens), TrimTokens);
  }

  [[maybe_unused]] static fextl::string Trim(fextl::string String, fextl::string TrimTokens = " \t\n\r") {
    return RightTrim(LeftTrim(String, TrimTokens), TrimTokens);
  }

}
