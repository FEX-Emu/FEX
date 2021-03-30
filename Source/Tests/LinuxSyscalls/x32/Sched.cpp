/*
$info$
tags: LinuxSyscalls|syscalls-x86-32
$end_info$
*/

#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/x32/Syscalls.h"

#include <stdint.h>
#include <sched.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace FEX::HLE::x32 {
  void RegisterSched() {
    REGISTER_SYSCALL_IMPL_X32(sched_rr_get_interval, [](FEXCore::Core::CpuStateFrame *Frame, pid_t pid, struct timespec32 *tp) -> uint64_t {
      struct timespec tp64{};
      uint64_t Result = ::sched_rr_get_interval(pid, tp ? &tp64 : nullptr);
      if (tp) {
        *tp = tp64;
      }
      SYSCALL_ERRNO();
    });
  }
}
