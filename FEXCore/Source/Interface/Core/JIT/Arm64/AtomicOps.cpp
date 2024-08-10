// SPDX-License-Identifier: MIT
/*
$info$
tags: backend|arm64
$end_info$
*/

#include "Interface/Context/Context.h"
#include "Interface/Core/Dispatcher/Dispatcher.h"
#include "Interface/Core/JIT/Arm64/JITClass.h"

namespace FEXCore::CPU {
#define DEF_OP(x) void Arm64JITCore::Op_##x(IR::IROp_Header const* IROp, IR::NodeID Node)
DEF_OP(CASPair) {
  auto Op = IROp->C<IR::IROp_CASPair>();
  LOGMAN_THROW_AA_FMT(IROp->ElementSize == 4 || IROp->ElementSize == 8, "Wrong element size");
  // Size is the size of each pair element
  auto Dst0 = GetReg(Op->OutLo.ID());
  auto Dst1 = GetReg(Op->OutHi.ID());
  auto Expected0 = GetReg(Op->ExpectedLo.ID());
  auto Expected1 = GetReg(Op->ExpectedHi.ID());
  auto Desired0 = GetReg(Op->DesiredLo.ID());
  auto Desired1 = GetReg(Op->DesiredHi.ID());
  auto MemSrc = GetReg(Op->Addr.ID());

  const auto EmitSize = IROp->ElementSize == 8 ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;
  if (CTX->HostFeatures.SupportsAtomics) {
    if (Desired1.Idx() != (Desired0.Idx() + 1) || Desired0.Idx() & 1) {
      mov(EmitSize, TMP1, Desired0);
      mov(EmitSize, TMP2, Desired1);
      Desired0 = TMP1;
      Desired1 = TMP2;
    }

    auto CaspalDst0 = Dst0;
    auto CaspalDst1 = Dst1;
    if (CaspalDst1.Idx() != (CaspalDst0.Idx() + 1) || CaspalDst0.Idx() & 1) {
      CaspalDst0 = TMP3;
      CaspalDst1 = TMP4;
    }

    mov(EmitSize, CaspalDst0, Expected0);
    mov(EmitSize, CaspalDst1, Expected1);
    caspal(EmitSize, CaspalDst0, CaspalDst1, Desired0, Desired1, MemSrc);

    if (CaspalDst0 != Dst0) {
      mov(EmitSize, Dst0, CaspalDst0);
      mov(EmitSize, Dst1, CaspalDst1);
    }
  } else {
    // Save NZCV so we don't have to mark this op as clobbering NZCV (the
    // SupportsAtomics does not clobber atomics and this !SupportsAtomics path
    // is so slow it's not worth the complexity of splitting the IR op.). We
    // clobber NZCV inside the hot loop and we can't replace cmp/ccmp/b.ne with
    // something NZCV-preserving without requiring an extra instruction.
    mrs(TMP1, ARMEmitter::SystemRegister::NZCV);

    ARMEmitter::BackwardLabel LoopTop;
    ARMEmitter::SingleUseForwardLabel LoopNotExpected;
    ARMEmitter::SingleUseForwardLabel LoopExpected;
    Bind(&LoopTop);

    // This instruction sequence must be synced with HandleCASPAL_Armv8.
    ldaxp(EmitSize, TMP2, TMP3, MemSrc);
    cmp(EmitSize, TMP2, Expected0);
    ccmp(EmitSize, TMP3, Expected1, ARMEmitter::StatusFlags::None, ARMEmitter::Condition::CC_EQ);
    b(ARMEmitter::Condition::CC_NE, &LoopNotExpected);
    stlxp(EmitSize, TMP2, Desired0, Desired1, MemSrc);
    cbnz(EmitSize, TMP2, &LoopTop);
    mov(EmitSize, Dst0, Expected0);
    mov(EmitSize, Dst1, Expected1);

    b(&LoopExpected);

    Bind(&LoopNotExpected);
    mov(EmitSize, Dst0, TMP2.R());
    mov(EmitSize, Dst1, TMP3.R());
    // exclusive monitor needs to be cleared here
    // Might have hit the case where ldaxr was hit but stlxr wasn't
    clrex();
    Bind(&LoopExpected);

    // Restore
    msr(ARMEmitter::SystemRegister::NZCV, TMP1);
  }
}

DEF_OP(CAS) {
  auto Op = IROp->C<IR::IROp_CAS>();
  const auto EmitSize = ConvertSize(IROp);
  const auto SubEmitSize = ConvertSubRegSize8(IROp->Size);
  // DataSrc = *Src1
  // if (DataSrc == Src3) { *Src1 == Src2; } Src2 = DataSrc
  // This will write to memory! Careful!

  auto Expected = GetReg(Op->Expected.ID());
  auto Desired = GetReg(Op->Desired.ID());
  auto MemSrc = GetReg(Op->Addr.ID());

  if (CTX->HostFeatures.SupportsAtomics) {
    mov(EmitSize, TMP2, Expected);
    casal(SubEmitSize, TMP2, Desired, MemSrc);
    mov(EmitSize, GetReg(Node), TMP2.R());
  } else {
    ARMEmitter::BackwardLabel LoopTop;
    ARMEmitter::SingleUseForwardLabel LoopNotExpected;
    ARMEmitter::SingleUseForwardLabel LoopExpected;
    Bind(&LoopTop);
    ldaxr(SubEmitSize, TMP2, MemSrc);
    if (IROp->Size == 1) {
      cmp(EmitSize, TMP2, Expected, ARMEmitter::ExtendedType::UXTB, 0);
    } else if (IROp->Size == 2) {
      cmp(EmitSize, TMP2, Expected, ARMEmitter::ExtendedType::UXTH, 0);
    } else {
      cmp(EmitSize, TMP2, Expected);
    }
    b(ARMEmitter::Condition::CC_NE, &LoopNotExpected);
    stlxr(SubEmitSize, TMP3, Desired, MemSrc);
    cbnz(EmitSize, TMP3, &LoopTop);
    mov(EmitSize, GetReg(Node), Expected);
    b(&LoopExpected);

    Bind(&LoopNotExpected);
    mov(EmitSize, GetReg(Node), TMP2.R());
    // exclusive monitor needs to be cleared here
    // Might have hit the case where ldaxr was hit but stlxr wasn't
    clrex();
    Bind(&LoopExpected);
  }
}

DEF_OP(AtomicAdd) {
  auto Op = IROp->C<IR::IROp_AtomicAdd>();
  const auto EmitSize = ConvertSize(IROp);
  const auto SubEmitSize = ConvertSubRegSize8(IROp->Size);

  auto MemSrc = GetReg(Op->Addr.ID());
  auto Src = GetReg(Op->Value.ID());

  if (CTX->HostFeatures.SupportsAtomics) {
    staddl(SubEmitSize, Src, MemSrc);
  } else {
    ARMEmitter::BackwardLabel LoopTop;
    Bind(&LoopTop);
    ldaxr(SubEmitSize, TMP2, MemSrc);
    add(EmitSize, TMP2, TMP2, Src);
    stlxr(SubEmitSize, TMP2, TMP2, MemSrc);
    cbnz(EmitSize, TMP2, &LoopTop);
  }
}

DEF_OP(AtomicSub) {
  auto Op = IROp->C<IR::IROp_AtomicSub>();
  const auto EmitSize = ConvertSize(IROp);
  const auto SubEmitSize = ConvertSubRegSize8(IROp->Size);

  auto MemSrc = GetReg(Op->Addr.ID());
  auto Src = GetReg(Op->Value.ID());

  if (CTX->HostFeatures.SupportsAtomics) {
    neg(EmitSize, TMP2, Src);
    staddl(SubEmitSize, TMP2, MemSrc);
  } else {
    ARMEmitter::BackwardLabel LoopTop;
    Bind(&LoopTop);
    ldaxr(SubEmitSize, TMP2, MemSrc);
    sub(EmitSize, TMP2, TMP2, Src);
    stlxr(SubEmitSize, TMP2, TMP2, MemSrc);
    cbnz(EmitSize, TMP2, &LoopTop);
  }
}

DEF_OP(AtomicAnd) {
  auto Op = IROp->C<IR::IROp_AtomicAnd>();
  const auto EmitSize = ConvertSize(IROp);
  const auto SubEmitSize = ConvertSubRegSize8(IROp->Size);

  auto MemSrc = GetReg(Op->Addr.ID());
  auto Src = GetReg(Op->Value.ID());

  if (CTX->HostFeatures.SupportsAtomics) {
    mvn(EmitSize, TMP2, Src);
    stclrl(SubEmitSize, TMP2, MemSrc);
  } else {
    ARMEmitter::BackwardLabel LoopTop;
    Bind(&LoopTop);
    ldaxr(SubEmitSize, TMP2, MemSrc);
    and_(EmitSize, TMP2, TMP2, Src);
    stlxr(SubEmitSize, TMP2, TMP2, MemSrc);
    cbnz(EmitSize, TMP2, &LoopTop);
  }
}

DEF_OP(AtomicCLR) {
  auto Op = IROp->C<IR::IROp_AtomicCLR>();
  const auto EmitSize = ConvertSize(IROp);
  const auto SubEmitSize = ConvertSubRegSize8(IROp->Size);

  auto MemSrc = GetReg(Op->Addr.ID());
  auto Src = GetReg(Op->Value.ID());

  if (CTX->HostFeatures.SupportsAtomics) {
    stclrl(SubEmitSize, Src, MemSrc);
  } else {
    ARMEmitter::BackwardLabel LoopTop;
    Bind(&LoopTop);
    ldaxr(SubEmitSize, TMP2, MemSrc);
    bic(EmitSize, TMP2, TMP2, Src);
    stlxr(SubEmitSize, TMP2, TMP2, MemSrc);
    cbnz(EmitSize, TMP2, &LoopTop);
  }
}

DEF_OP(AtomicOr) {
  auto Op = IROp->C<IR::IROp_AtomicOr>();
  const auto EmitSize = ConvertSize(IROp);
  const auto SubEmitSize = ConvertSubRegSize8(IROp->Size);

  auto MemSrc = GetReg(Op->Addr.ID());
  auto Src = GetReg(Op->Value.ID());

  if (CTX->HostFeatures.SupportsAtomics) {
    stsetl(SubEmitSize, Src, MemSrc);
  } else {
    ARMEmitter::BackwardLabel LoopTop;
    Bind(&LoopTop);
    ldaxr(SubEmitSize, TMP2, MemSrc);
    orr(EmitSize, TMP2, TMP2, Src);
    stlxr(SubEmitSize, TMP2, TMP2, MemSrc);
    cbnz(EmitSize, TMP2, &LoopTop);
  }
}

DEF_OP(AtomicXor) {
  auto Op = IROp->C<IR::IROp_AtomicXor>();
  const auto EmitSize = ConvertSize(IROp);
  const auto SubEmitSize = ConvertSubRegSize8(IROp->Size);

  auto MemSrc = GetReg(Op->Addr.ID());
  auto Src = GetReg(Op->Value.ID());

  if (CTX->HostFeatures.SupportsAtomics) {
    steorl(SubEmitSize, Src, MemSrc);
  } else {
    ARMEmitter::BackwardLabel LoopTop;
    Bind(&LoopTop);
    ldaxr(SubEmitSize, TMP2, MemSrc);
    eor(EmitSize, TMP2, TMP2, Src);
    stlxr(SubEmitSize, TMP2, TMP2, MemSrc);
    cbnz(EmitSize, TMP2, &LoopTop);
  }
}

DEF_OP(AtomicNeg) {
  auto Op = IROp->C<IR::IROp_AtomicNeg>();
  const auto EmitSize = ConvertSize(IROp);
  const auto SubEmitSize = ConvertSubRegSize8(IROp->Size);

  auto MemSrc = GetReg(Op->Addr.ID());

  ARMEmitter::BackwardLabel LoopTop;
  Bind(&LoopTop);
  ldaxr(SubEmitSize, TMP2, MemSrc);
  neg(EmitSize, TMP3, TMP2);
  stlxr(SubEmitSize, TMP4, TMP3, MemSrc);
  cbnz(EmitSize, TMP4, &LoopTop);
}

DEF_OP(AtomicSwap) {
  auto Op = IROp->C<IR::IROp_AtomicSwap>();
  uint8_t OpSize = IROp->Size;
  LOGMAN_THROW_AA_FMT(OpSize == 8 || OpSize == 4 || OpSize == 2 || OpSize == 1, "Unexpected CAS size");

  auto MemSrc = GetReg(Op->Addr.ID());
  auto Src = GetReg(Op->Value.ID());

  const auto EmitSize = ConvertSize(IROp);
  const auto SubEmitSize = OpSize == 8 ? ARMEmitter::SubRegSize::i64Bit :
                           OpSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
                           OpSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
                           OpSize == 1 ? ARMEmitter::SubRegSize::i8Bit :
                                         ARMEmitter::SubRegSize::i8Bit;

  if (CTX->HostFeatures.SupportsAtomics) {
    ldswpal(SubEmitSize, Src, GetReg(Node), MemSrc);
  } else {
    ARMEmitter::BackwardLabel LoopTop;
    Bind(&LoopTop);
    ldaxr(SubEmitSize, TMP2, MemSrc);
    stlxr(SubEmitSize, TMP4, Src, MemSrc);
    cbnz(EmitSize, TMP4, &LoopTop);
    ubfm(EmitSize, GetReg(Node), TMP2, 0, OpSize * 8 - 1);
  }
}

DEF_OP(AtomicFetchAdd) {
  auto Op = IROp->C<IR::IROp_AtomicFetchAdd>();
  const auto EmitSize = ConvertSize(IROp);
  const auto SubEmitSize = ConvertSubRegSize8(IROp->Size);

  auto MemSrc = GetReg(Op->Addr.ID());
  auto Src = GetReg(Op->Value.ID());

  if (CTX->HostFeatures.SupportsAtomics) {
    ldaddal(SubEmitSize, Src, GetReg(Node), MemSrc);
  } else {
    ARMEmitter::BackwardLabel LoopTop;
    Bind(&LoopTop);
    ldaxr(SubEmitSize, TMP2, MemSrc);
    add(EmitSize, TMP3, TMP2, Src);
    stlxr(SubEmitSize, TMP4, TMP3, MemSrc);
    cbnz(EmitSize, TMP4, &LoopTop);
    mov(EmitSize, GetReg(Node), TMP2.R());
  }
}

DEF_OP(AtomicFetchSub) {
  auto Op = IROp->C<IR::IROp_AtomicFetchSub>();
  const auto EmitSize = ConvertSize(IROp);
  const auto SubEmitSize = ConvertSubRegSize8(IROp->Size);

  auto MemSrc = GetReg(Op->Addr.ID());
  auto Src = GetReg(Op->Value.ID());

  if (CTX->HostFeatures.SupportsAtomics) {
    neg(EmitSize, TMP2, Src);
    ldaddal(SubEmitSize, TMP2, GetReg(Node), MemSrc);
  } else {
    ARMEmitter::BackwardLabel LoopTop;
    Bind(&LoopTop);
    ldaxr(SubEmitSize, TMP2, MemSrc);
    sub(EmitSize, TMP3, TMP2, Src);
    stlxr(SubEmitSize, TMP4, TMP3, MemSrc);
    cbnz(EmitSize, TMP4, &LoopTop);
    mov(EmitSize, GetReg(Node), TMP2.R());
  }
}

DEF_OP(AtomicFetchAnd) {
  auto Op = IROp->C<IR::IROp_AtomicFetchAnd>();
  const auto EmitSize = ConvertSize(IROp);
  const auto SubEmitSize = ConvertSubRegSize8(IROp->Size);

  auto MemSrc = GetReg(Op->Addr.ID());
  auto Src = GetReg(Op->Value.ID());

  if (CTX->HostFeatures.SupportsAtomics) {
    mvn(EmitSize, TMP2, Src);
    ldclral(SubEmitSize, TMP2, GetReg(Node), MemSrc);
  } else {
    ARMEmitter::BackwardLabel LoopTop;
    Bind(&LoopTop);
    ldaxr(SubEmitSize, TMP2, MemSrc);
    and_(EmitSize, TMP3, TMP2, Src);
    stlxr(SubEmitSize, TMP4, TMP3, MemSrc);
    cbnz(EmitSize, TMP4, &LoopTop);
    mov(EmitSize, GetReg(Node), TMP2.R());
  }
}

DEF_OP(AtomicFetchCLR) {
  auto Op = IROp->C<IR::IROp_AtomicFetchCLR>();
  const auto EmitSize = ConvertSize(IROp);
  const auto SubEmitSize = ConvertSubRegSize8(IROp->Size);

  auto MemSrc = GetReg(Op->Addr.ID());
  auto Src = GetReg(Op->Value.ID());

  if (CTX->HostFeatures.SupportsAtomics) {
    ldclral(SubEmitSize, Src, GetReg(Node), MemSrc);
  } else {
    ARMEmitter::BackwardLabel LoopTop;
    Bind(&LoopTop);
    ldaxr(SubEmitSize, TMP2, MemSrc);
    bic(EmitSize, TMP3, TMP2, Src);
    stlxr(SubEmitSize, TMP4, TMP3, MemSrc);
    cbnz(EmitSize, TMP4, &LoopTop);
    mov(EmitSize, GetReg(Node), TMP2.R());
  }
}

DEF_OP(AtomicFetchOr) {
  auto Op = IROp->C<IR::IROp_AtomicFetchOr>();
  const auto EmitSize = ConvertSize(IROp);
  const auto SubEmitSize = ConvertSubRegSize8(IROp->Size);

  auto MemSrc = GetReg(Op->Addr.ID());
  auto Src = GetReg(Op->Value.ID());

  if (CTX->HostFeatures.SupportsAtomics) {
    ldsetal(SubEmitSize, Src, GetReg(Node), MemSrc);
  } else {
    ARMEmitter::BackwardLabel LoopTop;
    Bind(&LoopTop);
    ldaxr(SubEmitSize, TMP2, MemSrc);
    orr(EmitSize, TMP3, TMP2, Src);
    stlxr(SubEmitSize, TMP4, TMP3, MemSrc);
    cbnz(EmitSize, TMP4, &LoopTop);
    mov(EmitSize, GetReg(Node), TMP2.R());
  }
}

DEF_OP(AtomicFetchXor) {
  auto Op = IROp->C<IR::IROp_AtomicFetchXor>();
  const auto EmitSize = ConvertSize(IROp);
  const auto SubEmitSize = ConvertSubRegSize8(IROp->Size);

  auto MemSrc = GetReg(Op->Addr.ID());
  auto Src = GetReg(Op->Value.ID());

  if (CTX->HostFeatures.SupportsAtomics) {
    ldeoral(SubEmitSize, Src, GetReg(Node), MemSrc);
  } else {
    ARMEmitter::BackwardLabel LoopTop;
    Bind(&LoopTop);
    ldaxr(SubEmitSize, TMP2, MemSrc);
    eor(EmitSize, TMP3, TMP2, Src);
    stlxr(SubEmitSize, TMP4, TMP3, MemSrc);
    cbnz(EmitSize, TMP4, &LoopTop);
    mov(EmitSize, GetReg(Node), TMP2.R());
  }
}

DEF_OP(AtomicFetchNeg) {
  auto Op = IROp->C<IR::IROp_AtomicFetchNeg>();
  const auto EmitSize = ConvertSize(IROp);
  const auto SubEmitSize = ConvertSubRegSize8(IROp->Size);

  auto MemSrc = GetReg(Op->Addr.ID());

  ARMEmitter::BackwardLabel LoopTop;
  Bind(&LoopTop);
  ldaxr(SubEmitSize, TMP2, MemSrc);
  neg(EmitSize, TMP3, TMP2);
  stlxr(SubEmitSize, TMP4, TMP3, MemSrc);
  cbnz(EmitSize, TMP4, &LoopTop);
  mov(EmitSize, GetReg(Node), TMP2.R());
}

DEF_OP(TelemetrySetValue) {
#ifndef FEX_DISABLE_TELEMETRY
  auto Op = IROp->C<IR::IROp_TelemetrySetValue>();
  auto Src = GetReg(Op->Value.ID());

  ldr(TMP2, STATE_PTR(CpuStateFrame, Pointers.Common.TelemetryValueAddresses[Op->TelemetryValueIndex]));

  // Cortex fuses cmp+cset.
  cmp(ARMEmitter::Size::i32Bit, Src, 0);
  cset(ARMEmitter::Size::i32Bit, TMP1, ARMEmitter::Condition::CC_NE);

  if (CTX->HostFeatures.SupportsAtomics) {
    stsetl(ARMEmitter::SubRegSize::i64Bit, TMP1, TMP2);
  } else {
    ARMEmitter::BackwardLabel LoopTop;
    Bind(&LoopTop);
    ldaxr(ARMEmitter::SubRegSize::i64Bit, TMP3, TMP2);
    orr(ARMEmitter::Size::i32Bit, TMP3, TMP3, Src);
    stlxr(ARMEmitter::SubRegSize::i64Bit, TMP3, TMP3, TMP2);
    cbnz(ARMEmitter::Size::i32Bit, TMP3, &LoopTop);
  }
#endif
}

#undef DEF_OP
} // namespace FEXCore::CPU
