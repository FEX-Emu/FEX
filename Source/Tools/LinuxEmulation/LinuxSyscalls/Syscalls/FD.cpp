// SPDX-License-Identifier: MIT
/*
$info$
tags: LinuxSyscalls|syscalls-shared
$end_info$
*/

#include "LinuxSyscalls/Syscalls.h"
#include "LinuxSyscalls/x64/Syscalls.h"
#include "LinuxSyscalls/x32/Syscalls.h"

#include <FEXCore/IR/IR.h>

#include <FEXHeaderUtils/Syscalls.h>

#include <fcntl.h>
#include <stdint.h>
#include <sys/file.h>
#include <sys/eventfd.h>
#include <sys/inotify.h>
#include <sys/mman.h>
#include <sys/timerfd.h>
#include <poll.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/eventfd.h>
#include <sys/syscall.h>

namespace FEX::HLE {
void RegisterFD(FEX::HLE::SyscallHandler* Handler) {
  using namespace FEXCore::IR;
  REGISTER_SYSCALL_IMPL(poll, [](FEXCore::Core::CpuStateFrame* Frame, struct pollfd* fds, nfds_t nfds, int timeout) -> uint64_t {
    if (nfds) {
      // fds is allowed to be garbage if nfds is zero.
      FaultSafeUserMemAccess::VerifyIsWritable(fds, sizeof(struct pollfd) * nfds);
    }
    uint64_t Result = ::poll(fds, nfds, timeout);
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL(open, [](FEXCore::Core::CpuStateFrame* Frame, const char* pathname, int flags, uint32_t mode) -> uint64_t {
    flags = FEX::HLE::RemapFromX86Flags(flags);
    uint64_t Result = FEX::HLE::_SyscallHandler->FM.Open(pathname, flags, mode);
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL(close, [](FEXCore::Core::CpuStateFrame* Frame, int fd) -> uint64_t {
    uint64_t Result = FEX::HLE::_SyscallHandler->FM.Close(fd);
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL(chown, [](FEXCore::Core::CpuStateFrame* Frame, const char* pathname, uid_t owner, gid_t group) -> uint64_t {
    uint64_t Result = ::chown(pathname, owner, group);
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL(lchown, [](FEXCore::Core::CpuStateFrame* Frame, const char* pathname, uid_t owner, gid_t group) -> uint64_t {
    uint64_t Result = ::lchown(pathname, owner, group);
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL(access, [](FEXCore::Core::CpuStateFrame* Frame, const char* pathname, int mode) -> uint64_t {
    uint64_t Result = FEX::HLE::_SyscallHandler->FM.Access(pathname, mode);
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL(pipe, [](FEXCore::Core::CpuStateFrame* Frame, int pipefd[2]) -> uint64_t {
    uint64_t Result = ::pipe(pipefd);
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL(dup3, [](FEXCore::Core::CpuStateFrame* Frame, int oldfd, int newfd, int flags) -> uint64_t {
    flags = FEX::HLE::RemapFromX86Flags(flags);
    uint64_t Result = ::dup3(oldfd, newfd, flags);
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL(inotify_init, [](FEXCore::Core::CpuStateFrame* Frame) -> uint64_t {
    uint64_t Result = ::inotify_init();
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL(openat, [](FEXCore::Core::CpuStateFrame* Frame, int dirfs, const char* pathname, int flags, uint32_t mode) -> uint64_t {
    flags = FEX::HLE::RemapFromX86Flags(flags);
    uint64_t Result = FEX::HLE::_SyscallHandler->FM.Openat(dirfs, pathname, flags, mode);
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL(readlinkat, [](FEXCore::Core::CpuStateFrame* Frame, int dirfd, const char* pathname, char* buf, size_t bufsiz) -> uint64_t {
    uint64_t Result = FEX::HLE::_SyscallHandler->FM.Readlinkat(dirfd, pathname, buf, bufsiz);
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL(faccessat, [](FEXCore::Core::CpuStateFrame* Frame, int dirfd, const char* pathname, int mode) -> uint64_t {
    uint64_t Result = FEX::HLE::_SyscallHandler->FM.FAccessat(dirfd, pathname, mode);
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL(faccessat2, [](FEXCore::Core::CpuStateFrame* Frame, int dirfd, const char* pathname, int mode, int flags) -> uint64_t {
    uint64_t Result = FEX::HLE::_SyscallHandler->FM.FAccessat2(dirfd, pathname, mode, flags);
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL(
    openat2, [](FEXCore::Core::CpuStateFrame* Frame, int dirfs, const char* pathname, struct open_how* how, size_t usize) -> uint64_t {
      open_how HostHow {};
      size_t HostSize = std::min(sizeof(open_how), usize);
      memcpy(&HostHow, how, HostSize);

      HostHow.flags = FEX::HLE::RemapFromX86Flags(HostHow.flags);
      uint64_t Result = FEX::HLE::_SyscallHandler->FM.Openat2(dirfs, pathname, &HostHow, HostSize);
      SYSCALL_ERRNO();
    });

  REGISTER_SYSCALL_IMPL(eventfd, [](FEXCore::Core::CpuStateFrame* Frame, uint32_t count) -> uint64_t {
    uint64_t Result = ::syscall(SYSCALL_DEF(eventfd2), count, 0);
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL(pipe2, [](FEXCore::Core::CpuStateFrame* Frame, int pipefd[2], int flags) -> uint64_t {
    flags = FEX::HLE::RemapFromX86Flags(flags);
    uint64_t Result = ::pipe2(pipefd, flags);
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL(
    statx, [](FEXCore::Core::CpuStateFrame* Frame, int dirfd, const char* pathname, int flags, uint32_t mask, struct statx* statxbuf) -> uint64_t {
      // Flags don't need remapped
      uint64_t Result = FEX::HLE::_SyscallHandler->FM.Statx(dirfd, pathname, flags, mask, statxbuf);
      SYSCALL_ERRNO();
    });

  REGISTER_SYSCALL_IMPL(close_range, [](FEXCore::Core::CpuStateFrame* Frame, unsigned int first, unsigned int last, unsigned int flags) -> uint64_t {
    uint64_t Result = FEX::HLE::_SyscallHandler->FM.CloseRange(first, last, flags);
    SYSCALL_ERRNO();
  });
}
} // namespace FEX::HLE
