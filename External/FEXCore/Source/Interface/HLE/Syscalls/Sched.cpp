#include "Interface/HLE/Syscalls.h"

#include <stdint.h>
#include <sched.h>
#include <sys/time.h>
#include <sys/resource.h>

namespace FEXCore::Core {
struct InternalThreadState;
}

namespace FEXCore::HLE {
  uint64_t Sched_Yield(FEXCore::Core::InternalThreadState *Thread) {
    uint64_t Result = ::sched_yield();
    SYSCALL_ERRNO();
  }

  uint64_t Getpriority(FEXCore::Core::InternalThreadState *Thread, int which, int who) {
    uint64_t Result = ::getpriority(which, who);
    SYSCALL_ERRNO();
  }

  uint64_t Setpriority(FEXCore::Core::InternalThreadState *Thread, int which, int who, int prio) {
    uint64_t Result = ::setpriority(which, who, prio);
    SYSCALL_ERRNO();
  }

  uint64_t Sched_Setparam(FEXCore::Core::InternalThreadState *Thread, pid_t pid, const struct sched_param *param) {
    uint64_t Result = ::sched_setparam(pid, param);
    SYSCALL_ERRNO();
  }

  uint64_t Sched_Getparam(FEXCore::Core::InternalThreadState *Thread, pid_t pid, struct sched_param *param) {
    uint64_t Result = ::sched_getparam(pid, param);
    SYSCALL_ERRNO();
  }

  uint64_t Sched_Setscheduler(FEXCore::Core::InternalThreadState *Thread, pid_t pid, int policy, const struct sched_param *param) {
    uint64_t Result = ::sched_setscheduler(pid, policy, param);
    SYSCALL_ERRNO();
  }

  uint64_t Sched_Getscheduler(FEXCore::Core::InternalThreadState *Thread, pid_t pid) {
    uint64_t Result = ::sched_getscheduler(pid);
    SYSCALL_ERRNO();
  }

  uint64_t Sched_Get_priority_max(FEXCore::Core::InternalThreadState *Thread, int policy) {
    uint64_t Result = ::sched_get_priority_max(policy);
    SYSCALL_ERRNO();
  }

  uint64_t Sched_Get_priority_min(FEXCore::Core::InternalThreadState *Thread, int policy) {
    uint64_t Result = ::sched_get_priority_min(policy);
    SYSCALL_ERRNO();
  }

  uint64_t Sched_rr_get_interval(FEXCore::Core::InternalThreadState *Thread, pid_t pid, struct timespec *tp) {
    uint64_t Result = ::sched_rr_get_interval(pid, tp);
    SYSCALL_ERRNO();
  }

  uint64_t Sched_Setaffinity(FEXCore::Core::InternalThreadState *Thread, pid_t pid, size_t cpusetsize, const unsigned long *mask) {
    return 0;
  }

  uint64_t Sched_Getaffinity(FEXCore::Core::InternalThreadState *Thread, pid_t pid, size_t cpusetsize, unsigned long *mask) {
    // If we don't have at least one byte in the resulting structure
    // then we need to return -EINVAL
    if (cpusetsize < 1) {
      return -EINVAL;
    }
    // Claim 1 CPU core
    mask[0] |= 1;
    // Returns the number of bytes written in to mask
    return 1;
  }
}
