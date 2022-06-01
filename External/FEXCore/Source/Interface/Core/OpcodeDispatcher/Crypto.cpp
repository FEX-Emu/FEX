/*
$info$
tags: frontend|x86-to-ir, opcodes|dispatcher-implementations
desc: Handles x86/64 Crypto instructions to IR
$end_info$
*/

#include <FEXCore/Debug/X86Tables.h>
#include <FEXCore/IR/IREmitter.h>
#include <FEXCore/Utils/LogManager.h>
#include "Interface/Core/OpcodeDispatcher.h"

#include <stdint.h>

namespace FEXCore::IR {
class OrderedNode;

#define OpcodeArgs [[maybe_unused]] FEXCore::X86Tables::DecodedOp Op

void OpDispatchBuilder::SHA1NEXTEOp(OpcodeArgs) {
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  auto Tmp = _Ror(_VExtractToGPR(16, 4, Dest, 3), _Constant(32, 2));
  auto Top = _Add(_VExtractToGPR(16, 4, Src, 3), Tmp);
  auto Result = _VInsGPR(16, 4, 3, Src, Top);

  StoreResult(FPRClass, Op, Result, -1);
}

void OpDispatchBuilder::SHA1MSG1Op(OpcodeArgs) {
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  auto W0 = _VExtractToGPR(16, 4, Dest, 3);
  auto W1 = _VExtractToGPR(16, 4, Dest, 2);
  auto W2 = _VExtractToGPR(16, 4, Dest, 1);
  auto W3 = _VExtractToGPR(16, 4, Dest, 0);
  auto W4 = _VExtractToGPR(16, 4, Src, 3);
  auto W5 = _VExtractToGPR(16, 4, Src, 2);

  auto D3 = _VInsGPR(16, 4, 3, Dest, _Xor(W2, W0));
  auto D2 = _VInsGPR(16, 4, 2, D3,   _Xor(W3, W1));
  auto D1 = _VInsGPR(16, 4, 1, D2,   _Xor(W4, W2));
  auto D0 = _VInsGPR(16, 4, 0, D1,   _Xor(W5, W3));

  StoreResult(FPRClass, Op, D0, -1);
}

void OpDispatchBuilder::SHA1MSG2Op(OpcodeArgs) {
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  // ROR by 31 is equivalent to a ROL by 1
  auto ThirtyOne = _Constant(32, 31);

  auto W13 = _VExtractToGPR(16, 4, Src, 2);
  auto W14 = _VExtractToGPR(16, 4, Src, 1);
  auto W15 = _VExtractToGPR(16, 4, Src, 0);
  auto W16 = _Ror(_Xor(_VExtractToGPR(16, 4, Dest, 3), W13), ThirtyOne);
  auto W17 = _Ror(_Xor(_VExtractToGPR(16, 4, Dest, 2), W14), ThirtyOne);
  auto W18 = _Ror(_Xor(_VExtractToGPR(16, 4, Dest, 1), W15), ThirtyOne);
  auto W19 = _Ror(_Xor(_VExtractToGPR(16, 4, Dest, 0), W16), ThirtyOne);

  auto D3 = _VInsGPR(16, 4, 3, Dest, W16);
  auto D2 = _VInsGPR(16, 4, 2, D3, W17);
  auto D1 = _VInsGPR(16, 4, 1, D2, W18);
  auto D0 = _VInsGPR(16, 4, 0, D1, W19);

  StoreResult(FPRClass, Op, D0, -1);
}

void OpDispatchBuilder::AESImcOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  auto Res = _VAESImc(Src);
  StoreResult(FPRClass, Op, Res, -1);
}

void OpDispatchBuilder::AESEncOp(OpcodeArgs) {
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  auto Res = _VAESEnc(Dest, Src);
  StoreResult(FPRClass, Op, Res, -1);
}

void OpDispatchBuilder::AESEncLastOp(OpcodeArgs) {
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  auto Res = _VAESEncLast(Dest, Src);
  StoreResult(FPRClass, Op, Res, -1);
}

void OpDispatchBuilder::AESDecOp(OpcodeArgs) {
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  auto Res = _VAESDec(Dest, Src);
  StoreResult(FPRClass, Op, Res, -1);
}

void OpDispatchBuilder::AESDecLastOp(OpcodeArgs) {
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  auto Res = _VAESDecLast(Dest, Src);
  StoreResult(FPRClass, Op, Res, -1);
}

void OpDispatchBuilder::AESKeyGenAssist(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  LOGMAN_THROW_A_FMT(Op->Src[1].IsLiteral(), "Src1 needs to be literal here");
  uint64_t RCON = Op->Src[1].Data.Literal.Value;

  auto Res = _VAESKeyGenAssist(Src, RCON);
  StoreResult(FPRClass, Op, Res, -1);
}

}
