#include <FEXCore/Core/SignalDelegator.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXHeaderUtils/Syscalls.h>
#include <FEXHeaderUtils/ScopedSignalMask.h>

#include <atomic>
#include <unistd.h>
#include <signal.h>
#include <csetjmp>

#include "FEXCore/Debug/InternalThreadState.h"

#define HOST_DEFER_TRACE do { } while (false)
//#define HOST_DEFER_TRACE do { char str[512]; write(1, str, sprintf(str,"%*s%d %s\n", Previous, "", Previous, __func__)); } while (false)

namespace FEXCore {
  struct ThreadState {
    FEXCore::Core::InternalThreadState *Thread{};
    sigjmp_buf HostDeferredSignalJump;
    sigset_t HostDeferredSigmask; // only 8 bytes used here, depends on kernel configuration
    
    std::atomic<uint16_t> HostDeferredSignalEnabled;
    std::atomic<uint16_t> HostDeferredSignalAutoEnabled;
    std::atomic<int> HostDeferredSignalPending;
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

  void SignalDelegator::DeferThreadHostSignals() {
    [[maybe_unused]] auto Previous = ThreadData.HostDeferredSignalEnabled.fetch_add(1, std::memory_order_relaxed);
    LOGMAN_THROW_A_FMT(Previous != UINT16_MAX - 1, "Signal Host Deferring Overflow");

    HOST_DEFER_TRACE;
  }

  void SignalDelegator::DeliverThreadHostDeferredSignals() {
    [[maybe_unused]] auto Previous = ThreadData.HostDeferredSignalEnabled.fetch_sub(1, std::memory_order_relaxed);
    LOGMAN_THROW_A_FMT(Previous != 0, "Signal Host Deferring Underflow");

    HOST_DEFER_TRACE;

    if (Previous == 1 && ThreadData.HostDeferredSignalPending) {
      // deliver Pending signal
      siglongjmp(ThreadData.HostDeferredSignalJump, true);
    }
  }

  void SignalDelegator::EnterAutoHostDefer() {
    [[maybe_unused]] auto Previous = ThreadData.HostDeferredSignalAutoEnabled.fetch_add(1, std::memory_order_relaxed);
    LOGMAN_THROW_A_FMT(Previous != UINT16_MAX - 1, "Signal Auto Host Deferring Overflow");

    HOST_DEFER_TRACE;
  }

  void SignalDelegator::LeaveAutoHostDefer() {
    [[maybe_unused]] auto Previous = ThreadData.HostDeferredSignalAutoEnabled.fetch_sub(1, std::memory_order_relaxed);
    LOGMAN_THROW_A_FMT(Previous != 0, "Signal Auto Host Deferring Underflow");

    HOST_DEFER_TRACE;
  }

  //FEX_TODO("Enforce that Host Deferred Signals don't get delivered between Acquire/Release")
  bool SignalDelegator::AcquireHostDeferredSignals() {
    if (ThreadData.HostDeferredSignalAutoEnabled.load(std::memory_order_relaxed)) {
      DeferThreadHostSignals();
      return true;
    }

    LOGMAN_THROW_A_FMT(ThreadData.HostDeferredSignalEnabled, "Host Signals need to be Deferred before AcquireHostDeferredSignals");
    return false;
  }

  void SignalDelegator::ReleaseHostDeferredSignals() {
    LOGMAN_THROW_A_FMT(ThreadData.HostDeferredSignalEnabled, "Host Signals need to be Delivered after ReleaseHostDeferredSignals");
    if (ThreadData.HostDeferredSignalAutoEnabled.load(std::memory_order_relaxed)) {
      DeliverThreadHostDeferredSignals();
    }
  }

  void SignalDelegator::HandleSignal(int Signal, void *Info, void *UContext) {
    // Let the host take first stab at handling the signal
    auto Thread = GetTLSThread();

    if (!ThreadData.HostDeferredSignalEnabled) {
      LOGMAN_THROW_A_FMT(!ThreadData.HostDeferredSignalPending, "Host Deferred signal tearing, delivering {} while pending {}", Signal, ThreadData.HostDeferredSignalPending);
      DoHandleSignal: {
        // No signals must be delivered in this scope - should be guaranteed.
        // FEX_TODO("Enforce no signal delivery in this scope")
        FHU::ScopedSignalHostDefer hd;

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
    } else {
      ucontext_t* _context = (ucontext_t*)UContext;

      // signal must be no re-entry here

      [[maybe_unused]] auto Previous = ThreadData.HostDeferredSignalPending.exchange(Signal, std::memory_order_relaxed);

      LOGMAN_THROW_A_FMT(!Previous, "Nested Host Deferred signal, {}", Previous);

      if (sigsetjmp(ThreadData.HostDeferredSignalJump, 1)) {
        // Host Deferred Delivery
        ThreadData.HostDeferredSignalPending = false;

        // Restore Deferred sigmask
        memcpy(&_context->uc_sigmask, &ThreadData.HostDeferredSigmask, SIGRTMAX / 8);
        goto DoHandleSignal;
      }

      // Store Deferred sigmask
      memcpy(&ThreadData.HostDeferredSigmask, &_context->uc_sigmask, SIGRTMAX / 8);

      // Block further signals on this thread
      sigfillset(&_context->uc_sigmask);
    }
  }
}
