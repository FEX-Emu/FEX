// SPDX-License-Identifier: MIT
/*
$info$
tags: LinuxSyscalls|syscalls-shared
$end_info$
*/

#include "LinuxSyscalls/Syscalls.h"
#include "LinuxSyscalls/Syscalls/Thread.h"
#include "LinuxSyscalls/x64/Syscalls.h"
#include "LinuxSyscalls/x32/Syscalls.h"

#include <FEXCore/IR/IR.h>

#include <signal.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace SignalDelegator {
  struct GuestSigAction;
}


namespace FEX::HLE {
  void RegisterNamespace(FEX::HLE::SyscallHandler *Handler) {
    using namespace FEXCore::IR;

    if (Handler->GetHostKernelVersion() >= FEX::HLE::SyscallHandler::KernelVersion(5, 1, 0)) {
      REGISTER_SYSCALL_IMPL_PASS_FLAGS(open_tree, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
        [](FEXCore::Core::CpuStateFrame *Frame, int dfd, const char *filename, unsigned int flags) -> uint64_t {
        uint64_t Result = ::syscall(SYSCALL_DEF(open_tree), dfd, filename, flags);
        SYSCALL_ERRNO();
      });

      REGISTER_SYSCALL_IMPL_PASS_FLAGS(move_mount, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
        [](FEXCore::Core::CpuStateFrame *Frame, int from_dfd, const char *from_pathname, int to_dfd, const char *to_pathname, unsigned int flags) -> uint64_t {
        uint64_t Result = ::syscall(SYSCALL_DEF(move_mount), from_dfd, from_pathname, to_dfd, to_pathname, flags);
        SYSCALL_ERRNO();
      });

      REGISTER_SYSCALL_IMPL_PASS_FLAGS(fsopen, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
        [](FEXCore::Core::CpuStateFrame *Frame, int dfd, const char *path, unsigned int flags) -> uint64_t {
        uint64_t Result = ::syscall(SYSCALL_DEF(fsopen), dfd, path, flags);
        SYSCALL_ERRNO();
      });

      REGISTER_SYSCALL_IMPL_PASS_FLAGS(fsconfig, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
        [](FEXCore::Core::CpuStateFrame *Frame, int fd, unsigned int cmd, const char *key, const void *value, int aux) -> uint64_t {
        uint64_t Result = ::syscall(SYSCALL_DEF(fsconfig), fd, cmd, key, value, aux);
        SYSCALL_ERRNO();
      });

      REGISTER_SYSCALL_IMPL_PASS_FLAGS(fsmount, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
        [](FEXCore::Core::CpuStateFrame *Frame, int fs_fd, uint32_t flags, uint32_t attr_flags) -> uint64_t {
        uint64_t Result = ::syscall(SYSCALL_DEF(fsmount), fs_fd, flags, attr_flags);
        SYSCALL_ERRNO();
      });

      REGISTER_SYSCALL_IMPL_PASS_FLAGS(fspick, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
        [](FEXCore::Core::CpuStateFrame *Frame, int dfd, const char *path, unsigned int flags) -> uint64_t {
        uint64_t Result = ::syscall(SYSCALL_DEF(fspick), dfd, path, flags);
        SYSCALL_ERRNO();
      });
    }
    else {
      REGISTER_SYSCALL_IMPL(open_tree, UnimplementedSyscallSafe);
      REGISTER_SYSCALL_IMPL(move_mount, UnimplementedSyscallSafe);
      REGISTER_SYSCALL_IMPL(fsopen, UnimplementedSyscallSafe);
      REGISTER_SYSCALL_IMPL(fsconfig, UnimplementedSyscallSafe);
      REGISTER_SYSCALL_IMPL(fsmount, UnimplementedSyscallSafe);
      REGISTER_SYSCALL_IMPL(fspick, UnimplementedSyscallSafe);
    }

    if (Handler->GetHostKernelVersion() >= FEX::HLE::SyscallHandler::KernelVersion(5, 12, 0)) {
      REGISTER_SYSCALL_IMPL_PASS_FLAGS(mount_setattr, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
        [](FEXCore::Core::CpuStateFrame *Frame, int dfd, const char *path, unsigned int flags, void *uattr, size_t usize) -> uint64_t {
        uint64_t Result = ::syscall(SYSCALL_DEF(mount_setattr), dfd, path, flags, uattr, usize);
        SYSCALL_ERRNO();
      });
    }
    else {
      REGISTER_SYSCALL_IMPL(mount_setattr, UnimplementedSyscallSafe);
    }
  }
}
