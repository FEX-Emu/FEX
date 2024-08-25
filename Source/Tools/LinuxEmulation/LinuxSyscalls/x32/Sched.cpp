// SPDX-License-Identifier: MIT
/*
$info$
tags: LinuxSyscalls|syscalls-x86-32
$end_info$
*/

#include "LinuxSyscalls/Syscalls.h"
#include "LinuxSyscalls/x64/Syscalls.h"
#include "LinuxSyscalls/x32/Syscalls.h"
#include "LinuxSyscalls/x32/Types.h"

#include <stdint.h>
#include <sched.h>
#include <time.h>
#include <unistd.h>

namespace FEXCore::Core {
struct CpuStateFrame;
}

namespace FEX::HLE::x32 {
void RegisterSched(FEX::HLE::SyscallHandler* Handler) {
  REGISTER_SYSCALL_IMPL_X32(sched_rr_get_interval, [](FEXCore::Core::CpuStateFrame* Frame, pid_t pid, struct timespec32* tp) -> uint64_t {
    struct timespec tp64 {};
    uint64_t Result = ::sched_rr_get_interval(pid, tp ? &tp64 : nullptr);
    if (tp) {
      FaultSafeUserMemAccess::VerifyIsWritable(tp, sizeof(*tp));
      *tp = tp64;
    }
    SYSCALL_ERRNO();
  });
}
} // namespace FEX::HLE::x32
