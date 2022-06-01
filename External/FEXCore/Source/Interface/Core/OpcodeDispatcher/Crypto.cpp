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
