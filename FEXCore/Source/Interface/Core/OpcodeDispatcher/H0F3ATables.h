// SPDX-License-Identifier: MIT
#pragma once
#include "Interface/Core/OpcodeDispatcher.h"

namespace FEXCore::IR {
constexpr inline void InstallToTable(auto& FinalTable, auto& LocalTable) {
  for (auto Op : LocalTable) {
    auto OpNum = std::get<0>(Op);
    auto Dispatcher = std::get<2>(Op);
    for (uint8_t i = 0; i < std::get<1>(Op); ++i) {
      auto& TableOp = FinalTable[OpNum + i];
      if (TableOp.OpcodeDispatcher) {
        LOGMAN_MSG_A_FMT("Duplicate Entry {} 0x{:x}", TableOp.Name, OpNum + i);
      }

      TableOp.OpcodeDispatcher = Dispatcher;
    }
  }
}

#define OPD(REX, prefix, opcode) ((REX << 9) | (prefix << 8) | opcode)
#define PF_3A_NONE 0
#define PF_3A_66 1
consteval inline void H0F3ATable_Install(auto& FinalTable) {
  constexpr std::tuple<uint16_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr> Table[] = {
    {OPD(0, PF_3A_66, 0x08), 1, &OpDispatchBuilder::VectorRound<4>},
    {OPD(0, PF_3A_66, 0x09), 1, &OpDispatchBuilder::VectorRound<8>},
    {OPD(0, PF_3A_66, 0x0A), 1, &OpDispatchBuilder::InsertScalarRound<4>},
    {OPD(0, PF_3A_66, 0x0B), 1, &OpDispatchBuilder::InsertScalarRound<8>},
    {OPD(0, PF_3A_66, 0x0C), 1, &OpDispatchBuilder::VectorBlend<4>},
    {OPD(0, PF_3A_66, 0x0D), 1, &OpDispatchBuilder::VectorBlend<8>},
    {OPD(0, PF_3A_66, 0x0E), 1, &OpDispatchBuilder::VectorBlend<2>},

    {OPD(0, PF_3A_NONE, 0x0F), 1, &OpDispatchBuilder::PAlignrOp},
    {OPD(0, PF_3A_66, 0x0F), 1, &OpDispatchBuilder::PAlignrOp},

    {OPD(0, PF_3A_66, 0x14), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::PExtrOp, 1>},
    {OPD(0, PF_3A_66, 0x15), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::PExtrOp, 2>},
    {OPD(0, PF_3A_66, 0x16), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::PExtrOp, 4>},
    {OPD(0, PF_3A_66, 0x17), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::PExtrOp, 4>},

    {OPD(0, PF_3A_66, 0x20), 1, &OpDispatchBuilder::PINSROp<1>},
    {OPD(0, PF_3A_66, 0x21), 1, &OpDispatchBuilder::InsertPSOp},
    {OPD(0, PF_3A_66, 0x22), 1, &OpDispatchBuilder::PINSROp<4>},
    {OPD(0, PF_3A_66, 0x40), 1, &OpDispatchBuilder::DPPOp<4>},
    {OPD(0, PF_3A_66, 0x41), 1, &OpDispatchBuilder::DPPOp<8>},
    {OPD(0, PF_3A_66, 0x42), 1, &OpDispatchBuilder::MPSADBWOp},

    {OPD(0, PF_3A_66, 0x60), 1, &OpDispatchBuilder::VPCMPESTRMOp},
    {OPD(0, PF_3A_66, 0x61), 1, &OpDispatchBuilder::VPCMPESTRIOp},
    {OPD(0, PF_3A_66, 0x62), 1, &OpDispatchBuilder::VPCMPISTRMOp},
    {OPD(0, PF_3A_66, 0x63), 1, &OpDispatchBuilder::VPCMPISTRIOp},

    {OPD(0, PF_3A_NONE, 0xCC), 1, &OpDispatchBuilder::SHA1RNDS4Op},
  };

  InstallToTable(FinalTable, Table);
}

inline void H0F3ATable_Install64(auto& FinalTable) {
  constexpr std::tuple<uint16_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr> Table[] = {
    {OPD(1, PF_3A_66, 0x0F), 1, &OpDispatchBuilder::PAlignrOp},
    {OPD(1, PF_3A_66, 0x16), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::PExtrOp, 8>},
    {OPD(1, PF_3A_66, 0x22), 1, &OpDispatchBuilder::PINSROp<8>},
  };

  InstallToTable(FinalTable, Table);
}

#undef PF_3A_NONE
#undef PF_3A_66

#undef OPD
} // namespace FEXCore::IR
