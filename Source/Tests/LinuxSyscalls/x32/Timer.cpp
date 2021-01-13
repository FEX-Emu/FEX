#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/x32/Syscalls.h"

#include <stddef.h>
#include <stdint.h>
#include <time.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/timex.h>
#include <unistd.h>
#include <utime.h>

namespace FEX::HLE::x32 {
  void RegisterTimer() {
    REGISTER_SYSCALL_IMPL_X32(timer_settime64, [](FEXCore::Core::InternalThreadState *Thread, timer_t timerid, int flags, const struct itimerspec *new_value, struct itimerspec *old_value) -> uint64_t {
      uint64_t Result = ::timer_settime(timerid, flags, new_value, old_value);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(timer_gettime64, [](FEXCore::Core::InternalThreadState *Thread, timer_t timerid, struct itimerspec *curr_value) -> uint64_t {
      uint64_t Result = ::timer_gettime(timerid, curr_value);
      SYSCALL_ERRNO();
    });
  }
}
