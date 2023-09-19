// SPDX-License-Identifier: MIT
/*
$info$
tags: backend|arm64
$end_info$
*/

#include "Interface/Core/JIT/Arm64/JITClass.h"

namespace FEXCore::CPU {
#define DEF_OP(x) void Arm64JITCore::Op_##x(IR::IROp_Header const *IROp, IR::NodeID Node)
DEF_OP(GetHostFlag) {
  auto Op = IROp->C<IR::IROp_GetHostFlag>();
  ubfx(ARMEmitter::Size::i64Bit, GetReg(Node), GetReg(Op->Value.ID()), Op->Flag, 1);
}

#undef DEF_OP
}

