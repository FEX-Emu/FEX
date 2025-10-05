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

} // namespace FEXCore::CPU
