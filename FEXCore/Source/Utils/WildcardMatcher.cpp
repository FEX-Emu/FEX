// SPDX-License-Identifier: MIT

#include <FEXCore/Utils/WildcardMatcher.h>

namespace FEXCore::Utils::Wildcard {
class WildcardMatcher {
private:
  std::string_view pattern;
  std::string_view text;

public:
  WildcardMatcher(std::string_view pattern, std::string_view text)
    : pattern(pattern)
    , text(text) {}

  bool matchHelper(size_t p_idx, size_t t_idx);
};

bool WildcardMatcher::matchHelper(size_t p_idx, size_t t_idx) {
  if (p_idx == pattern.size()) {
    // Pattern exhausted
    return (t_idx == text.size());
  } else if (pattern[p_idx] == '*') {
    // Wildcard: Try matching zero characters, or one or more characters
    return matchHelper(p_idx + 1, t_idx) || (t_idx < text.size() && matchHelper(p_idx, t_idx + 1));
  } else {
    // Match normally
    return (t_idx < text.size() && pattern[p_idx] == text[t_idx] && matchHelper(p_idx + 1, t_idx + 1));
  }
}

bool Matches(std::string_view pattern, std::string_view text) {
  WildcardMatcher matcher(pattern, text);
  return matcher.matchHelper(0, 0);
}

} // namespace FEXCore::Utils::Wildcard
