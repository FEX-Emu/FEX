#include "Interface/Context/Context.h"
#include "Interface/Core/InternalThreadState.h"
#include "Interface/Core/SignalDelegator.h"

#include <FEXCore/Core/X86Enums.h>
#include <LogManager.h>

#include <string.h>

#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace FEXCore {
  constexpr static uint32_t SS_AUTODISARM = (1U << 31);
  constexpr static uint32_t X86_MINSIGSTKSZ  = 0x2000U;

  // We can only have one delegator per process
  static SignalDelegator *GlobalDelegator{};

  struct ThreadState {
    FEXCore::Core::InternalThreadState *Thread{};
    void *AltStackPtr{};
    stack_t GuestAltStack {
      .ss_sp = nullptr,
      .ss_flags = SS_DISABLE, // By default the guest alt stack is disabled
      .ss_size = 0,
    };
    // Guest signal sa_mask is per thread!
    SignalDelegator::GuestSAMask Guest_sa_mask[SignalDelegator::MAX_SIGNALS]{};
    uint32_t CurrentSignal{};
    uint64_t GuestProcMask{};
    uint64_t PendingSignals{};
    bool Suspended {false};
  };

  thread_local ThreadState ThreadData{};

  static void SignalHandlerThunk(int Signal, siginfo_t *Info, void *UContext) {
    GlobalDelegator->HandleSignal(Signal, Info, UContext);
  }

  static bool IsSynchronous(int Signal) {
    switch (Signal) {
    case SIGBUS:
    case SIGFPE:
    case SIGILL:
    case SIGSEGV:
    case SIGTRAP:
      return true;
    default: break;
    };
    return false;
  }

  uint64_t SigIsMember(SignalDelegator::GuestSAMask *Set, int Signal) {
    // Signal 0 isn't real, so everything is offset by one inside the set
    Signal -= 1;
    return (Set->Val >> Signal) & 1;
  }

  void SignalDelegator::SetCurrentSignal(uint32_t Signal) {
    ThreadData.CurrentSignal = Signal;
  }

  void SignalDelegator::HandleSignal(int Signal, void *Info, void *UContext) {
    // Let the host take first stab at handling the signal
    siginfo_t *SigInfo = static_cast<siginfo_t*>(Info);
    auto Thread = ThreadData.Thread;
    SignalHandler &Handler = HostHandlers[Signal];

    if (!Thread) {
      LogMan::Msg::E("Thread has received a signal and hasn't registered itself with the delegate! Programming error!");
    }
    else {
      if (Handler.Handler &&
          Handler.Handler(Thread, Signal, Info, UContext)) {
        // If the host handler handled the fault then we can continue now
        return;
      }

      if (Handler.FrontendHandler &&
          Handler.FrontendHandler(Thread, Signal, Info, UContext)) {
        return;
      }

      // If the signal was sent by the user with kill then we can't block it
      // If it was sent by raise() then we /can/ block it
      bool SentByUser = SigInfo->si_code <= 0;

      if (Signal == SIGCHLD) {
        // Do some special handling around this signal
        // If the guest has a signal handler installed with SA_NOCLDSTOP or SA_NOCHLDWAIT then
        // handle carefully
        if (Handler.GuestAction.sa_flags & SA_NOCLDSTOP &&
            !SentByUser) {
          // If we were sent the signal from kill, tkill, or tgkill
          // then si_code is set to SI_TKILL and should be delivered to the guest
          // Otherwise if NOCLDSTOP is set without this code, just drop the signal
          return;
        }

        if (Handler.GuestAction.sa_flags & SA_NOCLDWAIT) {
          // Linux will still generate a signal for this
          // POSIX leaves it unspecific
          // "do not transform children in to zombies when they terminate"
          // XXX: Handle this
        }
      }

      // Signal specific mask check
      if (SigIsMember(&ThreadData.Guest_sa_mask[ThreadData.CurrentSignal], Signal)) {
        // If we are in a signal and our signal mask is blocking this signal then ignore it
        return;
      }

      // Thread specific mask check
      if (ThreadData.GuestProcMask & (Signal - 1)) {
        // If the thread specific mask masks the signal then it is hidden from the guest
        // It gets put in to the pending signals mask
        ThreadData.PendingSignals |= 1ULL << (Signal - 1);
        return;
      }

      ThreadData.CurrentSignal = Signal;

      // We have an emulation thread pointer, we can now modify its state
      if (Handler.GuestAction.sigaction_handler.handler == SIG_DFL) {
        if (Handler.DefaultBehaviour == DEFAULT_TERM) {
          if (Thread->State.ThreadManager.clear_child_tid) {
            std::atomic<uint32_t> *Addr = reinterpret_cast<std::atomic<uint32_t>*>(Thread->State.ThreadManager.clear_child_tid);
            Addr->store(0);
            syscall(SYS_futex,
                Thread->State.ThreadManager.clear_child_tid,
                FUTEX_WAKE,
                ~0ULL,
                0,
                0,
                0);
          }

          Thread->StatusCode = -Signal;

          // Doesn't return
          Thread->CTX->StopThread(Thread);
          std::unexpected();
        }
      }
      else if (Handler.GuestAction.sigaction_handler.handler == SIG_IGN) {
        return;
      }
      else {
        if (Handler.GuestHandler &&
            Handler.GuestHandler(Thread, Signal, Info, UContext, &Handler.GuestAction, &ThreadData.GuestAltStack)) {
          // If we were sigsuspended, then we will no longer be from here
          ThreadData.Suspended = false;
          return;
        }
        ERROR_AND_DIE("Unhandled guest exception");
      }
    }

    // Unhandled crash
    // Call back in to the previous handler
    if (Handler.OldAction.sa_flags & SA_SIGINFO) {
      Handler.OldAction.sa_sigaction(Signal, static_cast<siginfo_t*>(Info), UContext);
    }
    else if (Handler.OldAction.sa_handler == SIG_DFL) {
      signal(Signal, SIG_DFL);
    }
    else if (Handler.OldAction.sa_handler == SIG_IGN) {
    }
    else {
      Handler.OldAction.sa_handler(Signal);
    }
  }

  bool SignalDelegator::InstallHostThunk(int Signal) {
    SignalHandler &SignalHandler = HostHandlers[Signal];
    // If the host thunk is already installed for this, just return
    if (SignalHandler.Installed) {
      return false;
    }

    // Now install the thunk handler
    SignalHandler.HostAction.sa_sigaction = &SignalHandlerThunk;
    SignalHandler.HostAction.sa_flags = SA_SIGINFO | SA_RESTART | SA_ONSTACK;

    if (SignalHandler.GuestAction.sa_flags & SA_NODEFER) {
      // If the guest is using NODEFER then make sure to set it for the host as well
      SignalHandler.HostAction.sa_flags |= SA_NODEFER;
    }

    /*
     * XXX: This isn't quite as straightforward as a memcmp
     * There are conflicting definitions between sigset_t and __sigset_t causing problems here
    sigset_t EmptySet{};
    sigemptyset(&EmptySet);
    if (SignalHandler.GuestAction.sa_mask != EmptySet) {
      // If the guest has masked some signals then we need to also mask those signals
      SignalHandler.HostAction.sa_mask = SignalHandler.GuestAction.sa_mask;

      // If the guest tried masking SIGILL or SIGBUS then too bad, we actually need this on the host
      sigdelset(SignalHandler.HostAction.sa_mask, SIGILL);
      sigdelset(SignalHandler.HostAction.sa_mask, SIGBUS);
    }
    */

    // We don't care about the previous handler in this case
    int Result = sigaction(Signal, &SignalHandler.HostAction, &SignalHandler.OldAction);
    if (Result < 0) {
      LogMan::Msg::E("Failed to install host signal thunk for signal %d: %s", Signal, strerror(errno));
      return false;
    }

    SignalHandler.Installed = true;
    return true;
  }

  void SignalDelegator::UpdateHostThunk(int Signal) {
    SignalHandler &SignalHandler = HostHandlers[Signal];
    bool Changed{};

    // This only gets called if a guest thunk was already installed and we need to check if we need to update the flags or signal mask
    if ((SignalHandler.GuestAction.sa_flags ^ SignalHandler.HostAction.sa_flags) & SA_NODEFER) {
      // NODEFER changed, we need to update this
      SignalHandler.HostAction.sa_flags |= SignalHandler.GuestAction.sa_flags & SA_NODEFER;
      Changed = true;
    }

    /*
    if ((SignalHandler.GuestAction.sa_mask ^ SignalHandler.HostAction.sa_mask) & ~(SIGILL | SIGBUS)) {
      // If the signal ignore mask has updated (avoiding the two we need for the host) then we need to update
      SignalHandler.HostAction.sa_mask = SignalHandler.GuestAction.sa_mask;
      sigdelset(SignalHandler.HostAction.sa_mask, SIGILL);
      sigdelset(SignalHandler.HostAction.sa_mask, SIGBUS);
      Changed = true;
    }
    */

    if (!Changed) {
      return;
    }

    // Only update our host signal here
    int Result = sigaction(Signal, &SignalHandler.HostAction, nullptr);
    if (Result < 0) {
      LogMan::Msg::E("Failed to update host signal thunk for signal %d: %s", Signal, strerror(errno));
    }
  }

  SignalDelegator::SignalDelegator() {
    // Register this delegate
    LogMan::Throw::A(!GlobalDelegator, "Can't register global delegator multiple times!");
    GlobalDelegator = this;
    // Signal zero isn't real
    HostHandlers[0].Installed = true;

    // We can't capture SIGKILL or SIGSTOP
    HostHandlers[SIGKILL].Installed = true;
    HostHandlers[SIGSTOP].Installed = true;

    // glibc reserves these two signals internally
    // __SIGRTMIN(32) is used for a "cancellation" signal
    // __SIGRTMIN+1 is used for setuid handling
    // "Userspace" SIGRTMIN starts at 34 because of this
    HostHandlers[__SIGRTMIN].Installed   = true;
    HostHandlers[__SIGRTMIN+1].Installed = true;

    // Most signals default to termination
    // These ones are slightly different
    const std::vector<std::pair<int, SignalDelegator::DefaultBehaviour>> SignalDefaultBehaviours = {
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
    };

    for (auto Behaviour : SignalDefaultBehaviours) {
      HostHandlers[Behaviour.first].DefaultBehaviour = Behaviour.second;
    }
  }

  void SignalDelegator::RegisterTLSState(FEXCore::Core::InternalThreadState *Thread) {
    ThreadData.Thread = Thread;

    // Set up our signal alternative stack
    // This is per thread rather than per signal
    ThreadData.AltStackPtr = malloc(SIGSTKSZ);
    stack_t altstack{};
    altstack.ss_sp = ThreadData.AltStackPtr;
    altstack.ss_size = SIGSTKSZ;
    altstack.ss_flags = 0;
    LogMan::Throw::A(!!altstack.ss_sp, "Couldn't allocate stack pointer");

    // Register the alt stack
    int Result = sigaltstack(&altstack, nullptr);
    if (Result == -1) {
      LogMan::Msg::E("Failed to install alternative signal stack %s", strerror(errno));
    }
  }

  void SignalDelegator::UninstallTLSState(FEXCore::Core::InternalThreadState *Thread) {
    free(ThreadData.AltStackPtr);

    ThreadData.Thread = nullptr;
    ThreadData.AltStackPtr = nullptr;

    stack_t altstack{};
    altstack.ss_flags = SS_DISABLE;

    // Uninstall the alt stack
    int Result = sigaltstack(&altstack, nullptr);
    if (Result == -1) {
      LogMan::Msg::E("Failed to uninstall alternative signal stack %s", strerror(errno));
    }
  }

  void SignalDelegator::MaskSignals(int how, int Signal) {
    // If we have a helper thread, we need to mask a significant amount of signals so the an errant thread doesn't receive a signal that it shouldn't
    sigset_t SignalSet{};
    sigemptyset(&SignalSet);

    if (Signal == -1) {
      for (int i = 0; i < MAX_SIGNALS; ++i) {
        // If it is a synchronous signal then don't ignore it
        if (IsSynchronous(i)) {
          continue;
        }

        // Add this signal to the ignore list
        sigaddset(&SignalSet, i);
      }
    }
    else {
      sigaddset(&SignalSet, Signal);
    }

    // Be warned, a thread will inherit the signal mask if created from this thread
    int Result = pthread_sigmask(how, &SignalSet, nullptr);
    if (Result != 0) {
      LogMan::Msg::E("Couldn't register thread to mask signals");
    }
  }

  void SignalDelegator::MaskThreadSignals() {
    MaskSignals(SIG_BLOCK);
  }

  void SignalDelegator::ResetThreadSignalMask() {
    MaskSignals(SIG_UNBLOCK);
  }

  bool SignalDelegator::BlockSignal(int Signal) {
    MaskSignals(SIG_BLOCK, Signal);
    return true;
  }

  bool SignalDelegator::UnblockSignal(int Signal) {
    MaskSignals(SIG_UNBLOCK, Signal);
    return true;
  }

  void SignalDelegator::RegisterHostSignalHandler(int Signal, HostSignalDelegatorFunction Func) {
    // Linux signal handlers are per-process rather than per thread
    // Multiple threads could be calling in to this
    std::lock_guard<std::mutex> lk(HostDelegatorMutex);
    HostHandlers[Signal].Handler = Func;
    InstallHostThunk(Signal);
  }

  void SignalDelegator::RegisterFrontendHostSignalHandler(int Signal, HostSignalDelegatorFunction Func) {
    // Linux signal handlers are per-process rather than per thread
    // Multiple threads could be calling in to this
    std::lock_guard<std::mutex> lk(HostDelegatorMutex);
    HostHandlers[Signal].FrontendHandler = Func;
    InstallHostThunk(Signal);
  }

  void SignalDelegator::RegisterHostSignalHandlerForGuest(int Signal, HostSignalDelegatorFunctionForGuest Func) {
    std::lock_guard<std::mutex> lk(HostDelegatorMutex);
    HostHandlers[Signal].GuestHandler = Func;
    InstallHostThunk(Signal);
  }

  uint64_t SignalDelegator::RegisterGuestSignalHandler(int Signal, const GuestSigAction *Action, GuestSigAction *OldAction) {
    std::lock_guard<std::mutex> lk(GuestDelegatorMutex);

    // If we have an old signal set then give it back
    if (OldAction) {
      *OldAction = HostHandlers[Signal].GuestAction;
    }

    // Now assign the new action
    if (Action) {
      // These signal dispositions can't be changed on Linux
      if (Signal == SIGKILL || Signal == SIGSTOP || Signal > MAX_SIGNALS) {
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
    bool UsingAltStack{};
    uint64_t AltStackBase = reinterpret_cast<uint64_t>(ThreadData.GuestAltStack.ss_sp);
    uint64_t AltStackEnd = AltStackBase + ThreadData.GuestAltStack.ss_size;
    uint64_t GuestSP = ThreadData.Thread->State.State.gregs[X86State::REG_RSP];

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

  uint64_t SignalDelegator::GuestSigProcMask(int how, const uint64_t *set, uint64_t *oldset) {
    if (!!oldset) {
      *oldset = ThreadData.GuestProcMask;
    }

    if (!!set) {
      uint64_t IgnoredSignalsMask = ~((1ULL << (SIGKILL - 1)) | (1ULL << (SIGSTOP - 1)));
      if (how == SIG_BLOCK) {
        ThreadData.GuestProcMask |= *set & IgnoredSignalsMask;
      }
      else if (how == SIG_UNBLOCK) {
        ThreadData.GuestProcMask &= *set & IgnoredSignalsMask;
      }
      else if (how == SIG_SETMASK) {
        ThreadData.GuestProcMask = *set & IgnoredSignalsMask;
      }
      else {
        return -EINVAL;
      }
    }

    // Do we have any pending signals that became unmasked?
    uint64_t PendingSignals = ~ThreadData.GuestProcMask & ThreadData.PendingSignals;
    if (PendingSignals != 0) {
      for (int i = 0; i < 64; ++i) {
        if (PendingSignals & (1ULL << i)) {
          tgkill(ThreadData.Thread->State.ThreadManager.PID, ThreadData.Thread->State.ThreadManager.TID, i + 1);
          PendingSignals &= ~(1ULL << i);
        }
      }
    }

    return 0;
  }

  uint64_t SignalDelegator::GuestSigPending(uint64_t *set, size_t sigsetsize) {
    if (sigsetsize > sizeof(uint64_t)) {
      return -EINVAL;
    }

    *set = ThreadData.PendingSignals;
    return 0;
  }

  uint64_t SignalDelegator::GuestSigSuspend(uint64_t *set, size_t sigsetsize) {
    if (sigsetsize > sizeof(uint64_t)) {
      return -EINVAL;
    }

    uint64_t BackupMask = ThreadData.GuestProcMask;
    uint64_t IgnoredSignalsMask = ~((1ULL << (SIGKILL - 1)) | (1ULL << (SIGSTOP - 1)));
    ThreadData.GuestProcMask = *set & IgnoredSignalsMask;
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
    sigaddset(&HostSet, SIGNAL_FOR_PAUSE);

    // Spin this in a loop until we aren't sigsuspended
    // This can happen in the case that the guest has sent signal that we can't block
    uint64_t Result = 0;
    while (ThreadData.Suspended) {
      // This is technically a race on our end
      Result = sigsuspend(&HostSet);

      // On error then exit out
      // It can only ever error on EINTR
      if (Result == -1) {
        ThreadData.Suspended = false;
        break;
      }
    }

    ThreadData.GuestProcMask = BackupMask;

    SYSCALL_ERRNO();
  }
}
