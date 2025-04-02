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

#include <array>
#include <cstdint>
#include <tuple>
#include <utility>

namespace FEXCore::IR {
class OrderedNode;

#define OpcodeArgs [[maybe_unused]] FEXCore::X86Tables::DecodedOp Op

void OpDispatchBuilder::SHA1NEXTEOp(OpcodeArgs) {
  Ref Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

  Ref RotatedNode {};
  if (CTX->HostFeatures.SupportsSHA) {
    // ARMv8 SHA1 extension provides a `SHA1H` instruction which does a fixed rotate by 30.
    // This only operates on element 0 rather than element 3. We don't have the luxury of rewriting the x86 SHA algorithm to take advantage of this.
    // Move the element to zero, rotate, and then move back (Using duplicates).
    // Saves one instruction versus that path that doesn't support SHA extension.
    auto Duplicated = _VDupElement(OpSize::i128Bit, OpSize::i32Bit, Dest, 3);
    auto Sha1HRotated = _VSha1H(Duplicated);
    RotatedNode = _VDupElement(OpSize::i128Bit, OpSize::i32Bit, Sha1HRotated, 0);
  } else {
    // SHA1 extension missing, manually rotate.
    // Emulate rotate.
    auto ShiftLeft = _VShlI(OpSize::i128Bit, OpSize::i32Bit, Dest, 30);
    RotatedNode = _VUShraI(OpSize::i128Bit, OpSize::i32Bit, ShiftLeft, Dest, 2);
  }
  auto Tmp = _VAdd(OpSize::i128Bit, OpSize::i32Bit, Src, RotatedNode);
  auto Result = _VInsElement(OpSize::i128Bit, OpSize::i32Bit, 3, 3, Src, Tmp);

  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::SHA1MSG1Op(OpcodeArgs) {
  Ref Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

  Ref NewVec = _VExtr(OpSize::i128Bit, OpSize::i64Bit, Dest, Src, 1);

  // [W0, W1, W2, W3] ^ [W2, W3, W4, W5]
  Ref Result = _VXor(OpSize::i128Bit, OpSize::i8Bit, Dest, NewVec);

  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::SHA1MSG2Op(OpcodeArgs) {
  Ref Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

  Ref Result;
  if (CTX->HostFeatures.SupportsSHA) {
    // ARM SHA1 mostly matches x86 semantics, except the input and outputs are both flipped from elements 0,1,2,3 to 3,2,1,0.
    auto Src1 = SHADataShuffle(Dest);
    auto Src2 = SHADataShuffle(Src);

    // The result is swizzled differently than expected
    Result = SHADataShuffle(_VSha1SU1(Src1, Src2));
  } else {
    // Shift the incoming source left by a 32-bit element, inserting Zeros.
    // This could be slightly improved to use a VInsGPR with the zero register.
    const auto ZeroRegister = LoadZeroVector(OpSize::i32Bit);
    auto Src2Shift = _VExtr(OpSize::i128Bit, OpSize::i8Bit, Src, ZeroRegister, 12);
    auto Xor1 = _VXor(OpSize::i128Bit, OpSize::i8Bit, Dest, Src2Shift);

    // Emulate rotate.
    auto ShiftLeftXor1 = _VShlI(OpSize::i128Bit, OpSize::i32Bit, Xor1, 1);
    auto RotatedXor1 = _VUShraI(OpSize::i128Bit, OpSize::i32Bit, ShiftLeftXor1, Xor1, 31);

    // Element0 didn't get XOR'd with anything, so do it now.
    auto ExtractUpper = _VDupElement(OpSize::i128Bit, OpSize::i32Bit, RotatedXor1, 3);
    auto XorLower = _VXor(OpSize::i128Bit, OpSize::i8Bit, Dest, ExtractUpper);

    // Emulate rotate.
    auto ShiftLeftXorLower = _VShlI(OpSize::i128Bit, OpSize::i32Bit, XorLower, 1);
    auto RotatedXorLower = _VUShraI(OpSize::i128Bit, OpSize::i32Bit, ShiftLeftXorLower, XorLower, 31);

    Result = _VInsElement(OpSize::i128Bit, OpSize::i32Bit, 0, 0, RotatedXor1, RotatedXorLower);
  }


  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::SHA1RNDS4Op(OpcodeArgs) {
  using FnType = Ref (*)(OpDispatchBuilder&, Ref, Ref, Ref);

  const auto f0 = [](OpDispatchBuilder& Self, Ref B, Ref C, Ref D) -> Ref { // sha1c?
    return Self._Xor(OpSize::i32Bit, Self._And(OpSize::i32Bit, B, C), Self._Andn(OpSize::i32Bit, D, B));
  };
  const auto f1 = [](OpDispatchBuilder& Self, Ref B, Ref C, Ref D) -> Ref { // sha1p with different key
    return Self._Xor(OpSize::i32Bit, Self._Xor(OpSize::i32Bit, B, C), D);
  };
  const auto f2 = [](OpDispatchBuilder& Self, Ref B, Ref C, Ref D) -> Ref { // sha1m
    return Self.BitwiseAtLeastTwo(B, C, D);
  };
  const auto f3 = [](OpDispatchBuilder& Self, Ref B, Ref C, Ref D) -> Ref { // sha1p
    return Self._Xor(OpSize::i32Bit, Self._Xor(OpSize::i32Bit, B, C), D);
  };

  constexpr std::array<uint32_t, 4> k_array {
    0x5A827999U,
    0x6ED9EBA1U,
    0x8F1BBCDCU,
    0xCA62C1D6U,
  };

  constexpr std::array<FnType, 4> fn_array {
    f0,
    f1,
    f2,
    f3,
  };


  const uint64_t Imm8 = Op->Src[1].Literal() & 0b11;
  Ref Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

  Ref Result {};
  if (CTX->HostFeatures.SupportsSHA) {
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
  } else {
    const FnType Fn = fn_array[Imm8];
    auto K = _Constant(OpSize::i32Bit, k_array[Imm8]);
    auto W0E = _VExtractToGPR(OpSize::i128Bit, OpSize::i32Bit, Src, 3);

    using RoundResult = std::tuple<Ref, Ref, Ref, Ref, Ref>;

    const auto Round0 = [&]() -> RoundResult {
      auto A = _VExtractToGPR(OpSize::i128Bit, OpSize::i32Bit, Dest, 3);
      auto B = _VExtractToGPR(OpSize::i128Bit, OpSize::i32Bit, Dest, 2);
      auto C = _VExtractToGPR(OpSize::i128Bit, OpSize::i32Bit, Dest, 1);
      auto D = _VExtractToGPR(OpSize::i128Bit, OpSize::i32Bit, Dest, 0);

      auto A1 =
        _Add(OpSize::i32Bit,
             _Add(OpSize::i32Bit, _Add(OpSize::i32Bit, Fn(*this, B, C, D), _Ror(OpSize::i32Bit, A, _Constant(OpSize::i32Bit, 27))), W0E), K);
      auto B1 = A;
      auto C1 = _Ror(OpSize::i32Bit, B, _Constant(OpSize::i32Bit, 2));
      auto D1 = C;
      auto E1 = D;

      return {A1, B1, C1, D1, E1};
    };
    const auto Round1To3 = [&](Ref A, Ref B, Ref C, Ref D, Ref E, Ref Src, unsigned W_idx) -> RoundResult {
      // Kill W and E at the beginning
      auto W = _VExtractToGPR(OpSize::i128Bit, OpSize::i32Bit, Src, W_idx);
      auto Q = _Add(OpSize::i32Bit, W, E);

      auto ANext =
        _Add(OpSize::i32Bit,
             _Add(OpSize::i32Bit, _Add(OpSize::i32Bit, Fn(*this, B, C, D), _Ror(OpSize::i32Bit, A, _Constant(OpSize::i32Bit, 27))), Q), K);
      auto BNext = A;
      auto CNext = _Ror(OpSize::i32Bit, B, _Constant(OpSize::i32Bit, 2));
      auto DNext = C;
      auto ENext = D;

      return {ANext, BNext, CNext, DNext, ENext};
    };

    auto [A1, B1, C1, D1, E1] = Round0();
    auto [A2, B2, C2, D2, E2] = Round1To3(A1, B1, C1, D1, E1, Src, 2);
    auto [A3, B3, C3, D3, E3] = Round1To3(A2, B2, C2, D2, E2, Src, 1);
    auto Final = Round1To3(A3, B3, C3, D3, E3, Src, 0);

    auto Dest3 = _VInsGPR(OpSize::i128Bit, OpSize::i32Bit, 3, Dest, std::get<0>(Final));
    auto Dest2 = _VInsGPR(OpSize::i128Bit, OpSize::i32Bit, 2, Dest3, std::get<1>(Final));
    auto Dest1 = _VInsGPR(OpSize::i128Bit, OpSize::i32Bit, 1, Dest2, std::get<2>(Final));
    Result = _VInsGPR(OpSize::i128Bit, OpSize::i32Bit, 0, Dest1, std::get<3>(Final));
  }

  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::SHA256MSG1Op(OpcodeArgs) {
  Ref Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

  Ref Result {};

  if (CTX->HostFeatures.SupportsSHA) {
    Result = _VSha256U0(Dest, Src);
  } else {
    const auto Sigma0 = [this](Ref W) -> Ref {
      return _Xor(
        OpSize::i32Bit,
        _Xor(OpSize::i32Bit, _Ror(OpSize::i32Bit, W, _Constant(OpSize::i32Bit, 7)), _Ror(OpSize::i32Bit, W, _Constant(OpSize::i32Bit, 18))),
        _Lshr(OpSize::i32Bit, W, _Constant(OpSize::i32Bit, 3)));
    };

    auto W4 = _VExtractToGPR(OpSize::i128Bit, OpSize::i32Bit, Src, 0);
    auto W3 = _VExtractToGPR(OpSize::i128Bit, OpSize::i32Bit, Dest, 3);
    auto W2 = _VExtractToGPR(OpSize::i128Bit, OpSize::i32Bit, Dest, 2);
    auto W1 = _VExtractToGPR(OpSize::i128Bit, OpSize::i32Bit, Dest, 1);
    auto W0 = _VExtractToGPR(OpSize::i128Bit, OpSize::i32Bit, Dest, 0);

    auto Sig3 = _Add(OpSize::i32Bit, W3, Sigma0(W4));
    auto Sig2 = _Add(OpSize::i32Bit, W2, Sigma0(W3));
    auto Sig1 = _Add(OpSize::i32Bit, W1, Sigma0(W2));
    auto Sig0 = _Add(OpSize::i32Bit, W0, Sigma0(W1));

    auto D3 = _VInsGPR(OpSize::i128Bit, OpSize::i32Bit, 3, Dest, Sig3);
    auto D2 = _VInsGPR(OpSize::i128Bit, OpSize::i32Bit, 2, D3, Sig2);
    auto D1 = _VInsGPR(OpSize::i128Bit, OpSize::i32Bit, 1, D2, Sig1);
    Result = _VInsGPR(OpSize::i128Bit, OpSize::i32Bit, 0, D1, Sig0);
  }

  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::SHA256MSG2Op(OpcodeArgs) {
  const auto Sigma1 = [this](Ref W) -> Ref {
    return _Xor(
      OpSize::i32Bit,
      _Xor(OpSize::i32Bit, _Ror(OpSize::i32Bit, W, _Constant(OpSize::i32Bit, 17)), _Ror(OpSize::i32Bit, W, _Constant(OpSize::i32Bit, 19))),
      _Lshr(OpSize::i32Bit, W, _Constant(OpSize::i32Bit, 10)));
  };

  Ref Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

  Ref Result;
  if (CTX->HostFeatures.SupportsSHA) {
    auto Src1 = _VExtr(OpSize::i128Bit, OpSize::i32Bit, Dest, Dest, 3);
    auto DupDst = _VDupElement(OpSize::i128Bit, OpSize::i32Bit, Dest, 3);
    auto Src2 = _VZip2(OpSize::i128Bit, OpSize::i64Bit, DupDst, Src);

    Result = _VSha256U1(Src1, Src2);
  } else {
    auto W14 = _VExtractToGPR(OpSize::i128Bit, OpSize::i32Bit, Src, 2);
    auto W15 = _VExtractToGPR(OpSize::i128Bit, OpSize::i32Bit, Src, 3);
    auto W16 = _Add(OpSize::i32Bit, _VExtractToGPR(OpSize::i128Bit, OpSize::i32Bit, Dest, 0), Sigma1(W14));
    auto W17 = _Add(OpSize::i32Bit, _VExtractToGPR(OpSize::i128Bit, OpSize::i32Bit, Dest, 1), Sigma1(W15));
    auto W18 = _Add(OpSize::i32Bit, _VExtractToGPR(OpSize::i128Bit, OpSize::i32Bit, Dest, 2), Sigma1(W16));
    auto W19 = _Add(OpSize::i32Bit, _VExtractToGPR(OpSize::i128Bit, OpSize::i32Bit, Dest, 3), Sigma1(W17));

    auto D3 = _VInsGPR(OpSize::i128Bit, OpSize::i32Bit, 3, Dest, W19);
    auto D2 = _VInsGPR(OpSize::i128Bit, OpSize::i32Bit, 2, D3, W18);
    auto D1 = _VInsGPR(OpSize::i128Bit, OpSize::i32Bit, 1, D2, W17);
    Result = _VInsGPR(OpSize::i128Bit, OpSize::i32Bit, 0, D1, W16);
  }

  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

Ref OpDispatchBuilder::BitwiseAtLeastTwo(Ref A, Ref B, Ref C) {
  // Returns whether at least 2/3 of A/B/C is true.
  // Expressed as (A & (B | C)) | (B & C)
  //
  // Equivalent to expression in SHA calculations: (A & B) ^ (A & C) ^ (B & C)
  auto And = _And(OpSize::i32Bit, B, C);
  auto Or = _Or(OpSize::i32Bit, B, C);
  return _Or(OpSize::i32Bit, _And(OpSize::i32Bit, A, Or), And);
}

void OpDispatchBuilder::SHA256RNDS2Op(OpcodeArgs) {
  Ref Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  // Hardcoded to XMM0
  auto XMM0 = LoadXMMRegister(0);

  Ref Result;
  if (CTX->HostFeatures.SupportsSHA) {
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
    Result = shuffle_abcd(A, B);
  } else {
    const auto Ch = [this](Ref E, Ref F, Ref G) -> Ref {
      return _Xor(OpSize::i32Bit, _And(OpSize::i32Bit, E, F), _Andn(OpSize::i32Bit, G, E));
    };
    const auto Sigma0 = [this](Ref A) -> Ref {
      return _XorShift(OpSize::i32Bit, _XorShift(OpSize::i32Bit, _Ror(OpSize::i32Bit, A, _Constant(OpSize::i32Bit, 2)), A, ShiftType::ROR, 13),
                       A, ShiftType::ROR, 22);
    };
    const auto Sigma1 = [this](Ref E) -> Ref {
      return _XorShift(OpSize::i32Bit, _XorShift(OpSize::i32Bit, _Ror(OpSize::i32Bit, E, _Constant(OpSize::i32Bit, 6)), E, ShiftType::ROR, 11),
                       E, ShiftType::ROR, 25);
    };

    auto E0 = _VExtractToGPR(OpSize::i128Bit, OpSize::i32Bit, Src, 1);
    auto F0 = _VExtractToGPR(OpSize::i128Bit, OpSize::i32Bit, Src, 0);
    auto G0 = _VExtractToGPR(OpSize::i128Bit, OpSize::i32Bit, Dest, 1);
    Ref Q0 = _Add(OpSize::i32Bit, Ch(E0, F0, G0), Sigma1(E0));

    auto WK0 = _VExtractToGPR(OpSize::i128Bit, OpSize::i32Bit, XMM0, 0);
    Q0 = _Add(OpSize::i32Bit, Q0, WK0);

    auto H0 = _VExtractToGPR(OpSize::i128Bit, OpSize::i32Bit, Dest, 0);
    Q0 = _Add(OpSize::i32Bit, Q0, H0);

    auto A0 = _VExtractToGPR(OpSize::i128Bit, OpSize::i32Bit, Src, 3);
    auto B0 = _VExtractToGPR(OpSize::i128Bit, OpSize::i32Bit, Src, 2);
    auto C0 = _VExtractToGPR(OpSize::i128Bit, OpSize::i32Bit, Dest, 3);
    auto A1 = _Add(OpSize::i32Bit, _Add(OpSize::i32Bit, Q0, BitwiseAtLeastTwo(A0, B0, C0)), Sigma0(A0));

    auto D0 = _VExtractToGPR(OpSize::i128Bit, OpSize::i32Bit, Dest, 2);
    auto E1 = _Add(OpSize::i32Bit, Q0, D0);

    Ref Q1 = _Add(OpSize::i32Bit, Ch(E1, E0, F0), Sigma1(E1));

    auto WK1 = _VExtractToGPR(OpSize::i128Bit, OpSize::i32Bit, XMM0, 1);
    Q1 = _Add(OpSize::i32Bit, Q1, WK1);

    // Rematerialize G0. Costs a move but saves spilling, coming out ahead.
    G0 = _VExtractToGPR(OpSize::i128Bit, OpSize::i32Bit, Dest, 1);
    Q1 = _Add(OpSize::i32Bit, Q1, G0);

    auto A2 = _Add(OpSize::i32Bit, _Add(OpSize::i32Bit, Q1, BitwiseAtLeastTwo(A1, A0, B0)), Sigma0(A1));

    // Rematerialize C0. As with G0.
    C0 = _VExtractToGPR(OpSize::i128Bit, OpSize::i32Bit, Dest, 3);
    auto E2 = _Add(OpSize::i32Bit, Q1, C0);

    auto Res3 = _VInsGPR(OpSize::i128Bit, OpSize::i32Bit, 3, Dest, A2);
    auto Res2 = _VInsGPR(OpSize::i128Bit, OpSize::i32Bit, 2, Res3, A1);
    auto Res1 = _VInsGPR(OpSize::i128Bit, OpSize::i32Bit, 1, Res2, E2);
    Result = _VInsGPR(OpSize::i128Bit, OpSize::i32Bit, 0, Res1, E1);
  }

  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::AESImcOp(OpcodeArgs) {
  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Result = _VAESImc(Src);
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::AESEncOp(OpcodeArgs) {
  Ref Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Result = _VAESEnc(OpSize::i128Bit, Dest, Src, LoadZeroVector(OpSize::i128Bit));
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::VAESEncOp(OpcodeArgs) {
  const auto DstSize = OpSizeFromDst(Op);
  [[maybe_unused]] const auto Is128Bit = DstSize == OpSize::i128Bit;

  // TODO: Handle 256-bit VAESENC.
  LOGMAN_THROW_A_FMT(Is128Bit, "256-bit VAESENC unimplemented");

  Ref State = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Key = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);
  Ref Result = _VAESEnc(DstSize, State, Key, LoadZeroVector(DstSize));

  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::AESEncLastOp(OpcodeArgs) {
  Ref Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Result = _VAESEncLast(OpSize::i128Bit, Dest, Src, LoadZeroVector(OpSize::i128Bit));
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::VAESEncLastOp(OpcodeArgs) {
  const auto DstSize = OpSizeFromDst(Op);
  [[maybe_unused]] const auto Is128Bit = DstSize == OpSize::i128Bit;

  // TODO: Handle 256-bit VAESENCLAST.
  LOGMAN_THROW_A_FMT(Is128Bit, "256-bit VAESENCLAST unimplemented");

  Ref State = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Key = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);
  Ref Result = _VAESEncLast(DstSize, State, Key, LoadZeroVector(DstSize));

  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::AESDecOp(OpcodeArgs) {
  Ref Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Result = _VAESDec(OpSize::i128Bit, Dest, Src, LoadZeroVector(OpSize::i128Bit));
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::VAESDecOp(OpcodeArgs) {
  const auto DstSize = OpSizeFromDst(Op);
  [[maybe_unused]] const auto Is128Bit = DstSize == OpSize::i128Bit;

  // TODO: Handle 256-bit VAESDEC.
  LOGMAN_THROW_A_FMT(Is128Bit, "256-bit VAESDEC unimplemented");

  Ref State = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Key = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);
  Ref Result = _VAESDec(DstSize, State, Key, LoadZeroVector(DstSize));

  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::AESDecLastOp(OpcodeArgs) {
  Ref Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Result = _VAESDecLast(OpSize::i128Bit, Dest, Src, LoadZeroVector(OpSize::i128Bit));
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::VAESDecLastOp(OpcodeArgs) {
  const auto DstSize = OpSizeFromDst(Op);
  [[maybe_unused]] const auto Is128Bit = DstSize == OpSize::i128Bit;

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
