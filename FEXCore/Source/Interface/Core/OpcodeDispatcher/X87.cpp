// SPDX-License-Identifier: MIT
/*
$info$
tags: frontend|x86-to-ir, opcodes|dispatcher-implementations
desc: Handles x86/64 x87 to IR
$end_info$
*/

#include "Interface/Core/OpcodeDispatcher.h"
#include "Interface/Core/X86Tables/X86Tables.h"
#include "Interface/IR/IR.h"
#include "Interface/Core/Addressing.h"

#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/Utils/EnumUtils.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/FPState.h>

#include <cmath>
#include <stddef.h>
#include <stdint.h>

namespace FEXCore::IR {
class OrderedNode;
#define OpcodeArgs [[maybe_unused]] FEXCore::X86Tables::DecodedOp Op

Ref OpDispatchBuilder::GetX87Top() {
  // Yes, we are storing 3 bits in a single flag register.
  // Deal with it
  return _LoadContextGPR(OpSize::i8Bit, offsetof(FEXCore::Core::CPUState, flags) + FEXCore::X86State::X87FLAG_TOP_LOC);
}

void OpDispatchBuilder::SetX87FTW(Ref FTW) {
  _StackForceSlow(); // Invalidate x87 FTW register cache

  // For the output, we want a 1-bit for each pair not equal to 11 (Empty).
  static_assert(static_cast<uint8_t>(FPState::X87Tag::Empty) == 0b11);

  // Make even bits 1 if the pair is equal to 11, and 0 otherwise.
  FTW = _AndShift(OpSize::i32Bit, FTW, FTW, ShiftType::LSR, 1);

  // Invert FTW and clear the odd bits. Even bits are 1 if the pair
  // is not equal to 11, and odd bits are 0.
  FTW = _Andn(OpSize::i32Bit, Constant(0x55555555), FTW);

  // All that's left is to compact away the odd bits. That is a Morton
  // deinterleave operation, which has a standard solution. See
  // https://stackoverflow.com/questions/3137266/how-to-de-interleave-bits-unmortonizing
  FTW = _And(OpSize::i32Bit, _Orlshr(OpSize::i32Bit, FTW, FTW, 1), Constant(0x33333333));
  FTW = _And(OpSize::i32Bit, _Orlshr(OpSize::i32Bit, FTW, FTW, 2), Constant(0x0f0f0f0f));
  FTW = _Orlshr(OpSize::i32Bit, FTW, FTW, 4);

  // ...and that's it. StoreContext implicitly does the final masking.
  _StoreContextGPR(OpSize::i8Bit, FTW, offsetof(FEXCore::Core::CPUState, AbridgedFTW));
}

void OpDispatchBuilder::SetX87Top(Ref Value) {
  _StoreContextGPR(OpSize::i8Bit, Value, offsetof(FEXCore::Core::CPUState, flags) + FEXCore::X86State::X87FLAG_TOP_LOC);
}

// Float LoaD operation with memory operand
void OpDispatchBuilder::FLD(OpcodeArgs, IR::OpSize Width) {
  const auto ReadWidth = (Width == OpSize::f80Bit) ? OpSize::i128Bit : Width;

  Ref Data = LoadSourceFPR_WithOpSize(Op, Op->Src[0], Width, Op->Flags);
  Ref ConvertedData = Data;
  // Convert to 80bit float
  if (Width == OpSize::i32Bit || Width == OpSize::i64Bit) {
    ConvertedData = _F80CVTTo(Data, ReadWidth);
  }
  _PushStack(ConvertedData, Data, ReadWidth, true);
}

// Float LoaD operation with memory operand
void OpDispatchBuilder::FLDFromStack(OpcodeArgs) {
  _CopyPushStack(Op->OP & 7);
}

void OpDispatchBuilder::FBLD(OpcodeArgs) {
  // Read from memory
  Ref Data = LoadSourceFPR_WithOpSize(Op, Op->Src[0], OpSize::f80Bit, Op->Flags);
  Ref ConvertedData = _F80BCDLoad(Data);
  _PushStack(ConvertedData, Data, OpSize::i128Bit, true);
}

void OpDispatchBuilder::FBSTP(OpcodeArgs) {
  Ref converted = _F80BCDStore(_ReadStackValue(0));
  StoreResultFPR_WithOpSize(Op, Op->Dest, converted, OpSize::f80Bit, OpSize::i8Bit);
  _PopStackDestroy();
}

void OpDispatchBuilder::FLD_Const(OpcodeArgs, NamedVectorConstant K) {
  // Update TOP
  Ref Data = LoadAndCacheNamedVectorConstant(OpSize::i128Bit, K);
  _PushStack(Data, Data, OpSize::i128Bit, true);
}

void OpDispatchBuilder::FILD(OpcodeArgs) {
  const auto ReadWidth = OpSizeFromSrc(Op);
  // Read from memory
  Ref Data = LoadSourceGPR_WithOpSize(Op, Op->Src[0], ReadWidth, Op->Flags);

  // Sign extend to 64bits
  if (ReadWidth != OpSize::i64Bit) {
    Data = _Sbfe(OpSize::i64Bit, IR::OpSizeAsBits(ReadWidth), 0, Data);
  }

  // We're about to clobber flags to grab the sign, so save NZCV.
  SaveNZCV();

  // Extract sign and make integer absolute
  auto zero = Constant(0);
  _SubNZCV(OpSize::i64Bit, Data, zero);
  auto sign = _NZCVSelect(OpSize::i64Bit, CondClass::SLT, Constant(0x8000), zero);
  auto absolute = _Neg(OpSize::i64Bit, Data, CondClass::MI);

  // left justify the absolute integer
  auto shift = Sub(OpSize::i64Bit, Constant(63), _FindMSB(IR::OpSize::i64Bit, absolute));
  auto shifted = _Lshl(OpSize::i64Bit, absolute, shift);

  auto adjusted_exponent = Sub(OpSize::i64Bit, Constant(0x3fff + 63), shift);
  auto zeroed_exponent = _Select(OpSize::i64Bit, OpSize::i64Bit, CondClass::EQ, absolute, zero, zero, adjusted_exponent);
  auto upper = _Or(OpSize::i64Bit, sign, zeroed_exponent);

  Ref ConvertedData = _VLoadTwoGPRs(shifted, upper);
  _PushStack(ConvertedData, Data, ReadWidth, false);
}

void OpDispatchBuilder::FST(OpcodeArgs, IR::OpSize Width) {
  const auto SourceSize = ReducedPrecisionMode ? OpSize::i64Bit : OpSize::i128Bit;
  AddressMode A = DecodeAddress(Op, Op->Dest, MemoryAccessType::DEFAULT, false);

  A = SelectAddressMode(this, A, GetGPROpSize(), CTX->HostFeatures.SupportsTSOImm9, false, false, Width);
  _StoreStackMem(SourceSize, Width, A.Base, A.Index, OpSize::iInvalid, A.IndexType, A.IndexScale, /*Float=*/true);

  if (Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) {
    _PopStackDestroy();
  }
}

void OpDispatchBuilder::FSTToStack(OpcodeArgs) {
  const uint8_t Offset = Op->OP & 7;
  if (Offset != 0) {
    _StoreStackToStack(Offset);
  }

  if (Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) {
    _PopStackDestroy();
  }
}

// Store integer to memory (possibly with truncation)
void OpDispatchBuilder::FIST(OpcodeArgs, bool Truncate) {
  const auto Size = OpSizeFromSrc(Op);
  Ref Data = _ReadStackValue(0);

  // For 16-bit integers, we need to manually check for overflow
  // since _F80CVTInt doesn't handle 16-bit overflow detection properly
  if (Size == OpSize::i16Bit) {
    // Extract the 80-bit float value to check for special cases
    // Get the upper 64 bits which contain sign and exponent and then the exponent from upper.
    Ref Upper = _VExtractToGPR(OpSize::i128Bit, OpSize::i64Bit, Data, 1);
    Ref Exponent = _And(OpSize::i64Bit, Upper, Constant(0x7fff));

    // Check for NaN/Infinity: exponent = 0x7fff
    SaveNZCV();
    _TestNZ(OpSize::i64Bit, Exponent, Constant(0x7fff));
    Ref IsSpecial = _NZCVSelect01(CondClass::EQ);

    // For overflow detection, check if exponent indicates a value >= 2^15
    // Biased exponent for 2^15 is 0x3fff + 15 = 0x400e
    SubWithFlags(OpSize::i64Bit, Exponent, 0x400e);
    Ref IsOverflow = _NZCVSelect01(CondClass::UGE);

    // Set Invalid Operation flag if overflow or special value
    Ref InvalidFlag = _Or(OpSize::i64Bit, IsSpecial, IsOverflow);
    SetRFLAG<FEXCore::X86State::X87FLAG_IE_LOC>(InvalidFlag);
  }

  Data = _F80CVTInt(Size, Data, Truncate);

  StoreResultGPR_WithOpSize(Op, Op->Dest, Data, Size, OpSize::i8Bit);

  if ((Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) != 0) {
    _PopStackDestroy();
  }
}

void OpDispatchBuilder::FADD(OpcodeArgs, IR::OpSize Width, bool Integer, OpDispatchBuilder::OpResult ResInST0) {
  if (Op->Src[0].IsNone()) { // Implicit argument case
    auto Offset = Op->OP & 7;
    auto St0 = 0;
    if (ResInST0 == OpResult::RES_STI) {
      _F80AddStack(Offset, St0);
    } else {
      _F80AddStack(St0, Offset);
    }
    if (Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) {
      _PopStackDestroy();
    }
    return;
  }

  LOGMAN_THROW_A_FMT(Width != OpSize::f80Bit, "No 80-bit floats from memory");
  // We have one memory argument
  Ref Arg {};
  if (Integer) {
    Arg = LoadSourceGPR(Op, Op->Src[0], Op->Flags);
    Arg = _F80CVTToInt(Arg, Width);
  } else {
    Arg = LoadSourceFPR(Op, Op->Src[0], Op->Flags);
    Arg = _F80CVTTo(Arg, Width);
  }

  // top of stack is at offset zero
  _F80AddValue(0, Arg);
}

void OpDispatchBuilder::FMUL(OpcodeArgs, IR::OpSize Width, bool Integer, OpDispatchBuilder::OpResult ResInST0) {
  if (Op->Src[0].IsNone()) { // Implicit argument case
    auto offset = Op->OP & 7;
    auto st0 = 0;
    if (ResInST0 == OpResult::RES_STI) {
      _F80MulStack(offset, st0);
    } else {
      _F80MulStack(st0, offset);
    }
    if (Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) {
      _PopStackDestroy();
    }
    return;
  }

  LOGMAN_THROW_A_FMT(Width != OpSize::f80Bit, "No 80-bit floats from memory");
  // We have one memory argument
  Ref arg {};
  if (Integer) {
    arg = LoadSourceGPR(Op, Op->Src[0], Op->Flags);
    arg = _F80CVTToInt(arg, Width);
  } else {
    arg = LoadSourceFPR(Op, Op->Src[0], Op->Flags);
    arg = _F80CVTTo(arg, Width);
  }

  // top of stack is at offset zero
  _F80MulValue(0, arg);

  if (Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) {
    _PopStackDestroy();
  }
}

void OpDispatchBuilder::FDIV(OpcodeArgs, IR::OpSize Width, bool Integer, bool Reverse, OpDispatchBuilder::OpResult ResInST0) {
  if (Op->Src[0].IsNone()) {
    const uint8_t Offset = Op->OP & 7;
    const uint8_t St0 = 0;
    const uint8_t Result = (ResInST0 == OpResult::RES_STI) ? Offset : St0;

    if (Reverse ^ (ResInST0 == OpResult::RES_STI)) {
      _F80DivStack(Result, Offset, St0);
    } else {
      _F80DivStack(Result, St0, Offset);
    }

    if (Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) {
      _PopStackDestroy();
    }
    return;
  }

  LOGMAN_THROW_A_FMT(Width != OpSize::f80Bit, "No 80-bit floats from memory");
  // We have one memory argument
  Ref arg {};
  if (Integer) {
    arg = LoadSourceGPR(Op, Op->Src[0], Op->Flags);
    arg = _F80CVTToInt(arg, Width);
  } else {
    arg = LoadSourceFPR(Op, Op->Src[0], Op->Flags);
    arg = _F80CVTTo(arg, Width);
  }

  // top of stack is at offset zero
  if (Reverse) {
    _F80DivRValue(arg, 0);
  } else {
    _F80DivValue(0, arg);
  }

  if (Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) {
    _PopStackDestroy();
  }
}

void OpDispatchBuilder::FSUB(OpcodeArgs, IR::OpSize Width, bool Integer, bool Reverse, OpDispatchBuilder::OpResult ResInST0) {
  if (Op->Src[0].IsNone()) {
    const auto Offset = Op->OP & 7;
    const auto St0 = 0;
    const auto Result = (ResInST0 == OpResult::RES_STI) ? Offset : St0;

    if (Reverse ^ (ResInST0 == OpResult::RES_STI)) {
      _F80SubStack(Result, Offset, St0);
    } else {
      _F80SubStack(Result, St0, Offset);
    }

    if (Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) {
      _PopStackDestroy();
    }
    return;
  }

  LOGMAN_THROW_A_FMT(Width != OpSize::f80Bit, "No 80-bit floats from memory");
  // We have one memory argument
  Ref Arg {};
  if (Integer) {
    Arg = LoadSourceGPR(Op, Op->Src[0], Op->Flags);
    Arg = _F80CVTToInt(Arg, Width);
  } else {
    Arg = LoadSourceFPR(Op, Op->Src[0], Op->Flags);
    Arg = _F80CVTTo(Arg, Width);
  }

  // top of stack is at offset zero
  if (Reverse) {
    _F80SubRValue(Arg, 0);
  } else {
    _F80SubValue(0, Arg);
  }

  if (Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) {
    _PopStackDestroy();
  }
}

Ref OpDispatchBuilder::GetX87FTW_Helper() {
  // AbridgedFTWIndex has 1-bit per slot (8 slots). Duplicate each bit to get
  // 2-bits per slot (16-bit result). Duplicating bits is equivalent to
  // Morton interleaving a number with itself. To interleave efficiently two
  // bytes, we use the well-known bit twiddling algorithm:
  //
  // https://graphics.stanford.edu/~seander/bithacks.html#InterleaveBMN
  Ref X = _LoadContextGPR(OpSize::i8Bit, offsetof(FEXCore::Core::CPUState, AbridgedFTW));
  X = _Orlshl(OpSize::i32Bit, X, X, 4);
  X = _And(OpSize::i32Bit, X, Constant(0x0f0f0f0f));
  X = _Orlshl(OpSize::i32Bit, X, X, 2);
  X = _And(OpSize::i32Bit, X, Constant(0x33333333));
  X = _Orlshl(OpSize::i32Bit, X, X, 1);
  X = _And(OpSize::i32Bit, X, Constant(0x55555555));
  X = _Orlshl(OpSize::i32Bit, X, X, 1);

  // The above sequence sets valid to 11 and empty to 00, so invert to finalize.
  static_assert(static_cast<uint8_t>(FPState::X87Tag::Valid) == 0b00);
  static_assert(static_cast<uint8_t>(FPState::X87Tag::Empty) == 0b11);
  return _Xor(OpSize::i32Bit, X, Constant(0xffff));
}

void OpDispatchBuilder::X87FNSTENV(OpcodeArgs) {


  // 14 bytes for 16bit
  // 2 Bytes : FCW
  // 2 Bytes : FSW
  // 2 bytes : FTW
  // 2 bytes : Instruction offset
  // 2 bytes : Instruction CS selector
  // 2 bytes : Data offset
  // 2 bytes : Data selector

  // 28 bytes for 32bit
  // 4 bytes : FCW
  // 4 bytes : FSW
  // 4 bytes : FTW
  // 4 bytes : Instruction pointer
  // 2 bytes : Instruction pointer selector
  // 2 bytes : Opcode
  // 4 bytes : data pointer offset
  // 4 bytes : data pointer selector

  // Before we store anything we need to sync our stack to the registers.
  _SyncStackToSlow();

  const auto Size = OpSizeFromSrc(Op);
  Ref Mem = LoadSourceGPR(Op, Op->Dest, Op->Flags, {.LoadData = false});
  Mem = AppendSegmentOffset(Mem, Op->Flags);

  {
    auto FCW = _LoadContextGPR(OpSize::i16Bit, offsetof(FEXCore::Core::CPUState, FCW));
    _StoreMemGPR(Size, Mem, FCW, Size);
  }

  { _StoreMemGPR(Size, ReconstructFSW_Helper(), Mem, Constant(IR::OpSizeToSize(Size) * 1), Size, MemOffsetType::SXTX, 1); }

  auto ZeroConst = Constant(0);

  {
    // FTW
    _StoreMemGPR(Size, GetX87FTW_Helper(), Mem, Constant(IR::OpSizeToSize(Size) * 2), Size, MemOffsetType::SXTX, 1);
  }

  {
    // Instruction Offset
    _StoreMemGPR(Size, ZeroConst, Mem, Constant(IR::OpSizeToSize(Size) * 3), Size, MemOffsetType::SXTX, 1);
  }

  {
    // Instruction CS selector (+ Opcode)
    _StoreMemGPR(Size, ZeroConst, Mem, Constant(IR::OpSizeToSize(Size) * 4), Size, MemOffsetType::SXTX, 1);
  }

  {
    // Data pointer offset
    _StoreMemGPR(Size, ZeroConst, Mem, Constant(IR::OpSizeToSize(Size) * 5), Size, MemOffsetType::SXTX, 1);
  }

  {
    // Data pointer selector
    _StoreMemGPR(Size, ZeroConst, Mem, Constant(IR::OpSizeToSize(Size) * 6), Size, MemOffsetType::SXTX, 1);
  }
}

Ref OpDispatchBuilder::ReconstructX87StateFromFSW_Helper(Ref FSW) {
  auto Top = _Bfe(OpSize::i32Bit, 3, 11, FSW);
  SetX87Top(Top);

  auto C0 = _Bfe(OpSize::i32Bit, 1, 8, FSW);
  auto C1 = _Bfe(OpSize::i32Bit, 1, 9, FSW);
  auto C2 = _Bfe(OpSize::i32Bit, 1, 10, FSW);
  auto C3 = _Bfe(OpSize::i32Bit, 1, 14, FSW);
  auto IE = _Bfe(OpSize::i32Bit, 1, 0, FSW);

  SetRFLAG<FEXCore::X86State::X87FLAG_C0_LOC>(C0);
  SetRFLAG<FEXCore::X86State::X87FLAG_C1_LOC>(C1);
  SetRFLAG<FEXCore::X86State::X87FLAG_C2_LOC>(C2);
  SetRFLAG<FEXCore::X86State::X87FLAG_C3_LOC>(C3);
  SetRFLAG<FEXCore::X86State::X87FLAG_IE_LOC>(IE);
  return Top;
}

void OpDispatchBuilder::X87LDENV(OpcodeArgs) {
  _StackForceSlow();

  const auto Size = OpSizeFromSrc(Op);
  Ref Mem = LoadSourceGPR(Op, Op->Src[0], Op->Flags, {.LoadData = false});
  Mem = AppendSegmentOffset(Mem, Op->Flags);

  auto NewFCW = _LoadMemGPR(OpSize::i16Bit, Mem, OpSize::i16Bit);
  _StoreContextGPR(OpSize::i16Bit, NewFCW, offsetof(FEXCore::Core::CPUState, FCW));

  Ref MemLocation = Add(OpSize::i64Bit, Mem, IR::OpSizeToSize(Size) * 1);
  auto NewFSW = _LoadMemGPR(Size, MemLocation, Size);
  ReconstructX87StateFromFSW_Helper(NewFSW);

  {
    // FTW
    Ref MemLocation = Add(OpSize::i64Bit, Mem, IR::OpSizeToSize(Size) * 2);
    SetX87FTW(_LoadMemGPR(Size, MemLocation, Size));
  }
}

void OpDispatchBuilder::X87FNSAVE(OpcodeArgs) {
  _SyncStackToSlow();

  // 14 bytes for 16bit
  // 2 Bytes : FCW
  // 2 Bytes : FSW
  // 2 bytes : FTW
  // 2 bytes : Instruction offset
  // 2 bytes : Instruction CS selector
  // 2 bytes : Data offset
  // 2 bytes : Data selector

  // 28 bytes for 32bit
  // 4 bytes : FCW
  // 4 bytes : FSW
  // 4 bytes : FTW
  // 4 bytes : Instruction pointer
  // 2 bytes : instruction pointer selector
  // 2 bytes : Opcode
  // 4 bytes : data pointer offset
  // 4 bytes : data pointer selector
  const auto Size = OpSizeFromDst(Op);
  Ref Mem = MakeSegmentAddress(Op, Op->Dest);
  Ref Top = GetX87Top();
  {
    auto FCW = _LoadContextGPR(OpSize::i16Bit, offsetof(FEXCore::Core::CPUState, FCW));
    _StoreMemGPR(Size, Mem, FCW, Size);
  }

  { _StoreMemGPR(Size, ReconstructFSW_Helper(), Mem, Constant(IR::OpSizeToSize(Size) * 1), Size, MemOffsetType::SXTX, 1); }

  auto ZeroConst = Constant(0);

  {
    // FTW
    _StoreMemGPR(Size, GetX87FTW_Helper(), Mem, Constant(IR::OpSizeToSize(Size) * 2), Size, MemOffsetType::SXTX, 1);
  }

  {
    // Instruction Offset
    _StoreMemGPR(Size, ZeroConst, Mem, Constant(IR::OpSizeToSize(Size) * 3), Size, MemOffsetType::SXTX, 1);
  }

  {
    // Instruction CS selector (+ Opcode)
    _StoreMemGPR(Size, ZeroConst, Mem, Constant(IR::OpSizeToSize(Size) * 4), Size, MemOffsetType::SXTX, 1);
  }

  {
    // Data pointer offset
    _StoreMemGPR(Size, ZeroConst, Mem, Constant(IR::OpSizeToSize(Size) * 5), Size, MemOffsetType::SXTX, 1);
  }

  {
    // Data pointer selector
    _StoreMemGPR(Size, ZeroConst, Mem, Constant(IR::OpSizeToSize(Size) * 6), Size, MemOffsetType::SXTX, 1);
  }

  auto SevenConst = Constant(7);
  const auto LoadSize = ReducedPrecisionMode ? OpSize::i64Bit : OpSize::i128Bit;
  for (int i = 0; i < 7; ++i) {
    Ref data = _LoadContextFPRIndexed(Top, LoadSize, MMBaseOffset(), IR::OpSizeToSize(OpSize::i128Bit));
    if (ReducedPrecisionMode) {
      data = _F80CVTTo(data, OpSize::i64Bit);
    }
    _StoreMemFPR(OpSize::i128Bit, data, Mem, Constant((IR::OpSizeToSize(Size) * 7) + (10 * i)), OpSize::i8Bit, MemOffsetType::SXTX, 1);
    Top = _And(OpSize::i32Bit, Add(OpSize::i32Bit, Top, 1), SevenConst);
  }

  // The final st(7) needs a bit of special handling here
  Ref data = _LoadContextFPRIndexed(Top, LoadSize, MMBaseOffset(), IR::OpSizeToSize(OpSize::i128Bit));
  if (ReducedPrecisionMode) {
    data = _F80CVTTo(data, OpSize::i64Bit);
  }
  // ST7 broken in to two parts
  // Lower 64bits [63:0]
  // upper 16 bits [79:64]
  _StoreMemFPR(OpSize::i64Bit, data, Mem, Constant((IR::OpSizeToSize(Size) * 7) + (7 * 10)), OpSize::i8Bit, MemOffsetType::SXTX, 1);
  auto topBytes = _VDupElement(OpSize::i128Bit, OpSize::i16Bit, data, 4);
  _StoreMemFPR(OpSize::i16Bit, topBytes, Mem, Constant((IR::OpSizeToSize(Size) * 7) + (7 * 10) + 8), OpSize::i8Bit, MemOffsetType::SXTX, 1);

  // reset to default
  FNINIT(Op);
}

void OpDispatchBuilder::X87FRSTOR(OpcodeArgs) {
  _StackForceSlow();
  const auto Size = OpSizeFromSrc(Op);
  Ref Mem = MakeSegmentAddress(Op, Op->Src[0]);

  auto NewFCW = _LoadMemGPR(OpSize::i16Bit, Mem, OpSize::i16Bit);
  _StoreContextGPR(OpSize::i16Bit, NewFCW, offsetof(FEXCore::Core::CPUState, FCW));
  if (ReducedPrecisionMode) {
    // ignore the rounding precision, we're always 64-bit in F64.
    // extract rounding mode
    Ref roundingMode = NewFCW;
    auto roundShift = Constant(10);
    auto roundMask = Constant(3);
    roundingMode = _Lshr(OpSize::i32Bit, roundingMode, roundShift);
    roundingMode = _And(OpSize::i32Bit, roundingMode, roundMask);
    _SetRoundingMode(roundingMode, false, roundingMode);
  }

  auto NewFSW = _LoadMemGPR(Size, Mem, Constant(IR::OpSizeToSize(Size) * 1), Size, MemOffsetType::SXTX, 1);
  Ref Top = ReconstructX87StateFromFSW_Helper(NewFSW);
  {
    // FTW
    SetX87FTW(_LoadMemGPR(Size, Mem, Constant(IR::OpSizeToSize(Size) * 2), Size, MemOffsetType::SXTX, 1));
  }

  auto SevenConst = Constant(7);
  auto low = Constant(~0ULL);
  auto high = Constant(0xFFFF);
  Ref Mask = _VLoadTwoGPRs(low, high);
  const auto StoreSize = ReducedPrecisionMode ? OpSize::i64Bit : OpSize::i128Bit;
  for (int i = 0; i < 7; ++i) {
    Ref Reg = _LoadMemFPR(OpSize::i128Bit, Mem, Constant((IR::OpSizeToSize(Size) * 7) + (10 * i)), OpSize::i8Bit, MemOffsetType::SXTX, 1);
    // Mask off the top bits
    Reg = _VAnd(OpSize::i128Bit, OpSize::i128Bit, Reg, Mask);
    if (ReducedPrecisionMode) {
      // Convert to double precision
      Reg = _F80CVT(OpSize::i64Bit, Reg);
    }
    _StoreContextFPRIndexed(Reg, Top, StoreSize, MMBaseOffset(), IR::OpSizeToSize(OpSize::i128Bit));

    Top = _And(OpSize::i32Bit, Add(OpSize::i32Bit, Top, 1), SevenConst);
  }

  // The final st(7) needs a bit of special handling here
  // ST7 broken in to two parts
  // Lower 64bits [63:0]
  // upper 16 bits [79:64]
  Ref Reg = _LoadMemFPR(OpSize::i64Bit, Mem, Constant((IR::OpSizeToSize(Size) * 7) + (10 * 7)), OpSize::i8Bit, MemOffsetType::SXTX, 1);
  Ref RegHigh = _LoadMemFPR(OpSize::i16Bit, Mem, Constant((IR::OpSizeToSize(Size) * 7) + (10 * 7) + 8), OpSize::i8Bit, MemOffsetType::SXTX, 1);
  Reg = _VInsElement(OpSize::i128Bit, OpSize::i16Bit, 4, 0, Reg, RegHigh);
  if (ReducedPrecisionMode) {
    Reg = _F80CVT(OpSize::i64Bit, Reg); // Convert to double precision
  }
  _StoreContextFPRIndexed(Reg, Top, StoreSize, MMBaseOffset(), IR::OpSizeToSize(OpSize::i128Bit));
}

// Load / Store Control Word
void OpDispatchBuilder::X87FSTCW(OpcodeArgs) {
  auto FCW = _LoadContextGPR(OpSize::i16Bit, offsetof(FEXCore::Core::CPUState, FCW));
  StoreResultGPR(Op, FCW);
}

void OpDispatchBuilder::X87FLDCW(OpcodeArgs) {
  // FIXME: Because loading control flags will affect several instructions in fast path, we might have
  // to switch for now to slow mode whenever these are manually changed.
  // Remove the next line and try DF_04.asm in fast path.
  _StackForceSlow();
  Ref NewFCW = LoadSourceGPR(Op, Op->Src[0], Op->Flags);
  _StoreContextGPR(OpSize::i16Bit, NewFCW, offsetof(FEXCore::Core::CPUState, FCW));
}

void OpDispatchBuilder::FXCH(OpcodeArgs) {
  uint8_t Offset = Op->OP & 7;
  // fxch st0, st0 is for us essentially a nop
  if (Offset != 0) {
    _F80StackXchange(Offset);
  }
  SetRFLAG<FEXCore::X86State::X87FLAG_C1_LOC>(Constant(0));
}

void OpDispatchBuilder::X87FYL2X(OpcodeArgs, bool IsFYL2XP1) {
  if (IsFYL2XP1) {
    // create an add between top of stack and 1.
    Ref One = ReducedPrecisionMode ? _VCastFromGPR(OpSize::i64Bit, OpSize::i64Bit, Constant(0x3FF0000000000000)) :
                                     LoadAndCacheNamedVectorConstant(OpSize::i128Bit, NamedVectorConstant::NAMED_VECTOR_X87_ONE);
    _F80AddValue(0, One);
  }

  _F80FYL2XStack();
}

void OpDispatchBuilder::FCOMI(OpcodeArgs, IR::OpSize Width, bool Integer, OpDispatchBuilder::FCOMIFlags WhichFlags, bool PopTwice) {
  Ref arg {};
  Ref b {};

  Ref Res {};
  if (Op->Src[0].IsNone()) {
    // Implicit arg
    uint8_t Offset = Op->OP & 7;
    Res = _F80CmpStack(Offset);
  } else {
    if (Width == OpSize::i16Bit || Width == OpSize::i32Bit || Width == OpSize::i64Bit) {
      // Memory arg
      if (Integer) {
        arg = LoadSourceGPR(Op, Op->Src[0], Op->Flags);
        b = _F80CVTToInt(arg, Width);
      } else {
        arg = LoadSourceFPR(Op, Op->Src[0], Op->Flags);
        b = _F80CVTTo(arg, Width);
      }
    } else {
      FEX_UNREACHABLE;
    }
    Res = _F80CmpValue(b);
  }

  Ref HostFlag_CF = _Bfe(OpSize::i64Bit, 1, FCMP_FLAG_LT, Res);
  Ref HostFlag_ZF = _Bfe(OpSize::i64Bit, 1, FCMP_FLAG_EQ, Res);
  Ref HostFlag_Unordered = _Bfe(OpSize::i64Bit, 1, FCMP_FLAG_UNORDERED, Res);
  HostFlag_CF = _Or(OpSize::i32Bit, HostFlag_CF, HostFlag_Unordered);
  HostFlag_ZF = _Or(OpSize::i32Bit, HostFlag_ZF, HostFlag_Unordered);

  if (WhichFlags == FCOMIFlags::FLAGS_X87) {
    SetRFLAG<FEXCore::X86State::X87FLAG_C0_LOC>(HostFlag_CF);
    SetRFLAG<FEXCore::X86State::X87FLAG_C1_LOC>(Constant(0));
    SetRFLAG<FEXCore::X86State::X87FLAG_C2_LOC>(HostFlag_Unordered);
    SetRFLAG<FEXCore::X86State::X87FLAG_C3_LOC>(HostFlag_ZF);
  } else {
    // OF, SF, AF, PF all undefined
    SetCFDirect(HostFlag_CF);
    SetRFLAG<FEXCore::X86State::RFLAG_ZF_RAW_LOC>(HostFlag_ZF);

    // PF is stored inverted, so invert from the host flag.
    // TODO: This could perhaps be optimized?
    auto PF = _Xor(OpSize::i32Bit, HostFlag_Unordered, Constant(1));
    SetRFLAG<FEXCore::X86State::RFLAG_PF_RAW_LOC>(PF);
  }

  // Set Invalid Operation flag when unordered (NaN comparison)
  SetRFLAG<FEXCore::X86State::X87FLAG_IE_LOC>(HostFlag_Unordered);

  if (PopTwice) {
    _PopStackDestroy();
    _PopStackDestroy();
  } else if ((Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) != 0) {
    _PopStackDestroy();
  }
}

void OpDispatchBuilder::FTST(OpcodeArgs) {
  Ref Res = _F80StackTest(0);

  Ref HostFlag_CF = _Bfe(OpSize::i64Bit, 1, FCMP_FLAG_LT, Res);
  Ref HostFlag_ZF = _Bfe(OpSize::i64Bit, 1, FCMP_FLAG_EQ, Res);
  Ref HostFlag_Unordered = _Bfe(OpSize::i64Bit, 1, FCMP_FLAG_UNORDERED, Res);
  HostFlag_CF = _Or(OpSize::i32Bit, HostFlag_CF, HostFlag_Unordered);
  HostFlag_ZF = _Or(OpSize::i32Bit, HostFlag_ZF, HostFlag_Unordered);

  SetRFLAG<FEXCore::X86State::X87FLAG_C0_LOC>(HostFlag_CF);
  SetRFLAG<FEXCore::X86State::X87FLAG_C1_LOC>(Constant(0));
  SetRFLAG<FEXCore::X86State::X87FLAG_C2_LOC>(HostFlag_Unordered);
  SetRFLAG<FEXCore::X86State::X87FLAG_C3_LOC>(HostFlag_ZF);

  // Set Invalid Operation flag when unordered (NaN comparison)
  SetRFLAG<FEXCore::X86State::X87FLAG_IE_LOC>(HostFlag_Unordered);
}

void OpDispatchBuilder::X87OpHelper(OpcodeArgs, FEXCore::IR::IROps IROp, bool ZeroC2) {
  DeriveOp(Result, IROp, _F80SCALEStack());
  if (ZeroC2) {
    SetRFLAG<FEXCore::X86State::X87FLAG_C2_LOC>(Constant(0));
  }
}

void OpDispatchBuilder::X87ModifySTP(OpcodeArgs, bool Inc) {
  if (Inc) {
    _IncStackTop();
  } else {
    _DecStackTop();
  }
}

// Operations dealing with loading and storing environment pieces

// Reconstruct as a constant the Status Word of the FPU.
// We only track stack top and each of the code conditions (C flags)
// Top is 3 bits at bit 11.
// C0 is 1 bit at bit 8.
// C1 is 1 bit at bit 9.
// C2 is 1 bit at bit 10.
// C3 is 1 bit at bit 14.
// Optionally we can pass a pre calculated value for Top, otherwise we calculate it
// during the function runtime.
Ref OpDispatchBuilder::ReconstructFSW_Helper(Ref T) {
  // Start with the top value
  auto Top = T ? T : GetX87Top();
  Ref FSW = _Lshl(OpSize::i64Bit, Top, Constant(11));

  // We must construct the FSW from our various bits
  auto C0 = GetRFLAG(FEXCore::X86State::X87FLAG_C0_LOC);
  FSW = _Orlshl(OpSize::i64Bit, FSW, C0, 8);

  auto C1 = GetRFLAG(FEXCore::X86State::X87FLAG_C1_LOC);
  FSW = _Orlshl(OpSize::i64Bit, FSW, C1, 9);

  auto C2 = GetRFLAG(FEXCore::X86State::X87FLAG_C2_LOC);
  FSW = _Orlshl(OpSize::i64Bit, FSW, C2, 10);

  auto C3 = GetRFLAG(FEXCore::X86State::X87FLAG_C3_LOC);
  FSW = _Orlshl(OpSize::i64Bit, FSW, C3, 14);

  auto IE = GetRFLAG(FEXCore::X86State::X87FLAG_IE_LOC);
  FSW = _Or(OpSize::i64Bit, FSW, IE);

  return FSW;
}

// Store Status Word
// There's no load Status Word instruction but you can load it through frstor
// or fldenv.
void OpDispatchBuilder::X87FNSTSW(OpcodeArgs) {
  Ref TopValue = _SyncStackToSlow();
  Ref StatusWord = ReconstructFSW_Helper(TopValue);
  StoreResultGPR(Op, StatusWord);
}

void OpDispatchBuilder::FNCLEX(OpcodeArgs) {
  // Clear the exception flag bit
  SetRFLAG<FEXCore::X86State::X87FLAG_IE_LOC>(_Constant(0));
}

void OpDispatchBuilder::FNINIT(OpcodeArgs) {
  _SyncStackToSlow(); // Invalidate x87 register caches

  auto Zero = Constant(0);

  if (ReducedPrecisionMode) {
    _SetRoundingMode(Zero, false, Zero);
  }

  // Init FCW to 0x037F
  auto NewFCW = Constant(0x037F);
  _StoreContextGPR(OpSize::i16Bit, NewFCW, offsetof(FEXCore::Core::CPUState, FCW));

  // Set top to zero
  SetX87Top(Zero);
  // Tags all get marked as invalid
  _StoreContextGPR(OpSize::i8Bit, Zero, offsetof(FEXCore::Core::CPUState, AbridgedFTW));

  // Reinits the simulated stack
  _InitStack();

  SetRFLAG<FEXCore::X86State::X87FLAG_C0_LOC>(Zero);
  SetRFLAG<FEXCore::X86State::X87FLAG_C1_LOC>(Zero);
  SetRFLAG<FEXCore::X86State::X87FLAG_C2_LOC>(Zero);
  SetRFLAG<FEXCore::X86State::X87FLAG_C3_LOC>(Zero);
  SetRFLAG<FEXCore::X86State::X87FLAG_IE_LOC>(Zero);
}

void OpDispatchBuilder::X87FFREE(OpcodeArgs) {
  _InvalidateStack(Op->OP & 7);
}

void OpDispatchBuilder::X87EMMS(OpcodeArgs) {
  // Tags all get set to 0b11
  _InvalidateStack(0xff);
}

void OpDispatchBuilder::X87FCMOV(OpcodeArgs) {
  CalculateDeferredFlags();

  uint16_t Opcode = Op->OP & 0b1111'1111'1000;
  uint8_t CC = 0;

  switch (Opcode) {
  case 0x3'C0:
    CC = 0x3; // JNC
    break;
  case 0x2'C0:
    CC = 0x2; // JC
    break;
  case 0x2'C8:
    CC = 0x4; // JE
    break;
  case 0x3'C8:
    CC = 0x5; // JNE
    break;
  case 0x2'D0:
    CC = 0x6; // JNA
    break;
  case 0x3'D0:
    CC = 0x7; // JA
    break;
  case 0x2'D8:
    CC = 0xA; // JP
    break;
  case 0x3'D8:
    CC = 0xB; // JNP
    break;
  default: LOGMAN_MSG_A_FMT("Unhandled FCMOV op: 0x{:x}", Opcode); break;
  }

  Ref VecCond = _VDupFromGPR(OpSize::i128Bit, OpSize::i64Bit, SelectCC0All1(CC));
  _F80VBSLStack(OpSize::i128Bit, VecCond, Op->OP & 7, 0);
}

void OpDispatchBuilder::X87FXAM(OpcodeArgs) {
  auto a = _ReadStackValue(0);
  Ref Result =
    ReducedPrecisionMode ? _VExtractToGPR(OpSize::i64Bit, OpSize::i64Bit, a, 0) : _VExtractToGPR(OpSize::i128Bit, OpSize::i64Bit, a, 1);

  // Extract the sign bit
  Result = ReducedPrecisionMode ? _Bfe(OpSize::i64Bit, 1, 63, Result) : _Bfe(OpSize::i64Bit, 1, 15, Result);
  SetRFLAG<FEXCore::X86State::X87FLAG_C1_LOC>(Result);

  // Claim this is a normal number
  // We don't support anything else
  auto TopValid = _StackValidTag(0);

  // In the case of top being invalid then C3:C2:C0 is 0b101
  auto C3 = Select01(OpSize::i32Bit, CondClass::NEQ, TopValid, Constant(1));

  auto C2 = TopValid;
  auto C0 = C3; // Mirror C3 until something other than zero is supported
  SetRFLAG<FEXCore::X86State::X87FLAG_C0_LOC>(C0);
  SetRFLAG<FEXCore::X86State::X87FLAG_C2_LOC>(C2);
  SetRFLAG<FEXCore::X86State::X87FLAG_C3_LOC>(C3);
}

void OpDispatchBuilder::X87FXTRACT(OpcodeArgs) {
  auto Top = _ReadStackValue(0);

  _PopStackDestroy();
  auto Exp = _F80XTRACT_EXP(Top);
  auto Sig = _F80XTRACT_SIG(Top);
  _PushStack(Exp, Exp, OpSize::f80Bit, true);
  _PushStack(Sig, Sig, OpSize::f80Bit, true);
}

} // namespace FEXCore::IR
