// SPDX-License-Identifier: MIT
/*
$info$
tags: frontend|x86-tables
$end_info$
*/

#include "Interface/Core/X86Tables/X86Tables.h"
#include "Interface/Core/OpcodeDispatcher/VEXTables.h"

#include <iterator>

namespace FEXCore::X86Tables {
using namespace InstFlags;

namespace AVX128 {
  using namespace IR;
#define OPD(map_select, pp, opcode) (((map_select - 1) << 10) | (pp << 8) | (opcode))
  constexpr DispatchTableEntry BaseTable[] = {
    {OPD(1, 0b00, 0x10), 1, &OpDispatchBuilder::AVX128_VMOVAPS},
    {OPD(1, 0b01, 0x10), 1, &OpDispatchBuilder::AVX128_VMOVAPS},
    {OPD(1, 0b10, 0x10), 1, &OpDispatchBuilder::AVX128_VMOVSS},
    {OPD(1, 0b11, 0x10), 1, &OpDispatchBuilder::AVX128_VMOVSD},
    {OPD(1, 0b00, 0x11), 1, &OpDispatchBuilder::AVX128_VMOVAPS},
    {OPD(1, 0b01, 0x11), 1, &OpDispatchBuilder::AVX128_VMOVAPS},
    {OPD(1, 0b10, 0x11), 1, &OpDispatchBuilder::AVX128_VMOVSS},
    {OPD(1, 0b11, 0x11), 1, &OpDispatchBuilder::AVX128_VMOVSD},

    {OPD(1, 0b00, 0x12), 1, &OpDispatchBuilder::AVX128_VMOVLP},
    {OPD(1, 0b01, 0x12), 1, &OpDispatchBuilder::AVX128_VMOVLP},
    {OPD(1, 0b10, 0x12), 1, &OpDispatchBuilder::AVX128_VMOVSLDUP},
    {OPD(1, 0b11, 0x12), 1, &OpDispatchBuilder::AVX128_VMOVDDUP},
    {OPD(1, 0b00, 0x13), 1, &OpDispatchBuilder::AVX128_VMOVLP},
    {OPD(1, 0b01, 0x13), 1, &OpDispatchBuilder::AVX128_VMOVLP},

    {OPD(1, 0b00, 0x14), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VPUNPCKL, OpSize::i32Bit>},
    {OPD(1, 0b01, 0x14), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VPUNPCKL, OpSize::i64Bit>},

    {OPD(1, 0b00, 0x15), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VPUNPCKH, OpSize::i32Bit>},
    {OPD(1, 0b01, 0x15), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VPUNPCKH, OpSize::i64Bit>},

    {OPD(1, 0b00, 0x16), 1, &OpDispatchBuilder::AVX128_VMOVHP},
    {OPD(1, 0b01, 0x16), 1, &OpDispatchBuilder::AVX128_VMOVHP},
    {OPD(1, 0b10, 0x16), 1, &OpDispatchBuilder::AVX128_VMOVSHDUP},
    {OPD(1, 0b00, 0x17), 1, &OpDispatchBuilder::AVX128_VMOVHP},
    {OPD(1, 0b01, 0x17), 1, &OpDispatchBuilder::AVX128_VMOVHP},

    {OPD(1, 0b00, 0x28), 1, &OpDispatchBuilder::AVX128_VMOVAPS},
    {OPD(1, 0b01, 0x28), 1, &OpDispatchBuilder::AVX128_VMOVAPS},
    {OPD(1, 0b00, 0x29), 1, &OpDispatchBuilder::AVX128_VMOVAPS},
    {OPD(1, 0b01, 0x29), 1, &OpDispatchBuilder::AVX128_VMOVAPS},

    {OPD(1, 0b10, 0x2A), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_InsertCVTGPR_To_FPR, OpSize::i32Bit>},
    {OPD(1, 0b11, 0x2A), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_InsertCVTGPR_To_FPR, OpSize::i64Bit>},

    {OPD(1, 0b00, 0x2B), 1, &OpDispatchBuilder::AVX128_MOVVectorNT},
    {OPD(1, 0b01, 0x2B), 1, &OpDispatchBuilder::AVX128_MOVVectorNT},

    {OPD(1, 0b10, 0x2C), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_CVTFPR_To_GPR, OpSize::i32Bit, false>},
    {OPD(1, 0b11, 0x2C), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_CVTFPR_To_GPR, OpSize::i64Bit, false>},

    {OPD(1, 0b10, 0x2D), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_CVTFPR_To_GPR, OpSize::i32Bit, true>},
    {OPD(1, 0b11, 0x2D), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_CVTFPR_To_GPR, OpSize::i64Bit, true>},

    {OPD(1, 0b00, 0x2E), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_UCOMISx, OpSize::i32Bit>},
    {OPD(1, 0b01, 0x2E), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_UCOMISx, OpSize::i64Bit>},
    {OPD(1, 0b00, 0x2F), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_UCOMISx, OpSize::i32Bit>},
    {OPD(1, 0b01, 0x2F), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_UCOMISx, OpSize::i64Bit>},

    {OPD(1, 0b00, 0x50), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_MOVMSK, OpSize::i32Bit>},
    {OPD(1, 0b01, 0x50), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_MOVMSK, OpSize::i64Bit>},

    {OPD(1, 0b00, 0x51), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorUnary, IR::OP_VFSQRT, OpSize::i32Bit>},
    {OPD(1, 0b01, 0x51), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorUnary, IR::OP_VFSQRT, OpSize::i64Bit>},
    {OPD(1, 0b10, 0x51), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorScalarInsertALU, IR::OP_VFSQRTSCALARINSERT, OpSize::i32Bit>},
    {OPD(1, 0b11, 0x51), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorScalarInsertALU, IR::OP_VFSQRTSCALARINSERT, OpSize::i64Bit>},

    {OPD(1, 0b00, 0x52), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorUnary, IR::OP_VFRSQRT, OpSize::i32Bit>},
    {OPD(1, 0b10, 0x52), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorScalarInsertALU, IR::OP_VFRSQRTSCALARINSERT, OpSize::i32Bit>},

    {OPD(1, 0b00, 0x53), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorUnary, IR::OP_VFRECP, OpSize::i32Bit>},
    {OPD(1, 0b10, 0x53), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorScalarInsertALU, IR::OP_VFRECPSCALARINSERT, OpSize::i32Bit>},

    {OPD(1, 0b00, 0x54), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VAND, OpSize::i128Bit>},
    {OPD(1, 0b01, 0x54), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VAND, OpSize::i128Bit>},

    {OPD(1, 0b00, 0x55), 1, &OpDispatchBuilder::AVX128_VANDN},
    {OPD(1, 0b01, 0x55), 1, &OpDispatchBuilder::AVX128_VANDN},

    {OPD(1, 0b00, 0x56), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VOR, OpSize::i128Bit>},
    {OPD(1, 0b01, 0x56), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VOR, OpSize::i128Bit>},

    {OPD(1, 0b00, 0x57), 1, &OpDispatchBuilder::AVX128_VectorXOR},
    {OPD(1, 0b01, 0x57), 1, &OpDispatchBuilder::AVX128_VectorXOR},

    {OPD(1, 0b00, 0x58), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VFADD, OpSize::i32Bit>},
    {OPD(1, 0b01, 0x58), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VFADD, OpSize::i64Bit>},
    {OPD(1, 0b10, 0x58), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorScalarInsertALU, IR::OP_VFADDSCALARINSERT, OpSize::i32Bit>},
    {OPD(1, 0b11, 0x58), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorScalarInsertALU, IR::OP_VFADDSCALARINSERT, OpSize::i64Bit>},

    {OPD(1, 0b00, 0x59), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VFMUL, OpSize::i32Bit>},
    {OPD(1, 0b01, 0x59), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VFMUL, OpSize::i64Bit>},
    {OPD(1, 0b10, 0x59), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorScalarInsertALU, IR::OP_VFMULSCALARINSERT, OpSize::i32Bit>},
    {OPD(1, 0b11, 0x59), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorScalarInsertALU, IR::OP_VFMULSCALARINSERT, OpSize::i64Bit>},

    {OPD(1, 0b00, 0x5A), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_Vector_CVT_Float_To_Float, OpSize::i64Bit, OpSize::i32Bit>},
    {OPD(1, 0b01, 0x5A), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_Vector_CVT_Float_To_Float, OpSize::i32Bit, OpSize::i64Bit>},
    {OPD(1, 0b10, 0x5A), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_InsertScalar_CVT_Float_To_Float, OpSize::i64Bit, OpSize::i32Bit>},
    {OPD(1, 0b11, 0x5A), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_InsertScalar_CVT_Float_To_Float, OpSize::i32Bit, OpSize::i64Bit>},

    {OPD(1, 0b00, 0x5B), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_Vector_CVT_Int_To_Float, OpSize::i32Bit, false>},
    {OPD(1, 0b01, 0x5B), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_Vector_CVT_Float_To_Int, OpSize::i32Bit, true>},
    {OPD(1, 0b10, 0x5B), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_Vector_CVT_Float_To_Int, OpSize::i32Bit, false>},

    {OPD(1, 0b00, 0x5C), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VFSUB, OpSize::i32Bit>},
    {OPD(1, 0b01, 0x5C), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VFSUB, OpSize::i64Bit>},
    {OPD(1, 0b10, 0x5C), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorScalarInsertALU, IR::OP_VFSUBSCALARINSERT, OpSize::i32Bit>},
    {OPD(1, 0b11, 0x5C), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorScalarInsertALU, IR::OP_VFSUBSCALARINSERT, OpSize::i64Bit>},

    {OPD(1, 0b00, 0x5D), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VFMIN, OpSize::i32Bit>},
    {OPD(1, 0b01, 0x5D), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VFMIN, OpSize::i64Bit>},
    {OPD(1, 0b10, 0x5D), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorScalarInsertALU, IR::OP_VFMINSCALARINSERT, OpSize::i32Bit>},
    {OPD(1, 0b11, 0x5D), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorScalarInsertALU, IR::OP_VFMINSCALARINSERT, OpSize::i64Bit>},

    {OPD(1, 0b00, 0x5E), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VFDIV, OpSize::i32Bit>},
    {OPD(1, 0b01, 0x5E), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VFDIV, OpSize::i64Bit>},
    {OPD(1, 0b10, 0x5E), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorScalarInsertALU, IR::OP_VFDIVSCALARINSERT, OpSize::i32Bit>},
    {OPD(1, 0b11, 0x5E), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorScalarInsertALU, IR::OP_VFDIVSCALARINSERT, OpSize::i64Bit>},

    {OPD(1, 0b00, 0x5F), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VFMAX, OpSize::i32Bit>},
    {OPD(1, 0b01, 0x5F), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VFMAX, OpSize::i64Bit>},
    {OPD(1, 0b10, 0x5F), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorScalarInsertALU, IR::OP_VFMAXSCALARINSERT, OpSize::i32Bit>},
    {OPD(1, 0b11, 0x5F), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorScalarInsertALU, IR::OP_VFMAXSCALARINSERT, OpSize::i64Bit>},

    {OPD(1, 0b01, 0x60), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VPUNPCKL, OpSize::i8Bit>},
    {OPD(1, 0b01, 0x61), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VPUNPCKL, OpSize::i16Bit>},
    {OPD(1, 0b01, 0x62), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VPUNPCKL, OpSize::i32Bit>},
    {OPD(1, 0b01, 0x63), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VPACKSS, OpSize::i16Bit>},
    {OPD(1, 0b01, 0x64), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VCMPGT, OpSize::i8Bit>},
    {OPD(1, 0b01, 0x65), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VCMPGT, OpSize::i16Bit>},
    {OPD(1, 0b01, 0x66), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VCMPGT, OpSize::i32Bit>},
    {OPD(1, 0b01, 0x67), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VPACKUS, OpSize::i16Bit>},
    {OPD(1, 0b01, 0x68), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VPUNPCKH, OpSize::i8Bit>},
    {OPD(1, 0b01, 0x69), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VPUNPCKH, OpSize::i16Bit>},
    {OPD(1, 0b01, 0x6A), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VPUNPCKH, OpSize::i32Bit>},
    {OPD(1, 0b01, 0x6B), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VPACKSS, OpSize::i32Bit>},
    {OPD(1, 0b01, 0x6C), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VPUNPCKL, OpSize::i64Bit>},
    {OPD(1, 0b01, 0x6D), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VPUNPCKH, OpSize::i64Bit>},
    {OPD(1, 0b01, 0x6E), 1, &OpDispatchBuilder::AVX128_MOVBetweenGPR_FPR},

    {OPD(1, 0b01, 0x6F), 1, &OpDispatchBuilder::AVX128_VMOVAPS},
    {OPD(1, 0b10, 0x6F), 1, &OpDispatchBuilder::AVX128_VMOVAPS},

    {OPD(1, 0b01, 0x70), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VPERMILImm, OpSize::i32Bit>},
    {OPD(1, 0b10, 0x70), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VPSHUFW, false>},
    {OPD(1, 0b11, 0x70), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VPSHUFW, true>},

    {OPD(1, 0b01, 0x74), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VCMPEQ, OpSize::i8Bit>},
    {OPD(1, 0b01, 0x75), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VCMPEQ, OpSize::i16Bit>},
    {OPD(1, 0b01, 0x76), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VCMPEQ, OpSize::i32Bit>},

    {OPD(1, 0b00, 0x77), 1, &OpDispatchBuilder::AVX128_VZERO},

    {OPD(1, 0b01, 0x7C), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VHADDP, IR::OP_VFADDP, OpSize::i64Bit>},
    {OPD(1, 0b11, 0x7C), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VHADDP, IR::OP_VFADDP, OpSize::i32Bit>},
    {OPD(1, 0b01, 0x7D), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VHSUBP, OpSize::i64Bit>},
    {OPD(1, 0b11, 0x7D), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VHSUBP, OpSize::i32Bit>},

    {OPD(1, 0b01, 0x7E), 1, &OpDispatchBuilder::AVX128_MOVBetweenGPR_FPR},
    {OPD(1, 0b10, 0x7E), 1, &OpDispatchBuilder::AVX128_MOVQ},

    {OPD(1, 0b01, 0x7F), 1, &OpDispatchBuilder::AVX128_VMOVAPS},
    {OPD(1, 0b10, 0x7F), 1, &OpDispatchBuilder::AVX128_VMOVAPS},

    {OPD(1, 0b00, 0xC2), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VFCMP, OpSize::i32Bit>},
    {OPD(1, 0b01, 0xC2), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VFCMP, OpSize::i64Bit>},
    {OPD(1, 0b10, 0xC2), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_InsertScalarFCMP, OpSize::i32Bit>},
    {OPD(1, 0b11, 0xC2), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_InsertScalarFCMP, OpSize::i64Bit>},

    {OPD(1, 0b01, 0xC4), 1, &OpDispatchBuilder::AVX128_VPINSRW},
    {OPD(1, 0b01, 0xC5), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_PExtr, OpSize::i16Bit>},

    {OPD(1, 0b00, 0xC6), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VSHUF, OpSize::i32Bit>},
    {OPD(1, 0b01, 0xC6), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VSHUF, OpSize::i64Bit>},

    {OPD(1, 0b01, 0xD0), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VADDSUBP, OpSize::i64Bit>},
    {OPD(1, 0b11, 0xD0), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VADDSUBP, OpSize::i32Bit>},

    {OPD(1, 0b01, 0xD1), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorShiftWideImpl, OpSize::i16Bit, IROps::OP_VUSHRSWIDE>}, // VPSRL
    {OPD(1, 0b01, 0xD2), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorShiftWideImpl, OpSize::i32Bit, IROps::OP_VUSHRSWIDE>}, // VPSRL
    {OPD(1, 0b01, 0xD3), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorShiftWideImpl, OpSize::i64Bit, IROps::OP_VUSHRSWIDE>}, // VPSRL
    {OPD(1, 0b01, 0xD4), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VADD, OpSize::i64Bit>},
    {OPD(1, 0b01, 0xD5), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VMUL, OpSize::i16Bit>},
    {OPD(1, 0b01, 0xD6), 1, &OpDispatchBuilder::AVX128_MOVQ},
    {OPD(1, 0b01, 0xD7), 1, &OpDispatchBuilder::AVX128_MOVMSKB},

    {OPD(1, 0b01, 0xD8), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VUQSUB, OpSize::i8Bit>},
    {OPD(1, 0b01, 0xD9), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VUQSUB, OpSize::i16Bit>},
    {OPD(1, 0b01, 0xDA), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VUMIN, OpSize::i8Bit>},
    {OPD(1, 0b01, 0xDB), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VAND, OpSize::i128Bit>},
    {OPD(1, 0b01, 0xDC), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VUQADD, OpSize::i8Bit>},
    {OPD(1, 0b01, 0xDD), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VUQADD, OpSize::i16Bit>},
    {OPD(1, 0b01, 0xDE), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VUMAX, OpSize::i8Bit>},
    {OPD(1, 0b01, 0xDF), 1, &OpDispatchBuilder::AVX128_VANDN},

    {OPD(1, 0b01, 0xE0), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VURAVG, OpSize::i8Bit>},
    {OPD(1, 0b01, 0xE1), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorShiftWideImpl, OpSize::i16Bit, IROps::OP_VSSHRSWIDE>}, // VPSRA
    {OPD(1, 0b01, 0xE2), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorShiftWideImpl, OpSize::i32Bit, IROps::OP_VSSHRSWIDE>}, // VPSRA
    {OPD(1, 0b01, 0xE3), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VURAVG, OpSize::i16Bit>},
    {OPD(1, 0b01, 0xE4), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VPMULHW, false>},
    {OPD(1, 0b01, 0xE5), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VPMULHW, true>},

    {OPD(1, 0b01, 0xE6), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_Vector_CVT_Float_To_Int, OpSize::i64Bit, false>},
    {OPD(1, 0b10, 0xE6), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_Vector_CVT_Int_To_Float, OpSize::i32Bit, true>},
    {OPD(1, 0b11, 0xE6), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_Vector_CVT_Float_To_Int, OpSize::i64Bit, true>},

    {OPD(1, 0b01, 0xE7), 1, &OpDispatchBuilder::AVX128_MOVVectorNT},

    {OPD(1, 0b01, 0xE8), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VSQSUB, OpSize::i8Bit>},
    {OPD(1, 0b01, 0xE9), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VSQSUB, OpSize::i16Bit>},
    {OPD(1, 0b01, 0xEA), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VSMIN, OpSize::i16Bit>},
    {OPD(1, 0b01, 0xEB), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VOR, OpSize::i128Bit>},
    {OPD(1, 0b01, 0xEC), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VSQADD, OpSize::i8Bit>},
    {OPD(1, 0b01, 0xED), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VSQADD, OpSize::i16Bit>},
    {OPD(1, 0b01, 0xEE), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VSMAX, OpSize::i16Bit>},
    {OPD(1, 0b01, 0xEF), 1, &OpDispatchBuilder::AVX128_VectorXOR},

    {OPD(1, 0b11, 0xF0), 1, &OpDispatchBuilder::AVX128_MOVVectorUnaligned},
    {OPD(1, 0b01, 0xF1), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorShiftWideImpl, OpSize::i16Bit, IROps::OP_VUSHLSWIDE>}, // VPSLL
    {OPD(1, 0b01, 0xF2), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorShiftWideImpl, OpSize::i32Bit, IROps::OP_VUSHLSWIDE>}, // VPSLL
    {OPD(1, 0b01, 0xF3), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorShiftWideImpl, OpSize::i64Bit, IROps::OP_VUSHLSWIDE>}, // VPSLL
    {OPD(1, 0b01, 0xF4), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VPMULL, OpSize::i32Bit, false>},
    {OPD(1, 0b01, 0xF5), 1, &OpDispatchBuilder::AVX128_VPMADDWD},
    {OPD(1, 0b01, 0xF6), 1, &OpDispatchBuilder::AVX128_VPSADBW},
    {OPD(1, 0b01, 0xF7), 1, &OpDispatchBuilder::AVX128_MASKMOV},

    {OPD(1, 0b01, 0xF8), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VSUB, OpSize::i8Bit>},
    {OPD(1, 0b01, 0xF9), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VSUB, OpSize::i16Bit>},
    {OPD(1, 0b01, 0xFA), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VSUB, OpSize::i32Bit>},
    {OPD(1, 0b01, 0xFB), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VSUB, OpSize::i64Bit>},
    {OPD(1, 0b01, 0xFC), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VADD, OpSize::i8Bit>},
    {OPD(1, 0b01, 0xFD), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VADD, OpSize::i16Bit>},
    {OPD(1, 0b01, 0xFE), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VADD, OpSize::i32Bit>},

    {OPD(2, 0b01, 0x00), 1, &OpDispatchBuilder::AVX128_VPSHUFB},
    {OPD(2, 0b01, 0x01), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VHADDP, IR::OP_VADDP, OpSize::i16Bit>},
    {OPD(2, 0b01, 0x02), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VHADDP, IR::OP_VADDP, OpSize::i32Bit>},
    {OPD(2, 0b01, 0x03), 1, &OpDispatchBuilder::AVX128_VPHADDSW},
    {OPD(2, 0b01, 0x04), 1, &OpDispatchBuilder::AVX128_VPMADDUBSW},

    {OPD(2, 0b01, 0x05), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VPHSUB, OpSize::i16Bit>},
    {OPD(2, 0b01, 0x06), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VPHSUB, OpSize::i32Bit>},
    {OPD(2, 0b01, 0x07), 1, &OpDispatchBuilder::AVX128_VPHSUBSW},

    {OPD(2, 0b01, 0x08), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VPSIGN, OpSize::i8Bit>},
    {OPD(2, 0b01, 0x09), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VPSIGN, OpSize::i16Bit>},
    {OPD(2, 0b01, 0x0A), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VPSIGN, OpSize::i32Bit>},
    {OPD(2, 0b01, 0x0B), 1, &OpDispatchBuilder::AVX128_VPMULHRSW},
    {OPD(2, 0b01, 0x0C), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VPERMILReg, OpSize::i32Bit>},
    {OPD(2, 0b01, 0x0D), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VPERMILReg, OpSize::i64Bit>},
    {OPD(2, 0b01, 0x0E), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VTESTP, OpSize::i32Bit>},
    {OPD(2, 0b01, 0x0F), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VTESTP, OpSize::i64Bit>},


    {OPD(2, 0b01, 0x13), 1, &OpDispatchBuilder::AVX128_VCVTPH2PS},
    {OPD(2, 0b01, 0x16), 1, &OpDispatchBuilder::AVX128_VPERMD},
    {OPD(2, 0b01, 0x17), 1, &OpDispatchBuilder::AVX128_PTest},
    {OPD(2, 0b01, 0x18), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VBROADCAST, OpSize::i32Bit>},
    {OPD(2, 0b01, 0x19), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VBROADCAST, OpSize::i64Bit>},
    {OPD(2, 0b01, 0x1A), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VBROADCAST, OpSize::i128Bit>},
    {OPD(2, 0b01, 0x1C), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorUnary, IR::OP_VABS, OpSize::i8Bit>},
    {OPD(2, 0b01, 0x1D), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorUnary, IR::OP_VABS, OpSize::i16Bit>},
    {OPD(2, 0b01, 0x1E), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorUnary, IR::OP_VABS, OpSize::i32Bit>},

    {OPD(2, 0b01, 0x20), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_ExtendVectorElements, OpSize::i8Bit, OpSize::i16Bit, true>},
    {OPD(2, 0b01, 0x21), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_ExtendVectorElements, OpSize::i8Bit, OpSize::i32Bit, true>},
    {OPD(2, 0b01, 0x22), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_ExtendVectorElements, OpSize::i8Bit, OpSize::i64Bit, true>},
    {OPD(2, 0b01, 0x23), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_ExtendVectorElements, OpSize::i16Bit, OpSize::i32Bit, true>},
    {OPD(2, 0b01, 0x24), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_ExtendVectorElements, OpSize::i16Bit, OpSize::i64Bit, true>},
    {OPD(2, 0b01, 0x25), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_ExtendVectorElements, OpSize::i32Bit, OpSize::i64Bit, true>},

    {OPD(2, 0b01, 0x28), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VPMULL, OpSize::i32Bit, true>},
    {OPD(2, 0b01, 0x29), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VCMPEQ, OpSize::i64Bit>},
    {OPD(2, 0b01, 0x2A), 1, &OpDispatchBuilder::AVX128_MOVVectorNT},
    {OPD(2, 0b01, 0x2B), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VPACKUS, OpSize::i32Bit>},
    {OPD(2, 0b01, 0x2C), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VMASKMOV, OpSize::i32Bit, false>},
    {OPD(2, 0b01, 0x2D), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VMASKMOV, OpSize::i64Bit, false>},
    {OPD(2, 0b01, 0x2E), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VMASKMOV, OpSize::i32Bit, true>},
    {OPD(2, 0b01, 0x2F), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VMASKMOV, OpSize::i64Bit, true>},

    {OPD(2, 0b01, 0x30), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_ExtendVectorElements, OpSize::i8Bit, OpSize::i16Bit, false>},
    {OPD(2, 0b01, 0x31), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_ExtendVectorElements, OpSize::i8Bit, OpSize::i32Bit, false>},
    {OPD(2, 0b01, 0x32), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_ExtendVectorElements, OpSize::i8Bit, OpSize::i64Bit, false>},
    {OPD(2, 0b01, 0x33), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_ExtendVectorElements, OpSize::i16Bit, OpSize::i32Bit, false>},
    {OPD(2, 0b01, 0x34), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_ExtendVectorElements, OpSize::i16Bit, OpSize::i64Bit, false>},
    {OPD(2, 0b01, 0x35), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_ExtendVectorElements, OpSize::i32Bit, OpSize::i64Bit, false>},
    {OPD(2, 0b01, 0x36), 1, &OpDispatchBuilder::AVX128_VPERMD},

    {OPD(2, 0b01, 0x37), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VCMPGT, OpSize::i64Bit>},
    {OPD(2, 0b01, 0x38), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VSMIN, OpSize::i8Bit>},
    {OPD(2, 0b01, 0x39), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VSMIN, OpSize::i32Bit>},
    {OPD(2, 0b01, 0x3A), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VUMIN, OpSize::i16Bit>},
    {OPD(2, 0b01, 0x3B), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VUMIN, OpSize::i32Bit>},
    {OPD(2, 0b01, 0x3C), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VSMAX, OpSize::i8Bit>},
    {OPD(2, 0b01, 0x3D), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VSMAX, OpSize::i32Bit>},
    {OPD(2, 0b01, 0x3E), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VUMAX, OpSize::i16Bit>},
    {OPD(2, 0b01, 0x3F), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VUMAX, OpSize::i32Bit>},

    {OPD(2, 0b01, 0x40), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VMUL, OpSize::i32Bit>},
    {OPD(2, 0b01, 0x41), 1, &OpDispatchBuilder::AVX128_PHMINPOSUW},
    {OPD(2, 0b01, 0x45), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VariableShiftImpl, IROps::OP_VUSHR>}, // VPSRLV
    {OPD(2, 0b01, 0x46), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VariableShiftImpl, IROps::OP_VSSHR>}, // VPSRAVD
    {OPD(2, 0b01, 0x47), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VariableShiftImpl, IROps::OP_VUSHL>}, // VPSLLV

    {OPD(2, 0b01, 0x58), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VBROADCAST, OpSize::i32Bit>},
    {OPD(2, 0b01, 0x59), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VBROADCAST, OpSize::i64Bit>},
    {OPD(2, 0b01, 0x5A), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VBROADCAST, OpSize::i128Bit>},

    {OPD(2, 0b01, 0x78), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VBROADCAST, OpSize::i8Bit>},
    {OPD(2, 0b01, 0x79), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VBROADCAST, OpSize::i16Bit>},

    {OPD(2, 0b01, 0x8C), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VPMASKMOV, false>},
    {OPD(2, 0b01, 0x8E), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VPMASKMOV, true>},

    {OPD(2, 0b01, 0x90), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VPGATHER, OpSize::i32Bit>},
    {OPD(2, 0b01, 0x91), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VPGATHER, OpSize::i64Bit>},
    {OPD(2, 0b01, 0x92), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VPGATHER, OpSize::i32Bit>},
    {OPD(2, 0b01, 0x93), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VPGATHER, OpSize::i64Bit>},

    {OPD(2, 0b01, 0x96), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VFMAddSubImpl, true, 1, 3, 2>},  // VFMADDSUB
    {OPD(2, 0b01, 0x97), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VFMAddSubImpl, false, 1, 3, 2>}, // VFMSUBADD

    {OPD(2, 0b01, 0x98), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VFMAImpl, IR::OP_VFMLA, 1, 3, 2>}, // VFMADD
    {OPD(2, 0b01, 0x99), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VFMAScalarImpl, IR::OP_VFMLASCALARINSERT, 1, 3, 2>}, // VFMADD
    {OPD(2, 0b01, 0x9A), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VFMAImpl, IR::OP_VFMLS, 1, 3, 2>}, // VFMSUB
    {OPD(2, 0b01, 0x9B), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VFMAScalarImpl, IR::OP_VFMLSSCALARINSERT, 1, 3, 2>}, // VFMSUB
    {OPD(2, 0b01, 0x9C), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VFMAImpl, IR::OP_VFNMLA, 1, 3, 2>}, // VFNMADD
    {OPD(2, 0b01, 0x9D), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VFMAScalarImpl, IR::OP_VFNMLASCALARINSERT, 1, 3, 2>}, // VFNMADD
    {OPD(2, 0b01, 0x9E), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VFMAImpl, IR::OP_VFNMLS, 1, 3, 2>}, // VFNMSUB
    {OPD(2, 0b01, 0x9F), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VFMAScalarImpl, IR::OP_VFNMLSSCALARINSERT, 1, 3, 2>}, // VFNMSUB

    {OPD(2, 0b01, 0xA8), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VFMAImpl, IR::OP_VFMLA, 2, 1, 3>}, // VFMADD
    {OPD(2, 0b01, 0xA9), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VFMAScalarImpl, IR::OP_VFMLASCALARINSERT, 2, 1, 3>}, // VFMADD
    {OPD(2, 0b01, 0xAA), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VFMAImpl, IR::OP_VFMLS, 2, 1, 3>}, // VFMSUB
    {OPD(2, 0b01, 0xAB), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VFMAScalarImpl, IR::OP_VFMLSSCALARINSERT, 2, 1, 3>}, // VFMSUB
    {OPD(2, 0b01, 0xAC), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VFMAImpl, IR::OP_VFNMLA, 2, 1, 3>}, // VFNMADD
    {OPD(2, 0b01, 0xAD), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VFMAScalarImpl, IR::OP_VFNMLASCALARINSERT, 2, 1, 3>}, // VFNMADD
    {OPD(2, 0b01, 0xAE), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VFMAImpl, IR::OP_VFNMLS, 2, 1, 3>}, // VFNMSUB
    {OPD(2, 0b01, 0xAF), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VFMAScalarImpl, IR::OP_VFNMLSSCALARINSERT, 2, 1, 3>}, // VFNMSUB

    {OPD(2, 0b01, 0xB8), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VFMAImpl, IR::OP_VFMLA, 2, 3, 1>}, // VFMADD
    {OPD(2, 0b01, 0xB9), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VFMAScalarImpl, IR::OP_VFMLASCALARINSERT, 2, 3, 1>}, // VFMADD
    {OPD(2, 0b01, 0xBA), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VFMAImpl, IR::OP_VFMLS, 2, 3, 1>}, // VFMSUB
    {OPD(2, 0b01, 0xBB), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VFMAScalarImpl, IR::OP_VFMLSSCALARINSERT, 2, 3, 1>}, // VFMSUB
    {OPD(2, 0b01, 0xBC), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VFMAImpl, IR::OP_VFNMLA, 2, 3, 1>}, // VFNMADD
    {OPD(2, 0b01, 0xBD), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VFMAScalarImpl, IR::OP_VFNMLASCALARINSERT, 2, 3, 1>}, // VFNMADD
    {OPD(2, 0b01, 0xBE), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VFMAImpl, IR::OP_VFNMLS, 2, 3, 1>}, // VFNMSUB
    {OPD(2, 0b01, 0xBF), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VFMAScalarImpl, IR::OP_VFNMLSSCALARINSERT, 2, 3, 1>}, // VFNMSUB

    {OPD(2, 0b01, 0xA6), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VFMAddSubImpl, true, 2, 1, 3>},  // VFMADDSUB
    {OPD(2, 0b01, 0xA7), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VFMAddSubImpl, false, 2, 1, 3>}, // VFMSUBADD

    {OPD(2, 0b01, 0xB6), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VFMAddSubImpl, true, 2, 3, 1>},  // VFMADDSUB
    {OPD(2, 0b01, 0xB7), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VFMAddSubImpl, false, 2, 3, 1>}, // VFMSUBADD

    {OPD(2, 0b01, 0xDB), 1, &OpDispatchBuilder::AVX128_VAESImc},
    {OPD(2, 0b01, 0xDC), 1, &OpDispatchBuilder::AVX128_VAESEnc},
    {OPD(2, 0b01, 0xDD), 1, &OpDispatchBuilder::AVX128_VAESEncLast},
    {OPD(2, 0b01, 0xDE), 1, &OpDispatchBuilder::AVX128_VAESDec},
    {OPD(2, 0b01, 0xDF), 1, &OpDispatchBuilder::AVX128_VAESDecLast},

    {OPD(3, 0b01, 0x00), 1, &OpDispatchBuilder::AVX128_VPERMQ},
    {OPD(3, 0b01, 0x01), 1, &OpDispatchBuilder::AVX128_VPERMQ},
    {OPD(3, 0b01, 0x02), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VBLEND, OpSize::i32Bit>},
    {OPD(3, 0b01, 0x04), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VPERMILImm, OpSize::i32Bit>},
    {OPD(3, 0b01, 0x05), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VPERMILImm, OpSize::i64Bit>},
    {OPD(3, 0b01, 0x06), 1, &OpDispatchBuilder::AVX128_VPERM2},
    {OPD(3, 0b01, 0x08), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorRound, OpSize::i32Bit>},
    {OPD(3, 0b01, 0x09), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorRound, OpSize::i64Bit>},
    {OPD(3, 0b01, 0x0A), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_InsertScalarRound, OpSize::i32Bit>},
    {OPD(3, 0b01, 0x0B), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_InsertScalarRound, OpSize::i64Bit>},
    {OPD(3, 0b01, 0x0C), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VBLEND, OpSize::i32Bit>},
    {OPD(3, 0b01, 0x0D), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VBLEND, OpSize::i64Bit>},
    {OPD(3, 0b01, 0x0E), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VBLEND, OpSize::i16Bit>},
    {OPD(3, 0b01, 0x0F), 1, &OpDispatchBuilder::AVX128_VPALIGNR},

    {OPD(3, 0b01, 0x14), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_PExtr, OpSize::i8Bit>},
    {OPD(3, 0b01, 0x15), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_PExtr, OpSize::i16Bit>},
    {OPD(3, 0b01, 0x16), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_PExtr, OpSize::i32Bit>},
    {OPD(3, 0b01, 0x17), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_PExtr, OpSize::i32Bit>},

    {OPD(3, 0b01, 0x18), 1, &OpDispatchBuilder::AVX128_VINSERT},
    {OPD(3, 0b01, 0x19), 1, &OpDispatchBuilder::AVX128_VEXTRACT128},
    {OPD(3, 0b01, 0x1D), 1, &OpDispatchBuilder::AVX128_VCVTPS2PH},
    {OPD(3, 0b01, 0x20), 1, &OpDispatchBuilder::AVX128_VPINSRB},
    {OPD(3, 0b01, 0x21), 1, &OpDispatchBuilder::AVX128_VINSERTPS},
    {OPD(3, 0b01, 0x22), 1, &OpDispatchBuilder::AVX128_VPINSRDQ},

    {OPD(3, 0b01, 0x38), 1, &OpDispatchBuilder::AVX128_VINSERT},
    {OPD(3, 0b01, 0x39), 1, &OpDispatchBuilder::AVX128_VEXTRACT128},

    {OPD(3, 0b01, 0x40), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VDPP, OpSize::i32Bit>},
    {OPD(3, 0b01, 0x41), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VDPP, OpSize::i64Bit>},
    {OPD(3, 0b01, 0x42), 1, &OpDispatchBuilder::AVX128_VMPSADBW},
    {OPD(3, 0b01, 0x44), 1, &OpDispatchBuilder::AVX128_VPCLMULQDQ},

    {OPD(3, 0b01, 0x46), 1, &OpDispatchBuilder::AVX128_VPERM2},

    {OPD(3, 0b01, 0x4A), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorVariableBlend, OpSize::i32Bit>},
    {OPD(3, 0b01, 0x4B), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorVariableBlend, OpSize::i64Bit>},
    {OPD(3, 0b01, 0x4C), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorVariableBlend, OpSize::i8Bit>},

    {OPD(3, 0b01, 0x60), 1, &OpDispatchBuilder::AVX128_VPCMPESTRM},
    {OPD(3, 0b01, 0x61), 1, &OpDispatchBuilder::AVX128_VPCMPESTRI},
    {OPD(3, 0b01, 0x62), 1, &OpDispatchBuilder::AVX128_VPCMPISTRM},
    {OPD(3, 0b01, 0x63), 1, &OpDispatchBuilder::AVX128_VPCMPISTRI},

    {OPD(3, 0b01, 0xDF), 1, &OpDispatchBuilder::AVX128_VAESKeyGenAssist},
  };
#undef OPD

#define OPD(group, pp, opcode) (((group - X86Tables::TYPE_VEX_GROUP_12) << 4) | (pp << 3) | (opcode))
  constexpr DispatchTableEntry TableGroupOps[] {
    // VPSRLI
    {OPD(X86Tables::TYPE_VEX_GROUP_12, 1, 0b010), 1,
     &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorShiftImmImpl, OpSize::i16Bit, IROps::OP_VUSHRI>},
    // VPSLLI
    {OPD(X86Tables::TYPE_VEX_GROUP_12, 1, 0b110), 1,
     &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorShiftImmImpl, OpSize::i16Bit, IROps::OP_VSHLI>},
    // VPSRAI
    {OPD(X86Tables::TYPE_VEX_GROUP_12, 1, 0b100), 1,
     &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorShiftImmImpl, OpSize::i16Bit, IROps::OP_VSSHRI>},

    // VPSRLI
    {OPD(X86Tables::TYPE_VEX_GROUP_13, 1, 0b010), 1,
     &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorShiftImmImpl, OpSize::i32Bit, IROps::OP_VUSHRI>},
    // VPSLLI
    {OPD(X86Tables::TYPE_VEX_GROUP_13, 1, 0b110), 1,
     &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorShiftImmImpl, OpSize::i32Bit, IROps::OP_VSHLI>},
    // VPSRAI
    {OPD(X86Tables::TYPE_VEX_GROUP_13, 1, 0b100), 1,
     &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorShiftImmImpl, OpSize::i32Bit, IROps::OP_VSSHRI>},

    // VPSRLI
    {OPD(X86Tables::TYPE_VEX_GROUP_14, 1, 0b010), 1,
     &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorShiftImmImpl, OpSize::i64Bit, IROps::OP_VUSHRI>},
    // VPSRLDQ
    {OPD(X86Tables::TYPE_VEX_GROUP_14, 1, 0b011), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_ShiftDoubleImm, OpDispatchBuilder::ShiftDirection::RIGHT>},
    // VPSLLI
    {OPD(X86Tables::TYPE_VEX_GROUP_14, 1, 0b110), 1,
     &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorShiftImmImpl, OpSize::i64Bit, IROps::OP_VSHLI>},
    // VPSLLDQ
    {OPD(X86Tables::TYPE_VEX_GROUP_14, 1, 0b111), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_ShiftDoubleImm, OpDispatchBuilder::ShiftDirection::LEFT>},

    ///< Use the regular implementation. It just happens to be in the VEX table.
    {OPD(X86Tables::TYPE_VEX_GROUP_15, 0, 0b010), 1, &OpDispatchBuilder::LDMXCSR},
    {OPD(X86Tables::TYPE_VEX_GROUP_15, 0, 0b011), 1, &OpDispatchBuilder::STMXCSR},
  };
#undef OPD
}

namespace AVX256 {
  using namespace IR;
#define OPD(map_select, pp, opcode) (((map_select - 1) << 10) | (pp << 8) | (opcode))
  constexpr DispatchTableEntry BaseTable[] = {
    {OPD(1, 0b00, 0x10), 1, &OpDispatchBuilder::VMOVUPS_VMOVUPDOp},
    {OPD(1, 0b01, 0x10), 1, &OpDispatchBuilder::VMOVUPS_VMOVUPDOp},
    {OPD(1, 0b10, 0x10), 1, &OpDispatchBuilder::VMOVSSOp},
    {OPD(1, 0b11, 0x10), 1, &OpDispatchBuilder::VMOVSDOp},
    {OPD(1, 0b00, 0x11), 1, &OpDispatchBuilder::VMOVUPS_VMOVUPDOp},
    {OPD(1, 0b01, 0x11), 1, &OpDispatchBuilder::VMOVUPS_VMOVUPDOp},
    {OPD(1, 0b10, 0x11), 1, &OpDispatchBuilder::VMOVSSOp},
    {OPD(1, 0b11, 0x11), 1, &OpDispatchBuilder::VMOVSDOp},

    {OPD(1, 0b00, 0x12), 1, &OpDispatchBuilder::VMOVLPOp},
    {OPD(1, 0b01, 0x12), 1, &OpDispatchBuilder::VMOVLPOp},
    {OPD(1, 0b10, 0x12), 1, &OpDispatchBuilder::VMOVSLDUPOp},
    {OPD(1, 0b11, 0x12), 1, &OpDispatchBuilder::VMOVDDUPOp},
    {OPD(1, 0b00, 0x13), 1, &OpDispatchBuilder::VMOVLPOp},
    {OPD(1, 0b01, 0x13), 1, &OpDispatchBuilder::VMOVLPOp},

    {OPD(1, 0b00, 0x14), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VPUNPCKLOp, OpSize::i32Bit>},
    {OPD(1, 0b01, 0x14), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VPUNPCKLOp, OpSize::i64Bit>},

    {OPD(1, 0b00, 0x15), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VPUNPCKHOp, OpSize::i32Bit>},
    {OPD(1, 0b01, 0x15), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VPUNPCKHOp, OpSize::i64Bit>},

    {OPD(1, 0b00, 0x16), 1, &OpDispatchBuilder::VMOVHPOp},
    {OPD(1, 0b01, 0x16), 1, &OpDispatchBuilder::VMOVHPOp},
    {OPD(1, 0b10, 0x16), 1, &OpDispatchBuilder::VMOVSHDUPOp},
    {OPD(1, 0b00, 0x17), 1, &OpDispatchBuilder::VMOVHPOp},
    {OPD(1, 0b01, 0x17), 1, &OpDispatchBuilder::VMOVHPOp},

    {OPD(1, 0b00, 0x28), 1, &OpDispatchBuilder::VMOVAPS_VMOVAPDOp},
    {OPD(1, 0b01, 0x28), 1, &OpDispatchBuilder::VMOVAPS_VMOVAPDOp},
    {OPD(1, 0b00, 0x29), 1, &OpDispatchBuilder::VMOVAPS_VMOVAPDOp},
    {OPD(1, 0b01, 0x29), 1, &OpDispatchBuilder::VMOVAPS_VMOVAPDOp},

    {OPD(1, 0b10, 0x2A), 1, &OpDispatchBuilder::AVXInsertCVTGPR_To_FPR<OpSize::i32Bit>},
    {OPD(1, 0b11, 0x2A), 1, &OpDispatchBuilder::AVXInsertCVTGPR_To_FPR<OpSize::i64Bit>},

    {OPD(1, 0b00, 0x2B), 1, &OpDispatchBuilder::MOVVectorNTOp},
    {OPD(1, 0b01, 0x2B), 1, &OpDispatchBuilder::MOVVectorNTOp},

    {OPD(1, 0b10, 0x2C), 1, &OpDispatchBuilder::CVTFPR_To_GPR<OpSize::i32Bit, false>},
    {OPD(1, 0b11, 0x2C), 1, &OpDispatchBuilder::CVTFPR_To_GPR<OpSize::i64Bit, false>},

    {OPD(1, 0b10, 0x2D), 1, &OpDispatchBuilder::CVTFPR_To_GPR<OpSize::i32Bit, true>},
    {OPD(1, 0b11, 0x2D), 1, &OpDispatchBuilder::CVTFPR_To_GPR<OpSize::i64Bit, true>},

    {OPD(1, 0b00, 0x2E), 1, &OpDispatchBuilder::UCOMISxOp<OpSize::i32Bit>},
    {OPD(1, 0b01, 0x2E), 1, &OpDispatchBuilder::UCOMISxOp<OpSize::i64Bit>},
    {OPD(1, 0b00, 0x2F), 1, &OpDispatchBuilder::UCOMISxOp<OpSize::i32Bit>},
    {OPD(1, 0b01, 0x2F), 1, &OpDispatchBuilder::UCOMISxOp<OpSize::i64Bit>},

    {OPD(1, 0b00, 0x50), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::MOVMSKOp, OpSize::i32Bit>},
    {OPD(1, 0b01, 0x50), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::MOVMSKOp, OpSize::i64Bit>},

    {OPD(1, 0b00, 0x51), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorUnaryOp, IR::OP_VFSQRT, OpSize::i32Bit>},
    {OPD(1, 0b01, 0x51), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorUnaryOp, IR::OP_VFSQRT, OpSize::i64Bit>},
    {OPD(1, 0b10, 0x51), 1, &OpDispatchBuilder::AVXVectorScalarUnaryInsertALUOp<IR::OP_VFSQRTSCALARINSERT, OpSize::i32Bit>},
    {OPD(1, 0b11, 0x51), 1, &OpDispatchBuilder::AVXVectorScalarUnaryInsertALUOp<IR::OP_VFSQRTSCALARINSERT, OpSize::i64Bit>},

    {OPD(1, 0b00, 0x52), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorUnaryOp, IR::OP_VFRSQRT, OpSize::i32Bit>},
    {OPD(1, 0b10, 0x52), 1, &OpDispatchBuilder::AVXVectorScalarUnaryInsertALUOp<IR::OP_VFRSQRTSCALARINSERT, OpSize::i32Bit>},

    {OPD(1, 0b00, 0x53), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorUnaryOp, IR::OP_VFRECP, OpSize::i32Bit>},
    {OPD(1, 0b10, 0x53), 1, &OpDispatchBuilder::AVXVectorScalarUnaryInsertALUOp<IR::OP_VFRECPSCALARINSERT, OpSize::i32Bit>},

    {OPD(1, 0b00, 0x54), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorALUOp, IR::OP_VAND, OpSize::i128Bit>},
    {OPD(1, 0b01, 0x54), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorALUOp, IR::OP_VAND, OpSize::i128Bit>},

    {OPD(1, 0b00, 0x55), 1, &OpDispatchBuilder::VANDNOp},
    {OPD(1, 0b01, 0x55), 1, &OpDispatchBuilder::VANDNOp},

    {OPD(1, 0b00, 0x56), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorALUOp, IR::OP_VOR, OpSize::i128Bit>},
    {OPD(1, 0b01, 0x56), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorALUOp, IR::OP_VOR, OpSize::i128Bit>},

    {OPD(1, 0b00, 0x57), 1, &OpDispatchBuilder::AVXVectorXOROp},
    {OPD(1, 0b01, 0x57), 1, &OpDispatchBuilder::AVXVectorXOROp},

    {OPD(1, 0b00, 0x58), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorALUOp, IR::OP_VFADD, OpSize::i32Bit>},
    {OPD(1, 0b01, 0x58), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorALUOp, IR::OP_VFADD, OpSize::i64Bit>},
    {OPD(1, 0b10, 0x58), 1, &OpDispatchBuilder::AVXVectorScalarInsertALUOp<IR::OP_VFADDSCALARINSERT, OpSize::i32Bit>},
    {OPD(1, 0b11, 0x58), 1, &OpDispatchBuilder::AVXVectorScalarInsertALUOp<IR::OP_VFADDSCALARINSERT, OpSize::i64Bit>},

    {OPD(1, 0b00, 0x59), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorALUOp, IR::OP_VFMUL, OpSize::i32Bit>},
    {OPD(1, 0b01, 0x59), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorALUOp, IR::OP_VFMUL, OpSize::i64Bit>},
    {OPD(1, 0b10, 0x59), 1, &OpDispatchBuilder::AVXVectorScalarInsertALUOp<IR::OP_VFMULSCALARINSERT, OpSize::i32Bit>},
    {OPD(1, 0b11, 0x59), 1, &OpDispatchBuilder::AVXVectorScalarInsertALUOp<IR::OP_VFMULSCALARINSERT, OpSize::i64Bit>},

    {OPD(1, 0b00, 0x5A), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::Vector_CVT_Float_To_Float, OpSize::i64Bit, OpSize::i32Bit, true>},
    {OPD(1, 0b01, 0x5A), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::Vector_CVT_Float_To_Float, OpSize::i32Bit, OpSize::i64Bit, true>},
    {OPD(1, 0b10, 0x5A), 1, &OpDispatchBuilder::AVXInsertScalar_CVT_Float_To_Float<OpSize::i64Bit, OpSize::i32Bit>},
    {OPD(1, 0b11, 0x5A), 1, &OpDispatchBuilder::AVXInsertScalar_CVT_Float_To_Float<OpSize::i32Bit, OpSize::i64Bit>},

    {OPD(1, 0b00, 0x5B), 1, &OpDispatchBuilder::Vector_CVT_Int_To_Float<OpSize::i32Bit, false>},
    {OPD(1, 0b01, 0x5B), 1, &OpDispatchBuilder::Vector_CVT_Float_To_Int<OpSize::i32Bit, true>},
    {OPD(1, 0b10, 0x5B), 1, &OpDispatchBuilder::Vector_CVT_Float_To_Int<OpSize::i32Bit, false>},

    {OPD(1, 0b00, 0x5C), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorALUOp, IR::OP_VFSUB, OpSize::i32Bit>},
    {OPD(1, 0b01, 0x5C), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorALUOp, IR::OP_VFSUB, OpSize::i64Bit>},
    {OPD(1, 0b10, 0x5C), 1, &OpDispatchBuilder::AVXVectorScalarInsertALUOp<IR::OP_VFSUBSCALARINSERT, OpSize::i32Bit>},
    {OPD(1, 0b11, 0x5C), 1, &OpDispatchBuilder::AVXVectorScalarInsertALUOp<IR::OP_VFSUBSCALARINSERT, OpSize::i64Bit>},

    {OPD(1, 0b00, 0x5D), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorALUOp, IR::OP_VFMIN, OpSize::i32Bit>},
    {OPD(1, 0b01, 0x5D), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorALUOp, IR::OP_VFMIN, OpSize::i64Bit>},
    {OPD(1, 0b10, 0x5D), 1, &OpDispatchBuilder::AVXVectorScalarInsertALUOp<IR::OP_VFMINSCALARINSERT, OpSize::i32Bit>},
    {OPD(1, 0b11, 0x5D), 1, &OpDispatchBuilder::AVXVectorScalarInsertALUOp<IR::OP_VFMINSCALARINSERT, OpSize::i64Bit>},

    {OPD(1, 0b00, 0x5E), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorALUOp, IR::OP_VFDIV, OpSize::i32Bit>},
    {OPD(1, 0b01, 0x5E), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorALUOp, IR::OP_VFDIV, OpSize::i64Bit>},
    {OPD(1, 0b10, 0x5E), 1, &OpDispatchBuilder::AVXVectorScalarInsertALUOp<IR::OP_VFDIVSCALARINSERT, OpSize::i32Bit>},
    {OPD(1, 0b11, 0x5E), 1, &OpDispatchBuilder::AVXVectorScalarInsertALUOp<IR::OP_VFDIVSCALARINSERT, OpSize::i64Bit>},

    {OPD(1, 0b00, 0x5F), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorALUOp, IR::OP_VFMAX, OpSize::i32Bit>},
    {OPD(1, 0b01, 0x5F), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorALUOp, IR::OP_VFMAX, OpSize::i64Bit>},
    {OPD(1, 0b10, 0x5F), 1, &OpDispatchBuilder::AVXVectorScalarInsertALUOp<IR::OP_VFMAXSCALARINSERT, OpSize::i32Bit>},
    {OPD(1, 0b11, 0x5F), 1, &OpDispatchBuilder::AVXVectorScalarInsertALUOp<IR::OP_VFMAXSCALARINSERT, OpSize::i64Bit>},

    {OPD(1, 0b01, 0x60), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VPUNPCKLOp, OpSize::i8Bit>},
    {OPD(1, 0b01, 0x61), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VPUNPCKLOp, OpSize::i16Bit>},
    {OPD(1, 0b01, 0x62), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VPUNPCKLOp, OpSize::i32Bit>},
    {OPD(1, 0b01, 0x63), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VPACKSSOp, OpSize::i16Bit>},
    {OPD(1, 0b01, 0x64), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorALUOp, IR::OP_VCMPGT, OpSize::i8Bit>},
    {OPD(1, 0b01, 0x65), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorALUOp, IR::OP_VCMPGT, OpSize::i16Bit>},
    {OPD(1, 0b01, 0x66), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorALUOp, IR::OP_VCMPGT, OpSize::i32Bit>},
    {OPD(1, 0b01, 0x67), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VPACKUSOp, OpSize::i16Bit>},
    {OPD(1, 0b01, 0x68), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VPUNPCKHOp, OpSize::i8Bit>},
    {OPD(1, 0b01, 0x69), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VPUNPCKHOp, OpSize::i16Bit>},
    {OPD(1, 0b01, 0x6A), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VPUNPCKHOp, OpSize::i32Bit>},
    {OPD(1, 0b01, 0x6B), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VPACKSSOp, OpSize::i32Bit>},
    {OPD(1, 0b01, 0x6C), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VPUNPCKLOp, OpSize::i64Bit>},
    {OPD(1, 0b01, 0x6D), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VPUNPCKHOp, OpSize::i64Bit>},
    {OPD(1, 0b01, 0x6E), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::MOVBetweenGPR_FPR, OpDispatchBuilder::VectorOpType::AVX>},

    {OPD(1, 0b01, 0x6F), 1, &OpDispatchBuilder::VMOVAPS_VMOVAPDOp},
    {OPD(1, 0b10, 0x6F), 1, &OpDispatchBuilder::VMOVUPS_VMOVUPDOp},

    {OPD(1, 0b01, 0x70), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VPSHUFWOp, OpSize::i32Bit, true>},
    {OPD(1, 0b10, 0x70), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VPSHUFWOp, OpSize::i16Bit, false>},
    {OPD(1, 0b11, 0x70), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VPSHUFWOp, OpSize::i16Bit, true>},

    {OPD(1, 0b01, 0x74), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorALUOp, IR::OP_VCMPEQ, OpSize::i8Bit>},
    {OPD(1, 0b01, 0x75), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorALUOp, IR::OP_VCMPEQ, OpSize::i16Bit>},
    {OPD(1, 0b01, 0x76), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorALUOp, IR::OP_VCMPEQ, OpSize::i32Bit>},

    {OPD(1, 0b00, 0x77), 1, &OpDispatchBuilder::VZEROOp},

    {OPD(1, 0b01, 0x7C), 1, &OpDispatchBuilder::VHADDPOp<IR::OP_VFADDP, OpSize::i64Bit>},
    {OPD(1, 0b11, 0x7C), 1, &OpDispatchBuilder::VHADDPOp<IR::OP_VFADDP, OpSize::i32Bit>},
    {OPD(1, 0b01, 0x7D), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VHSUBPOp, OpSize::i64Bit>},
    {OPD(1, 0b11, 0x7D), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VHSUBPOp, OpSize::i32Bit>},

    {OPD(1, 0b01, 0x7E), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::MOVBetweenGPR_FPR, OpDispatchBuilder::VectorOpType::AVX>},
    {OPD(1, 0b10, 0x7E), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::MOVQOp, OpDispatchBuilder::VectorOpType::AVX>},

    {OPD(1, 0b01, 0x7F), 1, &OpDispatchBuilder::VMOVAPS_VMOVAPDOp},
    {OPD(1, 0b10, 0x7F), 1, &OpDispatchBuilder::VMOVUPS_VMOVUPDOp},

    {OPD(1, 0b00, 0xC2), 1, &OpDispatchBuilder::AVXVFCMPOp<OpSize::i32Bit>},
    {OPD(1, 0b01, 0xC2), 1, &OpDispatchBuilder::AVXVFCMPOp<OpSize::i64Bit>},
    {OPD(1, 0b10, 0xC2), 1, &OpDispatchBuilder::AVXInsertScalarFCMPOp<OpSize::i32Bit>},
    {OPD(1, 0b11, 0xC2), 1, &OpDispatchBuilder::AVXInsertScalarFCMPOp<OpSize::i64Bit>},

    {OPD(1, 0b01, 0xC4), 1, &OpDispatchBuilder::VPINSRWOp},
    {OPD(1, 0b01, 0xC5), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::PExtrOp, OpSize::i16Bit>},

    {OPD(1, 0b00, 0xC6), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VSHUFOp, OpSize::i32Bit>},
    {OPD(1, 0b01, 0xC6), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VSHUFOp, OpSize::i64Bit>},

    {OPD(1, 0b01, 0xD0), 1, &OpDispatchBuilder::VADDSUBPOp<OpSize::i64Bit>},
    {OPD(1, 0b11, 0xD0), 1, &OpDispatchBuilder::VADDSUBPOp<OpSize::i32Bit>},

    {OPD(1, 0b01, 0xD1), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VPSRLDOp, OpSize::i16Bit>},
    {OPD(1, 0b01, 0xD2), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VPSRLDOp, OpSize::i32Bit>},
    {OPD(1, 0b01, 0xD3), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VPSRLDOp, OpSize::i64Bit>},
    {OPD(1, 0b01, 0xD4), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorALUOp, IR::OP_VADD, OpSize::i64Bit>},
    {OPD(1, 0b01, 0xD5), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorALUOp, IR::OP_VMUL, OpSize::i16Bit>},
    {OPD(1, 0b01, 0xD6), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::MOVQOp, OpDispatchBuilder::VectorOpType::AVX>},
    {OPD(1, 0b01, 0xD7), 1, &OpDispatchBuilder::MOVMSKOpOne},

    {OPD(1, 0b01, 0xD8), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorALUOp, IR::OP_VUQSUB, OpSize::i8Bit>},
    {OPD(1, 0b01, 0xD9), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorALUOp, IR::OP_VUQSUB, OpSize::i16Bit>},
    {OPD(1, 0b01, 0xDA), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorALUOp, IR::OP_VUMIN, OpSize::i8Bit>},
    {OPD(1, 0b01, 0xDB), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorALUOp, IR::OP_VAND, OpSize::i128Bit>},
    {OPD(1, 0b01, 0xDC), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorALUOp, IR::OP_VUQADD, OpSize::i8Bit>},
    {OPD(1, 0b01, 0xDD), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorALUOp, IR::OP_VUQADD, OpSize::i16Bit>},
    {OPD(1, 0b01, 0xDE), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorALUOp, IR::OP_VUMAX, OpSize::i8Bit>},
    {OPD(1, 0b01, 0xDF), 1, &OpDispatchBuilder::VANDNOp},

    {OPD(1, 0b01, 0xE0), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorALUOp, IR::OP_VURAVG, OpSize::i8Bit>},
    {OPD(1, 0b01, 0xE1), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VPSRAOp, OpSize::i16Bit>},
    {OPD(1, 0b01, 0xE2), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VPSRAOp, OpSize::i32Bit>},
    {OPD(1, 0b01, 0xE3), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorALUOp, IR::OP_VURAVG, OpSize::i16Bit>},
    {OPD(1, 0b01, 0xE4), 1, &OpDispatchBuilder::VPMULHWOp<false>},
    {OPD(1, 0b01, 0xE5), 1, &OpDispatchBuilder::VPMULHWOp<true>},

    {OPD(1, 0b01, 0xE6), 1, &OpDispatchBuilder::Vector_CVT_Float_To_Int<OpSize::i64Bit, false>},
    {OPD(1, 0b10, 0xE6), 1, &OpDispatchBuilder::Vector_CVT_Int_To_Float<OpSize::i32Bit, true>},
    {OPD(1, 0b11, 0xE6), 1, &OpDispatchBuilder::Vector_CVT_Float_To_Int<OpSize::i64Bit, true>},

    {OPD(1, 0b01, 0xE7), 1, &OpDispatchBuilder::MOVVectorNTOp},

    {OPD(1, 0b01, 0xE8), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorALUOp, IR::OP_VSQSUB, OpSize::i8Bit>},
    {OPD(1, 0b01, 0xE9), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorALUOp, IR::OP_VSQSUB, OpSize::i16Bit>},
    {OPD(1, 0b01, 0xEA), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorALUOp, IR::OP_VSMIN, OpSize::i16Bit>},
    {OPD(1, 0b01, 0xEB), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorALUOp, IR::OP_VOR, OpSize::i128Bit>},
    {OPD(1, 0b01, 0xEC), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorALUOp, IR::OP_VSQADD, OpSize::i8Bit>},
    {OPD(1, 0b01, 0xED), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorALUOp, IR::OP_VSQADD, OpSize::i16Bit>},
    {OPD(1, 0b01, 0xEE), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorALUOp, IR::OP_VSMAX, OpSize::i16Bit>},
    {OPD(1, 0b01, 0xEF), 1, &OpDispatchBuilder::AVXVectorXOROp},

    {OPD(1, 0b11, 0xF0), 1, &OpDispatchBuilder::MOVVectorUnalignedOp},
    {OPD(1, 0b01, 0xF1), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VPSLLOp, OpSize::i16Bit>},
    {OPD(1, 0b01, 0xF2), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VPSLLOp, OpSize::i32Bit>},
    {OPD(1, 0b01, 0xF3), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VPSLLOp, OpSize::i64Bit>},
    {OPD(1, 0b01, 0xF4), 1, &OpDispatchBuilder::VPMULLOp<OpSize::i32Bit, false>},
    {OPD(1, 0b01, 0xF5), 1, &OpDispatchBuilder::VPMADDWDOp},
    {OPD(1, 0b01, 0xF6), 1, &OpDispatchBuilder::VPSADBWOp},
    {OPD(1, 0b01, 0xF7), 1, &OpDispatchBuilder::MASKMOVOp},

    {OPD(1, 0b01, 0xF8), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorALUOp, IR::OP_VSUB, OpSize::i8Bit>},
    {OPD(1, 0b01, 0xF9), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorALUOp, IR::OP_VSUB, OpSize::i16Bit>},
    {OPD(1, 0b01, 0xFA), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorALUOp, IR::OP_VSUB, OpSize::i32Bit>},
    {OPD(1, 0b01, 0xFB), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorALUOp, IR::OP_VSUB, OpSize::i64Bit>},
    {OPD(1, 0b01, 0xFC), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorALUOp, IR::OP_VADD, OpSize::i8Bit>},
    {OPD(1, 0b01, 0xFD), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorALUOp, IR::OP_VADD, OpSize::i16Bit>},
    {OPD(1, 0b01, 0xFE), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorALUOp, IR::OP_VADD, OpSize::i32Bit>},

    {OPD(2, 0b01, 0x00), 1, &OpDispatchBuilder::VPSHUFBOp},
    {OPD(2, 0b01, 0x01), 1, &OpDispatchBuilder::VHADDPOp<IR::OP_VADDP, OpSize::i16Bit>},
    {OPD(2, 0b01, 0x02), 1, &OpDispatchBuilder::VHADDPOp<IR::OP_VADDP, OpSize::i32Bit>},
    {OPD(2, 0b01, 0x03), 1, &OpDispatchBuilder::VPHADDSWOp},
    {OPD(2, 0b01, 0x04), 1, &OpDispatchBuilder::VPMADDUBSWOp},

    {OPD(2, 0b01, 0x05), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VPHSUBOp, OpSize::i16Bit>},
    {OPD(2, 0b01, 0x06), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VPHSUBOp, OpSize::i32Bit>},
    {OPD(2, 0b01, 0x07), 1, &OpDispatchBuilder::VPHSUBSWOp},

    {OPD(2, 0b01, 0x08), 1, &OpDispatchBuilder::VPSIGN<OpSize::i8Bit>},
    {OPD(2, 0b01, 0x09), 1, &OpDispatchBuilder::VPSIGN<OpSize::i16Bit>},
    {OPD(2, 0b01, 0x0A), 1, &OpDispatchBuilder::VPSIGN<OpSize::i32Bit>},
    {OPD(2, 0b01, 0x0B), 1, &OpDispatchBuilder::VPMULHRSWOp},
    {OPD(2, 0b01, 0x0C), 1, &OpDispatchBuilder::VPERMILRegOp<OpSize::i32Bit>},
    {OPD(2, 0b01, 0x0D), 1, &OpDispatchBuilder::VPERMILRegOp<OpSize::i64Bit>},
    {OPD(2, 0b01, 0x0E), 1, &OpDispatchBuilder::VTESTPOp<OpSize::i32Bit>},
    {OPD(2, 0b01, 0x0F), 1, &OpDispatchBuilder::VTESTPOp<OpSize::i64Bit>},

    {OPD(2, 0b01, 0x13), 1, &OpDispatchBuilder::VCVTPH2PSOp},
    {OPD(2, 0b01, 0x16), 1, &OpDispatchBuilder::VPERMDOp},
    {OPD(2, 0b01, 0x17), 1, &OpDispatchBuilder::PTestOp},
    {OPD(2, 0b01, 0x18), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VBROADCASTOp, OpSize::i32Bit>},
    {OPD(2, 0b01, 0x19), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VBROADCASTOp, OpSize::i64Bit>},
    {OPD(2, 0b01, 0x1A), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VBROADCASTOp, OpSize::i128Bit>},
    {OPD(2, 0b01, 0x1C), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorUnaryOp, IR::OP_VABS, OpSize::i8Bit>},
    {OPD(2, 0b01, 0x1D), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorUnaryOp, IR::OP_VABS, OpSize::i16Bit>},
    {OPD(2, 0b01, 0x1E), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorUnaryOp, IR::OP_VABS, OpSize::i32Bit>},

    {OPD(2, 0b01, 0x20), 1, &OpDispatchBuilder::ExtendVectorElements<OpSize::i8Bit, OpSize::i16Bit, true>},
    {OPD(2, 0b01, 0x21), 1, &OpDispatchBuilder::ExtendVectorElements<OpSize::i8Bit, OpSize::i32Bit, true>},
    {OPD(2, 0b01, 0x22), 1, &OpDispatchBuilder::ExtendVectorElements<OpSize::i8Bit, OpSize::i64Bit, true>},
    {OPD(2, 0b01, 0x23), 1, &OpDispatchBuilder::ExtendVectorElements<OpSize::i16Bit, OpSize::i32Bit, true>},
    {OPD(2, 0b01, 0x24), 1, &OpDispatchBuilder::ExtendVectorElements<OpSize::i16Bit, OpSize::i64Bit, true>},
    {OPD(2, 0b01, 0x25), 1, &OpDispatchBuilder::ExtendVectorElements<OpSize::i32Bit, OpSize::i64Bit, true>},

    {OPD(2, 0b01, 0x28), 1, &OpDispatchBuilder::VPMULLOp<OpSize::i32Bit, true>},
    {OPD(2, 0b01, 0x29), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorALUOp, IR::OP_VCMPEQ, OpSize::i64Bit>},
    {OPD(2, 0b01, 0x2A), 1, &OpDispatchBuilder::MOVVectorNTOp},
    {OPD(2, 0b01, 0x2B), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VPACKUSOp, OpSize::i32Bit>},
    {OPD(2, 0b01, 0x2C), 1, &OpDispatchBuilder::VMASKMOVOp<OpSize::i32Bit, false>},
    {OPD(2, 0b01, 0x2D), 1, &OpDispatchBuilder::VMASKMOVOp<OpSize::i64Bit, false>},
    {OPD(2, 0b01, 0x2E), 1, &OpDispatchBuilder::VMASKMOVOp<OpSize::i32Bit, true>},
    {OPD(2, 0b01, 0x2F), 1, &OpDispatchBuilder::VMASKMOVOp<OpSize::i64Bit, true>},

    {OPD(2, 0b01, 0x30), 1, &OpDispatchBuilder::ExtendVectorElements<OpSize::i8Bit, OpSize::i16Bit, false>},
    {OPD(2, 0b01, 0x31), 1, &OpDispatchBuilder::ExtendVectorElements<OpSize::i8Bit, OpSize::i32Bit, false>},
    {OPD(2, 0b01, 0x32), 1, &OpDispatchBuilder::ExtendVectorElements<OpSize::i8Bit, OpSize::i64Bit, false>},
    {OPD(2, 0b01, 0x33), 1, &OpDispatchBuilder::ExtendVectorElements<OpSize::i16Bit, OpSize::i32Bit, false>},
    {OPD(2, 0b01, 0x34), 1, &OpDispatchBuilder::ExtendVectorElements<OpSize::i16Bit, OpSize::i64Bit, false>},
    {OPD(2, 0b01, 0x35), 1, &OpDispatchBuilder::ExtendVectorElements<OpSize::i32Bit, OpSize::i64Bit, false>},
    {OPD(2, 0b01, 0x36), 1, &OpDispatchBuilder::VPERMDOp},

    {OPD(2, 0b01, 0x37), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorALUOp, IR::OP_VCMPGT, OpSize::i64Bit>},
    {OPD(2, 0b01, 0x38), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorALUOp, IR::OP_VSMIN, OpSize::i8Bit>},
    {OPD(2, 0b01, 0x39), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorALUOp, IR::OP_VSMIN, OpSize::i32Bit>},
    {OPD(2, 0b01, 0x3A), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorALUOp, IR::OP_VUMIN, OpSize::i16Bit>},
    {OPD(2, 0b01, 0x3B), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorALUOp, IR::OP_VUMIN, OpSize::i32Bit>},
    {OPD(2, 0b01, 0x3C), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorALUOp, IR::OP_VSMAX, OpSize::i8Bit>},
    {OPD(2, 0b01, 0x3D), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorALUOp, IR::OP_VSMAX, OpSize::i32Bit>},
    {OPD(2, 0b01, 0x3E), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorALUOp, IR::OP_VUMAX, OpSize::i16Bit>},
    {OPD(2, 0b01, 0x3F), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorALUOp, IR::OP_VUMAX, OpSize::i32Bit>},

    {OPD(2, 0b01, 0x40), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorALUOp, IR::OP_VMUL, OpSize::i32Bit>},
    {OPD(2, 0b01, 0x41), 1, &OpDispatchBuilder::PHMINPOSUWOp},
    {OPD(2, 0b01, 0x45), 1, &OpDispatchBuilder::VPSRLVOp},
    {OPD(2, 0b01, 0x46), 1, &OpDispatchBuilder::VPSRAVDOp},
    {OPD(2, 0b01, 0x47), 1, &OpDispatchBuilder::VPSLLVOp},

    {OPD(2, 0b01, 0x58), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VBROADCASTOp, OpSize::i32Bit>},
    {OPD(2, 0b01, 0x59), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VBROADCASTOp, OpSize::i64Bit>},
    {OPD(2, 0b01, 0x5A), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VBROADCASTOp, OpSize::i128Bit>},

    {OPD(2, 0b01, 0x78), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VBROADCASTOp, OpSize::i8Bit>},
    {OPD(2, 0b01, 0x79), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VBROADCASTOp, OpSize::i16Bit>},

    {OPD(2, 0b01, 0x8C), 1, &OpDispatchBuilder::VPMASKMOVOp<false>},
    {OPD(2, 0b01, 0x8E), 1, &OpDispatchBuilder::VPMASKMOVOp<true>},

    {OPD(2, 0b01, 0x90), 1, &OpDispatchBuilder::VPGATHER<OpSize::i32Bit>},
    {OPD(2, 0b01, 0x91), 1, &OpDispatchBuilder::VPGATHER<OpSize::i64Bit>},
    {OPD(2, 0b01, 0x92), 1, &OpDispatchBuilder::VPGATHER<OpSize::i32Bit>},
    {OPD(2, 0b01, 0x93), 1, &OpDispatchBuilder::VPGATHER<OpSize::i64Bit>},

    {OPD(2, 0b01, 0x96), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VFMAddSubImpl, true, 1, 3, 2>},  // VFMADDSUB
    {OPD(2, 0b01, 0x97), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VFMAddSubImpl, false, 1, 3, 2>}, // VFMSUBADD

    {OPD(2, 0b01, 0x98), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VFMAImpl, IR::OP_VFMLA, false, 1, 3, 2>},  // VFMADD
    {OPD(2, 0b01, 0x99), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VFMAImpl, IR::OP_VFMLA, true, 1, 3, 2>},   // VFMADD
    {OPD(2, 0b01, 0x9A), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VFMAImpl, IR::OP_VFMLS, false, 1, 3, 2>},  // VFMSUB
    {OPD(2, 0b01, 0x9B), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VFMAImpl, IR::OP_VFMLS, true, 1, 3, 2>},   // VFMSUB
    {OPD(2, 0b01, 0x9C), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VFMAImpl, IR::OP_VFNMLA, false, 1, 3, 2>}, // VFNMADD
    {OPD(2, 0b01, 0x9D), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VFMAImpl, IR::OP_VFNMLA, true, 1, 3, 2>},  // VFNMADD
    {OPD(2, 0b01, 0x9E), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VFMAImpl, IR::OP_VFNMLS, false, 1, 3, 2>}, // VFNMSUB
    {OPD(2, 0b01, 0x9F), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VFMAImpl, IR::OP_VFNMLS, true, 1, 3, 2>},  // VFNMSUB

    {OPD(2, 0b01, 0xA8), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VFMAImpl, IR::OP_VFMLA, false, 2, 1, 3>},  // VFMADD
    {OPD(2, 0b01, 0xA9), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VFMAImpl, IR::OP_VFMLA, true, 2, 1, 3>},   // VFMADD
    {OPD(2, 0b01, 0xAA), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VFMAImpl, IR::OP_VFMLS, false, 2, 1, 3>},  // VFMSUB
    {OPD(2, 0b01, 0xAB), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VFMAImpl, IR::OP_VFMLS, true, 2, 1, 3>},   // VFMSUB
    {OPD(2, 0b01, 0xAC), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VFMAImpl, IR::OP_VFNMLA, false, 2, 1, 3>}, // VFNMADD
    {OPD(2, 0b01, 0xAD), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VFMAImpl, IR::OP_VFNMLA, true, 2, 1, 3>},  // VFNMADD
    {OPD(2, 0b01, 0xAE), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VFMAImpl, IR::OP_VFNMLS, false, 2, 1, 3>}, // VFNMSUB
    {OPD(2, 0b01, 0xAF), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VFMAImpl, IR::OP_VFNMLS, true, 2, 1, 3>},  // VFNMSUB

    {OPD(2, 0b01, 0xB8), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VFMAImpl, IR::OP_VFMLA, false, 2, 3, 1>},  // VFMADD
    {OPD(2, 0b01, 0xB9), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VFMAImpl, IR::OP_VFMLA, true, 2, 3, 1>},   // VFMADD
    {OPD(2, 0b01, 0xBA), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VFMAImpl, IR::OP_VFMLS, false, 2, 3, 1>},  // VFMSUB
    {OPD(2, 0b01, 0xBB), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VFMAImpl, IR::OP_VFMLS, true, 2, 3, 1>},   // VFMSUB
    {OPD(2, 0b01, 0xBC), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VFMAImpl, IR::OP_VFNMLA, false, 2, 3, 1>}, // VFNMADD
    {OPD(2, 0b01, 0xBD), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VFMAImpl, IR::OP_VFNMLA, true, 2, 3, 1>},  // VFNMADD
    {OPD(2, 0b01, 0xBE), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VFMAImpl, IR::OP_VFNMLS, false, 2, 3, 1>}, // VFNMSUB
    {OPD(2, 0b01, 0xBF), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VFMAImpl, IR::OP_VFNMLS, true, 2, 3, 1>},  // VFNMSUB

    {OPD(2, 0b01, 0xA6), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VFMAddSubImpl, true, 2, 1, 3>},  // VFMADDSUB
    {OPD(2, 0b01, 0xA7), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VFMAddSubImpl, false, 2, 1, 3>}, // VFMSUBADD

    {OPD(2, 0b01, 0xB6), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VFMAddSubImpl, true, 2, 3, 1>},  // VFMADDSUB
    {OPD(2, 0b01, 0xB7), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VFMAddSubImpl, false, 2, 3, 1>}, // VFMSUBADD

    {OPD(2, 0b01, 0xDB), 1, &OpDispatchBuilder::AESImcOp},
    {OPD(2, 0b01, 0xDC), 1, &OpDispatchBuilder::VAESEncOp},
    {OPD(2, 0b01, 0xDD), 1, &OpDispatchBuilder::VAESEncLastOp},
    {OPD(2, 0b01, 0xDE), 1, &OpDispatchBuilder::VAESDecOp},
    {OPD(2, 0b01, 0xDF), 1, &OpDispatchBuilder::VAESDecLastOp},

    {OPD(3, 0b01, 0x00), 1, &OpDispatchBuilder::VPERMQOp},
    {OPD(3, 0b01, 0x01), 1, &OpDispatchBuilder::VPERMQOp},
    {OPD(3, 0b01, 0x02), 1, &OpDispatchBuilder::VPBLENDDOp},
    {OPD(3, 0b01, 0x04), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VPERMILImmOp, OpSize::i32Bit>},
    {OPD(3, 0b01, 0x05), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VPERMILImmOp, OpSize::i64Bit>},
    {OPD(3, 0b01, 0x06), 1, &OpDispatchBuilder::VPERM2Op},
    {OPD(3, 0b01, 0x08), 1, &OpDispatchBuilder::AVXVectorRound<OpSize::i32Bit>},
    {OPD(3, 0b01, 0x09), 1, &OpDispatchBuilder::AVXVectorRound<OpSize::i64Bit>},
    {OPD(3, 0b01, 0x0A), 1, &OpDispatchBuilder::AVXInsertScalarRound<OpSize::i32Bit>},
    {OPD(3, 0b01, 0x0B), 1, &OpDispatchBuilder::AVXInsertScalarRound<OpSize::i64Bit>},
    {OPD(3, 0b01, 0x0C), 1, &OpDispatchBuilder::VPBLENDDOp},
    {OPD(3, 0b01, 0x0D), 1, &OpDispatchBuilder::VBLENDPDOp},
    {OPD(3, 0b01, 0x0E), 1, &OpDispatchBuilder::VPBLENDWOp},
    {OPD(3, 0b01, 0x0F), 1, &OpDispatchBuilder::VPALIGNROp},

    {OPD(3, 0b01, 0x14), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::PExtrOp, OpSize::i8Bit>},
    {OPD(3, 0b01, 0x15), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::PExtrOp, OpSize::i16Bit>},
    {OPD(3, 0b01, 0x16), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::PExtrOp, OpSize::i32Bit>},
    {OPD(3, 0b01, 0x17), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::PExtrOp, OpSize::i32Bit>},

    {OPD(3, 0b01, 0x18), 1, &OpDispatchBuilder::VINSERTOp},
    {OPD(3, 0b01, 0x19), 1, &OpDispatchBuilder::VEXTRACT128Op},
    {OPD(3, 0b01, 0x1D), 1, &OpDispatchBuilder::VCVTPS2PHOp},
    {OPD(3, 0b01, 0x20), 1, &OpDispatchBuilder::VPINSRBOp},
    {OPD(3, 0b01, 0x21), 1, &OpDispatchBuilder::VINSERTPSOp},
    {OPD(3, 0b01, 0x22), 1, &OpDispatchBuilder::VPINSRDQOp},

    {OPD(3, 0b01, 0x38), 1, &OpDispatchBuilder::VINSERTOp},
    {OPD(3, 0b01, 0x39), 1, &OpDispatchBuilder::VEXTRACT128Op},

    {OPD(3, 0b01, 0x40), 1, &OpDispatchBuilder::VDPPOp<OpSize::i32Bit>},
    {OPD(3, 0b01, 0x41), 1, &OpDispatchBuilder::VDPPOp<OpSize::i64Bit>},
    {OPD(3, 0b01, 0x42), 1, &OpDispatchBuilder::VMPSADBWOp},
    {OPD(3, 0b01, 0x44), 1, &OpDispatchBuilder::VPCLMULQDQOp},

    {OPD(3, 0b01, 0x46), 1, &OpDispatchBuilder::VPERM2Op},

    {OPD(3, 0b01, 0x4A), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorVariableBlend, OpSize::i32Bit>},
    {OPD(3, 0b01, 0x4B), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorVariableBlend, OpSize::i64Bit>},
    {OPD(3, 0b01, 0x4C), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVXVectorVariableBlend, OpSize::i8Bit>},

    {OPD(3, 0b01, 0x60), 1, &OpDispatchBuilder::VPCMPESTRMOp},
    {OPD(3, 0b01, 0x61), 1, &OpDispatchBuilder::VPCMPESTRIOp},
    {OPD(3, 0b01, 0x62), 1, &OpDispatchBuilder::VPCMPISTRMOp},
    {OPD(3, 0b01, 0x63), 1, &OpDispatchBuilder::VPCMPISTRIOp},

    {OPD(3, 0b01, 0xDF), 1, &OpDispatchBuilder::AESKeyGenAssist},
  };
#undef OPD

#define OPD(group, pp, opcode) (((group - X86Tables::TYPE_VEX_GROUP_12) << 4) | (pp << 3) | (opcode))
  constexpr DispatchTableEntry TableGroupOps[] {
    {OPD(X86Tables::TYPE_VEX_GROUP_12, 1, 0b010), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VPSRLIOp, OpSize::i16Bit>},
    {OPD(X86Tables::TYPE_VEX_GROUP_12, 1, 0b110), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VPSLLIOp, OpSize::i16Bit>},
    {OPD(X86Tables::TYPE_VEX_GROUP_12, 1, 0b100), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VPSRAIOp, OpSize::i16Bit>},

    {OPD(X86Tables::TYPE_VEX_GROUP_13, 1, 0b010), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VPSRLIOp, OpSize::i32Bit>},
    {OPD(X86Tables::TYPE_VEX_GROUP_13, 1, 0b110), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VPSLLIOp, OpSize::i32Bit>},
    {OPD(X86Tables::TYPE_VEX_GROUP_13, 1, 0b100), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VPSRAIOp, OpSize::i32Bit>},

    {OPD(X86Tables::TYPE_VEX_GROUP_14, 1, 0b010), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VPSRLIOp, OpSize::i64Bit>},
    {OPD(X86Tables::TYPE_VEX_GROUP_14, 1, 0b011), 1, &OpDispatchBuilder::VPSRLDQOp},
    {OPD(X86Tables::TYPE_VEX_GROUP_14, 1, 0b110), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VPSLLIOp, OpSize::i64Bit>},
    {OPD(X86Tables::TYPE_VEX_GROUP_14, 1, 0b111), 1, &OpDispatchBuilder::VPSLLDQOp},

    {OPD(X86Tables::TYPE_VEX_GROUP_15, 0, 0b010), 1, &OpDispatchBuilder::LDMXCSR},
    {OPD(X86Tables::TYPE_VEX_GROUP_15, 0, 0b011), 1, &OpDispatchBuilder::STMXCSR},
  };
#undef OPD
}

auto BaseTableLambda = [](const auto RuntimeTable) consteval {
  std::array<X86InstInfo, MAX_VEX_TABLE_SIZE> Table{};
#define OPD(map_select, pp, opcode) (((map_select - 1) << 10) | (pp << 8) | (opcode))
  constexpr U16U8InfoStruct VEXTable[] = {
    // Map 0 (Reserved)
    // VEX Map 1
    {OPD(1, 0b00, 0x10), 1, X86InstInfo{"VMOVUPS",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0x10), 1, X86InstInfo{"VMOVUPD",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b10, 0x10), 1, X86InstInfo{"VMOVSS",    TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS | FLAGS_VEX_L_IGNORE, 0}},
    {OPD(1, 0b11, 0x10), 1, X86InstInfo{"VMOVSD",    TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS | FLAGS_VEX_L_IGNORE, 0}},

    {OPD(1, 0b00, 0x11), 1, X86InstInfo{"VMOVUPS",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0x11), 1, X86InstInfo{"VMOVUPD",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b10, 0x11), 1, X86InstInfo{"VMOVSS",    TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS | FLAGS_VEX_L_IGNORE, 0}},
    {OPD(1, 0b11, 0x11), 1, X86InstInfo{"VMOVSD",    TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS | FLAGS_VEX_L_IGNORE, 0}},

    {OPD(1, 0b00, 0x12), 1, X86InstInfo{"VMOVLPS",TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_VEX_1ST_SRC | FLAGS_VEX_L_0, 0}},
    {OPD(1, 0b01, 0x12), 1, X86InstInfo{"VMOVLPD",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MOD_MEM_ONLY | FLAGS_VEX_1ST_SRC | FLAGS_VEX_L_0, 0}},
    {OPD(1, 0b10, 0x12), 1, X86InstInfo{"VMOVSLDUP", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b11, 0x12), 1, X86InstInfo{"VMOVDDUP",  TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0}},

    {OPD(1, 0b00, 0x13), 1, X86InstInfo{"VMOVLPS",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS | FLAGS_VEX_L_0, 0}},
    {OPD(1, 0b01, 0x13), 1, X86InstInfo{"VMOVLPD",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS | FLAGS_VEX_L_0, 0}},

    {OPD(1, 0b00, 0x14), 1, X86InstInfo{"VUNPCKLPS", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0x14), 1, X86InstInfo{"VUNPCKLPD", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},

    {OPD(1, 0b00, 0x15), 1, X86InstInfo{"VUNPCKHPS", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0x15), 1, X86InstInfo{"VUNPCKHPD", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},

    {OPD(1, 0b00, 0x16), 1, X86InstInfo{"VMOV(L)HPS",TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_VEX_1ST_SRC | FLAGS_VEX_L_0, 0}},
    {OPD(1, 0b01, 0x16), 1, X86InstInfo{"VMOVHPD",   TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_64BIT) | FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY | FLAGS_XMM_FLAGS | FLAGS_VEX_1ST_SRC | FLAGS_VEX_L_0, 0}},
    {OPD(1, 0b10, 0x16), 1, X86InstInfo{"VMOVSHDUP", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0}},

    {OPD(1, 0b00, 0x17), 1, X86InstInfo{"VMOVHPS",   TYPE_INST, GenFlagsSizes(SIZE_64BIT, SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS | FLAGS_VEX_L_0, 0}},
    {OPD(1, 0b01, 0x17), 1, X86InstInfo{"VMOVHPD",   TYPE_INST, GenFlagsSizes(SIZE_64BIT, SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS | FLAGS_VEX_L_0, 0}},

    {OPD(1, 0b00, 0x50), 1, X86InstInfo{"VMOVMSKPS", TYPE_INST, GenFlagsSizes(SIZE_32BIT, SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_REG_ONLY | FLAGS_XMM_FLAGS | FLAGS_SF_DST_GPR, 0}},
    {OPD(1, 0b01, 0x50), 1, X86InstInfo{"VMOVMSKPD", TYPE_INST, GenFlagsSizes(SIZE_32BIT, SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_REG_ONLY | FLAGS_XMM_FLAGS | FLAGS_SF_DST_GPR, 0}},

    {OPD(1, 0b00, 0x51), 1, X86InstInfo{"VSQRTPS",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0x51), 1, X86InstInfo{"VSQRTPD",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b10, 0x51), 1, X86InstInfo{"VSQRTSS",   TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_32BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS | FLAGS_VEX_L_IGNORE, 0}},
    {OPD(1, 0b11, 0x51), 1, X86InstInfo{"VSQRTSD",   TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_64BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS | FLAGS_VEX_L_IGNORE, 0}},

    {OPD(1, 0b00, 0x52), 1, X86InstInfo{"VRSQRTPS",  TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b10, 0x52), 1, X86InstInfo{"VRSQRTSS",  TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_32BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS | FLAGS_VEX_L_IGNORE, 0}},

    {OPD(1, 0b00, 0x53), 1, X86InstInfo{"VRCPPS",    TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b10, 0x53), 1, X86InstInfo{"VRCPSS",    TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_32BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS | FLAGS_VEX_L_IGNORE, 0}},

    {OPD(1, 0b00, 0x54), 1, X86InstInfo{"VANDPS",    TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0x54), 1, X86InstInfo{"VANDPD",    TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},

    {OPD(1, 0b00, 0x55), 1, X86InstInfo{"VANDNPS",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0x55), 1, X86InstInfo{"VANDNPD",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},

    {OPD(1, 0b00, 0x56), 1, X86InstInfo{"VORPS",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0x56), 1, X86InstInfo{"VORPD",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},

    {OPD(1, 0b00, 0x57), 1, X86InstInfo{"VXORPS",    TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0x57), 1, X86InstInfo{"VXORPD",    TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},

    {OPD(1, 0b01, 0x60), 1, X86InstInfo{"VPUNPCKLBW", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0x61), 1, X86InstInfo{"VPUNPCKLWD", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0x62), 1, X86InstInfo{"VPUNPCKLDQ", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0x63), 1, X86InstInfo{"VPACKSSWB",  TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0x64), 1, X86InstInfo{"VPCMPGTB",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0x65), 1, X86InstInfo{"VPCMPGTW",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0x66), 1, X86InstInfo{"VPCMPGTD",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0x67), 1, X86InstInfo{"VPACKUSWB",  TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},

    {OPD(1, 0b01, 0x70), 1, X86InstInfo{"VPSHUFD",    TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 1}},
    {OPD(1, 0b10, 0x70), 1, X86InstInfo{"VPSHUFHW",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 1}},
    {OPD(1, 0b11, 0x70), 1, X86InstInfo{"VPSHUFLW",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 1}},

    {OPD(1, 0b01, 0x71), 1, X86InstInfo{"",           TYPE_VEX_GROUP_12, FLAGS_NONE, 0}}, // VEX Group 12
    {OPD(1, 0b01, 0x72), 1, X86InstInfo{"",           TYPE_VEX_GROUP_13, FLAGS_NONE, 0}}, // VEX Group 13
    {OPD(1, 0b01, 0x73), 1, X86InstInfo{"",           TYPE_VEX_GROUP_14, FLAGS_NONE, 0}}, // VEX Group 14

    {OPD(1, 0b01, 0x74), 1, X86InstInfo{"VPCMPEQB",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0x75), 1, X86InstInfo{"VPCMPEQW",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0x76), 1, X86InstInfo{"VPCMPEQD",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},

    {OPD(1, 0b00, 0x77), 1, X86InstInfo{"VZERO*",     TYPE_INST, GenFlagsDstSize(SIZE_128BIT), 0}},

    {OPD(1, 0b00, 0xC2), 1, X86InstInfo{"VCMPccPS",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 1}},
    {OPD(1, 0b01, 0xC2), 1, X86InstInfo{"VCMPccPD",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 1}},
    {OPD(1, 0b10, 0xC2), 1, X86InstInfo{"VCMPccSS",   TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_32BIT) | FLAGS_MODRM | FLAGS_VEX_L_IGNORE | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 1}},
    {OPD(1, 0b11, 0xC2), 1, X86InstInfo{"VCMPccSD",   TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_64BIT) | FLAGS_MODRM | FLAGS_VEX_L_IGNORE | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 1}},

    {OPD(1, 0b01, 0xC4), 1, X86InstInfo{"VPINSRW",    TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_16BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_SF_SRC_GPR | FLAGS_XMM_FLAGS | FLAGS_VEX_L_0, 1}},
    {OPD(1, 0b01, 0xC5), 1, X86InstInfo{"VPEXTRW",    TYPE_INST, GenFlagsSizes(SIZE_32BIT, SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_REG_ONLY | FLAGS_SF_DST_GPR | FLAGS_XMM_FLAGS | FLAGS_VEX_L_0, 1}},

    {OPD(1, 0b00, 0xC6), 1, X86InstInfo{"VSHUFPS",    TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 1}},
    {OPD(1, 0b01, 0xC6), 1, X86InstInfo{"VSHUFPD",    TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 1}},

    // The above ops are defined from `Table A-17. VEX Opcode Map 1, Low Nibble = [0h:7h]` of AMD Architecture programmer's manual Volume 3
    // This table doesn't state which VEX.pp is for which instruction
    // XXX: Confirm all the above encoding opcodes

    {OPD(1, 0b00, 0x28), 1, X86InstInfo{"VMOVAPS",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0x28), 1, X86InstInfo{"VMOVAPD",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0}},

    {OPD(1, 0b00, 0x29), 1, X86InstInfo{"VMOVAPS",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0x29), 1, X86InstInfo{"VMOVAPD",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS, 0}},

    {OPD(1, 0b10, 0x2A), 1, X86InstInfo{"VCVTSI2SS",   TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_32BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS | FLAGS_SF_SRC_GPR | FLAGS_VEX_L_IGNORE, 0}},
    {OPD(1, 0b11, 0x2A), 1, X86InstInfo{"VCVTSI2SD",   TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_32BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS | FLAGS_SF_SRC_GPR | FLAGS_VEX_L_IGNORE, 0}},

    {OPD(1, 0b00, 0x2B), 1, X86InstInfo{"VMOVNTPS",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0x2B), 1, X86InstInfo{"VMOVNTPD",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS, 0}},

    {OPD(1, 0b10, 0x2C), 1, X86InstInfo{"VCVTTSS2SI",   TYPE_INST, FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_DST_GPR | FLAGS_VEX_L_IGNORE, 0}},
    {OPD(1, 0b11, 0x2C), 1, X86InstInfo{"VCVTTSD2SI",   TYPE_INST, FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_DST_GPR | FLAGS_VEX_L_IGNORE, 0}},

    {OPD(1, 0b10, 0x2D), 1, X86InstInfo{"VCVTSS2SI",   TYPE_INST, FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_DST_GPR | FLAGS_VEX_L_IGNORE, 0}},
    {OPD(1, 0b11, 0x2D), 1, X86InstInfo{"VCVTSD2SI",   TYPE_INST, FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_DST_GPR | FLAGS_VEX_L_IGNORE, 0}},

    {OPD(1, 0b00, 0x2E), 1, X86InstInfo{"VUCOMISS",   TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_32BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_VEX_L_IGNORE, 0}},
    {OPD(1, 0b01, 0x2E), 1, X86InstInfo{"VUCOMISD",   TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_VEX_L_IGNORE, 0}},

    {OPD(1, 0b00, 0x2F), 1, X86InstInfo{"VCOMISS",   TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_32BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_VEX_L_IGNORE, 0}},
    {OPD(1, 0b01, 0x2F), 1, X86InstInfo{"VCOMISD",   TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_VEX_L_IGNORE, 0}},

    {OPD(1, 0b00, 0x58), 1, X86InstInfo{"VADDPS",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0x58), 1, X86InstInfo{"VADDPD",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b10, 0x58), 1, X86InstInfo{"VADDSS",   TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_32BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS | FLAGS_VEX_L_IGNORE, 0}},
    {OPD(1, 0b11, 0x58), 1, X86InstInfo{"VADDSD",   TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_64BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS | FLAGS_VEX_L_IGNORE, 0}},

    {OPD(1, 0b00, 0x59), 1, X86InstInfo{"VMULPS",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0x59), 1, X86InstInfo{"VMULPD",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b10, 0x59), 1, X86InstInfo{"VMULSS",   TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_32BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS | FLAGS_VEX_L_IGNORE, 0}},
    {OPD(1, 0b11, 0x59), 1, X86InstInfo{"VMULSD",   TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_64BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS | FLAGS_VEX_L_IGNORE, 0}},

    {OPD(1, 0b00, 0x5A), 1, X86InstInfo{"VCVTPS2PD",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0x5A), 1, X86InstInfo{"VCVTPD2PS",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b10, 0x5A), 1, X86InstInfo{"VCVTSS2SD",   TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_32BIT) | FLAGS_MODRM | FLAGS_VEX_L_IGNORE | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b11, 0x5A), 1, X86InstInfo{"VCVTSD2SS",   TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_64BIT) | FLAGS_MODRM | FLAGS_VEX_L_IGNORE |FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},

    {OPD(1, 0b00, 0x5B), 1, X86InstInfo{"VCVTDQ2PS",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0x5B), 1, X86InstInfo{"VCVTPS2DQ",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b10, 0x5B), 1, X86InstInfo{"VCVTTPS2DQ",  TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0}},

    {OPD(1, 0b00, 0x5C), 1, X86InstInfo{"VSUBPS",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0x5C), 1, X86InstInfo{"VSUBPD",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b10, 0x5C), 1, X86InstInfo{"VSUBSS",   TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_32BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS | FLAGS_VEX_L_IGNORE, 0}},
    {OPD(1, 0b11, 0x5C), 1, X86InstInfo{"VSUBSD",   TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_64BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS | FLAGS_VEX_L_IGNORE, 0}},

    {OPD(1, 0b00, 0x5D), 1, X86InstInfo{"VMINPS",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0x5D), 1, X86InstInfo{"VMINPD",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b10, 0x5D), 1, X86InstInfo{"VMINSS",   TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_32BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS | FLAGS_VEX_L_IGNORE, 0}},
    {OPD(1, 0b11, 0x5D), 1, X86InstInfo{"VMINSD",   TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_64BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS | FLAGS_VEX_L_IGNORE, 0}},

    {OPD(1, 0b00, 0x5E), 1, X86InstInfo{"VDIVPS",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0x5E), 1, X86InstInfo{"VDIVPD",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b10, 0x5E), 1, X86InstInfo{"VDIVSS",   TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_32BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS | FLAGS_VEX_L_IGNORE, 0}},
    {OPD(1, 0b11, 0x5E), 1, X86InstInfo{"VDIVSD",   TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_64BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS | FLAGS_VEX_L_IGNORE, 0}},

    {OPD(1, 0b00, 0x5F), 1, X86InstInfo{"VMAXPS",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0x5F), 1, X86InstInfo{"VMAXPD",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b10, 0x5F), 1, X86InstInfo{"VMAXSS",   TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_32BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS | FLAGS_VEX_L_IGNORE, 0}},
    {OPD(1, 0b11, 0x5F), 1, X86InstInfo{"VMAXSD",   TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_64BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS | FLAGS_VEX_L_IGNORE, 0}},

    {OPD(1, 0b01, 0x68), 1, X86InstInfo{"VPUNPCKHBW",  TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0x69), 1, X86InstInfo{"VPUNPCKHWD",  TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0x6A), 1, X86InstInfo{"VPUNPCKHDQ",  TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0x6B), 1, X86InstInfo{"VPACKSSDW",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0x6C), 1, X86InstInfo{"VPUNPCKLQDQ", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0x6D), 1, X86InstInfo{"VPUNPCKHQDQ", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0x6E), 1, X86InstInfo{"VMOV*",       TYPE_INST, GenFlagsDstSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_VEX_L_0 | FLAGS_SF_SRC_GPR, 0}},

    {OPD(1, 0b01, 0x6F), 1, X86InstInfo{"VMOVDQA",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b10, 0x6F), 1, X86InstInfo{"VMOVDQU",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0}},

    {OPD(1, 0b01, 0x7C), 1, X86InstInfo{"VHADDPD",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b11, 0x7C), 1, X86InstInfo{"VHADDPS",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},

    {OPD(1, 0b01, 0x7D), 1, X86InstInfo{"VHSUBPD",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b11, 0x7D), 1, X86InstInfo{"VHSUBPS",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},

    {OPD(1, 0b01, 0x7E), 1, X86InstInfo{"VMOV*",     TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_DST_GPR | FLAGS_VEX_L_0 | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b10, 0x7E), 1, X86InstInfo{"VMOVQ",     TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_VEX_L_0 | FLAGS_XMM_FLAGS, 0}},

    {OPD(1, 0b01, 0x7F), 1, X86InstInfo{"VMOVDQA",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b10, 0x7F), 1, X86InstInfo{"VMOVDQU",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS, 0}},

    {OPD(1, 0b00, 0xAE), 1, X86InstInfo{"",     TYPE_VEX_GROUP_15, FLAGS_NONE, 0}}, // VEX Group 15
    {OPD(1, 0b01, 0xAE), 1, X86InstInfo{"",     TYPE_VEX_GROUP_15, FLAGS_NONE, 0}}, // VEX Group 15
    {OPD(1, 0b10, 0xAE), 1, X86InstInfo{"",     TYPE_VEX_GROUP_15, FLAGS_NONE, 0}}, // VEX Group 15
    {OPD(1, 0b11, 0xAE), 1, X86InstInfo{"",     TYPE_VEX_GROUP_15, FLAGS_NONE, 0}}, // VEX Group 15

    {OPD(1, 0b01, 0xD0), 1, X86InstInfo{"VADDSUBPD",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b11, 0xD0), 1, X86InstInfo{"VADDSUBPS",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},

    {OPD(1, 0b01, 0xD1), 1, X86InstInfo{"VPSRLW",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0xD2), 1, X86InstInfo{"VPSRLD",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0xD3), 1, X86InstInfo{"VPSRLQ",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0xD4), 1, X86InstInfo{"VPADDQ",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0xD5), 1, X86InstInfo{"VPMULLW",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0xD6), 1, X86InstInfo{"VMOVQ",       TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_VEX_L_0 | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0xD7), 1, X86InstInfo{"VPMOVMSKB",   TYPE_INST, GenFlagsSizes(SIZE_32BIT, SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_DST_GPR | FLAGS_SF_MOD_REG_ONLY, 0}},

    {OPD(1, 0b01, 0xD8), 1, X86InstInfo{"VPSUBUSB", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0xD9), 1, X86InstInfo{"VPSUBUSW", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0xDA), 1, X86InstInfo{"VPMINUB",  TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0xDB), 1, X86InstInfo{"VPAND",    TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0xDC), 1, X86InstInfo{"VPADDUSB", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0xDD), 1, X86InstInfo{"VPADDUSW", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0xDE), 1, X86InstInfo{"VPMAXUB",  TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0xDF), 1, X86InstInfo{"VPANDN",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},

    {OPD(1, 0b01, 0xE0), 1, X86InstInfo{"VPAVGB",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0xE1), 1, X86InstInfo{"VPSRAW",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0xE2), 1, X86InstInfo{"VPSRAD",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0xE3), 1, X86InstInfo{"VPAVGW",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0xE4), 1, X86InstInfo{"VPMULHUW",    TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0xE5), 1, X86InstInfo{"VPMULHW",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},

    {OPD(1, 0b01, 0xE6), 1, X86InstInfo{"VCVTTPD2DQ",  TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b10, 0xE6), 1, X86InstInfo{"VCVTDQ2PD",   TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b11, 0xE6), 1, X86InstInfo{"VCVTPD2DQ",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0}},

    {OPD(1, 0b01, 0xE7), 1, X86InstInfo{"VMOVNTDQ",    TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_MOD_MEM_ONLY | FLAGS_XMM_FLAGS, 0}},

    {OPD(1, 0b01, 0xE8), 1, X86InstInfo{"VPSUBSB", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0xE9), 1, X86InstInfo{"VPSUBSW", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0xEA), 1, X86InstInfo{"VPMINSW", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0xEB), 1, X86InstInfo{"VPOR",    TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0xEC), 1, X86InstInfo{"VPADDSB", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0xED), 1, X86InstInfo{"VPADDSW", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0xEE), 1, X86InstInfo{"VPMAXSW", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0xEF), 1, X86InstInfo{"VPXOR",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},

    {OPD(1, 0b11, 0xF0), 1, X86InstInfo{"VLDDQU",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY | FLAGS_XMM_FLAGS, 0}},

    {OPD(1, 0b01, 0xF1), 1, X86InstInfo{"VPSLLW",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0xF2), 1, X86InstInfo{"VPSLLD",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0xF3), 1, X86InstInfo{"VPSLLQ",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0xF4), 1, X86InstInfo{"VPMULUDQ",    TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0xF5), 1, X86InstInfo{"VPMADDWD",    TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0xF6), 1, X86InstInfo{"VPSADBW",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0xF7), 1, X86InstInfo{"VMASKMOVDQU", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_REG_ONLY | FLAGS_XMM_FLAGS | FLAGS_VEX_L_0, 0}},

    {OPD(1, 0b01, 0xF8), 1, X86InstInfo{"VPSUBB", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0xF9), 1, X86InstInfo{"VPSUBW", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0xFA), 1, X86InstInfo{"VPSUBD", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0xFB), 1, X86InstInfo{"VPSUBQ", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0xFC), 1, X86InstInfo{"VPADDB", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0xFD), 1, X86InstInfo{"VPADDW", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(1, 0b01, 0xFE), 1, X86InstInfo{"VPADDD", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},

    // VEX Map 2
    {OPD(2, 0b01, 0x00), 1, X86InstInfo{"VPSHUFB", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0x01), 1, X86InstInfo{"VPHADDW", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0x02), 1, X86InstInfo{"VPHADDD", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0x03), 1, X86InstInfo{"VPHADDSW", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0x04), 1, X86InstInfo{"VPMADDUBSW", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0x05), 1, X86InstInfo{"VPHSUBW", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0x06), 1, X86InstInfo{"VPHSUBD", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0x07), 1, X86InstInfo{"VPHSUBSW", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},

    {OPD(2, 0b01, 0x08), 1, X86InstInfo{"VPSIGNB", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0x09), 1, X86InstInfo{"VPSIGNW", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0x0A), 1, X86InstInfo{"VPSIGND", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0x0B), 1, X86InstInfo{"VPMULHRSW", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0x0C), 1, X86InstInfo{"VPERMILPS", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_REX_W_0 | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0x0D), 1, X86InstInfo{"VPERMILPD", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_REX_W_0 | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0x0E), 1, X86InstInfo{"VTESTPS", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_REX_W_0 | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0x0F), 1, X86InstInfo{"VTESTPD", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_REX_W_0 | FLAGS_XMM_FLAGS, 0}},

    {OPD(2, 0b01, 0x13), 1, X86InstInfo{"VCVTPH2PS", TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_64BIT) | FLAGS_MODRM | FLAGS_REX_W_0 | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0x16), 1, X86InstInfo{"VPERMPS", TYPE_INST, GenFlagsSameSize(SIZE_256BIT) | FLAGS_MODRM | FLAGS_VEX_L_1 | FLAGS_VEX_1ST_SRC | FLAGS_REX_W_0 | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0x17), 1, X86InstInfo{"VPTEST", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0}},

    {OPD(2, 0b01, 0x18), 1, X86InstInfo{"VBROADCASTSS", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_REX_W_0 | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0x19), 1, X86InstInfo{"VBROADCASTSD", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_L_1 | FLAGS_REX_W_0 | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0x1A), 1, X86InstInfo{"VBROADCASTF128", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_L_1 | FLAGS_SF_MOD_MEM_ONLY | FLAGS_REX_W_0 | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0x1C), 1, X86InstInfo{"VPABSB", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0x1D), 1, X86InstInfo{"VPABSW", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0x1E), 1, X86InstInfo{"VPABSD", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0}},

    {OPD(2, 0b01, 0x20), 1, X86InstInfo{"VPMOVSXBW", TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0x21), 1, X86InstInfo{"VPMOVSXBD", TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_32BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0x22), 1, X86InstInfo{"VPMOVSXBQ", TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_16BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0x23), 1, X86InstInfo{"VPMOVSXWD", TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0x24), 1, X86InstInfo{"VPMOVSXWQ", TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_32BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0x25), 1, X86InstInfo{"VPMOVSXDQ", TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0}},

    {OPD(2, 0b01, 0x28), 1, X86InstInfo{"VPMULDQ", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0x29), 1, X86InstInfo{"VPCMPEQQ", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0x2A), 1, X86InstInfo{"VMOVNTDQA", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0x2B), 1, X86InstInfo{"VPACKUSDW", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0x2C), 1, X86InstInfo{"VMASKMOVPS", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY | FLAGS_VEX_1ST_SRC | FLAGS_REX_W_0 | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0x2D), 1, X86InstInfo{"VMASKMOVPD", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY | FLAGS_VEX_1ST_SRC | FLAGS_REX_W_0 | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0x2E), 1, X86InstInfo{"VMASKMOVPS", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY | FLAGS_SF_MOD_DST | FLAGS_VEX_1ST_SRC | FLAGS_REX_W_0 | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0x2F), 1, X86InstInfo{"VMASKMOVPD", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY | FLAGS_SF_MOD_DST | FLAGS_VEX_1ST_SRC | FLAGS_REX_W_0 | FLAGS_XMM_FLAGS, 0}},

    {OPD(2, 0b01, 0x30), 1, X86InstInfo{"VPMOVZXBW", TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0x31), 1, X86InstInfo{"VPMOVZXBD", TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_32BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0x32), 1, X86InstInfo{"VPMOVZXBQ", TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_16BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0x33), 1, X86InstInfo{"VPMOVZXWD", TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0x34), 1, X86InstInfo{"VPMOVZXWQ", TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_32BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0x35), 1, X86InstInfo{"VPMOVZXDQ", TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0x36), 1, X86InstInfo{"VPERMD", TYPE_INST, GenFlagsSameSize(SIZE_256BIT) | FLAGS_MODRM | FLAGS_VEX_L_1 | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0x37), 1, X86InstInfo{"VPCMPGTQ", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},

    {OPD(2, 0b01, 0x38), 1, X86InstInfo{"VPMINSB", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0x39), 1, X86InstInfo{"VPMINSD", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0x3A), 1, X86InstInfo{"VPMINUW", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0x3B), 1, X86InstInfo{"VPMINUD", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0x3C), 1, X86InstInfo{"VPMAXSB", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0x3D), 1, X86InstInfo{"VPMAXSD", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0x3E), 1, X86InstInfo{"VPMAXUW", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0x3F), 1, X86InstInfo{"VPMAXUD", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},

    {OPD(2, 0b01, 0x40), 1, X86InstInfo{"VPMULLD", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0x41), 1, X86InstInfo{"VPHMINPOSUW", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_VEX_L_0, 0}},
    {OPD(2, 0b01, 0x45), 1, X86InstInfo{"VPSRLV", TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_32BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0x46), 1, X86InstInfo{"VPSRAVD", TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_32BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_REX_W_0 | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0x47), 1, X86InstInfo{"VPSLLV", TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_32BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},

    {OPD(2, 0b01, 0x58), 1, X86InstInfo{"VPBROADCASTD", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_REX_W_0 | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0x59), 1, X86InstInfo{"VPBROADCASTQ", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_REX_W_0 | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0x5A), 1, X86InstInfo{"VBROADCASTI128", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_L_1 | FLAGS_SF_MOD_MEM_ONLY | FLAGS_REX_W_0 | FLAGS_XMM_FLAGS, 0}},

    {OPD(2, 0b01, 0x78), 1, X86InstInfo{"VPBROADCASTB", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_REX_W_0 | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0x79), 1, X86InstInfo{"VPBROADCASTW", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_REX_W_0 | FLAGS_XMM_FLAGS, 0}},

    {OPD(2, 0b01, 0x8C), 1, X86InstInfo{"VPMASKMOV", TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_32BIT) | FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0x8E), 1, X86InstInfo{"VPMASKMOV", TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_32BIT) | FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY | FLAGS_SF_MOD_DST | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},

    {OPD(2, 0b01, 0x90), 1, X86InstInfo{"VPGATHERDD/Q", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_2ND_SRC | FLAGS_VEX_VSIB | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0x91), 1, X86InstInfo{"VPGATHERQD/Q", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_2ND_SRC | FLAGS_VEX_VSIB | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0x92), 1, X86InstInfo{"VGATHERDPS/D", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_2ND_SRC | FLAGS_VEX_VSIB | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0x93), 1, X86InstInfo{"VGATHERQPS/D", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_2ND_SRC | FLAGS_VEX_VSIB | FLAGS_XMM_FLAGS, 0}},

    {OPD(2, 0b01, 0x96), 1, X86InstInfo{"VFMADDSUB132", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0x97), 1, X86InstInfo{"VFMSUBADD132", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},

    {OPD(2, 0b01, 0x98), 1, X86InstInfo{"VFMADD132", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0x99), 1, X86InstInfo{"VFMADD132_S", TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_32BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS | FLAGS_VEX_L_IGNORE, 0}},
    {OPD(2, 0b01, 0x9A), 1, X86InstInfo{"VFMSUB132", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0x9B), 1, X86InstInfo{"VFMSUB132_S", TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_32BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS | FLAGS_VEX_L_IGNORE, 0}},
    {OPD(2, 0b01, 0x9C), 1, X86InstInfo{"VFNMADD132", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0x9D), 1, X86InstInfo{"VFNMADD132_S", TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_32BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS | FLAGS_VEX_L_IGNORE, 0}},
    {OPD(2, 0b01, 0x9E), 1, X86InstInfo{"VFNMSUB132", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0x9F), 1, X86InstInfo{"VFNMSUB132_S", TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_32BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS | FLAGS_VEX_L_IGNORE, 0}},

    {OPD(2, 0b01, 0xA8), 1, X86InstInfo{"VFMADD213", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0xA9), 1, X86InstInfo{"VFMADD213_S", TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_32BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS | FLAGS_VEX_L_IGNORE, 0}},
    {OPD(2, 0b01, 0xAA), 1, X86InstInfo{"VFMSUB213", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0xAB), 1, X86InstInfo{"VFMSUB213_S", TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_32BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS | FLAGS_VEX_L_IGNORE, 0}},
    {OPD(2, 0b01, 0xAC), 1, X86InstInfo{"VFNMADD213", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0xAD), 1, X86InstInfo{"VFNMADD213_S", TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_32BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS | FLAGS_VEX_L_IGNORE, 0}},
    {OPD(2, 0b01, 0xAE), 1, X86InstInfo{"VFNMSUB213", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0xAF), 1, X86InstInfo{"VFNMSUB213_S", TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_32BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS | FLAGS_VEX_L_IGNORE, 0}},

    {OPD(2, 0b01, 0xB8), 1, X86InstInfo{"VFMADD231", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0xB9), 1, X86InstInfo{"VFMADD231_S", TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_32BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS | FLAGS_VEX_L_IGNORE, 0}},
    {OPD(2, 0b01, 0xBA), 1, X86InstInfo{"VFMSUB231", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0xBB), 1, X86InstInfo{"VFMSUB231_S", TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_32BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS | FLAGS_VEX_L_IGNORE, 0}},
    {OPD(2, 0b01, 0xBC), 1, X86InstInfo{"VFNMADD231", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0xBD), 1, X86InstInfo{"VFNMADD231_S", TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_32BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS | FLAGS_VEX_L_IGNORE, 0}},
    {OPD(2, 0b01, 0xBE), 1, X86InstInfo{"VFNMSUB231", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0xBF), 1, X86InstInfo{"VFNMSUB231_S", TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_32BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS | FLAGS_VEX_L_IGNORE, 0}},

    {OPD(2, 0b01, 0xA6), 1, X86InstInfo{"VFMADDSUB213", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0xA7), 1, X86InstInfo{"VFMSUBADD213", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},

    {OPD(2, 0b01, 0xB6), 1, X86InstInfo{"VFMADDSUB231", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0xB7), 1, X86InstInfo{"VFMSUBADD231", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},

    {OPD(2, 0b01, 0xDB), 1, X86InstInfo{"VAESIMC", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0xDC), 1, X86InstInfo{"VAESENC", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0xDD), 1, X86InstInfo{"VAESENCLAST", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0xDE), 1, X86InstInfo{"VAESDEC", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},
    {OPD(2, 0b01, 0xDF), 1, X86InstInfo{"VAESDECLAST", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 0}},

    {OPD(2, 0b00, 0xF2), 1, X86InstInfo{"ANDN", TYPE_INST, FLAGS_MODRM | FLAGS_VEX_1ST_SRC, 0}},

    {OPD(2, 0b00, 0xF3), 1, X86InstInfo{"", TYPE_VEX_GROUP_17, FLAGS_NONE, 0}}, // VEX Group 17
    {OPD(2, 0b01, 0xF3), 1, X86InstInfo{"", TYPE_VEX_GROUP_17, FLAGS_NONE, 0}}, // VEX Group 17
    {OPD(2, 0b10, 0xF3), 1, X86InstInfo{"", TYPE_VEX_GROUP_17, FLAGS_NONE, 0}}, // VEX Group 17
    {OPD(2, 0b11, 0xF3), 1, X86InstInfo{"", TYPE_VEX_GROUP_17, FLAGS_NONE, 0}}, // VEX Group 17

    {OPD(2, 0b00, 0xF5), 1, X86InstInfo{"BZHI", TYPE_INST, FLAGS_MODRM | FLAGS_VEX_L_0 | FLAGS_VEX_2ND_SRC, 0}},
    // AMD reference manual is incorrect. PEXT actually maps to 0b10, not 0b01.
    {OPD(2, 0b10, 0xF5), 1, X86InstInfo{"PEXT", TYPE_INST, FLAGS_MODRM | FLAGS_VEX_L_0 | FLAGS_VEX_1ST_SRC, 0}},
    {OPD(2, 0b11, 0xF5), 1, X86InstInfo{"PDEP", TYPE_INST, FLAGS_MODRM | FLAGS_VEX_L_0 | FLAGS_VEX_1ST_SRC, 0}},

    {OPD(2, 0b11, 0xF6), 1, X86InstInfo{"MULX", TYPE_INST, FLAGS_MODRM | FLAGS_VEX_L_0 | FLAGS_VEX_1ST_SRC, 0}},

    {OPD(2, 0b00, 0xF7), 1, X86InstInfo{"BEXTR", TYPE_INST, FLAGS_MODRM | FLAGS_VEX_L_0 | FLAGS_VEX_2ND_SRC, 0}},
    {OPD(2, 0b01, 0xF7), 1, X86InstInfo{"SHLX", TYPE_INST, FLAGS_MODRM | FLAGS_VEX_L_0 | FLAGS_VEX_2ND_SRC, 0}},
    {OPD(2, 0b10, 0xF7), 1, X86InstInfo{"SARX", TYPE_INST, FLAGS_MODRM | FLAGS_VEX_L_0 | FLAGS_VEX_2ND_SRC, 0}},
    {OPD(2, 0b11, 0xF7), 1, X86InstInfo{"SHRX", TYPE_INST, FLAGS_MODRM | FLAGS_VEX_L_0 | FLAGS_VEX_2ND_SRC, 0}},

    // VEX Map 3
    {OPD(3, 0b01, 0x00), 1, X86InstInfo{"VPERMQ", TYPE_INST, GenFlagsSameSize(SIZE_256BIT) | FLAGS_MODRM | FLAGS_VEX_L_1 | FLAGS_REX_W_1 | FLAGS_XMM_FLAGS, 1}},
    {OPD(3, 0b01, 0x01), 1, X86InstInfo{"VPERMPD", TYPE_INST, GenFlagsSameSize(SIZE_256BIT) | FLAGS_MODRM | FLAGS_VEX_L_1 | FLAGS_REX_W_1 | FLAGS_XMM_FLAGS, 1}},
    {OPD(3, 0b01, 0x02), 1, X86InstInfo{"VPBLENDD", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_REX_W_0 | FLAGS_XMM_FLAGS, 1}},
    {OPD(3, 0b01, 0x04), 1, X86InstInfo{"VPERMILPS", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_REX_W_0 | FLAGS_XMM_FLAGS, 1}},
    {OPD(3, 0b01, 0x05), 1, X86InstInfo{"VPERMILPD", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_REX_W_0 | FLAGS_XMM_FLAGS, 1}},
    {OPD(3, 0b01, 0x06), 1, X86InstInfo{"VPERM2F128", TYPE_INST, GenFlagsSameSize(SIZE_256BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_REX_W_0 | FLAGS_XMM_FLAGS | FLAGS_VEX_L_1, 1}},

    {OPD(3, 0b01, 0x08), 1, X86InstInfo{"VROUNDPS", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 1}},
    {OPD(3, 0b01, 0x09), 1, X86InstInfo{"VROUNDPD", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 1}},
    {OPD(3, 0b01, 0x0A), 1, X86InstInfo{"VROUNDSS", TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_32BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 1}},
    {OPD(3, 0b01, 0x0B), 1, X86InstInfo{"VROUNDSD", TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_64BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 1}},
    {OPD(3, 0b01, 0x0C), 1, X86InstInfo{"VBLENDPS", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 1}},
    {OPD(3, 0b01, 0x0D), 1, X86InstInfo{"VBLENDPD", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 1}},
    {OPD(3, 0b01, 0x0E), 1, X86InstInfo{"VPBLENDW", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 1}},
    {OPD(3, 0b01, 0x0F), 1, X86InstInfo{"VPALIGNR", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 1}},

    {OPD(3, 0b01, 0x14), 1, X86InstInfo{"VPEXTRB", TYPE_INST, GenFlagsSizes(SIZE_32BIT, SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_L_0 | FLAGS_SF_MOD_DST | FLAGS_SF_DST_GPR | FLAGS_XMM_FLAGS, 1}},
    {OPD(3, 0b01, 0x15), 1, X86InstInfo{"VPEXTRW", TYPE_INST, GenFlagsSizes(SIZE_16BIT, SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_L_0 | FLAGS_SF_MOD_DST | FLAGS_SF_DST_GPR | FLAGS_XMM_FLAGS, 1}},
    {OPD(3, 0b01, 0x16), 1, X86InstInfo{"VPEXTRD", TYPE_INST, GenFlagsSizes(SIZE_32BIT, SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_L_0 | FLAGS_SF_MOD_DST | FLAGS_SF_DST_GPR | FLAGS_XMM_FLAGS, 1}},
    {OPD(3, 0b01, 0x17), 1, X86InstInfo{"VEXTRACTPS", TYPE_INST, GenFlagsSizes(SIZE_32BIT, SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_L_0 | FLAGS_SF_MOD_DST | FLAGS_SF_DST_GPR | FLAGS_XMM_FLAGS, 1}},

    {OPD(3, 0b01, 0x18), 1, X86InstInfo{"VINSERTF128", TYPE_INST, GenFlagsSameSize(SIZE_256BIT) | FLAGS_MODRM | FLAGS_VEX_L_1 | FLAGS_REX_W_0 | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 1}},
    {OPD(3, 0b01, 0x19), 1, X86InstInfo{"VEXTRACTF128", TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_256BIT) | FLAGS_MODRM | FLAGS_VEX_L_1 | FLAGS_REX_W_0 | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS, 1}},
    {OPD(3, 0b01, 0x1D), 1, X86InstInfo{"VCVTPS2PH", TYPE_INST, GenFlagsSizes(SIZE_64BIT, SIZE_128BIT) | FLAGS_MODRM | FLAGS_REX_W_0 | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS, 1}},

    {OPD(3, 0b01, 0x20), 1, X86InstInfo{"VPINSRB", TYPE_INST, GenFlagsDstSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_L_0 | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS | FLAGS_SF_SRC_GPR, 1}},
    {OPD(3, 0b01, 0x21), 1, X86InstInfo{"VINSERTPS", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_L_0 | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 1}},
    {OPD(3, 0b01, 0x22), 1, X86InstInfo{"VPINSR{D,Q}", TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_32BIT) | FLAGS_MODRM | FLAGS_VEX_L_0 | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS | FLAGS_SF_SRC_GPR, 1}},

    {OPD(3, 0b01, 0x38), 1, X86InstInfo{"VINSERTI128", TYPE_INST, GenFlagsSameSize(SIZE_256BIT) | FLAGS_MODRM | FLAGS_VEX_L_1 | FLAGS_REX_W_0 | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 1}},
    {OPD(3, 0b01, 0x39), 1, X86InstInfo{"VEXTRACTI128", TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_256BIT) | FLAGS_MODRM | FLAGS_VEX_L_1 | FLAGS_REX_W_0 | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS, 1}},

    {OPD(3, 0b01, 0x40), 1, X86InstInfo{"VDPPS", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 1}},
    {OPD(3, 0b01, 0x41), 1, X86InstInfo{"VDPPD", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS | FLAGS_VEX_L_0, 1}},
    {OPD(3, 0b01, 0x42), 1, X86InstInfo{"VMPSADBW", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 1}},
    {OPD(3, 0b01, 0x44), 1, X86InstInfo{"VPCLMULQDQ", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 1}},
    {OPD(3, 0b01, 0x46), 1, X86InstInfo{"VPERM2I128", TYPE_INST, GenFlagsSameSize(SIZE_256BIT) | FLAGS_MODRM | FLAGS_VEX_L_1 | FLAGS_REX_W_0 | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 1}},

    {OPD(3, 0b01, 0x4A), 1, X86InstInfo{"VBLENDVPS", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_REX_W_0 | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 1}},
    {OPD(3, 0b01, 0x4B), 1, X86InstInfo{"VBLENDVPD", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_REX_W_0 | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 1}},
    {OPD(3, 0b01, 0x4C), 1, X86InstInfo{"VPBLENDVB", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_REX_W_0 | FLAGS_VEX_1ST_SRC | FLAGS_XMM_FLAGS, 1}},

    {OPD(3, 0b01, 0x5C), 1, X86InstInfo{"VFMADDSUBPS", TYPE_UNDEC, FLAGS_NONE, 0}}, ///< FMA4
    {OPD(3, 0b01, 0x5D), 1, X86InstInfo{"VFMADDSUBPD", TYPE_UNDEC, FLAGS_NONE, 0}}, ///< FMA4
    {OPD(3, 0b01, 0x5E), 1, X86InstInfo{"VFMSUBADDPS", TYPE_UNDEC, FLAGS_NONE, 0}}, ///< FMA4
    {OPD(3, 0b01, 0x5F), 1, X86InstInfo{"VFMSUBADDPD", TYPE_UNDEC, FLAGS_NONE, 0}}, ///< FMA4

    {OPD(3, 0b01, 0x60), 1, X86InstInfo{"VPCMPESTRM", TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_32BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_VEX_L_0, 1}},
    {OPD(3, 0b01, 0x61), 1, X86InstInfo{"VPCMPESTRI", TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_32BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_VEX_L_0, 1}},
    {OPD(3, 0b01, 0x62), 1, X86InstInfo{"VPCMPISTRM", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_VEX_L_0, 1}},
    {OPD(3, 0b01, 0x63), 1, X86InstInfo{"VPCMPISTRI", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_VEX_L_0, 1}},

    {OPD(3, 0b01, 0x68), 1, X86InstInfo{"VFMADDPS", TYPE_UNDEC, FLAGS_NONE, 0}}, ///< FMA4
    {OPD(3, 0b01, 0x69), 1, X86InstInfo{"VFMADDPD", TYPE_UNDEC, FLAGS_NONE, 0}}, ///< FMA4
    {OPD(3, 0b01, 0x6A), 1, X86InstInfo{"VFMADDSS", TYPE_UNDEC, FLAGS_NONE, 0}}, ///< FMA4
    {OPD(3, 0b01, 0x6B), 1, X86InstInfo{"VFMADDSD", TYPE_UNDEC, FLAGS_NONE, 0}}, ///< FMA4
    {OPD(3, 0b01, 0x6C), 1, X86InstInfo{"VFMSUBPS", TYPE_UNDEC, FLAGS_NONE, 0}}, ///< FMA4
    {OPD(3, 0b01, 0x6D), 1, X86InstInfo{"VFMSUBPD", TYPE_UNDEC, FLAGS_NONE, 0}}, ///< FMA4
    {OPD(3, 0b01, 0x6E), 1, X86InstInfo{"VFMSUBSS", TYPE_UNDEC, FLAGS_NONE, 0}}, ///< FMA4
    {OPD(3, 0b01, 0x6F), 1, X86InstInfo{"VFMSUBSD", TYPE_UNDEC, FLAGS_NONE, 0}}, ///< FMA4

    {OPD(3, 0b01, 0x78), 1, X86InstInfo{"VFNMADDPS", TYPE_UNDEC, FLAGS_NONE, 0}}, ///< FMA4
    {OPD(3, 0b01, 0x79), 1, X86InstInfo{"VFNMADDPD", TYPE_UNDEC, FLAGS_NONE, 0}}, ///< FMA4
    {OPD(3, 0b01, 0x7A), 1, X86InstInfo{"VFNMADDSS", TYPE_UNDEC, FLAGS_NONE, 0}}, ///< FMA4
    {OPD(3, 0b01, 0x7B), 1, X86InstInfo{"VFNMADDSD", TYPE_UNDEC, FLAGS_NONE, 0}}, ///< FMA4
    {OPD(3, 0b01, 0x7C), 1, X86InstInfo{"VFNMSUBPS", TYPE_UNDEC, FLAGS_NONE, 0}}, ///< FMA4
    {OPD(3, 0b01, 0x7D), 1, X86InstInfo{"VFNMSUBPD", TYPE_UNDEC, FLAGS_NONE, 0}}, ///< FMA4
    {OPD(3, 0b01, 0x7E), 1, X86InstInfo{"VFNMSUBSS", TYPE_UNDEC, FLAGS_NONE, 0}}, ///< FMA4
    {OPD(3, 0b01, 0x7F), 1, X86InstInfo{"VFNMSUBSD", TYPE_UNDEC, FLAGS_NONE, 0}}, ///< FMA4

    {OPD(3, 0b01, 0xDF), 1, X86InstInfo{"VAESKEYGENASSIST", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_VEX_L_0 | FLAGS_XMM_FLAGS, 1}},

    {OPD(3, 0b11, 0xF0), 1, X86InstInfo{"RORX", TYPE_INST, FLAGS_MODRM | FLAGS_VEX_L_0, 1}},

    // VEX Map 4 - 31 (Reserved)
  };
#undef OPD

  GenerateTable(&Table.at(0), VEXTable, std::size(VEXTable));

  IR::InstallToTable(Table, IR::OpDispatch_VEXTable);
  IR::InstallToTable(Table, RuntimeTable);
  return Table;
};

auto GroupTableLambda = [](const auto RuntimeTable) consteval {
  std::array<X86InstInfo, MAX_VEX_GROUP_TABLE_SIZE> Table{};

#define OPD(group, pp, opcode) (((group - TYPE_VEX_GROUP_12) << 4) | (pp << 3) | (opcode))
  constexpr U8U8InfoStruct VEXGroupTable[] = {
    {OPD(TYPE_VEX_GROUP_12, 1, 0b010), 1, X86InstInfo{"VPSRLW",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_REG_ONLY | FLAGS_VEX_DST | FLAGS_XMM_FLAGS, 1}},
    {OPD(TYPE_VEX_GROUP_12, 1, 0b100), 1, X86InstInfo{"VPSRAW",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_REG_ONLY | FLAGS_VEX_DST | FLAGS_XMM_FLAGS, 1}},
    {OPD(TYPE_VEX_GROUP_12, 1, 0b110), 1, X86InstInfo{"VPSLLW",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_REG_ONLY | FLAGS_VEX_DST | FLAGS_XMM_FLAGS, 1}},

    {OPD(TYPE_VEX_GROUP_13, 1, 0b010), 1, X86InstInfo{"VPSRLD",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_REG_ONLY | FLAGS_VEX_DST | FLAGS_XMM_FLAGS, 1}},
    {OPD(TYPE_VEX_GROUP_13, 1, 0b100), 1, X86InstInfo{"VPSRAD",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_REG_ONLY | FLAGS_VEX_DST | FLAGS_XMM_FLAGS, 1}},
    {OPD(TYPE_VEX_GROUP_13, 1, 0b110), 1, X86InstInfo{"VPSLLD",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_REG_ONLY | FLAGS_VEX_DST | FLAGS_XMM_FLAGS, 1}},

    {OPD(TYPE_VEX_GROUP_14, 1, 0b010), 1, X86InstInfo{"VPSRLQ",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_REG_ONLY | FLAGS_VEX_DST | FLAGS_XMM_FLAGS, 1}},
    {OPD(TYPE_VEX_GROUP_14, 1, 0b011), 1, X86InstInfo{"VPSRLDQ",  TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_REG_ONLY | FLAGS_VEX_DST | FLAGS_XMM_FLAGS, 1}},
    {OPD(TYPE_VEX_GROUP_14, 1, 0b110), 1, X86InstInfo{"VPSLLQ",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_REG_ONLY | FLAGS_VEX_DST | FLAGS_XMM_FLAGS, 1}},
    {OPD(TYPE_VEX_GROUP_14, 1, 0b111), 1, X86InstInfo{"VPSLLDQ",  TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_REG_ONLY | FLAGS_VEX_DST | FLAGS_XMM_FLAGS, 1}},

    {OPD(TYPE_VEX_GROUP_15, 0, 0b010), 1, X86InstInfo{"VLDMXCSR", TYPE_INST, GenFlagsSameSize(SIZE_32BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_VEX_L_0 | FLAGS_SF_MOD_MEM_ONLY, 0}},
    {OPD(TYPE_VEX_GROUP_15, 0, 0b011), 1, X86InstInfo{"VSTMXCSR", TYPE_INST, GenFlagsSameSize(SIZE_32BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_VEX_L_0 | FLAGS_SF_MOD_MEM_ONLY, 0}},

    {OPD(TYPE_VEX_GROUP_17, 0, 0b001), 1, X86InstInfo{"BLSR",     TYPE_INST, FLAGS_MODRM | FLAGS_VEX_DST, 0}},
    {OPD(TYPE_VEX_GROUP_17, 0, 0b010), 1, X86InstInfo{"BLSMSK",   TYPE_INST, FLAGS_MODRM | FLAGS_VEX_DST, 0}},
    {OPD(TYPE_VEX_GROUP_17, 0, 0b011), 1, X86InstInfo{"BLSI",     TYPE_INST, FLAGS_MODRM | FLAGS_VEX_DST, 0}},
  };
#undef OPD

  GenerateTable(&Table.at(0), VEXGroupTable, std::size(VEXGroupTable));

  IR::InstallToTable(Table, IR::OpDispatch_VEXGroupTable);
  IR::InstallToTable(Table, RuntimeTable);
  return Table;
};

const std::array<X86InstInfo, MAX_VEX_TABLE_SIZE> VEXTableOps = BaseTableLambda(std::to_array(AVX256::BaseTable));
const std::array<X86InstInfo, MAX_VEX_GROUP_TABLE_SIZE> VEXTableGroupOps = GroupTableLambda(std::to_array(AVX256::TableGroupOps));

const std::array<X86InstInfo, MAX_VEX_TABLE_SIZE> VEXTableOps_AVX128 = BaseTableLambda(std::to_array(AVX128::BaseTable));
const std::array<X86InstInfo, MAX_VEX_GROUP_TABLE_SIZE> VEXTableGroupOps_AVX128 = GroupTableLambda(std::to_array(AVX128::TableGroupOps));
}
