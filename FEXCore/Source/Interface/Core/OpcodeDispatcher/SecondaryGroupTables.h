// SPDX-License-Identifier: MIT
#pragma once
#include "Interface/Core/OpcodeDispatcher.h"

namespace FEXCore::IR {
#define OPD(group, prefix, Reg) (((group - FEXCore::X86Tables::TYPE_GROUP_6) << 5) | (prefix) << 3 | (Reg))
constexpr uint16_t PF_NONE = 0;
constexpr uint16_t PF_F3 = 1;
constexpr uint16_t PF_66 = 2;
constexpr uint16_t PF_F2 = 3;
constexpr std::tuple<uint16_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr> OpDispatch_SecondaryGroupTables[] = {
  // GROUP 6
  {OPD(FEXCore::X86Tables::TYPE_GROUP_6, PF_NONE, 3), 1, &OpDispatchBuilder::PermissionRestrictedOp},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_6, PF_F3, 3), 1, &OpDispatchBuilder::PermissionRestrictedOp},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_6, PF_66, 3), 1, &OpDispatchBuilder::PermissionRestrictedOp},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_6, PF_F2, 3), 1, &OpDispatchBuilder::PermissionRestrictedOp},

  // GROUP 7
  {OPD(FEXCore::X86Tables::TYPE_GROUP_7, PF_NONE, 0), 1, &OpDispatchBuilder::SGDTOp},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_7, PF_F3, 0), 1, &OpDispatchBuilder::SGDTOp},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_7, PF_66, 0), 1, &OpDispatchBuilder::SGDTOp},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_7, PF_F2, 0), 1, &OpDispatchBuilder::SGDTOp},

  {OPD(FEXCore::X86Tables::TYPE_GROUP_7, PF_NONE, 1), 1, &OpDispatchBuilder::SIDTOp},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_7, PF_F3, 1), 1, &OpDispatchBuilder::SIDTOp},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_7, PF_66, 1), 1, &OpDispatchBuilder::SIDTOp},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_7, PF_F2, 1), 1, &OpDispatchBuilder::SIDTOp},

  {OPD(FEXCore::X86Tables::TYPE_GROUP_7, PF_NONE, 3), 1, &OpDispatchBuilder::PermissionRestrictedOp},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_7, PF_F3, 3), 1, &OpDispatchBuilder::PermissionRestrictedOp},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_7, PF_66, 3), 1, &OpDispatchBuilder::PermissionRestrictedOp},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_7, PF_F2, 3), 1, &OpDispatchBuilder::PermissionRestrictedOp},

  {OPD(FEXCore::X86Tables::TYPE_GROUP_7, PF_NONE, 4), 1, &OpDispatchBuilder::SMSWOp},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_7, PF_F3, 4), 1, &OpDispatchBuilder::SMSWOp},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_7, PF_66, 4), 1, &OpDispatchBuilder::SMSWOp},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_7, PF_F2, 4), 1, &OpDispatchBuilder::SMSWOp},

  {OPD(FEXCore::X86Tables::TYPE_GROUP_7, PF_NONE, 6), 1, &OpDispatchBuilder::PermissionRestrictedOp},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_7, PF_F3, 6), 1, &OpDispatchBuilder::PermissionRestrictedOp},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_7, PF_66, 6), 1, &OpDispatchBuilder::PermissionRestrictedOp},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_7, PF_F2, 6), 1, &OpDispatchBuilder::PermissionRestrictedOp},

  {OPD(FEXCore::X86Tables::TYPE_GROUP_7, PF_NONE, 7), 1, &OpDispatchBuilder::PermissionRestrictedOp},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_7, PF_F3, 7), 1, &OpDispatchBuilder::PermissionRestrictedOp},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_7, PF_66, 7), 1, &OpDispatchBuilder::PermissionRestrictedOp},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_7, PF_F2, 7), 1, &OpDispatchBuilder::PermissionRestrictedOp},

  // GROUP 8
  {OPD(FEXCore::X86Tables::TYPE_GROUP_8, PF_NONE, 4), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::BTOp, 1, BTAction::BTNone>},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_8, PF_F3, 4), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::BTOp, 1, BTAction::BTNone>},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_8, PF_66, 4), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::BTOp, 1, BTAction::BTNone>},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_8, PF_F2, 4), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::BTOp, 1, BTAction::BTNone>},

  {OPD(FEXCore::X86Tables::TYPE_GROUP_8, PF_NONE, 5), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::BTOp, 1, BTAction::BTSet>},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_8, PF_F3, 5), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::BTOp, 1, BTAction::BTSet>},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_8, PF_66, 5), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::BTOp, 1, BTAction::BTSet>},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_8, PF_F2, 5), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::BTOp, 1, BTAction::BTSet>},

  {OPD(FEXCore::X86Tables::TYPE_GROUP_8, PF_NONE, 6), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::BTOp, 1, BTAction::BTClear>},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_8, PF_F3, 6), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::BTOp, 1, BTAction::BTClear>},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_8, PF_66, 6), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::BTOp, 1, BTAction::BTClear>},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_8, PF_F2, 6), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::BTOp, 1, BTAction::BTClear>},

  {OPD(FEXCore::X86Tables::TYPE_GROUP_8, PF_NONE, 7), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::BTOp, 1, BTAction::BTComplement>},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_8, PF_F3, 7), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::BTOp, 1, BTAction::BTComplement>},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_8, PF_66, 7), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::BTOp, 1, BTAction::BTComplement>},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_8, PF_F2, 7), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::BTOp, 1, BTAction::BTComplement>},

  // GROUP 9
  {OPD(FEXCore::X86Tables::TYPE_GROUP_9, PF_NONE, 1), 1, &OpDispatchBuilder::CMPXCHGPairOp},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_9, PF_F3, 1), 1, &OpDispatchBuilder::CMPXCHGPairOp},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_9, PF_66, 1), 1, &OpDispatchBuilder::CMPXCHGPairOp},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_9, PF_F2, 1), 1, &OpDispatchBuilder::CMPXCHGPairOp},

  {OPD(FEXCore::X86Tables::TYPE_GROUP_9, PF_F3, 7), 1, &OpDispatchBuilder::RDPIDOp},

  // GROUP 12
  {OPD(FEXCore::X86Tables::TYPE_GROUP_12, PF_NONE, 2), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::PSRLI, OpSize::i16Bit>},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_12, PF_NONE, 4), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::PSRAIOp, OpSize::i16Bit>},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_12, PF_NONE, 6), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::PSLLI, OpSize::i16Bit>},

  {OPD(FEXCore::X86Tables::TYPE_GROUP_12, PF_66, 2), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::PSRLI, OpSize::i16Bit>},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_12, PF_66, 4), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::PSRAIOp, OpSize::i16Bit>},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_12, PF_66, 6), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::PSLLI, OpSize::i16Bit>},

  // GROUP 13
  {OPD(FEXCore::X86Tables::TYPE_GROUP_13, PF_NONE, 2), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::PSRLI, OpSize::i32Bit>},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_13, PF_NONE, 4), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::PSRAIOp, OpSize::i32Bit>},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_13, PF_NONE, 6), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::PSLLI, OpSize::i32Bit>},

  {OPD(FEXCore::X86Tables::TYPE_GROUP_13, PF_66, 2), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::PSRLI, OpSize::i32Bit>},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_13, PF_66, 4), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::PSRAIOp, OpSize::i32Bit>},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_13, PF_66, 6), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::PSLLI, OpSize::i32Bit>},

  // GROUP 14
  {OPD(FEXCore::X86Tables::TYPE_GROUP_14, PF_NONE, 2), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::PSRLI, OpSize::i64Bit>},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_14, PF_NONE, 6), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::PSLLI, OpSize::i64Bit>},

  {OPD(FEXCore::X86Tables::TYPE_GROUP_14, PF_66, 2), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::PSRLI, OpSize::i64Bit>},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_14, PF_66, 3), 1, &OpDispatchBuilder::PSRLDQ},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_14, PF_66, 6), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::PSLLI, OpSize::i64Bit>},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_14, PF_66, 7), 1, &OpDispatchBuilder::PSLLDQ},

  // GROUP 15
  {OPD(FEXCore::X86Tables::TYPE_GROUP_15, PF_NONE, 0), 1, &OpDispatchBuilder::FXSaveOp},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_15, PF_NONE, 1), 1, &OpDispatchBuilder::FXRStoreOp},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_15, PF_NONE, 2), 1, &OpDispatchBuilder::LDMXCSR},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_15, PF_NONE, 3), 1, &OpDispatchBuilder::STMXCSR},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_15, PF_NONE, 4), 1, &OpDispatchBuilder::XSaveOp},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_15, PF_NONE, 5), 1, &OpDispatchBuilder::LoadFenceOrXRSTOR},   // LFENCE (or XRSTOR)
  {OPD(FEXCore::X86Tables::TYPE_GROUP_15, PF_NONE, 6), 1, &OpDispatchBuilder::MemFenceOrXSAVEOPT},  // MFENCE (or XSAVEOPT)
  {OPD(FEXCore::X86Tables::TYPE_GROUP_15, PF_NONE, 7), 1, &OpDispatchBuilder::StoreFenceOrCLFlush}, // SFENCE (or CLFLUSH)

  {OPD(FEXCore::X86Tables::TYPE_GROUP_15, PF_F3, 5), 1, &OpDispatchBuilder::UnimplementedOp},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_15, PF_F3, 6), 1, &OpDispatchBuilder::UnimplementedOp},

  {OPD(FEXCore::X86Tables::TYPE_GROUP_15, PF_66, 6), 1, &OpDispatchBuilder::CLWB},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_15, PF_66, 7), 1, &OpDispatchBuilder::CLFLUSHOPT},

  // GROUP 16
  {OPD(FEXCore::X86Tables::TYPE_GROUP_16, PF_NONE, 0), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::Prefetch, false, true, 1>},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_16, PF_NONE, 1), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::Prefetch, false, false, 1>},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_16, PF_NONE, 2), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::Prefetch, false, false, 2>},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_16, PF_NONE, 3), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::Prefetch, false, false, 3>},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_16, PF_NONE, 4), 4, &OpDispatchBuilder::NOPOp},

  {OPD(FEXCore::X86Tables::TYPE_GROUP_16, PF_F3, 0), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::Prefetch, false, true, 1>},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_16, PF_F3, 1), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::Prefetch, false, false, 1>},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_16, PF_F3, 2), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::Prefetch, false, false, 2>},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_16, PF_F3, 3), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::Prefetch, false, false, 3>},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_16, PF_F3, 4), 4, &OpDispatchBuilder::NOPOp},

  {OPD(FEXCore::X86Tables::TYPE_GROUP_16, PF_66, 0), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::Prefetch, false, true, 1>},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_16, PF_66, 1), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::Prefetch, false, false, 1>},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_16, PF_66, 2), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::Prefetch, false, false, 2>},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_16, PF_66, 3), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::Prefetch, false, false, 3>},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_16, PF_66, 4), 4, &OpDispatchBuilder::NOPOp},

  {OPD(FEXCore::X86Tables::TYPE_GROUP_16, PF_F2, 0), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::Prefetch, false, true, 1>},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_16, PF_F2, 1), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::Prefetch, false, false, 1>},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_16, PF_F2, 2), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::Prefetch, false, false, 2>},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_16, PF_F2, 3), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::Prefetch, false, false, 3>},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_16, PF_F2, 4), 4, &OpDispatchBuilder::NOPOp},

  // GROUP P
  {OPD(FEXCore::X86Tables::TYPE_GROUP_P, PF_NONE, 0), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::Prefetch, false, false, 1>},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_P, PF_NONE, 1), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::Prefetch, true, false, 1>},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_P, PF_NONE, 2), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::Prefetch, true, false, 1>},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_P, PF_NONE, 3), 5, &OpDispatchBuilder::NOPOp},

  {OPD(FEXCore::X86Tables::TYPE_GROUP_P, PF_F3, 0), 8, &OpDispatchBuilder::NOPOp},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_P, PF_66, 0), 8, &OpDispatchBuilder::NOPOp},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_P, PF_F2, 0), 8, &OpDispatchBuilder::NOPOp},
};

constexpr std::tuple<uint16_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr> OpDispatch_SecondaryGroupTables_64[] = {
  // GROUP 15
  {OPD(FEXCore::X86Tables::TYPE_GROUP_15, PF_F3, 0), 1,
   &OpDispatchBuilder::Bind<&OpDispatchBuilder::ReadSegmentReg, OpDispatchBuilder::Segment::FS>},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_15, PF_F3, 1), 1,
   &OpDispatchBuilder::Bind<&OpDispatchBuilder::ReadSegmentReg, OpDispatchBuilder::Segment::GS>},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_15, PF_F3, 2), 1,
   &OpDispatchBuilder::Bind<&OpDispatchBuilder::WriteSegmentReg, OpDispatchBuilder::Segment::FS>},
  {OPD(FEXCore::X86Tables::TYPE_GROUP_15, PF_F3, 3), 1,
   &OpDispatchBuilder::Bind<&OpDispatchBuilder::WriteSegmentReg, OpDispatchBuilder::Segment::GS>},
};

#undef OPD

} // namespace FEXCore::IR
