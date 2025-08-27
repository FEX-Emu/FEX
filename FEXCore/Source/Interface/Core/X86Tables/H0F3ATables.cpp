// SPDX-License-Identifier: MIT
/*
$info$
tags: frontend|x86-tables
$end_info$
*/

#include "Interface/Core/X86Tables/X86Tables.h"
#include "Interface/Core/OpcodeDispatcher/H0F3ATables.h"

#include <FEXCore/Core/Context.h>

#include <iterator>
#include <stdint.h>

namespace FEXCore::X86Tables {
using namespace InstFlags;
#define OPD(REX, prefix, opcode) ((REX << 9) | (prefix << 8) | opcode)
constexpr uint16_t PF_3A_NONE = 0;
constexpr uint16_t PF_3A_66   = 1;

enum H0F3A_LUT {
  ENTRY_1_3A_66_16,
  ENTRY_1_3A_66_22,
  ENTRY_MAX,
};

constexpr std::array<X86InstInfo[2], ENTRY_MAX> H0F3A_ArchSelect_LUT = {{
  // ENTRY_1_3A_66_16
  {
    {"", TYPE_INVALID, FLAGS_NONE, 0, { .OpDispatch = nullptr } },
    {"PEXTRQ",          TYPE_INST, GenFlagsSizes(SIZE_64BIT, SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_DST_GPR | FLAGS_XMM_FLAGS, 1, { .OpDispatch = &IR::OpDispatchBuilder::Bind<&IR::OpDispatchBuilder::PExtrOp, IR::OpSize::i64Bit> }},
  },
  // ENTRY_1_3A_66_22
  {
    {"", TYPE_INVALID, FLAGS_NONE, 0, { .OpDispatch = nullptr } },
    {"PINSRQ",          TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_SRC_GPR, 1, { .OpDispatch = &IR::OpDispatchBuilder::PINSROp<IR::OpSize::i64Bit> }},
  },
}};

constexpr std::array<X86InstInfo, MAX_0F_3A_TABLE_SIZE> H0F3ATableOps = []() consteval {
  std::array<X86InstInfo, MAX_0F_3A_TABLE_SIZE> Table{};
  auto TableGen = []<uint16_t REX>() consteval {
    constexpr U16U8InfoStruct Table[] = {
      {OPD(REX, PF_3A_NONE, 0x0F), 1, X86InstInfo{"PALIGNR",         TYPE_INST, GenFlagsSameSize(SIZE_64BIT)  | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX, 1}},
      {OPD(REX, PF_3A_66,   0x08), 1, X86InstInfo{"ROUNDPS",         TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 1}},
      {OPD(REX, PF_3A_66,   0x09), 1, X86InstInfo{"ROUNDPD",         TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 1}},
      {OPD(REX, PF_3A_66,   0x0A), 1, X86InstInfo{"ROUNDSS",         TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_32BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 1}},
      {OPD(REX, PF_3A_66,   0x0B), 1, X86InstInfo{"ROUNDSD",         TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 1}},
      {OPD(REX, PF_3A_66,   0x0C), 1, X86InstInfo{"BLENDPS",         TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,           1}},
      {OPD(REX, PF_3A_66,   0x0D), 1, X86InstInfo{"BLENDPD",         TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,           1}},
      {OPD(REX, PF_3A_66,   0x0E), 1, X86InstInfo{"PBLENDW",         TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,           1}},
      {OPD(REX, PF_3A_66,   0x0F), 1, X86InstInfo{"PALIGNR",         TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 1}},

      {OPD(REX, PF_3A_66,   0x14), 1, X86InstInfo{"PEXTRB",          TYPE_INST, GenFlagsSizes(SIZE_32BIT, SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_DST_GPR | FLAGS_XMM_FLAGS, 1}},
      {OPD(REX, PF_3A_66,   0x15), 1, X86InstInfo{"PEXTRW",          TYPE_INST, GenFlagsSizes(SIZE_16BIT, SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_DST_GPR | FLAGS_XMM_FLAGS, 1}},
      {OPD(REX, PF_3A_66,   0x17), 1, X86InstInfo{"EXTRACTPS",       TYPE_INST, GenFlagsSizes(SIZE_32BIT, SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_DST_GPR | FLAGS_XMM_FLAGS, 1}},

      {OPD(REX, PF_3A_66,   0x20), 1, X86InstInfo{"PINSRB",          TYPE_INST, GenFlagsDstSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_SRC_GPR,           1}},
      {OPD(REX, PF_3A_66,   0x21), 1, X86InstInfo{"INSERTPS",        TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,           1}},
      {OPD(REX, PF_3A_66,   0x40), 1, X86InstInfo{"DPPS",            TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 1}},
      {OPD(REX, PF_3A_66,   0x41), 1, X86InstInfo{"DPPD",            TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 1}},
      {OPD(REX, PF_3A_66,   0x42), 1, X86InstInfo{"MPSADBW",         TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 1}},
      {OPD(REX, PF_3A_66,   0x44), 1, X86InstInfo{"PCLMULQDQ",       TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 1}},

      {OPD(REX, PF_3A_66,   0x60), 1, X86InstInfo{"PCMPESTRM",       TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_32BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 1}},
      {OPD(REX, PF_3A_66,   0x61), 1, X86InstInfo{"PCMPESTRI",       TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_32BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 1}},
      {OPD(REX, PF_3A_66,   0x62), 1, X86InstInfo{"PCMPISTRM",       TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 1}},
      {OPD(REX, PF_3A_66,   0x63), 1, X86InstInfo{"PCMPISTRI",       TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 1}},

      {OPD(REX, PF_3A_NONE, 0xCC), 1, X86InstInfo{"SHA1RNDS4",       TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 1}},

      {OPD(REX, PF_3A_66,   0xDF), 1, X86InstInfo{"AESKEYGENASSIST", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 1}},
    };
    return std::to_array(Table);
  };
  constexpr auto H0F3ATable_IgnoresREX0 = TableGen.template operator()<0>();
  constexpr auto H0F3ATable_IgnoresREX1 = TableGen.template operator()<1>();

  GenerateTable(&Table.at(0), &H0F3ATable_IgnoresREX0.at(0), H0F3ATable_IgnoresREX0.size());
  GenerateTable(&Table.at(0), &H0F3ATable_IgnoresREX1.at(0), H0F3ATable_IgnoresREX1.size());

  constexpr U16U8InfoStruct TableNeedsREX0[] = {
    {OPD(0, PF_3A_66,   0x16), 1, X86InstInfo{"PEXTRD",          TYPE_INST, GenFlagsSizes(SIZE_32BIT, SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_DST_GPR | FLAGS_XMM_FLAGS, 1}},
    {OPD(0, PF_3A_66,   0x22), 1, X86InstInfo{"PINSRD",          TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_32BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_SRC_GPR,           1}},
  };
  GenerateTable(&Table.at(0), TableNeedsREX0, std::size(TableNeedsREX0));

  constexpr U16U8InfoStruct TableNeedsREX1[] = {
    {OPD(1, PF_3A_66,   0x16), 1, X86InstInfo{"", TYPE_ARCH_DISPATCHER, FLAGS_NONE, 0, { .Indirect = H0F3A_ArchSelect_LUT[ENTRY_1_3A_66_16] }}},
    {OPD(1, PF_3A_66,   0x22), 1, X86InstInfo{"", TYPE_ARCH_DISPATCHER, FLAGS_NONE, 0, { .Indirect = H0F3A_ArchSelect_LUT[ENTRY_1_3A_66_22] }}},
  };
  GenerateTable(&Table.at(0), TableNeedsREX1, std::size(TableNeedsREX1));

  IR::InstallToTable(Table, IR::OpDispatch_H0F3ATableIgnoreREX);
  IR::InstallToTable(Table, IR::OpDispatch_H0F3ATableNeedsREX0);

  return Table;
}();
}
