/*
$info$
tags: LinuxSyscalls|syscalls-x86-32
$end_info$
*/

#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/x32/Syscalls.h"
#include "Tests/LinuxSyscalls/x32/Types.h"

#include <bits/types/timer_t.h>
#include <stdint.h>
#include <syscall.h>
#include <sys/time.h>
#include <unistd.h>

namespace FEXCore::Core {
  struct CpuStateFrame;
}

namespace FEX::HLE::x32 {
  void RegisterTimer() {
    REGISTER_SYSCALL_IMPL_X32(timer_settime64, [](FEXCore::Core::CpuStateFrame *Frame, timer_t timerid, int flags, const struct itimerspec *new_value, struct itimerspec *old_value) -> uint64_t {
      uint64_t Result = ::syscall(SYS_timer_settime, timerid, flags, new_value, old_value);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(timer_gettime64, [](FEXCore::Core::CpuStateFrame *Frame, timer_t timerid, struct itimerspec *curr_value) -> uint64_t {
      uint64_t Result = ::syscall(SYS_timer_gettime, timerid, curr_value);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(getitimer, [](FEXCore::Core::CpuStateFrame *Frame, int which, FEX::HLE::x32::itimerval32 *curr_value) -> uint64_t {
      itimerval val{};
      itimerval *val_p{};
      if (curr_value) {
        val_p = &val;
      }
      uint64_t Result = ::getitimer(which, val_p);
      if (curr_value) {
        *curr_value = val;
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(setitimer, [](FEXCore::Core::CpuStateFrame *Frame, int which, const FEX::HLE::x32::itimerval32 *new_value, FEX::HLE::x32::itimerval32 *old_value) -> uint64_t {
      itimerval val{};
      itimerval old{};
      itimerval *val_p{};
      itimerval *old_p{};

      if (new_value) {
        val = *new_value;
        val_p = &val;
      }

      if (old_value) {
        old_p = &old;
      }

      uint64_t Result = ::setitimer(which, val_p, old_p);

      if (old_value) {
        *old_value = old;
      }
      SYSCALL_ERRNO();
    });
  }
}
