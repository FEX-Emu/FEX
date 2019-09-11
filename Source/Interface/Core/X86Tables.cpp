#include "LogManager.h"

#include <FEXCore/Debug/X86Tables.h>
#include <array>
#include <cstdint>
#include <tuple>
#include <vector>

namespace FEXCore::X86Tables {

std::array<X86InstInfo, MAX_PRIMARY_TABLE_SIZE> BaseOps;
std::array<X86InstInfo, MAX_SECOND_TABLE_SIZE> SecondBaseOps;

std::array<X86InstInfo, MAX_REP_MOD_TABLE_SIZE> RepModOps;
std::array<X86InstInfo, MAX_REPNE_MOD_TABLE_SIZE> RepNEModOps;
std::array<X86InstInfo, MAX_OPSIZE_MOD_TABLE_SIZE> OpSizeModOps;
std::array<X86InstInfo, MAX_INST_GROUP_TABLE_SIZE> PrimaryInstGroupOps;
std::array<X86InstInfo, MAX_INST_SECOND_GROUP_TABLE_SIZE> SecondInstGroupOps;
std::array<X86InstInfo, MAX_SECOND_MODRM_TABLE_SIZE> SecondModRMTableOps;
std::array<X86InstInfo, MAX_X87_TABLE_SIZE> X87Ops;
std::array<X86InstInfo, MAX_3DNOW_TABLE_SIZE> DDDNowOps;
std::array<X86InstInfo, MAX_0F_38_TABLE_SIZE> H0F38TableOps;
std::array<X86InstInfo, MAX_0F_3A_TABLE_SIZE> H0F3ATableOps;
std::array<X86InstInfo, MAX_VEX_TABLE_SIZE> VEXTableOps;
std::array<X86InstInfo, MAX_VEX_GROUP_TABLE_SIZE> VEXTableGroupOps;
std::array<X86InstInfo, MAX_XOP_TABLE_SIZE> XOPTableOps;
std::array<X86InstInfo, MAX_XOP_GROUP_TABLE_SIZE> XOPTableGroupOps;

void InitializeInfoTables() {
  using namespace FEXCore::X86Tables::InstFlags;
  auto UnknownOp = X86InstInfo{"UND", TYPE_UNKNOWN, FLAGS_NONE, 0, nullptr};

  for (auto &BaseOp : BaseOps)
      BaseOp = UnknownOp;
  for (auto &BaseOp : SecondBaseOps)
      BaseOp = UnknownOp;
  for (auto &BaseOp : RepModOps)
      BaseOp = UnknownOp;
  for (auto &BaseOp : RepNEModOps)
      BaseOp = UnknownOp;
  for (auto &BaseOp : OpSizeModOps)
      BaseOp = UnknownOp;
  for (auto &BaseOp : PrimaryInstGroupOps)
      BaseOp = UnknownOp;
  for (auto &BaseOp : SecondInstGroupOps)
      BaseOp = UnknownOp;
  for (auto &BaseOp : SecondModRMTableOps)
      BaseOp = UnknownOp;
  for (auto &BaseOp : X87Ops)
      BaseOp = UnknownOp;
  for (auto &BaseOp : DDDNowOps)
      BaseOp = UnknownOp;
  for (auto &BaseOp : H0F38TableOps)
      BaseOp = UnknownOp;
  for (auto &BaseOp : H0F3ATableOps)
      BaseOp = UnknownOp;
  for (auto &BaseOp : VEXTableOps)
      BaseOp = UnknownOp;
  for (auto &BaseOp : VEXTableGroupOps)
      BaseOp = UnknownOp;
  for (auto &BaseOp : XOPTableOps)
      BaseOp = UnknownOp;
  for (auto &BaseOp : XOPTableGroupOps)
      BaseOp = UnknownOp;

  const std::vector<std::tuple<uint8_t, uint8_t, X86InstInfo>> BaseOpTable = {
    // Prefixes
    // Operand size overide
    {0x66, 1, X86InstInfo{"",      TYPE_LEGACY_PREFIX, FLAGS_NONE, 0, nullptr}},
    // Address size override
    {0x67, 1, X86InstInfo{"",      TYPE_LEGACY_PREFIX, FLAGS_NONE, 0, nullptr}},
    {0x2E, 1, X86InstInfo{"CS",    TYPE_LEGACY_PREFIX, FLAGS_NONE, 0, nullptr}},
    {0x3E, 1, X86InstInfo{"DS",    TYPE_LEGACY_PREFIX, FLAGS_NONE, 0, nullptr}},
    {0x26, 1, X86InstInfo{"ES",    TYPE_LEGACY_PREFIX, FLAGS_NONE, 0, nullptr}},
    // These are still invalid on 64bit
    {0x64, 1, X86InstInfo{"FS",    TYPE_PREFIX, FLAGS_NONE,        0, nullptr}},
    {0x65, 1, X86InstInfo{"GS",    TYPE_PREFIX, FLAGS_NONE,        0, nullptr}},
    {0x36, 1, X86InstInfo{"SS",    TYPE_LEGACY_PREFIX, FLAGS_NONE, 0, nullptr}},
    {0xF0, 1, X86InstInfo{"LOCK",  TYPE_LEGACY_PREFIX, FLAGS_NONE, 0, nullptr}},
    {0xF2, 1, X86InstInfo{"REPNE", TYPE_LEGACY_PREFIX, FLAGS_NONE, 0, nullptr}},
    {0xF3, 1, X86InstInfo{"REP",   TYPE_LEGACY_PREFIX, FLAGS_NONE, 0, nullptr}},

    // REX
    {0x40, 16, X86InstInfo{"", TYPE_REX_PREFIX, FLAGS_NONE,        0, nullptr}},

    // Instructions
    {0x00, 1, X86InstInfo{"ADD",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                   0, nullptr}},
    {0x01, 1, X86InstInfo{"ADD",    TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_DISPLACE_SIZE_DIV_2,                                       0, nullptr}},
    {0x02, 1, X86InstInfo{"ADD",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM,                                                   0, nullptr}},
    {0x03, 1, X86InstInfo{"ADD",    TYPE_INST, FLAGS_MODRM,                                                                   0, nullptr}},
    {0x04, 1, X86InstInfo{"ADD",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_SF_DST_RAX  ,                              1, nullptr}},
    {0x05, 1, X86InstInfo{"ADD",    TYPE_INST, FLAGS_SF_DST_RAX | FLAGS_SRC_SEXT | FLAGS_DISPLACE_SIZE_DIV_2, 4, nullptr}},

    {0x06, 2, X86InstInfo{"[INV]",  TYPE_INVALID, FLAGS_NONE,                                                                     0, nullptr}},
    {0x08, 1, X86InstInfo{"OR",     TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                   0, nullptr}},
    {0x09, 1, X86InstInfo{"OR",     TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                                   0, nullptr}},
    {0x0A, 1, X86InstInfo{"OR",     TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM,                                                   0, nullptr}},
    {0x0B, 1, X86InstInfo{"OR",     TYPE_INST, FLAGS_MODRM,                                                                   0, nullptr}},
    {0x0C, 1, X86InstInfo{"OR",     TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_SF_DST_RAX ,                              1, nullptr}},
    {0x0D, 1, X86InstInfo{"OR",     TYPE_INST, FLAGS_SF_DST_RAX | FLAGS_SRC_SEXT | FLAGS_DISPLACE_SIZE_DIV_2, 4, nullptr}},

    {0x0E, 1, X86InstInfo{"[INV]",  TYPE_INVALID, FLAGS_NONE,                                                                     0, nullptr}},
    {0x10, 1, X86InstInfo{"ADC",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                   0, nullptr}},
    {0x11, 1, X86InstInfo{"ADC",    TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_DISPLACE_SIZE_DIV_2,                                       0, nullptr}},
    {0x12, 1, X86InstInfo{"ADC",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM,                                                   0, nullptr}},
    {0x13, 1, X86InstInfo{"ADC",    TYPE_INST, FLAGS_MODRM,                                                                   0, nullptr}},
    {0x14, 1, X86InstInfo{"ADC",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_SF_DST_RAX  ,                              1, nullptr}},
    {0x15, 1, X86InstInfo{"ADC",    TYPE_INST, FLAGS_SF_DST_RAX | FLAGS_SRC_SEXT | FLAGS_DISPLACE_SIZE_DIV_2, 4, nullptr}},

    {0x16, 2, X86InstInfo{"[INV]",  TYPE_INVALID, FLAGS_NONE,                                                                     0, nullptr}},
    {0x18, 1, X86InstInfo{"SBB",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                   0, nullptr}},
    {0x19, 1, X86InstInfo{"SBB",    TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_DISPLACE_SIZE_DIV_2,                                       0, nullptr}},
    {0x1A, 1, X86InstInfo{"SBB",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM,                                                   0, nullptr}},
    {0x1B, 1, X86InstInfo{"SBB",    TYPE_INST, FLAGS_MODRM,                                                                   0, nullptr}},
    {0x1C, 1, X86InstInfo{"SBB",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_SF_DST_RAX  ,                              1, nullptr}},
    {0x1D, 1, X86InstInfo{"SBB",    TYPE_INST, FLAGS_SF_DST_RAX | FLAGS_SRC_SEXT | FLAGS_DISPLACE_SIZE_DIV_2, 4, nullptr}},

    {0x1E, 2, X86InstInfo{"[INV]",  TYPE_INVALID, FLAGS_NONE,                                                                     0, nullptr}},
    {0x20, 1, X86InstInfo{"AND",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                   0, nullptr}},
    {0x21, 1, X86InstInfo{"AND",    TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                                   0, nullptr}},
    {0x22, 1, X86InstInfo{"AND",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM,                                                   0, nullptr}},
    {0x23, 1, X86InstInfo{"AND",    TYPE_INST, FLAGS_MODRM,                                                                   0, nullptr}},
    {0x24, 1, X86InstInfo{"AND",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_SF_DST_RAX  ,                              1, nullptr}},
    {0x25, 1, X86InstInfo{"AND",    TYPE_INST, FLAGS_SF_DST_RAX | FLAGS_SRC_SEXT | FLAGS_DISPLACE_SIZE_DIV_2, 4, nullptr}},

    {0x27, 1, X86InstInfo{"[INV]",  TYPE_INVALID, FLAGS_NONE,                                                                     0, nullptr}},
    {0x28, 1, X86InstInfo{"SUB",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                   0, nullptr}},
    {0x29, 1, X86InstInfo{"SUB",    TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                                   0, nullptr}},
    {0x2A, 1, X86InstInfo{"SUB",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM,                                                   0, nullptr}},
    {0x2B, 1, X86InstInfo{"SUB",    TYPE_INST, FLAGS_MODRM,                                                                   0, nullptr}},
    {0x2C, 1, X86InstInfo{"SUB",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_SF_DST_RAX ,                              1, nullptr}},
    {0x2D, 1, X86InstInfo{"SUB",    TYPE_INST, FLAGS_SF_DST_RAX | FLAGS_SRC_SEXT | FLAGS_DISPLACE_SIZE_DIV_2, 4, nullptr}},

    {0x2F, 1, X86InstInfo{"[INV]",  TYPE_INVALID, FLAGS_NONE,                                                                     0, nullptr}},
    {0x30, 1, X86InstInfo{"XOR",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                   0, nullptr}},
    {0x31, 1, X86InstInfo{"XOR",    TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                                   0, nullptr}},
    {0x32, 1, X86InstInfo{"XOR",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM,                                                   0, nullptr}},
    {0x33, 1, X86InstInfo{"XOR",    TYPE_INST, FLAGS_MODRM,                                                                   0, nullptr}},
    {0x34, 1, X86InstInfo{"XOR",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_SF_DST_RAX  ,                              1, nullptr}},
    {0x35, 1, X86InstInfo{"XOR",    TYPE_INST, FLAGS_SF_DST_RAX | FLAGS_SRC_SEXT | FLAGS_DISPLACE_SIZE_DIV_2, 4, nullptr}},

    {0x37, 1, X86InstInfo{"[INV]",  TYPE_INVALID, FLAGS_NONE,                                                                     0, nullptr}},
    {0x38, 1, X86InstInfo{"CMP",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                   0, nullptr}},
    {0x39, 1, X86InstInfo{"CMP",    TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                                   0, nullptr}},
    {0x3A, 1, X86InstInfo{"CMP",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM,                                                   0, nullptr}},
    {0x3B, 1, X86InstInfo{"CMP",    TYPE_INST, FLAGS_MODRM,                                                                   0, nullptr}},
    {0x3C, 1, X86InstInfo{"CMP",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_SF_DST_RAX  ,                              1, nullptr}},
    {0x3D, 1, X86InstInfo{"CMP",    TYPE_INST, FLAGS_SF_DST_RAX | FLAGS_SRC_SEXT | FLAGS_DISPLACE_SIZE_DIV_2, 4, nullptr}},

    {0x3F, 1, X86InstInfo{"[INV]",  TYPE_INVALID, FLAGS_NONE,                                                                     0, nullptr}},
    {0x50, 8, X86InstInfo{"PUSH",   TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_SF_REX_IN_BYTE | FLAGS_DEBUG_MEM_ACCESS ,                    0, nullptr}},
    {0x58, 8, X86InstInfo{"POP",    TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_SF_REX_IN_BYTE | FLAGS_DEBUG_MEM_ACCESS ,                    0, nullptr}},

    {0x60, 3, X86InstInfo{"[INV]",  TYPE_INVALID, FLAGS_NONE,                                                                           0, nullptr}},
    {0x63, 1, X86InstInfo{"MOVSXD", TYPE_INST, GenFlagsDstSize(SIZE_64BIT) | FLAGS_MODRM,                                                                         0, nullptr}},

    {0x68, 1, X86InstInfo{"PUSH",   TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_DEBUG_MEM_ACCESS | FLAGS_DISPLACE_SIZE_DIV_2 , 4, nullptr}},
    {0x69, 1, X86InstInfo{"IMUL",   TYPE_INST, FLAGS_MODRM | FLAGS_SRC_SEXT | FLAGS_DISPLACE_SIZE_DIV_2,        4, nullptr}},
    {0x6A, 1, X86InstInfo{"PUSH",   TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_DEBUG_MEM_ACCESS | FLAGS_SRC_SEXT ,            1, nullptr}},
    {0x6B, 1, X86InstInfo{"IMUL",   TYPE_INST, FLAGS_MODRM | FLAGS_SRC_SEXT ,                                    1, nullptr}},

    // This should just throw a GP
    {0x6C, 1, X86InstInfo{"INSB",   TYPE_INVALID, FLAGS_SUPPORTS_REP, 0, nullptr}},
    {0x6D, 1, X86InstInfo{"INSW",   TYPE_INVALID, FLAGS_SUPPORTS_REP, 0, nullptr}},
    {0x6E, 1, X86InstInfo{"OUTS",   TYPE_INVALID, FLAGS_SUPPORTS_REP, 0, nullptr}},
    {0x6F, 1, X86InstInfo{"OUTS",   TYPE_INVALID, FLAGS_SUPPORTS_REP, 0, nullptr}},

    {0x70, 1, X86InstInfo{"JO",     TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_SETS_RIP | FLAGS_SRC_SEXT , 1, nullptr}},
    {0x71, 1, X86InstInfo{"JNO",    TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_SETS_RIP | FLAGS_SRC_SEXT , 1, nullptr}},
    {0x72, 1, X86InstInfo{"JB",     TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_SETS_RIP | FLAGS_SRC_SEXT , 1, nullptr}},
    {0x73, 1, X86InstInfo{"JNB",    TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_SETS_RIP | FLAGS_SRC_SEXT , 1, nullptr}},
    {0x74, 1, X86InstInfo{"JZ",     TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_SETS_RIP | FLAGS_SRC_SEXT , 1, nullptr}},
    {0x75, 1, X86InstInfo{"JNZ",    TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_SETS_RIP | FLAGS_SRC_SEXT , 1, nullptr}},
    {0x76, 1, X86InstInfo{"JBE",    TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_SETS_RIP | FLAGS_SRC_SEXT , 1, nullptr}},
    {0x77, 1, X86InstInfo{"JNBE",   TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_SETS_RIP | FLAGS_SRC_SEXT , 1, nullptr}},
    {0x78, 1, X86InstInfo{"JS",     TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_SETS_RIP | FLAGS_SRC_SEXT , 1, nullptr}},
    {0x79, 1, X86InstInfo{"JNS",    TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_SETS_RIP | FLAGS_SRC_SEXT , 1, nullptr}},
    {0x7A, 1, X86InstInfo{"JP",     TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_SETS_RIP | FLAGS_SRC_SEXT , 1, nullptr}},
    {0x7B, 1, X86InstInfo{"JNP",    TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_SETS_RIP | FLAGS_SRC_SEXT , 1, nullptr}},
    {0x7C, 1, X86InstInfo{"JL",     TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_SETS_RIP | FLAGS_SRC_SEXT , 1, nullptr}},
    {0x7D, 1, X86InstInfo{"JNL",    TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_SETS_RIP | FLAGS_SRC_SEXT , 1, nullptr}},
    {0x7E, 1, X86InstInfo{"JLE",    TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_SETS_RIP | FLAGS_SRC_SEXT , 1, nullptr}},
    {0x7F, 1, X86InstInfo{"JNLE",   TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_SETS_RIP | FLAGS_SRC_SEXT , 1, nullptr}},

    {0x84, 1, X86InstInfo{"TEST",   TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,         0, nullptr}},
    {0x85, 1, X86InstInfo{"TEST",   TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                         0, nullptr}},
    {0x86, 1, X86InstInfo{"XCHG",   TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,         0, nullptr}},
    {0x87, 1, X86InstInfo{"XCHG",   TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                         0, nullptr}},

    {0x88, 1, X86InstInfo{"MOV",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,         0, nullptr}},
    {0x89, 1, X86InstInfo{"MOV",    TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                         0, nullptr}},
    {0x8A, 1, X86InstInfo{"MOV",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM,         0, nullptr}},
    {0x8B, 1, X86InstInfo{"MOV",    TYPE_INST, FLAGS_MODRM,                         0, nullptr}},
    {0x8C, 1, X86InstInfo{"MOV",    TYPE_INVALID, FLAGS_MODRM | FLAGS_SF_MOD_DST,                      0, nullptr}},
    {0x8D, 1, X86InstInfo{"LEA",    TYPE_INST, FLAGS_MODRM,                         0, nullptr}},
    {0x8E, 1, X86InstInfo{"MOV",    TYPE_INVALID, FLAGS_MODRM,                      0, nullptr}}, // MOV seg, modrM == invalid on x86-64
    {0x90, 8, X86InstInfo{"XCHG",   TYPE_INST, FLAGS_SF_REX_IN_BYTE | FLAGS_SF_SRC_RAX, 0, nullptr}},
    {0x98, 1, X86InstInfo{"CDQE",   TYPE_INST, FLAGS_SF_DST_RAX | FLAGS_SF_SRC_RAX,     0, nullptr}},
    {0x99, 1, X86InstInfo{"CQO",    TYPE_INST, FLAGS_SF_DST_RDX | FLAGS_SF_SRC_RAX,     0, nullptr}},
    {0x9A, 1, X86InstInfo{"[INV]",  TYPE_INVALID, FLAGS_NONE,                           0, nullptr}},

    // These three are all X87 instructions
    {0x9B, 1, X86InstInfo{"FWAIT",  TYPE_INVALID, FLAGS_NONE,                           0, nullptr}},
    {0x9C, 1, X86InstInfo{"PUSHF",  TYPE_INVALID, GenFlagsSameSize(SIZE_64BIT) ,          0, nullptr}},
    {0x9D, 1, X86InstInfo{"POPF",   TYPE_INVALID, GenFlagsSameSize(SIZE_64BIT) ,          0, nullptr}},

    {0x9E, 1, X86InstInfo{"SAHF",   TYPE_INST, FLAGS_NONE,                              0, nullptr}},
    {0x9F, 1, X86InstInfo{"LAHF",   TYPE_INST, FLAGS_NONE,                              0, nullptr}},

    {0xA0, 1, X86InstInfo{"MOV",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_SF_DST_RAX  | FLAGS_MEM_OFFSET,                                         1, nullptr}},
    {0xA1, 1, X86InstInfo{"MOV",    TYPE_INST, FLAGS_SF_DST_RAX | FLAGS_DISPLACE_SIZE_DIV_2 | FLAGS_DISPLACE_SIZE_MUL_2 | FLAGS_MEM_OFFSET, 4, nullptr}},
    {0xA2, 1, X86InstInfo{"MOV",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_SF_SRC_RAX  | FLAGS_MEM_OFFSET,                                         1, nullptr}},
    {0xA3, 1, X86InstInfo{"MOV",    TYPE_INST, FLAGS_SF_SRC_RAX | FLAGS_DISPLACE_SIZE_DIV_2 | FLAGS_DISPLACE_SIZE_MUL_2 | FLAGS_MEM_OFFSET, 4, nullptr}},

    {0xA4, 1, X86InstInfo{"MOVSB",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MEM_OFFSET  | FLAGS_SUPPORTS_REP,                                            0, nullptr}},
    {0xA5, 1, X86InstInfo{"MOVS",   TYPE_INST, FLAGS_MEM_OFFSET | FLAGS_SUPPORTS_REP,                                                            0, nullptr}},
    {0xA6, 1, X86InstInfo{"CMPSB",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MEM_OFFSET  | FLAGS_SUPPORTS_REP,                                            0, nullptr}},
    {0xA7, 1, X86InstInfo{"CMPS",   TYPE_INST, FLAGS_MEM_OFFSET | FLAGS_SUPPORTS_REP,                                                            0, nullptr}},

    {0xA8, 1, X86InstInfo{"TEST",   TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_SF_DST_RAX ,                                             1, nullptr}},
    {0xA9, 1, X86InstInfo{"TEST",   TYPE_INST, FLAGS_SF_DST_RAX | FLAGS_SRC_SEXT | FLAGS_DISPLACE_SIZE_DIV_2,                4, nullptr}},
    {0xAA, 1, X86InstInfo{"STOS",   TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_DEBUG_MEM_ACCESS  | FLAGS_SUPPORTS_REP | FLAGS_SF_SRC_RAX,                   0, nullptr}},
    {0xAB, 1, X86InstInfo{"STOS",   TYPE_INST, FLAGS_DEBUG_MEM_ACCESS | FLAGS_SUPPORTS_REP | FLAGS_SF_SRC_RAX,                                   0, nullptr}},
    {0xAC, 2, X86InstInfo{"LODS",   TYPE_INST, FLAGS_DEBUG_MEM_ACCESS | FLAGS_SUPPORTS_REP,                                                      0, nullptr}},
    {0xAE, 2, X86InstInfo{"SCAS",   TYPE_INST, FLAGS_DEBUG_MEM_ACCESS,                                                                           0, nullptr}},

    {0xB0, 8, X86InstInfo{"MOV",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_SF_REX_IN_BYTE ,                                         1, nullptr}},
    {0xB8, 8, X86InstInfo{"MOV",    TYPE_INST, FLAGS_SF_REX_IN_BYTE | FLAGS_DISPLACE_SIZE_DIV_2 | FLAGS_DISPLACE_SIZE_MUL_2, 4, nullptr}},

    {0xC2, 1, X86InstInfo{"RET",    TYPE_INST, FLAGS_SETS_RIP | FLAGS_BLOCK_END,                                             2, nullptr}},
    {0xC3, 1, X86InstInfo{"RET",    TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_SETS_RIP | FLAGS_BLOCK_END ,                                                0, nullptr}},
    {0xC8, 1, X86InstInfo{"ENTER",  TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_DEBUG_MEM_ACCESS ,                                      3, nullptr}},
    {0xC9, 1, X86InstInfo{"LEAVE",  TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_SETS_RIP | FLAGS_BLOCK_END ,                                                0, nullptr}},
    {0xCA, 2, X86InstInfo{"RETF",   TYPE_PRIV, GenFlagsSameSize(SIZE_64BIT) | FLAGS_SETS_RIP | FLAGS_BLOCK_END,                                                              0, nullptr}},
    {0xCC, 1, X86InstInfo{"INT3",   TYPE_INST, FLAGS_DEBUG,                                                                                      0, nullptr}},
    {0xCD, 1, X86InstInfo{"INT",    TYPE_INST, FLAGS_DEBUG ,                                                                  1, nullptr}},
    {0xCE, 1, X86InstInfo{"[INV]",  TYPE_INVALID, FLAGS_NONE,                                                                                    0, nullptr}},
    {0xCF, 1, X86InstInfo{"IRET",   TYPE_PRIV, FLAGS_NONE,                                                                                    0, nullptr}},

    {0xD4, 3, X86InstInfo{"[INV]",  TYPE_INVALID, FLAGS_NONE,                                                                                    0, nullptr}},
    {0xD7, 1, X86InstInfo{"XLAT",   TYPE_INST, FLAGS_DEBUG_MEM_ACCESS,                                                                           0, nullptr}},

    {0xE0, 1, X86InstInfo{"LOOPNE", TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_SETS_RIP | FLAGS_SRC_SEXT ,                             1, nullptr}},
    {0xE1, 1, X86InstInfo{"LOOPE",  TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_SETS_RIP | FLAGS_SRC_SEXT ,                             1, nullptr}},
    {0xE2, 1, X86InstInfo{"LOOP",   TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_SETS_RIP | FLAGS_SRC_SEXT ,                             1, nullptr}},
    {0xE3, 1, X86InstInfo{"JrCXZ",  TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_SETS_RIP | FLAGS_SRC_SEXT ,                             1, nullptr}},

    // Should just throw GP
    {0xE4, 2, X86InstInfo{"IN",     TYPE_INVALID, FLAGS_NONE,                                                                                                      0, nullptr}},
    {0xE6, 2, X86InstInfo{"OUT",    TYPE_INVALID, FLAGS_NONE,                                                                                                      0, nullptr}},

    {0xE8, 1, X86InstInfo{"CALL",   TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_SETS_RIP | FLAGS_SRC_SEXT | FLAGS_DISPLACE_SIZE_DIV_2 | FLAGS_BLOCK_END , 4, nullptr}},
    {0xE9, 1, X86InstInfo{"JMP",    TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_SETS_RIP | FLAGS_SRC_SEXT | FLAGS_DISPLACE_SIZE_DIV_2 | FLAGS_BLOCK_END , 4, nullptr}},
    {0xEA, 1, X86InstInfo{"[INV]",  TYPE_INVALID, FLAGS_NONE,                                                                                                      0, nullptr}},
    {0xEB, 1, X86InstInfo{"JMP",    TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_SETS_RIP | FLAGS_SRC_SEXT | FLAGS_BLOCK_END ,                             1, nullptr}},

    // Should just throw GP
    {0xEC, 2, X86InstInfo{"IN",     TYPE_INVALID, FLAGS_NONE,             0, nullptr}},
    {0xEE, 2, X86InstInfo{"OUT",    TYPE_INVALID, FLAGS_NONE,             0, nullptr}},

    {0xF1, 1, X86InstInfo{"INT1",   TYPE_INST, FLAGS_DEBUG,               0, nullptr}},
    {0xF4, 1, X86InstInfo{"HLT",    TYPE_INST, FLAGS_DEBUG | FLAGS_BLOCK_END,               0, nullptr}},
    {0xF5, 1, X86InstInfo{"CMC",    TYPE_INST, FLAGS_NONE,                0, nullptr}},
    {0xF8, 1, X86InstInfo{"CLC",    TYPE_INST, FLAGS_NONE,                0, nullptr}},
    {0xF9, 1, X86InstInfo{"STC",    TYPE_INST, FLAGS_NONE,                0, nullptr}},
    {0xFA, 1, X86InstInfo{"CLI",    TYPE_PRIV, FLAGS_NONE,                0, nullptr}},
    {0xFB, 1, X86InstInfo{"STI",    TYPE_PRIV, FLAGS_NONE,                0, nullptr}},
    {0xFC, 1, X86InstInfo{"CLD",    TYPE_INST, FLAGS_NONE,                0, nullptr}},
    {0xFD, 1, X86InstInfo{"STD",    TYPE_INST, FLAGS_NONE,                0, nullptr}},

    // Two Byte table
    {0x0F, 1, X86InstInfo{"",   TYPE_SECONDARY_TABLE_PREFIX, FLAGS_NONE,  0, nullptr}},

    // x87 table
    {0xD8, 8, X86InstInfo{"",   TYPE_X87_TABLE_PREFIX, FLAGS_NONE,        0, nullptr}},

    // ModRM table
    // MoreBytes field repurposed for valid bits mask
    {0x80, 1, X86InstInfo{"",   TYPE_GROUP_1, FLAGS_NONE, 0, nullptr}},
    {0x81, 1, X86InstInfo{"",   TYPE_GROUP_1, FLAGS_NONE, 1, nullptr}},
    {0x82, 1, X86InstInfo{"",   TYPE_GROUP_1, FLAGS_NONE, 2, nullptr}},
    {0x83, 1, X86InstInfo{"",   TYPE_GROUP_1, FLAGS_NONE, 3, nullptr}},
    {0xC0, 1, X86InstInfo{"",   TYPE_GROUP_2, FLAGS_NONE, 0, nullptr}},
    {0xC1, 1, X86InstInfo{"",   TYPE_GROUP_2, FLAGS_NONE, 1, nullptr}},
    {0xD0, 1, X86InstInfo{"",   TYPE_GROUP_2, FLAGS_NONE, 2, nullptr}},
    {0xD1, 1, X86InstInfo{"",   TYPE_GROUP_2, FLAGS_NONE, 3, nullptr}},
    {0xD2, 1, X86InstInfo{"",   TYPE_GROUP_2, FLAGS_NONE, 4, nullptr}},
    {0xD3, 1, X86InstInfo{"",   TYPE_GROUP_2, FLAGS_NONE, 5, nullptr}},
    {0xF6, 1, X86InstInfo{"",   TYPE_GROUP_3, FLAGS_NONE, 0, nullptr}},
    {0xF7, 1, X86InstInfo{"",   TYPE_GROUP_3, FLAGS_NONE, 1, nullptr}},
    {0xFE, 1, X86InstInfo{"",   TYPE_GROUP_4, FLAGS_NONE, 0, nullptr}},
    {0xFF, 1, X86InstInfo{"",   TYPE_GROUP_5, FLAGS_NONE, 0, nullptr}},

    // Group 11
    {0xC6, 1, X86InstInfo{"",   TYPE_GROUP_11, FLAGS_NONE, 0, nullptr}},
    {0xC7, 1, X86InstInfo{"",   TYPE_GROUP_11, FLAGS_NONE, 1, nullptr}},

    // VEX table
    {0xC4, 2, X86InstInfo{"",   TYPE_VEX_TABLE_PREFIX, FLAGS_NONE, 0, nullptr}},

    // XOP Table
    {0x8F, 1, X86InstInfo{"",   TYPE_XOP_TABLE_PREFIX, FLAGS_NONE, 0, nullptr}},
  };

  const std::vector<std::tuple<uint8_t, uint8_t, X86InstInfo>> TwoByteOpTable = {
    // Instructions
    {0x00, 1, X86InstInfo{"",           TYPE_GROUP_6, FLAGS_NONE,                                                                                       0, nullptr}},
    {0x01, 1, X86InstInfo{"",           TYPE_GROUP_7, FLAGS_NONE,                                                                                       0, nullptr}},
    // These two load segment register data
    {0x02, 1, X86InstInfo{"LAR",        TYPE_UNDEC, FLAGS_NONE,                                                                                         0, nullptr}},
    {0x03, 1, X86InstInfo{"LSL",        TYPE_UNDEC, FLAGS_NONE,                                                                                         0, nullptr}},
    {0x04, 1, X86InstInfo{"",           TYPE_INVALID, FLAGS_NONE,                                                                                       0, nullptr}},
    {0x05, 1, X86InstInfo{"SYSCALL",    TYPE_INST, FLAGS_BLOCK_END,                                                                                     0, nullptr}},
    {0x06, 1, X86InstInfo{"CLTS",       TYPE_PRIV, FLAGS_NONE,                                                                                          0, nullptr}},
    {0x07, 1, X86InstInfo{"SYSRET",     TYPE_PRIV, FLAGS_NONE,                                                                                          0, nullptr}},
    {0x08, 1, X86InstInfo{"INVD",       TYPE_PRIV, FLAGS_NONE,                                                                                          0, nullptr}},
    {0x09, 1, X86InstInfo{"WBINVD",     TYPE_PRIV, FLAGS_NONE,                                                                                          0, nullptr}},
    {0x0A, 1, X86InstInfo{"",           TYPE_INVALID, FLAGS_NONE,                                                                                       0, nullptr}},
    {0x0B, 1, X86InstInfo{"UD2",        TYPE_INST, FLAGS_DEBUG | FLAGS_BLOCK_END,                                                                       0, nullptr}},
    {0x0C, 1, X86InstInfo{"",           TYPE_INVALID, FLAGS_NONE,                                                                                       0, nullptr}},
    {0x0D, 1, X86InstInfo{"",           TYPE_GROUP_P, FLAGS_NONE,                                                                                       0, nullptr}},
    {0x0E, 1, X86InstInfo{"FEMMS",      TYPE_3DNOW_INST, FLAGS_BLOCK_END,                                                                               0, nullptr}},
    {0x0F, 1, X86InstInfo{"",           TYPE_3DNOW_TABLE, FLAGS_NONE,                                                                                   0, nullptr}},

    {0x10, 1, X86InstInfo{"MOVUPS",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                 0, nullptr}},
    {0x11, 1, X86InstInfo{"MOVUPS",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS,                                                 0, nullptr}},
    {0x12, 1, X86InstInfo{"MOVLPS",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY | FLAGS_XMM_FLAGS,                              0, nullptr}},
    {0x13, 1, X86InstInfo{"MOVLPS",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS,                              0, nullptr}},
    {0x14, 1, X86InstInfo{"UNPCKLPS",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                 0, nullptr}},
    {0x15, 1, X86InstInfo{"UNPCKHPS",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                 0, nullptr}},
    {0x16, 1, X86InstInfo{"MOVLHPS",    TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                 0, nullptr}},
    {0x17, 1, X86InstInfo{"MOVHPS",     TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_SF_HIGH_XMM_REG | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_MOD_MEM_ONLY | FLAGS_XMM_FLAGS, 0, nullptr}},
    {0x18, 1, X86InstInfo{"",           TYPE_GROUP_16, FLAGS_NONE,                                                                                      0, nullptr}},
    {0x19, 7, X86InstInfo{"NOP",        TYPE_INST, FLAGS_DEBUG | FLAGS_MODRM,                                                                                     0, nullptr}},

    {0x20, 2, X86InstInfo{"MOV",        TYPE_PRIV, GenFlagsSameSize(SIZE_64BIT) ,                                                                                      0, nullptr}},
    {0x22, 2, X86InstInfo{"MOV",        TYPE_PRIV, GenFlagsSameSize(SIZE_64BIT) ,                                                                                      0, nullptr}},
    {0x24, 4, X86InstInfo{"",           TYPE_INVALID, FLAGS_NONE,                                                                                       0, nullptr}},
    {0x28, 1, X86InstInfo{"MOVAPS",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                 0, nullptr}},
    {0x29, 1, X86InstInfo{"MOVAPS",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS,                                                 0, nullptr}},
    {0x2A, 1, X86InstInfo{"CVTPI2PS",   TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                   0, nullptr}},
    {0x2B, 1, X86InstInfo{"MOVNTPS",    TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS,                              0, nullptr}},
    {0x2C, 1, X86InstInfo{"CVTTPS2PI",  TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                   0, nullptr}},
    {0x2D, 1, X86InstInfo{"CVTPS2PI",   TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                   0, nullptr}},
    {0x2E, 1, X86InstInfo{"UCOMISS",    TYPE_INST, FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                                   0, nullptr}},
    {0x2F, 1, X86InstInfo{"COMISS",     TYPE_INST, FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                                   0, nullptr}},

    {0x30, 1, X86InstInfo{"WRMSR",      TYPE_PRIV, FLAGS_NONE,                                                                                          0, nullptr}},
    {0x31, 1, X86InstInfo{"RDTSC",      TYPE_INST, FLAGS_DEBUG,                                                                                          0, nullptr}},
    {0x32, 1, X86InstInfo{"RDMSR",      TYPE_PRIV, FLAGS_NONE,                                                                                          0, nullptr}},
    {0x33, 1, X86InstInfo{"RDPMC",      TYPE_PRIV, FLAGS_NONE,                                                                                          0, nullptr}},
    {0x34, 1, X86InstInfo{"SYSENTER",   TYPE_PRIV, FLAGS_NONE,                                                                                          0, nullptr}},
    {0x35, 1, X86InstInfo{"SYSEXIT",    TYPE_PRIV, FLAGS_NONE,                                                                                          0, nullptr}},
    {0x36, 2, X86InstInfo{"",           TYPE_INVALID, FLAGS_NONE,                                                                                       0, nullptr}},
    {0x38, 1, X86InstInfo{"",           TYPE_0F38_TABLE, FLAGS_NONE,                                                                                    0, nullptr}},
    {0x39, 1, X86InstInfo{"",           TYPE_INVALID, FLAGS_NONE,                                                                                       0, nullptr}},
    {0x3A, 1, X86InstInfo{"",           TYPE_0F3A_TABLE, FLAGS_NONE,                                                                                    0, nullptr}},
    {0x3B, 5, X86InstInfo{"",           TYPE_INVALID, FLAGS_NONE,                                                                                       0, nullptr}},

    {0x40, 1, X86InstInfo{"CMOVO",      TYPE_INST, FLAGS_MODRM,                                                                                     0, nullptr}},
    {0x41, 1, X86InstInfo{"CMOVNO",     TYPE_INST, FLAGS_MODRM,                                                                                     0, nullptr}},
    {0x42, 1, X86InstInfo{"CMOVB",      TYPE_INST, FLAGS_MODRM,                                                                                     0, nullptr}},
    {0x43, 1, X86InstInfo{"CMOVNB",     TYPE_INST, FLAGS_MODRM,                                                                                     0, nullptr}},
    {0x44, 1, X86InstInfo{"CMOVZ",      TYPE_INST, FLAGS_MODRM,                                                                                     0, nullptr}},
    {0x45, 1, X86InstInfo{"CMOVNZ",     TYPE_INST, FLAGS_MODRM,                                                                                     0, nullptr}},
    {0x46, 1, X86InstInfo{"CMOVBE",     TYPE_INST, FLAGS_MODRM,                                                                                     0, nullptr}},
    {0x47, 1, X86InstInfo{"CMOVNBE",    TYPE_INST, FLAGS_MODRM,                                                                                     0, nullptr}},
    {0x48, 1, X86InstInfo{"CMOVS",      TYPE_INST, FLAGS_MODRM,                                                                                     0, nullptr}},
    {0x49, 1, X86InstInfo{"CMOVNS",     TYPE_INST, FLAGS_MODRM,                                                                                     0, nullptr}},
    {0x4A, 1, X86InstInfo{"CMOVP",      TYPE_INST, FLAGS_MODRM,                                                                                     0, nullptr}},
    {0x4B, 1, X86InstInfo{"CMOVNP",     TYPE_INST, FLAGS_MODRM,                                                                                     0, nullptr}},
    {0x4C, 1, X86InstInfo{"CMOVL",      TYPE_INST, FLAGS_MODRM,                                                                                     0, nullptr}},
    {0x4D, 1, X86InstInfo{"CMOVNL",     TYPE_INST, FLAGS_MODRM,                                                                                     0, nullptr}},
    {0x4E, 1, X86InstInfo{"CMOVLE",     TYPE_INST, FLAGS_MODRM,                                                                                     0, nullptr}},
    {0x4F, 1, X86InstInfo{"CMOVNLE",    TYPE_INST, FLAGS_MODRM,                                                                                     0, nullptr}},

    {0x50, 1, X86InstInfo{"MOVMSKPS",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_REG_ONLY | FLAGS_XMM_FLAGS | FLAGS_SF_DST_GPR,      0, nullptr}},
    {0x51, 1, X86InstInfo{"SQRTPS",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                 0, nullptr}},
    {0x52, 1, X86InstInfo{"RSQRTSS",    TYPE_INST, FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                                   0, nullptr}},
    {0x53, 1, X86InstInfo{"RCPPS",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                 0, nullptr}},
    {0x54, 1, X86InstInfo{"ANDPS",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                 0, nullptr}},
    {0x55, 1, X86InstInfo{"ANDNPS",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                 0, nullptr}},
    {0x56, 1, X86InstInfo{"ORPS",       TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                 0, nullptr}},
    {0x57, 1, X86InstInfo{"XORPS",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                 0, nullptr}},
    {0x58, 1, X86InstInfo{"ANDPS",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                 0, nullptr}},
    {0x59, 1, X86InstInfo{"MULPS",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                 0, nullptr}},
    {0x5A, 1, X86InstInfo{"CVTPS2PD",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                 0, nullptr}},
    {0x5B, 1, X86InstInfo{"CVTDQ2PS",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                 0, nullptr}},
    {0x5C, 1, X86InstInfo{"SUBPS",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                 0, nullptr}},
    {0x5D, 1, X86InstInfo{"MINPS",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                 0, nullptr}},
    {0x5E, 1, X86InstInfo{"DIVPS",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                 0, nullptr}},
    {0x5F, 1, X86InstInfo{"MAXPS",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                 0, nullptr}},

    {0x60, 1, X86InstInfo{"PUNPCKLBW",  TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                   0, nullptr}},
    {0x61, 1, X86InstInfo{"PUNPCKLWD",  TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                   0, nullptr}},
    {0x62, 1, X86InstInfo{"PUNPCKLDQ",  TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                   0, nullptr}},
    {0x63, 1, X86InstInfo{"PACKSSWB",   TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                   0, nullptr}},
    {0x64, 1, X86InstInfo{"PCMPGTB",    TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                   0, nullptr}},
    {0x65, 1, X86InstInfo{"PCMPGTW",    TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                   0, nullptr}},
    {0x66, 1, X86InstInfo{"PCMPGTD",    TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                   0, nullptr}},
    {0x67, 1, X86InstInfo{"PACKUSWB",   TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                   0, nullptr}},
    {0x68, 1, X86InstInfo{"PUNPCKHBW",  TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                 0, nullptr}},
    {0x69, 1, X86InstInfo{"PUNPCKHBD",  TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                 0, nullptr}},
    {0x6A, 1, X86InstInfo{"PUNPCKHDQ",  TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                  0, nullptr}},
    {0x6B, 1, X86InstInfo{"PACKSSDW",   TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                   0, nullptr}},
    {0x6C, 2, X86InstInfo{"",           TYPE_INVALID, FLAGS_NONE,                                                                                       0, nullptr}},
    {0x6E, 1, X86InstInfo{"MOVD",       TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                   0, nullptr}},
    {0x6F, 1, X86InstInfo{"MOVQ",       TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                   0, nullptr}},

    {0x70, 1, X86InstInfo{"PSHUFW",     TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,       1, nullptr}},
    {0x71, 1, X86InstInfo{"",           TYPE_GROUP_12, FLAGS_NONE,                                                                                      0, nullptr}},
    {0x72, 1, X86InstInfo{"",           TYPE_GROUP_13, FLAGS_NONE,                                                                                      0, nullptr}},
    {0x73, 1, X86InstInfo{"",           TYPE_GROUP_14, FLAGS_NONE,                                                                                      0, nullptr}},
    {0x74, 1, X86InstInfo{"PCMPEQB",    TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                   0, nullptr}},
    {0x75, 1, X86InstInfo{"PCMPEQW",    TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                   0, nullptr}},
    {0x76, 1, X86InstInfo{"PCMPEQD",    TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                   0, nullptr}},
    {0x77, 1, X86InstInfo{"EMMS",       TYPE_3DNOW_INST, FLAGS_NONE,                                                                                    0, nullptr}},
    {0x78, 6, X86InstInfo{"",           TYPE_INVALID, FLAGS_NONE,                                                                                       0, nullptr}},
    {0x7E, 1, X86InstInfo{"MOVD",       TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS,                                                   0, nullptr}},
    {0x7F, 1, X86InstInfo{"MOVQ",       TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS,                                                   0, nullptr}},

    {0x80, 1, X86InstInfo{"JO",      TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_BLOCK_END | FLAGS_SRC_SEXT | FLAGS_DISPLACE_SIZE_DIV_2 ,                                            4, nullptr}},
    {0x81, 1, X86InstInfo{"JNO",     TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_BLOCK_END | FLAGS_SRC_SEXT | FLAGS_DISPLACE_SIZE_DIV_2 ,                                            4, nullptr}},
    {0x82, 1, X86InstInfo{"JB",      TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_BLOCK_END | FLAGS_SRC_SEXT | FLAGS_DISPLACE_SIZE_DIV_2 ,                                            4, nullptr}},
    {0x83, 1, X86InstInfo{"JNB",     TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_BLOCK_END | FLAGS_SRC_SEXT | FLAGS_DISPLACE_SIZE_DIV_2 ,                                            4, nullptr}},
    {0x84, 1, X86InstInfo{"JZ",      TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_BLOCK_END | FLAGS_SRC_SEXT | FLAGS_DISPLACE_SIZE_DIV_2 ,                                            4, nullptr}},
    {0x85, 1, X86InstInfo{"JNZ",     TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_BLOCK_END | FLAGS_SRC_SEXT | FLAGS_DISPLACE_SIZE_DIV_2 ,                                            4, nullptr}},
    {0x86, 1, X86InstInfo{"JBE",     TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_BLOCK_END | FLAGS_SRC_SEXT | FLAGS_DISPLACE_SIZE_DIV_2 ,                                            4, nullptr}},
    {0x87, 1, X86InstInfo{"JNBE",    TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_BLOCK_END | FLAGS_SRC_SEXT | FLAGS_DISPLACE_SIZE_DIV_2 ,                                            4, nullptr}},
    {0x88, 1, X86InstInfo{"JS",      TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_BLOCK_END | FLAGS_SRC_SEXT | FLAGS_DISPLACE_SIZE_DIV_2 ,                                            4, nullptr}},
    {0x89, 1, X86InstInfo{"JNS",     TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_BLOCK_END | FLAGS_SRC_SEXT | FLAGS_DISPLACE_SIZE_DIV_2 ,                                            4, nullptr}},
    {0x8A, 1, X86InstInfo{"JP",      TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_BLOCK_END | FLAGS_SRC_SEXT | FLAGS_DISPLACE_SIZE_DIV_2 ,                                            4, nullptr}},
    {0x8B, 1, X86InstInfo{"JNP",     TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_BLOCK_END | FLAGS_SRC_SEXT | FLAGS_DISPLACE_SIZE_DIV_2 ,                                            4, nullptr}},
    {0x8C, 1, X86InstInfo{"JL",      TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_BLOCK_END | FLAGS_SRC_SEXT | FLAGS_DISPLACE_SIZE_DIV_2 ,                                            4, nullptr}},
    {0x8D, 1, X86InstInfo{"JNL",     TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_BLOCK_END | FLAGS_SRC_SEXT | FLAGS_DISPLACE_SIZE_DIV_2 ,                                            4, nullptr}},
    {0x8E, 1, X86InstInfo{"JLE",     TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_BLOCK_END | FLAGS_SRC_SEXT | FLAGS_DISPLACE_SIZE_DIV_2 ,                                            4, nullptr}},
    {0x8F, 1, X86InstInfo{"JNLE",    TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_BLOCK_END | FLAGS_SRC_SEXT | FLAGS_DISPLACE_SIZE_DIV_2 ,                                            4, nullptr}},

    {0x90, 1, X86InstInfo{"SETO",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                                                        0, nullptr}},
    {0x91, 1, X86InstInfo{"SETNO",   TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                                                        0, nullptr}},
    {0x92, 1, X86InstInfo{"SETB",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                                                        0, nullptr}},
    {0x93, 1, X86InstInfo{"SETNB",   TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                                                        0, nullptr}},
    {0x94, 1, X86InstInfo{"SETZ",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                                                        0, nullptr}},
    {0x95, 1, X86InstInfo{"SETNZ",   TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                                                        0, nullptr}},
    {0x96, 1, X86InstInfo{"SETBE",   TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                                                        0, nullptr}},
    {0x97, 1, X86InstInfo{"SETNBE",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                                                        0, nullptr}},
    {0x98, 1, X86InstInfo{"SETS",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                                                        0, nullptr}},
    {0x99, 1, X86InstInfo{"SETNS",   TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                                                        0, nullptr}},
    {0x9A, 1, X86InstInfo{"SETP",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                                                        0, nullptr}},
    {0x9B, 1, X86InstInfo{"SETNP",   TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                                                        0, nullptr}},
    {0x9C, 1, X86InstInfo{"SETL",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                                                        0, nullptr}},
    {0x9D, 1, X86InstInfo{"SETNL",   TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                                                        0, nullptr}},
    {0x9E, 1, X86InstInfo{"SETLE",   TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                                                        0, nullptr}},
    {0x9F, 1, X86InstInfo{"SETNLE",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                                                        0, nullptr}},

    {0xA0, 1, X86InstInfo{"PUSH",    TYPE_INVALID,  FLAGS_NONE,                                                                                         0, nullptr}},
    {0xA1, 1, X86InstInfo{"POP FS",  TYPE_INVALID,  FLAGS_NONE,                                                                                         0, nullptr}},
    {0xA2, 1, X86InstInfo{"CPUID",   TYPE_INST,     FLAGS_DEBUG | FLAGS_SF_SRC_RAX,                                                                                   0, nullptr}},
    {0xA3, 1, X86InstInfo{"BT",      TYPE_INST,     FLAGS_DEBUG_MEM_ACCESS | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                                                    0, nullptr}},
    {0xA4, 1, X86InstInfo{"SHLD",    TYPE_INST,     FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                                                    1, nullptr}},
    {0xA5, 1, X86InstInfo{"SHLD",    TYPE_INST,     FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_SRC_RCX,                                                                 0, nullptr}},
    {0xA6, 2, X86InstInfo{"",        TYPE_INVALID,  FLAGS_NONE,                                                                                         0, nullptr}},
    {0xA8, 1, X86InstInfo{"PUSH",    TYPE_INVALID,  FLAGS_NONE,                                                                                         0, nullptr}},
    {0xA9, 1, X86InstInfo{"POP GS",  TYPE_INVALID,  FLAGS_NONE,                                                                                         0, nullptr}},
    {0xAA, 1, X86InstInfo{"RSM",     TYPE_PRIV,     FLAGS_NONE,                                                                                         0, nullptr}},
    {0xAB, 1, X86InstInfo{"BTS",     TYPE_INST,     FLAGS_DEBUG_MEM_ACCESS | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                                                    0, nullptr}},
    {0xAC, 1, X86InstInfo{"SHRD",    TYPE_INST,     FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                                                    1, nullptr}},
    {0xAD, 1, X86InstInfo{"SHRD",    TYPE_INST,     FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_SRC_RCX,                                                                 0, nullptr}},
    {0xAE, 1, X86InstInfo{"",        TYPE_GROUP_15, FLAGS_NONE,                                                                                         0, nullptr}},
    {0xAF, 1, X86InstInfo{"IMUL",    TYPE_INST,     FLAGS_MODRM,                                                                                    0, nullptr}},

    {0xB0, 1, X86InstInfo{"CMPXCHG", TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                                        0, nullptr}},
    {0xB1, 1, X86InstInfo{"CMPXCHG", TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                                                        0, nullptr}},
    {0xB2, 1, X86InstInfo{"LSS",     TYPE_INVALID, FLAGS_NONE,                                                                                          0, nullptr}},
    {0xB3, 1, X86InstInfo{"BTR",     TYPE_INST, FLAGS_DEBUG_MEM_ACCESS | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                                                        0, nullptr}},
    {0xB4, 1, X86InstInfo{"LFS",     TYPE_INVALID, FLAGS_NONE,                                                                                          0, nullptr}},
    {0xB5, 1, X86InstInfo{"LGS",     TYPE_INVALID, FLAGS_NONE,                                                                                          0, nullptr}},
    {0xB6, 1, X86InstInfo{"MOVZX",   TYPE_INST, GenFlagsSrcSize(SIZE_8BIT) | FLAGS_MODRM,                                                                                        0, nullptr}},
    {0xB7, 1, X86InstInfo{"MOVZX",   TYPE_INST, GenFlagsSrcSize(SIZE_16BIT) | FLAGS_MODRM,                                                                                        0, nullptr}},
    {0xB8, 1, X86InstInfo{"",        TYPE_INVALID, FLAGS_NONE,                                                                                          0, nullptr}},
    {0xB9, 1, X86InstInfo{"",        TYPE_GROUP_10, FLAGS_NONE,                                                                                         0, nullptr}},
    {0xBA, 1, X86InstInfo{"",        TYPE_GROUP_8, FLAGS_NONE,                                                                                          0, nullptr}},
    {0xBB, 1, X86InstInfo{"BTC",     TYPE_INST, FLAGS_DEBUG_MEM_ACCESS | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                                                        0, nullptr}},
    {0xBC, 1, X86InstInfo{"BSF",     TYPE_INST, FLAGS_MODRM,                                                                                        0, nullptr}},
    {0xBD, 1, X86InstInfo{"BSR",     TYPE_INST, FLAGS_MODRM,                                                                                        0, nullptr}},
    {0xBE, 1, X86InstInfo{"MOVSX",   TYPE_INST, GenFlagsSrcSize(SIZE_8BIT) | FLAGS_MODRM,                                                                                        0, nullptr}},
    {0xBF, 1, X86InstInfo{"MOVSX",   TYPE_INST, GenFlagsSrcSize(SIZE_16BIT) | FLAGS_MODRM,                                                                                        0, nullptr}},

    {0xC0, 1, X86InstInfo{"XADD",    TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                                        0, nullptr}},
    {0xC1, 1, X86InstInfo{"XADD",    TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                                                        0, nullptr}},
    {0xC2, 1, X86InstInfo{"CMPPS",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM,                                                                      1, nullptr}},
    {0xC3, 1, X86InstInfo{"MOVNTI",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY | FLAGS_SF_MOD_DST,                                                                                        0, nullptr}},
    {0xC4, 1, X86InstInfo{"PINSRW",  TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_SRC_GPR,                                   1, nullptr}},
    {0xC5, 1, X86InstInfo{"PEXTRW",  TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_XMM_FLAGS,                                                                        1, nullptr}},
    {0xC6, 1, X86InstInfo{"SHUFPS",  TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM,                                                                      1, nullptr}},
    {0xC7, 1, X86InstInfo{"",        TYPE_GROUP_9, FLAGS_NONE,                                                                                          0, nullptr}},
    {0xC8, 8, X86InstInfo{"BSWAP",   TYPE_INST, FLAGS_SF_REX_IN_BYTE,                                                                                             0, nullptr}},

    {0xD0, 1, X86InstInfo{"",         TYPE_INVALID, FLAGS_NONE,                                                                                         0, nullptr}},
    {0xD1, 1, X86InstInfo{"PSRLW",    TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                     0, nullptr}},
    {0xD2, 1, X86InstInfo{"PSRLD",    TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                     0, nullptr}},
    {0xD3, 1, X86InstInfo{"PSRLQ",    TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                     0, nullptr}},
    {0xD4, 1, X86InstInfo{"PADDQ",    TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                     0, nullptr}}, // SSE2 extending MMX
    {0xD5, 1, X86InstInfo{"PMULLW",   TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                     0, nullptr}},
    {0xD6, 1, X86InstInfo{"",         TYPE_INVALID, FLAGS_NONE,                                                                                         0, nullptr}},
    {0xD7, 1, X86InstInfo{"PMOVMSKB", TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_DST_GPR,                                  0, nullptr}},
    {0xD8, 1, X86InstInfo{"PSUBUSB",  TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                     0, nullptr}},
    {0xD9, 1, X86InstInfo{"PSUBUSW",  TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                     0, nullptr}},
    {0xDA, 1, X86InstInfo{"PMINUB",   TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                     0, nullptr}},
    {0xDB, 1, X86InstInfo{"PAND",     TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                     0, nullptr}},
    {0xDC, 1, X86InstInfo{"PADDUSB",  TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                     0, nullptr}},
    {0xDD, 1, X86InstInfo{"PADDUSW",  TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                     0, nullptr}},
    {0xDE, 1, X86InstInfo{"PMAXUB",   TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                     0, nullptr}},
    {0xDF, 1, X86InstInfo{"PANDN",    TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                     0, nullptr}},

    {0xE0, 1, X86InstInfo{"PAVGB",    TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                     0, nullptr}},
    {0xE1, 1, X86InstInfo{"PSRAW",    TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                     0, nullptr}},
    {0xE2, 1, X86InstInfo{"PSRAD",    TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                     0, nullptr}},
    {0xE3, 1, X86InstInfo{"PAVGW",    TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                     0, nullptr}},
    {0xE4, 1, X86InstInfo{"PMULHUW",  TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                     0, nullptr}},
    {0xE5, 1, X86InstInfo{"PMULHW",   TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                     0, nullptr}},
    {0xE6, 1, X86InstInfo{"",         TYPE_INVALID, FLAGS_NONE,                                                                                         0, nullptr}},
    {0xE7, 1, X86InstInfo{"MOVNTQ",   TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS,                                                     0, nullptr}},
    {0xE8, 1, X86InstInfo{"PSUBSB",   TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                     0, nullptr}},
    {0xE9, 1, X86InstInfo{"PSUBSW",   TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                     0, nullptr}},
    {0xEA, 1, X86InstInfo{"PMINSW",   TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                     0, nullptr}},
    {0xEB, 1, X86InstInfo{"POR",      TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                     0, nullptr}},
    {0xEC, 1, X86InstInfo{"PADDSB",   TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                     0, nullptr}},
    {0xED, 1, X86InstInfo{"PADDSW",   TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                     0, nullptr}},
    {0xEE, 1, X86InstInfo{"PMAXSW",   TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                     0, nullptr}},
    {0xEF, 1, X86InstInfo{"PXOR",     TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                     0, nullptr}},

    {0xF0, 1, X86InstInfo{"",         TYPE_INVALID, FLAGS_NONE,                                                                                         0, nullptr}},
    {0xF1, 1, X86InstInfo{"PSLLW",    TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                     0, nullptr}},
    {0xF2, 1, X86InstInfo{"PSLLD",    TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                     0, nullptr}},
    {0xF3, 1, X86InstInfo{"PSLLQ",    TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                     0, nullptr}},
    {0xF4, 1, X86InstInfo{"PMULUDQ",  TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                     0, nullptr}},
    {0xF5, 1, X86InstInfo{"PMADDWD",  TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                     0, nullptr}},
    {0xF6, 1, X86InstInfo{"PSADBW",   TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                     0, nullptr}},
    {0xF7, 1, X86InstInfo{"MASKMOVQ", TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                     0, nullptr}},
    {0xF8, 1, X86InstInfo{"PSUBB",    TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                     0, nullptr}},
    {0xF9, 1, X86InstInfo{"PSUBW",    TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                     0, nullptr}},
    {0xFA, 1, X86InstInfo{"PSUBD",    TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                     0, nullptr}},
    {0xFB, 1, X86InstInfo{"PSUBQ",    TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                     0, nullptr}},
    {0xFC, 1, X86InstInfo{"PADDB",    TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                     0, nullptr}},
    {0xFD, 1, X86InstInfo{"PADDW",    TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                     0, nullptr}},
    {0xFE, 1, X86InstInfo{"PADDD",    TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                   0, nullptr}},
    {0xFF, 1, X86InstInfo{"UD0",      TYPE_INST, FLAGS_DEBUG | FLAGS_BLOCK_END,                                                                                           0, nullptr}},
  };

  const std::vector<std::tuple<uint16_t, uint8_t, X86InstInfo>> PrimaryGroupOpTable = {
#define OPD(group, prefix, Reg) (((group - FEXCore::X86Tables::TYPE_GROUP_1) << 6) | (prefix) << 3 | (Reg))
    // GROUP_1 | 0x80 | reg
    {OPD(TYPE_GROUP_1, OpToIndex(0x80), 0), 1, X86InstInfo{"ADD",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                      1, nullptr}},
    {OPD(TYPE_GROUP_1, OpToIndex(0x80), 1), 1, X86InstInfo{"OR",   TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                      1, nullptr}},
    {OPD(TYPE_GROUP_1, OpToIndex(0x80), 2), 1, X86InstInfo{"ADC",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                      1, nullptr}},
    {OPD(TYPE_GROUP_1, OpToIndex(0x80), 3), 1, X86InstInfo{"SBB",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                      1, nullptr}},
    {OPD(TYPE_GROUP_1, OpToIndex(0x80), 4), 1, X86InstInfo{"AND",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                      1, nullptr}},
    {OPD(TYPE_GROUP_1, OpToIndex(0x80), 5), 1, X86InstInfo{"SUB",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                      1, nullptr}},
    {OPD(TYPE_GROUP_1, OpToIndex(0x80), 6), 1, X86InstInfo{"XOR",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                      1, nullptr}},
    {OPD(TYPE_GROUP_1, OpToIndex(0x80), 7), 1, X86InstInfo{"CMP",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                      1, nullptr}},

    {OPD(TYPE_GROUP_1, OpToIndex(0x81), 0), 1, X86InstInfo{"ADD",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_DISPLACE_SIZE_DIV_2,                          4, nullptr}},
    {OPD(TYPE_GROUP_1, OpToIndex(0x81), 1), 1, X86InstInfo{"OR",   TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_DISPLACE_SIZE_DIV_2,                          4, nullptr}},
    {OPD(TYPE_GROUP_1, OpToIndex(0x81), 2), 1, X86InstInfo{"ADC",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_DISPLACE_SIZE_DIV_2,                          4, nullptr}},
    {OPD(TYPE_GROUP_1, OpToIndex(0x81), 3), 1, X86InstInfo{"SBB",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_DISPLACE_SIZE_DIV_2,                          4, nullptr}},
    {OPD(TYPE_GROUP_1, OpToIndex(0x81), 4), 1, X86InstInfo{"AND",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_DISPLACE_SIZE_DIV_2,                          4, nullptr}},
    {OPD(TYPE_GROUP_1, OpToIndex(0x81), 5), 1, X86InstInfo{"SUB",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_DISPLACE_SIZE_DIV_2,                          4, nullptr}},
    {OPD(TYPE_GROUP_1, OpToIndex(0x81), 6), 1, X86InstInfo{"XOR",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_DISPLACE_SIZE_DIV_2,                          4, nullptr}},
    {OPD(TYPE_GROUP_1, OpToIndex(0x81), 7), 1, X86InstInfo{"CMP",  TYPE_INST, FLAGS_SRC_SEXT | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_DISPLACE_SIZE_DIV_2,                          4, nullptr}},

    // Invalid in 64bit mode
    {OPD(TYPE_GROUP_1, OpToIndex(0x82), 0), 8, X86InstInfo{"",     TYPE_INVALID, FLAGS_NONE,                                                        0, nullptr}},

    {OPD(TYPE_GROUP_1, OpToIndex(0x83), 0), 1, X86InstInfo{"ADD",  TYPE_INST, FLAGS_SRC_SEXT | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                     1, nullptr}},
    {OPD(TYPE_GROUP_1, OpToIndex(0x83), 1), 1, X86InstInfo{"OR",   TYPE_INST, FLAGS_SRC_SEXT | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                     1, nullptr}},
    {OPD(TYPE_GROUP_1, OpToIndex(0x83), 2), 1, X86InstInfo{"ADC",  TYPE_INST, FLAGS_SRC_SEXT | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                     1, nullptr}},
    {OPD(TYPE_GROUP_1, OpToIndex(0x83), 3), 1, X86InstInfo{"SBB",  TYPE_INST, FLAGS_SRC_SEXT | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                     1, nullptr}},
    {OPD(TYPE_GROUP_1, OpToIndex(0x83), 4), 1, X86InstInfo{"AND",  TYPE_INST, FLAGS_SRC_SEXT | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                     1, nullptr}},
    {OPD(TYPE_GROUP_1, OpToIndex(0x83), 5), 1, X86InstInfo{"SUB",  TYPE_INST, FLAGS_SRC_SEXT | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                     1, nullptr}},
    {OPD(TYPE_GROUP_1, OpToIndex(0x83), 6), 1, X86InstInfo{"XOR",  TYPE_INST, FLAGS_SRC_SEXT | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                     1, nullptr}},
    {OPD(TYPE_GROUP_1, OpToIndex(0x83), 7), 1, X86InstInfo{"CMP",  TYPE_INST, FLAGS_SRC_SEXT | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                     1, nullptr}},

    // GROUP 2
    {OPD(TYPE_GROUP_2, OpToIndex(0xC0), 0), 1, X86InstInfo{"ROL",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                      1, nullptr}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xC0), 1), 1, X86InstInfo{"ROR",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                      1, nullptr}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xC0), 2), 1, X86InstInfo{"RCL",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                      1, nullptr}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xC0), 3), 1, X86InstInfo{"RCR",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                      1, nullptr}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xC0), 4), 1, X86InstInfo{"SHL",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                      1, nullptr}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xC0), 5), 1, X86InstInfo{"SHR",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                      1, nullptr}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xC0), 6), 1, X86InstInfo{"SHL",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                      1, nullptr}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xC0), 7), 1, X86InstInfo{"SAR",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                      1, nullptr}},

    {OPD(TYPE_GROUP_2, OpToIndex(0xC1), 0), 1, X86InstInfo{"ROL",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                      1, nullptr}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xC1), 1), 1, X86InstInfo{"ROR",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                      1, nullptr}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xC1), 2), 1, X86InstInfo{"RCL",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                      1, nullptr}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xC1), 3), 1, X86InstInfo{"RCR",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                      1, nullptr}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xC1), 4), 1, X86InstInfo{"SHL",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                      1, nullptr}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xC1), 5), 1, X86InstInfo{"SHR",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                      1, nullptr}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xC1), 6), 1, X86InstInfo{"SHL",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                      1, nullptr}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xC1), 7), 1, X86InstInfo{"SAR",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                      1, nullptr}},

    {OPD(TYPE_GROUP_2, OpToIndex(0xD0), 0), 1, X86InstInfo{"ROL",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                      0, nullptr}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xD0), 1), 1, X86InstInfo{"ROR",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                      0, nullptr}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xD0), 2), 1, X86InstInfo{"RCL",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                      0, nullptr}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xD0), 3), 1, X86InstInfo{"RCR",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                      0, nullptr}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xD0), 4), 1, X86InstInfo{"SHL",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                      0, nullptr}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xD0), 5), 1, X86InstInfo{"SHR",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                      0, nullptr}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xD0), 6), 1, X86InstInfo{"SHL",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                      0, nullptr}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xD0), 7), 1, X86InstInfo{"SAR",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                      0, nullptr}},

    {OPD(TYPE_GROUP_2, OpToIndex(0xD1), 0), 1, X86InstInfo{"ROL",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                      0, nullptr}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xD1), 1), 1, X86InstInfo{"ROR",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                      0, nullptr}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xD1), 2), 1, X86InstInfo{"RCL",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                      0, nullptr}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xD1), 3), 1, X86InstInfo{"RCR",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                      0, nullptr}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xD1), 4), 1, X86InstInfo{"SHL",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                      0, nullptr}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xD1), 5), 1, X86InstInfo{"SHR",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                      0, nullptr}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xD1), 6), 1, X86InstInfo{"SHL",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                      0, nullptr}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xD1), 7), 1, X86InstInfo{"SAR",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                      0, nullptr}},

    {OPD(TYPE_GROUP_2, OpToIndex(0xD2), 0), 1, X86InstInfo{"ROL",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_SRC_RCX,                   0, nullptr}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xD2), 1), 1, X86InstInfo{"ROR",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_SRC_RCX,                   0, nullptr}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xD2), 2), 1, X86InstInfo{"RCL",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_SRC_RCX,                   0, nullptr}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xD2), 3), 1, X86InstInfo{"RCR",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_SRC_RCX,                   0, nullptr}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xD2), 4), 1, X86InstInfo{"SHL",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_SRC_RCX,                   0, nullptr}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xD2), 5), 1, X86InstInfo{"SHR",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_SRC_RCX,                   0, nullptr}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xD2), 6), 1, X86InstInfo{"SHL",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_SRC_RCX,                   0, nullptr}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xD2), 7), 1, X86InstInfo{"SAR",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_SRC_RCX,                   0, nullptr}},

    {OPD(TYPE_GROUP_2, OpToIndex(0xD3), 0), 1, X86InstInfo{"ROL",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_SRC_RCX,                                   0, nullptr}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xD3), 1), 1, X86InstInfo{"ROR",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_SRC_RCX,                                   0, nullptr}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xD3), 2), 1, X86InstInfo{"RCL",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_SRC_RCX,                                   0, nullptr}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xD3), 3), 1, X86InstInfo{"RCR",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_SRC_RCX,                                   0, nullptr}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xD3), 4), 1, X86InstInfo{"SHL",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_SRC_RCX,                                   0, nullptr}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xD3), 5), 1, X86InstInfo{"SHR",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_SRC_RCX,                                   0, nullptr}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xD3), 6), 1, X86InstInfo{"SHL",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_SRC_RCX,                                   0, nullptr}},
    {OPD(TYPE_GROUP_2, OpToIndex(0xD3), 7), 1, X86InstInfo{"SAR",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_SRC_RCX,                                   0, nullptr}},

    // GROUP 3
    {OPD(TYPE_GROUP_3, OpToIndex(0xF6), 0), 1, X86InstInfo{"TEST", TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                      1, nullptr}},
    {OPD(TYPE_GROUP_3, OpToIndex(0xF6), 1), 1, X86InstInfo{"TEST", TYPE_UNDEC, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                      1, nullptr}},
    {OPD(TYPE_GROUP_3, OpToIndex(0xF6), 2), 1, X86InstInfo{"NOT",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                      0, nullptr}},
    {OPD(TYPE_GROUP_3, OpToIndex(0xF6), 3), 1, X86InstInfo{"NEG",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                      0, nullptr}},
    {OPD(TYPE_GROUP_3, OpToIndex(0xF6), 4), 1, X86InstInfo{"MUL",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                      0, nullptr}},
    {OPD(TYPE_GROUP_3, OpToIndex(0xF6), 5), 1, X86InstInfo{"IMUL", TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                      0, nullptr}},
    {OPD(TYPE_GROUP_3, OpToIndex(0xF6), 6), 1, X86InstInfo{"DIV",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                      0, nullptr}},
    {OPD(TYPE_GROUP_3, OpToIndex(0xF6), 7), 1, X86InstInfo{"IDIV", TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                      0, nullptr}},

    {OPD(TYPE_GROUP_3, OpToIndex(0xF7), 0), 1, X86InstInfo{"TEST", TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_DISPLACE_SIZE_DIV_2,                          4, nullptr}},
    {OPD(TYPE_GROUP_3, OpToIndex(0xF7), 1), 1, X86InstInfo{"TEST", TYPE_UNDEC, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_DISPLACE_SIZE_DIV_2,                          4, nullptr}},
    {OPD(TYPE_GROUP_3, OpToIndex(0xF7), 2), 1, X86InstInfo{"NOT",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                      0, nullptr}},
    {OPD(TYPE_GROUP_3, OpToIndex(0xF7), 3), 1, X86InstInfo{"NEG",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                      0, nullptr}},
    {OPD(TYPE_GROUP_3, OpToIndex(0xF7), 4), 1, X86InstInfo{"MUL",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                      0, nullptr}},
    {OPD(TYPE_GROUP_3, OpToIndex(0xF7), 5), 1, X86InstInfo{"IMUL", TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                      0, nullptr}},
    {OPD(TYPE_GROUP_3, OpToIndex(0xF7), 6), 1, X86InstInfo{"DIV",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                      0, nullptr}},
    {OPD(TYPE_GROUP_3, OpToIndex(0xF7), 7), 1, X86InstInfo{"IDIV", TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                      0, nullptr}},

    // GROUP 4
    {OPD(TYPE_GROUP_4, OpToIndex(0xFE), 0), 1, X86InstInfo{"INC",   TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                     0, nullptr}},
    {OPD(TYPE_GROUP_4, OpToIndex(0xFE), 1), 1, X86InstInfo{"DEC",   TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                     0, nullptr}},
    {OPD(TYPE_GROUP_4, OpToIndex(0xFE), 2), 6, X86InstInfo{"",      TYPE_INVALID, FLAGS_NONE,                                                       0, nullptr}},

    // GROUP 5
    {OPD(TYPE_GROUP_5, OpToIndex(0xFF), 0), 1, X86InstInfo{"INC",   TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                     0, nullptr}},
    {OPD(TYPE_GROUP_5, OpToIndex(0xFF), 1), 1, X86InstInfo{"DEC",   TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                     0, nullptr}},
    {OPD(TYPE_GROUP_5, OpToIndex(0xFF), 2), 1, X86InstInfo{"CALL",  TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_SETS_RIP | FLAGS_MODRM | FLAGS_BLOCK_END , 0, nullptr}},
    {OPD(TYPE_GROUP_5, OpToIndex(0xFF), 3), 1, X86InstInfo{"CALLF", TYPE_INST, FLAGS_SETS_RIP | FLAGS_MODRM | FLAGS_BLOCK_END,                  0, nullptr}},
    {OPD(TYPE_GROUP_5, OpToIndex(0xFF), 4), 1, X86InstInfo{"JMP",   TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_SETS_RIP | FLAGS_MODRM | FLAGS_BLOCK_END , 0, nullptr}},
    {OPD(TYPE_GROUP_5, OpToIndex(0xFF), 5), 1, X86InstInfo{"JMPF",  TYPE_INST, FLAGS_SETS_RIP | FLAGS_MODRM | FLAGS_BLOCK_END,                  0, nullptr}},
    {OPD(TYPE_GROUP_5, OpToIndex(0xFF), 6), 1, X86InstInfo{"PUSH",  TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_DEBUG_MEM_ACCESS | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                     0, nullptr}},
    {OPD(TYPE_GROUP_5, OpToIndex(0xFF), 7), 1, X86InstInfo{"",      TYPE_INVALID, FLAGS_NONE,                                                       0, nullptr}},

    // GROUP 11
    {OPD(TYPE_GROUP_11, OpToIndex(0xC6), 0), 1, X86InstInfo{"MOV",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST  | FLAGS_SRC_SEXT,                   1, nullptr}},
    {OPD(TYPE_GROUP_11, OpToIndex(0xC6), 1), 6, X86InstInfo{"",     TYPE_INVALID, FLAGS_NONE,                                                       0, nullptr}},
    {OPD(TYPE_GROUP_11, OpToIndex(0xC7), 0), 1, X86InstInfo{"MOV",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST  | FLAGS_SRC_SEXT,                                   4, nullptr}},
    {OPD(TYPE_GROUP_11, OpToIndex(0xC7), 1), 6, X86InstInfo{"",     TYPE_INVALID, FLAGS_NONE,                                                       0, nullptr}},
#undef OPD
  };

  const std::vector<std::tuple<uint8_t, uint8_t, X86InstInfo>> RepModOpTable = {
    {0x0, 16, X86InstInfo{"",          TYPE_COPY_OTHER, FLAGS_NONE,                                     0, nullptr}},

    {0x10, 1, X86InstInfo{"MOVSS",     TYPE_INST, FLAGS_MODRM | FLAGS_XMM_FLAGS,                    0, nullptr}},
    {0x11, 1, X86InstInfo{"MOVSS",     TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS,                    0, nullptr}},
    {0x12, 1, X86InstInfo{"MOVSLDUP",  TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,  0, nullptr}},
    {0x13, 3, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                        0, nullptr}},
    {0x16, 1, X86InstInfo{"MOVSHDUP",  TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,  0, nullptr}},
    {0x17, 2, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                        0, nullptr}},
    {0x19, 7, X86InstInfo{"",          TYPE_COPY_OTHER, FLAGS_NONE,                                     0, nullptr}},

    {0x20, 4, X86InstInfo{"",          TYPE_COPY_OTHER, FLAGS_NONE,                                     0, nullptr}},
    {0x24, 6, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                        0, nullptr}},
    {0x2A, 1, X86InstInfo{"CVTSI2SS",  TYPE_INST, FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_SRC_GPR, 0, nullptr}},
    {0x2B, 1, X86InstInfo{"MOVNTSS",   TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS, 0, nullptr}},
    {0x2C, 1, X86InstInfo{"CVTTSS2SI", TYPE_INST, FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_DST_GPR, 0, nullptr}},
    {0x2D, 1, X86InstInfo{"CVTSS2SI",  TYPE_INST, FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_DST_GPR, 0, nullptr}},
    {0x2E, 2, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                        0, nullptr}},

    {0x30, 16, X86InstInfo{"",         TYPE_COPY_OTHER, FLAGS_NONE,                                     0, nullptr}},
    {0x40, 16, X86InstInfo{"",         TYPE_COPY_OTHER, FLAGS_NONE,                                     0, nullptr}},

    {0x50, 1, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                        0, nullptr}},
    {0x51, 1, X86InstInfo{"SQRTSS",    TYPE_INST, FLAGS_MODRM | FLAGS_XMM_FLAGS,                    0, nullptr}},
    {0x52, 1, X86InstInfo{"RSQRTSS",   TYPE_INST, FLAGS_MODRM | FLAGS_XMM_FLAGS,                    0, nullptr}},
    {0x53, 1, X86InstInfo{"RCPSS",     TYPE_INST, FLAGS_MODRM | FLAGS_XMM_FLAGS,                    0, nullptr}},
    {0x54, 4, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                        0, nullptr}},
    {0x58, 1, X86InstInfo{"ADDSS",     TYPE_INST, FLAGS_MODRM | FLAGS_XMM_FLAGS,                    0, nullptr}},
    {0x59, 1, X86InstInfo{"MULSS",     TYPE_INST, FLAGS_MODRM | FLAGS_XMM_FLAGS,                    0, nullptr}},
    {0x5A, 1, X86InstInfo{"CVTSS2SD",  TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,   0, nullptr}},
    {0x5B, 1, X86InstInfo{"CVTTPS2DQ", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,  0, nullptr}},
    {0x5C, 1, X86InstInfo{"SUBSS",     TYPE_INST, FLAGS_MODRM | FLAGS_XMM_FLAGS,                    0, nullptr}},
    {0x5D, 1, X86InstInfo{"MINSS",     TYPE_INST, FLAGS_MODRM | FLAGS_XMM_FLAGS,                    0, nullptr}},
    {0x5E, 1, X86InstInfo{"DIVSS",     TYPE_INST, FLAGS_MODRM | FLAGS_XMM_FLAGS,                    0, nullptr}},
    {0x5F, 1, X86InstInfo{"MAXSS",     TYPE_INST, FLAGS_MODRM | FLAGS_XMM_FLAGS,                    0, nullptr}},

    {0x60, 8, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                        0, nullptr}},
    {0x68, 7, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                        0, nullptr}},
    {0x6F, 1, X86InstInfo{"MOVDQU",    TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,  0, nullptr}},

    {0x70, 1, X86InstInfo{"PSHUFHW",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,  1, nullptr}},
    {0x71, 3, X86InstInfo{"",          TYPE_COPY_OTHER, FLAGS_NONE,                                     0, nullptr}},
    {0x74, 4, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                        0, nullptr}},
    {0x78, 6, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                        0, nullptr}},
    {0x7E, 1, X86InstInfo{"MOVQ",      TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,   0, nullptr}},
    {0x7F, 1, X86InstInfo{"MOVDQU",    TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS,  0, nullptr}},

    {0x80, 16, X86InstInfo{"",         TYPE_COPY_OTHER, FLAGS_NONE,                                     0, nullptr}},
    {0x90, 16, X86InstInfo{"",         TYPE_COPY_OTHER, FLAGS_NONE,                                     0, nullptr}},
    {0xA0, 16, X86InstInfo{"",         TYPE_COPY_OTHER, FLAGS_NONE,                                     0, nullptr}},

    {0xB0, 8, X86InstInfo{"",          TYPE_COPY_OTHER, FLAGS_NONE,                                     0, nullptr}},
    {0xB8, 1, X86InstInfo{"POPCNT",    TYPE_INST, FLAGS_MODRM,                                      0, nullptr}},
    {0xB9, 1, X86InstInfo{"",          TYPE_COPY_OTHER, FLAGS_NONE,                                        0, nullptr}},
    {0xBA, 1, X86InstInfo{"",          TYPE_COPY_OTHER, FLAGS_NONE,                                        0, nullptr}},
    {0xBB, 1, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                        0, nullptr}},
    {0xBC, 1, X86InstInfo{"TZCNT",     TYPE_INST, FLAGS_MODRM,                                      0, nullptr}},
    {0xBD, 1, X86InstInfo{"LZCNT",     TYPE_INST, FLAGS_MODRM,                                      0, nullptr}},
    {0xBE, 2, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                        0, nullptr}},

    {0xC0, 2, X86InstInfo{"",          TYPE_COPY_OTHER, FLAGS_NONE,                                     0, nullptr}},
    {0xC2, 1, X86InstInfo{"CMPSS",     TYPE_INST, FLAGS_MODRM | FLAGS_XMM_FLAGS,                    1, nullptr}},
    {0xC3, 5, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                        0, nullptr}},
    {0xC8, 8, X86InstInfo{"",          TYPE_COPY_OTHER, FLAGS_NONE,                                     0, nullptr}},

    {0xD0, 6, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                        0, nullptr}},
    {0xD6, 1, X86InstInfo{"MOVQ2DQ",   TYPE_MMX, GenFlagsSameSize(SIZE_128BIT) | FLAGS_XMM_FLAGS,                     0, nullptr}},
    {0xD7, 1, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                        0, nullptr}},
    {0xD8, 8, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                        0, nullptr}},

    {0xE0, 6, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                        0, nullptr}},
    {0xE6, 1, X86InstInfo{"CVTDQ2PD",  TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,  0, nullptr}},
    {0xE7, 1, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                        0, nullptr}},
    {0xE8, 8, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                        0, nullptr}},

    {0xF0, 8, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                        0, nullptr}},
    {0xF8, 7, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                        0, nullptr}},
    {0xFF, 1, X86InstInfo{"",          TYPE_COPY_OTHER, FLAGS_NONE,                                     0, nullptr}},
  };

  const std::vector<std::tuple<uint8_t, uint8_t, X86InstInfo>> RepNEModOpTable = {
    {0x0, 16, X86InstInfo{"",           TYPE_COPY_OTHER, FLAGS_NONE,                                                     0, nullptr}},

    {0x10, 1, X86InstInfo{"MOVSD",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                  0, nullptr}},
    {0x11, 1, X86InstInfo{"MOVSD",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS,                  0, nullptr}},
    {0x12, 1, X86InstInfo{"MOVDDUP",    TYPE_INST, GenFlagsDstSize(SIZE_128BIT) | GenFlagsSrcSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                  0, nullptr}},
    {0x13, 6, X86InstInfo{"",           TYPE_INVALID, FLAGS_NONE,                                                        0, nullptr}},
    {0x19, 7, X86InstInfo{"",           TYPE_COPY_OTHER, FLAGS_NONE,                                                     0, nullptr}},

    {0x20, 4, X86InstInfo{"",          TYPE_COPY_OTHER, FLAGS_NONE,                                                      0, nullptr}},
    {0x24, 6, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                                         0, nullptr}},
    {0x2A, 1, X86InstInfo{"CVTSI2SD",  TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                    0, nullptr}},
    {0x2B, 1, X86InstInfo{"MOVNTSD",   TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS, 0, nullptr}},
    {0x2C, 1, X86InstInfo{"CVTTSD2SI", TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_DST_GPR, 0, nullptr}},
    {0x2D, 1, X86InstInfo{"CVTSD2SI",  TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_DST_GPR, 0, nullptr}},
    {0x2E, 2, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                                         0, nullptr}},

    {0x30, 16, X86InstInfo{"",         TYPE_COPY_OTHER, FLAGS_NONE,                                                      0, nullptr}},
    {0x40, 16, X86InstInfo{"",         TYPE_COPY_OTHER, FLAGS_NONE,                                                      0, nullptr}},

    {0x50, 1, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                                         0, nullptr}},
    {0x51, 1, X86InstInfo{"SQRTSD",    TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                    0, nullptr}},
    {0x52, 6, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                                         0, nullptr}},
    {0x58, 1, X86InstInfo{"ADDSD",     TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                    0, nullptr}},
    {0x59, 1, X86InstInfo{"MULSD",     TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                    0, nullptr}},
    {0x5A, 1, X86InstInfo{"CVTSD2SS",  TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                    0, nullptr}},
    {0x5B, 1, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                                         0, nullptr}},
    {0x5C, 1, X86InstInfo{"SUBSD",     TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                    0, nullptr}},
    {0x5D, 1, X86InstInfo{"MINSD",     TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                    0, nullptr}},
    {0x5E, 1, X86InstInfo{"DIVSD",     TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                    0, nullptr}},
    {0x5F, 1, X86InstInfo{"MAXSD",     TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                    0, nullptr}},

    {0x60, 16, X86InstInfo{"",         TYPE_INVALID, FLAGS_NONE,                                                         0, nullptr}},

    {0x70, 1, X86InstInfo{"PSHUFLW",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                   1, nullptr}},
    {0x71, 3, X86InstInfo{"",          TYPE_COPY_OTHER, FLAGS_NONE,                                                      0, nullptr}},
    {0x74, 4, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                                         0, nullptr}},
    {0x78, 1, X86InstInfo{"INSERTQ",   TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_SF_MOD_REG_ONLY | FLAGS_XMM_FLAGS,2, nullptr}},
    {0x79, 1, X86InstInfo{"INSERTQ",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_REG_ONLY | FLAGS_XMM_FLAGS, 0, nullptr}},
    {0x7A, 2, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                                         0, nullptr}},
    {0x7C, 1, X86InstInfo{"HADDPS",    TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                   0, nullptr}},
    {0x7D, 1, X86InstInfo{"HSUBPS",    TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                   0, nullptr}},
    {0x7E, 2, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                                         0, nullptr}},

    {0x80, 16, X86InstInfo{"",         TYPE_COPY_OTHER, FLAGS_NONE,                                                      0, nullptr}},
    {0x90, 16, X86InstInfo{"",         TYPE_COPY_OTHER, FLAGS_NONE,                                                      0, nullptr}},
    {0xA0, 16, X86InstInfo{"",         TYPE_COPY_OTHER, FLAGS_NONE,                                                      0, nullptr}},
    {0xB0, 8,  X86InstInfo{"",         TYPE_COPY_OTHER, FLAGS_NONE,                                                      0, nullptr}},
    {0xB8, 1, X86InstInfo{"",           TYPE_INVALID, FLAGS_NONE,                                                               0, nullptr}},
    {0xB9, 1, X86InstInfo{"",          TYPE_COPY_OTHER, FLAGS_NONE,                                        0, nullptr}},
    {0xBA, 1, X86InstInfo{"",          TYPE_COPY_OTHER, FLAGS_NONE,                                        0, nullptr}},
    {0xBB, 5,  X86InstInfo{"",         TYPE_INVALID, FLAGS_NONE,                                                         0, nullptr}},
    {0xC0, 2, X86InstInfo{"",          TYPE_COPY_OTHER, FLAGS_NONE,                                                      0, nullptr}},
    {0xC2, 1, X86InstInfo{"CMPSD",     TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                    1, nullptr}},
    {0xC3, 5, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                                         0, nullptr}},
    {0xC8, 8, X86InstInfo{"",          TYPE_COPY_OTHER, FLAGS_NONE,                                                      0, nullptr}},

    {0xD0, 1, X86InstInfo{"ADDSUBPS",  TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                   0, nullptr}},
    {0xD1, 5, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                                         0, nullptr}},
    {0xD6, 1, X86InstInfo{"MOVDQ2Q",   TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_XMM_FLAGS,                                       0, nullptr}},
    {0xD7, 1, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                                         0, nullptr}},
    {0xD8, 8, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                                         0, nullptr}},

    {0xE0, 6, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                                         0, nullptr}},
    {0xE6, 1, X86InstInfo{"CVTPD2DQ",  TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                   0, nullptr}},
    {0xE7, 1, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                                         0, nullptr}},
    {0xE8, 8, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                                         0, nullptr}},

    {0xF0, 1, X86InstInfo{"LDDQU",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY | FLAGS_XMM_FLAGS,0, nullptr}},
    {0xF1, 7, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                                         0, nullptr}},
    {0xF8, 8, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                                         0, nullptr}},
  };

  const std::vector<std::tuple<uint8_t, uint8_t, X86InstInfo>> OpSizeModOpTable = {
    {0x0, 16, X86InstInfo{"",           TYPE_COPY_OTHER, FLAGS_NONE,                                                            0, nullptr}},

    {0x10, 1, X86InstInfo{"MOVUPD",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0x11, 1, X86InstInfo{"MOVUPD",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0x12, 1, X86InstInfo{"MOVLPD",     TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY | FLAGS_XMM_FLAGS,      0, nullptr}},
    {0x13, 1, X86InstInfo{"MOVLPD",     TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS,      0, nullptr}},
    {0x14, 1, X86InstInfo{"UNPCKLPD",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0x15, 1, X86InstInfo{"UNPCKHPD",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0x16, 1, X86InstInfo{"MOVHPD",     TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_64BIT) | FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY | FLAGS_XMM_FLAGS,      0, nullptr}},
    {0x17, 1, X86InstInfo{"MOVHPD",     TYPE_INST, GenFlagsSizes(SIZE_64BIT, SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS,      0, nullptr}},
    {0x18, 1, X86InstInfo{"",           TYPE_INVALID, FLAGS_NONE,                                                               0, nullptr}},
    {0x19, 7, X86InstInfo{"",           TYPE_COPY_OTHER, FLAGS_NONE,                                                            0, nullptr}},

    {0x20, 4, X86InstInfo{"",           TYPE_COPY_OTHER, FLAGS_NONE,                                                            0, nullptr}},
    {0x24, 4, X86InstInfo{"",           TYPE_INVALID, FLAGS_NONE,                                                               0, nullptr}},

    {0x28, 1, X86InstInfo{"MOVAPD",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0x29, 1, X86InstInfo{"MOVAPD",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0x2A, 1, X86InstInfo{"CVTPI2PD",   TYPE_MMX, FLAGS_NONE,                                                                   0, nullptr}},
    {0x2B, 1, X86InstInfo{"MOVNTPD",    TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS,      0, nullptr}},
    {0x2C, 1, X86InstInfo{"CVTTPD2PI",  TYPE_MMX, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                          0, nullptr}},
    {0x2D, 1, X86InstInfo{"CVTPD2PI",   TYPE_MMX, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                          0, nullptr}},
    {0x2E, 1, X86InstInfo{"UCOMISD",    TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                          0, nullptr}},
    {0x2F, 1, X86InstInfo{"COMISD",     TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                          0, nullptr}},

    {0x30, 16, X86InstInfo{"",          TYPE_COPY_OTHER, FLAGS_NONE,                                                            0, nullptr}},
    {0x40, 16, X86InstInfo{"",          TYPE_COPY_OTHER, FLAGS_NONE,                                                            0, nullptr}},

    {0x50, 1, X86InstInfo{"MOVMSKPD",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_REG_ONLY | FLAGS_XMM_FLAGS | FLAGS_SF_DST_GPR,     0, nullptr}},
    {0x51, 1, X86InstInfo{"SQRTPD",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0x52, 2, X86InstInfo{"",           TYPE_INVALID, FLAGS_NONE,                                                               0, nullptr}},
    {0x54, 1, X86InstInfo{"ANDPD",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                        0, nullptr}},
    {0x55, 1, X86InstInfo{"ANDNPD",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0x56, 1, X86InstInfo{"ORPD",       TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0x57, 1, X86InstInfo{"XORPD",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0x58, 1, X86InstInfo{"ADDPD",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0x59, 1, X86InstInfo{"MULPD",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0x5A, 1, X86InstInfo{"CVTPD2PS",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0x5B, 1, X86InstInfo{"CVTPS2DQ",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0x5C, 1, X86InstInfo{"SUBPD",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0x5D, 1, X86InstInfo{"MINPD",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0x5E, 1, X86InstInfo{"DIVPD",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0x5F, 1, X86InstInfo{"MAXPD",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},

    {0x60, 1, X86InstInfo{"PUNPCKLBW",  TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0x61, 1, X86InstInfo{"PUNPCKLWD",  TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0x62, 1, X86InstInfo{"PUNPCKLDQ",  TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0x63, 1, X86InstInfo{"PACKSSWB",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0x64, 1, X86InstInfo{"PCMPGTB",    TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0x65, 1, X86InstInfo{"PCMPGTW",    TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0x66, 1, X86InstInfo{"PCMPGTD",    TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0x67, 1, X86InstInfo{"PACKUSWB",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0x68, 1, X86InstInfo{"PUNPCKHBW",  TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0x69, 1, X86InstInfo{"PUNPCKHWD",  TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0x6A, 1, X86InstInfo{"PUNPCKHDQ",  TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0x6B, 1, X86InstInfo{"PACKSSDW",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0x6C, 1, X86InstInfo{"PUNPCKLQDQ", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0x6D, 1, X86InstInfo{"PUNPCKHQDQ", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0x6E, 1, X86InstInfo{"MOVD",       TYPE_INST, GenFlagsDstSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_SRC_GPR | FLAGS_XMM_FLAGS,      0, nullptr}},
    {0x6F, 1, X86InstInfo{"MOVDQA",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},

    {0x70, 1, X86InstInfo{"PSHUFD",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         1, nullptr}},
    {0x71, 3, X86InstInfo{"",           TYPE_COPY_OTHER, FLAGS_NONE,                                                            0, nullptr}},
    {0x74, 1, X86InstInfo{"PCMPEQB",    TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0x75, 1, X86InstInfo{"PCMPEQW",    TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0x76, 1, X86InstInfo{"PCMPEQD",    TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0x77, 1, X86InstInfo{"",           TYPE_INVALID, FLAGS_NONE,                                                               0, nullptr}},
    {0x78, 1, X86InstInfo{"",           TYPE_GROUP_17, FLAGS_NONE,                                                              0, nullptr}},

    {0x79, 1, X86InstInfo{"EXTRQ",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_REG_ONLY | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0x7A, 2, X86InstInfo{"",           TYPE_INVALID, FLAGS_NONE,                                                               0, nullptr}},
    {0x7C, 1, X86InstInfo{"HADDPD",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0x7D, 1, X86InstInfo{"HSUBPD",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0x7E, 1, X86InstInfo{"MOVD",       TYPE_INST, GenFlagsSrcSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_DST_GPR | FLAGS_XMM_FLAGS,      0, nullptr}},
    {0x7F, 1, X86InstInfo{"MOVDQA",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS,                         0, nullptr}},

    {0x80, 16, X86InstInfo{"",          TYPE_COPY_OTHER, FLAGS_NONE,                                                            0, nullptr}},
    {0x90, 16, X86InstInfo{"",          TYPE_COPY_OTHER, FLAGS_NONE,                                                            0, nullptr}},
    {0xA0, 16, X86InstInfo{"",          TYPE_COPY_OTHER, FLAGS_NONE,                                                            0, nullptr}},
    {0xB0, 8, X86InstInfo{"",           TYPE_COPY_OTHER, FLAGS_NONE,                                                            0, nullptr}},
    {0xB8, 1, X86InstInfo{"",           TYPE_INVALID, FLAGS_NONE,                                                               0, nullptr}},
    {0xB9, 1, X86InstInfo{"",          TYPE_COPY_OTHER, FLAGS_NONE,                                        0, nullptr}},
    {0xBA, 1, X86InstInfo{"",          TYPE_COPY_OTHER, FLAGS_NONE,                                        0, nullptr}},
    {0xBB, 5, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                        0, nullptr}},

    {0xC0, 2, X86InstInfo{"",           TYPE_COPY_OTHER, FLAGS_NONE,                                                            0, nullptr}},
    {0xC2, 1, X86InstInfo{"CMPPD",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         1, nullptr}},
    {0xC3, 1, X86InstInfo{"",           TYPE_INVALID, FLAGS_NONE,                                                               0, nullptr}},
    {0xC4, 1, X86InstInfo{"PINSRW",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_SRC_GPR | FLAGS_XMM_FLAGS,      1, nullptr}},
    {0xC5, 1, X86InstInfo{"PEXTRW",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_REG_ONLY | FLAGS_SF_DST_GPR | FLAGS_XMM_FLAGS,      1, nullptr}},
    {0xC6, 1, X86InstInfo{"SHUFPD",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         1, nullptr}},
    {0xC7, 1, X86InstInfo{"",           TYPE_INVALID, FLAGS_NONE,                                                               0, nullptr}},
    {0xC8, 8, X86InstInfo{"",           TYPE_COPY_OTHER, FLAGS_NONE,                                                            0, nullptr}},

    {0xD0, 1, X86InstInfo{"ADDSUBPD",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0xD1, 1, X86InstInfo{"PSRLW",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0xD2, 1, X86InstInfo{"PSRLD",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0xD3, 1, X86InstInfo{"PSRLQ",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0xD4, 1, X86InstInfo{"PADDQ",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0xD5, 1, X86InstInfo{"PMULLW",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0xD6, 1, X86InstInfo{"MOVQ",       TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS,       0, nullptr}},
    {0xD7, 1, X86InstInfo{"PMOVMSKB",   TYPE_INST, GenFlagsSizes(SIZE_32BIT, SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_REG_ONLY | FLAGS_XMM_FLAGS | FLAGS_SF_DST_GPR,      0, nullptr}},
    {0xD8, 1, X86InstInfo{"PSUBUSB",    TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0xD9, 1, X86InstInfo{"PSUBUSW",    TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0xDA, 1, X86InstInfo{"PMINUB",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0xDB, 1, X86InstInfo{"PAND",       TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0xDC, 1, X86InstInfo{"PADDUSB",    TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0xDD, 1, X86InstInfo{"PADDUSW",    TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0xDE, 1, X86InstInfo{"PMAXUB",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0xDF, 1, X86InstInfo{"PANDN",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},

    {0xE0, 1, X86InstInfo{"PAVGB",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0xE1, 1, X86InstInfo{"PSRAW",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0xE2, 1, X86InstInfo{"PSRAD",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0xE3, 1, X86InstInfo{"PAVGW",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0xE4, 1, X86InstInfo{"PMULHUW",    TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0xE5, 1, X86InstInfo{"PMULHW",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0xE6, 1, X86InstInfo{"CVTTPD2DQ",  TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0xE7, 1, X86InstInfo{"MOVNTDQ",    TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS,      0, nullptr}},
    {0xE8, 1, X86InstInfo{"PSUBSB",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0xE9, 1, X86InstInfo{"PSUBSW",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0xEA, 1, X86InstInfo{"PMINSW",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0xEB, 1, X86InstInfo{"POR",        TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0xEC, 1, X86InstInfo{"PADDSB",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0xED, 1, X86InstInfo{"PADDSW",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0xEE, 1, X86InstInfo{"PMAXSW",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0xEF, 1, X86InstInfo{"PXOR",       TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},

    {0xF0, 1, X86InstInfo{"",           TYPE_INVALID, FLAGS_NONE,                                                               0, nullptr}},
    {0xF1, 1, X86InstInfo{"PSLLW",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0xF2, 1, X86InstInfo{"PSLLD",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0xF3, 1, X86InstInfo{"PSLLQ",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0xF4, 1, X86InstInfo{"PMULUDQ",    TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0xF5, 1, X86InstInfo{"PMADDWD",    TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0xF6, 1, X86InstInfo{"PSADBW",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0xF7, 1, X86InstInfo{"MASKMOVDQU", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_REG_ONLY | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0xF8, 1, X86InstInfo{"PSUBB",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0xF9, 1, X86InstInfo{"PSUBW",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0xFA, 1, X86InstInfo{"PSUBD",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0xFB, 1, X86InstInfo{"PSUBQ",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0xFC, 1, X86InstInfo{"PADDB",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0xFD, 1, X86InstInfo{"PADDW",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0, nullptr}},
    {0xFE, 1, X86InstInfo{"PADDD",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,       0, nullptr}},
    {0xFF, 1, X86InstInfo{"",           TYPE_COPY_OTHER, FLAGS_NONE,                                                            0, nullptr}},
  };

#define PF_NONE 0
#define PF_F3 1
#define PF_66 2
#define PF_F2 3
#define OPD(group, prefix, Reg) (((group - FEXCore::X86Tables::TYPE_GROUP_6) << 5) | (prefix) << 3 | (Reg))
  const std::vector<std::tuple<uint16_t, uint8_t, X86InstInfo>> SecondaryExtensionOpTable = {
    // GROUP 1
    // GROUP 2
    // GROUP 3
    // GROUP 4
    // GROUP 5
    // Pulls from other MODRM table

    // GROUP 6
    {OPD(TYPE_GROUP_6, PF_NONE, 0), 1, X86InstInfo{"SLDT",  TYPE_UNDEC, FLAGS_MODRM | FLAGS_SF_MOD_DST, 0, nullptr}},
    {OPD(TYPE_GROUP_6, PF_NONE, 1), 1, X86InstInfo{"STR",   TYPE_PRIV, FLAGS_MODRM | FLAGS_SF_MOD_DST,  0, nullptr}},
    {OPD(TYPE_GROUP_6, PF_NONE, 2), 1, X86InstInfo{"LLDT",  TYPE_PRIV, FLAGS_NONE,       0, nullptr}},
    {OPD(TYPE_GROUP_6, PF_NONE, 3), 1, X86InstInfo{"LTR",   TYPE_PRIV, FLAGS_NONE,       0, nullptr}},
    {OPD(TYPE_GROUP_6, PF_NONE, 4), 1, X86InstInfo{"VERR",  TYPE_UNDEC, FLAGS_MODRM, 0, nullptr}},
    {OPD(TYPE_GROUP_6, PF_NONE, 5), 1, X86InstInfo{"VERW",  TYPE_UNDEC, FLAGS_MODRM, 0, nullptr}},
    {OPD(TYPE_GROUP_6, PF_NONE, 6), 1, X86InstInfo{"",      TYPE_INVALID, FLAGS_NONE,    0, nullptr}},
    {OPD(TYPE_GROUP_6, PF_NONE, 7), 1, X86InstInfo{"",      TYPE_INVALID, FLAGS_NONE,    0, nullptr}},

    {OPD(TYPE_GROUP_6, PF_F3, 0), 1, X86InstInfo{"SLDT",    TYPE_UNDEC, FLAGS_MODRM | FLAGS_SF_MOD_DST, 0, nullptr}},
    {OPD(TYPE_GROUP_6, PF_F3, 1), 1, X86InstInfo{"STR",     TYPE_PRIV, FLAGS_MODRM | FLAGS_SF_MOD_DST,  0, nullptr}},
    {OPD(TYPE_GROUP_6, PF_F3, 2), 1, X86InstInfo{"LLDT",    TYPE_PRIV, FLAGS_NONE,       0, nullptr}},
    {OPD(TYPE_GROUP_6, PF_F3, 3), 1, X86InstInfo{"LTR",     TYPE_PRIV, FLAGS_NONE,       0, nullptr}},
    {OPD(TYPE_GROUP_6, PF_F3, 4), 1, X86InstInfo{"VERR",    TYPE_UNDEC, FLAGS_MODRM, 0, nullptr}},
    {OPD(TYPE_GROUP_6, PF_F3, 5), 1, X86InstInfo{"VERW",    TYPE_UNDEC, FLAGS_MODRM, 0, nullptr}},
    {OPD(TYPE_GROUP_6, PF_F3, 6), 1, X86InstInfo{"",        TYPE_INVALID, FLAGS_NONE,    0, nullptr}},
    {OPD(TYPE_GROUP_6, PF_F3, 7), 1, X86InstInfo{"",        TYPE_INVALID, FLAGS_NONE,    0, nullptr}},

    {OPD(TYPE_GROUP_6, PF_66, 0), 1, X86InstInfo{"SLDT",    TYPE_UNDEC, FLAGS_MODRM | FLAGS_SF_MOD_DST, 0, nullptr}},
    {OPD(TYPE_GROUP_6, PF_66, 1), 1, X86InstInfo{"STR",     TYPE_PRIV, FLAGS_MODRM | FLAGS_SF_MOD_DST,  0, nullptr}},
    {OPD(TYPE_GROUP_6, PF_66, 2), 1, X86InstInfo{"LLDT",    TYPE_PRIV, FLAGS_NONE,       0, nullptr}},
    {OPD(TYPE_GROUP_6, PF_66, 3), 1, X86InstInfo{"LTR",     TYPE_PRIV, FLAGS_NONE,       0, nullptr}},
    {OPD(TYPE_GROUP_6, PF_66, 4), 1, X86InstInfo{"VERR",    TYPE_UNDEC, FLAGS_MODRM, 0, nullptr}},
    {OPD(TYPE_GROUP_6, PF_66, 5), 1, X86InstInfo{"VERW",    TYPE_UNDEC, FLAGS_MODRM, 0, nullptr}},
    {OPD(TYPE_GROUP_6, PF_66, 6), 1, X86InstInfo{"",        TYPE_INVALID, FLAGS_NONE,    0, nullptr}},
    {OPD(TYPE_GROUP_6, PF_66, 7), 1, X86InstInfo{"",        TYPE_INVALID, FLAGS_NONE,    0, nullptr}},

    {OPD(TYPE_GROUP_6, PF_F2, 0), 1, X86InstInfo{"SLDT",    TYPE_UNDEC, FLAGS_MODRM | FLAGS_SF_MOD_DST, 0, nullptr}},
    {OPD(TYPE_GROUP_6, PF_F2, 1), 1, X86InstInfo{"STR",     TYPE_PRIV, FLAGS_MODRM | FLAGS_SF_MOD_DST,  0, nullptr}},
    {OPD(TYPE_GROUP_6, PF_F2, 2), 1, X86InstInfo{"LLDT",    TYPE_PRIV, FLAGS_NONE,       0, nullptr}},
    {OPD(TYPE_GROUP_6, PF_F2, 3), 1, X86InstInfo{"LTR",     TYPE_PRIV, FLAGS_NONE,       0, nullptr}},
    {OPD(TYPE_GROUP_6, PF_F2, 4), 1, X86InstInfo{"VERR",    TYPE_UNDEC, FLAGS_MODRM, 0, nullptr}},
    {OPD(TYPE_GROUP_6, PF_F2, 5), 1, X86InstInfo{"VERW",    TYPE_UNDEC, FLAGS_MODRM, 0, nullptr}},
    {OPD(TYPE_GROUP_6, PF_F2, 6), 1, X86InstInfo{"",        TYPE_INVALID, FLAGS_NONE,    0, nullptr}},
    {OPD(TYPE_GROUP_6, PF_F2, 7), 1, X86InstInfo{"",        TYPE_INVALID, FLAGS_NONE,    0, nullptr}},

    // GROUP 7
    {OPD(TYPE_GROUP_7, PF_NONE, 0), 1, X86InstInfo{"SGDT", TYPE_UNDEC, FLAGS_MODRM | FLAGS_SF_MOD_DST,         0, nullptr}},
    {OPD(TYPE_GROUP_7, PF_NONE, 1), 1, X86InstInfo{"",     TYPE_SECOND_GROUP_MODRM, FLAGS_NONE, 0, nullptr}},
    {OPD(TYPE_GROUP_7, PF_NONE, 2), 1, X86InstInfo{"",     TYPE_SECOND_GROUP_MODRM, FLAGS_NONE, 0, nullptr}},
    {OPD(TYPE_GROUP_7, PF_NONE, 3), 1, X86InstInfo{"",     TYPE_SECOND_GROUP_MODRM, FLAGS_NONE, 0, nullptr}},
    {OPD(TYPE_GROUP_7, PF_NONE, 4), 1, X86InstInfo{"SMSW", TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,          0, nullptr}},
    {OPD(TYPE_GROUP_7, PF_NONE, 5), 1, X86InstInfo{"",     TYPE_INVALID, FLAGS_NONE,            0, nullptr}},
    {OPD(TYPE_GROUP_7, PF_NONE, 6), 1, X86InstInfo{"LMSW", TYPE_PRIV, FLAGS_MODRM,          0, nullptr}},
    {OPD(TYPE_GROUP_7, PF_NONE, 7), 1, X86InstInfo{"",     TYPE_SECOND_GROUP_MODRM, FLAGS_NONE, 0, nullptr}},

    {OPD(TYPE_GROUP_7, PF_F3, 0), 1, X86InstInfo{"SGDT", TYPE_UNDEC, FLAGS_MODRM | FLAGS_SF_MOD_DST,           0, nullptr}},
    {OPD(TYPE_GROUP_7, PF_F3, 1), 1, X86InstInfo{"",     TYPE_SECOND_GROUP_MODRM, FLAGS_NONE,   0, nullptr}},
    {OPD(TYPE_GROUP_7, PF_F3, 2), 1, X86InstInfo{"",     TYPE_SECOND_GROUP_MODRM, FLAGS_NONE,   0, nullptr}},
    {OPD(TYPE_GROUP_7, PF_F3, 3), 1, X86InstInfo{"",     TYPE_SECOND_GROUP_MODRM, FLAGS_NONE,   0, nullptr}},
    {OPD(TYPE_GROUP_7, PF_F3, 4), 1, X86InstInfo{"SMSW", TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,            0, nullptr}},
    {OPD(TYPE_GROUP_7, PF_F3, 5), 1, X86InstInfo{"",     TYPE_INVALID, FLAGS_NONE,              0, nullptr}},
    {OPD(TYPE_GROUP_7, PF_F3, 6), 1, X86InstInfo{"LMSW", TYPE_PRIV, FLAGS_MODRM,            0, nullptr}},
    {OPD(TYPE_GROUP_7, PF_F3, 7), 1, X86InstInfo{"",     TYPE_SECOND_GROUP_MODRM, FLAGS_NONE,   0, nullptr}},

    {OPD(TYPE_GROUP_7, PF_66, 0), 1, X86InstInfo{"SGDT", TYPE_UNDEC, FLAGS_MODRM | FLAGS_SF_MOD_DST,           0, nullptr}},
    {OPD(TYPE_GROUP_7, PF_66, 1), 1, X86InstInfo{"",     TYPE_SECOND_GROUP_MODRM, FLAGS_NONE,   0, nullptr}},
    {OPD(TYPE_GROUP_7, PF_66, 2), 1, X86InstInfo{"",     TYPE_SECOND_GROUP_MODRM, FLAGS_NONE,   0, nullptr}},
    {OPD(TYPE_GROUP_7, PF_66, 3), 1, X86InstInfo{"",     TYPE_SECOND_GROUP_MODRM, FLAGS_NONE,   0, nullptr}},
    {OPD(TYPE_GROUP_7, PF_66, 4), 1, X86InstInfo{"SMSW", TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,            0, nullptr}},
    {OPD(TYPE_GROUP_7, PF_66, 5), 1, X86InstInfo{"",     TYPE_INVALID, FLAGS_NONE,              0, nullptr}},
    {OPD(TYPE_GROUP_7, PF_66, 6), 1, X86InstInfo{"LMSW", TYPE_PRIV, FLAGS_MODRM,            0, nullptr}},
    {OPD(TYPE_GROUP_7, PF_66, 7), 1, X86InstInfo{"",     TYPE_SECOND_GROUP_MODRM, FLAGS_NONE,   0, nullptr}},

    {OPD(TYPE_GROUP_7, PF_F2, 0), 1, X86InstInfo{"SGDT", TYPE_UNDEC, FLAGS_MODRM | FLAGS_SF_MOD_DST,           0, nullptr}},
    {OPD(TYPE_GROUP_7, PF_F2, 1), 1, X86InstInfo{"",     TYPE_SECOND_GROUP_MODRM, FLAGS_NONE,   0, nullptr}},
    {OPD(TYPE_GROUP_7, PF_F2, 2), 1, X86InstInfo{"",     TYPE_SECOND_GROUP_MODRM, FLAGS_NONE,   0, nullptr}},
    {OPD(TYPE_GROUP_7, PF_F2, 3), 1, X86InstInfo{"",     TYPE_SECOND_GROUP_MODRM, FLAGS_NONE,   0, nullptr}},
    {OPD(TYPE_GROUP_7, PF_F2, 4), 1, X86InstInfo{"SMSW", TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,            0, nullptr}},
    {OPD(TYPE_GROUP_7, PF_F2, 5), 1, X86InstInfo{"",     TYPE_INVALID, FLAGS_NONE,              0, nullptr}},
    {OPD(TYPE_GROUP_7, PF_F2, 6), 1, X86InstInfo{"LMSW", TYPE_PRIV, FLAGS_MODRM,            0, nullptr}},
    {OPD(TYPE_GROUP_7, PF_F2, 7), 1, X86InstInfo{"",     TYPE_SECOND_GROUP_MODRM, FLAGS_NONE,   0, nullptr}},

    // GROUP 8
    {OPD(TYPE_GROUP_8, PF_NONE, 0), 1, X86InstInfo{"",    TYPE_INVALID, FLAGS_NONE,   0, nullptr}},
    {OPD(TYPE_GROUP_8, PF_NONE, 1), 1, X86InstInfo{"",    TYPE_INVALID, FLAGS_NONE,   0, nullptr}},
    {OPD(TYPE_GROUP_8, PF_NONE, 2), 1, X86InstInfo{"",    TYPE_INVALID, FLAGS_NONE,   0, nullptr}},
    {OPD(TYPE_GROUP_8, PF_NONE, 3), 1, X86InstInfo{"",    TYPE_INVALID, FLAGS_NONE,   0, nullptr}},
    {OPD(TYPE_GROUP_8, PF_NONE, 4), 1, X86InstInfo{"BT",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST, 1, nullptr}},
    {OPD(TYPE_GROUP_8, PF_NONE, 5), 1, X86InstInfo{"BTS", TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST, 1, nullptr}},
    {OPD(TYPE_GROUP_8, PF_NONE, 6), 1, X86InstInfo{"BTR", TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST, 1, nullptr}},
    {OPD(TYPE_GROUP_8, PF_NONE, 7), 1, X86InstInfo{"BTC", TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST, 1, nullptr}},

    {OPD(TYPE_GROUP_8, PF_F3, 0), 1, X86InstInfo{"",    TYPE_INVALID, FLAGS_NONE,     0, nullptr}},
    {OPD(TYPE_GROUP_8, PF_F3, 1), 1, X86InstInfo{"",    TYPE_INVALID, FLAGS_NONE,     0, nullptr}},
    {OPD(TYPE_GROUP_8, PF_F3, 2), 1, X86InstInfo{"",    TYPE_INVALID, FLAGS_NONE,     0, nullptr}},
    {OPD(TYPE_GROUP_8, PF_F3, 3), 1, X86InstInfo{"",    TYPE_INVALID, FLAGS_NONE,     0, nullptr}},
    {OPD(TYPE_GROUP_8, PF_F3, 4), 1, X86InstInfo{"BT",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,   1, nullptr}},
    {OPD(TYPE_GROUP_8, PF_F3, 5), 1, X86InstInfo{"BTS", TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,   1, nullptr}},
    {OPD(TYPE_GROUP_8, PF_F3, 6), 1, X86InstInfo{"BTR", TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,   1, nullptr}},
    {OPD(TYPE_GROUP_8, PF_F3, 7), 1, X86InstInfo{"BTC", TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,   1, nullptr}},

    {OPD(TYPE_GROUP_8, PF_66, 0), 1, X86InstInfo{"",    TYPE_INVALID, FLAGS_NONE,     0, nullptr}},
    {OPD(TYPE_GROUP_8, PF_66, 1), 1, X86InstInfo{"",    TYPE_INVALID, FLAGS_NONE,     0, nullptr}},
    {OPD(TYPE_GROUP_8, PF_66, 2), 1, X86InstInfo{"",    TYPE_INVALID, FLAGS_NONE,     0, nullptr}},
    {OPD(TYPE_GROUP_8, PF_66, 3), 1, X86InstInfo{"",    TYPE_INVALID, FLAGS_NONE,     0, nullptr}},
    {OPD(TYPE_GROUP_8, PF_66, 4), 1, X86InstInfo{"BT",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,   1, nullptr}},
    {OPD(TYPE_GROUP_8, PF_66, 5), 1, X86InstInfo{"BTS", TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,   1, nullptr}},
    {OPD(TYPE_GROUP_8, PF_66, 6), 1, X86InstInfo{"BTR", TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,   1, nullptr}},
    {OPD(TYPE_GROUP_8, PF_66, 7), 1, X86InstInfo{"BTC", TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,   1, nullptr}},

    {OPD(TYPE_GROUP_8, PF_F2, 0), 1, X86InstInfo{"",    TYPE_INVALID, FLAGS_NONE,     0, nullptr}},
    {OPD(TYPE_GROUP_8, PF_F2, 1), 1, X86InstInfo{"",    TYPE_INVALID, FLAGS_NONE,     0, nullptr}},
    {OPD(TYPE_GROUP_8, PF_F2, 2), 1, X86InstInfo{"",    TYPE_INVALID, FLAGS_NONE,     0, nullptr}},
    {OPD(TYPE_GROUP_8, PF_F2, 3), 1, X86InstInfo{"",    TYPE_INVALID, FLAGS_NONE,     0, nullptr}},
    {OPD(TYPE_GROUP_8, PF_F2, 4), 1, X86InstInfo{"BT",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,   1, nullptr}},
    {OPD(TYPE_GROUP_8, PF_F2, 5), 1, X86InstInfo{"BTS", TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,   1, nullptr}},
    {OPD(TYPE_GROUP_8, PF_F2, 6), 1, X86InstInfo{"BTR", TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,   1, nullptr}},
    {OPD(TYPE_GROUP_8, PF_F2, 7), 1, X86InstInfo{"BTC", TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,   1, nullptr}},

    // GROUP 9

    // AMD documentation is a bit broken for Group 9
    // Claims the entire group has n/a applied for the prefix (Implies that the prefix is ignored)
    // RDRAND/RDSEED only work with no prefix
    // CMPXCHG8B/16B works with all prefixes
    // Tooling fails to decode CMPXCHG with prefix
    {OPD(TYPE_GROUP_9, PF_NONE, 0), 1, X86InstInfo{"",           TYPE_INVALID, FLAGS_NONE,   0, nullptr}},
    {OPD(TYPE_GROUP_9, PF_NONE, 1), 1, X86InstInfo{"CMPXCHG16B", TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_MOD_MEM_ONLY, 0, nullptr}},
    {OPD(TYPE_GROUP_9, PF_NONE, 2), 1, X86InstInfo{"",           TYPE_INVALID, FLAGS_NONE,   0, nullptr}},
    {OPD(TYPE_GROUP_9, PF_NONE, 3), 1, X86InstInfo{"",           TYPE_INVALID, FLAGS_NONE,   0, nullptr}},
    {OPD(TYPE_GROUP_9, PF_NONE, 4), 1, X86InstInfo{"",           TYPE_INVALID, FLAGS_NONE,   0, nullptr}},
    {OPD(TYPE_GROUP_9, PF_NONE, 5), 1, X86InstInfo{"",           TYPE_INVALID, FLAGS_NONE,   0, nullptr}},
    {OPD(TYPE_GROUP_9, PF_NONE, 6), 1, X86InstInfo{"RDRAND",     TYPE_UNDEC, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_MOD_REG_ONLY, 0, nullptr}},
    {OPD(TYPE_GROUP_9, PF_NONE, 7), 1, X86InstInfo{"RDSEED",     TYPE_UNDEC, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_MOD_REG_ONLY, 0, nullptr}},

    {OPD(TYPE_GROUP_9, PF_F3, 0), 1, X86InstInfo{"",           TYPE_INVALID, FLAGS_NONE,     0, nullptr}},
    {OPD(TYPE_GROUP_9, PF_F3, 1), 1, X86InstInfo{"CMPXCHG16B", TYPE_INVALID, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_9, PF_F3, 2), 1, X86InstInfo{"",           TYPE_INVALID, FLAGS_NONE,     0, nullptr}},
    {OPD(TYPE_GROUP_9, PF_F3, 3), 1, X86InstInfo{"",           TYPE_INVALID, FLAGS_NONE,     0, nullptr}},
    {OPD(TYPE_GROUP_9, PF_F3, 4), 1, X86InstInfo{"",           TYPE_INVALID, FLAGS_NONE,     0, nullptr}},
    {OPD(TYPE_GROUP_9, PF_F3, 5), 1, X86InstInfo{"",           TYPE_INVALID, FLAGS_NONE,     0, nullptr}},
    {OPD(TYPE_GROUP_9, PF_F3, 6), 1, X86InstInfo{"",           TYPE_INVALID, FLAGS_NONE,     0, nullptr}},
    {OPD(TYPE_GROUP_9, PF_F3, 7), 1, X86InstInfo{"",           TYPE_INVALID, FLAGS_NONE,     0, nullptr}},

    {OPD(TYPE_GROUP_9, PF_66, 0), 1, X86InstInfo{"",           TYPE_INVALID, FLAGS_NONE,     0, nullptr}},
    {OPD(TYPE_GROUP_9, PF_66, 1), 1, X86InstInfo{"CMPXCHG16B", TYPE_INVALID, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_9, PF_66, 2), 1, X86InstInfo{"",           TYPE_INVALID, FLAGS_NONE,     0, nullptr}},
    {OPD(TYPE_GROUP_9, PF_66, 3), 1, X86InstInfo{"",           TYPE_INVALID, FLAGS_NONE,     0, nullptr}},
    {OPD(TYPE_GROUP_9, PF_66, 4), 1, X86InstInfo{"",           TYPE_INVALID, FLAGS_NONE,     0, nullptr}},
    {OPD(TYPE_GROUP_9, PF_66, 5), 1, X86InstInfo{"",           TYPE_INVALID, FLAGS_NONE,     0, nullptr}},
    {OPD(TYPE_GROUP_9, PF_66, 6), 1, X86InstInfo{"",           TYPE_INVALID, FLAGS_NONE,     0, nullptr}},
    {OPD(TYPE_GROUP_9, PF_66, 7), 1, X86InstInfo{"",           TYPE_INVALID, FLAGS_NONE,     0, nullptr}},

    {OPD(TYPE_GROUP_9, PF_F2, 0), 1, X86InstInfo{"",           TYPE_INVALID, FLAGS_NONE,     0, nullptr}},
    {OPD(TYPE_GROUP_9, PF_F2, 1), 1, X86InstInfo{"CMPXCHG16B", TYPE_INVALID, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_9, PF_F2, 2), 1, X86InstInfo{"",           TYPE_INVALID, FLAGS_NONE,     0, nullptr}},
    {OPD(TYPE_GROUP_9, PF_F2, 3), 1, X86InstInfo{"",           TYPE_INVALID, FLAGS_NONE,     0, nullptr}},
    {OPD(TYPE_GROUP_9, PF_F2, 4), 1, X86InstInfo{"",           TYPE_INVALID, FLAGS_NONE,     0, nullptr}},
    {OPD(TYPE_GROUP_9, PF_F2, 5), 1, X86InstInfo{"",           TYPE_INVALID, FLAGS_NONE,     0, nullptr}},
    {OPD(TYPE_GROUP_9, PF_F2, 6), 1, X86InstInfo{"",           TYPE_INVALID, FLAGS_NONE,     0, nullptr}},
    {OPD(TYPE_GROUP_9, PF_F2, 7), 1, X86InstInfo{"",           TYPE_INVALID, FLAGS_NONE,     0, nullptr}},

    // GROUP 10
    {OPD(TYPE_GROUP_10, PF_NONE, 0), 1, X86InstInfo{"UD1", TYPE_INST, FLAGS_DEBUG | FLAGS_BLOCK_END, 0, nullptr}},
    {OPD(TYPE_GROUP_10, PF_NONE, 1), 1, X86InstInfo{"UD1", TYPE_INST, FLAGS_DEBUG | FLAGS_BLOCK_END, 0, nullptr}},
    {OPD(TYPE_GROUP_10, PF_NONE, 2), 1, X86InstInfo{"UD1", TYPE_INST, FLAGS_DEBUG | FLAGS_BLOCK_END, 0, nullptr}},
    {OPD(TYPE_GROUP_10, PF_NONE, 3), 1, X86InstInfo{"UD1", TYPE_INST, FLAGS_DEBUG | FLAGS_BLOCK_END, 0, nullptr}},
    {OPD(TYPE_GROUP_10, PF_NONE, 4), 1, X86InstInfo{"UD1", TYPE_INST, FLAGS_DEBUG | FLAGS_BLOCK_END, 0, nullptr}},
    {OPD(TYPE_GROUP_10, PF_NONE, 5), 1, X86InstInfo{"UD1", TYPE_INST, FLAGS_DEBUG | FLAGS_BLOCK_END, 0, nullptr}},
    {OPD(TYPE_GROUP_10, PF_NONE, 6), 1, X86InstInfo{"UD1", TYPE_INST, FLAGS_DEBUG | FLAGS_BLOCK_END, 0, nullptr}},
    {OPD(TYPE_GROUP_10, PF_NONE, 7), 1, X86InstInfo{"UD1", TYPE_INST, FLAGS_DEBUG | FLAGS_BLOCK_END, 0, nullptr}},

    {OPD(TYPE_GROUP_10, PF_F3, 0), 1, X86InstInfo{"UD1", TYPE_INST, FLAGS_DEBUG | FLAGS_BLOCK_END,   0, nullptr}},
    {OPD(TYPE_GROUP_10, PF_F3, 1), 1, X86InstInfo{"UD1", TYPE_INST, FLAGS_DEBUG | FLAGS_BLOCK_END,   0, nullptr}},
    {OPD(TYPE_GROUP_10, PF_F3, 2), 1, X86InstInfo{"UD1", TYPE_INST, FLAGS_DEBUG | FLAGS_BLOCK_END,   0, nullptr}},
    {OPD(TYPE_GROUP_10, PF_F3, 3), 1, X86InstInfo{"UD1", TYPE_INST, FLAGS_DEBUG | FLAGS_BLOCK_END,   0, nullptr}},
    {OPD(TYPE_GROUP_10, PF_F3, 4), 1, X86InstInfo{"UD1", TYPE_INST, FLAGS_DEBUG | FLAGS_BLOCK_END,   0, nullptr}},
    {OPD(TYPE_GROUP_10, PF_F3, 5), 1, X86InstInfo{"UD1", TYPE_INST, FLAGS_DEBUG | FLAGS_BLOCK_END,   0, nullptr}},
    {OPD(TYPE_GROUP_10, PF_F3, 6), 1, X86InstInfo{"UD1", TYPE_INST, FLAGS_DEBUG | FLAGS_BLOCK_END,   0, nullptr}},
    {OPD(TYPE_GROUP_10, PF_F3, 7), 1, X86InstInfo{"UD1", TYPE_INST, FLAGS_DEBUG | FLAGS_BLOCK_END,   0, nullptr}},

    {OPD(TYPE_GROUP_10, PF_66, 0), 1, X86InstInfo{"UD1", TYPE_INST, FLAGS_DEBUG | FLAGS_BLOCK_END,   0, nullptr}},
    {OPD(TYPE_GROUP_10, PF_66, 1), 1, X86InstInfo{"UD1", TYPE_INST, FLAGS_DEBUG | FLAGS_BLOCK_END,   0, nullptr}},
    {OPD(TYPE_GROUP_10, PF_66, 2), 1, X86InstInfo{"UD1", TYPE_INST, FLAGS_DEBUG | FLAGS_BLOCK_END,   0, nullptr}},
    {OPD(TYPE_GROUP_10, PF_66, 3), 1, X86InstInfo{"UD1", TYPE_INST, FLAGS_DEBUG | FLAGS_BLOCK_END,   0, nullptr}},
    {OPD(TYPE_GROUP_10, PF_66, 4), 1, X86InstInfo{"UD1", TYPE_INST, FLAGS_DEBUG | FLAGS_BLOCK_END,   0, nullptr}},
    {OPD(TYPE_GROUP_10, PF_66, 5), 1, X86InstInfo{"UD1", TYPE_INST, FLAGS_DEBUG | FLAGS_BLOCK_END,   0, nullptr}},
    {OPD(TYPE_GROUP_10, PF_66, 6), 1, X86InstInfo{"UD1", TYPE_INST, FLAGS_DEBUG | FLAGS_BLOCK_END,   0, nullptr}},
    {OPD(TYPE_GROUP_10, PF_66, 7), 1, X86InstInfo{"UD1", TYPE_INST, FLAGS_DEBUG | FLAGS_BLOCK_END,   0, nullptr}},

    {OPD(TYPE_GROUP_10, PF_F2, 0), 1, X86InstInfo{"UD1", TYPE_INST, FLAGS_DEBUG | FLAGS_BLOCK_END,   0, nullptr}},
    {OPD(TYPE_GROUP_10, PF_F2, 1), 1, X86InstInfo{"UD1", TYPE_INST, FLAGS_DEBUG | FLAGS_BLOCK_END,   0, nullptr}},
    {OPD(TYPE_GROUP_10, PF_F2, 2), 1, X86InstInfo{"UD1", TYPE_INST, FLAGS_DEBUG | FLAGS_BLOCK_END,   0, nullptr}},
    {OPD(TYPE_GROUP_10, PF_F2, 3), 1, X86InstInfo{"UD1", TYPE_INST, FLAGS_DEBUG | FLAGS_BLOCK_END,   0, nullptr}},
    {OPD(TYPE_GROUP_10, PF_F2, 4), 1, X86InstInfo{"UD1", TYPE_INST, FLAGS_DEBUG | FLAGS_BLOCK_END,   0, nullptr}},
    {OPD(TYPE_GROUP_10, PF_F2, 5), 1, X86InstInfo{"UD1", TYPE_INST, FLAGS_DEBUG | FLAGS_BLOCK_END,   0, nullptr}},
    {OPD(TYPE_GROUP_10, PF_F2, 6), 1, X86InstInfo{"UD1", TYPE_INST, FLAGS_DEBUG | FLAGS_BLOCK_END,   0, nullptr}},
    {OPD(TYPE_GROUP_10, PF_F2, 7), 1, X86InstInfo{"UD1", TYPE_INST, FLAGS_DEBUG | FLAGS_BLOCK_END,   0, nullptr}},

    // GROUP 12
    {OPD(TYPE_GROUP_12, PF_NONE, 0), 1, X86InstInfo{"",      TYPE_INVALID, FLAGS_NONE,                                      0, nullptr}},
    {OPD(TYPE_GROUP_12, PF_NONE, 1), 1, X86InstInfo{"",      TYPE_INVALID, FLAGS_NONE,                                      0, nullptr}},
    {OPD(TYPE_GROUP_12, PF_NONE, 2), 1, X86InstInfo{"PSRLW", TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS, 1, nullptr}},
    {OPD(TYPE_GROUP_12, PF_NONE, 3), 1, X86InstInfo{"",      TYPE_INVALID, FLAGS_NONE,                                      0, nullptr}},
    {OPD(TYPE_GROUP_12, PF_NONE, 4), 1, X86InstInfo{"PSRAW", TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS, 1, nullptr}},
    {OPD(TYPE_GROUP_12, PF_NONE, 5), 1, X86InstInfo{"",      TYPE_INVALID, FLAGS_NONE,                                      0, nullptr}},
    {OPD(TYPE_GROUP_12, PF_NONE, 6), 1, X86InstInfo{"PSLLW", TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS, 1, nullptr}},
    {OPD(TYPE_GROUP_12, PF_NONE, 7), 1, X86InstInfo{"",      TYPE_INVALID, FLAGS_NONE,                                      0, nullptr}},

    {OPD(TYPE_GROUP_12, PF_66, 0), 1, X86InstInfo{"",      TYPE_INVALID, FLAGS_NONE,                                        0, nullptr}},
    {OPD(TYPE_GROUP_12, PF_66, 1), 1, X86InstInfo{"",      TYPE_INVALID, FLAGS_NONE,                                        0, nullptr}},
    {OPD(TYPE_GROUP_12, PF_66, 2), 1, X86InstInfo{"PSRLW", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_MOD_REG_ONLY | FLAGS_XMM_FLAGS,  1, nullptr}},
    {OPD(TYPE_GROUP_12, PF_66, 3), 1, X86InstInfo{"",      TYPE_INVALID, FLAGS_NONE,                                        0, nullptr}},
    {OPD(TYPE_GROUP_12, PF_66, 4), 1, X86InstInfo{"PSRAW", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_MOD_REG_ONLY | FLAGS_XMM_FLAGS,  1, nullptr}},
    {OPD(TYPE_GROUP_12, PF_66, 5), 1, X86InstInfo{"",      TYPE_INVALID, FLAGS_NONE,                                        0, nullptr}},
    {OPD(TYPE_GROUP_12, PF_66, 6), 1, X86InstInfo{"PSLLW", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_MOD_REG_ONLY | FLAGS_XMM_FLAGS,  1, nullptr}},
    {OPD(TYPE_GROUP_12, PF_66, 7), 1, X86InstInfo{"",      TYPE_INVALID, FLAGS_NONE,                                        0, nullptr}},

    {OPD(TYPE_GROUP_12, PF_F3, 0), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                             0, nullptr}},
    {OPD(TYPE_GROUP_12, PF_F3, 1), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                             0, nullptr}},
    {OPD(TYPE_GROUP_12, PF_F3, 2), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                             0, nullptr}},
    {OPD(TYPE_GROUP_12, PF_F3, 3), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                             0, nullptr}},
    {OPD(TYPE_GROUP_12, PF_F3, 4), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                             0, nullptr}},
    {OPD(TYPE_GROUP_12, PF_F3, 5), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                             0, nullptr}},
    {OPD(TYPE_GROUP_12, PF_F3, 6), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                             0, nullptr}},
    {OPD(TYPE_GROUP_12, PF_F3, 7), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                             0, nullptr}},

    {OPD(TYPE_GROUP_12, PF_F2, 0), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                             0, nullptr}},
    {OPD(TYPE_GROUP_12, PF_F2, 1), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                             0, nullptr}},
    {OPD(TYPE_GROUP_12, PF_F2, 2), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                             0, nullptr}},
    {OPD(TYPE_GROUP_12, PF_F2, 3), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                             0, nullptr}},
    {OPD(TYPE_GROUP_12, PF_F2, 4), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                             0, nullptr}},
    {OPD(TYPE_GROUP_12, PF_F2, 5), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                             0, nullptr}},
    {OPD(TYPE_GROUP_12, PF_F2, 6), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                             0, nullptr}},
    {OPD(TYPE_GROUP_12, PF_F2, 7), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                             0, nullptr}},

    // GROUP 13
    {OPD(TYPE_GROUP_13, PF_NONE, 0), 1, X86InstInfo{"",      TYPE_INVALID, FLAGS_NONE,                                      0, nullptr}},
    {OPD(TYPE_GROUP_13, PF_NONE, 1), 1, X86InstInfo{"",      TYPE_INVALID, FLAGS_NONE,                                      0, nullptr}},
    {OPD(TYPE_GROUP_13, PF_NONE, 2), 1, X86InstInfo{"PSRLD", TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS, 1, nullptr}},
    {OPD(TYPE_GROUP_13, PF_NONE, 3), 1, X86InstInfo{"",      TYPE_INVALID, FLAGS_NONE,                                      0, nullptr}},
    {OPD(TYPE_GROUP_13, PF_NONE, 4), 1, X86InstInfo{"PSRAD", TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS, 1, nullptr}},
    {OPD(TYPE_GROUP_13, PF_NONE, 5), 1, X86InstInfo{"",      TYPE_INVALID, FLAGS_NONE,                                      0, nullptr}},
    {OPD(TYPE_GROUP_13, PF_NONE, 6), 1, X86InstInfo{"PSLLD", TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS, 1, nullptr}},
    {OPD(TYPE_GROUP_13, PF_NONE, 7), 1, X86InstInfo{"",      TYPE_INVALID, FLAGS_NONE,                                      0, nullptr}},

    {OPD(TYPE_GROUP_13, PF_66, 0), 1, X86InstInfo{"",      TYPE_INVALID, FLAGS_NONE,                                        0, nullptr}},
    {OPD(TYPE_GROUP_13, PF_66, 1), 1, X86InstInfo{"",      TYPE_INVALID, FLAGS_NONE,                                        0, nullptr}},
    {OPD(TYPE_GROUP_13, PF_66, 2), 1, X86InstInfo{"PSRLD", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_MOD_REG_ONLY | FLAGS_XMM_FLAGS,  1, nullptr}},
    {OPD(TYPE_GROUP_13, PF_66, 3), 1, X86InstInfo{"",      TYPE_INVALID, FLAGS_NONE,                                        0, nullptr}},
    {OPD(TYPE_GROUP_13, PF_66, 4), 1, X86InstInfo{"PSRAD", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_MOD_REG_ONLY | FLAGS_XMM_FLAGS,  1, nullptr}},
    {OPD(TYPE_GROUP_13, PF_66, 5), 1, X86InstInfo{"",      TYPE_INVALID, FLAGS_NONE,                                        0, nullptr}},
    {OPD(TYPE_GROUP_13, PF_66, 6), 1, X86InstInfo{"PSLLD", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_MOD_REG_ONLY | FLAGS_XMM_FLAGS,  1, nullptr}},
    {OPD(TYPE_GROUP_13, PF_66, 7), 1, X86InstInfo{"",      TYPE_INVALID, FLAGS_NONE,                                        0, nullptr}},

    {OPD(TYPE_GROUP_13, PF_F3, 0), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                             0, nullptr}},
    {OPD(TYPE_GROUP_13, PF_F3, 1), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                             0, nullptr}},
    {OPD(TYPE_GROUP_13, PF_F3, 2), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                             0, nullptr}},
    {OPD(TYPE_GROUP_13, PF_F3, 3), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                             0, nullptr}},
    {OPD(TYPE_GROUP_13, PF_F3, 4), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                             0, nullptr}},
    {OPD(TYPE_GROUP_13, PF_F3, 5), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                             0, nullptr}},
    {OPD(TYPE_GROUP_13, PF_F3, 6), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                             0, nullptr}},
    {OPD(TYPE_GROUP_13, PF_F3, 7), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                             0, nullptr}},

    {OPD(TYPE_GROUP_13, PF_F2, 0), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                             0, nullptr}},
    {OPD(TYPE_GROUP_13, PF_F2, 1), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                             0, nullptr}},
    {OPD(TYPE_GROUP_13, PF_F2, 2), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                             0, nullptr}},
    {OPD(TYPE_GROUP_13, PF_F2, 3), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                             0, nullptr}},
    {OPD(TYPE_GROUP_13, PF_F2, 4), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                             0, nullptr}},
    {OPD(TYPE_GROUP_13, PF_F2, 5), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                             0, nullptr}},
    {OPD(TYPE_GROUP_13, PF_F2, 6), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                             0, nullptr}},
    {OPD(TYPE_GROUP_13, PF_F2, 7), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                             0, nullptr}},

    // GROUP 14
    {OPD(TYPE_GROUP_14, PF_NONE, 0), 1, X86InstInfo{"",      TYPE_INVALID, FLAGS_NONE,                                      0, nullptr}},
    {OPD(TYPE_GROUP_14, PF_NONE, 1), 1, X86InstInfo{"",      TYPE_INVALID, FLAGS_NONE,                                      0, nullptr}},
    {OPD(TYPE_GROUP_14, PF_NONE, 2), 1, X86InstInfo{"PSRLQ", TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS, 1, nullptr}},
    {OPD(TYPE_GROUP_14, PF_NONE, 3), 1, X86InstInfo{"",      TYPE_INVALID, FLAGS_NONE,                                      0, nullptr}},
    {OPD(TYPE_GROUP_14, PF_NONE, 4), 1, X86InstInfo{"",      TYPE_INVALID, FLAGS_NONE,                                      0, nullptr}},
    {OPD(TYPE_GROUP_14, PF_NONE, 5), 1, X86InstInfo{"",      TYPE_INVALID, FLAGS_NONE,                                      0, nullptr}},
    {OPD(TYPE_GROUP_14, PF_NONE, 6), 1, X86InstInfo{"PSLLQ", TYPE_MMX, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS, 1, nullptr}},
    {OPD(TYPE_GROUP_14, PF_NONE, 7), 1, X86InstInfo{"",      TYPE_INVALID, FLAGS_NONE,                                      0, nullptr}},

    {OPD(TYPE_GROUP_14, PF_66, 0), 1, X86InstInfo{"",      TYPE_INVALID, FLAGS_NONE,                                        0, nullptr}},
    {OPD(TYPE_GROUP_14, PF_66, 1), 1, X86InstInfo{"",      TYPE_INVALID, FLAGS_NONE,                                        0, nullptr}},
    {OPD(TYPE_GROUP_14, PF_66, 2), 1, X86InstInfo{"PSRLQ", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_MOD_REG_ONLY | FLAGS_XMM_FLAGS,  1, nullptr}},
    {OPD(TYPE_GROUP_14, PF_66, 3), 1, X86InstInfo{"PSRLDQ",TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_MOD_REG_ONLY | FLAGS_XMM_FLAGS,  1, nullptr}},
    {OPD(TYPE_GROUP_14, PF_66, 4), 1, X86InstInfo{"",      TYPE_INVALID, FLAGS_NONE,                                        0, nullptr}},
    {OPD(TYPE_GROUP_14, PF_66, 5), 1, X86InstInfo{"",      TYPE_INVALID, FLAGS_NONE,                                        0, nullptr}},
    {OPD(TYPE_GROUP_14, PF_66, 6), 1, X86InstInfo{"PSLLQ", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_MOD_REG_ONLY | FLAGS_XMM_FLAGS,  1, nullptr}},
    {OPD(TYPE_GROUP_14, PF_66, 7), 1, X86InstInfo{"PSLLDQ",TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_MOD_REG_ONLY | FLAGS_XMM_FLAGS,  1, nullptr}},

    {OPD(TYPE_GROUP_14, PF_F3, 0), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                             0, nullptr}},
    {OPD(TYPE_GROUP_14, PF_F3, 1), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                             0, nullptr}},
    {OPD(TYPE_GROUP_14, PF_F3, 2), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                             0, nullptr}},
    {OPD(TYPE_GROUP_14, PF_F3, 3), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                             0, nullptr}},
    {OPD(TYPE_GROUP_14, PF_F3, 4), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                             0, nullptr}},
    {OPD(TYPE_GROUP_14, PF_F3, 5), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                             0, nullptr}},
    {OPD(TYPE_GROUP_14, PF_F3, 6), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                             0, nullptr}},
    {OPD(TYPE_GROUP_14, PF_F3, 7), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                             0, nullptr}},

    {OPD(TYPE_GROUP_14, PF_F2, 0), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                             0, nullptr}},
    {OPD(TYPE_GROUP_14, PF_F2, 1), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                             0, nullptr}},
    {OPD(TYPE_GROUP_14, PF_F2, 2), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                             0, nullptr}},
    {OPD(TYPE_GROUP_14, PF_F2, 3), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                             0, nullptr}},
    {OPD(TYPE_GROUP_14, PF_F2, 4), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                             0, nullptr}},
    {OPD(TYPE_GROUP_14, PF_F2, 5), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                             0, nullptr}},
    {OPD(TYPE_GROUP_14, PF_F2, 6), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                             0, nullptr}},
    {OPD(TYPE_GROUP_14, PF_F2, 7), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                             0, nullptr}},

    // GROUP 15
    {OPD(TYPE_GROUP_15, PF_NONE, 0), 1, X86InstInfo{"FXSAVE",          TYPE_MMX, FLAGS_NONE,       0, nullptr}}, // MMX/x87
    {OPD(TYPE_GROUP_15, PF_NONE, 1), 1, X86InstInfo{"FXRSTOR",         TYPE_MMX, FLAGS_NONE,       0, nullptr}}, // MMX/x87
    {OPD(TYPE_GROUP_15, PF_NONE, 2), 1, X86InstInfo{"LDMXCSR",         TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_MOD_MEM_ONLY, 0, nullptr}},
    {OPD(TYPE_GROUP_15, PF_NONE, 3), 1, X86InstInfo{"STMXCSR",         TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_MOD_MEM_ONLY, 0, nullptr}},
    {OPD(TYPE_GROUP_15, PF_NONE, 4), 1, X86InstInfo{"XSAVE",           TYPE_PRIV, FLAGS_NONE,      0, nullptr}},
    {OPD(TYPE_GROUP_15, PF_NONE, 5), 1, X86InstInfo{"LFENCE/XRSTOR",   TYPE_PRIV, FLAGS_NONE,      0, nullptr}},
    {OPD(TYPE_GROUP_15, PF_NONE, 6), 1, X86InstInfo{"MFENCE/XSAVEOPT", TYPE_PRIV, FLAGS_NONE,      0, nullptr}},
    {OPD(TYPE_GROUP_15, PF_NONE, 7), 1, X86InstInfo{"SFENCE/CLFLUSH",  TYPE_PRIV, FLAGS_NONE,      0, nullptr}},

    {OPD(TYPE_GROUP_15, PF_F3, 0), 1, X86InstInfo{"RDFSBASE", TYPE_PRIV, FLAGS_MODRM | FLAGS_SF_MOD_DST,          0, nullptr}},
    {OPD(TYPE_GROUP_15, PF_F3, 1), 1, X86InstInfo{"RDGSBASE", TYPE_PRIV, FLAGS_MODRM | FLAGS_SF_MOD_DST,          0, nullptr}},
    {OPD(TYPE_GROUP_15, PF_F3, 2), 1, X86InstInfo{"WRFSBASE", TYPE_PRIV, FLAGS_MODRM,          0, nullptr}},
    {OPD(TYPE_GROUP_15, PF_F3, 3), 1, X86InstInfo{"WRGSBASE", TYPE_PRIV, FLAGS_MODRM,          0, nullptr}},
    {OPD(TYPE_GROUP_15, PF_F3, 4), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                    0, nullptr}},
    {OPD(TYPE_GROUP_15, PF_F3, 5), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                    0, nullptr}},
    {OPD(TYPE_GROUP_15, PF_F3, 6), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                    0, nullptr}},
    {OPD(TYPE_GROUP_15, PF_F3, 7), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                    0, nullptr}},

    {OPD(TYPE_GROUP_15, PF_66, 0), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                    0, nullptr}},
    {OPD(TYPE_GROUP_15, PF_66, 1), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                    0, nullptr}},
    {OPD(TYPE_GROUP_15, PF_66, 2), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                    0, nullptr}},
    {OPD(TYPE_GROUP_15, PF_66, 3), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                    0, nullptr}},
    {OPD(TYPE_GROUP_15, PF_66, 4), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                    0, nullptr}},
    {OPD(TYPE_GROUP_15, PF_66, 5), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                    0, nullptr}},
    {OPD(TYPE_GROUP_15, PF_66, 6), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                    0, nullptr}},
    {OPD(TYPE_GROUP_15, PF_66, 7), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                    0, nullptr}},

    {OPD(TYPE_GROUP_15, PF_F2, 0), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                    0, nullptr}},
    {OPD(TYPE_GROUP_15, PF_F2, 1), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                    0, nullptr}},
    {OPD(TYPE_GROUP_15, PF_F2, 2), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                    0, nullptr}},
    {OPD(TYPE_GROUP_15, PF_F2, 3), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                    0, nullptr}},
    {OPD(TYPE_GROUP_15, PF_F2, 4), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                    0, nullptr}},
    {OPD(TYPE_GROUP_15, PF_F2, 5), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                    0, nullptr}},
    {OPD(TYPE_GROUP_15, PF_F2, 6), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                    0, nullptr}},
    {OPD(TYPE_GROUP_15, PF_F2, 7), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                    0, nullptr}},

    // GROUP 16
    // AMD documentation claims again that this entire group is n/a to prefix
    // Tooling once again fails to disassemble oens with the prefix. Disable until proven otherwise
    {OPD(TYPE_GROUP_16, PF_NONE, 0), 1, X86InstInfo{"PREFETCH NTA", TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY, 0, nullptr}},
    {OPD(TYPE_GROUP_16, PF_NONE, 1), 1, X86InstInfo{"PREFETCH T0",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY, 0, nullptr}},
    {OPD(TYPE_GROUP_16, PF_NONE, 2), 1, X86InstInfo{"PREFETCH T1",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY, 0, nullptr}},
    {OPD(TYPE_GROUP_16, PF_NONE, 3), 1, X86InstInfo{"PREFETCH T2",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY, 0, nullptr}},
    {OPD(TYPE_GROUP_16, PF_NONE, 4), 1, X86InstInfo{"NOP",          TYPE_INST, FLAGS_MODRM, 0, nullptr}},
    {OPD(TYPE_GROUP_16, PF_NONE, 5), 1, X86InstInfo{"NOP",          TYPE_INST, FLAGS_MODRM, 0, nullptr}},
    {OPD(TYPE_GROUP_16, PF_NONE, 6), 1, X86InstInfo{"NOP",          TYPE_INST, FLAGS_MODRM, 0, nullptr}},
    {OPD(TYPE_GROUP_16, PF_NONE, 7), 1, X86InstInfo{"NOP",          TYPE_INST, FLAGS_MODRM, 0, nullptr}},

    {OPD(TYPE_GROUP_16, PF_F3, 0), 1, X86InstInfo{"PREFETCH NTA", TYPE_INVALID, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_16, PF_F3, 1), 1, X86InstInfo{"PREFETCH T0",  TYPE_INVALID, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_16, PF_F3, 2), 1, X86InstInfo{"PREFETCH T1",  TYPE_INVALID, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_16, PF_F3, 3), 1, X86InstInfo{"PREFETCH T2",  TYPE_INVALID, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_16, PF_F3, 4), 1, X86InstInfo{"NOP",          TYPE_INVALID, FLAGS_MODRM,   0, nullptr}},
    {OPD(TYPE_GROUP_16, PF_F3, 5), 1, X86InstInfo{"NOP",          TYPE_INVALID, FLAGS_MODRM,   0, nullptr}},
    {OPD(TYPE_GROUP_16, PF_F3, 6), 1, X86InstInfo{"NOP",          TYPE_INVALID, FLAGS_MODRM,   0, nullptr}},
    {OPD(TYPE_GROUP_16, PF_F3, 7), 1, X86InstInfo{"NOP",          TYPE_INVALID, FLAGS_MODRM,   0, nullptr}},

    {OPD(TYPE_GROUP_16, PF_66, 0), 1, X86InstInfo{"PREFETCH NTA", TYPE_INVALID, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_16, PF_66, 1), 1, X86InstInfo{"PREFETCH T0",  TYPE_INVALID, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_16, PF_66, 2), 1, X86InstInfo{"PREFETCH T1",  TYPE_INVALID, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_16, PF_66, 3), 1, X86InstInfo{"PREFETCH T2",  TYPE_INVALID, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_16, PF_66, 4), 1, X86InstInfo{"NOP",          TYPE_INVALID, FLAGS_MODRM,   0, nullptr}},
    {OPD(TYPE_GROUP_16, PF_66, 5), 1, X86InstInfo{"NOP",          TYPE_INVALID, FLAGS_MODRM,   0, nullptr}},
    {OPD(TYPE_GROUP_16, PF_66, 6), 1, X86InstInfo{"NOP",          TYPE_INVALID, FLAGS_MODRM,   0, nullptr}},
    {OPD(TYPE_GROUP_16, PF_66, 7), 1, X86InstInfo{"NOP",          TYPE_INVALID, FLAGS_MODRM,   0, nullptr}},

    {OPD(TYPE_GROUP_16, PF_F2, 0), 1, X86InstInfo{"PREFETCH NTA", TYPE_INVALID, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_16, PF_F2, 1), 1, X86InstInfo{"PREFETCH T0",  TYPE_INVALID, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_16, PF_F2, 2), 1, X86InstInfo{"PREFETCH T1",  TYPE_INVALID, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_16, PF_F2, 3), 1, X86InstInfo{"PREFETCH T2",  TYPE_INVALID, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_16, PF_F2, 4), 1, X86InstInfo{"NOP",          TYPE_INVALID, FLAGS_MODRM,   0, nullptr}},
    {OPD(TYPE_GROUP_16, PF_F2, 5), 1, X86InstInfo{"NOP",          TYPE_INVALID, FLAGS_MODRM,   0, nullptr}},
    {OPD(TYPE_GROUP_16, PF_F2, 6), 1, X86InstInfo{"NOP",          TYPE_INVALID, FLAGS_MODRM,   0, nullptr}},
    {OPD(TYPE_GROUP_16, PF_F2, 7), 1, X86InstInfo{"NOP",          TYPE_INVALID, FLAGS_MODRM,   0, nullptr}},

    // GROUP 17
    {OPD(TYPE_GROUP_17, PF_NONE, 0), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                          0, nullptr}},
    {OPD(TYPE_GROUP_17, PF_NONE, 1), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                          0, nullptr}},
    {OPD(TYPE_GROUP_17, PF_NONE, 2), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                          0, nullptr}},
    {OPD(TYPE_GROUP_17, PF_NONE, 3), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                          0, nullptr}},
    {OPD(TYPE_GROUP_17, PF_NONE, 4), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                          0, nullptr}},
    {OPD(TYPE_GROUP_17, PF_NONE, 5), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                          0, nullptr}},
    {OPD(TYPE_GROUP_17, PF_NONE, 6), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                          0, nullptr}},
    {OPD(TYPE_GROUP_17, PF_NONE, 7), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                          0, nullptr}},

    {OPD(TYPE_GROUP_17, PF_F3, 0), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                            0, nullptr}},
    {OPD(TYPE_GROUP_17, PF_F3, 1), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                            0, nullptr}},
    {OPD(TYPE_GROUP_17, PF_F3, 2), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                            0, nullptr}},
    {OPD(TYPE_GROUP_17, PF_F3, 3), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                            0, nullptr}},
    {OPD(TYPE_GROUP_17, PF_F3, 4), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                            0, nullptr}},
    {OPD(TYPE_GROUP_17, PF_F3, 5), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                            0, nullptr}},
    {OPD(TYPE_GROUP_17, PF_F3, 6), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                            0, nullptr}},
    {OPD(TYPE_GROUP_17, PF_F3, 7), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                            0, nullptr}},

    {OPD(TYPE_GROUP_17, PF_66, 0), 1, X86InstInfo{"EXTRQ", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_MOD_REG_ONLY | FLAGS_XMM_FLAGS, 2, nullptr}},
    {OPD(TYPE_GROUP_17, PF_66, 1), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                            0, nullptr}},
    {OPD(TYPE_GROUP_17, PF_66, 2), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                            0, nullptr}},
    {OPD(TYPE_GROUP_17, PF_66, 3), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                            0, nullptr}},
    {OPD(TYPE_GROUP_17, PF_66, 4), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                            0, nullptr}},
    {OPD(TYPE_GROUP_17, PF_66, 5), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                            0, nullptr}},
    {OPD(TYPE_GROUP_17, PF_66, 6), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                            0, nullptr}},
    {OPD(TYPE_GROUP_17, PF_66, 7), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                            0, nullptr}},

    {OPD(TYPE_GROUP_17, PF_F2, 0), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                            0, nullptr}},
    {OPD(TYPE_GROUP_17, PF_F2, 1), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                            0, nullptr}},
    {OPD(TYPE_GROUP_17, PF_F2, 2), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                            0, nullptr}},
    {OPD(TYPE_GROUP_17, PF_F2, 3), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                            0, nullptr}},
    {OPD(TYPE_GROUP_17, PF_F2, 4), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                            0, nullptr}},
    {OPD(TYPE_GROUP_17, PF_F2, 5), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                            0, nullptr}},
    {OPD(TYPE_GROUP_17, PF_F2, 6), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                            0, nullptr}},
    {OPD(TYPE_GROUP_17, PF_F2, 7), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                                            0, nullptr}},

    // GROUP P
    // AMD documentation claims n/a for all instructions in Group P
    // It also claims that instructions /2, /4, /5, /6, /7 all alias to /0
    // It claims that /3 is still Prefetch Mod
    // Tooling fails to decode past the /2 encoding but runs fine in hardware
    // Hardware also runs all the prefixes correctly
    {OPD(TYPE_GROUP_P, PF_NONE, 0), 1, X86InstInfo{"PREFETCH Ex",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY, 0, nullptr}},
    {OPD(TYPE_GROUP_P, PF_NONE, 1), 1, X86InstInfo{"PREFETCH Mod", TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY, 0, nullptr}},
    {OPD(TYPE_GROUP_P, PF_NONE, 2), 1, X86InstInfo{"PREFETCH Res", TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY, 0, nullptr}},
    {OPD(TYPE_GROUP_P, PF_NONE, 3), 1, X86InstInfo{"PREFETCH Mod", TYPE_INVALID, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY, 0, nullptr}},
    {OPD(TYPE_GROUP_P, PF_NONE, 4), 1, X86InstInfo{"PREFETCH Res", TYPE_INVALID, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY, 0, nullptr}},
    {OPD(TYPE_GROUP_P, PF_NONE, 5), 1, X86InstInfo{"PREFETCH Res", TYPE_INVALID, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY, 0, nullptr}},
    {OPD(TYPE_GROUP_P, PF_NONE, 6), 1, X86InstInfo{"PREFETCH Res", TYPE_INVALID, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY, 0, nullptr}},
    {OPD(TYPE_GROUP_P, PF_NONE, 7), 1, X86InstInfo{"PREFETCH Res", TYPE_INVALID, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY, 0, nullptr}},

    {OPD(TYPE_GROUP_P, PF_F3, 0), 1, X86InstInfo{"PREFETCH Ex",  TYPE_INVALID, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_P, PF_F3, 1), 1, X86InstInfo{"PREFETCH Mod", TYPE_INVALID, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_P, PF_F3, 2), 1, X86InstInfo{"PREFETCH Res", TYPE_INVALID, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_P, PF_F3, 3), 1, X86InstInfo{"PREFETCH Mod", TYPE_INVALID, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_P, PF_F3, 4), 1, X86InstInfo{"PREFETCH Res", TYPE_INVALID, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_P, PF_F3, 5), 1, X86InstInfo{"PREFETCH Res", TYPE_INVALID, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_P, PF_F3, 6), 1, X86InstInfo{"PREFETCH Res", TYPE_INVALID, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_P, PF_F3, 7), 1, X86InstInfo{"PREFETCH Res", TYPE_INVALID, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},

    {OPD(TYPE_GROUP_P, PF_66, 0), 1, X86InstInfo{"PREFETCH Ex",  TYPE_INVALID, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_P, PF_66, 1), 1, X86InstInfo{"PREFETCH Mod", TYPE_INVALID, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_P, PF_66, 2), 1, X86InstInfo{"PREFETCH Res", TYPE_INVALID, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_P, PF_66, 3), 1, X86InstInfo{"PREFETCH Mod", TYPE_INVALID, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_P, PF_66, 4), 1, X86InstInfo{"PREFETCH Res", TYPE_INVALID, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_P, PF_66, 5), 1, X86InstInfo{"PREFETCH Res", TYPE_INVALID, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_P, PF_66, 6), 1, X86InstInfo{"PREFETCH Res", TYPE_INVALID, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_P, PF_66, 7), 1, X86InstInfo{"PREFETCH Res", TYPE_INVALID, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},

    {OPD(TYPE_GROUP_P, PF_F2, 0), 1, X86InstInfo{"PREFETCH Ex",  TYPE_INVALID, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_P, PF_F2, 1), 1, X86InstInfo{"PREFETCH Mod", TYPE_INVALID, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_P, PF_F2, 2), 1, X86InstInfo{"PREFETCH Res", TYPE_INVALID, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_P, PF_F2, 3), 1, X86InstInfo{"PREFETCH Mod", TYPE_INVALID, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_P, PF_F2, 4), 1, X86InstInfo{"PREFETCH Res", TYPE_INVALID, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_P, PF_F2, 5), 1, X86InstInfo{"PREFETCH Res", TYPE_INVALID, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_P, PF_F2, 6), 1, X86InstInfo{"PREFETCH Res", TYPE_INVALID, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_P, PF_F2, 7), 1, X86InstInfo{"PREFETCH Res", TYPE_INVALID, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
  };
#undef OPD

  const std::vector<std::tuple<uint8_t, uint8_t, X86InstInfo>> SecondaryModRMExtensionOpTable = {
    // REG /1
    {((0 << 3) | 0), 1, X86InstInfo{"MONITOR",  TYPE_PRIV,    FLAGS_NONE, 0, nullptr}},
    {((0 << 3) | 1), 1, X86InstInfo{"MWAIT",    TYPE_PRIV,    FLAGS_NONE, 0, nullptr}},
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
    {((1 << 3) | 5), 1, X86InstInfo{"",         TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
    {((1 << 3) | 6), 1, X86InstInfo{"",         TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
    {((1 << 3) | 7), 1, X86InstInfo{"",         TYPE_INVALID, FLAGS_NONE, 0, nullptr}},

    // REG /3
    {((2 << 3) | 0), 1, X86InstInfo{"VMRUN",    TYPE_PRIV,    FLAGS_NONE, 0, nullptr}},
    {((2 << 3) | 1), 1, X86InstInfo{"VMMCALL",  TYPE_PRIV,    FLAGS_NONE, 0, nullptr}},
    {((2 << 3) | 2), 1, X86InstInfo{"VMLOAD",   TYPE_PRIV,    FLAGS_NONE, 0, nullptr}},
    {((2 << 3) | 3), 1, X86InstInfo{"VMSAVE",   TYPE_PRIV,    FLAGS_NONE, 0, nullptr}},
    {((2 << 3) | 4), 1, X86InstInfo{"STGI",     TYPE_PRIV,    FLAGS_NONE, 0, nullptr}},
    {((2 << 3) | 5), 1, X86InstInfo{"CLGI",     TYPE_PRIV,    FLAGS_NONE, 0, nullptr}},
    {((2 << 3) | 6), 1, X86InstInfo{"SKINIT",   TYPE_PRIV,    FLAGS_NONE, 0, nullptr}},
    {((2 << 3) | 7), 1, X86InstInfo{"INVLPGA",  TYPE_PRIV,    FLAGS_NONE, 0, nullptr}},

    // REG /7
    {((3 << 3) | 0), 1, X86InstInfo{"SWAPGS",   TYPE_PRIV,    FLAGS_NONE, 0, nullptr}},
    {((3 << 3) | 1), 1, X86InstInfo{"RDTSCP",   TYPE_PRIV,    FLAGS_NONE, 0, nullptr}},
    {((3 << 3) | 2), 1, X86InstInfo{"MONITORX", TYPE_PRIV,    FLAGS_NONE, 0, nullptr}},
    {((3 << 3) | 3), 1, X86InstInfo{"MWAITX",   TYPE_PRIV,    FLAGS_NONE, 0, nullptr}},
    {((3 << 3) | 4), 1, X86InstInfo{"",         TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
    {((3 << 3) | 5), 1, X86InstInfo{"",         TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
    {((3 << 3) | 6), 1, X86InstInfo{"",         TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
    {((3 << 3) | 7), 1, X86InstInfo{"",         TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
  };

#define OPD(op, modrmop) (((op - 0xD8) << 8) | modrmop)
#define OPDReg(op, reg) (((op - 0xD8) << 8) | (reg << 3))
  const std::vector<std::tuple<uint16_t, uint8_t, X86InstInfo>> X87OpTable = {
    // 0xD8
    {OPDReg(0xD8, 0), 1, X86InstInfo{"FADD",  TYPE_X87, FLAGS_NONE, 0, nullptr}},
    {OPDReg(0xD8, 1), 1, X86InstInfo{"FMUL",  TYPE_X87, FLAGS_NONE, 0, nullptr}},
    {OPDReg(0xD8, 2), 1, X86InstInfo{"FCOM",  TYPE_X87, FLAGS_NONE, 0, nullptr}},
    {OPDReg(0xD8, 3), 1, X86InstInfo{"FCOMP", TYPE_X87, FLAGS_NONE, 0, nullptr}},
    {OPDReg(0xD8, 4), 1, X86InstInfo{"FSUB",  TYPE_X87, FLAGS_NONE, 0, nullptr}},
    {OPDReg(0xD8, 5), 1, X86InstInfo{"FSUBR", TYPE_X87, FLAGS_NONE, 0, nullptr}},
    {OPDReg(0xD8, 6), 1, X86InstInfo{"FDIV",  TYPE_X87, FLAGS_NONE, 0, nullptr}},
    {OPDReg(0xD8, 7), 1, X86InstInfo{"FDIVR", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      //  / 0
      {OPD(0xD8, 0xC0), 8, X86InstInfo{"FADD", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      //  / 1
      {OPD(0xD8, 0xC8), 8, X86InstInfo{"FMUL", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      //  / 2
      {OPD(0xD8, 0xD0), 8, X86InstInfo{"FCOM", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      //  / 3
      {OPD(0xD8, 0xD8), 8, X86InstInfo{"FCOMP", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      //  / 4
      {OPD(0xD8, 0xE0), 8, X86InstInfo{"FSUB", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      //  / 5
      {OPD(0xD8, 0xE8), 8, X86InstInfo{"FSUBR", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      //  / 6
      {OPD(0xD8, 0xF0), 8, X86InstInfo{"FDIV", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      //  / 7
      {OPD(0xD8, 0xF8), 8, X86InstInfo{"FDIVR", TYPE_X87, FLAGS_NONE, 0, nullptr}},
    // 0xD9
    {OPDReg(0xD9, 0), 1, X86InstInfo{"FLD",     TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST, 0, nullptr}},
    {OPDReg(0xD9, 1), 1, X86InstInfo{"",        TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
    {OPDReg(0xD9, 2), 1, X86InstInfo{"FST",     TYPE_X87, FLAGS_NONE, 0, nullptr}},
    {OPDReg(0xD9, 3), 1, X86InstInfo{"FSTP",    TYPE_X87, FLAGS_NONE, 0, nullptr}},
    {OPDReg(0xD9, 4), 1, X86InstInfo{"FLDENV",  TYPE_X87, FLAGS_NONE, 0, nullptr}},
    {OPDReg(0xD9, 5), 1, X86InstInfo{"FLDCW",   TYPE_X87, FLAGS_NONE, 0, nullptr}},
    {OPDReg(0xD9, 6), 1, X86InstInfo{"FNSTENV", TYPE_X87, FLAGS_NONE, 0, nullptr}},
    {OPDReg(0xD9, 7), 1, X86InstInfo{"FNSTCW",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST, 0, nullptr}},
      //  / 0
      {OPD(0xD9, 0xC0), 8, X86InstInfo{"FLD",   TYPE_INST, FLAGS_NONE, 0, nullptr}},
      //  / 1
      {OPD(0xD9, 0xC8), 8, X86InstInfo{"FXCH",  TYPE_X87, FLAGS_NONE, 0, nullptr}},
      //  / 2
      {OPD(0xD9, 0xD0), 1, X86InstInfo{"FNOP",  TYPE_X87, FLAGS_NONE, 0, nullptr}},
      {OPD(0xD9, 0xD1), 7, X86InstInfo{"",      TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
      //  / 3
      {OPD(0xD9, 0xD8), 8, X86InstInfo{"",      TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
      //  / 4
      {OPD(0xD9, 0xE0), 1, X86InstInfo{"FCHS", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      {OPD(0xD9, 0xE1), 1, X86InstInfo{"FABS", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      {OPD(0xD9, 0xE2), 2, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
      {OPD(0xD9, 0xE4), 1, X86InstInfo{"FTST", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      {OPD(0xD9, 0xE5), 1, X86InstInfo{"FXAM", TYPE_INST,  FLAGS_NONE, 0, nullptr}},
      {OPD(0xD9, 0xE6), 2, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
      //  / 5
      {OPD(0xD9, 0xE8), 1, X86InstInfo{"FLD1", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      {OPD(0xD9, 0xE9), 1, X86InstInfo{"FLDL2T", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      {OPD(0xD9, 0xEA), 1, X86InstInfo{"FLDL2E", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      {OPD(0xD9, 0xEB), 1, X86InstInfo{"FLDPI", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      {OPD(0xD9, 0xEC), 1, X86InstInfo{"FLDLG2", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      {OPD(0xD9, 0xED), 1, X86InstInfo{"FLDLN2", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      {OPD(0xD9, 0xEE), 1, X86InstInfo{"FLDZ", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      {OPD(0xD9, 0xEF), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
      //  / 6
      {OPD(0xD9, 0xF0), 1, X86InstInfo{"F2XM1", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      {OPD(0xD9, 0xF1), 1, X86InstInfo{"FYL2X", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      {OPD(0xD9, 0xF2), 1, X86InstInfo{"FPTAN", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      {OPD(0xD9, 0xF3), 1, X86InstInfo{"FPATAN", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      {OPD(0xD9, 0xF4), 1, X86InstInfo{"FXTRACT", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      {OPD(0xD9, 0xF5), 1, X86InstInfo{"FPREM1", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      {OPD(0xD9, 0xF6), 1, X86InstInfo{"FDECSTP", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      {OPD(0xD9, 0xF7), 1, X86InstInfo{"FINCSTP", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      //  / 7
      {OPD(0xD9, 0xF8), 1, X86InstInfo{"FPREM", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      {OPD(0xD9, 0xF9), 1, X86InstInfo{"FYL2XP1", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      {OPD(0xD9, 0xFA), 1, X86InstInfo{"FSQRT", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      {OPD(0xD9, 0xFB), 1, X86InstInfo{"FSINCOS", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      {OPD(0xD9, 0xFC), 1, X86InstInfo{"FRNDINT", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      {OPD(0xD9, 0xFD), 1, X86InstInfo{"FSCALE", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      {OPD(0xD9, 0xFE), 1, X86InstInfo{"FSIN", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      {OPD(0xD9, 0xFF), 1, X86InstInfo{"FCOS", TYPE_X87, FLAGS_NONE, 0, nullptr}},
    // 0xDA
    {OPDReg(0xDA, 0), 1, X86InstInfo{"FIADD", TYPE_X87, FLAGS_NONE, 0, nullptr}},
    {OPDReg(0xDA, 1), 1, X86InstInfo{"FIMUL", TYPE_X87, FLAGS_NONE, 0, nullptr}},
    {OPDReg(0xDA, 2), 1, X86InstInfo{"FICOM", TYPE_X87, FLAGS_NONE, 0, nullptr}},
    {OPDReg(0xDA, 3), 1, X86InstInfo{"FICOMP", TYPE_X87, FLAGS_NONE, 0, nullptr}},
    {OPDReg(0xDA, 4), 1, X86InstInfo{"FISUB", TYPE_X87, FLAGS_NONE, 0, nullptr}},
    {OPDReg(0xDA, 5), 1, X86InstInfo{"FISUBR", TYPE_X87, FLAGS_NONE, 0, nullptr}},
    {OPDReg(0xDA, 6), 1, X86InstInfo{"FIDIV", TYPE_X87, FLAGS_NONE, 0, nullptr}},
    {OPDReg(0xDA, 7), 1, X86InstInfo{"FIDIVR", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      //  / 0
      {OPD(0xDA, 0xC0), 8, X86InstInfo{"FCMOVB", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      //  / 1
      {OPD(0xDA, 0xC8), 8, X86InstInfo{"FCMOVE", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      //  / 2
      {OPD(0xDA, 0xD0), 8, X86InstInfo{"FCMOVBE", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      //  / 3
      {OPD(0xDA, 0xD8), 8, X86InstInfo{"FCMOVU", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      //  / 4
      {OPD(0xDA, 0xE0), 8, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
      //  / 5
      {OPD(0xDA, 0xE8), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
      {OPD(0xDA, 0xE9), 1, X86InstInfo{"FUCOMPP", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      {OPD(0xDA, 0xEA), 6, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
      //  / 6
      {OPD(0xDA, 0xF0), 8, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
      //  / 7
      {OPD(0xDA, 0xF8), 8, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
    // 0xDB
    {OPDReg(0xDB, 0), 1, X86InstInfo{"FILD",   TYPE_X87,   FLAGS_NONE, 0, nullptr}},
    {OPDReg(0xDB, 1), 1, X86InstInfo{"FISTTP", TYPE_X87,   FLAGS_NONE, 0, nullptr}},
    {OPDReg(0xDB, 2), 1, X86InstInfo{"FIST",   TYPE_X87,   FLAGS_NONE, 0, nullptr}},
    {OPDReg(0xDB, 3), 1, X86InstInfo{"FISTP",  TYPE_X87,   FLAGS_NONE, 0, nullptr}},
    {OPDReg(0xDB, 4), 1, X86InstInfo{"",       TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
    {OPDReg(0xDB, 5), 1, X86InstInfo{"FLD",    TYPE_INST,    FLAGS_MODRM | FLAGS_SF_MOD_DST, 0, nullptr}},
    {OPDReg(0xDB, 6), 1, X86InstInfo{"",       TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
    {OPDReg(0xDB, 7), 1, X86InstInfo{"FSTP",   TYPE_X87,   FLAGS_NONE, 0, nullptr}},
      //  / 0
      {OPD(0xDB, 0xC0), 8, X86InstInfo{"FCMOVNB", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      //  / 1
      {OPD(0xDB, 0xC8), 8, X86InstInfo{"FCMOVNE", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      //  / 2
      {OPD(0xDB, 0xD0), 8, X86InstInfo{"FCMOVNBE", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      //  / 3
      {OPD(0xDB, 0xD8), 8, X86InstInfo{"FCMOVNU", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      //  / 4
      {OPD(0xDB, 0xE0), 2, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
      {OPD(0xDB, 0xE2), 1, X86InstInfo{"FNCLEX", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      {OPD(0xDB, 0xE3), 1, X86InstInfo{"FNINIT", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      {OPD(0xDB, 0xE4), 4, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
      //  / 5
      {OPD(0xDB, 0xE8), 8, X86InstInfo{"FUCOMI", TYPE_INST, FLAGS_NONE, 0, nullptr}},
      //  / 6
      {OPD(0xDB, 0xF0), 8, X86InstInfo{"FCOMI", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      //  / 7
      {OPD(0xDB, 0xF8), 8, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
    // 0xDC
    {OPDReg(0xDC, 0), 1, X86InstInfo{"FADD", TYPE_X87, FLAGS_NONE, 0, nullptr}},
    {OPDReg(0xDC, 1), 1, X86InstInfo{"FMUL", TYPE_X87, FLAGS_NONE, 0, nullptr}},
    {OPDReg(0xDC, 2), 1, X86InstInfo{"FCOM", TYPE_X87, FLAGS_NONE, 0, nullptr}},
    {OPDReg(0xDC, 3), 1, X86InstInfo{"FCOMP", TYPE_X87, FLAGS_NONE, 0, nullptr}},
    {OPDReg(0xDC, 4), 1, X86InstInfo{"FSUB", TYPE_X87, FLAGS_NONE, 0, nullptr}},
    {OPDReg(0xDC, 5), 1, X86InstInfo{"FSUBR", TYPE_X87, FLAGS_NONE, 0, nullptr}},
    {OPDReg(0xDC, 6), 1, X86InstInfo{"FDIV", TYPE_X87, FLAGS_NONE, 0, nullptr}},
    {OPDReg(0xDC, 7), 1, X86InstInfo{"FDIVR", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      //  / 0
      {OPD(0xDC, 0xC0), 8, X86InstInfo{"FADD", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      //  / 1
      {OPD(0xDC, 0xC8), 8, X86InstInfo{"FMUL", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      //  / 2
      {OPD(0xDC, 0xD0), 8, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
      //  / 3
      {OPD(0xDC, 0xD8), 8, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
      //  / 4
      {OPD(0xDC, 0xE0), 8, X86InstInfo{"FSUBR", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      //  / 5
      {OPD(0xDC, 0xE8), 8, X86InstInfo{"FSUB", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      //  / 6
      {OPD(0xDC, 0xF0), 8, X86InstInfo{"FDIVR", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      //  / 7
      {OPD(0xDC, 0xF8), 8, X86InstInfo{"FDIV", TYPE_X87, FLAGS_NONE, 0, nullptr}},
    // 0xDD
    {OPDReg(0xDD, 0), 1, X86InstInfo{"FLD", TYPE_X87, FLAGS_NONE, 0, nullptr}},
    {OPDReg(0xDD, 1), 1, X86InstInfo{"FISTTP", TYPE_X87, FLAGS_NONE, 0, nullptr}},
    {OPDReg(0xDD, 2), 1, X86InstInfo{"FST", TYPE_X87, FLAGS_NONE, 0, nullptr}},
    {OPDReg(0xDD, 3), 1, X86InstInfo{"FSTP", TYPE_X87, FLAGS_NONE, 0, nullptr}},
    {OPDReg(0xDD, 4), 1, X86InstInfo{"FRSTOR", TYPE_X87, FLAGS_NONE, 0, nullptr}},
    {OPDReg(0xDD, 5), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
    {OPDReg(0xDD, 6), 1, X86InstInfo{"FNSAVE", TYPE_X87, FLAGS_NONE, 0, nullptr}},
    {OPDReg(0xDD, 7), 1, X86InstInfo{"FNSTSW", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      //  / 0
      {OPD(0xDD, 0xC0), 8, X86InstInfo{"FFREE", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      //  / 1
      {OPD(0xDD, 0xC8), 8, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
      //  / 2
      {OPD(0xDD, 0xD0), 8, X86InstInfo{"FST", TYPE_INST, FLAGS_NONE, 0, nullptr}},
      //  / 3
      {OPD(0xDD, 0xD8), 8, X86InstInfo{"FSTP", TYPE_INST, FLAGS_NONE, 0, nullptr}},
      //  / 4
      {OPD(0xDD, 0xE0), 8, X86InstInfo{"FUCOM", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      //  / 5
      {OPD(0xDD, 0xE8), 8, X86InstInfo{"FUCOMP", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      //  / 6
      {OPD(0xDD, 0xF0), 8, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
      //  / 7
      {OPD(0xDD, 0xF8), 8, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
    // 0xDE
    {OPDReg(0xDE, 0), 1, X86InstInfo{"FIADD", TYPE_X87, FLAGS_NONE, 0, nullptr}},
    {OPDReg(0xDE, 1), 1, X86InstInfo{"FIMUL", TYPE_X87, FLAGS_NONE, 0, nullptr}},
    {OPDReg(0xDE, 2), 1, X86InstInfo{"FICOM", TYPE_X87, FLAGS_NONE, 0, nullptr}},
    {OPDReg(0xDE, 3), 1, X86InstInfo{"FICOMP", TYPE_X87, FLAGS_NONE, 0, nullptr}},
    {OPDReg(0xDE, 4), 1, X86InstInfo{"FISUB", TYPE_X87, FLAGS_NONE, 0, nullptr}},
    {OPDReg(0xDE, 5), 1, X86InstInfo{"FISUBR", TYPE_X87, FLAGS_NONE, 0, nullptr}},
    {OPDReg(0xDE, 6), 1, X86InstInfo{"FIDIV", TYPE_X87, FLAGS_NONE, 0, nullptr}},
    {OPDReg(0xDE, 7), 1, X86InstInfo{"FIDIVR", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      //  / 0
      {OPD(0xDE, 0xC0), 8, X86InstInfo{"FADDP", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      //  / 1
      {OPD(0xDE, 0xC8), 8, X86InstInfo{"FMULP", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      //  / 2
      {OPD(0xDE, 0xD0), 8, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
      //  / 3
      {OPD(0xDE, 0xD8), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
      {OPD(0xDE, 0xD9), 1, X86InstInfo{"FCOMPP", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      {OPD(0xDE, 0xDA), 6, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
      //  / 4
      {OPD(0xDE, 0xE0), 8, X86InstInfo{"FSUBRP", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      //  / 5
      {OPD(0xDE, 0xE8), 8, X86InstInfo{"FSUBP", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      //  / 6
      {OPD(0xDE, 0xF0), 8, X86InstInfo{"FDIVRP", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      //  / 7
      {OPD(0xDE, 0xF8), 8, X86InstInfo{"FDIVP", TYPE_X87, FLAGS_NONE, 0, nullptr}},
    // 0xDF
    {OPDReg(0xDF, 0), 1, X86InstInfo{"FILD", TYPE_X87, FLAGS_NONE, 0, nullptr}},
    {OPDReg(0xDF, 1), 1, X86InstInfo{"FISTTP", TYPE_X87, FLAGS_NONE, 0, nullptr}},
    {OPDReg(0xDF, 2), 1, X86InstInfo{"FIST", TYPE_X87, FLAGS_NONE, 0, nullptr}},
    {OPDReg(0xDF, 3), 1, X86InstInfo{"FISTP", TYPE_X87, FLAGS_NONE, 0, nullptr}},
    {OPDReg(0xDF, 4), 1, X86InstInfo{"FBLD", TYPE_X87, FLAGS_NONE, 0, nullptr}},
    {OPDReg(0xDF, 5), 1, X86InstInfo{"FILD", TYPE_X87, FLAGS_NONE, 0, nullptr}},
    {OPDReg(0xDF, 6), 1, X86InstInfo{"FBSTP", TYPE_X87, FLAGS_NONE, 0, nullptr}},
    {OPDReg(0xDF, 7), 1, X86InstInfo{"FISTP", TYPE_X87, FLAGS_NONE, 0, nullptr}},
      //  / 0
      {OPD(0xDF, 0xC0), 8, X86InstInfo{"",        TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
      //  / 1
      {OPD(0xDF, 0xC8), 8, X86InstInfo{"",        TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
      //  / 2
      {OPD(0xDF, 0xD0), 8, X86InstInfo{"",        TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
      //  / 3
      {OPD(0xDF, 0xD8), 8, X86InstInfo{"",        TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
      //  / 4
      {OPD(0xDF, 0xE0), 1, X86InstInfo{"FNSTSW",  TYPE_INST,    FLAGS_NONE, 0, nullptr}},
      {OPD(0xDF, 0xE1), 7, X86InstInfo{"",        TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
      //  / 5
      {OPD(0xDF, 0xE8), 8, X86InstInfo{"FUCOMIP", TYPE_INST,    FLAGS_NONE, 0, nullptr}},
      //  / 6
      {OPD(0xDF, 0xF0), 8, X86InstInfo{"FCOMIP",  TYPE_X87,   FLAGS_NONE, 0, nullptr}},
      //  / 7
      {OPD(0xDF, 0xF8), 8, X86InstInfo{"",        TYPE_INVALID, FLAGS_NONE, 0, nullptr}},
  };
#undef OPD
#undef OPDReg

  const std::vector<std::tuple<uint8_t, uint8_t, X86InstInfo>> DDDNowOpTable = {
    {0x0C, 1, X86InstInfo{"PI2FW",    TYPE_3DNOW_INST, FLAGS_NONE, 0, nullptr}},
    {0x0D, 1, X86InstInfo{"PI2FD",    TYPE_3DNOW_INST, FLAGS_NONE, 0, nullptr}},
    {0x1C, 1, X86InstInfo{"PF2IW",    TYPE_3DNOW_INST, FLAGS_NONE, 0, nullptr}},
    {0x1D, 1, X86InstInfo{"PF2ID",    TYPE_3DNOW_INST, FLAGS_NONE, 0, nullptr}},

    {0x8A, 1, X86InstInfo{"PFNACC",   TYPE_3DNOW_INST, FLAGS_NONE, 0, nullptr}},
    {0x8E, 1, X86InstInfo{"PFPNACC",  TYPE_3DNOW_INST, FLAGS_NONE, 0, nullptr}},

    {0x9A, 1, X86InstInfo{"PFSUB",    TYPE_3DNOW_INST, FLAGS_NONE, 0, nullptr}},
    {0x9E, 1, X86InstInfo{"PFADD",    TYPE_3DNOW_INST, FLAGS_NONE, 0, nullptr}},

    {0xAA, 1, X86InstInfo{"PFSUBR",   TYPE_3DNOW_INST, FLAGS_NONE, 0, nullptr}},
    {0xAE, 1, X86InstInfo{"PFACC",    TYPE_3DNOW_INST, FLAGS_NONE, 0, nullptr}},

    {0xBB, 1, X86InstInfo{"PSWAPD",   TYPE_3DNOW_INST, FLAGS_NONE, 0, nullptr}},
    {0xBF, 1, X86InstInfo{"PAVGUSB",  TYPE_3DNOW_INST, FLAGS_NONE, 0, nullptr}},

    {0x90, 1, X86InstInfo{"PFCMPGE",  TYPE_3DNOW_INST, FLAGS_NONE, 0, nullptr}},
    {0x94, 1, X86InstInfo{"PFMIN",    TYPE_3DNOW_INST, FLAGS_NONE, 0, nullptr}},
    {0x96, 1, X86InstInfo{"PFRCP",    TYPE_3DNOW_INST, FLAGS_NONE, 0, nullptr}},
    {0x97, 1, X86InstInfo{"PFRSQRT",  TYPE_3DNOW_INST, FLAGS_NONE, 0, nullptr}},

    {0xA0, 1, X86InstInfo{"PFCMPGT",  TYPE_3DNOW_INST, FLAGS_NONE, 0, nullptr}},
    {0xA4, 1, X86InstInfo{"PFMAX",    TYPE_3DNOW_INST, FLAGS_NONE, 0, nullptr}},
    {0xA6, 1, X86InstInfo{"PFRCPIT1", TYPE_3DNOW_INST, FLAGS_NONE, 0, nullptr}},
    {0xA7, 1, X86InstInfo{"PFRSQIT1", TYPE_3DNOW_INST, FLAGS_NONE, 0, nullptr}},

    {0xB0, 1, X86InstInfo{"PFCMPEQ",  TYPE_3DNOW_INST, FLAGS_NONE, 0, nullptr}},
    {0xB4, 1, X86InstInfo{"PFMUL",    TYPE_3DNOW_INST, FLAGS_NONE, 0, nullptr}},
    {0xB6, 1, X86InstInfo{"PFRCPIT2", TYPE_3DNOW_INST, FLAGS_NONE, 0, nullptr}},
    {0xB7, 1, X86InstInfo{"PMULHRW",  TYPE_3DNOW_INST, FLAGS_NONE, 0, nullptr}},
  };

#define OPD(prefix, opcode) ((prefix << 8) | opcode)
#define PF_38_NONE 0
#define PF_38_66 1
#define PF_38_F2 2

  const std::vector<std::tuple<uint16_t, uint8_t, X86InstInfo>> H0F38Table = {
    {OPD(PF_38_NONE, 0x00), 1, X86InstInfo{"PSHUFB",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_66,   0x00), 1, X86InstInfo{"PSHUFB",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_NONE, 0x01), 1, X86InstInfo{"PHADDW",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_66,   0x01), 1, X86InstInfo{"PHADDW",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_NONE, 0x02), 1, X86InstInfo{"PHADDD",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_66,   0x02), 1, X86InstInfo{"PHADDD",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_NONE, 0x03), 1, X86InstInfo{"PHADDSW",    TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_66,   0x03), 1, X86InstInfo{"PHADDSW",    TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_NONE, 0x04), 1, X86InstInfo{"PMADDUBSW",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_66,   0x04), 1, X86InstInfo{"PMADDUBSW",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_NONE, 0x05), 1, X86InstInfo{"PHSUBW",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_66,   0x05), 1, X86InstInfo{"PHSUBW",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_NONE, 0x06), 1, X86InstInfo{"PHSUBD",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_66,   0x06), 1, X86InstInfo{"PHSUBD",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_NONE, 0x07), 1, X86InstInfo{"PHSUBSW",    TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_66,   0x07), 1, X86InstInfo{"PHSUBSW",    TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_NONE, 0x08), 1, X86InstInfo{"PSIGNB",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_66,   0x08), 1, X86InstInfo{"PSIGNB",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_NONE, 0x09), 1, X86InstInfo{"PSIGNW",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_66,   0x09), 1, X86InstInfo{"PSIGNW",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_NONE, 0x0A), 1, X86InstInfo{"PSIGND",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_66,   0x0A), 1, X86InstInfo{"PSIGND",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_NONE, 0x0B), 1, X86InstInfo{"PMULHRSW",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_66,   0x0B), 1, X86InstInfo{"PMULHRSW",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(PF_38_66,   0x10), 1, X86InstInfo{"PBLENDVB",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_66,   0x14), 1, X86InstInfo{"BLENDVPS",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_66,   0x15), 1, X86InstInfo{"BLENDVPD",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_66,   0x17), 1, X86InstInfo{"PTEST",      TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_NONE, 0x1C), 1, X86InstInfo{"PABSB",      TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_66,   0x1C), 1, X86InstInfo{"PABSB",      TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_NONE, 0x1D), 1, X86InstInfo{"PABSW",      TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_66,   0x1D), 1, X86InstInfo{"PABSW",      TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_NONE, 0x1E), 1, X86InstInfo{"PABSD",      TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_66,   0x1E), 1, X86InstInfo{"PABSD",      TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

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
    {OPD(PF_38_66,   0x3B), 1, X86InstInfo{"PMINUD",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_66,   0x3C), 1, X86InstInfo{"PMAXSB",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_66,   0x3D), 1, X86InstInfo{"PMAXSD",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_66,   0x3E), 1, X86InstInfo{"PMAXUW",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_66,   0x3F), 1, X86InstInfo{"PMAXUD",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(PF_38_66,   0x40), 1, X86InstInfo{"PMULLD",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_66,   0x41), 1, X86InstInfo{"PHMINPOSUW", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(PF_38_66,   0xDB), 1, X86InstInfo{"AESIMC",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_66,   0xDC), 1, X86InstInfo{"AESENC",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_66,   0xDD), 1, X86InstInfo{"AESENCLAST", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_66,   0xDE), 1, X86InstInfo{"AESDEC",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_66,   0xDF), 1, X86InstInfo{"AESDECLAST", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(PF_38_NONE, 0xF0), 1, X86InstInfo{"MOVBE",      TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_NONE, 0xF1), 1, X86InstInfo{"MOVBE",      TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(PF_38_66,   0xF0), 1, X86InstInfo{"CRC32",      TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_66,   0xF1), 1, X86InstInfo{"CRC32",      TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(PF_38_F2,   0xF0), 1, X86InstInfo{"CRC32",      TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(PF_38_F2,   0xF1), 1, X86InstInfo{"CRC32",      TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
  };
#undef PF_38_NONE
#undef PF_38_66
#undef PF_38_F2
#undef OPD

#define OPD(REX, prefix, opcode) ((REX << 9) | (prefix << 8) | opcode)
#define PF_3A_NONE 0
#define PF_3A_66   1

  const std::vector<std::tuple<uint16_t, uint8_t, X86InstInfo>> H0F3ATable = {
    {OPD(0, PF_3A_NONE, 0x0F), 1, X86InstInfo{"PALIGNR",         TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(0, PF_3A_66,   0x08), 1, X86InstInfo{"ROUNDPS",         TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(0, PF_3A_66,   0x09), 1, X86InstInfo{"ROUNDPD",         TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(0, PF_3A_66,   0x0A), 1, X86InstInfo{"ROUNDSS",         TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(0, PF_3A_66,   0x0B), 1, X86InstInfo{"ROUNDSD",         TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(0, PF_3A_66,   0x0C), 1, X86InstInfo{"BLENDPS",         TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(0, PF_3A_66,   0x0D), 1, X86InstInfo{"BLENDPD",         TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(0, PF_3A_66,   0x0E), 1, X86InstInfo{"PBLENDW",         TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(0, PF_3A_66,   0x0F), 1, X86InstInfo{"PALIGNR",         TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(0, PF_3A_66,   0x14), 1, X86InstInfo{"PEXTRB",          TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(0, PF_3A_66,   0x15), 1, X86InstInfo{"PEXTRW",          TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(0, PF_3A_66,   0x16), 1, X86InstInfo{"PEXTRD",          TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, PF_3A_66,   0x16), 1, X86InstInfo{"PEXTRD",          TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(0, PF_3A_66,   0x17), 1, X86InstInfo{"EXTRACTPS",       TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(0, PF_3A_66,   0x20), 1, X86InstInfo{"PINSRB",          TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(0, PF_3A_66,   0x21), 1, X86InstInfo{"INSERTPS",        TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(0, PF_3A_66,   0x22), 1, X86InstInfo{"PINSRD",          TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, PF_3A_66,   0x22), 1, X86InstInfo{"PINSRQ",          TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(0, PF_3A_66,   0x40), 1, X86InstInfo{"DPPS",            TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(0, PF_3A_66,   0x41), 1, X86InstInfo{"DPPD",            TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(0, PF_3A_66,   0x42), 1, X86InstInfo{"MPSADBW",         TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(0, PF_3A_66,   0x44), 1, X86InstInfo{"PCLMULQDQ",       TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(0, PF_3A_66,   0x60), 1, X86InstInfo{"PCMPESTRM",       TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(0, PF_3A_66,   0x61), 1, X86InstInfo{"PCMPESTRI",       TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(0, PF_3A_66,   0x62), 1, X86InstInfo{"PCMPISTRM",       TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(0, PF_3A_66,   0x63), 1, X86InstInfo{"PCMPISTRI",       TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(0, PF_3A_66,   0xDF), 1, X86InstInfo{"AESKEYGENASSIST", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
  };
#undef PF_3A_NONE
#undef PF_3A_66

#undef OPD

#define OPD(map_select, pp, opcode) (((map_select - 1) << 10) | (pp << 8) | (opcode))
  const std::vector<std::tuple<uint16_t, uint8_t, X86InstInfo>> VEXTable = {
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
    {OPD(1, 0b01, 0x64), 1, X86InstInfo{"VPCMPGTB",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0x65), 1, X86InstInfo{"VPVMPGTW",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0x66), 1, X86InstInfo{"VPVMPGTD",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0x67), 1, X86InstInfo{"VPACKUSWB",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(1, 0b01, 0x70), 1, X86InstInfo{"VPSHUFD",    TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b10, 0x70), 1, X86InstInfo{"VPSHUFHW",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b11, 0x70), 1, X86InstInfo{"VPSHUFLW",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(1, 0b01, 0x71), 1, X86InstInfo{"",           TYPE_UNDEC, FLAGS_NONE, 0, nullptr}}, // VEX Group 12
    {OPD(1, 0b01, 0x72), 1, X86InstInfo{"",           TYPE_UNDEC, FLAGS_NONE, 0, nullptr}}, // VEX Group 13
    {OPD(1, 0b01, 0x73), 1, X86InstInfo{"",           TYPE_UNDEC, FLAGS_NONE, 0, nullptr}}, // VEX Group 14

    {OPD(1, 0b01, 0x74), 1, X86InstInfo{"VPCMPEQB",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0x75), 1, X86InstInfo{"VPCMPEQW",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0x76), 1, X86InstInfo{"VPCMPEQD",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(1, 0b00, 0x77), 1, X86InstInfo{"VZERO*",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

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
    {OPD(1, 0b01, 0x6E), 1, X86InstInfo{"VMOV*",       TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(1, 0b01, 0x6F), 1, X86InstInfo{"VMOVDQA",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b10, 0x6F), 1, X86InstInfo{"VMOVDQU",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(1, 0b01, 0x7C), 1, X86InstInfo{"VHADDPD",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b11, 0x7C), 1, X86InstInfo{"VHADDPS",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(1, 0b01, 0x7D), 1, X86InstInfo{"VHSUBPD",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b11, 0x7D), 1, X86InstInfo{"VHSUBPS",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(1, 0b01, 0x7E), 1, X86InstInfo{"VMOV*",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b10, 0x7E), 1, X86InstInfo{"VMOVQ",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(1, 0b01, 0x7F), 1, X86InstInfo{"VMOVDQA",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b10, 0x7F), 1, X86InstInfo{"VMOVDQU",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(1, 0b00, 0xAE), 1, X86InstInfo{"",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}}, // VEX Group 15
    {OPD(1, 0b01, 0xAE), 1, X86InstInfo{"",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}}, // VEX Group 15
    {OPD(1, 0b10, 0xAE), 1, X86InstInfo{"",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}}, // VEX Group 15
    {OPD(1, 0b11, 0xAE), 1, X86InstInfo{"",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}}, // VEX Group 15

    {OPD(1, 0b01, 0xD0), 1, X86InstInfo{"VADDSUBPD",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b11, 0xD0), 1, X86InstInfo{"VADDSUBPS",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(1, 0b01, 0xD1), 1, X86InstInfo{"VPSRLW",      TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0xD2), 1, X86InstInfo{"VPSRLD",      TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0xD3), 1, X86InstInfo{"VPSRLQ",      TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0xD4), 1, X86InstInfo{"VPADDQ",      TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0xD5), 1, X86InstInfo{"VPMULLW",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0xD6), 1, X86InstInfo{"VMOVQ",       TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0xD7), 1, X86InstInfo{"VPMOVMSKB",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(1, 0b01, 0xD8), 1, X86InstInfo{"VPSUBUSB", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0xD9), 1, X86InstInfo{"VPSUBUSW", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0xDA), 1, X86InstInfo{"VPMINUB",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0xDB), 1, X86InstInfo{"VPAND",    TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0xDC), 1, X86InstInfo{"VPADDUSB", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0xDD), 1, X86InstInfo{"VPADDUSW", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0xDE), 1, X86InstInfo{"VPMAXUB",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0xDF), 1, X86InstInfo{"VPANDN",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(1, 0b01, 0xE0), 1, X86InstInfo{"VPAVGB",      TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0xE1), 1, X86InstInfo{"VPSRAW",      TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0xE2), 1, X86InstInfo{"VPSRAD",      TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0xE3), 1, X86InstInfo{"VPAVGW",      TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0xE4), 1, X86InstInfo{"VPMULHUW",    TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0xE5), 1, X86InstInfo{"VPMULHW",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(1, 0b01, 0xE6), 1, X86InstInfo{"VCVTTPD2DQ",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b10, 0xE6), 1, X86InstInfo{"VCVTDQ2PD",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b11, 0xE6), 1, X86InstInfo{"VCVTPD2DQ",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(1, 0b01, 0xE7), 1, X86InstInfo{"VMOVNTDQ",    TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(1, 0b01, 0xE8), 1, X86InstInfo{"VPSUBSB", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0xE9), 1, X86InstInfo{"VPSUBSW", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0xEA), 1, X86InstInfo{"VPMINSW",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0xEB), 1, X86InstInfo{"VPOR",    TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0xEC), 1, X86InstInfo{"VPADDSB", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0xED), 1, X86InstInfo{"VPADDSW", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0xEE), 1, X86InstInfo{"VPMAXSW",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0xEF), 1, X86InstInfo{"VPXOR",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(1, 0b11, 0xF0), 1, X86InstInfo{"VLDDQU",      TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(1, 0b01, 0xF1), 1, X86InstInfo{"VPSLLW",      TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0xF2), 1, X86InstInfo{"VPSLLD",      TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0xF3), 1, X86InstInfo{"VPSLLQ",      TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0xF4), 1, X86InstInfo{"VPMULUDQ",    TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0xF5), 1, X86InstInfo{"VPMADDWD",    TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0xF6), 1, X86InstInfo{"VPSADBW",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0xF7), 1, X86InstInfo{"VMASKMOVDQU", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(1, 0b01, 0xF8), 1, X86InstInfo{"VPSUBB", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0xF9), 1, X86InstInfo{"VPSUBW", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0xFA), 1, X86InstInfo{"VPSUBD", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0xFB), 1, X86InstInfo{"VPSUBQ", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0xFC), 1, X86InstInfo{"VPADDB", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0xFD), 1, X86InstInfo{"VPADDW", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 0b01, 0xFE), 1, X86InstInfo{"VPADDD", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    // VEX Map 2
    {OPD(2, 0b01, 0x00), 1, X86InstInfo{"VSHUFB", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
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
    {OPD(2, 0b01, 0x3B), 1, X86InstInfo{"VPMINUD", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x3C), 1, X86InstInfo{"VPMAXSB", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x3D), 1, X86InstInfo{"VPMAXSD", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x3E), 1, X86InstInfo{"VPMAXUW", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x3F), 1, X86InstInfo{"VPMAXUD", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(2, 0b01, 0x40), 1, X86InstInfo{"VPMULLD", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x41), 1, X86InstInfo{"VPHMINPOSUW", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x45), 1, X86InstInfo{"VPSRLV", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x46), 1, X86InstInfo{"VPSRAVD", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x47), 1, X86InstInfo{"VPSLLV", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(2, 0b01, 0x58), 1, X86InstInfo{"VPBROADCASTD", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x59), 1, X86InstInfo{"VPBROADCASTQ", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x5A), 1, X86InstInfo{"VPBROADCASTI128", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(2, 0b01, 0x78), 1, X86InstInfo{"VPBROADCASTB", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 0b01, 0x79), 1, X86InstInfo{"VPBROADCASTW", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

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

    {OPD(2, 0b00, 0xF3), 1, X86InstInfo{"", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}}, // VEX Group 17
    {OPD(2, 0b01, 0xF3), 1, X86InstInfo{"", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}}, // VEX Group 17
    {OPD(2, 0b10, 0xF3), 1, X86InstInfo{"", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}}, // VEX Group 17
    {OPD(2, 0b11, 0xF3), 1, X86InstInfo{"", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}}, // VEX Group 17

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
    {OPD(3, 0b01, 0x0F), 1, X86InstInfo{"VALIGNR", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

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
    {OPD(3, 0b01, 0x63), 1, X86InstInfo{"VPCMPISTRI", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

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
#define OPD(group, opcode) ((group << 3) | (opcode))
#define VEX_GROUP_12 0
#define VEX_GROUP_13 1
#define VEX_GROUP_14 2
#define VEX_GROUP_15 3
#define VEX_GROUP_17 4

  const std::vector<std::tuple<uint8_t, uint8_t, X86InstInfo>> VEXGroupTable = {
    {OPD(VEX_GROUP_12, 0b010), 1, X86InstInfo{"VPSRLW",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(VEX_GROUP_12, 0b100), 1, X86InstInfo{"VPSRAW",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(VEX_GROUP_12, 0b110), 1, X86InstInfo{"VPSLLW",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(VEX_GROUP_13, 0b010), 1, X86InstInfo{"VPSRLD",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(VEX_GROUP_13, 0b100), 1, X86InstInfo{"VPSRAD",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(VEX_GROUP_13, 0b110), 1, X86InstInfo{"VPSLLD",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(VEX_GROUP_14, 0b010), 1, X86InstInfo{"VPSRLQ",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(VEX_GROUP_14, 0b011), 1, X86InstInfo{"VPSRLDQ",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(VEX_GROUP_14, 0b110), 1, X86InstInfo{"VPSLLQ",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(VEX_GROUP_14, 0b111), 1, X86InstInfo{"VPSLLDQ",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(VEX_GROUP_15, 0b010), 1, X86InstInfo{"VLDMXCSR", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(VEX_GROUP_15, 0b011), 1, X86InstInfo{"VSTMXCSR", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(VEX_GROUP_17, 0b001), 1, X86InstInfo{"BLSR",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(VEX_GROUP_17, 0b010), 1, X86InstInfo{"BLSMSK",   TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(VEX_GROUP_17, 0b011), 1, X86InstInfo{"BLSI",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
  };

#undef VEX_GROUP_12
#undef VEX_GROUP_13
#undef VEX_GROUP_14
#undef VEX_GROUP_15
#undef VEX_GROUP_17

#undef OPD

#define OPD(group, pp, opcode) ( (group << 10) | (pp << 8) | (opcode))
#define XOP_GROUP_8 0
#define XOP_GROUP_9 1
#define XOP_GROUP_A 2

  const std::vector<std::tuple<uint16_t, uint8_t, X86InstInfo>> XOPTable = {
    // Group 8
    {OPD(XOP_GROUP_8, 0, 0x85), 1, X86InstInfo{"VPMAXSSWW",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(XOP_GROUP_8, 0, 0x86), 1, X86InstInfo{"VPMACSSWD",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(XOP_GROUP_8, 0, 0x87), 1, X86InstInfo{"VPMAXSSDQL", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(XOP_GROUP_8, 0, 0x8E), 1, X86InstInfo{"VPMACSSDD",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(XOP_GROUP_8, 0, 0x8F), 1, X86InstInfo{"VPMACSSDQH", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(XOP_GROUP_8, 0, 0x95), 1, X86InstInfo{"VPMAXSWW",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(XOP_GROUP_8, 0, 0x96), 1, X86InstInfo{"VPMAXSWD",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(XOP_GROUP_8, 0, 0x97), 1, X86InstInfo{"VPMAXSDQL", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(XOP_GROUP_8, 0, 0x9E), 1, X86InstInfo{"VPMACSDD",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(XOP_GROUP_8, 0, 0x9F), 1, X86InstInfo{"VPMACSDQH", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(XOP_GROUP_8, 0, 0xA2), 1, X86InstInfo{"VPCMOV",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(XOP_GROUP_8, 0, 0xA3), 1, X86InstInfo{"VPPERM",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(XOP_GROUP_8, 0, 0xA6), 1, X86InstInfo{"VPMADCSSWD", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(XOP_GROUP_8, 0, 0xB6), 1, X86InstInfo{"VPMADCSWD", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(XOP_GROUP_8, 0, 0xC0), 1, X86InstInfo{"VPROTB",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(XOP_GROUP_8, 0, 0xC1), 1, X86InstInfo{"VPROTW",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(XOP_GROUP_8, 0, 0xC2), 1, X86InstInfo{"VPROTD", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(XOP_GROUP_8, 0, 0xC3), 1, X86InstInfo{"VPROTQ", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(XOP_GROUP_8, 0, 0xCC), 1, X86InstInfo{"VPCOMccB",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(XOP_GROUP_8, 0, 0xCD), 1, X86InstInfo{"VPCOMccW",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(XOP_GROUP_8, 0, 0xCE), 1, X86InstInfo{"VPCOMccD", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(XOP_GROUP_8, 0, 0xCF), 1, X86InstInfo{"VPCOMccQ", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(XOP_GROUP_8, 0, 0xEC), 1, X86InstInfo{"VPCOMccUB",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(XOP_GROUP_8, 0, 0xED), 1, X86InstInfo{"VPCOMccUW",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(XOP_GROUP_8, 0, 0xEE), 1, X86InstInfo{"VPCOMccUD", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(XOP_GROUP_8, 0, 0xEF), 1, X86InstInfo{"VPCOMccUQ", TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    // Group 9
    {OPD(XOP_GROUP_9, 0, 0x01), 1, X86InstInfo{"",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}}, // Group 1
    {OPD(XOP_GROUP_9, 0, 0x02), 1, X86InstInfo{"",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}}, // Group 2
    {OPD(XOP_GROUP_9, 0, 0x12), 1, X86InstInfo{"",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}}, // Group 3

    {OPD(XOP_GROUP_9, 0, 0x80), 1, X86InstInfo{"VFRZPS",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(XOP_GROUP_9, 0, 0x81), 1, X86InstInfo{"VFRCZPD",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(XOP_GROUP_9, 0, 0x82), 1, X86InstInfo{"VFRCZSS",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(XOP_GROUP_9, 0, 0x83), 1, X86InstInfo{"VFRCZSD",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(XOP_GROUP_9, 0, 0x90), 1, X86InstInfo{"VPROTB",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(XOP_GROUP_9, 0, 0x91), 1, X86InstInfo{"VPROTW",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(XOP_GROUP_9, 0, 0x92), 1, X86InstInfo{"VPROTD",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(XOP_GROUP_9, 0, 0x93), 1, X86InstInfo{"VRPTOQ",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(XOP_GROUP_9, 0, 0x94), 1, X86InstInfo{"VPSHLB",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(XOP_GROUP_9, 0, 0x95), 1, X86InstInfo{"VPSHLW",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(XOP_GROUP_9, 0, 0x96), 1, X86InstInfo{"VPSHLD",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(XOP_GROUP_9, 0, 0x97), 1, X86InstInfo{"VPSHLQ",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(XOP_GROUP_9, 0, 0x98), 1, X86InstInfo{"VPSHAB",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(XOP_GROUP_9, 0, 0x99), 1, X86InstInfo{"VPSHAW",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(XOP_GROUP_9, 0, 0x9A), 1, X86InstInfo{"VPSHAD",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(XOP_GROUP_9, 0, 0x9B), 1, X86InstInfo{"VPSHAQ",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(XOP_GROUP_9, 0, 0xC1), 1, X86InstInfo{"VPHADDBW",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(XOP_GROUP_9, 0, 0xC2), 1, X86InstInfo{"VPHADDBD",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(XOP_GROUP_9, 0, 0xC3), 1, X86InstInfo{"VPHADDBQ",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(XOP_GROUP_9, 0, 0xC6), 1, X86InstInfo{"VPHADDWD",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(XOP_GROUP_9, 0, 0xC7), 1, X86InstInfo{"VPHADDWQ",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(XOP_GROUP_9, 0, 0xCB), 1, X86InstInfo{"VPHADDDQ",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(XOP_GROUP_9, 0, 0xD1), 1, X86InstInfo{"VPHADDUBW",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(XOP_GROUP_9, 0, 0xD2), 1, X86InstInfo{"VPHADDUBD",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(XOP_GROUP_9, 0, 0xD3), 1, X86InstInfo{"VPHADDUBQ",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(XOP_GROUP_9, 0, 0xD6), 1, X86InstInfo{"VPHADDUWD",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(XOP_GROUP_9, 0, 0xD7), 1, X86InstInfo{"VPHADDUWQ",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(XOP_GROUP_9, 0, 0xDB), 1, X86InstInfo{"VPHADDUDQ",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    {OPD(XOP_GROUP_9, 0, 0xE1), 1, X86InstInfo{"VPHSUBBW",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(XOP_GROUP_9, 0, 0xE2), 1, X86InstInfo{"VPHSUBBD",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(XOP_GROUP_9, 0, 0xE3), 1, X86InstInfo{"VPHSUBDQ",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    // Group A
    {OPD(XOP_GROUP_A, 0, 0x10), 1, X86InstInfo{"BEXTR",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(XOP_GROUP_A, 0, 0x12), 1, X86InstInfo{"",  TYPE_UNDEC, FLAGS_NONE, 0, nullptr}}, // Group 4
  };

#undef XOP_GROUP_8
#undef XOP_GROUP_9
#undef XOP_GROUP_A

#undef OPD
#define OPD(subgroup, opcode)  (((subgroup - 1) << 3) | (opcode))
  const std::vector<std::tuple<uint8_t, uint8_t, X86InstInfo>> XOPGroupTable = {
    // Group 1
    {OPD(1, 1), 1, X86InstInfo{"BLCFILL",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 2), 1, X86InstInfo{"BLSFILL",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 3), 1, X86InstInfo{"BLCS",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 4), 1, X86InstInfo{"TZMSK",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 5), 1, X86InstInfo{"BLCIC",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 6), 1, X86InstInfo{"BLSIC",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(1, 7), 1, X86InstInfo{"T1MSKC",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    // Group 2
    {OPD(2, 1), 1, X86InstInfo{"BLCMSK",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(2, 6), 1, X86InstInfo{"BLCI",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    // Group 3
    {OPD(3, 0), 1, X86InstInfo{"LLWPCB",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(3, 1), 1, X86InstInfo{"SLWPCB",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},

    // Group 4
    {OPD(4, 0), 1, X86InstInfo{"LWPINS",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
    {OPD(4, 1), 1, X86InstInfo{"LWPVAL",     TYPE_UNDEC, FLAGS_NONE, 0, nullptr}},
  };
#undef OPD

  uint64_t Total{};
  uint64_t NumInsts{};
  auto GenerateTable = [&Total, &NumInsts](auto& FinalTable, auto& LocalTable) {
    for (auto Op : LocalTable) {
      auto OpNum = std::get<0>(Op);
      auto Info = std::get<2>(Op);
      for (uint8_t i = 0; i < std::get<1>(Op); ++i) {
        LogMan::Throw::A(FinalTable.at(OpNum + i).Type == TYPE_UNKNOWN, "Duplicate Entry %s->%s", FinalTable.at(OpNum + i).Name, Info.Name);
        FinalTable.at(OpNum + i) = Info;
        ++Total;
        if (Info.Type == TYPE_INST)
          NumInsts++;
      }
    }
  };

  auto GenerateX87Table = [&Total, &NumInsts](auto& FinalTable, auto& LocalTable) {
    for (auto Op : LocalTable) {
      auto OpNum = std::get<0>(Op);
      auto Info = std::get<2>(Op);
      for (uint8_t i = 0; i < std::get<1>(Op); ++i) {
        LogMan::Throw::A(FinalTable.at(OpNum + i).Type == TYPE_UNKNOWN, "Duplicate Entry %s->%s", FinalTable.at(OpNum + i).Name, Info.Name);
        if ((OpNum & 0b11'000'000) == 0b11'000'000) {
          // If the mod field is 0b11 then it is a regular op
          FinalTable.at(OpNum + i) = Info;
        }
        else {
          // If the mod field is !0b11 then this instruction is duplicated through the whole mod [0b00, 0b10] range
          // and the modrm.rm space because that is used part of the instruction encoding
          LogMan::Throw::A((OpNum & 0b11'000'000) == 0, "Only support mod field of zero in this path");
          for (uint16_t mod = 0b00'000'000; mod < 0b11'000'000; mod += 0b01'000'000) {
            for (uint16_t rm = 0b000; rm < 0b1'000; ++rm) {
              FinalTable.at((OpNum | mod | rm) + i) = Info;
            }
          }
        }
        Total++;
        if (Info.Type == TYPE_INST) {
          NumInsts++;
        }
      }
    }
  };

  auto GenerateTableWithCopy = [&Total, &NumInsts](auto& FinalTable, auto& LocalTable, auto& OtherLocal) {
    for (auto Op : LocalTable) {
      auto OpNum = std::get<0>(Op);
      auto Info = std::get<2>(Op);
      for (uint8_t i = 0; i < std::get<1>(Op); ++i) {
        LogMan::Throw::A(FinalTable.at(OpNum + i).Type == TYPE_UNKNOWN, "Duplicate Entry %s->%s", FinalTable.at(OpNum + i).Name, Info.Name);
        if (Info.Type == TYPE_COPY_OTHER) {
          FinalTable.at(OpNum + i) = OtherLocal.at(OpNum + i);
        }
        else {
          FinalTable.at(OpNum + i) = Info;
          Total++;
          if (Info.Type == TYPE_INST) {
            NumInsts++;
          }
        }
      }
    }
  };

  auto CheckTable = [&UnknownOp](auto& FinalTable) {
    for (size_t i = 0; i < FinalTable.size(); ++i) {
      auto const &Op = FinalTable.at(i);

      if (Op == UnknownOp) {
        LogMan::Msg::A("Unknown Op: 0x%lx", i);
      }
    }
  };

  GenerateTable(BaseOps, BaseOpTable);
  GenerateTable(SecondBaseOps, TwoByteOpTable);
  GenerateTable(PrimaryInstGroupOps, PrimaryGroupOpTable);

  GenerateTableWithCopy(RepModOps, RepModOpTable, SecondBaseOps);
  GenerateTableWithCopy(RepNEModOps, RepNEModOpTable, SecondBaseOps);
  GenerateTableWithCopy(OpSizeModOps, OpSizeModOpTable, SecondBaseOps);
  GenerateTable(SecondInstGroupOps, SecondaryExtensionOpTable);
  GenerateTable(SecondModRMTableOps, SecondaryModRMExtensionOpTable);

  GenerateX87Table(X87Ops, X87OpTable);
  GenerateTable(DDDNowOps, DDDNowOpTable);
  GenerateTable(H0F38TableOps, H0F38Table);
  GenerateTable(H0F3ATableOps, H0F3ATable);

  GenerateTable(VEXTableOps, VEXTable);
  GenerateTable(VEXTableGroupOps, VEXGroupTable);

  GenerateTable(XOPTableOps, XOPTable);
  GenerateTable(XOPTableGroupOps, XOPGroupTable);

  CheckTable(BaseOps);
  CheckTable(SecondBaseOps);

  CheckTable(RepModOps);
  CheckTable(RepNEModOps);
  CheckTable(OpSizeModOps);
  CheckTable(X87Ops);

#ifndef NDEBUG
  X86InstDebugInfo::InstallDebugInfo();
#endif

  printf("X86Tables had %ld total insts, and %ld labeled as understood\n", Total, NumInsts);
}

}
