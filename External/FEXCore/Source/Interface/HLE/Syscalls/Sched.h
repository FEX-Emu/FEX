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
  uint64_t Sched_Setparam(FEXCore::Core::InternalThreadState *Thread, pid_t pid, const struct sched_param *param);
  uint64_t Sched_Getparam(FEXCore::Core::InternalThreadState *Thread, pid_t pid, struct sched_param *param);
  uint64_t Sched_Setscheduler(FEXCore::Core::InternalThreadState *Thread, pid_t pid, int policy, const struct sched_param *param);
  uint64_t Sched_Getscheduler(FEXCore::Core::InternalThreadState *Thread, pid_t pid);
  uint64_t Sched_Get_priority_max(FEXCore::Core::InternalThreadState *Thread, int policy);
  uint64_t Sched_Get_priority_min(FEXCore::Core::InternalThreadState *Thread, int policy);
  uint64_t Sched_rr_get_interval(FEXCore::Core::InternalThreadState *Thread, pid_t pid, struct timespec *tp);
  uint64_t Sched_Setaffinity(FEXCore::Core::InternalThreadState *Thread, pid_t pid, size_t cpusetsize, const unsigned long *mask);
  uint64_t Sched_Getaffinity(FEXCore::Core::InternalThreadState *Thread, pid_t pid, size_t cpusetsize, unsigned long *mask);
  uint64_t Sched_Setattr(FEXCore::Core::InternalThreadState *Thread, pid_t pid, struct sched_attr *attr, unsigned int flags);
  uint64_t Sched_Getattr(FEXCore::Core::InternalThreadState *Thread, pid_t pid, struct sched_attr *attr, unsigned int size, unsigned int flags);
}
