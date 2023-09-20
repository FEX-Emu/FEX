// SPDX-License-Identifier: MIT
/*
$info$
tags: backend|arm64
$end_info$
*/

#ifndef _WIN32
#include <syscall.h>
#endif

#include "Interface/Core/ArchHelpers/CodeEmitter/Emitter.h"
#include "Interface/Core/JIT/Arm64/JITClass.h"
#include "FEXCore/Debug/InternalThreadState.h"

#include <FEXCore/Core/SignalDelegator.h>

namespace FEXCore::CPU {
#define DEF_OP(x) void Arm64JITCore::Op_##x(IR::IROp_Header const *IROp, IR::NodeID Node)

DEF_OP(GuestOpcode) {
  auto Op = IROp->C<IR::IROp_GuestOpcode>();
  // metadata
  DebugData->GuestOpcodes.push_back({Op->GuestEntryOffset, GetCursorAddress<uint8_t*>() - CodeData.BlockBegin});
}

DEF_OP(Fence) {
  auto Op = IROp->C<IR::IROp_Fence>();
  switch (Op->Fence) {
    case IR::Fence_Load.Val:
      dmb(FEXCore::ARMEmitter::BarrierScope::LD);
      break;
    case IR::Fence_LoadStore.Val:
      dmb(FEXCore::ARMEmitter::BarrierScope::SY);
      break;
    case IR::Fence_Store.Val:
      dmb(FEXCore::ARMEmitter::BarrierScope::ST);
      break;
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

  uint64_t Constant{};
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
  lsr(ARMEmitter::Size::i64Bit, Dst, Dst, 22);

  // FTZ is already in the correct location
  // Rounding mode is different
  and_(ARMEmitter::Size::i64Bit, TMP1, Dst, 0b11);

  cmp(ARMEmitter::Size::i64Bit, TMP1, 1);
  LoadConstant(ARMEmitter::Size::i64Bit, TMP3, IR::ROUND_MODE_POSITIVE_INFINITY);
  csel(ARMEmitter::Size::i64Bit, TMP2, TMP3, ARMEmitter::Reg::zr, ARMEmitter::Condition::CC_EQ);

  cmp(ARMEmitter::Size::i64Bit, TMP1, 2);
  LoadConstant(ARMEmitter::Size::i64Bit, TMP3, IR::ROUND_MODE_NEGATIVE_INFINITY);
  csel(ARMEmitter::Size::i64Bit, TMP2, TMP3, TMP2, ARMEmitter::Condition::CC_EQ);

  cmp(ARMEmitter::Size::i64Bit, TMP1, 3);
  LoadConstant(ARMEmitter::Size::i64Bit, TMP3, IR::ROUND_MODE_TOWARDS_ZERO);
  csel(ARMEmitter::Size::i64Bit, TMP2, TMP3, TMP2, ARMEmitter::Condition::CC_EQ);

  orr(ARMEmitter::Size::i64Bit, Dst, Dst, TMP2.R());

  bfi(ARMEmitter::Size::i64Bit, Dst, TMP2, 0, 2);
}

DEF_OP(SetRoundingMode) {
  auto Op = IROp->C<IR::IROp_SetRoundingMode>();
  auto Src = GetReg(Op->RoundMode.ID());

  // Setup the rounding flags correctly
  and_(ARMEmitter::Size::i64Bit, TMP1, Src, 0b11);

  cmp(ARMEmitter::Size::i64Bit, TMP1, IR::ROUND_MODE_POSITIVE_INFINITY);
  LoadConstant(ARMEmitter::Size::i64Bit, TMP3, 1);
  csel(ARMEmitter::Size::i64Bit, TMP2, TMP3, ARMEmitter::Reg::zr, ARMEmitter::Condition::CC_EQ);

  cmp(ARMEmitter::Size::i64Bit, TMP1, IR::ROUND_MODE_NEGATIVE_INFINITY);
  LoadConstant(ARMEmitter::Size::i64Bit, TMP3, 2);
  csel(ARMEmitter::Size::i64Bit, TMP2, TMP3, TMP2, ARMEmitter::Condition::CC_EQ);

  cmp(ARMEmitter::Size::i64Bit, TMP1, IR::ROUND_MODE_TOWARDS_ZERO);
  LoadConstant(ARMEmitter::Size::i64Bit, TMP3, 3);
  csel(ARMEmitter::Size::i64Bit, TMP2, TMP3, TMP2, ARMEmitter::Condition::CC_EQ);

  mrs(TMP1, ARMEmitter::SystemRegister::FPCR);

  // vixl simulator doesn't support anything beyond ties-to-even rounding
#ifndef VIXL_SIMULATOR
  // Insert the rounding flags
  bfi(ARMEmitter::Size::i64Bit, TMP1, TMP2, 22, 2);
#endif

  // Insert the FTZ flag
  lsr(ARMEmitter::Size::i64Bit, TMP2, Src, 2);
  bfi(ARMEmitter::Size::i64Bit, TMP1, TMP2, 24, 1);

  // Now save the new FPCR
  msr(ARMEmitter::SystemRegister::FPCR, TMP1);
}

DEF_OP(Print) {
  auto Op = IROp->C<IR::IROp_Print>();

  PushDynamicRegsAndLR(TMP1);
  SpillStaticRegs(TMP1);

  if (IsGPR(Op->Value.ID())) {
    mov(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::r0, GetReg(Op->Value.ID()));
    ldr(ARMEmitter::XReg::x3, STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.Common.PrintValue));
  }
  else {
    fmov(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::r0, GetVReg(Op->Value.ID()), false);
    fmov(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::r1, GetVReg(Op->Value.ID()), true);
    ldr(ARMEmitter::XReg::x3, STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.Common.PrintVectorValue));
  }

  blr(ARMEmitter::Reg::r3);

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
  LoadConstant(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::r8, SYS_getcpu);

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
  FillStaticRegs(false, SpillMask);

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

  // Results are in x0, x1
  // Results want to be in a i64v2 vector
  auto Dst = GetRegPair(Node);

  if (Op->GetReseeded) {
    mrs(Dst.first, ARMEmitter::SystemRegister::RNDRRS);
  }
  else {
    mrs(Dst.first, ARMEmitter::SystemRegister::RNDR);
  }

  // If the rng number is valid then NZCV is 0b0000, otherwise NZCV is 0b0100
  cset(ARMEmitter::Size::i64Bit, Dst.second, ARMEmitter::Condition::CC_NE);
}

DEF_OP(Yield) {
  yield();
}

#undef DEF_OP
}

