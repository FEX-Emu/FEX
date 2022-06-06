#pragma once

#include <FEXCore/Core/CPUBackend.h>

#include "Interface/Context/Context.h"
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

namespace FEXCore::CPU {

struct DispatcherConfig {
  bool ExecuteBlocksWithCall = false;
  uintptr_t ExitFunctionLink = 0;
  uintptr_t ExitFunctionLinkThis = 0;
  bool StaticRegisterAssignment = false;
};

class Dispatcher {
public:
  virtual ~Dispatcher() = default;
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
  uint64_t UnimplementedInstructionAddress{};
  uint64_t OverflowExceptionInstructionAddress{};

  uint64_t PauseReturnInstruction{};

  /**  @} */

  struct SynchronousFaultDataStruct {
    bool FaultToTopAndGeneratedException{};
    uint32_t TrapNo;
    uint32_t err_code;
    uint32_t si_code;
  } SynchronousFaultData;

  uint64_t Start{};
  uint64_t End{};

  bool HandleGuestSignal(int Signal, void *info, void *ucontext, GuestSigAction *GuestAction, stack_t *GuestStack);
  bool HandleSIGILL(int Signal, void *info, void *ucontext);
  bool HandleSignalPause(int Signal, void *info, void *ucontext);

  void RegisterCodeBuffer(uint8_t* start, size_t size) {
    CodeBuffers.emplace_back(reinterpret_cast<uint64_t>(start),
        reinterpret_cast<uint64_t>(start + size));
  }

  void RemoveCodeBuffer(uint8_t* start);

  bool IsAddressInJITCode(uint64_t Address, bool IncludeDispatcher = true) const;
  bool IsAddressInDispatcher(uint64_t Address) const {
    return Address >= Start && Address < End;
  }

protected:
  Dispatcher(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread)
    : CTX {ctx}
    , ThreadState {Thread} {}

  ArchHelpers::Context::ContextBackup* StoreThreadState(int Signal, void *ucontext);
  void RestoreThreadState(void *ucontext);
  std::stack<uint64_t, std::vector<uint64_t>> SignalFrames;

  bool SRAEnabled = false;
  virtual void SpillSRA(void *ucontext, uint32_t IgnoreMask) {}

  FEXCore::Context::Context *CTX;
  FEXCore::Core::InternalThreadState *ThreadState;

  static void SleepThread(FEXCore::Context::Context *ctx, FEXCore::Core::CpuStateFrame *Frame);

  static uint64_t GetCompileBlockPtr();

private:
  std::vector<std::tuple<uint64_t, uint64_t>> CodeBuffers; // Start, End
};

}
