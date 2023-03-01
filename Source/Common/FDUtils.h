#pragma once

#include <FEXCore/Utils/LogManager.h>

#include <fcntl.h>
#include <filesystem>
#include <linux/limits.h>
#include <optional>
#include <unistd.h>

namespace FEX {
[[maybe_unused]]
static
std::optional<std::string> get_fdpath(int fd) {
  char SymlinkPath[PATH_MAX];
  std::filesystem::path Path = std::filesystem::path("/proc/self/fd") / std::to_string(fd);
  int Result = readlinkat(AT_FDCWD, Path.c_str(), SymlinkPath, sizeof(SymlinkPath));
  if (Result != -1) {
    return std::string(SymlinkPath, Result);
  }

  // Not fatal if an FD doesn't point to a file
  return std::nullopt;
}

inline
int get_fdpath(int fd, char *SymlinkPath) {
  auto Path = fmt::format("/proc/self/fd/{}", fd);
  return readlinkat(AT_FDCWD, Path.c_str(), SymlinkPath, PATH_MAX);
}

}
