/*
$info$
tags: LinuxSyscalls|syscalls-x86-32
$end_info$
*/

#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/Syscalls/Thread.h"
#include "Tests/LinuxSyscalls/x32/Syscalls.h"

#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/Core/SignalDelegator.h>

#include <signal.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace SignalDelegator {
  struct GuestSigAction;
}

namespace FEX::HLE::x32 {
  void RegisterSignals() {
    REGISTER_SYSCALL_IMPL_X32(signal, [](FEXCore::Core::CpuStateFrame *Frame, int signum, uint32_t handler) -> uint64_t {
      FEXCore::GuestSigAction newact{};
      FEXCore::GuestSigAction oldact{};
      newact.sigaction_handler.handler = reinterpret_cast<decltype(newact.sigaction_handler.handler)>(handler);
      FEX::HLE::_SyscallHandler->GetSignalDelegator()->RegisterGuestSignalHandler(signum, &newact, &oldact);
      return static_cast<uint32_t>(reinterpret_cast<uint64_t>(oldact.sigaction_handler.handler));
    });

    REGISTER_SYSCALL_IMPL_X32(rt_sigaction, [](FEXCore::Core::CpuStateFrame *Frame, int signum, const GuestSigAction_32 *act, GuestSigAction_32 *oldact, size_t sigsetsize) -> uint64_t {
      if (sigsetsize != 8) {
        return -EINVAL;
      }

      FEXCore::GuestSigAction *act64_p{};
      FEXCore::GuestSigAction *old64_p{};

      FEXCore::GuestSigAction act64{};
      if (act) {
        act64 = *act;
        act64_p = &act64;
      }
      FEXCore::GuestSigAction old64{};

      if (oldact) {
        old64_p = &old64;
      }

      uint64_t Result = FEX::HLE::_SyscallHandler->GetSignalDelegator()->RegisterGuestSignalHandler(signum, act64_p, old64_p);
      if (Result == 0 && oldact) {
        *oldact = old64;
      }

      return Result;
    });

    REGISTER_SYSCALL_IMPL_X32(rt_sigtimedwait, [](FEXCore::Core::CpuStateFrame *Frame, uint64_t *set, siginfo_t *info, const struct timespec32* timeout, size_t sigsetsize) -> uint64_t {
      struct timespec* timeout_ptr{};
      struct timespec tp64{};
      if (timeout) {
        tp64 = *timeout;
        timeout_ptr = &tp64;
      }

      return FEX::HLE::_SyscallHandler->GetSignalDelegator()->GuestSigTimedWait(set, info, timeout_ptr, sigsetsize);
    });

  }
}
