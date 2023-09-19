// SPDX-License-Identifier: MIT
/*
$info$
tags: LinuxSyscalls|syscalls-x86-64
$end_info$
*/

#include "LinuxSyscalls/Syscalls.h"
#include "LinuxSyscalls/x64/Syscalls.h"

#include <stdint.h>
#include <sys/ioctl.h>

namespace FEXCore::Core {
  struct CpuStateFrame;
}

namespace FEX::HLE::x64 {
  void RegisterIoctl(FEX::HLE::SyscallHandler *Handler) {
    REGISTER_SYSCALL_IMPL_X64_PASS(ioctl, [](FEXCore::Core::CpuStateFrame *Frame, int fd, uint64_t request, void *args) -> uint64_t {
      uint64_t Result = ::ioctl(fd, request, args);
      SYSCALL_ERRNO();
    });
  }
}
