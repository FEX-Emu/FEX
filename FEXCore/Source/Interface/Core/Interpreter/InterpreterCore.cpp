#include "Interface/Context/Context.h"

#include "Interface/Core/Dispatcher/Dispatcher.h"
#include "Interface/Core/Interpreter/InterpreterClass.h"
#include <FEXCore/Config/Config.h>
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Debug/InternalThreadState.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/MathUtils.h>
#include <FEXCore/fextl/memory.h>

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

CPUBackend::CompiledCode InterpreterCore::CompileCode(uint64_t Entry, [[maybe_unused]] FEXCore::IR::IRListView const *IR, [[maybe_unused]] FEXCore::Core::DebugData *DebugData, FEXCore::IR::RegisterAllocationData *RAData, bool GDBEnabled) {

  const auto IRSize = AlignUp(IR->GetInlineSize(), 16);
  const auto MaxSize = IRSize + Dispatcher::MaxInterpreterTrampolineSize + GDBEnabled * Dispatcher::MaxGDBPauseCheckSize;

  if ((BufferUsed + MaxSize) > CurrentCodeBuffer->Size) {
    static_cast<Context::ContextImpl*>(ThreadState->CTX)->ClearCodeCache(ThreadState);
  }

  CPUBackend::CompiledCode CodeData{};

  const auto BufferStartOffset = BufferUsed;
  CodeData.BlockBegin = CodeData.BlockEntry = CurrentCodeBuffer->Ptr + BufferStartOffset;

  auto DestBuffer = CodeData.BlockBegin;

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

  CodeData.Size = BufferUsed - BufferStartOffset;

  return CodeData;
}

void InterpreterCore::ClearCache() {
  // Calling this one is needed to setup the initial CurrentCodeBuffer
  [[maybe_unused]] auto CodeBuffer = GetEmptyCodeBuffer();
  BufferUsed = 0;
}

fextl::unique_ptr<CPUBackend> CreateInterpreterCore(FEXCore::Context::ContextImpl *ctx, FEXCore::Core::InternalThreadState *Thread) {
  return fextl::make_unique<InterpreterCore>(ctx->Dispatcher.get(), Thread);
}

CPUBackendFeatures GetInterpreterBackendFeatures() {
  return CPUBackendFeatures {
    .SupportsVTBL2 = true,
  };
}
}
