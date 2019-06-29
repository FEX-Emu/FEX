#include "LogManager.h"

#include "Interface/Context/Context.h"
#include "Interface/HLE/FileManagement.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace FEXCore {

class STDFD final : public FD {
public:
  STDFD(FEXCore::Context::Context *ctx, int32_t fd, const char *pathname, int32_t flags, mode_t mode)
    : FD (ctx, fd, pathname, flags, mode) {
  }

  ssize_t writev(int fd, void *iov, int iovcnt) override {
    ssize_t FinalSize {};
    std::string OutputString;

    struct iovStruct {
      uint64_t base;
      size_t len;
    };
    iovStruct *iovObject = reinterpret_cast<iovStruct*>(iov);

    for (int i = 0; i < iovcnt; ++i) {
      const char *String = CTX->MemoryMapper.GetPointer<const char*>(iovObject[i].base);
      for (size_t j = 0; j < iovObject[i].len; ++j) {
        OutputString += String[j];
      }
      FinalSize += iovObject[i].len;
    }

    OutputString += '\0';

   if (FDOffset == STDOUT_FILENO)
     LogMan::Msg::OUT("[%ld] %s", FinalSize, OutputString.c_str());
   else if (FDOffset == STDERR_FILENO)
     LogMan::Msg::ERR("[%ld] %s", FinalSize, OutputString.c_str());

    return FinalSize;
  }

  uint64_t write(int fd, void *buf, size_t count) override {
   if (FDOffset == STDOUT_FILENO)
     LogMan::Msg::OUT("%s", reinterpret_cast<char*>(buf));
   else if (FDOffset == STDERR_FILENO)
     LogMan::Msg::ERR("%s", reinterpret_cast<char*>(buf));
    return count;
  }
};

uint64_t FD::read(int fd, void *buf, size_t count) {
  return ::read(HostFD, buf, count);
}

ssize_t FD::writev(int fd, void *iov, int iovcnt) {
  ssize_t FinalSize {};
  LogMan::Msg::I(">>> writev: %d %p %d", fd, iov, iovcnt);
  for (int i = 0; i < iovcnt; ++i) {
    struct iovStruct {
      uint64_t base;
      size_t len;
    };
    iovStruct *iovObject = reinterpret_cast<iovStruct*>(iov);
    const char *String = CTX->MemoryMapper.GetPointer<const char*>(iovObject->base);
    LogMan::Msg::I("\t0x%lx Size: 0x%zx %p", iovObject->base, iovObject->len, String);
    for (size_t j = 0; j < iovObject->len; ++j) {
      LogMan::Msg::I("%c", String[j]);
    }
    FinalSize += iovObject->len;
  }

  return FinalSize;
}

uint64_t FD::write(int fd, void *buf, size_t count) {
  return ::write(fd, buf, count);
}

int FD::openat(int dirfd, const char *pathname, int flags, mode_t mode) {
  HostFD = ::openat(dirfd, pathname, flags, mode);
  return HostFD;
}

int FD::fstat(int fd, struct stat *buf) {
  return ::fstat(HostFD, buf);
}

int FD::close(int fd) {
  LogMan::Msg::D("Closing: %s", PathName.c_str());
  return ::close(HostFD);
}

FileManager::FileManager(FEXCore::Context::Context *ctx)
  : CTX {ctx} {

  FDMap[CurrentFDOffset++] = new STDFD{CTX, STDIN_FILENO, "stdin", 0, 0};
  FDMap[CurrentFDOffset++] = new STDFD{CTX, STDOUT_FILENO, "stdout", 0, 0};
  FDMap[CurrentFDOffset++] = new STDFD{CTX, STDERR_FILENO, "stderr", 0, 0};
}

FileManager::~FileManager() {
  for (auto &FD : FDMap) {
    delete FD.second;
  }
}

uint64_t FileManager::Read(int fd, [[maybe_unused]] void *buf, [[maybe_unused]] size_t count) {
  auto FD = FDMap.find(fd);
  if (FD == FDMap.end()) {
    LogMan::Msg::I("XXX: Implement Read: %d", fd);
    return -1;
  }

  return FD->second->read(fd, buf, count);
}

uint64_t FileManager::Write(int fd, void *buf, size_t count) {
  auto FD = FDMap.find(fd);
  if (FD == FDMap.end()) {
    LogMan::Msg::I("XXX: Implement write: %d", fd);
    return -1;
  }

  return FD->second->write(fd, buf, count);
}

uint64_t FileManager::Open(const char *pathname, [[maybe_unused]] int flags, [[maybe_unused]] uint32_t mode) {
  LogMan::Msg::I("XXX: Trying to open: '%s'", pathname);
  return 0;
}

uint64_t FileManager::Close(int fd) {
  auto FD = FDMap.find(fd);
  if (FD == FDMap.end()) {
    LogMan::Msg::I("XXX: Trying to close: '%d'", fd);
    return 0;
  }

  int Result = FD->second->close(fd);
  delete FD->second;
  FDMap.erase(FD);
  return Result;
}

uint64_t FileManager::Stat(const char *pathname, void *buf) {
  return ::stat(pathname, reinterpret_cast<struct stat*>(buf));
}

uint64_t FileManager::Fstat(int fd, void *buf) {
  if (fd == STDOUT_FILENO || fd == STDERR_FILENO) {
    struct stat TmpBuf;
    int Result = fstat(fd, &TmpBuf);

    // Blow away access times
    // Causes issues with lockstep runner and file acesses
    memset(&TmpBuf.st_atime, 0, sizeof(time_t));
    memset(&TmpBuf.st_mtime, 0, sizeof(time_t));
    memset(&TmpBuf.st_ctime, 0, sizeof(time_t));
    TmpBuf.st_rdev = 0x8800 + fd;

    memcpy(buf, &TmpBuf, sizeof(struct stat));
    return Result;
  }
  else {
    auto FD = FDMap.find(fd);
    if (FD != FDMap.end()) {
      return FD->second->fstat(fd, reinterpret_cast<struct stat*>(buf));
    }
  }

  LogMan::Msg::D("Attempting to stat: %d", fd);
  return -1LL;
}

uint64_t FileManager::Lseek(int fd, uint64_t offset, int whence) {
  LogMan::Msg::E("XXX: Attempting to lseek %d 0x%lx 0x%x", fd, offset, whence);
  return -1LL;
}

uint64_t FileManager::Writev(int fd, void *iov, int iovcnt) {
  auto fdPtr = FDMap.find(fd);
  if (fdPtr == FDMap.end()) {
    LogMan::Msg::E("XXX: Trying to writev unknown fd: %d", fd);
    return FDMap.find(0)->second->writev(0, iov, iovcnt);
    return -1LL;
  }
  return fdPtr->second->writev(fd, iov, iovcnt);
}

uint64_t FileManager::Access(const char *pathname, [[maybe_unused]] int mode) {
  LogMan::Msg::D("Trying to read access of: %s", pathname);
  return access(pathname, mode);
}

uint64_t FileManager::Readlink(const char *pathname, char *buf, size_t bufsiz) {
  LogMan::Msg::D("Attemptign to readlink: '%s'", pathname);
  if (strcmp(pathname, "/proc/self/exe") == 0) {
    strncpy(buf, Filename.c_str(), bufsiz);
    return std::min(bufsiz, Filename.size());
  }

  return readlink(pathname, buf, bufsiz);
}

uint64_t FileManager::Openat([[maybe_unused]] int dirfs, const char *pathname, int flags, uint32_t mode) {
  int32_t fd = CurrentFDOffset;
  LogMan::Msg::D("Attempting to open '%s'", pathname);
  if (!strcmp(pathname, "/dev/tty")) {
    FDMap[CurrentFDOffset++] = new STDFD{CTX, STDOUT_FILENO, "/dev/tty", 0, 0};
    return fd;
  }

  if (!strcmp(pathname, "/etc/ld.so.cache")) {
    return -1;
  }

  auto fdPtr = new FD{CTX, fd, pathname, flags, mode};

  auto Result = fdPtr->openat(dirfs, pathname, flags, mode);
  if (Result == -1) {
    delete fdPtr;
    return -1;
  }

  FDMap[CurrentFDOffset++] = fdPtr;

  LogMan::Msg::D("Opening: %d(%d) %s\n", fd, Result, pathname);
  return fd;
}

int32_t FileManager::FindHostFD(int fd) {
  auto FD = FDMap.find(fd);
  if (FD == FDMap.end()) {
    return -1;
  }

  return FD->second->GetHostFD();
}

}
