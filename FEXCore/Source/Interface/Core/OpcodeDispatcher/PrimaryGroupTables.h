// SPDX-License-Identifier: MIT
#pragma once
#include "Interface/Core/OpcodeDispatcher.h"

namespace FEXCore::IR {
consteval inline void PrimaryGroupTables_Install(auto& FinalTable) {
  using X86Tables::OpToIndex;
#define OPD(group, prefix, Reg) (((group - FEXCore::X86Tables::TYPE_GROUP_1) << 6) | (prefix) << 3 | (Reg))
  constexpr std::tuple<uint16_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr> Table[] = {
    // GROUP 1
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x80), 0), 1, &OpDispatchBuilder::SecondaryALUOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x80), 1), 1, &OpDispatchBuilder::SecondaryALUOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x80), 2), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::ADCOp, 1>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x80), 3), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::SBBOp, 1>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x80), 4), 1, &OpDispatchBuilder::SecondaryALUOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x80), 5), 1, &OpDispatchBuilder::SecondaryALUOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x80), 6), 1, &OpDispatchBuilder::SecondaryALUOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x80), 7), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::CMPOp, 1>}, // CMP

    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x81), 0), 1, &OpDispatchBuilder::SecondaryALUOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x81), 1), 1, &OpDispatchBuilder::SecondaryALUOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x81), 2), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::ADCOp, 1>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x81), 3), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::SBBOp, 1>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x81), 4), 1, &OpDispatchBuilder::SecondaryALUOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x81), 5), 1, &OpDispatchBuilder::SecondaryALUOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x81), 6), 1, &OpDispatchBuilder::SecondaryALUOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x81), 7), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::CMPOp, 1>},

    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x83), 0), 1, &OpDispatchBuilder::SecondaryALUOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x83), 1), 1, &OpDispatchBuilder::SecondaryALUOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x83), 2), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::ADCOp, 1>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x83), 3), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::SBBOp, 1>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x83), 4), 1, &OpDispatchBuilder::SecondaryALUOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x83), 5), 1, &OpDispatchBuilder::SecondaryALUOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x83), 6), 1, &OpDispatchBuilder::SecondaryALUOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_1, OpToIndex(0x83), 7), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::CMPOp, 1>},

    // GROUP 2
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC0), 0), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::RotateOp, true, true, false>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC0), 1), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::RotateOp, false, true, false>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC0), 2), 1, &OpDispatchBuilder::RCLOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC0), 3), 1, &OpDispatchBuilder::RCROp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC0), 4), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::SHLImmediateOp, false>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC0), 5), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::SHRImmediateOp, false>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC0), 6), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::SHLImmediateOp, false>}, // SAL
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC0), 7), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::ASHROp, true, false>}, // SAR

    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC1), 0), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::RotateOp, true, true, false>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC1), 1), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::RotateOp, false, true, false>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC1), 2), 1, &OpDispatchBuilder::RCLOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC1), 3), 1, &OpDispatchBuilder::RCROp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC1), 4), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::SHLImmediateOp, false>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC1), 5), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::SHRImmediateOp, false>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC1), 6), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::SHLImmediateOp, false>}, // SAL
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xC1), 7), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::ASHROp, true, false>}, // SAR

    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD0), 0), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::RotateOp, true, true, true>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD0), 1), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::RotateOp, false, true, true>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD0), 2), 1, &OpDispatchBuilder::RCLOp1Bit},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD0), 3), 1, &OpDispatchBuilder::RCROp8x1Bit},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD0), 4), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::SHLImmediateOp, true>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD0), 5), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::SHRImmediateOp, true>}, // 1Bit SHR
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD0), 6), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::SHLImmediateOp, true>}, // SAL
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD0), 7), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::ASHROp, true, true>}, // SAR

    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD1), 0), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::RotateOp, true, true, true>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD1), 1), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::RotateOp, false, true, true>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD1), 2), 1, &OpDispatchBuilder::RCLOp1Bit},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD1), 3), 1, &OpDispatchBuilder::RCROp1Bit},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD1), 4), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::SHLImmediateOp, true>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD1), 5), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::SHRImmediateOp, true>}, // 1Bit SHR
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD1), 6), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::SHLImmediateOp, true>}, // SAL
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD1), 7), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::ASHROp, true, true>}, // SAR

    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD2), 0), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::RotateOp, true, false, false>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD2), 1), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::RotateOp, false, false, false>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD2), 2), 1, &OpDispatchBuilder::RCLSmallerOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD2), 3), 1, &OpDispatchBuilder::RCRSmallerOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD2), 4), 1, &OpDispatchBuilder::SHLOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD2), 5), 1, &OpDispatchBuilder::SHROp}, // SHR by CL
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD2), 6), 1, &OpDispatchBuilder::SHLOp}, // SAL
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD2), 7), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::ASHROp, false, false>}, // SAR

    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD3), 0), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::RotateOp, true, false, false>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD3), 1), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::RotateOp, false, false, false>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD3), 2), 1, &OpDispatchBuilder::RCLOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD3), 3), 1, &OpDispatchBuilder::RCROp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD3), 4), 1, &OpDispatchBuilder::SHLOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD3), 5), 1, &OpDispatchBuilder::SHROp}, // SHR by CL
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD3), 6), 1, &OpDispatchBuilder::SHLOp}, // SAL
    {OPD(FEXCore::X86Tables::TYPE_GROUP_2, OpToIndex(0xD3), 7), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::ASHROp, false, false>}, // SAR

    // GROUP 3
    {OPD(FEXCore::X86Tables::TYPE_GROUP_3, OpToIndex(0xF6), 0), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::TESTOp, 1>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_3, OpToIndex(0xF6), 1), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::TESTOp, 1>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_3, OpToIndex(0xF6), 2), 1, &OpDispatchBuilder::NOTOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_3, OpToIndex(0xF6), 3), 1, &OpDispatchBuilder::NEGOp}, // NEG
    {OPD(FEXCore::X86Tables::TYPE_GROUP_3, OpToIndex(0xF6), 4), 1, &OpDispatchBuilder::MULOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_3, OpToIndex(0xF6), 5), 1, &OpDispatchBuilder::IMULOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_3, OpToIndex(0xF6), 6), 1, &OpDispatchBuilder::DIVOp},  // DIV
    {OPD(FEXCore::X86Tables::TYPE_GROUP_3, OpToIndex(0xF6), 7), 1, &OpDispatchBuilder::IDIVOp}, // IDIV

    {OPD(FEXCore::X86Tables::TYPE_GROUP_3, OpToIndex(0xF7), 0), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::TESTOp, 1>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_3, OpToIndex(0xF7), 1), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::TESTOp, 1>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_3, OpToIndex(0xF7), 2), 1, &OpDispatchBuilder::NOTOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_3, OpToIndex(0xF7), 3), 1, &OpDispatchBuilder::NEGOp}, // NEG

    {OPD(FEXCore::X86Tables::TYPE_GROUP_3, OpToIndex(0xF7), 4), 1, &OpDispatchBuilder::MULOp}, // MUL
    {OPD(FEXCore::X86Tables::TYPE_GROUP_3, OpToIndex(0xF7), 5), 1, &OpDispatchBuilder::IMULOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_3, OpToIndex(0xF7), 6), 1, &OpDispatchBuilder::DIVOp},  // DIV
    {OPD(FEXCore::X86Tables::TYPE_GROUP_3, OpToIndex(0xF7), 7), 1, &OpDispatchBuilder::IDIVOp}, // IDIV

    // GROUP 4
    {OPD(FEXCore::X86Tables::TYPE_GROUP_4, OpToIndex(0xFE), 0), 1, &OpDispatchBuilder::INCOp}, // INC
    {OPD(FEXCore::X86Tables::TYPE_GROUP_4, OpToIndex(0xFE), 1), 1, &OpDispatchBuilder::DECOp}, // DEC

    // GROUP 5
    {OPD(FEXCore::X86Tables::TYPE_GROUP_5, OpToIndex(0xFF), 0), 1, &OpDispatchBuilder::INCOp}, // INC
    {OPD(FEXCore::X86Tables::TYPE_GROUP_5, OpToIndex(0xFF), 1), 1, &OpDispatchBuilder::DECOp}, // DEC
    {OPD(FEXCore::X86Tables::TYPE_GROUP_5, OpToIndex(0xFF), 2), 1, &OpDispatchBuilder::CALLAbsoluteOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_5, OpToIndex(0xFF), 4), 1, &OpDispatchBuilder::JUMPAbsoluteOp},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_5, OpToIndex(0xFF), 6), 1, &OpDispatchBuilder::PUSHOp},

    // GROUP 11
    {OPD(FEXCore::X86Tables::TYPE_GROUP_11, OpToIndex(0xC6), 0), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::MOVGPROp, 1>},
    {OPD(FEXCore::X86Tables::TYPE_GROUP_11, OpToIndex(0xC7), 0), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::MOVGPROp, 1>},
  };
#undef OPD

  InstallToTable(FinalTable, Table);
}

} // namespace FEXCore::IR
