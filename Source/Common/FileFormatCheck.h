// SPDX-License-Identifier: MIT
#pragma once

#include <FEXCore/fextl/string.h>

namespace FEX::FormatCheck {
bool IsSquashFS(const fextl::string& Filename);
bool IsEroFS(const fextl::string& Filename);
} // namespace FEX::FormatCheck
