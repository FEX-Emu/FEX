#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/Syscalls/Thread.h"
#include "Tests/LinuxSyscalls/x64/Syscalls.h"

#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/Core/SignalDelegator.h>

#include <signal.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace FEX::HLE::x64 {
  void RegisterSignals() {
    REGISTER_SYSCALL_IMPL_X64(rt_sigaction, [](FEXCore::Core::InternalThreadState *Thread, int signum, const FEXCore::GuestSigAction *act, FEXCore::GuestSigAction *oldact) -> uint64_t {
      return FEX::HLE::_SyscallHandler->GetSignalDelegator()->RegisterGuestSignalHandler(signum, act, oldact);
    });
  }
}

