// SPDX-License-Identifier: MIT
/*
$info$
tags: frontend|x86-to-ir, opcodes|dispatcher-implementations
desc: Handles x86/64 flag generation
$end_info$
*/

#include "Interface/Core/OpcodeDispatcher.h"
#include "Interface/Core/X86Tables/X86Tables.h"

#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/Config/Config.h>
#include <FEXCore/Utils/LogManager.h>

#include <array>
#include <cstdint>

namespace FEXCore::IR {
constexpr std::array<uint32_t, 17> FlagOffsets = {
  FEXCore::X86State::RFLAG_CF_RAW_LOC, FEXCore::X86State::RFLAG_PF_RAW_LOC, FEXCore::X86State::RFLAG_AF_RAW_LOC,
  FEXCore::X86State::RFLAG_ZF_RAW_LOC, FEXCore::X86State::RFLAG_SF_RAW_LOC, FEXCore::X86State::RFLAG_TF_RAW_LOC,
  FEXCore::X86State::RFLAG_IF_LOC,     FEXCore::X86State::RFLAG_DF_RAW_LOC, FEXCore::X86State::RFLAG_OF_RAW_LOC,
  FEXCore::X86State::RFLAG_IOPL_LOC,   FEXCore::X86State::RFLAG_NT_LOC,     FEXCore::X86State::RFLAG_RF_LOC,
  FEXCore::X86State::RFLAG_VM_LOC,     FEXCore::X86State::RFLAG_AC_LOC,     FEXCore::X86State::RFLAG_VIF_LOC,
  FEXCore::X86State::RFLAG_VIP_LOC,    FEXCore::X86State::RFLAG_ID_LOC,
};

void OpDispatchBuilder::ZeroPF_AF() {
  // PF is stored inverted, so invert it when we zero.
  SetRFLAG<FEXCore::X86State::RFLAG_PF_RAW_LOC>(Constant(1));
  SetAF(0);
}

void OpDispatchBuilder::SetPackedRFLAG(bool Lower8, Ref Src) {
  size_t NumFlags = FlagOffsets.size();
  if (Lower8) {
    // Calculate flags early.
    // This is only a partial overwrite of flags since OF isn't stored here.
    CalculateDeferredFlags();
    NumFlags = 5;
  }

  // PF and CF are both stored inverted, so hoist the invert.
  auto SrcInverted = _Not(OpSize::i32Bit, Src);

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
    } else if (FlagOffset == FEXCore::X86State::RFLAG_PF_RAW_LOC || FlagOffset == FEXCore::X86State::RFLAG_CF_RAW_LOC) {
      // PF and CF are both stored parity flipped.
      SetRFLAG(SrcInverted, FlagOffset, FlagOffset, true);
    } else {
      SetRFLAG(Src, FlagOffset, FlagOffset, true);
    }
  }

  CFInverted = true;
}

Ref OpDispatchBuilder::GetPackedRFLAG(uint32_t FlagsMask) {
  // Calculate flags early.
  CalculateDeferredFlags();

  // SF/ZF and N/Z are together on both arm64 and x86_64, so we special case that.
  bool GetNZ = (FlagsMask & (1 << FEXCore::X86State::RFLAG_SF_RAW_LOC)) && (FlagsMask & (1 << FEXCore::X86State::RFLAG_ZF_RAW_LOC));

  // Handle CF first, since it's at bit 0 and hence doesn't need shift or OR.
  LOGMAN_THROW_A_FMT(FlagsMask & (1 << FEXCore::X86State::RFLAG_CF_RAW_LOC), "CF always handled");
  static_assert(FEXCore::X86State::RFLAG_CF_RAW_LOC == 0);
  Ref Original = GetRFLAG(FEXCore::X86State::RFLAG_CF_RAW_LOC);

  for (size_t i = 0; i < FlagOffsets.size(); ++i) {
    const auto FlagOffset = FlagOffsets[i];
    if (!((1U << FlagOffset) & FlagsMask)) {
      continue;
    }

    if ((GetNZ && (FlagOffset == FEXCore::X86State::RFLAG_SF_RAW_LOC || FlagOffset == FEXCore::X86State::RFLAG_ZF_RAW_LOC)) ||
        FlagOffset == FEXCore::X86State::RFLAG_CF_RAW_LOC || FlagOffset == FEXCore::X86State::RFLAG_PF_RAW_LOC) {
      // Already handled
      continue;
    }

    // Note that the Bfi only considers the bottom bit of the flag, the rest of
    // the byte is allowed to be garbage.
    Ref Flag;
    if (FlagOffset == FEXCore::X86State::RFLAG_AF_RAW_LOC) {
      Flag = LoadAF();
    } else {
      Flag = GetRFLAG(FlagOffset);
    }

    Original = _Orlshl(OpSize::i64Bit, Original, Flag, FlagOffset);
  }

  // Raw PF value needs to have its bottom bit masked out and inverted. The
  // naive sequence is and/eor/orlshl. But we can do the inversion implicitly
  // instead.
  if (FlagsMask & (1 << FEXCore::X86State::RFLAG_PF_RAW_LOC)) {
    // Set every bit except the bottommost.
    auto OnesInvPF = _Or(OpSize::i64Bit, LoadPFRaw(false, false), _InlineConstant(~1ull));

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
    auto NZ = _And(OpSize::i64Bit, NZCV, _InlineConstant(0b11u << 30));
    Original = _Orlshr(OpSize::i64Bit, Original, NZ, 31 - FEXCore::X86State::RFLAG_SF_RAW_LOC);
  }

  // The constant is OR'ed in at the end, to avoid a pointless or xzr, #2.
  if ((1U << X86State::RFLAG_RESERVED_LOC) & FlagsMask) {
    Original = _Or(OpSize::i64Bit, Original, _InlineConstant(2));
  }

  return Original;
}

void OpDispatchBuilder::CalculateOF(IR::OpSize SrcSize, Ref Res, Ref Src1, Ref Src2, bool Sub) {
  LOGMAN_THROW_A_FMT(SrcSize >= IR::OpSize::i8Bit && SrcSize <= IR::OpSize::i64Bit, "Invalid size");
  const auto OpSize = SrcSize == OpSize::i64Bit ? OpSize::i64Bit : OpSize::i32Bit;
  const uint64_t SignBit = IR::OpSizeAsBits(SrcSize) - 1;
  Ref Anded = nullptr;

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

    if (Negative ^ Sub) {
      Anded = _Andn(OpSize, Src1, Res);
    } else {
      Anded = _Andn(OpSize, Res, Src1);
    }
  } else {
    auto XorOp1 = _Xor(OpSize, Src1, Src2);
    auto XorOp2 = _Xor(OpSize, Res, Src1);

    if (Sub) {
      Anded = _And(OpSize, XorOp2, XorOp1);
    } else {
      Anded = _Andn(OpSize, XorOp2, XorOp1);
    }
  }

  SetRFLAG<FEXCore::X86State::RFLAG_OF_RAW_LOC>(Anded, SignBit, true);
}

Ref OpDispatchBuilder::LoadPFRaw(bool Mask, bool Invert) {
  // Most blocks do not read parity, so PF optimization is gated on this flag.
  CurrentHeader->ReadsParity = true;

  // Evaluate parity on the deferred raw value.
  return _Parity(GetRFLAG(FEXCore::X86State::RFLAG_PF_RAW_LOC), Mask, Invert);
}

Ref OpDispatchBuilder::LoadAF() {
  // Read the stored value. This is the XOR of the arguments.
  auto AFWord = GetRFLAG(FEXCore::X86State::RFLAG_AF_RAW_LOC);

  // Read the result, stored for PF.
  auto Result = GetRFLAG(FEXCore::X86State::RFLAG_PF_RAW_LOC);

  // What's left is to XOR and extract. This is the deferred part. We
  // specifically use a 64-bit Xor here as we don't need masking.
  return _Bfe(OpSize::i32Bit, 1, 4, _Xor(OpSize::i64Bit, AFWord, Result));
}

void OpDispatchBuilder::FixupAF() {
  // The caller has set a desired value of AF in AF[4], regardless of the value
  // of PF. We need to fixup AF[4] so that we get the right value when we XOR in
  // PF[4] later. The easiest solution is to XOR by PF[4], since:
  //
  //  (AF[4] ^ PF[4]) ^ PF[4] = AF[4]

  auto PFRaw = GetRFLAG(FEXCore::X86State::RFLAG_PF_RAW_LOC);
  auto AFRaw = GetRFLAG(FEXCore::X86State::RFLAG_AF_RAW_LOC);

  // Again 64-bit as masking is more expensive.
  Ref XorRes = _Xor(OpSize::i64Bit, AFRaw, PFRaw);
  SetRFLAG<FEXCore::X86State::RFLAG_AF_RAW_LOC>(XorRes);
}

void OpDispatchBuilder::SetAFAndFixup(Ref AF) {
  // We have a value of AF, we shift into AF[4].  We need to fixup AF[4] so that
  // we get the right value when we XOR in PF[4] later. The easiest solution is
  // to XOR by PF[4], since:
  //
  //  (AF[4] ^ PF[4]) ^ PF[4] = AF[4]

  auto PFRaw = GetRFLAG(FEXCore::X86State::RFLAG_PF_RAW_LOC);

  Ref XorRes = _XorShift(OpSize::i32Bit, PFRaw, AF, ShiftType::LSL, 4);
  SetRFLAG<FEXCore::X86State::RFLAG_AF_RAW_LOC>(XorRes);
}

void OpDispatchBuilder::CalculatePF(Ref Res) {
  // Calculation is entirely deferred until load, just store the 8-bit result.
  SetRFLAG<FEXCore::X86State::RFLAG_PF_RAW_LOC>(Res);
}

void OpDispatchBuilder::CalculateAF(Ref Src1, Ref Src2) {
  // We only care about bit 4 in the subsequent XOR. If we'll XOR with 0,
  // there's no sense XOR'ing at all. If we'll XOR with 1, that's just
  // inverting.
  for (unsigned i = 0; i < 2; ++i) {
    Ref SrcA = i ? Src1 : Src2;
    Ref SrcB = i ? Src2 : Src1;

    uint64_t Const;
    if (IsValueConstant(WrapNode(SrcA), &Const)) {
      if (Const & (1u << 4)) {
        SetRFLAG<FEXCore::X86State::RFLAG_AF_RAW_LOC>(_Not(OpSize::i32Bit, SrcB));
      } else {
        SetRFLAG<FEXCore::X86State::RFLAG_AF_RAW_LOC>(SrcB);
      }

      return;
    }
  }

  // We store the XOR of the arguments. At read time, we XOR with the
  // appropriate bit of the result (available as the PF flag) and extract the
  // appropriate bit. Again 64-bit to avoid masking.
  Ref XorRes = Src1 == Src2 ? Constant(0) : _Xor(OpSize::i64Bit, Src1, Src2);
  SetRFLAG<FEXCore::X86State::RFLAG_AF_RAW_LOC>(XorRes);
}

void OpDispatchBuilder::CalculateDeferredFlags() {
  if (NZCVDirty && CachedNZCV) {
    _StoreNZCV(CachedNZCV);
  }

  CachedNZCV = nullptr;
  NZCVDirty = false;
}

Ref OpDispatchBuilder::IncrementByCarry(OpSize OpSize, Ref Src) {
  // If CF not inverted, we use .cc since the increment happens when the
  // condition is false. If CF inverted, invert to use .cs. A bit mindbendy.
  return _NZCVSelectIncrement(OpSize, CFInverted ? CondClass::UGE : CondClass::ULT, Src, Src);
}

Ref OpDispatchBuilder::CalculateFlags_ADC(IR::OpSize SrcSize, Ref Src1, Ref Src2) {
  auto OpSize = SrcSize == OpSize::i64Bit ? OpSize::i64Bit : OpSize::i32Bit;
  Ref Res;

  CalculateAF(Src1, Src2);

  if (SrcSize >= OpSize::i32Bit) {
    RectifyCarryInvert(false);
    HandleNZCV_RMW();
    Res = _AdcWithFlags(OpSize, Src1, Src2);
    CFInverted = false;
  } else {
    // Need to zero-extend for correct comparisons below
    Src2 = ARef(Src2).Bfe(0, IR::OpSizeAsBits(SrcSize)).Ref();

    // Note that we do not extend Src2PlusCF, since we depend on proper
    // 32-bit arithmetic to correctly handle the Src2 = 0xffff case.
    Ref Src2PlusCF = IncrementByCarry(OpSize, Src2);

    // Need to zero-extend for the comparison.
    Res = Add(OpSize, Src1, Src2PlusCF);
    Res = _Bfe(OpSize, IR::OpSizeAsBits(SrcSize), 0, Res);

    // TODO: We can fold that second Bfe in (cmp uxth).
    auto SelectCFInv = Select01(OpSize, CondClass::UGE, Res, Src2PlusCF);

    SetNZ_ZeroCV(SrcSize, Res);
    SetCFInverted(SelectCFInv);
    CalculateOF(SrcSize, Res, Src1, Src2, false);
  }

  CalculatePF(Res);
  return Res;
}

Ref OpDispatchBuilder::CalculateFlags_SBB(IR::OpSize SrcSize, Ref Src1, Ref Src2) {
  auto OpSize = SrcSize == OpSize::i64Bit ? OpSize::i64Bit : OpSize::i32Bit;

  CalculateAF(Src1, Src2);

  Ref Res;
  if (SrcSize >= OpSize::i32Bit) {
    // Arm's subtraction has inverted CF from x86, so rectify the input and
    // invert the output.
    RectifyCarryInvert(true);
    HandleNZCV_RMW();
    Res = _SbbWithFlags(OpSize, Src1, Src2);
    CFInverted = true;
  } else {
    // Zero extend for correct comparison behaviour with Src1 = 0xffff.
    Src1 = _Bfe(OpSize, IR::OpSizeAsBits(SrcSize), 0, Src1);
    Src2 = ARef(Src2).Bfe(0, IR::OpSizeAsBits(SrcSize)).Ref();

    auto Src2PlusCF = IncrementByCarry(OpSize, Src2);

    Res = Sub(OpSize, Src1, Src2PlusCF);
    Res = _Bfe(OpSize, IR::OpSizeAsBits(SrcSize), 0, Res);

    auto SelectCFInv = Select01(OpSize, CondClass::UGE, Src1, Src2PlusCF);

    SetNZ_ZeroCV(SrcSize, Res);
    SetCFInverted(SelectCFInv);
    CalculateOF(SrcSize, Res, Src1, Src2, true);
  }

  CalculatePF(Res);
  return Res;
}

Ref OpDispatchBuilder::CalculateFlags_SUB(IR::OpSize SrcSize, Ref Src1, Ref Src2, bool UpdateCF) {
  // Stash CF before stomping over it
  auto OldCFInv = UpdateCF ? nullptr : GetRFLAG(FEXCore::X86State::RFLAG_CF_RAW_LOC, true);

  HandleNZCVWrite();

  CalculateAF(Src1, Src2);

  Ref Res;
  if (SrcSize >= OpSize::i32Bit) {
    Res = SubWithFlags(SrcSize, Src1, Src2);
  } else {
    _SubNZCV(SrcSize, Src1, Src2);
    Res = Sub(OpSize::i32Bit, Src1, Src2);
  }

  CalculatePF(Res);

  // If we're updating CF, we need it to be inverted because SubNZCV is inverted
  // from x86. If we're not updating CF, we need to restore the CF since we
  // stomped over it.
  if (UpdateCF) {
    CFInverted = true;
  } else {
    SetCFInverted(OldCFInv);
  }

  return Res;
}

Ref OpDispatchBuilder::CalculateFlags_ADD(IR::OpSize SrcSize, Ref Src1, Ref Src2, bool UpdateCF) {
  // Stash CF before stomping over it
  auto OldCFInv = UpdateCF ? nullptr : GetRFLAG(FEXCore::X86State::RFLAG_CF_RAW_LOC, true);

  HandleNZCVWrite();

  CalculateAF(Src1, Src2);

  Ref Res;
  if (SrcSize >= OpSize::i32Bit) {
    Res = AddWithFlags(SrcSize, Src1, Src2);
  } else {
    _AddNZCV(SrcSize, Src1, Src2);
    Res = Add(OpSize::i32Bit, Src1, Src2);
  }

  CalculatePF(Res);

  // We stomped over CF while calculation flags, restore it.
  if (UpdateCF) {
    // Adds match between x86 and arm64.
    CFInverted = false;
  } else {
    SetCFInverted(OldCFInv);
  }

  return Res;
}

void OpDispatchBuilder::CalculateFlags_MUL(IR::OpSize SrcSize, Ref Res, Ref High) {
  HandleNZCVWrite();
  InvalidatePF_AF();

  // CF and OF are set if the result of the operation can't be fit in to the destination register
  // If the value can fit then the top bits will be zero
  auto SignBit = _Sbfe(OpSize::i64Bit, 1, IR::OpSizeAsBits(SrcSize) - 1, Res);
  _SubNZCV(OpSize::i64Bit, High, SignBit);

  // If High = SignBit, then sets to nZCv. Else sets to nzcV. Since SF/ZF
  // undefined, this does what we need after inverting carry.
  auto Zero = _InlineConstant(0);
  _CondSubNZCV(OpSize::i64Bit, Zero, Zero, CondClass::EQ, 0x1 /* nzcV */);
  CFInverted = true;
}

void OpDispatchBuilder::CalculateFlags_UMUL(Ref High) {
  HandleNZCVWrite();
  InvalidatePF_AF();

  auto Zero = _InlineConstant(0);
  const auto Size = GetOpSize(High);

  // CF and OF are set if the result of the operation can't be fit in to the destination register
  // The result register will be all zero if it can't fit due to how multiplication behaves
  _SubNZCV(Size, High, Zero);

  // If High = 0, then sets to nZCv. Else sets to nzcV. Since SF/ZF undefined,
  // this does what we need.
  _CondSubNZCV(Size, Zero, Zero, CondClass::EQ, 0x1 /* nzcV */);
  CFInverted = true;
}

void OpDispatchBuilder::CalculateFlags_Logical(IR::OpSize SrcSize, Ref Res) {
  InvalidateAF();
  SetNZP_ZeroCV(SrcSize, Res);
}

void OpDispatchBuilder::CalculateFlags_ShiftLeftImmediate(IR::OpSize SrcSize, Ref UnmaskedRes, Ref Src1, uint64_t Shift) {
  // No flags changed if shift is zero
  if (Shift == 0) {
    return;
  }

  auto OpSize = SrcSize == OpSize::i64Bit ? OpSize::i64Bit : OpSize::i32Bit;

  SetNZ_ZeroCV(SrcSize, UnmaskedRes);

  // CF
  {
    // Extract the last bit shifted in to CF. Shift is already masked, but for
    // 8/16-bit it might be >= SrcSizeBits, in which case CF is cleared. There's
    // nothing to do in that case since we already cleared CF above.
    const auto SrcSizeBits = IR::OpSizeAsBits(SrcSize);
    if (Shift < SrcSizeBits) {
      SetCFDirect(Src1, SrcSizeBits - Shift, true);
    }
  }

  CalculatePF(UnmaskedRes);
  InvalidateAF();

  // OF
  // In the case of left shift. OF is only set from the result of <Top Source Bit> XOR <Top Result Bit>
  if (Shift == 1) {
    auto Xor = _Xor(OpSize, UnmaskedRes, Src1);
    SetRFLAG<FEXCore::X86State::RFLAG_OF_RAW_LOC>(Xor, IR::OpSizeAsBits(SrcSize) - 1, true);
  } else {
    // Undefined, we choose to zero as part of SetNZ_ZeroCV
  }
}

void OpDispatchBuilder::CalculateFlags_SignShiftRightImmediate(IR::OpSize SrcSize, Ref Res, Ref Src1, uint64_t Shift) {
  // No flags changed if shift is zero
  if (Shift == 0) {
    return;
  }

  SetNZ_ZeroCV(SrcSize, Res);

  // Extract the last bit shifted in to CF
  SetCFDirect(Src1, Shift - 1, true);

  CalculatePF(Res);
  InvalidateAF();

  // OF
  // Only defined when Shift is 1 else undefined. Only is set if the top bit was set to 1 when
  // shifted So it is set to zero.  In the undefined case we choose to zero as well. Since it was
  // already zeroed there's nothing to do here.
}

void OpDispatchBuilder::CalculateFlags_ShiftRightImmediateCommon(IR::OpSize SrcSize, Ref Res, Ref Src1, uint64_t Shift) {
  // Set SF and PF. Clobbers OF, but OF only defined for Shift = 1 where it is
  // set below.
  SetNZ_ZeroCV(SrcSize, Res);

  // Extract the last bit shifted in to CF
  SetCFDirect(Src1, Shift - 1, true);

  CalculatePF(Res);
  InvalidateAF();
}

void OpDispatchBuilder::CalculateFlags_ShiftRightImmediate(IR::OpSize SrcSize, Ref Res, Ref Src1, uint64_t Shift) {
  // No flags changed if shift is zero
  if (Shift == 0) {
    return;
  }

  CalculateFlags_ShiftRightImmediateCommon(SrcSize, Res, Src1, Shift);

  // OF
  {
    // Only defined when Shift is 1 else undefined
    // Is set to the MSB of the original value
    if (Shift == 1) {
      SetRFLAG<FEXCore::X86State::RFLAG_OF_RAW_LOC>(Src1, IR::OpSizeAsBits(SrcSize) - 1, true);
    }
  }
}

void OpDispatchBuilder::CalculateFlags_ShiftRightDoubleImmediate(IR::OpSize SrcSize, Ref Res, Ref Src1, uint64_t Shift) {
  // No flags changed if shift is zero
  if (Shift == 0) {
    return;
  }

  const auto OpSize = SrcSize == OpSize::i64Bit ? OpSize::i64Bit : OpSize::i32Bit;
  CalculateFlags_ShiftRightImmediateCommon(SrcSize, Res, Src1, Shift);

  // OF
  {
    // Only defined when Shift is 1 else undefined
    // Is set if the MSB bit changes.
    // XOR of Result and Src1
    if (Shift == 1) {
      auto val = _Xor(OpSize, Src1, Res);
      SetRFLAG<FEXCore::X86State::RFLAG_OF_RAW_LOC>(val, IR::OpSizeAsBits(SrcSize) - 1, true);
    }
  }
}

void OpDispatchBuilder::CalculateFlags_ZCNT(IR::OpSize SrcSize, Ref Result) {
  // OF, SF, AF, PF all undefined
  // Test ZF of result, SF is undefined so this is ok.
  SetNZ_ZeroCV(SrcSize, Result);

  // Now set CF if the Result = SrcSize * 8. Since SrcSize is a power-of-two and
  // Result is <= SrcSize * 8, we equivalently check if the log2(SrcSize * 8)
  // bit is set. No masking is needed because no higher bits could be set.
  unsigned CarryBit = FEXCore::ilog2(IR::OpSizeAsBits(SrcSize));
  SetCFDirect(Result, CarryBit);
}

} // namespace FEXCore::IR
