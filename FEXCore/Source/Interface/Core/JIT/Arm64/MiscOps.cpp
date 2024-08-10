// SPDX-License-Identifier: MIT
/*
$info$
tags: backend|arm64
$end_info$
*/

#ifndef _WIN32
#include <syscall.h>
#endif

#include "Interface/Context/Context.h"
#include "Interface/Core/JIT/Arm64/JITClass.h"
#include "FEXCore/Debug/InternalThreadState.h"

#include <FEXCore/Core/SignalDelegator.h>

namespace FEXCore::CPU {
#define DEF_OP(x) void Arm64JITCore::Op_##x(IR::IROp_Header const* IROp, IR::NodeID Node)

DEF_OP(GuestOpcode) {
  auto Op = IROp->C<IR::IROp_GuestOpcode>();
  // metadata
  DebugData->GuestOpcodes.push_back({Op->GuestEntryOffset, GetCursorAddress<uint8_t*>() - CodeData.BlockBegin});
}

DEF_OP(Fence) {
  auto Op = IROp->C<IR::IROp_Fence>();
  switch (Op->Fence) {
  case IR::Fence_Load.Val: dmb(ARMEmitter::BarrierScope::LD); break;
  case IR::Fence_LoadStore.Val: dmb(ARMEmitter::BarrierScope::SY); break;
  case IR::Fence_Store.Val: dmb(ARMEmitter::BarrierScope::ST); break;
  default: LOGMAN_MSG_A_FMT("Unknown Fence: {}", Op->Fence); break;
  }
}

DEF_OP(Break) {
  auto Op = IROp->C<IR::IROp_Break>();

  // First we must reset the stack
  ResetStack();

  Core::CpuStateFrame::SynchronousFaultDataStruct State = {
    .FaultToTopAndGeneratedException = 1,
    .Signal = Op->Reason.Signal,
    .TrapNo = Op->Reason.TrapNumber,
    .si_code = Op->Reason.si_code,
    .err_code = Op->Reason.ErrorRegister,
  };

  uint64_t Constant {};
  memcpy(&Constant, &State, sizeof(State));

  LoadConstant(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::r1, Constant);
  str(ARMEmitter::XReg::x1, STATE, offsetof(FEXCore::Core::CpuStateFrame, SynchronousFaultData));

  switch (Op->Reason.Signal) {
  case Core::FAULT_SIGILL:
    ldr(TMP1, STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.Common.GuestSignal_SIGILL));
    br(TMP1);
    break;
  case Core::FAULT_SIGTRAP:
    ldr(TMP1, STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.Common.GuestSignal_SIGTRAP));
    br(TMP1);
    break;
  case Core::FAULT_SIGSEGV:
    ldr(TMP1, STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.Common.GuestSignal_SIGSEGV));
    br(TMP1);
    break;
  default:
    ldr(TMP1, STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.Common.GuestSignal_SIGTRAP));
    br(TMP1);
    break;
  }
}

DEF_OP(GetRoundingMode) {
  auto Dst = GetReg(Node);
  mrs(Dst, ARMEmitter::SystemRegister::FPCR);
  ubfx(ARMEmitter::Size::i64Bit, Dst, Dst, 22, 3);

  // FTZ is already in the correct location
  // Rounding mode is different
  //
  // Need to remap rounding mode from order nearest, pos inf, neg inf, toward
  // zero. Just swapping 01 and 10. That's a bitfield reverse. Round mode is in
  // bottom two bits. After reversing as a 32-bit operation, it'll be in [31:30]
  // and ripe for reinsertion back at 0.
  static_assert(IR::ROUND_MODE_NEAREST == 0);
  static_assert(IR::ROUND_MODE_NEGATIVE_INFINITY == 1);
  static_assert(IR::ROUND_MODE_POSITIVE_INFINITY == 2);
  static_assert(IR::ROUND_MODE_TOWARDS_ZERO == 3);

  rbit(ARMEmitter::Size::i32Bit, TMP1, Dst);
  bfi(ARMEmitter::Size::i64Bit, Dst, TMP1, 30, 2);
}

DEF_OP(SetRoundingMode) {
  auto Op = IROp->C<IR::IROp_SetRoundingMode>();
  auto Src = GetReg(Op->RoundMode.ID());
  auto MXCSR = GetReg(Op->MXCSR.ID());

  // As above, setup the rounding flags in [31:30]
  rbit(ARMEmitter::Size::i32Bit, TMP2, Src);
  // and extract
  lsr(ARMEmitter::Size::i32Bit, TMP2, TMP2, 30);

  mrs(TMP1, ARMEmitter::SystemRegister::FPCR);

  // vixl simulator doesn't support anything beyond ties-to-even rounding
  if (CTX->Config.DisableVixlIndirectCalls) [[likely]] {
    // Insert the rounding flags
    bfi(ARMEmitter::Size::i64Bit, TMP1, TMP2, 22, 2);
  }

  // Insert the FTZ flag
  lsr(ARMEmitter::Size::i64Bit, TMP2, Src, 2);
  bfi(ARMEmitter::Size::i64Bit, TMP1, TMP2, 24, 1);

  if (Op->SetDAZ && HostSupportsAFP) {
    // Extract DAZ from MXCSR and insert to in FPCR.FIZ
    bfxil(ARMEmitter::Size::i64Bit, TMP1, MXCSR, 6, 1);
  }

  // Now save the new FPCR
  msr(ARMEmitter::SystemRegister::FPCR, TMP1);
}

DEF_OP(PushRoundingMode) {
  auto Op = IROp->C<IR::IROp_PushRoundingMode>();
  auto Dest = GetReg(Node);

  // Save the old rounding mode
  mrs(Dest, ARMEmitter::SystemRegister::FPCR);

  // vixl simulator doesn't support anything beyond ties-to-even rounding
  if (!CTX->Config.DisableVixlIndirectCalls) [[unlikely]] {
    return;
  }

  // Insert the rounding flags, reversing the mode bits as above
  if (Op->RoundMode == 3) {
    orr(ARMEmitter::Size::i64Bit, TMP1, Dest, 3 << 22);
  } else if (Op->RoundMode == 0) {
    and_(ARMEmitter::Size::i64Bit, TMP1, Dest, ~(3 << 22));
  } else {
    LOGMAN_THROW_AA_FMT(Op->RoundMode == 1 || Op->RoundMode == 2, "expect a valid round mode");

    and_(ARMEmitter::Size::i64Bit, TMP1, Dest, ~(Op->RoundMode << 22));
    orr(ARMEmitter::Size::i64Bit, TMP1, TMP1, (Op->RoundMode == 2 ? 1 : 2) << 22);
  }

  // Now save the new FPCR
  msr(ARMEmitter::SystemRegister::FPCR, TMP1);
}

DEF_OP(PopRoundingMode) {
  auto Op = IROp->C<IR::IROp_PopRoundingMode>();
  msr(ARMEmitter::SystemRegister::FPCR, GetReg(Op->FPCR.ID()));
}

DEF_OP(Print) {
  auto Op = IROp->C<IR::IROp_Print>();

  PushDynamicRegsAndLR(TMP1);
  SpillStaticRegs(TMP1);

  if (IsGPR(Op->Value.ID())) {
    mov(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::r0, GetReg(Op->Value.ID()));
    ldr(ARMEmitter::XReg::x3, STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.Common.PrintValue));
  } else {
    fmov(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::r0, GetVReg(Op->Value.ID()), false);
    fmov(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::r1, GetVReg(Op->Value.ID()), true);
    ldr(ARMEmitter::XReg::x3, STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.Common.PrintVectorValue));
  }

  if (!CTX->Config.DisableVixlIndirectCalls) [[unlikely]] {
    if (IsGPR(Op->Value.ID())) {
      GenerateIndirectRuntimeCall<void, uint64_t>(ARMEmitter::Reg::r3);
    } else {
      GenerateIndirectRuntimeCall<void, uint64_t, uint64_t>(ARMEmitter::Reg::r3);
    }
  } else {
    blr(ARMEmitter::Reg::r3);
  }

  FillStaticRegs();
  PopDynamicRegsAndLR();
}

#ifndef _WIN32
DEF_OP(ProcessorID) {
  // We always need to spill x8 since we can't know if it is live at this SSA location
  uint32_t SpillMask = 1U << 8;

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

  // Allocate some temporary space for storing the uint32_t CPU and Node IDs
  sub(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::rsp, ARMEmitter::Reg::rsp, 16);

  // Load the getcpu syscall number
#if defined(_M_X86_64)
  // Just to ensure the syscall number doesn't change if compiled for an x86_64 host.
  constexpr auto GetCPUSyscallNum = 0xa8;
#else
  constexpr auto GetCPUSyscallNum = SYS_getcpu;
#endif
  LoadConstant(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::r8, GetCPUSyscallNum);

  // CPU pointer in x0
  add(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::r0, ARMEmitter::Reg::rsp, 0);
  // Node in x1
  add(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::r1, ARMEmitter::Reg::rsp, 4);

  svc(0);
  // On updated signal mask we can receive a signal RIGHT HERE

  // Load the values returned by the kernel
  ldp<ARMEmitter::IndexType::OFFSET>(ARMEmitter::WReg::w0, ARMEmitter::WReg::w1, ARMEmitter::Reg::rsp);
  // Deallocate stack space
  sub(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::rsp, ARMEmitter::Reg::rsp, 16);

  // Now that we are done in the syscall we need to carefully peel back the state
  // First unspill the registers from before
  FillStaticRegs(false, SpillMask, ~0U, ARMEmitter::Reg::r8, ARMEmitter::Reg::r2);

  // Now the registers we've spilled are back in their original host registers
  // We can safely claim we are no longer in a syscall
  str(ARMEmitter::XReg::zr, STATE, offsetof(FEXCore::Core::CpuStateFrame, InSyscallInfo));

  // Now store the result in the destination in the expected format
  // uint32_t Res = (node << 12) | cpu;
  // CPU is in w0
  // Node is in w1
  orr(ARMEmitter::Size::i64Bit, GetReg(Node), ARMEmitter::Reg::r0, ARMEmitter::Reg::r1, ARMEmitter::ShiftType::LSL, 12);
}
#else
DEF_OP(ProcessorID) {
  ERROR_AND_DIE_FMT("Unsupported");
}
#endif

DEF_OP(RDRAND) {
  auto Op = IROp->C<IR::IROp_RDRAND>();

  mrs(GetReg(Node), Op->GetReseeded ? ARMEmitter::SystemRegister::RNDRRS : ARMEmitter::SystemRegister::RNDR);
}

DEF_OP(Yield) {
  yield();
}

#undef DEF_OP
} // namespace FEXCore::CPU
