#pragma once

#include <atomic>
#include <functional>
#include <mutex>
#include <signal.h>

#include <FEXCore/Core/SignalDelegator.h>

namespace FEXCore {
namespace Context {
  struct Context;
}
namespace Core {
  struct InternalThreadState;
}

  class SignalDelegator {
  public:
    struct __attribute__((packed)) GuestSAMask {
      uint64_t Val;
    };

    struct __attribute__((packed)) GuestSigAction {
      union {
        void (*handler)(int);
        void (*sigaction)(int, siginfo_t *, void*);
      } sigaction_handler;

      uint64_t sa_flags;
      void (*restorer)(void);
      GuestSAMask sa_mask;
    };

    // Returns true if the host handled the signal
    // Arguments are the same as sigaction handler
    using HostSignalDelegatorFunctionForGuest = std::function<bool(FEXCore::Core::InternalThreadState *Thread, int Signal, void *info, void *ucontext, GuestSigAction *GuestAction, stack_t *GuestStack)>;
    SignalDelegator();

    /**
     * @brief Registers an emulated thread's object to a TLS object
     *
     * Required to know which thread has received the signal when it occurs
     */
    static void RegisterTLSState(FEXCore::Core::InternalThreadState *Thread);
    static void UninstallTLSState(FEXCore::Core::InternalThreadState *Thread);

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

    static bool BlockSignal(int Signal);
    static bool UnblockSignal(int Signal);

    /**
     * @brief Registers a signal handler for the host to handle a signal
     *
     * It's a process level signal handler so one must be careful
     */
    void RegisterHostSignalHandler(int Signal, HostSignalDelegatorFunction Func);
    void RegisterFrontendHostSignalHandler(int Signal, HostSignalDelegatorFunction Func);

    /**
     * @brief Registers a signal handler for the host to handle a signal specifically for guest handling
     *
     * It's a process level signal handler so one must be careful
     */
    void RegisterHostSignalHandlerForGuest(int Signal, HostSignalDelegatorFunctionForGuest Func);

    /**
     * @brief Allows the guest to register a signal handler that is run after the host attempts to resolve the handler first
     */
    uint64_t RegisterGuestSignalHandler(int Signal, const GuestSigAction *Action, struct GuestSigAction *OldAction);

    uint64_t RegisterGuestSigAltStack(const stack_t *ss, stack_t *old_ss);

    uint64_t GuestSigProcMask(int how, const uint64_t *set, uint64_t *oldset);
    uint64_t GuestSigPending(uint64_t *set, size_t sigsetsize);
    uint64_t GuestSigSuspend(uint64_t *set, size_t sigsetsize);

    // Called from the thunk handler to handle the signal
    void HandleSignal(int Signal, void *Info, void *UContext);

    void SetCurrentSignal(uint32_t Signal);

    constexpr static size_t MAX_SIGNALS {64};

    // Use the last signal just so we are less likely to ever conflict with something that the guest application is using
    // 64 is used internally by Valgrind
    constexpr static size_t SIGNAL_FOR_PAUSE {63};

  private:
    enum DefaultBehaviour {
      DEFAULT_TERM,
      // Core dump based signals are supposed to have a coredump appear
      // For FEX's behaviour we don't really care right now
      DEFAULT_COREDUMP = DEFAULT_TERM,
      DEFAULT_IGNORE,
    };

    struct SignalHandler {
      std::atomic<bool> Installed{};
      struct sigaction HostAction{};
      struct sigaction OldAction{};
      HostSignalDelegatorFunction Handler{};
      HostSignalDelegatorFunction FrontendHandler{};
      HostSignalDelegatorFunctionForGuest GuestHandler{};
      GuestSigAction GuestAction{};
      DefaultBehaviour DefaultBehaviour {DEFAULT_TERM};
    };

    SignalHandler HostHandlers[MAX_SIGNALS + 1]{};
    bool InstallHostThunk(int Signal);
    void UpdateHostThunk(int Signal);

    std::mutex HostDelegatorMutex;
    std::mutex GuestDelegatorMutex;

    static void MaskSignals(int how, int Signal = -1);
  };
}
