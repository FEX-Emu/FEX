#pragma once
#include <FEXCore/fextl/vector.h>
#include <FEXCore/fextl/fmt.h>

#include <algorithm>
#include <string_view>

namespace FHU {

/**
 * @brief Parses a shebang string, returning a vector of string_views.
 *
 * @param ArgumentString The shebang string to parse
 *
 * @return The array of parsed elements
 */
static inline fextl::vector<std::string_view> ParseArgumentsFromString(const std::string_view ArgumentString) {
  fextl::vector<std::string_view> Arguments;

  const auto SPACE = " \f\n\r\t\v";

  auto InterpBegin = ArgumentString.find_first_not_of(SPACE);
  if (InterpBegin == std::string::npos) {
    return Arguments;
  }

  auto InterpLen = ArgumentString.substr(InterpBegin).find_first_of(SPACE);
  Arguments.emplace_back(ArgumentString.substr(InterpBegin, InterpLen));
  if (InterpLen == std::string::npos) {
    return Arguments;
  }

  auto Arg = ArgumentString.substr(InterpBegin + InterpLen);
  auto ArgBegin = Arg.find_first_not_of(SPACE);
  if (ArgBegin == std::string::npos) {
    return Arguments;
  }

  Arg = Arg.substr(ArgBegin);

  auto ArgEnd = Arg.find_last_not_of(SPACE);
  Arguments.emplace_back(Arg.substr(0, ArgEnd + 1));

  return Arguments;
}
} // namespace FHU
