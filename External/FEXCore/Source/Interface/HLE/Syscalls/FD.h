#pragma once

#include "Interface/HLE/Syscalls.h"

#include <poll.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>

namespace FEXCore::Core {
struct InternalThreadState;
}

namespace FEXCore::HLE {
  uint64_t Read(FEXCore::Core::InternalThreadState *Thread, int fd, void *buf, size_t count);
  uint64_t Write(FEXCore::Core::InternalThreadState *Thread, int fd, void *buf, size_t count);
  uint64_t Open(FEXCore::Core::InternalThreadState *Thread, const char *pathname, int flags, uint32_t mode);
  uint64_t Close(FEXCore::Core::InternalThreadState *Thread, int fd);
  uint64_t Stat(FEXCore::Core::InternalThreadState *Thread, const char *pathname, FEXCore::guest_stat *buf);
  uint64_t Fstat(FEXCore::Core::InternalThreadState *Thread, int fd, FEXCore::guest_stat *buf);
  uint64_t Lstat(FEXCore::Core::InternalThreadState *Thread, const char *path, FEXCore::guest_stat *buf);
  uint64_t Poll(FEXCore::Core::InternalThreadState *Thread, struct pollfd *fds, nfds_t nfds, int timeout);
  uint64_t Lseek(FEXCore::Core::InternalThreadState *Thread, int fd, uint64_t offset, int whence);
  uint64_t PRead64(FEXCore::Core::InternalThreadState *Thread, int fd, void *buf, size_t count, off_t offset);
  uint64_t PWrite64(FEXCore::Core::InternalThreadState *Thread, int fd, void *buf, size_t count, off_t offset);
  uint64_t Readv(FEXCore::Core::InternalThreadState *Thread, int fd, const struct iovec *iov, int iovcnt);
  uint64_t Writev(FEXCore::Core::InternalThreadState *Thread, int fd, const struct iovec *iov, int iovcnt);
  uint64_t Access(FEXCore::Core::InternalThreadState *Thread, const char *pathname, int mode);
  uint64_t Pipe(FEXCore::Core::InternalThreadState *Thread, int pipefd[2]);
  uint64_t Select(FEXCore::Core::InternalThreadState *Thread, int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
  uint64_t Dup(FEXCore::Core::InternalThreadState *Thread, int oldfd);
  uint64_t Dup2(FEXCore::Core::InternalThreadState *Thread, int oldfd, int newfd);
  uint64_t Fcntl(FEXCore::Core::InternalThreadState *Thread, int fd, int cmd, uint64_t arg);
  uint64_t Ftruncate(FEXCore::Core::InternalThreadState *Thread, int fd, off_t length);
  uint64_t Getdents64(FEXCore::Core::InternalThreadState *Thread, int fd, void *dirp, uint32_t count);
  uint64_t Fadvise64(FEXCore::Core::InternalThreadState *Thread, int fd, off_t offset, off_t len, int advice);
  uint64_t Inotify_init(FEXCore::Core::InternalThreadState *Thread);
  uint64_t Inotify_add_watch(FEXCore::Core::InternalThreadState *Thread, int fd, const char *pathname, uint32_t mask);
  uint64_t Inotify_rm_watch(FEXCore::Core::InternalThreadState *Thread, int fd, int wd);
  uint64_t Openat(FEXCore::Core::InternalThreadState *Thread, int dirfs, const char *pathname, int flags, uint32_t mode);
  uint64_t NewFStatat(FEXCore::Core::InternalThreadState *Thread, int dirfd, const char *pathname, FEXCore::guest_stat *buf, int flag);
  uint64_t Readlinkat(FEXCore::Core::InternalThreadState *Thread, int dirfd, const char *pathname, char *buf, size_t bufsiz);
  uint64_t FAccessat(FEXCore::Core::InternalThreadState *Thread, int dirfd, const char *pathname, int mode, int flags);
  uint64_t Timerfd_Create(FEXCore::Core::InternalThreadState *Thread, int32_t clockid, int32_t flags);
  uint64_t Eventfd(FEXCore::Core::InternalThreadState *Thread, uint32_t initval, uint32_t flags);
  uint64_t Pipe2(FEXCore::Core::InternalThreadState *Thread, int pipefd[2], int flags);
  uint64_t Memfd_Create(FEXCore::Core::InternalThreadState *Thread, const char *name, uint32_t flags);
  uint64_t Statx(FEXCore::Core::InternalThreadState *Thread, int dirfd, const char *pathname, int flags, uint32_t mask, struct statx *statxbuf);
  uint64_t Ppoll(FEXCore::Core::InternalThreadState *Thread, struct pollfd *fds, nfds_t nfds, const struct timespec *timeout_ts, const sigset_t *sigmask, size_t sigsetsize);
}
