// SPDX-License-Identifier: MIT
#pragma once

#include "Interface/Core/ArchHelpers/Arm64Emitter.h"
#include "Interface/Core/Interpreter/InterpreterOps.h"

#include <FEXCore/Config/Config.h>
#include <FEXCore/fextl/memory.h>

#include <array>
#include <cstddef>
#include <cstdint>

namespace FEXCore {
struct GuestSigAction;
struct SignalDelegatorConfig;
} // namespace FEXCore

namespace FEXCore::Core {
struct CpuStateFrame;
struct InternalThreadState;
} // namespace FEXCore::Core

namespace FEXCore::Context {
class ContextImpl;
}

namespace FEXCore::CPU {

#define STATE_PTR(STATE_TYPE, FIELD) STATE.R(), offsetof(FEXCore::Core::STATE_TYPE, FIELD)

class Dispatcher final : public Arm64Emitter {
public:
  static fextl::unique_ptr<Dispatcher> Create(FEXCore::Context::ContextImpl* CTX);

  Dispatcher(FEXCore::Context::ContextImpl* ctx);
  ~Dispatcher();

  void InitThreadPointers(FEXCore::Core::InternalThreadState* Thread);

#ifdef VIXL_SIMULATOR
  void ExecuteDispatch(FEXCore::Core::CpuStateFrame* Frame);
  void ExecuteJITCallback(FEXCore::Core::CpuStateFrame* Frame, uint64_t RIP);
#else
  void ExecuteDispatch(FEXCore::Core::CpuStateFrame* Frame) {
    DispatchPtr(Frame, false);
  }

  void ExecuteJITCallback(FEXCore::Core::CpuStateFrame* Frame, uint64_t RIP) {
    CallbackPtr(Frame, RIP);
  }
#endif

  SignalDelegatorConfig MakeSignalDelegatorConfig() const;

protected:
  FEXCore::Context::ContextImpl* CTX;

  using AsmDispatch = void (*)(FEXCore::Core::CpuStateFrame* Frame, bool SingleInst);
  using JITCallback = void (*)(FEXCore::Core::CpuStateFrame* Frame, uint64_t RIP);

  AsmDispatch DispatchPtr;
  JITCallback CallbackPtr;
private:
  /**
   * @name Dispatch Helper functions
   * @{ */
  uint64_t ThreadStopHandlerAddress {};
  uint64_t ThreadStopHandlerAddressSpillSRA {};
  uint64_t AbsoluteLoopTopAddress {};
  uint64_t AbsoluteLoopTopAddressFillSRA {};
  uint64_t AbsoluteLoopTopAddressEnterEC {};
  uint64_t AbsoluteLoopTopAddressEnterECFillSRA {};
  uint64_t ThreadPauseHandlerAddress {};
  uint64_t ThreadPauseHandlerAddressSpillSRA {};
  uint64_t ExitFunctionLinkerAddress {};
  uint64_t SignalHandlerReturnAddress {};
  uint64_t SignalHandlerReturnAddressRT {};
  uint64_t GuestSignal_SIGILL {};
  uint64_t GuestSignal_SIGTRAP {};
  uint64_t GuestSignal_SIGSEGV {};

  uint64_t PauseReturnInstruction {};
  std::array<uint64_t, FallbackABI::FABI_UNKNOWN> ABIPointers {};
  /**  @} */

  uint64_t Start {};
  uint64_t End {};

  // Long division helpers
  uint64_t LUDIVHandlerAddress {};
  uint64_t LDIVHandlerAddress {};

  void EmitDispatcher();
  uint64_t GenerateABICall(FallbackABI ABI);

  FEX_CONFIG_OPT(DisableL2Cache, DISABLEL2CACHE);
};

} // namespace FEXCore::CPU
