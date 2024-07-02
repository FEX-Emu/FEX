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
// FCHS

// State loading duplicated from X87.cpp, setting host rounding mode
// See issue
void OpDispatchBuilder::FNINITF64(OpcodeArgs) {
  // FIXME: almost a duplicate of x87 version.
  CurrentHeader->HasX87 = true;
  // Init FCW to 0x037F
  auto NewFCW = _Constant(16, 0x037F);
  // Init host rounding mode to zero
  auto Zero = _Constant(0);
  _SetRoundingMode(Zero);
  _StoreContext(2, GPRClass, NewFCW, offsetof(FEXCore::Core::CPUState, FCW));

  // Init FSW to 0
  SetX87Top(Zero);
  // Tags all get marked as invalid
  _StoreContext(1, GPRClass, Zero, offsetof(FEXCore::Core::CPUState, AbridgedFTW));

  _InitStack();

  SetRFLAG<FEXCore::X86State::X87FLAG_C0_LOC>(Zero);
  SetRFLAG<FEXCore::X86State::X87FLAG_C1_LOC>(Zero);
  SetRFLAG<FEXCore::X86State::X87FLAG_C2_LOC>(Zero);
  SetRFLAG<FEXCore::X86State::X87FLAG_C3_LOC>(Zero);
}

void OpDispatchBuilder::X87LDENVF64(OpcodeArgs) {
  CurrentHeader->HasX87 = true;
  _StackForceSlow();

  const auto Size = GetSrcSize(Op);
  Ref Mem = MakeSegmentAddress(Op, Op->Src[0]);

  auto NewFCW = _LoadMem(GPRClass, 2, Mem, 2);
  // ignore the rounding precision, we're always 64-bit in F64.
  // extract rounding mode
  Ref roundingMode = _Bfe(OpSize::i32Bit, 3, 10, NewFCW);
  _SetRoundingMode(roundingMode);
  _StoreContext(2, GPRClass, NewFCW, offsetof(FEXCore::Core::CPUState, FCW));

  auto NewFSW = _LoadMem(GPRClass, Size, Mem, _Constant(Size * 1), Size, MEM_OFFSET_SXTX, 1);
  ReconstructX87StateFromFSW_Helper(NewFSW);

  {
    // FTW
    SetX87FTW(_LoadMem(GPRClass, Size, Mem, _Constant(Size * 2), Size, MEM_OFFSET_SXTX, 1));
  }
}


void OpDispatchBuilder::X87FLDCWF64(OpcodeArgs) {
  CurrentHeader->HasX87 = true;
  _StackForceSlow();

  Ref NewFCW = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
  // ignore the rounding precision, we're always 64-bit in F64.
  // extract rounding mode
  Ref roundingMode = _Bfe(OpSize::i32Bit, 3, 10, NewFCW);
  _SetRoundingMode(roundingMode);
  _StoreContext(2, GPRClass, NewFCW, offsetof(FEXCore::Core::CPUState, FCW));
}

// F64 ops
template<size_t width>
void OpDispatchBuilder::FLDF64(OpcodeArgs) {
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
    if constexpr (width == 32) {
      data = _Float_FToF(8, 4, data);
    } else if constexpr (width == 80) {
      data = _F80CVT(8, data);
    }
  }
  _PushStack(data, OpSize::i64Bit, true, read_width);
}

template void OpDispatchBuilder::FLDF64<32>(OpcodeArgs);
template void OpDispatchBuilder::FLDF64<64>(OpcodeArgs);
template void OpDispatchBuilder::FLDF64<80>(OpcodeArgs);

void OpDispatchBuilder::FBLDF64(OpcodeArgs) {
  CurrentHeader->HasX87 = true;
  // Read from memory
  Ref data = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], 16, Op->Flags);
  Ref converted = _F80BCDLoad(data);
  converted = _F80CVT(8, converted);
  _PushStack(converted, i64Bit, true, 8);
}

void OpDispatchBuilder::FBSTPF64(OpcodeArgs) {
  CurrentHeader->HasX87 = true;
  Ref converted = _F80CVTTo(_ReadStackValue(0), 8);
  converted = _F80BCDStore(converted);
  StoreResult_WithOpSize(FPRClass, Op, Op->Dest, converted, 10, 1);
  _PopStackDestroy();
}

template<uint64_t num>
void OpDispatchBuilder::FLDF64_Const(OpcodeArgs) {
  CurrentHeader->HasX87 = true;

  auto data = _VCastFromGPR(8, 8, _Constant(num));
  _PushStack(data, OpSize::i64Bit, true, 8);
}

template void OpDispatchBuilder::FLDF64_Const<0x3FF0000000000000>(OpcodeArgs); // 1.0
template void OpDispatchBuilder::FLDF64_Const<0x400A934F0979A372>(OpcodeArgs); // log2l(10)
template void OpDispatchBuilder::FLDF64_Const<0x3FF71547652B82FE>(OpcodeArgs); // log2l(e)
template void OpDispatchBuilder::FLDF64_Const<0x400921FB54442D18>(OpcodeArgs); // pi
template void OpDispatchBuilder::FLDF64_Const<0x3FD34413509F79FF>(OpcodeArgs); // log10l(2)
template void OpDispatchBuilder::FLDF64_Const<0x3FE62E42FEFA39EF>(OpcodeArgs); // log(2)
template void OpDispatchBuilder::FLDF64_Const<0>(OpcodeArgs);                  // 0.0

void OpDispatchBuilder::FILDF64(OpcodeArgs) {
  CurrentHeader->HasX87 = true;
  size_t read_width = GetSrcSize(Op);

  // Read from memory
  Ref data = LoadSource_WithOpSize(GPRClass, Op, Op->Src[0], read_width, Op->Flags);
  if (read_width == 2) {
    data = _Sbfe(OpSize::i64Bit, read_width * 8, 0, data);
  }
  auto converted = _Float_FromGPR_S(8, read_width == 4 ? 4 : 8, data);
  _PushStack(converted, i64Bit, false, read_width);
}

template<size_t width>
void OpDispatchBuilder::FSTF64(OpcodeArgs) {

  static_assert(width == 32 || width == 64 || width == 80, "Unsupported FST width");
  CurrentHeader->HasX87 = true;

  if (Op->Src[0].IsNone()) { // Destination is stack
    // FIXME: Why is this case not in x87f64 upstream code?
    auto offset = Op->OP & 7;
    _StoreStackToStack(offset);
  } else {
    Ref Mem = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.LoadData = false});
    _StoreStackMemory(Mem, OpSize::i64Bit, true, width / 8);
  }

  if (Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) {
    _PopStackDestroy();
  }
}

template void OpDispatchBuilder::FSTF64<32>(OpcodeArgs);
template void OpDispatchBuilder::FSTF64<64>(OpcodeArgs);
template void OpDispatchBuilder::FSTF64<80>(OpcodeArgs);

template<bool Truncate>
void OpDispatchBuilder::FISTF64(OpcodeArgs) {
  CurrentHeader->HasX87 = true;
  auto Size = GetSrcSize(Op);

  Ref data = _ReadStackValue(0);
  if constexpr (Truncate) {
    data = _Float_ToGPR_ZS(Size == 4 ? 4 : 8, 8, data);
  } else {
    data = _Float_ToGPR_S(Size == 4 ? 4 : 8, 8, data);
  }
  StoreResult_WithOpSize(GPRClass, Op, Op->Dest, data, Size, 1);

  if ((Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) != 0) {
    _PopStackDestroy();
  }
}

template void OpDispatchBuilder::FISTF64<false>(OpcodeArgs);
template void OpDispatchBuilder::FISTF64<true>(OpcodeArgs);

template<size_t width, bool Integer, OpDispatchBuilder::OpResult ResInST0>
void OpDispatchBuilder::FADDF64(OpcodeArgs) {
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

  if constexpr (Integer) {
    arg = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
    if (width == 16) {
      arg = _Sbfe(OpSize::i64Bit, 16, 0, arg);
    }
    arg = _Float_FromGPR_S(8, width == 64 ? 8 : 4, arg);
  } else if constexpr (width == 32) {
    arg = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
    arg = _Float_FToF(8, 4, arg);
  } else if constexpr (width == 64) {
    arg = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  }

  // top of stack is at offset zero
  _F80AddValue(0, arg);

  if (Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) {
    _PopStackDestroy();
  }
}

template void OpDispatchBuilder::FADDF64<32, false, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template void OpDispatchBuilder::FADDF64<64, false, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template void OpDispatchBuilder::FADDF64<80, false, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template void OpDispatchBuilder::FADDF64<80, false, OpDispatchBuilder::OpResult::RES_STI>(OpcodeArgs);

template void OpDispatchBuilder::FADDF64<16, true, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template void OpDispatchBuilder::FADDF64<32, true, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);

// FIXME: following is very similar to FADDF64
template<size_t width, bool Integer, OpDispatchBuilder::OpResult ResInST0>
void OpDispatchBuilder::FMULF64(OpcodeArgs) {
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

  if constexpr (Integer) {
    arg = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
    if (width == 16) {
      arg = _Sbfe(OpSize::i64Bit, 16, 0, arg);
    }
    arg = _Float_FromGPR_S(8, width == 64 ? 8 : 4, arg);
  } else if constexpr (width == 32) {
    arg = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
    arg = _Float_FToF(8, 4, arg);
  } else if constexpr (width == 64) {
    arg = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
  }

  // top of stack is at offset zero
  _F80MulValue(0, arg);

  if (Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) {
    _PopStackDestroy();
  }
}

template void OpDispatchBuilder::FMULF64<32, false, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template void OpDispatchBuilder::FMULF64<64, false, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template void OpDispatchBuilder::FMULF64<80, false, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template void OpDispatchBuilder::FMULF64<80, false, OpDispatchBuilder::OpResult::RES_STI>(OpcodeArgs);

template void OpDispatchBuilder::FMULF64<16, true, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template void OpDispatchBuilder::FMULF64<32, true, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);

template<size_t width, bool Integer, bool reverse, OpDispatchBuilder::OpResult ResInST0>
void OpDispatchBuilder::FDIVF64(OpcodeArgs) {
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
      if (width == 16) {
        arg = _Sbfe(OpSize::i64Bit, 16, 0, arg);
      }
      arg = _Float_FromGPR_S(8, width == 64 ? 8 : 4, arg);
    } else if constexpr (width == 32) {
      arg = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
      arg = _Float_FToF(8, 4, arg);
    } else if constexpr (width == 64) {
      arg = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
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

template void OpDispatchBuilder::FDIVF64<32, false, false, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template void OpDispatchBuilder::FDIVF64<32, false, true, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);

template void OpDispatchBuilder::FDIVF64<64, false, false, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template void OpDispatchBuilder::FDIVF64<64, false, true, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);

template void OpDispatchBuilder::FDIVF64<80, false, false, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template void OpDispatchBuilder::FDIVF64<80, false, true, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);

template void OpDispatchBuilder::FDIVF64<80, false, false, OpDispatchBuilder::OpResult::RES_STI>(OpcodeArgs);
template void OpDispatchBuilder::FDIVF64<80, false, true, OpDispatchBuilder::OpResult::RES_STI>(OpcodeArgs);

template void OpDispatchBuilder::FDIVF64<16, true, false, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template void OpDispatchBuilder::FDIVF64<16, true, true, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);

template void OpDispatchBuilder::FDIVF64<32, true, false, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template void OpDispatchBuilder::FDIVF64<32, true, true, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);


template<size_t width, bool Integer, bool reverse, OpDispatchBuilder::OpResult ResInST0>
void OpDispatchBuilder::FSUBF64(OpcodeArgs) {
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
      if (width == 16) {
        arg = _Sbfe(OpSize::i64Bit, 16, 0, arg);
      }
      arg = _Float_FromGPR_S(8, width == 64 ? 8 : 4, arg);
    } else if constexpr (width == 32) {
      arg = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
      arg = _Float_FToF(8, 4, arg);
    } else if constexpr (width == 64) {
      arg = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
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

template void OpDispatchBuilder::FSUBF64<32, false, false, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template void OpDispatchBuilder::FSUBF64<32, false, true, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);

template void OpDispatchBuilder::FSUBF64<64, false, false, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template void OpDispatchBuilder::FSUBF64<64, false, true, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);

template void OpDispatchBuilder::FSUBF64<80, false, false, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template void OpDispatchBuilder::FSUBF64<80, false, true, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);

template void OpDispatchBuilder::FSUBF64<80, false, false, OpDispatchBuilder::OpResult::RES_STI>(OpcodeArgs);
template void OpDispatchBuilder::FSUBF64<80, false, true, OpDispatchBuilder::OpResult::RES_STI>(OpcodeArgs);

template void OpDispatchBuilder::FSUBF64<16, true, false, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template void OpDispatchBuilder::FSUBF64<16, true, true, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);

template void OpDispatchBuilder::FSUBF64<32, true, false, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template void OpDispatchBuilder::FSUBF64<32, true, true, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);


void OpDispatchBuilder::FTSTF64(OpcodeArgs) {
  CurrentHeader->HasX87 = true;

  // We are going to clobber NZCV, make sure it's in a GPR first.
  GetNZCV();

  // Now we do our comparison.
  _F80StackTest(0, 0 /*No flags in reduced precision mode needed*/);
  PossiblySetNZCVBits = ~0;
  ConvertNZCVToX87();
}

template<size_t width, bool Integer, OpDispatchBuilder::FCOMIFlags whichflags, bool poptwice>
void OpDispatchBuilder::FCOMIF64(OpcodeArgs) {
  CurrentHeader->HasX87 = true;
  Ref arg {};
  Ref b {};

  if (Op->Src[0].IsNone()) {
    // Implicit arg
    uint8_t offset = Op->OP & 7;
    b = _ReadStackValue(offset);
  } else {
    // Memory arg
    if constexpr (width == 16 || width == 32 || width == 64) {
      if constexpr (Integer) {
        arg = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags);
        if (width == 16) {
          arg = _Sbfe(OpSize::i64Bit, 16, 0, arg);
        }
        b = _Float_FromGPR_S(8, width == 64 ? 8 : 4, arg);
      } else if constexpr (width == 32) {
        arg = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
        b = _Float_FToF(8, 4, arg);
      } else if constexpr (width == 64) {
        b = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
      }
    }
  }

  if constexpr (whichflags == FCOMIFlags::FLAGS_X87) {
    // We are going to clobber NZCV, make sure it's in a GPR first.
    GetNZCV();

    _F80CmpValue(b, 0 /*Flags are unused in x87f64*/);
    PossiblySetNZCVBits = ~0;
    ConvertNZCVToX87();
  } else {
    HandleNZCVWrite();
    _F80CmpValue(b, 0 /*Flags are unused in x87f64*/);
    ComissFlags(true /* InvalidateAF */);
  }

  if constexpr (poptwice) {
    _PopStackDestroy();
    _PopStackDestroy();
  } else if ((Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) != 0) {
    _PopStackDestroy();
  }
}

template void OpDispatchBuilder::FCOMIF64<32, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>(OpcodeArgs);

template void OpDispatchBuilder::FCOMIF64<64, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>(OpcodeArgs);

template void OpDispatchBuilder::FCOMIF64<80, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>(OpcodeArgs);
template void OpDispatchBuilder::FCOMIF64<80, false, OpDispatchBuilder::FCOMIFlags::FLAGS_RFLAGS, false>(OpcodeArgs);
template void OpDispatchBuilder::FCOMIF64<80, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, true>(OpcodeArgs);

template void OpDispatchBuilder::FCOMIF64<16, true, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>(OpcodeArgs);

template void OpDispatchBuilder::FCOMIF64<32, true, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>(OpcodeArgs);

void OpDispatchBuilder::X87FYL2XF64(OpcodeArgs) {
  CurrentHeader->HasX87 = true;

  if (Op->OP == 0x01F9) { // fyl2xp1
    // create an add between top of stack and 1.
    Ref One = _VCastFromGPR(8, 8, _Constant(0x3FF0000000000000));
    _F80AddValue(0, One);
  }

  _F80StackFYL2X();
}


// This function converts to F80 on save for compatibility

void OpDispatchBuilder::X87FNSAVEF64(OpcodeArgs) {
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
  CurrentHeader->HasX87 = true;
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
  _SetRoundingMode(roundingMode);
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

} // namespace FEXCore::IR
