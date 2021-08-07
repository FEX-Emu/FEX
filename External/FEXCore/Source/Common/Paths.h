#pragma once
#include <string>

namespace FEXCore::Paths {
  void InitializePaths();
  void ShutdownPaths();

  const char *GetHomeDirectory();

  std::string GetCachePath();
  std::string GetEntryCachePath();
}
