#include "Interface/HLE/Syscalls.h"
#include "Interface/HLE/x64/Syscalls.h"
#include "Interface/HLE/x32/Syscalls.h"

#include <stddef.h>
#include <stdint.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

namespace FEXCore::Core {
struct InternalThreadState;
}

namespace FEXCore::HLE {

  void RegisterTimer() {

    REGISTER_SYSCALL_IMPL(alarm, [](FEXCore::Core::InternalThreadState *Thread, unsigned int seconds) -> uint64_t {
      uint64_t Result = ::alarm(seconds);
      SYSCALL_ERRNO();
    });

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

    REGISTER_SYSCALL_IMPL(getitimer, [](FEXCore::Core::InternalThreadState *Thread, int which, struct itimerval *curr_value) -> uint64_t {
      uint64_t Result = ::getitimer(which, curr_value);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(setitimer, [](FEXCore::Core::InternalThreadState *Thread, int which, const struct itimerval *new_value, struct itimerval *old_value) -> uint64_t {
      uint64_t Result = ::setitimer(which, new_value, old_value);
      SYSCALL_ERRNO();
    });
  }
}
