/*
$info$
tags: backend|riscv64
$end_info$
*/

#include "Interface/Core/JIT/RISCV/JITClass.h"

namespace FEXCore::CPU {
using namespace biscuit;

#define DEF_OP(x) void RISCVJITCore::Op_##x(IR::IROp_Header *IROp, IR::NodeID Node)
DEF_OP(GetHostFlag) {
  auto Op = IROp->C<IR::IROp_GetHostFlag>();
  UBFX(GetReg(Node), GetReg(Op->Header.Args[0].ID()), Op->Flag, 1);
}

#undef DEF_OP
void RISCVJITCore::RegisterFlagHandlers() {
#define REGISTER_OP(op, x) OpHandlers[FEXCore::IR::IROps::OP_##op] = &RISCVJITCore::Op_##x
  REGISTER_OP(GETHOSTFLAG, GetHostFlag);
#undef REGISTER_OP
}
}

