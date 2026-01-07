// SPDX-License-Identifier: MIT
/*
$info$
tags: LinuxSyscalls|syscalls-x86-32
$end_info$
*/

#include "LinuxSyscalls/Syscalls.h"
#include "LinuxSyscalls/x32/IoctlEmulation.h"
#include "LinuxSyscalls/x32/Syscalls.h"
#include "LinuxSyscalls/x32/SyscallsEnum.h"
#include "LinuxSyscalls/x32/Types.h"

#include "LinuxSyscalls/x64/Syscalls.h"

#include <FEXCore/Debug/InternalThreadState.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/MathUtils.h>
#include <FEXCore/fextl/vector.h>

#include <algorithm>
#include <cstdint>
#include <fcntl.h>
#include <limits>
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

#ifdef ARCHITECTURE_x86_64
uint32_t ioctl_32(FEXCore::Core::CpuStateFrame*, int fd, uint32_t cmd, uint32_t args) {
  uint32_t Result {};
  __asm volatile("int $0x80;" : "=a"(Result) : "a"(SYSCALL_x86_ioctl), "b"(fd), "c"(cmd), "d"(args) : "memory");
  return Result;
}
#endif
// These are redefined to be their non-64bit tagged value on x86-64
constexpr int OP_GETLK64_32 = 12;
constexpr int OP_SETLK64_32 = 13;
constexpr int OP_SETLKW64_32 = 14;

auto fcntlHandler = [](FEXCore::Core::CpuStateFrame* Frame, int fd, int cmd, uint64_t arg) -> uint64_t {
  // fcntl64 struct directly matches the 64bit fcntl op
  // cmd just needs to be fixed up

  void* lock_arg = (void*)arg;
  struct flock tmp {};
  int old_cmd = cmd;

  switch (old_cmd) {
  case OP_GETLK64_32: {
    cmd = F_GETLK;
    lock_arg = (void*)&tmp;
    FaultSafeUserMemAccess::VerifyIsReadable(reinterpret_cast<void*>(arg), sizeof(flock64_32));
    tmp = *reinterpret_cast<flock64_32*>(arg);
    break;
  }
  case OP_SETLK64_32: {
    cmd = F_SETLK;
    lock_arg = (void*)&tmp;
    FaultSafeUserMemAccess::VerifyIsReadable(reinterpret_cast<void*>(arg), sizeof(flock64_32));
    tmp = *reinterpret_cast<flock64_32*>(arg);
    break;
  }
  case OP_SETLKW64_32: {
    cmd = F_SETLKW;
    lock_arg = (void*)&tmp;
    FaultSafeUserMemAccess::VerifyIsReadable(reinterpret_cast<void*>(arg), sizeof(flock64_32));
    tmp = *reinterpret_cast<flock64_32*>(arg);
    break;
  }
  case F_OFD_SETLK:
  case F_OFD_GETLK:
  case F_OFD_SETLKW: {
    lock_arg = (void*)&tmp;
    FaultSafeUserMemAccess::VerifyIsReadable(reinterpret_cast<void*>(arg), sizeof(flock64_32));
    tmp = *reinterpret_cast<flock64_32*>(arg);
    break;
  }
  case F_GETLK:
  case F_SETLK:
  case F_SETLKW: {
    lock_arg = (void*)&tmp;
    FaultSafeUserMemAccess::VerifyIsReadable(reinterpret_cast<void*>(arg), sizeof(flock_32));
    tmp = *reinterpret_cast<flock_32*>(arg);
    break;
  }

  case F_SETFL: lock_arg = reinterpret_cast<void*>(FEX::HLE::RemapFromX86Flags(arg)); break;
  // Everything else maps directly. Check `COMPAT_SYSCALL_DEFINE3(fcntl64, ...)` entrypoint in the kernel if this changes.
  default: break;
  }

  uint64_t Result = ::fcntl(fd, cmd, lock_arg);

  if (Result != -1) {
    switch (old_cmd) {
    case OP_GETLK64_32: {
      FaultSafeUserMemAccess::VerifyIsWritable(reinterpret_cast<void*>(arg), sizeof(flock64_32));
      *reinterpret_cast<flock64_32*>(arg) = tmp;
      break;
    }
    case F_OFD_GETLK: {
      FaultSafeUserMemAccess::VerifyIsWritable(reinterpret_cast<void*>(arg), sizeof(flock64_32));
      *reinterpret_cast<flock64_32*>(arg) = tmp;
      break;
    }
    case F_GETLK: {
      FaultSafeUserMemAccess::VerifyIsWritable(reinterpret_cast<void*>(arg), sizeof(flock_32));
      *reinterpret_cast<flock_32*>(arg) = tmp;
      break;
    } break;
    case F_DUPFD:
    case F_DUPFD_CLOEXEC: FEX::HLE::x32::CheckAndAddFDDuplication(fd, Result); break;
    case F_GETFL: {
      Result = FEX::HLE::RemapToX86Flags(Result);
      break;
    }
    default: break;
    }
  }
  SYSCALL_ERRNO();
};

auto fcntl32Handler = [](FEXCore::Core::CpuStateFrame* Frame, int fd, int cmd, uint64_t arg) -> uint64_t {
  // fcntl32 handler explicitly blocks these commands.
  switch (cmd) {
  case OP_GETLK64_32:
  case OP_SETLK64_32:
  case OP_SETLKW64_32:
  case F_OFD_GETLK:
  case F_OFD_SETLK:
  case F_OFD_SETLKW: return -EINVAL;
  default: break;
  }

  return fcntlHandler(Frame, fd, cmd, arg);
};

auto selectHandler = [](FEXCore::Core::CpuStateFrame* Frame, int nfds, fd_set32* readfds, fd_set32* writefds, fd_set32* exceptfds,
                        struct timeval32* timeout) -> uint64_t {
  struct timeval tp64 {};
  if (timeout) {
    FaultSafeUserMemAccess::VerifyIsReadable(timeout, sizeof(*timeout));
    tp64 = *timeout;
  }

  fd_set Host_readfds;
  fd_set Host_writefds;
  fd_set Host_exceptfds;
  FD_ZERO(&Host_readfds);
  FD_ZERO(&Host_writefds);
  FD_ZERO(&Host_exceptfds);

  // Round up to the full 32bit word
  uint32_t NumWords = FEXCore::AlignUp(nfds, 32) / 4;

  if (readfds) {
    FaultSafeUserMemAccess::VerifyIsReadable(readfds, sizeof(fd_set32) * NumWords);
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
    FaultSafeUserMemAccess::VerifyIsReadable(writefds, sizeof(fd_set32) * NumWords);
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
    FaultSafeUserMemAccess::VerifyIsReadable(exceptfds, sizeof(fd_set32) * NumWords);
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

  uint64_t Result = ::select(nfds, readfds ? &Host_readfds : nullptr, writefds ? &Host_writefds : nullptr,
                             exceptfds ? &Host_exceptfds : nullptr, timeout ? &tp64 : nullptr);
  if (readfds) {
    FaultSafeUserMemAccess::VerifyIsWritable(readfds, sizeof(fd_set32) * NumWords);
    for (int i = 0; i < nfds; ++i) {
      if (FD_ISSET(i, &Host_readfds)) {
        readfds[i / 32] |= 1 << (i & 31);
      } else {
        readfds[i / 32] &= ~(1 << (i & 31));
      }
    }
  }

  if (writefds) {
    FaultSafeUserMemAccess::VerifyIsWritable(writefds, sizeof(fd_set32) * NumWords);
    for (int i = 0; i < nfds; ++i) {
      if (FD_ISSET(i, &Host_writefds)) {
        writefds[i / 32] |= 1 << (i & 31);
      } else {
        writefds[i / 32] &= ~(1 << (i & 31));
      }
    }
  }

  if (exceptfds) {
    FaultSafeUserMemAccess::VerifyIsWritable(exceptfds, sizeof(fd_set32) * NumWords);
    for (int i = 0; i < nfds; ++i) {
      if (FD_ISSET(i, &Host_exceptfds)) {
        exceptfds[i / 32] |= 1 << (i & 31);
      } else {
        exceptfds[i / 32] &= ~(1 << (i & 31));
      }
    }
  }

  if (timeout) {
    FaultSafeUserMemAccess::VerifyIsWritable(timeout, sizeof(*timeout));
    *timeout = tp64;
  }
  SYSCALL_ERRNO();
};

void RegisterFD(FEX::HLE::SyscallHandler* Handler) {
  REGISTER_SYSCALL_IMPL_X32(ppoll,
                            [](FEXCore::Core::CpuStateFrame* Frame, struct pollfd* fds, nfds_t nfds, timespec32* timeout_ts,
                               const uint64_t* sigmask, size_t sigsetsize) -> uint64_t {
                              // sigsetsize is unused here since it is currently a constant and not exposed through glibc
                              struct timespec tp64 {};
                              struct timespec* timed_ptr {};
                              if (timeout_ts) {
                                struct timespec32 timeout {};
                                if (FaultSafeUserMemAccess::CopyFromUser(&timeout, timeout_ts, sizeof(timeout)) == EFAULT) {
                                  return -EFAULT;
                                }

                                tp64 = timeout;
                                timed_ptr = &tp64;
                              }

                              uint64_t Result = ::syscall(SYSCALL_DEF(ppoll), fds, nfds, timed_ptr, sigmask, sigsetsize);

                              if (timeout_ts) {
                                struct timespec32 timeout {};
                                timeout = tp64;

                                if (FaultSafeUserMemAccess::CopyToUser(timeout_ts, &timeout, sizeof(timeout)) == EFAULT) {
                                  // Write to user memory failed, this can occur if the timeout is defined in read-only memory.
                                  // This is okay to happen, kernel continues happily.
                                }
                              }

                              SYSCALL_ERRNO();
                            });

  REGISTER_SYSCALL_IMPL_X32(
    _llseek, [](FEXCore::Core::CpuStateFrame* Frame, uint32_t fd, uint32_t offset_high, uint32_t offset_low, loff_t* result, uint32_t whence) -> uint64_t {
      uint64_t Offset = offset_high;
      Offset <<= 32;
      Offset |= offset_low;
      uint64_t Result = lseek(fd, Offset, whence);
      if (Result != -1) {
        FaultSafeUserMemAccess::VerifyIsWritable(result, sizeof(*result));
        *result = Result;
        // On non-error result, llseek returns zero (As the result is returned in pointer).
        return 0;
      }
      SYSCALL_ERRNO();
    });

  REGISTER_SYSCALL_IMPL_X32(readv, [](FEXCore::Core::CpuStateFrame* Frame, int fd, const struct iovec32* iov, int iovcnt) -> uint64_t {
    FaultSafeUserMemAccess::VerifyIsReadable(iov, sizeof(struct iovec32) * SanitizeIOCount(iovcnt));
    fextl::vector<iovec> Host_iovec(iov, iov + SanitizeIOCount(iovcnt));
    uint64_t Result = ::readv(fd, Host_iovec.data(), iovcnt);
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL_X32(writev, [](FEXCore::Core::CpuStateFrame* Frame, int fd, const struct iovec32* iov, int iovcnt) -> uint64_t {
    FaultSafeUserMemAccess::VerifyIsReadable(iov, sizeof(struct iovec32) * SanitizeIOCount(iovcnt));
    fextl::vector<iovec> Host_iovec(iov, iov + SanitizeIOCount(iovcnt));
    uint64_t Result = ::writev(fd, Host_iovec.data(), iovcnt);
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL_X32(chown32, [](FEXCore::Core::CpuStateFrame* Frame, const char* pathname, uid_t owner, gid_t group) -> uint64_t {
    uint64_t Result = ::chown(pathname, owner, group);
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL_X32(lchown32, [](FEXCore::Core::CpuStateFrame* Frame, const char* pathname, uid_t owner, gid_t group) -> uint64_t {
    uint64_t Result = ::lchown(pathname, owner, group);
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL_X32(oldstat, [](FEXCore::Core::CpuStateFrame* Frame, const char* pathname, oldstat32* buf) -> uint64_t {
    struct stat host_stat;
    uint64_t Result = FEX::HLE::_SyscallHandler->FM.Stat(pathname, &host_stat);
    if (Result != -1) {
      if (host_stat.st_ino > std::numeric_limits<decltype(buf->st_ino)>::max()) {
        return -EOVERFLOW;
      }
      if (host_stat.st_nlink > std::numeric_limits<decltype(buf->st_nlink)>::max()) {
        return -EOVERFLOW;
      }
      FaultSafeUserMemAccess::VerifyIsWritable(buf, sizeof(*buf));

      *buf = host_stat;
    }
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL_X32(oldfstat, [](FEXCore::Core::CpuStateFrame* Frame, int fd, oldstat32* buf) -> uint64_t {
    struct stat host_stat;
    uint64_t Result = ::fstat(fd, &host_stat);
    if (Result != -1) {
      if (host_stat.st_ino > std::numeric_limits<decltype(buf->st_ino)>::max()) {
        return -EOVERFLOW;
      }
      if (host_stat.st_nlink > std::numeric_limits<decltype(buf->st_nlink)>::max()) {
        return -EOVERFLOW;
      }
      FaultSafeUserMemAccess::VerifyIsWritable(buf, sizeof(*buf));

      *buf = host_stat;
    }
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL_X32(oldlstat, [](FEXCore::Core::CpuStateFrame* Frame, const char* path, oldstat32* buf) -> uint64_t {
    struct stat host_stat;
    uint64_t Result = FEX::HLE::_SyscallHandler->FM.Lstat(path, &host_stat);
    if (Result != -1) {
      if (host_stat.st_ino > std::numeric_limits<decltype(buf->st_ino)>::max()) {
        return -EOVERFLOW;
      }
      if (host_stat.st_nlink > std::numeric_limits<decltype(buf->st_nlink)>::max()) {
        return -EOVERFLOW;
      }
      FaultSafeUserMemAccess::VerifyIsWritable(buf, sizeof(*buf));

      *buf = host_stat;
    }
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL_X32(stat, [](FEXCore::Core::CpuStateFrame* Frame, const char* pathname, stat32* buf) -> uint64_t {
    struct stat host_stat;
    uint64_t Result = FEX::HLE::_SyscallHandler->FM.Stat(pathname, &host_stat);
    if (Result != -1) {
      FaultSafeUserMemAccess::VerifyIsWritable(buf, sizeof(*buf));
      *buf = host_stat;
    }
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL_X32(fstat, [](FEXCore::Core::CpuStateFrame* Frame, int fd, stat32* buf) -> uint64_t {
    struct stat host_stat;
    uint64_t Result = ::fstat(fd, &host_stat);
    if (Result != -1) {
      FaultSafeUserMemAccess::VerifyIsWritable(buf, sizeof(*buf));
      *buf = host_stat;
    }
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL_X32(lstat, [](FEXCore::Core::CpuStateFrame* Frame, const char* path, stat32* buf) -> uint64_t {
    struct stat host_stat;
    uint64_t Result = FEX::HLE::_SyscallHandler->FM.Lstat(path, &host_stat);
    if (Result != -1) {
      FaultSafeUserMemAccess::VerifyIsWritable(buf, sizeof(*buf));
      *buf = host_stat;
    }
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL_X32(stat64, [](FEXCore::Core::CpuStateFrame* Frame, const char* pathname, stat64_32* buf) -> uint64_t {
    struct stat host_stat;
    uint64_t Result = FEX::HLE::_SyscallHandler->FM.Stat(pathname, &host_stat);
    if (Result != -1) {
      FaultSafeUserMemAccess::VerifyIsWritable(buf, sizeof(*buf));
      *buf = host_stat;
    }
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL_X32(lstat64, [](FEXCore::Core::CpuStateFrame* Frame, const char* path, stat64_32* buf) -> uint64_t {
    struct stat host_stat;
    uint64_t Result = FEX::HLE::_SyscallHandler->FM.Lstat(path, &host_stat);

    if (Result != -1) {
      FaultSafeUserMemAccess::VerifyIsWritable(buf, sizeof(*buf));
      *buf = host_stat;
    }
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL_X32(fstat64, [](FEXCore::Core::CpuStateFrame* Frame, int fd, stat64_32* buf) -> uint64_t {
    struct stat64 host_stat;
    uint64_t Result = ::fstat64(fd, &host_stat);
    if (Result != -1) {
      FaultSafeUserMemAccess::VerifyIsWritable(buf, sizeof(*buf));
      *buf = host_stat;
    }
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL_X32(statfs, [](FEXCore::Core::CpuStateFrame* Frame, const char* path, statfs32_32* buf) -> uint64_t {
    struct statfs host_stat;
    uint64_t Result = FEX::HLE::_SyscallHandler->FM.Statfs(path, &host_stat);
    if (Result != -1) {
      FaultSafeUserMemAccess::VerifyIsWritable(buf, sizeof(*buf));
      *buf = host_stat;
    }
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL_X32(fstatfs, [](FEXCore::Core::CpuStateFrame* Frame, int fd, statfs32_32* buf) -> uint64_t {
    struct statfs host_stat;
    uint64_t Result = ::fstatfs(fd, &host_stat);
    if (Result != -1) {
      FaultSafeUserMemAccess::VerifyIsWritable(buf, sizeof(*buf));
      *buf = host_stat;
    }
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL_X32(fstatfs64, [](FEXCore::Core::CpuStateFrame* Frame, int fd, size_t sz, struct statfs64_32* buf) -> uint64_t {
    LOGMAN_THROW_A_FMT(sz == sizeof(struct statfs64_32), "This needs to match");

    struct statfs64 host_stat;
    uint64_t Result = ::fstatfs64(fd, &host_stat);
    if (Result != -1) {
      FaultSafeUserMemAccess::VerifyIsWritable(buf, sizeof(*buf));
      *buf = host_stat;
    }
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL_X32(statfs64, [](FEXCore::Core::CpuStateFrame* Frame, const char* path, size_t sz, struct statfs64_32* buf) -> uint64_t {
    LOGMAN_THROW_A_FMT(sz == sizeof(struct statfs64_32), "This needs to match");

    struct statfs host_stat;
    uint64_t Result = FEX::HLE::_SyscallHandler->FM.Statfs(path, &host_stat);
    if (Result != -1) {
      FaultSafeUserMemAccess::VerifyIsWritable(buf, sizeof(*buf));
      *buf = host_stat;
    }

    SYSCALL_ERRNO();
  });

  // x86 32-bit fcntl syscall has a historical quirk that it uses the same handler as fcntl64
  // This is in direct opposition to all other 32-bit architectures that use the compat_fcntl handler
  // This quirk goes back to the start of the Linux 2.6.12-rc2 git history. Seeing history before
  // that point to see when this quirk happened would be difficult
  //
  // For more reference, the compat_fcntl handler blocks a few commands:
  // - F_GETLK64
  // - F_SETLK64
  // - F_SETLKW64
  // - F_OFD_GETLK
  // - F_OFD_SETLK
  // - F_OFD_SETLKW

  REGISTER_SYSCALL_IMPL_X32(fcntl, fcntl32Handler);
  REGISTER_SYSCALL_IMPL_X32(fcntl64, fcntlHandler);

  REGISTER_SYSCALL_IMPL_X32(dup, [](FEXCore::Core::CpuStateFrame* Frame, int oldfd) -> uint64_t {
    uint64_t Result = ::dup(oldfd);
    if (Result != -1) {
      CheckAndAddFDDuplication(oldfd, Result);
    }
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL_X32(dup2, [](FEXCore::Core::CpuStateFrame* Frame, int oldfd, int newfd) -> uint64_t {
    uint64_t Result = ::dup2(oldfd, newfd);
    if (Result != -1) {
      CheckAndAddFDDuplication(oldfd, newfd);
    }
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL_X32(
    preadv, [](FEXCore::Core::CpuStateFrame* Frame, int fd, const struct iovec32* iov, uint32_t iovcnt, uint32_t pos_low, uint32_t pos_high) -> uint64_t {
      FaultSafeUserMemAccess::VerifyIsReadable(iov, sizeof(struct iovec32) * SanitizeIOCount(iovcnt));
      fextl::vector<iovec> Host_iovec(iov, iov + SanitizeIOCount(iovcnt));

      uint64_t Result = ::syscall(SYSCALL_DEF(preadv), fd, Host_iovec.data(), iovcnt, pos_low, pos_high);
      SYSCALL_ERRNO();
    });

  REGISTER_SYSCALL_IMPL_X32(
    pwritev, [](FEXCore::Core::CpuStateFrame* Frame, int fd, const struct iovec32* iov, uint32_t iovcnt, uint32_t pos_low, uint32_t pos_high) -> uint64_t {
      FaultSafeUserMemAccess::VerifyIsReadable(iov, sizeof(struct iovec32) * SanitizeIOCount(iovcnt));
      fextl::vector<iovec> Host_iovec(iov, iov + SanitizeIOCount(iovcnt));

      uint64_t Result = ::syscall(SYSCALL_DEF(pwritev), fd, Host_iovec.data(), iovcnt, pos_low, pos_high);
      SYSCALL_ERRNO();
    });

  REGISTER_SYSCALL_IMPL_X32(process_vm_readv,
                            [](FEXCore::Core::CpuStateFrame* Frame, pid_t pid, const struct iovec32* local_iov, unsigned long liovcnt,
                               const struct iovec32* remote_iov, unsigned long riovcnt, unsigned long flags) -> uint64_t {
                              FaultSafeUserMemAccess::VerifyIsReadable(local_iov, sizeof(struct iovec32) * SanitizeIOCount(liovcnt));
                              FaultSafeUserMemAccess::VerifyIsReadable(remote_iov, sizeof(struct iovec32) * SanitizeIOCount(riovcnt));

                              fextl::vector<iovec> Host_local_iovec(local_iov, local_iov + SanitizeIOCount(liovcnt));
                              fextl::vector<iovec> Host_remote_iovec(remote_iov, remote_iov + SanitizeIOCount(riovcnt));

                              uint64_t Result =
                                ::process_vm_readv(pid, Host_local_iovec.data(), liovcnt, Host_remote_iovec.data(), riovcnt, flags);
                              SYSCALL_ERRNO();
                            });

  REGISTER_SYSCALL_IMPL_X32(process_vm_writev,
                            [](FEXCore::Core::CpuStateFrame* Frame, pid_t pid, const struct iovec32* local_iov, unsigned long liovcnt,
                               const struct iovec32* remote_iov, unsigned long riovcnt, unsigned long flags) -> uint64_t {
                              FaultSafeUserMemAccess::VerifyIsReadable(local_iov, sizeof(struct iovec32) * SanitizeIOCount(liovcnt));
                              FaultSafeUserMemAccess::VerifyIsReadable(remote_iov, sizeof(struct iovec32) * SanitizeIOCount(riovcnt));

                              fextl::vector<iovec> Host_local_iovec(local_iov, local_iov + SanitizeIOCount(liovcnt));
                              fextl::vector<iovec> Host_remote_iovec(remote_iov, remote_iov + SanitizeIOCount(riovcnt));

                              uint64_t Result =
                                ::process_vm_writev(pid, Host_local_iovec.data(), liovcnt, Host_remote_iovec.data(), riovcnt, flags);
                              SYSCALL_ERRNO();
                            });

  REGISTER_SYSCALL_IMPL_X32(preadv2,
                            [](FEXCore::Core::CpuStateFrame* Frame, int fd, const struct iovec32* iov, uint32_t iovcnt, uint32_t pos_low,
                               uint32_t pos_high, int flags) -> uint64_t {
                              FaultSafeUserMemAccess::VerifyIsReadable(iov, sizeof(struct iovec32) * SanitizeIOCount(iovcnt));
                              fextl::vector<iovec> Host_iovec(iov, iov + SanitizeIOCount(iovcnt));

                              uint64_t Result = ::syscall(SYSCALL_DEF(preadv2), fd, Host_iovec.data(), iovcnt, pos_low, pos_high, flags);
                              SYSCALL_ERRNO();
                            });

  REGISTER_SYSCALL_IMPL_X32(pwritev2,
                            [](FEXCore::Core::CpuStateFrame* Frame, int fd, const struct iovec32* iov, uint32_t iovcnt, uint32_t pos_low,
                               uint32_t pos_high, int flags) -> uint64_t {
                              FaultSafeUserMemAccess::VerifyIsReadable(iov, sizeof(struct iovec32) * SanitizeIOCount(iovcnt));
                              fextl::vector<iovec> Host_iovec(iov, iov + SanitizeIOCount(iovcnt));

                              uint64_t Result = ::syscall(SYSCALL_DEF(pwritev2), fd, Host_iovec.data(), iovcnt, pos_low, pos_high, flags);
                              SYSCALL_ERRNO();
                            });

  REGISTER_SYSCALL_IMPL_X32(fstatat_64, [](FEXCore::Core::CpuStateFrame* Frame, int dirfd, const char* pathname, stat64_32* buf, int flag) -> uint64_t {
    struct stat64 host_stat;
    uint64_t Result = FEX::HLE::_SyscallHandler->FM.NewFSStatAt64(dirfd, pathname, &host_stat, flag);
    if (Result != -1) {
      FaultSafeUserMemAccess::VerifyIsWritable(buf, sizeof(*buf));
      *buf = host_stat;
    }
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL_X32(ioctl, ioctl32);

  REGISTER_SYSCALL_IMPL_X32(getdents, [](FEXCore::Core::CpuStateFrame* Frame, int fd, void* dirp, uint32_t count) -> uint64_t {
    return GetDentsEmulation<true>(fd, reinterpret_cast<FEX::HLE::x32::linux_dirent_32*>(dirp), count);
  });

  REGISTER_SYSCALL_IMPL_X32(getdents64, [](FEXCore::Core::CpuStateFrame* Frame, int fd, void* dirp, uint32_t count) -> uint64_t {
    uint64_t Result = ::syscall(SYSCALL_DEF(getdents64), static_cast<uint64_t>(fd), dirp, static_cast<uint64_t>(count));
    if (Result != -1) {
      // Walk each offset
      // if we are passing the full d_off to the 32bit application then it seems to break things?
      for (size_t i = 0, num = 0; i < Result; ++num) {
        linux_dirent_64* Incoming = (linux_dirent_64*)(reinterpret_cast<uint64_t>(dirp) + i);
        Incoming->d_off = num;
        if (FEX::HLE::_SyscallHandler->FM.IsProtectedFile(fd, Incoming->d_ino)) {
          Result -= Incoming->d_reclen;
          memmove(Incoming, (linux_dirent_64*)(reinterpret_cast<uint64_t>(Incoming) + Incoming->d_reclen), Result - i);
          continue;
        }
        i += Incoming->d_reclen;
      }
    }
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL_X32(select, [](FEXCore::Core::CpuStateFrame* Frame, compat_select_args* arg) -> uint64_t {
    return selectHandler(Frame, arg->nfds, arg->readfds, arg->writefds, arg->exceptfds, arg->timeout);
  });

  REGISTER_SYSCALL_IMPL_X32(_newselect, selectHandler);

  REGISTER_SYSCALL_IMPL_X32(pselect6,
                            [](FEXCore::Core::CpuStateFrame* Frame, int nfds, fd_set32* readfds, fd_set32* writefds, fd_set32* exceptfds,
                               timespec32* timeout, compat_ptr<sigset_argpack32> sigmaskpack) -> uint64_t {
                              struct timespec tp64 {};
                              if (timeout) {
                                FaultSafeUserMemAccess::VerifyIsReadable(timeout, sizeof(*timeout));
                                tp64 = *timeout;
                              }

                              fd_set Host_readfds;
                              fd_set Host_writefds;
                              fd_set Host_exceptfds;
                              sigset_t HostSet {};

                              FD_ZERO(&Host_readfds);
                              FD_ZERO(&Host_writefds);
                              FD_ZERO(&Host_exceptfds);
                              sigemptyset(&HostSet);

                              // Round up to the full 32bit word
                              uint32_t NumWords = FEXCore::AlignUp(nfds, 32) / 4;

                              if (readfds) {
                                FaultSafeUserMemAccess::VerifyIsReadable(readfds, sizeof(fd_set32) * NumWords);
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
                                FaultSafeUserMemAccess::VerifyIsReadable(writefds, sizeof(fd_set32) * NumWords);
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
                                FaultSafeUserMemAccess::VerifyIsReadable(exceptfds, sizeof(fd_set32) * NumWords);
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

                              FaultSafeUserMemAccess::VerifyIsReadableOrNull(sigmaskpack, sizeof(*sigmaskpack));
                              if (sigmaskpack && sigmaskpack->sigset) {
                                FaultSafeUserMemAccess::VerifyIsReadable(sigmaskpack->sigset, sizeof(*sigmaskpack->sigset));
                                uint64_t* sigmask = sigmaskpack->sigset;
                                size_t sigsetsize = sigmaskpack->size;
                                for (int32_t i = 0; i < (sigsetsize * 8); ++i) {
                                  if (*sigmask & (1ULL << i)) {
                                    sigaddset(&HostSet, i + 1);
                                  }
                                }
                              }

                              uint64_t Result = ::pselect(nfds, readfds ? &Host_readfds : nullptr, writefds ? &Host_writefds : nullptr,
                                                          exceptfds ? &Host_exceptfds : nullptr, timeout ? &tp64 : nullptr, &HostSet);

                              if (readfds) {
                                FaultSafeUserMemAccess::VerifyIsWritable(readfds, sizeof(fd_set32) * NumWords);
                                for (int i = 0; i < nfds; ++i) {
                                  if (FD_ISSET(i, &Host_readfds)) {
                                    readfds[i / 32] |= 1 << (i & 31);
                                  } else {
                                    readfds[i / 32] &= ~(1 << (i & 31));
                                  }
                                }
                              }

                              if (writefds) {
                                FaultSafeUserMemAccess::VerifyIsWritable(writefds, sizeof(fd_set32) * NumWords);
                                for (int i = 0; i < nfds; ++i) {
                                  if (FD_ISSET(i, &Host_writefds)) {
                                    writefds[i / 32] |= 1 << (i & 31);
                                  } else {
                                    writefds[i / 32] &= ~(1 << (i & 31));
                                  }
                                }
                              }

                              if (exceptfds) {
                                FaultSafeUserMemAccess::VerifyIsWritable(exceptfds, sizeof(fd_set32) * NumWords);
                                for (int i = 0; i < nfds; ++i) {
                                  if (FD_ISSET(i, &Host_exceptfds)) {
                                    exceptfds[i / 32] |= 1 << (i & 31);
                                  } else {
                                    exceptfds[i / 32] &= ~(1 << (i & 31));
                                  }
                                }
                              }

                              if (timeout) {
                                FaultSafeUserMemAccess::VerifyIsWritable(timeout, sizeof(*timeout));
                                *timeout = tp64;
                              }
                              SYSCALL_ERRNO();
                            });

  REGISTER_SYSCALL_IMPL_X32(
    fadvise64, [](FEXCore::Core::CpuStateFrame* Frame, int32_t fd, uint32_t offset_low, uint32_t offset_high, uint32_t len, int advice) -> uint64_t {
      uint64_t Offset = offset_high;
      Offset <<= 32;
      Offset |= offset_low;
      uint64_t Result = ::posix_fadvise64(fd, Offset, len, advice);
      SYSCALL_ERRNO();
    });

  REGISTER_SYSCALL_IMPL_X32(fadvise64_64,
                            [](FEXCore::Core::CpuStateFrame* Frame, int32_t fd, uint32_t offset_low, uint32_t offset_high, uint32_t len_low,
                               uint32_t len_high, int advice) -> uint64_t {
                              uint64_t Offset = offset_high;
                              Offset <<= 32;
                              Offset |= offset_low;
                              uint64_t Len = len_high;
                              Len <<= 32;
                              Len |= len_low;
                              uint64_t Result = ::posix_fadvise64(fd, Offset, Len, advice);
                              SYSCALL_ERRNO();
                            });

  REGISTER_SYSCALL_IMPL_X32(timerfd_settime,
                            [](FEXCore::Core::CpuStateFrame* Frame, int fd, int flags, const FEX::HLE::x32::old_itimerspec32* new_value,
                               FEX::HLE::x32::old_itimerspec32* old_value) -> uint64_t {
                              struct itimerspec new_value_host {};
                              struct itimerspec old_value_host {};
                              struct itimerspec* old_value_host_p {};

                              new_value_host = *new_value;
                              if (old_value) {
                                FaultSafeUserMemAccess::VerifyIsReadable(old_value, sizeof(*old_value));
                                old_value_host_p = &old_value_host;
                              }

                              // Flags don't need remapped
                              uint64_t Result = ::timerfd_settime(fd, flags, &new_value_host, old_value_host_p);

                              if (Result != -1 && old_value) {
                                FaultSafeUserMemAccess::VerifyIsWritable(old_value, sizeof(*old_value));
                                *old_value = old_value_host;
                              }
                              SYSCALL_ERRNO();
                            });

  REGISTER_SYSCALL_IMPL_X32(timerfd_gettime, [](FEXCore::Core::CpuStateFrame* Frame, int fd, FEX::HLE::x32::old_itimerspec32* curr_value) -> uint64_t {
    struct itimerspec Host {};

    uint64_t Result = ::timerfd_gettime(fd, &Host);

    if (Result != -1) {
      FaultSafeUserMemAccess::VerifyIsWritable(curr_value, sizeof(*curr_value));
      *curr_value = Host;
    }

    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL_X32(pselect6_time64,
                            [](FEXCore::Core::CpuStateFrame* Frame, int nfds, fd_set32* readfds, fd_set32* writefds, fd_set32* exceptfds,
                               struct timespec* timeout, compat_ptr<sigset_argpack32> sigmaskpack) -> uint64_t {
                              fd_set Host_readfds;
                              fd_set Host_writefds;
                              fd_set Host_exceptfds;
                              sigset_t HostSet {};

                              FD_ZERO(&Host_readfds);
                              FD_ZERO(&Host_writefds);
                              FD_ZERO(&Host_exceptfds);
                              sigemptyset(&HostSet);

                              // Round up to the full 32bit word
                              uint32_t NumWords = FEXCore::AlignUp(nfds, 32) / 4;

                              if (readfds) {
                                FaultSafeUserMemAccess::VerifyIsReadable(readfds, sizeof(fd_set32) * NumWords);
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
                                FaultSafeUserMemAccess::VerifyIsReadable(writefds, sizeof(fd_set32) * NumWords);
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
                                FaultSafeUserMemAccess::VerifyIsReadable(exceptfds, sizeof(fd_set32) * NumWords);
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

                              FaultSafeUserMemAccess::VerifyIsReadableOrNull(sigmaskpack, sizeof(*sigmaskpack));
                              if (sigmaskpack && sigmaskpack->sigset) {
                                FaultSafeUserMemAccess::VerifyIsReadable(sigmaskpack->sigset, sizeof(*sigmaskpack->sigset));
                                uint64_t* sigmask = sigmaskpack->sigset;
                                size_t sigsetsize = sigmaskpack->size;
                                for (int32_t i = 0; i < (sigsetsize * 8); ++i) {
                                  if (*sigmask & (1ULL << i)) {
                                    sigaddset(&HostSet, i + 1);
                                  }
                                }
                              }

                              uint64_t Result = ::pselect(nfds, readfds ? &Host_readfds : nullptr, writefds ? &Host_writefds : nullptr,
                                                          exceptfds ? &Host_exceptfds : nullptr, timeout, &HostSet);

                              if (readfds) {
                                FaultSafeUserMemAccess::VerifyIsWritable(readfds, sizeof(fd_set32) * NumWords);
                                for (int i = 0; i < nfds; ++i) {
                                  if (FD_ISSET(i, &Host_readfds)) {
                                    readfds[i / 32] |= 1 << (i & 31);
                                  } else {
                                    readfds[i / 32] &= ~(1 << (i & 31));
                                  }
                                }
                              }

                              if (writefds) {
                                FaultSafeUserMemAccess::VerifyIsWritable(writefds, sizeof(fd_set32) * NumWords);
                                for (int i = 0; i < nfds; ++i) {
                                  if (FD_ISSET(i, &Host_writefds)) {
                                    writefds[i / 32] |= 1 << (i & 31);
                                  } else {
                                    writefds[i / 32] &= ~(1 << (i & 31));
                                  }
                                }
                              }

                              if (exceptfds) {
                                FaultSafeUserMemAccess::VerifyIsWritable(exceptfds, sizeof(fd_set32) * NumWords);
                                for (int i = 0; i < nfds; ++i) {
                                  if (FD_ISSET(i, &Host_exceptfds)) {
                                    exceptfds[i / 32] |= 1 << (i & 31);
                                  } else {
                                    exceptfds[i / 32] &= ~(1 << (i & 31));
                                  }
                                }
                              }

                              SYSCALL_ERRNO();
                            });

  REGISTER_SYSCALL_IMPL_X32(sendfile, [](FEXCore::Core::CpuStateFrame* Frame, int out_fd, int in_fd, compat_off_t* offset, size_t count) -> uint64_t {
    off_t Local {};
    off_t* Local_p {};
    if (offset) {
      Local_p = &Local;
      Local = *offset;
    }
    uint64_t Result = ::sendfile(out_fd, in_fd, Local_p, count);
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL_X32(
    pread_64, [](FEXCore::Core::CpuStateFrame* Frame, int fd, void* buf, uint32_t count, uint32_t offset_low, uint32_t offset_high) -> uint64_t {
      uint64_t Offset = offset_high;
      Offset <<= 32;
      Offset |= offset_low;

      uint64_t Result = ::pread64(fd, buf, count, Offset);
      SYSCALL_ERRNO();
    });

  REGISTER_SYSCALL_IMPL_X32(
    pwrite_64, [](FEXCore::Core::CpuStateFrame* Frame, int fd, void* buf, uint32_t count, uint32_t offset_low, uint32_t offset_high) -> uint64_t {
      uint64_t Offset = offset_high;
      Offset <<= 32;
      Offset |= offset_low;

      uint64_t Result = ::pwrite64(fd, buf, count, Offset);
      SYSCALL_ERRNO();
    });

  REGISTER_SYSCALL_IMPL_X32(
    readahead, [](FEXCore::Core::CpuStateFrame* Frame, int fd, uint32_t offset_low, uint64_t offset_high, size_t count) -> uint64_t {
      uint64_t Offset = offset_high;
      Offset <<= 32;
      Offset |= offset_low;

      uint64_t Result = ::readahead(fd, Offset, count);
      SYSCALL_ERRNO();
    });

  REGISTER_SYSCALL_IMPL_X32(sync_file_range,
                            [](FEXCore::Core::CpuStateFrame* Frame, int fd, uint32_t offset_low, uint32_t offset_high, uint32_t len_low,
                               uint32_t len_high, unsigned int flags) -> uint64_t {
                              // Flags don't need remapped
                              uint64_t Offset = offset_high;
                              Offset <<= 32;
                              Offset |= offset_low;

                              uint64_t Len = len_high;
                              Len <<= 32;
                              Len |= len_low;

                              uint64_t Result = ::syscall(SYSCALL_DEF(sync_file_range), fd, Offset, Len, flags);
                              SYSCALL_ERRNO();
                            });

  REGISTER_SYSCALL_IMPL_X32(fallocate,
                            [](FEXCore::Core::CpuStateFrame* Frame, int fd, int mode, uint32_t offset_low, uint32_t offset_high,
                               uint32_t len_low, uint32_t len_high) -> uint64_t {
                              uint64_t Offset = offset_high;
                              Offset <<= 32;
                              Offset |= offset_low;

                              uint64_t Len = len_high;
                              Len <<= 32;
                              Len |= len_low;

                              uint64_t Result = ::fallocate(fd, mode, Offset, Len);
                              SYSCALL_ERRNO();
                            });

  REGISTER_SYSCALL_IMPL_X32(
    vmsplice, [](FEXCore::Core::CpuStateFrame* Frame, int fd, const struct iovec32* iov, unsigned long nr_segs, unsigned int flags) -> uint64_t {
      FaultSafeUserMemAccess::VerifyIsReadable(iov, sizeof(struct iovec32) * SanitizeIOCount(nr_segs));
      fextl::vector<iovec> Host_iovec(iov, iov + nr_segs);
      uint64_t Result = ::vmsplice(fd, Host_iovec.data(), nr_segs, flags);
      SYSCALL_ERRNO();
    });
}
} // namespace FEX::HLE::x32
