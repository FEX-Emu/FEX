#pragma once
#include <FEXCore/fextl/string.h>

#include <algorithm>

namespace FEX::StringUtil {
void ltrim(fextl::string &s);
void rtrim(fextl::string &s);
void trim(fextl::string &s);
}
