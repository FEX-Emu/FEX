// SPDX-License-Identifier: MIT
/*
$info$
tags: frontend|x86-tables
$end_info$
*/

#include "Interface/Core/X86Tables/X86Tables.h"
#include "Interface/Core/OpcodeDispatcher/DDDTables.h"

#include <iterator>

namespace FEXCore::X86Tables {
using namespace InstFlags;

std::array<X86InstInfo, MAX_3DNOW_TABLE_SIZE> DDDNowOps = []() consteval {
  std::array<X86InstInfo, MAX_3DNOW_TABLE_SIZE> Table{};
  constexpr U8U8InfoStruct DDDNowOpTable[] = {
    {0x0C, 1, X86InstInfo{"PI2FW",    TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX, 0, nullptr}},
    {0x0D, 1, X86InstInfo{"PI2FD",    TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX, 0, nullptr}},
    {0x1C, 1, X86InstInfo{"PF2IW",    TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX, 0, nullptr}},
    {0x1D, 1, X86InstInfo{"PF2ID",    TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX, 0, nullptr}},

    // Inverse 3DNow! These two instructions are Geode product line specific
    // No CPUID for these, you're expected to read ID_CONFIG_MSR (1250h) bit 1
    {0x86, 1, X86InstInfo{"PFRCPV",   TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX, 0, nullptr}},
    {0x87, 1, X86InstInfo{"PFRSQRTV", TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX, 0, nullptr}},

    {0x8A, 1, X86InstInfo{"PFNACC",   TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX, 0, nullptr}},
    {0x8E, 1, X86InstInfo{"PFPNACC",  TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX, 0, nullptr}},

    {0x90, 1, X86InstInfo{"PFCMPGE",  TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX, 0, nullptr}},
    {0x94, 1, X86InstInfo{"PFMIN",    TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX, 0, nullptr}},
    {0x96, 1, X86InstInfo{"PFRCP",    TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX, 0, nullptr}},
    {0x97, 1, X86InstInfo{"PFRSQRT",  TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX, 0, nullptr}},

    {0x9A, 1, X86InstInfo{"PFSUB",    TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX, 0, nullptr}},
    {0x9E, 1, X86InstInfo{"PFADD",    TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX, 0, nullptr}},

    {0xA0, 1, X86InstInfo{"PFCMPGT",  TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX, 0, nullptr}},
    {0xA4, 1, X86InstInfo{"PFMAX",    TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX, 0, nullptr}},
    {0xA6, 1, X86InstInfo{"PFRCPIT1", TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX, 0, nullptr}},
    {0xA7, 1, X86InstInfo{"PFRSQIT1", TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX, 0, nullptr}},

    {0xAA, 1, X86InstInfo{"PFSUBR",   TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX, 0, nullptr}},
    {0xAE, 1, X86InstInfo{"PFACC",    TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX, 0, nullptr}},

    {0xB0, 1, X86InstInfo{"PFCMPEQ",  TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX, 0, nullptr}},
    {0xB4, 1, X86InstInfo{"PFMUL",    TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX, 0, nullptr}},
    {0xB6, 1, X86InstInfo{"PFRCPIT2", TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX, 0, nullptr}},
    {0xB7, 1, X86InstInfo{"PMULHRW",  TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX, 0, nullptr}},

    {0xBB, 1, X86InstInfo{"PSWAPD",   TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX, 0, nullptr}},
    {0xBF, 1, X86InstInfo{"PAVGUSB",  TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX, 0, nullptr}},
  };

  GenerateTable(&Table.at(0), DDDNowOpTable, std::size(DDDNowOpTable));

  IR::InstallToTable(Table, IR::OpDispatch_DDDTable);
  return Table;
}();

}
