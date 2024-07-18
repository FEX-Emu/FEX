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

#include <stddef.h>
#include <stdint.h>

namespace FEXCore::IR {
class OrderedNode;

#define OpcodeArgs [[maybe_unused]] FEXCore::X86Tables::DecodedOp Op

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

Ref OpDispatchBuilder::GetX87FTW() {
  Ref AbridgedFTW = LoadContext(AbridgedFTWIndex);
  Ref FTW = _Constant(0);

  for (int i = 0; i < 8; i++) {
    const auto RegTag = GetX87Tag(_Constant(i), AbridgedFTW);
    FTW = _Orlshl(OpSize::i32Bit, FTW, RegTag, i * 2);
  }

  return FTW;
}

void OpDispatchBuilder::SetX87Top(Ref Value) {
  _StoreContext(1, GPRClass, Value, offsetof(FEXCore::Core::CPUState, flags) + FEXCore::X86State::X87FLAG_TOP_LOC);
}

Ref OpDispatchBuilder::ReconstructFSW() {
  // We must construct the FSW from our various bits
  auto C0 = GetRFLAG(FEXCore::X86State::X87FLAG_C0_LOC);
  auto C1 = GetRFLAG(FEXCore::X86State::X87FLAG_C1_LOC);
  auto C2 = GetRFLAG(FEXCore::X86State::X87FLAG_C2_LOC);
  auto C3 = GetRFLAG(FEXCore::X86State::X87FLAG_C3_LOC);

  Ref FSW = _Lshl(OpSize::i64Bit, C0, _Constant(8));
  FSW = _Orlshl(OpSize::i64Bit, FSW, C1, 9);
  FSW = _Orlshl(OpSize::i64Bit, FSW, C2, 10);
  FSW = _Orlshl(OpSize::i64Bit, FSW, C3, 14);

  auto Top = GetX87Top();
  FSW = _Bfi(OpSize::i64Bit, 3, 11, FSW, Top);
  return FSW;
}

Ref OpDispatchBuilder::ReconstructX87StateFromFSW(Ref FSW) {
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

void OpDispatchBuilder::FLD(OpcodeArgs, size_t width) {
  // Update TOP
  auto orig_top = GetX87Top();
  auto mask = _Constant(7);

  size_t read_width = (width == 80) ? 16 : width / 8;

  Ref data {};

  if (!Op->Src[0].IsNone()) {
    // Read from memory
    data = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], read_width, Op->Flags);
  } else {
    // Implicit arg
    auto offset = _Constant(Op->OP & 7);
    data = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, orig_top, offset), mask);
    data = _LoadContextIndexed(data, 16, MMBaseOffset(), 16, FPRClass);
  }
  Ref converted = data;

  // Convert to 80bit float
  if (width == 32 || width == 64) {
    converted = _F80CVTTo(data, width / 8);
  }

  auto top = _And(OpSize::i32Bit, _Sub(OpSize::i32Bit, orig_top, _Constant(1)), mask);
  SetX87ValidTag(top, true);
  SetX87Top(top);
  // Write to ST[TOP]
  _StoreContextIndexed(converted, top, 16, MMBaseOffset(), 16, FPRClass);
}

void OpDispatchBuilder::FBLD(OpcodeArgs) {
  // Update TOP
  auto orig_top = GetX87Top();
  auto mask = _Constant(7);
  auto top = _And(OpSize::i32Bit, _Sub(OpSize::i32Bit, orig_top, _Constant(1)), mask);
  SetX87ValidTag(top, true);
  SetX87Top(top);

  // Read from memory
  Ref data = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], 16, Op->Flags);
  Ref converted = _F80BCDLoad(data);
  _StoreContextIndexed(converted, top, 16, MMBaseOffset(), 16, FPRClass);
}

void OpDispatchBuilder::FBSTP(OpcodeArgs) {
  auto orig_top = GetX87Top();
  auto data = _LoadContextIndexed(orig_top, 16, MMBaseOffset(), 16, FPRClass);

  Ref converted = _F80BCDStore(data);

  StoreResult_WithOpSize(FPRClass, Op, Op->Dest, converted, 10, 1);

  // if we are popping then we must first mark this location as empty
  SetX87ValidTag(orig_top, false);
  auto top = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, orig_top, _Constant(1)), _Constant(7));
  SetX87Top(top);
}

void OpDispatchBuilder::FLD_Const(OpcodeArgs, NamedVectorConstant constant) {
  // Update TOP
  auto orig_top = GetX87Top();
  auto top = _And(OpSize::i32Bit, _Sub(OpSize::i32Bit, orig_top, _Constant(1)), _Constant(7));
  SetX87ValidTag(top, true);
  SetX87Top(top);

  Ref data = LoadAndCacheNamedVectorConstant(16, constant);

  // Write to ST[TOP]
  _StoreContextIndexed(data, top, 16, MMBaseOffset(), 16, FPRClass);
}

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
  auto sign = _NZCVSelect(OpSize::i64Bit, CondClassType {COND_SLT}, _Constant(0x8000), zero);
  auto absolute = _Neg(OpSize::i64Bit, data, CondClassType {COND_MI});

  // left justify the absolute interger
  auto shift = _Sub(OpSize::i64Bit, _Constant(63), _FindMSB(IR::OpSize::i64Bit, absolute));
  auto shifted = _Lshl(OpSize::i64Bit, absolute, shift);

  auto adjusted_exponent = _Sub(OpSize::i64Bit, _Constant(0x3fff + 63), shift);
  auto zeroed_exponent = _Select(COND_EQ, absolute, zero, zero, adjusted_exponent);
  auto upper = _Or(OpSize::i64Bit, sign, zeroed_exponent);


  Ref converted = _VCastFromGPR(16, 8, shifted);
  converted = _VInsElement(16, 8, 1, 0, converted, _VCastFromGPR(16, 8, upper));

  // Write to ST[TOP]
  _StoreContextIndexed(converted, top, 16, MMBaseOffset(), 16, FPRClass);
}

void OpDispatchBuilder::FSTWithWidth(OpcodeArgs, size_t width) {
  auto orig_top = GetX87Top();
  auto data = _LoadContextIndexed(orig_top, 16, MMBaseOffset(), 16, FPRClass);
  if (width == 80) {
    StoreResult_WithOpSize(FPRClass, Op, Op->Dest, data, 10, 1);
  } else if (width == 32 || width == 64) {
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

void OpDispatchBuilder::FIST(OpcodeArgs, bool Truncate) {
  auto Size = GetSrcSize(Op);

  auto orig_top = GetX87Top();
  Ref data = _LoadContextIndexed(orig_top, 16, MMBaseOffset(), 16, FPRClass);
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

void OpDispatchBuilder::FADD(OpcodeArgs, size_t width, bool Integer, OpDispatchBuilder::OpResult ResInST0) {
  auto top = GetX87Top();
  Ref StackLocation = top;

  Ref arg {};
  Ref b {};

  auto mask = _Constant(7);

  if (!Op->Src[0].IsNone()) {
    // Memory arg
    if (width == 16 || width == 32 || width == 64) {
      if (Integer) {
        arg = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
        b = _F80CVTToInt(arg, width / 8);
      } else {
        arg = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
        b = _F80CVTTo(arg, width / 8);
      }
    }
  } else {
    // Implicit arg
    auto offset = _Constant(Op->OP & 7);
    arg = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, top, offset), mask);
    if (ResInST0 == OpResult::RES_STI) {
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

void OpDispatchBuilder::FMUL(OpcodeArgs, size_t width, bool Integer, OpDispatchBuilder::OpResult ResInST0) {
  auto top = GetX87Top();
  Ref StackLocation = top;
  Ref arg {};
  Ref b {};

  auto mask = _Constant(7);

  if (!Op->Src[0].IsNone()) {
    // Memory arg

    if (width == 16 || width == 32 || width == 64) {
      if (Integer) {
        arg = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
        b = _F80CVTToInt(arg, width / 8);
      } else {
        arg = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
        b = _F80CVTTo(arg, width / 8);
      }
    }
  } else {
    // Implicit arg
    auto offset = _Constant(Op->OP & 7);
    arg = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, top, offset), mask);
    if (ResInST0 == OpResult::RES_STI) {
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

void OpDispatchBuilder::FDIV(OpcodeArgs, size_t width, bool Integer, bool reverse, OpDispatchBuilder::OpResult ResInST0) {
  auto top = GetX87Top();
  Ref StackLocation = top;
  Ref arg {};
  Ref b {};

  auto mask = _Constant(7);

  if (!Op->Src[0].IsNone()) {
    // Memory arg

    if (width == 16 || width == 32 || width == 64) {
      if (Integer) {
        arg = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
        b = _F80CVTToInt(arg, width / 8);
      } else {
        arg = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
        b = _F80CVTTo(arg, width / 8);
      }
    }
  } else {
    // Implicit arg
    auto offset = _Constant(Op->OP & 7);
    arg = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, top, offset), mask);
    if (ResInST0 == OpResult::RES_STI) {
      StackLocation = arg;
    }

    b = _LoadContextIndexed(arg, 16, MMBaseOffset(), 16, FPRClass);
  }

  auto a = _LoadContextIndexed(top, 16, MMBaseOffset(), 16, FPRClass);

  Ref result {};
  if (reverse) {
    result = _F80Div(b, a);
  } else {
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

void OpDispatchBuilder::FSUB(OpcodeArgs, size_t width, bool Integer, bool reverse, OpDispatchBuilder::OpResult ResInST0) {
  auto top = GetX87Top();
  Ref StackLocation = top;
  Ref arg {};
  Ref b {};

  auto mask = _Constant(7);

  if (!Op->Src[0].IsNone()) {
    // Memory arg

    if (width == 16 || width == 32 || width == 64) {
      if (Integer) {
        arg = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
        b = _F80CVTToInt(arg, width / 8);
      } else {
        arg = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
        b = _F80CVTTo(arg, width / 8);
      }
    }
  } else {
    // Implicit arg
    auto offset = _Constant(Op->OP & 7);
    arg = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, top, offset), mask);
    if (ResInST0 == OpResult::RES_STI) {
      StackLocation = arg;
    }
    b = _LoadContextIndexed(arg, 16, MMBaseOffset(), 16, FPRClass);
  }

  auto a = _LoadContextIndexed(top, 16, MMBaseOffset(), 16, FPRClass);

  Ref result {};
  if (reverse) {
    result = _F80Sub(b, a);
  } else {
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

void OpDispatchBuilder::FCHS(OpcodeArgs) {
  auto top = GetX87Top();
  auto a = _LoadContextIndexed(top, 16, MMBaseOffset(), 16, FPRClass);

  auto low = _Constant(0);
  auto high = _Constant(0b1'000'0000'0000'0000ULL);
  Ref data = _VCastFromGPR(16, 8, low);
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
  Ref data = _VCastFromGPR(16, 8, low);
  data = _VInsGPR(16, 8, 1, data, high);

  auto result = _VAnd(16, 1, a, data);

  // Write to ST[TOP]
  _StoreContextIndexed(result, top, 16, MMBaseOffset(), 16, FPRClass);
}

void OpDispatchBuilder::FTST(OpcodeArgs) {
  auto top = GetX87Top();
  auto a = _LoadContextIndexed(top, 16, MMBaseOffset(), 16, FPRClass);

  auto low = _Constant(0);
  Ref data = _VCastFromGPR(16, 8, low);
  Ref Res = _F80Cmp(a, data);

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
  StoreContext(AbridgedFTWIndex, Zero);
}

void OpDispatchBuilder::FCOMI(OpcodeArgs, size_t width, bool Integer, OpDispatchBuilder::FCOMIFlags whichflags, bool poptwice) {
  auto top = GetX87Top();
  auto mask = _Constant(7);

  Ref arg {};
  Ref b {};

  if (!Op->Src[0].IsNone()) {
    // Memory arg
    if (width == 16 || width == 32 || width == 64) {
      if (Integer) {
        arg = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
        b = _F80CVTToInt(arg, width / 8);
      } else {
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
  Ref Res = _F80Cmp(a, b);

  Ref HostFlag_CF = _Bfe(OpSize::i64Bit, 1, FCMP_FLAG_LT, Res);
  Ref HostFlag_ZF = _Bfe(OpSize::i64Bit, 1, FCMP_FLAG_EQ, Res);
  Ref HostFlag_Unordered = _Bfe(OpSize::i64Bit, 1, FCMP_FLAG_UNORDERED, Res);
  HostFlag_CF = _Or(OpSize::i32Bit, HostFlag_CF, HostFlag_Unordered);
  HostFlag_ZF = _Or(OpSize::i32Bit, HostFlag_ZF, HostFlag_Unordered);

  if (whichflags == FCOMIFlags::FLAGS_X87) {
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

  if (poptwice) {
    // if we are popping then we must first mark this location as empty
    SetX87ValidTag(top, false);
    top = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, top, _Constant(1)), mask);
    SetX87ValidTag(top, false);
    // Set the new top now
    top = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, top, _Constant(1)), mask);
    SetX87Top(top);
  } else if ((Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) != 0) {
    // if we are popping then we must first mark this location as empty
    SetX87ValidTag(top, false);
    // Set the new top now
    top = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, top, _Constant(1)), mask);
    SetX87Top(top);
  }
}

void OpDispatchBuilder::FXCH(OpcodeArgs) {
  auto top = GetX87Top();
  Ref arg;

  auto mask = _Constant(7);

  // Implicit arg
  auto offset = _Constant(Op->OP & 7);
  arg = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, top, offset), mask);

  auto a = _LoadContextIndexed(top, 16, MMBaseOffset(), 16, FPRClass);
  auto b = _LoadContextIndexed(arg, 16, MMBaseOffset(), 16, FPRClass);

  // Set C1 to Zero
  SetRFLAG<FEXCore::X86State::X87FLAG_C1_LOC>(_Constant(0));

  // Write to ST[TOP]
  _StoreContextIndexed(b, top, 16, MMBaseOffset(), 16, FPRClass);
  _StoreContextIndexed(a, arg, 16, MMBaseOffset(), 16, FPRClass);
}

void OpDispatchBuilder::FST(OpcodeArgs) {
  auto top = GetX87Top();
  auto mask = _Constant(7);

  // Implicit arg
  auto offset = _Constant(Op->OP & 7);
  Ref arg = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, top, offset), mask);

  auto a = _LoadContextIndexed(top, 16, MMBaseOffset(), 16, FPRClass);

  // Write to ST[i]
  _StoreContextIndexed(a, arg, 16, MMBaseOffset(), 16, FPRClass);

  // Set Tag for ST[i]
  SetX87ValidTag(arg, true);

  if ((Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) != 0) {
    // if we are popping then we must first mark this location as empty
    SetX87ValidTag(top, false);
    top = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, top, _Constant(1)), _Constant(7));
    SetX87Top(top);
  }
}

void OpDispatchBuilder::X87UnaryOp(OpcodeArgs, FEXCore::IR::IROps IROp) {
  auto top = GetX87Top();
  auto a = _LoadContextIndexed(top, 16, MMBaseOffset(), 16, FPRClass);

  DeriveOp(result, IROp, _F80Round(a));

  if (IROp == IR::OP_F80SIN || IROp == IR::OP_F80COS) {
    // TODO: ACCURACY: should check source is in range –2^63 to +2^63
    SetRFLAG<FEXCore::X86State::X87FLAG_C2_LOC>(_Constant(0));
  }

  // Write to ST[TOP]
  _StoreContextIndexed(result, top, 16, MMBaseOffset(), 16, FPRClass);
}

void OpDispatchBuilder::X87BinaryOp(OpcodeArgs, FEXCore::IR::IROps IROp) {
  auto top = GetX87Top();

  auto mask = _Constant(7);
  Ref st1 = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, top, _Constant(1)), mask);

  auto a = _LoadContextIndexed(top, 16, MMBaseOffset(), 16, FPRClass);
  st1 = _LoadContextIndexed(st1, 16, MMBaseOffset(), 16, FPRClass);

  DeriveOp(result, IROp, _F80Add(a, st1));

  if (IROp == IR::OP_F80FPREM || IROp == IR::OP_F80FPREM1) {
    // TODO: Set C0 to Q2, C3 to Q1, C1 to Q0
    SetRFLAG<FEXCore::X86State::X87FLAG_C2_LOC>(_Constant(0));
  }

  // Write to ST[TOP]
  _StoreContextIndexed(result, top, 16, MMBaseOffset(), 16, FPRClass);
}

void OpDispatchBuilder::X87ModifySTP(OpcodeArgs, bool Inc) {
  auto orig_top = GetX87Top();
  if (Inc) {
    auto top = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, orig_top, _Constant(1)), _Constant(7));
    SetX87Top(top);
  } else {
    auto top = _And(OpSize::i32Bit, _Sub(OpSize::i32Bit, orig_top, _Constant(1)), _Constant(7));
    SetX87Top(top);
  }
}

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

  Ref st0 = _LoadContextIndexed(orig_top, 16, MMBaseOffset(), 16, FPRClass);
  Ref st1 = _LoadContextIndexed(top, 16, MMBaseOffset(), 16, FPRClass);

  if (Plus1) {
    Ref data = LoadAndCacheNamedVectorConstant(16, NamedVectorConstant::NAMED_VECTOR_X87_ONE);
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

  Ref data = LoadAndCacheNamedVectorConstant(16, NamedVectorConstant::NAMED_VECTOR_X87_ONE);

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
  Ref st1 = _LoadContextIndexed(top, 16, MMBaseOffset(), 16, FPRClass);

  auto result = _F80ATAN(st1, a);

  // Write to ST[TOP]
  _StoreContextIndexed(result, top, 16, MMBaseOffset(), 16, FPRClass);
}

void OpDispatchBuilder::X87LDENV(OpcodeArgs) {
  const auto Size = GetSrcSize(Op);
  Ref Mem = MakeSegmentAddress(Op, Op->Src[0]);

  auto NewFCW = _LoadMem(GPRClass, 2, Mem, 2);
  _StoreContext(2, GPRClass, NewFCW, offsetof(FEXCore::Core::CPUState, FCW));

  auto NewFSW = _LoadMem(GPRClass, Size, Mem, _Constant(Size * 1), Size, MEM_OFFSET_SXTX, 1);
  ReconstructX87StateFromFSW(NewFSW);

  {
    // FTW
    SetX87FTW(_LoadMem(GPRClass, Size, Mem, _Constant(Size * 2), Size, MEM_OFFSET_SXTX, 1));
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

  const auto Size = GetDstSize(Op);
  Ref Mem = MakeSegmentAddress(Op, Op->Dest);

  {
    auto FCW = _LoadContext(2, GPRClass, offsetof(FEXCore::Core::CPUState, FCW));
    _StoreMem(GPRClass, Size, Mem, FCW, Size);
  }

  { _StoreMem(GPRClass, Size, ReconstructFSW(), Mem, _Constant(Size * 1), Size, MEM_OFFSET_SXTX, 1); }

  auto ZeroConst = _Constant(0);

  {
    // FTW
    _StoreMem(GPRClass, Size, GetX87FTW(), Mem, _Constant(Size * 2), Size, MEM_OFFSET_SXTX, 1);
  }

  {
    // Instruction Offset
    _StoreMem(GPRClass, Size, ZeroConst, Mem, _Constant(Size * 3), Size, MEM_OFFSET_SXTX, 1);
  }

  {
    // Instruction CS selector (+ Opcode)
    _StoreMem(GPRClass, Size, ZeroConst, Mem, _Constant(Size * 4), Size, MEM_OFFSET_SXTX, 1);
  }

  {
    // Data pointer offset
    _StoreMem(GPRClass, Size, ZeroConst, Mem, _Constant(Size * 5), Size, MEM_OFFSET_SXTX, 1);
  }

  {
    // Data pointer selector
    _StoreMem(GPRClass, Size, ZeroConst, Mem, _Constant(Size * 6), Size, MEM_OFFSET_SXTX, 1);
  }
}

void OpDispatchBuilder::X87FLDCW(OpcodeArgs) {
  Ref NewFCW = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
  _StoreContext(2, GPRClass, NewFCW, offsetof(FEXCore::Core::CPUState, FCW));
}

void OpDispatchBuilder::X87FSTCW(OpcodeArgs) {
  auto FCW = _LoadContext(2, GPRClass, offsetof(FEXCore::Core::CPUState, FCW));

  StoreResult(GPRClass, Op, FCW, -1);
}

void OpDispatchBuilder::X87LDSW(OpcodeArgs) {
  Ref NewFSW = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
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

  const auto Size = GetDstSize(Op);
  Ref Mem = MakeSegmentAddress(Op, Op->Dest);
  Ref Top = GetX87Top();
  {
    auto FCW = _LoadContext(2, GPRClass, offsetof(FEXCore::Core::CPUState, FCW));
    _StoreMem(GPRClass, Size, Mem, FCW, Size);
  }

  { _StoreMem(GPRClass, Size, ReconstructFSW(), Mem, _Constant(Size * 1), Size, MEM_OFFSET_SXTX, 1); }

  auto ZeroConst = _Constant(0);

  {
    // FTW
    _StoreMem(GPRClass, Size, GetX87FTW(), Mem, _Constant(Size * 2), Size, MEM_OFFSET_SXTX, 1);
  }

  {
    // Instruction Offset
    _StoreMem(GPRClass, Size, ZeroConst, Mem, _Constant(Size * 3), Size, MEM_OFFSET_SXTX, 1);
  }

  {
    // Instruction CS selector (+ Opcode)
    _StoreMem(GPRClass, Size, ZeroConst, Mem, _Constant(Size * 4), Size, MEM_OFFSET_SXTX, 1);
  }

  {
    // Data pointer offset
    _StoreMem(GPRClass, Size, ZeroConst, Mem, _Constant(Size * 5), Size, MEM_OFFSET_SXTX, 1);
  }

  {
    // Data pointer selector
    _StoreMem(GPRClass, Size, ZeroConst, Mem, _Constant(Size * 6), Size, MEM_OFFSET_SXTX, 1);
  }

  auto OneConst = _Constant(1);
  auto SevenConst = _Constant(7);
  for (int i = 0; i < 7; ++i) {
    auto data = _LoadContextIndexed(Top, 16, MMBaseOffset(), 16, FPRClass);
    _StoreMem(FPRClass, 16, data, Mem, _Constant((Size * 7) + (10 * i)), 1, MEM_OFFSET_SXTX, 1);
    Top = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, Top, OneConst), SevenConst);
  }

  // The final st(7) needs a bit of special handling here
  auto data = _LoadContextIndexed(Top, 16, MMBaseOffset(), 16, FPRClass);
  // ST7 broken in to two parts
  // Lower 64bits [63:0]
  // upper 16 bits [79:64]
  _StoreMem(FPRClass, 8, data, Mem, _Constant((Size * 7) + (7 * 10)), 1, MEM_OFFSET_SXTX, 1);
  auto topBytes = _VDupElement(16, 2, data, 4);
  _StoreMem(FPRClass, 2, topBytes, Mem, _Constant((Size * 7) + (7 * 10) + 8), 1, MEM_OFFSET_SXTX, 1);

  // reset to default
  FNINIT(Op);
}

void OpDispatchBuilder::X87FRSTOR(OpcodeArgs) {
  const auto Size = GetSrcSize(Op);
  Ref Mem = MakeSegmentAddress(Op, Op->Src[0]);

  auto NewFCW = _LoadMem(GPRClass, 2, Mem, 2);
  _StoreContext(2, GPRClass, NewFCW, offsetof(FEXCore::Core::CPUState, FCW));

  auto NewFSW = _LoadMem(GPRClass, Size, Mem, _Constant(Size * 1), Size, MEM_OFFSET_SXTX, 1);
  auto Top = ReconstructX87StateFromFSW(NewFSW);
  {
    // FTW
    SetX87FTW(_LoadMem(GPRClass, Size, Mem, _Constant(Size * 2), Size, MEM_OFFSET_SXTX, 1));
  }

  auto OneConst = _Constant(1);
  auto SevenConst = _Constant(7);

  auto low = _Constant(~0ULL);
  auto high = _Constant(0xFFFF);
  Ref Mask = _VCastFromGPR(16, 8, low);
  Mask = _VInsGPR(16, 8, 1, Mask, high);

  for (int i = 0; i < 7; ++i) {
    Ref Reg = _LoadMem(FPRClass, 16, Mem, _Constant((Size * 7) + (10 * i)), 1, MEM_OFFSET_SXTX, 1);
    // Mask off the top bits
    Reg = _VAnd(16, 16, Reg, Mask);

    _StoreContextIndexed(Reg, Top, 16, MMBaseOffset(), 16, FPRClass);

    Top = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, Top, OneConst), SevenConst);
  }

  // The final st(7) needs a bit of special handling here
  // ST7 broken in to two parts
  // Lower 64bits [63:0]
  // upper 16 bits [79:64]

  Ref Reg = _LoadMem(FPRClass, 8, Mem, _Constant((Size * 7) + (10 * 7)), 1, MEM_OFFSET_SXTX, 1);
  Ref RegHigh = _LoadMem(FPRClass, 2, Mem, _Constant((Size * 7) + (10 * 7) + 8), 1, MEM_OFFSET_SXTX, 1);
  Reg = _VInsElement(16, 2, 4, 0, Reg, RegHigh);
  _StoreContextIndexed(Reg, Top, 16, MMBaseOffset(), 16, FPRClass);
}

void OpDispatchBuilder::X87FXAM(OpcodeArgs) {
  auto top = GetX87Top();
  auto a = _LoadContextIndexed(top, 16, MMBaseOffset(), 16, FPRClass);
  auto Tag = GetX87ValidTag(top);

  ///< Flags return in C3:C2:C1:C0 format
  Ref Flags = _F80XAM(Tag, a);
  SetRFLAG<FEXCore::X86State::X87FLAG_C0_LOC>(_Bfe(OpSize::i32Bit, 1, 0, Flags));
  SetRFLAG<FEXCore::X86State::X87FLAG_C1_LOC>(_Bfe(OpSize::i32Bit, 1, 1, Flags));
  SetRFLAG<FEXCore::X86State::X87FLAG_C2_LOC>(_Bfe(OpSize::i32Bit, 1, 2, Flags));
  SetRFLAG<FEXCore::X86State::X87FLAG_C3_LOC>(_Bfe(OpSize::i32Bit, 1, 3, Flags));
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
  Ref VecCond = _VDupFromGPR(16, 8, SrcCond);

  auto top = GetX87Top();
  Ref arg;

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
  StoreContext(AbridgedFTWIndex, _Constant(0));
}

void OpDispatchBuilder::X87FFREE(OpcodeArgs) {
  // Only sets the selected stack register's tag bits to EMPTY
  Ref top = GetX87Top();

  // Implicit arg
  auto offset = _Constant(Op->OP & 7);
  top = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, top, offset), _Constant(7));

  // Set this argument's tag as empty now
  SetX87ValidTag(top, false);
}

} // namespace FEXCore::IR
