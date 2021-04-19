#include "Common/MathUtils.h"
#include "Common/SoftFloat.h"
#include "Interface/Context/Context.h"

#include "Interface/Core/ArchHelpers/Arm64.h"
#include "Interface/Core/ArchHelpers/MContext.h"
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

#include "InterpreterOps.h"

namespace FEXCore::CPU {

static void InterpreterExecution(FEXCore::Core::CpuStateFrame *Frame) {
  auto Thread = Frame->Thread;

  auto LocalEntry = Thread->LocalIRCache.find(Thread->CurrentFrame->State.rip);

  InterpreterOps::InterpretIR(Thread, Thread->CurrentFrame->State.rip, LocalEntry->second.IR.get(), LocalEntry->second.DebugData.get());
}

bool InterpreterCore::HandleSIGBUS(int Signal, void *info, void *ucontext) {
#ifdef _M_ARM_64
  constexpr bool is_arm64 = true;
#else
  constexpr bool is_arm64 = false;
#endif

  if constexpr (is_arm64) {
    uint32_t *PC = reinterpret_cast<uint32_t*>(ArchHelpers::Context::GetPc(ucontext));
    uint32_t Instr = PC[0];
    if ((Instr & FEXCore::ArchHelpers::Arm64::CASPAL_MASK) == FEXCore::ArchHelpers::Arm64::CASPAL_INST) { // CASPAL
      if (FEXCore::ArchHelpers::Arm64::HandleCASPAL(ucontext, info, Instr)) {
        // Skip this instruction now
        ArchHelpers::Context::SetPc(ucontext, ArchHelpers::Context::GetPc(ucontext) + 4);
        return true;
      }
      else {
        LogMan::Msg::E("Unhandled JIT SIGBUS CASPAL: PC: %p Instruction: 0x%08x\n", PC, PC[0]);
        return false;
      }
    }
    else if ((Instr & FEXCore::ArchHelpers::Arm64::CASAL_MASK) == FEXCore::ArchHelpers::Arm64::CASAL_INST) { // CASAL
      if (FEXCore::ArchHelpers::Arm64::HandleCASAL(ucontext, info, Instr)) {
        // Skip this instruction now
        ArchHelpers::Context::SetPc(ucontext, ArchHelpers::Context::GetPc(ucontext) + 4);
        return true;
      }
      else {
        LogMan::Msg::E("Unhandled JIT SIGBUS CASAL: PC: %p Instruction: 0x%08x\n", PC, PC[0]);
        return false;
      }
    }
    else if ((Instr & FEXCore::ArchHelpers::Arm64::ATOMIC_MEM_MASK) == FEXCore::ArchHelpers::Arm64::ATOMIC_MEM_INST) { // Atomic memory op
      if (FEXCore::ArchHelpers::Arm64::HandleAtomicMemOp(ucontext, info, Instr)) {
        // Skip this instruction now
        ArchHelpers::Context::SetPc(ucontext, ArchHelpers::Context::GetPc(ucontext) + 4);
        return true;
      }
      else {
        uint8_t Op = (PC[0] >> 12) & 0xF;
        LogMan::Msg::E("Unhandled JIT SIGBUS Atomic mem op 0x%02x: PC: %p Instruction: 0x%08x\n", Op, PC, PC[0]);
        return false;
      }
    }
  }
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
      return Core->Dispatcher->HandleSignalPause(Signal, info, ucontext);
    });

    CTX->SignalDelegation->RegisterHostSignalHandler(SIGBUS, [](FEXCore::Core::InternalThreadState *Thread, int Signal, void *info, void *ucontext) -> bool {
      InterpreterCore *Core = reinterpret_cast<InterpreterCore*>(Thread->CPUBackend.get());
      return Core->HandleSIGBUS(Signal, info, ucontext);
    });

    auto GuestSignalHandler = [](FEXCore::Core::InternalThreadState *Thread, int Signal, void *info, void *ucontext, GuestSigAction *GuestAction, stack_t *GuestStack) -> bool {
      InterpreterCore *Core = reinterpret_cast<InterpreterCore*>(Thread->CPUBackend.get());
      return Core->Dispatcher->HandleGuestSignal(Signal, info, ucontext, GuestAction, GuestStack);
    };

    for (uint32_t Signal = 0; Signal < SignalDelegator::MAX_SIGNALS; ++Signal) {
      CTX->SignalDelegation->RegisterHostSignalHandlerForGuest(Signal, GuestSignalHandler);
    }
  }
}

void *InterpreterCore::CompileCode(uint64_t Entry, [[maybe_unused]] FEXCore::IR::IRListView const *IR, [[maybe_unused]] FEXCore::Core::DebugData *DebugData, FEXCore::IR::RegisterAllocationData *RAData) {
  return reinterpret_cast<void*>(InterpreterExecution);
}

FEXCore::CPU::CPUBackend *CreateInterpreterCore(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread, bool CompileThread) {
  return new InterpreterCore(ctx, Thread, CompileThread);
}

}
