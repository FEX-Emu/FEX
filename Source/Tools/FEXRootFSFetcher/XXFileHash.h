// SPDX-License-Identifier: MIT
#pragma once
#include <FEXCore/fextl/string.h>

#include <optional>

namespace XXFileHash {
std::optional<uint64_t> HashFile(const fextl::string& Filepath);
}
