/*
$info$
tags: backend|arm64
$end_info$
*/

#include "Interface/Core/JIT/Arm64/JITClass.h"

namespace FEXCore::CPU {

using namespace vixl;
using namespace vixl::aarch64;
#define DEF_OP(x) void Arm64JITCore::Op_##x(IR::IROp_Header const *IROp, IR::NodeID Node)
DEF_OP(ExtractElementPair) {
  auto Op = IROp->C<IR::IROp_ExtractElementPair>();
  switch (Op->Header.Size) {
    case 4: {
      auto Src = GetRegPair<RA_32>(Op->Pair.ID());
      std::array<aarch64::Register, 2> Regs = {Src.first, Src.second};
      mov (GetReg<RA_32>(Node), Regs[Op->Element]);
      break;
    }
    case 8: {
      auto Src = GetRegPair<RA_64>(Op->Pair.ID());
      std::array<aarch64::Register, 2> Regs = {Src.first, Src.second};
      mov (GetReg<RA_64>(Node), Regs[Op->Element]);
      break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown Size"); break;
  }
}

DEF_OP(CreateElementPair) {
  auto Op = IROp->C<IR::IROp_CreateElementPair>();
  std::pair<aarch64::Register, aarch64::Register> Dst;
  aarch64::Register RegFirst;
  aarch64::Register RegSecond;
  aarch64::Register RegTmp;

  switch (IROp->ElementSize) {
    case 4: {
      Dst = GetRegPair<RA_32>(Node);
      RegFirst = GetReg<RA_32>(Op->Lower.ID());
      RegSecond = GetReg<RA_32>(Op->Upper.ID());
      RegTmp = w0;
      break;
    }
    case 8: {
      Dst = GetRegPair<RA_64>(Node);
      RegFirst = GetReg<RA_64>(Op->Lower.ID());
      RegSecond = GetReg<RA_64>(Op->Upper.ID());
      RegTmp = x0;
      break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown Size"); break;
  }

  if (Dst.first.GetCode() != RegSecond.GetCode()) {
    mov(Dst.first, RegFirst);
    mov(Dst.second, RegSecond);
  } else if (Dst.second.GetCode() != RegFirst.GetCode()) {
    mov(Dst.second, RegSecond);
    mov(Dst.first, RegFirst);
  } else {
    mov(RegTmp, RegFirst);
    mov(Dst.second, RegSecond);
    mov(Dst.first, RegTmp);
  }
}

#undef DEF_OP
void Arm64JITCore::RegisterMoveHandlers() {
#define REGISTER_OP(op, x) OpHandlers[FEXCore::IR::IROps::OP_##op] = &Arm64JITCore::Op_##x
  REGISTER_OP(EXTRACTELEMENTPAIR, ExtractElementPair);
  REGISTER_OP(CREATEELEMENTPAIR,  CreateElementPair);
#undef REGISTER_OP
}
}

