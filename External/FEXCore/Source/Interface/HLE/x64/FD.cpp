#include "Interface/HLE/Syscalls.h"
#include "Interface/HLE/x64/Syscalls.h"
#include "Interface/Context/Context.h"

#include <fcntl.h>
#include <sys/uio.h>

namespace FEXCore::HLE::x64 {
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
    REGISTER_SYSCALL_IMPL_X64(stat, [](FEXCore::Core::InternalThreadState *Thread, const char *pathname, guest_stat *buf) -> uint64_t {
      struct stat host_stat;
      uint64_t Result = Thread->CTX->SyscallHandler->FM.Stat(pathname, &host_stat);
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
      uint64_t Result = Thread->CTX->SyscallHandler->FM.Lstat(path, &host_stat);
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
      uint64_t Result = ::fstatat(dirfd, pathname, &host_stat, flag);
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
  }
}
