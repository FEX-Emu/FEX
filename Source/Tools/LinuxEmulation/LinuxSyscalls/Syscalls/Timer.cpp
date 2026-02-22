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

#include <stddef.h>
#include <stdint.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace FEX::HLE {
auto alarm(FEXCore::Core::CpuStateFrame* Frame, unsigned int seconds) -> uint64_t {
  uint64_t Result = ::alarm(seconds);
  SYSCALL_ERRNO();
}

auto pause(FEXCore::Core::CpuStateFrame* Frame) -> uint64_t {
  uint64_t Result = ::pause();
  SYSCALL_ERRNO();
}

void RegisterTimer(FEX::HLE::SyscallHandler* Handler) {
  using namespace FEXCore::IR;
  REGISTER_SYSCALL_IMPL(alarm, alarm);
  REGISTER_SYSCALL_IMPL(pause, pause);
}
} // namespace FEX::HLE
