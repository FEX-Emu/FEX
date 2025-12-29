// SPDX-License-Identifier: MIT
/*
$info$
tags: backend|arm64
$end_info$
*/

#include "Interface/Context/Context.h"
#include "FEXCore/IR/IR.h"
#include "Interface/Core/LookupCache.h"

#include "Interface/Core/JIT/JITClass.h"

#include <FEXCore/Core/Thunks.h>
#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/Debug/InternalThreadState.h>
#include <FEXCore/HLE/SyscallHandler.h>
#include <FEXCore/Utils/MathUtils.h>

namespace FEXCore::CPU {

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

  if (CTX->HostFeatures.IsInstCountCI) [[unlikely]] {
    // Emit function end marker
    udf(0x420F);
  }

  uint64_t NewRIP;

  if (IsInlineConstant(Op->NewRIP, &NewRIP) || IsInlineEntrypointOffset(Op->NewRIP, &NewRIP)) {
#ifdef _M_ARM_64EC
    if (NewRIP < EC_CODE_BITMAP_MAX_ADDRESS && RtlIsEcCode(NewRIP)) {
      str(REG_CALLRET_SP, STATE_PTR(CpuStateFrame, State.callret_sp));
      add(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::rsp, StaticRegisters[X86State::REG_RSP], 0);
      InsertGuestRIPMove(EC_CALL_CHECKER_PC_REG, NewRIP);
      ldr(TMP2, STATE_PTR(CpuStateFrame, Pointers.Common.ExitFunctionEC));
      br(TMP2);
    } else {
#endif
      // In order to support direct branches without constantly hitting the L1 cache, we emit a call to a block linker,
      // this will compile the branch target block when it is hit and replace the branch to the linker at the callsite
      // with a direct branch to the destination block. Upon invalidation of the target block the backpatch is undone.
      //
      // In addition, to avoid needing to lookup in the cache for returns and any indirect branch prediction penalty,
      // a shadow stack of <GuestReturnRIP, HostReturnPC> pairs is maintained, acting as a first level cache for any
      // return operations. As the guest may not balance calls and returns exactly, an exception handler is expected to
      // be installed by the frontend, to reset the shadow stack to the middle of its valid bounds on overflow/underflow.
      // This shadow stack is also cleared on block invalidation operations or codebuffer switches, to ensure all pointed-to
      // host code is always valid.

      // This code will be backpatched by Arm64JITCore_ExitFunctionLink, below is an enumeration of all the possible cases.
      // Jump thunks are emitted in JIT.cpp after compilation of the entire multiblock.
      //
      // Call with known return block - unlinked
      //    00: adr TMP1, 0xC
      //    04: stp RetReg, TMP1, [SpReg, -0x10]!
      //    08: bl JmpThunk00
      //    JmpThunk00:
      //    00: b 0x8
      //    04: br TMP1
      //    08: ldr TMP1, <Shared exit linker>
      //    0c: blr TMP1
      //    10: HostCode
      //    18: GuestRIP
      //    20: CallerOffset
      //
      // Call with known return block after backpatching - linked in branch immediate range
      //    00: adr TMP1, 0xC
      //    04: stp RetReg, TMP1, [SpReg, -0x10]!
      //    08: bl HostCode                                        - MODIFIED
      //
      // Call with known return block after backpatching - linked out of range
      //    00: adr TMP1, 0xC
      //    04: stp RetReg, TMP1, [SpReg, -0x10]!
      //    08: bl JmpThunk00
      //    JmpThunk00:
      //    00: ldr TMP1, 0x10                                     - MODIFIED 2nd
      //    04: br TMP1
      //    08: ldr TMP1, <Shared exit linker>
      //    0c: blr TMP1
      //    10: HostCode                                           - MODIFIED 1st
      //    18: GuestRIP
      //    20: CallerOffset
      //
      // Jump - unlinked
      //    00: b JmpThunk00
      //    JmpThunk00:
      //    00: b 0x8
      //    04: br TMP1
      //    08: ldr TMP1, <Shared exit linker>
      //    0c: blr TMP1
      //    10: HostCode
      //    18: GuestRIP
      //    20: CallerOffset
      //
      // Jump after backpatching - linked in branch immediate range
      //    00: b HostCode                                         - MODIFIED
      //
      // Jump after backpatching - linked out of range
      //    00: b JmpThunk00
      //    JmpThunk00:
      //    00: ldr TMP1, 0x10                                     - MODIFIED 2nd
      //    04: br TMP1
      //    08: ldr TMP1, <Shared exit linker>
      //    0c: blr TMP1
      //    10: HostCode                                           - MODIFIED 1st
      //    18: GuestRIP
      //    20: CallerOffset

      ARMEmitter::ForwardLabel l_BranchHost;
      ARMEmitter::ForwardLabel l_CallReturn;
      if (Op->Hint == IR::BranchHint::Call) {
        if (!Op->CallReturnBlock.IsInvalid()) {
          auto CallReturnAddressReg = GetReg(Op->CallReturnAddress).X();
          PendingCallReturnTargetLabel = &CallReturnTargets.try_emplace(Op->CallReturnBlock.ID()).first->second;
          (void)adr(TMP1, &l_CallReturn);
          stp<ARMEmitter::IndexType::PRE>(CallReturnAddressReg, TMP1, REG_CALLRET_SP, -0x10);
        } else {
          stp<ARMEmitter::IndexType::PRE>(ARMEmitter::XReg::zr, ARMEmitter::XReg::zr, REG_CALLRET_SP, -0x10);
        }
      } else if (Op->Hint == IR::BranchHint::CheckTF) {
        ARMEmitter::ForwardLabel TFUnset;
        ldrb(TMP1, STATE_PTR(CpuStateFrame, State.flags[X86State::RFLAG_TF_RAW_LOC]));
        (void)cbz(ARMEmitter::Size::i32Bit, TMP1, &TFUnset);
        InsertGuestRIPMove(TMP1, NewRIP);
        str(TMP1, STATE, offsetof(FEXCore::Core::CpuStateFrame, State.rip));
        ldr(TMP2, STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.Common.DispatcherLoopTop));
        blr(TMP2);
        (void)Bind(&TFUnset);
      }

      EmitLinkedBranch(NewRIP, Op->Hint == IR::BranchHint::Call);
      (void)Bind(&l_CallReturn);
#ifdef _M_ARM_64EC
    }
#endif
  } else {
    ARMEmitter::ForwardLabel SkipFullLookup;
    auto RipReg = GetReg(Op->NewRIP);

    if (Op->Hint == IR::BranchHint::Return) {
      // First try to pop from the call-ret stack, otherwise follow the normal path (but ending in a ret)
      ldp<ARMEmitter::IndexType::POST>(TMP1, TMP2, REG_CALLRET_SP, 0x10);
      sub(TMP1, TMP1, RipReg.X());
      (void)cbz(ARMEmitter::Size::i64Bit, TMP1, &SkipFullLookup);
    }

    // L1 Cache
    ldp<ARMEmitter::IndexType::OFFSET>(TMP1, TMP2, STATE, offsetof(FEXCore::Core::CpuStateFrame, State.L1Pointer));

    // Calculate (tmp1 + ((ripreg & L1_ENTRIES_MASK) << 4)) for the address
    // L1Mask is pre-shifted.
    and_(ARMEmitter::Size::i64Bit, TMP2, TMP2, RipReg, ARMEmitter::ShiftType::LSL, FEXCore::ilog2(sizeof(LookupCache::LookupCacheEntry)));
    add(TMP1, TMP1, TMP2);

    ldp<ARMEmitter::IndexType::OFFSET>(TMP2, TMP1, TMP1, 0);

    // Note: sub+cbnz used over cmp+br to preserve flags.
    sub(TMP1, TMP1, RipReg.X());
    (void)cbz(ARMEmitter::Size::i64Bit, TMP1, &SkipFullLookup);
    ldr(TMP2, STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.Common.DispatcherLoopTop));
    str(RipReg.X(), STATE, offsetof(FEXCore::Core::CpuStateFrame, State.rip));

    (void)Bind(&SkipFullLookup);
    if (Op->Hint == IR::BranchHint::Call) {
      ARMEmitter::ForwardLabel l_CallReturn;
      if (!Op->CallReturnBlock.IsInvalid()) {
        auto CallReturnAddressReg = GetReg(Op->CallReturnAddress).X();
        PendingCallReturnTargetLabel = &CallReturnTargets.try_emplace(Op->CallReturnBlock.ID()).first->second;
        (void)adr(TMP1, &l_CallReturn);
        stp<ARMEmitter::IndexType::PRE>(CallReturnAddressReg, TMP1, REG_CALLRET_SP, -0x10);
      } else {
        stp<ARMEmitter::IndexType::PRE>(ARMEmitter::XReg::zr, ARMEmitter::XReg::zr, REG_CALLRET_SP, -0x10);
      }
      blr(TMP2);
      (void)Bind(&l_CallReturn);
    } else if (Op->Hint == IR::BranchHint::Return) {
      ret(TMP2);
    } else {
      br(TMP2);
    }
  }
}

DEF_OP(Jump) {
  const auto Op = IROp->C<IR::IROp_Jump>();

  PendingTargetLabel = JumpTarget(Op->TargetBlock);
}

DEF_OP(CondJump) {
  auto Op = IROp->C<IR::IROp_CondJump>();

  auto TrueTargetLabel = JumpTarget(Op->TrueBlock);

  if (Op->FromNZCV) {
    b_OrRestart(MapCC(Op->Cond), TrueTargetLabel);
  } else {
    uint64_t Const;
    const bool isConst = IsInlineConstant(Op->Cmp2, &Const);

    auto Reg = GetReg(Op->Cmp1);
    const auto Size = Op->CompareSize == IR::OpSize::i32Bit ? ARMEmitter::Size::i32Bit : ARMEmitter::Size::i64Bit;

    LOGMAN_THROW_A_FMT(IsGPR(Op->Cmp1), "CondJump: Expected GPR");
    LOGMAN_THROW_A_FMT(isConst, "CondJump: Expected constant source");

    if (Op->Cond == IR::CondClass::EQ) {
      LOGMAN_THROW_A_FMT(Const == 0, "CondJump: Expected 0 source");
      cbz_OrRestart(Size, Reg, TrueTargetLabel);
    } else if (Op->Cond == IR::CondClass::NEQ) {
      LOGMAN_THROW_A_FMT(Const == 0, "CondJump: Expected 0 source");
      cbnz_OrRestart(Size, Reg, TrueTargetLabel);
    } else if (Op->Cond == IR::CondClass::TSTZ) {
      LOGMAN_THROW_A_FMT(Const < 64, "CondJump: Expected valid bit source");
      tbz_OrRestart(Reg, Const, TrueTargetLabel);
    } else if (Op->Cond == IR::CondClass::TSTNZ) {
      LOGMAN_THROW_A_FMT(Const < 64, "CondJump: Expected valid bit source");
      tbnz_OrRestart(Reg, Const, TrueTargetLabel);
    } else {
      LOGMAN_THROW_A_FMT(false, "CondJump expected simple condition");
    }
  }

  PendingTargetLabel = JumpTarget(Op->FalseBlock);
}

DEF_OP(Syscall) {
  auto Op = IROp->C<IR::IROp_Syscall>();
  // Arguments are passed as follows:
  // X0: SyscallHandler
  // X1: ThreadState
  // X2: Pointer to SyscallArguments

  PushDynamicRegs(TMP1);

  uint32_t GPRSpillMask = ~0U;
  uint32_t FPRSpillMask = ~0U;

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
    if (Op->Header.Args[i].IsInvalid()) {
      continue;
    }
    str(GetReg(Op->Header.Args[i]).X(), ARMEmitter::Reg::rsp, i * 8);
  }

  ldr(ARMEmitter::XReg::x0, STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.Common.SyscallHandlerObj));
  ldr(ARMEmitter::XReg::x3, STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.Common.SyscallHandlerFunc));
  mov(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::r1, STATE.R());

  // SP supporting move
  add(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::r2, ARMEmitter::Reg::rsp, 0);
  if (!CTX->Config.DisableVixlIndirectCalls) [[unlikely]] {
    GenerateIndirectRuntimeCall<uint64_t, void*, void*, void*>(ARMEmitter::Reg::r3);
  } else {
    blr(ARMEmitter::Reg::r3);
  }

  add(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::rsp, ARMEmitter::Reg::rsp, SPOffset);

  // Result is now in x0
  // Fix the stack and any values that were stepped on
  FillStaticRegs(true, GPRSpillMask, FPRSpillMask, ARMEmitter::Reg::r1, ARMEmitter::Reg::r2);

  // Now the registers we've spilled are back in their original host registers
  // We can safely claim we are no longer in a syscall
  str(ARMEmitter::XReg::zr, STATE, offsetof(FEXCore::Core::CpuStateFrame, InSyscallInfo));

  PopDynamicRegs();

  const auto OSABI = CTX->SyscallHandler->GetOSABI();

  if (OSABI != FEXCore::HLE::SyscallOSABI::OS_GENERIC) {
    // Move result to its destination register.
    // Only if `NORETURNEDRESULT` wasn't set, otherwise we might overwrite the CPUState refilled with `FillStaticRegs`
    mov(ARMEmitter::Size::i64Bit, GetReg(Node), ARMEmitter::Reg::r0);
  }
}

DEF_OP(Thunk) {
  auto Op = IROp->C<IR::IROp_Thunk>();
  // Arguments are passed as follows:
  // X0: CTX
  // X1: Args (from guest stack)

  SpillStaticRegs(TMP1); // spill to ctx before ra64 spill

  PushDynamicRegs(TMP1);

  mov(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::r0, GetReg(Op->ArgPtr));

  InsertNamedThunkRelocation(ARMEmitter::Reg::r2, Op->ThunkNameHash);
  if (!CTX->Config.DisableVixlIndirectCalls) [[unlikely]] {
    GenerateIndirectRuntimeCall<void, void*, void*>(ARMEmitter::Reg::r2);
  } else {
    blr(ARMEmitter::Reg::r2);
  }

  PopDynamicRegs();

  FillStaticRegs(); // load from ctx after ra64 refill
}

DEF_OP(ValidateCode) {
  auto Op = IROp->C<IR::IROp_ValidateCode>();
  auto OldCode = Op->CodeOriginal.data();
  auto Base = GetReg(Op->Header.Args[0]).X();
  int len = Op->CodeLength;
  int Offset = 0;
  ARMEmitter::ForwardLabel Fail;

  const auto Dst = GetReg(Node);

  auto EmitCheck = [&](size_t Size, auto&& LoadData) {
    while (len >= Size) {
      LoadData();
      sub(ARMEmitter::Size::i64Bit, TMP1, TMP1, TMP2);
      cbnz_OrRestart(ARMEmitter::Size::i64Bit, TMP1, &Fail);
      len -= Size;
      Offset += Size;
    }
  };

  EmitCheck(8, [&]() {
    ldr(TMP1, Base, Offset);
    LoadConstant(ARMEmitter::Size::i64Bit, TMP2, *(const uint64_t*)(OldCode + Offset));
  });

  EmitCheck(4, [&]() {
    ldr(TMP1.W(), Base, Offset);
    LoadConstant(ARMEmitter::Size::i32Bit, TMP2, *(const uint32_t*)(OldCode + Offset));
  });

  EmitCheck(2, [&]() {
    ldrh(TMP1.W(), Base, Offset);
    LoadConstant(ARMEmitter::Size::i32Bit, TMP2, *(const uint16_t*)(OldCode + Offset));
  });

  EmitCheck(1, [&]() {
    ldrb(TMP1.W(), Base, Offset);
    LoadConstant(ARMEmitter::Size::i32Bit, TMP2, *(const uint8_t*)(OldCode + Offset));
  });

  ARMEmitter::ForwardLabel End;
  LoadConstant(ARMEmitter::Size::i32Bit, Dst, 0);
  b_OrRestart(&End);
  BindOrRestart(&Fail);
  LoadConstant(ARMEmitter::Size::i32Bit, Dst, 1);
  BindOrRestart(&End);
}

DEF_OP(ThreadRemoveCodeEntry) {
  PushDynamicRegs(TMP4);
  SpillStaticRegs(TMP4);

  // Arguments are passed as follows:
  // X0: Thread
  // X1: RIP
  mov(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::r0, STATE.R());

  // TODO: Relocations don't seem to be wired up to this...?
  LoadConstant(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::r1, Entry, CPU::Arm64Emitter::PadType::AUTOPAD);

  ldr(ARMEmitter::XReg::x2, STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.Common.ThreadRemoveCodeEntryFromJIT));
  if (!CTX->Config.DisableVixlIndirectCalls) [[unlikely]] {
    GenerateIndirectRuntimeCall<void, void*, void*>(ARMEmitter::Reg::r2);
  } else {
    blr(ARMEmitter::Reg::r2);
  }
  FillStaticRegs();

  // Fix the stack and any values that were stepped on
  PopDynamicRegs();
}

DEF_OP(CPUID) {
  auto Op = IROp->C<IR::IROp_CPUID>();

  isb();
  mov(ARMEmitter::Size::i64Bit, TMP2, GetReg(Op->Function));
  mov(ARMEmitter::Size::i64Bit, TMP3, GetReg(Op->Leaf));

  PushDynamicRegs(TMP4);
  SpillStaticRegs(TMP4);

  // x0 = CPUID Handler
  // x1 = CPUID Function
  // x2 = CPUID Leaf
  ldr(ARMEmitter::XReg::x0, STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.Common.CPUIDObj));
  ldr(ARMEmitter::XReg::x3, STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.Common.CPUIDFunction));

  if (!TMP_ABIARGS) {
    mov(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::r1, TMP2);
    mov(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::r2, TMP3);
  }

  if (!CTX->Config.DisableVixlIndirectCalls) [[unlikely]] {
    GenerateIndirectRuntimeCall<__uint128_t, void*, uint64_t, uint64_t>(ARMEmitter::Reg::r3);
  } else {
    blr(ARMEmitter::Reg::r3);
  }

  if (!TMP_ABIARGS) {
    mov(ARMEmitter::Size::i64Bit, TMP1, ARMEmitter::Reg::r0);
    mov(ARMEmitter::Size::i64Bit, TMP2, ARMEmitter::Reg::r1);
  }

  FillStaticRegs();

  PopDynamicRegs();

  // Results are in x0, x1
  // Results want to be 4xi32 scalars
  mov(ARMEmitter::Size::i32Bit, GetReg(Op->OutEAX), TMP1);
  mov(ARMEmitter::Size::i32Bit, GetReg(Op->OutECX), TMP2);
  ubfx(ARMEmitter::Size::i64Bit, GetReg(Op->OutEBX), TMP1, 32, 32);
  ubfx(ARMEmitter::Size::i64Bit, GetReg(Op->OutEDX), TMP2, 32, 32);
}

DEF_OP(XGetBV) {
  auto Op = IROp->C<IR::IROp_XGetBV>();

  PushDynamicRegs(TMP4);
  SpillStaticRegs(TMP4);

  mov(ARMEmitter::Size::i32Bit, ARMEmitter::Reg::r1, GetReg(Op->Function));

  // x0 = CPUID Handler
  // x1 = XCR Function
  ldr(ARMEmitter::XReg::x0, STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.Common.CPUIDObj));
  ldr(ARMEmitter::XReg::x2, STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.Common.XCRFunction));
  if (!CTX->Config.DisableVixlIndirectCalls) [[unlikely]] {
    GenerateIndirectRuntimeCall<uint64_t, void*, uint32_t>(ARMEmitter::Reg::r2);
  } else {
    blr(ARMEmitter::Reg::r2);
  }

  if (!TMP_ABIARGS) {
    mov(ARMEmitter::Size::i64Bit, TMP1, ARMEmitter::Reg::r0);
  }

  FillStaticRegs();

  PopDynamicRegs();

  // Results are in x0, need to split into i32 parts
  mov(ARMEmitter::Size::i32Bit, GetReg(Op->OutEAX), TMP1);
  ubfx(ARMEmitter::Size::i64Bit, GetReg(Op->OutEDX), TMP1, 32, 32);
}

} // namespace FEXCore::CPU
