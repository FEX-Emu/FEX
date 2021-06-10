#pragma once

#include <FEXCore/Utils/CompilerDefs.h>

#include <cstdint>
#include <functional>
#include <signal.h>

namespace FEXCore {
namespace Core {
  struct InternalThreadState;
}
  struct FEX_PACKED GuestSAMask {
    uint64_t Val;
  };

  struct FEX_PACKED GuestSigAction {
    union {
      void (*handler)(int);
      void (*sigaction)(int, siginfo_t *, void*);
    } sigaction_handler;

    uint64_t sa_flags;
    void (*restorer)(void);
    GuestSAMask sa_mask;
  };

  using HostSignalDelegatorFunction = std::function<bool(FEXCore::Core::InternalThreadState *Thread, int Signal, void *info, void *ucontext)>;
  using HostSignalDelegatorFunctionForGuest = std::function<bool(FEXCore::Core::InternalThreadState *Thread, int Signal, void *info, void *ucontext, GuestSigAction *GuestAction, stack_t *GuestStack)>;

  class SignalDelegator {
  public:
    virtual ~SignalDelegator() = default;

    /**
     * @brief Registers an emulated thread's object to a TLS object
     *
     * Required to know which thread has received the signal when it occurs
     */
    virtual void RegisterTLSState(FEXCore::Core::InternalThreadState *Thread) = 0;
    virtual void UninstallTLSState(FEXCore::Core::InternalThreadState *Thread) = 0;

    /**
     * @brief Masks a thread from receiving any signals that aren't synchronous
     *
     * Any helper thread that is generated needs to call this routine to ensure it doesn't receive errant signals to handle
     */
    virtual void MaskThreadSignals() = 0;

    virtual void SetCurrentSignal(uint32_t Signal) = 0;

    /**
     * @brief Registers a signal handler for the host to handle a signal
     *
     * It's a process level signal handler so one must be careful
     */
    virtual void RegisterHostSignalHandler(int Signal, HostSignalDelegatorFunction Func) = 0;
    virtual void RegisterFrontendHostSignalHandler(int Signal, HostSignalDelegatorFunction Func) = 0;

    /**
     * @brief Registers a signal handler for the host to handle a signal specifically for guest handling
     *
     * It's a process level signal handler so one must be careful
     */
    virtual void RegisterHostSignalHandlerForGuest(int Signal, HostSignalDelegatorFunctionForGuest Func) = 0;

    constexpr static size_t MAX_SIGNALS {64};

    // Use the last signal just so we are less likely to ever conflict with something that the guest application is using
    // 64 is used internally by Valgrind
    constexpr static size_t SIGNAL_FOR_PAUSE {63};
  };
}
