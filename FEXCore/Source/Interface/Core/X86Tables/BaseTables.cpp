// SPDX-License-Identifier: MIT
/*
$info$
tags: frontend|x86-tables
$end_info$
*/

#include "Interface/Core/X86Tables/X86Tables.h"
#include "Interface/Core/OpcodeDispatcher/BaseTables.h"

#include <FEXCore/Core/Context.h>

#include <iterator>

namespace FEXCore::X86Tables {
using namespace InstFlags;

enum Primary_LUT {
  ENTRY_06,
  ENTRY_07,
  ENTRY_0E,
  ENTRY_16,
  ENTRY_17,
  ENTRY_1E,
  ENTRY_1F,
  ENTRY_27,
  ENTRY_2F,
  ENTRY_37,
  ENTRY_3F,
  ENTRY_40,
  ENTRY_48,
  ENTRY_60,
  ENTRY_61,
  ENTRY_63,
  ENTRY_9A,
  ENTRY_A0,
  ENTRY_A1,
  ENTRY_A2,
  ENTRY_A3,
  ENTRY_CE,
  ENTRY_D4,
  ENTRY_D5,
  ENTRY_D6,
  ENTRY_EA,
  ENTRY_MAX,
};

constexpr std::array<X86InstInfo[2], ENTRY_MAX> Primary_ArchSelect_LUT = {{
  // ENTRY_06
  {
    {"PUSH ES",  TYPE_INST, GenFlagsSrcSize(SIZE_16BIT) | FLAGS_DEBUG_MEM_ACCESS, 0, { .OpDispatch = &IR::OpDispatchBuilder::Bind<&IR::OpDispatchBuilder::PUSHSegmentOp, FEXCore::X86Tables::DecodeFlags::FLAG_ES_PREFIX> } },
    {"", TYPE_INVALID, FLAGS_NONE, 0, { .OpDispatch = nullptr } },
  },
  // ENTRY_07
  {
    {"POP ES",   TYPE_INST, GenFlagsSizes(SIZE_16BIT, SIZE_DEF) | FLAGS_DEBUG_MEM_ACCESS, 0, { .OpDispatch = &IR::OpDispatchBuilder::Bind<&IR::OpDispatchBuilder::POPSegmentOp, FEXCore::X86Tables::DecodeFlags::FLAG_ES_PREFIX> } },
    {"", TYPE_INVALID, FLAGS_NONE, 0, { .OpDispatch = nullptr } },
  },
  // ENTRY_0E
  {
    {"PUSH CS",  TYPE_INST, GenFlagsSrcSize(SIZE_16BIT) | FLAGS_DEBUG_MEM_ACCESS, 0, { .OpDispatch = &IR::OpDispatchBuilder::Bind<&IR::OpDispatchBuilder::PUSHSegmentOp, FEXCore::X86Tables::DecodeFlags::FLAG_CS_PREFIX> } },
    {"", TYPE_INVALID, FLAGS_NONE, 0, { .OpDispatch = nullptr } },
  },
  // ENTRY_16
  {
    {"PUSH SS",  TYPE_INST, GenFlagsSrcSize(SIZE_16BIT) | FLAGS_DEBUG_MEM_ACCESS, 0, { .OpDispatch = &IR::OpDispatchBuilder::Bind<&IR::OpDispatchBuilder::PUSHSegmentOp, FEXCore::X86Tables::DecodeFlags::FLAG_SS_PREFIX> } },
    {"", TYPE_INVALID, FLAGS_NONE, 0, { .OpDispatch = nullptr } },
  },
  // ENTRY_17
  {
    {"POP SS",   TYPE_INST, GenFlagsSizes(SIZE_16BIT, SIZE_DEF) | FLAGS_DEBUG_MEM_ACCESS, 0, { .OpDispatch = &IR::OpDispatchBuilder::Bind<&IR::OpDispatchBuilder::POPSegmentOp, FEXCore::X86Tables::DecodeFlags::FLAG_SS_PREFIX> } },
    {"", TYPE_INVALID, FLAGS_NONE, 0, { .OpDispatch = nullptr } },
  },
  // ENTRY_1E
  {
    {"PUSH DS",  TYPE_INST, GenFlagsSrcSize(SIZE_16BIT) | FLAGS_DEBUG_MEM_ACCESS, 0, { .OpDispatch = &IR::OpDispatchBuilder::Bind<&IR::OpDispatchBuilder::PUSHSegmentOp, FEXCore::X86Tables::DecodeFlags::FLAG_DS_PREFIX> } },
    {"", TYPE_INVALID, FLAGS_NONE, 0, { .OpDispatch = nullptr } },
  },
  // ENTRY_1F
  {
    {"POP DS",   TYPE_INST, GenFlagsSizes(SIZE_16BIT, SIZE_DEF) | FLAGS_DEBUG_MEM_ACCESS, 0, { .OpDispatch = &IR::OpDispatchBuilder::Bind<&IR::OpDispatchBuilder::POPSegmentOp, FEXCore::X86Tables::DecodeFlags::FLAG_DS_PREFIX> } },
    {"", TYPE_INVALID, FLAGS_NONE, 0, { .OpDispatch = nullptr } },
  },
  // ENTRY_27
  {
    {"DAA",      TYPE_INST, GenFlagsDstSize(SIZE_8BIT) | FLAGS_SF_DST_RAX, 0, { .OpDispatch = &IR::OpDispatchBuilder::DAAOp } },
    {"", TYPE_INVALID, FLAGS_NONE, 0, { .OpDispatch = nullptr } },
  },
  // ENTRY_2F
  {
    {"DAS",      TYPE_INST, GenFlagsDstSize(SIZE_8BIT) | FLAGS_SF_DST_RAX, 0, { .OpDispatch = &IR::OpDispatchBuilder::DASOp } },
    {"", TYPE_INVALID, FLAGS_NONE, 0, { .OpDispatch = nullptr } },
  },
  // ENTRY_37
  {
    {"AAA",      TYPE_INST, GenFlagsDstSize(SIZE_16BIT) | FLAGS_SF_DST_RAX, 0, { .OpDispatch = &IR::OpDispatchBuilder::AAAOp } },
    {"", TYPE_INVALID, FLAGS_NONE, 0, { .OpDispatch = nullptr } },
  },
  // ENTRY_3F
  {
    {"AAS",      TYPE_INST, GenFlagsDstSize(SIZE_16BIT) | FLAGS_SF_DST_RAX, 0, { .OpDispatch = &IR::OpDispatchBuilder::AASOp } },
    {"", TYPE_INVALID, FLAGS_NONE, 0, { .OpDispatch = nullptr } },
  },
  // ENTRY_40
  {
    {"INC",    TYPE_INST, FLAGS_SF_REX_IN_BYTE, 0, { .OpDispatch = &IR::OpDispatchBuilder::INCOp } },
    // REX
    {"", TYPE_REX_PREFIX, FLAGS_NONE, 0},
  },
  // ENTRY_48
  {
    {"DEC",    TYPE_INST, FLAGS_SF_REX_IN_BYTE, 0, { .OpDispatch = &IR::OpDispatchBuilder::DECOp } },
    {"", TYPE_REX_PREFIX, FLAGS_NONE, 0},
  },
  // ENTRY_60
  {
    {"PUSHA",  TYPE_INST, FLAGS_DEBUG_MEM_ACCESS, 0, { .OpDispatch = &IR::OpDispatchBuilder::PUSHAOp } },
    {"", TYPE_INVALID, FLAGS_NONE, 0, { .OpDispatch = nullptr } },
  },
  // ENTRY_61
  {
    {"POPA",   TYPE_INST, FLAGS_DEBUG_MEM_ACCESS, 0, { .OpDispatch = &IR::OpDispatchBuilder::POPAOp } },
    {"", TYPE_INVALID, FLAGS_NONE, 0, { .OpDispatch = nullptr } },
  },
  // ENTRY_63
  {
    {"ARPL",   TYPE_INVALID, FLAGS_NONE, 0, { .OpDispatch = nullptr } },
    {"MOVSXD", TYPE_INST, GenFlagsDstSize(SIZE_64BIT) | FLAGS_MODRM, 0, { .OpDispatch = &IR::OpDispatchBuilder::MOVSXDOp } },
  },
  // ENTRY_9A
  {
    {"CALLF",  TYPE_INST, FLAGS_NONE, 0, { .OpDispatch = nullptr } },
    {"", TYPE_INVALID, FLAGS_NONE, 0, { .OpDispatch = nullptr } },
  },
  // ENTRY_A0
  {
    {"MOV",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_SF_DST_RAX | FLAGS_MEM_OFFSET, 4, { .OpDispatch = &IR::OpDispatchBuilder::MOVOffsetOp } },
    {"MOV",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_SF_DST_RAX | FLAGS_MEM_OFFSET, 8, { .OpDispatch = &IR::OpDispatchBuilder::MOVOffsetOp } },
  },
  // ENTRY_A1
  {
    {"MOV",    TYPE_INST, FLAGS_SF_DST_RAX | FLAGS_MEM_OFFSET, 4, { .OpDispatch = &IR::OpDispatchBuilder::MOVOffsetOp } },
    {"MOV",    TYPE_INST, FLAGS_SF_DST_RAX | FLAGS_MEM_OFFSET, 8, { .OpDispatch = &IR::OpDispatchBuilder::MOVOffsetOp } },
  },
  // ENTRY_A2
  {
    {"MOV",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_SF_SRC_RAX | FLAGS_MEM_OFFSET, 4, { .OpDispatch = &IR::OpDispatchBuilder::MOVOffsetOp } },
    {"MOV",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_SF_SRC_RAX | FLAGS_MEM_OFFSET, 8, { .OpDispatch = &IR::OpDispatchBuilder::MOVOffsetOp } },
  },
  // ENTRY_A3
  {
    {"MOV",    TYPE_INST, FLAGS_SF_SRC_RAX | FLAGS_MEM_OFFSET, 4, { .OpDispatch = &IR::OpDispatchBuilder::MOVOffsetOp } },
    {"MOV",    TYPE_INST, FLAGS_SF_SRC_RAX | FLAGS_MEM_OFFSET, 8, { .OpDispatch = &IR::OpDispatchBuilder::MOVOffsetOp } },
  },
  // ENTRY_CE
  {
    {"INTO",   TYPE_INST, FLAGS_NONE, 0, { .OpDispatch = &IR::OpDispatchBuilder::INTOp } },
    {"", TYPE_INVALID, FLAGS_NONE, 0, { .OpDispatch = nullptr } },
  },
  // ENTRY_D4
  {
    {"AAM",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_SF_DST_RAX, 1, { .OpDispatch = &IR::OpDispatchBuilder::AAMOp } },
    {"", TYPE_INVALID, FLAGS_NONE, 0, { .OpDispatch = nullptr } },
  },
  // ENTRY_D5
  {
    {"AAD",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_SF_DST_RAX, 1, { .OpDispatch = &IR::OpDispatchBuilder::AADOp } },
    {"", TYPE_INVALID, FLAGS_NONE, 0, { .OpDispatch = nullptr } },
  },
  // ENTRY_D6
  {
    {"SALC",   TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_SF_DST_RAX | FLAGS_SF_SRC_RAX, 0, { .OpDispatch = &IR::OpDispatchBuilder::SALCOp } },
    {"", TYPE_INVALID, FLAGS_NONE, 0, { .OpDispatch = nullptr } },
  },
  // ENTRY_EA
  {
    {"JMPF",   TYPE_INST, FLAGS_NONE, 0, { .OpDispatch = nullptr } },
    {"", TYPE_INVALID, FLAGS_NONE, 0, { .OpDispatch = nullptr } },
  },
}};

const std::array<X86InstInfo, MAX_PRIMARY_TABLE_SIZE> BaseOps = []() consteval {
  std::array<X86InstInfo, MAX_PRIMARY_TABLE_SIZE> Table{};

  constexpr U8U8InfoStruct BaseOpTable[] = {
    // Prefixes
    // Operand size overide
    {0x66, 1, X86InstInfo{"",      TYPE_PREFIX, FLAGS_NONE,        0}},
    // Address size override
    {0x67, 1, X86InstInfo{"",      TYPE_PREFIX, FLAGS_NONE,        0}},
    {0x26, 1, X86InstInfo{"ES",    TYPE_LEGACY_PREFIX, FLAGS_NONE, 0}},
    {0x2E, 1, X86InstInfo{"CS",    TYPE_LEGACY_PREFIX, FLAGS_NONE, 0}},
    {0x36, 1, X86InstInfo{"SS",    TYPE_LEGACY_PREFIX, FLAGS_NONE, 0}},
    {0x3E, 1, X86InstInfo{"DS",    TYPE_LEGACY_PREFIX, FLAGS_NONE, 0}},
    // These are still invalid on 64bit
    {0x64, 1, X86InstInfo{"FS",    TYPE_PREFIX, FLAGS_NONE,        0}},
    {0x65, 1, X86InstInfo{"GS",    TYPE_PREFIX, FLAGS_NONE,        0}},
    {0xF0, 1, X86InstInfo{"LOCK",  TYPE_PREFIX, FLAGS_NONE,        0}},
    {0xF2, 1, X86InstInfo{"REPNE", TYPE_PREFIX, FLAGS_NONE,        0}},
    {0xF3, 1, X86InstInfo{"REP",   TYPE_PREFIX, FLAGS_NONE,        0}},

    // Instructions
    {0x00, 1, X86InstInfo{"ADD",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                   0}},
    {0x01, 1, X86InstInfo{"ADD",    TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_DISPLACE_SIZE_DIV_2,                                       0}},
    {0x02, 1, X86InstInfo{"ADD",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM,                                                   0}},
    {0x03, 1, X86InstInfo{"ADD",    TYPE_INST, FLAGS_MODRM,                                                                   0}},
    {0x04, 1, X86InstInfo{"ADD",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_SF_DST_RAX  ,                              1}},
    {0x05, 1, X86InstInfo{"ADD",    TYPE_INST, FLAGS_SF_DST_RAX | FLAGS_SRC_SEXT | FLAGS_DISPLACE_SIZE_DIV_2, 4}},

    {0x06, 1, X86InstInfo{"", TYPE_ARCH_DISPATCHER, FLAGS_NONE, 0, { .Indirect = Primary_ArchSelect_LUT[ENTRY_06] }}},
    {0x07, 1, X86InstInfo{"", TYPE_ARCH_DISPATCHER, FLAGS_NONE, 0, { .Indirect = Primary_ArchSelect_LUT[ENTRY_07] }}},

    {0x08, 1, X86InstInfo{"OR",     TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                   0}},
    {0x09, 1, X86InstInfo{"OR",     TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                                   0}},
    {0x0A, 1, X86InstInfo{"OR",     TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM,                                                   0}},
    {0x0B, 1, X86InstInfo{"OR",     TYPE_INST, FLAGS_MODRM,                                                                   0}},
    {0x0C, 1, X86InstInfo{"OR",     TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_SF_DST_RAX ,                              1}},
    {0x0D, 1, X86InstInfo{"OR",     TYPE_INST, FLAGS_SF_DST_RAX | FLAGS_SRC_SEXT | FLAGS_DISPLACE_SIZE_DIV_2, 4}},
    {0x0E, 1, X86InstInfo{"", TYPE_ARCH_DISPATCHER, FLAGS_NONE, 0, { .Indirect = Primary_ArchSelect_LUT[ENTRY_0E] }}},

    {0x10, 1, X86InstInfo{"ADC",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                   0}},
    {0x11, 1, X86InstInfo{"ADC",    TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_DISPLACE_SIZE_DIV_2,                                       0}},
    {0x12, 1, X86InstInfo{"ADC",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM,                                                   0}},
    {0x13, 1, X86InstInfo{"ADC",    TYPE_INST, FLAGS_MODRM,                                                                   0}},
    {0x14, 1, X86InstInfo{"ADC",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_SF_DST_RAX  ,                              1}},
    {0x15, 1, X86InstInfo{"ADC",    TYPE_INST, FLAGS_SF_DST_RAX | FLAGS_SRC_SEXT | FLAGS_DISPLACE_SIZE_DIV_2, 4}},
    {0x16, 1, X86InstInfo{"", TYPE_ARCH_DISPATCHER, FLAGS_NONE, 0, { .Indirect = Primary_ArchSelect_LUT[ENTRY_16] }}},
    {0x17, 1, X86InstInfo{"", TYPE_ARCH_DISPATCHER, FLAGS_NONE, 0, { .Indirect = Primary_ArchSelect_LUT[ENTRY_17] }}},

    {0x18, 1, X86InstInfo{"SBB",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                   0}},
    {0x19, 1, X86InstInfo{"SBB",    TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_DISPLACE_SIZE_DIV_2,                                       0}},
    {0x1A, 1, X86InstInfo{"SBB",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM,                                                   0}},
    {0x1B, 1, X86InstInfo{"SBB",    TYPE_INST, FLAGS_MODRM,                                                                   0}},
    {0x1C, 1, X86InstInfo{"SBB",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_SF_DST_RAX  ,                              1}},
    {0x1D, 1, X86InstInfo{"SBB",    TYPE_INST, FLAGS_SF_DST_RAX | FLAGS_SRC_SEXT | FLAGS_DISPLACE_SIZE_DIV_2, 4}},
    {0x1E, 1, X86InstInfo{"", TYPE_ARCH_DISPATCHER, FLAGS_NONE, 0, { .Indirect = Primary_ArchSelect_LUT[ENTRY_1E] }}},
    {0x1F, 1, X86InstInfo{"", TYPE_ARCH_DISPATCHER, FLAGS_NONE, 0, { .Indirect = Primary_ArchSelect_LUT[ENTRY_1F] }}},

    {0x20, 1, X86InstInfo{"AND",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                   0}},
    {0x21, 1, X86InstInfo{"AND",    TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                                   0}},
    {0x22, 1, X86InstInfo{"AND",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM,                                                   0}},
    {0x23, 1, X86InstInfo{"AND",    TYPE_INST, FLAGS_MODRM,                                                                   0}},
    {0x24, 1, X86InstInfo{"AND",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_SF_DST_RAX  ,                              1}},
    {0x25, 1, X86InstInfo{"AND",    TYPE_INST, FLAGS_SF_DST_RAX | FLAGS_SRC_SEXT | FLAGS_DISPLACE_SIZE_DIV_2, 4}},

    {0x27, 1, X86InstInfo{"", TYPE_ARCH_DISPATCHER, FLAGS_NONE, 0, { .Indirect = Primary_ArchSelect_LUT[ENTRY_27] }}},
    {0x28, 1, X86InstInfo{"SUB",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                   0}},
    {0x29, 1, X86InstInfo{"SUB",    TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                                   0}},
    {0x2A, 1, X86InstInfo{"SUB",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM,                                                   0}},
    {0x2B, 1, X86InstInfo{"SUB",    TYPE_INST, FLAGS_MODRM,                                                                   0}},
    {0x2C, 1, X86InstInfo{"SUB",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_SF_DST_RAX ,                              1}},
    {0x2D, 1, X86InstInfo{"SUB",    TYPE_INST, FLAGS_SF_DST_RAX | FLAGS_SRC_SEXT | FLAGS_DISPLACE_SIZE_DIV_2, 4}},
    {0x2F, 1, X86InstInfo{"", TYPE_ARCH_DISPATCHER, FLAGS_NONE, 0, { .Indirect = Primary_ArchSelect_LUT[ENTRY_2F] }}},

    {0x30, 1, X86InstInfo{"XOR",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                   0}},
    {0x31, 1, X86InstInfo{"XOR",    TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                                   0}},
    {0x32, 1, X86InstInfo{"XOR",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM,                                                   0}},
    {0x33, 1, X86InstInfo{"XOR",    TYPE_INST, FLAGS_MODRM,                                                                   0}},
    {0x34, 1, X86InstInfo{"XOR",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_SF_DST_RAX  ,                              1}},
    {0x35, 1, X86InstInfo{"XOR",    TYPE_INST, FLAGS_SF_DST_RAX | FLAGS_SRC_SEXT | FLAGS_DISPLACE_SIZE_DIV_2, 4}},

    {0x37, 1, X86InstInfo{"", TYPE_ARCH_DISPATCHER, FLAGS_NONE, 0, { .Indirect = Primary_ArchSelect_LUT[ENTRY_37] }}},
    {0x38, 1, X86InstInfo{"CMP",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                   0}},
    {0x39, 1, X86InstInfo{"CMP",    TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                                   0}},
    {0x3A, 1, X86InstInfo{"CMP",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM,                                                   0}},
    {0x3B, 1, X86InstInfo{"CMP",    TYPE_INST, FLAGS_MODRM,                                                                   0}},
    {0x3C, 1, X86InstInfo{"CMP",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_SF_DST_RAX  ,                              1}},
    {0x3D, 1, X86InstInfo{"CMP",    TYPE_INST, FLAGS_SF_DST_RAX | FLAGS_SRC_SEXT | FLAGS_DISPLACE_SIZE_DIV_2, 4}},
    {0x3F, 1, X86InstInfo{"", TYPE_ARCH_DISPATCHER, FLAGS_NONE, 0, { .Indirect = Primary_ArchSelect_LUT[ENTRY_3F] }}},

    {0x40, 8, X86InstInfo{"", TYPE_ARCH_DISPATCHER, FLAGS_NONE, 0, { .Indirect = Primary_ArchSelect_LUT[ENTRY_40] }}},
    {0x48, 8, X86InstInfo{"", TYPE_ARCH_DISPATCHER, FLAGS_NONE, 0, { .Indirect = Primary_ArchSelect_LUT[ENTRY_48] }}},

    {0x50, 8, X86InstInfo{"PUSH",   TYPE_INST, GenFlagsSameSize(SIZE_64BITDEF) | FLAGS_SF_REX_IN_BYTE | FLAGS_DEBUG_MEM_ACCESS ,                    0}},
    {0x58, 8, X86InstInfo{"POP",    TYPE_INST, GenFlagsSameSize(SIZE_64BITDEF) | FLAGS_SF_REX_IN_BYTE | FLAGS_DEBUG_MEM_ACCESS ,                    0}},


    {0x60, 1, X86InstInfo{"", TYPE_ARCH_DISPATCHER, FLAGS_NONE, 0, { .Indirect = Primary_ArchSelect_LUT[ENTRY_60] }}},
    {0x61, 1, X86InstInfo{"", TYPE_ARCH_DISPATCHER, FLAGS_NONE, 0, { .Indirect = Primary_ArchSelect_LUT[ENTRY_61] }}},
    {0x62, 1, X86InstInfo{"",       TYPE_GROUP_EVEX, FLAGS_NONE,                                                                           0}},
    {0x63, 1, X86InstInfo{"", TYPE_ARCH_DISPATCHER, FLAGS_NONE, 0, { .Indirect = Primary_ArchSelect_LUT[ENTRY_63] }}},

    {0x68, 1, X86InstInfo{"PUSH",   TYPE_INST, GenFlagsSameSize(SIZE_64BITDEF) | FLAGS_DEBUG_MEM_ACCESS | FLAGS_DISPLACE_SIZE_DIV_2 | FLAGS_SRC_SEXT, 4}},
    {0x69, 1, X86InstInfo{"IMUL",   TYPE_INST, FLAGS_MODRM | FLAGS_SRC_SEXT | FLAGS_DISPLACE_SIZE_DIV_2,        4}},
    {0x6A, 1, X86InstInfo{"PUSH",   TYPE_INST, GenFlagsSameSize(SIZE_64BITDEF) | FLAGS_DEBUG_MEM_ACCESS | FLAGS_SRC_SEXT ,            1}},
    {0x6B, 1, X86InstInfo{"IMUL",   TYPE_INST, FLAGS_MODRM | FLAGS_SRC_SEXT ,                                    1}},

    // This should just throw a GP
    {0x6C, 1, X86InstInfo{"INSB",   TYPE_INST, FLAGS_BLOCK_END, 0}},
    {0x6D, 1, X86InstInfo{"INSW",   TYPE_INST, FLAGS_BLOCK_END, 0}},
    {0x6E, 1, X86InstInfo{"OUTS",   TYPE_INST, FLAGS_BLOCK_END, 0}},
    {0x6F, 1, X86InstInfo{"OUTS",   TYPE_INST, FLAGS_BLOCK_END, 0}},

    {0x70, 1, X86InstInfo{"JO",     TYPE_INST, GenFlagsSameSize(SIZE_64BITDEF) | FLAGS_SETS_RIP | FLAGS_SRC_SEXT , 1}},
    {0x71, 1, X86InstInfo{"JNO",    TYPE_INST, GenFlagsSameSize(SIZE_64BITDEF) | FLAGS_SETS_RIP | FLAGS_SRC_SEXT , 1}},
    {0x72, 1, X86InstInfo{"JB",     TYPE_INST, GenFlagsSameSize(SIZE_64BITDEF) | FLAGS_SETS_RIP | FLAGS_SRC_SEXT , 1}},
    {0x73, 1, X86InstInfo{"JNB",    TYPE_INST, GenFlagsSameSize(SIZE_64BITDEF) | FLAGS_SETS_RIP | FLAGS_SRC_SEXT , 1}},
    {0x74, 1, X86InstInfo{"JZ",     TYPE_INST, GenFlagsSameSize(SIZE_64BITDEF) | FLAGS_SETS_RIP | FLAGS_SRC_SEXT , 1}},
    {0x75, 1, X86InstInfo{"JNZ",    TYPE_INST, GenFlagsSameSize(SIZE_64BITDEF) | FLAGS_SETS_RIP | FLAGS_SRC_SEXT , 1}},
    {0x76, 1, X86InstInfo{"JBE",    TYPE_INST, GenFlagsSameSize(SIZE_64BITDEF) | FLAGS_SETS_RIP | FLAGS_SRC_SEXT , 1}},
    {0x77, 1, X86InstInfo{"JNBE",   TYPE_INST, GenFlagsSameSize(SIZE_64BITDEF) | FLAGS_SETS_RIP | FLAGS_SRC_SEXT , 1}},
    {0x78, 1, X86InstInfo{"JS",     TYPE_INST, GenFlagsSameSize(SIZE_64BITDEF) | FLAGS_SETS_RIP | FLAGS_SRC_SEXT , 1}},
    {0x79, 1, X86InstInfo{"JNS",    TYPE_INST, GenFlagsSameSize(SIZE_64BITDEF) | FLAGS_SETS_RIP | FLAGS_SRC_SEXT , 1}},
    {0x7A, 1, X86InstInfo{"JP",     TYPE_INST, GenFlagsSameSize(SIZE_64BITDEF) | FLAGS_SETS_RIP | FLAGS_SRC_SEXT , 1}},
    {0x7B, 1, X86InstInfo{"JNP",    TYPE_INST, GenFlagsSameSize(SIZE_64BITDEF) | FLAGS_SETS_RIP | FLAGS_SRC_SEXT , 1}},
    {0x7C, 1, X86InstInfo{"JL",     TYPE_INST, GenFlagsSameSize(SIZE_64BITDEF) | FLAGS_SETS_RIP | FLAGS_SRC_SEXT , 1}},
    {0x7D, 1, X86InstInfo{"JNL",    TYPE_INST, GenFlagsSameSize(SIZE_64BITDEF) | FLAGS_SETS_RIP | FLAGS_SRC_SEXT , 1}},
    {0x7E, 1, X86InstInfo{"JLE",    TYPE_INST, GenFlagsSameSize(SIZE_64BITDEF) | FLAGS_SETS_RIP | FLAGS_SRC_SEXT , 1}},
    {0x7F, 1, X86InstInfo{"JNLE",   TYPE_INST, GenFlagsSameSize(SIZE_64BITDEF) | FLAGS_SETS_RIP | FLAGS_SRC_SEXT , 1}},

    {0x84, 1, X86InstInfo{"TEST",   TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,         0}},
    {0x85, 1, X86InstInfo{"TEST",   TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                         0}},
    {0x86, 1, X86InstInfo{"XCHG",   TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,         0}},
    {0x87, 1, X86InstInfo{"XCHG",   TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                         0}},

    {0x88, 1, X86InstInfo{"MOV",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,         0}},
    {0x89, 1, X86InstInfo{"MOV",    TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                         0}},
    {0x8A, 1, X86InstInfo{"MOV",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM,         0}},
    {0x8B, 1, X86InstInfo{"MOV",    TYPE_INST, FLAGS_MODRM,                         0}},
    {0x8C, 1, X86InstInfo{"MOV",    TYPE_INST, GenFlagsSrcSize(SIZE_16BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                      0}},
    {0x8D, 1, X86InstInfo{"LEA",    TYPE_INST, GenFlagsSameSize(SIZE_64BITDEF) | FLAGS_MODRM,                         0}},
    {0x8E, 1, X86InstInfo{"MOV",    TYPE_INST, GenFlagsSameSize(SIZE_16BIT) | FLAGS_MODRM,                      0}},
    {0x8F, 1, X86InstInfo{"POP",    TYPE_INST, GenFlagsSameSize(SIZE_64BITDEF) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_MOD_ZERO_REG | FLAGS_DEBUG_MEM_ACCESS, 0}},
    {0x90, 8, X86InstInfo{"XCHG",   TYPE_INST, FLAGS_SF_REX_IN_BYTE | FLAGS_SF_SRC_RAX, 0}},
    {0x98, 1, X86InstInfo{"CDQE",   TYPE_INST, FLAGS_SF_DST_RAX | FLAGS_SF_SRC_RAX,     0}},
    {0x99, 1, X86InstInfo{"CQO",    TYPE_INST, FLAGS_SF_DST_RDX | FLAGS_SF_SRC_RAX,     0}},

    // These three are all X87 instructions
    {0x9B, 1, X86InstInfo{"FWAIT",  TYPE_INST, FLAGS_NONE,                              0}},
    {0x9C, 1, X86InstInfo{"PUSHF",  TYPE_INST, GenFlagsSameSize(SIZE_64BITDEF),         0}},
    {0x9D, 1, X86InstInfo{"POPF",   TYPE_INST, GenFlagsSameSize(SIZE_64BITDEF) | FLAGS_BLOCK_END,         0}},

    {0x9E, 1, X86InstInfo{"SAHF",   TYPE_INST, FLAGS_NONE,                              0}},
    {0x9F, 1, X86InstInfo{"LAHF",   TYPE_INST, FLAGS_NONE,                              0}},

    {0xA0, 1, X86InstInfo{"", TYPE_ARCH_DISPATCHER, FLAGS_NONE, 0, { .Indirect = Primary_ArchSelect_LUT[ENTRY_A0] }}},
    {0xA1, 1, X86InstInfo{"", TYPE_ARCH_DISPATCHER, FLAGS_NONE, 0, { .Indirect = Primary_ArchSelect_LUT[ENTRY_A1] }}},
    {0xA2, 1, X86InstInfo{"", TYPE_ARCH_DISPATCHER, FLAGS_NONE, 0, { .Indirect = Primary_ArchSelect_LUT[ENTRY_A2] }}},
    {0xA3, 1, X86InstInfo{"", TYPE_ARCH_DISPATCHER, FLAGS_NONE, 0, { .Indirect = Primary_ArchSelect_LUT[ENTRY_A3] }}},

    {0xA4, 1, X86InstInfo{"MOVSB",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_DEBUG_MEM_ACCESS,                                            0}},
    {0xA5, 1, X86InstInfo{"MOVS",   TYPE_INST, FLAGS_DEBUG_MEM_ACCESS,                                                            0}},
    {0xA6, 1, X86InstInfo{"CMPSB",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_DEBUG_MEM_ACCESS,                                            0}},
    {0xA7, 1, X86InstInfo{"CMPS",   TYPE_INST, FLAGS_DEBUG_MEM_ACCESS,                                                            0}},

    {0xA8, 1, X86InstInfo{"TEST",   TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_SF_DST_RAX ,                                             1}},
    {0xA9, 1, X86InstInfo{"TEST",   TYPE_INST, FLAGS_SF_DST_RAX | FLAGS_SRC_SEXT | FLAGS_DISPLACE_SIZE_DIV_2,                4}},
    {0xAA, 1, X86InstInfo{"STOS",   TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_DEBUG_MEM_ACCESS | FLAGS_SF_SRC_RAX,                   0}},
    {0xAB, 1, X86InstInfo{"STOS",   TYPE_INST, FLAGS_DEBUG_MEM_ACCESS | FLAGS_SF_SRC_RAX,                                   0}},
    {0xAC, 1, X86InstInfo{"LODS",   TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_SF_DST_RAX | FLAGS_DEBUG_MEM_ACCESS,                                                      0}},
    {0xAD, 1, X86InstInfo{"LODS",   TYPE_INST, FLAGS_SF_DST_RAX | FLAGS_DEBUG_MEM_ACCESS,                                                      0}},
    {0xAE, 1, X86InstInfo{"SCAS",   TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_DEBUG_MEM_ACCESS | FLAGS_SF_SRC_RAX,                                   0}},
    {0xAF, 1, X86InstInfo{"SCAS",   TYPE_INST, FLAGS_DEBUG_MEM_ACCESS | FLAGS_SF_SRC_RAX,                                   0}},

    {0xB0, 8, X86InstInfo{"MOV",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_SF_REX_IN_BYTE ,                                         1}},
    {0xB8, 8, X86InstInfo{"MOV",    TYPE_INST, FLAGS_SF_REX_IN_BYTE | FLAGS_DISPLACE_SIZE_DIV_2 | FLAGS_DISPLACE_SIZE_MUL_2, 4}},

    {0xC2, 1, X86InstInfo{"RET",    TYPE_INST, FLAGS_SETS_RIP | FLAGS_BLOCK_END,                                             2}},
    {0xC3, 1, X86InstInfo{"RET",    TYPE_INST, GenFlagsSameSize(SIZE_64BITDEF) | FLAGS_SETS_RIP | FLAGS_BLOCK_END ,                                                0}},
    {0xC8, 1, X86InstInfo{"ENTER",  TYPE_INST, GenFlagsSameSize(SIZE_64BITDEF) | FLAGS_DEBUG_MEM_ACCESS ,                                      3}},
    {0xC9, 1, X86InstInfo{"LEAVE",  TYPE_INST, GenFlagsSameSize(SIZE_64BITDEF) | FLAGS_DEBUG_MEM_ACCESS ,                                                0}},
    {0xCA, 1, X86InstInfo{"RETF",   TYPE_INST, GenFlagsSameSize(SIZE_64BITDEF) | FLAGS_SETS_RIP | FLAGS_BLOCK_END,                                                              2}},
    {0xCB, 1, X86InstInfo{"RETF",   TYPE_INST, GenFlagsSameSize(SIZE_64BITDEF) | FLAGS_SETS_RIP | FLAGS_BLOCK_END,                                                              0}},
    {0xCC, 1, X86InstInfo{"INT3",   TYPE_INST, FLAGS_BLOCK_END,                                                                                      0}},
    {0xCD, 1, X86InstInfo{"INT",    TYPE_INST, DEFAULT_SYSCALL_FLAGS,                                                                  1}},
    {0xCE, 1, X86InstInfo{"", TYPE_ARCH_DISPATCHER, FLAGS_NONE, 0, { .Indirect = Primary_ArchSelect_LUT[ENTRY_CE] }}},
    {0xCF, 1, X86InstInfo{"IRET",   TYPE_INST, FLAGS_SETS_RIP | FLAGS_BLOCK_END,                                                                                    0}},

    {0xD4, 1, X86InstInfo{"", TYPE_ARCH_DISPATCHER, FLAGS_NONE, 0, { .Indirect = Primary_ArchSelect_LUT[ENTRY_D4] }}},
    {0xD5, 1, X86InstInfo{"", TYPE_ARCH_DISPATCHER, FLAGS_NONE, 0, { .Indirect = Primary_ArchSelect_LUT[ENTRY_D5] }}},
    {0xD6, 1, X86InstInfo{"", TYPE_ARCH_DISPATCHER, FLAGS_NONE, 0, { .Indirect = Primary_ArchSelect_LUT[ENTRY_D6] }}},
    {0xD7, 1, X86InstInfo{"XLAT",   TYPE_INST, FLAGS_DEBUG_MEM_ACCESS,                                                                           0}},

    {0xE0, 1, X86InstInfo{"LOOPNE", TYPE_INST, GenFlagsSameSize(SIZE_64BITDEF) | FLAGS_SETS_RIP | FLAGS_SRC_SEXT | FLAGS_SF_SRC_RCX,                             1}},
    {0xE1, 1, X86InstInfo{"LOOPE",  TYPE_INST, GenFlagsSameSize(SIZE_64BITDEF) | FLAGS_SETS_RIP | FLAGS_SRC_SEXT | FLAGS_SF_SRC_RCX,                             1}},
    {0xE2, 1, X86InstInfo{"LOOP",   TYPE_INST, GenFlagsSameSize(SIZE_64BITDEF) | FLAGS_SETS_RIP | FLAGS_SRC_SEXT | FLAGS_SF_SRC_RCX,                             1}},
    {0xE3, 1, X86InstInfo{"JrCXZ",  TYPE_INST, GenFlagsSameSize(SIZE_64BITDEF) | FLAGS_SETS_RIP | FLAGS_SRC_SEXT ,                             1}},

    // Should just throw GP
    {0xE4, 2, X86InstInfo{"IN",     TYPE_INST, FLAGS_BLOCK_END,                                                                                                      1}},
    {0xE6, 2, X86InstInfo{"OUT",    TYPE_INST, FLAGS_BLOCK_END,                                                                                                      1}},

    {0xE8, 1, X86InstInfo{"CALL",   TYPE_INST, GenFlagsSameSize(SIZE_64BITDEF) | FLAGS_SETS_RIP | FLAGS_SRC_SEXT | FLAGS_DISPLACE_SIZE_DIV_2 | FLAGS_BLOCK_END | FLAGS_CALL , 4}},
    {0xE9, 1, X86InstInfo{"JMP",    TYPE_INST, GenFlagsSameSize(SIZE_64BITDEF) | FLAGS_SETS_RIP | FLAGS_SRC_SEXT | FLAGS_DISPLACE_SIZE_DIV_2 | FLAGS_BLOCK_END , 4}},
    {0xEB, 1, X86InstInfo{"JMP",    TYPE_INST, GenFlagsSameSize(SIZE_64BITDEF) | FLAGS_SETS_RIP | FLAGS_SRC_SEXT | FLAGS_BLOCK_END ,                             1}},

    // Should just throw GP
    {0xEC, 2, X86InstInfo{"IN",     TYPE_INST, FLAGS_BLOCK_END,             0}},
    {0xEE, 2, X86InstInfo{"OUT",    TYPE_INST, FLAGS_BLOCK_END,             0}},

    {0xF1, 1, X86InstInfo{"INT1",   TYPE_INST, FLAGS_BLOCK_END,               0}},
    {0xF4, 1, X86InstInfo{"HLT",    TYPE_INST, FLAGS_BLOCK_END,               0}},
    {0xF5, 1, X86InstInfo{"CMC",    TYPE_INST, FLAGS_NONE,                0}},
    {0xF8, 1, X86InstInfo{"CLC",    TYPE_INST, FLAGS_NONE,                0}},
    {0xF9, 1, X86InstInfo{"STC",    TYPE_INST, FLAGS_NONE,                0}},
    {0xFA, 1, X86InstInfo{"CLI",    TYPE_INST, FLAGS_NONE,                0}},
    {0xFB, 1, X86InstInfo{"STI",    TYPE_INST, FLAGS_NONE,                0}},
    {0xFC, 1, X86InstInfo{"CLD",    TYPE_INST, FLAGS_NONE,                0}},
    {0xFD, 1, X86InstInfo{"STD",    TYPE_INST, FLAGS_NONE,                0}},

    // Two Byte table
    {0x0F, 1, X86InstInfo{"",   TYPE_SECONDARY_TABLE_PREFIX, FLAGS_NONE,  0}},

    // x87 table
    {0xD8, 8, X86InstInfo{"",   TYPE_X87_TABLE_PREFIX, FLAGS_MODRM,        0}},

    // ModRM table
    // MoreBytes field repurposed for valid bits mask
    {0x80, 1, X86InstInfo{"",   TYPE_GROUP_1, FLAGS_MODRM, 0}},
    {0x81, 1, X86InstInfo{"",   TYPE_GROUP_1, FLAGS_MODRM, 1}},
    {0x82, 1, X86InstInfo{"",   TYPE_GROUP_1, FLAGS_MODRM, 2}},
    {0x83, 1, X86InstInfo{"",   TYPE_GROUP_1, FLAGS_MODRM, 3}},
    {0xC0, 1, X86InstInfo{"",   TYPE_GROUP_2, FLAGS_MODRM, 0}},
    {0xC1, 1, X86InstInfo{"",   TYPE_GROUP_2, FLAGS_MODRM, 1}},
    {0xD0, 1, X86InstInfo{"",   TYPE_GROUP_2, FLAGS_MODRM, 2}},
    {0xD1, 1, X86InstInfo{"",   TYPE_GROUP_2, FLAGS_MODRM, 3}},
    {0xD2, 1, X86InstInfo{"",   TYPE_GROUP_2, FLAGS_MODRM, 4}},
    {0xD3, 1, X86InstInfo{"",   TYPE_GROUP_2, FLAGS_MODRM, 5}},
    {0xF6, 1, X86InstInfo{"",   TYPE_GROUP_3, FLAGS_MODRM, 0}},
    {0xF7, 1, X86InstInfo{"",   TYPE_GROUP_3, FLAGS_MODRM, 1}},
    {0xFE, 1, X86InstInfo{"",   TYPE_GROUP_4, FLAGS_MODRM, 0}},
    {0xFF, 1, X86InstInfo{"",   TYPE_GROUP_5, FLAGS_MODRM, 0}},

    // Group 11
    {0xC6, 1, X86InstInfo{"",   TYPE_GROUP_11, FLAGS_MODRM, 0}},
    {0xC7, 1, X86InstInfo{"",   TYPE_GROUP_11, FLAGS_MODRM, 1}},

    // VEX table
    {0xC4, 2, X86InstInfo{"",   TYPE_VEX_TABLE_PREFIX, FLAGS_NONE, 0}},
  };

  GenerateTable(Table.data(), BaseOpTable, std::size(BaseOpTable));
  IR::InstallToTable(Table, IR::OpDispatch_BaseOpTable);

  return Table;
}();

}

