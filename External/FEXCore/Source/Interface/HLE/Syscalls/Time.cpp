#include "Interface/HLE/Syscalls.h"
#include "Interface/HLE/x64/Syscalls.h"

#include <stddef.h>
#include <stdint.h>
#include <time.h>
#include <sys/time.h>

namespace FEXCore::Core {
struct InternalThreadState;
}

namespace FEXCore::HLE {
  void RegisterTime() {
    REGISTER_SYSCALL_IMPL(nanosleep, [](FEXCore::Core::InternalThreadState *Thread, const struct timespec *req, struct timespec *rem) -> uint64_t {
      uint64_t Result = ::nanosleep(req, rem);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(gettimeofday, [](FEXCore::Core::InternalThreadState *Thread, struct timeval *tv, struct timezone *tz) -> uint64_t {
      uint64_t Result = ::gettimeofday(tv, tz);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(time, [](FEXCore::Core::InternalThreadState *Thread, time_t *tloc) -> uint64_t {
      uint64_t Result = ::time(tloc);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(clock_gettime, [](FEXCore::Core::InternalThreadState *Thread, clockid_t clk_id, struct timespec *tp) -> uint64_t {
      uint64_t Result = ::clock_gettime(clk_id, tp);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(clock_getres, [](FEXCore::Core::InternalThreadState *Thread, clockid_t clk_id, struct timespec *tp) -> uint64_t {
      uint64_t Result = ::clock_getres(clk_id, tp);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(clock_nanosleep, [](FEXCore::Core::InternalThreadState *Thread, clockid_t clockid, int flags, const struct timespec *request, struct timespec *remain) -> uint64_t {
      uint64_t Result = ::clock_nanosleep(clockid, flags, request, remain);
      SYSCALL_ERRNO();
    });
  }
}
