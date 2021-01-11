#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/x64/Syscalls.h"
#include <FEXCore/Core/Context.h>

#include <fcntl.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace FEX::HLE::x64 {
  struct __attribute__((packed)) guest_stat {
    __kernel_ulong_t  st_dev;
    __kernel_ulong_t  st_ino;
    __kernel_ulong_t  st_nlink;

    unsigned int    st_mode;
    unsigned int    st_uid;
    unsigned int    st_gid;
    unsigned int    __pad0;
    __kernel_ulong_t  st_rdev;
    __kernel_long_t   st_size;
    __kernel_long_t   st_blksize;
    __kernel_long_t   st_blocks;  /* Number 512-byte blocks allocated. */

    __kernel_ulong_t  st_atime_;
    __kernel_ulong_t  st_atime_nsec;
    __kernel_ulong_t  st_mtime_;
    __kernel_ulong_t  st_mtime_nsec;
    __kernel_ulong_t  st_ctime_;
    __kernel_ulong_t  st_ctime_nsec;
    __kernel_long_t   __unused[3];
  };

  static void CopyStat(guest_stat *guest, struct stat *host) {
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

  void RegisterFD() {
    REGISTER_SYSCALL_IMPL_X64(poll, [](FEXCore::Core::InternalThreadState *Thread, struct pollfd *fds, nfds_t nfds, int timeout) -> uint64_t {
      uint64_t Result = ::poll(fds, nfds, timeout);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(select, [](FEXCore::Core::InternalThreadState *Thread, int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout) -> uint64_t {
      uint64_t Result = ::select(nfds, readfds, writefds, exceptfds, timeout);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(fcntl, [](FEXCore::Core::InternalThreadState *Thread, int fd, int cmd, uint64_t arg) -> uint64_t {
      uint64_t Result = ::fcntl(fd, cmd, arg);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(futimesat, [](FEXCore::Core::InternalThreadState *Thread, int dirfd, const char *pathname, const struct timeval times[2]) -> uint64_t {
      uint64_t Result = ::futimesat(dirfd, pathname, times);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(utimensat, [](FEXCore::Core::InternalThreadState *Thread, int dirfd, const char *pathname, const struct timespec times[2], int flags) -> uint64_t {
      uint64_t Result = ::utimensat(dirfd, pathname, times, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(pselect6, [](FEXCore::Core::InternalThreadState *Thread, int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, const struct timespec *timeout, const sigset_t *sigmask) -> uint64_t {
      uint64_t Result = pselect(nfds, readfds, writefds, exceptfds, timeout, sigmask);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(stat, [](FEXCore::Core::InternalThreadState *Thread, const char *pathname, guest_stat *buf) -> uint64_t {
      struct stat host_stat;
      uint64_t Result = FEX::HLE::_SyscallHandler->FM.Stat(pathname, &host_stat);
      if (Result != -1) {
        CopyStat(buf, &host_stat);
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(fstat, [](FEXCore::Core::InternalThreadState *Thread, int fd, guest_stat *buf) -> uint64_t {
      struct stat host_stat;
      uint64_t Result = ::fstat(fd, &host_stat);
      if (Result != -1) {
        CopyStat(buf, &host_stat);
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(lstat, [](FEXCore::Core::InternalThreadState *Thread, const char *path, guest_stat *buf) -> uint64_t {
      struct stat host_stat;
      uint64_t Result = FEX::HLE::_SyscallHandler->FM.Lstat(path, &host_stat);
      if (Result != -1) {
        CopyStat(buf, &host_stat);
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(readv, [](FEXCore::Core::InternalThreadState *Thread, int fd, const struct iovec *iov, int iovcnt) -> uint64_t {
      uint64_t Result = ::readv(fd, iov, iovcnt);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(writev, [](FEXCore::Core::InternalThreadState *Thread, int fd, const struct iovec *iov, int iovcnt) -> uint64_t {
      uint64_t Result = ::writev(fd, iov, iovcnt);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(newfstatat, [](FEXCore::Core::InternalThreadState *Thread, int dirfd, const char *pathname, guest_stat *buf, int flag) -> uint64_t {
      struct stat host_stat;
      uint64_t Result = FEX::HLE::_SyscallHandler->FM.NewFSStatAt(dirfd, pathname, &host_stat, flag);
      if (Result != -1) {
        CopyStat(buf, &host_stat);
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(vmsplice, [](FEXCore::Core::InternalThreadState *Thread, int fd, const struct iovec *iov, unsigned long nr_segs, unsigned int flags) -> uint64_t {
      uint64_t Result = ::vmsplice(fd, iov, nr_segs, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(preadv, [](FEXCore::Core::InternalThreadState *Thread, int fd, const struct iovec *iov, int iovcnt, off_t offset) -> uint64_t {
      uint64_t Result = ::preadv(fd, iov, iovcnt, offset);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(pwritev, [](FEXCore::Core::InternalThreadState *Thread, int fd, const struct iovec *iov, int iovcnt, off_t offset) -> uint64_t {
      uint64_t Result = ::pwritev(fd, iov, iovcnt, offset);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(preadv2, [](FEXCore::Core::InternalThreadState *Thread, int fd, const struct iovec *iov, int iovcnt, off_t offset, int flags) -> uint64_t {
      uint64_t Result = ::preadv2(fd, iov, iovcnt, offset, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(pwritev2, [](FEXCore::Core::InternalThreadState *Thread, int fd, const struct iovec *iov, int iovcnt, off_t offset, int flags) -> uint64_t {
      uint64_t Result = ::pwritev2(fd, iov, iovcnt, offset, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(process_vm_readv, [](FEXCore::Core::InternalThreadState *Thread, pid_t pid, const struct iovec *local_iov, unsigned long liovcnt, const struct iovec *remote_iov, unsigned long riovcnt, unsigned long flags) -> uint64_t {
      uint64_t Result = ::process_vm_readv(pid, local_iov, liovcnt, remote_iov, riovcnt, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(process_vm_writev, [](FEXCore::Core::InternalThreadState *Thread, pid_t pid, const struct iovec *local_iov, unsigned long liovcnt, const struct iovec *remote_iov, unsigned long riovcnt, unsigned long flags) -> uint64_t {
      uint64_t Result = ::process_vm_writev(pid, local_iov, liovcnt, remote_iov, riovcnt, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(ppoll, [](FEXCore::Core::InternalThreadState *Thread, struct pollfd *fds, nfds_t nfds, const struct timespec *timeout_ts, const sigset_t *sigmask, size_t sigsetsize) -> uint64_t {
      // sigsetsize is unused here since it is currently a constant and not exposed through glibc
      uint64_t Result = ::ppoll(fds, nfds, timeout_ts, sigmask);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(getdents, [](FEXCore::Core::InternalThreadState *Thread, int fd, void *dirp, uint32_t count) -> uint64_t {
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

    REGISTER_SYSCALL_IMPL_X64(getdents64, [](FEXCore::Core::InternalThreadState *Thread, int fd, void *dirp, uint32_t count) -> uint64_t {
      uint64_t Result = syscall(SYS_getdents64,
        static_cast<uint64_t>(fd),
        reinterpret_cast<uint64_t>(dirp),
        static_cast<uint64_t>(count));
      SYSCALL_ERRNO();
    });
  }
}
