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

consteval inline void SecondaryModRMTables_Install(auto& FinalTable) {
  constexpr std::tuple<uint8_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr> Table[] = {
    // REG /1
    {((0 << 3) | 0), 1, &OpDispatchBuilder::UnimplementedOp},
    {((0 << 3) | 1), 1, &OpDispatchBuilder::UnimplementedOp},

    // REG /2
    {((1 << 3) | 0), 1, &OpDispatchBuilder::XGetBVOp},

    // REG /3
    {((2 << 3) | 7), 1, &OpDispatchBuilder::PermissionRestrictedOp},

    // REG /7
    {((3 << 3) | 0), 1, &OpDispatchBuilder::PermissionRestrictedOp},
    {((3 << 3) | 1), 1, &OpDispatchBuilder::RDTSCPOp},
  };

  InstallToTable(FinalTable, Table);
}

} // namespace FEXCore::IR
