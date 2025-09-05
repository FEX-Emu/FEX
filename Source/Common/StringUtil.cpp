// SPDX-License-Identifier: MIT
#include "Common/StringUtil.h"

#include <algorithm>

namespace FEX::StringUtil {
void ltrim(fextl::string& s) {
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) { return !std::isspace(ch); }));
}

void rtrim(fextl::string& s) {
  s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) { return !std::isspace(ch); }).base(), s.end());
}
void trim(fextl::string& s) {
  ltrim(s);
  rtrim(s);
}
} // namespace FEX::StringUtil
