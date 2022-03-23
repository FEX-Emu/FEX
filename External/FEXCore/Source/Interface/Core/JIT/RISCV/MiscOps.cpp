/*
$info$
tags: backend|riscv64
$end_info$
*/

#include "FEXCore/IR/IR.h"
#include "Interface/Core/LookupCache.h"

#include "Interface/Core/JIT/RISCV/JITClass.h"
#include "Interface/Core/InternalThreadState.h"

#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/HLE/SyscallHandler.h>
#include <FEXCore/Utils/MathUtils.h>
#include <Interface/HLE/Thunks/Thunks.h>

#include <biscuit/assembler.hpp>

namespace FEXCore::CPU {
using namespace biscuit;

#define DEF_OP(x) void RISCVJITCore::Op_##x(IR::IROp_Header *IROp, IR::NodeID Node)
DEF_OP(Fence) {
  ERROR_AND_DIE_FMT("Nope: {}", __func__);
}

DEF_OP(Break) {
  auto Op = IROp->C<IR::IROp_Break>();
  switch (Op->Reason) {
    case FEXCore::IR::Break_Unimplemented: // Hard fault
    case FEXCore::IR::Break_Interrupt: // Guest ud2
      EBREAK();
      break;
    case FEXCore::IR::Break_Overflow: // overflow
      ResetStack();
      LD(TMP1, offsetof(FEXCore::Core::CpuStateFrame, Pointers.RISCV.OverflowExceptionHandler), STATE);
      JR(TMP1);
      break;
    case FEXCore::IR::Break_Halt: { // HLT
      // Time to quit
      // Set our stack to the starting stack location
      LD(sp, offsetof(FEXCore::Core::CpuStateFrame, ReturningStackLocation), STATE);

      // Now we need to jump to the thread stop handler
      LD(TMP1, offsetof(FEXCore::Core::CpuStateFrame, Pointers.RISCV.ThreadStopHandlerSpillSRA), STATE);
      JR(TMP1);
      break;
    }
    case FEXCore::IR::Break_Interrupt3: { // INT3
      ResetStack();
      LD(sp, offsetof(FEXCore::Core::CpuStateFrame, Pointers.RISCV.ThreadPauseHandlerSpillSRA), STATE);
      JR(TMP1);
      break;
    }
    case FEXCore::IR::Break_InvalidInstruction:
    {
      ResetStack();
      LD(sp, offsetof(FEXCore::Core::CpuStateFrame, Pointers.RISCV.UnimplementedInstructionHandler), STATE);
      JR(TMP1);

      break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown Break reason: {}", Op->Reason);
  }
}

DEF_OP(GetRoundingMode) {
  ERROR_AND_DIE_FMT("Nope: {}", __func__);
}

DEF_OP(SetRoundingMode) {
  ERROR_AND_DIE_FMT("Nope: {}", __func__);
}

DEF_OP(Print) {
  ERROR_AND_DIE_FMT("Nope: {}", __func__);
}

DEF_OP(ProcessorID) {
  ERROR_AND_DIE_FMT("Nope: {}", __func__);
}

DEF_OP(RDRAND) {
  ERROR_AND_DIE_FMT("Nope: {}", __func__);
}

#undef DEF_OP
void RISCVJITCore::RegisterMiscHandlers() {
#define REGISTER_OP(op, x) OpHandlers[FEXCore::IR::IROps::OP_##op] = &RISCVJITCore::Op_##x
  REGISTER_OP(DUMMY,      NoOp);
  REGISTER_OP(IRHEADER,   NoOp);
  REGISTER_OP(CODEBLOCK,  NoOp);
  REGISTER_OP(BEGINBLOCK, NoOp);
  REGISTER_OP(ENDBLOCK,   NoOp);
  REGISTER_OP(FENCE,      Fence);
  REGISTER_OP(BREAK,      Break);
  REGISTER_OP(PHI,        NoOp);
  REGISTER_OP(PHIVALUE,   NoOp);
  REGISTER_OP(PRINT,      Print);
  REGISTER_OP(GETROUNDINGMODE, GetRoundingMode);
  REGISTER_OP(SETROUNDINGMODE, SetRoundingMode);
  REGISTER_OP(INVALIDATEFLAGS,   NoOp);
  REGISTER_OP(PROCESSORID,   ProcessorID);
  REGISTER_OP(RDRAND, RDRAND);
#undef REGISTER_OP
}
}

