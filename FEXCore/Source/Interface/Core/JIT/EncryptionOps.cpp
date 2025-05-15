// SPDX-License-Identifier: MIT
/*
$info$
tags: backend|arm64
$end_info$
*/

#include "Interface/Core/JIT/JITClass.h"

namespace FEXCore::CPU {
#define DEF_OP(x) void Arm64JITCore::Op_##x(IR::IROp_Header const* IROp, IR::Ref Node)

DEF_OP(VAESImc) {
  auto Op = IROp->C<IR::IROp_VAESImc>();
  aesimc(GetVReg(Node), GetVReg(Op->Vector));
}

DEF_OP(VAESEnc) {
  const auto Op = IROp->C<IR::IROp_VAESEnc>();
  [[maybe_unused]] const auto OpSize = IROp->Size;

  const auto Dst = GetVReg(Node);
  const auto Key = GetVReg(Op->Key);
  const auto State = GetVReg(Op->State);
  const auto ZeroReg = GetVReg(Op->ZeroReg);

  LOGMAN_THROW_A_FMT(OpSize == IR::OpSize::i128Bit, "Currently only supports 128-bit operations.");

  if (Dst == State && Dst != Key) {
    // Optimal case in which Dst already contains the starting state.
    // This matches the common case of XMM AES.
    aese(Dst.Q(), ZeroReg.Q());
    aesmc(Dst.Q(), Dst.Q());
    eor(Dst.Q(), Dst.Q(), Key.Q());
  } else {
    mov(VTMP1.Q(), State.Q());
    aese(VTMP1, ZeroReg.Q());
    aesmc(VTMP1, VTMP1);
    eor(Dst.Q(), VTMP1.Q(), Key.Q());
  }
}

DEF_OP(VAESEncLast) {
  const auto Op = IROp->C<IR::IROp_VAESEncLast>();
  [[maybe_unused]] const auto OpSize = IROp->Size;

  const auto Dst = GetVReg(Node);
  const auto Key = GetVReg(Op->Key);
  const auto State = GetVReg(Op->State);
  const auto ZeroReg = GetVReg(Op->ZeroReg);

  LOGMAN_THROW_A_FMT(OpSize == IR::OpSize::i128Bit, "Currently only supports 128-bit operations.");

  if (Dst == State && Dst != Key) {
    // Optimal case in which Dst already contains the starting state.
    // This matches the common case of XMM AES.
    aese(Dst.Q(), ZeroReg.Q());
    eor(Dst.Q(), Dst.Q(), Key.Q());
  } else {
    mov(VTMP1.Q(), State.Q());
    aese(VTMP1, ZeroReg.Q());
    eor(Dst.Q(), VTMP1.Q(), Key.Q());
  }
}

DEF_OP(VAESDec) {
  const auto Op = IROp->C<IR::IROp_VAESDec>();
  [[maybe_unused]] const auto OpSize = IROp->Size;

  const auto Dst = GetVReg(Node);
  const auto Key = GetVReg(Op->Key);
  const auto State = GetVReg(Op->State);
  const auto ZeroReg = GetVReg(Op->ZeroReg);

  LOGMAN_THROW_A_FMT(OpSize == IR::OpSize::i128Bit, "Currently only supports 128-bit operations.");

  if (Dst == State && Dst != Key) {
    // Optimal case in which Dst already contains the starting state.
    // This matches the common case of XMM AES.
    aesd(Dst.Q(), ZeroReg.Q());
    aesimc(Dst.Q(), Dst.Q());
    eor(Dst.Q(), Dst.Q(), Key.Q());
  } else {
    mov(VTMP1.Q(), State.Q());
    aesd(VTMP1, ZeroReg.Q());
    aesimc(VTMP1, VTMP1);
    eor(Dst.Q(), VTMP1.Q(), Key.Q());
  }
}

DEF_OP(VAESDecLast) {
  const auto Op = IROp->C<IR::IROp_VAESDecLast>();
  [[maybe_unused]] const auto OpSize = IROp->Size;

  const auto Dst = GetVReg(Node);
  const auto Key = GetVReg(Op->Key);
  const auto State = GetVReg(Op->State);
  const auto ZeroReg = GetVReg(Op->ZeroReg);

  LOGMAN_THROW_A_FMT(OpSize == IR::OpSize::i128Bit, "Currently only supports 128-bit operations.");

  if (Dst == State && Dst != Key) {
    // Optimal case in which Dst already contains the starting state.
    // This matches the common case of XMM AES.
    aesd(Dst.Q(), ZeroReg.Q());
    eor(Dst.Q(), Dst.Q(), Key.Q());
  } else {
    mov(VTMP1.Q(), State.Q());
    aesd(VTMP1, ZeroReg.Q());
    eor(Dst.Q(), VTMP1.Q(), Key.Q());
  }
}

DEF_OP(VAESKeyGenAssist) {
  auto Op = IROp->C<IR::IROp_VAESKeyGenAssist>();
  const auto Dst = GetVReg(Node);
  const auto Src = GetVReg(Op->Src);
  const auto Swizzle = GetVReg(Op->KeyGenTBLSwizzle);
  auto ZeroReg = GetVReg(Op->ZeroReg);

  if (Dst == ZeroReg) {
    // Seriously? ZeroReg ended up being the destination register?
    // Just copy it over in this case...
    mov(VTMP1.Q(), ZeroReg.Q());
    ZeroReg = VTMP1;
  }

  if (Dst != Src) {
    mov(Dst.Q(), Src.Q());
  }

  // Do a "regular" AESE step
  aese(Dst, ZeroReg.Q());

  // Now EOR in the RCON
  if (Op->RCON) {
    tbl(Dst.Q(), Dst.Q(), Swizzle.Q());

    LoadConstant(ARMEmitter::Size::i64Bit, TMP1, static_cast<uint64_t>(Op->RCON) << 32);
    dup(ARMEmitter::SubRegSize::i64Bit, VTMP2.Q(), TMP1);
    eor(Dst.Q(), Dst.Q(), VTMP2.Q());
  } else {
    tbl(Dst.Q(), Dst.Q(), Swizzle.Q());
  }
}

DEF_OP(CRC32) {
  auto Op = IROp->C<IR::IROp_CRC32>();

  const auto Dst = GetReg(Node);
  const auto Src1 = GetReg(Op->Src1);
  const auto Src2 = GetReg(Op->Src2);

  switch (Op->SrcSize) {
  case IR::OpSize::i8Bit: crc32cb(Dst.W(), Src1.W(), Src2.W()); break;
  case IR::OpSize::i16Bit: crc32ch(Dst.W(), Src1.W(), Src2.W()); break;
  case IR::OpSize::i32Bit: crc32cw(Dst.W(), Src1.W(), Src2.W()); break;
  case IR::OpSize::i64Bit: crc32cx(Dst.X(), Src1.X(), Src2.X()); break;
  default: LOGMAN_MSG_A_FMT("Unknown CRC32 size: {}", Op->SrcSize);
  }
}

DEF_OP(VSha1H) {
  auto Op = IROp->C<IR::IROp_VSha1H>();

  const auto Dst = GetVReg(Node);
  const auto Src = GetVReg(Op->Src);

  sha1h(Dst.S(), Src.S());
}

DEF_OP(VSha1C) {
  auto Op = IROp->C<IR::IROp_VSha1C>();

  const auto Dst = GetVReg(Node);
  const auto Src1 = GetVReg(Op->Src1);
  const auto Src2 = GetVReg(Op->Src2);
  const auto Src3 = GetVReg(Op->Src3);

  if (Dst == Src1) {
    sha1c(Dst, Src2.S(), Src3);
  } else if (Dst != Src2 && Dst != Src3) {
    mov(Dst.Q(), Src1.Q());
    sha1c(Dst, Src2.S(), Src3);
  } else {
    mov(VTMP1.Q(), Src1.Q());
    sha1c(VTMP1, Src2.S(), Src3);
    mov(Dst.Q(), VTMP1.Q());
  }
}

DEF_OP(VSha1M) {
  auto Op = IROp->C<IR::IROp_VSha1M>();

  const auto Dst = GetVReg(Node);
  const auto Src1 = GetVReg(Op->Src1);
  const auto Src2 = GetVReg(Op->Src2);
  const auto Src3 = GetVReg(Op->Src3);

  if (Dst == Src1) {
    sha1m(Dst, Src2.S(), Src3);
  } else if (Dst != Src2 && Dst != Src3) {
    mov(Dst.Q(), Src1.Q());
    sha1m(Dst, Src2.S(), Src3);
  } else {
    mov(VTMP1.Q(), Src1.Q());
    sha1m(VTMP1, Src2.S(), Src3);
    mov(Dst.Q(), VTMP1.Q());
  }
}

DEF_OP(VSha1P) {
  auto Op = IROp->C<IR::IROp_VSha1P>();

  const auto Dst = GetVReg(Node);
  const auto Src1 = GetVReg(Op->Src1);
  const auto Src2 = GetVReg(Op->Src2);
  const auto Src3 = GetVReg(Op->Src3);

  if (Dst == Src1) {
    sha1p(Dst, Src2.S(), Src3);
  } else if (Dst != Src2 && Dst != Src3) {
    mov(Dst.Q(), Src1.Q());
    sha1p(Dst, Src2.S(), Src3);
  } else {
    mov(VTMP1.Q(), Src1.Q());
    sha1p(VTMP1, Src2.S(), Src3);
    mov(Dst.Q(), VTMP1.Q());
  }
}

DEF_OP(VSha1SU1) {
  auto Op = IROp->C<IR::IROp_VSha1SU1>();

  const auto Dst = GetVReg(Node);
  const auto Src1 = GetVReg(Op->Src1);
  const auto Src2 = GetVReg(Op->Src2);

  if (Dst == Src1) {
    sha1su1(Dst, Src2);
  } else if (Dst != Src2) {
    mov(Dst.Q(), Src1.Q());
    sha1su1(Dst, Src2);
  } else {
    mov(VTMP1.Q(), Src1.Q());
    sha1su1(VTMP1, Src2);
    mov(Dst.Q(), VTMP1.Q());
  }
}

DEF_OP(VSha256H) {
  auto Op = IROp->C<IR::IROp_VSha256H>();

  const auto Dst = GetVReg(Node);
  const auto Src1 = GetVReg(Op->Src1);
  const auto Src2 = GetVReg(Op->Src2);
  const auto Src3 = GetVReg(Op->Src3);

  if (Dst == Src1) {
    sha256h(Dst, Src2, Src3);
  } else if (Dst != Src2 && Dst != Src3) {
    mov(Dst.Q(), Src1.Q());
    sha256h(Dst, Src2, Src3);
  } else {
    mov(VTMP1.Q(), Src1.Q());
    sha256h(VTMP1, Src2, Src3);
    mov(Dst.Q(), VTMP1.Q());
  }
}

DEF_OP(VSha256H2) {
  auto Op = IROp->C<IR::IROp_VSha256H2>();

  const auto Dst = GetVReg(Node);
  const auto Src1 = GetVReg(Op->Src1);
  const auto Src2 = GetVReg(Op->Src2);
  const auto Src3 = GetVReg(Op->Src3);

  if (Dst == Src1) {
    sha256h2(Dst, Src2, Src3);
  } else if (Dst != Src2 && Dst != Src3) {
    mov(Dst.Q(), Src1.Q());
    sha256h2(Dst, Src2, Src3);
  } else {
    mov(VTMP1.Q(), Src1.Q());
    sha256h2(VTMP1, Src2, Src3);
    mov(Dst.Q(), VTMP1.Q());
  }
}

DEF_OP(VSha256U0) {
  auto Op = IROp->C<IR::IROp_VSha256U0>();

  const auto Dst = GetVReg(Node);
  const auto Src1 = GetVReg(Op->Src1);
  const auto Src2 = GetVReg(Op->Src2);

  if (Dst == Src1) {
    sha256su0(Dst, Src2);
  } else {
    mov(VTMP1.Q(), Src1.Q());
    sha256su0(VTMP1, Src2);
    mov(Dst.Q(), VTMP1.Q());
  }
}

DEF_OP(VSha256U1) {
  auto Op = IROp->C<IR::IROp_VSha256U1>();

  const auto Dst = GetVReg(Node);
  const auto Src1 = GetVReg(Op->Src1);
  const auto Src2 = GetVReg(Op->Src2);

  if (Dst != Src1 && Dst != Src2) {
    movi(ARMEmitter::SubRegSize::i64Bit, Dst.Q(), 0);
    sha256su1(Dst, Src1, Src2);
  } else {
    movi(ARMEmitter::SubRegSize::i64Bit, VTMP1.Q(), 0);
    sha256su1(VTMP1, Src1, Src2);
    mov(Dst.Q(), VTMP1.Q());
  }
}

DEF_OP(PCLMUL) {
  const auto Op = IROp->C<IR::IROp_PCLMUL>();
  [[maybe_unused]] const auto OpSize = IROp->Size;

  const auto Dst = GetVReg(Node);
  const auto Src1 = GetVReg(Op->Src1);
  const auto Src2 = GetVReg(Op->Src2);

  LOGMAN_THROW_A_FMT(OpSize == IR::OpSize::i128Bit, "Currently only supports 128-bit operations.");

  switch (Op->Selector) {
  case 0b00000000: pmull(ARMEmitter::SubRegSize::i128Bit, Dst.D(), Src1.D(), Src2.D()); break;
  case 0b00000001:
    dup(ARMEmitter::SubRegSize::i64Bit, VTMP1.Q(), Src1.Q(), 1);
    pmull(ARMEmitter::SubRegSize::i128Bit, Dst.D(), VTMP1.D(), Src2.D());
    break;
  case 0b00010000:
    dup(ARMEmitter::SubRegSize::i64Bit, VTMP1.Q(), Src2.Q(), 1);
    pmull(ARMEmitter::SubRegSize::i128Bit, Dst.D(), VTMP1.D(), Src1.D());
    break;
  case 0b00010001: pmull2(ARMEmitter::SubRegSize::i128Bit, Dst.Q(), Src1.Q(), Src2.Q()); break;
  default: LOGMAN_MSG_A_FMT("Unknown PCLMUL selector: {}", Op->Selector); break;
  }
}

#undef DEF_OP
} // namespace FEXCore::CPU
