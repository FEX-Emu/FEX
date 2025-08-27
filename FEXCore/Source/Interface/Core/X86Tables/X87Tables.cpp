// SPDX-License-Identifier: MIT
/*
$info$
tags: frontend|x86-tables
$end_info$
*/

#include "Interface/Core/X86Tables/X86Tables.h"
#include "Interface/Core/OpcodeDispatcher.h"

#include <iterator>

namespace FEXCore::X86Tables {
using namespace InstFlags;
using namespace IR;
// Top bit indicating if it needs to be repeated with {0x40, 0x80} or'd in
// All OPDReg versions need it
#define OPDReg(op, reg) ((1 << 15) | ((op - 0xD8) << 8) | (reg << 3))
#define OPD(op, modrmop) (((op - 0xD8) << 8) | modrmop)
constexpr std::array<DispatchTableEntry, 133> X87F64OpTable = {{
  {OPDReg(0xD8, 0) | 0x00, 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FADDF64, OpSize::i32Bit, false, OpDispatchBuilder::OpResult::RES_ST0>},

  {OPDReg(0xD8, 1) | 0x00, 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FMULF64, OpSize::i32Bit, false, OpDispatchBuilder::OpResult::RES_ST0>},

  {OPDReg(0xD8, 2) | 0x00, 8,
   &OpDispatchBuilder::Bind<&OpDispatchBuilder::FCOMIF64, OpSize::i32Bit, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>},

  {OPDReg(0xD8, 3) | 0x00, 8,
   &OpDispatchBuilder::Bind<&OpDispatchBuilder::FCOMIF64, OpSize::i32Bit, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>},

  {OPDReg(0xD8, 4) | 0x00, 8,
   &OpDispatchBuilder::Bind<&OpDispatchBuilder::FSUBF64, OpSize::i32Bit, false, false, OpDispatchBuilder::OpResult::RES_ST0>},

  {OPDReg(0xD8, 5) | 0x00, 8,
   &OpDispatchBuilder::Bind<&OpDispatchBuilder::FSUBF64, OpSize::i32Bit, false, true, OpDispatchBuilder::OpResult::RES_ST0>},

  {OPDReg(0xD8, 6) | 0x00, 8,
   &OpDispatchBuilder::Bind<&OpDispatchBuilder::FDIVF64, OpSize::i32Bit, false, false, OpDispatchBuilder::OpResult::RES_ST0>},

  {OPDReg(0xD8, 7) | 0x00, 8,
   &OpDispatchBuilder::Bind<&OpDispatchBuilder::FDIVF64, OpSize::i32Bit, false, true, OpDispatchBuilder::OpResult::RES_ST0>},

  {OPD(0xD8, 0xC0), 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FADDF64, OpSize::f80Bit, false, OpDispatchBuilder::OpResult::RES_ST0>},
  {OPD(0xD8, 0xC8), 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FMULF64, OpSize::f80Bit, false, OpDispatchBuilder::OpResult::RES_ST0>},
  {OPD(0xD8, 0xD0), 8,
   &OpDispatchBuilder::Bind<&OpDispatchBuilder::FCOMIF64, OpSize::f80Bit, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>},
  {OPD(0xD8, 0xD8), 8,
   &OpDispatchBuilder::Bind<&OpDispatchBuilder::FCOMIF64, OpSize::f80Bit, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>},
  {OPD(0xD8, 0xE0), 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FSUBF64, OpSize::f80Bit, false, false, OpDispatchBuilder::OpResult::RES_ST0>},
  {OPD(0xD8, 0xE8), 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FSUBF64, OpSize::f80Bit, false, true, OpDispatchBuilder::OpResult::RES_ST0>},
  {OPD(0xD8, 0xF0), 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FDIVF64, OpSize::f80Bit, false, false, OpDispatchBuilder::OpResult::RES_ST0>},
  {OPD(0xD8, 0xF8), 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FDIVF64, OpSize::f80Bit, false, true, OpDispatchBuilder::OpResult::RES_ST0>},

  {OPDReg(0xD9, 0) | 0x00, 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FLDF64, OpSize::i32Bit>},

  // 1 = Invalid

  {OPDReg(0xD9, 2) | 0x00, 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FST, OpSize::i32Bit>},

  {OPDReg(0xD9, 3) | 0x00, 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FST, OpSize::i32Bit>},

  {OPDReg(0xD9, 4) | 0x00, 8, &OpDispatchBuilder::X87LDENVF64},

  {OPDReg(0xD9, 5) | 0x00, 8, &OpDispatchBuilder::X87FLDCWF64},

  {OPDReg(0xD9, 6) | 0x00, 8, &OpDispatchBuilder::X87FNSTENV},

  {OPDReg(0xD9, 7) | 0x00, 8, &OpDispatchBuilder::X87FSTCW},

  {OPD(0xD9, 0xC0), 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FLDFromStack>},
  {OPD(0xD9, 0xC8), 8, &OpDispatchBuilder::FXCH},
  {OPD(0xD9, 0xD0), 1, &OpDispatchBuilder::NOPOp}, // FNOP
  // D1 = Invalid
  // D8 = Invalid
  {OPD(0xD9, 0xE0), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::X87OpHelper, OP_F80STACKCHANGESIGN, false>},
  {OPD(0xD9, 0xE1), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::X87OpHelper, OP_F80STACKABS, false>},
  // E2 = Invalid
  {OPD(0xD9, 0xE4), 1, &OpDispatchBuilder::FTSTF64},
  {OPD(0xD9, 0xE5), 1, &OpDispatchBuilder::X87FXAM},
  // E6 = Invalid
  {OPD(0xD9, 0xE8), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FLDF64_Const, 0x3FF0000000000000>}, // 1.0
  {OPD(0xD9, 0xE9), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FLDF64_Const, 0x400A934F0979A372>}, // log2l(10)
  {OPD(0xD9, 0xEA), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FLDF64_Const, 0x3FF71547652B82FE>}, // log2l(e)
  {OPD(0xD9, 0xEB), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FLDF64_Const, 0x400921FB54442D18>}, // pi
  {OPD(0xD9, 0xEC), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FLDF64_Const, 0x3FD34413509F79FF>}, // log10l(2)
  {OPD(0xD9, 0xED), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FLDF64_Const, 0x3FE62E42FEFA39EF>}, // log(2)
  {OPD(0xD9, 0xEE), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FLDF64_Const, 0>},                  // 0.0

  // EF = Invalid
  {OPD(0xD9, 0xF0), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::X87OpHelper, OP_F80F2XM1STACK, false>},
  {OPD(0xD9, 0xF1), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::X87FYL2X, false>},
  {OPD(0xD9, 0xF2), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::X87OpHelper, OP_F80PTANSTACK, true>},
  {OPD(0xD9, 0xF3), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::X87OpHelper, OP_F80ATANSTACK, false>},
  {OPD(0xD9, 0xF4), 1, &OpDispatchBuilder::X87FXTRACTF64},
  {OPD(0xD9, 0xF5), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::X87OpHelper, OP_F80FPREM1STACK, true>},
  {OPD(0xD9, 0xF6), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::X87ModifySTP, false>},
  {OPD(0xD9, 0xF7), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::X87ModifySTP, true>},
  {OPD(0xD9, 0xF8), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::X87OpHelper, OP_F80FPREMSTACK, true>},
  {OPD(0xD9, 0xF9), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::X87FYL2X, true>},
  {OPD(0xD9, 0xFA), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::X87OpHelper, OP_F80SQRTSTACK, false>},
  {OPD(0xD9, 0xFB), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::X87OpHelper, OP_F80SINCOSSTACK, true>},
  {OPD(0xD9, 0xFC), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::X87OpHelper, OP_F80ROUNDSTACK, false>},
  {OPD(0xD9, 0xFD), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::X87OpHelper, OP_F80SCALESTACK, false>},
  {OPD(0xD9, 0xFE), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::X87OpHelper, OP_F80SINSTACK, true>},
  {OPD(0xD9, 0xFF), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::X87OpHelper, OP_F80COSSTACK, true>},

  {OPDReg(0xDA, 0) | 0x00, 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FADDF64, OpSize::i32Bit, true, OpDispatchBuilder::OpResult::RES_ST0>},

  {OPDReg(0xDA, 1) | 0x00, 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FMULF64, OpSize::i32Bit, true, OpDispatchBuilder::OpResult::RES_ST0>},

  {OPDReg(0xDA, 2) | 0x00, 8,
   &OpDispatchBuilder::Bind<&OpDispatchBuilder::FCOMIF64, OpSize::i32Bit, true, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>},

  {OPDReg(0xDA, 3) | 0x00, 8,
   &OpDispatchBuilder::Bind<&OpDispatchBuilder::FCOMIF64, OpSize::i32Bit, true, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>},

  {OPDReg(0xDA, 4) | 0x00, 8,
   &OpDispatchBuilder::Bind<&OpDispatchBuilder::FSUBF64, OpSize::i32Bit, true, false, OpDispatchBuilder::OpResult::RES_ST0>},

  {OPDReg(0xDA, 5) | 0x00, 8,
   &OpDispatchBuilder::Bind<&OpDispatchBuilder::FSUBF64, OpSize::i32Bit, true, true, OpDispatchBuilder::OpResult::RES_ST0>},

  {OPDReg(0xDA, 6) | 0x00, 8,
   &OpDispatchBuilder::Bind<&OpDispatchBuilder::FDIVF64, OpSize::i32Bit, true, false, OpDispatchBuilder::OpResult::RES_ST0>},

  {OPDReg(0xDA, 7) | 0x00, 8,
   &OpDispatchBuilder::Bind<&OpDispatchBuilder::FDIVF64, OpSize::i32Bit, true, true, OpDispatchBuilder::OpResult::RES_ST0>},

  {OPD(0xDA, 0xC0), 8, &OpDispatchBuilder::X87FCMOV},
  {OPD(0xDA, 0xC8), 8, &OpDispatchBuilder::X87FCMOV},
  {OPD(0xDA, 0xD0), 8, &OpDispatchBuilder::X87FCMOV},
  {OPD(0xDA, 0xD8), 8, &OpDispatchBuilder::X87FCMOV},
  // E0 = Invalid
  // E8 = Invalid
  {OPD(0xDA, 0xE9), 1,
   &OpDispatchBuilder::Bind<&OpDispatchBuilder::FCOMIF64, OpSize::f80Bit, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, true>},
  // EA = Invalid
  // F0 = Invalid
  // F8 = Invalid

  {OPDReg(0xDB, 0) | 0x00, 8, &OpDispatchBuilder::FILDF64},

  {OPDReg(0xDB, 1) | 0x00, 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FISTF64, true>},

  {OPDReg(0xDB, 2) | 0x00, 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FISTF64, false>},

  {OPDReg(0xDB, 3) | 0x00, 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FISTF64, false>},

  // 4 = Invalid

  {OPDReg(0xDB, 5) | 0x00, 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FLDF64, OpSize::f80Bit>},

  // 6 = Invalid

  {OPDReg(0xDB, 7) | 0x00, 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FST, OpSize::f80Bit>},


  {OPD(0xDB, 0xC0), 8, &OpDispatchBuilder::X87FCMOV},
  {OPD(0xDB, 0xC8), 8, &OpDispatchBuilder::X87FCMOV},
  {OPD(0xDB, 0xD0), 8, &OpDispatchBuilder::X87FCMOV},
  {OPD(0xDB, 0xD8), 8, &OpDispatchBuilder::X87FCMOV},
  // E0 = Invalid
  {OPD(0xDB, 0xE2), 1, &OpDispatchBuilder::FNCLEX},
  {OPD(0xDB, 0xE3), 1, &OpDispatchBuilder::FNINIT},
  // E4 = Invalid
  {OPD(0xDB, 0xE8), 8,
   &OpDispatchBuilder::Bind<&OpDispatchBuilder::FCOMIF64, OpSize::f80Bit, false, OpDispatchBuilder::FCOMIFlags::FLAGS_RFLAGS, false>},
  {OPD(0xDB, 0xF0), 8,
   &OpDispatchBuilder::Bind<&OpDispatchBuilder::FCOMIF64, OpSize::f80Bit, false, OpDispatchBuilder::FCOMIFlags::FLAGS_RFLAGS, false>},

  // F8 = Invalid

  {OPDReg(0xDC, 0) | 0x00, 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FADDF64, OpSize::i64Bit, false, OpDispatchBuilder::OpResult::RES_ST0>},

  {OPDReg(0xDC, 1) | 0x00, 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FMULF64, OpSize::i64Bit, false, OpDispatchBuilder::OpResult::RES_ST0>},

  {OPDReg(0xDC, 2) | 0x00, 8,
   &OpDispatchBuilder::Bind<&OpDispatchBuilder::FCOMIF64, OpSize::i64Bit, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>},

  {OPDReg(0xDC, 3) | 0x00, 8,
   &OpDispatchBuilder::Bind<&OpDispatchBuilder::FCOMIF64, OpSize::i64Bit, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>},

  {OPDReg(0xDC, 4) | 0x00, 8,
   &OpDispatchBuilder::Bind<&OpDispatchBuilder::FSUBF64, OpSize::i64Bit, false, false, OpDispatchBuilder::OpResult::RES_ST0>},

  {OPDReg(0xDC, 5) | 0x00, 8,
   &OpDispatchBuilder::Bind<&OpDispatchBuilder::FSUBF64, OpSize::i64Bit, false, true, OpDispatchBuilder::OpResult::RES_ST0>},

  {OPDReg(0xDC, 6) | 0x00, 8,
   &OpDispatchBuilder::Bind<&OpDispatchBuilder::FDIVF64, OpSize::i64Bit, false, false, OpDispatchBuilder::OpResult::RES_ST0>},

  {OPDReg(0xDC, 7) | 0x00, 8,
   &OpDispatchBuilder::Bind<&OpDispatchBuilder::FDIVF64, OpSize::i64Bit, false, true, OpDispatchBuilder::OpResult::RES_ST0>},

  {OPD(0xDC, 0xC0), 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FADDF64, OpSize::f80Bit, false, OpDispatchBuilder::OpResult::RES_STI>},
  {OPD(0xDC, 0xC8), 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FMULF64, OpSize::f80Bit, false, OpDispatchBuilder::OpResult::RES_STI>},
  {OPD(0xDC, 0xE0), 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FSUBF64, OpSize::f80Bit, false, true, OpDispatchBuilder::OpResult::RES_STI>},
  {OPD(0xDC, 0xE8), 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FSUBF64, OpSize::f80Bit, false, false, OpDispatchBuilder::OpResult::RES_STI>},
  {OPD(0xDC, 0xF0), 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FDIVF64, OpSize::f80Bit, false, true, OpDispatchBuilder::OpResult::RES_STI>},
  {OPD(0xDC, 0xF8), 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FDIVF64, OpSize::f80Bit, false, false, OpDispatchBuilder::OpResult::RES_STI>},

  {OPDReg(0xDD, 0) | 0x00, 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FLDF64, OpSize::i64Bit>},

  {OPDReg(0xDD, 1) | 0x00, 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FISTF64, true>},

  {OPDReg(0xDD, 2) | 0x00, 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FST, OpSize::i64Bit>},

  {OPDReg(0xDD, 3) | 0x00, 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FST, OpSize::i64Bit>},

  {OPDReg(0xDD, 4) | 0x00, 8, &OpDispatchBuilder::X87FRSTOR},

  // 5 = Invalid
  {OPDReg(0xDD, 6) | 0x00, 8, &OpDispatchBuilder::X87FNSAVE},

  {OPDReg(0xDD, 7) | 0x00, 8, &OpDispatchBuilder::X87FNSTSW},

  {OPD(0xDD, 0xC0), 8, &OpDispatchBuilder::X87FFREE},
  {OPD(0xDD, 0xD0), 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FSTToStack>}, // register-register from regular X87
  {OPD(0xDD, 0xD8), 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FSTToStack>}, //^

  {OPD(0xDD, 0xE0), 8,
   &OpDispatchBuilder::Bind<&OpDispatchBuilder::FCOMIF64, OpSize::f80Bit, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>},
  {OPD(0xDD, 0xE8), 8,
   &OpDispatchBuilder::Bind<&OpDispatchBuilder::FCOMIF64, OpSize::f80Bit, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>},

  {OPDReg(0xDE, 0) | 0x00, 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FADDF64, OpSize::i16Bit, true, OpDispatchBuilder::OpResult::RES_ST0>},

  {OPDReg(0xDE, 1) | 0x00, 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FMULF64, OpSize::i16Bit, true, OpDispatchBuilder::OpResult::RES_ST0>},

  {OPDReg(0xDE, 2) | 0x00, 8,
   &OpDispatchBuilder::Bind<&OpDispatchBuilder::FCOMIF64, OpSize::i16Bit, true, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>},

  {OPDReg(0xDE, 3) | 0x00, 8,
   &OpDispatchBuilder::Bind<&OpDispatchBuilder::FCOMIF64, OpSize::i16Bit, true, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>},

  {OPDReg(0xDE, 4) | 0x00, 8,
   &OpDispatchBuilder::Bind<&OpDispatchBuilder::FSUBF64, OpSize::i16Bit, true, false, OpDispatchBuilder::OpResult::RES_ST0>},

  {OPDReg(0xDE, 5) | 0x00, 8,
   &OpDispatchBuilder::Bind<&OpDispatchBuilder::FSUBF64, OpSize::i16Bit, true, true, OpDispatchBuilder::OpResult::RES_ST0>},

  {OPDReg(0xDE, 6) | 0x00, 8,
   &OpDispatchBuilder::Bind<&OpDispatchBuilder::FDIVF64, OpSize::i16Bit, true, false, OpDispatchBuilder::OpResult::RES_ST0>},

  {OPDReg(0xDE, 7) | 0x00, 8,
   &OpDispatchBuilder::Bind<&OpDispatchBuilder::FDIVF64, OpSize::i16Bit, true, true, OpDispatchBuilder::OpResult::RES_ST0>},

  {OPD(0xDE, 0xC0), 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FADDF64, OpSize::f80Bit, false, OpDispatchBuilder::OpResult::RES_STI>},
  {OPD(0xDE, 0xC8), 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FMULF64, OpSize::f80Bit, false, OpDispatchBuilder::OpResult::RES_STI>},
  {OPD(0xDE, 0xD9), 1,
   &OpDispatchBuilder::Bind<&OpDispatchBuilder::FCOMIF64, OpSize::f80Bit, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, true>},
  {OPD(0xDE, 0xE0), 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FSUBF64, OpSize::f80Bit, false, true, OpDispatchBuilder::OpResult::RES_STI>},
  {OPD(0xDE, 0xE8), 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FSUBF64, OpSize::f80Bit, false, false, OpDispatchBuilder::OpResult::RES_STI>},
  {OPD(0xDE, 0xF0), 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FDIVF64, OpSize::f80Bit, false, true, OpDispatchBuilder::OpResult::RES_STI>},
  {OPD(0xDE, 0xF8), 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FDIVF64, OpSize::f80Bit, false, false, OpDispatchBuilder::OpResult::RES_STI>},

  {OPDReg(0xDF, 0) | 0x00, 8, &OpDispatchBuilder::FILDF64},

  {OPDReg(0xDF, 1) | 0x00, 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FISTF64, true>},

  {OPDReg(0xDF, 2) | 0x00, 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FISTF64, false>},

  {OPDReg(0xDF, 3) | 0x00, 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FISTF64, false>},

  {OPDReg(0xDF, 4) | 0x00, 8, &OpDispatchBuilder::FBLDF64},

  {OPDReg(0xDF, 5) | 0x00, 8, &OpDispatchBuilder::FILDF64},

  {OPDReg(0xDF, 6) | 0x00, 8, &OpDispatchBuilder::FBSTPF64},

  {OPDReg(0xDF, 7) | 0x00, 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FISTF64, false>},

  // XXX: This should also set the x87 tag bits to empty
  // We don't support this currently, so just pop the stack
  {OPD(0xDF, 0xC0), 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::X87ModifySTP, true>},

  {OPD(0xDF, 0xE0), 8, &OpDispatchBuilder::X87FNSTSW},
  {OPD(0xDF, 0xE8), 8,
   &OpDispatchBuilder::Bind<&OpDispatchBuilder::FCOMIF64, OpSize::f80Bit, false, OpDispatchBuilder::FCOMIFlags::FLAGS_RFLAGS, false>},
  {OPD(0xDF, 0xF0), 8,
   &OpDispatchBuilder::Bind<&OpDispatchBuilder::FCOMIF64, OpSize::f80Bit, false, OpDispatchBuilder::FCOMIFlags::FLAGS_RFLAGS, false>},
}};

constexpr std::array<DispatchTableEntry, 133> X87F80OpTable = {{
  {OPDReg(0xD8, 0) | 0x00, 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FADD, OpSize::i32Bit, false, OpDispatchBuilder::OpResult::RES_ST0>},

  {OPDReg(0xD8, 1) | 0x00, 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FMUL, OpSize::i32Bit, false, OpDispatchBuilder::OpResult::RES_ST0>},

  {OPDReg(0xD8, 2) | 0x00, 8,
   &OpDispatchBuilder::Bind<&OpDispatchBuilder::FCOMI, OpSize::i32Bit, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>},

  {OPDReg(0xD8, 3) | 0x00, 8,
   &OpDispatchBuilder::Bind<&OpDispatchBuilder::FCOMI, OpSize::i32Bit, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>},

  {OPDReg(0xD8, 4) | 0x00, 8,
   &OpDispatchBuilder::Bind<&OpDispatchBuilder::FSUB, OpSize::i32Bit, false, false, OpDispatchBuilder::OpResult::RES_ST0>},

  {OPDReg(0xD8, 5) | 0x00, 8,
   &OpDispatchBuilder::Bind<&OpDispatchBuilder::FSUB, OpSize::i32Bit, false, true, OpDispatchBuilder::OpResult::RES_ST0>},

  {OPDReg(0xD8, 6) | 0x00, 8,
   &OpDispatchBuilder::Bind<&OpDispatchBuilder::FDIV, OpSize::i32Bit, false, false, OpDispatchBuilder::OpResult::RES_ST0>},

  {OPDReg(0xD8, 7) | 0x00, 8,
   &OpDispatchBuilder::Bind<&OpDispatchBuilder::FDIV, OpSize::i32Bit, false, true, OpDispatchBuilder::OpResult::RES_ST0>},

  {OPD(0xD8, 0xC0), 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FADD, OpSize::f80Bit, false, OpDispatchBuilder::OpResult::RES_ST0>},
  {OPD(0xD8, 0xC8), 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FMUL, OpSize::f80Bit, false, OpDispatchBuilder::OpResult::RES_ST0>},
  {OPD(0xD8, 0xD0), 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FCOMI, OpSize::f80Bit, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>},
  {OPD(0xD8, 0xD8), 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FCOMI, OpSize::f80Bit, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>},
  {OPD(0xD8, 0xE0), 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FSUB, OpSize::f80Bit, false, false, OpDispatchBuilder::OpResult::RES_ST0>},
  {OPD(0xD8, 0xE8), 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FSUB, OpSize::f80Bit, false, true, OpDispatchBuilder::OpResult::RES_ST0>},
  {OPD(0xD8, 0xF0), 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FDIV, OpSize::f80Bit, false, false, OpDispatchBuilder::OpResult::RES_ST0>},
  {OPD(0xD8, 0xF8), 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FDIV, OpSize::f80Bit, false, true, OpDispatchBuilder::OpResult::RES_ST0>},

  {OPDReg(0xD9, 0) | 0x00, 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FLD, OpSize::i32Bit>},

  // 1 = Invalid

  {OPDReg(0xD9, 2) | 0x00, 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FST, OpSize::i32Bit>},

  {OPDReg(0xD9, 3) | 0x00, 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FST, OpSize::i32Bit>},

  {OPDReg(0xD9, 4) | 0x00, 8, &OpDispatchBuilder::X87LDENV},

  {OPDReg(0xD9, 5) | 0x00, 8, &OpDispatchBuilder::X87FLDCW}, // XXX: stubbed FLDCW

  {OPDReg(0xD9, 6) | 0x00, 8, &OpDispatchBuilder::X87FNSTENV},

  {OPDReg(0xD9, 7) | 0x00, 8, &OpDispatchBuilder::X87FSTCW},

  {OPD(0xD9, 0xC0), 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FLDFromStack>},
  {OPD(0xD9, 0xC8), 8, &OpDispatchBuilder::FXCH},
  {OPD(0xD9, 0xD0), 1, &OpDispatchBuilder::NOPOp}, // FNOP
  // D1 = Invalid
  // D8 = Invalid
  {OPD(0xD9, 0xE0), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::X87OpHelper, OP_F80STACKCHANGESIGN, false>},
  {OPD(0xD9, 0xE1), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::X87OpHelper, OP_F80STACKABS, false>},
  // E2 = Invalid
  {OPD(0xD9, 0xE4), 1, &OpDispatchBuilder::FTST},
  {OPD(0xD9, 0xE5), 1, &OpDispatchBuilder::X87FXAM},
  // E6 = Invalid
  {OPD(0xD9, 0xE8), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FLD_Const, NamedVectorConstant::NAMED_VECTOR_X87_ONE>},     // 1.0
  {OPD(0xD9, 0xE9), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FLD_Const, NamedVectorConstant::NAMED_VECTOR_X87_LOG2_10>}, // log2l(10)
  {OPD(0xD9, 0xEA), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FLD_Const, NamedVectorConstant::NAMED_VECTOR_X87_LOG2_E>}, // log2l(e)
  {OPD(0xD9, 0xEB), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FLD_Const, NamedVectorConstant::NAMED_VECTOR_X87_PI>},     // pi
  {OPD(0xD9, 0xEC), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FLD_Const, NamedVectorConstant::NAMED_VECTOR_X87_LOG10_2>}, // log10l(2)
  {OPD(0xD9, 0xED), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FLD_Const, NamedVectorConstant::NAMED_VECTOR_X87_LOG_2>},   // log(2)
  {OPD(0xD9, 0xEE), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FLD_Const, NamedVectorConstant::NAMED_VECTOR_ZERO>},        // 0.0

  // EF = Invalid
  {OPD(0xD9, 0xF0), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::X87OpHelper, OP_F80F2XM1STACK, false>},
  {OPD(0xD9, 0xF1), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::X87FYL2X, false>},
  {OPD(0xD9, 0xF2), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::X87OpHelper, OP_F80PTANSTACK, true>},
  {OPD(0xD9, 0xF3), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::X87OpHelper, OP_F80ATANSTACK, false>},
  {OPD(0xD9, 0xF4), 1, &OpDispatchBuilder::X87FXTRACT},
  {OPD(0xD9, 0xF5), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::X87OpHelper, OP_F80FPREM1STACK, true>},
  {OPD(0xD9, 0xF6), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::X87ModifySTP, false>},
  {OPD(0xD9, 0xF7), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::X87ModifySTP, true>},
  {OPD(0xD9, 0xF8), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::X87OpHelper, OP_F80FPREMSTACK, true>},
  {OPD(0xD9, 0xF9), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::X87FYL2X, true>},
  {OPD(0xD9, 0xFA), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::X87OpHelper, OP_F80SQRTSTACK, false>},
  {OPD(0xD9, 0xFB), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::X87OpHelper, OP_F80SINCOSSTACK, true>},
  {OPD(0xD9, 0xFC), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::X87OpHelper, OP_F80ROUNDSTACK, false>},
  {OPD(0xD9, 0xFD), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::X87OpHelper, OP_F80SCALESTACK, false>},
  {OPD(0xD9, 0xFE), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::X87OpHelper, OP_F80SINSTACK, true>},
  {OPD(0xD9, 0xFF), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::X87OpHelper, OP_F80COSSTACK, true>},

  {OPDReg(0xDA, 0) | 0x00, 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FADD, OpSize::i32Bit, true, OpDispatchBuilder::OpResult::RES_ST0>},

  {OPDReg(0xDA, 1) | 0x00, 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FMUL, OpSize::i32Bit, true, OpDispatchBuilder::OpResult::RES_ST0>},

  {OPDReg(0xDA, 2) | 0x00, 8,
   &OpDispatchBuilder::Bind<&OpDispatchBuilder::FCOMI, OpSize::i32Bit, true, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>},

  {OPDReg(0xDA, 3) | 0x00, 8,
   &OpDispatchBuilder::Bind<&OpDispatchBuilder::FCOMI, OpSize::i32Bit, true, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>},

  {OPDReg(0xDA, 4) | 0x00, 8,
   &OpDispatchBuilder::Bind<&OpDispatchBuilder::FSUB, OpSize::i32Bit, true, false, OpDispatchBuilder::OpResult::RES_ST0>},

  {OPDReg(0xDA, 5) | 0x00, 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FSUB, OpSize::i32Bit, true, true, OpDispatchBuilder::OpResult::RES_ST0>},

  {OPDReg(0xDA, 6) | 0x00, 8,
   &OpDispatchBuilder::Bind<&OpDispatchBuilder::FDIV, OpSize::i32Bit, true, false, OpDispatchBuilder::OpResult::RES_ST0>},

  {OPDReg(0xDA, 7) | 0x00, 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FDIV, OpSize::i32Bit, true, true, OpDispatchBuilder::OpResult::RES_ST0>},

  {OPD(0xDA, 0xC0), 8, &OpDispatchBuilder::X87FCMOV},
  {OPD(0xDA, 0xC8), 8, &OpDispatchBuilder::X87FCMOV},
  {OPD(0xDA, 0xD0), 8, &OpDispatchBuilder::X87FCMOV},
  {OPD(0xDA, 0xD8), 8, &OpDispatchBuilder::X87FCMOV},
  // E0 = Invalid
  // E8 = Invalid
  {OPD(0xDA, 0xE9), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FCOMI, OpSize::f80Bit, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, true>},
  // EA = Invalid
  // F0 = Invalid
  // F8 = Invalid

  {OPDReg(0xDB, 0) | 0x00, 8, &OpDispatchBuilder::FILD},

  {OPDReg(0xDB, 1) | 0x00, 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FIST, true>},

  {OPDReg(0xDB, 2) | 0x00, 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FIST, false>},

  {OPDReg(0xDB, 3) | 0x00, 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FIST, false>},

  // 4 = Invalid

  {OPDReg(0xDB, 5) | 0x00, 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FLD, OpSize::f80Bit>},

  // 6 = Invalid

  {OPDReg(0xDB, 7) | 0x00, 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FST, OpSize::f80Bit>},


  {OPD(0xDB, 0xC0), 8, &OpDispatchBuilder::X87FCMOV},
  {OPD(0xDB, 0xC8), 8, &OpDispatchBuilder::X87FCMOV},
  {OPD(0xDB, 0xD0), 8, &OpDispatchBuilder::X87FCMOV},
  {OPD(0xDB, 0xD8), 8, &OpDispatchBuilder::X87FCMOV},
  // E0 = Invalid
  {OPD(0xDB, 0xE2), 1, &OpDispatchBuilder::FNCLEX},
  {OPD(0xDB, 0xE3), 1, &OpDispatchBuilder::FNINIT},
  // E4 = Invalid
  {OPD(0xDB, 0xE8), 8,
   &OpDispatchBuilder::Bind<&OpDispatchBuilder::FCOMI, OpSize::f80Bit, false, OpDispatchBuilder::FCOMIFlags::FLAGS_RFLAGS, false>},
  {OPD(0xDB, 0xF0), 8,
   &OpDispatchBuilder::Bind<&OpDispatchBuilder::FCOMI, OpSize::f80Bit, false, OpDispatchBuilder::FCOMIFlags::FLAGS_RFLAGS, false>},

  // F8 = Invalid

  {OPDReg(0xDC, 0) | 0x00, 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FADD, OpSize::i64Bit, false, OpDispatchBuilder::OpResult::RES_ST0>},

  {OPDReg(0xDC, 1) | 0x00, 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FMUL, OpSize::i64Bit, false, OpDispatchBuilder::OpResult::RES_ST0>},

  {OPDReg(0xDC, 2) | 0x00, 8,
   &OpDispatchBuilder::Bind<&OpDispatchBuilder::FCOMI, OpSize::i64Bit, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>},

  {OPDReg(0xDC, 3) | 0x00, 8,
   &OpDispatchBuilder::Bind<&OpDispatchBuilder::FCOMI, OpSize::i64Bit, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>},

  {OPDReg(0xDC, 4) | 0x00, 8,
   &OpDispatchBuilder::Bind<&OpDispatchBuilder::FSUB, OpSize::i64Bit, false, false, OpDispatchBuilder::OpResult::RES_ST0>},

  {OPDReg(0xDC, 5) | 0x00, 8,
   &OpDispatchBuilder::Bind<&OpDispatchBuilder::FSUB, OpSize::i64Bit, false, true, OpDispatchBuilder::OpResult::RES_ST0>},

  {OPDReg(0xDC, 6) | 0x00, 8,
   &OpDispatchBuilder::Bind<&OpDispatchBuilder::FDIV, OpSize::i64Bit, false, false, OpDispatchBuilder::OpResult::RES_ST0>},

  {OPDReg(0xDC, 7) | 0x00, 8,
   &OpDispatchBuilder::Bind<&OpDispatchBuilder::FDIV, OpSize::i64Bit, false, true, OpDispatchBuilder::OpResult::RES_ST0>},

  {OPD(0xDC, 0xC0), 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FADD, OpSize::f80Bit, false, OpDispatchBuilder::OpResult::RES_STI>},
  {OPD(0xDC, 0xC8), 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FMUL, OpSize::f80Bit, false, OpDispatchBuilder::OpResult::RES_STI>},
  {OPD(0xDC, 0xE0), 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FSUB, OpSize::f80Bit, false, true, OpDispatchBuilder::OpResult::RES_STI>},
  {OPD(0xDC, 0xE8), 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FSUB, OpSize::f80Bit, false, false, OpDispatchBuilder::OpResult::RES_STI>},
  {OPD(0xDC, 0xF0), 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FDIV, OpSize::f80Bit, false, true, OpDispatchBuilder::OpResult::RES_STI>},
  {OPD(0xDC, 0xF8), 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FDIV, OpSize::f80Bit, false, false, OpDispatchBuilder::OpResult::RES_STI>},

  {OPDReg(0xDD, 0) | 0x00, 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FLD, OpSize::i64Bit>},

  {OPDReg(0xDD, 1) | 0x00, 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FIST, true>},

  {OPDReg(0xDD, 2) | 0x00, 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FST, OpSize::i64Bit>},

  {OPDReg(0xDD, 3) | 0x00, 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FST, OpSize::i64Bit>},

  {OPDReg(0xDD, 4) | 0x00, 8, &OpDispatchBuilder::X87FRSTOR},

  // 5 = Invalid
  {OPDReg(0xDD, 6) | 0x00, 8, &OpDispatchBuilder::X87FNSAVE},

  {OPDReg(0xDD, 7) | 0x00, 8, &OpDispatchBuilder::X87FNSTSW},

  {OPD(0xDD, 0xC0), 8, &OpDispatchBuilder::X87FFREE},
  {OPD(0xDD, 0xD0), 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FSTToStack>},
  {OPD(0xDD, 0xD8), 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FSTToStack>},

  {OPD(0xDD, 0xE0), 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FCOMI, OpSize::f80Bit, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>},
  {OPD(0xDD, 0xE8), 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FCOMI, OpSize::f80Bit, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>},

  {OPDReg(0xDE, 0) | 0x00, 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FADD, OpSize::i16Bit, true, OpDispatchBuilder::OpResult::RES_ST0>},

  {OPDReg(0xDE, 1) | 0x00, 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FMUL, OpSize::i16Bit, true, OpDispatchBuilder::OpResult::RES_ST0>},

  {OPDReg(0xDE, 2) | 0x00, 8,
   &OpDispatchBuilder::Bind<&OpDispatchBuilder::FCOMI, OpSize::i16Bit, true, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>},

  {OPDReg(0xDE, 3) | 0x00, 8,
   &OpDispatchBuilder::Bind<&OpDispatchBuilder::FCOMI, OpSize::i16Bit, true, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, false>},

  {OPDReg(0xDE, 4) | 0x00, 8,
   &OpDispatchBuilder::Bind<&OpDispatchBuilder::FSUB, OpSize::i16Bit, true, false, OpDispatchBuilder::OpResult::RES_ST0>},

  {OPDReg(0xDE, 5) | 0x00, 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FSUB, OpSize::i16Bit, true, true, OpDispatchBuilder::OpResult::RES_ST0>},

  {OPDReg(0xDE, 6) | 0x00, 8,
   &OpDispatchBuilder::Bind<&OpDispatchBuilder::FDIV, OpSize::i16Bit, true, false, OpDispatchBuilder::OpResult::RES_ST0>},

  {OPDReg(0xDE, 7) | 0x00, 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FDIV, OpSize::i16Bit, true, true, OpDispatchBuilder::OpResult::RES_ST0>},

  {OPD(0xDE, 0xC0), 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FADD, OpSize::f80Bit, false, OpDispatchBuilder::OpResult::RES_STI>},
  {OPD(0xDE, 0xC8), 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FMUL, OpSize::f80Bit, false, OpDispatchBuilder::OpResult::RES_STI>},
  {OPD(0xDE, 0xD9), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FCOMI, OpSize::f80Bit, false, OpDispatchBuilder::FCOMIFlags::FLAGS_X87, true>},
  {OPD(0xDE, 0xE0), 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FSUB, OpSize::f80Bit, false, true, OpDispatchBuilder::OpResult::RES_STI>},
  {OPD(0xDE, 0xE8), 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FSUB, OpSize::f80Bit, false, false, OpDispatchBuilder::OpResult::RES_STI>},
  {OPD(0xDE, 0xF0), 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FDIV, OpSize::f80Bit, false, true, OpDispatchBuilder::OpResult::RES_STI>},
  {OPD(0xDE, 0xF8), 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FDIV, OpSize::f80Bit, false, false, OpDispatchBuilder::OpResult::RES_STI>},

  {OPDReg(0xDF, 0) | 0x00, 8, &OpDispatchBuilder::FILD},

  {OPDReg(0xDF, 1) | 0x00, 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FIST, true>},

  {OPDReg(0xDF, 2) | 0x00, 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FIST, false>},

  {OPDReg(0xDF, 3) | 0x00, 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FIST, false>},

  {OPDReg(0xDF, 4) | 0x00, 8, &OpDispatchBuilder::FBLD},

  {OPDReg(0xDF, 5) | 0x00, 8, &OpDispatchBuilder::FILD},

  {OPDReg(0xDF, 6) | 0x00, 8, &OpDispatchBuilder::FBSTP},

  {OPDReg(0xDF, 7) | 0x00, 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::FIST, false>},

  // XXX: This should also set the x87 tag bits to empty
  // We don't support this currently, so just pop the stack
  {OPD(0xDF, 0xC0), 8, &OpDispatchBuilder::Bind<&OpDispatchBuilder::X87ModifySTP, true>},

  {OPD(0xDF, 0xE0), 8, &OpDispatchBuilder::X87FNSTSW},
  {OPD(0xDF, 0xE8), 8,
   &OpDispatchBuilder::Bind<&OpDispatchBuilder::FCOMI, OpSize::f80Bit, false, OpDispatchBuilder::FCOMIFlags::FLAGS_RFLAGS, false>},
  {OPD(0xDF, 0xF0), 8,
   &OpDispatchBuilder::Bind<&OpDispatchBuilder::FCOMI, OpSize::f80Bit, false, OpDispatchBuilder::FCOMIFlags::FLAGS_RFLAGS, false>},
}};
#undef OPD
#undef OPDReg

auto GenerateX87TableLambda = [](const auto DispatchTable) consteval {
#define OPD(op, modrmop) (((op - 0xD8) << 8) | modrmop)
#define OPDReg(op, reg) (((op - 0xD8) << 8) | (reg << 3))
 std::array<X86InstInfo, MAX_X87_TABLE_SIZE> Table{};
  constexpr U16U8InfoStruct X87OpTable[] = {
    // 0xD8
    {OPDReg(0xD8, 0), 1, X86InstInfo{"FADD",  TYPE_X87, FLAGS_MODRM, 0}},
    {OPDReg(0xD8, 1), 1, X86InstInfo{"FMUL",  TYPE_X87, FLAGS_MODRM, 0}},
    {OPDReg(0xD8, 2), 1, X86InstInfo{"FCOM",  TYPE_X87, FLAGS_MODRM, 0}},
    {OPDReg(0xD8, 3), 1, X86InstInfo{"FCOMP", TYPE_X87, FLAGS_MODRM | FLAGS_POP, 0}},
    {OPDReg(0xD8, 4), 1, X86InstInfo{"FSUB",  TYPE_X87, FLAGS_MODRM, 0}},
    {OPDReg(0xD8, 5), 1, X86InstInfo{"FSUBR", TYPE_X87, FLAGS_MODRM, 0}},
    {OPDReg(0xD8, 6), 1, X86InstInfo{"FDIV",  TYPE_X87, FLAGS_MODRM, 0}},
    {OPDReg(0xD8, 7), 1, X86InstInfo{"FDIVR", TYPE_X87, FLAGS_MODRM, 0}},
      //  / 0
      {OPD(0xD8, 0xC0), 8, X86InstInfo{"FADD", TYPE_X87, FLAGS_NONE, 0}},
      //  / 1
      {OPD(0xD8, 0xC8), 8, X86InstInfo{"FMUL", TYPE_X87, FLAGS_NONE, 0}},
      //  / 2
      {OPD(0xD8, 0xD0), 8, X86InstInfo{"FCOM", TYPE_X87, FLAGS_NONE, 0}},
      //  / 3
      {OPD(0xD8, 0xD8), 8, X86InstInfo{"FCOMP", TYPE_X87, FLAGS_POP, 0}},
      //  / 4
      {OPD(0xD8, 0xE0), 8, X86InstInfo{"FSUB", TYPE_X87, FLAGS_NONE, 0}},
      //  / 5
      {OPD(0xD8, 0xE8), 8, X86InstInfo{"FSUBR", TYPE_X87, FLAGS_NONE, 0}},
      //  / 6
      {OPD(0xD8, 0xF0), 8, X86InstInfo{"FDIV", TYPE_X87, FLAGS_NONE, 0}},
      //  / 7
      {OPD(0xD8, 0xF8), 8, X86InstInfo{"FDIVR", TYPE_X87, FLAGS_NONE, 0}},
    // 0xD9
    {OPDReg(0xD9, 0), 1, X86InstInfo{"FLD",     TYPE_INST, FLAGS_MODRM, 0}},
    {OPDReg(0xD9, 1), 1, X86InstInfo{"",        TYPE_INVALID, FLAGS_NONE, 0}},
    {OPDReg(0xD9, 2), 1, X86InstInfo{"FST",     TYPE_X87, FLAGS_MODRM | FLAGS_SF_MOD_DST, 0}},
    {OPDReg(0xD9, 3), 1, X86InstInfo{"FSTP",    TYPE_X87, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_POP, 0}},
    {OPDReg(0xD9, 4), 1, X86InstInfo{"FLDENV",  TYPE_X87, FLAGS_MODRM, 0}},
    {OPDReg(0xD9, 5), 1, X86InstInfo{"FLDCW",   TYPE_X87, GenFlagsSameSize(SIZE_16BIT) | FLAGS_MODRM, 0}},
    {OPDReg(0xD9, 6), 1, X86InstInfo{"FNSTENV", TYPE_X87, FLAGS_MODRM | FLAGS_SF_MOD_DST, 0}},
    {OPDReg(0xD9, 7), 1, X86InstInfo{"FNSTCW",  TYPE_INST, GenFlagsSameSize(SIZE_16BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST, 0}},
      //  / 0
      {OPD(0xD9, 0xC0), 8, X86InstInfo{"FLD",   TYPE_INST, FLAGS_NONE, 0}},
      //  / 1
      {OPD(0xD9, 0xC8), 8, X86InstInfo{"FXCH",  TYPE_X87, FLAGS_NONE, 0}},
      //  / 2
      {OPD(0xD9, 0xD0), 1, X86InstInfo{"FNOP",  TYPE_X87, FLAGS_NONE, 0}},
      {OPD(0xD9, 0xD1), 7, X86InstInfo{"",      TYPE_INVALID, FLAGS_NONE, 0}},
      //  / 3
      {OPD(0xD9, 0xD8), 8, X86InstInfo{"",      TYPE_INVALID, FLAGS_NONE, 0}},
      //  / 4
      {OPD(0xD9, 0xE0), 1, X86InstInfo{"FCHS", TYPE_X87, FLAGS_NONE, 0}},
      {OPD(0xD9, 0xE1), 1, X86InstInfo{"FABS", TYPE_X87, FLAGS_NONE, 0}},
      {OPD(0xD9, 0xE2), 2, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE, 0}},
      {OPD(0xD9, 0xE4), 1, X86InstInfo{"FTST", TYPE_X87, FLAGS_NONE, 0}},
      {OPD(0xD9, 0xE5), 1, X86InstInfo{"FXAM", TYPE_INST,  FLAGS_NONE, 0}},
      {OPD(0xD9, 0xE6), 2, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE, 0}},
      //  / 5
      {OPD(0xD9, 0xE8), 1, X86InstInfo{"FLD1", TYPE_X87, FLAGS_NONE, 0}},
      {OPD(0xD9, 0xE9), 1, X86InstInfo{"FLDL2T", TYPE_X87, FLAGS_NONE, 0}},
      {OPD(0xD9, 0xEA), 1, X86InstInfo{"FLDL2E", TYPE_X87, FLAGS_NONE, 0}},
      {OPD(0xD9, 0xEB), 1, X86InstInfo{"FLDPI", TYPE_X87, FLAGS_NONE, 0}},
      {OPD(0xD9, 0xEC), 1, X86InstInfo{"FLDLG2", TYPE_X87, FLAGS_NONE, 0}},
      {OPD(0xD9, 0xED), 1, X86InstInfo{"FLDLN2", TYPE_X87, FLAGS_NONE, 0}},
      {OPD(0xD9, 0xEE), 1, X86InstInfo{"FLDZ", TYPE_X87, FLAGS_NONE, 0}},
      {OPD(0xD9, 0xEF), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE, 0}},
      //  / 6
      {OPD(0xD9, 0xF0), 1, X86InstInfo{"F2XM1", TYPE_X87, FLAGS_NONE, 0}},
      {OPD(0xD9, 0xF1), 1, X86InstInfo{"FYL2X", TYPE_X87, FLAGS_NONE, 0}},
      {OPD(0xD9, 0xF2), 1, X86InstInfo{"FPTAN", TYPE_X87, FLAGS_NONE, 0}},
      {OPD(0xD9, 0xF3), 1, X86InstInfo{"FPATAN", TYPE_X87, FLAGS_NONE, 0}},
      {OPD(0xD9, 0xF4), 1, X86InstInfo{"FXTRACT", TYPE_X87, FLAGS_NONE, 0}},
      {OPD(0xD9, 0xF5), 1, X86InstInfo{"FPREM1", TYPE_X87, FLAGS_NONE, 0}},
      {OPD(0xD9, 0xF6), 1, X86InstInfo{"FDECSTP", TYPE_X87, FLAGS_POP, 0}},
      {OPD(0xD9, 0xF7), 1, X86InstInfo{"FINCSTP", TYPE_X87, FLAGS_POP, 0}},
      //  / 7
      {OPD(0xD9, 0xF8), 1, X86InstInfo{"FPREM", TYPE_X87, FLAGS_NONE, 0}},
      {OPD(0xD9, 0xF9), 1, X86InstInfo{"FYL2XP1", TYPE_X87, FLAGS_NONE, 0}},
      {OPD(0xD9, 0xFA), 1, X86InstInfo{"FSQRT", TYPE_X87, FLAGS_NONE, 0}},
      {OPD(0xD9, 0xFB), 1, X86InstInfo{"FSINCOS", TYPE_X87, FLAGS_NONE, 0}},
      {OPD(0xD9, 0xFC), 1, X86InstInfo{"FRNDINT", TYPE_X87, FLAGS_NONE, 0}},
      {OPD(0xD9, 0xFD), 1, X86InstInfo{"FSCALE", TYPE_X87, FLAGS_NONE, 0}},
      {OPD(0xD9, 0xFE), 1, X86InstInfo{"FSIN", TYPE_X87, FLAGS_NONE, 0}},
      {OPD(0xD9, 0xFF), 1, X86InstInfo{"FCOS", TYPE_X87, FLAGS_NONE, 0}},
    // 0xDA
    {OPDReg(0xDA, 0), 1, X86InstInfo{"FIADD", TYPE_X87,  GenFlagsSrcSize(SIZE_32BIT) | FLAGS_MODRM, 0}},
    {OPDReg(0xDA, 1), 1, X86InstInfo{"FIMUL", TYPE_X87,  GenFlagsSrcSize(SIZE_32BIT) | FLAGS_MODRM, 0}},
    {OPDReg(0xDA, 2), 1, X86InstInfo{"FICOM", TYPE_X87,  GenFlagsSrcSize(SIZE_32BIT) | FLAGS_MODRM, 0}},
    {OPDReg(0xDA, 3), 1, X86InstInfo{"FICOMP", TYPE_X87, GenFlagsSrcSize(SIZE_32BIT) | FLAGS_MODRM | FLAGS_POP, 0}},
    {OPDReg(0xDA, 4), 1, X86InstInfo{"FISUB", TYPE_X87,  GenFlagsSrcSize(SIZE_32BIT) | FLAGS_MODRM, 0}},
    {OPDReg(0xDA, 5), 1, X86InstInfo{"FISUBR", TYPE_X87, GenFlagsSrcSize(SIZE_32BIT) | FLAGS_MODRM, 0}},
    {OPDReg(0xDA, 6), 1, X86InstInfo{"FIDIV", TYPE_X87,  GenFlagsSrcSize(SIZE_32BIT) | FLAGS_MODRM, 0}},
    {OPDReg(0xDA, 7), 1, X86InstInfo{"FIDIVR", TYPE_X87, GenFlagsSrcSize(SIZE_32BIT) | FLAGS_MODRM, 0}},
      //  / 0
      {OPD(0xDA, 0xC0), 8, X86InstInfo{"FCMOVB", TYPE_X87, FLAGS_NONE, 0}},
      //  / 1
      {OPD(0xDA, 0xC8), 8, X86InstInfo{"FCMOVE", TYPE_X87, FLAGS_NONE, 0}},
      //  / 2
      {OPD(0xDA, 0xD0), 8, X86InstInfo{"FCMOVBE", TYPE_X87, FLAGS_NONE, 0}},
      //  / 3
      {OPD(0xDA, 0xD8), 8, X86InstInfo{"FCMOVU", TYPE_X87, FLAGS_NONE, 0}},
      //  / 4
      {OPD(0xDA, 0xE0), 8, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE, 0}},
      //  / 5
      {OPD(0xDA, 0xE8), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE, 0}},
      {OPD(0xDA, 0xE9), 1, X86InstInfo{"FUCOMPP", TYPE_X87, FLAGS_POP, 0}},
      {OPD(0xDA, 0xEA), 6, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE, 0}},
      //  / 6
      {OPD(0xDA, 0xF0), 8, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE, 0}},
      //  / 7
      {OPD(0xDA, 0xF8), 8, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE, 0}},
    // 0xDB
    {OPDReg(0xDB, 0), 1, X86InstInfo{"FILD",   TYPE_X87, GenFlagsSrcSize(SIZE_32BIT) | FLAGS_MODRM, 0}},
    {OPDReg(0xDB, 1), 1, X86InstInfo{"FISTTP", TYPE_X87, GenFlagsSrcSize(SIZE_32BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_POP, 0}},
    {OPDReg(0xDB, 2), 1, X86InstInfo{"FIST",   TYPE_X87, GenFlagsSrcSize(SIZE_32BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST, 0}},
    {OPDReg(0xDB, 3), 1, X86InstInfo{"FISTP",  TYPE_X87, GenFlagsSrcSize(SIZE_32BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_POP, 0}},
    {OPDReg(0xDB, 4), 1, X86InstInfo{"",       TYPE_INVALID, FLAGS_NONE, 0}},
    {OPDReg(0xDB, 5), 1, X86InstInfo{"FLD",    TYPE_X87,    FLAGS_MODRM, 0}},
    {OPDReg(0xDB, 6), 1, X86InstInfo{"",       TYPE_INVALID, FLAGS_NONE, 0}},
    {OPDReg(0xDB, 7), 1, X86InstInfo{"FSTP",   TYPE_X87,   FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_POP, 0}},
      //  / 0
      {OPD(0xDB, 0xC0), 8, X86InstInfo{"FCMOVNB", TYPE_X87, FLAGS_NONE, 0}},
      //  / 1
      {OPD(0xDB, 0xC8), 8, X86InstInfo{"FCMOVNE", TYPE_X87, FLAGS_NONE, 0}},
      //  / 2
      {OPD(0xDB, 0xD0), 8, X86InstInfo{"FCMOVNBE", TYPE_X87, FLAGS_NONE, 0}},
      //  / 3
      {OPD(0xDB, 0xD8), 8, X86InstInfo{"FCMOVNU", TYPE_X87, FLAGS_NONE, 0}},
      //  / 4
      {OPD(0xDB, 0xE0), 2, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE, 0}},
      {OPD(0xDB, 0xE2), 1, X86InstInfo{"FNCLEX", TYPE_X87, FLAGS_NONE, 0}},
      {OPD(0xDB, 0xE3), 1, X86InstInfo{"FNINIT", TYPE_X87, FLAGS_NONE, 0}},
      {OPD(0xDB, 0xE4), 4, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE, 0}},
      //  / 5
      {OPD(0xDB, 0xE8), 8, X86InstInfo{"FUCOMI", TYPE_INST, FLAGS_NONE, 0}},
      //  / 6
      {OPD(0xDB, 0xF0), 8, X86InstInfo{"FCOMI", TYPE_X87, FLAGS_NONE, 0}},
      //  / 7
      {OPD(0xDB, 0xF8), 8, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE, 0}},
    // 0xDC
    {OPDReg(0xDC, 0), 1, X86InstInfo{"FADD", TYPE_X87, GenFlagsSrcSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_X87_FLAGS, 0}},
    {OPDReg(0xDC, 1), 1, X86InstInfo{"FMUL", TYPE_X87, GenFlagsSrcSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_X87_FLAGS, 0}},
    {OPDReg(0xDC, 2), 1, X86InstInfo{"FCOM", TYPE_X87, GenFlagsSrcSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_X87_FLAGS, 0}},
    {OPDReg(0xDC, 3), 1, X86InstInfo{"FCOMP", TYPE_X87, GenFlagsSrcSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_X87_FLAGS | FLAGS_POP, 0}},
    {OPDReg(0xDC, 4), 1, X86InstInfo{"FSUB", TYPE_X87, GenFlagsSrcSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_X87_FLAGS, 0}},
    {OPDReg(0xDC, 5), 1, X86InstInfo{"FSUBR", TYPE_X87, GenFlagsSrcSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_X87_FLAGS, 0}},
    {OPDReg(0xDC, 6), 1, X86InstInfo{"FDIV", TYPE_X87, GenFlagsSrcSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_X87_FLAGS, 0}},
    {OPDReg(0xDC, 7), 1, X86InstInfo{"FDIVR", TYPE_X87, GenFlagsSrcSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_X87_FLAGS, 0}},
      //  / 0
      {OPD(0xDC, 0xC0), 8, X86InstInfo{"FADD", TYPE_X87, FLAGS_NONE, 0}},
      //  / 1
      {OPD(0xDC, 0xC8), 8, X86InstInfo{"FMUL", TYPE_X87, FLAGS_NONE, 0}},
      //  / 2
      {OPD(0xDC, 0xD0), 8, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE, 0}},
      //  / 3
      {OPD(0xDC, 0xD8), 8, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE, 0}},
      //  / 4
      {OPD(0xDC, 0xE0), 8, X86InstInfo{"FSUBR", TYPE_X87, FLAGS_NONE, 0}},
      //  / 5
      {OPD(0xDC, 0xE8), 8, X86InstInfo{"FSUB", TYPE_X87, FLAGS_NONE, 0}},
      //  / 6
      {OPD(0xDC, 0xF0), 8, X86InstInfo{"FDIVR", TYPE_X87, FLAGS_NONE, 0}},
      //  / 7
      {OPD(0xDC, 0xF8), 8, X86InstInfo{"FDIV", TYPE_X87, FLAGS_NONE, 0}},
    // 0xDD
    {OPDReg(0xDD, 0), 1, X86InstInfo{"FLD", TYPE_X87, FLAGS_MODRM, 0}},
    {OPDReg(0xDD, 1), 1, X86InstInfo{"FISTTP", TYPE_X87, GenFlagsSrcSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_X87_FLAGS | FLAGS_SF_MOD_DST | FLAGS_POP, 0}},
    {OPDReg(0xDD, 2), 1, X86InstInfo{"FST", TYPE_X87, FLAGS_MODRM | FLAGS_SF_MOD_DST, 0}},
    {OPDReg(0xDD, 3), 1, X86InstInfo{"FSTP", TYPE_X87, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_POP, 0}},
    {OPDReg(0xDD, 4), 1, X86InstInfo{"FRSTOR", TYPE_X87, FLAGS_MODRM, 0}},
    {OPDReg(0xDD, 5), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE, 0}},
    {OPDReg(0xDD, 6), 1, X86InstInfo{"FNSAVE", TYPE_X87, FLAGS_MODRM | FLAGS_SF_MOD_DST, 0}},
    {OPDReg(0xDD, 7), 1, X86InstInfo{"FNSTSW", TYPE_X87, GenFlagsSameSize(SIZE_16BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST, 0}},
      //  / 0
      {OPD(0xDD, 0xC0), 8, X86InstInfo{"FFREE", TYPE_X87, FLAGS_NONE, 0}},
      //  / 1
      {OPD(0xDD, 0xC8), 8, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE, 0}},
      //  / 2
      {OPD(0xDD, 0xD0), 8, X86InstInfo{"FST", TYPE_INST, FLAGS_SF_MOD_DST, 0}},
      //  / 3
      {OPD(0xDD, 0xD8), 8, X86InstInfo{"FSTP", TYPE_X87, FLAGS_SF_MOD_DST | FLAGS_POP, 0}},
      //  / 4
      {OPD(0xDD, 0xE0), 8, X86InstInfo{"FUCOM", TYPE_X87, FLAGS_NONE, 0}},
      //  / 5
      {OPD(0xDD, 0xE8), 8, X86InstInfo{"FUCOMP", TYPE_X87, FLAGS_POP, 0}},
      //  / 6
      {OPD(0xDD, 0xF0), 8, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE, 0}},
      //  / 7
      {OPD(0xDD, 0xF8), 8, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE, 0}},
    // 0xDE
    {OPDReg(0xDE, 0), 1, X86InstInfo{"FIADD", TYPE_X87,  GenFlagsSrcSize(SIZE_16BIT) | FLAGS_MODRM, 0}},
    {OPDReg(0xDE, 1), 1, X86InstInfo{"FIMUL", TYPE_X87,  GenFlagsSrcSize(SIZE_16BIT) | FLAGS_MODRM, 0}},
    {OPDReg(0xDE, 2), 1, X86InstInfo{"FICOM", TYPE_X87,  GenFlagsSrcSize(SIZE_16BIT) | FLAGS_MODRM, 0}},
    {OPDReg(0xDE, 3), 1, X86InstInfo{"FICOMP", TYPE_X87, GenFlagsSrcSize(SIZE_16BIT) | FLAGS_MODRM | FLAGS_POP, 0}},
    {OPDReg(0xDE, 4), 1, X86InstInfo{"FISUB", TYPE_X87,  GenFlagsSrcSize(SIZE_16BIT) | FLAGS_MODRM, 0}},
    {OPDReg(0xDE, 5), 1, X86InstInfo{"FISUBR", TYPE_X87, GenFlagsSrcSize(SIZE_16BIT) | FLAGS_MODRM, 0}},
    {OPDReg(0xDE, 6), 1, X86InstInfo{"FIDIV", TYPE_X87,  GenFlagsSrcSize(SIZE_16BIT) | FLAGS_MODRM, 0}},
    {OPDReg(0xDE, 7), 1, X86InstInfo{"FIDIVR", TYPE_X87, GenFlagsSrcSize(SIZE_16BIT) | FLAGS_MODRM, 0}},
      //  / 0
      {OPD(0xDE, 0xC0), 8, X86InstInfo{"FADDP", TYPE_X87, FLAGS_POP, 0}},
      //  / 1
      {OPD(0xDE, 0xC8), 8, X86InstInfo{"FMULP", TYPE_X87, FLAGS_POP, 0}},
      //  / 2
      {OPD(0xDE, 0xD0), 8, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE, 0}},
      //  / 3
      {OPD(0xDE, 0xD8), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE, 0}},
      {OPD(0xDE, 0xD9), 1, X86InstInfo{"FCOMPP", TYPE_X87, FLAGS_POP, 0}},
      {OPD(0xDE, 0xDA), 6, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE, 0}},
      //  / 4
      {OPD(0xDE, 0xE0), 8, X86InstInfo{"FSUBRP", TYPE_X87, FLAGS_POP, 0}},
      //  / 5
      {OPD(0xDE, 0xE8), 8, X86InstInfo{"FSUBP", TYPE_X87, FLAGS_POP, 0}},
      //  / 6
      {OPD(0xDE, 0xF0), 8, X86InstInfo{"FDIVRP", TYPE_X87, FLAGS_POP, 0}},
      //  / 7
      {OPD(0xDE, 0xF8), 8, X86InstInfo{"FDIVP", TYPE_X87, FLAGS_POP, 0}},
    // 0xDF
    {OPDReg(0xDF, 0), 1, X86InstInfo{"FILD", TYPE_X87, GenFlagsSrcSize(SIZE_16BIT) | FLAGS_MODRM, 0}},
    {OPDReg(0xDF, 1), 1, X86InstInfo{"FISTTP", TYPE_X87, GenFlagsSrcSize(SIZE_16BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_POP, 0}},
    {OPDReg(0xDF, 2), 1, X86InstInfo{"FIST",   TYPE_X87, GenFlagsSrcSize(SIZE_16BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST, 0}},
    {OPDReg(0xDF, 3), 1, X86InstInfo{"FISTP",  TYPE_X87, GenFlagsSrcSize(SIZE_16BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_POP, 0}},
    {OPDReg(0xDF, 4), 1, X86InstInfo{"FBLD", TYPE_X87, FLAGS_MODRM, 0}},
    {OPDReg(0xDF, 5), 1, X86InstInfo{"FILD", TYPE_X87, GenFlagsSrcSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_X87_FLAGS, 0}},
    {OPDReg(0xDF, 6), 1, X86InstInfo{"FBSTP", TYPE_X87, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_POP, 0}},
    {OPDReg(0xDF, 7), 1, X86InstInfo{"FISTP", TYPE_X87, GenFlagsSrcSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_X87_FLAGS | FLAGS_SF_MOD_DST | FLAGS_POP, 0}},
      //  / 0
      //  This instruction is a bit special. This is an undocumented(Almost) x87 instruction.
      //  https://en.wikipedia.org/wiki/X86_instruction_listings#Undocumented_x87_instructions
      //  https://www.pagetable.com/?p=16
      //  AMD Athlon Processor x86 Code Optimization Guide - `Use FFREEP Macro to Pop One Register from the FPU Stack`
      //  ISA architecture manuals don't talk about this instruction at all
      //  At some point the Nvidia OpenGL binary driver uses this instruction.
      //  GCC may also end up emitting this instruction in some rare edge case!
      //  Almost all x86 CPUs implement this, and it is expected to be around
      {OPD(0xDF, 0xC0), 8, X86InstInfo{"FFREEP",  TYPE_X87, FLAGS_POP, 0}},
      //  / 1
      {OPD(0xDF, 0xC8), 8, X86InstInfo{"",        TYPE_INVALID, FLAGS_NONE, 0}},
      //  / 2
      {OPD(0xDF, 0xD0), 8, X86InstInfo{"",        TYPE_INVALID, FLAGS_NONE, 0}},
      //  / 3
      {OPD(0xDF, 0xD8), 8, X86InstInfo{"",        TYPE_INVALID, FLAGS_NONE, 0}},
      //  / 4
      {OPD(0xDF, 0xE0), 1, X86InstInfo{"FNSTSW",  TYPE_INST, GenFlagsSameSize(SIZE_16BIT) | FLAGS_SF_DST_RAX, 0}},
      {OPD(0xDF, 0xE1), 7, X86InstInfo{"",        TYPE_INVALID, FLAGS_NONE, 0}},
      //  / 5
      {OPD(0xDF, 0xE8), 8, X86InstInfo{"FUCOMIP", TYPE_INST,    FLAGS_POP, 0}},
      //  / 6
      {OPD(0xDF, 0xF0), 8, X86InstInfo{"FCOMIP",  TYPE_X87,   FLAGS_POP, 0}},
      //  / 7
      {OPD(0xDF, 0xF8), 8, X86InstInfo{"",        TYPE_INVALID, FLAGS_NONE, 0}},
  };
#undef OPD
#undef OPDReg

  auto InstallToX87Table = [](auto& FinalTable, auto& LocalTable) {
    for (auto Op : LocalTable) {
      auto OpNum = Op.Op;
      bool Repeat = (OpNum & 0x8000) != 0;
      OpNum = OpNum & 0x7FF;
      auto Dispatcher = Op.Ptr;
      for (uint8_t i = 0; i < Op.Count; ++i) {
        LOGMAN_THROW_A_FMT(FinalTable[OpNum + i].OpcodeDispatcher.OpDispatch == nullptr, "Duplicate Entry");

        FinalTable[OpNum + i].OpcodeDispatcher.OpDispatch = Dispatcher;

        // Flag to indicate if we need to repeat this op in {0x40, 0x80} ranges
        if (Repeat) {
          FinalTable[(OpNum | 0x40) + i].OpcodeDispatcher.OpDispatch = Dispatcher;
          FinalTable[(OpNum | 0x80) + i].OpcodeDispatcher.OpDispatch = Dispatcher;
        }
      }
    }
  };

  GenerateX87Table(&Table.at(0), X87OpTable, std::size(X87OpTable));
  InstallToX87Table(Table, DispatchTable);
  return Table;
};

constexpr std::array<X86InstInfo, MAX_X87_TABLE_SIZE> X87F80Ops = GenerateX87TableLambda(X87F80OpTable);
constexpr std::array<X86InstInfo, MAX_X87_TABLE_SIZE> X87F64Ops = GenerateX87TableLambda(X87F64OpTable);

}
