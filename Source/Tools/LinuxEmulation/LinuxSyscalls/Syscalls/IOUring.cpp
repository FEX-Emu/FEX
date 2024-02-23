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
void RegisterIOUring(FEX::HLE::SyscallHandler* Handler) {
  using namespace FEXCore::IR;

  if (Handler->IsHostKernelVersionAtLeast(5, 1, 0)) {
    REGISTER_SYSCALL_IMPL_PASS_FLAGS(io_uring_setup, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                     [](FEXCore::Core::CpuStateFrame* Frame, uint32_t entries, void* params) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(io_uring_setup), entries, params);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(io_uring_enter, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                     [](FEXCore::Core::CpuStateFrame* Frame, unsigned int fd, uint32_t to_submit, uint32_t min_complete,
                                        uint32_t flags, void* argp, size_t argsz) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(io_uring_enter), fd, to_submit, min_complete, flags, argp, argsz);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(
      io_uring_register, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame* Frame, unsigned int fd, unsigned int opcode, void* arg, uint32_t nr_args) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(io_uring_register), fd, opcode, arg, nr_args);
      SYSCALL_ERRNO();
    });
  } else {
    REGISTER_SYSCALL_IMPL(io_uring_setup, UnimplementedSyscallSafe);
    REGISTER_SYSCALL_IMPL(io_uring_enter, UnimplementedSyscallSafe);
    REGISTER_SYSCALL_IMPL(io_uring_register, UnimplementedSyscallSafe);
  }
}
} // namespace FEX::HLE
