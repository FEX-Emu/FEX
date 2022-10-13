/*
$info$
tags: frontend|x86-to-ir, opcodes|dispatcher-implementations
desc: Handles x86/64 flag generation
$end_info$
*/

#include "Interface/Context/Context.h"
#include "Interface/Core/OpcodeDispatcher.h"

#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/Config/Config.h>
#include <FEXCore/Debug/X86Tables.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/IR/IR.h>

#include <array>
#include <cstdint>

namespace FEXCore::IR {
constexpr std::array<uint32_t, 17> FlagOffsets = {
  FEXCore::X86State::RFLAG_CF_LOC,
  FEXCore::X86State::RFLAG_PF_LOC,
  FEXCore::X86State::RFLAG_AF_LOC,
  FEXCore::X86State::RFLAG_ZF_LOC,
  FEXCore::X86State::RFLAG_SF_LOC,
  FEXCore::X86State::RFLAG_TF_LOC,
  FEXCore::X86State::RFLAG_IF_LOC,
  FEXCore::X86State::RFLAG_DF_LOC,
  FEXCore::X86State::RFLAG_OF_LOC,
  FEXCore::X86State::RFLAG_IOPL_LOC,
  FEXCore::X86State::RFLAG_NT_LOC,
  FEXCore::X86State::RFLAG_RF_LOC,
  FEXCore::X86State::RFLAG_VM_LOC,
  FEXCore::X86State::RFLAG_AC_LOC,
  FEXCore::X86State::RFLAG_VIF_LOC,
  FEXCore::X86State::RFLAG_VIP_LOC,
  FEXCore::X86State::RFLAG_ID_LOC,
};

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

  auto OneConst = _Constant(1);
  for (size_t i = 0; i < NumFlags; ++i) {
    const auto FlagOffset = FlagOffsets[i];
    auto Tmp = _And(_Lshr(Src, _Constant(FlagOffset)), OneConst);
    SetRFLAG(Tmp, FlagOffset);
  }
}

OrderedNode *OpDispatchBuilder::GetPackedRFLAG(bool Lower8) {
  // Calculate flags early.
  CalculateDeferredFlags();

  OrderedNode *Original = _Constant(2);
  size_t NumFlags = FlagOffsets.size();
  if (Lower8) {
    NumFlags = 5;
  }

  for (size_t i = 0; i < NumFlags; ++i) {
    const auto FlagOffset = FlagOffsets[i];
    OrderedNode *Flag = _LoadFlag(FlagOffset);
    Flag = _Bfe(4, 32, 0, Flag);
    Flag = _Lshl(Flag, _Constant(FlagOffset));
    Original = _Or(Original, Flag);
  }
  return Original;
}

void OpDispatchBuilder::CalculateDeferredFlags(uint32_t FlagsToCalculateMask) {
  if (CurrentDeferredFlags.Type == FlagsGenerationType::TYPE_NONE) {
    // Nothing to do
    return;
  }

  switch (CurrentDeferredFlags.Type) {
    case FlagsGenerationType::TYPE_ADC:
      CalculcateFlags_ADC(
        CurrentDeferredFlags.SrcSize,
        CurrentDeferredFlags.Res,
        CurrentDeferredFlags.Sources.ThreeSource.Src1,
        CurrentDeferredFlags.Sources.ThreeSource.Src2,
        CurrentDeferredFlags.Sources.ThreeSource.Src3);
      break;
    case FlagsGenerationType::TYPE_SBB:
      CalculcateFlags_SBB(
        CurrentDeferredFlags.SrcSize,
        CurrentDeferredFlags.Res,
        CurrentDeferredFlags.Sources.ThreeSource.Src1,
        CurrentDeferredFlags.Sources.ThreeSource.Src2,
        CurrentDeferredFlags.Sources.ThreeSource.Src3);
      break;
    case FlagsGenerationType::TYPE_SUB:
      CalculcateFlags_SUB(
        CurrentDeferredFlags.SrcSize,
        CurrentDeferredFlags.Res,
        CurrentDeferredFlags.Sources.TwoSrcImmediate.Src1,
        CurrentDeferredFlags.Sources.TwoSrcImmediate.Src2,
        CurrentDeferredFlags.Sources.TwoSrcImmediate.UpdateCF);
      break;
    case FlagsGenerationType::TYPE_ADD:
      CalculcateFlags_ADD(
        CurrentDeferredFlags.SrcSize,
        CurrentDeferredFlags.Res,
        CurrentDeferredFlags.Sources.TwoSrcImmediate.Src1,
        CurrentDeferredFlags.Sources.TwoSrcImmediate.Src2,
        CurrentDeferredFlags.Sources.TwoSrcImmediate.UpdateCF);
      break;
    case FlagsGenerationType::TYPE_MUL:
      CalculcateFlags_MUL(
        CurrentDeferredFlags.SrcSize,
        CurrentDeferredFlags.Res,
        CurrentDeferredFlags.Sources.OneSource.Src1);
      break;
    case FlagsGenerationType::TYPE_UMUL:
      CalculcateFlags_UMUL(CurrentDeferredFlags.Res);
      break;
    case FlagsGenerationType::TYPE_LOGICAL:
      CalculcateFlags_Logical(
        CurrentDeferredFlags.SrcSize,
        CurrentDeferredFlags.Res,
        CurrentDeferredFlags.Sources.TwoSource.Src1,
        CurrentDeferredFlags.Sources.TwoSource.Src2);
      break;
    case FlagsGenerationType::TYPE_LSHL:
      CalculcateFlags_ShiftLeft(
        CurrentDeferredFlags.SrcSize,
        CurrentDeferredFlags.Res,
        CurrentDeferredFlags.Sources.TwoSource.Src1,
        CurrentDeferredFlags.Sources.TwoSource.Src2);
      break;
    case FlagsGenerationType::TYPE_LSHLI:
      CalculcateFlags_ShiftLeftImmediate(
        CurrentDeferredFlags.SrcSize,
        CurrentDeferredFlags.Res,
        CurrentDeferredFlags.Sources.OneSrcImmediate.Src1,
        CurrentDeferredFlags.Sources.OneSrcImmediate.Imm);
      break;
    case FlagsGenerationType::TYPE_LSHR:
      CalculcateFlags_ShiftRight(
        CurrentDeferredFlags.SrcSize,
        CurrentDeferredFlags.Res,
        CurrentDeferredFlags.Sources.TwoSource.Src1,
        CurrentDeferredFlags.Sources.TwoSource.Src2);
      break;
    case FlagsGenerationType::TYPE_LSHRI:
      CalculcateFlags_ShiftRightImmediate(
        CurrentDeferredFlags.SrcSize,
        CurrentDeferredFlags.Res,
        CurrentDeferredFlags.Sources.OneSrcImmediate.Src1,
        CurrentDeferredFlags.Sources.OneSrcImmediate.Imm);
      break;
    case FlagsGenerationType::TYPE_ASHR:
      CalculcateFlags_SignShiftRight(
        CurrentDeferredFlags.SrcSize,
        CurrentDeferredFlags.Res,
        CurrentDeferredFlags.Sources.TwoSource.Src1,
        CurrentDeferredFlags.Sources.TwoSource.Src2);
      break;
    case FlagsGenerationType::TYPE_ASHRI:
      CalculcateFlags_SignShiftRightImmediate(
        CurrentDeferredFlags.SrcSize,
        CurrentDeferredFlags.Res,
        CurrentDeferredFlags.Sources.OneSrcImmediate.Src1,
        CurrentDeferredFlags.Sources.OneSrcImmediate.Imm);
      break;
    case FlagsGenerationType::TYPE_ROR:
      CalculcateFlags_RotateRight(
        CurrentDeferredFlags.SrcSize,
        CurrentDeferredFlags.Res,
        CurrentDeferredFlags.Sources.TwoSource.Src1,
        CurrentDeferredFlags.Sources.TwoSource.Src2);
      break;
    case FlagsGenerationType::TYPE_RORI:
      CalculcateFlags_RotateRightImmediate(
        CurrentDeferredFlags.SrcSize,
        CurrentDeferredFlags.Res,
        CurrentDeferredFlags.Sources.OneSrcImmediate.Src1,
        CurrentDeferredFlags.Sources.OneSrcImmediate.Imm);
      break;
    case FlagsGenerationType::TYPE_ROL:
      CalculcateFlags_RotateLeft(
        CurrentDeferredFlags.SrcSize,
        CurrentDeferredFlags.Res,
        CurrentDeferredFlags.Sources.TwoSource.Src1,
        CurrentDeferredFlags.Sources.TwoSource.Src2);
      break;
    case FlagsGenerationType::TYPE_ROLI:
      CalculcateFlags_RotateLeftImmediate(
        CurrentDeferredFlags.SrcSize,
        CurrentDeferredFlags.Res,
        CurrentDeferredFlags.Sources.OneSrcImmediate.Src1,
        CurrentDeferredFlags.Sources.OneSrcImmediate.Imm);
      break;
    case FlagsGenerationType::TYPE_FCMP:
      CalculcateFlags_FCMP(
        CurrentDeferredFlags.SrcSize,
        CurrentDeferredFlags.Res,
        CurrentDeferredFlags.Sources.TwoSource.Src1,
        CurrentDeferredFlags.Sources.TwoSource.Src2);
      break;
    case FlagsGenerationType::TYPE_BEXTR:
      CalculcateFlags_BEXTR(CurrentDeferredFlags.Res);
      break;
    case FlagsGenerationType::TYPE_BLSI:
      CalculcateFlags_BLSI(
        CurrentDeferredFlags.SrcSize,
        CurrentDeferredFlags.Res);
      break;
    case FlagsGenerationType::TYPE_BLSMSK:
      CalculcateFlags_BLSMSK(CurrentDeferredFlags.Res);
      break;
    case FlagsGenerationType::TYPE_BLSR:
      CalculcateFlags_BLSR(
        CurrentDeferredFlags.SrcSize,
        CurrentDeferredFlags.Res,
        CurrentDeferredFlags.Sources.OneSource.Src1);
      break;
    case FlagsGenerationType::TYPE_POPCOUNT:
      CalculcateFlags_POPCOUNT(CurrentDeferredFlags.Res);
      break;
    case FlagsGenerationType::TYPE_BZHI:
      CalculcateFlags_BZHI(
        CurrentDeferredFlags.SrcSize,
        CurrentDeferredFlags.Res,
        CurrentDeferredFlags.Sources.OneSource.Src1);
      break;
    case FlagsGenerationType::TYPE_TZCNT:
      CalculcateFlags_TZCNT(CurrentDeferredFlags.Res);
      break;
    case FlagsGenerationType::TYPE_LZCNT:
      CalculcateFlags_LZCNT(
        CurrentDeferredFlags.SrcSize,
        CurrentDeferredFlags.Res);
      break;
    case FlagsGenerationType::TYPE_BITSELECT:
      CalculcateFlags_BITSELECT(CurrentDeferredFlags.Res);
      break;
    case FlagsGenerationType::TYPE_RDRAND:
      CalculcateFlags_RDRAND(CurrentDeferredFlags.Res);
      break;
    case FlagsGenerationType::TYPE_NONE:
    default: ERROR_AND_DIE_FMT("Unhandled flags type {}", CurrentDeferredFlags.Type);
  }

  // Done calculating
  CurrentDeferredFlags.Type = FlagsGenerationType::TYPE_NONE;
}

void OpDispatchBuilder::CalculcateFlags_ADC(uint8_t SrcSize, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2, OrderedNode *CF) {
  auto Size = SrcSize * 8;
  // AF
  {
    OrderedNode *AFRes = _Xor(_Xor(Src1, Src2), Res);
    AFRes = _Bfe(1, 4, AFRes);
    SetRFLAG<FEXCore::X86State::RFLAG_AF_LOC>(AFRes);
  }

  // SF
  {
    auto SignBitConst = _Constant(Size - 1);

    auto LshrOp = _Lshr(Res, SignBitConst);
    SetRFLAG<FEXCore::X86State::RFLAG_SF_LOC>(LshrOp);
  }

  // PF
  if (!CTX->Config.ABINoPF) {
    auto PopCountOp = _Popcount(_And(Res, _Constant(0xFF)));

    auto XorOp = _Xor(PopCountOp, _Constant(1));
    SetRFLAG<FEXCore::X86State::RFLAG_PF_LOC>(XorOp);
  } else {
    _InvalidateFlags(1UL << FEXCore::X86State::RFLAG_PF_LOC);
  }

  // ZF
  {
    auto SelectOp = _Select(FEXCore::IR::COND_EQ,
        Res, _Constant(0), _Constant(1), _Constant(0));
    SetRFLAG<FEXCore::X86State::RFLAG_ZF_LOC>(SelectOp);
  }

  // CF
  // Unsigned
  {
    auto SelectOpLT = _Select(FEXCore::IR::COND_ULT, Res, Src2, _Constant(1), _Constant(0));
    auto SelectOpLE = _Select(FEXCore::IR::COND_ULE, Res, Src2, _Constant(1), _Constant(0));
    auto SelectCF   = _Select(FEXCore::IR::COND_EQ, CF, _Constant(1), SelectOpLE, SelectOpLT);
    SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(SelectCF);
  }

  // OF
  // Signed
  {
    auto NegOne = _Constant(~0ULL);
    auto XorOp1 = _Xor(_Xor(Src1, Src2), NegOne);
    auto XorOp2 = _Xor(Res, Src1);
    OrderedNode *AndOp1 = _And(XorOp1, XorOp2);

    switch (Size) {
    case 8:
      AndOp1 = _Bfe(1, 7, AndOp1);
    break;
    case 16:
      AndOp1 = _Bfe(1, 15, AndOp1);
    break;
    case 32:
      AndOp1 = _Bfe(1, 31, AndOp1);
    break;
    case 64:
      AndOp1 = _Bfe(1, 63, AndOp1);
    break;
    default:
      LOGMAN_MSG_A_FMT("Unknown BFE size: {}", Size);
    break;
    }
    SetRFLAG<FEXCore::X86State::RFLAG_OF_LOC>(AndOp1);
  }
}

void OpDispatchBuilder::CalculcateFlags_SBB(uint8_t SrcSize, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2, OrderedNode *CF) {
  // AF
  {
    OrderedNode *AFRes = _Xor(_Xor(Src1, Src2), Res);
    AFRes = _Bfe(1, 4, AFRes);
    SetRFLAG<FEXCore::X86State::RFLAG_AF_LOC>(AFRes);
  }

  // SF
  {
    auto SignBitConst = _Constant(SrcSize * 8 - 1);

    auto LshrOp = _Lshr(Res, SignBitConst);
    SetRFLAG<FEXCore::X86State::RFLAG_SF_LOC>(LshrOp);
  }

  // PF
  if (!CTX->Config.ABINoPF) {
    auto PopCountOp = _Popcount(_And(Res, _Constant(0xFF)));

    auto XorOp = _Xor(PopCountOp, _Constant(1));
    SetRFLAG<FEXCore::X86State::RFLAG_PF_LOC>(XorOp);
  } else {
    _InvalidateFlags(1UL << FEXCore::X86State::RFLAG_PF_LOC);
  }

  // ZF
  {
    auto SelectOp = _Select(FEXCore::IR::COND_EQ,
        Res, _Constant(0), _Constant(1), _Constant(0));
    SetRFLAG<FEXCore::X86State::RFLAG_ZF_LOC>(SelectOp);
  }

  // CF
  // Unsigned
  {
    auto SelectOpLT = _Select(FEXCore::IR::COND_UGT, Res, Src1, _Constant(1), _Constant(0));
    auto SelectOpLE = _Select(FEXCore::IR::COND_UGE, Res, Src1, _Constant(1), _Constant(0));
    auto SelectCF   = _Select(FEXCore::IR::COND_EQ, CF, _Constant(1), SelectOpLE, SelectOpLT);
    SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(SelectCF);
  }

  // OF
  // Signed
  {
    auto XorOp1 = _Xor(Src1, Src2);
    auto XorOp2 = _Xor(Res, Src1);
    OrderedNode *AndOp1 = _And(XorOp1, XorOp2);

    switch (SrcSize) {
    case 1:
      AndOp1 = _Bfe(1, 7, AndOp1);
    break;
    case 2:
      AndOp1 = _Bfe(1, 15, AndOp1);
    break;
    case 4:
      AndOp1 = _Bfe(1, 31, AndOp1);
    break;
    case 8:
      AndOp1 = _Bfe(1, 63, AndOp1);
    break;
    default:
      LOGMAN_MSG_A_FMT("Unknown BFE size: {}", SrcSize);
    break;
    }
    SetRFLAG<FEXCore::X86State::RFLAG_OF_LOC>(AndOp1);
  }
}

void OpDispatchBuilder::CalculcateFlags_SUB(uint8_t SrcSize, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2, bool UpdateCF) {
  // AF
  {
    OrderedNode *AFRes = _Xor(_Xor(Src1, Src2), Res);
    AFRes = _Bfe(1, 4, AFRes);
    SetRFLAG<FEXCore::X86State::RFLAG_AF_LOC>(AFRes);
  }

  // SF
  {
    auto SignBitConst = _Constant(SrcSize * 8 - 1);

    auto LshrOp = _Lshr(Res, SignBitConst);
    SetRFLAG<FEXCore::X86State::RFLAG_SF_LOC>(LshrOp);
  }

  // PF
  if (!CTX->Config.ABINoPF) {
    auto EightBitMask = _Constant(0xFF);
    auto PopCountOp = _Popcount(_And(Res, EightBitMask));
    auto XorOp = _Xor(PopCountOp, _Constant(1));
    SetRFLAG<FEXCore::X86State::RFLAG_PF_LOC>(XorOp);
  } else {
    _InvalidateFlags(1UL << FEXCore::X86State::RFLAG_PF_LOC);
  }

  // ZF
  {
    auto ZeroConst = _Constant(0);
    auto OneConst = _Constant(1);
    auto SelectOp = _Select(FEXCore::IR::COND_EQ,
        Res, ZeroConst, OneConst, ZeroConst);
    SetRFLAG<FEXCore::X86State::RFLAG_ZF_LOC>(SelectOp);
  }

  // CF
  if (UpdateCF) {
    auto ZeroConst = _Constant(0);
    auto OneConst = _Constant(1);

    auto SelectOp = _Select(FEXCore::IR::COND_ULT,
        Src1, Src2, OneConst, ZeroConst);

    SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(SelectOp);
  }
  // OF
  {
    auto XorOp1 = _Xor(Src1, Src2);
    auto XorOp2 = _Xor(Res, Src1);
    OrderedNode *FinalAnd = _And(XorOp1, XorOp2);

    FinalAnd = _Bfe(1, SrcSize * 8 - 1, FinalAnd);

    SetRFLAG<FEXCore::X86State::RFLAG_OF_LOC>(FinalAnd);
  }
}

void OpDispatchBuilder::CalculcateFlags_ADD(uint8_t SrcSize, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2, bool UpdateCF) {
  // AF
  {
    OrderedNode *AFRes = _Xor(_Xor(Src1, Src2), Res);
    AFRes = _Bfe(1, 4, AFRes);
    SetRFLAG<FEXCore::X86State::RFLAG_AF_LOC>(AFRes);
  }

  // SF
  {
    auto SignBitConst = _Constant(SrcSize * 8 - 1);

    auto LshrOp = _Lshr(Res, SignBitConst);
    SetRFLAG<FEXCore::X86State::RFLAG_SF_LOC>(LshrOp);
  }

  // PF
  if (!CTX->Config.ABINoPF) {
    auto EightBitMask = _Constant(0xFF);
    auto PopCountOp = _Popcount(_And(Res, EightBitMask));
    auto XorOp = _Xor(PopCountOp, _Constant(1));
    SetRFLAG<FEXCore::X86State::RFLAG_PF_LOC>(XorOp);
  } else {
    _InvalidateFlags(1UL << FEXCore::X86State::RFLAG_PF_LOC);
  }

  // ZF
  {
    auto SelectOp = _Select(FEXCore::IR::COND_EQ,
        Res, _Constant(0), _Constant(1), _Constant(0));
    SetRFLAG<FEXCore::X86State::RFLAG_ZF_LOC>(SelectOp);
  }
  // CF
  if (UpdateCF) {
    auto SelectOp = _Select(FEXCore::IR::COND_ULT, Res, Src2, _Constant(1), _Constant(0));

    SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(SelectOp);
  }

  // OF
  {
    auto NegOne = _Constant(~0ULL);
    auto XorOp1 = _Xor(_Xor(Src1, Src2), NegOne);
    auto XorOp2 = _Xor(Res, Src1);

    OrderedNode *AndOp1 = _And(XorOp1, XorOp2);

    switch (SrcSize) {
    case 1:
      AndOp1 = _Bfe(1, 7, AndOp1);
    break;
    case 2:
      AndOp1 = _Bfe(1, 15, AndOp1);
    break;
    case 4:
      AndOp1 = _Bfe(1, 31, AndOp1);
    break;
    case 8:
      AndOp1 = _Bfe(1, 63, AndOp1);
    break;
    default:
      LOGMAN_MSG_A_FMT("Unknown BFE size: {}", SrcSize);
    break;
    }
    SetRFLAG<FEXCore::X86State::RFLAG_OF_LOC>(AndOp1);
  }
}

void OpDispatchBuilder::CalculcateFlags_MUL(uint8_t SrcSize, OrderedNode *Res, OrderedNode *High) {
  // PF/AF/ZF/SF
  // Undefined
  {
    SetRFLAG<FEXCore::X86State::RFLAG_PF_LOC>(_Constant(0));
    SetRFLAG<FEXCore::X86State::RFLAG_AF_LOC>(_Constant(0));
    SetRFLAG<FEXCore::X86State::RFLAG_ZF_LOC>(_Constant(0));
    SetRFLAG<FEXCore::X86State::RFLAG_SF_LOC>(_Constant(0));
  }

  // CF/OF
  {
    // CF and OF are set if the result of the operation can't be fit in to the destination register
    // If the value can fit then the top bits will be zero

    auto SignBit = _Sbfe(1, SrcSize * 8 - 1, Res);

    auto SelectOp = _Select(FEXCore::IR::COND_EQ, High, SignBit, _Constant(0), _Constant(1));

    SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(SelectOp);
    SetRFLAG<FEXCore::X86State::RFLAG_OF_LOC>(SelectOp);
  }
}

void OpDispatchBuilder::CalculcateFlags_UMUL(OrderedNode *High) {
  // AF/SF/PF/ZF
  // Undefined
  {
    SetRFLAG<FEXCore::X86State::RFLAG_AF_LOC>(_Constant(0));
    SetRFLAG<FEXCore::X86State::RFLAG_SF_LOC>(_Constant(0));
    SetRFLAG<FEXCore::X86State::RFLAG_PF_LOC>(_Constant(0));
    SetRFLAG<FEXCore::X86State::RFLAG_ZF_LOC>(_Constant(0));
  }

  // CF/OF
  {
    // CF and OF are set if the result of the operation can't be fit in to the destination register
    // The result register will be all zero if it can't fit due to how multiplication behaves

    auto SelectOp = _Select(FEXCore::IR::COND_EQ, High, _Constant(0), _Constant(0), _Constant(1));

    SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(SelectOp);
    SetRFLAG<FEXCore::X86State::RFLAG_OF_LOC>(SelectOp);
  }
}

void OpDispatchBuilder::CalculcateFlags_Logical(uint8_t SrcSize, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2) {
  // AF
  {
    // Undefined
    // Set to zero anyway
    SetRFLAG<FEXCore::X86State::RFLAG_AF_LOC>(_Constant(0));
  }

  // SF
  {
    auto SignBitConst = _Constant(SrcSize * 8 - 1);

    auto LshrOp = _Lshr(Res, SignBitConst);
    SetRFLAG<FEXCore::X86State::RFLAG_SF_LOC>(LshrOp);
  }

  // PF
  if (!CTX->Config.ABINoPF) {
    auto EightBitMask = _Constant(0xFF);
    auto PopCountOp = _Popcount(_And(Res, EightBitMask));
    auto XorOp = _Xor(PopCountOp, _Constant(1));
    SetRFLAG<FEXCore::X86State::RFLAG_PF_LOC>(XorOp);
  } else {
    _InvalidateFlags(1UL << FEXCore::X86State::RFLAG_PF_LOC);
  }

  // ZF
  {
    auto SelectOp = _Select(FEXCore::IR::COND_EQ,
        Res, _Constant(0), _Constant(1), _Constant(0));
    SetRFLAG<FEXCore::X86State::RFLAG_ZF_LOC>(SelectOp);
  }

  // CF/OF
  {
    SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(_Constant(0));
    SetRFLAG<FEXCore::X86State::RFLAG_OF_LOC>(_Constant(0));
  }
}

#define COND_FLAG_SET(cond, flag, newflag) \
auto oldflag = GetRFLAG(FEXCore::X86State::flag);\
auto newval = _Select(FEXCore::IR::COND_EQ, cond, _Constant(0), oldflag, newflag);\
SetRFLAG<FEXCore::X86State::flag>(newval);

void OpDispatchBuilder::CalculcateFlags_ShiftLeft(uint8_t SrcSize, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2) {
  // CF
  {
    // Extract the last bit shifted in to CF
    auto Size = _Constant(SrcSize * 8);
    auto ShiftAmt = _Sub(Size, Src2);
    auto LastBit = _And(_Lshr(Src1, ShiftAmt), _Constant(1));
    COND_FLAG_SET(Src2, RFLAG_CF_LOC, LastBit);
  }

  // PF
  if (!CTX->Config.ABINoPF) {
    auto EightBitMask = _Constant(0xFF);
    auto PopCountOp = _Popcount(_And(Res, EightBitMask));
    auto XorOp = _Xor(PopCountOp, _Constant(1));
    COND_FLAG_SET(Src2, RFLAG_PF_LOC, XorOp);
  } else {
    _InvalidateFlags(1UL << FEXCore::X86State::RFLAG_PF_LOC);
  }

  // AF
  {
    // Undefined
    // Set to zero anyway
    COND_FLAG_SET(Src2, RFLAG_AF_LOC, _Constant(0));
  }

  // ZF
  {
    auto SelectOp = _Select(FEXCore::IR::COND_EQ,
        Res, _Constant(0), _Constant(1), _Constant(0));
    COND_FLAG_SET(Src2, RFLAG_ZF_LOC, SelectOp);
  }

  // SF
  {
    auto val = _Bfe(1, SrcSize * 8 - 1, Res);
    COND_FLAG_SET(Src2, RFLAG_SF_LOC, val);
  }

  // OF
  {
    // In the case of left shift. OF is only set from the result of <Top Source Bit> XOR <Top Result Bit>
    // When Shift > 1 then OF is undefined
    auto val = _Bfe(1, SrcSize * 8 - 1, _Xor(Src1, Res));
    COND_FLAG_SET(Src2, RFLAG_OF_LOC, val);
  }
}

void OpDispatchBuilder::CalculcateFlags_ShiftRight(uint8_t SrcSize, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2) {
  // CF
  {
    // Extract the last bit shifted in to CF
    auto ShiftAmt = _Sub(Src2, _Constant(1));
    auto LastBit = _And(_Lshr(Src1, ShiftAmt), _Constant(1));
    COND_FLAG_SET(Src2, RFLAG_CF_LOC, LastBit);
  }

  // PF
  if (!CTX->Config.ABINoPF) {
    auto EightBitMask = _Constant(0xFF);
    auto PopCountOp = _Popcount(_And(Res, EightBitMask));
    auto XorOp = _Xor(PopCountOp, _Constant(1));
    COND_FLAG_SET(Src2, RFLAG_PF_LOC, XorOp);
  } else {
    _InvalidateFlags(1UL << FEXCore::X86State::RFLAG_PF_LOC);
  }

  // AF
  {
    // Undefined
    // Set to zero anyway
    COND_FLAG_SET(Src2, RFLAG_AF_LOC, _Constant(0));
  }

  // ZF
  {
    auto SelectOp = _Select(FEXCore::IR::COND_EQ,
        Res, _Constant(0), _Constant(1), _Constant(0));
    COND_FLAG_SET(Src2, RFLAG_ZF_LOC, SelectOp);
  }

  // SF
  {
    auto val =_Bfe(1, SrcSize * 8 - 1, Res);
    COND_FLAG_SET(Src2, RFLAG_SF_LOC, val);
  }

  // OF
  {
    // Only defined when Shift is 1 else undefined
    // OF flag is set if a sign change occurred
    auto val = _Bfe(1, SrcSize * 8 - 1, _Xor(Src1, Res));
    COND_FLAG_SET(Src2, RFLAG_OF_LOC, val);
  }
}

void OpDispatchBuilder::CalculcateFlags_SignShiftRight(uint8_t SrcSize, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2) {
  // CF
  {
    // Extract the last bit shifted in to CF
    auto ShiftAmt = _Sub(Src2, _Constant(1));
    auto LastBit = _And(_Lshr(Src1, ShiftAmt), _Constant(1));
    COND_FLAG_SET(Src2, RFLAG_CF_LOC, LastBit);
  }

  // PF
  if (!CTX->Config.ABINoPF) {
    auto EightBitMask = _Constant(0xFF);
    auto PopCountOp = _Popcount(_And(Res, EightBitMask));
    auto XorOp = _Xor(PopCountOp, _Constant(1));
    COND_FLAG_SET(Src2, RFLAG_PF_LOC, XorOp);
  } else {
    _InvalidateFlags(1UL << FEXCore::X86State::RFLAG_PF_LOC);
  }

  // AF
  {
    // Undefined
    // Set to zero anyway
    COND_FLAG_SET(Src2, RFLAG_AF_LOC, _Constant(0));
  }

  // ZF
  {
    auto SelectOp = _Select(FEXCore::IR::COND_EQ,
        Res, _Constant(0), _Constant(1), _Constant(0));
    COND_FLAG_SET(Src2, RFLAG_ZF_LOC, SelectOp);
  }

  // SF
  {
    auto SignBitConst = _Constant(SrcSize * 8 - 1);

    auto LshrOp = _Lshr(Res, SignBitConst);
    COND_FLAG_SET(Src2, RFLAG_SF_LOC, LshrOp);
  }

  // OF
  {
    COND_FLAG_SET(Src2, RFLAG_OF_LOC, _Constant(0));
  }
}

void OpDispatchBuilder::CalculcateFlags_ShiftLeftImmediate(uint8_t SrcSize, OrderedNode *Res, OrderedNode *Src1, uint64_t Shift) {
  // No flags changed if shift is zero
  if (Shift == 0) return;

  // CF
  {
    // Extract the last bit shifted in to CF
    auto OpSize = SrcSize * 8;
    if (SrcSize * 8 < Shift) {
      Shift &= (OpSize - 1);
    }
    SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(_Bfe(1, OpSize - Shift, Src1));
  }

  // PF
  if (!CTX->Config.ABINoPF) {
    auto EightBitMask = _Constant(0xFF);
    auto PopCountOp = _Popcount(_And(Res, EightBitMask));
    auto XorOp = _Xor(PopCountOp, _Constant(1));
    SetRFLAG<FEXCore::X86State::RFLAG_PF_LOC>(XorOp);
  } else {
    _InvalidateFlags(1UL << FEXCore::X86State::RFLAG_PF_LOC);
  }

  // AF
  {
    // Undefined
    // Set to zero anyway
    SetRFLAG<FEXCore::X86State::RFLAG_AF_LOC>(_Constant(0));
  }

  // ZF
  {
    auto SelectOp = _Select(FEXCore::IR::COND_EQ,
        Res, _Constant(0), _Constant(1), _Constant(0));
    SetRFLAG<FEXCore::X86State::RFLAG_ZF_LOC>(SelectOp);
  }

  // SF
  {
    auto LshrOp = _Bfe(1, SrcSize * 8 - 1, Res);

    SetRFLAG<FEXCore::X86State::RFLAG_SF_LOC>(LshrOp);

    // OF
    // In the case of left shift. OF is only set from the result of <Top Source Bit> XOR <Top Result Bit>
    if (Shift == 1) {
      auto SourceBit = _Bfe(1, SrcSize * 8 - 1, Src1);
      SetRFLAG<FEXCore::X86State::RFLAG_OF_LOC>(_Xor(SourceBit, LshrOp));
    }
  }
}

void OpDispatchBuilder::CalculcateFlags_SignShiftRightImmediate(uint8_t SrcSize, OrderedNode *Res, OrderedNode *Src1, uint64_t Shift) {
  // No flags changed if shift is zero
  if (Shift == 0) return;

  // CF
  {
    // Extract the last bit shifted in to CF
    SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(_Bfe(1, Shift-1, Src1));
  }

  // PF
  if (!CTX->Config.ABINoPF) {
    auto EightBitMask = _Constant(0xFF);
    auto PopCountOp = _Popcount(_And(Res, EightBitMask));
    auto XorOp = _Xor(PopCountOp, _Constant(1));
    SetRFLAG<FEXCore::X86State::RFLAG_PF_LOC>(XorOp);
  } else {
    _InvalidateFlags(1UL << FEXCore::X86State::RFLAG_PF_LOC);
  }

  // AF
  {
    // Undefined
    // Set to zero anyway
    SetRFLAG<FEXCore::X86State::RFLAG_AF_LOC>(_Constant(0));
  }

  // ZF
  {
    auto SelectOp = _Select(FEXCore::IR::COND_EQ,
        Res, _Constant(0), _Constant(1), _Constant(0));
    SetRFLAG<FEXCore::X86State::RFLAG_ZF_LOC>(SelectOp);
  }

  // SF
  {
    auto SignBitConst = _Constant(SrcSize * 8 - 1);

    auto LshrOp = _Lshr(Res, SignBitConst);
    SetRFLAG<FEXCore::X86State::RFLAG_SF_LOC>(LshrOp);

    // OF
    // Only defined when Shift is 1 else undefined
    // Only is set if the top bit was set to 1 when shifted
    // So it is set to same value as SF
    if (Shift == 1) {
      SetRFLAG<FEXCore::X86State::RFLAG_OF_LOC>(_Constant(0));
    }
  }
}

void OpDispatchBuilder::CalculcateFlags_ShiftRightImmediate(uint8_t SrcSize, OrderedNode *Res, OrderedNode *Src1, uint64_t Shift) {
  // No flags changed if shift is zero
  if (Shift == 0) return;

  // CF
  {
    // Extract the last bit shifted in to CF
    SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(_Bfe(1, Shift-1, Src1));
  }

  // PF
  if (!CTX->Config.ABINoPF) {
    auto EightBitMask = _Constant(0xFF);
    auto PopCountOp = _Popcount(_And(Res, EightBitMask));
    auto XorOp = _Xor(PopCountOp, _Constant(1));
    SetRFLAG<FEXCore::X86State::RFLAG_PF_LOC>(XorOp);
  } else {
    _InvalidateFlags(1UL << FEXCore::X86State::RFLAG_PF_LOC);
  }

  // AF
  {
    // Undefined
    // Set to zero anyway
    SetRFLAG<FEXCore::X86State::RFLAG_AF_LOC>(_Constant(0));
  }

  // ZF
  {
    auto SelectOp = _Select(FEXCore::IR::COND_EQ,
        Res, _Constant(0), _Constant(1), _Constant(0));
    SetRFLAG<FEXCore::X86State::RFLAG_ZF_LOC>(SelectOp);
  }

  // SF
  {
    auto SignBitConst = _Constant(SrcSize * 8 - 1);

    auto LshrOp = _Lshr(Res, SignBitConst);
    SetRFLAG<FEXCore::X86State::RFLAG_SF_LOC>(LshrOp);
  }

  // OF
  {
    // Only defined when Shift is 1 else undefined
    // Is set to the MSB of the original value
    if (Shift == 1) {
      SetRFLAG<FEXCore::X86State::RFLAG_OF_LOC>(_Bfe(1, SrcSize * 8 - 1, Src1));
    }
  }
}

void OpDispatchBuilder::CalculcateFlags_RotateRight(uint8_t SrcSize, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2) {
  auto OpSize = SrcSize * 8;

  // Extract the last bit shifted in to CF
  auto NewCF = _Bfe(1, OpSize - 1, Res);

  // CF
  {
    auto OldCF = GetRFLAG(FEXCore::X86State::RFLAG_CF_LOC);
    auto CF = _Select(FEXCore::IR::COND_EQ, Src2, _Constant(0), OldCF, NewCF);

    // Extract the last bit shifted in to CF
    SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(CF);
  }

  // OF
  {
    auto OldOF = GetRFLAG(FEXCore::X86State::RFLAG_OF_LOC);

    // OF is set to the XOR of the new CF bit and the most significant bit of the result
    auto NewOF = _Xor(_Bfe(1, OpSize - 2, Res), NewCF);

    // If shift == 0, don't update flags
    auto OF = _Select(FEXCore::IR::COND_EQ, Src2, _Constant(0), OldOF, NewOF);

    SetRFLAG<FEXCore::X86State::RFLAG_OF_LOC>(OF);
  }
}

void OpDispatchBuilder::CalculcateFlags_RotateLeft(uint8_t SrcSize, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2) {
  auto OpSize = SrcSize * 8;

  // Extract the last bit shifted in to CF
  //auto Size = _Constant(GetSrcSize(Res) * 8);
  //auto ShiftAmt = _Sub(Size, Src2);
  auto NewCF = _Bfe(1, 0, Res);

  // CF
  {
    auto OldCF = GetRFLAG(FEXCore::X86State::RFLAG_CF_LOC);
    auto CF = _Select(FEXCore::IR::COND_EQ, Src2, _Constant(0), OldCF, NewCF);

    // Extract the last bit shifted in to CF
    SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(CF);
  }

  // OF
  {
    auto OldOF = GetRFLAG(FEXCore::X86State::RFLAG_OF_LOC);
    // OF is set to the XOR of the new CF bit and the most significant bit of the result
    auto NewOF = _Xor(_Bfe(1, OpSize - 1, Res), NewCF);

    auto OF = _Select(FEXCore::IR::COND_EQ, Src2, _Constant(0), OldOF, NewOF);

    // If shift == 0, don't update flags
    SetRFLAG<FEXCore::X86State::RFLAG_OF_LOC>(OF);
  }
}

void OpDispatchBuilder::CalculcateFlags_RotateRightImmediate(uint8_t SrcSize, OrderedNode *Res, OrderedNode *Src1, uint64_t Shift) {
  if (Shift == 0) return;

  auto OpSize = SrcSize * 8;
  if (SrcSize * 8 < Shift) {
    Shift &= (OpSize - 1);
  }
  auto NewCF = _Bfe(1, OpSize - Shift, Src1);

  // CF
  {
    // Extract the last bit shifted in to CF
    SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(NewCF);
  }

  // OF
  {
    if (Shift == 1) {
      // OF is set to the XOR of the new CF bit and the most significant bit of the result
      SetRFLAG<FEXCore::X86State::RFLAG_OF_LOC>(_Xor(_Bfe(1, OpSize - 1, Res), NewCF));
    }
  }
}

void OpDispatchBuilder::CalculcateFlags_RotateLeftImmediate(uint8_t SrcSize, OrderedNode *Res, OrderedNode *Src1, uint64_t Shift) {
  if (Shift == 0) return;

  auto OpSize = SrcSize * 8;

  // CF
  {
    // Extract the last bit shifted in to CF
    SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(_Bfe(1, Shift, Src1));
  }

  // OF
  {
    if (Shift == 1) {
      // OF is the top two MSBs XOR'd together
      SetRFLAG<FEXCore::X86State::RFLAG_OF_LOC>(_Xor(_Bfe(1, OpSize - 1, Src1), _Bfe(1, OpSize - 2, Src1)));
    }
  }
}

void OpDispatchBuilder::CalculcateFlags_FCMP(uint8_t SrcSize, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2) {
  OrderedNode *HostFlag_CF = _GetHostFlag(Res, FCMP_FLAG_LT);
  OrderedNode *HostFlag_ZF = _GetHostFlag(Res, FCMP_FLAG_EQ);
  OrderedNode *HostFlag_Unordered  = _GetHostFlag(Res, FCMP_FLAG_UNORDERED);

  SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(HostFlag_CF);
  SetRFLAG<FEXCore::X86State::RFLAG_ZF_LOC>(HostFlag_ZF);
  SetRFLAG<FEXCore::X86State::RFLAG_PF_LOC>(HostFlag_Unordered);

  auto ZeroConst = _Constant(0);
  SetRFLAG<FEXCore::X86State::RFLAG_AF_LOC>(ZeroConst);
  SetRFLAG<FEXCore::X86State::RFLAG_SF_LOC>(ZeroConst);
  SetRFLAG<FEXCore::X86State::RFLAG_OF_LOC>(ZeroConst);
}

void OpDispatchBuilder::CalculcateFlags_BEXTR(OrderedNode *Src) {
  // Handle flag setting.
  //
  // All that matters primarily for this instruction is
  // that we only set the ZF flag properly.
  //
  // CF and OF are defined as being set to zero
  //
  SetRFLAG<X86State::RFLAG_CF_LOC>(_Constant(0));
  SetRFLAG<X86State::RFLAG_OF_LOC>(_Constant(0));

  // Every other flag is considered undefined after a
  // BEXTR instruction, but we opt to reliably clear them.
  //
  SetRFLAG<X86State::RFLAG_AF_LOC>(_Constant(0));
  SetRFLAG<X86State::RFLAG_SF_LOC>(_Constant(0));

  // PF
  if (CTX->Config.ABINoPF) {
    _InvalidateFlags(1UL << X86State::RFLAG_PF_LOC);
  } else {
    SetRFLAG<X86State::RFLAG_PF_LOC>(_Constant(0));
  }

  // ZF
  auto ZeroOp = _Select(IR::COND_EQ,
                        Src, _Constant(0),
                        _Constant(1), _Constant(0));
  SetRFLAG<X86State::RFLAG_ZF_LOC>(ZeroOp);
}

void OpDispatchBuilder::CalculcateFlags_BLSI(uint8_t SrcSize, OrderedNode *Src) {
  // Now for the flags:
  //
  // Only CF, SF, ZF and OF are defined as being updated
  // CF is cleared if Src is zero, otherwise it's set.
  // SF is set to the value of the most significant operand bit of Result.
  // OF is always cleared
  // ZF is set, as usual, if Result is zero or not.
  //
  // AF and PF are documented as being in an undefined state after
  // a BLSI operation, however, we choose to reliably clear them.

  auto Zero = _Constant(0);
  auto One = _Constant(1);

  SetRFLAG<X86State::RFLAG_OF_LOC>(Zero);
  SetRFLAG<X86State::RFLAG_AF_LOC>(Zero);
  if (CTX->Config.ABINoPF) {
    _InvalidateFlags(1UL << X86State::RFLAG_PF_LOC);
  } else {
    SetRFLAG<X86State::RFLAG_PF_LOC>(Zero);
  }

  // ZF
  {
    auto ZFOp = _Select(IR::COND_EQ,
                        Src, Zero,
                        One, Zero);
    SetRFLAG<X86State::RFLAG_ZF_LOC>(ZFOp);
  }

  // CF
  {
    auto CFOp = _Select(IR::COND_EQ,
                        Src, Zero,
                        Zero, One);
    SetRFLAG<X86State::RFLAG_CF_LOC>(CFOp);
  }

  // SF
  {
    auto SignBit = _Constant(SrcSize * 8 - 1);
    auto SFOp = _Lshr(Src, SignBit);

    SetRFLAG<X86State::RFLAG_SF_LOC>(SFOp);
  }
}

void OpDispatchBuilder::CalculcateFlags_BLSMSK(OrderedNode *Src) {
  // Now for the flags.

  auto Zero = _Constant(0);
  auto One = _Constant(1);

  SetRFLAG<X86State::RFLAG_ZF_LOC>(Zero);
  SetRFLAG<X86State::RFLAG_OF_LOC>(Zero);
  SetRFLAG<X86State::RFLAG_AF_LOC>(Zero);
  if (CTX->Config.ABINoPF) {
    _InvalidateFlags(1UL << X86State::RFLAG_PF_LOC);
  } else {
    SetRFLAG<X86State::RFLAG_PF_LOC>(Zero);
  }

  auto CFOp = _Select(IR::COND_EQ,
                      Src, Zero,
                      Zero, One);
  SetRFLAG<X86State::RFLAG_CF_LOC>(CFOp);
}

void OpDispatchBuilder::CalculcateFlags_BLSR(uint8_t SrcSize, OrderedNode *Result, OrderedNode *Src) {
  // Now for flags.
  auto Zero = _Constant(0);
  auto One = _Constant(1);

  SetRFLAG<X86State::RFLAG_OF_LOC>(Zero);
  SetRFLAG<X86State::RFLAG_AF_LOC>(Zero);
  if (CTX->Config.ABINoPF) {
    _InvalidateFlags(1UL << X86State::RFLAG_PF_LOC);
  } else {
    SetRFLAG<X86State::RFLAG_PF_LOC>(Zero);
  }

  // ZF
  {
    auto ZFOp = _Select(IR::COND_EQ,
                        Result, Zero,
                        One, Zero);
    SetRFLAG<X86State::RFLAG_ZF_LOC>(ZFOp);
  }

  // CF
  {
    auto CFOp = _Select(IR::COND_EQ,
                        Src, Zero,
                        Zero, One);
    SetRFLAG<X86State::RFLAG_CF_LOC>(CFOp);
  }

  // SF
  {
    auto SignBit = _Constant(SrcSize * 8 - 1);
    auto SFOp = _Lshr(Result, SignBit);

    SetRFLAG<X86State::RFLAG_SF_LOC>(SFOp);
  }
}

void OpDispatchBuilder::CalculcateFlags_POPCOUNT(OrderedNode *Src) {
  // Set ZF
  auto Zero = _Constant(0);
  auto ZFResult = _Select(FEXCore::IR::COND_EQ,
      Src,  Zero,
      _Constant(1), Zero);

  // Set flags
  SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(Zero);
  SetRFLAG<FEXCore::X86State::RFLAG_PF_LOC>(Zero);
  SetRFLAG<FEXCore::X86State::RFLAG_AF_LOC>(Zero);
  SetRFLAG<FEXCore::X86State::RFLAG_ZF_LOC>(ZFResult);
  SetRFLAG<FEXCore::X86State::RFLAG_SF_LOC>(Zero);
  SetRFLAG<FEXCore::X86State::RFLAG_OF_LOC>(Zero);
}

void OpDispatchBuilder::CalculcateFlags_BZHI(uint8_t SrcSize, OrderedNode *Result, OrderedNode *Src) {
  // Now for the flags

  auto Bounds = _Constant(SrcSize * 8- 1);
  auto Zero = _Constant(0);
  auto One = _Constant(1);

  SetRFLAG<X86State::RFLAG_OF_LOC>(Zero);
  SetRFLAG<X86State::RFLAG_AF_LOC>(Zero);
  if (CTX->Config.ABINoPF) {
    _InvalidateFlags(1UL << X86State::RFLAG_PF_LOC);
  } else {
    SetRFLAG<X86State::RFLAG_PF_LOC>(Zero);
  }

  // ZF
  {
    auto ZFOp = _Select(IR::COND_EQ,
                        Result, Zero,
                        One, Zero);
    SetRFLAG<X86State::RFLAG_ZF_LOC>(ZFOp);
  }

  // CF
  {
    auto CFOp = _Select(IR::COND_UGT,
                        Src, Bounds,
                        One, Zero);
    SetRFLAG<X86State::RFLAG_CF_LOC>(CFOp);
  }

  // SF
  {
    auto SFOp = _Lshr(Result, Bounds);

    SetRFLAG<X86State::RFLAG_SF_LOC>(SFOp);
  }
}

void OpDispatchBuilder::CalculcateFlags_TZCNT(OrderedNode *Src) {
  // OF, SF, AF, PF all undefined
  auto Zero = _Constant(0);
  auto ZFResult = _Select(FEXCore::IR::COND_EQ,
      Src,  Zero,
      _Constant(1), Zero);

  // Set flags
  SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(ZFResult);
  SetRFLAG<FEXCore::X86State::RFLAG_ZF_LOC>(_Bfe(1, 0, Src));
}

void OpDispatchBuilder::CalculcateFlags_LZCNT(uint8_t SrcSize, OrderedNode *Src) {
  // OF, SF, AF, PF all undefined

  auto Zero = _Constant(0);
  auto ZFResult = _Select(FEXCore::IR::COND_EQ,
      Src,  Zero,
      _Constant(1), Zero);

  // Set flags
  SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(ZFResult);
  SetRFLAG<FEXCore::X86State::RFLAG_ZF_LOC>(_Bfe(1, SrcSize * 8 - 1, Src));
}

void OpDispatchBuilder::CalculcateFlags_BITSELECT(OrderedNode *Src) {
  // OF, SF, AF, PF, CF all undefined

  auto ZeroConst = _Constant(0);
  auto OneConst = _Constant(1);

  // ZF is set to 1 if the source was zero
  auto ZFSelectOp = _Select(FEXCore::IR::COND_EQ,
      Src, ZeroConst,
      OneConst, ZeroConst);

  SetRFLAG<FEXCore::X86State::RFLAG_ZF_LOC>(ZFSelectOp);
}

void OpDispatchBuilder::CalculcateFlags_RDRAND(OrderedNode *Src) {
  // OF, SF, ZF, AF, PF all zero
  // CF is set to the incoming source

  auto ZeroConst = _Constant(0);
  SetRFLAG<X86State::RFLAG_OF_LOC>(ZeroConst);
  SetRFLAG<X86State::RFLAG_SF_LOC>(ZeroConst);
  SetRFLAG<X86State::RFLAG_ZF_LOC>(ZeroConst);
  SetRFLAG<X86State::RFLAG_AF_LOC>(ZeroConst);
  SetRFLAG<X86State::RFLAG_PF_LOC>(ZeroConst);
  SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(Src);
}

}
