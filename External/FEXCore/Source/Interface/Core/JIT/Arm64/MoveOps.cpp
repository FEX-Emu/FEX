/*
$info$
tags: backend|arm64
$end_info$
*/

#include "Interface/Core/JIT/Arm64/JITClass.h"

namespace FEXCore::CPU {

using namespace vixl;
using namespace vixl::aarch64;
#define DEF_OP(x) void Arm64JITCore::Op_##x(FEXCore::IR::IROp_Header *IROp, uint32_t Node)
DEF_OP(ExtractElementPair) {
  auto Op = IROp->C<IR::IROp_ExtractElementPair>();
  switch (Op->Header.Size) {
    case 4: {
      auto Src = GetSrcPair<RA_32>(Op->Header.Args[0].ID());
      std::array<aarch64::Register, 2> Regs = {Src.first, Src.second};
      mov (GetReg<RA_32>(Node), Regs[Op->Element]);
      break;
    }
    case 8: {
      auto Src = GetSrcPair<RA_64>(Op->Header.Args[0].ID());
      std::array<aarch64::Register, 2> Regs = {Src.first, Src.second};
      mov (GetReg<RA_64>(Node), Regs[Op->Element]);
      break;
    }
    default: LogMan::Msg::A("Unknown Size"); break;
  }
}

DEF_OP(CreateElementPair) {
  auto Op = IROp->C<IR::IROp_CreateElementPair>();
  std::pair<aarch64::Register, aarch64::Register> Dst;
  aarch64::Register RegFirst;
  aarch64::Register RegSecond;
  aarch64::Register RegTmp;

  switch (Op->Header.Size) {
    case 4: {
      Dst = GetSrcPair<RA_32>(Node);
      RegFirst = GetReg<RA_32>(Op->Header.Args[0].ID());
      RegSecond = GetReg<RA_32>(Op->Header.Args[1].ID());
      RegTmp = w0;
      break;
    }
    case 8: {
      Dst = GetSrcPair<RA_64>(Node);
      RegFirst = GetReg<RA_64>(Op->Header.Args[0].ID());
      RegSecond = GetReg<RA_64>(Op->Header.Args[1].ID());
      RegTmp = x0;
      break;
    }
    default: LogMan::Msg::A("Unknown Size"); break;
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

DEF_OP(Mov) {
  auto Op = IROp->C<IR::IROp_Mov>();
  mov(GetReg<RA_64>(Node), GetReg<RA_64>(Op->Header.Args[0].ID()));
}

#undef DEF_OP
void Arm64JITCore::RegisterMoveHandlers() {
#define REGISTER_OP(op, x) OpHandlers[FEXCore::IR::IROps::OP_##op] = &Arm64JITCore::Op_##x
  REGISTER_OP(EXTRACTELEMENTPAIR, ExtractElementPair);
  REGISTER_OP(CREATEELEMENTPAIR,  CreateElementPair);
  REGISTER_OP(MOV,                Mov);
#undef REGISTER_OP
}
}

