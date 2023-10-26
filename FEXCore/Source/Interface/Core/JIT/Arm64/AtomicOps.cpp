// SPDX-License-Identifier: MIT
/*
$info$
tags: backend|arm64
$end_info$
*/

#include "Interface/Context/Context.h"
#include "Interface/Core/ArchHelpers/CodeEmitter/Emitter.h"
#include "Interface/Core/Dispatcher/Dispatcher.h"
#include "Interface/Core/JIT/Arm64/JITClass.h"

namespace FEXCore::CPU {
#define DEF_OP(x) void Arm64JITCore::Op_##x(IR::IROp_Header const *IROp, IR::NodeID Node)
DEF_OP(CASPair) {
  auto Op = IROp->C<IR::IROp_CASPair>();
  LOGMAN_THROW_AA_FMT(IROp->ElementSize == 4 || IROp->ElementSize == 8, "Wrong element size");
  // Size is the size of each pair element
  auto Dst = GetRegPair(Node);
  auto Expected = GetRegPair(Op->Expected.ID());
  auto Desired = GetRegPair(Op->Desired.ID());
  auto MemSrc = GetReg(Op->Addr.ID());

  const auto EmitSize = IROp->ElementSize == 8 ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;
  if (CTX->HostFeatures.SupportsAtomics) {
    mov(EmitSize, TMP3, Expected.first);
    mov(EmitSize, TMP4, Expected.second);

    caspal(EmitSize, TMP3, TMP4, Desired.first, Desired.second, MemSrc);
    mov(EmitSize, Dst.first, TMP3.R());
    mov(EmitSize, Dst.second, TMP4.R());
  }
  else {
    ARMEmitter::BackwardLabel LoopTop;
    ARMEmitter::ForwardLabel LoopNotExpected;
    ARMEmitter::ForwardLabel LoopExpected;
    Bind(&LoopTop);

    ldaxp(EmitSize, TMP2, TMP3, MemSrc);
    cmp(EmitSize, TMP2, Expected.first);
    ccmp(EmitSize, TMP3, Expected.second, ARMEmitter::StatusFlags::None, ARMEmitter::Condition::CC_EQ);
    b(ARMEmitter::Condition::CC_NE, &LoopNotExpected);
    stlxp(EmitSize, TMP2, Desired.first, Desired.second, MemSrc);
    cbnz(EmitSize, TMP2, &LoopTop);
    mov(EmitSize, Dst.first, Expected.first);
    mov(EmitSize, Dst.second, Expected.second);

    b(&LoopExpected);

      Bind(&LoopNotExpected);
      mov(EmitSize, Dst.first, TMP2.R());
      mov(EmitSize, Dst.second, TMP3.R());
      // exclusive monitor needs to be cleared here
      // Might have hit the case where ldaxr was hit but stlxr wasn't
      clrex();
    Bind(&LoopExpected);
  }
}

DEF_OP(CAS) {
  auto Op = IROp->C<IR::IROp_CAS>();
  uint8_t OpSize = IROp->Size;
  LOGMAN_THROW_AA_FMT(OpSize == 8 || OpSize == 4 || OpSize == 2 || OpSize == 1, "Unexpected CAS size");
  // DataSrc = *Src1
  // if (DataSrc == Src3) { *Src1 == Src2; } Src2 = DataSrc
  // This will write to memory! Careful!

  auto Expected = GetReg(Op->Expected.ID());
  auto Desired = GetReg(Op->Desired.ID());
  auto MemSrc = GetReg(Op->Addr.ID());

  const auto EmitSize = OpSize == 8 ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;
  const auto SubEmitSize = OpSize == 8 ? ARMEmitter::SubRegSize::i64Bit :
    OpSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    OpSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    OpSize == 1 ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

  if (CTX->HostFeatures.SupportsAtomics) {
    mov(EmitSize, TMP2, Expected);
    casal(SubEmitSize, TMP2, Desired, MemSrc);
    mov(EmitSize, GetReg(Node), TMP2.R());
  }
  else {
    ARMEmitter::BackwardLabel LoopTop;
    ARMEmitter::ForwardLabel LoopNotExpected;
    ARMEmitter::ForwardLabel LoopExpected;
    Bind(&LoopTop);
    ldaxr(SubEmitSize, TMP2, MemSrc);
    if (OpSize == 1) {
      cmp(EmitSize, TMP2, Expected, ARMEmitter::ExtendedType::UXTB, 0);
    }
    else if (OpSize == 2) {
      cmp(EmitSize, TMP2, Expected, ARMEmitter::ExtendedType::UXTH, 0);
    }
    else {
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
  uint8_t OpSize = IROp->Size;
  LOGMAN_THROW_AA_FMT(OpSize == 8 || OpSize == 4 || OpSize == 2 || OpSize == 1, "Unexpected CAS size");

  auto MemSrc = GetReg(Op->Addr.ID());
  auto Src = GetReg(Op->Value.ID());

  const auto EmitSize = OpSize == 8 ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;
  const auto SubEmitSize = OpSize == 8 ? ARMEmitter::SubRegSize::i64Bit :
    OpSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    OpSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    OpSize == 1 ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

  if (CTX->HostFeatures.SupportsAtomics) {
    staddl(SubEmitSize, Src, MemSrc);
  }
  else {
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
  uint8_t OpSize = IROp->Size;
  LOGMAN_THROW_AA_FMT(OpSize == 8 || OpSize == 4 || OpSize == 2 || OpSize == 1, "Unexpected CAS size");

  auto MemSrc = GetReg(Op->Addr.ID());
  auto Src = GetReg(Op->Value.ID());

  const auto EmitSize = OpSize == 8 ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;
  const auto SubEmitSize = OpSize == 8 ? ARMEmitter::SubRegSize::i64Bit :
    OpSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    OpSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    OpSize == 1 ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

  if (CTX->HostFeatures.SupportsAtomics) {
    neg(EmitSize, TMP2, Src);
    staddl(SubEmitSize, TMP2, MemSrc);
  }
  else {
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
  uint8_t OpSize = IROp->Size;
  LOGMAN_THROW_AA_FMT(OpSize == 8 || OpSize == 4 || OpSize == 2 || OpSize == 1, "Unexpected CAS size");

  auto MemSrc = GetReg(Op->Addr.ID());
  auto Src = GetReg(Op->Value.ID());

  const auto EmitSize = OpSize == 8 ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;
  const auto SubEmitSize = OpSize == 8 ? ARMEmitter::SubRegSize::i64Bit :
    OpSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    OpSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    OpSize == 1 ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

  if (CTX->HostFeatures.SupportsAtomics) {
    mvn(EmitSize, TMP2, Src);
    stclrl(SubEmitSize, TMP2, MemSrc);
  }
  else {
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
  uint8_t OpSize = IROp->Size;
  LOGMAN_THROW_AA_FMT(OpSize == 8 || OpSize == 4 || OpSize == 2 || OpSize == 1, "Unexpected CAS size");

  auto MemSrc = GetReg(Op->Addr.ID());
  auto Src = GetReg(Op->Value.ID());

  const auto EmitSize = OpSize == 8 ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;
  const auto SubEmitSize = OpSize == 8 ? ARMEmitter::SubRegSize::i64Bit :
    OpSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    OpSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    OpSize == 1 ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

  if (CTX->HostFeatures.SupportsAtomics) {
    stclrl(SubEmitSize, Src, MemSrc);
  }
  else {
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
  uint8_t OpSize = IROp->Size;
  LOGMAN_THROW_AA_FMT(OpSize == 8 || OpSize == 4 || OpSize == 2 || OpSize == 1, "Unexpected CAS size");

  auto MemSrc = GetReg(Op->Addr.ID());
  auto Src = GetReg(Op->Value.ID());

  const auto EmitSize = OpSize == 8 ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;
  const auto SubEmitSize = OpSize == 8 ? ARMEmitter::SubRegSize::i64Bit :
    OpSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    OpSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    OpSize == 1 ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

  if (CTX->HostFeatures.SupportsAtomics) {
    stsetl(SubEmitSize, Src, MemSrc);
  }
  else {
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
  uint8_t OpSize = IROp->Size;
  LOGMAN_THROW_AA_FMT(OpSize == 8 || OpSize == 4 || OpSize == 2 || OpSize == 1, "Unexpected CAS size");

  auto MemSrc = GetReg(Op->Addr.ID());
  auto Src = GetReg(Op->Value.ID());

  const auto EmitSize = OpSize == 8 ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;
  const auto SubEmitSize = OpSize == 8 ? ARMEmitter::SubRegSize::i64Bit :
    OpSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    OpSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    OpSize == 1 ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

  if (CTX->HostFeatures.SupportsAtomics) {
    steorl(SubEmitSize, Src, MemSrc);
  }
  else {
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
  uint8_t OpSize = IROp->Size;
  LOGMAN_THROW_AA_FMT(OpSize == 8 || OpSize == 4 || OpSize == 2 || OpSize == 1, "Unexpected CAS size");

  auto MemSrc = GetReg(Op->Addr.ID());

  const auto EmitSize = OpSize == 8 ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;
  const auto SubEmitSize = OpSize == 8 ? ARMEmitter::SubRegSize::i64Bit :
    OpSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    OpSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    OpSize == 1 ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

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

  const auto EmitSize = OpSize == 8 ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;
  const auto SubEmitSize = OpSize == 8 ? ARMEmitter::SubRegSize::i64Bit :
    OpSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    OpSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    OpSize == 1 ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

  if (CTX->HostFeatures.SupportsAtomics) {
    mov(EmitSize, TMP2, Src);
    ldswpal(SubEmitSize, TMP2, GetReg(Node), MemSrc);
  }
  else {
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
  uint8_t OpSize = IROp->Size;
  LOGMAN_THROW_AA_FMT(OpSize == 8 || OpSize == 4 || OpSize == 2 || OpSize == 1, "Unexpected CAS size");

  auto MemSrc = GetReg(Op->Addr.ID());
  auto Src = GetReg(Op->Value.ID());

  const auto EmitSize = OpSize == 8 ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;
  const auto SubEmitSize = OpSize == 8 ? ARMEmitter::SubRegSize::i64Bit :
    OpSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    OpSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    OpSize == 1 ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

  if (CTX->HostFeatures.SupportsAtomics) {
    ldaddal(SubEmitSize, Src, GetReg(Node), MemSrc);
  }
  else {
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
  uint8_t OpSize = IROp->Size;
  LOGMAN_THROW_AA_FMT(OpSize == 8 || OpSize == 4 || OpSize == 2 || OpSize == 1, "Unexpected CAS size");

  auto MemSrc = GetReg(Op->Addr.ID());
  auto Src = GetReg(Op->Value.ID());

  const auto EmitSize = OpSize == 8 ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;
  const auto SubEmitSize = OpSize == 8 ? ARMEmitter::SubRegSize::i64Bit :
    OpSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    OpSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    OpSize == 1 ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

  if (CTX->HostFeatures.SupportsAtomics) {
    neg(EmitSize, TMP2, Src);
    ldaddal(SubEmitSize, TMP2, GetReg(Node), MemSrc);
  }
  else {
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
  uint8_t OpSize = IROp->Size;
  LOGMAN_THROW_AA_FMT(OpSize == 8 || OpSize == 4 || OpSize == 2 || OpSize == 1, "Unexpected CAS size");

  auto MemSrc = GetReg(Op->Addr.ID());
  auto Src = GetReg(Op->Value.ID());

  const auto EmitSize = OpSize == 8 ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;
  const auto SubEmitSize = OpSize == 8 ? ARMEmitter::SubRegSize::i64Bit :
    OpSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    OpSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    OpSize == 1 ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

  if (CTX->HostFeatures.SupportsAtomics) {
    mvn(EmitSize, TMP2, Src);
    ldclral(SubEmitSize, TMP2, GetReg(Node), MemSrc);
  }
  else {
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
  uint8_t OpSize = IROp->Size;
  LOGMAN_THROW_AA_FMT(OpSize == 8 || OpSize == 4 || OpSize == 2 || OpSize == 1, "Unexpected CAS size");

  auto MemSrc = GetReg(Op->Addr.ID());
  auto Src = GetReg(Op->Value.ID());

  const auto EmitSize = OpSize == 8 ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;
  const auto SubEmitSize = OpSize == 8 ? ARMEmitter::SubRegSize::i64Bit :
    OpSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    OpSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    OpSize == 1 ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

  if (CTX->HostFeatures.SupportsAtomics) {
    ldclral(SubEmitSize, Src, GetReg(Node), MemSrc);
  }
  else {
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
  uint8_t OpSize = IROp->Size;
  LOGMAN_THROW_AA_FMT(OpSize == 8 || OpSize == 4 || OpSize == 2 || OpSize == 1, "Unexpected CAS size");

  auto MemSrc = GetReg(Op->Addr.ID());
  auto Src = GetReg(Op->Value.ID());

  const auto EmitSize = OpSize == 8 ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;
  const auto SubEmitSize = OpSize == 8 ? ARMEmitter::SubRegSize::i64Bit :
    OpSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    OpSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    OpSize == 1 ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

  if (CTX->HostFeatures.SupportsAtomics) {
    ldsetal(SubEmitSize, Src, GetReg(Node), MemSrc);
  }
  else {
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
  uint8_t OpSize = IROp->Size;
  LOGMAN_THROW_AA_FMT(OpSize == 8 || OpSize == 4 || OpSize == 2 || OpSize == 1, "Unexpected CAS size");

  auto MemSrc = GetReg(Op->Addr.ID());
  auto Src = GetReg(Op->Value.ID());

  const auto EmitSize = OpSize == 8 ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;
  const auto SubEmitSize = OpSize == 8 ? ARMEmitter::SubRegSize::i64Bit :
    OpSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    OpSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    OpSize == 1 ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

  if (CTX->HostFeatures.SupportsAtomics) {
    ldeoral(SubEmitSize, Src, GetReg(Node), MemSrc);
  }
  else {
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
  uint8_t OpSize = IROp->Size;
  LOGMAN_THROW_AA_FMT(OpSize == 8 || OpSize == 4 || OpSize == 2 || OpSize == 1, "Unexpected CAS size");

  auto MemSrc = GetReg(Op->Addr.ID());

  const auto EmitSize = OpSize == 8 ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;
  const auto SubEmitSize = OpSize == 8 ? ARMEmitter::SubRegSize::i64Bit :
    OpSize == 4 ? ARMEmitter::SubRegSize::i32Bit :
    OpSize == 2 ? ARMEmitter::SubRegSize::i16Bit :
    OpSize == 1 ? ARMEmitter::SubRegSize::i8Bit : ARMEmitter::SubRegSize::i8Bit;

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
  }
  else {
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
}

