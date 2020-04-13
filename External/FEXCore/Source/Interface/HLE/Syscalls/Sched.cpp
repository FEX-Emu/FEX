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
