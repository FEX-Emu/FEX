// SPDX-License-Identifier: MIT
/*
$info$
tags: frontend|x86-to-ir, opcodes|dispatcher-implementations
desc: Handles x86/64 AVX instructions to 128-bit IR
$end_info$
*/

#include "Interface/Core/X86Tables/X86Tables.h"

#include <FEXCore/Utils/LogManager.h>
#include "Interface/Core/OpcodeDispatcher.h"

#include <array>
#include <cstdint>
#include <tuple>
#include <utility>

namespace FEXCore::IR {
class OrderedNode;

#define OpcodeArgs [[maybe_unused]] FEXCore::X86Tables::DecodedOp Op

OpDispatchBuilder::RefPair OpDispatchBuilder::AVX128_LoadSource_WithOpSize(
  const X86Tables::DecodedOp& Op, const X86Tables::DecodedOperand& Operand, uint32_t Flags, bool NeedsHigh, MemoryAccessType AccessType) {

  if (Operand.IsGPR()) {
    const auto gpr = Operand.Data.GPR.GPR;
    LOGMAN_THROW_A_FMT(gpr >= FEXCore::X86State::REG_XMM_0 && gpr <= FEXCore::X86State::REG_XMM_15, "must be AVX reg");
    const auto gprIndex = gpr - X86State::REG_XMM_0;
    return {
      .Low = AVX128_LoadXMMRegister(gprIndex, false),
      .High = NeedsHigh ? AVX128_LoadXMMRegister(gprIndex, true) : nullptr,
    };
  } else {
    LOGMAN_THROW_A_FMT(IsOperandMem(Operand, true), "only memory sources");

    if (Operand.IsSIB()) {
      const bool IsVSIB = (Op->Flags & X86Tables::DecodeFlags::FLAG_VSIB_BYTE) != 0;
      LOGMAN_THROW_A_FMT(!IsVSIB, "VSIB uses LoadVSIB instead");
    }

    const AddressMode A = DecodeAddress(Op, Operand, AccessType, true /* IsLoad */);
    if (NeedsHigh) {
      return _LoadMemPairFPRAutoTSO(OpSize::i128Bit, A, OpSize::i8Bit);
    } else {
      return {.Low = _LoadMemFPRAutoTSO(OpSize::i128Bit, A, OpSize::i8Bit)};
    }
  }
}

OpDispatchBuilder::RefVSIB
OpDispatchBuilder::AVX128_LoadVSIB(const X86Tables::DecodedOp& Op, const X86Tables::DecodedOperand& Operand, uint32_t Flags, bool NeedsHigh) {
  const bool IsVSIB = (Op->Flags & X86Tables::DecodeFlags::FLAG_VSIB_BYTE) != 0;
  LOGMAN_THROW_A_FMT((Operand.IsSIB() || Operand.IsSIBRelocation()) && IsVSIB, "Trying to load VSIB for something that isn't the correct "
                                                                               "type!");

  // VSIB is a very special case which has a ton of encoded data.
  // Get it in a format we can reason about.

  const auto Index_gpr = Operand.Data.SIB.Index;
  const auto Base_gpr = Operand.Data.SIB.Base;
  LOGMAN_THROW_A_FMT(Index_gpr >= FEXCore::X86State::REG_XMM_0 && Index_gpr <= FEXCore::X86State::REG_XMM_15, "must be AVX reg");
  LOGMAN_THROW_A_FMT(Base_gpr == FEXCore::X86State::REG_INVALID || (Base_gpr >= FEXCore::X86State::REG_RAX && Base_gpr <= FEXCore::X86State::REG_R15),
                     "Base must be a GPR.");
  const auto Index_XMM_gpr = Index_gpr - X86State::REG_XMM_0;

  OpDispatchBuilder::RefVSIB A {
    .Low = AVX128_LoadXMMRegister(Index_XMM_gpr, false),
    .High = NeedsHigh ? AVX128_LoadXMMRegister(Index_XMM_gpr, true) : Invalid(),
    .BaseAddr = Base_gpr != FEXCore::X86State::REG_INVALID ? LoadGPRRegister(Base_gpr, OpSize::i64Bit, 0, false) : nullptr,
    .Scale = Operand.Data.SIB.Scale,
  };

  if (Operand.IsSIBRelocation()) {
    auto EPOffset = _EntrypointOffset(OpSize::i64Bit, Operand.Data.SIB.Offset);
    if (A.BaseAddr) {
      A.BaseAddr = Add(OpSize::i64Bit, EPOffset, A.BaseAddr);
    } else {
      A.BaseAddr = EPOffset;
    }
  } else {
    A.Displacement = static_cast<int32_t>(Operand.Data.SIB.Offset);
  }

  return A;
}

void OpDispatchBuilder::AVX128_StoreResult_WithOpSize(FEXCore::X86Tables::DecodedOp Op, const FEXCore::X86Tables::DecodedOperand& Operand,
                                                      const RefPair Src, MemoryAccessType AccessType) {
  if (Operand.IsGPR()) {
    const auto gpr = Operand.Data.GPR.GPR;
    LOGMAN_THROW_A_FMT(gpr >= FEXCore::X86State::REG_XMM_0 && gpr <= FEXCore::X86State::REG_XMM_15, "expected AVX register");
    const auto gprIndex = gpr - X86State::REG_XMM_0;

    if (Src.Low) {
      AVX128_StoreXMMRegister(gprIndex, Src.Low, false);
    }

    if (Src.High) {
      AVX128_StoreXMMRegister(gprIndex, Src.High, true);
    }
  } else {
    AddressMode A = DecodeAddress(Op, Operand, AccessType, false /* IsLoad */);

    if (Src.High) {
      _StoreMemPairFPRAutoTSO(OpSize::i128Bit, A, Src.Low, Src.High, OpSize::i8Bit);
    } else {
      _StoreMemFPRAutoTSO(OpSize::i128Bit, A, Src.Low, OpSize::i8Bit);
    }
  }
}

Ref OpDispatchBuilder::AVX128_LoadXMMRegister(uint32_t XMM, bool High) {
  if (High) {
    return LoadContext(AVXHigh0Index + XMM);
  } else {
    return LoadXMMRegister(XMM);
  }
}

void OpDispatchBuilder::AVX128_StoreXMMRegister(uint32_t XMM, const Ref Src, bool High) {
  if (High) {
    StoreContext(AVXHigh0Index + XMM, Src);
  } else {
    StoreXMMRegister(XMM, Src);
  }
}

void OpDispatchBuilder::AVX128_VMOVAPS(OpcodeArgs) {
  // Reg <- Mem or Reg <- Reg
  const auto SrcSize = GetSrcSize(Op);
  const auto Is128Bit = SrcSize == Core::CPUState::XMM_SSE_REG_SIZE;

  if (Is128Bit) {
    // Zero upper 128-bits
    auto Src = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, false);

    ///< Zero upper bits when destination is GPR.
    if (Op->Dest.IsGPR()) {
      Src.High = LoadZeroVector(OpSize::i128Bit);
    }
    AVX128_StoreResult_WithOpSize(Op, Op->Dest, Src);
  } else {
    // Copy or memory load
    auto Src = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, true);
    AVX128_StoreResult_WithOpSize(Op, Op->Dest, Src);
  }
}

void OpDispatchBuilder::AVX128_VMOVScalarImpl(OpcodeArgs, IR::OpSize ElementSize) {
  if (Op->Dest.IsGPR() && Op->Src[0].IsGPR() && Op->Src[1].IsGPR()) {
    // VMOVSS/SD xmm1, xmm2, xmm3
    // Lower 128-bits are merged
    // Upper 128-bits are zero'd
    auto Src1 = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, false);
    auto Src2 = AVX128_LoadSource_WithOpSize(Op, Op->Src[1], Op->Flags, false);
    Ref Result = _VInsElement(OpSize::i128Bit, ElementSize, 0, 0, Src1.Low, Src2.Low);
    auto High = LoadZeroVector(OpSize::i128Bit);
    AVX128_StoreResult_WithOpSize(Op, Op->Dest, RefPair {.Low = Result, .High = High});
  } else if (Op->Dest.IsGPR()) {
    // VMOVSS/SD xmm1, mem32/mem64
    Ref Src = LoadSourceFPR_WithOpSize(Op, Op->Src[1], ElementSize, Op->Flags);
    auto High = LoadZeroVector(OpSize::i128Bit);
    AVX128_StoreResult_WithOpSize(Op, Op->Dest, RefPair {.Low = Src, .High = High});
  } else {
    // VMOVSS/SD mem32/mem64, xmm1
    auto Src = AVX128_LoadSource_WithOpSize(Op, Op->Src[1], Op->Flags, false);
    StoreResultFPR_WithOpSize(Op, Op->Dest, Src.Low, ElementSize);
  }
}

void OpDispatchBuilder::AVX128_VMOVSD(OpcodeArgs) {
  AVX128_VMOVScalarImpl(Op, OpSize::i64Bit);
}

void OpDispatchBuilder::AVX128_VMOVSS(OpcodeArgs) {
  AVX128_VMOVScalarImpl(Op, OpSize::i32Bit);
}

void OpDispatchBuilder::AVX128_VectorALU(OpcodeArgs, IROps IROp, IR::OpSize ElementSize) {
  const auto SrcSize = GetSrcSize(Op);
  const auto Is128Bit = SrcSize == Core::CPUState::XMM_SSE_REG_SIZE;

  auto Src1 = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, !Is128Bit);
  auto Src2 = AVX128_LoadSource_WithOpSize(Op, Op->Src[1], Op->Flags, !Is128Bit);
  DeriveOp(Result_Low, IROp, _VAdd(OpSize::i128Bit, ElementSize, Src1.Low, Src2.Low));

  if (Is128Bit) {
    auto High = LoadZeroVector(OpSize::i128Bit);
    AVX128_StoreResult_WithOpSize(Op, Op->Dest, RefPair {.Low = Result_Low, .High = High});
  } else {
    DeriveOp(Result_High, IROp, _VAdd(OpSize::i128Bit, ElementSize, Src1.High, Src2.High));
    AVX128_StoreResult_WithOpSize(Op, Op->Dest, RefPair {.Low = Result_Low, .High = Result_High});
  }
}

void OpDispatchBuilder::AVX128_VectorUnary(OpcodeArgs, IROps IROp, IR::OpSize ElementSize) {
  const auto SrcSize = GetSrcSize(Op);
  const auto Is128Bit = SrcSize == Core::CPUState::XMM_SSE_REG_SIZE;

  auto Src = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, !Is128Bit);
  DeriveOp(Result_Low, IROp, _VFSqrt(OpSize::i128Bit, ElementSize, Src.Low));

  if (Is128Bit) {
    auto High = LoadZeroVector(OpSize::i128Bit);
    AVX128_StoreResult_WithOpSize(Op, Op->Dest, RefPair {.Low = Result_Low, .High = High});
  } else {
    DeriveOp(Result_High, IROp, _VFSqrt(OpSize::i128Bit, ElementSize, Src.High));
    AVX128_StoreResult_WithOpSize(Op, Op->Dest, RefPair {.Low = Result_Low, .High = Result_High});
  }
}

void OpDispatchBuilder::AVX128_VectorUnaryImpl(OpcodeArgs, IR::OpSize SrcSize, IR::OpSize ElementSize,
                                               std::function<Ref(IR::OpSize ElementSize, Ref Src)> Helper) {
  const auto Is128Bit = SrcSize == OpSize::i128Bit;

  auto Src = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, !Is128Bit);
  RefPair Result {};
  Result.Low = Helper(ElementSize, Src.Low);

  if (Is128Bit) {
    Result.High = LoadZeroVector(OpSize::i128Bit);
  } else {
    Result.High = Helper(ElementSize, Src.High);
  }

  AVX128_StoreResult_WithOpSize(Op, Op->Dest, Result);
}

void OpDispatchBuilder::AVX128_VectorBinaryImpl(OpcodeArgs, IR::OpSize SrcSize, IR::OpSize ElementSize,
                                                std::function<Ref(IR::OpSize ElementSize, Ref Src1, Ref Src2)> Helper) {
  const auto Is128Bit = SrcSize == OpSize::i128Bit;

  auto Src1 = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, !Is128Bit);
  auto Src2 = AVX128_LoadSource_WithOpSize(Op, Op->Src[1], Op->Flags, !Is128Bit);
  RefPair Result {};
  Result.Low = Helper(ElementSize, Src1.Low, Src2.Low);

  if (Is128Bit) {
    Result.High = LoadZeroVector(OpSize::i128Bit);
  } else {
    Result.High = Helper(ElementSize, Src1.High, Src2.High);
  }

  AVX128_StoreResult_WithOpSize(Op, Op->Dest, Result);
}

void OpDispatchBuilder::AVX128_VectorTrinaryImpl(OpcodeArgs, IR::OpSize SrcSize, IR::OpSize ElementSize, Ref Src3,
                                                 std::function<Ref(IR::OpSize ElementSize, Ref Src1, Ref Src2, Ref Src3)> Helper) {
  const auto Is128Bit = SrcSize == OpSize::i128Bit;

  auto Src1 = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, !Is128Bit);
  auto Src2 = AVX128_LoadSource_WithOpSize(Op, Op->Src[1], Op->Flags, !Is128Bit);
  RefPair Result {};
  Result.Low = Helper(ElementSize, Src1.Low, Src2.Low, Src3);

  if (Is128Bit) {
    Result.High = LoadZeroVector(OpSize::i128Bit);
  } else {
    Result.High = Helper(ElementSize, Src1.High, Src2.High, Src3);
  }

  AVX128_StoreResult_WithOpSize(Op, Op->Dest, Result);
}

void OpDispatchBuilder::AVX128_VectorShiftWideImpl(OpcodeArgs, IR::OpSize ElementSize, IROps IROp) {
  const auto Is128Bit = GetSrcSize(Op) == Core::CPUState::XMM_SSE_REG_SIZE;

  auto Src1 = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, !Is128Bit);
  auto Src2 = AVX128_LoadSource_WithOpSize(Op, Op->Src[1], Op->Flags, false);

  // Incoming element size for the shift source is always 8-bytes in the lower register.
  DeriveOp(Low, IROp, _VUShrSWide(OpSize::i128Bit, ElementSize, Src1.Low, Src2.Low));

  RefPair Result {};
  Result.Low = Low;

  if (Is128Bit) {
    Result.High = LoadZeroVector(OpSize::i128Bit);
  } else {
    DeriveOp(High, IROp, _VUShrSWide(OpSize::i128Bit, ElementSize, Src1.High, Src2.Low));
    Result.High = High;
  }

  AVX128_StoreResult_WithOpSize(Op, Op->Dest, Result);
}

void OpDispatchBuilder::AVX128_VectorShiftImmImpl(OpcodeArgs, IR::OpSize ElementSize, IROps IROp) {
  const auto DstSize = GetDstSize(Op);
  const auto Is128Bit = DstSize == Core::CPUState::XMM_SSE_REG_SIZE;
  const uint64_t ShiftConstant = Op->Src[1].Literal();

  auto Src = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, !Is128Bit);
  RefPair Result {};

  if (ShiftConstant == 0) [[unlikely]] {
    Result = Src;
  } else {
    DeriveOp(Low, IROp, _VUShrI(OpSize::i128Bit, ElementSize, Src.Low, ShiftConstant));
    Result.Low = Low;

    if (!Is128Bit) {
      DeriveOp(High, IROp, _VUShrI(OpSize::i128Bit, ElementSize, Src.High, ShiftConstant));
      Result.High = High;
    }
  }

  if (Is128Bit) {
    Result.High = LoadZeroVector(OpSize::i128Bit);
  }

  AVX128_StoreResult_WithOpSize(Op, Op->Dest, Result);
}

void OpDispatchBuilder::AVX128_VectorXOR(OpcodeArgs) {
  // Special case for vector xor with itself being the optimal way for x86 to zero vector registers.
  if (Op->Src[0].IsGPR() && Op->Src[1].IsGPR() && Op->Src[0].Data.GPR.GPR == Op->Src[1].Data.GPR.GPR) {
    AVX128_StoreResult_WithOpSize(Op, Op->Dest, AVX128_Zext(LoadZeroVector(OpSize::i128Bit)));
    return;
  }

  ///< Regular code path
  AVX128_VectorALU(Op, OP_VXOR, OpSize::i128Bit);
}

void OpDispatchBuilder::AVX128_VZERO(OpcodeArgs) {
  const auto DstSize = GetDstSize(Op);
  const auto IsVZEROALL = DstSize == Core::CPUState::XMM_AVX_REG_SIZE;
  const auto NumRegs = Is64BitMode ? 16U : 8U;

  if (IsVZEROALL) {
    // NOTE: Despite the name being VZEROALL, this will still only ever
    //       zero out up to the first 16 registers (even on AVX-512, where we have 32 registers)
    Ref ZeroVector {};

    for (uint32_t i = 0; i < NumRegs; i++) {
      // Explicitly not caching named vector zero. This ensures that every register gets movi #0.0 directly.
      ZeroVector = LoadUncachedZeroVector(OpSize::i128Bit);
      AVX128_StoreXMMRegister(i, ZeroVector, false);
    }

    InvalidateHighAVXRegisters();
    _ContextClear(offsetof(FEXCore::Core::CPUState, avx_high), sizeof(FEXCore::Core::CPUState::avx_high[0]) * NumRegs);
  } else {
    // Likewise, VZEROUPPER will only ever zero only up to the first 16 registers
    InvalidateHighAVXRegisters();
    _ContextClear(offsetof(FEXCore::Core::CPUState, avx_high), sizeof(FEXCore::Core::CPUState::avx_high[0]) * NumRegs);
  }
}

void OpDispatchBuilder::AVX128_MOVVectorNT(OpcodeArgs) {
  const auto SrcSize = GetSrcSize(Op);
  const auto Is128Bit = SrcSize == Core::CPUState::XMM_SSE_REG_SIZE;

  if (Op->Dest.IsGPR()) {
    ///< MOVNTDQA load non-temporal comes from SSE4.1 and is extended by AVX/AVX2.
    RefPair Src {};
    Ref SrcAddr = LoadSourceGPR(Op, Op->Src[0], Op->Flags, {.LoadData = false});
    Src.Low = _VLoadNonTemporal(OpSize::i128Bit, SrcAddr, 0);

    if (Is128Bit) {
      Src.High = LoadZeroVector(OpSize::i128Bit);
    } else {
      Src.High = _VLoadNonTemporal(OpSize::i128Bit, SrcAddr, 16);
    }
    AVX128_StoreResult_WithOpSize(Op, Op->Dest, Src);
  } else {
    auto Src = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, !Is128Bit, MemoryAccessType::STREAM);
    Ref Dest = LoadSourceGPR(Op, Op->Dest, Op->Flags, {.LoadData = false});

    if (Is128Bit) {
      // Single store non-temporal for 128-bit operations.
      _VStoreNonTemporal(OpSize::i128Bit, Src.Low, Dest, 0);
    } else {
      // For a 256-bit store, use a non-temporal store pair
      _VStoreNonTemporalPair(OpSize::i128Bit, Src.Low, Src.High, Dest, 0);
    }
  }
}

void OpDispatchBuilder::AVX128_MOVQ(OpcodeArgs) {
  RefPair Src {};
  if (Op->Src[0].IsGPR()) {
    Src = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, false);
  } else {
    Src.Low = LoadSourceFPR_WithOpSize(Op, Op->Src[0], OpSize::i64Bit, Op->Flags);
  }

  // This instruction is a bit special that if the destination is a register then it'll ZEXT the 64bit source to 256bit
  if (Op->Dest.IsGPR()) {
    // Zero bits [127:64] as well.
    Src.Low = VZeroExtendOperand(OpSize::i64Bit, Op->Src[0], Src.Low);
    Ref ZeroVector = LoadZeroVector(OpSize::i128Bit);
    Src.High = ZeroVector;
    AVX128_StoreResult_WithOpSize(Op, Op->Dest, Src);
  } else {
    StoreResultFPR_WithOpSize(Op, Op->Dest, Src.Low, OpSize::i64Bit, OpSize::i64Bit);
  }
}

void OpDispatchBuilder::AVX128_VMOVLP(OpcodeArgs) {
  auto Src1 = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, false);

  if (!Op->Dest.IsGPR()) {
    ///< VMOVLPS/PD mem64, xmm1
    StoreResultFPR_WithOpSize(Op, Op->Dest, Src1.Low, OpSize::i64Bit, OpSize::i64Bit);
  } else if (!Op->Src[1].IsGPR()) {
    ///< VMOVLPS/PD xmm1, xmm2, mem64
    // Bits[63:0] come from Src2[63:0]
    // Bits[127:64] come from Src1[127:64]
    auto Src2 = MakeSegmentAddress(Op, Op->Src[1]);
    Ref Result_Low = _VLoadVectorElement(OpSize::i128Bit, OpSize::i64Bit, Src1.Low, 0, Src2);
    Ref ZeroVector = LoadZeroVector(OpSize::i128Bit);

    AVX128_StoreResult_WithOpSize(Op, Op->Dest, RefPair {.Low = Result_Low, .High = ZeroVector});
  } else {
    ///< VMOVHLPS/PD xmm1, xmm2, xmm3
    auto Src2 = AVX128_LoadSource_WithOpSize(Op, Op->Src[1], Op->Flags, false);

    Ref Result_Low = _VInsElement(OpSize::i128Bit, OpSize::i64Bit, 0, 1, Src1.Low, Src2.Low);
    Ref ZeroVector = LoadZeroVector(OpSize::i128Bit);

    AVX128_StoreResult_WithOpSize(Op, Op->Dest, RefPair {.Low = Result_Low, .High = ZeroVector});
  }
}

void OpDispatchBuilder::AVX128_VMOVHP(OpcodeArgs) {
  auto Src1 = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, false);

  if (!Op->Dest.IsGPR()) {
    ///< VMOVHPS/PD mem64, xmm1
    // Need to store Bits[127:64]. Use a vector element store.
    auto Dest = MakeSegmentAddress(Op, Op->Dest);
    _VStoreVectorElement(OpSize::i128Bit, OpSize::i64Bit, Src1.Low, 1, Dest);
  } else if (!Op->Src[1].IsGPR()) {
    ///< VMOVHPS/PD xmm2, xmm1, mem64
    auto Src2 = MakeSegmentAddress(Op, Op->Src[1]);

    // Bits[63:0] come from Src1[63:0]
    // Bits[127:64] come from Src2[63:0]
    Ref Result_Low = _VLoadVectorElement(OpSize::i128Bit, OpSize::i64Bit, Src1.Low, 1, Src2);
    Ref ZeroVector = LoadZeroVector(OpSize::i128Bit);

    AVX128_StoreResult_WithOpSize(Op, Op->Dest, RefPair {.Low = Result_Low, .High = ZeroVector});
  } else {
    // VMOVLHPS xmm1, xmm2, xmm3
    auto Src2 = AVX128_LoadSource_WithOpSize(Op, Op->Src[1], Op->Flags, false);

    Ref Result_Low = _VZip(OpSize::i128Bit, OpSize::i64Bit, Src1.Low, Src2.Low);
    Ref ZeroVector = LoadZeroVector(OpSize::i128Bit);

    AVX128_StoreResult_WithOpSize(Op, Op->Dest, RefPair {.Low = Result_Low, .High = ZeroVector});
  }
}

void OpDispatchBuilder::AVX128_VMOVDDUP(OpcodeArgs) {
  const auto SrcSize = GetSrcSize(Op);
  const auto Is128Bit = SrcSize == Core::CPUState::XMM_SSE_REG_SIZE;

  const auto IsSrcGPR = Op->Src[0].IsGPR();

  RefPair Src {};
  if (IsSrcGPR) {
    Src = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, !Is128Bit);
  } else {
    // Accesses from memory are a little weird.
    // 128-bit operation only loads 8-bytes.
    // 256-bit operation loads a full 32-bytes.
    if (Is128Bit) {
      Src.Low = LoadSourceFPR_WithOpSize(Op, Op->Src[0], OpSize::i64Bit, Op->Flags);
    } else {
      Src = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, true);
    }
  }

  if (Is128Bit) {
    // Duplicate Src[63:0] in to low 128-bits
    auto Result_Low = _VDupElement(OpSize::i128Bit, OpSize::i64Bit, Src.Low, 0);
    Ref ZeroVector = LoadZeroVector(OpSize::i128Bit);

    AVX128_StoreResult_WithOpSize(Op, Op->Dest, RefPair {.Low = Result_Low, .High = ZeroVector});
  } else {
    // Duplicate Src.Low[63:0] in to low 128-bits
    auto Result_Low = _VDupElement(OpSize::i128Bit, OpSize::i64Bit, Src.Low, 0);
    // Duplicate Src.High[63:0] in to high 128-bits
    auto Result_High = _VDupElement(OpSize::i128Bit, OpSize::i64Bit, Src.High, 0);
    AVX128_StoreResult_WithOpSize(Op, Op->Dest, RefPair {.Low = Result_Low, .High = Result_High});
  }
}

void OpDispatchBuilder::AVX128_VMOVSLDUP(OpcodeArgs) {
  AVX128_VectorUnaryImpl(Op, OpSizeFromSrc(Op), OpSize::i32Bit,
                         [this](IR::OpSize ElementSize, Ref Src) { return _VTrn(OpSize::i128Bit, ElementSize, Src, Src); });
}

void OpDispatchBuilder::AVX128_VMOVSHDUP(OpcodeArgs) {
  AVX128_VectorUnaryImpl(Op, OpSizeFromSrc(Op), OpSize::i32Bit,
                         [this](IR::OpSize ElementSize, Ref Src) { return _VTrn2(OpSize::i128Bit, ElementSize, Src, Src); });
}

void OpDispatchBuilder::AVX128_VBROADCAST(OpcodeArgs, IR::OpSize ElementSize) {
  const auto DstSize = GetDstSize(Op);
  const auto Is128Bit = DstSize == Core::CPUState::XMM_SSE_REG_SIZE;
  RefPair Src {};

  if (Op->Src[0].IsGPR()) {
    Src = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, false);
    if (ElementSize != OpSize::i128Bit) {
      // Only duplicate if not VBROADCASTF128.
      Src.Low = _VDupElement(OpSize::i128Bit, ElementSize, Src.Low, 0);
    }
  } else {
    // Get the address to broadcast from into a GPR.
    Ref Address = MakeSegmentAddress(Op, Op->Src[0], GetGPROpSize());
    Src.Low = _VBroadcastFromMem(OpSize::i128Bit, ElementSize, Address);
  }

  if (Is128Bit) {
    Src.High = LoadZeroVector(OpSize::i128Bit);
  } else {
    Src.High = Src.Low;
  }

  AVX128_StoreResult_WithOpSize(Op, Op->Dest, Src);
}

void OpDispatchBuilder::AVX128_VPUNPCKL(OpcodeArgs, IR::OpSize ElementSize) {
  AVX128_VectorBinaryImpl(Op, OpSizeFromSrc(Op), ElementSize,
                          [this](IR::OpSize _ElementSize, Ref Src1, Ref Src2) { return _VZip(OpSize::i128Bit, _ElementSize, Src1, Src2); });
}

void OpDispatchBuilder::AVX128_VPUNPCKH(OpcodeArgs, IR::OpSize ElementSize) {
  AVX128_VectorBinaryImpl(Op, OpSizeFromSrc(Op), ElementSize,
                          [this](IR::OpSize _ElementSize, Ref Src1, Ref Src2) { return _VZip2(OpSize::i128Bit, _ElementSize, Src1, Src2); });
}

void OpDispatchBuilder::AVX128_MOVVectorUnaligned(OpcodeArgs) {
  const auto SrcSize = GetSrcSize(Op);
  const auto Is128Bit = SrcSize == Core::CPUState::XMM_SSE_REG_SIZE;

  if (!Is128Bit && Op->Dest.IsGPR() && Op->Src[0].IsGPR() && Op->Dest.Data.GPR.GPR == Op->Src[0].Data.GPR.GPR) {
    // Nop
    return;
  }

  auto Src = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, !Is128Bit);

  if (Is128Bit) {
    Src.High = LoadZeroVector(OpSize::i128Bit);
  }

  AVX128_StoreResult_WithOpSize(Op, Op->Dest, Src);
}

void OpDispatchBuilder::AVX128_InsertCVTGPR_To_FPR(OpcodeArgs, IR::OpSize DstElementSize) {
  const auto SrcSize = OpSizeFromSrc(Op);
  const auto DstSize = OpSizeFromDst(Op);

  auto Src1 = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, false);

  RefPair Result {};

  if (Op->Src[1].IsGPR()) {
    // If the source is a GPR then convert directly from the GPR.
    auto Src2 = LoadSourceGPR_WithOpSize(Op, Op->Src[1], GetGPROpSize(), Op->Flags);
    Result.Low = _VSToFGPRInsert(OpSize::i128Bit, DstElementSize, SrcSize, Src1.Low, Src2, false);
  } else if (SrcSize != DstElementSize) {
    // If the source is from memory but the Source size and destination size aren't the same,
    // then it is more optimal to load in to a GPR and convert between GPR->FPR.
    // ARM GPR->FPR conversion supports different size source and destinations while FPR->FPR doesn't.
    auto Src2 = LoadSourceGPR(Op, Op->Src[1], Op->Flags);
    Result.Low = _VSToFGPRInsert(DstSize, DstElementSize, SrcSize, Src1.Low, Src2, false);
  } else {
    // In the case of cvtsi2s{s,d} where the source and destination are the same size,
    // then it is more optimal to load in to the FPR register directly and convert there.
    auto Src2 = LoadSourceFPR(Op, Op->Src[1], Op->Flags);
    // Always signed
    Result.Low = _VSToFVectorInsert(DstSize, DstElementSize, DstElementSize, Src1.Low, Src2, false, false);
  }

  const auto Is128Bit = DstSize == OpSize::i128Bit;
  LOGMAN_THROW_A_FMT(Is128Bit, "Programming Error: This should never occur!");
  Result.High = LoadZeroVector(OpSize::i128Bit);

  AVX128_StoreResult_WithOpSize(Op, Op->Dest, Result);
}

void OpDispatchBuilder::AVX128_CVTFPR_To_GPR(OpcodeArgs, IR::OpSize SrcElementSize, bool HostRoundingMode) {
  // If loading a vector, use the full size, so we don't
  // unnecessarily zero extend the vector. Otherwise, if
  // memory, then we want to load the element size exactly.
  RefPair Src {};
  if (Op->Src[0].IsGPR()) {
    Src = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, false);
  } else {
    Src.Low = LoadSourceFPR_WithOpSize(Op, Op->Src[0], SrcElementSize, Op->Flags);
  }

  Ref Result = CVTFPR_To_GPRImpl(Op, Src.Low, SrcElementSize, HostRoundingMode);
  StoreResultGPR(Op, Result);
}

void OpDispatchBuilder::AVX128_VANDN(OpcodeArgs) {
  AVX128_VectorBinaryImpl(Op, OpSizeFromSrc(Op), OpSize::i128Bit,
                          [this](IR::OpSize _ElementSize, Ref Src1, Ref Src2) { return _VAndn(OpSize::i128Bit, _ElementSize, Src2, Src1); });
}

void OpDispatchBuilder::AVX128_VPACKSS(OpcodeArgs, IR::OpSize ElementSize) {
  AVX128_VectorBinaryImpl(Op, OpSizeFromSrc(Op), ElementSize, [this](IR::OpSize _ElementSize, Ref Src1, Ref Src2) {
    return _VSQXTNPair(OpSize::i128Bit, _ElementSize, Src1, Src2);
  });
}

void OpDispatchBuilder::AVX128_VPACKUS(OpcodeArgs, IR::OpSize ElementSize) {
  AVX128_VectorBinaryImpl(Op, OpSizeFromSrc(Op), ElementSize, [this](IR::OpSize _ElementSize, Ref Src1, Ref Src2) {
    return _VSQXTUNPair(OpSize::i128Bit, _ElementSize, Src1, Src2);
  });
}

Ref OpDispatchBuilder::AVX128_PSIGNImpl(IR::OpSize ElementSize, Ref Src1, Ref Src2) {
  Ref Control = _VSQSHL(OpSize::i128Bit, ElementSize, Src2, IR::OpSizeAsBits(ElementSize) - 1);
  Control = _VSRSHR(OpSize::i128Bit, ElementSize, Control, IR::OpSizeAsBits(ElementSize) - 1);
  return _VMul(OpSize::i128Bit, ElementSize, Src1, Control);
}

void OpDispatchBuilder::AVX128_VPSIGN(OpcodeArgs, IR::OpSize ElementSize) {
  AVX128_VectorBinaryImpl(Op, OpSizeFromSrc(Op), ElementSize,
                          [this](IR::OpSize _ElementSize, Ref Src1, Ref Src2) { return AVX128_PSIGNImpl(_ElementSize, Src1, Src2); });
}

void OpDispatchBuilder::AVX128_UCOMISx(OpcodeArgs, IR::OpSize ElementSize) {
  const auto SrcSize = Op->Src[0].IsGPR() ? GetGuestVectorLength() : ElementSize;

  auto Src1 = AVX128_LoadSource_WithOpSize(Op, Op->Dest, Op->Flags, false);

  RefPair Src2 {};

  // Careful here, if the source is from a GPR then we want to load the full 128-bit lower half.
  // If it is memory then we only want to load the element size.
  if (Op->Src[0].IsGPR()) {
    Src2 = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, false);
  } else {
    Src2.Low = LoadSourceFPR_WithOpSize(Op, Op->Src[0], SrcSize, Op->Flags);
  }

  Comiss(ElementSize, Src1.Low, Src2.Low);
}

void OpDispatchBuilder::AVX128_VectorScalarInsertALU(OpcodeArgs, FEXCore::IR::IROps IROp, IR::OpSize ElementSize) {
  // We load the full vector width when dealing with a source vector,
  // so that we don't do any unnecessary zero extension to the scalar
  // element that we're going to operate on.
  const auto SrcSize = OpSizeFromSrc(Op);

  auto Src1 = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, false);
  RefPair Src2 {};
  if (Op->Src[1].IsGPR()) {
    Src2 = AVX128_LoadSource_WithOpSize(Op, Op->Src[1], Op->Flags, false);
  } else {
    Src2.Low = LoadSourceFPR_WithOpSize(Op, Op->Src[1], SrcSize, Op->Flags);
  }

  // If OpSize == ElementSize then it only does the lower scalar op
  DeriveOp(Result_Low, IROp, _VFAddScalarInsert(OpSize::i128Bit, ElementSize, Src1.Low, Src2.Low, false));
  auto High = LoadZeroVector(OpSize::i128Bit);
  AVX128_StoreResult_WithOpSize(Op, Op->Dest, RefPair {.Low = Result_Low, .High = High});
}

void OpDispatchBuilder::AVX128_VFCMP(OpcodeArgs, IR::OpSize ElementSize) {
  const uint8_t CompType = Op->Src[2].Literal();

  struct {
    FEXCore::X86Tables::DecodedOp Op;
    uint8_t CompType {};
  } Capture {
    .Op = Op,
    .CompType = CompType,
  };

  AVX128_VectorBinaryImpl(Op, OpSizeFromSrc(Op), ElementSize, [this, &Capture](IR::OpSize _ElementSize, Ref Src1, Ref Src2) {
    return VFCMPOpImpl(OpSize::i128Bit, _ElementSize, Src1, Src2, Capture.CompType);
  });
}

void OpDispatchBuilder::AVX128_InsertScalarFCMP(OpcodeArgs, IR::OpSize ElementSize) {
  // We load the full vector width when dealing with a source vector,
  // so that we don't do any unnecessary zero extension to the scalar
  // element that we're going to operate on.
  const auto SrcSize = OpSizeFromSrc(Op);

  auto Src1 = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, false);
  RefPair Src2 {};

  if (Op->Src[1].IsGPR()) {
    Src2 = AVX128_LoadSource_WithOpSize(Op, Op->Src[1], Op->Flags, false);
  } else {
    Src2.Low = LoadSourceFPR_WithOpSize(Op, Op->Src[1], SrcSize, Op->Flags);
  }

  const uint8_t CompType = Op->Src[2].Literal();

  RefPair Result {};
  Result.Low = InsertScalarFCMPOpImpl(OpSize::i128Bit, OpSize::i128Bit, ElementSize, Src1.Low, Src2.Low, CompType, false);
  Result.High = LoadZeroVector(OpSize::i128Bit);
  AVX128_StoreResult_WithOpSize(Op, Op->Dest, Result);
}

void OpDispatchBuilder::AVX128_MOVBetweenGPR_FPR(OpcodeArgs) {
  if (Op->Dest.IsGPR() && Op->Dest.Data.GPR.GPR >= FEXCore::X86State::REG_XMM_0) {
    ///< XMM <- Reg/Mem

    RefPair Result {};
    if (Op->Src[0].IsGPR()) {
      // Loading from GPR and moving to Vector.
      Ref Src = LoadSourceFPR_WithOpSize(Op, Op->Src[0], GetGPROpSize(), Op->Flags);
      // zext to 128bit
      Result.Low = _VCastFromGPR(OpSize::i128Bit, OpSizeFromSrc(Op), Src);
    } else {
      // Loading from Memory as a scalar. Zero extend
      Result.Low = LoadSourceFPR(Op, Op->Src[0], Op->Flags);
    }

    Result.High = LoadZeroVector(OpSize::i128Bit);
    AVX128_StoreResult_WithOpSize(Op, Op->Dest, Result);
  } else {
    ///< Reg/Mem <- XMM
    auto Src = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, false);

    if (Op->Dest.IsGPR()) {
      auto ElementSize = OpSizeFromDst(Op);
      // Extract element from GPR. Zero extending in the process.
      Src.Low = _VExtractToGPR(OpSizeFromSrc(Op), ElementSize, Src.Low, 0);
      StoreResultGPR(Op, Op->Dest, Src.Low);
    } else {
      // Storing first element to memory.
      Ref Dest = LoadSourceGPR(Op, Op->Dest, Op->Flags, {.LoadData = false});
      _StoreMemFPR(OpSizeFromDst(Op), Dest, Src.Low, OpSize::i8Bit);
    }
  }
}

void OpDispatchBuilder::AVX128_PExtr(OpcodeArgs, IR::OpSize ElementSize) {
  const auto DstSize = OpSizeFromDst(Op);

  auto Src = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, false);
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
  const uint8_t NumElements = IR::NumElements(std::min(OpSizeFromSrc(Op), OpSize::i128Bit), OverridenElementSize);
  Index &= NumElements - 1;

  if (Op->Dest.IsGPR()) {
    const auto GPRSize = GetGPROpSize();
    // Extract already zero extends the result.
    Ref Result = _VExtractToGPR(OpSize::i128Bit, OverridenElementSize, Src.Low, Index);
    StoreResultGPR_WithOpSize(Op, Op->Dest, Result, GPRSize);
    return;
  }

  // If we are storing to memory then we store the size of the element extracted
  Ref Dest = MakeSegmentAddress(Op, Op->Dest);
  _VStoreVectorElement(OpSize::i128Bit, OverridenElementSize, Src.Low, Index, Dest);
}

void OpDispatchBuilder::AVX128_ExtendVectorElements(OpcodeArgs, IR::OpSize ElementSize, IR::OpSize DstElementSize, bool Signed) {
  const auto DstSize = OpSizeFromDst(Op);

  const auto GetSrc = [&] {
    if (Op->Src[0].IsGPR()) {
      return AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, false).Low;
    } else {
      // For memory operands the 256-bit variant loads twice the size specified in the table.
      const auto Is256Bit = DstSize == OpSize::i256Bit;
      const auto SrcSize = OpSizeFromSrc(Op);
      const auto LoadSize = Is256Bit ? IR::SizeToOpSize(IR::OpSizeToSize(SrcSize) * 2) : SrcSize;

      return LoadSourceFPR_WithOpSize(Op, Op->Src[0], LoadSize, Op->Flags);
    }
  };

  auto Transform = [=, this](Ref Src) {
    for (auto CurrentElementSize = ElementSize; CurrentElementSize != DstElementSize; CurrentElementSize = CurrentElementSize << 1) {
      if (Signed) {
        Src = _VSXTL(OpSize::i128Bit, CurrentElementSize, Src);
      } else {
        Src = _VUXTL(OpSize::i128Bit, CurrentElementSize, Src);
      }
    }
    return Src;
  };

  Ref Src = GetSrc();
  RefPair Result {};

  if (DstSize == OpSize::i128Bit) {
    // 128-bit operation is easy, it stays within the single register.
    Result.Low = Transform(Src);
  } else {
    // 256-bit operation is a bit special. It splits the incoming source between lower and upper registers.
    size_t TotalElementCount = IR::NumElements(OpSize::i256Bit, DstElementSize);
    size_t TotalElementsToSplitSize = (TotalElementCount / 2) * IR::OpSizeToSize(ElementSize);

    // Split the number of elements in half between lower and upper.
    Ref SrcHigh = _VDupElement(OpSize::i128Bit, IR::SizeToOpSize(TotalElementsToSplitSize), Src, 1);
    Result.Low = Transform(Src);
    Result.High = Transform(SrcHigh);
  }

  if (DstSize == OpSize::i128Bit) {
    // Regular zero-extending semantics.
    Result.High = LoadZeroVector(OpSize::i128Bit);
  }

  AVX128_StoreResult_WithOpSize(Op, Op->Dest, Result);
}

void OpDispatchBuilder::AVX128_MOVMSK(OpcodeArgs, IR::OpSize ElementSize) {
  const auto SrcSize = OpSizeFromSrc(Op);
  const auto Is128Bit = SrcSize == OpSize::i128Bit;

  auto Src = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, !Is128Bit);

  auto Mask8Byte = [this](Ref Src) {
    // UnZip2 the 64-bit elements as 32-bit to get the sign bits closer.
    // Sign bits are now in bit positions 31 and 63 after this.
    Src = _VUnZip2(OpSize::i128Bit, OpSize::i32Bit, Src, Src);

    // Extract the low 64-bits to GPR in one move.
    Ref GPR = _VExtractToGPR(OpSize::i128Bit, OpSize::i64Bit, Src, 0);
    // BFI the sign bit in 31 in to 62.
    // Inserting the full lower 32-bits offset 31 so the sign bit ends up at offset 63.
    GPR = _Bfi(OpSize::i64Bit, 32, 31, GPR, GPR);
    // Shift right to only get the two sign bits we care about.
    return _Lshr(OpSize::i64Bit, GPR, Constant(62));
  };

  auto Mask4Byte = [this](Ref Src) {
    // Shift all the sign bits to the bottom of their respective elements.
    Src = _VUShrI(OpSize::i128Bit, OpSize::i32Bit, Src, 31);
    // Load the specific 128-bit movmskps shift elements operator.
    auto ConstantUSHL = LoadAndCacheNamedVectorConstant(OpSize::i128Bit, NAMED_VECTOR_MOVMSKPS_SHIFT);
    // Shift the sign bits in to specific locations.
    Src = _VUShl(OpSize::i128Bit, OpSize::i32Bit, Src, ConstantUSHL, false);
    // Add across the vector so the sign bits will end up in bits [3:0]
    Src = _VAddV(OpSize::i128Bit, OpSize::i32Bit, Src);
    // Extract to a GPR.
    return _VExtractToGPR(OpSize::i128Bit, OpSize::i32Bit, Src, 0);
  };

  Ref GPR {};
  if (Is128Bit) {
    if (ElementSize == OpSize::i64Bit) {
      GPR = Mask8Byte(Src.Low);
    } else {
      GPR = Mask4Byte(Src.Low);
    }
  } else if (ElementSize == OpSize::i32Bit) {
    auto GPRLow = Mask4Byte(Src.Low);
    auto GPRHigh = Mask4Byte(Src.High);
    GPR = _Orlshl(OpSize::i64Bit, GPRLow, GPRHigh, 4);
  } else {
    auto GPRLow = Mask8Byte(Src.Low);
    auto GPRHigh = Mask8Byte(Src.High);
    GPR = _Orlshl(OpSize::i64Bit, GPRLow, GPRHigh, 2);
  }
  StoreResultGPR_WithOpSize(Op, Op->Dest, GPR, GetGPROpSize());
}

void OpDispatchBuilder::AVX128_MOVMSKB(OpcodeArgs) {
  const auto SrcSize = GetSrcSize(Op);
  const auto Is128Bit = SrcSize == Core::CPUState::XMM_SSE_REG_SIZE;

  auto Src = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, !Is128Bit);
  Ref VMask = LoadAndCacheNamedVectorConstant(OpSize::i128Bit, NAMED_VECTOR_MOVMASKB);

  auto Mask1Byte = [this](Ref Src, Ref VMask) {
    auto VCMP = _VCMPLTZ(OpSize::i128Bit, OpSize::i8Bit, Src);
    auto VAnd = _VAnd(OpSize::i128Bit, OpSize::i8Bit, VCMP, VMask);

    auto VAdd1 = _VAddP(OpSize::i128Bit, OpSize::i8Bit, VAnd, VAnd);
    auto VAdd2 = _VAddP(OpSize::i128Bit, OpSize::i8Bit, VAdd1, VAdd1);
    auto VAdd3 = _VAddP(OpSize::i64Bit, OpSize::i8Bit, VAdd2, VAdd2);

    ///< 16-bits of data per 128-bit
    return _VExtractToGPR(OpSize::i128Bit, OpSize::i16Bit, VAdd3, 0);
  };

  Ref Result = Mask1Byte(Src.Low, VMask);

  if (!Is128Bit) {
    auto ResultHigh = Mask1Byte(Src.High, VMask);
    Result = _Orlshl(OpSize::i64Bit, Result, ResultHigh, 16);
  }

  StoreResultGPR(Op, Result);
}

void OpDispatchBuilder::AVX128_PINSRImpl(OpcodeArgs, IR::OpSize ElementSize, const X86Tables::DecodedOperand& Src1Op,
                                         const X86Tables::DecodedOperand& Src2Op, const X86Tables::DecodedOperand& Imm) {
  const auto NumElements = IR::NumElements(OpSize::i128Bit, ElementSize);
  const uint64_t Index = Imm.Literal() & (NumElements - 1);
  auto Src1 = AVX128_LoadSource_WithOpSize(Op, Src1Op, Op->Flags, false);

  RefPair Result {};

  if (Src2Op.IsGPR()) {
    // If the source is a GPR then convert directly from the GPR.
    auto Src2 = LoadSourceGPR_WithOpSize(Op, Src2Op, GetGPROpSize(), Op->Flags);
    Result.Low = _VInsGPR(OpSize::i128Bit, ElementSize, Index, Src1.Low, Src2);
  } else {
    // If loading from memory then we only load the element size
    auto Src2 = MakeSegmentAddress(Op, Src2Op);
    Result.Low = _VLoadVectorElement(OpSize::i128Bit, ElementSize, Src1.Low, Index, Src2);
  }

  Result.High = LoadZeroVector(OpSize::i128Bit);
  AVX128_StoreResult_WithOpSize(Op, Op->Dest, Result);
}

void OpDispatchBuilder::AVX128_VPINSRB(OpcodeArgs) {
  AVX128_PINSRImpl(Op, OpSize::i8Bit, Op->Src[0], Op->Src[1], Op->Src[2]);
}

void OpDispatchBuilder::AVX128_VPINSRW(OpcodeArgs) {
  AVX128_PINSRImpl(Op, OpSize::i16Bit, Op->Src[0], Op->Src[1], Op->Src[2]);
}

void OpDispatchBuilder::AVX128_VPINSRDQ(OpcodeArgs) {
  const auto SrcSize = OpSizeFromSrc(Op);
  AVX128_PINSRImpl(Op, SrcSize, Op->Src[0], Op->Src[1], Op->Src[2]);
}

void OpDispatchBuilder::AVX128_VariableShiftImpl(OpcodeArgs, IROps IROp) {
  AVX128_VectorBinaryImpl(Op, OpSizeFromDst(Op), OpSizeFromSrc(Op), [this, IROp](IR::OpSize ElementSize, Ref Src1, Ref Src2) {
    DeriveOp(Shift, IROp, _VUShr(OpSize::i128Bit, ElementSize, Src1, Src2, true));
    return Shift;
  });
}

void OpDispatchBuilder::AVX128_ShiftDoubleImm(OpcodeArgs, ShiftDirection Dir) {
  const auto DstSize = GetDstSize(Op);
  const auto Is128Bit = DstSize == Core::CPUState::XMM_SSE_REG_SIZE;
  const bool Right = Dir == ShiftDirection::RIGHT;

  const uint64_t Shift = Op->Src[1].Literal();
  const uint64_t ExtrShift = Right ? Shift : IR::OpSizeToSize(OpSize::i128Bit) - Shift;

  auto Src = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, !Is128Bit);

  RefPair Result {};
  if (Shift == 0) [[unlikely]] {
    Result = Src;
  } else if (Shift >= Core::CPUState::XMM_SSE_REG_SIZE) {
    Result.Low = LoadZeroVector(OpSize::i128Bit);
    Result.High = Result.Low;
  } else {
    Ref ZeroVector = LoadZeroVector(OpSize::i128Bit);
    RefPair Zero {ZeroVector, ZeroVector};
    RefPair Src1 = Right ? Zero : Src;
    RefPair Src2 = Right ? Src : Zero;

    Result.Low = _VExtr(OpSize::i128Bit, OpSize::i8Bit, Src1.Low, Src2.Low, ExtrShift);
    if (!Is128Bit) {
      Result.High = _VExtr(OpSize::i128Bit, OpSize::i8Bit, Src1.High, Src2.High, ExtrShift);
    }
  }

  if (Is128Bit) {
    Result.High = LoadZeroVector(OpSize::i128Bit);
  }

  AVX128_StoreResult_WithOpSize(Op, Op->Dest, Result);
}

void OpDispatchBuilder::AVX128_VINSERT(OpcodeArgs) {
  const auto DstSize = GetDstSize(Op);
  const auto Is128Bit = DstSize == Core::CPUState::XMM_SSE_REG_SIZE;
  const auto Selector = Op->Src[2].Literal() & 1;

  auto Result = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, !Is128Bit);
  auto Src2 = AVX128_LoadSource_WithOpSize(Op, Op->Src[1], Op->Flags, false);

  if (Selector == 0) {
    // Insert in to Low bits
    Result.Low = Src2.Low;
  } else {
    // Insert in to the High bits
    Result.High = Src2.Low;
  }

  AVX128_StoreResult_WithOpSize(Op, Op->Dest, Result);
}

void OpDispatchBuilder::AVX128_VINSERTPS(OpcodeArgs) {
  Ref Result = InsertPSOpImpl(Op, Op->Src[0], Op->Src[1], Op->Src[2]);
  AVX128_StoreResult_WithOpSize(Op, Op->Dest, AVX128_Zext(Result));
}

void OpDispatchBuilder::AVX128_VPHSUB(OpcodeArgs, IR::OpSize ElementSize) {
  AVX128_VectorBinaryImpl(Op, OpSizeFromDst(Op), ElementSize, [this](IR::OpSize _ElementSize, Ref Src1, Ref Src2) {
    return PHSUBOpImpl(OpSize::i128Bit, Src1, Src2, _ElementSize);
  });
}

void OpDispatchBuilder::AVX128_VPHSUBSW(OpcodeArgs) {
  AVX128_VectorBinaryImpl(Op, OpSizeFromDst(Op), OpSize::i16Bit,
                          [this](IR::OpSize _ElementSize, Ref Src1, Ref Src2) { return PHSUBSOpImpl(OpSize::i128Bit, Src1, Src2); });
}

void OpDispatchBuilder::AVX128_VADDSUBP(OpcodeArgs, IR::OpSize ElementSize) {
  AVX128_VectorBinaryImpl(Op, OpSizeFromDst(Op), ElementSize, [this](IR::OpSize _ElementSize, Ref Src1, Ref Src2) {
    return ADDSUBPOpImpl(OpSize::i128Bit, _ElementSize, Src1, Src2);
  });
}

void OpDispatchBuilder::AVX128_VPMULL(OpcodeArgs, IR::OpSize ElementSize, bool Signed) {
  LOGMAN_THROW_A_FMT(ElementSize == OpSize::i32Bit, "Currently only handles 32-bit -> 64-bit");

  AVX128_VectorBinaryImpl(Op, OpSizeFromDst(Op), ElementSize, [&](IR::OpSize _ElementSize, Ref Src1, Ref Src2) -> Ref {
    return PMULLOpImpl(OpSize::i128Bit, _ElementSize, Signed, Src1, Src2);
  });
}

void OpDispatchBuilder::AVX128_VPMULHRSW(OpcodeArgs) {
  AVX128_VectorBinaryImpl(Op, OpSizeFromDst(Op), OpSize::i16Bit,
                          [this](IR::OpSize _ElementSize, Ref Src1, Ref Src2) -> Ref { return PMULHRSWOpImpl(OpSize::i128Bit, Src1, Src2); });
}

void OpDispatchBuilder::AVX128_VPMULHW(OpcodeArgs, bool Signed) {
  AVX128_VectorBinaryImpl(Op, OpSizeFromDst(Op), OpSize::i16Bit, [&](IR::OpSize _ElementSize, Ref Src1, Ref Src2) -> Ref {
    if (Signed) {
      return _VSMulH(OpSize::i128Bit, _ElementSize, Src1, Src2);
    } else {
      return _VUMulH(OpSize::i128Bit, _ElementSize, Src1, Src2);
    }
  });
}

void OpDispatchBuilder::AVX128_InsertScalar_CVT_Float_To_Float(OpcodeArgs, IR::OpSize DstElementSize, IR::OpSize SrcElementSize) {
  // Gotta be careful with this operation.
  // It inserts in to the lowest element, retaining the remainder of the lower 128-bits.
  // Then zero extends the top 128-bit.
  const auto SrcSize = Op->Src[1].IsGPR() ? OpSize::i128Bit : SrcElementSize;
  auto Src1 = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, false);
  Ref Src2 = LoadSourceFPR_WithOpSize(Op, Op->Src[1], SrcSize, Op->Flags, {.AllowUpperGarbage = true});

  Ref Result = _VFToFScalarInsert(OpSize::i128Bit, DstElementSize, SrcElementSize, Src1.Low, Src2, false);
  AVX128_StoreResult_WithOpSize(Op, Op->Dest, AVX128_Zext(Result));
}

void OpDispatchBuilder::AVX128_Vector_CVT_Float_To_Float(OpcodeArgs, IR::OpSize DstElementSize, IR::OpSize SrcElementSize) {
  const auto SrcSize = OpSizeFromSrc(Op);
  const auto DstSize = OpSizeFromDst(Op);

  const auto IsFloatSrc = SrcElementSize == OpSize::i32Bit;
  auto Is128BitSrc = SrcSize == OpSize::i128Bit;
  auto Is128BitDst = DstSize == OpSize::i128Bit;

  ///< Decompose correctly.
  if (DstElementSize > SrcElementSize && !Is128BitDst) {
    Is128BitSrc = true;
  } else if (SrcElementSize > DstElementSize && !Is128BitSrc) {
    Is128BitDst = true;
  }

  const auto LoadSize = IsFloatSrc && !Op->Src[0].IsGPR() ? IR::SizeToOpSize(IR::OpSizeToSize(SrcSize) / 2) : SrcSize;

  RefPair Src {};
  if (Op->Src[0].IsGPR() || LoadSize >= OpSize::i128Bit) {
    Src = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, !Is128BitSrc);
  } else {
    // Handle 64-bit memory source.
    // In the case of cvtps2pd xmm, m64.
    Src.Low = LoadSourceFPR_WithOpSize(Op, Op->Src[0], LoadSize, Op->Flags);
  }

  RefPair Result {};

  auto TransformLow = [&](Ref Src) -> Ref {
    return _Vector_FToF(OpSize::i128Bit, DstElementSize, Src, SrcElementSize);
  };

  auto TransformHigh = [&](Ref Src) -> Ref {
    return _VFCVTL2(OpSize::i128Bit, SrcElementSize, Src);
  };

  Result.Low = TransformLow(Src.Low);
  if (Is128BitSrc) {
    if (Is128BitDst) {
      // cvtps2pd xmm, xmm or cvtpd2ps xmm, xmm
      // Done here
    } else {
      LOGMAN_THROW_A_FMT(DstElementSize > SrcElementSize, "cvtpd2ps ymm, xmm doesn't exist");

      // cvtps2pd ymm, xmm
      Result.High = TransformHigh(Src.Low);
    }
  } else {
    // 256-bit src
    LOGMAN_THROW_A_FMT(Is128BitDst, "Not real: cvt{ps2pd,pd2ps} ymm, ymm");
    LOGMAN_THROW_A_FMT(DstElementSize < SrcElementSize, "cvtps2pd xmm, ymm doesn't exist");

    // cvtpd2ps xmm, ymm
    Result.Low = _VInsElement(OpSize::i128Bit, OpSize::i64Bit, 1, 0, Result.Low, TransformLow(Src.High));
  }

  if (Is128BitDst) {
    Result = AVX128_Zext(Result.Low);
  }

  AVX128_StoreResult_WithOpSize(Op, Op->Dest, Result);
}

void OpDispatchBuilder::AVX128_Vector_CVT_Float_To_Int(OpcodeArgs, IR::OpSize SrcElementSize, bool HostRoundingMode) {
  const auto SrcSize = GetSrcSize(Op);

  const auto Is128BitSrc = SrcSize == Core::CPUState::XMM_SSE_REG_SIZE;

  // VCVTPD2DQ/VCVTTPD2DQ only use the bottom lane, even for the 256-bit version.
  auto Src = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, !Is128BitSrc);
  RefPair Result {};

  Result.Low = Vector_CVT_Float_To_Int32Impl(Op, OpSize::i128Bit, Src.Low, OpSize::i128Bit, SrcElementSize, HostRoundingMode, Is128BitSrc);
  if (Is128BitSrc) {
    // Zero the upper 128-bit lane of the result.
    Result = AVX128_Zext(Result.Low);
  } else {
    Result.High = Vector_CVT_Float_To_Int32Impl(Op, OpSize::i128Bit, Src.High, OpSize::i128Bit, SrcElementSize, HostRoundingMode, false);
    // Also convert the upper 128-bit lane
    if (SrcElementSize == OpSize::i64Bit) {
      // Zip the two halves together in to the lower 128-bits
      Result.Low = _VZip(OpSize::i128Bit, OpSize::i64Bit, Result.Low, Result.High);

      // Zero the upper 128-bit lane of the result.
      Result = AVX128_Zext(Result.Low);
    }
  }

  AVX128_StoreResult_WithOpSize(Op, Op->Dest, Result);
}

void OpDispatchBuilder::AVX128_Vector_CVT_Int_To_Float(OpcodeArgs, IR::OpSize SrcElementSize, bool Widen) {
  const auto Size = OpSizeFromDst(Op);
  const auto Is128Bit = Size == OpSize::i128Bit;

  RefPair Src = [&] {
    if (Widen && !Op->Src[0].IsGPR()) {
      // If loading a vector, use the full size, so we don't
      // unnecessarily zero extend the vector. Otherwise, if
      // memory, then we want to load the element size exactly.
      const auto LoadSize = IR::SizeToOpSize(8 * (IR::OpSizeToSize(Size) / 16));
      return RefPair {.Low = LoadSourceFPR_WithOpSize(Op, Op->Src[0], LoadSize, Op->Flags)};
    } else {
      return AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, !Is128Bit);
    }
  }();

  auto Convert = [&](Ref Src, IROps Op) -> Ref {
    auto ElementSize = SrcElementSize;
    if (Widen) {
      DeriveOp(Extended, Op, _VSXTL(OpSize::i128Bit, ElementSize, Src));
      Src = Extended;
      ElementSize = ElementSize << 1;
    }

    return _Vector_SToF(OpSize::i128Bit, ElementSize, Src);
  };

  RefPair Result {};
  Result.Low = Convert(Src.Low, IROps::OP_VSXTL);

  if (Is128Bit) {
    Result = AVX128_Zext(Result.Low);
  } else {
    if (Widen) {
      Result.High = Convert(Src.Low, IROps::OP_VSXTL2);
    } else {
      Result.High = Convert(Src.High, IROps::OP_VSXTL);
    }
  }

  AVX128_StoreResult_WithOpSize(Op, Op->Dest, Result);
}

void OpDispatchBuilder::AVX128_VEXTRACT128(OpcodeArgs) {
  const auto DstIsXMM = Op->Dest.IsGPR();
  const auto Selector = Op->Src[1].Literal() & 0b1;

  auto Src = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, true);

  RefPair Result {};
  if (Selector == 0) {
    Result.Low = Src.Low;
  } else {
    Result.Low = Src.High;
  }

  if (DstIsXMM) {
    // Only zero the upper-half when destination is XMM, otherwise this is a memory store.
    Result = AVX128_Zext(Result.Low);
  }

  AVX128_StoreResult_WithOpSize(Op, Op->Dest, Result);
}

void OpDispatchBuilder::AVX128_VAESImc(OpcodeArgs) {
  ///< 128-bit only.
  AVX128_VectorUnaryImpl(Op, OpSize::i128Bit, OpSize::i128Bit, [this](IR::OpSize, Ref Src) { return _VAESImc(Src); });
}

void OpDispatchBuilder::AVX128_VAESEnc(OpcodeArgs) {
  AVX128_VectorTrinaryImpl(Op, OpSizeFromDst(Op), OpSize::i128Bit, LoadZeroVector(OpSize::i128Bit),
                           [this](IR::OpSize, Ref Src1, Ref Src2, Ref Src3) { return _VAESEnc(OpSize::i128Bit, Src1, Src2, Src3); });
}

void OpDispatchBuilder::AVX128_VAESEncLast(OpcodeArgs) {
  AVX128_VectorTrinaryImpl(Op, OpSizeFromDst(Op), OpSize::i128Bit, LoadZeroVector(OpSize::i128Bit),
                           [this](IR::OpSize, Ref Src1, Ref Src2, Ref Src3) { return _VAESEncLast(OpSize::i128Bit, Src1, Src2, Src3); });
}

void OpDispatchBuilder::AVX128_VAESDec(OpcodeArgs) {
  AVX128_VectorTrinaryImpl(Op, OpSizeFromDst(Op), OpSize::i128Bit, LoadZeroVector(OpSize::i128Bit),
                           [this](IR::OpSize, Ref Src1, Ref Src2, Ref Src3) { return _VAESDec(OpSize::i128Bit, Src1, Src2, Src3); });
}

void OpDispatchBuilder::AVX128_VAESDecLast(OpcodeArgs) {
  AVX128_VectorTrinaryImpl(Op, OpSizeFromDst(Op), OpSize::i128Bit, LoadZeroVector(OpSize::i128Bit),
                           [this](IR::OpSize, Ref Src1, Ref Src2, Ref Src3) { return _VAESDecLast(OpSize::i128Bit, Src1, Src2, Src3); });
}

void OpDispatchBuilder::AVX128_VAESKeyGenAssist(OpcodeArgs) {
  ///< 128-bit only.
  const uint64_t RCON = Op->Src[1].Literal();
  auto ZeroRegister = LoadZeroVector(OpSize::i128Bit);
  auto KeyGenSwizzle = LoadAndCacheNamedVectorConstant(OpSize::i128Bit, NAMED_VECTOR_AESKEYGENASSIST_SWIZZLE);

  struct {
    Ref ZeroRegister;
    Ref KeyGenSwizzle;
    uint64_t RCON;
  } Capture {
    .ZeroRegister = ZeroRegister,
    .KeyGenSwizzle = KeyGenSwizzle,
    .RCON = RCON,
  };

  AVX128_VectorUnaryImpl(Op, OpSize::i128Bit, OpSize::i128Bit, [this, &Capture](IR::OpSize, Ref Src) {
    return _VAESKeyGenAssist(Src, Capture.KeyGenSwizzle, Capture.ZeroRegister, Capture.RCON);
  });
}

void OpDispatchBuilder::AVX128_VPCMPESTRI(OpcodeArgs) {
  PCMPXSTRXOpImpl(Op, true, false);

  ///< Does not zero anything.
}

void OpDispatchBuilder::AVX128_VPCMPESTRM(OpcodeArgs) {
  PCMPXSTRXOpImpl(Op, true, true);

  ///< Zero the upper 128-bits of hardcoded YMM0
  AVX128_StoreXMMRegister(0, LoadZeroVector(OpSize::i128Bit), true);
}

void OpDispatchBuilder::AVX128_VPCMPISTRI(OpcodeArgs) {
  PCMPXSTRXOpImpl(Op, false, false);

  ///< Does not zero anything.
}

void OpDispatchBuilder::AVX128_VPCMPISTRM(OpcodeArgs) {
  PCMPXSTRXOpImpl(Op, false, true);

  ///< Zero the upper 128-bits of hardcoded YMM0
  AVX128_StoreXMMRegister(0, LoadZeroVector(OpSize::i128Bit), true);
}

void OpDispatchBuilder::AVX128_PHMINPOSUW(OpcodeArgs) {
  Ref Result = PHMINPOSUWOpImpl(Op);
  AVX128_StoreResult_WithOpSize(Op, Op->Dest, AVX128_Zext(Result));
}

void OpDispatchBuilder::AVX128_VectorRound(OpcodeArgs, IR::OpSize ElementSize) {
  const auto Size = OpSizeFromSrc(Op);
  const auto Mode = Op->Src[1].Literal();

  AVX128_VectorUnaryImpl(Op, Size, ElementSize,
                         [this, Mode](IR::OpSize ElementSize, Ref Src) { return VectorRoundImpl(OpSize::i128Bit, ElementSize, Src, Mode); });
}

void OpDispatchBuilder::AVX128_InsertScalarRound(OpcodeArgs, IR::OpSize ElementSize) {
  // We load the full vector width when dealing with a source vector,
  // so that we don't do any unnecessary zero extension to the scalar
  // element that we're going to operate on.
  const auto SrcSize = OpSizeFromSrc(Op);

  auto Src1 = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, false);
  RefPair Src2 {};
  if (Op->Src[1].IsGPR()) {
    Src2 = AVX128_LoadSource_WithOpSize(Op, Op->Src[1], Op->Flags, false);
  } else {
    Src2.Low = LoadSourceFPR_WithOpSize(Op, Op->Src[1], SrcSize, Op->Flags);
  }

  // If OpSize == ElementSize then it only does the lower scalar op
  const auto SourceMode = TranslateRoundType(Op->Src[2].Literal());

  Ref Result = _VFToIScalarInsert(OpSize::i128Bit, ElementSize, Src1.Low, Src2.Low, SourceMode, false);
  AVX128_StoreResult_WithOpSize(Op, Op->Dest, AVX128_Zext(Result));
}

void OpDispatchBuilder::AVX128_VDPP(OpcodeArgs, IR::OpSize ElementSize) {
  const uint64_t Literal = Op->Src[2].Literal();

  AVX128_VectorBinaryImpl(Op, OpSizeFromSrc(Op), ElementSize, [this, Literal](IR::OpSize ElementSize, Ref Src1, Ref Src2) {
    return DPPOpImpl(OpSize::i128Bit, Src1, Src2, Literal, ElementSize);
  });
}

void OpDispatchBuilder::AVX128_VPERMQ(OpcodeArgs) {
  ///< Only ever 256-bit.
  auto Src = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, true);
  const auto Selector = Op->Src[1].Literal();

  RefPair Result {};

  // Crack the operation in to two halves and implement per half
  uint8_t SelectorLow = Selector & 0b1111;
  uint8_t SelectorHigh = (Selector >> 4) & 0b1111;
  auto SelectLane = [this](uint8_t Selector, RefPair Src) -> Ref {
    LOGMAN_THROW_A_FMT(Selector < 16, "Selector too large!");

    switch (Selector) {
    case 0b00'00: return _VDupElement(OpSize::i128Bit, OpSize::i64Bit, Src.Low, 0);
    case 0b00'01: return _VExtr(OpSize::i128Bit, OpSize::i8Bit, Src.Low, Src.Low, 8);
    case 0b00'10: return _VZip(OpSize::i128Bit, OpSize::i64Bit, Src.High, Src.Low);
    case 0b00'11: return _VExtr(OpSize::i128Bit, OpSize::i8Bit, Src.Low, Src.High, 8);
    case 0b01'00: return Src.Low;
    case 0b01'01: return _VDupElement(OpSize::i128Bit, OpSize::i64Bit, Src.Low, 1);
    case 0b01'10: return _VInsElement(OpSize::i128Bit, OpSize::i64Bit, 1, 1, Src.High, Src.Low);
    case 0b01'11: return _VTrn2(OpSize::i128Bit, OpSize::i64Bit, Src.High, Src.Low);
    case 0b10'00: return _VZip(OpSize::i128Bit, OpSize::i64Bit, Src.Low, Src.High);
    case 0b10'01: return _VExtr(OpSize::i128Bit, OpSize::i8Bit, Src.High, Src.Low, 8);
    case 0b10'10: return _VDupElement(OpSize::i128Bit, OpSize::i64Bit, Src.High, 0);
    case 0b10'11: return _VExtr(OpSize::i128Bit, OpSize::i8Bit, Src.High, Src.High, 8);
    case 0b11'00: return _VInsElement(OpSize::i128Bit, OpSize::i64Bit, 1, 1, Src.Low, Src.High);
    case 0b11'01: return _VTrn2(OpSize::i128Bit, OpSize::i64Bit, Src.Low, Src.High);
    case 0b11'10: return Src.High;
    case 0b11'11: return _VDupElement(OpSize::i128Bit, OpSize::i64Bit, Src.High, 1);
    default: FEX_UNREACHABLE;
    }
  };

  Result.Low = SelectLane(SelectorLow, Src);
  Result.High = SelectorLow == SelectorHigh ? Result.Low : SelectLane(SelectorHigh, Src);

  AVX128_StoreResult_WithOpSize(Op, Op->Dest, Result);
}

void OpDispatchBuilder::AVX128_VPSHUFW(OpcodeArgs, bool Low) {
  auto Shuffle = Op->Src[1].Literal();

  struct DataPacking {
    OpDispatchBuilder* This;
    uint8_t Shuffle;
    bool Low;
  };

  DataPacking Pack {
    .This = this,
    .Shuffle = static_cast<uint8_t>(Shuffle),
    .Low = Low,
  };

  AVX128_VectorUnaryImpl(Op, OpSizeFromSrc(Op), OpSize::i16Bit, [Pack](IR::OpSize, Ref Src) {
    const auto IndexedVectorConstant = Pack.Low ? FEXCore::IR::IndexNamedVectorConstant::INDEXED_NAMED_VECTOR_PSHUFLW :
                                                  FEXCore::IR::IndexNamedVectorConstant::INDEXED_NAMED_VECTOR_PSHUFHW;

    return Pack.This->PShufWLane(OpSize::i128Bit, IndexedVectorConstant, Pack.Low, Src, Pack.Shuffle);
  });
}

void OpDispatchBuilder::AVX128_VSHUF(OpcodeArgs, IR::OpSize ElementSize) {
  const auto SrcSize = GetSrcSize(Op);
  const auto Is128Bit = SrcSize == Core::CPUState::XMM_SSE_REG_SIZE;
  auto Shuffle = Op->Src[2].Literal();

  auto Src1 = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, !Is128Bit);
  auto Src2 = AVX128_LoadSource_WithOpSize(Op, Op->Src[1], Op->Flags, !Is128Bit);

  RefPair Result {};
  Result.Low = SHUFOpImpl(Op, OpSize::i128Bit, ElementSize, Src1.Low, Src2.Low, Shuffle);

  if (Is128Bit) {
    Result.High = LoadZeroVector(OpSize::i128Bit);
  } else {
    const uint8_t ShiftAmount = ElementSize == OpSize::i32Bit ? 0 : 2;
    Result.High = SHUFOpImpl(Op, OpSize::i128Bit, ElementSize, Src1.High, Src2.High, Shuffle >> ShiftAmount);
  }
  AVX128_StoreResult_WithOpSize(Op, Op->Dest, Result);
}

void OpDispatchBuilder::AVX128_VPERMILImm(OpcodeArgs, IR::OpSize ElementSize) {
  const auto SrcSize = GetSrcSize(Op);
  const auto Is128Bit = SrcSize == Core::CPUState::XMM_SSE_REG_SIZE;

  const auto Selector = Op->Src[1].Literal() & 0xFF;
  auto Src = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, !Is128Bit);

  RefPair Result = AVX128_Zext(LoadZeroVector(OpSize::i128Bit));

  if (ElementSize == OpSize::i64Bit) {
    auto DoSwizzle64 = [this](Ref Src, uint8_t Selector) -> Ref {
      switch (Selector) {
      case 0b00:
      case 0b11: return _VDupElement(OpSize::i128Bit, OpSize::i64Bit, Src, Selector & 1);
      case 0b01: return _VExtr(OpSize::i128Bit, OpSize::i8Bit, Src, Src, 8);
      case 0b10:
        // No swizzle
        return Src;
      default: FEX_UNREACHABLE;
      }
    };
    Result.Low = DoSwizzle64(Src.Low, Selector & 0b11);

    if (!Is128Bit) {
      Result.High = DoSwizzle64(Src.High, (Selector >> 2) & 0b11);
    }
  } else {
    Result.Low = Single128Bit4ByteVectorShuffle(Src.Low, Selector);

    if (!Is128Bit) {
      Result.High = Single128Bit4ByteVectorShuffle(Src.High, Selector);
    }
  }

  AVX128_StoreResult_WithOpSize(Op, Op->Dest, Result);
}

void OpDispatchBuilder::AVX128_VHADDP(OpcodeArgs, IROps IROp, IR::OpSize ElementSize) {
  AVX128_VectorBinaryImpl(Op, OpSizeFromSrc(Op), ElementSize, [&](IR::OpSize ElementSize, Ref Src1, Ref Src2) {
    DeriveOp(Res, IROp, _VFAddP(OpSize::i128Bit, ElementSize, Src1, Src2));
    return Res;
  });
}

void OpDispatchBuilder::AVX128_VPHADDSW(OpcodeArgs) {
  AVX128_VectorBinaryImpl(Op, OpSizeFromDst(Op), OpSize::i16Bit,
                          [this](IR::OpSize _ElementSize, Ref Src1, Ref Src2) { return PHADDSOpImpl(OpSize::i128Bit, Src1, Src2); });
}

void OpDispatchBuilder::AVX128_VPMADDUBSW(OpcodeArgs) {
  AVX128_VectorBinaryImpl(Op, OpSizeFromDst(Op), OpSize::i128Bit,
                          [this](IR::OpSize _ElementSize, Ref Src1, Ref Src2) { return PMADDUBSWOpImpl(OpSize::i128Bit, Src1, Src2); });
}

void OpDispatchBuilder::AVX128_VPMADDWD(OpcodeArgs) {
  AVX128_VectorBinaryImpl(Op, OpSizeFromDst(Op), OpSize::i128Bit,
                          [this](IR::OpSize _ElementSize, Ref Src1, Ref Src2) { return PMADDWDOpImpl(OpSize::i128Bit, Src1, Src2); });
}

void OpDispatchBuilder::AVX128_VBLEND(OpcodeArgs, IR::OpSize ElementSize) {
  const auto SrcSize = OpSizeFromSrc(Op);
  const auto Is128Bit = SrcSize == OpSize::i128Bit;
  const uint64_t Selector = Op->Src[2].Literal();

  ///< High Selector shift depends on element size:
  /// i16Bit: Reuses same bits, no shift
  /// i32Bit: Shift by 4
  /// i64Bit: Shift by 2
  const uint64_t SelectorShift = ElementSize == OpSize::i64Bit ? 2 : ElementSize == OpSize::i32Bit ? 4 : 0;

  auto Src1 = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, !Is128Bit);
  auto Src2 = AVX128_LoadSource_WithOpSize(Op, Op->Src[1], Op->Flags, !Is128Bit);

  RefPair Result {};
  Result.Low = VectorBlend(OpSize::i128Bit, ElementSize, Src1.Low, Src2.Low, Selector);

  if (Is128Bit) {
    Result = AVX128_Zext(Result.Low);
  } else {
    Result.High = VectorBlend(OpSize::i128Bit, ElementSize, Src1.High, Src2.High, (Selector >> SelectorShift));
  }

  AVX128_StoreResult_WithOpSize(Op, Op->Dest, Result);
}

void OpDispatchBuilder::AVX128_VHSUBP(OpcodeArgs, IR::OpSize ElementSize) {
  AVX128_VectorBinaryImpl(Op, OpSizeFromDst(Op), ElementSize,
                          [&](IR::OpSize, Ref Src1, Ref Src2) { return HSUBPOpImpl(OpSize::i128Bit, ElementSize, Src1, Src2); });
}

void OpDispatchBuilder::AVX128_VPSHUFB(OpcodeArgs) {
  auto MaskVector = GeneratePSHUFBMask(OpSize::i128Bit);
  AVX128_VectorBinaryImpl(Op, OpSizeFromDst(Op), OpSize::i8Bit, [this, MaskVector](IR::OpSize, Ref Src1, Ref Src2) {
    return PSHUFBOpImpl(OpSize::i128Bit, Src1, Src2, MaskVector);
  });
}

void OpDispatchBuilder::AVX128_VPSADBW(OpcodeArgs) {
  AVX128_VectorBinaryImpl(Op, OpSizeFromDst(Op), OpSize::i8Bit,
                          [this](IR::OpSize, Ref Src1, Ref Src2) { return PSADBWOpImpl(OpSize::i128Bit, Src1, Src2); });
}

void OpDispatchBuilder::AVX128_VMPSADBW(OpcodeArgs) {
  const auto SrcSize = GetSrcSize(Op);
  const auto Is128Bit = SrcSize == Core::CPUState::XMM_SSE_REG_SIZE;
  const uint64_t Selector = Op->Src[2].Literal();

  auto Src1 = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, !Is128Bit);
  auto Src2 = AVX128_LoadSource_WithOpSize(Op, Op->Src[1], Op->Flags, !Is128Bit);

  RefPair Result {};
  auto ZeroRegister = LoadZeroVector(OpSize::i128Bit);

  Result.Low = MPSADBWOpImpl(OpSize::i128Bit, Src1.Low, Src2.Low, Selector);

  if (Is128Bit) {
    Result.High = ZeroRegister;
  } else {
    Result.High = MPSADBWOpImpl(OpSize::i128Bit, Src1.High, Src2.High, Selector >> 3);
  }

  AVX128_StoreResult_WithOpSize(Op, Op->Dest, Result);
}

void OpDispatchBuilder::AVX128_VPALIGNR(OpcodeArgs) {
  const auto Index = Op->Src[2].Literal();
  const auto Size = OpSizeFromDst(Op);
  const auto SanitizedDstSize = std::min(Size, OpSize::i128Bit);

  AVX128_VectorBinaryImpl(Op, Size, SanitizedDstSize, [this, Index](IR::OpSize SanitizedDstSize, Ref Src1, Ref Src2) -> Ref {
    if (Index >= (IR::OpSizeToSize(SanitizedDstSize) * 2)) {
      // If the immediate is greater than both vectors combined then it zeroes the vector
      return LoadZeroVector(OpSize::i128Bit);
    }

    if (Index == 0) {
      return Src2;
    }

    if (Index == 16) {
      return Src1;
    }

    auto SanitizedIndex = Index;
    if (Index > 16) {
      Src2 = Src1;
      Src1 = LoadZeroVector(OpSize::i128Bit);
      SanitizedIndex -= 16;
    }

    return _VExtr(OpSize::i128Bit, OpSize::i8Bit, Src1, Src2, SanitizedIndex);
  });
}

void OpDispatchBuilder::AVX128_VMASKMOVImpl(OpcodeArgs, IR::OpSize ElementSize, IR::OpSize DstSize, bool IsStore,
                                            const X86Tables::DecodedOperand& MaskOp, const X86Tables::DecodedOperand& DataOp) {
  const auto Is128Bit = DstSize == OpSize::i128Bit;

  auto Mask = AVX128_LoadSource_WithOpSize(Op, MaskOp, Op->Flags, !Is128Bit);

  const auto MakeAddress = [this, Op](const X86Tables::DecodedOperand& Data) {
    return MakeSegmentAddress(Op, Data, GetGPROpSize());
  };

  if (IsStore) {
    auto Address = MakeAddress(Op->Dest);

    auto Data = AVX128_LoadSource_WithOpSize(Op, DataOp, Op->Flags, !Is128Bit);
    _VStoreVectorMasked(OpSize::i128Bit, ElementSize, Mask.Low, Data.Low, Address, Invalid(), MemOffsetType::SXTX, 1);
    if (!Is128Bit) {
      _VStoreVectorMasked(OpSize::i128Bit, ElementSize, Mask.High, Data.High, Address, _InlineConstant(16), MemOffsetType::SXTX, 1);
    }
  } else {
    auto Address = MakeAddress(DataOp);

    RefPair Result {};
    Result.Low = _VLoadVectorMasked(OpSize::i128Bit, ElementSize, Mask.Low, Address, Invalid(), MemOffsetType::SXTX, 1);

    if (Is128Bit) {
      Result.High = LoadZeroVector(OpSize::i128Bit);
    } else {
      Result.High = _VLoadVectorMasked(OpSize::i128Bit, ElementSize, Mask.High, Address, _InlineConstant(16), MemOffsetType::SXTX, 1);
    }
    AVX128_StoreResult_WithOpSize(Op, Op->Dest, Result);
  }
}

void OpDispatchBuilder::AVX128_VPMASKMOV(OpcodeArgs, bool IsStore) {
  AVX128_VMASKMOVImpl(Op, OpSizeFromSrc(Op), OpSizeFromDst(Op), IsStore, Op->Src[0], Op->Src[1]);
}

void OpDispatchBuilder::AVX128_VMASKMOV(OpcodeArgs, IR::OpSize ElementSize, bool IsStore) {
  AVX128_VMASKMOVImpl(Op, ElementSize, OpSizeFromDst(Op), IsStore, Op->Src[0], Op->Src[1]);
}

void OpDispatchBuilder::AVX128_MASKMOV(OpcodeArgs) {
  ///< This instruction only supports 128-bit.
  const auto Size = OpSizeFromSrc(Op);
  const auto Is128Bit = Size == OpSize::i128Bit;

  auto MaskSrc = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, !Is128Bit);

  // Mask only cares about the top bit of each byte
  MaskSrc.Low = _VCMPLTZ(Size, OpSize::i8Bit, MaskSrc.Low);

  // Vector that will overwrite byte elements.
  auto VectorSrc = AVX128_LoadSource_WithOpSize(Op, Op->Dest, Op->Flags, !Is128Bit);

  // RDI source (DS prefix by default)
  auto MemDest = MakeSegmentAddress(X86State::REG_RDI, Op->Flags, X86Tables::DecodeFlags::FLAG_DS_PREFIX);

  Ref XMMReg = _LoadMemFPR(Size, MemDest, OpSize::i8Bit);

  // If the Mask element high bit is set then overwrite the element with the source, else keep the memory variant
  XMMReg = _VBSL(Size, MaskSrc.Low, VectorSrc.Low, XMMReg);
  _StoreMemFPR(Size, MemDest, XMMReg, OpSize::i8Bit);
}

void OpDispatchBuilder::AVX128_VectorVariableBlend(OpcodeArgs, IR::OpSize ElementSize) {
  const auto Size = OpSizeFromSrc(Op);
  const auto Is128Bit = Size == OpSize::i128Bit;
  const auto Src3Selector = Op->Src[2].Literal();

  auto Src1 = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, !Is128Bit);
  auto Src2 = AVX128_LoadSource_WithOpSize(Op, Op->Src[1], Op->Flags, !Is128Bit);

  uint8_t MaskRegister = (Src3Selector >> 4) & 0b1111;
  RefPair Mask {.Low = AVX128_LoadXMMRegister(MaskRegister, false)};

  if (!Is128Bit) {
    Mask.High = AVX128_LoadXMMRegister(MaskRegister, true);
  }

  auto Convert = [&](Ref Src1, Ref Src2, Ref Mask) {
    const auto ElementSizeBits = IR::OpSizeAsBits(ElementSize);
    Ref Shifted = _VSShrI(OpSize::i128Bit, ElementSize, Mask, ElementSizeBits - 1);
    return _VBSL(OpSize::i128Bit, Shifted, Src2, Src1);
  };

  RefPair Result {};
  Result.Low = Convert(Src1.Low, Src2.Low, Mask.Low);
  if (!Is128Bit) {
    Result.High = Convert(Src1.High, Src2.High, Mask.High);
  } else {
    Result = AVX128_Zext(Result.Low);
  }

  AVX128_StoreResult_WithOpSize(Op, Op->Dest, Result);
}

void OpDispatchBuilder::AVX128_SaveAVXState(Ref MemBase) {
  const auto NumRegs = Is64BitMode ? 16U : 8U;

  for (uint32_t i = 0; i < NumRegs; i += 2) {
    RefPair Pair = LoadContextPair(OpSize::i128Bit, AVXHigh0Index + i);
    _StoreMemPairFPR(OpSize::i128Bit, Pair.Low, Pair.High, MemBase, i * 16 + 576);
  }
}

void OpDispatchBuilder::AVX128_RestoreAVXState(Ref MemBase) {
  const auto NumRegs = Is64BitMode ? 16U : 8U;

  for (uint32_t i = 0; i < NumRegs; i += 2) {
    auto YMMHRegs = LoadMemPairFPR(OpSize::i128Bit, MemBase, i * 16 + 576);

    AVX128_StoreXMMRegister(i, YMMHRegs.Low, true);
    AVX128_StoreXMMRegister(i + 1, YMMHRegs.High, true);
  }
}

void OpDispatchBuilder::AVX128_DefaultAVXState() {
  const auto NumRegs = Is64BitMode ? 16U : 8U;

  auto ZeroRegister = LoadZeroVector(OpSize::i128Bit);
  for (uint32_t i = 0; i < NumRegs; i++) {
    AVX128_StoreXMMRegister(i, ZeroRegister, true);
  }
}

void OpDispatchBuilder::AVX128_VPERM2(OpcodeArgs) {
  auto Src1 = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, true);
  auto Src2 = AVX128_LoadSource_WithOpSize(Op, Op->Src[1], Op->Flags, true);
  const auto Selector = Op->Src[2].Literal();

  RefPair Result = AVX128_Zext(LoadZeroVector(OpSize::i128Bit));
  Ref Elements[4] = {Src1.Low, Src1.High, Src2.Low, Src2.High};

  if ((Selector & 0b00001000) == 0) {
    Result.Low = Elements[Selector & 0b11];
  }

  if ((Selector & 0b10000000) == 0) {
    Result.High = Elements[(Selector >> 4) & 0b11];
  }

  AVX128_StoreResult_WithOpSize(Op, Op->Dest, Result);
}

void OpDispatchBuilder::AVX128_VTESTP(OpcodeArgs, IR::OpSize ElementSize) {
  const auto Size = GetSrcSize(Op);
  const auto Is128Bit = Size == Core::CPUState::XMM_SSE_REG_SIZE;

  auto Src1 = AVX128_LoadSource_WithOpSize(Op, Op->Dest, Op->Flags, !Is128Bit);
  auto Src2 = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, !Is128Bit);

  // For 128-bit, we use the common path.
  if (Is128Bit) {
    VTESTOpImpl(OpSize::i128Bit, ElementSize, Src1.Low, Src2.Low);
    return;
  }

  // For 256-bit, we need to split up the operation. This is nontrivial.
  // Let's go the simple route here.
  Ref ZF, CFInv;

  const auto ElementSizeInBits = IR::OpSizeAsBits(ElementSize);

  {
    // Calculate ZF first.
    auto AndLow = _VAnd(OpSize::i128Bit, OpSize::i8Bit, Src2.Low, Src1.Low);
    auto AndHigh = _VAnd(OpSize::i128Bit, OpSize::i8Bit, Src2.High, Src1.High);

    auto ShiftLow = _VUShrI(OpSize::i128Bit, ElementSize, AndLow, ElementSizeInBits - 1);
    auto ShiftHigh = _VUShrI(OpSize::i128Bit, ElementSize, AndHigh, ElementSizeInBits - 1);
    // Only have the signs now, add it all
    auto AddResult = _VAdd(OpSize::i128Bit, ElementSize, ShiftHigh, ShiftLow);
    Ref AddWide {};
    if (ElementSize == OpSize::i32Bit) {
      AddWide = _VAddV(OpSize::i128Bit, ElementSize, AddResult);
    } else {
      AddWide = _VAddP(OpSize::i128Bit, ElementSize, AddResult, AddResult);
    }

    // ExtGPR will either be [0, 8] or [0, 16] If 0 then set Flag.
    ZF = _VExtractToGPR(OpSize::i128Bit, ElementSize, AddWide, 0);
  }

  {
    // Calculate CF Second
    auto AndLow = _VAndn(OpSize::i128Bit, OpSize::i8Bit, Src2.Low, Src1.Low);
    auto AndHigh = _VAndn(OpSize::i128Bit, OpSize::i8Bit, Src2.High, Src1.High);

    auto ShiftLow = _VUShrI(OpSize::i128Bit, ElementSize, AndLow, ElementSizeInBits - 1);
    auto ShiftHigh = _VUShrI(OpSize::i128Bit, ElementSize, AndHigh, ElementSizeInBits - 1);
    // Only have the signs now, add it all
    auto AddResult = _VAdd(OpSize::i128Bit, ElementSize, ShiftHigh, ShiftLow);
    Ref AddWide {};
    if (ElementSize == OpSize::i32Bit) {
      AddWide = _VAddV(OpSize::i128Bit, ElementSize, AddResult);
    } else {
      AddWide = _VAddP(OpSize::i128Bit, ElementSize, AddResult, AddResult);
    }

    // ExtGPR will either be [0, 8] or [0, 16] If 0 then set Flag.
    auto ExtGPR = _VExtractToGPR(OpSize::i128Bit, ElementSize, AddWide, 0);
    CFInv = To01(OpSize::i64Bit, ExtGPR);
  }

  // As in PTest, this sets Z appropriately while zeroing the rest of NZCV.
  SetNZ_ZeroCV(OpSize::i32Bit, ZF);
  SetCFInverted(CFInv);
  ZeroPF_AF();
}

void OpDispatchBuilder::AVX128_PTest(OpcodeArgs) {
  const auto Size = GetSrcSize(Op);
  const auto Is128Bit = Size == Core::CPUState::XMM_SSE_REG_SIZE;

  auto Src1 = AVX128_LoadSource_WithOpSize(Op, Op->Dest, Op->Flags, !Is128Bit);
  auto Src2 = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, !Is128Bit);

  // For 128-bit, use the common path.
  if (Is128Bit) {
    PTestOpImpl(OpSize::i128Bit, Src1.Low, Src2.Low);
    return;
  }

  // For 256-bit, we need to unroll. This is nontrivial.
  Ref Test1Low = _VAnd(OpSize::i128Bit, OpSize::i8Bit, Src1.Low, Src2.Low);
  Ref Test2Low = _VAndn(OpSize::i128Bit, OpSize::i8Bit, Src2.Low, Src1.Low);

  Ref Test1High = _VAnd(OpSize::i128Bit, OpSize::i8Bit, Src1.High, Src2.High);
  Ref Test2High = _VAndn(OpSize::i128Bit, OpSize::i8Bit, Src2.High, Src1.High);

  // Element size must be less than 32-bit for the sign bit tricks.
  Ref Test1Max = _VUMax(OpSize::i128Bit, OpSize::i16Bit, Test1Low, Test1High);
  Ref Test2Max = _VUMax(OpSize::i128Bit, OpSize::i16Bit, Test2Low, Test2High);

  Ref Test1 = _VUMaxV(OpSize::i128Bit, OpSize::i16Bit, Test1Max);
  Ref Test2 = _VUMaxV(OpSize::i128Bit, OpSize::i16Bit, Test2Max);

  Test1 = _VExtractToGPR(OpSize::i128Bit, OpSize::i16Bit, Test1, 0);
  Test2 = _VExtractToGPR(OpSize::i128Bit, OpSize::i16Bit, Test2, 0);

  Test2 = To01(OpSize::i64Bit, Test2);

  // Careful, these flags are different between {V,}PTEST and VTESTP{S,D}
  // Set ZF according to Test1. SF will be zeroed since we do a 32-bit test on
  // the results of a 16-bit value from the UMaxV, so the 32-bit sign bit is
  // cleared even if the 16-bit scalars were negative.
  SetNZ_ZeroCV(OpSize::i32Bit, Test1);
  SetCFInverted(Test2);
  ZeroPF_AF();
}

void OpDispatchBuilder::AVX128_VPERMILReg(OpcodeArgs, IR::OpSize ElementSize) {
  AVX128_VectorBinaryImpl(Op, OpSizeFromSrc(Op), ElementSize, [this](IR::OpSize _ElementSize, Ref Src, Ref Indices) {
    return VPERMILRegOpImpl(OpSize::i128Bit, _ElementSize, Src, Indices);
  });
}

void OpDispatchBuilder::AVX128_VPERMD(OpcodeArgs) {
  // Only 256-bit
  auto Indices = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, true);
  auto Src = AVX128_LoadSource_WithOpSize(Op, Op->Src[1], Op->Flags, true);

  auto DoPerm = [this](RefPair Src, Ref Indices, Ref IndexMask, Ref AddVector) {
    Ref FinalIndices = VPERMDIndices(OpSize::i128Bit, Indices, IndexMask, AddVector);
    return _VTBL2(OpSize::i128Bit, Src.Low, Src.High, FinalIndices);
  };

  RefPair Result {};

  Ref IndexMask = _VectorImm(OpSize::i128Bit, OpSize::i32Bit, 0b111);
  Ref AddConst = Constant(0x03020100);
  Ref Repeating3210 = _VDupFromGPR(OpSize::i128Bit, OpSize::i32Bit, AddConst);

  Result.Low = DoPerm(Src, Indices.Low, IndexMask, Repeating3210);
  Result.High = DoPerm(Src, Indices.High, IndexMask, Repeating3210);

  AVX128_StoreResult_WithOpSize(Op, Op->Dest, Result);
}

void OpDispatchBuilder::AVX128_VPCLMULQDQ(OpcodeArgs) {
  if (!CTX->HostFeatures.SupportsPMULL_128Bit) {
    UnimplementedOp(Op);
    return;
  }

  const auto Selector = static_cast<uint8_t>(Op->Src[2].Literal());

  AVX128_VectorBinaryImpl(Op, OpSizeFromSrc(Op), OpSize::iInvalid, [this, Selector](IR::OpSize, Ref Src1, Ref Src2) {
    return _PCLMUL(OpSize::i128Bit, Src1, Src2, Selector & 0b1'0001);
  });
}

// FMA differences between AArch64 and x86 make this really confusing to remember how things match.
// Here's a little guide for remembering how these instructions related across the architectures.
//
///< AArch64 Vector FMA behaviour
// FMLA vd, vn, vm
// - vd = (vn * vm) + vd
// FMLS vd, vn, vm
// - vd = (-vn * vm) + vd
//
// SVE ONLY! No FNMLA or FNMLS variants until SVE!
// FMLA zda, pg/m, zn, zm - Ignore predicate here
// - zda = (zn * zm) + zda
// FMLS zda, pg/m, zn, zm - Ignore predicate here
// - zda = (-zn * zm) + zda
// FNMLA zda, pg/m, zn, zm - Ignore predicate here
// - zda = (-zn * zm) - zda
// FNMLS zda, pg/m, zn, zm - Ignore predicate here
// - zda = (zn * zm) - zda
//
///< AArch64 Scalar FMA behaviour (FMA4 versions!)
// All variants support 16-bit, 32-bit, and 64-bit.
// FMADD d, n, m, a
// - d = (n * m) + a
// FMSUB d, n, m, a
// - d = (-n * m) + a
// FNMADD d, n, m, a
// - d = (-n * m) - a
// FNMSUB d, n, m, a
// - d = (n * m) - a
//
///< x86 FMA behaviour
// ## Packed variants
// - VFMADD{PD,PS}suffix src1, src2, src3/mem
// - 132 - src1 = (src1 * src3) + src2
// - 213 - src1 = (src2 * src1) + src3
// - 231 - src1 = (src2 * src3) + src1
//   ^ Matches ARM FMLA
//
// - VFMSUB{PD,PS}suffix src1, src2, src3/mem
// - 132 - src1 = (src1 * src3) - src2
// - 213 - src1 = (src2 * src1) - src3
// - 231 - src1 = (src2 * src3) - src1
//   ^ Matches ARM FMLA with addend negated first
//   ^ Or just SVE FNMLS
//   ^ or scalar FNMSUB
//
// - VFNMADD{PD,PS}suffix src1, src2, src3/mem
// - 132 - src1 = (-src1 * src3) + src2
// - 213 - src1 = (-src2 * src1) + src3
// - 231 - src1 = (-src2 * src3) + src1
//   ^ Matches ARM FMLS behaviour! (REALLY CONFUSINGLY NAMED!)
//   ^ Or Scalar FMSUB
//
// - VFNMSUB{PD,PS}suffix src1, src2, src3/mem
// - 132 - src1 = (-src1 * src3) - src2
// - 213 - src1 = (-src2 * src1) - src3
// - 231 - src1 = (-src2 * src3) - src1
//   ^ Matches ARM FMLS behaviour with addend negated first! (REALLY CONFUSINGLY NAMED!)
//   ^ Or just SVE FNMLA
//   ^ Or scalar FNMADD
//
// - VFNMADDSUB{PD,PS}suffix src1, src2, src3/mem
// - 132 - src1.odd  = (src1.odd  * src3.odd)  + src2.odd
//       - src1.even = (src1.even * src3.even) - src2.even
// - 213 - src1.odd  = (src2.odd  * src1.odd)  + src3.odd
//       - src1.even = (src2.even * src1.even) - src3.even
// - 231 - src1.odd  = (src2.odd  * src3.odd)  + src1.odd
//       - src1.even = (src2.even * src3.even) - src1.even
//   ^ Matches ARM FMLA behaviour with addend.even negated first!
//
// - VFNMSUBADD{PD,PS}suffix src1, src2, src3/mem
// - 132 - src1.odd  = (src1.odd  * src3.odd)  - src2.odd
//       - src1.even = (src1.even * src3.even) + src2.even
// - 213 - src1.odd  = (src2.odd  * src1.odd)  - src3.odd
//       - src1.even = (src2.even * src1.even) + src3.even
// - 231 - src1.odd  = (src2.odd  * src3.odd)  - src1.odd
//       - src1.even = (src2.even * src3.even) + src1.even
//   ^ Matches ARM FMLA behaviour with addend.odd negated first!
//
// As shown only the 231 suffixed instructions matches AArch64 behaviour.
// FEX will insert moves to transpose the vectors to match AArch64 behaviour for 132 and 213 variants.

void OpDispatchBuilder::AVX128_VFMAImpl(OpcodeArgs, IROps IROp, uint8_t Src1Idx, uint8_t Src2Idx, uint8_t AddendIdx) {
  const auto Size = GetDstSize(Op);
  const auto Is128Bit = Size == Core::CPUState::XMM_SSE_REG_SIZE;

  const OpSize ElementSize = Op->Flags & X86Tables::DecodeFlags::FLAG_OPTION_AVX_W ? OpSize::i64Bit : OpSize::i32Bit;


  auto Dest = AVX128_LoadSource_WithOpSize(Op, Op->Dest, Op->Flags, !Is128Bit);
  auto Src1 = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, !Is128Bit);
  auto Src2 = AVX128_LoadSource_WithOpSize(Op, Op->Src[1], Op->Flags, !Is128Bit);

  RefPair Sources[3] = {Dest, Src1, Src2};

  RefPair Result {};
  DeriveOp(Result_Low, IROp, _VFMLA(OpSize::i128Bit, ElementSize, Sources[Src1Idx - 1].Low, Sources[Src2Idx - 1].Low, Sources[AddendIdx - 1].Low));
  Result.Low = Result_Low;
  if (Is128Bit) {
    Result.High = LoadZeroVector(OpSize::i128Bit);
  } else {
    DeriveOp(Result_High, IROp,
             _VFMLA(OpSize::i128Bit, ElementSize, Sources[Src1Idx - 1].High, Sources[Src2Idx - 1].High, Sources[AddendIdx - 1].High));
    Result.High = Result_High;
  }
  AVX128_StoreResult_WithOpSize(Op, Op->Dest, Result);
}

void OpDispatchBuilder::AVX128_VFMAScalarImpl(OpcodeArgs, IROps IROp, uint8_t Src1Idx, uint8_t Src2Idx, uint8_t AddendIdx) {
  const OpSize ElementSize = Op->Flags & X86Tables::DecodeFlags::FLAG_OPTION_AVX_W ? OpSize::i64Bit : OpSize::i32Bit;

  auto Dest = AVX128_LoadSource_WithOpSize(Op, Op->Dest, Op->Flags, false).Low;
  auto Src1 = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, false).Low;
  Ref Src2 {};
  if (Op->Src[1].IsGPR()) {
    Src2 = AVX128_LoadSource_WithOpSize(Op, Op->Src[1], Op->Flags, false).Low;
  } else {
    Src2 = LoadSourceFPR_WithOpSize(Op, Op->Src[1], ElementSize, Op->Flags);
  }

  Ref Sources[3] = {Dest, Src1, Src2};

  DeriveOp(Result_Low, IROp,
           _VFMLAScalarInsert(OpSize::i128Bit, ElementSize, Dest, Sources[Src1Idx - 1], Sources[Src2Idx - 1], Sources[AddendIdx - 1]));
  AVX128_StoreResult_WithOpSize(Op, Op->Dest, AVX128_Zext(Result_Low));
}

void OpDispatchBuilder::AVX128_VFMAddSubImpl(OpcodeArgs, bool AddSub, uint8_t Src1Idx, uint8_t Src2Idx, uint8_t AddendIdx) {
  const auto Size = GetDstSize(Op);
  const auto Is128Bit = Size == Core::CPUState::XMM_SSE_REG_SIZE;

  const OpSize ElementSize = Op->Flags & X86Tables::DecodeFlags::FLAG_OPTION_AVX_W ? OpSize::i64Bit : OpSize::i32Bit;

  auto Dest = AVX128_LoadSource_WithOpSize(Op, Op->Dest, Op->Flags, !Is128Bit);
  auto Src1 = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, !Is128Bit);
  auto Src2 = AVX128_LoadSource_WithOpSize(Op, Op->Src[1], Op->Flags, !Is128Bit);

  RefPair Sources[3] = {
    Dest,
    Src1,
    Src2,
  };

  RefPair Result {};

  Ref ConstantEOR {};
  if (AddSub) {
    ConstantEOR = LoadAndCacheNamedVectorConstant(
      OpSize::i128Bit, ElementSize == OpSize::i32Bit ? NAMED_VECTOR_PADDSUBPS_INVERT : NAMED_VECTOR_PADDSUBPD_INVERT);
  } else {
    ConstantEOR = LoadAndCacheNamedVectorConstant(
      OpSize::i128Bit, ElementSize == OpSize::i32Bit ? NAMED_VECTOR_PSUBADDPS_INVERT : NAMED_VECTOR_PSUBADDPD_INVERT);
  }
  auto InvertedSourceLow = _VXor(OpSize::i128Bit, ElementSize, Sources[AddendIdx - 1].Low, ConstantEOR);

  Result.Low = _VFMLA(OpSize::i128Bit, ElementSize, Sources[Src1Idx - 1].Low, Sources[Src2Idx - 1].Low, InvertedSourceLow);
  if (Is128Bit) {
    Result.High = LoadZeroVector(OpSize::i128Bit);
  } else {
    auto InvertedSourceHigh = _VXor(OpSize::i128Bit, ElementSize, Sources[AddendIdx - 1].High, ConstantEOR);
    Result.High = _VFMLA(OpSize::i128Bit, ElementSize, Sources[Src1Idx - 1].High, Sources[Src2Idx - 1].High, InvertedSourceHigh);
  }
  AVX128_StoreResult_WithOpSize(Op, Op->Dest, Result);
}

OpDispatchBuilder::RefPair OpDispatchBuilder::AVX128_VPGatherImpl(OpcodeArgs, OpSize Size, OpSize ElementLoadSize, OpSize AddrElementSize,
                                                                  RefPair Dest, RefPair Mask, RefVSIB VSIB) {
  LOGMAN_THROW_A_FMT(AddrElementSize == OpSize::i32Bit || AddrElementSize == OpSize::i64Bit, "Unknown address element size");
  const auto Is128Bit = Size == OpSize::i128Bit;

  ///< BaseAddr doesn't need to exist, calculate that here.
  Ref BaseAddr = VSIB.BaseAddr;
  if (BaseAddr && VSIB.Displacement) {
    BaseAddr = Add(OpSize::i64Bit, BaseAddr, VSIB.Displacement);
  } else if (VSIB.Displacement) {
    BaseAddr = Constant(VSIB.Displacement);
  } else if (!BaseAddr) {
    BaseAddr = Invalid();
  }

  if (CTX->HostFeatures.SupportsSVE128) {
    if (ElementLoadSize == OpSize::i64Bit && AddrElementSize == OpSize::i32Bit) {
      // In the case that FEX is loading double the amount of data than the number of address bits then we can optimize this case.
      // For 256-bits of data we need to sign extend all four 32-bit address elements to be 64-bit.
      // For 128-bits of data we only need to sign extend the lower two 32-bit address elements.
      LOGMAN_THROW_A_FMT(VSIB.High == Invalid(), "Need to not have a high VSIB source");

      if (!Is128Bit) {
        VSIB.High = _VSSHLL2(OpSize::i128Bit, OpSize::i32Bit, VSIB.Low, FEXCore::ilog2(VSIB.Scale));
      }
      VSIB.Low = _VSSHLL(OpSize::i128Bit, OpSize::i32Bit, VSIB.Low, FEXCore::ilog2(VSIB.Scale));

      ///< Set the scale to one now that it has been prescaled as well.
      VSIB.Scale = 1;

      // Set the address element size to 64-bit now that the elements are extended.
      AddrElementSize = OpSize::i64Bit;
    } else if (ElementLoadSize == OpSize::i64Bit && AddrElementSize == OpSize::i64Bit && (VSIB.Scale == 2 || VSIB.Scale == 4)) {
      // SVE gather instructions don't support scaling their vector elements by anything other than 1 or the address element size.
      // Pre-scale 64-bit addresses in the case that scale doesn't match in-order to hit SVE code paths more frequently.
      // Only hit this path if the host supports SVE. Otherwise it's a degradation for the ASIMD codepath.
      VSIB.Low = _VShlI(OpSize::i128Bit, OpSize::i64Bit, VSIB.Low, FEXCore::ilog2(VSIB.Scale));
      if (!Is128Bit) {
        VSIB.High = _VShlI(OpSize::i128Bit, OpSize::i64Bit, VSIB.High, FEXCore::ilog2(VSIB.Scale));
      }
      ///< Set the scale to one now that it has been prescaled.
      VSIB.Scale = 1;
    }
  }

  const auto GPRSize = GetGPROpSize();
  auto AddrSize = (Op->Flags & X86Tables::DecodeFlags::FLAG_ADDRESS_SIZE) != 0 ? (GPRSize >> 1) : GPRSize;

  RefPair Result {};
  ///< Calculate the low-half.
  Result.Low = _VLoadVectorGatherMasked(OpSize::i128Bit, ElementLoadSize, Dest.Low, Mask.Low, BaseAddr, VSIB.Low, VSIB.High,
                                        AddrElementSize, VSIB.Scale, 0, 0, AddrSize);

  if (Is128Bit) {
    Result.High = LoadZeroVector(OpSize::i128Bit);
    if (AddrElementSize == OpSize::i64Bit && ElementLoadSize == OpSize::i32Bit) {
      // Special case for the 128-bit gather load using 64-bit address indexes with 32-bit results.
      // Only loads two 32-bit elements in to the lower 64-bits of the first destination.
      // Bits [255:65] all become zero.
      Result.Low = _VZip(OpSize::i128Bit, OpSize::i64Bit, Result.Low, Result.High);
    }
  } else {
    RefPair AddrAddressing {};

    Ref DestReg = Dest.High;
    Ref MaskReg = Mask.High;
    uint8_t IndexElementOffset {};
    uint8_t DataElementOffset {};
    if (AddrElementSize == ElementLoadSize) {
      // If the address size matches the loading element size then it will be fetching at the same rate between low and high
      AddrAddressing.Low = VSIB.High;
      AddrAddressing.High = Invalid();
    } else if (AddrElementSize == OpSize::i32Bit && ElementLoadSize == OpSize::i64Bit) {
      // If the address element size if half the size of the Element load size then we need to start fetching half-way through the low register.
      AddrAddressing.Low = VSIB.Low;
      AddrAddressing.High = VSIB.High;
      IndexElementOffset = IR::NumElements(OpSize::i128Bit, AddrElementSize) / 2;
    } else if (AddrElementSize == OpSize::i64Bit && ElementLoadSize == OpSize::i32Bit) {
      AddrAddressing.Low = VSIB.High;
      AddrAddressing.High = Invalid();
      DestReg = Result.Low; ///< Start mixing with the low register.
      MaskReg = Mask.Low;   ///< Mask starts with the low mask here.
      IndexElementOffset = 0;
      DataElementOffset = IR::NumElements(OpSize::i128Bit, ElementLoadSize) / 2;
    }

    ///< Calculate the high-half.
    auto ResultHigh = _VLoadVectorGatherMasked(OpSize::i128Bit, ElementLoadSize, DestReg, MaskReg, BaseAddr, AddrAddressing.Low,
                                               AddrAddressing.High, AddrElementSize, VSIB.Scale, DataElementOffset, IndexElementOffset, AddrSize);

    if (AddrElementSize == OpSize::i64Bit && ElementLoadSize == OpSize::i32Bit) {
      // If we only fetched 128-bits worth of data then the upper-result is all zero.
      Result = AVX128_Zext(ResultHigh);
    } else {
      Result.High = ResultHigh;
    }
  }

  return Result;
}

OpDispatchBuilder::RefPair OpDispatchBuilder::AVX128_VPGatherQPSImpl(OpcodeArgs, Ref Dest, Ref Mask, RefVSIB VSIB) {

  ///< BaseAddr doesn't need to exist, calculate that here.
  Ref BaseAddr = VSIB.BaseAddr;
  if (BaseAddr && VSIB.Displacement) {
    BaseAddr = Add(OpSize::i64Bit, BaseAddr, VSIB.Displacement);
  } else if (VSIB.Displacement) {
    BaseAddr = Constant(VSIB.Displacement);
  } else if (!BaseAddr) {
    BaseAddr = Invalid();
  }

  bool NeedsSVEScale = (VSIB.Scale == 2 || VSIB.Scale == 8) || (BaseAddr == Invalid() && VSIB.Scale != 1);

  if (CTX->HostFeatures.SupportsSVE128 && NeedsSVEScale) {
    // SVE gather instructions don't support scaling their vector elements by anything other than 1 or the address element size.
    // Pre-scale 64-bit addresses in the case that scale doesn't match in-order to hit SVE code paths more frequently.
    // Only hit this path if the host supports SVE. Otherwise it's a degradation for the ASIMD codepath.
    VSIB.Low = _VShlI(OpSize::i128Bit, OpSize::i64Bit, VSIB.Low, FEXCore::ilog2(VSIB.Scale));
    if (VSIB.High != Invalid()) {
      VSIB.High = _VShlI(OpSize::i128Bit, OpSize::i64Bit, VSIB.High, FEXCore::ilog2(VSIB.Scale));
    }
    ///< Set the scale to one now that it has been prescaled.
    VSIB.Scale = 1;
  }

  RefPair Result {};

  const auto GPRSize = GetGPROpSize();
  auto AddrSize = (Op->Flags & X86Tables::DecodeFlags::FLAG_ADDRESS_SIZE) != 0 ? (GPRSize >> 1) : GPRSize;

  ///< Calculate the low-half.
  Result.Low = _VLoadVectorGatherMaskedQPS(OpSize::i128Bit, OpSize::i32Bit, Dest, Mask, BaseAddr, VSIB.Low, VSIB.High, VSIB.Scale, AddrSize);
  Result.High = LoadZeroVector(OpSize::i128Bit);
  if (VSIB.High == Invalid()) {
    // Special case for only loading two floats.
    // The upper 64-bits of the lower lane also gets zero.
    Result.Low = _VZip(OpSize::i128Bit, OpSize::i64Bit, Result.Low, Result.High);
  }

  return Result;
}

void OpDispatchBuilder::AVX128_VPGATHER(OpcodeArgs, OpSize AddrElementSize) {

  const auto Size = OpSizeFromDst(Op);
  const auto Is128Bit = Size == OpSize::i128Bit;

  ///< Element size is determined by W flag.
  const OpSize ElementLoadSize = Op->Flags & X86Tables::DecodeFlags::FLAG_OPTION_AVX_W ? OpSize::i64Bit : OpSize::i32Bit;

  // We only need the high address register if the number of data elements is more than what the low half can consume.
  // But also the number of address elements is clamped by the destination size as well.
  const size_t NumDataElements = IR::NumElements(Size, ElementLoadSize);
  const size_t NumAddrElementBytes = std::min<size_t>(IR::OpSizeToSize(Size), (NumDataElements * IR::OpSizeToSize(AddrElementSize)));
  const bool NeedsHighAddrBytes = NumAddrElementBytes > IR::OpSizeToSize(OpSize::i128Bit);

  auto Dest = AVX128_LoadSource_WithOpSize(Op, Op->Dest, Op->Flags, !Is128Bit);
  auto VSIB = AVX128_LoadVSIB(Op, Op->Src[0], Op->Flags, NeedsHighAddrBytes);
  auto Mask = AVX128_LoadSource_WithOpSize(Op, Op->Src[1], Op->Flags, !Is128Bit);

  bool NeedsSVEScale = (VSIB.Scale == 2 || VSIB.Scale == 8) || (VSIB.BaseAddr == Invalid() && VSIB.Scale != 1);

  const bool NeedsExplicitSVEPath =
    CTX->HostFeatures.SupportsSVE128 && AddrElementSize == OpSize::i32Bit && ElementLoadSize == OpSize::i32Bit && NeedsSVEScale;

  RefPair Result {};
  if (NeedsExplicitSVEPath) {
    // Special case for VGATHERDPS/VPGATHERDD (32-bit addresses loading 32-bit elements) that can't use the SVE codepath.
    // The problem is due to the scale not matching SVE limitations, we need to prescale the addresses to be 64-bit.
    auto ScaleVSIBHalf = [this](Ref VSIB, Ref BaseAddr, int32_t Displacement, uint8_t Scale) -> RefVSIB {
      RefVSIB Result {};
      Result.High = _VSSHLL2(OpSize::i128Bit, OpSize::i32Bit, VSIB, FEXCore::ilog2(Scale));
      Result.Low = _VSSHLL(OpSize::i128Bit, OpSize::i32Bit, VSIB, FEXCore::ilog2(Scale));

      Result.Displacement = Displacement;
      Result.BaseAddr = BaseAddr;

      ///< Set the scale to one now that it has been prescaled as well.
      Result.Scale = 1;
      return Result;
    };

    RefVSIB VSIBLow = ScaleVSIBHalf(VSIB.Low, VSIB.BaseAddr, VSIB.Displacement, VSIB.Scale);
    RefVSIB VSIBHigh {};

    if (NeedsHighAddrBytes) {
      VSIBHigh = ScaleVSIBHalf(VSIB.High, VSIB.BaseAddr, VSIB.Displacement, VSIB.Scale);
    }

    ///< AddressElementSize is now OpSize::i64Bit
    Result = AVX128_VPGatherQPSImpl(Op, Dest.Low, Mask.Low, VSIBLow);
    if (NeedsHighAddrBytes) {
      auto Res = AVX128_VPGatherQPSImpl(Op, Dest.High, Mask.High, VSIBHigh);
      Result.High = Res.Low;
    }
  } else if (AddrElementSize == OpSize::i64Bit && ElementLoadSize == OpSize::i32Bit) {
    Result = AVX128_VPGatherQPSImpl(Op, Dest.Low, Mask.Low, VSIB);
  } else {
    Result = AVX128_VPGatherImpl(Op, Size, ElementLoadSize, AddrElementSize, Dest, Mask, VSIB);
  }
  AVX128_StoreResult_WithOpSize(Op, Op->Dest, Result);

  ///< Assume non-faulting behaviour and clear the mask register.
  RefPair ZeroPair {};
  ZeroPair.Low = LoadZeroVector(OpSize::i128Bit);
  ZeroPair.High = ZeroPair.Low;
  AVX128_StoreResult_WithOpSize(Op, Op->Src[1], ZeroPair);
}

void OpDispatchBuilder::AVX128_VCVTPH2PS(OpcodeArgs) {
  const auto DstSize = OpSizeFromDst(Op);
  const auto SrcSize = IR::SizeToOpSize(IR::OpSizeToSize(DstSize) / 2);
  const auto Is128BitSrc = SrcSize == OpSize::i128Bit;
  const auto Is128BitDst = DstSize == OpSize::i128Bit;

  RefPair Src {};
  if (Op->Src[0].IsGPR()) {
    Src = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, !Is128BitSrc);
  } else {
    // In the event that a memory operand is used as the source operand,
    // the access width will always be half the size of the destination vector width
    // (i.e. 128-bit vector -> 64-bit mem, 256-bit vector -> 128-bit mem)
    Src.Low = LoadSourceFPR_WithOpSize(Op, Op->Src[0], SrcSize, Op->Flags);
  }

  RefPair Result {};
  Result.Low = _Vector_FToF(OpSize::i128Bit, OpSize::i32Bit, Src.Low, OpSize::i16Bit);

  if (Is128BitSrc) {
    Result.High = _VFCVTL2(OpSize::i128Bit, OpSize::i16Bit, Src.Low);
  }

  if (Is128BitDst) {
    Result = AVX128_Zext(Result.Low);
  }

  AVX128_StoreResult_WithOpSize(Op, Op->Dest, Result);
}

void OpDispatchBuilder::AVX128_VCVTPS2PH(OpcodeArgs) {
  const auto SrcSize = OpSizeFromSrc(Op);
  const auto Is128BitSrc = SrcSize == OpSize::i128Bit;
  const auto StoreSize = Op->Dest.IsGPR() ? OpSize::i128Bit : IR::SizeToOpSize(IR::OpSizeToSize(SrcSize) / 2);

  const auto Imm8 = Op->Src[1].Literal();
  const auto UseMXCSR = (Imm8 & 0b100) != 0;

  auto Src = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, !Is128BitSrc);

  RefPair Result {};

  Ref OldFPCR {};
  if (!UseMXCSR) {
    // No ARM float conversion instructions allow passing in
    // a rounding mode as an immediate. All of them depend on
    // the RM field in the FPCR. And so! We have to do some ugly
    // rounding mode shuffling.
    const auto NewRMode = Imm8 & 0b11;
    OldFPCR = _PushRoundingMode(NewRMode);
  }

  Result.Low = _Vector_FToF(OpSize::i128Bit, OpSize::i16Bit, Src.Low, OpSize::i32Bit);
  if (!Is128BitSrc) {
    Result.Low = _VFCVTN2(OpSize::i128Bit, OpSize::i32Bit, Result.Low, Src.High);
  }

  if (!UseMXCSR) {
    _PopRoundingMode(OldFPCR);
  }

  // We need to eliminate upper junk if we're storing into a register with
  // a 256-bit source (VCVTPS2PH's destination for registers is an XMM).
  if (Op->Src[0].IsGPR() && SrcSize == OpSize::i256Bit) {
    Result = AVX128_Zext(Result.Low);
  }

  if (!Op->Dest.IsGPR()) {
    StoreResultFPR_WithOpSize(Op, Op->Dest, Result.Low, StoreSize);
  } else {
    AVX128_StoreResult_WithOpSize(Op, Op->Dest, Result);
  }
}

} // namespace FEXCore::IR
