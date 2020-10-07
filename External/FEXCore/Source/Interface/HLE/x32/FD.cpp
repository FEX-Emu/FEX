#include "Interface/HLE/Syscalls.h"
#include "Interface/HLE/x32/Syscalls.h"
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

namespace FEXCore::Core {
struct InternalThreadState;
}

namespace FEXCore::HLE::x32 {
  struct iovec_32 {
    uint32_t iov_base;
    uint32_t iov_len;
  };

  struct __attribute__((packed)) guest_stat32 {
    uint32_t st_dev;
    uint32_t st_ino;
    uint32_t st_nlink;

    uint16_t st_mode;
    uint16_t st_uid;
    uint16_t st_gid;
    uint16_t __pad0;

    uint32_t st_rdev;
    uint32_t st_size;
    uint32_t st_blksize;
    uint32_t st_blocks;  /* Number 512-byte blocks allocated. */
    uint32_t st_atime_;
    uint32_t st_atime_nsec;
    uint32_t st_mtime_;
    uint32_t st_mtime_nsec;
    uint32_t st_ctime_;
    uint32_t st_ctime_nsec;
    uint32_t __unused[3];
  };

  struct __attribute__((packed)) guest_stat64_32 {
    uint64_t st_dev;
    uint64_t pad0;
    uint32_t __st_ino;

    uint32_t st_mode;
    uint32_t st_nlink;

    uint32_t st_uid;
    uint32_t st_gid;

    uint64_t st_rdev;
    uint32_t pad3;
    int64_t st_size;
    uint32_t st_blksize;
    uint64_t st_blocks;  /* Number 512-byte blocks allocated. */
    uint32_t st_atime_;
    uint32_t st_atime_nsec;
    uint32_t st_mtime_;
    uint32_t st_mtime_nsec;
    uint32_t st_ctime_;
    uint32_t st_ctime_nsec;
    uint64_t st_ino;
  };

  static void CopyStat32(guest_stat32 *guest, struct stat *host) {
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

  static void CopyStat64_32(guest_stat64_32 *guest, struct stat *host) {
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

    guest->__st_ino = host->st_ino;

    guest->st_atime_ = host->st_atim.tv_sec;
    guest->st_atime_nsec = host->st_atim.tv_nsec;

    guest->st_mtime_ = host->st_mtime;
    guest->st_mtime_nsec = host->st_mtim.tv_nsec;

    guest->st_ctime_ = host->st_ctime;
    guest->st_ctime_nsec = host->st_ctim.tv_nsec;
#undef COPY
  }


  void RegisterFD() {
    REGISTER_SYSCALL_IMPL_X32(readv, [](FEXCore::Core::InternalThreadState *Thread, int fd, const struct iovec_32 *iov, int iovcnt) -> uint64_t {
      std::vector<iovec> Host_iovec(iovcnt);
      for (int i = 0; i < iovcnt; ++i) {
        Host_iovec[i].iov_base = reinterpret_cast<void*>(iov[i].iov_base);
        Host_iovec[i].iov_len = iov[i].iov_len;
      }

      uint64_t Result = ::readv(fd, &Host_iovec.at(0), iovcnt);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(writev, [](FEXCore::Core::InternalThreadState *Thread, int fd, const struct iovec_32 *iov, int iovcnt) -> uint32_t {
      std::vector<iovec> Host_iovec(iovcnt);
      for (int i = 0; i < iovcnt; ++i) {
        Host_iovec[i].iov_base = reinterpret_cast<void*>(iov[i].iov_base);
        Host_iovec[i].iov_len = iov[i].iov_len;
      }
      uint64_t Result = ::writev(fd, &Host_iovec.at(0), iovcnt);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(stat, [](FEXCore::Core::InternalThreadState *Thread, const char *pathname, guest_stat32 *buf) -> uint64_t {
      struct stat host_stat;
      uint64_t Result = Thread->CTX->SyscallHandler->FM.Stat(pathname, &host_stat);
      if (Result != -1) {
        CopyStat32(buf, &host_stat);
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(fstat, [](FEXCore::Core::InternalThreadState *Thread, int fd, guest_stat32 *buf) -> uint64_t {
      struct stat host_stat;
      uint64_t Result = ::fstat(fd, &host_stat);
      if (Result != -1) {
        CopyStat32(buf, &host_stat);
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(lstat, [](FEXCore::Core::InternalThreadState *Thread, const char *path, guest_stat32 *buf) -> uint64_t {
      struct stat host_stat;
      uint64_t Result = Thread->CTX->SyscallHandler->FM.Lstat(path, &host_stat);
      if (Result != -1) {
        CopyStat32(buf, &host_stat);
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(stat64, [](FEXCore::Core::InternalThreadState *Thread, const char *pathname, guest_stat64_32 *buf) -> uint64_t {
      struct stat host_stat;
      uint64_t Result = Thread->CTX->SyscallHandler->FM.Stat(pathname, &host_stat);
      if (Result != -1) {
        CopyStat64_32(buf, &host_stat);
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(lstat64, [](FEXCore::Core::InternalThreadState *Thread, const char *path, guest_stat64_32 *buf) -> uint64_t {
      struct stat host_stat;
      uint64_t Result = Thread->CTX->SyscallHandler->FM.Lstat(path, &host_stat);
      if (Result != -1) {
        CopyStat64_32(buf, &host_stat);
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(fstat64, [](FEXCore::Core::InternalThreadState *Thread, int fd, guest_stat64_32 *buf) -> uint64_t {
      struct stat host_stat;
      uint64_t Result = ::fstat(fd, &host_stat);
      if (Result != -1) {
        CopyStat64_32(buf, &host_stat);
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(preadv, [](FEXCore::Core::InternalThreadState *Thread, int fd, const struct iovec_32 *iov, int iovcnt, off_t offset) -> uint64_t {
      std::vector<iovec> Host_iovec(iovcnt);
      for (int i = 0; i < iovcnt; ++i) {
        Host_iovec[i].iov_base = reinterpret_cast<void*>(iov[i].iov_base);
        Host_iovec[i].iov_len = iov[i].iov_len;
      }

      uint64_t Result = ::preadv(fd, &Host_iovec.at(0), iovcnt, offset);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(pwritev, [](FEXCore::Core::InternalThreadState *Thread, int fd, const struct iovec_32 *iov, int iovcnt, off_t offset) -> uint64_t {
      std::vector<iovec> Host_iovec(iovcnt);
      for (int i = 0; i < iovcnt; ++i) {
        Host_iovec[i].iov_base = reinterpret_cast<void*>(iov[i].iov_base);
        Host_iovec[i].iov_len = iov[i].iov_len;
      }

      uint64_t Result = ::pwritev(fd, &Host_iovec.at(0), iovcnt, offset);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(process_vm_readv, [](FEXCore::Core::InternalThreadState *Thread, pid_t pid, const struct iovec_32 *local_iov, unsigned long liovcnt, const struct iovec_32 *remote_iov, unsigned long riovcnt, unsigned long flags) -> uint64_t {
      std::vector<iovec> Host_local_iovec(liovcnt);
      std::vector<iovec> Host_remote_iovec(riovcnt);

      for (int i = 0; i < liovcnt; ++i) {
        Host_local_iovec[i].iov_base = reinterpret_cast<void*>(local_iov[i].iov_base);
        Host_local_iovec[i].iov_len = local_iov[i].iov_len;
      }

      for (int i = 0; i < riovcnt; ++i) {
        Host_remote_iovec[i].iov_base = reinterpret_cast<void*>(remote_iov[i].iov_base);
        Host_remote_iovec[i].iov_len = remote_iov[i].iov_len;
      }

      uint64_t Result = ::process_vm_readv(pid, &Host_local_iovec.at(0), liovcnt, &Host_remote_iovec.at(0), riovcnt, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(process_vm_writev, [](FEXCore::Core::InternalThreadState *Thread, pid_t pid, const struct iovec_32 *local_iov, unsigned long liovcnt, const struct iovec_32 *remote_iov, unsigned long riovcnt, unsigned long flags) -> uint64_t {
      std::vector<iovec> Host_local_iovec(liovcnt);
      std::vector<iovec> Host_remote_iovec(riovcnt);

      for (int i = 0; i < liovcnt; ++i) {
        Host_local_iovec[i].iov_base = reinterpret_cast<void*>(local_iov[i].iov_base);
        Host_local_iovec[i].iov_len = local_iov[i].iov_len;
      }

      for (int i = 0; i < riovcnt; ++i) {
        Host_remote_iovec[i].iov_base = reinterpret_cast<void*>(remote_iov[i].iov_base);
        Host_remote_iovec[i].iov_len = remote_iov[i].iov_len;
      }

      uint64_t Result = ::process_vm_writev(pid, &Host_local_iovec.at(0), liovcnt, &Host_remote_iovec.at(0), riovcnt, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(preadv2, [](FEXCore::Core::InternalThreadState *Thread, int fd, const struct iovec_32 *iov, int iovcnt, off_t offset, int flags) -> uint64_t {
      std::vector<iovec> Host_iovec(iovcnt);
      for (int i = 0; i < iovcnt; ++i) {
        Host_iovec[i].iov_base = reinterpret_cast<void*>(iov[i].iov_base);
        Host_iovec[i].iov_len = iov[i].iov_len;
      }

      uint64_t Result = ::preadv2(fd, &Host_iovec.at(0), iovcnt, offset, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(pwritev2, [](FEXCore::Core::InternalThreadState *Thread, int fd, const struct iovec_32 *iov, int iovcnt, off_t offset, int flags) -> uint64_t {
      std::vector<iovec> Host_iovec(iovcnt);
      for (int i = 0; i < iovcnt; ++i) {
        Host_iovec[i].iov_base = reinterpret_cast<void*>(iov[i].iov_base);
        Host_iovec[i].iov_len = iov[i].iov_len;
      }

      uint64_t Result = ::pwritev2(fd, &Host_iovec.at(0), iovcnt, offset, flags);
      SYSCALL_ERRNO();
    });

  }
}
