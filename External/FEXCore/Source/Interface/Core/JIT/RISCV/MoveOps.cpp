/*
$info$
tags: backend|riscv64
$end_info$
*/

#include "Interface/Core/JIT/RISCV/JITClass.h"

namespace FEXCore::CPU {
using namespace biscuit;
#define DEF_OP(x) void RISCVJITCore::Op_##x(IR::IROp_Header *IROp, IR::NodeID Node)
DEF_OP(ExtractElementPair) {
  auto Op = IROp->C<IR::IROp_ExtractElementPair>();
  switch (Op->Header.Size) {
    case 4: {
      auto Src = GetSrcPair(Op->Header.Args[0].ID());
      std::array<biscuit::GPR, 2> Regs = {Src.first, Src.second};
      UXTW(GetReg(Node), Regs[Op->Element]);
      break;
    }
    case 8: {
      auto Src = GetSrcPair(Op->Header.Args[0].ID());
      std::array<biscuit::GPR, 2> Regs = {Src.first, Src.second};
      MV(GetReg(Node), Regs[Op->Element]);
      break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown Size"); break;
  }
}

DEF_OP(CreateElementPair) {
  auto Op = IROp->C<IR::IROp_CreateElementPair>();
  auto Dst = GetSrcPair(Node);
  biscuit::GPR RegFirst = GetReg(Op->Header.Args[0].ID());
  biscuit::GPR RegSecond = GetReg(Op->Header.Args[1].ID());

  if (Dst.first != RegSecond) {
    MV(Dst.first, RegFirst);
    MV(Dst.second, RegSecond);
  } else if (Dst.second != RegFirst) {
    MV(Dst.second, RegSecond);
    MV(Dst.first, RegFirst);
  } else {
    MV(TMP1, RegFirst);
    MV(Dst.second, RegSecond);
    MV(Dst.first, TMP1);
  }
}

DEF_OP(Mov) {
  auto Op = IROp->C<IR::IROp_Mov>();
  MV(GetReg(Node), GetReg(Op->Header.Args[0].ID()));
}

#undef DEF_OP
void RISCVJITCore::RegisterMoveHandlers() {
#define REGISTER_OP(op, x) OpHandlers[FEXCore::IR::IROps::OP_##op] = &RISCVJITCore::Op_##x
  REGISTER_OP(EXTRACTELEMENTPAIR, ExtractElementPair);
  REGISTER_OP(CREATEELEMENTPAIR,  CreateElementPair);
  REGISTER_OP(MOV,                Mov);
#undef REGISTER_OP
}
}

