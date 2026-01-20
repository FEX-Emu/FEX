// SPDX-License-Identifier: MIT

#pragma once
#include <string_view>

namespace FEXCore::Utils::Wildcard {
bool Matches(std::string_view pattern, std::string_view text);
} // namespace FEXCore::Utils::Wildcard
