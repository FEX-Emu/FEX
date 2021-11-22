/*
$info$
tags: LinuxSyscalls|syscalls-shared
$end_info$
*/

#include "Tests/LinuxSyscalls/SignalDelegator.h"
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
      uint64_t Result = ::syscall(SYSCALL_DEF(userfaultfd), flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(signalfd, [](FEXCore::Core::CpuStateFrame *Frame, int fd, const uint64_t *mask, size_t sigsetsize) -> uint64_t {
      return FEX::HLE::_SyscallHandler->GetSignalDelegator()->GuestSignalFD(fd, mask, sigsetsize, 0);
    });

    REGISTER_SYSCALL_IMPL(signalfd4, [](FEXCore::Core::CpuStateFrame *Frame, int fd, const uint64_t *mask, size_t sigsetsize, int flags) -> uint64_t {
      return FEX::HLE::_SyscallHandler->GetSignalDelegator()->GuestSignalFD(fd, mask, sigsetsize, flags);
    });

    REGISTER_SYSCALL_IMPL_PASS(rt_sigqueueinfo, [](FEXCore::Core::CpuStateFrame *Frame, pid_t pid, int sig, siginfo_t *info) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(rt_sigqueueinfo), pid, sig, info);
      SYSCALL_ERRNO();
    });

    // XXX: siginfo_t definitely isn't correct for 32-bit
    REGISTER_SYSCALL_IMPL_PASS(rt_tgsigqueueinfo, [](FEXCore::Core::CpuStateFrame *Frame, pid_t tgid, pid_t tid, int sig, siginfo_t *info) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(rt_tgsigqueueinfo), tgid, tid, sig, info);
      SYSCALL_ERRNO();
    });

    if (Handler->IsHostKernelVersionAtLeast(5, 1, 0)) {
      // XXX: siginfo_t definitely isn't correct for 32-bit
      REGISTER_SYSCALL_IMPL_PASS(pidfd_send_signal, [](FEXCore::Core::CpuStateFrame *Frame, int pidfd, int sig, siginfo_t *info, unsigned int flags) -> uint64_t {
        uint64_t Result = ::syscall(SYSCALL_DEF(pidfd_send_signal), pidfd, sig, info, flags);
        SYSCALL_ERRNO();
      });
    }
    else {
      REGISTER_SYSCALL_IMPL(pidfd_send_signal, UnimplementedSyscallSafe);
    }
  }
}
