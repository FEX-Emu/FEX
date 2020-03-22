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

  FD(FD *Prev, int32_t fd)
    : CTX {Prev->CTX}
    , FDOffset {fd}
    , PathName {Prev->PathName}
    , Flags {Prev->Flags}
    , Mode {Prev->Mode} {
  }
  virtual ~FD() {}

  virtual uint64_t read(int fd, void *buf, size_t count);
  virtual ssize_t writev(int fd, void *iov, int iovcnt);
  virtual uint64_t write(int fd, void *buf, size_t count);
  int openat(int dirfd, const char *pathname, int flags, mode_t mode);
  int fstat(int fd, struct stat *buf);
  int close(int fd);
  int ioctl(int fd, uint64_t request, void *args);
  int lseek(int fd, off_t offset, int wehnce);

  int GetHostFD() const { return FDOffset; }
  void SetHostFD(int fd) { FDOffset = fd; }

  std::string const& GetName() const { return PathName; }

protected:
  FEXCore::Context::Context *CTX;
  [[maybe_unused]] int32_t FDOffset{};
  std::string PathName;
  [[maybe_unused]] int32_t Flags;
  [[maybe_unused]] mode_t Mode;
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
  uint64_t Lstat(const char *path, void *buf);
  uint64_t Lseek(int fd, uint64_t offset, int whence);
  uint64_t Writev(int fd, void *iov, int iovcnt);
  uint64_t Access(const char *pathname, int mode);
  uint64_t FAccessat(int dirfd, const char *pathname, int mode, int flags);
  uint64_t Pipe(int pipefd[2]);
  uint64_t Pipe2(int pipefd[2], int flags);
  uint64_t Readlink(const char *pathname, char *buf, size_t bufsiz);
  uint64_t Chmod(const char *pathname, mode_t mode);
  uint64_t NewFStatAt(int dirfd, const char *pathname, struct stat *buf, int flag);
  uint64_t Readlinkat(int dirfd, const char *pathname, char *buf, size_t bufsiz);
  uint64_t Openat(int dirfs, const char *pathname, int flags, uint32_t mode);
  uint64_t Ioctl(int fd, uint64_t request, void *args);
  uint64_t GetDents(int fd, void *dirp, uint32_t count);
  uint64_t PRead64(int fd, void *buf, size_t count, off_t offset);
  uint64_t Statx(int dirfd, const char *pathname, int flags, uint32_t mask, struct statx *statxbuf);
  uint64_t Mknod(const char *pathname, mode_t mode, dev_t dev);
  uint64_t Ftruncate(int fd, off_t length);

  // EPoll
  uint64_t EPoll_Create1(int flags);
  uint64_t EPoll_Ctl(int epfd, int op, int fd, void *event);
  uint64_t EPoll_Pwait(int epfd, void *events, int maxevent, int timeout, const void* sigmask);

  // vfs
  uint64_t Statfs(const char *path, void *buf);
  uint64_t FStatfs(int fd, void *buf);

  // Eventfd
  uint64_t Eventfd(uint32_t initval, uint32_t flags);

  // Sockets
  uint64_t Socket(int domain, int type, int protocol);
  uint64_t Connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
  uint64_t Sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);
  uint64_t Recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
  uint64_t Sendmsg(int sockfd, const struct msghdr *msg, int flags);
  uint64_t Recvmsg(int sockfd, struct msghdr *msg, int flags);
  uint64_t Shutdown(int sockfd, int how);
  uint64_t Bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
  uint64_t Listen(int sockfd, int backlog);
  uint64_t GetSockName(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
  uint64_t GetPeerName(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
  uint64_t SetSockOpt(int sockfd, int level, int optname, const void *optval, socklen_t optlen);
  uint64_t GetSockOpt(int sockfd, int level, int optname, void *optval, socklen_t *optlen);
  uint64_t Poll(struct pollfd *fds, nfds_t nfds, int timeout);
  uint64_t Select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
  uint64_t Sendmmsg(int sockfd, struct mmsghdr *msgvec, uint32_t vlen, int flags);

  // Timers
  uint64_t Timer_Create(int32_t clockid, int32_t flags);

  // MemFD
  uint64_t Memfd_Create(const char *name, uint32_t flags);

  // Syslog
  uint64_t Syslog(int type, char *bufp, int len);

  int32_t FindHostFD(int fd);

  std::string *FindFDName(int fd);
  FD const* GetFDBacking(int fd);
  int32_t DupFD(int prevFD);
  int32_t DupFD(int prevFD, int newFD);

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
