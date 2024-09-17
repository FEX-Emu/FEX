// SPDX-License-Identifier: MIT
#pragma once
#include "Interface/Core/OpcodeDispatcher.h"

namespace FEXCore::IR {
constexpr std::tuple<uint8_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr> OpDispatch_SecondaryModRMTables[] = {
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

} // namespace FEXCore::IR
