#pragma once
#include <FEXCore/fextl/string.h>

namespace FEXCore::Paths {
  void InitializePaths();
  void ShutdownPaths();

  const char *GetHomeDirectory();

  fextl::string GetCachePath();
  fextl::string GetEntryCachePath();
}
