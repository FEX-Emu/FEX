// SPDX-License-Identifier: MIT
/*
$info$
tags: frontend|x86-to-ir, opcodes|dispatcher-implementations
desc: Handles x86/64 Vector instructions to IR
$end_info$
*/

#include "Interface/Context/Context.h"
#include "Interface/Core/OpcodeDispatcher.h"
#include "Interface/Core/X86Tables/X86Tables.h"
#include "Interface/IR/IR.h"

#include <FEXCore/Config/Config.h>
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/Utils/LogManager.h>

#include <array>
#include <bit>
#include <cstdint>
#include <stddef.h>

namespace FEXCore::IR {
#define OpcodeArgs [[maybe_unused]] FEXCore::X86Tables::DecodedOp Op

void OpDispatchBuilder::MOVVectorAlignedOp(OpcodeArgs) {
  if (Op->Dest.IsGPR() && Op->Src[0].IsGPR() && Op->Dest.Data.GPR.GPR == Op->Src[0].Data.GPR.GPR) {
    // Nop
    return;
  }
  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  StoreResult(FPRClass, Op, Src, OpSize::iInvalid);
}

void OpDispatchBuilder::MOVVectorUnalignedOp(OpcodeArgs) {
  if (Op->Dest.IsGPR() && Op->Src[0].IsGPR() && Op->Dest.Data.GPR.GPR == Op->Src[0].Data.GPR.GPR) {
    // Nop
    return;
  }
  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, {.Align = OpSize::i8Bit});
  StoreResult(FPRClass, Op, Src, OpSize::i8Bit);
}

void OpDispatchBuilder::MOVVectorNTOp(OpcodeArgs) {
  const auto Size = OpSizeFromDst(Op);

  if (Op->Dest.IsGPR() && Size >= OpSize::i128Bit) {
    ///< MOVNTDQA load non-temporal comes from SSE4.1 and is extended by AVX/AVX2.
    Ref SrcAddr = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, {.LoadData = false});
    auto Src = _VLoadNonTemporal(Size, SrcAddr, 0);

    StoreResult(FPRClass, Op, Src, OpSize::i8Bit, MemoryAccessType::STREAM);
  } else if (Op->Dest.IsGPR()) {
    Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, {.Align = OpSize::i8Bit, .AccessType = MemoryAccessType::STREAM});
    StoreResult(FPRClass, Op, Src, OpSize::i8Bit, MemoryAccessType::STREAM);
  } else {
    LOGMAN_THROW_A_FMT(!Op->Dest.IsGPR(), "Destination can't be GPR for non-temporal stores");
    Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, {.Align = OpSize::i8Bit, .AccessType = MemoryAccessType::STREAM});
    if (Size < OpSize::i128Bit) {
      // Normal streaming store if less than 128-bit
      // XMM Scalar 32-bit and 64-bit comes from SSE4a MOVNTSS, MOVNTSD
      // MMX 64-bit comes from MOVNTQ
      StoreResult(FPRClass, Op, Src, OpSize::i8Bit, MemoryAccessType::STREAM);
    } else {
      Ref Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.LoadData = false});

      // Single store non-temporal for larger operations.
      _VStoreNonTemporal(Size, Src, Dest, 0);
    }
  }
}

void OpDispatchBuilder::VMOVAPS_VMOVAPDOp(OpcodeArgs) {
  const auto SrcSize = GetSrcSize(Op);
  const auto Is128Bit = SrcSize == Core::CPUState::XMM_SSE_REG_SIZE;

  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

  if (Is128Bit && Op->Dest.IsGPR()) {
    Src = _VMov(OpSize::i128Bit, Src);
  }
  StoreResult(FPRClass, Op, Src, OpSize::iInvalid);
}

void OpDispatchBuilder::VMOVUPS_VMOVUPDOp(OpcodeArgs) {
  const auto SrcSize = GetSrcSize(Op);
  const auto Is128Bit = SrcSize == Core::CPUState::XMM_SSE_REG_SIZE;

  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, {.Align = OpSize::i8Bit});

  if (Is128Bit && Op->Dest.IsGPR()) {
    Src = _VMov(OpSize::i128Bit, Src);
  }
  StoreResult(FPRClass, Op, Src, OpSize::i8Bit);
}

void OpDispatchBuilder::MOVHPDOp(OpcodeArgs) {
  if (Op->Dest.IsGPR()) {
    if (Op->Src[0].IsGPR()) {
      // MOVLHPS between two vector registers.
      Ref Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
      Ref Dest = LoadSource_WithOpSize(FPRClass, Op, Op->Dest, OpSize::i128Bit, Op->Flags);
      auto Result = _VInsElement(OpSize::i128Bit, OpSize::i64Bit, 1, 0, Dest, Src);
      StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
    } else {
      // If the destination is a GPR then the source is memory
      // xmm1[127:64] = src
      Ref Src = MakeSegmentAddress(Op, Op->Src[0]);
      Ref Dest = LoadSource_WithOpSize(FPRClass, Op, Op->Dest, OpSize::i128Bit, Op->Flags);
      auto Result = _VLoadVectorElement(OpSize::i128Bit, OpSize::i64Bit, Dest, 1, Src);
      StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
    }
  } else {
    // In this case memory is the destination and the high bits of the XMM are source
    // Mem64 = xmm1[127:64]
    Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
    Ref Dest = MakeSegmentAddress(Op, Op->Dest);
    _VStoreVectorElement(OpSize::i128Bit, OpSize::i64Bit, Src, 1, Dest);
  }
}

void OpDispatchBuilder::VMOVHPOp(OpcodeArgs) {
  if (Op->Dest.IsGPR()) {
    Ref Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, {.Align = OpSize::i128Bit});
    Ref Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags, {.Align = OpSize::i64Bit});
    Ref Result = _VInsElement(OpSize::i128Bit, OpSize::i64Bit, 1, 0, Src1, Src2);

    StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
  } else {
    Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, {.Align = OpSize::i128Bit});
    Ref Result = _VInsElement(OpSize::i128Bit, OpSize::i64Bit, 0, 1, Src, Src);
    StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Result, OpSize::i64Bit, OpSize::i64Bit);
  }
}

void OpDispatchBuilder::MOVLPOp(OpcodeArgs) {
  if (Op->Dest.IsGPR()) {
    // xmm, xmm is movhlps special case
    if (Op->Src[0].IsGPR()) {
      Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, {.Align = OpSize::i128Bit});
      Ref Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, {.Align = OpSize::i128Bit});
      auto Result = _VInsElement(OpSize::i128Bit, OpSize::i64Bit, 0, 1, Dest, Src);
      StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Result, OpSize::i128Bit, OpSize::i128Bit);
    } else {
      const auto DstSize = OpSizeFromDst(Op);
      Ref Src = MakeSegmentAddress(Op, Op->Src[0]);
      Ref Dest = LoadSource_WithOpSize(FPRClass, Op, Op->Dest, DstSize, Op->Flags);
      auto Result = _VLoadVectorElement(OpSize::i128Bit, OpSize::i64Bit, Dest, 0, Src);
      StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
    }
  } else {
    Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, {.Align = OpSize::i64Bit});
    StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Src, OpSize::i64Bit, OpSize::i64Bit);
  }
}

void OpDispatchBuilder::VMOVLPOp(OpcodeArgs) {
  Ref Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, {.Align = OpSize::i128Bit});

  if (!Op->Dest.IsGPR()) {
    ///< VMOVLPS/PD mem64, xmm1
    StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Src1, OpSize::i64Bit, OpSize::i64Bit);
  } else if (!Op->Src[1].IsGPR()) {
    ///< VMOVLPS/PD xmm1, xmm2, mem64
    // Bits[63:0] come from Src2[63:0]
    // Bits[127:64] come from Src1[127:64]
    Ref Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags, {.Align = OpSize::i64Bit});
    Ref Result = _VInsElement(OpSize::i128Bit, OpSize::i64Bit, 1, 1, Src2, Src1);
    StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
  } else {
    ///< VMOVHLPS/PD xmm1, xmm2, xmm3
    Ref Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags, {.Align = OpSize::i128Bit});
    Ref Result = _VInsElement(OpSize::i128Bit, OpSize::i64Bit, 0, 1, Src1, Src2);
    StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
  }
}

void OpDispatchBuilder::VMOVSHDUPOp(OpcodeArgs) {
  const auto SrcSize = GetSrcSize(Op);
  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Result = _VTrn2(SrcSize, OpSize::i32Bit, Src, Src);
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::VMOVSLDUPOp(OpcodeArgs) {
  const auto SrcSize = GetSrcSize(Op);
  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Result = _VTrn(SrcSize, OpSize::i32Bit, Src, Src);
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::MOVScalarOpImpl(OpcodeArgs, IR::OpSize ElementSize) {
  if (Op->Dest.IsGPR() && Op->Src[0].IsGPR()) {
    // MOVSS/SD xmm1, xmm2
    Ref Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
    Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
    auto Result = _VInsElement(OpSize::i128Bit, ElementSize, 0, 0, Dest, Src);
    StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
  } else if (Op->Dest.IsGPR()) {
    // MOVSS/SD xmm1, mem32/mem64
    // xmm1[127:0] <- zext(mem32/mem64)
    Ref Src = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], ElementSize, Op->Flags);
    StoreResult(FPRClass, Op, Src, OpSize::iInvalid);
  } else {
    // MOVSS/SD mem32/mem64, xmm1
    Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
    StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Src, ElementSize, OpSize::iInvalid);
  }
}

void OpDispatchBuilder::MOVSSOp(OpcodeArgs) {
  MOVScalarOpImpl(Op, OpSize::i32Bit);
}

void OpDispatchBuilder::MOVSDOp(OpcodeArgs) {
  MOVScalarOpImpl(Op, OpSize::i64Bit);
}

void OpDispatchBuilder::VMOVScalarOpImpl(OpcodeArgs, IR::OpSize ElementSize) {
  if (Op->Dest.IsGPR() && Op->Src[0].IsGPR() && Op->Src[1].IsGPR()) {
    // VMOVSS/SD xmm1, xmm2, xmm3
    Ref Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
    Ref Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);
    Ref Result = _VInsElement(OpSize::i128Bit, ElementSize, 0, 0, Src1, Src2);
    StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
  } else if (Op->Dest.IsGPR()) {
    // VMOVSS/SD xmm1, mem32/mem64
    Ref Src = LoadSource_WithOpSize(FPRClass, Op, Op->Src[1], ElementSize, Op->Flags);
    StoreResult(FPRClass, Op, Src, OpSize::iInvalid);
  } else {
    // VMOVSS/SD mem32/mem64, xmm1
    Ref Src = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);
    StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Src, ElementSize, OpSize::iInvalid);
  }
}

void OpDispatchBuilder::VMOVSDOp(OpcodeArgs) {
  VMOVScalarOpImpl(Op, OpSize::i64Bit);
}

void OpDispatchBuilder::VMOVSSOp(OpcodeArgs) {
  VMOVScalarOpImpl(Op, OpSize::i32Bit);
}

void OpDispatchBuilder::VectorALUOp(OpcodeArgs, IROps IROp, size_t ElementSize) {
  const auto Size = GetSrcSize(Op);
  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);

  DeriveOp(ALUOp, IROp, _VAdd(Size, ElementSize, Dest, Src));

  StoreResult(FPRClass, Op, ALUOp, OpSize::iInvalid);
}

void OpDispatchBuilder::VectorXOROp(OpcodeArgs) {
  const auto Size = GetSrcSize(Op);

  // Special case for vector xor with itself being the optimal way for x86 to zero vector registers.
  if (Op->Dest.IsGPR() && Op->Src[0].IsGPR() && Op->Dest.Data.GPR.GPR == Op->Src[0].Data.GPR.GPR) {
    const auto ZeroRegister = LoadZeroVector(Size);
    StoreResult(FPRClass, Op, ZeroRegister, OpSize::iInvalid);
    return;
  }

  ///< Regular code path
  VectorALUOp(Op, OP_VXOR, Size);
}

void OpDispatchBuilder::AVXVectorALUOp(OpcodeArgs, IROps IROp, size_t ElementSize) {
  const auto Size = GetSrcSize(Op);

  Ref Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);

  DeriveOp(ALUOp, IROp, _VAdd(Size, ElementSize, Src1, Src2));

  StoreResult(FPRClass, Op, ALUOp, OpSize::iInvalid);
}

void OpDispatchBuilder::AVXVectorXOROp(OpcodeArgs) {
  // Special case for vector xor with itself being the optimal way for x86 to zero vector registers.
  if (Op->Src[0].IsGPR() && Op->Src[1].IsGPR() && Op->Src[0].Data.GPR.GPR == Op->Src[1].Data.GPR.GPR) {
    const auto DstSize = GetDstSize(Op);
    const auto ZeroRegister = LoadZeroVector(DstSize);
    StoreResult(FPRClass, Op, ZeroRegister, OpSize::iInvalid);
    return;
  }

  ///< Regular code path
  AVXVectorALUOp(Op, OP_VXOR, OpSize::i128Bit);
}

void OpDispatchBuilder::VectorALUROp(OpcodeArgs, IROps IROp, size_t ElementSize) {
  const auto Size = GetSrcSize(Op);
  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);

  DeriveOp(ALUOp, IROp, _VAdd(Size, ElementSize, Src, Dest));

  StoreResult(FPRClass, Op, ALUOp, OpSize::iInvalid);
}

Ref OpDispatchBuilder::VectorScalarInsertALUOpImpl(OpcodeArgs, IROps IROp, IR::OpSize DstSize, IR::OpSize ElementSize,
                                                   const X86Tables::DecodedOperand& Src1Op, const X86Tables::DecodedOperand& Src2Op,
                                                   bool ZeroUpperBits) {
  // We load the full vector width when dealing with a source vector,
  // so that we don't do any unnecessary zero extension to the scalar
  // element that we're going to operate on.
  const auto SrcSize = OpSizeFromSrc(Op);

  Ref Src1 = LoadSource_WithOpSize(FPRClass, Op, Src1Op, DstSize, Op->Flags);
  Ref Src2 = LoadSource_WithOpSize(FPRClass, Op, Src2Op, SrcSize, Op->Flags, {.AllowUpperGarbage = true});

  // If OpSize == ElementSize then it only does the lower scalar op
  DeriveOp(ALUOp, IROp, _VFAddScalarInsert(DstSize, ElementSize, Src1, Src2, ZeroUpperBits));
  return ALUOp;
}

template<IROps IROp, IR::OpSize ElementSize>
void OpDispatchBuilder::VectorScalarInsertALUOp(OpcodeArgs) {
  const auto DstSize = GetGuestVectorLength();
  auto Result = VectorScalarInsertALUOpImpl(Op, IROp, DstSize, ElementSize, Op->Dest, Op->Src[0], false);
  StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Result, DstSize, OpSize::iInvalid);
}

template void OpDispatchBuilder::VectorScalarInsertALUOp<IR::OP_VFADDSCALARINSERT, OpSize::i32Bit>(OpcodeArgs);
template void OpDispatchBuilder::VectorScalarInsertALUOp<IR::OP_VFADDSCALARINSERT, OpSize::i64Bit>(OpcodeArgs);
template void OpDispatchBuilder::VectorScalarInsertALUOp<IR::OP_VFSUBSCALARINSERT, OpSize::i32Bit>(OpcodeArgs);
template void OpDispatchBuilder::VectorScalarInsertALUOp<IR::OP_VFSUBSCALARINSERT, OpSize::i64Bit>(OpcodeArgs);
template void OpDispatchBuilder::VectorScalarInsertALUOp<IR::OP_VFMULSCALARINSERT, OpSize::i32Bit>(OpcodeArgs);
template void OpDispatchBuilder::VectorScalarInsertALUOp<IR::OP_VFMULSCALARINSERT, OpSize::i64Bit>(OpcodeArgs);
template void OpDispatchBuilder::VectorScalarInsertALUOp<IR::OP_VFDIVSCALARINSERT, OpSize::i32Bit>(OpcodeArgs);
template void OpDispatchBuilder::VectorScalarInsertALUOp<IR::OP_VFDIVSCALARINSERT, OpSize::i64Bit>(OpcodeArgs);
template void OpDispatchBuilder::VectorScalarInsertALUOp<IR::OP_VFMINSCALARINSERT, OpSize::i32Bit>(OpcodeArgs);
template void OpDispatchBuilder::VectorScalarInsertALUOp<IR::OP_VFMINSCALARINSERT, OpSize::i64Bit>(OpcodeArgs);
template void OpDispatchBuilder::VectorScalarInsertALUOp<IR::OP_VFMAXSCALARINSERT, OpSize::i32Bit>(OpcodeArgs);
template void OpDispatchBuilder::VectorScalarInsertALUOp<IR::OP_VFMAXSCALARINSERT, OpSize::i64Bit>(OpcodeArgs);

template<IROps IROp, IR::OpSize ElementSize>
void OpDispatchBuilder::AVXVectorScalarInsertALUOp(OpcodeArgs) {
  const auto DstSize = GetGuestVectorLength();
  auto Result = VectorScalarInsertALUOpImpl(Op, IROp, DstSize, ElementSize, Op->Src[0], Op->Src[1], true);
  StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Result, DstSize, OpSize::iInvalid);
}

template void OpDispatchBuilder::AVXVectorScalarInsertALUOp<IR::OP_VFADDSCALARINSERT, OpSize::i32Bit>(OpcodeArgs);
template void OpDispatchBuilder::AVXVectorScalarInsertALUOp<IR::OP_VFADDSCALARINSERT, OpSize::i64Bit>(OpcodeArgs);
template void OpDispatchBuilder::AVXVectorScalarInsertALUOp<IR::OP_VFSUBSCALARINSERT, OpSize::i32Bit>(OpcodeArgs);
template void OpDispatchBuilder::AVXVectorScalarInsertALUOp<IR::OP_VFSUBSCALARINSERT, OpSize::i64Bit>(OpcodeArgs);
template void OpDispatchBuilder::AVXVectorScalarInsertALUOp<IR::OP_VFMULSCALARINSERT, OpSize::i32Bit>(OpcodeArgs);
template void OpDispatchBuilder::AVXVectorScalarInsertALUOp<IR::OP_VFMULSCALARINSERT, OpSize::i64Bit>(OpcodeArgs);
template void OpDispatchBuilder::AVXVectorScalarInsertALUOp<IR::OP_VFDIVSCALARINSERT, OpSize::i32Bit>(OpcodeArgs);
template void OpDispatchBuilder::AVXVectorScalarInsertALUOp<IR::OP_VFDIVSCALARINSERT, OpSize::i64Bit>(OpcodeArgs);
template void OpDispatchBuilder::AVXVectorScalarInsertALUOp<IR::OP_VFMINSCALARINSERT, OpSize::i32Bit>(OpcodeArgs);
template void OpDispatchBuilder::AVXVectorScalarInsertALUOp<IR::OP_VFMINSCALARINSERT, OpSize::i64Bit>(OpcodeArgs);
template void OpDispatchBuilder::AVXVectorScalarInsertALUOp<IR::OP_VFMAXSCALARINSERT, OpSize::i32Bit>(OpcodeArgs);
template void OpDispatchBuilder::AVXVectorScalarInsertALUOp<IR::OP_VFMAXSCALARINSERT, OpSize::i64Bit>(OpcodeArgs);

Ref OpDispatchBuilder::VectorScalarUnaryInsertALUOpImpl(OpcodeArgs, IROps IROp, IR::OpSize DstSize, IR::OpSize ElementSize,
                                                        const X86Tables::DecodedOperand& Src1Op, const X86Tables::DecodedOperand& Src2Op,
                                                        bool ZeroUpperBits) {
  // We load the full vector width when dealing with a source vector,
  // so that we don't do any unnecessary zero extension to the scalar
  // element that we're going to operate on.
  const auto SrcSize = OpSizeFromSrc(Op);

  Ref Src1 = LoadSource_WithOpSize(FPRClass, Op, Src1Op, DstSize, Op->Flags);
  Ref Src2 = LoadSource_WithOpSize(FPRClass, Op, Src2Op, SrcSize, Op->Flags, {.AllowUpperGarbage = true});

  // If OpSize == ElementSize then it only does the lower scalar op
  DeriveOp(ALUOp, IROp, _VFSqrtScalarInsert(DstSize, ElementSize, Src1, Src2, ZeroUpperBits));
  return ALUOp;
}

template<IROps IROp, IR::OpSize ElementSize>
void OpDispatchBuilder::VectorScalarUnaryInsertALUOp(OpcodeArgs) {
  const auto DstSize = GetGuestVectorLength();
  auto Result = VectorScalarInsertALUOpImpl(Op, IROp, DstSize, ElementSize, Op->Dest, Op->Src[0], false);
  StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Result, DstSize, OpSize::iInvalid);
}

template void OpDispatchBuilder::VectorScalarUnaryInsertALUOp<IR::OP_VFSQRTSCALARINSERT, OpSize::i32Bit>(OpcodeArgs);
template void OpDispatchBuilder::VectorScalarUnaryInsertALUOp<IR::OP_VFSQRTSCALARINSERT, OpSize::i64Bit>(OpcodeArgs);

template void OpDispatchBuilder::VectorScalarUnaryInsertALUOp<IR::OP_VFRSQRTSCALARINSERT, OpSize::i32Bit>(OpcodeArgs);
template void OpDispatchBuilder::VectorScalarUnaryInsertALUOp<IR::OP_VFRSQRTSCALARINSERT, OpSize::i64Bit>(OpcodeArgs);

template void OpDispatchBuilder::VectorScalarUnaryInsertALUOp<IR::OP_VFRECPSCALARINSERT, OpSize::i32Bit>(OpcodeArgs);
template void OpDispatchBuilder::VectorScalarUnaryInsertALUOp<IR::OP_VFRECPSCALARINSERT, OpSize::i64Bit>(OpcodeArgs);

template<IROps IROp, IR::OpSize ElementSize>
void OpDispatchBuilder::AVXVectorScalarUnaryInsertALUOp(OpcodeArgs) {
  const auto DstSize = GetGuestVectorLength();
  auto Result = VectorScalarInsertALUOpImpl(Op, IROp, DstSize, ElementSize, Op->Src[0], Op->Src[1], true);
  StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Result, DstSize, OpSize::iInvalid);
}

template void OpDispatchBuilder::AVXVectorScalarUnaryInsertALUOp<IR::OP_VFSQRTSCALARINSERT, OpSize::i32Bit>(OpcodeArgs);
template void OpDispatchBuilder::AVXVectorScalarUnaryInsertALUOp<IR::OP_VFSQRTSCALARINSERT, OpSize::i64Bit>(OpcodeArgs);

template void OpDispatchBuilder::AVXVectorScalarUnaryInsertALUOp<IR::OP_VFRSQRTSCALARINSERT, OpSize::i32Bit>(OpcodeArgs);
template void OpDispatchBuilder::AVXVectorScalarUnaryInsertALUOp<IR::OP_VFRSQRTSCALARINSERT, OpSize::i64Bit>(OpcodeArgs);

template void OpDispatchBuilder::AVXVectorScalarUnaryInsertALUOp<IR::OP_VFRECPSCALARINSERT, OpSize::i32Bit>(OpcodeArgs);
template void OpDispatchBuilder::AVXVectorScalarUnaryInsertALUOp<IR::OP_VFRECPSCALARINSERT, OpSize::i64Bit>(OpcodeArgs);

void OpDispatchBuilder::InsertMMX_To_XMM_Vector_CVT_Int_To_Float(OpcodeArgs) {
  // We load the full vector width when dealing with a source vector,
  // so that we don't do any unnecessary zero extension to the scalar
  // element that we're going to operate on.
  const auto DstSize = GetGuestVectorLength();
  const auto SrcSize = Op->Src[0].IsGPR() ? OpSize::i64Bit : OpSizeFromSrc(Op);

  Ref Dest = LoadSource_WithOpSize(FPRClass, Op, Op->Dest, DstSize, Op->Flags);
  Ref Src = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], SrcSize, Op->Flags);

  // Always 32-bit.
  const auto ElementSize = OpSize::i32Bit;
  // Always signed
  Dest = _VSToFVectorInsert(IR::SizeToOpSize(DstSize), ElementSize, ElementSize, Dest, Src, true, false);

  StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Dest, DstSize, OpSize::iInvalid);
}

Ref OpDispatchBuilder::InsertCVTGPR_To_FPRImpl(OpcodeArgs, IR::OpSize DstSize, IR::OpSize DstElementSize, const X86Tables::DecodedOperand& Src1Op,
                                               const X86Tables::DecodedOperand& Src2Op, bool ZeroUpperBits) {
  // We load the full vector width when dealing with a source vector,
  // so that we don't do any unnecessary zero extension to the scalar
  // element that we're going to operate on.
  const auto SrcSize = GetSrcSize(Op);

  Ref Src1 = LoadSource_WithOpSize(FPRClass, Op, Src1Op, DstSize, Op->Flags);

  if (Src2Op.IsGPR()) {
    // If the source is a GPR then convert directly from the GPR.
    auto Src2 = LoadSource_WithOpSize(GPRClass, Op, Src2Op, CTX->GetGPROpSize(), Op->Flags);
    return _VSToFGPRInsert(IR::SizeToOpSize(DstSize), DstElementSize, SrcSize, Src1, Src2, ZeroUpperBits);
  } else if (SrcSize != DstElementSize) {
    // If the source is from memory but the Source size and destination size aren't the same,
    // then it is more optimal to load in to a GPR and convert between GPR->FPR.
    // ARM GPR->FPR conversion supports different size source and destinations while FPR->FPR doesn't.
    auto Src2 = LoadSource(GPRClass, Op, Src2Op, Op->Flags);
    return _VSToFGPRInsert(IR::SizeToOpSize(DstSize), DstElementSize, SrcSize, Src1, Src2, ZeroUpperBits);
  }

  // In the case of cvtsi2s{s,d} where the source and destination are the same size,
  // then it is more optimal to load in to the FPR register directly and convert there.
  auto Src2 = LoadSource(FPRClass, Op, Src2Op, Op->Flags);
  // Always signed
  return _VSToFVectorInsert(IR::SizeToOpSize(DstSize), DstElementSize, DstElementSize, Src1, Src2, false, ZeroUpperBits);
}

template<IR::OpSize DstElementSize>
void OpDispatchBuilder::InsertCVTGPR_To_FPR(OpcodeArgs) {
  const auto DstSize = GetGuestVectorLength();
  auto Result = InsertCVTGPR_To_FPRImpl(Op, DstSize, DstElementSize, Op->Dest, Op->Src[0], false);
  StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Result, DstSize, OpSize::iInvalid);
}

template void OpDispatchBuilder::InsertCVTGPR_To_FPR<OpSize::i32Bit>(OpcodeArgs);
template void OpDispatchBuilder::InsertCVTGPR_To_FPR<OpSize::i64Bit>(OpcodeArgs);

template<IR::OpSize DstElementSize>
void OpDispatchBuilder::AVXInsertCVTGPR_To_FPR(OpcodeArgs) {
  const auto DstSize = GetGuestVectorLength();
  Ref Result = InsertCVTGPR_To_FPRImpl(Op, DstSize, DstElementSize, Op->Src[0], Op->Src[1], true);
  StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Result, DstSize, OpSize::iInvalid);
}
template void OpDispatchBuilder::AVXInsertCVTGPR_To_FPR<OpSize::i32Bit>(OpcodeArgs);
template void OpDispatchBuilder::AVXInsertCVTGPR_To_FPR<OpSize::i64Bit>(OpcodeArgs);

Ref OpDispatchBuilder::InsertScalar_CVT_Float_To_FloatImpl(OpcodeArgs, IR::OpSize DstSize, IR::OpSize DstElementSize,
                                                           IR::OpSize SrcElementSize, const X86Tables::DecodedOperand& Src1Op,
                                                           const X86Tables::DecodedOperand& Src2Op, bool ZeroUpperBits) {

  // We load the full vector width when dealing with a source vector,
  // so that we don't do any unnecessary zero extension to the scalar
  // element that we're going to operate on.
  const auto SrcSize = OpSizeFromSrc(Op);

  Ref Src1 = LoadSource_WithOpSize(FPRClass, Op, Src1Op, DstSize, Op->Flags);
  Ref Src2 = LoadSource_WithOpSize(FPRClass, Op, Src2Op, SrcSize, Op->Flags, {.AllowUpperGarbage = true});

  return _VFToFScalarInsert(IR::SizeToOpSize(DstSize), DstElementSize, SrcElementSize, Src1, Src2, ZeroUpperBits);
}

template<IR::OpSize DstElementSize, IR::OpSize SrcElementSize>
void OpDispatchBuilder::InsertScalar_CVT_Float_To_Float(OpcodeArgs) {
  const auto DstSize = GetGuestVectorLength();
  Ref Result = InsertScalar_CVT_Float_To_FloatImpl(Op, DstSize, DstElementSize, SrcElementSize, Op->Dest, Op->Src[0], false);
  StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Result, DstSize, OpSize::iInvalid);
}

template void OpDispatchBuilder::InsertScalar_CVT_Float_To_Float<OpSize::i32Bit, OpSize::i64Bit>(OpcodeArgs);
template void OpDispatchBuilder::InsertScalar_CVT_Float_To_Float<OpSize::i64Bit, OpSize::i32Bit>(OpcodeArgs);

template<IR::OpSize DstElementSize, IR::OpSize SrcElementSize>
void OpDispatchBuilder::AVXInsertScalar_CVT_Float_To_Float(OpcodeArgs) {
  const auto DstSize = GetGuestVectorLength();
  Ref Result = InsertScalar_CVT_Float_To_FloatImpl(Op, DstSize, DstElementSize, SrcElementSize, Op->Src[0], Op->Src[1], true);
  StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Result, DstSize, OpSize::iInvalid);
}

template void OpDispatchBuilder::AVXInsertScalar_CVT_Float_To_Float<OpSize::i32Bit, OpSize::i64Bit>(OpcodeArgs);
template void OpDispatchBuilder::AVXInsertScalar_CVT_Float_To_Float<OpSize::i64Bit, OpSize::i32Bit>(OpcodeArgs);

RoundType OpDispatchBuilder::TranslateRoundType(uint8_t Mode) {
  const uint64_t RoundControlSource = (Mode >> 2) & 1;
  uint64_t RoundControl = Mode & 0b11;

  static constexpr std::array SourceModes = {
    FEXCore::IR::Round_Nearest,
    FEXCore::IR::Round_Negative_Infinity,
    FEXCore::IR::Round_Positive_Infinity,
    FEXCore::IR::Round_Towards_Zero,
  };

  return RoundControlSource ? Round_Host : SourceModes[RoundControl];
}

Ref OpDispatchBuilder::InsertScalarRoundImpl(OpcodeArgs, IR::OpSize DstSize, size_t ElementSize, const X86Tables::DecodedOperand& Src1Op,
                                             const X86Tables::DecodedOperand& Src2Op, uint64_t Mode, bool ZeroUpperBits) {
  // We load the full vector width when dealing with a source vector,
  // so that we don't do any unnecessary zero extension to the scalar
  // element that we're going to operate on.
  const auto SrcSize = OpSizeFromSrc(Op);

  Ref Src1 = LoadSource_WithOpSize(FPRClass, Op, Src1Op, DstSize, Op->Flags);
  Ref Src2 = LoadSource_WithOpSize(FPRClass, Op, Src2Op, SrcSize, Op->Flags, {.AllowUpperGarbage = true});

  const auto SourceMode = TranslateRoundType(Mode);
  auto ALUOp = _VFToIScalarInsert(IR::SizeToOpSize(DstSize), ElementSize, Src1, Src2, SourceMode, ZeroUpperBits);

  return ALUOp;
}

template<size_t ElementSize>
void OpDispatchBuilder::InsertScalarRound(OpcodeArgs) {
  const uint64_t Mode = Op->Src[1].Literal();
  const auto DstSize = GetGuestVectorLength();

  Ref Result = InsertScalarRoundImpl(Op, DstSize, ElementSize, Op->Dest, Op->Src[0], Mode, false);
  StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Result, DstSize, OpSize::iInvalid);
}

template void OpDispatchBuilder::InsertScalarRound<OpSize::i32Bit>(OpcodeArgs);
template void OpDispatchBuilder::InsertScalarRound<OpSize::i64Bit>(OpcodeArgs);

template<size_t ElementSize>
void OpDispatchBuilder::AVXInsertScalarRound(OpcodeArgs) {
  const uint64_t Mode = Op->Src[2].Literal();
  const auto DstSize = GetGuestVectorLength();

  Ref Result = InsertScalarRoundImpl(Op, DstSize, ElementSize, Op->Dest, Op->Src[0], Mode, true);
  StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Result, DstSize, OpSize::iInvalid);
}

template void OpDispatchBuilder::AVXInsertScalarRound<OpSize::i32Bit>(OpcodeArgs);
template void OpDispatchBuilder::AVXInsertScalarRound<OpSize::i64Bit>(OpcodeArgs);


Ref OpDispatchBuilder::InsertScalarFCMPOpImpl(OpSize Size, uint8_t OpDstSize, size_t ElementSize, Ref Src1, Ref Src2, uint8_t CompType,
                                              bool ZeroUpperBits) {
  switch (CompType & 7) {
  case 0x0: // EQ
    return _VFCMPScalarInsert(Size, ElementSize, Src1, Src2, FloatCompareOp::EQ, ZeroUpperBits);
  case 0x1: // LT, GT(Swapped operand)
    return _VFCMPScalarInsert(Size, ElementSize, Src1, Src2, FloatCompareOp::LT, ZeroUpperBits);
  case 0x2: // LE, GE(Swapped operand)
    return _VFCMPScalarInsert(Size, ElementSize, Src1, Src2, FloatCompareOp::LE, ZeroUpperBits);
  case 0x3: // Unordered
    return _VFCMPScalarInsert(Size, ElementSize, Src1, Src2, FloatCompareOp::UNO, ZeroUpperBits);
  case 0x4: // NEQ
    return _VFCMPScalarInsert(Size, ElementSize, Src1, Src2, FloatCompareOp::NEQ, ZeroUpperBits);
  case 0x5: { // NLT, NGT(Swapped operand)
    Ref Result = _VFCMPLT(ElementSize, ElementSize, Src1, Src2);
    Result = _VNot(ElementSize, ElementSize, Result);
    // Insert the lower bits
    return _VInsElement(OpDstSize, ElementSize, 0, 0, Src1, Result);
  }
  case 0x6: { // NLE, NGE(Swapped operand)
    Ref Result = _VFCMPLE(ElementSize, ElementSize, Src1, Src2);
    Result = _VNot(ElementSize, ElementSize, Result);
    // Insert the lower bits
    return _VInsElement(OpDstSize, ElementSize, 0, 0, Src1, Result);
  }
  case 0x7: // Ordered
    return _VFCMPScalarInsert(Size, ElementSize, Src1, Src2, FloatCompareOp::ORD, ZeroUpperBits);
  }
  FEX_UNREACHABLE;
}

template<size_t ElementSize>
void OpDispatchBuilder::InsertScalarFCMPOp(OpcodeArgs) {
  const uint8_t CompType = Op->Src[1].Literal();
  const auto DstSize = GetGuestVectorLength();
  const auto SrcSize = OpSizeFromSrc(Op);

  Ref Src1 = LoadSource_WithOpSize(FPRClass, Op, Op->Dest, DstSize, Op->Flags);
  Ref Src2 = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], SrcSize, Op->Flags, {.AllowUpperGarbage = true});

  Ref Result = InsertScalarFCMPOpImpl(IR::SizeToOpSize(DstSize), GetDstSize(Op), ElementSize, Src1, Src2, CompType, false);
  StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Result, DstSize, OpSize::iInvalid);
}

template void OpDispatchBuilder::InsertScalarFCMPOp<OpSize::i32Bit>(OpcodeArgs);
template void OpDispatchBuilder::InsertScalarFCMPOp<OpSize::i64Bit>(OpcodeArgs);

template<size_t ElementSize>
void OpDispatchBuilder::AVXInsertScalarFCMPOp(OpcodeArgs) {
  const uint8_t CompType = Op->Src[2].Literal();
  const auto DstSize = GetGuestVectorLength();
  const auto SrcSize = OpSizeFromSrc(Op);

  // We load the full vector width when dealing with a source vector,
  // so that we don't do any unnecessary zero extension to the scalar
  // element that we're going to operate on.
  Ref Src1 = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], DstSize, Op->Flags);
  Ref Src2 = LoadSource_WithOpSize(FPRClass, Op, Op->Src[1], SrcSize, Op->Flags, {.AllowUpperGarbage = true});

  Ref Result = InsertScalarFCMPOpImpl(IR::SizeToOpSize(DstSize), GetDstSize(Op), ElementSize, Src1, Src2, CompType, true);
  StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Result, DstSize, OpSize::iInvalid);
}

template void OpDispatchBuilder::AVXInsertScalarFCMPOp<OpSize::i32Bit>(OpcodeArgs);
template void OpDispatchBuilder::AVXInsertScalarFCMPOp<OpSize::i64Bit>(OpcodeArgs);

void OpDispatchBuilder::VectorUnaryOp(OpcodeArgs, IROps IROp, size_t ElementSize) {
  // In the event of a scalar operation and a vector source, then
  // we can specify the entire vector length in order to avoid
  // unnecessary sign extension on the element to be operated on.
  // In the event of a memory operand, we load the exact element size.
  const auto SrcSize = OpSizeFromSrc(Op);

  Ref Src = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], SrcSize, Op->Flags);

  DeriveOp(ALUOp, IROp, _VFSqrt(SrcSize, ElementSize, Src));

  StoreResult(FPRClass, Op, ALUOp, OpSize::iInvalid);
}

void OpDispatchBuilder::AVXVectorUnaryOp(OpcodeArgs, IROps IROp, size_t ElementSize) {
  // In the event of a scalar operation and a vector source, then
  // we can specify the entire vector length in order to avoid
  // unnecessary sign extension on the element to be operated on.
  // In the event of a memory operand, we load the exact element size.
  const auto SrcSize = OpSizeFromSrc(Op);

  Ref Src = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], SrcSize, Op->Flags);

  DeriveOp(ALUOp, IROp, _VFSqrt(SrcSize, ElementSize, Src));

  // NOTE: We don't need to clear the upper lanes here, since the
  //       IR ops make use of 128-bit AdvSimd for 128-bit cases,
  //       which, on hardware with SVE, zero-extends as part of
  //       storing into the destination.

  StoreResult(FPRClass, Op, ALUOp, OpSize::iInvalid);
}

void OpDispatchBuilder::VectorUnaryDuplicateOpImpl(OpcodeArgs, IROps IROp, size_t ElementSize) {
  const auto Size = GetSrcSize(Op);

  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

  DeriveOp(ALUOp, IROp, _VFSqrt(ElementSize, ElementSize, Src));

  // Duplicate the lower bits
  auto Result = _VDupElement(Size, ElementSize, ALUOp, 0);
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

template<IROps IROp, size_t ElementSize>
void OpDispatchBuilder::VectorUnaryDuplicateOp(OpcodeArgs) {
  VectorUnaryDuplicateOpImpl(Op, IROp, ElementSize);
}

template void OpDispatchBuilder::VectorUnaryDuplicateOp<IR::OP_VFRSQRT, OpSize::i32Bit>(OpcodeArgs);
template void OpDispatchBuilder::VectorUnaryDuplicateOp<IR::OP_VFRECP, OpSize::i32Bit>(OpcodeArgs);

void OpDispatchBuilder::MOVQOp(OpcodeArgs, VectorOpType VectorType) {
  const auto SrcSize = Op->Src[0].IsGPR() ? OpSize::i128Bit : OpSizeFromSrc(Op);
  Ref Src = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], SrcSize, Op->Flags);
  // This instruction is a bit special that if the destination is a register then it'll ZEXT the 64bit source to 128bit
  if (Op->Dest.IsGPR()) {
    const auto gpr = Op->Dest.Data.GPR.GPR;
    const auto gprIndex = gpr - X86State::REG_XMM_0;

    auto Reg = _VMov(OpSize::i64Bit, Src);
    StoreXMMRegister_WithAVXInsert(VectorType, gprIndex, Reg);
  } else {
    // This is simple, just store the result
    StoreResult(FPRClass, Op, Src, OpSize::iInvalid);
  }
}

void OpDispatchBuilder::MOVQMMXOp(OpcodeArgs) {
  // Partial store into bottom 64-bits, leave the upper bits unaffected.
  if (MMXState == MMXState_X87) {
    ChgStateX87_MMX();
  }
  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, {.Align = OpSize::i8Bit});
  StoreResult(FPRClass, Op, Src, OpSize::i8Bit);
}

void OpDispatchBuilder::MOVMSKOp(OpcodeArgs, IR::OpSize ElementSize) {
  const auto Size = OpSizeFromSrc(Op);
  uint8_t NumElements = Size / ElementSize;

  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

  if (Size == OpSize::i128Bit && ElementSize == OpSize::i64Bit) {
    // UnZip2 the 64-bit elements as 32-bit to get the sign bits closer.
    // Sign bits are now in bit positions 31 and 63 after this.
    Src = _VUnZip2(Size, OpSize::i32Bit, Src, Src);

    // Extract the low 64-bits to GPR in one move.
    Ref GPR = _VExtractToGPR(Size, OpSize::i64Bit, Src, 0);
    // BFI the sign bit in 31 in to 62.
    // Inserting the full lower 32-bits offset 31 so the sign bit ends up at offset 63.
    GPR = _Bfi(OpSize::i64Bit, 32, 31, GPR, GPR);
    // Shift right to only get the two sign bits we care about.
    GPR = _Lshr(OpSize::i64Bit, GPR, _Constant(62));
    StoreResult_WithOpSize(GPRClass, Op, Op->Dest, GPR, CTX->GetGPROpSize(), OpSize::iInvalid);
  } else if (Size == OpSize::i128Bit && ElementSize == OpSize::i32Bit) {
    // Shift all the sign bits to the bottom of their respective elements.
    Src = _VUShrI(Size, OpSize::i32Bit, Src, 31);
    // Load the specific 128-bit movmskps shift elements operator.
    auto ConstantUSHL = LoadAndCacheNamedVectorConstant(Size, NAMED_VECTOR_MOVMSKPS_SHIFT);
    // Shift the sign bits in to specific locations.
    Src = _VUShl(Size, OpSize::i32Bit, Src, ConstantUSHL, false);
    // Add across the vector so the sign bits will end up in bits [3:0]
    Src = _VAddV(Size, OpSize::i32Bit, Src);
    // Extract to a GPR.
    Ref GPR = _VExtractToGPR(Size, OpSize::i32Bit, Src, 0);
    StoreResult_WithOpSize(GPRClass, Op, Op->Dest, GPR, CTX->GetGPROpSize(), OpSize::iInvalid);
  } else {
    Ref CurrentVal = _Constant(0);

    for (unsigned i = 0; i < NumElements; ++i) {
      // Extract the top bit of the element
      Ref Tmp = _VExtractToGPR(Size, ElementSize, Src, i);
      Tmp = _Bfe(IR::SizeToOpSize(ElementSize), 1, ElementSize * 8 - 1, Tmp);

      // Shift it to the correct location
      Tmp = _Lshl(IR::SizeToOpSize(ElementSize), Tmp, _Constant(i));

      // Or it with the current value
      CurrentVal = _Or(OpSize::i64Bit, CurrentVal, Tmp);
    }
    StoreResult(GPRClass, Op, CurrentVal, OpSize::iInvalid);
  }
}

void OpDispatchBuilder::MOVMSKOpOne(OpcodeArgs) {
  const auto SrcSize = OpSizeFromSrc(Op);
  const auto Is256Bit = SrcSize == Core::CPUState::XMM_AVX_REG_SIZE;
  const auto ExtractSize = Is256Bit ? OpSize::i32Bit : OpSize::i16Bit;

  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref VMask = LoadAndCacheNamedVectorConstant(SrcSize, NAMED_VECTOR_MOVMASKB);

  auto VCMP = _VCMPLTZ(SrcSize, OpSize::i8Bit, Src);
  auto VAnd = _VAnd(SrcSize, OpSize::i8Bit, VCMP, VMask);

  // Since we also handle the MM MOVMSKB here too,
  // we need to clamp the lower bound.
  const auto VAdd1Size = std::max<uint8_t>(SrcSize, OpSize::i128Bit);
  const auto VAdd2Size = std::max<uint8_t>(SrcSize / 2, OpSize::i64Bit);

  auto VAdd1 = _VAddP(VAdd1Size, OpSize::i8Bit, VAnd, VAnd);
  auto VAdd2 = _VAddP(VAdd2Size, OpSize::i8Bit, VAdd1, VAdd1);
  auto VAdd3 = _VAddP(OpSize::i64Bit, OpSize::i8Bit, VAdd2, VAdd2);

  auto Result = _VExtractToGPR(SrcSize, ExtractSize, VAdd3, 0);

  StoreResult(GPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::PUNPCKLOp(OpcodeArgs, size_t ElementSize) {
  auto Size = GetSrcSize(Op);

  Ref Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

  auto ALUOp = _VZip(Size, ElementSize, Dest, Src);
  StoreResult(FPRClass, Op, ALUOp, OpSize::iInvalid);
}

void OpDispatchBuilder::VPUNPCKLOp(OpcodeArgs, size_t ElementSize) {
  const auto SrcSize = GetSrcSize(Op);
  const auto Is128Bit = SrcSize == Core::CPUState::XMM_SSE_REG_SIZE;

  Ref Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);

  Ref Result {};
  if (Is128Bit) {
    Result = _VZip(SrcSize, ElementSize, Src1, Src2);
  } else {
    Ref ZipLo = _VZip(SrcSize, ElementSize, Src1, Src2);
    Ref ZipHi = _VZip2(SrcSize, ElementSize, Src1, Src2);

    Result = _VInsElement(SrcSize, OpSize::i128Bit, 1, 0, ZipLo, ZipHi);
  }

  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::PUNPCKHOp(OpcodeArgs, size_t ElementSize) {
  auto Size = GetSrcSize(Op);
  Ref Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

  auto ALUOp = _VZip2(Size, ElementSize, Dest, Src);
  StoreResult(FPRClass, Op, ALUOp, OpSize::iInvalid);
}

void OpDispatchBuilder::VPUNPCKHOp(OpcodeArgs, size_t ElementSize) {
  const auto SrcSize = GetSrcSize(Op);
  const auto Is128Bit = SrcSize == Core::CPUState::XMM_SSE_REG_SIZE;

  Ref Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);

  Ref Result {};
  if (Is128Bit) {
    Result = _VZip2(SrcSize, ElementSize, Src1, Src2);
  } else {
    Ref ZipLo = _VZip(SrcSize, ElementSize, Src1, Src2);
    Ref ZipHi = _VZip2(SrcSize, ElementSize, Src1, Src2);

    Result = _VInsElement(SrcSize, OpSize::i128Bit, 0, 1, ZipHi, ZipLo);
  }

  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

Ref OpDispatchBuilder::GeneratePSHUFBMask(uint8_t SrcSize) {
  // PSHUFB doesn't 100% match VTBL behaviour
  // VTBL will set the element zero if the index is greater than
  // the number of elements in the array
  //
  // Bit 7 is the only bit that is supposed to set elements to zero with PSHUFB
  // Mask the selection bits and top bit correctly
  // Bits [6:4] is reserved for 128-bit/256-bit
  // Bits [6:3] is reserved for 64-bit
  const uint8_t MaskImm = SrcSize == OpSize::i64Bit ? 0b1000'0111 : 0b1000'1111;

  return _VectorImm(SrcSize, 1, MaskImm);
}

Ref OpDispatchBuilder::PSHUFBOpImpl(uint8_t SrcSize, Ref Src1, Ref Src2, Ref MaskVector) {
  const auto Is256Bit = SrcSize == Core::CPUState::XMM_AVX_REG_SIZE;

  // We perform the 256-bit version as two 128-bit operations due to
  // the lane splitting behavior, so cap the maximum size at 16.
  const auto SanitizedSrcSize = std::min<uint8_t>(SrcSize, OpSize::i128Bit);

  Ref MaskedIndices = _VAnd(SrcSize, SrcSize, Src2, MaskVector);

  Ref Low = _VTBL1(SanitizedSrcSize, Src1, MaskedIndices);
  if (!Is256Bit) {
    return Low;
  }

  Ref HighSrc1 = _VInsElement(SrcSize, OpSize::i128Bit, 0, 1, Src1, Src1);
  Ref High = _VTBL1(SanitizedSrcSize, HighSrc1, MaskedIndices);
  return _VInsElement(SrcSize, OpSize::i128Bit, 1, 0, Low, High);
}

void OpDispatchBuilder::PSHUFBOp(OpcodeArgs) {
  const auto SrcSize = GetSrcSize(Op);
  Ref Src1 = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  Ref Src2 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

  Ref Result = PSHUFBOpImpl(SrcSize, Src1, Src2, GeneratePSHUFBMask(SrcSize));
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::VPSHUFBOp(OpcodeArgs) {
  const auto SrcSize = GetSrcSize(Op);
  Ref Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);

  Ref Result = PSHUFBOpImpl(SrcSize, Src1, Src2, GeneratePSHUFBMask(SrcSize));
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

Ref OpDispatchBuilder::PShufWLane(size_t Size, FEXCore::IR::IndexNamedVectorConstant IndexConstant, bool LowLane, Ref IncomingLane, uint8_t Shuffle) {
  constexpr auto IdentityCopy = 0b11'10'01'00;

  const bool Is128BitLane = Size == OpSize::i128Bit;
  const uint8_t NumElements = Size / 2;
  const uint8_t HalfNumElements = NumElements >> 1;

  // TODO: There can be more optimized copies here.
  switch (Shuffle) {
  case IdentityCopy: {
    // Special case identity copy.
    return IncomingLane;
  }
  case 0b00'00'00'00:
  case 0b01'01'01'01:
  case 0b10'10'10'10:
  case 0b11'11'11'11: {
    // Special case element duplicate and broadcast to low or high 64-bits.
    Ref Dup = _VDupElement(Size, OpSize::i16Bit, IncomingLane, (LowLane ? 0 : HalfNumElements) + (Shuffle & 0b11));
    if (Is128BitLane) {
      if (LowLane) {
        // DUP goes low.
        // Source goes high.
        Dup = _VTrn2(Size, OpSize::i64Bit, Dup, IncomingLane);
      } else {
        // DUP goes high.
        // Source goes low.
        Dup = _VTrn(Size, OpSize::i64Bit, IncomingLane, Dup);
      }
    }

    return Dup;
  }
  default: {
    // PSHUFLW needs to scale index by 16.
    // PSHUFHW needs to scale index by 16.
    // PSHUFW (mmx) also needs to scale by 16 to get correct low element.
    auto LookupIndexes = LoadAndCacheIndexedNamedVectorConstant(Size, IndexConstant, Shuffle * 16);
    return _VTBL1(Size, IncomingLane, LookupIndexes);
  }
  }
}

void OpDispatchBuilder::PSHUFW8ByteOp(OpcodeArgs) {
  uint16_t Shuffle = Op->Src[1].Data.Literal.Value;
  const auto Size = GetSrcSize(Op);
  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Dest = PShufWLane(Size, FEXCore::IR::INDEXED_NAMED_VECTOR_PSHUFLW, true, Src, Shuffle);
  StoreResult(FPRClass, Op, Dest, OpSize::iInvalid);
}

void OpDispatchBuilder::PSHUFWOp(OpcodeArgs, bool Low) {
  uint16_t Shuffle = Op->Src[1].Data.Literal.Value;
  const auto Size = GetSrcSize(Op);
  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  const auto IndexedVectorConstant = Low ? FEXCore::IR::IndexNamedVectorConstant::INDEXED_NAMED_VECTOR_PSHUFLW :
                                           FEXCore::IR::IndexNamedVectorConstant::INDEXED_NAMED_VECTOR_PSHUFHW;

  Ref Dest = PShufWLane(Size, IndexedVectorConstant, Low, Src, Shuffle);

  StoreResult(FPRClass, Op, Dest, OpSize::iInvalid);
}

Ref OpDispatchBuilder::Single128Bit4ByteVectorShuffle(Ref Src, uint8_t Shuffle) {
  constexpr auto IdentityCopy = 0b11'10'01'00;

  // TODO: There can be more optimized copies here.
  switch (Shuffle) {
  case IdentityCopy: {
    // Special case identity copy.
    return Src;
  }
  case 0b00'00'00'00:
  case 0b01'01'01'01:
  case 0b10'10'10'10:
  case 0b11'11'11'11: {
    // Special case element duplicate and broadcast to low or high 64-bits.
    return _VDupElement(OpSize::i128Bit, OpSize::i32Bit, Src, Shuffle & 0b11);
  }

  case 0b00'00'10'10: {
    // Weird reverse low elements and broadcast to each half of the register
    Ref Tmp = _VUnZip(OpSize::i128Bit, OpSize::i32Bit, Src, Src);
    Tmp = _VRev64(OpSize::i128Bit, OpSize::i32Bit, Tmp);
    return _VZip(OpSize::i128Bit, OpSize::i32Bit, Tmp, Tmp);
  }
  case 0b00'01'00'01: {
    ///< Weird reversed low elements and broadcast
    Ref Tmp = _VRev64(OpSize::i128Bit, OpSize::i32Bit, Src);
    return _VZip(OpSize::i128Bit, OpSize::i64Bit, Tmp, Tmp);
  }
  case 0b00'01'01'00: {
    ///< Weird reverse low two elements in to high half
    Ref Tmp = _VZip(OpSize::i128Bit, OpSize::i32Bit, Src, Src);
    return _VExtr(OpSize::i128Bit, OpSize::i8Bit, Tmp, Tmp, 4);
  }
  case 0b00'10'00'10: {
    ///< Weird reversed even elements and broadcast
    Ref Tmp = _VUnZip(OpSize::i128Bit, OpSize::i32Bit, Src, Src);
    return _VExtr(OpSize::i128Bit, OpSize::i8Bit, Tmp, Tmp, 4);
  }
  case 0b00'10'10'00: {
    // Weird reversed low elements in upper half of the register
    Ref Tmp = _VUnZip(OpSize::i128Bit, OpSize::i32Bit, Src, Src);
    Tmp = _VZip(OpSize::i128Bit, OpSize::i32Bit, Tmp, Tmp);
    return _VExtr(OpSize::i128Bit, OpSize::i8Bit, Tmp, Tmp, 4);
  }
  case 0b00'11'00'11: {
    ///< Weird Low plus high element reversed and broadcast
    Ref Tmp = _VExtr(OpSize::i128Bit, OpSize::i8Bit, Src, Src, 4);
    return _VZip2(OpSize::i128Bit, OpSize::i64Bit, Tmp, Tmp);
  }
  case 0b00'11'10'01:
    ///< Vector rotate - One element
    return _VExtr(OpSize::i128Bit, OpSize::i8Bit, Src, Src, 4);
  case 0b00'11'11'00: {
    // Weird reversed low and high elements in upper half of the register
    Ref Tmp = _VExtr(OpSize::i128Bit, OpSize::i8Bit, Src, Src, 4);
    Tmp = _VZip2(OpSize::i128Bit, OpSize::i32Bit, Tmp, Tmp);
    return _VExtr(OpSize::i128Bit, OpSize::i8Bit, Tmp, Tmp, 12);
  }
  case 0b01'00'00'01: {
    ///< Weird duplicate bottom two elements, then rotate in the low half
    Ref Tmp = _VZip(OpSize::i128Bit, OpSize::i32Bit, Src, Src);
    return _VExtr(OpSize::i128Bit, OpSize::i8Bit, Tmp, Tmp, 12);
  }
  case 0b01'00'01'00:
    ///< Duplicate bottom 64-bits
    return _VDupElement(OpSize::i128Bit, OpSize::i64Bit, Src, 0);
  case 0b01'00'11'10:
    ///< Vector rotate - Two elements
    return _VExtr(OpSize::i128Bit, OpSize::i8Bit, Src, Src, 8);
  case 0b01'01'00'00: {
    // Zip with self.
    // Dest[0] = Src[0]
    // Dest[1] = Src[0]
    // Dest[2] = Src[1]
    // Dest[3] = Src[1]
    return _VZip(OpSize::i128Bit, OpSize::i32Bit, Src, Src);
  }
  case 0b01'01'10'10: {
    ///< Weird reverse middle elements and broadcast to each half of the register
    Ref Tmp = _VExtr(OpSize::i128Bit, OpSize::i8Bit, Src, Src, 4);
    Tmp = _VRev64(OpSize::i128Bit, OpSize::i32Bit, Tmp);
    return _VZip(OpSize::i128Bit, OpSize::i32Bit, Tmp, Tmp);
  }
  case 0b01'01'11'11: {
    ///< Weird reverse odd elements and broadcast to each half of the register
    Ref Tmp = _VUnZip2(OpSize::i128Bit, OpSize::i32Bit, Src, Src);
    Tmp = _VRev64(OpSize::i128Bit, OpSize::i32Bit, Tmp);
    return _VZip(OpSize::i128Bit, OpSize::i32Bit, Tmp, Tmp);
  }
  case 0b01'10'01'10: {
    ///< Weird middle elements swizzle plus broadcast
    Ref Tmp = _VExtr(OpSize::i128Bit, OpSize::i8Bit, Src, Src, 4);
    Tmp = _VRev64(OpSize::i128Bit, OpSize::i32Bit, Tmp);
    return _VZip(OpSize::i128Bit, OpSize::i64Bit, Tmp, Tmp);
  }
  case 0b01'10'10'01: {
    ///< Weird middle elements swizzle plus broadcast and reverse
    Ref Tmp = _VExtr(OpSize::i128Bit, OpSize::i8Bit, Src, Src, 4);
    Tmp = _VZip(OpSize::i128Bit, OpSize::i32Bit, Tmp, Tmp);
    return _VExtr(OpSize::i128Bit, OpSize::i8Bit, Tmp, Tmp, 4);
  }
  case 0b01'11'01'11: {
    ///< Weird reversed odd elements and broadcast
    Ref Tmp = _VUnZip2(OpSize::i128Bit, OpSize::i32Bit, Src, Src);
    return _VExtr(OpSize::i128Bit, OpSize::i8Bit, Tmp, Tmp, 4);
  }
  case 0b01'11'11'01: {
    ///< Weird odd elements swizzle plus broadcast and reverse
    Ref Tmp = _VUnZip2(OpSize::i128Bit, OpSize::i32Bit, Src, Src);
    Tmp = _VZip(OpSize::i128Bit, OpSize::i32Bit, Tmp, Tmp);
    return _VExtr(OpSize::i128Bit, OpSize::i8Bit, Tmp, Tmp, 4);
  }
  case 0b10'00'00'10: {
    ///< Weird even elements swizzle plus broadcast and reverse
    Ref Tmp = _VUnZip(OpSize::i128Bit, OpSize::i32Bit, Src, Src);
    Tmp = _VZip(OpSize::i128Bit, OpSize::i32Bit, Tmp, Tmp);
    return _VExtr(OpSize::i128Bit, OpSize::i8Bit, Tmp, Tmp, 12);
  }
  case 0b10'00'10'00:
    ///< Even elements broadcast
    return _VUnZip(OpSize::i128Bit, OpSize::i32Bit, Src, Src);
  case 0b10'01'00'11:
    ///< Vector rotate - Three elements
    return _VExtr(OpSize::i128Bit, OpSize::i8Bit, Src, Src, 12);

  case 0b10'01'01'10: {
    ///< Weird odd elements swizzle plus broadcast and reverse
    Ref Tmp = _VExtr(OpSize::i128Bit, OpSize::i8Bit, Src, Src, 4);
    Tmp = _VZip(OpSize::i128Bit, OpSize::i32Bit, Tmp, Tmp);
    return _VExtr(OpSize::i128Bit, OpSize::i8Bit, Tmp, Tmp, 12);
  }
  case 0b10'01'10'01: {
    ///< Middle two elements broadcast
    Ref Tmp = _VExtr(OpSize::i128Bit, OpSize::i8Bit, Src, Src, 4);
    return _VZip(OpSize::i128Bit, OpSize::i64Bit, Tmp, Tmp);
  }
  case 0b10'10'00'00: {
    ///< Broadcast even elements to each half of the register
    Ref Tmp = _VUnZip(OpSize::i128Bit, OpSize::i32Bit, Src, Src);
    return _VZip(OpSize::i128Bit, OpSize::i32Bit, Tmp, Tmp);
  }
  case 0b10'10'01'01: {
    ///< Broadcast middle elements to each half of the register
    Ref Tmp = _VExtr(OpSize::i128Bit, OpSize::i8Bit, Src, Src, 4);
    return _VZip(OpSize::i128Bit, OpSize::i32Bit, Tmp, Tmp);
  }
  case 0b10'10'11'11: {
    ///< Reverse top two elements and broadcast to each half of the register
    Ref Tmp = _VZip2(OpSize::i128Bit, OpSize::i32Bit, Src, Src);
    return _VExtr(OpSize::i128Bit, OpSize::i8Bit, Tmp, Tmp, 8);
  }
  case 0b10'11'10'11: {
    ///< Weird top two elements reverse and broadcast
    Ref Tmp = _VZip2(OpSize::i128Bit, OpSize::i64Bit, Src, Src);
    return _VExtr(OpSize::i128Bit, OpSize::i8Bit, Tmp, Tmp, 4);
  }
  case 0b10'11'11'10: {
    ///< Weird move top two elements to bottom and reverse in the top half
    Ref Tmp = _VZip2(OpSize::i128Bit, OpSize::i32Bit, Src, Src);
    return _VExtr(OpSize::i128Bit, OpSize::i8Bit, Tmp, Tmp, 4);
  }
  case 0b11'00'00'11: {
    ///< Weird low plus high elements swizzle plus broadcast and reverse
    Ref Tmp = _VExtr(OpSize::i128Bit, OpSize::i8Bit, Src, Src, 4);
    Tmp = _VZip2(OpSize::i128Bit, OpSize::i32Bit, Tmp, Tmp);
    return _VExtr(OpSize::i128Bit, OpSize::i8Bit, Tmp, Tmp, 4);
  }
  case 0b11'00'11'00: {
    ///< Weird low plus high element broadcast
    Ref Tmp = _VExtr(OpSize::i128Bit, OpSize::i8Bit, Src, Src, 4);
    Tmp = _VZip2(OpSize::i128Bit, OpSize::i64Bit, Tmp, Tmp);
    return _VExtr(OpSize::i128Bit, OpSize::i8Bit, Tmp, Tmp, 4);
  }
  case 0b11'01'01'11: {
    ///< Weird odd elements swizzle plus broadcast and reverse
    Ref Tmp = _VUnZip2(OpSize::i128Bit, OpSize::i32Bit, Src, Src);
    Tmp = _VZip(OpSize::i128Bit, OpSize::i32Bit, Tmp, Tmp);
    return _VExtr(OpSize::i128Bit, OpSize::i8Bit, Tmp, Tmp, 12);
  }
  case 0b11'01'11'01:
    ///< Odd elements broadcast
    return _VUnZip2(OpSize::i128Bit, OpSize::i32Bit, Src, Src);
  case 0b11'10'10'11: {
    ///< Rotate top two elements in to bottom half of the register
    Ref Tmp = _VZip2(OpSize::i128Bit, OpSize::i32Bit, Src, Src);
    return _VExtr(OpSize::i128Bit, OpSize::i8Bit, Tmp, Tmp, 12);
  }
  case 0b11'10'11'10:
    ///< Duplicate Top 64-bits
    return _VDupElement(OpSize::i128Bit, OpSize::i64Bit, Src, 1);
  case 0b11'11'00'00: {
    ///< Weird Broadcast bottom and top element to each half of the register
    Ref Tmp = _VExtr(OpSize::i128Bit, OpSize::i8Bit, Src, Src, 12);
    Tmp = _VRev64(OpSize::i128Bit, OpSize::i32Bit, Tmp);
    return _VZip(OpSize::i128Bit, OpSize::i32Bit, Tmp, Tmp);
  }
  case 0b11'11'01'01: {
    ///< Broadcast odd elements to each half of the register
    Ref Tmp = _VUnZip2(OpSize::i128Bit, OpSize::i32Bit, Src, Src);
    return _VZip(OpSize::i128Bit, OpSize::i32Bit, Tmp, Tmp);
  }
  case 0b11'11'10'10:
    ///< Broadcast top two elements to each half of the register
    return _VZip2(OpSize::i128Bit, OpSize::i32Bit, Src, Src);
  default: {
    // PSHUFD needs to scale index by 16.
    auto LookupIndexes =
      LoadAndCacheIndexedNamedVectorConstant(OpSize::i128Bit, FEXCore::IR::IndexNamedVectorConstant::INDEXED_NAMED_VECTOR_PSHUFD, Shuffle * 16);
    return _VTBL1(OpSize::i128Bit, Src, LookupIndexes);
  }
  }
}

void OpDispatchBuilder::PSHUFDOp(OpcodeArgs) {
  uint16_t Shuffle = Op->Src[1].Data.Literal.Value;
  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  StoreResult(FPRClass, Op, Single128Bit4ByteVectorShuffle(Src, Shuffle), OpSize::iInvalid);
}

void OpDispatchBuilder::VPSHUFWOp(OpcodeArgs, size_t ElementSize, bool Low) {
  const auto SrcSize = GetSrcSize(Op);
  const auto Is256Bit = SrcSize == Core::CPUState::XMM_AVX_REG_SIZE;
  auto Shuffle = Op->Src[1].Literal();

  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

  // Note/TODO: With better immediate facilities or vector loading in our IR
  //            much of this can be reduced to setting up a table index register
  //            and then using TBL
  //
  //            SVE has the INDEX instruction that works essentially like
  //            std::iota (setting a range to an initial value and progressively
  //            incrementing each successive element), so it's well suited for this.
  //            It's just a matter of exposing these facilities in a way that works
  //            well together.
  //
  //            Should be much nicer than doing repeated inserts in any case.

  const size_t BaseElement = Low ? 0 : 4;
  Ref Result = Src;
  if (Is256Bit) {
    for (size_t i = 0; i < 4; i++) {
      const auto Index = Shuffle & 0b11;
      const auto UpperLaneOffset = Core::CPUState::XMM_SSE_REG_SIZE / ElementSize;

      const auto LowDstIndex = BaseElement + i;
      const auto LowSrcIndex = BaseElement + Index;

      const auto HighDstIndex = BaseElement + UpperLaneOffset + i;
      const auto HighSrcIndex = BaseElement + UpperLaneOffset + Index;

      // Take care of both lanes per iteration
      Result = _VInsElement(SrcSize, ElementSize, LowDstIndex, LowSrcIndex, Result, Src);
      Result = _VInsElement(SrcSize, ElementSize, HighDstIndex, HighSrcIndex, Result, Src);

      Shuffle >>= 2;
    }
  } else {
    for (size_t i = 0; i < 4; i++) {
      const auto Index = Shuffle & 0b11;
      Result = _VInsElement(SrcSize, ElementSize, BaseElement + i, BaseElement + Index, Result, Src);
      Shuffle >>= 2;
    }
  }

  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

Ref OpDispatchBuilder::SHUFOpImpl(OpcodeArgs, size_t DstSize, size_t ElementSize, Ref Src1, Ref Src2, uint8_t Shuffle) {
  // Since 256-bit variants and up don't lane cross, we can construct
  // everything in terms of the 128-variant, as each lane is essentially
  // its own 128-bit segment.
  const uint8_t NumElements = Core::CPUState::XMM_SSE_REG_SIZE / ElementSize;
  const uint8_t HalfNumElements = NumElements >> 1;

  const bool Is256Bit = DstSize == Core::CPUState::XMM_AVX_REG_SIZE;

  std::array<Ref, 4> Srcs {};
  for (size_t i = 0; i < HalfNumElements; ++i) {
    Srcs[i] = Src1;
  }
  for (size_t i = HalfNumElements; i < NumElements; ++i) {
    Srcs[i] = Src2;
  }

  Ref Dest = Src1;
  const uint8_t SelectionMask = NumElements - 1;
  const uint8_t ShiftAmount = std::popcount(SelectionMask);

  if (Is256Bit) {
    for (uint8_t Element = 0; Element < NumElements; ++Element) {
      const auto SrcIndex1 = Shuffle & SelectionMask;

      // AVX differs the behavior of VSHUFPD and VSHUFPS.
      // The same immediate bits are used for both lanes with VSHUFPS,
      // but VSHUFPD uses different immediate bits for each lane.
      const auto SrcIndex2 = ElementSize == 4 ? SrcIndex1 : ((Shuffle >> 2) & SelectionMask);

      Ref Insert = _VInsElement(DstSize, ElementSize, Element, SrcIndex1, Dest, Srcs[Element]);
      Dest = _VInsElement(DstSize, ElementSize, Element + NumElements, SrcIndex2 + NumElements, Insert, Srcs[Element]);

      Shuffle >>= ShiftAmount;
    }
  } else {
    if (ElementSize == OpSize::i32Bit) {
      // We can shuffle optimally in a lot of cases.
      // TODO: We can optimize more of these cases.
      switch (Shuffle) {
      case 0b01'00'01'00:
        // Combining of low 64-bits.
        // Dest[63:0]   = Src1[63:0]
        // Dest[127:64] = Src2[63:0]
        return _VZip(DstSize, OpSize::i64Bit, Src1, Src2);
      case 0b11'10'11'10:
        // Combining of high 64-bits.
        // Dest[63:0]   = Src1[127:64]
        // Dest[127:64] = Src2[127:64]
        return _VZip2(DstSize, OpSize::i64Bit, Src1, Src2);
      case 0b11'10'01'00:
        // Mixing Low and high elements
        // Dest[63:0]   = Src1[63:0]
        // Dest[127:64] = Src2[127:64]
        return _VInsElement(DstSize, OpSize::i64Bit, 1, 1, Src1, Src2);
      case 0b01'00'11'10:
        // Mixing Low and high elements, inverse of above
        // Dest[63:0]   = Src1[127:64]
        // Dest[127:64] = Src2[63:0]
        return _VExtr(DstSize, OpSize::i8Bit, Src2, Src1, 8);
      case 0b10'00'10'00:
        // Mixing even elements.
        // Dest[31:0]   = Src1[31:0]
        // Dest[63:32]  = Src1[95:64]
        // Dest[95:64]  = Src2[31:0]
        // Dest[127:96] = Src2[95:64]
        return _VUnZip(DstSize, ElementSize, Src1, Src2);
      case 0b11'01'11'01:
        // Mixing odd elements.
        // Dest[31:0]   = Src1[63:32]
        // Dest[63:32]  = Src1[127:96]
        // Dest[95:64]  = Src2[63:32]
        // Dest[127:96] = Src2[127:96]
        return _VUnZip2(DstSize, ElementSize, Src1, Src2);
      case 0b11'10'00'00:
      case 0b11'10'01'01:
      case 0b11'10'10'10:
      case 0b11'10'11'11: {
        // Bottom elements duplicated, Top 64-bits inserted
        auto DupSrc1 = _VDupElement(DstSize, ElementSize, Src1, Shuffle & 0b11);
        return _VZip2(DstSize, OpSize::i64Bit, DupSrc1, Src2);
      }
      case 0b01'00'00'00:
      case 0b01'00'01'01:
      case 0b01'00'10'10:
      case 0b01'00'11'11: {
        // Bottom elements duplicated, Bottom 64-bits inserted
        auto DupSrc1 = _VDupElement(DstSize, ElementSize, Src1, Shuffle & 0b11);
        return _VZip(DstSize, OpSize::i64Bit, DupSrc1, Src2);
      }
      case 0b00'00'01'00:
      case 0b01'01'01'00:
      case 0b10'10'01'00:
      case 0b11'11'01'00: {
        // Top elements duplicated, Bottom 64-bits inserted
        auto DupSrc2 = _VDupElement(DstSize, ElementSize, Src2, (Shuffle >> 4) & 0b11);
        return _VZip(DstSize, OpSize::i64Bit, Src1, DupSrc2);
      }
      case 0b00'00'11'10:
      case 0b01'01'11'10:
      case 0b10'10'11'10:
      case 0b11'11'11'10: {
        // Top elements duplicated, Top 64-bits inserted
        auto DupSrc2 = _VDupElement(DstSize, ElementSize, Src2, (Shuffle >> 4) & 0b11);
        return _VZip2(DstSize, OpSize::i64Bit, Src1, DupSrc2);
      }
      case 0b01'00'01'11: {
        // TODO: This doesn't generate optimal code.
        // RA doesn't understand that Src1 is dead after VInsElement due to SRA class differences.
        // With RA fixes this would be 2 instructions.
        // Odd elements inverted, Low 64-bits inserted
        Src1 = _VInsElement(DstSize, OpSize::i32Bit, 0, 3, Src1, Src1);
        return _VZip(DstSize, OpSize::i64Bit, Src1, Src2);
      }
      case 0b11'10'01'11: {
        // TODO: This doesn't generate optimal code.
        // RA doesn't understand that Src1 is dead after VInsElement due to SRA class differences.
        // With RA fixes this would be 2 instructions.
        // Odd elements inverted, Top 64-bits inserted
        Src1 = _VInsElement(DstSize, OpSize::i32Bit, 0, 3, Src1, Src1);
        return _VInsElement(DstSize, OpSize::i64Bit, 1, 1, Src1, Src2);
      }
      case 0b01'00'00'01: {
        // Lower 32-bit elements inverted, low 64-bits inserted
        Src1 = _VRev64(DstSize, OpSize::i32Bit, Src1);
        return _VZip(DstSize, OpSize::i64Bit, Src1, Src2);
      }
      case 0b11'10'00'01: {
        // TODO: This doesn't generate optimal code.
        // RA doesn't understand that Src1 is dead after VInsElement due to SRA class differences.
        // With RA fixes this would be 2 instructions.
        // Lower 32-bit elements inverted, Top 64-bits inserted
        Src1 = _VRev64(DstSize, OpSize::i32Bit, Src1);
        return _VInsElement(DstSize, OpSize::i64Bit, 1, 1, Src1, Src2);
      }
      case 0b00'00'00'00:
      case 0b00'00'01'01:
      case 0b00'00'10'10:
      case 0b00'00'11'11:
      case 0b01'01'00'00:
      case 0b01'01'01'01:
      case 0b01'01'10'10:
      case 0b01'01'11'11:
      case 0b10'10'00'00:
      case 0b10'10'01'01:
      case 0b10'10'10'10:
      case 0b10'10'11'11:
      case 0b11'11'00'00:
      case 0b11'11'01'01:
      case 0b11'11'10'10:
      case 0b11'11'11'11: {
        // Duplicate element in upper and lower across each 64-bit segment.
        auto DupSrc1 = _VDupElement(DstSize, ElementSize, Src1, Shuffle & 0b11);
        auto DupSrc2 = _VDupElement(DstSize, ElementSize, Src2, (Shuffle >> 4) & 0b11);
        return _VZip(DstSize, OpSize::i64Bit, DupSrc1, DupSrc2);
      }
      default:
        // Use a TBL2 operation to handle this implementation.
        auto LookupIndexes =
          LoadAndCacheIndexedNamedVectorConstant(DstSize, FEXCore::IR::IndexNamedVectorConstant::INDEXED_NAMED_VECTOR_SHUFPS, Shuffle * 16);
        return _VTBL2(DstSize, Src1, Src2, LookupIndexes);
      }
    } else {
      switch (Shuffle & 0b11) {
      case 0b00:
        // Low 64-bits of each source interleaved.
        return _VZip(DstSize, ElementSize, Src1, Src2);
      case 0b01:
        // Upper 64-bits of Src1 in lower bits
        // Lower 64-bits of Src2 in upper bits.
        return _VExtr(DstSize, OpSize::i8Bit, Src2, Src1, 8);
      case 0b10:
        // Lower 32-bits of Src1 in lower bits.
        // Upper 64-bits of Src2 in upper bits.
        return _VInsElement(DstSize, ElementSize, 1, 1, Src1, Src2);
      case 0b11:
        // Upper 64-bits of each source interleaved.
        return _VZip2(DstSize, ElementSize, Src1, Src2);
      }
    }

    for (uint8_t Element = 0; Element < NumElements; ++Element) {
      const auto SrcIndex = Shuffle & SelectionMask;
      Dest = _VInsElement(DstSize, ElementSize, Element, SrcIndex, Dest, Srcs[Element]);
      Shuffle >>= ShiftAmount;
    }
  }

  return Dest;
}

void OpDispatchBuilder::SHUFOp(OpcodeArgs, size_t ElementSize) {
  Ref Src1Node = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  Ref Src2Node = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  uint8_t Shuffle = Op->Src[1].Literal();

  Ref Result = SHUFOpImpl(Op, GetDstSize(Op), ElementSize, Src1Node, Src2Node, Shuffle);
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::VSHUFOp(OpcodeArgs, size_t ElementSize) {
  Ref Src1Node = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Src2Node = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);
  uint8_t Shuffle = Op->Src[2].Literal();

  Ref Result = SHUFOpImpl(Op, GetDstSize(Op), ElementSize, Src1Node, Src2Node, Shuffle);
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::VANDNOp(OpcodeArgs) {
  const auto SrcSize = GetSrcSize(Op);

  Ref Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);
  Ref Dest = _VAndn(SrcSize, SrcSize, Src2, Src1);

  StoreResult(FPRClass, Op, Dest, OpSize::iInvalid);
}

template<IROps IROp, size_t ElementSize>
void OpDispatchBuilder::VHADDPOp(OpcodeArgs) {
  const auto SrcSize = GetSrcSize(Op);
  const auto Is256Bit = SrcSize == Core::CPUState::XMM_AVX_REG_SIZE;

  Ref Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);

  DeriveOp(Res, IROp, _VFAddP(SrcSize, ElementSize, Src1, Src2));

  Ref Dest = Res;
  if (Is256Bit) {
    Dest = _VInsElement(SrcSize, OpSize::i64Bit, 1, 2, Res, Res);
    Dest = _VInsElement(SrcSize, OpSize::i64Bit, 2, 1, Dest, Res);
  }

  StoreResult(FPRClass, Op, Dest, OpSize::iInvalid);
}

template void OpDispatchBuilder::VHADDPOp<IR::OP_VADDP, 2>(OpcodeArgs);
template void OpDispatchBuilder::VHADDPOp<IR::OP_VADDP, 4>(OpcodeArgs);
template void OpDispatchBuilder::VHADDPOp<IR::OP_VFADDP, 4>(OpcodeArgs);
template void OpDispatchBuilder::VHADDPOp<IR::OP_VFADDP, 8>(OpcodeArgs);

void OpDispatchBuilder::VBROADCASTOp(OpcodeArgs, IR::OpSize ElementSize) {
  const auto DstSize = OpSizeFromDst(Op);
  Ref Result {};

  if (Op->Src[0].IsGPR()) {
    Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
    Result = _VDupElement(DstSize, ElementSize, Src, 0);
  } else {
    // Get the address to broadcast from into a GPR.
    Ref Address = MakeSegmentAddress(Op, Op->Src[0], CTX->GetGPROpSize());
    Result = _VBroadcastFromMem(DstSize, ElementSize, Address);
  }

  // No need to zero-extend result, since implementations
  // use zero extending AdvSIMD or zeroing SVE loads internally.

  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

Ref OpDispatchBuilder::PINSROpImpl(OpcodeArgs, IR::OpSize ElementSize, const X86Tables::DecodedOperand& Src1Op,
                                   const X86Tables::DecodedOperand& Src2Op, const X86Tables::DecodedOperand& Imm) {
  const auto Size = OpSizeFromDst(Op);
  const auto NumElements = Size / ElementSize;
  const uint64_t Index = Imm.Literal() & (NumElements - 1);
  Ref Src1 = LoadSource_WithOpSize(FPRClass, Op, Src1Op, Size, Op->Flags);

  if (Src2Op.IsGPR()) {
    // If the source is a GPR then convert directly from the GPR.
    auto Src2 = LoadSource_WithOpSize(GPRClass, Op, Src2Op, CTX->GetGPROpSize(), Op->Flags);
    return _VInsGPR(Size, ElementSize, Index, Src1, Src2);
  }

  // If loading from memory then we only load the element size
  Ref Src2 = MakeSegmentAddress(Op, Src2Op);
  return _VLoadVectorElement(Size, ElementSize, Src1, Index, Src2);
}

template<IR::OpSize ElementSize>
void OpDispatchBuilder::PINSROp(OpcodeArgs) {
  Ref Result = PINSROpImpl(Op, ElementSize, Op->Dest, Op->Src[0], Op->Src[1]);
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

template void OpDispatchBuilder::PINSROp<OpSize::i8Bit>(OpcodeArgs);
template void OpDispatchBuilder::PINSROp<OpSize::i16Bit>(OpcodeArgs);
template void OpDispatchBuilder::PINSROp<OpSize::i32Bit>(OpcodeArgs);
template void OpDispatchBuilder::PINSROp<OpSize::i64Bit>(OpcodeArgs);

void OpDispatchBuilder::VPINSRBOp(OpcodeArgs) {
  Ref Result = PINSROpImpl(Op, OpSize::i8Bit, Op->Src[0], Op->Src[1], Op->Src[2]);
  if (Op->Dest.Data.GPR.GPR == Op->Src[0].Data.GPR.GPR) {
    Result = _VMov(OpSize::i128Bit, Result);
  }
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::VPINSRDQOp(OpcodeArgs) {
  const auto SrcSize = OpSizeFromSrc(Op);
  Ref Result = PINSROpImpl(Op, SrcSize, Op->Src[0], Op->Src[1], Op->Src[2]);
  if (Op->Dest.Data.GPR.GPR == Op->Src[0].Data.GPR.GPR) {
    Result = _VMov(OpSize::i128Bit, Result);
  }
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::VPINSRWOp(OpcodeArgs) {
  Ref Result = PINSROpImpl(Op, OpSize::i16Bit, Op->Src[0], Op->Src[1], Op->Src[2]);
  if (Op->Dest.Data.GPR.GPR == Op->Src[0].Data.GPR.GPR) {
    Result = _VMov(OpSize::i128Bit, Result);
  }
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

Ref OpDispatchBuilder::InsertPSOpImpl(OpcodeArgs, const X86Tables::DecodedOperand& Src1, const X86Tables::DecodedOperand& Src2,
                                      const X86Tables::DecodedOperand& Imm) {
  const uint8_t ImmValue = Imm.Literal();
  uint8_t CountS = (ImmValue >> 6);
  uint8_t CountD = (ImmValue >> 4) & 0b11;
  const uint8_t ZMask = ImmValue & 0xF;

  const auto DstSize = OpSizeFromDst(Op);

  Ref Dest {};
  if (ZMask != 0xF) {
    // Only need to load destination if it isn't a full zero
    Dest = LoadSource_WithOpSize(FPRClass, Op, Src1, DstSize, Op->Flags);
  }

  if ((ZMask & (1 << CountD)) == 0) {
    // In the case that ZMask overwrites the destination element, then don't even insert
    Ref Src {};
    if (Src2.IsGPR()) {
      Src = LoadSource(FPRClass, Op, Src2, Op->Flags);
    } else {
      // If loading from memory then CountS is forced to zero
      CountS = 0;
      Src = LoadSource_WithOpSize(FPRClass, Op, Src2, OpSize::i32Bit, Op->Flags);
    }

    Dest = _VInsElement(DstSize, OpSize::i32Bit, CountD, CountS, Dest, Src);
  }

  // ZMask happens after insert
  if (ZMask == 0xF) {
    return LoadZeroVector(DstSize);
  }

  if (ZMask) {
    auto Zero = LoadZeroVector(DstSize);
    for (size_t i = 0; i < 4; ++i) {
      if ((ZMask & (1 << i)) != 0) {
        Dest = _VInsElement(DstSize, OpSize::i32Bit, i, 0, Dest, Zero);
      }
    }
  }

  return Dest;
}

void OpDispatchBuilder::InsertPSOp(OpcodeArgs) {
  Ref Result = InsertPSOpImpl(Op, Op->Dest, Op->Src[0], Op->Src[1]);
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::VINSERTPSOp(OpcodeArgs) {
  Ref Result = InsertPSOpImpl(Op, Op->Src[0], Op->Src[1], Op->Src[2]);
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::PExtrOp(OpcodeArgs, IR::OpSize ElementSize) {
  const auto DstSize = OpSizeFromDst(Op);

  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  uint64_t Index = Op->Src[1].Literal();

  // Fixup of 32-bit element size.
  // When the element size is 32-bit then it can be overriden as 64-bit because the encoding of PEXTRD/PEXTRQ
  // is the same except that REX.W or VEX.W is set to 1. Incredibly frustrating.
  // Use the destination size as the element size in this case.
  auto OverridenElementSize = ElementSize;
  if (ElementSize == OpSize::i32Bit) {
    OverridenElementSize = DstSize;
  }

  // AVX version only operates on 128-bit.
  const uint8_t NumElements = std::min<uint8_t>(GetSrcSize(Op), OpSize::i128Bit) / OverridenElementSize;
  Index &= NumElements - 1;

  if (Op->Dest.IsGPR()) {
    const auto GPRSize = CTX->GetGPROpSize();
    // Extract already zero extends the result.
    Ref Result = _VExtractToGPR(OpSize::i128Bit, OverridenElementSize, Src, Index);
    StoreResult_WithOpSize(GPRClass, Op, Op->Dest, Result, GPRSize, OpSize::iInvalid);
    return;
  }

  // If we are storing to memory then we store the size of the element extracted
  Ref Dest = MakeSegmentAddress(Op, Op->Dest);
  _VStoreVectorElement(OpSize::i128Bit, OverridenElementSize, Src, Index, Dest);
}

void OpDispatchBuilder::VEXTRACT128Op(OpcodeArgs) {
  const auto DstIsXMM = Op->Dest.IsGPR();
  const auto StoreSize = DstIsXMM ? OpSize::i256Bit : OpSize::i128Bit;
  const auto Selector = Op->Src[1].Literal() & 0b1;

  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

  // A selector of zero is the same as doing a 128-bit vector move.
  if (Selector == 0) {
    Ref Result = DstIsXMM ? _VMov(OpSize::i128Bit, Src) : Src;
    StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Result, StoreSize, OpSize::iInvalid);
    return;
  }

  // Otherwise replicate the element and only store the first 128-bits.
  Ref Result = _VDupElement(OpSize::i256Bit, OpSize::i128Bit, Src, Selector);
  if (DstIsXMM) {
    Result = _VMov(OpSize::i128Bit, Result);
  }
  StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Result, StoreSize, OpSize::iInvalid);
}

Ref OpDispatchBuilder::PSIGNImpl(OpcodeArgs, size_t ElementSize, Ref Src1, Ref Src2) {
  const auto Size = GetSrcSize(Op);

  Ref Control = _VSQSHL(Size, ElementSize, Src2, (ElementSize * 8) - 1);
  Control = _VSRSHR(Size, ElementSize, Control, (ElementSize * 8) - 1);
  return _VMul(Size, ElementSize, Src1, Control);
}

template<size_t ElementSize>
void OpDispatchBuilder::PSIGN(OpcodeArgs) {
  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  Ref Res = PSIGNImpl(Op, ElementSize, Dest, Src);

  StoreResult(FPRClass, Op, Res, OpSize::iInvalid);
}

template void OpDispatchBuilder::PSIGN<OpSize::i8Bit>(OpcodeArgs);
template void OpDispatchBuilder::PSIGN<OpSize::i16Bit>(OpcodeArgs);
template void OpDispatchBuilder::PSIGN<OpSize::i32Bit>(OpcodeArgs);

template<size_t ElementSize>
void OpDispatchBuilder::VPSIGN(OpcodeArgs) {
  Ref Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);
  Ref Res = PSIGNImpl(Op, ElementSize, Src1, Src2);

  StoreResult(FPRClass, Op, Res, OpSize::iInvalid);
}

template void OpDispatchBuilder::VPSIGN<OpSize::i8Bit>(OpcodeArgs);
template void OpDispatchBuilder::VPSIGN<OpSize::i16Bit>(OpcodeArgs);
template void OpDispatchBuilder::VPSIGN<OpSize::i32Bit>(OpcodeArgs);

Ref OpDispatchBuilder::PSRLDOpImpl(OpcodeArgs, size_t ElementSize, Ref Src, Ref ShiftVec) {
  const auto Size = GetSrcSize(Op);

  // Incoming element size for the shift source is always 8
  return _VUShrSWide(Size, ElementSize, Src, ShiftVec);
}

void OpDispatchBuilder::PSRLDOp(OpcodeArgs, size_t ElementSize) {
  Ref Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Result = PSRLDOpImpl(Op, ElementSize, Dest, Src);

  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::VPSRLDOp(OpcodeArgs, size_t ElementSize) {
  const auto DstSize = GetDstSize(Op);
  const auto Is128Bit = DstSize == Core::CPUState::XMM_SSE_REG_SIZE;

  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Shift = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);
  Ref Result = PSRLDOpImpl(Op, ElementSize, Src, Shift);

  if (Is128Bit) {
    Result = _VMov(OpSize::i128Bit, Result);
  }
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::PSRLI(OpcodeArgs, size_t ElementSize) {
  const uint64_t ShiftConstant = Op->Src[1].Literal();
  if (ShiftConstant == 0) [[unlikely]] {
    // Nothing to do, value is already in Dest.
    return;
  }

  const auto Size = GetSrcSize(Op);

  Ref Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  Ref Shift = _VUShrI(Size, ElementSize, Dest, ShiftConstant);
  StoreResult(FPRClass, Op, Shift, OpSize::iInvalid);
}

void OpDispatchBuilder::VPSRLIOp(OpcodeArgs, size_t ElementSize) {
  const auto Size = GetSrcSize(Op);
  const auto Is128Bit = Size == Core::CPUState::XMM_SSE_REG_SIZE;
  const uint64_t ShiftConstant = Op->Src[1].Literal();

  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Result = Src;

  if (ShiftConstant != 0) [[likely]] {
    Result = _VUShrI(Size, ElementSize, Src, ShiftConstant);
  } else {
    if (Is128Bit) {
      Result = _VMov(OpSize::i128Bit, Result);
    }
  }

  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

Ref OpDispatchBuilder::PSLLIImpl(OpcodeArgs, size_t ElementSize, Ref Src, uint64_t Shift) {
  if (Shift == 0) [[unlikely]] {
    // If zero-shift then just return the source.
    return Src;
  }
  const auto Size = GetSrcSize(Op);
  return _VShlI(Size, ElementSize, Src, Shift);
}

void OpDispatchBuilder::PSLLI(OpcodeArgs, size_t ElementSize) {
  const uint64_t ShiftConstant = Op->Src[1].Literal();
  if (ShiftConstant == 0) [[unlikely]] {
    // Nothing to do, value is already in Dest.
    return;
  }

  Ref Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  Ref Result = PSLLIImpl(Op, ElementSize, Dest, ShiftConstant);

  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::VPSLLIOp(OpcodeArgs, size_t ElementSize) {
  const uint64_t ShiftConstant = Op->Src[1].Literal();
  const auto DstSize = GetDstSize(Op);
  const auto Is128Bit = DstSize == Core::CPUState::XMM_SSE_REG_SIZE;

  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Result = PSLLIImpl(Op, ElementSize, Src, ShiftConstant);
  if (ShiftConstant == 0 && Is128Bit) {
    Result = _VMov(OpSize::i128Bit, Result);
  }

  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

Ref OpDispatchBuilder::PSLLImpl(OpcodeArgs, size_t ElementSize, Ref Src, Ref ShiftVec) {
  const auto Size = GetDstSize(Op);

  // Incoming element size for the shift source is always 8
  return _VUShlSWide(Size, ElementSize, Src, ShiftVec);
}

void OpDispatchBuilder::PSLL(OpcodeArgs, size_t ElementSize) {
  Ref Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Result = PSLLImpl(Op, ElementSize, Dest, Src);

  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::VPSLLOp(OpcodeArgs, size_t ElementSize) {
  const auto DstSize = GetDstSize(Op);
  const auto Is128Bit = DstSize == Core::CPUState::XMM_SSE_REG_SIZE;

  Ref Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Src2 = LoadSource_WithOpSize(FPRClass, Op, Op->Src[1], OpSize::i128Bit, Op->Flags);
  Ref Result = PSLLImpl(Op, ElementSize, Src1, Src2);

  if (Is128Bit) {
    Result = _VMov(OpSize::i128Bit, Result);
  }
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

Ref OpDispatchBuilder::PSRAOpImpl(OpcodeArgs, size_t ElementSize, Ref Src, Ref ShiftVec) {
  const auto Size = GetDstSize(Op);

  // Incoming element size for the shift source is always 8
  return _VSShrSWide(Size, ElementSize, Src, ShiftVec);
}

void OpDispatchBuilder::PSRAOp(OpcodeArgs, size_t ElementSize) {
  Ref Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Result = PSRAOpImpl(Op, ElementSize, Dest, Src);

  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::VPSRAOp(OpcodeArgs, size_t ElementSize) {
  const auto DstSize = GetDstSize(Op);
  const auto Is128Bit = DstSize == Core::CPUState::XMM_SSE_REG_SIZE;

  Ref Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);
  Ref Result = PSRAOpImpl(Op, ElementSize, Src1, Src2);

  if (Is128Bit) {
    Result = _VMov(OpSize::i128Bit, Result);
  }
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::PSRLDQ(OpcodeArgs) {
  const uint64_t Shift = Op->Src[1].Literal();
  if (Shift == 0) [[unlikely]] {
    // Nothing to do, value is already in Dest.
    return;
  }

  const auto Size = GetDstSize(Op);

  Ref Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  Ref Result = LoadZeroVector(Size);

  if (Shift < Size) {
    Result = _VExtr(Size, OpSize::i8Bit, Result, Dest, Shift);
  }
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::VPSRLDQOp(OpcodeArgs) {
  const auto DstSize = GetDstSize(Op);
  const auto Is128Bit = DstSize == Core::CPUState::XMM_SSE_REG_SIZE;
  const uint64_t Shift = Op->Src[1].Literal();

  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

  Ref Result {};
  if (Shift == 0) [[unlikely]] {
    if (Is128Bit) {
      Result = _VMov(OpSize::i128Bit, Src);
    } else {
      Result = Src;
    }
  } else {
    Result = LoadZeroVector(DstSize);

    if (Is128Bit) {
      if (Shift < DstSize) {
        Result = _VExtr(DstSize, OpSize::i8Bit, Result, Src, Shift);
      }
    } else {
      if (Shift < Core::CPUState::XMM_SSE_REG_SIZE) {
        Ref ResultBottom = _VExtr(OpSize::i128Bit, 1, Result, Src, Shift);
        Ref ResultTop = _VExtr(DstSize, OpSize::i8Bit, Result, Src, 16 + Shift);

        Result = _VInsElement(DstSize, OpSize::i128Bit, 1, 0, ResultBottom, ResultTop);
      }
    }
  }

  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::PSLLDQ(OpcodeArgs) {
  const uint64_t Shift = Op->Src[1].Literal();
  if (Shift == 0) [[unlikely]] {
    // Nothing to do, value is already in Dest.
    return;
  }

  const auto Size = GetDstSize(Op);

  Ref Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  Ref Result = LoadZeroVector(Size);
  if (Shift < Size) {
    Result = _VExtr(Size, OpSize::i8Bit, Dest, Result, Size - Shift);
  }

  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::VPSLLDQOp(OpcodeArgs) {
  const auto DstSize = GetDstSize(Op);
  const auto Is128Bit = DstSize == Core::CPUState::XMM_SSE_REG_SIZE;
  const uint64_t Shift = Op->Src[1].Literal();

  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

  Ref Result = Src;

  if (Shift == 0) {
    if (Is128Bit) {
      Result = _VMov(OpSize::i128Bit, Result);
    }
  } else {
    Result = LoadZeroVector(DstSize);
    if (Is128Bit) {
      if (Shift < DstSize) {
        Result = _VExtr(DstSize, OpSize::i8Bit, Src, Result, DstSize - Shift);
      }
    } else {
      if (Shift < Core::CPUState::XMM_SSE_REG_SIZE) {
        Ref ResultBottom = _VExtr(OpSize::i128Bit, OpSize::i8Bit, Src, Result, 16 - Shift);
        Ref ResultTop = _VExtr(DstSize, OpSize::i8Bit, Src, Result, DstSize - Shift);

        Result = _VInsElement(DstSize, OpSize::i128Bit, 1, 0, ResultBottom, ResultTop);
      }
    }
  }
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::PSRAIOp(OpcodeArgs, size_t ElementSize) {
  const uint64_t Shift = Op->Src[1].Literal();
  if (Shift == 0) [[unlikely]] {
    // Nothing to do, value is already in Dest.
    return;
  }

  const auto Size = GetDstSize(Op);

  Ref Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  Ref Result = _VSShrI(Size, ElementSize, Dest, Shift);
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::VPSRAIOp(OpcodeArgs, size_t ElementSize) {
  const uint64_t Shift = Op->Src[1].Literal();
  const auto Size = GetDstSize(Op);
  const auto Is128Bit = Size == Core::CPUState::XMM_SSE_REG_SIZE;

  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Result = Src;

  if (Shift != 0) [[likely]] {
    Result = _VSShrI(Size, ElementSize, Src, Shift);
  } else {
    if (Is128Bit) {
      Result = _VMov(OpSize::i128Bit, Result);
    }
  }

  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::AVXVariableShiftImpl(OpcodeArgs, IROps IROp) {
  const auto DstSize = OpSizeFromDst(Op);
  const auto SrcSize = OpSizeFromSrc(Op);

  Ref Vector = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], DstSize, Op->Flags);
  Ref ShiftVector = LoadSource_WithOpSize(FPRClass, Op, Op->Src[1], DstSize, Op->Flags);

  DeriveOp(Shift, IROp, _VUShr(DstSize, SrcSize, Vector, ShiftVector, true));

  StoreResult(FPRClass, Op, Shift, OpSize::iInvalid);
}

void OpDispatchBuilder::VPSLLVOp(OpcodeArgs) {
  AVXVariableShiftImpl(Op, IROps::OP_VUSHL);
}

void OpDispatchBuilder::VPSRAVDOp(OpcodeArgs) {
  AVXVariableShiftImpl(Op, IROps::OP_VSSHR);
}

void OpDispatchBuilder::VPSRLVOp(OpcodeArgs) {
  AVXVariableShiftImpl(Op, IROps::OP_VUSHR);
}

void OpDispatchBuilder::MOVDDUPOp(OpcodeArgs) {
  // If loading a vector, use the full size, so we don't
  // unnecessarily zero extend the vector. Otherwise, if
  // memory, then we want to load the element size exactly.
  const auto SrcSize = Op->Src[0].IsGPR() ? OpSize::i128Bit : OpSizeFromSrc(Op);
  Ref Src = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], SrcSize, Op->Flags);
  Ref Res = _VDupElement(OpSize::i128Bit, GetSrcSize(Op), Src, 0);

  StoreResult(FPRClass, Op, Res, OpSize::iInvalid);
}

void OpDispatchBuilder::VMOVDDUPOp(OpcodeArgs) {
  const auto SrcSize = OpSizeFromSrc(Op);
  const auto IsSrcGPR = Op->Src[0].IsGPR();
  const auto Is256Bit = SrcSize == Core::CPUState::XMM_AVX_REG_SIZE;
  const auto MemSize = Is256Bit ? OpSize::i256Bit : OpSize::i64Bit;

  Ref Src = IsSrcGPR ? LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], SrcSize, Op->Flags) :
                       LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], MemSize, Op->Flags);

  Ref Res {};
  if (Is256Bit) {
    Res = _VTrn(SrcSize, OpSize::i64Bit, Src, Src);
  } else {
    Res = _VDupElement(SrcSize, OpSize::i64Bit, Src, 0);
  }

  StoreResult(FPRClass, Op, Res, OpSize::iInvalid);
}

Ref OpDispatchBuilder::CVTGPR_To_FPRImpl(OpcodeArgs, size_t DstElementSize, const X86Tables::DecodedOperand& Src1Op,
                                         const X86Tables::DecodedOperand& Src2Op) {
  const auto SrcSize = GetSrcSize(Op);

  Ref Src1 = LoadSource_WithOpSize(FPRClass, Op, Src1Op, OpSize::i128Bit, Op->Flags);
  Ref Converted {};
  if (Src2Op.IsGPR()) {
    // If the source is a GPR then convert directly from the GPR.
    auto Src2 = LoadSource_WithOpSize(GPRClass, Op, Src2Op, CTX->GetGPROpSize(), Op->Flags);
    Converted = _Float_FromGPR_S(DstElementSize, SrcSize, Src2);
  } else if (SrcSize != DstElementSize) {
    // If the source is from memory but the Source size and destination size aren't the same,
    // then it is more optimal to load in to a GPR and convert between GPR->FPR.
    // ARM GPR->FPR conversion supports different size source and destinations while FPR->FPR doesn't.
    auto Src2 = LoadSource(GPRClass, Op, Src2Op, Op->Flags);
    Converted = _Float_FromGPR_S(DstElementSize, SrcSize, Src2);
  } else {
    // In the case of cvtsi2s{s,d} where the source and destination are the same size,
    // then it is more optimal to load in to the FPR register directly and convert there.
    auto Src2 = LoadSource(FPRClass, Op, Src2Op, Op->Flags);
    Converted = _Vector_SToF(SrcSize, SrcSize, Src2);
  }

  return _VInsElement(OpSize::i128Bit, DstElementSize, 0, 0, Src1, Converted);
}

template<size_t DstElementSize>
void OpDispatchBuilder::CVTGPR_To_FPR(OpcodeArgs) {
  Ref Result = CVTGPR_To_FPRImpl(Op, DstElementSize, Op->Dest, Op->Src[0]);
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

template void OpDispatchBuilder::CVTGPR_To_FPR<OpSize::i32Bit>(OpcodeArgs);
template void OpDispatchBuilder::CVTGPR_To_FPR<OpSize::i64Bit>(OpcodeArgs);

template<size_t DstElementSize>
void OpDispatchBuilder::AVXCVTGPR_To_FPR(OpcodeArgs) {
  Ref Result = CVTGPR_To_FPRImpl(Op, DstElementSize, Op->Src[0], Op->Src[1]);
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}
template void OpDispatchBuilder::AVXCVTGPR_To_FPR<OpSize::i32Bit>(OpcodeArgs);
template void OpDispatchBuilder::AVXCVTGPR_To_FPR<OpSize::i64Bit>(OpcodeArgs);

template<IR::OpSize SrcElementSize, bool HostRoundingMode>
void OpDispatchBuilder::CVTFPR_To_GPR(OpcodeArgs) {
  // If loading a vector, use the full size, so we don't
  // unnecessarily zero extend the vector. Otherwise, if
  // memory, then we want to load the element size exactly.
  const auto SrcSize = Op->Src[0].IsGPR() ? OpSize::i128Bit : OpSizeFromSrc(Op);
  Ref Src = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], SrcSize, Op->Flags);

  // GPR size is determined by REX.W
  // Source Element size is determined by instruction
  const auto GPRSize = OpSizeFromDst(Op);

  if constexpr (HostRoundingMode) {
    Src = _Float_ToGPR_S(GPRSize, SrcElementSize, Src);
  } else {
    Src = _Float_ToGPR_ZS(GPRSize, SrcElementSize, Src);
  }

  StoreResult_WithOpSize(GPRClass, Op, Op->Dest, Src, GPRSize, OpSize::iInvalid);
}

template void OpDispatchBuilder::CVTFPR_To_GPR<OpSize::i32Bit, true>(OpcodeArgs);
template void OpDispatchBuilder::CVTFPR_To_GPR<OpSize::i32Bit, false>(OpcodeArgs);

template void OpDispatchBuilder::CVTFPR_To_GPR<OpSize::i64Bit, true>(OpcodeArgs);
template void OpDispatchBuilder::CVTFPR_To_GPR<OpSize::i64Bit, false>(OpcodeArgs);

Ref OpDispatchBuilder::Vector_CVT_Int_To_FloatImpl(OpcodeArgs, size_t SrcElementSize, bool Widen) {
  const auto Size = OpSizeFromDst(Op);

  Ref Src = [&] {
    if (Widen) {
      // If loading a vector, use the full size, so we don't
      // unnecessarily zero extend the vector. Otherwise, if
      // memory, then we want to load the element size exactly.
      const auto LoadSize = Op->Src[0].IsGPR() ? OpSize::i128Bit : IR::SizeToOpSize(8 * (IR::OpSizeToSize(Size) / 16));
      return LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], LoadSize, Op->Flags);
    } else {
      return LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
    }
  }();

  size_t ElementSize = SrcElementSize;
  if (Widen) {
    Src = _VSXTL(Size, ElementSize, Src);
    ElementSize <<= 1;
  }

  return _Vector_SToF(Size, ElementSize, Src);
}

template<size_t SrcElementSize, bool Widen>
void OpDispatchBuilder::Vector_CVT_Int_To_Float(OpcodeArgs) {
  Ref Result = Vector_CVT_Int_To_FloatImpl(Op, SrcElementSize, Widen);
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

template void OpDispatchBuilder::Vector_CVT_Int_To_Float<OpSize::i32Bit, true>(OpcodeArgs);
template void OpDispatchBuilder::Vector_CVT_Int_To_Float<OpSize::i32Bit, false>(OpcodeArgs);

template<size_t SrcElementSize, bool Widen>
void OpDispatchBuilder::AVXVector_CVT_Int_To_Float(OpcodeArgs) {
  Ref Result = Vector_CVT_Int_To_FloatImpl(Op, SrcElementSize, Widen);
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

template void OpDispatchBuilder::AVXVector_CVT_Int_To_Float<OpSize::i32Bit, false>(OpcodeArgs);
template void OpDispatchBuilder::AVXVector_CVT_Int_To_Float<OpSize::i32Bit, true>(OpcodeArgs);

Ref OpDispatchBuilder::Vector_CVT_Float_To_IntImpl(OpcodeArgs, size_t SrcElementSize, bool Narrow, bool HostRoundingMode) {
  const size_t DstSize = GetDstSize(Op);
  size_t ElementSize = SrcElementSize;

  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

  if (Narrow) {
    Src = _Vector_FToF(DstSize, SrcElementSize >> 1, Src, SrcElementSize);
    ElementSize >>= 1;
  }

  if (HostRoundingMode) {
    return _Vector_FToS(DstSize, ElementSize, Src);
  } else {
    return _Vector_FToZS(DstSize, ElementSize, Src);
  }
}

template<size_t SrcElementSize, bool Narrow, bool HostRoundingMode>
void OpDispatchBuilder::Vector_CVT_Float_To_Int(OpcodeArgs) {
  const auto DstSize = OpSizeFromDst(Op);

  Ref Result {};
  if (SrcElementSize == OpSize::i64Bit && Narrow) {
    ///< Special case for CVTTPD2DQ because it has weird rounding requirements.
    Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
    Result = _Vector_F64ToI32(DstSize, Src, HostRoundingMode ? Round_Host : Round_Towards_Zero, true);
  } else {
    Result = Vector_CVT_Float_To_IntImpl(Op, SrcElementSize, Narrow, HostRoundingMode);
  }

  StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Result, DstSize, OpSize::iInvalid);
}

template void OpDispatchBuilder::Vector_CVT_Float_To_Int<OpSize::i32Bit, false, false>(OpcodeArgs);
template void OpDispatchBuilder::Vector_CVT_Float_To_Int<OpSize::i32Bit, false, true>(OpcodeArgs);
template void OpDispatchBuilder::Vector_CVT_Float_To_Int<OpSize::i32Bit, true, false>(OpcodeArgs);

template void OpDispatchBuilder::Vector_CVT_Float_To_Int<OpSize::i64Bit, true, true>(OpcodeArgs);
template void OpDispatchBuilder::Vector_CVT_Float_To_Int<OpSize::i64Bit, true, false>(OpcodeArgs);

template<size_t SrcElementSize, bool Narrow, bool HostRoundingMode>
void OpDispatchBuilder::AVXVector_CVT_Float_To_Int(OpcodeArgs) {
  const auto DstSize = OpSizeFromDst(Op);

  Ref Result {};
  if (SrcElementSize == OpSize::i64Bit && Narrow) {
    ///< Special case for CVTPD2DQ/CVTTPD2DQ because it has weird rounding requirements.
    Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
    Result = _Vector_F64ToI32(DstSize, Src, HostRoundingMode ? Round_Host : Round_Towards_Zero, true);
  } else {
    Result = Vector_CVT_Float_To_IntImpl(Op, SrcElementSize, Narrow, HostRoundingMode);
  }

  StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Result, DstSize, OpSize::iInvalid);
}

template void OpDispatchBuilder::AVXVector_CVT_Float_To_Int<OpSize::i32Bit, false, false>(OpcodeArgs);
template void OpDispatchBuilder::AVXVector_CVT_Float_To_Int<OpSize::i32Bit, false, true>(OpcodeArgs);

template void OpDispatchBuilder::AVXVector_CVT_Float_To_Int<OpSize::i64Bit, true, false>(OpcodeArgs);
template void OpDispatchBuilder::AVXVector_CVT_Float_To_Int<OpSize::i64Bit, true, true>(OpcodeArgs);

Ref OpDispatchBuilder::Scalar_CVT_Float_To_FloatImpl(OpcodeArgs, IR::OpSize DstElementSize, IR::OpSize SrcElementSize,
                                                     const X86Tables::DecodedOperand& Src1Op, const X86Tables::DecodedOperand& Src2Op) {
  // In the case of vectors, we can just specify the full vector length,
  // so that we don't unnecessarily zero-extend the entire vector.
  // Otherwise, if it's a memory load, then we only want to load its exact size.
  const auto Src2Size = Src2Op.IsGPR() ? OpSize::i128Bit : SrcElementSize;

  Ref Src1 = LoadSource_WithOpSize(FPRClass, Op, Src1Op, OpSize::i128Bit, Op->Flags);
  Ref Src2 = LoadSource_WithOpSize(FPRClass, Op, Src2Op, Src2Size, Op->Flags);

  Ref Converted = _Float_FToF(DstElementSize, SrcElementSize, Src2);

  return _VInsElement(OpSize::i128Bit, DstElementSize, 0, 0, Src1, Converted);
}

template<IR::OpSize DstElementSize, IR::OpSize SrcElementSize>
void OpDispatchBuilder::Scalar_CVT_Float_To_Float(OpcodeArgs) {
  Ref Result = Scalar_CVT_Float_To_FloatImpl(Op, DstElementSize, SrcElementSize, Op->Dest, Op->Src[0]);
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

template void OpDispatchBuilder::Scalar_CVT_Float_To_Float<OpSize::i32Bit, OpSize::i64Bit>(OpcodeArgs);
template void OpDispatchBuilder::Scalar_CVT_Float_To_Float<OpSize::i64Bit, OpSize::i32Bit>(OpcodeArgs);

template<IR::OpSize DstElementSize, IR::OpSize SrcElementSize>
void OpDispatchBuilder::AVXScalar_CVT_Float_To_Float(OpcodeArgs) {
  Ref Result = Scalar_CVT_Float_To_FloatImpl(Op, DstElementSize, SrcElementSize, Op->Src[0], Op->Src[1]);
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

template void OpDispatchBuilder::AVXScalar_CVT_Float_To_Float<OpSize::i32Bit, OpSize::i64Bit>(OpcodeArgs);
template void OpDispatchBuilder::AVXScalar_CVT_Float_To_Float<OpSize::i64Bit, OpSize::i32Bit>(OpcodeArgs);

void OpDispatchBuilder::Vector_CVT_Float_To_Float(OpcodeArgs, size_t DstElementSize, size_t SrcElementSize, bool IsAVX) {
  const auto SrcSize = OpSizeFromSrc(Op);

  const auto IsFloatSrc = SrcElementSize == OpSize::i32Bit;
  const auto Is128Bit = SrcSize == Core::CPUState::XMM_SSE_REG_SIZE;

  const auto LoadSize = IsFloatSrc && !Op->Src[0].IsGPR() ? IR::SizeToOpSize(IR::OpSizeToSize(SrcSize) / 2) : SrcSize;

  Ref Src = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], LoadSize, Op->Flags);

  Ref Result {};
  if (DstElementSize > SrcElementSize) {
    Result = _Vector_FToF(SrcSize, SrcElementSize << 1, Src, SrcElementSize);
  } else {
    Result = _Vector_FToF(SrcSize, SrcElementSize >> 1, Src, SrcElementSize);
  }

  if (IsAVX) {
    if (!IsFloatSrc && !Is128Bit) {
      // VCVTPD2PS path
      Result = _VMov(OpSize::i128Bit, Result);
    } else if (IsFloatSrc && Is128Bit) {
      // VCVTPS2PD path
      Result = _VMov(OpSize::i128Bit, Result);
    }
  }
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::MMX_To_XMM_Vector_CVT_Int_To_Float(OpcodeArgs) {
  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

  // Always 32-bit.
  size_t ElementSize = OpSize::i32Bit;
  size_t DstSize = GetDstSize(Op);

  Src = _VSXTL(DstSize, ElementSize, Src);
  ElementSize <<= 1;

  // Always signed
  Src = _Vector_SToF(DstSize, ElementSize, Src);

  StoreResult(FPRClass, Op, Src, OpSize::iInvalid);
}

template<size_t SrcElementSize, bool Narrow, bool HostRoundingMode>
void OpDispatchBuilder::XMM_To_MMX_Vector_CVT_Float_To_Int(OpcodeArgs) {
  // This function causes a change in MMX state from X87 to MMX
  if (MMXState == MMXState_X87) {
    ChgStateX87_MMX();
  }

  // If loading a vector, use the full size, so we don't
  // unnecessarily zero extend the vector. Otherwise, if
  // memory, then we want to load the element size exactly.
  const auto SrcSize = Op->Src[0].IsGPR() ? OpSize::i128Bit : OpSizeFromSrc(Op);
  Ref Src = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], SrcSize, Op->Flags);

  size_t ElementSize = SrcElementSize;
  const auto Size = OpSizeFromDst(Op);

  if (Narrow) {
    Src = _Vector_FToF(Size, SrcElementSize >> 1, Src, SrcElementSize);
    ElementSize >>= 1;
  }

  if constexpr (HostRoundingMode) {
    Src = _Vector_FToS(Size, ElementSize, Src);
  } else {
    Src = _Vector_FToZS(Size, ElementSize, Src);
  }

  StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Src, Size, OpSize::iInvalid);
}

template void OpDispatchBuilder::XMM_To_MMX_Vector_CVT_Float_To_Int<OpSize::i32Bit, false, false>(OpcodeArgs);
template void OpDispatchBuilder::XMM_To_MMX_Vector_CVT_Float_To_Int<OpSize::i32Bit, false, true>(OpcodeArgs);
template void OpDispatchBuilder::XMM_To_MMX_Vector_CVT_Float_To_Int<OpSize::i64Bit, true, false>(OpcodeArgs);
template void OpDispatchBuilder::XMM_To_MMX_Vector_CVT_Float_To_Int<OpSize::i64Bit, true, true>(OpcodeArgs);

void OpDispatchBuilder::MASKMOVOp(OpcodeArgs) {
  const auto Size = OpSizeFromSrc(Op);

  Ref MaskSrc = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
  // Mask only cares about the top bit of each byte
  MaskSrc = _VCMPLTZ(Size, OpSize::i8Bit, MaskSrc);

  // Vector that will overwrite byte elements.
  Ref VectorSrc = LoadSource(GPRClass, Op, Op->Dest, Op->Flags);

  // RDI source (DS prefix by default)
  auto MemDest = MakeSegmentAddress(X86State::REG_RDI, Op->Flags, X86Tables::DecodeFlags::FLAG_DS_PREFIX);

  Ref XMMReg = _LoadMem(FPRClass, Size, MemDest, OpSize::i8Bit);

  // If the Mask element high bit is set then overwrite the element with the source, else keep the memory variant
  XMMReg = _VBSL(Size, MaskSrc, VectorSrc, XMMReg);
  _StoreMem(FPRClass, Size, MemDest, XMMReg, OpSize::i8Bit);
}

void OpDispatchBuilder::VMASKMOVOpImpl(OpcodeArgs, IR::OpSize ElementSize, IR::OpSize DataSize, bool IsStore,
                                       const X86Tables::DecodedOperand& MaskOp, const X86Tables::DecodedOperand& DataOp) {

  const auto MakeAddress = [this, Op](const X86Tables::DecodedOperand& Data) {
    return MakeSegmentAddress(Op, Data, CTX->GetGPROpSize());
  };

  Ref Mask = LoadSource_WithOpSize(FPRClass, Op, MaskOp, DataSize, Op->Flags);

  if (IsStore) {
    Ref Data = LoadSource_WithOpSize(FPRClass, Op, DataOp, DataSize, Op->Flags);
    Ref Address = MakeAddress(Op->Dest);
    _VStoreVectorMasked(DataSize, ElementSize, Mask, Data, Address, Invalid(), MEM_OFFSET_SXTX, 1);
  } else {
    const auto Is128Bit = GetDstSize(Op) == Core::CPUState::XMM_SSE_REG_SIZE;

    Ref Address = MakeAddress(DataOp);
    Ref Result = _VLoadVectorMasked(DataSize, ElementSize, Mask, Address, Invalid(), MEM_OFFSET_SXTX, 1);

    if (Is128Bit) {
      Result = _VMov(OpSize::i128Bit, Result);
    }
    StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
  }
}

template<IR::OpSize ElementSize, bool IsStore>
void OpDispatchBuilder::VMASKMOVOp(OpcodeArgs) {
  VMASKMOVOpImpl(Op, ElementSize, OpSizeFromDst(Op), IsStore, Op->Src[0], Op->Src[1]);
}
template void OpDispatchBuilder::VMASKMOVOp<OpSize::i32Bit, false>(OpcodeArgs);
template void OpDispatchBuilder::VMASKMOVOp<OpSize::i32Bit, true>(OpcodeArgs);
template void OpDispatchBuilder::VMASKMOVOp<OpSize::i64Bit, false>(OpcodeArgs);
template void OpDispatchBuilder::VMASKMOVOp<OpSize::i64Bit, true>(OpcodeArgs);

template<bool IsStore>
void OpDispatchBuilder::VPMASKMOVOp(OpcodeArgs) {
  VMASKMOVOpImpl(Op, OpSizeFromSrc(Op), OpSizeFromDst(Op), IsStore, Op->Src[0], Op->Src[1]);
}
template void OpDispatchBuilder::VPMASKMOVOp<false>(OpcodeArgs);
template void OpDispatchBuilder::VPMASKMOVOp<true>(OpcodeArgs);

void OpDispatchBuilder::MOVBetweenGPR_FPR(OpcodeArgs, VectorOpType VectorType) {
  if (Op->Dest.IsGPR() && Op->Dest.Data.GPR.GPR >= FEXCore::X86State::REG_XMM_0) {
    Ref Result {};
    if (Op->Src[0].IsGPR()) {
      // Loading from GPR and moving to Vector.
      Ref Src = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], CTX->GetGPROpSize(), Op->Flags);
      // zext to 128bit
      Result = _VCastFromGPR(OpSize::i128Bit, GetSrcSize(Op), Src);
    } else {
      // Loading from Memory as a scalar. Zero extend
      Result = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
    }

    StoreResult_WithAVXInsert(VectorType, FPRClass, Op, Result, OpSize::iInvalid);
  } else {
    Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

    if (Op->Dest.IsGPR()) {
      const auto ElementSize = OpSizeFromDst(Op);
      // Extract element from GPR. Zero extending in the process.
      Src = _VExtractToGPR(OpSizeFromSrc(Op), ElementSize, Src, 0);
      StoreResult(GPRClass, Op, Op->Dest, Src, OpSize::iInvalid);
    } else {
      // Storing first element to memory.
      Ref Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.LoadData = false});
      _StoreMem(FPRClass, OpSizeFromDst(Op), Dest, Src, OpSize::i8Bit);
    }
  }
}

Ref OpDispatchBuilder::VFCMPOpImpl(OpSize Size, size_t ElementSize, Ref Src1, Ref Src2, uint8_t CompType) {
  Ref Result {};
  switch (CompType & 0x7) {
  case 0x0: // EQ
    return _VFCMPEQ(Size, ElementSize, Src1, Src2);
  case 0x1: // LT, GT(Swapped operand)
    return _VFCMPLT(Size, ElementSize, Src1, Src2);
  case 0x2: // LE, GE(Swapped operand)
    return _VFCMPLE(Size, ElementSize, Src1, Src2);
  case 0x3: // Unordered
    return _VFCMPUNO(Size, ElementSize, Src1, Src2);
  case 0x4: // NEQ
    return _VFCMPNEQ(Size, ElementSize, Src1, Src2);
  case 0x5: // NLT, NGT(Swapped operand)
    Result = _VFCMPLT(Size, ElementSize, Src1, Src2);
    return _VNot(Size, ElementSize, Result);
  case 0x6: // NLE, NGE(Swapped operand)
    Result = _VFCMPLE(Size, ElementSize, Src1, Src2);
    return _VNot(Size, ElementSize, Result);
  case 0x7: // Ordered
    return _VFCMPORD(Size, ElementSize, Src1, Src2);
  }
  FEX_UNREACHABLE;
}

template<size_t ElementSize>
void OpDispatchBuilder::VFCMPOp(OpcodeArgs) {
  // No need for zero-extending in the scalar case, since
  // all we need is an insert at the end of the operation.
  const auto SrcSize = OpSizeFromSrc(Op);
  const auto DstSize = OpSizeFromDst(Op);

  Ref Src = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], SrcSize, Op->Flags);
  Ref Dest = LoadSource_WithOpSize(FPRClass, Op, Op->Dest, DstSize, Op->Flags);
  const uint8_t CompType = Op->Src[1].Data.Literal.Value;

  Ref Result = VFCMPOpImpl(OpSizeFromSrc(Op), ElementSize, Dest, Src, CompType);

  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

template void OpDispatchBuilder::VFCMPOp<OpSize::i32Bit>(OpcodeArgs);
template void OpDispatchBuilder::VFCMPOp<OpSize::i64Bit>(OpcodeArgs);

template<size_t ElementSize>
void OpDispatchBuilder::AVXVFCMPOp(OpcodeArgs) {
  // No need for zero-extending in the scalar case, since
  // all we need is an insert at the end of the operation.
  const auto SrcSize = OpSizeFromSrc(Op);
  const auto DstSize = OpSizeFromDst(Op);
  const uint8_t CompType = Op->Src[2].Literal();

  Ref Src1 = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], DstSize, Op->Flags);
  Ref Src2 = LoadSource_WithOpSize(FPRClass, Op, Op->Src[1], SrcSize, Op->Flags);
  Ref Result = VFCMPOpImpl(OpSizeFromSrc(Op), ElementSize, Src1, Src2, CompType);

  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

template void OpDispatchBuilder::AVXVFCMPOp<OpSize::i32Bit>(OpcodeArgs);
template void OpDispatchBuilder::AVXVFCMPOp<OpSize::i64Bit>(OpcodeArgs);

void OpDispatchBuilder::FXSaveOp(OpcodeArgs) {
  Ref Mem = MakeSegmentAddress(Op, Op->Dest);

  SaveX87State(Op, Mem);
  SaveSSEState(Mem);
  SaveMXCSRState(Mem);
}

void OpDispatchBuilder::XSaveOp(OpcodeArgs) {
  XSaveOpImpl(Op);
}

Ref OpDispatchBuilder::XSaveBase(X86Tables::DecodedOp Op) {
  return MakeSegmentAddress(Op, Op->Dest);
}

void OpDispatchBuilder::XSaveOpImpl(OpcodeArgs) {
  // NOTE: Mask should be EAX and EDX concatenated, but we only need to test
  //       for features that are in the lower 32 bits, so EAX only is sufficient.
  const auto OpSize = IR::SizeToOpSize(CTX->GetGPRSize());

  const auto StoreIfFlagSet = [this, OpSize](uint32_t BitIndex, auto fn, uint32_t FieldSize = 1) {
    Ref Mask = LoadGPRRegister(X86State::REG_RAX);
    Ref BitFlag = _Bfe(OpSize, FieldSize, BitIndex, Mask);
    auto CondJump_ = CondJump(BitFlag, {COND_NEQ});

    auto StoreBlock = CreateNewCodeBlockAfter(GetCurrentBlock());
    SetTrueJumpTarget(CondJump_, StoreBlock);
    SetCurrentCodeBlock(StoreBlock);
    StartNewBlock();
    { fn(); }
    auto Jump_ = Jump();
    auto NextJumpTarget = CreateNewCodeBlockAfter(StoreBlock);
    SetJumpTarget(Jump_, NextJumpTarget);
    SetFalseJumpTarget(CondJump_, NextJumpTarget);
    SetCurrentCodeBlock(NextJumpTarget);
    StartNewBlock();
  };

  // x87
  {
    StoreIfFlagSet(0, [this, Op] { SaveX87State(Op, XSaveBase(Op)); });
  }
  // SSE
  {
    StoreIfFlagSet(1, [this, Op] { SaveSSEState(XSaveBase(Op)); });
  }
  // AVX
  if (CTX->HostFeatures.SupportsAVX) {
    StoreIfFlagSet(2, [this, Op] { std::invoke(SaveAVXStateFunc, this, XSaveBase(Op)); });
  }

  // We need to save MXCSR and MXCSR_MASK if either SSE or AVX are requested to be saved
  {
    StoreIfFlagSet(
      1, [this, Op] { SaveMXCSRState(XSaveBase(Op)); }, 2);
  }

  // Update XSTATE_BV region of the XSAVE header
  {
    Ref Base = XSaveBase(Op);

    // NOTE: We currently only support the first 3 bits (x87, SSE, and AVX)
    Ref Mask = LoadGPRRegister(X86State::REG_RAX);
    Ref RequestedFeatures = _Bfe(OpSize, 3, 0, Mask);

    // XSTATE_BV section of the header is 8 bytes in size, but we only really
    // care about setting at most 3 bits in the first byte. We zero out the rest.
    _StoreMem(GPRClass, OpSize::i64Bit, RequestedFeatures, Base, _Constant(512), OpSize::i8Bit, MEM_OFFSET_SXTX, 1);
  }
}

void OpDispatchBuilder::SaveX87State(OpcodeArgs, Ref MemBase) {
  // Saves 512bytes to the memory location provided
  // Header changes depending on if REX.W is set or not
  if (Op->Flags & X86Tables::DecodeFlags::FLAG_REX_WIDENING) {
    // BYTE | 0 1 | 2 3 | 4   | 5     | 6 7 | 8 9 | a b | c d | e f |
    // ------------------------------------------
    //   00 | FCW | FSW | FTW | <R>   | FOP | FIP                   |
    //   16 | FDP                           | MXCSR     | MXCSR_MASK|
  } else {
    // BYTE | 0 1 | 2 3 | 4   | 5     | 6 7 | 8 9 | a b | c d | e f |
    // ------------------------------------------
    //   00 | FCW | FSW | FTW | <R>   | FOP | FIP[31:0] | FCS | <R> |
    //   16 | FDP[31:0] | FDS         | <R> | MXCSR     | MXCSR_MASK|
  }

  {
    auto FCW = _LoadContext(OpSize::i16Bit, GPRClass, offsetof(FEXCore::Core::CPUState, FCW));
    _StoreMem(GPRClass, OpSize::i16Bit, MemBase, FCW, OpSize::i16Bit);
  }

  { _StoreMem(GPRClass, OpSize::i16Bit, ReconstructFSW_Helper(), MemBase, _Constant(2), OpSize::i16Bit, MEM_OFFSET_SXTX, 1); }

  {
    // Abridged FTW
    _StoreMem(GPRClass, OpSize::i8Bit, LoadContext(AbridgedFTWIndex), MemBase, _Constant(4), OpSize::i8Bit, MEM_OFFSET_SXTX, 1);
  }

  // BYTE | 0 1 | 2 3 | 4   | 5     | 6 7 | 8 9 | a b | c d | e f |
  // ------------------------------------------
  //   32 | ST0/MM0                             | <R>
  //   48 | ST1/MM1                             | <R>
  //   64 | ST2/MM2                             | <R>
  //   80 | ST3/MM3                             | <R>
  //   96 | ST4/MM4                             | <R>
  //  112 | ST5/MM5                             | <R>
  //  128 | ST6/MM6                             | <R>
  //  144 | ST7/MM7                             | <R>
  //  160 | XMM0
  //  173 | XMM1
  //  192 | XMM2
  //  208 | XMM3
  //  224 | XMM4
  //  240 | XMM5
  //  256 | XMM6
  //  272 | XMM7
  //  288 | 64BitMode ? <R> : XMM8
  //  304 | 64BitMode ? <R> : XMM9
  //  320 | 64BitMode ? <R> : XMM10
  //  336 | 64BitMode ? <R> : XMM11
  //  352 | 64BitMode ? <R> : XMM12
  //  368 | 64BitMode ? <R> : XMM13
  //  384 | 64BitMode ? <R> : XMM14
  //  400 | 64BitMode ? <R> : XMM15
  //  416 | <R>
  //  432 | <R>
  //  448 | <R>
  //  464 | Available
  //  480 | Available
  //  496 | Available
  // FCW: x87 FPU control word
  // FSW: x87 FPU status word
  // FTW: x87 FPU Tag word (Abridged)
  // FOP: x87 FPU opcode. Lower 11 bits of the opcode
  // FIP: x87 FPU instructyion pointer offset
  // FCS: x87 FPU instruction pointer selector. If CPUID_0000_0007_0000_00000:EBX[bit 13] = 1 then this is deprecated and stores as 0
  // FDP: x87 FPU instruction operand (data) pointer offset
  // FDS: x87 FPU instruction operand (data) pointer selector. Same deprecation as FCS
  // MXCSR: If OSFXSR bit in CR4 is not set then this may not be saved
  // MXCSR_MASK: Mask for writes to the MXCSR register
  // If OSFXSR bit in CR4 is not set than FXSAVE /may/ not save the XMM registers
  // This is implementation dependent
  for (uint32_t i = 0; i < Core::CPUState::NUM_MMS; i += 2) {
    RefPair MMRegs = LoadContextPair(OpSize::i128Bit, MM0Index + i);
    _StoreMemPair(FPRClass, OpSize::i128Bit, MMRegs.Low, MMRegs.High, MemBase, i * 16 + 32);
  }
}

void OpDispatchBuilder::SaveSSEState(Ref MemBase) {
  const auto NumRegs = CTX->Config.Is64BitMode ? 16U : 8U;

  for (uint32_t i = 0; i < NumRegs; i += 2) {
    _StoreMemPair(FPRClass, OpSize::i128Bit, LoadXMMRegister(i), LoadXMMRegister(i + 1), MemBase, i * 16 + 160);
  }
}

void OpDispatchBuilder::SaveMXCSRState(Ref MemBase) {
  // Store MXCSR and the mask for all bits.
  _StoreMemPair(GPRClass, OpSize::i32Bit, GetMXCSR(), _Constant(0xFFFF), MemBase, 24);
}

void OpDispatchBuilder::SaveAVXState(Ref MemBase) {
  const auto NumRegs = CTX->Config.Is64BitMode ? 16U : 8U;

  for (uint32_t i = 0; i < NumRegs; i += 2) {
    Ref Upper0 = _VDupElement(OpSize::i256Bit, OpSize::i128Bit, LoadXMMRegister(i + 0), 1);
    Ref Upper1 = _VDupElement(OpSize::i256Bit, OpSize::i128Bit, LoadXMMRegister(i + 1), 1);

    _StoreMemPair(FPRClass, OpSize::i128Bit, Upper0, Upper1, MemBase, i * 16 + 576);
  }
}

Ref OpDispatchBuilder::GetMXCSR() {
  Ref MXCSR = _LoadContext(OpSize::i32Bit, GPRClass, offsetof(FEXCore::Core::CPUState, mxcsr));
  // Mask out unsupported bits
  // Keeps FZ, RC, exception masks, and DAZ
  MXCSR = _And(OpSize::i32Bit, MXCSR, _Constant(0xFFC0));
  return MXCSR;
}

void OpDispatchBuilder::FXRStoreOp(OpcodeArgs) {
  Ref Mem = MakeSegmentAddress(Op, Op->Src[0]);

  RestoreX87State(Mem);
  RestoreSSEState(Mem);

  Ref MXCSR = _LoadMem(GPRClass, OpSize::i32Bit, Mem, _Constant(24), OpSize::i32Bit, MEM_OFFSET_SXTX, 1);
  RestoreMXCSRState(MXCSR);
}

void OpDispatchBuilder::XRstorOpImpl(OpcodeArgs) {
  const auto OpSize = IR::SizeToOpSize(CTX->GetGPRSize());

  // If a bit in our XSTATE_BV is set, then we restore from that region of the XSAVE area,
  // otherwise, if not set, then we need to set the relevant data the bit corresponds to
  // to it's defined initial configuration.
  const auto RestoreIfFlagSetOrDefault = [this, Op, OpSize](uint32_t BitIndex, auto restore_fn, auto default_fn, uint32_t FieldSize = 1) {
    // Set up base address for the XSAVE region to restore from, and also read
    // the XSTATE_BV bit flags out of the XSTATE header.
    //
    // Note: we rematerialize Base/Mask in each block to avoid crossblock
    // liveness.
    Ref Base = XSaveBase(Op);
    Ref Mask = _LoadMem(GPRClass, OpSize::i64Bit, Base, _Constant(512), OpSize::i64Bit, MEM_OFFSET_SXTX, 1);

    Ref BitFlag = _Bfe(OpSize, FieldSize, BitIndex, Mask);
    auto CondJump_ = CondJump(BitFlag, {COND_NEQ});

    auto RestoreBlock = CreateNewCodeBlockAfter(GetCurrentBlock());
    SetTrueJumpTarget(CondJump_, RestoreBlock);
    SetCurrentCodeBlock(RestoreBlock);
    StartNewBlock();
    { restore_fn(); }
    auto RestoreExitJump = Jump();
    auto DefaultBlock = CreateNewCodeBlockAfter(RestoreBlock);
    auto ExitBlock = CreateNewCodeBlockAfter(DefaultBlock);
    SetJumpTarget(RestoreExitJump, ExitBlock);
    SetFalseJumpTarget(CondJump_, DefaultBlock);
    SetCurrentCodeBlock(DefaultBlock);
    StartNewBlock();
    { default_fn(); }
    auto DefaultExitJump = Jump();
    SetJumpTarget(DefaultExitJump, ExitBlock);
    SetCurrentCodeBlock(ExitBlock);
    StartNewBlock();
  };

  // x87
  {
    RestoreIfFlagSetOrDefault(
      0, [this, Op] { RestoreX87State(XSaveBase(Op)); }, [this, Op] { DefaultX87State(Op); });
  }
  // SSE
  {
    RestoreIfFlagSetOrDefault(
      1, [this, Op] { RestoreSSEState(XSaveBase(Op)); }, [this] { DefaultSSEState(); });
  }
  // AVX
  if (CTX->HostFeatures.SupportsAVX) {
    RestoreIfFlagSetOrDefault(
      2, [this, Op] { std::invoke(RestoreAVXStateFunc, this, XSaveBase(Op)); }, [this] { std::invoke(DefaultAVXStateFunc, this); });
  }

  {
    // We need to restore the MXCSR if either SSE or AVX are requested to be saved
    RestoreIfFlagSetOrDefault(
      1,
      [this, Op] {
      Ref Base = XSaveBase(Op);
      Ref MXCSR = _LoadMem(GPRClass, OpSize::i32Bit, Base, _Constant(24), OpSize::i32Bit, MEM_OFFSET_SXTX, 1);
      RestoreMXCSRState(MXCSR);
      },
      [] { /* Intentionally do nothing*/ }, 2);
  }
}

void OpDispatchBuilder::RestoreX87State(Ref MemBase) {
  auto NewFCW = _LoadMem(GPRClass, OpSize::i16Bit, MemBase, OpSize::i16Bit);
  _StoreContext(OpSize::i16Bit, GPRClass, NewFCW, offsetof(FEXCore::Core::CPUState, FCW));

  {
    auto NewFSW = _LoadMem(GPRClass, OpSize::i16Bit, MemBase, _Constant(2), OpSize::i16Bit, MEM_OFFSET_SXTX, 1);
    ReconstructX87StateFromFSW_Helper(NewFSW);
  }

  {
    // Abridged FTW
    StoreContext(AbridgedFTWIndex, _LoadMem(GPRClass, OpSize::i8Bit, MemBase, _Constant(4), OpSize::i8Bit, MEM_OFFSET_SXTX, 1));
  }

  for (uint32_t i = 0; i < Core::CPUState::NUM_MMS; i += 2) {
    auto MMRegs = LoadMemPair(FPRClass, OpSize::i128Bit, MemBase, i * 16 + 32);

    StoreContext(MM0Index + i, MMRegs.Low);
    StoreContext(MM0Index + i + 1, MMRegs.High);
  }
}

void OpDispatchBuilder::RestoreSSEState(Ref MemBase) {
  const auto NumRegs = CTX->Config.Is64BitMode ? 16U : 8U;

  for (uint32_t i = 0; i < NumRegs; i += 2) {
    auto XMMRegs = LoadMemPair(FPRClass, OpSize::i128Bit, MemBase, i * 16 + 160);

    StoreXMMRegister(i, XMMRegs.Low);
    StoreXMMRegister(i + 1, XMMRegs.High);
  }
}

void OpDispatchBuilder::RestoreMXCSRState(Ref MXCSR) {
  // Mask out unsupported bits
  MXCSR = _And(OpSize::i32Bit, MXCSR, _Constant(0xFFC0));

  _StoreContext(OpSize::i32Bit, GPRClass, MXCSR, offsetof(FEXCore::Core::CPUState, mxcsr));
  // We only support the rounding mode and FTZ bit being set
  Ref RoundingMode = _Bfe(OpSize::i32Bit, 3, 13, MXCSR);
  _SetRoundingMode(RoundingMode, true, MXCSR);
}

void OpDispatchBuilder::RestoreAVXState(Ref MemBase) {
  const auto NumRegs = CTX->Config.Is64BitMode ? 16U : 8U;

  for (uint32_t i = 0; i < NumRegs; i += 2) {
    Ref XMMReg0 = LoadXMMRegister(i + 0);
    Ref XMMReg1 = LoadXMMRegister(i + 1);
    auto YMMHRegs = LoadMemPair(FPRClass, OpSize::i128Bit, MemBase, i * 16 + 576);
    StoreXMMRegister(i + 0, _VInsElement(OpSize::i256Bit, OpSize::i128Bit, 1, 0, XMMReg0, YMMHRegs.Low));
    StoreXMMRegister(i + 1, _VInsElement(OpSize::i256Bit, OpSize::i128Bit, 1, 0, XMMReg1, YMMHRegs.High));
  }
}

void OpDispatchBuilder::DefaultX87State(OpcodeArgs) {
  // We can piggy-back on FNINIT's implementation, since
  // it performs the same behavior as required by XRSTOR for resetting flags
  FNINIT(Op);

  // On top of resetting the flags to a default state, we also need to clear
  // all of the ST0-7/MM0-7 registers to zero.
  Ref ZeroVector = LoadZeroVector(Core::CPUState::MM_REG_SIZE);
  for (uint32_t i = 0; i < Core::CPUState::NUM_MMS; ++i) {
    StoreContext(MM0Index + i, ZeroVector);
  }
}

void OpDispatchBuilder::DefaultSSEState() {
  const auto NumRegs = CTX->Config.Is64BitMode ? 16U : 8U;

  Ref ZeroVector = LoadZeroVector(Core::CPUState::XMM_SSE_REG_SIZE);
  for (uint32_t i = 0; i < NumRegs; ++i) {
    StoreXMMRegister(i, ZeroVector);
  }
}

void OpDispatchBuilder::DefaultAVXState() {
  const auto NumRegs = CTX->Config.Is64BitMode ? 16U : 8U;

  for (uint32_t i = 0; i < NumRegs; i++) {
    Ref Reg = LoadXMMRegister(i);
    Ref Dst = _VMov(OpSize::i128Bit, Reg);
    StoreXMMRegister(i, Dst);
  }
}

Ref OpDispatchBuilder::PALIGNROpImpl(OpcodeArgs, const X86Tables::DecodedOperand& Src1, const X86Tables::DecodedOperand& Src2,
                                     const X86Tables::DecodedOperand& Imm, bool IsAVX) {
  // For the 256-bit case we handle it as pairs of 128-bit halves.
  const auto DstSize = GetDstSize(Op);
  const auto SanitizedDstSize = std::min<uint8_t>(DstSize, OpSize::i128Bit);

  const auto Is256Bit = DstSize == Core::CPUState::XMM_AVX_REG_SIZE;
  const auto Index = Imm.Literal();

  Ref Src2Node = LoadSource(FPRClass, Op, Src2, Op->Flags);
  if (Index == 0) {
    if (IsAVX && !Is256Bit) {
      // 128-bit AVX needs to zero the upper bits.
      return _VMov(OpSize::i128Bit, Src2Node);
    } else {
      return Src2Node;
    }
  }
  Ref Src1Node = LoadSource(FPRClass, Op, Src1, Op->Flags);

  if (Index >= (SanitizedDstSize * 2)) {
    // If the immediate is greater than both vectors combined then it zeroes the vector
    return LoadZeroVector(DstSize);
  }

  Ref Low = _VExtr(SanitizedDstSize, 1, Src1Node, Src2Node, Index);
  if (!Is256Bit) {
    return Low;
  }

  Ref HighSrc1 = _VInsElement(DstSize, OpSize::i128Bit, 0, 1, Src1Node, Src1Node);
  Ref HighSrc2 = _VInsElement(DstSize, OpSize::i128Bit, 0, 1, Src2Node, Src2Node);
  Ref High = _VExtr(SanitizedDstSize, 1, HighSrc1, HighSrc2, Index);
  return _VInsElement(DstSize, OpSize::i128Bit, 1, 0, Low, High);
}

void OpDispatchBuilder::PAlignrOp(OpcodeArgs) {
  Ref Result = PALIGNROpImpl(Op, Op->Dest, Op->Src[0], Op->Src[1], false);
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::VPALIGNROp(OpcodeArgs) {
  Ref Result = PALIGNROpImpl(Op, Op->Src[0], Op->Src[1], Op->Src[2], true);
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

template<IR::OpSize ElementSize>
void OpDispatchBuilder::UCOMISxOp(OpcodeArgs) {
  const auto SrcSize = Op->Src[0].IsGPR() ? GetGuestVectorLength() : OpSizeFromSrc(Op);
  Ref Src1 = LoadSource_WithOpSize(FPRClass, Op, Op->Dest, GetGuestVectorLength(), Op->Flags);
  Ref Src2 = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], SrcSize, Op->Flags);

  Comiss(ElementSize, Src1, Src2);
}

template void OpDispatchBuilder::UCOMISxOp<OpSize::i32Bit>(OpcodeArgs);
template void OpDispatchBuilder::UCOMISxOp<OpSize::i64Bit>(OpcodeArgs);

void OpDispatchBuilder::LDMXCSR(OpcodeArgs) {
  Ref Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags);
  RestoreMXCSRState(Dest);
}

void OpDispatchBuilder::STMXCSR(OpcodeArgs) {
  StoreResult(GPRClass, Op, GetMXCSR(), OpSize::iInvalid);
}

template<size_t ElementSize>
void OpDispatchBuilder::PACKUSOp(OpcodeArgs) {
  Ref Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Result = _VSQXTUNPair(GetSrcSize(Op), ElementSize, Dest, Src);

  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

template void OpDispatchBuilder::PACKUSOp<OpSize::i16Bit>(OpcodeArgs);
template void OpDispatchBuilder::PACKUSOp<OpSize::i32Bit>(OpcodeArgs);

void OpDispatchBuilder::VPACKUSOp(OpcodeArgs, size_t ElementSize) {
  const auto DstSize = GetDstSize(Op);
  const auto Is256Bit = DstSize == Core::CPUState::XMM_AVX_REG_SIZE;

  Ref Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);
  Ref Result = _VSQXTUNPair(GetSrcSize(Op), ElementSize, Src1, Src2);

  if (Is256Bit) {
    // We do a little cheeky 64-bit swapping to interleave the result.
    Ref Swapped = _VInsElement(DstSize, OpSize::i64Bit, 2, 1, Result, Result);
    Result = _VInsElement(DstSize, OpSize::i64Bit, 1, 2, Swapped, Result);
  }
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

template<size_t ElementSize>
void OpDispatchBuilder::PACKSSOp(OpcodeArgs) {
  Ref Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Result = _VSQXTNPair(GetSrcSize(Op), ElementSize, Dest, Src);

  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

template void OpDispatchBuilder::PACKSSOp<OpSize::i16Bit>(OpcodeArgs);
template void OpDispatchBuilder::PACKSSOp<OpSize::i32Bit>(OpcodeArgs);

void OpDispatchBuilder::VPACKSSOp(OpcodeArgs, size_t ElementSize) {
  const auto DstSize = GetDstSize(Op);
  const auto Is256Bit = DstSize == Core::CPUState::XMM_AVX_REG_SIZE;

  Ref Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);
  Ref Result = _VSQXTNPair(GetSrcSize(Op), ElementSize, Src1, Src2);

  if (Is256Bit) {
    // We do a little cheeky 64-bit swapping to interleave the result.
    Ref Swapped = _VInsElement(DstSize, OpSize::i64Bit, 2, 1, Result, Result);
    Result = _VInsElement(DstSize, OpSize::i64Bit, 1, 2, Swapped, Result);
  }
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

Ref OpDispatchBuilder::PMULLOpImpl(OpSize Size, size_t ElementSize, bool Signed, Ref Src1, Ref Src2) {
  if (Size == OpSize::i64Bit) {
    if (Signed) {
      return _VSMull(OpSize::i128Bit, ElementSize, Src1, Src2);
    } else {
      return _VUMull(OpSize::i128Bit, ElementSize, Src1, Src2);
    }
  } else {
    auto InsSrc1 = _VUnZip(Size, ElementSize, Src1, Src1);
    auto InsSrc2 = _VUnZip(Size, ElementSize, Src2, Src2);

    if (Signed) {
      return _VSMull(Size, ElementSize, InsSrc1, InsSrc2);
    } else {
      return _VUMull(Size, ElementSize, InsSrc1, InsSrc2);
    }
  }
}

template<size_t ElementSize, bool Signed>
void OpDispatchBuilder::PMULLOp(OpcodeArgs) {
  static_assert(ElementSize == sizeof(uint32_t), "Currently only handles 32-bit -> 64-bit");

  Ref Src1 = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  Ref Src2 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Res = PMULLOpImpl(OpSizeFromSrc(Op), ElementSize, Signed, Src1, Src2);

  StoreResult(FPRClass, Op, Res, OpSize::iInvalid);
}

template void OpDispatchBuilder::PMULLOp<OpSize::i32Bit, false>(OpcodeArgs);
template void OpDispatchBuilder::PMULLOp<OpSize::i32Bit, true>(OpcodeArgs);

template<size_t ElementSize, bool Signed>
void OpDispatchBuilder::VPMULLOp(OpcodeArgs) {
  static_assert(ElementSize == sizeof(uint32_t), "Currently only handles 32-bit -> 64-bit");

  Ref Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);
  Ref Result = PMULLOpImpl(OpSizeFromSrc(Op), ElementSize, Signed, Src1, Src2);

  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

template void OpDispatchBuilder::VPMULLOp<OpSize::i32Bit, false>(OpcodeArgs);
template void OpDispatchBuilder::VPMULLOp<OpSize::i32Bit, true>(OpcodeArgs);

template<bool ToXMM>
void OpDispatchBuilder::MOVQ2DQ(OpcodeArgs) {
  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

  // This instruction is a bit special in that if the source is MMX then it zexts to 128bit
  if constexpr (ToXMM) {
    const auto Index = Op->Dest.Data.GPR.GPR - FEXCore::X86State::REG_XMM_0;

    Src = _VMov(OpSize::i128Bit, Src);
    StoreXMMRegister(Index, Src);
  } else {
    // This is simple, just store the result
    StoreResult(FPRClass, Op, Src, OpSize::iInvalid);
  }
}

template void OpDispatchBuilder::MOVQ2DQ<false>(OpcodeArgs);
template void OpDispatchBuilder::MOVQ2DQ<true>(OpcodeArgs);

Ref OpDispatchBuilder::ADDSUBPOpImpl(OpSize Size, size_t ElementSize, Ref Src1, Ref Src2) {
  if (CTX->HostFeatures.SupportsFCMA) {
    if (ElementSize == OpSize::i32Bit) {
      auto Swizzle = _VRev64(Size, 4, Src2);
      return _VFCADD(Size, ElementSize, Src1, Swizzle, 90);
    } else {
      auto Swizzle = _VExtr(Size, 1, Src2, Src2, 8);
      return _VFCADD(Size, ElementSize, Src1, Swizzle, 90);
    }
  } else {
    auto ConstantEOR =
      LoadAndCacheNamedVectorConstant(Size, ElementSize == OpSize::i32Bit ? NAMED_VECTOR_PADDSUBPS_INVERT : NAMED_VECTOR_PADDSUBPD_INVERT);
    auto InvertedSource = _VXor(Size, ElementSize, Src2, ConstantEOR);
    return _VFAdd(Size, ElementSize, Src1, InvertedSource);
  }
}

template<size_t ElementSize>
void OpDispatchBuilder::ADDSUBPOp(OpcodeArgs) {
  Ref Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Result = ADDSUBPOpImpl(OpSizeFromSrc(Op), ElementSize, Dest, Src);

  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

template void OpDispatchBuilder::ADDSUBPOp<OpSize::i32Bit>(OpcodeArgs);
template void OpDispatchBuilder::ADDSUBPOp<OpSize::i64Bit>(OpcodeArgs);

template<size_t ElementSize>
void OpDispatchBuilder::VADDSUBPOp(OpcodeArgs) {
  Ref Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);
  Ref Result = ADDSUBPOpImpl(OpSizeFromSrc(Op), ElementSize, Src1, Src2);

  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

template void OpDispatchBuilder::VADDSUBPOp<OpSize::i32Bit>(OpcodeArgs);
template void OpDispatchBuilder::VADDSUBPOp<OpSize::i64Bit>(OpcodeArgs);

void OpDispatchBuilder::PFNACCOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);

  Ref Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

  auto DestUnzip = _VUnZip(Size, OpSize::i32Bit, Dest, Src);
  auto SrcUnzip = _VUnZip2(Size, OpSize::i32Bit, Dest, Src);
  auto Result = _VFSub(Size, OpSize::i32Bit, DestUnzip, SrcUnzip);

  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::PFPNACCOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);

  Ref Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

  Ref ResAdd {};
  Ref ResSub {};
  auto UpperSubDest = _VDupElement(Size, OpSize::i32Bit, Dest, 1);

  ResSub = _VFSub(OpSize::i32Bit, OpSize::i32Bit, Dest, UpperSubDest);
  ResAdd = _VFAddP(Size, OpSize::i32Bit, Src, Src);

  auto Result = _VInsElement(OpSize::i64Bit, OpSize::i32Bit, 1, 0, ResSub, ResAdd);

  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::PSWAPDOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);
  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

  auto Result = _VRev64(Size, OpSize::i32Bit, Src);
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::PI2FWOp(OpcodeArgs) {
  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

  const auto Size = OpSizeFromDst(Op);

  // We now need to transpose the lower 16-bits of each element together
  // Only needing to move the upper element down in this case
  Src = _VUnZip(Size, OpSize::i16Bit, Src, Src);

  // Now we need to sign extend the 16bit value to 32-bit
  Src = _VSXTL(Size, OpSize::i16Bit, Src);

  // int32_t to float
  Src = _Vector_SToF(Size, OpSize::i32Bit, Src);

  StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Src, Size, OpSize::iInvalid);
}

void OpDispatchBuilder::PF2IWOp(OpcodeArgs) {
  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

  const auto Size = OpSizeFromDst(Op);

  // Float to int32_t
  Src = _Vector_FToZS(Size, OpSize::i32Bit, Src);

  // We now need to transpose the lower 16-bits of each element together
  // Only needing to move the upper element down in this case
  Src = _VUnZip(Size, OpSize::i16Bit, Src, Src);

  // Now we need to sign extend the 16bit value to 32-bit
  Src = _VSXTL(Size, OpSize::i16Bit, Src);
  StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Src, Size, OpSize::iInvalid);
}

void OpDispatchBuilder::PMULHRWOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);

  Ref Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

  Ref Res {};

  // Implementation is more efficient for 8byte registers
  // Multiplies 4 16bit values in to 4 32bit values
  Res = _VSMull(Size * 2, OpSize::i16Bit, Dest, Src);

  // Load 0x0000_8000 in to each 32-bit element.
  Ref VConstant = _VectorImm(OpSize::i128Bit, OpSize::i32Bit, 0x80, 8);

  Res = _VAdd(Size * 2, OpSize::i32Bit, Res, VConstant);

  // Now shift and narrow to convert 32-bit values to 16bit, storing the top 16bits
  Res = _VUShrNI(Size * 2, OpSize::i32Bit, Res, 16);

  StoreResult(FPRClass, Op, Res, OpSize::iInvalid);
}

template<uint8_t CompType>
void OpDispatchBuilder::VPFCMPOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);
  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Dest = LoadSource_WithOpSize(FPRClass, Op, Op->Dest, OpSizeFromDst(Op), Op->Flags);

  Ref Result {};
  // This maps 1:1 to an AArch64 NEON Op
  // auto ALUOp = _VCMPGT(Size, 4, Dest, Src);
  switch (CompType) {
  case 0x00: // EQ
    Result = _VFCMPEQ(Size, OpSize::i32Bit, Dest, Src);
    break;
  case 0x01: // GE(Swapped operand)
    Result = _VFCMPLE(Size, OpSize::i32Bit, Src, Dest);
    break;
  case 0x02: // GT
    Result = _VFCMPGT(Size, OpSize::i32Bit, Dest, Src);
    break;
  default: LOGMAN_MSG_A_FMT("Unknown Comparison type: {}", CompType); break;
  }

  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

template void OpDispatchBuilder::VPFCMPOp<0>(OpcodeArgs);
template void OpDispatchBuilder::VPFCMPOp<1>(OpcodeArgs);
template void OpDispatchBuilder::VPFCMPOp<2>(OpcodeArgs);

Ref OpDispatchBuilder::PMADDWDOpImpl(size_t Size, Ref Src1, Ref Src2) {
  // This is a pretty curious operation
  // Does two MADD operations across 4 16bit signed integers and accumulates to 32bit integers in the destination
  //
  // x86 PMADDWD: xmm1, xmm2
  //              xmm1[31:0]  = (xmm1[15:0] * xmm2[15:0]) + (xmm1[31:16] * xmm2[31:16])
  //              xmm1[63:32] = (xmm1[47:32] * xmm2[47:32]) + (xmm1[63:48] * xmm2[63:48])
  //              etc.. for larger registers

  if (Size == OpSize::i64Bit) {
    // MMX implementation can be slightly more optimal
    Size <<= 1;
    auto MullResult = _VSMull(Size, OpSize::i16Bit, Src1, Src2);
    return _VAddP(Size, OpSize::i32Bit, MullResult, MullResult);
  }

  auto Lower = _VSMull(Size, OpSize::i16Bit, Src1, Src2);
  auto Upper = _VSMull2(Size, OpSize::i16Bit, Src1, Src2);

  // [15:0 ] + [31:16], [32:47 ] + [63:48  ], [79:64] + [95:80], [111:96] + [127:112]
  return _VAddP(Size, OpSize::i32Bit, Lower, Upper);
}

void OpDispatchBuilder::PMADDWD(OpcodeArgs) {
  const auto Size = GetSrcSize(Op);

  Ref Src1 = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  Ref Src2 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

  Ref Result = PMADDWDOpImpl(Size, Src1, Src2);
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::VPMADDWDOp(OpcodeArgs) {
  const auto Size = GetSrcSize(Op);

  Ref Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);

  Ref Result = PMADDWDOpImpl(Size, Src1, Src2);
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

Ref OpDispatchBuilder::PMADDUBSWOpImpl(size_t Size, Ref Src1, Ref Src2) {
  if (Size == OpSize::i64Bit) {
    // 64bit is more efficient

    // Src1 is unsigned
    auto Src1_16b = _VUXTL(Size * 2, OpSize::i8Bit, Src1); // [7:0 ], [15:8], [23:16], [31:24], [39:32], [47:40], [55:48], [63:56]

    // Src2 is signed
    auto Src2_16b = _VSXTL(Size * 2, OpSize::i8Bit, Src2); // [7:0 ], [15:8], [23:16], [31:24], [39:32], [47:40], [55:48], [63:56]

    auto ResMul_L = _VSMull(Size * 2, OpSize::i16Bit, Src1_16b, Src2_16b);
    auto ResMul_H = _VSMull2(Size * 2, OpSize::i16Bit, Src1_16b, Src2_16b);

    // Now add pairwise across the vector
    auto ResAdd = _VAddP(Size * 2, OpSize::i32Bit, ResMul_L, ResMul_H);

    // Add saturate back down to 16bit
    return _VSQXTN(Size * 2, OpSize::i32Bit, ResAdd);
  }

  // V{U,S}XTL{,2}/ and VUnZip{,2} can be optimized in this solution to save about one instruction.
  // We can up-front zero extend and sign extend the elements in-place.
  // This means extracting even and odd elements up-front so the unzips aren't required.
  // Requires implementing IR ops for BIC (vector, immediate) although.

  // Src1 is unsigned
  auto Src1_16b_L = _VUXTL(Size, OpSize::i8Bit, Src1); // [7:0 ], [15:8], [23:16], [31:24], [39:32], [47:40], [55:48], [63:56]
  auto Src2_16b_L = _VSXTL(Size, OpSize::i8Bit, Src2); // [7:0 ], [15:8], [23:16], [31:24], [39:32], [47:40], [55:48], [63:56]
  auto ResMul_L = _VMul(Size, OpSize::i16Bit, Src1_16b_L, Src2_16b_L);

  // Src2 is signed
  auto Src1_16b_H = _VUXTL2(Size, OpSize::i8Bit, Src1); // Offset to +64bits [7:0 ], [15:8], [23:16], [31:24], [39:32], [47:40], [55:48], [63:56]
  auto Src2_16b_H = _VSXTL2(Size, OpSize::i8Bit, Src2); // Offset to +64bits [7:0 ], [15:8], [23:16], [31:24], [39:32], [47:40], [55:48], [63:56]
  auto ResMul_L_H = _VMul(Size, OpSize::i16Bit, Src1_16b_H, Src2_16b_H);

  auto TmpZip1 = _VUnZip(Size, OpSize::i16Bit, ResMul_L, ResMul_L_H);
  auto TmpZip2 = _VUnZip2(Size, OpSize::i16Bit, ResMul_L, ResMul_L_H);

  return _VSQAdd(Size, OpSize::i16Bit, TmpZip1, TmpZip2);
}

void OpDispatchBuilder::PMADDUBSW(OpcodeArgs) {
  const auto Size = GetSrcSize(Op);

  Ref Src1 = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  Ref Src2 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

  Ref Result = PMADDUBSWOpImpl(Size, Src1, Src2);
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::VPMADDUBSWOp(OpcodeArgs) {
  const auto Size = GetSrcSize(Op);

  Ref Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);

  Ref Result = PMADDUBSWOpImpl(Size, Src1, Src2);
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

Ref OpDispatchBuilder::PMULHWOpImpl(OpcodeArgs, bool Signed, Ref Src1, Ref Src2) {
  const auto Size = GetSrcSize(Op);
  if (Signed) {
    return _VSMulH(Size, OpSize::i16Bit, Src1, Src2);
  } else {
    return _VUMulH(Size, OpSize::i16Bit, Src1, Src2);
  }
}

template<bool Signed>
void OpDispatchBuilder::PMULHW(OpcodeArgs) {
  Ref Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Result = PMULHWOpImpl(Op, Signed, Dest, Src);

  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

template void OpDispatchBuilder::PMULHW<false>(OpcodeArgs);
template void OpDispatchBuilder::PMULHW<true>(OpcodeArgs);

template<bool Signed>
void OpDispatchBuilder::VPMULHWOp(OpcodeArgs) {
  const auto DstSize = GetDstSize(Op);
  const auto Is128Bit = DstSize == Core::CPUState::XMM_SSE_REG_SIZE;

  Ref Dest = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Src = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);
  Ref Result = PMULHWOpImpl(Op, Signed, Dest, Src);

  if (Is128Bit) {
    Result = _VMov(OpSize::i128Bit, Result);
  }
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

template void OpDispatchBuilder::VPMULHWOp<false>(OpcodeArgs);
template void OpDispatchBuilder::VPMULHWOp<true>(OpcodeArgs);

Ref OpDispatchBuilder::PMULHRSWOpImpl(OpSize Size, Ref Src1, Ref Src2) {
  Ref Res {};
  if (Size == OpSize::i64Bit) {
    // Implementation is more efficient for 8byte registers
    Res = _VSMull(Size * 2, OpSize::i16Bit, Src1, Src2);
    Res = _VSShrI(Size * 2, OpSize::i32Bit, Res, 14);
    auto OneVector = _VectorImm(Size * 2, OpSize::i32Bit, 1);
    Res = _VAdd(Size * 2, OpSize::i32Bit, Res, OneVector);
    return _VUShrNI(Size * 2, OpSize::i32Bit, Res, 1);
  } else {
    // 128-bit and 256-bit are less efficient
    Ref ResultLow;
    Ref ResultHigh;

    ResultLow = _VSMull(Size, OpSize::i16Bit, Src1, Src2);
    ResultHigh = _VSMull2(Size, OpSize::i16Bit, Src1, Src2);

    ResultLow = _VSShrI(Size, OpSize::i32Bit, ResultLow, 14);
    ResultHigh = _VSShrI(Size, OpSize::i32Bit, ResultHigh, 14);
    auto OneVector = _VectorImm(Size, OpSize::i32Bit, 1);

    ResultLow = _VAdd(Size, OpSize::i32Bit, ResultLow, OneVector);
    ResultHigh = _VAdd(Size, OpSize::i32Bit, ResultHigh, OneVector);

    // Combine the results
    Res = _VUShrNI(Size, OpSize::i32Bit, ResultLow, 1);
    return _VUShrNI2(Size, OpSize::i32Bit, Res, ResultHigh, 1);
  }
}

void OpDispatchBuilder::PMULHRSW(OpcodeArgs) {
  Ref Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Result = PMULHRSWOpImpl(OpSizeFromSrc(Op), Dest, Src);

  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::VPMULHRSWOp(OpcodeArgs) {
  Ref Dest = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Src = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);
  Ref Result = PMULHRSWOpImpl(OpSizeFromSrc(Op), Dest, Src);

  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

Ref OpDispatchBuilder::HSUBPOpImpl(OpSize SrcSize, size_t ElementSize, Ref Src1, Ref Src2) {
  auto Even = _VUnZip(SrcSize, ElementSize, Src1, Src2);
  auto Odd = _VUnZip2(SrcSize, ElementSize, Src1, Src2);
  return _VFSub(SrcSize, ElementSize, Even, Odd);
}

template<size_t ElementSize>
void OpDispatchBuilder::HSUBP(OpcodeArgs) {
  Ref Src1 = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  Ref Src2 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Result = HSUBPOpImpl(OpSizeFromSrc(Op), ElementSize, Src1, Src2);
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

template void OpDispatchBuilder::HSUBP<OpSize::i32Bit>(OpcodeArgs);
template void OpDispatchBuilder::HSUBP<OpSize::i64Bit>(OpcodeArgs);

void OpDispatchBuilder::VHSUBPOp(OpcodeArgs, size_t ElementSize) {
  const auto DstSize = GetDstSize(Op);
  const auto Is256Bit = DstSize == Core::CPUState::XMM_AVX_REG_SIZE;

  Ref Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);

  Ref Result = HSUBPOpImpl(OpSizeFromSrc(Op), ElementSize, Src1, Src2);
  Ref Dest = Result;
  if (Is256Bit) {
    Dest = _VInsElement(DstSize, OpSize::i64Bit, 1, 2, Result, Result);
    Dest = _VInsElement(DstSize, OpSize::i64Bit, 2, 1, Dest, Result);
  }

  StoreResult(FPRClass, Op, Dest, OpSize::iInvalid);
}

Ref OpDispatchBuilder::PHSUBOpImpl(OpSize Size, Ref Src1, Ref Src2, size_t ElementSize) {
  auto Even = _VUnZip(Size, ElementSize, Src1, Src2);
  auto Odd = _VUnZip2(Size, ElementSize, Src1, Src2);
  return _VSub(Size, ElementSize, Even, Odd);
}

template<size_t ElementSize>
void OpDispatchBuilder::PHSUB(OpcodeArgs) {
  Ref Src1 = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  Ref Src2 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Result = PHSUBOpImpl(OpSizeFromSrc(Op), Src1, Src2, ElementSize);
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

template void OpDispatchBuilder::PHSUB<2>(OpcodeArgs);
template void OpDispatchBuilder::PHSUB<4>(OpcodeArgs);

void OpDispatchBuilder::VPHSUBOp(OpcodeArgs, size_t ElementSize) {
  const auto DstSize = GetDstSize(Op);
  const auto Is256Bit = DstSize == Core::CPUState::XMM_AVX_REG_SIZE;

  Ref Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);
  Ref Result = PHSUBOpImpl(OpSizeFromSrc(Op), Src1, Src2, ElementSize);
  if (Is256Bit) {
    Ref Inserted = _VInsElement(DstSize, OpSize::i64Bit, 1, 2, Result, Result);
    Result = _VInsElement(DstSize, OpSize::i64Bit, 2, 1, Inserted, Result);
  }
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

Ref OpDispatchBuilder::PHADDSOpImpl(OpSize Size, Ref Src1, Ref Src2) {
  const uint8_t ElementSize = 2;

  auto Even = _VUnZip(Size, ElementSize, Src1, Src2);
  auto Odd = _VUnZip2(Size, ElementSize, Src1, Src2);

  // Saturate back down to the result
  return _VSQAdd(Size, ElementSize, Even, Odd);
}

void OpDispatchBuilder::PHADDS(OpcodeArgs) {
  Ref Src1 = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  Ref Src2 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

  Ref Result = PHADDSOpImpl(OpSizeFromSrc(Op), Src1, Src2);
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::VPHADDSWOp(OpcodeArgs) {
  const auto SrcSize = GetSrcSize(Op);
  const auto Is256Bit = SrcSize == Core::CPUState::XMM_AVX_REG_SIZE;

  Ref Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);

  Ref Result = PHADDSOpImpl(OpSizeFromSrc(Op), Src1, Src2);
  Ref Dest = Result;

  if (Is256Bit) {
    Dest = _VInsElement(SrcSize, OpSize::i64Bit, 1, 2, Result, Result);
    Dest = _VInsElement(SrcSize, OpSize::i64Bit, 2, 1, Dest, Result);
  }

  StoreResult(FPRClass, Op, Dest, OpSize::iInvalid);
}

Ref OpDispatchBuilder::PHSUBSOpImpl(OpSize Size, Ref Src1, Ref Src2) {
  const uint8_t ElementSize = OpSize::i16Bit;

  auto Even = _VUnZip(Size, ElementSize, Src1, Src2);
  auto Odd = _VUnZip2(Size, ElementSize, Src1, Src2);

  // Saturate back down to the result
  return _VSQSub(Size, ElementSize, Even, Odd);
}

void OpDispatchBuilder::PHSUBS(OpcodeArgs) {
  Ref Src1 = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  Ref Src2 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Result = PHSUBSOpImpl(OpSizeFromSrc(Op), Src1, Src2);
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::VPHSUBSWOp(OpcodeArgs) {
  const auto DstSize = GetDstSize(Op);
  const auto Is256Bit = DstSize == Core::CPUState::XMM_AVX_REG_SIZE;

  Ref Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);
  Ref Result = PHSUBSOpImpl(OpSizeFromSrc(Op), Src1, Src2);

  Ref Dest = Result;
  if (Is256Bit) {
    Dest = _VInsElement(DstSize, OpSize::i64Bit, 1, 2, Result, Result);
    Dest = _VInsElement(DstSize, OpSize::i64Bit, 2, 1, Dest, Result);
  }

  StoreResult(FPRClass, Op, Dest, OpSize::iInvalid);
}

Ref OpDispatchBuilder::PSADBWOpImpl(size_t Size, Ref Src1, Ref Src2) {
  // The documentation is actually incorrect in how this instruction operates
  // It strongly implies that the `abs(dest[i] - src[i])` operates in 8bit space
  // but it actually operates in more than 8bit space
  // This can be seen with `abs(0 - 0xFF)` returning a different result depending
  // on bit length
  const auto Is128Bit = Size == Core::CPUState::XMM_SSE_REG_SIZE;

  if (Size == OpSize::i64Bit) {
    auto AbsResult = _VUABDL(Size * 2, OpSize::i8Bit, Src1, Src2);

    // Now vector-wide add the results for each
    return _VAddV(Size * 2, OpSize::i16Bit, AbsResult);
  }

  auto AbsResult_Low = _VUABDL(Size, OpSize::i8Bit, Src1, Src2);
  auto AbsResult_High = _VUABDL2(Size, OpSize::i8Bit, Src1, Src2);

  Ref Result_Low = _VAddV(OpSize::i128Bit, OpSize::i16Bit, AbsResult_Low);
  Ref Result_High = _VAddV(OpSize::i128Bit, OpSize::i16Bit, AbsResult_High);
  auto Low = _VZip(Size, OpSize::i64Bit, Result_Low, Result_High);

  if (Is128Bit) {
    return Low;
  }

  Ref HighSrc1 = _VDupElement(Size, OpSize::i128Bit, AbsResult_Low, 1);
  Ref HighSrc2 = _VDupElement(Size, OpSize::i128Bit, AbsResult_High, 1);

  Ref HighResult_Low = _VAddV(OpSize::i128Bit, OpSize::i16Bit, HighSrc1);
  Ref HighResult_High = _VAddV(OpSize::i128Bit, OpSize::i16Bit, HighSrc2);

  Ref High = _VInsElement(Size, OpSize::i64Bit, 1, 0, HighResult_Low, HighResult_High);
  Ref Full = _VInsElement(Size, OpSize::i128Bit, 1, 0, Low, High);

  Ref Tmp = _VInsElement(Size, OpSize::i64Bit, 2, 1, Full, Full);
  return _VInsElement(Size, OpSize::i64Bit, 1, 2, Tmp, Full);
}

void OpDispatchBuilder::PSADBW(OpcodeArgs) {
  const auto Size = GetSrcSize(Op);

  Ref Src1 = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  Ref Src2 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

  Ref Result = PSADBWOpImpl(Size, Src1, Src2);
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::VPSADBWOp(OpcodeArgs) {
  const auto Size = GetSrcSize(Op);

  Ref Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);

  Ref Result = PSADBWOpImpl(Size, Src1, Src2);
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

Ref OpDispatchBuilder::ExtendVectorElementsImpl(OpcodeArgs, size_t ElementSize, size_t DstElementSize, bool Signed) {
  const auto DstSize = OpSizeFromDst(Op);

  const auto GetSrc = [&] {
    if (Op->Src[0].IsGPR()) {
      return LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], DstSize, Op->Flags);
    } else {
      // For memory operands the 256-bit variant loads twice the size specified in the table.
      const auto Is256Bit = DstSize == Core::CPUState::XMM_AVX_REG_SIZE;
      const auto SrcSize = OpSizeFromSrc(Op);
      const auto LoadSize = Is256Bit ? IR::SizeToOpSize(IR::OpSizeToSize(SrcSize) * 2) : SrcSize;

      return LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], LoadSize, Op->Flags);
    }
  };

  Ref Src = GetSrc();
  Ref Result {Src};

  for (size_t CurrentElementSize = ElementSize; CurrentElementSize != DstElementSize; CurrentElementSize <<= 1) {
    if (Signed) {
      Result = _VSXTL(DstSize, CurrentElementSize, Result);
    } else {
      Result = _VUXTL(DstSize, CurrentElementSize, Result);
    }
  }

  return Result;
}

template<size_t ElementSize, size_t DstElementSize, bool Signed>
void OpDispatchBuilder::ExtendVectorElements(OpcodeArgs) {
  Ref Result = ExtendVectorElementsImpl(Op, ElementSize, DstElementSize, Signed);
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

template void OpDispatchBuilder::ExtendVectorElements<OpSize::i8Bit, OpSize::i16Bit, false>(OpcodeArgs);
template void OpDispatchBuilder::ExtendVectorElements<OpSize::i8Bit, OpSize::i32Bit, false>(OpcodeArgs);
template void OpDispatchBuilder::ExtendVectorElements<OpSize::i8Bit, OpSize::i64Bit, false>(OpcodeArgs);
template void OpDispatchBuilder::ExtendVectorElements<OpSize::i16Bit, OpSize::i32Bit, false>(OpcodeArgs);
template void OpDispatchBuilder::ExtendVectorElements<OpSize::i16Bit, OpSize::i64Bit, false>(OpcodeArgs);
template void OpDispatchBuilder::ExtendVectorElements<OpSize::i32Bit, OpSize::i64Bit, false>(OpcodeArgs);

template void OpDispatchBuilder::ExtendVectorElements<OpSize::i8Bit, OpSize::i16Bit, true>(OpcodeArgs);
template void OpDispatchBuilder::ExtendVectorElements<OpSize::i8Bit, OpSize::i32Bit, true>(OpcodeArgs);
template void OpDispatchBuilder::ExtendVectorElements<OpSize::i8Bit, OpSize::i64Bit, true>(OpcodeArgs);
template void OpDispatchBuilder::ExtendVectorElements<OpSize::i16Bit, OpSize::i32Bit, true>(OpcodeArgs);
template void OpDispatchBuilder::ExtendVectorElements<OpSize::i16Bit, OpSize::i64Bit, true>(OpcodeArgs);
template void OpDispatchBuilder::ExtendVectorElements<OpSize::i32Bit, OpSize::i64Bit, true>(OpcodeArgs);

Ref OpDispatchBuilder::VectorRoundImpl(OpSize Size, size_t ElementSize, Ref Src, uint64_t Mode) {
  return _Vector_FToI(Size, ElementSize, Src, TranslateRoundType(Mode));
}

template<size_t ElementSize>
void OpDispatchBuilder::VectorRound(OpcodeArgs) {
  // No need to zero extend the vector in the event we have a
  // scalar source, especially since it's only inserted into another vector.
  const auto SrcSize = OpSizeFromSrc(Op);
  Ref Src = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], SrcSize, Op->Flags);

  const uint64_t Mode = Op->Src[1].Literal();
  Src = VectorRoundImpl(OpSizeFromDst(Op), ElementSize, Src, Mode);

  StoreResult(FPRClass, Op, Src, OpSize::iInvalid);
}

template void OpDispatchBuilder::VectorRound<OpSize::i32Bit>(OpcodeArgs);
template void OpDispatchBuilder::VectorRound<OpSize::i64Bit>(OpcodeArgs);

template<size_t ElementSize>
void OpDispatchBuilder::AVXVectorRound(OpcodeArgs) {
  const auto Mode = Op->Src[1].Literal();

  // No need to zero extend the vector in the event we have a
  // scalar source, especially since it's only inserted into another vector.
  const auto SrcSize = OpSizeFromSrc(Op);

  Ref Src = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], SrcSize, Op->Flags);
  Ref Result = VectorRoundImpl(OpSizeFromDst(Op), ElementSize, Src, Mode);

  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

template void OpDispatchBuilder::AVXVectorRound<OpSize::i32Bit>(OpcodeArgs);
template void OpDispatchBuilder::AVXVectorRound<OpSize::i64Bit>(OpcodeArgs);

Ref OpDispatchBuilder::VectorBlend(OpSize Size, size_t ElementSize, Ref Src1, Ref Src2, uint8_t Selector) {
  if (ElementSize == OpSize::i32Bit) {
    Selector &= 0b1111;
    switch (Selector) {
    case 0b0000:
      // Dest[31:0]   = Src1[31:0]
      // Dest[63:32]  = Src1[63:32]
      // Dest[95:64]  = Src1[95:64]
      // Dest[127:96] = Src1[127:96]
      // Copy
      return Src1;
    case 0b0001:
      // Dest[31:0]   = Src2[31:0]
      // Dest[63:32]  = Src1[63:32]
      // Dest[95:64]  = Src1[95:64]
      // Dest[127:96] = Src1[127:96]
      return _VInsElement(Size, ElementSize, 0, 0, Src1, Src2);
    case 0b0010:
      // Dest[31:0]   = Src1[31:0]
      // Dest[63:32]  = Src2[63:32]
      // Dest[95:64]  = Src1[95:64]
      // Dest[127:96] = Src1[127:96]
      return _VInsElement(Size, ElementSize, 1, 1, Src1, Src2);
    case 0b0011:
      // Dest[31:0]   = Src2[31:0]
      // Dest[63:32]  = Src2[63:32]
      // Dest[95:64]  = Src1[95:64]
      // Dest[127:96] = Src1[127:96]
      return _VInsElement(Size, OpSize::i64Bit, 0, 0, Src1, Src2);
    case 0b0100:
      // Dest[31:0]   = Src1[31:0]
      // Dest[63:32]  = Src1[63:32]
      // Dest[95:64]  = Src2[95:64]
      // Dest[127:96] = Src1[127:96]
      return _VInsElement(Size, ElementSize, 2, 2, Src1, Src2);
    case 0b0101: {
      // Dest[31:0]   = Src2[31:0]
      // Dest[63:32]  = Src1[63:32]
      // Dest[95:64]  = Src2[95:64]
      // Dest[127:96] = Src1[127:96]
      // Rotate the elements of the incoming source so they end up in the correct location.
      // Then trn2 keeps the destination results in the expected location.
      auto Temp = _VRev64(Size, OpSize::i32Bit, Src2);
      return _VTrn2(Size, ElementSize, Temp, Src1);
    }
    case 0b0110: {
      // Dest[31:0]   = Src1[31:0]
      // Dest[63:32]  = Src2[63:32]
      // Dest[95:64]  = Src2[95:64]
      // Dest[127:96] = Src1[127:96]
      auto ConstantSwizzle = LoadAndCacheNamedVectorConstant(Size, FEXCore::IR::NamedVectorConstant::NAMED_VECTOR_BLENDPS_0110B);
      return _VTBX1(Size, Src1, Src2, ConstantSwizzle);
    }
    case 0b0111: {
      // Dest[31:0]   = Src2[31:0]
      // Dest[63:32]  = Src2[63:32]
      // Dest[95:64]  = Src2[95:64]
      // Dest[127:96] = Src1[127:96]
      auto ConstantSwizzle = LoadAndCacheNamedVectorConstant(Size, FEXCore::IR::NamedVectorConstant::NAMED_VECTOR_BLENDPS_0111B);
      return _VTBX1(Size, Src1, Src2, ConstantSwizzle);
    }
    case 0b1000:
      // Dest[31:0]   = Src1[31:0]
      // Dest[63:32]  = Src1[63:32]
      // Dest[95:64]  = Src1[95:64]
      // Dest[127:96] = Src2[127:96]
      return _VInsElement(Size, ElementSize, 3, 3, Src1, Src2);
    case 0b1001: {
      // Dest[31:0]   = Src2[31:0]
      // Dest[63:32]  = Src1[63:32]
      // Dest[95:64]  = Src1[95:64]
      // Dest[127:96] = Src2[127:96]
      auto ConstantSwizzle = LoadAndCacheNamedVectorConstant(Size, FEXCore::IR::NamedVectorConstant::NAMED_VECTOR_BLENDPS_1001B);
      return _VTBX1(Size, Src1, Src2, ConstantSwizzle);
    }
    case 0b1010: {
      // Dest[31:0]   = Src1[31:0]
      // Dest[63:32]  = Src2[63:32]
      // Dest[95:64]  = Src1[95:64]
      // Dest[127:96] = Src2[127:96]
      // Rotate the elements of the incoming destination so they end up in the correct location.
      // Then trn2 keeps the source results in the expected location.
      auto Temp = _VRev64(Size, OpSize::i32Bit, Src1);
      return _VTrn2(Size, ElementSize, Temp, Src2);
    }
    case 0b1011: {
      // Dest[31:0]   = Src2[31:0]
      // Dest[63:32]  = Src2[63:32]
      // Dest[95:64]  = Src1[95:64]
      // Dest[127:96] = Src2[127:96]
      auto ConstantSwizzle = LoadAndCacheNamedVectorConstant(Size, FEXCore::IR::NamedVectorConstant::NAMED_VECTOR_BLENDPS_1011B);
      return _VTBX1(Size, Src1, Src2, ConstantSwizzle);
    }
    case 0b1100:
      // Dest[31:0]   = Src1[31:0]
      // Dest[63:32]  = Src1[63:32]
      // Dest[95:64]  = Src2[95:64]
      // Dest[127:96] = Src2[127:96]
      return _VInsElement(Size, OpSize::i64Bit, 1, 1, Src1, Src2);
    case 0b1101: {
      // Dest[31:0]   = Src2[31:0]
      // Dest[63:32]  = Src1[63:32]
      // Dest[95:64]  = Src2[95:64]
      // Dest[127:96] = Src2[127:96]
      auto ConstantSwizzle = LoadAndCacheNamedVectorConstant(Size, FEXCore::IR::NamedVectorConstant::NAMED_VECTOR_BLENDPS_1101B);
      return _VTBX1(Size, Src1, Src2, ConstantSwizzle);
    }
    case 0b1110: {
      // Dest[31:0]   = Src1[31:0]
      // Dest[63:32]  = Src2[63:32]
      // Dest[95:64]  = Src2[95:64]
      // Dest[127:96] = Src2[127:96]
      auto ConstantSwizzle = LoadAndCacheNamedVectorConstant(Size, FEXCore::IR::NamedVectorConstant::NAMED_VECTOR_BLENDPS_1110B);
      return _VTBX1(Size, Src1, Src2, ConstantSwizzle);
    }
    case 0b1111:
      // Dest[31:0]   = Src2[31:0]
      // Dest[63:32]  = Src2[63:32]
      // Dest[95:64]  = Src2[95:64]
      // Dest[127:96] = Src2[127:96]
      // Copy
      return Src2;
    default: break;
    }
  } else if (ElementSize == OpSize::i64Bit) {
    Selector &= 0b11;
    switch (Selector) {
    case 0b00:
      // No-op
      return Src1;
    case 0b01:
      // Dest[63:0]   = Src2[63:0]
      // Dest[127:64] = Src1[127:64]
      return _VInsElement(Size, ElementSize, 0, 0, Src1, Src2);
    case 0b10:
      // Dest[63:0]   = Src1[63:0]
      // Dest[127:64] = Src2[127:64]
      return _VInsElement(Size, ElementSize, 1, 1, Src1, Src2);
    case 0b11:
      // Copy
      return Src2;
    }
  } else {
    ///< Zero instruction copies
    switch (Selector) {
    case 0b0000'0000: return Src1;
    case 0b1111'1111: return Src2;
    default: break;
    }

    ///< Single instruction implementation
    switch (Selector) {
    case 0b0000'0001:
    case 0b0000'0010:
    case 0b0000'0100:
    case 0b0000'1000:
    case 0b0001'0000:
    case 0b0010'0000:
    case 0b0100'0000:
    case 0b1000'0000: {
      // Single 16-bit element insert.
      const auto Element = FEXCore::ilog2(Selector);
      return _VInsElement(Size, ElementSize, Element, Element, Src1, Src2);
    }
    case 0b1111'1110:
    case 0b1111'1101:
    case 0b1111'1011:
    case 0b1111'0111:
    case 0b1110'1111:
    case 0b1101'1111:
    case 0b1011'1111:
    case 0b0111'1111: {
      // Single 16-bit element insert, inverted
      uint8_t SelectorInvert = ~Selector;
      const auto Element = FEXCore::ilog2(SelectorInvert);
      return _VInsElement(Size, ElementSize, Element, Element, Src2, Src1);
    }
    case 0b0000'0011:
    case 0b0000'1100:
    case 0b0011'0000:
    case 0b1100'0000: {
      // Single 32-bit element insert.
      const auto Element = std::countr_zero(Selector) / 2;
      return _VInsElement(Size, OpSize::i32Bit, Element, Element, Src1, Src2);
    }
    case 0b1111'1100:
    case 0b1111'0011:
    case 0b1100'1111:
    case 0b0011'1111: {
      // Single 32-bit element insert, inverted
      uint8_t SelectorInvert = ~Selector;
      const auto Element = std::countr_zero(SelectorInvert) / 2;
      return _VInsElement(Size, OpSize::i32Bit, Element, Element, Src2, Src1);
    }
    case 0b0000'1111:
    case 0b1111'0000: {
      // Single 64-bit element insert.
      const auto Element = std::countr_zero(Selector) / 4;
      return _VInsElement(Size, OpSize::i64Bit, Element, Element, Src1, Src2);
    }
    default: break;
    }

    ///< Two instruction implementation
    switch (Selector) {
    ///< Fancy double VExtr
    case 0b0'0'0'0'0'1'1'1: {
      auto Tmp = _VExtr(OpSize::i128Bit, OpSize::i8Bit, Src2, Src1, 6);
      return _VExtr(OpSize::i128Bit, OpSize::i8Bit, Tmp, Tmp, 10);
    }
    case 0b0'0'0'1'1'1'1'1: {
      auto Tmp = _VExtr(OpSize::i128Bit, OpSize::i8Bit, Src2, Src1, 10);
      return _VExtr(OpSize::i128Bit, OpSize::i8Bit, Tmp, Tmp, 6);
    }
    case 0b1'1'1'0'0'0'0'0: {
      auto Tmp = _VExtr(OpSize::i128Bit, OpSize::i8Bit, Src1, Src2, 10);
      return _VExtr(OpSize::i128Bit, OpSize::i8Bit, Tmp, Tmp, 6);
    }
    case 0b1'1'1'1'1'0'0'0: {
      auto Tmp = _VExtr(OpSize::i128Bit, OpSize::i8Bit, Src1, Src2, 6);
      return _VExtr(OpSize::i128Bit, OpSize::i8Bit, Tmp, Tmp, 10);
    }
    default: break;
    }

    // TODO: There are some of these swizzles that can be more optimal.
    // NamedConstant + VTBX1 is quite quick already.
    // Implement more if it becomes relevant.
    auto ConstantSwizzle =
      LoadAndCacheIndexedNamedVectorConstant(Size, FEXCore::IR::IndexNamedVectorConstant::INDEXED_NAMED_VECTOR_PBLENDW, Selector * 16);
    return _VTBX1(Size, Src1, Src2, ConstantSwizzle);
  }

  FEX_UNREACHABLE;
}

template<size_t ElementSize>
void OpDispatchBuilder::VectorBlend(OpcodeArgs) {
  uint8_t Select = Op->Src[1].Literal();

  Ref Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Dest = VectorBlend(OpSize::i128Bit, ElementSize, Dest, Src, Select);
  StoreResult(FPRClass, Op, Dest, OpSize::iInvalid);
}

template void OpDispatchBuilder::VectorBlend<OpSize::i16Bit>(OpcodeArgs);
template void OpDispatchBuilder::VectorBlend<OpSize::i32Bit>(OpcodeArgs);
template void OpDispatchBuilder::VectorBlend<OpSize::i64Bit>(OpcodeArgs);

void OpDispatchBuilder::VectorVariableBlend(OpcodeArgs, size_t ElementSize) {
  auto Size = GetSrcSize(Op);

  Ref Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

  auto Mask = LoadXMMRegister(0);

  // Each element is selected by the high bit of that element size
  // Dest[ElementIdx] = Xmm0[ElementIndex][HighBit] ? Src : Dest;
  //
  // To emulate this on AArch64
  // Arithmetic shift right by the element size, then use BSL to select the registers
  Mask = _VSShrI(Size, ElementSize, Mask, (ElementSize * 8) - 1);

  auto Result = _VBSL(Size, Mask, Src, Dest);

  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::AVXVectorVariableBlend(OpcodeArgs, size_t ElementSize) {
  const auto SrcSize = GetSrcSize(Op);
  const auto ElementSizeBits = ElementSize * 8;

  Ref Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);

  // Mask register is encoded within bits [7:4] of the selector
  const auto Src3Selector = Op->Src[2].Literal();
  Ref Mask = LoadXMMRegister((Src3Selector >> 4) & 0b1111);

  Ref Shifted = _VSShrI(SrcSize, ElementSize, Mask, ElementSizeBits - 1);
  Ref Result = _VBSL(SrcSize, Shifted, Src2, Src1);
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::PTestOpImpl(OpSize Size, Ref Dest, Ref Src) {
  // Invalidate deferred flags early
  InvalidateDeferredFlags();

  Ref Test1 = _VAnd(Size, OpSize::i8Bit, Dest, Src);
  Ref Test2 = _VAndn(Size, OpSize::i8Bit, Src, Dest);

  // Element size must be less than 32-bit for the sign bit tricks.
  Test1 = _VUMaxV(Size, OpSize::i16Bit, Test1);
  Test2 = _VUMaxV(Size, OpSize::i16Bit, Test2);

  Test1 = _VExtractToGPR(Size, OpSize::i16Bit, Test1, 0);
  Test2 = _VExtractToGPR(Size, OpSize::i16Bit, Test2, 0);

  auto ZeroConst = _Constant(0);
  auto OneConst = _Constant(1);

  Test2 = _Select(FEXCore::IR::COND_NEQ, Test2, ZeroConst, OneConst, ZeroConst);

  // Careful, these flags are different between {V,}PTEST and VTESTP{S,D}
  // Set ZF according to Test1. SF will be zeroed since we do a 32-bit test on
  // the results of a 16-bit value from the UMaxV, so the 32-bit sign bit is
  // cleared even if the 16-bit scalars were negative.
  SetNZ_ZeroCV(32, Test1);
  SetCFInverted(Test2);
  ZeroPF_AF();
}

void OpDispatchBuilder::PTestOp(OpcodeArgs) {
  Ref Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

  PTestOpImpl(OpSizeFromSrc(Op), Dest, Src);
}

void OpDispatchBuilder::VTESTOpImpl(OpSize SrcSize, size_t ElementSize, Ref Src1, Ref Src2) {
  InvalidateDeferredFlags();

  const auto ElementSizeInBits = ElementSize * 8;
  const auto MaskConstant = uint64_t {1} << (ElementSizeInBits - 1);

  Ref Mask = _VDupFromGPR(SrcSize, ElementSize, _Constant(MaskConstant));

  Ref AndTest = _VAnd(SrcSize, OpSize::i8Bit, Src2, Src1);
  Ref AndNotTest = _VAndn(SrcSize, OpSize::i8Bit, Src2, Src1);

  Ref MaskedAnd = _VAnd(SrcSize, OpSize::i8Bit, AndTest, Mask);
  Ref MaskedAndNot = _VAnd(SrcSize, OpSize::i8Bit, AndNotTest, Mask);

  Ref MaxAnd = _VUMaxV(SrcSize, OpSize::i16Bit, MaskedAnd);
  Ref MaxAndNot = _VUMaxV(SrcSize, OpSize::i16Bit, MaskedAndNot);

  Ref AndGPR = _VExtractToGPR(SrcSize, OpSize::i16Bit, MaxAnd, 0);
  Ref AndNotGPR = _VExtractToGPR(SrcSize, OpSize::i16Bit, MaxAndNot, 0);

  Ref ZeroConst = _Constant(0);
  Ref OneConst = _Constant(1);

  Ref CFInv = _Select(IR::COND_NEQ, AndNotGPR, ZeroConst, OneConst, ZeroConst);

  // As in PTest, this sets Z appropriately while zeroing the rest of NZCV.
  SetNZ_ZeroCV(32, AndGPR);
  SetCFInverted(CFInv);
  ZeroPF_AF();
}

template<size_t ElementSize>
void OpDispatchBuilder::VTESTPOp(OpcodeArgs) {
  Ref Src1 = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  Ref Src2 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

  VTESTOpImpl(OpSizeFromSrc(Op), ElementSize, Src1, Src2);
}
template void OpDispatchBuilder::VTESTPOp<OpSize::i32Bit>(OpcodeArgs);
template void OpDispatchBuilder::VTESTPOp<OpSize::i64Bit>(OpcodeArgs);

Ref OpDispatchBuilder::PHMINPOSUWOpImpl(OpcodeArgs) {
  const auto Size = GetSrcSize(Op);

  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

  // Setup a vector swizzle
  // Initially load a 64-bit mask of immediates
  // Then zero-extend that to 128-bit mask with the immediates in the lower 16-bits of each element
  auto ConstantSwizzle = LoadAndCacheNamedVectorConstant(Size, FEXCore::IR::NamedVectorConstant::NAMED_VECTOR_INCREMENTAL_U16_INDEX);

  // We now need to zip the vector sources together to become two uint32x4_t vectors
  // Upper:
  // [127:96]: ([127:112] << 16) | (7)
  // [95:64] : ([111:96]  << 16) | (6)
  // [63:32] : ([95:80]   << 16) | (5)
  // [31:0]  : ([79:64]   << 16) | (4)

  // Lower:
  // [127:96]: ([63:48] << 16) | (3)
  // [95:64] : ([47:32] << 16) | (2)
  // [63:32] : ([31:16] << 16) | (1)
  // [31:0]  : ([15:0]  << 16) | (0)

  auto ZipLower = _VZip(Size, OpSize::i16Bit, ConstantSwizzle, Src);
  auto ZipUpper = _VZip2(Size, OpSize::i16Bit, ConstantSwizzle, Src);
  // The elements are now 32-bit between two vectors.
  auto MinBetween = _VUMin(Size, OpSize::i32Bit, ZipLower, ZipUpper);

  // Now do a horizontal vector minimum
  auto Min = _VUMinV(Size, OpSize::i32Bit, MinBetween);

  // We now have a value in the bottom 32-bits in the order of:
  // [31:0]: (Src[<Min>] << 16) | <Index>
  // This instruction wants it in the form of:
  // [31:0]: (<Index> << 16) | Src[<Min>]
  // Rev32 does this for us
  return _VRev32(Size, OpSize::i16Bit, Min);
}

void OpDispatchBuilder::PHMINPOSUWOp(OpcodeArgs) {
  Ref Result = PHMINPOSUWOpImpl(Op);
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

Ref OpDispatchBuilder::DPPOpImpl(size_t DstSize, Ref Src1, Ref Src2, uint8_t Mask, size_t ElementSize) {
  const auto SizeMask = [ElementSize]() {
    if (ElementSize == OpSize::i32Bit) {
      return 0b1111;
    }
    return 0b11;
  }();

  const uint8_t SrcMask = (Mask >> 4) & SizeMask;
  const uint8_t DstMask = Mask & SizeMask;

  const auto NamedIndexMask = [ElementSize]() {
    if (ElementSize == OpSize::i32Bit) {
      return FEXCore::IR::IndexNamedVectorConstant::INDEXED_NAMED_VECTOR_DPPS_MASK;
    }

    return FEXCore::IR::IndexNamedVectorConstant::INDEXED_NAMED_VECTOR_DPPD_MASK;
  }();

  Ref ZeroVec = LoadZeroVector(DstSize);
  if (SrcMask == 0 || DstMask == 0) {
    // What are you even doing here? Go away.
    return ZeroVec;
  }

  // First step is to do an FMUL
  Ref Temp = _VFMul(DstSize, ElementSize, Src1, Src2);

  // Now mask results based on IndexMask.
  if (SrcMask != SizeMask) {
    auto InputMask = LoadAndCacheIndexedNamedVectorConstant(DstSize, NamedIndexMask, SrcMask * 16);
    Temp = _VAnd(DstSize, ElementSize, Temp, InputMask);
  }

  // Now due a float reduction
  Temp = _VFAddV(DstSize, ElementSize, Temp);

  // Now using the destination mask we choose where the result ends up
  // It can duplicate and zero results
  if (ElementSize == 8) {
    switch (DstMask) {
    case 0b01:
      // Dest[63:0] = Result
      // Dest[127:64] = Zero
      return _VZip(DstSize, ElementSize, Temp, ZeroVec);
    case 0b10:
      // Dest[63:0] = Zero
      // Dest[127:64] = Result
      return _VZip(DstSize, ElementSize, ZeroVec, Temp);
    case 0b11:
      // Broadcast
      // Dest[63:0] = Result
      // Dest[127:64] = Result
      return _VDupElement(DstSize, ElementSize, Temp, 0);
    case 0:
    default: LOGMAN_MSG_A_FMT("Unsupported");
    }
  } else {
    auto BadPath = [&]() {
      Ref Result = ZeroVec;

      for (size_t i = 0; i < (DstSize / ElementSize); ++i) {
        const auto Bit = 1U << (i % 4);

        if ((DstMask & Bit) != 0) {
          Result = _VInsElement(DstSize, ElementSize, i, 0, Result, Temp);
        }
      }

      return Result;
    };
    switch (DstMask) {
    case 0b0001:
      // Dest[31:0]   = Result
      // Dest[63:32]  = Zero
      // Dest[95:64]  = Zero
      // Dest[127:96] = Zero
      return _VZip(DstSize, ElementSize, Temp, ZeroVec);
    case 0b0010:
      // Dest[31:0]   = Zero
      // Dest[63:32]  = Result
      // Dest[95:64]  = Zero
      // Dest[127:96] = Zero
      return _VZip(DstSize / 2, ElementSize, ZeroVec, Temp);
    case 0b0011:
      // Dest[31:0]   = Result
      // Dest[63:32]  = Result
      // Dest[95:64]  = Zero
      // Dest[127:96] = Zero
      return _VDupElement(DstSize / 2, ElementSize, Temp, 0);
    case 0b0100:
      // Dest[31:0]   = Zero
      // Dest[63:32]  = Zero
      // Dest[95:64]  = Result
      // Dest[127:96] = Zero
      return _VZip(DstSize, OpSize::i64Bit, ZeroVec, Temp);
    case 0b0101:
      // Dest[31:0]   = Result
      // Dest[63:32]  = Zero
      // Dest[95:64]  = Result
      // Dest[127:96] = Zero
      return _VZip(DstSize, OpSize::i64Bit, Temp, Temp);
    case 0b0110:
      // Dest[31:0]   = Zero
      // Dest[63:32]  = Result
      // Dest[95:64]  = Result
      // Dest[127:96] = Zero
      return BadPath();
    case 0b0111:
      // Dest[31:0]   = Result
      // Dest[63:32]  = Result
      // Dest[95:64]  = Result
      // Dest[127:96] = Zero
      Temp = _VDupElement(DstSize, ElementSize, Temp, 0);
      return _VInsElement(DstSize, ElementSize, 3, 0, Temp, ZeroVec);
    case 0b1000:
      // Dest[31:0]   = Zero
      // Dest[63:32]  = Zero
      // Dest[95:64]  = Zero
      // Dest[127:96] = Result
      return _VExtr(DstSize, OpSize::i8Bit, Temp, ZeroVec, 4);
    case 0b1001:
      // Dest[31:0]   = Result
      // Dest[63:32]  = Zero
      // Dest[95:64]  = Zero
      // Dest[127:96] = Result
      return BadPath();
    case 0b1010:
      // Dest[31:0]   = Zero
      // Dest[63:32]  = Result
      // Dest[95:64]  = Zero
      // Dest[127:96] = Result
      Temp = _VDupElement(DstSize, ElementSize, Temp, 0);
      return _VZip(DstSize, OpSize::i32Bit, ZeroVec, Temp);
    case 0b1011:
      // Dest[31:0]   = Result
      // Dest[63:32]  = Result
      // Dest[95:64]  = Zero
      // Dest[127:96] = Result
      Temp = _VDupElement(DstSize, ElementSize, Temp, 0);
      return _VInsElement(DstSize, ElementSize, 2, 0, Temp, ZeroVec);
    case 0b1100:
      // Dest[31:0]   = Zero
      // Dest[63:32]  = Zero
      // Dest[95:64]  = Result
      // Dest[127:96] = Result
      Temp = _VDupElement(DstSize, ElementSize, Temp, 0);
      return _VZip(DstSize, OpSize::i64Bit, ZeroVec, Temp);
    case 0b1101:
      // Dest[31:0]   = Result
      // Dest[63:32]  = Zero
      // Dest[95:64]  = Result
      // Dest[127:96] = Result
      Temp = _VDupElement(DstSize, ElementSize, Temp, 0);
      return _VInsElement(DstSize, ElementSize, 1, 0, Temp, ZeroVec);
    case 0b1110:
      // Dest[31:0]   = Zero
      // Dest[63:32]  = Result
      // Dest[95:64]  = Result
      // Dest[127:96] = Result
      Temp = _VDupElement(DstSize, ElementSize, Temp, 0);
      return _VInsElement(DstSize, ElementSize, 0, 0, Temp, ZeroVec);
    case 0b1111:
      // Broadcast
      // Dest[31:0]   = Result
      // Dest[63:32]  = Zero
      // Dest[95:64]  = Zero
      // Dest[127:96] = Zero
      return _VDupElement(DstSize, ElementSize, Temp, 0);
    case 0:
    default: LOGMAN_MSG_A_FMT("Unsupported");
    }
  }
  FEX_UNREACHABLE;
}

template<size_t ElementSize>
void OpDispatchBuilder::DPPOp(OpcodeArgs) {

  Ref Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

  Ref Result = DPPOpImpl(GetDstSize(Op), Dest, Src, Op->Src[1].Literal(), ElementSize);
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

template void OpDispatchBuilder::DPPOp<OpSize::i32Bit>(OpcodeArgs);
template void OpDispatchBuilder::DPPOp<OpSize::i64Bit>(OpcodeArgs);

Ref OpDispatchBuilder::VDPPSOpImpl(OpcodeArgs, const X86Tables::DecodedOperand& Src1, const X86Tables::DecodedOperand& Src2,
                                   const X86Tables::DecodedOperand& Imm) {
  constexpr size_t ElementSize = OpSize::i32Bit;
  const uint8_t Mask = Imm.Literal();
  const uint8_t SrcMask = Mask >> 4;
  const uint8_t DstMask = Mask & 0xF;

  const auto DstSize = GetDstSize(Op);

  Ref Src1V = LoadSource(FPRClass, Op, Src1, Op->Flags);
  Ref Src2V = LoadSource(FPRClass, Op, Src2, Op->Flags);

  Ref ZeroVec = LoadZeroVector(DstSize);

  // First step is to do an FMUL
  Ref Temp = _VFMul(DstSize, ElementSize, Src1V, Src2V);

  // Now we zero out elements based on src mask
  for (size_t i = 0; i < (DstSize / ElementSize); ++i) {
    const auto Bit = 1U << (i % 4);

    if ((SrcMask & Bit) == 0) {
      Temp = _VInsElement(DstSize, ElementSize, i, 0, Temp, ZeroVec);
    }
  }

  // Now we need to do a horizontal add of the elements
  // We only have pairwise float add so this needs to be done in steps
  Temp = _VFAddP(DstSize, ElementSize, Temp, ZeroVec);

  if (ElementSize == OpSize::i32Bit) {
    // For 32-bit float we need one more step to add all four results together
    Temp = _VFAddP(DstSize, ElementSize, Temp, ZeroVec);
  }

  // Now using the destination mask we choose where the result ends up
  // It can duplicate and zero results
  Ref Result = ZeroVec;

  for (size_t i = 0; i < (DstSize / ElementSize); ++i) {
    const auto Bit = 1U << (i % 4);

    if ((DstMask & Bit) != 0) {
      Result = _VInsElement(DstSize, ElementSize, i, 0, Result, Temp);
    }
  }

  return Result;
}

template<size_t ElementSize>
void OpDispatchBuilder::VDPPOp(OpcodeArgs) {
  const auto DstSize = GetDstSize(Op);

  Ref Result {};
  if (ElementSize == 4 && DstSize == Core::CPUState::XMM_AVX_REG_SIZE) {
    // 256-bit DPPS isn't handled by the 128-bit solution.
    Result = VDPPSOpImpl(Op, Op->Src[0], Op->Src[1], Op->Src[2]);
  } else {
    Ref Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
    Ref Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);

    Result = DPPOpImpl(GetDstSize(Op), Src1, Src2, Op->Src[2].Literal(), ElementSize);
  }

  // We don't need to emit a _VMov to clear the upper lane, since DPPOpImpl uses a zero vector
  // to construct the results, so the upper lane will always be cleared for the 128-bit version.
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

template void OpDispatchBuilder::VDPPOp<OpSize::i32Bit>(OpcodeArgs);
template void OpDispatchBuilder::VDPPOp<OpSize::i64Bit>(OpcodeArgs);

Ref OpDispatchBuilder::MPSADBWOpImpl(size_t SrcSize, Ref Src1, Ref Src2, uint8_t Select) {
  const auto LaneHelper = [&, this](uint32_t Selector_Src1, uint32_t Selector_Src2, Ref Src1, Ref Src2) {
    // Src2 will grab a 32bit element and duplicate it across the 128bits
    Ref DupSrc = _VDupElement(OpSize::i128Bit, OpSize::i32Bit, Src2, Selector_Src2);

    // Src1/Dest needs a bunch of magic

    // Shift right by selected bytes
    // This will give us Dest[15:0], and Dest[79:64]
    Ref Dest1 = _VExtr(OpSize::i128Bit, OpSize::i8Bit, Src1, Src1, Selector_Src1 + 0);
    // This will give us Dest[31:16], and Dest[95:80]
    Ref Dest2 = _VExtr(OpSize::i128Bit, OpSize::i8Bit, Src1, Src1, Selector_Src1 + 1);
    // This will give us Dest[47:32], and Dest[111:96]
    Ref Dest3 = _VExtr(OpSize::i128Bit, OpSize::i8Bit, Src1, Src1, Selector_Src1 + 2);
    // This will give us Dest[63:48], and Dest[127:112]
    Ref Dest4 = _VExtr(OpSize::i128Bit, OpSize::i8Bit, Src1, Src1, Selector_Src1 + 3);

    // For each shifted section, we now have two 32-bit values per vector that can be used
    // Dest1.S[0] and Dest1.S[1] = Bytes - 0,1,2,3:4,5,6,7
    // Dest2.S[0] and Dest2.S[1] = Bytes - 1,2,3,4:5,6,7,8
    // Dest3.S[0] and Dest3.S[1] = Bytes - 2,3,4,5:6,7,8,9
    // Dest4.S[0] and Dest4.S[1] = Bytes - 3,4,5,6:7,8,9,10
    Dest1 = _VUABDL(OpSize::i128Bit, OpSize::i8Bit, Dest1, DupSrc);
    Dest2 = _VUABDL(OpSize::i128Bit, OpSize::i8Bit, Dest2, DupSrc);
    Dest3 = _VUABDL(OpSize::i128Bit, OpSize::i8Bit, Dest3, DupSrc);
    Dest4 = _VUABDL(OpSize::i128Bit, OpSize::i8Bit, Dest4, DupSrc);

    // Dest[1,2,3,4] Now contains the data prior to combining
    // Temp[0,1,2,3] for each step

    // Each destination now has 16bit x 8 elements in it that were the absolute difference for each byte
    // Needs each to be 16bit to store the next step
    // Next stage is to sum pairwise
    // Dest1:
    //  ADDP Dest3, Dest1: TmpCombine1
    //  ADDP Dest4, Dest2: TmpCombine2
    //    TmpCombine1.8H[0] = Dest1.8H[0] + Dest1.8H[1];
    //    TmpCombine1.8H[1] = Dest1.8H[2] + Dest1.8H[3];
    //    TmpCombine1.8H[2] = Dest1.8H[4] + Dest1.8H[5];
    //    TmpCombine1.8H[3] = Dest1.8H[6] + Dest1.8H[7];
    //    TmpCombine1.8H[4] = Dest3.8H[0] + Dest3.8H[1];
    //    TmpCombine1.8H[5] = Dest3.8H[2] + Dest3.8H[3];
    //    TmpCombine1.8H[6] = Dest3.8H[4] + Dest3.8H[5];
    //    TmpCombine1.8H[7] = Dest3.8H[6] + Dest3.8H[7];
    //    <Repeat for Dest4 and Dest3>
    auto TmpCombine1 = _VAddP(OpSize::i128Bit, OpSize::i16Bit, Dest1, Dest3);
    auto TmpCombine2 = _VAddP(OpSize::i128Bit, OpSize::i16Bit, Dest2, Dest4);

    // TmpTranspose1:
    // VTrn TmpCombine1, TmpCombine2: TmpTranspose1
    // Transposes Even and odd elements so we can use vaddp for final results.
    auto TmpTranspose1 = _VTrn(OpSize::i128Bit, OpSize::i32Bit, TmpCombine1, TmpCombine2);
    auto TmpTranspose2 = _VTrn2(OpSize::i128Bit, OpSize::i32Bit, TmpCombine1, TmpCombine2);

    // ADDP TmpTranspose1, TmpTranspose2: FinalCombine
    //    FinalCombine.8H[0] = TmpTranspose1.8H[0] + TmpTranspose1.8H[1]
    //    FinalCombine.8H[1] = TmpTranspose1.8H[2] + TmpTranspose1.8H[3]
    //    FinalCombine.8H[2] = TmpTranspose1.8H[4] + TmpTranspose1.8H[5]
    //    FinalCombine.8H[3] = TmpTranspose1.8H[6] + TmpTranspose1.8H[7]
    //    FinalCombine.8H[4] = TmpTranspose2.8H[0] + TmpTranspose2.8H[1]
    //    FinalCombine.8H[5] = TmpTranspose2.8H[2] + TmpTranspose2.8H[3]
    //    FinalCombine.8H[6] = TmpTranspose2.8H[4] + TmpTranspose2.8H[5]
    //    FinalCombine.8H[7] = TmpTranspose2.8H[6] + TmpTranspose2.8H[7]

    return _VAddP(OpSize::i128Bit, OpSize::i16Bit, TmpTranspose1, TmpTranspose2);
  };

  const auto Is128Bit = SrcSize == Core::CPUState::XMM_SSE_REG_SIZE;

  // Src1 needs to be in byte offset
  const uint8_t Select_Src1_Low = ((Select & 0b100) >> 2) * 32 / 8;
  const uint8_t Select_Src2_Low = Select & 0b11;

  Ref Lower = LaneHelper(Select_Src1_Low, Select_Src2_Low, Src1, Src2);
  if (Is128Bit) {
    return Lower;
  }

  const uint8_t Select_Src1_High = ((Select & 0b100000) >> 5) * 32 / 8;
  const uint8_t Select_Src2_High = (Select & 0b11000) >> 3;

  Ref UpperSrc1 = _VDupElement(OpSize::i256Bit, OpSize::i128Bit, Src1, 1);
  Ref UpperSrc2 = _VDupElement(OpSize::i256Bit, OpSize::i128Bit, Src2, 1);
  Ref Upper = LaneHelper(Select_Src1_High, Select_Src2_High, UpperSrc1, UpperSrc2);
  return _VInsElement(OpSize::i256Bit, OpSize::i128Bit, 1, 0, Lower, Upper);
}

void OpDispatchBuilder::MPSADBWOp(OpcodeArgs) {
  const uint8_t Select = Op->Src[1].Literal();
  const uint8_t SrcSize = GetSrcSize(Op);
  Ref Src1 = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  Ref Src2 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

  Ref Result = MPSADBWOpImpl(SrcSize, Src1, Src2, Select);
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::VMPSADBWOp(OpcodeArgs) {
  const uint8_t Select = Op->Src[2].Literal();
  const uint8_t SrcSize = GetSrcSize(Op);
  Ref Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);

  Ref Result = MPSADBWOpImpl(SrcSize, Src1, Src2, Select);
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::VINSERTOp(OpcodeArgs) {
  const auto DstSize = GetDstSize(Op);
  Ref Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Src2 = LoadSource_WithOpSize(FPRClass, Op, Op->Src[1], OpSize::i128Bit, Op->Flags);

  const auto Selector = Op->Src[2].Literal() & 1;
  Ref Result = _VInsElement(DstSize, OpSize::i128Bit, Selector, 0, Src1, Src2);

  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::VCVTPH2PSOp(OpcodeArgs) {
  // In the event that a memory operand is used as the source operand,
  // the access width will always be half the size of the destination vector width
  // (i.e. 128-bit vector -> 64-bit mem, 256-bit vector -> 128-bit mem)
  const auto DstSize = OpSizeFromDst(Op);
  const auto SrcLoadSize = Op->Src[0].IsGPR() ? DstSize : IR::SizeToOpSize(IR::OpSizeToSize(DstSize) / 2);

  Ref Src = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], SrcLoadSize, Op->Flags);
  Ref Result = _Vector_FToF(DstSize, OpSize::i32Bit, Src, OpSize::i16Bit);

  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::VCVTPS2PHOp(OpcodeArgs) {
  const auto SrcSize = OpSizeFromSrc(Op);
  const auto StoreSize = Op->Dest.IsGPR() ? OpSize::i128Bit : IR::SizeToOpSize(IR::OpSizeToSize(SrcSize) / 2);

  const auto Imm8 = Op->Src[1].Literal();
  const auto UseMXCSR = (Imm8 & 0b100) != 0;

  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

  Ref Result = nullptr;
  if (UseMXCSR) {
    Result = _Vector_FToF(SrcSize, OpSize::i16Bit, Src, OpSize::i32Bit);
  } else {
    // No ARM float conversion instructions allow passing in
    // a rounding mode as an immediate. All of them depend on
    // the RM field in the FPCR. And so! We have to do some ugly
    // rounding mode shuffling.
    const auto NewRMode = Imm8 & 0b11;
    Ref SavedFPCR = _PushRoundingMode(NewRMode);

    Result = _Vector_FToF(SrcSize, OpSize::i16Bit, Src, OpSize::i32Bit);
    _PopRoundingMode(SavedFPCR);
  }

  // We need to eliminate upper junk if we're storing into a register with
  // a 256-bit source (VCVTPS2PH's destination for registers is an XMM).
  if (Op->Src[0].IsGPR() && SrcSize == Core::CPUState::XMM_AVX_REG_SIZE) {
    Result = _VMov(OpSize::i128Bit, Result);
  }

  StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Result, StoreSize, OpSize::iInvalid);
}

void OpDispatchBuilder::VPERM2Op(OpcodeArgs) {
  const auto DstSize = GetDstSize(Op);
  Ref Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);

  const auto Selector = Op->Src[2].Literal();
  Ref Result = LoadZeroVector(DstSize);

  const auto SelectElement = [&](uint64_t Index, uint64_t SelectorIdx) {
    switch (SelectorIdx) {
    case 0:
    case 1: return _VInsElement(DstSize, OpSize::i128Bit, Index, SelectorIdx, Result, Src1);
    case 2:
    case 3:
    default: return _VInsElement(DstSize, OpSize::i128Bit, Index, SelectorIdx - 2, Result, Src2);
    }
  };

  if ((Selector & 0b00001000) == 0) {
    Result = SelectElement(0, Selector & 0b11);
  }
  if ((Selector & 0b10000000) == 0) {
    Result = SelectElement(1, (Selector >> 4) & 0b11);
  }

  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

Ref OpDispatchBuilder::VPERMDIndices(OpSize DstSize, Ref Indices, Ref IndexMask, Ref Repeating3210) {
  // Get rid of any junk unrelated to the relevant selector index bits (bits [2:0])
  Ref SanitizedIndices = _VAnd(DstSize, OpSize::i8Bit, Indices, IndexMask);

  // Build up the broadcasted index mask. e.g. On x86-64, the selector index
  // is always in the lower 3 bits of a 32-bit element. However, in order to
  // build up a vector we can use with the ARMv8 TBL instruction, we need the
  // selector index for each particular element to be within each byte of the
  // 32-bit element.
  //
  // We can do this by TRN-ing the selector index vector twice. Once using byte elements
  // then once more using half-word elements.
  //
  // The first pass creates the half-word elements, and then the second pass uses those
  // halfword elements to place the indices in the top part of the 32-bit element.
  //
  // e.g. Consider a selector vector with indices in 32-bit elements like:
  //
  // ╔═══════════╗╔═══════════╗╔═══════════╗╔═══════════╗╔═══════════╗╔═══════════╗╔═══════════╗╔═══════════╗
  // ║     4     ║║     1     ║║     2     ║║     6     ║║     7     ║║     0     ║║     3     ║║     5     ║
  // ╚═══════════╝╚═══════════╝╚═══════════╝╚═══════════╝╚═══════════╝╚═══════════╝╚═══════════╝╚═══════════╝
  //
  // TRNing once using byte elements by itself will create a vector with 8-bit elements like:
  // ╔═══╗╔═══╗╔═══╗╔═══╗╔═══╗╔═══╗╔═══╗╔═══╗╔═══╗╔═══╗╔═══╗╔═══╗╔═══╗╔═══╗╔═══╗╔═══╗╔═══╗╔═══╗╔═══╗╔═══╗╔═══╗╔═══╗╔═══╗╔═══╗╔═══╗╔═══╗╔═══╗╔═══╗╔═══╗╔═══╗╔═══╗╔═══╗
  // ║ 0 ║║ 0 ║║ 4 ║║ 4 ║║ 0 ║║ 0 ║║ 1 ║║ 1 ║║ 0 ║║ 0 ║║ 2 ║║ 2 ║║ 0 ║║ 0 ║║ 6 ║║ 6 ║║ 0 ║║ 0 ║║ 7 ║║ 7 ║║ 0 ║║ 0 ║║ 0 ║║ 0 ║║ 0 ║║ 0 ║║ 3 ║║ 3 ║║ 0 ║║ 0 ║║ 5 ║║ 5 ║
  // ╚═══╝╚═══╝╚═══╝╚═══╝╚═══╝╚═══╝╚═══╝╚═══╝╚═══╝╚═══╝╚═══╝╚═══╝╚═══╝╚═══╝╚═══╝╚═══╝╚═══╝╚═══╝╚═══╝╚═══╝╚═══╝╚═══╝╚═══╝╚═══╝╚═══╝╚═══╝╚═══╝╚═══╝╚═══╝╚═══╝╚═══╝╚═══╝
  //
  // TRNing once using half-word elements by itself will then transform the vector into:
  // ╔═══╗╔═══╗╔═══╗╔═══╗╔═══╗╔═══╗╔═══╗╔═══╗╔═══╗╔═══╗╔═══╗╔═══╗╔═══╗╔═══╗╔═══╗╔═══╗╔═══╗╔═══╗╔═══╗╔═══╗╔═══╗╔═══╗╔═══╗╔═══╗╔═══╗╔═══╗╔═══╗╔═══╗╔═══╗╔═══╗╔═══╗╔═══╗
  // ║ 4 ║║ 4 ║║ 4 ║║ 4 ║║ 1 ║║ 1 ║║ 1 ║║ 1 ║║ 2 ║║ 2 ║║ 2 ║║ 2 ║║ 6 ║║ 6 ║║ 6 ║║ 6 ║║ 7 ║║ 7 ║║ 7 ║║ 7 ║║ 0 ║║ 0 ║║ 0 ║║ 0 ║║ 3 ║║ 3 ║║ 3 ║║ 3 ║║ 5 ║║ 5 ║║ 5 ║║ 5 ║
  // ╚═══╝╚═══╝╚═══╝╚═══╝╚═══╝╚═══╝╚═══╝╚═══╝╚═══╝╚═══╝╚═══╝╚═══╝╚═══╝╚═══╝╚═══╝╚═══╝╚═══╝╚═══╝╚═══╝╚═══╝╚═══╝╚═══╝╚═══╝╚═══╝╚═══╝╚═══╝╚═══╝╚═══╝╚═══╝╚═══╝╚═══╝╚═══╝
  //
  // Cool! We now have everything we need to take this further.

  Ref IndexTrn1 = _VTrn(DstSize, OpSize::i8Bit, SanitizedIndices, SanitizedIndices);
  Ref IndexTrn2 = _VTrn(DstSize, OpSize::i16Bit, IndexTrn1, IndexTrn1);

  // Now that we have the indices set up, now we need to multiply each
  // element by 4 to convert the elements into byte indices rather than
  // 32-bit word indices.
  //
  // e.g. We turn our vector into:
  // ╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗
  // ║ 16 ║║ 16 ║║ 16 ║║ 16 ║║ 4  ║║ 4  ║║ 4  ║║ 4  ║║ 8  ║║ 8  ║║ 8  ║║ 8  ║║ 24 ║║ 24 ║║ 24 ║║ 24 ║║ 28 ║║ 28 ║║ 28 ║║ 28 ║║ 0  ║║ 0  ║║ 00 ║║ 0  ║║ 12 ║║ 12 ║║ 12 ║║ 12 ║║ 20 ║║ 20 ║║ 20 ║║ 20 ║
  // ╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝
  //
  Ref ShiftedIndices = _VShlI(DstSize, OpSize::i8Bit, IndexTrn2, 2);

  // Now we need to add a byte vector containing [3, 2, 1, 0] repeating for the
  // entire length of it, to the index register, so that we specify the bytes
  // that make up the entire word in the source register.
  //
  // e.g. Our vector finally looks like so:
  //
  // ╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗
  // ║ 19 ║║ 18 ║║ 17 ║║ 16 ║║ 7  ║║ 6  ║║ 5  ║║ 4  ║║ 11 ║║ 10 ║║ 9  ║║ 8  ║║ 27 ║║ 26 ║║ 25 ║║ 24 ║║ 31 ║║ 30 ║║ 29 ║║ 28 ║║ 3  ║║ 2  ║║ 01 ║║ 0  ║║ 15 ║║ 14 ║║ 13 ║║ 12 ║║ 23 ║║ 22 ║║ 21 ║║ 20 ║
  // ╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝
  //
  // Which finally lets us permute the source vector and be done with everything.
  return _VAdd(DstSize, OpSize::i8Bit, ShiftedIndices, Repeating3210);
}

void OpDispatchBuilder::VPERMDOp(OpcodeArgs) {
  const auto DstSize = GetDstSize(Op);

  Ref Indices = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Src = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);

  // Get rid of any junk unrelated to the relevant selector index bits (bits [2:0])
  Ref IndexMask = _VectorImm(DstSize, OpSize::i32Bit, 0b111);

  Ref AddConst = _Constant(0x03020100);
  Ref Repeating3210 = _VDupFromGPR(DstSize, OpSize::i32Bit, AddConst);
  Ref FinalIndices = VPERMDIndices(OpSizeFromDst(Op), Indices, IndexMask, Repeating3210);

  // Now lets finally shuffle this bad boy around.
  Ref Result = _VTBL1(DstSize, Src, FinalIndices);

  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::VPERMQOp(OpcodeArgs) {
  const auto DstSize = GetDstSize(Op);
  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

  const auto Selector = Op->Src[1].Literal();
  Ref Result {};

  // If we're just broadcasting one element in particular across the vector
  // then this can be done fairly simply without any individual inserts.
  if (Selector == 0x00 || Selector == 0x55 || Selector == 0xAA || Selector == 0xFF) {
    const auto Index = Selector & 0b11;
    Result = _VDupElement(DstSize, OpSize::i64Bit, Src, Index);
  } else {
    Result = LoadZeroVector(DstSize);
    for (size_t i = 0; i < DstSize / 8; i++) {
      const auto SrcIndex = (Selector >> (i * 2)) & 0b11;
      Result = _VInsElement(DstSize, OpSize::i64Bit, i, SrcIndex, Result, Src);
    }
  }
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

Ref OpDispatchBuilder::VBLENDOpImpl(uint32_t VecSize, uint32_t ElementSize, Ref Src1, Ref Src2, Ref ZeroRegister, uint64_t Selector) {
  const std::array Sources {Src1, Src2};

  Ref Result = ZeroRegister;
  const int NumElements = VecSize / ElementSize;
  for (int i = 0; i < NumElements; i++) {
    const auto SelectorIndex = (Selector >> i) & 1;

    Result = _VInsElement(VecSize, ElementSize, i, i, Result, Sources[SelectorIndex]);
  }

  return Result;
}

void OpDispatchBuilder::VBLENDPDOp(OpcodeArgs) {
  const auto DstSize = GetDstSize(Op);
  const auto Is256Bit = DstSize == Core::CPUState::XMM_AVX_REG_SIZE;
  const auto Selector = Op->Src[2].Literal();

  Ref Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);

  if (Selector == 0) {
    Ref Result = Is256Bit ? Src1 : _VMov(OpSize::i128Bit, Src1);
    StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
    return;
  }
  // Only the first four bits of the 8-bit immediate are used, so only check them.
  if (((Selector & 0b11) == 0b11 && !Is256Bit) || (Selector & 0b1111) == 0b1111) {
    Ref Result = Is256Bit ? Src2 : _VMov(OpSize::i128Bit, Src2);
    StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
    return;
  }

  const auto ZeroRegister = LoadZeroVector(DstSize);
  Ref Result = VBLENDOpImpl(DstSize, OpSize::i64Bit, Src1, Src2, ZeroRegister, Selector);
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::VPBLENDDOp(OpcodeArgs) {
  const auto DstSize = GetDstSize(Op);
  const auto Is256Bit = DstSize == Core::CPUState::XMM_AVX_REG_SIZE;
  const auto Selector = Op->Src[2].Literal();

  Ref Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);

  // Each bit in the selector chooses between Src1 and Src2.
  // If a bit is set, then we select it's corresponding 32-bit element from Src2
  // If a bit is not set, then we select it's corresponding 32-bit element from Src1

  // Cases where we can exit out early, since the selector is indicating a copy
  // of an entire input vector. Unlikely to occur, since it's slower than
  // just an equivalent vector move instruction. but just in case something
  // silly is happening, we have your back.

  if (Selector == 0) {
    Ref Result = Is256Bit ? Src1 : _VMov(OpSize::i128Bit, Src1);
    StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
    return;
  }
  if (Selector == 0xFF && Is256Bit) {
    Ref Result = Is256Bit ? Src2 : _VMov(OpSize::i128Bit, Src2);
    StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
    return;
  }
  // The only bits we care about from the 8-bit immediate for 128-bit operations
  // are the first four bits. We do a bitwise check here to catch cases where
  // silliness is going on and the upper bits are being set even when they'll
  // be ignored
  if ((Selector & 0xF) == 0xF && !Is256Bit) {
    StoreResult(FPRClass, Op, _VMov(OpSize::i128Bit, Src2), OpSize::iInvalid);
    return;
  }

  const auto ZeroRegister = LoadZeroVector(DstSize);
  Ref Result = VBLENDOpImpl(DstSize, OpSize::i32Bit, Src1, Src2, ZeroRegister, Selector);
  if (!Is256Bit) {
    Result = _VMov(OpSize::i128Bit, Result);
  }
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::VPBLENDWOp(OpcodeArgs) {
  const auto DstSize = GetDstSize(Op);
  const auto Is128Bit = DstSize == Core::CPUState::XMM_SSE_REG_SIZE;
  const auto Selector = Op->Src[2].Literal();

  Ref Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);

  if (Selector == 0) {
    Ref Result = Is128Bit ? _VMov(OpSize::i128Bit, Src1) : Src1;
    StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
    return;
  }
  if (Selector == 0xFF) {
    Ref Result = Is128Bit ? _VMov(OpSize::i128Bit, Src2) : Src2;
    StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
    return;
  }

  // 256-bit VPBLENDW acts as if the 8-bit selector values were also applied
  // to the upper bits, so we can just replicate the bits by forming a 16-bit
  // imm for the helper function to use.
  const auto NewSelector = Selector << 8 | Selector;

  const auto ZeroRegister = LoadZeroVector(DstSize);
  Ref Result = VBLENDOpImpl(DstSize, OpSize::i16Bit, Src1, Src2, ZeroRegister, NewSelector);
  if (Is128Bit) {
    Result = _VMov(OpSize::i128Bit, Result);
  }
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::VZEROOp(OpcodeArgs) {
  const auto DstSize = GetDstSize(Op);
  const auto IsVZEROALL = DstSize == Core::CPUState::XMM_AVX_REG_SIZE;
  const auto NumRegs = CTX->Config.Is64BitMode ? 16U : 8U;

  if (IsVZEROALL) {
    // NOTE: Despite the name being VZEROALL, this will still only ever
    //       zero out up to the first 16 registers (even on AVX-512, where we have 32 registers)

    for (uint32_t i = 0; i < NumRegs; i++) {
      // Explicitly not caching named vector zero. This ensures that every register gets movi #0.0 directly.
      Ref ZeroVector = LoadUncachedZeroVector(DstSize);
      StoreXMMRegister(i, ZeroVector);
    }
  } else {
    // Likewise, VZEROUPPER will only ever zero only up to the first 16 registers

    for (uint32_t i = 0; i < NumRegs; i++) {
      Ref Reg = LoadXMMRegister(i);
      Ref Dst = _VMov(OpSize::i128Bit, Reg);
      StoreXMMRegister(i, Dst);
    }
  }
}

void OpDispatchBuilder::VPERMILImmOp(OpcodeArgs, size_t ElementSize) {
  const auto DstSize = GetDstSize(Op);
  const auto Is256Bit = DstSize == Core::CPUState::XMM_AVX_REG_SIZE;
  const auto Selector = Op->Src[1].Literal() & 0xFF;

  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Result = LoadZeroVector(DstSize);

  if (ElementSize == 8) {
    Result = _VInsElement(DstSize, ElementSize, 0, Selector & 0b0001, Result, Src);
    Result = _VInsElement(DstSize, ElementSize, 1, (Selector & 0b0010) >> 1, Result, Src);

    if (Is256Bit) {
      Result = _VInsElement(DstSize, ElementSize, 2, ((Selector & 0b0100) >> 2) + 2, Result, Src);
      Result = _VInsElement(DstSize, ElementSize, 3, ((Selector & 0b1000) >> 3) + 2, Result, Src);
    }
  } else {
    Result = _VInsElement(DstSize, ElementSize, 0, Selector & 0b00000011, Result, Src);
    Result = _VInsElement(DstSize, ElementSize, 1, (Selector & 0b00001100) >> 2, Result, Src);
    Result = _VInsElement(DstSize, ElementSize, 2, (Selector & 0b00110000) >> 4, Result, Src);
    Result = _VInsElement(DstSize, ElementSize, 3, (Selector & 0b11000000) >> 6, Result, Src);

    if (Is256Bit) {
      Result = _VInsElement(DstSize, ElementSize, 4, (Selector & 0b00000011) + 4, Result, Src);
      Result = _VInsElement(DstSize, ElementSize, 5, ((Selector & 0b00001100) >> 2) + 4, Result, Src);
      Result = _VInsElement(DstSize, ElementSize, 6, ((Selector & 0b00110000) >> 4) + 4, Result, Src);
      Result = _VInsElement(DstSize, ElementSize, 7, ((Selector & 0b11000000) >> 6) + 4, Result, Src);
    }
  }

  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

Ref OpDispatchBuilder::VPERMILRegOpImpl(OpSize DstSize, size_t ElementSize, Ref Src, Ref Indices) {
  // NOTE: See implementation of VPERMD for the gist of what we do to make this work.
  //
  //       The only difference here is that we need to add 16 to the upper lane
  //       before doing the final addition to build up the indices for TBL.

  const auto Is256Bit = DstSize == Core::CPUState::XMM_AVX_REG_SIZE;
  auto IsPD = ElementSize == OpSize::i64Bit;

  if (IsPD) {
    // VPERMILPD stores the selector in the second bit, rather than the
    // first bit of each element in the index vector. So move it over by one.
    Indices = _VUShrI(DstSize, ElementSize, Indices, 1);
  }

  // Sanitize indices first
  const auto ShiftAmount = 0b11 >> static_cast<uint32_t>(IsPD);
  Ref IndexMask = _VectorImm(DstSize, ElementSize, ShiftAmount);
  Ref SanitizedIndices = _VAnd(DstSize, OpSize::i8Bit, Indices, IndexMask);

  Ref IndexTrn1 = _VTrn(DstSize, OpSize::i8Bit, SanitizedIndices, SanitizedIndices);
  Ref IndexTrn2 = _VTrn(DstSize, OpSize::i16Bit, IndexTrn1, IndexTrn1);
  Ref IndexTrn3 = IndexTrn2;
  if (IsPD) {
    IndexTrn3 = _VTrn(DstSize, OpSize::i32Bit, IndexTrn2, IndexTrn2);
  }

  auto IndexShift = IsPD ? 3 : 2;
  Ref ShiftedIndices = _VShlI(DstSize, OpSize::i8Bit, IndexTrn3, IndexShift);

  uint64_t VConstant = IsPD ? 0x0706050403020100 : 0x03020100;
  Ref VectorConst = _VDupFromGPR(DstSize, ElementSize, _Constant(VConstant));
  Ref FinalIndices {};

  if (Is256Bit) {
    const auto ZeroRegister = LoadZeroVector(DstSize);
    Ref Vector16 = _VInsElement(DstSize, OpSize::i128Bit, 1, 0, ZeroRegister, _VectorImm(DstSize, 1, 16));
    Ref IndexOffsets = _VAdd(DstSize, OpSize::i8Bit, VectorConst, Vector16);

    FinalIndices = _VAdd(DstSize, OpSize::i8Bit, IndexOffsets, ShiftedIndices);
  } else {
    FinalIndices = _VAdd(DstSize, OpSize::i8Bit, VectorConst, ShiftedIndices);
  }

  return _VTBL1(DstSize, Src, FinalIndices);
}

template<size_t ElementSize>
void OpDispatchBuilder::VPERMILRegOp(OpcodeArgs) {
  Ref Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Indices = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);

  Ref Result = VPERMILRegOpImpl(OpSizeFromDst(Op), ElementSize, Src, Indices);
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

template void OpDispatchBuilder::VPERMILRegOp<OpSize::i32Bit>(OpcodeArgs);
template void OpDispatchBuilder::VPERMILRegOp<OpSize::i64Bit>(OpcodeArgs);

void OpDispatchBuilder::PCMPXSTRXOpImpl(OpcodeArgs, bool IsExplicit, bool IsMask) {
  const uint16_t Control = Op->Src[1].Literal();

  // SSE4.2 string instructions modify flags, so invalidate
  // any previously deferred flags.
  InvalidateDeferredFlags();

  // NOTE: Unlike most other SSE/AVX instructions, the SSE4.2 string and text
  //       instructions do *not* require memory operands to be aligned on a 16 byte
  //       boundary (see "Other Exceptions" descriptions for the relevant
  //       instructions in the Intel Software Development Manual).
  //
  //       So, we specify Src2 as having an alignment of 1 to indicate this.
  Ref Src1 = LoadSource_WithOpSize(FPRClass, Op, Op->Dest, OpSize::i128Bit, Op->Flags);
  Ref Src2 = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], OpSize::i128Bit, Op->Flags, {.Align = OpSize::i8Bit});

  Ref IntermediateResult {};
  if (IsExplicit) {
    // Will be 4 in the absence of a REX.W bit and 8 in the presence of a REX.W bit.
    //
    // While the control bit immediate for the instruction itself is only ever 8 bits
    // in size, we use it as a 16-bit value so that we can use the 8th bit to signify
    // whether or not RAX and RDX should be interpreted as a 64-bit value.
    const auto SrcSize = GetSrcSize(Op);
    const auto Is64Bit = SrcSize == OpSize::i64Bit;
    const auto NewControl = uint16_t(Control | (uint16_t(Is64Bit) << 8));

    Ref SrcRAX = LoadGPRRegister(X86State::REG_RAX);
    Ref SrcRDX = LoadGPRRegister(X86State::REG_RDX);

    IntermediateResult = _VPCMPESTRX(Src1, Src2, SrcRAX, SrcRDX, NewControl);
  } else {
    IntermediateResult = _VPCMPISTRX(Src1, Src2, Control);
  }

  Ref ZeroConst = _Constant(0);

  if (IsMask) {
    // For the masked variant of the instructions, if control[6] is set, then we
    // need to expand the intermediate result into a byte or word mask (depending
    // on data size specified in control[1]) along the entire length of XMM0,
    // where set bits in the intermediate result set the corresponding entry
    // in XMM0 to all 1s and unset bits set the corresponding entry to all 0s.
    //
    // If control[6] is not set, then we just store the intermediate result as-is
    // into the least significant bits of XMM0 and zero extend it.
    const auto IsExpandedMask = (Control & 0b0100'0000) != 0;

    if (IsExpandedMask) {
      // We need to iterate over the intermediate result and
      // expand the mask into XMM0 elements.
      const auto ElementSize = 1U << (Control & 1);
      const auto NumElements = 16U >> (Control & 1);

      Ref Result = LoadZeroVector(Core::CPUState::XMM_SSE_REG_SIZE);
      for (uint32_t i = 0; i < NumElements; i++) {
        Ref SignBit = _Sbfe(OpSize::i64Bit, 1, i, IntermediateResult);
        Result = _VInsGPR(Core::CPUState::XMM_SSE_REG_SIZE, ElementSize, i, Result, SignBit);
      }
      StoreXMMRegister(0, Result);
    } else {
      // We insert the intermediate result as-is.
      StoreXMMRegister(0, _VCastFromGPR(OpSize::i128Bit, OpSize::i16Bit, IntermediateResult));
    }
  } else {
    // For the indexed variant of the instructions, if control[6] is set, then we
    // store the index of the most significant bit into ECX. If it's not set,
    // then we store the least significant bit.
    const auto UseMSBIndex = (Control & 0b0100'0000) != 0;

    Ref ResultNoFlags = _Bfe(OpSize::i32Bit, 16, 0, IntermediateResult);

    Ref IfZero = _Constant(16 >> (Control & 1));
    Ref IfNotZero = UseMSBIndex ? _FindMSB(IR::OpSize::i32Bit, ResultNoFlags) : _FindLSB(IR::OpSize::i32Bit, ResultNoFlags);
    Ref Result = _Select(IR::COND_EQ, ResultNoFlags, ZeroConst, IfZero, IfNotZero);

    // Store the result, it is already zero-extended to 64-bit implicitly.
    StoreGPRRegister(X86State::REG_RCX, Result);
  }

  // Set all of the necessary flags. NZCV stored in bits 28...31 like the hw op.
  SetNZCV(IntermediateResult);
  CFInverted = false;
  PossiblySetNZCVBits = ~0;
  ZeroPF_AF();
}

void OpDispatchBuilder::VPCMPESTRIOp(OpcodeArgs) {
  PCMPXSTRXOpImpl(Op, true, false);
}
void OpDispatchBuilder::VPCMPESTRMOp(OpcodeArgs) {
  PCMPXSTRXOpImpl(Op, true, true);
}
void OpDispatchBuilder::VPCMPISTRIOp(OpcodeArgs) {
  PCMPXSTRXOpImpl(Op, false, false);
}
void OpDispatchBuilder::VPCMPISTRMOp(OpcodeArgs) {
  PCMPXSTRXOpImpl(Op, false, true);
}

void OpDispatchBuilder::VFMAImpl(OpcodeArgs, IROps IROp, bool Scalar, uint8_t Src1Idx, uint8_t Src2Idx, uint8_t AddendIdx) {
  const auto Size = GetDstSize(Op);
  const auto Is256Bit = Size == Core::CPUState::XMM_AVX_REG_SIZE;

  const OpSize ElementSize = Op->Flags & X86Tables::DecodeFlags::FLAG_OPTION_AVX_W ? OpSize::i64Bit : OpSize::i32Bit;

  Ref Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  Ref Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);

  Ref Sources[3] = {
    Dest,
    Src1,
    Src2,
  };

  DeriveOp(FMAResult, IROp, _VFMLA(Size, ElementSize, Sources[Src1Idx - 1], Sources[Src2Idx - 1], Sources[AddendIdx - 1]));
  Ref Result = FMAResult;
  if (Scalar) {
    // Special case, scalar inserts in to the low bits of the destination.
    Result = _VInsElement(OpSize::i128Bit, ElementSize, 0, 0, Dest, Result);
  }

  if (!Is256Bit) {
    Result = _VMov(OpSize::i128Bit, Result);
  }
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

void OpDispatchBuilder::VFMAddSubImpl(OpcodeArgs, bool AddSub, uint8_t Src1Idx, uint8_t Src2Idx, uint8_t AddendIdx) {
  const auto Size = GetDstSize(Op);
  const auto Is256Bit = Size == Core::CPUState::XMM_AVX_REG_SIZE;

  const OpSize ElementSize = Op->Flags & X86Tables::DecodeFlags::FLAG_OPTION_AVX_W ? OpSize::i64Bit : OpSize::i32Bit;

  Ref Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  Ref Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  Ref Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);

  Ref Sources[3] = {
    Dest,
    Src1,
    Src2,
  };

  Ref ConstantEOR {};
  if (AddSub) {
    ConstantEOR =
      LoadAndCacheNamedVectorConstant(Size, ElementSize == OpSize::i32Bit ? NAMED_VECTOR_PADDSUBPS_INVERT : NAMED_VECTOR_PADDSUBPD_INVERT);
  } else {
    ConstantEOR =
      LoadAndCacheNamedVectorConstant(Size, ElementSize == OpSize::i32Bit ? NAMED_VECTOR_PSUBADDPS_INVERT : NAMED_VECTOR_PSUBADDPD_INVERT);
  }

  auto InvertedSourc = _VXor(Size, ElementSize, Sources[AddendIdx - 1], ConstantEOR);

  Ref Result = _VFMLA(Size, ElementSize, Sources[Src1Idx - 1], Sources[Src2Idx - 1], InvertedSourc);
  if (!Is256Bit) {
    Result = _VMov(OpSize::i128Bit, Result);
  }
  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);
}

OpDispatchBuilder::RefVSIB OpDispatchBuilder::LoadVSIB(const X86Tables::DecodedOp& Op, const X86Tables::DecodedOperand& Operand, uint32_t Flags) {
  [[maybe_unused]] const bool IsVSIB = (Op->Flags & X86Tables::DecodeFlags::FLAG_VSIB_BYTE) != 0;
  LOGMAN_THROW_A_FMT(Operand.IsSIB() && IsVSIB, "Trying to load VSIB for something that isn't the correct type!");

  // VSIB is a very special case which has a ton of encoded data.
  // Get it in a format we can reason about.

  const auto Index_gpr = Operand.Data.SIB.Index;
  const auto Base_gpr = Operand.Data.SIB.Base;
  LOGMAN_THROW_AA_FMT(Index_gpr >= FEXCore::X86State::REG_XMM_0 && Index_gpr <= FEXCore::X86State::REG_XMM_15, "must be AVX reg");
  LOGMAN_THROW_AA_FMT(
    Base_gpr == FEXCore::X86State::REG_INVALID || (Base_gpr >= FEXCore::X86State::REG_RAX && Base_gpr <= FEXCore::X86State::REG_R15),
    "Base must be a GPR.");
  const auto Index_XMM_gpr = Index_gpr - X86State::REG_XMM_0;

  return {
    .Low = LoadXMMRegister(Index_XMM_gpr),
    .BaseAddr = Base_gpr != FEXCore::X86State::REG_INVALID ? LoadGPRRegister(Base_gpr, OpSize::i64Bit, 0, false) : nullptr,
    .Displacement = Operand.Data.SIB.Offset,
    .Scale = Operand.Data.SIB.Scale,
  };
}

template<OpSize AddrElementSize>
void OpDispatchBuilder::VPGATHER(OpcodeArgs) {
  LOGMAN_THROW_A_FMT(AddrElementSize == OpSize::i32Bit || AddrElementSize == OpSize::i64Bit, "Unknown address element size");

  const auto Size = OpSizeFromDst(Op);
  const auto Is128Bit = Size == Core::CPUState::XMM_SSE_REG_SIZE;

  ///< Element size is determined by W flag.
  const OpSize ElementLoadSize = Op->Flags & X86Tables::DecodeFlags::FLAG_OPTION_AVX_W ? OpSize::i64Bit : OpSize::i32Bit;

  // We only need the high address register if the number of data elements is more than what the low half can consume.
  // But also the number of address elements is clamped by the destination size as well.
  const size_t NumDataElements = Size / ElementLoadSize;
  const size_t NumAddrElementBytes = std::min<size_t>(Size, (NumDataElements * AddrElementSize));
  const bool Needs128BitHighAddrBytes = NumAddrElementBytes > OpSize::i128Bit;

  auto VSIB = LoadVSIB(Op, Op->Src[0], Op->Flags);

  const bool SupportsSVELoad = (VSIB.Scale == 1 || VSIB.Scale == AddrElementSize) && (AddrElementSize == ElementLoadSize);

  Ref Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  Ref Mask = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);

  Ref Result {};
  if (!SupportsSVELoad) {
    // We need to go down the fallback path in the case that we don't hit the backend's SVE mode.
    RefPair Dest128 {
      .Low = Dest,
      .High = _VDupElement(OpSize::i256Bit, OpSize::i128Bit, Dest, 1),
    };

    RefPair Mask128 {
      .Low = Mask,
      .High = _VDupElement(OpSize::i256Bit, OpSize::i128Bit, Mask, 1),
    };

    RefVSIB VSIB128 = VSIB;
    VSIB128.High = Invalid();

    if (Needs128BitHighAddrBytes) {
      if (Is128Bit) {
        ///< A bit careful for the VSIB index register duplicating.
        VSIB128.High = VSIB128.Low;
      } else {
        VSIB128.High = _VDupElement(OpSize::i256Bit, OpSize::i128Bit, VSIB128.Low, 1);
      }
    }

    auto Result128 = AVX128_VPGatherImpl(SizeToOpSize(Size), ElementLoadSize, AddrElementSize, Dest128, Mask128, VSIB128);
    // The registers are current split, need to merge them.
    Result = _VInsElement(OpSize::i256Bit, OpSize::i128Bit, 1, 0, Result128.Low, Result128.High);
  } else {
    ///< Calculate the full operation.
    ///< BaseAddr doesn't need to exist, calculate that here.
    Ref BaseAddr = VSIB.BaseAddr;
    if (BaseAddr && VSIB.Displacement) {
      BaseAddr = _Add(OpSize::i64Bit, BaseAddr, _Constant(VSIB.Displacement));
    } else if (VSIB.Displacement) {
      BaseAddr = _Constant(VSIB.Displacement);
    } else if (!BaseAddr) {
      BaseAddr = Invalid();
    }

    Result = _VLoadVectorGatherMasked(Size, ElementLoadSize, Dest, Mask, BaseAddr, VSIB.Low, Invalid(), AddrElementSize, VSIB.Scale, 0, 0);
  }

  if (Is128Bit) {
    if (AddrElementSize == OpSize::i64Bit && ElementLoadSize == OpSize::i32Bit) {
      // Special case for the 128-bit gather load using 64-bit address indexes with 32-bit results.
      // Only loads two 32-bit elements in to the lower 64-bits of the first destination.
      // Bits [255:65] all become zero.
      Result = _VMov(OpSize::i64Bit, Result);
    } else if (Is128Bit) {
      Result = _VMov(OpSize::i128Bit, Result);
    }
  } else {
    if (AddrElementSize == OpSize::i64Bit && ElementLoadSize == OpSize::i32Bit) {
      // If we only fetched 128-bits worth of data then the upper-result is all zero.
      Result = _VMov(OpSize::i128Bit, Result);
    }
  }

  StoreResult(FPRClass, Op, Result, OpSize::iInvalid);

  ///< Assume non-faulting behaviour and clear the mask register.
  auto Zero = LoadZeroVector(Size);
  StoreResult_WithOpSize(FPRClass, Op, Op->Src[1], Zero, Size, OpSize::iInvalid);
}

template void OpDispatchBuilder::VPGATHER<OpSize::i32Bit>(OpcodeArgs);
template void OpDispatchBuilder::VPGATHER<OpSize::i64Bit>(OpcodeArgs);

} // namespace FEXCore::IR
