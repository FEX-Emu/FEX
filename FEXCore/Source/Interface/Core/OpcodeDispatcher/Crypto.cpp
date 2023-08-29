/*
$info$
tags: frontend|x86-to-ir, opcodes|dispatcher-implementations
desc: Handles x86/64 Crypto instructions to IR
$end_info$
*/

#include "Interface/Core/X86Tables/X86Tables.h"

#include <FEXCore/IR/IREmitter.h>
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
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  auto Tmp = _Ror(OpSize::i32Bit, _VExtractToGPR(16, 4, Dest, 3), _Constant(32, 2));
  auto Top = _Add(_VExtractToGPR(16, 4, Src, 3), Tmp);
  auto Result = _VInsGPR(16, 4, 3, Src, Top);

  StoreResult(FPRClass, Op, Result, -1);
}

void OpDispatchBuilder::SHA1MSG1Op(OpcodeArgs) {
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  OrderedNode *NewVec = _VExtr(16, 8, Dest, Src, 1);

  // [W0, W1, W2, W3] ^ [W2, W3, W4, W5]
  OrderedNode *Result = _VXor(16, 1, Dest, NewVec);

  StoreResult(FPRClass, Op, Result, -1);
}

void OpDispatchBuilder::SHA1MSG2Op(OpcodeArgs) {
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  // ROR by 31 is equivalent to a ROL by 1
  auto ThirtyOne = _Constant(32, 31);

  auto W13 = _VExtractToGPR(16, 4, Src, 2);
  auto W14 = _VExtractToGPR(16, 4, Src, 1);
  auto W15 = _VExtractToGPR(16, 4, Src, 0);
  auto W16 = _Ror(OpSize::i32Bit, _Xor(OpSize::i32Bit, _VExtractToGPR(16, 4, Dest, 3), W13), ThirtyOne);
  auto W17 = _Ror(OpSize::i32Bit, _Xor(OpSize::i32Bit, _VExtractToGPR(16, 4, Dest, 2), W14), ThirtyOne);
  auto W18 = _Ror(OpSize::i32Bit, _Xor(OpSize::i32Bit, _VExtractToGPR(16, 4, Dest, 1), W15), ThirtyOne);
  auto W19 = _Ror(OpSize::i32Bit, _Xor(OpSize::i32Bit, _VExtractToGPR(16, 4, Dest, 0), W16), ThirtyOne);

  auto D3 = _VInsGPR(16, 4, 3, Dest, W16);
  auto D2 = _VInsGPR(16, 4, 2, D3, W17);
  auto D1 = _VInsGPR(16, 4, 1, D2, W18);
  auto D0 = _VInsGPR(16, 4, 0, D1, W19);

  StoreResult(FPRClass, Op, D0, -1);
}

void OpDispatchBuilder::SHA1RNDS4Op(OpcodeArgs) {
  LOGMAN_THROW_A_FMT(Op->Src[1].IsLiteral(),
                     "Src1 needs to be literal here to indicate function and constants");

  using FnType = OrderedNode* (*)(OpDispatchBuilder&, OrderedNode*, OrderedNode*, OrderedNode*);

  const auto f0 = [](OpDispatchBuilder &Self, OrderedNode *B, OrderedNode *C, OrderedNode *D) -> OrderedNode* {
    return Self._Xor(OpSize::i32Bit, Self._And(B, C), Self._Andn(OpSize::i32Bit, D, B));
  };
  const auto f1 = [](OpDispatchBuilder &Self, OrderedNode *B, OrderedNode *C, OrderedNode *D) -> OrderedNode* {
    return Self._Xor(OpSize::i32Bit, Self._Xor(OpSize::i32Bit, B, C), D);
  };
  const auto f2 = [](OpDispatchBuilder &Self, OrderedNode *B, OrderedNode *C, OrderedNode *D) -> OrderedNode* {
    return Self._Xor(OpSize::i32Bit, Self._Xor(OpSize::i32Bit, Self._And(B, C), Self._And(B, D)), Self._And(C, D));
  };
  const auto f3 = [](OpDispatchBuilder &Self, OrderedNode *B, OrderedNode *C, OrderedNode *D) -> OrderedNode* {
    return Self._Xor(OpSize::i32Bit, Self._Xor(OpSize::i32Bit, B, C), D);
  };

  constexpr std::array<uint32_t, 4> k_array{
    0x5A827999U,
    0x6ED9EBA1U,
    0x8F1BBCDCU,
    0xCA62C1D6U,
  };

  constexpr std::array<FnType, 4> fn_array{
    f0, f1, f2, f3,
  };

  const uint64_t Imm8 = Op->Src[1].Data.Literal.Value & 0b11;
  const FnType Fn = fn_array[Imm8];
  auto K = _Constant(32, k_array[Imm8]);

  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  auto W0E = _VExtractToGPR(16, 4, Src, 3);
  auto W1  = _VExtractToGPR(16, 4, Src, 2);
  auto W2  = _VExtractToGPR(16, 4, Src, 1);
  auto W3  = _VExtractToGPR(16, 4, Src, 0);

  using RoundResult = std::tuple<OrderedNode*, OrderedNode*, OrderedNode*, OrderedNode*, OrderedNode*>;

  const auto Round0 = [&]() -> RoundResult {
    auto A = _VExtractToGPR(16, 4, Dest, 3);
    auto B = _VExtractToGPR(16, 4, Dest, 2);
    auto C = _VExtractToGPR(16, 4, Dest, 1);
    auto D = _VExtractToGPR(16, 4, Dest, 0);

    auto A1 = _Add(_Add(_Add(Fn(*this, B, C, D), _Ror(OpSize::i32Bit, A, _Constant(32, 27))), W0E), K);
    auto B1 = A;
    auto C1 = _Ror(OpSize::i32Bit, B, _Constant(32, 2));
    auto D1 = C;
    auto E1 = D;

    return {A1, B1, C1, D1, E1};
  };
  const auto Round1To3 = [&](OrderedNode *A, OrderedNode *B, OrderedNode *C,
                             OrderedNode *D, OrderedNode *E, OrderedNode *W) -> RoundResult {
    auto ANext = _Add(_Add(_Add(_Add(Fn(*this, B, C, D), _Ror(OpSize::i32Bit, A, _Constant(32, 27))), W), E), K);
    auto BNext = A;
    auto CNext = _Ror(OpSize::i32Bit, B, _Constant(32, 2));
    auto DNext = C;
    auto ENext = D;

    return {ANext, BNext, CNext, DNext, ENext};
  };

  auto [A1, B1, C1, D1, E1] = Round0();
  auto [A2, B2, C2, D2, E2] = Round1To3(A1, B1, C1, D1, E1, W1);
  auto [A3, B3, C3, D3, E3] = Round1To3(A2, B2, C2, D2, E2, W2);
  auto Final                = Round1To3(A3, B3, C3, D3, E3, W3);

  auto Dest3 = _VInsGPR(16, 4, 3, Dest,  std::get<0>(Final));
  auto Dest2 = _VInsGPR(16, 4, 2, Dest3, std::get<1>(Final));
  auto Dest1 = _VInsGPR(16, 4, 1, Dest2, std::get<2>(Final));
  auto Dest0 = _VInsGPR(16, 4, 0, Dest1, std::get<3>(Final));

  StoreResult(FPRClass, Op, Dest0, -1);
}

void OpDispatchBuilder::SHA256MSG1Op(OpcodeArgs) {
  const auto Sigma0 = [this](OrderedNode* W) -> OrderedNode* {
    return _Xor(OpSize::i32Bit, _Xor(OpSize::i32Bit, _Ror(OpSize::i32Bit, W, _Constant(32, 7)), _Ror(OpSize::i32Bit, W, _Constant(32, 18))), _Lshr(OpSize::i32Bit, W, _Constant(32, 3)));
  };

  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  auto W4 = _VExtractToGPR(16, 4, Src, 0);
  auto W3 = _VExtractToGPR(16, 4, Dest, 3);
  auto W2 = _VExtractToGPR(16, 4, Dest, 2);
  auto W1 = _VExtractToGPR(16, 4, Dest, 1);
  auto W0 = _VExtractToGPR(16, 4, Dest, 0);

  auto Sig3 = _Add(W3, Sigma0(W4));
  auto Sig2 = _Add(W2, Sigma0(W3));
  auto Sig1 = _Add(W1, Sigma0(W2));
  auto Sig0 = _Add(W0, Sigma0(W1));

  auto D3 = _VInsGPR(16, 4, 3, Dest, Sig3);
  auto D2 = _VInsGPR(16, 4, 2, D3, Sig2);
  auto D1 = _VInsGPR(16, 4, 1, D2, Sig1);
  auto D0 = _VInsGPR(16, 4, 0, D1, Sig0);

  StoreResult(FPRClass, Op, D0, -1);
}

void OpDispatchBuilder::SHA256MSG2Op(OpcodeArgs) {
  const auto Sigma1 = [this](OrderedNode* W) -> OrderedNode* {
    return _Xor(OpSize::i32Bit, _Xor(OpSize::i32Bit, _Ror(OpSize::i32Bit, W, _Constant(32, 17)), _Ror(OpSize::i32Bit, W, _Constant(32, 19))), _Lshr(OpSize::i32Bit, W, _Constant(32, 10)));
  };

  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  auto W14 = _VExtractToGPR(16, 4, Src, 2);
  auto W15 = _VExtractToGPR(16, 4, Src, 3);
  auto W16 = _Add(_VExtractToGPR(16, 4, Dest, 0), Sigma1(W14));
  auto W17 = _Add(_VExtractToGPR(16, 4, Dest, 1), Sigma1(W15));
  auto W18 = _Add(_VExtractToGPR(16, 4, Dest, 2), Sigma1(W16));
  auto W19 = _Add(_VExtractToGPR(16, 4, Dest, 3), Sigma1(W17));

  auto D3 = _VInsGPR(16, 4, 3, Dest, W19);
  auto D2 = _VInsGPR(16, 4, 2, D3, W18);
  auto D1 = _VInsGPR(16, 4, 1, D2, W17);
  auto D0 = _VInsGPR(16, 4, 0, D1, W16);

  StoreResult(FPRClass, Op, D0, -1);
}

void OpDispatchBuilder::SHA256RNDS2Op(OpcodeArgs) {
  const auto Ch = [this](OrderedNode *E, OrderedNode *F, OrderedNode *G) -> OrderedNode* {
    return _Xor(OpSize::i32Bit, _And(E, F), _Andn(OpSize::i32Bit, G, E));
  };
  const auto Major = [this](OrderedNode *A, OrderedNode *B, OrderedNode *C) -> OrderedNode* {
    return _Xor(OpSize::i32Bit, _Xor(OpSize::i32Bit, _And(A, B), _And(A, C)), _And(B, C));
  };
  const auto Sigma0 = [this](OrderedNode *A) -> OrderedNode* {
    return _Xor(OpSize::i32Bit, _Xor(OpSize::i32Bit, _Ror(OpSize::i32Bit, A, _Constant(32, 2)), _Ror(OpSize::i32Bit, A, _Constant(32, 13))), _Ror(OpSize::i32Bit, A, _Constant(32, 22)));
  };
  const auto Sigma1 = [this](OrderedNode *E) -> OrderedNode* {
    return _Xor(OpSize::i32Bit, _Xor(OpSize::i32Bit, _Ror(OpSize::i32Bit, E, _Constant(32, 6)), _Ror(OpSize::i32Bit, E, _Constant(32, 11))), _Ror(OpSize::i32Bit, E, _Constant(32, 25)));
  };

  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  // Hardcoded to XMM0
  auto XMM0 = LoadXMMRegister(0);

  auto A0 = _VExtractToGPR(16, 4, Src, 3);
  auto B0 = _VExtractToGPR(16, 4, Src, 2);
  auto C0 = _VExtractToGPR(16, 4, Dest, 3);
  auto D0 = _VExtractToGPR(16, 4, Dest, 2);
  auto E0 = _VExtractToGPR(16, 4, Src, 1);
  auto F0 = _VExtractToGPR(16, 4, Src, 0);
  auto G0 = _VExtractToGPR(16, 4, Dest, 1);
  auto H0 = _VExtractToGPR(16, 4, Dest, 0);
  auto WK0 = _VExtractToGPR(16, 4, XMM0, 0);
  auto WK1 = _VExtractToGPR(16, 4, XMM0, 1);

  using RoundResult = std::tuple<OrderedNode*, OrderedNode*, OrderedNode*, OrderedNode*,
                                 OrderedNode*, OrderedNode*, OrderedNode*, OrderedNode*>;
  const auto Round = [&](OrderedNode *A, OrderedNode *B, OrderedNode *C, OrderedNode *D,
                         OrderedNode *E, OrderedNode *F, OrderedNode *G, OrderedNode *H,
                         OrderedNode* WK) -> RoundResult {
    auto ANext = _Add(_Add(_Add(_Add(_Add(Ch(E, F, G), Sigma1(E)), WK), H), Major(A, B, C)), Sigma0(A));
    auto BNext = A;
    auto CNext = B;
    auto DNext = C;
    auto ENext = _Add(_Add(_Add(_Add(Ch(E, F, G), Sigma1(E)), WK), H), D);
    auto FNext = E;
    auto GNext = F;
    auto HNext = G;

    return {ANext, BNext, CNext, DNext, ENext, FNext, GNext, HNext};
  };


  auto [A1, B1, C1, D1, E1, F1, G1, H1] = Round(A0, B0, C0, D0, E0, F0, G0, H0, WK0);
  auto Final                            = Round(A1, B1, C1, D1, E1, F1, G1, H1, WK1);

  auto Res3 = _VInsGPR(16, 4, 3, Dest, std::get<0>(Final));
  auto Res2 = _VInsGPR(16, 4, 2, Res3, std::get<1>(Final));
  auto Res1 = _VInsGPR(16, 4, 1, Res2, std::get<4>(Final));
  auto Res0 = _VInsGPR(16, 4, 0, Res1, std::get<5>(Final));

  StoreResult(FPRClass, Op, Res0, -1);
}

void OpDispatchBuilder::AESImcOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Result = _VAESImc(Src);
  StoreResult(FPRClass, Op, Result, -1);
}

void OpDispatchBuilder::AESEncOp(OpcodeArgs) {
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Result = _VAESEnc(16, Dest, Src);
  StoreResult(FPRClass, Op, Result, -1);
}

void OpDispatchBuilder::VAESEncOp(OpcodeArgs) {
  const auto DstSize = GetDstSize(Op);
  [[maybe_unused]] const auto Is128Bit = DstSize == Core::CPUState::XMM_SSE_REG_SIZE;

  // TODO: Handle 256-bit VAESENC.
  LOGMAN_THROW_A_FMT(Is128Bit, "256-bit VAESENC unimplemented");

  OrderedNode *State = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Key = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags, -1);
  OrderedNode *Result = _VAESEnc(DstSize, State, Key);

  StoreResult(FPRClass, Op, Result, -1);
}

void OpDispatchBuilder::AESEncLastOp(OpcodeArgs) {
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Result = _VAESEncLast(16, Dest, Src);
  StoreResult(FPRClass, Op, Result, -1);
}

void OpDispatchBuilder::VAESEncLastOp(OpcodeArgs) {
  const auto DstSize = GetDstSize(Op);
  [[maybe_unused]] const auto Is128Bit = DstSize == Core::CPUState::XMM_SSE_REG_SIZE;

  // TODO: Handle 256-bit VAESENCLAST.
  LOGMAN_THROW_A_FMT(Is128Bit, "256-bit VAESENCLAST unimplemented");

  OrderedNode *State = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Key = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags, -1);
  OrderedNode *Result = _VAESEncLast(DstSize, State, Key);

  StoreResult(FPRClass, Op, Result, -1);
}

void OpDispatchBuilder::AESDecOp(OpcodeArgs) {
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Result = _VAESDec(16, Dest, Src);
  StoreResult(FPRClass, Op, Result, -1);
}

void OpDispatchBuilder::VAESDecOp(OpcodeArgs) {
  const auto DstSize = GetDstSize(Op);
  [[maybe_unused]] const auto Is128Bit = DstSize == Core::CPUState::XMM_SSE_REG_SIZE;

  // TODO: Handle 256-bit VAESDEC.
  LOGMAN_THROW_A_FMT(Is128Bit, "256-bit VAESDEC unimplemented");

  OrderedNode *State = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Key = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags, -1);
  OrderedNode *Result = _VAESDec(DstSize, State, Key);

  StoreResult(FPRClass, Op, Result, -1);
}

void OpDispatchBuilder::AESDecLastOp(OpcodeArgs) {
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Result = _VAESDecLast(16, Dest, Src);
  StoreResult(FPRClass, Op, Result, -1);
}

void OpDispatchBuilder::VAESDecLastOp(OpcodeArgs) {
  const auto DstSize = GetDstSize(Op);
  [[maybe_unused]] const auto Is128Bit = DstSize == Core::CPUState::XMM_SSE_REG_SIZE;

  // TODO: Handle 256-bit VAESDECLAST.
  LOGMAN_THROW_A_FMT(Is128Bit, "256-bit VAESDECLAST unimplemented");

  OrderedNode *State = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Key = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags, -1);
  OrderedNode *Result = _VAESDecLast(DstSize, State, Key);

  StoreResult(FPRClass, Op, Result, -1);
}

OrderedNode* OpDispatchBuilder::AESKeyGenAssistImpl(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  LOGMAN_THROW_A_FMT(Op->Src[1].IsLiteral(), "Src1 needs to be literal here");
  const uint64_t RCON = Op->Src[1].Data.Literal.Value;

  return _VAESKeyGenAssist(Src, RCON);
}

void OpDispatchBuilder::AESKeyGenAssist(OpcodeArgs) {
  OrderedNode *Result = AESKeyGenAssistImpl(Op);
  StoreResult(FPRClass, Op, Result, -1);
}

void OpDispatchBuilder::PCLMULQDQOp(OpcodeArgs) {
  LOGMAN_THROW_A_FMT(Op->Src[1].IsLiteral(), "Selector needs to be literal here");

  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  const auto Selector = static_cast<uint8_t>(Op->Src[1].Data.Literal.Value);

  auto Res = _PCLMUL(16, Dest, Src, Selector);
  StoreResult(FPRClass, Op, Res, -1);
}

void OpDispatchBuilder::VPCLMULQDQOp(OpcodeArgs) {
  LOGMAN_THROW_A_FMT(Op->Src[2].IsLiteral(), "Selector needs to be literal here");

  const auto DstSize = GetDstSize(Op);

  OrderedNode *Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags, -1);
  const auto Selector = static_cast<uint8_t>(Op->Src[2].Data.Literal.Value);

  OrderedNode *Res = _PCLMUL(DstSize, Src1, Src2, Selector);
  StoreResult(FPRClass, Op, Res, -1);
}

}
