// SPDX-License-Identifier: MIT
/*
$info$
tags: frontend|x86-to-ir, opcodes|dispatcher-implementations
desc: Handles x86/64 AVX instructions to 128-bit IR
$end_info$
*/

#include "Interface/Core/X86Tables/X86Tables.h"

#include <FEXCore/Utils/LogManager.h>
#include "Interface/Core/OpcodeDispatcher.h"

#include <array>
#include <cstdint>
#include <tuple>
#include <utility>

namespace FEXCore::IR {
class OrderedNode;

#define OpcodeArgs [[maybe_unused]] FEXCore::X86Tables::DecodedOp Op

void OpDispatchBuilder::InstallAVX128Handlers() {
#define OPD(map_select, pp, opcode) (((map_select - 1) << 10) | (pp << 8) | (opcode))
  static constexpr std::tuple<uint16_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr> AVX128Table[] = {
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

    {OPD(1, 0b00, 0x14), 1, &OpDispatchBuilder::AVX128_VPUNPCKL<4>},
    {OPD(1, 0b01, 0x14), 1, &OpDispatchBuilder::AVX128_VPUNPCKL<8>},

    {OPD(1, 0b00, 0x15), 1, &OpDispatchBuilder::AVX128_VPUNPCKH<4>},
    {OPD(1, 0b01, 0x15), 1, &OpDispatchBuilder::AVX128_VPUNPCKH<8>},

    {OPD(1, 0b00, 0x16), 1, &OpDispatchBuilder::AVX128_VMOVHP},
    {OPD(1, 0b01, 0x16), 1, &OpDispatchBuilder::AVX128_VMOVHP},
    {OPD(1, 0b10, 0x16), 1, &OpDispatchBuilder::AVX128_VMOVSHDUP},
    {OPD(1, 0b00, 0x17), 1, &OpDispatchBuilder::AVX128_VMOVHP},
    {OPD(1, 0b01, 0x17), 1, &OpDispatchBuilder::AVX128_VMOVHP},

    {OPD(1, 0b00, 0x28), 1, &OpDispatchBuilder::AVX128_VMOVAPS},
    {OPD(1, 0b01, 0x28), 1, &OpDispatchBuilder::AVX128_VMOVAPS},
    {OPD(1, 0b00, 0x29), 1, &OpDispatchBuilder::AVX128_VMOVAPS},
    {OPD(1, 0b01, 0x29), 1, &OpDispatchBuilder::AVX128_VMOVAPS},

    {OPD(1, 0b10, 0x2A), 1, &OpDispatchBuilder::AVX128_InsertCVTGPR_To_FPR<4>},
    {OPD(1, 0b11, 0x2A), 1, &OpDispatchBuilder::AVX128_InsertCVTGPR_To_FPR<8>},

    {OPD(1, 0b00, 0x2B), 1, &OpDispatchBuilder::AVX128_MOVVectorNT},
    {OPD(1, 0b01, 0x2B), 1, &OpDispatchBuilder::AVX128_MOVVectorNT},

    {OPD(1, 0b10, 0x2C), 1, &OpDispatchBuilder::AVX128_CVTFPR_To_GPR<4, false>},
    {OPD(1, 0b11, 0x2C), 1, &OpDispatchBuilder::AVX128_CVTFPR_To_GPR<8, false>},

    {OPD(1, 0b10, 0x2D), 1, &OpDispatchBuilder::AVX128_CVTFPR_To_GPR<4, true>},
    {OPD(1, 0b11, 0x2D), 1, &OpDispatchBuilder::AVX128_CVTFPR_To_GPR<8, true>},

    {OPD(1, 0b00, 0x2E), 1, &OpDispatchBuilder::AVX128_UCOMISx<4>},
    {OPD(1, 0b01, 0x2E), 1, &OpDispatchBuilder::AVX128_UCOMISx<8>},
    {OPD(1, 0b00, 0x2F), 1, &OpDispatchBuilder::AVX128_UCOMISx<4>},
    {OPD(1, 0b01, 0x2F), 1, &OpDispatchBuilder::AVX128_UCOMISx<8>},

    {OPD(1, 0b00, 0x50), 1, &OpDispatchBuilder::AVX128_MOVMSK<4>},
    {OPD(1, 0b01, 0x50), 1, &OpDispatchBuilder::AVX128_MOVMSK<8>},

    {OPD(1, 0b00, 0x51), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorUnary, IR::OP_VFSQRT, 4>},
    {OPD(1, 0b01, 0x51), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorUnary, IR::OP_VFSQRT, 8>},
    {OPD(1, 0b10, 0x51), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorScalarInsertALU, IR::OP_VFSQRTSCALARINSERT, 4>},
    {OPD(1, 0b11, 0x51), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorScalarInsertALU, IR::OP_VFSQRTSCALARINSERT, 8>},

    {OPD(1, 0b00, 0x52), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorUnary, IR::OP_VFRSQRT, 4>},
    {OPD(1, 0b10, 0x52), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorScalarInsertALU, IR::OP_VFRSQRTSCALARINSERT, 4>},

    {OPD(1, 0b00, 0x53), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorUnary, IR::OP_VFRECP, 4>},
    {OPD(1, 0b10, 0x53), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorScalarInsertALU, IR::OP_VFRECPSCALARINSERT, 4>},

    {OPD(1, 0b00, 0x54), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VAND, 16>},
    {OPD(1, 0b01, 0x54), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VAND, 16>},

    {OPD(1, 0b00, 0x55), 1, &OpDispatchBuilder::AVX128_VANDN},
    {OPD(1, 0b01, 0x55), 1, &OpDispatchBuilder::AVX128_VANDN},

    {OPD(1, 0b00, 0x56), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VOR, 16>},
    {OPD(1, 0b01, 0x56), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VOR, 16>},

    {OPD(1, 0b00, 0x57), 1, &OpDispatchBuilder::AVX128_VectorXOR},
    {OPD(1, 0b01, 0x57), 1, &OpDispatchBuilder::AVX128_VectorXOR},

    {OPD(1, 0b00, 0x58), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VFADD, 4>},
    {OPD(1, 0b01, 0x58), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VFADD, 8>},
    {OPD(1, 0b10, 0x58), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorScalarInsertALU, IR::OP_VFADDSCALARINSERT, 4>},
    {OPD(1, 0b11, 0x58), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorScalarInsertALU, IR::OP_VFADDSCALARINSERT, 8>},

    {OPD(1, 0b00, 0x59), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VFMUL, 4>},
    {OPD(1, 0b01, 0x59), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VFMUL, 8>},
    {OPD(1, 0b10, 0x59), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorScalarInsertALU, IR::OP_VFMULSCALARINSERT, 4>},
    {OPD(1, 0b11, 0x59), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorScalarInsertALU, IR::OP_VFMULSCALARINSERT, 8>},

    {OPD(1, 0b00, 0x5A), 1, &OpDispatchBuilder::AVX128_Vector_CVT_Float_To_Float<8, 4>},
    {OPD(1, 0b01, 0x5A), 1, &OpDispatchBuilder::AVX128_Vector_CVT_Float_To_Float<4, 8>},
    {OPD(1, 0b10, 0x5A), 1, &OpDispatchBuilder::AVX128_InsertScalar_CVT_Float_To_Float<8, 4>},
    {OPD(1, 0b11, 0x5A), 1, &OpDispatchBuilder::AVX128_InsertScalar_CVT_Float_To_Float<4, 8>},

    {OPD(1, 0b00, 0x5B), 1, &OpDispatchBuilder::AVX128_Vector_CVT_Int_To_Float<4, false>},
    {OPD(1, 0b01, 0x5B), 1, &OpDispatchBuilder::AVX128_Vector_CVT_Float_To_Int<4, false, true>},
    {OPD(1, 0b10, 0x5B), 1, &OpDispatchBuilder::AVX128_Vector_CVT_Float_To_Int<4, false, false>},

    {OPD(1, 0b00, 0x5C), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VFSUB, 4>},
    {OPD(1, 0b01, 0x5C), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VFSUB, 8>},
    {OPD(1, 0b10, 0x5C), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorScalarInsertALU, IR::OP_VFSUBSCALARINSERT, 4>},
    {OPD(1, 0b11, 0x5C), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorScalarInsertALU, IR::OP_VFSUBSCALARINSERT, 8>},

    {OPD(1, 0b00, 0x5D), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VFMIN, 4>},
    {OPD(1, 0b01, 0x5D), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VFMIN, 8>},
    {OPD(1, 0b10, 0x5D), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorScalarInsertALU, IR::OP_VFMINSCALARINSERT, 4>},
    {OPD(1, 0b11, 0x5D), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorScalarInsertALU, IR::OP_VFMINSCALARINSERT, 8>},

    {OPD(1, 0b00, 0x5E), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VFDIV, 4>},
    {OPD(1, 0b01, 0x5E), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VFDIV, 8>},
    {OPD(1, 0b10, 0x5E), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorScalarInsertALU, IR::OP_VFDIVSCALARINSERT, 4>},
    {OPD(1, 0b11, 0x5E), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorScalarInsertALU, IR::OP_VFDIVSCALARINSERT, 8>},

    {OPD(1, 0b00, 0x5F), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VFMAX, 4>},
    {OPD(1, 0b01, 0x5F), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VFMAX, 8>},
    {OPD(1, 0b10, 0x5F), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorScalarInsertALU, IR::OP_VFMAXSCALARINSERT, 4>},
    {OPD(1, 0b11, 0x5F), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorScalarInsertALU, IR::OP_VFMAXSCALARINSERT, 8>},

    {OPD(1, 0b01, 0x60), 1, &OpDispatchBuilder::AVX128_VPUNPCKL<1>},
    {OPD(1, 0b01, 0x61), 1, &OpDispatchBuilder::AVX128_VPUNPCKL<2>},
    {OPD(1, 0b01, 0x62), 1, &OpDispatchBuilder::AVX128_VPUNPCKL<4>},
    {OPD(1, 0b01, 0x63), 1, &OpDispatchBuilder::AVX128_VPACKSS<2>},
    {OPD(1, 0b01, 0x64), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VCMPGT, 1>},
    {OPD(1, 0b01, 0x65), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VCMPGT, 2>},
    {OPD(1, 0b01, 0x66), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VCMPGT, 4>},
    {OPD(1, 0b01, 0x67), 1, &OpDispatchBuilder::AVX128_VPACKUS<2>},
    {OPD(1, 0b01, 0x68), 1, &OpDispatchBuilder::AVX128_VPUNPCKH<1>},
    {OPD(1, 0b01, 0x69), 1, &OpDispatchBuilder::AVX128_VPUNPCKH<2>},
    {OPD(1, 0b01, 0x6A), 1, &OpDispatchBuilder::AVX128_VPUNPCKH<4>},
    {OPD(1, 0b01, 0x6B), 1, &OpDispatchBuilder::AVX128_VPACKSS<4>},
    {OPD(1, 0b01, 0x6C), 1, &OpDispatchBuilder::AVX128_VPUNPCKL<8>},
    {OPD(1, 0b01, 0x6D), 1, &OpDispatchBuilder::AVX128_VPUNPCKH<8>},
    {OPD(1, 0b01, 0x6E), 1, &OpDispatchBuilder::AVX128_MOVBetweenGPR_FPR},

    {OPD(1, 0b01, 0x6F), 1, &OpDispatchBuilder::AVX128_VMOVAPS},
    {OPD(1, 0b10, 0x6F), 1, &OpDispatchBuilder::AVX128_VMOVAPS},

    {OPD(1, 0b01, 0x70), 1, &OpDispatchBuilder::AVX128_VPERMILImm<4>},
    {OPD(1, 0b10, 0x70), 1, &OpDispatchBuilder::AVX128_VPSHUF<2, false>},
    {OPD(1, 0b11, 0x70), 1, &OpDispatchBuilder::AVX128_VPSHUF<2, true>},

    {OPD(1, 0b01, 0x74), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VCMPEQ, 1>},
    {OPD(1, 0b01, 0x75), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VCMPEQ, 2>},
    {OPD(1, 0b01, 0x76), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VCMPEQ, 4>},

    {OPD(1, 0b00, 0x77), 1, &OpDispatchBuilder::AVX128_VZERO},

    {OPD(1, 0b01, 0x7C), 1, &OpDispatchBuilder::AVX128_VHADDP<IR::OP_VFADDP, 8>},
    {OPD(1, 0b11, 0x7C), 1, &OpDispatchBuilder::AVX128_VHADDP<IR::OP_VFADDP, 4>},
    {OPD(1, 0b01, 0x7D), 1, &OpDispatchBuilder::AVX128_VHSUBP<OpSize::i64Bit>},
    {OPD(1, 0b11, 0x7D), 1, &OpDispatchBuilder::AVX128_VHSUBP<OpSize::i32Bit>},

    {OPD(1, 0b01, 0x7E), 1, &OpDispatchBuilder::AVX128_MOVBetweenGPR_FPR},
    {OPD(1, 0b10, 0x7E), 1, &OpDispatchBuilder::AVX128_MOVQ},

    {OPD(1, 0b01, 0x7F), 1, &OpDispatchBuilder::AVX128_VMOVAPS},
    {OPD(1, 0b10, 0x7F), 1, &OpDispatchBuilder::AVX128_VMOVAPS},

    {OPD(1, 0b00, 0xC2), 1, &OpDispatchBuilder::AVX128_VFCMP<4>},
    {OPD(1, 0b01, 0xC2), 1, &OpDispatchBuilder::AVX128_VFCMP<8>},
    {OPD(1, 0b10, 0xC2), 1, &OpDispatchBuilder::AVX128_InsertScalarFCMP<4>},
    {OPD(1, 0b11, 0xC2), 1, &OpDispatchBuilder::AVX128_InsertScalarFCMP<8>},

    {OPD(1, 0b01, 0xC4), 1, &OpDispatchBuilder::AVX128_VPINSRW},
    {OPD(1, 0b01, 0xC5), 1, &OpDispatchBuilder::AVX128_PExtr<2>},

    {OPD(1, 0b00, 0xC6), 1, &OpDispatchBuilder::AVX128_VSHUF<4>},
    {OPD(1, 0b01, 0xC6), 1, &OpDispatchBuilder::AVX128_VSHUF<8>},

    {OPD(1, 0b01, 0xD0), 1, &OpDispatchBuilder::AVX128_VADDSUBP<8>},
    {OPD(1, 0b11, 0xD0), 1, &OpDispatchBuilder::AVX128_VADDSUBP<4>},

    {OPD(1, 0b01, 0xD1), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorShiftWideImpl, 2, IROps::OP_VUSHRSWIDE>}, // VPSRL
    {OPD(1, 0b01, 0xD2), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorShiftWideImpl, 4, IROps::OP_VUSHRSWIDE>}, // VPSRL
    {OPD(1, 0b01, 0xD3), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorShiftWideImpl, 8, IROps::OP_VUSHRSWIDE>}, // VPSRL
    {OPD(1, 0b01, 0xD4), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VADD, 8>},
    {OPD(1, 0b01, 0xD5), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VMUL, 2>},
    {OPD(1, 0b01, 0xD6), 1, &OpDispatchBuilder::AVX128_MOVQ},
    {OPD(1, 0b01, 0xD7), 1, &OpDispatchBuilder::AVX128_MOVMSKB},

    {OPD(1, 0b01, 0xD8), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VUQSUB, 1>},
    {OPD(1, 0b01, 0xD9), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VUQSUB, 2>},
    {OPD(1, 0b01, 0xDA), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VUMIN, 1>},
    {OPD(1, 0b01, 0xDB), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VAND, 16>},
    {OPD(1, 0b01, 0xDC), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VUQADD, 1>},
    {OPD(1, 0b01, 0xDD), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VUQADD, 2>},
    {OPD(1, 0b01, 0xDE), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VUMAX, 1>},
    {OPD(1, 0b01, 0xDF), 1, &OpDispatchBuilder::AVX128_VANDN},

    {OPD(1, 0b01, 0xE0), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VURAVG, 1>},
    {OPD(1, 0b01, 0xE1), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorShiftWideImpl, 2, IROps::OP_VSSHRSWIDE>}, // VPSRA
    {OPD(1, 0b01, 0xE2), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorShiftWideImpl, 4, IROps::OP_VSSHRSWIDE>}, // VPSRA
    {OPD(1, 0b01, 0xE3), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VURAVG, 2>},
    {OPD(1, 0b01, 0xE4), 1, &OpDispatchBuilder::AVX128_VPMULHW<false>},
    {OPD(1, 0b01, 0xE5), 1, &OpDispatchBuilder::AVX128_VPMULHW<true>},

    {OPD(1, 0b01, 0xE6), 1, &OpDispatchBuilder::AVX128_Vector_CVT_Float_To_Int<8, true, false>},
    {OPD(1, 0b10, 0xE6), 1, &OpDispatchBuilder::AVX128_Vector_CVT_Int_To_Float<4, true>},
    {OPD(1, 0b11, 0xE6), 1, &OpDispatchBuilder::AVX128_Vector_CVT_Float_To_Int<8, true, true>},

    {OPD(1, 0b01, 0xE7), 1, &OpDispatchBuilder::AVX128_MOVVectorNT},

    {OPD(1, 0b01, 0xE8), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VSQSUB, 1>},
    {OPD(1, 0b01, 0xE9), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VSQSUB, 2>},
    {OPD(1, 0b01, 0xEA), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VSMIN, 2>},
    {OPD(1, 0b01, 0xEB), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VOR, 16>},
    {OPD(1, 0b01, 0xEC), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VSQADD, 1>},
    {OPD(1, 0b01, 0xED), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VSQADD, 2>},
    {OPD(1, 0b01, 0xEE), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VSMAX, 2>},
    {OPD(1, 0b01, 0xEF), 1, &OpDispatchBuilder::AVX128_VectorXOR},

    {OPD(1, 0b11, 0xF0), 1, &OpDispatchBuilder::AVX128_MOVVectorUnaligned},
    {OPD(1, 0b01, 0xF1), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorShiftWideImpl, 2, IROps::OP_VUSHLSWIDE>}, // VPSLL
    {OPD(1, 0b01, 0xF2), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorShiftWideImpl, 4, IROps::OP_VUSHLSWIDE>}, // VPSLL
    {OPD(1, 0b01, 0xF3), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorShiftWideImpl, 8, IROps::OP_VUSHLSWIDE>}, // VPSLL
    {OPD(1, 0b01, 0xF4), 1, &OpDispatchBuilder::AVX128_VPMULL<4, false>},
    {OPD(1, 0b01, 0xF5), 1, &OpDispatchBuilder::AVX128_VPMADDWD},
    {OPD(1, 0b01, 0xF6), 1, &OpDispatchBuilder::AVX128_VPSADBW},
    {OPD(1, 0b01, 0xF7), 1, &OpDispatchBuilder::AVX128_MASKMOV},

    {OPD(1, 0b01, 0xF8), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VSUB, 1>},
    {OPD(1, 0b01, 0xF9), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VSUB, 2>},
    {OPD(1, 0b01, 0xFA), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VSUB, 4>},
    {OPD(1, 0b01, 0xFB), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VSUB, 8>},
    {OPD(1, 0b01, 0xFC), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VADD, 1>},
    {OPD(1, 0b01, 0xFD), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VADD, 2>},
    {OPD(1, 0b01, 0xFE), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VADD, 4>},

    {OPD(2, 0b01, 0x00), 1, &OpDispatchBuilder::AVX128_VPSHUFB},
    {OPD(2, 0b01, 0x01), 1, &OpDispatchBuilder::AVX128_VHADDP<IR::OP_VADDP, 2>},
    {OPD(2, 0b01, 0x02), 1, &OpDispatchBuilder::AVX128_VHADDP<IR::OP_VADDP, 4>},
    {OPD(2, 0b01, 0x03), 1, &OpDispatchBuilder::AVX128_VPHADDSW},
    {OPD(2, 0b01, 0x04), 1, &OpDispatchBuilder::AVX128_VPMADDUBSW},

    {OPD(2, 0b01, 0x05), 1, &OpDispatchBuilder::AVX128_VPHSUB<2>},
    {OPD(2, 0b01, 0x06), 1, &OpDispatchBuilder::AVX128_VPHSUB<4>},
    {OPD(2, 0b01, 0x07), 1, &OpDispatchBuilder::AVX128_VPHSUBSW},

    {OPD(2, 0b01, 0x08), 1, &OpDispatchBuilder::AVX128_VPSIGN<1>},
    {OPD(2, 0b01, 0x09), 1, &OpDispatchBuilder::AVX128_VPSIGN<2>},
    {OPD(2, 0b01, 0x0A), 1, &OpDispatchBuilder::AVX128_VPSIGN<4>},
    {OPD(2, 0b01, 0x0B), 1, &OpDispatchBuilder::AVX128_VPMULHRSW},
    {OPD(2, 0b01, 0x0C), 1, &OpDispatchBuilder::AVX128_VPERMILReg<4>},
    {OPD(2, 0b01, 0x0D), 1, &OpDispatchBuilder::AVX128_VPERMILReg<8>},
    {OPD(2, 0b01, 0x0E), 1, &OpDispatchBuilder::AVX128_VTESTP<OpSize::i32Bit>},
    {OPD(2, 0b01, 0x0F), 1, &OpDispatchBuilder::AVX128_VTESTP<OpSize::i64Bit>},


    {OPD(2, 0b01, 0x13), 1, &OpDispatchBuilder::AVX128_VCVTPH2PS},
    {OPD(2, 0b01, 0x16), 1, &OpDispatchBuilder::AVX128_VPERMD},
    {OPD(2, 0b01, 0x17), 1, &OpDispatchBuilder::AVX128_PTest},
    {OPD(2, 0b01, 0x18), 1, &OpDispatchBuilder::AVX128_VBROADCAST<4>},
    {OPD(2, 0b01, 0x19), 1, &OpDispatchBuilder::AVX128_VBROADCAST<8>},
    {OPD(2, 0b01, 0x1A), 1, &OpDispatchBuilder::AVX128_VBROADCAST<16>},
    {OPD(2, 0b01, 0x1C), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorUnary, IR::OP_VABS, 1>},
    {OPD(2, 0b01, 0x1D), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorUnary, IR::OP_VABS, 2>},
    {OPD(2, 0b01, 0x1E), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorUnary, IR::OP_VABS, 4>},

    {OPD(2, 0b01, 0x20), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_ExtendVectorElements, 1, 2, true>},
    {OPD(2, 0b01, 0x21), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_ExtendVectorElements, 1, 4, true>},
    {OPD(2, 0b01, 0x22), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_ExtendVectorElements, 1, 8, true>},
    {OPD(2, 0b01, 0x23), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_ExtendVectorElements, 2, 4, true>},
    {OPD(2, 0b01, 0x24), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_ExtendVectorElements, 2, 8, true>},
    {OPD(2, 0b01, 0x25), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_ExtendVectorElements, 4, 8, true>},

    {OPD(2, 0b01, 0x28), 1, &OpDispatchBuilder::AVX128_VPMULL<4, true>},
    {OPD(2, 0b01, 0x29), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VCMPEQ, 8>},
    {OPD(2, 0b01, 0x2A), 1, &OpDispatchBuilder::AVX128_MOVVectorNT},
    {OPD(2, 0b01, 0x2B), 1, &OpDispatchBuilder::AVX128_VPACKUS<4>},
    {OPD(2, 0b01, 0x2C), 1, &OpDispatchBuilder::AVX128_VMASKMOV<OpSize::i32Bit, false>},
    {OPD(2, 0b01, 0x2D), 1, &OpDispatchBuilder::AVX128_VMASKMOV<OpSize::i64Bit, false>},
    {OPD(2, 0b01, 0x2E), 1, &OpDispatchBuilder::AVX128_VMASKMOV<OpSize::i32Bit, true>},
    {OPD(2, 0b01, 0x2F), 1, &OpDispatchBuilder::AVX128_VMASKMOV<OpSize::i64Bit, true>},

    {OPD(2, 0b01, 0x30), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_ExtendVectorElements, 1, 2, false>},
    {OPD(2, 0b01, 0x31), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_ExtendVectorElements, 1, 4, false>},
    {OPD(2, 0b01, 0x32), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_ExtendVectorElements, 1, 8, false>},
    {OPD(2, 0b01, 0x33), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_ExtendVectorElements, 2, 4, false>},
    {OPD(2, 0b01, 0x34), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_ExtendVectorElements, 2, 8, false>},
    {OPD(2, 0b01, 0x35), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_ExtendVectorElements, 4, 8, false>},
    {OPD(2, 0b01, 0x36), 1, &OpDispatchBuilder::AVX128_VPERMD},

    {OPD(2, 0b01, 0x37), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VCMPGT, 8>},
    {OPD(2, 0b01, 0x38), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VSMIN, 1>},
    {OPD(2, 0b01, 0x39), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VSMIN, 4>},
    {OPD(2, 0b01, 0x3A), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VUMIN, 2>},
    {OPD(2, 0b01, 0x3B), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VUMIN, 4>},
    {OPD(2, 0b01, 0x3C), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VSMAX, 1>},
    {OPD(2, 0b01, 0x3D), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VSMAX, 4>},
    {OPD(2, 0b01, 0x3E), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VUMAX, 2>},
    {OPD(2, 0b01, 0x3F), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VUMAX, 4>},

    {OPD(2, 0b01, 0x40), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorALU, IR::OP_VMUL, 4>},
    {OPD(2, 0b01, 0x41), 1, &OpDispatchBuilder::AVX128_PHMINPOSUW},
    {OPD(2, 0b01, 0x45), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VariableShiftImpl, IROps::OP_VUSHR>}, // VPSRLV
    {OPD(2, 0b01, 0x46), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VariableShiftImpl, IROps::OP_VSSHR>}, // VPSRAVD
    {OPD(2, 0b01, 0x47), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VariableShiftImpl, IROps::OP_VUSHL>}, // VPSLLV

    {OPD(2, 0b01, 0x58), 1, &OpDispatchBuilder::AVX128_VBROADCAST<4>},
    {OPD(2, 0b01, 0x59), 1, &OpDispatchBuilder::AVX128_VBROADCAST<8>},
    {OPD(2, 0b01, 0x5A), 1, &OpDispatchBuilder::AVX128_VBROADCAST<16>},

    {OPD(2, 0b01, 0x78), 1, &OpDispatchBuilder::AVX128_VBROADCAST<1>},
    {OPD(2, 0b01, 0x79), 1, &OpDispatchBuilder::AVX128_VBROADCAST<2>},

    {OPD(2, 0b01, 0x8C), 1, &OpDispatchBuilder::AVX128_VPMASKMOV<false>},
    {OPD(2, 0b01, 0x8E), 1, &OpDispatchBuilder::AVX128_VPMASKMOV<true>},

    {OPD(2, 0b01, 0x90), 1, &OpDispatchBuilder::AVX128_VPGATHER<OpSize::i32Bit>},
    {OPD(2, 0b01, 0x91), 1, &OpDispatchBuilder::AVX128_VPGATHER<OpSize::i64Bit>},
    {OPD(2, 0b01, 0x92), 1, &OpDispatchBuilder::AVX128_VPGATHER<OpSize::i32Bit>},
    {OPD(2, 0b01, 0x93), 1, &OpDispatchBuilder::AVX128_VPGATHER<OpSize::i64Bit>},

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
    {OPD(3, 0b01, 0x02), 1, &OpDispatchBuilder::AVX128_VBLEND<OpSize::i32Bit>},
    {OPD(3, 0b01, 0x04), 1, &OpDispatchBuilder::AVX128_VPERMILImm<4>},
    {OPD(3, 0b01, 0x05), 1, &OpDispatchBuilder::AVX128_VPERMILImm<8>},
    {OPD(3, 0b01, 0x06), 1, &OpDispatchBuilder::AVX128_VPERM2},
    {OPD(3, 0b01, 0x08), 1, &OpDispatchBuilder::AVX128_VectorRound<4>},
    {OPD(3, 0b01, 0x09), 1, &OpDispatchBuilder::AVX128_VectorRound<8>},
    {OPD(3, 0b01, 0x0A), 1, &OpDispatchBuilder::AVX128_InsertScalarRound<4>},
    {OPD(3, 0b01, 0x0B), 1, &OpDispatchBuilder::AVX128_InsertScalarRound<8>},
    {OPD(3, 0b01, 0x0C), 1, &OpDispatchBuilder::AVX128_VBLEND<OpSize::i32Bit>},
    {OPD(3, 0b01, 0x0D), 1, &OpDispatchBuilder::AVX128_VBLEND<OpSize::i64Bit>},
    {OPD(3, 0b01, 0x0E), 1, &OpDispatchBuilder::AVX128_VBLEND<OpSize::i16Bit>},
    {OPD(3, 0b01, 0x0F), 1, &OpDispatchBuilder::AVX128_VPALIGNR},

    {OPD(3, 0b01, 0x14), 1, &OpDispatchBuilder::AVX128_PExtr<1>},
    {OPD(3, 0b01, 0x15), 1, &OpDispatchBuilder::AVX128_PExtr<2>},
    {OPD(3, 0b01, 0x16), 1, &OpDispatchBuilder::AVX128_PExtr<4>},
    {OPD(3, 0b01, 0x17), 1, &OpDispatchBuilder::AVX128_PExtr<4>},

    {OPD(3, 0b01, 0x18), 1, &OpDispatchBuilder::AVX128_VINSERT},
    {OPD(3, 0b01, 0x19), 1, &OpDispatchBuilder::AVX128_VEXTRACT128},
    {OPD(3, 0b01, 0x1D), 1, &OpDispatchBuilder::AVX128_VCVTPS2PH},
    {OPD(3, 0b01, 0x20), 1, &OpDispatchBuilder::AVX128_VPINSRB},
    {OPD(3, 0b01, 0x21), 1, &OpDispatchBuilder::VINSERTPSOp},
    {OPD(3, 0b01, 0x22), 1, &OpDispatchBuilder::AVX128_VPINSRDQ},

    {OPD(3, 0b01, 0x38), 1, &OpDispatchBuilder::AVX128_VINSERT},
    {OPD(3, 0b01, 0x39), 1, &OpDispatchBuilder::AVX128_VEXTRACT128},

    {OPD(3, 0b01, 0x40), 1, &OpDispatchBuilder::AVX128_VDPP<4>},
    {OPD(3, 0b01, 0x41), 1, &OpDispatchBuilder::AVX128_VDPP<8>},
    {OPD(3, 0b01, 0x42), 1, &OpDispatchBuilder::AVX128_VMPSADBW},

    {OPD(3, 0b01, 0x46), 1, &OpDispatchBuilder::AVX128_VPERM2},

    {OPD(3, 0b01, 0x4A), 1, &OpDispatchBuilder::AVX128_VectorVariableBlend<4>},
    {OPD(3, 0b01, 0x4B), 1, &OpDispatchBuilder::AVX128_VectorVariableBlend<8>},
    {OPD(3, 0b01, 0x4C), 1, &OpDispatchBuilder::AVX128_VectorVariableBlend<1>},

    {OPD(3, 0b01, 0x60), 1, &OpDispatchBuilder::AVX128_VPCMPESTRM},
    {OPD(3, 0b01, 0x61), 1, &OpDispatchBuilder::AVX128_VPCMPESTRI},
    {OPD(3, 0b01, 0x62), 1, &OpDispatchBuilder::AVX128_VPCMPISTRM},
    {OPD(3, 0b01, 0x63), 1, &OpDispatchBuilder::AVX128_VPCMPISTRI},

    {OPD(3, 0b01, 0xDF), 1, &OpDispatchBuilder::AVX128_VAESKeyGenAssist},
  };
#undef OPD

#define OPD(group, pp, opcode) (((group - X86Tables::TYPE_VEX_GROUP_12) << 4) | (pp << 3) | (opcode))
  static constexpr std::tuple<uint8_t, uint8_t, X86Tables::OpDispatchPtr> VEX128TableGroupOps[] {
    // VPSRLI
    {OPD(X86Tables::TYPE_VEX_GROUP_12, 1, 0b010), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorShiftImmImpl, 2, IROps::OP_VUSHRI>},
    // VPSLLI
    {OPD(X86Tables::TYPE_VEX_GROUP_12, 1, 0b110), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorShiftImmImpl, 2, IROps::OP_VSHLI>},
    // VPSRAI
    {OPD(X86Tables::TYPE_VEX_GROUP_12, 1, 0b100), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorShiftImmImpl, 2, IROps::OP_VSSHRI>},

    // VPSRLI
    {OPD(X86Tables::TYPE_VEX_GROUP_13, 1, 0b010), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorShiftImmImpl, 4, IROps::OP_VUSHRI>},
    // VPSLLI
    {OPD(X86Tables::TYPE_VEX_GROUP_13, 1, 0b110), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorShiftImmImpl, 4, IROps::OP_VSHLI>},
    // VPSRAI
    {OPD(X86Tables::TYPE_VEX_GROUP_13, 1, 0b100), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorShiftImmImpl, 4, IROps::OP_VSSHRI>},

    // VPSRLI
    {OPD(X86Tables::TYPE_VEX_GROUP_14, 1, 0b010), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorShiftImmImpl, 8, IROps::OP_VUSHRI>},
    // VPSRLDQ
    {OPD(X86Tables::TYPE_VEX_GROUP_14, 1, 0b011), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_ShiftDoubleImm, ShiftDirection::RIGHT>},
    // VPSLLI
    {OPD(X86Tables::TYPE_VEX_GROUP_14, 1, 0b110), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_VectorShiftImmImpl, 8, IROps::OP_VSHLI>},
    // VPSLLDQ
    {OPD(X86Tables::TYPE_VEX_GROUP_14, 1, 0b111), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::AVX128_ShiftDoubleImm, ShiftDirection::LEFT>},

    ///< Use the regular implementation. It just happens to be in the VEX table.
    {OPD(X86Tables::TYPE_VEX_GROUP_15, 0, 0b010), 1, &OpDispatchBuilder::LDMXCSR},
    {OPD(X86Tables::TYPE_VEX_GROUP_15, 0, 0b011), 1, &OpDispatchBuilder::STMXCSR},
  };
#undef OPD

#define OPD(map_select, pp, opcode) (((map_select - 1) << 10) | (pp << 8) | (opcode))
  constexpr std::tuple<uint16_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr> VEX128_PCLMUL[] = {
    {OPD(3, 0b01, 0x44), 1, &OpDispatchBuilder::AVX128_VPCLMULQDQ},
  };
#undef OPD

  auto InstallToTable = [](auto& FinalTable, auto& LocalTable) {
    for (auto Op : LocalTable) {
      auto OpNum = std::get<0>(Op);
      auto Dispatcher = std::get<2>(Op);
      for (uint8_t i = 0; i < std::get<1>(Op); ++i) {
        LOGMAN_THROW_A_FMT(FinalTable[OpNum + i].OpcodeDispatcher == nullptr, "Duplicate Entry");
        FinalTable[OpNum + i].OpcodeDispatcher = Dispatcher;
      }
    }
  };

  InstallToTable(FEXCore::X86Tables::VEXTableOps, AVX128Table);
  InstallToTable(FEXCore::X86Tables::VEXTableGroupOps, VEX128TableGroupOps);
  if (CTX->HostFeatures.SupportsPMULL_128Bit) {
    InstallToTable(FEXCore::X86Tables::VEXTableOps, VEX128_PCLMUL);
  }

  SaveAVXStateFunc = &OpDispatchBuilder::AVX128_SaveAVXState;
  RestoreAVXStateFunc = &OpDispatchBuilder::AVX128_RestoreAVXState;
  DefaultAVXStateFunc = &OpDispatchBuilder::AVX128_DefaultAVXState;
}

OpDispatchBuilder::RefPair OpDispatchBuilder::AVX128_LoadSource_WithOpSize(
  const X86Tables::DecodedOp& Op, const X86Tables::DecodedOperand& Operand, uint32_t Flags, bool NeedsHigh, MemoryAccessType AccessType) {

  if (Operand.IsGPR()) {
    const auto gpr = Operand.Data.GPR.GPR;
    LOGMAN_THROW_AA_FMT(gpr >= FEXCore::X86State::REG_XMM_0 && gpr <= FEXCore::X86State::REG_XMM_15, "must be AVX reg");
    const auto gprIndex = gpr - X86State::REG_XMM_0;
    return {
      .Low = AVX128_LoadXMMRegister(gprIndex, false),
      .High = NeedsHigh ? AVX128_LoadXMMRegister(gprIndex, true) : nullptr,
    };
  } else {
    LOGMAN_THROW_A_FMT(IsOperandMem(Operand, true), "only memory sources");

    AddressMode A = DecodeAddress(Op, Operand, AccessType, true /* IsLoad */);

    AddressMode HighA = A;
    HighA.Offset += 16;

    if (Operand.IsSIB()) {
      const bool IsVSIB = (Op->Flags & X86Tables::DecodeFlags::FLAG_VSIB_BYTE) != 0;
      LOGMAN_THROW_AA_FMT(!IsVSIB, "VSIB uses LoadVSIB instead");
    }

    if (NeedsHigh) {
      return _LoadMemPairAutoTSO(FPRClass, 16, A, 1);
    } else {
      return {.Low = _LoadMemAutoTSO(FPRClass, 16, A, 1)};
    }
  }
}

OpDispatchBuilder::RefVSIB
OpDispatchBuilder::AVX128_LoadVSIB(const X86Tables::DecodedOp& Op, const X86Tables::DecodedOperand& Operand, uint32_t Flags, bool NeedsHigh) {
  [[maybe_unused]] const bool IsVSIB = (Op->Flags & X86Tables::DecodeFlags::FLAG_VSIB_BYTE) != 0;
  LOGMAN_THROW_A_FMT(Operand.IsSIB() && IsVSIB, "Trying to load VSIB for something that isn't the correct type!");

  // VSIB is a very special case which has a ton of encoded data.
  // Get it in a format we can reason about.

  const auto Index_gpr = Operand.Data.SIB.Index;
  const auto Base_gpr = Operand.Data.SIB.Base;
  LOGMAN_THROW_AA_FMT(Index_gpr >= FEXCore::X86State::REG_XMM_0 && Index_gpr <= FEXCore::X86State::REG_XMM_15, "must be AVX reg");
  LOGMAN_THROW_AA_FMT(
    Base_gpr == FEXCore::X86State::REG_INVALID || (Base_gpr >= FEXCore::X86State::REG_RAX && Base_gpr <= FEXCore::X86State::REG_R15),
    "Base must be a GPR.");
  const auto Index_XMM_gpr = Index_gpr - X86State::REG_XMM_0;

  return {
    .Low = AVX128_LoadXMMRegister(Index_XMM_gpr, false),
    .High = NeedsHigh ? AVX128_LoadXMMRegister(Index_XMM_gpr, true) : Invalid(),
    .BaseAddr = Base_gpr != FEXCore::X86State::REG_INVALID ? LoadGPRRegister(Base_gpr, OpSize::i64Bit, 0, false) : nullptr,
    .Displacement = Operand.Data.SIB.Offset,
    .Scale = Operand.Data.SIB.Scale,
  };
}

void OpDispatchBuilder::AVX128_StoreResult_WithOpSize(FEXCore::X86Tables::DecodedOp Op, const FEXCore::X86Tables::DecodedOperand& Operand,
                                                      const RefPair Src, MemoryAccessType AccessType) {
  if (Operand.IsGPR()) {
    const auto gpr = Operand.Data.GPR.GPR;
    LOGMAN_THROW_AA_FMT(gpr >= FEXCore::X86State::REG_XMM_0 && gpr <= FEXCore::X86State::REG_XMM_15, "expected AVX register");
    const auto gprIndex = gpr - X86State::REG_XMM_0;

    if (Src.Low) {
      AVX128_StoreXMMRegister(gprIndex, Src.Low, false);
    }

    if (Src.High) {
      AVX128_StoreXMMRegister(gprIndex, Src.High, true);
    }
  } else {
    AddressMode A = DecodeAddress(Op, Operand, AccessType, false /* IsLoad */);

    if (Src.High) {
      _StoreMemPairAutoTSO(FPRClass, 16, A, Src.Low, Src.High, 1);
    } else {
      _StoreMemAutoTSO(FPRClass, 16, A, Src.Low, 1);
    }
  }
}

Ref OpDispatchBuilder::AVX128_LoadXMMRegister(uint32_t XMM, bool High) {
  if (High) {
    return LoadContext(AVXHigh0Index + XMM);
  } else {
    return LoadXMMRegister(XMM);
  }
}

void OpDispatchBuilder::AVX128_StoreXMMRegister(uint32_t XMM, const Ref Src, bool High) {
  if (High) {
    StoreContext(AVXHigh0Index + XMM, Src);
  } else {
    StoreXMMRegister(XMM, Src);
  }
}

void OpDispatchBuilder::AVX128_VMOVAPS(OpcodeArgs) {
  // Reg <- Mem or Reg <- Reg
  const auto SrcSize = GetSrcSize(Op);
  const auto Is128Bit = SrcSize == Core::CPUState::XMM_SSE_REG_SIZE;

  if (Is128Bit) {
    // Zero upper 128-bits
    auto Src = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, false);

    ///< Zero upper bits when destination is GPR.
    if (Op->Dest.IsGPR()) {
      Src.High = LoadZeroVector(OpSize::i128Bit);
    }
    AVX128_StoreResult_WithOpSize(Op, Op->Dest, Src);
  } else {
    // Copy or memory load
    auto Src = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, true);
    AVX128_StoreResult_WithOpSize(Op, Op->Dest, Src);
  }
}

void OpDispatchBuilder::AVX128_VMOVScalarImpl(OpcodeArgs, size_t ElementSize) {
  if (Op->Dest.IsGPR() && Op->Src[0].IsGPR() && Op->Src[1].IsGPR()) {
    // VMOVSS/SD xmm1, xmm2, xmm3
    // Lower 128-bits are merged
    // Upper 128-bits are zero'd
    auto Src1 = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, false);
    auto Src2 = AVX128_LoadSource_WithOpSize(Op, Op->Src[1], Op->Flags, false);
    Ref Result = _VInsElement(16, ElementSize, 0, 0, Src1.Low, Src2.Low);
    auto High = LoadZeroVector(OpSize::i128Bit);
    AVX128_StoreResult_WithOpSize(Op, Op->Dest, RefPair {.Low = Result, .High = High});
  } else if (Op->Dest.IsGPR()) {
    // VMOVSS/SD xmm1, mem32/mem64
    Ref Src = LoadSource_WithOpSize(FPRClass, Op, Op->Src[1], ElementSize, Op->Flags);
    auto High = LoadZeroVector(OpSize::i128Bit);
    AVX128_StoreResult_WithOpSize(Op, Op->Dest, RefPair {.Low = Src, .High = High});
  } else {
    // VMOVSS/SD mem32/mem64, xmm1
    auto Src = AVX128_LoadSource_WithOpSize(Op, Op->Src[1], Op->Flags, false);
    StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Src.Low, ElementSize, -1);
  }
}

void OpDispatchBuilder::AVX128_VMOVSD(OpcodeArgs) {
  AVX128_VMOVScalarImpl(Op, OpSize::i64Bit);
}

void OpDispatchBuilder::AVX128_VMOVSS(OpcodeArgs) {
  AVX128_VMOVScalarImpl(Op, OpSize::i32Bit);
}

void OpDispatchBuilder::AVX128_VectorALU(OpcodeArgs, IROps IROp, size_t ElementSize) {
  const auto SrcSize = GetSrcSize(Op);
  const auto Is128Bit = SrcSize == Core::CPUState::XMM_SSE_REG_SIZE;

  auto Src1 = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, !Is128Bit);
  auto Src2 = AVX128_LoadSource_WithOpSize(Op, Op->Src[1], Op->Flags, !Is128Bit);
  DeriveOp(Result_Low, IROp, _VAdd(16, ElementSize, Src1.Low, Src2.Low));

  if (Is128Bit) {
    auto High = LoadZeroVector(OpSize::i128Bit);
    AVX128_StoreResult_WithOpSize(Op, Op->Dest, RefPair {.Low = Result_Low, .High = High});
  } else {
    DeriveOp(Result_High, IROp, _VAdd(16, ElementSize, Src1.High, Src2.High));
    AVX128_StoreResult_WithOpSize(Op, Op->Dest, RefPair {.Low = Result_Low, .High = Result_High});
  }
}

void OpDispatchBuilder::AVX128_VectorUnary(OpcodeArgs, IROps IROp, size_t ElementSize) {
  const auto SrcSize = GetSrcSize(Op);
  const auto Is128Bit = SrcSize == Core::CPUState::XMM_SSE_REG_SIZE;

  auto Src = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, !Is128Bit);
  DeriveOp(Result_Low, IROp, _VFSqrt(16, ElementSize, Src.Low));

  if (Is128Bit) {
    auto High = LoadZeroVector(OpSize::i128Bit);
    AVX128_StoreResult_WithOpSize(Op, Op->Dest, RefPair {.Low = Result_Low, .High = High});
  } else {
    DeriveOp(Result_High, IROp, _VFSqrt(16, ElementSize, Src.High));
    AVX128_StoreResult_WithOpSize(Op, Op->Dest, RefPair {.Low = Result_Low, .High = Result_High});
  }
}

void OpDispatchBuilder::AVX128_VectorUnaryImpl(OpcodeArgs, size_t SrcSize, size_t ElementSize,
                                               std::function<Ref(size_t ElementSize, Ref Src)> Helper) {
  const auto Is128Bit = SrcSize == Core::CPUState::XMM_SSE_REG_SIZE;

  auto Src = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, !Is128Bit);
  RefPair Result {};
  Result.Low = Helper(ElementSize, Src.Low);

  if (Is128Bit) {
    Result.High = LoadZeroVector(OpSize::i128Bit);
  } else {
    Result.High = Helper(ElementSize, Src.High);
  }

  AVX128_StoreResult_WithOpSize(Op, Op->Dest, Result);
}

void OpDispatchBuilder::AVX128_VectorBinaryImpl(OpcodeArgs, size_t SrcSize, size_t ElementSize,
                                                std::function<Ref(size_t ElementSize, Ref Src1, Ref Src2)> Helper) {
  const auto Is128Bit = SrcSize == Core::CPUState::XMM_SSE_REG_SIZE;

  auto Src1 = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, !Is128Bit);
  auto Src2 = AVX128_LoadSource_WithOpSize(Op, Op->Src[1], Op->Flags, !Is128Bit);
  RefPair Result {};
  Result.Low = Helper(ElementSize, Src1.Low, Src2.Low);

  if (Is128Bit) {
    Result.High = LoadZeroVector(OpSize::i128Bit);
  } else {
    Result.High = Helper(ElementSize, Src1.High, Src2.High);
  }

  AVX128_StoreResult_WithOpSize(Op, Op->Dest, Result);
}

void OpDispatchBuilder::AVX128_VectorTrinaryImpl(OpcodeArgs, size_t SrcSize, size_t ElementSize, Ref Src3,
                                                 std::function<Ref(size_t ElementSize, Ref Src1, Ref Src2, Ref Src3)> Helper) {
  const auto Is128Bit = SrcSize == Core::CPUState::XMM_SSE_REG_SIZE;

  auto Src1 = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, !Is128Bit);
  auto Src2 = AVX128_LoadSource_WithOpSize(Op, Op->Src[1], Op->Flags, !Is128Bit);
  RefPair Result {};
  Result.Low = Helper(ElementSize, Src1.Low, Src2.Low, Src3);

  if (Is128Bit) {
    Result.High = LoadZeroVector(OpSize::i128Bit);
  } else {
    Result.High = Helper(ElementSize, Src1.High, Src2.High, Src3);
  }

  AVX128_StoreResult_WithOpSize(Op, Op->Dest, Result);
}

void OpDispatchBuilder::AVX128_VectorShiftWideImpl(OpcodeArgs, size_t ElementSize, IROps IROp) {
  const auto Is128Bit = GetSrcSize(Op) == Core::CPUState::XMM_SSE_REG_SIZE;

  auto Src1 = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, !Is128Bit);
  auto Src2 = AVX128_LoadSource_WithOpSize(Op, Op->Src[1], Op->Flags, false);

  // Incoming element size for the shift source is always 8-bytes in the lower register.
  DeriveOp(Low, IROp, _VUShrSWide(OpSize::i128Bit, ElementSize, Src1.Low, Src2.Low));

  RefPair Result {};
  Result.Low = Low;

  if (Is128Bit) {
    Result.High = LoadZeroVector(OpSize::i128Bit);
  } else {
    DeriveOp(High, IROp, _VUShrSWide(OpSize::i128Bit, ElementSize, Src1.High, Src2.Low));
    Result.High = High;
  }

  AVX128_StoreResult_WithOpSize(Op, Op->Dest, Result);
}

void OpDispatchBuilder::AVX128_VectorShiftImmImpl(OpcodeArgs, size_t ElementSize, IROps IROp) {
  const auto DstSize = GetDstSize(Op);
  const auto Is128Bit = DstSize == Core::CPUState::XMM_SSE_REG_SIZE;
  const uint64_t ShiftConstant = Op->Src[1].Literal();

  auto Src = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, !Is128Bit);
  RefPair Result {};

  if (ShiftConstant == 0) [[unlikely]] {
    Result = Src;
  } else {
    DeriveOp(Low, IROp, _VUShrI(OpSize::i128Bit, ElementSize, Src.Low, ShiftConstant));
    Result.Low = Low;

    if (!Is128Bit) {
      DeriveOp(High, IROp, _VUShrI(OpSize::i128Bit, ElementSize, Src.High, ShiftConstant));
      Result.High = High;
    }
  }

  if (Is128Bit) {
    Result.High = LoadZeroVector(OpSize::i128Bit);
  }

  AVX128_StoreResult_WithOpSize(Op, Op->Dest, Result);
}

void OpDispatchBuilder::AVX128_VectorXOR(OpcodeArgs) {
  // Special case for vector xor with itself being the optimal way for x86 to zero vector registers.
  if (Op->Src[0].IsGPR() && Op->Src[1].IsGPR() && Op->Src[0].Data.GPR.GPR == Op->Src[1].Data.GPR.GPR) {
    AVX128_StoreResult_WithOpSize(Op, Op->Dest, AVX128_Zext(LoadZeroVector(OpSize::i128Bit)));
    return;
  }

  ///< Regular code path
  AVX128_VectorALU(Op, OP_VXOR, OpSize::i128Bit);
}

void OpDispatchBuilder::AVX128_VZERO(OpcodeArgs) {
  const auto DstSize = GetDstSize(Op);
  const auto IsVZEROALL = DstSize == Core::CPUState::XMM_AVX_REG_SIZE;
  const auto NumRegs = CTX->Config.Is64BitMode ? 16U : 8U;

  if (IsVZEROALL) {
    // NOTE: Despite the name being VZEROALL, this will still only ever
    //       zero out up to the first 16 registers (even on AVX-512, where we have 32 registers)
    Ref ZeroVector;

    for (uint32_t i = 0; i < NumRegs; i++) {
      // Explicitly not caching named vector zero. This ensures that every register gets movi #0.0 directly.
      ZeroVector = LoadUncachedZeroVector(OpSize::i128Bit);
      AVX128_StoreXMMRegister(i, ZeroVector, false);
    }

    // More efficient for non-SRA upper-halves to use a cached constant and store directly.
    for (uint32_t i = 0; i < NumRegs; i++) {
      AVX128_StoreXMMRegister(i, ZeroVector, true);
    }
  } else {
    // Likewise, VZEROUPPER will only ever zero only up to the first 16 registers
    const auto ZeroVector = LoadZeroVector(OpSize::i128Bit);
    for (uint32_t i = 0; i < NumRegs; i++) {
      AVX128_StoreXMMRegister(i, ZeroVector, true);
    }
  }
}

void OpDispatchBuilder::AVX128_MOVVectorNT(OpcodeArgs) {
  const auto SrcSize = GetSrcSize(Op);
  const auto Is128Bit = SrcSize == Core::CPUState::XMM_SSE_REG_SIZE;

  if (Op->Dest.IsGPR()) {
    ///< MOVNTDQA load non-temporal comes from SSE4.1 and is extended by AVX/AVX2.
    RefPair Src {};
    Ref SrcAddr = LoadSource(GPRClass, Op, Op->Src[0], Op->Flags, {.LoadData = false});
    Src.Low = _VLoadNonTemporal(OpSize::i128Bit, SrcAddr, 0);

    if (Is128Bit) {
      Src.High = LoadZeroVector(OpSize::i128Bit);
    } else {
      Src.High = _VLoadNonTemporal(OpSize::i128Bit, SrcAddr, 16);
    }
    AVX128_StoreResult_WithOpSize(Op, Op->Dest, Src);
  } else {
    auto Src = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, !Is128Bit, MemoryAccessType::STREAM);
    Ref Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.LoadData = false});

    if (Is128Bit) {
      // Single store non-temporal for 128-bit operations.
      _VStoreNonTemporal(OpSize::i128Bit, Src.Low, Dest, 0);
    } else {
      // For a 256-bit store, use a non-temporal store pair
      _VStoreNonTemporalPair(OpSize::i128Bit, Src.Low, Src.High, Dest, 0);
    }
  }
}

void OpDispatchBuilder::AVX128_MOVQ(OpcodeArgs) {
  RefPair Src {};
  if (Op->Src[0].IsGPR()) {
    Src = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, false);
  } else {
    Src.Low = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], OpSize::i64Bit, Op->Flags);
  }

  // This instruction is a bit special that if the destination is a register then it'll ZEXT the 64bit source to 256bit
  if (Op->Dest.IsGPR()) {
    // Zero bits [127:64] as well.
    Src.Low = _VMov(OpSize::i64Bit, Src.Low);
    Ref ZeroVector = LoadZeroVector(OpSize::i128Bit);
    Src.High = ZeroVector;
    AVX128_StoreResult_WithOpSize(Op, Op->Dest, Src);
  } else {
    StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Src.Low, OpSize::i64Bit, OpSize::i64Bit);
  }
}

void OpDispatchBuilder::AVX128_VMOVLP(OpcodeArgs) {
  auto Src1 = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, false);

  if (!Op->Dest.IsGPR()) {
    ///< VMOVLPS/PD mem64, xmm1
    StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Src1.Low, OpSize::i64Bit, OpSize::i64Bit);
  } else if (!Op->Src[1].IsGPR()) {
    ///< VMOVLPS/PD xmm1, xmm2, mem64
    // Bits[63:0] come from Src2[63:0]
    // Bits[127:64] come from Src1[127:64]
    auto Src2 = LoadSource_WithOpSize(GPRClass, Op, Op->Src[1], OpSize::i64Bit, Op->Flags, {.LoadData = false});
    Ref Result_Low = _VLoadVectorElement(OpSize::i128Bit, OpSize::i64Bit, Src1.Low, 0, Src2);
    Ref ZeroVector = LoadZeroVector(OpSize::i128Bit);

    AVX128_StoreResult_WithOpSize(Op, Op->Dest, RefPair {.Low = Result_Low, .High = ZeroVector});
  } else {
    ///< VMOVHLPS/PD xmm1, xmm2, xmm3
    auto Src2 = AVX128_LoadSource_WithOpSize(Op, Op->Src[1], Op->Flags, false);

    Ref Result_Low = _VInsElement(OpSize::i128Bit, OpSize::i64Bit, 0, 1, Src1.Low, Src2.Low);
    Ref ZeroVector = LoadZeroVector(OpSize::i128Bit);

    AVX128_StoreResult_WithOpSize(Op, Op->Dest, RefPair {.Low = Result_Low, .High = ZeroVector});
  }
}

void OpDispatchBuilder::AVX128_VMOVHP(OpcodeArgs) {
  auto Src1 = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, false);

  if (!Op->Dest.IsGPR()) {
    ///< VMOVHPS/PD mem64, xmm1
    // Need to store Bits[127:64]. Use a vector element store.
    auto Dest = LoadSource_WithOpSize(GPRClass, Op, Op->Dest, OpSize::i64Bit, Op->Flags, {.LoadData = false});
    _VStoreVectorElement(OpSize::i128Bit, OpSize::i64Bit, Src1.Low, 1, Dest);
  } else if (!Op->Src[1].IsGPR()) {
    ///< VMOVHPS/PD xmm2, xmm1, mem64
    auto Src2 = LoadSource_WithOpSize(GPRClass, Op, Op->Src[1], OpSize::i64Bit, Op->Flags, {.LoadData = false});

    // Bits[63:0] come from Src1[63:0]
    // Bits[127:64] come from Src2[63:0]
    Ref Result_Low = _VLoadVectorElement(OpSize::i128Bit, OpSize::i64Bit, Src1.Low, 1, Src2);
    Ref ZeroVector = LoadZeroVector(OpSize::i128Bit);

    AVX128_StoreResult_WithOpSize(Op, Op->Dest, RefPair {.Low = Result_Low, .High = ZeroVector});
  } else {
    // VMOVLHPS xmm1, xmm2, xmm3
    auto Src2 = AVX128_LoadSource_WithOpSize(Op, Op->Src[1], Op->Flags, false);

    Ref Result_Low = _VZip(OpSize::i128Bit, OpSize::i64Bit, Src1.Low, Src2.Low);
    Ref ZeroVector = LoadZeroVector(OpSize::i128Bit);

    AVX128_StoreResult_WithOpSize(Op, Op->Dest, RefPair {.Low = Result_Low, .High = ZeroVector});
  }
}

void OpDispatchBuilder::AVX128_VMOVDDUP(OpcodeArgs) {
  const auto SrcSize = GetSrcSize(Op);
  const auto Is128Bit = SrcSize == Core::CPUState::XMM_SSE_REG_SIZE;

  const auto IsSrcGPR = Op->Src[0].IsGPR();

  RefPair Src {};
  if (IsSrcGPR) {
    Src = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, !Is128Bit);
  } else {
    // Accesses from memory are a little weird.
    // 128-bit operation only loads 8-bytes.
    // 256-bit operation loads a full 32-bytes.
    if (Is128Bit) {
      Src.Low = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], OpSize::i64Bit, Op->Flags);
    } else {
      Src = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, true);
    }
  }

  if (Is128Bit) {
    // Duplicate Src[63:0] in to low 128-bits
    auto Result_Low = _VDupElement(OpSize::i128Bit, OpSize::i64Bit, Src.Low, 0);
    Ref ZeroVector = LoadZeroVector(OpSize::i128Bit);

    AVX128_StoreResult_WithOpSize(Op, Op->Dest, RefPair {.Low = Result_Low, .High = ZeroVector});
  } else {
    // Duplicate Src.Low[63:0] in to low 128-bits
    auto Result_Low = _VDupElement(OpSize::i128Bit, OpSize::i64Bit, Src.Low, 0);
    // Duplicate Src.High[63:0] in to high 128-bits
    auto Result_High = _VDupElement(OpSize::i128Bit, OpSize::i64Bit, Src.High, 0);
    AVX128_StoreResult_WithOpSize(Op, Op->Dest, RefPair {.Low = Result_Low, .High = Result_High});
  }
}

void OpDispatchBuilder::AVX128_VMOVSLDUP(OpcodeArgs) {
  AVX128_VectorUnaryImpl(Op, GetSrcSize(Op), OpSize::i32Bit,
                         [this](size_t ElementSize, Ref Src) { return _VTrn(OpSize::i128Bit, ElementSize, Src, Src); });
}

void OpDispatchBuilder::AVX128_VMOVSHDUP(OpcodeArgs) {
  AVX128_VectorUnaryImpl(Op, GetSrcSize(Op), OpSize::i32Bit,
                         [this](size_t ElementSize, Ref Src) { return _VTrn2(OpSize::i128Bit, ElementSize, Src, Src); });
}

template<size_t ElementSize>
void OpDispatchBuilder::AVX128_VBROADCAST(OpcodeArgs) {
  const auto DstSize = GetDstSize(Op);
  const auto Is128Bit = DstSize == Core::CPUState::XMM_SSE_REG_SIZE;
  RefPair Src {};

  if (Op->Src[0].IsGPR()) {
    Src = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, false);
    if (ElementSize != OpSize::i128Bit) {
      // Only duplicate if not VBROADCASTF128.
      Src.Low = _VDupElement(OpSize::i128Bit, ElementSize, Src.Low, 0);
    }
  } else {
    // Get the address to broadcast from into a GPR.
    Ref Address = MakeSegmentAddress(Op, Op->Src[0], CTX->GetGPRSize());
    Src.Low = _VBroadcastFromMem(OpSize::i128Bit, ElementSize, Address);
  }

  if (Is128Bit) {
    Src.High = LoadZeroVector(OpSize::i128Bit);
  } else {
    Src.High = Src.Low;
  }

  AVX128_StoreResult_WithOpSize(Op, Op->Dest, Src);
}

template<size_t ElementSize>
void OpDispatchBuilder::AVX128_VPUNPCKL(OpcodeArgs) {
  AVX128_VectorBinaryImpl(Op, GetSrcSize(Op), ElementSize,
                          [this](size_t _ElementSize, Ref Src1, Ref Src2) { return _VZip(OpSize::i128Bit, _ElementSize, Src1, Src2); });
}

template<size_t ElementSize>
void OpDispatchBuilder::AVX128_VPUNPCKH(OpcodeArgs) {
  AVX128_VectorBinaryImpl(Op, GetSrcSize(Op), ElementSize,
                          [this](size_t _ElementSize, Ref Src1, Ref Src2) { return _VZip2(OpSize::i128Bit, _ElementSize, Src1, Src2); });
}

void OpDispatchBuilder::AVX128_MOVVectorUnaligned(OpcodeArgs) {
  const auto SrcSize = GetSrcSize(Op);
  const auto Is128Bit = SrcSize == Core::CPUState::XMM_SSE_REG_SIZE;

  if (!Is128Bit && Op->Dest.IsGPR() && Op->Src[0].IsGPR() && Op->Dest.Data.GPR.GPR == Op->Src[0].Data.GPR.GPR) {
    // Nop
    return;
  }

  auto Src = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, !Is128Bit);

  if (Is128Bit) {
    Src.High = LoadZeroVector(OpSize::i128Bit);
  }

  AVX128_StoreResult_WithOpSize(Op, Op->Dest, Src);
}

template<size_t DstElementSize>
void OpDispatchBuilder::AVX128_InsertCVTGPR_To_FPR(OpcodeArgs) {
  const auto SrcSize = GetSrcSize(Op);
  const auto DstSize = GetDstSize(Op);

  auto Src1 = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, false);

  RefPair Result {};

  if (Op->Src[1].IsGPR()) {
    // If the source is a GPR then convert directly from the GPR.
    auto Src2 = LoadSource_WithOpSize(GPRClass, Op, Op->Src[1], CTX->GetGPRSize(), Op->Flags);
    Result.Low = _VSToFGPRInsert(OpSize::i128Bit, DstElementSize, SrcSize, Src1.Low, Src2, false);
  } else if (SrcSize != DstElementSize) {
    // If the source is from memory but the Source size and destination size aren't the same,
    // then it is more optimal to load in to a GPR and convert between GPR->FPR.
    // ARM GPR->FPR conversion supports different size source and destinations while FPR->FPR doesn't.
    auto Src2 = LoadSource(GPRClass, Op, Op->Src[1], Op->Flags);
    Result.Low = _VSToFGPRInsert(IR::SizeToOpSize(DstSize), DstElementSize, SrcSize, Src1.Low, Src2, false);
  } else {
    // In the case of cvtsi2s{s,d} where the source and destination are the same size,
    // then it is more optimal to load in to the FPR register directly and convert there.
    auto Src2 = AVX128_LoadSource_WithOpSize(Op, Op->Src[1], Op->Flags, false);
    // Always signed
    Result.Low = _VSToFVectorInsert(IR::SizeToOpSize(DstSize), DstElementSize, DstElementSize, Src1.Low, Src2.Low, false, false);
  }

  [[maybe_unused]] const auto Is128Bit = DstSize == Core::CPUState::XMM_SSE_REG_SIZE;
  LOGMAN_THROW_A_FMT(Is128Bit, "Programming Error: This should never occur!");
  Result.High = LoadZeroVector(OpSize::i128Bit);

  AVX128_StoreResult_WithOpSize(Op, Op->Dest, Result);
}

template<size_t SrcElementSize, bool HostRoundingMode>
void OpDispatchBuilder::AVX128_CVTFPR_To_GPR(OpcodeArgs) {
  // If loading a vector, use the full size, so we don't
  // unnecessarily zero extend the vector. Otherwise, if
  // memory, then we want to load the element size exactly.
  RefPair Src {};
  if (Op->Src[0].IsGPR()) {
    Src = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, false);
  } else {
    Src.Low = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], GetSrcSize(Op), Op->Flags);
  }

  // GPR size is determined by REX.W
  // Source Element size is determined by instruction
  size_t GPRSize = GetDstSize(Op);

  Ref Result {};
  if constexpr (HostRoundingMode) {
    Result = _Float_ToGPR_S(GPRSize, SrcElementSize, Src.Low);
  } else {
    Result = _Float_ToGPR_ZS(GPRSize, SrcElementSize, Src.Low);
  }

  StoreResult_WithOpSize(GPRClass, Op, Op->Dest, Result, GPRSize, -1);
}

void OpDispatchBuilder::AVX128_VANDN(OpcodeArgs) {
  AVX128_VectorBinaryImpl(Op, GetSrcSize(Op), OpSize::i128Bit,
                          [this](size_t _ElementSize, Ref Src1, Ref Src2) { return _VAndn(OpSize::i128Bit, _ElementSize, Src2, Src1); });
}

template<size_t ElementSize>
void OpDispatchBuilder::AVX128_VPACKSS(OpcodeArgs) {
  AVX128_VectorBinaryImpl(Op, GetSrcSize(Op), ElementSize,
                          [this](size_t _ElementSize, Ref Src1, Ref Src2) { return _VSQXTNPair(OpSize::i128Bit, _ElementSize, Src1, Src2); });
}

template<size_t ElementSize>
void OpDispatchBuilder::AVX128_VPACKUS(OpcodeArgs) {
  AVX128_VectorBinaryImpl(Op, GetSrcSize(Op), ElementSize,
                          [this](size_t _ElementSize, Ref Src1, Ref Src2) { return _VSQXTUNPair(OpSize::i128Bit, _ElementSize, Src1, Src2); });
}

Ref OpDispatchBuilder::AVX128_PSIGNImpl(size_t ElementSize, Ref Src1, Ref Src2) {
  Ref Control = _VSQSHL(OpSize::i128Bit, ElementSize, Src2, (ElementSize * 8) - 1);
  Control = _VSRSHR(OpSize::i128Bit, ElementSize, Control, (ElementSize * 8) - 1);
  return _VMul(OpSize::i128Bit, ElementSize, Src1, Control);
}

template<size_t ElementSize>
void OpDispatchBuilder::AVX128_VPSIGN(OpcodeArgs) {
  AVX128_VectorBinaryImpl(Op, GetSrcSize(Op), ElementSize,
                          [this](size_t _ElementSize, Ref Src1, Ref Src2) { return AVX128_PSIGNImpl(_ElementSize, Src1, Src2); });
}

template<size_t ElementSize>
void OpDispatchBuilder::AVX128_UCOMISx(OpcodeArgs) {
  const auto SrcSize = Op->Src[0].IsGPR() ? GetGuestVectorLength() : GetSrcSize(Op);

  auto Src1 = AVX128_LoadSource_WithOpSize(Op, Op->Dest, Op->Flags, false);

  RefPair Src2 {};

  // Careful here, if the source is from a GPR then we want to load the full 128-bit lower half.
  // If it is memory then we only want to load the element size.
  if (Op->Src[0].IsGPR()) {
    Src2 = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, false);
  } else {
    Src2.Low = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], SrcSize, Op->Flags);
  }

  Comiss(ElementSize, Src1.Low, Src2.Low);
}

void OpDispatchBuilder::AVX128_VectorScalarInsertALU(OpcodeArgs, FEXCore::IR::IROps IROp, size_t ElementSize) {
  // We load the full vector width when dealing with a source vector,
  // so that we don't do any unnecessary zero extension to the scalar
  // element that we're going to operate on.
  const auto SrcSize = GetSrcSize(Op);

  auto Src1 = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, false);
  RefPair Src2 {};
  if (Op->Src[1].IsGPR()) {
    Src2 = AVX128_LoadSource_WithOpSize(Op, Op->Src[1], Op->Flags, false);
  } else {
    Src2.Low = LoadSource_WithOpSize(FPRClass, Op, Op->Src[1], SrcSize, Op->Flags);
  }

  // If OpSize == ElementSize then it only does the lower scalar op
  DeriveOp(Result_Low, IROp, _VFAddScalarInsert(OpSize::i128Bit, ElementSize, Src1.Low, Src2.Low, false));
  auto High = LoadZeroVector(OpSize::i128Bit);
  AVX128_StoreResult_WithOpSize(Op, Op->Dest, RefPair {.Low = Result_Low, .High = High});
}

template<size_t ElementSize>
void OpDispatchBuilder::AVX128_VFCMP(OpcodeArgs) {
  const uint8_t CompType = Op->Src[2].Literal();

  struct {
    FEXCore::X86Tables::DecodedOp Op;
    uint8_t CompType {};
  } Capture {
    .Op = Op,
    .CompType = CompType,
  };

  AVX128_VectorBinaryImpl(Op, GetSrcSize(Op), ElementSize, [this, &Capture](size_t _ElementSize, Ref Src1, Ref Src2) {
    return VFCMPOpImpl(Capture.Op, _ElementSize, Src1, Src2, Capture.CompType);
  });
}

template<size_t ElementSize>
void OpDispatchBuilder::AVX128_InsertScalarFCMP(OpcodeArgs) {
  // We load the full vector width when dealing with a source vector,
  // so that we don't do any unnecessary zero extension to the scalar
  // element that we're going to operate on.
  const auto SrcSize = GetSrcSize(Op);

  auto Src1 = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, false);
  RefPair Src2 {};

  if (Op->Src[1].IsGPR()) {
    Src2 = AVX128_LoadSource_WithOpSize(Op, Op->Src[1], Op->Flags, false);
  } else {
    Src2.Low = LoadSource_WithOpSize(FPRClass, Op, Op->Src[1], SrcSize, Op->Flags);
  }

  const uint8_t CompType = Op->Src[2].Literal();

  RefPair Result {};
  Result.Low = InsertScalarFCMPOpImpl(OpSize::i128Bit, OpSize::i128Bit, ElementSize, Src1.Low, Src2.Low, CompType, false);
  Result.High = LoadZeroVector(OpSize::i128Bit);
  AVX128_StoreResult_WithOpSize(Op, Op->Dest, Result);
}

void OpDispatchBuilder::AVX128_MOVBetweenGPR_FPR(OpcodeArgs) {
  if (Op->Dest.IsGPR() && Op->Dest.Data.GPR.GPR >= FEXCore::X86State::REG_XMM_0) {
    ///< XMM <- Reg/Mem

    RefPair Result {};
    if (Op->Src[0].IsGPR()) {
      // Loading from GPR and moving to Vector.
      Ref Src = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], CTX->GetGPRSize(), Op->Flags);
      // zext to 128bit
      Result.Low = _VCastFromGPR(OpSize::i128Bit, GetSrcSize(Op), Src);
    } else {
      // Loading from Memory as a scalar. Zero extend
      Result.Low = LoadSource(FPRClass, Op, Op->Src[0], Op->Flags);
    }

    Result.High = LoadZeroVector(OpSize::i128Bit);
    AVX128_StoreResult_WithOpSize(Op, Op->Dest, Result);
  } else {
    ///< Reg/Mem <- XMM
    auto Src = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, false);

    if (Op->Dest.IsGPR()) {
      auto ElementSize = GetDstSize(Op);
      // Extract element from GPR. Zero extending in the process.
      Src.Low = _VExtractToGPR(GetSrcSize(Op), ElementSize, Src.Low, 0);
      StoreResult(GPRClass, Op, Op->Dest, Src.Low, -1);
    } else {
      // Storing first element to memory.
      Ref Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.LoadData = false});
      _StoreMem(FPRClass, GetDstSize(Op), Dest, Src.Low, 1);
    }
  }
}

template<size_t ElementSize>
void OpDispatchBuilder::AVX128_PExtr(OpcodeArgs) {
  const auto DstSize = GetDstSize(Op);

  auto Src = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, false);
  uint64_t Index = Op->Src[1].Literal();

  // Fixup of 32-bit element size.
  // When the element size is 32-bit then it can be overriden as 64-bit because the encoding of PEXTRD/PEXTRQ
  // is the same except that REX.W or VEX.W is set to 1. Incredibly frustrating.
  // Use the destination size as the element size in this case.
  size_t OverridenElementSize = ElementSize;
  if constexpr (ElementSize == 4) {
    OverridenElementSize = DstSize;
  }

  // AVX version only operates on 128-bit.
  const uint8_t NumElements = std::min<uint8_t>(GetSrcSize(Op), 16) / OverridenElementSize;
  Index &= NumElements - 1;

  if (Op->Dest.IsGPR()) {
    const uint8_t GPRSize = CTX->GetGPRSize();
    // Extract already zero extends the result.
    Ref Result = _VExtractToGPR(OpSize::i128Bit, OverridenElementSize, Src.Low, Index);
    StoreResult_WithOpSize(GPRClass, Op, Op->Dest, Result, GPRSize, -1);
    return;
  }

  // If we are storing to memory then we store the size of the element extracted
  Ref Dest = LoadSource(GPRClass, Op, Op->Dest, Op->Flags, {.LoadData = false});
  _VStoreVectorElement(OpSize::i128Bit, OverridenElementSize, Src.Low, Index, Dest);
}

void OpDispatchBuilder::AVX128_ExtendVectorElements(OpcodeArgs, size_t ElementSize, size_t DstElementSize, bool Signed) {
  const auto DstSize = GetDstSize(Op);

  const auto GetSrc = [&] {
    if (Op->Src[0].IsGPR()) {
      return AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, false).Low;
    } else {
      // For memory operands the 256-bit variant loads twice the size specified in the table.
      const auto Is256Bit = DstSize == Core::CPUState::XMM_AVX_REG_SIZE;
      const auto SrcSize = GetSrcSize(Op);
      const auto LoadSize = Is256Bit ? SrcSize * 2 : SrcSize;

      return LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], LoadSize, Op->Flags);
    }
  };

  auto Transform = [=, this](Ref Src) {
    for (size_t CurrentElementSize = ElementSize; CurrentElementSize != DstElementSize; CurrentElementSize <<= 1) {
      if (Signed) {
        Src = _VSXTL(OpSize::i128Bit, CurrentElementSize, Src);
      } else {
        Src = _VUXTL(OpSize::i128Bit, CurrentElementSize, Src);
      }
    }
    return Src;
  };

  Ref Src = GetSrc();
  RefPair Result {};

  if (DstSize == OpSize::i128Bit) {
    // 128-bit operation is easy, it stays within the single register.
    Result.Low = Transform(Src);
  } else {
    // 256-bit operation is a bit special. It splits the incoming source between lower and upper registers.
    size_t TotalElementCount = OpSize::i256Bit / DstElementSize;
    size_t TotalElementsToSplitSize = (TotalElementCount / 2) * ElementSize;

    // Split the number of elements in half between lower and upper.
    Ref SrcHigh = _VDupElement(OpSize::i128Bit, TotalElementsToSplitSize, Src, 1);
    Result.Low = Transform(Src);
    Result.High = Transform(SrcHigh);
  }

  if (DstSize == OpSize::i128Bit) {
    // Regular zero-extending semantics.
    Result.High = LoadZeroVector(OpSize::i128Bit);
  }

  AVX128_StoreResult_WithOpSize(Op, Op->Dest, Result);
}

template<size_t ElementSize>
void OpDispatchBuilder::AVX128_MOVMSK(OpcodeArgs) {
  const auto SrcSize = GetSrcSize(Op);
  const auto Is128Bit = SrcSize == Core::CPUState::XMM_SSE_REG_SIZE;

  auto Src = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, !Is128Bit);

  auto Mask8Byte = [this](Ref Src) {
    // UnZip2 the 64-bit elements as 32-bit to get the sign bits closer.
    // Sign bits are now in bit positions 31 and 63 after this.
    Src = _VUnZip2(OpSize::i128Bit, OpSize::i32Bit, Src, Src);

    // Extract the low 64-bits to GPR in one move.
    Ref GPR = _VExtractToGPR(OpSize::i128Bit, OpSize::i64Bit, Src, 0);
    // BFI the sign bit in 31 in to 62.
    // Inserting the full lower 32-bits offset 31 so the sign bit ends up at offset 63.
    GPR = _Bfi(OpSize::i64Bit, 32, 31, GPR, GPR);
    // Shift right to only get the two sign bits we care about.
    return _Lshr(OpSize::i64Bit, GPR, _Constant(62));
  };

  auto Mask4Byte = [this](Ref Src) {
    // Shift all the sign bits to the bottom of their respective elements.
    Src = _VUShrI(OpSize::i128Bit, OpSize::i32Bit, Src, 31);
    // Load the specific 128-bit movmskps shift elements operator.
    auto ConstantUSHL = LoadAndCacheNamedVectorConstant(OpSize::i128Bit, NAMED_VECTOR_MOVMSKPS_SHIFT);
    // Shift the sign bits in to specific locations.
    Src = _VUShl(OpSize::i128Bit, OpSize::i32Bit, Src, ConstantUSHL, false);
    // Add across the vector so the sign bits will end up in bits [3:0]
    Src = _VAddV(OpSize::i128Bit, OpSize::i32Bit, Src);
    // Extract to a GPR.
    return _VExtractToGPR(OpSize::i128Bit, OpSize::i32Bit, Src, 0);
  };

  Ref GPR {};
  if (SrcSize == 16 && ElementSize == 8) {
    GPR = Mask8Byte(Src.Low);
  } else if (SrcSize == 16 && ElementSize == 4) {
    GPR = Mask4Byte(Src.Low);
  } else if (ElementSize == 4) {
    auto GPRLow = Mask4Byte(Src.Low);
    auto GPRHigh = Mask4Byte(Src.High);
    GPR = _Orlshl(OpSize::i64Bit, GPRLow, GPRHigh, 4);
  } else {
    auto GPRLow = Mask8Byte(Src.Low);
    auto GPRHigh = Mask8Byte(Src.High);
    GPR = _Orlshl(OpSize::i64Bit, GPRLow, GPRHigh, 2);
  }
  StoreResult_WithOpSize(GPRClass, Op, Op->Dest, GPR, CTX->GetGPRSize(), -1);
}

void OpDispatchBuilder::AVX128_MOVMSKB(OpcodeArgs) {
  const auto SrcSize = GetSrcSize(Op);
  const auto Is128Bit = SrcSize == Core::CPUState::XMM_SSE_REG_SIZE;

  auto Src = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, !Is128Bit);
  Ref VMask = LoadAndCacheNamedVectorConstant(OpSize::i128Bit, NAMED_VECTOR_MOVMASKB);

  auto Mask1Byte = [this](Ref Src, Ref VMask) {
    auto VCMP = _VCMPLTZ(OpSize::i128Bit, OpSize::i8Bit, Src);
    auto VAnd = _VAnd(OpSize::i128Bit, OpSize::i8Bit, VCMP, VMask);

    auto VAdd1 = _VAddP(OpSize::i128Bit, OpSize::i8Bit, VAnd, VAnd);
    auto VAdd2 = _VAddP(OpSize::i128Bit, OpSize::i8Bit, VAdd1, VAdd1);
    auto VAdd3 = _VAddP(OpSize::i64Bit, OpSize::i8Bit, VAdd2, VAdd2);

    ///< 16-bits of data per 128-bit
    return _VExtractToGPR(OpSize::i128Bit, 2, VAdd3, 0);
  };

  Ref Result = Mask1Byte(Src.Low, VMask);

  if (!Is128Bit) {
    auto ResultHigh = Mask1Byte(Src.High, VMask);
    Result = _Orlshl(OpSize::i64Bit, Result, ResultHigh, 16);
  }

  StoreResult(GPRClass, Op, Result, -1);
}

void OpDispatchBuilder::AVX128_PINSRImpl(OpcodeArgs, size_t ElementSize, const X86Tables::DecodedOperand& Src1Op,
                                         const X86Tables::DecodedOperand& Src2Op, const X86Tables::DecodedOperand& Imm) {
  const auto NumElements = OpSize::i128Bit / ElementSize;
  const uint64_t Index = Imm.Literal() & (NumElements - 1);
  auto Src1 = AVX128_LoadSource_WithOpSize(Op, Src1Op, Op->Flags, false);

  RefPair Result {};

  if (Src2Op.IsGPR()) {
    // If the source is a GPR then convert directly from the GPR.
    auto Src2 = LoadSource_WithOpSize(GPRClass, Op, Src2Op, CTX->GetGPRSize(), Op->Flags);
    Result.Low = _VInsGPR(OpSize::i128Bit, ElementSize, Index, Src1.Low, Src2);
  } else {
    // If loading from memory then we only load the element size
    auto Src2 = LoadSource_WithOpSize(GPRClass, Op, Src2Op, ElementSize, Op->Flags, {.LoadData = false});
    Result.Low = _VLoadVectorElement(OpSize::i128Bit, ElementSize, Src1.Low, Index, Src2);
  }

  Result.High = LoadZeroVector(OpSize::i128Bit);
  AVX128_StoreResult_WithOpSize(Op, Op->Dest, Result);
}

void OpDispatchBuilder::AVX128_VPINSRB(OpcodeArgs) {
  AVX128_PINSRImpl(Op, 1, Op->Src[0], Op->Src[1], Op->Src[2]);
}

void OpDispatchBuilder::AVX128_VPINSRW(OpcodeArgs) {
  AVX128_PINSRImpl(Op, 2, Op->Src[0], Op->Src[1], Op->Src[2]);
}

void OpDispatchBuilder::AVX128_VPINSRDQ(OpcodeArgs) {
  const auto SrcSize = GetSrcSize(Op);
  AVX128_PINSRImpl(Op, SrcSize, Op->Src[0], Op->Src[1], Op->Src[2]);
}

void OpDispatchBuilder::AVX128_VariableShiftImpl(OpcodeArgs, IROps IROp) {
  AVX128_VectorBinaryImpl(Op, GetDstSize(Op), GetSrcSize(Op), [this, IROp](size_t ElementSize, Ref Src1, Ref Src2) {
    DeriveOp(Shift, IROp, _VUShr(OpSize::i128Bit, ElementSize, Src1, Src2, true));
    return Shift;
  });
}

void OpDispatchBuilder::AVX128_ShiftDoubleImm(OpcodeArgs, ShiftDirection Dir) {
  const auto DstSize = GetDstSize(Op);
  const auto Is128Bit = DstSize == Core::CPUState::XMM_SSE_REG_SIZE;
  const bool Right = Dir == ShiftDirection::RIGHT;

  const uint64_t Shift = Op->Src[1].Literal();
  const uint64_t ExtrShift = Right ? Shift : OpSize::i128Bit - Shift;

  auto Src = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, !Is128Bit);

  RefPair Result {};
  if (Shift == 0) [[unlikely]] {
    Result = Src;
  } else if (Shift >= Core::CPUState::XMM_SSE_REG_SIZE) {
    Result.Low = LoadZeroVector(OpSize::i128Bit);
    Result.High = Result.High;
  } else {
    Ref ZeroVector = LoadZeroVector(OpSize::i128Bit);
    RefPair Zero {ZeroVector, ZeroVector};
    RefPair Src1 = Right ? Zero : Src;
    RefPair Src2 = Right ? Src : Zero;

    Result.Low = _VExtr(OpSize::i128Bit, OpSize::i8Bit, Src1.Low, Src2.Low, ExtrShift);
    if (!Is128Bit) {
      Result.High = _VExtr(OpSize::i128Bit, OpSize::i8Bit, Src1.High, Src2.High, ExtrShift);
    }
  }

  if (Is128Bit) {
    Result.High = LoadZeroVector(OpSize::i128Bit);
  }

  AVX128_StoreResult_WithOpSize(Op, Op->Dest, Result);
}

void OpDispatchBuilder::AVX128_VINSERT(OpcodeArgs) {
  const auto DstSize = GetDstSize(Op);
  const auto Is128Bit = DstSize == Core::CPUState::XMM_SSE_REG_SIZE;
  const auto Selector = Op->Src[2].Literal() & 1;

  auto Result = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, !Is128Bit);
  auto Src2 = AVX128_LoadSource_WithOpSize(Op, Op->Src[1], Op->Flags, false);

  if (Selector == 0) {
    // Insert in to Low bits
    Result.Low = Src2.Low;
  } else {
    // Insert in to the High bits
    Result.High = Src2.Low;
  }

  AVX128_StoreResult_WithOpSize(Op, Op->Dest, Result);
}

void OpDispatchBuilder::AVX128_VINSERTPS(OpcodeArgs) {
  Ref Result = InsertPSOpImpl(Op, Op->Src[0], Op->Src[1], Op->Src[2]);
  AVX128_StoreResult_WithOpSize(Op, Op->Dest, AVX128_Zext(Result));
}

template<size_t ElementSize>
void OpDispatchBuilder::AVX128_VPHSUB(OpcodeArgs) {
  AVX128_VectorBinaryImpl(Op, GetDstSize(Op), ElementSize,
                          [this](size_t _ElementSize, Ref Src1, Ref Src2) { return PHSUBOpImpl(OpSize::i128Bit, Src1, Src2, _ElementSize); });
}

void OpDispatchBuilder::AVX128_VPHSUBSW(OpcodeArgs) {
  AVX128_VectorBinaryImpl(Op, GetDstSize(Op), OpSize::i16Bit,
                          [this](size_t _ElementSize, Ref Src1, Ref Src2) { return PHSUBSOpImpl(OpSize::i128Bit, Src1, Src2); });
}

template<size_t ElementSize>
void OpDispatchBuilder::AVX128_VADDSUBP(OpcodeArgs) {
  AVX128_VectorBinaryImpl(Op, GetDstSize(Op), ElementSize, [this](size_t _ElementSize, Ref Src1, Ref Src2) {
    return ADDSUBPOpImpl(OpSize::i128Bit, _ElementSize, Src1, Src2);
  });
}

template<size_t ElementSize, bool Signed>
void OpDispatchBuilder::AVX128_VPMULL(OpcodeArgs) {
  static_assert(ElementSize == sizeof(uint32_t), "Currently only handles 32-bit -> 64-bit");

  AVX128_VectorBinaryImpl(Op, GetDstSize(Op), ElementSize, [this](size_t _ElementSize, Ref Src1, Ref Src2) -> Ref {
    return PMULLOpImpl(OpSize::i128Bit, ElementSize, Signed, Src1, Src2);
  });
}

void OpDispatchBuilder::AVX128_VPMULHRSW(OpcodeArgs) {
  AVX128_VectorBinaryImpl(Op, GetDstSize(Op), OpSize::i16Bit,
                          [this](size_t _ElementSize, Ref Src1, Ref Src2) -> Ref { return PMULHRSWOpImpl(OpSize::i128Bit, Src1, Src2); });
}

template<bool Signed>
void OpDispatchBuilder::AVX128_VPMULHW(OpcodeArgs) {
  AVX128_VectorBinaryImpl(Op, GetDstSize(Op), OpSize::i16Bit, [this](size_t _ElementSize, Ref Src1, Ref Src2) -> Ref {
    if (Signed) {
      return _VSMulH(OpSize::i128Bit, _ElementSize, Src1, Src2);
    } else {
      return _VUMulH(OpSize::i128Bit, _ElementSize, Src1, Src2);
    }
  });
}

template<size_t DstElementSize, size_t SrcElementSize>
void OpDispatchBuilder::AVX128_InsertScalar_CVT_Float_To_Float(OpcodeArgs) {
  // Gotta be careful with this operation.
  // It inserts in to the lowest element, retaining the remainder of the lower 128-bits.
  // Then zero extends the top 128-bit.
  const auto SrcSize = GetSrcSize(Op);
  auto Src1 = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, false);
  Ref Src2 = LoadSource_WithOpSize(FPRClass, Op, Op->Src[1], SrcSize, Op->Flags, {.AllowUpperGarbage = true});

  Ref Result = _VFToFScalarInsert(OpSize::i128Bit, DstElementSize, SrcElementSize, Src1.Low, Src2, false);
  AVX128_StoreResult_WithOpSize(Op, Op->Dest, AVX128_Zext(Result));
}

template<size_t DstElementSize, size_t SrcElementSize>
void OpDispatchBuilder::AVX128_Vector_CVT_Float_To_Float(OpcodeArgs) {
  const auto SrcSize = GetSrcSize(Op);
  const auto DstSize = GetDstSize(Op);

  const auto IsFloatSrc = SrcElementSize == 4;
  auto Is128BitSrc = SrcSize == Core::CPUState::XMM_SSE_REG_SIZE;
  auto Is128BitDst = DstSize == Core::CPUState::XMM_SSE_REG_SIZE;

  ///< Decompose correctly.
  if (DstElementSize > SrcElementSize && !Is128BitDst) {
    Is128BitSrc = true;
  } else if (SrcElementSize > DstElementSize && !Is128BitSrc) {
    Is128BitDst = true;
  }

  const auto LoadSize = IsFloatSrc && !Op->Src[0].IsGPR() ? SrcSize / 2 : SrcSize;

  RefPair Src {};
  if (Op->Src[0].IsGPR() || LoadSize >= OpSize::i128Bit) {
    Src = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, !Is128BitSrc);
  } else {
    // Handle 64-bit memory source.
    // In the case of cvtps2pd xmm, m64.
    Src.Low = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], LoadSize, Op->Flags);
  }

  RefPair Result {};

  auto TransformLow = [this](Ref Src) -> Ref {
    return _Vector_FToF(OpSize::i128Bit, DstElementSize, Src, SrcElementSize);
  };

  auto TransformHigh = [this](Ref Src) -> Ref {
    return _VFCVTL2(OpSize::i128Bit, SrcElementSize, Src);
  };

  Result.Low = TransformLow(Src.Low);
  if (Is128BitSrc) {
    if (Is128BitDst) {
      // cvtps2pd xmm, xmm or cvtpd2ps xmm, xmm
      // Done here
    } else {
      LOGMAN_THROW_A_FMT(DstElementSize > SrcElementSize, "cvtpd2ps ymm, xmm doesn't exist");

      // cvtps2pd ymm, xmm
      Result.High = TransformHigh(Src.Low);
    }
  } else {
    // 256-bit src
    LOGMAN_THROW_A_FMT(Is128BitDst, "Not real: cvt{ps2pd,pd2ps} ymm, ymm");
    LOGMAN_THROW_A_FMT(DstElementSize < SrcElementSize, "cvtps2pd xmm, ymm doesn't exist");

    // cvtpd2ps xmm, ymm
    Result.Low = _VInsElement(OpSize::i128Bit, OpSize::i64Bit, 1, 0, Result.Low, TransformLow(Src.High));
  }

  if (Is128BitDst) {
    Result = AVX128_Zext(Result.Low);
  }

  AVX128_StoreResult_WithOpSize(Op, Op->Dest, Result);
}

template<size_t SrcElementSize, bool Narrow, bool HostRoundingMode>
void OpDispatchBuilder::AVX128_Vector_CVT_Float_To_Int(OpcodeArgs) {
  const auto SrcSize = GetSrcSize(Op);

  const auto Is128BitSrc = SrcSize == Core::CPUState::XMM_SSE_REG_SIZE;

  // VCVTPD2DQ/VCVTTPD2DQ only use the bottom lane, even for the 256-bit version.
  auto Src = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, !Is128BitSrc);
  RefPair Result {};

  if (SrcElementSize == 8 && Narrow) {
    ///< Special case for VCVTPD2DQ/CVTTPD2DQ because it has weird rounding requirements.
    Result.Low = _Vector_F64ToI32(OpSize::i128Bit, Src.Low, HostRoundingMode ? Round_Host : Round_Towards_Zero, Is128BitSrc);

    if (!Is128BitSrc) {
      // Also convert the upper 128-bit lane
      auto ResultHigh = _Vector_F64ToI32(OpSize::i128Bit, Src.High, HostRoundingMode ? Round_Host : Round_Towards_Zero, false);

      // Zip the two halves together in to the lower 128-bits
      Result.Low = _VZip(OpSize::i128Bit, OpSize::i64Bit, Result.Low, ResultHigh);
    }
  } else {
    auto Convert = [this](Ref Src) -> Ref {
      size_t ElementSize = SrcElementSize;
      if (Narrow) {
        ElementSize >>= 1;
        Src = _Vector_FToF(OpSize::i128Bit, ElementSize, Src, SrcElementSize);
      }

      if (HostRoundingMode) {
        return _Vector_FToS(OpSize::i128Bit, ElementSize, Src);
      } else {
        return _Vector_FToZS(OpSize::i128Bit, ElementSize, Src);
      }
    };

    Result.Low = Convert(Src.Low);

    if (!Is128BitSrc) {
      if (!Narrow) {
        Result.High = Convert(Src.High);
      } else {
        Result.Low = _VInsElement(OpSize::i128Bit, OpSize::i64Bit, 1, 0, Result.Low, Convert(Src.High));
      }
    }
  }

  if (Narrow || Is128BitSrc) {
    // Zero the upper 128-bit lane of the result.
    Result = AVX128_Zext(Result.Low);
  }

  AVX128_StoreResult_WithOpSize(Op, Op->Dest, Result);
}

template<size_t SrcElementSize, bool Widen>
void OpDispatchBuilder::AVX128_Vector_CVT_Int_To_Float(OpcodeArgs) {
  const size_t Size = GetDstSize(Op);
  const auto Is128Bit = Size == Core::CPUState::XMM_SSE_REG_SIZE;

  RefPair Src = [&] {
    if (Widen && !Op->Src[0].IsGPR()) {
      // If loading a vector, use the full size, so we don't
      // unnecessarily zero extend the vector. Otherwise, if
      // memory, then we want to load the element size exactly.
      const auto LoadSize = 8 * (Size / 16);
      return RefPair {.Low = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], LoadSize, Op->Flags)};
    } else {
      return AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, !Is128Bit);
    }
  }();

  auto Convert = [this](size_t Size, Ref Src, IROps Op) -> Ref {
    size_t ElementSize = SrcElementSize;
    if (Widen) {
      DeriveOp(Extended, Op, _VSXTL(Size, ElementSize, Src));
      Src = Extended;
      ElementSize <<= 1;
    }

    return _Vector_SToF(Size, ElementSize, Src);
  };

  RefPair Result {};
  Result.Low = Convert(Size, Src.Low, IROps::OP_VSXTL);

  if (Is128Bit) {
    Result = AVX128_Zext(Result.Low);
  } else {
    if (Widen) {
      Result.High = Convert(Size, Src.Low, IROps::OP_VSXTL2);
    } else {
      Result.High = Convert(Size, Src.High, IROps::OP_VSXTL);
    }
  }

  AVX128_StoreResult_WithOpSize(Op, Op->Dest, Result);
}

void OpDispatchBuilder::AVX128_VEXTRACT128(OpcodeArgs) {
  const auto DstIsXMM = Op->Dest.IsGPR();
  const auto Selector = Op->Src[1].Literal() & 0b1;

  ///< TODO: Once we support loading only upper-half of the ymm register we can load the half depending on selection literal.
  auto Src = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, true);

  RefPair Result {};
  if (Selector == 0) {
    Result.Low = Src.Low;
  } else {
    Result.Low = Src.High;
  }

  if (DstIsXMM) {
    // Only zero the upper-half when destination is XMM, otherwise this is a memory store.
    Result = AVX128_Zext(Result.Low);
  }

  AVX128_StoreResult_WithOpSize(Op, Op->Dest, Result);
}

void OpDispatchBuilder::AVX128_VAESImc(OpcodeArgs) {
  ///< 128-bit only.
  AVX128_VectorUnaryImpl(Op, OpSize::i128Bit, OpSize::i128Bit, [this](size_t, Ref Src) { return _VAESImc(Src); });
}

void OpDispatchBuilder::AVX128_VAESEnc(OpcodeArgs) {
  AVX128_VectorTrinaryImpl(Op, GetDstSize(Op), OpSize::i128Bit, LoadZeroVector(OpSize::i128Bit),
                           [this](size_t, Ref Src1, Ref Src2, Ref Src3) { return _VAESEnc(OpSize::i128Bit, Src1, Src2, Src3); });
}

void OpDispatchBuilder::AVX128_VAESEncLast(OpcodeArgs) {
  AVX128_VectorTrinaryImpl(Op, GetDstSize(Op), OpSize::i128Bit, LoadZeroVector(OpSize::i128Bit),
                           [this](size_t, Ref Src1, Ref Src2, Ref Src3) { return _VAESEncLast(OpSize::i128Bit, Src1, Src2, Src3); });
}

void OpDispatchBuilder::AVX128_VAESDec(OpcodeArgs) {
  AVX128_VectorTrinaryImpl(Op, GetDstSize(Op), OpSize::i128Bit, LoadZeroVector(OpSize::i128Bit),
                           [this](size_t, Ref Src1, Ref Src2, Ref Src3) { return _VAESDec(OpSize::i128Bit, Src1, Src2, Src3); });
}

void OpDispatchBuilder::AVX128_VAESDecLast(OpcodeArgs) {
  AVX128_VectorTrinaryImpl(Op, GetDstSize(Op), OpSize::i128Bit, LoadZeroVector(OpSize::i128Bit),
                           [this](size_t, Ref Src1, Ref Src2, Ref Src3) { return _VAESDecLast(OpSize::i128Bit, Src1, Src2, Src3); });
}

void OpDispatchBuilder::AVX128_VAESKeyGenAssist(OpcodeArgs) {
  ///< 128-bit only.
  const uint64_t RCON = Op->Src[1].Literal();
  auto ZeroRegister = LoadZeroVector(OpSize::i128Bit);
  auto KeyGenSwizzle = LoadAndCacheNamedVectorConstant(OpSize::i128Bit, NAMED_VECTOR_AESKEYGENASSIST_SWIZZLE);

  struct {
    Ref ZeroRegister;
    Ref KeyGenSwizzle;
    uint64_t RCON;
  } Capture {
    .ZeroRegister = ZeroRegister,
    .KeyGenSwizzle = KeyGenSwizzle,
    .RCON = RCON,
  };

  AVX128_VectorUnaryImpl(Op, OpSize::i128Bit, OpSize::i128Bit, [this, &Capture](size_t, Ref Src) {
    return _VAESKeyGenAssist(Src, Capture.KeyGenSwizzle, Capture.ZeroRegister, Capture.RCON);
  });
}

void OpDispatchBuilder::AVX128_VPCMPESTRI(OpcodeArgs) {
  PCMPXSTRXOpImpl(Op, true, false);

  ///< Does not zero anything.
}

void OpDispatchBuilder::AVX128_VPCMPESTRM(OpcodeArgs) {
  PCMPXSTRXOpImpl(Op, true, true);

  ///< Zero the upper 128-bits of hardcoded YMM0
  AVX128_StoreXMMRegister(0, LoadZeroVector(OpSize::i128Bit), true);
}

void OpDispatchBuilder::AVX128_VPCMPISTRI(OpcodeArgs) {
  PCMPXSTRXOpImpl(Op, false, false);

  ///< Does not zero anything.
}

void OpDispatchBuilder::AVX128_VPCMPISTRM(OpcodeArgs) {
  PCMPXSTRXOpImpl(Op, false, true);

  ///< Zero the upper 128-bits of hardcoded YMM0
  AVX128_StoreXMMRegister(0, LoadZeroVector(OpSize::i128Bit), true);
}

void OpDispatchBuilder::AVX128_PHMINPOSUW(OpcodeArgs) {
  Ref Result = PHMINPOSUWOpImpl(Op);
  AVX128_StoreResult_WithOpSize(Op, Op->Dest, AVX128_Zext(Result));
}

template<size_t ElementSize>
void OpDispatchBuilder::AVX128_VectorRound(OpcodeArgs) {
  const auto Size = GetSrcSize(Op);
  const auto Mode = Op->Src[1].Literal();

  AVX128_VectorUnaryImpl(Op, Size, ElementSize,
                         [this, Mode](size_t, Ref Src) { return VectorRoundImpl(OpSize::i128Bit, ElementSize, Src, Mode); });
}

template<size_t ElementSize>
void OpDispatchBuilder::AVX128_InsertScalarRound(OpcodeArgs) {
  // We load the full vector width when dealing with a source vector,
  // so that we don't do any unnecessary zero extension to the scalar
  // element that we're going to operate on.
  const auto SrcSize = GetSrcSize(Op);

  auto Src1 = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, false);
  RefPair Src2 {};
  if (Op->Src[1].IsGPR()) {
    Src2 = AVX128_LoadSource_WithOpSize(Op, Op->Src[1], Op->Flags, false);
  } else {
    Src2.Low = LoadSource_WithOpSize(FPRClass, Op, Op->Src[1], SrcSize, Op->Flags);
  }

  // If OpSize == ElementSize then it only does the lower scalar op
  const auto SourceMode = TranslateRoundType(Op->Src[2].Literal());

  Ref Result = _VFToIScalarInsert(OpSize::i128Bit, ElementSize, Src1.Low, Src2.Low, SourceMode, false);
  AVX128_StoreResult_WithOpSize(Op, Op->Dest, AVX128_Zext(Result));
}

template<size_t ElementSize>
void OpDispatchBuilder::AVX128_VDPP(OpcodeArgs) {
  const uint64_t Literal = Op->Src[2].Literal();

  AVX128_VectorBinaryImpl(Op, GetSrcSize(Op), ElementSize, [this, Literal](size_t, Ref Src1, Ref Src2) {
    return DPPOpImpl(OpSize::i128Bit, Src1, Src2, Literal, ElementSize);
  });
}

void OpDispatchBuilder::AVX128_VPERMQ(OpcodeArgs) {
  ///< Only ever 256-bit.
  auto Src = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, true);
  const auto Selector = Op->Src[1].Literal();

  RefPair Result {};

  // Crack the operation in to two halves and implement per half
  uint8_t SelectorLow = Selector & 0b1111;
  uint8_t SelectorHigh = (Selector >> 4) & 0b1111;
  auto SelectLane = [this](uint8_t Selector, RefPair Src) -> Ref {
    LOGMAN_THROW_AA_FMT(Selector < 16, "Selector too large!");

    switch (Selector) {
    case 0b00'00: return _VDupElement(OpSize::i128Bit, OpSize::i64Bit, Src.Low, 0);
    case 0b00'01: return _VExtr(OpSize::i128Bit, OpSize::i8Bit, Src.Low, Src.Low, 8);
    case 0b00'10: return _VZip(OpSize::i128Bit, OpSize::i64Bit, Src.High, Src.Low);
    case 0b00'11: return _VExtr(OpSize::i128Bit, OpSize::i8Bit, Src.Low, Src.High, 8);
    case 0b01'00: return Src.Low;
    case 0b01'01: return _VDupElement(OpSize::i128Bit, OpSize::i64Bit, Src.Low, 1);
    case 0b01'10: return _VInsElement(OpSize::i128Bit, OpSize::i64Bit, 1, 1, Src.High, Src.Low);
    case 0b01'11: return _VTrn2(OpSize::i128Bit, OpSize::i64Bit, Src.High, Src.Low);
    case 0b10'00: return _VZip(OpSize::i128Bit, OpSize::i64Bit, Src.Low, Src.High);
    case 0b10'01: return _VExtr(OpSize::i128Bit, OpSize::i8Bit, Src.High, Src.Low, 8);
    case 0b10'10: return _VDupElement(OpSize::i128Bit, OpSize::i64Bit, Src.High, 0);
    case 0b10'11: return _VExtr(OpSize::i128Bit, OpSize::i8Bit, Src.High, Src.High, 8);
    case 0b11'00: return _VInsElement(OpSize::i128Bit, OpSize::i64Bit, 1, 1, Src.Low, Src.High);
    case 0b11'01: return _VTrn2(OpSize::i128Bit, OpSize::i64Bit, Src.Low, Src.High);
    case 0b11'10: return Src.High;
    case 0b11'11: return _VDupElement(OpSize::i128Bit, OpSize::i64Bit, Src.High, 1);
    default: FEX_UNREACHABLE;
    }
  };

  Result.Low = SelectLane(SelectorLow, Src);
  Result.High = SelectorLow == SelectorHigh ? Result.Low : SelectLane(SelectorHigh, Src);

  AVX128_StoreResult_WithOpSize(Op, Op->Dest, Result);
}

template<size_t ElementSize, bool Low>
void OpDispatchBuilder::AVX128_VPSHUF(OpcodeArgs) {
  auto Shuffle = Op->Src[1].Literal();

  AVX128_VectorUnaryImpl(Op, GetSrcSize(Op), ElementSize, [this, Shuffle](size_t _, Ref Src) {
    Ref Result = Src;
    const size_t BaseElement = Low ? 0 : 4;

    for (size_t i = 0; i < 4; i++) {
      const auto Index = (Shuffle >> (2 * i)) & 0b11;
      Result = _VInsElement(OpSize::i128Bit, ElementSize, BaseElement + i, BaseElement + Index, Result, Src);
    }

    return Result;
  });
}

template<size_t ElementSize>
void OpDispatchBuilder::AVX128_VSHUF(OpcodeArgs) {
  const auto SrcSize = GetSrcSize(Op);
  const auto Is128Bit = SrcSize == Core::CPUState::XMM_SSE_REG_SIZE;
  auto Shuffle = Op->Src[2].Literal();

  auto Src1 = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, !Is128Bit);
  auto Src2 = AVX128_LoadSource_WithOpSize(Op, Op->Src[1], Op->Flags, !Is128Bit);

  RefPair Result {};
  Result.Low = SHUFOpImpl(Op, OpSize::i128Bit, ElementSize, Src1.Low, Src2.Low, Shuffle);

  if (Is128Bit) {
    Result.High = LoadZeroVector(OpSize::i128Bit);
  } else {
    constexpr uint8_t ShiftAmount = ElementSize == OpSize::i32Bit ? 0 : 2;
    Result.High = SHUFOpImpl(Op, OpSize::i128Bit, ElementSize, Src1.High, Src2.High, Shuffle >> ShiftAmount);
  }
  AVX128_StoreResult_WithOpSize(Op, Op->Dest, Result);
}

template<size_t ElementSize>
void OpDispatchBuilder::AVX128_VPERMILImm(OpcodeArgs) {
  const auto SrcSize = GetSrcSize(Op);
  const auto Is128Bit = SrcSize == Core::CPUState::XMM_SSE_REG_SIZE;

  const auto Selector = Op->Src[1].Literal() & 0xFF;
  auto Src = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, !Is128Bit);

  RefPair Result = AVX128_Zext(LoadZeroVector(OpSize::i128Bit));

  if constexpr (ElementSize == OpSize::i64Bit) {
    auto DoSwizzle64 = [this](Ref Src, uint8_t Selector) -> Ref {
      switch (Selector) {
      case 0b00:
      case 0b11: return _VDupElement(OpSize::i128Bit, OpSize::i64Bit, Src, Selector & 1);
      case 0b01: return _VExtr(OpSize::i128Bit, OpSize::i8Bit, Src, Src, 8);
      case 0b10:
        // No swizzle
        return Src;
      default: FEX_UNREACHABLE;
      }
    };
    Result.Low = DoSwizzle64(Src.Low, Selector & 0b11);

    if (!Is128Bit) {
      Result.High = DoSwizzle64(Src.High, (Selector >> 2) & 0b11);
    }
  } else {
    Result.Low = Single128Bit4ByteVectorShuffle(Src.Low, Selector);

    if (!Is128Bit) {
      Result.High = Single128Bit4ByteVectorShuffle(Src.High, Selector);
    }
  }

  AVX128_StoreResult_WithOpSize(Op, Op->Dest, Result);
}

template<IROps IROp, size_t ElementSize>
void OpDispatchBuilder::AVX128_VHADDP(OpcodeArgs) {
  AVX128_VectorBinaryImpl(Op, GetSrcSize(Op), ElementSize, [this](size_t, Ref Src1, Ref Src2) {
    DeriveOp(Res, IROp, _VFAddP(OpSize::i128Bit, ElementSize, Src1, Src2));
    return Res;
  });
}

void OpDispatchBuilder::AVX128_VPHADDSW(OpcodeArgs) {
  AVX128_VectorBinaryImpl(Op, GetDstSize(Op), OpSize::i16Bit,
                          [this](size_t _ElementSize, Ref Src1, Ref Src2) { return PHADDSOpImpl(OpSize::i128Bit, Src1, Src2); });
}

void OpDispatchBuilder::AVX128_VPMADDUBSW(OpcodeArgs) {
  AVX128_VectorBinaryImpl(Op, GetSrcSize(Op), OpSize::i128Bit,
                          [this](size_t _ElementSize, Ref Src1, Ref Src2) { return PMADDUBSWOpImpl(OpSize::i128Bit, Src1, Src2); });
}

void OpDispatchBuilder::AVX128_VPMADDWD(OpcodeArgs) {
  AVX128_VectorBinaryImpl(Op, GetSrcSize(Op), OpSize::i128Bit,
                          [this](size_t _ElementSize, Ref Src1, Ref Src2) { return PMADDWDOpImpl(OpSize::i128Bit, Src1, Src2); });
}

template<size_t ElementSize>
void OpDispatchBuilder::AVX128_VBLEND(OpcodeArgs) {
  const auto SrcSize = GetSrcSize(Op);
  const auto Is128Bit = SrcSize == Core::CPUState::XMM_SSE_REG_SIZE;
  const uint64_t Selector = Op->Src[2].Literal();

  ///< High Selector shift depends on element size:
  /// i16Bit: Reuses same bits, no shift
  /// i32Bit: Shift by 4
  /// i64Bit: Shift by 2
  constexpr uint64_t SelectorShift = ElementSize == OpSize::i64Bit ? 2 : ElementSize == OpSize::i32Bit ? 4 : 0;

  auto Src1 = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, !Is128Bit);
  auto Src2 = AVX128_LoadSource_WithOpSize(Op, Op->Src[1], Op->Flags, !Is128Bit);

  RefPair Result {};
  Result.Low = VectorBlend(OpSize::i128Bit, ElementSize, Src1.Low, Src2.Low, Selector);

  if (Is128Bit) {
    Result = AVX128_Zext(Result.Low);
  } else {
    Result.High = VectorBlend(OpSize::i128Bit, ElementSize, Src1.High, Src2.High, (Selector >> SelectorShift));
  }

  AVX128_StoreResult_WithOpSize(Op, Op->Dest, Result);
}

template<size_t ElementSize>
void OpDispatchBuilder::AVX128_VHSUBP(OpcodeArgs) {
  AVX128_VectorBinaryImpl(Op, GetDstSize(Op), ElementSize,
                          [this](size_t, Ref Src1, Ref Src2) { return HSUBPOpImpl(OpSize::i128Bit, ElementSize, Src1, Src2); });
}

void OpDispatchBuilder::AVX128_VPSHUFB(OpcodeArgs) {
  auto MaskVector = GeneratePSHUFBMask(OpSize::i128Bit);
  AVX128_VectorBinaryImpl(Op, GetDstSize(Op), OpSize::i8Bit,
                          [this, MaskVector](size_t, Ref Src1, Ref Src2) { return PSHUFBOpImpl(OpSize::i128Bit, Src1, Src2, MaskVector); });
}

void OpDispatchBuilder::AVX128_VPSADBW(OpcodeArgs) {
  AVX128_VectorBinaryImpl(Op, GetDstSize(Op), OpSize::i8Bit,
                          [this](size_t, Ref Src1, Ref Src2) { return PSADBWOpImpl(OpSize::i128Bit, Src1, Src2); });
}

void OpDispatchBuilder::AVX128_VMPSADBW(OpcodeArgs) {
  const auto SrcSize = GetSrcSize(Op);
  const auto Is128Bit = SrcSize == Core::CPUState::XMM_SSE_REG_SIZE;
  const uint64_t Selector = Op->Src[2].Literal();

  auto Src1 = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, !Is128Bit);
  auto Src2 = AVX128_LoadSource_WithOpSize(Op, Op->Src[1], Op->Flags, !Is128Bit);

  RefPair Result {};
  auto ZeroRegister = LoadZeroVector(OpSize::i128Bit);

  Result.Low = MPSADBWOpImpl(OpSize::i128Bit, Src1.Low, Src2.Low, Selector);

  if (Is128Bit) {
    Result.High = ZeroRegister;
  } else {
    Result.High = MPSADBWOpImpl(OpSize::i128Bit, Src1.High, Src2.High, Selector >> 3);
  }

  AVX128_StoreResult_WithOpSize(Op, Op->Dest, Result);
}

void OpDispatchBuilder::AVX128_VPALIGNR(OpcodeArgs) {
  const auto Index = Op->Src[2].Literal();
  const auto Size = GetDstSize(Op);
  const auto SanitizedDstSize = std::min(Size, uint8_t {16});

  AVX128_VectorBinaryImpl(Op, Size, SanitizedDstSize, [this, Index](size_t SanitizedDstSize, Ref Src1, Ref Src2) -> Ref {
    if (Index >= (SanitizedDstSize * 2)) {
      // If the immediate is greater than both vectors combined then it zeroes the vector
      return LoadZeroVector(OpSize::i128Bit);
    }

    if (Index == 0) {
      return Src2;
    }

    return _VExtr(OpSize::i128Bit, OpSize::i8Bit, Src1, Src2, Index);
  });
}

void OpDispatchBuilder::AVX128_VMASKMOVImpl(OpcodeArgs, size_t ElementSize, size_t DstSize, bool IsStore,
                                            const X86Tables::DecodedOperand& MaskOp, const X86Tables::DecodedOperand& DataOp) {
  const auto Is128Bit = DstSize == Core::CPUState::XMM_SSE_REG_SIZE;

  auto Mask = AVX128_LoadSource_WithOpSize(Op, MaskOp, Op->Flags, !Is128Bit);

  const auto MakeAddress = [this, Op](const X86Tables::DecodedOperand& Data) {
    return MakeSegmentAddress(Op, Data, CTX->GetGPRSize());
  };

  if (IsStore) {
    auto Address = MakeAddress(Op->Dest);

    auto Data = AVX128_LoadSource_WithOpSize(Op, DataOp, Op->Flags, !Is128Bit);
    _VStoreVectorMasked(OpSize::i128Bit, ElementSize, Mask.Low, Data.Low, Address, Invalid(), MEM_OFFSET_SXTX, 1);
    if (!Is128Bit) {
      ///< TODO: This can be cleaner if AVX128_LoadSource_WithOpSize could return both constructed addresses.
      auto AddressHigh = _Add(OpSize::i64Bit, Address, _Constant(16));
      _VStoreVectorMasked(OpSize::i128Bit, ElementSize, Mask.High, Data.High, AddressHigh, Invalid(), MEM_OFFSET_SXTX, 1);
    }
  } else {
    auto Address = MakeAddress(DataOp);

    RefPair Result {};
    Result.Low = _VLoadVectorMasked(OpSize::i128Bit, ElementSize, Mask.Low, Address, Invalid(), MEM_OFFSET_SXTX, 1);

    if (Is128Bit) {
      Result.High = LoadZeroVector(OpSize::i128Bit);
    } else {
      ///< TODO: This can be cleaner if AVX128_LoadSource_WithOpSize could return both constructed addresses.
      auto AddressHigh = _Add(OpSize::i64Bit, Address, _Constant(16));
      Result.High = _VLoadVectorMasked(OpSize::i128Bit, ElementSize, Mask.High, AddressHigh, Invalid(), MEM_OFFSET_SXTX, 1);
    }
    AVX128_StoreResult_WithOpSize(Op, Op->Dest, Result);
  }
}

template<bool IsStore>
void OpDispatchBuilder::AVX128_VPMASKMOV(OpcodeArgs) {
  AVX128_VMASKMOVImpl(Op, GetSrcSize(Op), GetDstSize(Op), IsStore, Op->Src[0], Op->Src[1]);
}

template<size_t ElementSize, bool IsStore>
void OpDispatchBuilder::AVX128_VMASKMOV(OpcodeArgs) {
  AVX128_VMASKMOVImpl(Op, ElementSize, GetDstSize(Op), IsStore, Op->Src[0], Op->Src[1]);
}

void OpDispatchBuilder::AVX128_MASKMOV(OpcodeArgs) {
  ///< This instruction only supports 128-bit.
  const auto Size = GetSrcSize(Op);
  const auto Is128Bit = Size == Core::CPUState::XMM_SSE_REG_SIZE;

  auto MaskSrc = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, !Is128Bit);

  // Mask only cares about the top bit of each byte
  MaskSrc.Low = _VCMPLTZ(Size, 1, MaskSrc.Low);

  // Vector that will overwrite byte elements.
  auto VectorSrc = AVX128_LoadSource_WithOpSize(Op, Op->Dest, Op->Flags, !Is128Bit);

  // RDI source (DS prefix by default)
  auto MemDest = MakeSegmentAddress(X86State::REG_RDI, Op->Flags, X86Tables::DecodeFlags::FLAG_DS_PREFIX);

  Ref XMMReg = _LoadMem(FPRClass, Size, MemDest, 1);

  // If the Mask element high bit is set then overwrite the element with the source, else keep the memory variant
  XMMReg = _VBSL(Size, MaskSrc.Low, VectorSrc.Low, XMMReg);
  _StoreMem(FPRClass, Size, MemDest, XMMReg, 1);
}

template<size_t ElementSize>
void OpDispatchBuilder::AVX128_VectorVariableBlend(OpcodeArgs) {
  const auto Size = GetSrcSize(Op);
  const auto Is128Bit = Size == Core::CPUState::XMM_SSE_REG_SIZE;
  const auto Src3Selector = Op->Src[2].Literal();

  constexpr auto ElementSizeBits = ElementSize * 8;

  auto Src1 = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, !Is128Bit);
  auto Src2 = AVX128_LoadSource_WithOpSize(Op, Op->Src[1], Op->Flags, !Is128Bit);

  uint8_t MaskRegister = (Src3Selector >> 4) & 0b1111;
  RefPair Mask {.Low = AVX128_LoadXMMRegister(MaskRegister, false)};

  if (!Is128Bit) {
    Mask.High = AVX128_LoadXMMRegister(MaskRegister, true);
  }

  auto Convert = [this](Ref Src1, Ref Src2, Ref Mask) {
    Ref Shifted = _VSShrI(OpSize::i128Bit, ElementSize, Mask, ElementSizeBits - 1);
    return _VBSL(OpSize::i128Bit, Shifted, Src2, Src1);
  };

  RefPair Result {};
  Result.Low = Convert(Src1.Low, Src2.Low, Mask.Low);
  if (!Is128Bit) {
    Result.High = Convert(Src1.High, Src2.High, Mask.High);
  }

  AVX128_StoreResult_WithOpSize(Op, Op->Dest, Result);
}

void OpDispatchBuilder::AVX128_SaveAVXState(Ref MemBase) {
  const auto NumRegs = CTX->Config.Is64BitMode ? 16U : 8U;

  for (uint32_t i = 0; i < NumRegs; ++i) {
    Ref Upper = AVX128_LoadXMMRegister(i, true);
    _StoreMem(FPRClass, 16, Upper, MemBase, _Constant(i * 16 + 576), 16, MEM_OFFSET_SXTX, 1);
  }
}

void OpDispatchBuilder::AVX128_RestoreAVXState(Ref MemBase) {
  const auto NumRegs = CTX->Config.Is64BitMode ? 16U : 8U;

  for (uint32_t i = 0; i < NumRegs; ++i) {
    Ref YMMHReg = _LoadMem(FPRClass, 16, MemBase, _Constant(i * 16 + 576), 16, MEM_OFFSET_SXTX, 1);
    AVX128_StoreXMMRegister(i, YMMHReg, true);
  }
}

void OpDispatchBuilder::AVX128_DefaultAVXState() {
  const auto NumRegs = CTX->Config.Is64BitMode ? 16U : 8U;

  auto ZeroRegister = LoadZeroVector(OpSize::i128Bit);
  for (uint32_t i = 0; i < NumRegs; i++) {
    AVX128_StoreXMMRegister(i, ZeroRegister, true);
  }
}

void OpDispatchBuilder::AVX128_VPERM2(OpcodeArgs) {
  auto Src1 = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, true);
  auto Src2 = AVX128_LoadSource_WithOpSize(Op, Op->Src[1], Op->Flags, true);
  const auto Selector = Op->Src[2].Literal();

  RefPair Result = AVX128_Zext(LoadZeroVector(OpSize::i128Bit));
  Ref Elements[4] = {Src1.Low, Src1.High, Src2.Low, Src2.High};

  if ((Selector & 0b00001000) == 0) {
    Result.Low = Elements[Selector & 0b11];
  }

  if ((Selector & 0b10000000) == 0) {
    Result.High = Elements[(Selector >> 4) & 0b11];
  }

  AVX128_StoreResult_WithOpSize(Op, Op->Dest, Result);
}

template<size_t ElementSize>
void OpDispatchBuilder::AVX128_VTESTP(OpcodeArgs) {
  InvalidateDeferredFlags();

  const auto Size = GetSrcSize(Op);
  const auto Is128Bit = Size == Core::CPUState::XMM_SSE_REG_SIZE;

  auto Src1 = AVX128_LoadSource_WithOpSize(Op, Op->Dest, Op->Flags, !Is128Bit);
  auto Src2 = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, !Is128Bit);

  // For 128-bit, we use the common path.
  if (Is128Bit) {
    VTESTOpImpl(OpSize::i128Bit, ElementSize, Src1.Low, Src2.Low);
    return;
  }

  // For 256-bit, we need to split up the operation. This is nontrivial.
  // Let's go the simple route here.
  Ref ZF, CFInv;
  Ref ZeroConst = _Constant(0);
  Ref OneConst = _Constant(1);

  const auto ElementSizeInBits = ElementSize * 8;

  {
    // Calculate ZF first.
    auto AndLow = _VAnd(OpSize::i128Bit, OpSize::i8Bit, Src2.Low, Src1.Low);
    auto AndHigh = _VAnd(OpSize::i128Bit, OpSize::i8Bit, Src2.High, Src1.High);

    auto ShiftLow = _VUShrI(OpSize::i128Bit, ElementSize, AndLow, ElementSizeInBits - 1);
    auto ShiftHigh = _VUShrI(OpSize::i128Bit, ElementSize, AndHigh, ElementSizeInBits - 1);
    // Only have the signs now, add it all
    auto AddResult = _VAdd(OpSize::i128Bit, ElementSize, ShiftHigh, ShiftLow);
    Ref AddWide {};
    if (ElementSize == OpSize::i32Bit) {
      AddWide = _VAddV(OpSize::i128Bit, ElementSize, AddResult);
    } else {
      AddWide = _VAddP(OpSize::i128Bit, ElementSize, AddResult, AddResult);
    }

    // ExtGPR will either be [0, 8] or [0, 16] If 0 then set Flag.
    ZF = _VExtractToGPR(OpSize::i128Bit, ElementSize, AddWide, 0);
  }

  {
    // Calculate CF Second
    auto AndLow = _VAndn(OpSize::i128Bit, OpSize::i8Bit, Src2.Low, Src1.Low);
    auto AndHigh = _VAndn(OpSize::i128Bit, OpSize::i8Bit, Src2.High, Src1.High);

    auto ShiftLow = _VUShrI(OpSize::i128Bit, ElementSize, AndLow, ElementSizeInBits - 1);
    auto ShiftHigh = _VUShrI(OpSize::i128Bit, ElementSize, AndHigh, ElementSizeInBits - 1);
    // Only have the signs now, add it all
    auto AddResult = _VAdd(OpSize::i128Bit, ElementSize, ShiftHigh, ShiftLow);
    Ref AddWide {};
    if (ElementSize == OpSize::i32Bit) {
      AddWide = _VAddV(OpSize::i128Bit, ElementSize, AddResult);
    } else {
      AddWide = _VAddP(OpSize::i128Bit, ElementSize, AddResult, AddResult);
    }

    // ExtGPR will either be [0, 8] or [0, 16] If 0 then set Flag.
    auto ExtGPR = _VExtractToGPR(OpSize::i128Bit, ElementSize, AddWide, 0);
    CFInv = _Select(IR::COND_NEQ, ExtGPR, ZeroConst, OneConst, ZeroConst);
  }

  // As in PTest, this sets Z appropriately while zeroing the rest of NZCV.
  SetNZ_ZeroCV(32, ZF);
  SetCFInverted(CFInv);
  ZeroPF_AF();
}

void OpDispatchBuilder::AVX128_PTest(OpcodeArgs) {
  // Invalidate deferred flags early
  InvalidateDeferredFlags();

  const auto Size = GetSrcSize(Op);
  const auto Is128Bit = Size == Core::CPUState::XMM_SSE_REG_SIZE;

  auto Src1 = AVX128_LoadSource_WithOpSize(Op, Op->Dest, Op->Flags, !Is128Bit);
  auto Src2 = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, !Is128Bit);

  // For 128-bit, use the common path.
  if (Is128Bit) {
    PTestOpImpl(OpSize::i128Bit, Src1.Low, Src2.Low);
    return;
  }

  // For 256-bit, we need to unroll. This is nontrivial.
  Ref Test1Low = _VAnd(OpSize::i128Bit, OpSize::i8Bit, Src1.Low, Src2.Low);
  Ref Test2Low = _VAndn(OpSize::i128Bit, OpSize::i8Bit, Src2.Low, Src1.Low);

  Ref Test1High = _VAnd(OpSize::i128Bit, OpSize::i8Bit, Src1.High, Src2.High);
  Ref Test2High = _VAndn(OpSize::i128Bit, OpSize::i8Bit, Src2.High, Src1.High);

  // Element size must be less than 32-bit for the sign bit tricks.
  Ref Test1Max = _VUMax(OpSize::i128Bit, OpSize::i16Bit, Test1Low, Test1High);
  Ref Test2Max = _VUMax(OpSize::i128Bit, OpSize::i16Bit, Test2Low, Test2High);

  Ref Test1 = _VUMaxV(OpSize::i128Bit, OpSize::i16Bit, Test1Max);
  Ref Test2 = _VUMaxV(OpSize::i128Bit, OpSize::i16Bit, Test2Max);

  Test1 = _VExtractToGPR(OpSize::i128Bit, OpSize::i16Bit, Test1, 0);
  Test2 = _VExtractToGPR(OpSize::i128Bit, OpSize::i16Bit, Test2, 0);

  auto ZeroConst = _Constant(0);
  auto OneConst = _Constant(1);

  Test2 = _Select(FEXCore::IR::COND_NEQ, Test2, ZeroConst, OneConst, ZeroConst);

  // Careful, these flags are different between {V,}PTEST and VTESTP{S,D}
  // Set ZF according to Test1. SF will be zeroed since we do a 32-bit test on
  // the results of a 16-bit value from the UMaxV, so the 32-bit sign bit is
  // cleared even if the 16-bit scalars were negative.
  SetNZ_ZeroCV(32, Test1);
  SetCFInverted(Test2);
  ZeroPF_AF();
}

template<size_t ElementSize>
void OpDispatchBuilder::AVX128_VPERMILReg(OpcodeArgs) {
  AVX128_VectorBinaryImpl(Op, GetSrcSize(Op), ElementSize, [this](size_t _ElementSize, Ref Src, Ref Indices) {
    return VPERMILRegOpImpl(OpSize::i128Bit, ElementSize, Src, Indices);
  });
}

void OpDispatchBuilder::AVX128_VPERMD(OpcodeArgs) {
  // Only 256-bit
  auto Indices = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, true);
  auto Src = AVX128_LoadSource_WithOpSize(Op, Op->Src[1], Op->Flags, true);

  auto DoPerm = [this](RefPair Src, Ref Indices, Ref IndexMask, Ref AddVector) {
    Ref FinalIndices = VPERMDIndices(OpSize::i128Bit, Indices, IndexMask, AddVector);
    return _VTBL2(OpSize::i128Bit, Src.Low, Src.High, FinalIndices);
  };

  RefPair Result {};

  Ref IndexMask = _VectorImm(OpSize::i128Bit, OpSize::i32Bit, 0b111);
  Ref AddConst = _Constant(0x03020100);
  Ref Repeating3210 = _VDupFromGPR(OpSize::i128Bit, OpSize::i32Bit, AddConst);

  Result.Low = DoPerm(Src, Indices.Low, IndexMask, Repeating3210);
  Result.High = DoPerm(Src, Indices.High, IndexMask, Repeating3210);

  AVX128_StoreResult_WithOpSize(Op, Op->Dest, Result);
}

void OpDispatchBuilder::AVX128_VPCLMULQDQ(OpcodeArgs) {
  const auto Selector = static_cast<uint8_t>(Op->Src[2].Literal());

  AVX128_VectorBinaryImpl(Op, GetSrcSize(Op), 0, [this, Selector](size_t _, Ref Src1, Ref Src2) {
    return _PCLMUL(OpSize::i128Bit, Src1, Src2, Selector & 0b1'0001);
  });
}

// FMA differences between AArch64 and x86 make this really confusing to remember how things match.
// Here's a little guide for remembering how these instructions related across the architectures.
//
///< AArch64 Vector FMA behaviour
// FMLA vd, vn, vm
// - vd = (vn * vm) + vd
// FMLS vd, vn, vm
// - vd = (-vn * vm) + vd
//
// SVE ONLY! No FNMLA or FNMLS variants until SVE!
// FMLA zda, pg/m, zn, zm - Ignore predicate here
// - zda = (zn * zm) + zda
// FMLS zda, pg/m, zn, zm - Ignore predicate here
// - zda = (-zn * zm) + zda
// FNMLA zda, pg/m, zn, zm - Ignore predicate here
// - zda = (-zn * zm) - zda
// FNMLS zda, pg/m, zn, zm - Ignore predicate here
// - zda = (zn * zm) - zda
//
///< AArch64 Scalar FMA behaviour (FMA4 versions!)
// All variants support 16-bit, 32-bit, and 64-bit.
// FMADD d, n, m, a
// - d = (n * m) + a
// FMSUB d, n, m, a
// - d = (-n * m) + a
// FNMADD d, n, m, a
// - d = (-n * m) - a
// FNMSUB d, n, m, a
// - d = (n * m) - a
//
///< x86 FMA behaviour
// ## Packed variants
// - VFMADD{PD,PS}suffix src1, src2, src3/mem
// - 132 - src1 = (src1 * src3) + src2
// - 213 - src1 = (src2 * src1) + src3
// - 231 - src1 = (src2 * src3) + src1
//   ^ Matches ARM FMLA
//
// - VFMSUB{PD,PS}suffix src1, src2, src3/mem
// - 132 - src1 = (src1 * src3) - src2
// - 213 - src1 = (src2 * src1) - src3
// - 231 - src1 = (src2 * src3) - src1
//   ^ Matches ARM FMLA with addend negated first
//   ^ Or just SVE FNMLS
//   ^ or scalar FNMSUB
//
// - VFNMADD{PD,PS}suffix src1, src2, src3/mem
// - 132 - src1 = (-src1 * src3) + src2
// - 213 - src1 = (-src2 * src1) + src3
// - 231 - src1 = (-src2 * src3) + src1
//   ^ Matches ARM FMLS behaviour! (REALLY CONFUSINGLY NAMED!)
//   ^ Or Scalar FMSUB
//
// - VFNMSUB{PD,PS}suffix src1, src2, src3/mem
// - 132 - src1 = (-src1 * src3) - src2
// - 213 - src1 = (-src2 * src1) - src3
// - 231 - src1 = (-src2 * src3) - src1
//   ^ Matches ARM FMLS behaviour with addend negated first! (REALLY CONFUSINGLY NAMED!)
//   ^ Or just SVE FNMLA
//   ^ Or scalar FNMADD
//
// - VFNMADDSUB{PD,PS}suffix src1, src2, src3/mem
// - 132 - src1.odd  = (src1.odd  * src3.odd)  + src2.odd
//       - src1.even = (src1.even * src3.even) - src2.even
// - 213 - src1.odd  = (src2.odd  * src1.odd)  + src3.odd
//       - src1.even = (src2.even * src1.even) - src3.even
// - 231 - src1.odd  = (src2.odd  * src3.odd)  + src1.odd
//       - src1.even = (src2.even * src3.even) - src1.even
//   ^ Matches ARM FMLA behaviour with addend.even negated first!
//
// - VFNMSUBADD{PD,PS}suffix src1, src2, src3/mem
// - 132 - src1.odd  = (src1.odd  * src3.odd)  - src2.odd
//       - src1.even = (src1.even * src3.even) + src2.even
// - 213 - src1.odd  = (src2.odd  * src1.odd)  - src3.odd
//       - src1.even = (src2.even * src1.even) + src3.even
// - 231 - src1.odd  = (src2.odd  * src3.odd)  - src1.odd
//       - src1.even = (src2.even * src3.even) + src1.even
//   ^ Matches ARM FMLA behaviour with addend.odd negated first!
//
// As shown only the 231 suffixed instructions matches AArch64 behaviour.
// FEX will insert moves to transpose the vectors to match AArch64 behaviour for 132 and 213 variants.

void OpDispatchBuilder::AVX128_VFMAImpl(OpcodeArgs, IROps IROp, uint8_t Src1Idx, uint8_t Src2Idx, uint8_t AddendIdx) {
  const auto Size = GetDstSize(Op);
  const auto Is128Bit = Size == Core::CPUState::XMM_SSE_REG_SIZE;

  const OpSize ElementSize = Op->Flags & X86Tables::DecodeFlags::FLAG_OPTION_AVX_W ? OpSize::i64Bit : OpSize::i32Bit;


  auto Dest = AVX128_LoadSource_WithOpSize(Op, Op->Dest, Op->Flags, !Is128Bit);
  auto Src1 = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, !Is128Bit);
  auto Src2 = AVX128_LoadSource_WithOpSize(Op, Op->Src[1], Op->Flags, !Is128Bit);

  RefPair Sources[3] = {Dest, Src1, Src2};

  RefPair Result {};
  DeriveOp(Result_Low, IROp, _VFMLA(OpSize::i128Bit, ElementSize, Sources[Src1Idx - 1].Low, Sources[Src2Idx - 1].Low, Sources[AddendIdx - 1].Low));
  Result.Low = Result_Low;
  if (Is128Bit) {
    Result.High = LoadZeroVector(OpSize::i128Bit);
  } else {
    DeriveOp(Result_High, IROp,
             _VFMLA(OpSize::i128Bit, ElementSize, Sources[Src1Idx - 1].High, Sources[Src2Idx - 1].High, Sources[AddendIdx - 1].High));
    Result.High = Result_High;
  }
  AVX128_StoreResult_WithOpSize(Op, Op->Dest, Result);
}

void OpDispatchBuilder::AVX128_VFMAScalarImpl(OpcodeArgs, IROps IROp, uint8_t Src1Idx, uint8_t Src2Idx, uint8_t AddendIdx) {
  const auto Size = GetDstSize(Op);
  const auto Is128Bit = Size == Core::CPUState::XMM_SSE_REG_SIZE;
  LOGMAN_THROW_A_FMT(Is128Bit, "This can't be 256-bit");

  const OpSize ElementSize = Op->Flags & X86Tables::DecodeFlags::FLAG_OPTION_AVX_W ? OpSize::i64Bit : OpSize::i32Bit;

  auto Dest = AVX128_LoadSource_WithOpSize(Op, Op->Dest, Op->Flags, !Is128Bit);
  auto Src1 = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, !Is128Bit);
  auto Src2 = AVX128_LoadSource_WithOpSize(Op, Op->Src[1], Op->Flags, !Is128Bit);

  RefPair Sources[3] = {Dest, Src1, Src2};

  DeriveOp(Result_Low, IROp,
           _VFMLAScalarInsert(OpSize::i128Bit, ElementSize, Sources[Src1Idx - 1].Low, Sources[Src2Idx - 1].Low, Sources[AddendIdx - 1].Low));
  AVX128_StoreResult_WithOpSize(Op, Op->Dest, AVX128_Zext(Result_Low));
}

void OpDispatchBuilder::AVX128_VFMAddSubImpl(OpcodeArgs, bool AddSub, uint8_t Src1Idx, uint8_t Src2Idx, uint8_t AddendIdx) {
  const auto Size = GetDstSize(Op);
  const auto Is128Bit = Size == Core::CPUState::XMM_SSE_REG_SIZE;

  const OpSize ElementSize = Op->Flags & X86Tables::DecodeFlags::FLAG_OPTION_AVX_W ? OpSize::i64Bit : OpSize::i32Bit;

  auto Dest = AVX128_LoadSource_WithOpSize(Op, Op->Dest, Op->Flags, !Is128Bit);
  auto Src1 = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, !Is128Bit);
  auto Src2 = AVX128_LoadSource_WithOpSize(Op, Op->Src[1], Op->Flags, !Is128Bit);

  RefPair Sources[3] = {
    Dest,
    Src1,
    Src2,
  };

  RefPair Result {};

  Ref ConstantEOR {};
  if (AddSub) {
    ConstantEOR = LoadAndCacheNamedVectorConstant(
      OpSize::i128Bit, ElementSize == OpSize::i32Bit ? NAMED_VECTOR_PADDSUBPS_INVERT : NAMED_VECTOR_PADDSUBPD_INVERT);
  } else {
    ConstantEOR = LoadAndCacheNamedVectorConstant(
      OpSize::i128Bit, ElementSize == OpSize::i32Bit ? NAMED_VECTOR_PSUBADDPS_INVERT : NAMED_VECTOR_PSUBADDPD_INVERT);
  }
  auto InvertedSourceLow = _VXor(OpSize::i128Bit, ElementSize, Sources[AddendIdx - 1].Low, ConstantEOR);

  Result.Low = _VFMLA(OpSize::i128Bit, ElementSize, Sources[Src1Idx - 1].Low, Sources[Src2Idx - 1].Low, InvertedSourceLow);
  if (Is128Bit) {
    Result.High = LoadZeroVector(OpSize::i128Bit);
  } else {
    auto InvertedSourceHigh = _VXor(OpSize::i128Bit, ElementSize, Sources[AddendIdx - 1].High, ConstantEOR);
    Result.High = _VFMLA(OpSize::i128Bit, ElementSize, Sources[Src1Idx - 1].High, Sources[Src2Idx - 1].High, InvertedSourceHigh);
  }
  AVX128_StoreResult_WithOpSize(Op, Op->Dest, Result);
}

OpDispatchBuilder::RefPair OpDispatchBuilder::AVX128_VPGatherImpl(OpSize Size, OpSize ElementLoadSize, OpSize AddrElementSize, RefPair Dest,
                                                                  RefPair Mask, RefVSIB VSIB) {
  LOGMAN_THROW_A_FMT(AddrElementSize == OpSize::i32Bit || AddrElementSize == OpSize::i64Bit, "Unknown address element size");
  const auto Is128Bit = Size == Core::CPUState::XMM_SSE_REG_SIZE;

  ///< BaseAddr doesn't need to exist, calculate that here.
  Ref BaseAddr = VSIB.BaseAddr;
  if (BaseAddr && VSIB.Displacement) {
    BaseAddr = _Add(OpSize::i64Bit, BaseAddr, _Constant(VSIB.Displacement));
  } else if (VSIB.Displacement) {
    BaseAddr = _Constant(VSIB.Displacement);
  } else if (!BaseAddr) {
    BaseAddr = Invalid();
  }

  if (CTX->HostFeatures.SupportsSVE128) {
    if (ElementLoadSize == OpSize::i64Bit && AddrElementSize == OpSize::i32Bit) {
      // In the case that FEX is loading double the amount of data than the number of address bits then we can optimize this case.
      // For 256-bits of data we need to sign extend all four 32-bit address elements to be 64-bit.
      // For 128-bits of data we only need to sign extend the lower two 32-bit address elements.
      LOGMAN_THROW_A_FMT(VSIB.High == Invalid(), "Need to not have a high VSIB source");

      if (!Is128Bit) {
        VSIB.High = _VSSHLL2(OpSize::i128Bit, OpSize::i32Bit, VSIB.Low, FEXCore::ilog2(VSIB.Scale));
      }
      VSIB.Low = _VSSHLL(OpSize::i128Bit, OpSize::i32Bit, VSIB.Low, FEXCore::ilog2(VSIB.Scale));

      ///< Set the scale to one now that it has been prescaled as well.
      VSIB.Scale = 1;

      // Set the address element size to 64-bit now that the elements are extended.
      AddrElementSize = OpSize::i64Bit;
    } else if (ElementLoadSize == OpSize::i64Bit && AddrElementSize == OpSize::i64Bit && (VSIB.Scale == 2 || VSIB.Scale == 4)) {
      // SVE gather instructions don't support scaling their vector elements by anything other than 1 or the address element size.
      // Pre-scale 64-bit addresses in the case that scale doesn't match in-order to hit SVE code paths more frequently.
      // Only hit this path if the host supports SVE. Otherwise it's a degradation for the ASIMD codepath.
      VSIB.Low = _VShlI(OpSize::i128Bit, OpSize::i64Bit, VSIB.Low, FEXCore::ilog2(VSIB.Scale));
      if (!Is128Bit) {
        VSIB.High = _VShlI(OpSize::i128Bit, OpSize::i64Bit, VSIB.High, FEXCore::ilog2(VSIB.Scale));
      }
      ///< Set the scale to one now that it has been prescaled.
      VSIB.Scale = 1;
    }
  }

  RefPair Result {};
  ///< Calculate the low-half.
  Result.Low = _VLoadVectorGatherMasked(OpSize::i128Bit, ElementLoadSize, Dest.Low, Mask.Low, BaseAddr, VSIB.Low, VSIB.High,
                                        AddrElementSize, VSIB.Scale, 0, 0);

  if (Is128Bit) {
    Result.High = LoadZeroVector(OpSize::i128Bit);
    if (AddrElementSize == OpSize::i64Bit && ElementLoadSize == OpSize::i32Bit) {
      // Special case for the 128-bit gather load using 64-bit address indexes with 32-bit results.
      // Only loads two 32-bit elements in to the lower 64-bits of the first destination.
      // Bits [255:65] all become zero.
      Result.Low = _VZip(OpSize::i128Bit, OpSize::i64Bit, Result.Low, Result.High);
    }
  } else {
    RefPair AddrAddressing {};

    Ref DestReg = Dest.High;
    Ref MaskReg = Mask.High;
    uint8_t IndexElementOffset {};
    uint8_t DataElementOffset {};
    if (AddrElementSize == ElementLoadSize) {
      // If the address size matches the loading element size then it will be fetching at the same rate between low and high
      AddrAddressing.Low = VSIB.High;
      AddrAddressing.High = Invalid();
    } else if (AddrElementSize == OpSize::i32Bit && ElementLoadSize == OpSize::i64Bit) {
      // If the address element size if half the size of the Element load size then we need to start fetching half-way through the low register.
      AddrAddressing.Low = VSIB.Low;
      AddrAddressing.High = VSIB.High;
      IndexElementOffset = OpSize::i128Bit / AddrElementSize / 2;
    } else if (AddrElementSize == OpSize::i64Bit && ElementLoadSize == OpSize::i32Bit) {
      AddrAddressing.Low = VSIB.High;
      AddrAddressing.High = Invalid();
      DestReg = Result.Low; ///< Start mixing with the low register.
      MaskReg = Mask.Low;   ///< Mask starts with the low mask here.
      IndexElementOffset = 0;
      DataElementOffset = OpSize::i128Bit / ElementLoadSize / 2;
    }

    ///< Calculate the high-half.
    auto ResultHigh = _VLoadVectorGatherMasked(OpSize::i128Bit, ElementLoadSize, DestReg, MaskReg, BaseAddr, AddrAddressing.Low,
                                               AddrAddressing.High, AddrElementSize, VSIB.Scale, DataElementOffset, IndexElementOffset);

    if (AddrElementSize == OpSize::i64Bit && ElementLoadSize == OpSize::i32Bit) {
      // If we only fetched 128-bits worth of data then the upper-result is all zero.
      Result = AVX128_Zext(ResultHigh);
    } else {
      Result.High = ResultHigh;
    }
  }

  return Result;
}

OpDispatchBuilder::RefPair OpDispatchBuilder::AVX128_VPGatherQPSImpl(Ref Dest, Ref Mask, RefVSIB VSIB) {

  ///< BaseAddr doesn't need to exist, calculate that here.
  Ref BaseAddr = VSIB.BaseAddr;
  if (BaseAddr && VSIB.Displacement) {
    BaseAddr = _Add(OpSize::i64Bit, BaseAddr, _Constant(VSIB.Displacement));
  } else if (VSIB.Displacement) {
    BaseAddr = _Constant(VSIB.Displacement);
  } else if (!BaseAddr) {
    BaseAddr = Invalid();
  }

  bool NeedsSVEScale = (VSIB.Scale == 2 || VSIB.Scale == 8) || (BaseAddr == Invalid() && VSIB.Scale != 1);

  if (CTX->HostFeatures.SupportsSVE128 && NeedsSVEScale) {
    // SVE gather instructions don't support scaling their vector elements by anything other than 1 or the address element size.
    // Pre-scale 64-bit addresses in the case that scale doesn't match in-order to hit SVE code paths more frequently.
    // Only hit this path if the host supports SVE. Otherwise it's a degradation for the ASIMD codepath.
    VSIB.Low = _VShlI(OpSize::i128Bit, OpSize::i64Bit, VSIB.Low, FEXCore::ilog2(VSIB.Scale));
    if (VSIB.High != Invalid()) {
      VSIB.High = _VShlI(OpSize::i128Bit, OpSize::i64Bit, VSIB.High, FEXCore::ilog2(VSIB.Scale));
    }
    ///< Set the scale to one now that it has been prescaled.
    VSIB.Scale = 1;
  }

  RefPair Result {};

  ///< Calculate the low-half.
  Result.Low = _VLoadVectorGatherMaskedQPS(OpSize::i128Bit, OpSize::i32Bit, Dest, Mask, BaseAddr, VSIB.Low, VSIB.High, VSIB.Scale);
  Result.High = LoadZeroVector(OpSize::i128Bit);
  if (VSIB.High == Invalid()) {
    // Special case for only loading two floats.
    // The upper 64-bits of the lower lane also gets zero.
    Result.Low = _VZip(OpSize::i128Bit, OpSize::i64Bit, Result.Low, Result.High);
  }

  return Result;
}

template<OpSize AddrElementSize>
void OpDispatchBuilder::AVX128_VPGATHER(OpcodeArgs) {

  const auto Size = GetDstSize(Op);
  const auto Is128Bit = Size == Core::CPUState::XMM_SSE_REG_SIZE;

  ///< Element size is determined by W flag.
  const OpSize ElementLoadSize = Op->Flags & X86Tables::DecodeFlags::FLAG_OPTION_AVX_W ? OpSize::i64Bit : OpSize::i32Bit;

  // We only need the high address register if the number of data elements is more than what the low half can consume.
  // But also the number of address elements is clamped by the destination size as well.
  const size_t NumDataElements = Size / ElementLoadSize;
  const size_t NumAddrElementBytes = std::min<size_t>(Size, (NumDataElements * AddrElementSize));
  const bool NeedsHighAddrBytes = NumAddrElementBytes > OpSize::i128Bit;

  auto Dest = AVX128_LoadSource_WithOpSize(Op, Op->Dest, Op->Flags, !Is128Bit);
  auto VSIB = AVX128_LoadVSIB(Op, Op->Src[0], Op->Flags, NeedsHighAddrBytes);
  auto Mask = AVX128_LoadSource_WithOpSize(Op, Op->Src[1], Op->Flags, !Is128Bit);

  bool NeedsSVEScale = (VSIB.Scale == 2 || VSIB.Scale == 8) || (VSIB.BaseAddr == Invalid() && VSIB.Scale != 1);

  const bool NeedsExplicitSVEPath =
    CTX->HostFeatures.SupportsSVE128 && AddrElementSize == OpSize::i32Bit && ElementLoadSize == OpSize::i32Bit && NeedsSVEScale;

  RefPair Result {};
  if (NeedsExplicitSVEPath) {
    // Special case for VGATHERDPS/VPGATHERDD (32-bit addresses loading 32-bit elements) that can't use the SVE codepath.
    // The problem is due to the scale not matching SVE limitations, we need to prescale the addresses to be 64-bit.
    auto ScaleVSIBHalf = [this](Ref VSIB, Ref BaseAddr, int32_t Displacement, uint8_t Scale) -> RefVSIB {
      RefVSIB Result {};
      Result.High = _VSSHLL2(OpSize::i128Bit, OpSize::i32Bit, VSIB, FEXCore::ilog2(Scale));
      Result.Low = _VSSHLL(OpSize::i128Bit, OpSize::i32Bit, VSIB, FEXCore::ilog2(Scale));

      Result.Displacement = Displacement;
      Result.BaseAddr = BaseAddr;

      ///< Set the scale to one now that it has been prescaled as well.
      Result.Scale = 1;
      return Result;
    };

    RefVSIB VSIBLow = ScaleVSIBHalf(VSIB.Low, VSIB.BaseAddr, VSIB.Displacement, VSIB.Scale);
    RefVSIB VSIBHigh {};

    if (NeedsHighAddrBytes) {
      VSIBHigh = ScaleVSIBHalf(VSIB.High, VSIB.BaseAddr, VSIB.Displacement, VSIB.Scale);
    }

    ///< AddressElementSize is now OpSize::i64Bit
    Result = AVX128_VPGatherQPSImpl(Dest.Low, Mask.Low, VSIBLow);
    if (NeedsHighAddrBytes) {
      auto Res = AVX128_VPGatherQPSImpl(Dest.High, Mask.High, VSIBHigh);
      Result.High = Res.Low;
    }
  } else if (AddrElementSize == OpSize::i64Bit && ElementLoadSize == OpSize::i32Bit) {
    Result = AVX128_VPGatherQPSImpl(Dest.Low, Mask.Low, VSIB);
  } else {
    Result = AVX128_VPGatherImpl(SizeToOpSize(Size), ElementLoadSize, AddrElementSize, Dest, Mask, VSIB);
  }
  AVX128_StoreResult_WithOpSize(Op, Op->Dest, Result);

  ///< Assume non-faulting behaviour and clear the mask register.
  RefPair ZeroPair {};
  ZeroPair.Low = LoadZeroVector(OpSize::i128Bit);
  ZeroPair.High = ZeroPair.Low;
  AVX128_StoreResult_WithOpSize(Op, Op->Src[1], ZeroPair);
}

void OpDispatchBuilder::AVX128_VCVTPH2PS(OpcodeArgs) {
  const auto DstSize = GetDstSize(Op);
  const auto SrcSize = DstSize / 2;
  const auto Is128BitSrc = SrcSize == Core::CPUState::XMM_SSE_REG_SIZE;
  const auto Is128BitDst = DstSize == Core::CPUState::XMM_SSE_REG_SIZE;

  RefPair Src {};
  if (Op->Src[0].IsGPR()) {
    Src = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, !Is128BitSrc);
  } else {
    // In the event that a memory operand is used as the source operand,
    // the access width will always be half the size of the destination vector width
    // (i.e. 128-bit vector -> 64-bit mem, 256-bit vector -> 128-bit mem)
    Src.Low = LoadSource_WithOpSize(FPRClass, Op, Op->Src[0], SrcSize, Op->Flags);
  }

  RefPair Result {};
  Result.Low = _Vector_FToF(OpSize::i128Bit, OpSize::i32Bit, Src.Low, OpSize::i16Bit);

  if (Is128BitSrc) {
    Result.High = _VFCVTL2(OpSize::i128Bit, OpSize::i16Bit, Src.Low);
  }

  if (Is128BitDst) {
    Result = AVX128_Zext(Result.Low);
  }

  AVX128_StoreResult_WithOpSize(Op, Op->Dest, Result);
}

void OpDispatchBuilder::AVX128_VCVTPS2PH(OpcodeArgs) {
  const auto SrcSize = GetSrcSize(Op);
  const auto Is128BitSrc = SrcSize == Core::CPUState::XMM_SSE_REG_SIZE;
  const auto StoreSize = Op->Dest.IsGPR() ? OpSize::i128Bit : SrcSize / 2;

  const auto Imm8 = Op->Src[1].Literal();
  const auto UseMXCSR = (Imm8 & 0b100) != 0;

  auto Src = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, !Is128BitSrc);

  RefPair Result {};

  Ref OldFPCR {};
  if (!UseMXCSR) {
    // No ARM float conversion instructions allow passing in
    // a rounding mode as an immediate. All of them depend on
    // the RM field in the FPCR. And so! We have to do some ugly
    // rounding mode shuffling.
    const auto NewRMode = Imm8 & 0b11;
    OldFPCR = _PushRoundingMode(NewRMode);
  }

  Result.Low = _Vector_FToF(OpSize::i128Bit, OpSize::i16Bit, Src.Low, OpSize::i32Bit);
  if (!Is128BitSrc) {
    Result.Low = _VFCVTN2(OpSize::i128Bit, OpSize::i32Bit, Result.Low, Src.High);
  }

  if (!UseMXCSR) {
    _PopRoundingMode(OldFPCR);
  }

  // We need to eliminate upper junk if we're storing into a register with
  // a 256-bit source (VCVTPS2PH's destination for registers is an XMM).
  if (Op->Src[0].IsGPR() && SrcSize == Core::CPUState::XMM_AVX_REG_SIZE) {
    Result = AVX128_Zext(Result.Low);
  }

  if (!Op->Dest.IsGPR()) {
    StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Result.Low, StoreSize, -1);
  } else {
    AVX128_StoreResult_WithOpSize(Op, Op->Dest, Result);
  }
}

} // namespace FEXCore::IR
