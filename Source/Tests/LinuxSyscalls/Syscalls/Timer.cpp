/*
$info$
tags: LinuxSyscalls|syscalls-shared
$end_info$
*/

#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/x64/Syscalls.h"
#include "Tests/LinuxSyscalls/x32/Syscalls.h"

#include <stddef.h>
#include <stdint.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace FEX::HLE {
  void RegisterTimer() {

    REGISTER_SYSCALL_IMPL(alarm, [](FEXCore::Core::CpuStateFrame *Frame, unsigned int seconds) -> uint64_t {
      uint64_t Result = ::alarm(seconds);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(timer_create, [](FEXCore::Core::CpuStateFrame *Frame, clockid_t clockid, struct sigevent *sevp, timer_t *timerid) -> uint64_t {
      uint64_t Result = ::syscall(SYS_timer_create, clockid, sevp, timerid);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(timer_settime, [](FEXCore::Core::CpuStateFrame *Frame, timer_t timerid, int flags, const struct itimerspec *new_value, struct itimerspec *old_value) -> uint64_t {
      uint64_t Result = ::syscall(SYS_timer_settime, timerid, flags, new_value, old_value);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(timer_gettime, [](FEXCore::Core::CpuStateFrame *Frame, timer_t timerid, struct itimerspec *curr_value) -> uint64_t {
      uint64_t Result = ::syscall(SYS_timer_gettime, timerid, curr_value);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(timer_getoverrun, [](FEXCore::Core::CpuStateFrame *Frame, timer_t timerid) -> uint64_t {
      uint64_t Result = ::syscall(SYS_timer_getoverrun, timerid);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(timer_delete, [](FEXCore::Core::CpuStateFrame *Frame, timer_t timerid) -> uint64_t {
      uint64_t Result = ::syscall(SYS_timer_delete, timerid);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(getitimer, [](FEXCore::Core::CpuStateFrame *Frame, int which, struct itimerval *curr_value) -> uint64_t {
      uint64_t Result = ::getitimer(which, curr_value);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(setitimer, [](FEXCore::Core::CpuStateFrame *Frame, int which, const struct itimerval *new_value, struct itimerval *old_value) -> uint64_t {
      uint64_t Result = ::setitimer(which, new_value, old_value);
      SYSCALL_ERRNO();
    });
  }
}
