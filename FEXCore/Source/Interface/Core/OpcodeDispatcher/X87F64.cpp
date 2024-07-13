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

#include <stddef.h>
#include <stdint.h>

namespace FEXCore::IR {
class OrderedNode;

#define OpcodeArgs [[maybe_unused]] FEXCore::X86Tables::DecodedOp Op

// Functions in X87.cpp (no change required)
// GetX87Top
// SetX87ValidTag
// GetX87ValidTag
// GetX87Tag (will need changing once special tag handling is implemented)
// SetX87FTW
// GetX87FTW (will need changing once special tag handling is implemented)
// SetX87Top
// X87ModifySTP
// EMMS
// FFREE
// FNSTENV
// FSTCW
// LDSW
// FNSTSW
// FXCH
// FCMOV
// FST(register to register)

// State loading duplicated from X87.cpp, setting host rounding mode
// See issue
void OpDispatchBuilder::FNINITF64(OpcodeArgs) {
  // Init FCW to 0x037F
  auto NewFCW = _Constant(16, 0x037F);
  // Init host rounding mode to zero
  auto Zero = _Constant(0);
  _SetRoundingMode(Zero);
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

void OpDispatchBuilder::X87LDENVF64(OpcodeArgs) {
  const auto Size = GetSrcSize(Op);
  Ref Mem = MakeSegmentAddress(Op, Op->Src[0]);

  auto NewFCW = _LoadMem(GPRClass, 2, Mem, 2);
  // ignore the rounding precision, we're always 64-bit in F64.
  // extract rounding mode
  Ref roundingMode = _Bfe(OpSize::i32Bit, 3, 10, NewFCW);
  _SetRoundingMode(roundingMode);
  _StoreContext(2, GPRClass, NewFCW, offsetof(FEXCore::Core::CPUState, FCW));

  auto NewFSW = _LoadMem(GPRClass, Size, Mem, _Constant(Size * 1), Size, MEM_OFFSET_SXTX, 1);
  ReconstructX87StateFromFSW(NewFSW);

  {
    // FTW
    SetX87FTW(_LoadMem(GPRClass, Size, Mem, _Constant(Size * 2), Size, MEM_OFFSET_SXTX, 1));
  }
}


void OpDispatchBuilder::X87FLDCWF64(OpcodeArgs) {
  Ref NewFCW = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
  // ignore the rounding precision, we're always 64-bit in F64.
  // extract rounding mode
  Ref roundingMode = _Bfe(OpSize::i32Bit, 3, 10, NewFCW);
  _SetRoundingMode(roundingMode);
  _StoreContext(2, GPRClass, NewFCW, offsetof(FEXCore::Core::CPUState, FCW));
}

// F64 ops

void OpDispatchBuilder::FLDF64(OpcodeArgs, size_t width) {
  // Update TOP
  auto orig_top = GetX87Top();
  auto mask = _Constant(7);

  size_t read_width = (width == 80) ? 16 : width / 8;

  Ref data {};
  Ref converted {};

  if (!Op->Src[0].IsNone()) {
    // Read from memory
    data = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], read_width, Op->Flags);
    // Convert to 64bit float
    if (width == 32) {
      converted = _Float_FToF(8, 4, data);
    } else if (width == 80) {
      converted = _F80CVT(8, data);
    } else {
      converted = data;
    }
  } else {
    // Implicit arg (does this need to change with width?)
    auto offset = _Constant(Op->OP & 7);
    data = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, orig_top, offset), mask);
    data = _LoadContextIndexed(data, 8, MMBaseOffset(), 16, FPRClass);
    converted = data;
  }

  auto top = _And(OpSize::i32Bit, _Sub(OpSize::i32Bit, orig_top, _Constant(1)), mask);
  SetX87ValidTag(top, true);
  SetX87Top(top);
  // Write to ST[TOP]
  _StoreContextIndexed(converted, top, 8, MMBaseOffset(), 16, FPRClass);
}

void OpDispatchBuilder::FBLDF64(OpcodeArgs) {
  // Update TOP
  auto orig_top = GetX87Top();
  auto mask = _Constant(7);
  auto top = _And(OpSize::i32Bit, _Sub(OpSize::i32Bit, orig_top, _Constant(1)), mask);
  SetX87ValidTag(top, true);
  SetX87Top(top);

  // Read from memory
  Ref data = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], 16, Op->Flags);
  Ref converted = _F80BCDLoad(data);
  converted = _F80CVT(8, converted);
  _StoreContextIndexed(converted, top, 8, MMBaseOffset(), 16, FPRClass);
}

void OpDispatchBuilder::FBSTPF64(OpcodeArgs) {
  auto orig_top = GetX87Top();
  auto data = _LoadContextIndexed(orig_top, 8, MMBaseOffset(), 16, FPRClass);

  Ref converted = _F80CVTTo(data, 8);
  converted = _F80BCDStore(converted);

  StoreResult_WithOpSize(FPRClass, Op, Op->Dest, converted, 10, 1);

  // if we are popping then we must first mark this location as empty
  SetX87ValidTag(orig_top, false);
  auto top = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, orig_top, _Constant(1)), _Constant(7));
  SetX87Top(top);
}

void OpDispatchBuilder::FLDF64_Const(OpcodeArgs, uint64_t num) {
  // Update TOP
  auto orig_top = GetX87Top();
  auto top = _And(OpSize::i32Bit, _Sub(OpSize::i32Bit, orig_top, _Constant(1)), _Constant(7));
  SetX87ValidTag(top, true);
  SetX87Top(top);
  auto data = _VCastFromGPR(8, 8, _Constant(num));
  // Write to ST[TOP]
  _StoreContextIndexed(data, top, 8, MMBaseOffset(), 16, FPRClass);
}

void OpDispatchBuilder::FILDF64(OpcodeArgs) {
  // Update TOP
  auto orig_top = GetX87Top();
  auto top = _And(OpSize::i32Bit, _Sub(OpSize::i32Bit, orig_top, _Constant(1)), _Constant(7));
  SetX87ValidTag(top, true);
  SetX87Top(top);

  size_t read_width = GetSrcSize(Op);
  // Read from memory
  auto data = LoadSource_WithOpSize(GPRClass, Op, Op->Src[0], read_width, Op->Flags);
  if (read_width == 2) {
    data = _Sbfe(OpSize::i64Bit, read_width * 8, 0, data);
  }
  auto converted = _Float_FromGPR_S(8, read_width == 4 ? 4 : 8, data);
  // Write to ST[TOP]
  _StoreContextIndexed(converted, top, 8, MMBaseOffset(), 16, FPRClass);
}

void OpDispatchBuilder::FSTF64WithWidth(OpcodeArgs, size_t width) {
  auto orig_top = GetX87Top();
  auto data = _LoadContextIndexed(orig_top, 8, MMBaseOffset(), 16, FPRClass);
  if (width == 64) {
    // Store 64-bit float directly
    StoreResult_WithOpSize(FPRClass, Op, Op->Dest, data, 8, 1);
  } else if (width == 32) {
    // Convert to 32-bit float and store
    auto result = _Float_FToF(4, 8, data);
    StoreResult_WithOpSize(FPRClass, Op, Op->Dest, result, 4, 1);
  } else if (width == 80) {
    // Convert to 80-bit float
    auto result = _F80CVTTo(data, 8);
    StoreResult_WithOpSize(FPRClass, Op, Op->Dest, result, 10, 1);
  }

  if ((Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) != 0) {
    // if we are popping then we must first mark this location as empty
    SetX87ValidTag(orig_top, false);
    // Set the new top now
    auto top = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, orig_top, _Constant(1)), _Constant(7));
    SetX87Top(top);
  }
}

void OpDispatchBuilder::FISTF64(OpcodeArgs, bool Truncate) {
  auto Size = GetSrcSize(Op);

  auto orig_top = GetX87Top();
  Ref data = _LoadContextIndexed(orig_top, 8, MMBaseOffset(), 16, FPRClass);
  if (Truncate) {
    data = _Float_ToGPR_ZS(Size == 4 ? 4 : 8, 8, data);
  } else {
    data = _Float_ToGPR_S(Size == 4 ? 4 : 8, 8, data);
  }
  StoreResult_WithOpSize(GPRClass, Op, Op->Dest, data, Size, 1);

  if ((Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) != 0) {
    // if we are popping then we must first mark this location as empty
    SetX87ValidTag(orig_top, false);
    // Set the new top now
    auto top = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, orig_top, _Constant(1)), _Constant(7));
    SetX87Top(top);
  }
}

void OpDispatchBuilder::FADDF64(OpcodeArgs, size_t width, bool Integer, OpDispatchBuilder::OpResult ResInST0) {
  auto top = GetX87Top();
  Ref StackLocation = top;

  Ref arg {};
  Ref b {};

  auto mask = _Constant(7);

  if (!Op->Src[0].IsNone()) {
    // Memory arg
    if (Integer) {
      arg = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
      if (width == 16) {
        arg = _Sbfe(OpSize::i64Bit, 16, 0, arg);
      }
      b = _Float_FromGPR_S(8, width == 64 ? 8 : 4, arg);
    } else if (width == 32) {
      arg = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
      b = _Float_FToF(8, 4, arg);
    } else if (width == 64) {
      b = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
    }
  } else {
    // Implicit arg
    auto offset = _Constant(Op->OP & 7);
    arg = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, top, offset), mask);
    if (ResInST0 == OpResult::RES_STI) {
      StackLocation = arg;
    }
    b = _LoadContextIndexed(arg, 8, MMBaseOffset(), 16, FPRClass);
  }

  auto a = _LoadContextIndexed(top, 8, MMBaseOffset(), 16, FPRClass);
  auto result = _VFAdd(8, 8, a, b);
  if ((Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) != 0) {
    // if we are popping then we must first mark this location as empty
    SetX87ValidTag(top, false);
    // Set the new top now
    top = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, top, _Constant(1)), mask);
    SetX87Top(top);
  }

  // Write to ST[TOP]
  _StoreContextIndexed(result, StackLocation, 8, MMBaseOffset(), 16, FPRClass);
}

void OpDispatchBuilder::FMULF64(OpcodeArgs, size_t width, bool Integer, OpDispatchBuilder::OpResult ResInST0) {
  auto top = GetX87Top();
  Ref StackLocation = top;
  Ref arg {};
  Ref b {};

  auto mask = _Constant(7);

  if (!Op->Src[0].IsNone()) {
    // Memory arg
    if (Integer) {
      arg = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
      if (width == 16) {
        arg = _Sbfe(OpSize::i64Bit, 16, 0, arg);
      }
      b = _Float_FromGPR_S(8, width == 64 ? 8 : 4, arg);
    } else if (width == 32) {
      arg = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
      b = _Float_FToF(8, 4, arg);
    } else if (width == 64) {
      b = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
    }
  } else {
    // Implicit arg
    auto offset = _Constant(Op->OP & 7);
    arg = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, top, offset), mask);
    if (ResInST0 == OpResult::RES_STI) {
      StackLocation = arg;
    }

    b = _LoadContextIndexed(arg, 8, MMBaseOffset(), 16, FPRClass);
  }

  auto a = _LoadContextIndexed(top, 8, MMBaseOffset(), 16, FPRClass);

  auto result = _VFMul(8, 8, a, b);

  if ((Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) != 0) {
    // if we are popping then we must first mark this location as empty
    SetX87ValidTag(top, false);
    // Set the new top now
    top = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, top, _Constant(1)), mask);
    SetX87Top(top);
  }

  // Write to ST[TOP]
  _StoreContextIndexed(result, StackLocation, 8, MMBaseOffset(), 16, FPRClass);
}

void OpDispatchBuilder::FDIVF64(OpcodeArgs, size_t width, bool Integer, bool reverse, OpDispatchBuilder::OpResult ResInST0) {
  auto top = GetX87Top();
  Ref StackLocation = top;
  Ref arg {};
  Ref b {};

  auto mask = _Constant(7);

  if (!Op->Src[0].IsNone()) {
    // Memory arg
    if (Integer) {
      arg = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
      if (width == 16) {
        arg = _Sbfe(OpSize::i64Bit, 16, 0, arg);
      }
      b = _Float_FromGPR_S(8, width == 64 ? 8 : 4, arg);
    } else if (width == 32) {
      arg = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
      b = _Float_FToF(8, 4, arg);
    } else if (width == 64) {
      b = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
    }
  } else {
    // Implicit arg
    auto offset = _Constant(Op->OP & 7);
    arg = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, top, offset), mask);
    if (ResInST0 == OpResult::RES_STI) {
      StackLocation = arg;
    }

    b = _LoadContextIndexed(arg, 8, MMBaseOffset(), 16, FPRClass);
  }

  auto a = _LoadContextIndexed(top, 8, MMBaseOffset(), 16, FPRClass);

  Ref result {};
  if (reverse) {
    result = _VFDiv(8, 8, b, a);
  } else {
    result = _VFDiv(8, 8, a, b);
  }

  if ((Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) != 0) {
    // if we are popping then we must first mark this location as empty
    SetX87ValidTag(top, false);
    // Set the new top now
    top = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, top, _Constant(1)), mask);
    SetX87Top(top);
  }

  // Write to ST[TOP]
  _StoreContextIndexed(result, StackLocation, 8, MMBaseOffset(), 16, FPRClass);
}

void OpDispatchBuilder::FSUBF64(OpcodeArgs, size_t width, bool Integer, bool reverse, OpDispatchBuilder::OpResult ResInST0) {
  auto top = GetX87Top();
  Ref StackLocation = top;
  Ref arg {};
  Ref b {};

  auto mask = _Constant(7);

  if (!Op->Src[0].IsNone()) {
    // Memory arg
    if (Integer) {
      arg = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
      if (width == 16) {
        arg = _Sbfe(OpSize::i64Bit, 16, 0, arg);
      }
      b = _Float_FromGPR_S(8, width == 64 ? 8 : 4, arg);
    } else if (width == 32) {
      arg = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
      b = _Float_FToF(8, 4, arg);
    } else if (width == 64) {
      b = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
    }
  } else {
    // Implicit arg
    auto offset = _Constant(Op->OP & 7);
    arg = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, top, offset), mask);
    if (ResInST0 == OpResult::RES_STI) {
      StackLocation = arg;
    }

    b = _LoadContextIndexed(arg, 8, MMBaseOffset(), 16, FPRClass);
  }

  auto a = _LoadContextIndexed(top, 8, MMBaseOffset(), 16, FPRClass);

  Ref result {};
  if (reverse) {
    result = _VFSub(8, 8, b, a);
  } else {
    result = _VFSub(8, 8, a, b);
  }

  if ((Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) != 0) {
    // if we are popping then we must first mark this location as empty
    SetX87ValidTag(top, false);
    // Set the new top now

    top = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, top, _Constant(1)), mask);
    SetX87Top(top);
  }

  // Write to ST[TOP]
  _StoreContextIndexed(result, StackLocation, 8, MMBaseOffset(), 16, FPRClass);
}

void OpDispatchBuilder::FCHSF64(OpcodeArgs) {
  auto top = GetX87Top();
  auto a = _LoadContextIndexed(top, 8, MMBaseOffset(), 16, FPRClass);
  auto result = _VFNeg(8, 8, a);
  // Write to ST[TOP]
  _StoreContextIndexed(result, top, 8, MMBaseOffset(), 16, FPRClass);
}

void OpDispatchBuilder::FABSF64(OpcodeArgs) {
  auto top = GetX87Top();
  auto a = _LoadContextIndexed(top, 8, MMBaseOffset(), 16, FPRClass);
  auto result = _VFAbs(8, 8, a);
  // Write to ST[TOP]
  _StoreContextIndexed(result, top, 8, MMBaseOffset(), 16, FPRClass);
}

void OpDispatchBuilder::FTSTF64(OpcodeArgs) {
  auto top = GetX87Top();
  auto a = _LoadContextIndexed(top, 8, MMBaseOffset(), 16, FPRClass);

  auto low = _Constant(0);
  Ref data = _VCastFromGPR(8, 8, low);

  // We are going to clobber NZCV, make sure it's in a GPR first.
  GetNZCV();

  // Now we do our comparison.
  _FCmp(8, a, data);
  PossiblySetNZCVBits = ~0;
  ConvertNZCVToX87();
}

// TODO: This should obey rounding mode
void OpDispatchBuilder::FRNDINTF64(OpcodeArgs) {
  auto top = GetX87Top();
  auto a = _LoadContextIndexed(top, 8, MMBaseOffset(), 16, FPRClass);

  auto result = _Vector_FToI(8, 8, a, FEXCore::IR::Round_Host);

  // Write to ST[TOP]
  _StoreContextIndexed(result, top, 8, MMBaseOffset(), 16, FPRClass);
}

void OpDispatchBuilder::FXTRACTF64(OpcodeArgs) {
  auto orig_top = GetX87Top();
  auto top = _And(OpSize::i32Bit, _Sub(OpSize::i32Bit, orig_top, _Constant(1)), _Constant(7));
  SetX87ValidTag(top, true);
  SetX87Top(top);

  auto a = _LoadContextIndexed(orig_top, 8, MMBaseOffset(), 16, FPRClass);
  auto gpr = _VExtractToGPR(8, 8, a, 0);
  Ref exp = _And(OpSize::i64Bit, gpr, _Constant(0x7ff0000000000000LL));
  exp = _Lshr(OpSize::i64Bit, exp, _Constant(52));
  exp = _Sub(OpSize::i64Bit, exp, _Constant(1023));
  exp = _Float_FromGPR_S(8, 8, exp);
  Ref sig = _And(OpSize::i64Bit, gpr, _Constant(0x800fffffffffffffLL));
  sig = _Or(OpSize::i64Bit, sig, _Constant(0x3ff0000000000000LL));
  sig = _VCastFromGPR(8, 8, sig);
  // Write to ST[TOP]
  _StoreContextIndexed(exp, orig_top, 8, MMBaseOffset(), 16, FPRClass);
  _StoreContextIndexed(sig, top, 8, MMBaseOffset(), 16, FPRClass);
}


void OpDispatchBuilder::FCOMIF64(OpcodeArgs, size_t width, bool Integer, OpDispatchBuilder::FCOMIFlags whichflags, bool poptwice) {
  auto top = GetX87Top();
  auto mask = _Constant(7);

  Ref arg {};
  Ref b {};

  if (!Op->Src[0].IsNone()) {
    // Memory arg
    if (Integer) {
      arg = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
      if (width == 16) {
        arg = _Sbfe(OpSize::i64Bit, 16, 0, arg);
      }
      b = _Float_FromGPR_S(8, width == 64 ? 8 : 4, arg);
    } else if (width == 32) {
      arg = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
      b = _Float_FToF(8, 4, arg);
    } else if (width == 64) {
      b = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
    }
  } else {
    // Implicit arg
    auto offset = _Constant(Op->OP & 7);
    arg = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, top, offset), mask);
    b = _LoadContextIndexed(arg, 8, MMBaseOffset(), 16, FPRClass);
  }

  auto a = _LoadContextIndexed(top, 8, MMBaseOffset(), 16, FPRClass);

  if (whichflags == FCOMIFlags::FLAGS_X87) {
    // We are going to clobber NZCV, make sure it's in a GPR first.
    GetNZCV();

    _FCmp(8, a, b);
    PossiblySetNZCVBits = ~0;
    ConvertNZCVToX87();
  } else {
    Comiss(8, a, b, true /* InvalidateAF */);
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

void OpDispatchBuilder::FSQRTF64(OpcodeArgs) {
  auto top = GetX87Top();
  auto a = _LoadContextIndexed(top, 8, MMBaseOffset(), 16, FPRClass);

  auto result = _VFSqrt(8, 8, a);

  // Write to ST[TOP]
  _StoreContextIndexed(result, top, 8, MMBaseOffset(), 16, FPRClass);
}


void OpDispatchBuilder::X87UnaryOpF64(OpcodeArgs, FEXCore::IR::IROps IROp) {
  auto top = GetX87Top();
  auto a = _LoadContextIndexed(top, 8, MMBaseOffset(), 16, FPRClass);

  DeriveOp(result, IROp, _F64SIN(a));

  if (IROp == IR::OP_F64SIN || IROp == IR::OP_F64COS) {
    // TODO: ACCURACY: should check source is in range –2^63 to +2^63
    SetRFLAG<FEXCore::X86State::X87FLAG_C2_LOC>(_Constant(0));
  }

  // Write to ST[TOP]
  _StoreContextIndexed(result, top, 8, MMBaseOffset(), 16, FPRClass);
}

void OpDispatchBuilder::X87BinaryOpF64(OpcodeArgs, FEXCore::IR::IROps IROp) {
  auto top = GetX87Top();

  auto mask = _Constant(7);
  Ref st1 = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, top, _Constant(1)), mask);

  auto a = _LoadContextIndexed(top, 8, MMBaseOffset(), 16, FPRClass);
  st1 = _LoadContextIndexed(st1, 8, MMBaseOffset(), 16, FPRClass);

  DeriveOp(result, IROp, _F64ATAN(a, st1));

  if (IROp == IR::OP_F64FPREM || IROp == IR::OP_F64FPREM1) {
    // TODO: Set C0 to Q2, C3 to Q1, C1 to Q0
    SetRFLAG<FEXCore::X86State::X87FLAG_C2_LOC>(_Constant(0));
  }

  // Write to ST[TOP]
  _StoreContextIndexed(result, top, 8, MMBaseOffset(), 16, FPRClass);
}

void OpDispatchBuilder::X87SinCosF64(OpcodeArgs) {
  auto orig_top = GetX87Top();
  auto top = _And(OpSize::i32Bit, _Sub(OpSize::i32Bit, orig_top, _Constant(1)), _Constant(7));
  SetX87ValidTag(top, true);
  SetX87Top(top);

  auto a = _LoadContextIndexed(orig_top, 8, MMBaseOffset(), 16, FPRClass);

  auto sin = _F64SIN(a);
  auto cos = _F64COS(a);

  // TODO: ACCURACY: should check source is in range –2^63 to +2^63
  SetRFLAG<FEXCore::X86State::X87FLAG_C2_LOC>(_Constant(0));

  // Write to ST[TOP]
  _StoreContextIndexed(sin, orig_top, 8, MMBaseOffset(), 16, FPRClass);
  _StoreContextIndexed(cos, top, 8, MMBaseOffset(), 16, FPRClass);
}

void OpDispatchBuilder::X87FYL2XF64(OpcodeArgs) {
  bool Plus1 = Op->OP == 0x01F9; // FYL2XP

  auto orig_top = GetX87Top();
  // if we are popping then we must first mark this location as empty
  SetX87ValidTag(orig_top, false);
  auto top = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, orig_top, _Constant(1)), _Constant(7));
  SetX87Top(top);

  Ref st0 = _LoadContextIndexed(orig_top, 8, MMBaseOffset(), 16, FPRClass);
  Ref st1 = _LoadContextIndexed(top, 8, MMBaseOffset(), 16, FPRClass);

  if (Plus1) {
    auto one = _VCastFromGPR(8, 8, _Constant(0x3FF0000000000000));
    st0 = _VFAdd(8, 8, st0, one);
  }

  auto result = _F64FYL2X(st0, st1);

  // Write to ST[TOP]
  _StoreContextIndexed(result, top, 8, MMBaseOffset(), 16, FPRClass);
}

void OpDispatchBuilder::X87TANF64(OpcodeArgs) {
  auto orig_top = GetX87Top();
  auto top = _And(OpSize::i32Bit, _Sub(OpSize::i32Bit, orig_top, _Constant(1)), _Constant(7));
  SetX87ValidTag(top, true);
  SetX87Top(top);

  auto a = _LoadContextIndexed(orig_top, 8, MMBaseOffset(), 16, FPRClass);

  auto result = _F64TAN(a);

  auto one = _VCastFromGPR(8, 8, _Constant(0x3FF0000000000000));

  // TODO: ACCURACY: should check source is in range –2^63 to +2^63
  SetRFLAG<FEXCore::X86State::X87FLAG_C2_LOC>(_Constant(0));

  // Write to ST[TOP]
  _StoreContextIndexed(result, orig_top, 8, MMBaseOffset(), 16, FPRClass);
  _StoreContextIndexed(one, top, 8, MMBaseOffset(), 16, FPRClass);
}

void OpDispatchBuilder::X87ATANF64(OpcodeArgs) {
  auto orig_top = GetX87Top();
  // if we are popping then we must first mark this location as empty
  SetX87ValidTag(orig_top, false);
  auto top = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, orig_top, _Constant(1)), _Constant(7));
  SetX87Top(top);

  auto a = _LoadContextIndexed(orig_top, 8, MMBaseOffset(), 16, FPRClass);
  Ref st1 = _LoadContextIndexed(top, 8, MMBaseOffset(), 16, FPRClass);

  auto result = _F64ATAN(st1, a);

  // Write to ST[TOP]
  _StoreContextIndexed(result, top, 8, MMBaseOffset(), 16, FPRClass);
}

// This function converts to F80 on save for compatibility

void OpDispatchBuilder::X87FNSAVEF64(OpcodeArgs) {
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
    Ref data = _LoadContextIndexed(Top, 8, MMBaseOffset(), 16, FPRClass);
    data = _F80CVTTo(data, 8);
    _StoreMem(FPRClass, 16, data, Mem, _Constant((Size * 7) + (i * 10)), 1, MEM_OFFSET_SXTX, 1);
    Top = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, Top, OneConst), SevenConst);
  }

  // The final st(7) needs a bit of special handling here
  Ref data = _LoadContextIndexed(Top, 8, MMBaseOffset(), 16, FPRClass);
  data = _F80CVTTo(data, 8);
  // ST7 broken in to two parts
  // Lower 64bits [63:0]
  // upper 16 bits [79:64]
  _StoreMem(FPRClass, 8, data, Mem, _Constant((Size * 7) + (7 * 10)), 1, MEM_OFFSET_SXTX, 1);
  auto topBytes = _VDupElement(16, 2, data, 4);
  _StoreMem(FPRClass, 2, topBytes, Mem, _Constant((Size * 7) + (7 * 10) + 8), 1, MEM_OFFSET_SXTX, 1);

  // reset to default
  FNINIT(Op);
}

// This function converts from F80 on load for compatibility

void OpDispatchBuilder::X87FRSTORF64(OpcodeArgs) {
  const auto Size = GetSrcSize(Op);
  Ref Mem = MakeSegmentAddress(Op, Op->Src[0]);

  auto NewFCW = _LoadMem(GPRClass, 2, Mem, 2);
  // ignore the rounding precision, we're always 64-bit in F64.
  // extract rounding mode
  Ref roundingMode = NewFCW;
  auto roundShift = _Constant(10);
  auto roundMask = _Constant(3);
  roundingMode = _Lshr(OpSize::i32Bit, roundingMode, roundShift);
  roundingMode = _And(OpSize::i32Bit, roundingMode, roundMask);
  _SetRoundingMode(roundingMode);
  _StoreContext(2, GPRClass, NewFCW, offsetof(FEXCore::Core::CPUState, FCW));
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
    Ref Reg = _LoadMem(FPRClass, 16, Mem, _Constant((Size * 7) + (i * 10)), 1, MEM_OFFSET_SXTX, 1);
    // Mask off the top bits
    Reg = _VAnd(16, 16, Reg, Mask);
    // Convert to double precision
    Reg = _F80CVT(8, Reg);
    _StoreContextIndexed(Reg, Top, 8, MMBaseOffset(), 16, FPRClass);

    Top = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, Top, OneConst), SevenConst);
  }

  // The final st(7) needs a bit of special handling here
  // ST7 broken in to two parts
  // Lower 64bits [63:0]
  // upper 16 bits [79:64]

  Ref Reg = _LoadMem(FPRClass, 8, Mem, _Constant((Size * 7) + (7 * 10)), 1, MEM_OFFSET_SXTX, 1);
  Ref RegHigh = _LoadMem(FPRClass, 2, Mem, _Constant((Size * 7) + (7 * 10) + 8), 1, MEM_OFFSET_SXTX, 1);
  Reg = _VInsElement(16, 2, 4, 0, Reg, RegHigh);
  Reg = _F80CVT(8, Reg); // Convert to double precision
  _StoreContextIndexed(Reg, Top, 8, MMBaseOffset(), 16, FPRClass);
}


// FXAM needs change
void OpDispatchBuilder::X87FXAMF64(OpcodeArgs) {
  auto top = GetX87Top();
  auto a = _LoadContextIndexed(top, 8, MMBaseOffset(), 16, FPRClass);
  Ref Result = _VExtractToGPR(8, 8, a, 0);

  // Extract the sign bit
  Result = _Bfe(OpSize::i64Bit, 1, 63, Result);
  SetRFLAG<FEXCore::X86State::X87FLAG_C1_LOC>(Result);

  // Claim this is a normal number
  // We don't support anything else
  auto TopValid = GetX87ValidTag(top);
  auto ZeroConst = _Constant(0);
  auto OneConst = _Constant(1);

  // In the case of top being invalid then C3:C2:C0 is 0b101
  auto C3 = _Select(FEXCore::IR::COND_EQ, TopValid, OneConst, ZeroConst, OneConst);

  auto C2 = TopValid;
  auto C0 = C3; // Mirror C3 until something other than zero is supported
  SetRFLAG<FEXCore::X86State::X87FLAG_C0_LOC>(C0);
  SetRFLAG<FEXCore::X86State::X87FLAG_C2_LOC>(C2);
  SetRFLAG<FEXCore::X86State::X87FLAG_C3_LOC>(C3);
}


} // namespace FEXCore::IR
