// SPDX-License-Identifier: MIT
#include <FEXCore/Core/CoreState.h>

#include "Interface/Core/Interpreter/InterpreterOps.h"
#include "Interface/Core/Interpreter/Fallbacks/F80Fallbacks.h"
#include "Interface/Core/Interpreter/Fallbacks/VectorFallbacks.h"

#include <cstddef>
#include <cstdint>

namespace FEXCore::CPU {

template<typename R, typename... Args>
static FallbackInfo GetFallbackInfo(R (*fn)(Args...), FEXCore::Core::FallbackHandlerIndex HandlerIndex) {
  return {FABI_UNKNOWN, HandlerIndex};
}

void InterpreterOps::FillFallbackIndexPointers(Core::FallbackABIInfo* Info, uint64_t* ABIHandlers) {
  Info[Core::OPINDEX_F80CVTTO_4] = {ABIHandlers[FABI_F80_I16_F32_PTR],
                                    reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F80CVTTO>::handle4)};
  Info[Core::OPINDEX_F80CVTTO_8] = {ABIHandlers[FABI_F80_I16_F64_PTR],
                                    reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F80CVTTO>::handle8)};
  Info[Core::OPINDEX_F80CVT_4] = {ABIHandlers[FABI_F32_I16_F80_PTR], reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F80CVT>::handle4)};
  Info[Core::OPINDEX_F80CVT_8] = {ABIHandlers[FABI_F64_I16_F80_PTR], reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F80CVT>::handle8)};
  Info[Core::OPINDEX_F80CVTINT_2] = {ABIHandlers[FABI_I16_I16_F80_PTR],
                                     reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F80CVTINT>::handle2)};
  Info[Core::OPINDEX_F80CVTINT_4] = {ABIHandlers[FABI_I32_I16_F80_PTR],
                                     reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F80CVTINT>::handle4)};
  Info[Core::OPINDEX_F80CVTINT_8] = {ABIHandlers[FABI_I64_I16_F80_PTR],
                                     reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F80CVTINT>::handle8)};
  Info[Core::OPINDEX_F80CVTINT_TRUNC2] = {ABIHandlers[FABI_I16_I16_F80_PTR],
                                          reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F80CVTINT>::handle2t)};
  Info[Core::OPINDEX_F80CVTINT_TRUNC4] = {ABIHandlers[FABI_I32_I16_F80_PTR],
                                          reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F80CVTINT>::handle4t)};
  Info[Core::OPINDEX_F80CVTINT_TRUNC8] = {ABIHandlers[FABI_I64_I16_F80_PTR],
                                          reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F80CVTINT>::handle8t)};
  Info[Core::OPINDEX_F80CMP] = {ABIHandlers[FABI_I64_I16_F80_F80_PTR],
                                reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F80CMP>::handle)};
  Info[Core::OPINDEX_F80CVTTOINT_2] = {ABIHandlers[FABI_F80_I16_I16_PTR],
                                       reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F80CVTTOINT>::handle2)};
  Info[Core::OPINDEX_F80CVTTOINT_4] = {ABIHandlers[FABI_F80_I16_I32_PTR],
                                       reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F80CVTTOINT>::handle4)};

  // Unary
  Info[Core::OPINDEX_F80ROUND] = {ABIHandlers[FABI_F80_I16_F80_PTR],
                                  reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F80ROUND>::handle)};
  Info[Core::OPINDEX_F80F2XM1] = {ABIHandlers[FABI_F80_I16_F80_PTR],
                                  reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F80F2XM1>::handle)};
  Info[Core::OPINDEX_F80TAN] = {ABIHandlers[FABI_F80_I16_F80_PTR], reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F80TAN>::handle)};
  Info[Core::OPINDEX_F80SQRT] = {ABIHandlers[FABI_F80_I16_F80_PTR], reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F80SQRT>::handle)};
  Info[Core::OPINDEX_F80SIN] = {ABIHandlers[FABI_F80_I16_F80_PTR], reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F80SIN>::handle)};
  Info[Core::OPINDEX_F80COS] = {ABIHandlers[FABI_F80_I16_F80_PTR], reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F80COS>::handle)};
  Info[Core::OPINDEX_F80XTRACT_EXP] = {ABIHandlers[FABI_F80_I16_F80_PTR],
                                       reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F80XTRACT_EXP>::handle)};
  Info[Core::OPINDEX_F80XTRACT_SIG] = {ABIHandlers[FABI_F80_I16_F80_PTR],
                                       reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F80XTRACT_SIG>::handle)};
  Info[Core::OPINDEX_F80BCDSTORE] = {ABIHandlers[FABI_F80_I16_F80_PTR],
                                     reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F80BCDSTORE>::handle)};
  Info[Core::OPINDEX_F80BCDLOAD] = {ABIHandlers[FABI_F80_I16_F80_PTR],
                                    reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F80BCDLOAD>::handle)};

  // Binary
  Info[Core::OPINDEX_F80ADD] = {ABIHandlers[FABI_F80_I16_F80_F80_PTR],
                                reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F80ADD>::handle)};
  Info[Core::OPINDEX_F80SUB] = {ABIHandlers[FABI_F80_I16_F80_F80_PTR],
                                reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F80SUB>::handle)};
  Info[Core::OPINDEX_F80MUL] = {ABIHandlers[FABI_F80_I16_F80_F80_PTR],
                                reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F80MUL>::handle)};
  Info[Core::OPINDEX_F80DIV] = {ABIHandlers[FABI_F80_I16_F80_F80_PTR],
                                reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F80DIV>::handle)};
  Info[Core::OPINDEX_F80FYL2X] = {ABIHandlers[FABI_F80_I16_F80_F80_PTR],
                                  reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F80FYL2X>::handle)};
  Info[Core::OPINDEX_F80ATAN] = {ABIHandlers[FABI_F80_I16_F80_F80_PTR],
                                 reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F80ATAN>::handle)};
  Info[Core::OPINDEX_F80FPREM1] = {ABIHandlers[FABI_F80_I16_F80_F80_PTR],
                                   reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F80FPREM1>::handle)};
  Info[Core::OPINDEX_F80FPREM] = {ABIHandlers[FABI_F80_I16_F80_F80_PTR],
                                  reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F80FPREM>::handle)};
  Info[Core::OPINDEX_F80SCALE] = {ABIHandlers[FABI_F80_I16_F80_F80_PTR],
                                  reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F80SCALE>::handle)};

  // Double Precision Unary
  Info[Core::OPINDEX_F64SIN] = {ABIHandlers[FABI_F64_I16_F64_PTR], reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F64SIN>::handle)};
  Info[Core::OPINDEX_F64COS] = {ABIHandlers[FABI_F64_I16_F64_PTR], reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F64COS>::handle)};
  Info[Core::OPINDEX_F64TAN] = {ABIHandlers[FABI_F64_I16_F64_PTR], reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F64TAN>::handle)};
  Info[Core::OPINDEX_F64F2XM1] = {ABIHandlers[FABI_F64_I16_F64_PTR],
                                  reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F64F2XM1>::handle)};

  // Double Precision Binary
  Info[Core::OPINDEX_F64ATAN] = {ABIHandlers[FABI_F64_I16_F64_F64_PTR],
                                 reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F64ATAN>::handle)};
  Info[Core::OPINDEX_F64FPREM] = {ABIHandlers[FABI_F64_I16_F64_F64_PTR],
                                  reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F64FPREM>::handle)};
  Info[Core::OPINDEX_F64FPREM1] = {ABIHandlers[FABI_F64_I16_F64_F64_PTR],
                                   reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F64FPREM1>::handle)};
  Info[Core::OPINDEX_F64FYL2X] = {ABIHandlers[FABI_F64_I16_F64_F64_PTR],
                                  reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F64FYL2X>::handle)};
  Info[Core::OPINDEX_F64SCALE] = {ABIHandlers[FABI_F64_I16_F64_F64_PTR],
                                  reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F64SCALE>::handle)};

  // SSE4.2 string instructions
  Info[Core::OPINDEX_VPCMPESTRX] = {ABIHandlers[FABI_I32_I64_I64_V128_V128_I16],
                                    reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_VPCMPESTRX>::handle)};
  Info[Core::OPINDEX_VPCMPISTRX] = {ABIHandlers[FABI_I32_V128_V128_I16],
                                    reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_VPCMPISTRX>::handle)};
}

bool InterpreterOps::GetFallbackHandler(const IR::IROp_Header* IROp, FallbackInfo* Info) {
  const auto OpSize = IROp->Size;
  switch (IROp->Op) {
  case IR::OP_F80CVTTO: {
    auto Op = IROp->C<IR::IROp_F80CVTTo>();

    switch (Op->SrcSize) {
    case IR::OpSize::i32Bit: {
      *Info = {FABI_F80_I16_F32_PTR, Core::OPINDEX_F80CVTTO_4};
      return true;
    }
    case IR::OpSize::i64Bit: {
      *Info = {FABI_F80_I16_F64_PTR, Core::OPINDEX_F80CVTTO_8};
      return true;
    }
    default: LogMan::Msg::DFmt("Unhandled size: {}", OpSize);
    }
    break;
  }
  case IR::OP_F80CVT: {
    switch (OpSize) {
    case IR::OpSize::i32Bit: {
      *Info = {FABI_F32_I16_F80_PTR, Core::OPINDEX_F80CVT_4};
      return true;
    }
    case IR::OpSize::i64Bit: {
      *Info = {FABI_F64_I16_F80_PTR, Core::OPINDEX_F80CVT_8};
      return true;
    }
    default: LogMan::Msg::DFmt("Unhandled size: {}", OpSize);
    }
    break;
  }
  case IR::OP_F80CVTINT: {
    auto Op = IROp->C<IR::IROp_F80CVTInt>();

    switch (OpSize) {
    case IR::OpSize::i16Bit: {
      if (Op->Truncate) {
        *Info = {FABI_I16_I16_F80_PTR, Core::OPINDEX_F80CVTINT_TRUNC2};
      } else {
        *Info = {FABI_I16_I16_F80_PTR, Core::OPINDEX_F80CVTINT_2};
      }
      return true;
    }
    case IR::OpSize::i32Bit: {
      if (Op->Truncate) {
        *Info = {FABI_I32_I16_F80_PTR, Core::OPINDEX_F80CVTINT_TRUNC4};
      } else {
        *Info = {FABI_I32_I16_F80_PTR, Core::OPINDEX_F80CVTINT_4};
      }
      return true;
    }
    case IR::OpSize::i64Bit: {
      if (Op->Truncate) {
        *Info = {FABI_I64_I16_F80_PTR, Core::OPINDEX_F80CVTINT_TRUNC8};
      } else {
        *Info = {FABI_I64_I16_F80_PTR, Core::OPINDEX_F80CVTINT_8};
      }
      return true;
    }
    default: LogMan::Msg::DFmt("Unhandled size: {}", OpSize);
    }
    break;
  }
  case IR::OP_F80CMP: {
    *Info = {FABI_I64_I16_F80_F80_PTR, (Core::FallbackHandlerIndex)(Core::OPINDEX_F80CMP)};
    return true;
  }

  case IR::OP_F80CVTTOINT: {
    auto Op = IROp->C<IR::IROp_F80CVTToInt>();

    switch (Op->SrcSize) {
    case IR::OpSize::i16Bit: {
      *Info = {FABI_F80_I16_I16_PTR, Core::OPINDEX_F80CVTTOINT_2};
      return true;
    }
    case IR::OpSize::i32Bit: {
      *Info = {FABI_F80_I16_I32_PTR, Core::OPINDEX_F80CVTTOINT_4};
      return true;
    }
    default: LogMan::Msg::DFmt("Unhandled size: {}", OpSize);
    }
    break;
  }

#define COMMON_UNARY_X87_OP(OP)                            \
  case IR::OP_F80##OP: {                                   \
    *Info = {FABI_F80_I16_F80_PTR, Core::OPINDEX_F80##OP}; \
    return true;                                           \
  }

#define COMMON_BINARY_X87_OP(OP)                               \
  case IR::OP_F80##OP: {                                       \
    *Info = {FABI_F80_I16_F80_F80_PTR, Core::OPINDEX_F80##OP}; \
    return true;                                               \
  }

#define COMMON_F64_OP(OP)                                                                              \
  case IR::OP_F64##OP: {                                                                               \
    *Info = GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F64##OP>::handle, Core::OPINDEX_F64##OP); \
    return true;                                                                                       \
  }

#define COMMON_UNARY_F64_OP(OP)                            \
  case IR::OP_F64##OP: {                                   \
    *Info = {FABI_F64_I16_F64_PTR, Core::OPINDEX_F64##OP}; \
    return true;                                           \
  }
#define COMMON_BINARY_F64_OP(OP)                               \
  case IR::OP_F64##OP: {                                       \
    *Info = {FABI_F64_I16_F64_F64_PTR, Core::OPINDEX_F64##OP}; \
    return true;                                               \
  }

    // Unary
    COMMON_UNARY_X87_OP(ROUND)
    COMMON_UNARY_X87_OP(F2XM1)
    COMMON_UNARY_X87_OP(TAN)
    COMMON_UNARY_X87_OP(SQRT)
    COMMON_UNARY_X87_OP(SIN)
    COMMON_UNARY_X87_OP(COS)
    COMMON_UNARY_X87_OP(XTRACT_EXP)
    COMMON_UNARY_X87_OP(XTRACT_SIG)
    COMMON_UNARY_X87_OP(BCDSTORE)
    COMMON_UNARY_X87_OP(BCDLOAD)

    // Binary
    COMMON_BINARY_X87_OP(ADD)
    COMMON_BINARY_X87_OP(SUB)
    COMMON_BINARY_X87_OP(MUL)
    COMMON_BINARY_X87_OP(DIV)
    COMMON_BINARY_X87_OP(FYL2X)
    COMMON_BINARY_X87_OP(ATAN)
    COMMON_BINARY_X87_OP(FPREM1)
    COMMON_BINARY_X87_OP(FPREM)
    COMMON_BINARY_X87_OP(SCALE)

    // Double Precision Unary
    COMMON_UNARY_F64_OP(F2XM1)
    COMMON_UNARY_F64_OP(TAN)
    COMMON_UNARY_F64_OP(SIN)
    COMMON_UNARY_F64_OP(COS)

    // Double Precision Binary
    COMMON_BINARY_F64_OP(FYL2X)
    COMMON_BINARY_F64_OP(ATAN)
    COMMON_BINARY_F64_OP(FPREM1)
    COMMON_BINARY_F64_OP(FPREM)
    COMMON_BINARY_F64_OP(SCALE)

  // SSE4.2 Fallbacks
  case IR::OP_VPCMPESTRX: *Info = {FABI_I32_I64_I64_V128_V128_I16, Core::OPINDEX_VPCMPESTRX}; return true;
  case IR::OP_VPCMPISTRX: *Info = {FABI_I32_V128_V128_I16, Core::OPINDEX_VPCMPISTRX}; return true;

  default: break;
  }

  return false;
}


} // namespace FEXCore::CPU
