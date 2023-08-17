#pragma once

#include <FEXCore/Core/CPUBackend.h>
#include <FEXCore/fextl/memory.h>

#include <cstdint>
#include <signal.h>
#include <stddef.h>
#include <stack>
#include <tuple>

namespace FEXCore {
struct GuestSigAction;
}

namespace FEXCore::Core {
struct CpuStateFrame;
struct InternalThreadState;
}

namespace FEXCore::Context {
class ContextImpl;
}

namespace FEXCore::CPU {

struct DispatcherConfig {
  bool StaticRegisterAllocation = false;
};

class Dispatcher {
public:
  virtual ~Dispatcher() = default;

  /**
   * @name Dispatch Helper functions
   * @{ */
  uint64_t ThreadStopHandlerAddress{};
  uint64_t ThreadStopHandlerAddressSpillSRA{};
  uint64_t AbsoluteLoopTopAddress{};
  uint64_t AbsoluteLoopTopAddressFillSRA{};
  uint64_t ThreadPauseHandlerAddress{};
  uint64_t ThreadPauseHandlerAddressSpillSRA{};
  uint64_t ExitFunctionLinkerAddress{};
  uint64_t SignalHandlerReturnAddress{};
  uint64_t SignalHandlerReturnAddressRT{};
  uint64_t GuestSignal_SIGILL{};
  uint64_t GuestSignal_SIGTRAP{};
  uint64_t GuestSignal_SIGSEGV{};
  uint64_t IntCallbackReturnAddress{};

  uint64_t PauseReturnInstruction{};

  /**  @} */

  uint64_t Start{};
  uint64_t End{};

  virtual void InitThreadPointers(FEXCore::Core::InternalThreadState *Thread) = 0;

  // These are across all arches for now
  static constexpr size_t MaxGDBPauseCheckSize = 128;
  static constexpr size_t MaxInterpreterTrampolineSize = 128;

  virtual size_t GenerateGDBPauseCheck(uint8_t *CodeBuffer, uint64_t GuestRIP) = 0;
  virtual size_t GenerateInterpreterTrampoline(uint8_t *CodeBuffer) = 0;

  static fextl::unique_ptr<Dispatcher> CreateX86(FEXCore::Context::ContextImpl *CTX, const DispatcherConfig &Config);
  static fextl::unique_ptr<Dispatcher> CreateArm64(FEXCore::Context::ContextImpl *CTX, const DispatcherConfig &Config);

  virtual void ExecuteDispatch(FEXCore::Core::CpuStateFrame *Frame) {
    DispatchPtr(Frame);
  }

  virtual void ExecuteJITCallback(FEXCore::Core::CpuStateFrame *Frame, uint64_t RIP) {
    CallbackPtr(Frame, RIP);
  }

  virtual uint16_t GetSRAGPRCount() const {
    return 0U;
  }

  virtual uint16_t GetSRAFPRCount() const {
    return 0U;
  }

  virtual void GetSRAGPRMapping(uint8_t Mapping[16]) const {
  }

  virtual void GetSRAFPRMapping(uint8_t Mapping[16]) const {
  }

  const DispatcherConfig& GetConfig() const { return config; }

protected:
  Dispatcher(FEXCore::Context::ContextImpl *ctx, const DispatcherConfig &Config)
    : CTX {ctx}
    , config {Config}
    {}

  FEXCore::Context::ContextImpl *CTX;
  DispatcherConfig config;

  static void SleepThread(FEXCore::Context::ContextImpl *ctx, FEXCore::Core::CpuStateFrame *Frame);

  static uint64_t GetCompileBlockPtr();

  using AsmDispatch = void(*)(FEXCore::Core::CpuStateFrame *Frame);
  using JITCallback = void(*)(FEXCore::Core::CpuStateFrame *Frame, uint64_t RIP);

  AsmDispatch DispatchPtr;
  JITCallback CallbackPtr;
};

}
