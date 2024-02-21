// SPDX-License-Identifier: MIT
/*
$info$
tags: frontend|x86-to-ir, opcodes|dispatcher-implementations
desc: Handles x86/64 flag generation
$end_info$
*/

#include "Interface/Context/Context.h"
#include "Interface/Core/OpcodeDispatcher.h"
#include "Interface/Core/X86Tables/X86Tables.h"

#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/Config/Config.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/IR/IR.h>

#include <array>
#include <cstdint>

namespace FEXCore::IR {
constexpr std::array<uint32_t, 17> FlagOffsets = {
  FEXCore::X86State::RFLAG_CF_RAW_LOC,
  FEXCore::X86State::RFLAG_PF_RAW_LOC,
  FEXCore::X86State::RFLAG_AF_RAW_LOC,
  FEXCore::X86State::RFLAG_ZF_RAW_LOC,
  FEXCore::X86State::RFLAG_SF_RAW_LOC,
  FEXCore::X86State::RFLAG_TF_LOC,
  FEXCore::X86State::RFLAG_IF_LOC,
  FEXCore::X86State::RFLAG_DF_LOC,
  FEXCore::X86State::RFLAG_OF_RAW_LOC,
  FEXCore::X86State::RFLAG_IOPL_LOC,
  FEXCore::X86State::RFLAG_NT_LOC,
  FEXCore::X86State::RFLAG_RF_LOC,
  FEXCore::X86State::RFLAG_VM_LOC,
  FEXCore::X86State::RFLAG_AC_LOC,
  FEXCore::X86State::RFLAG_VIF_LOC,
  FEXCore::X86State::RFLAG_VIP_LOC,
  FEXCore::X86State::RFLAG_ID_LOC,
};

void OpDispatchBuilder::ZeroMultipleFlags(uint32_t FlagsMask) {
  auto ZeroConst = _Constant(0);

  if (ContainsNZCV(FlagsMask)) {
    // NZCV is stored packed together.
    // It's more optimal to zero NZCV with move+bic instead of multiple bics.
    auto NZCVFlagsMask = FlagsMask & FullNZCVMask;
    if (NZCVFlagsMask == FullNZCVMask) {
      ZeroNZCV();
    }
    else {
      const auto IndexMask = NZCVIndexMask(FlagsMask);

      if (std::popcount(NZCVFlagsMask) == 1) {
        // It's more optimal to store only one here.

        for (size_t i = 0; NZCVFlagsMask && i < FlagOffsets.size(); ++i) {
          const auto FlagOffset = FlagOffsets[i];
          const auto FlagMask = 1U << FlagOffset;
          if (!(FlagMask & NZCVFlagsMask)) {
            continue;
          }
          SetRFLAG(ZeroConst, FlagOffset);
          NZCVFlagsMask &= ~(FlagMask);
        }
      }
      else {
        auto IndexMaskConstant = _Constant(IndexMask);
        auto NewNZCV = _Andn(OpSize::i64Bit, GetNZCV(), IndexMaskConstant);
        SetNZCV(NewNZCV);
      }
      // Unset the possibly set bits.
      PossiblySetNZCVBits &= ~IndexMask;
    }

    // Handled NZCV, so remove it from the mask.
    FlagsMask &= ~FullNZCVMask;
  }

  // PF is stored inverted, so invert it when we zero.
  if (FlagsMask & (1u << X86State::RFLAG_PF_RAW_LOC)) {
    SetRFLAG<FEXCore::X86State::RFLAG_PF_RAW_LOC>(_Constant(1));
    FlagsMask &= ~(1u << X86State::RFLAG_PF_RAW_LOC);
  }

  // Handle remaining masks.
  for (size_t i = 0; FlagsMask && i < FlagOffsets.size(); ++i) {
    const auto FlagOffset = FlagOffsets[i];
    const auto FlagMask = 1U << FlagOffset;
    if (!(FlagMask & FlagsMask)) {
      continue;
    }
    SetRFLAG(ZeroConst, FlagOffset);
    FlagsMask &= ~(FlagMask);
  }
}

void OpDispatchBuilder::SetPackedRFLAG(bool Lower8, OrderedNode *Src) {
  size_t NumFlags = FlagOffsets.size();
  if (Lower8) {
    // Calculate flags early.
    // Could use InvalidateDeferredFlags() if we had masked invalidation.
    // This is only a partial overwrite of flags since OF isn't stored here.
    CalculateDeferredFlags();
    NumFlags = 5;
  }
  else {
    // We are overwriting all RFLAGS. Invalidate the deferred flag state.
    InvalidateDeferredFlags();
  }

  for (size_t i = 0; i < NumFlags; ++i) {
    const auto FlagOffset = FlagOffsets[i];

    if (FlagOffset == FEXCore::X86State::RFLAG_AF_RAW_LOC) {
      // AF is in bit 4 architecturally, and we need to store it to bit 4 of our
      // AF register, with garbage in the other bits. The extract is deferred.
      // We also defer a XOR with the result bit, which is implemented as XOR
      // with PF[4]. But the _Bfe below reliably zeros bit 4 of the PF byte, so
      // that will be a no-op and we get the right result.
      //
      // So we write out the whole flags byte to AF without an extract.
      static_assert(FEXCore::X86State::RFLAG_AF_RAW_LOC == 4);
      SetRFLAG(Src, FEXCore::X86State::RFLAG_AF_RAW_LOC);
    } else if (FlagOffset == FEXCore::X86State::RFLAG_PF_RAW_LOC) {
      // PF is stored parity flipped
      OrderedNode *Tmp = _Bfe(OpSize::i32Bit, 1, FlagOffset, Src);
      Tmp = _Xor(OpSize::i32Bit, Tmp, _Constant(1));
      SetRFLAG(Tmp, FlagOffset);
    } else {
      SetRFLAG(Src, FlagOffset, FlagOffset, true);
    }
  }
}

OrderedNode *OpDispatchBuilder::GetPackedRFLAG(uint32_t FlagsMask) {
  // Calculate flags early.
  CalculateDeferredFlags();

  OrderedNode *Original = _Constant(0);

  // SF/ZF and N/Z are together on both arm64 and x86_64, so we special case that.
  bool GetNZ = (FlagsMask & (1 << FEXCore::X86State::RFLAG_SF_RAW_LOC)) &&
               (FlagsMask & (1 << FEXCore::X86State::RFLAG_ZF_RAW_LOC));

  // Handle CF first, since it's at bit 0 and hence doesn't need shift or OR.
  if (FlagsMask & (1 << FEXCore::X86State::RFLAG_CF_RAW_LOC)) {
    static_assert(FEXCore::X86State::RFLAG_CF_RAW_LOC == 0);
    Original = GetRFLAG(FEXCore::X86State::RFLAG_CF_RAW_LOC);
  }

  for (size_t i = 0; i < FlagOffsets.size(); ++i) {
    const auto FlagOffset = FlagOffsets[i];
    if (!((1U << FlagOffset) & FlagsMask)) {
      continue;
    }

    if ((GetNZ && (FlagOffset == FEXCore::X86State::RFLAG_SF_RAW_LOC ||
                   FlagOffset == FEXCore::X86State::RFLAG_ZF_RAW_LOC)) ||
        FlagOffset == FEXCore::X86State::RFLAG_CF_RAW_LOC ||
        FlagOffset == FEXCore::X86State::RFLAG_PF_RAW_LOC) {
      // Already handled
      continue;
    }

    // Note that the Bfi only considers the bottom bit of the flag, the rest of
    // the byte is allowed to be garbage.
    OrderedNode *Flag;
    if (FlagOffset == FEXCore::X86State::RFLAG_AF_RAW_LOC)
      Flag = LoadAF();
    else
      Flag = GetRFLAG(FlagOffset);

    Original = _Orlshl(OpSize::i64Bit, Original, Flag, FlagOffset);
  }

  // Raw PF value needs to have its bottom bit masked out and inverted. The
  // naive sequence is and/eor/orlshl. But we can do the inversion implicitly
  // instead.
  if (FlagsMask & (1 << FEXCore::X86State::RFLAG_PF_RAW_LOC)) {
    // Set every bit except the bottommost.
    auto OnesInvPF = _Or(OpSize::i64Bit, LoadPFRaw(), _Constant(~1ull));

    // Rotate the bottom bit to the appropriate location for PF, so we get
    // something like 111P1111. Then invert that to get 000p0000. Then OR that
    // into the flags. This is 1 A64 instruction :-)
    auto RightRotation = 64 - FEXCore::X86State::RFLAG_PF_RAW_LOC;
    Original = _Ornror(OpSize::i64Bit, Original, OnesInvPF, RightRotation);
  }

  // OR in the SF/ZF flags at the end, allowing the lshr to fold with the OR
  if (GetNZ) {
    static_assert(FEXCore::X86State::RFLAG_SF_RAW_LOC == (FEXCore::X86State::RFLAG_ZF_RAW_LOC + 1));
    auto NZCV = GetNZCV();
    auto NZ = _And(OpSize::i64Bit, NZCV, _Constant(0b11u << 30));
    Original = _Orlshr(OpSize::i64Bit, Original, NZ, 31 - FEXCore::X86State::RFLAG_SF_RAW_LOC);
  }

  // The constant is OR'ed in at the end, to avoid a pointless or xzr, #2.
  if ((1U << X86State::RFLAG_RESERVED_LOC) & FlagsMask)
    Original = _Or(OpSize::i64Bit, Original, _Constant(2));

  return Original;
}

void OpDispatchBuilder::CalculateOF(uint8_t SrcSize, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2, bool Sub) {
  auto OpSize = SrcSize == 8 ? OpSize::i64Bit : OpSize::i32Bit;
  uint64_t SignBit = (SrcSize * 8) - 1;
  OrderedNode *Anded = nullptr;

  // For add, OF is set iff the sources have the same sign but the destination
  // sign differs. If we know a source sign, we can simplify the expression: if
  // source 2 is known to be positive, we set OF if source 1 is positive and
  // source 2 is negative. Similarly if source 2 is known negative.
  //
  // For sub, OF is set iff the sources have differing signs and the destination
  // sign matches the second source. If source 2 is known positive, set iff
  // source 1 negative and source 2 positive.
  uint64_t Const;
  if (IsValueConstant(WrapNode(Src2), &Const)) {
    bool Negative = (Const & (1ull << SignBit)) != 0;

    if (Negative ^ Sub)
      Anded = _Andn(OpSize, Src1, Res);
    else
      Anded = _Andn(OpSize, Res, Src1);
  } else {
    auto XorOp1 = _Xor(OpSize, Src1, Src2);
    auto XorOp2 = _Xor(OpSize, Res, Src1);

    if (Sub)
      Anded = _And(OpSize, XorOp2, XorOp1);
    else
      Anded = _Andn(OpSize, XorOp2, XorOp1);
  }

  SetRFLAG<FEXCore::X86State::RFLAG_OF_RAW_LOC>(Anded, SrcSize * 8 - 1, true);
}

OrderedNode *OpDispatchBuilder::LoadPFRaw() {
  // Read the stored byte. This is the original result (up to 64-bits), it needs
  // parity calculated.
  auto Result = GetRFLAG(FEXCore::X86State::RFLAG_PF_RAW_LOC);

  // Cast the input to a 32-bit FPR. Logically we only need 8-bit, but that would
  // generate unwanted an ubfx instruction. VPopcount will ignore the upper bits anyway.
  auto InputFPR = _VCastFromGPR(4, 4, Result);

  // Calculate the popcount.
  auto Count = _VPopcount(1, 1, InputFPR);
  return _VExtractToGPR(8, 1, Count, 0);
}

OrderedNode *OpDispatchBuilder::LoadAF() {
  // Read the stored value. This is the XOR of the arguments.
  auto AFWord = GetRFLAG(FEXCore::X86State::RFLAG_AF_RAW_LOC);

  // Read the result, stored for PF.
  auto Result = GetRFLAG(FEXCore::X86State::RFLAG_PF_RAW_LOC);

  // What's left is to XOR and extract. This is the deferred part.
  return _Bfe(OpSize::i32Bit, 1, 4, _Xor(OpSize::i32Bit, AFWord, Result));
}

void OpDispatchBuilder::FixupAF() {
  // The caller has set a desired value of AF in AF[4], regardless of the value
  // of PF. We need to fixup AF[4] so that we get the right value when we XOR in
  // PF[4] later. The easiest solution is to XOR by PF[4], since:
  //
  //  (AF[4] ^ PF[4]) ^ PF[4] = AF[4]

  auto PFRaw = GetRFLAG(FEXCore::X86State::RFLAG_PF_RAW_LOC);
  auto AFRaw = GetRFLAG(FEXCore::X86State::RFLAG_AF_RAW_LOC);

  OrderedNode *XorRes = _Xor(OpSize::i32Bit, AFRaw, PFRaw);
  SetRFLAG<FEXCore::X86State::RFLAG_AF_RAW_LOC>(XorRes);
}

void OpDispatchBuilder::CalculatePF(OrderedNode *Res) {
  // Calculation is entirely deferred until load, just store the 8-bit result.
  SetRFLAG<FEXCore::X86State::RFLAG_PF_RAW_LOC>(Res);
}

void OpDispatchBuilder::CalculateAF(OrderedNode *Src1, OrderedNode *Src2) {
  // We only care about bit 4 in the subsequent XOR. If we'll XOR with 0,
  // there's no sense XOR'ing at all. This affects INC.
  uint64_t Const;
  if (IsValueConstant(WrapNode(Src2), &Const) && (Const & (1u << 4)) == 0) {
    SetRFLAG<FEXCore::X86State::RFLAG_AF_RAW_LOC>(Src1);
    return;
  }

  // We store the XOR of the arguments. At read time, we XOR with the
  // appropriate bit of the result (available as the PF flag) and extract the
  // appropriate bit.
  OrderedNode *XorRes = _Xor(OpSize::i32Bit, Src1, Src2);
  SetRFLAG<FEXCore::X86State::RFLAG_AF_RAW_LOC>(XorRes);
}

void OpDispatchBuilder::CalculateDeferredFlags(uint32_t FlagsToCalculateMask) {
  if (CurrentDeferredFlags.Type == FlagsGenerationType::TYPE_NONE) {
    // Nothing to do
    if (NZCVDirty && CachedNZCV)
      _StoreNZCV(CachedNZCV);

    CachedNZCV = nullptr;
    NZCVDirty = false;
    return;
  }

  switch (CurrentDeferredFlags.Type) {
    case FlagsGenerationType::TYPE_ADC:
      CalculateFlags_ADC(
        CurrentDeferredFlags.SrcSize,
        CurrentDeferredFlags.Res,
        CurrentDeferredFlags.Sources.ThreeSource.Src1,
        CurrentDeferredFlags.Sources.ThreeSource.Src2,
        CurrentDeferredFlags.Sources.ThreeSource.Src3);
      break;
    case FlagsGenerationType::TYPE_SBB:
      CalculateFlags_SBB(
        CurrentDeferredFlags.SrcSize,
        CurrentDeferredFlags.Res,
        CurrentDeferredFlags.Sources.ThreeSource.Src1,
        CurrentDeferredFlags.Sources.ThreeSource.Src2,
        CurrentDeferredFlags.Sources.ThreeSource.Src3);
      break;
    case FlagsGenerationType::TYPE_SUB:
      CalculateFlags_SUB(
        CurrentDeferredFlags.SrcSize,
        CurrentDeferredFlags.Sources.TwoSrcImmediate.Src1,
        CurrentDeferredFlags.Sources.TwoSrcImmediate.Src2,
        CurrentDeferredFlags.Sources.TwoSrcImmediate.UpdateCF);
      break;
    case FlagsGenerationType::TYPE_ADD:
      CalculateFlags_ADD(
        CurrentDeferredFlags.SrcSize,
        CurrentDeferredFlags.Res,
        CurrentDeferredFlags.Sources.TwoSrcImmediate.Src1,
        CurrentDeferredFlags.Sources.TwoSrcImmediate.Src2,
        CurrentDeferredFlags.Sources.TwoSrcImmediate.UpdateCF);
      break;
    case FlagsGenerationType::TYPE_MUL:
      CalculateFlags_MUL(
        CurrentDeferredFlags.SrcSize,
        CurrentDeferredFlags.Res,
        CurrentDeferredFlags.Sources.OneSource.Src1);
      break;
    case FlagsGenerationType::TYPE_UMUL:
      CalculateFlags_UMUL(CurrentDeferredFlags.Res);
      break;
    case FlagsGenerationType::TYPE_LOGICAL:
      CalculateFlags_Logical(
        CurrentDeferredFlags.SrcSize,
        CurrentDeferredFlags.Res,
        CurrentDeferredFlags.Sources.TwoSource.Src1,
        CurrentDeferredFlags.Sources.TwoSource.Src2);
      break;
    case FlagsGenerationType::TYPE_LSHL:
      CalculateFlags_ShiftLeft(
        CurrentDeferredFlags.SrcSize,
        CurrentDeferredFlags.Res,
        CurrentDeferredFlags.Sources.TwoSource.Src1,
        CurrentDeferredFlags.Sources.TwoSource.Src2);
      break;
    case FlagsGenerationType::TYPE_LSHLI:
      CalculateFlags_ShiftLeftImmediate(
        CurrentDeferredFlags.SrcSize,
        CurrentDeferredFlags.Res,
        CurrentDeferredFlags.Sources.OneSrcImmediate.Src1,
        CurrentDeferredFlags.Sources.OneSrcImmediate.Imm);
      break;
    case FlagsGenerationType::TYPE_LSHR:
      CalculateFlags_ShiftRight(
        CurrentDeferredFlags.SrcSize,
        CurrentDeferredFlags.Res,
        CurrentDeferredFlags.Sources.TwoSource.Src1,
        CurrentDeferredFlags.Sources.TwoSource.Src2);
      break;
    case FlagsGenerationType::TYPE_LSHRI:
      CalculateFlags_ShiftRightImmediate(
        CurrentDeferredFlags.SrcSize,
        CurrentDeferredFlags.Res,
        CurrentDeferredFlags.Sources.OneSrcImmediate.Src1,
        CurrentDeferredFlags.Sources.OneSrcImmediate.Imm);
      break;
    case FlagsGenerationType::TYPE_LSHRDI:
      CalculateFlags_ShiftRightDoubleImmediate(
        CurrentDeferredFlags.SrcSize,
        CurrentDeferredFlags.Res,
        CurrentDeferredFlags.Sources.OneSrcImmediate.Src1,
        CurrentDeferredFlags.Sources.OneSrcImmediate.Imm);
      break;
    case FlagsGenerationType::TYPE_ASHR:
      CalculateFlags_SignShiftRight(
        CurrentDeferredFlags.SrcSize,
        CurrentDeferredFlags.Res,
        CurrentDeferredFlags.Sources.TwoSource.Src1,
        CurrentDeferredFlags.Sources.TwoSource.Src2);
      break;
    case FlagsGenerationType::TYPE_ASHRI:
      CalculateFlags_SignShiftRightImmediate(
        CurrentDeferredFlags.SrcSize,
        CurrentDeferredFlags.Res,
        CurrentDeferredFlags.Sources.OneSrcImmediate.Src1,
        CurrentDeferredFlags.Sources.OneSrcImmediate.Imm);
      break;
    case FlagsGenerationType::TYPE_ROR:
      CalculateFlags_RotateRight(
        CurrentDeferredFlags.SrcSize,
        CurrentDeferredFlags.Res,
        CurrentDeferredFlags.Sources.TwoSource.Src1,
        CurrentDeferredFlags.Sources.TwoSource.Src2);
      break;
    case FlagsGenerationType::TYPE_RORI:
      CalculateFlags_RotateRightImmediate(
        CurrentDeferredFlags.SrcSize,
        CurrentDeferredFlags.Res,
        CurrentDeferredFlags.Sources.OneSrcImmediate.Src1,
        CurrentDeferredFlags.Sources.OneSrcImmediate.Imm);
      break;
    case FlagsGenerationType::TYPE_ROL:
      CalculateFlags_RotateLeft(
        CurrentDeferredFlags.SrcSize,
        CurrentDeferredFlags.Res,
        CurrentDeferredFlags.Sources.TwoSource.Src1,
        CurrentDeferredFlags.Sources.TwoSource.Src2);
      break;
    case FlagsGenerationType::TYPE_ROLI:
      CalculateFlags_RotateLeftImmediate(
        CurrentDeferredFlags.SrcSize,
        CurrentDeferredFlags.Res,
        CurrentDeferredFlags.Sources.OneSrcImmediate.Src1,
        CurrentDeferredFlags.Sources.OneSrcImmediate.Imm);
      break;
    case FlagsGenerationType::TYPE_BEXTR:
      CalculateFlags_BEXTR(CurrentDeferredFlags.Res);
      break;
    case FlagsGenerationType::TYPE_BLSI:
      CalculateFlags_BLSI(
        CurrentDeferredFlags.SrcSize,
        CurrentDeferredFlags.Res);
      break;
    case FlagsGenerationType::TYPE_BLSMSK:
      CalculateFlags_BLSMSK(
        CurrentDeferredFlags.SrcSize,
        CurrentDeferredFlags.Res,
        CurrentDeferredFlags.Sources.OneSource.Src1);
      break;
    case FlagsGenerationType::TYPE_BLSR:
      CalculateFlags_BLSR(
        CurrentDeferredFlags.SrcSize,
        CurrentDeferredFlags.Res,
        CurrentDeferredFlags.Sources.OneSource.Src1);
      break;
    case FlagsGenerationType::TYPE_POPCOUNT:
      CalculateFlags_POPCOUNT(CurrentDeferredFlags.Res);
      break;
    case FlagsGenerationType::TYPE_BZHI:
      CalculateFlags_BZHI(
        CurrentDeferredFlags.SrcSize,
        CurrentDeferredFlags.Res,
        CurrentDeferredFlags.Sources.OneSource.Src1);
      break;
    case FlagsGenerationType::TYPE_ZCNT:
      CalculateFlags_ZCNT(
        CurrentDeferredFlags.SrcSize,
        CurrentDeferredFlags.Res);
      break;
    case FlagsGenerationType::TYPE_RDRAND:
      CalculateFlags_RDRAND(CurrentDeferredFlags.Res);
      break;
    case FlagsGenerationType::TYPE_NONE:
    default: ERROR_AND_DIE_FMT("Unhandled flags type {}", CurrentDeferredFlags.Type);
  }

  // Done calculating
  CurrentDeferredFlags.Type = FlagsGenerationType::TYPE_NONE;

  if (NZCVDirty && CachedNZCV)
    _StoreNZCV(CachedNZCV);

  CachedNZCV = nullptr;
  NZCVDirty = false;
}

void OpDispatchBuilder::CalculateFlags_ADC(uint8_t SrcSize, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2, OrderedNode *CF) {
  auto Zero = _Constant(0);
  auto One = _Constant(1);
  auto OpSize = SrcSize == 8 ? OpSize::i64Bit : OpSize::i32Bit;

  CalculateAF(Src1, Src2);
  CalculatePF(Res);

  if (SrcSize >= 4) {
    HandleNZCV_RMW();
    _AdcNZCV(OpSize, Src1, Src2);
  } else {
    // SF/ZF
    SetNZ_ZeroCV(SrcSize, Res);

    // CF
    // Unsigned
    {
      auto SelectOpLT = _Select(FEXCore::IR::COND_ULT, Res, Src2, One, Zero);
      auto SelectOpLE = _Select(FEXCore::IR::COND_ULE, Res, Src2, One, Zero);
      auto SelectCF   = _Select(FEXCore::IR::COND_EQ, CF, One, SelectOpLE, SelectOpLT);
      SetRFLAG<FEXCore::X86State::RFLAG_CF_RAW_LOC>(SelectCF);
    }

    // Signed
    CalculateOF(SrcSize, Res, Src1, Src2, false);
  }
}

void OpDispatchBuilder::CalculateFlags_SBB(uint8_t SrcSize, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2, OrderedNode *CF) {
  auto Zero = _Constant(0);
  auto One = _Constant(1);
  auto OpSize = SrcSize == 8 ? OpSize::i64Bit : OpSize::i32Bit;

  CalculateAF(Src1, Src2);
  CalculatePF(Res);

  if (SrcSize >= 4) {
    // Rectify input carry
    CarryInvert();

    HandleNZCV_RMW();
    _SbbNZCV(OpSize, Src1, Src2);

    // Rectify output carry
    CarryInvert();
  } else {
    // SF/ZF
    SetNZ_ZeroCV(SrcSize, Res);

    // CF
    // Unsigned
    {
      auto SelectOpLT = _Select(FEXCore::IR::COND_UGT, Res, Src1, One, Zero);
      auto SelectOpLE = _Select(FEXCore::IR::COND_UGE, Res, Src1, One, Zero);
      auto SelectCF   = _Select(FEXCore::IR::COND_EQ, CF, One, SelectOpLE, SelectOpLT);
      SetRFLAG<FEXCore::X86State::RFLAG_CF_RAW_LOC>(SelectCF);
    }

    // Signed
    CalculateOF(SrcSize, Res, Src1, Src2, true);
  }
}

void OpDispatchBuilder::CalculateFlags_SUB(uint8_t SrcSize, OrderedNode *Src1, OrderedNode *Src2, bool UpdateCF) {
  // Stash CF before stomping over it
  auto OldCF = UpdateCF ? nullptr : GetRFLAG(FEXCore::X86State::RFLAG_CF_RAW_LOC);

  HandleNZCVWrite();

  CalculateAF(Src1, Src2);

  OrderedNode *Res;
  if (SrcSize >= 4) {
    Res = _SubWithFlags(IR::SizeToOpSize(SrcSize), Src1, Src2);
  } else {
    _SubNZCV(IR::SizeToOpSize(SrcSize), Src1, Src2);
    Res = _Sub(OpSize::i32Bit, Src1, Src2);
  }

  CalculatePF(Res);

  // If we're updating CF, we need to invert it for correctness. If we're not
  // updating CF, we need to restore the CF since we stomped over it.
  if (UpdateCF)
    CarryInvert();
  else
    SetRFLAG<FEXCore::X86State::RFLAG_CF_RAW_LOC>(OldCF);
}

void OpDispatchBuilder::CalculateFlags_ADD(uint8_t SrcSize, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2, bool UpdateCF) {
  CalculateAF(Src1, Src2);
  CalculatePF(Res);

  // Stash CF before stomping over it
  auto OldCF = UpdateCF ? nullptr : GetRFLAG(FEXCore::X86State::RFLAG_CF_RAW_LOC);

  HandleNZCVWrite();
  _AddNZCV(IR::SizeToOpSize(SrcSize), Src1, Src2);

  // We stomped over CF while calculation flags, restore it.
  if (!UpdateCF)
    SetRFLAG<FEXCore::X86State::RFLAG_CF_RAW_LOC>(OldCF);
}

void OpDispatchBuilder::CalculateFlags_MUL(uint8_t SrcSize, OrderedNode *Res, OrderedNode *High) {
  HandleNZCVWrite();

  // PF/AF/ZF/SF
  // Undefined
  {
    _InvalidateFlags(1 << X86State::RFLAG_PF_RAW_LOC);
    _InvalidateFlags(1 << X86State::RFLAG_AF_RAW_LOC);
  }

  // CF/OF
  {
    // CF and OF are set if the result of the operation can't be fit in to the destination register
    // If the value can fit then the top bits will be zero
    auto SignBit = _Sbfe(OpSize::i64Bit, 1, SrcSize * 8 - 1, Res);
    _SubNZCV(OpSize::i64Bit, High, SignBit);

    // If High = SignBit, then sets to nZcv. Else sets to nzCV. Since SF/ZF
    // undefined, this does what we need.
    auto Zero = _Constant(0);
    _CondAddNZCV(OpSize::i64Bit, Zero, Zero, CondClassType{COND_EQ}, 0x3 /* nzCV */);
  }
}

void OpDispatchBuilder::CalculateFlags_UMUL(OrderedNode *High) {
  HandleNZCVWrite();

  auto Zero = _Constant(0);
  OpSize Size = IR::SizeToOpSize(GetOpSize(High));

  // AF/SF/PF/ZF
  // Undefined
  {
    _InvalidateFlags(1 << X86State::RFLAG_PF_RAW_LOC);
    _InvalidateFlags(1 << X86State::RFLAG_AF_RAW_LOC);
  }

  // CF/OF
  {
    // CF and OF are set if the result of the operation can't be fit in to the destination register
    // The result register will be all zero if it can't fit due to how multiplication behaves
    _SubNZCV(Size, High, Zero);

    // If High = 0, then sets to nZcv. Else sets to nzCV. Since SF/ZF undefined,
    // this does what we need.
    _CondAddNZCV(Size, Zero, Zero, CondClassType{COND_EQ}, 0x3 /* nzCV */);
  }
}

void OpDispatchBuilder::CalculateFlags_Logical(uint8_t SrcSize, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2) {
  // AF
  // Undefined
  _InvalidateFlags(1 << X86State::RFLAG_AF_RAW_LOC);

  CalculatePF(Res);

  // SF/ZF/CF/OF
  SetNZ_ZeroCV(SrcSize, Res);
}

void OpDispatchBuilder::CalculateFlags_ShiftLeft(uint8_t SrcSize, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2) {
  CalculateFlags_ShiftVariable(Src2, [this, SrcSize, Res, Src1, Src2](){
    const auto OpSize = SrcSize == 8 ? OpSize::i64Bit : OpSize::i32Bit;
    SetNZ_ZeroCV(SrcSize, Res);

    // Extract the last bit shifted in to CF
    auto Size = _Constant(SrcSize * 8);
    auto ShiftAmt = _Sub(OpSize, Size, Src2);
    auto LastBit = _Lshr(OpSize, Src1, ShiftAmt);
    SetRFLAG<FEXCore::X86State::RFLAG_CF_RAW_LOC>(LastBit, 0, true);

    CalculatePF(Res);

    // AF
    // Undefined
    _InvalidateFlags(1 << X86State::RFLAG_AF_RAW_LOC);

    // In the case of left shift. OF is only set from the result of <Top Source Bit> XOR <Top Result Bit>
    // When Shift > 1 then OF is undefined
    auto OFXor = _Xor(OpSize, Src1, Res);
    SetRFLAG<FEXCore::X86State::RFLAG_OF_RAW_LOC>(OFXor, SrcSize * 8 - 1, true);
  });
}

void OpDispatchBuilder::CalculateFlags_ShiftRight(uint8_t SrcSize, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2) {
  CalculateFlags_ShiftVariable(Src2, [this, SrcSize, Res, Src1, Src2](){
    const auto OpSize = SrcSize == 8 ? OpSize::i64Bit : OpSize::i32Bit;
    SetNZ_ZeroCV(SrcSize, Res);

    // Extract the last bit shifted in to CF
    auto ShiftAmt = _Sub(OpSize::i64Bit, Src2, _Constant(1));
    const auto CFSize = IR::SizeToOpSize(std::max<uint8_t>(4u, SrcSize));
    auto LastBit = _Lshr(CFSize, Src1, ShiftAmt);
    SetRFLAG<FEXCore::X86State::RFLAG_CF_RAW_LOC>(LastBit, 0, true);

    CalculatePF(Res);

    // AF
    // Undefined
    _InvalidateFlags(1 << X86State::RFLAG_AF_RAW_LOC);

    // Only defined when Shift is 1 else undefined
    // OF flag is set if a sign change occurred
    auto val = _Xor(OpSize, Src1, Res);
    SetRFLAG<FEXCore::X86State::RFLAG_OF_RAW_LOC>(val, SrcSize * 8 - 1, true);
  });
}

void OpDispatchBuilder::CalculateFlags_SignShiftRight(uint8_t SrcSize, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2) {
  CalculateFlags_ShiftVariable(Src2, [this, SrcSize, Res, Src1, Src2](){
    // SF/ZF/OF
    SetNZ_ZeroCV(SrcSize, Res);

    // Extract the last bit shifted in to CF
    const auto CFSize = IR::SizeToOpSize(std::max<uint32_t>(4u, GetOpSize(Src1)));
    auto ShiftAmt = _Sub(OpSize::i64Bit, Src2, _Constant(1));
    auto LastBit = _Lshr(CFSize, Src1, ShiftAmt);
    SetRFLAG<FEXCore::X86State::RFLAG_CF_RAW_LOC>(LastBit, 0, true);

    CalculatePF(Res);

    // AF
    // Undefined
    _InvalidateFlags(1 << X86State::RFLAG_AF_RAW_LOC);
  });
}

void OpDispatchBuilder::CalculateFlags_ShiftLeftImmediate(uint8_t SrcSize, OrderedNode *UnmaskedRes, OrderedNode *Src1, uint64_t Shift) {
  // No flags changed if shift is zero
  if (Shift == 0) return;

  auto OpSize = SrcSize == 8 ? OpSize::i64Bit : OpSize::i32Bit;

  SetNZ_ZeroCV(SrcSize, UnmaskedRes);

  // CF
  {
    // Extract the last bit shifted in to CF
    auto SrcSizeBits = SrcSize * 8;
    if (SrcSizeBits < Shift) {
      Shift &= (SrcSizeBits - 1);
    }
    SetRFLAG<FEXCore::X86State::RFLAG_CF_RAW_LOC>(Src1, SrcSizeBits - Shift, true);
  }

  CalculatePF(UnmaskedRes);

  // AF
  // Undefined
  _InvalidateFlags(1 << X86State::RFLAG_AF_RAW_LOC);

  // OF
  // In the case of left shift. OF is only set from the result of <Top Source Bit> XOR <Top Result Bit>
  if (Shift == 1) {
    auto Xor = _Xor(OpSize, UnmaskedRes, Src1);
    SetRFLAG<FEXCore::X86State::RFLAG_OF_RAW_LOC>(Xor, SrcSize * 8 - 1, true);
  } else {
    // Undefined, we choose to zero as part of SetNZ_ZeroCV
  }
}

void OpDispatchBuilder::CalculateFlags_SignShiftRightImmediate(uint8_t SrcSize, OrderedNode *Res, OrderedNode *Src1, uint64_t Shift) {
  // No flags changed if shift is zero
  if (Shift == 0) return;

  SetNZ_ZeroCV(SrcSize, Res);

  // CF
  {
    // Extract the last bit shifted in to CF
    SetRFLAG<FEXCore::X86State::RFLAG_CF_RAW_LOC>(Src1, Shift-1, true);
  }

  CalculatePF(Res);

  // AF
  // Undefined
  _InvalidateFlags(1 << X86State::RFLAG_AF_RAW_LOC);

  // OF
  // Only defined when Shift is 1 else undefined. Only is set if the top bit was set to 1 when
  // shifted So it is set to zero.  In the undefined case we choose to zero as well. Since it was
  // already zeroed there's nothing to do here.
}

void OpDispatchBuilder::CalculateFlags_ShiftRightImmediateCommon(uint8_t SrcSize, OrderedNode *Res, OrderedNode *Src1, uint64_t Shift) {
  // Set SF and PF. Clobbers OF, but OF only defined for Shift = 1 where it is
  // set below.
  SetNZ_ZeroCV(SrcSize, Res);

  // CF
  {
    // Extract the last bit shifted in to CF
    SetRFLAG<FEXCore::X86State::RFLAG_CF_RAW_LOC>(Src1, Shift-1, true);
  }

  CalculatePF(Res);

  // AF
  // Undefined
  _InvalidateFlags(1 << X86State::RFLAG_AF_RAW_LOC);
}

void OpDispatchBuilder::CalculateFlags_ShiftRightImmediate(uint8_t SrcSize, OrderedNode *Res, OrderedNode *Src1, uint64_t Shift) {
  // No flags changed if shift is zero
  if (Shift == 0) return;

  CalculateFlags_ShiftRightImmediateCommon(SrcSize, Res, Src1, Shift);

  // OF
  {
    // Only defined when Shift is 1 else undefined
    // Is set to the MSB of the original value
    if (Shift == 1) {
      SetRFLAG<FEXCore::X86State::RFLAG_OF_RAW_LOC>(Src1, SrcSize * 8 - 1, true);
    }
  }
}

void OpDispatchBuilder::CalculateFlags_ShiftRightDoubleImmediate(uint8_t SrcSize, OrderedNode *Res, OrderedNode *Src1, uint64_t Shift) {
  // No flags changed if shift is zero
  if (Shift == 0) return;

  const auto OpSize = SrcSize == 8 ? OpSize::i64Bit : OpSize::i32Bit;
  CalculateFlags_ShiftRightImmediateCommon(SrcSize, Res, Src1, Shift);

  // OF
  {
    // Only defined when Shift is 1 else undefined
    // Is set if the MSB bit changes.
    // XOR of Result and Src1
    if (Shift == 1) {
      auto val = _Xor(OpSize, Src1, Res);
      SetRFLAG<FEXCore::X86State::RFLAG_OF_RAW_LOC>(val, SrcSize * 8 - 1, true);
    }
  }
}

void OpDispatchBuilder::CalculateFlags_RotateRight(uint8_t SrcSize, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2) {
  CalculateFlags_ShiftVariable(Src2, [this, SrcSize, Res](){
    auto SizeBits = SrcSize * 8;
    const auto OpSize = SrcSize == 8 ? OpSize::i64Bit : OpSize::i32Bit;

    // Ends up faster overall if we don't have FlagM, slower if we do...
    // If Shift != 1, OF is undefined so we choose to zero here.
    if (!CTX->HostFeatures.SupportsFlagM)
      ZeroCV();

    // Extract the last bit shifted in to CF
    SetRFLAG<FEXCore::X86State::RFLAG_CF_RAW_LOC>(Res, SizeBits - 1, true);

    // OF is set to the XOR of the new CF bit and the most significant bit of the result
    // OF is architecturally only defined for 1-bit rotate, which is why this only happens when the shift is one.
    auto NewOF = _XorShift(OpSize, Res, Res, ShiftType::LSR, 1);
    SetRFLAG<FEXCore::X86State::RFLAG_OF_RAW_LOC>(NewOF, SizeBits - 2, true);
  });
}

void OpDispatchBuilder::CalculateFlags_RotateLeft(uint8_t SrcSize, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2) {
  CalculateFlags_ShiftVariable(Src2, [this, SrcSize, Res](){
    const auto OpSize = SrcSize == 8 ? OpSize::i64Bit : OpSize::i32Bit;
    auto SizeBits = SrcSize * 8;

    // Ends up faster overall if we don't have FlagM, slower if we do...
    // If Shift != 1, OF is undefined so we choose to zero here.
    if (!CTX->HostFeatures.SupportsFlagM)
      ZeroCV();

    // Extract the last bit shifted in to CF
    //auto Size = _Constant(GetSrcSize(Res) * 8);
    //auto ShiftAmt = _Sub(OpSize::i64Bit, Size, Src2);
    SetRFLAG<FEXCore::X86State::RFLAG_CF_RAW_LOC>(Res, 0, true);

    // OF is the LSB and MSB XOR'd together.
    // OF is set to the XOR of the new CF bit and the most significant bit of the result.
    // OF is architecturally only defined for 1-bit rotate, which is why this only happens when the shift is one.
    auto NewOF = _XorShift(OpSize, Res, Res, ShiftType::LSR, SizeBits - 1);
    SetRFLAG<FEXCore::X86State::RFLAG_OF_RAW_LOC>(NewOF, 0, true);
  });
}

void OpDispatchBuilder::CalculateFlags_RotateRightImmediate(uint8_t SrcSize, OrderedNode *Res, OrderedNode *Src1, uint64_t Shift) {
  if (Shift == 0) return;

  const auto OpSize = SrcSize == 8 ? OpSize::i64Bit : OpSize::i32Bit;
  auto SizeBits = SrcSize * 8;

  // Ends up faster overall if we don't have FlagM, slower if we do...
  // If Shift != 1, OF is undefined so we choose to zero here.
  if (!CTX->HostFeatures.SupportsFlagM)
    ZeroCV();

  // CF
  {
    // Extract the last bit shifted in to CF
    SetRFLAG<FEXCore::X86State::RFLAG_CF_RAW_LOC>(Res, SizeBits - 1, true);
  }

  // OF
  {
    if (Shift == 1) {
      // OF is the top two MSBs XOR'd together
      // OF is architecturally only defined for 1-bit rotate, which is why this only happens when the shift is one.
      auto NewOF = _XorShift(OpSize, Res, Res, ShiftType::LSR, 1);
      SetRFLAG<FEXCore::X86State::RFLAG_OF_RAW_LOC>(NewOF, SizeBits - 2, 1);
    }
  }
}

void OpDispatchBuilder::CalculateFlags_RotateLeftImmediate(uint8_t SrcSize, OrderedNode *Res, OrderedNode *Src1, uint64_t Shift) {
  if (Shift == 0) return;

  const auto OpSize = SrcSize == 8 ? OpSize::i64Bit : OpSize::i32Bit;
  auto SizeBits = SrcSize * 8;

  // Ends up faster overall if we don't have FlagM, slower if we do...
  // If Shift != 1, OF is undefined so we choose to zero here.
  if (!CTX->HostFeatures.SupportsFlagM)
    ZeroCV();

  // CF
  {
    // Extract the last bit shifted in to CF
    SetRFLAG<FEXCore::X86State::RFLAG_CF_RAW_LOC>(Res, 0, true);
  }

  // OF
  {
    if (Shift == 1) {
      // OF is the LSB and MSB XOR'd together.
      // OF is set to the XOR of the new CF bit and the most significant bit of the result.
      // OF is architecturally only defined for 1-bit rotate, which is why this only happens when the shift is one.
      auto NewOF = _XorShift(OpSize, Res, Res, ShiftType::LSR, SizeBits - 1);

      SetRFLAG<FEXCore::X86State::RFLAG_OF_RAW_LOC>(NewOF, 0, true);
    }
  }
}

void OpDispatchBuilder::CalculateFlags_BEXTR(OrderedNode *Src) {
  // ZF is set properly. CF and OF are defined as being set to zero. SF, PF, and
  // AF are undefined.
  SetNZ_ZeroCV(GetOpSize(Src), Src);

  _InvalidateFlags((1UL << X86State::RFLAG_PF_RAW_LOC) |
                   (1UL << X86State::RFLAG_AF_RAW_LOC));
}

void OpDispatchBuilder::CalculateFlags_BLSI(uint8_t SrcSize, OrderedNode *Result) {
  // CF is cleared if Src is zero, otherwise it's set. However, Src is zero iff
  // Result is zero, so we can test the result instead. So, CF is just the
  // inverted ZF.
  //
  // ZF/SF/OF set as usual.
  SetNZ_ZeroCV(SrcSize, Result);

  auto CFOp = GetRFLAG(X86State::RFLAG_ZF_RAW_LOC, true /* Invert */);
  SetRFLAG<X86State::RFLAG_CF_RAW_LOC>(CFOp);

  // PF/AF undefined
  _InvalidateFlags((1UL << X86State::RFLAG_PF_RAW_LOC) |
                   (1UL << X86State::RFLAG_AF_RAW_LOC));
}

void OpDispatchBuilder::CalculateFlags_BLSMSK(uint8_t SrcSize, OrderedNode *Result, OrderedNode *Src) {
  // PF/AF undefined
  _InvalidateFlags((1UL << X86State::RFLAG_PF_RAW_LOC) |
                   (1UL << X86State::RFLAG_AF_RAW_LOC));

  // CF set according to the Src
  auto Zero = _Constant(0);
  auto One = _Constant(1);
  auto CFOp = _Select(IR::COND_EQ, Src, Zero, One, Zero);

  // The output of BLSMSK is always nonzero, so TST will clear Z (along with C
  // and O) while setting S.
  SetNZ_ZeroCV(SrcSize, Result);
  SetRFLAG<X86State::RFLAG_CF_RAW_LOC>(CFOp);
}

void OpDispatchBuilder::CalculateFlags_BLSR(uint8_t SrcSize, OrderedNode *Result, OrderedNode *Src) {
  auto Zero = _Constant(0);
  auto One = _Constant(1);
  auto CFOp = _Select(IR::COND_EQ, Src, Zero, One, Zero);

  SetNZ_ZeroCV(SrcSize, Result);
  SetRFLAG<X86State::RFLAG_CF_RAW_LOC>(CFOp);

  // PF/AF undefined
  _InvalidateFlags((1UL << X86State::RFLAG_PF_RAW_LOC) |
                   (1UL << X86State::RFLAG_AF_RAW_LOC));
}

void OpDispatchBuilder::CalculateFlags_POPCOUNT(OrderedNode *Result) {
  // We need to set ZF while clearing the rest of NZCV. The result of a popcount
  // is in the range [0, 63]. In particular, it is always positive. So a
  // combined NZ test will correctly zero SF/CF/OF while setting ZF.
  SetNZ_ZeroCV(OpSize::i32Bit, Result);

  ZeroMultipleFlags((1U << X86State::RFLAG_AF_RAW_LOC) |
                    (1U << X86State::RFLAG_PF_RAW_LOC));
}

void OpDispatchBuilder::CalculateFlags_BZHI(uint8_t SrcSize, OrderedNode *Result, OrderedNode *Src) {
  // PF/AF undefined
  _InvalidateFlags((1UL << X86State::RFLAG_PF_RAW_LOC) |
                   (1UL << X86State::RFLAG_AF_RAW_LOC));

  SetNZ_ZeroCV(SrcSize, Result);
  SetRFLAG<X86State::RFLAG_CF_RAW_LOC>(Src);
}

void OpDispatchBuilder::CalculateFlags_ZCNT(uint8_t SrcSize, OrderedNode *Result) {
  // OF, SF, AF, PF all undefined
  // Test ZF of result, SF is undefined so this is ok.
  SetNZ_ZeroCV(SrcSize, Result);

  // Now set CF if the Result = SrcSize * 8. Since SrcSize is a power-of-two and
  // Result is <= SrcSize * 8, we equivalently check if the log2(SrcSize * 8)
  // bit is set. No masking is needed because no higher bits could be set.
  unsigned CarryBit = FEXCore::ilog2(SrcSize * 8u);
  SetRFLAG<FEXCore::X86State::RFLAG_CF_RAW_LOC>(Result, CarryBit);
}

void OpDispatchBuilder::CalculateFlags_RDRAND(OrderedNode *Src) {
  // OF, SF, ZF, AF, PF all zero
  // CF is set to the incoming source

  uint32_t FlagsMaskToZero =
    FullNZCVMask |
    (1U << X86State::RFLAG_AF_RAW_LOC) |
    (1U << X86State::RFLAG_PF_RAW_LOC);

  ZeroMultipleFlags(FlagsMaskToZero);

  SetRFLAG<FEXCore::X86State::RFLAG_CF_RAW_LOC>(Src);
}

}
