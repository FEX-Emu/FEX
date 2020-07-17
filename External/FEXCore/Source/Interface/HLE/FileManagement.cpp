#include "LogManager.h"

#include "Interface/Context/Context.h"
#include "Interface/HLE/FileManagement.h"
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <sys/timerfd.h>
#include <sys/uio.h>
#include <sys/vfs.h>
#include <unistd.h>

namespace FEXCore {

FileManager::FileManager(FEXCore::Context::Context *ctx)
  : CTX {ctx}
  , EmuFD {ctx} {

    // calculate the non-self link to exe
    // Some executables do getpid, stat("/proc/$pid/exe")
    int pid = getpid();

    char buf[50];
    snprintf(buf, 50, "/proc/%i/exe", pid);

    PidSelfPath = std::string(buf);
}

FileManager::~FileManager() {
}

std::string FileManager::GetEmulatedPath(const char *pathname) {
  if (pathname[0] != '/' ||
      CTX->Config.RootFSPath.empty())
    return {};

  return CTX->Config.RootFSPath + pathname;
}

uint64_t FileManager::Open(const char *pathname, [[maybe_unused]] int flags, [[maybe_unused]] uint32_t mode) {
  return ::open(pathname, flags, mode);
}

uint64_t FileManager::Close(int fd) {
  FDToNameMap.erase(fd);
  return ::close(fd);
}

uint64_t FileManager::Stat(const char *pathname, void *buf) {
  auto Path = GetEmulatedPath(pathname);
  if (!Path.empty()) {
    uint64_t Result = ::stat(Path.c_str(), reinterpret_cast<struct stat*>(buf));
    if (Result != -1)
      return Result;
  }
  return ::stat(pathname, reinterpret_cast<struct stat*>(buf));
}

uint64_t FileManager::Lstat(const char *path, void *buf) {
  auto Path = GetEmulatedPath(path);
  if (!Path.empty()) {
    uint64_t Result = ::lstat(Path.c_str(), reinterpret_cast<struct stat*>(buf));
    if (Result != -1)
      return Result;
  }

  return ::lstat(path, reinterpret_cast<struct stat*>(buf));
}

uint64_t FileManager::Access(const char *pathname, [[maybe_unused]] int mode) {
  auto Path = GetEmulatedPath(pathname);
  if (!Path.empty()) {
    uint64_t Result = ::access(Path.c_str(), mode);
    if (Result != -1)
      return Result;
  }

  return ::access(pathname, mode);
}

uint64_t FileManager::FAccessat(int dirfd, const char *pathname, int mode, int flags) {
  auto Path = GetEmulatedPath(pathname);
  if (!Path.empty()) {
    uint64_t Result = ::faccessat(dirfd, Path.c_str(), mode, flags);
    if (Result != -1)
      return Result;
  }

  return ::faccessat(dirfd, pathname, mode, flags);
}

uint64_t FileManager::Readlink(const char *pathname, char *buf, size_t bufsiz) {
  if (strcmp(pathname, "/proc/self/exe") == 0 || strcmp(pathname, PidSelfPath.c_str()) == 0) {
    strncpy(buf, Filename.c_str(), bufsiz);
    return std::min(bufsiz, Filename.size());
  }

  auto Path = GetEmulatedPath(pathname);
  if (!Path.empty()) {
    uint64_t Result = ::readlink(Path.c_str(), buf, bufsiz);
    if (Result != -1)
      return Result;
  }

  return ::readlink(pathname, buf, bufsiz);
}

uint64_t FileManager::Chmod(const char *pathname, mode_t mode) {
  auto Path = GetEmulatedPath(pathname);
  if (!Path.empty()) {
    uint64_t Result = ::chmod(Path.c_str(), mode);
    if (Result != -1)
      return Result;
  }

  return ::chmod(pathname, mode);
}

uint64_t FileManager::Readlinkat(int dirfd, const char *pathname, char *buf, size_t bufsiz) {
  if (strcmp(pathname, "/proc/self/exe") == 0) {
    strncpy(buf, Filename.c_str(), bufsiz);
    return std::min(bufsiz, Filename.size());
  }

  auto Path = GetEmulatedPath(pathname);
  if (!Path.empty()) {
    uint64_t Result = ::readlinkat(dirfd, Path.c_str(), buf, bufsiz);
    if (Result != -1)
      return Result;
  }

  return ::readlinkat(dirfd, pathname, buf, bufsiz);
}

uint64_t FileManager::Openat([[maybe_unused]] int dirfs, const char *pathname, int flags, uint32_t mode) {
  int32_t fd = -1;

  fd = EmuFD.OpenAt(dirfs, pathname, flags, mode);
  if (fd == -1) {
    auto Path = GetEmulatedPath(pathname);
    if (!Path.empty()) {
      fd = ::openat(dirfs, Path.c_str(), flags, mode);
    }

    if (fd == -1)
      fd = ::openat(dirfs, pathname, flags, mode);
  }

  if (fd != -1)
    FDToNameMap[fd] = pathname;

  return fd;
}

uint64_t FileManager::Statx(int dirfd, const char *pathname, int flags, uint32_t mask, struct statx *statxbuf) {
  auto Path = GetEmulatedPath(pathname);
  if (!Path.empty()) {
    uint64_t Result = ::statx(dirfd, Path.c_str(), flags, mask, statxbuf);
    if (Result != -1)
      return Result;
  }
  return ::statx(dirfd, pathname, flags, mask, statxbuf);
}

uint64_t FileManager::Mknod(const char *pathname, mode_t mode, dev_t dev) {
  auto Path = GetEmulatedPath(pathname);
  if (!Path.empty()) {
    uint64_t Result = ::mknod(Path.c_str(), mode, dev);
    if (Result != -1)
      return Result;
  }
  return ::mknod(pathname, mode, dev);
}

uint64_t FileManager::Statfs(const char *path, void *buf) {
  auto Path = GetEmulatedPath(path);
  if (!Path.empty()) {
    uint64_t Result = ::statfs(Path.c_str(), reinterpret_cast<struct statfs*>(buf));
    if (Result != -1)
      return Result;
  }
  return ::statfs(path, reinterpret_cast<struct statfs*>(buf));
}

std::string *FileManager::FindFDName(int fd) {
  auto it = FDToNameMap.find(fd);
  if (it == FDToNameMap.end()) {
    return nullptr;
  }
  return &it->second;
}

}
