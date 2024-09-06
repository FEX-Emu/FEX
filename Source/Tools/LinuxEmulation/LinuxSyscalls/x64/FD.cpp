// SPDX-License-Identifier: MIT
/*
$info$
tags: LinuxSyscalls|syscalls-x86-64
$end_info$
*/

#include "LinuxSyscalls/FileManagement.h"
#include "LinuxSyscalls/Syscalls.h"
#include "LinuxSyscalls/x64/Syscalls.h"
#include "LinuxSyscalls/x64/Types.h"

#include <FEXCore/Utils/CompilerDefs.h>
#include <FEXCore/Utils/MathUtils.h>

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
void RegisterFD(FEX::HLE::SyscallHandler* Handler) {
  REGISTER_SYSCALL_IMPL_X64(
    select, [](FEXCore::Core::CpuStateFrame* Frame, int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, struct timeval* timeout) -> uint64_t {
      ///< All FD arrays need to be writable
      FaultSafeUserMemAccess::VerifyIsWritableOrNull(readfds, sizeof(uint64_t) * nfds);
      FaultSafeUserMemAccess::VerifyIsWritableOrNull(writefds, sizeof(uint64_t) * nfds);
      FaultSafeUserMemAccess::VerifyIsWritableOrNull(exceptfds, sizeof(uint64_t) * nfds);
      FaultSafeUserMemAccess::VerifyIsReadableOrNull(timeout, sizeof(*timeout));
      ///< timeout doesn't actually need to be writable, this is a quirk of glibc. Kernel just doesn't update timeout if not possible.
      FaultSafeUserMemAccess::VerifyIsWritableOrNull(timeout, sizeof(*timeout));
      uint64_t Result = ::select(nfds, readfds, writefds, exceptfds, timeout);
      SYSCALL_ERRNO();
    });

  REGISTER_SYSCALL_IMPL_X64(fcntl, [](FEXCore::Core::CpuStateFrame* Frame, int fd, int cmd, uint64_t arg) -> uint64_t {
    uint64_t Result {};
    switch (cmd) {
    case F_GETFL:
      Result = ::fcntl(fd, cmd, arg);
      if (Result != -1) {
        Result = FEX::HLE::RemapToX86Flags(Result);
      }
      break;
    case F_SETFL: Result = ::fcntl(fd, cmd, FEX::HLE::RemapFromX86Flags(arg)); break;
    default: Result = ::fcntl(fd, cmd, arg); break;
    }
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL_X64(
    futimesat, [](FEXCore::Core::CpuStateFrame* Frame, int dirfd, const char* pathname, const struct timeval times[2]) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(futimesat), dirfd, pathname, times);
      SYSCALL_ERRNO();
    });

  REGISTER_SYSCALL_IMPL_X64(stat, [](FEXCore::Core::CpuStateFrame* Frame, const char* pathname, FEX::HLE::x64::guest_stat* buf) -> uint64_t {
    FaultSafeUserMemAccess::VerifyIsStringReadableMaxSize(pathname, PATH_MAX);
    FaultSafeUserMemAccess::VerifyIsWritable(buf, sizeof(*buf));
    struct stat host_stat;
    uint64_t Result = FEX::HLE::_SyscallHandler->FM.Stat(pathname, &host_stat);
    if (Result != -1) {
      *buf = host_stat;
    }
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL_X64(fstat, [](FEXCore::Core::CpuStateFrame* Frame, int fd, FEX::HLE::x64::guest_stat* buf) -> uint64_t {
    FaultSafeUserMemAccess::VerifyIsWritable(buf, sizeof(*buf));
    struct stat host_stat;
    uint64_t Result = ::fstat(fd, &host_stat);
    if (Result != -1) {
      *buf = host_stat;
    }
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL_X64(lstat, [](FEXCore::Core::CpuStateFrame* Frame, const char* path, FEX::HLE::x64::guest_stat* buf) -> uint64_t {
    FaultSafeUserMemAccess::VerifyIsStringReadableMaxSize(path, PATH_MAX);
    FaultSafeUserMemAccess::VerifyIsWritable(buf, sizeof(*buf));
    struct stat host_stat;
    uint64_t Result = FEX::HLE::_SyscallHandler->FM.Lstat(path, &host_stat);
    if (Result != -1) {
      *buf = host_stat;
    }
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL_X64(
    newfstatat, [](FEXCore::Core::CpuStateFrame* Frame, int dirfd, const char* pathname, FEX::HLE::x64::guest_stat* buf, int flag) -> uint64_t {
      FaultSafeUserMemAccess::VerifyIsStringReadableMaxSize(pathname, PATH_MAX);
      FaultSafeUserMemAccess::VerifyIsWritable(buf, sizeof(*buf));
      struct stat host_stat;
      uint64_t Result = FEX::HLE::_SyscallHandler->FM.NewFSStatAt(dirfd, pathname, &host_stat, flag);
      if (Result != -1) {
        *buf = host_stat;
      }
      SYSCALL_ERRNO();
    });

  REGISTER_SYSCALL_IMPL_X64(getdents, [](FEXCore::Core::CpuStateFrame* Frame, int fd, void* dirp, uint32_t count) -> uint64_t {
    return GetDentsEmulation<false>(fd, reinterpret_cast<FEX::HLE::x64::linux_dirent*>(dirp), count);
  });

  REGISTER_SYSCALL_IMPL_X64(dup2, [](FEXCore::Core::CpuStateFrame* Frame, int oldfd, int newfd) -> uint64_t {
    uint64_t Result = ::dup2(oldfd, newfd);
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL_X64(statfs, [](FEXCore::Core::CpuStateFrame* Frame, const char* path, struct statfs* buf) -> uint64_t {
    FaultSafeUserMemAccess::VerifyIsStringReadableMaxSize(path, PATH_MAX);
    FaultSafeUserMemAccess::VerifyIsWritable(buf, sizeof(*buf));
    uint64_t Result = FEX::HLE::_SyscallHandler->FM.Statfs(path, buf);
    SYSCALL_ERRNO();
  });
}
} // namespace FEX::HLE::x64
