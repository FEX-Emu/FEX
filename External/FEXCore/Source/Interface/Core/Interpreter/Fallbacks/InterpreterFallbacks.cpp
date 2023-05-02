#include <FEXCore/Core/CoreState.h>

#include "Interface/Core/Interpreter/InterpreterOps.h"
#include "Interface/Core/Interpreter/Fallbacks/F80Fallbacks.h"
#include "Interface/Core/Interpreter/Fallbacks/VectorFallbacks.h"

#include <cstddef>
#include <cstdint>

namespace FEXCore::CPU {

template<typename R, typename... Args>
static FallbackInfo GetFallbackInfo(R(*fn)(Args...), FEXCore::Core::FallbackHandlerIndex HandlerIndex) {
  return {FABI_UNKNOWN, (void*)fn, HandlerIndex};
}

template<>
FallbackInfo GetFallbackInfo(X80SoftFloat(*fn)(float), FEXCore::Core::FallbackHandlerIndex HandlerIndex) {
  return {FABI_F80_F32, (void*)fn, HandlerIndex};
}

template<>
FallbackInfo GetFallbackInfo(X80SoftFloat(*fn)(double), FEXCore::Core::FallbackHandlerIndex HandlerIndex) {
  return {FABI_F80_F64, (void*)fn, HandlerIndex};
}

template<>
FallbackInfo GetFallbackInfo(X80SoftFloat(*fn)(int16_t), FEXCore::Core::FallbackHandlerIndex HandlerIndex) {
  return {FABI_F80_I16, (void*)fn, HandlerIndex};
}

template<>
FallbackInfo GetFallbackInfo(void(*fn)(uint16_t), FEXCore::Core::FallbackHandlerIndex HandlerIndex) {
  return {FABI_VOID_U16, (void*)fn, HandlerIndex};
}

template<>
FallbackInfo GetFallbackInfo(X80SoftFloat(*fn)(int32_t), FEXCore::Core::FallbackHandlerIndex HandlerIndex) {
  return {FABI_F80_I32, (void*)fn, HandlerIndex};
}

template<>
FallbackInfo GetFallbackInfo(float(*fn)(X80SoftFloat), FEXCore::Core::FallbackHandlerIndex HandlerIndex) {
  return {FABI_F32_F80, (void*)fn, HandlerIndex};
}

template<>
FallbackInfo GetFallbackInfo(double(*fn)(X80SoftFloat), FEXCore::Core::FallbackHandlerIndex HandlerIndex) {
  return {FABI_F64_F80, (void*)fn, HandlerIndex};
}

template<>
FallbackInfo GetFallbackInfo(double(*fn)(double), FEXCore::Core::FallbackHandlerIndex HandlerIndex) {
  return {FABI_F64_F64, (void*)fn, HandlerIndex};
}

template<>
FallbackInfo GetFallbackInfo(double(*fn)(double,double), FEXCore::Core::FallbackHandlerIndex HandlerIndex) {
  return {FABI_F64_F64_F64, (void*)fn, HandlerIndex};
}

template<>
FallbackInfo GetFallbackInfo(int16_t(*fn)(X80SoftFloat), FEXCore::Core::FallbackHandlerIndex HandlerIndex) {
  return {FABI_I16_F80, (void*)fn, HandlerIndex};
}

template<>
FallbackInfo GetFallbackInfo(int32_t(*fn)(X80SoftFloat), FEXCore::Core::FallbackHandlerIndex HandlerIndex) {
  return {FABI_I32_F80, (void*)fn, HandlerIndex};
}

template<>
FallbackInfo GetFallbackInfo(int64_t(*fn)(X80SoftFloat), FEXCore::Core::FallbackHandlerIndex HandlerIndex) {
  return {FABI_I64_F80, (void*)fn, HandlerIndex};
}

template<>
FallbackInfo GetFallbackInfo(uint64_t(*fn)(X80SoftFloat, X80SoftFloat), FEXCore::Core::FallbackHandlerIndex HandlerIndex) {
  return {FABI_I64_F80_F80, (void*)fn, HandlerIndex};
}

template<>
FallbackInfo GetFallbackInfo(X80SoftFloat(*fn)(X80SoftFloat), FEXCore::Core::FallbackHandlerIndex HandlerIndex) {
  return {FABI_F80_F80, (void*)fn, HandlerIndex};
}

template<>
FallbackInfo GetFallbackInfo(X80SoftFloat(*fn)(X80SoftFloat, X80SoftFloat), FEXCore::Core::FallbackHandlerIndex HandlerIndex) {
  return {FABI_F80_F80_F80, (void*)fn, HandlerIndex};
}

template<>
FallbackInfo GetFallbackInfo(uint32_t(*fn)(uint64_t, uint64_t, __uint128_t, __uint128_t, uint16_t), FEXCore::Core::FallbackHandlerIndex HandlerIndex) {
  return {FABI_I32_I64_I64_I128_I128_I16, (void*)fn, HandlerIndex};
}

template<>
FallbackInfo GetFallbackInfo(uint32_t(*fn)(__uint128_t, __uint128_t, uint16_t), FEXCore::Core::FallbackHandlerIndex HandlerIndex) {
  return {FABI_I32_I128_I128_I16, (void*)fn, HandlerIndex};
}

void InterpreterOps::FillFallbackIndexPointers(uint64_t *Info) {
  Info[Core::OPINDEX_F80LOADFCW] = reinterpret_cast<uint64_t>(GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F80LOADFCW>::handle, Core::OPINDEX_F80LOADFCW).fn);
  Info[Core::OPINDEX_F80CVTTO_4] = reinterpret_cast<uint64_t>(GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F80CVTTO>::handle4, Core::OPINDEX_F80CVTTO_4).fn);
  Info[Core::OPINDEX_F80CVTTO_8] = reinterpret_cast<uint64_t>(GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F80CVTTO>::handle8, Core::OPINDEX_F80CVTTO_8).fn);
  Info[Core::OPINDEX_F80CVT_4] = reinterpret_cast<uint64_t>(GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F80CVT>::handle4, Core::OPINDEX_F80CVT_4).fn);
  Info[Core::OPINDEX_F80CVT_8] = reinterpret_cast<uint64_t>(GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F80CVT>::handle8, Core::OPINDEX_F80CVT_8).fn);
  Info[Core::OPINDEX_F80CVTINT_2] = reinterpret_cast<uint64_t>(GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F80CVTINT>::handle2, Core::OPINDEX_F80CVTINT_2).fn);
  Info[Core::OPINDEX_F80CVTINT_4] = reinterpret_cast<uint64_t>(GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F80CVTINT>::handle4, Core::OPINDEX_F80CVTINT_4).fn);
  Info[Core::OPINDEX_F80CVTINT_8] = reinterpret_cast<uint64_t>(GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F80CVTINT>::handle8, Core::OPINDEX_F80CVTINT_8).fn);
  Info[Core::OPINDEX_F80CVTINT_TRUNC2] = reinterpret_cast<uint64_t>(GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F80CVTINT>::handle2t, Core::OPINDEX_F80CVTINT_TRUNC2).fn);
  Info[Core::OPINDEX_F80CVTINT_TRUNC4] = reinterpret_cast<uint64_t>(GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F80CVTINT>::handle4t, Core::OPINDEX_F80CVTINT_TRUNC4).fn);
  Info[Core::OPINDEX_F80CVTINT_TRUNC8] = reinterpret_cast<uint64_t>(GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F80CVTINT>::handle8t, Core::OPINDEX_F80CVTINT_TRUNC8).fn);
  Info[Core::OPINDEX_F80CMP_0] = reinterpret_cast<uint64_t>(GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F80CMP>::handle<0>, Core::OPINDEX_F80CMP_0).fn);
  Info[Core::OPINDEX_F80CMP_1] = reinterpret_cast<uint64_t>(GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F80CMP>::handle<1>, Core::OPINDEX_F80CMP_1).fn);
  Info[Core::OPINDEX_F80CMP_2] = reinterpret_cast<uint64_t>(GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F80CMP>::handle<2>, Core::OPINDEX_F80CMP_2).fn);
  Info[Core::OPINDEX_F80CMP_3] = reinterpret_cast<uint64_t>(GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F80CMP>::handle<3>, Core::OPINDEX_F80CMP_3).fn);
  Info[Core::OPINDEX_F80CMP_4] = reinterpret_cast<uint64_t>(GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F80CMP>::handle<4>, Core::OPINDEX_F80CMP_4).fn);
  Info[Core::OPINDEX_F80CMP_5] = reinterpret_cast<uint64_t>(GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F80CMP>::handle<5>, Core::OPINDEX_F80CMP_5).fn);
  Info[Core::OPINDEX_F80CMP_6] = reinterpret_cast<uint64_t>(GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F80CMP>::handle<6>, Core::OPINDEX_F80CMP_6).fn);
  Info[Core::OPINDEX_F80CMP_7] = reinterpret_cast<uint64_t>(GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F80CMP>::handle<7>, Core::OPINDEX_F80CMP_7).fn);
  Info[Core::OPINDEX_F80CVTTOINT_2] = reinterpret_cast<uint64_t>(GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F80CVTTOINT>::handle2, Core::OPINDEX_F80CVTTOINT_2).fn);
  Info[Core::OPINDEX_F80CVTTOINT_4] = reinterpret_cast<uint64_t>(GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F80CVTTOINT>::handle4, Core::OPINDEX_F80CVTTOINT_4).fn);

  // Unary
  Info[Core::OPINDEX_F80ROUND] = reinterpret_cast<uint64_t>(GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F80ROUND>::handle, Core::OPINDEX_F80ROUND).fn);
  Info[Core::OPINDEX_F80F2XM1] = reinterpret_cast<uint64_t>(GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F80F2XM1>::handle, Core::OPINDEX_F80F2XM1).fn);
  Info[Core::OPINDEX_F80TAN] = reinterpret_cast<uint64_t>(GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F80TAN>::handle, Core::OPINDEX_F80TAN).fn);
  Info[Core::OPINDEX_F80SQRT] = reinterpret_cast<uint64_t>(GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F80SQRT>::handle, Core::OPINDEX_F80SQRT).fn);
  Info[Core::OPINDEX_F80SIN] = reinterpret_cast<uint64_t>(GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F80SIN>::handle, Core::OPINDEX_F80SIN).fn);
  Info[Core::OPINDEX_F80COS] = reinterpret_cast<uint64_t>(GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F80COS>::handle, Core::OPINDEX_F80COS).fn);
  Info[Core::OPINDEX_F80XTRACT_EXP] = reinterpret_cast<uint64_t>(GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F80XTRACT_EXP>::handle, Core::OPINDEX_F80XTRACT_EXP).fn);
  Info[Core::OPINDEX_F80XTRACT_SIG] = reinterpret_cast<uint64_t>(GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F80XTRACT_SIG>::handle, Core::OPINDEX_F80XTRACT_SIG).fn);
  Info[Core::OPINDEX_F80BCDSTORE] = reinterpret_cast<uint64_t>(GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F80BCDSTORE>::handle, Core::OPINDEX_F80BCDSTORE).fn);
  Info[Core::OPINDEX_F80BCDLOAD] = reinterpret_cast<uint64_t>(GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F80BCDLOAD>::handle, Core::OPINDEX_F80BCDLOAD).fn);

  // Binary
  Info[Core::OPINDEX_F80ADD] = reinterpret_cast<uint64_t>(GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F80ADD>::handle, Core::OPINDEX_F80ADD).fn);
  Info[Core::OPINDEX_F80SUB] = reinterpret_cast<uint64_t>(GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F80SUB>::handle, Core::OPINDEX_F80SUB).fn);
  Info[Core::OPINDEX_F80MUL] = reinterpret_cast<uint64_t>(GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F80MUL>::handle, Core::OPINDEX_F80MUL).fn);
  Info[Core::OPINDEX_F80DIV] = reinterpret_cast<uint64_t>(GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F80DIV>::handle, Core::OPINDEX_F80DIV).fn);
  Info[Core::OPINDEX_F80FYL2X] = reinterpret_cast<uint64_t>(GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F80FYL2X>::handle, Core::OPINDEX_F80FYL2X).fn);
  Info[Core::OPINDEX_F80ATAN] = reinterpret_cast<uint64_t>(GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F80ATAN>::handle, Core::OPINDEX_F80ATAN).fn);
  Info[Core::OPINDEX_F80FPREM1] = reinterpret_cast<uint64_t>(GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F80FPREM1>::handle, Core::OPINDEX_F80FPREM1).fn);
  Info[Core::OPINDEX_F80FPREM] = reinterpret_cast<uint64_t>(GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F80FPREM>::handle, Core::OPINDEX_F80FPREM).fn);
  Info[Core::OPINDEX_F80SCALE] = reinterpret_cast<uint64_t>(GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F80SCALE>::handle, Core::OPINDEX_F80SCALE).fn);

  // Double Precision
  Info[Core::OPINDEX_F64SIN] = reinterpret_cast<uint64_t>(GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F64SIN>::handle, Core::OPINDEX_F64SIN).fn);
  Info[Core::OPINDEX_F64COS] = reinterpret_cast<uint64_t>(GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F64COS>::handle, Core::OPINDEX_F64COS).fn);
  Info[Core::OPINDEX_F64TAN] = reinterpret_cast<uint64_t>(GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F64TAN>::handle, Core::OPINDEX_F64TAN).fn);
  Info[Core::OPINDEX_F64ATAN] = reinterpret_cast<uint64_t>(GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F64ATAN>::handle, Core::OPINDEX_F64ATAN).fn);
  Info[Core::OPINDEX_F64F2XM1] = reinterpret_cast<uint64_t>(GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F64F2XM1>::handle, Core::OPINDEX_F64F2XM1).fn);
  Info[Core::OPINDEX_F64FYL2X] = reinterpret_cast<uint64_t>(GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F64FYL2X>::handle, Core::OPINDEX_F64FYL2X).fn);
  Info[Core::OPINDEX_F64FPREM] = reinterpret_cast<uint64_t>(GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F64FPREM>::handle, Core::OPINDEX_F64FPREM).fn);
  Info[Core::OPINDEX_F64FPREM1] = reinterpret_cast<uint64_t>(GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F64FPREM1>::handle, Core::OPINDEX_F64FPREM1).fn);
  Info[Core::OPINDEX_F64SCALE] = reinterpret_cast<uint64_t>(GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F64SCALE>::handle, Core::OPINDEX_F64SCALE).fn);

  // SSE4.2 string instructions
  Info[Core::OPINDEX_VPCMPESTRX] = reinterpret_cast<uint64_t>(GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_VPCMPESTRX>::handle, Core::OPINDEX_VPCMPESTRX).fn);
  Info[Core::OPINDEX_VPCMPISTRX] = reinterpret_cast<uint64_t>(GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_VPCMPISTRX>::handle, Core::OPINDEX_VPCMPISTRX).fn);
}

bool InterpreterOps::GetFallbackHandler(IR::IROp_Header const *IROp, FallbackInfo *Info) {
  uint8_t OpSize = IROp->Size;
  switch(IROp->Op) {
    case IR::OP_F80LOADFCW: {
      *Info = GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F80LOADFCW>::handle, Core::OPINDEX_F80LOADFCW);
      return true;
    }

    case IR::OP_F80CVTTO: {
      auto Op = IROp->C<IR::IROp_F80CVTTo>();

      switch (Op->SrcSize) {
        case 4: {
          *Info = GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F80CVTTO>::handle4, Core::OPINDEX_F80CVTTO_4);
          return true;
        }
        case 8: {
          *Info = GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F80CVTTO>::handle8, Core::OPINDEX_F80CVTTO_8);
          return true;
        }
      default: LogMan::Msg::DFmt("Unhandled size: {}", OpSize);
      }
      break;
    }
    case IR::OP_F80CVT: {
      switch (OpSize) {
        case 4: {
          *Info = GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F80CVT>::handle4, Core::OPINDEX_F80CVT_4);
          return true;
        }
        case 8: {
          *Info = GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F80CVT>::handle8, Core::OPINDEX_F80CVT_8);
          return true;
        }
        default: LogMan::Msg::DFmt("Unhandled size: {}", OpSize);
      }
      break;
    }
    case IR::OP_F80CVTINT: {
      auto Op = IROp->C<IR::IROp_F80CVTInt>();

      switch (OpSize) {
        case 2: {
          if (Op->Truncate) {
            *Info = GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F80CVTINT>::handle2t, Core::OPINDEX_F80CVTINT_TRUNC2);
          }
          else {
            *Info = GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F80CVTINT>::handle2, Core::OPINDEX_F80CVTINT_2);
          }
          return true;
        }
        case 4: {
          if (Op->Truncate) {
            *Info = GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F80CVTINT>::handle4t, Core::OPINDEX_F80CVTINT_TRUNC4);
          }
          else {
            *Info = GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F80CVTINT>::handle4, Core::OPINDEX_F80CVTINT_4);
          }
          return true;
        }
        case 8: {
          if (Op->Truncate) {
            *Info = GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F80CVTINT>::handle8t, Core::OPINDEX_F80CVTINT_TRUNC8);
          }
          else {
            *Info = GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F80CVTINT>::handle8, Core::OPINDEX_F80CVTINT_8);
          }
          return true;
        }
        default: LogMan::Msg::DFmt("Unhandled size: {}", OpSize);
      }
      break;
    }
    case IR::OP_F80CMP: {
      auto Op = IROp->C<IR::IROp_F80Cmp>();

      static constexpr std::array handlers{
        &FEXCore::CPU::OpHandlers<IR::OP_F80CMP>::handle<0>,
        &FEXCore::CPU::OpHandlers<IR::OP_F80CMP>::handle<1>,
        &FEXCore::CPU::OpHandlers<IR::OP_F80CMP>::handle<2>,
        &FEXCore::CPU::OpHandlers<IR::OP_F80CMP>::handle<3>,
        &FEXCore::CPU::OpHandlers<IR::OP_F80CMP>::handle<4>,
        &FEXCore::CPU::OpHandlers<IR::OP_F80CMP>::handle<5>,
        &FEXCore::CPU::OpHandlers<IR::OP_F80CMP>::handle<6>,
        &FEXCore::CPU::OpHandlers<IR::OP_F80CMP>::handle<7>,
      };

      *Info = GetFallbackInfo(handlers[Op->Flags], (Core::FallbackHandlerIndex)(Core::OPINDEX_F80CMP_0 + Op->Flags));
      return true;
    }

    case IR::OP_F80CVTTOINT: {
      auto Op = IROp->C<IR::IROp_F80CVTToInt>();

      switch (Op->SrcSize) {
        case 2: {
          *Info = GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F80CVTTOINT>::handle2, Core::OPINDEX_F80CVTTOINT_2);
          return true;
        }
        case 4: {
          *Info = GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F80CVTTOINT>::handle4, Core::OPINDEX_F80CVTTOINT_4);
          return true;
        }
        default: LogMan::Msg::DFmt("Unhandled size: {}", OpSize);
      }
      break;
    }

#define COMMON_X87_OP(OP) \
    case IR::OP_F80##OP: { \
      *Info = GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F80##OP>::handle, Core::OPINDEX_F80##OP); \
      return true; \
    }

#define COMMON_F64_OP(OP) \
    case IR::OP_F64##OP: { \
      *Info = GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_F64##OP>::handle, Core::OPINDEX_F64##OP); \
      return true; \
    }

    // Unary
    COMMON_X87_OP(ROUND)
    COMMON_X87_OP(F2XM1)
    COMMON_X87_OP(TAN)
    COMMON_X87_OP(SQRT)
    COMMON_X87_OP(SIN)
    COMMON_X87_OP(COS)
    COMMON_X87_OP(XTRACT_EXP)
    COMMON_X87_OP(XTRACT_SIG)
    COMMON_X87_OP(BCDSTORE)
    COMMON_X87_OP(BCDLOAD)

    // Binary
    COMMON_X87_OP(ADD)
    COMMON_X87_OP(SUB)
    COMMON_X87_OP(MUL)
    COMMON_X87_OP(DIV)
    COMMON_X87_OP(FYL2X)
    COMMON_X87_OP(ATAN)
    COMMON_X87_OP(FPREM1)
    COMMON_X87_OP(FPREM)
    COMMON_X87_OP(SCALE)

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
      *Info = GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_VPCMPESTRX>::handle, Core::OPINDEX_VPCMPESTRX);
      return true;
    case IR::OP_VPCMPISTRX:
      *Info = GetFallbackInfo(&FEXCore::CPU::OpHandlers<IR::OP_VPCMPISTRX>::handle, Core::OPINDEX_VPCMPISTRX);
      return true;

    default:
      break;
  }

  return false;
}


}
