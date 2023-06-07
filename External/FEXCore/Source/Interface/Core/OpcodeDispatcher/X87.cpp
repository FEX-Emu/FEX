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

void OpDispatchBuilder::SetX87TopTag(OrderedNode *Value, X87Tag Tag) {
  // if we are popping then we must first mark this location as empty
  auto FTW = _LoadContext(2, GPRClass, offsetof(FEXCore::Core::CPUState, FTW));
  OrderedNode *Mask = _Constant(0b11);
  auto TopOffset = _Lshl(Value, _Constant(1));
  Mask = _Lshl(Mask, TopOffset);
  OrderedNode *NewFTW = _Andn(FTW, Mask);
  if (Tag != X87Tag::Valid) {
    auto TagVal = _Lshl(_Constant(ToUnderlying(Tag)), TopOffset);
    NewFTW = _Or(NewFTW, TagVal);
  }

  _StoreContext(2, GPRClass, NewFTW, offsetof(FEXCore::Core::CPUState, FTW));
}

OrderedNode *OpDispatchBuilder::GetX87FTW(OrderedNode *Value) {
  auto FTW = _LoadContext(2, GPRClass, offsetof(FEXCore::Core::CPUState, FTW));
  OrderedNode *Mask = _Constant(0b11);
  auto TopOffset = _Lshl(Value, _Constant(1));
  auto NewFTW = _Lshr(FTW, TopOffset);
  return _And(NewFTW, Mask);
}

void OpDispatchBuilder::SetX87Top(OrderedNode *Value) {
  _StoreContext(1, GPRClass, Value, offsetof(FEXCore::Core::CPUState, flags) + FEXCore::X86State::X87FLAG_TOP_LOC);
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
    data = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], read_width, Op->Flags, -1);
  }
  else {
    // Implicit arg
    auto offset = _Constant(Op->OP & 7);
    data = _And(_Add(orig_top, offset), mask);
    data = _LoadContextIndexed(data, 16, MMBaseOffset(), 16, FPRClass);
  }
  OrderedNode *converted = data;

  // Convert to 80bit float
  if constexpr (width == 32 || width == 64) {
    converted = _F80CVTTo(data, width / 8);
  }

  auto top = _And(_Sub(orig_top, _Constant(1)), mask);
  SetX87TopTag(top, X87Tag::Valid);
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
  auto top = _And(_Sub(orig_top, _Constant(1)), mask);
  SetX87TopTag(top, X87Tag::Valid);
  SetX87Top(top);

  // Read from memory
  OrderedNode *data = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], 16, Op->Flags, -1);
  OrderedNode *converted = _F80BCDLoad(data);
  _StoreContextIndexed(converted, top, 16, MMBaseOffset(), 16, FPRClass);
}

void OpDispatchBuilder::FBSTP(OpcodeArgs) {
  auto orig_top = GetX87Top();
  auto data = _LoadContextIndexed(orig_top, 16, MMBaseOffset(), 16, FPRClass);

  OrderedNode *converted = _F80BCDStore(data);

  StoreResult_WithOpSize(FPRClass, Op, Op->Dest, converted, 10, 1);

  // if we are popping then we must first mark this location as empty
  SetX87TopTag(orig_top, X87Tag::Empty);
  auto top = _And(_Add(orig_top, _Constant(1)), _Constant(7));
  SetX87Top(top);
}

template<uint64_t Lower, uint32_t Upper>
void OpDispatchBuilder::FLD_Const(OpcodeArgs) {
  // Update TOP
  auto orig_top = GetX87Top();
  auto top = _And(_Sub(orig_top, _Constant(1)), _Constant(7));
  SetX87TopTag(top, X87Tag::Valid);
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
  auto top = _And(_Sub(orig_top, _Constant(1)), _Constant(7));
  SetX87TopTag(top, X87Tag::Valid);
  SetX87Top(top);

  size_t read_width = GetSrcSize(Op);

  // Read from memory
  auto data = LoadSource_WithOpSize(GPRClass, Op, Op->Src[0], read_width, Op->Flags, -1);

  auto zero = _Constant(0);

  // Sign extend to 64bits
  if (read_width != 8)
    data = _Sext(read_width * 8, data);

  // Extract sign and make interger absolute
  auto sign = _Select(COND_SLT, data, zero, _Constant(0x8000), zero);
  auto absolute =  _Select(COND_SLT, data, zero, _Sub(zero, data), data);

  // left justify the absolute interger
  auto shift = _Sub(_Constant(63), _FindMSB(absolute));
  auto shifted = _Lshl(absolute, shift);

  auto adjusted_exponent = _Sub(_Constant(0x3fff + 63), shift);
  auto zeroed_exponent = _Select(COND_EQ, absolute, zero, zero, adjusted_exponent);
  auto upper = _Or(sign, zeroed_exponent);


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
    SetX87TopTag(orig_top, X87Tag::Empty);
    // Set the new top now
    auto top = _And(_Add(orig_top, _Constant(1)), _Constant(7));
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
    SetX87TopTag(orig_top, X87Tag::Empty);
    // Set the new top now
    auto top = _And(_Add(orig_top, _Constant(1)), _Constant(7));
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
        arg = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);
        b = _F80CVTToInt(arg, width / 8);
      }
      else {
        arg = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
        b = _F80CVTTo(arg, width / 8);
      }
    }
  } else {
    // Implicit arg
    auto offset = _Constant(Op->OP & 7);
    arg = _And(_Add(top, offset), mask);
    if constexpr (ResInST0 == OpResult::RES_STI) {
      StackLocation = arg;
    }
    b = _LoadContextIndexed(arg, 16, MMBaseOffset(), 16, FPRClass);
  }

  auto a = _LoadContextIndexed(top, 16, MMBaseOffset(), 16, FPRClass);
  auto result = _F80Add(a, b);

  if ((Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) != 0) {
    // if we are popping then we must first mark this location as empty
    SetX87TopTag(top, X87Tag::Empty);
    // Set the new top now
    top = _And(_Add(top, _Constant(1)), mask);
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
        arg = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);
        b = _F80CVTToInt(arg, width / 8);
      }
      else {
        arg = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
        b = _F80CVTTo(arg, width / 8);
      }
    }
  } else {
    // Implicit arg
    auto offset = _Constant(Op->OP & 7);
    arg = _And(_Add(top, offset), mask);
    if constexpr (ResInST0 == OpResult::RES_STI) {
      StackLocation = arg;
    }

    b = _LoadContextIndexed(arg, 16, MMBaseOffset(), 16, FPRClass);
  }

  auto a = _LoadContextIndexed(top, 16, MMBaseOffset(), 16, FPRClass);

  auto result = _F80Mul(a, b);

  if ((Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) != 0) {
    // if we are popping then we must first mark this location as empty
    SetX87TopTag(top, X87Tag::Empty);
    // Set the new top now
    top = _And(_Add(top, _Constant(1)), mask);
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
        arg = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);
        b = _F80CVTToInt(arg, width / 8);
      }
      else {
        arg = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
        b = _F80CVTTo(arg, width / 8);
      }
    }
  } else {
    // Implicit arg
    auto offset = _Constant(Op->OP & 7);
    arg = _And(_Add(top, offset), mask);
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
    SetX87TopTag(top, X87Tag::Empty);
    // Set the new top now
    top = _And(_Add(top, _Constant(1)), mask);
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
        arg = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);
        b = _F80CVTToInt(arg, width / 8);
      }
      else {
        arg = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
        b = _F80CVTTo(arg, width / 8);
      }
    }
  } else {
    // Implicit arg
    auto offset = _Constant(Op->OP & 7);
    arg = _And(_Add(top, offset), mask);
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
    SetX87TopTag(top, X87Tag::Empty);
    // Set the new top now

    top = _And(_Add(top, _Constant(1)), mask);
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
  HostFlag_CF = _Or(HostFlag_CF, HostFlag_Unordered);
  HostFlag_ZF = _Or(HostFlag_ZF, HostFlag_Unordered);

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
  auto top = _And(_Sub(orig_top, _Constant(1)), _Constant(7));
  SetX87TopTag(top, X87Tag::Valid);
  SetX87Top(top);

  auto a = _LoadContextIndexed(orig_top, 16, MMBaseOffset(), 16, FPRClass);

  auto exp = _F80XTRACT_EXP(a);
  auto sig = _F80XTRACT_SIG(a);

  // Write to ST[TOP]
  _StoreContextIndexed(exp, orig_top, 16, MMBaseOffset(), 16, FPRClass);
  _StoreContextIndexed(sig, top, 16, MMBaseOffset(), 16, FPRClass);
}

void OpDispatchBuilder::FNINIT(OpcodeArgs) {
  // Init FCW to 0x037F
  auto NewFCW = _Constant(16, 0x037F);
  _F80LoadFCW(NewFCW);
  _StoreContext(2, GPRClass, NewFCW, offsetof(FEXCore::Core::CPUState, FCW));

  // Init FSW to 0
  SetX87Top(_Constant(0));

  SetRFLAG<FEXCore::X86State::X87FLAG_C0_LOC>(_Constant(0));
  SetRFLAG<FEXCore::X86State::X87FLAG_C1_LOC>(_Constant(0));
  SetRFLAG<FEXCore::X86State::X87FLAG_C2_LOC>(_Constant(0));
  SetRFLAG<FEXCore::X86State::X87FLAG_C3_LOC>(_Constant(0));

  // Tags all get set to 0b11
  _StoreContext(2, GPRClass, _Constant(0xFFFF), offsetof(FEXCore::Core::CPUState, FTW));
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
        arg = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);
        b = _F80CVTToInt(arg, width / 8);
      }
      else {
        arg = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
        b = _F80CVTTo(arg, width / 8);
      }
    }
  } else {
    // Implicit arg
    auto offset = _Constant(Op->OP & 7);
    arg = _And(_Add(top, offset), mask);
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
  HostFlag_CF = _Or(HostFlag_CF, HostFlag_Unordered);
  HostFlag_ZF = _Or(HostFlag_ZF, HostFlag_Unordered);

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

    SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(HostFlag_CF);
    SetRFLAG<FEXCore::X86State::RFLAG_ZF_LOC>(HostFlag_ZF);
    SetRFLAG<FEXCore::X86State::RFLAG_PF_LOC>(HostFlag_Unordered);
  }

  if constexpr (poptwice) {
    // if we are popping then we must first mark this location as empty
    SetX87TopTag(top, X87Tag::Empty);
    top = _And(_Add(top, _Constant(1)), mask);
    SetX87TopTag(top, X87Tag::Empty);
    // Set the new top now
    top = _And(_Add(top, _Constant(1)), mask);
    SetX87Top(top);
  }
  else if ((Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) != 0) {
    // if we are popping then we must first mark this location as empty
    SetX87TopTag(top, X87Tag::Empty);
    // Set the new top now
    top = _And(_Add(top, _Constant(1)), mask);
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
  arg = _And(_Add(top, offset), mask);

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
  arg = _And(_Add(top, offset), mask);

  auto a = _LoadContextIndexed(top, 16, MMBaseOffset(), 16, FPRClass);

  // Write to ST[TOP]
  _StoreContextIndexed(a, arg, 16, MMBaseOffset(), 16, FPRClass);

  if ((Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) != 0) {
    // if we are popping then we must first mark this location as empty
    SetX87TopTag(top, X87Tag::Empty);
    top = _And(_Add(top, _Constant(1)), _Constant(7));
    SetX87Top(top);
  }
}

template<FEXCore::IR::IROps IROp>
void OpDispatchBuilder::X87UnaryOp(OpcodeArgs) {
  auto top = GetX87Top();
  auto a = _LoadContextIndexed(top, 16, MMBaseOffset(), 16, FPRClass);

  auto result = _F80Round(a);
  // Overwrite the op
  result.first->Header.Op = IROp;

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
  OrderedNode *st1 = _And(_Add(top, _Constant(1)), mask);

  auto a = _LoadContextIndexed(top, 16, MMBaseOffset(), 16, FPRClass);
  st1 = _LoadContextIndexed(st1, 16, MMBaseOffset(), 16, FPRClass);

  auto result = _F80Add(a, st1);
  // Overwrite the op
  result.first->Header.Op = IROp;

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
    auto top = _And(_Add(orig_top, _Constant(1)), _Constant(7));
    SetX87Top(top);
  }
  else {
    auto top = _And(_Sub(orig_top, _Constant(1)), _Constant(7));
    SetX87Top(top);
  }
}

template
void OpDispatchBuilder::X87ModifySTP<false>(OpcodeArgs);
template
void OpDispatchBuilder::X87ModifySTP<true>(OpcodeArgs);

void OpDispatchBuilder::X87SinCos(OpcodeArgs) {
  auto orig_top = GetX87Top();
  auto top = _And(_Sub(orig_top, _Constant(1)), _Constant(7));
  SetX87TopTag(top, X87Tag::Valid);
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
  SetX87TopTag(orig_top, X87Tag::Empty);
  auto top = _And(_Add(orig_top, _Constant(1)), _Constant(7));
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
  auto top = _And(_Sub(orig_top, _Constant(1)), _Constant(7));
  SetX87TopTag(top, X87Tag::Valid);
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
  SetX87TopTag(orig_top, X87Tag::Empty);
  auto top = _And(_Add(orig_top, _Constant(1)), _Constant(7));
  SetX87Top(top);

  auto a = _LoadContextIndexed(orig_top, 16, MMBaseOffset(), 16, FPRClass);
  OrderedNode *st1 = _LoadContextIndexed(top, 16, MMBaseOffset(), 16, FPRClass);

  auto result = _F80ATAN(st1, a);

  // Write to ST[TOP]
  _StoreContextIndexed(result, top, 16, MMBaseOffset(), 16, FPRClass);
}

void OpDispatchBuilder::X87LDENV(OpcodeArgs) {
  auto Size = GetSrcSize(Op);
  OrderedNode *Mem = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1, false);
  Mem = AppendSegmentOffset(Mem, Op->Flags);

  auto NewFCW = _LoadMem(GPRClass, 2, Mem, 2);
  _F80LoadFCW(NewFCW);
  _StoreContext(2, GPRClass, NewFCW, offsetof(FEXCore::Core::CPUState, FCW));

  OrderedNode *MemLocation = _Add(Mem, _Constant(Size * 1));
  auto NewFSW = _LoadMem(GPRClass, Size, MemLocation, Size);

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

  {
    // FTW
    OrderedNode *MemLocation = _Add(Mem, _Constant(Size * 2));
    auto NewFTW = _LoadMem(GPRClass, Size, MemLocation, Size);
    _StoreContext(2, GPRClass, NewFTW, offsetof(FEXCore::Core::CPUState, FTW));
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
  OrderedNode *Mem = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1, false);
  Mem = AppendSegmentOffset(Mem, Op->Flags);

  {
    auto FCW = _LoadContext(2, GPRClass, offsetof(FEXCore::Core::CPUState, FCW));
    _StoreMem(GPRClass, Size, Mem, FCW, Size);
  }

  {
    OrderedNode *MemLocation = _Add(Mem, _Constant(Size * 1));
    // We must construct the FSW from our various bits
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
    _StoreMem(GPRClass, Size, MemLocation, FSW, Size);
  }

  auto ZeroConst = _Constant(0);

  {
    // FTW
    OrderedNode *MemLocation = _Add(Mem, _Constant(Size * 2));
    auto FTW = _LoadContext(2, GPRClass, offsetof(FEXCore::Core::CPUState, FTW));
    _StoreMem(GPRClass, Size, MemLocation, FTW, Size);
  }

  {
    // Instruction Offset
    OrderedNode *MemLocation = _Add(Mem, _Constant(Size * 3));
    _StoreMem(GPRClass, Size, MemLocation, ZeroConst, Size);
  }

  {
    // Instruction CS selector (+ Opcode)
    OrderedNode *MemLocation = _Add(Mem, _Constant(Size * 4));
    _StoreMem(GPRClass, Size, MemLocation, ZeroConst, Size);
  }

  {
    // Data pointer offset
    OrderedNode *MemLocation = _Add(Mem, _Constant(Size * 5));
    _StoreMem(GPRClass, Size, MemLocation, ZeroConst, Size);
  }

  {
    // Data pointer selector
    OrderedNode *MemLocation = _Add(Mem, _Constant(Size * 6));
    _StoreMem(GPRClass, Size, MemLocation, ZeroConst, Size);
  }
}

void OpDispatchBuilder::X87FLDCW(OpcodeArgs) {
  OrderedNode *NewFCW = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);
  _F80LoadFCW(NewFCW);
  _StoreContext(2, GPRClass, NewFCW, offsetof(FEXCore::Core::CPUState, FCW));
}

void OpDispatchBuilder::X87FSTCW(OpcodeArgs) {
  auto FCW = _LoadContext(2, GPRClass, offsetof(FEXCore::Core::CPUState, FCW));

  StoreResult(GPRClass, Op, FCW, -1);
}

void OpDispatchBuilder::X87LDSW(OpcodeArgs) {
  OrderedNode *NewFSW = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);
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

void OpDispatchBuilder::X87FNSTSW(OpcodeArgs) {
  // We must construct the FSW from our various bits
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

  StoreResult(GPRClass, Op, FSW, -1);
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
  OrderedNode *Mem = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1, false);
  Mem = AppendSegmentOffset(Mem, Op->Flags);

  OrderedNode *Top = GetX87Top();
  {
    auto FCW = _LoadContext(2, GPRClass, offsetof(FEXCore::Core::CPUState, FCW));
    _StoreMem(GPRClass, Size, Mem, FCW, Size);
  }

  {
    OrderedNode *MemLocation = _Add(Mem, _Constant(Size * 1));
    // We must construct the FSW from our various bits
    OrderedNode *FSW = _Constant(0);
    FSW = _Or(FSW, _Lshl(Top, _Constant(11)));

    auto C0 = GetRFLAG(FEXCore::X86State::X87FLAG_C0_LOC);
    auto C1 = GetRFLAG(FEXCore::X86State::X87FLAG_C1_LOC);
    auto C2 = GetRFLAG(FEXCore::X86State::X87FLAG_C2_LOC);
    auto C3 = GetRFLAG(FEXCore::X86State::X87FLAG_C3_LOC);

    FSW = _Or(FSW, _Lshl(C0, _Constant(8)));
    FSW = _Or(FSW, _Lshl(C1, _Constant(9)));
    FSW = _Or(FSW, _Lshl(C2, _Constant(10)));
    FSW = _Or(FSW, _Lshl(C3, _Constant(14)));
    _StoreMem(GPRClass, Size, MemLocation, FSW, Size);
  }

  auto ZeroConst = _Constant(0);

  {
    // FTW
    OrderedNode *MemLocation = _Add(Mem, _Constant(Size * 2));
    auto FTW = _LoadContext(2, GPRClass, offsetof(FEXCore::Core::CPUState, FTW));
    _StoreMem(GPRClass, Size, MemLocation, FTW, Size);
  }

  {
    // Instruction Offset
    OrderedNode *MemLocation = _Add(Mem, _Constant(Size * 3));
    _StoreMem(GPRClass, Size, MemLocation, ZeroConst, Size);
  }

  {
    // Instruction CS selector (+ Opcode)
    OrderedNode *MemLocation = _Add(Mem, _Constant(Size * 4));
    _StoreMem(GPRClass, Size, MemLocation, ZeroConst, Size);
  }

  {
    // Data pointer offset
    OrderedNode *MemLocation = _Add(Mem, _Constant(Size * 5));
    _StoreMem(GPRClass, Size, MemLocation, ZeroConst, Size);
  }

  {
    // Data pointer selector
    OrderedNode *MemLocation = _Add(Mem, _Constant(Size * 6));
    _StoreMem(GPRClass, Size, MemLocation, ZeroConst, Size);
  }

  OrderedNode *ST0Location = _Add(Mem, _Constant(Size * 7));

  auto OneConst = _Constant(1);
  auto SevenConst = _Constant(7);
  auto TenConst = _Constant(10);
  for (int i = 0; i < 7; ++i) {
    auto data = _LoadContextIndexed(Top, 16, MMBaseOffset(), 16, FPRClass);
    _StoreMem(FPRClass, 16, ST0Location, data, 1);
    ST0Location = _Add(ST0Location, TenConst);
    Top = _And(_Add(Top, OneConst), SevenConst);
  }

  // The final st(7) needs a bit of special handling here
  auto data = _LoadContextIndexed(Top, 16, MMBaseOffset(), 16, FPRClass);
  // ST7 broken in to two parts
  // Lower 64bits [63:0]
  // upper 16 bits [79:64]
  _StoreMem(FPRClass, 8, ST0Location, data, 1);
  ST0Location = _Add(ST0Location, _Constant(8));
  auto topBytes = _VDupElement(16, 2, data, 4);
  _StoreMem(FPRClass, 2, ST0Location, topBytes, 1);

  // reset to default
  FNINIT(Op);
}

void OpDispatchBuilder::X87FRSTOR(OpcodeArgs) {
  auto Size = GetSrcSize(Op);
  OrderedNode *Mem = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1, false);
  Mem = AppendSegmentOffset(Mem, Op->Flags);

  auto NewFCW = _LoadMem(GPRClass, 2, Mem, 2);
  _F80LoadFCW(NewFCW);
  _StoreContext(2, GPRClass, NewFCW, offsetof(FEXCore::Core::CPUState, FCW));

  OrderedNode *MemLocation = _Add(Mem, _Constant(Size * 1));
  auto NewFSW = _LoadMem(GPRClass, Size, MemLocation, Size);

  // Strip out the FSW information
  OrderedNode *Top = _Bfe(3, 11, NewFSW);
  SetX87Top(Top);

  auto C0 = _Bfe(1, 8,  NewFSW);
  auto C1 = _Bfe(1, 9,  NewFSW);
  auto C2 = _Bfe(1, 10, NewFSW);
  auto C3 = _Bfe(1, 14, NewFSW);

  SetRFLAG<FEXCore::X86State::X87FLAG_C0_LOC>(C0);
  SetRFLAG<FEXCore::X86State::X87FLAG_C1_LOC>(C1);
  SetRFLAG<FEXCore::X86State::X87FLAG_C2_LOC>(C2);
  SetRFLAG<FEXCore::X86State::X87FLAG_C3_LOC>(C3);

  {
    // FTW
    OrderedNode *MemLocation = _Add(Mem, _Constant(Size * 2));
    auto NewFTW = _LoadMem(GPRClass, Size, MemLocation, Size);
    _StoreContext(2, GPRClass, NewFTW, offsetof(FEXCore::Core::CPUState, FTW));
  }

  OrderedNode *ST0Location = _Add(Mem, _Constant(Size * 7));

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

    ST0Location = _Add(ST0Location, TenConst);
    Top = _And(_Add(Top, OneConst), SevenConst);
  }

  // The final st(7) needs a bit of special handling here
  // ST7 broken in to two parts
  // Lower 64bits [63:0]
  // upper 16 bits [79:64]

  OrderedNode *Reg = _LoadMem(FPRClass, 8, ST0Location, 1);
  ST0Location = _Add(ST0Location, _Constant(8));
  OrderedNode *RegHigh = _LoadMem(FPRClass, 2, ST0Location, 1);
  Reg = _VInsElement(16, 2, 4, 0, Reg, RegHigh);
  _StoreContextIndexed(Reg, Top, 16, MMBaseOffset(), 16, FPRClass);
}

void OpDispatchBuilder::X87FXAM(OpcodeArgs) {
  auto top = GetX87Top();
  auto a = _LoadContextIndexed(top, 16, MMBaseOffset(), 16, FPRClass);
  OrderedNode *Result = _VExtractToGPR(16, 8, a, 1);

  // Extract the sign bit
  Result = _Lshr(Result, _Constant(15));
  SetRFLAG<FEXCore::X86State::X87FLAG_C1_LOC>(Result);

  // Claim this is a normal number
  // We don't support anything else
  auto FTW = GetX87FTW(top);
  auto X87Zero = _Constant(0b11);
  auto ZeroConst = _Constant(0);
  auto OneConst = _Constant(1);

  // In the case of Zero 0b11 then C3:C2:C0 is 0b101
  auto C3 = _Select(FEXCore::IR::COND_EQ,
    FTW, X87Zero,
    OneConst, ZeroConst);

  auto C2 = _Select(FEXCore::IR::COND_EQ,
    FTW, X87Zero,
    ZeroConst, OneConst);

  auto C0 = C3; // Mirror C3 until something other than zero is supported
  SetRFLAG<FEXCore::X86State::X87FLAG_C0_LOC>(C0);
  SetRFLAG<FEXCore::X86State::X87FLAG_C2_LOC>(C2);
  SetRFLAG<FEXCore::X86State::X87FLAG_C3_LOC>(C3);
}

void OpDispatchBuilder::X87FCMOV(OpcodeArgs) {
  enum CompareType {
    COMPARE_ZERO,
    COMPARE_NOTZERO,
  };
  uint32_t FLAGMask{};
  CompareType Type = COMPARE_ZERO;
  OrderedNode *SrcCond;

  auto ZeroConst = _Constant(0);
  auto OneConst = _Constant(1);

  uint16_t Opcode = Op->OP & 0b1111'1111'1000;
  switch (Opcode) {
  case 0x3'C0:
    FLAGMask = 1 << FEXCore::X86State::RFLAG_CF_LOC;
    Type = COMPARE_ZERO;
  break;
  case 0x2'C0:
    FLAGMask = 1 << FEXCore::X86State::RFLAG_CF_LOC;
    Type = COMPARE_NOTZERO;
  break;
  case 0x2'C8:
    FLAGMask = 1 << FEXCore::X86State::RFLAG_ZF_LOC;
    Type = COMPARE_NOTZERO;
  break;
  case 0x3'C8:
    FLAGMask = 1 << FEXCore::X86State::RFLAG_ZF_LOC;
    Type = COMPARE_ZERO;
  break;
  case 0x2'D0:
    FLAGMask = (1 << FEXCore::X86State::RFLAG_ZF_LOC) | (1 << FEXCore::X86State::RFLAG_CF_LOC);
    Type = COMPARE_NOTZERO;
  break;
  case 0x3'D0:
    FLAGMask = (1 << FEXCore::X86State::RFLAG_ZF_LOC) | (1 << FEXCore::X86State::RFLAG_CF_LOC);
    Type = COMPARE_ZERO;
  break;
  case 0x2'D8:
    FLAGMask = 1 << FEXCore::X86State::RFLAG_PF_LOC;
    Type = COMPARE_NOTZERO;
  break;
  case 0x3'D8:
    FLAGMask = 1 << FEXCore::X86State::RFLAG_PF_LOC;
    Type = COMPARE_ZERO;
  break;
  default:
    LOGMAN_MSG_A_FMT("Unhandled FCMOV op: 0x{:x}", Opcode);
  break;
  }

  auto MaskConst = _Constant(FLAGMask);

  auto RFLAG = GetPackedRFLAG(false);

  auto AndOp = _And(RFLAG, MaskConst);
  switch (Type) {
    case COMPARE_ZERO: {
      SrcCond = _Select(FEXCore::IR::COND_EQ,
      AndOp, ZeroConst, OneConst, ZeroConst);
      break;
    }
    case COMPARE_NOTZERO: {
      SrcCond = _Select(FEXCore::IR::COND_EQ,
      AndOp, ZeroConst, ZeroConst, OneConst);
      break;
    }
  }

  SrcCond = _Sbfe(1, 0, SrcCond);

  OrderedNode *VecCond = _VDupFromGPR(16, 8, SrcCond);

  auto top = GetX87Top();
  OrderedNode* arg;

  auto mask = _Constant(7);

  // Implicit arg
  auto offset = _Constant(Op->OP & 7);
  arg = _And(_Add(top, offset), mask);

  auto a = _LoadContextIndexed(top, 16, MMBaseOffset(), 16, FPRClass);
  auto b = _LoadContextIndexed(arg, 16, MMBaseOffset(), 16, FPRClass);
  auto Result = _VBSL(16, VecCond, b, a);

  // Write to ST[TOP]
  _StoreContextIndexed(Result, top, 16, MMBaseOffset(), 16, FPRClass);
}

void OpDispatchBuilder::X87EMMS(OpcodeArgs) {
  // Tags all get set to 0b11
  _StoreContext(2, GPRClass, _Constant(0xFFFF), offsetof(FEXCore::Core::CPUState, FTW));
}

void OpDispatchBuilder::X87FFREE(OpcodeArgs) {
  // Only sets the selected stack register's tag bits to EMPTY
  OrderedNode *top = GetX87Top();

  // Implicit arg
  auto offset = _Constant(Op->OP & 7);
  top = _And(_Add(top, offset), _Constant(7));

  // Set this argument's tag as empty now
  SetX87TopTag(top, X87Tag::Empty);
}

}
