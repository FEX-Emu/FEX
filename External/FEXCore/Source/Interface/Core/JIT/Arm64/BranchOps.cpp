/*
$info$
tags: backend|arm64
$end_info$
*/

#include "FEXCore/IR/IR.h"
#include "Interface/Core/LookupCache.h"

#include "Interface/Core/JIT/Arm64/JITClass.h"
#include "Interface/Core/InternalThreadState.h"

#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/HLE/SyscallHandler.h>
#include <FEXCore/Utils/MathUtils.h>
#include <Interface/HLE/Thunks/Thunks.h>

namespace FEXCore::CPU {
using namespace vixl;
using namespace vixl::aarch64;
#define DEF_OP(x) void Arm64JITCore::Op_##x(IR::IROp_Header *IROp, IR::NodeID Node)
DEF_OP(GuestCallDirect) {
  LogMan::Msg::DFmt("Unimplemented");
}

DEF_OP(GuestCallIndirect) {
  LogMan::Msg::DFmt("Unimplemented");
}

DEF_OP(SignalReturn) {
  // First we must reset the stack
  ResetStack();

  // Now branch to our signal return helper
  // This can't be a direct branch since the code needs to live at a constant location
  ldr(x0, MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.Common.SignalReturnHandler)));
  br(x0);
}

DEF_OP(CallbackReturn) {

  // spill back to CTX
  SpillStaticRegs();

  // First we must reset the stack
  ResetStack();

  // We can now lower the ref counter again
  
  ldr(w2, MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, SignalHandlerRefCounter)));
  sub(w2, w2, 1);
  str(w2, MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, SignalHandlerRefCounter)));

  // We need to adjust an additional 8 bytes to get back to the original "misaligned" RSP state
  ldr(x2, MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, State.gregs[X86State::REG_RSP])));
  add(x2, x2, 8);
  str(x2, MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, State.gregs[X86State::REG_RSP])));

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
    Literal l_BranchHost{Dispatcher->ExitFunctionLinkerAddress};
    Literal l_BranchGuest{NewRIP};

    ldr(x0, &l_BranchHost);
    blr(x0);

    place(&l_BranchHost);
    place(&l_BranchGuest);
  } else {
    RipReg = GetReg<RA_64>(Op->Header.Args[0].ID());

    // L1 Cache
    ldr(x0, MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.Common.L1Pointer)));

    and_(x3, RipReg, LookupCache::L1_ENTRIES_MASK);
    add(x0, x0, Operand(x3, Shift::LSL, 4));

    ldp(x1, x0, MemOperand(x0));
    cmp(x0, RipReg);
    b(&FullLookup, Condition::ne);
    br(x1);

    bind(&FullLookup);
    ldr(TMP1, MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.Common.DispatcherLoopTop)));
    str(RipReg, MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, State.rip)));
    br(TMP1);
  }
}

DEF_OP(Jump) {
  const auto Op = IROp->C<IR::IROp_Jump>();
  const auto ArgID = Op->Args(0).ID();

  PendingTargetLabel = &JumpTargets.try_emplace(ArgID).first->second;
}

#define GRCMP(Node) (Op->CompareSize == 4 ? GetReg<RA_32>(Node) : GetReg<RA_64>(Node))
#define GRFCMP(Node) (Op->CompareSize == 4 ? GetDst(Node).S() : GetDst(Node).D())

static Condition MapBranchCC(IR::CondClassType Cond) {
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
  case FEXCore::IR::COND_FGT: return Condition::gt;
  case FEXCore::IR::COND_FU:  return Condition::vs;
  case FEXCore::IR::COND_FNU: return Condition::vc;
  case FEXCore::IR::COND_VS:
  case FEXCore::IR::COND_VC:
  case FEXCore::IR::COND_MI:
  case FEXCore::IR::COND_PL:
  default:
  LOGMAN_MSG_A_FMT("Unsupported compare type");
  return Condition::nv;
  }
}


DEF_OP(CondJump) {
  auto Op = IROp->C<IR::IROp_CondJump>();

  Label *TrueTargetLabel = &JumpTargets.try_emplace(Op->TrueBlock.ID()).first->second;

  uint64_t Const;
  const bool isConst = IsInlineConstant(Op->Cmp2, &Const);

  if (isConst && Const == 0 && Op->Cond.Val == FEXCore::IR::COND_EQ) {
    LOGMAN_THROW_A_FMT(IsGPR(Op->Cmp1.ID()), "CondJump: Expected GPR");
    cbz(GRCMP(Op->Cmp1.ID()), TrueTargetLabel);
  } else if (isConst && Const == 0 && Op->Cond.Val == FEXCore::IR::COND_NEQ) {
    LOGMAN_THROW_A_FMT(IsGPR(Op->Cmp1.ID()), "CondJump: Expected GPR");
    cbnz(GRCMP(Op->Cmp1.ID()), TrueTargetLabel);
  } else {
    if (IsGPR(Op->Cmp1.ID())) {
      if (isConst) {
        cmp(GRCMP(Op->Cmp1.ID()), Const);
      } else {
        cmp(GRCMP(Op->Cmp1.ID()), GRCMP(Op->Cmp2.ID()));
      }
    } else if (IsFPR(Op->Cmp1.ID())) {
      fcmp(GRFCMP(Op->Cmp1.ID()), GRFCMP(Op->Cmp2.ID()));
    } else {
      LOGMAN_MSG_A_FMT("CondJump: Expected GPR or FPR");
    }

    b(TrueTargetLabel, MapBranchCC(Op->Cond));
  }

  PendingTargetLabel = &JumpTargets.try_emplace(Op->FalseBlock.ID()).first->second;
}

DEF_OP(Syscall) {
  auto Op = IROp->C<IR::IROp_Syscall>();
  // Arguments are passed as follows:
  // X0: SyscallHandler
  // X1: ThreadState
  // X2: Pointer to SyscallArguments

  FEXCore::IR::SyscallFlags Flags = Op->Flags;
  PushDynamicRegsAndLR();

  if ((Flags & FEXCore::IR::SyscallFlags::NOSYNCSTATEONENTRY) != FEXCore::IR::SyscallFlags::NOSYNCSTATEONENTRY) {
    SpillStaticRegs();
  }
  else {
    // Need to spill all caller saved registers still
    SpillStaticRegs(true, CALLER_GPR_MASK, CALLER_FPR_MASK);
  }

  uint64_t SPOffset = AlignUp(FEXCore::HLE::SyscallArguments::MAX_ARGS * 8, 16);
  sub(sp, sp, SPOffset);
  for (uint32_t i = 0; i < FEXCore::HLE::SyscallArguments::MAX_ARGS; ++i) {
    if (Op->Header.Args[i].IsInvalid()) continue;
    str(GetReg<RA_64>(Op->Header.Args[i].ID()), MemOperand(sp, i * 8));
  }

  ldr(x0, MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.Common.SyscallHandlerObj)));
  ldr(x3, MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.Common.SyscallHandlerFunc)));
  mov(x1, STATE);
  mov(x2, sp);
  blr(x3);

  add(sp, sp, SPOffset);

  if ((Flags & FEXCore::IR::SyscallFlags::NOSYNCSTATEONENTRY) != FEXCore::IR::SyscallFlags::NOSYNCSTATEONENTRY &&
      (Flags & FEXCore::IR::SyscallFlags::NORETURN) != FEXCore::IR::SyscallFlags::NORETURN) {
    FillStaticRegs();
  }
  else {
    // Result is now in x0
    // Fix the stack and any values that were stepped on
    FillStaticRegs(true, CALLER_GPR_MASK, CALLER_FPR_MASK);
  }

  PopDynamicRegsAndLR();

  if ((Flags & FEXCore::IR::SyscallFlags::NORETURN) != FEXCore::IR::SyscallFlags::NORETURN) {
    // Move result to its destination register
    mov(GetReg<RA_64>(Node), x0);
  }
}

DEF_OP(InlineSyscall) {
  auto Op = IROp->C<IR::IROp_InlineSyscall>();
  // Arguments are passed as follows:
  // X8: SyscallNumber - RA INTERSECT
  // X0: Arg0 & Return
  // X1: Arg1
  // X2: Arg2
  // X3: Arg3
  // X4: Arg4 - RA INTERSECT
  // X5: Arg5 - RA INTERSECT
  // X6: Arg6 - Doesn't exist in x86-64 land. RA INTERSECT

  // One argument is removed from the SyscallArguments::MAX_ARGS since the first argument was syscall number
  const static std::array<vixl::aarch64::Register, FEXCore::HLE::SyscallArguments::MAX_ARGS-1> RegArgs = {{
    x0, x1, x2, x3, x4, x5
  }};

  bool Intersects{};
  // We always need to spill x8 since we can't know if it is live at this SSA location
  uint32_t SpillMask = 1U << 8;
  std::vector<vixl::aarch64::Register> IntersectRegs(FEXCore::HLE::SyscallArguments::MAX_ARGS);
  for (uint32_t i = 0; i < FEXCore::HLE::SyscallArguments::MAX_ARGS-1; ++i) {
    if (Op->Header.Args[i].IsInvalid()) break;

    auto Reg = GetReg<RA_64>(Op->Header.Args[i].ID());
    if (Reg.GetCode() == x8.GetCode() ||
        Reg.GetCode() == x4.GetCode() ||
        Reg.GetCode() == x5.GetCode()) {

      SpillMask |= (1U << Reg.GetCode());
      Intersects = true;
    }
  }
  // XXX: For some reason spilling only the x4, x5, and x8 registers was causing issues
  // Come back to this once investigation reveals why it fails the gvisor ioctl test
  // For now override to all GPRs
  SpillMask = ~0U;

  // Ordering is incredibly important here
  // We must spill any overlapping registers first THEN claim we are in a syscall without invalidating state at all
  // Only spill the registers that intersect with our usage
  SpillStaticRegs(false, SpillMask);

  // Now that we are spilled, store in the state that we are in a syscall
  // Still without overwriting registers that matter
  // 16bit LoadConstant to be a single instruction
  // We must always spill at least one register (x8) so this value always has a bit set
  // This gives the signal handler a value to check to see if we are in a syscall at all
  LoadConstant(x0, SpillMask & 0xFFFF);
  str(x0, MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, InSyscallInfo)));

  // Now that we have claimed to be a syscall we can set up the arguments
  if (Intersects) {
    for (uint32_t i = 0; i < FEXCore::HLE::SyscallArguments::MAX_ARGS-1; ++i) {
      if (Op->Header.Args[i].IsInvalid()) break;

      if (CTX->Config.Is64BitMode()) {
        auto Reg = GetReg<RA_64>(Op->Header.Args[i].ID());
        // In the case of intersection with x4, x5, or x8 then these are currently SRA
        // for registers RAX, RBX, and RSI. Which have just been spilled
        // Just load back from the context. Could be slightly smarter but this is fairly uncommon
        if (Reg.GetCode() == x8.GetCode()) {
          ldr(RegArgs[i], MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, State.gregs[X86State::REG_RSI])));
        }
        else if (Reg.GetCode() == x4.GetCode()) {
          ldr(RegArgs[i], MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, State.gregs[X86State::REG_RAX])));
        }
        else if (Reg.GetCode() == x5.GetCode()) {
          ldr(RegArgs[i], MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, State.gregs[X86State::REG_RBX])));
        }
      }
    }

    for (uint32_t i = 0; i < FEXCore::HLE::SyscallArguments::MAX_ARGS-1; ++i) {
      if (Op->Header.Args[i].IsInvalid()) break;

      if (CTX->Config.Is64BitMode()) {
        auto Reg = GetReg<RA_64>(Op->Header.Args[i].ID());
        // In the case of intersection with x4, x5, or x8 then these are currently SRA
        // for registers RAX, RBX, and RSI. Which have just been spilled
        // Just load back from the context. Could be slightly smarter but this is fairly uncommon
        if (Reg.GetCode() == x8.GetCode()) {
          ldr(RegArgs[i], MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, State.gregs[X86State::REG_RSI])));
        }
        else if (Reg.GetCode() == x4.GetCode()) {
          ldr(RegArgs[i], MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, State.gregs[X86State::REG_RAX])));
        }
        else if (Reg.GetCode() == x5.GetCode()) {
          ldr(RegArgs[i], MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, State.gregs[X86State::REG_RBX])));
        }
        else {
          mov(RegArgs[i], Reg);
        }
      }
      else {
        auto Reg = GetReg<RA_32>(Op->Header.Args[i].ID());
        if (Reg.GetCode() == w8.GetCode()) {
          ldr(RegArgs[i].W(), MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, State.gregs[X86State::REG_RSI])));
        }
        else if (Reg.GetCode() == w4.GetCode()) {
          ldr(RegArgs[i].W(), MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, State.gregs[X86State::REG_RAX])));
        }
        else if (Reg.GetCode() == w5.GetCode()) {
          ldr(RegArgs[i].W(), MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, State.gregs[X86State::REG_RBX])));
        }
        else {
          uxtw(RegArgs[i].W(), Reg);
        }
      }
    }
  }
  else {
    for (uint32_t i = 0; i < FEXCore::HLE::SyscallArguments::MAX_ARGS-1; ++i) {
      if (Op->Header.Args[i].IsInvalid()) break;

      if (CTX->Config.Is64BitMode()) {
        mov(RegArgs[i], GetReg<RA_64>(Op->Header.Args[i].ID()));
      }
      else {
        uxtw(RegArgs[i], GetReg<RA_64>(Op->Header.Args[i].ID()));
      }
    }
  }

  LoadConstant(x8, Op->HostSyscallNumber);
  svc(0);
  // On updated signal mask we can receive a signal RIGHT HERE

  if ((Op->Flags & FEXCore::IR::SyscallFlags::NORETURN) != FEXCore::IR::SyscallFlags::NORETURN) {
    // Now that we are done in the syscall we need to carefully peel back the state
    // First unspill the registers from before
    FillStaticRegs(false, SpillMask);

    // Now the registers we've spilled are back in their original host registers
    // We can safely claim we are no longer in a syscall
    str(xzr, MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, InSyscallInfo)));

    // Result is now in x0
    // Move result to its destination register
    if (CTX->Config.Is64BitMode()) {
      mov(GetReg<RA_64>(Node), x0);
    }
    else {
      uxtw(GetReg<RA_64>(Node), x0);
    }
  }
}

DEF_OP(Thunk) {
  auto Op = IROp->C<IR::IROp_Thunk>();
  // Arguments are passed as follows:
  // X0: CTX
  // X1: Args (from guest stack)

  SpillStaticRegs(); // spill to ctx before ra64 spill

  PushDynamicRegsAndLR();

  mov(x0, GetReg<RA_64>(Op->Header.Args[0].ID()));

  auto thunkFn = ThreadState->CTX->ThunkHandler->LookupThunk(Op->ThunkNameHash);
  LoadConstant(x2, (uintptr_t)thunkFn);
  blr(x2);

  PopDynamicRegsAndLR();

  FillStaticRegs(); // load from ctx after ra64 refill
}

DEF_OP(ValidateCode) {
  auto Op = IROp->C<IR::IROp_ValidateCode>();
  const auto *OldCode = (const uint8_t *)&Op->CodeOriginalLow;
  int len = Op->CodeLength;
  int idx = 0;

  LoadConstant(GetReg<RA_64>(Node), 0);
  LoadConstant(x0, Entry + Op->Offset);
  LoadConstant(x1, 1);

  while (len >= 8)
  {
    ldr(x2, MemOperand(x0, idx));
    LoadConstant(x3, *(const uint32_t *)(OldCode + idx));
    cmp(x2, x3);
    csel(GetReg<RA_64>(Node), GetReg<RA_64>(Node), x1, Condition::eq);
    len -= 8;
    idx += 8;
  }
  while (len >= 4)
  {
    ldr(w2, MemOperand(x0, idx));
    LoadConstant(w3, *(const uint32_t *)(OldCode + idx));
    cmp(w2, w3);
    csel(GetReg<RA_64>(Node), GetReg<RA_64>(Node), x1, Condition::eq);
    len -= 4;
    idx += 4;
  }
  while (len >= 2)
  {
    ldrh(w2, MemOperand(x0, idx));
    LoadConstant(w3, *(const uint16_t *)(OldCode + idx));
    cmp(w2, w3);
    csel(GetReg<RA_64>(Node), GetReg<RA_64>(Node), x1, Condition::eq);
    len -= 2;
    idx += 2;
  }
  while (len >= 1)
  {
    ldrb(w2, MemOperand(x0, idx));
    LoadConstant(w3, *(const uint8_t *)(OldCode + idx));
    cmp(w2, w3);
    csel(GetReg<RA_64>(Node), GetReg<RA_64>(Node), x1, Condition::eq);
    len -= 1;
    idx += 1;
  }
}

DEF_OP(RemoveThreadCodeEntry) {
  // Arguments are passed as follows:
  // X0: Thread
  // X1: RIP

  PushDynamicRegsAndLR();

  mov(x0, STATE);
  LoadConstant(x1, Entry);

  ldr(x2, MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.Common.RemoveThreadCodeEntryFromJIT)));
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
  // x2 = CPUID Leaf
  ldr(x0, MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.Common.CPUIDObj)));
  ldr(x3, MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.Common.CPUIDFunction)));
  mov(x1, GetReg<RA_64>(Op->Header.Args[0].ID()));
  mov(x2, GetReg<RA_64>(Op->Header.Args[1].ID()));
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
void Arm64JITCore::RegisterBranchHandlers() {
#define REGISTER_OP(op, x) OpHandlers[FEXCore::IR::IROps::OP_##op] = &Arm64JITCore::Op_##x
  REGISTER_OP(GUESTCALLDIRECT,   GuestCallDirect);
  REGISTER_OP(GUESTCALLINDIRECT, GuestCallIndirect);
  REGISTER_OP(SIGNALRETURN,      SignalReturn);
  REGISTER_OP(CALLBACKRETURN,    CallbackReturn);
  REGISTER_OP(EXITFUNCTION,      ExitFunction);
  REGISTER_OP(JUMP,              Jump);
  REGISTER_OP(CONDJUMP,          CondJump);
  REGISTER_OP(SYSCALL,           Syscall);
  REGISTER_OP(INLINESYSCALL,     InlineSyscall);
  REGISTER_OP(THUNK,             Thunk);
  REGISTER_OP(VALIDATECODE,      ValidateCode);
  REGISTER_OP(REMOVETHREADCODEENTRY,   RemoveThreadCodeEntry);
  REGISTER_OP(CPUID,             CPUID);
#undef REGISTER_OP
}
}

