#include "Interface/Core/JIT/Arm64/JITClass.h"
#include "Interface/IR/Passes/RegisterAllocationPass.h"

namespace FEXCore::CPU {
using namespace vixl;
using namespace vixl::aarch64;
#define DEF_OP(x) void JITCore::Op_##x(FEXCore::IR::IROp_Header *IROp, uint32_t Node)

DEF_OP(AESImc) {
  auto Op = IROp->C<IR::IROp_VAESImc>();
  aesimc(GetDst(Node).V16B(), GetSrc(Op->Header.Args[0].ID()).V16B());
}

DEF_OP(AESEnc) {
	auto Op = IROp->C<IR::IROp_VAESEnc>();
  eor(VTMP2.V16B(), VTMP2.V16B(), VTMP2.V16B());
  mov(VTMP1.V16B(), GetSrc(Op->Header.Args[0].ID()).V16B());
  aese(VTMP1.V16B(), VTMP2.V16B());
  aesmc(VTMP1.V16B(), VTMP1.V16B());
  eor(GetDst(Node).V16B(), VTMP1.V16B(), GetSrc(Op->Header.Args[1].ID()).V16B());
}

DEF_OP(AESEncLast) {
	auto Op = IROp->C<IR::IROp_VAESEncLast>();
  eor(VTMP2.V16B(), VTMP2.V16B(), VTMP2.V16B());
  mov(VTMP1.V16B(), GetSrc(Op->Header.Args[0].ID()).V16B());
  aese(VTMP1.V16B(), VTMP2.V16B());
  eor(GetDst(Node).V16B(), VTMP1.V16B(), GetSrc(Op->Header.Args[1].ID()).V16B());
}

DEF_OP(AESDec) {
	auto Op = IROp->C<IR::IROp_VAESDec>();
  eor(VTMP2.V16B(), VTMP2.V16B(), VTMP2.V16B());
  mov(VTMP1.V16B(), GetSrc(Op->Header.Args[0].ID()).V16B());
  aesd(VTMP1.V16B(), VTMP2.V16B());
  aesimc(VTMP1.V16B(), VTMP1.V16B());
  eor(GetDst(Node).V16B(), VTMP1.V16B(), GetSrc(Op->Header.Args[1].ID()).V16B());
}

DEF_OP(AESDecLast) {
	auto Op = IROp->C<IR::IROp_VAESDecLast>();
  eor(VTMP2.V16B(), VTMP2.V16B(), VTMP2.V16B());
  mov(VTMP1.V16B(), GetSrc(Op->Header.Args[0].ID()).V16B());
  aesd(VTMP1.V16B(), VTMP2.V16B());
  eor(GetDst(Node).V16B(), VTMP1.V16B(), GetSrc(Op->Header.Args[1].ID()).V16B());
}

DEF_OP(AESKeyGenAssist) {
	auto Op = IROp->C<IR::IROp_VAESKeyGenAssist>();

  aarch64::Label Constant;
  aarch64::Label PastConstant;

  // Do a "regular" AESE step
  eor(VTMP2.V16B(), VTMP2.V16B(), VTMP2.V16B());
  mov(VTMP1.V16B(), GetSrc(Op->Header.Args[0].ID()).V16B());
  aese(VTMP1.V16B(), VTMP2.V16B());

  // Do a table shuffle to undo ShiftRows
  adr(TMP1.X(), &Constant);
  ldr(VTMP3, MemOperand(TMP1.X()));

  // Now EOR in the RCON
  if (Op->RCON) {
    tbl(VTMP1.V16B(), VTMP1.V16B(), VTMP3.V16B());

    LoadConstant(TMP1.W(), Op->RCON);
    ins(VTMP2.V4S(), 1, TMP1.W());
    ins(VTMP2.V4S(), 3, TMP1.W());
    eor(GetDst(Node).V16B(), VTMP1.V16B(), VTMP2.V16B());
  }
  else {
    tbl(GetDst(Node).V16B(), VTMP1.V16B(), VTMP3.V16B());
  }

  b(&PastConstant);
  bind(&Constant);
  dc32(0x0B0E0104);
  dc32(0x040B0E01);
  dc32(0x0306090C);
  dc32(0x0C030609);
  bind(&PastConstant);
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
