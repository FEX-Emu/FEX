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
#include <FEXCore/Utils/MathUtils.h>

#include <memory>
#include <signal.h>
#include <stdint.h>
#include <utility>

#include "InterpreterOps.h"

#if defined(_M_X86_64)
  #include "Interface/Core/Dispatcher/X86Dispatcher.h"
#elif defined(_M_ARM_64)
  #include "Interface/Core/Dispatcher/Arm64Dispatcher.h"
#else
  #error missing arch
#endif

static constexpr size_t INITIAL_CODE_SIZE = 1024 * 1024 * 16;
static constexpr size_t MAX_CODE_SIZE = 1024 * 1024 * 128;

namespace FEXCore::IR {
  class IRListView;
  class RegisterAllocationData;
}


namespace FEXCore::CPU {

InterpreterCore::InterpreterCore(Dispatcher *Dispatcher, FEXCore::Core::InternalThreadState *Thread)
  : CPUBackend(Thread, INITIAL_CODE_SIZE, MAX_CODE_SIZE)
  , Dispatch(Dispatcher)
  {

  auto &Interpreter = Thread->CurrentFrame->Pointers.Interpreter;

  Interpreter.FragmentExecuter = reinterpret_cast<uint64_t>(&InterpreterOps::InterpretIR);

  ClearCache();
}

void InterpreterCore::InitializeSignalHandlers(FEXCore::Context::Context *CTX) {
  CTX->SignalDelegation->RegisterHostSignalHandler(SIGILL, [](FEXCore::Core::InternalThreadState *Thread, int Signal, void *info, void *ucontext) -> bool {
    return Thread->CTX->Dispatcher->HandleSIGILL(Thread, Signal, info, ucontext);
  }, true);

#ifdef _M_ARM_64
  CTX->SignalDelegation->RegisterHostSignalHandler(SIGBUS, [](FEXCore::Core::InternalThreadState *Thread, int Signal, void *info, void *ucontext) -> bool {
    return FEXCore::ArchHelpers::Arm64::HandleSIGBUS(true, Signal, info, ucontext);
  }, true);
#endif
}

void *InterpreterCore::CompileCode(uint64_t Entry, [[maybe_unused]] FEXCore::IR::IRListView const *IR, [[maybe_unused]] FEXCore::Core::DebugData *DebugData, FEXCore::IR::RegisterAllocationData *RAData, bool GDBEnabled) {

  const auto IRSize = AlignUp(IR->GetInlineSize(), 16);
  const auto MaxSize = IRSize + Dispatcher::MaxInterpreterTrampolineSize + GDBEnabled * Dispatcher::MaxGDBPauseCheckSize;

  if ((BufferUsed + MaxSize) > CurrentCodeBuffer->Size) {
    ThreadState->CTX->ClearCodeCache(ThreadState);
  }

  const auto BufferStart = CurrentCodeBuffer->Ptr + BufferUsed;

  auto DestBuffer = BufferStart;

  if (GDBEnabled) {
    const auto GDBSize = Dispatch->GenerateGDBPauseCheck(DestBuffer, Entry);
    DestBuffer += GDBSize;
    BufferUsed += GDBSize;
  }

  const auto TrampolineSize = Dispatch->GenerateInterpreterTrampoline(DestBuffer);
  DestBuffer += TrampolineSize;
  BufferUsed += TrampolineSize;


  IR->Serialize(DestBuffer);
  DestBuffer += IRSize;
  BufferUsed += IRSize;

  return BufferStart;
}

void InterpreterCore::ClearCache() {
  // Calling this one is needed to setup the initial CurrentCodeBuffer
  [[maybe_unused]] auto CodeBuffer = GetEmptyCodeBuffer();
  BufferUsed = 0;
}

std::unique_ptr<CPUBackend> CreateInterpreterCore(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread) {
  return std::make_unique<InterpreterCore>(ctx->Dispatcher.get(), Thread);
}

void InitializeInterpreterSignalHandlers(FEXCore::Context::Context *CTX) {
  InterpreterCore::InitializeSignalHandlers(CTX);
}

CPUBackendFeatures GetInterpreterBackendFeatures() {
  return CPUBackendFeatures { };
}
}
