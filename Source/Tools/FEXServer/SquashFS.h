#pragma once
#include <FEXCore/fextl/string.h>

namespace SquashFS {
  bool InitializeSquashFS();
  void UnmountRootFS();
  fextl::string GetMountFolder();
}
