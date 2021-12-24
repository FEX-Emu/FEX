#pragma once
#include <string>

namespace XXFileHash {
  std::pair<bool, uint64_t> HashFile(const std::string &Filepath);
}

