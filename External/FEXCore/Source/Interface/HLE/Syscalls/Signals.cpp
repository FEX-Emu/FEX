#include "Interface/Context/Context.h"
#include "Interface/Core/InternalThreadState.h"
#include "Interface/HLE/Syscalls.h"

#include <FEXCore/Core/X86Enums.h>

#include <signal.h>

namespace FEXCore::HLE {
  uint64_t RT_Sigaction(FEXCore::Core::InternalThreadState *Thread, int signum, const struct sigaction *act, struct sigaction *oldact) {
    uint64_t Result = ::sigaction(signum, act, oldact);
    SYSCALL_ERRNO();
  }

  uint64_t RT_Sigprocmask(FEXCore::Core::InternalThreadState *Thread, int how, const sigset_t *set, sigset_t *oldset) {
    uint64_t Result = ::sigprocmask(how, set, oldset);
    SYSCALL_ERRNO();
  }

  uint64_t Sigaltstack(FEXCore::Core::InternalThreadState *Thread, const stack_t *ss, stack_t *old_ss) {
    uint64_t Result = ::sigaltstack(ss, old_ss);
    SYSCALL_ERRNO();
  }
}
