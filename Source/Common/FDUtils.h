// SPDX-License-Identifier: MIT
#pragma once

#include <FEXCore/Utils/Allocator.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/fextl/fmt.h>
#include <FEXCore/fextl/string.h>

#include <fcntl.h>
#include <filesystem>
#include <linux/limits.h>
#include <optional>
#include <unistd.h>

namespace FEX {

[[nodiscard]]
inline int get_fdpath(int fd, char* SymlinkPath) {
  auto Path = fextl::fmt::format("/proc/self/fd/{}", fd);
  return readlinkat(AT_FDCWD, Path.c_str(), SymlinkPath, PATH_MAX);
}

} // namespace FEX
