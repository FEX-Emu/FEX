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

//////////////////////?????????<<<< TOREMOVE


Ref OpDispatchBuilder::GetX87Top() {
  // Yes, we are storing 3 bits in a single flag register.
  // Deal with it
  return _LoadContext(1, GPRClass, offsetof(FEXCore::Core::CPUState, flags) + FEXCore::X86State::X87FLAG_TOP_LOC);
}

void OpDispatchBuilder::SetX87ValidTag(Ref Value, bool Valid) {
  // if we are popping then we must first mark this location as empty
  Ref AbridgedFTW = LoadContext(AbridgedFTWIndex);
  Ref RegMask = _Lshl(OpSize::i32Bit, _Constant(1), Value);
  Ref NewAbridgedFTW = Valid ? _Or(OpSize::i32Bit, AbridgedFTW, RegMask) : _Andn(OpSize::i32Bit, AbridgedFTW, RegMask);
  StoreContext(AbridgedFTWIndex, NewAbridgedFTW);
}

Ref OpDispatchBuilder::GetX87ValidTag(Ref Value) {
  Ref AbridgedFTW = LoadContext(AbridgedFTWIndex);
  return _And(OpSize::i32Bit, _Lshr(OpSize::i32Bit, AbridgedFTW, Value), _Constant(1));
}

Ref OpDispatchBuilder::GetX87Tag(Ref Value, Ref AbridgedFTW) {
  Ref RegValid = _And(OpSize::i32Bit, _Lshr(OpSize::i32Bit, AbridgedFTW, Value), _Constant(1));
  Ref X87Empty = _Constant(static_cast<uint8_t>(FPState::X87Tag::Empty));
  Ref X87Valid = _Constant(static_cast<uint8_t>(FPState::X87Tag::Valid));

  return _Select(FEXCore::IR::COND_EQ, RegValid, _Constant(0), X87Empty, X87Valid);
}

Ref OpDispatchBuilder::GetX87Tag(Ref Value) {
  return GetX87Tag(Value, LoadContext(AbridgedFTWIndex));
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
  _StoreContext(1, GPRClass, Value, offsetof(FEXCore::Core::CPUState, flags) + FEXCore::X86State::X87FLAG_TOP_LOC);
}
//////////////////////?????????<<<< TOREMOVE

// Float LoaD operation
template<size_t width>
void OpDispatchBuilder::FLD(OpcodeArgs) {
  static_assert(width == 32 || width == 64 || width == 80, "Unsupported FLD width");

  CurrentHeader->HasX87 = true;
  size_t read_width = (width == 80) ? 16 : width / 8;
  Ref data {};

  if (Op->Src[0].IsNone()) {
    // Implicit arg
    data = _ReadStackValue(Op->OP & 7);
  } else {
    // Read from memory
    data = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], read_width, Op->Flags);

    // Convert to 80bit float
    if constexpr (width == 32 || width == 64) {
      data = _F80CVTTo(data, read_width);
    }
  }
  _PushStack(data, OpSize::i128Bit, true, read_width);
}

template void OpDispatchBuilder::FLD<32>(OpcodeArgs);
template void OpDispatchBuilder::FLD<64>(OpcodeArgs);
template void OpDispatchBuilder::FLD<80>(OpcodeArgs);


void OpDispatchBuilder::FBLD(OpcodeArgs) {
  CurrentHeader->HasX87 = true;

  // Read from memory
  Ref data = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], 16, Op->Flags);
  Ref converted = _F80BCDLoad(data);
  _PushStack(converted, i128Bit, true, 16);
}


void OpDispatchBuilder::FBSTP(OpcodeArgs) {
  CurrentHeader->HasX87 = true;
  Ref converted = _F80BCDStore(_ReadStackValue(0));
  StoreResult_WithOpSize(FPRClass, Op, Op->Dest, converted, 10, 1);

  _PopStackDestroy();
}

template<NamedVectorConstant constant>
void OpDispatchBuilder::FLD_Const(OpcodeArgs) {
  CurrentHeader->HasX87 = true;
  // Update TOP
  Ref data = LoadAndCacheNamedVectorConstant(16, constant);
  _PushStack(data, OpSize::i128Bit, true, 16);
}

template void OpDispatchBuilder::FLD_Const<NamedVectorConstant::NAMED_VECTOR_X87_ONE>(OpcodeArgs);     // 1.0
template void OpDispatchBuilder::FLD_Const<NamedVectorConstant::NAMED_VECTOR_X87_LOG2_10>(OpcodeArgs); // log2l(10)
template void OpDispatchBuilder::FLD_Const<NamedVectorConstant::NAMED_VECTOR_X87_LOG2_E>(OpcodeArgs);  // log2l(e)
template void OpDispatchBuilder::FLD_Const<NamedVectorConstant::NAMED_VECTOR_X87_PI>(OpcodeArgs);      // pi
template void OpDispatchBuilder::FLD_Const<NamedVectorConstant::NAMED_VECTOR_X87_LOG10_2>(OpcodeArgs); // log10l(2)
template void OpDispatchBuilder::FLD_Const<NamedVectorConstant::NAMED_VECTOR_X87_LOG_2>(OpcodeArgs);   // log(2)
template void OpDispatchBuilder::FLD_Const<NamedVectorConstant::NAMED_VECTOR_ZERO>(OpcodeArgs);        // 0.0


void OpDispatchBuilder::FILD(OpcodeArgs) {
  CurrentHeader->HasX87 = true;
  size_t read_width = GetSrcSize(Op);
  // Read from memory
  auto* data = LoadSource_WithOpSize(GPRClass, Op, Op->Src[0], read_width, Op->Flags);

  auto zero = _Constant(0);

  // Sign extend to 64bits
  if (read_width != 8) {
    data = _Sbfe(OpSize::i64Bit, read_width * 8, 0, data);
  }

  // We're about to clobber flags to grab the sign, so save NZCV.
  SaveNZCV();

  // Extract sign and make integer absolute
  _SubNZCV(OpSize::i64Bit, data, zero);
  auto sign = _NZCVSelect(OpSize::i64Bit, CondClassType {COND_SLT}, _Constant(0x8000), zero);
  auto absolute = _Neg(OpSize::i64Bit, data, CondClassType {COND_MI});

  // left justify the absolute integer
  auto shift = _Sub(OpSize::i64Bit, _Constant(63), _FindMSB(IR::OpSize::i64Bit, absolute));
  auto shifted = _Lshl(OpSize::i64Bit, absolute, shift);

  auto adjusted_exponent = _Sub(OpSize::i64Bit, _Constant(0x3fff + 63), shift);
  auto zeroed_exponent = _Select(COND_EQ, absolute, zero, zero, adjusted_exponent);
  auto upper = _Or(OpSize::i64Bit, sign, zeroed_exponent);


  Ref converted = _VCastFromGPR(16, 8, shifted);
  converted = _VInsElement(16, 8, 1, 0, converted, _VCastFromGPR(16, 8, upper));
  _PushStack(converted, OpSize::i128Bit, false, read_width);
}


template<size_t width>
void OpDispatchBuilder::FST(OpcodeArgs) {
  static_assert(width == 32 || width == 64 || width == 80, "Unsupported FST width");
  CurrentHeader->HasX87 = true;

  if (Op->Src[0].IsNone()) { // Destination is stack
    auto offset = Op->OP & 7;
    _StoreStackToStack(offset);
  } else {
    // Destination is memory
    Ref Mem = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.LoadData = false});
    if constexpr (width == 80) {
      _StoreStackMemory(Mem, OpSize::i128Bit, true, 10);
    } else if constexpr (width == 32 || width == 64) {
      _StoreStackMemory(Mem, OpSize::i128Bit, true, width / 8);
    }
  }
  if (Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) {
    _PopStackDestroy();
  }
}

template void OpDispatchBuilder::FST<32>(OpcodeArgs);
template void OpDispatchBuilder::FST<64>(OpcodeArgs);
template void OpDispatchBuilder::FST<80>(OpcodeArgs);


void OpDispatchBuilder::FST(OpcodeArgs) {
  CurrentHeader->HasX87 = true;
  const uint8_t Offset = Op->OP & 7;
  if (Offset != 0) {
    _StoreStackToStack(Offset);
  } else {
    LogMan::Msg::DFmt("FST: _StoreStackToStack(0) is a nop.\n");
  }
  if (Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) {
    _PopStackDestroy();
  }
}


// Store integer to memory (possibly with truncation)
template<bool Truncate>
void OpDispatchBuilder::FIST(OpcodeArgs) {
  CurrentHeader->HasX87 = true;
  auto Size = GetSrcSize(Op);
  // FIXME(pmatos): is there any advantage of using STORESTACKMEMORY here?
  // Do we need STORESTACKMEMORY at all?
  Ref Data = _ReadStackValue(0);
  Data = _F80CVTInt(Size, Data, Truncate);

  StoreResult_WithOpSize(GPRClass, Op, Op->Dest, Data, Size, 1);

  if ((Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) != 0) {
    _PopStackDestroy();
  }
}

template void OpDispatchBuilder::FIST<false>(OpcodeArgs);
template void OpDispatchBuilder::FIST<true>(OpcodeArgs);


template<size_t width, bool Integer, OpDispatchBuilder::OpResult ResInST0>
void OpDispatchBuilder::FADD(OpcodeArgs) {
  static_assert(width == 16 || width == 32 || width == 64 || width == 80, "Unsupported FADD width");

  CurrentHeader->HasX87 = true;

  if (Op->Src[0].IsNone()) { // Implicit argument case
    auto offset = Op->OP & 7;
    auto st0 = 0;
    if constexpr (ResInST0 == OpResult::RES_STI) {
      _F80AddStack(offset, st0);
    } else {
      _F80AddStack(st0, offset);
    }
    if (Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) {
      _PopStackDestroy();
    }
    return;
  }

  // We have one memory argument
  Ref arg {};

  if constexpr (width == 16 || width == 32 || width == 64) {
    if constexpr (Integer) {
      arg = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
      arg = _F80CVTToInt(arg, width / 8);
    } else {
      arg = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
      arg = _F80CVTTo(arg, width / 8);
    }
  }

  // top of stack is at offset zero
  _F80AddValue(0, arg);

  if (Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) {
    _PopStackDestroy();
  }
}

template void OpDispatchBuilder::FADD<32, false, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template void OpDispatchBuilder::FADD<64, false, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template void OpDispatchBuilder::FADD<80, false, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template void OpDispatchBuilder::FADD<80, false, OpDispatchBuilder::OpResult::RES_STI>(OpcodeArgs);

template void OpDispatchBuilder::FADD<16, true, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template void OpDispatchBuilder::FADD<32, true, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);

template<size_t width, bool Integer, OpDispatchBuilder::OpResult ResInST0>
void OpDispatchBuilder::FMUL(OpcodeArgs) {
  static_assert(width == 16 || width == 32 || width == 64 || width == 80, "Unsupported FMUL width");

  CurrentHeader->HasX87 = true;
  if (Op->Src[0].IsNone()) { // Implicit argument case
    auto offset = Op->OP & 7;
    auto st0 = 0;
    if constexpr (ResInST0 == OpResult::RES_STI) {
      _F80MulStack(offset, st0);
    } else {
      _F80MulStack(st0, offset);
    }
    if (Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) {
      _PopStackDestroy();
    }
    return;
  }

  // We have one memory argument
  Ref arg {};

  if constexpr (width == 16 || width == 32 || width == 64) {
    if constexpr (Integer) {
      arg = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
      arg = _F80CVTToInt(arg, width / 8);
    } else {
      arg = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
      arg = _F80CVTTo(arg, width / 8);
    }
  }

  // top of stack is at offset zero
  _F80MulValue(0, arg);

  if (Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) {
    _PopStackDestroy();
  }
}

template void OpDispatchBuilder::FMUL<32, false, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template void OpDispatchBuilder::FMUL<64, false, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template void OpDispatchBuilder::FMUL<80, false, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template void OpDispatchBuilder::FMUL<80, false, OpDispatchBuilder::OpResult::RES_STI>(OpcodeArgs);

template void OpDispatchBuilder::FMUL<16, true, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template void OpDispatchBuilder::FMUL<32, true, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);

template<size_t width, bool Integer, bool reverse, OpDispatchBuilder::OpResult ResInST0>
void OpDispatchBuilder::FDIV(OpcodeArgs) {
  static_assert(width == 16 || width == 32 || width == 64 || width == 80, "Unsupported FDIV width");
  CurrentHeader->HasX87 = true;

  if (Op->Src[0].IsNone()) {
    const auto offset = Op->OP & 7;
    const auto st0 = 0;

    if constexpr (reverse) {
      if constexpr (ResInST0 == OpResult::RES_STI) {
        _F80DivStack(offset, st0, offset);
      } else {
        _F80DivStack(st0, offset, st0);
      }
    } else {
      if constexpr (ResInST0 == OpResult::RES_STI) {
        _F80DivStack(offset, offset, st0);
      } else {
        _F80DivStack(st0, st0, offset);
      }
    }

    if (Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) {
      _PopStackDestroy();
    }
    return;
  }

  // We have one memory argument
  Ref arg {};

  if constexpr (width == 16 || width == 32 || width == 64) {
    if constexpr (Integer) {
      arg = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
      arg = _F80CVTToInt(arg, width / 8);
    } else {
      arg = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
      arg = _F80CVTTo(arg, width / 8);
    }
  }

  // top of stack is at offset zero
  if constexpr (reverse) {
    _F80DivRValue(arg, 0);
  } else {
    _F80DivValue(0, arg);
  }

  if (Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) {
    _PopStackDestroy();
  }
}

template void OpDispatchBuilder::FDIV<32, false, false, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template void OpDispatchBuilder::FDIV<32, false, true, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);

template void OpDispatchBuilder::FDIV<64, false, false, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template void OpDispatchBuilder::FDIV<64, false, true, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);

template void OpDispatchBuilder::FDIV<80, false, false, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template void OpDispatchBuilder::FDIV<80, false, true, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);

template void OpDispatchBuilder::FDIV<80, false, false, OpDispatchBuilder::OpResult::RES_STI>(OpcodeArgs);
template void OpDispatchBuilder::FDIV<80, false, true, OpDispatchBuilder::OpResult::RES_STI>(OpcodeArgs);

template void OpDispatchBuilder::FDIV<16, true, false, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template void OpDispatchBuilder::FDIV<16, true, true, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);

template void OpDispatchBuilder::FDIV<32, true, false, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template void OpDispatchBuilder::FDIV<32, true, true, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);


template<size_t width, bool Integer, bool reverse, OpDispatchBuilder::OpResult ResInST0>
void OpDispatchBuilder::FSUB(OpcodeArgs) {
  static_assert(width == 16 || width == 32 || width == 64 || width == 80, "Unsupported FSUB width");
  CurrentHeader->HasX87 = true;

  if (Op->Src[0].IsNone()) {
    const auto offset = Op->OP & 7;
    const auto st0 = 0;

    if constexpr (reverse) {
      if constexpr (ResInST0 == OpResult::RES_STI) {
        _F80SubStack(offset, st0, offset);
      } else {
        _F80SubStack(st0, offset, st0);
      }
    } else {
      if constexpr (ResInST0 == OpResult::RES_STI) {
        _F80SubStack(offset, offset, st0);
      } else {
        _F80SubStack(st0, st0, offset);
      }
    }

    if (Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) {
      _PopStackDestroy();
    }
    return;
  }

  // We have one memory argument
  Ref arg {};

  if constexpr (width == 16 || width == 32 || width == 64) {
    if constexpr (Integer) {
      arg = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
      arg = _F80CVTToInt(arg, width / 8);
    } else {
      arg = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
      arg = _F80CVTTo(arg, width / 8);
    }
  }

  // top of stack is at offset zero
  if constexpr (reverse) {
    _F80SubRValue(arg, 0);
  } else {
    _F80SubValue(0, arg);
  }

  if (Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) {
    _PopStackDestroy();
  }
}

template void OpDispatchBuilder::FSUB<32, false, false, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template void OpDispatchBuilder::FSUB<32, false, true, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);

template void OpDispatchBuilder::FSUB<64, false, false, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template void OpDispatchBuilder::FSUB<64, false, true, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);

template void OpDispatchBuilder::FSUB<80, false, false, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template void OpDispatchBuilder::FSUB<80, false, true, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);

template void OpDispatchBuilder::FSUB<80, false, false, OpDispatchBuilder::OpResult::RES_STI>(OpcodeArgs);
template void OpDispatchBuilder::FSUB<80, false, true, OpDispatchBuilder::OpResult::RES_STI>(OpcodeArgs);

template void OpDispatchBuilder::FSUB<16, true, false, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template void OpDispatchBuilder::FSUB<16, true, true, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);

template void OpDispatchBuilder::FSUB<32, true, false, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template void OpDispatchBuilder::FSUB<32, true, true, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);

Ref OpDispatchBuilder::GetX87FTW_Helper() {
  Ref AbridgedFTW = _LoadContext(1, GPRClass, offsetof(FEXCore::Core::CPUState, AbridgedFTW));
  Ref FTW = _Constant(0);

  for (int i = 0; i < 8; i++) {
    auto* const RegTag = GetX87Tag(_Constant(i), AbridgedFTW);
    FTW = _Orlshl(OpSize::i32Bit, FTW, RegTag, i * 2);
  }

  return FTW;
}

void OpDispatchBuilder::X87FNSTENV(OpcodeArgs) {
  CurrentHeader->HasX87 = true;

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

  auto Size = GetDstSize(Op);
  Ref Mem = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.LoadData = false});
  Mem = AppendSegmentOffset(Mem, Op->Flags);

  {
    auto FCW = _LoadContext(2, GPRClass, offsetof(FEXCore::Core::CPUState, FCW));
    _StoreMem(GPRClass, Size, Mem, FCW, Size);
  }

  {
    Ref MemLocation = _Add(OpSize::i64Bit, Mem, _Constant(Size * 1));
    _StoreMem(GPRClass, Size, MemLocation, ReconstructFSW_Helper(), Size);
  }

  auto ZeroConst = _Constant(0);

  {
    // FTW
    Ref MemLocation = _Add(OpSize::i64Bit, Mem, _Constant(Size * 2));
    _StoreMem(GPRClass, Size, MemLocation, GetX87FTW_Helper(), Size);
  }

  {
    // Instruction Offset
    Ref MemLocation = _Add(OpSize::i64Bit, Mem, _Constant(Size * 3));
    _StoreMem(GPRClass, Size, MemLocation, ZeroConst, Size);
  }

  {
    // Instruction CS selector (+ Opcode)
    Ref MemLocation = _Add(OpSize::i64Bit, Mem, _Constant(Size * 4));
    _StoreMem(GPRClass, Size, MemLocation, ZeroConst, Size);
  }

  {
    // Data pointer offset
    Ref MemLocation = _Add(OpSize::i64Bit, Mem, _Constant(Size * 5));
    _StoreMem(GPRClass, Size, MemLocation, ZeroConst, Size);
  }

  {
    // Data pointer selector
    Ref MemLocation = _Add(OpSize::i64Bit, Mem, _Constant(Size * 6));
    _StoreMem(GPRClass, Size, MemLocation, ZeroConst, Size);
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
  CurrentHeader->HasX87 = true;
  _StackForceSlow();

  auto Size = GetSrcSize(Op);
  Ref Mem = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, {.LoadData = false});
  Mem = AppendSegmentOffset(Mem, Op->Flags);

  auto NewFCW = _LoadMem(GPRClass, 2, Mem, 2);
  _StoreContext(2, GPRClass, NewFCW, offsetof(FEXCore::Core::CPUState, FCW));

  Ref MemLocation = _Add(OpSize::i64Bit, Mem, _Constant(Size * 1));
  auto NewFSW = _LoadMem(GPRClass, Size, MemLocation, Size);
  ReconstructX87StateFromFSW_Helper(NewFSW);

  {
    // FTW
    Ref MemLocation = _Add(OpSize::i64Bit, Mem, _Constant(Size * 2));
    SetX87FTW(_LoadMem(GPRClass, Size, MemLocation, Size));
  }
}

void OpDispatchBuilder::X87FNSAVE(OpcodeArgs) {
  CurrentHeader->HasX87 = true;
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

  const auto Size = GetDstSize(Op);
  Ref Mem = MakeSegmentAddress(Op, Op->Dest);
  Ref Top = GetX87Top();
  {
    auto FCW = _LoadContext(2, GPRClass, offsetof(FEXCore::Core::CPUState, FCW));
    _StoreMem(GPRClass, Size, Mem, FCW, Size);
  }

  {
    Ref MemLocation = _Add(OpSize::i64Bit, Mem, _Constant(Size * 1));
    _StoreMem(GPRClass, Size, MemLocation, ReconstructFSW_Helper(), Size);
  }

  auto ZeroConst = _Constant(0);

  {
    // FTW
    Ref MemLocation = _Add(OpSize::i64Bit, Mem, _Constant(Size * 2));
    _StoreMem(GPRClass, Size, MemLocation, GetX87FTW_Helper(), Size);
  }

  {
    // Instruction Offset
    Ref MemLocation = _Add(OpSize::i64Bit, Mem, _Constant(Size * 3));
    _StoreMem(GPRClass, Size, MemLocation, ZeroConst, Size);
  }

  {
    // Instruction CS selector (+ Opcode)
    Ref MemLocation = _Add(OpSize::i64Bit, Mem, _Constant(Size * 4));
    _StoreMem(GPRClass, Size, MemLocation, ZeroConst, Size);
  }

  {
    // Data pointer offset
    Ref MemLocation = _Add(OpSize::i64Bit, Mem, _Constant(Size * 5));
    _StoreMem(GPRClass, Size, MemLocation, ZeroConst, Size);
  }

  {
    // Data pointer selector
    Ref MemLocation = _Add(OpSize::i64Bit, Mem, _Constant(Size * 6));
    _StoreMem(GPRClass, Size, MemLocation, ZeroConst, Size);
  }

  Ref ST0Location = _Add(OpSize::i64Bit, Mem, _Constant(Size * 7));

  auto OneConst = _Constant(1);
  auto SevenConst = _Constant(7);
  auto TenConst = _Constant(10);
  for (int i = 0; i < 7; ++i) {
    auto data = _LoadContextIndexed(Top, 16, MMBaseOffset(), 16, FPRClass);
    _StoreMem(FPRClass, 16, ST0Location, data, 1);
    ST0Location = _Add(OpSize::i64Bit, ST0Location, TenConst);
    Top = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, Top, OneConst), SevenConst);
  }

  // The final st(7) needs a bit of special handling here
  auto data = _LoadContextIndexed(Top, 16, MMBaseOffset(), 16, FPRClass);
  // ST7 broken in to two parts
  // Lower 64bits [63:0]
  // upper 16 bits [79:64]
  _StoreMem(FPRClass, 8, ST0Location, data, 1);
  ST0Location = _Add(OpSize::i64Bit, ST0Location, _Constant(8));
  auto topBytes = _VDupElement(16, 2, data, 4);
  _StoreMem(FPRClass, 2, ST0Location, topBytes, 1);

  // reset to default
  FNINIT(Op);
}

void OpDispatchBuilder::X87FRSTOR(OpcodeArgs) {
  CurrentHeader->HasX87 = true;
  _StackForceSlow();
  const auto Size = GetSrcSize(Op);
  Ref Mem = MakeSegmentAddress(Op, Op->Src[0]);

  auto NewFCW = _LoadMem(GPRClass, 2, Mem, 2);
  _StoreContext(2, GPRClass, NewFCW, offsetof(FEXCore::Core::CPUState, FCW));

  Ref MemLocation = _Add(OpSize::i64Bit, Mem, _Constant(Size * 1));
  auto NewFSW = _LoadMem(GPRClass, Size, MemLocation, Size);
  Ref Top = ReconstructX87StateFromFSW_Helper(NewFSW);
  {
    // FTW
    Ref MemLocation = _Add(OpSize::i64Bit, Mem, _Constant(Size * 2));
    SetX87FTW(_LoadMem(GPRClass, Size, MemLocation, Size));
  }

  Ref ST0Location = _Add(OpSize::i64Bit, Mem, _Constant(Size * 7));

  auto OneConst = _Constant(1);
  auto SevenConst = _Constant(7);
  auto TenConst = _Constant(10);

  auto low = _Constant(~0ULL);
  auto high = _Constant(0xFFFF);
  Ref Mask = _VCastFromGPR(16, 8, low);
  Mask = _VInsGPR(16, 8, 1, Mask, high);

  for (int i = 0; i < 7; ++i) {
    Ref Reg = _LoadMem(FPRClass, 16, ST0Location, 1);
    // Mask off the top bits
    Reg = _VAnd(16, 16, Reg, Mask);

    _StoreContextIndexed(Reg, Top, 16, MMBaseOffset(), 16, FPRClass);

    ST0Location = _Add(OpSize::i64Bit, ST0Location, TenConst);
    Top = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, Top, OneConst), SevenConst);
  }

  // The final st(7) needs a bit of special handling here
  // ST7 broken in to two parts
  // Lower 64bits [63:0]
  // upper 16 bits [79:64]

  Ref Reg = _LoadMem(FPRClass, 8, ST0Location, 1);
  ST0Location = _Add(OpSize::i64Bit, ST0Location, _Constant(8));
  Ref RegHigh = _LoadMem(FPRClass, 2, ST0Location, 1);
  Reg = _VInsElement(16, 2, 4, 0, Reg, RegHigh);
  _StoreContextIndexed(Reg, Top, 16, MMBaseOffset(), 16, FPRClass);
}

// Load / Store Control Word
void OpDispatchBuilder::X87FSTCW(OpcodeArgs) {
  auto FCW = _LoadContext(2, GPRClass, offsetof(FEXCore::Core::CPUState, FCW));
  StoreResult(GPRClass, Op, FCW, -1);
}


void OpDispatchBuilder::X87FLDCW(OpcodeArgs) {
  CurrentHeader->HasX87 = true;
  // FIXME: Because loading control flags will affect several instructions in fast path, we might have
  // to switch for now to slow mode whenever these are manually changed.
  // Remove the next line and try DF_04.asm in fast path.
  _StackForceSlow();
  Ref NewFCW = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
  _StoreContext(2, GPRClass, NewFCW, offsetof(FEXCore::Core::CPUState, FCW));
}


void OpDispatchBuilder::FXCH(OpcodeArgs) {
  CurrentHeader->HasX87 = true;
  uint8_t Offset = Op->OP & 7;
  // fxch st0, st0 is for us essentially a nop
  if (Offset != 0) {
    _F80StackXchange(Offset);
  }
  SetRFLAG<FEXCore::X86State::X87FLAG_C1_LOC>(_Constant(0));
}

void OpDispatchBuilder::FCHS(OpcodeArgs) {
  CurrentHeader->HasX87 = true;
  _F80StackChangeSign();
}

void OpDispatchBuilder::FABS(OpcodeArgs) {
  CurrentHeader->HasX87 = true;
  _F80StackAbs();
}

void OpDispatchBuilder::X87FYL2X(OpcodeArgs) {
  CurrentHeader->HasX87 = true;

  if (Op->OP == 0x01F9) { // fyl2xp1
    // create an add between top of stack and 1.
    Ref f80one = LoadAndCacheNamedVectorConstant(16, NamedVectorConstant::NAMED_VECTOR_X87_ONE);
    _F80AddValue(0, f80one);
  }

  _F80StackFYL2X();
}


template<size_t width, bool Integer, OpDispatchBuilder::FCOMIFlags whichflags, bool poptwice>
void OpDispatchBuilder::FCOMI(OpcodeArgs) {
  CurrentHeader->HasX87 = true;
  Ref arg {};
  Ref b {};

  Ref Res = nullptr;
  if (Op->Src[0].IsNone()) {
    // Implicit arg
    uint8_t offset = Op->OP & 7;
    Res = _F80CmpStack(offset, (1 << FCMP_FLAG_EQ) | (1 << FCMP_FLAG_LT) | (1 << FCMP_FLAG_UNORDERED));
  } else {
    // Memory arg
    if constexpr (width == 16 || width == 32 || width == 64) {
      if constexpr (Integer) {
        arg = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
        b = _F80CVTToInt(arg, width / 8);
      } else {
        arg = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
        b = _F80CVTTo(arg, width / 8);
      }
    }
    Res = _F80CmpValue(b, (1 << FCMP_FLAG_EQ) | (1 << FCMP_FLAG_LT) | (1 << FCMP_FLAG_UNORDERED));
  }

  Ref HostFlag_CF = _GetHostFlag(Res, FCMP_FLAG_LT);
  Ref HostFlag_ZF = _GetHostFlag(Res, FCMP_FLAG_EQ);
  Ref HostFlag_Unordered = _GetHostFlag(Res, FCMP_FLAG_UNORDERED);
  HostFlag_CF = _Or(OpSize::i32Bit, HostFlag_CF, HostFlag_Unordered);
  HostFlag_ZF = _Or(OpSize::i32Bit, HostFlag_ZF, HostFlag_Unordered);

  if constexpr (whichflags == FCOMIFlags::FLAGS_X87) {
    SetRFLAG<FEXCore::X86State::X87FLAG_C0_LOC>(HostFlag_CF);
    SetRFLAG<FEXCore::X86State::X87FLAG_C1_LOC>(_Constant(0));
    SetRFLAG<FEXCore::X86State::X87FLAG_C2_LOC>(HostFlag_Unordered);
    SetRFLAG<FEXCore::X86State::X87FLAG_C3_LOC>(HostFlag_ZF);
  } else {
    // Invalidate deferred flags early
    // OF, SF, AF, PF all undefined
    InvalidateDeferredFlags();

    SetRFLAG<FEXCore::X86State::RFLAG_CF_RAW_LOC>(HostFlag_CF);
    SetRFLAG<FEXCore::X86State::RFLAG_ZF_RAW_LOC>(HostFlag_ZF);

    // PF is stored inverted, so invert from the host flag.
    // TODO: This could perhaps be optimized?
    auto PF = _Xor(OpSize::i32Bit, HostFlag_Unordered, _Constant(1));
    SetRFLAG<FEXCore::X86State::RFLAG_PF_RAW_LOC>(PF);
  }

  if constexpr (poptwice) {
    _PopStackDestroy();
    _PopStackDestroy();
  } else if ((Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) != 0) {
    _PopStackDestroy();
  }
}

template void OpDispatchBuilder::FCOMI<32, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>(OpcodeArgs);

template void OpDispatchBuilder::FCOMI<64, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>(OpcodeArgs);

template void OpDispatchBuilder::FCOMI<80, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>(OpcodeArgs);
template void OpDispatchBuilder::FCOMI<80, false, OpDispatchBuilder::FCOMIFlags::FLAGS_RFLAGS, false>(OpcodeArgs);
template void OpDispatchBuilder::FCOMI<80, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, true>(OpcodeArgs);

template void OpDispatchBuilder::FCOMI<16, true, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>(OpcodeArgs);

template void OpDispatchBuilder::FCOMI<32, true, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>(OpcodeArgs);


void OpDispatchBuilder::FTST(OpcodeArgs) {
  CurrentHeader->HasX87 = true;
  Ref Res = _F80StackTest(0, (1 << FCMP_FLAG_EQ) | (1 << FCMP_FLAG_LT) | (1 << FCMP_FLAG_UNORDERED));

  Ref HostFlag_CF = _GetHostFlag(Res, FCMP_FLAG_LT);
  Ref HostFlag_ZF = _GetHostFlag(Res, FCMP_FLAG_EQ);
  Ref HostFlag_Unordered = _GetHostFlag(Res, FCMP_FLAG_UNORDERED);
  HostFlag_CF = _Or(OpSize::i32Bit, HostFlag_CF, HostFlag_Unordered);
  HostFlag_ZF = _Or(OpSize::i32Bit, HostFlag_ZF, HostFlag_Unordered);

  SetRFLAG<FEXCore::X86State::X87FLAG_C0_LOC>(HostFlag_CF);
  SetRFLAG<FEXCore::X86State::X87FLAG_C1_LOC>(_Constant(0));
  SetRFLAG<FEXCore::X86State::X87FLAG_C2_LOC>(HostFlag_Unordered);
  SetRFLAG<FEXCore::X86State::X87FLAG_C3_LOC>(HostFlag_ZF);
}


void OpDispatchBuilder::X87ATAN(OpcodeArgs) {
  CurrentHeader->HasX87 = true;
  _F80ATANStack();
}


void OpDispatchBuilder::FXTRACT(OpcodeArgs) {
  CurrentHeader->HasX87 = true;
  _F80XTRACTStack();
}

// TODO: The following 3 functions were dealt with by a single templatized
// X80BinaryOp<>. Can we redo it as a template?
void OpDispatchBuilder::F80FPREM(OpcodeArgs) {
  CurrentHeader->HasX87 = true;
  if (ReducedPrecisionMode && ImplicitFlagClobber(OP_F64FPREM)) {
    SaveNZCV();
  }

  _F80FPREMStack();
  // TODO: Set C0 to Q2, C3 to Q1, C1 to Q0
  SetRFLAG<FEXCore::X86State::X87FLAG_C2_LOC>(_Constant(0));
}

void OpDispatchBuilder::F80FPREM1(OpcodeArgs) {
  CurrentHeader->HasX87 = true;
  if (ReducedPrecisionMode && ImplicitFlagClobber(OP_F64FPREM1)) {
    SaveNZCV();
  }

  _F80FPREM1Stack();
  // TODO: Set C0 to Q2, C3 to Q1, C1 to Q0
  SetRFLAG<FEXCore::X86State::X87FLAG_C2_LOC>(_Constant(0));
}

void OpDispatchBuilder::F80SCALE(OpcodeArgs) {
  CurrentHeader->HasX87 = true;
  if (ReducedPrecisionMode && ImplicitFlagClobber(OP_F64SCALE)) {
    SaveNZCV();
  }

  _F80SCALEStack();
}

template<bool Inc>
void OpDispatchBuilder::X87ModifySTP(OpcodeArgs) {
  CurrentHeader->HasX87 = true;
  if (Inc) {
    _IncStackTop();
  } else {
    _DecStackTop();
  }
}

template void OpDispatchBuilder::X87ModifySTP<false>(OpcodeArgs);
template void OpDispatchBuilder::X87ModifySTP<true>(OpcodeArgs);

// TODO(pmatos): the next 4 operations used to be abstracted into a UnaryOp templatized op.
// can we abstract it again?
void OpDispatchBuilder::F80SIN(OpcodeArgs) {
  CurrentHeader->HasX87 = true;
  _F80SINStack();

  // TODO: ACCURACY: should check source is in range –2^63 to +2^63
  SetRFLAG<FEXCore::X86State::X87FLAG_C2_LOC>(_Constant(0));
}

void OpDispatchBuilder::F80COS(OpcodeArgs) {
  CurrentHeader->HasX87 = true;
  _F80COSStack();

  // TODO: ACCURACY: should check source is in range –2^63 to +2^63
  SetRFLAG<FEXCore::X86State::X87FLAG_C2_LOC>(_Constant(0));
}

void OpDispatchBuilder::X87SinCos(OpcodeArgs) {
  CurrentHeader->HasX87 = true;

  // Compute the sine and cosine of ST(0); replace ST(0) with the approximate sine, and push the approximate cosine onto the register stack.
  auto st0 = _ReadStackValue(0);
  _F80SINStack();
  if (ReducedPrecisionMode) {
    _PushStack(st0, i64Bit, true, 64);
  } else {
    _PushStack(st0, OpSize::i128Bit, true, 80);
  }
  _F80COSStack();

  // TODO: ACCURACY: should check source is in range –2^63 to +2^63
  SetRFLAG<FEXCore::X86State::X87FLAG_C2_LOC>(_Constant(0));
}

void OpDispatchBuilder::FRNDINT(OpcodeArgs) {
  CurrentHeader->HasX87 = true;
  _F80RoundStack();
}

void OpDispatchBuilder::F80SQRT(OpcodeArgs) {
  CurrentHeader->HasX87 = true;
  _F80SQRTStack();
}

void OpDispatchBuilder::F80F2XM1(OpcodeArgs) {
  CurrentHeader->HasX87 = true;
  _F80F2XM1Stack();
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
  // We must construct the FSW from our various bits
  Ref FSW = _Constant(0);
  auto* Top = T ? T : GetX87Top();
  FSW = _Bfi(OpSize::i64Bit, 3, 11, FSW, Top);

  auto* C0 = GetRFLAG(FEXCore::X86State::X87FLAG_C0_LOC);
  auto* C1 = GetRFLAG(FEXCore::X86State::X87FLAG_C1_LOC);
  auto* C2 = GetRFLAG(FEXCore::X86State::X87FLAG_C2_LOC);
  auto* C3 = GetRFLAG(FEXCore::X86State::X87FLAG_C3_LOC);

  FSW = _Orlshl(OpSize::i64Bit, FSW, C0, 8);
  FSW = _Orlshl(OpSize::i64Bit, FSW, C1, 9);
  FSW = _Orlshl(OpSize::i64Bit, FSW, C2, 10);
  FSW = _Orlshl(OpSize::i64Bit, FSW, C3, 14);
  return FSW;
}

// Store Status Word
// There's no load Status Word instruction
void OpDispatchBuilder::X87FNSTSW(OpcodeArgs) {
  CurrentHeader->HasX87 = true;
  Ref TopValue = _SyncStackToSlow();
  Ref StatusWord = ReconstructFSW_Helper(TopValue);
  StoreResult(GPRClass, Op, StatusWord, -1);
}

void OpDispatchBuilder::FNINIT(OpcodeArgs) {
  CurrentHeader->HasX87 = true;
  auto Zero = _Constant(0);

  // Init FCW to 0x037F
  auto NewFCW = _Constant(16, 0x037F);
  _StoreContext(2, GPRClass, NewFCW, offsetof(FEXCore::Core::CPUState, FCW));

  // Set top to zero
  SetX87Top(Zero);
  // Tags all get marked as invalid
  _StoreContext(1, GPRClass, Zero, offsetof(FEXCore::Core::CPUState, AbridgedFTW));

  // Reinits the simulated stack
  _InitStack();

  SetRFLAG<FEXCore::X86State::X87FLAG_C0_LOC>(Zero);
  SetRFLAG<FEXCore::X86State::X87FLAG_C1_LOC>(Zero);
  SetRFLAG<FEXCore::X86State::X87FLAG_C2_LOC>(Zero);
  SetRFLAG<FEXCore::X86State::X87FLAG_C3_LOC>(Zero);
}

void OpDispatchBuilder::X87FFREE(OpcodeArgs) {
  CurrentHeader->HasX87 = true;
  _InvalidateStack(Op->OP & 7);
}

void OpDispatchBuilder::X87EMMS(OpcodeArgs) {
  // Tags all get set to 0b11
  CurrentHeader->HasX87 = true;
  _InvalidateStack(0xff);
}

void OpDispatchBuilder::X87FCMOV(OpcodeArgs) {
  CurrentHeader->HasX87 = true;
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
  Ref VecCond = _VDupFromGPR(16, 8, SrcCond);
  _F80VBSLStack(16, VecCond, Op->OP & 7, 0);
}

void OpDispatchBuilder::X87FXAM(OpcodeArgs) {
  CurrentHeader->HasX87 = true;

  auto a = _ReadStackValue(0);
  Ref Result = ReducedPrecisionMode ? _VExtractToGPR(8, 8, a, 0) : _VExtractToGPR(16, 8, a, 1);

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

void OpDispatchBuilder::X87TAN(OpcodeArgs) {
  CurrentHeader->HasX87 = true;
  _F80PTANStack();

  // TODO: ACCURACY: should check source is in range –2^63 to +2^63
  SetRFLAG<FEXCore::X86State::X87FLAG_C2_LOC>(_Constant(0));
}
} // namespace FEXCore::IR
