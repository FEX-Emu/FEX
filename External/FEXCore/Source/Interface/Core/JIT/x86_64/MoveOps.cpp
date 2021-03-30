/*
$info$
tags: backend|x86-64
$end_info$
*/

#include "Interface/Core/JIT/x86_64/JITClass.h"
#include "Interface/IR/Passes/RegisterAllocationPass.h"

namespace FEXCore::CPU {

#define DEF_OP(x) void X86JITCore::Op_##x(FEXCore::IR::IROp_Header *IROp, uint32_t Node)
DEF_OP(ExtractElementPair) {
  auto Op = IROp->C<IR::IROp_ExtractElementPair>();
  switch (Op->Header.Size) {
    case 4: {
      auto Src = GetSrcPair<RA_32>(Op->Header.Args[0].ID());
      std::array<Xbyak::Reg, 2> Regs = {Src.first, Src.second};
      mov (GetDst<RA_32>(Node), Regs[Op->Element]);
      break;
    }
    case 8: {
      auto Src = GetSrcPair<RA_64>(Op->Header.Args[0].ID());
      std::array<Xbyak::Reg, 2> Regs = {Src.first, Src.second};
      mov (GetDst<RA_64>(Node), Regs[Op->Element]);
      break;
    }
    default: LogMan::Msg::A("Unknown Size"); break;
  }
}

DEF_OP(CreateElementPair) {
  auto Op = IROp->C<IR::IROp_CreateElementPair>();
  std::pair<Xbyak::Reg, Xbyak::Reg> Dst;
  Xbyak::Reg RegFirst;
  Xbyak::Reg RegSecond;
  Xbyak::Reg RegTmp;

  switch (Op->Header.Size) {
    case 4: {
      Dst = GetSrcPair<RA_32>(Node);
      RegFirst = GetSrc<RA_32>(Op->Header.Args[0].ID());
      RegSecond = GetSrc<RA_32>(Op->Header.Args[1].ID());
      RegTmp = eax;
      break;
    }
    case 8: {
      Dst = GetSrcPair<RA_64>(Node);
      RegFirst = GetSrc<RA_64>(Op->Header.Args[0].ID());
      RegSecond = GetSrc<RA_64>(Op->Header.Args[1].ID());
      RegTmp = rax;
      break;
    }
    default: LogMan::Msg::A("Unknown Size"); break;
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

DEF_OP(Mov) {
  auto Op = IROp->C<IR::IROp_Mov>();
  mov (GetDst<RA_64>(Node), GetSrc<RA_64>(Op->Header.Args[0].ID()));
}

#undef DEF_OP
void X86JITCore::RegisterMoveHandlers() {
#define REGISTER_OP(op, x) OpHandlers[FEXCore::IR::IROps::OP_##op] = &X86JITCore::Op_##x
  REGISTER_OP(EXTRACTELEMENTPAIR, ExtractElementPair);
  REGISTER_OP(CREATEELEMENTPAIR,  CreateElementPair);
  REGISTER_OP(MOV,                Mov);
#undef REGISTER_OP
}
}

