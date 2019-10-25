#include "Interface/Core/X86Tables/X86Tables.h"

namespace FEXCore::X86Tables {
using namespace InstFlags;

void InitializeDDDTables() {
  const U8U8InfoStruct DDDNowOpTable[] = {
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

  GenerateTable(DDDNowOps, DDDNowOpTable, sizeof(DDDNowOpTable) / sizeof(DDDNowOpTable[0]));
}
}
