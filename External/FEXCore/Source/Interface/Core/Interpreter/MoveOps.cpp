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
  uintptr_t Src = GetSrc<uintptr_t>(Data->SSAData, Op->Header.Args[0]);
  memcpy(GDP,
    reinterpret_cast<void*>(Src + Op->Header.Size * Op->Element), Op->Header.Size);
}

DEF_OP(CreateElementPair) {
  auto Op = IROp->C<IR::IROp_CreateElementPair>();
  void *Src_Lower = GetSrc<void*>(Data->SSAData, Op->Header.Args[0]);
  void *Src_Upper = GetSrc<void*>(Data->SSAData, Op->Header.Args[1]);

  uint8_t *Dst = GetDest<uint8_t*>(Data->SSAData, Node);

  memcpy(Dst, Src_Lower, Op->Header.Size);
  memcpy(Dst + Op->Header.Size, Src_Upper, Op->Header.Size);
}

DEF_OP(Mov) {
  auto Op = IROp->C<IR::IROp_Mov>();
  uint8_t OpSize = IROp->Size;

  memcpy(GDP, GetSrc<void*>(Data->SSAData, Op->Header.Args[0]), OpSize);
}

#undef DEF_OP

} // namespace FEXCore::CPU
