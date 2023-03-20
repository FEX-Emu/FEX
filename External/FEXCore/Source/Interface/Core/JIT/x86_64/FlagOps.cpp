/*
$info$
tags: backend|x86-64
$end_info$
*/

#include "Interface/Core/JIT/x86_64/JITClass.h"
#include "Interface/Core/Dispatcher/X86Dispatcher.h"

#include <FEXCore/IR/IR.h>

#include <array>
#include <stdint.h>

namespace FEXCore::CPU {

#define DEF_OP(x) void X86JITCore::Op_##x(IR::IROp_Header *IROp, IR::NodeID Node)
DEF_OP(GetHostFlag) {
  auto Op = IROp->C<IR::IROp_GetHostFlag>();

  mov(rax, GetSrc<RA_64>(Op->Value.ID()));
  shr(rax, Op->Flag);
  and_(rax, 1);
  mov(GetDst<RA_64>(Node), rax);
}

#undef DEF_OP
void X86JITCore::RegisterFlagHandlers() {
#define REGISTER_OP(op, x) OpHandlers[FEXCore::IR::IROps::OP_##op] = &X86JITCore::Op_##x
  REGISTER_OP(GETHOSTFLAG, GetHostFlag);
#undef REGISTER_OP
}
}

