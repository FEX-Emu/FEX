/*
$info$
tags: backend|x86-64
$end_info$
*/

#include "Interface/Core/JIT/x86_64/JITClass.h"
#include "Interface/IR/Passes/RegisterAllocationPass.h"

namespace FEXCore::CPU {
static void PrintValue(uint64_t Value) {
  LogMan::Msg::D("Value: 0x%lx", Value);
}

#define DEF_OP(x) void X86JITCore::Op_##x(FEXCore::IR::IROp_Header *IROp, uint32_t Node)

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
    default: LOGMAN_MSG_A("Unknown Fence: %d", Op->Fence); break;
  }
}

DEF_OP(Break) {
  auto Op = IROp->C<IR::IROp_Break>();
  switch (Op->Reason) {
    case 0: // Hard fault
    case 5: // Guest ud2
      ud2();
    break;
    case 4: { // HLT
      // Time to quit
      // Set our stack to the starting stack location
      mov(rsp, qword [STATE + offsetof(FEXCore::Core::CpuStateFrame, ReturningStackLocation)]);

      // Now we need to jump to the thread stop handler
      mov(TMP1, ThreadSharedData.Dispatcher->ThreadStopHandlerAddress);
      jmp(TMP1);
      break;
    }
    case 6: // INT3
    {
      if (CTX->GetGdbServerStatus()) {
        // Adjust the stack first for a regular return
        if (SpillSlots) {
          add(rsp, SpillSlots * 16);
        }

        // This jump target needs to be a constant offset here
        mov(TMP1, ThreadSharedData.Dispatcher->ThreadPauseHandlerAddress);
        jmp(TMP1);
      }
      else {
        // If we don't have a gdb server attached then....crash?
        // Treat this case like HLT
        mov(rsp, qword [STATE + offsetof(FEXCore::Core::CpuStateFrame, ReturningStackLocation)]);

        // Now we need to jump to the thread stop handler
        mov(TMP1, ThreadSharedData.Dispatcher->ThreadStopHandlerAddress);
        jmp(TMP1);
      }
    break;
    }
    //default: LOGMAN_MSG_A("Unknown Break reason: %d", Op->Reason);
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

  for (auto &Reg : RA64)
    push(Reg);

  auto NumPush = RA64.size();
  if (NumPush & 1)
    sub(rsp, 8); // Align

  mov (rdi, GetSrc<RA_64>(Op->Header.Args[0].ID()));

  mov(rax, reinterpret_cast<uintptr_t>(PrintValue));

  call(rax);

  if (NumPush & 1)
    add(rsp, 8); // Align

  for (uint32_t i = RA64.size(); i > 0; --i)
    pop(RA64[i - 1]);
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
#undef REGISTER_OP
}
}

