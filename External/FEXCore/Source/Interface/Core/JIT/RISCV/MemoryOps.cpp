/*
$info$
tags: backend|riscv64
$end_info$
*/

#include "Interface/Core/JIT/RISCV/JITClass.h"
#include <FEXCore/Utils/CompilerDefs.h>

namespace FEXCore::CPU {
using namespace biscuit;

GPR RISCVJITCore::GenerateMemOperand(uint8_t AccessSize, GPR Base, IR::OrderedNodeWrapper Offset, IR::MemOffsetType OffsetType, uint8_t OffsetScale) {
  if (Offset.IsInvalid()) {
    return Base;
  } else {
    if (OffsetScale != 1 && OffsetScale != AccessSize) {
        LOGMAN_MSG_A_FMT("Unhandled GenerateMemOperand OffsetScale: {}", OffsetScale);
    }
    uint64_t Const;
    if (IsInlineConstant(Offset, &Const)) {
      LoadConstant(TMP4, Const);
      ADD(TMP4, Base, TMP4);
      return TMP4;
    } else {
      auto RegOffset = GetReg(Offset.ID());
      ADD(TMP4, Base, RegOffset);
      return TMP4;
    }
  }

  FEX_UNREACHABLE;
}
#define DEF_OP(x) void RISCVJITCore::Op_##x(IR::IROp_Header *IROp, IR::NodeID Node)

DEF_OP(LoadContext) {
  auto Op = IROp->C<IR::IROp_LoadContext>();
  uint8_t OpSize = IROp->Size;
  if (Op->Class == FEXCore::IR::GPRClass) {
    LoadConstant(TMP1, Op->Offset);
    ADD(TMP4, STATE, TMP1);
    switch (OpSize) {
    case 1:
      LBU(GetReg(Node), 0, TMP4);
    break;
    case 2:
      LHU(GetReg(Node), 0, TMP4);
    break;
    case 4:
      LWU(GetReg(Node), 0, TMP4);
    break;
    case 8:
      LD(GetReg(Node), 0, TMP4);
    break;
    default:  LOGMAN_MSG_A_FMT("Unhandled LoadContext size: {}", OpSize);
    }
  }
  else {
    auto Dst = GetVIndex(Node);
    SD(zero, Dst * 16 + 0, FPRSTATE);
    SD(zero, Dst * 16 + 8, FPRSTATE);
    ADDI(TMP2, STATE, Op->Offset);
    switch (OpSize) {
    case 1:
      LBU(TMP1, 0, TMP2);
      SB(TMP1, Dst * 16 + 0, FPRSTATE);
    break;
    case 2:
      LHU(TMP1, 0, TMP2);
      SH(TMP1, Dst * 16 + 0, FPRSTATE);
    break;
    case 4:
      LWU(TMP1, 0, TMP2);
      SW(TMP1, Dst * 16 + 0, FPRSTATE);
    break;
    case 8:
      LD(TMP1, 0, TMP2);
      SD(TMP1, Dst * 16 + 0, FPRSTATE);
    break;
    case 16:
      LD(TMP1, 0, TMP2);
      SD(TMP1, Dst * 16 + 0, FPRSTATE);
      LD(TMP1, 8, TMP2);
      SD(TMP1, Dst * 16 + 8, FPRSTATE);
    break;
    default:  LOGMAN_MSG_A_FMT("Unhandled LoadContext size: {}", OpSize);
    }
  }
}

DEF_OP(StoreContext) {
  auto Op = IROp->C<IR::IROp_StoreContext>();
  uint8_t OpSize = IROp->Size;
  if (Op->Class == FEXCore::IR::GPRClass) {
    switch (OpSize) {
    case 1:
      SB(GetReg(Op->Value.ID()), Op->Offset, STATE);
    break;
    case 2:
      SH(GetReg(Op->Value.ID()), Op->Offset, STATE);
    break;
    case 4:
      SW(GetReg(Op->Value.ID()), Op->Offset, STATE);
    break;
    case 8:
      SD(GetReg(Op->Value.ID()), Op->Offset, STATE);
    break;
    default:  LOGMAN_MSG_A_FMT("Unhandled StoreContext size: {}", OpSize);
    }
  }
  else {
    auto Dst = GetVIndex(Op->Header.Args[0].ID());
    ADDI(TMP2, STATE, Op->Offset);
    switch (OpSize) {
    case 1:
      LBU(TMP1, Dst * 16 + 0, FPRSTATE);
      SB(TMP1, 0, TMP2);
    break;
    case 2:
      LHU(TMP1, Dst * 16 + 0, FPRSTATE);
      SH(TMP1, 0, TMP2);
    break;
    case 4:
      LWU(TMP1, Dst * 16 + 0, FPRSTATE);
      SW(TMP1, 0, TMP2);
    break;
    case 8:
      LD(TMP1, Dst * 16 + 0, FPRSTATE);
      SD(TMP1, 0, TMP2);
    break;
    case 16:
      LD(TMP1, Dst * 16 + 0, FPRSTATE);
      SD(TMP1, 0, TMP2);
      LD(TMP1, Dst * 16 + 8, FPRSTATE);
      SD(TMP1, 8, TMP2);
    break;
    default:  LOGMAN_MSG_A_FMT("Unhandled StoreContext size: {}", OpSize);
    }
  }
}

DEF_OP(LoadContextIndexed) {
  auto Op = IROp->C<IR::IROp_LoadContextIndexed>();
  size_t size = IROp->Size;
  auto index = GetReg(Op->Header.Args[0].ID());

  if (Op->Class == FEXCore::IR::GPRClass) {
    switch (Op->Stride) {
    case 1:
    case 2:
    case 4:
    case 8: {
      LoadConstant(TMP1, Op->Stride);
      LoadConstant(TMP2, Op->BaseOffset);
      MUL(TMP1, index, TMP1);
      ADD(TMP1, STATE, TMP1);
      ADD(TMP1, TMP1, TMP2);

      switch (size) {
      case 1:
        LBU(GetReg(Node), 0, TMP1);
        break;
      case 2:
        LHU(GetReg(Node), 0, TMP1);
        break;
      case 4:
        LWU(GetReg(Node), 0, TMP1);
        break;
      case 8:
        LD(GetReg(Node), 0, TMP1);
        break;
      default:
        LOGMAN_MSG_A_FMT("Unhandled LoadContextIndexed size: {}", IROp->Size);
        break;
      }
      break;
    }
    case 16:
      LOGMAN_MSG_A_FMT("Invalid Class load of size 16");
      break;
    default:
      LOGMAN_MSG_A_FMT("Unhandled LoadContextIndexed stride: {}", Op->Stride);
      break;
    }
  }
  else {
    auto Dst  = GetVIndex(Node);
    switch (Op->Stride) {
    case 1:
    case 2:
    case 4:
    case 8:
    case 16: {
      LoadConstant(TMP1, Op->Stride);
      LoadConstant(TMP2, Op->BaseOffset);
      MUL(TMP1, index, TMP1);
      ADD(TMP1, STATE, TMP1);
      ADD(TMP1, TMP1, TMP2);

      switch (size) {
      case 1:
        LBU(TMP2, 0, TMP1);
        STSize(size, TMP2, Dst * 16 + 0, FPRSTATE);
        break;
      case 2:
        LHU(TMP2, 0, TMP1);
        STSize(size, TMP2, Dst * 16 + 0, FPRSTATE);
        break;
      case 4:
        LWU(TMP2, 0, TMP1);
        STSize(size, TMP2, Dst * 16 + 0, FPRSTATE);
        break;
      case 8:
        LD(TMP2, 0, TMP1);
        STSize(size, TMP2, Dst * 16 + 0, FPRSTATE);
        break;
      case 16:
        LD(TMP2, 0, TMP1);
        STSize(8, TMP2, Dst * 16 + 0, FPRSTATE);
        LD(TMP2, 8, TMP1);
        STSize(8, TMP2, Dst * 16 + 8, FPRSTATE);
        break;
      default:
        LOGMAN_MSG_A_FMT("Unhandled LoadContextIndexed size: {}", IROp->Size);
        break;
      }
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unhandled LoadContextIndexed stride: {}", Op->Stride);
      break;
    }
  }
}

DEF_OP(StoreContextIndexed) {
  auto Op = IROp->C<IR::IROp_StoreContextIndexed>();
  size_t size = IROp->Size;
  auto index = GetReg(Op->Index.ID());

  if (Op->Class == FEXCore::IR::GPRClass) {
    auto value = GetReg(Op->Header.Args[0].ID());

    switch (Op->Stride) {
    case 1:
    case 2:
    case 4:
    case 8: {
      LoadConstant(TMP1, Op->Stride);
      LoadConstant(TMP2, Op->BaseOffset);
      MUL(TMP1, index, TMP1);
      ADD(TMP1, STATE, TMP1);
      ADD(TMP1, TMP1, TMP2);

      switch (size) {
      case 1:
        SB(value, 0, TMP1);
        break;
      case 2:
        SH(value, 0, TMP1);
        break;
      case 4:
        SW(value, 0, TMP1);
        break;
      case 8:
        SD(value, 0, TMP1);
        break;
      default:
        LOGMAN_MSG_A_FMT("Unhandled StoreContextIndexed size: {}", IROp->Size);
        break;
      }
      break;
    }
    case 16:
      LOGMAN_MSG_A_FMT("Invalid Class store of size 16");
      break;
    default:
      LOGMAN_MSG_A_FMT("Unhandled StoreContextIndexed stride: {}", Op->Stride);
      break;
    }
  }
  else {
    auto Src1 = GetVIndex(Op->Value.ID());

    switch (Op->Stride) {
    case 1:
    case 2:
    case 4:
    case 8:
    case 16: {
      LoadConstant(TMP1, Op->Stride);
      LoadConstant(TMP2, Op->BaseOffset);
      MUL(TMP1, index, TMP1);
      ADD(TMP1, STATE, TMP1);
      ADD(TMP1, TMP1, TMP2);

      switch (size) {
      case 1:
        LDSize(size, TMP2, Src1 * 16 + 0, FPRSTATE);
        SB(TMP2, 0, TMP1);
        break;
      case 2:
        LDSize(size, TMP2, Src1 * 16 + 0, FPRSTATE);
        SH(TMP2, 0, TMP1);
        break;
      case 4:
        LDSize(size, TMP2, Src1 * 16 + 0, FPRSTATE);
        SW(TMP2, 0, TMP1);
        break;
      case 8:
        LDSize(size, TMP2, Src1 * 16 + 0, FPRSTATE);
        SD(TMP2, 0, TMP1);
        break;
      case 16:
        LDSize(8, TMP2, Src1 * 16 + 0, FPRSTATE);
        SD(TMP2, 0, TMP1);
        LDSize(8, TMP2, Src1 * 16 + 8, FPRSTATE);
        SD(TMP2, 8, TMP1);
        break;
      default:
        LOGMAN_MSG_A_FMT("Unhandled StoreContextIndexed size: {}", IROp->Size);
        break;
      }
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unhandled StoreContextIndexed stride: {}", Op->Stride);
      break;
    }
  }
}

DEF_OP(LoadFlag) {
  auto Op = IROp->C<IR::IROp_LoadFlag>();
  auto Dst = GetReg(Node);
  LBU(Dst, offsetof(FEXCore::Core::CPUState, flags[0]) + Op->Flag, STATE);
}

DEF_OP(StoreFlag) {
  auto Op = IROp->C<IR::IROp_StoreFlag>();
  SB(GetReg(Op->Header.Args[0].ID()), offsetof(FEXCore::Core::CPUState, flags[0]) + Op->Flag, STATE);
}

DEF_OP(LoadMem) {
  auto Op = IROp->C<IR::IROp_LoadMem>();

  auto MemReg = GetReg(Op->Header.Args[0].ID());
  auto MemSrc = GenerateMemOperand(IROp->Size, MemReg, Op->Offset, Op->OffsetType, Op->OffsetScale);

  if (Op->Class == FEXCore::IR::GPRClass) {
    auto Dst = GetReg(Node);
    switch (IROp->Size) {
      case 1:
        LBU(Dst, 0, MemSrc);
        break;
      case 2:
        LHU(Dst, 0, MemSrc);
        break;
      case 4:
        LWU(Dst, 0, MemSrc);
        break;
      case 8:
        LD(Dst, 0, MemSrc);
        break;
      default:  LOGMAN_MSG_A_FMT("Unhandled LoadMem size: {}", IROp->Size);
    }
  }
  else {
    auto Dst = GetVIndex(Node);
    SD(zero, Dst * 16 + 0, FPRSTATE);
    SD(zero, Dst * 16 + 8, FPRSTATE);
    switch (IROp->Size) {
      case 1:
        LBU(TMP1, 0, MemSrc);
        SB(TMP1, Dst * 16 + 0, FPRSTATE);
        break;
      case 2:
        LHU(TMP1, 0, MemSrc);
        SH(TMP1, Dst * 16 + 0, FPRSTATE);
        break;
      case 4:
        LWU(TMP1, 0, MemSrc);
        SW(TMP1, Dst * 16 + 0, FPRSTATE);
        break;
      case 8:
        LD(TMP1, 0, MemSrc);
        SD(TMP1, Dst * 16 + 0, FPRSTATE);
        break;
      case 16:
        LD(TMP1, 0, MemSrc);
        SD(TMP1, Dst * 16 + 0, FPRSTATE);
        LD(TMP1, 8, MemSrc);
        SD(TMP1, Dst * 16 + 8, FPRSTATE);
        break;
      default:  LOGMAN_MSG_A_FMT("Unhandled LoadMem size: {}", IROp->Size);
    }
  }
}

DEF_OP(LoadMemTSO) {
  auto Op = IROp->C<IR::IROp_LoadMemTSO>();

  auto MemSrc = GetReg(Op->Header.Args[0].ID());

  if (!Op->Offset.IsInvalid()) {
    LOGMAN_MSG_A_FMT("LoadMemTSO: No offset allowed");
  }

  FENCE(FenceOrder::RW, FenceOrder::RW);
  if (Op->Class == FEXCore::IR::GPRClass) {
    auto Dst = GetReg(Node);
    switch (IROp->Size) {
      case 1:
        LBU(Dst, 0, MemSrc);
        break;
      case 2:
        LHU(Dst, 0, MemSrc);
        break;
      case 4:
        LWU(Dst, 0, MemSrc);
        break;
      case 8:
        LD(Dst, 0, MemSrc);
        break;
      default:  LOGMAN_MSG_A_FMT("Unhandled LoadMem size: {}", IROp->Size);
    }
  }
  else {
    auto Dst = GetVIndex(Node);
    SD(zero, Dst * 16 + 0, FPRSTATE);
    SD(zero, Dst * 16 + 8, FPRSTATE);
    switch (IROp->Size) {
      case 1:
        LBU(TMP1, 0, MemSrc);
        SB(TMP1, Dst * 16 + 0, FPRSTATE);
        break;
      case 2:
        LHU(TMP1, 0, MemSrc);
        SH(TMP1, Dst * 16 + 0, FPRSTATE);
        break;
      case 4:
        LWU(TMP1, 0, MemSrc);
        SW(TMP1, Dst * 16 + 0, FPRSTATE);
        break;
      case 8:
        LD(TMP1, 0, MemSrc);
        SD(TMP1, Dst * 16 + 0, FPRSTATE);
        break;
      case 16:
        LD(TMP1, 0, MemSrc);
        SD(TMP1, Dst * 16 + 0, FPRSTATE);
        LD(TMP1, 8, MemSrc);
        SD(TMP1, Dst * 16 + 8, FPRSTATE);
        break;
      default:  LOGMAN_MSG_A_FMT("Unhandled LoadMem size: {}", IROp->Size);
    }
  }
  FENCE(FenceOrder::R, FenceOrder::RW);
}

DEF_OP(StoreMem) {
  auto Op = IROp->C<IR::IROp_StoreMem>();

  auto MemReg = GetReg(Op->Addr.ID());

  auto MemSrc = GenerateMemOperand(IROp->Size, MemReg, Op->Offset, Op->OffsetType, Op->OffsetScale);

  if (Op->Class == FEXCore::IR::GPRClass) {
    switch (IROp->Size) {
      case 1:
        SB(GetReg(Op->Value.ID()), 0, MemSrc);
        break;
      case 2:
        SH(GetReg(Op->Value.ID()), 0, MemSrc);
        break;
      case 4:
        SW(GetReg(Op->Value.ID()), 0, MemSrc);
        break;
      case 8:
        SD(GetReg(Op->Value.ID()), 0, MemSrc);
        break;
      default:  LOGMAN_MSG_A_FMT("Unhandled StoreMem size: {}", IROp->Size);
    }
  }
  else {
    auto Dst = GetVIndex(Op->Value.ID());
    switch (IROp->Size) {
      case 1:
        LBU(TMP1, Dst * 16 + 0, FPRSTATE);
        SB(TMP1, 0, MemSrc);
        break;
      case 2:
        LHU(TMP1, Dst * 16 + 0, FPRSTATE);
        SH(TMP1, 0, MemSrc);
        break;
      case 4:
        LWU(TMP1, Dst * 16 + 0, FPRSTATE);
        SW(TMP1, 0, MemSrc);
        break;
      case 8:
        LD(TMP1, Dst * 16 + 0, FPRSTATE);
        SD(TMP1, 0, MemSrc);
        break;
      case 16:
        LD(TMP1, Dst * 16 + 0, FPRSTATE);
        SD(TMP1, 0, MemSrc);
        LD(TMP1, Dst * 16 + 8, FPRSTATE);
        SD(TMP1, 8, MemSrc);
        break;
      default:  LOGMAN_MSG_A_FMT("Unhandled StoreMem size: {}", IROp->Size);
    }
  }
}

DEF_OP(StoreMemTSO) {
  auto Op = IROp->C<IR::IROp_StoreMem>();

  auto MemSrc = GetReg(Op->Addr.ID());

  if (!Op->Offset.IsInvalid()) {
    LOGMAN_MSG_A_FMT("StoreMemTSO: No offset allowed");
  }

  FENCE(FenceOrder::RW, FenceOrder::W);
  if (Op->Class == FEXCore::IR::GPRClass) {
    switch (IROp->Size) {
      case 1:
        SB(GetReg(Op->Value.ID()), 0, MemSrc);
        break;
      case 2:
        SH(GetReg(Op->Value.ID()), 0, MemSrc);
        break;
      case 4:
        SW(GetReg(Op->Value.ID()), 0, MemSrc);
        break;
      case 8:
        SD(GetReg(Op->Value.ID()), 0, MemSrc);
        break;
      default:  LOGMAN_MSG_A_FMT("Unhandled StoreMem size: {}", IROp->Size);
    }
  }
  else {
    auto Dst = GetVIndex(Op->Value.ID());
    switch (IROp->Size) {
      case 1:
        LBU(TMP1, Dst * 16 + 0, FPRSTATE);
        SB(TMP1, 0, MemSrc);
        break;
      case 2:
        LHU(TMP1, Dst * 16 + 0, FPRSTATE);
        SH(TMP1, 0, MemSrc);
        break;
      case 4:
        LWU(TMP1, Dst * 16 + 0, FPRSTATE);
        SW(TMP1, 0, MemSrc);
        break;
      case 8:
        LD(TMP1, Dst * 16 + 0, FPRSTATE);
        SD(TMP1, 0, MemSrc);
        break;
      case 16:
        LD(TMP1, Dst * 16 + 0, FPRSTATE);
        SD(TMP1, 0, MemSrc);
        LD(TMP1, Dst * 16 + 8, FPRSTATE);
        SD(TMP1, 8, MemSrc);
        break;
      default:  LOGMAN_MSG_A_FMT("Unhandled StoreMem size: {}", IROp->Size);
    }
  }
}

#undef DEF_OP
void RISCVJITCore::RegisterMemoryHandlers() {
#define REGISTER_OP(op, x) OpHandlers[FEXCore::IR::IROps::OP_##op] = &RISCVJITCore::Op_##x
  REGISTER_OP(LOADCONTEXT,         LoadContext);
  REGISTER_OP(STORECONTEXT,        StoreContext);
  //REGISTER_OP(LOADREGISTER,        LoadRegister);
  //REGISTER_OP(STOREREGISTER,       StoreRegister);
  REGISTER_OP(LOADCONTEXTINDEXED,  LoadContextIndexed);
  REGISTER_OP(STORECONTEXTINDEXED, StoreContextIndexed);
  //REGISTER_OP(SPILLREGISTER,       SpillRegister);
  //REGISTER_OP(FILLREGISTER,        FillRegister);
  REGISTER_OP(LOADFLAG,            LoadFlag);
  REGISTER_OP(STOREFLAG,           StoreFlag);
  REGISTER_OP(LOADMEM,             LoadMem);
  REGISTER_OP(STOREMEM,            StoreMem);
  REGISTER_OP(LOADMEMTSO,          LoadMemTSO);
  REGISTER_OP(STOREMEMTSO,         StoreMemTSO);
  //REGISTER_OP(VLOADMEMELEMENT,     VLoadMemElement);
  //REGISTER_OP(VSTOREMEMELEMENT,    VStoreMemElement);
  //REGISTER_OP(CACHELINECLEAR,      CacheLineClear);
  //REGISTER_OP(CACHELINEZERO,       CacheLineZero);
#undef REGISTER_OP
}
}


