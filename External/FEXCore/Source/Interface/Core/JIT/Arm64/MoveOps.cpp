#include "Interface/Core/JIT/Arm64/JITClass.h"

namespace FEXCore::CPU {

using namespace vixl;
using namespace vixl::aarch64;
#define DEF_OP(x) void JITCore::Op_##x(FEXCore::IR::IROp_Header *IROp, uint32_t Node)
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
  switch (Op->Header.Size) {
    case 4: {
      auto Dst = GetSrcPair<RA_32>(Node);
      mov(Dst.first, GetReg<RA_32>(Op->Header.Args[0].ID()));
      mov(Dst.second, GetReg<RA_32>(Op->Header.Args[1].ID()));
      break;
    }
    case 8: {
      auto Dst = GetSrcPair<RA_64>(Node);
      mov(Dst.first, GetReg<RA_64>(Op->Header.Args[0].ID()));
      mov(Dst.second, GetReg<RA_64>(Op->Header.Args[1].ID()));
      break;
    }
    default: LogMan::Msg::A("Unknown Size"); break;
  }
}

DEF_OP(Mov) {
  auto Op = IROp->C<IR::IROp_Mov>();
  mov(GetReg<RA_64>(Node), GetReg<RA_64>(Op->Header.Args[0].ID()));
}

#undef DEF_OP
void JITCore::RegisterMoveHandlers() {
#define REGISTER_OP(op, x) OpHandlers[FEXCore::IR::IROps::OP_##op] = &JITCore::Op_##x
  REGISTER_OP(EXTRACTELEMENTPAIR, ExtractElementPair);
  REGISTER_OP(CREATEELEMENTPAIR,  CreateElementPair);
  REGISTER_OP(MOV,                Mov);
#undef REGISTER_OP
}
}

