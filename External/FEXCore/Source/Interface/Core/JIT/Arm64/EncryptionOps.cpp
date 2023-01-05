/*
$info$
tags: backend|arm64
$end_info$
*/

#include "Interface/Core/ArchHelpers/CodeEmitter/Emitter.h"
#include "Interface/Core/JIT/Arm64/JITClass.h"
#include "Interface/IR/Passes/RegisterAllocationPass.h"

namespace FEXCore::CPU {
#define DEF_OP(x) void Arm64JITCore::Op_##x(IR::IROp_Header const *IROp, IR::NodeID Node)

DEF_OP(AESImc) {
  auto Op = IROp->C<IR::IROp_VAESImc>();
  aesimc(GetVReg(Node), GetVReg(Op->Vector.ID()));
}

DEF_OP(AESEnc) {
  auto Op = IROp->C<IR::IROp_VAESEnc>();
  eor(VTMP2.Q(), VTMP2.Q(), VTMP2.Q());
  mov(VTMP1.Q(), GetVReg(Op->State.ID()).Q());
  aese(VTMP1, VTMP2);
  aesmc(VTMP1, VTMP1);
  eor(GetVReg(Node).Q(), VTMP1.Q(), GetVReg(Op->Key.ID()).Q());
}

DEF_OP(AESEncLast) {
  auto Op = IROp->C<IR::IROp_VAESEncLast>();
  eor(VTMP2.Q(), VTMP2.Q(), VTMP2.Q());
  mov(VTMP1.Q(), GetVReg(Op->State.ID()).Q());
  aese(VTMP1, VTMP2);
  eor(GetVReg(Node).Q(), VTMP1.Q(), GetVReg(Op->Key.ID()).Q());
}

DEF_OP(AESDec) {
  auto Op = IROp->C<IR::IROp_VAESDec>();
  eor(VTMP2.Q(), VTMP2.Q(), VTMP2.Q());
  mov(VTMP1.Q(), GetVReg(Op->State.ID()).Q());
  aesd(VTMP1, VTMP2);
  aesimc(VTMP1, VTMP1);
  eor(GetVReg(Node).Q(), VTMP1.Q(), GetVReg(Op->Key.ID()).Q());
}

DEF_OP(AESDecLast) {
  auto Op = IROp->C<IR::IROp_VAESDecLast>();
  eor(VTMP2.Q(), VTMP2.Q(), VTMP2.Q());
  mov(VTMP1.Q(), GetVReg(Op->State.ID()).Q());
  aesd(VTMP1, VTMP2);
  eor(GetVReg(Node).Q(), VTMP1.Q(), GetVReg(Op->Key.ID()).Q());
}

DEF_OP(AESKeyGenAssist) {
  auto Op = IROp->C<IR::IROp_VAESKeyGenAssist>();

  ARMEmitter::ForwardLabel Constant;
  ARMEmitter::ForwardLabel PastConstant;

  // Do a "regular" AESE step
  eor(VTMP2.Q(), VTMP2.Q(), VTMP2.Q());
  mov(VTMP1.Q(), GetVReg(Op->Src.ID()).Q());
  aese(VTMP1, VTMP2);

  // Do a table shuffle to undo ShiftRows
  ldr(VTMP3.Q(), &Constant);

  // Now EOR in the RCON
  if (Op->RCON) {
    tbl(VTMP1.Q(), VTMP1.Q(), VTMP3.Q());

    LoadConstant(ARMEmitter::Size::i64Bit, TMP1, static_cast<uint64_t>(Op->RCON) << 32);
    dup(ARMEmitter::SubRegSize::i64Bit, VTMP2.Q(), TMP1);
    eor(GetVReg(Node).Q(), VTMP1.Q(), VTMP2.Q());
  }
  else {
    tbl(GetVReg(Node).Q(), VTMP1.Q(), VTMP3.Q());
  }

  b(&PastConstant);
  Bind(&Constant);
  dc64(0x040B0E01'0B0E0104ULL);
  dc64(0x0C030609'0306090CULL);
  Bind(&PastConstant);
}

DEF_OP(CRC32) {
  auto Op = IROp->C<IR::IROp_CRC32>();

  const auto Dst = GetReg(Node);
  const auto Src1 = GetReg(Op->Src1.ID());
  const auto Src2 = GetReg(Op->Src2.ID());

  switch (Op->SrcSize) {
    case 1:
      crc32cb(Dst.W(), Src1.W(), Src2.W());
      break;
    case 2:
      crc32ch(Dst.W(), Src1.W(), Src2.W());
      break;
    case 4:
      crc32cw(Dst.W(), Src1.W(), Src2.W());
      break;
    case 8:
      crc32cx(Dst, Src1, Src2);
      break;
    default: LOGMAN_MSG_A_FMT("Unknown CRC32 size: {}", Op->SrcSize);
  }
}

DEF_OP(PCLMUL) {
  auto Op = IROp->C<IR::IROp_PCLMUL>();

  auto Dst  = GetVReg(Node);
  auto Src1 = GetVReg(Op->Src1.ID());
  auto Src2 = GetVReg(Op->Src2.ID());

  switch (Op->Selector) {
  case 0b00000000:
    pmull(ARMEmitter::SubRegSize::i128Bit, Dst.D(), Src1.D(), Src2.D());
    break;
  case 0b00000001:
    dup(ARMEmitter::SubRegSize::i64Bit, VTMP1.Q(), Src1.Q(), 1);
    pmull(ARMEmitter::SubRegSize::i128Bit, Dst.D(), VTMP1.D(), Src2.D());
    break;
  case 0b00010000:
    dup(ARMEmitter::SubRegSize::i64Bit, VTMP1.Q(), Src2.Q(), 1);
    pmull(ARMEmitter::SubRegSize::i128Bit, Dst.D(), VTMP1.D(), Src1.D());
    break;
  case 0b00010001:
    pmull2(ARMEmitter::SubRegSize::i128Bit, Dst.Q(), Src1.Q(), Src2.Q());
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
