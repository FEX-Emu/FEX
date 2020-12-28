#include "Interface/Core/X86Tables/X86Tables.h"

namespace FEXCore::X86Tables {
using namespace InstFlags;

void InitializeSecondaryGroupTables() {
#define OPD(group, prefix, Reg) (((group - FEXCore::X86Tables::TYPE_GROUP_6) << 5) | (prefix) << 3 | (Reg))
  constexpr uint16_t PF_NONE = 0;
  constexpr uint16_t PF_F3   = 1;
  constexpr uint16_t PF_66   = 2;
  constexpr uint16_t PF_F2   = 3;

  const U16U8InfoStruct SecondaryExtensionOpTable[] = {
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
    {OPD(TYPE_GROUP_12, PF_NONE, 2), 1, X86InstInfo{"PSRLW", TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS | FLAGS_SF_MMX, 1, nullptr}},
    {OPD(TYPE_GROUP_12, PF_NONE, 3), 1, X86InstInfo{"",      TYPE_INVALID, FLAGS_NONE,                                      0, nullptr}},
    {OPD(TYPE_GROUP_12, PF_NONE, 4), 1, X86InstInfo{"PSRAW", TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS | FLAGS_SF_MMX, 1, nullptr}},
    {OPD(TYPE_GROUP_12, PF_NONE, 5), 1, X86InstInfo{"",      TYPE_INVALID, FLAGS_NONE,                                      0, nullptr}},
    {OPD(TYPE_GROUP_12, PF_NONE, 6), 1, X86InstInfo{"PSLLW", TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS | FLAGS_SF_MMX, 1, nullptr}},
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
    {OPD(TYPE_GROUP_13, PF_NONE, 2), 1, X86InstInfo{"PSRLD", TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS | FLAGS_SF_MMX, 1, nullptr}},
    {OPD(TYPE_GROUP_13, PF_NONE, 3), 1, X86InstInfo{"",      TYPE_INVALID, FLAGS_NONE,                                      0, nullptr}},
    {OPD(TYPE_GROUP_13, PF_NONE, 4), 1, X86InstInfo{"PSRAD", TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS | FLAGS_SF_MMX, 1, nullptr}},
    {OPD(TYPE_GROUP_13, PF_NONE, 5), 1, X86InstInfo{"",      TYPE_INVALID, FLAGS_NONE,                                      0, nullptr}},
    {OPD(TYPE_GROUP_13, PF_NONE, 6), 1, X86InstInfo{"PSLLD", TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS | FLAGS_SF_MMX, 1, nullptr}},
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
    {OPD(TYPE_GROUP_14, PF_NONE, 2), 1, X86InstInfo{"PSRLQ", TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS | FLAGS_SF_MMX, 1, nullptr}},
    {OPD(TYPE_GROUP_14, PF_NONE, 3), 1, X86InstInfo{"",      TYPE_INVALID, FLAGS_NONE,                                      0, nullptr}},
    {OPD(TYPE_GROUP_14, PF_NONE, 4), 1, X86InstInfo{"",      TYPE_INVALID, FLAGS_NONE,                                      0, nullptr}},
    {OPD(TYPE_GROUP_14, PF_NONE, 5), 1, X86InstInfo{"",      TYPE_INVALID, FLAGS_NONE,                                      0, nullptr}},
    {OPD(TYPE_GROUP_14, PF_NONE, 6), 1, X86InstInfo{"PSLLQ", TYPE_INST, GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_XMM_FLAGS | FLAGS_SF_MMX, 1, nullptr}},
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
    {OPD(TYPE_GROUP_15, PF_NONE, 0), 1, X86InstInfo{"FXSAVE",          TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,       0, nullptr}}, // MMX/x87
    {OPD(TYPE_GROUP_15, PF_NONE, 1), 1, X86InstInfo{"FXRSTOR",         TYPE_INST, FLAGS_MODRM,       0, nullptr}}, // MMX/x87
    {OPD(TYPE_GROUP_15, PF_NONE, 2), 1, X86InstInfo{"LDMXCSR",         TYPE_INST, GenFlagsSameSize(SIZE_32BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_MOD_MEM_ONLY, 0, nullptr}},
    {OPD(TYPE_GROUP_15, PF_NONE, 3), 1, X86InstInfo{"STMXCSR",         TYPE_INST, GenFlagsSameSize(SIZE_32BIT) | FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_MOD_MEM_ONLY, 0, nullptr}},
    {OPD(TYPE_GROUP_15, PF_NONE, 4), 1, X86InstInfo{"XSAVE",           TYPE_PRIV, FLAGS_NONE,      0, nullptr}},
    {OPD(TYPE_GROUP_15, PF_NONE, 5), 1, X86InstInfo{"LFENCE/XRSTOR",   TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,      0, nullptr}},
    {OPD(TYPE_GROUP_15, PF_NONE, 6), 1, X86InstInfo{"MFENCE/XSAVEOPT", TYPE_INST, FLAGS_MODRM,      0, nullptr}},
    {OPD(TYPE_GROUP_15, PF_NONE, 7), 1, X86InstInfo{"SFENCE/CLFLUSH",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST,      0, nullptr}},

    {OPD(TYPE_GROUP_15, PF_F3, 0), 1, X86InstInfo{"RDFSBASE", TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_MOD_REG_ONLY,          0, nullptr}},
    {OPD(TYPE_GROUP_15, PF_F3, 1), 1, X86InstInfo{"RDGSBASE", TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_MOD_REG_ONLY,          0, nullptr}},
    {OPD(TYPE_GROUP_15, PF_F3, 2), 1, X86InstInfo{"WRFSBASE", TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_REG_ONLY,          0, nullptr}},
    {OPD(TYPE_GROUP_15, PF_F3, 3), 1, X86InstInfo{"WRGSBASE", TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_REG_ONLY,          0, nullptr}},
    {OPD(TYPE_GROUP_15, PF_F3, 4), 1, X86InstInfo{"", TYPE_INVALID, FLAGS_NONE,                    0, nullptr}},
    {OPD(TYPE_GROUP_15, PF_F3, 5), 1, X86InstInfo{"INCSSPQ", TYPE_INST, FLAGS_MODRM,                    0, nullptr}},
    {OPD(TYPE_GROUP_15, PF_F3, 6), 1, X86InstInfo{"CLRSSBSY", TYPE_INST, FLAGS_NONE,                    0, nullptr}},
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

    {OPD(TYPE_GROUP_16, PF_F3, 0), 1, X86InstInfo{"PREFETCH NTA", TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_16, PF_F3, 1), 1, X86InstInfo{"PREFETCH T0",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_16, PF_F3, 2), 1, X86InstInfo{"PREFETCH T1",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_16, PF_F3, 3), 1, X86InstInfo{"PREFETCH T2",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_16, PF_F3, 4), 1, X86InstInfo{"NOP",          TYPE_INST, FLAGS_MODRM,   0, nullptr}},
    {OPD(TYPE_GROUP_16, PF_F3, 5), 1, X86InstInfo{"NOP",          TYPE_INST, FLAGS_MODRM,   0, nullptr}},
    {OPD(TYPE_GROUP_16, PF_F3, 6), 1, X86InstInfo{"NOP",          TYPE_INST, FLAGS_MODRM,   0, nullptr}},
    {OPD(TYPE_GROUP_16, PF_F3, 7), 1, X86InstInfo{"NOP",          TYPE_INST, FLAGS_MODRM,   0, nullptr}},

    {OPD(TYPE_GROUP_16, PF_66, 0), 1, X86InstInfo{"PREFETCH NTA", TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_16, PF_66, 1), 1, X86InstInfo{"PREFETCH T0",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_16, PF_66, 2), 1, X86InstInfo{"PREFETCH T1",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_16, PF_66, 3), 1, X86InstInfo{"PREFETCH T2",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_16, PF_66, 4), 1, X86InstInfo{"NOP",          TYPE_INST, FLAGS_MODRM,   0, nullptr}},
    {OPD(TYPE_GROUP_16, PF_66, 5), 1, X86InstInfo{"NOP",          TYPE_INST, FLAGS_MODRM,   0, nullptr}},
    {OPD(TYPE_GROUP_16, PF_66, 6), 1, X86InstInfo{"NOP",          TYPE_INST, FLAGS_MODRM,   0, nullptr}},
    {OPD(TYPE_GROUP_16, PF_66, 7), 1, X86InstInfo{"NOP",          TYPE_INST, FLAGS_MODRM,   0, nullptr}},

    {OPD(TYPE_GROUP_16, PF_F2, 0), 1, X86InstInfo{"PREFETCH NTA", TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_16, PF_F2, 1), 1, X86InstInfo{"PREFETCH T0",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_16, PF_F2, 2), 1, X86InstInfo{"PREFETCH T1",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_16, PF_F2, 3), 1, X86InstInfo{"PREFETCH T2",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_16, PF_F2, 4), 1, X86InstInfo{"NOP",          TYPE_INST, FLAGS_MODRM,   0, nullptr}},
    {OPD(TYPE_GROUP_16, PF_F2, 5), 1, X86InstInfo{"NOP",          TYPE_INST, FLAGS_MODRM,   0, nullptr}},
    {OPD(TYPE_GROUP_16, PF_F2, 6), 1, X86InstInfo{"NOP",          TYPE_INST, FLAGS_MODRM,   0, nullptr}},
    {OPD(TYPE_GROUP_16, PF_F2, 7), 1, X86InstInfo{"NOP",          TYPE_INST, FLAGS_MODRM,   0, nullptr}},

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
    {OPD(TYPE_GROUP_P, PF_NONE, 3), 1, X86InstInfo{"PREFETCH Mod", TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY, 0, nullptr}},
    {OPD(TYPE_GROUP_P, PF_NONE, 4), 1, X86InstInfo{"PREFETCH Res", TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY, 0, nullptr}},
    {OPD(TYPE_GROUP_P, PF_NONE, 5), 1, X86InstInfo{"PREFETCH Res", TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY, 0, nullptr}},
    {OPD(TYPE_GROUP_P, PF_NONE, 6), 1, X86InstInfo{"PREFETCH Res", TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY, 0, nullptr}},
    {OPD(TYPE_GROUP_P, PF_NONE, 7), 1, X86InstInfo{"PREFETCH Res", TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY, 0, nullptr}},

    {OPD(TYPE_GROUP_P, PF_F3, 0), 1, X86InstInfo{"PREFETCH Ex",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_P, PF_F3, 1), 1, X86InstInfo{"PREFETCH Mod", TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_P, PF_F3, 2), 1, X86InstInfo{"PREFETCH Res", TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_P, PF_F3, 3), 1, X86InstInfo{"PREFETCH Mod", TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_P, PF_F3, 4), 1, X86InstInfo{"PREFETCH Res", TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_P, PF_F3, 5), 1, X86InstInfo{"PREFETCH Res", TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_P, PF_F3, 6), 1, X86InstInfo{"PREFETCH Res", TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_P, PF_F3, 7), 1, X86InstInfo{"PREFETCH Res", TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},

    {OPD(TYPE_GROUP_P, PF_66, 0), 1, X86InstInfo{"PREFETCH Ex",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_P, PF_66, 1), 1, X86InstInfo{"PREFETCH Mod", TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_P, PF_66, 2), 1, X86InstInfo{"PREFETCH Res", TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_P, PF_66, 3), 1, X86InstInfo{"PREFETCH Mod", TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_P, PF_66, 4), 1, X86InstInfo{"PREFETCH Res", TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_P, PF_66, 5), 1, X86InstInfo{"PREFETCH Res", TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_P, PF_66, 6), 1, X86InstInfo{"PREFETCH Res", TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_P, PF_66, 7), 1, X86InstInfo{"PREFETCH Res", TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},

    {OPD(TYPE_GROUP_P, PF_F2, 0), 1, X86InstInfo{"PREFETCH Ex",  TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_P, PF_F2, 1), 1, X86InstInfo{"PREFETCH Mod", TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_P, PF_F2, 2), 1, X86InstInfo{"PREFETCH Res", TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_P, PF_F2, 3), 1, X86InstInfo{"PREFETCH Mod", TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_P, PF_F2, 4), 1, X86InstInfo{"PREFETCH Res", TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_P, PF_F2, 5), 1, X86InstInfo{"PREFETCH Res", TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_P, PF_F2, 6), 1, X86InstInfo{"PREFETCH Res", TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
    {OPD(TYPE_GROUP_P, PF_F2, 7), 1, X86InstInfo{"PREFETCH Res", TYPE_INST, FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY,   0, nullptr}},
  };
#undef OPD

  GenerateTable(SecondInstGroupOps, SecondaryExtensionOpTable, sizeof(SecondaryExtensionOpTable) / sizeof(SecondaryExtensionOpTable[0]));
}

}
