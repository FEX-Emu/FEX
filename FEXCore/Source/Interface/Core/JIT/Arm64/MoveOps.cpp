// SPDX-License-Identifier: MIT
/*
$info$
tags: backend|arm64
$end_info$
*/

#include "Interface/Core/JIT/Arm64/JITClass.h"

namespace FEXCore::CPU {
#define DEF_OP(x) void Arm64JITCore::Op_##x(IR::IROp_Header const* IROp, IR::NodeID Node)
DEF_OP(Copy) {
  auto Op = IROp->C<IR::IROp_Copy>();

  mov(ARMEmitter::Size::i64Bit, GetReg(Node), GetReg(Op->Source.ID()));
}

DEF_OP(Swap1) {
  auto Op = IROp->C<IR::IROp_Swap1>();
  auto A = GetReg(Op->A.ID()), B = GetReg(Op->B.ID());
  LOGMAN_THROW_A_FMT(B == GetReg(Node), "Invariant");

  mov(ARMEmitter::Size::i64Bit, TMP1, A);
  mov(ARMEmitter::Size::i64Bit, A, B);
  mov(ARMEmitter::Size::i64Bit, B, TMP1);
}

DEF_OP(Swap2) {
  // Implemented above
}

#undef DEF_OP
} // namespace FEXCore::CPU
