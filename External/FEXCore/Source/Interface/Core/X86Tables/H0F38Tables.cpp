#include "Interface/Core/X86Tables/X86Tables.h"

namespace FEXCore::X86Tables {
using namespace InstFlags;

void InitializeH0F38Tables() {
#define OPD(prefix, opcode) ((prefix << 8) | opcode)
  constexpr uint16_t PF_38_NONE = 0;
  constexpr uint16_t PF_38_66   = 1;
  constexpr uint16_t PF_38_F2   = 2;

  const U16U8InfoStruct H0F38Table[] = {
    {OPD(PF_38_NONE, 0x00), 1, X86InstInfo{"PSHUFB",     TYPE_INST, GenFlagsSameSize(SIZE_64BIT)  | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX, 0, nullptr}},
    {OPD(PF_38_66,   0x00), 1, X86InstInfo{"PSHUFB",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0, nullptr}},
    {OPD(PF_38_NONE, 0x01), 1, X86InstInfo{"PHADDW",     TYPE_INST, GenFlagsSameSize(SIZE_64BIT)  | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX, 0, nullptr}},
    {OPD(PF_38_66,   0x01), 1, X86InstInfo{"PHADDW",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0, nullptr}},
    {OPD(PF_38_NONE, 0x02), 1, X86InstInfo{"PHADDD",     TYPE_INST, GenFlagsSameSize(SIZE_64BIT)  | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX, 0, nullptr}},
    {OPD(PF_38_66,   0x02), 1, X86InstInfo{"PHADDD",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0, nullptr}},
    {OPD(PF_38_NONE, 0x03), 1, X86InstInfo{"PHADDSW",    TYPE_INST, GenFlagsSameSize(SIZE_64BIT)  | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX, 0, nullptr}},
    {OPD(PF_38_66,   0x03), 1, X86InstInfo{"PHADDSW",    TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0, nullptr}},
    {OPD(PF_38_NONE, 0x04), 1, X86InstInfo{"PMADDUBSW",  TYPE_INST, GenFlagsSameSize(SIZE_64BIT)  | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX, 0, nullptr}},
    {OPD(PF_38_66,   0x04), 1, X86InstInfo{"PMADDUBSW",  TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0, nullptr}},
    {OPD(PF_38_NONE, 0x05), 1, X86InstInfo{"PHSUBW",     TYPE_INST, GenFlagsSameSize(SIZE_64BIT)  | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX, 0, nullptr}},
    {OPD(PF_38_66,   0x05), 1, X86InstInfo{"PHSUBW",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0, nullptr}},
    {OPD(PF_38_NONE, 0x06), 1, X86InstInfo{"PHSUBD",     TYPE_INST, GenFlagsSameSize(SIZE_64BIT)  | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX, 0, nullptr}},
    {OPD(PF_38_66,   0x06), 1, X86InstInfo{"PHSUBD",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0, nullptr}},
    {OPD(PF_38_NONE, 0x07), 1, X86InstInfo{"PHSUBSW",    TYPE_INST, GenFlagsSameSize(SIZE_64BIT)  | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX, 0, nullptr}},
    {OPD(PF_38_66,   0x07), 1, X86InstInfo{"PHSUBSW",    TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0, nullptr}},
    {OPD(PF_38_NONE, 0x08), 1, X86InstInfo{"PSIGNB",     TYPE_INST, GenFlagsSameSize(SIZE_64BIT)  | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX, 0, nullptr}},
    {OPD(PF_38_66,   0x08), 1, X86InstInfo{"PSIGNB",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0, nullptr}},
    {OPD(PF_38_NONE, 0x09), 1, X86InstInfo{"PSIGNW",     TYPE_INST, GenFlagsSameSize(SIZE_64BIT)  | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX, 0, nullptr}},
    {OPD(PF_38_66,   0x09), 1, X86InstInfo{"PSIGNW",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0, nullptr}},
    {OPD(PF_38_NONE, 0x0A), 1, X86InstInfo{"PSIGND",     TYPE_INST, GenFlagsSameSize(SIZE_64BIT)  | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX, 0, nullptr}},
    {OPD(PF_38_66,   0x0A), 1, X86InstInfo{"PSIGND",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0, nullptr}},
    {OPD(PF_38_NONE, 0x0B), 1, X86InstInfo{"PMULHRSW",   TYPE_INST, GenFlagsSameSize(SIZE_64BIT)  | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX, 0, nullptr}},
    {OPD(PF_38_66,   0x0B), 1, X86InstInfo{"PMULHRSW",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0, nullptr}},

    {OPD(PF_38_66,   0x10), 1, X86InstInfo{"PBLENDVB",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_66,   0x14), 1, X86InstInfo{"BLENDVPS",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_66,   0x15), 1, X86InstInfo{"BLENDVPD",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_66,   0x17), 1, X86InstInfo{"PTEST",      TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_NONE, 0x1C), 1, X86InstInfo{"PABSB",      TYPE_INST, GenFlagsSameSize(SIZE_64BIT)  | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX, 0, nullptr}},
    {OPD(PF_38_66,   0x1C), 1, X86InstInfo{"PABSB",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0, nullptr}},
    {OPD(PF_38_NONE, 0x1D), 1, X86InstInfo{"PABSW",      TYPE_INST, GenFlagsSameSize(SIZE_64BIT)  | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX, 0, nullptr}},
    {OPD(PF_38_66,   0x1D), 1, X86InstInfo{"PABSW",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0, nullptr}},
    {OPD(PF_38_NONE, 0x1E), 1, X86InstInfo{"PABSD",      TYPE_INST, GenFlagsSameSize(SIZE_64BIT)  | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX, 0, nullptr}},
    {OPD(PF_38_66,   0x1E), 1, X86InstInfo{"PABSD",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0, nullptr}},

    {OPD(PF_38_66,   0x20), 1, X86InstInfo{"PMOVSXBW",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_66,   0x21), 1, X86InstInfo{"PMOVSXBD",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_66,   0x22), 1, X86InstInfo{"PMOVSXBQ",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_66,   0x23), 1, X86InstInfo{"PMOVSXWD",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_66,   0x24), 1, X86InstInfo{"PMOVSXWQ",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_66,   0x25), 1, X86InstInfo{"PMOVSXDQ",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_66,   0x28), 1, X86InstInfo{"PMULDQ",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_66,   0x29), 1, X86InstInfo{"PCMPEQQ",    TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_66,   0x2A), 1, X86InstInfo{"MOVNTDQA",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_66,   0x2B), 1, X86InstInfo{"PACKUSDW",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(PF_38_66,   0x30), 1, X86InstInfo{"PMOVZXBW",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_66,   0x31), 1, X86InstInfo{"PMOVZXBD",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_66,   0x32), 1, X86InstInfo{"PMOVZXBQ",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_66,   0x33), 1, X86InstInfo{"PMOVZXWD",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_66,   0x34), 1, X86InstInfo{"PMOVZXWQ",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_66,   0x35), 1, X86InstInfo{"PMOVZXDQ",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_66,   0x38), 1, X86InstInfo{"PMINSB",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_66,   0x39), 1, X86InstInfo{"PMINSD",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_66,   0x3A), 1, X86InstInfo{"PMINUW",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_66,   0x3B), 1, X86InstInfo{"PMINUD",     TYPE_INST, FLAGS_MODRM | FLAGS_XMM_FLAGS, 0, nullptr}},
    {OPD(PF_38_66,   0x3C), 1, X86InstInfo{"PMAXSB",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_66,   0x3D), 1, X86InstInfo{"PMAXSD",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_66,   0x3E), 1, X86InstInfo{"PMAXUW",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_66,   0x3F), 1, X86InstInfo{"PMAXUD",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(PF_38_66,   0x40), 1, X86InstInfo{"PMULLD",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_66,   0x41), 1, X86InstInfo{"PHMINPOSUW", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(PF_38_66,   0xDB), 1, X86InstInfo{"AESIMC",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0, nullptr}},
    {OPD(PF_38_66,   0xDC), 1, X86InstInfo{"AESENC",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0, nullptr}},
    {OPD(PF_38_66,   0xDD), 1, X86InstInfo{"AESENCLAST", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0, nullptr}},
    {OPD(PF_38_66,   0xDE), 1, X86InstInfo{"AESDEC",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0, nullptr}},
    {OPD(PF_38_66,   0xDF), 1, X86InstInfo{"AESDECLAST", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0, nullptr}},

    {OPD(PF_38_NONE, 0xF0), 1, X86InstInfo{"MOVBE",      TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY, 0, nullptr}},
    {OPD(PF_38_NONE, 0xF1), 1, X86InstInfo{"MOVBE",      TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_MOD_MEM_ONLY, 0, nullptr}},

    {OPD(PF_38_66, 0xF0), 1, X86InstInfo{"MOVBE",      TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY, 0, nullptr}},
    {OPD(PF_38_66, 0xF1), 1, X86InstInfo{"MOVBE",      TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_MOD_MEM_ONLY, 0, nullptr}},

    {OPD(PF_38_F2,   0xF0), 1, X86InstInfo{"CRC32",      TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_F2,   0xF1), 1, X86InstInfo{"CRC32",      TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
  };
#undef OPD

  GenerateTable(H0F38TableOps, H0F38Table, sizeof(H0F38Table) / sizeof(H0F38Table[0]));
}
}
