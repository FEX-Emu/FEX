/*
$info$
tags: LinuxSyscalls|common
$end_info$
*/


#pragma once

#include <array>
#include <atomic>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <mutex>

#include <FEXCore/Core/SignalDelegator.h>

namespace FEXCore {
namespace Core {
  struct InternalThreadState;
}
}

namespace FEX::HLE {
  class SignalDelegator final : public FEXCore::SignalDelegator {
  public:
    void *operator new(size_t size) {
      return FEXCore::Allocator::malloc(size);
    }

    void operator delete(void *ptr) {
      return FEXCore::Allocator::free(ptr);
    }

    // Returns true if the host handled the signal
    // Arguments are the same as sigaction handler
    SignalDelegator();
    ~SignalDelegator() override;

    /**
     * @brief Registers a signal handler for the host to handle a signal specifically for guest handling
     *
     * It's a process level signal handler so one must be careful
     */
    void RegisterHostSignalHandlerForGuest(int Signal, FEXCore::HostSignalDelegatorFunctionForGuest Func) override;

    /**
     * @name These functions are all for Linux signal emulation
     * @{ */
      /**
       * @brief Allows the guest to register a signal handler that is run after the host attempts to resolve the handler first
       */
      uint64_t RegisterGuestSignalHandler(int Signal, const FEXCore::GuestSigAction *Action, struct FEXCore::GuestSigAction *OldAction);

      uint64_t RegisterGuestSigAltStack(const stack_t *ss, stack_t *old_ss);

      uint64_t GuestSigProcMask(int how, const uint64_t *set, uint64_t *oldset);
      uint64_t GuestSigPending(uint64_t *set, size_t sigsetsize);
      uint64_t GuestSigSuspend(uint64_t *set, size_t sigsetsize);
      uint64_t GuestSigTimedWait(uint64_t *set, siginfo_t *info, const struct timespec *timeout, size_t sigsetsize);
      uint64_t GuestSignalFD(int fd, const uint64_t *set, size_t sigsetsize , int flags);
    /**  @} */

      void CheckXIDHandler() override;

      void UninstallHostHandler(int Signal);
  protected:
    // Called from the thunk handler to handle the signal
    void HandleGuestSignal(FEXCore::Core::InternalThreadState *Thread, int Signal, void *Info, void *UContext) override;

    void RegisterFrontendTLSState(FEXCore::Core::InternalThreadState *Thread) override;
    void UninstallFrontendTLSState(FEXCore::Core::InternalThreadState *Thread) override;

    /**
     * @brief Registers a signal handler for the host to handle a signal
     *
     * It's a process level signal handler so one must be careful
     */
    void FrontendRegisterHostSignalHandler(int Signal, FEXCore::HostSignalDelegatorFunction Func, bool Required) override;
    void FrontendRegisterFrontendHostSignalHandler(int Signal, FEXCore::HostSignalDelegatorFunction Func, bool Required) override;

  private:
    enum DefaultBehaviour {
      DEFAULT_TERM,
      // Core dump based signals are supposed to have a coredump appear
      // For FEX's behaviour we don't really care right now
      DEFAULT_COREDUMP = DEFAULT_TERM,
      DEFAULT_IGNORE,
    };

    struct kernel_sigaction {
      union {
        void (*handler)(int);
        void (*sigaction)(int, siginfo_t*, void*);
      };

      uint64_t sa_flags;

      void (*restorer)();
      uint64_t sa_mask;
    };

    struct SignalHandler {
      std::atomic<bool> Installed{};
      std::atomic<bool> Required{};
      kernel_sigaction HostAction{};
      kernel_sigaction OldAction{};
      FEXCore::HostSignalDelegatorFunctionForGuest GuestHandler{};
      FEXCore::GuestSigAction GuestAction{};
      DefaultBehaviour DefaultBehaviour {DEFAULT_TERM};
    };

    std::array<SignalHandler, MAX_SIGNALS + 1> HostHandlers{};
    bool InstallHostThunk(int Signal);
    bool UpdateHostThunk(int Signal);

    std::mutex HostDelegatorMutex;
    std::mutex GuestDelegatorMutex;
  };
}
