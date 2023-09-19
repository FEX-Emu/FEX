// SPDX-License-Identifier: MIT
#pragma once
#include <FEXCore/fextl/string.h>

namespace XXFileHash {
  std::pair<bool, uint64_t> HashFile(const fextl::string &Filepath);
}

