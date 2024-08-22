// SPDX-License-Identifier: MIT
/*
$info$
tags: LinuxSyscalls|syscalls-x86-32
$end_info$
*/

#include "ArchHelpers/UContext.h"
#include "LinuxSyscalls/SignalDelegator.h"
#include "LinuxSyscalls/Syscalls.h"
#include "LinuxSyscalls/x64/Syscalls.h"
#include "LinuxSyscalls/x32/Syscalls.h"
#include "LinuxSyscalls/x32/Types.h"

#include <FEXCore/Core/SignalDelegator.h>
#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <time.h>

namespace FEXCore::Core {
struct CpuStateFrame;
}

ARG_TO_STR(FEX::HLE::x32::compat_ptr<FEXCore::x86::siginfo_t>, "%lx")

namespace FEX::HLE::x32 {
void CopySigInfo(FEXCore::x86::siginfo_t* Info, const siginfo_t& Host) {
  // Copy the basic things first
  Info->si_signo = Host.si_signo;
  Info->si_errno = Host.si_errno;
  Info->si_code = Host.si_code;

  // Check si_code to determine how we need to interpret this
  if (Info->si_code == SI_TIMER) {
    // SI_TIMER means pid, uid, value
    Info->_sifields._timer.tid = Host.si_timerid;
    Info->_sifields._timer.overrun = Host.si_overrun;
    Info->_sifields._timer.sigval.sival_int = Host.si_value.sival_int;
  } else {
    // Now we need to copy over the more complex things
    switch (Info->si_signo) {
    case SIGSEGV:
    case SIGBUS:
      // This is the address trying to be accessed, not the RIP
      Info->_sifields._sigfault.addr = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(Host.si_addr));
      break;
    case SIGFPE:
    case SIGILL:
      // Can't really give a real result here. This is the RIP causing a sigill or sigfpe
      // Claim at RIP 0 for now
      Info->_sifields._sigfault.addr = 0;
      break;
    case SIGCHLD:
      Info->_sifields._sigchld.pid = Host.si_pid;
      Info->_sifields._sigchld.uid = Host.si_uid;
      Info->_sifields._sigchld.status = Host.si_status;
      Info->_sifields._sigchld.utime = Host.si_utime;
      Info->_sifields._sigchld.stime = Host.si_stime;
      break;
    case SIGALRM:
    case SIGVTALRM:
      Info->_sifields._timer.tid = Host.si_timerid;
      Info->_sifields._timer.overrun = Host.si_overrun;
      Info->_sifields._timer.sigval.sival_int = Host.si_int;
      break;
    default: LogMan::Msg::EFmt("Unhandled siginfo_t for sigtimedwait: {}", Info->si_signo); break;
    }
  }
}

void RegisterSignals(FEX::HLE::SyscallHandler* Handler) {

  // Only gets the lower 32-bits of the signal mask
  REGISTER_SYSCALL_IMPL_X32(sgetmask, [](FEXCore::Core::CpuStateFrame* Frame) -> uint64_t {
    uint64_t Set {};
    FEX::HLE::_SyscallHandler->GetSignalDelegator()->GuestSigProcMask(FEX::HLE::ThreadManager::GetStateObjectFromCPUState(Frame), 0, nullptr, &Set);
    return Set & ~0U;
  });

  // Only controls the lower 32-bits of the signal mask
  // Blocks the upper 32-bits
  REGISTER_SYSCALL_IMPL_X32(ssetmask, [](FEXCore::Core::CpuStateFrame* Frame, uint32_t New) -> uint64_t {
    uint64_t Set {};
    uint64_t NewSet = (~0ULL << 32) | New;
    FEX::HLE::_SyscallHandler->GetSignalDelegator()->GuestSigProcMask(FEX::HLE::ThreadManager::GetStateObjectFromCPUState(Frame),
                                                                      SIG_SETMASK, &NewSet, &Set);
    return Set & ~0U;
  });

  // Only masks the lower 32-bits of the signal mask
  // The upper 32-bits are still active (unmasked) and can signal the program
  REGISTER_SYSCALL_IMPL_X32(sigsuspend, [](FEXCore::Core::CpuStateFrame* Frame, uint32_t Mask) -> uint64_t {
    uint64_t Mask64 = Mask;
    return FEX::HLE::_SyscallHandler->GetSignalDelegator()->GuestSigSuspend(FEX::HLE::ThreadManager::GetStateObjectFromCPUState(Frame), &Mask64, 8);
  });

  REGISTER_SYSCALL_IMPL_X32(sigpending, [](FEXCore::Core::CpuStateFrame* Frame, compat_old_sigset_t* set) -> uint64_t {
    uint64_t HostSet {};
    uint64_t Result =
      FEX::HLE::_SyscallHandler->GetSignalDelegator()->GuestSigPending(FEX::HLE::ThreadManager::GetStateObjectFromCPUState(Frame), &HostSet, 8);
    if (Result == 0) {
      // This old interface only returns the lower signals
      FaultSafeUserMemAccess::VerifyIsWritable(set, sizeof(*set));
      *set = HostSet & ~0U;
    }
    return Result;
  });

  REGISTER_SYSCALL_IMPL_X32(signal, [](FEXCore::Core::CpuStateFrame* Frame, int signum, uint32_t handler) -> uint64_t {
    GuestSigAction newact {};
    GuestSigAction oldact {};
    newact.sigaction_handler.handler = reinterpret_cast<decltype(newact.sigaction_handler.handler)>(handler);
    FEX::HLE::_SyscallHandler->GetSignalDelegator()->RegisterGuestSignalHandler(signum, &newact, &oldact);
    return static_cast<uint32_t>(reinterpret_cast<uint64_t>(oldact.sigaction_handler.handler));
  });

  REGISTER_SYSCALL_IMPL_X32(
    sigaction, [](FEXCore::Core::CpuStateFrame* Frame, int signum, const OldGuestSigAction_32* act, OldGuestSigAction_32* oldact) -> uint64_t {
      GuestSigAction* act64_p {};
      GuestSigAction* old64_p {};

      GuestSigAction act64 {};
      if (act) {
        FaultSafeUserMemAccess::VerifyIsReadable(act, sizeof(*act));
        act64 = *act;
        act64_p = &act64;
      }
      GuestSigAction old64 {};

      if (oldact) {
        old64_p = &old64;
      }

      uint64_t Result = FEX::HLE::_SyscallHandler->GetSignalDelegator()->RegisterGuestSignalHandler(signum, act64_p, old64_p);
      if (Result == 0 && oldact) {
        FaultSafeUserMemAccess::VerifyIsWritable(oldact, sizeof(*oldact));
        *oldact = old64;
      }

      return Result;
    });

  REGISTER_SYSCALL_IMPL_X32(
    rt_sigaction,
    [](FEXCore::Core::CpuStateFrame* Frame, int signum, const GuestSigAction_32* act, GuestSigAction_32* oldact, size_t sigsetsize) -> uint64_t {
      if (sigsetsize != 8) {
        return -EINVAL;
      }

      GuestSigAction* act64_p {};
      GuestSigAction* old64_p {};

      GuestSigAction act64 {};
      if (act) {
        FaultSafeUserMemAccess::VerifyIsReadable(act, sizeof(*act));
        act64 = *act;
        act64_p = &act64;
      }
      GuestSigAction old64 {};

      if (oldact) {
        old64_p = &old64;
      }

      uint64_t Result = FEX::HLE::_SyscallHandler->GetSignalDelegator()->RegisterGuestSignalHandler(signum, act64_p, old64_p);
      if (Result == 0 && oldact) {
        FaultSafeUserMemAccess::VerifyIsWritable(oldact, sizeof(*oldact));
        *oldact = old64;
      }

      return Result;
    });

  REGISTER_SYSCALL_IMPL_X32(rt_sigtimedwait,
                            [](FEXCore::Core::CpuStateFrame* Frame, uint64_t* set, compat_ptr<FEXCore::x86::siginfo_t> info,
                               const struct timespec32* timeout, size_t sigsetsize) -> uint64_t {
                              struct timespec* timeout_ptr {};
                              struct timespec tp64 {};
                              if (timeout) {
                                FaultSafeUserMemAccess::VerifyIsReadable(timeout, sizeof(*timeout));
                                tp64 = *timeout;
                                timeout_ptr = &tp64;
                              }

                              siginfo_t HostInfo {};
                              uint64_t Result =
                                FEX::HLE::_SyscallHandler->GetSignalDelegator()->GuestSigTimedWait(set, &HostInfo, timeout_ptr, sigsetsize);
                              if (Result != -1) {
                                FaultSafeUserMemAccess::VerifyIsWritable(info, sizeof(*info));
                                // We need to translate the 64-bit siginfo_t to 32-bit siginfo_t
                                CopySigInfo(info, HostInfo);
                              }
                              return Result;
                            });


  REGISTER_SYSCALL_IMPL_X32(rt_sigtimedwait_time64,
                            [](FEXCore::Core::CpuStateFrame* Frame, uint64_t* set, compat_ptr<FEXCore::x86::siginfo_t> info,
                               const struct timespec* timeout, size_t sigsetsize) -> uint64_t {
                              siginfo_t HostInfo {};
                              uint64_t Result =
                                FEX::HLE::_SyscallHandler->GetSignalDelegator()->GuestSigTimedWait(set, &HostInfo, timeout, sigsetsize);
                              if (Result != -1) {
                                FaultSafeUserMemAccess::VerifyIsWritable(info, sizeof(*info));
                                // We need to translate the 64-bit siginfo_t to 32-bit siginfo_t
                                CopySigInfo(info, HostInfo);
                              }
                              return Result;
                            });

  if (Handler->IsHostKernelVersionAtLeast(5, 1, 0)) {
    REGISTER_SYSCALL_IMPL_X32(
      pidfd_send_signal,
      [](FEXCore::Core::CpuStateFrame* Frame, int pidfd, int sig, compat_ptr<FEXCore::x86::siginfo_t> info, unsigned int flags) -> uint64_t {
        siginfo_t* InfoHost_ptr {};
        siginfo_t InfoHost {};
        if (info) {
          FaultSafeUserMemAccess::VerifyIsReadable(info, sizeof(*info));
          InfoHost = *info;
          InfoHost_ptr = &InfoHost;
        }

        uint64_t Result = ::syscall(SYSCALL_DEF(pidfd_send_signal), pidfd, sig, InfoHost_ptr, flags);
        SYSCALL_ERRNO();
      });
  } else {
    REGISTER_SYSCALL_IMPL_X32(pidfd_send_signal, UnimplementedSyscallSafe);
  }

  REGISTER_SYSCALL_IMPL_X32(
    rt_sigqueueinfo, [](FEXCore::Core::CpuStateFrame* Frame, pid_t pid, int sig, compat_ptr<FEXCore::x86::siginfo_t> info) -> uint64_t {
      siginfo_t info64 {};
      siginfo_t* info64_p {};

      if (info) {
        FaultSafeUserMemAccess::VerifyIsReadable(info, sizeof(*info));
        info64 = *info;
        info64_p = &info64;
      }

      uint64_t Result = ::syscall(SYSCALL_DEF(rt_sigqueueinfo), pid, sig, info64_p);
      SYSCALL_ERRNO();
    });

  REGISTER_SYSCALL_IMPL_X32(
    rt_tgsigqueueinfo, [](FEXCore::Core::CpuStateFrame* Frame, pid_t tgid, pid_t tid, int sig, compat_ptr<FEXCore::x86::siginfo_t> info) -> uint64_t {
      siginfo_t info64 {};
      siginfo_t* info64_p {};

      if (info) {
        FaultSafeUserMemAccess::VerifyIsReadable(info, sizeof(*info));
        info64 = *info;
        info64_p = &info64;
      }

      uint64_t Result = ::syscall(SYSCALL_DEF(rt_tgsigqueueinfo), tgid, tid, sig, info64_p);
      SYSCALL_ERRNO();
    });
}
} // namespace FEX::HLE::x32
