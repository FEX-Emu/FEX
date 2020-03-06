#include "Interface/Core/X86Tables/X86Tables.h"

namespace FEXCore::X86Tables {
using namespace InstFlags;

void InitializeEVEXTables() {
  const U16U8InfoStruct EVEXTable[] = {
    {0x10, 1, X86InstInfo{"VMOVUPS",         TYPE_INST, FLAGS_MODRM | FLAGS_XMM_FLAGS, 0, nullptr}},
    {0x11, 1, X86InstInfo{"VMOVUPS",         TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS, 0, nullptr}},
    {0x18, 1, X86InstInfo{"VBROADCASTSS",    TYPE_INST, FLAGS_MODRM | FLAGS_XMM_FLAGS, 0, nullptr}},
    {0x19, 1, X86InstInfo{"VBROADCASTD",     TYPE_INST, FLAGS_MODRM | FLAGS_XMM_FLAGS, 0, nullptr}},
    {0x1A, 1, X86InstInfo{"VBROADCASTSD",    TYPE_INST, FLAGS_MODRM | FLAGS_XMM_FLAGS, 0, nullptr}},
    {0x1B, 1, X86InstInfo{"VBROADCASTF64X4", TYPE_INST, FLAGS_MODRM | FLAGS_XMM_FLAGS, 0, nullptr}},
    {0x28, 1, X86InstInfo{"VMOVAPS",         TYPE_INST, FLAGS_MODRM | FLAGS_XMM_FLAGS, 0, nullptr}},
    {0x29, 1, X86InstInfo{"VMOVAPS",         TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS, 0, nullptr}},
    {0x59, 1, X86InstInfo{"VBROADCASTQ",     TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS, 0, nullptr}},
    {0x6F, 1, X86InstInfo{"VMOVDQU64",       TYPE_INST, FLAGS_MODRM | FLAGS_XMM_FLAGS, 0, nullptr}},
    {0x73, 1, X86InstInfo{"VPSLLDQ",         TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS, 0, nullptr}},
    {0x7F, 1, X86InstInfo{"VMOVDQU64",       TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS, 0, nullptr}},
    {0xE7, 1, X86InstInfo{"VMOVNTDQ",        TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS, 0, nullptr}},
  };

  GenerateTable(EVEXTableOps, EVEXTable, sizeof(EVEXTable) / sizeof(EVEXTable[0]));
}
}
