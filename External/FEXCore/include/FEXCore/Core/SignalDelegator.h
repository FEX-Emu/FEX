#pragma once

#include <FEXCore/Utils/CompilerDefs.h>

#include <array>
#include <cstdint>
#include <functional>
#include <utility>
#include <signal.h>
#include <stddef.h>

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
    void RegisterTLSState(FEXCore::Core::InternalThreadState *Thread);
    void UninstallTLSState(FEXCore::Core::InternalThreadState *Thread);

    /**
     * @brief Masks a thread from receiving any signals that aren't synchronous
     *
     * Any helper thread that is generated needs to call this routine to ensure it doesn't receive errant signals to handle
     */
    void MaskThreadSignals();

    /**
     * @brief Registers a signal handler for the host to handle a signal
     *
     * It's a process level signal handler so one must be careful
     */
    void RegisterHostSignalHandler(int Signal, HostSignalDelegatorFunction Func, bool Required);
    void RegisterFrontendHostSignalHandler(int Signal, HostSignalDelegatorFunction Func, bool Required);

    /**
     * @brief Registers a signal handler for the host to handle a signal specifically for guest handling
     *
     * It's a process level signal handler so one must be careful
     */
    virtual void RegisterHostSignalHandlerForGuest(int Signal, HostSignalDelegatorFunctionForGuest Func) = 0;

    // Called from the thunk handler to handle the signal
    void HandleSignal(int Signal, void *Info, void *UContext);

    constexpr static size_t MAX_SIGNALS {64};

    // Use the last signal just so we are less likely to ever conflict with something that the guest application is using
    // 64 is used internally by Valgrind
    constexpr static size_t SIGNAL_FOR_PAUSE {63};

  protected:
    FEXCore::Core::InternalThreadState *GetTLSThread();
    virtual void HandleGuestSignal(FEXCore::Core::InternalThreadState *Thread, int Signal, void *info, void *ucontext) = 0;

    /**
     * @brief Registers an emulated thread's object to a TLS object
     *
     * Required to know which thread has received the signal when it occurs
     */
    virtual void RegisterFrontendTLSState(FEXCore::Core::InternalThreadState *Thread) = 0;
    virtual void UninstallFrontendTLSState(FEXCore::Core::InternalThreadState *Thread) = 0;

    /**
     * @brief Registers a signal handler for the host to handle a signal
     *
     * It's a process level signal handler so one must be careful
     */
    virtual void FrontendRegisterHostSignalHandler(int Signal, HostSignalDelegatorFunction Func, bool Required) = 0;
    virtual void FrontendRegisterFrontendHostSignalHandler(int Signal, HostSignalDelegatorFunction Func, bool Required) = 0;

  private:
    struct HostSignalHandler {
      std::vector<FEXCore::HostSignalDelegatorFunction> Handlers{};
      FEXCore::HostSignalDelegatorFunction FrontendHandler{};
    };
    std::array<HostSignalHandler, MAX_SIGNALS> HostHandlers{};

  protected:
    void SetHostSignalHandler(int Signal, HostSignalDelegatorFunction Func, bool Required) {
      HostHandlers[Signal - 1].Handlers.push_back(std::move(Func));
    }
    void SetFrontendHostSignalHandler(int Signal, HostSignalDelegatorFunction Func, bool Required) {
      HostHandlers[Signal - 1].FrontendHandler = std::move(Func);
    }
  };
}
