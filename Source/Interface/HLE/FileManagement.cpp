#include "LogManager.h"

#include "Interface/Context/Context.h"
#include "Interface/HLE/FileManagement.h"
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <sys/timerfd.h>
#include <sys/uio.h>
#include <sys/vfs.h>
#include <unistd.h>

namespace FEXCore {

class STDFD final : public FD {
public:
  STDFD(FEXCore::Context::Context *ctx, int32_t fd, const char *pathname, int32_t flags, mode_t mode)
    : FD (ctx, fd, pathname, flags, mode) {
    HostFD = fd;
  }

  ssize_t writev(int fd, void *iov, int iovcnt) override {
    struct Object {
      void *iov_base;
      size_t iov_len;
    };
    Object *iovObject = reinterpret_cast<Object*>(iov);

    if (CTX->Config.AccurateSTDOut) {
      if (CTX->Config.UnifiedMemory) {
        return ::writev(fd, reinterpret_cast<iovec const *>(iov), iovcnt);
      }
      else {
        std::vector<iovec> HostStructs(iovcnt);
        for (int i = 0; i < iovcnt; ++i) {
          HostStructs[i].iov_base = CTX->MemoryMapper.GetPointer<void*>(reinterpret_cast<uint64_t>(iovObject[i].iov_base));
          HostStructs[i].iov_len = iovObject[i].iov_len;
        }

        return ::writev(fd, &HostStructs.at(0), iovcnt);
      }
    }
    else {
      ssize_t FinalSize {};
      std::string OutputString;

      for (int i = 0; i < iovcnt; ++i) {
        const char *String{};
        if (CTX->Config.UnifiedMemory) {
          String = reinterpret_cast<const char*>(iovObject[i].iov_base);
        }
        else {
          String = CTX->MemoryMapper.GetPointer<const char*>(reinterpret_cast<uint64_t>(iovObject[i].iov_base));
        }
        for (size_t j = 0; j < iovObject[i].iov_len; ++j) {
          OutputString += String[j];
        }
        FinalSize += iovObject[i].iov_len;
      }

      OutputString += '\0';

      if (FDOffset == STDERR_FILENO)
        LogMan::Msg::ERR("[%ld] %s", FinalSize, OutputString.c_str());
      else
        LogMan::Msg::OUT("[%ld] %s", FinalSize, OutputString.c_str());

      return FinalSize;
    }
  }

  uint64_t write(int fd, void *buf, size_t count) override {
    if (CTX->Config.AccurateSTDOut) {
      return ::write(fd, buf, count);
    }
    else {
      if (FDOffset == STDERR_FILENO)
        LogMan::Msg::ERR("%s", reinterpret_cast<char*>(buf));
      else
        LogMan::Msg::OUT("%s", reinterpret_cast<char*>(buf));
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

class EventFD final : public FD {
public:
  EventFD(FEXCore::Context::Context *ctx, int32_t fd, uint32_t initval, uint32_t flags)
    : FD (ctx, fd, "Event", 0, 0) {
    HostFD = eventfd(initval, flags);
  }

};

class TimerFD final : public FD {
public:
  TimerFD(FEXCore::Context::Context *ctx, int32_t fd, int32_t clockid, int32_t flags)
    : FD (ctx, fd, "Timer", 0, 0) {
    HostFD = timerfd_create(clockid, flags);
  }

};

uint64_t FD::read(int fd, void *buf, size_t count) {
  return ::read(HostFD, buf, count);
}

ssize_t FD::writev(int fd, void *iov, int iovcnt) {
  if (CTX->Config.UnifiedMemory) {
    return ::writev(HostFD, reinterpret_cast<const struct iovec*>(iov), iovcnt);
  }

  const struct iovec *Guestiov = reinterpret_cast<const struct iovec*>(iov);
  std::vector<struct iovec> Hostiov;
  Hostiov.resize(iovcnt);
  for (int i = 0; i < iovcnt; ++i) {
    if (CTX->Config.UnifiedMemory) {
      Hostiov[i].iov_base = reinterpret_cast<void*>(Guestiov[i].iov_base);
    }
    else {
      Hostiov[i].iov_base = CTX->MemoryMapper.GetPointer(reinterpret_cast<uint64_t>(Guestiov[i].iov_base));
    }
    Hostiov[i].iov_len = Guestiov[i].iov_len;
  }

  return ::writev(HostFD, &Hostiov.at(0), iovcnt);
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
  : CTX {ctx}
  , EmuFD {ctx} {

  FDMap[CurrentFDOffset++] = new STDFD{CTX, STDIN_FILENO, "stdin", 0, 0};
  FDMap[CurrentFDOffset++] = new STDFD{CTX, STDOUT_FILENO, "stdout", 0, 0};
  FDMap[CurrentFDOffset++] = new STDFD{CTX, STDERR_FILENO, "stderr", 0, 0};
}

FileManager::~FileManager() {
  for (auto &FD : FDMap) {
    delete FD.second;
  }
}

std::string FileManager::GetEmulatedPath(const char *pathname) {
  if (pathname[0] != '/' ||
      CTX->Config.RootFSPath.empty())
    return {};

  return CTX->Config.RootFSPath + pathname;
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
  auto Path = GetEmulatedPath(pathname);
  if (!Path.empty()) {
    uint64_t Result = ::stat(Path.c_str(), reinterpret_cast<struct stat*>(buf));
    if (Result != -1)
      return Result;
  }
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

  LogMan::Msg::D("Attempting to fstat: %d", fd);
  return -1LL;
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
  auto Path = GetEmulatedPath(pathname);
  if (!Path.empty()) {
    uint64_t Result = ::access(Path.c_str(), mode);
    if (Result != -1)
      return Result;
  }

  int Result = ::access(pathname, mode);
  if (Result == -1)
    return -errno;
  return Result;
}

uint64_t FileManager::FAccessat(int dirfd, const char *pathname, int mode, int flags) {
  auto Path = GetEmulatedPath(pathname);
  if (!Path.empty()) {
    uint64_t Result = ::faccessat(dirfd, Path.c_str(), mode, flags);
    if (Result != -1)
      return Result;
  }

  int Result = ::faccessat(dirfd, pathname, mode, flags);
  if (Result == -1)
    return -errno;
  return Result;
}

uint64_t FileManager::Pipe(int pipefd[2]) {
  int HostFD[2];
  int Result = ::pipe(HostFD);

  {
    int32_t fd = CurrentFDOffset;
    auto fdPtr = new FD{CTX, fd, "<Pipe>", 0, 0};
    fdPtr->SetHostFD(HostFD[0]);
    pipefd[0] = fd;
    FDMap[CurrentFDOffset++] = fdPtr;
  }
  {
    int32_t fd = CurrentFDOffset;
    auto fdPtr = new FD{CTX, fd, "<Pipe>", 0, 0};
    fdPtr->SetHostFD(HostFD[1]);
    pipefd[1] = fd;
    FDMap[CurrentFDOffset++] = fdPtr;
  }

  return Result;
}
uint64_t FileManager::Pipe2(int pipefd[2], int flags) {
  int HostFD[2];
  int Result = ::pipe2(HostFD, flags);

  {
    int32_t fd = CurrentFDOffset;
    auto fdPtr = new FD{CTX, fd, "<Pipe>", 0, 0};
    fdPtr->SetHostFD(HostFD[0]);
    pipefd[0] = fd;
    FDMap[CurrentFDOffset++] = fdPtr;
  }
  {
    int32_t fd = CurrentFDOffset;
    auto fdPtr = new FD{CTX, fd, "<Pipe>", 0, 0};
    fdPtr->SetHostFD(HostFD[1]);
    pipefd[1] = fd;
    FDMap[CurrentFDOffset++] = fdPtr;
  }

  return Result;
}

uint64_t FileManager::Readlink(const char *pathname, char *buf, size_t bufsiz) {
  if (strcmp(pathname, "/proc/self/exe") == 0) {
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
  int32_t fd = CurrentFDOffset;
  if (!strcmp(pathname, "/dev/tty")) {
    FDMap[CurrentFDOffset++] = new STDFD{CTX, STDOUT_FILENO, "/dev/tty", 0, 0};
    return fd;
  }

  auto fdPtr = EmuFD.OpenAt(dirfs, pathname, flags, mode);
  if (!fdPtr) {
    fdPtr = new FD{CTX, fd, pathname, flags, mode};

    uint64_t Result = -1;
    auto Path = GetEmulatedPath(pathname);
    if (!Path.empty()) {
      Result = fdPtr->openat(dirfs, Path.c_str(), flags, mode);
    }

    if (Result == -1)
      Result = fdPtr->openat(dirfs, pathname, flags, mode);

    if (Result == -1) {
      delete fdPtr;
      return -errno;
    }
  }

  FDMap[CurrentFDOffset++] = fdPtr;

  return fd;
}

uint64_t FileManager::Ioctl(int fd, uint64_t request, void *args) {
  auto FD = FDMap.find(fd);
  if (FD == FDMap.end()) {
    return -1;
  }

  uint64_t Result = FD->second->ioctl(fd, request, args);
  if (Result == -1) {
    return -errno;
  }

  return Result;
}

uint64_t FileManager::GetDents(int fd, void *dirp, uint32_t count) {
  auto FD = FDMap.find(fd);
  if (FD == FDMap.end()) {
    return -1;
  }

  return syscall(SYS_getdents64,
      static_cast<uint64_t>(FD->second->GetHostFD()),
      reinterpret_cast<uint64_t>(dirp),
      static_cast<uint64_t>(count));
}

uint64_t FileManager::PRead64(int fd, void *buf, size_t count, off_t offset) {
  auto FD = FDMap.find(fd);
  if (FD == FDMap.end()) {
    return -1;
  }
  return pread(FD->second->GetHostFD(), buf, count, offset);
}

uint64_t FileManager::Statx(int dirfd, const char *pathname, int flags, uint32_t mask, struct statx *statxbuf) {
  // Do we need the dirfd?
  if (pathname[0] == '/' ||
      ((flags & AT_EMPTY_PATH) && pathname[0] == '\0')) {
    auto FD = FDMap.find(dirfd);
    if (FD == FDMap.end()) {
      return EBADF;
    }
    dirfd = FD->second->GetHostFD();
  }
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


uint64_t FileManager::EPoll_Create1(int flags) {
  int HostFD = epoll_create1(flags);
  int32_t fd = CurrentFDOffset;
  auto fdPtr = new FD{CTX, fd, "<EPoll>", 0, 0};
  fdPtr->SetHostFD(HostFD);
  FDMap[CurrentFDOffset++] = fdPtr;
  return fd;
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

uint64_t FileManager::Eventfd(uint32_t initval, uint32_t flags) {
  int32_t fd = CurrentFDOffset;

  auto fdPtr = new EventFD{CTX, fd, initval, flags};

  auto Result = fdPtr->GetHostFD();
  if (Result == -1) {
    delete fdPtr;
    return -errno;
  }

  FDMap[CurrentFDOffset++] = fdPtr;

  return fd;
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

  int Result = connect(FD->second->GetHostFD(), addr, addrlen);
  if (Result == -1) {
    return -errno;
  }
  return Result;
}

uint64_t FileManager::Recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen) {
  auto FD = FDMap.find(sockfd);
  if (FD == FDMap.end()) {
    return -1;
  }

  return recvfrom(FD->second->GetHostFD(), buf, len, flags, src_addr, addrlen);
}

uint64_t FileManager::Sendmsg(int sockfd, const struct msghdr *msg, int flags) {
  LogMan::Throw::A(!CTX->Config.UnifiedMemory, "Not yet supported");
  auto FD = FDMap.find(sockfd);
  if (FD == FDMap.end()) {
    return -1;
  }

  std::vector<iovec> Hostvec;
  struct msghdr Hosthdr;
  memcpy(&Hosthdr, msg, sizeof(struct msghdr));

  if (Hosthdr.msg_name)
    Hosthdr.msg_name = CTX->MemoryMapper.GetPointer(reinterpret_cast<uint64_t>(Hosthdr.msg_name));

  if (Hosthdr.msg_control)
    Hosthdr.msg_control = CTX->MemoryMapper.GetPointer(reinterpret_cast<uint64_t>(Hosthdr.msg_control));

  Hostvec.resize(Hosthdr.msg_iovlen);

  struct iovec *Guestiov = CTX->MemoryMapper.GetPointer<struct iovec*>(reinterpret_cast<uint64_t>(Hosthdr.msg_iov));
  for (int i = 0; i < Hosthdr.msg_iovlen; ++i) {
    Hostvec[i].iov_base = CTX->MemoryMapper.GetPointer(reinterpret_cast<uint64_t>(Guestiov[i].iov_base));
    Hostvec[i].iov_len = Guestiov[i].iov_len;
  }

  Hosthdr.msg_iov = &Hostvec.at(0);

  int Result = sendmsg(FD->second->GetHostFD(), &Hosthdr, flags);
  if (Result == -1) {
    if (errno == EAGAIN ||
        errno == EWOULDBLOCK) {
      return -errno;
    }
  }
  return Result;
}

uint64_t FileManager::Recvmsg(int sockfd, struct msghdr *msg, int flags) {
  auto FD = FDMap.find(sockfd);
  if (FD == FDMap.end()) {
    return -1;
  }

  int Result{};
  if (CTX->Config.UnifiedMemory) {
    Result = recvmsg(FD->second->GetHostFD(), msg, flags);
  }
  else {
    std::vector<iovec> Hostvec;
    struct msghdr Hosthdr;
    memcpy(&Hosthdr, msg, sizeof(struct msghdr));

    if (Hosthdr.msg_name)
      Hosthdr.msg_name = CTX->MemoryMapper.GetPointer(reinterpret_cast<uint64_t>(Hosthdr.msg_name));

    if (Hosthdr.msg_control)
      Hosthdr.msg_control = CTX->MemoryMapper.GetPointer(reinterpret_cast<uint64_t>(Hosthdr.msg_control));

    Hostvec.resize(Hosthdr.msg_iovlen);

    struct iovec *Guestiov = CTX->MemoryMapper.GetPointer<struct iovec*>(reinterpret_cast<uint64_t>(Hosthdr.msg_iov));
    for (int i = 0; i < Hosthdr.msg_iovlen; ++i) {
      Hostvec[i].iov_base = CTX->MemoryMapper.GetPointer(reinterpret_cast<uint64_t>(Guestiov[i].iov_base));
      Hostvec[i].iov_len = Guestiov[i].iov_len;
    }

    Hosthdr.msg_iov = &Hostvec.at(0);

    Result = recvmsg(FD->second->GetHostFD(), &Hosthdr, flags);
  }

  if (Result == -1) {
    if (errno == EAGAIN ||
        errno == EWOULDBLOCK) {
      return -errno;
    }
  }

  if (msg->msg_control && msg->msg_controllen > 0) {
    // Handle the Linux ancillary data
    struct cmsghdr *cmsg = reinterpret_cast<struct cmsghdr*>(msg->msg_control);
    switch (cmsg->cmsg_type) {
      case SCM_RIGHTS: {
        uint32_t *HostFDs = reinterpret_cast<uint32_t*>(CMSG_DATA(cmsg));
        uint32_t NumFiles = (msg->msg_controllen - cmsg->cmsg_len) / sizeof(uint32_t);
        for (size_t i = 0; i < NumFiles; ++i) {
          int32_t fd = CurrentFDOffset;
          auto fdPtr = new FEXCore::FD{CTX, fd, "<Shared>", 0, 0};
          fdPtr->SetHostFD(HostFDs[i]);
          FDMap[CurrentFDOffset++] = fdPtr;

          // Remap host FD to guest
          HostFDs[i] = fd;
        }
        break;
      }
      default: LogMan::Msg::A("Unhandled Ancillary socket type: 0x%x", cmsg->cmsg_type); break;
    }
  }
  return Result;
}

uint64_t FileManager::Shutdown(int sockfd, int how) {
  auto FD = FDMap.find(sockfd);
  if (FD == FDMap.end()) {
    return -1;
  }

  return shutdown(FD->second->GetHostFD(), how);
}

uint64_t FileManager::GetSockName(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
  auto FD = FDMap.find(sockfd);
  if (FD == FDMap.end()) {
    return -1;
  }

  return getsockname(FD->second->GetHostFD(), addr, addrlen);
}

uint64_t FileManager::GetPeerName(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
  auto FD = FDMap.find(sockfd);
  if (FD == FDMap.end()) {
    return -1;
  }

  return getpeername(FD->second->GetHostFD(), addr, addrlen);
}

uint64_t FileManager::SetSockOpt(int sockfd, int level, int optname, const void *optval, socklen_t optlen) {
  auto FD = FDMap.find(sockfd);
  if (FD == FDMap.end()) {
    return -1;
  }

  int Result = ::setsockopt(FD->second->GetHostFD(), level, optname, optval, optlen);
  if (Result == -1)
    return -errno;
  return Result;
}

uint64_t FileManager::GetSockOpt(int sockfd, int level, int optname, void *optval, socklen_t *optlen) {
  auto FD = FDMap.find(sockfd);
  if (FD == FDMap.end()) {
    return -1;
  }

  int Result = ::getsockopt(FD->second->GetHostFD(), level, optname, optval, optlen);
  if (Result == -1)
    return -errno;
  return Result;
}

uint64_t FileManager::Poll(struct pollfd *fds, nfds_t nfds, int timeout) {
  std::vector<pollfd> HostFDs;
  HostFDs.resize(nfds);
  memcpy(&HostFDs.at(0), fds, sizeof(pollfd) * nfds);
  for (auto &FD : HostFDs) {
    auto HostFD = FDMap.find(FD.fd);
    if (HostFD == FDMap.end()) {
      LogMan::Msg::D("Poll. Failed to map FD: %d", FD.fd);
      return -1;
    }
    FD.fd = HostFD->second->GetHostFD();
  }

  int Result = poll(&HostFDs.at(0), nfds, timeout);

  for (int i = 0; i < nfds; ++i) {
    fds[i].revents = HostFDs[i].revents;
  }
  return Result;
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

uint64_t FileManager::Sendmmsg(int sockfd, struct mmsghdr *msgvec, uint32_t vlen, int flags) {
  LogMan::Throw::A(!CTX->Config.UnifiedMemory, "Not yet supported");

  auto FD = FDMap.find(sockfd);
  if (FD == FDMap.end()) {
    return -1;
  }

  std::vector<mmsghdr> Hostmmghdr;
  std::vector<iovec> Hostvec;

  Hostmmghdr.resize(vlen);
  memcpy(&Hostmmghdr.at(0), msgvec, sizeof(mmsghdr) * vlen);

  size_t HostVecSize{};
  for (auto &MHdr : Hostmmghdr) {
    HostVecSize += MHdr.msg_hdr.msg_iovlen;
  }

  Hostvec.resize(HostVecSize);

  size_t HostVecOffset{};
  for (auto &MHdr : Hostmmghdr) {
    struct msghdr &Hosthdr = MHdr.msg_hdr;

    if (Hosthdr.msg_name)
      Hosthdr.msg_name = CTX->MemoryMapper.GetPointer(reinterpret_cast<uint64_t>(Hosthdr.msg_name));

    if (Hosthdr.msg_control)
      Hosthdr.msg_control = CTX->MemoryMapper.GetPointer(reinterpret_cast<uint64_t>(Hosthdr.msg_control));

    struct iovec *Guestiov = CTX->MemoryMapper.GetPointer<struct iovec*>(reinterpret_cast<uint64_t>(Hosthdr.msg_iov));
    for (int i = 0; i < Hosthdr.msg_iovlen; ++i) {
      Hostvec[HostVecOffset + i].iov_base = CTX->MemoryMapper.GetPointer(reinterpret_cast<uint64_t>(Guestiov[i].iov_base));
      Hostvec[HostVecOffset + i].iov_len = Guestiov[i].iov_len;
    }

    Hosthdr.msg_iov = &Hostvec.at(HostVecOffset);

    HostVecOffset += Hosthdr.msg_iovlen;
  }

  int Result = ::sendmmsg(FD->second->GetHostFD(), &Hostmmghdr.at(0), vlen, flags);
  if (Result == -1)
    return -errno;
  return Result;
}

uint64_t FileManager::Timer_Create(int32_t clockid, int32_t flags) {
  int32_t fd = CurrentFDOffset;

  auto fdPtr = new TimerFD{CTX, fd, clockid, flags};

  auto Result = fdPtr->GetHostFD();
  if (Result == -1) {
    delete fdPtr;
    return -errno;
  }

  FDMap[CurrentFDOffset++] = fdPtr;

  return fd;
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

int32_t FileManager::DupFD(int prevFD, int newFD) {
  auto prevFDClass = FDMap.find(prevFD);
  if (prevFDClass == FDMap.end()) {
    return -1;
  }

  int32_t fd = CurrentFDOffset;

  auto fdPtr = new FD{prevFDClass->second, fd};
  fdPtr->SetHostFD(newFD);
  FDMap[CurrentFDOffset++] = fdPtr;

  return fd;
}

}
