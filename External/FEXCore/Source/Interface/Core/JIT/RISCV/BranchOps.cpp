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
DEF_OP(GuestCallDirect) {
  LogMan::Msg::DFmt("Unimplemented");
}

DEF_OP(GuestCallIndirect) {
  LogMan::Msg::DFmt("Unimplemented");
}

DEF_OP(SignalReturn) {
  ERROR_AND_DIE_FMT("Nope: {}", __func__);
}

DEF_OP(CallbackReturn) {
  ERROR_AND_DIE_FMT("Nope: {}", __func__);
}

DEF_OP(ExitFunction) {
  auto Op = IROp->C<IR::IROp_ExitFunction>();

  Label FullLookup;

  ResetStack();

  GPR RipReg;
  uint64_t NewRIP;

  if (IsInlineConstant(Op->NewRIP, &NewRIP) || IsInlineEntrypointOffset(Op->NewRIP, &NewRIP)) {
    RipReg = TMP4;
    LoadConstant(RipReg, NewRIP);
    LogMan::Msg::DFmt("Next rip: 0x{:x}", NewRIP);
  }
  else {
    RipReg = GetReg(Op->Header.Args[0].ID());
  }

  {
    // L1 Cache
    LD(TMP1, offsetof(FEXCore::Core::CpuStateFrame, Pointers.RISCV.L1Pointer), STATE);

    LoadConstant(TMP2, LookupCache::L1_ENTRIES_MASK);
    AND(TMP2, RipReg, TMP2);
    SLLI64(TMP2, TMP2, 4);
    ADD(TMP1, TMP1, TMP2);

    // {HostPC, GuestRIP}
    LD(TMP2, 8, TMP1); // Load cached RIP in to t1
    LD(TMP1, 0, TMP1); // Load target in to t0
    BNE(TMP2, RipReg, &FullLookup);

    JALR(zero, 0, TMP1);

    Bind(&FullLookup);
    LD(TMP1, offsetof(FEXCore::Core::CpuStateFrame, Pointers.RISCV.DispatcherLoopTop), STATE);
    SD(RipReg, offsetof(FEXCore::Core::CpuStateFrame, State.rip), STATE);
    JALR(zero, 0, TMP1);
  }
}

DEF_OP(Jump) {
  const auto Op = IROp->C<IR::IROp_Jump>();
  const auto ArgID = Op->Args(0).ID();

  PendingTargetLabel = &JumpTargets.try_emplace(ArgID).first->second;
}

DEF_OP(CondJump) {
  ERROR_AND_DIE_FMT("Nope: {}", __func__);
}

DEF_OP(Syscall) {
  ERROR_AND_DIE_FMT("Nope: {}", __func__);
}

DEF_OP(InlineSyscall) {
  ERROR_AND_DIE_FMT("Nope: {}", __func__);
}

DEF_OP(Thunk) {
  ERROR_AND_DIE_FMT("Nope: {}", __func__);
}

DEF_OP(ValidateCode) {
  ERROR_AND_DIE_FMT("Nope: {}", __func__);
}

DEF_OP(RemoveCodeEntry) {
  ERROR_AND_DIE_FMT("Nope: {}", __func__);
}

DEF_OP(CPUID) {
  ERROR_AND_DIE_FMT("Nope: {}", __func__);
}

#undef DEF_OP
void RISCVJITCore::RegisterBranchHandlers() {
#define REGISTER_OP(op, x) OpHandlers[FEXCore::IR::IROps::OP_##op] = &RISCVJITCore::Op_##x
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
  REGISTER_OP(REMOVECODEENTRY,   RemoveCodeEntry);
  REGISTER_OP(CPUID,             CPUID);
#undef REGISTER_OP
}
}

