/*
$info$
tags: LinuxSyscalls|syscalls-x86-64
$end_info$
*/

#include "Tests/LinuxSyscalls/FileManagement.h"
#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/x64/Syscalls.h"
#include "Tests/LinuxSyscalls/x64/Types.h"

#include <FEXCore/Utils/CompilerDefs.h>

#include <fcntl.h>
#include <poll.h>
#include <stdint.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/sendfile.h>
#include <sys/timerfd.h>
#include <syscall.h>
#include <time.h>
#include <unistd.h>

namespace FEX::HLE::x64 {
  void RegisterFD() {
    REGISTER_SYSCALL_IMPL_X64_PASS(poll, [](FEXCore::Core::CpuStateFrame *Frame, struct pollfd *fds, nfds_t nfds, int timeout) -> uint64_t {
      uint64_t Result = ::poll(fds, nfds, timeout);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64_PASS(select, [](FEXCore::Core::CpuStateFrame *Frame, int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout) -> uint64_t {
      uint64_t Result = ::select(nfds, readfds, writefds, exceptfds, timeout);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(fcntl, [](FEXCore::Core::CpuStateFrame *Frame, int fd, int cmd, uint64_t arg) -> uint64_t {
      uint64_t Result{};
      switch (cmd) {
      case F_GETFL:
        Result = ::fcntl(fd, cmd, arg);
        if (Result != -1) {
          Result = FEX::HLE::RemapToX86Flags(Result);
        }
        break;
      case F_SETFL:
        Result = ::fcntl(fd, cmd, FEX::HLE::RemapFromX86Flags(arg));
        break;
      default:
        Result = ::fcntl(fd, cmd, arg);
        break;
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64_PASS(futimesat, [](FEXCore::Core::CpuStateFrame *Frame, int dirfd, const char *pathname, const struct timeval times[2]) -> uint64_t {
      uint64_t Result = ::futimesat(dirfd, pathname, times);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64_PASS(utimensat, [](FEXCore::Core::CpuStateFrame *Frame, int dirfd, const char *pathname, const struct timespec times[2], int flags) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(utimensat), dirfd, pathname, times, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64_PASS(pselect6, [](FEXCore::Core::CpuStateFrame *Frame, int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, const struct timespec *timeout, const void *sigmaskpack) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(pselect6), nfds, readfds, writefds, exceptfds, timeout, sigmaskpack);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(stat, [](FEXCore::Core::CpuStateFrame *Frame, const char *pathname, FEX::HLE::x64::guest_stat *buf) -> uint64_t {
      struct stat host_stat;
      uint64_t Result = FEX::HLE::_SyscallHandler->FM.Stat(pathname, &host_stat);
      if (Result != -1) {
        *buf = host_stat;
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(fstat, [](FEXCore::Core::CpuStateFrame *Frame, int fd, FEX::HLE::x64::guest_stat *buf) -> uint64_t {
      struct stat host_stat;
      uint64_t Result = ::fstat(fd, &host_stat);
      if (Result != -1) {
        *buf = host_stat;
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(lstat, [](FEXCore::Core::CpuStateFrame *Frame, const char *path, FEX::HLE::x64::guest_stat *buf) -> uint64_t {
      struct stat host_stat;
      uint64_t Result = FEX::HLE::_SyscallHandler->FM.Lstat(path, &host_stat);
      if (Result != -1) {
        *buf = host_stat;
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64_PASS(readv, [](FEXCore::Core::CpuStateFrame *Frame, int fd, const struct iovec *iov, int iovcnt) -> uint64_t {
      uint64_t Result = ::readv(fd, iov, iovcnt);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64_PASS(writev, [](FEXCore::Core::CpuStateFrame *Frame, int fd, const struct iovec *iov, int iovcnt) -> uint64_t {
      uint64_t Result = ::writev(fd, iov, iovcnt);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64_PASS(readahead, [](FEXCore::Core::CpuStateFrame *Frame, int fd, off64_t offset, size_t count) -> uint64_t {
      uint64_t Result = ::readahead(fd, offset, count);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(newfstatat, [](FEXCore::Core::CpuStateFrame *Frame, int dirfd, const char *pathname, FEX::HLE::x64::guest_stat *buf, int flag) -> uint64_t {
      struct stat host_stat;
      uint64_t Result = FEX::HLE::_SyscallHandler->FM.NewFSStatAt(dirfd, pathname, &host_stat, flag);
      if (Result != -1) {
        *buf = host_stat;
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64_PASS(vmsplice, [](FEXCore::Core::CpuStateFrame *Frame, int fd, const struct iovec *iov, unsigned long nr_segs, unsigned int flags) -> uint64_t {
      uint64_t Result = ::vmsplice(fd, iov, nr_segs, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64_PASS(preadv, [](FEXCore::Core::CpuStateFrame *Frame,
      int fd,
      const struct iovec *iov,
      uint64_t vlen,
      uint64_t pos_l,
      uint64_t pos_h) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(preadv), fd, iov, vlen, pos_l, pos_h);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64_PASS(pwritev, [](FEXCore::Core::CpuStateFrame *Frame,
      int fd,
      const struct iovec *iov,
      uint64_t vlen,
      uint64_t pos_l,
      uint64_t pos_h) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(pwritev), fd, iov, vlen, pos_l, pos_h);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64_PASS(preadv2, [](FEXCore::Core::CpuStateFrame *Frame,
      int fd,
      const struct iovec *iov,
      uint64_t vlen,
      uint64_t pos_l,
      uint64_t pos_h,
      int flags) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(preadv2), fd, iov, vlen, pos_l, pos_h, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64_PASS(pwritev2, [](FEXCore::Core::CpuStateFrame *Frame,
      int fd,
      const struct iovec *iov,
      uint64_t vlen,
      uint64_t pos_l,
      uint64_t pos_h,
      int flags) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(pwritev2), fd, iov, vlen, pos_l, pos_h, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64_PASS(pread64, [](FEXCore::Core::CpuStateFrame *Frame, int fd, void *buf, size_t count, off_t offset) -> uint64_t {
      uint64_t Result = ::pread64(fd, buf, count, offset);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64_PASS(pwrite64, [](FEXCore::Core::CpuStateFrame *Frame, int fd, void *buf, size_t count, off_t offset) -> uint64_t {
      uint64_t Result = ::pwrite64(fd, buf, count, offset);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64_PASS(process_vm_readv, [](FEXCore::Core::CpuStateFrame *Frame, pid_t pid, const struct iovec *local_iov, unsigned long liovcnt, const struct iovec *remote_iov, unsigned long riovcnt, unsigned long flags) -> uint64_t {
      uint64_t Result = ::process_vm_readv(pid, local_iov, liovcnt, remote_iov, riovcnt, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64_PASS(process_vm_writev, [](FEXCore::Core::CpuStateFrame *Frame, pid_t pid, const struct iovec *local_iov, unsigned long liovcnt, const struct iovec *remote_iov, unsigned long riovcnt, unsigned long flags) -> uint64_t {
      uint64_t Result = ::process_vm_writev(pid, local_iov, liovcnt, remote_iov, riovcnt, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64_PASS(ppoll, [](FEXCore::Core::CpuStateFrame *Frame, struct pollfd *fds, nfds_t nfds, struct timespec *timeout_ts, const uint64_t *sigmask, size_t sigsetsize) -> uint64_t {
      // glibc wrapper doesn't allow timeout_ts to be modified like the kernel does
      int Result = ::syscall(SYSCALL_DEF(ppoll), fds, nfds, timeout_ts, sigmask, sigsetsize);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(getdents, [](FEXCore::Core::CpuStateFrame *Frame, int fd, void *dirp, uint32_t count) -> uint64_t {
  #ifdef SYS_getdents
      uint64_t Result = syscall(SYSCALL_DEF(getdents),
        static_cast<uint64_t>(fd),
        reinterpret_cast<uint64_t>(dirp),
        static_cast<uint64_t>(count));
      SYSCALL_ERRNO();
  #else
      // XXX: Emulate
      return -EFAULT;
  #endif
    });

    REGISTER_SYSCALL_IMPL_X64_PASS(getdents64, [](FEXCore::Core::CpuStateFrame *Frame, int fd, void *dirp, uint32_t count) -> uint64_t {
      uint64_t Result = syscall(SYSCALL_DEF(getdents64),
        static_cast<uint64_t>(fd),
        reinterpret_cast<uint64_t>(dirp),
        static_cast<uint64_t>(count));
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64_PASS(sendfile, [](FEXCore::Core::CpuStateFrame *Frame, int out_fd, int in_fd, off_t *offset, size_t count) -> uint64_t {
      uint64_t Result = ::sendfile(out_fd, in_fd, offset, count);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64_PASS(dup, [](FEXCore::Core::CpuStateFrame *Frame, int oldfd) -> uint64_t {
      uint64_t Result = ::dup(oldfd);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64_PASS(dup2, [](FEXCore::Core::CpuStateFrame *Frame, int oldfd, int newfd) -> uint64_t {
      uint64_t Result = ::dup2(oldfd, newfd);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(statfs, [](FEXCore::Core::CpuStateFrame *Frame, const char *path, struct statfs *buf) -> uint64_t {
      uint64_t Result = FEX::HLE::_SyscallHandler->FM.Statfs(path, buf);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64_PASS(fstatfs, [](FEXCore::Core::CpuStateFrame *Frame, int fd, struct statfs *buf) -> uint64_t {
      uint64_t Result = ::fstatfs(fd, buf);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64_PASS(sync_file_range, [](FEXCore::Core::CpuStateFrame *Frame, int fd, off64_t offset, off64_t nbytes, unsigned int flags) -> uint64_t {
      // Flags don't need remapped
      uint64_t Result = ::sync_file_range(fd, offset, nbytes, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64_PASS(fallocate, [](FEXCore::Core::CpuStateFrame *Frame, int fd, int mode, off_t offset, off_t len) -> uint64_t {
      uint64_t Result = ::fallocate(fd, mode, offset, len);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64_PASS(timerfd_settime, [](FEXCore::Core::CpuStateFrame *Frame, int fd, int flags, const struct itimerspec *new_value, struct itimerspec *old_value) -> uint64_t {
      // Flags don't need remapped
      uint64_t Result = ::timerfd_settime(fd, flags, new_value, old_value);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64_PASS(timerfd_gettime, [](FEXCore::Core::CpuStateFrame *Frame, int fd, struct itimerspec *curr_value) -> uint64_t {
      uint64_t Result = ::timerfd_gettime(fd, curr_value);
      SYSCALL_ERRNO();
    });
  }
}
