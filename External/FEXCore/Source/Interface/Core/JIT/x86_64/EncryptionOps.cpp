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
  const auto Op = IROp->C<IR::IROp_VAESEnc>();
  const auto OpSize = IROp->Size;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto Key = GetSrc(Op->Key.ID());
  const auto State = GetSrc(Op->State.ID());

  if (Is256Bit) {
    vaesenc(ToYMM(Dst), ToYMM(State), ToYMM(Key));
  } else {
    vaesenc(Dst, State, Key);
  }
}

DEF_OP(AESEncLast) {
  const auto Op = IROp->C<IR::IROp_VAESEncLast>();
  const auto OpSize = IROp->Size;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto Key = GetSrc(Op->Key.ID());
  const auto State = GetSrc(Op->State.ID());

  if (Is256Bit) {
    vaesenclast(ToYMM(Dst), ToYMM(State), ToYMM(Key));
  } else {
    vaesenclast(Dst, State, Key);
  }
}

DEF_OP(AESDec) {
  const auto Op = IROp->C<IR::IROp_VAESDec>();
  const auto OpSize = IROp->Size;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto Key = GetSrc(Op->Key.ID());
  const auto State = GetSrc(Op->State.ID());

  if (Is256Bit) {
    vaesdec(ToYMM(Dst), ToYMM(State), ToYMM(Key));
  } else {
    vaesdec(Dst, State, Key);
  }
}

DEF_OP(AESDecLast) {
  const auto Op = IROp->C<IR::IROp_VAESDecLast>();
  const auto OpSize = IROp->Size;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto Key = GetSrc(Op->Key.ID());
  const auto State = GetSrc(Op->State.ID());

  if (Is256Bit) {
    vaesdeclast(ToYMM(Dst), ToYMM(State), ToYMM(Key));
  } else {
    vaesdeclast(Dst, State, Key);
  }
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
  const auto Op = IROp->C<IR::IROp_PCLMUL>();
  const auto OpSize = IROp->Size;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto Src1 = GetSrc(Op->Src1.ID());
  const auto Src2 = GetSrc(Op->Src2.ID());

  switch (Op->Selector) {
  case 0b00000000:
  case 0b00000001:
  case 0b00010000:
  case 0b00010001:
    if (Is256Bit) {
      vpclmulqdq(ToYMM(Dst), ToYMM(Src1), ToYMM(Src2), Op->Selector);
    } else {
      vpclmulqdq(Dst, Src1, Src2, Op->Selector);
    }
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
