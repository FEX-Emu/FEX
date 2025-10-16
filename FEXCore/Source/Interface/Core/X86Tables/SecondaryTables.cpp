// SPDX-License-Identifier: MIT
/*
$info$
tags: frontend|x86-tables
$end_info$
*/

#include "Interface/Core/X86Tables/X86Tables.h"
#include "Interface/Core/OpcodeDispatcher/SecondaryTables.h"

#include <FEXCore/Core/Context.h>

#include <iterator>

namespace FEXCore::X86Tables {
using namespace InstFlags;

enum Secondary_LUT {
  ENTRY_05,
  ENTRY_A0,
  ENTRY_A1,
  ENTRY_A8,
  ENTRY_A9,
  ENTRY_MAX,
};

constexpr std::array<X86InstInfo[2], ENTRY_MAX> Secondary_ArchSelect_LUT = {{
  {
    {"SYSCALL", TYPE_INST, DEFAULT_SYSCALL_FLAGS, 0, { .OpDispatch = &IR::OpDispatchBuilder::NOPOp } },
    {"SYSCALL", TYPE_INST, DEFAULT_SYSCALL_FLAGS, 0, { .OpDispatch = &IR::OpDispatchBuilder::Bind<&IR::OpDispatchBuilder::SyscallOp, true> } },
  },
  {
    {"PUSH FS", TYPE_INST, GenFlagsSrcSize(SIZE_16BIT) | FLAGS_DEBUG_MEM_ACCESS | FLAGS_NO_OVERLAY, 0, { .OpDispatch = &IR::OpDispatchBuilder::Bind<&IR::OpDispatchBuilder::PUSHSegmentOp, FEXCore::X86Tables::DecodeFlags::FLAG_FS_PREFIX> } },
    {"PUSH FS", TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_DEBUG_MEM_ACCESS | FLAGS_NO_OVERLAY, 0, { .OpDispatch = &IR::OpDispatchBuilder::Bind<&IR::OpDispatchBuilder::PUSHSegmentOp, FEXCore::X86Tables::DecodeFlags::FLAG_FS_PREFIX> } },
  },
  {
    {"POP FS",  TYPE_INST, GenFlagsSizes(SIZE_16BIT, SIZE_DEF) | FLAGS_DEBUG_MEM_ACCESS | FLAGS_NO_OVERLAY, 0, { .OpDispatch = &IR::OpDispatchBuilder::Bind<&IR::OpDispatchBuilder::POPSegmentOp, FEXCore::X86Tables::DecodeFlags::FLAG_FS_PREFIX> } },
    {"POP FS",  TYPE_INST, GenFlagsSizes(SIZE_16BIT, SIZE_64BIT) | FLAGS_DEBUG_MEM_ACCESS | FLAGS_NO_OVERLAY, 0, { .OpDispatch = &IR::OpDispatchBuilder::Bind<&IR::OpDispatchBuilder::POPSegmentOp, FEXCore::X86Tables::DecodeFlags::FLAG_FS_PREFIX> } },
  },
  {
    {"PUSH GS", TYPE_INST, GenFlagsSrcSize(SIZE_16BIT) | FLAGS_DEBUG_MEM_ACCESS | FLAGS_NO_OVERLAY, 0, { .OpDispatch = &IR::OpDispatchBuilder::Bind<&IR::OpDispatchBuilder::PUSHSegmentOp, FEXCore::X86Tables::DecodeFlags::FLAG_GS_PREFIX> } },
    {"PUSH GS", TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_DEBUG_MEM_ACCESS | FLAGS_NO_OVERLAY, 0, { .OpDispatch = &IR::OpDispatchBuilder::Bind<&IR::OpDispatchBuilder::PUSHSegmentOp, FEXCore::X86Tables::DecodeFlags::FLAG_GS_PREFIX> } },
  },
  {
    {"POP GS",  TYPE_INST, GenFlagsSizes(SIZE_16BIT, SIZE_DEF) | FLAGS_DEBUG_MEM_ACCESS | FLAGS_NO_OVERLAY, 0, { .OpDispatch = &IR::OpDispatchBuilder::Bind<&IR::OpDispatchBuilder::POPSegmentOp, FEXCore::X86Tables::DecodeFlags::FLAG_GS_PREFIX> } },
    {"POP GS",  TYPE_INST, GenFlagsSizes(SIZE_16BIT, SIZE_64BIT) | FLAGS_DEBUG_MEM_ACCESS | FLAGS_NO_OVERLAY, 0, { .OpDispatch = &IR::OpDispatchBuilder::Bind<&IR::OpDispatchBuilder::POPSegmentOp, FEXCore::X86Tables::DecodeFlags::FLAG_GS_PREFIX> } },
  },
}};

constexpr std::array<X86InstInfo, MAX_SECOND_TABLE_SIZE> SecondBaseOps = []() consteval {
  std::array<X86InstInfo, MAX_SECOND_TABLE_SIZE> Table{};

  constexpr U8U8InfoStruct TwoByteOpTable[] = {
    // Instructions
    {0x00, 1, X86InstInfo{"",           TYPE_GROUP_6, FLAGS_MODRM | FLAGS_NO_OVERLAY,                                                                                 0}},
    {0x01, 1, X86InstInfo{"",           TYPE_GROUP_7, FLAGS_NO_OVERLAY,                                                                                 0}},
    // These two load segment register data
    {0x02, 1, X86InstInfo{"LAR",        TYPE_UNDEC, FLAGS_NO_OVERLAY,                                                                                   0}},
    {0x03, 1, X86InstInfo{"LSL",        TYPE_INST, GenFlagsSrcSize(SIZE_16BIT) | FLAGS_MODRM | FLAGS_NO_OVERLAY,                                                                                    0}},
    {0x04, 1, X86InstInfo{"",           TYPE_INVALID, FLAGS_NO_OVERLAY,                                                                                 0}},
    {0x05, 1, X86InstInfo{"",           TYPE_ARCH_DISPATCHER, FLAGS_NONE, 0, { .Indirect = Secondary_ArchSelect_LUT[ENTRY_05] }}},
    {0x06, 1, X86InstInfo{"CLTS",       TYPE_INST, FLAGS_NO_OVERLAY,                                                                                    0}},
    {0x07, 1, X86InstInfo{"SYSRET",     TYPE_INST, FLAGS_NO_OVERLAY,                                                                                    0}},
    {0x08, 1, X86InstInfo{"INVD",       TYPE_PRIV, FLAGS_NO_OVERLAY,                                                                                    0}},
    {0x09, 1, X86InstInfo{"WBINVD",     TYPE_PRIV, FLAGS_NO_OVERLAY,                                                                                    0}},
    {0x0A, 1, X86InstInfo{"",           TYPE_INVALID, FLAGS_NO_OVERLAY,                                                                                 0}},
    {0x0B, 1, X86InstInfo{"UD2",        TYPE_INST, FLAGS_BLOCK_END | FLAGS_NO_OVERLAY,                                                    0}},
    {0x0C, 1, X86InstInfo{"",           TYPE_INVALID, FLAGS_NO_OVERLAY,                                                                                 0}},
    {0x0D, 1, X86InstInfo{"",           TYPE_GROUP_P, FLAGS_MODRM | FLAGS_NO_OVERLAY,                                                                                 0}},
    {0x0E, 1, X86InstInfo{"FEMMS",      TYPE_INST, FLAGS_NO_OVERLAY,                                                            0}},
    {0x0F, 1, X86InstInfo{"",           TYPE_3DNOW_TABLE, FLAGS_NO_OVERLAY,                                                                             0}},

    {0x10, 1, X86InstInfo{"MOVUPS",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                 0}},
    {0x11, 1, X86InstInfo{"MOVUPS",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS,                                                 0}},
    {0x12, 1, X86InstInfo{"MOVLPS",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                              0}},
    {0x13, 1, X86InstInfo{"MOVLPS",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS,                              0}},
    {0x14, 1, X86InstInfo{"UNPCKLPS",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                 0}},
    {0x15, 1, X86InstInfo{"UNPCKHPS",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                 0}},
    {0x16, 1, X86InstInfo{"MOVLHPS",    TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                 0}},
    {0x17, 1, X86InstInfo{"MOVHPS",     TYPE_INST, GenFlagsSizes(SIZE_64BIT, SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS, 0}},
    {0x18, 1, X86InstInfo{"",           TYPE_GROUP_16, FLAGS_NO_OVERLAY,                                                                                      0}},
    {0x19, 7, X86InstInfo{"NOP",        TYPE_INST, FLAGS_MODRM | FLAGS_NO_OVERLAY,                                                                                     0}},

    {0x20, 2, X86InstInfo{"MOV",        TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_NO_OVERLAY,                                                     0}},
    {0x22, 2, X86InstInfo{"MOV",        TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_NO_OVERLAY,                                                     0}},
    {0x24, 4, X86InstInfo{"",           TYPE_INVALID, FLAGS_NONE,                                                                                       0}},
    {0x28, 1, X86InstInfo{"MOVAPS",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                 0}},
    {0x29, 1, X86InstInfo{"MOVAPS",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS,                                                 0}},
    {0x2A, 1, X86InstInfo{"CVTPI2PS",   TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX_SRC,                                                   0}},
    {0x2B, 1, X86InstInfo{"MOVNTPS",    TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS,                              0}},
    {0x2C, 1, X86InstInfo{"CVTTPS2PI",  TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX_DST,                                                   0}},
    {0x2D, 1, X86InstInfo{"CVTPS2PI",   TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX_DST,                                                   0}},
    {0x2E, 1, X86InstInfo{"UCOMISS",    TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_32BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                                   0}},
    {0x2F, 1, X86InstInfo{"COMISS",     TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_32BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                                   0}},

    {0x30, 1, X86InstInfo{"WRMSR",      TYPE_INST, FLAGS_NO_OVERLAY,                                                                             0}},
    {0x31, 1, X86InstInfo{"RDTSC",      TYPE_INST, FLAGS_NO_OVERLAY,                                                               0}},
    {0x32, 1, X86InstInfo{"RDMSR",      TYPE_INST, FLAGS_NO_OVERLAY,                                                                             0}},
    {0x33, 1, X86InstInfo{"RDPMC",      TYPE_INST, FLAGS_NO_OVERLAY,                                                                             0}},
    {0x34, 1, X86InstInfo{"SYSENTER",   TYPE_INST, FLAGS_NO_OVERLAY,                                                                             0}},
    {0x35, 1, X86InstInfo{"SYSEXIT",    TYPE_INST, FLAGS_NO_OVERLAY,                                                                             0}},
    {0x36, 1, X86InstInfo{"",           TYPE_INVALID, FLAGS_NO_OVERLAY,                                                                          0}},
    {0x37, 1, X86InstInfo{"GETSEC",     TYPE_INVALID, FLAGS_NO_OVERLAY,                                                                          0}},
    {0x38, 1, X86InstInfo{"",           TYPE_0F38_TABLE, FLAGS_NO_OVERLAY,                                                                       0}},
    {0x39, 1, X86InstInfo{"",           TYPE_INVALID, FLAGS_NO_OVERLAY,                                                                          0}},
    {0x3A, 1, X86InstInfo{"",           TYPE_0F3A_TABLE, FLAGS_NO_OVERLAY,                                                                       0}},
    {0x3B, 3, X86InstInfo{"",           TYPE_INVALID, FLAGS_NO_OVERLAY,                                                                          0}},

    {0x40, 1, X86InstInfo{"CMOVO",      TYPE_INST, FLAGS_MODRM | FLAGS_NO_OVERLAY,                                                               0}},
    {0x41, 1, X86InstInfo{"CMOVNO",     TYPE_INST, FLAGS_MODRM | FLAGS_NO_OVERLAY,                                                               0}},
    {0x42, 1, X86InstInfo{"CMOVB",      TYPE_INST, FLAGS_MODRM | FLAGS_NO_OVERLAY,                                                               0}},
    {0x43, 1, X86InstInfo{"CMOVNB",     TYPE_INST, FLAGS_MODRM | FLAGS_NO_OVERLAY,                                                               0}},
    {0x44, 1, X86InstInfo{"CMOVZ",      TYPE_INST, FLAGS_MODRM | FLAGS_NO_OVERLAY,                                                               0}},
    {0x45, 1, X86InstInfo{"CMOVNZ",     TYPE_INST, FLAGS_MODRM | FLAGS_NO_OVERLAY,                                                               0}},
    {0x46, 1, X86InstInfo{"CMOVBE",     TYPE_INST, FLAGS_MODRM | FLAGS_NO_OVERLAY,                                                               0}},
    {0x47, 1, X86InstInfo{"CMOVNBE",    TYPE_INST, FLAGS_MODRM | FLAGS_NO_OVERLAY,                                                               0}},
    {0x48, 1, X86InstInfo{"CMOVS",      TYPE_INST, FLAGS_MODRM | FLAGS_NO_OVERLAY,                                                               0}},
    {0x49, 1, X86InstInfo{"CMOVNS",     TYPE_INST, FLAGS_MODRM | FLAGS_NO_OVERLAY,                                                               0}},
    {0x4A, 1, X86InstInfo{"CMOVP",      TYPE_INST, FLAGS_MODRM | FLAGS_NO_OVERLAY,                                                               0}},
    {0x4B, 1, X86InstInfo{"CMOVNP",     TYPE_INST, FLAGS_MODRM | FLAGS_NO_OVERLAY,                                                               0}},
    {0x4C, 1, X86InstInfo{"CMOVL",      TYPE_INST, FLAGS_MODRM | FLAGS_NO_OVERLAY,                                                               0}},
    {0x4D, 1, X86InstInfo{"CMOVNL",     TYPE_INST, FLAGS_MODRM | FLAGS_NO_OVERLAY,                                                               0}},
    {0x4E, 1, X86InstInfo{"CMOVLE",     TYPE_INST, FLAGS_MODRM | FLAGS_NO_OVERLAY,                                                               0}},
    {0x4F, 1, X86InstInfo{"CMOVNLE",    TYPE_INST, FLAGS_MODRM | FLAGS_NO_OVERLAY,                                                               0}},

    {0x50, 1, X86InstInfo{"MOVMSKPS",   TYPE_INST, GenFlagsSizes(SIZE_32BIT, SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_REG_ONLY | FLAGS_XMM_FLAGS | FLAGS_SF_DST_GPR,      0}},
    {0x51, 1, X86InstInfo{"SQRTPS",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                 0}},
    {0x52, 1, X86InstInfo{"RSQRTPS",    TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                 0}},
    {0x53, 1, X86InstInfo{"RCPPS",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                 0}},
    {0x54, 1, X86InstInfo{"ANDPS",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                 0}},
    {0x55, 1, X86InstInfo{"ANDNPS",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                 0}},
    {0x56, 1, X86InstInfo{"ORPS",       TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                 0}},
    {0x57, 1, X86InstInfo{"XORPS",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                 0}},
    {0x58, 1, X86InstInfo{"ADDPS",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                 0}},
    {0x59, 1, X86InstInfo{"MULPS",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                 0}},
    {0x5A, 1, X86InstInfo{"CVTPS2PD",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                 0}},
    {0x5B, 1, X86InstInfo{"CVTDQ2PS",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                 0}},
    {0x5C, 1, X86InstInfo{"SUBPS",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                 0}},
    {0x5D, 1, X86InstInfo{"MINPS",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                 0}},
    {0x5E, 1, X86InstInfo{"DIVPS",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                 0}},
    {0x5F, 1, X86InstInfo{"MAXPS",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                 0}},

    {0x60, 1, X86InstInfo{"PUNPCKLBW",  TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX,                                   0}},
    {0x61, 1, X86InstInfo{"PUNPCKLWD",  TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX,                                   0}},
    {0x62, 1, X86InstInfo{"PUNPCKLDQ",  TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX,                                   0}},
    {0x63, 1, X86InstInfo{"PACKSSWB",   TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX,                                   0}},
    {0x64, 1, X86InstInfo{"PCMPGTB",    TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX,                                   0}},
    {0x65, 1, X86InstInfo{"PCMPGTW",    TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX,                                   0}},
    {0x66, 1, X86InstInfo{"PCMPGTD",    TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX,                                   0}},
    {0x67, 1, X86InstInfo{"PACKUSWB",   TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX,                                   0}},
    {0x68, 1, X86InstInfo{"PUNPCKHBW",  TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX,                                   0}},
    {0x69, 1, X86InstInfo{"PUNPCKHWD",  TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX,                                   0}},
    {0x6A, 1, X86InstInfo{"PUNPCKHDQ",  TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX,                                   0}},
    {0x6B, 1, X86InstInfo{"PACKSSDW",   TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX,                                   0}},
    {0x6C, 2, X86InstInfo{"",           TYPE_INVALID, FLAGS_NONE,                                                                                       0}},
    {0x6E, 1, X86InstInfo{"MOVD",       TYPE_INST, GenFlagsDstSize(SIZE_64BIT)   | FLAGS_MODRM | FLAGS_SF_SRC_GPR | FLAGS_XMM_FLAGS | FLAGS_SF_MMX,                0}},
    {0x6F, 1, X86InstInfo{"MOVQ",       TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX,                                   0}},

    {0x70, 1, X86InstInfo{"PSHUFW",     TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX,                                   1}},
    {0x71, 1, X86InstInfo{"",           TYPE_GROUP_12, FLAGS_NO_OVERLAY,                                                                                0}},
    {0x72, 1, X86InstInfo{"",           TYPE_GROUP_13, FLAGS_NO_OVERLAY,                                                                                0}},
    {0x73, 1, X86InstInfo{"",           TYPE_GROUP_14, FLAGS_NO_OVERLAY,                                                                                0}},
    {0x74, 1, X86InstInfo{"PCMPEQB",    TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX,                         0}},
    {0x75, 1, X86InstInfo{"PCMPEQW",    TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX,                         0}},
    {0x76, 1, X86InstInfo{"PCMPEQD",    TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX,                         0}},
    {0x77, 1, X86InstInfo{"EMMS",       TYPE_INST, FLAGS_NONE,                                                                                    0}},
    {0x78, 6, X86InstInfo{"",           TYPE_INVALID, FLAGS_NONE,                                                                                       0}},
    {0x7E, 1, X86InstInfo{"MOVD",       TYPE_INST, GenFlagsSrcSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_DST_GPR | FLAGS_XMM_FLAGS | FLAGS_SF_MMX, 0}},
    {0x7F, 1, X86InstInfo{"MOVQ",       TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS | FLAGS_SF_MMX,                    0}},

    {0x80, 1, X86InstInfo{"JO",      TYPE_INST, GenFlagsSameSize(SIZE_64BITDEF) | FLAGS_SETS_RIP | FLAGS_SRC_SEXT | FLAGS_DISPLACE_SIZE_DIV_2 | FLAGS_NO_OVERLAY,    4}},
    {0x81, 1, X86InstInfo{"JNO",     TYPE_INST, GenFlagsSameSize(SIZE_64BITDEF) | FLAGS_SETS_RIP | FLAGS_SRC_SEXT | FLAGS_DISPLACE_SIZE_DIV_2 | FLAGS_NO_OVERLAY,    4}},
    {0x82, 1, X86InstInfo{"JB",      TYPE_INST, GenFlagsSameSize(SIZE_64BITDEF) | FLAGS_SETS_RIP | FLAGS_SRC_SEXT | FLAGS_DISPLACE_SIZE_DIV_2 | FLAGS_NO_OVERLAY,    4}},
    {0x83, 1, X86InstInfo{"JNB",     TYPE_INST, GenFlagsSameSize(SIZE_64BITDEF) | FLAGS_SETS_RIP | FLAGS_SRC_SEXT | FLAGS_DISPLACE_SIZE_DIV_2 | FLAGS_NO_OVERLAY,    4}},
    {0x84, 1, X86InstInfo{"JZ",      TYPE_INST, GenFlagsSameSize(SIZE_64BITDEF) | FLAGS_SETS_RIP | FLAGS_SRC_SEXT | FLAGS_DISPLACE_SIZE_DIV_2 | FLAGS_NO_OVERLAY,    4}},
    {0x85, 1, X86InstInfo{"JNZ",     TYPE_INST, GenFlagsSameSize(SIZE_64BITDEF) | FLAGS_SETS_RIP | FLAGS_SRC_SEXT | FLAGS_DISPLACE_SIZE_DIV_2 | FLAGS_NO_OVERLAY,    4}},
    {0x86, 1, X86InstInfo{"JBE",     TYPE_INST, GenFlagsSameSize(SIZE_64BITDEF) | FLAGS_SETS_RIP | FLAGS_SRC_SEXT | FLAGS_DISPLACE_SIZE_DIV_2 | FLAGS_NO_OVERLAY,    4}},
    {0x87, 1, X86InstInfo{"JNBE",    TYPE_INST, GenFlagsSameSize(SIZE_64BITDEF) | FLAGS_SETS_RIP | FLAGS_SRC_SEXT | FLAGS_DISPLACE_SIZE_DIV_2 | FLAGS_NO_OVERLAY,    4}},
    {0x88, 1, X86InstInfo{"JS",      TYPE_INST, GenFlagsSameSize(SIZE_64BITDEF) | FLAGS_SETS_RIP | FLAGS_SRC_SEXT | FLAGS_DISPLACE_SIZE_DIV_2 | FLAGS_NO_OVERLAY,    4}},
    {0x89, 1, X86InstInfo{"JNS",     TYPE_INST, GenFlagsSameSize(SIZE_64BITDEF) | FLAGS_SETS_RIP | FLAGS_SRC_SEXT | FLAGS_DISPLACE_SIZE_DIV_2 | FLAGS_NO_OVERLAY,    4}},
    {0x8A, 1, X86InstInfo{"JP",      TYPE_INST, GenFlagsSameSize(SIZE_64BITDEF) | FLAGS_SETS_RIP | FLAGS_SRC_SEXT | FLAGS_DISPLACE_SIZE_DIV_2 | FLAGS_NO_OVERLAY,    4}},
    {0x8B, 1, X86InstInfo{"JNP",     TYPE_INST, GenFlagsSameSize(SIZE_64BITDEF) | FLAGS_SETS_RIP | FLAGS_SRC_SEXT | FLAGS_DISPLACE_SIZE_DIV_2 | FLAGS_NO_OVERLAY,    4}},
    {0x8C, 1, X86InstInfo{"JL",      TYPE_INST, GenFlagsSameSize(SIZE_64BITDEF) | FLAGS_SETS_RIP | FLAGS_SRC_SEXT | FLAGS_DISPLACE_SIZE_DIV_2 | FLAGS_NO_OVERLAY,    4}},
    {0x8D, 1, X86InstInfo{"JNL",     TYPE_INST, GenFlagsSameSize(SIZE_64BITDEF) | FLAGS_SETS_RIP | FLAGS_SRC_SEXT | FLAGS_DISPLACE_SIZE_DIV_2 | FLAGS_NO_OVERLAY,    4}},
    {0x8E, 1, X86InstInfo{"JLE",     TYPE_INST, GenFlagsSameSize(SIZE_64BITDEF) | FLAGS_SETS_RIP | FLAGS_SRC_SEXT | FLAGS_DISPLACE_SIZE_DIV_2 | FLAGS_NO_OVERLAY,    4}},
    {0x8F, 1, X86InstInfo{"JNLE",    TYPE_INST, GenFlagsSameSize(SIZE_64BITDEF) | FLAGS_SETS_RIP | FLAGS_SRC_SEXT | FLAGS_DISPLACE_SIZE_DIV_2 | FLAGS_NO_OVERLAY,    4}},

    {0x90, 1, X86InstInfo{"SETO",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_NO_OVERLAY,                        0}},
    {0x91, 1, X86InstInfo{"SETNO",   TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_NO_OVERLAY,                        0}},
    {0x92, 1, X86InstInfo{"SETB",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_NO_OVERLAY,                        0}},
    {0x93, 1, X86InstInfo{"SETNB",   TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_NO_OVERLAY,                        0}},
    {0x94, 1, X86InstInfo{"SETZ",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_NO_OVERLAY,                        0}},
    {0x95, 1, X86InstInfo{"SETNZ",   TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_NO_OVERLAY,                        0}},
    {0x96, 1, X86InstInfo{"SETBE",   TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_NO_OVERLAY,                        0}},
    {0x97, 1, X86InstInfo{"SETNBE",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_NO_OVERLAY,                        0}},
    {0x98, 1, X86InstInfo{"SETS",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_NO_OVERLAY,                        0}},
    {0x99, 1, X86InstInfo{"SETNS",   TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_NO_OVERLAY,                        0}},
    {0x9A, 1, X86InstInfo{"SETP",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_NO_OVERLAY,                        0}},
    {0x9B, 1, X86InstInfo{"SETNP",   TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_NO_OVERLAY,                        0}},
    {0x9C, 1, X86InstInfo{"SETL",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_NO_OVERLAY,                        0}},
    {0x9D, 1, X86InstInfo{"SETNL",   TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_NO_OVERLAY,                        0}},
    {0x9E, 1, X86InstInfo{"SETLE",   TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_NO_OVERLAY,                        0}},
    {0x9F, 1, X86InstInfo{"SETNLE",  TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_NO_OVERLAY,                        0}},

    {0xA0, 1, X86InstInfo{"",        TYPE_ARCH_DISPATCHER, FLAGS_DEBUG_MEM_ACCESS | FLAGS_NO_OVERLAY, 0, { .Indirect = Secondary_ArchSelect_LUT[ENTRY_A0] }}},
    {0xA1, 1, X86InstInfo{"",        TYPE_ARCH_DISPATCHER, FLAGS_DEBUG_MEM_ACCESS | FLAGS_NO_OVERLAY, 0, { .Indirect = Secondary_ArchSelect_LUT[ENTRY_A1] }}},
    {0xA2, 1, X86InstInfo{"CPUID",   TYPE_INST,     FLAGS_SF_SRC_RAX | FLAGS_NO_OVERLAY,                                              0}},
    {0xA3, 1, X86InstInfo{"BT",      TYPE_INST,     FLAGS_DEBUG_MEM_ACCESS | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_NO_OVERLAY,                     0}},
    {0xA4, 1, X86InstInfo{"SHLD",    TYPE_INST,     FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_NO_OVERLAY,                                              1}},
    {0xA5, 1, X86InstInfo{"SHLD",    TYPE_INST,     FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_SRC_RCX | FLAGS_NO_OVERLAY,                           0}},
    {0xA6, 2, X86InstInfo{"",        TYPE_INVALID,  FLAGS_NO_OVERLAY,                                                                               0}},
    {0xA8, 1, X86InstInfo{"",        TYPE_ARCH_DISPATCHER, FLAGS_DEBUG_MEM_ACCESS | FLAGS_NO_OVERLAY, 0, { .Indirect = Secondary_ArchSelect_LUT[ENTRY_A8] }}},
    {0xA9, 1, X86InstInfo{"",        TYPE_ARCH_DISPATCHER, FLAGS_DEBUG_MEM_ACCESS | FLAGS_NO_OVERLAY, 0, { .Indirect = Secondary_ArchSelect_LUT[ENTRY_A9] }}},
    {0xAA, 1, X86InstInfo{"RSM",     TYPE_PRIV,     FLAGS_NO_OVERLAY,                                                                               0}},
    {0xAB, 1, X86InstInfo{"BTS",     TYPE_INST,     FLAGS_DEBUG_MEM_ACCESS | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_NO_OVERLAY,                     0}},
    {0xAC, 1, X86InstInfo{"SHRD",    TYPE_INST,     FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_NO_OVERLAY,                                              1}},
    {0xAD, 1, X86InstInfo{"SHRD",    TYPE_INST,     FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_SRC_RCX | FLAGS_NO_OVERLAY,                           0}},
    {0xAE, 1, X86InstInfo{"",        TYPE_GROUP_15, FLAGS_NO_OVERLAY,                                                                               0}},
    {0xAF, 1, X86InstInfo{"IMUL",    TYPE_INST,     FLAGS_MODRM | FLAGS_NO_OVERLAY,                                                                 0}},

    {0xB0, 1, X86InstInfo{"CMPXCHG", TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_NO_OVERLAY,                    0}},
    {0xB1, 1, X86InstInfo{"CMPXCHG", TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_NO_OVERLAY,                                                  0}},
    {0xB2, 1, X86InstInfo{"LSS",     TYPE_INVALID, FLAGS_NO_OVERLAY,                                                                                0}},
    {0xB3, 1, X86InstInfo{"BTR",     TYPE_INST, FLAGS_DEBUG_MEM_ACCESS | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_NO_OVERLAY,                         0}},
    {0xB4, 1, X86InstInfo{"LFS",     TYPE_INVALID, FLAGS_NO_OVERLAY,                                                                                0}},
    {0xB5, 1, X86InstInfo{"LGS",     TYPE_INVALID, FLAGS_NO_OVERLAY,                                                                                0}},
    {0xB6, 1, X86InstInfo{"MOVZX",   TYPE_INST, GenFlagsSrcSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_NO_OVERLAY,                                        0}},
    {0xB7, 1, X86InstInfo{"MOVZX",   TYPE_INST, GenFlagsSrcSize(SIZE_16BIT) | FLAGS_MODRM | FLAGS_NO_OVERLAY,                                       0}},
    {0xB8, 1, X86InstInfo{"",        TYPE_INVALID, FLAGS_NONE,                                                                                      0}},
    {0xB9, 1, X86InstInfo{"",        TYPE_GROUP_10, FLAGS_NO_OVERLAY,                                                                               0}},
    {0xBA, 1, X86InstInfo{"",        TYPE_GROUP_8, FLAGS_NO_OVERLAY,                                                                                0}},
    {0xBB, 1, X86InstInfo{"BTC",     TYPE_INST, FLAGS_DEBUG_MEM_ACCESS | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_NO_OVERLAY,                         0}},
    {0xBC, 1, X86InstInfo{"BSF",     TYPE_INST, FLAGS_MODRM | FLAGS_NO_OVERLAY66,                                                                   0}},
    {0xBD, 1, X86InstInfo{"BSR",     TYPE_INST, FLAGS_MODRM | FLAGS_NO_OVERLAY66,                                                                   0}},
    {0xBE, 1, X86InstInfo{"MOVSX",   TYPE_INST, GenFlagsSrcSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_NO_OVERLAY,                                        0}},
    {0xBF, 1, X86InstInfo{"MOVSX",   TYPE_INST, GenFlagsSrcSize(SIZE_16BIT) | FLAGS_MODRM | FLAGS_NO_OVERLAY,                                       0}},

    {0xC0, 1, X86InstInfo{"XADD",    TYPE_INST, GenFlagsSameSize(SIZE_8BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST,                                                       0}},
    {0xC1, 1, X86InstInfo{"XADD",    TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_NO_OVERLAY,                                                                 0}},
    {0xC2, 1, X86InstInfo{"CMPPS",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                     1}},
    {0xC3, 1, X86InstInfo{"MOVNTI",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY | FLAGS_SF_MOD_DST,                                                            0}},
    {0xC4, 1, X86InstInfo{"PINSRW",  TYPE_INST, GenFlagsSizes(SIZE_64BIT, SIZE_16BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX | FLAGS_SF_SRC_GPR,           1}},
    {0xC5, 1, X86InstInfo{"PEXTRW",  TYPE_INST, GenFlagsSizes(SIZE_32BIT, SIZE_64BIT) | FLAGS_MODRM | FLAGS_SF_MOD_REG_ONLY | FLAGS_SF_DST_GPR | FLAGS_XMM_FLAGS | FLAGS_SF_MMX, 1}},
    {0xC6, 1, X86InstInfo{"SHUFPS",  TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                                                     1}},
    {0xC7, 1, X86InstInfo{"",        TYPE_GROUP_9, FLAGS_NO_OVERLAY,                                                                                               0}},
    {0xC8, 8, X86InstInfo{"BSWAP",   TYPE_INST, FLAGS_SF_REX_IN_BYTE | FLAGS_NO_OVERLAY,                                                                           0}},

    {0xD0, 1, X86InstInfo{"",         TYPE_INVALID, FLAGS_NONE,                                                                                         0}},
    {0xD1, 1, X86InstInfo{"PSRLW",    TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX,                                      0}},
    {0xD2, 1, X86InstInfo{"PSRLD",    TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX,                                      0}},
    {0xD3, 1, X86InstInfo{"PSRLQ",    TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX,                                      0}},
    {0xD4, 1, X86InstInfo{"PADDQ",    TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX,                                      0}},
    {0xD5, 1, X86InstInfo{"PMULLW",   TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX,                                      0}},
    {0xD6, 1, X86InstInfo{"",         TYPE_INVALID, FLAGS_NONE,                                                                                         0}},
    {0xD7, 1, X86InstInfo{"PMOVMSKB", TYPE_INST, GenFlagsSizes(SIZE_32BIT, SIZE_64BIT) | FLAGS_MODRM | FLAGS_SF_MOD_REG_ONLY | FLAGS_XMM_FLAGS | FLAGS_SF_DST_GPR | FLAGS_SF_MMX_SRC,                                  0}},
    {0xD8, 1, X86InstInfo{"PSUBUSB",  TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX,                                      0}},
    {0xD9, 1, X86InstInfo{"PSUBUSW",  TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX,                                      0}},
    {0xDA, 1, X86InstInfo{"PMINUB",   TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX,                                      0}},
    {0xDB, 1, X86InstInfo{"PAND",     TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX,                                      0}},
    {0xDC, 1, X86InstInfo{"PADDUSB",  TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX,                                      0}},
    {0xDD, 1, X86InstInfo{"PADDUSW",  TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX,                                      0}},
    {0xDE, 1, X86InstInfo{"PMAXUB",   TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX,                                      0}},
    {0xDF, 1, X86InstInfo{"PANDN",    TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX,                                      0}},

    {0xE0, 1, X86InstInfo{"PAVGB",    TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX,                                      0}},
    {0xE1, 1, X86InstInfo{"PSRAW",    TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX,                                      0}},
    {0xE2, 1, X86InstInfo{"PSRAD",    TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX,                                      0}},
    {0xE3, 1, X86InstInfo{"PAVGW",    TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX,                                       0}},
    {0xE4, 1, X86InstInfo{"PMULHUW",  TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX,                                      0}},
    {0xE5, 1, X86InstInfo{"PMULHW",   TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX,                                      0}},
    {0xE6, 1, X86InstInfo{"",         TYPE_INVALID, FLAGS_NONE,                                                                                         0}},
    {0xE7, 1, X86InstInfo{"MOVNTQ",   TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS | FLAGS_SF_MMX,                   0}},
    {0xE8, 1, X86InstInfo{"PSUBSB",   TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX,                                      0}},
    {0xE9, 1, X86InstInfo{"PSUBSW",   TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX,                                      0}},
    {0xEA, 1, X86InstInfo{"PMINSW",   TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX,                                      0}},
    {0xEB, 1, X86InstInfo{"POR",      TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX,                                      0}},
    {0xEC, 1, X86InstInfo{"PADDSB",   TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX,                                      0}},
    {0xED, 1, X86InstInfo{"PADDSW",   TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX,                                      0}},
    {0xEE, 1, X86InstInfo{"PMAXSW",   TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX,                                      0}},
    {0xEF, 1, X86InstInfo{"PXOR",     TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX,                                      0}},

    {0xF0, 1, X86InstInfo{"",         TYPE_INVALID, FLAGS_NONE,                                                                                         0}},
    {0xF1, 1, X86InstInfo{"PSLLW",    TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX,                                      0}},
    {0xF2, 1, X86InstInfo{"PSLLD",    TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX,                                      0}},
    {0xF3, 1, X86InstInfo{"PSLLQ",    TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX,                                      0}},
    {0xF4, 1, X86InstInfo{"PMULUDQ",  TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX,                                      0}},
    {0xF5, 1, X86InstInfo{"PMADDWD",  TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX,                                      0}},
    {0xF6, 1, X86InstInfo{"PSADBW",   TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX,                                      0}},
    {0xF7, 1, X86InstInfo{"MASKMOVQ", TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_SF_MOD_REG_ONLY | FLAGS_XMM_FLAGS | FLAGS_SF_MMX,                                      0}},
    {0xF8, 1, X86InstInfo{"PSUBB",    TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX,                                      0}},
    {0xF9, 1, X86InstInfo{"PSUBW",    TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX,                                      0}},
    {0xFA, 1, X86InstInfo{"PSUBD",    TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX,                                      0}},
    {0xFB, 1, X86InstInfo{"PSUBQ",    TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX,                                      0}},
    {0xFC, 1, X86InstInfo{"PADDB",    TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX,                                      0}},
    {0xFD, 1, X86InstInfo{"PADDW",    TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX,                                      0}},
    {0xFE, 1, X86InstInfo{"PADDD",    TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX,                                      0}},
    {0xFF, 1, X86InstInfo{"UD0",      TYPE_INST, FLAGS_BLOCK_END,                                                                                           0}},

#ifndef _WIN32
    // FEX reserved instructions
    // Unused x86 encoding instruction.

    {0x3E, 1, X86InstInfo{"CALLBACKRET",  TYPE_INST, FLAGS_BLOCK_END | FLAGS_NO_OVERLAY | FLAGS_SETS_RIP,                                                                          0}},

    // This was originally used by VIA to jump to its alternative instruction set. Used for OP_THUNK
    {0x3F, 1, X86InstInfo{"ALTINST",      TYPE_INST, FLAGS_BLOCK_END | FLAGS_NO_OVERLAY | FLAGS_SETS_RIP,                                                            0}},
#endif
  };

  GenerateTable(Table.data(), TwoByteOpTable, std::size(TwoByteOpTable));

  IR::InstallToTable(Table, IR::OpDispatch_TwoByteOpTable);

  return Table;
}();

constexpr std::array<X86InstInfo, MAX_REP_MOD_TABLE_SIZE> RepModOps = []() consteval {
  std::array<X86InstInfo, MAX_REP_MOD_TABLE_SIZE> Table{};

  constexpr U8U8InfoStruct RepModOpTable[] = {
    {0x0, 16, X86InstInfo{"",          TYPE_COPY_OTHER, FLAGS_NONE,                                     0}},

    {0x10, 1, X86InstInfo{"MOVSS",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                    0}},
    {0x11, 1, X86InstInfo{"MOVSS",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS,                    0}},
    {0x12, 1, X86InstInfo{"MOVSLDUP",  TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,  0}},
    {0x13, 3, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                        0}},
    {0x16, 1, X86InstInfo{"MOVSHDUP",  TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,  0}},
    {0x17, 2, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                        0}},
    {0x19, 7, X86InstInfo{"",          TYPE_COPY_OTHER, FLAGS_NONE,                                     0}},

    {0x20, 4, X86InstInfo{"",          TYPE_COPY_OTHER, FLAGS_NONE,                                     0}},
    {0x24, 6, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                        0}},
    {0x2A, 1, X86InstInfo{"CVTSI2SS",  TYPE_INST, GenFlagsDstSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_SRC_GPR, 0}},
    {0x2B, 1, X86InstInfo{"MOVNTSS",   TYPE_INST, GenFlagsSizes(SIZE_32BIT, SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS, 0}},
    {0x2C, 1, X86InstInfo{"CVTTSS2SI", TYPE_INST, FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_DST_GPR, 0}},
    {0x2D, 1, X86InstInfo{"CVTSS2SI",  TYPE_INST, FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_DST_GPR, 0}},
    {0x2E, 2, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                        0}},

    {0x30, 16, X86InstInfo{"",         TYPE_COPY_OTHER, FLAGS_NONE,                                     0}},
    {0x40, 16, X86InstInfo{"",         TYPE_INVALID, FLAGS_NONE,                                        0}},

    {0x50, 1, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                        0}},
    {0x51, 1, X86InstInfo{"SQRTSS",    TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_32BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                    0}},
    {0x52, 1, X86InstInfo{"RSQRTSS",   TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_32BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                    0}},
    {0x53, 1, X86InstInfo{"RCPSS",     TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_32BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                    0}},
    {0x54, 4, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                        0}},
    {0x58, 1, X86InstInfo{"ADDSS",     TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_32BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                    0}},
    {0x59, 1, X86InstInfo{"MULSS",     TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_32BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                    0}},
    {0x5A, 1, X86InstInfo{"CVTSS2SD",  TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,   0}},
    {0x5B, 1, X86InstInfo{"CVTTPS2DQ", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,  0}},
    {0x5C, 1, X86InstInfo{"SUBSS",     TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_32BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                    0}},
    {0x5D, 1, X86InstInfo{"MINSS",     TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_32BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                    0}},
    {0x5E, 1, X86InstInfo{"DIVSS",     TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_32BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                    0}},
    {0x5F, 1, X86InstInfo{"MAXSS",     TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_32BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                    0}},

    {0x60, 8, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                        0}},
    {0x68, 7, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                        0}},
    {0x6F, 1, X86InstInfo{"MOVDQU",    TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,  0}},

    {0x70, 1, X86InstInfo{"PSHUFHW",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,  1}},
    {0x71, 3, X86InstInfo{"",          TYPE_COPY_OTHER, FLAGS_NONE,                                     0}},
    {0x74, 4, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                        0}},
    {0x78, 6, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                        0}},
    {0x7E, 1, X86InstInfo{"MOVQ",      TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,   0}},
    {0x7F, 1, X86InstInfo{"MOVDQU",    TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS,  0}},

    {0x80, 16, X86InstInfo{"",         TYPE_COPY_OTHER, FLAGS_NONE,                                     0}},
    {0x90, 16, X86InstInfo{"",         TYPE_COPY_OTHER, FLAGS_NONE,                                     0}},
    {0xA0, 16, X86InstInfo{"",         TYPE_COPY_OTHER, FLAGS_NONE,                                     0}},

    {0xB0, 8, X86InstInfo{"",          TYPE_COPY_OTHER, FLAGS_NONE,                                     0}},
    {0xB8, 1, X86InstInfo{"POPCNT",    TYPE_INST, FLAGS_MODRM,                                      0}},
    {0xB9, 1, X86InstInfo{"",          TYPE_COPY_OTHER, FLAGS_NONE,                                        0}},
    {0xBA, 1, X86InstInfo{"",          TYPE_COPY_OTHER, FLAGS_NONE,                                        0}},
    {0xBB, 1, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                        0}},
    {0xBC, 1, X86InstInfo{"TZCNT",     TYPE_INST, FLAGS_MODRM,                                      0}},
    {0xBD, 1, X86InstInfo{"LZCNT",     TYPE_INST, FLAGS_MODRM,                                      0}},
    {0xBE, 2, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                        0}},

    {0xC0, 2, X86InstInfo{"",          TYPE_COPY_OTHER, FLAGS_NONE,                                     0}},
    {0xC2, 1, X86InstInfo{"CMPSS",     TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_32BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                    1}},
    {0xC3, 5, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                        0}},
    {0xC8, 8, X86InstInfo{"",          TYPE_COPY_OTHER, FLAGS_NONE,                                     0}},

    {0xD0, 6, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                        0}},
    {0xD6, 1, X86InstInfo{"MOVQ2DQ",   TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX_SRC, 0}},
    {0xD7, 1, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                        0}},
    {0xD8, 8, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                        0}},

    {0xE0, 6, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                        0}},
    {0xE6, 1, X86InstInfo{"CVTDQ2PD",  TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,  0}},
    {0xE7, 1, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                        0}},
    {0xE8, 8, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                        0}},

    {0xF0, 8, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                        0}},
    {0xF8, 7, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                        0}},
    {0xFF, 1, X86InstInfo{"",          TYPE_COPY_OTHER, FLAGS_NONE,                                     0}},
  };

  GenerateTableWithCopy(Table.data(), RepModOpTable, std::size(RepModOpTable), SecondBaseOps.data());

  IR::InstallToTable(Table, IR::OpDispatch_SecondaryRepModTables);
  return Table;
}();

constexpr std::array<X86InstInfo, MAX_REPNE_MOD_TABLE_SIZE> RepNEModOps = []() consteval {
  std::array<X86InstInfo, MAX_REPNE_MOD_TABLE_SIZE> Table{};

  constexpr U8U8InfoStruct RepNEModOpTable[] = {
    {0x0, 16, X86InstInfo{"",           TYPE_COPY_OTHER, FLAGS_NONE,                                                     0}},

    {0x10, 1, X86InstInfo{"MOVSD",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                  0}},
    {0x11, 1, X86InstInfo{"MOVSD",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS,                  0}},
    {0x12, 1, X86InstInfo{"MOVDDUP",    TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                  0}},
    {0x13, 6, X86InstInfo{"",           TYPE_INVALID, FLAGS_NONE,                                                        0}},
    {0x19, 7, X86InstInfo{"",           TYPE_COPY_OTHER, FLAGS_NONE,                                                     0}},

    {0x20, 4, X86InstInfo{"",          TYPE_COPY_OTHER, FLAGS_NONE,                                                      0}},
    {0x24, 6, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                                         0}},
    {0x2A, 1, X86InstInfo{"CVTSI2SD",  TYPE_INST, GenFlagsDstSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_SRC_GPR,                    0}},
    {0x2B, 1, X86InstInfo{"MOVNTSD",   TYPE_INST, GenFlagsSizes(SIZE_64BIT, SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS, 0}},
    {0x2C, 1, X86InstInfo{"CVTTSD2SI", TYPE_INST, FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_DST_GPR, 0}},
    {0x2D, 1, X86InstInfo{"CVTSD2SI",  TYPE_INST, FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_DST_GPR, 0}},
    {0x2E, 2, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                                         0}},

    {0x30, 16, X86InstInfo{"",         TYPE_COPY_OTHER, FLAGS_NONE,                                                      0}},
    {0x40, 16, X86InstInfo{"",         TYPE_INVALID, FLAGS_NONE,                                                         0}},

    {0x50, 1, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                                         0}},
    {0x51, 1, X86InstInfo{"SQRTSD",    TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS, 0}},
    {0x52, 6, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                                         0}},
    {0x58, 1, X86InstInfo{"ADDSD",     TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                    0}},
    {0x59, 1, X86InstInfo{"MULSD",     TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                    0}},
    {0x5A, 1, X86InstInfo{"CVTSD2SS",  TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                    0}},
    {0x5B, 1, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                                         0}},
    {0x5C, 1, X86InstInfo{"SUBSD",     TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                    0}},
    {0x5D, 1, X86InstInfo{"MINSD",     TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                    0}},
    {0x5E, 1, X86InstInfo{"DIVSD",     TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                    0}},
    {0x5F, 1, X86InstInfo{"MAXSD",     TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                    0}},

    {0x60, 16, X86InstInfo{"",         TYPE_INVALID, FLAGS_NONE,                                                         0}},

    {0x70, 1, X86InstInfo{"PSHUFLW",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                   1}},
    {0x71, 3, X86InstInfo{"",          TYPE_COPY_OTHER, FLAGS_NONE,                                                      0}},
    {0x74, 4, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                                         0}},
    {0x78, 1, X86InstInfo{"INSERTQ",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_REG_ONLY | FLAGS_XMM_FLAGS,2}},
    {0x79, 1, X86InstInfo{"INSERTQ",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_REG_ONLY | FLAGS_XMM_FLAGS, 0}},
    {0x7A, 2, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                                         0}},
    {0x7C, 1, X86InstInfo{"HADDPS",    TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                   0}},
    {0x7D, 1, X86InstInfo{"HSUBPS",    TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                   0}},
    {0x7E, 2, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                                         0}},

    {0x80, 16, X86InstInfo{"",         TYPE_COPY_OTHER, FLAGS_NONE,                                                      0}},
    {0x90, 16, X86InstInfo{"",         TYPE_COPY_OTHER, FLAGS_NONE,                                                      0}},
    {0xA0, 16, X86InstInfo{"",         TYPE_COPY_OTHER, FLAGS_NONE,                                                      0}},
    {0xB0, 8,  X86InstInfo{"",         TYPE_COPY_OTHER, FLAGS_NONE,                                                      0}},
    {0xB8, 1, X86InstInfo{"",           TYPE_INVALID, FLAGS_NONE,                                                               0}},
    {0xB9, 1, X86InstInfo{"",          TYPE_COPY_OTHER, FLAGS_NONE,                                        0}},
    {0xBA, 1, X86InstInfo{"",          TYPE_COPY_OTHER, FLAGS_NONE,                                        0}},
    {0xBB, 5,  X86InstInfo{"",         TYPE_INVALID, FLAGS_NONE,                                                         0}},
    {0xC0, 2, X86InstInfo{"",          TYPE_COPY_OTHER, FLAGS_NONE,                                                      0}},
    {0xC2, 1, X86InstInfo{"CMPSD",     TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                    1}},
    {0xC3, 5, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                                         0}},
    {0xC8, 8, X86InstInfo{"",          TYPE_COPY_OTHER, FLAGS_NONE,                                                      0}},

    {0xD0, 1, X86InstInfo{"ADDSUBPS",  TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                   0}},
    {0xD1, 5, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                                         0}},
    {0xD6, 1, X86InstInfo{"MOVDQ2Q",   TYPE_INST, GenFlagsSizes(SIZE_64BIT, SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX_DST,     0}},
    {0xD7, 1, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                                         0}},
    {0xD8, 8, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                                         0}},

    {0xE0, 6, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                                         0}},
    {0xE6, 1, X86InstInfo{"CVTPD2DQ",  TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                   0}},
    {0xE7, 1, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                                         0}},
    {0xE8, 8, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                                         0}},

    {0xF0, 1, X86InstInfo{"LDDQU",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY | FLAGS_XMM_FLAGS,0}},
    {0xF1, 7, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                                         0}},
    {0xF8, 8, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                                         0}},
  };

  GenerateTableWithCopy(Table.data(), RepNEModOpTable,   std::size(RepNEModOpTable), SecondBaseOps.data());

  IR::InstallToTable(Table, IR::OpDispatch_SecondaryRepNEModTables);
  return Table;
}();

constexpr std::array<X86InstInfo, MAX_OPSIZE_MOD_TABLE_SIZE> OpSizeModOps = []() consteval {
  std::array<X86InstInfo, MAX_OPSIZE_MOD_TABLE_SIZE> Table{};

  constexpr U8U8InfoStruct OpSizeModOpTable[] = {
    {0x0, 16, X86InstInfo{"",           TYPE_COPY_OTHER, FLAGS_NONE,                                                            0}},

    {0x10, 1, X86InstInfo{"MOVUPD",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0x11, 1, X86InstInfo{"MOVUPD",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS,                         0}},
    {0x12, 1, X86InstInfo{"MOVLPD",     TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_64BIT) | FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY | FLAGS_XMM_FLAGS,      0}},
    {0x13, 1, X86InstInfo{"MOVLPD",     TYPE_INST, GenFlagsSizes(SIZE_64BIT, SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS,      0}},
    {0x14, 1, X86InstInfo{"UNPCKLPD",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0x15, 1, X86InstInfo{"UNPCKHPD",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0x16, 1, X86InstInfo{"MOVHPD",     TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_64BIT) | FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY | FLAGS_XMM_FLAGS,      0}},
    {0x17, 1, X86InstInfo{"MOVHPD",     TYPE_INST, GenFlagsSizes(SIZE_64BIT, SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS,      0}},
    {0x18, 1, X86InstInfo{"",           TYPE_INVALID, FLAGS_NONE,                                                               0}},
    {0x19, 7, X86InstInfo{"",           TYPE_COPY_OTHER, FLAGS_NONE,                                                            0}},

    {0x20, 4, X86InstInfo{"",           TYPE_COPY_OTHER, FLAGS_NONE,                                                            0}},
    {0x24, 4, X86InstInfo{"",           TYPE_INVALID, FLAGS_NONE,                                                               0}},

    {0x28, 1, X86InstInfo{"MOVAPD",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0x29, 1, X86InstInfo{"MOVAPD",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS,                         0}},
    {0x2A, 1, X86InstInfo{"CVTPI2PD",   TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX_SRC,                                                                   0}},
    {0x2B, 1, X86InstInfo{"MOVNTPD",    TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS,      0}},
    {0x2C, 1, X86InstInfo{"CVTTPD2PI",  TYPE_INST, GenFlagsSizes(SIZE_64BIT, SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX_DST,                          0}},
    {0x2D, 1, X86InstInfo{"CVTPD2PI",   TYPE_INST, GenFlagsSizes(SIZE_64BIT, SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX_DST,                          0}},
    {0x2E, 1, X86InstInfo{"UCOMISD",    TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                          0}},
    {0x2F, 1, X86InstInfo{"COMISD",     TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                          0}},

    {0x30, 16, X86InstInfo{"",          TYPE_COPY_OTHER, FLAGS_NONE,                                                            0}},
    {0x40, 16, X86InstInfo{"",          TYPE_COPY_OTHER, FLAGS_NONE,                                                            0}},

    {0x50, 1, X86InstInfo{"MOVMSKPD",   TYPE_INST, GenFlagsSizes(SIZE_32BIT, SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_REG_ONLY | FLAGS_XMM_FLAGS | FLAGS_SF_DST_GPR,     0}},
    {0x51, 1, X86InstInfo{"SQRTPD",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0x52, 2, X86InstInfo{"",           TYPE_INVALID, FLAGS_NONE,                                                               0}},
    {0x54, 1, X86InstInfo{"ANDPD",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                        0}},
    {0x55, 1, X86InstInfo{"ANDNPD",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0x56, 1, X86InstInfo{"ORPD",       TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0x57, 1, X86InstInfo{"XORPD",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0x58, 1, X86InstInfo{"ADDPD",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0x59, 1, X86InstInfo{"MULPD",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0x5A, 1, X86InstInfo{"CVTPD2PS",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0x5B, 1, X86InstInfo{"CVTPS2DQ",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0x5C, 1, X86InstInfo{"SUBPD",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0x5D, 1, X86InstInfo{"MINPD",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0x5E, 1, X86InstInfo{"DIVPD",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0x5F, 1, X86InstInfo{"MAXPD",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},

    {0x60, 1, X86InstInfo{"PUNPCKLBW",  TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0x61, 1, X86InstInfo{"PUNPCKLWD",  TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0x62, 1, X86InstInfo{"PUNPCKLDQ",  TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0x63, 1, X86InstInfo{"PACKSSWB",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0x64, 1, X86InstInfo{"PCMPGTB",    TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0x65, 1, X86InstInfo{"PCMPGTW",    TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0x66, 1, X86InstInfo{"PCMPGTD",    TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0x67, 1, X86InstInfo{"PACKUSWB",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0x68, 1, X86InstInfo{"PUNPCKHBW",  TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0x69, 1, X86InstInfo{"PUNPCKHWD",  TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0x6A, 1, X86InstInfo{"PUNPCKHDQ",  TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0x6B, 1, X86InstInfo{"PACKSSDW",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0x6C, 1, X86InstInfo{"PUNPCKLQDQ", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0x6D, 1, X86InstInfo{"PUNPCKHQDQ", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0x6E, 1, X86InstInfo{"MOVD",       TYPE_INST, GenFlagsDstSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_SRC_GPR | FLAGS_XMM_FLAGS,      0}},
    {0x6F, 1, X86InstInfo{"MOVDQA",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},

    {0x70, 1, X86InstInfo{"PSHUFD",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         1}},
    {0x71, 3, X86InstInfo{"",           TYPE_COPY_OTHER, FLAGS_NONE,                                                            0}},
    {0x74, 1, X86InstInfo{"PCMPEQB",    TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0x75, 1, X86InstInfo{"PCMPEQW",    TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0x76, 1, X86InstInfo{"PCMPEQD",    TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0x77, 1, X86InstInfo{"",           TYPE_INVALID, FLAGS_NONE,                                                               0}},
    {0x78, 1, X86InstInfo{"",           TYPE_GROUP_17, FLAGS_NONE,                                                              0}},

    {0x79, 1, X86InstInfo{"EXTRQ",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_REG_ONLY | FLAGS_XMM_FLAGS,                         0}},
    {0x7A, 2, X86InstInfo{"",           TYPE_INVALID, FLAGS_NONE,                                                               0}},
    {0x7C, 1, X86InstInfo{"HADDPD",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0x7D, 1, X86InstInfo{"HSUBPD",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0x7E, 1, X86InstInfo{"MOVD",       TYPE_INST, GenFlagsSrcSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_DST_GPR | FLAGS_XMM_FLAGS,      0}},
    {0x7F, 1, X86InstInfo{"MOVDQA",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS,                         0}},

    {0x80, 16, X86InstInfo{"",          TYPE_COPY_OTHER, FLAGS_NONE,                                                            0}},
    {0x90, 16, X86InstInfo{"",          TYPE_COPY_OTHER, FLAGS_NONE,                                                            0}},
    {0xA0, 16, X86InstInfo{"",          TYPE_COPY_OTHER, FLAGS_NONE,                                                            0}},
    {0xB0, 8, X86InstInfo{"",           TYPE_COPY_OTHER, FLAGS_NONE,                                                            0}},
    {0xB8, 1, X86InstInfo{"",           TYPE_INVALID, FLAGS_NONE,                                                               0}},
    {0xB9, 1, X86InstInfo{"",          TYPE_COPY_OTHER, FLAGS_NONE,                                        0}},
    {0xBA, 1, X86InstInfo{"",          TYPE_COPY_OTHER, FLAGS_NONE,                                        0}},
    {0xBB, 5, X86InstInfo{"",          TYPE_INVALID, FLAGS_NONE,                                        0}},

    {0xC0, 2, X86InstInfo{"",           TYPE_COPY_OTHER, FLAGS_NONE,                                                            0}},
    {0xC2, 1, X86InstInfo{"CMPPD",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         1}},
    {0xC3, 1, X86InstInfo{"",           TYPE_INVALID, FLAGS_NONE,                                                               0}},
    {0xC4, 1, X86InstInfo{"PINSRW",     TYPE_INST, GenFlagsSizes(SIZE_128BIT, SIZE_16BIT) | FLAGS_MODRM | FLAGS_SF_SRC_GPR | FLAGS_XMM_FLAGS,      1}},
    {0xC5, 1, X86InstInfo{"PEXTRW",     TYPE_INST, GenFlagsSizes(SIZE_32BIT, SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_REG_ONLY | FLAGS_SF_DST_GPR | FLAGS_XMM_FLAGS,      1}},
    {0xC6, 1, X86InstInfo{"SHUFPD",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         1}},
    {0xC7, 1, X86InstInfo{"",           TYPE_INVALID, FLAGS_NONE,                                                               0}},
    {0xC8, 8, X86InstInfo{"",           TYPE_COPY_OTHER, FLAGS_NONE,                                                            0}},

    {0xD0, 1, X86InstInfo{"ADDSUBPD",   TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0xD1, 1, X86InstInfo{"PSRLW",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0xD2, 1, X86InstInfo{"PSRLD",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0xD3, 1, X86InstInfo{"PSRLQ",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0xD4, 1, X86InstInfo{"PADDQ",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0xD5, 1, X86InstInfo{"PMULLW",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0xD6, 1, X86InstInfo{"MOVQ",       TYPE_INST, GenFlagsSizes(SIZE_64BIT, SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS,       0}},
    {0xD7, 1, X86InstInfo{"PMOVMSKB",   TYPE_INST, GenFlagsSizes(SIZE_32BIT, SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_REG_ONLY | FLAGS_XMM_FLAGS | FLAGS_SF_DST_GPR,      0}},
    {0xD8, 1, X86InstInfo{"PSUBUSB",    TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0xD9, 1, X86InstInfo{"PSUBUSW",    TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0xDA, 1, X86InstInfo{"PMINUB",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0xDB, 1, X86InstInfo{"PAND",       TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0xDC, 1, X86InstInfo{"PADDUSB",    TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0xDD, 1, X86InstInfo{"PADDUSW",    TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0xDE, 1, X86InstInfo{"PMAXUB",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0xDF, 1, X86InstInfo{"PANDN",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},

    {0xE0, 1, X86InstInfo{"PAVGB",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0xE1, 1, X86InstInfo{"PSRAW",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0xE2, 1, X86InstInfo{"PSRAD",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0xE3, 1, X86InstInfo{"PAVGW",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0xE4, 1, X86InstInfo{"PMULHUW",    TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0xE5, 1, X86InstInfo{"PMULHW",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0xE6, 1, X86InstInfo{"CVTTPD2DQ",  TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0xE7, 1, X86InstInfo{"MOVNTDQ",    TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS,      0}},
    {0xE8, 1, X86InstInfo{"PSUBSB",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0xE9, 1, X86InstInfo{"PSUBSW",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0xEA, 1, X86InstInfo{"PMINSW",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0xEB, 1, X86InstInfo{"POR",        TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0xEC, 1, X86InstInfo{"PADDSB",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0xED, 1, X86InstInfo{"PADDSW",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0xEE, 1, X86InstInfo{"PMAXSW",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0xEF, 1, X86InstInfo{"PXOR",       TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},

    {0xF0, 1, X86InstInfo{"",           TYPE_INVALID, FLAGS_NONE,                                                               0}},
    {0xF1, 1, X86InstInfo{"PSLLW",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0xF2, 1, X86InstInfo{"PSLLD",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0xF3, 1, X86InstInfo{"PSLLQ",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0xF4, 1, X86InstInfo{"PMULUDQ",    TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0xF5, 1, X86InstInfo{"PMADDWD",    TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0xF6, 1, X86InstInfo{"PSADBW",     TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0xF7, 1, X86InstInfo{"MASKMOVDQU", TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_REG_ONLY | FLAGS_XMM_FLAGS,                         0}},
    {0xF8, 1, X86InstInfo{"PSUBB",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0xF9, 1, X86InstInfo{"PSUBW",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0xFA, 1, X86InstInfo{"PSUBD",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0xFB, 1, X86InstInfo{"PSUBQ",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0xFC, 1, X86InstInfo{"PADDB",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0xFD, 1, X86InstInfo{"PADDW",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,                         0}},
    {0xFE, 1, X86InstInfo{"PADDD",      TYPE_INST, GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS,       0}},
    {0xFF, 1, X86InstInfo{"",           TYPE_COPY_OTHER, FLAGS_NONE,                                                            0}},
  };

  GenerateTableWithCopy(Table.data(), OpSizeModOpTable, std::size(OpSizeModOpTable), SecondBaseOps.data());

  IR::InstallToTable(Table, IR::OpDispatch_SecondaryOpSizeModTables);
  return Table;
}();
}
