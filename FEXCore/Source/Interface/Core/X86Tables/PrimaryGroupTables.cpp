// SPDX-License-Identifier: MIT
/*
$info$
tags: frontend|x86-tables
$end_info$
*/

#include "Interface/Core/X86Tables/X86Tables.h"
#include "Interface/Core/OpcodeDispatcher/PrimaryGroupTables.h"

#include <FEXCore/Core/Context.h>

#include <iterator>

namespace FEXCore::X86Tables {
using namespace InstFlags;
enum PrimaryGroup_LUT {
  ENTRY_1_82_0,
  ENTRY_1_82_1,
  ENTRY_1_82_2,
  ENTRY_1_82_3,
  ENTRY_1_82_4,
  ENTRY_1_82_5,
  ENTRY_1_82_6,
  ENTRY_1_82_7,
  ENTRY_MAX,
};

constexpr std::array<X86InstInfo[2], ENTRY_MAX> PrimaryGroup_ArchSelect_LUT = {{
  {
    {"ADD",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST, 1, { .OpDispatch = &IR::OpDispatchBuilder::SecondaryALUOp }},
    {"", TYPE_INVALID, FLAGS_NONE, 0, { .OpDispatch = nullptr } },
  },
  {
    {"OR",   TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST, 1, { .OpDispatch = &IR::OpDispatchBuilder::SecondaryALUOp }},
    {"", TYPE_INVALID, FLAGS_NONE, 0, { .OpDispatch = nullptr } },
  },
  {
    {"ADC",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST, 1, { .OpDispatch = &IR::OpDispatchBuilder::Bind<&IR::OpDispatchBuilder::ADCOp, 1> }},
    {"", TYPE_INVALID, FLAGS_NONE, 0, { .OpDispatch = nullptr } },
  },
  {
    {"SBB",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST, 1, { .OpDispatch = &IR::OpDispatchBuilder::Bind<&IR::OpDispatchBuilder::SBBOp, 1> }},
    {"", TYPE_INVALID, FLAGS_NONE, 0, { .OpDispatch = nullptr } },
  },
  {
    {"AND",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST, 1, { .OpDispatch = &IR::OpDispatchBuilder::SecondaryALUOp }},
    {"", TYPE_INVALID, FLAGS_NONE, 0, { .OpDispatch = nullptr } },
  },
  {
    {"SUB",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST, 1, { .OpDispatch = &IR::OpDispatchBuilder::SecondaryALUOp }},
    {"", TYPE_INVALID, FLAGS_NONE, 0, { .OpDispatch = nullptr } },
  },
  {
    {"XOR",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST, 1, { .OpDispatch = &IR::OpDispatchBuilder::SecondaryALUOp }},
    {"", TYPE_INVALID, FLAGS_NONE, 0, { .OpDispatch = nullptr } },
  },
  {
    {"CMP",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST, 1, { .OpDispatch = &IR::OpDispatchBuilder::Bind<&IR::OpDispatchBuilder::CMPOp, 1> }},
    {"", TYPE_INVALID, FLAGS_NONE, 0, { .OpDispatch = nullptr } },
  },
}};

constexpr std::array<X86InstInfo, MAX_INST_GROUP_TABLE_SIZE> PrimaryInstGroupOps = []() consteval {
  std::array<X86InstInfo, MAX_INST_GROUP_TABLE_SIZE> Table{};
#define OPD(group, prefix, Reg) (((group - FEXCore::X86Tables::TYPE_GROUP_1) << 6) | (prefix) << 3 | (Reg))
  constexpr U16U8InfoStruct PrimaryGroupOpTable[] = {
    // GROUP_1 | 0x80 | reg
    {OPD(TYPE_GROUP_1, OpToIndex(0x80), 0), 1, X86InstInfo{"ADD",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                      1}},
    {OPD(TYPE_GROUP_1, OpToIndex(0x80), 1), 1, X86InstInfo{"OR",   TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                      1}},
    {OPD(TYPE_GROUP_1, OpToIndex(0x80), 2), 1, X86InstInfo{"ADC",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                      1}},
    {OPD(TYPE_GROUP_1, OpToIndex(0x80), 3), 1, X86InstInfo{"SBB",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                      1}},
    {OPD(TYPE_GROUP_1, OpToIndex(0x80), 4), 1, X86InstInfo{"AND",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                      1}},
    {OPD(TYPE_GROUP_1, OpToIndex(0x80), 5), 1, X86InstInfo{"SUB",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                      1}},
    {OPD(TYPE_GROUP_1, OpToIndex(0x80), 6), 1, X86InstInfo{"XOR",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                      1}},
    {OPD(TYPE_GROUP_1, OpToIndex(0x80), 7), 1, X86InstInfo{"CMP",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                      1}},

    {OPD(TYPE_GROUP_1, OpToIndex(0x81), 0), 1, X86InstInfo{"ADD",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SRC_SEXT64BIT | FLAGS_DISPLACE_SIZE_DIV_2,                          4}},
    {OPD(TYPE_GROUP_1, OpToIndex(0x81), 1), 1, X86InstInfo{"OR",   TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SRC_SEXT64BIT | FLAGS_DISPLACE_SIZE_DIV_2,                          4}},
    {OPD(TYPE_GROUP_1, OpToIndex(0x81), 2), 1, X86InstInfo{"ADC",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SRC_SEXT64BIT | FLAGS_DISPLACE_SIZE_DIV_2,                          4}},
    {OPD(TYPE_GROUP_1, OpToIndex(0x81), 3), 1, X86InstInfo{"SBB",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SRC_SEXT64BIT | FLAGS_DISPLACE_SIZE_DIV_2,                          4}},
    {OPD(TYPE_GROUP_1, OpToIndex(0x81), 4), 1, X86InstInfo{"AND",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SRC_SEXT64BIT | FLAGS_DISPLACE_SIZE_DIV_2,                          4}},
    {OPD(TYPE_GROUP_1, OpToIndex(0x81), 5), 1, X86InstInfo{"SUB",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SRC_SEXT64BIT | FLAGS_DISPLACE_SIZE_DIV_2,                          4}},
    {OPD(TYPE_GROUP_1, OpToIndex(0x81), 6), 1, X86InstInfo{"XOR",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SRC_SEXT64BIT | FLAGS_DISPLACE_SIZE_DIV_2,                          4}},
    {OPD(TYPE_GROUP_1, OpToIndex(0x81), 7), 1, X86InstInfo{"CMP",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SRC_SEXT64BIT | FLAGS_DISPLACE_SIZE_DIV_2,                          4}},

    // Duplicates the 0x80 opcode group
    {OPD(TYPE_GROUP_1, OpToIndex(0x82), 0), 1, X86InstInfo{"",  TYPE_ARCH_DISPATCHER, FLAGS_NONE, 0, { .Indirect = PrimaryGroup_ArchSelect_LUT[ENTRY_1_82_0] }}},
    {OPD(TYPE_GROUP_1, OpToIndex(0x82), 1), 1, X86InstInfo{"",  TYPE_ARCH_DISPATCHER, FLAGS_NONE, 0, { .Indirect = PrimaryGroup_ArchSelect_LUT[ENTRY_1_82_1] }}},
    {OPD(TYPE_GROUP_1, OpToIndex(0x82), 2), 1, X86InstInfo{"",  TYPE_ARCH_DISPATCHER, FLAGS_NONE, 0, { .Indirect = PrimaryGroup_ArchSelect_LUT[ENTRY_1_82_2] }}},
    {OPD(TYPE_GROUP_1, OpToIndex(0x82), 3), 1, X86InstInfo{"",  TYPE_ARCH_DISPATCHER, FLAGS_NONE, 0, { .Indirect = PrimaryGroup_ArchSelect_LUT[ENTRY_1_82_3] }}},
    {OPD(TYPE_GROUP_1, OpToIndex(0x82), 4), 1, X86InstInfo{"",  TYPE_ARCH_DISPATCHER, FLAGS_NONE, 0, { .Indirect = PrimaryGroup_ArchSelect_LUT[ENTRY_1_82_4] }}},
    {OPD(TYPE_GROUP_1, OpToIndex(0x82), 5), 1, X86InstInfo{"",  TYPE_ARCH_DISPATCHER, FLAGS_NONE, 0, { .Indirect = PrimaryGroup_ArchSelect_LUT[ENTRY_1_82_5] }}},
    {OPD(TYPE_GROUP_1, OpToIndex(0x82), 6), 1, X86InstInfo{"",  TYPE_ARCH_DISPATCHER, FLAGS_NONE, 0, { .Indirect = PrimaryGroup_ArchSelect_LUT[ENTRY_1_82_6] }}},
    {OPD(TYPE_GROUP_1, OpToIndex(0x82), 7), 1, X86InstInfo{"",  TYPE_ARCH_DISPATCHER, FLAGS_NONE, 0, { .Indirect = PrimaryGroup_ArchSelect_LUT[ENTRY_1_82_7] }}},

    {OPD(TYPE_GROUP_1, OpToIndex(0x83), 0), 1, X86InstInfo{"ADD",  TYPE_INST, FLAGS_SRC_SEXT | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                     1}},
    {OPD(TYPE_GROUP_1, OpToIndex(0x83), 1), 1, X86InstInfo{"OR",   TYPE_INST, FLAGS_SRC_SEXT | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                     1}},
    {OPD(TYPE_GROUP_1, OpToIndex(0x83), 2), 1, X86InstInfo{"ADC",  TYPE_INST, FLAGS_SRC_SEXT | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                     1}},
    {OPD(TYPE_GROUP_1, OpToIndex(0x83), 3), 1, X86InstInfo{"SBB",  TYPE_INST, FLAGS_SRC_SEXT | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                     1}},
    {OPD(TYPE_GROUP_1, OpToIndex(0x83), 4), 1, X86InstInfo{"AND",  TYPE_INST, FLAGS_SRC_SEXT | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                     1}},
    {OPD(TYPE_GROUP_1, OpToIndex(0x83), 5), 1, X86InstInfo{"SUB",  TYPE_INST, FLAGS_SRC_SEXT | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                     1}},
    {OPD(TYPE_GROUP_1, OpToIndex(0x83), 6), 1, X86InstInfo{"XOR",  TYPE_INST, FLAGS_SRC_SEXT | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                     1}},
    {OPD(TYPE_GROUP_1, OpToIndex(0x83), 7), 1, X86InstInfo{"CMP",  TYPE_INST, FLAGS_SRC_SEXT | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                     1}},

    // GROUP 2
    {OPD(TYPE_GROUP_2, OpToIndex(0xC0), 0), 1, X86InstInfo{"ROL",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                      1}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xC0), 1), 1, X86InstInfo{"ROR",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                      1}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xC0), 2), 1, X86InstInfo{"RCL",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                      1}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xC0), 3), 1, X86InstInfo{"RCR",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                      1}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xC0), 4), 1, X86InstInfo{"SHL",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                      1}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xC0), 5), 1, X86InstInfo{"SHR",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                      1}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xC0), 6), 1, X86InstInfo{"SHL",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                      1}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xC0), 7), 1, X86InstInfo{"SAR",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                      1}},

    {OPD(TYPE_GROUP_2, OpToIndex(0xC1), 0), 1, X86InstInfo{"ROL",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                      1}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xC1), 1), 1, X86InstInfo{"ROR",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                      1}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xC1), 2), 1, X86InstInfo{"RCL",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                      1}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xC1), 3), 1, X86InstInfo{"RCR",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                      1}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xC1), 4), 1, X86InstInfo{"SHL",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                      1}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xC1), 5), 1, X86InstInfo{"SHR",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                      1}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xC1), 6), 1, X86InstInfo{"SHL",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                      1}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xC1), 7), 1, X86InstInfo{"SAR",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                      1}},

    {OPD(TYPE_GROUP_2, OpToIndex(0xD0), 0), 1, X86InstInfo{"ROL",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                      0}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xD0), 1), 1, X86InstInfo{"ROR",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                      0}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xD0), 2), 1, X86InstInfo{"RCL",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                      0}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xD0), 3), 1, X86InstInfo{"RCR",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                      0}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xD0), 4), 1, X86InstInfo{"SHL",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                      0}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xD0), 5), 1, X86InstInfo{"SHR",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                      0}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xD0), 6), 1, X86InstInfo{"SHL",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                      0}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xD0), 7), 1, X86InstInfo{"SAR",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                      0}},

    {OPD(TYPE_GROUP_2, OpToIndex(0xD1), 0), 1, X86InstInfo{"ROL",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                      0}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xD1), 1), 1, X86InstInfo{"ROR",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                      0}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xD1), 2), 1, X86InstInfo{"RCL",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                      0}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xD1), 3), 1, X86InstInfo{"RCR",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                      0}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xD1), 4), 1, X86InstInfo{"SHL",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                      0}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xD1), 5), 1, X86InstInfo{"SHR",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                      0}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xD1), 6), 1, X86InstInfo{"SHL",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                      0}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xD1), 7), 1, X86InstInfo{"SAR",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                      0}},

    {OPD(TYPE_GROUP_2, OpToIndex(0xD2), 0), 1, X86InstInfo{"ROL",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_SRC_RCX,                   0}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xD2), 1), 1, X86InstInfo{"ROR",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_SRC_RCX,                   0}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xD2), 2), 1, X86InstInfo{"RCL",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_SRC_RCX,                   0}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xD2), 3), 1, X86InstInfo{"RCR",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_SRC_RCX,                   0}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xD2), 4), 1, X86InstInfo{"SHL",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_SRC_RCX,                   0}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xD2), 5), 1, X86InstInfo{"SHR",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_SRC_RCX,                   0}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xD2), 6), 1, X86InstInfo{"SHL",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_SRC_RCX,                   0}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xD2), 7), 1, X86InstInfo{"SAR",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_SRC_RCX,                   0}},

    {OPD(TYPE_GROUP_2, OpToIndex(0xD3), 0), 1, X86InstInfo{"ROL",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_SRC_RCX,                                   0}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xD3), 1), 1, X86InstInfo{"ROR",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_SRC_RCX,                                   0}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xD3), 2), 1, X86InstInfo{"RCL",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_SRC_RCX,                                   0}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xD3), 3), 1, X86InstInfo{"RCR",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_SRC_RCX,                                   0}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xD3), 4), 1, X86InstInfo{"SHL",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_SRC_RCX,                                   0}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xD3), 5), 1, X86InstInfo{"SHR",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_SRC_RCX,                                   0}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xD3), 6), 1, X86InstInfo{"SHL",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_SRC_RCX,                                   0}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xD3), 7), 1, X86InstInfo{"SAR",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_SRC_RCX,                                   0}},

    // GROUP 3
    {OPD(TYPE_GROUP_3, OpToIndex(0xF6), 0), 1, X86InstInfo{"TEST", TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                      1}},
    {OPD(TYPE_GROUP_3, OpToIndex(0xF6), 1), 1, X86InstInfo{"TEST", TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                      1}},
    {OPD(TYPE_GROUP_3, OpToIndex(0xF6), 2), 1, X86InstInfo{"NOT",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                      0}},
    {OPD(TYPE_GROUP_3, OpToIndex(0xF6), 3), 1, X86InstInfo{"NEG",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                      0}},
    {OPD(TYPE_GROUP_3, OpToIndex(0xF6), 4), 1, X86InstInfo{"MUL",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                      0}},
    {OPD(TYPE_GROUP_3, OpToIndex(0xF6), 5), 1, X86InstInfo{"IMUL", TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                      0}},
    {OPD(TYPE_GROUP_3, OpToIndex(0xF6), 6), 1, X86InstInfo{"DIV",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                      0}},
    {OPD(TYPE_GROUP_3, OpToIndex(0xF6), 7), 1, X86InstInfo{"IDIV", TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                      0}},

    {OPD(TYPE_GROUP_3, OpToIndex(0xF7), 0), 1, X86InstInfo{"TEST", TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SRC_SEXT64BIT | FLAGS_DISPLACE_SIZE_DIV_2,                          4}},
    {OPD(TYPE_GROUP_3, OpToIndex(0xF7), 1), 1, X86InstInfo{"TEST", TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SRC_SEXT64BIT | FLAGS_DISPLACE_SIZE_DIV_2,                          4}},
    {OPD(TYPE_GROUP_3, OpToIndex(0xF7), 2), 1, X86InstInfo{"NOT",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                      0}},
    {OPD(TYPE_GROUP_3, OpToIndex(0xF7), 3), 1, X86InstInfo{"NEG",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                      0}},
    {OPD(TYPE_GROUP_3, OpToIndex(0xF7), 4), 1, X86InstInfo{"MUL",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                      0}},
    {OPD(TYPE_GROUP_3, OpToIndex(0xF7), 5), 1, X86InstInfo{"IMUL", TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                      0}},
    {OPD(TYPE_GROUP_3, OpToIndex(0xF7), 6), 1, X86InstInfo{"DIV",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                      0}},
    {OPD(TYPE_GROUP_3, OpToIndex(0xF7), 7), 1, X86InstInfo{"IDIV", TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                      0}},

    // GROUP 4
    {OPD(TYPE_GROUP_4, OpToIndex(0xFE), 0), 1, X86InstInfo{"INC",   TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                     0}},
    {OPD(TYPE_GROUP_4, OpToIndex(0xFE), 1), 1, X86InstInfo{"DEC",   TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                     0}},
    {OPD(TYPE_GROUP_4, OpToIndex(0xFE), 2), 6, X86InstInfo{"",      TYPE_INVALID, FLAGS_NONE,                                                       0}},

    // GROUP 5
    {OPD(TYPE_GROUP_5, OpToIndex(0xFF), 0), 1, X86InstInfo{"INC",   TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                     0}},
    {OPD(TYPE_GROUP_5, OpToIndex(0xFF), 1), 1, X86InstInfo{"DEC",   TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                     0}},
    {OPD(TYPE_GROUP_5, OpToIndex(0xFF), 2), 1, X86InstInfo{"CALL",  TYPE_INST, GenFlagsSameSize(SIZE_64BITDEF) | FLAGS_SETS_RIP | FLAGS_MODRM | FLAGS_BLOCK_END | FLAGS_CALL , 0}},
    {OPD(TYPE_GROUP_5, OpToIndex(0xFF), 3), 1, X86InstInfo{"CALLF", TYPE_INST, FLAGS_SETS_RIP | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_MOD_MEM_ONLY | FLAGS_BLOCK_END,                  0}},
    {OPD(TYPE_GROUP_5, OpToIndex(0xFF), 4), 1, X86InstInfo{"JMP",   TYPE_INST, GenFlagsSameSize(SIZE_64BITDEF) | FLAGS_SETS_RIP | FLAGS_MODRM | FLAGS_BLOCK_END , 0}},
    {OPD(TYPE_GROUP_5, OpToIndex(0xFF), 5), 1, X86InstInfo{"JMPF",  TYPE_INST, FLAGS_SETS_RIP | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_MOD_MEM_ONLY | FLAGS_BLOCK_END,                  0}},
    {OPD(TYPE_GROUP_5, OpToIndex(0xFF), 6), 1, X86InstInfo{"PUSH",  TYPE_INST, GenFlagsSameSize(SIZE_64BITDEF) | FLAGS_DEBUG_MEM_ACCESS | FLAGS_MODRM,                                                     0}},
    {OPD(TYPE_GROUP_5, OpToIndex(0xFF), 7), 1, X86InstInfo{"",      TYPE_INVALID, FLAGS_NONE,                                                       0}},

    // GROUP 11
    {OPD(TYPE_GROUP_11, OpToIndex(0xC6), 0), 1, X86InstInfo{"MOV",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST  | FLAGS_SRC_SEXT,                   1}},
    {OPD(TYPE_GROUP_11, OpToIndex(0xC6), 1), 5, X86InstInfo{"",     TYPE_INVALID, FLAGS_NONE,                                                       0}},
    {OPD(TYPE_GROUP_11, OpToIndex(0xC6), 7), 1, X86InstInfo{"XABORT", TYPE_INST, FLAGS_MODRM,                                                       1}},
    {OPD(TYPE_GROUP_11, OpToIndex(0xC7), 0), 1, X86InstInfo{"MOV",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SRC_SEXT | FLAGS_DISPLACE_SIZE_DIV_2,                                   4}},
    {OPD(TYPE_GROUP_11, OpToIndex(0xC7), 1), 5, X86InstInfo{"",     TYPE_INVALID, FLAGS_NONE,                                                       0}},
    {OPD(TYPE_GROUP_11, OpToIndex(0xC7), 7), 1, X86InstInfo{"XBEGIN", TYPE_INST, FLAGS_MODRM | FLAGS_SRC_SEXT | FLAGS_SETS_RIP | FLAGS_DISPLACE_SIZE_DIV_2,                                                       4}},
  };

  GenerateTable(Table.data(), PrimaryGroupOpTable, std::size(PrimaryGroupOpTable));

  IR::InstallToTable(Table, IR::OpDispatch_PrimaryGroupTables);
  return Table;
}();

}
