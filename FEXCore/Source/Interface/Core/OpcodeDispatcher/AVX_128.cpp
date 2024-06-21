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

    // TODO: {OPD(1, 0b00, 0x5A), 1, &OpDispatchBuilder::AVXVector_CVT_Float_To_Float<8, 4>},
    // TODO: {OPD(1, 0b01, 0x5A), 1, &OpDispatchBuilder::AVXVector_CVT_Float_To_Float<4, 8>},
    // TODO: {OPD(1, 0b10, 0x5A), 1, &OpDispatchBuilder::AVXInsertScalar_CVT_Float_To_Float<8, 4>},
    // TODO: {OPD(1, 0b11, 0x5A), 1, &OpDispatchBuilder::AVXInsertScalar_CVT_Float_To_Float<4, 8>},

    // TODO: {OPD(1, 0b00, 0x5B), 1, &OpDispatchBuilder::AVXVector_CVT_Int_To_Float<4, false>},
    // TODO: {OPD(1, 0b01, 0x5B), 1, &OpDispatchBuilder::AVXVector_CVT_Float_To_Int<4, false, true>},
    // TODO: {OPD(1, 0b10, 0x5B), 1, &OpDispatchBuilder::AVXVector_CVT_Float_To_Int<4, false, false>},

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

    // TODO: {OPD(1, 0b01, 0x70), 1, &OpDispatchBuilder::VPSHUFWOp<4, true>},
    // TODO: {OPD(1, 0b10, 0x70), 1, &OpDispatchBuilder::VPSHUFWOp<2, false>},
    // TODO: {OPD(1, 0b11, 0x70), 1, &OpDispatchBuilder::VPSHUFWOp<2, true>},

    {OPD(1, 0b01, 0x74), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VCMPEQ, 1>},
    {OPD(1, 0b01, 0x75), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VCMPEQ, 2>},
    {OPD(1, 0b01, 0x76), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VCMPEQ, 4>},

    {OPD(1, 0b00, 0x77), 1, &OpDispatchBuilder::AVX128_VZERO},

    // TODO: {OPD(1, 0b01, 0x7C), 1, &OpDispatchBuilder::VHADDPOp<IR::OP_VFADDP, 8>},
    // TODO: {OPD(1, 0b11, 0x7C), 1, &OpDispatchBuilder::VHADDPOp<IR::OP_VFADDP, 4>},
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

    // TODO: {OPD(1, 0b01, 0xC4), 1, &OpDispatchBuilder::VPINSRWOp},
    {OPD(1, 0b01, 0xC5), 1, &OpDispatchBuilder::AVX128_PExtr<2>},

    // TODO: {OPD(1, 0b00, 0xC6), 1, &OpDispatchBuilder::VSHUFOp<4>},
    // TODO: {OPD(1, 0b01, 0xC6), 1, &OpDispatchBuilder::VSHUFOp<8>},

    // TODO: {OPD(1, 0b01, 0xD0), 1, &OpDispatchBuilder::VADDSUBPOp<8>},
    // TODO: {OPD(1, 0b11, 0xD0), 1, &OpDispatchBuilder::VADDSUBPOp<4>},

    // TODO: {OPD(1, 0b01, 0xD1), 1, &OpDispatchBuilder::VPSRLDOp<2>},
    // TODO: {OPD(1, 0b01, 0xD2), 1, &OpDispatchBuilder::VPSRLDOp<4>},
    // TODO: {OPD(1, 0b01, 0xD3), 1, &OpDispatchBuilder::VPSRLDOp<8>},
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
    // TODO: {OPD(1, 0b01, 0xE1), 1, &OpDispatchBuilder::VPSRAOp<2>},
    // TODO: {OPD(1, 0b01, 0xE2), 1, &OpDispatchBuilder::VPSRAOp<4>},
    {OPD(1, 0b01, 0xE3), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VURAVG, 2>},
    // TODO: {OPD(1, 0b01, 0xE4), 1, &OpDispatchBuilder::VPMULHWOp<false>},
    // TODO: {OPD(1, 0b01, 0xE5), 1, &OpDispatchBuilder::VPMULHWOp<true>},

    // TODO: {OPD(1, 0b01, 0xE6), 1, &OpDispatchBuilder::AVXVector_CVT_Float_To_Int<8, true, false>},
    // TODO: {OPD(1, 0b10, 0xE6), 1, &OpDispatchBuilder::AVXVector_CVT_Int_To_Float<4, true>},
    // TODO: {OPD(1, 0b11, 0xE6), 1, &OpDispatchBuilder::AVXVector_CVT_Float_To_Int<8, true, true>},

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
    // TODO: {OPD(1, 0b01, 0xF1), 1, &OpDispatchBuilder::VPSLLOp<2>},
    // TODO: {OPD(1, 0b01, 0xF2), 1, &OpDispatchBuilder::VPSLLOp<4>},
    // TODO: {OPD(1, 0b01, 0xF3), 1, &OpDispatchBuilder::VPSLLOp<8>},
    // TODO: {OPD(1, 0b01, 0xF4), 1, &OpDispatchBuilder::VPMULLOp<4, false>},
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
    // TODO: {OPD(2, 0b01, 0x01), 1, &OpDispatchBuilder::VHADDPOp<IR::OP_VADDP, 2>},
    // TODO: {OPD(2, 0b01, 0x02), 1, &OpDispatchBuilder::VHADDPOp<IR::OP_VADDP, 4>},
    // TODO: {OPD(2, 0b01, 0x03), 1, &OpDispatchBuilder::VPHADDSWOp},
    // TODO: {OPD(2, 0b01, 0x04), 1, &OpDispatchBuilder::VPMADDUBSWOp},

    // TODO: {OPD(2, 0b01, 0x05), 1, &OpDispatchBuilder::VPHSUBOp<2>},
    // TODO: {OPD(2, 0b01, 0x06), 1, &OpDispatchBuilder::VPHSUBOp<4>},
    // TODO: {OPD(2, 0b01, 0x07), 1, &OpDispatchBuilder::VPHSUBSWOp},

    {OPD(2, 0b01, 0x08), 1, &OpDispatchBuilder::AVX128_VPSIGN<1>},
    {OPD(2, 0b01, 0x09), 1, &OpDispatchBuilder::AVX128_VPSIGN<2>},
    {OPD(2, 0b01, 0x0A), 1, &OpDispatchBuilder::AVX128_VPSIGN<4>},
    // TODO: {OPD(2, 0b01, 0x0B), 1, &OpDispatchBuilder::VPMULHRSWOp},
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

    // TODO: {OPD(2, 0b01, 0x28), 1, &OpDispatchBuilder::VPMULLOp<4, true>},
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
    // TODO: {OPD(2, 0b01, 0x41), 1, &OpDispatchBuilder::PHMINPOSUWOp},
    // TODO: {OPD(2, 0b01, 0x45), 1, &OpDispatchBuilder::VPSRLVOp},
    // TODO: {OPD(2, 0b01, 0x46), 1, &OpDispatchBuilder::VPSRAVDOp},
    // TODO: {OPD(2, 0b01, 0x47), 1, &OpDispatchBuilder::VPSLLVOp},

    {OPD(2, 0b01, 0x58), 1, &OpDispatchBuilder::AVX128_VBROADCAST<4>},
    {OPD(2, 0b01, 0x59), 1, &OpDispatchBuilder::AVX128_VBROADCAST<8>},
    {OPD(2, 0b01, 0x5A), 1, &OpDispatchBuilder::AVX128_VBROADCAST<16>},

    {OPD(2, 0b01, 0x78), 1, &OpDispatchBuilder::AVX128_VBROADCAST<1>},
    {OPD(2, 0b01, 0x79), 1, &OpDispatchBuilder::AVX128_VBROADCAST<2>},

    // TODO: {OPD(2, 0b01, 0x8C), 1, &OpDispatchBuilder::VPMASKMOVOp<false>},
    // TODO: {OPD(2, 0b01, 0x8E), 1, &OpDispatchBuilder::VPMASKMOVOp<true>},

    // TODO: {OPD(2, 0b01, 0xDB), 1, &OpDispatchBuilder::AESImcOp},
    // TODO: {OPD(2, 0b01, 0xDC), 1, &OpDispatchBuilder::VAESEncOp},
    // TODO: {OPD(2, 0b01, 0xDD), 1, &OpDispatchBuilder::VAESEncLastOp},
    // TODO: {OPD(2, 0b01, 0xDE), 1, &OpDispatchBuilder::VAESDecOp},
    // TODO: {OPD(2, 0b01, 0xDF), 1, &OpDispatchBuilder::VAESDecLastOp},

    // TODO: {OPD(3, 0b01, 0x00), 1, &OpDispatchBuilder::VPERMQOp},
    // TODO: {OPD(3, 0b01, 0x01), 1, &OpDispatchBuilder::VPERMQOp},
    // TODO: {OPD(3, 0b01, 0x02), 1, &OpDispatchBuilder::VPBLENDDOp},
    // TODO: {OPD(3, 0b01, 0x04), 1, &OpDispatchBuilder::VPERMILImmOp<4>},
    // TODO: {OPD(3, 0b01, 0x05), 1, &OpDispatchBuilder::VPERMILImmOp<8>},
    // TODO: {OPD(3, 0b01, 0x06), 1, &OpDispatchBuilder::VPERM2Op},
    // TODO: {OPD(3, 0b01, 0x08), 1, &OpDispatchBuilder::AVXVectorRound<4>},
    // TODO: {OPD(3, 0b01, 0x09), 1, &OpDispatchBuilder::AVXVectorRound<8>},
    // TODO: {OPD(3, 0b01, 0x0A), 1, &OpDispatchBuilder::AVXInsertScalarRound<4>},
    // TODO: {OPD(3, 0b01, 0x0B), 1, &OpDispatchBuilder::AVXInsertScalarRound<8>},
    // TODO: {OPD(3, 0b01, 0x0C), 1, &OpDispatchBuilder::VPBLENDDOp},
    // TODO: {OPD(3, 0b01, 0x0D), 1, &OpDispatchBuilder::VBLENDPDOp},
    // TODO: {OPD(3, 0b01, 0x0E), 1, &OpDispatchBuilder::VPBLENDWOp},
    // TODO: {OPD(3, 0b01, 0x0F), 1, &OpDispatchBuilder::VPALIGNROp},

    {OPD(3, 0b01, 0x14), 1, &OpDispatchBuilder::AVX128_PExtr<1>},
    {OPD(3, 0b01, 0x15), 1, &OpDispatchBuilder::AVX128_PExtr<2>},
    {OPD(3, 0b01, 0x16), 1, &OpDispatchBuilder::AVX128_PExtr<4>},
    {OPD(3, 0b01, 0x17), 1, &OpDispatchBuilder::AVX128_PExtr<4>},

    // TODO: {OPD(3, 0b01, 0x18), 1, &OpDispatchBuilder::VINSERTOp},
    // TODO: {OPD(3, 0b01, 0x19), 1, &OpDispatchBuilder::VEXTRACT128Op},
    // TODO: {OPD(3, 0b01, 0x20), 1, &OpDispatchBuilder::VPINSRBOp},
    // TODO: {OPD(3, 0b01, 0x21), 1, &OpDispatchBuilder::VINSERTPSOp},
    // TODO: {OPD(3, 0b01, 0x22), 1, &OpDispatchBuilder::VPINSRDQOp},

    // TODO: {OPD(3, 0b01, 0x38), 1, &OpDispatchBuilder::VINSERTOp},
    // TODO: {OPD(3, 0b01, 0x39), 1, &OpDispatchBuilder::VEXTRACT128Op},

    // TODO: {OPD(3, 0b01, 0x40), 1, &OpDispatchBuilder::VDPPOp<4>},
    // TODO: {OPD(3, 0b01, 0x41), 1, &OpDispatchBuilder::VDPPOp<8>},
    // TODO: {OPD(3, 0b01, 0x42), 1, &OpDispatchBuilder::VMPSADBWOp},

    // TODO: {OPD(3, 0b01, 0x46), 1, &OpDispatchBuilder::VPERM2Op},

    // TODO: {OPD(3, 0b01, 0x4A), 1, &OpDispatchBuilder::AVXVectorVariableBlend<4>},
    // TODO: {OPD(3, 0b01, 0x4B), 1, &OpDispatchBuilder::AVXVectorVariableBlend<8>},
    // TODO: {OPD(3, 0b01, 0x4C), 1, &OpDispatchBuilder::AVXVectorVariableBlend<1>},

    // TODO: {OPD(3, 0b01, 0x60), 1, &OpDispatchBuilder::VPCMPESTRMOp},
    // TODO: {OPD(3, 0b01, 0x61), 1, &OpDispatchBuilder::VPCMPESTRIOp},
    // TODO: {OPD(3, 0b01, 0x62), 1, &OpDispatchBuilder::VPCMPISTRMOp},
    // TODO: {OPD(3, 0b01, 0x63), 1, &OpDispatchBuilder::VPCMPISTRIOp},

    // TODO: {OPD(3, 0b01, 0xDF), 1, &OpDispatchBuilder::AESKeyGenAssist},
  };
#undef OPD

#define OPD(group, pp, opcode) (((group - X86Tables::TYPE_VEX_GROUP_12) << 4) | (pp << 3) | (opcode))
  static constexpr std::tuple<uint8_t, uint8_t, X86Tables::OpDispatchPtr> VEX128TableGroupOps[] {
    // TODO: {OPD(X86Tables::TYPE_VEX_GROUP_12, 1, 0b010), 1, &OpDispatchBuilder::VPSRLIOp<2>},
    // TODO: {OPD(X86Tables::TYPE_VEX_GROUP_12, 1, 0b110), 1, &OpDispatchBuilder::VPSLLIOp<2>},
    // TODO: {OPD(X86Tables::TYPE_VEX_GROUP_12, 1, 0b100), 1, &OpDispatchBuilder::VPSRAIOp<2>},

    // TODO: {OPD(X86Tables::TYPE_VEX_GROUP_13, 1, 0b010), 1, &OpDispatchBuilder::VPSRLIOp<4>},
    // TODO: {OPD(X86Tables::TYPE_VEX_GROUP_13, 1, 0b110), 1, &OpDispatchBuilder::VPSLLIOp<4>},
    // TODO: {OPD(X86Tables::TYPE_VEX_GROUP_13, 1, 0b100), 1, &OpDispatchBuilder::VPSRAIOp<4>},

    // TODO: {OPD(X86Tables::TYPE_VEX_GROUP_14, 1, 0b010), 1, &OpDispatchBuilder::VPSRLIOp<8>},
    // TODO: {OPD(X86Tables::TYPE_VEX_GROUP_14, 1, 0b011), 1, &OpDispatchBuilder::VPSRLDQOp},
    // TODO: {OPD(X86Tables::TYPE_VEX_GROUP_14, 1, 0b110), 1, &OpDispatchBuilder::VPSLLIOp<8>},
    // TODO: {OPD(X86Tables::TYPE_VEX_GROUP_14, 1, 0b111), 1, &OpDispatchBuilder::VPSLLDQOp},

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

  HandleNZCVWrite();
  _FCmp(ElementSize, Src1.Low, Src2.Low);
  ConvertNZCVToSSE();

  // Zero AF. Note that the comparison sets the raw PF to 0/1 above, so PF[4] is
  // 0 so the XOR with PF will have no effect, so setting the AF byte to zero
  // will indeed zero AF as intended.
  SetRFLAG<FEXCore::X86State::RFLAG_AF_RAW_LOC>(_Constant(0));
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

Ref OpDispatchBuilder::AVX128_VFCMPImpl(size_t ElementSize, Ref Src1, Ref Src2, uint8_t CompType) {
  Ref Result {};
  switch (CompType) {
  case 0x00:
  case 0x08:
  case 0x10:
  case 0x18: // EQ
    return _VFCMPEQ(OpSize::i128Bit, ElementSize, Src1, Src2);
    break;
  case 0x01:
  case 0x09:
  case 0x11:
  case 0x19: // LT, GT(Swapped operand)
    return _VFCMPLT(OpSize::i128Bit, ElementSize, Src1, Src2);
    break;
  case 0x02:
  case 0x0A:
  case 0x12:
  case 0x1A: // LE, GE(Swapped operand)
    return _VFCMPLE(OpSize::i128Bit, ElementSize, Src1, Src2);
    break;
  case 0x03:
  case 0x0B:
  case 0x13:
  case 0x1B: // Unordered
    return _VFCMPUNO(OpSize::i128Bit, ElementSize, Src1, Src2);
    break;
  case 0x04:
  case 0x0C:
  case 0x14:
  case 0x1C: // NEQ
    return _VFCMPNEQ(OpSize::i128Bit, ElementSize, Src1, Src2);
    break;
  case 0x05:
  case 0x0D:
  case 0x15:
  case 0x1D: // NLT, NGT(Swapped operand)
    Result = _VFCMPLT(OpSize::i128Bit, ElementSize, Src1, Src2);
    return _VNot(OpSize::i128Bit, ElementSize, Result);
    break;
  case 0x06:
  case 0x0E:
  case 0x16:
  case 0x1E: // NLE, NGE(Swapped operand)
    Result = _VFCMPLE(OpSize::i128Bit, ElementSize, Src1, Src2);
    return _VNot(OpSize::i128Bit, ElementSize, Result);
    break;
  case 0x07:
  case 0x0F:
  case 0x17:
  case 0x1F: // Ordered
    return _VFCMPORD(OpSize::i128Bit, ElementSize, Src1, Src2);
    break;
  default: LOGMAN_MSG_A_FMT("Unknown Comparison type: {}", CompType); break;
  }

  FEX_UNREACHABLE;
}

template<size_t ElementSize>
void OpDispatchBuilder::AVX128_VFCMP(OpcodeArgs) {
  LOGMAN_THROW_A_FMT(Op->Src[2].IsLiteral(), "Src[2] needs to be literal");
  const uint8_t CompType = Op->Src[2].Data.Literal.Value;

  AVX128_VectorBinaryImpl(Op, GetSrcSize(Op), ElementSize, [this, Op, CompType](size_t _ElementSize, Ref Src1, Ref Src2) {
    return VFCMPOpImpl(Op, _ElementSize, Src1, Src2, CompType);
  });
}

Ref OpDispatchBuilder::AVX128_InsertScalarFCMPImpl(size_t ElementSize, Ref Src1, Ref Src2, uint8_t CompType) {
  switch (CompType) {
  case 0x00:
  case 0x08:
  case 0x10:
  case 0x18: // EQ
    return _VFCMPScalarInsert(OpSize::i128Bit, ElementSize, Src1, Src2, FloatCompareOp::EQ, false);
  case 0x01:
  case 0x09:
  case 0x11:
  case 0x19: // LT, GT(Swapped operand)
    return _VFCMPScalarInsert(OpSize::i128Bit, ElementSize, Src1, Src2, FloatCompareOp::LT, false);
  case 0x02:
  case 0x0A:
  case 0x12:
  case 0x1A: // LE, GE(Swapped operand)
    return _VFCMPScalarInsert(OpSize::i128Bit, ElementSize, Src1, Src2, FloatCompareOp::LE, false);
  case 0x03:
  case 0x0B:
  case 0x13:
  case 0x1B: // Unordered
    return _VFCMPScalarInsert(OpSize::i128Bit, ElementSize, Src1, Src2, FloatCompareOp::UNO, false);
  case 0x04:
  case 0x0C:
  case 0x14:
  case 0x1C: // NEQ
    return _VFCMPScalarInsert(OpSize::i128Bit, ElementSize, Src1, Src2, FloatCompareOp::NEQ, false);
  case 0x05:
  case 0x0D:
  case 0x15:
  case 0x1D: { // NLT, NGT(Swapped operand)
    Ref Result = _VFCMPLT(ElementSize, ElementSize, Src1, Src2);
    Result = _VNot(ElementSize, ElementSize, Result);
    // Insert the lower bits
    return _VInsElement(OpSize::i128Bit, ElementSize, 0, 0, Src1, Result);
  }
  case 0x06:
  case 0x0E:
  case 0x16:
  case 0x1E: { // NLE, NGE(Swapped operand)
    Ref Result = _VFCMPLE(ElementSize, ElementSize, Src1, Src2);
    Result = _VNot(ElementSize, ElementSize, Result);
    // Insert the lower bits
    return _VInsElement(OpSize::i128Bit, ElementSize, 0, 0, Src1, Result);
  }
  case 0x07:
  case 0x0F:
  case 0x17:
  case 0x1F: // Ordered
    return _VFCMPScalarInsert(OpSize::i128Bit, ElementSize, Src1, Src2, FloatCompareOp::ORD, false);
  default: LOGMAN_MSG_A_FMT("Unknown Comparison type: {}", CompType); break;
  }
  FEX_UNREACHABLE;
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

  LOGMAN_THROW_A_FMT(Op->Src[2].IsLiteral(), "Src[2] needs to be literal");
  const uint8_t CompType = Op->Src[2].Data.Literal.Value;

  RefPair Result {};
  Result.Low = AVX128_InsertScalarFCMPImpl(ElementSize, Src1.Low, Src2.Low, CompType);
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

  LOGMAN_THROW_A_FMT(Op->Src[1].IsLiteral(), "Src1 needs to be literal here");
  uint64_t Index = Op->Src[1].Data.Literal.Value;

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

} // namespace FEXCore::IR
