// SPDX-License-Identifier: MIT
#pragma once
#include "Interface/Core/OpcodeDispatcher.h"

namespace FEXCore::IR {
constexpr inline DispatchTableEntry OpDispatch_BaseOpTable[] = {
  // Instructions
  {0x00, 6, &OpDispatchBuilder::Bind<&OpDispatchBuilder::ALUOp, FEXCore::IR::IROps::OP_ADD, FEXCore::IR::IROps::OP_ATOMICFETCHADD, 0>},

  {0x08, 6, &OpDispatchBuilder::Bind<&OpDispatchBuilder::ALUOp, FEXCore::IR::IROps::OP_OR, FEXCore::IR::IROps::OP_ATOMICFETCHOR, 0>},

  {0x10, 6, &OpDispatchBuilder::Bind<&OpDispatchBuilder::ADCOp, 0>},

  {0x18, 6, &OpDispatchBuilder::Bind<&OpDispatchBuilder::SBBOp, 0>},

  {0x20, 6, &OpDispatchBuilder::Bind<&OpDispatchBuilder::ALUOp, FEXCore::IR::IROps::OP_ANDWITHFLAGS, FEXCore::IR::IROps::OP_ATOMICFETCHAND, 0>},

  {0x28, 6, &OpDispatchBuilder::Bind<&OpDispatchBuilder::ALUOp, FEXCore::IR::IROps::OP_SUB, FEXCore::IR::IROps::OP_ATOMICFETCHSUB, 0>},

  {0x30, 6, &OpDispatchBuilder::Bind<&OpDispatchBuilder::ALUOp, FEXCore::IR::IROps::OP_XOR, FEXCore::IR::IROps::OP_ATOMICFETCHXOR, 0>},

  {0x38, 6, &OpDispatchBuilder::Bind<&OpDispatchBuilder::CMPOp, 0>},
  {0x50, 8, &OpDispatchBuilder::PUSHREGOp},
  {0x58, 8, &OpDispatchBuilder::POPOp},
  {0x68, 1, &OpDispatchBuilder::PUSHOp},
  {0x69, 1, &OpDispatchBuilder::IMUL2SrcOp},
  {0x6A, 1, &OpDispatchBuilder::PUSHOp},
  {0x6B, 1, &OpDispatchBuilder::IMUL2SrcOp},
  {0x6C, 4, &OpDispatchBuilder::PermissionRestrictedOp},

  {0x70, 16, &OpDispatchBuilder::CondJUMPOp},
  {0x84, 2, &OpDispatchBuilder::Bind<&OpDispatchBuilder::TESTOp, 0>},
  {0x86, 2, &OpDispatchBuilder::XCHGOp},
  {0x88, 4, &OpDispatchBuilder::Bind<&OpDispatchBuilder::MOVGPROp, 0>},

  {0x8C, 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::MOVSegOp, false>},
  {0x8D, 1, &OpDispatchBuilder::LEAOp},
  {0x8E, 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::MOVSegOp, true>},
  {0x8F, 1, &OpDispatchBuilder::POPOp},
  {0x90, 8, &OpDispatchBuilder::XCHGOp},

  {0x98, 1, &OpDispatchBuilder::CDQOp},
  {0x99, 1, &OpDispatchBuilder::CQOOp},
  {0x9B, 1, &OpDispatchBuilder::NOPOp},
  {0x9C, 1, &OpDispatchBuilder::PUSHFOp},
  {0x9D, 1, &OpDispatchBuilder::POPFOp},
  {0x9E, 1, &OpDispatchBuilder::SAHFOp},
  {0x9F, 1, &OpDispatchBuilder::LAHFOp},
  {0xA4, 2, &OpDispatchBuilder::MOVSOp},

  {0xA6, 2, &OpDispatchBuilder::CMPSOp},
  {0xA8, 2, &OpDispatchBuilder::Bind<&OpDispatchBuilder::TESTOp, 0>},
  {0xAA, 2, &OpDispatchBuilder::STOSOp},
  {0xAC, 2, &OpDispatchBuilder::LODSOp},
  {0xAE, 2, &OpDispatchBuilder::SCASOp},
  {0xB0, 16, &OpDispatchBuilder::Bind<&OpDispatchBuilder::MOVGPROp, 0>},
  {0xC2, 2, &OpDispatchBuilder::RETOp},
  {0xC8, 1, &OpDispatchBuilder::EnterOp},
  {0xC9, 1, &OpDispatchBuilder::LEAVEOp},
  {0xCA, 2, &OpDispatchBuilder::RETFARIndirectOp},
  {0xCC, 2, &OpDispatchBuilder::INTOp},
  {0xCF, 1, &OpDispatchBuilder::IRETOp},
  {0xD7, 2, &OpDispatchBuilder::XLATOp},
  {0xE0, 3, &OpDispatchBuilder::LoopOp},
  {0xE3, 1, &OpDispatchBuilder::CondJUMPRCXOp},
  {0xE4, 4, &OpDispatchBuilder::PermissionRestrictedOp},
  {0xE8, 1, &OpDispatchBuilder::CALLOp},
  {0xE9, 1, &OpDispatchBuilder::JUMPOp},
  {0xEB, 1, &OpDispatchBuilder::JUMPOp},
  {0xEC, 4, &OpDispatchBuilder::PermissionRestrictedOp},
  {0xF1, 1, &OpDispatchBuilder::INTOp},
  {0xF4, 1, &OpDispatchBuilder::INTOp},

  {0xF5, 1, &OpDispatchBuilder::FLAGControlOp},
  {0xF8, 2, &OpDispatchBuilder::FLAGControlOp},
  {0xFA, 2, &OpDispatchBuilder::PermissionRestrictedOp},
  {0xFC, 2, &OpDispatchBuilder::FLAGControlOp},
};
} // namespace FEXCore::IR
