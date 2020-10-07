#include "Interface/Context/Context.h"
#include "Interface/Core/InternalThreadState.h"
#include "Interface/HLE/Syscalls.h"
#include "Interface/HLE/x64/Syscalls.h"
#include "Interface/HLE/x32/Syscalls.h"

#include <FEXCore/Core/X86Enums.h>

#include <signal.h>
#include <sys/syscall.h>
#include <unistd.h>

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

    REGISTER_SYSCALL_IMPL(rt_sigpending, [](FEXCore::Core::InternalThreadState *Thread, uint64_t *set, size_t sigsetsize) -> uint64_t {
      return Thread->CTX->SignalDelegation.GuestSigPending(set, sigsetsize);
    });

    REGISTER_SYSCALL_IMPL(rt_sigsuspend, [](FEXCore::Core::InternalThreadState *Thread, uint64_t *unewset, size_t sigsetsize) -> uint64_t {
      return Thread->CTX->SignalDelegation.GuestSigSuspend(unewset, sigsetsize);
    });

    REGISTER_SYSCALL_IMPL(userfaultfd, [](FEXCore::Core::InternalThreadState *Thread, int flags) -> uint64_t {
      uint64_t Result = ::syscall(SYS_userfaultfd, flags);
      SYSCALL_ERRNO();
    });
  }
}
