// SPDX-License-Identifier: MIT
/*
$info$
tags: backend|arm64
$end_info$
*/

#include "Interface/Context/Context.h"
#include "FEXCore/IR/IR.h"
#include "Interface/Core/ArchHelpers/CodeEmitter/Emitter.h"
#include "Interface/Core/LookupCache.h"

#include "Interface/Core/JIT/Arm64/JITClass.h"
#include "Interface/Core/InternalThreadState.h"

#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/HLE/SyscallHandler.h>
#include <FEXCore/Utils/MathUtils.h>
#include <Interface/HLE/Thunks/Thunks.h>

namespace FEXCore::CPU {
#define DEF_OP(x) void Arm64JITCore::Op_##x(IR::IROp_Header const *IROp, IR::NodeID Node)

DEF_OP(CallbackReturn) {
  // spill back to CTX
  SpillStaticRegs(TMP1);

  // First we must reset the stack
  ResetStack();

  // We can now lower the ref counter again

  ldr(ARMEmitter::WReg::w2, STATE, offsetof(FEXCore::Core::CpuStateFrame, SignalHandlerRefCounter));
  sub(ARMEmitter::Size::i32Bit, ARMEmitter::Reg::r2, ARMEmitter::Reg::r2, 1);
  str(ARMEmitter::WReg::w2, STATE, offsetof(FEXCore::Core::CpuStateFrame, SignalHandlerRefCounter));

  // We need to adjust an additional 8 bytes to get back to the original "misaligned" RSP state
  ldr(ARMEmitter::XReg::x2, STATE, offsetof(FEXCore::Core::CpuStateFrame, State.gregs[X86State::REG_RSP]));
  add(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::r2, ARMEmitter::Reg::r2, 8);
  str(ARMEmitter::XReg::x2, STATE, offsetof(FEXCore::Core::CpuStateFrame, State.gregs[X86State::REG_RSP]));

  PopCalleeSavedRegisters();

  // Return to the thunk
  ret();
}

DEF_OP(ExitFunction) {
  auto Op = IROp->C<IR::IROp_ExitFunction>();

  ResetStack();

  uint64_t NewRIP;

  if (IsInlineConstant(Op->NewRIP, &NewRIP) || IsInlineEntrypointOffset(Op->NewRIP, &NewRIP)) {
    ARMEmitter::ForwardLabel l_BranchHost;
    ARMEmitter::ForwardLabel l_BranchGuest;

    ldr(ARMEmitter::XReg::x0, &l_BranchHost);
    blr(ARMEmitter::Reg::r0);

    Bind(&l_BranchHost);
    dc64(ThreadState->CurrentFrame->Pointers.Common.ExitFunctionLinker);
    Bind(&l_BranchGuest);
    dc64(NewRIP);

  } else {

    ARMEmitter::ForwardLabel FullLookup;
    auto RipReg = GetReg(Op->NewRIP.ID());

    // L1 Cache
    ldr(ARMEmitter::XReg::x0, STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.Common.L1Pointer));

    and_(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::r3, RipReg, LookupCache::L1_ENTRIES_MASK);
    add(ARMEmitter::XReg::x0, ARMEmitter::XReg::x0, ARMEmitter::XReg::x3, ARMEmitter::ShiftType::LSL, 4);

    // Note: sub+cbnz used over cmp+br to preserve flags.
    ldp<ARMEmitter::IndexType::OFFSET>(ARMEmitter::XReg::x1, ARMEmitter::XReg::x0, ARMEmitter::Reg::r0, 0);
    sub(TMP1, ARMEmitter::XReg::x0, RipReg.X());
    cbnz(ARMEmitter::Size::i64Bit, TMP1, &FullLookup);
    br(ARMEmitter::Reg::r1);

    Bind(&FullLookup);
    ldr(TMP1, STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.Common.DispatcherLoopTop));
    str(RipReg.X(), STATE, offsetof(FEXCore::Core::CpuStateFrame, State.rip));
    br(TMP1);
  }
}

DEF_OP(Jump) {
  const auto Op = IROp->C<IR::IROp_Jump>();
  const auto Target = Op->TargetBlock.ID();

  PendingTargetLabel = &JumpTargets.try_emplace(Target).first->second;
}

static ARMEmitter::Condition MapBranchCC(IR::CondClassType Cond) {
  switch (Cond.Val) {
  case FEXCore::IR::COND_ANDZ:
  case FEXCore::IR::COND_EQ:  return ARMEmitter::Condition::CC_EQ;
  case FEXCore::IR::COND_ANDNZ:
  case FEXCore::IR::COND_NEQ: return ARMEmitter::Condition::CC_NE;
  case FEXCore::IR::COND_SGE: return ARMEmitter::Condition::CC_GE;
  case FEXCore::IR::COND_SLT: return ARMEmitter::Condition::CC_LT;
  case FEXCore::IR::COND_SGT: return ARMEmitter::Condition::CC_GT;
  case FEXCore::IR::COND_SLE: return ARMEmitter::Condition::CC_LE;
  case FEXCore::IR::COND_UGE: return ARMEmitter::Condition::CC_CS;
  case FEXCore::IR::COND_ULT: return ARMEmitter::Condition::CC_CC;
  case FEXCore::IR::COND_UGT: return ARMEmitter::Condition::CC_HI;
  case FEXCore::IR::COND_ULE: return ARMEmitter::Condition::CC_LS;
  case FEXCore::IR::COND_FLU: return ARMEmitter::Condition::CC_LT;
  case FEXCore::IR::COND_FGE: return ARMEmitter::Condition::CC_GE;
  case FEXCore::IR::COND_FLEU:return ARMEmitter::Condition::CC_LE;
  case FEXCore::IR::COND_FGT: return ARMEmitter::Condition::CC_GT;
  case FEXCore::IR::COND_FU:  return ARMEmitter::Condition::CC_VS;
  case FEXCore::IR::COND_FNU: return ARMEmitter::Condition::CC_VC;
  case FEXCore::IR::COND_VS:
  case FEXCore::IR::COND_VC:
  case FEXCore::IR::COND_MI:  return ARMEmitter::Condition::CC_MI;
  case FEXCore::IR::COND_PL:  return ARMEmitter::Condition::CC_PL;
  default:
  LOGMAN_MSG_A_FMT("Unsupported compare type");
  return ARMEmitter::Condition::CC_NV;
  }
}

DEF_OP(CondJump) {
  auto Op = IROp->C<IR::IROp_CondJump>();

  auto TrueTargetLabel = &JumpTargets.try_emplace(Op->TrueBlock.ID()).first->second;

  if (Op->FromNZCV) {
    b(MapBranchCC(Op->Cond), TrueTargetLabel);
  } else {
    uint64_t Const;
    const bool isConst = IsInlineConstant(Op->Cmp2, &Const);

    const auto Size = Op->CompareSize == 4 ? ARMEmitter::Size::i32Bit : ARMEmitter::Size::i64Bit;

    LOGMAN_THROW_A_FMT(IsGPR(Op->Cmp1.ID()), "CondJump: Expected GPR");
    LOGMAN_THROW_A_FMT(isConst && Const == 0, "CondJump: Expected 0 source");
    LOGMAN_THROW_A_FMT(Op->Cond.Val == FEXCore::IR::COND_EQ ||
                       Op->Cond.Val == FEXCore::IR::COND_NEQ,
                       "CondJump: Expected simple condition");

    if (Op->Cond.Val == FEXCore::IR::COND_EQ) {
      cbz(Size, GetReg(Op->Cmp1.ID()), TrueTargetLabel);
    } else  {
      cbnz(Size, GetReg(Op->Cmp1.ID()), TrueTargetLabel);
    }

    // TODO: Wire up tbz/tbnz
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
  PushDynamicRegsAndLR(TMP1);

  uint32_t GPRSpillMask = ~0U;
  uint32_t FPRSpillMask = ~0U;
  if ((Flags & FEXCore::IR::SyscallFlags::NOSYNCSTATEONENTRY) == FEXCore::IR::SyscallFlags::NOSYNCSTATEONENTRY) {
    // Need to spill all caller saved registers still
    GPRSpillMask = CALLER_GPR_MASK;
    FPRSpillMask = CALLER_FPR_MASK;
  }

  SpillStaticRegs(TMP1, true, GPRSpillMask, FPRSpillMask);

  // Now that we are spilled, store in the state that we are in a syscall
  // Still without overwriting registers that matter
  // 16bit LoadConstant to be a single instruction
  // This gives the signal handler a value to check to see if we are in a syscall at all
  LoadConstant(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::r0, GPRSpillMask & 0xFFFF);
  str(ARMEmitter::XReg::x0, STATE, offsetof(FEXCore::Core::CpuStateFrame, InSyscallInfo));

  uint64_t SPOffset = AlignUp(FEXCore::HLE::SyscallArguments::MAX_ARGS * 8, 16);
  sub(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::rsp, ARMEmitter::Reg::rsp, SPOffset);
  for (uint32_t i = 0; i < FEXCore::HLE::SyscallArguments::MAX_ARGS; ++i) {
    if (Op->Header.Args[i].IsInvalid()) continue;
    str(GetReg(Op->Header.Args[i].ID()).X(), ARMEmitter::Reg::rsp, i * 8);
  }

  ldr(ARMEmitter::XReg::x0, STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.Common.SyscallHandlerObj));
  ldr(ARMEmitter::XReg::x3, STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.Common.SyscallHandlerFunc));
  mov(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::r1, STATE.R());

  // SP supporting move
  add(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::r2, ARMEmitter::Reg::rsp, 0);
  if (!CTX->Config.DisableVixlIndirectCalls) [[unlikely]] {
    GenerateIndirectRuntimeCall<uint64_t, void*, void*, void*>(ARMEmitter::Reg::r3);
  }
  else {
    blr(ARMEmitter::Reg::r3);
  }

  add(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::rsp, ARMEmitter::Reg::rsp, SPOffset);

  if ((Flags & FEXCore::IR::SyscallFlags::NORETURN) != FEXCore::IR::SyscallFlags::NORETURN) {
    // Result is now in x0
    // Fix the stack and any values that were stepped on
    FillStaticRegs(true, GPRSpillMask, FPRSpillMask);

    // Now the registers we've spilled are back in their original host registers
    // We can safely claim we are no longer in a syscall
    str(ARMEmitter::XReg::zr, STATE, offsetof(FEXCore::Core::CpuStateFrame, InSyscallInfo));

    PopDynamicRegsAndLR();

    if ((Flags & FEXCore::IR::SyscallFlags::NORETURNEDRESULT) != FEXCore::IR::SyscallFlags::NORETURNEDRESULT) {
      // Move result to its destination register.
      // Only if `NORETURNEDRESULT` wasn't set, otherwise we might overwrite the CPUState refilled with `FillStaticRegs`
      mov(ARMEmitter::Size::i64Bit, GetReg(Node), ARMEmitter::Reg::r0);
    }
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
  const static std::array<ARMEmitter::XRegister, FEXCore::HLE::SyscallArguments::MAX_ARGS-1> RegArgs = {{
    ARMEmitter::XReg::x0, ARMEmitter::XReg::x1, ARMEmitter::XReg::x2, ARMEmitter::XReg::x3, ARMEmitter::XReg::x4, ARMEmitter::XReg::x5
  }};

  bool Intersects{};
  // We always need to spill x8 since we can't know if it is live at this SSA location
  uint32_t SpillMask = 1U << 8;
  for (uint32_t i = 0; i < FEXCore::HLE::SyscallArguments::MAX_ARGS-1; ++i) {
    if (Op->Header.Args[i].IsInvalid()) break;

    auto Reg = GetReg(Op->Header.Args[i].ID());
    if (Reg == ARMEmitter::Reg::r8 ||
        Reg == ARMEmitter::Reg::r4 ||
        Reg == ARMEmitter::Reg::r5) {

      SpillMask |= (1U << Reg.Idx());
      Intersects = true;
    }
  }

  // Ordering is incredibly important here
  // We must spill any overlapping registers first THEN claim we are in a syscall without invalidating state at all
  // Only spill the registers that intersect with our usage
  SpillStaticRegs(TMP1, false, SpillMask);

  // Now that we are spilled, store in the state that we are in a syscall
  // Still without overwriting registers that matter
  // 16bit LoadConstant to be a single instruction
  // We must always spill at least one register (x8) so this value always has a bit set
  // This gives the signal handler a value to check to see if we are in a syscall at all
  LoadConstant(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::r0, SpillMask & 0xFFFF);
  str(ARMEmitter::XReg::x0, STATE, offsetof(FEXCore::Core::CpuStateFrame, InSyscallInfo));

  // Now that we have claimed to be a syscall we can set up the arguments
  const auto EmitSize = CTX->Config.Is64BitMode() ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;
  const auto EmitSubSize = CTX->Config.Is64BitMode() ? ARMEmitter::SubRegSize::i64Bit : ARMEmitter::SubRegSize::i32Bit;
  if (Intersects) {
    for (uint32_t i = 0; i < FEXCore::HLE::SyscallArguments::MAX_ARGS-1; ++i) {
      if (Op->Header.Args[i].IsInvalid()) break;

      auto Reg = GetReg(Op->Header.Args[i].ID());
      // In the case of intersection with x4, x5, or x8 then these are currently SRA
      // for registers RAX, RBX, and RSI. Which have just been spilled
      // Just load back from the context. Could be slightly smarter but this is fairly uncommon
      if (Reg == ARMEmitter::Reg::r8) {
        ldr(EmitSubSize, RegArgs[i].R(), STATE, offsetof(FEXCore::Core::CpuStateFrame, State.gregs[X86State::REG_RSP]));
      }
      else if (Reg == ARMEmitter::Reg::r4) {
        ldr(EmitSubSize, RegArgs[i].R(), STATE, offsetof(FEXCore::Core::CpuStateFrame, State.gregs[X86State::REG_RAX]));
      }
      else if (Reg == ARMEmitter::Reg::r5) {
        ldr(EmitSubSize, RegArgs[i].R(), STATE, offsetof(FEXCore::Core::CpuStateFrame, State.gregs[X86State::REG_RCX]));
      }
      else {
        mov(EmitSize, RegArgs[i].R(), Reg);
      }
    }
  }
  else {
    for (uint32_t i = 0; i < FEXCore::HLE::SyscallArguments::MAX_ARGS-1; ++i) {
      if (Op->Header.Args[i].IsInvalid()) break;

      mov(EmitSize, RegArgs[i].R(), GetReg(Op->Header.Args[i].ID()));
    }
  }

  LoadConstant(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::r8, Op->HostSyscallNumber);
  svc(0);
  // On updated signal mask we can receive a signal RIGHT HERE

  if ((Op->Flags & FEXCore::IR::SyscallFlags::NORETURN) != FEXCore::IR::SyscallFlags::NORETURN) {
    // Now that we are done in the syscall we need to carefully peel back the state
    // First unspill the registers from before
    FillStaticRegs(false, SpillMask);

    // Now the registers we've spilled are back in their original host registers
    // We can safely claim we are no longer in a syscall
    str(ARMEmitter::XReg::zr, STATE, offsetof(FEXCore::Core::CpuStateFrame, InSyscallInfo));

    // Result is now in x0
    // Move result to its destination register
    mov(EmitSize, GetReg(Node), ARMEmitter::Reg::r0);
  }
}

DEF_OP(Thunk) {
  auto Op = IROp->C<IR::IROp_Thunk>();
  // Arguments are passed as follows:
  // X0: CTX
  // X1: Args (from guest stack)

  SpillStaticRegs(TMP1); // spill to ctx before ra64 spill

  PushDynamicRegsAndLR(TMP1);

  mov(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::r0, GetReg(Op->ArgPtr.ID()));

  auto thunkFn = static_cast<Context::ContextImpl*>(ThreadState->CTX)->ThunkHandler->LookupThunk(Op->ThunkNameHash);
  LoadConstant(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::r2, (uintptr_t)thunkFn);
  if (!CTX->Config.DisableVixlIndirectCalls) [[unlikely]] {
    GenerateIndirectRuntimeCall<void, void*, void*>(ARMEmitter::Reg::r2);
  }
  else {
    blr(ARMEmitter::Reg::r2);
  }

  PopDynamicRegsAndLR();

  FillStaticRegs(); // load from ctx after ra64 refill
}

DEF_OP(ValidateCode) {
  auto Op = IROp->C<IR::IROp_ValidateCode>();
  const auto *OldCode = (const uint8_t *)&Op->CodeOriginalLow;
  int len = Op->CodeLength;
  int idx = 0;

  LoadConstant(ARMEmitter::Size::i64Bit, GetReg(Node), 0);
  LoadConstant(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::r0, Entry + Op->Offset);
  LoadConstant(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::r1, 1);

  const auto Dst = GetReg(Node);

  while (len >= 8)
  {
    ldr(ARMEmitter::XReg::x2, ARMEmitter::Reg::r0, idx);
    LoadConstant(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::r3, *(const uint32_t *)(OldCode + idx));
    cmp(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::r2, ARMEmitter::Reg::r3);
    csel(ARMEmitter::Size::i64Bit, Dst, Dst, ARMEmitter::Reg::r1, ARMEmitter::Condition::CC_EQ);
    len -= 8;
    idx += 8;
  }
  while (len >= 4)
  {
    ldr(ARMEmitter::WReg::w2, ARMEmitter::Reg::r0, idx);
    LoadConstant(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::r3, *(const uint32_t *)(OldCode + idx));
    cmp(ARMEmitter::Size::i32Bit, ARMEmitter::Reg::r2, ARMEmitter::Reg::r3);
    csel(ARMEmitter::Size::i64Bit, Dst, Dst, ARMEmitter::Reg::r1, ARMEmitter::Condition::CC_EQ);
    len -= 4;
    idx += 4;
  }
  while (len >= 2)
  {
    ldrh(ARMEmitter::Reg::r2, ARMEmitter::Reg::r0, idx);
    LoadConstant(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::r3, *(const uint16_t *)(OldCode + idx));
    cmp(ARMEmitter::Size::i32Bit, ARMEmitter::Reg::r2, ARMEmitter::Reg::r3);
    csel(ARMEmitter::Size::i64Bit, Dst, Dst, ARMEmitter::Reg::r1, ARMEmitter::Condition::CC_EQ);
    len -= 2;
    idx += 2;
  }
  while (len >= 1)
  {
    ldrb(ARMEmitter::Reg::r2, ARMEmitter::Reg::r0, idx);
    LoadConstant(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::r3, *(const uint8_t *)(OldCode + idx));
    cmp(ARMEmitter::Size::i32Bit, ARMEmitter::Reg::r2, ARMEmitter::Reg::r3);
    csel(ARMEmitter::Size::i64Bit, Dst, Dst, ARMEmitter::Reg::r1, ARMEmitter::Condition::CC_EQ);
    len -= 1;
    idx += 1;
  }
}

DEF_OP(ThreadRemoveCodeEntry) {
  // Arguments are passed as follows:
  // X0: Thread
  // X1: RIP

  PushDynamicRegsAndLR(TMP1);
  SpillStaticRegs(TMP1);

  mov(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::r0, STATE.R());
  LoadConstant(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::r1, Entry);

  ldr(ARMEmitter::XReg::x2, STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.Common.ThreadRemoveCodeEntryFromJIT));
  if (!CTX->Config.DisableVixlIndirectCalls) [[unlikely]] {
    GenerateIndirectRuntimeCall<void, void*, void*>(ARMEmitter::Reg::r2);
  }
  else {
    blr(ARMEmitter::Reg::r2);
  }
  FillStaticRegs();

  // Fix the stack and any values that were stepped on
  PopDynamicRegsAndLR();
}

DEF_OP(CPUID) {
  auto Op = IROp->C<IR::IROp_CPUID>();

  PushDynamicRegsAndLR(TMP1);
  SpillStaticRegs(TMP1);

  // x0 = CPUID Handler
  // x1 = CPUID Function
  // x2 = CPUID Leaf
  ldr(ARMEmitter::XReg::x0, STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.Common.CPUIDObj));
  ldr(ARMEmitter::XReg::x3, STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.Common.CPUIDFunction));
  mov(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::r1, GetReg(Op->Function.ID()));
  mov(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::r2, GetReg(Op->Leaf.ID()));
  if (!CTX->Config.DisableVixlIndirectCalls) [[unlikely]] {
    GenerateIndirectRuntimeCall<__uint128_t, void*, uint64_t, uint64_t>(ARMEmitter::Reg::r3);
  }
  else {
    blr(ARMEmitter::Reg::r3);
  }

  FillStaticRegs();

  PopDynamicRegsAndLR();

  // Results are in x0, x1
  // Results want to be in a i64v2 vector
  auto Dst = GetRegPair(Node);
  mov(ARMEmitter::Size::i64Bit, Dst.first,  ARMEmitter::Reg::r0);
  mov(ARMEmitter::Size::i64Bit, Dst.second, ARMEmitter::Reg::r1);
}

DEF_OP(XGetBV) {
  auto Op = IROp->C<IR::IROp_XGetBV>();

  PushDynamicRegsAndLR(TMP1);
  SpillStaticRegs(TMP1);

  // x0 = CPUID Handler
  // x1 = XCR Function
  ldr(ARMEmitter::XReg::x0, STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.Common.CPUIDObj));
  ldr(ARMEmitter::XReg::x2, STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.Common.XCRFunction));
  mov(ARMEmitter::Size::i32Bit, ARMEmitter::Reg::r1, GetReg(Op->Function.ID()));
  if (!CTX->Config.DisableVixlIndirectCalls) [[unlikely]] {
    GenerateIndirectRuntimeCall<uint64_t, void*, uint32_t>(ARMEmitter::Reg::r2);
  }
  else {
    blr(ARMEmitter::Reg::r2);
  }

  FillStaticRegs();

  PopDynamicRegsAndLR();

  // Results are in x0
  // Results want to be in a i32v2 vector
  auto Dst = GetRegPair(Node);
  mov(ARMEmitter::Size::i32Bit, Dst.first,  ARMEmitter::Reg::r0);
  lsr(ARMEmitter::Size::i64Bit, Dst.second, ARMEmitter::Reg::r0, 32);
}

#undef DEF_OP
}

