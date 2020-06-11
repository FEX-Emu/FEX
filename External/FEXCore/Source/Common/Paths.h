#pragma once
#include <string>

namespace FEXCore::Paths {
  void InitializePaths();
  std::string GetCachePath();
  std::string GetEntryCachePath();
}
