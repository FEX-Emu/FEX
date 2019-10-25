#include "Interface/Core/X86Tables/X86Tables.h"

namespace FEXCore::X86Tables {
using namespace InstFlags;

void InitializeEVEXTables() {
  const U16U8InfoStruct EVEXTable[] = {
    {0x10, 1, X86InstInfo{"VMOVUPS",         TYPE_INST, FLAGS_MODRM | FLAGS_XMM_FLAGS, 0, nullptr}},
    {0x11, 1, X86InstInfo{"VMOVUPS",         TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS, 0, nullptr}},
    {0x59, 1, X86InstInfo{"VBROADCASTQ",     TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS, 0, nullptr}},
    {0x7F, 1, X86InstInfo{"VMOVDQU64",       TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS, 0, nullptr}},
  };

  GenerateTable(EVEXTableOps, EVEXTable, sizeof(EVEXTable) / sizeof(EVEXTable[0]));
}
}
