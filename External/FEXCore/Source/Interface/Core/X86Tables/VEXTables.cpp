#include "Interface/Core/X86Tables/X86Tables.h"

namespace FEXCore::X86Tables {
using namespace InstFlags;

void InitializeVEXTables() {
#define OPD(map_select, pp, opcode) (((map_select - 1) << 10) | (pp << 8) | (opcode))
  const U16U8InfoStruct VEXTable[] = {
    // Map 0 (Reserved)
    // VEX Map 1
    {OPD(1, 0b00, 0x10), 1, X86InstInfo{"VMOVUPS",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0x10), 1, X86InstInfo{"VMODUPD",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b10, 0x10), 1, X86InstInfo{"VMOVSS",    TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b11, 0x10), 1, X86InstInfo{"VMOVSD",    TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(1, 0b00, 0x11), 1, X86InstInfo{"VMOVUPS",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0x11), 1, X86InstInfo{"VMODUPD",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b10, 0x11), 1, X86InstInfo{"VMOVSS",    TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b11, 0x11), 1, X86InstInfo{"VMOVSD",    TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(1, 0b00, 0x12), 1, X86InstInfo{"VMOVLPS",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0x12), 1, X86InstInfo{"VMOVLPD",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b10, 0x12), 1, X86InstInfo{"VMOVSLDUP", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b11, 0x12), 1, X86InstInfo{"VMOVDDUP",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(1, 0b00, 0x13), 1, X86InstInfo{"VMOVLPS",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0x13), 1, X86InstInfo{"VMOVLPD",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(1, 0b00, 0x14), 1, X86InstInfo{"VUNPCKLPS", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0x14), 1, X86InstInfo{"VUNPCKLPD", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(1, 0b00, 0x15), 1, X86InstInfo{"VUNPCKHPS", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0x15), 1, X86InstInfo{"VUNPCKHPD", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(1, 0b00, 0x16), 1, X86InstInfo{"VMOVHPS",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0x16), 1, X86InstInfo{"VMOVHPD",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b10, 0x16), 1, X86InstInfo{"VMOVSHDUP", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(1, 0b00, 0x17), 1, X86InstInfo{"VMOVHPS",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0x17), 1, X86InstInfo{"VMOVHPD",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(1, 0b00, 0x50), 1, X86InstInfo{"VMOVMSKPS", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0x50), 1, X86InstInfo{"VMOVMSKPD", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(1, 0b00, 0x51), 1, X86InstInfo{"VSQRTPS",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0x51), 1, X86InstInfo{"VSQRTPD",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b10, 0x51), 1, X86InstInfo{"VSQRTSS",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b11, 0x51), 1, X86InstInfo{"VSQRTSD",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(1, 0b00, 0x52), 1, X86InstInfo{"VRSQRTPS",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b10, 0x52), 1, X86InstInfo{"VRSQRTSS",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(1, 0b00, 0x53), 1, X86InstInfo{"VRCPPS",    TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b10, 0x53), 1, X86InstInfo{"VRCPSS",    TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(1, 0b00, 0x54), 1, X86InstInfo{"VANDPS",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0x54), 1, X86InstInfo{"VANDPD",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(1, 0b00, 0x55), 1, X86InstInfo{"VANDNPS",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0x55), 1, X86InstInfo{"VANDNPD",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(1, 0b00, 0x56), 1, X86InstInfo{"VORPS",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0x56), 1, X86InstInfo{"VORPD",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(1, 0b00, 0x57), 1, X86InstInfo{"VXORPS",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0x57), 1, X86InstInfo{"VDORPD",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(1, 0b01, 0x60), 1, X86InstInfo{"VPUNPCKLBW", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0x61), 1, X86InstInfo{"VPUNPCKLWD", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0x62), 1, X86InstInfo{"VPUNPCKLDQ", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0x63), 1, X86InstInfo{"VPACKSSWB",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0x64), 1, X86InstInfo{"VPCMPGTB",   TYPE_INST, FLAGS_MODRM | FLAGS_XMM_FLAGS, 0, nullptr}},
    {OPD(1, 0b01, 0x65), 1, X86InstInfo{"VPVMPGTW",   TYPE_INST, FLAGS_MODRM | FLAGS_XMM_FLAGS, 0, nullptr}},
    {OPD(1, 0b01, 0x66), 1, X86InstInfo{"VPVMPGTD",   TYPE_INST, FLAGS_MODRM | FLAGS_XMM_FLAGS, 0, nullptr}},
    {OPD(1, 0b01, 0x67), 1, X86InstInfo{"VPACKUSWB",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(1, 0b01, 0x70), 1, X86InstInfo{"VPSHUFD",    TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b10, 0x70), 1, X86InstInfo{"VPSHUFHW",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b11, 0x70), 1, X86InstInfo{"VPSHUFLW",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(1, 0b01, 0x71), 1, X86InstInfo{"",           TYPE_VEX_GROUP_12, FLAGS_NONE, 0, nullptr}}, // VEX Group 12
    {OPD(1, 0b01, 0x72), 1, X86InstInfo{"",           TYPE_VEX_GROUP_13, FLAGS_NONE, 0, nullptr}}, // VEX Group 13
    {OPD(1, 0b01, 0x73), 1, X86InstInfo{"",           TYPE_VEX_GROUP_14, FLAGS_NONE, 0, nullptr}}, // VEX Group 14

    {OPD(1, 0b01, 0x74), 1, X86InstInfo{"VPCMPEQB",   TYPE_INST, FLAGS_MODRM | FLAGS_XMM_FLAGS, 0, nullptr}},
    {OPD(1, 0b01, 0x75), 1, X86InstInfo{"VPCMPEQW",   TYPE_INST, FLAGS_MODRM | FLAGS_XMM_FLAGS, 0, nullptr}},
    {OPD(1, 0b01, 0x76), 1, X86InstInfo{"VPCMPEQD",   TYPE_INST, FLAGS_MODRM | FLAGS_XMM_FLAGS, 0, nullptr}},

    {OPD(1, 0b00, 0x77), 1, X86InstInfo{"VZERO*",     TYPE_INST, FLAGS_NONE, 0, nullptr}},

    {OPD(1, 0b00, 0xC2), 1, X86InstInfo{"VCMPccPS",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0xC2), 1, X86InstInfo{"VCMPccPD",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b10, 0xC2), 1, X86InstInfo{"VCMPccSS",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b11, 0xC2), 1, X86InstInfo{"VCMPccSD",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(1, 0b01, 0xC4), 1, X86InstInfo{"VPINSRW",    TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0xC5), 1, X86InstInfo{"VPEXTRW",    TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(1, 0b00, 0xC6), 1, X86InstInfo{"VSHUFPS",    TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0xC6), 1, X86InstInfo{"VSHUFPD",    TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    // The above ops are defined from `Table A-17. VEX Opcode Map 1, Low Nibble = [0h:7h]` of AMD Architecture programmer's manual Volume 3
    // This table doesn't state which VEX.pp is for which instruction
    // XXX: Confirm all the above encoding opcodes

    {OPD(1, 0b00, 0x28), 1, X86InstInfo{"VMOVAPS",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0x28), 1, X86InstInfo{"VMOVAPD",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(1, 0b00, 0x29), 1, X86InstInfo{"VMOVAPS",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0x29), 1, X86InstInfo{"VMOVAPD",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(1, 0b10, 0x2A), 1, X86InstInfo{"VCVTSI2SS",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b11, 0x2A), 1, X86InstInfo{"VCVTSI2SD",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(1, 0b00, 0x2B), 1, X86InstInfo{"VMOVNTPS",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0x2B), 1, X86InstInfo{"VMOVNTPD",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(1, 0b10, 0x2C), 1, X86InstInfo{"VCVTTSS2SI",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b11, 0x2C), 1, X86InstInfo{"VCVTTSD2SI",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(1, 0b10, 0x2D), 1, X86InstInfo{"VCVTSS2SI",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b11, 0x2D), 1, X86InstInfo{"VCVTSD2SI",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(1, 0b00, 0x2E), 1, X86InstInfo{"VUCOMISS",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0x2E), 1, X86InstInfo{"VUCOMISD",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(1, 0b00, 0x2F), 1, X86InstInfo{"VUCOMISS",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0x2F), 1, X86InstInfo{"VUCOMISD",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(1, 0b00, 0x58), 1, X86InstInfo{"VADDPS",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0x58), 1, X86InstInfo{"VADDPD",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b10, 0x58), 1, X86InstInfo{"VADDSS",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b11, 0x58), 1, X86InstInfo{"VADDSD",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(1, 0b00, 0x59), 1, X86InstInfo{"VMULPS",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0x59), 1, X86InstInfo{"VMULPD",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b10, 0x59), 1, X86InstInfo{"VMULSS",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b11, 0x59), 1, X86InstInfo{"VMULSD",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(1, 0b00, 0x5B), 1, X86InstInfo{"VCVTDQ2PS",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0x5B), 1, X86InstInfo{"VCVTPS2DQ",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b10, 0x5B), 1, X86InstInfo{"VCVTPS2DQ",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(1, 0b00, 0x5C), 1, X86InstInfo{"VSUBPS",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0x5C), 1, X86InstInfo{"VSUBPD",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b10, 0x5C), 1, X86InstInfo{"VSUBSS",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b11, 0x5C), 1, X86InstInfo{"VSUBSD",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(1, 0b00, 0x5D), 1, X86InstInfo{"VMINPS",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0x5D), 1, X86InstInfo{"VMINPD",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b10, 0x5D), 1, X86InstInfo{"VMINSS",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b11, 0x5D), 1, X86InstInfo{"VMINSD",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(1, 0b00, 0x5E), 1, X86InstInfo{"VDIVPS",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0x5E), 1, X86InstInfo{"VDIVPD",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b10, 0x5E), 1, X86InstInfo{"VDIVSS",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b11, 0x5E), 1, X86InstInfo{"VDIVSD",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(1, 0b00, 0x5F), 1, X86InstInfo{"VMAXPS",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0x5F), 1, X86InstInfo{"VMAXPD",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b10, 0x5F), 1, X86InstInfo{"VMAXSS",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b11, 0x5F), 1, X86InstInfo{"VMAXSD",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},


    {OPD(1, 0b01, 0x68), 1, X86InstInfo{"VPUNPCKHBW",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0x69), 1, X86InstInfo{"VPUNPCKHWD",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0x6A), 1, X86InstInfo{"VPUNPCKHDQ",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0x6B), 1, X86InstInfo{"VPACKSSDW",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0x6C), 1, X86InstInfo{"VPUNPCKLQDQ", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0x6D), 1, X86InstInfo{"VPUNPCKHQDQ", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0x6E), 1, X86InstInfo{"VMOV*",       TYPE_INST, GenFlagsDstSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_SRC_GPR, 0, nullptr}},

    {OPD(1, 0b01, 0x6F), 1, X86InstInfo{"VMOVDQA",     TYPE_INST, FLAGS_MODRM | FLAGS_XMM_FLAGS, 0, nullptr}},
    {OPD(1, 0b10, 0x6F), 1, X86InstInfo{"VMOVDQU",     TYPE_INST, FLAGS_MODRM | FLAGS_XMM_FLAGS, 0, nullptr}},

    {OPD(1, 0b01, 0x7C), 1, X86InstInfo{"VHADDPD",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b11, 0x7C), 1, X86InstInfo{"VHADDPS",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(1, 0b01, 0x7D), 1, X86InstInfo{"VHSUBPD",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b11, 0x7D), 1, X86InstInfo{"VHSUBPS",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(1, 0b01, 0x7E), 1, X86InstInfo{"VMOV*",     TYPE_INST, FLAGS_MODRM | FLAGS_XMM_FLAGS, 0, nullptr}},
    {OPD(1, 0b10, 0x7E), 1, X86InstInfo{"VMOVQ",     TYPE_INST, FLAGS_MODRM | FLAGS_XMM_FLAGS, 0, nullptr}},

    {OPD(1, 0b01, 0x7F), 1, X86InstInfo{"VMOVDQA",     TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS, 0, nullptr}},
    {OPD(1, 0b10, 0x7F), 1, X86InstInfo{"VMOVDQU",     TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS, 0, nullptr}},

    {OPD(1, 0b00, 0xAE), 1, X86InstInfo{"",     TYPE_VEX_GROUP_15, FLAGS_NONE, 0, nullptr}}, // VEX Group 15
    {OPD(1, 0b01, 0xAE), 1, X86InstInfo{"",     TYPE_VEX_GROUP_15, FLAGS_NONE, 0, nullptr}}, // VEX Group 15
    {OPD(1, 0b10, 0xAE), 1, X86InstInfo{"",     TYPE_VEX_GROUP_15, FLAGS_NONE, 0, nullptr}}, // VEX Group 15
    {OPD(1, 0b11, 0xAE), 1, X86InstInfo{"",     TYPE_VEX_GROUP_15, FLAGS_NONE, 0, nullptr}}, // VEX Group 15

    {OPD(1, 0b01, 0xD0), 1, X86InstInfo{"VADDSUBPD",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b11, 0xD0), 1, X86InstInfo{"VADDSUBPS",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(1, 0b01, 0xD1), 1, X86InstInfo{"VPSRLW",      TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0xD2), 1, X86InstInfo{"VPSRLD",      TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0xD3), 1, X86InstInfo{"VPSRLQ",      TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0xD4), 1, X86InstInfo{"VPADDQ",      TYPE_INST, GenFlagsSameSize(SIZE_256BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0, nullptr}},
    {OPD(1, 0b01, 0xD5), 1, X86InstInfo{"VPMULLW",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0xD6), 1, X86InstInfo{"VMOVQ",       TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS, 0, nullptr}},
    {OPD(1, 0b01, 0xD7), 1, X86InstInfo{"VPMOVMSKB",   TYPE_INST, FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_DST_GPR | FLAGS_SF_MOD_REG_ONLY, 0, nullptr}},

    {OPD(1, 0b01, 0xD8), 1, X86InstInfo{"VPSUBUSB", TYPE_INST, FLAGS_MODRM | FLAGS_XMM_FLAGS, 0, nullptr}},
    {OPD(1, 0b01, 0xD9), 1, X86InstInfo{"VPSUBUSW", TYPE_INST, FLAGS_MODRM | FLAGS_XMM_FLAGS, 0, nullptr}},
    {OPD(1, 0b01, 0xDA), 1, X86InstInfo{"VPMINUB",  TYPE_INST, FLAGS_MODRM | FLAGS_XMM_FLAGS, 0, nullptr}},
    {OPD(1, 0b01, 0xDB), 1, X86InstInfo{"VPAND",    TYPE_INST, FLAGS_MODRM | FLAGS_XMM_FLAGS, 0, nullptr}},
    {OPD(1, 0b01, 0xDC), 1, X86InstInfo{"VPADDUSB", TYPE_INST, FLAGS_MODRM | FLAGS_XMM_FLAGS, 0, nullptr}},
    {OPD(1, 0b01, 0xDD), 1, X86InstInfo{"VPADDUSW", TYPE_INST, FLAGS_MODRM | FLAGS_XMM_FLAGS, 0, nullptr}},
    {OPD(1, 0b01, 0xDE), 1, X86InstInfo{"VPMAXUB",  TYPE_INST, FLAGS_MODRM | FLAGS_XMM_FLAGS, 0, nullptr}},
    {OPD(1, 0b01, 0xDF), 1, X86InstInfo{"VPANDN",   TYPE_INST, FLAGS_MODRM | FLAGS_XMM_FLAGS, 0, nullptr}},

    {OPD(1, 0b01, 0xE0), 1, X86InstInfo{"VPAVGB",      TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0xE1), 1, X86InstInfo{"VPSRAW",      TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0xE2), 1, X86InstInfo{"VPSRAD",      TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0xE3), 1, X86InstInfo{"VPAVGW",      TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0xE4), 1, X86InstInfo{"VPMULHUW",    TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0xE5), 1, X86InstInfo{"VPMULHW",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(1, 0b01, 0xE6), 1, X86InstInfo{"VCVTTPD2DQ",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b10, 0xE6), 1, X86InstInfo{"VCVTDQ2PD",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b11, 0xE6), 1, X86InstInfo{"VCVTPD2DQ",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(1, 0b01, 0xE7), 1, X86InstInfo{"VMOVNTDQ",    TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS, 0, nullptr}},

    {OPD(1, 0b01, 0xE8), 1, X86InstInfo{"VPSUBSB", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0xE9), 1, X86InstInfo{"VPSUBSW", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0xEA), 1, X86InstInfo{"VPMINSW",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0xEB), 1, X86InstInfo{"VPOR",    TYPE_INST, FLAGS_MODRM | FLAGS_XMM_FLAGS, 0, nullptr}},
    {OPD(1, 0b01, 0xEC), 1, X86InstInfo{"VPADDSB", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0xED), 1, X86InstInfo{"VPADDSW", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0xEE), 1, X86InstInfo{"VPMAXSW",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0xEF), 1, X86InstInfo{"VPXOR",   TYPE_INST, FLAGS_MODRM | FLAGS_XMM_FLAGS, 0, nullptr}},

    {OPD(1, 0b11, 0xF0), 1, X86InstInfo{"VLDDQU",      TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(1, 0b01, 0xF1), 1, X86InstInfo{"VPSLLW",      TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0xF2), 1, X86InstInfo{"VPSLLD",      TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0xF3), 1, X86InstInfo{"VPSLLQ",      TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0xF4), 1, X86InstInfo{"VPMULUDQ",    TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0xF5), 1, X86InstInfo{"VPMADDWD",    TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0xF6), 1, X86InstInfo{"VPSADBW",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0xF7), 1, X86InstInfo{"VMASKMOVDQU", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(1, 0b01, 0xF8), 1, X86InstInfo{"VPSUBB", TYPE_INST, GenFlagsSameSize(SIZE_256BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0, nullptr}},
    {OPD(1, 0b01, 0xF9), 1, X86InstInfo{"VPSUBW", TYPE_INST, GenFlagsSameSize(SIZE_256BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0, nullptr}},
    {OPD(1, 0b01, 0xFA), 1, X86InstInfo{"VPSUBD", TYPE_INST, GenFlagsSameSize(SIZE_256BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0, nullptr}},
    {OPD(1, 0b01, 0xFB), 1, X86InstInfo{"VPSUBQ", TYPE_INST, GenFlagsSameSize(SIZE_256BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0, nullptr}},
    {OPD(1, 0b01, 0xFC), 1, X86InstInfo{"VPADDB", TYPE_INST, GenFlagsSameSize(SIZE_256BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0, nullptr}},
    {OPD(1, 0b01, 0xFD), 1, X86InstInfo{"VPADDW", TYPE_INST, GenFlagsSameSize(SIZE_256BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0, nullptr}},
    {OPD(1, 0b01, 0xFE), 1, X86InstInfo{"VPADDD", TYPE_INST, GenFlagsSameSize(SIZE_256BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0, nullptr}},

    // VEX Map 2
    {OPD(2, 0b01, 0x00), 1, X86InstInfo{"VPSHUFB", TYPE_INST, FLAGS_MODRM | FLAGS_XMM_FLAGS, 0, nullptr}},
    {OPD(2, 0b01, 0x01), 1, X86InstInfo{"VPADDW", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x02), 1, X86InstInfo{"VPHADDD", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x03), 1, X86InstInfo{"VPHADDSW", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x04), 1, X86InstInfo{"VPMADDUBSW", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x05), 1, X86InstInfo{"VPHSUBW", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x06), 1, X86InstInfo{"VPHSUBD", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x07), 1, X86InstInfo{"VPHSUBSW", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(2, 0b01, 0x08), 1, X86InstInfo{"VPSIGNB", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x09), 1, X86InstInfo{"VPSIGNW", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x0A), 1, X86InstInfo{"VPSIGND", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x0B), 1, X86InstInfo{"VPMULHRSW", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x0C), 1, X86InstInfo{"VPERMILPS", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x0D), 1, X86InstInfo{"VPERMILPD", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x0E), 1, X86InstInfo{"VTESTPS", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x0F), 1, X86InstInfo{"VTESTPD", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(2, 0b01, 0x13), 1, X86InstInfo{"VCVTPH2PS", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x16), 1, X86InstInfo{"VPERMPS", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x17), 1, X86InstInfo{"VPTEST", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(2, 0b01, 0x18), 1, X86InstInfo{"VBROADCASTSS", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x19), 1, X86InstInfo{"VBROADCASTSD", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x1A), 1, X86InstInfo{"VBROADCASTF128", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x1C), 1, X86InstInfo{"VPABSB", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x1D), 1, X86InstInfo{"VPABSW", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x1E), 1, X86InstInfo{"VPABSD", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(2, 0b01, 0x20), 1, X86InstInfo{"VPMOVSXBW", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x21), 1, X86InstInfo{"VPMOVSXBD", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x22), 1, X86InstInfo{"VPMOVSXBQ", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x23), 1, X86InstInfo{"VPMOVSXWD", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x24), 1, X86InstInfo{"VPMOVSXWQ", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x25), 1, X86InstInfo{"VPMOVSXDQ", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(2, 0b01, 0x28), 1, X86InstInfo{"VPMULDQ", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x29), 1, X86InstInfo{"VPCMPEQQ", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x2A), 1, X86InstInfo{"VMOVNTDQA", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x2B), 1, X86InstInfo{"VPACKUSDW", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x2C), 1, X86InstInfo{"VMASKMOVPS", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x2D), 1, X86InstInfo{"VMASKMOVPD", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x2E), 1, X86InstInfo{"VMASKMOVPS", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x2F), 1, X86InstInfo{"VMASKMOVPD", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(2, 0b01, 0x30), 1, X86InstInfo{"VPMOVZXBW", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x31), 1, X86InstInfo{"VPMOVZXBD", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x32), 1, X86InstInfo{"VPMOVZXBQ", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x33), 1, X86InstInfo{"VPMOVZXWD", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x34), 1, X86InstInfo{"VPMOVZXWQ", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x35), 1, X86InstInfo{"VPMOVZXDQ", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x36), 1, X86InstInfo{"VPERMD", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x37), 1, X86InstInfo{"VPVMPGTQ", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(2, 0b01, 0x38), 1, X86InstInfo{"VPMINSB", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x39), 1, X86InstInfo{"VPMINSD", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x3A), 1, X86InstInfo{"VPMINUW", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x3B), 1, X86InstInfo{"VPMINUD", TYPE_INST, FLAGS_MODRM | FLAGS_XMM_FLAGS, 0, nullptr}},
    {OPD(2, 0b01, 0x3C), 1, X86InstInfo{"VPMAXSB", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x3D), 1, X86InstInfo{"VPMAXSD", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x3E), 1, X86InstInfo{"VPMAXUW", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x3F), 1, X86InstInfo{"VPMAXUD", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(2, 0b01, 0x40), 1, X86InstInfo{"VPMULLD", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x41), 1, X86InstInfo{"VPHMINPOSUW", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x45), 1, X86InstInfo{"VPSRLV", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x46), 1, X86InstInfo{"VPSRAVD", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x47), 1, X86InstInfo{"VPSLLV", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(2, 0b01, 0x58), 1, X86InstInfo{"VPBROADCASTD", TYPE_INST, FLAGS_MODRM, 0, nullptr}},
    {OPD(2, 0b01, 0x59), 1, X86InstInfo{"VPBROADCASTQ", TYPE_INST, FLAGS_MODRM, 0, nullptr}},
    {OPD(2, 0b01, 0x5A), 1, X86InstInfo{"VBBROADCASTI128", TYPE_INST, FLAGS_MODRM, 0, nullptr}},

    {OPD(2, 0b01, 0x78), 1, X86InstInfo{"VPBROADCASTB", TYPE_INST, FLAGS_MODRM, 0, nullptr}},
    {OPD(2, 0b01, 0x79), 1, X86InstInfo{"VPBROADCASTW", TYPE_INST, FLAGS_MODRM, 0, nullptr}},

    {OPD(2, 0b01, 0x8C), 1, X86InstInfo{"VPMASKMOV", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x8E), 1, X86InstInfo{"VPMASKMOV", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(2, 0b01, 0x90), 1, X86InstInfo{"VPGATHERD", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x91), 1, X86InstInfo{"VPGATHERQ", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x92), 1, X86InstInfo{"VPGATHERD", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x93), 1, X86InstInfo{"VPGATHERQ", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(2, 0b01, 0x96), 1, X86InstInfo{"VFMADDSUB132", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x97), 1, X86InstInfo{"VFMSUBADD132", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(2, 0b01, 0x98), 1, X86InstInfo{"VFMADD132", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x99), 1, X86InstInfo{"VFMADD132", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x9A), 1, X86InstInfo{"VFMSUB132", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x9B), 1, X86InstInfo{"VFMSUB132", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x9C), 1, X86InstInfo{"VFNMADD132", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x9D), 1, X86InstInfo{"VFNMADD132", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x9E), 1, X86InstInfo{"VFNMSUB132", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x9F), 1, X86InstInfo{"VFNMSUB132", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(2, 0b01, 0xA8), 1, X86InstInfo{"VFMADD213", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0xA9), 1, X86InstInfo{"VFMADD213", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0xAA), 1, X86InstInfo{"VFMSUB213", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0xAB), 1, X86InstInfo{"VFMSUB213", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0xAC), 1, X86InstInfo{"VFNMADD213", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0xAD), 1, X86InstInfo{"VFNMADD213", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0xAE), 1, X86InstInfo{"VFNMSUB213", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0xAF), 1, X86InstInfo{"VFNMSUB213", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(2, 0b01, 0xB8), 1, X86InstInfo{"VFMADD231", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0xB9), 1, X86InstInfo{"VFMADD231", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0xBA), 1, X86InstInfo{"VFMSUB231", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0xBB), 1, X86InstInfo{"VFMSUB231", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0xBC), 1, X86InstInfo{"VFNMADD231", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0xBD), 1, X86InstInfo{"VFNMADD231", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0xBE), 1, X86InstInfo{"VFNMSUB231", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0xBF), 1, X86InstInfo{"VFNMSUB231", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(2, 0b01, 0xA6), 1, X86InstInfo{"VFMADDSUB213", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0xA7), 1, X86InstInfo{"VFMSUBADD213", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(2, 0b01, 0xB6), 1, X86InstInfo{"VFMADDSUB231", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0xB7), 1, X86InstInfo{"VFMSUBADD231", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(2, 0b01, 0xDB), 1, X86InstInfo{"VAESIMC", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0xDC), 1, X86InstInfo{"VAESENC", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0xDD), 1, X86InstInfo{"VAESENCLAST", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0xDE), 1, X86InstInfo{"VAESDEC", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0xDF), 1, X86InstInfo{"VAESDECLAST", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(2, 0b00, 0xF2), 1, X86InstInfo{"ANDN", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(2, 0b00, 0xF3), 1, X86InstInfo{"", TYPE_VEX_GROUP_17, FLAGS_NONE, 0, nullptr}}, // VEX Group 17
    {OPD(2, 0b01, 0xF3), 1, X86InstInfo{"", TYPE_VEX_GROUP_17, FLAGS_NONE, 0, nullptr}}, // VEX Group 17
    {OPD(2, 0b10, 0xF3), 1, X86InstInfo{"", TYPE_VEX_GROUP_17, FLAGS_NONE, 0, nullptr}}, // VEX Group 17
    {OPD(2, 0b11, 0xF3), 1, X86InstInfo{"", TYPE_VEX_GROUP_17, FLAGS_NONE, 0, nullptr}}, // VEX Group 17

    {OPD(2, 0b00, 0xF5), 1, X86InstInfo{"BZHI", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0xF5), 1, X86InstInfo{"PEXT", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b11, 0xF5), 1, X86InstInfo{"PDEP", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(2, 0b11, 0xF6), 1, X86InstInfo{"MULX", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(2, 0b00, 0xF7), 1, X86InstInfo{"BEXTR", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0xF7), 1, X86InstInfo{"SHLX", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b10, 0xF7), 1, X86InstInfo{"SARX", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b11, 0xF7), 1, X86InstInfo{"SHRX", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    // VEX Map 3
    {OPD(3, 0b01, 0x00), 1, X86InstInfo{"VPERMQ", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(3, 0b01, 0x01), 1, X86InstInfo{"VPERMPD", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(3, 0b01, 0x02), 1, X86InstInfo{"VPBLENDD", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(3, 0b01, 0x04), 1, X86InstInfo{"VPERMILPS", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(3, 0b01, 0x05), 1, X86InstInfo{"VPERMILPD", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(3, 0b01, 0x06), 1, X86InstInfo{"VPERM2F128", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(3, 0b01, 0x08), 1, X86InstInfo{"VROUNDPS", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(3, 0b01, 0x09), 1, X86InstInfo{"VROUNDPD", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(3, 0b01, 0x0A), 1, X86InstInfo{"VROUNDSS", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(3, 0b01, 0x0B), 1, X86InstInfo{"VROUNDSD", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(3, 0b01, 0x0C), 1, X86InstInfo{"VBLENDPS", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(3, 0b01, 0x0D), 1, X86InstInfo{"VBLENDPD", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(3, 0b01, 0x0E), 1, X86InstInfo{"VBLENDW", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(3, 0b01, 0x0F), 1, X86InstInfo{"VPALIGNR", TYPE_INST, FLAGS_MODRM | FLAGS_XMM_FLAGS, 1, nullptr}},

    {OPD(3, 0b01, 0x14), 1, X86InstInfo{"VPEXTRB", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(3, 0b01, 0x15), 1, X86InstInfo{"VPEXTRW", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(3, 0b01, 0x16), 1, X86InstInfo{"VPEXTRD", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(3, 0b01, 0x17), 1, X86InstInfo{"VEXTRACTPS", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(3, 0b01, 0x18), 1, X86InstInfo{"VINSERTF128", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(3, 0b01, 0x19), 1, X86InstInfo{"VEXTRACTF128", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(3, 0b01, 0x1D), 1, X86InstInfo{"VCVTPS2PH", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(3, 0b01, 0x20), 1, X86InstInfo{"VPINSRB", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(3, 0b01, 0x21), 1, X86InstInfo{"VINSERTPS", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(3, 0b01, 0x22), 1, X86InstInfo{"VPINSRD", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(3, 0b01, 0x38), 1, X86InstInfo{"VINSERTI128", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(3, 0b01, 0x39), 1, X86InstInfo{"VEXTRACTI128", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(3, 0b01, 0x40), 1, X86InstInfo{"VDPPS", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(3, 0b01, 0x41), 1, X86InstInfo{"VDPPD", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(3, 0b01, 0x42), 1, X86InstInfo{"VMPSADBW", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(3, 0b01, 0x44), 1, X86InstInfo{"VPCLMULQDQ", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(3, 0b01, 0x46), 1, X86InstInfo{"VPERM2I128", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(3, 0b01, 0x48), 1, X86InstInfo{"VPERMILzz2PS", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(3, 0b01, 0x49), 1, X86InstInfo{"VPERMILzz2PD", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(3, 0b01, 0x4A), 1, X86InstInfo{"VBLENDVPS", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(3, 0b01, 0x4B), 1, X86InstInfo{"VBLENDVPD", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(3, 0b01, 0x4C), 1, X86InstInfo{"VBLENDVB", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(3, 0b01, 0x5C), 1, X86InstInfo{"VFMADDSUBPS", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(3, 0b01, 0x5D), 1, X86InstInfo{"VFMADDSUBPD", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(3, 0b01, 0x5E), 1, X86InstInfo{"VMFSUBADDPS", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(3, 0b01, 0x5F), 1, X86InstInfo{"VFMSUBADDPD", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(3, 0b01, 0x60), 1, X86InstInfo{"VPCMPESTRM", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(3, 0b01, 0x61), 1, X86InstInfo{"VPCMPESTRI", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(3, 0b01, 0x62), 1, X86InstInfo{"VPCMPISTRM", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(3, 0b01, 0x63), 1, X86InstInfo{"VPCMPISTRI", TYPE_INST, FLAGS_MODRM | FLAGS_XMM_FLAGS, 1, nullptr}},

    {OPD(3, 0b01, 0x68), 1, X86InstInfo{"VFMADDPS", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(3, 0b01, 0x69), 1, X86InstInfo{"VFMADDPD", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(3, 0b01, 0x6A), 1, X86InstInfo{"VFMADDSS", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(3, 0b01, 0x6B), 1, X86InstInfo{"VFMADDSD", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(3, 0b01, 0x6C), 1, X86InstInfo{"VFMSUBPS", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(3, 0b01, 0x6D), 1, X86InstInfo{"VFMSUBPD", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(3, 0b01, 0x6E), 1, X86InstInfo{"VFMSUBSS", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(3, 0b01, 0x6F), 1, X86InstInfo{"VFMSUBSD", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(3, 0b01, 0x78), 1, X86InstInfo{"VFNMADDPS", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(3, 0b01, 0x79), 1, X86InstInfo{"VFNMADDPD", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(3, 0b01, 0x7A), 1, X86InstInfo{"VFNMADDSS", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(3, 0b01, 0x7B), 1, X86InstInfo{"VFNMADDSD", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(3, 0b01, 0x7C), 1, X86InstInfo{"VFNMSUBPS", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(3, 0b01, 0x7D), 1, X86InstInfo{"VFNMSUBPD", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(3, 0b01, 0x7E), 1, X86InstInfo{"VFNMSUBSS", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(3, 0b01, 0x7F), 1, X86InstInfo{"VFNMSUBSD", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(3, 0b01, 0xDF), 1, X86InstInfo{"VAESKEYGENASSIST", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(3, 0b11, 0xF0), 1, X86InstInfo{"RORX", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    // VEX Map 4 - 31 (Reserved)
  };
#undef OPD

#define OPD(group, pp, opcode) (((group - TYPE_VEX_GROUP_12) << 4) | (pp << 3) | (opcode))
  const U8U8InfoStruct VEXGroupTable[] = {
    {OPD(TYPE_VEX_GROUP_12, 1, 0b010), 1, X86InstInfo{"VPSRLW",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(TYPE_VEX_GROUP_12, 1, 0b100), 1, X86InstInfo{"VPSRAW",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(TYPE_VEX_GROUP_12, 1, 0b110), 1, X86InstInfo{"VPSLLW",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(TYPE_VEX_GROUP_13, 1, 0b010), 1, X86InstInfo{"VPSRLD",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(TYPE_VEX_GROUP_13, 1, 0b100), 1, X86InstInfo{"VPSRAD",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(TYPE_VEX_GROUP_13, 1, 0b110), 1, X86InstInfo{"VPSLLD",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(TYPE_VEX_GROUP_14, 1, 0b010), 1, X86InstInfo{"VPSRLQ",   TYPE_INST, FLAGS_MODRM | FLAGS_XMM_FLAGS, 1, nullptr}},
    {OPD(TYPE_VEX_GROUP_14, 1, 0b011), 1, X86InstInfo{"VPSRLDQ",  TYPE_INST, FLAGS_MODRM | FLAGS_XMM_FLAGS, 1, nullptr}},
    {OPD(TYPE_VEX_GROUP_14, 1, 0b110), 1, X86InstInfo{"VPSLLQ",   TYPE_INST, FLAGS_MODRM | FLAGS_XMM_FLAGS, 1, nullptr}},
    {OPD(TYPE_VEX_GROUP_14, 1, 0b111), 1, X86InstInfo{"VPSLLDQ",  TYPE_INST, FLAGS_MODRM | FLAGS_XMM_FLAGS, 1, nullptr}},

    {OPD(TYPE_VEX_GROUP_15, 1, 0b010), 1, X86InstInfo{"VLDMXCSR", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(TYPE_VEX_GROUP_15, 1, 0b011), 1, X86InstInfo{"VSTMXCSR", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(TYPE_VEX_GROUP_17, 0, 0b001), 1, X86InstInfo{"BLSR",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(TYPE_VEX_GROUP_17, 0, 0b010), 1, X86InstInfo{"BLSMSK",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(TYPE_VEX_GROUP_17, 0, 0b011), 1, X86InstInfo{"BLSI",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
  };
#undef OPD

  GenerateTable(VEXTableOps, VEXTable, sizeof(VEXTable) / sizeof(VEXTable[0]));
  GenerateTable(VEXTableGroupOps, VEXGroupTable, sizeof(VEXGroupTable) / sizeof(VEXGroupTable[0]));
}
}
