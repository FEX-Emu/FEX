#include "Interface/Context/Context.h"
#include "Interface/Core/InternalThreadState.h"
#include "Interface/HLE/Syscalls.h"
#include "Interface/HLE/x64/Syscalls.h"

#include <FEXCore/Core/X86Enums.h>

#include <signal.h>

namespace FEXCore::HLE {

  void RegisterSignals() {
    REGISTER_SYSCALL_IMPL(rt_sigaction, [](FEXCore::Core::InternalThreadState *Thread, int signum, const SignalDelegator::GuestSigAction *act, SignalDelegator::GuestSigAction *oldact) -> uint64_t {
      return Thread->CTX->SignalDelegation.RegisterGuestSignalHandler(signum, act, oldact);
    });

    REGISTER_SYSCALL_IMPL(rt_sigprocmask, [](FEXCore::Core::InternalThreadState *Thread, int how, const uint64_t *set, uint64_t *oldset) -> uint64_t {
      return Thread->CTX->SignalDelegation.GuestSigProcMask(how, set, oldset);
    });

    REGISTER_SYSCALL_IMPL(sigaltstack, [](FEXCore::Core::InternalThreadState *Thread, const stack_t *ss, stack_t *old_ss) -> uint64_t {
      return Thread->CTX->SignalDelegation.RegisterGuestSigAltStack(ss, old_ss);
    });
  }
}
