#pragma once
#include <cstdint>
#include <unordered_map>
#include <vector>
#include <sys/stat.h>

namespace FEXCore::Context {
struct Context;
}

namespace FEXCore {

class FD {
public:
  FD() = delete;
  FD(FD &&) = delete;

  FD(FEXCore::Context::Context *ctx, int32_t fd, const char *pathname, int32_t flags, mode_t mode)
    : CTX {ctx}
    , FDOffset {fd}
    , PathName {pathname}
    , Flags {flags}
    , Mode {mode} {
  }
  virtual ~FD() {}

  uint64_t read(int fd, void *buf, size_t count);
  virtual ssize_t writev(int fd, void *iov, int iovcnt);
  virtual uint64_t write(int fd, void *buf, size_t count);
  int openat(int dirfd, const char *pathname, int flags, mode_t mode);
  int fstat(int fd, struct stat *buf);
  int close(int fd);
  int ioctl(int fd, uint64_t request, void *args);
  int lseek(int fd, off_t offset, int wehnce);

  int GetHostFD() const { return HostFD; }

protected:
  FEXCore::Context::Context *CTX;
  [[maybe_unused]] int32_t FDOffset{};
  std::string PathName;
  [[maybe_unused]] int32_t Flags;
  [[maybe_unused]] mode_t Mode;

  int32_t HostFD;
};

class FileManager final {
public:
  FileManager() = delete;
  FileManager(FileManager &&) = delete;

  FileManager(FEXCore::Context::Context *ctx);
  ~FileManager();
  uint64_t Read(int fd, void *buf, size_t count);
  uint64_t Write(int fd, void *buf, size_t count);
  uint64_t Open(const char *pathname, int flags, uint32_t mode);
  uint64_t Close(int fd);
  uint64_t Stat(const char *pathname, void *buf);
  uint64_t Fstat(int fd, void *buf);
  uint64_t Lseek(int fd, uint64_t offset, int whence);
  uint64_t Writev(int fd, void *iov, int iovcnt);
  uint64_t Access(const char *pathname, int mode);
  uint64_t Readlink(const char *pathname, char *buf, size_t bufsiz);
  uint64_t Openat(int dirfs, const char *pathname, int flags, uint32_t mode);
  uint64_t Ioctl(int fd, uint64_t request, void *args);


  int32_t FindHostFD(int fd);

  void SetFilename(std::string const &File) { Filename = File; }
  std::string const & GetFilename() const { return Filename; }

private:
  FEXCore::Context::Context *CTX;
  int32_t CurrentFDOffset{0};
  std::unordered_map<int32_t, FD*> FDMap;

  std::string Filename;
};
}
