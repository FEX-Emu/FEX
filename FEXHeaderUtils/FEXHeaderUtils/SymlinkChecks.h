#pragma once
#include <FEXCore/fextl/string.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <span>
#include <unistd.h>

namespace FHU::Symlinks {
#ifndef _WIN32
// Checks to see if a filepath is a symlink.
inline bool IsSymlink(const fextl::string &Filename) {
  struct stat Buffer{};
  int Result = lstat(Filename.c_str(), &Buffer);
  return Result == 0 && S_ISLNK(Buffer.st_mode);
}

// Resolves a symlink path.
// Doesn't handle recursive symlinks.
// Doesn't append null terminator character.
// Returns a string_view of the resolved path, or an empty view on error.
inline std::string_view ResolveSymlink(const fextl::string &Filename, std::span<char> ResultBuffer) {
  ssize_t Result = readlink(Filename.c_str(), ResultBuffer.data(), ResultBuffer.size());
  if (Result == -1) {
    return {};
  }

  return std::string_view(ResultBuffer.data(), Result);
}
#endif
}
