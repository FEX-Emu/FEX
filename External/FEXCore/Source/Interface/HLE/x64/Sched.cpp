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

namespace FEXCore::HLE::x64 {
  void RegisterSched() {
    REGISTER_SYSCALL_IMPL_X64(sched_rr_get_interval, [](FEXCore::Core::InternalThreadState *Thread, pid_t pid, struct timespec *tp) -> uint64_t {
      uint64_t Result = ::sched_rr_get_interval(pid, tp);
      SYSCALL_ERRNO();
    });
  }
}
