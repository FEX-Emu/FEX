#pragma once
#include <string>

namespace SquashFS {
  bool InitializeSquashFS();
  void UnmountRootFS();
  std::string GetMountFolder();
}
