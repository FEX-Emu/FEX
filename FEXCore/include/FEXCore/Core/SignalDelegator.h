// SPDX-License-Identifier: MIT
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

    struct SignalDelegatorConfig {
      bool SupportsAVX{};

      // Dispatcher information
      uint64_t DispatcherBegin;
      uint64_t DispatcherEnd;

      // Dispatcher entrypoint.
      uint64_t AbsoluteLoopTopAddress{};
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
  };
}
