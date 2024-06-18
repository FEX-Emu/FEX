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

    // TODO: {OPD(1, 0b10, 0x2A), 1, &OpDispatchBuilder::AVXInsertCVTGPR_To_FPR<4>},
    // TODO: {OPD(1, 0b11, 0x2A), 1, &OpDispatchBuilder::AVXInsertCVTGPR_To_FPR<8>},

    {OPD(1, 0b00, 0x2B), 1, &OpDispatchBuilder::AVX128_MOVVectorNT},
    {OPD(1, 0b01, 0x2B), 1, &OpDispatchBuilder::AVX128_MOVVectorNT},

    // TODO: {OPD(1, 0b10, 0x2C), 1, &OpDispatchBuilder::CVTFPR_To_GPR<4, false>},
    // TODO: {OPD(1, 0b11, 0x2C), 1, &OpDispatchBuilder::CVTFPR_To_GPR<8, false>},

    // TODO: {OPD(1, 0b10, 0x2D), 1, &OpDispatchBuilder::CVTFPR_To_GPR<4, true>},
    // TODO: {OPD(1, 0b11, 0x2D), 1, &OpDispatchBuilder::CVTFPR_To_GPR<8, true>},

    // TODO: {OPD(1, 0b00, 0x2E), 1, &OpDispatchBuilder::UCOMISxOp<4>},
    // TODO: {OPD(1, 0b01, 0x2E), 1, &OpDispatchBuilder::UCOMISxOp<8>},
    // TODO: {OPD(1, 0b00, 0x2F), 1, &OpDispatchBuilder::UCOMISxOp<4>},
    // TODO: {OPD(1, 0b01, 0x2F), 1, &OpDispatchBuilder::UCOMISxOp<8>},

    // TODO: {OPD(1, 0b00, 0x50), 1, &OpDispatchBuilder::MOVMSKOp<4>},
    // TODO: {OPD(1, 0b01, 0x50), 1, &OpDispatchBuilder::MOVMSKOp<8>},

    {OPD(1, 0b00, 0x51), 1, &OpDispatchBuilder::AVX128_VectorUnary<IR::OP_VFSQRT, 4>},
    {OPD(1, 0b01, 0x51), 1, &OpDispatchBuilder::AVX128_VectorUnary<IR::OP_VFSQRT, 8>},
    // TODO: {OPD(1, 0b10, 0x51), 1, &OpDispatchBuilder::AVXVectorScalarUnaryInsertALUOp<IR::OP_VFSQRTSCALARINSERT, 4>},
    // TODO: {OPD(1, 0b11, 0x51), 1, &OpDispatchBuilder::AVXVectorScalarUnaryInsertALUOp<IR::OP_VFSQRTSCALARINSERT, 8>},

    {OPD(1, 0b00, 0x52), 1, &OpDispatchBuilder::AVX128_VectorUnary<IR::OP_VFRSQRT, 4>},
    // TODO: {OPD(1, 0b10, 0x52), 1, &OpDispatchBuilder::AVXVectorScalarUnaryInsertALUOp<IR::OP_VFRSQRTSCALARINSERT, 4>},

    {OPD(1, 0b00, 0x53), 1, &OpDispatchBuilder::AVX128_VectorUnary<IR::OP_VFRECP, 4>},
    // TODO: {OPD(1, 0b10, 0x53), 1, &OpDispatchBuilder::AVXVectorScalarUnaryInsertALUOp<IR::OP_VFRECPSCALARINSERT, 4>},

    {OPD(1, 0b00, 0x54), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VAND, 16>},
    {OPD(1, 0b01, 0x54), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VAND, 16>},

    // TODO: {OPD(1, 0b00, 0x55), 1, &OpDispatchBuilder::VANDNOp},
    // TODO: {OPD(1, 0b01, 0x55), 1, &OpDispatchBuilder::VANDNOp},

    {OPD(1, 0b00, 0x56), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VOR, 16>},
    {OPD(1, 0b01, 0x56), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VOR, 16>},

    {OPD(1, 0b00, 0x57), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VXOR, 16>},
    {OPD(1, 0b01, 0x57), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VXOR, 16>},

    {OPD(1, 0b00, 0x58), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VFADD, 4>},
    {OPD(1, 0b01, 0x58), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VFADD, 8>},
    // TODO: {OPD(1, 0b10, 0x58), 1, &OpDispatchBuilder::AVXVectorScalarInsertALUOp<IR::OP_VFADDSCALARINSERT, 4>},
    // TODO: {OPD(1, 0b11, 0x58), 1, &OpDispatchBuilder::AVXVectorScalarInsertALUOp<IR::OP_VFADDSCALARINSERT, 8>},

    {OPD(1, 0b00, 0x59), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VFMUL, 4>},
    {OPD(1, 0b01, 0x59), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VFMUL, 8>},
    // TODO: {OPD(1, 0b10, 0x59), 1, &OpDispatchBuilder::AVXVectorScalarInsertALUOp<IR::OP_VFMULSCALARINSERT, 4>},
    // TODO: {OPD(1, 0b11, 0x59), 1, &OpDispatchBuilder::AVXVectorScalarInsertALUOp<IR::OP_VFMULSCALARINSERT, 8>},

    // TODO: {OPD(1, 0b00, 0x5A), 1, &OpDispatchBuilder::AVXVector_CVT_Float_To_Float<8, 4>},
    // TODO: {OPD(1, 0b01, 0x5A), 1, &OpDispatchBuilder::AVXVector_CVT_Float_To_Float<4, 8>},
    // TODO: {OPD(1, 0b10, 0x5A), 1, &OpDispatchBuilder::AVXInsertScalar_CVT_Float_To_Float<8, 4>},
    // TODO: {OPD(1, 0b11, 0x5A), 1, &OpDispatchBuilder::AVXInsertScalar_CVT_Float_To_Float<4, 8>},

    // TODO: {OPD(1, 0b00, 0x5B), 1, &OpDispatchBuilder::AVXVector_CVT_Int_To_Float<4, false>},
    // TODO: {OPD(1, 0b01, 0x5B), 1, &OpDispatchBuilder::AVXVector_CVT_Float_To_Int<4, false, true>},
    // TODO: {OPD(1, 0b10, 0x5B), 1, &OpDispatchBuilder::AVXVector_CVT_Float_To_Int<4, false, false>},

    {OPD(1, 0b00, 0x5C), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VFSUB, 4>},
    {OPD(1, 0b01, 0x5C), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VFSUB, 8>},
    // TODO: {OPD(1, 0b10, 0x5C), 1, &OpDispatchBuilder::AVXVectorScalarInsertALUOp<IR::OP_VFSUBSCALARINSERT, 4>},
    // TODO: {OPD(1, 0b11, 0x5C), 1, &OpDispatchBuilder::AVXVectorScalarInsertALUOp<IR::OP_VFSUBSCALARINSERT, 8>},

    {OPD(1, 0b00, 0x5D), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VFMIN, 4>},
    {OPD(1, 0b01, 0x5D), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VFMIN, 8>},
    // TODO: {OPD(1, 0b10, 0x5D), 1, &OpDispatchBuilder::AVXVectorScalarInsertALUOp<IR::OP_VFMINSCALARINSERT, 4>},
    // TODO: {OPD(1, 0b11, 0x5D), 1, &OpDispatchBuilder::AVXVectorScalarInsertALUOp<IR::OP_VFMINSCALARINSERT, 8>},

    {OPD(1, 0b00, 0x5E), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VFDIV, 4>},
    {OPD(1, 0b01, 0x5E), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VFDIV, 8>},
    // TODO: {OPD(1, 0b10, 0x5E), 1, &OpDispatchBuilder::AVXVectorScalarInsertALUOp<IR::OP_VFDIVSCALARINSERT, 4>},
    // TODO: {OPD(1, 0b11, 0x5E), 1, &OpDispatchBuilder::AVXVectorScalarInsertALUOp<IR::OP_VFDIVSCALARINSERT, 8>},

    {OPD(1, 0b00, 0x5F), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VFMAX, 4>},
    {OPD(1, 0b01, 0x5F), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VFMAX, 8>},
    // TODO: {OPD(1, 0b10, 0x5F), 1, &OpDispatchBuilder::AVXVectorScalarInsertALUOp<IR::OP_VFMAXSCALARINSERT, 4>},
    // TODO: {OPD(1, 0b11, 0x5F), 1, &OpDispatchBuilder::AVXVectorScalarInsertALUOp<IR::OP_VFMAXSCALARINSERT, 8>},

    {OPD(1, 0b01, 0x60), 1, &OpDispatchBuilder::AVX128_VPUNPCKL<1>},
    {OPD(1, 0b01, 0x61), 1, &OpDispatchBuilder::AVX128_VPUNPCKL<2>},
    {OPD(1, 0b01, 0x62), 1, &OpDispatchBuilder::AVX128_VPUNPCKL<4>},
    // TODO: {OPD(1, 0b01, 0x63), 1, &OpDispatchBuilder::VPACKSSOp<2>},
    {OPD(1, 0b01, 0x64), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VCMPGT, 1>},
    {OPD(1, 0b01, 0x65), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VCMPGT, 2>},
    {OPD(1, 0b01, 0x66), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VCMPGT, 4>},
    // TODO: {OPD(1, 0b01, 0x67), 1, &OpDispatchBuilder::VPACKUSOp<2>},
    {OPD(1, 0b01, 0x68), 1, &OpDispatchBuilder::AVX128_VPUNPCKH<1>},
    {OPD(1, 0b01, 0x69), 1, &OpDispatchBuilder::AVX128_VPUNPCKH<2>},
    {OPD(1, 0b01, 0x6A), 1, &OpDispatchBuilder::AVX128_VPUNPCKH<4>},
    // TODO: {OPD(1, 0b01, 0x6B), 1, &OpDispatchBuilder::VPACKSSOp<4>},
    {OPD(1, 0b01, 0x6C), 1, &OpDispatchBuilder::AVX128_VPUNPCKL<8>},
    {OPD(1, 0b01, 0x6D), 1, &OpDispatchBuilder::AVX128_VPUNPCKH<8>},
    // TODO: {OPD(1, 0b01, 0x6E), 1, &OpDispatchBuilder::MOVBetweenGPR_FPR},

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

    // TODO: {OPD(1, 0b01, 0x7E), 1, &OpDispatchBuilder::MOVBetweenGPR_FPR},
    {OPD(1, 0b10, 0x7E), 1, &OpDispatchBuilder::AVX128_MOVQ},

    {OPD(1, 0b01, 0x7F), 1, &OpDispatchBuilder::AVX128_VMOVAPS},
    {OPD(1, 0b10, 0x7F), 1, &OpDispatchBuilder::AVX128_VMOVAPS},

    // TODO: {OPD(1, 0b00, 0xC2), 1, &OpDispatchBuilder::AVXVFCMPOp<4>},
    // TODO: {OPD(1, 0b01, 0xC2), 1, &OpDispatchBuilder::AVXVFCMPOp<8>},
    // TODO: {OPD(1, 0b10, 0xC2), 1, &OpDispatchBuilder::AVXInsertScalarFCMPOp<4>},
    // TODO: {OPD(1, 0b11, 0xC2), 1, &OpDispatchBuilder::AVXInsertScalarFCMPOp<8>},

    // TODO: {OPD(1, 0b01, 0xC4), 1, &OpDispatchBuilder::VPINSRWOp},
    // TODO: {OPD(1, 0b01, 0xC5), 1, &OpDispatchBuilder::PExtrOp<2>},

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
    // TODO: {OPD(1, 0b01, 0xD7), 1, &OpDispatchBuilder::MOVMSKOpOne},

    {OPD(1, 0b01, 0xD8), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VUQSUB, 1>},
    {OPD(1, 0b01, 0xD9), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VUQSUB, 2>},
    {OPD(1, 0b01, 0xDA), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VUMIN, 1>},
    {OPD(1, 0b01, 0xDB), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VAND, 16>},
    {OPD(1, 0b01, 0xDC), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VUQADD, 1>},
    {OPD(1, 0b01, 0xDD), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VUQADD, 2>},
    {OPD(1, 0b01, 0xDE), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VUMAX, 1>},
    // TODO: {OPD(1, 0b01, 0xDF), 1, &OpDispatchBuilder::VANDNOp},

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

    // TODO: {OPD(1, 0b11, 0xF0), 1, &OpDispatchBuilder::MOVVectorUnalignedOp},
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

    // TODO: {OPD(2, 0b01, 0x08), 1, &OpDispatchBuilder::VPSIGN<1>},
    // TODO: {OPD(2, 0b01, 0x09), 1, &OpDispatchBuilder::VPSIGN<2>},
    // TODO: {OPD(2, 0b01, 0x0A), 1, &OpDispatchBuilder::VPSIGN<4>},
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

    // TODO: {OPD(2, 0b01, 0x20), 1, &OpDispatchBuilder::ExtendVectorElements<1, 2, true>},
    // TODO: {OPD(2, 0b01, 0x21), 1, &OpDispatchBuilder::ExtendVectorElements<1, 4, true>},
    // TODO: {OPD(2, 0b01, 0x22), 1, &OpDispatchBuilder::ExtendVectorElements<1, 8, true>},
    // TODO: {OPD(2, 0b01, 0x23), 1, &OpDispatchBuilder::ExtendVectorElements<2, 4, true>},
    // TODO: {OPD(2, 0b01, 0x24), 1, &OpDispatchBuilder::ExtendVectorElements<2, 8, true>},
    // TODO: {OPD(2, 0b01, 0x25), 1, &OpDispatchBuilder::ExtendVectorElements<4, 8, true>},

    // TODO: {OPD(2, 0b01, 0x28), 1, &OpDispatchBuilder::VPMULLOp<4, true>},
    {OPD(2, 0b01, 0x29), 1, &OpDispatchBuilder::AVX128_VectorALU<IR::OP_VCMPEQ, 8>},
    {OPD(2, 0b01, 0x2A), 1, &OpDispatchBuilder::AVX128_MOVVectorNT},
    // TODO: {OPD(2, 0b01, 0x2B), 1, &OpDispatchBuilder::VPACKUSOp<4>},
    // TODO: {OPD(2, 0b01, 0x2C), 1, &OpDispatchBuilder::VMASKMOVOp<4, false>},
    // TODO: {OPD(2, 0b01, 0x2D), 1, &OpDispatchBuilder::VMASKMOVOp<8, false>},
    // TODO: {OPD(2, 0b01, 0x2E), 1, &OpDispatchBuilder::VMASKMOVOp<4, true>},
    // TODO: {OPD(2, 0b01, 0x2F), 1, &OpDispatchBuilder::VMASKMOVOp<8, true>},

    // TODO: {OPD(2, 0b01, 0x30), 1, &OpDispatchBuilder::ExtendVectorElements<1, 2, false>},
    // TODO: {OPD(2, 0b01, 0x31), 1, &OpDispatchBuilder::ExtendVectorElements<1, 4, false>},
    // TODO: {OPD(2, 0b01, 0x32), 1, &OpDispatchBuilder::ExtendVectorElements<1, 8, false>},
    // TODO: {OPD(2, 0b01, 0x33), 1, &OpDispatchBuilder::ExtendVectorElements<2, 4, false>},
    // TODO: {OPD(2, 0b01, 0x34), 1, &OpDispatchBuilder::ExtendVectorElements<2, 8, false>},
    // TODO: {OPD(2, 0b01, 0x35), 1, &OpDispatchBuilder::ExtendVectorElements<4, 8, false>},
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

    // TODO: {OPD(3, 0b01, 0x14), 1, &OpDispatchBuilder::PExtrOp<1>},
    // TODO: {OPD(3, 0b01, 0x15), 1, &OpDispatchBuilder::PExtrOp<2>},
    // TODO: {OPD(3, 0b01, 0x16), 1, &OpDispatchBuilder::PExtrOp<4>},
    // TODO: {OPD(3, 0b01, 0x17), 1, &OpDispatchBuilder::PExtrOp<4>},

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

} // namespace FEXCore::IR
