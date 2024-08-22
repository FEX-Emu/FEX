// SPDX-License-Identifier: MIT
/*
$info$
tags: LinuxSyscalls|syscalls-shared
$end_info$
*/

#include "LinuxSyscalls/SignalDelegator.h"
#include "LinuxSyscalls/Syscalls.h"
#include "LinuxSyscalls/Syscalls/Thread.h"
#include "LinuxSyscalls/x64/Syscalls.h"
#include "LinuxSyscalls/x32/Syscalls.h"

#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/Core/SignalDelegator.h>

#include <signal.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace SignalDelegator {
struct GuestSigAction;
}

namespace FEX::HLE {
void RegisterSignals(FEX::HLE::SyscallHandler* Handler) {
  REGISTER_SYSCALL_IMPL(rt_sigprocmask, [](FEXCore::Core::CpuStateFrame* Frame, int how, const uint64_t* set, uint64_t* oldset) -> uint64_t {
    return FEX::HLE::_SyscallHandler->GetSignalDelegator()->GuestSigProcMask(FEX::HLE::ThreadManager::GetStateObjectFromCPUState(Frame),
                                                                             how, set, oldset);
  });

  REGISTER_SYSCALL_IMPL(rt_sigpending, [](FEXCore::Core::CpuStateFrame* Frame, uint64_t* set, size_t sigsetsize) -> uint64_t {
    return FEX::HLE::_SyscallHandler->GetSignalDelegator()->GuestSigPending(FEX::HLE::ThreadManager::GetStateObjectFromCPUState(Frame), set,
                                                                            sigsetsize);
  });

  REGISTER_SYSCALL_IMPL(rt_sigsuspend, [](FEXCore::Core::CpuStateFrame* Frame, uint64_t* unewset, size_t sigsetsize) -> uint64_t {
    return FEX::HLE::_SyscallHandler->GetSignalDelegator()->GuestSigSuspend(FEX::HLE::ThreadManager::GetStateObjectFromCPUState(Frame),
                                                                            unewset, sigsetsize);
  });

  REGISTER_SYSCALL_IMPL(userfaultfd, [](FEXCore::Core::CpuStateFrame* Frame, int flags) -> uint64_t {
    // Disable userfaultfd until we can properly emulate it
    // This is okay because the kernel configuration allows you to disable it at compile time
    return -ENOSYS;
    uint64_t Result = ::syscall(SYSCALL_DEF(userfaultfd), flags);
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL(signalfd, [](FEXCore::Core::CpuStateFrame* Frame, int fd, const uint64_t* mask, size_t sigsetsize) -> uint64_t {
    return FEX::HLE::_SyscallHandler->GetSignalDelegator()->GuestSignalFD(fd, mask, sigsetsize, 0);
  });

  REGISTER_SYSCALL_IMPL(signalfd4, [](FEXCore::Core::CpuStateFrame* Frame, int fd, const uint64_t* mask, size_t sigsetsize, int flags) -> uint64_t {
    return FEX::HLE::_SyscallHandler->GetSignalDelegator()->GuestSignalFD(fd, mask, sigsetsize, flags);
  });
}
} // namespace FEX::HLE
