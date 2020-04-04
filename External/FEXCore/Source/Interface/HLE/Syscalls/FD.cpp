#include "Interface/HLE/Syscalls.h"
#include "Interface/HLE/Syscalls/FD.h"
#include "Interface/Context/Context.h"

#include <fcntl.h>
#include <stdint.h>
#include <sys/eventfd.h>
#include <sys/inotify.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/timerfd.h>
#include <sys/uio.h>

namespace FEXCore::HLE {
  static void CopyStat(FEXCore::guest_stat *guest, struct stat *host) {
#define COPY(x) guest->x = host->x
    COPY(st_dev);
    COPY(st_ino);
    COPY(st_nlink);

    COPY(st_mode);
    COPY(st_uid);
    COPY(st_gid);

    COPY(st_rdev);
    COPY(st_size);
    COPY(st_blksize);
    COPY(st_blocks);

    guest->st_atime_ = host->st_atim.tv_sec;
    guest->st_atime_nsec = host->st_atim.tv_nsec;

    guest->st_mtime_ = host->st_mtime;
    guest->st_mtime_nsec = host->st_mtim.tv_nsec;

    guest->st_ctime_ = host->st_ctime;
    guest->st_ctime_nsec = host->st_ctim.tv_nsec;
#undef COPY
  }

  uint64_t Read(FEXCore::Core::InternalThreadState *Thread, int fd, void *buf, size_t count) {
    uint64_t Result = ::read(fd, buf, count);
    SYSCALL_ERRNO();
  }

  uint64_t Write(FEXCore::Core::InternalThreadState *Thread, int fd, void *buf, size_t count) {
    uint64_t Result = ::write(fd, buf, count);
    SYSCALL_ERRNO();
  }

  uint64_t Open(FEXCore::Core::InternalThreadState *Thread, const char *pathname, int flags, uint32_t mode) {
    uint64_t Result = Thread->CTX->SyscallHandler->FM.Open(pathname, flags, mode);
    SYSCALL_ERRNO();
  }

  uint64_t Close(FEXCore::Core::InternalThreadState *Thread, int fd) {
    uint64_t Result = Thread->CTX->SyscallHandler->FM.Close(fd);
    SYSCALL_ERRNO();
  }

  uint64_t Stat(FEXCore::Core::InternalThreadState *Thread, const char *pathname, FEXCore::guest_stat *buf) {
    struct stat host_stat;
    uint64_t Result = Thread->CTX->SyscallHandler->FM.Stat(pathname, &host_stat);
    if (Result != -1) {
      CopyStat(buf, &host_stat);
    }
    SYSCALL_ERRNO();
  }
  uint64_t Fstat(FEXCore::Core::InternalThreadState *Thread, int fd, FEXCore::guest_stat *buf) {
    struct stat host_stat;
    uint64_t Result = ::fstat(fd, &host_stat);
    if (Result != -1) {
      CopyStat(buf, &host_stat);
    }
    SYSCALL_ERRNO();
  }

  uint64_t Lstat(FEXCore::Core::InternalThreadState *Thread, const char *path, FEXCore::guest_stat *buf) {
    struct stat host_stat;
    uint64_t Result = Thread->CTX->SyscallHandler->FM.Lstat(path, &host_stat);
    if (Result != -1) {
      CopyStat(buf, &host_stat);
    }
    SYSCALL_ERRNO();
  }

  uint64_t Poll(FEXCore::Core::InternalThreadState *Thread, struct pollfd *fds, nfds_t nfds, int timeout) {
    uint64_t Result = ::poll(fds, nfds, timeout);
    SYSCALL_ERRNO();
  }

  uint64_t Lseek(FEXCore::Core::InternalThreadState *Thread, int fd, uint64_t offset, int whence) {
    uint64_t Result = ::lseek(fd, offset, whence);
    SYSCALL_ERRNO();
  }

  uint64_t PRead64(FEXCore::Core::InternalThreadState *Thread, int fd, void *buf, size_t count, off_t offset) {
    uint64_t Result = ::pread64(fd, buf, count, offset);
    SYSCALL_ERRNO();
  }

  uint64_t PWrite64(FEXCore::Core::InternalThreadState *Thread, int fd, void *buf, size_t count, off_t offset) {
    uint64_t Result = ::pwrite64(fd, buf, count, offset);
    SYSCALL_ERRNO();
  }

  uint64_t Readv(FEXCore::Core::InternalThreadState *Thread, int fd, const struct iovec *iov, int iovcnt) {
    uint64_t Result = ::readv(fd, iov, iovcnt);
    SYSCALL_ERRNO();
  }

  uint64_t Writev(FEXCore::Core::InternalThreadState *Thread, int fd, const struct iovec *iov, int iovcnt) {
    uint64_t Result = ::writev(fd, iov, iovcnt);
    SYSCALL_ERRNO();
  }

  uint64_t Access(FEXCore::Core::InternalThreadState *Thread, const char *pathname, int mode) {
    uint64_t Result = Thread->CTX->SyscallHandler->FM.Access(pathname, mode);
    SYSCALL_ERRNO();
  }

  uint64_t Pipe(FEXCore::Core::InternalThreadState *Thread, int pipefd[2]) {
    uint64_t Result = ::pipe(pipefd);
    SYSCALL_ERRNO();
  }

  uint64_t Select(FEXCore::Core::InternalThreadState *Thread, int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout) {
    uint64_t Result = ::select(nfds, readfds, writefds, exceptfds, timeout);
    SYSCALL_ERRNO();
  }

  uint64_t Dup(FEXCore::Core::InternalThreadState *Thread, int oldfd) {
    uint64_t Result = ::dup(oldfd);
    SYSCALL_ERRNO();
  }

  uint64_t Dup2(FEXCore::Core::InternalThreadState *Thread, int oldfd, int newfd) {
    uint64_t Result = ::dup2(oldfd, newfd);
    SYSCALL_ERRNO();
  }

  uint64_t Fcntl(FEXCore::Core::InternalThreadState *Thread, int fd, int cmd, uint64_t arg) {
    uint64_t Result = ::fcntl(fd, cmd, arg);
    SYSCALL_ERRNO();
  }

  uint64_t Ftruncate(FEXCore::Core::InternalThreadState *Thread, int fd, off_t length) {
    uint64_t Result = ::ftruncate(fd, length);
    SYSCALL_ERRNO();
  }

  uint64_t Getdents64(FEXCore::Core::InternalThreadState *Thread, int fd, void *dirp, uint32_t count) {
    return syscall(SYS_getdents64,
      static_cast<uint64_t>(fd),
      reinterpret_cast<uint64_t>(dirp),
      static_cast<uint64_t>(count));
  }

  uint64_t Fadvise64(FEXCore::Core::InternalThreadState *Thread, int fd, off_t offset, off_t len, int advice) {
    uint64_t Result = ::posix_fadvise64(fd, offset, len, advice);
    SYSCALL_ERRNO();
  }

  uint64_t Inotify_init(FEXCore::Core::InternalThreadState *Thread) {
    uint64_t Result = ::inotify_init();
    SYSCALL_ERRNO();
  }

  uint64_t Inotify_add_watch(FEXCore::Core::InternalThreadState *Thread, int fd, const char *pathname, uint32_t mask) {
    uint64_t Result = ::inotify_add_watch(fd, pathname, mask);
    SYSCALL_ERRNO();
  }

  uint64_t Inotify_rm_watch(FEXCore::Core::InternalThreadState *Thread, int fd, int wd) {
    uint64_t Result = ::inotify_rm_watch(fd, wd);
    SYSCALL_ERRNO();
  }

  uint64_t Openat(FEXCore::Core::InternalThreadState *Thread, int dirfs, const char *pathname, int flags, uint32_t mode) {
    uint64_t Result = Thread->CTX->SyscallHandler->FM.Openat(dirfs, pathname, flags, mode);
    SYSCALL_ERRNO();
  }

  uint64_t NewFStatat(FEXCore::Core::InternalThreadState *Thread, int dirfd, const char *pathname, FEXCore::guest_stat *buf, int flag) {
    struct stat host_stat;
    uint64_t Result = ::fstatat(dirfd, pathname, &host_stat, flag);
    if (Result != -1) {
      CopyStat(buf, &host_stat);
    }
    SYSCALL_ERRNO();
  }

  uint64_t Readlinkat(FEXCore::Core::InternalThreadState *Thread, int dirfd, const char *pathname, char *buf, size_t bufsiz) {
    uint64_t Result = Thread->CTX->SyscallHandler->FM.Readlinkat(dirfd, pathname, buf, bufsiz);
    SYSCALL_ERRNO();
  }

  uint64_t FAccessat(FEXCore::Core::InternalThreadState *Thread, int dirfd, const char *pathname, int mode, int flags) {
    uint64_t Result = Thread->CTX->SyscallHandler->FM.FAccessat(dirfd, pathname, mode, flags);
    SYSCALL_ERRNO();
  }

  uint64_t Timerfd_Create(FEXCore::Core::InternalThreadState *Thread, int32_t clockid, int32_t flags) {
    uint64_t Result = ::timerfd_create(clockid, flags);
    SYSCALL_ERRNO();
  }

  uint64_t Eventfd(FEXCore::Core::InternalThreadState *Thread, uint32_t initval, uint32_t flags) {
    uint64_t Result = ::eventfd(initval, flags);
    SYSCALL_ERRNO();
  }

  uint64_t Pipe2(FEXCore::Core::InternalThreadState *Thread, int pipefd[2], int flags) {
    uint64_t Result = ::pipe2(pipefd, flags);
    SYSCALL_ERRNO();
   }

  uint64_t Memfd_Create(FEXCore::Core::InternalThreadState *Thread, const char *name, uint32_t flags) {
    uint64_t Result = ::memfd_create(name, flags);
    SYSCALL_ERRNO();
  }

  uint64_t Statx(FEXCore::Core::InternalThreadState *Thread, int dirfd, const char *pathname, int flags, uint32_t mask, struct statx *statxbuf) {
    uint64_t Result = Thread->CTX->SyscallHandler->FM.Statx(dirfd, pathname, flags, mask, statxbuf);
    SYSCALL_ERRNO();
  }

  uint64_t Ppoll(FEXCore::Core::InternalThreadState *Thread, struct pollfd *fds, nfds_t nfds, const struct timespec *timeout_ts, const sigset_t *sigmask, size_t sigsetsize) {
    // sigsetsize is unused here since it is currently a constant and not exposed through glibc
    uint64_t Result = ::ppoll(fds, nfds, timeout_ts, sigmask);
    SYSCALL_ERRNO();
  }
}
