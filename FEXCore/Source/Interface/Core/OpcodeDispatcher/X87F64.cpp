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

#include <stddef.h>
#include <stdint.h>

namespace FEXCore::IR {
class OrderedNode;

#define OpcodeArgs [[maybe_unused]] FEXCore::X86Tables::DecodedOp Op

void OpDispatchBuilder::X87LDENVF64(OpcodeArgs) {
  _StackForceSlow();

  const auto Size = OpSizeFromSrc(Op);
  Ref Mem = MakeSegmentAddress(Op, Op->Src[0]);

  auto NewFCW = _LoadMem(GPRClass, OpSize::i16Bit, Mem, OpSize::i16Bit);
  // ignore the rounding precision, we're always 64-bit in F64.
  // extract rounding mode
  Ref roundingMode = _Bfe(OpSize::i32Bit, 3, 10, NewFCW);
  _SetRoundingMode(roundingMode, false, roundingMode);
  _StoreContext(OpSize::i16Bit, GPRClass, NewFCW, offsetof(FEXCore::Core::CPUState, FCW));

  auto NewFSW = _LoadMem(GPRClass, Size, Mem, _Constant(Size * 1), Size, MEM_OFFSET_SXTX, 1);
  ReconstructX87StateFromFSW_Helper(NewFSW);

  {
    // FTW
    SetX87FTW(_LoadMem(GPRClass, Size, Mem, _Constant(Size * 2), Size, MEM_OFFSET_SXTX, 1));
  }
}

void OpDispatchBuilder::X87FLDCWF64(OpcodeArgs) {
  _StackForceSlow();

  Ref NewFCW = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
  // ignore the rounding precision, we're always 64-bit in F64.
  // extract rounding mode
  Ref roundingMode = _Bfe(OpSize::i32Bit, 3, 10, NewFCW);
  _SetRoundingMode(roundingMode, false, roundingMode);
  _StoreContext(OpSize::i16Bit, GPRClass, NewFCW, offsetof(FEXCore::Core::CPUState, FCW));
}

// F64 ops
// Float load op with memory operand
void OpDispatchBuilder::FLDF64(OpcodeArgs, size_t Width) {
  const auto ReadWidth = (Width == 80) ? OpSize::i128Bit : IR::SizeToOpSize(Width / 8);
  Ref Data = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], ReadWidth, Op->Flags);
  // Convert to 64bit float
  Ref ConvertedData = Data;
  if (Width == 32) {
    ConvertedData = _Float_FToF(OpSize::i64Bit, OpSize::i32Bit, Data);
  } else if (Width == 80) {
    ConvertedData = _F80CVT(OpSize::i64Bit, Data);
  }
  _PushStack(ConvertedData, Data, ReadWidth, true);
}

void OpDispatchBuilder::FBLDF64(OpcodeArgs) {
  // Read from memory
  Ref Data = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], OpSize::i128Bit, Op->Flags);
  Ref ConvertedData = _F80BCDLoad(Data);
  ConvertedData = _F80CVT(OpSize::i64Bit, ConvertedData);
  _PushStack(ConvertedData, Data, OpSize::i64Bit, true);
}

void OpDispatchBuilder::FBSTPF64(OpcodeArgs) {
  Ref converted = _F80CVTTo(_ReadStackValue(0), OpSize::i64Bit);
  converted = _F80BCDStore(converted);
  StoreResult_WithOpSize(FPRClass, Op, Op->Dest, converted, OpSize::f80Bit, OpSize::i8Bit);
  _PopStackDestroy();
}

void OpDispatchBuilder::FLDF64_Const(OpcodeArgs, uint64_t Num) {
  auto Data = _VCastFromGPR(OpSize::i64Bit, OpSize::i64Bit, _Constant(Num));
  _PushStack(Data, Data, OpSize::i64Bit, true);
}

void OpDispatchBuilder::FILDF64(OpcodeArgs) {
  const auto ReadWidth = OpSizeFromSrc(Op);

  // Read from memory
  Ref Data = LoadSource_WithOpSize(GPRClass, Op, Op->Src[0], ReadWidth, Op->Flags);
  if (ReadWidth == OpSize::i16Bit) {
    Data = _Sbfe(OpSize::i64Bit, ReadWidth * 8, 0, Data);
  }
  auto ConvertedData = _Float_FromGPR_S(OpSize::i64Bit, ReadWidth == OpSize::i32Bit ? OpSize::i32Bit : OpSize::i64Bit, Data);
  _PushStack(ConvertedData, Data, ReadWidth, false);
}

void OpDispatchBuilder::FSTF64(OpcodeArgs, size_t Width) {
  Ref Mem = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.LoadData = false});
  _StoreStackMemory(Mem, OpSize::i64Bit, true, Width / 8);

  if (Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) {
    _PopStackDestroy();
  }
}

void OpDispatchBuilder::FISTF64(OpcodeArgs, bool Truncate) {
  const auto Size = OpSizeFromSrc(Op);

  Ref data = _ReadStackValue(0);
  if (Truncate) {
    data = _Float_ToGPR_ZS(Size == 4 ? OpSize::i32Bit : OpSize::i64Bit, OpSize::i64Bit, data);
  } else {
    data = _Float_ToGPR_S(Size == 4 ? OpSize::i32Bit : OpSize::i64Bit, OpSize::i64Bit, data);
  }
  StoreResult_WithOpSize(GPRClass, Op, Op->Dest, data, Size, OpSize::i8Bit);

  if ((Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) != 0) {
    _PopStackDestroy();
  }
}

void OpDispatchBuilder::FADDF64(OpcodeArgs, size_t Width, bool Integer, OpDispatchBuilder::OpResult ResInST0) {
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

  // We have one memory argument
  Ref arg {};

  if (Integer) {
    arg = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
    if (Width == 16) {
      arg = _Sbfe(OpSize::i64Bit, 16, 0, arg);
    }
    arg = _Float_FromGPR_S(OpSize::i64Bit, Width == 64 ? OpSize::i64Bit : OpSize::i32Bit, arg);
  } else if (Width == 32) {
    arg = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
    arg = _Float_FToF(OpSize::i64Bit, OpSize::i32Bit, arg);
  } else if (Width == 64) {
    arg = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  }

  // top of stack is at offset zero
  _F80AddValue(0, arg);
}

// FIXME: following is very similar to FADDF64
void OpDispatchBuilder::FMULF64(OpcodeArgs, size_t Width, bool Integer, OpDispatchBuilder::OpResult ResInST0) {
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

  // We have one memory argument
  Ref arg {};

  if (Integer) {
    arg = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
    if (Width == 16) {
      arg = _Sbfe(OpSize::i64Bit, 16, 0, arg);
    }
    arg = _Float_FromGPR_S(8, Width == 64 ? OpSize::i64Bit : OpSize::i32Bit, arg);
  } else if (Width == 32) {
    arg = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
    arg = _Float_FToF(OpSize::i64Bit, OpSize::i32Bit, arg);
  } else if (Width == 64) {
    arg = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  }

  // top of stack is at offset zero
  _F80MulValue(0, arg);

  if (Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) {
    _PopStackDestroy();
  }
}

void OpDispatchBuilder::FDIVF64(OpcodeArgs, size_t Width, bool Integer, bool Reverse, OpDispatchBuilder::OpResult ResInST0) {
  if (Op->Src[0].IsNone()) {
    const auto offset = Op->OP & 7;
    const auto st0 = 0;

    if (Reverse) {
      if (ResInST0 == OpResult::RES_STI) {
        _F80DivStack(offset, st0, offset);
      } else {
        _F80DivStack(st0, offset, st0);
      }
    } else {
      if (ResInST0 == OpResult::RES_STI) {
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
  Ref Arg {};

  if (Width == 16 || Width == 32 || Width == 64) {
    if (Integer) {
      Arg = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
      if (Width == 16) {
        Arg = _Sbfe(OpSize::i64Bit, 16, 0, Arg);
      }
      Arg = _Float_FromGPR_S(8, Width == 64 ? OpSize::i64Bit : OpSize::i32Bit, Arg);
    } else if (Width == 32) {
      Arg = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
      Arg = _Float_FToF(OpSize::i64Bit, OpSize::i32Bit, Arg);
    } else if (Width == 64) {
      Arg = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
    }
  }

  // top of stack is at offset zero
  if (Reverse) {
    _F80DivRValue(Arg, 0);
  } else {
    _F80DivValue(0, Arg);
  }

  if (Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) {
    _PopStackDestroy();
  }
}

void OpDispatchBuilder::FSUBF64(OpcodeArgs, size_t Width, bool Integer, bool Reverse, OpDispatchBuilder::OpResult ResInST0) {
  if (Op->Src[0].IsNone()) {
    const auto Offset = Op->OP & 7;
    const auto St0 = 0;

    if (Reverse) {
      if (ResInST0 == OpResult::RES_STI) {
        _F80SubStack(Offset, St0, Offset);
      } else {
        _F80SubStack(St0, Offset, St0);
      }
    } else {
      if (ResInST0 == OpResult::RES_STI) {
        _F80SubStack(Offset, Offset, St0);
      } else {
        _F80SubStack(St0, St0, Offset);
      }
    }

    if (Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) {
      _PopStackDestroy();
    }
    return;
  }

  // We have one memory argument
  Ref arg {};

  if (Width == 16 || Width == 32 || Width == 64) {
    if (Integer) {
      arg = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
      if (Width == 16) {
        arg = _Sbfe(OpSize::i64Bit, 16, 0, arg);
      }
      arg = _Float_FromGPR_S(8, Width == 64 ? OpSize::i64Bit : OpSize::i32Bit, arg);
    } else if (Width == 32) {
      arg = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
      arg = _Float_FToF(OpSize::i64Bit, OpSize::i32Bit, arg);
    } else if (Width == 64) {
      arg = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
    }
  }

  // top of stack is at offset zero
  if (Reverse) {
    _F80SubRValue(arg, 0);
  } else {
    _F80SubValue(0, arg);
  }

  if (Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) {
    _PopStackDestroy();
  }
}

void OpDispatchBuilder::FTSTF64(OpcodeArgs) {
  // We are going to clobber NZCV, make sure it's in a GPR first.
  GetNZCV();

  // Now we do our comparison.
  _F80StackTest(0);
  PossiblySetNZCVBits = ~0;
  ConvertNZCVToX87();
}

void OpDispatchBuilder::FCOMIF64(OpcodeArgs, size_t Width, bool Integer, OpDispatchBuilder::FCOMIFlags WhichFlags, bool PopTwice) {
  Ref arg {};
  Ref b {};

  if (Op->Src[0].IsNone()) {
    // Implicit arg
    uint8_t offset = Op->OP & 7;
    b = _ReadStackValue(offset);
  } else {
    // Memory arg
    if (Width == 16 || Width == 32 || Width == 64) {
      if (Integer) {
        arg = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
        if (Width == 16) {
          arg = _Sbfe(OpSize::i64Bit, 16, 0, arg);
        }
        b = _Float_FromGPR_S(8, Width == 64 ? OpSize::i64Bit : OpSize::i32Bit, arg);
      } else if (Width == 32) {
        arg = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
        b = _Float_FToF(OpSize::i64Bit, OpSize::i32Bit, arg);
      } else if (Width == 64) {
        b = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
      }
    }
  }

  if (WhichFlags == FCOMIFlags::FLAGS_X87) {
    // We are going to clobber NZCV, make sure it's in a GPR first.
    GetNZCV();

    _F80CmpValue(b);
    PossiblySetNZCVBits = ~0;
    ConvertNZCVToX87();
  } else {
    HandleNZCVWrite();
    _F80CmpValue(b);
    ComissFlags(true /* InvalidateAF */);
  }

  if (PopTwice) {
    _PopStackDestroy();
    _PopStackDestroy();
  } else if ((Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) != 0) {
    _PopStackDestroy();
  }
}

void OpDispatchBuilder::X87FXTRACTF64(OpcodeArgs) {
  // Split node into SIG and EXP while handling the special zero case.
  // i.e. if val == 0.0, then sig = 0.0, exp = -inf
  // if val == -0.0, then sig = -0.0, exp = -inf
  // otherwise we just extract the 64-bit sig and exp as normal.
  Ref Node = _ReadStackValue(0);

  Ref Gpr = _VExtractToGPR(OpSize::i64Bit, OpSize::i64Bit, Node, 0);

  // zero case
  Ref ExpZV = _VCastFromGPR(OpSize::i64Bit, OpSize::i64Bit, _Constant(0xfff0'0000'0000'0000UL));
  Ref SigZV = Node;

  // non zero case
  Ref ExpNZ = _Bfe(OpSize::i64Bit, 11, 52, Gpr);
  ExpNZ = _Sub(OpSize::i64Bit, ExpNZ, _Constant(1023));
  Ref ExpNZV = _Float_FromGPR_S(OpSize::i64Bit, OpSize::i64Bit, ExpNZ);

  Ref SigNZ = _And(OpSize::i64Bit, Gpr, _Constant(0x800f'ffff'ffff'ffffLL));
  SigNZ = _Or(OpSize::i64Bit, SigNZ, _Constant(0x3ff0'0000'0000'0000LL));
  Ref SigNZV = _VCastFromGPR(OpSize::i64Bit, OpSize::i64Bit, SigNZ);

  // Comparison and select to push onto stack
  SaveNZCV();
  _TestNZ(OpSize::i64Bit, Gpr, _Constant(0x7fff'ffff'ffff'ffffUL));

  Ref Sig = _NZCVSelectV(OpSize::i64Bit, {COND_EQ}, SigZV, SigNZV);
  Ref Exp = _NZCVSelectV(OpSize::i64Bit, {COND_EQ}, ExpZV, ExpNZV);

  _PopStackDestroy();
  _PushStack(Exp, Exp, 64, true);
  _PushStack(Sig, Sig, 64, true);
}
} // namespace FEXCore::IR
