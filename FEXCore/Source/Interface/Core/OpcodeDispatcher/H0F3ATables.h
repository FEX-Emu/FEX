// SPDX-License-Identifier: MIT
#pragma once
#include "Interface/Core/OpcodeDispatcher.h"

namespace FEXCore::IR {
#define OPD(REX, prefix, opcode) ((REX << 9) | (prefix << 8) | opcode)
#define PF_3A_NONE 0
#define PF_3A_66 1
constexpr std::tuple<uint16_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr> OpDispatch_H0F3ATable[] = {
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

  {OPD(0, PF_3A_66, 0x20), 1, &OpDispatchBuilder::PINSROp<OpSize::i8Bit>},
  {OPD(0, PF_3A_66, 0x21), 1, &OpDispatchBuilder::InsertPSOp},
  {OPD(0, PF_3A_66, 0x22), 1, &OpDispatchBuilder::PINSROp<OpSize::i32Bit>},
  {OPD(0, PF_3A_66, 0x40), 1, &OpDispatchBuilder::DPPOp<4>},
  {OPD(0, PF_3A_66, 0x41), 1, &OpDispatchBuilder::DPPOp<8>},
  {OPD(0, PF_3A_66, 0x42), 1, &OpDispatchBuilder::MPSADBWOp},

  {OPD(0, PF_3A_66, 0x60), 1, &OpDispatchBuilder::VPCMPESTRMOp},
  {OPD(0, PF_3A_66, 0x61), 1, &OpDispatchBuilder::VPCMPESTRIOp},
  {OPD(0, PF_3A_66, 0x62), 1, &OpDispatchBuilder::VPCMPISTRMOp},
  {OPD(0, PF_3A_66, 0x63), 1, &OpDispatchBuilder::VPCMPISTRIOp},

  {OPD(0, PF_3A_NONE, 0xCC), 1, &OpDispatchBuilder::SHA1RNDS4Op},
};

constexpr std::tuple<uint16_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr> OpDispatch_H0F3ATable_64[] = {
  {OPD(1, PF_3A_66, 0x0F), 1, &OpDispatchBuilder::PAlignrOp},
  {OPD(1, PF_3A_66, 0x16), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::PExtrOp, 8>},
  {OPD(1, PF_3A_66, 0x22), 1, &OpDispatchBuilder::PINSROp<OpSize::i64Bit>},
};

#undef PF_3A_NONE
#undef PF_3A_66

#undef OPD
} // namespace FEXCore::IR
