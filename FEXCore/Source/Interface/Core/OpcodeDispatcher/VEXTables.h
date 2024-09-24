// SPDX-License-Identifier: MIT
#pragma once
#include "Interface/Core/OpcodeDispatcher.h"

namespace FEXCore::IR {
#define OPD(map_select, pp, opcode) (((map_select - 1) << 10) | (pp << 8) | (opcode))
constexpr std::tuple<uint16_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr> OpDispatch_VEXTable[] = {
  {OPD(2, 0b00, 0xF2), 1, &OpDispatchBuilder::ANDNBMIOp}, {OPD(2, 0b00, 0xF5), 1, &OpDispatchBuilder::BZHI},
  {OPD(2, 0b10, 0xF5), 1, &OpDispatchBuilder::PEXT},      {OPD(2, 0b11, 0xF5), 1, &OpDispatchBuilder::PDEP},
  {OPD(2, 0b11, 0xF6), 1, &OpDispatchBuilder::MULX},      {OPD(2, 0b00, 0xF7), 1, &OpDispatchBuilder::BEXTRBMIOp},
  {OPD(2, 0b01, 0xF7), 1, &OpDispatchBuilder::BMI2Shift}, {OPD(2, 0b10, 0xF7), 1, &OpDispatchBuilder::BMI2Shift},
  {OPD(2, 0b11, 0xF7), 1, &OpDispatchBuilder::BMI2Shift},

  {OPD(3, 0b11, 0xF0), 1, &OpDispatchBuilder::RORX},
};
#undef OPD

#define OPD(group, pp, opcode) (((group - X86Tables::InstType::TYPE_VEX_GROUP_12) << 4) | (pp << 3) | (opcode))
constexpr std::tuple<uint8_t, uint8_t, X86Tables::OpDispatchPtr> OpDispatch_VEXGroupTable[] = {
  {OPD(X86Tables::InstType::TYPE_VEX_GROUP_17, 0, 0b001), 1, &OpDispatchBuilder::BLSRBMIOp},
  {OPD(X86Tables::InstType::TYPE_VEX_GROUP_17, 0, 0b010), 1, &OpDispatchBuilder::BLSMSKBMIOp},
  {OPD(X86Tables::InstType::TYPE_VEX_GROUP_17, 0, 0b011), 1, &OpDispatchBuilder::BLSIBMIOp},
};
#undef OPD

} // namespace FEXCore::IR
