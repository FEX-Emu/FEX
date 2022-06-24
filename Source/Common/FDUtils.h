#pragma once

#include <FEXCore/Utils/LogManager.h>

#include <fcntl.h>
#include <filesystem>
#include <linux/limits.h>
#include <unistd.h>

namespace FEX {
[[maybe_unused]]
static
std::string get_fdpath(int fd) {
  char SymlinkPath[PATH_MAX];
  std::filesystem::path Path = std::filesystem::path("/proc/self/fd") / std::to_string(fd);
  int Result = readlinkat(AT_FDCWD, Path.c_str(), SymlinkPath, sizeof(SymlinkPath));
  if (Result != -1) {
    return std::string(SymlinkPath, Result);
  }

  LOGMAN_MSG_A_FMT("Couldn't get symlink from /proc/self/fd/{}", fd);
  return {};
}

}
