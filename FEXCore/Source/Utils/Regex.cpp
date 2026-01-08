// SPDX-License-Identifier: MIT

#include <FEXCore/Utils/Regex.h>
#include "FEXCore/fextl/map.h"
namespace FEXCore::Utils {
class RegexMatcher {
private:
  std::string_view pattern;
  std::string_view text;
  fextl::map<std::pair<size_t, size_t>, bool> cache;

public:
  RegexMatcher(std::string_view pattern, std::string_view text)
      : pattern(pattern), text(text) {}

  bool matchHelper(size_t p_idx, size_t t_idx);
};

bool RegexMatcher::matchHelper(size_t p_idx, size_t t_idx) {
  auto key = std::make_pair(p_idx, t_idx);

  // Check cache
  if (auto it = cache.find(key); it != cache.end()) {
    return it->second;
  }

  bool result;

  // Pattern exhausted
  if (p_idx == pattern.size()) {
    result = (t_idx == text.size());
  }
  // Wildcard 
  else if (pattern[p_idx] == '*') {
    // Try matching zero characters, or one or more characters
    result = matchHelper(p_idx + 1, t_idx) ||
             (t_idx < text.size() && matchHelper(p_idx, t_idx + 1));
  }
  // Match normally
  else {
    result = (t_idx < text.size() && pattern[p_idx] == text[t_idx] &&
              matchHelper(p_idx + 1, t_idx + 1));
  }
  cache[key] = result;
  return result;
}

bool Regex::matches(std::string_view pattern, std::string_view text) {
  RegexMatcher matcher(pattern, text);
  return matcher.matchHelper(0, 0);
}

} // namespace FEXCore::Utils
