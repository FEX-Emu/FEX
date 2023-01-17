/*
$info$
tags: backend|interpreter
$end_info$
*/

#include "Interface/Context/Context.h"
#include "Interface/Core/Interpreter/InterpreterClass.h"
#include "Interface/Core/Interpreter/InterpreterOps.h"
#include "Interface/Core/Interpreter/InterpreterDefines.h"
#include "Interface/HLE/Thunks/Thunks.h"

#include <FEXCore/Utils/BitUtils.h>
#include <FEXCore/HLE/SyscallHandler.h>

#include <cstdint>
#include <unistd.h>

namespace FEXCore::CPU {
#define DEF_OP(x) void InterpreterOps::Op_##x(IR::IROp_Header *IROp, IROpData *Data, IR::NodeID Node)

DEF_OP(CallbackReturn) {
  Data->State->CurrentFrame->Pointers.Interpreter.CallbackReturn(Data->State, Data->StackEntry);
}

DEF_OP(ExitFunction) {
  auto Op = IROp->C<IR::IROp_ExitFunction>();
  uint8_t OpSize = IROp->Size;

  uintptr_t* ContextPtr = reinterpret_cast<uintptr_t*>(Data->State->CurrentFrame);

  void *ContextData = reinterpret_cast<void*>(ContextPtr);
  void *Src = GetSrc<void*>(Data->SSAData, Op->NewRIP);

  memcpy(ContextData, Src, OpSize);

  Data->BlockResults.Quit = true;
}

DEF_OP(Jump) {
  auto Op = IROp->C<IR::IROp_Jump>();
  const uintptr_t ListBegin = Data->CurrentIR->GetListData();
  const uintptr_t DataBegin = Data->CurrentIR->GetData();

  Data->BlockIterator = IR::NodeIterator(ListBegin, DataBegin, Op->TargetBlock);
  Data->BlockResults.Redo = true;
}

DEF_OP(CondJump) {
  auto Op = IROp->C<IR::IROp_CondJump>();
  const uintptr_t ListBegin = Data->CurrentIR->GetListData();
  const uintptr_t DataBegin = Data->CurrentIR->GetData();

  bool CompResult;

  const uint64_t Src1 = *GetSrc<uint64_t*>(Data->SSAData, Op->Cmp1);
  const uint64_t Src2 = *GetSrc<uint64_t*>(Data->SSAData, Op->Cmp2);

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

DEF_OP(InlineSyscall) {
  auto Op = IROp->C<IR::IROp_InlineSyscall>();

  FEXCore::HLE::SyscallArguments Args;
  for (size_t j = 0; j < FEXCore::HLE::SyscallArguments::MAX_ARGS; ++j) {
    if (Op->Header.Args[j].IsInvalid()) break;
    Args.Argument[j] = *GetSrc<uint64_t*>(Data->SSAData, Op->Header.Args[j]);
  }

  // We don't want the errno handling but I also don't want to write inline ASM atm
  uint64_t Res = syscall(
    Op->HostSyscallNumber,
    Args.Argument[0],
    Args.Argument[1],
    Args.Argument[2],
    Args.Argument[3],
    Args.Argument[4],
    Args.Argument[5],
    Args.Argument[6]
  );

  if (Res == -1) {
    Res = -errno;
  }

  GD = Res;
}

DEF_OP(Thunk) {
  auto Op = IROp->C<IR::IROp_Thunk>();

  auto thunkFn = Data->State->CTX->ThunkHandler->LookupThunk(Op->ThunkNameHash);
  thunkFn(*GetSrc<void**>(Data->SSAData, Op->ArgPtr));
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

DEF_OP(ThreadRemoveCodeEntry) {
  Data->State->CTX->ThreadRemoveCodeEntryFromJit(Data->State->CurrentFrame, Data->CurrentEntry);
}

DEF_OP(CPUID) {
  auto Op = IROp->C<IR::IROp_CPUID>();
  uint64_t *DstPtr = GetDest<uint64_t*>(Data->SSAData, Node);
  const uint64_t Arg = *GetSrc<uint64_t*>(Data->SSAData, Op->Function);
  const uint64_t Leaf = *GetSrc<uint64_t*>(Data->SSAData, Op->Leaf);

  auto Results = Data->State->CTX->CPUID.RunFunction(Arg, Leaf);
  memcpy(DstPtr, &Results, sizeof(uint32_t) * 4);
}

#undef DEF_OP

} // namespace FEXCore::CPU
