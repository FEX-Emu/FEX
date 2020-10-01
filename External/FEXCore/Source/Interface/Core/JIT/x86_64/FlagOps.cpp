#include "Interface/Core/JIT/x86_64/JITClass.h"
#include "Interface/IR/Passes/RegisterAllocationPass.h"

namespace FEXCore::CPU {

#define DEF_OP(x) void JITCore::Op_##x(FEXCore::IR::IROp_Header *IROp, uint32_t Node)
DEF_OP(GetHostFlag) {
  auto Op = IROp->C<IR::IROp_GetHostFlag>();

  mov(rax, GetSrc<RA_64>(Op->Header.Args[0].ID()));
  shr(rax, Op->Flag);
  and(rax, 1);
  mov(GetDst<RA_64>(Node), rax);
}

#undef DEF_OP
void JITCore::RegisterFlagHandlers() {
#define REGISTER_OP(op, x) OpHandlers[FEXCore::IR::IROps::OP_##op] = &JITCore::Op_##x
  REGISTER_OP(GETHOSTFLAG, GetHostFlag);
#undef REGISTER_OP
}
}

