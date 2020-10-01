#include "Interface/Core/JIT/x86_64/JITClass.h"
#include "Interface/IR/Passes/RegisterAllocationPass.h"

namespace FEXCore::CPU {
#define DEF_OP(x) void JITCore::Op_##x(FEXCore::IR::IROp_Header *IROp, uint32_t Node)

DEF_OP(AESImc) {
  auto Op = IROp->C<IR::IROp_VAESImc>();
  vaesimc(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
}

DEF_OP(AESEnc) {
  auto Op = IROp->C<IR::IROp_VAESEnc>();
  vaesenc(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
}

DEF_OP(AESEncLast) {
  auto Op = IROp->C<IR::IROp_VAESEncLast>();
  vaesenclast(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
}

DEF_OP(AESDec) {
  auto Op = IROp->C<IR::IROp_VAESDec>();
  vaesdec(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
}

DEF_OP(AESDecLast) {
  auto Op = IROp->C<IR::IROp_VAESDecLast>();
  vaesdeclast(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
}

DEF_OP(AESKeyGenAssist) {
  auto Op = IROp->C<IR::IROp_VAESKeyGenAssist>();
  vaeskeygenassist(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), Op->RCON);
}

#undef DEF_OP
void JITCore::RegisterEncryptionHandlers() {
#define REGISTER_OP(op, x) OpHandlers[FEXCore::IR::IROps::OP_##op] = &JITCore::Op_##x
  REGISTER_OP(VAESIMC,     AESImc);
  REGISTER_OP(VAESENC,     AESEnc);
  REGISTER_OP(VAESENCLAST, AESEncLast);
  REGISTER_OP(VAESDEC,     AESDec);
  REGISTER_OP(VAESDECLAST, AESDecLast);
  REGISTER_OP(VAESKEYGENASSIST, AESKeyGenAssist);

#undef REGISTER_OP
}
}
