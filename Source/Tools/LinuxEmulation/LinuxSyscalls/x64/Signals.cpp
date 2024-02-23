// SPDX-License-Identifier: MIT
/*
$info$
tags: LinuxSyscalls|syscalls-x86-64
$end_info$
*/

#include "LinuxSyscalls/SignalDelegator.h"
#include "LinuxSyscalls/Syscalls.h"
#include "LinuxSyscalls/Syscalls/Thread.h"

#include "LinuxSyscalls/x64/Syscalls.h"

#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/Core/SignalDelegator.h>

#include <signal.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace FEX::HLE::x64 {
void RegisterSignals(FEX::HLE::SyscallHandler* Handler) {
  REGISTER_SYSCALL_IMPL_X64(
    rt_sigaction, [](FEXCore::Core::CpuStateFrame* Frame, int signum, const GuestSigAction* act, GuestSigAction* oldact, size_t sigsetsize) -> uint64_t {
    if (sigsetsize != 8) {
      return -EINVAL;
    }

    return FEX::HLE::_SyscallHandler->GetSignalDelegator()->RegisterGuestSignalHandler(signum, act, oldact);
  });

  REGISTER_SYSCALL_IMPL_X64(
    rt_sigtimedwait,
    [](FEXCore::Core::CpuStateFrame* Frame, uint64_t* set, siginfo_t* info, const struct timespec* timeout, size_t sigsetsize) -> uint64_t {
    return FEX::HLE::_SyscallHandler->GetSignalDelegator()->GuestSigTimedWait(set, info, timeout, sigsetsize);
  });

  if (Handler->IsHostKernelVersionAtLeast(5, 1, 0)) {
    REGISTER_SYSCALL_IMPL_X64_PASS(
      pidfd_send_signal, [](FEXCore::Core::CpuStateFrame* Frame, int pidfd, int sig, siginfo_t* info, unsigned int flags) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(pidfd_send_signal), pidfd, sig, info, flags);
      SYSCALL_ERRNO();
    });
  } else {
    REGISTER_SYSCALL_IMPL_X64(pidfd_send_signal, UnimplementedSyscallSafe);
  }

  REGISTER_SYSCALL_IMPL_X64_PASS(rt_sigqueueinfo, [](FEXCore::Core::CpuStateFrame* Frame, pid_t pid, int sig, siginfo_t* info) -> uint64_t {
    uint64_t Result = ::syscall(SYSCALL_DEF(rt_sigqueueinfo), pid, sig, info);
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL_X64_PASS(rt_tgsigqueueinfo,
                                 [](FEXCore::Core::CpuStateFrame* Frame, pid_t tgid, pid_t tid, int sig, siginfo_t* info) -> uint64_t {
    uint64_t Result = ::syscall(SYSCALL_DEF(rt_tgsigqueueinfo), tgid, tid, sig, info);
    SYSCALL_ERRNO();
  });
}
} // namespace FEX::HLE::x64
