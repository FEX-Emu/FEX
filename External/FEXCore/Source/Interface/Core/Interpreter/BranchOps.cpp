/*
$info$
tags: backend|interpreter
$end_info$
*/

#include "Interface/Core/Interpreter/InterpreterClass.h"
#include "Interface/Core/Interpreter/InterpreterOps.h"
#include "Interface/Core/Interpreter/InterpreterDefines.h"
#include "Interface/HLE/Thunks/Thunks.h"

#include <FEXCore/Utils/BitUtils.h>
#include <FEXCore/HLE/SyscallHandler.h>

#include <cstdint>

namespace FEXCore::CPU {
[[noreturn]]
static void SignalReturn(FEXCore::Core::InternalThreadState *Thread) {
  Thread->CTX->SignalThread(Thread, FEXCore::Core::SignalEvent::Return);

  LOGMAN_MSG_A_FMT("unreachable");
  FEX_UNREACHABLE;
}

#define DEF_OP(x) void InterpreterOps::Op_##x(FEXCore::IR::IROp_Header *IROp, IROpData *Data, uint32_t Node)
DEF_OP(GuestCallDirect) {
  LogMan::Msg::DFmt("Unimplemented");
}

DEF_OP(GuestCallIndirect) {
  LogMan::Msg::DFmt("Unimplemented");
}

DEF_OP(GuestReturn) {
  LogMan::Msg::DFmt("Unimplemented");
}

DEF_OP(SignalReturn) {
  SignalReturn(Data->State);
}

DEF_OP(CallbackReturn) {
  Data->State->CTX->InterpreterCallbackReturn(Data->State, Data->StackEntry);
}

DEF_OP(ExitFunction) {
  auto Op = IROp->C<IR::IROp_ExitFunction>();
  uint8_t OpSize = IROp->Size;

  uintptr_t* ContextPtr = reinterpret_cast<uintptr_t*>(Data->State->CurrentFrame);

  void *ContextData = reinterpret_cast<void*>(ContextPtr);
  void *Src = GetSrc<void*>(Data->SSAData, Op->Header.Args[0]);

  memcpy(ContextData, Src, OpSize);

  Data->BlockResults.Quit = true;
}

DEF_OP(Jump) {
  auto Op = IROp->C<IR::IROp_Jump>();
  uintptr_t ListBegin = Data->CurrentIR->GetListData();
  uintptr_t DataBegin = Data->CurrentIR->GetData();

  Data->BlockIterator = IR::NodeIterator(ListBegin, DataBegin, Op->Header.Args[0]);
  Data->BlockResults.Redo = true;
}

DEF_OP(CondJump) {
  auto Op = IROp->C<IR::IROp_CondJump>();
  uintptr_t ListBegin = Data->CurrentIR->GetListData();
  uintptr_t DataBegin = Data->CurrentIR->GetData();

  bool CompResult;

  uint64_t Src1 = *GetSrc<uint64_t*>(Data->SSAData, Op->Cmp1);
  uint64_t Src2 = *GetSrc<uint64_t*>(Data->SSAData, Op->Cmp2);

  if (Op->CompareSize == 4)
    CompResult = IsConditionTrue<uint32_t, int32_t, float>(Op->Cond.Val, Src1, Src2);
  else
    CompResult = IsConditionTrue<uint64_t, int64_t, double>(Op->Cond.Val, Src1, Src2);

  if (CompResult) {
    Data->BlockIterator = IR::NodeIterator(ListBegin, DataBegin, Op->TrueBlock);
  }
  else  {
    Data->BlockIterator = IR::NodeIterator(ListBegin, DataBegin, Op->FalseBlock);
  }
  Data->BlockResults.Redo = true;
}

DEF_OP(Syscall) {
  auto Op = IROp->C<IR::IROp_Syscall>();

  FEXCore::HLE::SyscallArguments Args;
  for (size_t j = 0; j < FEXCore::HLE::SyscallArguments::MAX_ARGS; ++j) {
    if (Op->Header.Args[j].IsInvalid()) break;
    Args.Argument[j] = *GetSrc<uint64_t*>(Data->SSAData, Op->Header.Args[j]);
  }

  uint64_t Res = FEXCore::Context::HandleSyscall(Data->State->CTX->SyscallHandler, Data->State->CurrentFrame, &Args);
  GD = Res;
}

DEF_OP(Thunk) {
  auto Op = IROp->C<IR::IROp_Thunk>();

  auto thunkFn = Data->State->CTX->ThunkHandler->LookupThunk(Op->ThunkNameHash);
  thunkFn(*GetSrc<void**>(Data->SSAData, Op->Header.Args[0]));
}

DEF_OP(ValidateCode) {
  auto Op = IROp->C<IR::IROp_ValidateCode>();

  auto CodePtr = Data->CurrentEntry + Op->Offset;
  if (memcmp((void*)CodePtr, &Op->CodeOriginalLow, Op->CodeLength) != 0) {
    GD = 1;
  } else {
    GD = 0;
  }
}

DEF_OP(RemoveCodeEntry) {
  Data->State->CTX->RemoveCodeEntry(Data->State, Data->CurrentEntry);
}

DEF_OP(CPUID) {
  auto Op = IROp->C<IR::IROp_CPUID>();
  uint64_t *DstPtr = GetDest<uint64_t*>(Data->SSAData, Node);
  uint64_t Arg = *GetSrc<uint64_t*>(Data->SSAData, Op->Header.Args[0]);
  uint64_t Leaf = *GetSrc<uint64_t*>(Data->SSAData, Op->Header.Args[1]);

  auto Results = Data->State->CTX->CPUID.RunFunction(Arg, Leaf);
  memcpy(DstPtr, &Results, sizeof(uint32_t) * 4);
}

#undef DEF_OP
void InterpreterOps::RegisterBranchHandlers() {
#define REGISTER_OP(op, x) OpHandlers[FEXCore::IR::IROps::OP_##op] = &InterpreterOps::Op_##x
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
