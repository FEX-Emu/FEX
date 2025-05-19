// SPDX-License-Identifier: MIT
/*
$info$
tags: backend|arm64
$end_info$
*/

#include "Interface/Core/JIT/JITClass.h"

namespace FEXCore::CPU {
DEF_OP(Copy) {
  auto Op = IROp->C<IR::IROp_Copy>();

  mov(ARMEmitter::Size::i64Bit, GetReg(Node), GetReg(Op->Source));
}

DEF_OP(RMWHandle) {
  mov(ARMEmitter::Size::i64Bit, GetReg(Node), GetReg(IROp->Args[0]));
}

DEF_OP(Swap1) {
  auto Op = IROp->C<IR::IROp_Swap1>();
  auto A = GetReg(Op->A), B = GetReg(Op->B);
  LOGMAN_THROW_A_FMT(B == GetReg(Node), "Invariant");

  mov(ARMEmitter::Size::i64Bit, TMP1, A);
  mov(ARMEmitter::Size::i64Bit, A, B);
  mov(ARMEmitter::Size::i64Bit, B, TMP1);
}

DEF_OP(Swap2) {
  // Implemented above
}

} // namespace FEXCore::CPU
