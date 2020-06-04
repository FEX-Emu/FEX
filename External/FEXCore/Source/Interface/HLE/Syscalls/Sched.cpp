#include "Interface/Context/Context.h"
#include "Interface/HLE/Syscalls.h"

#include <stdint.h>
#include <sched.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <unistd.h>

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
    uint64_t Cores = Thread->CTX->Config.EmulatedCPUCores;
    uint64_t BytesPerCore = Cores >> 3;
    // If we don't have at least one byte in the resulting structure
    // then we need to return -EINVAL
    if (cpusetsize < BytesPerCore) {
      return -EINVAL;
    }

    for (uint64_t i = 0; i < Cores; ++i) {
      mask[Cores / 8] |= (1 << (i % 8));
    }

    // Returns the number of bytes written in to mask
    return BytesPerCore;
  }

  uint64_t Sched_Setattr(FEXCore::Core::InternalThreadState *Thread, pid_t pid, struct sched_attr *attr, unsigned int flags) {
    uint64_t Result = ::syscall(SYS_sched_setattr, pid, attr, flags);
    SYSCALL_ERRNO();
  }

  uint64_t Sched_Getattr(FEXCore::Core::InternalThreadState *Thread, pid_t pid, struct sched_attr *attr, unsigned int size, unsigned int flags) {
    uint64_t Result = ::syscall(SYS_sched_getattr, pid, attr, size, flags);
    SYSCALL_ERRNO();
  }
}
