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
  Ref Dest = LoadSourceFPR(Op, Op->Dest, Op->Flags);
  Ref Src = LoadSourceFPR(Op, Op->Src[0], Op->Flags);

  Ref Result {};
  if (CTX->HostFeatures.SupportsSVE128) {
    auto ZeroVec = LoadZeroVector(OpSize::i128Bit);
    auto Tmp = _VInsElement(OpSize::i128Bit, OpSize::i32Bit, 3, 3, ZeroVec, Dest);
    auto Xar = _VXar(OpSize::i128Bit, OpSize::i32Bit, ZeroVec, Tmp, 2);
    Result = _VAdd(OpSize::i128Bit, OpSize::i32Bit, Src, Xar);
  } else {
    // ARMv8 SHA1 extension provides a `SHA1H` instruction which does a fixed rotate by 30.
    // This only operates on element 0 rather than element 3. We don't have the luxury of rewriting the x86 SHA algorithm to take advantage of this.
    // Move the element to zero, rotate, and then move back (Using duplicates).
    // Saves one instruction versus that path that doesn't support SHA extension.
    auto Duplicated = _VDupElement(OpSize::i128Bit, OpSize::i32Bit, Dest, 3);
    auto Sha1HRotated = _VSha1H(Duplicated);
    auto RotatedNode = _VDupElement(OpSize::i128Bit, OpSize::i32Bit, Sha1HRotated, 0);
    auto Tmp = _VAdd(OpSize::i128Bit, OpSize::i32Bit, Src, RotatedNode);
    Result = _VInsElement(OpSize::i128Bit, OpSize::i32Bit, 3, 3, Src, Tmp);
  }

  StoreResult_WithAVXInsert(VectorOpType::SSE, RegClass::FPR, Op, Result);
}

void OpDispatchBuilder::SHA1MSG1Op(OpcodeArgs) {
  if (!CTX->HostFeatures.SupportsSHA) {
    UnimplementedOp(Op);
    return;
  }
  Ref Dest = LoadSourceFPR(Op, Op->Dest, Op->Flags);
  Ref Src = LoadSourceFPR(Op, Op->Src[0], Op->Flags);

  Ref NewVec = _VExtr(OpSize::i128Bit, OpSize::i64Bit, Dest, Src, 1);

  // [W0, W1, W2, W3] ^ [W2, W3, W4, W5]
  Ref Result = _VXor(OpSize::i128Bit, Dest, NewVec);

  StoreResult_WithAVXInsert(VectorOpType::SSE, RegClass::FPR, Op, Result);
}

void OpDispatchBuilder::SHA1MSG2Op(OpcodeArgs) {
  if (!CTX->HostFeatures.SupportsSHA) {
    UnimplementedOp(Op);
    return;
  }
  Ref Dest = LoadSourceFPR(Op, Op->Dest, Op->Flags);
  Ref Src = LoadSourceFPR(Op, Op->Src[0], Op->Flags);

  // ARM SHA1 mostly matches x86 semantics, except the input and outputs are both flipped from elements 0,1,2,3 to 3,2,1,0.
  auto Src1 = SHADataShuffle(Dest);
  auto Src2 = SHADataShuffle(Src);

  // The result is swizzled differently than expected
  auto Result = SHADataShuffle(_VSha1SU1(Src1, Src2));

  StoreResult_WithAVXInsert(VectorOpType::SSE, RegClass::FPR, Op, Result);
}

void OpDispatchBuilder::SHA1RNDS4Op(OpcodeArgs) {
  if (!CTX->HostFeatures.SupportsSHA) {
    UnimplementedOp(Op);
    return;
  }
  const uint64_t Imm8 = Op->Src[1].Literal() & 0b11;
  Ref Dest = LoadSourceFPR(Op, Op->Dest, Op->Flags);
  Ref Src = LoadSourceFPR(Op, Op->Src[0], Op->Flags);

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

  const auto ZeroRegister = LoadZeroVector(OpSize::i128Bit);

  Ref Src1 = SHADataShuffle(Dest);
  Ref Src2 = SHADataShuffle(Src);
  Src2 = _VAdd(OpSize::i128Bit, OpSize::i32Bit, Src2, ConstantVector);

  switch (Imm8) {
  case 0: Result = SHADataShuffle(_VSha1C(Src1, ZeroRegister, Src2)); break;
  case 2: Result = SHADataShuffle(_VSha1M(Src1, ZeroRegister, Src2)); break;
  case 1:
  case 3: Result = SHADataShuffle(_VSha1P(Src1, ZeroRegister, Src2)); break;
  }

  StoreResult_WithAVXInsert(VectorOpType::SSE, RegClass::FPR, Op, Result);
}

void OpDispatchBuilder::SHA256MSG1Op(OpcodeArgs) {
  if (!CTX->HostFeatures.SupportsSHA) {
    UnimplementedOp(Op);
    return;
  }
  Ref Dest = LoadSourceFPR(Op, Op->Dest, Op->Flags);
  Ref Src = LoadSourceFPR(Op, Op->Src[0], Op->Flags);

  auto Result = _VSha256U0(Dest, Src);

  StoreResult_WithAVXInsert(VectorOpType::SSE, RegClass::FPR, Op, Result);
}

void OpDispatchBuilder::SHA256MSG2Op(OpcodeArgs) {
  if (!CTX->HostFeatures.SupportsSHA) {
    UnimplementedOp(Op);
    return;
  }
  Ref Dest = LoadSourceFPR(Op, Op->Dest, Op->Flags);
  Ref Src = LoadSourceFPR(Op, Op->Src[0], Op->Flags);

  auto Src1 = _VExtr(OpSize::i128Bit, OpSize::i32Bit, Dest, Dest, 3);
  auto DupDst = _VDupElement(OpSize::i128Bit, OpSize::i32Bit, Dest, 3);
  auto Src2 = _VZip2(OpSize::i128Bit, OpSize::i64Bit, DupDst, Src);

  auto Result = _VSha256U1(Src1, Src2);

  StoreResult_WithAVXInsert(VectorOpType::SSE, RegClass::FPR, Op, Result);
}

void OpDispatchBuilder::SHA256RNDS2Op(OpcodeArgs) {
  if (!CTX->HostFeatures.SupportsSHA) {
    UnimplementedOp(Op);
    return;
  }
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

  // Peephole: Attempt to identify this pattern:
  //   sha256rnds2 xmm1, xmm2
  //   pshufd xmm0, xmm0, 0xe (or psrldq xmm0, 8)
  //   sha256rnds2 xmm2, xmm1
  // This is an idiom used by various crypto libraries (OpenSSL etc).
  // Since the ARM64 SHA256 instructions compute all four rounds,
  // we can elide the second SHA256RNDS2 and the key shuffle.
  // Since this matching is done on IR nodes, this is somewhat fragile,
  // since changes to the translation of the constitiuent instructions
  // would break the pattern.
  //
  // NB: xmm1 and xmm2 are used here as examples, but note that
  // xmm0 is hardcoded as the key register in SHA256RNDS2.

  if (Op->Dest.IsGPR() && Op->Src[0].IsGPR()) {
    // xmm1 in our example above.
    const auto X = Op->Src[0].Data.GPR.GPR - X86State::REG_XMM_0;
    // xmm2 in our example above.
    const auto Y = Op->Dest.Data.GPR.GPR - X86State::REG_XMM_0;

    const int KeyIndex = FPR0Index;
    const int XIndex = FPR0Index + X;
    const int YIndex = FPR0Index + Y;

    // It is helpful to remember that the current Op
    // is the "second" SHA256RNDS2 in the example, so we are looking
    // backwards to find the rest of the pattern.
    const Ref XLastWritten = RegCache.LastDef[XIndex];
    const auto XLastWrittenIndex = RegCache.LastDefIndex[XIndex];
    const Ref KeyLastWritten = RegCache.LastDef[KeyIndex];
    // xmm0 and X must have been written to in in this block.
    if (!XLastWritten || !KeyLastWritten) {
      goto no_peephole;
    }

    // True if no instruction wrote Y after the first SHA256RNDS2.
    const bool YUnchanged = !RegCache.LastDef[YIndex] || RegCache.LastDefIndex[YIndex] < XLastWrittenIndex;
    // True if the key shift is after the first SHA256RNDS2, and no instruction wrote XMM0 afterwards.
    const bool KeyUnchanged = RegCache.LastDefIndex[KeyIndex] > XLastWrittenIndex &&
                              (!RegCache.PrevDef[KeyIndex] || RegCache.PrevDefIndex[KeyIndex] < XLastWrittenIndex);
    if (!YUnchanged || !KeyUnchanged) {
      goto no_peephole;
    }

    // Now make sure that the op that wrote the key is what we expect.
    // This should match the translation of a `pshufd xmm0, xmm0, 0xe` or `psrldq xmm0, 8` instruction.
    const auto KeyShift = GetOpHeader(WrapNode(KeyLastWritten));
    const bool IsVExtr = KeyShift->Op == OP_VEXTR && KeyShift->Size == OpSize::i128Bit;
    if (!IsVExtr) {
      goto no_peephole;
    }

    // Compute shift in bytes, since pshufd and psrldq use different element sizes.
    const auto ShiftInBytes = KeyShift->C<IR::IROp_VExtr>()->Index * IR::OpSizeToSize(KeyShift->ElementSize);
    const bool IsKeyShift = ShiftInBytes == 8 && IsValueLoadRegister(KeyShift->C<IR::IROp_VExtr>()->VectorUpper, RegClass::FPR, 0);
    if (!IsKeyShift) {
      goto no_peephole;
    }

    // Now check that the rest of the OPs match the pattern.
    const Ref FirstZip = GetArgIfOp(XLastWritten, OP_VREV64, 0);
    // A and B are the SHA256H and SHA256H2 output of the first SHA256RNDS2 instruction
    const Ref A = GetArgIfOp(FirstZip, OP_VZIP2, 1);
    const Ref B = GetArgIfOp(FirstZip, OP_VZIP2, 0);
    // Figure out based on A what the source of the first SHA256RNDS2 was.
    const Ref FirstSrc = GetArgIfOp(GetArgIfOp(GetArgIfOp(A, OP_VSHA256H, 0), OP_VREV64, 0), OP_VZIP2, 0);
    // True if X holds the result of the first SHA256RNDS2, and its source register was Y.
    const bool IsFirstResult = FirstSrc && IsValueLoadRegister(WrapNode(FirstSrc), RegClass::FPR, Y);
    if (!IsFirstResult) {
      goto no_peephole;
    }

    // The pattern matched, so we can elide the second SHA256RNDS2,
    // and instead use the result of the first pair of SHA256H and SHA256H2 instructions
    // to populate the final result.
    StoreResult_WithAVXInsert(VectorOpType::SSE, RegClass::FPR, Op, shuffle_efgh(A, B));
    return;
  }

no_peephole:
  Ref Dest = LoadSourceFPR(Op, Op->Dest, Op->Flags);
  Ref Src = LoadSourceFPR(Op, Op->Src[0], Op->Flags);
  // Hardcoded to XMM0
  auto XMM0 = LoadXMMRegister(0);

  auto ABCD = shuffle_abcd(Dest, Src);
  auto EFGH = shuffle_efgh(Dest, Src);

  // x86 uses only the lower 64 bits of the key for two rounds.
  // ARM64 does four rounds, so in order to support the peephole optimization,
  // we use the full key always.
  auto A = _VSha256H(ABCD, EFGH, XMM0);
  auto B = _VSha256H2(EFGH, ABCD, XMM0);
  auto Result = shuffle_abcd(A, B);

  StoreResult_WithAVXInsert(VectorOpType::SSE, RegClass::FPR, Op, Result);
}

void OpDispatchBuilder::AESImcOp(OpcodeArgs, bool IsAVX) {
  if (!CTX->HostFeatures.SupportsAES) {
    UnimplementedOp(Op);
    return;
  }
  Ref Src = LoadSourceFPR(Op, Op->Src[0], Op->Flags);
  Ref Result = _VAESImc(Src);

  if (IsAVX) {
    StoreResultFPR(Op, Result);
  } else {
    StoreResult_WithAVXInsert(VectorOpType::SSE, RegClass::FPR, Op, Result);
  }
}

void OpDispatchBuilder::AESEncOp(OpcodeArgs) {
  if (!CTX->HostFeatures.SupportsAES) {
    UnimplementedOp(Op);
    return;
  }
  Ref Dest = LoadSourceFPR(Op, Op->Dest, Op->Flags);
  Ref Src = LoadSourceFPR(Op, Op->Src[0], Op->Flags);
  Ref Result = _VAESEnc(OpSize::i128Bit, Dest, Src, LoadZeroVector(OpSize::i128Bit));
  StoreResult_WithAVXInsert(VectorOpType::SSE, RegClass::FPR, Op, Result);
}

void OpDispatchBuilder::VAESEncOp(OpcodeArgs) {
  const auto DstSize = OpSizeFromDst(Op);
  const auto Is256Bit = DstSize == OpSize::i256Bit;

  Ref State = LoadSourceFPR(Op, Op->Src[0], Op->Flags);
  Ref Key = LoadSourceFPR(Op, Op->Src[1], Op->Flags);
  Ref ZeroVec = LoadZeroVector(DstSize);

  Ref Result {};
  if (Is256Bit) {
    // TODO: Handle as one operation once vixl supports it.
    auto UpperState = _VDupElement(DstSize, OpSize::i128Bit, State, 1);
    auto UpperKey = _VDupElement(DstSize, OpSize::i128Bit, Key, 1);

    auto Lower = _VAESEnc(OpSize::i128Bit, State, Key, ZeroVec);
    auto Upper = _VAESEnc(OpSize::i128Bit, UpperState, UpperKey, ZeroVec);

    Result = _VInsElement(DstSize, OpSize::i128Bit, 1, 0, Lower, Upper);
  } else {
    Result = _VAESEnc(DstSize, State, Key, ZeroVec);
  }

  StoreResultFPR(Op, Result);
}

void OpDispatchBuilder::AESEncLastOp(OpcodeArgs) {
  if (!CTX->HostFeatures.SupportsAES) {
    UnimplementedOp(Op);
    return;
  }
  Ref Dest = LoadSourceFPR(Op, Op->Dest, Op->Flags);
  Ref Src = LoadSourceFPR(Op, Op->Src[0], Op->Flags);
  Ref Result = _VAESEncLast(OpSize::i128Bit, Dest, Src, LoadZeroVector(OpSize::i128Bit));
  StoreResult_WithAVXInsert(VectorOpType::SSE, RegClass::FPR, Op, Result);
}

void OpDispatchBuilder::VAESEncLastOp(OpcodeArgs) {
  const auto DstSize = OpSizeFromDst(Op);
  const auto Is256Bit = DstSize == OpSize::i256Bit;

  Ref State = LoadSourceFPR(Op, Op->Src[0], Op->Flags);
  Ref Key = LoadSourceFPR(Op, Op->Src[1], Op->Flags);
  Ref ZeroVec = LoadZeroVector(DstSize);

  Ref Result {};
  if (Is256Bit) {
    // TODO: Handle as one operation once vixl supports it.
    auto UpperState = _VDupElement(DstSize, OpSize::i128Bit, State, 1);
    auto UpperKey = _VDupElement(DstSize, OpSize::i128Bit, Key, 1);

    auto Lower = _VAESEncLast(OpSize::i128Bit, State, Key, ZeroVec);
    auto Upper = _VAESEncLast(OpSize::i128Bit, UpperState, UpperKey, ZeroVec);

    Result = _VInsElement(DstSize, OpSize::i128Bit, 1, 0, Lower, Upper);
  } else {
    Result = _VAESEncLast(DstSize, State, Key, ZeroVec);
  }

  StoreResultFPR(Op, Result);
}

void OpDispatchBuilder::AESDecOp(OpcodeArgs) {
  if (!CTX->HostFeatures.SupportsAES) {
    UnimplementedOp(Op);
    return;
  }
  Ref Dest = LoadSourceFPR(Op, Op->Dest, Op->Flags);
  Ref Src = LoadSourceFPR(Op, Op->Src[0], Op->Flags);
  Ref Result = _VAESDec(OpSize::i128Bit, Dest, Src, LoadZeroVector(OpSize::i128Bit));
  StoreResult_WithAVXInsert(VectorOpType::SSE, RegClass::FPR, Op, Result);
}

void OpDispatchBuilder::VAESDecOp(OpcodeArgs) {
  const auto DstSize = OpSizeFromDst(Op);
  const auto Is256Bit = DstSize == OpSize::i256Bit;

  Ref State = LoadSourceFPR(Op, Op->Src[0], Op->Flags);
  Ref Key = LoadSourceFPR(Op, Op->Src[1], Op->Flags);
  Ref ZeroVec = LoadZeroVector(DstSize);

  Ref Result {};
  if (Is256Bit) {
    // TODO: Handle as one operation once vixl supports it.
    auto UpperState = _VDupElement(DstSize, OpSize::i128Bit, State, 1);
    auto UpperKey = _VDupElement(DstSize, OpSize::i128Bit, Key, 1);

    auto Lower = _VAESDec(OpSize::i128Bit, State, Key, ZeroVec);
    auto Upper = _VAESDec(OpSize::i128Bit, UpperState, UpperKey, ZeroVec);

    Result = _VInsElement(DstSize, OpSize::i128Bit, 1, 0, Lower, Upper);
  } else {
    Result = _VAESDec(DstSize, State, Key, ZeroVec);
  }

  StoreResultFPR(Op, Result);
}

void OpDispatchBuilder::AESDecLastOp(OpcodeArgs) {
  if (!CTX->HostFeatures.SupportsAES) {
    UnimplementedOp(Op);
    return;
  }
  Ref Dest = LoadSourceFPR(Op, Op->Dest, Op->Flags);
  Ref Src = LoadSourceFPR(Op, Op->Src[0], Op->Flags);
  Ref Result = _VAESDecLast(OpSize::i128Bit, Dest, Src, LoadZeroVector(OpSize::i128Bit));
  StoreResult_WithAVXInsert(VectorOpType::SSE, RegClass::FPR, Op, Result);
}

void OpDispatchBuilder::VAESDecLastOp(OpcodeArgs) {
  const auto DstSize = OpSizeFromDst(Op);
  const auto Is256Bit = DstSize == OpSize::i256Bit;

  Ref State = LoadSourceFPR(Op, Op->Src[0], Op->Flags);
  Ref Key = LoadSourceFPR(Op, Op->Src[1], Op->Flags);
  Ref ZeroVec = LoadZeroVector(DstSize);

  Ref Result {};
  if (Is256Bit) {
    // TODO: Handle as one operation once vixl supports it.
    auto UpperState = _VDupElement(DstSize, OpSize::i128Bit, State, 1);
    auto UpperKey = _VDupElement(DstSize, OpSize::i128Bit, Key, 1);

    auto Lower = _VAESDecLast(OpSize::i128Bit, State, Key, ZeroVec);
    auto Upper = _VAESDecLast(OpSize::i128Bit, UpperState, UpperKey, ZeroVec);

    Result = _VInsElement(DstSize, OpSize::i128Bit, 1, 0, Lower, Upper);
  } else {
    Result = _VAESDecLast(DstSize, State, Key, ZeroVec);
  }

  StoreResultFPR(Op, Result);
}

Ref OpDispatchBuilder::AESKeyGenAssistImpl(OpcodeArgs) {
  Ref Src = LoadSourceFPR(Op, Op->Src[0], Op->Flags);
  const uint64_t RCON = Op->Src[1].Literal();

  auto KeyGenSwizzle = LoadAndCacheNamedVectorConstant(OpSize::i128Bit, NAMED_VECTOR_AESKEYGENASSIST_SWIZZLE);
  return _VAESKeyGenAssist(Src, KeyGenSwizzle, LoadZeroVector(OpSize::i128Bit), RCON);
}

void OpDispatchBuilder::AESKeyGenAssist(OpcodeArgs, bool IsAVX) {
  if (!CTX->HostFeatures.SupportsAES) {
    UnimplementedOp(Op);
    return;
  }

  Ref Result = AESKeyGenAssistImpl(Op);

  if (IsAVX) {
    StoreResultFPR(Op, Result);
  } else {
    StoreResult_WithAVXInsert(VectorOpType::SSE, RegClass::FPR, Op, Result);
  }
}

void OpDispatchBuilder::PCLMULQDQOp(OpcodeArgs) {
  if (!CTX->HostFeatures.SupportsPMULL_128Bit) {
    UnimplementedOp(Op);
    return;
  }
  Ref Dest = LoadSourceFPR(Op, Op->Dest, Op->Flags);
  Ref Src = LoadSourceFPR(Op, Op->Src[0], Op->Flags);
  const auto Selector = static_cast<uint8_t>(Op->Src[1].Literal());

  auto Result = _PCLMUL(OpSize::i128Bit, Dest, Src, Selector & 0b1'0001);
  StoreResult_WithAVXInsert(VectorOpType::SSE, RegClass::FPR, Op, Result);
}

void OpDispatchBuilder::VPCLMULQDQOp(OpcodeArgs) {
  if (!CTX->HostFeatures.SupportsPMULL_128Bit) {
    UnimplementedOp(Op);
    return;
  }
  const auto DstSize = OpSizeFromDst(Op);

  Ref Src1 = LoadSourceFPR(Op, Op->Src[0], Op->Flags);
  Ref Src2 = LoadSourceFPR(Op, Op->Src[1], Op->Flags);
  const auto Selector = static_cast<uint8_t>(Op->Src[2].Literal());

  Ref Res = _PCLMUL(DstSize, Src1, Src2, Selector & 0b1'0001);
  StoreResultFPR(Op, Res);
}

} // namespace FEXCore::IR
