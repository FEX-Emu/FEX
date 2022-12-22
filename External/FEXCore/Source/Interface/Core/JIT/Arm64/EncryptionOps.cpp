/*
$info$
tags: backend|arm64
$end_info$
*/

#include "Interface/Core/JIT/Arm64/JITClass.h"
#include "Interface/IR/Passes/RegisterAllocationPass.h"

namespace FEXCore::CPU {
using namespace vixl;
using namespace vixl::aarch64;
#define DEF_OP(x) void Arm64JITCore::Op_##x(IR::IROp_Header const *IROp, IR::NodeID Node)

DEF_OP(AESImc) {
  auto Op = IROp->C<IR::IROp_VAESImc>();
  aesimc(GetVReg(Node).V16B(), GetVReg(Op->Vector.ID()).V16B());
}

DEF_OP(AESEnc) {
  auto Op = IROp->C<IR::IROp_VAESEnc>();
  eor(VTMP2.V16B(), VTMP2.V16B(), VTMP2.V16B());
  mov(VTMP1.V16B(), GetVReg(Op->State.ID()).V16B());
  aese(VTMP1.V16B(), VTMP2.V16B());
  aesmc(VTMP1.V16B(), VTMP1.V16B());
  eor(GetVReg(Node).V16B(), VTMP1.V16B(), GetVReg(Op->Key.ID()).V16B());
}

DEF_OP(AESEncLast) {
  auto Op = IROp->C<IR::IROp_VAESEncLast>();
  eor(VTMP2.V16B(), VTMP2.V16B(), VTMP2.V16B());
  mov(VTMP1.V16B(), GetVReg(Op->State.ID()).V16B());
  aese(VTMP1.V16B(), VTMP2.V16B());
  eor(GetVReg(Node).V16B(), VTMP1.V16B(), GetVReg(Op->Key.ID()).V16B());
}

DEF_OP(AESDec) {
  auto Op = IROp->C<IR::IROp_VAESDec>();
  eor(VTMP2.V16B(), VTMP2.V16B(), VTMP2.V16B());
  mov(VTMP1.V16B(), GetVReg(Op->State.ID()).V16B());
  aesd(VTMP1.V16B(), VTMP2.V16B());
  aesimc(VTMP1.V16B(), VTMP1.V16B());
  eor(GetVReg(Node).V16B(), VTMP1.V16B(), GetVReg(Op->Key.ID()).V16B());
}

DEF_OP(AESDecLast) {
  auto Op = IROp->C<IR::IROp_VAESDecLast>();
  eor(VTMP2.V16B(), VTMP2.V16B(), VTMP2.V16B());
  mov(VTMP1.V16B(), GetVReg(Op->State.ID()).V16B());
  aesd(VTMP1.V16B(), VTMP2.V16B());
  eor(GetVReg(Node).V16B(), VTMP1.V16B(), GetVReg(Op->Key.ID()).V16B());
}

DEF_OP(AESKeyGenAssist) {
  auto Op = IROp->C<IR::IROp_VAESKeyGenAssist>();

  aarch64::Literal ConstantLiteral (0x0C030609'0306090CULL, 0x040B0E01'0B0E0104ULL);
  aarch64::Label PastConstant;

  // Do a "regular" AESE step
  eor(VTMP2.V16B(), VTMP2.V16B(), VTMP2.V16B());
  mov(VTMP1.V16B(), GetVReg(Op->Src.ID()).V16B());
  aese(VTMP1.V16B(), VTMP2.V16B());

  // Do a table shuffle to undo ShiftRows
  ldr(VTMP3, &ConstantLiteral);

  // Now EOR in the RCON
  if (Op->RCON) {
    tbl(VTMP1.V16B(), VTMP1.V16B(), VTMP3.V16B());

    LoadConstant(TMP1, static_cast<uint64_t>(Op->RCON) << 32);
    dup(VTMP2.V2D(), TMP1);
    eor(GetVReg(Node).V16B(), VTMP1.V16B(), VTMP2.V16B());
  }
  else {
    tbl(GetVReg(Node).V16B(), VTMP1.V16B(), VTMP3.V16B());
  }

  b(&PastConstant);
  place(&ConstantLiteral);
  bind(&PastConstant);
}

DEF_OP(CRC32) {
  auto Op = IROp->C<IR::IROp_CRC32>();
  switch (Op->SrcSize) {
    case 1:
      crc32cb(GetReg<RA_32>(Node), GetReg<RA_32>(Op->Src1.ID()), GetReg<RA_32>(Op->Src2.ID()));
      break;
    case 2:
      crc32ch(GetReg<RA_32>(Node), GetReg<RA_32>(Op->Src1.ID()), GetReg<RA_32>(Op->Src2.ID()));
      break;
    case 4:
      crc32cw(GetReg<RA_32>(Node), GetReg<RA_32>(Op->Src1.ID()), GetReg<RA_32>(Op->Src2.ID()));
      break;
    case 8:
      crc32cx(GetReg<RA_32>(Node), GetReg<RA_32>(Op->Src1.ID()), GetReg<RA_64>(Op->Src2.ID()));
      break;
    default: LOGMAN_MSG_A_FMT("Unknown CRC32 size: {}", Op->SrcSize);
  }
}

DEF_OP(PCLMUL) {
  auto Op = IROp->C<IR::IROp_PCLMUL>();

  auto Dst  = GetVReg(Node).Q();
  auto Src1 = GetVReg(Op->Src1.ID()).V2D();
  auto Src2 = GetVReg(Op->Src2.ID()).V2D();

  switch (Op->Selector) {
  case 0b00000000:
    pmull(Dst, Src1, Src2);
    break;
  case 0b00000001:
    mov(VTMP1.V1D(), Src1, 1);
    pmull(Dst, VTMP1.V2D(), Src2);
    break;
  case 0b00010000:
    mov(VTMP1.V1D(), Src2, 1);
    pmull(Dst, VTMP1.V2D(), Src1);
    break;
  case 0b00010001:
    pmull2(Dst, Src1, Src2);
    break;
  default:
    LOGMAN_MSG_A_FMT("Unknown PCLMUL selector: {}", Op->Selector);
    break;
  }
}

#undef DEF_OP
void Arm64JITCore::RegisterEncryptionHandlers() {
#define REGISTER_OP(op, x) OpHandlers[FEXCore::IR::IROps::OP_##op] = &Arm64JITCore::Op_##x
  REGISTER_OP(VAESIMC,           AESImc);
  REGISTER_OP(VAESENC,           AESEnc);
  REGISTER_OP(VAESENCLAST,       AESEncLast);
  REGISTER_OP(VAESDEC,           AESDec);
  REGISTER_OP(VAESDECLAST,       AESDecLast);
  REGISTER_OP(VAESKEYGENASSIST,  AESKeyGenAssist);
  REGISTER_OP(CRC32,             CRC32);
  REGISTER_OP(PCLMUL,            PCLMUL);
#undef REGISTER_OP
}
}
