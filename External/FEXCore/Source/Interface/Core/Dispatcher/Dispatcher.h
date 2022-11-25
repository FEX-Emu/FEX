#pragma once

#include <FEXCore/Core/CPUBackend.h>
#include "Interface/Core/ArchHelpers/MContext.h"

#include <cstdint>
#include <signal.h>
#include <stddef.h>
#include <stack>
#include <tuple>
#include <vector>

namespace FEXCore {
struct GuestSigAction;
}

namespace FEXCore::Core {
struct CpuStateFrame;
struct InternalThreadState;
}

namespace FEXCore::Context {
struct Context;
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
  uint64_t SignalHandlerReturnAddressRT{};
  uint64_t SignalHandlerReturnAddress{};
  uint64_t GuestSignal_SIGILL{};
  uint64_t GuestSignal_SIGTRAP{};
  uint64_t GuestSignal_SIGSEGV{};
  uint64_t IntCallbackReturnAddress{};

  uint64_t PauseReturnInstruction{};

  /**  @} */

  uint64_t Start{};
  uint64_t End{};

  bool HandleGuestSignal(FEXCore::Core::InternalThreadState *Thread, int Signal, void *info, void *ucontext, GuestSigAction *GuestAction, stack_t *GuestStack);
  bool HandleSIGILL(FEXCore::Core::InternalThreadState *Thread, int Signal, void *info, void *ucontext);
  bool HandleSignalPause(FEXCore::Core::InternalThreadState *Thread, int Signal, void *info, void *ucontext);

  bool IsAddressInDispatcher(uint64_t Address) const {
    return Address >= Start && Address < End;
  }

  virtual void InitThreadPointers(FEXCore::Core::InternalThreadState *Thread) = 0;

  // These are across all arches for now
  static constexpr size_t MaxGDBPauseCheckSize = 128;
  static constexpr size_t MaxInterpreterTrampolineSize = 128;

  virtual size_t GenerateGDBPauseCheck(uint8_t *CodeBuffer, uint64_t GuestRIP) = 0;
  virtual size_t GenerateInterpreterTrampoline(uint8_t *CodeBuffer) = 0;

  static std::unique_ptr<Dispatcher> CreateX86(FEXCore::Context::Context *CTX, const DispatcherConfig &Config);
  static std::unique_ptr<Dispatcher> CreateArm64(FEXCore::Context::Context *CTX, const DispatcherConfig &Config);

  virtual void ExecuteDispatch(FEXCore::Core::CpuStateFrame *Frame) {
    DispatchPtr(Frame);
  }

  virtual void ExecuteJITCallback(FEXCore::Core::CpuStateFrame *Frame, uint64_t RIP) {
    CallbackPtr(Frame, RIP);
  }

protected:
  Dispatcher(FEXCore::Context::Context *ctx, const DispatcherConfig &Config)
    : CTX {ctx}
    , config {Config}
    {}

  ArchHelpers::Context::ContextBackup* StoreThreadState(FEXCore::Core::InternalThreadState *Thread, int Signal, void *ucontext);
  enum RestoreType {
    TYPE_RT,
    TYPE_NONRT,
    TYPE_PAUSE,
  };
  void RestoreThreadState(FEXCore::Core::InternalThreadState *Thread, void *ucontext, RestoreType Type);
  std::stack<uint64_t, std::vector<uint64_t>> SignalFrames;

  virtual void SpillSRA(FEXCore::Core::InternalThreadState *Thread, void *ucontext, uint32_t IgnoreMask) {}

  FEXCore::Context::Context *CTX;
  DispatcherConfig config;

  static void SleepThread(FEXCore::Context::Context *ctx, FEXCore::Core::CpuStateFrame *Frame);

  static uint64_t GetCompileBlockPtr();

  using AsmDispatch = void(*)(FEXCore::Core::CpuStateFrame *Frame);
  using JITCallback = void(*)(FEXCore::Core::CpuStateFrame *Frame, uint64_t RIP);

  AsmDispatch DispatchPtr;
  JITCallback CallbackPtr;
};

}
