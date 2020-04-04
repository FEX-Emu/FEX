#include "Interface/HLE/Syscalls.h"

#include <stddef.h>
#include <stdint.h>
#include <time.h>
#include <sys/time.h>

namespace FEXCore::Core {
struct InternalThreadState;
}

namespace FEXCore::HLE {
  uint64_t Nanosleep(FEXCore::Core::InternalThreadState *Thread, const struct timespec *req, struct timespec *rem) {
    uint64_t Result = ::nanosleep(req, rem);
    SYSCALL_ERRNO();
  }

  uint64_t Gettimeofday(FEXCore::Core::InternalThreadState *Thread, struct timeval *tv, struct timezone *tz) {
    uint64_t Result = ::gettimeofday(tv, tz);
    SYSCALL_ERRNO();
  }

  uint64_t Time(FEXCore::Core::InternalThreadState *Thread, time_t *tloc) {
    uint64_t Result = ::time(tloc);
    SYSCALL_ERRNO();
  }

  uint64_t Clock_gettime(FEXCore::Core::InternalThreadState *Thread, clockid_t clk_id, struct timespec *tp) {
    uint64_t Result = ::clock_gettime(clk_id, tp);
    SYSCALL_ERRNO();
  }

  uint64_t Clock_getres(FEXCore::Core::InternalThreadState *Thread, clockid_t clk_id, struct timespec *tp) {
    uint64_t Result = ::clock_getres(clk_id, tp);
    SYSCALL_ERRNO();
  }
}
