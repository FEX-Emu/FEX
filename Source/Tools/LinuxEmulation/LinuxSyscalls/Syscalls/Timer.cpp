// SPDX-License-Identifier: MIT
/*
$info$
tags: LinuxSyscalls|syscalls-shared
$end_info$
*/

#include "LinuxSyscalls/Syscalls.h"
#include "LinuxSyscalls/Types.h"
#include "LinuxSyscalls/x64/Syscalls.h"
#include "LinuxSyscalls/x32/Syscalls.h"

#include <cstddef>
#include <cstdint>
#include <csignal>
#include <ctime>
#include <sys/time.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace FEX::HLE {
void RegisterTimer(FEX::HLE::SyscallHandler* Handler) {
  REGISTER_SYSCALL_IMPL(alarm, [](FEXCore::Core::CpuStateFrame* Frame, unsigned int seconds) -> uint64_t {
    uint64_t Result = ::alarm(seconds);
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL(pause, [](FEXCore::Core::CpuStateFrame* Frame) -> uint64_t {
    uint64_t Result = ::pause();
    SYSCALL_ERRNO();
  });
}
} // namespace FEX::HLE
