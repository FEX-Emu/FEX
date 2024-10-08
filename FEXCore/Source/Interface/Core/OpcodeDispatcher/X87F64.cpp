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

void OpDispatchBuilder::FNINITF64(OpcodeArgs) {
  // Init host rounding mode to zero
  auto Zero = _Constant(0);
  _SetRoundingMode(Zero, false, Zero);

  // Call generic version
  FNINIT(Op);
}

void OpDispatchBuilder::X87LDENVF64(OpcodeArgs) {
  _StackForceSlow();

  const auto Size = GetSrcSize(Op);
  Ref Mem = MakeSegmentAddress(Op, Op->Src[0]);

  auto NewFCW = _LoadMem(GPRClass, 2, Mem, 2);
  // ignore the rounding precision, we're always 64-bit in F64.
  // extract rounding mode
  Ref roundingMode = _Bfe(OpSize::i32Bit, 3, 10, NewFCW);
  _SetRoundingMode(roundingMode, false, roundingMode);
  _StoreContext(2, GPRClass, NewFCW, offsetof(FEXCore::Core::CPUState, FCW));

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
  _StoreContext(2, GPRClass, NewFCW, offsetof(FEXCore::Core::CPUState, FCW));
}

// F64 ops
// Float load op with memory operand
void OpDispatchBuilder::FLDF64(OpcodeArgs, size_t Width) {
  size_t ReadWidth = (Width == 80) ? 16 : Width / 8;
  Ref Data = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], ReadWidth, Op->Flags);
  // Convert to 64bit float
  Ref ConvertedData = Data;
  if (Width == 32) {
    ConvertedData = _Float_FToF(8, 4, Data);
  } else if (Width == 80) {
    ConvertedData = _F80CVT(8, Data);
  }
  _PushStack(ConvertedData, Data, ReadWidth, true);
}

void OpDispatchBuilder::FBLDF64(OpcodeArgs) {
  // Read from memory
  Ref Data = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], 16, Op->Flags);
  Ref ConvertedData = _F80BCDLoad(Data);
  ConvertedData = _F80CVT(8, ConvertedData);
  _PushStack(ConvertedData, Data, 8, true);
}

void OpDispatchBuilder::FBSTPF64(OpcodeArgs) {
  Ref converted = _F80CVTTo(_ReadStackValue(0), 8);
  converted = _F80BCDStore(converted);
  StoreResult_WithOpSize(FPRClass, Op, Op->Dest, converted, 10, 1);
  _PopStackDestroy();
}

void OpDispatchBuilder::FLDF64_Const(OpcodeArgs, uint64_t Num) {
  auto Data = _VCastFromGPR(8, 8, _Constant(Num));
  _PushStack(Data, Data, 8, true);
}

void OpDispatchBuilder::FILDF64(OpcodeArgs) {
  size_t ReadWidth = GetSrcSize(Op);

  // Read from memory
  Ref Data = LoadSource_WithOpSize(GPRClass, Op, Op->Src[0], ReadWidth, Op->Flags);
  if (ReadWidth == 2) {
    Data = _Sbfe(OpSize::i64Bit, ReadWidth * 8, 0, Data);
  }
  auto ConvertedData = _Float_FromGPR_S(8, ReadWidth == 4 ? 4 : 8, Data);
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
  auto Size = GetSrcSize(Op);

  Ref data = _ReadStackValue(0);
  if (Truncate) {
    data = _Float_ToGPR_ZS(Size == 4 ? 4 : 8, 8, data);
  } else {
    data = _Float_ToGPR_S(Size == 4 ? 4 : 8, 8, data);
  }
  StoreResult_WithOpSize(GPRClass, Op, Op->Dest, data, Size, 1);

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
    arg = _Float_FromGPR_S(8, Width == 64 ? 8 : 4, arg);
  } else if (Width == 32) {
    arg = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
    arg = _Float_FToF(8, 4, arg);
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
    arg = _Float_FromGPR_S(8, Width == 64 ? 8 : 4, arg);
  } else if (Width == 32) {
    arg = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
    arg = _Float_FToF(8, 4, arg);
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
      Arg = _Float_FromGPR_S(8, Width == 64 ? 8 : 4, Arg);
    } else if (Width == 32) {
      Arg = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
      Arg = _Float_FToF(8, 4, Arg);
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
      arg = _Float_FromGPR_S(8, Width == 64 ? 8 : 4, arg);
    } else if (Width == 32) {
      arg = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
      arg = _Float_FToF(8, 4, arg);
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
        b = _Float_FromGPR_S(8, Width == 64 ? 8 : 4, arg);
      } else if (Width == 32) {
        arg = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
        b = _Float_FToF(8, 4, arg);
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

// This function converts to F80 on save for compatibility
void OpDispatchBuilder::X87FNSAVEF64(OpcodeArgs) {
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

  { _StoreMem(GPRClass, Size, ReconstructFSW_Helper(), Mem, _Constant(Size * 1), Size, MEM_OFFSET_SXTX, 1); }

  auto ZeroConst = _Constant(0);

  {
    // FTW
    _StoreMem(GPRClass, Size, GetX87FTW_Helper(), Mem, _Constant(Size * 2), Size, MEM_OFFSET_SXTX, 1);
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
  for (int i = 0; i < 8; ++i) {
    Ref data = _LoadContextIndexed(Top, 8, MMBaseOffset(), 16, FPRClass);
    data = _F80CVTTo(data, 8);
    _StoreMem(FPRClass, 16, data, Mem, _Constant((Size * 7) + (i * 10)), 1, MEM_OFFSET_SXTX, 1);
    Top = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, Top, OneConst), SevenConst);
  }

  // reset to default
  FNINITF64(Op);
}

// This function converts from F80 on load for compatibility

void OpDispatchBuilder::X87FRSTORF64(OpcodeArgs) {
  _StackForceSlow();
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
  _SetRoundingMode(roundingMode, false, roundingMode);
  _StoreContext(2, GPRClass, NewFCW, offsetof(FEXCore::Core::CPUState, FCW));
  _StoreContext(2, GPRClass, NewFCW, offsetof(FEXCore::Core::CPUState, FCW));

  auto NewFSW = _LoadMem(GPRClass, Size, Mem, _Constant(Size * 1), Size, MEM_OFFSET_SXTX, 1);
  Ref Top = ReconstructX87StateFromFSW_Helper(NewFSW);

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

  for (int i = 0; i < 8; ++i) {
    Ref Reg = _LoadMem(FPRClass, 16, Mem, _Constant((Size * 7) + (i * 10)), 1, MEM_OFFSET_SXTX, 1);
    // Mask off the top bits
    Reg = _VAnd(16, 16, Reg, Mask);
    // Convert to double precision
    Reg = _F80CVT(8, Reg);
    _StoreContextIndexed(Reg, Top, 8, MMBaseOffset(), 16, FPRClass);

    Top = _And(OpSize::i32Bit, _Add(OpSize::i32Bit, Top, OneConst), SevenConst);
  }
}

} // namespace FEXCore::IR
