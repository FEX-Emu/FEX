/*
$info$
tags: backend|x86-64
$end_info$
*/

#include "Interface/Context/Context.h"
#include "Interface/Core/CPUID.h"
#include "Interface/Core/Dispatcher/Dispatcher.h"
#include "Interface/Core/LookupCache.h"
#include "Interface/Core/JIT/x86_64/JITClass.h"
#include "Interface/HLE/Thunks/Thunks.h"

#include <FEXCore/Core/CPUID.h>
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/Debug/InternalThreadState.h>
#include <FEXCore/HLE/SyscallHandler.h>
#include <FEXCore/IR/IR.h>
#include <FEXCore/Utils/LogManager.h>

#include <array>
#include <memory>
#include <stddef.h>
#include <stdint.h>
#include <unordered_map>
#include <utility>
#include <xbyak/xbyak.h>

namespace FEXCore::CPU {
#define DEF_OP(x) void X86JITCore::Op_##x(IR::IROp_Header *IROp, IR::NodeID Node)
DEF_OP(GuestCallDirect) {
  LogMan::Msg::DFmt("Unimplemented");
}

DEF_OP(GuestCallIndirect) {
  LogMan::Msg::DFmt("Unimplemented");
}

DEF_OP(SignalReturn) {
  // Adjust the stack first for a regular return
  if (SpillSlots) {
    add(rsp, SpillSlots * 16); // + 8 to consume return address
  }

  jmp(qword [STATE + offsetof(FEXCore::Core::CpuStateFrame, Pointers.X86.SignalReturnHandler)]);
}

DEF_OP(CallbackReturn) {
  // Adjust the stack first for a regular return
  if (SpillSlots) {
    add(rsp, SpillSlots * 16); // + 8 to consume return address
  }

  // Make sure to adjust the refcounter so we don't clear the cache now
  sub(qword [STATE + offsetof(FEXCore::Core::CpuStateFrame, SignalHandlerRefCounter)], 1);

  // We need to adjust an additional 8 bytes to get back to the original "misaligned" RSP state
  add(qword [STATE + offsetof(FEXCore::Core::CpuStateFrame, State.gregs[X86State::REG_RSP])], 8);

  // Now jump back to the thunk
  // XXX: XMM?
  add(rsp, 8);

  pop(r15);
  pop(r14);
  pop(r13);
  pop(r12);
  pop(rbp);
  pop(rbx);

  ret();
}

DEF_OP(ExitFunction) {
  Label FullLookup;
  auto Op = IROp->C<IR::IROp_ExitFunction>();


  if (SpillSlots) {
    add(rsp, SpillSlots * 16);
  }

  uint64_t NewRIP;

  if (IsInlineConstant(Op->NewRIP, &NewRIP) || IsInlineEntrypointOffset(Op->NewRIP, &NewRIP)) {
    Label l_BranchHost;
    Label l_BranchGuest;

    lea(rax, ptr[rip + l_BranchHost]);
    jmp(qword[rax]);

    L(l_BranchHost);
    dq(Dispatcher->ExitFunctionLinkerAddress);
    L(l_BranchGuest);
    dq(NewRIP);
  } else {
    Xbyak::Reg RipReg = GetSrc<RA_64>(Op->NewRIP.ID());

    // L1 Cache
    mov(rcx, qword [STATE + offsetof(FEXCore::Core::CpuStateFrame, Pointers.X86.L1Pointer)]);

    mov(rax, RipReg);

    and_(rax, LookupCache::L1_ENTRIES_MASK);
    shl(rax, 4);

    Xbyak::RegExp LookupBase = rcx + rax;

    cmp(qword[LookupBase + 8], RipReg);
    jne(FullLookup);
    jmp(qword[LookupBase + 0]);

    L(FullLookup);
    mov(qword [STATE + offsetof(FEXCore::Core::CpuStateFrame, State.rip)], RipReg);
    jmp(qword [STATE + offsetof(FEXCore::Core::CpuStateFrame, Pointers.X86.DispatcherLoopTop)]);
  }

#ifdef BLOCKSTATS
  ExitBlock();
#endif
}

DEF_OP(Jump) {
  const auto Op = IROp->C<IR::IROp_Jump>();
  const auto ArgID = Op->Args(0).ID();

  PendingTargetLabel = &JumpTargets.try_emplace(ArgID).first->second;
}

#define GRCMP(Node) (Op->CompareSize == 4 ? GetSrc<RA_32>(Node) : GetSrc<RA_64>(Node))

DEF_OP(CondJump) {
  auto Op = IROp->C<IR::IROp_CondJump>();

  Label *TrueTargetLabel = &JumpTargets.try_emplace(Op->TrueBlock.ID()).first->second;

  if (IsGPR(Op->Cmp1.ID())) {
    uint64_t Const;
    if (IsInlineConstant(Op->Cmp2, &Const)) {
      cmp(GRCMP(Op->Cmp1.ID()), Const);
    } else {
      cmp(GRCMP(Op->Cmp1.ID()), GRCMP(Op->Cmp2.ID()));
    }
  } else if (IsFPR(Op->Cmp1.ID())) {
    if (Op->CompareSize == 4) {
      ucomiss(GetSrc(Op->Cmp1.ID()), GetSrc(Op->Cmp2.ID()));
    } else {
      ucomisd(GetSrc(Op->Cmp1.ID()), GetSrc(Op->Cmp2.ID()));
    }
  }

  auto [_, __, JCC] = GetCC(Op->Cond);

  (this->*JCC)(*TrueTargetLabel, T_NEAR);

  PendingTargetLabel = &JumpTargets.try_emplace(Op->FalseBlock.ID()).first->second;
}

DEF_OP(Syscall) {
  auto Op = IROp->C<IR::IROp_Syscall>();
  // XXX: This is very terrible, but I don't care for right now

  auto NumPush = RA64.size();

  for (auto &Reg : RA64)
    push(Reg);

  // Syscall ABI for x86-64
  // this: rdi
  // Thread: rsi
  // ArgPointer: rdx (Stack)
  //
  // Result: RAX

  // These are pushed in reverse order because stacks
  for (uint32_t i = FEXCore::HLE::SyscallArguments::MAX_ARGS; i > 0; --i) {
    if (Op->Header.Args[i - 1].IsInvalid()) continue;
    push(GetSrc<RA_64>(Op->Header.Args[i - 1].ID()));
    ++NumPush;
  }

  mov(rsi, STATE); // Move thread in to rsi
  mov(rdi, qword [STATE + offsetof(FEXCore::Core::CpuStateFrame, Pointers.X86.SyscallHandlerObj)]);
  mov(rdx, rsp);

  if (NumPush & 1)
    sub(rsp, 8); // Align
  // {rdi, rsi, rdx}
  call(qword [STATE + offsetof(FEXCore::Core::CpuStateFrame, Pointers.X86.SyscallHandlerFunc)]);

  if (NumPush & 1)
    add(rsp, 8); // Align

  // Reload arguments just in case they are sill live after the fact
  for (uint32_t i = 0; i < FEXCore::HLE::SyscallArguments::MAX_ARGS; ++i) {
    if (Op->Header.Args[i].IsInvalid()) continue;
    pop(GetSrc<RA_64>(Op->Header.Args[i].ID()));
  }

  for (uint32_t i = RA64.size(); i > 0; --i)
    pop(RA64[i - 1]);

  mov (GetDst<RA_64>(Node), rax);
}

DEF_OP(Thunk) {
  auto Op = IROp->C<IR::IROp_Thunk>();

  auto NumPush = RA64.size();

  for (auto &Reg : RA64)
    push(Reg);

  if (NumPush & 1)
    sub(rsp, 8); // Align

  mov(rdi, GetSrc<RA_64>(Op->Header.Args[0].ID()));

  auto thunkFn = ThreadState->CTX->ThunkHandler->LookupThunk(Op->ThunkNameHash);

  mov(rax, reinterpret_cast<uintptr_t>(thunkFn));
  call(rax);

  if (NumPush & 1)
    add(rsp, 8); // Align

  for (uint32_t i = RA64.size(); i > 0; --i)
    pop(RA64[i - 1]);
}

DEF_OP(ValidateCode) {
  auto Op = IROp->C<IR::IROp_ValidateCode>();
  const auto* OldCode = (const uint8_t*)&Op->CodeOriginalLow;
  int len = Op->CodeLength;
  int idx = 0;

  xor_(GetDst<RA_64>(Node), GetDst<RA_64>(Node));
  mov(rax, Entry + Op->Offset);
  mov(rbx, 1);
  while (len >= 4) {
    cmp(dword[rax + idx], *(const uint32_t*)(OldCode + idx));
    cmovne(GetDst<RA_64>(Node), rbx);
    len-=4;
    idx+=4;
  }
  while (len >= 2) {
    mov(rcx, *(const uint16_t*)(OldCode + idx));
    cmp(word[rax + idx], cx);
    cmovne(GetDst<RA_64>(Node), rbx);
    len-=2;
    idx+=2;
  }
  while (len >= 1) {
    cmp(byte[rax + idx], *(const uint8_t*)(OldCode + idx));
    cmovne(GetDst<RA_64>(Node), rbx);
    len-=1;
    idx+=1;
  }
}

DEF_OP(RemoveThreadCodeEntry) {
  auto NumPush = RA64.size();

  for (auto &Reg : RA64)
    push(Reg);

  if (NumPush & 1)
    sub(rsp, 8); // Align

  mov(rdi, STATE);
  mov(rax, Entry); // imm64 move
  mov(rsi, rax);

  call(qword [STATE + offsetof(FEXCore::Core::CpuStateFrame, Pointers.X86.RemoveThreadCodeEntryFromJIT)]);

  if (NumPush & 1)
    add(rsp, 8); // Align

  for (uint32_t i = RA64.size(); i > 0; --i)
    pop(RA64[i - 1]);
}

DEF_OP(CPUID) {
  auto Op = IROp->C<IR::IROp_CPUID>();

  for (auto &Reg : RA64)
    push(Reg);

  // CPUID ABI
  // this: rdi
  // Function: rsi
  //
  // Result: RAX, RDX. 4xi32

  // rsi can be in the source registers, so copy argument to edx first
  mov (edx, GetSrc<RA_32>(Op->Header.Args[1].ID()));
  mov (esi, GetSrc<RA_32>(Op->Header.Args[0].ID()));
  mov (rdi, qword [STATE + offsetof(FEXCore::Core::CpuStateFrame, Pointers.X86.CPUIDObj)]);

  auto NumPush = RA64.size();

  if (NumPush & 1)
    sub(rsp, 8); // Align

  // {rdi, rsi, rdx}
  call(qword [STATE + offsetof(FEXCore::Core::CpuStateFrame, Pointers.X86.CPUIDFunction)]);

  if (NumPush & 1)
    add(rsp, 8); // Align

  for (uint32_t i = RA64.size(); i > 0; --i)
    pop(RA64[i - 1]);

  auto Dst = GetSrcPair<RA_64>(Node);
  mov(Dst.first, rax);
  mov(Dst.second, rdx);
}

#undef DEF_OP
void X86JITCore::RegisterBranchHandlers() {
#define REGISTER_OP(op, x) OpHandlers[FEXCore::IR::IROps::OP_##op] = &X86JITCore::Op_##x
  REGISTER_OP(GUESTCALLDIRECT,   GuestCallDirect);
  REGISTER_OP(GUESTCALLINDIRECT, GuestCallIndirect);
  REGISTER_OP(SIGNALRETURN,      SignalReturn);
  REGISTER_OP(CALLBACKRETURN,    CallbackReturn);
  REGISTER_OP(EXITFUNCTION,      ExitFunction);
  REGISTER_OP(JUMP,              Jump);
  REGISTER_OP(CONDJUMP,          CondJump);
  REGISTER_OP(SYSCALL,           Syscall);
  REGISTER_OP(THUNK,             Thunk);
  REGISTER_OP(VALIDATECODE,      ValidateCode);
  REGISTER_OP(REMOVETHREADCODEENTRY,   RemoveThreadCodeEntry);
  REGISTER_OP(CPUID,             CPUID);
#undef REGISTER_OP
}
}
