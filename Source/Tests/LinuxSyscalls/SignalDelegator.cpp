/*
$info$
tags: LinuxSyscalls|common
desc: Handles host -> host and host -> guest signal routing, emulates procmask & co
$end_info$
*/

#include <FEXCore/Core/Context.h>
#include <FEXCore/Debug/InternalThreadState.h>
#include "Tests/LinuxSyscalls/SignalDelegator.h"

#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/Utils/Allocator.h>
#include <FEXCore/Utils/LogManager.h>

#include <string.h>

#include <linux/futex.h>
#include <bits/types/stack_t.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/signalfd.h>
#include <unistd.h>

namespace FEX::HLE {
#ifdef _M_X86_64
  __attribute__((naked))
  static void sigrestore() {
    __asm volatile("syscall;"
        :: "a" (0xF)
        : "memory");
  }
#endif

  constexpr static uint32_t SS_AUTODISARM = (1U << 31);
  constexpr static uint32_t X86_MINSIGSTKSZ  = 0x2000U;

  // We can only have one delegator per process
  static SignalDelegator *GlobalDelegator{};

  struct ThreadState {
    void *AltStackPtr{};
    stack_t GuestAltStack {
      .ss_sp = nullptr,
      .ss_flags = SS_DISABLE, // By default the guest alt stack is disabled
      .ss_size = 0,
    };
    // Guest signal sa_mask is per thread!
    // This is the sa_mask from sigaction which is orr'd to the current signal mask
    FEXCore::GuestSAMask Guest_sa_mask[SignalDelegator::MAX_SIGNALS]{};
    // This is the thread's current signal mask
    FEXCore::GuestSAMask CurrentSignalMask{};
    // The mask prior to a suspend
    FEXCore::GuestSAMask PreviousSuspendMask{};

    uint64_t PendingSignals{};
    bool Suspended {false};
  };

  thread_local ThreadState ThreadData{};

  static void SignalHandlerThunk(int Signal, siginfo_t *Info, void *UContext) {
    GlobalDelegator->HandleSignal(Signal, Info, UContext);
  }

  uint64_t SigIsMember(FEXCore::GuestSAMask *Set, int Signal) {
    // Signal 0 isn't real, so everything is offset by one inside the set
    Signal -= 1;
    return (Set->Val >> Signal) & 1;
  }

  uint64_t SetSignal(FEXCore::GuestSAMask *Set, int Signal) {
    // Signal 0 isn't real, so everything is offset by one inside the set
    Signal -= 1;
    return Set->Val | (1ULL << Signal);
  }

  void SignalDelegator::HandleGuestSignal(FEXCore::Core::InternalThreadState *Thread, int Signal, void *Info, void *UContext) {
    // Let the host take first stab at handling the signal
    siginfo_t *SigInfo = static_cast<siginfo_t*>(Info);
    SignalHandler &Handler = HostHandlers[Signal];

    if (Signal == SIGCHLD) {
      bool StopOrResume = SigInfo->si_code == CLD_STOPPED || SigInfo->si_code == CLD_CONTINUED || SigInfo->si_code == CLD_TRAPPED;

      // Do some special handling around this signal
      // If the guest has a signal handler installed with SA_NOCLDSTOP or SA_NOCHLDWAIT then
      // handle carefully
      if (Handler.GuestAction.sa_flags & SA_NOCLDSTOP &&
          StopOrResume) {
        // SA_NOCLDSTOP blocks SIGCHLD when si_code is CLD_STOPPED/CLD_CONTINUED/CLD_TRAPPED
        // in that case, drop the signal
        return;
      }

      if (Handler.GuestAction.sa_flags & SA_NOCLDWAIT) {
        // Linux will still generate a signal for this
        // POSIX leaves it unspecific
        // "do not transform children in to zombies when they terminate"
        // XXX: Handle this
      }
    }

    // Check the thread's current signal mask
    if (SigIsMember(&ThreadData.CurrentSignalMask, Signal) != ThreadData.Suspended) {
      ThreadData.PendingSignals |= 1ULL << (Signal - 1);
      return;
    }

    if (ThreadData.Suspended) {
      // If we were suspended then swap the mask back to the original
      ThreadData.CurrentSignalMask = ThreadData.PreviousSuspendMask;
      ThreadData.PreviousSuspendMask.Val = 0;
      ThreadData.Suspended = false;
    }

    // OR in the sa_mask
    ThreadData.CurrentSignalMask.Val |= ThreadData.Guest_sa_mask[Signal].Val;

    // If NODEFER isn't set then also mask the current signal
    if (!(Handler.GuestAction.sa_flags & SA_NODEFER)) {
      SetSignal(&ThreadData.CurrentSignalMask, Signal);
    }

    // Remove the pending signal
    ThreadData.PendingSignals &= ~(1ULL << (Signal - 1));

    // We have an emulation thread pointer, we can now modify its state
    if (Handler.GuestAction.sigaction_handler.handler == SIG_DFL) {
      if (Handler.DefaultBehaviour == DEFAULT_TERM) {
        if (Thread->ThreadManager.clear_child_tid) {
          std::atomic<uint32_t> *Addr = reinterpret_cast<std::atomic<uint32_t>*>(Thread->ThreadManager.clear_child_tid);
          Addr->store(0);
          syscall(SYS_futex,
              Thread->ThreadManager.clear_child_tid,
              FUTEX_WAKE,
              ~0ULL,
              0,
              0,
              0);
        }

        Thread->StatusCode = -Signal;

        // Doesn't return
        FEXCore::Context::StopThread(Thread->CTX, Thread);
        std::terminate();
      }
    }
    else if (Handler.GuestAction.sigaction_handler.handler == SIG_IGN) {
      return;
    }
    else {
      if (Handler.GuestHandler &&
          Handler.GuestHandler(Thread, Signal, Info, UContext, &Handler.GuestAction, &ThreadData.GuestAltStack)) {
        return;
      }
      ERROR_AND_DIE("Unhandled guest exception");
    }

    // Unhandled crash
    // Call back in to the previous handler
    if (Handler.OldAction.sa_flags & SA_SIGINFO) {
      Handler.OldAction.sigaction(Signal, static_cast<siginfo_t*>(Info), UContext);
    }
    else if (Handler.OldAction.handler == SIG_IGN ||
      (Handler.OldAction.handler == SIG_DFL &&
       Handler.DefaultBehaviour == DEFAULT_IGNORE)) {
      // Do nothing
    }
    else if (Handler.OldAction.handler == SIG_DFL &&
      (Handler.DefaultBehaviour == DEFAULT_COREDUMP ||
       Handler.DefaultBehaviour == DEFAULT_TERM)) {
      // Reassign back to DFL and crash
      signal(Signal, SIG_DFL);
    }
    else {
      Handler.OldAction.handler(Signal);
    }
  }

  bool SignalDelegator::InstallHostThunk(int Signal) {
    SignalHandler &SignalHandler = HostHandlers[Signal];
    // If the host thunk is already installed for this, just return
    if (SignalHandler.Installed) {
      return false;
    }

    // Default flags for us
    SignalHandler.HostAction.sa_flags = SA_SIGINFO | SA_ONSTACK;

    bool Result = UpdateHostThunk(Signal);

    SignalHandler.Installed = Result;
    return Result;
  }

  bool SignalDelegator::UpdateHostThunk(int Signal) {
    SignalHandler &SignalHandler = HostHandlers[Signal];

    // Now install the thunk handler
    SignalHandler.HostAction.sigaction = SignalHandlerThunk;

    if (SignalHandler.GuestAction.sa_flags & SA_NODEFER) {
      // If the guest is using NODEFER then make sure to set it for the host as well
      SignalHandler.HostAction.sa_flags |= SA_NODEFER;
    }

    if ((SignalHandler.HostAction.sa_flags ^ SignalHandler.GuestAction.sa_flags) & SA_RESTART) {
      // If the guest is using SA_RESTART then make sure to set it for the host as well
      SignalHandler.HostAction.sa_flags &= ~SA_RESTART;
      SignalHandler.HostAction.sa_flags |= SignalHandler.GuestAction.sa_flags & SA_RESTART;
    }

#ifdef _M_X86_64
#define SA_RESTORER 0x04000000
    SignalHandler.HostAction.sa_flags |= SA_RESTORER;
    SignalHandler.HostAction.restorer = sigrestore;
#endif

    // Walk the signals we have that are required and make sure to remove it from the mask
    // This'll likely be SIGILL, SIGBUS, SIG63

    // If the guest has masked some signals then we need to also mask those signals
    for (size_t i = 1; i < HostHandlers.size(); ++i) {
      if (HostHandlers[i].Required.load(std::memory_order_relaxed)) {
        SignalHandler.HostAction.sa_mask &= ~(1ULL << (i - 1));
      }
      else if (SigIsMember(&SignalHandler.GuestAction.sa_mask, i)) {
        SignalHandler.HostAction.sa_mask |= (1ULL << (i - 1));
      }
    }

    // Only update the old action if we haven't ever been installed
    int Result = ::syscall(SYS_rt_sigaction, Signal, &SignalHandler.HostAction, SignalHandler.Installed ? nullptr : &SignalHandler.OldAction, 8);
    if (Result < 0) {
      // Signal 32 and 33 are consumed by glibc. We don't handle this atm
      LogMan::Msg::A("Failed to install host signal thunk for signal %d: %s", Signal, strerror(errno));
      return false;
    }

    return true;
  }

  SignalDelegator::SignalDelegator() {
    // Register this delegate
    LOGMAN_THROW_A(!GlobalDelegator, "Can't register global delegator multiple times!");
    GlobalDelegator = this;
    // Signal zero isn't real
    HostHandlers[0].Installed = true;

    // We can't capture SIGKILL or SIGSTOP
    HostHandlers[SIGKILL].Installed = true;
    HostHandlers[SIGSTOP].Installed = true;

    // Most signals default to termination
    // These ones are slightly different
    static constexpr std::array<std::pair<int, SignalDelegator::DefaultBehaviour>, 14> SignalDefaultBehaviours = {{
      {SIGQUIT,   DEFAULT_COREDUMP},
      {SIGILL,    DEFAULT_COREDUMP},
      {SIGTRAP,   DEFAULT_COREDUMP},
      {SIGABRT,   DEFAULT_COREDUMP},
      {SIGBUS,    DEFAULT_COREDUMP},
      {SIGFPE,    DEFAULT_COREDUMP},
      {SIGSEGV,   DEFAULT_COREDUMP},
      {SIGCHLD,   DEFAULT_IGNORE},
      {SIGCONT,   DEFAULT_IGNORE},
      {SIGURG,    DEFAULT_IGNORE},
      {SIGXCPU,   DEFAULT_COREDUMP},
      {SIGXFSZ,   DEFAULT_COREDUMP},
      {SIGSYS,    DEFAULT_COREDUMP},
      {SIGWINCH,  DEFAULT_IGNORE},
    }};

    for (const auto &[Signal, Behaviour] : SignalDefaultBehaviours) {
      HostHandlers[Signal].DefaultBehaviour = Behaviour;
    }
  }

  SignalDelegator::~SignalDelegator() {
    for (int i = 0; i < MAX_SIGNALS; ++i) {
      if (i == 0 ||
          i == SIGKILL ||
          i == SIGSTOP ||
          !HostHandlers[i].Installed
          ) {
        continue;
      }
      ::syscall(SYS_rt_sigaction, i, &HostHandlers[i].OldAction, nullptr, 8);
      HostHandlers[i].Installed = false;
    }
    GlobalDelegator = nullptr;
  }

  void SignalDelegator::RegisterFrontendTLSState(FEXCore::Core::InternalThreadState *Thread) {
    // Set up our signal alternative stack
    // This is per thread rather than per signal
    ThreadData.AltStackPtr = FEXCore::Allocator::mmap(nullptr, SIGSTKSZ, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    stack_t altstack{};
    altstack.ss_sp = ThreadData.AltStackPtr;
    altstack.ss_size = SIGSTKSZ;
    altstack.ss_flags = 0;
    LOGMAN_THROW_A(!!altstack.ss_sp, "Couldn't allocate stack pointer");

    // Register the alt stack
    int Result = sigaltstack(&altstack, nullptr);
    if (Result == -1) {
      LogMan::Msg::E("Failed to install alternative signal stack %s", strerror(errno));
    }

    // Get the current host signal mask
    ::syscall(SYS_rt_sigprocmask, 0, nullptr, &ThreadData.CurrentSignalMask.Val, 8);
  }

  void SignalDelegator::UninstallFrontendTLSState(FEXCore::Core::InternalThreadState *Thread) {
    FEXCore::Allocator::munmap(ThreadData.AltStackPtr, SIGSTKSZ);

    ThreadData.AltStackPtr = nullptr;

    stack_t altstack{};
    altstack.ss_flags = SS_DISABLE;

    // Uninstall the alt stack
    int Result = sigaltstack(&altstack, nullptr);
    if (Result == -1) {
      LogMan::Msg::E("Failed to uninstall alternative signal stack %s", strerror(errno));
    }
  }

  void SignalDelegator::FrontendRegisterHostSignalHandler(int Signal, FEXCore::HostSignalDelegatorFunction Func, bool Required) {
    // Linux signal handlers are per-process rather than per thread
    // Multiple threads could be calling in to this
    std::lock_guard lk(HostDelegatorMutex);
    HostHandlers[Signal].Required = Required;
    InstallHostThunk(Signal);
  }

  void SignalDelegator::FrontendRegisterFrontendHostSignalHandler(int Signal, FEXCore::HostSignalDelegatorFunction Func, bool Required) {
    // Linux signal handlers are per-process rather than per thread
    // Multiple threads could be calling in to this
    std::lock_guard lk(HostDelegatorMutex);
    HostHandlers[Signal].Required = Required;
    InstallHostThunk(Signal);
  }

  void SignalDelegator::RegisterHostSignalHandlerForGuest(int Signal, FEXCore::HostSignalDelegatorFunctionForGuest Func) {
    std::lock_guard lk(HostDelegatorMutex);
    HostHandlers[Signal].GuestHandler = std::move(Func);
  }

  uint64_t SignalDelegator::RegisterGuestSignalHandler(int Signal, const FEXCore::GuestSigAction *Action, FEXCore::GuestSigAction *OldAction) {
    std::lock_guard lk(GuestDelegatorMutex);

    // Invalid signal specified
    if (Signal > MAX_SIGNALS) {
      return -EINVAL;
    }

    // If we have an old signal set then give it back
    if (OldAction) {
      *OldAction = HostHandlers[Signal].GuestAction;
    }

    // Now assign the new action
    if (Action) {
      // These signal dispositions can't be changed on Linux
      if (Signal == SIGKILL || Signal == SIGSTOP) {
        return -EINVAL;
      }

      HostHandlers[Signal].GuestAction = *Action;
      ThreadData.Guest_sa_mask[Signal] = Action->sa_mask;
      // Only attempt to install a new thunk handler if we were installing a new guest action
      if (!InstallHostThunk(Signal)) {
        UpdateHostThunk(Signal);
      }
    }

    return 0;
  }

  uint64_t SignalDelegator::RegisterGuestSigAltStack(const stack_t *ss, stack_t *old_ss) {
    auto Thread = GetTLSThread();
    bool UsingAltStack{};
    uint64_t AltStackBase = reinterpret_cast<uint64_t>(ThreadData.GuestAltStack.ss_sp);
    uint64_t AltStackEnd = AltStackBase + ThreadData.GuestAltStack.ss_size;
    uint64_t GuestSP = Thread->CurrentFrame->State.gregs[FEXCore::X86State::REG_RSP];

    if (!(ThreadData.GuestAltStack.ss_flags & SS_DISABLE) &&
        GuestSP >= AltStackBase &&
        GuestSP <= AltStackEnd) {
      UsingAltStack = true;
    }

    // If we have an old signal set then give it back
    if (old_ss) {
      *old_ss = ThreadData.GuestAltStack;

      if (UsingAltStack) {
        // We are currently operating on the alt stack
        // Let the guest know
        old_ss->ss_flags |= SS_ONSTACK;
      }
      else {
        old_ss->ss_flags |= SS_DISABLE;
      }
    }

    // Now assign the new action
    if (ss) {
      // If we tried setting the alt stack while we are using it then throw an error
      if (UsingAltStack) {
        return -EPERM;
      }

      // We need to check for invalid flags
      // The only flag that can be passed is SS_AUTODISARM and SS_DISABLE
      if (ss->ss_flags & ~(SS_AUTODISARM | SS_DISABLE)) {
        // A flag remained that isn't one of the supported ones?
        return -EINVAL;
      }

      if (ss->ss_flags & SS_DISABLE) {
        // If SS_DISABLE Is specified then the rest of the details are ignored
        ThreadData.GuestAltStack = *ss;
        return 0;
      }

      // stack size needs to be MINSIGSTKSZ (0x2000)
      if (ss->ss_size < X86_MINSIGSTKSZ) {
        return -ENOMEM;
      }

      ThreadData.GuestAltStack = *ss;
    }

    return 0;
  }

  static void CheckForPendingSignals(FEXCore::Core::InternalThreadState *Thread) {
    // Do we have any pending signals that became unmasked?
    uint64_t PendingSignals = ~ThreadData.CurrentSignalMask.Val & ThreadData.PendingSignals;
    if (PendingSignals != 0) {
      for (int i = 0; i < 64; ++i) {
        if (PendingSignals & (1ULL << i)) {
          tgkill(Thread->ThreadManager.PID, Thread->ThreadManager.TID, i + 1);
          // We might not even return here which is spooky
        }
      }
    }
  }

  uint64_t SignalDelegator::GuestSigProcMask(int how, const uint64_t *set, uint64_t *oldset) {
    if (!!oldset) {
      *oldset = ThreadData.CurrentSignalMask.Val;
    }

    if (!!set) {
      uint64_t IgnoredSignalsMask = ~((1ULL << (SIGKILL - 1)) | (1ULL << (SIGSTOP - 1)));
      if (how == SIG_BLOCK) {
        ThreadData.CurrentSignalMask.Val |= *set & IgnoredSignalsMask;
      }
      else if (how == SIG_UNBLOCK) {
        ThreadData.CurrentSignalMask.Val &= ~(*set & IgnoredSignalsMask);
      }
      else if (how == SIG_SETMASK) {
        ThreadData.CurrentSignalMask.Val = *set & IgnoredSignalsMask;
      }
      else {
        return -EINVAL;
      }

      uint64_t HostMask = ThreadData.CurrentSignalMask.Val;
      // Now actually set the host mask
      // This will hide from the guest that we are not actually setting all of the masks it wants
      for (size_t i = 0; i < MAX_SIGNALS; ++i) {
        if (HostHandlers[i + 1].Required.load(std::memory_order_relaxed)) {
          // If it is a required host signal then we can't mask it
          HostMask &= ~(1ULL << i);
        }
      }

      ::syscall(SYS_rt_sigprocmask, SIG_SETMASK, &HostMask, nullptr, 8);
    }

    CheckForPendingSignals(GetTLSThread());

    return 0;
  }

  uint64_t SignalDelegator::GuestSigPending(uint64_t *set, size_t sigsetsize) {
    if (sigsetsize > sizeof(uint64_t)) {
      return -EINVAL;
    }

    *set = ThreadData.PendingSignals;

    sigset_t HostSet{};
    if (sigpending(&HostSet) == 0) {
      uint64_t HostSignals{};
      for (size_t i = 0; i < MAX_SIGNALS; ++i) {
        if (sigismember(&HostSet, i + 1)) {
          HostSignals |= (1ULL << i);
        }
      }

      // Merge the real pending signal mask as well
      *set |= HostSignals;
    }
    return 0;
  }

  uint64_t SignalDelegator::GuestSigSuspend(uint64_t *set, size_t sigsetsize) {
    if (sigsetsize > sizeof(uint64_t)) {
      return -EINVAL;
    }

    uint64_t IgnoredSignalsMask = ~((1ULL << (SIGKILL - 1)) | (1ULL << (SIGSTOP - 1)));

    // Backup the mask
    ThreadData.PreviousSuspendMask = ThreadData.CurrentSignalMask;
    // Set the new mask
    ThreadData.CurrentSignalMask.Val = *set & IgnoredSignalsMask;
    ThreadData.Suspended = true;
    sigset_t HostSet{};

    sigemptyset(&HostSet);

    for (int32_t i = 0; i < MAX_SIGNALS; ++i) {
      if (*set & (1ULL << i)) {
        sigaddset(&HostSet, i + 1);
      }
    }

    // Additionally we must always listen to SIGNAL_FOR_PAUSE
    // This technically forces us in to a race but should be fine
    // SIGBUS and SIGILL can't happen so we don't need to listen for them
    //sigaddset(&HostSet, SIGNAL_FOR_PAUSE);

    // Spin this in a loop until we aren't sigsuspended
    // This can happen in the case that the guest has sent signal that we can't block
    uint64_t Result = sigsuspend(&HostSet);

    CheckForPendingSignals(GetTLSThread());

    return Result == -1 ? -errno : Result;

  }

  uint64_t SignalDelegator::GuestSigTimedWait(uint64_t *set, siginfo_t *info, const struct timespec *timeout, size_t sigsetsize) {
    if (sigsetsize > sizeof(uint64_t)) {
      return -EINVAL;
    }

    sigset_t HostSet{};
    sigemptyset(&HostSet);

    for (int32_t i = 0; i < MAX_SIGNALS; ++i) {
      if (*set & (1ULL << i)) {
        sigaddset(&HostSet, i + 1);
      }
    }

    uint64_t Result = sigtimedwait(&HostSet, info, timeout);

    return Result == -1 ? -errno : Result;
  }

  uint64_t SignalDelegator::GuestSignalFD(int fd, const uint64_t *set, size_t sigsetsize, int flags) {
    if (sigsetsize > sizeof(uint64_t)) {
      return -EINVAL;
    }

    sigset_t HostSet{};
    sigemptyset(&HostSet);

    for (size_t i = 0; i < MAX_SIGNALS; ++i) {
      if (HostHandlers[i + 1].Required.load(std::memory_order_relaxed)) {
        // For now skip our internal signals
        continue;
      }

      if (ThreadData.CurrentSignalMask.Val & (1ULL << i)) {
        sigaddset(&HostSet, i + 1);
      }
    }

    // XXX: This is a barebones implementation just to get applications that listen for SIGCHLD to work
    // In the future we need our own listern thread that forwards the result
    // Thread is necessary to prevent deadlocks for a thread that has signaled on the same thread listening to the FD and blocking is enabled
    uint64_t Result = signalfd(fd, &HostSet, flags);

    return Result == -1 ? -errno : Result;
  }

}
