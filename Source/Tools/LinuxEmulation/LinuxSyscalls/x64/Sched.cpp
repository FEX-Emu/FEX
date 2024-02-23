// SPDX-License-Identifier: MIT
/*
$info$
tags: LinuxSyscalls|syscalls-x86-64
$end_info$
*/

#include "LinuxSyscalls/Syscalls.h"
#include "LinuxSyscalls/x64/Syscalls.h"

#include <stdint.h>
#include <sched.h>
#include <unistd.h>

namespace FEXCore::Core {
struct CpuStateFrame;
}

namespace FEX::HLE::x64 {
void RegisterSched(FEX::HLE::SyscallHandler* Handler) {
  REGISTER_SYSCALL_IMPL_X64_PASS(sched_rr_get_interval, [](FEXCore::Core::CpuStateFrame* Frame, pid_t pid, struct timespec* tp) -> uint64_t {
    uint64_t Result = ::sched_rr_get_interval(pid, tp);
    SYSCALL_ERRNO();
  });
}
} // namespace FEX::HLE::x64
