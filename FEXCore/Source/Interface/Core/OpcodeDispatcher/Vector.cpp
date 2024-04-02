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
  if (Op->Dest.IsGPR() && Op->Src[0].IsGPR() &&
      Op->Dest.Data.GPR.GPR == Op->Src[0].Data.GPR.GPR) {
    // Nop
    return;
  }
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  StoreResult(FPRClass, Op, Src, -1);
}

void OpDispatchBuilder::MOVVectorUnalignedOp(OpcodeArgs) {
  if (Op->Dest.IsGPR() && Op->Src[0].IsGPR() &&
      Op->Dest.Data.GPR.GPR == Op->Src[0].Data.GPR.GPR) {
    // Nop
    return;
  }
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, {.Align = 1});
  StoreResult(FPRClass, Op, Src, 1);
}

void OpDispatchBuilder::MOVVectorNTOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags,
                                {.Align = 1, .AccessType = MemoryAccessType::STREAM});
  StoreResult(FPRClass, Op, Src, 1, MemoryAccessType::STREAM);
}

void OpDispatchBuilder::VMOVAPS_VMOVAPDOp(OpcodeArgs) {
  const auto SrcSize = GetSrcSize(Op);
  const auto Is128Bit = SrcSize == Core::CPUState::XMM_SSE_REG_SIZE;

  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

  if (Is128Bit && Op->Dest.IsGPR()) {
    Src = _VMov(16, Src);
  }
  StoreResult(FPRClass, Op, Src, -1);
}

void OpDispatchBuilder::VMOVUPS_VMOVUPDOp(OpcodeArgs) {
  const auto SrcSize = GetSrcSize(Op);
  const auto Is128Bit = SrcSize == Core::CPUState::XMM_SSE_REG_SIZE;

  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, {.Align = 1});

  if (Is128Bit && Op->Dest.IsGPR()) {
    Src = _VMov(16, Src);
  }
  StoreResult(FPRClass, Op, Src, 1);
}

void OpDispatchBuilder::MOVHPDOp(OpcodeArgs) {
  if (Op->Dest.IsGPR()) {
    if (Op->Src[0].IsGPR()) {
      // MOVLHPS between two vector registers.
      OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
      OrderedNode *Dest = LoadSource_WithOpSize(FPRClass, Op, Op->Dest, 16, Op->Flags);
      auto Result = _VInsElement(16, 8, 1, 0, Dest, Src);
      StoreResult(FPRClass, Op, Result, -1);
    }
    else {
      // If the destination is a GPR then the source is memory
      // xmm1[127:64] = src
      OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, {.LoadData = false});
      OrderedNode *Dest = LoadSource_WithOpSize(FPRClass, Op, Op->Dest, 16, Op->Flags);
      auto Result = _VLoadVectorElement(16, 8, Dest, 1, Src);
      StoreResult(FPRClass, Op, Result, -1);
    }
  }
  else {
    // In this case memory is the destination and the high bits of the XMM are source
    // Mem64 = xmm1[127:64]
    OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
    OrderedNode *Dest = LoadSource_WithOpSize(GPRClass, Op, Op->Dest, 8, Op->Flags, {.LoadData = false});
    _VStoreVectorElement(16, 8, Src, 1, Dest);
  }
}

void OpDispatchBuilder::VMOVHPOp(OpcodeArgs) {
  if (Op->Dest.IsGPR()) {
    OrderedNode *Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, {.Align = 16});
    OrderedNode *Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags, {.Align = 8});
    OrderedNode *Result = _VInsElement(16, 8, 1, 0, Src1, Src2);

    StoreResult(FPRClass, Op, Result, -1);
  } else {
    OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, {.Align = 16});
    OrderedNode *Result = _VInsElement(16, 8, 0, 1, Src, Src);
    StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Result, 8, 8);
  }
}

void OpDispatchBuilder::MOVLPOp(OpcodeArgs) {
  if (Op->Dest.IsGPR()) {
    // xmm, xmm is movhlps special case
    if (Op->Src[0].IsGPR()) {
      OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, {.Align = 16});
      OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, {.Align = 16});
      auto Result = _VInsElement(16, 8, 0, 1, Dest, Src);
      StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Result, 16, 16);
    }
    else {
      auto DstSize = GetDstSize(Op);
      OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, {.Align = 8, .LoadData = false});
      OrderedNode *Dest = LoadSource_WithOpSize(FPRClass, Op, Op->Dest, DstSize, Op->Flags);
      auto Result = _VLoadVectorElement(16, 8, Dest, 0, Src);
      StoreResult(FPRClass, Op, Result, -1);
    }
  }
  else {
    OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, {.Align = 8});
    StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Src, 8, 8);
  }
}

void OpDispatchBuilder::VMOVLPOp(OpcodeArgs) {
  if (Op->Dest.IsGPR()) {
    OrderedNode *Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, {.Align = 16});
    OrderedNode *Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags, {.Align = 8});
    OrderedNode *Result = _VInsElement(16, 8, 0, 0, Src1, Src2);

    StoreResult(FPRClass, Op, Result, -1);
  } else {
    OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, {.Align = 8});
    StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Src, 8, 8);
  }
}

void OpDispatchBuilder::VMOVSHDUPOp(OpcodeArgs) {
  const auto SrcSize = GetSrcSize(Op);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  OrderedNode *Result = _VTrn2(SrcSize, 4, Src, Src);
  StoreResult(FPRClass, Op, Result, -1);
}

void OpDispatchBuilder::VMOVSLDUPOp(OpcodeArgs) {
  const auto SrcSize = GetSrcSize(Op);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  OrderedNode *Result = _VTrn(SrcSize, 4, Src, Src);
  StoreResult(FPRClass, Op, Result, -1);
}

void OpDispatchBuilder::MOVScalarOpImpl(OpcodeArgs, size_t ElementSize) {
  if (Op->Dest.IsGPR() && Op->Src[0].IsGPR()) {
    // MOVSS/SD xmm1, xmm2
    OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
    OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
    auto Result = _VInsElement(16, ElementSize, 0, 0, Dest, Src);
    StoreResult(FPRClass, Op, Result, -1);
  }
  else if (Op->Dest.IsGPR()) {
    // MOVSS/SD xmm1, mem32/mem64
    // xmm1[127:0] <- zext(mem32/mem64)
    OrderedNode *Src = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], ElementSize, Op->Flags);
    StoreResult(FPRClass, Op, Src, -1);
  }
  else {
    // MOVSS/SD mem32/mem64, xmm1
    OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
    StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Src, ElementSize, -1);
  }
}

void OpDispatchBuilder::MOVSSOp(OpcodeArgs) {
  MOVScalarOpImpl(Op, 4);
}

void OpDispatchBuilder::MOVSDOp(OpcodeArgs) {
  MOVScalarOpImpl(Op, 8);
}

void OpDispatchBuilder::VMOVScalarOpImpl(OpcodeArgs, size_t ElementSize) {
  if (Op->Dest.IsGPR() && Op->Src[0].IsGPR() && Op->Src[1].IsGPR()) {
    // VMOVSS/SD xmm1, xmm2, xmm3
    OrderedNode *Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
    OrderedNode *Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);
    OrderedNode *Result = _VInsElement(16, ElementSize, 0, 0, Src1, Src2);
    StoreResult(FPRClass, Op, Result, -1);
  } else if (Op->Dest.IsGPR()) {
    // VMOVSS/SD xmm1, mem32/mem64
    OrderedNode *Src = LoadSource_WithOpSize(FPRClass, Op, Op->Src[1], ElementSize, Op->Flags);
    StoreResult(FPRClass, Op, Src, -1);
  } else {
    // VMOVSS/SD mem32/mem64, xmm1
    OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);
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
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);

  DeriveOp(ALUOp, IROp, _VAdd(Size, ElementSize, Dest, Src));

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
void OpDispatchBuilder::VectorALUOp<IR::OP_VMUL, 2>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUOp<IR::OP_VMUL, 4>(OpcodeArgs);
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

  OrderedNode *Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  OrderedNode *Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);

  DeriveOp(ALUOp, IROp, _VAdd(Size, ElementSize, Src1, Src2));

  StoreResult(FPRClass, Op, ALUOp, -1);
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
void OpDispatchBuilder::AVXVectorALUOp<IR::OP_VMUL, 2>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorALUOp<IR::OP_VMUL, 4>(OpcodeArgs);

void OpDispatchBuilder::VectorALUROpImpl(OpcodeArgs, IROps IROp, size_t ElementSize) {
  const auto Size = GetSrcSize(Op);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);

  DeriveOp(ALUOp, IROp, _VAdd(Size, ElementSize, Src, Dest));

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

OrderedNode* OpDispatchBuilder::VectorScalarInsertALUOpImpl(OpcodeArgs, IROps IROp,
                                   size_t DstSize, size_t ElementSize,
                                   const X86Tables::DecodedOperand& Src1Op,
                                   const X86Tables::DecodedOperand& Src2Op,
                                   bool ZeroUpperBits) {
  // We load the full vector width when dealing with a source vector,
  // so that we don't do any unnecessary zero extension to the scalar
  // element that we're going to operate on.
  const auto SrcSize = GetSrcSize(Op);

  OrderedNode *Src1 = LoadSource_WithOpSize(FPRClass, Op, Src1Op, DstSize, Op->Flags);
  OrderedNode *Src2 = LoadSource_WithOpSize(FPRClass, Op, Src2Op, SrcSize, Op->Flags,
                                            {.AllowUpperGarbage = true});

  // If OpSize == ElementSize then it only does the lower scalar op
  DeriveOp(ALUOp, IROp,
           _VFAddScalarInsert(IR::SizeToOpSize(DstSize), ElementSize, Src1, Src2, ZeroUpperBits));
  return ALUOp;
}

template <IROps IROp, size_t ElementSize>
void OpDispatchBuilder::VectorScalarInsertALUOp(OpcodeArgs) {
  const auto DstSize = GetGuestVectorLength();
  auto Result = VectorScalarInsertALUOpImpl(Op, IROp, DstSize, ElementSize, Op->Dest, Op->Src[0], false);
  StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Result, DstSize, -1);
}

template
void OpDispatchBuilder::VectorScalarInsertALUOp<IR::OP_VFADDSCALARINSERT, 4>(OpcodeArgs);
template
void OpDispatchBuilder::VectorScalarInsertALUOp<IR::OP_VFADDSCALARINSERT, 8>(OpcodeArgs);
template
void OpDispatchBuilder::VectorScalarInsertALUOp<IR::OP_VFSUBSCALARINSERT, 4>(OpcodeArgs);
template
void OpDispatchBuilder::VectorScalarInsertALUOp<IR::OP_VFSUBSCALARINSERT, 8>(OpcodeArgs);
template
void OpDispatchBuilder::VectorScalarInsertALUOp<IR::OP_VFMULSCALARINSERT, 4>(OpcodeArgs);
template
void OpDispatchBuilder::VectorScalarInsertALUOp<IR::OP_VFMULSCALARINSERT, 8>(OpcodeArgs);
template
void OpDispatchBuilder::VectorScalarInsertALUOp<IR::OP_VFDIVSCALARINSERT, 4>(OpcodeArgs);
template
void OpDispatchBuilder::VectorScalarInsertALUOp<IR::OP_VFDIVSCALARINSERT, 8>(OpcodeArgs);
template
void OpDispatchBuilder::VectorScalarInsertALUOp<IR::OP_VFMINSCALARINSERT, 4>(OpcodeArgs);
template
void OpDispatchBuilder::VectorScalarInsertALUOp<IR::OP_VFMINSCALARINSERT, 8>(OpcodeArgs);
template
void OpDispatchBuilder::VectorScalarInsertALUOp<IR::OP_VFMAXSCALARINSERT, 4>(OpcodeArgs);
template
void OpDispatchBuilder::VectorScalarInsertALUOp<IR::OP_VFMAXSCALARINSERT, 8>(OpcodeArgs);

template <IROps IROp, size_t ElementSize>
void OpDispatchBuilder::AVXVectorScalarInsertALUOp(OpcodeArgs) {
  const auto DstSize = GetGuestVectorLength();
  auto Result = VectorScalarInsertALUOpImpl(Op, IROp, DstSize, ElementSize, Op->Src[0], Op->Src[1], true);
  StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Result, DstSize, -1);
}

template
void OpDispatchBuilder::AVXVectorScalarInsertALUOp<IR::OP_VFADDSCALARINSERT, 4>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorScalarInsertALUOp<IR::OP_VFADDSCALARINSERT, 8>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorScalarInsertALUOp<IR::OP_VFSUBSCALARINSERT, 4>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorScalarInsertALUOp<IR::OP_VFSUBSCALARINSERT, 8>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorScalarInsertALUOp<IR::OP_VFMULSCALARINSERT, 4>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorScalarInsertALUOp<IR::OP_VFMULSCALARINSERT, 8>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorScalarInsertALUOp<IR::OP_VFDIVSCALARINSERT, 4>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorScalarInsertALUOp<IR::OP_VFDIVSCALARINSERT, 8>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorScalarInsertALUOp<IR::OP_VFMINSCALARINSERT, 4>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorScalarInsertALUOp<IR::OP_VFMINSCALARINSERT, 8>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorScalarInsertALUOp<IR::OP_VFMAXSCALARINSERT, 4>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorScalarInsertALUOp<IR::OP_VFMAXSCALARINSERT, 8>(OpcodeArgs);

OrderedNode* OpDispatchBuilder::VectorScalarUnaryInsertALUOpImpl(OpcodeArgs, IROps IROp,
                                   size_t DstSize, size_t ElementSize,
                                   const X86Tables::DecodedOperand& Src1Op,
                                   const X86Tables::DecodedOperand& Src2Op,
                                   bool ZeroUpperBits) {
  // We load the full vector width when dealing with a source vector,
  // so that we don't do any unnecessary zero extension to the scalar
  // element that we're going to operate on.
  const auto SrcSize = GetSrcSize(Op);

  OrderedNode *Src1 = LoadSource_WithOpSize(FPRClass, Op, Src1Op, DstSize, Op->Flags);
  OrderedNode *Src2 = LoadSource_WithOpSize(FPRClass, Op, Src2Op, SrcSize, Op->Flags,
                                            {.AllowUpperGarbage = true});

  // If OpSize == ElementSize then it only does the lower scalar op
  DeriveOp(ALUOp, IROp, _VFSqrtScalarInsert(IR::SizeToOpSize(DstSize), ElementSize, Src1, Src2, ZeroUpperBits));
  return ALUOp;
}

template <IROps IROp, size_t ElementSize>
void OpDispatchBuilder::VectorScalarUnaryInsertALUOp(OpcodeArgs) {
  const auto DstSize = GetGuestVectorLength();
  auto Result = VectorScalarInsertALUOpImpl(Op, IROp, DstSize, ElementSize, Op->Dest, Op->Src[0], false);
  StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Result, DstSize, -1);
}

template
void OpDispatchBuilder::VectorScalarUnaryInsertALUOp<IR::OP_VFSQRTSCALARINSERT, 4>(OpcodeArgs);
template
void OpDispatchBuilder::VectorScalarUnaryInsertALUOp<IR::OP_VFSQRTSCALARINSERT, 8>(OpcodeArgs);

template
void OpDispatchBuilder::VectorScalarUnaryInsertALUOp<IR::OP_VFRSQRTSCALARINSERT, 4>(OpcodeArgs);
template
void OpDispatchBuilder::VectorScalarUnaryInsertALUOp<IR::OP_VFRSQRTSCALARINSERT, 8>(OpcodeArgs);

template
void OpDispatchBuilder::VectorScalarUnaryInsertALUOp<IR::OP_VFRECPSCALARINSERT, 4>(OpcodeArgs);
template
void OpDispatchBuilder::VectorScalarUnaryInsertALUOp<IR::OP_VFRECPSCALARINSERT, 8>(OpcodeArgs);

template <IROps IROp, size_t ElementSize>
void OpDispatchBuilder::AVXVectorScalarUnaryInsertALUOp(OpcodeArgs) {
  const auto DstSize = GetGuestVectorLength();
  auto Result = VectorScalarInsertALUOpImpl(Op, IROp, DstSize, ElementSize, Op->Src[0], Op->Src[1], true);
  StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Result, DstSize, -1);
}

template
void OpDispatchBuilder::AVXVectorScalarUnaryInsertALUOp<IR::OP_VFSQRTSCALARINSERT, 4>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorScalarUnaryInsertALUOp<IR::OP_VFSQRTSCALARINSERT, 8>(OpcodeArgs);

template
void OpDispatchBuilder::AVXVectorScalarUnaryInsertALUOp<IR::OP_VFRSQRTSCALARINSERT, 4>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorScalarUnaryInsertALUOp<IR::OP_VFRSQRTSCALARINSERT, 8>(OpcodeArgs);

template
void OpDispatchBuilder::AVXVectorScalarUnaryInsertALUOp<IR::OP_VFRECPSCALARINSERT, 4>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorScalarUnaryInsertALUOp<IR::OP_VFRECPSCALARINSERT, 8>(OpcodeArgs);

void OpDispatchBuilder::InsertMMX_To_XMM_Vector_CVT_Int_To_Float(OpcodeArgs) {
  // We load the full vector width when dealing with a source vector,
  // so that we don't do any unnecessary zero extension to the scalar
  // element that we're going to operate on.
  const auto DstSize = GetGuestVectorLength();
  const auto SrcSize = Op->Src[0].IsGPR() ? 8 : GetSrcSize(Op);

  OrderedNode *Dest = LoadSource_WithOpSize(FPRClass, Op, Op->Dest, DstSize, Op->Flags);
  OrderedNode *Src = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], SrcSize, Op->Flags);

  // Always 32-bit.
  const size_t ElementSize = 4;
  // Always signed
  Dest = _VSToFVectorInsert(IR::SizeToOpSize(DstSize), ElementSize, ElementSize, Dest, Src, true, false);

  StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Dest, DstSize, -1);
}

OrderedNode* OpDispatchBuilder::InsertCVTGPR_To_FPRImpl(OpcodeArgs,
                                                  size_t DstSize, size_t DstElementSize,
                                                  const X86Tables::DecodedOperand& Src1Op,
                                                  const X86Tables::DecodedOperand& Src2Op,
                                                  bool ZeroUpperBits) {
  // We load the full vector width when dealing with a source vector,
  // so that we don't do any unnecessary zero extension to the scalar
  // element that we're going to operate on.
  const auto SrcSize = GetSrcSize(Op);

  OrderedNode *Src1 = LoadSource_WithOpSize(FPRClass, Op, Src1Op, DstSize, Op->Flags);

  if (Src2Op.IsGPR()) {
    // If the source is a GPR then convert directly from the GPR.
    auto Src2 = LoadSource_WithOpSize(GPRClass, Op, Src2Op, CTX->GetGPRSize(), Op->Flags);
    return _VSToFGPRInsert(IR::SizeToOpSize(DstSize), DstElementSize, SrcSize, Src1, Src2, ZeroUpperBits);
  }
  else if (SrcSize != DstElementSize) {
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

template<size_t DstElementSize>
void OpDispatchBuilder::InsertCVTGPR_To_FPR(OpcodeArgs) {
  const auto DstSize = GetGuestVectorLength();
  auto Result = InsertCVTGPR_To_FPRImpl(Op, DstSize, DstElementSize, Op->Dest, Op->Src[0], false);
  StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Result, DstSize, -1);
}

template
void OpDispatchBuilder::InsertCVTGPR_To_FPR<4>(OpcodeArgs);
template
void OpDispatchBuilder::InsertCVTGPR_To_FPR<8>(OpcodeArgs);

template <size_t DstElementSize>
void OpDispatchBuilder::AVXInsertCVTGPR_To_FPR(OpcodeArgs) {
  const auto DstSize = GetGuestVectorLength();
  OrderedNode *Result = InsertCVTGPR_To_FPRImpl(Op, DstSize, DstElementSize, Op->Src[0], Op->Src[1], true);
  StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Result, DstSize, -1);
}
template
void OpDispatchBuilder::AVXInsertCVTGPR_To_FPR<4>(OpcodeArgs);
template
void OpDispatchBuilder::AVXInsertCVTGPR_To_FPR<8>(OpcodeArgs);

OrderedNode* OpDispatchBuilder::InsertScalar_CVT_Float_To_FloatImpl(OpcodeArgs,
                                                              size_t DstSize, size_t DstElementSize, size_t SrcElementSize,
                                                              const X86Tables::DecodedOperand& Src1Op,
                                                              const X86Tables::DecodedOperand& Src2Op,
                                                              bool ZeroUpperBits) {

  // We load the full vector width when dealing with a source vector,
  // so that we don't do any unnecessary zero extension to the scalar
  // element that we're going to operate on.
  const auto SrcSize = GetSrcSize(Op);

  OrderedNode *Src1 = LoadSource_WithOpSize(FPRClass, Op, Src1Op, DstSize, Op->Flags);
  OrderedNode *Src2 = LoadSource_WithOpSize(FPRClass, Op, Src2Op, SrcSize, Op->Flags,
                                            {.AllowUpperGarbage = true});

  return _VFToFScalarInsert(IR::SizeToOpSize(DstSize), DstElementSize, SrcElementSize, Src1, Src2, ZeroUpperBits);
}

template<size_t DstElementSize, size_t SrcElementSize>
void OpDispatchBuilder::InsertScalar_CVT_Float_To_Float(OpcodeArgs) {
  const auto DstSize = GetGuestVectorLength();
  OrderedNode *Result = InsertScalar_CVT_Float_To_FloatImpl(Op, DstSize, DstElementSize, SrcElementSize, Op->Dest, Op->Src[0], false);
  StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Result, DstSize, -1);
}

template
void OpDispatchBuilder::InsertScalar_CVT_Float_To_Float<4, 8>(OpcodeArgs);
template
void OpDispatchBuilder::InsertScalar_CVT_Float_To_Float<8, 4>(OpcodeArgs);

template <size_t DstElementSize, size_t SrcElementSize>
void OpDispatchBuilder::AVXInsertScalar_CVT_Float_To_Float(OpcodeArgs) {
  const auto DstSize = GetGuestVectorLength();
  OrderedNode *Result = InsertScalar_CVT_Float_To_FloatImpl(Op, DstSize, DstElementSize, SrcElementSize, Op->Src[0], Op->Src[1], true);
  StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Result, DstSize, -1);
}

template
void OpDispatchBuilder::AVXInsertScalar_CVT_Float_To_Float<4, 8>(OpcodeArgs);
template
void OpDispatchBuilder::AVXInsertScalar_CVT_Float_To_Float<8, 4>(OpcodeArgs);

OrderedNode* OpDispatchBuilder::InsertScalarRoundImpl(OpcodeArgs,
                                   size_t DstSize, size_t ElementSize,
                                   const X86Tables::DecodedOperand& Src1Op,
                                   const X86Tables::DecodedOperand& Src2Op,
                                   uint64_t Mode, bool ZeroUpperBits) {
  // We load the full vector width when dealing with a source vector,
  // so that we don't do any unnecessary zero extension to the scalar
  // element that we're going to operate on.
  const auto SrcSize = GetSrcSize(Op);

  OrderedNode *Src1 = LoadSource_WithOpSize(FPRClass, Op, Src1Op, DstSize, Op->Flags);
  OrderedNode *Src2 = LoadSource_WithOpSize(FPRClass, Op, Src2Op, SrcSize, Op->Flags,
                                            {.AllowUpperGarbage = true});

  const uint64_t RoundControlSource = (Mode >> 2) & 1;
  uint64_t RoundControl = Mode & 0b11;

  static constexpr std::array SourceModes = {
    FEXCore::IR::Round_Nearest,
    FEXCore::IR::Round_Negative_Infinity,
    FEXCore::IR::Round_Positive_Infinity,
    FEXCore::IR::Round_Towards_Zero,
  };

  const auto SourceMode = RoundControlSource ? Round_Host : SourceModes[RoundControl];

  auto ALUOp = _VFToIScalarInsert(IR::SizeToOpSize(DstSize), ElementSize, Src1, Src2, SourceMode, ZeroUpperBits);

  return ALUOp;
}

template<size_t ElementSize>
void OpDispatchBuilder::InsertScalarRound(OpcodeArgs) {
  LOGMAN_THROW_A_FMT(Op->Src[1].IsLiteral(), "Src1 needs to be literal here");
  const uint64_t Mode = Op->Src[1].Data.Literal.Value;

  const auto DstSize = GetGuestVectorLength();
  OrderedNode *Result = InsertScalarRoundImpl(Op, DstSize, ElementSize, Op->Dest, Op->Src[0], Mode, false);
  StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Result, DstSize, -1);
}

template
void OpDispatchBuilder::InsertScalarRound<4>(OpcodeArgs);
template
void OpDispatchBuilder::InsertScalarRound<8>(OpcodeArgs);

template<size_t ElementSize>
void OpDispatchBuilder::AVXInsertScalarRound(OpcodeArgs) {
  LOGMAN_THROW_A_FMT(Op->Src[2].IsLiteral(), "Src1 needs to be literal here");
  const uint64_t Mode = Op->Src[2].Data.Literal.Value;

  const auto DstSize = GetGuestVectorLength();
  OrderedNode *Result = InsertScalarRoundImpl(Op, DstSize, ElementSize, Op->Dest, Op->Src[0], Mode, true);
  StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Result, DstSize, -1);
}

template
void OpDispatchBuilder::AVXInsertScalarRound<4>(OpcodeArgs);
template
void OpDispatchBuilder::AVXInsertScalarRound<8>(OpcodeArgs);


OrderedNode* OpDispatchBuilder::InsertScalarFCMPOpImpl(OpcodeArgs,
                                   size_t DstSize, size_t ElementSize,
                                   const X86Tables::DecodedOperand& Src1Op,
                                   const X86Tables::DecodedOperand& Src2Op,
                                   uint8_t CompType, bool ZeroUpperBits) {
  // We load the full vector width when dealing with a source vector,
  // so that we don't do any unnecessary zero extension to the scalar
  // element that we're going to operate on.
  const auto SrcSize = GetSrcSize(Op);

  OrderedNode *Src1 = LoadSource_WithOpSize(FPRClass, Op, Src1Op, DstSize, Op->Flags);
  OrderedNode *Src2 = LoadSource_WithOpSize(FPRClass, Op, Src2Op, SrcSize, Op->Flags,
                                            {.AllowUpperGarbage = true});

  switch (CompType) {
    case 0x00: case 0x08: case 0x10: case 0x18: // EQ
      return _VFCMPScalarInsert(IR::SizeToOpSize(DstSize), ElementSize, Src1, Src2, FloatCompareOp::EQ, ZeroUpperBits);
    case 0x01: case 0x09: case 0x11: case 0x19: // LT, GT(Swapped operand)
      return _VFCMPScalarInsert(IR::SizeToOpSize(DstSize), ElementSize, Src1, Src2, FloatCompareOp::LT, ZeroUpperBits);
    case 0x02: case 0x0A: case 0x12: case 0x1A: // LE, GE(Swapped operand)
      return _VFCMPScalarInsert(IR::SizeToOpSize(DstSize), ElementSize, Src1, Src2, FloatCompareOp::LE, ZeroUpperBits);
    case 0x03: case 0x0B: case 0x13: case 0x1B: // Unordered
      return _VFCMPScalarInsert(IR::SizeToOpSize(DstSize), ElementSize, Src1, Src2, FloatCompareOp::UNO, ZeroUpperBits);
    case 0x04: case 0x0C: case 0x14: case 0x1C: // NEQ
      return _VFCMPScalarInsert(IR::SizeToOpSize(DstSize), ElementSize, Src1, Src2, FloatCompareOp::NEQ, ZeroUpperBits);
    case 0x05: case 0x0D: case 0x15: case 0x1D: { // NLT, NGT(Swapped operand)
      OrderedNode *Result = _VFCMPLT(ElementSize, ElementSize, Src1, Src2);
      Result = _VNot(ElementSize, ElementSize, Result);
      // Insert the lower bits
      return _VInsElement(GetDstSize(Op), ElementSize, 0, 0, Src1, Result);
    }
    case 0x06: case 0x0E: case 0x16: case 0x1E: { // NLE, NGE(Swapped operand)
      OrderedNode *Result = _VFCMPLE(ElementSize, ElementSize, Src1, Src2);
      Result = _VNot(ElementSize, ElementSize, Result);
      // Insert the lower bits
      return _VInsElement(GetDstSize(Op), ElementSize, 0, 0, Src1, Result);
    }
    case 0x07: case 0x0F: case 0x17: case 0x1F: // Ordered
      return _VFCMPScalarInsert(IR::SizeToOpSize(DstSize), ElementSize, Src1, Src2, FloatCompareOp::ORD, ZeroUpperBits);
    default:
      LOGMAN_MSG_A_FMT("Unknown Comparison type: {}", CompType);
      break;
  }
  FEX_UNREACHABLE;
}

template<size_t ElementSize>
void OpDispatchBuilder::InsertScalarFCMPOp(OpcodeArgs) {
  LOGMAN_THROW_A_FMT(Op->Src[1].IsLiteral(), "Src[2] needs to be literal");
  const uint8_t CompType = Op->Src[1].Data.Literal.Value;

  const auto DstSize = GetGuestVectorLength();
  OrderedNode *Result = InsertScalarFCMPOpImpl(Op, DstSize, ElementSize, Op->Dest, Op->Src[0], CompType, false);
  StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Result, DstSize, -1);
}

template
void OpDispatchBuilder::InsertScalarFCMPOp<4>(OpcodeArgs);
template
void OpDispatchBuilder::InsertScalarFCMPOp<8>(OpcodeArgs);

template<size_t ElementSize>
void OpDispatchBuilder::AVXInsertScalarFCMPOp(OpcodeArgs) {
  LOGMAN_THROW_A_FMT(Op->Src[2].IsLiteral(), "Src[2] needs to be literal");
  const uint8_t CompType = Op->Src[2].Data.Literal.Value;

  const auto DstSize = GetGuestVectorLength();
  OrderedNode *Result = InsertScalarFCMPOpImpl(Op, DstSize, ElementSize, Op->Src[0], Op->Src[1], CompType, true);
  StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Result, DstSize, -1);
}

template
void OpDispatchBuilder::AVXInsertScalarFCMPOp<4>(OpcodeArgs);
template
void OpDispatchBuilder::AVXInsertScalarFCMPOp<8>(OpcodeArgs);

void OpDispatchBuilder::VectorUnaryOpImpl(OpcodeArgs, IROps IROp, size_t ElementSize) {
  // In the event of a scalar operation and a vector source, then
  // we can specify the entire vector length in order to avoid
  // unnecessary sign extension on the element to be operated on.
  // In the event of a memory operand, we load the exact element size.
  const auto SrcSize = GetSrcSize(Op);
  const auto OpSize = GetSrcSize(Op);

  OrderedNode *Src = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], SrcSize, Op->Flags);

  DeriveOp(ALUOp, IROp, _VFSqrt(OpSize, ElementSize, Src));

  StoreResult(FPRClass, Op, ALUOp, -1);
}

template <IROps IROp, size_t ElementSize>
void OpDispatchBuilder::VectorUnaryOp(OpcodeArgs) {
  VectorUnaryOpImpl(Op, IROp, ElementSize);
}

template
void OpDispatchBuilder::VectorUnaryOp<IR::OP_VFSQRT, 4>(OpcodeArgs);
template
void OpDispatchBuilder::VectorUnaryOp<IR::OP_VFRSQRT, 4>(OpcodeArgs);
template
void OpDispatchBuilder::VectorUnaryOp<IR::OP_VFRECP, 4>(OpcodeArgs);

template
void OpDispatchBuilder::VectorUnaryOp<IR::OP_VFSQRT, 8>(OpcodeArgs);

template
void OpDispatchBuilder::VectorUnaryOp<IR::OP_VABS, 1>(OpcodeArgs);
template
void OpDispatchBuilder::VectorUnaryOp<IR::OP_VABS, 2>(OpcodeArgs);
template
void OpDispatchBuilder::VectorUnaryOp<IR::OP_VABS, 4>(OpcodeArgs);

void OpDispatchBuilder::AVXVectorUnaryOpImpl(OpcodeArgs, IROps IROp, size_t ElementSize) {
  // In the event of a scalar operation and a vector source, then
  // we can specify the entire vector length in order to avoid
  // unnecessary sign extension on the element to be operated on.
  // In the event of a memory operand, we load the exact element size.
  const auto SrcSize = GetSrcSize(Op);
  const auto OpSize = GetSrcSize(Op);

  OrderedNode *Src = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], SrcSize, Op->Flags);

  DeriveOp(ALUOp, IROp, _VFSqrt(OpSize, ElementSize, Src));

  // NOTE: We don't need to clear the upper lanes here, since the
  //       IR ops make use of 128-bit AdvSimd for 128-bit cases,
  //       which, on hardware with SVE, zero-extends as part of
  //       storing into the destination.

  StoreResult(FPRClass, Op, ALUOp, -1);
}

template <IROps IROp, size_t ElementSize>
void OpDispatchBuilder::AVXVectorUnaryOp(OpcodeArgs) {
  AVXVectorUnaryOpImpl(Op, IROp, ElementSize);
}

template
void OpDispatchBuilder::AVXVectorUnaryOp<IR::OP_VABS, 1>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorUnaryOp<IR::OP_VABS, 2>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorUnaryOp<IR::OP_VABS, 4>(OpcodeArgs);

template
void OpDispatchBuilder::AVXVectorUnaryOp<IR::OP_VFRECP, 4>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorUnaryOp<IR::OP_VFSQRT, 4>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorUnaryOp<IR::OP_VFSQRT, 8>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorUnaryOp<IR::OP_VFRSQRT, 4>(OpcodeArgs);

void OpDispatchBuilder::VectorUnaryDuplicateOpImpl(OpcodeArgs, IROps IROp, size_t ElementSize) {
  const auto Size = GetSrcSize(Op);

  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

  DeriveOp(ALUOp, IROp, _VFSqrt(ElementSize, ElementSize, Src));

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
  const auto SrcSize = Op->Src[0].IsGPR() ? 16U : GetSrcSize(Op);
  OrderedNode *Src = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], SrcSize, Op->Flags);
  // This instruction is a bit special that if the destination is a register then it'll ZEXT the 64bit source to 128bit
  if (Op->Dest.IsGPR()) {
    const auto gpr = Op->Dest.Data.GPR.GPR;
    const auto gprIndex = gpr - X86State::REG_XMM_0;

    auto Reg = _VMov(8, Src);
    StoreXMMRegister(gprIndex, Reg);
  }
  else {
    // This is simple, just store the result
    StoreResult(FPRClass, Op, Src, -1);
  }
}

void OpDispatchBuilder::MOVQMMXOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, {.Align = 1});
  StoreResult(FPRClass, Op, Src, 1);
}

template<size_t ElementSize>
void OpDispatchBuilder::MOVMSKOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);
  uint8_t NumElements = Size / ElementSize;

  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

  if (Size == 16 && ElementSize == 8) {
    // UnZip2 the 64-bit elements as 32-bit to get the sign bits closer.
    // Sign bits are now in bit positions 31 and 63 after this.
    Src =_VUnZip2(Size, 4, Src, Src);

    // Extract the low 64-bits to GPR in one move.
    OrderedNode *GPR = _VExtractToGPR(Size, 8, Src, 0);
    // BFI the sign bit in 31 in to 62.
    // Inserting the full lower 32-bits offset 31 so the sign bit ends up at offset 63.
    GPR = _Bfi(OpSize::i64Bit, 32, 31, GPR, GPR);
    // Shift right to only get the two sign bits we care about.
    GPR = _Lshr(OpSize::i64Bit, GPR, _Constant(62));
    StoreResult_WithOpSize(GPRClass, Op, Op->Dest, GPR, CTX->GetGPRSize(), -1);
  }
  else if (Size == 16 && ElementSize == 4) {
    // Shift all the sign bits to the bottom of their respective elements.
    Src = _VUShrI(Size, 4, Src, 31);
    // Load the specific 128-bit movmskps shift elements operator.
    auto ConstantUSHL = LoadAndCacheNamedVectorConstant(Size, NAMED_VECTOR_MOVMSKPS_SHIFT);
    // Shift the sign bits in to specific locations.
    Src = _VUShl(Size, 4, Src, ConstantUSHL, false);
    // Add across the vector so the sign bits will end up in bits [3:0]
    Src = _VAddV(Size, 4, Src);
    // Extract to a GPR.
    OrderedNode *GPR = _VExtractToGPR(Size, 4, Src, 0);
    StoreResult_WithOpSize(GPRClass, Op, Op->Dest, GPR, CTX->GetGPRSize(), -1);
  }
  else {
    OrderedNode *CurrentVal = _Constant(0);

    for (unsigned i = 0; i < NumElements; ++i) {
      // Extract the top bit of the element
      OrderedNode *Tmp = _VExtractToGPR(Size, ElementSize, Src, i);
      Tmp = _Bfe(IR::SizeToOpSize(ElementSize), 1, ElementSize * 8 - 1, Tmp);

      // Shift it to the correct location
      Tmp = _Lshl(IR::SizeToOpSize(ElementSize), Tmp, _Constant(i));

      // Or it with the current value
      CurrentVal = _Or(OpSize::i64Bit, CurrentVal, Tmp);
    }
    StoreResult(GPRClass, Op, CurrentVal, -1);
  }
}

template
void OpDispatchBuilder::MOVMSKOp<4>(OpcodeArgs);
template
void OpDispatchBuilder::MOVMSKOp<8>(OpcodeArgs);

void OpDispatchBuilder::MOVMSKOpOne(OpcodeArgs) {
  const auto SrcSize = GetSrcSize(Op);
  const auto Is256Bit = SrcSize == Core::CPUState::XMM_AVX_REG_SIZE;
  const auto ExtractSize = Is256Bit ? 4 : 2;

  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  OrderedNode *VMask = LoadAndCacheNamedVectorConstant(SrcSize, NAMED_VECTOR_MOVMASKB);

  auto VCMP = _VCMPLTZ(SrcSize, 1, Src);
  auto VAnd = _VAnd(SrcSize, 1, VCMP, VMask);

  // Since we also handle the MM MOVMSKB here too,
  // we need to clamp the lower bound.
  const auto VAdd1Size = std::max(SrcSize, uint8_t{16});
  const auto VAdd2Size = std::max(SrcSize / 2, 8);

  auto VAdd1 = _VAddP(VAdd1Size, 1, VAnd, VAnd);
  auto VAdd2 = _VAddP(VAdd2Size, 1, VAdd1, VAdd1);
  auto VAdd3 = _VAddP(8, 1, VAdd2, VAdd2);

  auto Result = _VExtractToGPR(SrcSize, ExtractSize, VAdd3, 0);

  StoreResult(GPRClass, Op, Result, -1);
}

template<size_t ElementSize>
void OpDispatchBuilder::PUNPCKLOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);

  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

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

  OrderedNode *Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  OrderedNode *Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);

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
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

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

  OrderedNode *Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  OrderedNode *Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);

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
  OrderedNode *Src1Node = LoadSource(FPRClass, Op, Src1, Op->Flags);
  OrderedNode *Src2Node = LoadSource(FPRClass, Op, Src2, Op->Flags);

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

void OpDispatchBuilder::PSHUFW8ByteOp(OpcodeArgs) {
  constexpr auto IdentityCopy = 0b11'10'01'00;

  uint16_t Shuffle = Op->Src[1].Data.Literal.Value;
  const auto Size = GetSrcSize(Op);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  OrderedNode *Dest{};

  // TODO: There can be more optimized copies here.
  switch (Shuffle) {
    case IdentityCopy: {
      // Special case identity copy.
      Dest = Src;
      break;
    }
    case 0b01'01'00'00: {
      // Zip with self.
      // Dest[0] = Src[0]
      // Dest[1] = Src[0]
      // Dest[2] = Src[1]
      // Dest[3] = Src[1]
      Dest = _VZip(Size, 2, Src, Src);
      break;
    }
    case 0b00'00'00'00:
    case 0b01'01'01'01:
    case 0b10'10'10'10:
    case 0b11'11'11'11: {
      // Special case element duplicate and broadcasts.
      Dest = _VDupElement(Size, 2, Src, (Shuffle & 0b11));
      break;
    }
    default: {
      // PSHUFW (mmx) also needs to scale by 16 to get correct low element.
      auto LookupIndexes = LoadAndCacheIndexedNamedVectorConstant(Size, FEXCore::IR::IndexNamedVectorConstant::INDEXED_NAMED_VECTOR_PSHUFLW, Shuffle * 16);
      Dest = _VTBL1(Size, Src, LookupIndexes);
      break;
    }
  }

  StoreResult(FPRClass, Op, Dest, -1);
}

template<bool Low>
void OpDispatchBuilder::PSHUFWOp(OpcodeArgs) {
  constexpr auto IdentityCopy = 0b11'10'01'00;

  uint16_t Shuffle = Op->Src[1].Data.Literal.Value;
  const auto Size = GetSrcSize(Op);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  OrderedNode *Dest{};

  const uint8_t NumElements = Size / 2;
  const uint8_t HalfNumElements = NumElements >> 1;

  // TODO: There can be more optimized copies here.
  switch (Shuffle) {
    case IdentityCopy: {
      // Special case identity copy.
      Dest = Src;
      break;
    }
    case 0b01'01'00'00: {
      // Zip with self.
      // Dest[0] = Src[0]
      // Dest[1] = Src[0]
      // Dest[2] = Src[1]
      // Dest[3] = Src[1]
      auto Zip = _VZip(Size, 2, Src, Src);
      if (Low) {
        Dest = _VZip(Size, 8, Zip, Src);
      }
      else {
        Dest = _VZip(Size, 8, Src, Zip);
      }
      break;
    }
    case 0b00'00'00'00:
    case 0b01'01'01'01:
    case 0b10'10'10'10:
    case 0b11'11'11'11: {
      // Special case element duplicate and broadcast to low or high 64-bits.
      auto DUP = _VDupElement(Size, 2, Src, (Low ? 0 : HalfNumElements) + (Shuffle & 0b11));
      if (Low) {
        // DUP goes low.
        // Source goes high.
        Dest = _VTrn2(Size, 8, DUP, Src);
      }
      else {
        // DUP goes high.
        // Source goes low.
        Dest = _VTrn(Size, 8, Src, DUP);
      }
      break;
    }
    default: {
      // PSHUFLW needs to scale index by 16.
      // PSHUFHW needs to scale index by 16.
      // PSHUFW (mmx) also needs to scale by 16 to get correct low element.
      const auto IndexedVectorConstant = Low ?
        FEXCore::IR::IndexNamedVectorConstant::INDEXED_NAMED_VECTOR_PSHUFLW :
        FEXCore::IR::IndexNamedVectorConstant::INDEXED_NAMED_VECTOR_PSHUFHW;
      auto LookupIndexes = LoadAndCacheIndexedNamedVectorConstant(Size, IndexedVectorConstant, Shuffle * 16);
      Dest = _VTBL1(Size, Src, LookupIndexes);
      break;
    }
  }

  StoreResult(FPRClass, Op, Dest, -1);
}

template
void OpDispatchBuilder::PSHUFWOp<false>(OpcodeArgs);
template
void OpDispatchBuilder::PSHUFWOp<true>(OpcodeArgs);

void OpDispatchBuilder::PSHUFDOp(OpcodeArgs) {
  constexpr auto IdentityCopy = 0b11'10'01'00;

  uint16_t Shuffle = Op->Src[1].Data.Literal.Value;
  const auto Size = GetSrcSize(Op);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  OrderedNode *Dest{};

  // TODO: There can be more optimized copies here.
  switch (Shuffle) {
    case IdentityCopy: {
      // Special case identity copy.
      Dest = Src;
      break;
    }
    case 0b01'01'00'00: {
      // Zip with self.
      // Dest[0] = Src[0]
      // Dest[1] = Src[0]
      // Dest[2] = Src[1]
      // Dest[3] = Src[1]
      Dest = _VZip(Size, 4, Src, Src);
      break;
    }
    case 0b00'00'00'00:
    case 0b01'01'01'01:
    case 0b10'10'10'10:
    case 0b11'11'11'11: {
      // Special case element duplicate and broadcast to low or high 64-bits.
      Dest = _VDupElement(Size, 4, Src, Shuffle & 0b11);
      break;
    }
    default: {
      // PSHUFD needs to scale index by 16.
      auto LookupIndexes = LoadAndCacheIndexedNamedVectorConstant(Size, FEXCore::IR::IndexNamedVectorConstant::INDEXED_NAMED_VECTOR_PSHUFD, Shuffle * 16);
      Dest = _VTBL1(Size, Src, LookupIndexes);
      break;
    }
  }

  StoreResult(FPRClass, Op, Dest, -1);
}

template <size_t ElementSize, bool Low>
void OpDispatchBuilder::VPSHUFWOp(OpcodeArgs) {
  const auto SrcSize = GetSrcSize(Op);
  const auto Is256Bit = SrcSize == Core::CPUState::XMM_AVX_REG_SIZE;

  LOGMAN_THROW_A_FMT(Op->Src[1].IsLiteral(), "Src[1] needs to be a literal");
  auto Shuffle = Op->Src[1].Data.Literal.Value;

  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

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
  OrderedNode *Src1Node = LoadSource(FPRClass, Op, Src1, Op->Flags);
  OrderedNode *Src2Node = LoadSource(FPRClass, Op, Src2, Op->Flags);

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
    if (ElementSize == 4) {
      // We can shuffle optimally in a lot of cases.
      // TODO: We can optimize more of these cases.
      switch (Shuffle) {
        case 0b01'00'01'00:
          // Combining of low 64-bits.
          // Dest[63:0]   = Src1[63:0]
          // Dest[127:64] = Src2[63:0]
          return _VZip(DstSize, 8, Src1Node, Src2Node);
        case 0b11'10'11'10:
          // Combining of high 64-bits.
          // Dest[63:0]   = Src1[127:64]
          // Dest[127:64] = Src2[127:64]
          return _VZip2(DstSize, 8, Src1Node, Src2Node);
        case 0b11'10'01'00:
          // Mixing Low and high elements
          // Dest[63:0]   = Src1[63:0]
          // Dest[127:64] = Src2[127:64]
          return _VInsElement(DstSize, 8, 1, 1, Src1Node, Src2Node);
        case 0b01'00'11'10:
          // Mixing Low and high elements, inverse of above
          // Dest[63:0]   = Src1[127:64]
          // Dest[127:64] = Src2[63:0]
          return _VExtr(DstSize, 1, Src2Node, Src1Node, 8);
        case 0b10'00'10'00:
          // Mixing even elements.
          // Dest[31:0]   = Src1[31:0]
          // Dest[63:32]  = Src1[95:64]
          // Dest[95:64]  = Src2[31:0]
          // Dest[127:96] = Src2[95:64]
          return _VUnZip(DstSize, ElementSize, Src1Node, Src2Node);
        case 0b11'01'11'01:
          // Mixing odd elements.
          // Dest[31:0]   = Src1[63:32]
          // Dest[63:32]  = Src1[127:96]
          // Dest[95:64]  = Src2[63:32]
          // Dest[127:96] = Src2[127:96]
          return _VUnZip2(DstSize, ElementSize, Src1Node, Src2Node);
        case 0b11'10'00'00:
        case 0b11'10'01'01:
        case 0b11'10'10'10:
        case 0b11'10'11'11: {
          // Bottom elements duplicated, Top 64-bits inserted
          auto DupSrc1 = _VDupElement(DstSize, ElementSize, Src1Node, Shuffle & 0b11);
          return _VZip2(DstSize, 8, DupSrc1, Src2Node);
        }
        case 0b01'00'00'00:
        case 0b01'00'01'01:
        case 0b01'00'10'10:
        case 0b01'00'11'11: {
          // Bottom elements duplicated, Bottom 64-bits inserted
          auto DupSrc1 = _VDupElement(DstSize, ElementSize, Src1Node, Shuffle & 0b11);
          return _VZip(DstSize, 8, DupSrc1, Src2Node);
        }
        case 0b00'00'01'00:
        case 0b01'01'01'00:
        case 0b10'10'01'00:
        case 0b11'11'01'00: {
          // Top elements duplicated, Bottom 64-bits inserted
          auto DupSrc2 = _VDupElement(DstSize, ElementSize, Src2Node, (Shuffle >> 4) & 0b11);
          return _VZip(DstSize, 8, Src1Node, DupSrc2);
        }
        case 0b00'00'11'10:
        case 0b01'01'11'10:
        case 0b10'10'11'10:
        case 0b11'11'11'10: {
          // Top elements duplicated, Top 64-bits inserted
          auto DupSrc2 = _VDupElement(DstSize, ElementSize, Src2Node, (Shuffle >> 4) & 0b11);
          return _VZip2(DstSize, 8, Src1Node, DupSrc2);
        }
        case 0b01'00'01'11: {
          // TODO: This doesn't generate optimal code.
          // RA doesn't understand that Src1Node is dead after VInsElement due to SRA class differences.
          // With RA fixes this would be 2 instructions.
          // Odd elements inverted, Low 64-bits inserted
          Src1Node = _VInsElement(DstSize, 4, 0, 3, Src1Node, Src1Node);
          return _VZip(DstSize, 8, Src1Node, Src2Node);
        }
        case 0b11'10'01'11: {
          // TODO: This doesn't generate optimal code.
          // RA doesn't understand that Src1Node is dead after VInsElement due to SRA class differences.
          // With RA fixes this would be 2 instructions.
          // Odd elements inverted, Top 64-bits inserted
          Src1Node = _VInsElement(DstSize, 4, 0, 3, Src1Node, Src1Node);
          return _VInsElement(DstSize, 8, 1, 1, Src1Node, Src2Node);
        }
        case 0b01'00'00'01: {
          // Lower 32-bit elements inverted, low 64-bits inserted
          Src1Node = _VRev64(DstSize, 4, Src1Node);
          return _VZip(DstSize, 8, Src1Node, Src2Node);
        }
        case 0b11'10'00'01: {
          // TODO: This doesn't generate optimal code.
          // RA doesn't understand that Src1Node is dead after VInsElement due to SRA class differences.
          // With RA fixes this would be 2 instructions.
          // Lower 32-bit elements inverted, Top 64-bits inserted
          Src1Node = _VRev64(DstSize, 4, Src1Node);
          return _VInsElement(DstSize, 8, 1, 1, Src1Node, Src2Node);
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
        case 0b11'11'11'11:
        {
          // Duplicate element in upper and lower across each 64-bit segment.
          auto DupSrc1 = _VDupElement(DstSize, ElementSize, Src1Node, Shuffle & 0b11);
          auto DupSrc2 = _VDupElement(DstSize, ElementSize, Src2Node, (Shuffle >> 4) & 0b11);
          return _VZip(DstSize, 8, DupSrc1, DupSrc2);
        }
        default:
          // Use a TBL2 operation to handle this implementation. If the backend supports it.
          if (CTX->BackendFeatures.SupportsVTBL2) {
            auto LookupIndexes = LoadAndCacheIndexedNamedVectorConstant(DstSize, FEXCore::IR::IndexNamedVectorConstant::INDEXED_NAMED_VECTOR_SHUFPS, Shuffle * 16);
            return _VTBL2(DstSize, Src1Node, Src2Node, LookupIndexes);
          }
          break;
      }
    }
    else {
      switch (Shuffle & 0b11) {
        case 0b00:
          // Low 64-bits of each source interleaved.
          return _VZip(DstSize, ElementSize, Src1Node, Src2Node);
        case 0b01:
          // Upper 64-bits of Src1 in lower bits
          // Lower 64-bits of Src2 in upper bits.
          return _VExtr(DstSize, 1, Src2Node, Src1Node, 8);
        case 0b10:
          // Lower 32-bits of Src1 in lower bits.
          // Upper 64-bits of Src2 in upper bits.
          return _VInsElement(DstSize, ElementSize, 1, 1, Src1Node, Src2Node);
        case 0b11:
          // Upper 64-bits of each source interleaved.
          return _VZip2(DstSize, ElementSize, Src1Node, Src2Node);
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
  const auto SrcSize = GetSrcSize(Op);

  OrderedNode *Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  OrderedNode *Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);
  OrderedNode *Dest = _VBic(SrcSize, SrcSize, Src2, Src1);

  StoreResult(FPRClass, Op, Dest, -1);
}

template <IROps IROp, size_t ElementSize>
void OpDispatchBuilder::VHADDPOp(OpcodeArgs) {
  const auto SrcSize = GetSrcSize(Op);
  const auto Is256Bit = SrcSize == Core::CPUState::XMM_AVX_REG_SIZE;

  OrderedNode *Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  OrderedNode *Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);

  DeriveOp(Res, IROp, _VFAddP(SrcSize, ElementSize, Src1, Src2));

  OrderedNode *Dest = Res;
   if (Is256Bit) {
    Dest = _VInsElement(SrcSize, 8, 1, 2, Res, Res);
    Dest = _VInsElement(SrcSize, 8, 2, 1, Dest, Res);
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
  OrderedNode *Result{};

  if (Op->Src[0].IsGPR()) {
    OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
    Result = _VDupElement(DstSize, ElementSize, Src, 0);
  } else {
    // Get the address to broadcast from into a GPR.
    OrderedNode *Address = LoadSource_WithOpSize(GPRClass, Op, Op->Src[0], CTX->GetGPRSize(), Op->Flags,
                                                 {.LoadData = false});
    Address = AppendSegmentOffset(Address, Op->Flags);

    Result = _VBroadcastFromMem(DstSize, ElementSize, Address);
  }

  // No need to zero-extend result, since implementations
  // use zero extending AdvSIMD or zeroing SVE loads internally.

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

OrderedNode* OpDispatchBuilder::PINSROpImpl(OpcodeArgs, size_t ElementSize,
                                            const X86Tables::DecodedOperand& Src1Op,
                                            const X86Tables::DecodedOperand& Src2Op,
                                            const X86Tables::DecodedOperand& Imm) {
  const auto Size = GetDstSize(Op);
  const auto NumElements = Size / ElementSize;
  LOGMAN_THROW_A_FMT(Imm.IsLiteral(), "Imm needs to be literal here");
  const uint64_t Index = Imm.Data.Literal.Value & (NumElements - 1);
  OrderedNode *Src1 = LoadSource_WithOpSize(FPRClass, Op, Src1Op, Size, Op->Flags);

  if (Src2Op.IsGPR()) {
    // If the source is a GPR then convert directly from the GPR.
    auto Src2 = LoadSource_WithOpSize(GPRClass, Op, Src2Op, CTX->GetGPRSize(), Op->Flags);
    return _VInsGPR(Size, ElementSize, Index, Src1, Src2);
  }

  // If loading from memory then we only load the element size
  auto Src2 = LoadSource_WithOpSize(GPRClass, Op, Src2Op, ElementSize, Op->Flags, {.LoadData = false});
  return _VLoadVectorElement(Size, ElementSize, Src1, Index, Src2);
}

template<size_t ElementSize>
void OpDispatchBuilder::PINSROp(OpcodeArgs) {
  OrderedNode *Result = PINSROpImpl(Op, ElementSize, Op->Dest, Op->Src[0], Op->Src[1]);
  StoreResult(FPRClass, Op, Result, -1);
}

template
void OpDispatchBuilder::PINSROp<1>(OpcodeArgs);
template
void OpDispatchBuilder::PINSROp<2>(OpcodeArgs);
template
void OpDispatchBuilder::PINSROp<4>(OpcodeArgs);
template
void OpDispatchBuilder::PINSROp<8>(OpcodeArgs);

void OpDispatchBuilder::VPINSRBOp(OpcodeArgs) {
  OrderedNode *Result = PINSROpImpl(Op, 1, Op->Src[0], Op->Src[1], Op->Src[2]);
  if (Op->Dest.Data.GPR.GPR == Op->Src[0].Data.GPR.GPR) {
    Result = _VMov(16, Result);
  }
  StoreResult(FPRClass, Op, Result, -1);
}

void OpDispatchBuilder::VPINSRDQOp(OpcodeArgs) {
  const auto SrcSize = GetSrcSize(Op);
  OrderedNode *Result = PINSROpImpl(Op, SrcSize, Op->Src[0], Op->Src[1], Op->Src[2]);
  if (Op->Dest.Data.GPR.GPR == Op->Src[0].Data.GPR.GPR) {
    Result = _VMov(16, Result);
  }
  StoreResult(FPRClass, Op, Result, -1);
}

void OpDispatchBuilder::VPINSRWOp(OpcodeArgs) {
  OrderedNode *Result = PINSROpImpl(Op, 2, Op->Src[0], Op->Src[1], Op->Src[2]);
  if (Op->Dest.Data.GPR.GPR == Op->Src[0].Data.GPR.GPR) {
    Result = _VMov(16, Result);
  }
  StoreResult(FPRClass, Op, Result, -1);
}

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
    Dest = LoadSource_WithOpSize(FPRClass, Op, Src1, DstSize, Op->Flags);
  }

  if ((ZMask & (1 << CountD)) == 0) {
    // In the case that ZMask overwrites the destination element, then don't even insert
    OrderedNode *Src{};
    if (Src2.IsGPR()) {
      Src = LoadSource(FPRClass, Op, Src2, Op->Flags);
    } else {
      // If loading from memory then CountS is forced to zero
      CountS = 0;
      Src = LoadSource_WithOpSize(FPRClass, Op, Src2, 4, Op->Flags);
    }

    Dest = _VInsElement(DstSize, 4, CountD, CountS, Dest, Src);
  }

  // ZMask happens after insert
  if (ZMask == 0xF) {
    return LoadAndCacheNamedVectorConstant(DstSize, FEXCore::IR::NamedVectorConstant::NAMED_VECTOR_ZERO);
  }

  if (ZMask) {
    auto Zero = LoadAndCacheNamedVectorConstant(DstSize, FEXCore::IR::NamedVectorConstant::NAMED_VECTOR_ZERO);
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
  OrderedNode *Result = InsertPSOpImpl(Op, Op->Src[0], Op->Src[1], Op->Src[2]);
  StoreResult(FPRClass, Op, Result, -1);
}

template<size_t ElementSize>
void OpDispatchBuilder::PExtrOp(OpcodeArgs) {
  const auto DstSize = GetDstSize(Op);

  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  LOGMAN_THROW_A_FMT(Op->Src[1].IsLiteral(), "Src1 needs to be literal here");
  uint64_t Index = Op->Src[1].Data.Literal.Value;

  // Fixup of 32-bit element size.
  // When the element size is 32-bit then it can be overriden as 64-bit because the encoding of PEXTRD/PEXTRQ
  // is the same except that REX.W or VEX.W is set to 1. Incredibly frustrating.
  // Use the destination size as the element size in this case.
  size_t OverridenElementSize = ElementSize;
  if constexpr (ElementSize == 4) {
    OverridenElementSize = DstSize;
  }

  // AVX version only operates on 128-bit.
  const uint8_t NumElements = std::min<uint8_t>(GetSrcSize(Op), 16) / OverridenElementSize;
  Index &= NumElements - 1;

  if (Op->Dest.IsGPR()) {
    const uint8_t GPRSize = CTX->GetGPRSize();
    // Extract already zero extends the result.
    OrderedNode *Result = _VExtractToGPR(16, OverridenElementSize, Src, Index);
    StoreResult_WithOpSize(GPRClass, Op, Op->Dest, Result, GPRSize, -1);
    return;
  }

  // If we are storing to memory then we store the size of the element extracted
  OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.LoadData = false});
  _VStoreVectorElement(16, OverridenElementSize, Src, Index, Dest);
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

  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

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

  if (CTX->BackendFeatures.SupportsSaturatingRoundingShifts) {
    OrderedNode *Control = _VSQSHL(Size, ElementSize, Src2, (ElementSize * 8) - 1);
    Control = _VSRSHR(Size, ElementSize, Control, (ElementSize * 8) - 1);
    return _VMul(Size, ElementSize, Src1, Control);
  }
  else {
    auto NegVec = _VNeg(Size, ElementSize, Src1);
    OrderedNode *CmpLT = _VCMPLTZ(Size, ElementSize, Src2);
    OrderedNode *CmpEQ = _VCMPEQZ(Size, ElementSize, Src2);
    auto BSLResult = _VBSL(Size, CmpLT, NegVec, Src1);
    return _VBic(Size, Size, BSLResult, CmpEQ);
  }
}

template <size_t ElementSize>
void OpDispatchBuilder::PSIGN(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
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
  OrderedNode *Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  OrderedNode *Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);
  OrderedNode* Res = PSIGNImpl(Op, ElementSize, Src1, Src2);

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
  return _VUShrSWide(Size, ElementSize, Src, ShiftVec);
}

template<size_t ElementSize>
void OpDispatchBuilder::PSRLDOp(OpcodeArgs) {
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
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

  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  OrderedNode *Shift = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);
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
  LOGMAN_THROW_A_FMT(Op->Src[1].IsLiteral(), "Src[1] needs to be literal here");
  const uint64_t ShiftConstant = Op->Src[1].Data.Literal.Value;
  if (ShiftConstant == 0) [[unlikely]] {
    // Nothing to do, value is already in Dest.
    return;
  }

  const auto Size = GetSrcSize(Op);

  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  OrderedNode *Shift = _VUShrI(Size, ElementSize, Dest, ShiftConstant);
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

  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  OrderedNode *Result = Src;

  if (ShiftConstant != 0) [[likely]] {
    Result = _VUShrI(Size, ElementSize, Src, ShiftConstant);
  } else {
    if (Is128Bit) {
      Result = _VMov(16, Result);
    }
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
  if (Shift == 0) [[unlikely]] {
    // If zero-shift then just return the source.
    return Src;
  }
  const auto Size = GetSrcSize(Op);
  return _VShlI(Size, ElementSize, Src, Shift);
}

template<size_t ElementSize>
void OpDispatchBuilder::PSLLI(OpcodeArgs) {
  LOGMAN_THROW_A_FMT(Op->Src[1].IsLiteral(), "Src[1] needs to be literal here");
  const uint64_t ShiftConstant = Op->Src[1].Data.Literal.Value;
  if (ShiftConstant == 0) [[unlikely]] {
    // Nothing to do, value is already in Dest.
    return;
  }

  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
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
  LOGMAN_THROW_A_FMT(Op->Src[1].IsLiteral(), "Src1 needs to be literal here");
  const uint64_t ShiftConstant = Op->Src[1].Data.Literal.Value;
  const auto DstSize = GetDstSize(Op);
  const auto Is128Bit = DstSize == Core::CPUState::XMM_SSE_REG_SIZE;

  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  OrderedNode *Result = PSLLIImpl(Op, ElementSize, Src, ShiftConstant);
  if (ShiftConstant == 0 && Is128Bit) {
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
  const auto Size = GetDstSize(Op);

  // Incoming element size for the shift source is always 8
  return _VUShlSWide(Size, ElementSize, Src, ShiftVec);
}

template<size_t ElementSize>
void OpDispatchBuilder::PSLL(OpcodeArgs) {
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
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

  OrderedNode *Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  OrderedNode *Src2 = LoadSource_WithOpSize(FPRClass, Op, Op->Src[1], 16, Op->Flags);
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
  return _VSShrSWide(Size, ElementSize, Src, ShiftVec);
}

template<size_t ElementSize>
void OpDispatchBuilder::PSRAOp(OpcodeArgs) {
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
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

  OrderedNode *Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  OrderedNode *Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);
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
  const uint64_t Shift = Op->Src[1].Data.Literal.Value;
  if (Shift == 0) [[unlikely]] {
    // Nothing to do, value is already in Dest.
    return;
  }

  const auto Size = GetDstSize(Op);

  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  OrderedNode *Result = LoadAndCacheNamedVectorConstant(Size, FEXCore::IR::NamedVectorConstant::NAMED_VECTOR_ZERO);

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

  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

  OrderedNode *Result{};
  if (Shift == 0) [[unlikely]] {
    if (Is128Bit) {
      Result = _VMov(16, Src);
    } else {
      Result = Src;
    }
  } else {
    Result = LoadAndCacheNamedVectorConstant(DstSize, FEXCore::IR::NamedVectorConstant::NAMED_VECTOR_ZERO);

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
  }

  StoreResult(FPRClass, Op, Result, -1);
}

void OpDispatchBuilder::PSLLDQ(OpcodeArgs) {
  LOGMAN_THROW_A_FMT(Op->Src[1].IsLiteral(), "Src[1] needs to be literal here");
  const uint64_t Shift = Op->Src[1].Data.Literal.Value;
  if (Shift == 0) [[unlikely]] {
    // Nothing to do, value is already in Dest.
    return;
  }

  const auto Size = GetDstSize(Op);

  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  OrderedNode *Result = LoadAndCacheNamedVectorConstant(Size, FEXCore::IR::NamedVectorConstant::NAMED_VECTOR_ZERO);
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

  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

  OrderedNode *Result = Src;

  if (Shift == 0) {
    if (Is128Bit) {
      Result = _VMov(16, Result);
    }
  } else {
    Result = LoadAndCacheNamedVectorConstant(DstSize, FEXCore::IR::NamedVectorConstant::NAMED_VECTOR_ZERO);
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
  }
  StoreResult(FPRClass, Op, Result, -1);
}

template<size_t ElementSize>
void OpDispatchBuilder::PSRAIOp(OpcodeArgs) {
  LOGMAN_THROW_A_FMT(Op->Src[1].IsLiteral(), "Src[1] needs to be literal here");
  const uint64_t Shift = Op->Src[1].Data.Literal.Value;
  if (Shift == 0) [[unlikely]] {
    // Nothing to do, value is already in Dest.
    return;
  }

  const auto Size = GetDstSize(Op);

  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  OrderedNode *Result = _VSShrI(Size, ElementSize, Dest, Shift);
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

  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  OrderedNode *Result = Src;

  if (Shift != 0) [[likely]] {
    Result = _VSShrI(Size, ElementSize, Src, Shift);
  } else {
    if (Is128Bit) {
      Result = _VMov(16, Result);
    }
  }

  StoreResult(FPRClass, Op, Result, -1);
}

template
void OpDispatchBuilder::VPSRAIOp<2>(OpcodeArgs);
template
void OpDispatchBuilder::VPSRAIOp<4>(OpcodeArgs);

void OpDispatchBuilder::AVXVariableShiftImpl(OpcodeArgs, IROps IROp) {
  const auto DstSize = GetDstSize(Op);
  const auto SrcSize = GetSrcSize(Op);

  OrderedNode *Vector = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], DstSize, Op->Flags);
  OrderedNode *ShiftVector = LoadSource_WithOpSize(FPRClass, Op, Op->Src[1], DstSize, Op->Flags);

  DeriveOp(Shift, IROp, _VUShr(DstSize, SrcSize, Vector, ShiftVector, true));

  StoreResult(FPRClass, Op, Shift, -1);
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
  const auto SrcSize = Op->Src[0].IsGPR() ? 16U : GetSrcSize(Op);
  OrderedNode *Src = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], SrcSize, Op->Flags);
  OrderedNode *Res = _VDupElement(16, GetSrcSize(Op), Src, 0);

  StoreResult(FPRClass, Op, Res, -1);
}

void OpDispatchBuilder::VMOVDDUPOp(OpcodeArgs) {
  const auto SrcSize = GetSrcSize(Op);
  const auto IsSrcGPR = Op->Src[0].IsGPR();
  const auto Is256Bit = SrcSize == Core::CPUState::XMM_AVX_REG_SIZE;
  const auto MemSize = Is256Bit ? 32 : 8;

  OrderedNode *Src = IsSrcGPR ? LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], SrcSize, Op->Flags)
                              : LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], MemSize, Op->Flags);

  OrderedNode *Res{};
  if (Is256Bit) {
    Res = _VTrn(SrcSize, 8, Src, Src);
  } else {
    Res = _VDupElement(SrcSize, 8, Src, 0);
  }

  StoreResult(FPRClass, Op, Res, -1);
}

OrderedNode* OpDispatchBuilder::CVTGPR_To_FPRImpl(OpcodeArgs, size_t DstElementSize,
                                                  const X86Tables::DecodedOperand& Src1Op,
                                                  const X86Tables::DecodedOperand& Src2Op) {
  const auto SrcSize = GetSrcSize(Op);

  OrderedNode *Src1 = LoadSource_WithOpSize(FPRClass, Op, Src1Op, 16, Op->Flags);
  OrderedNode *Converted{};
  if (Src2Op.IsGPR()) {
    // If the source is a GPR then convert directly from the GPR.
    auto Src2 = LoadSource_WithOpSize(GPRClass, Op, Src2Op, CTX->GetGPRSize(), Op->Flags);
    Converted = _Float_FromGPR_S(DstElementSize, SrcSize, Src2);
  }
  else if (SrcSize != DstElementSize) {
    // If the source is from memory but the Source size and destination size aren't the same,
    // then it is more optimal to load in to a GPR and convert between GPR->FPR.
    // ARM GPR->FPR conversion supports different size source and destinations while FPR->FPR doesn't.
    auto Src2 = LoadSource(GPRClass, Op, Src2Op, Op->Flags);
    Converted = _Float_FromGPR_S(DstElementSize, SrcSize, Src2);
  }
  else {
    // In the case of cvtsi2s{s,d} where the source and destination are the same size,
    // then it is more optimal to load in to the FPR register directly and convert there.
    auto Src2 = LoadSource(FPRClass, Op, Src2Op, Op->Flags);
    Converted = _Vector_SToF(SrcSize, SrcSize, Src2);
  }

  return _VInsElement(16, DstElementSize, 0, 0, Src1, Converted);
}

template<size_t DstElementSize>
void OpDispatchBuilder::CVTGPR_To_FPR(OpcodeArgs) {
  OrderedNode *Result = CVTGPR_To_FPRImpl(Op, DstElementSize, Op->Dest, Op->Src[0]);
  StoreResult(FPRClass, Op, Result, -1);
}

template
void OpDispatchBuilder::CVTGPR_To_FPR<4>(OpcodeArgs);
template
void OpDispatchBuilder::CVTGPR_To_FPR<8>(OpcodeArgs);

template <size_t DstElementSize>
void OpDispatchBuilder::AVXCVTGPR_To_FPR(OpcodeArgs) {
  OrderedNode *Result = CVTGPR_To_FPRImpl(Op, DstElementSize, Op->Src[0], Op->Src[1]);
  StoreResult(FPRClass, Op, Result, -1);
}
template
void OpDispatchBuilder::AVXCVTGPR_To_FPR<4>(OpcodeArgs);
template
void OpDispatchBuilder::AVXCVTGPR_To_FPR<8>(OpcodeArgs);

template<size_t SrcElementSize, bool HostRoundingMode>
void OpDispatchBuilder::CVTFPR_To_GPR(OpcodeArgs) {
  // If loading a vector, use the full size, so we don't
  // unnecessarily zero extend the vector. Otherwise, if
  // memory, then we want to load the element size exactly.
  const auto SrcSize = Op->Src[0].IsGPR() ? 16U : GetSrcSize(Op);
  OrderedNode *Src = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], SrcSize, Op->Flags);

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
      // If loading a vector, use the full size, so we don't
      // unnecessarily zero extend the vector. Otherwise, if
      // memory, then we want to load the element size exactly.
      const auto LoadSize = Op->Src[0].IsGPR() ? 16U : 8 * (Size / 16);
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
  OrderedNode *Result = Vector_CVT_Int_To_FloatImpl(Op, SrcElementSize, Widen);
  StoreResult(FPRClass, Op, Result, -1);
}

template
void OpDispatchBuilder::Vector_CVT_Int_To_Float<4, true>(OpcodeArgs);
template
void OpDispatchBuilder::Vector_CVT_Int_To_Float<4, false>(OpcodeArgs);

template <size_t SrcElementSize, bool Widen>
void OpDispatchBuilder::AVXVector_CVT_Int_To_Float(OpcodeArgs) {
  OrderedNode *Result = Vector_CVT_Int_To_FloatImpl(Op, SrcElementSize, Widen);
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

  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

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

  // VCVTPD2DQ/VCVTTPD2DQ only use the bottom lane, even for the 256-bit version.
  const auto Truncate = SrcElementSize == 8 && Narrow;

  OrderedNode *Result = Vector_CVT_Float_To_IntImpl(Op, SrcElementSize, Narrow, HostRoundingMode);

  if (Truncate) {
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

OrderedNode* OpDispatchBuilder::Scalar_CVT_Float_To_FloatImpl(OpcodeArgs, size_t DstElementSize, size_t SrcElementSize,
                                                              const X86Tables::DecodedOperand& Src1Op,
                                                              const X86Tables::DecodedOperand& Src2Op) {
  // In the case of vectors, we can just specify the full vector length,
  // so that we don't unnecessarily zero-extend the entire vector.
  // Otherwise, if it's a memory load, then we only want to load its exact size.
  const auto Src2Size = Src2Op.IsGPR() ? 16U : SrcElementSize;

  OrderedNode *Src1 = LoadSource_WithOpSize(FPRClass, Op, Src1Op, 16, Op->Flags);
  OrderedNode *Src2 = LoadSource_WithOpSize(FPRClass, Op, Src2Op, Src2Size, Op->Flags);

  OrderedNode *Converted = _Float_FToF(DstElementSize, SrcElementSize, Src2);

  return _VInsElement(16, DstElementSize, 0, 0, Src1, Converted);
}

template<size_t DstElementSize, size_t SrcElementSize>
void OpDispatchBuilder::Scalar_CVT_Float_To_Float(OpcodeArgs) {
  OrderedNode *Result = Scalar_CVT_Float_To_FloatImpl(Op, DstElementSize, SrcElementSize, Op->Dest, Op->Src[0]);
  StoreResult(FPRClass, Op, Result, -1);
}

template
void OpDispatchBuilder::Scalar_CVT_Float_To_Float<4, 8>(OpcodeArgs);
template
void OpDispatchBuilder::Scalar_CVT_Float_To_Float<8, 4>(OpcodeArgs);

template <size_t DstElementSize, size_t SrcElementSize>
void OpDispatchBuilder::AVXScalar_CVT_Float_To_Float(OpcodeArgs) {
  OrderedNode *Result = Scalar_CVT_Float_To_FloatImpl(Op, DstElementSize, SrcElementSize, Op->Src[0], Op->Src[1]);
  StoreResult(FPRClass, Op, Result, -1);
}

template
void OpDispatchBuilder::AVXScalar_CVT_Float_To_Float<4, 8>(OpcodeArgs);
template
void OpDispatchBuilder::AVXScalar_CVT_Float_To_Float<8, 4>(OpcodeArgs);

void OpDispatchBuilder::Vector_CVT_Float_To_FloatImpl(OpcodeArgs, size_t DstElementSize, size_t SrcElementSize, bool IsAVX) {
  const auto SrcSize = GetSrcSize(Op);

  const auto IsFloatSrc = SrcElementSize == 4;
  const auto Is128Bit = SrcSize == Core::CPUState::XMM_SSE_REG_SIZE;

  const auto LoadSize = IsFloatSrc && !Op->Src[0].IsGPR() ?
    SrcSize / 2 :
    SrcSize;

  OrderedNode *Src = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], LoadSize, Op->Flags);

  OrderedNode *Result{};
  if (DstElementSize > SrcElementSize) {
    Result = _Vector_FToF(SrcSize, SrcElementSize << 1, Src, SrcElementSize);
  } else {
    Result = _Vector_FToF(SrcSize, SrcElementSize >> 1, Src, SrcElementSize);
  }

  if (IsAVX) {
    if (!IsFloatSrc && !Is128Bit) {
      // VCVTPD2PS path
      Result = _VMov(16, Result);
    } else if (IsFloatSrc && Is128Bit) {
      // VCVTPS2PD path
      Result = _VMov(16, Result);
    }
  }
  StoreResult(FPRClass, Op, Result, -1);
}

template<size_t DstElementSize, size_t SrcElementSize>
void OpDispatchBuilder::Vector_CVT_Float_To_Float(OpcodeArgs) {
  Vector_CVT_Float_To_FloatImpl(Op, DstElementSize, SrcElementSize, false);
}
template
void OpDispatchBuilder::Vector_CVT_Float_To_Float<4, 8>(OpcodeArgs);
template
void OpDispatchBuilder::Vector_CVT_Float_To_Float<8, 4>(OpcodeArgs);

template<size_t DstElementSize, size_t SrcElementSize>
void OpDispatchBuilder::AVXVector_CVT_Float_To_Float(OpcodeArgs) {
  Vector_CVT_Float_To_FloatImpl(Op, DstElementSize, SrcElementSize, true);
}
template
void OpDispatchBuilder::AVXVector_CVT_Float_To_Float<4, 8>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVector_CVT_Float_To_Float<8, 4>(OpcodeArgs);

void OpDispatchBuilder::MMX_To_XMM_Vector_CVT_Int_To_Float(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

  // Always 32-bit.
  size_t ElementSize = 4;
  size_t DstSize = GetDstSize(Op);

  Src = _VSXTL(DstSize, ElementSize, Src);
  ElementSize <<= 1;

  // Always signed
  Src = _Vector_SToF(DstSize, ElementSize, Src);

  StoreResult(FPRClass, Op, Src, -1);
}

template<size_t SrcElementSize, bool Narrow, bool HostRoundingMode>
void OpDispatchBuilder::XMM_To_MMX_Vector_CVT_Float_To_Int(OpcodeArgs) {
  // If loading a vector, use the full size, so we don't
  // unnecessarily zero extend the vector. Otherwise, if
  // memory, then we want to load the element size exactly.
  const auto SrcSize = Op->Src[0].IsGPR() ? 16U : GetSrcSize(Op);
  OrderedNode *Src = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], SrcSize, Op->Flags);

  size_t ElementSize = SrcElementSize;
  size_t Size = GetDstSize(Op);

  if (Narrow) {
    Src = _Vector_FToF(Size, SrcElementSize >> 1, Src, SrcElementSize);
    ElementSize >>= 1;
  }

  if constexpr (HostRoundingMode) {
    Src = _Vector_FToS(Size, ElementSize, Src);
  }
  else {
    Src = _Vector_FToZS(Size, ElementSize, Src);
  }

  StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Src, Size, -1);
}

template
void OpDispatchBuilder::XMM_To_MMX_Vector_CVT_Float_To_Int<4, false, false>(OpcodeArgs);
template
void OpDispatchBuilder::XMM_To_MMX_Vector_CVT_Float_To_Int<4, false, true>(OpcodeArgs);
template
void OpDispatchBuilder::XMM_To_MMX_Vector_CVT_Float_To_Int<8, true, false>(OpcodeArgs);
template
void OpDispatchBuilder::XMM_To_MMX_Vector_CVT_Float_To_Int<8, true, true>(OpcodeArgs);

void OpDispatchBuilder::MASKMOVOp(OpcodeArgs) {
  const auto Size = GetSrcSize(Op);

  OrderedNode *MaskSrc = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
  // Mask only cares about the top bit of each byte
  MaskSrc = _VCMPLTZ(Size, 1, MaskSrc);

  // Vector that will overwrite byte elements.
  OrderedNode *VectorSrc = LoadSource(GPRClass, Op, Op->Dest, Op->Flags);

  // RDI source
  auto MemDest = LoadGPRRegister(X86State::REG_RDI);

  // DS prefix by default.
  MemDest = AppendSegmentOffset(MemDest, Op->Flags, FEXCore::X86Tables::DecodeFlags::FLAG_DS_PREFIX);

  OrderedNode *XMMReg = _LoadMem(FPRClass, Size, MemDest, 1);

  // If the Mask element high bit is set then overwrite the element with the source, else keep the memory variant
  XMMReg = _VBSL(Size, MaskSrc, VectorSrc, XMMReg);
  _StoreMem(FPRClass, Size, MemDest, XMMReg, 1);
}

void OpDispatchBuilder::VMASKMOVOpImpl(OpcodeArgs, size_t ElementSize, size_t DataSize, bool IsStore,
                                       const X86Tables::DecodedOperand& MaskOp,
                                       const X86Tables::DecodedOperand& DataOp) {

  const auto MakeAddress = [this, Op](const X86Tables::DecodedOperand& Data) {
    OrderedNode *BaseAddr = LoadSource_WithOpSize(GPRClass, Op, Data, CTX->GetGPRSize(), Op->Flags,
                                                  {.LoadData = false});
    return AppendSegmentOffset(BaseAddr, Op->Flags);
  };

  OrderedNode *Mask = LoadSource_WithOpSize(FPRClass, Op, MaskOp, DataSize, Op->Flags);

  if (IsStore) {
    OrderedNode *Data = LoadSource_WithOpSize(FPRClass, Op, DataOp, DataSize, Op->Flags);
    OrderedNode *Address = MakeAddress(Op->Dest);
    _VStoreVectorMasked(DataSize, ElementSize, Mask, Data, Address, Invalid(), MEM_OFFSET_SXTX, 1);
  } else {
    const auto Is128Bit = GetDstSize(Op) == Core::CPUState::XMM_SSE_REG_SIZE;

    OrderedNode *Address = MakeAddress(DataOp);
    OrderedNode *Result = _VLoadVectorMasked(DataSize, ElementSize, Mask, Address, Invalid(), MEM_OFFSET_SXTX, 1);

    if (Is128Bit) {
      Result = _VMov(16, Result);
    }
    StoreResult(FPRClass, Op, Result, -1);
  }
}

template <size_t ElementSize, bool IsStore>
void OpDispatchBuilder::VMASKMOVOp(OpcodeArgs) {
  VMASKMOVOpImpl(Op, ElementSize, GetDstSize(Op), IsStore, Op->Src[0], Op->Src[1]);
}
template
void OpDispatchBuilder::VMASKMOVOp<4, false>(OpcodeArgs);
template
void OpDispatchBuilder::VMASKMOVOp<4, true>(OpcodeArgs);
template
void OpDispatchBuilder::VMASKMOVOp<8, false>(OpcodeArgs);
template
void OpDispatchBuilder::VMASKMOVOp<8, true>(OpcodeArgs);

template <bool IsStore>
void OpDispatchBuilder::VPMASKMOVOp(OpcodeArgs) {
  VMASKMOVOpImpl(Op, GetSrcSize(Op), GetDstSize(Op), IsStore, Op->Src[0], Op->Src[1]);
}
template
void OpDispatchBuilder::VPMASKMOVOp<false>(OpcodeArgs);
template
void OpDispatchBuilder::VPMASKMOVOp<true>(OpcodeArgs);

void OpDispatchBuilder::MOVBetweenGPR_FPR(OpcodeArgs) {
  if (Op->Dest.IsGPR() &&
      Op->Dest.Data.GPR.GPR >= FEXCore::X86State::REG_XMM_0) {
    if (Op->Src[0].IsGPR()) {
      // Loading from GPR and moving to Vector.
      OrderedNode *Src = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], CTX->GetGPRSize(), Op->Flags);
      // zext to 128bit
      auto Converted = _VCastFromGPR(16, GetSrcSize(Op), Src);
      StoreResult(FPRClass, Op, Op->Dest, Converted, -1);
    }
    else {
      // Loading from Memory as a scalar. Zero extend
      OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
      StoreResult(FPRClass, Op, Op->Dest, Src, -1);
    }
  }
  else {
    OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0],Op->Flags);

    if (Op->Dest.IsGPR()) {
      auto ElementSize = GetDstSize(Op);
      // Extract element from GPR. Zero extending in the process.
      Src = _VExtractToGPR(GetSrcSize(Op), ElementSize, Src, 0);
      StoreResult(GPRClass, Op, Op->Dest, Src, -1);
    }
    else {
      // Storing first element to memory.
      OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.LoadData = false});
      _StoreMem(FPRClass, GetDstSize(Op), Dest, Src, 1);
    }
  }
}

OrderedNode* OpDispatchBuilder::VFCMPOpImpl(OpcodeArgs, size_t ElementSize,
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

  return Result;
}

template<size_t ElementSize>
void OpDispatchBuilder::VFCMPOp(OpcodeArgs) {
  // No need for zero-extending in the scalar case, since
  // all we need is an insert at the end of the operation.
  const auto SrcSize = GetSrcSize(Op);
  const auto DstSize = GetDstSize(Op);

  OrderedNode *Src = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], SrcSize, Op->Flags);
  OrderedNode *Dest = LoadSource_WithOpSize(FPRClass, Op, Op->Dest, DstSize, Op->Flags);
  const uint8_t CompType = Op->Src[1].Data.Literal.Value;

  OrderedNode* Result = VFCMPOpImpl(Op, ElementSize, Dest, Src, CompType);

  StoreResult(FPRClass, Op, Result, -1);
}

template
void OpDispatchBuilder::VFCMPOp<4>(OpcodeArgs);
template
void OpDispatchBuilder::VFCMPOp<8>(OpcodeArgs);

template <size_t ElementSize>
void OpDispatchBuilder::AVXVFCMPOp(OpcodeArgs) {
  // No need for zero-extending in the scalar case, since
  // all we need is an insert at the end of the operation.
  const auto SrcSize = GetSrcSize(Op);
  const auto DstSize = GetDstSize(Op);

  LOGMAN_THROW_A_FMT(Op->Src[2].IsLiteral(), "Src[2] needs to be literal");
  const uint8_t CompType = Op->Src[2].Data.Literal.Value;

  OrderedNode *Src1 = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], DstSize, Op->Flags);
  OrderedNode *Src2 = LoadSource_WithOpSize(FPRClass, Op, Op->Src[1], SrcSize, Op->Flags);
  OrderedNode *Result = VFCMPOpImpl(Op, ElementSize, Src1, Src2, CompType);

  StoreResult(FPRClass, Op, Result, -1);
}

template
void OpDispatchBuilder::AVXVFCMPOp<4>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVFCMPOp<8>(OpcodeArgs);

void OpDispatchBuilder::FXSaveOp(OpcodeArgs) {
  OrderedNode *Mem = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.LoadData = false});
  Mem = AppendSegmentOffset(Mem, Op->Flags);

  SaveX87State(Op, Mem);
  SaveSSEState(Mem);
  SaveMXCSRState(Mem);
}

void OpDispatchBuilder::XSaveOp(OpcodeArgs) {
  XSaveOpImpl(Op);
}

OrderedNode *OpDispatchBuilder::XSaveBase(X86Tables::DecodedOp Op) {
  OrderedNode *Mem = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.LoadData = false});
  return AppendSegmentOffset(Mem, Op->Flags);
}

void OpDispatchBuilder::XSaveOpImpl(OpcodeArgs) {
  // NOTE: Mask should be EAX and EDX concatenated, but we only need to test
  //       for features that are in the lower 32 bits, so EAX only is sufficient.
  OrderedNode *Mask = LoadGPRRegister(X86State::REG_RAX);
  const auto OpSize = IR::SizeToOpSize(CTX->GetGPRSize());

  const auto StoreIfFlagSet = [&](uint32_t BitIndex, auto fn, uint32_t FieldSize = 1){
    OrderedNode *BitFlag = _Bfe(OpSize, FieldSize, BitIndex, Mask);
    auto CondJump_ = CondJump(BitFlag, {COND_NEQ});

    auto StoreBlock = CreateNewCodeBlockAfter(GetCurrentBlock());
    SetTrueJumpTarget(CondJump_, StoreBlock);
    SetCurrentCodeBlock(StoreBlock);
    StartNewBlock();
    {
      fn();
    }
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
  if (CTX->HostFeatures.SupportsAVX)
  {
    StoreIfFlagSet(2, [this, Op] { SaveAVXState(XSaveBase(Op)); });
  }

  // We need to save MXCSR and MXCSR_MASK if either SSE or AVX are requested to be saved
  {
    StoreIfFlagSet(1, [this, Op] { SaveMXCSRState(XSaveBase(Op)); }, 2);
  }

  // Update XSTATE_BV region of the XSAVE header
  {
    OrderedNode *Base = XSaveBase(Op);
    OrderedNode *HeaderOffset = _Add(OpSize, Base, _Constant(512));

    // NOTE: We currently only support the first 3 bits (x87, SSE, and AVX)
    OrderedNode *RequestedFeatures = _Bfe(OpSize, 3, 0, Mask);

    // XSTATE_BV section of the header is 8 bytes in size, but we only really
    // care about setting at most 3 bits in the first byte. We zero out the rest.
    _StoreMem(GPRClass, 8, HeaderOffset, RequestedFeatures);
  }
}

void OpDispatchBuilder::SaveX87State(OpcodeArgs, OrderedNode *MemBase) {
  const auto OpSize = IR::SizeToOpSize(CTX->GetGPRSize());
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
    _StoreMem(GPRClass, 2, MemBase, FCW, 2);
  }

  {
    OrderedNode *MemLocation = _Add(OpSize, MemBase, _Constant(2));
    _StoreMem(GPRClass, 2, MemLocation, ReconstructFSW(), 2);
  }

  {
    // Abridged FTW
    OrderedNode *MemLocation = _Add(OpSize, MemBase, _Constant(4));
    auto AbridgedFTW = _LoadContext(1, GPRClass, offsetof(FEXCore::Core::CPUState, AbridgedFTW));
    _StoreMem(GPRClass, 1, MemLocation, AbridgedFTW, 2);
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
  for (uint32_t i = 0; i < Core::CPUState::NUM_MMS; ++i) {
    OrderedNode *MMReg = _LoadContext(16, FPRClass, offsetof(FEXCore::Core::CPUState, mm[i]));
    OrderedNode *MemLocation = _Add(OpSize, MemBase, _Constant(i * 16 + 32));

    _StoreMem(FPRClass, 16, MemLocation, MMReg, 16);
  }
}

void OpDispatchBuilder::SaveSSEState(OrderedNode *MemBase) {
  const auto OpSize = IR::SizeToOpSize(CTX->GetGPRSize());
  const auto NumRegs = CTX->Config.Is64BitMode ? 16U : 8U;

  for (uint32_t i = 0; i < NumRegs; ++i) {
    OrderedNode *XMMReg = LoadXMMRegister(i);
    OrderedNode *MemLocation = _Add(OpSize, MemBase, _Constant(i * 16 + 160));

    _StoreMem(FPRClass, 16, MemLocation, XMMReg, 16);
  }
}

void OpDispatchBuilder::SaveMXCSRState(OrderedNode *MemBase) {
  const auto OpSize = IR::SizeToOpSize(CTX->GetGPRSize());

  OrderedNode *MXCSR = GetMXCSR();
  OrderedNode *MXCSRLocation = _Add(OpSize, MemBase, _Constant(24));
  _StoreMem(GPRClass, 4, MXCSRLocation, MXCSR, 4);

  // Store the mask for all bits.
  OrderedNode *MXCSRMaskLocation = _Add(OpSize, MXCSRLocation, _Constant(4));
  _StoreMem(GPRClass, 4, MXCSRMaskLocation, _Constant(0xFFFF), 4);
}

void OpDispatchBuilder::SaveAVXState(OrderedNode *MemBase) {
  const auto OpSize = IR::SizeToOpSize(CTX->GetGPRSize());
  const auto NumRegs = CTX->Config.Is64BitMode ? 16U : 8U;

  for (uint32_t i = 0; i < NumRegs; ++i) {
    OrderedNode *Upper = _VDupElement(32, 16, LoadXMMRegister(i), 1);
    OrderedNode *MemLocation = _Add(OpSize, MemBase, _Constant(i * 16 + 576));

    _StoreMem(FPRClass, 16, MemLocation, Upper, 16);
  }
}

OrderedNode *OpDispatchBuilder::GetMXCSR() {
  // Default MXCSR Value
  OrderedNode *MXCSR = _Constant(0x1F80);
  OrderedNode *RoundingMode = _GetRoundingMode();
  return _Bfi(OpSize::i32Bit, 3, 13, MXCSR, RoundingMode);
}

void OpDispatchBuilder::FXRStoreOp(OpcodeArgs) {
  const auto OpSize = IR::SizeToOpSize(CTX->GetGPRSize());

  OrderedNode *Mem = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, {.LoadData = false});
  Mem = AppendSegmentOffset(Mem, Op->Flags);

  RestoreX87State(Mem);
  RestoreSSEState(Mem);

  OrderedNode *MXCSRLocation = _Add(OpSize, Mem, _Constant(24));
  OrderedNode *MXCSR = _LoadMem(GPRClass, 4, MXCSRLocation, 4);
  RestoreMXCSRState(MXCSR);
}

void OpDispatchBuilder::XRstorOpImpl(OpcodeArgs) {
  const auto OpSize = IR::SizeToOpSize(CTX->GetGPRSize());

  // Set up base address for the XSAVE region to restore from, and also read the
  // XSTATE_BV bit flags out of the XSTATE header.
  //
  // Note: we rematerialize Base in each block to avoid crossblock liveness.
  OrderedNode *Base = XSaveBase(Op);
  OrderedNode *Mask = _LoadMem(GPRClass, 8, _Add(OpSize, Base, _Constant(512)), 8);

  // If a bit in our XSTATE_BV is set, then we restore from that region of the XSAVE area,
  // otherwise, if not set, then we need to set the relevant data the bit corresponds to
  // to it's defined initial configuration.
  const auto RestoreIfFlagSetOrDefault = [&](uint32_t BitIndex, auto restore_fn, auto default_fn, uint32_t FieldSize = 1){
    OrderedNode *BitFlag = _Bfe(OpSize, FieldSize, BitIndex, Mask);
    auto CondJump_ = CondJump(BitFlag, {COND_NEQ});

    auto RestoreBlock = CreateNewCodeBlockAfter(GetCurrentBlock());
    SetTrueJumpTarget(CondJump_, RestoreBlock);
    SetCurrentCodeBlock(RestoreBlock);
    StartNewBlock();
    {
      restore_fn();
    }
    auto RestoreExitJump = Jump();
    auto DefaultBlock = CreateNewCodeBlockAfter(RestoreBlock);
    auto ExitBlock = CreateNewCodeBlockAfter(DefaultBlock);
    SetJumpTarget(RestoreExitJump, ExitBlock);
    SetFalseJumpTarget(CondJump_, DefaultBlock);
    SetCurrentCodeBlock(DefaultBlock);
    StartNewBlock();
    {
      default_fn();
    }
    auto DefaultExitJump = Jump();
    SetJumpTarget(DefaultExitJump, ExitBlock);
    SetCurrentCodeBlock(ExitBlock);
    StartNewBlock();
  };

  // x87
  {
    RestoreIfFlagSetOrDefault(0,
                              [this, Op] { RestoreX87State(XSaveBase(Op)); },
                              [this, Op] { DefaultX87State(Op); });
  }
  // SSE
  {
    RestoreIfFlagSetOrDefault(1,
                              [this, Op] { RestoreSSEState(XSaveBase(Op)); },
                              [this] { DefaultSSEState(); });
  }
  // AVX
  if (CTX->HostFeatures.SupportsAVX)
  {
    RestoreIfFlagSetOrDefault(2,
                              [this, Op] { RestoreAVXState(XSaveBase(Op)); },
                              [this] { DefaultAVXState(); });
  }

  {
    // We need to restore the MXCSR if either SSE or AVX are requested to be saved
    RestoreIfFlagSetOrDefault(1,
                              [this, Op, OpSize] {
                                OrderedNode *Base = XSaveBase(Op);
                                OrderedNode *MXCSRLocation = _Add(OpSize, Base, _Constant(24));
                                OrderedNode *MXCSR = _LoadMem(GPRClass, 4, MXCSRLocation, 4);
                                RestoreMXCSRState(MXCSR);
                              },
                              [] { /* Intentionally do nothing*/ }, 2);
  }
}

void OpDispatchBuilder::RestoreX87State(OrderedNode *MemBase) {
  const auto OpSize = IR::SizeToOpSize(CTX->GetGPRSize());

  auto NewFCW = _LoadMem(GPRClass, 2, MemBase, 2);
  _StoreContext(2, GPRClass, NewFCW, offsetof(FEXCore::Core::CPUState, FCW));

  {
    OrderedNode *MemLocation = _Add(OpSize, MemBase, _Constant(2));
    auto NewFSW = _LoadMem(GPRClass, 2, MemLocation, 2);
    ReconstructX87StateFromFSW(NewFSW);
  }

  {
    // Abridged FTW
    OrderedNode *MemLocation = _Add(OpSize, MemBase, _Constant(4));
    auto NewAbridgedFTW = _LoadMem(GPRClass, 1, MemLocation, 2);
    _StoreContext(1, GPRClass, NewAbridgedFTW, offsetof(FEXCore::Core::CPUState, AbridgedFTW));
  }

  for (uint32_t i = 0; i < Core::CPUState::NUM_MMS; ++i) {
    OrderedNode *MemLocation = _Add(OpSize, MemBase, _Constant(i * 16 + 32));
    auto MMReg = _LoadMem(FPRClass, 16, MemLocation, 16);
    _StoreContext(16, FPRClass, MMReg, offsetof(FEXCore::Core::CPUState, mm[i]));
  }
}

void OpDispatchBuilder::RestoreSSEState(OrderedNode *MemBase) {
  const auto OpSize = IR::SizeToOpSize(CTX->GetGPRSize());
  const auto NumRegs = CTX->Config.Is64BitMode ? 16U : 8U;

  for (uint32_t i = 0; i < NumRegs; ++i) {
    OrderedNode *MemLocation = _Add(OpSize, MemBase, _Constant(i * 16 + 160));
    OrderedNode *XMMReg = _LoadMem(FPRClass, 16, MemLocation, 16);
    StoreXMMRegister(i, XMMReg);
  }
}

void OpDispatchBuilder::RestoreMXCSRState(OrderedNode *MXCSR) {
  // We only support the rounding mode and FTZ bit being set
  OrderedNode *RoundingMode = _Bfe(OpSize::i32Bit, 3, 13, MXCSR);
  _SetRoundingMode(RoundingMode);
}

void OpDispatchBuilder::RestoreAVXState(OrderedNode *MemBase) {
  const auto OpSize = IR::SizeToOpSize(CTX->GetGPRSize());
  const auto NumRegs = CTX->Config.Is64BitMode ? 16U : 8U;

  for (uint32_t i = 0; i < NumRegs; ++i) {
    OrderedNode *XMMReg = LoadXMMRegister(i);
    OrderedNode *MemLocation = _Add(OpSize, MemBase, _Constant(i * 16 + 576));
    OrderedNode *YMMHReg = _LoadMem(FPRClass, 16, MemLocation, 16);
    OrderedNode *YMM = _VInsElement(32, 16, 1, 0, XMMReg, YMMHReg);
    StoreXMMRegister(i, YMM);
  }
}

void OpDispatchBuilder::DefaultX87State(OpcodeArgs) {
  // We can piggy-back on FNINIT's implementation, since
  // it performs the same behavior as required by XRSTOR for resetting flags
  FNINIT(Op);

  // On top of resetting the flags to a default state, we also need to clear
  // all of the ST0-7/MM0-7 registers to zero.
  OrderedNode *ZeroVector = LoadAndCacheNamedVectorConstant(Core::CPUState::MM_REG_SIZE, FEXCore::IR::NamedVectorConstant::NAMED_VECTOR_ZERO);
  for (uint32_t i = 0; i < Core::CPUState::NUM_MMS; ++i) {
    _StoreContext(16, FPRClass, ZeroVector, offsetof(FEXCore::Core::CPUState, mm[i]));
  }
}

void OpDispatchBuilder::DefaultSSEState() {
  const auto NumRegs = CTX->Config.Is64BitMode ? 16U : 8U;

  OrderedNode *ZeroVector = LoadAndCacheNamedVectorConstant(Core::CPUState::XMM_SSE_REG_SIZE, FEXCore::IR::NamedVectorConstant::NAMED_VECTOR_ZERO);
  for (uint32_t i = 0; i < NumRegs; ++i) {
    StoreXMMRegister(i, ZeroVector);
  }
}

void OpDispatchBuilder::DefaultAVXState() {
  const auto NumRegs = CTX->Config.Is64BitMode ? 16U : 8U;

  for (uint32_t i = 0; i < NumRegs; i++) {
      OrderedNode* Reg = LoadXMMRegister(i);
      OrderedNode* Dst = _VMov(16, Reg);
      StoreXMMRegister(i, Dst);
    }
}

OrderedNode* OpDispatchBuilder::PALIGNROpImpl(OpcodeArgs, const X86Tables::DecodedOperand& Src1,
                                              const X86Tables::DecodedOperand& Src2,
                                              const X86Tables::DecodedOperand& Imm,
                                              bool IsAVX) {
  LOGMAN_THROW_A_FMT(Imm.IsLiteral(), "Imm needs to be a literal");
  // For the 256-bit case we handle it as pairs of 128-bit halves.
  const auto DstSize = GetDstSize(Op);
  const auto SanitizedDstSize = std::min(DstSize, uint8_t{16});

  const auto Is256Bit = DstSize == Core::CPUState::XMM_AVX_REG_SIZE;
  const auto Index = Imm.Data.Literal.Value;

  OrderedNode *Src2Node = LoadSource(FPRClass, Op, Src2, Op->Flags);
  if (Index == 0) {
    if (IsAVX && !Is256Bit) {
      // 128-bit AVX needs to zero the upper bits.
      return _VMov(16, Src2Node);
    }
    else {
      return Src2Node;
    }
  }
  OrderedNode *Src1Node = LoadSource(FPRClass, Op, Src1, Op->Flags);

  if (Index >= (SanitizedDstSize * 2)) {
    // If the immediate is greater than both vectors combined then it zeroes the vector
    return LoadAndCacheNamedVectorConstant(DstSize, FEXCore::IR::NamedVectorConstant::NAMED_VECTOR_ZERO);
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
  OrderedNode *Result = PALIGNROpImpl(Op, Op->Dest, Op->Src[0], Op->Src[1], false);
  StoreResult(FPRClass, Op, Result, -1);
}

void OpDispatchBuilder::VPALIGNROp(OpcodeArgs) {
  OrderedNode *Result = PALIGNROpImpl(Op, Op->Src[0], Op->Src[1], Op->Src[2], true);
  StoreResult(FPRClass, Op, Result, -1);
}

template<size_t ElementSize>
void OpDispatchBuilder::UCOMISxOp(OpcodeArgs) {
  const auto SrcSize = Op->Src[0].IsGPR() ? GetGuestVectorLength() : GetSrcSize(Op);
  OrderedNode *Src1 = LoadSource_WithOpSize(FPRClass, Op, Op->Dest, GetGuestVectorLength(), Op->Flags);
  OrderedNode *Src2 = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], SrcSize, Op->Flags);

  HandleNZCVWrite();
  _FCmp(ElementSize, Src1, Src2);
  ConvertNZCVToSSE();

  // Zero AF. Note that the comparison sets the raw PF to 0/1 above, so PF[4] is
  // 0 so the XOR with PF will have no effect, so setting the AF byte to zero
  // will indeed zero AF as intended.
  SetRFLAG<FEXCore::X86State::RFLAG_AF_RAW_LOC>(_Constant(0));
}

template
void OpDispatchBuilder::UCOMISxOp<4>(OpcodeArgs);
template
void OpDispatchBuilder::UCOMISxOp<8>(OpcodeArgs);

void OpDispatchBuilder::LDMXCSR(OpcodeArgs) {
  OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags);
  RestoreMXCSRState(Dest);
}

void OpDispatchBuilder::STMXCSR(OpcodeArgs) {
  StoreResult(GPRClass, Op, GetMXCSR(), -1);
}

OrderedNode* OpDispatchBuilder::PACKUSOpImpl(OpcodeArgs, size_t ElementSize,
                                             OrderedNode *Src1, OrderedNode *Src2) {
  const auto Size = GetSrcSize(Op);

  return _VSQXTUNPair(Size, ElementSize, Src1, Src2);
}

template<size_t ElementSize>
void OpDispatchBuilder::PACKUSOp(OpcodeArgs) {
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
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
  const auto Is256Bit = DstSize == Core::CPUState::XMM_AVX_REG_SIZE;

  OrderedNode *Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  OrderedNode *Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);
  OrderedNode *Result = PACKUSOpImpl(Op, ElementSize, Src1, Src2);

  if (Is256Bit) {
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
  return _VSQXTNPair(Size, ElementSize, Src1, Src2);
}

template<size_t ElementSize>
void OpDispatchBuilder::PACKSSOp(OpcodeArgs) {
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
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
  const auto Is256Bit = DstSize == Core::CPUState::XMM_AVX_REG_SIZE;

  OrderedNode *Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  OrderedNode *Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);
  OrderedNode *Result = PACKSSOpImpl(Op, ElementSize, Src1, Src2);

  if (Is256Bit) {
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
  static_assert(ElementSize == sizeof(uint32_t),
                "Currently only handles 32-bit -> 64-bit");

  OrderedNode *Src1 = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  OrderedNode *Src2 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
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

  OrderedNode *Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  OrderedNode *Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);
  OrderedNode *Result = PMULLOpImpl(Op, ElementSize, Signed, Src1, Src2);

  StoreResult(FPRClass, Op, Result, -1);
}

template
void OpDispatchBuilder::VPMULLOp<4, false>(OpcodeArgs);
template
void OpDispatchBuilder::VPMULLOp<4, true>(OpcodeArgs);

template<bool ToXMM>
void OpDispatchBuilder::MOVQ2DQ(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

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

  if (CTX->HostFeatures.SupportsFCMA) {
    if (ElementSize == 4) {
      auto Swizzle = _VRev64(Size, 4, Src2);
      return _VFCADD(Size, ElementSize, Src1, Swizzle, 90);
    }
    else {
      auto Swizzle = _VExtr(Size, 1, Src2, Src2, 8);
      return _VFCADD(Size, ElementSize, Src1, Swizzle, 90);
    }
  }
  else {
    auto ConstantEOR = LoadAndCacheNamedVectorConstant(Size, ElementSize == 4 ? NAMED_VECTOR_PADDSUBPS_INVERT : NAMED_VECTOR_PADDSUBPD_INVERT);
    auto InvertedSource = _VXor(Size, ElementSize, Src2, ConstantEOR);
    return _VFAdd(Size, ElementSize, Src1, InvertedSource);
  }
}

template<size_t ElementSize>
void OpDispatchBuilder::ADDSUBPOp(OpcodeArgs) {
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  OrderedNode *Result = ADDSUBPOpImpl(Op, ElementSize, Dest, Src);

  StoreResult(FPRClass, Op, Result, -1);
}

template
void OpDispatchBuilder::ADDSUBPOp<4>(OpcodeArgs);
template
void OpDispatchBuilder::ADDSUBPOp<8>(OpcodeArgs);

template<size_t ElementSize>
void OpDispatchBuilder::VADDSUBPOp(OpcodeArgs) {
  OrderedNode *Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  OrderedNode *Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);
  OrderedNode *Result = ADDSUBPOpImpl(Op, ElementSize, Src1, Src2);

  StoreResult(FPRClass, Op, Result, -1);
}

template
void OpDispatchBuilder::VADDSUBPOp<4>(OpcodeArgs);
template
void OpDispatchBuilder::VADDSUBPOp<8>(OpcodeArgs);

void OpDispatchBuilder::PFNACCOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);

  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

  auto DestUnzip = _VUnZip(Size, 4, Dest, Src);
  auto SrcUnzip = _VUnZip2(Size, 4, Dest, Src);
  auto Result = _VFSub(Size, 4, DestUnzip, SrcUnzip);

  StoreResult(FPRClass, Op, Result, -1);
}

void OpDispatchBuilder::PFPNACCOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);

  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

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
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

  auto Result = _VRev64(Size, 4, Src);
  StoreResult(FPRClass, Op, Result, -1);
}

void OpDispatchBuilder::PI2FWOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

  size_t Size = GetDstSize(Op);

  // We now need to transpose the lower 16-bits of each element together
  // Only needing to move the upper element down in this case
  Src = _VUnZip(Size, 2, Src, Src);

  // Now we need to sign extend the 16bit value to 32-bit
  Src = _VSXTL(Size, 2, Src);

  // int32_t to float
  Src = _Vector_SToF(Size, 4, Src);

  StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Src, Size, -1);
}

void OpDispatchBuilder::PF2IWOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

  size_t Size = GetDstSize(Op);

  // Float to int32_t
  Src = _Vector_FToZS(Size, 4, Src);

  // We now need to transpose the lower 16-bits of each element together
  // Only needing to move the upper element down in this case
  Src = _VUnZip(Size, 2, Src, Src);

  // Now we need to sign extend the 16bit value to 32-bit
  Src = _VSXTL(Size, 2, Src);
  StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Src, Size, -1);
}

void OpDispatchBuilder::PMULHRWOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);

  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

  OrderedNode *Res{};

  // Implementation is more efficient for 8byte registers
  // Multiplies 4 16bit values in to 4 32bit values
  Res = _VSMull(Size * 2, 2, Dest, Src);

  // Load 0x0000_8000 in to each 32-bit element.
  OrderedNode *VConstant = _VectorImm(16, 4, 0x80, 8);

  Res = _VAdd(Size * 2, 4, Res, VConstant);

  // Now shift and narrow to convert 32-bit values to 16bit, storing the top 16bits
  Res = _VUShrNI(Size * 2, 4, Res, 16);

  StoreResult(FPRClass, Op, Res, -1);
}

template<uint8_t CompType>
void OpDispatchBuilder::VPFCMPOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  OrderedNode *Dest = LoadSource_WithOpSize(FPRClass, Op, Op->Dest, GetDstSize(Op), Op->Flags);

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

  OrderedNode *Src1Node = LoadSource(FPRClass, Op, Src1, Op->Flags);
  OrderedNode *Src2Node = LoadSource(FPRClass, Op, Src2, Op->Flags);

  if (Size == 8) {
    // MMX implementation can be slightly more optimal
    Size <<= 1;
    auto MullResult = _VSMull(Size, 2, Src1Node, Src2Node);
    return _VAddP(Size, 4, MullResult, MullResult);
  }

  auto Lower = _VSMull(Size, 2, Src1Node, Src2Node);
  auto Upper = _VSMull2(Size, 2, Src1Node, Src2Node);

  // [15:0 ] + [31:16], [32:47 ] + [63:48  ], [79:64] + [95:80], [111:96] + [127:112]
  return _VAddP(Size, 4, Lower, Upper);
}

void OpDispatchBuilder::PMADDWD(OpcodeArgs) {
  OrderedNode *Result = PMADDWDOpImpl(Op, Op->Dest, Op->Src[0]);
  StoreResult(FPRClass, Op, Result, -1);
}

void OpDispatchBuilder::VPMADDWDOp(OpcodeArgs) {
  OrderedNode *Result = PMADDWDOpImpl(Op, Op->Src[0], Op->Src[1]);
  StoreResult(FPRClass, Op, Result, -1);
}

OrderedNode* OpDispatchBuilder::PMADDUBSWOpImpl(OpcodeArgs, const X86Tables::DecodedOperand& Src1Op,
                                                const X86Tables::DecodedOperand& Src2Op) {
  const auto Size = GetSrcSize(Op);

  OrderedNode *Src1 = LoadSource(FPRClass, Op, Src1Op, Op->Flags);
  OrderedNode *Src2 = LoadSource(FPRClass, Op, Src2Op, Op->Flags);

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
    return _VSQXTN(Size * 2, 4, ResAdd);
  }

  // V{U,S}XTL{,2}/ and VUnZip{,2} can be optimized in this solution to save about one instruction.
  // We can up-front zero extend and sign extend the elements in-place.
  // This means extracting even and odd elements up-front so the unzips aren't required.
  // Requires implementing IR ops for BIC (vector, immediate) although.

  // Src1 is unsigned
  auto Src1_16b_L = _VUXTL(Size, 1, Src1);  // [7:0 ], [15:8], [23:16], [31:24], [39:32], [47:40], [55:48], [63:56]
  auto Src2_16b_L = _VSXTL(Size, 1, Src2);  // [7:0 ], [15:8], [23:16], [31:24], [39:32], [47:40], [55:48], [63:56]
  auto ResMul_L   = _VMul(Size, 2, Src1_16b_L, Src2_16b_L);

  // Src2 is signed
  auto Src1_16b_H = _VUXTL2(Size, 1, Src1);  // Offset to +64bits [7:0 ], [15:8], [23:16], [31:24], [39:32], [47:40], [55:48], [63:56]
  auto Src2_16b_H = _VSXTL2(Size, 1, Src2);  // Offset to +64bits [7:0 ], [15:8], [23:16], [31:24], [39:32], [47:40], [55:48], [63:56]
  auto ResMul_L_H = _VMul(Size, 2, Src1_16b_H, Src2_16b_H);

  auto TmpZip1 = _VUnZip(Size, 2, ResMul_L, ResMul_L_H);
  auto TmpZip2 = _VUnZip2(Size, 2, ResMul_L, ResMul_L_H);

  return _VSQAdd(Size, 2, TmpZip1, TmpZip2);
}

void OpDispatchBuilder::PMADDUBSW(OpcodeArgs) {
  OrderedNode * Result = PMADDUBSWOpImpl(Op, Op->Dest, Op->Src[0]);
  StoreResult(FPRClass, Op, Result, -1);
}

void OpDispatchBuilder::VPMADDUBSWOp(OpcodeArgs) {
  OrderedNode * Result = PMADDUBSWOpImpl(Op, Op->Src[0], Op->Src[1]);
  StoreResult(FPRClass, Op, Result, -1);
}

OrderedNode* OpDispatchBuilder::PMULHWOpImpl(OpcodeArgs, bool Signed,
                                             OrderedNode *Src1, OrderedNode *Src2) {
  const auto Size = GetSrcSize(Op);
  if (Signed) {
    return _VSMulH(Size, 2, Src1, Src2);
  }
  else {
    return _VUMulH(Size, 2, Src1, Src2);
  }
}

template<bool Signed>
void OpDispatchBuilder::PMULHW(OpcodeArgs) {
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
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

  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);
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
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  OrderedNode *Result = PMULHRSWOpImpl(Op, Dest, Src);

  StoreResult(FPRClass, Op, Result, -1);
}

void OpDispatchBuilder::VPMULHRSWOp(OpcodeArgs) {
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);
  OrderedNode *Result = PMULHRSWOpImpl(Op, Dest, Src);

  StoreResult(FPRClass, Op, Result, -1);
}

OrderedNode* OpDispatchBuilder::HSUBPOpImpl(OpcodeArgs, size_t ElementSize,
                                            const X86Tables::DecodedOperand& Src1Op,
                                            const X86Tables::DecodedOperand& Src2Op) {
  const auto SrcSize = GetSrcSize(Op);

  OrderedNode *Src1 = LoadSource(FPRClass, Op, Src1Op, Op->Flags);
  OrderedNode *Src2 = LoadSource(FPRClass, Op, Src2Op, Op->Flags);

  auto Even = _VUnZip(SrcSize, ElementSize, Src1, Src2);
  auto Odd = _VUnZip2(SrcSize, ElementSize, Src1, Src2);
  return _VFSub(SrcSize, ElementSize, Even, Odd);
}

template<size_t ElementSize>
void OpDispatchBuilder::HSUBP(OpcodeArgs) {
  OrderedNode *Result = HSUBPOpImpl(Op, ElementSize, Op->Dest, Op->Src[0]);
  StoreResult(FPRClass, Op, Result, -1);
}

template
void OpDispatchBuilder::HSUBP<4>(OpcodeArgs);
template
void OpDispatchBuilder::HSUBP<8>(OpcodeArgs);

template <size_t ElementSize>
void OpDispatchBuilder::VHSUBPOp(OpcodeArgs) {
  const auto DstSize = GetDstSize(Op);
  const auto Is256Bit = DstSize == Core::CPUState::XMM_AVX_REG_SIZE;

  OrderedNode *Result = HSUBPOpImpl(Op, ElementSize, Op->Src[0], Op->Src[1]);
  OrderedNode *Dest = Result;
  if (Is256Bit) {
    Dest = _VInsElement(DstSize, 8, 1, 2, Result, Result);
    Dest = _VInsElement(DstSize, 8, 2, 1, Dest, Result);
  }

  StoreResult(FPRClass, Op, Dest, -1);
}

template
void OpDispatchBuilder::VHSUBPOp<4>(OpcodeArgs);
template
void OpDispatchBuilder::VHSUBPOp<8>(OpcodeArgs);

OrderedNode* OpDispatchBuilder::PHSUBOpImpl(OpcodeArgs, const X86Tables::DecodedOperand& Src1,
                                            const X86Tables::DecodedOperand& Src2, size_t ElementSize) {
  const auto Size = GetSrcSize(Op);

  OrderedNode *Src1V = LoadSource(FPRClass, Op, Src1, Op->Flags);
  OrderedNode *Src2V = LoadSource(FPRClass, Op, Src2, Op->Flags);

  auto Even = _VUnZip(Size, ElementSize, Src1V, Src2V);
  auto Odd = _VUnZip2(Size, ElementSize, Src1V, Src2V);
  return _VSub(Size, ElementSize, Even, Odd);
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
  const auto Is256Bit = DstSize == Core::CPUState::XMM_AVX_REG_SIZE;

  OrderedNode *Result = PHSUBOpImpl(Op, Op->Src[0], Op->Src[1], ElementSize);
  if (Is256Bit) {
    OrderedNode *Inserted = _VInsElement(DstSize, 8, 1, 2, Result, Result);
    Result = _VInsElement(DstSize, 8, 2, 1, Inserted, Result);
  }
  StoreResult(FPRClass, Op, Result, -1);
}

template
void OpDispatchBuilder::VPHSUBOp<2>(OpcodeArgs);
template
void OpDispatchBuilder::VPHSUBOp<4>(OpcodeArgs);

OrderedNode* OpDispatchBuilder::PHADDSOpImpl(OpcodeArgs, const X86Tables::DecodedOperand& Src1Op,
                                             const X86Tables::DecodedOperand& Src2Op) {
  const auto Size = GetSrcSize(Op);
  const uint8_t ElementSize = 2;

  OrderedNode *Src1 = LoadSource(FPRClass, Op, Src1Op, Op->Flags);
  OrderedNode *Src2 = LoadSource(FPRClass, Op, Src2Op, Op->Flags);

  auto Even = _VUnZip(Size, ElementSize, Src1, Src2);
  auto Odd = _VUnZip2(Size, ElementSize, Src1, Src2);

  // Saturate back down to the result
  return _VSQAdd(Size, ElementSize, Even, Odd);
}

void OpDispatchBuilder::PHADDS(OpcodeArgs) {
  OrderedNode *Result = PHADDSOpImpl(Op, Op->Dest, Op->Src[0]);
  StoreResult(FPRClass, Op, Result, -1);
}

void OpDispatchBuilder::VPHADDSWOp(OpcodeArgs) {
  const auto SrcSize = GetSrcSize(Op);
  const auto Is256Bit = SrcSize == Core::CPUState::XMM_AVX_REG_SIZE;

  OrderedNode *Result = PHADDSOpImpl(Op, Op->Src[0], Op->Src[1]);
  OrderedNode *Dest = Result;

  if (Is256Bit) {
    Dest = _VInsElement(SrcSize, 8, 1, 2, Result, Result);
    Dest = _VInsElement(SrcSize, 8, 2, 1, Dest, Result);
  }

  StoreResult(FPRClass, Op, Dest, -1);
}

OrderedNode* OpDispatchBuilder::PHSUBSOpImpl(OpcodeArgs, const X86Tables::DecodedOperand& Src1Op,
                                             const X86Tables::DecodedOperand& Src2Op) {
  const auto Size = GetSrcSize(Op);
  const uint8_t ElementSize = 2;

  OrderedNode *Src1 = LoadSource(FPRClass, Op, Src1Op, Op->Flags);
  OrderedNode *Src2 = LoadSource(FPRClass, Op, Src2Op, Op->Flags);

  auto Even = _VUnZip(Size, ElementSize, Src1, Src2);
  auto Odd = _VUnZip2(Size, ElementSize, Src1, Src2);

  // Saturate back down to the result
  return _VSQSub(Size, ElementSize, Even, Odd);
}

void OpDispatchBuilder::PHSUBS(OpcodeArgs) {
  OrderedNode *Result = PHSUBSOpImpl(Op, Op->Dest, Op->Src[0]);
  StoreResult(FPRClass, Op, Result, -1);
}

void OpDispatchBuilder::VPHSUBSWOp(OpcodeArgs) {
  const auto DstSize = GetDstSize(Op);
  const auto Is256Bit = DstSize == Core::CPUState::XMM_AVX_REG_SIZE;

  OrderedNode *Result = PHSUBSOpImpl(Op, Op->Src[0], Op->Src[1]);
  OrderedNode *Dest = Result;
  if (Is256Bit) {
    Dest = _VInsElement(DstSize, 8, 1, 2, Result, Result);
    Dest = _VInsElement(DstSize, 8, 2, 1, Dest, Result);
  }

  StoreResult(FPRClass, Op, Dest, -1);
}

OrderedNode* OpDispatchBuilder::PSADBWOpImpl(OpcodeArgs,
                                             const X86Tables::DecodedOperand& Src1Op,
                                             const X86Tables::DecodedOperand& Src2Op) {
  // The documentation is actually incorrect in how this instruction operates
  // It strongly implies that the `abs(dest[i] - src[i])` operates in 8bit space
  // but it actually operates in more than 8bit space
  // This can be seen with `abs(0 - 0xFF)` returning a different result depending
  // on bit length
  const auto Size = GetSrcSize(Op);
  const auto Is128Bit = Size == Core::CPUState::XMM_SSE_REG_SIZE;

  OrderedNode *Src1 = LoadSource(FPRClass, Op, Src1Op, Op->Flags);
  OrderedNode *Src2 = LoadSource(FPRClass, Op, Src2Op, Op->Flags);

  if (Size == 8) {
    auto AbsResult = _VUABDL(Size * 2, 1, Src1, Src2);

    // Now vector-wide add the results for each
    return _VAddV(Size * 2, 2, AbsResult);
  }

  auto AbsResult_Low = _VUABDL(Size, 1, Src1, Src2);
  auto AbsResult_High = _VUABDL2(Size, 1, Src1, Src2);

  OrderedNode *Result_Low = _VAddV(16, 2, AbsResult_Low);
  OrderedNode *Result_High = _VAddV(16, 2, AbsResult_High);
  auto Low = _VZip(Size, 8, Result_Low, Result_High);

  if (Is128Bit) {
    return Low;
  }

  OrderedNode *HighSrc1 = _VDupElement(Size, 16, AbsResult_Low, 1);
  OrderedNode *HighSrc2 = _VDupElement(Size, 16, AbsResult_High, 1);

  OrderedNode *HighResult_Low = _VAddV(16, 2, HighSrc1);
  OrderedNode *HighResult_High = _VAddV(16, 2, HighSrc2);

  OrderedNode *High = _VInsElement(Size, 8, 1, 0, HighResult_Low, HighResult_High);
  OrderedNode *Full = _VInsElement(Size, 16, 1, 0, Low, High);

  OrderedNode *Tmp = _VInsElement(Size, 8, 2, 1, Full, Full);
  return _VInsElement(Size, 8, 1, 2, Tmp, Full);
}

void OpDispatchBuilder::PSADBW(OpcodeArgs) {
  OrderedNode *Result = PSADBWOpImpl(Op, Op->Dest, Op->Src[0]);
  StoreResult(FPRClass, Op, Result, -1);
}

void OpDispatchBuilder::VPSADBWOp(OpcodeArgs) {
  OrderedNode *Result = PSADBWOpImpl(Op, Op->Src[0], Op->Src[1]);
  StoreResult(FPRClass, Op, Result, -1);
}

OrderedNode* OpDispatchBuilder::ExtendVectorElementsImpl(OpcodeArgs, size_t ElementSize,
                                                         size_t DstElementSize, bool Signed) {
  const auto DstSize = GetDstSize(Op);

  const auto GetSrc = [&] {
    if (Op->Src[0].IsGPR()) {
      return LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], DstSize, Op->Flags);
    } else {
      // For memory operands the 256-bit variant loads twice the size specified in the table.
      const auto Is256Bit = DstSize == Core::CPUState::XMM_AVX_REG_SIZE;
      const auto SrcSize = GetSrcSize(Op);
      const auto LoadSize = Is256Bit ? SrcSize * 2 : SrcSize;

      return LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], LoadSize, Op->Flags);
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

  const auto SourceMode = SourceModes[(RoundControlSource << 2) | RoundControl];
  return _Vector_FToI(Size, ElementSize, Src, SourceMode);
}

template<size_t ElementSize>
void OpDispatchBuilder::VectorRound(OpcodeArgs) {
  // No need to zero extend the vector in the event we have a
  // scalar source, especially since it's only inserted into another vector.
  const auto SrcSize = GetSrcSize(Op);
  OrderedNode *Src = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], SrcSize, Op->Flags);

  LOGMAN_THROW_A_FMT(Op->Src[1].IsLiteral(), "Src1 needs to be literal here");
  const uint64_t Mode = Op->Src[1].Data.Literal.Value;

  Src = VectorRoundImpl(Op, ElementSize, Src, Mode);

  StoreResult(FPRClass, Op, Src, -1);
}

template
void OpDispatchBuilder::VectorRound<4>(OpcodeArgs);
template
void OpDispatchBuilder::VectorRound<8>(OpcodeArgs);

template <size_t ElementSize>
void OpDispatchBuilder::AVXVectorRound(OpcodeArgs) {
  LOGMAN_THROW_A_FMT(Op->Src[1].IsLiteral(), "Src1 needs to be literal here");
  const auto Mode = Op->Src[1].Data.Literal.Value;

  // No need to zero extend the vector in the event we have a
  // scalar source, especially since it's only inserted into another vector.
  const auto SrcSize = GetSrcSize(Op);

  OrderedNode *Src = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], SrcSize, Op->Flags);
  OrderedNode *Result = VectorRoundImpl(Op, ElementSize, Src, Mode);

  StoreResult(FPRClass, Op, Result, -1);
}

template
void OpDispatchBuilder::AVXVectorRound<4>(OpcodeArgs);
template
void OpDispatchBuilder::AVXVectorRound<8>(OpcodeArgs);

template<size_t ElementSize>
void OpDispatchBuilder::VectorBlend(OpcodeArgs) {
  const auto DstSize = GetDstSize(Op);

  LOGMAN_THROW_A_FMT(Op->Src[1].IsLiteral(), "Src1 needs to be literal here");
  uint8_t Select = Op->Src[1].Data.Literal.Value;

  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

  if constexpr (ElementSize == 4) {
    Select &= 0b1111;
    switch (Select) {
      case 0b0000:
        // No-op
        return;
      case 0b0001:
        // Dest[31:0] = Src[31:0]
        Dest = _VInsElement(DstSize, ElementSize, 0, 0, Dest, Src);
        break;
      case 0b0010:
        // Dest[63:32] = Src[63:32]
        Dest = _VInsElement(DstSize, ElementSize, 1, 1, Dest, Src);
        break;
      case 0b0011:
        // Dest[31:0] = Src[31:0]
        // Dest[63:32] = Src[63:32]
        Dest = _VInsElement(DstSize, 8, 0, 0, Dest, Src);
        break;
      case 0b0100:
        // Dest[95:64] = Src[95:64]
        Dest = _VInsElement(DstSize, ElementSize, 2, 2, Dest, Src);
        break;
      case 0b0101: {
        // Dest[31:0]   = Src[31:0]
        // Dest[63:32]  = Dest[63:32]
        // Dest[95:64]  = Src[95:64]
        // Dest[127:96] = Dest[127:96]
        // Rotate the elements of the incoming source so they end up in the correct location.
        // Then trn2 keeps the destination results in the expected location.
        auto Temp = _VRev64(DstSize, 4, Src);
        Dest = _VTrn2(DstSize, ElementSize, Temp, Dest);
        break;
      }
      case 0b0110: {
        // Dest[63:32] = Src[63:32]
        // Dest[95:64] = Src[95:64]
        auto ConstantSwizzle = LoadAndCacheNamedVectorConstant(DstSize, FEXCore::IR::NamedVectorConstant::NAMED_VECTOR_BLENDPS_0110B);
        Dest = _VTBX1(DstSize, Dest, Src, ConstantSwizzle);
        break;
      }
      case 0b0111: {
        // Dest[31:0]  = Src[31:0]
        // Dest[63:32] = Src[63:32]
        // Dest[95:64] = Src[95:64]
        auto ConstantSwizzle = LoadAndCacheNamedVectorConstant(DstSize, FEXCore::IR::NamedVectorConstant::NAMED_VECTOR_BLENDPS_0111B);
        Dest = _VTBX1(DstSize, Dest, Src, ConstantSwizzle);
        break;
      }
      case 0b1000:
        // Dest[127:96] = Src[127:96]
        Dest = _VInsElement(DstSize, ElementSize, 3, 3, Dest, Src);
        break;
      case 0b1001: {
        // Dest[31:0]   = Src[31:0]
        // Dest[127:96] = Src[127:96]
        auto ConstantSwizzle = LoadAndCacheNamedVectorConstant(DstSize, FEXCore::IR::NamedVectorConstant::NAMED_VECTOR_BLENDPS_1001B);
        Dest = _VTBX1(DstSize, Dest, Src, ConstantSwizzle);
        break;
      }
      case 0b1010: {
        // Dest[31:0]   = Dest[31:0]
        // Dest[63:32]  = Src[63:32]
        // Dest[95:64]  = Dest[95:64]
        // Dest[127:96] = Src[127:96]
        // Rotate the elements of the incoming destination so they end up in the correct location.
        // Then trn2 keeps the source results in the expected location.
        auto Temp = _VRev64(DstSize, 4, Dest);
        Dest = _VTrn2(DstSize, ElementSize, Temp, Src);
        break;
      }
      case 0b1011: {
        // Dest[31:0]  = Src[31:0]
        // Dest[63:32] = Src[63:32]
        // Dest[95:64] = Src[95:64]
        auto ConstantSwizzle = LoadAndCacheNamedVectorConstant(DstSize, FEXCore::IR::NamedVectorConstant::NAMED_VECTOR_BLENDPS_1011B);
        Dest = _VTBX1(DstSize, Dest, Src, ConstantSwizzle);
        break;
      }
      case 0b1100:
        // Dest[95:64] = Src[95:64]
        // Dest[127:96] = Src[127:96]
        Dest = _VInsElement(DstSize, 8, 1, 1, Dest, Src);
        break;
      case 0b1101: {
        // Dest[31:0]  = Src[31:0]
        // Dest[95:64] = Src[95:64]
        // Dest[127:96] = Src[127:96]
        auto ConstantSwizzle = LoadAndCacheNamedVectorConstant(DstSize, FEXCore::IR::NamedVectorConstant::NAMED_VECTOR_BLENDPS_1101B);
        Dest = _VTBX1(DstSize, Dest, Src, ConstantSwizzle);
        break;
      }
      case 0b1110: {
        // Dest[63:32] = Src[63:32]
        // Dest[95:64] = Src[95:64]
        // Dest[127:96] = Src[127:96]
        auto ConstantSwizzle = LoadAndCacheNamedVectorConstant(DstSize, FEXCore::IR::NamedVectorConstant::NAMED_VECTOR_BLENDPS_1110B);
        Dest = _VTBX1(DstSize, Dest, Src, ConstantSwizzle);
        break;
      }
      case 0b1111:
        // Copy
        Dest = Src;
        break;
      default: break;
    }
  }
  else if constexpr (ElementSize == 8) {
    Select &= 0b11;
    switch (Select) {
      case 0b00:
        // No-op
        return;
      case 0b01:
        // Dest[63:0] = Src[63:0]
        Dest = _VInsElement(DstSize, ElementSize, 0, 0, Dest, Src);
        break;
      case 0b10:
        // Dest[127:64] = Src[127:64]
        Dest = _VInsElement(DstSize, ElementSize, 1, 1, Dest, Src);
        break;
      case 0b11:
        // Copy
        Dest = Src;
        break;
    }
  }
  else {
    // TODO: There are some of these swizzles that can be more optimal.
    // NamedConstant + VTBX1 is quite quick already.
    // Implement more if it becomes relevant.
    switch (Select) {
      case 0b0000'0000:
        // No-op
        return;
      case 0b0000'0001:
      case 0b0000'0010:
      case 0b0000'0100:
      case 0b0000'1000:
      case 0b0001'0000:
      case 0b0010'0000:
      case 0b0100'0000:
      case 0b1000'0000: {
        // Single 16-bit element insert.
        const auto Element = FEXCore::ilog2(Select);
        Dest = _VInsElement(DstSize, ElementSize, Element, Element, Dest, Src);
        break;
      }
      case 0b0000'0011:
      case 0b0000'1100:
      case 0b0011'0000:
      case 0b1100'0000: {
        // Single 32-bit element insert.
        const auto Element = std::countr_zero(Select) / 2;
        Dest = _VInsElement(DstSize, 4, Element, Element, Dest, Src);
        break;
      }
      case 0b0000'1111:
      case 0b1111'0000: {
        // Single 64-bit element insert.
        const auto Element = std::countr_zero(Select) / 4;
        Dest = _VInsElement(DstSize, 8, Element, Element, Dest, Src);
        break;
      }
      case 0b1111'1111:
        // Copy
        Dest = Src;
        break;
      default: {
        auto ConstantSwizzle = LoadAndCacheIndexedNamedVectorConstant(DstSize, FEXCore::IR::IndexNamedVectorConstant::INDEXED_NAMED_VECTOR_PBLENDW, Select * 16);
        Dest = _VTBX1(DstSize, Dest, Src, ConstantSwizzle);
        break;
      }
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

  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

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

  OrderedNode *Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  OrderedNode *Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);

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

  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

  OrderedNode *Test1 = _VAnd(Size, 1, Dest, Src);
  OrderedNode *Test2 = _VBic(Size, 1, Src, Dest);

  // Element size must be less than 32-bit for the sign bit tricks.
  Test1 = _VUMaxV(Size, 2, Test1);
  Test2 = _VUMaxV(Size, 2, Test2);

  Test1 = _VExtractToGPR(Size, 2, Test1, 0);
  Test2 = _VExtractToGPR(Size, 2, Test2, 0);

  auto ZeroConst = _Constant(0);
  auto OneConst = _Constant(1);

  Test2 = _Select(FEXCore::IR::COND_EQ,
      Test2, ZeroConst, OneConst, ZeroConst);

  // Careful, these flags are different between {V,}PTEST and VTESTP{S,D}
  // Set ZF according to Test1. SF will be zeroed since we do a 32-bit test on
  // the results of a 16-bit value from the UMaxV, so the 32-bit sign bit is
  // cleared even if the 16-bit scalars were negative.
  SetNZ_ZeroCV(32, Test1);
  SetRFLAG<FEXCore::X86State::RFLAG_CF_RAW_LOC>(Test2);

  ZeroPF_AF();
}

void OpDispatchBuilder::VTESTOpImpl(OpcodeArgs, size_t ElementSize) {
  InvalidateDeferredFlags();

  const auto SrcSize = GetSrcSize(Op);
  const auto ElementSizeInBits = ElementSize * 8;
  const auto MaskConstant = uint64_t{1} << (ElementSizeInBits - 1);

  OrderedNode *Src1 = LoadSource(FPRClass, Op, Op->Dest, Op->Flags);
  OrderedNode *Src2 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

  OrderedNode *Mask = _VDupFromGPR(SrcSize, ElementSize, _Constant(MaskConstant));

  OrderedNode *AndTest = _VAnd(SrcSize, 1, Src2, Src1);
  OrderedNode *AndNotTest = _VBic(SrcSize, 1, Src2, Src1);

  OrderedNode *MaskedAnd = _VAnd(SrcSize, 1, AndTest, Mask);
  OrderedNode *MaskedAndNot = _VAnd(SrcSize, 1, AndNotTest, Mask);

  OrderedNode *MaxAnd = _VUMaxV(SrcSize, 2, MaskedAnd);
  OrderedNode *MaxAndNot = _VUMaxV(SrcSize, 2, MaskedAndNot);

  OrderedNode *AndGPR = _VExtractToGPR(SrcSize, 2, MaxAnd, 0);
  OrderedNode *AndNotGPR = _VExtractToGPR(SrcSize, 2, MaxAndNot, 0);

  OrderedNode *ZeroConst = _Constant(0);
  OrderedNode *OneConst = _Constant(1);

  OrderedNode *CFResult = _Select(IR::COND_EQ, AndNotGPR, ZeroConst,
                                  OneConst, ZeroConst);

  // As in PTest, this sets Z appropriately while zeroing the rest of NZCV.
  SetNZ_ZeroCV(32, AndGPR);
  SetRFLAG<X86State::RFLAG_CF_RAW_LOC>(CFResult);

  ZeroPF_AF();
}

template <size_t ElementSize>
void OpDispatchBuilder::VTESTPOp(OpcodeArgs) {
  VTESTOpImpl(Op, ElementSize);
}
template
void OpDispatchBuilder::VTESTPOp<4>(OpcodeArgs);
template
void OpDispatchBuilder::VTESTPOp<8>(OpcodeArgs);

OrderedNode* OpDispatchBuilder::PHMINPOSUWOpImpl(OpcodeArgs) {
  const auto Size = GetSrcSize(Op);

  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

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

  auto ZipLower = _VZip(Size, 2, ConstantSwizzle, Src);
  auto ZipUpper = _VZip2(Size, 2, ConstantSwizzle, Src);
  // The elements are now 32-bit between two vectors.
  auto MinBetween = _VUMin(Size, 4, ZipLower, ZipUpper);

  // Now do a horizontal vector minimum
  auto Min = _VUMinV(Size, 4, MinBetween);

  // We now have a value in the bottom 32-bits in the order of:
  // [31:0]: (Src[<Min>] << 16) | <Index>
  // This instruction wants it in the form of:
  // [31:0]: (<Index> << 16) | Src[<Min>]
  // Rev32 does this for us
  return _VRev32(Size, 2, Min);
}

void OpDispatchBuilder::PHMINPOSUWOp(OpcodeArgs) {
  OrderedNode *Result = PHMINPOSUWOpImpl(Op);
  StoreResult(FPRClass, Op, Result, -1);
}

OrderedNode* OpDispatchBuilder::DPPOpImpl(OpcodeArgs, const X86Tables::DecodedOperand& Src1,
                                          const X86Tables::DecodedOperand& Src2,
                                          const X86Tables::DecodedOperand& Imm, size_t ElementSize) {
  LOGMAN_THROW_A_FMT(Imm.IsLiteral(), "Imm needs to be literal here");
  const uint8_t Mask = Imm.Data.Literal.Value;
  const auto SizeMask = [ElementSize]() {
    if (ElementSize == 4) {
      return 0b1111;
    }
    return 0b11;
  }();

  const uint8_t SrcMask = (Mask >> 4) & SizeMask;
  const uint8_t DstMask = Mask & SizeMask;

  const auto NamedIndexMask = [ElementSize]() {
    if (ElementSize == 4) {
      return FEXCore::IR::IndexNamedVectorConstant::INDEXED_NAMED_VECTOR_DPPS_MASK;
    }

    return FEXCore::IR::IndexNamedVectorConstant::INDEXED_NAMED_VECTOR_DPPD_MASK;
  }();
  const auto DstSize = GetDstSize(Op);

  OrderedNode *ZeroVec = LoadAndCacheNamedVectorConstant(DstSize, FEXCore::IR::NamedVectorConstant::NAMED_VECTOR_ZERO);
  if (SrcMask == 0 || DstMask == 0) {
    // What are you even doing here? Go away.
    return ZeroVec;
  }

  OrderedNode *Src1V = LoadSource(FPRClass, Op, Src1, Op->Flags);
  OrderedNode *Src2V = LoadSource(FPRClass, Op, Src2, Op->Flags);

  // First step is to do an FMUL
  OrderedNode *Temp = _VFMul(DstSize, ElementSize, Src1V, Src2V);

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
      default:
        LOGMAN_MSG_A_FMT("Unsupported");
    }
  }
  else {
    auto BadPath = [&]() {
      OrderedNode *Result = ZeroVec;

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
        return _VZip(DstSize, 8, ZeroVec, Temp);
      case 0b0101:
        // Dest[31:0]   = Result
        // Dest[63:32]  = Zero
        // Dest[95:64]  = Result
        // Dest[127:96] = Zero
        return _VZip(DstSize, 8, Temp, Temp);
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
        return _VExtr(DstSize, 1, Temp, ZeroVec, 4);
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
        return _VZip(DstSize, 4, ZeroVec, Temp);
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
        return _VZip(DstSize, 8, ZeroVec, Temp);
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
      default:
        LOGMAN_MSG_A_FMT("Unsupported");
    }
  }
  FEX_UNREACHABLE;
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

OrderedNode* OpDispatchBuilder::VDPPSOpImpl(OpcodeArgs, const X86Tables::DecodedOperand& Src1,
                         const X86Tables::DecodedOperand& Src2,
                         const X86Tables::DecodedOperand& Imm) {
  LOGMAN_THROW_A_FMT(Imm.IsLiteral(), "Imm needs to be literal here");
  constexpr size_t ElementSize = 4;
  const uint8_t Mask = Imm.Data.Literal.Value;
  const uint8_t SrcMask = Mask >> 4;
  const uint8_t DstMask = Mask & 0xF;

  const auto DstSize = GetDstSize(Op);

  OrderedNode *Src1V = LoadSource(FPRClass, Op, Src1, Op->Flags);
  OrderedNode *Src2V = LoadSource(FPRClass, Op, Src2, Op->Flags);

  OrderedNode *ZeroVec = LoadAndCacheNamedVectorConstant(DstSize, FEXCore::IR::NamedVectorConstant::NAMED_VECTOR_ZERO);

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

template <size_t ElementSize>
void OpDispatchBuilder::VDPPOp(OpcodeArgs) {
  const auto DstSize = GetDstSize(Op);

  OrderedNode *Result{};
  if (ElementSize == 4 && DstSize == Core::CPUState::XMM_AVX_REG_SIZE) {
    // 256-bit DPPS isn't handled by the 128-bit solution.
    Result = VDPPSOpImpl(Op, Op->Src[0], Op->Src[1], Op->Src[2]);
  }
  else {
    Result = DPPOpImpl(Op, Op->Src[0], Op->Src[1], Op->Src[2], ElementSize);
  }

  // We don't need to emit a _VMov to clear the upper lane, since DPPOpImpl uses a zero vector
  // to construct the results, so the upper lane will always be cleared for the 128-bit version.
  StoreResult(FPRClass, Op, Result, -1);
}

template
void OpDispatchBuilder::VDPPOp<4>(OpcodeArgs);
template
void OpDispatchBuilder::VDPPOp<8>(OpcodeArgs);

OrderedNode* OpDispatchBuilder::MPSADBWOpImpl(OpcodeArgs, const X86Tables::DecodedOperand& Src1Op,
                                              const X86Tables::DecodedOperand& Src2Op,
                                              const X86Tables::DecodedOperand& ImmOp) {
  const auto LaneHelper = [&, this](uint32_t Selector_Src1, uint32_t Selector_Src2,
                                    OrderedNode *Src1, OrderedNode *Src2) {
    // Src2 will grab a 32bit element and duplicate it across the 128bits
    OrderedNode *DupSrc = _VDupElement(16, 4, Src2, Selector_Src2);

    // Src1/Dest needs a bunch of magic

    // Shift right by selected bytes
    // This will give us Dest[15:0], and Dest[79:64]
    OrderedNode *Dest1 = _VExtr(16, 1, Src1, Src1, Selector_Src1 + 0);
    // This will give us Dest[31:16], and Dest[95:80]
    OrderedNode *Dest2 = _VExtr(16, 1, Src1, Src1, Selector_Src1 + 1);
    // This will give us Dest[47:32], and Dest[111:96]
    OrderedNode *Dest3 = _VExtr(16, 1, Src1, Src1, Selector_Src1 + 2);
    // This will give us Dest[63:48], and Dest[127:112]
    OrderedNode *Dest4 = _VExtr(16, 1, Src1, Src1, Selector_Src1 + 3);

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
    auto TmpCombine1 = _VAddP(16, 2, Dest1, Dest3);
    auto TmpCombine2 = _VAddP(16, 2, Dest2, Dest4);

    // TmpTranspose1:
    // VTrn TmpCombine1, TmpCombine2: TmpTranspose1
    // Transposes Even and odd elements so we can use vaddp for final results.
    auto TmpTranspose1 = _VTrn(16, 4, TmpCombine1, TmpCombine2);
    auto TmpTranspose2 = _VTrn2(16, 4, TmpCombine1, TmpCombine2);

    // ADDP TmpTranspose1, TmpTranspose2: FinalCombine
    //    FinalCombine.8H[0] = TmpTranspose1.8H[0] + TmpTranspose1.8H[1]
    //    FinalCombine.8H[1] = TmpTranspose1.8H[2] + TmpTranspose1.8H[3]
    //    FinalCombine.8H[2] = TmpTranspose1.8H[4] + TmpTranspose1.8H[5]
    //    FinalCombine.8H[3] = TmpTranspose1.8H[6] + TmpTranspose1.8H[7]
    //    FinalCombine.8H[4] = TmpTranspose2.8H[0] + TmpTranspose2.8H[1]
    //    FinalCombine.8H[5] = TmpTranspose2.8H[2] + TmpTranspose2.8H[3]
    //    FinalCombine.8H[6] = TmpTranspose2.8H[4] + TmpTranspose2.8H[5]
    //    FinalCombine.8H[7] = TmpTranspose2.8H[6] + TmpTranspose2.8H[7]

    return _VAddP(16, 2, TmpTranspose1, TmpTranspose2);
  };

  LOGMAN_THROW_A_FMT(ImmOp.IsLiteral(), "ImmOp needs to be literal here");
  const uint8_t Select = ImmOp.Data.Literal.Value;
  const uint8_t SrcSize = GetSrcSize(Op);
  const auto Is128Bit = SrcSize == Core::CPUState::XMM_SSE_REG_SIZE;

  // Src1 needs to be in byte offset
  const uint8_t Select_Src1_Low = ((Select & 0b100) >> 2) * 32 / 8;
  const uint8_t Select_Src2_Low = Select & 0b11;

  OrderedNode *Src1 = LoadSource(FPRClass, Op, Src1Op, Op->Flags);
  OrderedNode *Src2 = LoadSource(FPRClass, Op, Src2Op, Op->Flags);

  OrderedNode *Lower = LaneHelper(Select_Src1_Low, Select_Src2_Low, Src1, Src2);
  if (Is128Bit) {
    return Lower;
  }

  const uint8_t Select_Src1_High = ((Select & 0b100000) >> 5) * 32 / 8;
  const uint8_t Select_Src2_High = (Select & 0b11000) >> 3;

  OrderedNode *UpperSrc1 = _VDupElement(32, 16, Src1, 1);
  OrderedNode *UpperSrc2 = _VDupElement(32, 16, Src2, 1);
  OrderedNode *Upper = LaneHelper(Select_Src1_High, Select_Src2_High, UpperSrc1, UpperSrc2);
  return _VInsElement(32, 16, 1, 0, Lower, Upper);
}

void OpDispatchBuilder::MPSADBWOp(OpcodeArgs) {
  OrderedNode *Result = MPSADBWOpImpl(Op, Op->Dest, Op->Src[0], Op->Src[1]);
  StoreResult(FPRClass, Op, Result, -1);
}

void OpDispatchBuilder::VMPSADBWOp(OpcodeArgs) {
  OrderedNode *Result = MPSADBWOpImpl(Op, Op->Src[0], Op->Src[1], Op->Src[2]);
  StoreResult(FPRClass, Op, Result, -1);
}

void OpDispatchBuilder::VINSERTOp(OpcodeArgs) {
  const auto DstSize = GetDstSize(Op);
  OrderedNode *Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  OrderedNode *Src2 = LoadSource_WithOpSize(FPRClass, Op, Op->Src[1], 16, Op->Flags);

  LOGMAN_THROW_A_FMT(Op->Src[2].IsLiteral(), "Src2 needs to be literal here");
  const auto Selector = Op->Src[2].Data.Literal.Value & 1;

  OrderedNode *Result = _VInsElement(DstSize, 16, Selector, 0, Src1, Src2);

  StoreResult(FPRClass, Op, Result, -1);
}

void OpDispatchBuilder::VPERM2Op(OpcodeArgs) {
  const auto DstSize = GetDstSize(Op);
  OrderedNode *Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  OrderedNode *Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);

  LOGMAN_THROW_A_FMT(Op->Src[2].IsLiteral(), "Src2 needs to be literal here");
  const auto Selector = Op->Src[2].Data.Literal.Value;

  OrderedNode *Result = LoadAndCacheNamedVectorConstant(DstSize, FEXCore::IR::NamedVectorConstant::NAMED_VECTOR_ZERO);

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

void OpDispatchBuilder::VPERMDOp(OpcodeArgs) {
  const auto DstSize = GetDstSize(Op);

  OrderedNode *Indices = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);

  // Get rid of any junk unrelated to the relevant selector index bits (bits [2:0])
  OrderedNode *IndexMask = _VectorImm(DstSize, 4, 0b111);
  OrderedNode *SanitizedIndices = _VAnd(DstSize, 1, Indices, IndexMask);

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

  OrderedNode *IndexTrn1 = _VTrn(DstSize, 1, SanitizedIndices, SanitizedIndices);
  OrderedNode *IndexTrn2 = _VTrn(DstSize, 2, IndexTrn1, IndexTrn1);

  // Now that we have the indices set up, now we need to multiply each
  // element by 4 to convert the elements into byte indices rather than
  // 32-bit word indices.
  //
  // e.g. We turn our vector into:
  // ╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗
  // ║ 16 ║║ 16 ║║ 16 ║║ 16 ║║ 4  ║║ 4  ║║ 4  ║║ 4  ║║ 8  ║║ 8  ║║ 8  ║║ 8  ║║ 24 ║║ 24 ║║ 24 ║║ 24 ║║ 28 ║║ 28 ║║ 28 ║║ 28 ║║ 0  ║║ 0  ║║ 0  ║║ 0  ║║ 12 ║║ 12 ║║ 12 ║║ 12 ║║ 20 ║║ 20 ║║ 20 ║║ 20 ║
  // ╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝
  //
  OrderedNode *ShiftedIndices = _VShlI(DstSize, 1, IndexTrn2, 2);

  // Now we need to add a byte vector containing [3, 2, 1, 0] repeating for the
  // entire length of it, to the index register, so that we specify the bytes
  // that make up the entire word in the source register.
  //
  // e.g. Our vector finally looks like so:
  //
  // ╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗╔════╗
  // ║ 19 ║║ 18 ║║ 17 ║║ 16 ║║ 7  ║║ 6  ║║ 5  ║║ 4  ║║ 11 ║║ 10 ║║ 9  ║║ 8  ║║ 27 ║║ 26 ║║ 25 ║║ 24 ║║ 31 ║║ 30 ║║ 29 ║║ 28 ║║ 3  ║║ 2  ║║ 1  ║║ 0  ║║ 15 ║║ 14 ║║ 13 ║║ 12 ║║ 23 ║║ 22 ║║ 21 ║║ 20 ║
  // ╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝╚════╝
  //
  // Which finally lets us permute the source vector and be done with everything.
  OrderedNode *AddConst = _Constant(0x03020100);
  OrderedNode *AddVector = _VDupFromGPR(DstSize, 4, AddConst);
  OrderedNode *FinalIndices = _VAdd(DstSize, 1, ShiftedIndices, AddVector);

  // Now lets finally shuffle this bad boy around.
  OrderedNode *Result = _VTBL1(DstSize, Src, FinalIndices);

  StoreResult(FPRClass, Op, Result, -1);
}

void OpDispatchBuilder::VPERMQOp(OpcodeArgs) {
  const auto DstSize = GetDstSize(Op);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);

  LOGMAN_THROW_A_FMT(Op->Src[1].IsLiteral(), "Src1 needs to be literal here");
  const auto Selector = Op->Src[1].Data.Literal.Value;

  OrderedNode *Result{};

  // If we're just broadcasting one element in particular across the vector
  // then this can be done fairly simply without any individual inserts.
  if (Selector == 0x00 || Selector == 0x55 || Selector == 0xAA || Selector == 0xFF) {
    const auto Index = Selector & 0b11;
    Result = _VDupElement(DstSize, 8, Src, Index);
  } else {
    Result = LoadAndCacheNamedVectorConstant(DstSize, FEXCore::IR::NamedVectorConstant::NAMED_VECTOR_ZERO);
    for (size_t i = 0; i < DstSize / 8; i++) {
      const auto SrcIndex = (Selector >> (i * 2)) & 0b11;
      Result = _VInsElement(DstSize, 8, i, SrcIndex, Result, Src);
    }
  }
  StoreResult(FPRClass, Op, Result, -1);
}

static OrderedNode* VBLENDOpImpl(IREmitter& IR, uint32_t VecSize, uint32_t ElementSize,
                                 OrderedNode *Src1, OrderedNode *Src2, OrderedNode *ZeroRegister, uint64_t Selector) {
  const std::array Sources{Src1, Src2};

  OrderedNode *Result = ZeroRegister;
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

  OrderedNode *Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  OrderedNode *Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);

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

  const auto ZeroRegister = LoadAndCacheNamedVectorConstant(DstSize, FEXCore::IR::NamedVectorConstant::NAMED_VECTOR_ZERO);
  OrderedNode *Result = VBLENDOpImpl(*this, DstSize, 8, Src1, Src2, ZeroRegister, Selector);
  StoreResult(FPRClass, Op, Result, -1);
}

void OpDispatchBuilder::VPBLENDDOp(OpcodeArgs) {
  const auto DstSize = GetDstSize(Op);
  const auto Is256Bit = DstSize == Core::CPUState::XMM_AVX_REG_SIZE;

  OrderedNode *Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  OrderedNode *Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);

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
    OrderedNode* Result = Is256Bit ? Src1 : _VMov(16, Src1);
    StoreResult(FPRClass, Op, Result, -1);
    return;
  }
  if (Selector == 0xFF && Is256Bit) {
    OrderedNode* Result = Is256Bit ? Src2 : _VMov(16, Src2);
    StoreResult(FPRClass, Op, Result, -1);
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

  const auto ZeroRegister = LoadAndCacheNamedVectorConstant(DstSize, FEXCore::IR::NamedVectorConstant::NAMED_VECTOR_ZERO);
  OrderedNode *Result = VBLENDOpImpl(*this, DstSize, 4, Src1, Src2, ZeroRegister, Selector);
  if (!Is256Bit) {
    Result = _VMov(16, Result);
  }
  StoreResult(FPRClass, Op, Result, -1);
}

void OpDispatchBuilder::VPBLENDWOp(OpcodeArgs) {
  const auto DstSize = GetDstSize(Op);
  const auto Is128Bit = DstSize == Core::CPUState::XMM_SSE_REG_SIZE;

  OrderedNode *Src1 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  OrderedNode *Src2 = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);

  LOGMAN_THROW_A_FMT(Op->Src[2].IsLiteral(), "Src[2] needs to be literal here");
  const auto Selector = Op->Src[2].Data.Literal.Value;

  if (Selector == 0) {
    OrderedNode *Result = Is128Bit ? _VMov(16, Src1) : Src1;
    StoreResult(FPRClass, Op, Result, -1);
    return;
  }
  if (Selector == 0xFF) {
    OrderedNode *Result = Is128Bit ? _VMov(16, Src2) : Src2;
    StoreResult(FPRClass, Op, Result, -1);
    return;
  }

  // 256-bit VPBLENDW acts as if the 8-bit selector values were also applied
  // to the upper bits, so we can just replicate the bits by forming a 16-bit
  // imm for the helper function to use.
  const auto NewSelector = Selector << 8 | Selector;

  const auto ZeroRegister = LoadAndCacheNamedVectorConstant(DstSize, FEXCore::IR::NamedVectorConstant::NAMED_VECTOR_ZERO);
  OrderedNode *Result = VBLENDOpImpl(*this, DstSize, 2, Src1, Src2, ZeroRegister, NewSelector);
  if (Is128Bit) {
    Result = _VMov(16, Result);
  }
  StoreResult(FPRClass, Op, Result, -1);
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
      OrderedNode* ZeroVector = _LoadNamedVectorConstant(DstSize, FEXCore::IR::NamedVectorConstant::NAMED_VECTOR_ZERO);
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

  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  OrderedNode *Result = LoadAndCacheNamedVectorConstant(DstSize, FEXCore::IR::NamedVectorConstant::NAMED_VECTOR_ZERO);

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

template <size_t ElementSize>
void OpDispatchBuilder::VPERMILRegOp(OpcodeArgs) {
  // NOTE: See implementation of VPERMD for the gist of what we do to make this work.
  //
  //       The only difference here is that we need to add 16 to the upper lane
  //       before doing the final addition to build up the indices for TBL.

  const auto DstSize = GetDstSize(Op);
  const auto Is256Bit = DstSize == Core::CPUState::XMM_AVX_REG_SIZE;
  constexpr auto IsPD = ElementSize == 8;

  const auto SanitizeIndices = [&](OrderedNode *Indices) {
    const auto ShiftAmount = 0b11 >> static_cast<uint32_t>(IsPD);
    OrderedNode *IndexMask = _VectorImm(DstSize, ElementSize, ShiftAmount);
    return _VAnd(DstSize, 1, Indices, IndexMask);
  };

  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  OrderedNode *Indices = LoadSource(FPRClass, Op, Op->Src[1], Op->Flags);
  if constexpr (IsPD) {
    // VPERMILPD stores the selector in the second bit, rather than the
    // first bit of each element in the index vector. So move it over by one.
    Indices = _VUShrI(DstSize, ElementSize, Indices, 1);
  }

  OrderedNode *SanitizedIndices = SanitizeIndices(Indices);
  OrderedNode *IndexTrn1 = _VTrn(DstSize, 1, SanitizedIndices, SanitizedIndices);
  OrderedNode *IndexTrn2 = _VTrn(DstSize, 2, IndexTrn1, IndexTrn1);
  OrderedNode *IndexTrn3 = IndexTrn2;
  if constexpr (IsPD) {
    IndexTrn3 = _VTrn(DstSize, 4, IndexTrn2, IndexTrn2);
  }

  constexpr auto IndexShift = IsPD ? 3 : 2;
  OrderedNode *ShiftedIndices = _VShlI(DstSize, 1, IndexTrn3, IndexShift);

  constexpr uint64_t VConstant = IsPD ? 0x0706050403020100 : 0x03020100;
  OrderedNode *VectorConst = _VDupFromGPR(DstSize, ElementSize, _Constant(VConstant));
  OrderedNode *FinalIndices{};

  if (Is256Bit) {
    const auto ZeroRegister = LoadAndCacheNamedVectorConstant(DstSize, FEXCore::IR::NamedVectorConstant::NAMED_VECTOR_ZERO);
    OrderedNode *Vector16 = _VInsElement(DstSize, 16, 1, 0, ZeroRegister, _VectorImm(DstSize, 1, 16));
    OrderedNode *IndexOffsets = _VAdd(DstSize, 1, VectorConst, Vector16);

    FinalIndices = _VAdd(DstSize, 1, IndexOffsets, ShiftedIndices);
  } else {
    FinalIndices = _VAdd(DstSize, 1, VectorConst, ShiftedIndices);
  }

  OrderedNode *Result = _VTBL1(DstSize, Src, FinalIndices);

  StoreResult(FPRClass, Op, Result, -1);
}

template
void OpDispatchBuilder::VPERMILRegOp<4>(OpcodeArgs);
template
void OpDispatchBuilder::VPERMILRegOp<8>(OpcodeArgs);

void OpDispatchBuilder::PCMPXSTRXOpImpl(OpcodeArgs, bool IsExplicit, bool IsMask) {
  LOGMAN_THROW_A_FMT(Op->Src[1].IsLiteral(), "Src[1] needs to be a literal");
  const uint16_t Control = Op->Src[1].Data.Literal.Value;

  // SSE4.2 string instructions modify flags, so invalidate
  // any previously deferred flags.
  InvalidateDeferredFlags();

  // NOTE: Unlike most other SSE/AVX instructions, the SSE4.2 string and text
  //       instructions do *not* require memory operands to be aligned on a 16 byte
  //       boundary (see "Other Exceptions" descriptions for the relevant
  //       instructions in the Intel Software Development Manual).
  //
  //       So, we specify Src2 as having an alignment of 1 to indicate this.
  OrderedNode *Src1 = LoadSource_WithOpSize(FPRClass, Op, Op->Dest, 16, Op->Flags);
  OrderedNode *Src2 = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], 16, Op->Flags,
                                            {.Align = 1});

  OrderedNode *IntermediateResult{};
  if (IsExplicit) {
    // Will be 4 in the absence of a REX.W bit and 8 in the presence of a REX.W bit.
    //
    // While the control bit immediate for the instruction itself is only ever 8 bits
    // in size, we use it as a 16-bit value so that we can use the 8th bit to signify
    // whether or not RAX and RDX should be interpreted as a 64-bit value.
    const auto SrcSize = GetSrcSize(Op);
    const auto Is64Bit = SrcSize == 8;
    const auto NewControl = uint16_t(Control | (uint16_t(Is64Bit) << 8));

    OrderedNode *SrcRAX = LoadGPRRegister(X86State::REG_RAX);
    OrderedNode *SrcRDX = LoadGPRRegister(X86State::REG_RDX);

    IntermediateResult = _VPCMPESTRX(Src1, Src2, SrcRAX, SrcRDX, NewControl);
  } else {
    IntermediateResult = _VPCMPISTRX(Src1, Src2, Control);
  }

  OrderedNode *ZeroConst = _Constant(0);

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
      const auto ElementSize = 1U  << (Control & 1);
      const auto NumElements = 16U >> (Control & 1);

      OrderedNode *Result = LoadAndCacheNamedVectorConstant(Core::CPUState::XMM_SSE_REG_SIZE, FEXCore::IR::NamedVectorConstant::NAMED_VECTOR_ZERO);
      for (uint32_t i = 0; i < NumElements; i++) {
        OrderedNode *SignBit = _Sbfe(OpSize::i64Bit, 1, i, IntermediateResult);
        Result = _VInsGPR(Core::CPUState::XMM_SSE_REG_SIZE, ElementSize, i, Result, SignBit);
      }
      StoreXMMRegister(0, Result);
    } else {
      // We insert the intermediate result as-is.
      StoreXMMRegister(0, _VCastFromGPR(16, 2, IntermediateResult));
    }
  } else {
    // For the indexed variant of the instructions, if control[6] is set, then we
    // store the index of the most significant bit into ECX. If it's not set,
    // then we store the least significant bit.
    const auto UseMSBIndex = (Control & 0b0100'0000) != 0;

    OrderedNode *ResultNoFlags = _Bfe(OpSize::i32Bit, 16, 0, IntermediateResult);

    OrderedNode *IfZero = _Constant(16 >> (Control & 1));
    OrderedNode *IfNotZero = UseMSBIndex ? _FindMSB(IR::OpSize::i32Bit, ResultNoFlags)
                                         : _FindLSB(IR::OpSize::i32Bit, ResultNoFlags);

    OrderedNode *Result = _Select(IR::COND_EQ, ResultNoFlags, ZeroConst,
                                  IfZero, IfNotZero);

    const uint8_t GPRSize = CTX->GetGPRSize();
    if (GPRSize == 8) {
      // If being stored to an 8-byte register, zero extend the 4-byte result.
      Result = _Bfe(OpSize::i64Bit, 32, 0, Result);
    }
    StoreGPRRegister(X86State::REG_RCX, Result);
  }

  // Set all of the necessary flags.
  // We use the top 16-bits of the result to store the flags
  // in the form:
  //
  // Bit:  19   18   17   16
  //      [OF | CF | SF | ZF]
  //
  const auto GetFlagBit = [this, IntermediateResult](int BitIndex) {
    return _Bfe(OpSize::i32Bit, 1, BitIndex, IntermediateResult);
  };

  SetRFLAG<X86State::RFLAG_ZF_RAW_LOC>(GetFlagBit(16));
  SetRFLAG<X86State::RFLAG_SF_RAW_LOC>(GetFlagBit(17));
  SetRFLAG<X86State::RFLAG_CF_RAW_LOC>(GetFlagBit(18));
  SetRFLAG<X86State::RFLAG_OF_RAW_LOC>(GetFlagBit(19));

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

} // namespace FEXCore::IR
