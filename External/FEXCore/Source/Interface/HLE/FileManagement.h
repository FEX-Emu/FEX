#pragma once
#include <cstdint>
#include <unordered_map>
#include <vector>
#include <sys/stat.h>
#include <sys/socket.h>
#include <poll.h>

#include "Interface/HLE/EmulatedFiles/EmulatedFiles.h"

namespace FEXCore::Context {
struct Context;
}

namespace FEXCore {

class FileManager final {
public:
  FileManager() = delete;
  FileManager(FileManager &&) = delete;

  FileManager(FEXCore::Context::Context *ctx);
  ~FileManager();
  uint64_t Open(const char *pathname, int flags, uint32_t mode);
  uint64_t Close(int fd);
  uint64_t Stat(const char *pathname, void *buf);
  uint64_t Lstat(const char *path, void *buf);
  uint64_t Access(const char *pathname, int mode);
  uint64_t FAccessat(int dirfd, const char *pathname, int mode, int flags);
  uint64_t Readlink(const char *pathname, char *buf, size_t bufsiz);
  uint64_t Chmod(const char *pathname, mode_t mode);
  uint64_t Readlinkat(int dirfd, const char *pathname, char *buf, size_t bufsiz);
  uint64_t Openat(int dirfs, const char *pathname, int flags, uint32_t mode);
  uint64_t Statx(int dirfd, const char *pathname, int flags, uint32_t mask, struct statx *statxbuf);
  uint64_t Mknod(const char *pathname, mode_t mode, dev_t dev);

  // vfs
  uint64_t Statfs(const char *path, void *buf);

  std::string *FindFDName(int fd);

  void SetFilename(std::string const &File) { Filename = File; }
  std::string const & GetFilename() const { return Filename; }

private:
  FEXCore::Context::Context *CTX;
  FEXCore::EmulatedFile::EmulatedFDManager EmuFD;

  std::unordered_map<int32_t, std::string> FDToNameMap;
  std::string Filename;
  std::string GetEmulatedPath(const char *pathname);
};
}
