// SPDX-License-Identifier: MIT
/*
$info$
tags: backend|arm64
$end_info$
*/

#include "Interface/Core/JIT/Arm64/JITClass.h"

namespace FEXCore::CPU {
#define DEF_OP(x) void Arm64JITCore::Op_##x(IR::IROp_Header const* IROp, IR::NodeID Node)
DEF_OP(ExtractElementPair) {
  auto Op = IROp->C<IR::IROp_ExtractElementPair>();

  const auto Dst = GetReg(Node);
  const auto Pair = GetRegPair(Op->Pair.ID());
  const auto Src = Op->Element == 0 ? Pair.first : Pair.second;

  if (Dst != Src) {
    mov(ConvertSize48(IROp), Dst, Src);
  }
}

DEF_OP(CreateElementPair) {
  auto Op = IROp->C<IR::IROp_CreateElementPair>();
  LOGMAN_THROW_AA_FMT(IROp->ElementSize == 4 || IROp->ElementSize == 8, "Invalid size");
  std::pair<ARMEmitter::Register, ARMEmitter::Register> Dst = GetRegPair(Node);
  ARMEmitter::Register RegFirst = GetReg(Op->Lower.ID());
  ARMEmitter::Register RegSecond = GetReg(Op->Upper.ID());
  ARMEmitter::Register RegTmp = TMP1.R();

  const auto EmitSize = IROp->ElementSize == 8 ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;

  if (Dst.first.Idx() != RegSecond.Idx()) {
    mov(EmitSize, Dst.first, RegFirst);
    mov(EmitSize, Dst.second, RegSecond);
  } else if (Dst.second.Idx() != RegFirst.Idx()) {
    mov(EmitSize, Dst.second, RegSecond);
    mov(EmitSize, Dst.first, RegFirst);
  } else {
    mov(EmitSize, RegTmp, RegFirst);
    mov(EmitSize, Dst.second, RegSecond);
    mov(EmitSize, Dst.first, RegTmp);
  }
}

DEF_OP(Copy) {
  auto Op = IROp->C<IR::IROp_Copy>();

  mov(ARMEmitter::Size::i64Bit, GetReg(Node), GetReg(Op->Source.ID()));
}

DEF_OP(Swap1) {
  auto Op = IROp->C<IR::IROp_Swap1>();
  auto A = GetReg(Op->A.ID()), B = GetReg(Op->B.ID());
  LOGMAN_THROW_AA_FMT(B == GetReg(Node), "Invariant");

  mov(ARMEmitter::Size::i64Bit, TMP1, A);
  mov(ARMEmitter::Size::i64Bit, A, B);
  mov(ARMEmitter::Size::i64Bit, B, TMP1);
}

DEF_OP(Swap2) {
  // Implemented above
}

#undef DEF_OP
} // namespace FEXCore::CPU
