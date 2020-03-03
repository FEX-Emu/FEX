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

class STDFD final : public FD {
public:
  STDFD(FEXCore::Context::Context *ctx, int32_t fd, const char *pathname, int32_t flags, mode_t mode)
    : FD (ctx, fd, pathname, flags, mode) {
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

uint64_t FD::read(int fd, void *buf, size_t count) {
  return ::read(fd, buf, count);
}

ssize_t FD::writev(int fd, void *iov, int iovcnt) {
  if (CTX->Config.UnifiedMemory) {
    return ::writev(fd, reinterpret_cast<const struct iovec*>(iov), iovcnt);
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

  return ::writev(fd, &Hostiov.at(0), iovcnt);
}

uint64_t FD::write(int fd, void *buf, size_t count) {
  return ::write(fd, buf, count);
}

int FD::openat(int dirfd, const char *pathname, int flags, mode_t mode) {
  FDOffset  = ::openat(dirfd, pathname, flags, mode);
  return FDOffset;
}

int FD::fstat(int fd, struct stat *buf) {
  return ::fstat(fd, buf);
}

int FD::close(int fd) {
  return ::close(fd);
}

int FD::ioctl(int fd, uint64_t request, void *args) {
  return ::ioctl(fd, request, args);
}

int FD::lseek(int fd, off_t offset, int whence) {
  return ::lseek(fd, offset, whence);
}

FileManager::FileManager(FEXCore::Context::Context *ctx)
  : CTX {ctx}
  , EmuFD {ctx} {
}

FileManager::~FileManager() {
}

std::string FileManager::GetEmulatedPath(const char *pathname) {
  if (pathname[0] != '/' ||
      CTX->Config.RootFSPath.empty())
    return {};

  return CTX->Config.RootFSPath + pathname;
}

uint64_t FileManager::Read(int fd, [[maybe_unused]] void *buf, [[maybe_unused]] size_t count) {
  return ::read(fd, buf, count);
}

uint64_t FileManager::Write(int fd, void *buf, size_t count) {
  return ::write(fd, buf, count);
}

uint64_t FileManager::Open(const char *pathname, [[maybe_unused]] int flags, [[maybe_unused]] uint32_t mode) {
  LogMan::Msg::I("XXX: Trying to open: '%s'", pathname);
  return 0;
}

uint64_t FileManager::Close(int fd) {
  int Result = ::close(fd);
  FDToNameMap.erase(fd);
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
    int Result = ::fstat(fd, &TmpBuf);

    // Blow away access times
    // Causes issues with lockstep runner and file acesses
    memset(&TmpBuf.st_atime, 0, sizeof(time_t));
    memset(&TmpBuf.st_mtime, 0, sizeof(time_t));
    memset(&TmpBuf.st_ctime, 0, sizeof(time_t));
    TmpBuf.st_rdev = 0x8800 + fd;

    memcpy(buf, &TmpBuf, sizeof(struct stat));
    return Result;
  }

  return ::fstat(fd, reinterpret_cast<struct stat*>(buf));
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
  return ::lseek(fd, offset, whence);
}

uint64_t FileManager::Writev(int fd, void *iov, int iovcnt) {
  return ::writev(fd, reinterpret_cast<iovec const *>(iov), iovcnt);
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
  int Result = ::pipe(pipefd);
  return Result;
}
uint64_t FileManager::Pipe2(int pipefd[2], int flags) {
  int Result = ::pipe2(pipefd, flags);
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
  int32_t fd = -1;

  fd = EmuFD.OpenAt(dirfs, pathname, flags, mode);
  if (fd == -1) {
    auto Path = GetEmulatedPath(pathname);
    if (!Path.empty()) {
      fd = ::openat(dirfs, Path.c_str(), flags, mode);
    }

    if (fd == -1)
      fd = ::openat(dirfs, pathname, flags, mode);

    if (fd == -1) {
      return -errno;
    }
  }

  FDToNameMap[fd] = pathname;
  return fd;
}

uint64_t FileManager::Ioctl(int fd, uint64_t request, void *args) {
  uint64_t Result = ::ioctl(fd, request, args);
  if (Result == -1) {
    return -errno;
  }

  return Result;
}

uint64_t FileManager::GetDents(int fd, void *dirp, uint32_t count) {
  return syscall(SYS_getdents64,
      static_cast<uint64_t>(fd),
      reinterpret_cast<uint64_t>(dirp),
      static_cast<uint64_t>(count));
}

uint64_t FileManager::PRead64(int fd, void *buf, size_t count, off_t offset) {
  return pread(fd, buf, count, offset);
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

uint64_t FileManager::Ftruncate(int fd, off_t length) {
  return ftruncate(fd, length);
}

uint64_t FileManager::EPoll_Create1(int flags) {
  return epoll_create1(flags);
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

uint64_t FileManager::FStatfs(int fd, void *buf) {
  return ::fstatfs(fd, reinterpret_cast<struct statfs*>(buf));
}

uint64_t FileManager::Eventfd(uint32_t initval, uint32_t flags) {
  int32_t fd = eventfd(initval, flags);
  if (fd == -1) {
    return -errno;
  }

  return fd;
}

uint64_t FileManager::Socket(int domain, int type, int protocol) {
  int fd = socket(domain, type, protocol);
  if (fd == -1) {
    return -errno;
  }

  return fd;
}

uint64_t FileManager::Connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
  int Result = connect(sockfd, addr, addrlen);
  if (Result == -1) {
    return -errno;
  }
  return Result;
}

uint64_t FileManager::Recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen) {
  int Result = recvfrom(sockfd, buf, len, flags, src_addr, addrlen);
  if (Result == -1) {
    return -errno;
  }
  return Result;

}

uint64_t FileManager::Sendmsg(int sockfd, const struct msghdr *msg, int flags) {
  int Result{};
  if (CTX->Config.UnifiedMemory) {
    Result = sendmsg(sockfd, msg, flags);
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

    Result = sendmsg(sockfd, &Hosthdr, flags);
  }
  if (Result == -1) {
    return -errno;
  }
  return Result;
}

uint64_t FileManager::Recvmsg(int sockfd, struct msghdr *msg, int flags) {
  int Result{};
  if (CTX->Config.UnifiedMemory) {
    Result = recvmsg(sockfd, msg, flags);
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

    Result = recvmsg(sockfd, &Hosthdr, flags);
  }

  if (Result == -1) {
    return -errno;
  }

  return Result;
}

uint64_t FileManager::Shutdown(int sockfd, int how) {
  return shutdown(sockfd, how);
}

uint64_t FileManager::GetSockName(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
  return getsockname(sockfd, addr, addrlen);
}

uint64_t FileManager::GetPeerName(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
  return getpeername(sockfd, addr, addrlen);
}

uint64_t FileManager::SetSockOpt(int sockfd, int level, int optname, const void *optval, socklen_t optlen) {
  int Result = ::setsockopt(sockfd, level, optname, optval, optlen);
  if (Result == -1)
    return -errno;
  return Result;
}

uint64_t FileManager::GetSockOpt(int sockfd, int level, int optname, void *optval, socklen_t *optlen) {
  int Result = ::getsockopt(sockfd, level, optname, optval, optlen);
  if (Result == -1)
    return -errno;
  return Result;
}

uint64_t FileManager::Poll(struct pollfd *fds, nfds_t nfds, int timeout) {
  return ::poll(fds, nfds, timeout);
}

uint64_t FileManager::Select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout) {
   return select(nfds, readfds, writefds, exceptfds, timeout);
}

uint64_t FileManager::Sendmmsg(int sockfd, struct mmsghdr *msgvec, uint32_t vlen, int flags) {
  LogMan::Throw::A(!CTX->Config.UnifiedMemory, "Not yet supported: Sendmmsg");

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

  int Result = ::sendmmsg(sockfd, &Hostmmghdr.at(0), vlen, flags);
  if (Result == -1)
    return -errno;
  return Result;
}

uint64_t FileManager::Timer_Create(int32_t clockid, int32_t flags) {
  int32_t fd = timerfd_create(clockid, flags);
  if (fd == -1) {
    return -errno;
  }
  return fd;
}

uint64_t FileManager::Memfd_Create(const char *name, uint32_t flags) {
  int32_t fd = memfd_create(name, flags);
  if (fd== -1) {
    return -errno;
  }

  return fd;
}

int32_t FileManager::FindHostFD(int fd) {
  return fd;
}

std::string *FileManager::FindFDName(int fd) {
  auto it = FDToNameMap.find(fd);
  if (it == FDToNameMap.end()) {
    return nullptr;
  }
  return &it->second;
}

FD const* FileManager::GetFDBacking(int fd) {
  LogMan::Msg::A("DERP");
  return nullptr;
}

int32_t FileManager::DupFD(int prevFD, int newFD) {
  return ::dup2(prevFD, newFD);
}

}
