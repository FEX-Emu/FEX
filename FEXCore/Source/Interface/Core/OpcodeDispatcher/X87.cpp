// SPDX-License-Identifier: MIT
/*
$info$
tags: frontend|x86-to-ir, opcodes|dispatcher-implementations
desc: Handles x86/64 x87 to IR
$end_info$
*/

#include "Interface/Core/OpcodeDispatcher.h"
#include "Interface/Core/X86Tables/X86Tables.h"

#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/Utils/EnumUtils.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/FPState.h>
#include <FEXCore/IR/IREmitter.h>

#include <stddef.h>
#include <stdint.h>

namespace FEXCore::IR {
class OrderedNode;

#define OpcodeArgs [[maybe_unused]] FEXCore::X86Tables::DecodedOp Op

OrderedNode *OpDispatchBuilder::GetX87Top() {
  // Yes, we are storing 3 bits in a single flag register.
  // Deal with it
  return _LoadContext(1, GPRClass, offsetof(FEXCore::Core::CPUState, flags) + FEXCore::X86State::X87FLAG_TOP_LOC);
}

void OpDispatchBuilder::SetX87ValidTag(OrderedNode *Value, bool Valid) {
  // if we are popping then we must first mark this location as empty
  OrderedNode *AbridgedFTW = _LoadContext(1, GPRClass, offsetof(FEXCore::Core::CPUState, AbridgedFTW));
  OrderedNode *RegMask = _Lshl(OpSize::i32Bit, _Constant(1), Value);
  OrderedNode *NewAbridgedFTW = Valid ? _Or(OpSize::i32Bit, AbridgedFTW, RegMask) : _Andn(OpSize::i32Bit, AbridgedFTW, RegMask);
  _StoreContext(1, GPRClass, NewAbridgedFTW, offsetof(FEXCore::Core::CPUState, AbridgedFTW));
}

OrderedNode *OpDispatchBuilder::GetX87ValidTag(OrderedNode *Value) {
  OrderedNode *AbridgedFTW = _LoadContext(1, GPRClass, offsetof(FEXCore::Core::CPUState, AbridgedFTW));
  return _And(OpSize::i32Bit, _Lshr(OpSize::i32Bit, AbridgedFTW, Value), _Constant(1));
}

OrderedNode *OpDispatchBuilder::GetX87Tag(OrderedNode *Value, OrderedNode *AbridgedFTW) {
  OrderedNode *RegValid = _And(OpSize::i32Bit, _Lshr(OpSize::i32Bit, AbridgedFTW, Value), _Constant(1));
  OrderedNode *X87Empty = _Constant(static_cast<uint8_t>(FPState::X87Tag::Empty));
  OrderedNode *X87Valid = _Constant(static_cast<uint8_t>(FPState::X87Tag::Valid));

  return _Select(FEXCore::IR::COND_EQ,
    RegValid, _Constant(0),
    X87Empty, X87Valid);
}

OrderedNode *OpDispatchBuilder::GetX87Tag(OrderedNode *Value) {
  OrderedNode *AbridgedFTW = _LoadContext(1, GPRClass, offsetof(FEXCore::Core::CPUState, AbridgedFTW));
  return GetX87Tag(Value, AbridgedFTW);
}

void OpDispatchBuilder::SetX87FTW(OrderedNode *FTW) {
  OrderedNode *X87Empty = _Constant(static_cast<uint8_t>(FPState::X87Tag::Empty));
  OrderedNode *NewAbridgedFTW;

  for (int i = 0; i < 8; i++) {
    OrderedNode *RegTag = _Bfe(OpSize::i32Bit, 2, i * 2, FTW);
    OrderedNode *RegValid = _Select(FEXCore::IR::COND_NEQ,
      RegTag, X87Empty,
      _Constant(1), _Constant(0));

    if (i)
      NewAbridgedFTW = _Orlshl(OpSize::i32Bit, NewAbridgedFTW, RegValid, i);
    else
      NewAbridgedFTW = RegValid;
  }

  _StoreContext(1, GPRClass, NewAbridgedFTW, offsetof(FEXCore::Core::CPUState, AbridgedFTW));
}

OrderedNode *OpDispatchBuilder::GetX87FTW() {
  OrderedNode *AbridgedFTW = _LoadContext(1, GPRClass, offsetof(FEXCore::Core::CPUState, AbridgedFTW));
  OrderedNode *FTW = _Constant(0);

  for (int i = 0; i < 8; i++) {
    const auto RegTag = GetX87Tag(_Constant(i), AbridgedFTW);
    FTW = _Orlshl(OpSize::i32Bit, FTW, RegTag, i * 2);
  }

  return FTW;
}

void OpDispatchBuilder::SetX87Top(OrderedNode *Value) {
  _StoreContext(1, GPRClass, Value, offsetof(FEXCore::Core::CPUState, flags) + FEXCore::X86State::X87FLAG_TOP_LOC);
}

OrderedNode *OpDispatchBuilder::ReconstructFSW() {
  // We must construct the FSW from our various bits
  OrderedNode *FSW = _Constant(0);
  auto Top = GetX87Top();
  FSW = _Bfi(OpSize::i64Bit, 3, 11, FSW, Top);

  auto C0 = GetRFLAG(FEXCore::X86State::X87FLAG_C0_LOC);
  auto C1 = GetRFLAG(FEXCore::X86State::X87FLAG_C1_LOC);
  auto C2 = GetRFLAG(FEXCore::X86State::X87FLAG_C2_LOC);
  auto C3 = GetRFLAG(FEXCore::X86State::X87FLAG_C3_LOC);

  FSW = _Orlshl(OpSize::i64Bit, FSW, C0, 8);
  FSW = _Orlshl(OpSize::i64Bit, FSW, C1, 9);
  FSW = _Orlshl(OpSize::i64Bit, FSW, C2, 10);
  FSW = _Orlshl(OpSize::i64Bit, FSW, C3, 14);
  return FSW;
}

OrderedNode *OpDispatchBuilder::ReconstructX87StateFromFSW(OrderedNode *FSW) {
  auto Top = _Bfe(OpSize::i32Bit, 3, 11, FSW);
  SetX87Top(Top);

  auto C0 = _Bfe(OpSize::i32Bit, 1, 8,  FSW);
  auto C1 = _Bfe(OpSize::i32Bit, 1, 9,  FSW);
  auto C2 = _Bfe(OpSize::i32Bit, 1, 10, FSW);
  auto C3 = _Bfe(OpSize::i32Bit, 1, 14, FSW);

  SetRFLAG<FEXCore::X86State::X87FLAG_C0_LOC>(C0);
  SetRFLAG<FEXCore::X86State::X87FLAG_C1_LOC>(C1);
  SetRFLAG<FEXCore::X86State::X87FLAG_C2_LOC>(C2);
  SetRFLAG<FEXCore::X86State::X87FLAG_C3_LOC>(C3);
  return Top;
}

template<size_t width>
void OpDispatchBuilder::FLD(OpcodeArgs) {
  // Update TOP
  auto orig_top = GetX87Top();
  auto mask = _Constant(7);

  size_t read_width = (width == 80) ? 16 : width / 8;

  OrderedNode *data{};

  if (!Op->Src[0].IsNone()) {
    // Read from memory
    data = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], read_width, Op->Flags);
  }
  else {
    // Implicit arg
    auto offset = _Constant(Op->OP & 7);
    data = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, orig_top, offset), mask);
    data = _LoadContextIndexed(data, 16, MMBaseOffset(), 16, FPRClass);
  }
  OrderedNode *converted = data;

  // Convert to 80bit float
  if constexpr (width == 32 || width == 64) {
    converted = _F80CVTTo(data, width / 8);
  }

  auto top = _And(OpSize::i32Bit, _Sub(OpSize::i32Bit, orig_top, _Constant(1)), mask);
  SetX87ValidTag(top, true);
  SetX87Top(top);
  // Write to ST[TOP]
  _StoreContextIndexed(converted, top, 16, MMBaseOffset(), 16, FPRClass);
  //_StoreContext(converted, 16, offsetof(FEXCore::Core::CPUState, mm[7][0]));
}

template
void OpDispatchBuilder::FLD<32>(OpcodeArgs);
template
void OpDispatchBuilder::FLD<64>(OpcodeArgs);
template
void OpDispatchBuilder::FLD<80>(OpcodeArgs);

void OpDispatchBuilder::FBLD(OpcodeArgs) {
  // Update TOP
  auto orig_top = GetX87Top();
  auto mask = _Constant(7);
  auto top = _And(OpSize::i32Bit, _Sub(OpSize::i32Bit, orig_top, _Constant(1)), mask);
  SetX87ValidTag(top, true);
  SetX87Top(top);

  // Read from memory
  OrderedNode *data = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], 16, Op->Flags);
  OrderedNode *converted = _F80BCDLoad(data);
  _StoreContextIndexed(converted, top, 16, MMBaseOffset(), 16, FPRClass);
}

void OpDispatchBuilder::FBSTP(OpcodeArgs) {
  auto orig_top = GetX87Top();
  auto data = _LoadContextIndexed(orig_top, 16, MMBaseOffset(), 16, FPRClass);

  OrderedNode *converted = _F80BCDStore(data);

  StoreResult_WithOpSize(FPRClass, Op, Op->Dest, converted, 10, 1);

  // if we are popping then we must first mark this location as empty
  SetX87ValidTag(orig_top, false);
  auto top = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, orig_top, _Constant(1)), _Constant(7));
  SetX87Top(top);
}

template<uint64_t Lower, uint32_t Upper>
void OpDispatchBuilder::FLD_Const(OpcodeArgs) {
  // Update TOP
  auto orig_top = GetX87Top();
  auto top = _And(OpSize::i32Bit, _Sub(OpSize::i32Bit, orig_top, _Constant(1)), _Constant(7));
  SetX87ValidTag(top, true);
  SetX87Top(top);

  auto low = _Constant(Lower);
  auto high = _Constant(Upper);
  OrderedNode *data = _VCastFromGPR(16, 8, low);
  data = _VInsGPR(16, 8, 1, data, high);
  // Write to ST[TOP]
  _StoreContextIndexed(data, top, 16, MMBaseOffset(), 16, FPRClass);
}

template
void OpDispatchBuilder::FLD_Const<0x8000'0000'0000'0000ULL, 0b0'011'1111'1111'1111ULL>(OpcodeArgs); // 1.0
template
void OpDispatchBuilder::FLD_Const<0xD49A'784B'CD1B'8AFEULL, 0x4000ULL>(OpcodeArgs); // log2l(10)
template
void OpDispatchBuilder::FLD_Const<0xB8AA'3B29'5C17'F0BCULL, 0x3FFFULL>(OpcodeArgs); // log2l(e)
template
void OpDispatchBuilder::FLD_Const<0xC90F'DAA2'2168'C235ULL, 0x4000ULL>(OpcodeArgs); // pi
template
void OpDispatchBuilder::FLD_Const<0x9A20'9A84'FBCF'F799ULL, 0x3FFDULL>(OpcodeArgs); // log10l(2)
template
void OpDispatchBuilder::FLD_Const<0xB172'17F7'D1CF'79ACULL, 0x3FFEULL>(OpcodeArgs); // log(2)
template
void OpDispatchBuilder::FLD_Const<0, 0>(OpcodeArgs); // 0.0

void OpDispatchBuilder::FILD(OpcodeArgs) {
  // Update TOP
  auto orig_top = GetX87Top();
  auto top = _And(OpSize::i32Bit, _Sub(OpSize::i32Bit, orig_top, _Constant(1)), _Constant(7));
  SetX87ValidTag(top, true);
  SetX87Top(top);

  size_t read_width = GetSrcSize(Op);

  // Read from memory
  auto data = LoadSource_WithOpSize(GPRClass, Op, Op->Src[0], read_width, Op->Flags);

  auto zero = _Constant(0);

  // Sign extend to 64bits
  if (read_width != 8) {
    data = _Sbfe(OpSize::i64Bit, read_width * 8, 0, data);
  }

  // We're about to clobber flags to grab the sign, so save NZCV.
  SaveNZCV();

  // Extract sign and make interger absolute
  _SubNZCV(OpSize::i64Bit, data, zero);
  auto sign = _NZCVSelect(OpSize::i64Bit, CondClassType{COND_SLT}, _Constant(0x8000), zero);
  auto absolute = _Neg(OpSize::i64Bit, data, CondClassType{COND_MI});

  // left justify the absolute interger
  auto shift = _Sub(OpSize::i64Bit, _Constant(63), _FindMSB(IR::OpSize::i64Bit, absolute));
  auto shifted = _Lshl(OpSize::i64Bit, absolute, shift);

  auto adjusted_exponent = _Sub(OpSize::i64Bit, _Constant(0x3fff + 63), shift);
  auto zeroed_exponent = _Select(COND_EQ, absolute, zero, zero, adjusted_exponent);
  auto upper = _Or(OpSize::i64Bit, sign, zeroed_exponent);


  OrderedNode *converted = _VCastFromGPR(16, 8, shifted);
  converted = _VInsElement(16, 8, 1, 0, converted, _VCastFromGPR(16, 8, upper));

  // Write to ST[TOP]
  _StoreContextIndexed(converted, top, 16, MMBaseOffset(), 16, FPRClass);
}

template<size_t width>
void OpDispatchBuilder::FST(OpcodeArgs) {
  auto orig_top = GetX87Top();
  auto data = _LoadContextIndexed(orig_top, 16, MMBaseOffset(), 16, FPRClass);
  if constexpr (width == 80) {
    StoreResult_WithOpSize(FPRClass, Op, Op->Dest, data, 10, 1);
  }
  else if constexpr (width == 32 || width == 64) {
    auto result = _F80CVT(width / 8, data);
    StoreResult_WithOpSize(FPRClass, Op, Op->Dest, result, width / 8, 1);
  }

  if ((Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) != 0) {
    // if we are popping then we must first mark this location as empty
    SetX87ValidTag(orig_top, false);
    // Set the new top now
    auto top = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, orig_top, _Constant(1)), _Constant(7));
    SetX87Top(top);
  }
}

template
void OpDispatchBuilder::FST<32>(OpcodeArgs);
template
void OpDispatchBuilder::FST<64>(OpcodeArgs);
template
void OpDispatchBuilder::FST<80>(OpcodeArgs);

template<bool Truncate>
void OpDispatchBuilder::FIST(OpcodeArgs) {
  auto Size = GetSrcSize(Op);

  auto orig_top = GetX87Top();
  OrderedNode *data = _LoadContextIndexed(orig_top, 16, MMBaseOffset(), 16, FPRClass);
  data = _F80CVTInt(Size, data, Truncate);

  StoreResult_WithOpSize(GPRClass, Op, Op->Dest, data, Size, 1);

  if ((Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) != 0) {
    // if we are popping then we must first mark this location as empty
    SetX87ValidTag(orig_top, false);
    // Set the new top now
    auto top = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, orig_top, _Constant(1)), _Constant(7));
    SetX87Top(top);
  }
}

template
void OpDispatchBuilder::FIST<false>(OpcodeArgs);
template
void OpDispatchBuilder::FIST<true>(OpcodeArgs);

template <size_t width, bool Integer, OpDispatchBuilder::OpResult ResInST0>
void OpDispatchBuilder::FADD(OpcodeArgs) {
  auto top = GetX87Top();
  OrderedNode *StackLocation = top;

  OrderedNode *arg{};
  OrderedNode *b{};

  auto mask = _Constant(7);

  if (!Op->Src[0].IsNone()) {
    // Memory arg
    if constexpr (width == 16 || width == 32 || width == 64) {
      if constexpr (Integer) {
        arg = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
        b = _F80CVTToInt(arg, width / 8);
      }
      else {
        arg = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
        b = _F80CVTTo(arg, width / 8);
      }
    }
  } else {
    // Implicit arg
    auto offset = _Constant(Op->OP & 7);
    arg = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, top, offset), mask);
    if constexpr (ResInST0 == OpResult::RES_STI) {
      StackLocation = arg;
    }
    b = _LoadContextIndexed(arg, 16, MMBaseOffset(), 16, FPRClass);
  }

  auto a = _LoadContextIndexed(top, 16, MMBaseOffset(), 16, FPRClass);
  auto result = _F80Add(a, b);

  if ((Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) != 0) {
    // if we are popping then we must first mark this location as empty
    SetX87ValidTag(top, false);
    // Set the new top now
    top = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, top, _Constant(1)), mask);
    SetX87Top(top);
  }

  // Write to ST[TOP]
  _StoreContextIndexed(result, StackLocation, 16, MMBaseOffset(), 16, FPRClass);
}

template
void OpDispatchBuilder::FADD<32, false, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template
void OpDispatchBuilder::FADD<64, false, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template
void OpDispatchBuilder::FADD<80, false, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template
void OpDispatchBuilder::FADD<80, false, OpDispatchBuilder::OpResult::RES_STI>(OpcodeArgs);

template
void OpDispatchBuilder::FADD<16, true, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template
void OpDispatchBuilder::FADD<32, true, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);

template<size_t width, bool Integer, OpDispatchBuilder::OpResult ResInST0>
void OpDispatchBuilder::FMUL(OpcodeArgs) {
  auto top = GetX87Top();
  OrderedNode *StackLocation = top;
  OrderedNode *arg{};
  OrderedNode *b{};

  auto mask = _Constant(7);

  if (!Op->Src[0].IsNone()) {
    // Memory arg

    if constexpr (width == 16 || width == 32 || width == 64) {
      if constexpr (Integer) {
        arg = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
        b = _F80CVTToInt(arg, width / 8);
      }
      else {
        arg = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
        b = _F80CVTTo(arg, width / 8);
      }
    }
  } else {
    // Implicit arg
    auto offset = _Constant(Op->OP & 7);
    arg = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, top, offset), mask);
    if constexpr (ResInST0 == OpResult::RES_STI) {
      StackLocation = arg;
    }

    b = _LoadContextIndexed(arg, 16, MMBaseOffset(), 16, FPRClass);
  }

  auto a = _LoadContextIndexed(top, 16, MMBaseOffset(), 16, FPRClass);

  auto result = _F80Mul(a, b);

  if ((Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) != 0) {
    // if we are popping then we must first mark this location as empty
    SetX87ValidTag(top, false);
    // Set the new top now
    top = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, top, _Constant(1)), mask);
    SetX87Top(top);
  }

  // Write to ST[TOP]
  _StoreContextIndexed(result, StackLocation, 16, MMBaseOffset(), 16, FPRClass);
}

template
void OpDispatchBuilder::FMUL<32, false, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template
void OpDispatchBuilder::FMUL<64, false, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template
void OpDispatchBuilder::FMUL<80, false, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template
void OpDispatchBuilder::FMUL<80, false, OpDispatchBuilder::OpResult::RES_STI>(OpcodeArgs);

template
void OpDispatchBuilder::FMUL<16, true, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template
void OpDispatchBuilder::FMUL<32, true, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);

template<size_t width, bool Integer, bool reverse, OpDispatchBuilder::OpResult ResInST0>
void OpDispatchBuilder::FDIV(OpcodeArgs) {
  auto top = GetX87Top();
  OrderedNode *StackLocation = top;
  OrderedNode *arg{};
  OrderedNode *b{};

  auto mask = _Constant(7);

  if (!Op->Src[0].IsNone()) {
    // Memory arg

    if constexpr (width == 16 || width == 32 || width == 64) {
      if constexpr (Integer) {
        arg = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
        b = _F80CVTToInt(arg, width / 8);
      }
      else {
        arg = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
        b = _F80CVTTo(arg, width / 8);
      }
    }
  } else {
    // Implicit arg
    auto offset = _Constant(Op->OP & 7);
    arg = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, top, offset), mask);
    if constexpr (ResInST0 == OpResult::RES_STI) {
      StackLocation = arg;
    }

    b = _LoadContextIndexed(arg, 16, MMBaseOffset(), 16, FPRClass);
  }

  auto a = _LoadContextIndexed(top, 16, MMBaseOffset(), 16, FPRClass);

  OrderedNode *result{};
  if constexpr (reverse) {
    result = _F80Div(b, a);
  }
  else {
    result = _F80Div(a, b);
  }

  if ((Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) != 0) {
    // if we are popping then we must first mark this location as empty
    SetX87ValidTag(top, false);
    // Set the new top now
    top = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, top, _Constant(1)), mask);
    SetX87Top(top);
  }

  // Write to ST[TOP]
  _StoreContextIndexed(result, StackLocation, 16, MMBaseOffset(), 16, FPRClass);
}

template
void OpDispatchBuilder::FDIV<32, false, false, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template
void OpDispatchBuilder::FDIV<32, false, true, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);

template
void OpDispatchBuilder::FDIV<64, false, false, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template
void OpDispatchBuilder::FDIV<64, false, true, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);

template
void OpDispatchBuilder::FDIV<80, false, false, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template
void OpDispatchBuilder::FDIV<80, false, true, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);

template
void OpDispatchBuilder::FDIV<80, false, false, OpDispatchBuilder::OpResult::RES_STI>(OpcodeArgs);
template
void OpDispatchBuilder::FDIV<80, false, true, OpDispatchBuilder::OpResult::RES_STI>(OpcodeArgs);

template
void OpDispatchBuilder::FDIV<16, true, false, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template
void OpDispatchBuilder::FDIV<16, true, true, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);

template
void OpDispatchBuilder::FDIV<32, true, false, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template
void OpDispatchBuilder::FDIV<32, true, true, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);

template<size_t width, bool Integer, bool reverse, OpDispatchBuilder::OpResult ResInST0>
void OpDispatchBuilder::FSUB(OpcodeArgs) {
  auto top = GetX87Top();
  OrderedNode *StackLocation = top;
  OrderedNode *arg{};
  OrderedNode *b{};

  auto mask = _Constant(7);

  if (!Op->Src[0].IsNone()) {
    // Memory arg

    if constexpr (width == 16 || width == 32 || width == 64) {
      if constexpr (Integer) {
        arg = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
        b = _F80CVTToInt(arg, width / 8);
      }
      else {
        arg = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
        b = _F80CVTTo(arg, width / 8);
      }
    }
  } else {
    // Implicit arg
    auto offset = _Constant(Op->OP & 7);
    arg = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, top, offset), mask);
    if constexpr (ResInST0 == OpResult::RES_STI) {
      StackLocation = arg;
    }
    b = _LoadContextIndexed(arg, 16, MMBaseOffset(), 16, FPRClass);
  }

  auto a = _LoadContextIndexed(top, 16, MMBaseOffset(), 16, FPRClass);

  OrderedNode *result{};
  if constexpr (reverse) {
    result = _F80Sub(b, a);
  }
  else {
    result = _F80Sub(a, b);
  }

  if ((Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) != 0) {
    // if we are popping then we must first mark this location as empty
    SetX87ValidTag(top, false);
    // Set the new top now

    top = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, top, _Constant(1)), mask);
    SetX87Top(top);
  }

  // Write to ST[TOP]
  _StoreContextIndexed(result, StackLocation, 16, MMBaseOffset(), 16, FPRClass);
}

template
void OpDispatchBuilder::FSUB<32, false, false, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template
void OpDispatchBuilder::FSUB<32, false, true, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);

template
void OpDispatchBuilder::FSUB<64, false, false, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template
void OpDispatchBuilder::FSUB<64, false, true, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);

template
void OpDispatchBuilder::FSUB<80, false, false, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template
void OpDispatchBuilder::FSUB<80, false, true, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);

template
void OpDispatchBuilder::FSUB<80, false, false, OpDispatchBuilder::OpResult::RES_STI>(OpcodeArgs);
template
void OpDispatchBuilder::FSUB<80, false, true, OpDispatchBuilder::OpResult::RES_STI>(OpcodeArgs);

template
void OpDispatchBuilder::FSUB<16, true, false, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template
void OpDispatchBuilder::FSUB<16, true, true, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);

template
void OpDispatchBuilder::FSUB<32, true, false, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template
void OpDispatchBuilder::FSUB<32, true, true, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);

void OpDispatchBuilder::FCHS(OpcodeArgs) {
  auto top = GetX87Top();
  auto a = _LoadContextIndexed(top, 16, MMBaseOffset(), 16, FPRClass);

  auto low = _Constant(0);
  auto high = _Constant(0b1'000'0000'0000'0000ULL);
  OrderedNode *data = _VCastFromGPR(16, 8, low);
  data = _VInsGPR(16, 8, 1, data, high);

  auto result = _VXor(16, 1, a, data);

  // Write to ST[TOP]
  _StoreContextIndexed(result, top, 16, MMBaseOffset(), 16, FPRClass);
}

void OpDispatchBuilder::FABS(OpcodeArgs) {
  auto top = GetX87Top();
  auto a = _LoadContextIndexed(top, 16, MMBaseOffset(), 16, FPRClass);

  auto low = _Constant(~0ULL);
  auto high = _Constant(0b0'111'1111'1111'1111ULL);
  OrderedNode *data = _VCastFromGPR(16, 8, low);
  data = _VInsGPR(16, 8, 1, data, high);

  auto result = _VAnd(16, 1, a, data);

  // Write to ST[TOP]
  _StoreContextIndexed(result, top, 16, MMBaseOffset(), 16, FPRClass);
}

void OpDispatchBuilder::FTST(OpcodeArgs) {
  auto top = GetX87Top();
  auto a = _LoadContextIndexed(top, 16, MMBaseOffset(), 16, FPRClass);

  auto low = _Constant(0);
  OrderedNode *data = _VCastFromGPR(16, 8, low);

  OrderedNode *Res = _F80Cmp(a, data,
    (1 << FCMP_FLAG_EQ) |
    (1 << FCMP_FLAG_LT) |
    (1 << FCMP_FLAG_UNORDERED));

  OrderedNode *HostFlag_CF = _GetHostFlag(Res, FCMP_FLAG_LT);
  OrderedNode *HostFlag_ZF = _GetHostFlag(Res, FCMP_FLAG_EQ);
  OrderedNode *HostFlag_Unordered  = _GetHostFlag(Res, FCMP_FLAG_UNORDERED);
  HostFlag_CF = _Or(OpSize::i32Bit, HostFlag_CF, HostFlag_Unordered);
  HostFlag_ZF = _Or(OpSize::i32Bit, HostFlag_ZF, HostFlag_Unordered);

  SetRFLAG<FEXCore::X86State::X87FLAG_C0_LOC>(HostFlag_CF);
  SetRFLAG<FEXCore::X86State::X87FLAG_C1_LOC>(_Constant(0));
  SetRFLAG<FEXCore::X86State::X87FLAG_C2_LOC>(HostFlag_Unordered);
  SetRFLAG<FEXCore::X86State::X87FLAG_C3_LOC>(HostFlag_ZF);
}

void OpDispatchBuilder::FRNDINT(OpcodeArgs) {
  auto top = GetX87Top();
  auto a = _LoadContextIndexed(top, 16, MMBaseOffset(), 16, FPRClass);

  auto result = _F80Round(a);

  // Write to ST[TOP]
  _StoreContextIndexed(result, top, 16, MMBaseOffset(), 16, FPRClass);
}

void OpDispatchBuilder::FXTRACT(OpcodeArgs) {
  auto orig_top = GetX87Top();
  auto top = _And(OpSize::i32Bit, _Sub(OpSize::i32Bit, orig_top, _Constant(1)), _Constant(7));
  SetX87ValidTag(top, true);
  SetX87Top(top);

  auto a = _LoadContextIndexed(orig_top, 16, MMBaseOffset(), 16, FPRClass);

  auto exp = _F80XTRACT_EXP(a);
  auto sig = _F80XTRACT_SIG(a);

  // Write to ST[TOP]
  _StoreContextIndexed(exp, orig_top, 16, MMBaseOffset(), 16, FPRClass);
  _StoreContextIndexed(sig, top, 16, MMBaseOffset(), 16, FPRClass);
}

void OpDispatchBuilder::FNINIT(OpcodeArgs) {
  auto Zero = _Constant(0);
  // Init FCW to 0x037F
  auto NewFCW = _Constant(16, 0x037F);
  _StoreContext(2, GPRClass, NewFCW, offsetof(FEXCore::Core::CPUState, FCW));

  // Init FSW to 0
  SetX87Top(Zero);

  SetRFLAG<FEXCore::X86State::X87FLAG_C0_LOC>(Zero);
  SetRFLAG<FEXCore::X86State::X87FLAG_C1_LOC>(Zero);
  SetRFLAG<FEXCore::X86State::X87FLAG_C2_LOC>(Zero);
  SetRFLAG<FEXCore::X86State::X87FLAG_C3_LOC>(Zero);

  // Tags all get marked as invalid
  _StoreContext(1, GPRClass, Zero, offsetof(FEXCore::Core::CPUState, AbridgedFTW));
}

template<size_t width, bool Integer, OpDispatchBuilder::FCOMIFlags whichflags, bool poptwice>
void OpDispatchBuilder::FCOMI(OpcodeArgs) {
  auto top = GetX87Top();
  auto mask = _Constant(7);

  OrderedNode *arg{};
  OrderedNode *b{};

  if (!Op->Src[0].IsNone()) {
    // Memory arg
    if constexpr (width == 16 || width == 32 || width == 64) {
      if constexpr (Integer) {
        arg = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
        b = _F80CVTToInt(arg, width / 8);
      }
      else {
        arg = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
        b = _F80CVTTo(arg, width / 8);
      }
    }
  } else {
    // Implicit arg
    auto offset = _Constant(Op->OP & 7);
    arg = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, top, offset), mask);
    b = _LoadContextIndexed(arg, 16, MMBaseOffset(), 16, FPRClass);
  }

  auto a = _LoadContextIndexed(top, 16, MMBaseOffset(), 16, FPRClass);

  OrderedNode *Res = _F80Cmp(a, b,
    (1 << FCMP_FLAG_EQ) |
    (1 << FCMP_FLAG_LT) |
    (1 << FCMP_FLAG_UNORDERED));

  OrderedNode *HostFlag_CF = _GetHostFlag(Res, FCMP_FLAG_LT);
  OrderedNode *HostFlag_ZF = _GetHostFlag(Res, FCMP_FLAG_EQ);
  OrderedNode *HostFlag_Unordered  = _GetHostFlag(Res, FCMP_FLAG_UNORDERED);
  HostFlag_CF = _Or(OpSize::i32Bit, HostFlag_CF, HostFlag_Unordered);
  HostFlag_ZF = _Or(OpSize::i32Bit, HostFlag_ZF, HostFlag_Unordered);

  if constexpr (whichflags == FCOMIFlags::FLAGS_X87) {
    SetRFLAG<FEXCore::X86State::X87FLAG_C0_LOC>(HostFlag_CF);
    SetRFLAG<FEXCore::X86State::X87FLAG_C1_LOC>(_Constant(0));
    SetRFLAG<FEXCore::X86State::X87FLAG_C2_LOC>(HostFlag_Unordered);
    SetRFLAG<FEXCore::X86State::X87FLAG_C3_LOC>(HostFlag_ZF);
  }
  else {
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
    // if we are popping then we must first mark this location as empty
    SetX87ValidTag(top, false);
    top = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, top, _Constant(1)), mask);
    SetX87ValidTag(top, false);
    // Set the new top now
    top = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, top, _Constant(1)), mask);
    SetX87Top(top);
  }
  else if ((Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) != 0) {
    // if we are popping then we must first mark this location as empty
    SetX87ValidTag(top, false);
    // Set the new top now
    top = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, top, _Constant(1)), mask);
    SetX87Top(top);
  }
}

template
void OpDispatchBuilder::FCOMI<32, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>(OpcodeArgs);

template
void OpDispatchBuilder::FCOMI<64, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>(OpcodeArgs);

template
void OpDispatchBuilder::FCOMI<80, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>(OpcodeArgs);
template
void OpDispatchBuilder::FCOMI<80, false, OpDispatchBuilder::FCOMIFlags::FLAGS_RFLAGS, false>(OpcodeArgs);
template
void OpDispatchBuilder::FCOMI<80, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, true>(OpcodeArgs);

template
void OpDispatchBuilder::FCOMI<16, true, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>(OpcodeArgs);

template
void OpDispatchBuilder::FCOMI<32, true, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>(OpcodeArgs);


void OpDispatchBuilder::FXCH(OpcodeArgs) {
  auto top = GetX87Top();
  OrderedNode* arg;

  auto mask = _Constant(7);

  // Implicit arg
  auto offset = _Constant(Op->OP & 7);
  arg = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, top, offset), mask);

  auto a = _LoadContextIndexed(top, 16, MMBaseOffset(), 16, FPRClass);
  auto b = _LoadContextIndexed(arg, 16, MMBaseOffset(), 16, FPRClass);

  // Write to ST[TOP]
  _StoreContextIndexed(b, top, 16, MMBaseOffset(), 16, FPRClass);
  _StoreContextIndexed(a, arg, 16, MMBaseOffset(), 16, FPRClass);
}

void OpDispatchBuilder::FST(OpcodeArgs) {
  auto top = GetX87Top();
  OrderedNode* arg;

  auto mask = _Constant(7);

  // Implicit arg
  auto offset = _Constant(Op->OP & 7);
  arg = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, top, offset), mask);

  auto a = _LoadContextIndexed(top, 16, MMBaseOffset(), 16, FPRClass);

  // Write to ST[TOP]
  _StoreContextIndexed(a, arg, 16, MMBaseOffset(), 16, FPRClass);

  if ((Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) != 0) {
    // if we are popping then we must first mark this location as empty
    SetX87ValidTag(top, false);
    top = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, top, _Constant(1)), _Constant(7));
    SetX87Top(top);
  }
}

template<FEXCore::IR::IROps IROp>
void OpDispatchBuilder::X87UnaryOp(OpcodeArgs) {
  auto top = GetX87Top();
  auto a = _LoadContextIndexed(top, 16, MMBaseOffset(), 16, FPRClass);

  DeriveOp(result, IROp, _F80Round(a));

  if constexpr (IROp == IR::OP_F80SIN ||
                IROp == IR::OP_F80COS) {
    // TODO: ACCURACY: should check source is in range –2^63 to +2^63
    SetRFLAG<FEXCore::X86State::X87FLAG_C2_LOC>(_Constant(0));
  }

  // Write to ST[TOP]
  _StoreContextIndexed(result, top, 16, MMBaseOffset(), 16, FPRClass);
}

template
void OpDispatchBuilder::X87UnaryOp<IR::OP_F80F2XM1>(OpcodeArgs);
template
void OpDispatchBuilder::X87UnaryOp<IR::OP_F80SQRT>(OpcodeArgs);
template
void OpDispatchBuilder::X87UnaryOp<IR::OP_F80SIN>(OpcodeArgs);
template
void OpDispatchBuilder::X87UnaryOp<IR::OP_F80COS>(OpcodeArgs);

template<FEXCore::IR::IROps IROp>
void OpDispatchBuilder::X87BinaryOp(OpcodeArgs) {
  auto top = GetX87Top();

  auto mask = _Constant(7);
  OrderedNode *st1 = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, top, _Constant(1)), mask);

  auto a = _LoadContextIndexed(top, 16, MMBaseOffset(), 16, FPRClass);
  st1 = _LoadContextIndexed(st1, 16, MMBaseOffset(), 16, FPRClass);

  DeriveOp(result, IROp, _F80Add(a, st1));

  if constexpr (IROp == IR::OP_F80FPREM ||
    IROp == IR::OP_F80FPREM1) {
    //TODO: Set C0 to Q2, C3 to Q1, C1 to Q0
    SetRFLAG<FEXCore::X86State::X87FLAG_C2_LOC>(_Constant(0));
  }

  // Write to ST[TOP]
  _StoreContextIndexed(result, top, 16, MMBaseOffset(), 16, FPRClass);
}

template
void OpDispatchBuilder::X87BinaryOp<IR::OP_F80FPREM1>(OpcodeArgs);
template
void OpDispatchBuilder::X87BinaryOp<IR::OP_F80FPREM>(OpcodeArgs);
template
void OpDispatchBuilder::X87BinaryOp<IR::OP_F80SCALE>(OpcodeArgs);

template<bool Inc>
void OpDispatchBuilder::X87ModifySTP(OpcodeArgs) {
  auto orig_top = GetX87Top();
  if (Inc) {
    auto top = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, orig_top, _Constant(1)), _Constant(7));
    SetX87Top(top);
  }
  else {
    auto top = _And(OpSize::i32Bit, _Sub(OpSize::i32Bit, orig_top, _Constant(1)), _Constant(7));
    SetX87Top(top);
  }
}

template
void OpDispatchBuilder::X87ModifySTP<false>(OpcodeArgs);
template
void OpDispatchBuilder::X87ModifySTP<true>(OpcodeArgs);

void OpDispatchBuilder::X87SinCos(OpcodeArgs) {
  auto orig_top = GetX87Top();
  auto top = _And(OpSize::i32Bit, _Sub(OpSize::i32Bit, orig_top, _Constant(1)), _Constant(7));
  SetX87ValidTag(top, true);
  SetX87Top(top);

  auto a = _LoadContextIndexed(orig_top, 16, MMBaseOffset(), 16, FPRClass);

  auto sin = _F80SIN(a);
  auto cos = _F80COS(a);

  // TODO: ACCURACY: should check source is in range –2^63 to +2^63
  SetRFLAG<FEXCore::X86State::X87FLAG_C2_LOC>(_Constant(0));

  // Write to ST[TOP]
  _StoreContextIndexed(sin, orig_top, 16, MMBaseOffset(), 16, FPRClass);
  _StoreContextIndexed(cos, top, 16, MMBaseOffset(), 16, FPRClass);
}

void OpDispatchBuilder::X87FYL2X(OpcodeArgs) {
  bool Plus1 = Op->OP == 0x01F9; // FYL2XP

  auto orig_top = GetX87Top();
  // if we are popping then we must first mark this location as empty
  SetX87ValidTag(orig_top, false);
  auto top = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, orig_top, _Constant(1)), _Constant(7));
  SetX87Top(top);

  OrderedNode *st0 = _LoadContextIndexed(orig_top, 16, MMBaseOffset(), 16, FPRClass);
  OrderedNode *st1 = _LoadContextIndexed(top, 16, MMBaseOffset(), 16, FPRClass);

  if (Plus1) {
    auto low = _Constant(0x8000'0000'0000'0000ULL);
    auto high = _Constant(0b0'011'1111'1111'1111);
    OrderedNode *data = _VCastFromGPR(16, 8, low);
    data = _VInsGPR(16, 8, 1, data, high);
    st0 = _F80Add(st0, data);
  }

  auto result = _F80FYL2X(st0, st1);

  // Write to ST[TOP]
  _StoreContextIndexed(result, top, 16, MMBaseOffset(), 16, FPRClass);
}

void OpDispatchBuilder::X87TAN(OpcodeArgs) {
  auto orig_top = GetX87Top();
  auto top = _And(OpSize::i32Bit, _Sub(OpSize::i32Bit, orig_top, _Constant(1)), _Constant(7));
  SetX87ValidTag(top, true);
  SetX87Top(top);

  auto a = _LoadContextIndexed(orig_top, 16, MMBaseOffset(), 16, FPRClass);

  auto result = _F80TAN(a);

  auto low = _Constant(0x8000'0000'0000'0000ULL);
  auto high = _Constant(0b0'011'1111'1111'1111ULL);
  OrderedNode *data = _VCastFromGPR(16, 8, low);
  data = _VInsGPR(16, 8, 1, data, high);

  // TODO: ACCURACY: should check source is in range –2^63 to +2^63
  SetRFLAG<FEXCore::X86State::X87FLAG_C2_LOC>(_Constant(0));

  // Write to ST[TOP]
  _StoreContextIndexed(result, orig_top, 16, MMBaseOffset(), 16, FPRClass);
  _StoreContextIndexed(data, top, 16, MMBaseOffset(), 16, FPRClass);
}

void OpDispatchBuilder::X87ATAN(OpcodeArgs) {
  auto orig_top = GetX87Top();
  // if we are popping then we must first mark this location as empty
  SetX87ValidTag(orig_top, false);
  auto top = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, orig_top, _Constant(1)), _Constant(7));
  SetX87Top(top);

  auto a = _LoadContextIndexed(orig_top, 16, MMBaseOffset(), 16, FPRClass);
  OrderedNode *st1 = _LoadContextIndexed(top, 16, MMBaseOffset(), 16, FPRClass);

  auto result = _F80ATAN(st1, a);

  // Write to ST[TOP]
  _StoreContextIndexed(result, top, 16, MMBaseOffset(), 16, FPRClass);
}

void OpDispatchBuilder::X87LDENV(OpcodeArgs) {
  auto Size = GetSrcSize(Op);
  OrderedNode *Mem = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, {.LoadData = false});
  Mem = AppendSegmentOffset(Mem, Op->Flags);

  auto NewFCW = _LoadMem(GPRClass, 2, Mem, 2);
  _StoreContext(2, GPRClass, NewFCW, offsetof(FEXCore::Core::CPUState, FCW));

  OrderedNode *MemLocation = _Add(OpSize::i64Bit, Mem, _Constant(Size * 1));
  auto NewFSW = _LoadMem(GPRClass, Size, MemLocation, Size);
  ReconstructX87StateFromFSW(NewFSW);

  {
    // FTW
    OrderedNode *MemLocation = _Add(OpSize::i64Bit, Mem, _Constant(Size * 2));
    SetX87FTW(_LoadMem(GPRClass, Size, MemLocation, Size));
  }
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
  // 2 bytes : instruction pointer selector
  // 2 bytes : Opcode
  // 4 bytes : data pointer offset
  // 4 bytes : data pointer selector

  auto Size = GetDstSize(Op);
  OrderedNode *Mem = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.LoadData = false});
  Mem = AppendSegmentOffset(Mem, Op->Flags);

  {
    auto FCW = _LoadContext(2, GPRClass, offsetof(FEXCore::Core::CPUState, FCW));
    _StoreMem(GPRClass, Size, Mem, FCW, Size);
  }

  {
    OrderedNode *MemLocation = _Add(OpSize::i64Bit, Mem, _Constant(Size * 1));
    _StoreMem(GPRClass, Size, MemLocation, ReconstructFSW(), Size);
  }

  auto ZeroConst = _Constant(0);

  {
    // FTW
    OrderedNode *MemLocation = _Add(OpSize::i64Bit, Mem, _Constant(Size * 2));
    _StoreMem(GPRClass, Size, MemLocation, GetX87FTW(), Size);
  }

  {
    // Instruction Offset
    OrderedNode *MemLocation = _Add(OpSize::i64Bit, Mem, _Constant(Size * 3));
    _StoreMem(GPRClass, Size, MemLocation, ZeroConst, Size);
  }

  {
    // Instruction CS selector (+ Opcode)
    OrderedNode *MemLocation = _Add(OpSize::i64Bit, Mem, _Constant(Size * 4));
    _StoreMem(GPRClass, Size, MemLocation, ZeroConst, Size);
  }

  {
    // Data pointer offset
    OrderedNode *MemLocation = _Add(OpSize::i64Bit, Mem, _Constant(Size * 5));
    _StoreMem(GPRClass, Size, MemLocation, ZeroConst, Size);
  }

  {
    // Data pointer selector
    OrderedNode *MemLocation = _Add(OpSize::i64Bit, Mem, _Constant(Size * 6));
    _StoreMem(GPRClass, Size, MemLocation, ZeroConst, Size);
  }
}

void OpDispatchBuilder::X87FLDCW(OpcodeArgs) {
  OrderedNode *NewFCW = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
  _StoreContext(2, GPRClass, NewFCW, offsetof(FEXCore::Core::CPUState, FCW));
}

void OpDispatchBuilder::X87FSTCW(OpcodeArgs) {
  auto FCW = _LoadContext(2, GPRClass, offsetof(FEXCore::Core::CPUState, FCW));

  StoreResult(GPRClass, Op, FCW, -1);
}

void OpDispatchBuilder::X87LDSW(OpcodeArgs) {
  OrderedNode *NewFSW = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
  ReconstructX87StateFromFSW(NewFSW);
}

void OpDispatchBuilder::X87FNSTSW(OpcodeArgs) {
  StoreResult(GPRClass, Op, ReconstructFSW(), -1);
}

void OpDispatchBuilder::X87FNSAVE(OpcodeArgs) {
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

  auto Size = GetDstSize(Op);
  OrderedNode *Mem = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.LoadData = false});
  Mem = AppendSegmentOffset(Mem, Op->Flags);

  OrderedNode *Top = GetX87Top();
  {
    auto FCW = _LoadContext(2, GPRClass, offsetof(FEXCore::Core::CPUState, FCW));
    _StoreMem(GPRClass, Size, Mem, FCW, Size);
  }

  {
    OrderedNode *MemLocation = _Add(OpSize::i64Bit, Mem, _Constant(Size * 1));
    _StoreMem(GPRClass, Size, MemLocation, ReconstructFSW(), Size);
  }

  auto ZeroConst = _Constant(0);

  {
    // FTW
    OrderedNode *MemLocation = _Add(OpSize::i64Bit, Mem, _Constant(Size * 2));
    _StoreMem(GPRClass, Size, MemLocation, GetX87FTW(), Size);
  }

  {
    // Instruction Offset
    OrderedNode *MemLocation = _Add(OpSize::i64Bit, Mem, _Constant(Size * 3));
    _StoreMem(GPRClass, Size, MemLocation, ZeroConst, Size);
  }

  {
    // Instruction CS selector (+ Opcode)
    OrderedNode *MemLocation = _Add(OpSize::i64Bit, Mem, _Constant(Size * 4));
    _StoreMem(GPRClass, Size, MemLocation, ZeroConst, Size);
  }

  {
    // Data pointer offset
    OrderedNode *MemLocation = _Add(OpSize::i64Bit, Mem, _Constant(Size * 5));
    _StoreMem(GPRClass, Size, MemLocation, ZeroConst, Size);
  }

  {
    // Data pointer selector
    OrderedNode *MemLocation = _Add(OpSize::i64Bit, Mem, _Constant(Size * 6));
    _StoreMem(GPRClass, Size, MemLocation, ZeroConst, Size);
  }

  OrderedNode *ST0Location = _Add(OpSize::i64Bit, Mem, _Constant(Size * 7));

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
  auto Size = GetSrcSize(Op);
  OrderedNode *Mem = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, {.LoadData = false});
  Mem = AppendSegmentOffset(Mem, Op->Flags);

  auto NewFCW = _LoadMem(GPRClass, 2, Mem, 2);
  _StoreContext(2, GPRClass, NewFCW, offsetof(FEXCore::Core::CPUState, FCW));

  OrderedNode *MemLocation = _Add(OpSize::i64Bit, Mem, _Constant(Size * 1));
  auto NewFSW = _LoadMem(GPRClass, Size, MemLocation, Size);
  auto Top = ReconstructX87StateFromFSW(NewFSW);
  {
    // FTW
    OrderedNode *MemLocation = _Add(OpSize::i64Bit, Mem, _Constant(Size * 2));
    SetX87FTW(_LoadMem(GPRClass, Size, MemLocation, Size));
  }

  OrderedNode *ST0Location = _Add(OpSize::i64Bit, Mem, _Constant(Size * 7));

  auto OneConst = _Constant(1);
  auto SevenConst = _Constant(7);
  auto TenConst = _Constant(10);

  auto low = _Constant(~0ULL);
  auto high = _Constant(0xFFFF);
  OrderedNode *Mask = _VCastFromGPR(16, 8, low);
  Mask = _VInsGPR(16, 8, 1, Mask, high);

  for (int i = 0; i < 7; ++i) {
    OrderedNode *Reg = _LoadMem(FPRClass, 16, ST0Location, 1);
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

  OrderedNode *Reg = _LoadMem(FPRClass, 8, ST0Location, 1);
  ST0Location = _Add(OpSize::i64Bit, ST0Location, _Constant(8));
  OrderedNode *RegHigh = _LoadMem(FPRClass, 2, ST0Location, 1);
  Reg = _VInsElement(16, 2, 4, 0, Reg, RegHigh);
  _StoreContextIndexed(Reg, Top, 16, MMBaseOffset(), 16, FPRClass);
}

void OpDispatchBuilder::X87FXAM(OpcodeArgs) {
  auto top = GetX87Top();
  auto a = _LoadContextIndexed(top, 16, MMBaseOffset(), 16, FPRClass);
  OrderedNode *Result = _VExtractToGPR(16, 8, a, 1);

  // Extract the sign bit
  Result = _Bfe(OpSize::i64Bit, 1, 15, Result);
  SetRFLAG<FEXCore::X86State::X87FLAG_C1_LOC>(Result);

  // Claim this is a normal number
  // We don't support anything else
  auto TopValid = GetX87ValidTag(top);
  auto ZeroConst = _Constant(0);
  auto OneConst = _Constant(1);

  // In the case of top being invalid then C3:C2:C0 is 0b101
  auto C3 = _Select(FEXCore::IR::COND_NEQ,
    TopValid, OneConst,
    OneConst, ZeroConst);

  auto C2 = TopValid;
  auto C0 = C3; // Mirror C3 until something other than zero is supported
  SetRFLAG<FEXCore::X86State::X87FLAG_C0_LOC>(C0);
  SetRFLAG<FEXCore::X86State::X87FLAG_C2_LOC>(C2);
  SetRFLAG<FEXCore::X86State::X87FLAG_C3_LOC>(C3);
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
  default:
    LOGMAN_MSG_A_FMT("Unhandled FCMOV op: 0x{:x}", Opcode);
  break;
  }

  auto ZeroConst = _Constant(0);
  auto AllOneConst = _Constant(0xffff'ffff'ffff'ffffull);

  OrderedNode *SrcCond = SelectCC(CC, OpSize::i64Bit, AllOneConst, ZeroConst);
  OrderedNode *VecCond = _VDupFromGPR(16, 8, SrcCond);

  auto top = GetX87Top();
  OrderedNode* arg;

  auto mask = _Constant(7);

  // Implicit arg
  auto offset = _Constant(Op->OP & 7);
  arg = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, top, offset), mask);

  auto a = _LoadContextIndexed(top, 16, MMBaseOffset(), 16, FPRClass);
  auto b = _LoadContextIndexed(arg, 16, MMBaseOffset(), 16, FPRClass);
  auto Result = _VBSL(16, VecCond, b, a);

  // Write to ST[TOP]
  _StoreContextIndexed(Result, top, 16, MMBaseOffset(), 16, FPRClass);
}

void OpDispatchBuilder::X87EMMS(OpcodeArgs) {
  // Tags all get set to 0b11
  _StoreContext(1, GPRClass, _Constant(0), offsetof(FEXCore::Core::CPUState, AbridgedFTW));
}

void OpDispatchBuilder::X87FFREE(OpcodeArgs) {
  // Only sets the selected stack register's tag bits to EMPTY
  OrderedNode *top = GetX87Top();

  // Implicit arg
  auto offset = _Constant(Op->OP & 7);
  top = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, top, offset), _Constant(7));

  // Set this argument's tag as empty now
  SetX87ValidTag(top, false);
}

}
