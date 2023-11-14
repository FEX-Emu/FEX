// SPDX-License-Identifier: MIT
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

DEF_OP(VAESImc) {
  auto Op = IROp->C<IR::IROp_VAESImc>();
  aesimc(GetVReg(Node), GetVReg(Op->Vector.ID()));
}

DEF_OP(VAESEnc) {
  const auto Op = IROp->C<IR::IROp_VAESEnc>();
  const auto OpSize = IROp->Size;

  const auto Dst = GetVReg(Node);
  const auto Key = GetVReg(Op->Key.ID());
  const auto State = GetVReg(Op->State.ID());
  const auto ZeroReg = GetVReg(Op->ZeroReg.ID());

  LOGMAN_THROW_AA_FMT(OpSize == Core::CPUState::XMM_SSE_REG_SIZE,
                      "Currently only supports 128-bit operations.");

  if (Dst == State && Dst != Key) {
    // Optimal case in which Dst already contains the starting state.
    // This matches the common case of XMM AES.
    aese(Dst.Q(), ZeroReg.Q());
    aesmc(Dst.Q(), Dst.Q());
    eor(Dst.Q(), Dst.Q(), Key.Q());
  }
  else {
    mov(VTMP1.Q(), State.Q());
    aese(VTMP1, ZeroReg.Q());
    aesmc(VTMP1, VTMP1);
    eor(Dst.Q(), VTMP1.Q(), Key.Q());
  }
}

DEF_OP(VAESEncLast) {
  const auto Op = IROp->C<IR::IROp_VAESEncLast>();
  const auto OpSize = IROp->Size;

  const auto Dst = GetVReg(Node);
  const auto Key = GetVReg(Op->Key.ID());
  const auto State = GetVReg(Op->State.ID());
  const auto ZeroReg = GetVReg(Op->ZeroReg.ID());

  LOGMAN_THROW_AA_FMT(OpSize == Core::CPUState::XMM_SSE_REG_SIZE,
                      "Currently only supports 128-bit operations.");

  if (Dst == State && Dst != Key) {
    // Optimal case in which Dst already contains the starting state.
    // This matches the common case of XMM AES.
    aese(Dst.Q(), ZeroReg.Q());
    eor(Dst.Q(), Dst.Q(), Key.Q());
  }
  else {
    mov(VTMP1.Q(), State.Q());
    aese(VTMP1, ZeroReg.Q());
    eor(Dst.Q(), VTMP1.Q(), Key.Q());
  }
}

DEF_OP(VAESDec) {
  const auto Op = IROp->C<IR::IROp_VAESDec>();
  const auto OpSize = IROp->Size;

  const auto Dst = GetVReg(Node);
  const auto Key = GetVReg(Op->Key.ID());
  const auto State = GetVReg(Op->State.ID());
  const auto ZeroReg = GetVReg(Op->ZeroReg.ID());

  LOGMAN_THROW_AA_FMT(OpSize == Core::CPUState::XMM_SSE_REG_SIZE,
                      "Currently only supports 128-bit operations.");

  if (Dst == State && Dst != Key) {
    // Optimal case in which Dst already contains the starting state.
    // This matches the common case of XMM AES.
    aesd(Dst.Q(), ZeroReg.Q());
    aesimc(Dst.Q(), Dst.Q());
    eor(Dst.Q(), Dst.Q(), Key.Q());
  }
  else {
    mov(VTMP1.Q(), State.Q());
    aesd(VTMP1, ZeroReg.Q());
    aesimc(VTMP1, VTMP1);
    eor(Dst.Q(), VTMP1.Q(), Key.Q());
  }
}

DEF_OP(VAESDecLast) {
  const auto Op = IROp->C<IR::IROp_VAESDecLast>();
  const auto OpSize = IROp->Size;

  const auto Dst = GetVReg(Node);
  const auto Key = GetVReg(Op->Key.ID());
  const auto State = GetVReg(Op->State.ID());
  const auto ZeroReg = GetVReg(Op->ZeroReg.ID());

  LOGMAN_THROW_AA_FMT(OpSize == Core::CPUState::XMM_SSE_REG_SIZE,
                      "Currently only supports 128-bit operations.");

  if (Dst == State && Dst != Key) {
    // Optimal case in which Dst already contains the starting state.
    // This matches the common case of XMM AES.
    aesd(Dst.Q(), ZeroReg.Q());
    eor(Dst.Q(), Dst.Q(), Key.Q());
  }
  else {
    mov(VTMP1.Q(), State.Q());
    aesd(VTMP1, ZeroReg.Q());
    eor(Dst.Q(), VTMP1.Q(), Key.Q());
  }
}

DEF_OP(VAESKeyGenAssist) {
  auto Op = IROp->C<IR::IROp_VAESKeyGenAssist>();
  const auto Dst = GetVReg(Node);
  const auto Src = GetVReg(Op->Src.ID());
  const auto Swizzle = GetVReg(Op->KeyGenTBLSwizzle.ID());
  auto ZeroReg = GetVReg(Op->ZeroReg.ID());

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
  }
  else {
    tbl(Dst.Q(), Dst.Q(), Swizzle.Q());
  }
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
      crc32cx(Dst.X(), Src1.X(), Src2.X());
      break;
    default: LOGMAN_MSG_A_FMT("Unknown CRC32 size: {}", Op->SrcSize);
  }
}

DEF_OP(VSha1H) {
  auto Op = IROp->C<IR::IROp_VSha1H>();

  const auto Dst = GetVReg(Node);
  const auto Src = GetVReg(Op->Src.ID());

  sha1h(Dst.S(), Src.S());
}

DEF_OP(VSha256U0) {
  auto Op = IROp->C<IR::IROp_VSha256U0>();

  const auto Dst = GetVReg(Node);
  const auto Src1 = GetVReg(Op->Src1.ID());
  const auto Src2 = GetVReg(Op->Src2.ID());

  if (Dst == Src1) {
    sha256su0(Dst, Src2);
  }
  else {
    mov(VTMP1.Q(), Src1.Q());
    sha256su0(VTMP1, Src2);
    mov(Dst.Q(), Src1.Q());
  }
}

DEF_OP(PCLMUL) {
  const auto Op = IROp->C<IR::IROp_PCLMUL>();
  const auto OpSize = IROp->Size;

  const auto Dst  = GetVReg(Node);
  const auto Src1 = GetVReg(Op->Src1.ID());
  const auto Src2 = GetVReg(Op->Src2.ID());

  LOGMAN_THROW_AA_FMT(OpSize == Core::CPUState::XMM_SSE_REG_SIZE,
                      "Currently only supports 128-bit operations.");

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
}
