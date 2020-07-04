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

    auto Thread = ThreadData.Thread;
    SignalHandler &Handler = HostHandlers[Signal];
    if (Handler.Handler &&
        Handler.Handler(Thread, Signal, Info, UContext)) {
      // If the host handler handled the fault then we can continue now
      return;
    }

    if (Signal == SIGCHLD) {
      // Do some special handling around this signal
      // If the guest has a signal handler installed with SA_NOCLDSTOP or SA_NOCHLDWAIT then
      // handle carefully
      if (Handler.GuestAction.sa_flags & SA_NOCLDSTOP) {
        // No signal is generated in this case
        // just safely return
        // XXX: If this was a child that exited then don't deliver the signal
        // If called from signal then it still need to be delivered
        // Handle this
      }

      if (Handler.GuestAction.sa_flags & SA_NOCLDWAIT) {
        // Linux will still generate a signal for this
        // POSIX leaves it unspecific
        // "do not transform children in to zombies when they terminate"
        // XXX: Handle this
      }
    }

    // XXX: Setup our state to jump back in to the JIT at the guest handler's location
    // TLS is safe on x86-64 and AArch64 hosts
    if (!Thread) {
      LogMan::Msg::E("Thread has received a signal and hasn't registered itself with the delegate! Programming error!");
    }
    else {
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
        // XXX: Handle the guest signal and return here
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

  void SignalDelegator::InstallHostThunk(int Signal) {
    // If the host thunk is already installed for this, just return
    if (HostHandlers[Signal].Installed) {
      return;
    }

    // Now install the thunk handler
    struct sigaction act{};
    act.sa_sigaction = &SignalHandlerThunk;
    act.sa_flags = SA_SIGINFO | SA_RESTART;

    // We don't care about the previous handler in this case
    int Result = sigaction(Signal, &act, &HostHandlers[Signal].OldAction);
    if (Result < 0) {
      LogMan::Msg::D("Failed to install host signal thunk for signal %d: %s", Signal, strerror(errno));
      return;
    }

    HostHandlers[Signal].Installed = true;
  }

  SignalDelegator::SignalDelegator(FEXCore::Context::Context *ctx)
    : CTX {ctx} {
    // Register this delegate
    LogMan::Throw::A(!GlobalDelegator, "Can't register global delegator multiple times!");
    GlobalDelegator = this;

    // XXX: Let's just say that these are installed for now
    // We can't have the guest capturing these yet so it would do nothing
    // Resulting in Ctrl-C and Ctrl-\ breaking
    HostHandlers[SIGINT].Installed = true;
    HostHandlers[SIGQUIT].Installed = true;

    // glibc reserves these two signals internally
    // __SIGRTMIN(32) is used for a "cancellation" signal
    // __SIGRTMIN+1 is used for setuid handling
    // "Userspace" SIGRTMIN starts at 34 because of this
    HostHandlers[__SIGRTMIN].Installed   = true;
    HostHandlers[__SIGRTMIN+1].Installed = true;
  }

  void SignalDelegator::RegisterTLSState(FEXCore::Core::InternalThreadState *Thread) {
    ThreadData.Thread = Thread;
  }

  void SignalDelegator::MaskSignals(int how) {
    // If we have a helper thread, we need to mask a significant amount of signals so the an errant thread doesn't receive a signal that it shouldn't
    sigset_t SignalSet{};
    sigemptyset(&SignalSet);

    for (int i = 0; i < MAX_SIGNALS; ++i) {
      // If it is a synchronous signal then don't ignore it
      if (IsSynchronous(i)) {
        continue;
      }

      // Add this signal to the ignore list
      sigaddset(&SignalSet, i);
    }

    // Be warned, a thread will inherit the signal mask if created from this thread
    int Result = pthread_sigmask(how, &SignalSet, nullptr);
    if (Result != 0) {
      LogMan::Msg::D("Couldn't register thread to mask signals");
    }
  }

  void SignalDelegator::MaskThreadSignals() {
    MaskSignals(SIG_BLOCK);
  }

  void SignalDelegator::ResetThreadSignalMask() {
    MaskSignals(SIG_UNBLOCK);
  }

  void SignalDelegator::RegisterHostSignalHandler(int Signal, HostSignalDelegatorFunction Func) {
    // Linux signal handlers are per-process rather than per thread
    // Multiple threads could be calling in to this
    std::lock_guard<std::mutex> lk(HostDelegatorMutex);
    HostHandlers[Signal].Handler = Func;
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
      InstallHostThunk(Signal);
    }

    return 0;
  }
}
