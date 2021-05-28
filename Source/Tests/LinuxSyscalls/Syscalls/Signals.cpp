/*
$info$
tags: LinuxSyscalls|syscalls-shared
$end_info$
*/

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
  void RegisterSignals(FEX::HLE::SyscallHandler *const Handler) {
    REGISTER_SYSCALL_IMPL(rt_sigprocmask, [](FEXCore::Core::CpuStateFrame *Frame, int how, const uint64_t *set, uint64_t *oldset) -> uint64_t {
      return FEX::HLE::_SyscallHandler->GetSignalDelegator()->GuestSigProcMask(how, set, oldset);
    });

    REGISTER_SYSCALL_IMPL(rt_sigpending, [](FEXCore::Core::CpuStateFrame *Frame, uint64_t *set, size_t sigsetsize) -> uint64_t {
      return FEX::HLE::_SyscallHandler->GetSignalDelegator()->GuestSigPending(set, sigsetsize);
    });

    REGISTER_SYSCALL_IMPL(rt_sigsuspend, [](FEXCore::Core::CpuStateFrame *Frame, uint64_t *unewset, size_t sigsetsize) -> uint64_t {
      return FEX::HLE::_SyscallHandler->GetSignalDelegator()->GuestSigSuspend(unewset, sigsetsize);
    });

    REGISTER_SYSCALL_IMPL(userfaultfd, [](FEXCore::Core::CpuStateFrame *Frame, int flags) -> uint64_t {
      // Disable userfaultfd until we can properly emulate it
      // This is okay because the kernel configuration allows you to disable it at compile time
      return -ENOSYS;
      uint64_t Result = ::syscall(SYS_userfaultfd, flags);
      SYSCALL_ERRNO();
    });

    if (Handler->GetHostKernelVersion() >= FEX::HLE::SyscallHandler::KernelVersion(5, 1, 0)) {
      REGISTER_SYSCALL_IMPL(pidfd_send_signal, [](FEXCore::Core::CpuStateFrame *Frame, int pidfd, int sig, siginfo_t *info, unsigned int flags) -> uint64_t {
        uint64_t Result = ::syscall(SYS_pidfd_send_signal);
        SYSCALL_ERRNO();
      });
    }
    else {
      REGISTER_SYSCALL_IMPL(pidfd_send_signal, UnimplementedSyscallSafe);
    }
  }
}
