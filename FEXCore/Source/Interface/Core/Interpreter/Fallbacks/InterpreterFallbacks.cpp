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
  return {FABI_UNKNOWN, (void*)fn, HandlerIndex, false};
}

template<>
FallbackInfo GetFallbackInfo(double (*fn)(uint16_t, double), FEXCore::Core::FallbackHandlerIndex HandlerIndex) {
  return {FABI_F64_I16_F64, (void*)fn, HandlerIndex, false};
}

template<>
FallbackInfo GetFallbackInfo(double (*fn)(uint16_t, double, double), FEXCore::Core::FallbackHandlerIndex HandlerIndex) {
  return {FABI_F64_I16_F64_F64, (void*)fn, HandlerIndex, false};
}

void InterpreterOps::FillFallbackIndexPointers(uint64_t* Info) {
  Info[Core::OPINDEX_F80CVTTO_4] = reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F80CVTTO>::handle4);
  Info[Core::OPINDEX_F80CVTTO_8] = reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F80CVTTO>::handle8);
  Info[Core::OPINDEX_F80CVT_4] = reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F80CVT>::handle4);
  Info[Core::OPINDEX_F80CVT_8] = reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F80CVT>::handle8);
  Info[Core::OPINDEX_F80CVTINT_2] = reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F80CVTINT>::handle2);
  Info[Core::OPINDEX_F80CVTINT_4] = reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F80CVTINT>::handle4);
  Info[Core::OPINDEX_F80CVTINT_8] = reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F80CVTINT>::handle8);
  Info[Core::OPINDEX_F80CVTINT_TRUNC2] = reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F80CVTINT>::handle2t);
  Info[Core::OPINDEX_F80CVTINT_TRUNC4] = reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F80CVTINT>::handle4t);
  Info[Core::OPINDEX_F80CVTINT_TRUNC8] = reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F80CVTINT>::handle8t);
  Info[Core::OPINDEX_F80CMP] = reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F80CMP>::handle);
  Info[Core::OPINDEX_F80CVTTOINT_2] = reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F80CVTTOINT>::handle2);
  Info[Core::OPINDEX_F80CVTTOINT_4] = reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F80CVTTOINT>::handle4);

  // Unary
  Info[Core::OPINDEX_F80ROUND] = reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F80ROUND>::handle);
  Info[Core::OPINDEX_F80F2XM1] = reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F80F2XM1>::handle);
  Info[Core::OPINDEX_F80TAN] = reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F80TAN>::handle);
  Info[Core::OPINDEX_F80SQRT] = reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F80SQRT>::handle);
  Info[Core::OPINDEX_F80SIN] = reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F80SIN>::handle);
  Info[Core::OPINDEX_F80COS] = reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F80COS>::handle);
  Info[Core::OPINDEX_F80XTRACT_EXP] = reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F80XTRACT_EXP>::handle);
  Info[Core::OPINDEX_F80XTRACT_SIG] = reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F80XTRACT_SIG>::handle);
  Info[Core::OPINDEX_F80BCDSTORE] = reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F80BCDSTORE>::handle);
  Info[Core::OPINDEX_F80BCDLOAD] = reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F80BCDLOAD>::handle);

  // Binary
  Info[Core::OPINDEX_F80ADD] = reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F80ADD>::handle);
  Info[Core::OPINDEX_F80SUB] = reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F80SUB>::handle);
  Info[Core::OPINDEX_F80MUL] = reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F80MUL>::handle);
  Info[Core::OPINDEX_F80DIV] = reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F80DIV>::handle);
  Info[Core::OPINDEX_F80FYL2X] = reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F80FYL2X>::handle);
  Info[Core::OPINDEX_F80ATAN] = reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F80ATAN>::handle);
  Info[Core::OPINDEX_F80FPREM1] = reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F80FPREM1>::handle);
  Info[Core::OPINDEX_F80FPREM] = reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F80FPREM>::handle);
  Info[Core::OPINDEX_F80SCALE] = reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F80SCALE>::handle);

  // Double Precision
  Info[Core::OPINDEX_F64SIN] = reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F64SIN>::handle);
  Info[Core::OPINDEX_F64COS] = reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F64COS>::handle);
  Info[Core::OPINDEX_F64TAN] = reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F64TAN>::handle);
  Info[Core::OPINDEX_F64ATAN] = reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F64ATAN>::handle);
  Info[Core::OPINDEX_F64F2XM1] = reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F64F2XM1>::handle);
  Info[Core::OPINDEX_F64FYL2X] = reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F64FYL2X>::handle);
  Info[Core::OPINDEX_F64FPREM] = reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F64FPREM>::handle);
  Info[Core::OPINDEX_F64FPREM1] = reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F64FPREM1>::handle);
  Info[Core::OPINDEX_F64SCALE] = reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_F64SCALE>::handle);

  // SSE4.2 string instructions
  Info[Core::OPINDEX_VPCMPESTRX] = reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_VPCMPESTRX>::handle);
  Info[Core::OPINDEX_VPCMPISTRX] = reinterpret_cast<uint64_t>(&FEXCore::CPU::OpHandlers<IR::OP_VPCMPISTRX>::handle);
}

bool InterpreterOps::GetFallbackHandler(bool SupportsPreserveAllABI, const IR::IROp_Header* IROp, FallbackInfo* Info) {
  const auto OpSize = IROp->Size;
  switch (IROp->Op) {
  case IR::OP_F80CVTTO: {
    auto Op = IROp->C<IR::IROp_F80CVTTo>();

    switch (Op->SrcSize) {
    case IR::OpSize::i32Bit: {
      *Info = {FABI_F80_I16_F32, (void*)&FEXCore::CPU::OpHandlers<IR::OP_F80CVTTO>::handle4, Core::OPINDEX_F80CVTTO_4, SupportsPreserveAllABI};
      return true;
    }
    case IR::OpSize::i64Bit: {
      *Info = {FABI_F80_I16_F64, (void*)&FEXCore::CPU::OpHandlers<IR::OP_F80CVTTO>::handle8, Core::OPINDEX_F80CVTTO_8, SupportsPreserveAllABI};
      return true;
    }
    default: LogMan::Msg::DFmt("Unhandled size: {}", OpSize);
    }
    break;
  }
  case IR::OP_F80CVT: {
    switch (OpSize) {
    case IR::OpSize::i32Bit: {
      *Info = {FABI_F32_I16_F80, (void*)&FEXCore::CPU::OpHandlers<IR::OP_F80CVT>::handle4, Core::OPINDEX_F80CVT_4, SupportsPreserveAllABI};
      return true;
    }
    case IR::OpSize::i64Bit: {
      *Info = {FABI_F64_I16_F80, (void*)&FEXCore::CPU::OpHandlers<IR::OP_F80CVT>::handle8, Core::OPINDEX_F80CVT_8, SupportsPreserveAllABI};
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
        *Info = {FABI_I16_I16_F80, (void*)&FEXCore::CPU::OpHandlers<IR::OP_F80CVTINT>::handle2t, Core::OPINDEX_F80CVTINT_TRUNC2,
                 SupportsPreserveAllABI};
      } else {
        *Info = {FABI_I16_I16_F80, (void*)&FEXCore::CPU::OpHandlers<IR::OP_F80CVTINT>::handle2, Core::OPINDEX_F80CVTINT_2, SupportsPreserveAllABI};
      }
      return true;
    }
    case IR::OpSize::i32Bit: {
      if (Op->Truncate) {
        *Info = {FABI_I32_I16_F80, (void*)&FEXCore::CPU::OpHandlers<IR::OP_F80CVTINT>::handle4t, Core::OPINDEX_F80CVTINT_TRUNC4,
                 SupportsPreserveAllABI};
      } else {
        *Info = {FABI_I32_I16_F80, (void*)&FEXCore::CPU::OpHandlers<IR::OP_F80CVTINT>::handle4, Core::OPINDEX_F80CVTINT_4, SupportsPreserveAllABI};
      }
      return true;
    }
    case IR::OpSize::i64Bit: {
      if (Op->Truncate) {
        *Info = {FABI_I64_I16_F80, (void*)&FEXCore::CPU::OpHandlers<IR::OP_F80CVTINT>::handle8t, Core::OPINDEX_F80CVTINT_TRUNC8,
                 SupportsPreserveAllABI};
      } else {
        *Info = {FABI_I64_I16_F80, (void*)&FEXCore::CPU::OpHandlers<IR::OP_F80CVTINT>::handle8, Core::OPINDEX_F80CVTINT_8, SupportsPreserveAllABI};
      }
      return true;
    }
    default: LogMan::Msg::DFmt("Unhandled size: {}", OpSize);
    }
    break;
  }
  case IR::OP_F80CMP: {
    *Info = {FABI_I64_I16_F80_F80, (void*)&FEXCore::CPU::OpHandlers<IR::OP_F80CMP>::handle,
             (Core::FallbackHandlerIndex)(Core::OPINDEX_F80CMP), SupportsPreserveAllABI};
    return true;
  }

  case IR::OP_F80CVTTOINT: {
    auto Op = IROp->C<IR::IROp_F80CVTToInt>();

    switch (Op->SrcSize) {
    case IR::OpSize::i16Bit: {
      *Info = {FABI_F80_I16_I16, (void*)&FEXCore::CPU::OpHandlers<IR::OP_F80CVTTOINT>::handle2, Core::OPINDEX_F80CVTTOINT_2, SupportsPreserveAllABI};
      return true;
    }
    case IR::OpSize::i32Bit: {
      *Info = {FABI_F80_I16_I32, (void*)&FEXCore::CPU::OpHandlers<IR::OP_F80CVTTOINT>::handle4, Core::OPINDEX_F80CVTTOINT_4, SupportsPreserveAllABI};
      return true;
    }
    default: LogMan::Msg::DFmt("Unhandled size: {}", OpSize);
    }
    break;
  }

#define COMMON_UNARY_X87_OP(OP)                                                                                                          \
  case IR::OP_F80##OP: {                                                                                                                 \
    *Info = {FABI_F80_I16_F80, (void*)&FEXCore::CPU::OpHandlers<IR::OP_F80##OP>::handle, Core::OPINDEX_F80##OP, SupportsPreserveAllABI}; \
    return true;                                                                                                                         \
  }

#define COMMON_BINARY_X87_OP(OP)                                                                                                             \
  case IR::OP_F80##OP: {                                                                                                                     \
    *Info = {FABI_F80_I16_F80_F80, (void*)&FEXCore::CPU::OpHandlers<IR::OP_F80##OP>::handle, Core::OPINDEX_F80##OP, SupportsPreserveAllABI}; \
    return true;                                                                                                                             \
  }

#define COMMON_F64_OP(OP)                                                                              \
  case IR::OP_F64##OP: {                                                                               \
    *Info = GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F64##OP>::handle, Core::OPINDEX_F64##OP); \
    return true;                                                                                       \
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
    COMMON_F64_OP(F2XM1)
    COMMON_F64_OP(TAN)
    COMMON_F64_OP(SIN)
    COMMON_F64_OP(COS)

    // Double Precision Binary
    COMMON_F64_OP(FYL2X)
    COMMON_F64_OP(ATAN)
    COMMON_F64_OP(FPREM1)
    COMMON_F64_OP(FPREM)
    COMMON_F64_OP(SCALE)

  // SSE4.2 Fallbacks
  case IR::OP_VPCMPESTRX:
    *Info = {FABI_I32_I64_I64_I128_I128_I16, (void*)&FEXCore::CPU::OpHandlers<IR::OP_VPCMPESTRX>::handle, Core::OPINDEX_VPCMPESTRX,
             SupportsPreserveAllABI};
    return true;
  case IR::OP_VPCMPISTRX:
    *Info = {FABI_I32_I128_I128_I16, (void*)&FEXCore::CPU::OpHandlers<IR::OP_VPCMPISTRX>::handle, Core::OPINDEX_VPCMPISTRX, SupportsPreserveAllABI};
    return true;

  default: break;
  }

  return false;
}


} // namespace FEXCore::CPU
