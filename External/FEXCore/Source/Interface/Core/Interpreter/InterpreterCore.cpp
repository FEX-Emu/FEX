#include "Interface/Context/Context.h"

#include "Interface/Core/ArchHelpers/Arm64.h"
#include "Interface/Core/ArchHelpers/MContext.h"
#include "Interface/Core/Dispatcher/Dispatcher.h"
#include "Interface/Core/Interpreter/InterpreterClass.h"
#include <FEXCore/Config/Config.h>
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Core/SignalDelegator.h>
#include <FEXCore/Debug/InternalThreadState.h>
#include <FEXCore/Utils/LogManager.h>

#include <memory>
#include <signal.h>
#include <stdint.h>
#include <unordered_map>
#include <utility>

#include "InterpreterOps.h"

namespace FEXCore::IR {
  class IRListView;
  class RegisterAllocationData;
}

namespace FEXCore::CPU {
class CPUBackend;

static void InterpreterExecution(FEXCore::Core::CpuStateFrame *Frame) {
  auto Thread = Frame->Thread;

  auto LocalEntry = Thread->LocalIRCache.find(Thread->CurrentFrame->State.rip);

  InterpreterOps::InterpretIR(Thread, Thread->CurrentFrame->State.rip, LocalEntry->second.IR.get(), LocalEntry->second.DebugData.get());
}

InterpreterCore::InterpreterCore(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread)
  : CTX {ctx}
  , State {Thread} {

  CreateAsmDispatch(ctx, Thread);
}

void InterpreterCore::InitializeSignalHandlers(FEXCore::Context::Context *CTX) {
  CTX->SignalDelegation->RegisterHostSignalHandler(SignalDelegator::SIGNAL_FOR_PAUSE, [](FEXCore::Core::InternalThreadState *Thread, int Signal, void *info, void *ucontext) -> bool {
    InterpreterCore *Core = reinterpret_cast<InterpreterCore*>(Thread->CPUBackend.get());
    return Core->Dispatcher->HandleSignalPause(Signal, info, ucontext);
  }, true);

#ifdef _M_ARM_64
  CTX->SignalDelegation->RegisterHostSignalHandler(SIGBUS, [](FEXCore::Core::InternalThreadState *Thread, int Signal, void *info, void *ucontext) -> bool {
    return FEXCore::ArchHelpers::Arm64::HandleSIGBUS(true, Signal, info, ucontext);
  }, true);
#endif

  auto GuestSignalHandler = [](FEXCore::Core::InternalThreadState *Thread, int Signal, void *info, void *ucontext, GuestSigAction *GuestAction, stack_t *GuestStack) -> bool {
    InterpreterCore *Core = reinterpret_cast<InterpreterCore*>(Thread->CPUBackend.get());
    return Core->Dispatcher->HandleGuestSignal(Signal, info, ucontext, GuestAction, GuestStack);
  };

  for (uint32_t Signal = 0; Signal <= SignalDelegator::MAX_SIGNALS; ++Signal) {
    CTX->SignalDelegation->RegisterHostSignalHandlerForGuest(Signal, GuestSignalHandler);
  }
}

void *InterpreterCore::CompileCode(uint64_t Entry, [[maybe_unused]] FEXCore::IR::IRListView const *IR, [[maybe_unused]] FEXCore::Core::DebugData *DebugData, FEXCore::IR::RegisterAllocationData *RAData) {
  return reinterpret_cast<void*>(InterpreterExecution);
}

std::unique_ptr<CPUBackend> CreateInterpreterCore(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread) {
  return std::make_unique<InterpreterCore>(ctx, Thread);
}

void InitializeInterpreterSignalHandlers(FEXCore::Context::Context *CTX) {
  InterpreterCore::InitializeSignalHandlers(CTX);
}

}
