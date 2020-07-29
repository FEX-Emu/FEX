#include "Interface/Context/Context.h"
#include "Interface/Core/InternalThreadState.h"
#include "Interface/HLE/Syscalls.h"
#include "Interface/HLE/x64/Syscalls.h"

#include <FEXCore/Core/X86Enums.h>

#include <signal.h>

namespace FEXCore::HLE {

  void RegisterSignals() {
    REGISTER_SYSCALL_IMPL(rt_sigaction, [](FEXCore::Core::InternalThreadState *Thread, int signum, const struct sigaction *act, struct sigaction *oldact) -> uint64_t {
      return Thread->CTX->SignalDelegation.RegisterGuestSignalHandler(signum, act, oldact);
    });

    REGISTER_SYSCALL_IMPL(rt_sigprocmask, [](FEXCore::Core::InternalThreadState *Thread, int how, const sigset_t *set, sigset_t *oldset) -> uint64_t {
      // XXX: Pass through SignalDelegator
      uint64_t Result = ::sigprocmask(how, set, oldset);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(sigaltstack, [](FEXCore::Core::InternalThreadState *Thread, const stack_t *ss, stack_t *old_ss) -> uint64_t {
      return Thread->CTX->SignalDelegation.RegisterGuestSigAltStack(ss, old_ss);
    });
  }
}
