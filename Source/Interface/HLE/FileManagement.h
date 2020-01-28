#pragma once
#include <cstdint>
#include <unordered_map>
#include <vector>
#include <sys/stat.h>
#include <sys/socket.h>
#include <poll.h>

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

  uint64_t read(int fd, void *buf, size_t count);
  virtual ssize_t writev(int fd, void *iov, int iovcnt);
  virtual uint64_t write(int fd, void *buf, size_t count);
  int openat(int dirfd, const char *pathname, int flags, mode_t mode);
  int fstat(int fd, struct stat *buf);
  int close(int fd);
  int ioctl(int fd, uint64_t request, void *args);
  int lseek(int fd, off_t offset, int wehnce);

  int GetHostFD() const { return HostFD; }
  void SetHostFD(int fd) { HostFD = fd; }

  std::string const& GetName() const { return PathName; }

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
  uint64_t Lstat(const char *path, void *buf);
  uint64_t Lseek(int fd, uint64_t offset, int whence);
  uint64_t Writev(int fd, void *iov, int iovcnt);
  uint64_t Access(const char *pathname, int mode);
  uint64_t Pipe(int pipefd[2]);
  uint64_t Pipe2(int pipefd[2], int flags);
  uint64_t Readlink(const char *pathname, char *buf, size_t bufsiz);
  uint64_t Openat(int dirfs, const char *pathname, int flags, uint32_t mode);
  uint64_t Ioctl(int fd, uint64_t request, void *args);
  uint64_t GetDents(int fd, void *dirp, uint32_t count);
  uint64_t PRead64(int fd, void *buf, size_t count, off_t offset);
  uint64_t Statx(int dirfd, const char *pathname, int flags, uint32_t mask, struct statx *statxbuf);
  uint64_t Mknod(const char *pathname, mode_t mode, dev_t dev);

  // EPoll
  uint64_t EPoll_Create1(int flags);

  // vfs
  uint64_t Statfs(const char *path, void *buf);

  // Eventfd
  uint64_t Eventfd(uint32_t initval, uint32_t flags);

  // Sockets
  uint64_t Socket(int domain, int type, int protocol);
  uint64_t Connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
  uint64_t Recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
  uint64_t Sendmsg(int sockfd, const struct msghdr *msg, int flags);
  uint64_t Recvmsg(int sockfd, struct msghdr *msg, int flags);
  uint64_t Shutdown(int sockfd, int how);
  uint64_t GetSockName(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
  uint64_t GetPeerName(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
  uint64_t SetSockOpt(int sockfd, int level, int optname, const void *optval, socklen_t optlen);
  uint64_t GetSockOpt(int sockfd, int level, int optname, void *optval, socklen_t *optlen);
  uint64_t Poll(struct pollfd *fds, nfds_t nfds, int timeout);
  uint64_t Select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
  uint64_t Sendmmsg(int sockfd, struct mmsghdr *msgvec, uint32_t vlen, int flags);

  int32_t FindHostFD(int fd);

  FD const* GetFDBacking(int fd);
  int32_t DupFD(int prevFD, int newFD);

  void SetFilename(std::string const &File) { Filename = File; }
  std::string const & GetFilename() const { return Filename; }

private:
  FEXCore::Context::Context *CTX;
  int32_t CurrentFDOffset{0};
  std::unordered_map<int32_t, FD*> FDMap;

  std::string Filename;
  std::string GetEmulatedPath(const char *pathname);
};
}
