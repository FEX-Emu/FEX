/*
$info$
tags: backend|interpreter
$end_info$
*/

#include "Interface/Core/Interpreter/InterpreterClass.h"
#include "Interface/Core/Interpreter/InterpreterOps.h"
#include "Interface/Core/Interpreter/InterpreterDefines.h"

#include <cstdint>

namespace FEXCore::CPU {
#define DEF_OP(x) void InterpreterOps::Op_##x(IR::IROp_Header *IROp, IROpData *Data, IR::NodeID Node)
DEF_OP(ExtractElementPair) {
  auto Op = IROp->C<IR::IROp_ExtractElementPair>();
  const auto Src = GetSrc<uintptr_t>(Data->SSAData, Op->Pair);
  memcpy(GDP,
    reinterpret_cast<void*>(Src + Op->Header.Size * Op->Element), Op->Header.Size);
}

DEF_OP(CreateElementPair) {
  auto Op = IROp->C<IR::IROp_CreateElementPair>();
  const void *Src_Lower = GetSrc<void*>(Data->SSAData, Op->Lower);
  const void *Src_Upper = GetSrc<void*>(Data->SSAData, Op->Upper);

  uint8_t *Dst = GetDest<uint8_t*>(Data->SSAData, Node);

  memcpy(Dst, Src_Lower, IROp->ElementSize);
  memcpy(Dst + IROp->ElementSize, Src_Upper, IROp->ElementSize);
}

#undef DEF_OP

} // namespace FEXCore::CPU
