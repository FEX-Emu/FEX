#pragma once

#include <atomic>
#include <functional>
#include <mutex>
#include <signal.h>

namespace FEXCore {
namespace Context {
  struct Context;
}
namespace Core {
  struct InternalThreadState;
}

  class SignalDelegator {
  public:
    // Returns true if the host handled the signal
    // Arguments are the same as sigaction handler
    using HostSignalDelegatorFunction = std::function<bool(FEXCore::Core::InternalThreadState *Thread, int Signal, void *info, void *ucontext)>;
    SignalDelegator(FEXCore::Context::Context *ctx);

    /**
     * @brief Registers an emulated thread's object to a TLS object
     *
     * Required to know which thread has received the signal when it occurs
     */
    static void RegisterTLSState(FEXCore::Core::InternalThreadState *Thread);

    /**
     * @brief Masks a thread from receiving any signals that aren't synchronous
     *
     * Any helper thread that is generated needs to call this routine to ensure it doesn't receive errant signals to handle
     */
    static void MaskThreadSignals();

    /**
     * @brief Reallows a thread to receive signals
     *
     * Must be careful with this if you're planning on having signals enabled while emulation is running
     */
    static void ResetThreadSignalMask();

    /**
     * @brief Registers a signal handler for the host to handle a signal
     *
     * It's a process level signal handler so one must be careful
     */
    void RegisterHostSignalHandler(int Signal, HostSignalDelegatorFunction Func);
    /**
     * @brief Allows the guest to register a signal handler that is run after the host attempts to resolve the handler first
     */
    uint64_t RegisterGuestSignalHandler(int Signal, const struct sigaction *Action, struct sigaction *OldAction);

    // Called from the thunk handler to handle the signal
    void HandleSignal(int Signal, void *Info, void *UContext);

  private:
    FEXCore::Context::Context *CTX;

    constexpr static size_t MAX_SIGNALS {64};
    struct SignalHandler {
      std::atomic<bool> Installed{};
      struct sigaction OldAction{};
      HostSignalDelegatorFunction Handler{};
      struct sigaction GuestAction{};
    };

    SignalHandler HostHandlers[MAX_SIGNALS]{};
    void InstallHostThunk(int Signal);

    std::mutex HostDelegatorMutex;
    std::mutex GuestDelegatorMutex;

    static void MaskSignals(int how);
  };
}
