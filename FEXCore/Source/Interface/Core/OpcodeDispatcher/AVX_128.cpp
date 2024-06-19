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

    {OPD(1, 0b00, 0x51), 1, &OpDispatchBuilder::AVX128_VectorUnary<IR::OP_VFSQRT, 4>},
    {OPD(1, 0b01, 0x51), 1, &OpDispatchBuilder::AVX128_VectorUnary<IR::OP_VFSQRT, 8>},
    {OPD(1, 0b10, 0x51), 1, &OpDispatchBuilder::AVX128_VectorScalarInsertALU<IR::OP_VFSQRTSCALARINSERT, 4>},
    {OPD(1, 0b11, 0x51), 1, &OpDispatchBuilder::AVX128_VectorScalarInsertALU<IR::OP_VFSQRTSCALARINSERT, 8>},

    {OPD(1, 0b00, 0x52), 1, &OpDispatchBuilder::AVX128_VectorUnary<IR::OP_VFRSQRT, 4>},
    {OPD(1, 0b10, 0x52), 1, &OpDispatchBuilder::AVX128_VectorScalarInsertALU<IR::OP_VFRSQRTSCALARINSERT, 4>},

    {OPD(1, 0b00, 0x53), 1, &OpDispatchBuilder::AVX128_VectorUnary<IR::OP_VFRECP, 4>},
    {OPD(1, 0b10, 0x53), 1, &OpDispatchBuilder::AVX128_VectorScalarInsertALU<IR::OP_VFRECPSCALARINSERT, 4>},

    {OPD(1, 0b00, 0x54), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VAND, 16>},
    {OPD(1, 0b01, 0x54), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VAND, 16>},

    {OPD(1, 0b00, 0x55), 1, &OpDispatchBuilder::AVX128_VANDN},
    {OPD(1, 0b01, 0x55), 1, &OpDispatchBuilder::AVX128_VANDN},

    {OPD(1, 0b00, 0x56), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VOR, 16>},
    {OPD(1, 0b01, 0x56), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VOR, 16>},

    {OPD(1, 0b00, 0x57), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VXOR, 16>},
    {OPD(1, 0b01, 0x57), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VXOR, 16>},

    {OPD(1, 0b00, 0x58), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VFADD, 4>},
    {OPD(1, 0b01, 0x58), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VFADD, 8>},
    {OPD(1, 0b10, 0x58), 1, &OpDispatchBuilder::AVX128_VectorScalarInsertALU<IR::OP_VFADDSCALARINSERT, 4>},
    {OPD(1, 0b11, 0x58), 1, &OpDispatchBuilder::AVX128_VectorScalarInsertALU<IR::OP_VFADDSCALARINSERT, 8>},

    {OPD(1, 0b00, 0x59), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VFMUL, 4>},
    {OPD(1, 0b01, 0x59), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VFMUL, 8>},
    {OPD(1, 0b10, 0x59), 1, &OpDispatchBuilder::AVX128_VectorScalarInsertALU<IR::OP_VFMULSCALARINSERT, 4>},
    {OPD(1, 0b11, 0x59), 1, &OpDispatchBuilder::AVX128_VectorScalarInsertALU<IR::OP_VFMULSCALARINSERT, 8>},

    {OPD(1, 0b00, 0x5A), 1, &OpDispatchBuilder::AVX128_Vector_CVT_Float_To_Float<8, 4>},
    {OPD(1, 0b01, 0x5A), 1, &OpDispatchBuilder::AVX128_Vector_CVT_Float_To_Float<4, 8>},
    {OPD(1, 0b10, 0x5A), 1, &OpDispatchBuilder::AVX128_InsertScalar_CVT_Float_To_Float<8, 4>},
    {OPD(1, 0b11, 0x5A), 1, &OpDispatchBuilder::AVX128_InsertScalar_CVT_Float_To_Float<4, 8>},

    {OPD(1, 0b00, 0x5B), 1, &OpDispatchBuilder::AVX128_Vector_CVT_Int_To_Float<4, false>},
    {OPD(1, 0b01, 0x5B), 1, &OpDispatchBuilder::AVX128_Vector_CVT_Float_To_Int<4, false, true>},
    {OPD(1, 0b10, 0x5B), 1, &OpDispatchBuilder::AVX128_Vector_CVT_Float_To_Int<4, false, false>},

    {OPD(1, 0b00, 0x5C), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VFSUB, 4>},
    {OPD(1, 0b01, 0x5C), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VFSUB, 8>},
    {OPD(1, 0b10, 0x5C), 1, &OpDispatchBuilder::AVX128_VectorScalarInsertALU<IR::OP_VFSUBSCALARINSERT, 4>},
    {OPD(1, 0b11, 0x5C), 1, &OpDispatchBuilder::AVX128_VectorScalarInsertALU<IR::OP_VFSUBSCALARINSERT, 8>},

    {OPD(1, 0b00, 0x5D), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VFMIN, 4>},
    {OPD(1, 0b01, 0x5D), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VFMIN, 8>},
    {OPD(1, 0b10, 0x5D), 1, &OpDispatchBuilder::AVX128_VectorScalarInsertALU<IR::OP_VFMINSCALARINSERT, 4>},
    {OPD(1, 0b11, 0x5D), 1, &OpDispatchBuilder::AVX128_VectorScalarInsertALU<IR::OP_VFMINSCALARINSERT, 8>},

    {OPD(1, 0b00, 0x5E), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VFDIV, 4>},
    {OPD(1, 0b01, 0x5E), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VFDIV, 8>},
    {OPD(1, 0b10, 0x5E), 1, &OpDispatchBuilder::AVX128_VectorScalarInsertALU<IR::OP_VFDIVSCALARINSERT, 4>},
    {OPD(1, 0b11, 0x5E), 1, &OpDispatchBuilder::AVX128_VectorScalarInsertALU<IR::OP_VFDIVSCALARINSERT, 8>},

    {OPD(1, 0b00, 0x5F), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VFMAX, 4>},
    {OPD(1, 0b01, 0x5F), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VFMAX, 8>},
    {OPD(1, 0b10, 0x5F), 1, &OpDispatchBuilder::AVX128_VectorScalarInsertALU<IR::OP_VFMAXSCALARINSERT, 4>},
    {OPD(1, 0b11, 0x5F), 1, &OpDispatchBuilder::AVX128_VectorScalarInsertALU<IR::OP_VFMAXSCALARINSERT, 8>},

    {OPD(1, 0b01, 0x60), 1, &OpDispatchBuilder::AVX128_VPUNPCKL<1>},
    {OPD(1, 0b01, 0x61), 1, &OpDispatchBuilder::AVX128_VPUNPCKL<2>},
    {OPD(1, 0b01, 0x62), 1, &OpDispatchBuilder::AVX128_VPUNPCKL<4>},
    {OPD(1, 0b01, 0x63), 1, &OpDispatchBuilder::AVX128_VPACKSS<2>},
    {OPD(1, 0b01, 0x64), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VCMPGT, 1>},
    {OPD(1, 0b01, 0x65), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VCMPGT, 2>},
    {OPD(1, 0b01, 0x66), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VCMPGT, 4>},
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

    {OPD(1, 0b01, 0x70), 1, &OpDispatchBuilder::AVX128_VPSHUF<4, true>},
    {OPD(1, 0b10, 0x70), 1, &OpDispatchBuilder::AVX128_VPSHUF<2, false>},
    {OPD(1, 0b11, 0x70), 1, &OpDispatchBuilder::AVX128_VPSHUF<2, true>},

    {OPD(1, 0b01, 0x74), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VCMPEQ, 1>},
    {OPD(1, 0b01, 0x75), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VCMPEQ, 2>},
    {OPD(1, 0b01, 0x76), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VCMPEQ, 4>},

    {OPD(1, 0b00, 0x77), 1, &OpDispatchBuilder::AVX128_VZERO},

    {OPD(1, 0b01, 0x7C), 1, &OpDispatchBuilder::AVX128_VHADDP<IR::OP_VFADDP, 8>},
    {OPD(1, 0b11, 0x7C), 1, &OpDispatchBuilder::AVX128_VHADDP<IR::OP_VFADDP, 4>},
    // TODO: {OPD(1, 0b01, 0x7D), 1, &OpDispatchBuilder::VHSUBPOp<8>},
    // TODO: {OPD(1, 0b11, 0x7D), 1, &OpDispatchBuilder::VHSUBPOp<4>},

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

    {OPD(1, 0b01, 0xD1), 1, &OpDispatchBuilder::AVX128_VPSRL<2>},
    {OPD(1, 0b01, 0xD2), 1, &OpDispatchBuilder::AVX128_VPSRL<4>},
    {OPD(1, 0b01, 0xD3), 1, &OpDispatchBuilder::AVX128_VPSRL<8>},
    {OPD(1, 0b01, 0xD4), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VADD, 8>},
    {OPD(1, 0b01, 0xD5), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VMUL, 2>},
    {OPD(1, 0b01, 0xD6), 1, &OpDispatchBuilder::AVX128_MOVQ},
    {OPD(1, 0b01, 0xD7), 1, &OpDispatchBuilder::AVX128_MOVMSKB},

    {OPD(1, 0b01, 0xD8), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VUQSUB, 1>},
    {OPD(1, 0b01, 0xD9), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VUQSUB, 2>},
    {OPD(1, 0b01, 0xDA), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VUMIN, 1>},
    {OPD(1, 0b01, 0xDB), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VAND, 16>},
    {OPD(1, 0b01, 0xDC), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VUQADD, 1>},
    {OPD(1, 0b01, 0xDD), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VUQADD, 2>},
    {OPD(1, 0b01, 0xDE), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VUMAX, 1>},
    {OPD(1, 0b01, 0xDF), 1, &OpDispatchBuilder::AVX128_VANDN},

    {OPD(1, 0b01, 0xE0), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VURAVG, 1>},
    {OPD(1, 0b01, 0xE1), 1, &OpDispatchBuilder::AVX128_VPSRA<2>},
    {OPD(1, 0b01, 0xE2), 1, &OpDispatchBuilder::AVX128_VPSRA<4>},
    {OPD(1, 0b01, 0xE3), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VURAVG, 2>},
    {OPD(1, 0b01, 0xE4), 1, &OpDispatchBuilder::AVX128_VPMULHW<false>},
    {OPD(1, 0b01, 0xE5), 1, &OpDispatchBuilder::AVX128_VPMULHW<true>},

    {OPD(1, 0b01, 0xE6), 1, &OpDispatchBuilder::AVX128_Vector_CVT_Float_To_Int<8, true, false>},
    {OPD(1, 0b10, 0xE6), 1, &OpDispatchBuilder::AVX128_Vector_CVT_Int_To_Float<4, true>},
    {OPD(1, 0b11, 0xE6), 1, &OpDispatchBuilder::AVX128_Vector_CVT_Float_To_Int<8, true, true>},

    {OPD(1, 0b01, 0xE7), 1, &OpDispatchBuilder::AVX128_MOVVectorNT},

    {OPD(1, 0b01, 0xE8), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VSQSUB, 1>},
    {OPD(1, 0b01, 0xE9), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VSQSUB, 2>},
    {OPD(1, 0b01, 0xEA), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VSMIN, 2>},
    {OPD(1, 0b01, 0xEB), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VOR, 16>},
    {OPD(1, 0b01, 0xEC), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VSQADD, 1>},
    {OPD(1, 0b01, 0xED), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VSQADD, 2>},
    {OPD(1, 0b01, 0xEE), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VSMAX, 2>},
    {OPD(1, 0b01, 0xEF), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VXOR, 16>},

    {OPD(1, 0b11, 0xF0), 1, &OpDispatchBuilder::AVX128_MOVVectorUnaligned},
    {OPD(1, 0b01, 0xF1), 1, &OpDispatchBuilder::AVX128_VPSLL<2>},
    {OPD(1, 0b01, 0xF2), 1, &OpDispatchBuilder::AVX128_VPSLL<4>},
    {OPD(1, 0b01, 0xF3), 1, &OpDispatchBuilder::AVX128_VPSLL<8>},
    {OPD(1, 0b01, 0xF4), 1, &OpDispatchBuilder::AVX128_VPMULL<4, false>},
    // TODO: {OPD(1, 0b01, 0xF5), 1, &OpDispatchBuilder::VPMADDWDOp},
    // TODO: {OPD(1, 0b01, 0xF6), 1, &OpDispatchBuilder::VPSADBWOp},
    // TODO: {OPD(1, 0b01, 0xF7), 1, &OpDispatchBuilder::MASKMOVOp},

    {OPD(1, 0b01, 0xF8), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VSUB, 1>},
    {OPD(1, 0b01, 0xF9), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VSUB, 2>},
    {OPD(1, 0b01, 0xFA), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VSUB, 4>},
    {OPD(1, 0b01, 0xFB), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VSUB, 8>},
    {OPD(1, 0b01, 0xFC), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VADD, 1>},
    {OPD(1, 0b01, 0xFD), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VADD, 2>},
    {OPD(1, 0b01, 0xFE), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VADD, 4>},

    // TODO: {OPD(2, 0b01, 0x00), 1, &OpDispatchBuilder::VPSHUFBOp},
    {OPD(2, 0b01, 0x01), 1, &OpDispatchBuilder::AVX128_VHADDP<IR::OP_VADDP, 2>},
    {OPD(2, 0b01, 0x02), 1, &OpDispatchBuilder::AVX128_VHADDP<IR::OP_VADDP, 4>},
    // TODO: {OPD(2, 0b01, 0x03), 1, &OpDispatchBuilder::VPHADDSWOp},
    // TODO: {OPD(2, 0b01, 0x04), 1, &OpDispatchBuilder::VPMADDUBSWOp},

    {OPD(2, 0b01, 0x05), 1, &OpDispatchBuilder::AVX128_VPHSUB<2>},
    {OPD(2, 0b01, 0x06), 1, &OpDispatchBuilder::AVX128_VPHSUB<4>},
    {OPD(2, 0b01, 0x07), 1, &OpDispatchBuilder::AVX128_VPHSUBSW},

    {OPD(2, 0b01, 0x08), 1, &OpDispatchBuilder::AVX128_VPSIGN<1>},
    {OPD(2, 0b01, 0x09), 1, &OpDispatchBuilder::AVX128_VPSIGN<2>},
    {OPD(2, 0b01, 0x0A), 1, &OpDispatchBuilder::AVX128_VPSIGN<4>},
    {OPD(2, 0b01, 0x0B), 1, &OpDispatchBuilder::AVX128_VPMULHRSW},
    // TODO: {OPD(2, 0b01, 0x0C), 1, &OpDispatchBuilder::VPERMILRegOp<4>},
    // TODO: {OPD(2, 0b01, 0x0D), 1, &OpDispatchBuilder::VPERMILRegOp<8>},
    // TODO: {OPD(2, 0b01, 0x0E), 1, &OpDispatchBuilder::VTESTPOp<4>},
    // TODO: {OPD(2, 0b01, 0x0F), 1, &OpDispatchBuilder::VTESTPOp<8>},

    // TODO: {OPD(2, 0b01, 0x16), 1, &OpDispatchBuilder::VPERMDOp},
    // TODO: {OPD(2, 0b01, 0x17), 1, &OpDispatchBuilder::PTestOp},
    {OPD(2, 0b01, 0x18), 1, &OpDispatchBuilder::AVX128_VBROADCAST<4>},
    {OPD(2, 0b01, 0x19), 1, &OpDispatchBuilder::AVX128_VBROADCAST<8>},
    {OPD(2, 0b01, 0x1A), 1, &OpDispatchBuilder::AVX128_VBROADCAST<16>},
    {OPD(2, 0b01, 0x1C), 1, &OpDispatchBuilder::AVX128_VectorUnary<IR::OP_VABS, 1>},
    {OPD(2, 0b01, 0x1D), 1, &OpDispatchBuilder::AVX128_VectorUnary<IR::OP_VABS, 2>},
    {OPD(2, 0b01, 0x1E), 1, &OpDispatchBuilder::AVX128_VectorUnary<IR::OP_VABS, 4>},

    {OPD(2, 0b01, 0x20), 1, &OpDispatchBuilder::AVX128_ExtendVectorElements<1, 2, true>},
    {OPD(2, 0b01, 0x21), 1, &OpDispatchBuilder::AVX128_ExtendVectorElements<1, 4, true>},
    {OPD(2, 0b01, 0x22), 1, &OpDispatchBuilder::AVX128_ExtendVectorElements<1, 8, true>},
    {OPD(2, 0b01, 0x23), 1, &OpDispatchBuilder::AVX128_ExtendVectorElements<2, 4, true>},
    {OPD(2, 0b01, 0x24), 1, &OpDispatchBuilder::AVX128_ExtendVectorElements<2, 8, true>},
    {OPD(2, 0b01, 0x25), 1, &OpDispatchBuilder::AVX128_ExtendVectorElements<4, 8, true>},

    {OPD(2, 0b01, 0x28), 1, &OpDispatchBuilder::AVX128_VPMULL<4, true>},
    {OPD(2, 0b01, 0x29), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VCMPEQ, 8>},
    {OPD(2, 0b01, 0x2A), 1, &OpDispatchBuilder::AVX128_MOVVectorNT},
    {OPD(2, 0b01, 0x2B), 1, &OpDispatchBuilder::AVX128_VPACKUS<4>},
    // TODO: {OPD(2, 0b01, 0x2C), 1, &OpDispatchBuilder::VMASKMOVOp<4, false>},
    // TODO: {OPD(2, 0b01, 0x2D), 1, &OpDispatchBuilder::VMASKMOVOp<8, false>},
    // TODO: {OPD(2, 0b01, 0x2E), 1, &OpDispatchBuilder::VMASKMOVOp<4, true>},
    // TODO: {OPD(2, 0b01, 0x2F), 1, &OpDispatchBuilder::VMASKMOVOp<8, true>},

    {OPD(2, 0b01, 0x30), 1, &OpDispatchBuilder::AVX128_ExtendVectorElements<1, 2, false>},
    {OPD(2, 0b01, 0x31), 1, &OpDispatchBuilder::AVX128_ExtendVectorElements<1, 4, false>},
    {OPD(2, 0b01, 0x32), 1, &OpDispatchBuilder::AVX128_ExtendVectorElements<1, 8, false>},
    {OPD(2, 0b01, 0x33), 1, &OpDispatchBuilder::AVX128_ExtendVectorElements<2, 4, false>},
    {OPD(2, 0b01, 0x34), 1, &OpDispatchBuilder::AVX128_ExtendVectorElements<2, 8, false>},
    {OPD(2, 0b01, 0x35), 1, &OpDispatchBuilder::AVX128_ExtendVectorElements<4, 8, false>},
    // TODO: {OPD(2, 0b01, 0x36), 1, &OpDispatchBuilder::VPERMDOp},

    {OPD(2, 0b01, 0x37), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VCMPGT, 8>},
    {OPD(2, 0b01, 0x38), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VSMIN, 1>},
    {OPD(2, 0b01, 0x39), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VSMIN, 4>},
    {OPD(2, 0b01, 0x3A), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VUMIN, 2>},
    {OPD(2, 0b01, 0x3B), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VUMIN, 4>},
    {OPD(2, 0b01, 0x3C), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VSMAX, 1>},
    {OPD(2, 0b01, 0x3D), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VSMAX, 4>},
    {OPD(2, 0b01, 0x3E), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VUMAX, 2>},
    {OPD(2, 0b01, 0x3F), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VUMAX, 4>},

    {OPD(2, 0b01, 0x40), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VMUL, 4>},
    {OPD(2, 0b01, 0x41), 1, &OpDispatchBuilder::AVX128_PHMINPOSUW},
    {OPD(2, 0b01, 0x45), 1, &OpDispatchBuilder::AVX128_VPSRLV},
    {OPD(2, 0b01, 0x46), 1, &OpDispatchBuilder::AVX128_VPSRAVD},
    {OPD(2, 0b01, 0x47), 1, &OpDispatchBuilder::AVX128_VPSLLV},

    {OPD(2, 0b01, 0x58), 1, &OpDispatchBuilder::AVX128_VBROADCAST<4>},
    {OPD(2, 0b01, 0x59), 1, &OpDispatchBuilder::AVX128_VBROADCAST<8>},
    {OPD(2, 0b01, 0x5A), 1, &OpDispatchBuilder::AVX128_VBROADCAST<16>},

    {OPD(2, 0b01, 0x78), 1, &OpDispatchBuilder::AVX128_VBROADCAST<1>},
    {OPD(2, 0b01, 0x79), 1, &OpDispatchBuilder::AVX128_VBROADCAST<2>},

    // TODO: {OPD(2, 0b01, 0x8C), 1, &OpDispatchBuilder::VPMASKMOVOp<false>},
    // TODO: {OPD(2, 0b01, 0x8E), 1, &OpDispatchBuilder::VPMASKMOVOp<true>},

    {OPD(2, 0b01, 0xDB), 1, &OpDispatchBuilder::AVX128_VAESImc},
    {OPD(2, 0b01, 0xDC), 1, &OpDispatchBuilder::AVX128_VAESEnc},
    {OPD(2, 0b01, 0xDD), 1, &OpDispatchBuilder::AVX128_VAESEncLast},
    {OPD(2, 0b01, 0xDE), 1, &OpDispatchBuilder::AVX128_VAESDec},
    {OPD(2, 0b01, 0xDF), 1, &OpDispatchBuilder::AVX128_VAESDecLast},

    {OPD(3, 0b01, 0x00), 1, &OpDispatchBuilder::AVX128_VPERMQ},
    {OPD(3, 0b01, 0x01), 1, &OpDispatchBuilder::AVX128_VPERMQ},
    // TODO: {OPD(3, 0b01, 0x02), 1, &OpDispatchBuilder::VPBLENDDOp},
    {OPD(3, 0b01, 0x04), 1, &OpDispatchBuilder::AVX128_VPERMILImm<4>},
    {OPD(3, 0b01, 0x05), 1, &OpDispatchBuilder::AVX128_VPERMILImm<8>},
    // TODO: {OPD(3, 0b01, 0x06), 1, &OpDispatchBuilder::VPERM2Op},
    {OPD(3, 0b01, 0x08), 1, &OpDispatchBuilder::AVX128_VectorRound<4>},
    {OPD(3, 0b01, 0x09), 1, &OpDispatchBuilder::AVX128_VectorRound<8>},
    {OPD(3, 0b01, 0x0A), 1, &OpDispatchBuilder::AVX128_InsertScalarRound<4>},
    {OPD(3, 0b01, 0x0B), 1, &OpDispatchBuilder::AVX128_InsertScalarRound<8>},
    // TODO: {OPD(3, 0b01, 0x0C), 1, &OpDispatchBuilder::VPBLENDDOp},
    // TODO: {OPD(3, 0b01, 0x0D), 1, &OpDispatchBuilder::VBLENDPDOp},
    // TODO: {OPD(3, 0b01, 0x0E), 1, &OpDispatchBuilder::VPBLENDWOp},
    // TODO: {OPD(3, 0b01, 0x0F), 1, &OpDispatchBuilder::VPALIGNROp},

    {OPD(3, 0b01, 0x14), 1, &OpDispatchBuilder::AVX128_PExtr<1>},
    {OPD(3, 0b01, 0x15), 1, &OpDispatchBuilder::AVX128_PExtr<2>},
    {OPD(3, 0b01, 0x16), 1, &OpDispatchBuilder::AVX128_PExtr<4>},
    {OPD(3, 0b01, 0x17), 1, &OpDispatchBuilder::AVX128_PExtr<4>},

    {OPD(3, 0b01, 0x18), 1, &OpDispatchBuilder::AVX128_VINSERT},
    {OPD(3, 0b01, 0x19), 1, &OpDispatchBuilder::AVX128_VEXTRACT128},
    {OPD(3, 0b01, 0x20), 1, &OpDispatchBuilder::AVX128_VPINSRB},
    {OPD(3, 0b01, 0x21), 1, &OpDispatchBuilder::VINSERTPSOp},
    {OPD(3, 0b01, 0x22), 1, &OpDispatchBuilder::AVX128_VPINSRDQ},

    {OPD(3, 0b01, 0x38), 1, &OpDispatchBuilder::AVX128_VINSERT},
    {OPD(3, 0b01, 0x39), 1, &OpDispatchBuilder::AVX128_VEXTRACT128},

    {OPD(3, 0b01, 0x40), 1, &OpDispatchBuilder::AVX128_VDPP<4>},
    {OPD(3, 0b01, 0x41), 1, &OpDispatchBuilder::AVX128_VDPP<8>},
    // TODO: {OPD(3, 0b01, 0x42), 1, &OpDispatchBuilder::VMPSADBWOp},

    // TODO: {OPD(3, 0b01, 0x46), 1, &OpDispatchBuilder::VPERM2Op},

    // TODO: {OPD(3, 0b01, 0x4A), 1, &OpDispatchBuilder::AVXVectorVariableBlend<4>},
    // TODO: {OPD(3, 0b01, 0x4B), 1, &OpDispatchBuilder::AVXVectorVariableBlend<8>},
    // TODO: {OPD(3, 0b01, 0x4C), 1, &OpDispatchBuilder::AVXVectorVariableBlend<1>},

    {OPD(3, 0b01, 0x60), 1, &OpDispatchBuilder::AVX128_VPCMPESTRM},
    {OPD(3, 0b01, 0x61), 1, &OpDispatchBuilder::AVX128_VPCMPESTRI},
    {OPD(3, 0b01, 0x62), 1, &OpDispatchBuilder::AVX128_VPCMPISTRM},
    {OPD(3, 0b01, 0x63), 1, &OpDispatchBuilder::AVX128_VPCMPISTRI},

    {OPD(3, 0b01, 0xDF), 1, &OpDispatchBuilder::AVX128_VAESKeyGenAssist},
  };
#undef OPD

#define OPD(group, pp, opcode) (((group - X86Tables::TYPE_VEX_GROUP_12) << 4) | (pp << 3) | (opcode))
  static constexpr std::tuple<uint8_t, uint8_t, X86Tables::OpDispatchPtr> VEX128TableGroupOps[] {
    {OPD(X86Tables::TYPE_VEX_GROUP_12, 1, 0b010), 1, &OpDispatchBuilder::AVX128_VPSRLI<2>},
    {OPD(X86Tables::TYPE_VEX_GROUP_12, 1, 0b110), 1, &OpDispatchBuilder::AVX128_VPSLLI<2>},
    {OPD(X86Tables::TYPE_VEX_GROUP_12, 1, 0b100), 1, &OpDispatchBuilder::AVX128_VPSRAI<2>},

    {OPD(X86Tables::TYPE_VEX_GROUP_13, 1, 0b010), 1, &OpDispatchBuilder::AVX128_VPSRLI<4>},
    {OPD(X86Tables::TYPE_VEX_GROUP_13, 1, 0b110), 1, &OpDispatchBuilder::AVX128_VPSLLI<4>},
    {OPD(X86Tables::TYPE_VEX_GROUP_13, 1, 0b100), 1, &OpDispatchBuilder::AVX128_VPSRAI<4>},

    {OPD(X86Tables::TYPE_VEX_GROUP_14, 1, 0b010), 1, &OpDispatchBuilder::AVX128_VPSRLI<8>},
    {OPD(X86Tables::TYPE_VEX_GROUP_14, 1, 0b011), 1, &OpDispatchBuilder::AVX128_VPSRLDQ},
    {OPD(X86Tables::TYPE_VEX_GROUP_14, 1, 0b110), 1, &OpDispatchBuilder::AVX128_VPSLLI<8>},
    {OPD(X86Tables::TYPE_VEX_GROUP_14, 1, 0b111), 1, &OpDispatchBuilder::AVX128_VPSLLDQ},

    // TODO: {OPD(X86Tables::TYPE_VEX_GROUP_15, 0, 0b010), 1, &OpDispatchBuilder::LDMXCSR},
    // TODO: {OPD(X86Tables::TYPE_VEX_GROUP_15, 0, 0b011), 1, &OpDispatchBuilder::STMXCSR},
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
    A = AddSegmentToAddress(A, Flags);

    AddressMode HighA = A;
    HighA.Offset += 16;

    ///< TODO: Implement VSIB once we get there.
    if (Operand.IsSIB()) {
      const bool IsVSIB = (Op->Flags & X86Tables::DecodeFlags::FLAG_VSIB_BYTE) != 0;
      LOGMAN_THROW_AA_FMT(!IsVSIB, "VSIB currently unsupported");
    }

    return {
      .Low = _LoadMemAutoTSO(FPRClass, 16, A, 1),
      .High = NeedsHigh ? _LoadMemAutoTSO(FPRClass, 16, HighA, 1) : nullptr,
    };
  }
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
    A = AddSegmentToAddress(A, Op->Flags);

    _StoreMemAutoTSO(FPRClass, 16, A, Src.Low, 1);

    if (Src.High) {
      AddressMode HighA = A;
      HighA.Offset += 16;

      _StoreMemAutoTSO(FPRClass, 16, HighA, Src.High, 1);
    }
  }
}

Ref OpDispatchBuilder::AVX128_LoadXMMRegister(uint32_t XMM, bool High) {
  if (High) {
    return _LoadContext(16, FPRClass, offsetof(FEXCore::Core::CPUState, avx_high[XMM][0]));
  } else {
    return _LoadRegister(XMM, FPRClass, 16);
  }
}

void OpDispatchBuilder::AVX128_StoreXMMRegister(uint32_t XMM, const Ref Src, bool High) {
  if (High) {
    _StoreContext(16, FPRClass, Src, offsetof(FEXCore::Core::CPUState, avx_high[XMM][0]));
  } else {
    _StoreRegister(Src, XMM, FPRClass, 16);
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

void OpDispatchBuilder::AVX128_VectorALUImpl(OpcodeArgs, IROps IROp, size_t ElementSize) {
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

void OpDispatchBuilder::AVX128_VectorUnaryImpl(OpcodeArgs, IROps IROp, size_t ElementSize) {
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
    DeriveOp(High, IROp, _VUShrSWide(OpSize::i128Bit, ElementSize, Src1.High, Src2.High));
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
      DeriveOp(High, IROp, _VUShrI(OpSize::i128Bit, ElementSize, Src.Low, ShiftConstant));
      Result.High = High;
    }
  }

  if (Is128Bit) {
    Result.High = LoadZeroVector(OpSize::i128Bit);
  }

  AVX128_StoreResult_WithOpSize(Op, Op->Dest, Result);
}

template<IROps IROp, size_t ElementSize>
void OpDispatchBuilder::AVX128_VectorALU(OpcodeArgs) {
  AVX128_VectorALUImpl(Op, IROp, ElementSize);
}

template<IROps IROp, size_t ElementSize>
void OpDispatchBuilder::AVX128_VectorUnary(OpcodeArgs) {
  AVX128_VectorUnaryImpl(Op, IROp, ElementSize);
}

void OpDispatchBuilder::AVX128_VZERO(OpcodeArgs) {
  const auto DstSize = GetDstSize(Op);
  const auto IsVZEROALL = DstSize == Core::CPUState::XMM_AVX_REG_SIZE;
  const auto NumRegs = CTX->Config.Is64BitMode ? 16U : 8U;

  if (IsVZEROALL) {
    // NOTE: Despite the name being VZEROALL, this will still only ever
    //       zero out up to the first 16 registers (even on AVX-512, where we have 32 registers)

    for (uint32_t i = 0; i < NumRegs; i++) {
      // Explicitly not caching named vector zero. This ensures that every register gets movi #0.0 directly.
      Ref ZeroVector = LoadUncachedZeroVector(OpSize::i128Bit);
      AVX128_StoreXMMRegister(i, ZeroVector, false);
    }

    // More efficient for non-SRA upper-halves to cache the constant and store directly.
    const auto ZeroVector = LoadZeroVector(OpSize::i128Bit);
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

  auto Src = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, !Is128Bit, MemoryAccessType::STREAM);
  AVX128_StoreResult_WithOpSize(Op, Op->Dest, Src);
}

void OpDispatchBuilder::AVX128_MOVQ(OpcodeArgs) {
  auto Src = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, false);
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

  if (Op->Dest.IsGPR()) {
    auto Src2 = AVX128_LoadSource_WithOpSize(Op, Op->Src[1], Op->Flags, false);

    // Bits[63:0] come from Src2[63:0]
    // Bits[127:64] come from Src1[127:64]
    Ref Result_Low = _VInsElement(OpSize::i128Bit, OpSize::i64Bit, 1, 1, Src2.Low, Src1.Low);
    Ref ZeroVector = LoadZeroVector(OpSize::i128Bit);

    AVX128_StoreResult_WithOpSize(Op, Op->Dest, RefPair {.Low = Result_Low, .High = ZeroVector});
  } else {
    StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Src1.Low, OpSize::i64Bit, OpSize::i64Bit);
  }
}

void OpDispatchBuilder::AVX128_VMOVHP(OpcodeArgs) {
  auto Src1 = AVX128_LoadSource_WithOpSize(Op, Op->Src[0], Op->Flags, false);

  if (Op->Dest.IsGPR()) {
    auto Src2 = AVX128_LoadSource_WithOpSize(Op, Op->Src[1], Op->Flags, false);

    // Bits[63:0] come from Src1[63:0]
    // Bits[127:64] come from Src2[63:0]
    Ref Result_Low = _VZip(OpSize::i128Bit, OpSize::i64Bit, Src1.Low, Src2.Low);
    Ref ZeroVector = LoadZeroVector(OpSize::i128Bit);

    AVX128_StoreResult_WithOpSize(Op, Op->Dest, RefPair {.Low = Result_Low, .High = ZeroVector});
  } else {
    // Need to store Bits[127:64]. Duplicate the element to get it in the low bits.
    Src1.Low = _VDupElement(OpSize::i128Bit, OpSize::i64Bit, Src1.Low, 1);
    StoreResult_WithOpSize(FPRClass, Op, Op->Dest, Src1.Low, OpSize::i64Bit, OpSize::i64Bit);
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
  const auto Is128Bit = DstSize == Core::CPUState::XMM_SSE_REG_SIZE;

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

  Result.High = LoadZeroVector(OpSize::i128Bit);
  LOGMAN_THROW_A_FMT(Is128Bit, "Programming Error: This should never occur!");

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

template<IROps IROp, size_t ElementSize>
void OpDispatchBuilder::AVX128_VectorScalarInsertALU(OpcodeArgs) {
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

  AVX128_VectorBinaryImpl(Op, GetSrcSize(Op), ElementSize, [this, Op, CompType](size_t _ElementSize, Ref Src1, Ref Src2) {
    return VFCMPOpImpl(Op, _ElementSize, Src1, Src2, CompType);
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

template<size_t ElementSize, size_t DstElementSize, bool Signed>
void OpDispatchBuilder::AVX128_ExtendVectorElements(OpcodeArgs) {
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

  auto Transform = [this](Ref Src) {
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

template<size_t ElementSize>
void OpDispatchBuilder::AVX128_VPSRA(OpcodeArgs) {
  AVX128_VectorShiftWideImpl(Op, ElementSize, IROps::OP_VSSHRSWIDE);
}

template<size_t ElementSize>
void OpDispatchBuilder::AVX128_VPSLL(OpcodeArgs) {
  AVX128_VectorShiftWideImpl(Op, ElementSize, IROps::OP_VUSHLSWIDE);
}

template<size_t ElementSize>
void OpDispatchBuilder::AVX128_VPSRL(OpcodeArgs) {
  AVX128_VectorShiftWideImpl(Op, ElementSize, IROps::OP_VUSHRSWIDE);
}

void OpDispatchBuilder::AVX128_VariableShiftImpl(OpcodeArgs, IROps IROp) {
  AVX128_VectorBinaryImpl(Op, GetDstSize(Op), GetSrcSize(Op), [this, IROp](size_t ElementSize, Ref Src1, Ref Src2) {
    DeriveOp(Shift, IROp, _VUShr(OpSize::i128Bit, ElementSize, Src1, Src2, true));
    return Shift;
  });
}

void OpDispatchBuilder::AVX128_VPSLLV(OpcodeArgs) {
  AVX128_VariableShiftImpl(Op, IROps::OP_VUSHL);
}

void OpDispatchBuilder::AVX128_VPSRAVD(OpcodeArgs) {
  AVX128_VariableShiftImpl(Op, IROps::OP_VSSHR);
}

void OpDispatchBuilder::AVX128_VPSRLV(OpcodeArgs) {
  AVX128_VariableShiftImpl(Op, IROps::OP_VUSHR);
}

template<size_t ElementSize>
void OpDispatchBuilder::AVX128_VPSRLI(OpcodeArgs) {
  AVX128_VectorShiftImmImpl(Op, ElementSize, IROps::OP_VUSHRI);
}

template<size_t ElementSize>
void OpDispatchBuilder::AVX128_VPSLLI(OpcodeArgs) {
  AVX128_VectorShiftImmImpl(Op, ElementSize, IROps::OP_VSHLI);
}

template<size_t ElementSize>
void OpDispatchBuilder::AVX128_VPSRAI(OpcodeArgs) {
  AVX128_VectorShiftImmImpl(Op, ElementSize, IROps::OP_VSSHRI);
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

void OpDispatchBuilder::AVX128_VPSRLDQ(OpcodeArgs) {
  AVX128_ShiftDoubleImm(Op, ShiftDirection::RIGHT);
}

void OpDispatchBuilder::AVX128_VPSLLDQ(OpcodeArgs) {
  AVX128_ShiftDoubleImm(Op, ShiftDirection::LEFT);
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
    return _Vector_FToF2(OpSize::i128Bit, DstElementSize, Src, SrcElementSize);
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

  RefPair Result {};

  Result.Low = Convert(Src.Low);

  if (!Is128BitSrc) {
    if (!Narrow) {
      Result.High = Convert(Src.High);
    } else {
      Result.Low = _VInsElement(OpSize::i128Bit, OpSize::i64Bit, 1, 0, Result.Low, Convert(Src.High));
    }
  } else {
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

  AVX128_VectorUnaryImpl(Op, OpSize::i128Bit, OpSize::i128Bit, [this, ZeroRegister, KeyGenSwizzle, RCON](size_t, Ref Src) {
    return _VAESKeyGenAssist(Src, KeyGenSwizzle, ZeroRegister, RCON);
  });
}

void OpDispatchBuilder::AVX128_VPCMPESTRI(OpcodeArgs) {
  PCMPXSTRXOpImpl(Op, true, false);

  ///< Zero the upper 128-bits of hardcoded YMM0
  AVX128_StoreXMMRegister(0, LoadZeroVector(OpSize::i128Bit), true);
}

void OpDispatchBuilder::AVX128_VPCMPESTRM(OpcodeArgs) {
  PCMPXSTRXOpImpl(Op, true, true);

  ///< Zero the upper 128-bits of hardcoded YMM0
  AVX128_StoreXMMRegister(0, LoadZeroVector(OpSize::i128Bit), true);
}

void OpDispatchBuilder::AVX128_VPCMPISTRI(OpcodeArgs) {
  PCMPXSTRXOpImpl(Op, false, false);

  ///< Zero the upper 128-bits of hardcoded YMM0
  AVX128_StoreXMMRegister(0, LoadZeroVector(OpSize::i128Bit), true);
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

  if (Selector == 0x00 || Selector == 0x55) {
    // If we're just broadcasting one element in particular across the vector
    // then this can be done fairly simply without any individual inserts.
    // Low 128-bit version.
    const auto Index = Selector & 0b11;
    Result.Low = _VDupElement(OpSize::i128Bit, OpSize::i64Bit, Src.Low, Index);
    Result.High = Result.Low;
  } else if (Selector == 0xAA || Selector == 0xFF) {
    // High 128-bit version.
    const auto Index = (Selector & 0b11) - 2;
    Result.Low = _VDupElement(OpSize::i128Bit, OpSize::i64Bit, Src.High, Index);
    Result.High = Result.Low;
  } else {
    ///< TODO: This can be more optimized.
    auto ZeroRegister = LoadZeroVector(OpSize::i128Bit);
    Ref Selections[4] = {
      Src.Low,
      Src.Low,
      Src.High,
      Src.High,
    };
    uint8_t SrcIndex {};
    SrcIndex = (Selector >> (0 * 2)) & 0b11;
    Result.Low = _VInsElement(OpSize::i128Bit, OpSize::i64Bit, 0, SrcIndex & 1, ZeroRegister, Selections[SrcIndex]);
    SrcIndex = (Selector >> (1 * 2)) & 0b11;
    Result.Low = _VInsElement(OpSize::i128Bit, OpSize::i64Bit, 1, SrcIndex & 1, Result.Low, Selections[SrcIndex]);

    SrcIndex = (Selector >> (2 * 2)) & 0b11;
    Result.High = _VInsElement(OpSize::i128Bit, OpSize::i64Bit, 0, SrcIndex & 1, ZeroRegister, Selections[SrcIndex]);
    SrcIndex = (Selector >> (3 * 2)) & 0b11;
    Result.High = _VInsElement(OpSize::i128Bit, OpSize::i64Bit, 1, SrcIndex & 1, Result.High, Selections[SrcIndex]);
  }

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

  ///< TODO: This could be optimized.
  if constexpr (ElementSize == OpSize::i64Bit) {
    Result.Low = _VInsElement(OpSize::i128Bit, ElementSize, 0, Selector & 0b0001, Result.Low, Src.Low);
    Result.Low = _VInsElement(OpSize::i128Bit, ElementSize, 1, (Selector & 0b0010) >> 1, Result.Low, Src.Low);

    if (!Is128Bit) {
      Result.High = _VInsElement(OpSize::i128Bit, ElementSize, 0, ((Selector & 0b0100) >> 2), Result.High, Src.High);
      Result.High = _VInsElement(OpSize::i128Bit, ElementSize, 1, ((Selector & 0b1000) >> 3), Result.High, Src.High);
    }
  } else {
    Result.Low = _VInsElement(OpSize::i128Bit, ElementSize, 0, Selector & 0b00000011, Result.Low, Src.Low);
    Result.Low = _VInsElement(OpSize::i128Bit, ElementSize, 1, (Selector & 0b00001100) >> 2, Result.Low, Src.Low);
    Result.Low = _VInsElement(OpSize::i128Bit, ElementSize, 2, (Selector & 0b00110000) >> 4, Result.Low, Src.Low);
    Result.Low = _VInsElement(OpSize::i128Bit, ElementSize, 3, (Selector & 0b11000000) >> 6, Result.Low, Src.Low);

    if (!Is128Bit) {
      Result.High = _VInsElement(OpSize::i128Bit, ElementSize, 0, (Selector & 0b00000011), Result.High, Src.High);
      Result.High = _VInsElement(OpSize::i128Bit, ElementSize, 1, ((Selector & 0b00001100) >> 2), Result.High, Src.High);
      Result.High = _VInsElement(OpSize::i128Bit, ElementSize, 2, ((Selector & 0b00110000) >> 4), Result.High, Src.High);
      Result.High = _VInsElement(OpSize::i128Bit, ElementSize, 3, ((Selector & 0b11000000) >> 6), Result.High, Src.High);
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

} // namespace FEXCore::IR
