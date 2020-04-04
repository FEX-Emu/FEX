#pragma once
#include <stdint.h>
#include <sched.h>

namespace FEXCore::Core {
struct InternalThreadState;
}

namespace FEXCore::HLE {
  uint64_t Sched_Yield(FEXCore::Core::InternalThreadState *Thread);
  uint64_t Getpriority(FEXCore::Core::InternalThreadState *Thread, int which, int who);
  uint64_t Setpriority(FEXCore::Core::InternalThreadState *Thread, int which, int who, int prio);
  uint64_t Sched_Setaffinity(FEXCore::Core::InternalThreadState *Thread, pid_t pid, size_t cpusetsize, const cpu_set_t *mask);
  uint64_t Sched_Getaffinity(FEXCore::Core::InternalThreadState *Thread, pid_t pid, size_t cpusetsize, cpu_set_t *mask);
}
