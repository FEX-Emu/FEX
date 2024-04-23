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
  LOGMAN_THROW_AA_FMT(Op->Header.Size == 4 || Op->Header.Size == 8, "Invalid size");
  const auto EmitSize = Op->Header.Size == 8 ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;

  const auto Src = GetRegPair(Op->Pair.ID());
  const std::array<ARMEmitter::Register, 2> Regs = {Src.first, Src.second};
  mov(EmitSize, GetReg(Node), Regs[Op->Element]);
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

#undef DEF_OP
} // namespace FEXCore::CPU
