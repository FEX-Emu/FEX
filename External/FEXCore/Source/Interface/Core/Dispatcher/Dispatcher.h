#pragma once

#include <FEXCore/Core/CPUBackend.h>
#include <FEXCore/Core/SignalDelegator.h>

#include "Interface/Context/Context.h"
#include "Interface/Core/CodeBuffer.h"

#include <stack>

namespace FEXCore::CPU {

struct DispatcherConfig {
  bool ExecuteBlocksWithCall = false;
  uintptr_t ExitFunctionLink = 0;
  uintptr_t ExitFunctionLinkThis = 0;
  bool StaticRegisterAssignment = false;
};

class Dispatcher {
public:
  CPUBackend::AsmDispatch DispatchPtr;
  CPUBackend::JITCallback CallbackPtr;
  FEXCore::Context::Context::IntCallbackReturn ReturnPtr;

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
  uint64_t PauseReturnInstruction{};

  /**  @} */

  uint32_t SignalHandlerRefCounter{};

  uint64_t Start{};
  uint64_t End{};

  bool HandleGuestSignal(int Signal, void *info, void *ucontext, GuestSigAction *GuestAction, stack_t *GuestStack);
  bool HandleSIGILL(int Signal, void *info, void *ucontext);
  bool HandleSignalPause(int Signal, void *info, void *ucontext);

  void RegisterCodeBuffer(CodeBuffer& Buffer) {
    CodeBuffers.emplace_back(reinterpret_cast<uint64_t>(Buffer.Ptr),
        reinterpret_cast<uint64_t>(Buffer.Ptr + Buffer.Size));
  }

  void ForgetCodeBuffer(CodeBuffer& Buffer);

  bool IsAddressInJITCode(uint64_t Address, bool IncludeDispatcher = true);
  bool IsAddressInDispatcher(uint64_t Address) {
    return Address >= Start && Address < End;
  }

  virtual ~Dispatcher() {};

protected:
  Dispatcher(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread)
    : CTX {ctx}
    , ThreadState {Thread} {}

  static constexpr size_t MAX_DISPATCHER_CODE_SIZE = 4096;

  void StoreThreadState(int Signal, void *ucontext);
  void RestoreThreadState(void *ucontext);
  std::stack<uint64_t> SignalFrames;

  bool SRAEnabled = false;
  virtual void SpillSRA(void *ucontext) {}

  FEXCore::Context::Context *CTX;
  FEXCore::Core::InternalThreadState *ThreadState;

  static void SleepThread(FEXCore::Context::Context *ctx, FEXCore::Core::CpuStateFrame *Frame);

  static uint64_t GetCompileBlockPtr();

private:
  std::vector<std::tuple<uint64_t, uint64_t>> CodeBuffers; // Start, End
};

}