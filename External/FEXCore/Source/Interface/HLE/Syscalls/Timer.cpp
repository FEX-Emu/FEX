#include "Interface/HLE/Syscalls.h"
#include "Interface/HLE/x64/Syscalls.h"

#include <stddef.h>
#include <stdint.h>
#include <signal.h>
#include <time.h>

namespace FEXCore::Core {
struct InternalThreadState;
}

namespace FEXCore::HLE {

  void RegisterTimer() {
    REGISTER_SYSCALL_IMPL(timer_create, [](FEXCore::Core::InternalThreadState *Thread, clockid_t clockid, struct sigevent *sevp, timer_t *timerid) -> uint64_t {
      uint64_t Result = ::timer_create(clockid, sevp, timerid);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(timer_settime, [](FEXCore::Core::InternalThreadState *Thread, timer_t timerid, int flags, const struct itimerspec *new_value, struct itimerspec *old_value) -> uint64_t {
      uint64_t Result = ::timer_settime(timerid, flags, new_value, old_value);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(timer_gettime, [](FEXCore::Core::InternalThreadState *Thread, timer_t timerid, struct itimerspec *curr_value) -> uint64_t {
      uint64_t Result = ::timer_gettime(timerid, curr_value);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(timer_getoverrun, [](FEXCore::Core::InternalThreadState *Thread, timer_t timerid) -> uint64_t {
      uint64_t Result = ::timer_getoverrun(timerid);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(timer_delete, [](FEXCore::Core::InternalThreadState *Thread, timer_t timerid) -> uint64_t {
      uint64_t Result = ::timer_delete(timerid);
      SYSCALL_ERRNO();
    });
  }
}
