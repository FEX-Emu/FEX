// SPDX-License-Identifier: MIT
/*
$info$
tags: backend|x86-64
$end_info$
*/

#include "Interface/Core/JIT/x86_64/JITClass.h"
#include "Interface/Core/Dispatcher/X86Dispatcher.h"
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/IR/IR.h>

#include <array>
#include <stdint.h>
#include <utility>

namespace FEXCore::CPU {

#define DEF_OP(x) void X86JITCore::Op_##x(IR::IROp_Header *IROp, IR::NodeID Node)
DEF_OP(ExtractElementPair) {
  auto Op = IROp->C<IR::IROp_ExtractElementPair>();
  switch (Op->Header.Size) {
    case 4: {
      auto Src = GetSrcPair<RA_32>(Op->Pair.ID());
      std::array<Xbyak::Reg, 2> Regs = {Src.first, Src.second};
      mov (GetDst<RA_32>(Node), Regs[Op->Element]);
      break;
    }
    case 8: {
      auto Src = GetSrcPair<RA_64>(Op->Pair.ID());
      std::array<Xbyak::Reg, 2> Regs = {Src.first, Src.second};
      mov (GetDst<RA_64>(Node), Regs[Op->Element]);
      break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown Size"); break;
  }
}

DEF_OP(CreateElementPair) {
  auto Op = IROp->C<IR::IROp_CreateElementPair>();
  std::pair<Xbyak::Reg, Xbyak::Reg> Dst;
  Xbyak::Reg RegFirst;
  Xbyak::Reg RegSecond;
  Xbyak::Reg RegTmp;

  switch (IROp->ElementSize) {
    case 4: {
      Dst = GetSrcPair<RA_32>(Node);
      RegFirst = GetSrc<RA_32>(Op->Lower.ID());
      RegSecond = GetSrc<RA_32>(Op->Upper.ID());
      RegTmp = eax;
      break;
    }
    case 8: {
      Dst = GetSrcPair<RA_64>(Node);
      RegFirst = GetSrc<RA_64>(Op->Lower.ID());
      RegSecond = GetSrc<RA_64>(Op->Upper.ID());
      RegTmp = rax;
      break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown Size"); break;
  }

  if (Dst.first != RegSecond) {
    mov(Dst.first, RegFirst);
    mov(Dst.second, RegSecond);
  } else if (Dst.second != RegFirst) {
    mov(Dst.second, RegSecond);
    mov(Dst.first, RegFirst);
  } else {
    mov(RegTmp, RegFirst);
    mov(Dst.second, RegSecond);
    mov(Dst.first, RegTmp);
  }
}

#undef DEF_OP
void X86JITCore::RegisterMoveHandlers() {
#define REGISTER_OP(op, x) OpHandlers[FEXCore::IR::IROps::OP_##op] = &X86JITCore::Op_##x
  REGISTER_OP(EXTRACTELEMENTPAIR, ExtractElementPair);
  REGISTER_OP(CREATEELEMENTPAIR,  CreateElementPair);
#undef REGISTER_OP
}
}

