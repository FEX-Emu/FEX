#pragma once
#include <stddef.h>
#include <stdint.h>
#include <sys/time.h>
#include <time.h>

namespace FEXCore::Core {
struct InternalThreadState;
}

namespace FEXCore::HLE {
  uint64_t Nanosleep(FEXCore::Core::InternalThreadState *Thread, const struct timespec *req, struct timespec *rem);
  uint64_t Gettimeofday(FEXCore::Core::InternalThreadState *Thread, struct timeval *tv, struct timezone *tz);
  uint64_t Time(FEXCore::Core::InternalThreadState *Thread, time_t *tloc);
  uint64_t Clock_gettime(FEXCore::Core::InternalThreadState *Thread, clockid_t clk_id, struct timespec *tp);
  uint64_t Clock_getres(FEXCore::Core::InternalThreadState *Thread, clockid_t clk_id, struct timespec *tp);
}
