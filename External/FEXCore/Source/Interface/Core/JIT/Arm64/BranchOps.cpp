#include "Interface/Core/JIT/Arm64/JITClass.h"

namespace FEXCore::CPU {
static uint64_t SyscallThunk(FEXCore::SyscallHandler *Handler, FEXCore::Core::InternalThreadState *Thread, FEXCore::HLE::SyscallArguments *Args) {
  return FEXCore::HandleSyscall(Handler, Thread, Args);
}

static FEXCore::CPUIDEmu::FunctionResults CPUIDThunk(FEXCore::CPUIDEmu *CPUID, uint64_t Function) {
  return CPUID->RunFunction(Function);
}

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

DEF_OP(ExitFunction) {
  ldp(TMP1, lr, MemOperand(sp, 16, PostIndex));
  add(sp, TMP1, 0); // Move that supports SP
  ret();
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

  b(TargetLabel);
}

DEF_OP(CondJump) {
  auto Op = IROp->C<IR::IROp_CondJump>();

  Label *TrueTargetLabel;
  Label *FalseTargetLabel;

  auto TrueIter = JumpTargets.find(Op->Header.Args[1].ID());
  auto FalseIter = JumpTargets.find(Op->Header.Args[2].ID());

  if (TrueIter == JumpTargets.end()) {
    TrueTargetLabel = &JumpTargets.try_emplace(Op->Header.Args[1].ID()).first->second;
  }
  else {
    TrueTargetLabel = &TrueIter->second;
  }

  if (FalseIter == JumpTargets.end()) {
    FalseTargetLabel = &JumpTargets.try_emplace(Op->Header.Args[2].ID()).first->second;
  }
  else {
    FalseTargetLabel = &FalseIter->second;
  }

  cbnz(GetReg<RA_64>(Op->Header.Args[0].ID()), TrueTargetLabel);
  b(FalseTargetLabel);
}

DEF_OP(Syscall) {
  auto Op = IROp->C<IR::IROp_Syscall>();
  // Arguments are passed as follows:
  // X0: SyscallHandler
  // X1: ThreadState
  // X2: Pointer to SyscallArguments

  uint64_t SPOffset = AlignUp((RA64.size() + 7 + 1) * 8, 16);

  sub(sp, sp, SPOffset);
  for (uint32_t i = 0; i < FEXCore::HLE::SyscallArguments::MAX_ARGS; ++i) {
    if (Op->Header.Args[i].IsInvalid()) continue;
    str(GetReg<RA_64>(Op->Header.Args[i].ID()), MemOperand(sp, 0 + i * 8));
  }

  int i = 0;
  for (auto RA : RA64) {
    str(RA, MemOperand(sp, 7 * 8 + i * 8));
    i++;
  }
  str(lr,       MemOperand(sp, 7 * 8 + RA64.size() * 8 + 0 * 8));

  LoadConstant(x0, reinterpret_cast<uint64_t>(CTX->SyscallHandler.get()));
  mov(x1, STATE);
  mov(x2, sp);

#if _M_X86_64
  CallRuntime(SyscallThunk);
#else
  LoadConstant(x3, reinterpret_cast<uint64_t>(FEXCore::HandleSyscall));
  blr(x3);
#endif

  // Result is now in x0
  // Fix the stack and any values that were stepped on
  i = 0;
  for (auto RA : RA64) {
    ldr(RA, MemOperand(sp, 7 * 8 + i * 8));
    i++;
  }

  // Move result to its destination register
  mov(GetReg<RA_64>(Node), x0);

  ldr(lr,       MemOperand(sp, 7 * 8 + RA64.size() * 8 + 0 * 8));

  add(sp, sp, SPOffset);
}

DEF_OP(CPUID) {
  auto Op = IROp->C<IR::IROp_CPUID>();
  uint64_t SPOffset = AlignUp((RA64.size() + 2 + 2) * 8, 16);
  sub(sp, sp, SPOffset);

  int i = 0;
  for (auto RA : RA64) {
    str(RA, MemOperand(sp, 0 + i * 8));
    i++;
  }

  str(lr,       MemOperand(sp, RA64.size() * 8 + 0 * 8));

  // x0 = CPUID Handler
  // x1 = CPUID Function
  LoadConstant(x0, reinterpret_cast<uint64_t>(&CTX->CPUID));
  mov(x1, GetReg<RA_64>(Op->Header.Args[0].ID()));
#if _M_X86_64
  CallRuntime(CPUIDThunk);
#else
  LoadConstant(x3, (uint64_t)CPUIDThunk);
  blr(x3);
#endif
  i = 0;
  for (auto RA : RA64) {
    ldr(RA, MemOperand(sp, 0 + i * 8));
    i++;
  }

  // Results are in x0, x1
  // Results want to be in a i64v2 vector
  auto Dst = GetSrcPair<RA_64>(Node);
  mov(Dst.first,  x0);
  mov(Dst.second, x1);

  ldr(lr,       MemOperand(sp, RA64.size() * 8 + 0 * 8));

  add(sp, sp, SPOffset);
}

#undef DEF_OP
void JITCore::RegisterBranchHandlers() {
#define REGISTER_OP(op, x) OpHandlers[FEXCore::IR::IROps::OP_##op] = &JITCore::Op_##x
  REGISTER_OP(GUESTCALLDIRECT,   GuestCallDirect);
  REGISTER_OP(GUESTCALLINDIRECT, GuestCallIndirect);
  REGISTER_OP(GUESTRETURN,       GuestReturn);
  REGISTER_OP(EXITFUNCTION,      ExitFunction);
  REGISTER_OP(JUMP,              Jump);
  REGISTER_OP(CONDJUMP,          CondJump);
  REGISTER_OP(SYSCALL,           Syscall);
  REGISTER_OP(CPUID,             CPUID);
#undef REGISTER_OP
}
}

