// SPDX-License-Identifier: MIT
#pragma once
#include <FEXCore/fextl/vector.h>
#include <FEXCore/fextl/fmt.h>

#include <algorithm>
#include <string_view>

namespace FHU {

/**
 * @brief Parses a string of arguments, returning a vector of string_views.
 *
 * @param ArgumentString The string of arguments to parse
 *
 * @return The array of parsed arguments
 */
static inline fextl::vector<std::string_view> ParseArgumentsFromString(const std::string_view ArgumentString) {
  fextl::vector<std::string_view> Arguments;

  auto Begin = ArgumentString.begin();
  auto ArgEnd = Begin;
  const auto End = ArgumentString.end();
  while (ArgEnd != End && Begin != End) {
    // The end of an argument ends with a space or the end of the interpreter line.
    ArgEnd = std::find(Begin, End, ' ');

    if (Begin != ArgEnd) {
      const auto View = std::string_view(Begin, ArgEnd - Begin);
      if (!View.empty()) {
        Arguments.emplace_back(View);
      }
    }

    Begin = ArgEnd + 1;
  }

  return Arguments;
}
} // namespace FHU
