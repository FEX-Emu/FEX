#include "LogManager.h"

#include "Interface/Context/Context.h"
#include "Interface/HLE/FileManagement.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <unistd.h>

#ifdef _M_X86_64
// x86-64 Syscall argument ABI
// RAX - Syscall number
// RDI - Arg 1
// RSI - Arg 2
// RDX - Arg 3
// R10 - Arg 4
// R8  - Arg 5
// R9  - Arg 6
static uint64_t DoSyscall(uint64_t Syscall) {
  uint64_t Result;
  asm volatile (
    "syscall;"
    : "=r" (Result)
    : "0" (Syscall)
    : "memory", "cc", "r11", "cx"
  );
  return Result;
}

static uint64_t DoSyscall(uint64_t Syscall, uint64_t Arg1, uint64_t Arg2, uint64_t Arg3) {
  uint64_t Result;
  register uint64_t _Arg3 asm ("rdx") = Arg3;
  register uint64_t _Arg2 asm ("rsi") = Arg2;
  register uint64_t _Arg1 asm ("rdi") = Arg1;
  asm volatile (
    "syscall;"
    : "=r" (Result)
    : "0" (Syscall), "r" (_Arg1), "r" (_Arg2), "r" (_Arg3)
    : "memory", "cc", "r11", "cx"
  );
  return Result;
}
#else
static uint64_t DoSyscall(uint64_t Syscall) {
  LogMan::Msg::A("Can't do syscall on this platform yet");
}
static uint64_t DoSyscall(uint64_t Syscall, uint64_t Arg1, uint64_t Arg2, uint64_t Arg3) {
  LogMan::Msg::A("Can't do syscall on this platform yet");
}
#endif

namespace FEXCore {

class STDFD final : public FD {
public:
  STDFD(FEXCore::Context::Context *ctx, int32_t fd, const char *pathname, int32_t flags, mode_t mode)
    : FD (ctx, fd, pathname, flags, mode) {
    HostFD = fd;
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

    if (CTX->Config.AccurateSTDOut) {
      return ::writev(fd, (const struct iovec*)iov, iovcnt);
    }
    else {
      if (FDOffset == STDOUT_FILENO)
        LogMan::Msg::OUT("[%ld] %s", FinalSize, OutputString.c_str());
      else if (FDOffset == STDERR_FILENO)
        LogMan::Msg::ERR("[%ld] %s", FinalSize, OutputString.c_str());
      return FinalSize;
    }
  }

  uint64_t write(int fd, void *buf, size_t count) override {
    if (CTX->Config.AccurateSTDOut) {
      return ::write(fd, buf, count);
    }
    else {
      if (FDOffset == STDOUT_FILENO)
        LogMan::Msg::OUT("%s", reinterpret_cast<char*>(buf));
      else if (FDOffset == STDERR_FILENO)
        LogMan::Msg::ERR("%s", reinterpret_cast<char*>(buf));
      return count;
    }
  }
};

class SocketFD final : public FD {
public:
  SocketFD(FEXCore::Context::Context *ctx, int32_t fd, int domain, int type, int protocol)
    : FD (ctx, fd, "Socket", 0, 0) {
    HostFD = socket(domain, type, protocol);
  }

private:

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
  return ::close(HostFD);
}

int FD::ioctl(int fd, uint64_t request, void *args) {
  return ::ioctl(HostFD, request, args);
}

int FD::lseek(int fd, off_t offset, int whence) {
  return ::lseek(HostFD, offset, whence);
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
  auto fdPtr = FDMap.find(fd);
  if (fdPtr == FDMap.end()) {
    LogMan::Msg::E("XXX: Trying to lseek unknown fd: %d", fd);
  return -1LL;
}
  return fdPtr->second->lseek(fd, offset, whence);
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
  if (!strcmp(pathname, "/dev/tty")) {
    FDMap[CurrentFDOffset++] = new STDFD{CTX, STDOUT_FILENO, "/dev/tty", 0, 0};
    return fd;
  }

  auto fdPtr = new FD{CTX, fd, pathname, flags, mode};

  auto Result = fdPtr->openat(dirfs, pathname, flags, mode);
  if (Result == -1) {
    delete fdPtr;
    return -1;
  }

  FDMap[CurrentFDOffset++] = fdPtr;

  return fd;
}

uint64_t FileManager::Ioctl(int fd, uint64_t request, void *args) {
  auto FD = FDMap.find(fd);
  if (FD == FDMap.end()) {
    return -1;
  }

  return FD->second->ioctl(fd, request, args);
}

uint64_t FileManager::GetDents(int fd, void *dirp, uint32_t count) {
  auto FD = FDMap.find(fd);
  if (FD == FDMap.end()) {
    return -1;
  }

  return DoSyscall(SYSCALL_GETDENTS64,
      static_cast<uint64_t>(FD->second->GetHostFD()),
      reinterpret_cast<uint64_t>(dirp),
      static_cast<uint64_t>(count));
}

uint64_t FileManager::Socket(int domain, int type, int protocol) {
  int32_t fd = CurrentFDOffset;

  auto fdPtr = new SocketFD{CTX, fd, domain, type, protocol};

  auto Result = fdPtr->GetHostFD();
  if (Result == -1) {
    delete fdPtr;
    return -1;
  }

  FDMap[CurrentFDOffset++] = fdPtr;

  return fd;
}

uint64_t FileManager::Connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
  auto FD = FDMap.find(sockfd);
  if (FD == FDMap.end()) {
    return -1;
  }

  return connect(FD->second->GetHostFD(), addr, addrlen);
}

uint64_t FileManager::Poll(struct pollfd *fds, nfds_t nfds, int timeout) {
  std::vector<pollfd> HostFDs;
  HostFDs.resize(nfds);
  memcpy(&HostFDs.at(0), fds, sizeof(pollfd) * nfds);
  for (auto &FD : HostFDs) {
    auto HostFD = FDMap.find(FD.fd);
    if (HostFD == FDMap.end())
      return -1;
    FD.fd = HostFD->second->GetHostFD();
  }

  return poll(fds, nfds, timeout);
}

uint64_t FileManager::Select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout) {
  fd_set Host_readfds;
  fd_set Host_writefds;
  fd_set Host_exceptfds;

  FD_ZERO(&Host_readfds);
  FD_ZERO(&Host_writefds);
  FD_ZERO(&Host_exceptfds);
  int Host_nfds = 0;
  if (readfds) {
    for (int i = 0; i < nfds; ++i) {
      if (FD_ISSET(i, readfds)) {
        auto HostFD = FDMap.find(i);
        if (HostFD == FDMap.end())
          return -1;
        Host_nfds = std::max(Host_nfds, HostFD->second->GetHostFD());
        FD_SET(HostFD->second->GetHostFD(), &Host_readfds);
      }
    }
  }
  if (writefds) {
    for (int i = 0; i < nfds; ++i) {
      if (FD_ISSET(i, writefds)) {
        auto HostFD = FDMap.find(i);
        if (HostFD == FDMap.end())
          return -1;
        Host_nfds = std::max(Host_nfds, HostFD->second->GetHostFD());
        FD_SET(HostFD->second->GetHostFD(), &Host_writefds);
      }
    }
  }
  if (exceptfds) {
    for (int i = 0; i < nfds; ++i) {
      if (FD_ISSET(i, exceptfds)) {
        auto HostFD = FDMap.find(i);
        if (HostFD == FDMap.end())
          return -1;
        Host_nfds = std::max(Host_nfds, HostFD->second->GetHostFD());
        FD_SET(HostFD->second->GetHostFD(), &Host_exceptfds);
      }
    }
  }

  return select(Host_nfds, readfds ? &Host_readfds : nullptr, writefds ? &Host_writefds : nullptr, exceptfds ? &Host_exceptfds : nullptr, timeout);
}

int32_t FileManager::FindHostFD(int fd) {
  auto FD = FDMap.find(fd);
  if (FD == FDMap.end()) {
    return -1;
  }

  return FD->second->GetHostFD();
}

FD const* FileManager::GetFDBacking(int fd) {
  auto FD = FDMap.find(fd);
  if (FD == FDMap.end()) {
    return nullptr;
  }

  return FD->second;
}

}
