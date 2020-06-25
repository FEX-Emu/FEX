#include "Interface/Context/Context.h"
#include "Interface/HLE/Syscalls.h"
#include "Interface/HLE/x64/Syscalls.h"

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

  void RegisterSched() {
      
    REGISTER_SYSCALL_IMPL(sched_yield, [](FEXCore::Core::InternalThreadState *Thread) -> uint64_t {
      uint64_t Result = ::sched_yield();
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(getpriority, [](FEXCore::Core::InternalThreadState *Thread, int which, int who) -> uint64_t {
      uint64_t Result = ::getpriority(which, who);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(setpriority, [](FEXCore::Core::InternalThreadState *Thread, int which, int who, int prio) -> uint64_t {
      uint64_t Result = ::setpriority(which, who, prio);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(sched_setparam, [](FEXCore::Core::InternalThreadState *Thread, pid_t pid, const struct sched_param *param) -> uint64_t {
      uint64_t Result = ::sched_setparam(pid, param);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(sched_getparam, [](FEXCore::Core::InternalThreadState *Thread, pid_t pid, struct sched_param *param) -> uint64_t {
      uint64_t Result = ::sched_getparam(pid, param);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(sched_setscheduler, [](FEXCore::Core::InternalThreadState *Thread, pid_t pid, int policy, const struct sched_param *param) -> uint64_t {
      uint64_t Result = ::sched_setscheduler(pid, policy, param);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(sched_getscheduler, [](FEXCore::Core::InternalThreadState *Thread, pid_t pid) -> uint64_t {
      uint64_t Result = ::sched_getscheduler(pid);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(sched_get_priority_max, [](FEXCore::Core::InternalThreadState *Thread, int policy) -> uint64_t {
      uint64_t Result = ::sched_get_priority_max(policy);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(sched_get_priority_min, [](FEXCore::Core::InternalThreadState *Thread, int policy) -> uint64_t {
      uint64_t Result = ::sched_get_priority_min(policy);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(sched_rr_get_interval, [](FEXCore::Core::InternalThreadState *Thread, pid_t pid, struct timespec *tp) -> uint64_t {
      uint64_t Result = ::sched_rr_get_interval(pid, tp);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(sched_setaffinity, [](FEXCore::Core::InternalThreadState *Thread, pid_t pid, size_t cpusetsize, const unsigned long *mask) -> uint64_t {
      return 0;
    });

    REGISTER_SYSCALL_IMPL(sched_getaffinity, [](FEXCore::Core::InternalThreadState *Thread, pid_t pid, size_t cpusetsize, unsigned long *mask) -> uint64_t {
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
    });

    REGISTER_SYSCALL_IMPL(sched_setattr, [](FEXCore::Core::InternalThreadState *Thread, pid_t pid, struct sched_attr *attr, unsigned int flags) -> uint64_t {
      uint64_t Result = ::syscall(SYS_sched_setattr, pid, attr, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(sched_getattr, [](FEXCore::Core::InternalThreadState *Thread, pid_t pid, struct sched_attr *attr, unsigned int size, unsigned int flags) -> uint64_t {
      uint64_t Result = ::syscall(SYS_sched_getattr, pid, attr, size, flags);
      SYSCALL_ERRNO();
    });
  }
}
