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

void OpDispatchBuilder::MOVAPSOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  StoreResult(FPRClass, Op, Src, -1);
}

void OpDispatchBuilder::VMOVAPS_VMOVAPD_Op(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  const auto Is128BitDest = GetDstSize(Op) == Core::CPUState::XMM_SSE_REG_SIZE;

  if (Op->Dest.IsGPR() && Is128BitDest) {
    // Perform 32 byte store to clear the upper lane.
    StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Src, 32, -1);
  } else {
    StoreResult(FPRClass, Op, Src, -1);
  }
}

void OpDispatchBuilder::VMOVUPS_VMOVUPD_Op(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, 1);
  const auto Is128BitDest = GetDstSize(Op) == Core::CPUState::XMM_SSE_REG_SIZE;

  if (Op->Dest.IsGPR() && Is128BitDest) {
    // Perform 32 byte store to clear the upper lane.
    StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Src, 32, 1);
  } else {
    StoreResult(FPRClass, Op, Src, 1);
  }
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
    StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Result, 32, -1);
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
      OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, 8, 16);
      auto Result = _VInsElement(16, 8, 0, 0, Dest, Src);
      StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Result, 8, 16);
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
    StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Result, 32, -1);
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
  }

  StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Result, 32, -1);
}

void OpDispatchBuilder::MOVSLDUPOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, 8);
  OrderedNode *Result = _VInsElement(16, 4, 3, 2, Src, Src);
  Result = _VInsElement(16, 4, 2, 2, Result, Src);
  Result = _VInsElement(16, 4, 1, 0, Result, Src);
  Result = _VInsElement(16, 4, 0, 0, Result, Src);
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

template<size_t ElementSize>
void OpDispatchBuilder::PADDQOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);

  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);

  auto ALUOp = _VAdd(Size, ElementSize, Dest, Src);
  StoreResult(FPRClass, Op, ALUOp, -1);
}

template
void OpDispatchBuilder::PADDQOp<1>(OpcodeArgs);
template
void OpDispatchBuilder::PADDQOp<2>(OpcodeArgs);
template
void OpDispatchBuilder::PADDQOp<4>(OpcodeArgs);
template
void OpDispatchBuilder::PADDQOp<8>(OpcodeArgs);

template<size_t ElementSize>
void OpDispatchBuilder::PSUBQOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);

  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);

  auto ALUOp = _VSub(Size, ElementSize, Dest, Src);
  StoreResult(FPRClass, Op, ALUOp, -1);
}

template
void OpDispatchBuilder::PSUBQOp<1>(OpcodeArgs);
template
void OpDispatchBuilder::PSUBQOp<2>(OpcodeArgs);
template
void OpDispatchBuilder::PSUBQOp<4>(OpcodeArgs);
template
void OpDispatchBuilder::PSUBQOp<8>(OpcodeArgs);

template<FEXCore::IR::IROps IROp, size_t ElementSize>
void OpDispatchBuilder::VectorALUOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);

  auto ALUOp = _VAdd(Size, ElementSize, Dest, Src);
  // Overwrite our IR's op type
  ALUOp.first->Header.Op = IROp;

  StoreResult(FPRClass, Op, ALUOp, -1);
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
void OpDispatchBuilder::VectorALUOp<IR::OP_VFADD, 4>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUOp<IR::OP_VFADD, 8>(OpcodeArgs);
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
void OpDispatchBuilder::VectorALUOp<IR::OP_VUQADD, 1>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUOp<IR::OP_VUQADD, 2>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUOp<IR::OP_VUQSUB, 1>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUOp<IR::OP_VUQSUB, 2>(OpcodeArgs);

template<FEXCore::IR::IROps IROp, size_t ElementSize>
void OpDispatchBuilder::VectorALUROp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);

  auto ALUOp = _VAdd(Size, ElementSize, Src, Dest);
  // Overwrite our IR's op type
  ALUOp.first->Header.Op = IROp;

  StoreResult(FPRClass, Op, ALUOp, -1);
}

template
void OpDispatchBuilder::VectorALUROp<IR::OP_VFSUB, 4>(OpcodeArgs);
template
void OpDispatchBuilder::VectorALUROp<IR::OP_VFSUB, 8>(OpcodeArgs);

template<FEXCore::IR::IROps IROp, size_t ElementSize>
void OpDispatchBuilder::VectorScalarALUOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  // If OpSize == ElementSize then it only does the lower scalar op
  auto ALUOp = _VAdd(ElementSize, ElementSize, Dest, Src);
  // Overwrite our IR's op type
  ALUOp.first->Header.Op = IROp;

  OrderedNode* Result = ALUOp;

  if (Size != ElementSize) {
    // Insert the lower bits
    Result = _VInsElement(Size, ElementSize, 0, 0, Dest, Result);
  }

  StoreResult(FPRClass, Op, Result, -1);
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

template<FEXCore::IR::IROps IROp, size_t ElementSize, bool Scalar>
void OpDispatchBuilder::VectorUnaryOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);
  if constexpr (Scalar) {
    Size = ElementSize;
  }
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);

  auto ALUOp = _VFSqrt(Size, ElementSize, Src);
  // Overwrite our IR's op type
  ALUOp.first->Header.Op = IROp;

  if constexpr (Scalar) {
    // Insert the lower bits
    auto Result = _VInsElement(GetSrcSize(Op), ElementSize, 0, 0, Dest, ALUOp);
    StoreResult(FPRClass, Op, Result, -1);
  }
  else {
    StoreResult(FPRClass, Op, ALUOp, -1);
  }
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

template<FEXCore::IR::IROps IROp, size_t ElementSize>
void OpDispatchBuilder::VectorUnaryDuplicateOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);

  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  auto ALUOp = _VFSqrt(ElementSize, ElementSize, Src);
  // Overwrite our IR's op type
  ALUOp.first->Header.Op = IROp;

  // Duplicate the lower bits
  auto Result = _VDupElement(Size, ElementSize, ALUOp, 0);
  StoreResult(FPRClass, Op, Result, -1);
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

    const auto fprLowOffset = CTX->HostFeatures.SupportsAVX ? offsetof(Core::CPUState, xmm.avx.data[gprIndex][0])
                                                            : offsetof(Core::CPUState, xmm.sse.data[gprIndex][0]);
    const auto fprHighOffset = CTX->HostFeatures.SupportsAVX ? offsetof(Core::CPUState, xmm.avx.data[gprIndex][1])
                                                             : offsetof(Core::CPUState, xmm.sse.data[gprIndex][1]);

    _StoreContext(8, FPRClass, Src, fprLowOffset);
    auto Const = _Constant(0);
    _StoreContext(8, GPRClass, Const, fprHighOffset);
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

  //TODO: We could remove this VCastFromGOR + VInsGPR pair if we had a VDUPFromGPR instruction that maps directly to AArch64.
  auto M = _Constant(0x80'40'20'10'08'04'02'01ULL);
  OrderedNode *VMask = _VCastFromGPR(16, 8, M);

  VMask = _VInsGPR(16, 8, 1, VMask, M);

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

void OpDispatchBuilder::PSHUFBOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  // PSHUFB doesn't 100% match VTBL behaviour
  // VTBL will set the element zero if the index is greater than the number of elements
  // In the array
  // Bit 7 is the only bit that is supposed to set elements to zero with PSHUFB
  // Mask the selection bits and top bit correctly
  // Bits [6:4] is reserved for 128bit
  // Bits [6:3] is reserved for 64bit
  if (Size == 8) {
    auto MaskVector = _VectorImm(Size, 1, 0b1000'0111);
    Src = _VAnd(Size, Size, Src, MaskVector);
  }
  else {
    auto MaskVector = _VectorImm(Size, 1, 0b1000'1111);
    Src = _VAnd(Size, Size, Src, MaskVector);
  }
  auto Res = _VTBL1(Size, Dest, Src);
  StoreResult(FPRClass, Op, Res, -1);
}

template<size_t ElementSize, bool HalfSize, bool Low>
void OpDispatchBuilder::PSHUFDOp(OpcodeArgs) {
  LOGMAN_THROW_AA_FMT(ElementSize != 0, "What. No element size?");
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

template<size_t ElementSize>
void OpDispatchBuilder::SHUFOp(OpcodeArgs) {
  LOGMAN_THROW_AA_FMT(ElementSize != 0, "What. No element size?");
  const auto Size = GetSrcSize(Op);
  OrderedNode *Src1 = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src2 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  uint8_t Shuffle = Op->Src[1].Data.Literal.Value;

  uint8_t NumElements = Size / ElementSize;

  auto Dest = Src1;
  std::array<OrderedNode*, 4> Srcs = {
  };

  for (int i = 0; i < (NumElements >> 1); ++i) {
    Srcs[i] = Src1;
  }

  for (int i = (NumElements >> 1); i < NumElements; ++i) {
    Srcs[i] = Src2;
  }

  // 32bit:
  // [31:0]   = Src1[Selection]
  // [63:32]  = Src1[Selection]
  // [95:64]  = Src2[Selection]
  // [127:96] = Src2[Selection]
  // 64bit:
  // [63:0]   = Src1[Selection]
  // [127:64] = Src2[Selection]
  uint8_t SelectionMask = NumElements - 1;
  uint8_t ShiftAmount = std::popcount(SelectionMask);
  for (uint8_t Element = 0; Element < NumElements; ++Element) {
    Dest = _VInsElement(Size, ElementSize, Element, Shuffle & SelectionMask, Dest, Srcs[Element]);
    Shuffle >>= ShiftAmount;
  }

  StoreResult(FPRClass, Op, Dest, -1);
}

template
void OpDispatchBuilder::SHUFOp<4>(OpcodeArgs);
template
void OpDispatchBuilder::SHUFOp<8>(OpcodeArgs);

void OpDispatchBuilder::ANDNOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);
  OrderedNode *Src1 = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src2 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  // Dest = ~Src1 & Src2

  Src1 = _VNot(Size, Size, Src1);
  auto Dest = _VAnd(Size, Size, Src1, Src2);

  StoreResult(FPRClass, Op, Dest, -1);
}

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

void OpDispatchBuilder::InsertPSOp(OpcodeArgs) {
  LOGMAN_THROW_A_FMT(Op->Src[1].IsLiteral(), "Src1 needs to be literal here");
  uint8_t Imm = Op->Src[1].Data.Literal.Value;
  uint8_t CountS = (Imm >> 6);
  uint8_t CountD = (Imm >> 4) & 0b11;
  uint8_t ZMask = Imm & 0xF;

  OrderedNode *Dest{};
  if (ZMask != 0xF) {
    // Only need to load destination if it isn't a full zero
    Dest = LoadSource_WithOpSize(FPRClass, Op, Op->Dest, GetDstSize(Op), Op->Flags, -1);
  }

  if (!(ZMask & (1 << CountD))) {
    // In the case that ZMask overwrites the destination element, then don't even insert
    OrderedNode *Src{};
    if (Op->Src[0].IsGPR()) {
      Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
    }
    else {
      // If loading from memory then CountS is forced to zero
      CountS = 0;
      Src = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], 4, Op->Flags, -1);
    }

    Dest = _VInsElement(GetDstSize(Op), 4, CountD, CountS, Dest, Src);
  }

  // ZMask happens after insert
  if (ZMask == 0xF) {
    Dest = _VectorImm(16, 4, 0);
  }
  else if (ZMask) {
    auto Zero = _VectorImm(16, 4, 0);
    for (size_t i = 0; i < 4; ++i) {
      if (ZMask & (1 << i)) {
        Dest = _VInsElement(GetDstSize(Op), 4, i, 0, Dest, Zero);
      }
    }
  }

  StoreResult(FPRClass, Op, Dest, -1);
}

template<size_t ElementSize>
void OpDispatchBuilder::PExtrOp(OpcodeArgs) {
  const auto Size = GetSrcSize(Op);

  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  LOGMAN_THROW_A_FMT(Op->Src[1].IsLiteral(), "Src1 needs to be literal here");
  uint64_t Index = Op->Src[1].Data.Literal.Value;

  const uint8_t NumElements = Size / ElementSize;
  Index &= NumElements - 1;

  OrderedNode *Result = _VExtractToGPR(16, ElementSize, Src, Index);

  if (Op->Dest.IsGPR()) {
    const uint8_t GPRSize = CTX->GetGPRSize();

    // If we are storing to a GPR then we zero extend it
    if constexpr (ElementSize < 4) {
      Result = _Bfe(GPRSize, ElementSize * 8, 0, Result);
    }
    StoreResult_WithOpSize(GPRClass, Op, Op->Dest, Result, GPRSize, -1);
  }
  else {
    // If we are storing to memory then we store the size of the element extracted
    StoreResult_WithOpSize(GPRClass, Op, Op->Dest, Result, ElementSize, -1);
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

template<size_t ElementSize>
void OpDispatchBuilder::PSIGN(OpcodeArgs) {
  auto Size = GetSrcSize(Op);

  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);

  auto ZeroVec = _VectorZero(Size);
  auto NegVec = _VNeg(Size, ElementSize, Dest);

  OrderedNode *CmpLT = _VCMPLTZ(Size, ElementSize, Src);
  OrderedNode *CmpEQ = _VCMPEQZ(Size, ElementSize, Src);
  OrderedNode *CmpGT = _VCMPGTZ(Size, ElementSize, Src);

  // Negative elements return -dest
  CmpLT = _VAnd(Size, ElementSize, CmpLT, NegVec);

  // Zero elements return 0
  CmpEQ = _VAnd(Size, ElementSize, CmpEQ, ZeroVec);

  // Positive elements return dest
  CmpGT = _VAnd(Size, ElementSize, CmpGT, Dest);

  // Or our results
  OrderedNode *Res = _VOr(Size, ElementSize, CmpGT, _VOr(Size, ElementSize, CmpLT, CmpEQ));
  StoreResult(FPRClass, Op, Res, -1);
}

template
void OpDispatchBuilder::PSIGN<1>(OpcodeArgs);
template
void OpDispatchBuilder::PSIGN<2>(OpcodeArgs);
template
void OpDispatchBuilder::PSIGN<4>(OpcodeArgs);

template<size_t ElementSize>
void OpDispatchBuilder::PSRLDOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);

  auto Size = GetSrcSize(Op);

  OrderedNode *Result{};

  // Incoming element size for the shift source is always 8
  auto MaxShift = _VectorImm(8, 8, ElementSize * 8);
  Src = _VUMin(8, 8, MaxShift, Src);
  Result = _VUShrS(Size, ElementSize, Dest, Src);

  StoreResult(FPRClass, Op, Result, -1);
}

template
void OpDispatchBuilder::PSRLDOp<2>(OpcodeArgs);
template
void OpDispatchBuilder::PSRLDOp<4>(OpcodeArgs);
template
void OpDispatchBuilder::PSRLDOp<8>(OpcodeArgs);

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

template<size_t ElementSize>
void OpDispatchBuilder::PSLLI(OpcodeArgs) {
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);

  LOGMAN_THROW_A_FMT(Op->Src[1].IsLiteral(), "Src1 needs to be literal here");
  uint64_t ShiftConstant = Op->Src[1].Data.Literal.Value;

  auto Size = GetSrcSize(Op);

  auto Shift = _VShlI(Size, ElementSize, Dest, ShiftConstant);
  StoreResult(FPRClass, Op, Shift, -1);
}

template
void OpDispatchBuilder::PSLLI<2>(OpcodeArgs);
template
void OpDispatchBuilder::PSLLI<4>(OpcodeArgs);
template
void OpDispatchBuilder::PSLLI<8>(OpcodeArgs);

template<size_t ElementSize>
void OpDispatchBuilder::PSLL(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);

  auto Size = GetDstSize(Op);

  OrderedNode *Result{};

  // Incoming element size for the shift source is always 8
  auto MaxShift = _VectorImm(8, 8, ElementSize * 8);
  Src = _VUMin(8, 8, MaxShift, Src);
  Result = _VUShlS(Size, ElementSize, Dest, Src);

  StoreResult(FPRClass, Op, Result, -1);
}

template
void OpDispatchBuilder::PSLL<2>(OpcodeArgs);
template
void OpDispatchBuilder::PSLL<4>(OpcodeArgs);
template
void OpDispatchBuilder::PSLL<8>(OpcodeArgs);

template<size_t ElementSize>
void OpDispatchBuilder::PSRAOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);

  auto Size = GetDstSize(Op);

  OrderedNode *Result{};

  // Incoming element size for the shift source is always 8
  auto MaxShift = _VectorImm(8, 8, ElementSize * 8);
  Src = _VUMin(8, 8, MaxShift, Src);
  Result = _VSShrS(Size, ElementSize, Dest, Src);

  StoreResult(FPRClass, Op, Result, -1);
}

template
void OpDispatchBuilder::PSRAOp<2>(OpcodeArgs);
template
void OpDispatchBuilder::PSRAOp<4>(OpcodeArgs);

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

template<size_t ElementSize>
void OpDispatchBuilder::PAVGOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);

  auto Result = _VURAvg(Size, ElementSize, Dest, Src);
  StoreResult(FPRClass, Op, Result, -1);
}

template
void OpDispatchBuilder::PAVGOp<1>(OpcodeArgs);
template
void OpDispatchBuilder::PAVGOp<2>(OpcodeArgs);

void OpDispatchBuilder::MOVDDUPOp(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Res = _VDupElement(16, GetSrcSize(Op), Src, 0);

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

template<size_t SrcElementSize, bool Widen>
void OpDispatchBuilder::Vector_CVT_Int_To_Float(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  size_t ElementSize = SrcElementSize;
  size_t Size = GetDstSize(Op);
  if constexpr (Widen) {
    Src = _VSXTL(Size, ElementSize, Src);
    ElementSize <<= 1;
  }

  Src = _Vector_SToF(Size, ElementSize, Src);

  StoreResult(FPRClass, Op, Src, -1);
}

template
void OpDispatchBuilder::Vector_CVT_Int_To_Float<4, true>(OpcodeArgs);
template
void OpDispatchBuilder::Vector_CVT_Int_To_Float<4, false>(OpcodeArgs);

template<size_t SrcElementSize, bool Narrow, bool HostRoundingMode>
void OpDispatchBuilder::Vector_CVT_Float_To_Int(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  size_t ElementSize = SrcElementSize;
  size_t Size = GetDstSize(Op);

  if constexpr (Narrow) {
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
void OpDispatchBuilder::Vector_CVT_Float_To_Int<4, false, false>(OpcodeArgs);
template
void OpDispatchBuilder::Vector_CVT_Float_To_Int<4, false, true>(OpcodeArgs);
template
void OpDispatchBuilder::Vector_CVT_Float_To_Int<4, true, false>(OpcodeArgs);

template
void OpDispatchBuilder::Vector_CVT_Float_To_Int<8, true, true>(OpcodeArgs);
template
void OpDispatchBuilder::Vector_CVT_Float_To_Int<8, true, false>(OpcodeArgs);

template<size_t DstElementSize, size_t SrcElementSize>
void OpDispatchBuilder::Scalar_CVT_Float_To_Float(OpcodeArgs) {
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  Src = _Float_FToF(DstElementSize, SrcElementSize, Src);
  Src = _VInsElement(16, DstElementSize, 0, 0, Dest, Src);

  StoreResult(FPRClass, Op, Src, -1);
}

template
void OpDispatchBuilder::Scalar_CVT_Float_To_Float<4, 8>(OpcodeArgs);
template
void OpDispatchBuilder::Scalar_CVT_Float_To_Float<8, 4>(OpcodeArgs);

template<size_t DstElementSize, size_t SrcElementSize>
void OpDispatchBuilder::Vector_CVT_Float_To_Float(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  size_t Size = GetDstSize(Op);

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
  const auto GPRSize = CTX->GetGPRSize();
  const auto Size = uint32_t{GetSrcSize(Op)} * 8;

  OrderedNode *Src = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1);

  OrderedNode *MemDest = _LoadContext(GPRSize, GPRClass, GPROffset(X86State::REG_RDI));

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

template<size_t ElementSize, bool Scalar>
void OpDispatchBuilder::VFCMPOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  OrderedNode *Dest = LoadSource_WithOpSize(FPRClass, Op, Op->Dest, GetDstSize(Op), Op->Flags, -1);
  uint8_t CompType = Op->Src[1].Data.Literal.Value;

  OrderedNode *Result{};
  // This maps 1:1 to an AArch64 NEON Op
  //auto ALUOp = _VCMPGT(Size, ElementSize, Dest, Src);
  switch (CompType) {
    case 0x00: case 0x08: case 0x10: case 0x18: // EQ
      Result = _VFCMPEQ(Size, ElementSize, Dest, Src);
    break;
    case 0x01: case 0x09: case 0x11: case 0x19: // LT, GT(Swapped operand)
      Result = _VFCMPLT(Size, ElementSize, Dest, Src);
    break;
    case 0x02: case 0x0A: case 0x12: case 0x1A: // LE, GE(Swapped operand)
      Result = _VFCMPLE(Size, ElementSize, Dest, Src);
    break;
    case 0x03: case 0x0B: case 0x13: case 0x1B: // Unordered
      Result = _VFCMPUNO(Size, ElementSize, Dest, Src);
    break;
    case 0x04: case 0x0C: case 0x14: case 0x1C: // NEQ
      Result = _VFCMPNEQ(Size, ElementSize, Dest, Src);
    break;
    case 0x05: case 0x0D: case 0x15: case 0x1D: // NLT, NGT(Swapped operand)
      Result = _VFCMPLT(Size, ElementSize, Dest, Src);
      Result = _VNot(Size, ElementSize, Result);
    break;
    case 0x06: case 0x0E: case 0x16: case 0x1E: // NLE, NGE(Swapped operand)
      Result = _VFCMPLE(Size, ElementSize, Dest, Src);
      Result = _VNot(Size, ElementSize, Result);
    break;
    case 0x07: case 0x0F: case 0x17: case 0x1F: // Ordered
      Result = _VFCMPORD(Size, ElementSize, Dest, Src);
    break;
    default:
      LOGMAN_MSG_A_FMT("Unknown Comparison type: {}", CompType);
    break;
  }

  if constexpr (Scalar) {
    // Insert the lower bits
    Result = _VInsElement(GetDstSize(Op), ElementSize, 0, 0, Dest, Result);
  }

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
  const auto GetXMMOffset = [this](size_t i) {
    if (CTX->HostFeatures.SupportsAVX) {
      return offsetof(Core::CPUState, xmm.avx.data[i]);
    } else {
      return offsetof(Core::CPUState, xmm.sse.data[i]);
    }
  };

  for (unsigned i = 0; i < NumRegs; ++i) {
    OrderedNode *XMMReg = _LoadContext(16, FPRClass, GetXMMOffset(i));
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
  const auto GetXMMOffset = [this](size_t i) {
    if (CTX->HostFeatures.SupportsAVX) {
      return offsetof(Core::CPUState, xmm.avx.data[i]);
    } else {
      return offsetof(Core::CPUState, xmm.sse.data[i]);
    }
  };

  for (unsigned i = 0; i < NumRegs; ++i) {
    OrderedNode *MemLocation = _Add(Mem, _Constant(i * 16 + 160));
    auto XMMReg = _LoadMem(FPRClass, 16, MemLocation, 16);
    _StoreContext(16, FPRClass, XMMReg, GetXMMOffset(i));
  }
}

void OpDispatchBuilder::PAlignrOp(OpcodeArgs) {
  OrderedNode *Src1 = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src2 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
  auto Size = GetDstSize(Op);

  uint8_t Index = Op->Src[1].Data.Literal.Value;
  OrderedNode *Res{};
  if (Index >= (Size * 2)) {
    // If the immediate is greater than both vectors combined then it zeroes the vector
    Res = _VectorZero(Size);
  }
  else {
    Res = _VExtr(Size, 1, Src1, Src2, Index);
  }
  StoreResult(FPRClass, Op, Res, -1);
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
  // We only support the rounding mode being set
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

template<size_t ElementSize>
void OpDispatchBuilder::PACKUSOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);

  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  OrderedNode *Res = _VSQXTUN(Size, ElementSize, Dest);
  Res = _VSQXTUN2(Size, ElementSize, Res, Src);

  StoreResult(FPRClass, Op, Res, -1);
}

template
void OpDispatchBuilder::PACKUSOp<2>(OpcodeArgs);
template
void OpDispatchBuilder::PACKUSOp<4>(OpcodeArgs);

template<size_t ElementSize>
void OpDispatchBuilder::PACKSSOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);

  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  OrderedNode *Res = _VSQXTN(Size, ElementSize, Dest);
  Res = _VSQXTN2(Size, ElementSize, Res, Src);

  StoreResult(FPRClass, Op, Res, -1);
}

template
void OpDispatchBuilder::PACKSSOp<2>(OpcodeArgs);
template
void OpDispatchBuilder::PACKSSOp<4>(OpcodeArgs);

template<size_t ElementSize, bool Signed>
void OpDispatchBuilder::PMULLOp(OpcodeArgs) {
  static_assert(ElementSize == sizeof(uint32_t),
                "Currently only handles 32-bit -> 64-bit");

  auto Size = GetSrcSize(Op);

  OrderedNode *Src1 = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src2 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  OrderedNode *Res{};

  if (Size == 8) {
    if constexpr (Signed) {
      Res = _VSMull(16, ElementSize, Src1, Src2);
    }
    else {
      Res = _VUMull(16, ElementSize, Src1, Src2);
    }
  }
  else {
    Src1 = _VInsElement(Size, ElementSize, 1, 2, Src1, Src1);
    Src2 = _VInsElement(Size, ElementSize, 1, 2, Src2, Src2);

    if constexpr (Signed) {
      Res = _VSMull(Size, ElementSize, Src1, Src2);
    }
    else {
      Res = _VUMull(Size, ElementSize, Src1, Src2);
    }
  }
  StoreResult(FPRClass, Op, Res, -1);
}

template
void OpDispatchBuilder::PMULLOp<4, false>(OpcodeArgs);
template
void OpDispatchBuilder::PMULLOp<4, true>(OpcodeArgs);

template<bool ToXMM>
void OpDispatchBuilder::MOVQ2DQ(OpcodeArgs) {
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  // This instruction is a bit special in that if the source is MMX then it zexts to 128bit
  if constexpr (ToXMM) {
    const auto Index = Op->Dest.Data.GPR.GPR - FEXCore::X86State::REG_XMM_0;
    const auto Offset = CTX->HostFeatures.SupportsAVX ? offsetof(FEXCore::Core::CPUState, xmm.avx.data[Index][0])
                                                      : offsetof(FEXCore::Core::CPUState, xmm.sse.data[Index][0]);

    Src = _VMov(16, Src);
    _StoreContext(16, FPRClass, Src, Offset);
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

template<size_t ElementSize, bool Signed>
void OpDispatchBuilder::PADDSOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);

  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  OrderedNode *Res{};
  if constexpr (Signed) {
    Res = _VSQAdd(Size, ElementSize, Dest, Src);
  }
  else {
    Res = _VUQAdd(Size, ElementSize, Dest, Src);
  }

  StoreResult(FPRClass, Op, Res, -1);
}

template
void OpDispatchBuilder::PADDSOp<1, false>(OpcodeArgs);
template
void OpDispatchBuilder::PADDSOp<1, true>(OpcodeArgs);
template
void OpDispatchBuilder::PADDSOp<2, false>(OpcodeArgs);
template
void OpDispatchBuilder::PADDSOp<2, true>(OpcodeArgs);

template<size_t ElementSize, bool Signed>
void OpDispatchBuilder::PSUBSOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);

  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  OrderedNode *Res{};
  if constexpr (Signed) {
    Res = _VSQSub(Size, ElementSize, Dest, Src);
  }
  else {
    Res = _VUQSub(Size, ElementSize, Dest, Src);
  }

  StoreResult(FPRClass, Op, Res, -1);
}

template
void OpDispatchBuilder::PSUBSOp<1, false>(OpcodeArgs);
template
void OpDispatchBuilder::PSUBSOp<1, true>(OpcodeArgs);
template
void OpDispatchBuilder::PSUBSOp<2, false>(OpcodeArgs);
template
void OpDispatchBuilder::PSUBSOp<2, true>(OpcodeArgs);

template<size_t ElementSize>
void OpDispatchBuilder::ADDSUBPOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);

  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  OrderedNode *ResAdd{};
  OrderedNode *ResSub{};
  ResAdd = _VFAdd(Size, ElementSize, Dest, Src);
  ResSub = _VFSub(Size, ElementSize, Dest, Src);

  // We now need to swizzle results
  uint8_t NumElements = Size / ElementSize;
  // Even elements are the sub result
  // Odd elements are the add results
  for (size_t i = 0; i < NumElements; i += 2) {
    ResAdd = _VInsElement(Size, ElementSize, i, i, ResAdd, ResSub);
  }
  StoreResult(FPRClass, Op, ResAdd, -1);
}

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

template
void OpDispatchBuilder::ADDSUBPOp<4>(OpcodeArgs);
template
void OpDispatchBuilder::ADDSUBPOp<8>(OpcodeArgs);

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
  //TODO: We could remove this VCastFromGOR + VInsGPR pair if we had a VDUPFromGPR instruction that maps directly to AArch64.
  auto M = _Constant(0x0000'8000'0000'8000ULL);
  OrderedNode *VConstant = _VCastFromGPR(16, 8, M);
  VConstant = _VInsGPR(16, 8, 1, VConstant, M);

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
  LogMan::Msg::DFmt("CompType: {} Size: {}", CompType, Size);
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

void OpDispatchBuilder::PMADDWD(OpcodeArgs) {
  // This is a pretty curious operation
  // Does two MADD operations across 4 16bit signed integers and accumulates to 32bit integers in the destination
  //
  // x86 PMADDWD: xmm1, xmm2
  //              xmm1[31:0]  = (xmm1[15:0] * xmm2[15:0]) + (xmm1[31:16] * xmm2[31:16])
  //              xmm1[63:32] = (xmm1[47:32] * xmm2[47:32]) + (xmm1[63:48] * xmm2[63:48])
  //              etc.. for larger registers

  auto Size = GetSrcSize(Op);

  OrderedNode *Src1 = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src2 = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  if (Size == 8) {
    Size <<= 1;
  }

  auto Src1_L = _VSXTL(Size, 2, Src1);  // [15:0 ], [31:16], [32:47 ], [63:48  ]
  auto Src1_H = _VSXTL2(Size, 2, Src1); // [79:64], [95:80], [111:96], [127:112]

  auto Src2_L = _VSXTL(Size, 2, Src2);  // [15:0 ], [31:16], [32:47 ], [63:48  ]
  auto Src2_H = _VSXTL2(Size, 2, Src2); // [79:64], [95:80], [111:96], [127:112]

  auto Res_L = _VSMul(Size, 4, Src1_L, Src2_L); // [15:0 ], [31:16], [32:47 ], [63:48  ] : Original elements
  auto Res_H = _VSMul(Size, 4, Src1_H, Src2_H); // [79:64], [95:80], [111:96], [127:112] : Original elements

  // [15:0 ] + [31:16], [32:47 ] + [63:48  ], [79:64] + [95:80], [111:96] + [127:112]
  auto Res = _VAddP(Size, 4, Res_L, Res_H);
  StoreResult(FPRClass, Op, Res, -1);
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

template<bool Signed>
void OpDispatchBuilder::PMULHW(OpcodeArgs) {
  auto Size = GetSrcSize(Op);

  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  OrderedNode *Res{};
  if (Size == 8) {
    // Implementation is more efficient for 8byte registers
    if (Signed)
      Res = _VSMull(Size * 2, 2, Dest, Src);
    else
      Res = _VUMull(Size * 2, 2, Dest, Src);

    Res = _VUShrNI(Size * 2, 4, Res, 16);
  }
  else {
    // 128bit is less efficient
    OrderedNode *ResultLow;
    OrderedNode *ResultHigh;
    if (Signed) {
      ResultLow = _VSMull(Size, 2, Dest, Src);
      ResultHigh = _VSMull2(Size, 2, Dest, Src);
    }
    else {
      ResultLow = _VUMull(Size, 2, Dest, Src);
      ResultHigh = _VUMull2(Size, 2, Dest, Src);
    }

    // Combine the results
    Res = _VUShrNI(Size, 4, ResultLow, 16);
    Res = _VUShrNI2(Size, 4, Res, ResultHigh, 16);
  }

  StoreResult(FPRClass, Op, Res, -1);
}

template
void OpDispatchBuilder::PMULHW<false>(OpcodeArgs);
template
void OpDispatchBuilder::PMULHW<true>(OpcodeArgs);

void OpDispatchBuilder::PMULHRSW(OpcodeArgs) {
  auto Size = GetSrcSize(Op);

  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  OrderedNode *Res{};
  if (Size == 8) {
    // Implementation is more efficient for 8byte registers
    Res = _VSMull(Size * 2, 2, Dest, Src);
    Res = _VSShrI(Size * 2, 4, Res, 14);
    auto OneVector = _VectorImm(Size * 2, 4, 1);
    Res = _VAdd(Size * 2, 4, Res, OneVector);
    Res = _VUShrNI(Size * 2, 4, Res, 1);
  }
  else {
    // 128bit is less efficient
    OrderedNode *ResultLow;
    OrderedNode *ResultHigh;

    ResultLow = _VSMull(Size, 2, Dest, Src);
    ResultHigh = _VSMull2(Size, 2, Dest, Src);

    ResultLow = _VSShrI(Size, 4, ResultLow, 14);
    ResultHigh = _VSShrI(Size, 4, ResultHigh, 14);
    auto OneVector = _VectorImm(Size, 4, 1);

    ResultLow = _VAdd(Size, 4, ResultLow, OneVector);
    ResultHigh = _VAdd(Size, 4, ResultHigh, OneVector);

    // Combine the results
    Res = _VUShrNI(Size, 4, ResultLow, 1);
    Res = _VUShrNI2(Size, 4, Res, ResultHigh, 1);
  }

  StoreResult(FPRClass, Op, Res, -1);
}

template<size_t ElementSize>
void OpDispatchBuilder::HADDP(OpcodeArgs) {
  auto Size = GetSrcSize(Op);
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  OrderedNode *Res = _VFAddP(Size, ElementSize, Dest, Src);
  StoreResult(FPRClass, Op, Res, -1);
}

template
void OpDispatchBuilder::HADDP<4>(OpcodeArgs);
template
void OpDispatchBuilder::HADDP<8>(OpcodeArgs);

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

template<size_t ElementSize>
void OpDispatchBuilder::PHADD(OpcodeArgs) {
  auto Size = GetSrcSize(Op);
  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  OrderedNode *Res = _VAddP(Size, ElementSize, Dest, Src);
  StoreResult(FPRClass, Op, Res, -1);
}

template
void OpDispatchBuilder::PHADD<2>(OpcodeArgs);
template
void OpDispatchBuilder::PHADD<4>(OpcodeArgs);

template<size_t ElementSize>
void OpDispatchBuilder::PHSUB(OpcodeArgs) {
  auto Size = GetSrcSize(Op);
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

  OrderedNode *Res = _VAddP(Size, ElementSize, Swizzle_Dest, Swizzle_Src);
  StoreResult(FPRClass, Op, Res, -1);
}

template
void OpDispatchBuilder::PHSUB<2>(OpcodeArgs);
template
void OpDispatchBuilder::PHSUB<4>(OpcodeArgs);

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

template<size_t ElementSize, size_t DstElementSize, bool Signed>
void OpDispatchBuilder::ExtendVectorElements(OpcodeArgs) {
  auto Size = GetDstSize(Op);

  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  OrderedNode *Result {Src};

  for (size_t CurrentElementSize = ElementSize;
       CurrentElementSize != DstElementSize;
       CurrentElementSize <<= 1) {
    if constexpr (Signed) {
      Result = _VSXTL(Size, CurrentElementSize, Result);
    }
    else {
      Result = _VUXTL(Size, CurrentElementSize, Result);
    }
  }
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

template<size_t ElementSize, bool Scalar>
void OpDispatchBuilder::VectorRound(OpcodeArgs) {
  LOGMAN_THROW_A_FMT(Op->Src[1].IsLiteral(), "Src1 needs to be literal here");
  uint64_t Mode = Op->Src[1].Data.Literal.Value;
  uint64_t RoundControlSource = (Mode >> 2) & 1;
  uint64_t RoundControl = Mode & 0b11;

  auto Size = GetSrcSize(Op);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  if (RoundControlSource) {
    RoundControl = 0; // MXCSR
  }

  std::array<FEXCore::IR::RoundType, 5> SourceModes = {
    FEXCore::IR::Round_Nearest,
    FEXCore::IR::Round_Negative_Infinity,
    FEXCore::IR::Round_Positive_Infinity,
    FEXCore::IR::Round_Towards_Zero,
    FEXCore::IR::Round_Host,
  };

  Src = _Vector_FToI(Size, ElementSize, Src, SourceModes[(RoundControlSource << 2) | RoundControl]);

  if constexpr (Scalar) {
    // Insert the lower bits
    OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
    auto Result = _VInsElement(GetDstSize(Op), ElementSize, 0, 0, Dest, Src);
    StoreResult(FPRClass, Op, Result, -1);
  }
  else {
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
  OrderedNode *Mask = _LoadContext(16, FPRClass, offsetof(FEXCore::Core::CPUState, xmm.avx.data[0]));
  // Each element is selected by the high bit of that element size
  // Dest[ElementIdx] = Xmm0[ElementIndex][HighBit] ? Src : Dest;
  //
  // To emulate this on AArch64
  // Arithmetic shift right by the element size, then use BSL to select the registers
  Mask = _VSShrI(Size, ElementSize, Mask, (ElementSize * 8) - 1);

  auto Result = _VBSL(Mask, Src, Dest);

  StoreResult(FPRClass, Op, Result, -1);
}
template
void OpDispatchBuilder::VectorVariableBlend<1>(OpcodeArgs);
template
void OpDispatchBuilder::VectorVariableBlend<4>(OpcodeArgs);
template
void OpDispatchBuilder::VectorVariableBlend<8>(OpcodeArgs);

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

void OpDispatchBuilder::PHMINPOSUWOp(OpcodeArgs) {
  auto Size = GetSrcSize(Op);

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
  Result = _VInsGPR(16, 2, 1, Result, Pos);

  StoreResult(FPRClass, Op, Result, -1);
}

template<size_t ElementSize>
void OpDispatchBuilder::DPPOp(OpcodeArgs) {
  LOGMAN_THROW_A_FMT(Op->Src[1].IsLiteral(), "Src1 needs to be literal here");
  uint8_t Mask = Op->Src[1].Data.Literal.Value;
  uint8_t SrcMask = Mask >> 4;
  uint8_t DstMask = Mask & 0xF;

  OrderedNode *ZeroVec = _VectorZero(16);

  OrderedNode *Dest = LoadSource(FPRClass, Op, Op->Dest, Op->Flags, -1);
  OrderedNode *Src = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);

  // First step is to do an FMUL
  OrderedNode *Temp = _VFMul(16, ElementSize, Dest, Src);

  // Now we zero out elements based on src mask
  for (size_t i = 0; i < (16 / ElementSize); ++i) {
    if ((SrcMask & (1 << i)) == 0) {
      Temp = _VInsElement(16, ElementSize, i, 0, Temp, ZeroVec);
    }
  }

  // Now we need to do a horizontal add of the elements
  // We only have pairwise float add so this needs to be done in steps
  Temp = _VFAddP(16, ElementSize, Temp, ZeroVec);

  if constexpr (ElementSize == 4) {
    // For 32-bit float we need one more step to add all four results together
    Temp = _VFAddP(16, ElementSize, Temp, ZeroVec);
  }

  // Now using the destination mask we choose where the result ends up
  // It can duplicate and zero results
  auto Result = ZeroVec;

  for (size_t i = 0; i < (16 / ElementSize); ++i) {
    if (DstMask & (1 << i)) {
      Result = _VInsElement(16, ElementSize, i, 0, Result, Temp);
    }
  }

  StoreResult(FPRClass, Op, Result, -1);
}

template
void OpDispatchBuilder::DPPOp<4>(OpcodeArgs);
template
void OpDispatchBuilder::DPPOp<8>(OpcodeArgs);

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

}
