#include "Interface/Core/JIT/Arm64/JITClass.h"
#include "Interface/Core/InternalThreadState.h"

#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/HLE/SyscallHandler.h>

namespace FEXCore::CPU {
using namespace vixl;
using namespace vixl::aarch64;
#define DEF_OP(x) void JITCore::Op_##x(FEXCore::IR::IROp_Header *IROp, uint32_t Node)
DEF_OP(GuestCallDirect) {
  LogMan::Msg::D("Unimplemented");
}

DEF_OP(GuestCallIndirect) {
  LogMan::Msg::D("Unimplemented");
}

DEF_OP(GuestReturn) {
  LogMan::Msg::D("Unimplemented");
}

DEF_OP(SignalReturn) {
  // First we must reset the stack
  ResetStack();

  // Now branch to our signal return helper
  // This can't be a direct branch since the code needs to live at a constant location
  LoadConstant(x0, ThreadSharedData.SignalReturnInstruction);
  br(x0);
}

DEF_OP(CallbackReturn) {

  // spill back to CTX
  SpillStaticRegs();
  
  // First we must reset the stack
  ResetStack();

  // We can now lower the ref counter again
  LoadConstant(x0, reinterpret_cast<uint64_t>(ThreadSharedData.SignalHandlerRefCounterPtr));
  ldr(w2, MemOperand(x0));
  sub(w2, w2, 1);
  str(w2, MemOperand(x0));

  // We need to adjust an additional 8 bytes to get back to the original "misaligned" RSP state
  ldr(x2, MemOperand(STATE, offsetof(FEXCore::Core::InternalThreadState, State.State.gregs[X86State::REG_RSP])));
  add(x2, x2, 8);
  str(x2, MemOperand(STATE, offsetof(FEXCore::Core::InternalThreadState, State.State.gregs[X86State::REG_RSP])));

  PopCalleeSavedRegisters();

  // Return to the thunk
  ret();
}

DEF_OP(ExitFunction) {
  auto Op = IROp->C<IR::IROp_ExitFunction>();

  Label FullLookup;

  ResetStack();

  aarch64::Register RipReg;
  uint64_t NewRIP;

  if (IsInlineConstant(Op->NewRIP, &NewRIP) || IsInlineEntrypointOffset(Op->NewRIP, &NewRIP)) {
    Literal l_BranchHost{ExitFunctionLinkerAddress};
    Literal l_BranchGuest{NewRIP};

    ldr(x0, &l_BranchHost);
    blr(x0);

    place(&l_BranchHost);
    place(&l_BranchGuest);
  } else {
    RipReg = GetReg<RA_64>(Op->Header.Args[0].ID());
    
    // L1 Cache
    LoadConstant(x0, State->LookupCache->GetL1Pointer());

    and_(x3, RipReg, LookupCache::L1_ENTRIES_MASK);
    add(x0, x0, Operand(x3, Shift::LSL, 4));

    ldp(x1, x0, MemOperand(x0));
    cmp(x0, RipReg);
    b(&FullLookup, Condition::ne);
    br(x1);

    bind(&FullLookup);
    LoadConstant(TMP1, AbsoluteLoopTopAddress);
    str(RipReg, MemOperand(STATE, offsetof(FEXCore::Core::ThreadState, State.rip)));
    br(TMP1);
  }
}

DEF_OP(Jump) {
  auto Op = IROp->C<IR::IROp_Jump>();

  Label *TargetLabel;
  auto IsTarget = JumpTargets.find(Op->Header.Args[0].ID());
  if (IsTarget == JumpTargets.end()) {
    TargetLabel = &JumpTargets.try_emplace(Op->Header.Args[0].ID()).first->second;
  }
  else {
    TargetLabel = &IsTarget->second;
  }
  PendingTargetLabel = TargetLabel;
}

#define GRCMP(Node) (Op->CompareSize == 4 ? GetReg<RA_32>(Node) : GetReg<RA_64>(Node))
#define GRFCMP(Node) (Op->CompareSize == 4 ? GetDst(Node).S() : GetDst(Node).D())

Condition MapBranchCC(IR::CondClassType Cond) {
  switch (Cond.Val) {
  case FEXCore::IR::COND_EQ: return Condition::eq;
  case FEXCore::IR::COND_NEQ: return Condition::ne;
  case FEXCore::IR::COND_SGE: return Condition::ge;
  case FEXCore::IR::COND_SLT: return Condition::lt;
  case FEXCore::IR::COND_SGT: return Condition::gt;
  case FEXCore::IR::COND_SLE: return Condition::le;
  case FEXCore::IR::COND_UGE: return Condition::cs;
  case FEXCore::IR::COND_ULT: return Condition::cc;
  case FEXCore::IR::COND_UGT: return Condition::hi;
  case FEXCore::IR::COND_ULE: return Condition::ls;
  case FEXCore::IR::COND_FLU: return Condition::lt;
  case FEXCore::IR::COND_FGE: return Condition::ge;
  case FEXCore::IR::COND_FLEU:return Condition::le;
  case FEXCore::IR::COND_FGT: return Condition::hi;
  case FEXCore::IR::COND_FU:  return Condition::vs;
  case FEXCore::IR::COND_FNU: return Condition::vc;
  case FEXCore::IR::COND_VS:;
  case FEXCore::IR::COND_VC:;
  case FEXCore::IR::COND_MI:
  case FEXCore::IR::COND_PL:
  default:
  LogMan::Msg::A("Unsupported compare type");
  return Condition::nv;
  }
}


DEF_OP(CondJump) {
  auto Op = IROp->C<IR::IROp_CondJump>();

  Label *TrueTargetLabel;
  Label *FalseTargetLabel;

  auto TrueIter = JumpTargets.find(Op->TrueBlock.ID());
  auto FalseIter = JumpTargets.find(Op->FalseBlock.ID());

  if (TrueIter == JumpTargets.end()) {
    TrueTargetLabel = &JumpTargets.try_emplace(Op->TrueBlock.ID()).first->second;
  }
  else {
    TrueTargetLabel = &TrueIter->second;
  }


  uint64_t Const;
  bool isConst = IsInlineConstant(Op->Cmp2, &Const);

  if (isConst && Const == 0 && Op->Cond.Val == FEXCore::IR::COND_EQ) {
    LogMan::Throw::A(IsGPR(Op->Cmp1.ID()), "CondJump: Expected GPR");
    cbz(GRCMP(Op->Cmp1.ID()), TrueTargetLabel);
  } else if (isConst && Const == 0 && Op->Cond.Val == FEXCore::IR::COND_NEQ) {
    LogMan::Throw::A(IsGPR(Op->Cmp1.ID()), "CondJump: Expected GPR");
    cbnz(GRCMP(Op->Cmp1.ID()), TrueTargetLabel);
  } else {
    if (IsGPR(Op->Cmp1.ID())) {
      if (isConst)
        cmp(GRCMP(Op->Cmp1.ID()), Const);
      else
        cmp(GRCMP(Op->Cmp1.ID()), GRCMP(Op->Cmp2.ID()));
    } else if (IsFPR(Op->Cmp1.ID())) {
      fcmp(GRFCMP(Op->Cmp1.ID()), GRFCMP(Op->Cmp2.ID()));
    } else {
      LogMan::Msg::A("CondJump: Expected GPR or FPR");
    }

    b(TrueTargetLabel, MapBranchCC(Op->Cond));
  }
  
  if (FalseIter == JumpTargets.end()) {
    FalseTargetLabel = &JumpTargets.try_emplace(Op->FalseBlock.ID()).first->second;
  }
  else {
    FalseTargetLabel = &FalseIter->second;
  }
  PendingTargetLabel = FalseTargetLabel;
}

DEF_OP(Syscall) {
  auto Op = IROp->C<IR::IROp_Syscall>();
  // Arguments are passed as follows:
  // X0: SyscallHandler
  // X1: ThreadState
  // X2: Pointer to SyscallArguments

  PushDynamicRegsAndLR();
  SpillStaticRegs();

  uint64_t SPOffset = AlignUp(FEXCore::HLE::SyscallArguments::MAX_ARGS * 8, 16);
  sub(sp, sp, SPOffset);
  for (uint32_t i = 0; i < FEXCore::HLE::SyscallArguments::MAX_ARGS; ++i) {
    if (Op->Header.Args[i].IsInvalid()) continue;
    str(GetReg<RA_64>(Op->Header.Args[i].ID()), MemOperand(sp, i * 8));
  }

  LoadConstant(x0, reinterpret_cast<uint64_t>(CTX->SyscallHandler));
  mov(x1, STATE);
  mov(x2, sp);

  LoadConstant(x3, reinterpret_cast<uint64_t>(FEXCore::Context::HandleSyscall));
  blr(x3);

  add(sp, sp, SPOffset);
  
  // Result is now in x0
  // Fix the stack and any values that were stepped on
  FillStaticRegs();
  PopDynamicRegsAndLR();

  // Move result to its destination register
  mov(GetReg<RA_64>(Node), x0);
}

DEF_OP(Thunk) {
  auto Op = IROp->C<IR::IROp_Thunk>();
  // Arguments are passed as follows:
  // X0: CTX
  // X1: Args (from guest stack)

  SpillStaticRegs(); // spill to ctx before ra64 spill

  PushDynamicRegsAndLR();

  mov(x0, GetReg<RA_64>(Op->Header.Args[0].ID()));

#if _M_X86_64
  ERROR_AND_DIE("JIT: OP_THUNK not supported with arm simulator")
#else
  LoadConstant(x2, Op->ThunkFnPtr);
  blr(x2);
#endif

  PopDynamicRegsAndLR();
  
  FillStaticRegs(); // load from ctx after ra64 refill
}


DEF_OP(ValidateCode) {
  auto Op = IROp->C<IR::IROp_ValidateCode>();
  uint8_t *OldCode = (uint8_t *)&Op->CodeOriginalLow;
  int len = Op->CodeLength;
  int idx = 0;

  LoadConstant(GetReg<RA_64>(Node), 0);
  LoadConstant(x0, IR->GetHeader()->Entry + Op->Offset);
  LoadConstant(x1, 1);
  
  while (len >= 8)
  {
    ldr(x2, MemOperand(x0, idx));
    LoadConstant(x3, *(uint32_t *)(OldCode + idx));
    cmp(x2, x3);
    csel(GetReg<RA_64>(Node), GetReg<RA_64>(Node), x1, Condition::eq);
    len -= 8;
    idx += 8;
  }
  while (len >= 4)
  {
    ldr(w2, MemOperand(x0, idx));
    LoadConstant(w3, *(uint32_t *)(OldCode + idx));
    cmp(w2, w3);
    csel(GetReg<RA_64>(Node), GetReg<RA_64>(Node), x1, Condition::eq);
    len -= 4;
    idx += 4;
  }
  while (len >= 2)
  {
    ldrh(w2, MemOperand(x0, idx));
    LoadConstant(w3, *(uint16_t *)(OldCode + idx));
    cmp(w2, w3);
    csel(GetReg<RA_64>(Node), GetReg<RA_64>(Node), x1, Condition::eq);
    len -= 2;
    idx += 2;
  }
  while (len >= 1)
  {
    ldrb(w2, MemOperand(x0, idx));
    LoadConstant(w3, *(uint8_t *)(OldCode + idx));
    cmp(w2, w3);
    csel(GetReg<RA_64>(Node), GetReg<RA_64>(Node), x1, Condition::eq);
    len -= 1;
    idx += 1;
  }
}

DEF_OP(RemoveCodeEntry) {
  auto Op = IROp->C<IR::IROp_RemoveCodeEntry>();
  // Arguments are passed as follows:
  // X0: Thread
  // X1: RIP

  PushDynamicRegsAndLR();
  
  mov(x0, STATE);
  LoadConstant(x1, Op->RIP);
 
  LoadConstant(x2, reinterpret_cast<uintptr_t>(&Context::Context::RemoveCodeEntry));
  SpillStaticRegs();
  blr(x2);
  FillStaticRegs();

  // Fix the stack and any values that were stepped on
  PopDynamicRegsAndLR();
}

DEF_OP(CPUID) {
  auto Op = IROp->C<IR::IROp_CPUID>();
  
  PushDynamicRegsAndLR();

  // x0 = CPUID Handler
  // x1 = CPUID Function
  LoadConstant(x0, reinterpret_cast<uint64_t>(&CTX->CPUID));
  mov(x1, GetReg<RA_64>(Op->Header.Args[0].ID()));

  using ClassPtrType = FEXCore::CPUID::FunctionResults (FEXCore::CPUIDEmu::*)(uint32_t);
  union PtrCast {
    ClassPtrType ClassPtr;
    uintptr_t Data;
  };

  PtrCast Ptr;
  Ptr.ClassPtr = &FEXCore::CPUIDEmu::RunFunction;
  LoadConstant(x3, Ptr.Data);
  SpillStaticRegs();
  blr(x3);
  FillStaticRegs();

  PopDynamicRegsAndLR();

  // Results are in x0, x1
  // Results want to be in a i64v2 vector
  auto Dst = GetSrcPair<RA_64>(Node);
  mov(Dst.first,  x0);
  mov(Dst.second, x1);
}

#undef DEF_OP
void JITCore::RegisterBranchHandlers() {
#define REGISTER_OP(op, x) OpHandlers[FEXCore::IR::IROps::OP_##op] = &JITCore::Op_##x
  REGISTER_OP(GUESTCALLDIRECT,   GuestCallDirect);
  REGISTER_OP(GUESTCALLINDIRECT, GuestCallIndirect);
  REGISTER_OP(GUESTRETURN,       GuestReturn);
  REGISTER_OP(SIGNALRETURN,      SignalReturn);
  REGISTER_OP(CALLBACKRETURN,    CallbackReturn);
  REGISTER_OP(EXITFUNCTION,      ExitFunction);
  REGISTER_OP(JUMP,              Jump);
  REGISTER_OP(CONDJUMP,          CondJump);
  REGISTER_OP(SYSCALL,           Syscall);
  REGISTER_OP(THUNK,             Thunk);
  REGISTER_OP(VALIDATECODE,      ValidateCode);
  REGISTER_OP(REMOVECODEENTRY,   RemoveCodeEntry);
  REGISTER_OP(CPUID,             CPUID);
#undef REGISTER_OP
}
}

