// SPDX-License-Identifier: MIT

#include <FEXCore/Utils/WildcardMatcher.h>

namespace FEXCore::Utils::Wildcard {
static bool matchHelper(std::string_view pattern, std::string_view text, size_t p_idx, size_t t_idx) {
  if (p_idx == pattern.size()) {
    // Pattern exhausted
    return (t_idx == text.size());
  } else if (pattern[p_idx] == '*') {
    // Wildcard: Try matching zero characters, or one or more characters
    return matchHelper(pattern, text, p_idx + 1, t_idx) || (t_idx < text.size() && matchHelper(pattern, text, p_idx, t_idx + 1));
  } else {
    // Match normally
    return (t_idx < text.size() && pattern[p_idx] == text[t_idx] && matchHelper(pattern, text, p_idx + 1, t_idx + 1));
  }
}

bool Matches(std::string_view pattern, std::string_view text) {
  return matchHelper(pattern, text, 0, 0);
}

} // namespace FEXCore::Utils::Wildcard
