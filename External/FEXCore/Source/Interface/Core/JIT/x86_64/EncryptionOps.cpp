/*
$info$
tags: backend|x86-64
$end_info$
*/

#include "Interface/Core/JIT/x86_64/JITClass.h"

#include <FEXCore/IR/IR.h>

#include <array>
#include <stdint.h>
#include <xbyak/xbyak.h>

namespace FEXCore::CPU {
#define DEF_OP(x) void X86JITCore::Op_##x(IR::IROp_Header *IROp, IR::NodeID Node)

DEF_OP(AESImc) {
  auto Op = IROp->C<IR::IROp_VAESImc>();
  vaesimc(GetDst(Node), GetSrc(Op->Vector.ID()));
}

DEF_OP(AESEnc) {
  auto Op = IROp->C<IR::IROp_VAESEnc>();
  vaesenc(GetDst(Node), GetSrc(Op->State.ID()), GetSrc(Op->Key.ID()));
}

DEF_OP(AESEncLast) {
  auto Op = IROp->C<IR::IROp_VAESEncLast>();
  vaesenclast(GetDst(Node), GetSrc(Op->State.ID()), GetSrc(Op->Key.ID()));
}

DEF_OP(AESDec) {
  auto Op = IROp->C<IR::IROp_VAESDec>();
  vaesdec(GetDst(Node), GetSrc(Op->State.ID()), GetSrc(Op->Key.ID()));
}

DEF_OP(AESDecLast) {
  auto Op = IROp->C<IR::IROp_VAESDecLast>();
  vaesdeclast(GetDst(Node), GetSrc(Op->State.ID()), GetSrc(Op->Key.ID()));
}

DEF_OP(AESKeyGenAssist) {
  auto Op = IROp->C<IR::IROp_VAESKeyGenAssist>();
  vaeskeygenassist(GetDst(Node), GetSrc(Op->Src.ID()), Op->RCON);
}

DEF_OP(CRC32) {
  auto Op = IROp->C<IR::IROp_CRC32>();
  switch (IROp->Size) {
  case 4:
    mov(TMP1, GetSrc<RA_32>(Op->Src2.ID()));
    mov(GetDst<RA_32>(Node), GetSrc<RA_32>(Op->Src1.ID()));
  break;
  case 8:
    mov(TMP1, GetSrc<RA_64>(Op->Src2.ID()));
    mov(GetDst<RA_64>(Node), GetSrc<RA_64>(Op->Src1.ID()));
  break;
  default: LOGMAN_MSG_A_FMT("Unknown CRC32 size: {}", IROp->Size);
  }

  switch (Op->SrcSize) {
    case 1:
      crc32(GetDst<RA_32>(Node).cvt32(), TMP1.cvt8());
      break;
    case 2:
      crc32(GetDst<RA_32>(Node).cvt32(), TMP1.cvt16());
      break;
    case 4:
      crc32(GetDst<RA_32>(Node).cvt32(), TMP1.cvt32());
      break;
    case 8:
      crc32(GetDst<RA_64>(Node).cvt64(), TMP1.cvt64());
      break;
  }
}

DEF_OP(PCLMUL) {
  auto Op = IROp->C<IR::IROp_PCLMUL>();

  auto Dst = GetDst(Node);
  auto Src1 = GetSrc(Op->Src1.ID());
  auto Src2 = GetSrc(Op->Src2.ID());

  switch (Op->Selector) {
  case 0b00000000:
  case 0b00000001:
  case 0b00010000:
  case 0b00010001:
    vpclmulqdq(Dst, Src1, Src2, Op->Selector);
    break;
  default:
    LOGMAN_MSG_A_FMT("Unknown PCLMUL selector: {}", Op->Selector);
    break;
  }
}

#undef DEF_OP
void X86JITCore::RegisterEncryptionHandlers() {
#define REGISTER_OP(op, x) OpHandlers[FEXCore::IR::IROps::OP_##op] = &X86JITCore::Op_##x
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
