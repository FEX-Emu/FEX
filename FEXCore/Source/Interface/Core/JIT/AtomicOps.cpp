// SPDX-License-Identifier: MIT
/*
$info$
tags: backend|arm64
$end_info$
*/

#include "Interface/Context/Context.h"
#include "Interface/Core/Dispatcher/Dispatcher.h"
#include "Interface/Core/JIT/JITClass.h"

namespace FEXCore::CPU {
DEF_OP(CASPair) {
  auto Op = IROp->C<IR::IROp_CASPair>();
  LOGMAN_THROW_A_FMT(IROp->ElementSize == IR::OpSize::i32Bit || IROp->ElementSize == IR::OpSize::i64Bit, "Wrong element size");
  // Size is the size of each pair element
  auto Dst0 = GetReg(Op->OutLo);
  auto Dst1 = GetReg(Op->OutHi);
  auto Expected0 = GetReg(Op->ExpectedLo);
  auto Expected1 = GetReg(Op->ExpectedHi);
  auto Desired0 = GetReg(Op->DesiredLo);
  auto Desired1 = GetReg(Op->DesiredHi);
  auto MemSrc = GetReg(Op->Addr);

  const auto EmitSize = IROp->ElementSize == IR::OpSize::i64Bit ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;
  if (CTX->HostFeatures.SupportsAtomics) {
    // RA has heuristics to try to pair sources, but we need to handle the cases
    // where they fail. We do so by moving to temporaries. Note we use 64-bit
    // moves here even for 32-bit cmpxchg, for the Firestorm register renamer.
    if (Desired1.Idx() != (Desired0.Idx() + 1) || Desired0.Idx() & 1) {
      mov(ARMEmitter::Size::i64Bit, TMP1, Desired0);
      mov(ARMEmitter::Size::i64Bit, TMP2, Desired1);
      Desired0 = TMP1;
      Desired1 = TMP2;
    }

    auto CaspalDst0 = Dst0;
    auto CaspalDst1 = Dst1;
    if (CaspalDst1.Idx() != (CaspalDst0.Idx() + 1) || CaspalDst0.Idx() & 1) {
      CaspalDst0 = TMP3;
      CaspalDst1 = TMP4;
    }

    // We can't clobber the source, these moves are inherently required due to
    // ISA limitations. But by making them 64-bit, Firestorm can rename.
    mov(ARMEmitter::Size::i64Bit, CaspalDst0, Expected0);
    mov(ARMEmitter::Size::i64Bit, CaspalDst1, Expected1);
    caspal(EmitSize, CaspalDst0, CaspalDst1, Desired0, Desired1, MemSrc);

    if (CaspalDst0 != Dst0) {
      mov(ARMEmitter::Size::i64Bit, Dst0, CaspalDst0);
      mov(ARMEmitter::Size::i64Bit, Dst1, CaspalDst1);
    }
  } else {
    // Save NZCV so we don't have to mark this op as clobbering NZCV (the
    // SupportsAtomics does not clobber atomics and this !SupportsAtomics path
    // is so slow it's not worth the complexity of splitting the IR op.). We
    // clobber NZCV inside the hot loop and we can't replace cmp/ccmp/b.ne with
    // something NZCV-preserving without requiring an extra instruction.
    mrs(TMP1, ARMEmitter::SystemRegister::NZCV);

    ARMEmitter::BackwardLabel LoopTop;
    ARMEmitter::ForwardLabel LoopNotExpected;
    ARMEmitter::ForwardLabel LoopExpected;
    (void)Bind(&LoopTop);

    // This instruction sequence must be synced with HandleCASPAL_Armv8.
    ldaxp(EmitSize, TMP2, TMP3, MemSrc);
    cmp(EmitSize, TMP2, Expected0);
    ccmp(EmitSize, TMP3, Expected1, ARMEmitter::StatusFlags::None, ARMEmitter::Condition::CC_EQ);
    (void)b(ARMEmitter::Condition::CC_NE, &LoopNotExpected);
    stlxp(EmitSize, TMP2, Desired0, Desired1, MemSrc);
    (void)cbnz(EmitSize, TMP2, &LoopTop);
    mov(EmitSize, Dst0, Expected0);
    mov(EmitSize, Dst1, Expected1);

    (void)b(&LoopExpected);

    (void)Bind(&LoopNotExpected);
    mov(EmitSize, Dst0, TMP2.R());
    mov(EmitSize, Dst1, TMP3.R());
    // exclusive monitor needs to be cleared here
    // Might have hit the case where ldaxr was hit but stlxr wasn't
    clrex();
    (void)Bind(&LoopExpected);

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

  auto Expected = GetReg(Op->Expected);
  auto Desired = GetReg(Op->Desired);
  auto MemSrc = GetReg(Op->Addr);
  auto Dst = GetReg(Node);

  if (CTX->HostFeatures.SupportsAtomics) {
    if (Expected == Dst && Dst != MemSrc && Dst != Desired) {
      casal(SubEmitSize, Dst, Desired, MemSrc);
    } else {
      mov(EmitSize, TMP2, Expected);
      casal(SubEmitSize, TMP2, Desired, MemSrc);
      mov(EmitSize, Dst, TMP2.R());
    }
  } else {
    ARMEmitter::BackwardLabel LoopTop;
    ARMEmitter::ForwardLabel LoopNotExpected;
    ARMEmitter::ForwardLabel LoopExpected;
    (void)Bind(&LoopTop);
    ldaxr(SubEmitSize, TMP2, MemSrc);
    if (IROp->Size == IR::OpSize::i8Bit) {
      cmp(EmitSize, TMP2, Expected, ARMEmitter::ExtendedType::UXTB, 0);
    } else if (IROp->Size == IR::OpSize::i16Bit) {
      cmp(EmitSize, TMP2, Expected, ARMEmitter::ExtendedType::UXTH, 0);
    } else {
      cmp(EmitSize, TMP2, Expected);
    }
    (void)b(ARMEmitter::Condition::CC_NE, &LoopNotExpected);
    stlxr(SubEmitSize, TMP3, Desired, MemSrc);
    (void)cbnz(EmitSize, TMP3, &LoopTop);
    mov(EmitSize, Dst, Expected);
    (void)b(&LoopExpected);

    (void)Bind(&LoopNotExpected);
    mov(EmitSize, Dst, TMP2.R());
    // exclusive monitor needs to be cleared here
    // Might have hit the case where ldaxr was hit but stlxr wasn't
    clrex();
    (void)Bind(&LoopExpected);
  }
}

DEF_OP(AtomicXor) {
  auto Op = IROp->C<IR::IROp_AtomicXor>();
  const auto EmitSize = ConvertSize(IROp);
  const auto SubEmitSize = ConvertSubRegSize8(IROp->Size);

  auto MemSrc = GetReg(Op->Addr);
  auto Src = GetReg(Op->Value);

  if (CTX->HostFeatures.SupportsAtomics) {
    steorl(SubEmitSize, Src, MemSrc);
  } else {
    ARMEmitter::BackwardLabel LoopTop;
    (void)Bind(&LoopTop);
    ldaxr(SubEmitSize, TMP2, MemSrc);
    eor(EmitSize, TMP2, TMP2, Src);
    stlxr(SubEmitSize, TMP2, TMP2, MemSrc);
    (void)cbnz(EmitSize, TMP2, &LoopTop);
  }
}

DEF_OP(AtomicSwap) {
  auto Op = IROp->C<IR::IROp_AtomicSwap>();
  const auto OpSize = IROp->Size;
  LOGMAN_THROW_A_FMT(
    OpSize == IR::OpSize::i64Bit || OpSize == IR::OpSize::i32Bit || OpSize == IR::OpSize::i16Bit || OpSize == IR::OpSize::i8Bit, "Unexpecte"
                                                                                                                                 "d CAS "
                                                                                                                                 "size");

  auto MemSrc = GetReg(Op->Addr);
  auto Src = GetReg(Op->Value);

  const auto EmitSize = ConvertSize(IROp);
  const auto SubEmitSize = OpSize == IR::OpSize::i64Bit ? ARMEmitter::SubRegSize::i64Bit :
                           OpSize == IR::OpSize::i32Bit ? ARMEmitter::SubRegSize::i32Bit :
                           OpSize == IR::OpSize::i16Bit ? ARMEmitter::SubRegSize::i16Bit :
                                                          ARMEmitter::SubRegSize::i8Bit;

  if (CTX->HostFeatures.SupportsAtomics) {
    ldswpal(SubEmitSize, Src, GetReg(Node), MemSrc);
  } else {
    ARMEmitter::BackwardLabel LoopTop;
    (void)Bind(&LoopTop);
    ldaxr(SubEmitSize, TMP2, MemSrc);
    stlxr(SubEmitSize, TMP4, Src, MemSrc);
    (void)cbnz(EmitSize, TMP4, &LoopTop);
    ubfm(EmitSize, GetReg(Node), TMP2, 0, IR::OpSizeAsBits(OpSize) - 1);
  }
}

DEF_OP(AtomicFetchAdd) {
  auto Op = IROp->C<IR::IROp_AtomicFetchAdd>();
  const auto EmitSize = ConvertSize(IROp);
  const auto SubEmitSize = ConvertSubRegSize8(IROp->Size);

  auto MemSrc = GetReg(Op->Addr);
  auto Src = GetReg(Op->Value);

  if (CTX->HostFeatures.SupportsAtomics) {
    ldaddal(SubEmitSize, Src, GetReg(Node), MemSrc);
  } else {
    ARMEmitter::BackwardLabel LoopTop;
    (void)Bind(&LoopTop);
    ldaxr(SubEmitSize, TMP2, MemSrc);
    add(EmitSize, TMP3, TMP2, Src);
    stlxr(SubEmitSize, TMP4, TMP3, MemSrc);
    (void)cbnz(EmitSize, TMP4, &LoopTop);
    mov(EmitSize, GetReg(Node), TMP2.R());
  }
}

DEF_OP(AtomicFetchSub) {
  auto Op = IROp->C<IR::IROp_AtomicFetchSub>();
  const auto EmitSize = ConvertSize(IROp);
  const auto SubEmitSize = ConvertSubRegSize8(IROp->Size);

  auto MemSrc = GetReg(Op->Addr);
  auto Src = GetReg(Op->Value);

  if (CTX->HostFeatures.SupportsAtomics) {
    neg(EmitSize, TMP2, Src);
    ldaddal(SubEmitSize, TMP2, GetReg(Node), MemSrc);
  } else {
    ARMEmitter::BackwardLabel LoopTop;
    (void)Bind(&LoopTop);
    ldaxr(SubEmitSize, TMP2, MemSrc);
    sub(EmitSize, TMP3, TMP2, Src);
    stlxr(SubEmitSize, TMP4, TMP3, MemSrc);
    (void)cbnz(EmitSize, TMP4, &LoopTop);
    mov(EmitSize, GetReg(Node), TMP2.R());
  }
}

DEF_OP(AtomicFetchAnd) {
  auto Op = IROp->C<IR::IROp_AtomicFetchAnd>();
  const auto EmitSize = ConvertSize(IROp);
  const auto SubEmitSize = ConvertSubRegSize8(IROp->Size);

  auto MemSrc = GetReg(Op->Addr);
  auto Src = GetReg(Op->Value);

  if (CTX->HostFeatures.SupportsAtomics) {
    mvn(EmitSize, TMP2, Src);
    ldclral(SubEmitSize, TMP2, GetReg(Node), MemSrc);
  } else {
    ARMEmitter::BackwardLabel LoopTop;
    (void)Bind(&LoopTop);
    ldaxr(SubEmitSize, TMP2, MemSrc);
    and_(EmitSize, TMP3, TMP2, Src);
    stlxr(SubEmitSize, TMP4, TMP3, MemSrc);
    (void)cbnz(EmitSize, TMP4, &LoopTop);
    mov(EmitSize, GetReg(Node), TMP2.R());
  }
}

DEF_OP(AtomicFetchCLR) {
  auto Op = IROp->C<IR::IROp_AtomicFetchCLR>();
  const auto EmitSize = ConvertSize(IROp);
  const auto SubEmitSize = ConvertSubRegSize8(IROp->Size);

  auto MemSrc = GetReg(Op->Addr);
  auto Src = GetReg(Op->Value);

  if (CTX->HostFeatures.SupportsAtomics) {
    ldclral(SubEmitSize, Src, GetReg(Node), MemSrc);
  } else {
    ARMEmitter::BackwardLabel LoopTop;
    (void)Bind(&LoopTop);
    ldaxr(SubEmitSize, TMP2, MemSrc);
    bic(EmitSize, TMP3, TMP2, Src);
    stlxr(SubEmitSize, TMP4, TMP3, MemSrc);
    (void)cbnz(EmitSize, TMP4, &LoopTop);
    mov(EmitSize, GetReg(Node), TMP2.R());
  }
}

DEF_OP(AtomicFetchOr) {
  auto Op = IROp->C<IR::IROp_AtomicFetchOr>();
  const auto EmitSize = ConvertSize(IROp);
  const auto SubEmitSize = ConvertSubRegSize8(IROp->Size);

  auto MemSrc = GetReg(Op->Addr);
  auto Src = GetReg(Op->Value);

  if (CTX->HostFeatures.SupportsAtomics) {
    ldsetal(SubEmitSize, Src, GetReg(Node), MemSrc);
  } else {
    ARMEmitter::BackwardLabel LoopTop;
    (void)Bind(&LoopTop);
    ldaxr(SubEmitSize, TMP2, MemSrc);
    orr(EmitSize, TMP3, TMP2, Src);
    stlxr(SubEmitSize, TMP4, TMP3, MemSrc);
    (void)cbnz(EmitSize, TMP4, &LoopTop);
    mov(EmitSize, GetReg(Node), TMP2.R());
  }
}

DEF_OP(AtomicFetchXor) {
  auto Op = IROp->C<IR::IROp_AtomicFetchXor>();
  const auto EmitSize = ConvertSize(IROp);
  const auto SubEmitSize = ConvertSubRegSize8(IROp->Size);

  auto MemSrc = GetReg(Op->Addr);
  auto Src = GetReg(Op->Value);

  if (CTX->HostFeatures.SupportsAtomics) {
    ldeoral(SubEmitSize, Src, GetReg(Node), MemSrc);
  } else {
    ARMEmitter::BackwardLabel LoopTop;
    (void)Bind(&LoopTop);
    ldaxr(SubEmitSize, TMP2, MemSrc);
    eor(EmitSize, TMP3, TMP2, Src);
    stlxr(SubEmitSize, TMP4, TMP3, MemSrc);
    (void)cbnz(EmitSize, TMP4, &LoopTop);
    mov(EmitSize, GetReg(Node), TMP2.R());
  }
}

DEF_OP(AtomicFetchNeg) {
  auto Op = IROp->C<IR::IROp_AtomicFetchNeg>();
  const auto EmitSize = ConvertSize(IROp);
  const auto SubEmitSize = ConvertSubRegSize8(IROp->Size);

  auto MemSrc = GetReg(Op->Addr);

  if (CTX->HostFeatures.SupportsAtomics) {
    // Use a CAS loop to avoid needing to emulate unaligned LLSC atomics
    ldr(SubEmitSize, TMP2, MemSrc);
    ARMEmitter::BackwardLabel LoopTop;
    (void)Bind(&LoopTop);
    mov(EmitSize, TMP4, TMP2);
    neg(EmitSize, TMP3, TMP2);
    casal(SubEmitSize, TMP2, TMP3, MemSrc);
    sub(EmitSize, TMP3, TMP2, TMP4);
    (void)cbnz(EmitSize, TMP3, &LoopTop);
    mov(EmitSize, GetReg(Node), TMP2.R());
  } else {
    ARMEmitter::BackwardLabel LoopTop;
    (void)Bind(&LoopTop);
    ldaxr(SubEmitSize, TMP2, MemSrc);
    neg(EmitSize, TMP3, TMP2);
    stlxr(SubEmitSize, TMP4, TMP3, MemSrc);
    (void)cbnz(EmitSize, TMP4, &LoopTop);
    mov(EmitSize, GetReg(Node), TMP2.R());
  }
}

DEF_OP(TelemetrySetValue) {
#ifndef FEX_DISABLE_TELEMETRY
  auto Op = IROp->C<IR::IROp_TelemetrySetValue>();
  auto Src = GetReg(Op->Value);

  ldr(TMP2, STATE_PTR(CpuStateFrame, Pointers.Common.TelemetryValueAddresses[Op->TelemetryValueIndex]));

  // Cortex fuses cmp+cset.
  cmp(ARMEmitter::Size::i32Bit, Src, 0);
  cset(ARMEmitter::Size::i32Bit, TMP1, ARMEmitter::Condition::CC_NE);

  if (CTX->HostFeatures.SupportsAtomics) {
    stsetl(ARMEmitter::SubRegSize::i64Bit, TMP1, TMP2);
  } else {
    ARMEmitter::BackwardLabel LoopTop;
    (void)Bind(&LoopTop);
    ldaxr(ARMEmitter::SubRegSize::i64Bit, TMP3, TMP2);
    orr(ARMEmitter::Size::i32Bit, TMP3, TMP3, Src);
    stlxr(ARMEmitter::SubRegSize::i64Bit, TMP3, TMP3, TMP2);
    (void)cbnz(ARMEmitter::Size::i32Bit, TMP3, &LoopTop);
  }
#endif
}

} // namespace FEXCore::CPU
