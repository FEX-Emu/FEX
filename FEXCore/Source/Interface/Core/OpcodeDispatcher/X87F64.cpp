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

//Functions in X87.cpp (no change required)
//GetX87Top
//SetX87TopTag
//GetX87FTW
//SetX87Top
//X87ModifySTP
//EMMS
//FFREE
//FNSTENV
//FSTCW
//LDSW
//FNSTSW
//FXCH
//FCMOV
//FST(register to register)

// State loading duplicated from X87.cpp, setting host rounding mode
// See issue
void OpDispatchBuilder::FNINITF64(OpcodeArgs) {
  // Init FCW to 0x037F
  auto NewFCW = _Constant(16, 0x037F);
  _F80LoadFCW(NewFCW);
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

  // Tags all get set to 0b11
  _StoreContext(2, GPRClass, _Constant(0xFFFF), offsetof(FEXCore::Core::CPUState, FTW));
}

void OpDispatchBuilder::X87LDENVF64(OpcodeArgs) {
  auto Size = GetSrcSize(Op);
  OrderedNode *Mem = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1, false);
  Mem = AppendSegmentOffset(Mem, Op->Flags);

  auto NewFCW = _LoadMem(GPRClass, 2, Mem, 2);
  //ignore the rounding precision, we're always 64-bit in F64.
  //extract rounding mode
  OrderedNode *roundingMode = NewFCW;
  auto roundShift = _Constant(10);
  auto roundMask = _Constant(3);
  roundingMode = _Lshr(OpSize::i32Bit, roundingMode, roundShift);
  roundingMode = _And(OpSize::i32Bit, roundingMode, roundMask);
  _SetRoundingMode(roundingMode);
  _F80LoadFCW(NewFCW);

  _StoreContext(2, GPRClass, NewFCW, offsetof(FEXCore::Core::CPUState, FCW));

  OrderedNode *MemLocation = _Add(OpSize::i64Bit, Mem, _Constant(Size * 1));
  auto NewFSW = _LoadMem(GPRClass, Size, MemLocation, Size);
  ReconstructX87StateFromFSW(NewFSW);

  {
    // FTW
    OrderedNode *MemLocation = _Add(OpSize::i64Bit, Mem, _Constant(Size * 2));
    auto NewFTW = _LoadMem(GPRClass, Size, MemLocation, Size);
    _StoreContext(2, GPRClass, NewFTW, offsetof(FEXCore::Core::CPUState, FTW));
  }
}


void OpDispatchBuilder::X87FLDCWF64(OpcodeArgs) {
  OrderedNode *NewFCW = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);
  _F80LoadFCW(NewFCW); //keeps BCD code working
  //ignore the rounding precision, we're always 64-bit in F64.
  //extract rounding mode
  OrderedNode *roundingMode = NewFCW;
  auto shift = _Constant(10);
  auto mask = _Constant(3);
  roundingMode = _Lshr(OpSize::i32Bit, roundingMode, shift);
  roundingMode = _And(OpSize::i32Bit, roundingMode, mask);
  _SetRoundingMode(roundingMode);
  _StoreContext(2, GPRClass, NewFCW, offsetof(FEXCore::Core::CPUState, FCW));
}

// F64 ops

template<size_t width>
void OpDispatchBuilder::FLDF64(OpcodeArgs) {
  // Update TOP
  auto orig_top = GetX87Top();
  auto mask = _Constant(7);

  size_t read_width = (width == 80) ? 16 : width / 8;

  OrderedNode *data{};
  OrderedNode *converted{};

  if (!Op->Src[0].IsNone()) {
    // Read from memory
    data = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], read_width, Op->Flags, -1);
     // Convert to 64bit float
    if constexpr (width == 32) {
      converted = _Float_FToF(8, 4, data);
    } else if constexpr (width == 80) {
      converted = _F80CVT(8, data);
    } else {
      converted = data;
    }
  }
  else {
    // Implicit arg (does this need to change with width?)
    auto offset = _Constant(Op->OP & 7);
    data = _And(OpSize::i64Bit, _Add(OpSize::i64Bit, orig_top, offset), mask);
    data = _LoadContextIndexed(data, 8, MMBaseOffset(), 16, FPRClass);
    converted = data;
  }

  auto top = _And(OpSize::i64Bit, _Sub(OpSize::i64Bit, orig_top, _Constant(1)), mask);
  SetX87TopTag(top, X87Tag::Valid);
  SetX87Top(top);
  // Write to ST[TOP]
  _StoreContextIndexed(converted, top, 8, MMBaseOffset(), 16, FPRClass);
}

template
void OpDispatchBuilder::FLDF64<32>(OpcodeArgs);
template
void OpDispatchBuilder::FLDF64<64>(OpcodeArgs);
template
void OpDispatchBuilder::FLDF64<80>(OpcodeArgs);

void OpDispatchBuilder::FBLDF64(OpcodeArgs) {
  // Update TOP
  auto orig_top = GetX87Top();
  auto mask = _Constant(7);
  auto top = _And(OpSize::i64Bit, _Sub(OpSize::i64Bit, orig_top, _Constant(1)), mask);
  SetX87TopTag(top, X87Tag::Valid);
  SetX87Top(top);

  // Read from memory
  OrderedNode *data = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], 16, Op->Flags, -1);
  OrderedNode *converted = _F80BCDLoad(data);
  converted = _F80CVT(8, converted);
  _StoreContextIndexed(converted, top, 8, MMBaseOffset(), 16, FPRClass);
}

void OpDispatchBuilder::FBSTPF64(OpcodeArgs) {
  auto orig_top = GetX87Top();
  auto data = _LoadContextIndexed(orig_top, 8, MMBaseOffset(), 16, FPRClass);

  OrderedNode *converted = _F80CVTTo(data, 8);
  converted = _F80BCDStore(converted);

  StoreResult_WithOpSize(FPRClass, Op, Op->Dest, converted, 10, 1);

  // if we are popping then we must first mark this location as empty
  SetX87TopTag(orig_top, X87Tag::Empty);
  auto top = _And(OpSize::i64Bit, _Add(OpSize::i64Bit, orig_top, _Constant(1)), _Constant(7));
  SetX87Top(top);
}

template<uint64_t num>
void OpDispatchBuilder::FLDF64_Const(OpcodeArgs) {
  // Update TOP
  auto orig_top = GetX87Top();
  auto top = _And(OpSize::i64Bit, _Sub(OpSize::i64Bit, orig_top, _Constant(1)), _Constant(7));
  SetX87TopTag(top, X87Tag::Valid);
  SetX87Top(top);
  auto data = _VCastFromGPR(8, 8, _Constant(num));
  // Write to ST[TOP]
  _StoreContextIndexed(data, top, 8, MMBaseOffset(), 16, FPRClass);
}

template
void OpDispatchBuilder::FLDF64_Const<0x3FF0000000000000>(OpcodeArgs); // 1.0
template
void OpDispatchBuilder::FLDF64_Const<0x400A934F0979A372>(OpcodeArgs); // log2l(10)
template
void OpDispatchBuilder::FLDF64_Const<0x3FF71547652B82FE>(OpcodeArgs); // log2l(e)
template
void OpDispatchBuilder::FLDF64_Const<0x400921FB54442D18>(OpcodeArgs); // pi
template
void OpDispatchBuilder::FLDF64_Const<0x3FD34413509F79FF>(OpcodeArgs); // log10l(2)
template
void OpDispatchBuilder::FLDF64_Const<0x3FE62E42FEFA39EF>(OpcodeArgs); // log(2)
template
void OpDispatchBuilder::FLDF64_Const<0>(OpcodeArgs); // 0.0

void OpDispatchBuilder::FILDF64(OpcodeArgs) {
  // Update TOP
  auto orig_top = GetX87Top();
  auto top = _And(OpSize::i64Bit, _Sub(OpSize::i64Bit, orig_top, _Constant(1)), _Constant(7));
  SetX87TopTag(top, X87Tag::Valid);
  SetX87Top(top);

  size_t read_width = GetSrcSize(Op);
  // Read from memory
  auto data = LoadSource_WithOpSize(GPRClass, Op, Op->Src[0], read_width, Op->Flags, -1);
  if(read_width == 2) {
    data = _Sbfe(OpSize::i64Bit, read_width * 8, 0, data);
  }
  auto converted = _Float_FromGPR_S(8, read_width == 4 ? 4 : 8, data);
  // Write to ST[TOP]
  _StoreContextIndexed(converted, top, 8, MMBaseOffset(), 16, FPRClass);
}

template<size_t width>
void OpDispatchBuilder::FSTF64(OpcodeArgs) {
  auto orig_top = GetX87Top();
  auto data = _LoadContextIndexed(orig_top, 8, MMBaseOffset(), 16, FPRClass);
  if constexpr (width == 64) {
    //Store 64-bit float directly
    StoreResult_WithOpSize(FPRClass, Op, Op->Dest, data, 8, 1);
  } else if constexpr (width == 32) {
    //Convert to 32-bit float and store
    auto result = _Float_FToF(4, 8, data);
    StoreResult_WithOpSize(FPRClass, Op, Op->Dest, result, 4, 1);
  } else if constexpr (width == 80) {
    //Convert to 80-bit float
    auto result = _F80CVTTo(data, 8);
    StoreResult_WithOpSize(FPRClass, Op, Op->Dest, result, 10, 1);
  }

  if ((Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) != 0) {
    // if we are popping then we must first mark this location as empty
    SetX87TopTag(orig_top, X87Tag::Empty);
    // Set the new top now
    auto top = _And(OpSize::i64Bit, _Add(OpSize::i64Bit, orig_top, _Constant(1)), _Constant(7));
    SetX87Top(top);
  }
}

template
void OpDispatchBuilder::FSTF64<32>(OpcodeArgs);
template
void OpDispatchBuilder::FSTF64<64>(OpcodeArgs);
template
void OpDispatchBuilder::FSTF64<80>(OpcodeArgs);

template<bool Truncate>
void OpDispatchBuilder::FISTF64(OpcodeArgs) {
  auto Size = GetSrcSize(Op);

  auto orig_top = GetX87Top();
  OrderedNode *data = _LoadContextIndexed(orig_top, 8, MMBaseOffset(), 16, FPRClass);
  if constexpr (Truncate) {
    data = _Float_ToGPR_ZS(Size == 4 ? 4 : 8, 8, data);
  } else {
    data = _Float_ToGPR_S(Size == 4 ? 4 : 8, 8, data);
  }
  StoreResult_WithOpSize(GPRClass, Op, Op->Dest, data, Size, 1);

  if ((Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) != 0) {
    // if we are popping then we must first mark this location as empty
    SetX87TopTag(orig_top, X87Tag::Empty);
    // Set the new top now
    auto top = _And(OpSize::i64Bit, _Add(OpSize::i64Bit, orig_top, _Constant(1)), _Constant(7));
    SetX87Top(top);
  }
}

template
void OpDispatchBuilder::FISTF64<false>(OpcodeArgs);
template
void OpDispatchBuilder::FISTF64<true>(OpcodeArgs);

template <size_t width, bool Integer, OpDispatchBuilder::OpResult ResInST0>
void OpDispatchBuilder::FADDF64(OpcodeArgs) {
  auto top = GetX87Top();
  OrderedNode *StackLocation = top;

  OrderedNode *arg{};
  OrderedNode *b{};

  auto mask = _Constant(7);

  if (!Op->Src[0].IsNone()) {
    // Memory arg
    if constexpr (Integer) {
      arg = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);
      if(width == 16) {
        arg = _Sbfe(OpSize::i64Bit, 16, 0, arg);
      }
      b = _Float_FromGPR_S(8, width == 64 ? 8 : 4, arg);
    } else if constexpr (width == 32) {
      arg = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
      b = _Float_FToF(8, 4, arg);
    } else if constexpr (width == 64) {
      b = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
    }
  } else {
    // Implicit arg
    auto offset = _Constant(Op->OP & 7);
    arg = _And(OpSize::i64Bit, _Add(OpSize::i64Bit, top, offset), mask);
    if constexpr (ResInST0 == OpResult::RES_STI) {
      StackLocation = arg;
    }
    b = _LoadContextIndexed(arg, 8, MMBaseOffset(), 16, FPRClass);
  }

  auto a = _LoadContextIndexed(top, 8, MMBaseOffset(), 16, FPRClass);
  auto result = _VFAdd(8, 8, a, b);
  if ((Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) != 0) {
    // if we are popping then we must first mark this location as empty
    SetX87TopTag(top, X87Tag::Empty);
    // Set the new top now
    top = _And(OpSize::i64Bit, _Add(OpSize::i64Bit, top, _Constant(1)), mask);
    SetX87Top(top);
  }

  // Write to ST[TOP]
  _StoreContextIndexed(result, StackLocation, 8, MMBaseOffset(), 16, FPRClass);
}

template
void OpDispatchBuilder::FADDF64<32, false, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template
void OpDispatchBuilder::FADDF64<64, false, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template
void OpDispatchBuilder::FADDF64<80, false, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template
void OpDispatchBuilder::FADDF64<80, false, OpDispatchBuilder::OpResult::RES_STI>(OpcodeArgs);

template
void OpDispatchBuilder::FADDF64<16, true, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template
void OpDispatchBuilder::FADDF64<32, true, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);

template<size_t width, bool Integer, OpDispatchBuilder::OpResult ResInST0>
void OpDispatchBuilder::FMULF64(OpcodeArgs) {
  auto top = GetX87Top();
  OrderedNode *StackLocation = top;
  OrderedNode *arg{};
  OrderedNode *b{};

  auto mask = _Constant(7);

  if (!Op->Src[0].IsNone()) {
    // Memory arg
    if constexpr (Integer) {
      arg = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);
      if(width == 16) {
        arg = _Sbfe(OpSize::i64Bit, 16, 0, arg);
      }
      b = _Float_FromGPR_S(8, width == 64 ? 8 : 4, arg);
    } else if constexpr (width == 32) {
      arg = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
      b = _Float_FToF(8, 4, arg);
    } else if constexpr (width == 64) {
      b = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
    }
  } else {
    // Implicit arg
    auto offset = _Constant(Op->OP & 7);
    arg = _And(OpSize::i64Bit, _Add(OpSize::i64Bit, top, offset), mask);
    if constexpr (ResInST0 == OpResult::RES_STI) {
      StackLocation = arg;
    }

    b = _LoadContextIndexed(arg, 8, MMBaseOffset(), 16, FPRClass);
  }

  auto a = _LoadContextIndexed(top, 8, MMBaseOffset(), 16, FPRClass);

  auto result = _VFMul(8, 8, a, b);

  if ((Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) != 0) {
    // if we are popping then we must first mark this location as empty
    SetX87TopTag(top, X87Tag::Empty);
    // Set the new top now
    top = _And(OpSize::i64Bit, _Add(OpSize::i64Bit, top, _Constant(1)), mask);
    SetX87Top(top);
  }

  // Write to ST[TOP]
  _StoreContextIndexed(result, StackLocation, 8, MMBaseOffset(), 16, FPRClass);
}

template
void OpDispatchBuilder::FMULF64<32, false, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template
void OpDispatchBuilder::FMULF64<64, false, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template
void OpDispatchBuilder::FMULF64<80, false, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template
void OpDispatchBuilder::FMULF64<80, false, OpDispatchBuilder::OpResult::RES_STI>(OpcodeArgs);

template
void OpDispatchBuilder::FMULF64<16, true, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template
void OpDispatchBuilder::FMULF64<32, true, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);

template<size_t width, bool Integer, bool reverse, OpDispatchBuilder::OpResult ResInST0>
void OpDispatchBuilder::FDIVF64(OpcodeArgs) {
  auto top = GetX87Top();
  OrderedNode *StackLocation = top;
  OrderedNode *arg{};
  OrderedNode *b{};

  auto mask = _Constant(7);

 if (!Op->Src[0].IsNone()) {
    // Memory arg
    if constexpr (Integer) {
      arg = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);
      if(width == 16) {
        arg = _Sbfe(OpSize::i64Bit, 16, 0, arg);
      }
      b = _Float_FromGPR_S(8, width == 64 ? 8 : 4, arg);
    } else if constexpr (width == 32) {
      arg = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
      b = _Float_FToF(8, 4, arg);
    } else if constexpr (width == 64) {
      b = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
    }
  } else {
    // Implicit arg
    auto offset = _Constant(Op->OP & 7);
    arg = _And(OpSize::i64Bit, _Add(OpSize::i64Bit, top, offset), mask);
    if constexpr (ResInST0 == OpResult::RES_STI) {
      StackLocation = arg;
    }

    b = _LoadContextIndexed(arg, 8, MMBaseOffset(), 16, FPRClass);
  }

  auto a = _LoadContextIndexed(top, 8, MMBaseOffset(), 16, FPRClass);

  OrderedNode *result{};
  if constexpr (reverse) {
    result = _VFDiv(8, 8, b, a);
  }
  else {
    result = _VFDiv(8, 8, a, b);
  }

  if ((Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) != 0) {
    // if we are popping then we must first mark this location as empty
    SetX87TopTag(top, X87Tag::Empty);
    // Set the new top now
    top = _And(OpSize::i64Bit, _Add(OpSize::i64Bit, top, _Constant(1)), mask);
    SetX87Top(top);
  }

  // Write to ST[TOP]
  _StoreContextIndexed(result, StackLocation, 8, MMBaseOffset(), 16, FPRClass);
}

template
void OpDispatchBuilder::FDIVF64<32, false, false, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template
void OpDispatchBuilder::FDIVF64<32, false, true, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);

template
void OpDispatchBuilder::FDIVF64<64, false, false, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template
void OpDispatchBuilder::FDIVF64<64, false, true, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);

template
void OpDispatchBuilder::FDIVF64<80, false, false, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template
void OpDispatchBuilder::FDIVF64<80, false, true, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);

template
void OpDispatchBuilder::FDIVF64<80, false, false, OpDispatchBuilder::OpResult::RES_STI>(OpcodeArgs);
template
void OpDispatchBuilder::FDIVF64<80, false, true, OpDispatchBuilder::OpResult::RES_STI>(OpcodeArgs);

template
void OpDispatchBuilder::FDIVF64<16, true, false, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template
void OpDispatchBuilder::FDIVF64<16, true, true, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);

template
void OpDispatchBuilder::FDIVF64<32, true, false, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template
void OpDispatchBuilder::FDIVF64<32, true, true, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);

template<size_t width, bool Integer, bool reverse, OpDispatchBuilder::OpResult ResInST0>
void OpDispatchBuilder::FSUBF64(OpcodeArgs) {
  auto top = GetX87Top();
  OrderedNode *StackLocation = top;
  OrderedNode *arg{};
  OrderedNode *b{};

  auto mask = _Constant(7);

 if (!Op->Src[0].IsNone()) {
    // Memory arg
    if constexpr (Integer) {
      arg = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);
      if(width == 16) {
        arg = _Sbfe(OpSize::i64Bit, 16, 0, arg);
      }
      b = _Float_FromGPR_S(8, width == 64 ? 8 : 4, arg);
    } else if constexpr (width == 32) {
      arg = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
      b = _Float_FToF(8, 4, arg);
    } else if constexpr (width == 64) {
      b = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
    }
  } else {
    // Implicit arg
    auto offset = _Constant(Op->OP & 7);
    arg = _And(OpSize::i64Bit, _Add(OpSize::i64Bit, top, offset), mask);
    if constexpr (ResInST0 == OpResult::RES_STI) {
      StackLocation = arg;
    }

    b = _LoadContextIndexed(arg, 8, MMBaseOffset(), 16, FPRClass);
  }

  auto a = _LoadContextIndexed(top, 8, MMBaseOffset(), 16, FPRClass);

  OrderedNode *result{};
  if constexpr (reverse) {
    result = _VFSub(8, 8, b, a);
  }
  else {
    result = _VFSub(8, 8, a, b);
  }

  if ((Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) != 0) {
    // if we are popping then we must first mark this location as empty
    SetX87TopTag(top, X87Tag::Empty);
    // Set the new top now

    top = _And(OpSize::i64Bit, _Add(OpSize::i64Bit, top, _Constant(1)), mask);
    SetX87Top(top);
  }

  // Write to ST[TOP]
  _StoreContextIndexed(result, StackLocation, 8, MMBaseOffset(), 16, FPRClass);
}

template
void OpDispatchBuilder::FSUBF64<32, false, false, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template
void OpDispatchBuilder::FSUBF64<32, false, true, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);

template
void OpDispatchBuilder::FSUBF64<64, false, false, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template
void OpDispatchBuilder::FSUBF64<64, false, true, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);

template
void OpDispatchBuilder::FSUBF64<80, false, false, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template
void OpDispatchBuilder::FSUBF64<80, false, true, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);

template
void OpDispatchBuilder::FSUBF64<80, false, false, OpDispatchBuilder::OpResult::RES_STI>(OpcodeArgs);
template
void OpDispatchBuilder::FSUBF64<80, false, true, OpDispatchBuilder::OpResult::RES_STI>(OpcodeArgs);

template
void OpDispatchBuilder::FSUBF64<16, true, false, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template
void OpDispatchBuilder::FSUBF64<16, true, true, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);

template
void OpDispatchBuilder::FSUBF64<32, true, false, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);
template
void OpDispatchBuilder::FSUBF64<32, true, true, OpDispatchBuilder::OpResult::RES_ST0>(OpcodeArgs);

void OpDispatchBuilder::FCHSF64(OpcodeArgs) {
  auto top = GetX87Top();
  auto a = _LoadContextIndexed(top, 8, MMBaseOffset(), 16, FPRClass);
  auto b = _VCastFromGPR(8, 8, _Constant(0x8000000000000000));

  auto result = _VXor(8, 8, a, b);
  // Write to ST[TOP]
  _StoreContextIndexed(result, top, 8, MMBaseOffset(), 16, FPRClass);
}

void OpDispatchBuilder::FABSF64(OpcodeArgs) {
  auto top = GetX87Top();
  auto a = _LoadContextIndexed(top, 8, MMBaseOffset(), 16, FPRClass);
  auto b = _VCastFromGPR(8, 8, _Constant(0x7fffffffffffffff));
  auto result = _VAnd(8, 8, a, b);

  // Write to ST[TOP]
  _StoreContextIndexed(result, top, 8, MMBaseOffset(), 16, FPRClass);
}

void OpDispatchBuilder::FTSTF64(OpcodeArgs) {
  auto top = GetX87Top();
  auto a = _LoadContextIndexed(top, 8, MMBaseOffset(), 16, FPRClass);

  auto low = _Constant(0);
  OrderedNode *data = _VCastFromGPR(8, 8, low);

  OrderedNode *Res = _FCmp(8, a, data,
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

//TODO: This should obey rounding mode
void OpDispatchBuilder::FRNDINTF64(OpcodeArgs) {
  auto top = GetX87Top();
  auto a = _LoadContextIndexed(top, 8, MMBaseOffset(), 16, FPRClass);

  auto result = _Vector_FToI(8, 8, a, FEXCore::IR::Round_Host);

  // Write to ST[TOP]
  _StoreContextIndexed(result, top, 8, MMBaseOffset(), 16, FPRClass);
}

void OpDispatchBuilder::FXTRACTF64(OpcodeArgs) {
  auto orig_top = GetX87Top();
  auto top = _And(OpSize::i64Bit, _Sub(OpSize::i64Bit, orig_top, _Constant(1)), _Constant(7));
  SetX87TopTag(top, X87Tag::Valid);
  SetX87Top(top);

  auto a = _LoadContextIndexed(orig_top, 8, MMBaseOffset(), 16, FPRClass);
  auto gpr = _VExtractToGPR(8, 8, a, 0);
  OrderedNode* exp = _And(OpSize::i64Bit, gpr, _Constant(0x7ff0000000000000LL));
  exp = _Lshr(OpSize::i64Bit, exp, _Constant(52));
  exp = _Sub(OpSize::i64Bit, exp, _Constant(1023));
  exp = _Float_FromGPR_S(8, 8, exp);
  OrderedNode* sig = _And(OpSize::i64Bit, gpr, _Constant(0x800fffffffffffffLL));
  sig = _Or(OpSize::i64Bit, sig, _Constant(0x3ff0000000000000LL));
  sig = _VCastFromGPR(8, 8, sig);
  // Write to ST[TOP]
  _StoreContextIndexed(exp, orig_top, 8, MMBaseOffset(), 16, FPRClass);
  _StoreContextIndexed(sig, top, 8, MMBaseOffset(), 16, FPRClass);
}


template<size_t width, bool Integer, OpDispatchBuilder::FCOMIFlags whichflags, bool poptwice>
void OpDispatchBuilder::FCOMIF64(OpcodeArgs) {
  auto top = GetX87Top();
  auto mask = _Constant(7);

  OrderedNode *arg{};
  OrderedNode *b{};

  if (!Op->Src[0].IsNone()) {
    // Memory arg
    if constexpr (Integer) {
      arg = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1);
      if(width == 16) {
        arg = _Sbfe(OpSize::i64Bit, 16, 0, arg);
      }
      b = _Float_FromGPR_S(8, width == 64 ? 8 : 4, arg);
    } else if constexpr (width == 32) {
      arg = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
      b = _Float_FToF(8, 4, arg);
    } else if constexpr (width == 64) {
      b = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags, -1);
    }
  } else {
    // Implicit arg
    auto offset = _Constant(Op->OP & 7);
    arg = _And(OpSize::i64Bit, _Add(OpSize::i64Bit, top, offset), mask);
    b = _LoadContextIndexed(arg, 8, MMBaseOffset(), 16, FPRClass);
  }

  auto a = _LoadContextIndexed(top, 8, MMBaseOffset(), 16, FPRClass);

  OrderedNode *Res = _FCmp(8, a, b,
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

    SetRFLAG<FEXCore::X86State::RFLAG_CF_LOC>(HostFlag_CF);
    SetRFLAG<FEXCore::X86State::RFLAG_ZF_LOC>(HostFlag_ZF);
    SetRFLAG<FEXCore::X86State::RFLAG_PF_LOC>(HostFlag_Unordered);
  }

  if constexpr (poptwice) {
    // if we are popping then we must first mark this location as empty
    SetX87TopTag(top, X87Tag::Empty);
    top = _And(OpSize::i64Bit, _Add(OpSize::i64Bit, top, _Constant(1)), mask);
    SetX87TopTag(top, X87Tag::Empty);
    // Set the new top now
    top = _And(OpSize::i64Bit, _Add(OpSize::i64Bit, top, _Constant(1)), mask);
    SetX87Top(top);
  }
  else if ((Op->TableInfo->Flags & X86Tables::InstFlags::FLAGS_POP) != 0) {
    // if we are popping then we must first mark this location as empty
    SetX87TopTag(top, X87Tag::Empty);
    // Set the new top now
    top = _And(OpSize::i64Bit, _Add(OpSize::i64Bit, top, _Constant(1)), mask);
    SetX87Top(top);
  }
}

template
void OpDispatchBuilder::FCOMIF64<32, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>(OpcodeArgs);

template
void OpDispatchBuilder::FCOMIF64<64, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>(OpcodeArgs);

template
void OpDispatchBuilder::FCOMIF64<80, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>(OpcodeArgs);
template
void OpDispatchBuilder::FCOMIF64<80, false, OpDispatchBuilder::FCOMIFlags::FLAGS_RFLAGS, false>(OpcodeArgs);
template
void OpDispatchBuilder::FCOMIF64<80, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, true>(OpcodeArgs);

template
void OpDispatchBuilder::FCOMIF64<16, true, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>(OpcodeArgs);

template
void OpDispatchBuilder::FCOMIF64<32, true, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>(OpcodeArgs);


void OpDispatchBuilder::FSQRTF64(OpcodeArgs) {
  auto top = GetX87Top();
  auto a = _LoadContextIndexed(top, 8, MMBaseOffset(), 16, FPRClass);

  auto result = _VFSqrt(8, 8, a);

  // Write to ST[TOP]
  _StoreContextIndexed(result, top, 8, MMBaseOffset(), 16, FPRClass);
}


template<FEXCore::IR::IROps IROp>
void OpDispatchBuilder::X87UnaryOpF64(OpcodeArgs) {
  auto top = GetX87Top();
  auto a = _LoadContextIndexed(top, 8, MMBaseOffset(), 16, FPRClass);

  auto result = _F64SIN(a);
  // Overwrite the op
  result.first->Header.Op = IROp;

  if constexpr (IROp == IR::OP_F64SIN ||
                IROp == IR::OP_F64COS) {
    // TODO: ACCURACY: should check source is in range –2^63 to +2^63
    SetRFLAG<FEXCore::X86State::X87FLAG_C2_LOC>(_Constant(0));
  }

  // Write to ST[TOP]
  _StoreContextIndexed(result, top, 8, MMBaseOffset(), 16, FPRClass);
}

template
void OpDispatchBuilder::X87UnaryOpF64<IR::OP_F64F2XM1>(OpcodeArgs);
template
void OpDispatchBuilder::X87UnaryOpF64<IR::OP_F64SIN>(OpcodeArgs);
template
void OpDispatchBuilder::X87UnaryOpF64<IR::OP_F64COS>(OpcodeArgs);


template<FEXCore::IR::IROps IROp>
void OpDispatchBuilder::X87BinaryOpF64(OpcodeArgs) {
  auto top = GetX87Top();

  auto mask = _Constant(7);
  OrderedNode *st1 = _And(OpSize::i64Bit, _Add(OpSize::i64Bit, top, _Constant(1)), mask);

  auto a = _LoadContextIndexed(top, 8, MMBaseOffset(), 16, FPRClass);
  st1 = _LoadContextIndexed(st1, 8, MMBaseOffset(), 16, FPRClass);

  auto result = _F64ATAN(a, st1);
  // Overwrite the op
  result.first->Header.Op = IROp;

  if constexpr (IROp == IR::OP_F64FPREM ||
    IROp == IR::OP_F64FPREM1) {
    //TODO: Set C0 to Q2, C3 to Q1, C1 to Q0
    SetRFLAG<FEXCore::X86State::X87FLAG_C2_LOC>(_Constant(0));
  }

  // Write to ST[TOP]
  _StoreContextIndexed(result, top, 8, MMBaseOffset(), 16, FPRClass);
}

template
void OpDispatchBuilder::X87BinaryOpF64<IR::OP_F64FPREM1>(OpcodeArgs);
template
void OpDispatchBuilder::X87BinaryOpF64<IR::OP_F64FPREM>(OpcodeArgs);
template
void OpDispatchBuilder::X87BinaryOpF64<IR::OP_F64SCALE>(OpcodeArgs);

void OpDispatchBuilder::X87SinCosF64(OpcodeArgs) {
  auto orig_top = GetX87Top();
  auto top = _And(OpSize::i64Bit, _Sub(OpSize::i64Bit, orig_top, _Constant(1)), _Constant(7));
  SetX87TopTag(top, X87Tag::Valid);
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
  SetX87TopTag(orig_top, X87Tag::Empty);
  auto top = _And(OpSize::i64Bit, _Add(OpSize::i64Bit, orig_top, _Constant(1)), _Constant(7));
  SetX87Top(top);

  OrderedNode *st0 = _LoadContextIndexed(orig_top, 8, MMBaseOffset(), 16, FPRClass);
  OrderedNode *st1 = _LoadContextIndexed(top, 8, MMBaseOffset(), 16, FPRClass);

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
  auto top = _And(OpSize::i64Bit, _Sub(OpSize::i64Bit, orig_top, _Constant(1)), _Constant(7));
  SetX87TopTag(top, X87Tag::Valid);
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
  SetX87TopTag(orig_top, X87Tag::Empty);
  auto top = _And(OpSize::i64Bit, _Add(OpSize::i64Bit, orig_top, _Constant(1)), _Constant(7));
  SetX87Top(top);

  auto a = _LoadContextIndexed(orig_top, 8, MMBaseOffset(), 16, FPRClass);
  OrderedNode *st1 = _LoadContextIndexed(top, 8, MMBaseOffset(), 16, FPRClass);

  auto result = _F64ATAN(st1, a);

  // Write to ST[TOP]
  _StoreContextIndexed(result, top, 8, MMBaseOffset(), 16, FPRClass);
}

//This function converts to F80 on save for compatibility

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

  auto Size = GetDstSize(Op);
  OrderedNode *Mem = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, -1, false);
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
    auto FTW = _LoadContext(2, GPRClass, offsetof(FEXCore::Core::CPUState, FTW));
    _StoreMem(GPRClass, Size, MemLocation, FTW, Size);
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
    OrderedNode* data = _LoadContextIndexed(Top, 8, MMBaseOffset(), 16, FPRClass);
    data = _F80CVTTo(data, 8);
    _StoreMem(FPRClass, 16, ST0Location, data, 1);
    ST0Location = _Add(OpSize::i64Bit, ST0Location, TenConst);
    Top = _And(OpSize::i64Bit, _Add(OpSize::i64Bit, Top, OneConst), SevenConst);
  }

  // The final st(7) needs a bit of special handling here
  OrderedNode* data = _LoadContextIndexed(Top, 8, MMBaseOffset(), 16, FPRClass);
  data = _F80CVTTo(data, 8);
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

//This function converts from F80 on load for compatibility

void OpDispatchBuilder::X87FRSTORF64(OpcodeArgs) {
  auto Size = GetSrcSize(Op);
  OrderedNode *Mem = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, -1, false);
  Mem = AppendSegmentOffset(Mem, Op->Flags);

  auto NewFCW = _LoadMem(GPRClass, 2, Mem, 2);
  //ignore the rounding precision, we're always 64-bit in F64.
  //extract rounding mode
  OrderedNode *roundingMode = NewFCW;
  auto roundShift = _Constant(10);
  auto roundMask = _Constant(3);
  roundingMode = _Lshr(OpSize::i32Bit, roundingMode, roundShift);
  roundingMode = _And(OpSize::i32Bit, roundingMode, roundMask);
  _SetRoundingMode(roundingMode);
  _F80LoadFCW(NewFCW);
  _StoreContext(2, GPRClass, NewFCW, offsetof(FEXCore::Core::CPUState, FCW));
  _StoreContext(2, GPRClass, NewFCW, offsetof(FEXCore::Core::CPUState, FCW));

  OrderedNode *MemLocation = _Add(OpSize::i64Bit, Mem, _Constant(Size * 1));
  auto NewFSW = _LoadMem(GPRClass, Size, MemLocation, Size);
  auto Top = ReconstructX87StateFromFSW(NewFSW);

  {
    // FTW
    OrderedNode *MemLocation = _Add(OpSize::i64Bit, Mem, _Constant(Size * 2));
    auto NewFTW = _LoadMem(GPRClass, Size, MemLocation, Size);
    _StoreContext(2, GPRClass, NewFTW, offsetof(FEXCore::Core::CPUState, FTW));
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
    //Convert to double precision
    Reg = _F80CVT(8, Reg);
    _StoreContextIndexed(Reg, Top, 8, MMBaseOffset(), 16, FPRClass);

    ST0Location = _Add(OpSize::i64Bit, ST0Location, TenConst);
    Top = _And(OpSize::i64Bit, _Add(OpSize::i64Bit, Top, OneConst), SevenConst);
  }

  // The final st(7) needs a bit of special handling here
  // ST7 broken in to two parts
  // Lower 64bits [63:0]
  // upper 16 bits [79:64]

  OrderedNode *Reg = _LoadMem(FPRClass, 8, ST0Location, 1);
  ST0Location = _Add(OpSize::i64Bit, ST0Location, _Constant(8));
  OrderedNode *RegHigh = _LoadMem(FPRClass, 2, ST0Location, 1);
  Reg = _VInsElement(16, 2, 4, 0, Reg, RegHigh);
  Reg = _F80CVT(8, Reg); //Convert to double precision
  _StoreContextIndexed(Reg, Top, 8, MMBaseOffset(), 16, FPRClass);
}


//FXAM needs change
void OpDispatchBuilder::X87FXAMF64(OpcodeArgs) {
  auto top = GetX87Top();
  auto a = _LoadContextIndexed(top, 8, MMBaseOffset(), 16, FPRClass);
  OrderedNode *Result = _VExtractToGPR(8, 8, a, 0);

  // Extract the sign bit
  Result = _Bfe(OpSize::i64Bit, 1, 63, Result);
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


}
