/*
$info$
tags: LinuxSyscalls|syscalls-x86-32
$end_info$
*/

#include "Common/MathUtils.h"

#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/x32/IoctlEmulation.h"
#include "Tests/LinuxSyscalls/x32/Syscalls.h"
#include "Tests/LinuxSyscalls/x32/SyscallsEnum.h"
#include "Tests/LinuxSyscalls/x32/Types.h"

#include "Tests/LinuxSyscalls/x64/Syscalls.h"

#include <FEXCore/Debug/InternalThreadState.h>
#include <FEXCore/Utils/LogManager.h>

#include <algorithm>
#include <bits/types/struct_iovec.h>
#include <cstdint>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stddef.h>
#include <string.h>
#include <sys/select.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/time.h>
#include <sys/timerfd.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <syscall.h>
#include <time.h>
#include <type_traits>
#include <unistd.h>
#include <vector>

ARG_TO_STR(FEX::HLE::x32::compat_ptr<FEX::HLE::x32::sigset_argpack32>, "%lx")

namespace FEXCore::Core {
  struct CpuStateFrame;
}

namespace FEX::HLE::x32 {
  // Used to ensure no bogus values are passed into readv/writev family syscalls.
  // This is mainly to sanitize vector sizing. It's fine for the bogus value
  // itself to pass into the syscall, since the kernel will handle it.
  static constexpr int SanitizeIOCount(int count) {
    return std::max(0, count);
  }

  using fd_set32 = uint32_t;
#ifdef _M_X86_64
  uint32_t ioctl_32(FEXCore::Core::CpuStateFrame*, int fd, uint32_t cmd, uint32_t args) {
    uint32_t Result{};
    __asm volatile("int $0x80;"
        : "=a" (Result)
        : "a" (SYSCALL_x86_ioctl)
        , "b" (fd)
        , "c" (cmd)
        , "d" (args)
        : "memory");
    return Result;
  }
#endif

  void RegisterFD() {
    REGISTER_SYSCALL_IMPL_X32_PASS(poll, [](FEXCore::Core::CpuStateFrame *Frame, struct pollfd *fds, nfds_t nfds, int timeout) -> uint64_t {
      uint64_t Result = ::poll(fds, nfds, timeout);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(ppoll, [](FEXCore::Core::CpuStateFrame *Frame, struct pollfd *fds, nfds_t nfds, timespec32 *timeout_ts, const uint64_t *sigmask, size_t sigsetsize) -> uint64_t {
      // sigsetsize is unused here since it is currently a constant and not exposed through glibc
      struct timespec tp64{};
      struct timespec *timed_ptr{};
      if (timeout_ts) {
        tp64 = *timeout_ts;
        timed_ptr = &tp64;
      }

      uint64_t Result = ::syscall(SYSCALL_DEF(ppoll),
        fds,
        nfds,
        timed_ptr,
        sigmask,
        sigsetsize);

      if (timeout_ts) {
        *timeout_ts = tp64;
      }

      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL(ppoll_time64, ppoll, [](FEXCore::Core::CpuStateFrame *Frame, struct pollfd *fds, nfds_t nfds, struct timespec *timeout_ts, const uint64_t *sigmask, size_t sigsetsize) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(ppoll),
        fds,
        nfds,
        timeout_ts,
        sigmask,
        sigsetsize);

      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(_llseek, [](FEXCore::Core::CpuStateFrame *Frame, uint32_t fd, uint32_t offset_high, uint32_t offset_low, loff_t *result, uint32_t whence) -> uint64_t {
      uint64_t Offset = offset_high;
      Offset <<= 32;
      Offset |= offset_low;
      uint64_t Result = lseek(fd, Offset, whence);
      if (Result != -1) {
        *result = Result;
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(readv, [](FEXCore::Core::CpuStateFrame *Frame, int fd, const struct iovec32 *iov, int iovcnt) -> uint64_t {
      std::vector<iovec> Host_iovec(iov, iov + SanitizeIOCount(iovcnt));
      uint64_t Result = ::readv(fd, Host_iovec.data(), iovcnt);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(writev, [](FEXCore::Core::CpuStateFrame *Frame, int fd, const struct iovec32 *iov, int iovcnt) -> uint64_t {
      std::vector<iovec> Host_iovec(iov, iov + SanitizeIOCount(iovcnt));
      uint64_t Result = ::writev(fd, Host_iovec.data(), iovcnt);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL(chown32, chown, [](FEXCore::Core::CpuStateFrame *Frame, const char *pathname, uid_t owner, gid_t group) -> uint64_t {
      uint64_t Result = ::chown(pathname, owner, group);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL(fchown32, fchown, [](FEXCore::Core::CpuStateFrame *Frame, int fd, uid_t owner, gid_t group) -> uint64_t {
      uint64_t Result = ::fchown(fd, owner, group);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL(lchown32, lchown, [](FEXCore::Core::CpuStateFrame *Frame, const char *pathname, uid_t owner, gid_t group) -> uint64_t {
      uint64_t Result = ::lchown(pathname, owner, group);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(stat, [](FEXCore::Core::CpuStateFrame *Frame, const char *pathname, stat32 *buf) -> uint64_t {
      struct stat host_stat;
      uint64_t Result = FEX::HLE::_SyscallHandler->FM.Stat(pathname, &host_stat);
      if (Result != -1) {
        *buf = host_stat;
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(fstat, [](FEXCore::Core::CpuStateFrame *Frame, int fd, stat32 *buf) -> uint64_t {
      struct stat host_stat;
      uint64_t Result = ::fstat(fd, &host_stat);
      if (Result != -1) {
        *buf = host_stat;
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(lstat, [](FEXCore::Core::CpuStateFrame *Frame, const char *path, stat32 *buf) -> uint64_t {
      struct stat host_stat;
      uint64_t Result = FEX::HLE::_SyscallHandler->FM.Lstat(path, &host_stat);
      if (Result != -1) {
        *buf = host_stat;
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(stat64, [](FEXCore::Core::CpuStateFrame *Frame, const char *pathname, stat64_32 *buf) -> uint64_t {
      struct stat host_stat;
      uint64_t Result = FEX::HLE::_SyscallHandler->FM.Stat(pathname, &host_stat);
      if (Result != -1) {
        *buf = host_stat;
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(lstat64, [](FEXCore::Core::CpuStateFrame *Frame, const char *path, stat64_32 *buf) -> uint64_t {
      struct stat host_stat;
      uint64_t Result = FEX::HLE::_SyscallHandler->FM.Lstat(path, &host_stat);

      if (Result != -1) {
        *buf = host_stat;
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(fstat64, [](FEXCore::Core::CpuStateFrame *Frame, int fd, stat64_32 *buf) -> uint64_t {
      struct stat64 host_stat;
      uint64_t Result = ::fstat64(fd, &host_stat);
      if (Result != -1) {
        *buf = host_stat;
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(statfs, [](FEXCore::Core::CpuStateFrame *Frame, const char *path, statfs32_32 *buf) -> uint64_t {
      struct statfs host_stat;
      uint64_t Result = FEX::HLE::_SyscallHandler->FM.Statfs(path, &host_stat);
      if (Result != -1) {
        *buf = host_stat;
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(fstatfs, [](FEXCore::Core::CpuStateFrame *Frame, int fd, statfs32_32 *buf) -> uint64_t {
      struct statfs host_stat;
      uint64_t Result = ::fstatfs(fd, &host_stat);
      if (Result != -1) {
        *buf = host_stat;
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(fstatfs64, [](FEXCore::Core::CpuStateFrame *Frame, int fd, size_t sz, struct statfs64_32 *buf) -> uint64_t {
      LOGMAN_THROW_A(sz == sizeof(struct statfs64_32), "This needs to match");

      struct statfs64 host_stat;
      uint64_t Result = ::fstatfs64(fd, &host_stat);
      if (Result != -1) {
        *buf = host_stat;
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(statfs64, [](FEXCore::Core::CpuStateFrame *Frame, const char *path, size_t sz, struct statfs64_32 *buf) -> uint64_t {
      LOGMAN_THROW_A(sz == sizeof(struct statfs64_32), "This needs to match");

      struct statfs host_stat;
      uint64_t Result = FEX::HLE::_SyscallHandler->FM.Statfs(path, &host_stat);
      if (Result != -1) {
        *buf = host_stat;
      }

      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(fcntl64, [](FEXCore::Core::CpuStateFrame *Frame, int fd, int cmd, uint64_t arg) -> uint64_t {
      // fcntl64 struct directly matches the 64bit fcntl op
      // cmd just needs to be fixed up
      // These are redefined to be their non-64bit tagged value on x86-64
      constexpr int OP_GETLK64_32 = 12;
      constexpr int OP_SETLK64_32 = 13;
      constexpr int OP_SETLKW64_32 = 14;

      void *lock_arg = (void*)arg;
      struct flock tmp{};
      int old_cmd = cmd;

      switch (old_cmd) {
        case OP_GETLK64_32: {
          cmd = F_GETLK;
          lock_arg = (void*)&tmp;
          tmp = *reinterpret_cast<flock64_32*>(arg);
          break;
        }
        case OP_SETLK64_32: {
          cmd = F_SETLK;
          lock_arg = (void*)&tmp;
          tmp = *reinterpret_cast<flock64_32*>(arg);
          break;
        }
        case OP_SETLKW64_32: {
          cmd = F_SETLKW;
          lock_arg = (void*)&tmp;
          tmp = *reinterpret_cast<flock64_32*>(arg);
          break;
        }
        case F_OFD_SETLK:
        case F_OFD_GETLK:
        case F_OFD_SETLKW: {
          lock_arg = (void*)&tmp;
          tmp = *reinterpret_cast<flock64_32*>(arg);
          break;
        }
        case F_GETLK:
        case F_SETLK:
        case F_SETLKW: {
          lock_arg = (void*)&tmp;
          tmp = *reinterpret_cast<flock_32*>(arg);
          break;
        }

        case F_SETFL:
          lock_arg = reinterpret_cast<void*>(FEX::HLE::RemapFromX86Flags(arg));
          break;
        // Maps directly
        case F_DUPFD:
        case F_DUPFD_CLOEXEC:
        case F_GETFD:
        case F_SETFD:
        case F_GETFL:
          break;

        default: LOGMAN_MSG_A("Unhandled fcntl64: 0x%x", cmd); break;
      }

      uint64_t Result = ::fcntl(fd, cmd, lock_arg);

      if (Result != -1) {
        switch (old_cmd) {
          case OP_GETLK64_32: {
            *reinterpret_cast<flock64_32*>(arg) = tmp;
            break;
          }
          case F_OFD_GETLK: {
            *reinterpret_cast<flock64_32*>(arg) = tmp;
            break;
          }
          case F_GETLK: {
            *reinterpret_cast<flock_32*>(arg) = tmp;
            break;
          }
          break;
          case F_DUPFD:
          case F_DUPFD_CLOEXEC:
            FEX::HLE::x32::CheckAndAddFDDuplication(fd, Result);
            break;
          case F_GETFL: {
            Result = FEX::HLE::RemapToX86Flags(Result);
            break;
          }
          default: break;
        }
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(dup, [](FEXCore::Core::CpuStateFrame *Frame, int oldfd) -> uint64_t {
      uint64_t Result = ::dup(oldfd);
      if (Result != -1) {
        CheckAndAddFDDuplication(oldfd, Result);
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(dup2, [](FEXCore::Core::CpuStateFrame *Frame, int oldfd, int newfd) -> uint64_t {
      uint64_t Result = ::dup2(oldfd, newfd);
      if (Result != -1) {
        CheckAndAddFDDuplication(oldfd, newfd);
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(preadv, [](FEXCore::Core::CpuStateFrame *Frame,
      int fd,
      const struct iovec32 *iov,
      uint32_t iovcnt,
      uint32_t pos_low,
      uint32_t pos_high) -> uint64_t {
      std::vector<iovec> Host_iovec(iov, iov + SanitizeIOCount(iovcnt));

      uint64_t Result = ::syscall(SYSCALL_DEF(preadv), fd, Host_iovec.data(), iovcnt, pos_low, pos_high);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(pwritev, [](FEXCore::Core::CpuStateFrame *Frame,
      int fd,
      const struct iovec32 *iov,
      uint32_t iovcnt,
      uint32_t pos_low,
      uint32_t pos_high) -> uint64_t {
      std::vector<iovec> Host_iovec(iov, iov + SanitizeIOCount(iovcnt));

      uint64_t Result = ::syscall(SYSCALL_DEF(pwritev), fd, Host_iovec.data(), iovcnt, pos_low, pos_high);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(process_vm_readv, [](FEXCore::Core::CpuStateFrame *Frame, pid_t pid, const struct iovec32 *local_iov, unsigned long liovcnt, const struct iovec32 *remote_iov, unsigned long riovcnt, unsigned long flags) -> uint64_t {
      std::vector<iovec> Host_local_iovec(local_iov, local_iov + SanitizeIOCount(liovcnt));
      std::vector<iovec> Host_remote_iovec(remote_iov, remote_iov + SanitizeIOCount(riovcnt));

      uint64_t Result = ::process_vm_readv(pid, Host_local_iovec.data(), liovcnt, Host_remote_iovec.data(), riovcnt, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(process_vm_writev, [](FEXCore::Core::CpuStateFrame *Frame, pid_t pid, const struct iovec32 *local_iov, unsigned long liovcnt, const struct iovec32 *remote_iov, unsigned long riovcnt, unsigned long flags) -> uint64_t {
      std::vector<iovec> Host_local_iovec(local_iov, local_iov + SanitizeIOCount(liovcnt));
      std::vector<iovec> Host_remote_iovec(remote_iov, remote_iov + SanitizeIOCount(riovcnt));

      uint64_t Result = ::process_vm_writev(pid, Host_local_iovec.data(), liovcnt, Host_remote_iovec.data(), riovcnt, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(preadv2, [](FEXCore::Core::CpuStateFrame *Frame,
      int fd,
      const struct iovec32 *iov,
      uint32_t iovcnt,
      uint32_t pos_low,
      uint32_t pos_high,
      int flags) -> uint64_t {
      std::vector<iovec> Host_iovec(iov, iov + SanitizeIOCount(iovcnt));

      uint64_t Result = ::syscall(SYSCALL_DEF(preadv2), fd, Host_iovec.data(), iovcnt, pos_low, pos_high, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(pwritev2, [](FEXCore::Core::CpuStateFrame *Frame,
      int fd,
      const struct iovec32 *iov,
      uint32_t iovcnt,
      uint32_t pos_low,
      uint32_t pos_high,
      int flags) -> uint64_t {
      std::vector<iovec> Host_iovec(iov, iov + SanitizeIOCount(iovcnt));

      uint64_t Result = ::syscall(SYSCALL_DEF(pwritev2), fd, Host_iovec.data(),iovcnt, pos_low, pos_high, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(fstatat64, [](FEXCore::Core::CpuStateFrame *Frame, int dirfd, const char *pathname, stat64_32 *buf, int flag) -> uint64_t {
      struct stat64 host_stat;
      uint64_t Result = FEX::HLE::_SyscallHandler->FM.NewFSStatAt64(dirfd, pathname, &host_stat, flag);
      if (Result != -1) {
        *buf = host_stat;
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(ioctl, ioctl32);

    REGISTER_SYSCALL_IMPL_X32(getdents, [](FEXCore::Core::CpuStateFrame *Frame, int fd, void *dirp, uint32_t count) -> uint64_t {
#ifdef SYS_getdents
      std::vector<uint8_t> TmpVector(count);
      void *TmpPtr = reinterpret_cast<void*>(&TmpVector.at(0));

      // Copy the incoming structures to our temporary array
      for (uint64_t Offset = 0, TmpOffset = 0;
          Offset < count;) {
        linux_dirent_32 *Incoming = (linux_dirent_32*)(reinterpret_cast<uint64_t>(dirp) + Offset);
        linux_dirent *Tmp = (linux_dirent*)(reinterpret_cast<uint64_t>(TmpPtr) + TmpOffset);

        if (!Incoming->d_reclen ||
            (Offset + Incoming->d_reclen) > count) {
          break;
        }

        size_t NewRecLen = Incoming->d_reclen + (sizeof(linux_dirent) - sizeof(linux_dirent_32));
        Tmp->d_ino    = Incoming->d_ino;
        Tmp->d_off    = Incoming->d_off;
        Tmp->d_reclen = NewRecLen;

        // This actually copies two more bytes than the string of d_name
        // Copies a null byte for the string
        // Copies a d_type flag that lives after the name
        size_t CopySize = std::clamp<uint32_t>(Incoming->d_reclen - offsetof(linux_dirent_32, d_name), 0U, count - Offset);
        memcpy(Tmp->d_name, Incoming->d_name, CopySize);

        // We take up 8 more bytes of space
        TmpOffset += NewRecLen;
        Offset += Incoming->d_reclen;
      }

      uint64_t Result = syscall(SYSCALL_DEF(getdents),
        static_cast<uint64_t>(fd),
        TmpPtr,
        static_cast<uint64_t>(count));

      // Now copy back in to the array we were given
      if (Result != -1) {
        uint64_t Offset = 0;
        // With how the emulation occurs we will always return a smaller buffer than what was given to us
        for (uint64_t TmpOffset = 0, num = 0; TmpOffset < Result; ++num) {
          linux_dirent_32 *Outgoing = (linux_dirent_32*)(reinterpret_cast<uint64_t>(dirp) + Offset);
          linux_dirent *Tmp = (linux_dirent*)(reinterpret_cast<uint64_t>(TmpPtr) + TmpOffset);

          if (!Tmp->d_reclen) {
            break;
          }

          size_t NewRecLen = Tmp->d_reclen - (sizeof(std::remove_reference<decltype(*Tmp)>::type) - sizeof(*Outgoing));
          Outgoing->d_ino = Tmp->d_ino;
          // If we pass d_off directly then we seem to encounter issues?
          Outgoing->d_off = num; //Tmp->d_off;
          size_t OffsetOfName = offsetof(std::remove_reference<decltype(*Tmp)>::type, d_name);
          Outgoing->d_reclen = NewRecLen;

          // Copies null character and d_type flag as well
          memcpy(Outgoing->d_name, Tmp->d_name, Tmp->d_reclen - OffsetOfName);

          TmpOffset += Tmp->d_reclen;
          // Outgoing is 8 bytes smaller
          Offset += NewRecLen;
        }
        Result = Offset;
      }
      SYSCALL_ERRNO();
#else
      // XXX: Emulate
      return -ENOSYS;
#endif
    });

    REGISTER_SYSCALL_IMPL_X32(getdents64, [](FEXCore::Core::CpuStateFrame *Frame, int fd, void *dirp, uint32_t count) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(getdents64),
        static_cast<uint64_t>(fd),
        dirp,
        static_cast<uint64_t>(count));
      if (Result != -1) {
        // Walk each offset
        // if we are passing the full d_off to the 32bit application then it seems to break things?
        for (size_t i = 0, num = 0; i < Result; ++num) {
          linux_dirent_64 *Incoming = (linux_dirent_64*)(reinterpret_cast<uint64_t>(dirp) + i);
          Incoming->d_off = num;
          i += Incoming->d_reclen;
        }
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(_newselect, [](FEXCore::Core::CpuStateFrame *Frame, int nfds, fd_set32 *readfds, fd_set32 *writefds, fd_set32 *exceptfds, struct timeval32 *timeout) -> uint64_t {
      struct timeval tp64{};
      if (timeout) {
        tp64 = *timeout;
      }

      fd_set Host_readfds;
      fd_set Host_writefds;
      fd_set Host_exceptfds;
      FD_ZERO(&Host_readfds);
      FD_ZERO(&Host_writefds);
      FD_ZERO(&Host_exceptfds);

      // Round up to the full 32bit word
      uint32_t NumWords = AlignUp(nfds, 32) / 4;

      if (readfds) {
        for (int i = 0; i < NumWords; ++i) {
          uint32_t FD = readfds[i];
          int32_t Rem = nfds - (i * 32);
          for (int j = 0; j < 32 && j < Rem; ++j) {
            if ((FD >> j) & 1) {
              FD_SET(i * 32 + j, &Host_readfds);
            }
          }
        }
      }

      if (writefds) {
        for (int i = 0; i < NumWords; ++i) {
          uint32_t FD = writefds[i];
          int32_t Rem = nfds - (i * 32);
          for (int j = 0; j < 32 && j < Rem; ++j) {
            if ((FD >> j) & 1) {
              FD_SET(i * 32 + j, &Host_writefds);
            }
          }
        }
      }

      if (exceptfds) {
        for (int i = 0; i < NumWords; ++i) {
          uint32_t FD = exceptfds[i];
          int32_t Rem = nfds - (i * 32);
          for (int j = 0; j < 32 && j < Rem; ++j) {
            if ((FD >> j) & 1) {
              FD_SET(i * 32 + j, &Host_exceptfds);
            }
          }
        }
      }

      uint64_t Result = ::select(nfds,
        readfds ? &Host_readfds : nullptr,
        writefds ? &Host_writefds : nullptr,
        exceptfds ? &Host_exceptfds : nullptr,
        timeout ? &tp64 : nullptr);
      if (readfds) {
        for (int i = 0; i < nfds; ++i) {
          if (FD_ISSET(i, &Host_readfds)) {
            readfds[i/32] |= 1 << (i & 31);
          } else {
            readfds[i/32] &= ~(1 << (i & 31));
          }
        }
      }

      if (writefds) {
        for (int i = 0; i < nfds; ++i) {
          if (FD_ISSET(i, &Host_writefds)) {
            writefds[i/32] |= 1 << (i & 31);
          } else {
            writefds[i/32] &= ~(1 << (i & 31));
          }
        }
      }

      if (exceptfds) {
        for (int i = 0; i < nfds; ++i) {
          if (FD_ISSET(i, &Host_exceptfds)) {
            exceptfds[i/32] |= 1 << (i & 31);
          } else {
            exceptfds[i/32] &= ~(1 << (i & 31));
          }
        }
      }

      if (timeout) {
        *timeout = tp64;
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(pselect6, [](FEXCore::Core::CpuStateFrame *Frame, int nfds, fd_set32 *readfds, fd_set32 *writefds, fd_set32 *exceptfds, timespec32 *timeout, compat_ptr<sigset_argpack32> sigmaskpack) -> uint64_t {
      struct timespec tp64{};
      if (timeout) {
        tp64 = *timeout;
      }

      fd_set Host_readfds;
      fd_set Host_writefds;
      fd_set Host_exceptfds;
      sigset_t HostSet{};

      FD_ZERO(&Host_readfds);
      FD_ZERO(&Host_writefds);
      FD_ZERO(&Host_exceptfds);
      sigemptyset(&HostSet);

      // Round up to the full 32bit word
      uint32_t NumWords = AlignUp(nfds, 32) / 4;

      if (readfds) {
        for (int i = 0; i < NumWords; ++i) {
          uint32_t FD = readfds[i];
          int32_t Rem = nfds - (i * 32);
          for (int j = 0; j < 32 && j < Rem; ++j) {
            if ((FD >> j) & 1) {
              FD_SET(i * 32 + j, &Host_readfds);
            }
          }
        }
      }

      if (writefds) {
        for (int i = 0; i < NumWords; ++i) {
          uint32_t FD = writefds[i];
          int32_t Rem = nfds - (i * 32);
          for (int j = 0; j < 32 && j < Rem; ++j) {
            if ((FD >> j) & 1) {
              FD_SET(i * 32 + j, &Host_writefds);
            }
          }
        }
      }

      if (exceptfds) {
        for (int i = 0; i < NumWords; ++i) {
          uint32_t FD = exceptfds[i];
          int32_t Rem = nfds - (i * 32);
          for (int j = 0; j < 32 && j < Rem; ++j) {
            if ((FD >> j) & 1) {
              FD_SET(i * 32 + j, &Host_exceptfds);
            }
          }
        }
      }

      if (sigmaskpack && sigmaskpack->sigset) {
        uint64_t *sigmask = sigmaskpack->sigset;
        size_t sigsetsize = sigmaskpack->size;
        for (int32_t i = 0; i < (sigsetsize * 8); ++i) {
          if (*sigmask & (1ULL << i)) {
            sigaddset(&HostSet, i + 1);
          }
        }
      }

      uint64_t Result = ::pselect(nfds,
        readfds ? &Host_readfds : nullptr,
        writefds ? &Host_writefds : nullptr,
        exceptfds ? &Host_exceptfds : nullptr,
        timeout ? &tp64 : nullptr,
        &HostSet);

      if (readfds) {
        for (int i = 0; i < nfds; ++i) {
          if (FD_ISSET(i, &Host_readfds)) {
            readfds[i/32] |= 1 << (i & 31);
          } else {
            readfds[i/32] &= ~(1 << (i & 31));
          }
        }
      }

      if (writefds) {
        for (int i = 0; i < nfds; ++i) {
          if (FD_ISSET(i, &Host_writefds)) {
            writefds[i/32] |= 1 << (i & 31);
          } else {
            writefds[i/32] &= ~(1 << (i & 31));
          }
        }
      }

      if (exceptfds) {
        for (int i = 0; i < nfds; ++i) {
          if (FD_ISSET(i, &Host_exceptfds)) {
            exceptfds[i/32] |= 1 << (i & 31);
          } else {
            exceptfds[i/32] &= ~(1 << (i & 31));
          }
        }
      }

      if (timeout) {
        *timeout = tp64;
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(fadvise64_64, [](FEXCore::Core::CpuStateFrame *Frame, int32_t fd, uint32_t offset_low, uint32_t offset_high, uint32_t len_low, uint32_t len_high, int advice) -> uint64_t {
      uint64_t Offset = offset_high;
      Offset <<= 32;
      Offset |= offset_low;
      uint64_t Len = len_high;
      Len <<= 32;
      Len |= len_low;
      uint64_t Result = ::posix_fadvise64(fd, Offset, Len, advice);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL(timerfd_settime64, timerfd_settime, [](FEXCore::Core::CpuStateFrame *Frame, int fd, int flags, const struct itimerspec *new_value, struct itimerspec *old_value) -> uint64_t {
      uint64_t Result = ::timerfd_settime(fd, flags, new_value, old_value);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL(timerfd_gettime64, timerfd_gettime, [](FEXCore::Core::CpuStateFrame *Frame, int fd, struct itimerspec *curr_value) -> uint64_t {
      uint64_t Result = ::timerfd_gettime(fd, curr_value);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(timerfd_settime, [](FEXCore::Core::CpuStateFrame *Frame,
      int fd,
      int flags,
      const FEX::HLE::x32::old_itimerspec32 *new_value,
      FEX::HLE::x32::old_itimerspec32 *old_value) -> uint64_t {
      struct itimerspec new_value_host{};
      struct itimerspec old_value_host{};
      struct itimerspec *old_value_host_p{};

      new_value_host = *new_value;
      if (old_value) {
        old_value_host_p = &old_value_host;
      }

      // Flags don't need remapped
      uint64_t Result = ::timerfd_settime(fd, flags, &new_value_host, old_value_host_p);

      if (Result != -1 && old_value) {
        *old_value = old_value_host;
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(timerfd_gettime, [](FEXCore::Core::CpuStateFrame *Frame, int fd, FEX::HLE::x32::old_itimerspec32 *curr_value) -> uint64_t {
      struct itimerspec Host{};

      uint64_t Result = ::timerfd_gettime(fd, &Host);

      if (Result != -1) {
        *curr_value = Host;
      }

      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(pselect6_time64, [](FEXCore::Core::CpuStateFrame *Frame, int nfds, fd_set32 *readfds, fd_set32 *writefds, fd_set32 *exceptfds, struct timespec *timeout, compat_ptr<sigset_argpack32> sigmaskpack) -> uint64_t {
      fd_set Host_readfds;
      fd_set Host_writefds;
      fd_set Host_exceptfds;
      sigset_t HostSet{};

      FD_ZERO(&Host_readfds);
      FD_ZERO(&Host_writefds);
      FD_ZERO(&Host_exceptfds);
      sigemptyset(&HostSet);

      // Round up to the full 32bit word
      uint32_t NumWords = AlignUp(nfds, 32) / 4;

      if (readfds) {
        for (int i = 0; i < NumWords; ++i) {
          uint32_t FD = readfds[i];
          int32_t Rem = nfds - (i * 32);
          for (int j = 0; j < 32 && j < Rem; ++j) {
            if ((FD >> j) & 1) {
              FD_SET(i * 32 + j, &Host_readfds);
            }
          }
        }
      }

      if (writefds) {
        for (int i = 0; i < NumWords; ++i) {
          uint32_t FD = writefds[i];
          int32_t Rem = nfds - (i * 32);
          for (int j = 0; j < 32 && j < Rem; ++j) {
            if ((FD >> j) & 1) {
              FD_SET(i * 32 + j, &Host_writefds);
            }
          }
        }
      }

      if (exceptfds) {
        for (int i = 0; i < NumWords; ++i) {
          uint32_t FD = exceptfds[i];
          int32_t Rem = nfds - (i * 32);
          for (int j = 0; j < 32 && j < Rem; ++j) {
            if ((FD >> j) & 1) {
              FD_SET(i * 32 + j, &Host_exceptfds);
            }
          }
        }
      }

      if (sigmaskpack && sigmaskpack->sigset) {
        uint64_t *sigmask = sigmaskpack->sigset;
        size_t sigsetsize = sigmaskpack->size;
        for (int32_t i = 0; i < (sigsetsize * 8); ++i) {
          if (*sigmask & (1ULL << i)) {
            sigaddset(&HostSet, i + 1);
          }
        }
      }

      uint64_t Result = ::pselect(nfds,
        readfds ? &Host_readfds : nullptr,
        writefds ? &Host_writefds : nullptr,
        exceptfds ? &Host_exceptfds : nullptr,
        timeout,
        &HostSet);

      if (readfds) {
        for (int i = 0; i < nfds; ++i) {
          if (FD_ISSET(i, &Host_readfds)) {
            readfds[i/32] |= 1 << (i & 31);
          } else {
            readfds[i/32] &= ~(1 << (i & 31));
          }
        }
      }

      if (writefds) {
        for (int i = 0; i < nfds; ++i) {
          if (FD_ISSET(i, &Host_writefds)) {
            writefds[i/32] |= 1 << (i & 31);
          } else {
            writefds[i/32] &= ~(1 << (i & 31));
          }
        }
      }

      if (exceptfds) {
        for (int i = 0; i < nfds; ++i) {
          if (FD_ISSET(i, &Host_exceptfds)) {
            exceptfds[i/32] |= 1 << (i & 31);
          } else {
            exceptfds[i/32] &= ~(1 << (i & 31));
          }
        }
      }

      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(sendfile, [](FEXCore::Core::CpuStateFrame *Frame, int out_fd, int in_fd, compat_off_t *offset, size_t count) -> uint64_t {
      off_t Local{};
      off_t *Local_p{};
      if (offset) {
        Local_p = &Local;
        Local = *offset;
      }
      uint64_t Result = ::sendfile(out_fd, in_fd, Local_p, count);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL(sendfile64, sendfile, [](FEXCore::Core::CpuStateFrame *Frame, int out_fd, int in_fd, off_t *offset, compat_size_t count) -> uint64_t {
      // Linux definition for this is a bit confusing
      // Defines offset as compat_loff_t* but loads loff_t worth of data
      // count is defined as compat_size_t still
      uint64_t Result = ::sendfile(out_fd, in_fd, offset, count);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(pread64, [](FEXCore::Core::CpuStateFrame *Frame, int fd, void *buf, uint32_t count, uint32_t offset_low, uint32_t offset_high) -> uint64_t {
      uint64_t Offset = offset_high;
      Offset <<= 32;
      Offset |= offset_low;

      uint64_t Result = ::pread64(fd, buf, count, Offset);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(pwrite64, [](FEXCore::Core::CpuStateFrame *Frame, int fd, void *buf, uint32_t count, uint32_t offset_low, uint32_t offset_high) -> uint64_t {
      uint64_t Offset = offset_high;
      Offset <<= 32;
      Offset |= offset_low;

      uint64_t Result = ::pwrite64(fd, buf, count, Offset);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(readahead, [](FEXCore::Core::CpuStateFrame *Frame, int fd, uint32_t offset_low, uint64_t offset_high, size_t count) -> uint64_t {
      uint64_t Offset = offset_high;
      Offset <<= 32;
      Offset |= offset_low;

      uint64_t Result = ::readahead(fd, Offset, count);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(sync_file_range, [](FEXCore::Core::CpuStateFrame *Frame,
      int fd,
      uint32_t offset_low,
      uint32_t offset_high,
      uint32_t len_low,
      uint32_t len_high,
      unsigned int flags) -> uint64_t {
      // Flags don't need remapped
      uint64_t Offset = offset_high;
      Offset <<= 32;
      Offset |= offset_low;

      uint64_t Len = len_high;
      Len <<= 32;
      Len |= len_low;

      uint64_t Result = ::sync_file_range(fd, Offset, Len, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(fallocate, [](FEXCore::Core::CpuStateFrame *Frame,
      int fd,
      int mode,
      uint32_t offset_low,
      uint32_t offset_high,
      uint32_t len_low,
      uint32_t len_high) -> uint64_t {
      uint64_t Offset = offset_high;
      Offset <<= 32;
      Offset |= offset_low;

      uint64_t Len = len_high;
      Len <<= 32;
      Len |= len_low;

      uint64_t Result = ::fallocate(fd, mode, Offset, Len);
      SYSCALL_ERRNO();
    });
  }

}
