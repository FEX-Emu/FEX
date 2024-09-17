// SPDX-License-Identifier: MIT
/*
$info$
tags: frontend|x86-tables
$end_info$
*/

#include "Interface/Core/X86Tables/X86Tables.h"
#include "Interface/Core/OpcodeDispatcher/SecondaryModRMTables.h"

#include <iterator>

namespace FEXCore::X86Tables {
using namespace InstFlags;
std::array<X86InstInfo, MAX_SECOND_MODRM_TABLE_SIZE> SecondModRMTableOps = []() consteval {
  std::array<X86InstInfo, MAX_SECOND_MODRM_TABLE_SIZE> Table{};
  constexpr U8U8InfoStruct SecondaryModRMExtensionOpTable[] = {
    // REG /1
    {((0 << 3) | 0), 1, X86InstInfo{"MONITOR",  TYPE_INST,    FLAGS_NONE, 0, nullptr}},
    {((0 << 3) | 1), 1, X86InstInfo{"MWAIT",    TYPE_INST,    FLAGS_NONE, 0, nullptr}},
    {((0 << 3) | 2), 1, X86InstInfo{"",         TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
    {((0 << 3) | 3), 1, X86InstInfo{"",         TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
    {((0 << 3) | 4), 1, X86InstInfo{"",         TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
    {((0 << 3) | 5), 1, X86InstInfo{"",         TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
    {((0 << 3) | 6), 1, X86InstInfo{"",         TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
    {((0 << 3) | 7), 1, X86InstInfo{"",         TYPE_INVALID, FLAGS_NONE, 0, nullptr}},

    // REG /2
    {((1 << 3) | 0), 1, X86InstInfo{"XGETBV",   TYPE_INST,    FLAGS_NONE, 0, nullptr}},
    {((1 << 3) | 1), 1, X86InstInfo{"XSETBV",   TYPE_PRIV,    FLAGS_NONE, 0, nullptr}},
    {((1 << 3) | 2), 1, X86InstInfo{"",         TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
    {((1 << 3) | 3), 1, X86InstInfo{"",         TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
    {((1 << 3) | 4), 1, X86InstInfo{"",         TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
    {((1 << 3) | 5), 1, X86InstInfo{"XEND",     TYPE_INST, FLAGS_NONE, 0, nullptr}},
    {((1 << 3) | 6), 1, X86InstInfo{"XTEST",    TYPE_INST, FLAGS_NONE, 0, nullptr}},
    {((1 << 3) | 7), 1, X86InstInfo{"",         TYPE_INVALID, FLAGS_NONE, 0, nullptr}},

    // REG /3
    {((2 << 3) | 0), 1, X86InstInfo{"VMRUN",    TYPE_PRIV,    FLAGS_NONE, 0, nullptr}},
    {((2 << 3) | 1), 1, X86InstInfo{"VMMCALL",  TYPE_PRIV,    FLAGS_NONE, 0, nullptr}},
    {((2 << 3) | 2), 1, X86InstInfo{"VMLOAD",   TYPE_PRIV,    FLAGS_NONE, 0, nullptr}},
    {((2 << 3) | 3), 1, X86InstInfo{"VMSAVE",   TYPE_PRIV,    FLAGS_NONE, 0, nullptr}},
    {((2 << 3) | 4), 1, X86InstInfo{"STGI",     TYPE_PRIV,    FLAGS_NONE, 0, nullptr}},
    {((2 << 3) | 5), 1, X86InstInfo{"CLGI",     TYPE_PRIV,    FLAGS_NONE, 0, nullptr}},
    {((2 << 3) | 6), 1, X86InstInfo{"SKINIT",   TYPE_PRIV,    FLAGS_NONE, 0, nullptr}},
    {((2 << 3) | 7), 1, X86InstInfo{"INVLPGA",  TYPE_INST,    FLAGS_NONE, 0, nullptr}},

    // REG /7
    {((3 << 3) | 0), 1, X86InstInfo{"SWAPGS",   TYPE_INST,    FLAGS_NONE, 0, nullptr}},
    {((3 << 3) | 1), 1, X86InstInfo{"RDTSCP",   TYPE_INST,    FLAGS_NONE, 0, nullptr}},
    {((3 << 3) | 2), 1, X86InstInfo{"MONITORX", TYPE_PRIV,    FLAGS_NONE, 0, nullptr}},
    {((3 << 3) | 3), 1, X86InstInfo{"MWAITX",   TYPE_PRIV,    FLAGS_NONE, 0, nullptr}},
    {((3 << 3) | 4), 1, X86InstInfo{"CLZERO",   TYPE_INST,    GenFlagsSameSize(SIZE_64BITDEF) | FLAGS_SF_SRC_RAX | FLAGS_DEBUG_MEM_ACCESS, 0, nullptr}},
    {((3 << 3) | 5), 1, X86InstInfo{"",         TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
    {((3 << 3) | 6), 1, X86InstInfo{"",         TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
    {((3 << 3) | 7), 1, X86InstInfo{"",         TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
  };

  GenerateTable(&Table.at(0), SecondaryModRMExtensionOpTable, std::size(SecondaryModRMExtensionOpTable));

  IR::InstallToTable(Table, IR::OpDispatch_SecondaryModRMTables);
  return Table;
}();

}
