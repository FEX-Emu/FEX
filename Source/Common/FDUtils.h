// SPDX-License-Identifier: MIT
#pragma once

#include <FEXCore/fextl/fmt.h>

#include <fcntl.h>
#include <linux/limits.h>
#include <unistd.h>

namespace FEX {

[[nodiscard]]
inline int get_fdpath(int fd, char* SymlinkPath) {
  auto Path = fextl::fmt::format("/proc/self/fd/{}", fd);
  return readlinkat(AT_FDCWD, Path.c_str(), SymlinkPath, PATH_MAX);
}

} // namespace FEX
