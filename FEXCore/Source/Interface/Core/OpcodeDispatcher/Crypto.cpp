// SPDX-License-Identifier: MIT
/*
$info$
tags: frontend|x86-to-ir, opcodes|dispatcher-implementations
desc: Handles x86/64 Crypto instructions to IR
$end_info$
*/

#include "Interface/Core/X86Tables/X86Tables.h"

#include <FEXCore/Utils/LogManager.h>
#include "Interface/Core/OpcodeDispatcher.h"

#include <cstdint>

namespace FEXCore::IR {
class OrderedNode;

#define OpcodeArgs [[maybe_unused]] FEXCore::X86Tables::DecodedOp Op

void OpDispatchBuilder::SHA1NEXTEOp(OpcodeArgs) {
  if (!CTX->HostFeatures.SupportsSHA) {
    UnimplementedOp(Op);
    return;
  }
  Ref Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

  // ARMv8 SHA1 extension provides a `SHA1H` instruction which does a fixed rotate by 30.
  // This only operates on element 0 rather than element 3. We don't have the luxury of rewriting the x86 SHA algorithm to take advantage of this.
  // Move the element to zero, rotate, and then move back (Using duplicates).
  // Saves one instruction versus that path that doesn't support SHA extension.
  auto Duplicated = _VDupElement(OpSize::i128Bit, OpSize::i32Bit, Dest, 3);
  auto Sha1HRotated = _VSha1H(Duplicated);
  auto RotatedNode = _VDupElement(OpSize::i128Bit, OpSize::i32Bit, Sha1HRotated, 0);
  auto Tmp = _VAdd(OpSize::i128Bit, OpSize::i32Bit, Src, RotatedNode);
  auto Result = _VInsElement(OpSize::i128Bit, OpSize::i32Bit, 3, 3, Src, Tmp);

  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::SHA1MSG1Op(OpcodeArgs) {
  if (!CTX->HostFeatures.SupportsSHA) {
    UnimplementedOp(Op);
    return;
  }
  Ref Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

  Ref NewVec = _VExtr(OpSize::i128Bit, OpSize::i64Bit, Dest, Src, 1);

  // [W0, W1, W2, W3] ^ [W2, W3, W4, W5]
  Ref Result = _VXor(OpSize::i128Bit, OpSize::i8Bit, Dest, NewVec);

  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::SHA1MSG2Op(OpcodeArgs) {
  if (!CTX->HostFeatures.SupportsSHA) {
    UnimplementedOp(Op);
    return;
  }
  Ref Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

  // ARM SHA1 mostly matches x86 semantics, except the input and outputs are both flipped from elements 0,1,2,3 to 3,2,1,0.
  auto Src1 = SHADataShuffle(Dest);
  auto Src2 = SHADataShuffle(Src);

  // The result is swizzled differently than expected
  auto Result = SHADataShuffle(_VSha1SU1(Src1, Src2));

  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::SHA1RNDS4Op(OpcodeArgs) {
  if (!CTX->HostFeatures.SupportsSHA) {
    UnimplementedOp(Op);
    return;
  }
  const uint64_t Imm8 = Op->Src[1].Literal() & 0b11;
  Ref Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

  Ref Result {};
  Ref ConstantVector {};
  switch (Imm8) {
  case 0:
    ConstantVector = LoadAndCacheNamedVectorConstant(OpSize::i128Bit, FEXCore::IR::NamedVectorConstant::NAMED_VECTOR_SHA1RNDS_K0);
    break;
  case 1:
    ConstantVector = LoadAndCacheNamedVectorConstant(OpSize::i128Bit, FEXCore::IR::NamedVectorConstant::NAMED_VECTOR_SHA1RNDS_K1);
    break;
  case 2:
    ConstantVector = LoadAndCacheNamedVectorConstant(OpSize::i128Bit, FEXCore::IR::NamedVectorConstant::NAMED_VECTOR_SHA1RNDS_K2);
    break;
  case 3:
    ConstantVector = LoadAndCacheNamedVectorConstant(OpSize::i128Bit, FEXCore::IR::NamedVectorConstant::NAMED_VECTOR_SHA1RNDS_K3);
    break;
  }

  const auto ZeroRegister = LoadZeroVector(OpSize::i32Bit);

  Ref Src1 = SHADataShuffle(Dest);
  Ref Src2 = SHADataShuffle(Src);
  Src2 = _VAdd(OpSize::i128Bit, OpSize::i32Bit, Src2, ConstantVector);

  switch (Imm8) {
  case 0: Result = SHADataShuffle(_VSha1C(Src1, ZeroRegister, Src2)); break;
  case 2: Result = SHADataShuffle(_VSha1M(Src1, ZeroRegister, Src2)); break;
  case 1:
  case 3: Result = SHADataShuffle(_VSha1P(Src1, ZeroRegister, Src2)); break;
  }

  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::SHA256MSG1Op(OpcodeArgs) {
  if (!CTX->HostFeatures.SupportsSHA) {
    UnimplementedOp(Op);
    return;
  }
  Ref Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

  auto Result = _VSha256U0(Dest, Src);

  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::SHA256MSG2Op(OpcodeArgs) {
  if (!CTX->HostFeatures.SupportsSHA) {
    UnimplementedOp(Op);
    return;
  }
  Ref Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

  auto Src1 = _VExtr(OpSize::i128Bit, OpSize::i32Bit, Dest, Dest, 3);
  auto DupDst = _VDupElement(OpSize::i128Bit, OpSize::i32Bit, Dest, 3);
  auto Src2 = _VZip2(OpSize::i128Bit, OpSize::i64Bit, DupDst, Src);

  auto Result = _VSha256U1(Src1, Src2);

  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::SHA256RNDS2Op(OpcodeArgs) {
  if (!CTX->HostFeatures.SupportsSHA) {
    UnimplementedOp(Op);
    return;
  }
  Ref Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  // Hardcoded to XMM0
  auto XMM0 = LoadXMMRegister(0);

  auto shuffle_abcd = [this](Ref Src1, Ref Src2) -> Ref {
    // Generates a suitable SHA256 `abcd` configuration from x86 format.
    auto Tmp = _VZip2(OpSize::i128Bit, OpSize::i64Bit, Src2, Src1);
    return _VRev64(OpSize::i128Bit, OpSize::i32Bit, Tmp);
  };

  auto shuffle_efgh = [this](Ref Src1, Ref Src2) -> Ref {
    // Generates a suitable SHA256 `efgh` configuration from x86 format.
    auto Tmp = _VZip(OpSize::i128Bit, OpSize::i64Bit, Src2, Src1);
    return _VRev64(OpSize::i128Bit, OpSize::i32Bit, Tmp);
  };

  auto ABCD = shuffle_abcd(Dest, Src);
  auto EFGH = shuffle_efgh(Dest, Src);

  // x86 uses only the bottom 64-bits of the key, so duplicate to match ARM64 semantics.
  auto Key = _VDupElement(OpSize::i128Bit, OpSize::i64Bit, XMM0, 0);

  auto A = _VSha256H(ABCD, EFGH, Key);
  auto B = _VSha256H2(EFGH, ABCD, Key);
  auto Result = shuffle_abcd(A, B);

  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::AESImcOp(OpcodeArgs) {
  if (!CTX->HostFeatures.SupportsAES) {
    UnimplementedOp(Op);
    return;
  }
  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Result = _VAESImc(Src);
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::AESEncOp(OpcodeArgs) {
  if (!CTX->HostFeatures.SupportsAES) {
    UnimplementedOp(Op);
    return;
  }
  Ref Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Result = _VAESEnc(OpSize::i128Bit, Dest, Src, LoadZeroVector(OpSize::i128Bit));
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::VAESEncOp(OpcodeArgs) {
  const auto DstSize = OpSizeFromDst(Op);
  const auto Is128Bit = DstSize == OpSize::i128Bit;

  // TODO: Handle 256-bit VAESENC.
  LOGMAN_THROW_A_FMT(Is128Bit, "256-bit VAESENC unimplemented");

  Ref State = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Key = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);
  Ref Result = _VAESEnc(DstSize, State, Key, LoadZeroVector(DstSize));

  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::AESEncLastOp(OpcodeArgs) {
  if (!CTX->HostFeatures.SupportsAES) {
    UnimplementedOp(Op);
    return;
  }
  Ref Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Result = _VAESEncLast(OpSize::i128Bit, Dest, Src, LoadZeroVector(OpSize::i128Bit));
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::VAESEncLastOp(OpcodeArgs) {
  const auto DstSize = OpSizeFromDst(Op);
  const auto Is128Bit = DstSize == OpSize::i128Bit;

  // TODO: Handle 256-bit VAESENCLAST.
  LOGMAN_THROW_A_FMT(Is128Bit, "256-bit VAESENCLAST unimplemented");

  Ref State = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Key = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);
  Ref Result = _VAESEncLast(DstSize, State, Key, LoadZeroVector(DstSize));

  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::AESDecOp(OpcodeArgs) {
  if (!CTX->HostFeatures.SupportsAES) {
    UnimplementedOp(Op);
    return;
  }
  Ref Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Result = _VAESDec(OpSize::i128Bit, Dest, Src, LoadZeroVector(OpSize::i128Bit));
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::VAESDecOp(OpcodeArgs) {
  const auto DstSize = OpSizeFromDst(Op);
  const auto Is128Bit = DstSize == OpSize::i128Bit;

  // TODO: Handle 256-bit VAESDEC.
  LOGMAN_THROW_A_FMT(Is128Bit, "256-bit VAESDEC unimplemented");

  Ref State = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Key = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);
  Ref Result = _VAESDec(DstSize, State, Key, LoadZeroVector(DstSize));

  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::AESDecLastOp(OpcodeArgs) {
  if (!CTX->HostFeatures.SupportsAES) {
    UnimplementedOp(Op);
    return;
  }
  Ref Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Result = _VAESDecLast(OpSize::i128Bit, Dest, Src, LoadZeroVector(OpSize::i128Bit));
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::VAESDecLastOp(OpcodeArgs) {
  const auto DstSize = OpSizeFromDst(Op);
  const auto Is128Bit = DstSize == OpSize::i128Bit;

  // TODO: Handle 256-bit VAESDECLAST.
  LOGMAN_THROW_A_FMT(Is128Bit, "256-bit VAESDECLAST unimplemented");

  Ref State = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Key = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);
  Ref Result = _VAESDecLast(DstSize, State, Key, LoadZeroVector(DstSize));

  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

Ref OpDispatchBuilder::AESKeyGenAssistImpl(OpcodeArgs) {
  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  const uint64_t RCON = Op->Src[1].Literal();

  auto KeyGenSwizzle = LoadAndCacheNamedVectorConstant(OpSize::i128Bit, NAMED_VECTOR_AESKEYGENASSIST_SWIZZLE);
  return _VAESKeyGenAssist(Src, KeyGenSwizzle, LoadZeroVector(OpSize::i128Bit), RCON);
}

void OpDispatchBuilder::AESKeyGenAssist(OpcodeArgs) {
  Ref Result = AESKeyGenAssistImpl(Op);
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::PCLMULQDQOp(OpcodeArgs) {
  Ref Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  const auto Selector = static_cast<uint8_t>(Op->Src[1].Literal());

  auto Res = _PCLMUL(OpSize::i128Bit, Dest, Src, Selector & 0b1'0001);
  StoreResult(FPRClass, Op, Res, OpSize::iInvalid);
}

void OpDispatchBuilder::VPCLMULQDQOp(OpcodeArgs) {
  const auto DstSize = OpSizeFromDst(Op);

  Ref Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);
  const auto Selector = static_cast<uint8_t>(Op->Src[2].Literal());

  Ref Res = _PCLMUL(DstSize, Src1, Src2, Selector & 0b1'0001);
  StoreResult(FPRClass, Op, Res, OpSize::iInvalid);
}

} // namespace FEXCore::IR
