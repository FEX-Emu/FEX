// SPDX-License-Identifier: MIT
#pragma once
#include "Interface/Core/OpcodeDispatcher.h"

namespace FEXCore::IR {
#define OPD(prefix, opcode) (((prefix) << 8) | opcode)
constexpr uint16_t PF_38_NONE = 0;
constexpr uint16_t PF_38_66 = (1U << 0);
constexpr uint16_t PF_38_F3 = (1U << 2);

constexpr std::tuple<uint16_t, uint8_t, FEXCore::X86Tables::OpDispatchPtr> OpDispatch_H0F38Table[] = {
  {OPD(PF_38_NONE, 0x00), 1, &OpDispatchBuilder::PSHUFBOp},
  {OPD(PF_38_66, 0x00), 1, &OpDispatchBuilder::PSHUFBOp},
  {OPD(PF_38_NONE, 0x01), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VectorALUOp, IR::OP_VADDP, OpSize::i16Bit>},
  {OPD(PF_38_66, 0x01), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VectorALUOp, IR::OP_VADDP, OpSize::i16Bit>},
  {OPD(PF_38_NONE, 0x02), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VectorALUOp, IR::OP_VADDP, OpSize::i32Bit>},
  {OPD(PF_38_66, 0x02), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VectorALUOp, IR::OP_VADDP, OpSize::i32Bit>},
  {OPD(PF_38_NONE, 0x03), 1, &OpDispatchBuilder::PHADDS},
  {OPD(PF_38_66, 0x03), 1, &OpDispatchBuilder::PHADDS},
  {OPD(PF_38_NONE, 0x04), 1, &OpDispatchBuilder::PMADDUBSW},
  {OPD(PF_38_66, 0x04), 1, &OpDispatchBuilder::PMADDUBSW},
  {OPD(PF_38_NONE, 0x05), 1, &OpDispatchBuilder::PHSUB<OpSize::i16Bit>},
  {OPD(PF_38_66, 0x05), 1, &OpDispatchBuilder::PHSUB<OpSize::i16Bit>},
  {OPD(PF_38_NONE, 0x06), 1, &OpDispatchBuilder::PHSUB<OpSize::i32Bit>},
  {OPD(PF_38_66, 0x06), 1, &OpDispatchBuilder::PHSUB<OpSize::i32Bit>},
  {OPD(PF_38_NONE, 0x07), 1, &OpDispatchBuilder::PHSUBS},
  {OPD(PF_38_66, 0x07), 1, &OpDispatchBuilder::PHSUBS},
  {OPD(PF_38_NONE, 0x08), 1, &OpDispatchBuilder::PSIGN<OpSize::i8Bit>},
  {OPD(PF_38_66, 0x08), 1, &OpDispatchBuilder::PSIGN<OpSize::i8Bit>},
  {OPD(PF_38_NONE, 0x09), 1, &OpDispatchBuilder::PSIGN<OpSize::i16Bit>},
  {OPD(PF_38_66, 0x09), 1, &OpDispatchBuilder::PSIGN<OpSize::i16Bit>},
  {OPD(PF_38_NONE, 0x0A), 1, &OpDispatchBuilder::PSIGN<OpSize::i32Bit>},
  {OPD(PF_38_66, 0x0A), 1, &OpDispatchBuilder::PSIGN<OpSize::i32Bit>},
  {OPD(PF_38_NONE, 0x0B), 1, &OpDispatchBuilder::PMULHRSW},
  {OPD(PF_38_66, 0x0B), 1, &OpDispatchBuilder::PMULHRSW},
  {OPD(PF_38_66, 0x10), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VectorVariableBlend, OpSize::i8Bit>},
  {OPD(PF_38_66, 0x14), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VectorVariableBlend, OpSize::i32Bit>},
  {OPD(PF_38_66, 0x15), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VectorVariableBlend, OpSize::i64Bit>},
  {OPD(PF_38_66, 0x17), 1, &OpDispatchBuilder::PTestOp},
  {OPD(PF_38_NONE, 0x1C), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VectorUnaryOp, IR::OP_VABS, OpSize::i8Bit>},
  {OPD(PF_38_66, 0x1C), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VectorUnaryOp, IR::OP_VABS, OpSize::i8Bit>},
  {OPD(PF_38_NONE, 0x1D), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VectorUnaryOp, IR::OP_VABS, OpSize::i16Bit>},
  {OPD(PF_38_66, 0x1D), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VectorUnaryOp, IR::OP_VABS, OpSize::i16Bit>},
  {OPD(PF_38_NONE, 0x1E), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VectorUnaryOp, IR::OP_VABS, OpSize::i32Bit>},
  {OPD(PF_38_66, 0x1E), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VectorUnaryOp, IR::OP_VABS, OpSize::i32Bit>},
  {OPD(PF_38_66, 0x20), 1, &OpDispatchBuilder::ExtendVectorElements<OpSize::i8Bit, OpSize::i16Bit, true>},
  {OPD(PF_38_66, 0x21), 1, &OpDispatchBuilder::ExtendVectorElements<OpSize::i8Bit, OpSize::i32Bit, true>},
  {OPD(PF_38_66, 0x22), 1, &OpDispatchBuilder::ExtendVectorElements<OpSize::i8Bit, OpSize::i64Bit, true>},
  {OPD(PF_38_66, 0x23), 1, &OpDispatchBuilder::ExtendVectorElements<OpSize::i16Bit, OpSize::i32Bit, true>},
  {OPD(PF_38_66, 0x24), 1, &OpDispatchBuilder::ExtendVectorElements<OpSize::i16Bit, OpSize::i64Bit, true>},
  {OPD(PF_38_66, 0x25), 1, &OpDispatchBuilder::ExtendVectorElements<OpSize::i32Bit, OpSize::i64Bit, true>},
  {OPD(PF_38_66, 0x28), 1, &OpDispatchBuilder::PMULLOp<4, true>},
  {OPD(PF_38_66, 0x29), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VectorALUOp, IR::OP_VCMPEQ, OpSize::i64Bit>},
  {OPD(PF_38_66, 0x2A), 1, &OpDispatchBuilder::MOVVectorNTOp},
  {OPD(PF_38_66, 0x2B), 1, &OpDispatchBuilder::PACKUSOp<OpSize::i32Bit>},
  {OPD(PF_38_66, 0x30), 1, &OpDispatchBuilder::ExtendVectorElements<OpSize::i8Bit, OpSize::i16Bit, false>},
  {OPD(PF_38_66, 0x31), 1, &OpDispatchBuilder::ExtendVectorElements<OpSize::i8Bit, OpSize::i32Bit, false>},
  {OPD(PF_38_66, 0x32), 1, &OpDispatchBuilder::ExtendVectorElements<OpSize::i8Bit, OpSize::i64Bit, false>},
  {OPD(PF_38_66, 0x33), 1, &OpDispatchBuilder::ExtendVectorElements<OpSize::i16Bit, OpSize::i32Bit, false>},
  {OPD(PF_38_66, 0x34), 1, &OpDispatchBuilder::ExtendVectorElements<OpSize::i16Bit, OpSize::i64Bit, false>},
  {OPD(PF_38_66, 0x35), 1, &OpDispatchBuilder::ExtendVectorElements<OpSize::i32Bit, OpSize::i64Bit, false>},
  {OPD(PF_38_66, 0x37), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VectorALUOp, IR::OP_VCMPGT, OpSize::i64Bit>},
  {OPD(PF_38_66, 0x38), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VectorALUOp, IR::OP_VSMIN, OpSize::i8Bit>},
  {OPD(PF_38_66, 0x39), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VectorALUOp, IR::OP_VSMIN, OpSize::i32Bit>},
  {OPD(PF_38_66, 0x3A), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VectorALUOp, IR::OP_VUMIN, OpSize::i16Bit>},
  {OPD(PF_38_66, 0x3B), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VectorALUOp, IR::OP_VUMIN, OpSize::i32Bit>},
  {OPD(PF_38_66, 0x3C), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VectorALUOp, IR::OP_VSMAX, OpSize::i8Bit>},
  {OPD(PF_38_66, 0x3D), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VectorALUOp, IR::OP_VSMAX, OpSize::i32Bit>},
  {OPD(PF_38_66, 0x3E), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VectorALUOp, IR::OP_VUMAX, OpSize::i16Bit>},
  {OPD(PF_38_66, 0x3F), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VectorALUOp, IR::OP_VUMAX, OpSize::i32Bit>},
  {OPD(PF_38_66, 0x40), 1, &OpDispatchBuilder::Bind<&OpDispatchBuilder::VectorALUOp, IR::OP_VMUL, OpSize::i32Bit>},
  {OPD(PF_38_66, 0x41), 1, &OpDispatchBuilder::PHMINPOSUWOp},

  {OPD(PF_38_NONE, 0xF0), 2, &OpDispatchBuilder::MOVBEOp},
  {OPD(PF_38_66, 0xF0), 2, &OpDispatchBuilder::MOVBEOp},

  {OPD(PF_38_66, 0xF6), 1, &OpDispatchBuilder::ADXOp},
  {OPD(PF_38_F3, 0xF6), 1, &OpDispatchBuilder::ADXOp},
};
#undef OPD

} // namespace FEXCore::IR
