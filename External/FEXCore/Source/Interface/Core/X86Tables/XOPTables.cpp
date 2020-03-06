#include "Interface/Core/X86Tables/X86Tables.h"

namespace FEXCore::X86Tables {
using namespace InstFlags;

void InitializeXOPTables() {
#define OPD(group, pp, opcode) ( (group << 10) | (pp << 8) | (opcode))
  constexpr uint16_t XOP_GROUP_8 = 0;
  constexpr uint16_t XOP_GROUP_9 = 1;
  constexpr uint16_t XOP_GROUP_A = 2;

  const U16U8InfoStruct XOPTable[] = {
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
#undef OPD

#define OPD(subgroup, opcode)  (((subgroup - 1) << 3) | (opcode))
  const U8U8InfoStruct XOPGroupTable[] = {
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

  GenerateTable(XOPTableOps, XOPTable, sizeof(XOPTable) / sizeof(XOPTable[0]));
  GenerateTable(XOPTableGroupOps, XOPGroupTable, sizeof(XOPGroupTable) / sizeof(XOPGroupTable[0]));
}
}
