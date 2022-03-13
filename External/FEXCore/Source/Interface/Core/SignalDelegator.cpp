#include <FEXCore/Core/SignalDelegator.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXHeaderUtils/Syscalls.h>

#include <unistd.h>
#include <signal.h>

namespace FEXCore {
  struct ThreadState {
    FEXCore::Core::InternalThreadState *Thread{};
  };

  thread_local ThreadState ThreadData{};

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

  /**
   * @brief Masks signals from the signal mask
   *
   * @param how Argument to sigmask. SIG_{BLOCK, SETMASK, UNBLOCK}
   * @param Signal Which signal to set or -1 to sweep through them all
   */
  static void MaskSignals(int how, int Signal = -1) {
    // If we have a helper thread, we need to mask a significant amount of signals so the an errant thread doesn't receive a signal that it shouldn't
    sigset_t SignalSet{};
    sigemptyset(&SignalSet);

    if (Signal == -1) {
      for (int i = 0; i <= SignalDelegator::MAX_SIGNALS; ++i) {
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
      LogMan::Msg::EFmt("Couldn't register thread to mask signals");
    }
  }

  void SignalDelegator::MaskThreadSignals() {
    MaskSignals(SIG_BLOCK);
  }

  FEXCore::Core::InternalThreadState *SignalDelegator::GetTLSThread() {
    return ThreadData.Thread;
  }

  void SignalDelegator::RegisterTLSState(FEXCore::Core::InternalThreadState *Thread) {
    ThreadData.Thread = Thread;
    RegisterFrontendTLSState(Thread);
  }

  void SignalDelegator::UninstallTLSState(FEXCore::Core::InternalThreadState *Thread) {
    UninstallFrontendTLSState(Thread);
    ThreadData.Thread = nullptr;
  }

  void SignalDelegator::RegisterHostSignalHandler(int Signal, HostSignalDelegatorFunction Func, bool Required) {
    SetHostSignalHandler(Signal, Func, Required);
    FrontendRegisterHostSignalHandler(Signal, Func, Required);
  }

  void SignalDelegator::RegisterFrontendHostSignalHandler(int Signal, HostSignalDelegatorFunction Func, bool Required) {
    SetFrontendHostSignalHandler(Signal, Func, Required);
    FrontendRegisterFrontendHostSignalHandler(Signal, Func, Required);
  }

  void SignalDelegator::HandleSignal(int Signal, void *Info, void *UContext) {
    // Let the host take first stab at handling the signal
    auto Thread = GetTLSThread();
    HostSignalHandler &Handler = HostHandlers[Signal];

    if (!Thread) {
      LogMan::Msg::EFmt("[{}] Thread has received a signal and hasn't registered itself with the delegate! Programming error!", FHU::Syscalls::gettid());
    }
    else {
      for (auto &Handler : Handler.Handlers) {
        if (Handler(Thread, Signal, Info, UContext)) {
          // If the host handler handled the fault then we can continue now
          return;
        }
      }

      if (Handler.FrontendHandler &&
          Handler.FrontendHandler(Thread, Signal, Info, UContext)) {
        return;
      }

      // Now let the frontend handle the signal
      // It's clearly a guest signal and this ends up being an OS specific issue
      HandleGuestSignal(Thread, Signal, Info, UContext);
    }
  }
}
