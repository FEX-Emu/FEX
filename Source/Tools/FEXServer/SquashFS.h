// SPDX-License-Identifier: MIT
#pragma once
#include <FEXCore/fextl/string.h>

namespace SquashFS {
bool InitializeSquashFS();
void UnmountRootFS();
const fextl::string& GetMountFolder();
} // namespace SquashFS
