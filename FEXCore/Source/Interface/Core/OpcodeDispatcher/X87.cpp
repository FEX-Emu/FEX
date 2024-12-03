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

#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/Utils/EnumUtils.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/FPState.h>

#include <stddef.h>
#include <stdint.h>

namespace FEXCore::IR {
class OrderedNode;
#define OpcodeArgs [[maybe_unused]] FEXCore::X86Tables::DecodedOp Op

Ref OpDispatchBuilder::GetX87Top() {
  // Yes, we are storing 3 bits in a single flag register.
  // Deal with it
  return _LoadContext(OpSize::i8Bit, GPRClass, offsetof(FEXCore::Core::CPUState, flags) + FEXCore::X86State::X87FLAG_TOP_LOC);
}

Ref OpDispatchBuilder::GetX87Tag(Ref Value, Ref AbridgedFTW) {
  Ref RegValid = _And(OpSize::i32Bit, _Lshr(OpSize::i32Bit, AbridgedFTW, Value), _Constant(1));
  Ref X87Empty = _Constant(static_cast<uint8_t>(FPState::X87Tag::Empty));
  Ref X87Valid = _Constant(static_cast<uint8_t>(FPState::X87Tag::Valid));

  return _Select(FEXCore::IR::COND_EQ, RegValid, _Constant(0), X87Empty, X87Valid);
}

void OpDispatchBuilder::SetX87FTW(Ref FTW) {
  Ref X87Empty = _Constant(static_cast<uint8_t>(FPState::X87Tag::Empty));
  Ref NewAbridgedFTW;

  for (int i = 0; i < 8; i++) {
    Ref RegTag = _Bfe(OpSize::i32Bit, 2, i * 2, FTW);
    Ref RegValid = _Select(FEXCore::IR::COND_NEQ, RegTag, X87Empty, _Constant(1), _Constant(0));

    if (i) {
      NewAbridgedFTW = _Orlshl(OpSize::i32Bit, NewAbridgedFTW, RegValid, i);
    } else {
      NewAbridgedFTW = RegValid;
    }
  }

  StoreContext(AbridgedFTWIndex, NewAbridgedFTW);
}

void OpDispatchBuilder::SetX87Top(Ref Value) {
  _StoreContext(OpSize::i8Bit, GPRClass, Value, offsetof(FEXCore::Core::CPUState, flags) + FEXCore::X86State::X87FLAG_TOP_LOC);
}

// Float LoaD operation with memory operand
void OpDispatchBuilder::FLD(OpcodeArgs, IR::OpSize Width) {
  const auto ReadWidth = (Width == OpSize::f80Bit) ? OpSize::i128Bit : Width;

  Ref Data = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], Width, Op->Flags);
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
  Ref Data = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], OpSize::f80Bit, Op->Flags);
  Ref ConvertedData = _F80BCDLoad(Data);
  _PushStack(ConvertedData, Data, OpSize::i128Bit, true);
}

void OpDispatchBuilder::FBSTP(OpcodeArgs) {
  Ref converted = _F80BCDStore(_ReadStackValue(0));
  StoreResult_WithOpSize(FPRClass, Op, Op->Dest, converted, OpSize::f80Bit, OpSize::i8Bit);
  _PopStackDestroy();
}

void OpDispatchBuilder::FLD_Const(OpcodeArgs, NamedVectorConstant Constant) {
  // Update TOP
  Ref Data = LoadAndCacheNamedVectorConstant(OpSize::i128Bit, Constant);
  _PushStack(Data, Data, OpSize::i128Bit, true);
}

void OpDispatchBuilder::FILD(OpcodeArgs) {
  const auto ReadWidth = OpSizeFromSrc(Op);
  // Read from memory
  Ref Data = LoadSource_WithOpSize(GPRClass, Op, Op->Src[0], ReadWidth, Op->Flags);

  // Sign extend to 64bits
  if (ReadWidth != OpSize::i64Bit) {
    Data = _Sbfe(OpSize::i64Bit, IR::OpSizeAsBits(ReadWidth), 0, Data);
  }

  // We're about to clobber flags to grab the sign, so save NZCV.
  SaveNZCV();

  // Extract sign and make integer absolute
  auto zero = _Constant(0);
  _SubNZCV(OpSize::i64Bit, Data, zero);
  auto sign = _NZCVSelect(OpSize::i64Bit, CondClassType {COND_SLT}, _Constant(0x8000), zero);
  auto absolute = _Neg(OpSize::i64Bit, Data, CondClassType {COND_MI});

  // left justify the absolute integer
  auto shift = _Sub(OpSize::i64Bit, _Constant(63), _FindMSB(IR::OpSize::i64Bit, absolute));
  auto shifted = _Lshl(OpSize::i64Bit, absolute, shift);

  auto adjusted_exponent = _Sub(OpSize::i64Bit, _Constant(0x3fff + 63), shift);
  auto zeroed_exponent = _Select(COND_EQ, absolute, zero, zero, adjusted_exponent);
  auto upper = _Or(OpSize::i64Bit, sign, zeroed_exponent);

  Ref ConvertedData = _VCastFromGPR(OpSize::i64Bit, OpSize::i64Bit, shifted);
  ConvertedData = _VInsElement(OpSize::i128Bit, OpSize::i64Bit, 1, 0, ConvertedData, _VCastFromGPR(OpSize::i128Bit, OpSize::i64Bit, upper));
  _PushStack(ConvertedData, Data, ReadWidth, false);
}

void OpDispatchBuilder::FST(OpcodeArgs, IR::OpSize Width) {
  Ref Mem = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.LoadData = false});
  Ref PredReg = Invalid();
  if (CTX->HostFeatures.SupportsSVE128 || CTX->HostFeatures.SupportsSVE256) {
    PredReg = InitPredicateCached(OpSize::i16Bit, ARMEmitter::PredicatePattern::SVE_VL5);
  }
  _StoreStackMemory(PredReg, Mem, OpSize::i128Bit, true, Width);
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
  Data = _F80CVTInt(Size, Data, Truncate);

  StoreResult_WithOpSize(GPRClass, Op, Op->Dest, Data, Size, OpSize::i8Bit);

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
    Arg = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
    Arg = _F80CVTToInt(Arg, Width);
  } else {
    Arg = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
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
    arg = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
    arg = _F80CVTToInt(arg, Width);
  } else {
    arg = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
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
    const auto Offset = Op->OP & 7;
    const auto St0 = 0;
    const auto Result = (ResInST0 == OpResult::RES_STI) ? Offset : St0;

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
    arg = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
    arg = _F80CVTToInt(arg, Width);
  } else {
    arg = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
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
    const uint8_t Offset = Op->OP & 7;
    const uint8_t St0 = 0;
    const uint8_t Result = (ResInST0 == OpResult::RES_STI) ? Offset : St0;

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
    Arg = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
    Arg = _F80CVTToInt(Arg, Width);
  } else {
    Arg = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
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
  Ref FTW = _Constant(0);

  for (int i = 0; i < 8; i++) {
    Ref RegTag = GetX87Tag(_Constant(i), LoadContext(AbridgedFTWIndex));
    FTW = _Orlshl(OpSize::i32Bit, FTW, RegTag, i * 2);
  }

  return FTW;
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
  Ref Mem = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.LoadData = false});
  Mem = AppendSegmentOffset(Mem, Op->Flags);

  {
    auto FCW = _LoadContext(OpSize::i16Bit, GPRClass, offsetof(FEXCore::Core::CPUState, FCW));
    _StoreMem(GPRClass, Size, Mem, FCW, Size);
  }

  { _StoreMem(GPRClass, Size, ReconstructFSW_Helper(), Mem, _Constant(IR::OpSizeToSize(Size) * 1), Size, MEM_OFFSET_SXTX, 1); }

  auto ZeroConst = _Constant(0);

  {
    // FTW
    _StoreMem(GPRClass, Size, GetX87FTW_Helper(), Mem, _Constant(IR::OpSizeToSize(Size) * 2), Size, MEM_OFFSET_SXTX, 1);
  }

  {
    // Instruction Offset
    _StoreMem(GPRClass, Size, ZeroConst, Mem, _Constant(IR::OpSizeToSize(Size) * 3), Size, MEM_OFFSET_SXTX, 1);
  }

  {
    // Instruction CS selector (+ Opcode)
    _StoreMem(GPRClass, Size, ZeroConst, Mem, _Constant(IR::OpSizeToSize(Size) * 4), Size, MEM_OFFSET_SXTX, 1);
  }

  {
    // Data pointer offset
    _StoreMem(GPRClass, Size, ZeroConst, Mem, _Constant(IR::OpSizeToSize(Size) * 5), Size, MEM_OFFSET_SXTX, 1);
  }

  {
    // Data pointer selector
    _StoreMem(GPRClass, Size, ZeroConst, Mem, _Constant(IR::OpSizeToSize(Size) * 6), Size, MEM_OFFSET_SXTX, 1);
  }
}

Ref OpDispatchBuilder::ReconstructX87StateFromFSW_Helper(Ref FSW) {
  auto Top = _Bfe(OpSize::i32Bit, 3, 11, FSW);
  SetX87Top(Top);

  auto C0 = _Bfe(OpSize::i32Bit, 1, 8, FSW);
  auto C1 = _Bfe(OpSize::i32Bit, 1, 9, FSW);
  auto C2 = _Bfe(OpSize::i32Bit, 1, 10, FSW);
  auto C3 = _Bfe(OpSize::i32Bit, 1, 14, FSW);

  SetRFLAG<FEXCore::X86State::X87FLAG_C0_LOC>(C0);
  SetRFLAG<FEXCore::X86State::X87FLAG_C1_LOC>(C1);
  SetRFLAG<FEXCore::X86State::X87FLAG_C2_LOC>(C2);
  SetRFLAG<FEXCore::X86State::X87FLAG_C3_LOC>(C3);
  return Top;
}

void OpDispatchBuilder::X87LDENV(OpcodeArgs) {
  _StackForceSlow();

  const auto Size = OpSizeFromSrc(Op);
  Ref Mem = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, {.LoadData = false});
  Mem = AppendSegmentOffset(Mem, Op->Flags);

  auto NewFCW = _LoadMem(GPRClass, OpSize::i16Bit, Mem, OpSize::i16Bit);
  _StoreContext(OpSize::i16Bit, GPRClass, NewFCW, offsetof(FEXCore::Core::CPUState, FCW));

  Ref MemLocation = _Add(OpSize::i64Bit, Mem, _Constant(IR::OpSizeToSize(Size) * 1));
  auto NewFSW = _LoadMem(GPRClass, Size, MemLocation, Size);
  ReconstructX87StateFromFSW_Helper(NewFSW);

  {
    // FTW
    Ref MemLocation = _Add(OpSize::i64Bit, Mem, _Constant(IR::OpSizeToSize(Size) * 2));
    SetX87FTW(_LoadMem(GPRClass, Size, MemLocation, Size));
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
    auto FCW = _LoadContext(OpSize::i16Bit, GPRClass, offsetof(FEXCore::Core::CPUState, FCW));
    _StoreMem(GPRClass, Size, Mem, FCW, Size);
  }

  { _StoreMem(GPRClass, Size, ReconstructFSW_Helper(), Mem, _Constant(IR::OpSizeToSize(Size) * 1), Size, MEM_OFFSET_SXTX, 1); }

  auto ZeroConst = _Constant(0);

  {
    // FTW
    _StoreMem(GPRClass, Size, GetX87FTW_Helper(), Mem, _Constant(IR::OpSizeToSize(Size) * 2), Size, MEM_OFFSET_SXTX, 1);
  }

  {
    // Instruction Offset
    _StoreMem(GPRClass, Size, ZeroConst, Mem, _Constant(IR::OpSizeToSize(Size) * 3), Size, MEM_OFFSET_SXTX, 1);
  }

  {
    // Instruction CS selector (+ Opcode)
    _StoreMem(GPRClass, Size, ZeroConst, Mem, _Constant(IR::OpSizeToSize(Size) * 4), Size, MEM_OFFSET_SXTX, 1);
  }

  {
    // Data pointer offset
    _StoreMem(GPRClass, Size, ZeroConst, Mem, _Constant(IR::OpSizeToSize(Size) * 5), Size, MEM_OFFSET_SXTX, 1);
  }

  {
    // Data pointer selector
    _StoreMem(GPRClass, Size, ZeroConst, Mem, _Constant(IR::OpSizeToSize(Size) * 6), Size, MEM_OFFSET_SXTX, 1);
  }

  auto OneConst = _Constant(1);
  auto SevenConst = _Constant(7);
  const auto LoadSize = ReducedPrecisionMode ? OpSize::i64Bit : OpSize::i128Bit;
  for (int i = 0; i < 7; ++i) {
    Ref data = _LoadContextIndexed(Top, LoadSize, MMBaseOffset(), IR::OpSizeToSize(OpSize::i128Bit), FPRClass);
    if (ReducedPrecisionMode) {
      data = _F80CVTTo(data, OpSize::i64Bit);
    }
    _StoreMem(FPRClass, OpSize::i128Bit, data, Mem, _Constant((IR::OpSizeToSize(Size) * 7) + (10 * i)), OpSize::i8Bit, MEM_OFFSET_SXTX, 1);
    Top = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, Top, OneConst), SevenConst);
  }

  // The final st(7) needs a bit of special handling here
  Ref data = _LoadContextIndexed(Top, LoadSize, MMBaseOffset(), IR::OpSizeToSize(OpSize::i128Bit), FPRClass);
  if (ReducedPrecisionMode) {
    data = _F80CVTTo(data, OpSize::i64Bit);
  }
  // ST7 broken in to two parts
  // Lower 64bits [63:0]
  // upper 16 bits [79:64]
  _StoreMem(FPRClass, OpSize::i64Bit, data, Mem, _Constant((IR::OpSizeToSize(Size) * 7) + (7 * 10)), OpSize::i8Bit, MEM_OFFSET_SXTX, 1);
  auto topBytes = _VDupElement(OpSize::i128Bit, OpSize::i16Bit, data, 4);
  _StoreMem(FPRClass, OpSize::i16Bit, topBytes, Mem, _Constant((IR::OpSizeToSize(Size) * 7) + (7 * 10) + 8), OpSize::i8Bit, MEM_OFFSET_SXTX, 1);

  // reset to default
  FNINIT(Op);
}

void OpDispatchBuilder::X87FRSTOR(OpcodeArgs) {
  _StackForceSlow();
  const auto Size = OpSizeFromSrc(Op);
  Ref Mem = MakeSegmentAddress(Op, Op->Src[0]);

  auto NewFCW = _LoadMem(GPRClass, OpSize::i16Bit, Mem, OpSize::i16Bit);
  _StoreContext(OpSize::i16Bit, GPRClass, NewFCW, offsetof(FEXCore::Core::CPUState, FCW));
  if (ReducedPrecisionMode) {
    // ignore the rounding precision, we're always 64-bit in F64.
    // extract rounding mode
    Ref roundingMode = NewFCW;
    auto roundShift = _Constant(10);
    auto roundMask = _Constant(3);
    roundingMode = _Lshr(OpSize::i32Bit, roundingMode, roundShift);
    roundingMode = _And(OpSize::i32Bit, roundingMode, roundMask);
    _SetRoundingMode(roundingMode, false, roundingMode);
  }

  auto NewFSW = _LoadMem(GPRClass, Size, Mem, _Constant(IR::OpSizeToSize(Size) * 1), Size, MEM_OFFSET_SXTX, 1);
  Ref Top = ReconstructX87StateFromFSW_Helper(NewFSW);
  {
    // FTW
    SetX87FTW(_LoadMem(GPRClass, Size, Mem, _Constant(IR::OpSizeToSize(Size) * 2), Size, MEM_OFFSET_SXTX, 1));
  }

  auto OneConst = _Constant(1);
  auto SevenConst = _Constant(7);

  auto low = _Constant(~0ULL);
  auto high = _Constant(0xFFFF);
  Ref Mask = _VCastFromGPR(OpSize::i128Bit, OpSize::i64Bit, low);
  Mask = _VInsGPR(OpSize::i128Bit, OpSize::i64Bit, 1, Mask, high);
  const auto StoreSize = ReducedPrecisionMode ? OpSize::i64Bit : OpSize::i128Bit;
  for (int i = 0; i < 7; ++i) {
    Ref Reg = _LoadMem(FPRClass, OpSize::i128Bit, Mem, _Constant((IR::OpSizeToSize(Size) * 7) + (10 * i)), OpSize::i8Bit, MEM_OFFSET_SXTX, 1);
    // Mask off the top bits
    Reg = _VAnd(OpSize::i128Bit, OpSize::i128Bit, Reg, Mask);
    if (ReducedPrecisionMode) {
      // Convert to double precision
      Reg = _F80CVT(OpSize::i64Bit, Reg);
    }
    _StoreContextIndexed(Reg, Top, StoreSize, MMBaseOffset(), IR::OpSizeToSize(OpSize::i128Bit), FPRClass);

    Top = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, Top, OneConst), SevenConst);
  }

  // The final st(7) needs a bit of special handling here
  // ST7 broken in to two parts
  // Lower 64bits [63:0]
  // upper 16 bits [79:64]
  Ref Reg = _LoadMem(FPRClass, OpSize::i64Bit, Mem, _Constant((IR::OpSizeToSize(Size) * 7) + (10 * 7)), OpSize::i8Bit, MEM_OFFSET_SXTX, 1);
  Ref RegHigh =
    _LoadMem(FPRClass, OpSize::i16Bit, Mem, _Constant((IR::OpSizeToSize(Size) * 7) + (10 * 7) + 8), OpSize::i8Bit, MEM_OFFSET_SXTX, 1);
  Reg = _VInsElement(OpSize::i128Bit, OpSize::i16Bit, 4, 0, Reg, RegHigh);
  if (ReducedPrecisionMode) {
    Reg = _F80CVT(OpSize::i64Bit, Reg); // Convert to double precision
  }
  _StoreContextIndexed(Reg, Top, StoreSize, MMBaseOffset(), IR::OpSizeToSize(OpSize::i128Bit), FPRClass);
}

// Load / Store Control Word
void OpDispatchBuilder::X87FSTCW(OpcodeArgs) {
  auto FCW = _LoadContext(OpSize::i16Bit, GPRClass, offsetof(FEXCore::Core::CPUState, FCW));
  StoreResult(GPRClass, Op, FCW, OpSize::iInvalid);
}

void OpDispatchBuilder::X87FLDCW(OpcodeArgs) {
  // FIXME: Because loading control flags will affect several instructions in fast path, we might have
  // to switch for now to slow mode whenever these are manually changed.
  // Remove the next line and try DF_04.asm in fast path.
  _StackForceSlow();
  Ref NewFCW = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
  _StoreContext(OpSize::i16Bit, GPRClass, NewFCW, offsetof(FEXCore::Core::CPUState, FCW));
}

void OpDispatchBuilder::FXCH(OpcodeArgs) {
  uint8_t Offset = Op->OP & 7;
  // fxch st0, st0 is for us essentially a nop
  if (Offset != 0) {
    _F80StackXchange(Offset);
  }
  SetRFLAG<FEXCore::X86State::X87FLAG_C1_LOC>(_Constant(0));
}

void OpDispatchBuilder::X87FYL2X(OpcodeArgs, bool IsFYL2XP1) {
  if (IsFYL2XP1) {
    // create an add between top of stack and 1.
    Ref One = ReducedPrecisionMode ? _VCastFromGPR(OpSize::i64Bit, OpSize::i64Bit, _Constant(0x3FF0000000000000)) :
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
        arg = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
        b = _F80CVTToInt(arg, Width);
      } else {
        arg = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
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
    SetRFLAG<FEXCore::X86State::X87FLAG_C1_LOC>(_Constant(0));
    SetRFLAG<FEXCore::X86State::X87FLAG_C2_LOC>(HostFlag_Unordered);
    SetRFLAG<FEXCore::X86State::X87FLAG_C3_LOC>(HostFlag_ZF);
  } else {
    // OF, SF, AF, PF all undefined
    SetCFDirect(HostFlag_CF);
    SetRFLAG<FEXCore::X86State::RFLAG_ZF_RAW_LOC>(HostFlag_ZF);

    // PF is stored inverted, so invert from the host flag.
    // TODO: This could perhaps be optimized?
    auto PF = _Xor(OpSize::i32Bit, HostFlag_Unordered, _Constant(1));
    SetRFLAG<FEXCore::X86State::RFLAG_PF_RAW_LOC>(PF);
  }

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
  SetRFLAG<FEXCore::X86State::X87FLAG_C1_LOC>(_Constant(0));
  SetRFLAG<FEXCore::X86State::X87FLAG_C2_LOC>(HostFlag_Unordered);
  SetRFLAG<FEXCore::X86State::X87FLAG_C3_LOC>(HostFlag_ZF);
}

void OpDispatchBuilder::X87OpHelper(OpcodeArgs, FEXCore::IR::IROps IROp, bool ZeroC2) {
  DeriveOp(Result, IROp, _F80SCALEStack());
  if (ZeroC2) {
    SetRFLAG<FEXCore::X86State::X87FLAG_C2_LOC>(_Constant(0));
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
  Ref FSW = _Lshl(OpSize::i64Bit, Top, _Constant(11));

  // We must construct the FSW from our various bits
  auto C0 = GetRFLAG(FEXCore::X86State::X87FLAG_C0_LOC);
  FSW = _Orlshl(OpSize::i64Bit, FSW, C0, 8);

  auto C1 = GetRFLAG(FEXCore::X86State::X87FLAG_C1_LOC);
  FSW = _Orlshl(OpSize::i64Bit, FSW, C1, 9);

  auto C2 = GetRFLAG(FEXCore::X86State::X87FLAG_C2_LOC);
  FSW = _Orlshl(OpSize::i64Bit, FSW, C2, 10);

  auto C3 = GetRFLAG(FEXCore::X86State::X87FLAG_C3_LOC);
  FSW = _Orlshl(OpSize::i64Bit, FSW, C3, 14);

  return FSW;
}

// Store Status Word
// There's no load Status Word instruction but you can load it through frstor
// or fldenv.
void OpDispatchBuilder::X87FNSTSW(OpcodeArgs) {
  Ref TopValue = _SyncStackToSlow();
  Ref StatusWord = ReconstructFSW_Helper(TopValue);
  StoreResult(GPRClass, Op, StatusWord, OpSize::iInvalid);
}

void OpDispatchBuilder::FNINIT(OpcodeArgs) {
  auto Zero = _Constant(0);

  if (ReducedPrecisionMode) {
    _SetRoundingMode(Zero, false, Zero);
  }

  // Init FCW to 0x037F
  auto NewFCW = _Constant(OpSize::i16Bit, 0x037F);
  _StoreContext(OpSize::i16Bit, GPRClass, NewFCW, offsetof(FEXCore::Core::CPUState, FCW));

  // Set top to zero
  SetX87Top(Zero);
  // Tags all get marked as invalid
  StoreContext(AbridgedFTWIndex, Zero);

  // Reinits the simulated stack
  _InitStack();

  SetRFLAG<FEXCore::X86State::X87FLAG_C0_LOC>(Zero);
  SetRFLAG<FEXCore::X86State::X87FLAG_C1_LOC>(Zero);
  SetRFLAG<FEXCore::X86State::X87FLAG_C2_LOC>(Zero);
  SetRFLAG<FEXCore::X86State::X87FLAG_C3_LOC>(Zero);
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

  auto ZeroConst = _Constant(0);
  auto AllOneConst = _Constant(0xffff'ffff'ffff'ffffull);

  Ref SrcCond = SelectCC(CC, OpSize::i64Bit, AllOneConst, ZeroConst);
  Ref VecCond = _VDupFromGPR(OpSize::i128Bit, OpSize::i64Bit, SrcCond);
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
  auto ZeroConst = _Constant(0);
  auto OneConst = _Constant(1);

  // In the case of top being invalid then C3:C2:C0 is 0b101
  auto C3 = _Select(FEXCore::IR::COND_NEQ, TopValid, OneConst, OneConst, ZeroConst);

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
