#pragma once

#include <FEXCore/Utils/CompilerDefs.h>
#include <FEXCore/fextl/vector.h>

#include <array>
#include <cstdint>
#include <functional>
#include <utility>
#include <signal.h>
#include <stddef.h>

namespace FEXCore {
namespace Core {
  struct InternalThreadState;

  enum class SignalEvent {
    Nothing, // If the guest uses our signal we need to know it was errant on our end
    Pause,
    Stop,
    Return,
    ReturnRT,
  };

  enum SignalNumber {
#ifndef _WIN32
    FAULT_SIGSEGV = SIGSEGV,
    FAULT_SIGTRAP = SIGTRAP,
    FAULT_SIGILL = SIGILL,
#else
    FAULT_SIGSEGV = 11,
    FAULT_SIGTRAP = 5,
    FAULT_SIGILL = 4,
#endif
  };
}
  using HostSignalDelegatorFunction = std::function<bool(FEXCore::Core::InternalThreadState *Thread, int Signal, void *info, void *ucontext)>;

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
     * @brief Registers a signal handler for the host to handle a signal
     *
     * It's a process level signal handler so one must be careful
     */
    void RegisterHostSignalHandler(int Signal, HostSignalDelegatorFunction Func, bool Required);

    // Called from the thunk handler to handle the signal
    void HandleSignal(int Signal, void *Info, void *UContext);

    /**
     * @brief Check to ensure the XID handler is still set to the FEX handler
     *
     * On a new thread GLIBC will set the XID handler underneath us.
     * After the first thread is created check this.
     */
    virtual void CheckXIDHandler() = 0;

    constexpr static size_t MAX_SIGNALS {64};

    // Use the last signal just so we are less likely to ever conflict with something that the guest application is using
    // 64 is used internally by Valgrind
    constexpr static size_t SIGNAL_FOR_PAUSE {63};

    struct SignalDelegatorConfig {
      bool StaticRegisterAllocation{};
      bool SupportsAVX{};

      // Dispatcher information
      uint64_t DispatcherBegin;
      uint64_t DispatcherEnd;

      // Dispatcher entrypoint.
      uint64_t AbsoluteLoopTopAddressFillSRA{};

      // Signal return pointers.
      uint64_t SignalHandlerReturnAddress{};
      uint64_t SignalHandlerReturnAddressRT{};

      // Pause handlers.
      uint64_t PauseReturnInstruction{};
      uint64_t ThreadPauseHandlerAddressSpillSRA{};
      uint64_t ThreadPauseHandlerAddress{};

      // Stop handlers.
      uint64_t ThreadStopHandlerAddressSpillSRA;
      uint64_t ThreadStopHandlerAddress{};

      // SRA information.
      uint16_t SRAGPRCount;
      uint16_t SRAFPRCount;

      // SRA index mapping.
      uint8_t SRAGPRMapping[16];
      uint8_t SRAFPRMapping[16];
    };

    void SetConfig(const SignalDelegatorConfig& _Config) {
      Config = _Config;
    }

    const SignalDelegatorConfig &GetConfig() const {
      return Config;
    }

    /**
     * @brief Signals a thread with a specific core event.
     *
     * @param Thread Which thread to signal.
     * @param Event Which event to signal the event with.
     */
    virtual void SignalThread(FEXCore::Core::InternalThreadState *Thread, Core::SignalEvent Event) = 0;

  protected:
    SignalDelegatorConfig Config;

    virtual FEXCore::Core::InternalThreadState *GetTLSThread() = 0;
    virtual void HandleGuestSignal(FEXCore::Core::InternalThreadState *Thread, int Signal, void *info, void *ucontext) = 0;

    /**
     * @brief Registers a signal handler for the host to handle a signal
     *
     * It's a process level signal handler so one must be careful
     */
    virtual void FrontendRegisterHostSignalHandler(int Signal, HostSignalDelegatorFunction Func, bool Required) = 0;
    virtual void FrontendRegisterFrontendHostSignalHandler(int Signal, HostSignalDelegatorFunction Func, bool Required) = 0;

  private:
    struct HostSignalHandler {
      fextl::vector<FEXCore::HostSignalDelegatorFunction> Handlers{};
      FEXCore::HostSignalDelegatorFunction FrontendHandler{};
    };
    std::array<HostSignalHandler, MAX_SIGNALS + 1> HostHandlers{};

  protected:
    void SetHostSignalHandler(int Signal, HostSignalDelegatorFunction Func, bool Required) {
      HostHandlers[Signal].Handlers.push_back(std::move(Func));
    }
    void SetFrontendHostSignalHandler(int Signal, HostSignalDelegatorFunction Func, bool Required) {
      HostHandlers[Signal].FrontendHandler = std::move(Func);
    }
  };
}
