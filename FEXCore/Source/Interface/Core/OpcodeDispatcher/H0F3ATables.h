// SPDX-License-Identifier: MIT
#pragma once
#include "Interface/Core/OpcodeDispatcher.h"

namespace FEXCore::IR {
#define OPD(REX, prefix, opcode) ((REX << 9) | (prefix << 8) | opcode)
#define PF_3A_NONE 0
#define PF_3A_66 1
constexpr auto OpDispatchTableGenH0F3A = []() consteval {
  constexpr auto OpDispatchTableGenH0F3AREX = []<uint16_t REX>() consteval {
    constexpr DispatchTableEntry Table[] = {
      {OPD(REX, PF_3A_66, 0x08), 1, &OpDispatchBuilder::VectorRound<OpSize::i32Bit>},
      {OPD(REX, PF_3A_66, 0x09), 1, &OpDispatchBuilder::VectorRound<OpSize::i64Bit>},
      {OPD(REX, PF_3A_66, 0x0A), 1, &OpDispatchBuilder::InsertScalarRound<OpSize::i32Bit>},
      {OPD(REX, PF_3A_66, 0x0B), 1, &OpDispatchBuilder::InsertScalarRound<OpSize::i64Bit>},
      {OPD(REX, PF_3A_66, 0x0C), 1, &OpDispatchBuilder::VectorBlend<OpSize::i32Bit>},
      {OPD(REX, PF_3A_66, 0x0D), 1, &OpDispatchBuilder::VectorBlend<OpSize::i64Bit>},
      {OPD(REX, PF_3A_66, 0x0E), 1, &OpDispatchBuilder::VectorBlend<OpSize::i16Bit>},

      {OPD(REX, PF_3A_NONE, 0x0F), 1, &OpDispatchBuilder::PAlignrOp},
      {OPD(REX, PF_3A_66, 0x0F), 1, &OpDispatchBuilder::PAlignrOp},

      {OPD(REX, PF_3A_66, 0x14), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::PExtrOp, OpSize::i8Bit>},
      {OPD(REX, PF_3A_66, 0x15), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::PExtrOp, OpSize::i16Bit>},
      {OPD(REX, PF_3A_66, 0x17), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::PExtrOp, OpSize::i32Bit>},

      {OPD(REX, PF_3A_66, 0x20), 1, &OpDispatchBuilder::PINSROp<OpSize::i8Bit>},
      {OPD(REX, PF_3A_66, 0x21), 1, &OpDispatchBuilder::InsertPSOp},
      {OPD(REX, PF_3A_66, 0x40), 1, &OpDispatchBuilder::DPPOp<OpSize::i32Bit>},
      {OPD(REX, PF_3A_66, 0x41), 1, &OpDispatchBuilder::DPPOp<OpSize::i64Bit>},
      {OPD(REX, PF_3A_66, 0x42), 1, &OpDispatchBuilder::MPSADBWOp},

      {OPD(REX, PF_3A_66, 0x60), 1, &OpDispatchBuilder::VPCMPESTRMOp},
      {OPD(REX, PF_3A_66, 0x61), 1, &OpDispatchBuilder::VPCMPESTRIOp},
      {OPD(REX, PF_3A_66, 0x62), 1, &OpDispatchBuilder::VPCMPISTRMOp},
      {OPD(REX, PF_3A_66, 0x63), 1, &OpDispatchBuilder::VPCMPISTRIOp},

      {OPD(REX, PF_3A_NONE, 0xCC), 1, &OpDispatchBuilder::SHA1RNDS4Op},
    };
    return std::to_array(Table);
  };

  auto REX0 = OpDispatchTableGenH0F3AREX.template operator()<0>();
  auto REX1 = OpDispatchTableGenH0F3AREX.template operator()<1>();
  auto concat = []<typename T, size_t N1, size_t N2>(const std::array<T, N1>& lhs,
                                                     const std::array<T, N2>& rhs) consteval -> std::array<T, N1 + N2> {
    std::array<T, N1 + N2> Table {};
    for (size_t i = 0; i < N1; ++i) {
      Table[i] = lhs[i];
    }

    for (size_t i = 0; i < N2; ++i) {
      Table[N1 + i] = rhs[i];
    }

    return Table;
  };
  return concat(REX0, REX1);
};

constexpr auto OpDispatch_H0F3ATableIgnoreREX = OpDispatchTableGenH0F3A();

constexpr DispatchTableEntry OpDispatch_H0F3ATableNeedsREX0[] = {
  {OPD(0, PF_3A_66, 0x16), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::PExtrOp, OpSize::i32Bit>},
  {OPD(0, PF_3A_66, 0x22), 1, &OpDispatchBuilder::PINSROp<OpSize::i32Bit>},
};

constexpr DispatchTableEntry OpDispatch_H0F3ATable_64[] = {
  {OPD(1, PF_3A_66, 0x16), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::PExtrOp, OpSize::i64Bit>},
  {OPD(1, PF_3A_66, 0x22), 1, &OpDispatchBuilder::PINSROp<OpSize::i64Bit>},
};

#undef PF_3A_NONE
#undef PF_3A_66

#undef OPD
} // namespace FEXCore::IR
