/*
$info$
tags: backend|riscv64
$end_info$
*/

#include "Interface/Core/JIT/RISCV/JITClass.h"
#include <FEXCore/Utils/CompilerDefs.h>

namespace FEXCore::CPU {
using namespace biscuit;
#define DEF_OP(x) void RISCVJITCore::Op_##x(IR::IROp_Header *IROp, IR::NodeID Node)
DEF_OP(VectorZero) {
  VXOR(GetVReg(Node), GetVReg(Node), GetVReg(Node));
}

DEF_OP(VectorImm) {
  auto Op = IROp->C<IR::IROp_VectorImm>();
  auto Dst  = GetVIndex(Node);

  LoadConstant(TMP1, Op->Immediate);
  uint8_t Elements = IROp->Size / Op->Header.ElementSize;

  for (size_t i = 0; i < Elements; ++i) {
    STSize(Op->Header.ElementSize, TMP1, Dst * 16 + (i * Op->Header.ElementSize), FPRSTATE);
  }
}

DEF_OP(SplatVector2) {
  auto Op = IROp->C<IR::IROp_SplatVector2>();
  LOGMAN_THROW_A_FMT(IROp->Size <= 16, "Can't handle a vector of size: {}", IROp->Size);

  auto Dst  = GetVIndex(Node);
  auto Src1 = GetVIndex(Op->Header.Args[0].ID());

  uint8_t Elements = IROp->Size / Op->Header.ElementSize;

  LDUSize(Op->Header.ElementSize, TMP1, Src1 * 16 + 0, FPRSTATE);

  for (size_t i = 0; i < Elements; ++i) {
    STSize(Op->Header.ElementSize, TMP1, Dst * 16 + (i * Op->Header.ElementSize), FPRSTATE);
  }
}

DEF_OP(SplatVector4) {
  auto Op = IROp->C<IR::IROp_SplatVector2>();
  LOGMAN_THROW_A_FMT(IROp->Size <= 16, "Can't handle a vector of size: {}", IROp->Size);

  auto Dst  = GetVIndex(Node);
  auto Src1 = GetVIndex(Op->Header.Args[0].ID());

  uint8_t Elements = IROp->Size / Op->Header.ElementSize;

  LDUSize(Op->Header.ElementSize, TMP1, Src1 * 16 + 0, FPRSTATE);

  for (size_t i = 0; i < Elements; ++i) {
    STSize(Op->Header.ElementSize, TMP1, Dst * 16 + (i * Op->Header.ElementSize), FPRSTATE);
  }
}

DEF_OP(VMov) {
  auto Op = IROp->C<IR::IROp_VMov>();
  LOGMAN_THROW_A_FMT(IROp->Size <= 16, "Can't handle a vector of size: {}", IROp->Size);

  auto Dst  = GetVIndex(Node);
  auto Src1 = GetVIndex(Op->Header.Args[0].ID());

  if (IROp->Size == 16) {
    LDUSize(8, TMP1, Src1 * 16 + 0, FPRSTATE);
    STSize(8, TMP1, Dst * 16 + 0, FPRSTATE);
    LDUSize(8, TMP1, Src1 * 16 + 8, FPRSTATE);
    STSize(8, TMP1, Dst * 16 + 8, FPRSTATE);
  }
  else {
    LDUSize(Op->Header.ElementSize, TMP1, Src1 * 16 + 0, FPRSTATE);
    SD(zero, Dst * 16 + 0, FPRSTATE);
    SD(zero, Dst * 16 + 8, FPRSTATE);
    STSize(Op->Header.ElementSize, TMP1, Dst * 16 + 0, FPRSTATE);
  }
}

DEF_OP(VBitcast) {
  auto Op = IROp->C<IR::IROp_VBitcast>();
  LOGMAN_THROW_A_FMT(IROp->Size <= 16, "Can't handle a vector of size: {}", IROp->Size);

  auto Dst  = GetVIndex(Node);
  auto Src1 = GetVIndex(Op->Header.Args[0].ID());

  if (IROp->Size == 16) {
    LDUSize(8, TMP1, Src1 * 16 + 0, FPRSTATE);
    STSize(8, TMP1, Dst * 16 + 0, FPRSTATE);
    LDUSize(8, TMP1, Src1 * 16 + 8, FPRSTATE);
    STSize(8, TMP1, Dst * 16 + 8, FPRSTATE);
  }
  else {
    LDUSize(Op->Header.ElementSize, TMP1, Src1 * 16 + 0, FPRSTATE);
    SD(zero, Dst * 16 + 0, FPRSTATE);
    SD(zero, Dst * 16 + 8, FPRSTATE);
    STSize(Op->Header.ElementSize, TMP1, Dst * 16 + 0, FPRSTATE);
  }
}

DEF_OP(VAnd) {
  auto Op = IROp->C<IR::IROp_VAnd>();
  auto Dst  = GetVIndex(Node);
  auto Src1 = GetVIndex(Op->Header.Args[0].ID());
  auto Src2 = GetVIndex(Op->Header.Args[1].ID());

  for (size_t i = 0; i < 2; ++i) {
    LD(TMP1, Src1 * 16 + (i * 8), FPRSTATE);
    LD(TMP2, Src2 * 16 + (i * 8), FPRSTATE);
    AND(TMP1, TMP1, TMP2);
    SD(TMP1, Dst * 16 + (i * 8), FPRSTATE);
  }
}

DEF_OP(VBic) {
  auto Op = IROp->C<IR::IROp_VBic>();
  auto Dst  = GetVIndex(Node);
  auto Src1 = GetVIndex(Op->Header.Args[0].ID());
  auto Src2 = GetVIndex(Op->Header.Args[1].ID());

  for (size_t i = 0; i < 2; ++i) {
    LD(TMP1, Src1 * 16 + (i * 8), FPRSTATE);
    LD(TMP2, Src2 * 16 + (i * 8), FPRSTATE);
    NOT(TMP2, TMP2);
    AND(TMP1, TMP1, TMP2);
    SD(TMP1, Dst * 16 + (i * 8), FPRSTATE);
  }
}

DEF_OP(VOr) {
  auto Op = IROp->C<IR::IROp_VOr>();

  auto Dst  = GetVIndex(Node);
  auto Src1 = GetVIndex(Op->Header.Args[0].ID());
  auto Src2 = GetVIndex(Op->Header.Args[1].ID());

  for (size_t i = 0; i < 2; ++i) {
    LD(TMP1, Src1 * 16 + (i * 8), FPRSTATE);
    LD(TMP2, Src2 * 16 + (i * 8), FPRSTATE);
    OR(TMP1, TMP1, TMP2);
    SD(TMP1, Dst * 16 + (i * 8), FPRSTATE);
  }
}

DEF_OP(VXor) {
  auto Op = IROp->C<IR::IROp_VXor>();

  auto Dst  = GetVIndex(Node);
  auto Src1 = GetVIndex(Op->Header.Args[0].ID());
  auto Src2 = GetVIndex(Op->Header.Args[1].ID());

  for (size_t i = 0; i < 2; ++i) {
    LD(TMP1, Src1 * 16 + (i * 8), FPRSTATE);
    LD(TMP2, Src2 * 16 + (i * 8), FPRSTATE);
    XOR(TMP1, TMP1, TMP2);
    SD(TMP1, Dst * 16 + (i * 8), FPRSTATE);
  }
}

DEF_OP(VAdd) {
  auto Op = IROp->C<IR::IROp_VAdd>();

  auto Dst  = GetVIndex(Node);
  auto Src1 = GetVIndex(Op->Header.Args[0].ID());
  auto Src2 = GetVIndex(Op->Header.Args[1].ID());

  uint8_t Elements = IROp->Size / Op->Header.ElementSize;

  for (size_t i = 0; i < Elements; ++i) {
    LDUSize(Op->Header.ElementSize, TMP1, Src1 * 16 + (i * Op->Header.ElementSize), FPRSTATE);
    LDUSize(Op->Header.ElementSize, TMP2, Src2 * 16 + (i * Op->Header.ElementSize), FPRSTATE);
    ADD(TMP1, TMP1, TMP2);
    STSize(Op->Header.ElementSize, TMP1, Dst * 16 + (i * Op->Header.ElementSize), FPRSTATE);
  }
}

DEF_OP(VNeg) {
  auto Op = IROp->C<IR::IROp_VNeg>();

  auto Dst  = GetVIndex(Node);
  auto Src1 = GetVIndex(Op->Header.Args[0].ID());

  uint8_t Elements = IROp->Size / Op->Header.ElementSize;

  for (size_t i = 0; i < Elements; ++i) {
    LDUSize(Op->Header.ElementSize, TMP1, Src1 * 16 + (i * Op->Header.ElementSize), FPRSTATE);
    NEG(TMP1, TMP1);
    STSize(Op->Header.ElementSize, TMP1, Dst * 16 + (i * Op->Header.ElementSize), FPRSTATE);
  }
}

DEF_OP(VNot) {
  auto Op = IROp->C<IR::IROp_VNot>();

  auto Dst  = GetVIndex(Node);
  auto Src1 = GetVIndex(Op->Header.Args[0].ID());

  uint8_t Elements = IROp->Size / Op->Header.ElementSize;

  for (size_t i = 0; i < Elements; ++i) {
    LDUSize(Op->Header.ElementSize, TMP1, Src1 * 16 + (i * Op->Header.ElementSize), FPRSTATE);
    NOT(TMP1, TMP1);
    STSize(Op->Header.ElementSize, TMP1, Dst * 16 + (i * Op->Header.ElementSize), FPRSTATE);
  }
}

DEF_OP(VUMin) {
  auto Op = IROp->C<IR::IROp_VUMin>();

  auto Dst  = GetVIndex(Node);
  auto Src1 = GetVIndex(Op->Header.Args[0].ID());
  auto Src2 = GetVIndex(Op->Header.Args[1].ID());

  uint8_t Elements = IROp->Size / Op->Header.ElementSize;

  for (size_t i = 0; i < Elements; ++i) {
    LDUSize(Op->Header.ElementSize, TMP1, Src1 * 16 + (i * Op->Header.ElementSize), FPRSTATE);
    LDUSize(Op->Header.ElementSize, TMP2, Src2 * 16 + (i * Op->Header.ElementSize), FPRSTATE);
    Label l{};
    BLTU(TMP1, TMP2, &l);
    MV(TMP1, TMP2);
    Bind(&l);
    STSize(Op->Header.ElementSize, TMP1, Dst * 16 + (i * Op->Header.ElementSize), FPRSTATE);
  }
}

DEF_OP(VSMin) {
  auto Op = IROp->C<IR::IROp_VSMin>();

  auto Dst  = GetVIndex(Node);
  auto Src1 = GetVIndex(Op->Header.Args[0].ID());
  auto Src2 = GetVIndex(Op->Header.Args[1].ID());

  uint8_t Elements = IROp->Size / Op->Header.ElementSize;

  for (size_t i = 0; i < Elements; ++i) {
    LDSize(Op->Header.ElementSize, TMP1, Src1 * 16 + (i * Op->Header.ElementSize), FPRSTATE);
    LDSize(Op->Header.ElementSize, TMP2, Src2 * 16 + (i * Op->Header.ElementSize), FPRSTATE);
    Label l{};
    BLT(TMP1, TMP2, &l);
    MV(TMP1, TMP2);
    Bind(&l);
    STSize(Op->Header.ElementSize, TMP1, Dst * 16 + (i * Op->Header.ElementSize), FPRSTATE);
  }
}

DEF_OP(VUMax) {
  auto Op = IROp->C<IR::IROp_VUMax>();

  auto Dst  = GetVIndex(Node);
  auto Src1 = GetVIndex(Op->Header.Args[0].ID());
  auto Src2 = GetVIndex(Op->Header.Args[1].ID());

  uint8_t Elements = IROp->Size / Op->Header.ElementSize;

  for (size_t i = 0; i < Elements; ++i) {
    LDUSize(Op->Header.ElementSize, TMP1, Src1 * 16 + (i * Op->Header.ElementSize), FPRSTATE);
    LDUSize(Op->Header.ElementSize, TMP2, Src2 * 16 + (i * Op->Header.ElementSize), FPRSTATE);
    Label l{};
    BLTU(TMP1, TMP2, &l);
    MV(TMP2, TMP1);
    Bind(&l);
    STSize(Op->Header.ElementSize, TMP2, Dst * 16 + (i * Op->Header.ElementSize), FPRSTATE);
  }
}

DEF_OP(VSMax) {
  auto Op = IROp->C<IR::IROp_VSMax>();

  auto Dst  = GetVIndex(Node);
  auto Src1 = GetVIndex(Op->Header.Args[0].ID());
  auto Src2 = GetVIndex(Op->Header.Args[1].ID());

  uint8_t Elements = IROp->Size / Op->Header.ElementSize;

  for (size_t i = 0; i < Elements; ++i) {
    LDSize(Op->Header.ElementSize, TMP1, Src1 * 16 + (i * Op->Header.ElementSize), FPRSTATE);
    LDSize(Op->Header.ElementSize, TMP2, Src2 * 16 + (i * Op->Header.ElementSize), FPRSTATE);
    Label l{};
    BLT(TMP1, TMP2, &l);
    MV(TMP2, TMP1);
    Bind(&l);
    STSize(Op->Header.ElementSize, TMP2, Dst * 16 + (i * Op->Header.ElementSize), FPRSTATE);
  }
}

DEF_OP(VCMPEQ) {
  auto Op = IROp->C<IR::IROp_VCMPEQ>();

  auto Dst  = GetVIndex(Node);
  auto Src1 = GetVIndex(Op->Header.Args[0].ID());
  auto Src2 = GetVIndex(Op->Header.Args[1].ID());

  uint8_t Elements = IROp->Size / Op->Header.ElementSize;

  for (size_t i = 0; i < Elements; ++i) {
    LDSize(Op->Header.ElementSize, TMP1, Src1 * 16 + (i * Op->Header.ElementSize), FPRSTATE);
    LDSize(Op->Header.ElementSize, TMP2, Src2 * 16 + (i * Op->Header.ElementSize), FPRSTATE);
    Label l{};
    Label l2{};
    Label l3{};
    BEQ(TMP1, TMP2, &l);
    MV(TMP1, zero);
    J(&l3);

    Bind(&l);
    NOT(TMP1, zero);
    Bind(&l3);
    STSize(Op->Header.ElementSize, TMP1, Dst * 16 + (i * Op->Header.ElementSize), FPRSTATE);
  }

  if (Op->Header.ElementSize == IROp->Size) {
    if (Op->Header.ElementSize == 8) {
      SD(zero, Dst * 16 + 8, FPRSTATE);
    }
    else {
      // 4 bytes
      SW(zero, Dst * 16 + 4, FPRSTATE);
      SD(zero, Dst * 16 + 8, FPRSTATE);
    }
  }
}

DEF_OP(VCMPEQZ) {
  auto Op = IROp->C<IR::IROp_VCMPEQZ>();

  auto Dst  = GetVIndex(Node);
  auto Src1 = GetVIndex(Op->Header.Args[0].ID());

  uint8_t Elements = IROp->Size / Op->Header.ElementSize;

  for (size_t i = 0; i < Elements; ++i) {
    LDSize(Op->Header.ElementSize, TMP1, Src1 * 16 + (i * Op->Header.ElementSize), FPRSTATE);
    Label l{};
    Label l2{};
    Label l3{};
    BEQZ(TMP1, &l);
    MV(TMP1, zero);
    J(&l3);

    Bind(&l);
    NOT(TMP1, zero);
    Bind(&l3);
    STSize(Op->Header.ElementSize, TMP1, Dst * 16 + (i * Op->Header.ElementSize), FPRSTATE);
  }

  if (Op->Header.ElementSize == IROp->Size) {
    if (Op->Header.ElementSize == 8) {
      SD(zero, Dst * 16 + 8, FPRSTATE);
    }
    else {
      // 4 bytes
      SW(zero, Dst * 16 + 4, FPRSTATE);
      SD(zero, Dst * 16 + 8, FPRSTATE);
    }
  }
}

DEF_OP(VCMPGT) {
  auto Op = IROp->C<IR::IROp_VCMPGT>();

  auto Dst  = GetVIndex(Node);
  auto Src1 = GetVIndex(Op->Header.Args[0].ID());
  auto Src2 = GetVIndex(Op->Header.Args[1].ID());

  uint8_t Elements = IROp->Size / Op->Header.ElementSize;

  for (size_t i = 0; i < Elements; ++i) {
    LDSize(Op->Header.ElementSize, TMP1, Src1 * 16 + (i * Op->Header.ElementSize), FPRSTATE);
    LDSize(Op->Header.ElementSize, TMP2, Src2 * 16 + (i * Op->Header.ElementSize), FPRSTATE);
    Label l{};
    Label l2{};
    Label l3{};
    BGT(TMP1, TMP2, &l);
    MV(TMP1, zero);
    J(&l3);

    Bind(&l);
    NOT(TMP1, zero);
    Bind(&l3);
    STSize(Op->Header.ElementSize, TMP1, Dst * 16 + (i * Op->Header.ElementSize), FPRSTATE);
  }

  if (Op->Header.ElementSize == IROp->Size) {
    if (Op->Header.ElementSize == 8) {
      SD(zero, Dst * 16 + 8, FPRSTATE);
    }
    else {
      // 4 bytes
      SW(zero, Dst * 16 + 4, FPRSTATE);
      SD(zero, Dst * 16 + 8, FPRSTATE);
    }
  }
}

DEF_OP(VCMPGTZ) {
  auto Op = IROp->C<IR::IROp_VCMPGTZ>();

  auto Dst  = GetVIndex(Node);
  auto Src1 = GetVIndex(Op->Header.Args[0].ID());

  uint8_t Elements = IROp->Size / Op->Header.ElementSize;

  for (size_t i = 0; i < Elements; ++i) {
    LDSize(Op->Header.ElementSize, TMP1, Src1 * 16 + (i * Op->Header.ElementSize), FPRSTATE);
    Label l{};
    Label l2{};
    Label l3{};
    BGTZ(TMP1, &l);
    MV(TMP1, zero);
    J(&l3);

    Bind(&l);
    NOT(TMP1, zero);
    Bind(&l3);
    STSize(Op->Header.ElementSize, TMP1, Dst * 16 + (i * Op->Header.ElementSize), FPRSTATE);
  }

  if (Op->Header.ElementSize == IROp->Size) {
    if (Op->Header.ElementSize == 8) {
      SD(zero, Dst * 16 + 8, FPRSTATE);
    }
    else {
      // 4 bytes
      SW(zero, Dst * 16 + 4, FPRSTATE);
      SD(zero, Dst * 16 + 8, FPRSTATE);
    }
  }
}

DEF_OP(VCMPLTZ) {
  auto Op = IROp->C<IR::IROp_VCMPLTZ>();

  auto Dst  = GetVIndex(Node);
  auto Src1 = GetVIndex(Op->Header.Args[0].ID());

  uint8_t Elements = IROp->Size / Op->Header.ElementSize;

  for (size_t i = 0; i < Elements; ++i) {
    LDSize(Op->Header.ElementSize, TMP1, Src1 * 16 + (i * Op->Header.ElementSize), FPRSTATE);
    Label l{};
    Label l2{};
    Label l3{};
    BLTZ(TMP1, &l);
    MV(TMP1, zero);
    J(&l3);

    Bind(&l);
    NOT(TMP1, zero);
    Bind(&l3);
    STSize(Op->Header.ElementSize, TMP1, Dst * 16 + (i * Op->Header.ElementSize), FPRSTATE);
  }

  if (Op->Header.ElementSize == IROp->Size) {
    if (Op->Header.ElementSize == 8) {
      SD(zero, Dst * 16 + 8, FPRSTATE);
    }
    else {
      // 4 bytes
      SW(zero, Dst * 16 + 4, FPRSTATE);
      SD(zero, Dst * 16 + 8, FPRSTATE);
    }
  }
}

DEF_OP(VSub) {
  auto Op = IROp->C<IR::IROp_VSub>();
  auto Dst  = GetVIndex(Node);
  auto Src1 = GetVIndex(Op->Header.Args[0].ID());
  auto Src2 = GetVIndex(Op->Header.Args[1].ID());

  uint8_t Elements = IROp->Size / Op->Header.ElementSize;

  for (size_t i = 0; i < Elements; ++i) {
    LDUSize(Op->Header.ElementSize, TMP1, Src1 * 16 + (i * Op->Header.ElementSize), FPRSTATE);
    LDUSize(Op->Header.ElementSize, TMP2, Src2 * 16 + (i * Op->Header.ElementSize), FPRSTATE);
    SUB(TMP1, TMP1, TMP2);
    STSize(Op->Header.ElementSize, TMP1, Dst * 16 + (i * Op->Header.ElementSize), FPRSTATE);
  }
}

DEF_OP(VAbs) {
  auto Op = IROp->C<IR::IROp_VAbs>();
  auto Dst  = GetVIndex(Node);
  auto Src1 = GetVIndex(Op->Header.Args[0].ID());

  uint8_t Elements = IROp->Size / Op->Header.ElementSize;

  for (size_t i = 0; i < Elements; ++i) {
    LD(TMP1, Src1 * 16 + (i * 8), FPRSTATE);
    // Open-coded integer abs
    SRAI(TMP2, TMP1, Op->Header.ElementSize);
    XOR(TMP1, TMP1, TMP2);
    SUB(TMP1, TMP1, TMP2);
    SD(TMP1, Dst * 16 + (i * 8), FPRSTATE);
  }

  if (Op->Header.ElementSize == IROp->Size) {
    if (Op->Header.ElementSize == 8) {
      SD(zero, Dst * 16 + 8, FPRSTATE);
    }
    else {
      // 4 bytes
      SW(zero, Dst * 16 + 4, FPRSTATE);
      SD(zero, Dst * 16 + 8, FPRSTATE);
    }
  }
}

DEF_OP(VFAdd) {
  auto Op = IROp->C<IR::IROp_VFAdd>();
  auto Dst  = GetVIndex(Node);
  auto Src1 = GetVIndex(Op->Header.Args[0].ID());
  auto Src2 = GetVIndex(Op->Header.Args[1].ID());

  uint8_t Elements = IROp->Size / Op->Header.ElementSize;

  for (size_t i = 0; i < Elements; ++i) {
    LDSize(Op->Header.ElementSize, VTMP1, Src1 * 16 + (i * Op->Header.ElementSize), FPRSTATE);
    LDSize(Op->Header.ElementSize, VTMP2, Src2 * 16 + (i * Op->Header.ElementSize), FPRSTATE);
    switch (Op->Header.ElementSize) {
      case 4: FADD_S(VTMP1, VTMP1, VTMP2); break;
      case 8: FADD_D(VTMP1, VTMP1, VTMP2); break;
      default: LOGMAN_MSG_A_FMT("Unknown element size"); break;
    }
    STSize(Op->Header.ElementSize, VTMP1, Dst * 16 + (i * Op->Header.ElementSize), FPRSTATE);
  }

  if (Op->Header.ElementSize == IROp->Size) {
    if (Op->Header.ElementSize == 8) {
      SD(zero, Dst * 16 + 8, FPRSTATE);
    }
    else {
      // 4 bytes
      SW(zero, Dst * 16 + 4, FPRSTATE);
      SD(zero, Dst * 16 + 8, FPRSTATE);
    }
  }
}

DEF_OP(VFSub) {
  auto Op = IROp->C<IR::IROp_VFSub>();
  auto Dst  = GetVIndex(Node);
  auto Src1 = GetVIndex(Op->Header.Args[0].ID());
  auto Src2 = GetVIndex(Op->Header.Args[1].ID());

  uint8_t Elements = IROp->Size / Op->Header.ElementSize;

  for (size_t i = 0; i < Elements; ++i) {
    LDSize(Op->Header.ElementSize, VTMP1, Src1 * 16 + (i * Op->Header.ElementSize), FPRSTATE);
    LDSize(Op->Header.ElementSize, VTMP2, Src2 * 16 + (i * Op->Header.ElementSize), FPRSTATE);
    switch (Op->Header.ElementSize) {
      case 4: FSUB_S(VTMP1, VTMP1, VTMP2); break;
      case 8: FSUB_D(VTMP1, VTMP1, VTMP2); break;
      default: LOGMAN_MSG_A_FMT("Unknown element size"); break;
    }
    STSize(Op->Header.ElementSize, VTMP1, Dst * 16 + (i * Op->Header.ElementSize), FPRSTATE);
  }

  if (Op->Header.ElementSize == IROp->Size) {
    if (Op->Header.ElementSize == 8) {
      SD(zero, Dst * 16 + 8, FPRSTATE);
    }
    else {
      // 4 bytes
      SW(zero, Dst * 16 + 4, FPRSTATE);
      SD(zero, Dst * 16 + 8, FPRSTATE);
    }
  }
}

DEF_OP(VFMul) {
  auto Op = IROp->C<IR::IROp_VFMul>();
  auto Dst  = GetVIndex(Node);
  auto Src1 = GetVIndex(Op->Header.Args[0].ID());
  auto Src2 = GetVIndex(Op->Header.Args[1].ID());

  uint8_t Elements = IROp->Size / Op->Header.ElementSize;

  for (size_t i = 0; i < Elements; ++i) {
    LDSize(Op->Header.ElementSize, VTMP1, Src1 * 16 + (i * Op->Header.ElementSize), FPRSTATE);
    LDSize(Op->Header.ElementSize, VTMP2, Src2 * 16 + (i * Op->Header.ElementSize), FPRSTATE);
    switch (Op->Header.ElementSize) {
      case 4: FMUL_S(VTMP1, VTMP1, VTMP2); break;
      case 8: FMUL_D(VTMP1, VTMP1, VTMP2); break;
      default: LOGMAN_MSG_A_FMT("Unknown element size"); break;
    }
    STSize(Op->Header.ElementSize, VTMP1, Dst * 16 + (i * Op->Header.ElementSize), FPRSTATE);
  }

  if (Op->Header.ElementSize == IROp->Size) {
    if (Op->Header.ElementSize == 8) {
      SD(zero, Dst * 16 + 8, FPRSTATE);
    }
    else {
      // 4 bytes
      SW(zero, Dst * 16 + 4, FPRSTATE);
      SD(zero, Dst * 16 + 8, FPRSTATE);
    }
  }
}

DEF_OP(VFDiv) {
  auto Op = IROp->C<IR::IROp_VFDiv>();
  auto Dst  = GetVIndex(Node);
  auto Src1 = GetVIndex(Op->Header.Args[0].ID());
  auto Src2 = GetVIndex(Op->Header.Args[1].ID());

  uint8_t Elements = IROp->Size / Op->Header.ElementSize;

  for (size_t i = 0; i < Elements; ++i) {
    LDSize(Op->Header.ElementSize, VTMP1, Src1 * 16 + (i * Op->Header.ElementSize), FPRSTATE);
    LDSize(Op->Header.ElementSize, VTMP2, Src2 * 16 + (i * Op->Header.ElementSize), FPRSTATE);
    switch (Op->Header.ElementSize) {
      case 4: FDIV_S(VTMP1, VTMP1, VTMP2); break;
      case 8: FDIV_D(VTMP1, VTMP1, VTMP2); break;
      default: LOGMAN_MSG_A_FMT("Unknown element size"); break;
    }
    STSize(Op->Header.ElementSize, VTMP1, Dst * 16 + (i * Op->Header.ElementSize), FPRSTATE);
  }

  if (Op->Header.ElementSize == IROp->Size) {
    if (Op->Header.ElementSize == 8) {
      SD(zero, Dst * 16 + 8, FPRSTATE);
    }
    else {
      // 4 bytes
      SW(zero, Dst * 16 + 4, FPRSTATE);
      SD(zero, Dst * 16 + 8, FPRSTATE);
    }
  }

}

DEF_OP(VFMin) {
  auto Op = IROp->C<IR::IROp_VFMin>();
  auto Dst  = GetVIndex(Node);
  auto Src1 = GetVIndex(Op->Header.Args[0].ID());
  auto Src2 = GetVIndex(Op->Header.Args[1].ID());

  uint8_t Elements = IROp->Size / Op->Header.ElementSize;

  for (size_t i = 0; i < Elements; ++i) {
    LDSize(Op->Header.ElementSize, VTMP1, Src1 * 16 + (i * Op->Header.ElementSize), FPRSTATE);
    LDSize(Op->Header.ElementSize, VTMP2, Src2 * 16 + (i * Op->Header.ElementSize), FPRSTATE);
    switch (Op->Header.ElementSize) {
      case 4: FMIN_S(VTMP1, VTMP1, VTMP2); break;
      case 8: FMIN_D(VTMP1, VTMP1, VTMP2); break;
      default: LOGMAN_MSG_A_FMT("Unknown element size"); break;
    }
    STSize(Op->Header.ElementSize, VTMP1, Dst * 16 + (i * Op->Header.ElementSize), FPRSTATE);
  }

  if (Op->Header.ElementSize == IROp->Size) {
    if (Op->Header.ElementSize == 8) {
      SD(zero, Dst * 16 + 8, FPRSTATE);
    }
    else {
      // 4 bytes
      SW(zero, Dst * 16 + 4, FPRSTATE);
      SD(zero, Dst * 16 + 8, FPRSTATE);
    }
  }
}

DEF_OP(VFMax) {
  auto Op = IROp->C<IR::IROp_VFMax>();
  auto Dst  = GetVIndex(Node);
  auto Src1 = GetVIndex(Op->Header.Args[0].ID());
  auto Src2 = GetVIndex(Op->Header.Args[1].ID());

  uint8_t Elements = IROp->Size / Op->Header.ElementSize;

  for (size_t i = 0; i < Elements; ++i) {
    LDSize(Op->Header.ElementSize, VTMP1, Src1 * 16 + (i * Op->Header.ElementSize), FPRSTATE);
    LDSize(Op->Header.ElementSize, VTMP2, Src2 * 16 + (i * Op->Header.ElementSize), FPRSTATE);
    switch (Op->Header.ElementSize) {
      case 4: FMAX_S(VTMP1, VTMP1, VTMP2); break;
      case 8: FMAX_D(VTMP1, VTMP1, VTMP2); break;
      default: LOGMAN_MSG_A_FMT("Unknown element size"); break;
    }
    STSize(Op->Header.ElementSize, VTMP1, Dst * 16 + (i * Op->Header.ElementSize), FPRSTATE);
  }

  if (Op->Header.ElementSize == IROp->Size) {
    if (Op->Header.ElementSize == 8) {
      SD(zero, Dst * 16 + 8, FPRSTATE);
    }
    else {
      // 4 bytes
      SW(zero, Dst * 16 + 4, FPRSTATE);
      SD(zero, Dst * 16 + 8, FPRSTATE);
    }
  }
}

DEF_OP(VFSqrt) {
  auto Op = IROp->C<IR::IROp_VFSqrt>();
  auto Dst  = GetVIndex(Node);
  auto Src1 = GetVIndex(Op->Header.Args[0].ID());

  uint8_t Elements = IROp->Size / Op->Header.ElementSize;

  for (size_t i = 0; i < Elements; ++i) {
    LDSize(Op->Header.ElementSize, VTMP1, Src1 * 16 + (i * Op->Header.ElementSize), FPRSTATE);
    switch (Op->Header.ElementSize) {
      case 4: FSQRT_S(VTMP1, VTMP1); break;
      case 8: FSQRT_D(VTMP1, VTMP1); break;
      default: LOGMAN_MSG_A_FMT("Unknown element size"); break;
    }
    STSize(Op->Header.ElementSize, VTMP1, Dst * 16 + (i * Op->Header.ElementSize), FPRSTATE);
  }

  if (Op->Header.ElementSize == IROp->Size) {
    if (Op->Header.ElementSize == 8) {
      SD(zero, Dst * 16 + 8, FPRSTATE);
    }
    else {
      // 4 bytes
      SW(zero, Dst * 16 + 4, FPRSTATE);
      SD(zero, Dst * 16 + 8, FPRSTATE);
    }
  }
}

DEF_OP(VFNeg) {
  auto Op = IROp->C<IR::IROp_VFNeg>();
  auto Dst  = GetVIndex(Node);
  auto Src1 = GetVIndex(Op->Header.Args[0].ID());

  uint8_t Elements = IROp->Size / Op->Header.ElementSize;

  for (size_t i = 0; i < Elements; ++i) {
    LDSize(Op->Header.ElementSize, VTMP1, Src1 * 16 + (i * Op->Header.ElementSize), FPRSTATE);
    switch (Op->Header.ElementSize) {
      case 4: FNEG_S(VTMP1, VTMP1); break;
      case 8: FNEG_D(VTMP1, VTMP1); break;
      default: LOGMAN_MSG_A_FMT("Unknown element size"); break;
    }
    STSize(Op->Header.ElementSize, VTMP1, Dst * 16 + (i * Op->Header.ElementSize), FPRSTATE);
  }

  if (Op->Header.ElementSize == IROp->Size) {
    if (Op->Header.ElementSize == 8) {
      SD(zero, Dst * 16 + 8, FPRSTATE);
    }
    else {
      // 4 bytes
      SW(zero, Dst * 16 + 4, FPRSTATE);
      SD(zero, Dst * 16 + 8, FPRSTATE);
    }
  }
}

DEF_OP(VUShl) {
  LOGMAN_MSG_A_FMT("Unimplemented");
}

DEF_OP(VUShr) {
  LOGMAN_MSG_A_FMT("Unimplemented");
}

DEF_OP(VSShr) {
  LOGMAN_MSG_A_FMT("Unimplemented");
}

DEF_OP(VUShlS) {
  auto Op = IROp->C<IR::IROp_VUShlS>();
  auto Dst  = GetVIndex(Node);
  auto Src1 = GetVIndex(Op->Header.Args[0].ID());
  auto Src2 = GetVIndex(Op->Header.Args[1].ID());

  uint8_t Elements = IROp->Size / Op->Header.ElementSize;

  LDUSize(Op->Header.ElementSize, TMP2, Src2 * 16 + 0, FPRSTATE);

  for (size_t i = 0; i < Elements; ++i) {
    LDUSize(Op->Header.ElementSize, TMP1, Src1 * 16 + (i * Op->Header.ElementSize), FPRSTATE);
    SLL(TMP1, TMP1, TMP2);
    STSize(Op->Header.ElementSize, TMP1, Dst * 16 + (i * Op->Header.ElementSize), FPRSTATE);
  }
}

DEF_OP(VUShrS) {
  auto Op = IROp->C<IR::IROp_VUShrS>();
  auto Dst  = GetVIndex(Node);
  auto Src1 = GetVIndex(Op->Header.Args[0].ID());
  auto Src2 = GetVIndex(Op->Header.Args[1].ID());

  uint8_t Elements = IROp->Size / Op->Header.ElementSize;

  LDUSize(Op->Header.ElementSize, TMP2, Src2 * 16 + 0, FPRSTATE);

  for (size_t i = 0; i < Elements; ++i) {
    LDUSize(Op->Header.ElementSize, TMP1, Src1 * 16 + (i * Op->Header.ElementSize), FPRSTATE);
    SRL(TMP1, TMP1, TMP2);
    STSize(Op->Header.ElementSize, TMP1, Dst * 16 + (i * Op->Header.ElementSize), FPRSTATE);
  }
}

DEF_OP(VSShrS) {
  auto Op = IROp->C<IR::IROp_VSShrS>();
  auto Dst  = GetVIndex(Node);
  auto Src1 = GetVIndex(Op->Header.Args[0].ID());
  auto Src2 = GetVIndex(Op->Header.Args[1].ID());

  uint8_t Elements = IROp->Size / Op->Header.ElementSize;

  LDUSize(Op->Header.ElementSize, TMP2, Src2 * 16 + 0, FPRSTATE);

  for (size_t i = 0; i < Elements; ++i) {
    LDSize(Op->Header.ElementSize, TMP1, Src1 * 16 + (i * Op->Header.ElementSize), FPRSTATE);
    SRA(TMP1, TMP1, TMP2);
    STSize(Op->Header.ElementSize, TMP1, Dst * 16 + (i * Op->Header.ElementSize), FPRSTATE);
  }
}

DEF_OP(VUShrI) {
  auto Op = IROp->C<IR::IROp_VUShrI>();
  auto Dst  = GetVIndex(Node);
  auto Src1 = GetVIndex(Op->Header.Args[0].ID());

  uint8_t Elements = IROp->Size / Op->Header.ElementSize;

  if (Op->BitShift >= (Op->Header.ElementSize * 8)) {
    SD(zero, Dst * 16 + 0, FPRSTATE);
    SD(zero, Dst * 16 + 8, FPRSTATE);
  }
  else {
    for (size_t i = 0; i < Elements; ++i) {
      LDUSize(Op->Header.ElementSize, TMP1, Src1 * 16 + (i * Op->Header.ElementSize), FPRSTATE);
      SRLI(TMP1, TMP1, Op->BitShift);
      STSize(Op->Header.ElementSize, TMP1, Dst * 16 + (i * Op->Header.ElementSize), FPRSTATE);
    }
  }
}

DEF_OP(VSShrI) {
  auto Op = IROp->C<IR::IROp_VSShrI>();
  auto Dst  = GetVIndex(Node);
  auto Src1 = GetVIndex(Op->Header.Args[0].ID());

  uint8_t Elements = IROp->Size / Op->Header.ElementSize;

  if (Op->BitShift >= (Op->Header.ElementSize * 8)) {
    SD(zero, Dst * 16 + 0, FPRSTATE);
    SD(zero, Dst * 16 + 8, FPRSTATE);
  }
  else {
    for (size_t i = 0; i < Elements; ++i) {
      LDSize(Op->Header.ElementSize, TMP1, Src1 * 16 + (i * Op->Header.ElementSize), FPRSTATE);
      SRAI(TMP1, TMP1, Op->BitShift);
      STSize(Op->Header.ElementSize, TMP1, Dst * 16 + (i * Op->Header.ElementSize), FPRSTATE);
    }
  }
}

DEF_OP(VShlI) {
  auto Op = IROp->C<IR::IROp_VShlI>();
  auto Dst  = GetVIndex(Node);
  auto Src1 = GetVIndex(Op->Header.Args[0].ID());

  uint8_t Elements = IROp->Size / Op->Header.ElementSize;

  if (Op->BitShift >= (Op->Header.ElementSize * 8)) {
    SD(zero, Dst * 16 + 0, FPRSTATE);
    SD(zero, Dst * 16 + 8, FPRSTATE);
  }
  else {
    for (size_t i = 0; i < Elements; ++i) {
      LDUSize(Op->Header.ElementSize, TMP1, Src1 * 16 + (i * Op->Header.ElementSize), FPRSTATE);
      SLLI(TMP1, TMP1, Op->BitShift);
      STSize(Op->Header.ElementSize, TMP1, Dst * 16 + (i * Op->Header.ElementSize), FPRSTATE);
    }
  }
}

DEF_OP(VInsElement) {
  auto Op = IROp->C<IR::IROp_VInsElement>();
  auto Dst  = GetVIndex(Node);
  auto Src1 = GetVIndex(Op->Header.Args[0].ID());
  auto Src2 = GetVIndex(Op->Header.Args[1].ID());

  LD(TMP1, Src1 * 16 + 0, FPRSTATE);
  SD(TMP1, Dst * 16 + 8, FPRSTATE);
  LD(TMP1, Src1 * 16 + 8, FPRSTATE);
  SD(TMP1, Dst * 16 + 8, FPRSTATE);

  LDUSize(Op->Header.ElementSize, TMP1, Src2 * 16 + (Op->SrcIdx * Op->Header.ElementSize), FPRSTATE);
  STSize(Op->Header.ElementSize, TMP1, Dst * 16 + (Op->DestIdx * Op->Header.ElementSize), FPRSTATE);
}

DEF_OP(VInsScalarElement) {
  auto Op = IROp->C<IR::IROp_VInsScalarElement>();

  auto Dst  = GetVIndex(Node);
  auto Src1 = GetVIndex(Op->Header.Args[0].ID());
  auto Src2 = GetVIndex(Op->Header.Args[1].ID());

  LD(TMP1, Src1 * 16 + 0, FPRSTATE);
  SD(TMP1, Dst * 16 + 8, FPRSTATE);
  LD(TMP1, Src1 * 16 + 8, FPRSTATE);
  SD(TMP1, Dst * 16 + 8, FPRSTATE);

  LDUSize(Op->Header.ElementSize, TMP1, Src2 * 16 + 0, FPRSTATE);
  STSize(Op->Header.ElementSize, TMP1, Dst * 16 + (Op->DestIdx * Op->Header.ElementSize), FPRSTATE);
}

DEF_OP(VExtractElement) {
  auto Op = IROp->C<IR::IROp_VExtractElement>();
  auto Dst  = GetVIndex(Node);
  auto Src1 = GetVIndex(Op->Header.Args[0].ID());

  LDSize(Op->Header.ElementSize, VTMP1, Src1 * 16 + (Op->Index * Op->Header.ElementSize), FPRSTATE);
  SD(zero, Dst * 16 + 0, FPRSTATE);
  SD(zero, Dst * 16 + 8, FPRSTATE);
  STSize(Op->Header.ElementSize, VTMP1, Dst * 16 + 0, FPRSTATE);
}

DEF_OP(VDupElement) {
  auto Op = IROp->C<IR::IROp_VDupElement>();
  auto Dst  = GetVIndex(Node);
  auto Src1 = GetVIndex(Op->Header.Args[0].ID());

  uint8_t Elements = IROp->Size / Op->Header.ElementSize;

  LDSize(Op->Header.ElementSize, VTMP1, Src1 * 16 + (Op->Index * Op->Header.ElementSize), FPRSTATE);
  for (size_t i = 0; i < Elements; ++i) {
    STSize(Op->Header.ElementSize, VTMP1, Dst * 16 + (i * Op->Header.ElementSize), FPRSTATE);
  }
}

DEF_OP(VUMul) {
  auto Op = IROp->C<IR::IROp_VUMul>();
  auto Dst  = GetVIndex(Node);
  auto Src1 = GetVIndex(Op->Header.Args[0].ID());
  auto Src2 = GetVIndex(Op->Header.Args[1].ID());

  uint8_t Elements = IROp->Size / Op->Header.ElementSize;

  for (size_t i = 0; i < Elements; ++i) {
    LDUSize(Op->Header.ElementSize, TMP1, Src1 * 16 + (i * Op->Header.ElementSize), FPRSTATE);
    LDUSize(Op->Header.ElementSize, TMP2, Src2 * 16 + (i * Op->Header.ElementSize), FPRSTATE);
    MUL(TMP1, TMP1, TMP2);
    STSize(Op->Header.ElementSize, TMP1, Dst * 16 + (i * Op->Header.ElementSize), FPRSTATE);
  }
}

DEF_OP(VSMul) {
  auto Op = IROp->C<IR::IROp_VSMul>();
  auto Dst  = GetVIndex(Node);
  auto Src1 = GetVIndex(Op->Header.Args[0].ID());
  auto Src2 = GetVIndex(Op->Header.Args[1].ID());

  uint8_t Elements = IROp->Size / Op->Header.ElementSize;

  for (size_t i = 0; i < Elements; ++i) {
    LDSize(Op->Header.ElementSize, TMP1, Src1 * 16 + (i * Op->Header.ElementSize), FPRSTATE);
    LDSize(Op->Header.ElementSize, TMP2, Src2 * 16 + (i * Op->Header.ElementSize), FPRSTATE);
    MUL(TMP1, TMP1, TMP2);
    STSize(Op->Header.ElementSize, TMP1, Dst * 16 + (i * Op->Header.ElementSize), FPRSTATE);
  }
}

#undef DEF_OP
void RISCVJITCore::RegisterVectorHandlers() {
#define REGISTER_OP(op, x) OpHandlers[FEXCore::IR::IROps::OP_##op] = &RISCVJITCore::Op_##x
  REGISTER_OP(VECTORZERO,        VectorZero);
  REGISTER_OP(VECTORIMM,         VectorImm);
  REGISTER_OP(SPLATVECTOR2,      SplatVector2);
  REGISTER_OP(SPLATVECTOR4,      SplatVector4);
  REGISTER_OP(VMOV,              VMov);
  REGISTER_OP(VAND,              VAnd);
  REGISTER_OP(VBIC,              VBic);
  REGISTER_OP(VOR,               VOr);
  REGISTER_OP(VXOR,              VXor);
  REGISTER_OP(VADD,              VAdd);
  REGISTER_OP(VSUB,              VSub);
  //REGISTER_OP(VUQADD,            VUQAdd);
  //REGISTER_OP(VUQSUB,            VUQSub);
  //REGISTER_OP(VSQADD,            VSQAdd);
  //REGISTER_OP(VSQSUB,            VSQSub);
  //REGISTER_OP(VADDP,             VAddP);
  //REGISTER_OP(VADDV,             VAddV);
  //REGISTER_OP(VUMINV,            VUMinV);
  //REGISTER_OP(VURAVG,            VURAvg);
  REGISTER_OP(VABS,              VAbs);
  //REGISTER_OP(VPOPCOUNT,         VPopcount);
  REGISTER_OP(VFADD,             VFAdd);
  //REGISTER_OP(VFADDP,            VFAddP);
  REGISTER_OP(VFSUB,             VFSub);
  REGISTER_OP(VFMUL,             VFMul);
  REGISTER_OP(VFDIV,             VFDiv);
  REGISTER_OP(VFMIN,             VFMin);
  REGISTER_OP(VFMAX,             VFMax);
  //REGISTER_OP(VFRECP,            VFRecp);
  REGISTER_OP(VFSQRT,            VFSqrt);
  //REGISTER_OP(VFRSQRT,           VFRSqrt);
  REGISTER_OP(VNEG,              VNeg);
  REGISTER_OP(VFNEG,             VFNeg);
  REGISTER_OP(VNOT,              VNot);
  REGISTER_OP(VUMIN,             VUMin);
  REGISTER_OP(VSMIN,             VSMin);
  REGISTER_OP(VUMAX,             VUMax);
  REGISTER_OP(VSMAX,             VSMax);
  //REGISTER_OP(VZIP,              VZip);
  //REGISTER_OP(VZIP2,             VZip2);
  //REGISTER_OP(VUNZIP,            VUnZip);
  //REGISTER_OP(VUNZIP2,           VUnZip2);
  //REGISTER_OP(VBSL,              VBSL);
  REGISTER_OP(VCMPEQ,            VCMPEQ);
  REGISTER_OP(VCMPEQZ,           VCMPEQZ);
  REGISTER_OP(VCMPGT,            VCMPGT);
  REGISTER_OP(VCMPGTZ,           VCMPGTZ);
  REGISTER_OP(VCMPLTZ,           VCMPLTZ);
  //REGISTER_OP(VFCMPEQ,           VFCMPEQ);
  //REGISTER_OP(VFCMPNEQ,          VFCMPNEQ);
  //REGISTER_OP(VFCMPLT,           VFCMPLT);
  //REGISTER_OP(VFCMPGT,           VFCMPGT);
  //REGISTER_OP(VFCMPLE,           VFCMPLE);
  //REGISTER_OP(VFCMPORD,          VFCMPORD);
  //REGISTER_OP(VFCMPUNO,          VFCMPUNO);
  REGISTER_OP(VUSHL,             VUShl);
  REGISTER_OP(VUSHR,             VUShr);
  REGISTER_OP(VSSHR,             VSShr);
  REGISTER_OP(VUSHLS,            VUShlS);
  REGISTER_OP(VUSHRS,            VUShrS);
  REGISTER_OP(VSSHRS,            VSShrS);
  REGISTER_OP(VINSELEMENT,       VInsElement);
  REGISTER_OP(VINSSCALARELEMENT, VInsScalarElement);
  REGISTER_OP(VEXTRACTELEMENT,   VExtractElement);
  REGISTER_OP(VDUPELEMENT,       VDupElement);
  //REGISTER_OP(VEXTR,             VExtr);
  //REGISTER_OP(VSLI,              VSLI);
  //REGISTER_OP(VSRI,              VSRI);
  REGISTER_OP(VUSHRI,            VUShrI);
  REGISTER_OP(VSSHRI,            VSShrI);
  REGISTER_OP(VSHLI,             VShlI);
  //REGISTER_OP(VUSHRNI,           VUShrNI);
  //REGISTER_OP(VUSHRNI2,          VUShrNI2);
  REGISTER_OP(VBITCAST,          VBitcast);
  //REGISTER_OP(VSXTL,             VSXTL);
  //REGISTER_OP(VSXTL2,            VSXTL2);
  //REGISTER_OP(VUXTL,             VUXTL);
  //REGISTER_OP(VUXTL2,            VUXTL2);
  //REGISTER_OP(VSQXTN,            VSQXTN);
  //REGISTER_OP(VSQXTN2,           VSQXTN2);
  //REGISTER_OP(VSQXTUN,           VSQXTUN);
  //REGISTER_OP(VSQXTUN2,          VSQXTUN2);
  REGISTER_OP(VUMUL,             VUMul);
  REGISTER_OP(VSMUL,             VSMul);
  //REGISTER_OP(VUMULL,            VUMull);
  //REGISTER_OP(VSMULL,            VSMull);
  //REGISTER_OP(VUMULL2,           VUMull2);
  //REGISTER_OP(VSMULL2,           VSMull2);
  //REGISTER_OP(VUABDL,            VUABDL);
  //REGISTER_OP(VTBL1,             VTBL1);
  //REGISTER_OP(VREV64,            VRev64);
#undef REGISTER_OP
}
}

