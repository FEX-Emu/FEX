#include "Interface/HLE/Syscalls.h"
#include "Interface/HLE/x64/Syscalls.h"
#include "Interface/Context/Context.h"

#include <fcntl.h>
#include <stdint.h>
#include <sys/file.h>
#include <sys/eventfd.h>
#include <sys/inotify.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/timerfd.h>
#include <sys/uio.h>
#include <poll.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

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

  static int RemapFlags(int flags) {
#ifdef _M_X86_64
    // Nothing to change here
#elif _M_ARM_64
    constexpr int X86_64_FLAG_O_DIRECT    =  040000;
    constexpr int X86_64_FLAG_O_LARGEFILE = 0100000;
    constexpr int X86_64_FLAG_O_DIRECTORY = 0200000;
    constexpr int X86_64_FLAG_O_NOFOLLOW  = 0400000;
    if (flags & X86_64_FLAG_O_DIRECT)    flags = (flags & ~X86_64_FLAG_O_DIRECT)    | O_DIRECT;
    if (flags & X86_64_FLAG_O_LARGEFILE) flags = (flags & ~X86_64_FLAG_O_LARGEFILE) | O_LARGEFILE;
    if (flags & X86_64_FLAG_O_DIRECTORY) flags = (flags & ~X86_64_FLAG_O_DIRECTORY) | O_DIRECTORY;
    if (flags & X86_64_FLAG_O_NOFOLLOW)  flags = (flags & ~X86_64_FLAG_O_NOFOLLOW)  | O_NOFOLLOW;
#else
#error Unknown flag remappings for this host platform
#endif
    return flags;
  }

  void RegisterFD() {
    REGISTER_SYSCALL_IMPL(read, [](FEXCore::Core::InternalThreadState *Thread, int fd, void *buf, size_t count) -> uint64_t {
      uint64_t Result = ::read(fd, buf, count);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(write, [](FEXCore::Core::InternalThreadState *Thread, int fd, void *buf, size_t count) -> uint64_t {
      uint64_t Result = ::write(fd, buf, count);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(open, [](FEXCore::Core::InternalThreadState *Thread, const char *pathname, int flags, uint32_t mode) -> uint64_t {
      flags = RemapFlags(flags);
      uint64_t Result = Thread->CTX->SyscallHandler->FM.Open(pathname, flags, mode);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(close, [](FEXCore::Core::InternalThreadState *Thread, int fd) -> uint64_t {
      uint64_t Result = Thread->CTX->SyscallHandler->FM.Close(fd);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(stat, [](FEXCore::Core::InternalThreadState *Thread, const char *pathname, FEXCore::guest_stat *buf) -> uint64_t {
      struct stat host_stat;
      uint64_t Result = Thread->CTX->SyscallHandler->FM.Stat(pathname, &host_stat);
      if (Result != -1) {
        CopyStat(buf, &host_stat);
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(fstat, [](FEXCore::Core::InternalThreadState *Thread, int fd, FEXCore::guest_stat *buf) -> uint64_t {
      struct stat host_stat;
      uint64_t Result = ::fstat(fd, &host_stat);
      if (Result != -1) {
        CopyStat(buf, &host_stat);
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(lstat, [](FEXCore::Core::InternalThreadState *Thread, const char *path, FEXCore::guest_stat *buf) -> uint64_t {
      struct stat host_stat;
      uint64_t Result = Thread->CTX->SyscallHandler->FM.Lstat(path, &host_stat);
      if (Result != -1) {
        CopyStat(buf, &host_stat);
      }
      SYSCALL_ERRNO();
    });

    /*
    REGISTER_SYSCALL_IMPL(chown, [](FEXCore::Core::InternalThreadState *Thread, const char *pathname, uid_t owner, gid_t group) -> uint64_t {
      SYSCALL_STUB(chown);
    });*/
    REGISTER_SYSCALL_FORWARD_ERRNO(chown);

    /*
    REGISTER_SYSCALL_IMPL(fchown, [](FEXCore::Core::InternalThreadState *Thread, int fd, uid_t owner, gid_t group) -> uint64_t {
      SYSCALL_STUB(fchown);
    });*/
    REGISTER_SYSCALL_FORWARD_ERRNO(fchown);

    /*
    REGISTER_SYSCALL_IMPL(lchown, [](FEXCore::Core::InternalThreadState *Thread, const char *pathname, uid_t owner, gid_t group) -> uint64_t {
      SYSCALL_STUB(lchown);
    });*/
    REGISTER_SYSCALL_FORWARD_ERRNO(lchown);

    REGISTER_SYSCALL_IMPL(poll, [](FEXCore::Core::InternalThreadState *Thread, struct pollfd *fds, nfds_t nfds, int timeout) -> uint64_t {
      uint64_t Result = ::poll(fds, nfds, timeout);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(lseek, [](FEXCore::Core::InternalThreadState *Thread, int fd, uint64_t offset, int whence) -> uint64_t {
      uint64_t Result = ::lseek(fd, offset, whence);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(pread64, [](FEXCore::Core::InternalThreadState *Thread, int fd, void *buf, size_t count, off_t offset) -> uint64_t {
      uint64_t Result = ::pread64(fd, buf, count, offset);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(pwrite64, [](FEXCore::Core::InternalThreadState *Thread, int fd, void *buf, size_t count, off_t offset) -> uint64_t {
      uint64_t Result = ::pwrite64(fd, buf, count, offset);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(readv, [](FEXCore::Core::InternalThreadState *Thread, int fd, const struct iovec *iov, int iovcnt) -> uint64_t {
      uint64_t Result = ::readv(fd, iov, iovcnt);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(writev, [](FEXCore::Core::InternalThreadState *Thread, int fd, const struct iovec *iov, int iovcnt) -> uint64_t {
      uint64_t Result = ::writev(fd, iov, iovcnt);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(access, [](FEXCore::Core::InternalThreadState *Thread, const char *pathname, int mode) -> uint64_t {
      uint64_t Result = Thread->CTX->SyscallHandler->FM.Access(pathname, mode);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(pipe, [](FEXCore::Core::InternalThreadState *Thread, int pipefd[2]) -> uint64_t {
      uint64_t Result = ::pipe(pipefd);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(select, [](FEXCore::Core::InternalThreadState *Thread, int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout) -> uint64_t {
      uint64_t Result = ::select(nfds, readfds, writefds, exceptfds, timeout);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(dup, [](FEXCore::Core::InternalThreadState *Thread, int oldfd) -> uint64_t {
      uint64_t Result = ::dup(oldfd);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(dup2, [](FEXCore::Core::InternalThreadState *Thread, int oldfd, int newfd) -> uint64_t {
      uint64_t Result = ::dup2(oldfd, newfd);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(dup3, [](FEXCore::Core::InternalThreadState* Thread, int oldfd, int newfd, int flags) -> uint64_t {
      flags = RemapFlags(flags);
      uint64_t Result = ::dup3(oldfd, newfd, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(fcntl, [](FEXCore::Core::InternalThreadState *Thread, int fd, int cmd, uint64_t arg) -> uint64_t {
      uint64_t Result = ::fcntl(fd, cmd, arg);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(flock, [](FEXCore::Core::InternalThreadState *Thread, int fd, int operation) -> uint64_t {
      uint64_t Result = ::flock(fd, operation);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(fsync, [](FEXCore::Core::InternalThreadState *Thread, int fd) -> uint64_t {
      uint64_t Result = ::fsync(fd);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(fdatasync, [](FEXCore::Core::InternalThreadState *Thread, int fd) -> uint64_t {
      uint64_t Result = ::fdatasync(fd);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(ftruncate, [](FEXCore::Core::InternalThreadState *Thread, int fd, off_t length) -> uint64_t {
      uint64_t Result = ::ftruncate(fd, length);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(fallocate, [](FEXCore::Core::InternalThreadState *Thread, int fd, int mode, off_t offset, off_t len) -> uint64_t {
      uint64_t Result = ::fallocate(fd, mode, offset, len);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(getdents, [](FEXCore::Core::InternalThreadState *Thread, int fd, void *dirp, uint32_t count) -> uint64_t {
  #ifdef SYS_getdents
      uint64_t Result = syscall(SYS_getdents,
        static_cast<uint64_t>(fd),
        reinterpret_cast<uint64_t>(dirp),
        static_cast<uint64_t>(count));
      SYSCALL_ERRNO();
  #else
      // XXX: Emulate
      return -EFAULT;
  #endif
    });

    REGISTER_SYSCALL_IMPL(fchmod, [](FEXCore::Core::InternalThreadState *Thread, int fd, int mode) -> uint64_t {
      uint64_t Result = ::fchmod(fd, mode);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(getdents64, [](FEXCore::Core::InternalThreadState *Thread, int fd, void *dirp, uint32_t count) -> uint64_t {
      uint64_t Result = syscall(SYS_getdents64,
        static_cast<uint64_t>(fd),
        reinterpret_cast<uint64_t>(dirp),
        static_cast<uint64_t>(count));
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(fadvise64, [](FEXCore::Core::InternalThreadState *Thread, int fd, off_t offset, off_t len, int advice) -> uint64_t {
      uint64_t Result = ::posix_fadvise64(fd, offset, len, advice);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(inotify_init, [](FEXCore::Core::InternalThreadState *Thread) -> uint64_t {
      uint64_t Result = ::inotify_init();
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(inotify_add_watch, [](FEXCore::Core::InternalThreadState *Thread, int fd, const char *pathname, uint32_t mask) -> uint64_t {
      uint64_t Result = ::inotify_add_watch(fd, pathname, mask);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(inotify_rm_watch, [](FEXCore::Core::InternalThreadState *Thread, int fd, int wd) -> uint64_t {
      uint64_t Result = ::inotify_rm_watch(fd, wd);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(openat, [](FEXCore::Core::InternalThreadState *Thread, int dirfs, const char *pathname, int flags, uint32_t mode) -> uint64_t {
      flags = RemapFlags(flags);
      uint64_t Result = Thread->CTX->SyscallHandler->FM.Openat(dirfs, pathname, flags, mode);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(newfstatat, [](FEXCore::Core::InternalThreadState *Thread, int dirfd, const char *pathname, FEXCore::guest_stat *buf, int flag) -> uint64_t {
      struct stat host_stat;
      uint64_t Result = ::fstatat(dirfd, pathname, &host_stat, flag);
      if (Result != -1) {
        CopyStat(buf, &host_stat);
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(readlinkat, [](FEXCore::Core::InternalThreadState *Thread, int dirfd, const char *pathname, char *buf, size_t bufsiz) -> uint64_t {
      uint64_t Result = Thread->CTX->SyscallHandler->FM.Readlinkat(dirfd, pathname, buf, bufsiz);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(faccessat, [](FEXCore::Core::InternalThreadState *Thread, int dirfd, const char *pathname, int mode, int flags) -> uint64_t {
      uint64_t Result = Thread->CTX->SyscallHandler->FM.FAccessat(dirfd, pathname, mode, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(timerfd_create, [](FEXCore::Core::InternalThreadState *Thread, int32_t clockid, int32_t flags) -> uint64_t {
      uint64_t Result = ::timerfd_create(clockid, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(timerfd_settime, [](FEXCore::Core::InternalThreadState *Thread, int fd, int flags, const struct itimerspec *new_value, struct itimerspec *old_value) -> uint64_t {
      uint64_t Result = ::timerfd_settime(fd, flags, new_value, old_value);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(timerfd_gettime, [](FEXCore::Core::InternalThreadState *Thread, int fd, struct itimerspec *curr_value) -> uint64_t {
      uint64_t Result = ::timerfd_gettime(fd, curr_value);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(eventfd, [](FEXCore::Core::InternalThreadState *Thread, uint32_t initval, uint32_t flags) -> uint64_t {
      uint64_t Result = ::eventfd(initval, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(pipe2, [](FEXCore::Core::InternalThreadState *Thread, int pipefd[2], int flags) -> uint64_t {
      flags = RemapFlags(flags);
      uint64_t Result = ::pipe2(pipefd, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(inotify_init1, [](FEXCore::Core::InternalThreadState *Thread, int flags) -> uint64_t {
      flags = RemapFlags(flags);
      uint64_t Result = ::inotify_init1(flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(memfd_create, [](FEXCore::Core::InternalThreadState *Thread, const char *name, uint32_t flags) -> uint64_t {
      uint64_t Result = ::memfd_create(name, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(statx, [](FEXCore::Core::InternalThreadState *Thread, int dirfd, const char *pathname, int flags, uint32_t mask, struct statx *statxbuf) -> uint64_t {
      uint64_t Result = Thread->CTX->SyscallHandler->FM.Statx(dirfd, pathname, flags, mask, statxbuf);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(pselect6, [](FEXCore::Core::InternalThreadState *Thread, int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, const struct timespec *timeout, const sigset_t *sigmask) -> uint64_t {
      uint64_t Result = pselect(nfds, readfds, writefds, exceptfds, timeout, sigmask);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(ppoll, [](FEXCore::Core::InternalThreadState *Thread, struct pollfd *fds, nfds_t nfds, const struct timespec *timeout_ts, const sigset_t *sigmask, size_t sigsetsize) -> uint64_t {
      // sigsetsize is unused here since it is currently a constant and not exposed through glibc
      uint64_t Result = ::ppoll(fds, nfds, timeout_ts, sigmask);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(name_to_handle_at, [](FEXCore::Core::InternalThreadState *Thread, int dirfd, const char *pathname, struct file_handle *handle, int *mount_id, int flags) -> uint64_t {
      flags = RemapFlags(flags);
      uint64_t Result = ::name_to_handle_at(dirfd, pathname, handle, mount_id, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(open_by_handle_at, [](FEXCore::Core::InternalThreadState *Thread, int mount_fd, struct file_handle *handle, int flags) -> uint64_t {
      flags = RemapFlags(flags);
      uint64_t Result = ::open_by_handle_at(mount_fd, handle, flags);
      SYSCALL_ERRNO();
    });
  }
}
