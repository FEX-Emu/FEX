#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/Syscalls/Thread.h"
#include "Tests/LinuxSyscalls/x64/Syscalls.h"
#include "Tests/LinuxSyscalls/x32/Syscalls.h"

#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/Core/SignalDelegator.h>

#include <signal.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace SignalDelegator {
  struct GuestSigAction;
}

namespace FEX::HLE {
  void RegisterSignals() {
    REGISTER_SYSCALL_IMPL(rt_sigprocmask, [](FEXCore::Core::InternalThreadState *Thread, int how, const uint64_t *set, uint64_t *oldset) -> uint64_t {
      return FEX::HLE::_SyscallHandler->GetSignalDelegator()->GuestSigProcMask(how, set, oldset);
    });

    REGISTER_SYSCALL_IMPL(rt_sigpending, [](FEXCore::Core::InternalThreadState *Thread, uint64_t *set, size_t sigsetsize) -> uint64_t {
      return FEX::HLE::_SyscallHandler->GetSignalDelegator()->GuestSigPending(set, sigsetsize);
    });

    REGISTER_SYSCALL_IMPL(rt_sigsuspend, [](FEXCore::Core::InternalThreadState *Thread, uint64_t *unewset, size_t sigsetsize) -> uint64_t {
      return FEX::HLE::_SyscallHandler->GetSignalDelegator()->GuestSigSuspend(unewset, sigsetsize);
    });

    REGISTER_SYSCALL_IMPL(userfaultfd, [](FEXCore::Core::InternalThreadState *Thread, int flags) -> uint64_t {
      uint64_t Result = ::syscall(SYS_userfaultfd, flags);
      SYSCALL_ERRNO();
    });
  }
}
