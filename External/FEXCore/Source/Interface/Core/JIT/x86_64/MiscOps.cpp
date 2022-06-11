/*
$info$
tags: backend|x86-64
$end_info$
*/

#include "Interface/Context/Context.h"
#include "Interface/Core/Dispatcher/Dispatcher.h"
#include "Interface/Core/JIT/x86_64/JITClass.h"

#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/IR/IR.h>

#include <array>
#include <stddef.h>
#include <stdint.h>
#include <xbyak/xbyak.h>

namespace FEXCore::CPU {
#define DEF_OP(x) void X86JITCore::Op_##x(IR::IROp_Header *IROp, IR::NodeID Node)

DEF_OP(Fence) {
  auto Op = IROp->C<IR::IROp_Fence>();
  switch (Op->Fence) {
    case IR::Fence_Load.Val:
      lfence();
      break;
    case IR::Fence_LoadStore.Val:
      mfence();
      break;
    case IR::Fence_Store.Val:
      sfence();
      break;
    default: LOGMAN_MSG_A_FMT("Unknown Fence: {}", Op->Fence); break;
  }
}

DEF_OP(Break) {
  auto Op = IROp->C<IR::IROp_Break>();
  switch (Op->Reason) {
    case FEXCore::IR::Break_Unimplemented: // Hard fault
    case FEXCore::IR::Break_Interrupt: // Guest ud2
      ud2();
      break;
    case FEXCore::IR::Break_Overflow: // overflow
      // Need to be outside of JIT cache space to ensure cache clearing correctness
      jmp(qword [STATE + offsetof(FEXCore::Core::CpuStateFrame, Pointers.Common.OverflowExceptionHandler)]);
      break;
    case FEXCore::IR::Break_Halt: { // HLT
      // Time to quit
      // Set our stack to the starting stack location
      mov(rsp, qword [STATE + offsetof(FEXCore::Core::CpuStateFrame, ReturningStackLocation)]);

      // Now we need to jump to the thread stop handler
      jmp(qword [STATE + offsetof(FEXCore::Core::CpuStateFrame, Pointers.Common.ThreadStopHandlerSpillSRA)]);
      break;
    }
    case FEXCore::IR::Break_Interrupt3: // INT3
    {
      if (CTX->GetGdbServerStatus()) {
        // Adjust the stack first for a regular return
        if (SpillSlots) {
          add(rsp, SpillSlots * 16);
        }

        // This jump target needs to be a constant offset here
        jmp(qword [STATE + offsetof(FEXCore::Core::CpuStateFrame, Pointers.Common.ThreadPauseHandlerSpillSRA)]);
      }
      else {
        // If we don't have a gdb server attached then....crash?
        // Treat this case like HLT
        mov(rsp, qword [STATE + offsetof(FEXCore::Core::CpuStateFrame, ReturningStackLocation)]);

        // Now we need to jump to the thread stop handler
        jmp(qword [STATE + offsetof(FEXCore::Core::CpuStateFrame, Pointers.Common.ThreadStopHandlerSpillSRA)]);
      }
    break;
    }
    case FEXCore::IR::Break_InvalidInstruction:
    {
      if (SpillSlots) {
        add(rsp, SpillSlots * 16);
      }

      // Need to be outside of JIT cache space to ensure cache clearing correctness
      jmp(qword [STATE + offsetof(FEXCore::Core::CpuStateFrame, Pointers.Common.UnimplementedInstructionHandler)]);
      break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown Break reason: {}", Op->Reason);
  }
}

DEF_OP(GetRoundingMode) {
  auto Dst = GetDst<RA_32>(Node);
  sub(rsp, 4);
  // Only stores to memory
  stmxcsr(dword [rsp]);
  mov(Dst, dword [rsp]);
  add(rsp, 4);
  shr(Dst, 13);
}

DEF_OP(SetRoundingMode) {
  auto Op = IROp->C<IR::IROp_SetRoundingMode>();
  auto Src = GetSrc<RA_32>(Op->Header.Args[0].ID());

  // Load old mxcsr
  // Only stores to memory
  sub(rsp, 4);
  stmxcsr(dword [rsp]);
  mov(TMP1.cvt32(), dword [rsp]);

  // Insert the new rounding mode
  and_(TMP1.cvt32(), ~(0b111 << 13));
  mov(TMP2.cvt32(), Src);
  shl(TMP2.cvt32(), 13);
  or_(TMP1.cvt32(), TMP2.cvt32());

  // Store it to mxcsr
  // Only loads from memory
  mov(dword [rsp], TMP1.cvt32());
  ldmxcsr(dword [rsp]);
  add(rsp, 4);
}

DEF_OP(Print) {
  auto Op = IROp->C<IR::IROp_Print>();

  PushRegs();
  if (IsGPR(Op->Header.Args[0].ID())) {
    mov (rdi, GetSrc<RA_64>(Op->Header.Args[0].ID()));
    call(qword [STATE + offsetof(FEXCore::Core::CpuStateFrame, Pointers.Common.PrintValue)]);
  }
  else {
    pextrq(rdi, GetSrc(Op->Header.Args[0].ID()), 0);
    pextrq(rsi, GetSrc(Op->Header.Args[0].ID()), 1);

    call(qword [STATE + offsetof(FEXCore::Core::CpuStateFrame, Pointers.Common.PrintVectorValue)]);
  }

  PopRegs();
}

DEF_OP(ProcessorID) {
  // Cyclecounter in EDX:EAX
  // IA32_TSC_AUX in ECX
  rdtscp();
  mov (GetDst<RA_32>(Node), ecx);
}

DEF_OP(RDRAND) {
  auto Op = IROp->C<IR::IROp_RDRAND>();

  auto Dst = GetSrcPair<RA_64>(Node);

  if (Op->GetReseeded) {
    rdrand(Dst.first);
  }
  else {
    rdseed(Dst.first);
  }

  // In the case of RDRAND or RDSEED returning a valid number then CF = 1, else 0
  mov (Dst.second, 0);
  setc(Dst.second.cvt8());
}

DEF_OP(Yield) {
  pause();
}

#undef DEF_OP
void X86JITCore::RegisterMiscHandlers() {
#define REGISTER_OP(op, x) OpHandlers[FEXCore::IR::IROps::OP_##op] = &X86JITCore::Op_##x
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
  REGISTER_OP(YIELD, Yield);
#undef REGISTER_OP
}
}

