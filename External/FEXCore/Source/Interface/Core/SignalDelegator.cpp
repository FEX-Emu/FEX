#include "Interface/Context/Context.h"
#include "Interface/Core/InternalThreadState.h"
#include "Interface/Core/SignalDelegator.h"
#include <LogManager.h>

#include <string.h>

namespace FEXCore {
  // We can only have one delegator per process
  static SignalDelegator *GlobalDelegator{};

  struct ThreadState {
    FEXCore::Core::InternalThreadState *Thread{};
    void *AltStackPtr{};
    stack_t GuestAltStack{};
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

  void SignalDelegator::HandleSignal(int Signal, void *Info, void *UContext) {
    // Let the host take first stab at handling the signal
    siginfo_t *SigInfo = static_cast<siginfo_t*>(Info);
    auto Thread = ThreadData.Thread;
    SignalHandler &Handler = HostHandlers[Signal];

    if (!Thread) {
      LogMan::Msg::E("Thread has received a signal and hasn't registered itself with the delegate! Programming error!");
      return;
    }

    if (Handler.Handler &&
        Handler.Handler(Thread, Signal, Info, UContext)) {
      // If the host handler handled the fault then we can continue now
      return;
    }

    if (Signal == SIGCHLD) {
      // Do some special handling around this signal
      // If the guest has a signal handler installed with SA_NOCLDSTOP or SA_NOCHLDWAIT then
      // handle carefully
      if (Handler.GuestAction.sa_flags & SA_NOCLDSTOP &&
          SigInfo->si_code != SI_TKILL) {
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

    // We have an emulation thread pointer, we can now modify its state
    if (Handler.GuestAction.sa_handler == SIG_DFL) {
      // XXX: Maybe this should actually go down guest handler state?
      signal(Signal, SIG_DFL);
      return;
    }
    else if (Handler.GuestAction.sa_handler == SIG_IGN) {
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

  SignalDelegator::SignalDelegator(FEXCore::Context::Context *ctx)
    : CTX {ctx} {
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

  void SignalDelegator::RegisterHostSignalHandlerForGuest(int Signal, HostSignalDelegatorFunctionForGuest Func) {
    std::lock_guard<std::mutex> lk(HostDelegatorMutex);
    HostHandlers[Signal].GuestHandler = Func;
    InstallHostThunk(Signal);
  }

  uint64_t SignalDelegator::RegisterGuestSignalHandler(int Signal, const struct sigaction *Action, struct sigaction *OldAction) {
    std::lock_guard<std::mutex> lk(GuestDelegatorMutex);

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
      // Only attempt to install a new thunk handler if we were installing a new guest action
      if (!InstallHostThunk(Signal)) {
        UpdateHostThunk(Signal);
      }
    }

    return 0;
  }

  uint64_t SignalDelegator::RegisterGuestSigAltStack(const stack_t *ss, stack_t *old_ss) {
    // If we have an old signal set then give it back
    if (old_ss) {
      *old_ss = ThreadData.GuestAltStack;
    }

    // Now assign the new action
    if (ss) {
      ThreadData.GuestAltStack = *ss;
    }

    return 0;
  }

}
