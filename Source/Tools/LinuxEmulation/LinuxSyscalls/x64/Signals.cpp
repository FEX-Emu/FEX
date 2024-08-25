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
      FaultSafeUserMemAccess::VerifyIsReadableOrNull(act, sizeof(GuestSigAction));
      FaultSafeUserMemAccess::VerifyIsWritableOrNull(oldact, sizeof(GuestSigAction));

      return FEX::HLE::_SyscallHandler->GetSignalDelegator()->RegisterGuestSignalHandler(signum, act, oldact);
    });

  REGISTER_SYSCALL_IMPL_X64(
    rt_sigtimedwait,
    [](FEXCore::Core::CpuStateFrame* Frame, uint64_t* set, siginfo_t* info, const struct timespec* timeout, size_t sigsetsize) -> uint64_t {
      FaultSafeUserMemAccess::VerifyIsReadable(set, sizeof(sigsetsize));
      FaultSafeUserMemAccess::VerifyIsWritableOrNull(info, sizeof(siginfo_t));
      FaultSafeUserMemAccess::VerifyIsReadableOrNull(timeout, sizeof(timespec));
      return FEX::HLE::_SyscallHandler->GetSignalDelegator()->GuestSigTimedWait(set, info, timeout, sigsetsize);
    });
}
} // namespace FEX::HLE::x64
