#include "Common/MathUtils.h"
#include "Common/SoftFloat.h"
#include "Interface/Context/Context.h"

#ifdef _M_ARM_64
#include "Interface/Core/ArchHelpers/Arm64.h"
#endif
#include "Interface/Core/LookupCache.h"
#include "Interface/Core/DebugData.h"
#include "Interface/Core/InternalThreadState.h"
#include "Interface/Core/Interpreter/InterpreterClass.h"
#include <FEXCore/Utils/LogManager.h>

#include <FEXCore/Core/CPUBackend.h>
#include <FEXCore/HLE/SyscallHandler.h>
#include <FEXCore/IR/IR.h>
#include <FEXCore/IR/IntrusiveIRList.h>

#include "Interface/HLE/Thunks/Thunks.h"

#include <atomic>
#include <cmath>
#include <limits>
#include <vector>
#ifdef _M_X86_64
#include <xmmintrin.h>
#endif

#include "InterpreterOps.h"

namespace FEXCore::CPU {

static void InterpreterExecution(FEXCore::Core::InternalThreadState *Thread) {
  auto IR = Thread->IRLists.find(Thread->State.State.rip)->second.get();
  
  FEXCore::Core::DebugData *DebugData = nullptr;
  
  // DebugData is only used in debug builds
  #ifndef NDEBUG
  DebugData = Thread->DebugData.find(Thread->State.State.rip)->second.get();
  #endif

  InterpreterOps::InterpretIR(Thread, IR, DebugData);
}


bool InterpreterCore::HandleSIGBUS(int Signal, void *info, void *ucontext) {
#ifdef _M_ARM_64
  ucontext_t* _context = (ucontext_t*)ucontext;
  mcontext_t* _mcontext = &_context->uc_mcontext;
  uint32_t *PC = (uint32_t*)_mcontext->pc;
  uint32_t Instr = PC[0];
  if ((Instr & FEXCore::ArchHelpers::Arm64::CASPAL_MASK) == FEXCore::ArchHelpers::Arm64::CASPAL_INST) { // CASPAL
    if (FEXCore::ArchHelpers::Arm64::HandleCASPAL(_mcontext, info, Instr)) {
      // Skip this instruction now
      _mcontext->pc += 4;
      return true;
    }
    else {
      LogMan::Msg::E("Unhandled JIT SIGBUS CASPAL: PC: %p Instruction: 0x%08x\n", PC, PC[0]);
      return false;
    }
  }
  else if ((Instr & FEXCore::ArchHelpers::Arm64::CASAL_MASK) == FEXCore::ArchHelpers::Arm64::CASAL_INST) { // CASAL
    if (FEXCore::ArchHelpers::Arm64::HandleCASAL(_mcontext, info, Instr)) {
      // Skip this instruction now
      _mcontext->pc += 4;
      return true;
    }
    else {
      LogMan::Msg::E("Unhandled JIT SIGBUS CASAL: PC: %p Instruction: 0x%08x\n", PC, PC[0]);
      return false;
    }
  }
#endif
  return false;
}

InterpreterCore::InterpreterCore(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread, bool CompileThread)
  : CTX {ctx}
  , State {Thread} {
  // Grab our space for temporary data

  if (!CompileThread &&
      CTX->Config.Core == FEXCore::Config::CONFIG_INTERPRETER) {
    CreateAsmDispatch(ctx, Thread);
    CTX->SignalDelegation->RegisterHostSignalHandler(SignalDelegator::SIGNAL_FOR_PAUSE, [](FEXCore::Core::InternalThreadState *Thread, int Signal, void *info, void *ucontext) -> bool {
      InterpreterCore *Core = reinterpret_cast<InterpreterCore*>(Thread->CPUBackend.get());
      return Core->HandleSignalPause(Signal, info, ucontext);
    });

    CTX->SignalDelegation->RegisterHostSignalHandler(SIGBUS, [](FEXCore::Core::InternalThreadState *Thread, int Signal, void *info, void *ucontext) -> bool {
      InterpreterCore *Core = reinterpret_cast<InterpreterCore*>(Thread->CPUBackend.get());
      return Core->HandleSIGBUS(Signal, info, ucontext);
    });

    auto GuestSignalHandler = [](FEXCore::Core::InternalThreadState *Thread, int Signal, void *info, void *ucontext, GuestSigAction *GuestAction, stack_t *GuestStack) -> bool {
      InterpreterCore *Core = reinterpret_cast<InterpreterCore*>(Thread->CPUBackend.get());
      return Core->HandleGuestSignal(Signal, info, ucontext, GuestAction, GuestStack);
    };

    for (uint32_t Signal = 0; Signal < SignalDelegator::MAX_SIGNALS; ++Signal) {
      CTX->SignalDelegation->RegisterHostSignalHandlerForGuest(Signal, GuestSignalHandler);
    }
  }
}


InterpreterCore::~InterpreterCore() {
  DeleteAsmDispatch();
}


void *InterpreterCore::CompileCode([[maybe_unused]] FEXCore::IR::IRListView const *IR, [[maybe_unused]] FEXCore::Core::DebugData *DebugData, FEXCore::IR::RegisterAllocationData *RAData) {
  return reinterpret_cast<void*>(InterpreterExecution);
}

FEXCore::CPU::CPUBackend *CreateInterpreterCore(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread, bool CompileThread) {
  return new InterpreterCore(ctx, Thread, CompileThread);
}

}
