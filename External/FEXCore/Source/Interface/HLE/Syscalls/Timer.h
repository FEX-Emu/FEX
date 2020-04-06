#pragma once
#include <stddef.h>
#include <stdint.h>
#include <signal.h>
#include <time.h>

namespace FEXCore::Core {
struct InternalThreadState;
}

namespace FEXCore::HLE {
  uint64_t Timer_Create(FEXCore::Core::InternalThreadState *Thread, clockid_t clockid, struct sigevent *sevp, timer_t *timerid);
  uint64_t Timer_Settime(FEXCore::Core::InternalThreadState *Thread, timer_t timerid, int flags, const struct itimerspec *new_value, struct itimerspec *old_value);
  uint64_t Timer_Gettime(FEXCore::Core::InternalThreadState *Thread, timer_t timerid, struct itimerspec *curr_value);
  uint64_t Timer_Getoverrun(FEXCore::Core::InternalThreadState *Thread, timer_t timerid);
  uint64_t Timer_Delete(FEXCore::Core::InternalThreadState *Thread, timer_t timerid);
}
