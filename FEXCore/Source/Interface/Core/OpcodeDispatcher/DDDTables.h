// SPDX-License-Identifier: MIT
#pragma once
#include "Interface/Core/OpcodeDispatcher.h"

namespace FEXCore::IR {
consteval inline void DDDTable_Install(auto& FinalTable) {
  constexpr std::tuple<uint8_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr> Table[] = {
    {0x0C, 1, &OpDispatchBuilder::PI2FWOp},
    {0x0D, 1, &OpDispatchBuilder::Vector_CVT_Int_To_Float<4, false>},
    {0x1C, 1, &OpDispatchBuilder::PF2IWOp},
    {0x1D, 1, &OpDispatchBuilder::Vector_CVT_Float_To_Int<4, false, false>},

    {0x86, 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VectorUnaryOp, IR::OP_VFRECP, 4>},
    {0x87, 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VectorUnaryOp, IR::OP_VFRSQRT, 4>},

    {0x8A, 1, &OpDispatchBuilder::PFNACCOp},
    {0x8E, 1, &OpDispatchBuilder::PFPNACCOp},

    {0x90, 1, &OpDispatchBuilder::VPFCMPOp<1>},
    {0x94, 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VectorALUOp, IR::OP_VFMIN, 4>},
    {0x96, 1, &OpDispatchBuilder::VectorUnaryDuplicateOp<IR::OP_VFRECP, 4>},
    {0x97, 1, &OpDispatchBuilder::VectorUnaryDuplicateOp<IR::OP_VFRSQRT, 4>},

    {0x9A, 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VectorALUOp, IR::OP_VFSUB, 4>},
    {0x9E, 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VectorALUOp, IR::OP_VFADD, 4>},

    {0xA0, 1, &OpDispatchBuilder::VPFCMPOp<2>},
    {0xA4, 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VectorALUOp, IR::OP_VFMAX, 4>},
    // Can be treated as a move
    {0xA6, 1, &OpDispatchBuilder::MOVVectorUnalignedOp},
    {0xA7, 1, &OpDispatchBuilder::MOVVectorUnalignedOp},

    {0xAA, 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VectorALUROp, IR::OP_VFSUB, 4>},
    {0xAE, 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VectorALUOp, IR::OP_VFADDP, 4>},

    {0xB0, 1, &OpDispatchBuilder::VPFCMPOp<0>},
    {0xB4, 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VectorALUOp, IR::OP_VFMUL, 4>},
    // Can be treated as a move
    {0xB6, 1, &OpDispatchBuilder::MOVVectorUnalignedOp},
    {0xB7, 1, &OpDispatchBuilder::PMULHRWOp},

    {0xBB, 1, &OpDispatchBuilder::PSWAPDOp},
    {0xBF, 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VectorALUOp, IR::OP_VURAVG, 1>},
  };

  InstallToTable(FinalTable, Table);
}

} // namespace FEXCore::IR
