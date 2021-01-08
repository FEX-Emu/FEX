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
    REGISTER_SYSCALL_IMPL_X32(signal, [](FEXCore::Core::InternalThreadState *Thread, int signum, uint32_t handler) -> uint64_t {
      FEXCore::GuestSigAction newact{};
      FEXCore::GuestSigAction oldact{};
      newact.sigaction_handler.handler = reinterpret_cast<decltype(newact.sigaction_handler.handler)>(handler);
      FEX::HLE::_SyscallHandler->GetSignalDelegator()->RegisterGuestSignalHandler(signum, &newact, &oldact);
      return static_cast<uint32_t>(reinterpret_cast<uint64_t>(oldact.sigaction_handler.handler));
    });

    REGISTER_SYSCALL_IMPL_X32(rt_sigaction, [](FEXCore::Core::InternalThreadState *Thread, int signum, const GuestSigAction_32 *act, GuestSigAction_32 *oldact) -> uint64_t {
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
      if (Result != -1 && oldact) {
        *oldact = old64;
      }

      SYSCALL_ERRNO();
    });

  }
}
