/*
$info$
tags: frontend|x86-to-ir, opcodes|dispatcher-implementations
desc: Handles x86/64 Vector instructions to IR
$end_info$
*/

#include "Interface/Context/Context.h"
#include "Interface/Core/OpcodeDispatcher.h"

#include <FEXCore/Config/Config.h>
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/Debug/X86Tables.h>
#include <FEXCore/IR/IR.h>
#include <FEXCore/Utils/LogManager.h>

#include <array>
#include <bit>
#include <cstdint>
#include <stddef.h>

namespace FEXCore::IR {
#define OpcodeArgs [[maybe_unused]] FEXCore::X86Tables::DecodedOp Op

void OpDispatchBuilder::MOVVectorOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, 1);
  StoreResult(FPRClass, Op, Src, 1);
}

void OpDispatchBuilder::MOVVectorNTOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, 1, true, false, MemoryAccessType::ACCESS_STREAM);
  StoreResult(FPRClass, Op, Src, 1, MemoryAccessType::ACCESS_STREAM);
}

void OpDispatchBuilder::VMOVVectorNTOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, 1, true, false, MemoryAccessType::ACCESS_STREAM);
  const auto Is128BitDest = GetDstSize(Op) == Core::CPUState::XMM_SSE_REG_SIZE;

  if (Op->Dest.IsGPR() && Is128BitDest) {
    // Clear the upper lane
    Src = _VMov(16, Src);
  }

  StoreResult(FPRClass, Op, Src, 1, MemoryAccessType::ACCESS_STREAM);
}

void OpDispatchBuilder::MOVAPSOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  StoreResult(FPRClass, Op, Src, -1);
}

void OpDispatchBuilder::VMOVAPS_VMOVAPD_Op(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  const auto Is128BitDest = GetDstSize(Op) == Core::CPUState::XMM_SSE_REG_SIZE;

  if (Op->Dest.IsGPR() && Is128BitDest) {
    // Clear the upper lane
    Src = _VMov(16, Src);
  }

  StoreResult(FPRClass, Op, Src, -1);
}

void OpDispatchBuilder::VMOVUPS_VMOVUPD_Op(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, 1);
  const auto Is128BitDest = GetDstSize(Op) == Core::CPUState::XMM_SSE_REG_SIZE;

  if (Op->Dest.IsGPR() && Is128BitDest) {
    // Clear the upper lane
    Src = _VMov(16, Src);
  }

  StoreResult(FPRClass, Op, Src, 1);
}

void OpDispatchBuilder::MOVUPSOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, 1);
  StoreResult(FPRClass, Op, Src, 1);
}

void OpDispatchBuilder::MOVLHPSOp(OpcodeArgs) {
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, 8);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, 8);
  auto Result = _VInsElement(16, 8, 1, 0, Dest, Src);
  StoreResult(FPRClass, Op, Result, 8);
}

void OpDispatchBuilder::MOVHPDOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  // This instruction is a bit special that if the destination is a register then it'll ZEXT the 64bit source to 128bit
  if (Op->Dest.IsGPR()) {
    // If the destination is a GPR then the source is memory
    // xmm1[127:64] = src
    OrderedNode *Dest = LoadSource_WithOpSize(FPRClass, Op, Op->Dest, 16, Op->Flags, -1);
    auto Result = _VInsElement(16, 8, 1, 0, Dest, Src);
    StoreResult(FPRClass, Op, Result, -1);
  }
  else {
    // In this case memory is the destination and the high bits of the XMM are source
    // Mem64 = xmm1[127:64]
    auto Result = _VExtractToGPR(16, 8, Src, 1);
    StoreResult(GPRClass, Op, Result, -1);
  }
}

void OpDispatchBuilder::VMOVHPOp(OpcodeArgs) {
  if (Op->Dest.IsGPR()) {
    OrderedNode *Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, 16);
    OrderedNode *Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags, 8);
    OrderedNode *Result = _VInsElement(16, 8, 1, 0, Src1, Src2);

    // Clear the upper lane.
    Result = _VMov(16, Result);

    StoreResult(FPRClass, Op, Result, -1);
  } else {
    OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, 16);
    OrderedNode *Result = _VInsElement(16, 8, 0, 1, Src, Src);
    StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Result, 8, 8);
  }
}

void OpDispatchBuilder::MOVLPOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, 8);
  if (Op->Dest.IsGPR()) {
    // xmm, xmm is movhlps special case
    if (Op->Src[0].IsGPR()) {
      OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, 8, 16);
      auto Result = _VInsElement(16, 8, 0, 1, Dest, Src);
      StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Result, 16, 16);
    }
    else {
      auto DstSize = GetDstSize(Op);
      OrderedNode *Dest = LoadSource_WithOpSize(FPRClass, Op, Op->Dest, DstSize, Op->Flags, -1);
      auto Result = _VInsElement(16, 8, 0, 0, Dest, Src);
      StoreResult(FPRClass, Op, Result, -1);
    }
  }
  else {
    StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Src, 8, 8);
  }
}

void OpDispatchBuilder::VMOVLPOp(OpcodeArgs) {
  if (Op->Dest.IsGPR()) {
    OrderedNode *Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, 16);
    OrderedNode *Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags, 8);
    OrderedNode *Result = _VInsElement(16, 8, 0, 0, Src1, Src2);

    // Clear the upper lane.
    Result = _VMov(16, Result);

    StoreResult(FPRClass, Op, Result, -1);
  } else {
    OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, 8);
    StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Src, 8, 8);
  }
}

void OpDispatchBuilder::MOVSHDUPOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, 8);
  OrderedNode *Result = _VInsElement(16, 4, 2, 3, Src, Src);
  Result = _VInsElement(16, 4, 0, 1, Result, Src);
  StoreResult(FPRClass, Op, Result, -1);
}

void OpDispatchBuilder::VMOVSHDUPOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  const auto SrcSize = GetSrcSize(Op);
  const auto Is256Bit = SrcSize == Core::CPUState::XMM_AVX_REG_SIZE;

  OrderedNode *Result = _VInsElement(SrcSize, 4, 2, 3, Src, Src);
  Result = _VInsElement(SrcSize, 4, 0, 1, Result, Src);
  if (Is256Bit) {
    Result = _VInsElement(SrcSize, 4, 4, 5, Result, Src);
    Result = _VInsElement(SrcSize, 4, 6, 7, Result, Src);
  } else {
    // Clear upper lane
    Result = _VMov(16, Result);
  }

  StoreResult(FPRClass, Op, Result, -1);
}

void OpDispatchBuilder::MOVSLDUPOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, 8);
  OrderedNode *Result = _VInsElement(16, 4, 3, 2, Src, Src);
  Result = _VInsElement(16, 4, 1, 0, Result, Src);
  StoreResult(FPRClass, Op, Result, -1);
}

void OpDispatchBuilder::VMOVSLDUPOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  const auto SrcSize = GetSrcSize(Op);
  const auto Is256Bit = SrcSize == Core::CPUState::XMM_AVX_REG_SIZE;

  OrderedNode *Result = _VInsElement(SrcSize, 4, 3, 2, Src, Src);
  Result = _VInsElement(SrcSize, 4, 1, 0, Result, Src);
  if (Is256Bit) {
    Result = _VInsElement(SrcSize, 4, 5, 4, Result, Src);
    Result = _VInsElement(SrcSize, 4, 7, 6, Result, Src);
  } else {
    // Clear upper lane
    Result = _VMov(16, Result);
  }

  StoreResult(FPRClass, Op, Result, -1);
}

void OpDispatchBuilder::MOVSSOp(OpcodeArgs) {
  if (Op->Dest.IsGPR() && Op->Src[0].IsGPR()) {
    // MOVSS xmm1, xmm2
    OrderedNode *Dest = LoadSource_WithOpSize(FPRClass, Op, Op->Dest, 16, Op->Flags, -1);
    OrderedNode *Src = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], 4, Op->Flags, -1);
    auto Result = _VInsElement(16, 4, 0, 0, Dest, Src);
    StoreResult(FPRClass, Op, Result, -1);
  }
  else if (Op->Dest.IsGPR()) {
    // MOVSS xmm1, mem32
    // xmm1[127:0] <- zext(mem32)
    OrderedNode *Src = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], 4, Op->Flags, -1);
    StoreResult(FPRClass, Op, Src, -1);
  }
  else {
    // MOVSS mem32, xmm1
    OrderedNode *Src = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], 4, Op->Flags, -1);
    StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Src, 4, -1);
  }
}

void OpDispatchBuilder::MOVSDOp(OpcodeArgs) {
  if (Op->Dest.IsGPR() && Op->Src[0].IsGPR()) {
    // xmm1[63:0] <- xmm2[63:0]
    OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
    OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
    auto Result = _VInsElement(16, 8, 0, 0, Dest, Src);
    StoreResult(FPRClass, Op, Result, -1);
  }
  else if (Op->Dest.IsGPR()) {
    // xmm1[127:0] <- zext(mem64)
    OrderedNode *Src = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], 8, Op->Flags, -1);
    StoreResult(FPRClass, Op, Src, -1);
  }
  else {
    // In this case memory is the destination and the low bits of the XMM are source
    // Mem64 = xmm2[63:0]
    OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
    StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Src, 8, -1);
  }
}

void OpDispatchBuilder::VMOVScalarOpImpl(OpcodeArgs, size_t ElementSize) {
  if (Op->Dest.IsGPR() && Op->Src[0].IsGPR() && Op->Src[1].IsGPR()) {
    // VMOVSS/SD xmm1, xmm2, xmm3
    OrderedNode *Src1 = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], 16, Op->Flags, -1);
    OrderedNode *Src2 = LoadSource_WithOpSize(FPRClass, Op, Op->Src[1], ElementSize, Op->Flags, -1);
    OrderedNode *Result = _VInsElement(16, ElementSize, 0, 0, Src1, Src2);
    StoreResult(FPRClass, Op, Result, -1);
  } else if (Op->Dest.IsGPR()) {
    // VMOVSS/SD xmm1, mem32/mem64
    OrderedNode *Src = LoadSource_WithOpSize(FPRClass, Op, Op->Src[1], ElementSize, Op->Flags, -1);
    StoreResult(FPRClass, Op, Src, -1);
  } else {
    // VMOVSS/SD mem32/mem64, xmm1
    OrderedNode *Src = LoadSource_WithOpSize(FPRClass, Op, Op->Src[1], ElementSize, Op->Flags, -1);
    StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Src, ElementSize, -1);
  }
}

void OpDispatchBuilder::VMOVSDOp(OpcodeArgs) {
  VMOVScalarOpImpl(Op, 8);
}

void OpDispatchBuilder::VMOVSSOp(OpcodeArgs) {
  VMOVScalarOpImpl(Op, 4);
}

void OpDispatchBuilder::VectorALUOpImpl(OpcodeArgs, IROps IROp, size_t ElementSize) {
  const auto Size = GetSrcSize(Op);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);

  auto ALUOp = _VAdd(Size, ElementSize, Dest, Src);
  // Overwrite our IR's op type
  ALUOp.first->Header.Op = IROp;

  StoreResult(FPRClass, Op, ALUOp, -1);
}

template <IROps IROp, size_t ElementSize>
void OpDispatchBuilder::VectorALUOp(OpcodeArgs) {
  VectorALUOpImpl(Op, IROp, ElementSize);
}

template
void OpDispatchBuilder::VectorALUOp<IR::OP_VAND, 8>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUOp<IR::OP_VAND, 16>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUOp<IR::OP_VOR, 8>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUOp<IR::OP_VOR, 16>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUOp<IR::OP_VXOR, 8>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUOp<IR::OP_VXOR, 16>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUOp<IR::OP_VADD, 1>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUOp<IR::OP_VADD, 2>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUOp<IR::OP_VADD, 4>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUOp<IR::OP_VADD, 8>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUOp<IR::OP_VADDP, 2>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUOp<IR::OP_VADDP, 4>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUOp<IR::OP_VSUB, 1>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUOp<IR::OP_VSUB, 2>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUOp<IR::OP_VSUB, 4>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUOp<IR::OP_VSUB, 8>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUOp<IR::OP_VFADD, 4>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUOp<IR::OP_VFADD, 8>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUOp<IR::OP_VFADDP, 4>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUOp<IR::OP_VFADDP, 8>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUOp<IR::OP_VFSUB, 4>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUOp<IR::OP_VFSUB, 8>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUOp<IR::OP_VFMUL, 4>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUOp<IR::OP_VFMUL, 8>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUOp<IR::OP_VFDIV, 4>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUOp<IR::OP_VFDIV, 8>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUOp<IR::OP_VFMIN, 4>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUOp<IR::OP_VFMIN, 8>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUOp<IR::OP_VFMAX, 4>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUOp<IR::OP_VFMAX, 8>(OpcodeArgs);

template
void OpDispatchBuilder::VectorALUOp<IR::OP_VCMPGT, 1>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUOp<IR::OP_VCMPGT, 2>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUOp<IR::OP_VCMPGT, 4>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUOp<IR::OP_VCMPGT, 8>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUOp<IR::OP_VCMPEQ, 1>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUOp<IR::OP_VCMPEQ, 2>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUOp<IR::OP_VCMPEQ, 4>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUOp<IR::OP_VCMPEQ, 8>(OpcodeArgs);

template
void OpDispatchBuilder::VectorALUOp<IR::OP_VSMUL, 2>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUOp<IR::OP_VSMUL, 4>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUOp<IR::OP_VSMIN, 1>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUOp<IR::OP_VSMIN, 2>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUOp<IR::OP_VSMIN, 4>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUOp<IR::OP_VSMAX, 1>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUOp<IR::OP_VSMAX, 2>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUOp<IR::OP_VSMAX, 4>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUOp<IR::OP_VSQADD, 1>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUOp<IR::OP_VSQADD, 2>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUOp<IR::OP_VSQSUB, 1>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUOp<IR::OP_VSQSUB, 2>(OpcodeArgs);

template
void OpDispatchBuilder::VectorALUOp<IR::OP_VUMIN, 1>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUOp<IR::OP_VUMIN, 2>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUOp<IR::OP_VUMIN, 4>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUOp<IR::OP_VUMAX, 1>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUOp<IR::OP_VUMAX, 2>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUOp<IR::OP_VUMAX, 4>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUOp<IR::OP_VURAVG, 1>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUOp<IR::OP_VURAVG, 2>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUOp<IR::OP_VUQADD, 1>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUOp<IR::OP_VUQADD, 2>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUOp<IR::OP_VUQSUB, 1>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUOp<IR::OP_VUQSUB, 2>(OpcodeArgs);

void OpDispatchBuilder::AVXVectorALUOpImpl(OpcodeArgs, IROps IROp, size_t ElementSize) {
  const auto Size = GetSrcSize(Op);
  const auto Is128Bit = Size == Core::CPUState::XMM_SSE_REG_SIZE;

  OrderedNode *Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags, -1);

  auto ALUOp = _VAdd(Size, ElementSize, Src1, Src2);
  // Overwrite our IR's op type
  ALUOp.first->Header.Op = IROp;

  OrderedNode* Result = ALUOp;
  if (Is128Bit) {
    // 128-bit variants need to zero the upper lane.
    Result = _VMov(Size, ALUOp);
  }

  StoreResult(FPRClass, Op, Result, -1);
}

template <IROps IROp, size_t ElementSize>
void OpDispatchBuilder::AVXVectorALUOp(OpcodeArgs) {
  AVXVectorALUOpImpl(Op, IROp, ElementSize);
}

template
void OpDispatchBuilder::AVXVectorALUOp<IR::OP_VADD, 1>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorALUOp<IR::OP_VADD, 2>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorALUOp<IR::OP_VADD, 4>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorALUOp<IR::OP_VADD, 8>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorALUOp<IR::OP_VSQADD, 1>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorALUOp<IR::OP_VSQADD, 2>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorALUOp<IR::OP_VUQADD, 1>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorALUOp<IR::OP_VUQADD, 2>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorALUOp<IR::OP_VSUB, 1>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorALUOp<IR::OP_VSUB, 2>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorALUOp<IR::OP_VSUB, 4>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorALUOp<IR::OP_VSUB, 8>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorALUOp<IR::OP_VSQSUB, 1>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorALUOp<IR::OP_VSQSUB, 2>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorALUOp<IR::OP_VUQSUB, 1>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorALUOp<IR::OP_VUQSUB, 2>(OpcodeArgs);

template
void OpDispatchBuilder::AVXVectorALUOp<IR::OP_VCMPEQ, 1>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorALUOp<IR::OP_VCMPEQ, 2>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorALUOp<IR::OP_VCMPEQ, 4>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorALUOp<IR::OP_VCMPEQ, 8>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorALUOp<IR::OP_VCMPGT, 1>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorALUOp<IR::OP_VCMPGT, 2>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorALUOp<IR::OP_VCMPGT, 4>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorALUOp<IR::OP_VCMPGT, 8>(OpcodeArgs);

template
void OpDispatchBuilder::AVXVectorALUOp<IR::OP_VFADD, 4>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorALUOp<IR::OP_VFADD, 8>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorALUOp<IR::OP_VFDIV, 4>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorALUOp<IR::OP_VFDIV, 8>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorALUOp<IR::OP_VFMAX, 4>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorALUOp<IR::OP_VFMAX, 8>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorALUOp<IR::OP_VFMIN, 4>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorALUOp<IR::OP_VFMIN, 8>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorALUOp<IR::OP_VFMUL, 4>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorALUOp<IR::OP_VFMUL, 8>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorALUOp<IR::OP_VFSUB, 4>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorALUOp<IR::OP_VFSUB, 8>(OpcodeArgs);

template
void OpDispatchBuilder::AVXVectorALUOp<IR::OP_VAND, 16>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorALUOp<IR::OP_VOR, 16>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorALUOp<IR::OP_VXOR, 16>(OpcodeArgs);

template
void OpDispatchBuilder::AVXVectorALUOp<IR::OP_VURAVG, 1>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorALUOp<IR::OP_VURAVG, 2>(OpcodeArgs);

template
void OpDispatchBuilder::AVXVectorALUOp<IR::OP_VSMAX, 1>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorALUOp<IR::OP_VSMAX, 2>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorALUOp<IR::OP_VSMAX, 4>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorALUOp<IR::OP_VUMAX, 1>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorALUOp<IR::OP_VUMAX, 2>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorALUOp<IR::OP_VUMAX, 4>(OpcodeArgs);

template
void OpDispatchBuilder::AVXVectorALUOp<IR::OP_VSMIN, 1>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorALUOp<IR::OP_VSMIN, 2>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorALUOp<IR::OP_VSMIN, 4>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorALUOp<IR::OP_VUMIN, 1>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorALUOp<IR::OP_VUMIN, 2>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorALUOp<IR::OP_VUMIN, 4>(OpcodeArgs);

template
void OpDispatchBuilder::AVXVectorALUOp<IR::OP_VSMUL, 2>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorALUOp<IR::OP_VSMUL, 4>(OpcodeArgs);

void OpDispatchBuilder::VectorALUROpImpl(OpcodeArgs, IROps IROp, size_t ElementSize) {
  const auto Size = GetSrcSize(Op);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);

  auto ALUOp = _VAdd(Size, ElementSize, Src, Dest);
  // Overwrite our IR's op type
  ALUOp.first->Header.Op = IROp;

  StoreResult(FPRClass, Op, ALUOp, -1);
}

template <IROps IROp, size_t ElementSize>
void OpDispatchBuilder::VectorALUROp(OpcodeArgs) {
  VectorALUROpImpl(Op, IROp, ElementSize);
}

template
void OpDispatchBuilder::VectorALUROp<IR::OP_VBIC, 8>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUROp<IR::OP_VFSUB, 4>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUROp<IR::OP_VFSUB, 8>(OpcodeArgs);

void OpDispatchBuilder::VectorScalarALUOpImpl(OpcodeArgs, IROps IROp, size_t ElementSize) {
  const auto DstSize = GetDstSize(Op);

  OrderedNode *Dest = LoadSource_WithOpSize(FPRClass, Op, Op->Dest, DstSize, Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  // If OpSize == ElementSize then it only does the lower scalar op
  auto ALUOp = _VAdd(ElementSize, ElementSize, Dest, Src);
  // Overwrite our IR's op type
  ALUOp.first->Header.Op = IROp;

  OrderedNode* Result = ALUOp;

  if (DstSize != ElementSize) {
    // Insert the lower bits
    Result = _VInsElement(DstSize, ElementSize, 0, 0, Dest, ALUOp);
  }

  StoreResult(FPRClass, Op, Result, -1);
}

template <IROps IROp, size_t ElementSize>
void OpDispatchBuilder::VectorScalarALUOp(OpcodeArgs) {
  VectorScalarALUOpImpl(Op, IROp, ElementSize);
}

template
void OpDispatchBuilder::VectorScalarALUOp<IR::OP_VFADD, 4>(OpcodeArgs);
template
void OpDispatchBuilder::VectorScalarALUOp<IR::OP_VFADD, 8>(OpcodeArgs);
template
void OpDispatchBuilder::VectorScalarALUOp<IR::OP_VFSUB, 4>(OpcodeArgs);
template
void OpDispatchBuilder::VectorScalarALUOp<IR::OP_VFSUB, 8>(OpcodeArgs);
template
void OpDispatchBuilder::VectorScalarALUOp<IR::OP_VFMUL, 4>(OpcodeArgs);
template
void OpDispatchBuilder::VectorScalarALUOp<IR::OP_VFMUL, 8>(OpcodeArgs);
template
void OpDispatchBuilder::VectorScalarALUOp<IR::OP_VFDIV, 4>(OpcodeArgs);
template
void OpDispatchBuilder::VectorScalarALUOp<IR::OP_VFDIV, 8>(OpcodeArgs);
template
void OpDispatchBuilder::VectorScalarALUOp<IR::OP_VFMIN, 4>(OpcodeArgs);
template
void OpDispatchBuilder::VectorScalarALUOp<IR::OP_VFMIN, 8>(OpcodeArgs);
template
void OpDispatchBuilder::VectorScalarALUOp<IR::OP_VFMAX, 4>(OpcodeArgs);
template
void OpDispatchBuilder::VectorScalarALUOp<IR::OP_VFMAX, 8>(OpcodeArgs);

void OpDispatchBuilder::AVXVectorScalarALUOpImpl(OpcodeArgs, IROps IROp, size_t ElementSize) {
  const auto DstSize = GetDstSize(Op);

  OrderedNode *Src1 = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], DstSize, Op->Flags, -1);
  OrderedNode *Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags, -1);

  // If OpSize == ElementSize then it only does the lower scalar op
  auto ALUOp = _VAdd(ElementSize, ElementSize, Src1, Src2);
  // Overwrite our IR's op type
  ALUOp.first->Header.Op = IROp;

  OrderedNode* Result = ALUOp;

  if (DstSize != ElementSize) {
    // Insert the lower bits
    Result = _VInsElement(DstSize, ElementSize, 0, 0, Src1, ALUOp);
  }

  // AVX scalar ops always clear the upper lane
  Result = _VMov(16, Result);

  StoreResult(FPRClass, Op, Result, -1);
}

template <IROps IROp, size_t ElementSize>
void OpDispatchBuilder::AVXVectorScalarALUOp(OpcodeArgs) {
  AVXVectorScalarALUOpImpl(Op, IROp, ElementSize);
}

template
void OpDispatchBuilder::AVXVectorScalarALUOp<IR::OP_VFADD, 4>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorScalarALUOp<IR::OP_VFADD, 8>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorScalarALUOp<IR::OP_VFDIV, 4>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorScalarALUOp<IR::OP_VFDIV, 8>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorScalarALUOp<IR::OP_VFMAX, 4>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorScalarALUOp<IR::OP_VFMAX, 8>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorScalarALUOp<IR::OP_VFMIN, 4>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorScalarALUOp<IR::OP_VFMIN, 8>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorScalarALUOp<IR::OP_VFMUL, 4>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorScalarALUOp<IR::OP_VFMUL, 8>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorScalarALUOp<IR::OP_VFSUB, 4>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorScalarALUOp<IR::OP_VFSUB, 8>(OpcodeArgs);

void OpDispatchBuilder::VectorUnaryOpImpl(OpcodeArgs, IROps IROp, size_t ElementSize, bool Scalar) {
  const auto Size = Scalar ? ElementSize : GetSrcSize(Op);
  const auto DstSize = GetDstSize(Op);

  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Dest = LoadSource_WithOpSize(FPRClass, Op, Op->Dest, DstSize, Op->Flags, -1);

  auto ALUOp = _VFSqrt(Size, ElementSize, Src);
  // Overwrite our IR's op type
  ALUOp.first->Header.Op = IROp;

  if (Scalar) {
    // Insert the lower bits
    auto Result = _VInsElement(DstSize, ElementSize, 0, 0, Dest, ALUOp);
    StoreResult(FPRClass, Op, Result, -1);
  } else {
    StoreResult(FPRClass, Op, ALUOp, -1);
  }
}

template <IROps IROp, size_t ElementSize, bool Scalar>
void OpDispatchBuilder::VectorUnaryOp(OpcodeArgs) {
  VectorUnaryOpImpl(Op, IROp, ElementSize, Scalar);
}

template
void OpDispatchBuilder::VectorUnaryOp<IR::OP_VFSQRT, 4, false>(OpcodeArgs);
template
void OpDispatchBuilder::VectorUnaryOp<IR::OP_VFRSQRT, 4, false>(OpcodeArgs);
template
void OpDispatchBuilder::VectorUnaryOp<IR::OP_VFRECP, 4, false>(OpcodeArgs);

template
void OpDispatchBuilder::VectorUnaryOp<IR::OP_VFSQRT, 4, true>(OpcodeArgs);
template
void OpDispatchBuilder::VectorUnaryOp<IR::OP_VFRSQRT, 4, true>(OpcodeArgs);
template
void OpDispatchBuilder::VectorUnaryOp<IR::OP_VFRECP, 4, true>(OpcodeArgs);

template
void OpDispatchBuilder::VectorUnaryOp<IR::OP_VFSQRT, 8, false>(OpcodeArgs);
template
void OpDispatchBuilder::VectorUnaryOp<IR::OP_VFSQRT, 8, true>(OpcodeArgs);

template
void OpDispatchBuilder::VectorUnaryOp<IR::OP_VABS, 1, false>(OpcodeArgs);
template
void OpDispatchBuilder::VectorUnaryOp<IR::OP_VABS, 2, false>(OpcodeArgs);
template
void OpDispatchBuilder::VectorUnaryOp<IR::OP_VABS, 4, false>(OpcodeArgs);

void OpDispatchBuilder::AVXVectorUnaryOpImpl(OpcodeArgs, IROps IROp, size_t ElementSize, bool Scalar) {
  const auto Size = Scalar ? ElementSize : GetSrcSize(Op);
  const auto DstSize = GetDstSize(Op);
  const auto Is128Bit = DstSize == Core::CPUState::XMM_SSE_REG_SIZE;

  OrderedNode *Src = [&] {
    const auto SrcIndex = Scalar ? 1 : 0;
    return LoadSource(FPRClass, Op, Op->Src[SrcIndex], Op->Flags, -1);
  }();
  OrderedNode *Dest = [&] {
    const auto& Operand = Scalar ? Op->Src[0] : Op->Dest;
    return LoadSource_WithOpSize(FPRClass, Op, Operand, DstSize, Op->Flags, -1);
  }();

  auto ALUOp = _VFSqrt(Size, ElementSize, Src);
  // Overwrite our IR's op type
  ALUOp.first->Header.Op = IROp;

  OrderedNode* Result = ALUOp;
  if (Scalar) {
    // Insert the lower bits
    Result = _VInsElement(DstSize, ElementSize, 0, 0, Dest, Result);
  }
  if (Is128Bit) {
    // Clear the upper lane.
    Result = _VMov(16, Result);
  }

  StoreResult(FPRClass, Op, Result, -1);
}

template <IROps IROp, size_t ElementSize, bool Scalar>
void OpDispatchBuilder::AVXVectorUnaryOp(OpcodeArgs) {
  AVXVectorUnaryOpImpl(Op, IROp, ElementSize, Scalar);
}

template
void OpDispatchBuilder::AVXVectorUnaryOp<IR::OP_VABS, 1, false>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorUnaryOp<IR::OP_VABS, 2, false>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorUnaryOp<IR::OP_VABS, 4, false>(OpcodeArgs);

template
void OpDispatchBuilder::AVXVectorUnaryOp<IR::OP_VFRECP, 4, false>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorUnaryOp<IR::OP_VFRECP, 4, true>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorUnaryOp<IR::OP_VFSQRT, 4, false>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorUnaryOp<IR::OP_VFSQRT, 4, true>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorUnaryOp<IR::OP_VFSQRT, 8, false>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorUnaryOp<IR::OP_VFSQRT, 8, true>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorUnaryOp<IR::OP_VFRSQRT, 4, false>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorUnaryOp<IR::OP_VFRSQRT, 4, true>(OpcodeArgs);

void OpDispatchBuilder::VectorUnaryDuplicateOpImpl(OpcodeArgs, IROps IROp, size_t ElementSize) {
  const auto Size = GetSrcSize(Op);

  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  auto ALUOp = _VFSqrt(ElementSize, ElementSize, Src);
  // Overwrite our IR's op type
  ALUOp.first->Header.Op = IROp;

  // Duplicate the lower bits
  auto Result = _VDupElement(Size, ElementSize, ALUOp, 0);
  StoreResult(FPRClass, Op, Result, -1);
}

template <IROps IROp, size_t ElementSize>
void OpDispatchBuilder::VectorUnaryDuplicateOp(OpcodeArgs) {
  VectorUnaryDuplicateOpImpl(Op, IROp, ElementSize);
}

template
void OpDispatchBuilder::VectorUnaryDuplicateOp<IR::OP_VFRSQRT, 4>(OpcodeArgs);
template
void OpDispatchBuilder::VectorUnaryDuplicateOp<IR::OP_VFRECP, 4>(OpcodeArgs);

void OpDispatchBuilder::MOVQOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  // This instruction is a bit special that if the destination is a register then it'll ZEXT the 64bit source to 128bit
  if (Op->Dest.IsGPR()) {
    const auto gpr = Op->Dest.Data.GPR.GPR;
    const auto gprIndex = gpr - X86State::REG_XMM_0;

    auto Reg = _VMov(16, Src);
    StoreXMMRegister(gprIndex, Reg);
  }
  else {
    // This is simple, just store the result
    StoreResult(FPRClass, Op, Src, -1);
  }
}

template<size_t ElementSize>
void OpDispatchBuilder::MOVMSKOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);
  uint8_t NumElements = Size / ElementSize;

  OrderedNode *CurrentVal = _Constant(0);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  for (unsigned i = 0; i < NumElements; ++i) {
    // Extract the top bit of the element
    OrderedNode *Tmp = _VExtractToGPR(16, ElementSize, Src, i);
    Tmp = _Bfe(1, ElementSize * 8 - 1, Tmp);

    // Shift it to the correct location
    Tmp = _Lshl(Tmp, _Constant(i));

    // Or it with the current value
    CurrentVal = _Or(CurrentVal, Tmp);
  }
  StoreResult(GPRClass, Op, CurrentVal, -1);
}

template
void OpDispatchBuilder::MOVMSKOp<4>(OpcodeArgs);
template
void OpDispatchBuilder::MOVMSKOp<8>(OpcodeArgs);

void OpDispatchBuilder::MOVMSKOpOne(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *VMask = _VDupFromGPR(16, 8, _Constant(0x80'40'20'10'08'04'02'01ULL));

  auto VCMP = _VCMPLTZ(16, 1, Src);
  auto VAnd = _VAnd(16, 1, VCMP, VMask);

  auto VAdd1 = _VAddP(16, 1, VAnd, VAnd);
  auto VAdd2 = _VAddP(8, 1, VAdd1, VAdd1);
  auto VAdd3 = _VAddP(8, 1, VAdd2, VAdd2);

  StoreResult(GPRClass, Op, _VExtractToGPR(16, 2, VAdd3, 0), -1);
}

template<size_t ElementSize>
void OpDispatchBuilder::PUNPCKLOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);

  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  auto ALUOp = _VZip(Size, ElementSize, Dest, Src);
  StoreResult(FPRClass, Op, ALUOp, -1);
}

template
void OpDispatchBuilder::PUNPCKLOp<1>(OpcodeArgs);
template
void OpDispatchBuilder::PUNPCKLOp<2>(OpcodeArgs);
template
void OpDispatchBuilder::PUNPCKLOp<4>(OpcodeArgs);
template
void OpDispatchBuilder::PUNPCKLOp<8>(OpcodeArgs);

template <size_t ElementSize>
void OpDispatchBuilder::VPUNPCKLOp(OpcodeArgs) {
  const auto SrcSize = GetSrcSize(Op);
  const auto Is128Bit = SrcSize == Core::CPUState::XMM_SSE_REG_SIZE;

  OrderedNode *Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags, -1);

  OrderedNode *Result{};
  if (Is128Bit) {
    Result = _VZip(SrcSize, ElementSize, Src1, Src2);
  } else {
    OrderedNode *ZipLo = _VZip(SrcSize, ElementSize, Src1, Src2);
    OrderedNode *ZipHi = _VZip2(SrcSize, ElementSize, Src1, Src2);

    Result = _VInsElement(SrcSize, 16, 1, 0, ZipLo, ZipHi);
  }

  StoreResult(FPRClass, Op, Result, -1);
}

template
void OpDispatchBuilder::VPUNPCKLOp<1>(OpcodeArgs);
template
void OpDispatchBuilder::VPUNPCKLOp<2>(OpcodeArgs);
template
void OpDispatchBuilder::VPUNPCKLOp<4>(OpcodeArgs);
template
void OpDispatchBuilder::VPUNPCKLOp<8>(OpcodeArgs);

template<size_t ElementSize>
void OpDispatchBuilder::PUNPCKHOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  auto ALUOp = _VZip2(Size, ElementSize, Dest, Src);
  StoreResult(FPRClass, Op, ALUOp, -1);
}

template
void OpDispatchBuilder::PUNPCKHOp<1>(OpcodeArgs);
template
void OpDispatchBuilder::PUNPCKHOp<2>(OpcodeArgs);
template
void OpDispatchBuilder::PUNPCKHOp<4>(OpcodeArgs);
template
void OpDispatchBuilder::PUNPCKHOp<8>(OpcodeArgs);

template <size_t ElementSize>
void OpDispatchBuilder::VPUNPCKHOp(OpcodeArgs) {
  const auto SrcSize = GetSrcSize(Op);
  const auto Is128Bit = SrcSize == Core::CPUState::XMM_SSE_REG_SIZE;

  OrderedNode *Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags, -1);

  OrderedNode *Result{};
  if (Is128Bit) {
    Result = _VZip2(SrcSize, ElementSize, Src1, Src2);
  } else {
    OrderedNode *ZipLo = _VZip(SrcSize, ElementSize, Src1, Src2);
    OrderedNode *ZipHi = _VZip2(SrcSize, ElementSize, Src1, Src2);

    Result = _VInsElement(SrcSize, 16, 0, 1, ZipHi, ZipLo);
  }

  StoreResult(FPRClass, Op, Result, -1);
}

template
void OpDispatchBuilder::VPUNPCKHOp<1>(OpcodeArgs);
template
void OpDispatchBuilder::VPUNPCKHOp<2>(OpcodeArgs);
template
void OpDispatchBuilder::VPUNPCKHOp<4>(OpcodeArgs);
template
void OpDispatchBuilder::VPUNPCKHOp<8>(OpcodeArgs);

OrderedNode* OpDispatchBuilder::PSHUFBOpImpl(OpcodeArgs, const X86Tables::DecodedOperand& Src1,
                                             const X86Tables::DecodedOperand& Src2) {
  OrderedNode *Src1Node = LoadSource(FPRClass, Op, Src1, Op->Flags, -1);
  OrderedNode *Src2Node = LoadSource(FPRClass, Op, Src2, Op->Flags, -1);

  const auto SrcSize = GetSrcSize(Op);
  const auto Is256Bit = SrcSize == Core::CPUState::XMM_AVX_REG_SIZE;

  // We perform the 256-bit version as two 128-bit operations due to
  // the lane splitting behavior, so cap the maximum size at 16.
  const auto SanitizedSrcSize = std::min(SrcSize, uint8_t{16});

  // PSHUFB doesn't 100% match VTBL behaviour
  // VTBL will set the element zero if the index is greater than
  // the number of elements in the array
  //
  // Bit 7 is the only bit that is supposed to set elements to zero with PSHUFB
  // Mask the selection bits and top bit correctly
  // Bits [6:4] is reserved for 128-bit/256-bit
  // Bits [6:3] is reserved for 64-bit
  const uint8_t MaskImm = SrcSize == 8 ? 0b1000'0111
                                       : 0b1000'1111;

  OrderedNode *MaskVector = _VectorImm(SrcSize, 1, MaskImm);
  OrderedNode *MaskedIndices = _VAnd(SrcSize, SrcSize, Src2Node, MaskVector);

  OrderedNode *Low = _VTBL1(SanitizedSrcSize, Src1Node, MaskedIndices);
  if (!Is256Bit) {
    return Low;
  }

  OrderedNode *HighSrc1 = _VInsElement(SrcSize, 16, 0, 1, Src1Node, Src1Node);
  OrderedNode *High = _VTBL1(SanitizedSrcSize, HighSrc1, MaskedIndices);
  return _VInsElement(SrcSize, 16, 1, 0, Low, High);
}

void OpDispatchBuilder::PSHUFBOp(OpcodeArgs) {
  OrderedNode *Result = PSHUFBOpImpl(Op, Op->Dest, Op->Src[0]);
  StoreResult(FPRClass, Op, Result, -1);
}

void OpDispatchBuilder::VPSHUFBOp(OpcodeArgs) {
  OrderedNode *Result = PSHUFBOpImpl(Op, Op->Src[0], Op->Src[1]);
  StoreResult(FPRClass, Op, Result, -1);
}

template<size_t ElementSize, bool HalfSize, bool Low>
void OpDispatchBuilder::PSHUFDOp(OpcodeArgs) {
  static_assert(ElementSize != 0);

  const auto Size = GetSrcSize(Op);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  uint8_t Shuffle = Op->Src[1].Data.Literal.Value;

  uint8_t NumElements = Size / ElementSize;

  // 16bit is a bit special of a shuffle
  // It only ever operates on half the register
  // Then there is a high and low variant of the instruction to determine where the destination goes
  // and where the source comes from
  if constexpr (HalfSize) {
    NumElements /= 2;
  }

  uint8_t BaseElement = Low ? 0 : NumElements;

  auto Dest = Src;
  for (uint8_t Element = 0; Element < NumElements; ++Element) {
    Dest = _VInsElement(Size, ElementSize, BaseElement + Element, BaseElement + (Shuffle & 0b11), Dest, Src);
    Shuffle >>= 2;
  }

  StoreResult(FPRClass, Op, Dest, -1);
}

template
void OpDispatchBuilder::PSHUFDOp<2, false, true>(OpcodeArgs);
template
void OpDispatchBuilder::PSHUFDOp<2, true, false>(OpcodeArgs);
template
void OpDispatchBuilder::PSHUFDOp<2, true, true>(OpcodeArgs);
template
void OpDispatchBuilder::PSHUFDOp<4, false, true>(OpcodeArgs);

template <size_t ElementSize, bool Low>
void OpDispatchBuilder::VPSHUFWOp(OpcodeArgs) {
  const auto SrcSize = GetSrcSize(Op);
  const auto Is256Bit = SrcSize == Core::CPUState::XMM_AVX_REG_SIZE;

  LOGMAN_THROW_A_FMT(Op->Src[1].IsLiteral(), "Src[1] needs to be a literal");
  auto Shuffle = Op->Src[1].Data.Literal.Value;

  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

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
  OrderedNode *Result = Src;
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

  StoreResult(FPRClass, Op, Result, -1);
}
template
void OpDispatchBuilder::VPSHUFWOp<2, false>(OpcodeArgs);
template
void OpDispatchBuilder::VPSHUFWOp<2, true>(OpcodeArgs);
template
void OpDispatchBuilder::VPSHUFWOp<4, true>(OpcodeArgs);

OrderedNode* OpDispatchBuilder::SHUFOpImpl(OpcodeArgs, size_t ElementSize,
                                           const X86Tables::DecodedOperand& Src1,
                                           const X86Tables::DecodedOperand& Src2,
                                           const X86Tables::DecodedOperand& Imm) {
  OrderedNode *Src1Node = LoadSource(FPRClass, Op, Src1, Op->Flags, -1);
  OrderedNode *Src2Node = LoadSource(FPRClass, Op, Src2, Op->Flags, -1);

  LOGMAN_THROW_A_FMT(Imm.IsLiteral(), "Imm needs to be a literal");
  uint8_t Shuffle = Imm.Data.Literal.Value;

  // Since 256-bit variants and up don't lane cross, we can construct
  // everything in terms of the 128-variant, as each lane is essentially
  // its own 128-bit segment.
  const uint8_t NumElements = Core::CPUState::XMM_SSE_REG_SIZE / ElementSize;
  const uint8_t HalfNumElements = NumElements >> 1;

  const uint8_t DstSize = GetDstSize(Op);
  const bool Is256Bit = DstSize == Core::CPUState::XMM_AVX_REG_SIZE;

  std::array<OrderedNode*, 4> Srcs{};
  for (size_t i = 0; i < HalfNumElements; ++i) {
    Srcs[i] = Src1Node;
  }
  for (size_t i = HalfNumElements; i < NumElements; ++i) {
    Srcs[i] = Src2Node;
  }

  OrderedNode *Dest = Src1Node;
  const uint8_t SelectionMask = NumElements - 1;
  const uint8_t ShiftAmount = std::popcount(SelectionMask);

  if (Is256Bit) {
    for (uint8_t Element = 0; Element < NumElements; ++Element) {
      const auto SrcIndex1  = Shuffle & SelectionMask;

      // AVX differs the behavior of VSHUFPD and VSHUFPS.
      // The same immediate bits are used for both lanes with VSHUFPS,
      // but VSHUFPD uses different immediate bits for each lane.
      const auto SrcIndex2 = ElementSize == 4 ? SrcIndex1
                                              : ((Shuffle >> 2) & SelectionMask);

      OrderedNode *Insert = _VInsElement(DstSize, ElementSize, Element, SrcIndex1, Dest, Srcs[Element]);
      Dest = _VInsElement(DstSize, ElementSize, Element + NumElements, SrcIndex2 + NumElements, Insert, Srcs[Element]);

      Shuffle >>= ShiftAmount;
    }
  } else {
    for (uint8_t Element = 0; Element < NumElements; ++Element) {
      const auto SrcIndex = Shuffle & SelectionMask;
      Dest = _VInsElement(DstSize, ElementSize, Element, SrcIndex, Dest, Srcs[Element]);
      Shuffle >>= ShiftAmount;
    }
  }

  return Dest;
}

template<size_t ElementSize>
void OpDispatchBuilder::SHUFOp(OpcodeArgs) {
  OrderedNode *Result = SHUFOpImpl(Op, ElementSize, Op->Dest, Op->Src[0], Op->Src[1]);
  StoreResult(FPRClass, Op, Result, -1);
}
template
void OpDispatchBuilder::SHUFOp<4>(OpcodeArgs);
template
void OpDispatchBuilder::SHUFOp<8>(OpcodeArgs);

template <size_t ElementSize>
void OpDispatchBuilder::VSHUFOp(OpcodeArgs) {
  OrderedNode *Result = SHUFOpImpl(Op, ElementSize, Op->Src[0], Op->Src[1], Op->Src[2]);
  StoreResult(FPRClass, Op, Result, -1);
}
template
void OpDispatchBuilder::VSHUFOp<4>(OpcodeArgs);
template
void OpDispatchBuilder::VSHUFOp<8>(OpcodeArgs);

void OpDispatchBuilder::VANDNOp(OpcodeArgs) {
  const auto Size = GetSrcSize(Op);
  const auto Is128Bit = Size == Core::CPUState::XMM_SSE_REG_SIZE;

  OrderedNode *Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags, -1);

  OrderedNode *Dest = _VBic(Size, Size, Src2, Src1);
  if (Is128Bit) {
    Dest = _VMov(16, Dest);
  }

  StoreResult(FPRClass, Op, Dest, -1);
}

template <IROps IROp, size_t ElementSize>
void OpDispatchBuilder::VHADDPOp(OpcodeArgs) {
  const auto SrcSize = GetSrcSize(Op);
  const auto Is256Bit = SrcSize == Core::CPUState::XMM_AVX_REG_SIZE;

  OrderedNode *Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags, -1);

  auto Res = _VFAddP(SrcSize, ElementSize, Src1, Src2);
  Res.first->Header.Op = IROp;

  OrderedNode *Dest{};

   if (Is256Bit) {
    Dest = _VInsElement(SrcSize, 8, 1, 2, Res, Res);
    Dest = _VInsElement(SrcSize, 8, 2, 1, Dest, Res);
  } else {
    Dest = _VMov(SrcSize, Res);
  }

  StoreResult(FPRClass, Op, Dest, -1);
}

template
void OpDispatchBuilder::VHADDPOp<IR::OP_VADDP, 2>(OpcodeArgs);
template
void OpDispatchBuilder::VHADDPOp<IR::OP_VADDP, 4>(OpcodeArgs);
template
void OpDispatchBuilder::VHADDPOp<IR::OP_VFADDP, 4>(OpcodeArgs);
template
void OpDispatchBuilder::VHADDPOp<IR::OP_VFADDP, 8>(OpcodeArgs);

template <size_t ElementSize>
void OpDispatchBuilder::VBROADCASTOp(OpcodeArgs) {
  const auto DstSize = GetDstSize(Op);
  const auto Is128Bit = DstSize == Core::CPUState::XMM_SSE_REG_SIZE;

  OrderedNode *Result{};

  if (Op->Src[0].IsGPR()) {
    OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
    Result = _VDupElement(DstSize, ElementSize, Src, 0);
  } else {
    OrderedNode *Src = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], ElementSize, Op->Flags, -1);
    Result = _VDupElement(DstSize, ElementSize, Src, 0);
  }

  if (Is128Bit) {
    Result = _VMov(16, Result);
  }

  StoreResult(FPRClass, Op, Result, -1);
}

template
void OpDispatchBuilder::VBROADCASTOp<1>(OpcodeArgs);
template
void OpDispatchBuilder::VBROADCASTOp<2>(OpcodeArgs);
template
void OpDispatchBuilder::VBROADCASTOp<4>(OpcodeArgs);
template
void OpDispatchBuilder::VBROADCASTOp<8>(OpcodeArgs);
template
void OpDispatchBuilder::VBROADCASTOp<16>(OpcodeArgs);

template<size_t ElementSize>
void OpDispatchBuilder::PINSROp(OpcodeArgs) {
  auto Size = GetDstSize(Op);

  OrderedNode *Src{};
  if (Op->Src[0].IsGPR()) {
    Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);
  }
  else {
    // If loading from memory then we only load the element size
    Src = LoadSource_WithOpSize(GPRClass, Op, Op->Src[0], ElementSize, Op->Flags, -1);
  }
  OrderedNode *Dest = LoadSource_WithOpSize(FPRClass, Op, Op->Dest, GetDstSize(Op), Op->Flags, -1);
  LOGMAN_THROW_A_FMT(Op->Src[1].IsLiteral(), "Src1 needs to be literal here");
  uint64_t Index = Op->Src[1].Data.Literal.Value;

  uint8_t NumElements = Size / ElementSize;
  Index &= NumElements - 1;

  // This maps 1:1 to an AArch64 NEON Op
  auto ALUOp = _VInsGPR(Size, ElementSize, Index, Dest, Src);
  StoreResult(FPRClass, Op, ALUOp, -1);
}

template
void OpDispatchBuilder::PINSROp<1>(OpcodeArgs);
template
void OpDispatchBuilder::PINSROp<2>(OpcodeArgs);
template
void OpDispatchBuilder::PINSROp<4>(OpcodeArgs);
template
void OpDispatchBuilder::PINSROp<8>(OpcodeArgs);

OrderedNode* OpDispatchBuilder::InsertPSOpImpl(OpcodeArgs, const X86Tables::DecodedOperand& Src1,
                                               const X86Tables::DecodedOperand& Src2,
                                               const X86Tables::DecodedOperand& Imm) {
  LOGMAN_THROW_A_FMT(Imm.IsLiteral(), "Imm needs to be literal here");
  const uint8_t ImmValue = Imm.Data.Literal.Value;
  uint8_t CountS = (ImmValue >> 6);
  uint8_t CountD = (ImmValue >> 4) & 0b11;
  const uint8_t ZMask = ImmValue & 0xF;

  const auto DstSize = GetDstSize(Op);

  OrderedNode *Dest{};
  if (ZMask != 0xF) {
    // Only need to load destination if it isn't a full zero
    Dest = LoadSource_WithOpSize(FPRClass, Op, Src1, DstSize, Op->Flags, -1);
  }

  if ((ZMask & (1 << CountD)) == 0) {
    // In the case that ZMask overwrites the destination element, then don't even insert
    OrderedNode *Src{};
    if (Src2.IsGPR()) {
      Src = LoadSource(FPRClass, Op, Src2, Op->Flags, -1);
    } else {
      // If loading from memory then CountS is forced to zero
      CountS = 0;
      Src = LoadSource_WithOpSize(FPRClass, Op, Src2, 4, Op->Flags, -1);
    }

    Dest = _VInsElement(DstSize, 4, CountD, CountS, Dest, Src);
  }

  // ZMask happens after insert
  if (ZMask == 0xF) {
    return _VectorImm(16, 4, 0);
  }

  if (ZMask) {
    auto Zero = _VectorImm(16, 4, 0);
    for (size_t i = 0; i < 4; ++i) {
      if ((ZMask & (1 << i)) != 0) {
        Dest = _VInsElement(DstSize, 4, i, 0, Dest, Zero);
      }
    }
  }

  return Dest;
}

void OpDispatchBuilder::InsertPSOp(OpcodeArgs) {
  OrderedNode *Result = InsertPSOpImpl(Op, Op->Dest, Op->Src[0], Op->Src[1]);
  StoreResult(FPRClass, Op, Result, -1);
}

void OpDispatchBuilder::VINSERTPSOp(OpcodeArgs) {
  OrderedNode *Insert = InsertPSOpImpl(Op, Op->Src[0], Op->Src[1], Op->Src[2]);
  OrderedNode *Result = _VMov(16, Insert);

  StoreResult(FPRClass, Op, Result, -1);
}

template<size_t ElementSize>
void OpDispatchBuilder::PExtrOp(OpcodeArgs) {
  const auto Size = GetSrcSize(Op);
  const auto DstSize = GetDstSize(Op);
  const auto Is32Bit = ElementSize == 4;

  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  LOGMAN_THROW_A_FMT(Op->Src[1].IsLiteral(), "Src1 needs to be literal here");
  uint64_t Index = Op->Src[1].Data.Literal.Value;

  const uint8_t NumElements = Is32Bit ? Size / DstSize
                                      : Size / ElementSize;
  Index &= NumElements - 1;

  OrderedNode *Result = Is32Bit ? _VExtractToGPR(16, DstSize, Src, Index)
                                : _VExtractToGPR(16, ElementSize, Src, Index);

  if (Op->Dest.IsGPR()) {
    const uint8_t GPRSize = CTX->GetGPRSize();

    // If we are storing to a GPR then we zero extend it
    if constexpr (ElementSize < 4) {
      Result = _Bfe(GPRSize, ElementSize * 8, 0, Result);
    }
    StoreResult_WithOpSize(GPRClass, Op, Op->Dest, Result, GPRSize, -1);
  } else {
    // If we are storing to memory then we store the size of the element extracted
    const auto StoreSize = Is32Bit ? DstSize : ElementSize;
    StoreResult_WithOpSize(GPRClass, Op, Op->Dest, Result, StoreSize, -1);
  }
}

template
void OpDispatchBuilder::PExtrOp<1>(OpcodeArgs);
template
void OpDispatchBuilder::PExtrOp<2>(OpcodeArgs);
template
void OpDispatchBuilder::PExtrOp<4>(OpcodeArgs);
template
void OpDispatchBuilder::PExtrOp<8>(OpcodeArgs);

void OpDispatchBuilder::VEXTRACT128Op(OpcodeArgs) {
  const auto DstIsXMM = Op->Dest.IsGPR();
  const auto StoreSize = DstIsXMM ? 32 : 16;

  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  LOGMAN_THROW_A_FMT(Op->Src[1].IsLiteral(), "Src[1] needs to be literal here");
  const auto Selector = Op->Src[1].Data.Literal.Value & 0b1;

  // A selector of zero is the same as doing a 128-bit vector move.
  if (Selector == 0) {
    OrderedNode *Result = DstIsXMM ? _VMov(16, Src) : Src;
    StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Result, StoreSize, -1);
    return;
  }

  // Otherwise replicate the element and only store the first 128-bits.
  OrderedNode *Result = _VDupElement(32, 16, Src, Selector);
  if (DstIsXMM) {
    Result = _VMov(16, Result);
  }
  StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Result, StoreSize, -1);
}

OrderedNode* OpDispatchBuilder::PSIGNImpl(OpcodeArgs, size_t ElementSize,
                                          OrderedNode *Src1, OrderedNode *Src2) {
  const auto Size = GetSrcSize(Op);

  auto ZeroVec = _VectorZero(Size);
  auto NegVec = _VNeg(Size, ElementSize, Src1);

  OrderedNode *CmpLT = _VCMPLTZ(Size, ElementSize, Src2);
  OrderedNode *CmpEQ = _VCMPEQZ(Size, ElementSize, Src2);
  OrderedNode *CmpGT = _VCMPGTZ(Size, ElementSize, Src2);

  // Negative elements return -dest
  CmpLT = _VAnd(Size, ElementSize, CmpLT, NegVec);

  // Zero elements return 0
  CmpEQ = _VAnd(Size, ElementSize, CmpEQ, ZeroVec);

  // Positive elements return dest
  CmpGT = _VAnd(Size, ElementSize, CmpGT, Src1);

  // Or our results
  return _VOr(Size, ElementSize, CmpGT, _VOr(Size, ElementSize, CmpLT, CmpEQ));
}

template <size_t ElementSize>
void OpDispatchBuilder::PSIGN(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode* Res = PSIGNImpl(Op, ElementSize, Dest, Src);
  
  StoreResult(FPRClass, Op, Res, -1);
}

template
void OpDispatchBuilder::PSIGN<1>(OpcodeArgs);
template
void OpDispatchBuilder::PSIGN<2>(OpcodeArgs);
template
void OpDispatchBuilder::PSIGN<4>(OpcodeArgs);

template <size_t ElementSize>
void OpDispatchBuilder::VPSIGN(OpcodeArgs) {
  const auto Is128Bit = GetSrcSize(Op) == Core::CPUState::XMM_SSE_REG_SIZE;

  OrderedNode *Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags, -1);
  OrderedNode* Res = PSIGNImpl(Op, ElementSize, Src1, Src2);

  if (Is128Bit) {
    Res = _VMov(16, Res);
  }

  StoreResult(FPRClass, Op, Res, -1);
}

template
void OpDispatchBuilder::VPSIGN<1>(OpcodeArgs);
template
void OpDispatchBuilder::VPSIGN<2>(OpcodeArgs);
template
void OpDispatchBuilder::VPSIGN<4>(OpcodeArgs);

OrderedNode* OpDispatchBuilder::PSRLDOpImpl(OpcodeArgs, size_t ElementSize,
                                            OrderedNode *Src, OrderedNode *ShiftVec) {
  const auto Size = GetSrcSize(Op);

  // Incoming element size for the shift source is always 8
  auto MaxShift = _VectorImm(8, 8, ElementSize * 8);
  ShiftVec = _VUMin(8, 8, MaxShift, ShiftVec);

  return _VUShrS(Size, ElementSize, Src, ShiftVec);
}

template<size_t ElementSize>
void OpDispatchBuilder::PSRLDOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Result = PSRLDOpImpl(Op, ElementSize, Dest, Src);

  StoreResult(FPRClass, Op, Result, -1);
}

template
void OpDispatchBuilder::PSRLDOp<2>(OpcodeArgs);
template
void OpDispatchBuilder::PSRLDOp<4>(OpcodeArgs);
template
void OpDispatchBuilder::PSRLDOp<8>(OpcodeArgs);

template <size_t ElementSize>
void OpDispatchBuilder::VPSRLDOp(OpcodeArgs) {
  const auto DstSize = GetDstSize(Op);
  const auto Is128Bit = DstSize == Core::CPUState::XMM_SSE_REG_SIZE;

  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Shift = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags, -1);
  OrderedNode *Result = PSRLDOpImpl(Op, ElementSize, Src, Shift);

  if (Is128Bit) {
    Result = _VMov(16, Result);
  }
  StoreResult(FPRClass, Op, Result, -1);
}

template
void OpDispatchBuilder::VPSRLDOp<2>(OpcodeArgs);
template
void OpDispatchBuilder::VPSRLDOp<4>(OpcodeArgs);
template
void OpDispatchBuilder::VPSRLDOp<8>(OpcodeArgs);

template<size_t ElementSize>
void OpDispatchBuilder::PSRLI(OpcodeArgs) {
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);

  LOGMAN_THROW_A_FMT(Op->Src[1].IsLiteral(), "Src1 needs to be literal here");
  uint64_t ShiftConstant = Op->Src[1].Data.Literal.Value;

  auto Size = GetSrcSize(Op);

  auto Shift = _VUShrI(Size, ElementSize, Dest, ShiftConstant);
  StoreResult(FPRClass, Op, Shift, -1);
}

template
void OpDispatchBuilder::PSRLI<2>(OpcodeArgs);
template
void OpDispatchBuilder::PSRLI<4>(OpcodeArgs);
template
void OpDispatchBuilder::PSRLI<8>(OpcodeArgs);

template <size_t ElementSize>
void OpDispatchBuilder::VPSRLIOp(OpcodeArgs) {
  const auto Size = GetSrcSize(Op);
  const auto Is128Bit = Size == Core::CPUState::XMM_SSE_REG_SIZE;

  LOGMAN_THROW_A_FMT(Op->Src[1].IsLiteral(), "Src1 needs to be literal here");
  const uint64_t ShiftConstant = Op->Src[1].Data.Literal.Value;

  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Result = _VUShrI(Size, ElementSize, Src, ShiftConstant);

  if (Is128Bit) {
    Result = _VMov(16, Result);
  }
  StoreResult(FPRClass, Op, Result, -1);
}

template
void OpDispatchBuilder::VPSRLIOp<2>(OpcodeArgs);
template
void OpDispatchBuilder::VPSRLIOp<4>(OpcodeArgs);
template
void OpDispatchBuilder::VPSRLIOp<8>(OpcodeArgs);

OrderedNode* OpDispatchBuilder::PSLLIImpl(OpcodeArgs, size_t ElementSize,
                                          OrderedNode *Src, uint64_t Shift) {
  const auto Size = GetSrcSize(Op);
  return _VShlI(Size, ElementSize, Src, Shift);
}

template<size_t ElementSize>
void OpDispatchBuilder::PSLLI(OpcodeArgs) {
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);

  LOGMAN_THROW_A_FMT(Op->Src[1].IsLiteral(), "Src1 needs to be literal here");
  const uint64_t ShiftConstant = Op->Src[1].Data.Literal.Value;

  OrderedNode *Result = PSLLIImpl(Op, ElementSize, Dest, ShiftConstant);

  StoreResult(FPRClass, Op, Result, -1);
}

template
void OpDispatchBuilder::PSLLI<2>(OpcodeArgs);
template
void OpDispatchBuilder::PSLLI<4>(OpcodeArgs);
template
void OpDispatchBuilder::PSLLI<8>(OpcodeArgs);

template <size_t ElementSize>
void OpDispatchBuilder::VPSLLIOp(OpcodeArgs) {
  const auto DstSize = GetDstSize(Op);
  const auto Is128Bit = DstSize == Core::CPUState::XMM_SSE_REG_SIZE;
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  LOGMAN_THROW_A_FMT(Op->Src[1].IsLiteral(), "Src1 needs to be literal here");
  const uint64_t ShiftConstant = Op->Src[1].Data.Literal.Value;

  OrderedNode *Result = PSLLIImpl(Op, ElementSize, Src, ShiftConstant);
  if (Is128Bit) {
    Result = _VMov(16, Result);
  }

  StoreResult(FPRClass, Op, Result, -1);
}

template
void OpDispatchBuilder::VPSLLIOp<2>(OpcodeArgs);
template
void OpDispatchBuilder::VPSLLIOp<4>(OpcodeArgs);
template
void OpDispatchBuilder::VPSLLIOp<8>(OpcodeArgs);

OrderedNode* OpDispatchBuilder::PSLLImpl(OpcodeArgs, size_t ElementSize,
                                         OrderedNode *Src, OrderedNode *ShiftVec) {
  const auto DstSize = GetDstSize(Op);

  // Incoming element size for the shift source is always 8
  auto MaxShift = _VectorImm(8, 8, ElementSize * 8);
  ShiftVec = _VUMin(8, 8, MaxShift, ShiftVec);

  return _VUShlS(DstSize, ElementSize, Src, ShiftVec);
}

template<size_t ElementSize>
void OpDispatchBuilder::PSLL(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Result = PSLLImpl(Op, ElementSize, Dest, Src);

  StoreResult(FPRClass, Op, Result, -1);
}

template
void OpDispatchBuilder::PSLL<2>(OpcodeArgs);
template
void OpDispatchBuilder::PSLL<4>(OpcodeArgs);
template
void OpDispatchBuilder::PSLL<8>(OpcodeArgs);

template <size_t ElementSize>
void OpDispatchBuilder::VPSLLOp(OpcodeArgs) {
  const auto DstSize = GetDstSize(Op);
  const auto Is128Bit = DstSize == Core::CPUState::XMM_SSE_REG_SIZE;

  OrderedNode *Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Src2 = LoadSource_WithOpSize(FPRClass, Op, Op->Src[1], 16, Op->Flags, -1);
  OrderedNode *Result = PSLLImpl(Op, ElementSize, Src1, Src2);

  if (Is128Bit) {
    Result = _VMov(16, Result);
  }
  StoreResult(FPRClass, Op, Result, -1);
}

template
void OpDispatchBuilder::VPSLLOp<2>(OpcodeArgs);
template
void OpDispatchBuilder::VPSLLOp<4>(OpcodeArgs);
template
void OpDispatchBuilder::VPSLLOp<8>(OpcodeArgs);

OrderedNode* OpDispatchBuilder::PSRAOpImpl(OpcodeArgs, size_t ElementSize,
                                           OrderedNode *Src, OrderedNode *ShiftVec) {
  const auto Size = GetDstSize(Op);

  // Incoming element size for the shift source is always 8
  auto MaxShift = _VectorImm(8, 8, ElementSize * 8);
  ShiftVec = _VUMin(8, 8, MaxShift, ShiftVec);

  return _VSShrS(Size, ElementSize, Src, ShiftVec);
}

template<size_t ElementSize>
void OpDispatchBuilder::PSRAOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Result = PSRAOpImpl(Op, ElementSize, Dest, Src);

  StoreResult(FPRClass, Op, Result, -1);
}

template
void OpDispatchBuilder::PSRAOp<2>(OpcodeArgs);
template
void OpDispatchBuilder::PSRAOp<4>(OpcodeArgs);

template <size_t ElementSize>
void OpDispatchBuilder::VPSRAOp(OpcodeArgs) {
  const auto DstSize = GetDstSize(Op);
  const auto Is128Bit = DstSize == Core::CPUState::XMM_SSE_REG_SIZE;

  OrderedNode *Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags, -1);
  OrderedNode *Result = PSRAOpImpl(Op, ElementSize, Src1, Src2);

  if (Is128Bit) {
    Result = _VMov(16, Result);
  }
  StoreResult(FPRClass, Op, Result, -1);
}

template
void OpDispatchBuilder::VPSRAOp<2>(OpcodeArgs);
template
void OpDispatchBuilder::VPSRAOp<4>(OpcodeArgs);

void OpDispatchBuilder::PSRLDQ(OpcodeArgs) {
  LOGMAN_THROW_A_FMT(Op->Src[1].IsLiteral(), "Src1 needs to be literal here");
  uint64_t Shift = Op->Src[1].Data.Literal.Value;

  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);

  auto Size = GetDstSize(Op);

  OrderedNode *Result = _VectorZero(Size);
  if (Shift < Size) {
    Result = _VExtr(Size, 1, Result, Dest, Shift);
  }

  StoreResult(FPRClass, Op, Result, -1);
}

void OpDispatchBuilder::VPSRLDQOp(OpcodeArgs) {
  const auto DstSize = GetDstSize(Op);
  const auto Is128Bit = DstSize == Core::CPUState::XMM_SSE_REG_SIZE;

  LOGMAN_THROW_A_FMT(Op->Src[1].IsLiteral(), "Src1 needs to be literal here");
  const uint64_t Shift = Op->Src[1].Data.Literal.Value;

  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  OrderedNode *Result = _VectorZero(DstSize);
  if (Is128Bit) {
    if (Shift < DstSize) {
      Result = _VExtr(DstSize, 1, Result, Src, Shift);
    }
  } else {
    if (Shift < Core::CPUState::XMM_SSE_REG_SIZE) {
      OrderedNode *ResultBottom = _VExtr(16, 1, Result, Src, Shift);
      OrderedNode *ResultTop    = _VExtr(DstSize, 1, Result, Src, 16 + Shift);

      Result = _VInsElement(DstSize, 16, 1, 0, ResultBottom, ResultTop);
    }
  }

  StoreResult(FPRClass, Op, Result, -1);
}

void OpDispatchBuilder::PSLLDQ(OpcodeArgs) {
  LOGMAN_THROW_A_FMT(Op->Src[1].IsLiteral(), "Src1 needs to be literal here");
  uint64_t Shift = Op->Src[1].Data.Literal.Value;

  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);

  auto Size = GetDstSize(Op);

  OrderedNode *Result = _VectorZero(Size);
  if (Shift < Size) {
    Result = _VExtr(Size, 1, Dest, Result, Size - Shift);
  }
  StoreResult(FPRClass, Op, Result, -1);
}

void OpDispatchBuilder::VPSLLDQOp(OpcodeArgs) {
  const auto DstSize = GetDstSize(Op);
  const auto Is128Bit = DstSize == Core::CPUState::XMM_SSE_REG_SIZE;

  LOGMAN_THROW_A_FMT(Op->Src[1].IsLiteral(), "Src1 needs to be literal here");
  const uint64_t Shift = Op->Src[1].Data.Literal.Value;

  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  OrderedNode *Result = _VectorZero(DstSize);
  if (Is128Bit) {
    if (Shift < DstSize) {
      Result = _VExtr(DstSize, 1, Src, Result, DstSize - Shift);
    }
  } else {
    if (Shift < Core::CPUState::XMM_SSE_REG_SIZE) {
      OrderedNode *ResultBottom = _VExtr(16, 1, Src, Result, 16 - Shift);
      OrderedNode* ResultTop    = _VExtr(DstSize, 1, Src, Result, DstSize - Shift);

      Result = _VInsElement(DstSize, 16, 1, 0, ResultBottom, ResultTop);
    }
  }
  StoreResult(FPRClass, Op, Result, -1);
}

template<size_t ElementSize>
void OpDispatchBuilder::PSRAIOp(OpcodeArgs) {
  LOGMAN_THROW_A_FMT(Op->Src[1].IsLiteral(), "Src1 needs to be literal here");
  uint64_t Shift = Op->Src[1].Data.Literal.Value;

  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);

  auto Size = GetDstSize(Op);

  auto Result = _VSShrI(Size, ElementSize, Dest, Shift);
  StoreResult(FPRClass, Op, Result, -1);
}

template
void OpDispatchBuilder::PSRAIOp<2>(OpcodeArgs);
template
void OpDispatchBuilder::PSRAIOp<4>(OpcodeArgs);

template <size_t ElementSize>
void OpDispatchBuilder::VPSRAIOp(OpcodeArgs) {
  LOGMAN_THROW_A_FMT(Op->Src[1].IsLiteral(), "Src1 needs to be literal here");
  const uint64_t Shift = Op->Src[1].Data.Literal.Value;

  const auto Size = GetDstSize(Op);
  const auto Is128Bit = Size == Core::CPUState::XMM_SSE_REG_SIZE;

  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Result = _VSShrI(Size, ElementSize, Src, Shift);

  if (Is128Bit) {
    Result = _VMov(16, Result);
  }
  StoreResult(FPRClass, Op, Result, -1);
}

template
void OpDispatchBuilder::VPSRAIOp<2>(OpcodeArgs);
template
void OpDispatchBuilder::VPSRAIOp<4>(OpcodeArgs);

void OpDispatchBuilder::VPSRAVDOp(OpcodeArgs) {
  const auto SrcSize = GetSrcSize(Op);
  const auto Is128Bit = SrcSize == Core::CPUState::XMM_SSE_REG_SIZE;

  OrderedNode *Vector = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *ShiftVector = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags, -1);
  OrderedNode *Result = _VSShr(SrcSize, 4, Vector, ShiftVector);
  if (Is128Bit) {
    Result = _VMov(16, Result);
  }

  StoreResult(FPRClass, Op, Result, -1);
}

void OpDispatchBuilder::MOVDDUPOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Res = _VDupElement(16, GetSrcSize(Op), Src, 0);

  StoreResult(FPRClass, Op, Res, -1);
}

void OpDispatchBuilder::VMOVDDUPOp(OpcodeArgs) {
  const auto SrcSize = GetSrcSize(Op);
  const auto IsSrcGPR = Op->Src[0].IsGPR();
  const auto Is256Bit = SrcSize == Core::CPUState::XMM_AVX_REG_SIZE;
  const auto MemSize = Is256Bit ? 32 : 8;

  OrderedNode *Src = IsSrcGPR ? LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], SrcSize, Op->Flags, -1)
                              : LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], MemSize, Op->Flags, -1);

  OrderedNode *Res = _VInsElement(SrcSize, 8, 1, 0, Src, Src);
  if (Is256Bit) {
    Res = _VInsElement(SrcSize, 8, 3, 2, Res, Src);
  } else {
    // Clear the upper lane
    Res = _VMov(16, Res);
  }

  StoreResult(FPRClass, Op, Res, -1);
}

template<size_t DstElementSize>
void OpDispatchBuilder::CVTGPR_To_FPR(OpcodeArgs) {
  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);

  size_t GPRSize = GetSrcSize(Op);

  Src = _Float_FromGPR_S(DstElementSize, GPRSize, Src);

  OrderedNode *Dest = LoadSource_WithOpSize(FPRClass, Op, Op->Dest, 16, Op->Flags, -1);

  Src = _VInsElement(16, DstElementSize, 0, 0, Dest, Src);

  StoreResult(FPRClass, Op, Src, -1);
}

template
void OpDispatchBuilder::CVTGPR_To_FPR<4>(OpcodeArgs);
template
void OpDispatchBuilder::CVTGPR_To_FPR<8>(OpcodeArgs);

template<size_t SrcElementSize, bool HostRoundingMode>
void OpDispatchBuilder::CVTFPR_To_GPR(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  // GPR size is determined by REX.W
  // Source Element size is determined by instruction
  size_t GPRSize = GetDstSize(Op);

  if constexpr (HostRoundingMode) {
    Src = _Float_ToGPR_S(GPRSize, SrcElementSize, Src);
  }
  else {
    Src = _Float_ToGPR_ZS(GPRSize, SrcElementSize, Src);
  }

  StoreResult_WithOpSize(GPRClass, Op, Op->Dest, Src, GPRSize, -1);
}

template
void OpDispatchBuilder::CVTFPR_To_GPR<4, true>(OpcodeArgs);
template
void OpDispatchBuilder::CVTFPR_To_GPR<4, false>(OpcodeArgs);

template
void OpDispatchBuilder::CVTFPR_To_GPR<8, true>(OpcodeArgs);
template
void OpDispatchBuilder::CVTFPR_To_GPR<8, false>(OpcodeArgs);

OrderedNode* OpDispatchBuilder::Vector_CVT_Int_To_FloatImpl(OpcodeArgs, size_t SrcElementSize, bool Widen) {
  const size_t Size = GetDstSize(Op);

  OrderedNode *Src = [&] {
    if (Widen) {
      const auto LoadSize = 8 * (Size / 16);
      return LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], LoadSize, Op->Flags, -1);
    } else {
      return LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
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
  OrderedNode *Result = Vector_CVT_Int_To_FloatImpl(Op, SrcElementSize, Widen);
  StoreResult(FPRClass, Op, Result, -1);
}

template
void OpDispatchBuilder::Vector_CVT_Int_To_Float<4, true>(OpcodeArgs);
template
void OpDispatchBuilder::Vector_CVT_Int_To_Float<4, false>(OpcodeArgs);

template <size_t SrcElementSize, bool Widen>
void OpDispatchBuilder::AVXVector_CVT_Int_To_Float(OpcodeArgs) {
  const auto DstSize = GetDstSize(Op);
  const auto Is128Bit = DstSize == Core::CPUState::XMM_SSE_REG_SIZE;

  OrderedNode *Result = Vector_CVT_Int_To_FloatImpl(Op, SrcElementSize, Widen);

  if (Is128Bit) {
    Result = _VMov(16, Result);
  }
  StoreResult(FPRClass, Op, Result, -1);
}

template
void OpDispatchBuilder::AVXVector_CVT_Int_To_Float<4, false>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVector_CVT_Int_To_Float<4, true>(OpcodeArgs);

OrderedNode* OpDispatchBuilder::Vector_CVT_Float_To_IntImpl(OpcodeArgs, size_t SrcElementSize,
                                                            bool Narrow, bool HostRoundingMode) {
  const size_t DstSize = GetDstSize(Op);
  size_t ElementSize = SrcElementSize;

  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

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
  const size_t DstSize = GetDstSize(Op);
  OrderedNode *Result = Vector_CVT_Float_To_IntImpl(Op, SrcElementSize, Narrow, HostRoundingMode);

  StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Result, DstSize, -1);
}

template
void OpDispatchBuilder::Vector_CVT_Float_To_Int<4, false, false>(OpcodeArgs);
template
void OpDispatchBuilder::Vector_CVT_Float_To_Int<4, false, true>(OpcodeArgs);
template
void OpDispatchBuilder::Vector_CVT_Float_To_Int<4, true, false>(OpcodeArgs);

template
void OpDispatchBuilder::Vector_CVT_Float_To_Int<8, true, true>(OpcodeArgs);
template
void OpDispatchBuilder::Vector_CVT_Float_To_Int<8, true, false>(OpcodeArgs);

template <size_t SrcElementSize, bool Narrow, bool HostRoundingMode>
void OpDispatchBuilder::AVXVector_CVT_Float_To_Int(OpcodeArgs) {
  const auto DstSize = GetDstSize(Op);
  const auto Is128Bit = DstSize == Core::CPUState::XMM_SSE_REG_SIZE;

  // VCVTPD2DQ/VCVTTPD2DQ only use the bottom lane, even for the 256-bit version.
  const auto Truncate = SrcElementSize == 8 && Narrow;

  OrderedNode *Result = Vector_CVT_Float_To_IntImpl(Op, SrcElementSize, Narrow, HostRoundingMode);

  if (Is128Bit || Truncate) {
    Result = _VMov(16, Result);
  }
  StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Result, DstSize, -1);
}

template
void OpDispatchBuilder::AVXVector_CVT_Float_To_Int<4, false, false>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVector_CVT_Float_To_Int<4, false, true>(OpcodeArgs);

template
void OpDispatchBuilder::AVXVector_CVT_Float_To_Int<8, true, false>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVector_CVT_Float_To_Int<8, true, true>(OpcodeArgs);

template<size_t DstElementSize, size_t SrcElementSize>
void OpDispatchBuilder::Scalar_CVT_Float_To_Float(OpcodeArgs) {
  const auto DstSize = GetDstSize(Op);

  OrderedNode *Dest = LoadSource_WithOpSize(FPRClass, Op, Op->Dest, DstSize, Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  Src = _Float_FToF(DstElementSize, SrcElementSize, Src);
  Src = _VInsElement(16, DstElementSize, 0, 0, Dest, Src);

  auto Result = _VInsElement(DstSize, DstElementSize, 0, 0, Dest, Src);
  StoreResult(FPRClass, Op, Result, -1);
}

template
void OpDispatchBuilder::Scalar_CVT_Float_To_Float<4, 8>(OpcodeArgs);
template
void OpDispatchBuilder::Scalar_CVT_Float_To_Float<8, 4>(OpcodeArgs);

template<size_t DstElementSize, size_t SrcElementSize>
void OpDispatchBuilder::Vector_CVT_Float_To_Float(OpcodeArgs) {
  const auto Size = GetDstSize(Op);

  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  if constexpr (DstElementSize > SrcElementSize) {
    Src = _Vector_FToF(Size, SrcElementSize << 1, Src, SrcElementSize);
  }
  else {
    Src = _Vector_FToF(Size, SrcElementSize >> 1, Src, SrcElementSize);
  }

  StoreResult(FPRClass, Op, Src, -1);
}

template
void OpDispatchBuilder::Vector_CVT_Float_To_Float<4, 8>(OpcodeArgs);
template
void OpDispatchBuilder::Vector_CVT_Float_To_Float<8, 4>(OpcodeArgs);

template<size_t SrcElementSize, bool Widen>
void OpDispatchBuilder::MMX_To_XMM_Vector_CVT_Int_To_Float(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  size_t ElementSize = SrcElementSize;
  size_t DstSize = GetDstSize(Op);
  if constexpr (Widen) {
    Src = _VSXTL(DstSize, ElementSize, Src);
    ElementSize <<= 1;
  }

  // Always signed
  Src = _Vector_SToF(DstSize, ElementSize, Src);

  OrderedNode *Dest{};
  if constexpr (Widen) {
    Dest = Src;
  }
  else {
    Dest = LoadSource_WithOpSize(FPRClass, Op, Op->Dest, DstSize, Op->Flags, -1);
    // Insert the lower bits
    Dest = _VInsElement(GetDstSize(Op), 8, 0, 0, Dest, Src);
  }

  StoreResult(FPRClass, Op, Dest, -1);
}

template
void OpDispatchBuilder::MMX_To_XMM_Vector_CVT_Int_To_Float<4, false>(OpcodeArgs);
template
void OpDispatchBuilder::MMX_To_XMM_Vector_CVT_Int_To_Float<4, true>(OpcodeArgs);

template<size_t SrcElementSize, bool HostRoundingMode>
void OpDispatchBuilder::XMM_To_MMX_Vector_CVT_Float_To_Int(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  size_t ElementSize = SrcElementSize;
  size_t Size = GetDstSize(Op);

  // Always narrows
  Src = _Vector_FToF(Size, SrcElementSize >> 1, Src, SrcElementSize);
  ElementSize >>= 1;

  if constexpr (HostRoundingMode) {
    Src = _Vector_FToS(Size, ElementSize, Src);
  }
  else {
    Src = _Vector_FToZS(Size, ElementSize, Src);
  }

  StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Src, Size, -1);
}

template
void OpDispatchBuilder::XMM_To_MMX_Vector_CVT_Float_To_Int<8, false>(OpcodeArgs);
template
void OpDispatchBuilder::XMM_To_MMX_Vector_CVT_Float_To_Int<8, true>(OpcodeArgs);

void OpDispatchBuilder::MASKMOVOp(OpcodeArgs) {
  // Until we get correct PHI nodes this is required to be a loop unroll
  const auto Size = uint32_t{GetSrcSize(Op)} * 8;

  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);

  auto MemDest = LoadGPRRegister(X86State::REG_RDI);

  const size_t NumElements = Size / 64;
  for (size_t Element = 0; Element < NumElements; ++Element) {
    // Extract the current element
    auto SrcElement = _VExtractToGPR(GetSrcSize(Op), 8, Src, Element);
    auto DestElement = _VExtractToGPR(GetSrcSize(Op), 8, Dest, Element);

    constexpr size_t NumSelectBits = 64 / 8;
    for (size_t Select = 0; Select < NumSelectBits; ++Select) {
      auto SelectMask = _Bfe(1, 8 * Select + 7, SrcElement);
      auto CondJump = _CondJump(SelectMask, {COND_EQ});
      auto StoreBlock = CreateNewCodeBlockAfter(GetCurrentBlock());
      SetFalseJumpTarget(CondJump, StoreBlock);
      SetCurrentCodeBlock(StoreBlock);
      {
        auto DestByte = _Bfe(8, 8 * Select, DestElement);
        auto MemLocation = _Add(MemDest, _Constant(Element * 8 + Select));
        // MASKMOVDQU/MASKMOVQ is explicitly weakly-ordered on its store
        _StoreMem(GPRClass, 1, MemLocation, DestByte, 1);
      }
      auto Jump = _Jump();
      auto NextJumpTarget = CreateNewCodeBlockAfter(StoreBlock);
      SetJumpTarget(Jump, NextJumpTarget);
      SetTrueJumpTarget(CondJump, NextJumpTarget);
      SetCurrentCodeBlock(NextJumpTarget);
    }
  }
}

void OpDispatchBuilder::MOVBetweenGPR_FPR(OpcodeArgs) {
  if (Op->Dest.IsGPR() &&
      Op->Dest.Data.GPR.GPR >= FEXCore::X86State::REG_XMM_0) {
    OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);
    // zext to 128bit
    auto Converted = _VCastFromGPR(16, GetSrcSize(Op), Src);
    StoreResult(FPRClass, Op, Op->Dest, Converted, -1);
  }
  else {
    // Destination is GPR or mem
    // Extract from XMM first
    auto ElementSize = GetDstSize(Op);
    OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0],Op->Flags, -1);

    Src = _VExtractToGPR(GetSrcSize(Op), ElementSize, Src, 0);

    StoreResult(GPRClass, Op, Op->Dest, Src, -1);
  }
}

OrderedNode* OpDispatchBuilder::VFCMPOpImpl(OpcodeArgs, size_t ElementSize, bool Scalar,
                                            OrderedNode *Src1, OrderedNode *Src2, uint8_t CompType) {
  const auto Size = GetSrcSize(Op);

  OrderedNode *Result{};
  switch (CompType) {
    case 0x00: case 0x08: case 0x10: case 0x18: // EQ
      Result = _VFCMPEQ(Size, ElementSize, Src1, Src2);
      break;
    case 0x01: case 0x09: case 0x11: case 0x19: // LT, GT(Swapped operand)
      Result = _VFCMPLT(Size, ElementSize, Src1, Src2);
      break;
    case 0x02: case 0x0A: case 0x12: case 0x1A: // LE, GE(Swapped operand)
      Result = _VFCMPLE(Size, ElementSize, Src1, Src2);
      break;
    case 0x03: case 0x0B: case 0x13: case 0x1B: // Unordered
      Result = _VFCMPUNO(Size, ElementSize, Src1, Src2);
      break;
    case 0x04: case 0x0C: case 0x14: case 0x1C: // NEQ
      Result = _VFCMPNEQ(Size, ElementSize, Src1, Src2);
      break;
    case 0x05: case 0x0D: case 0x15: case 0x1D: // NLT, NGT(Swapped operand)
      Result = _VFCMPLT(Size, ElementSize, Src1, Src2);
      Result = _VNot(Size, ElementSize, Result);
      break;
    case 0x06: case 0x0E: case 0x16: case 0x1E: // NLE, NGE(Swapped operand)
      Result = _VFCMPLE(Size, ElementSize, Src1, Src2);
      Result = _VNot(Size, ElementSize, Result);
      break;
    case 0x07: case 0x0F: case 0x17: case 0x1F: // Ordered
      Result = _VFCMPORD(Size, ElementSize, Src1, Src2);
      break;
    default:
      LOGMAN_MSG_A_FMT("Unknown Comparison type: {}", CompType);
      break;
  }

  if (Scalar) {
    // Insert the lower bits
    Result = _VInsElement(GetDstSize(Op), ElementSize, 0, 0, Src1, Result);
  }

  return Result;
}

template<size_t ElementSize, bool Scalar>
void OpDispatchBuilder::VFCMPOp(OpcodeArgs) {
  const auto DstSize = GetDstSize(Op);

  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Dest = LoadSource_WithOpSize(FPRClass, Op, Op->Dest, DstSize, Op->Flags, -1);
  const uint8_t CompType = Op->Src[1].Data.Literal.Value;

  OrderedNode* Result = VFCMPOpImpl(Op, ElementSize, Scalar, Dest, Src, CompType);

  StoreResult(FPRClass, Op, Result, -1);
}

template
void OpDispatchBuilder::VFCMPOp<4, false>(OpcodeArgs);
template
void OpDispatchBuilder::VFCMPOp<4, true>(OpcodeArgs);
template
void OpDispatchBuilder::VFCMPOp<8, false>(OpcodeArgs);
template
void OpDispatchBuilder::VFCMPOp<8, true>(OpcodeArgs);

template <size_t ElementSize, bool Scalar>
void OpDispatchBuilder::AVXVFCMPOp(OpcodeArgs) {
  const auto DstSize = GetDstSize(Op);
  const auto Is128Bit = DstSize == Core::CPUState::XMM_SSE_REG_SIZE;

  LOGMAN_THROW_A_FMT(Op->Src[2].IsLiteral(), "Src[2] needs to be literal");
  const uint8_t CompType = Op->Src[2].Data.Literal.Value;

  OrderedNode *Src1 = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], DstSize, Op->Flags, -1);
  OrderedNode *Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags, -1);
  OrderedNode *Result = VFCMPOpImpl(Op, ElementSize, Scalar, Src1, Src2, CompType);

  if (Is128Bit) {
    Result = _VMov(16, Result);
  }
  StoreResult(FPRClass, Op, Result, -1);
}

template
void OpDispatchBuilder::AVXVFCMPOp<4, false>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVFCMPOp<4, true>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVFCMPOp<8, false>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVFCMPOp<8, true>(OpcodeArgs);

void OpDispatchBuilder::FXSaveOp(OpcodeArgs) {
  OrderedNode *Mem = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1, false);
  Mem = AppendSegmentOffset(Mem, Op->Flags);

  // Saves 512bytes to the memory location provided
  // Header changes depending on if REX.W is set or not
  if (Op->Flags & X86Tables::DecodeFlags::FLAG_REX_WIDENING) {
    // BYTE | 0 1 | 2 3 | 4   | 5     | 6 7 | 8 9 | a b | c d | e f |
    // ------------------------------------------
    //   00 | FCW | FSW | FTW | <R>   | FOP | FIP                   |
    //   16 | FDP                           | MXCSR     | MXCSR_MASK|
  }
  else {
    // BYTE | 0 1 | 2 3 | 4   | 5     | 6 7 | 8 9 | a b | c d | e f |
    // ------------------------------------------
    //   00 | FCW | FSW | FTW | <R>   | FOP | FIP[31:0] | FCS | <R> |
    //   16 | FDP[31:0] | FDS         | <R> | MXCSR     | MXCSR_MASK|
  }

  {
    auto FCW = _LoadContext(2, GPRClass, offsetof(FEXCore::Core::CPUState, FCW));
    _StoreMem(GPRClass, 2, Mem, FCW, 2);
  }

  {
    // We must construct the FSW from our various bits
    OrderedNode *MemLocation = _Add(Mem, _Constant(2));
    OrderedNode *FSW = _Constant(0);
    auto Top = GetX87Top();
    FSW = _Or(FSW, _Lshl(Top, _Constant(11)));

    auto C0 = GetRFLAG(FEXCore::X86State::X87FLAG_C0_LOC);
    auto C1 = GetRFLAG(FEXCore::X86State::X87FLAG_C1_LOC);
    auto C2 = GetRFLAG(FEXCore::X86State::X87FLAG_C2_LOC);
    auto C3 = GetRFLAG(FEXCore::X86State::X87FLAG_C3_LOC);

    FSW = _Or(FSW, _Lshl(C0, _Constant(8)));
    FSW = _Or(FSW, _Lshl(C1, _Constant(9)));
    FSW = _Or(FSW, _Lshl(C2, _Constant(10)));
    FSW = _Or(FSW, _Lshl(C3, _Constant(14)));
    _StoreMem(GPRClass, 2, MemLocation, FSW, 2);
  }

  {
    // FTW
    OrderedNode *MemLocation = _Add(Mem, _Constant(4));
    auto FTW = _LoadContext(2, GPRClass, offsetof(FEXCore::Core::CPUState, FTW));
    _StoreMem(GPRClass, 2, MemLocation, FTW, 2);
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
  for (unsigned i = 0; i < 8; ++i) {
    OrderedNode *MMReg = _LoadContext(16, FPRClass, offsetof(FEXCore::Core::CPUState, mm[i]));
    OrderedNode *MemLocation = _Add(Mem, _Constant(i * 16 + 32));

    _StoreMem(FPRClass, 16, MemLocation, MMReg, 16);
  }

  const auto NumRegs = CTX->Config.Is64BitMode ? 16U : 8U;

  for (unsigned i = 0; i < NumRegs; ++i) {
    OrderedNode *XMMReg = LoadXMMRegister(i);
    OrderedNode *MemLocation = _Add(Mem, _Constant(i * 16 + 160));

    _StoreMem(FPRClass, 16, MemLocation, XMMReg, 16);
  }
}

void OpDispatchBuilder::FXRStoreOp(OpcodeArgs) {
  OrderedNode *Mem = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1, false);
  Mem = AppendSegmentOffset(Mem, Op->Flags);

  auto NewFCW = _LoadMem(GPRClass, 2, Mem, 2);
  _F80LoadFCW(NewFCW);
  _StoreContext(2, GPRClass, NewFCW, offsetof(FEXCore::Core::CPUState, FCW));

  {
    OrderedNode *MemLocation = _Add(Mem, _Constant(2));
    auto NewFSW = _LoadMem(GPRClass, 2, MemLocation, 2);

    // Strip out the FSW information
    auto Top = _Bfe(3, 11, NewFSW);
    SetX87Top(Top);

    auto C0 = _Bfe(1, 8,  NewFSW);
    auto C1 = _Bfe(1, 9,  NewFSW);
    auto C2 = _Bfe(1, 10, NewFSW);
    auto C3 = _Bfe(1, 14, NewFSW);

    SetRFLAG<FEXCore::X86State::X87FLAG_C0_LOC>(C0);
    SetRFLAG<FEXCore::X86State::X87FLAG_C1_LOC>(C1);
    SetRFLAG<FEXCore::X86State::X87FLAG_C2_LOC>(C2);
    SetRFLAG<FEXCore::X86State::X87FLAG_C3_LOC>(C3);
  }

  {
    // FTW
    OrderedNode *MemLocation = _Add(Mem, _Constant(4));
    auto NewFTW = _LoadMem(GPRClass, 2, MemLocation, 2);
    _StoreContext(2, GPRClass, NewFTW, offsetof(FEXCore::Core::CPUState, FTW));
  }

  for (unsigned i = 0; i < 8; ++i) {
    OrderedNode *MemLocation = _Add(Mem, _Constant(i * 16 + 32));
    auto MMReg = _LoadMem(FPRClass, 16, MemLocation, 16);
    _StoreContext(16, FPRClass, MMReg, offsetof(FEXCore::Core::CPUState, mm[i]));
  }

  const auto NumRegs = CTX->Config.Is64BitMode ? 16U : 8U;

  for (unsigned i = 0; i < NumRegs; ++i) {
    OrderedNode *MemLocation = _Add(Mem, _Constant(i * 16 + 160));
    auto XMMReg = _LoadMem(FPRClass, 16, MemLocation, 16);
    StoreXMMRegister(i, XMMReg);
  }
}

OrderedNode* OpDispatchBuilder::PALIGNROpImpl(OpcodeArgs, const X86Tables::DecodedOperand& Src1,
                                              const X86Tables::DecodedOperand& Src2,
                                              const X86Tables::DecodedOperand& Imm) {
  LOGMAN_THROW_A_FMT(Imm.IsLiteral(), "Imm needs to be a literal");

  OrderedNode *Src1Node = LoadSource(FPRClass, Op, Src1, Op->Flags, -1);
  OrderedNode *Src2Node = LoadSource(FPRClass, Op, Src2, Op->Flags, -1);

  // For the 256-bit case we handle it as pairs of 128-bit halves.
  const auto DstSize = GetDstSize(Op);
  const auto SanitizedDstSize = std::min(DstSize, uint8_t{16});

  const auto Is256Bit = DstSize == Core::CPUState::XMM_AVX_REG_SIZE;
  const auto Index = Imm.Data.Literal.Value;

  if (Index >= (SanitizedDstSize * 2)) {
    // If the immediate is greater than both vectors combined then it zeroes the vector
    return _VectorZero(DstSize);
  }

  OrderedNode *Low = _VExtr(SanitizedDstSize, 1, Src1Node, Src2Node, Index);
  if (!Is256Bit) {
    return Low;
  }

  OrderedNode *HighSrc1 = _VInsElement(DstSize, 16, 0, 1, Src1Node, Src1Node);
  OrderedNode *HighSrc2 = _VInsElement(DstSize, 16, 0, 1, Src2Node, Src2Node);
  OrderedNode *High = _VExtr(SanitizedDstSize, 1, HighSrc1, HighSrc2, Index);
  return _VInsElement(DstSize, 16, 1, 0, Low, High);
}

void OpDispatchBuilder::PAlignrOp(OpcodeArgs) {
  OrderedNode *Result = PALIGNROpImpl(Op, Op->Dest, Op->Src[0], Op->Src[1]);
  StoreResult(FPRClass, Op, Result, -1);
}

void OpDispatchBuilder::VPALIGNROp(OpcodeArgs) {
  OrderedNode *Result = PALIGNROpImpl(Op, Op->Src[0], Op->Src[1], Op->Src[2]);
  StoreResult(FPRClass, Op, Result, -1);
}

template<size_t ElementSize>
void OpDispatchBuilder::UCOMISxOp(OpcodeArgs) {
  OrderedNode *Src1 = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src2 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Res = _FCmp(ElementSize, Src1, Src2,
    (1 << FCMP_FLAG_EQ) |
    (1 << FCMP_FLAG_LT) |
    (1 << FCMP_FLAG_UNORDERED));

  GenerateFlags_FCMP(Op, Res, Src1, Src2);

  flagsOp = SelectionFlag::FCMP;
  flagsOpDest = Src1;
  flagsOpSrc = Src2;
  flagsOpSize = GetSrcSize(Op);
}

template
void OpDispatchBuilder::UCOMISxOp<4>(OpcodeArgs);
template
void OpDispatchBuilder::UCOMISxOp<8>(OpcodeArgs);

void OpDispatchBuilder::LDMXCSR(OpcodeArgs) {
  OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);
  // We only support the rounding mode and FTZ bit being set
  OrderedNode *RoundingMode = _Bfe(4, 3, 13, Dest);
  _SetRoundingMode(RoundingMode);
}

void OpDispatchBuilder::STMXCSR(OpcodeArgs) {
  // Default MXCSR
  OrderedNode *MXCSR = _Constant(32, 0x1F80);
  OrderedNode *RoundingMode = _GetRoundingMode();
  MXCSR = _Bfi(4, 3, 13, MXCSR, RoundingMode);

  StoreResult(GPRClass, Op, MXCSR, -1);
}

OrderedNode* OpDispatchBuilder::PACKUSOpImpl(OpcodeArgs, size_t ElementSize,
                                             OrderedNode *Src1, OrderedNode *Src2) {
  const auto Size = GetSrcSize(Op);

  OrderedNode *Res = _VSQXTUN(Size, ElementSize, Src1);
  return _VSQXTUN2(Size, ElementSize, Res, Src2);
}

template<size_t ElementSize>
void OpDispatchBuilder::PACKUSOp(OpcodeArgs) {
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Result = PACKUSOpImpl(Op, ElementSize, Dest, Src);

  StoreResult(FPRClass, Op, Result, -1);
}

template
void OpDispatchBuilder::PACKUSOp<2>(OpcodeArgs);
template
void OpDispatchBuilder::PACKUSOp<4>(OpcodeArgs);

template<size_t ElementSize>
void OpDispatchBuilder::VPACKUSOp(OpcodeArgs) {
  const auto DstSize = GetDstSize(Op);
  const auto Is128Bit = DstSize == Core::CPUState::XMM_SSE_REG_SIZE;

  OrderedNode *Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags, -1);
  OrderedNode *Result = PACKUSOpImpl(Op, ElementSize, Src1, Src2);

  if (Is128Bit) {
    Result = _VMov(16, Result);
  } else {
    // We do a little cheeky 64-bit swapping to interleave the result.
    OrderedNode* Swapped = _VInsElement(DstSize, 8, 2, 1, Result, Result);
    Result = _VInsElement(DstSize, 8, 1, 2, Swapped, Result);
  }
  StoreResult(FPRClass, Op, Result, -1);
}

template
void OpDispatchBuilder::VPACKUSOp<2>(OpcodeArgs);
template
void OpDispatchBuilder::VPACKUSOp<4>(OpcodeArgs);

OrderedNode* OpDispatchBuilder::PACKSSOpImpl(OpcodeArgs, size_t ElementSize,
                                             OrderedNode *Src1, OrderedNode *Src2) {
  const auto Size = GetSrcSize(Op);

  OrderedNode *Res = _VSQXTN(Size, ElementSize, Src1);
  return _VSQXTN2(Size, ElementSize, Res, Src2);
}

template<size_t ElementSize>
void OpDispatchBuilder::PACKSSOp(OpcodeArgs) {
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Result = PACKSSOpImpl(Op, ElementSize, Dest, Src);

  StoreResult(FPRClass, Op, Result, -1);
}

template
void OpDispatchBuilder::PACKSSOp<2>(OpcodeArgs);
template
void OpDispatchBuilder::PACKSSOp<4>(OpcodeArgs);

template<size_t ElementSize>
void OpDispatchBuilder::VPACKSSOp(OpcodeArgs) {
  const auto DstSize = GetDstSize(Op);
  const auto Is128Bit = DstSize == Core::CPUState::XMM_SSE_REG_SIZE;

  OrderedNode *Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags, -1);
  OrderedNode *Result = PACKSSOpImpl(Op, ElementSize, Src1, Src2);

  if (Is128Bit) {
    Result = _VMov(16, Result);
  } else {
    // We do a little cheeky 64-bit swapping to interleave the result.
    OrderedNode* Swapped = _VInsElement(DstSize, 8, 2, 1, Result, Result);
    Result = _VInsElement(DstSize, 8, 1, 2, Swapped, Result);
  }
  StoreResult(FPRClass, Op, Result, -1);
}

template
void OpDispatchBuilder::VPACKSSOp<2>(OpcodeArgs);
template
void OpDispatchBuilder::VPACKSSOp<4>(OpcodeArgs);

OrderedNode* OpDispatchBuilder::PMULLOpImpl(OpcodeArgs, size_t ElementSize, bool Signed,
                                            OrderedNode *Src1, OrderedNode *Src2) {
  const auto Size = GetSrcSize(Op);

  if (Size == 8) {
    if (Signed) {
      return _VSMull(16, ElementSize, Src1, Src2);
    } else {
      return _VUMull(16, ElementSize, Src1, Src2);
    }
  } else {
    const auto Is256Bit = Size == Core::CPUState::XMM_AVX_REG_SIZE;

    OrderedNode *InsSrc1 = _VInsElement(Size, ElementSize, 1, 2, Src1, Src1);
    OrderedNode *InsSrc2 = _VInsElement(Size, ElementSize, 1, 2, Src2, Src2);
    if (Is256Bit) {
      InsSrc1 = _VInsElement(Size, ElementSize, 2, 4, InsSrc1, Src1);
      InsSrc1 = _VInsElement(Size, ElementSize, 3, 6, InsSrc1, Src1);

      InsSrc2 = _VInsElement(Size, ElementSize, 2, 4, InsSrc2, Src2);
      InsSrc2 = _VInsElement(Size, ElementSize, 3, 6, InsSrc2, Src2);
    }

    if (Signed) {
      return _VSMull(Size, ElementSize, InsSrc1, InsSrc2);
    } else {
      return _VUMull(Size, ElementSize, InsSrc1, InsSrc2);
    }
  }
}

template<size_t ElementSize, bool Signed>
void OpDispatchBuilder::PMULLOp(OpcodeArgs) {
  static_assert(ElementSize == sizeof(uint32_t),
                "Currently only handles 32-bit -> 64-bit");

  OrderedNode *Src1 = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src2 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Res = PMULLOpImpl(Op, ElementSize, Signed, Src1, Src2);

  StoreResult(FPRClass, Op, Res, -1);
}

template
void OpDispatchBuilder::PMULLOp<4, false>(OpcodeArgs);
template
void OpDispatchBuilder::PMULLOp<4, true>(OpcodeArgs);

template <size_t ElementSize, bool Signed>
void OpDispatchBuilder::VPMULLOp(OpcodeArgs) {
  static_assert(ElementSize == sizeof(uint32_t),
                "Currently only handles 32-bit -> 64-bit");

  const auto DstSize = GetDstSize(Op);
  const auto Is128Bit = DstSize == Core::CPUState::XMM_SSE_REG_SIZE;

  OrderedNode *Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags, -1);
  OrderedNode *Result = PMULLOpImpl(Op, ElementSize, Signed, Src1, Src2);

  if (Is128Bit) {
    Result = _VMov(16, Result);
  }

  StoreResult(FPRClass, Op, Result, -1);
}

template
void OpDispatchBuilder::VPMULLOp<4, false>(OpcodeArgs);
template
void OpDispatchBuilder::VPMULLOp<4, true>(OpcodeArgs);

template<bool ToXMM>
void OpDispatchBuilder::MOVQ2DQ(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  // This instruction is a bit special in that if the source is MMX then it zexts to 128bit
  if constexpr (ToXMM) {
    const auto Index = Op->Dest.Data.GPR.GPR - FEXCore::X86State::REG_XMM_0;

    Src = _VMov(16, Src);
    StoreXMMRegister(Index, Src);
  }
  else {
    // This is simple, just store the result
    StoreResult(FPRClass, Op, Src, -1);
  }
}

template
void OpDispatchBuilder::MOVQ2DQ<false>(OpcodeArgs);
template
void OpDispatchBuilder::MOVQ2DQ<true>(OpcodeArgs);

OrderedNode* OpDispatchBuilder::ADDSUBPOpImpl(OpcodeArgs, size_t ElementSize,
                                              OrderedNode *Src1, OrderedNode *Src2) {
  const auto Size = GetSrcSize(Op);

  OrderedNode *ResAdd = _VFAdd(Size, ElementSize, Src1, Src2);
  OrderedNode *ResSub = _VFSub(Size, ElementSize, Src1, Src2);

  // Even elements are the sub result
  // Odd elements are the add results
  OrderedNode *UnzipSub = _VUnZip(Size, ElementSize, ResSub, ResSub);
  OrderedNode *UnzipAdd = _VUnZip2(Size, ElementSize, ResAdd, ResAdd);

  return _VZip(Size, ElementSize, UnzipSub, UnzipAdd);
}

template<size_t ElementSize>
void OpDispatchBuilder::ADDSUBPOp(OpcodeArgs) {
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Result = ADDSUBPOpImpl(Op, ElementSize, Dest, Src);

  StoreResult(FPRClass, Op, Result, -1);
}

template
void OpDispatchBuilder::ADDSUBPOp<4>(OpcodeArgs);
template
void OpDispatchBuilder::ADDSUBPOp<8>(OpcodeArgs);

template<size_t ElementSize>
void OpDispatchBuilder::VADDSUBPOp(OpcodeArgs) {
  const auto DstSize = GetDstSize(Op);
  const auto Is128Bit = DstSize == Core::CPUState::XMM_SSE_REG_SIZE;

  OrderedNode *Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags, -1);
  OrderedNode *Result = ADDSUBPOpImpl(Op, ElementSize, Src1, Src2);

  if (Is128Bit) {
    Result = _VMov(16, Result);
  }
  StoreResult(FPRClass, Op, Result, -1);
}

template
void OpDispatchBuilder::VADDSUBPOp<4>(OpcodeArgs);
template
void OpDispatchBuilder::VADDSUBPOp<8>(OpcodeArgs);

void OpDispatchBuilder::PFNACCOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);

  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  OrderedNode *ResSubSrc{};
  OrderedNode *ResSubDest{};
  auto UpperSubDest = _VDupElement(Size, 4, Dest, 1);
  auto UpperSubSrc = _VDupElement(Size, 4, Src, 1);

  ResSubDest = _VFSub(4, 4, Dest, UpperSubDest);
  ResSubSrc = _VFSub(4, 4, Src, UpperSubSrc);

  auto Result = _VInsElement(8, 4, 1, 0, ResSubDest, ResSubSrc);

  StoreResult(FPRClass, Op, Result, -1);
}

void OpDispatchBuilder::PFPNACCOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);

  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  OrderedNode *ResAdd{};
  OrderedNode *ResSub{};
  auto UpperSubDest = _VDupElement(Size, 4, Dest, 1);

  ResSub = _VFSub(4, 4, Dest, UpperSubDest);
  ResAdd = _VFAddP(Size, 4, Src, Src);

  auto Result = _VInsElement(8, 4, 1, 0, ResSub, ResAdd);

  StoreResult(FPRClass, Op, Result, -1);
}

void OpDispatchBuilder::PSWAPDOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  auto Result = _VRev64(Size, 4, Src);
  StoreResult(FPRClass, Op, Result, -1);
}

void OpDispatchBuilder::PI2FWOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  size_t Size = GetDstSize(Op);

  // We now need to transpose the lower 16-bits of each element together
  // Only needing to move the upper element down in this case
  Src = _VInsElement(Size, 2, 1, 2, Src, Src);

  // Now we need to sign extend the 16bit value to 32-bit
  Src = _VSXTL(Size, 2, Src);

  // int32_t to float
  Src = _Vector_SToF(Size, 4, Src);

  StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Src, Size, -1);
}

void OpDispatchBuilder::PF2IWOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  size_t Size = GetDstSize(Op);

  // Float to int32_t
  Src = _Vector_FToZS(Size, 4, Src);

  // We now need to transpose the lower 16-bits of each element together
  // Only needing to move the upper element down in this case
  Src = _VInsElement(Size, 2, 1, 2, Src, Src);

  // Now we need to sign extend the 16bit value to 32-bit
  Src = _VSXTL(Size, 2, Src);
  StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Src, Size, -1);
}

void OpDispatchBuilder::PMULHRWOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);

  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  OrderedNode *Res{};

  // Implementation is more efficient for 8byte registers
  // Multiplies 4 16bit values in to 4 32bit values
  Res = _VSMull(Size * 2, 2, Dest, Src);

  OrderedNode *VConstant = _VDupFromGPR(16, 8, _Constant(0x0000'8000'0000'8000ULL));

  Res = _VAdd(Size * 2, 4, Res, VConstant);

  // Now shift and narrow to convert 32-bit values to 16bit, storing the top 16bits
  Res = _VUShrNI(Size * 2, 4, Res, 16);

  StoreResult(FPRClass, Op, Res, -1);
}

template<uint8_t CompType>
void OpDispatchBuilder::VPFCMPOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Dest = LoadSource_WithOpSize(FPRClass, Op, Op->Dest, GetDstSize(Op), Op->Flags, -1);

  OrderedNode *Result{};
  // This maps 1:1 to an AArch64 NEON Op
  //auto ALUOp = _VCMPGT(Size, 4, Dest, Src);
  switch (CompType) {
    case 0x00: // EQ
      Result = _VFCMPEQ(Size, 4, Dest, Src);
    break;
    case 0x01: // GE(Swapped operand)
      Result = _VFCMPLE(Size, 4, Src, Dest);
    break;
    case 0x02: // GT
      Result = _VFCMPGT(Size, 4, Dest, Src);
    break;
    default:
      LOGMAN_MSG_A_FMT("Unknown Comparison type: {}", CompType);
    break;
  }

  StoreResult(FPRClass, Op, Result, -1);
}

template
void OpDispatchBuilder::VPFCMPOp<0>(OpcodeArgs);
template
void OpDispatchBuilder::VPFCMPOp<1>(OpcodeArgs);
template
void OpDispatchBuilder::VPFCMPOp<2>(OpcodeArgs);

OrderedNode* OpDispatchBuilder::PMADDWDOpImpl(OpcodeArgs, const X86Tables::DecodedOperand& Src1,
                                              const X86Tables::DecodedOperand& Src2) {
  // This is a pretty curious operation
  // Does two MADD operations across 4 16bit signed integers and accumulates to 32bit integers in the destination
  //
  // x86 PMADDWD: xmm1, xmm2
  //              xmm1[31:0]  = (xmm1[15:0] * xmm2[15:0]) + (xmm1[31:16] * xmm2[31:16])
  //              xmm1[63:32] = (xmm1[47:32] * xmm2[47:32]) + (xmm1[63:48] * xmm2[63:48])
  //              etc.. for larger registers

  auto Size = GetSrcSize(Op);

  OrderedNode *Src1Node = LoadSource(FPRClass, Op, Src1, Op->Flags, -1);
  OrderedNode *Src2Node = LoadSource(FPRClass, Op, Src2, Op->Flags, -1);

  if (Size == 8) {
    Size <<= 1;
  }

  auto Src1_L = _VSXTL(Size, 2, Src1Node);  // [15:0 ], [31:16], [32:47 ], [63:48  ]
  auto Src1_H = _VSXTL2(Size, 2, Src1Node); // [79:64], [95:80], [111:96], [127:112]

  auto Src2_L = _VSXTL(Size, 2, Src2Node);  // [15:0 ], [31:16], [32:47 ], [63:48  ]
  auto Src2_H = _VSXTL2(Size, 2, Src2Node); // [79:64], [95:80], [111:96], [127:112]

  auto Res_L = _VSMul(Size, 4, Src1_L, Src2_L); // [15:0 ], [31:16], [32:47 ], [63:48  ] : Original elements
  auto Res_H = _VSMul(Size, 4, Src1_H, Src2_H); // [79:64], [95:80], [111:96], [127:112] : Original elements

  // [15:0 ] + [31:16], [32:47 ] + [63:48  ], [79:64] + [95:80], [111:96] + [127:112]
  return _VAddP(Size, 4, Res_L, Res_H);
}

void OpDispatchBuilder::PMADDWD(OpcodeArgs) {
  OrderedNode *Result = PMADDWDOpImpl(Op, Op->Dest, Op->Src[0]);
  StoreResult(FPRClass, Op, Result, -1);
}

void OpDispatchBuilder::VPMADDWDOp(OpcodeArgs) {
  OrderedNode *Result = PMADDWDOpImpl(Op, Op->Src[0], Op->Src[1]);
  StoreResult(FPRClass, Op, Result, -1);
}

void OpDispatchBuilder::PMADDUBSW(OpcodeArgs) {
  // This is a pretty curious operation
  // Does four MADD operations across 8 8bit signed and unsigned integers and accumulates to 16bit integers in the destination WITH saturation
  //
  // x86 PMADDUBSW: mm1, mm2
  //    mm1[15:0]  = SaturateSigned16(((s8)mm2[15:8]  * (u8)mm1[15:8])  + ((s8)mm2[7:0]   * (u8)mm1[7:0]))
  //    mm1[31:16] = SaturateSigned16(((s8)mm2[31:24] * (u8)mm1[31:24]) + ((s8)mm2[23:16] * (u8)mm1[23:16]))
  //    mm1[47:32] = SaturateSigned16(((s8)mm2[47:40] * (u8)mm1[47:40]) + ((s8)mm2[39:32] * (u8)mm1[39:32]))
  //    mm1[63:48] = SaturateSigned16(((s8)mm2[63:56] * (u8)mm1[63:56]) + ((s8)mm2[55:48] * (u8)mm1[55:48]))
  // Extends to larger registers
  auto Size = GetSrcSize(Op);

  OrderedNode *Src1 = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src2 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  if (Size == 8) {
    // 64bit is more efficient

    // Src1 is unsigned
    auto Src1_16b = _VUXTL(Size * 2, 1, Src1);  // [7:0 ], [15:8], [23:16], [31:24], [39:32], [47:40], [55:48], [63:56]

    // Src2 is signed
    auto Src2_16b = _VSXTL(Size * 2, 1, Src2);  // [7:0 ], [15:8], [23:16], [31:24], [39:32], [47:40], [55:48], [63:56]

    auto ResMul_L = _VSMull(Size * 2, 2, Src1_16b, Src2_16b);
    auto ResMul_H = _VSMull2(Size * 2, 2, Src1_16b, Src2_16b);

    // Now add pairwise across the vector
    auto ResAdd = _VAddP(Size * 2, 4, ResMul_L, ResMul_H);

    // Add saturate back down to 16bit
    OrderedNode *Res = _VSQXTN(Size * 2, 4, ResAdd);
    StoreResult(FPRClass, Op, Res, -1);
  }
  else {
    // Src1 is unsigned
    auto Src1_16b_L = _VUXTL(Size, 1, Src1);  // [7:0 ], [15:8], [23:16], [31:24], [39:32], [47:40], [55:48], [63:56]
    auto Src1_16b_H = _VUXTL2(Size, 1, Src1);  // Offset to +64bits [7:0 ], [15:8], [23:16], [31:24], [39:32], [47:40], [55:48], [63:56]

    // Src2 is signed
    auto Src2_16b_L = _VSXTL(Size, 1, Src2);  // [7:0 ], [15:8], [23:16], [31:24], [39:32], [47:40], [55:48], [63:56]
    auto Src2_16b_H = _VSXTL2(Size, 1, Src2);  // Offset to +64bits [7:0 ], [15:8], [23:16], [31:24], [39:32], [47:40], [55:48], [63:56]

    auto ResMul_L   = _VSMull(Size, 2, Src1_16b_L, Src2_16b_L);
    auto ResMul_L_H = _VSMull2(Size, 2, Src1_16b_L, Src2_16b_L);

    auto ResMul_H   = _VSMull(Size, 2, Src1_16b_H, Src2_16b_H);
    auto ResMul_H_H = _VSMull2(Size, 2, Src1_16b_H, Src2_16b_H);

    // Now add pairwise across the vector
    auto ResAdd_L = _VAddP(Size, 4, ResMul_L, ResMul_L_H);
    auto ResAdd_H = _VAddP(Size, 4, ResMul_H, ResMul_H_H);

    // Add saturate back down to 16bit
    OrderedNode *Res = _VSQXTN(Size, 4, ResAdd_L);
    Res = _VSQXTN2(Size, 4, Res, ResAdd_H);

    StoreResult(FPRClass, Op, Res, -1);
  }
}

OrderedNode* OpDispatchBuilder::PMULHWOpImpl(OpcodeArgs, bool Signed,
                                             OrderedNode *Src1, OrderedNode *Src2) {
  const auto Size = GetSrcSize(Op);
  OrderedNode *Res{};

  if (Size == 8) {
    // Implementation is more efficient for 8byte registers
    if (Signed) {
      Res = _VSMull(Size * 2, 2, Src1, Src2);
    } else {
      Res = _VUMull(Size * 2, 2, Src1, Src2);
    }

    return _VUShrNI(Size * 2, 4, Res, 16);
  } else {
    // 128-bit and 256-bit is less efficient
    OrderedNode *ResultLow;
    OrderedNode *ResultHigh;
    if (Signed) {
      ResultLow = _VSMull(Size, 2, Src1, Src2);
      ResultHigh = _VSMull2(Size, 2, Src1, Src2);
    } else {
      ResultLow = _VUMull(Size, 2, Src1, Src2);
      ResultHigh = _VUMull2(Size, 2, Src1, Src2);
    }

    // Combine the results
    Res = _VUShrNI(Size, 4, ResultLow, 16);
    return _VUShrNI2(Size, 4, Res, ResultHigh, 16);
  }
}

template<bool Signed>
void OpDispatchBuilder::PMULHW(OpcodeArgs) {
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Result = PMULHWOpImpl(Op, Signed, Dest, Src);

  StoreResult(FPRClass, Op, Result, -1);
}

template
void OpDispatchBuilder::PMULHW<false>(OpcodeArgs);
template
void OpDispatchBuilder::PMULHW<true>(OpcodeArgs);

template <bool Signed>
void OpDispatchBuilder::VPMULHWOp(OpcodeArgs) {
  const auto DstSize = GetDstSize(Op);
  const auto Is128Bit = DstSize == Core::CPUState::XMM_SSE_REG_SIZE;

  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags, -1);
  OrderedNode *Result = PMULHWOpImpl(Op, Signed, Dest, Src);

  if (Is128Bit) {
    Result = _VMov(16, Result);
  }
  StoreResult(FPRClass, Op, Result, -1);
}

template
void OpDispatchBuilder::VPMULHWOp<false>(OpcodeArgs);
template
void OpDispatchBuilder::VPMULHWOp<true>(OpcodeArgs);

OrderedNode* OpDispatchBuilder::PMULHRSWOpImpl(OpcodeArgs, OrderedNode *Src1, OrderedNode *Src2) {
  const auto Size = GetSrcSize(Op);

  OrderedNode *Res{};
  if (Size == 8) {
    // Implementation is more efficient for 8byte registers
    Res = _VSMull(Size * 2, 2, Src1, Src2);
    Res = _VSShrI(Size * 2, 4, Res, 14);
    auto OneVector = _VectorImm(Size * 2, 4, 1);
    Res = _VAdd(Size * 2, 4, Res, OneVector);
    return _VUShrNI(Size * 2, 4, Res, 1);
  } else {
    // 128-bit and 256-bit are less efficient
    OrderedNode *ResultLow;
    OrderedNode *ResultHigh;

    ResultLow = _VSMull(Size, 2, Src1, Src2);
    ResultHigh = _VSMull2(Size, 2, Src1, Src2);

    ResultLow = _VSShrI(Size, 4, ResultLow, 14);
    ResultHigh = _VSShrI(Size, 4, ResultHigh, 14);
    auto OneVector = _VectorImm(Size, 4, 1);

    ResultLow = _VAdd(Size, 4, ResultLow, OneVector);
    ResultHigh = _VAdd(Size, 4, ResultHigh, OneVector);

    // Combine the results
    Res = _VUShrNI(Size, 4, ResultLow, 1);
    return _VUShrNI2(Size, 4, Res, ResultHigh, 1);
  }
}

void OpDispatchBuilder::PMULHRSW(OpcodeArgs) {
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Result = PMULHRSWOpImpl(Op, Dest, Src);

  StoreResult(FPRClass, Op, Result, -1);
}

void OpDispatchBuilder::VPMULHRSWOp(OpcodeArgs) {
  const auto DstSize = GetDstSize(Op);
  const auto Is128Bit = DstSize == Core::CPUState::XMM_SSE_REG_SIZE;

  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags, -1);
  OrderedNode *Result = PMULHRSWOpImpl(Op, Dest, Src);

  if (Is128Bit) {
    Result = _VMov(16, Result);
  }
  StoreResult(FPRClass, Op, Result, -1);
}

template<size_t ElementSize>
void OpDispatchBuilder::HSUBP(OpcodeArgs) {
  auto Size = GetSrcSize(Op);
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  // This is a bit complicated since AArch64 doesn't support a pairwise subtract
  auto Dest_Neg = _VFNeg(Size, ElementSize, Dest);
  auto Src_Neg = _VFNeg(Size, ElementSize, Src);

  // Now we need to swizzle the values
  OrderedNode *Swizzle_Dest = Dest;
  OrderedNode *Swizzle_Src = Src;

  if constexpr (ElementSize == 4) {
    Swizzle_Dest = _VInsElement(Size, ElementSize, 1, 1, Swizzle_Dest, Dest_Neg);
    Swizzle_Dest = _VInsElement(Size, ElementSize, 3, 3, Swizzle_Dest, Dest_Neg);

    Swizzle_Src = _VInsElement(Size, ElementSize, 1, 1, Swizzle_Src, Src_Neg);
    Swizzle_Src = _VInsElement(Size, ElementSize, 3, 3, Swizzle_Src, Src_Neg);
  }
  else {
    Swizzle_Dest = _VInsElement(Size, ElementSize, 1, 1, Swizzle_Dest, Dest_Neg);
    Swizzle_Src = _VInsElement(Size, ElementSize, 1, 1, Swizzle_Src, Src_Neg);
  }

  OrderedNode *Res = _VFAddP(Size, ElementSize, Swizzle_Dest, Swizzle_Src);
  StoreResult(FPRClass, Op, Res, -1);
}

template
void OpDispatchBuilder::HSUBP<4>(OpcodeArgs);
template
void OpDispatchBuilder::HSUBP<8>(OpcodeArgs);

OrderedNode* OpDispatchBuilder::PHSUBOpImpl(OpcodeArgs, const X86Tables::DecodedOperand& Src1,
                                            const X86Tables::DecodedOperand& Src2, size_t ElementSize) {
  const auto Size = GetSrcSize(Op);

  OrderedNode *Src1V = LoadSource(FPRClass, Op, Src1, Op->Flags, -1);
  OrderedNode *Src2V = LoadSource(FPRClass, Op, Src2, Op->Flags, -1);

  // This is a bit complicated since AArch64 doesn't support a pairwise subtract
  OrderedNode *Src1_Neg = _VNeg(Size, ElementSize, Src1V);
  OrderedNode *Src2_Neg = _VNeg(Size, ElementSize, Src2V);

  // Now we need to swizzle the values
  OrderedNode *Swizzle_Src1{};
  OrderedNode *Swizzle_Src2{};
  if (Size == 8 && ElementSize == 4) {
    Swizzle_Src1 = _VInsElement(Size, ElementSize, 1, 1, Src1V, Src1_Neg);
    Swizzle_Src2 = _VInsElement(Size, ElementSize, 1, 1, Src2V, Src2_Neg);
  } else {
    OrderedNode *UzpSrc1 = _VUnZip(Size, ElementSize, Src1V, Src1V);
    OrderedNode *UzpSrc2 = _VUnZip(Size, ElementSize, Src2V, Src2V);

    OrderedNode *UzpSrc1Neg = _VUnZip2(Size, ElementSize, Src1_Neg, Src1_Neg);
    OrderedNode *UzpSrc2Neg = _VUnZip2(Size, ElementSize, Src2_Neg, Src2_Neg);

    Swizzle_Src1 = _VZip(Size, ElementSize, UzpSrc1, UzpSrc1Neg);
    Swizzle_Src2 = _VZip(Size, ElementSize, UzpSrc2, UzpSrc2Neg);
  }

  return _VAddP(Size, ElementSize, Swizzle_Src1, Swizzle_Src2);
}

template<size_t ElementSize>
void OpDispatchBuilder::PHSUB(OpcodeArgs) {
  OrderedNode *Result = PHSUBOpImpl(Op, Op->Dest, Op->Src[0], ElementSize);
  StoreResult(FPRClass, Op, Result, -1);
}

template
void OpDispatchBuilder::PHSUB<2>(OpcodeArgs);
template
void OpDispatchBuilder::PHSUB<4>(OpcodeArgs);

template <size_t ElementSize>
void OpDispatchBuilder::VPHSUBOp(OpcodeArgs) {
  const auto DstSize = GetDstSize(Op);
  const auto Is128Bit = DstSize == Core::CPUState::XMM_SSE_REG_SIZE;

  OrderedNode *Result = PHSUBOpImpl(Op, Op->Src[0], Op->Src[1], ElementSize);
  if (Is128Bit) {
    Result = _VMov(16, Result);
  } else {
    OrderedNode *Inserted = _VInsElement(DstSize, 8, 1, 2, Result, Result);
    Result = _VInsElement(DstSize, 8, 2, 1, Inserted, Result);
  }
  StoreResult(FPRClass, Op, Result, -1);
}

template
void OpDispatchBuilder::VPHSUBOp<2>(OpcodeArgs);
template
void OpDispatchBuilder::VPHSUBOp<4>(OpcodeArgs);

void OpDispatchBuilder::PHADDS(OpcodeArgs) {
  auto Size = GetSrcSize(Op);
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  if (Size == 8) {
    // Implementation is more efficient for 8byte registers
    auto Dest_Larger = _VSXTL(Size * 2, 2, Dest);
    auto Src_Larger = _VSXTL(Size * 2, 2, Src);

    OrderedNode *AddRes = _VAddP(Size * 2, 4, Dest_Larger, Src_Larger);

    // Saturate back down to the result
    OrderedNode *Res = _VSQXTN(Size * 2, 4, AddRes);
    StoreResult(FPRClass, Op, Res, -1);
  }
  else {
    auto Dest_Larger = _VSXTL(Size, 2, Dest);
    auto Dest_Larger_H = _VSXTL2(Size, 2, Dest);

    auto Src_Larger = _VSXTL(Size, 2, Src);
    auto Src_Larger_H = _VSXTL2(Size, 2, Src);

    OrderedNode *AddRes_L = _VAddP(Size, 4, Dest_Larger, Dest_Larger_H);
    OrderedNode *AddRes_H = _VAddP(Size, 4, Src_Larger, Src_Larger_H);

    // Saturate back down to the result
    OrderedNode *Res = _VSQXTN(Size, 4, AddRes_L);
    Res = _VSQXTN2(Size, 4, Res, AddRes_H);

    StoreResult(FPRClass, Op, Res, -1);
  }
}

void OpDispatchBuilder::PHSUBS(OpcodeArgs) {
  auto Size = GetSrcSize(Op);
  uint8_t ElementSize = 2;
  uint8_t NumElements = Size / ElementSize;

  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  // This is a bit complicated since AArch64 doesn't support a pairwise subtract
  auto Dest_Neg = _VNeg(Size, ElementSize, Dest);
  auto Src_Neg = _VNeg(Size, ElementSize, Src);

  // Now we need to swizzle the values
  OrderedNode *Swizzle_Dest = Dest;
  OrderedNode *Swizzle_Src = Src;

  // Odd elements turn in to negated elements
  for (size_t i = 1; i < NumElements; i += 2) {
    Swizzle_Dest = _VInsElement(Size, ElementSize, i, i, Swizzle_Dest, Dest_Neg);
    Swizzle_Src = _VInsElement(Size, ElementSize, i, i, Swizzle_Src, Src_Neg);
  }

  Dest = Swizzle_Dest;
  Src = Swizzle_Src;

  if (Size == 8) {
    // Implementation is more efficient for 8byte registers
    auto Dest_Larger = _VSXTL(Size * 2, 2, Dest);
    auto Src_Larger = _VSXTL(Size * 2, 2, Src);

    OrderedNode *AddRes = _VAddP(Size * 2, 4, Dest_Larger, Src_Larger);

    // Saturate back down to the result
    OrderedNode *Res = _VSQXTN(Size * 2, 4, AddRes);
    StoreResult(FPRClass, Op, Res, -1);
  }
  else {
    auto Dest_Larger = _VSXTL(Size, 2, Dest);
    auto Dest_Larger_H = _VSXTL2(Size, 2, Dest);

    auto Src_Larger = _VSXTL(Size, 2, Src);
    auto Src_Larger_H = _VSXTL2(Size, 2, Src);

    OrderedNode *AddRes_L = _VAddP(Size, 4, Dest_Larger, Dest_Larger_H);
    OrderedNode *AddRes_H = _VAddP(Size, 4, Src_Larger, Src_Larger_H);

    // Saturate back down to the result
    OrderedNode *Res = _VSQXTN(Size, 4, AddRes_L);
    Res = _VSQXTN2(Size, 4, Res, AddRes_H);

    StoreResult(FPRClass, Op, Res, -1);
  }
}

void OpDispatchBuilder::PSADBW(OpcodeArgs) {
  // The documentation is actually incorrect in how this instruction operates
  // It strongly implies that the `abs(dest[i] - src[i])` operates in 8bit space
  // but it actually operates in more than 8bit space
  // This can be seen with `abs(0 - 0xFF)` returning a different result depending
  // on bit length
  auto Size = GetSrcSize(Op);

  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  OrderedNode *Result{};

  if (Size == 8) {
    Dest = _VUXTL(Size*2, 1, Dest);
    Src = _VUXTL(Size*2, 1, Src);

    OrderedNode *SubResult = _VSub(Size*2, 2, Dest, Src);
    OrderedNode *AbsResult = _VAbs(Size*2, 2, SubResult);

    // Now vector-wide add the results for each
    Result = _VAddV(Size * 2, 2, AbsResult);
  }
  else {
    OrderedNode *Dest_Low = _VUXTL(Size, 1, Dest);
    OrderedNode *Dest_High = _VUXTL2(Size, 1, Dest);

    OrderedNode *Src_Low = _VUXTL(Size, 1, Src);
    OrderedNode *Src_High = _VUXTL2(Size, 1, Src);

    OrderedNode *SubResult_Low = _VSub(Size, 2, Dest_Low, Src_Low);
    OrderedNode *SubResult_High = _VSub(Size, 2, Dest_High, Src_High);

    OrderedNode *AbsResult_Low = _VAbs(Size, 2, SubResult_Low);
    OrderedNode *AbsResult_High = _VAbs(Size, 2, SubResult_High);

    // Now vector pairwise add all four of these
    OrderedNode * Result_Low = _VAddV(Size, 2, AbsResult_Low);
    OrderedNode * Result_High = _VAddV(Size, 2, AbsResult_High);

    Result = _VInsElement(Size, 8, 1, 0, Result_Low, Result_High);
  }

  StoreResult(FPRClass, Op, Result, -1);
}

OrderedNode* OpDispatchBuilder::ExtendVectorElementsImpl(OpcodeArgs, size_t ElementSize,
                                                         size_t DstElementSize, bool Signed) {
  const auto DstSize = GetDstSize(Op);

  const auto GetSrc = [&] {
    if (Op->Src[0].IsGPR()) {
      return LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], DstSize, Op->Flags, -1);
    } else {
      // For memory operands the 256-bit variant loads twice the size specified in the table.
      const auto Is256Bit = DstSize == Core::CPUState::XMM_AVX_REG_SIZE;
      const auto SrcSize = GetSrcSize(Op);
      const auto LoadSize = Is256Bit ? SrcSize * 2 : SrcSize;

      return LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], LoadSize, Op->Flags, -1);
    }
  };

  OrderedNode *Src = GetSrc();
  OrderedNode *Result{Src};

  for (size_t CurrentElementSize = ElementSize;
       CurrentElementSize != DstElementSize;
       CurrentElementSize <<= 1) {
    if (Signed) {
      Result = _VSXTL(DstSize, CurrentElementSize, Result);
    } else {
      Result = _VUXTL(DstSize, CurrentElementSize, Result);
    }
  }

  return Result;
}

template <size_t ElementSize, size_t DstElementSize, bool Signed>
void OpDispatchBuilder::ExtendVectorElements(OpcodeArgs) {
  OrderedNode *Result = ExtendVectorElementsImpl(Op, ElementSize, DstElementSize, Signed);
  StoreResult(FPRClass, Op, Result, -1);
}

template
void OpDispatchBuilder::ExtendVectorElements<1, 2, false>(OpcodeArgs);
template
void OpDispatchBuilder::ExtendVectorElements<1, 4, false>(OpcodeArgs);
template
void OpDispatchBuilder::ExtendVectorElements<1, 8, false>(OpcodeArgs);
template
void OpDispatchBuilder::ExtendVectorElements<2, 4, false>(OpcodeArgs);
template
void OpDispatchBuilder::ExtendVectorElements<2, 8, false>(OpcodeArgs);
template
void OpDispatchBuilder::ExtendVectorElements<4, 8, false>(OpcodeArgs);

template
void OpDispatchBuilder::ExtendVectorElements<1, 2, true>(OpcodeArgs);
template
void OpDispatchBuilder::ExtendVectorElements<1, 4, true>(OpcodeArgs);
template
void OpDispatchBuilder::ExtendVectorElements<1, 8, true>(OpcodeArgs);
template
void OpDispatchBuilder::ExtendVectorElements<2, 4, true>(OpcodeArgs);
template
void OpDispatchBuilder::ExtendVectorElements<2, 8, true>(OpcodeArgs);
template
void OpDispatchBuilder::ExtendVectorElements<4, 8, true>(OpcodeArgs);

template <size_t ElementSize, size_t DstElementSize, bool Signed>
void OpDispatchBuilder::AVXExtendVectorElements(OpcodeArgs) {
  const auto DstSize = GetDstSize(Op);
  const auto Is128Bit = DstSize == Core::CPUState::XMM_SSE_REG_SIZE;

  OrderedNode *Result = ExtendVectorElementsImpl(Op, ElementSize, DstElementSize, Signed);

  if (Is128Bit) {
    Result = _VMov(16, Result);
  }
  StoreResult(FPRClass, Op, Result, -1);
}

template
void OpDispatchBuilder::AVXExtendVectorElements<1, 2, false>(OpcodeArgs);
template
void OpDispatchBuilder::AVXExtendVectorElements<1, 4, false>(OpcodeArgs);
template
void OpDispatchBuilder::AVXExtendVectorElements<1, 8, false>(OpcodeArgs);
template
void OpDispatchBuilder::AVXExtendVectorElements<2, 4, false>(OpcodeArgs);
template
void OpDispatchBuilder::AVXExtendVectorElements<2, 8, false>(OpcodeArgs);
template
void OpDispatchBuilder::AVXExtendVectorElements<4, 8, false>(OpcodeArgs);

template
void OpDispatchBuilder::AVXExtendVectorElements<1, 2, true>(OpcodeArgs);
template
void OpDispatchBuilder::AVXExtendVectorElements<1, 4, true>(OpcodeArgs);
template
void OpDispatchBuilder::AVXExtendVectorElements<1, 8, true>(OpcodeArgs);
template
void OpDispatchBuilder::AVXExtendVectorElements<2, 4, true>(OpcodeArgs);
template
void OpDispatchBuilder::AVXExtendVectorElements<2, 8, true>(OpcodeArgs);
template
void OpDispatchBuilder::AVXExtendVectorElements<4, 8, true>(OpcodeArgs);

OrderedNode* OpDispatchBuilder::VectorRoundImpl(OpcodeArgs, size_t ElementSize,
                                                OrderedNode *Src, uint64_t Mode) {
  const auto Size = GetDstSize(Op);
  const uint64_t RoundControlSource = (Mode >> 2) & 1;
  uint64_t RoundControl = Mode & 0b11;

  if (RoundControlSource) {
    RoundControl = 0; // MXCSR
  }

  static constexpr std::array SourceModes = {
    FEXCore::IR::Round_Nearest,
    FEXCore::IR::Round_Negative_Infinity,
    FEXCore::IR::Round_Positive_Infinity,
    FEXCore::IR::Round_Towards_Zero,
    FEXCore::IR::Round_Host,
  };

  return _Vector_FToI(Size, ElementSize, Src, SourceModes[(RoundControlSource << 2) | RoundControl]);
}

template<size_t ElementSize, bool Scalar>
void OpDispatchBuilder::VectorRound(OpcodeArgs) {
  const auto Size = GetDstSize(Op);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  LOGMAN_THROW_A_FMT(Op->Src[1].IsLiteral(), "Src1 needs to be literal here");
  const uint64_t Mode = Op->Src[1].Data.Literal.Value;

  Src = VectorRoundImpl(Op, ElementSize, Src, Mode);

  if constexpr (Scalar) {
    // Insert the lower bits
    OrderedNode *Dest = LoadSource_WithOpSize(FPRClass, Op, Op->Dest, Size, Op->Flags, -1);
    auto Result = _VInsElement(Size, ElementSize, 0, 0, Dest, Src);
    StoreResult(FPRClass, Op, Result, -1);
  } else {
    StoreResult(FPRClass, Op, Src, -1);
  }
}

template
void OpDispatchBuilder::VectorRound<4, false>(OpcodeArgs);
template
void OpDispatchBuilder::VectorRound<8, false>(OpcodeArgs);

template
void OpDispatchBuilder::VectorRound<4, true>(OpcodeArgs);
template
void OpDispatchBuilder::VectorRound<8, true>(OpcodeArgs);

template <size_t ElementSize, bool Scalar>
void OpDispatchBuilder::AVXVectorRound(OpcodeArgs) {
  const auto GetMode = [&] {
    if constexpr (Scalar) {
      LOGMAN_THROW_A_FMT(Op->Src[2].IsLiteral(), "Src2 needs to be literal here");
      return Op->Src[2].Data.Literal.Value;
    } else {
      LOGMAN_THROW_A_FMT(Op->Src[1].IsLiteral(), "Src1 needs to be literal here");
      return Op->Src[1].Data.Literal.Value;
    }
  };

  const auto GetSrc = [&] {
    if constexpr (Scalar) {
      return LoadSource(FPRClass, Op, Op->Src[1], Op->Flags, -1);
    } else {
      return LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
    }
  };

  const auto Size = GetDstSize(Op);
  const auto Is128Bit = Size == Core::CPUState::XMM_SSE_REG_SIZE;

  OrderedNode *Src = GetSrc();
  OrderedNode *Result = VectorRoundImpl(Op, ElementSize, Src, GetMode());

  if constexpr (Scalar) {
    // Insert the lower bits
    OrderedNode *Dest = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], Size, Op->Flags, -1);
    Result = _VInsElement(Size, ElementSize, 0, 0, Dest, Result);
  }
  if (Is128Bit) {
    Result = _VMov(16, Result);
  }

  StoreResult(FPRClass, Op, Result, -1);
}

template
void OpDispatchBuilder::AVXVectorRound<4, false>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorRound<8, false>(OpcodeArgs);

template
void OpDispatchBuilder::AVXVectorRound<4, true>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorRound<8, true>(OpcodeArgs);

template<size_t ElementSize>
void OpDispatchBuilder::VectorBlend(OpcodeArgs) {
  LOGMAN_THROW_A_FMT(Op->Src[1].IsLiteral(), "Src1 needs to be literal here");
  uint8_t Select = Op->Src[1].Data.Literal.Value;

  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  for (size_t i = 0; i < (16 / ElementSize); ++i) {
    if (Select & (1 << i)) {
      // This could be optimized if it becomes costly
      Dest = _VInsElement(16, ElementSize, i, i, Dest, Src);
    }
  }
  StoreResult(FPRClass, Op, Dest, -1);
}

template
void OpDispatchBuilder::VectorBlend<2>(OpcodeArgs);
template
void OpDispatchBuilder::VectorBlend<4>(OpcodeArgs);
template
void OpDispatchBuilder::VectorBlend<8>(OpcodeArgs);

template<size_t ElementSize>
void OpDispatchBuilder::VectorVariableBlend(OpcodeArgs) {
  auto Size = GetSrcSize(Op);

  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  // The mask is hardcoded to be xmm0 in this instruction
  auto Mask = LoadXMMRegister(0);

  // Each element is selected by the high bit of that element size
  // Dest[ElementIdx] = Xmm0[ElementIndex][HighBit] ? Src : Dest;
  //
  // To emulate this on AArch64
  // Arithmetic shift right by the element size, then use BSL to select the registers
  Mask = _VSShrI(Size, ElementSize, Mask, (ElementSize * 8) - 1);

  auto Result = _VBSL(Size, Mask, Src, Dest);

  StoreResult(FPRClass, Op, Result, -1);
}
template
void OpDispatchBuilder::VectorVariableBlend<1>(OpcodeArgs);
template
void OpDispatchBuilder::VectorVariableBlend<4>(OpcodeArgs);
template
void OpDispatchBuilder::VectorVariableBlend<8>(OpcodeArgs);

template <size_t ElementSize>
void OpDispatchBuilder::AVXVectorVariableBlend(OpcodeArgs) {
  const auto SrcSize = GetSrcSize(Op);
  constexpr auto ElementSizeBits = ElementSize * 8;

  OrderedNode *Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags, -1);

  LOGMAN_THROW_A_FMT(Op->Src[2].IsLiteral(), "Src[2] needs to be literal here");
  const auto Src3Selector = Op->Src[2].Data.Literal.Value;

  // Mask register is encoded within bits [7:4] of the selector
  OrderedNode *Mask = LoadXMMRegister((Src3Selector >> 4) & 0b1111);

  OrderedNode *Shifted = _VSShrI(SrcSize, ElementSize, Mask, ElementSizeBits - 1);
  OrderedNode *Result = _VBSL(SrcSize, Shifted, Src2, Src1);
  StoreResult(FPRClass, Op, Result, -1);
}
template
void OpDispatchBuilder::AVXVectorVariableBlend<1>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorVariableBlend<4>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorVariableBlend<8>(OpcodeArgs);

void OpDispatchBuilder::PTestOp(OpcodeArgs) {
  // Invalidate deferred flags early
  InvalidateDeferredFlags();

  auto Size = GetSrcSize(Op);

  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  OrderedNode *Test1 = _VAnd(Size, 1, Dest, Src);
  OrderedNode *Test2 = _VBic(Size, 1, Src, Dest);

  Test1 = _VPopcount(Size, 1, Test1);
  Test2 = _VPopcount(Size, 1, Test2);

  // Element size doesn't matter here
  // x86-64 doesn't support a horizontal byte add though
  Test1 = _VAddV(Size, 2, Test1);
  Test2 = _VAddV(Size, 2, Test2);

  Test1 = _VExtractToGPR(16, 2, Test1, 0);
  Test2 = _VExtractToGPR(16, 2, Test2, 0);

  auto ZeroConst = _Constant(0);
  auto OneConst = _Constant(1);

  Test1 = _Select(FEXCore::IR::COND_EQ,
      Test1, ZeroConst, OneConst, ZeroConst);

  Test2 = _Select(FEXCore::IR::COND_EQ,
      Test2, ZeroConst, OneConst, ZeroConst);

  // Careful, these flags are different between {V,}PTEST and VTESTP{S,D}
  SetRFLAG<FEXCore::X86State::RFLAG_ZF_LOC>(Test1);
  SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(Test2);

  SetRFLAG<FEXCore::X86State::RFLAG_AF_LOC>(ZeroConst);
  SetRFLAG<FEXCore::X86State::RFLAG_SF_LOC>(ZeroConst);
  SetRFLAG<FEXCore::X86State::RFLAG_OF_LOC>(ZeroConst);
  SetRFLAG<FEXCore::X86State::RFLAG_PF_LOC>(ZeroConst);
}

OrderedNode* OpDispatchBuilder::PHMINPOSUWOpImpl(OpcodeArgs) {
  const auto Size = GetSrcSize(Op);

  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  auto Min = _VUMinV(Size, 2, Src);

  std::array<OrderedNode *, 8> Indexes {
    _Constant(0),
    _Constant(1),
    _Constant(2),
    _Constant(3),
    _Constant(4),
    _Constant(5),
    _Constant(6),
    _Constant(7),
  };

  auto Pos = Indexes[7];
  auto MinGPR = _VExtractToGPR(16, 2, Min, 0);

  // Calculate position
  // This doesn't match with ARM behaviour at all
  // Instruction returns the minimum matching index
  for (size_t i = 8; i > 0; --i) {
    auto Element = _VExtractToGPR(16, 2, Src, i - 1);
    Pos = _Select(FEXCore::IR::COND_EQ,
                  Element, MinGPR, Indexes[i - 1], Pos);
  }

  // Insert the minimum in to bits [15:0]
  OrderedNode *Result = _VMov(2, Min);

  // Insert position in to bits [18:16]
  return _VInsGPR(16, 2, 1, Result, Pos);
}

void OpDispatchBuilder::PHMINPOSUWOp(OpcodeArgs) {
  OrderedNode *Result = PHMINPOSUWOpImpl(Op);
  StoreResult(FPRClass, Op, Result, -1);
}

void OpDispatchBuilder::VPHMINPOSUWOp(OpcodeArgs) {
  OrderedNode *MinPos = PHMINPOSUWOpImpl(Op);
  OrderedNode *Result = _VMov(16, MinPos);
  StoreResult(FPRClass, Op, Result, -1);
}

OrderedNode* OpDispatchBuilder::DPPOpImpl(OpcodeArgs, const X86Tables::DecodedOperand& Src1,
                                          const X86Tables::DecodedOperand& Src2,
                                          const X86Tables::DecodedOperand& Imm, size_t ElementSize) {
  LOGMAN_THROW_A_FMT(Imm.IsLiteral(), "Imm needs to be literal here");
  const uint8_t Mask = Imm.Data.Literal.Value;
  const uint8_t SrcMask = Mask >> 4;
  const uint8_t DstMask = Mask & 0xF;

  const auto DstSize = GetDstSize(Op);

  OrderedNode *Src1V = LoadSource(FPRClass, Op, Src1, Op->Flags, -1);
  OrderedNode *Src2V = LoadSource(FPRClass, Op, Src2, Op->Flags, -1);

  OrderedNode *ZeroVec = _VectorZero(DstSize);

  // First step is to do an FMUL
  OrderedNode *Temp = _VFMul(DstSize, ElementSize, Src1V, Src2V);

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

  if (ElementSize == 4) {
    // For 32-bit float we need one more step to add all four results together
    Temp = _VFAddP(DstSize, ElementSize, Temp, ZeroVec);
  }

  // Now using the destination mask we choose where the result ends up
  // It can duplicate and zero results
  OrderedNode *Result = ZeroVec;

  for (size_t i = 0; i < (DstSize / ElementSize); ++i) {
    const auto Bit = 1U << (i % 4);

    if ((DstMask & Bit) != 0) {
      Result = _VInsElement(DstSize, ElementSize, i, 0, Result, Temp);
    }
  }

  return Result;
}

template<size_t ElementSize>
void OpDispatchBuilder::DPPOp(OpcodeArgs) {
  OrderedNode *Result = DPPOpImpl(Op, Op->Dest, Op->Src[0], Op->Src[1], ElementSize);
  StoreResult(FPRClass, Op, Result, -1);
}

template
void OpDispatchBuilder::DPPOp<4>(OpcodeArgs);
template
void OpDispatchBuilder::DPPOp<8>(OpcodeArgs);

template <size_t ElementSize>
void OpDispatchBuilder::VDPPOp(OpcodeArgs) {
  OrderedNode *Result = DPPOpImpl(Op, Op->Src[0], Op->Src[1], Op->Src[2], ElementSize);

  // We don't need to emit a _VMov to clear the upper lane, since DPPOpImpl uses a zero vector
  // to construct the results, so the upper lane will always be cleared for the 128-bit version.
  StoreResult(FPRClass, Op, Result, -1);
}

template
void OpDispatchBuilder::VDPPOp<4>(OpcodeArgs);
template
void OpDispatchBuilder::VDPPOp<8>(OpcodeArgs);

void OpDispatchBuilder::MPSADBWOp(OpcodeArgs) {
  LOGMAN_THROW_A_FMT(Op->Src[1].IsLiteral(), "Src1 needs to be literal here");
  uint8_t Select = Op->Src[1].Data.Literal.Value;

  // Src1 needs to be in byte offset
  uint8_t Select_Dest = ((Select & 0b100) >> 2) * 32 / 8;
  uint8_t Select_Src2 = Select & 0b11;

  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  // Src2 will grab a 32bit element and duplicate it across the 128bits
  OrderedNode *DupSrc = _VDupElement(16, 4, Src, Select_Src2);

  // Src1/Dest needs a bunch of magic

  // Shift right by selected bytes
  // This will give us Dest[15:0], and Dest[79:64]
  OrderedNode *Dest1 = _VExtr(16, 1, Dest, Dest, Select_Dest + 0);
  // This will give us Dest[31:16], and Dest[95:80]
  OrderedNode *Dest2 = _VExtr(16, 1, Dest, Dest, Select_Dest + 1);
  // This will give us Dest[47:32], and Dest[111:96]
  OrderedNode *Dest3 = _VExtr(16, 1, Dest, Dest, Select_Dest + 2);
  // This will give us Dest[63:48], and Dest[127:112]
  OrderedNode *Dest4 = _VExtr(16, 1, Dest, Dest, Select_Dest + 3);

  // For each shifted section, we now have two 32-bit values per vector that can be used
  // Dest1.S[0] and Dest1.S[1] = Bytes - 0,1,2,3:4,5,6,7
  // Dest2.S[0] and Dest2.S[1] = Bytes - 1,2,3,4:5,6,7,8
  // Dest3.S[0] and Dest3.S[1] = Bytes - 2,3,4,5:6,7,8,9
  // Dest4.S[0] and Dest4.S[1] = Bytes - 3,4,5,6:7,8,9,10
  Dest1 = _VUABDL(16, 1, Dest1, DupSrc);
  Dest2 = _VUABDL(16, 1, Dest2, DupSrc);
  Dest3 = _VUABDL(16, 1, Dest3, DupSrc);
  Dest4 = _VUABDL(16, 1, Dest4, DupSrc);

  // Dest[1,2,3,4] Now contains the data prior to combining
  // Temp[0,1,2,3] for each step

  // Each destination now has 16bit x 8 elements in it that were the absolute difference for each byte
  // Needs each to be 16bit to store the next step
  // Next stage is to sum pairwise
  // Dest1:
  //  ADDP Dest2, Dest1: TmpCombine1
  //  ADDP Dest4, Dest3: TmpCombine2
  //    TmpCombine1.8H[0] = Dest1.8H[0] + Dest1.8H[1];
  //    TmpCombine1.8H[1] = Dest1.8H[2] + Dest1.8H[3];
  //    TmpCombine1.8H[2] = Dest1.8H[4] + Dest1.8H[5];
  //    TmpCombine1.8H[3] = Dest1.8H[6] + Dest1.8H[7];
  //    TmpCombine1.8H[4] = Dest2.8H[0] + Dest2.8H[1];
  //    TmpCombine1.8H[5] = Dest2.8H[2] + Dest2.8H[3];
  //    TmpCombine1.8H[6] = Dest2.8H[4] + Dest2.8H[5];
  //    TmpCombine1.8H[7] = Dest2.8H[6] + Dest2.8H[7];
  //    <Repeat for Dest4 and Dest3>
  // ADDP TmpCombine2, TmpCombine1: FinalCombine
  //    FinalCombine.8H[0] = TmpCombine1.8H[0] + TmpCombine1.8H[1]
  //    FinalCombine.8H[1] = TmpCombine1.8H[2] + TmpCombine1.8H[3]
  //    FinalCombine.8H[2] = TmpCombine1.8H[4] + TmpCombine1.8H[5]
  //    FinalCombine.8H[3] = TmpCombine1.8H[6] + TmpCombine1.8H[7]
  //    FinalCombine.8H[4] = TmpCombine2.8H[0] + TmpCombine2.8H[1]
  //    FinalCombine.8H[5] = TmpCombine2.8H[2] + TmpCombine2.8H[3]
  //    FinalCombine.8H[6] = TmpCombine2.8H[4] + TmpCombine2.8H[5]
  //    FinalCombine.8H[7] = TmpCombine2.8H[6] + TmpCombine2.8H[7]

  auto TmpCombine1 = _VAddP(16, 2, Dest1, Dest2);
  auto TmpCombine2 = _VAddP(16, 2, Dest3, Dest4);

  auto FinalCombine = _VAddP(16, 2, TmpCombine1, TmpCombine2);

  // This now contains our results but they are in the wrong order.
  // We need to swizzle the results in to the correct ordering
  // Result.8H[0] = FinalCombine.8H[0]
  // Result.8H[1] = FinalCombine.8H[2]
  // Result.8H[2] = FinalCombine.8H[4]
  // Result.8H[3] = FinalCombine.8H[6]
  // Result.8H[4] = FinalCombine.8H[1]
  // Result.8H[5] = FinalCombine.8H[3]
  // Result.8H[6] = FinalCombine.8H[5]
  // Result.8H[7] = FinalCombine.8H[7]

  auto Even = _VUnZip(16, 2, FinalCombine, FinalCombine);
  auto Odd = _VUnZip2(16, 2, FinalCombine, FinalCombine);
  auto Result = _VInsElement(16, 8, 1, 0, Even, Odd);

  StoreResult(FPRClass, Op, Result, -1);
}

void OpDispatchBuilder::VINSERTOp(OpcodeArgs) {
  const auto DstSize = GetDstSize(Op);
  OrderedNode *Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Src2 = LoadSource_WithOpSize(FPRClass, Op, Op->Src[1], 16, Op->Flags, -1);

  LOGMAN_THROW_A_FMT(Op->Src[2].IsLiteral(), "Src2 needs to be literal here");
  const auto Selector = Op->Src[2].Data.Literal.Value & 1;

  OrderedNode *Result = _VInsElement(DstSize, 16, Selector, 0, Src1, Src2);

  StoreResult(FPRClass, Op, Result, -1);
}

void OpDispatchBuilder::VPERM2Op(OpcodeArgs) {
  const auto DstSize = GetDstSize(Op);
  OrderedNode *Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags, -1);

  LOGMAN_THROW_A_FMT(Op->Src[2].IsLiteral(), "Src2 needs to be literal here");
  const auto Selector = Op->Src[2].Data.Literal.Value;

  OrderedNode *Result = _VectorZero(DstSize);

  const auto SelectElement = [&](uint64_t Index, uint64_t SelectorIdx) {
    switch (SelectorIdx) {
      case 0:
      case 1:
        return _VInsElement(DstSize, 16, Index, SelectorIdx, Result, Src1);
      case 2:
      case 3:
      default:
        return _VInsElement(DstSize, 16, Index, SelectorIdx - 2, Result, Src2);
    }
  };

  if ((Selector & 0b00001000) == 0) {
    Result = SelectElement(0, Selector & 0b11);
  }
  if ((Selector & 0b10000000) == 0) {
    Result = SelectElement(1, (Selector >> 4) & 0b11);
  }

  StoreResult(FPRClass, Op, Result, -1);
}

void OpDispatchBuilder::VPERMQOp(OpcodeArgs) {
  const auto DstSize = GetDstSize(Op);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  LOGMAN_THROW_A_FMT(Op->Src[1].IsLiteral(), "Src1 needs to be literal here");
  const auto Selector = Op->Src[1].Data.Literal.Value;

  OrderedNode *Result = _VectorZero(DstSize);
  for (size_t i = 0; i < DstSize / 8; i++) {
    const auto SrcIndex = (Selector >> (i * 2)) & 0b11;
    Result = _VInsElement(DstSize, 8, i, SrcIndex, Result, Src);
  }
  StoreResult(FPRClass, Op, Result, -1);
}

static OrderedNode* VBLENDOpImpl(IREmitter& IR, uint32_t VecSize, uint32_t ElementSize,
                                 OrderedNode *Src1, OrderedNode *Src2, uint64_t Selector) {
  const std::array Sources{Src1, Src2};

  OrderedNode *Result = IR._VectorZero(VecSize);
  const int NumElements = VecSize / ElementSize;
  for (int i = 0; i < NumElements; i++) {
    const auto SelectorIndex = (Selector >> i) & 1;

    Result = IR._VInsElement(VecSize, ElementSize, i, i, Result, Sources[SelectorIndex]);
  }

  return Result;
}

void OpDispatchBuilder::VBLENDPDOp(OpcodeArgs) {
  const auto DstSize = GetDstSize(Op);
  const auto Is256Bit = DstSize == Core::CPUState::XMM_AVX_REG_SIZE;

  OrderedNode *Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags, -1);

  LOGMAN_THROW_A_FMT(Op->Src[2].IsLiteral(), "Src[2] needs to be literal here");
  const auto Selector = Op->Src[2].Data.Literal.Value;

  if (Selector == 0) {
    OrderedNode *Result = Is256Bit ? Src1 : _VMov(16, Src1);
    StoreResult(FPRClass, Op, Result, -1);
    return;
  }
  // Only the first four bits of the 8-bit immediate are used, so only check them.
  if (((Selector & 0b11) == 0b11 && !Is256Bit) || (Selector & 0b1111) == 0b1111) {
    OrderedNode *Result = Is256Bit ? Src2 : _VMov(16, Src2);
    StoreResult(FPRClass, Op, Result, -1);
    return;
  }

  OrderedNode *Result = VBLENDOpImpl(*this, DstSize, 8, Src1, Src2, Selector);
  StoreResult(FPRClass, Op, Result, -1);
}

void OpDispatchBuilder::VPBLENDDOp(OpcodeArgs) {
  const auto DstSize = GetDstSize(Op);
  const auto Is256Bit = DstSize == Core::CPUState::XMM_AVX_REG_SIZE;

  OrderedNode *Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags, -1);

  LOGMAN_THROW_A_FMT(Op->Src[2].IsLiteral(), "Src[2] needs to be literal here");
  const auto Selector = Op->Src[2].Data.Literal.Value;

  // Each bit in the selector chooses between Src1 and Src2.
  // If a bit is set, then we select it's corresponding 32-bit element from Src2
  // If a bit is not set, then we select it's corresponding 32-bit element from Src1

  // Cases where we can exit out early, since the selector is indicating a copy
  // of an entire input vector. Unlikely to occur, since it's slower than
  // just an equivalent vector move instruction. but just in case something
  // silly is happening, we have your back.

  if (Selector == 0) {
    OrderedNode *Result = Is256Bit ? Src1 : _VMov(16, Src1);
    StoreResult(FPRClass, Op, Result, -1);
    return;
  }
  if (Selector == 0xFF && Is256Bit) {
    StoreResult(FPRClass, Op, Src2, -1);
    return;
  }
  // The only bits we care about from the 8-bit immediate for 128-bit operations
  // are the first four bits. We do a bitwise check here to catch cases where
  // silliness is going on and the upper bits are being set even when they'll
  // be ignored
  if ((Selector & 0xF) == 0xF && !Is256Bit) {
    StoreResult(FPRClass, Op, _VMov(16, Src2), -1);
    return;
  }

  OrderedNode *Result = VBLENDOpImpl(*this, DstSize, 4, Src1, Src2, Selector);
  StoreResult(FPRClass, Op, Result, -1);
}

void OpDispatchBuilder::VPBLENDWOp(OpcodeArgs) {
  const auto DstSize = GetDstSize(Op);
  const auto Is256Bit = DstSize == Core::CPUState::XMM_AVX_REG_SIZE;

  OrderedNode *Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags, -1);

  LOGMAN_THROW_A_FMT(Op->Src[2].IsLiteral(), "Src[2] needs to be literal here");
  const auto Selector = Op->Src[2].Data.Literal.Value;

  if (Selector == 0) {
    OrderedNode *Result = Is256Bit ? Src1 : _VMov(16, Src1);
    StoreResult(FPRClass, Op, Result, -1);
    return;
  }
  if (Selector == 0xFF) {
    OrderedNode *Result = Is256Bit ? Src2 : _VMov(16, Src2);
    StoreResult(FPRClass, Op, Result, -1);
    return;
  }

  // 256-bit VPBLENDW acts as if the 8-bit selector values were also applied
  // to the upper bits, so we can just replicate the bits by forming a 16-bit
  // imm for the helper function to use.
  const auto NewSelector = Selector << 8 | Selector;

  OrderedNode *Result = VBLENDOpImpl(*this, DstSize, 2, Src1, Src2, NewSelector);
  StoreResult(FPRClass, Op, Result, -1);
}

void OpDispatchBuilder::VZEROOp(OpcodeArgs) {
  const auto DstSize = GetDstSize(Op);
  const auto IsVZEROALL = DstSize == Core::CPUState::XMM_AVX_REG_SIZE;
  const auto NumRegs = CTX->Config.Is64BitMode ? 16U : 8U;

  if (IsVZEROALL) {
    // NOTE: Despite the name being VZEROALL, this will still only ever
    //       zero out up to the first 16 registers (even on AVX-512, where we have 32 registers)

    OrderedNode* ZeroVector = _VectorZero(DstSize);
    for (uint32_t i = 0; i < NumRegs; i++) {
      StoreXMMRegister(i, ZeroVector);
    }
  } else {
    // Likewise, VZEROUPPER will only ever zero only up to the first 16 registers

    for (uint32_t i = 0; i < NumRegs; i++) {
      OrderedNode* Reg = LoadXMMRegister(i);
      OrderedNode* Dst = _VMov(16, Reg);
      StoreXMMRegister(i, Dst);
    }
  }
}

template <size_t ElementSize>
void OpDispatchBuilder::VPERMILImmOp(OpcodeArgs) {
  const auto DstSize = GetDstSize(Op);
  const auto Is256Bit = DstSize == Core::CPUState::XMM_AVX_REG_SIZE;

  LOGMAN_THROW_A_FMT(Op->Src[1].IsLiteral(), "Src1 needs to be literal here");
  const auto Selector = Op->Src[1].Data.Literal.Value & 0xFF;

  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Result = _VectorZero(DstSize);

  if constexpr (ElementSize == 8) {
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

  StoreResult(FPRClass, Op, Result, -1);
}

template
void OpDispatchBuilder::VPERMILImmOp<4>(OpcodeArgs);
template
void OpDispatchBuilder::VPERMILImmOp<8>(OpcodeArgs);

}
