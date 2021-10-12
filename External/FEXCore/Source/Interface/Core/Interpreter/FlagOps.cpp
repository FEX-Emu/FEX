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
#define DEF_OP(x) void InterpreterOps::Op_##x(FEXCore::IR::IROp_Header *IROp, IROpData *Data, uint32_t Node)
DEF_OP(GetHostFlag) {
  auto Op = IROp->C<IR::IROp_GetHostFlag>();
  GD = (*GetSrc<uint64_t*>(Data->SSAData, Op->Header.Args[0]) >> Op->Flag) & 1;
}

#undef DEF_OP
void InterpreterOps::RegisterFlagHandlers() {
#define REGISTER_OP(op, x) OpHandlers[FEXCore::IR::IROps::OP_##op] = &InterpreterOps::Op_##x
  REGISTER_OP(GETHOSTFLAG, GetHostFlag);
#undef REGISTER_OP
}
}

