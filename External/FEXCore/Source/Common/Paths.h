#pragma once
#include <string>

namespace FEXCore::Paths {
  void InitializePaths();
  void ShutdownPaths();
  std::string GetCachePath();
  std::string GetEntryCachePath();
}
