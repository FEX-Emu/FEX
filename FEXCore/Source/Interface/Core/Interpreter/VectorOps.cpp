/*
$info$
tags: backend|interpreter
$end_info$
*/

#include "Interface/Core/Interpreter/InterpreterClass.h"
#include "Interface/Core/Interpreter/InterpreterOps.h"
#include "Interface/Core/Interpreter/InterpreterDefines.h"

#include "Interface/Core/Interpreter/Fallbacks/VectorFallbacks.h"

#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Utils/BitUtils.h>

#include <array>
#include <bit>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace FEXCore::CPU {
#define DEF_OP(x) void InterpreterOps::Op_##x(IR::IROp_Header *IROp, IROpData *Data, IR::NodeID Node)
DEF_OP(VectorZero) {
  uint8_t OpSize = IROp->Size;
  memset(GDP, 0, OpSize);
}

DEF_OP(VectorImm) {
  auto Op = IROp->C<IR::IROp_VectorImm>();
  uint8_t OpSize = IROp->Size;

  TempVectorDataArray Tmp;;

  uint8_t Elements = OpSize / Op->Header.ElementSize;
  uint8_t Imm = Op->Immediate;

  auto Func = [Imm]() { return Imm; };
  switch (Op->Header.ElementSize) {
    DO_VECTOR_0SRC_OP(1, int8_t, Func)
    DO_VECTOR_0SRC_OP(2, int16_t, Func)
    DO_VECTOR_0SRC_OP(4, int32_t, Func)
    DO_VECTOR_0SRC_OP(8, int64_t, Func)
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(LoadNamedVectorConstant) {
  auto Op = IROp->C<IR::IROp_LoadNamedVectorConstant>();
  uint8_t OpSize = IROp->Size;

  switch (Op->Constant) {
    case FEXCore::IR::NamedVectorConstant::NAMED_VECTOR_ZERO:
      memset(GDP, 0, OpSize);
      return;
    default:
      // Intentionally doing nothing.
      break;
  }

  memcpy(GDP, reinterpret_cast<void*>(Data->State->CurrentFrame->Pointers.Common.NamedVectorConstantPointers[Op->Constant]), OpSize);
}

DEF_OP(LoadNamedVectorIndexedConstant) {
  auto Op = IROp->C<IR::IROp_LoadNamedVectorIndexedConstant>();
  uint8_t OpSize = IROp->Size;

  memcpy(GDP, reinterpret_cast<void*>(Data->State->CurrentFrame->Pointers.Common.NamedVectorConstantPointers[Op->Constant] + Op->Index), OpSize);
}

DEF_OP(VMov) {
  auto Op = IROp->C<IR::IROp_VMov>();
  const uint8_t OpSize = IROp->Size;

  LOGMAN_THROW_AA_FMT(OpSize <= Core::CPUState::XMM_AVX_REG_SIZE,
                      "Moves larger than 256-bit aren't supported");

  const auto Src = *GetSrc<InterpVector256*>(Data->SSAData, Op->Source);

  memcpy(GDP, &Src, OpSize);
}

DEF_OP(VAnd) {
  const auto Op = IROp->C<IR::IROp_VAnd>();
  const auto OpSize = IROp->Size;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  if (Is256Bit) {
    const auto Src1 = *GetSrc<InterpVector256*>(Data->SSAData, Op->Vector1);
    const auto Src2 = *GetSrc<InterpVector256*>(Data->SSAData, Op->Vector2);

    const auto Dst = InterpVector256{
      .Lower = Src1.Lower & Src2.Lower,
      .Upper = Src1.Upper & Src2.Upper,
    };

    memcpy(GDP, &Dst, sizeof(Dst));
  } else {
    const auto Src1 = *GetSrc<__uint128_t*>(Data->SSAData, Op->Vector1);
    const auto Src2 = *GetSrc<__uint128_t*>(Data->SSAData, Op->Vector2);

    const auto Dst = Src1 & Src2;
    memcpy(GDP, &Dst, sizeof(Dst));
  }
}

DEF_OP(VBic) {
  const auto Op = IROp->C<IR::IROp_VBic>();
  const auto OpSize = IROp->Size;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  if (Is256Bit) {
    const auto Src1 = *GetSrc<InterpVector256*>(Data->SSAData, Op->Vector1);
    const auto Src2 = *GetSrc<InterpVector256*>(Data->SSAData, Op->Vector2);

    const auto Dst = InterpVector256{
      .Lower = Src1.Lower & ~Src2.Lower,
      .Upper = Src1.Upper & ~Src2.Upper,
    };

    memcpy(GDP, &Dst, sizeof(Dst));
  } else {
    const auto Src1 = *GetSrc<__uint128_t*>(Data->SSAData, Op->Vector1);
    const auto Src2 = *GetSrc<__uint128_t*>(Data->SSAData, Op->Vector2);

    const auto Dst = Src1 & ~Src2;
    memcpy(GDP, &Dst, sizeof(Dst));
  }
}

DEF_OP(VOr) {
  const auto Op = IROp->C<IR::IROp_VOr>();
  const auto OpSize = IROp->Size;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  if (Is256Bit) {
    const auto Src1 = *GetSrc<InterpVector256*>(Data->SSAData, Op->Vector1);
    const auto Src2 = *GetSrc<InterpVector256*>(Data->SSAData, Op->Vector2);

    const auto Dst = InterpVector256{
      .Lower = Src1.Lower | Src2.Lower,
      .Upper = Src1.Upper | Src2.Upper,
    };

    memcpy(GDP, &Dst, sizeof(Dst));
  } else {
    const auto Src1 = *GetSrc<__uint128_t*>(Data->SSAData, Op->Vector1);
    const auto Src2 = *GetSrc<__uint128_t*>(Data->SSAData, Op->Vector2);

    const auto Dst = Src1 | Src2;
    memcpy(GDP, &Dst, sizeof(Dst));
  }
}

DEF_OP(VXor) {
  const auto Op = IROp->C<IR::IROp_VXor>();
  const auto OpSize = IROp->Size;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  if (Is256Bit) {
    const auto Src1 = *GetSrc<InterpVector256*>(Data->SSAData, Op->Vector1);
    const auto Src2 = *GetSrc<InterpVector256*>(Data->SSAData, Op->Vector2);

    const auto Dst = InterpVector256{
      .Lower = Src1.Lower ^ Src2.Lower,
      .Upper = Src1.Upper ^ Src2.Upper,
    };

    memcpy(GDP, &Dst, sizeof(Dst));
  } else {
    const auto Src1 = *GetSrc<__uint128_t*>(Data->SSAData, Op->Vector1);
    const auto Src2 = *GetSrc<__uint128_t*>(Data->SSAData, Op->Vector2);

    const auto Dst = Src1 ^ Src2;
    memcpy(GDP, &Dst, sizeof(Dst));
  }
}

DEF_OP(VAdd) {
  auto Op = IROp->C<IR::IROp_VAdd>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);
  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;

  const auto Func = [](auto a, auto b) { return a + b; };
  switch (ElementSize) {
    DO_VECTOR_OP(1, uint8_t,  Func)
    DO_VECTOR_OP(2, uint16_t, Func)
    DO_VECTOR_OP(4, uint32_t, Func)
    DO_VECTOR_OP(8, uint64_t, Func)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VSub) {
  auto Op = IROp->C<IR::IROp_VSub>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);
  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;

  const auto Func = [](auto a, auto b) { return a - b; };
  switch (ElementSize) {
    DO_VECTOR_OP(1, uint8_t,  Func)
    DO_VECTOR_OP(2, uint16_t, Func)
    DO_VECTOR_OP(4, uint32_t, Func)
    DO_VECTOR_OP(8, uint64_t, Func)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VUQAdd) {
  auto Op = IROp->C<IR::IROp_VUQAdd>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);
  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;

  const auto Func = [](auto a, auto b) {
    decltype(a) res = a + b;
    return res < a ? ~0U : res;
  };
  switch (ElementSize) {
    DO_VECTOR_OP(1, uint8_t,  Func)
    DO_VECTOR_OP(2, uint16_t, Func)
    DO_VECTOR_OP(4, uint32_t, Func)
    DO_VECTOR_OP(8, uint64_t, Func)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VUQSub) {
  auto Op = IROp->C<IR::IROp_VUQSub>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);
  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;

  const auto Func = [](auto a, auto b) {
    decltype(a) res = a - b;
    return res > a ? 0U : res;
  };
  switch (ElementSize) {
    DO_VECTOR_OP(1, uint8_t,  Func)
    DO_VECTOR_OP(2, uint16_t, Func)
    DO_VECTOR_OP(4, uint32_t, Func)
    DO_VECTOR_OP(8, uint64_t, Func)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VSQAdd) {
  auto Op = IROp->C<IR::IROp_VSQAdd>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);
  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;

  const auto Func = [](auto a, auto b) {
    static_assert(std::is_same_v<decltype(a), decltype(b)>);
    using Type = decltype(a);
    using Limits = std::numeric_limits<Type>;

    const Type res = a + b;

    if (a > 0) {
      if (b > (Limits::max() - a)) {
        return Limits::max();
      }
    }
    else if (b < (Limits::min() - a)) {
      return Limits::min();
    }

    return res;
  };
  switch (ElementSize) {
    DO_VECTOR_OP(1, int8_t,  Func)
    DO_VECTOR_OP(2, int16_t, Func)
    DO_VECTOR_OP(4, int32_t, Func)
    DO_VECTOR_OP(8, int64_t, Func)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VSQSub) {
  auto Op = IROp->C<IR::IROp_VSQSub>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);
  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;

  const auto Func = [](auto a, auto b) {
    static_assert(std::is_same_v<decltype(a), decltype(b)>);
    using Type = decltype(a);
    using Limits = std::numeric_limits<Type>;

    const __int128_t res = a - b;
    if (res < Limits::min()) {
      return Limits::min();
    }
    if (res > Limits::max()) {
      return Limits::max();
    }

    return (Type)res;
  };

  switch (ElementSize) {
    DO_VECTOR_OP(1, int8_t,  Func)
    DO_VECTOR_OP(2, int16_t, Func)
    DO_VECTOR_OP(4, int32_t, Func)
    DO_VECTOR_OP(8, int64_t, Func)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VAddP) {
  const auto Op = IROp->C<IR::IROp_VAddP>();
  const auto OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->VectorLower);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->VectorUpper);
  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = (OpSize / ElementSize) / 2;

  const auto Func = [](auto a, auto b) { return a + b; };
  switch (ElementSize) {
    DO_VECTOR_PAIR_OP(1, uint8_t,  Func)
    DO_VECTOR_PAIR_OP(2, uint16_t, Func)
    DO_VECTOR_PAIR_OP(4, uint32_t, Func)
    DO_VECTOR_PAIR_OP(8, uint64_t, Func)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VAddV) {
  const auto Op = IROp->C<IR::IROp_VAddV>();
  const auto OpSize = IROp->Size;

  void *Src = GetSrc<void*>(Data->SSAData, Op->Vector);
  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;

  const auto Func = [](auto current, auto a) { return current + a; };
  switch (ElementSize) {
    DO_VECTOR_REDUCE_1SRC_OP(1, int8_t, Func, 0)
    DO_VECTOR_REDUCE_1SRC_OP(2, int16_t, Func, 0)
    DO_VECTOR_REDUCE_1SRC_OP(4, int32_t, Func, 0)
    DO_VECTOR_REDUCE_1SRC_OP(8, int64_t, Func, 0)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      return;
  }
  memcpy(GDP, Tmp.data(), ElementSize);
}

DEF_OP(VUMinV) {
  auto Op = IROp->C<IR::IROp_VUMinV>();
  const int8_t OpSize = IROp->Size;

  void *Src = GetSrc<void*>(Data->SSAData, Op->Vector);
  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;

  const auto Func = [](auto current, auto a) { return std::min(current, a); };
  switch (ElementSize) {
    DO_VECTOR_REDUCE_1SRC_OP(1, uint8_t, Func, ~0)
    DO_VECTOR_REDUCE_1SRC_OP(2, uint16_t, Func, ~0)
    DO_VECTOR_REDUCE_1SRC_OP(4, uint32_t, Func, ~0U)
    DO_VECTOR_REDUCE_1SRC_OP(8, uint64_t, Func, ~0ULL)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
  memcpy(GDP, Tmp.data(), ElementSize);
}

DEF_OP(VURAvg) {
  const auto Op = IROp->C<IR::IROp_VURAvg>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);
  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;

  const auto Func = [](auto a, auto b) { return (a + b + 1) >> 1; };
  switch (ElementSize) {
    DO_VECTOR_OP(1, uint8_t,  Func)
    DO_VECTOR_OP(2, uint16_t, Func)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VAbs) {
  const auto Op = IROp->C<IR::IROp_VAbs>();
  const uint8_t OpSize = IROp->Size;

  void *Src = GetSrc<void*>(Data->SSAData, Op->Vector);
  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;

  const auto Func = [](auto a) { return std::abs(a); };
  switch (ElementSize) {
    DO_VECTOR_1SRC_OP(1, int8_t, Func)
    DO_VECTOR_1SRC_OP(2, int16_t, Func)
    DO_VECTOR_1SRC_OP(4, int32_t, Func)
    DO_VECTOR_1SRC_OP(8, int64_t, Func)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VPopcount) {
  const auto Op = IROp->C<IR::IROp_VPopcount>();
  const uint8_t OpSize = IROp->Size;

  void *Src = GetSrc<void*>(Data->SSAData, Op->Vector);
  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;

  const auto Func = [](auto a) { return std::popcount(a); };
  switch (ElementSize) {
    DO_VECTOR_1SRC_OP(1, uint8_t, Func)
    DO_VECTOR_1SRC_OP(2, uint16_t, Func)
    DO_VECTOR_1SRC_OP(4, uint32_t, Func)
    DO_VECTOR_1SRC_OP(8, uint64_t, Func)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VFAdd) {
  const auto Op = IROp->C<IR::IROp_VFAdd>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);
  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;

  const auto Func = [](auto a, auto b) { return a + b; };
  switch (ElementSize) {
    DO_VECTOR_OP(4, float, Func)
    DO_VECTOR_OP(8, double, Func)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VFAddP) {
  const auto Op = IROp->C<IR::IROp_VFAddP>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->VectorLower);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->VectorUpper);
  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = (OpSize / ElementSize) / 2;

  const auto Func = [](auto a, auto b) { return a + b; };
  switch (ElementSize) {
    DO_VECTOR_PAIR_OP(4, float, Func)
    DO_VECTOR_PAIR_OP(8, double, Func)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VFSub) {
  const auto Op = IROp->C<IR::IROp_VFSub>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);
  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;

  const auto Func = [](auto a, auto b) { return a - b; };
  switch (ElementSize) {
    DO_VECTOR_OP(4, float, Func)
    DO_VECTOR_OP(8, double, Func)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VFMul) {
  const auto Op = IROp->C<IR::IROp_VFMul>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);
  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;

  const auto Func = [](auto a, auto b) { return a * b; };
  switch (ElementSize) {
    DO_VECTOR_OP(4, float, Func)
    DO_VECTOR_OP(8, double, Func)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VFDiv) {
  const auto Op = IROp->C<IR::IROp_VFDiv>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);
  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;

  const auto Func = [](auto a, auto b) { return a / b; };
  switch (ElementSize) {
    DO_VECTOR_OP(4, float, Func)
    DO_VECTOR_OP(8, double, Func)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VFMin) {
  const auto Op = IROp->C<IR::IROp_VFMin>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);
  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;

  const auto Func = [](auto a, auto b) { return std::min(a, b); };
  switch (ElementSize) {
    DO_VECTOR_OP(4, float, Func)
    DO_VECTOR_OP(8, double, Func)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VFMax) {
  const auto Op = IROp->C<IR::IROp_VFMax>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);
  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;

  const auto Func = [](auto a, auto b) { return std::max(a, b); };
  switch (ElementSize) {
    DO_VECTOR_OP(4, float, Func)
    DO_VECTOR_OP(8, double, Func)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VFRecp) {
  const auto Op = IROp->C<IR::IROp_VFRecp>();
  const uint8_t OpSize = IROp->Size;

  void *Src = GetSrc<void*>(Data->SSAData, Op->Vector);
  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;

  const auto Func = [](auto a) { return 1.0 / a; };
  switch (ElementSize) {
    DO_VECTOR_1SRC_OP(4, float, Func)
    DO_VECTOR_1SRC_OP(8, double, Func)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VFSqrt) {
  const auto Op = IROp->C<IR::IROp_VFSqrt>();
  const uint8_t OpSize = IROp->Size;

  void *Src = GetSrc<void*>(Data->SSAData, Op->Vector);
  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;

  const auto Func = [](auto a) { return std::sqrt(a); };
  switch (ElementSize) {
    DO_VECTOR_1SRC_OP(4, float, Func)
    DO_VECTOR_1SRC_OP(8, double, Func)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VFRSqrt) {
  const auto Op = IROp->C<IR::IROp_VFRSqrt>();
  const uint8_t OpSize = IROp->Size;

  void *Src = GetSrc<void*>(Data->SSAData, Op->Vector);
  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;

  const auto Func = [](auto a) { return 1.0 / std::sqrt(a); };
  switch (ElementSize) {
    DO_VECTOR_1SRC_OP(4, float, Func)
    DO_VECTOR_1SRC_OP(8, double, Func)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VNeg) {
  const auto Op = IROp->C<IR::IROp_VNeg>();
  const uint8_t OpSize = IROp->Size;

  void *Src = GetSrc<void*>(Data->SSAData, Op->Vector);
  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;

  const auto Func = [](auto a) { return -a; };
  switch (Op->Header.ElementSize) {
    DO_VECTOR_1SRC_OP(1, int8_t, Func)
    DO_VECTOR_1SRC_OP(2, int16_t, Func)
    DO_VECTOR_1SRC_OP(4, int32_t, Func)
    DO_VECTOR_1SRC_OP(8, int64_t, Func)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VFNeg) {
  const auto Op = IROp->C<IR::IROp_VFNeg>();
  const uint8_t OpSize = IROp->Size;

  void *Src = GetSrc<void*>(Data->SSAData, Op->Vector);
  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;

  const auto Func = [](auto a) { return -a; };
  switch (ElementSize) {
    DO_VECTOR_1SRC_OP(4, float, Func)
    DO_VECTOR_1SRC_OP(8, double, Func)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VNot) {
  const auto Op = IROp->C<IR::IROp_VNot>();
  const auto Src = *GetSrc<InterpVector256*>(Data->SSAData, Op->Vector);

  const auto Dst = InterpVector256{
    .Lower = ~Src.Lower,
    .Upper = ~Src.Upper,
  };

  memcpy(GDP, &Dst, sizeof(Dst));
}

DEF_OP(VUMin) {
  const auto Op = IROp->C<IR::IROp_VUMin>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);
  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;

  const auto Func = [](auto a, auto b) { return std::min(a, b); };
  switch (ElementSize) {
    DO_VECTOR_OP(1, uint8_t,  Func)
    DO_VECTOR_OP(2, uint16_t, Func)
    DO_VECTOR_OP(4, uint32_t, Func)
    DO_VECTOR_OP(8, uint64_t, Func)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VSMin) {
  const auto Op = IROp->C<IR::IROp_VSMin>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);
  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;

  const auto Func = [](auto a, auto b) { return std::min(a, b); };
  switch (ElementSize) {
    DO_VECTOR_OP(1, int8_t,  Func)
    DO_VECTOR_OP(2, int16_t, Func)
    DO_VECTOR_OP(4, int32_t, Func)
    DO_VECTOR_OP(8, int64_t, Func)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VUMax) {
  const auto Op = IROp->C<IR::IROp_VUMax>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);
  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;

  const auto Func = [](auto a, auto b) { return std::max(a, b); };
  switch (ElementSize) {
    DO_VECTOR_OP(1, uint8_t,  Func)
    DO_VECTOR_OP(2, uint16_t, Func)
    DO_VECTOR_OP(4, uint32_t, Func)
    DO_VECTOR_OP(8, uint64_t, Func)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VSMax) {
  const auto Op = IROp->C<IR::IROp_VSMax>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);
  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;

  const auto Func = [](auto a, auto b) { return std::max(a, b); };
  switch (ElementSize) {
    DO_VECTOR_OP(1, int8_t,  Func)
    DO_VECTOR_OP(2, int16_t, Func)
    DO_VECTOR_OP(4, int32_t, Func)
    DO_VECTOR_OP(8, int64_t, Func)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VZip) {
  const auto Op = IROp->C<IR::IROp_VZip>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->VectorLower);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->VectorUpper);
  TempVectorDataArray Tmp;
  const uint8_t ElementSize = Op->Header.ElementSize;
  uint8_t Elements = OpSize / ElementSize;
  const uint8_t BaseOffset = IROp->Op == IR::OP_VZIP2 ? (Elements / 2) : 0;
  Elements >>= 1;

  switch (ElementSize) {
    case 1: {
      auto *Dst_d  = reinterpret_cast<uint8_t*>(Tmp.data());
      auto *Src1_d = reinterpret_cast<uint8_t*>(Src1);
      auto *Src2_d = reinterpret_cast<uint8_t*>(Src2);
      for (unsigned i = 0; i < Elements; ++i) {
        Dst_d[i*2] = Src1_d[BaseOffset + i];
        Dst_d[i*2+1] = Src2_d[BaseOffset + i];
      }
      break;
    }
    case 2: {
      auto *Dst_d  = reinterpret_cast<uint16_t*>(Tmp.data());
      auto *Src1_d = reinterpret_cast<uint16_t*>(Src1);
      auto *Src2_d = reinterpret_cast<uint16_t*>(Src2);
      for (unsigned i = 0; i < Elements; ++i) {
        Dst_d[i*2] = Src1_d[BaseOffset + i];
        Dst_d[i*2+1] = Src2_d[BaseOffset + i];
      }
      break;
    }
    case 4: {
      auto *Dst_d  = reinterpret_cast<uint32_t*>(Tmp.data());
      auto *Src1_d = reinterpret_cast<uint32_t*>(Src1);
      auto *Src2_d = reinterpret_cast<uint32_t*>(Src2);
      for (unsigned i = 0; i < Elements; ++i) {
        Dst_d[i*2] = Src1_d[BaseOffset + i];
        Dst_d[i*2+1] = Src2_d[BaseOffset + i];
      }
      break;
    }
    case 8: {
      auto *Dst_d  = reinterpret_cast<uint64_t*>(Tmp.data());
      auto *Src1_d = reinterpret_cast<uint64_t*>(Src1);
      auto *Src2_d = reinterpret_cast<uint64_t*>(Src2);
      for (unsigned i = 0; i < Elements; ++i) {
        Dst_d[i*2] = Src1_d[BaseOffset + i];
        Dst_d[i*2+1] = Src2_d[BaseOffset + i];
      }
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }

  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VTrn) {
  const auto Op = IROp->C<IR::IROp_VTrn>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->VectorLower);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->VectorUpper);
  TempVectorDataArray Tmp{};
  const uint8_t ElementSize = Op->Header.ElementSize;
  uint8_t Elements = OpSize / ElementSize;
  const uint8_t BaseOffset = IROp->Op == IR::OP_VTRN2 ? 1 : 0;
  Elements >>= 1;

  switch (ElementSize) {
    case 1: {
      auto *Dst_d  = reinterpret_cast<uint8_t*>(Tmp.data());
      auto *Src1_d = reinterpret_cast<uint8_t*>(Src1);
      auto *Src2_d = reinterpret_cast<uint8_t*>(Src2);
      for (unsigned i = 0; i < Elements; ++i) {
        Dst_d[i*2] = Src1_d[i*2 + BaseOffset];
        Dst_d[i*2+1] = Src2_d[i*2 + BaseOffset];
      }
      break;
    }
    case 2: {
      auto *Dst_d  = reinterpret_cast<uint16_t*>(Tmp.data());
      auto *Src1_d = reinterpret_cast<uint16_t*>(Src1);
      auto *Src2_d = reinterpret_cast<uint16_t*>(Src2);
      for (unsigned i = 0; i < Elements; ++i) {
        Dst_d[i*2] = Src1_d[i*2 + BaseOffset];
        Dst_d[i*2+1] = Src2_d[i*2 + BaseOffset];
      }
      break;
    }
    case 4: {
      auto *Dst_d  = reinterpret_cast<uint32_t*>(Tmp.data());
      auto *Src1_d = reinterpret_cast<uint32_t*>(Src1);
      auto *Src2_d = reinterpret_cast<uint32_t*>(Src2);
      for (unsigned i = 0; i < Elements; ++i) {
        Dst_d[i*2] = Src1_d[i*2 + BaseOffset];
        Dst_d[i*2+1] = Src2_d[i*2 + BaseOffset];
      }
      break;
    }
    case 8: {
      auto *Dst_d  = reinterpret_cast<uint64_t*>(Tmp.data());
      auto *Src1_d = reinterpret_cast<uint64_t*>(Src1);
      auto *Src2_d = reinterpret_cast<uint64_t*>(Src2);
      for (unsigned i = 0; i < Elements; ++i) {
        Dst_d[i*2] = Src1_d[i*2 + BaseOffset];
        Dst_d[i*2+1] = Src2_d[i*2 + BaseOffset];
      }
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }

  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VUnZip) {
  const auto Op = IROp->C<IR::IROp_VUnZip>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->VectorLower);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->VectorUpper);
  TempVectorDataArray Tmp;
  const uint8_t ElementSize = Op->Header.ElementSize;
  uint8_t Elements = OpSize / ElementSize;
  const unsigned Start = IROp->Op == IR::OP_VUNZIP ? 0 : 1;
  Elements >>= 1;

  switch (ElementSize) {
    case 1: {
      auto *Dst_d  = reinterpret_cast<uint8_t*>(Tmp.data());
      auto *Src1_d = reinterpret_cast<uint8_t*>(Src1);
      auto *Src2_d = reinterpret_cast<uint8_t*>(Src2);
      for (unsigned i = 0; i < Elements; ++i) {
        Dst_d[i] = Src1_d[Start + (i * 2)];
        Dst_d[Elements+i] = Src2_d[Start + (i * 2)];
      }
      break;
    }
    case 2: {
      auto *Dst_d  = reinterpret_cast<uint16_t*>(Tmp.data());
      auto *Src1_d = reinterpret_cast<uint16_t*>(Src1);
      auto *Src2_d = reinterpret_cast<uint16_t*>(Src2);
      for (unsigned i = 0; i < Elements; ++i) {
        Dst_d[i] = Src1_d[Start + (i * 2)];
        Dst_d[Elements+i] = Src2_d[Start + (i * 2)];
      }
      break;
    }
    case 4: {
      auto *Dst_d  = reinterpret_cast<uint32_t*>(Tmp.data());
      auto *Src1_d = reinterpret_cast<uint32_t*>(Src1);
      auto *Src2_d = reinterpret_cast<uint32_t*>(Src2);
      for (unsigned i = 0; i < Elements; ++i) {
        Dst_d[i] = Src1_d[Start + (i * 2)];
        Dst_d[Elements+i] = Src2_d[Start + (i * 2)];
      }
      break;
    }
    case 8: {
      auto *Dst_d  = reinterpret_cast<uint64_t*>(Tmp.data());
      auto *Src1_d = reinterpret_cast<uint64_t*>(Src1);
      auto *Src2_d = reinterpret_cast<uint64_t*>(Src2);
      for (unsigned i = 0; i < Elements; ++i) {
        Dst_d[i] = Src1_d[Start + (i * 2)];
        Dst_d[Elements+i] = Src2_d[Start + (i * 2)];
      }
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }

  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VBSL) {
  const auto Op = IROp->C<IR::IROp_VBSL>();
  const auto OpSize = IROp->Size;

  const auto Src1 = *GetSrc<InterpVector256*>(Data->SSAData, Op->VectorMask);
  const auto Src2 = *GetSrc<InterpVector256*>(Data->SSAData, Op->VectorTrue);
  const auto Src3 = *GetSrc<InterpVector256*>(Data->SSAData, Op->VectorFalse);

  const auto Tmp = InterpVector256{
    .Lower = (Src2.Lower & Src1.Lower) | (Src3.Lower & ~Src1.Lower),
    .Upper = (Src2.Upper & Src1.Upper) | (Src3.Upper & ~Src1.Upper),
  };

  memset(GDP, 0, sizeof(InterpVector256));
  memcpy(GDP, &Tmp, OpSize);
}

DEF_OP(VCMPEQ) {
  const auto Op = IROp->C<IR::IROp_VCMPEQ>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);
  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;

  const auto Func = [](auto a, auto b) { return a == b ? ~0ULL : 0; };
  switch (ElementSize) {
    DO_VECTOR_OP(1, uint8_t,   Func)
    DO_VECTOR_OP(2, uint16_t,  Func)
    DO_VECTOR_OP(4, uint32_t,  Func)
    DO_VECTOR_OP(8, uint64_t,  Func)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }

  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VCMPEQZ) {
  const auto Op = IROp->C<IR::IROp_VCMPEQZ>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector);
  uint8_t Src2[Core::CPUState::XMM_AVX_REG_SIZE]{};
  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;

  const auto Func = [](auto a, auto b) { return a == b ? ~0ULL : 0; };
  switch (ElementSize) {
    DO_VECTOR_OP(1, uint8_t,   Func)
    DO_VECTOR_OP(2, uint16_t,  Func)
    DO_VECTOR_OP(4, uint32_t,  Func)
    DO_VECTOR_OP(8, uint64_t,  Func)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }

  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VCMPGT) {
  const auto Op = IROp->C<IR::IROp_VCMPGT>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);
  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;

  const auto Func = [](auto a, auto b) { return a > b ? ~0ULL : 0; };
  switch (ElementSize) {
    DO_VECTOR_OP(1, int8_t,   Func)
    DO_VECTOR_OP(2, int16_t,  Func)
    DO_VECTOR_OP(4, int32_t,  Func)
    DO_VECTOR_OP(8, int64_t,  Func)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }

  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VCMPGTZ) {
  const auto Op = IROp->C<IR::IROp_VCMPGTZ>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector);
  uint8_t Src2[Core::CPUState::XMM_AVX_REG_SIZE]{};
  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;

  const auto Func = [](auto a, auto b) { return a > b ? ~0ULL : 0; };
  switch (ElementSize) {
    DO_VECTOR_OP(1, int8_t,   Func)
    DO_VECTOR_OP(2, int16_t,  Func)
    DO_VECTOR_OP(4, int32_t,  Func)
    DO_VECTOR_OP(8, int64_t,  Func)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }

  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VCMPLTZ) {
  const auto Op = IROp->C<IR::IROp_VCMPLTZ>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector);
  uint8_t Src2[Core::CPUState::XMM_AVX_REG_SIZE]{};
  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;

  const auto Func = [](auto a, auto b) { return a < b ? ~0ULL : 0; };
  switch (ElementSize) {
    DO_VECTOR_OP(1, int8_t,   Func)
    DO_VECTOR_OP(2, int16_t,  Func)
    DO_VECTOR_OP(4, int32_t,  Func)
    DO_VECTOR_OP(8, int64_t,  Func)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }

  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VFCMPEQ) {
  const auto Op = IROp->C<IR::IROp_VFCMPEQ>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);

  const auto Func = [](auto a, auto b) { return a == b ? ~0ULL : 0; };

  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;
  const auto IsScalar = ElementSize == OpSize;

  if (IsScalar) {
    switch (ElementSize) {
    DO_SCALAR_COMPARE_OP(4, float, uint32_t, Func);
    DO_SCALAR_COMPARE_OP(8, double, uint64_t, Func);
    default:
      LOGMAN_MSG_A_FMT("Unsupported elementSize: {}", ElementSize);
      break;
    }
  }
  else {
    switch (ElementSize) {
    DO_VECTOR_COMPARE_OP(4, float, uint32_t, Func);
    DO_VECTOR_COMPARE_OP(8, double, uint64_t, Func);
    default:
      LOGMAN_MSG_A_FMT("Unsupported elementSize: {}", ElementSize);
      break;
    }
  }

  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VFCMPNEQ) {
  const auto Op = IROp->C<IR::IROp_VFCMPNEQ>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);

  const auto Func = [](auto a, auto b) { return a != b ? ~0ULL : 0; };

  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;
  const auto IsScalar = ElementSize == OpSize;

  if (IsScalar) {
    switch (ElementSize) {
    DO_SCALAR_COMPARE_OP(4, float, uint32_t, Func);
    DO_SCALAR_COMPARE_OP(8, double, uint64_t, Func);
    default:
      LOGMAN_MSG_A_FMT("Unsupported element size: {}", ElementSize);
      break;
    }
  }
  else {
    switch (ElementSize) {
    DO_VECTOR_COMPARE_OP(4, float, uint32_t, Func);
    DO_VECTOR_COMPARE_OP(8, double, uint64_t, Func);
    default:
      LOGMAN_MSG_A_FMT("Unsupported element size: {}", ElementSize);
      break;
    }
  }

  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VFCMPLT) {
  const auto Op = IROp->C<IR::IROp_VFCMPLT>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);

  const auto Func = [](auto a, auto b) { return a < b ? ~0ULL : 0; };

  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;
  const auto IsScalar = ElementSize == OpSize;

  if (IsScalar) {
    switch (ElementSize) {
    DO_SCALAR_COMPARE_OP(4, float, uint32_t, Func);
    DO_SCALAR_COMPARE_OP(8, double, uint64_t, Func);
    default:
      LOGMAN_MSG_A_FMT("Unsupported elementSize: {}", ElementSize);
      break;
    }
  }
  else {
    switch (ElementSize) {
    DO_VECTOR_COMPARE_OP(4, float, uint32_t, Func);
    DO_VECTOR_COMPARE_OP(8, double, uint64_t, Func);
    default:
      LOGMAN_MSG_A_FMT("Unsupported elementSize: {}", ElementSize);
      break;
    }
  }

  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VFCMPGT) {
  const auto Op = IROp->C<IR::IROp_VFCMPLT>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);

  const auto Func = [](auto a, auto b) { return a > b ? ~0ULL : 0; };

  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;
  const auto IsScalar = ElementSize == OpSize;

  if (IsScalar) {
    switch (ElementSize) {
    DO_SCALAR_COMPARE_OP(4, float, uint32_t, Func);
    DO_SCALAR_COMPARE_OP(8, double, uint64_t, Func);
    default:
      LOGMAN_MSG_A_FMT("Unsupported elementSize: {}", ElementSize);
      break;
    }
  }
  else {
    switch (ElementSize) {
    DO_VECTOR_COMPARE_OP(4, float, uint32_t, Func);
    DO_VECTOR_COMPARE_OP(8, double, uint64_t, Func);
    default:
      LOGMAN_MSG_A_FMT("Unsupported elementSize: {}", ElementSize);
      break;
    }
  }

  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VFCMPLE) {
  const auto Op = IROp->C<IR::IROp_VFCMPLE>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);

  const auto Func = [](auto a, auto b) { return a <= b ? ~0ULL : 0; };

  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;
  const auto IsScalar = ElementSize == OpSize;

  if (IsScalar) {
    switch (ElementSize) {
    DO_SCALAR_COMPARE_OP(4, float, uint32_t, Func);
    DO_SCALAR_COMPARE_OP(8, double, uint64_t, Func);
    default:
      LOGMAN_MSG_A_FMT("Unsupported elementSize: {}", ElementSize);
      break;
    }
  }
  else {
    switch (ElementSize) {
    DO_VECTOR_COMPARE_OP(4, float, uint32_t, Func);
    DO_VECTOR_COMPARE_OP(8, double, uint64_t, Func);
    default:
      LOGMAN_MSG_A_FMT("Unsupported elementSize: {}", ElementSize);
      break;
    }
  }

  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VFCMPORD) {
  const auto Op = IROp->C<IR::IROp_VFCMPORD>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);

  const auto Func = [](auto a, auto b) { return (!std::isnan(a) && !std::isnan(b)) ? ~0ULL : 0; };

  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;
  const auto IsScalar = ElementSize == OpSize;

  if (IsScalar) {
    switch (ElementSize) {
    DO_SCALAR_COMPARE_OP(4, float, uint32_t, Func);
    DO_SCALAR_COMPARE_OP(8, double, uint64_t, Func);
    default:
      LOGMAN_MSG_A_FMT("Unsupported element size: {}", ElementSize);
      break;
    }
  }
  else {
    switch (ElementSize) {
    DO_VECTOR_COMPARE_OP(4, float, uint32_t, Func);
    DO_VECTOR_COMPARE_OP(8, double, uint64_t, Func);
    default:
      LOGMAN_MSG_A_FMT("Unsupported element size: {}", ElementSize);
      break;
    }
  }

  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VFCMPUNO) {
  const auto Op = IROp->C<IR::IROp_VFCMPUNO>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);

  const auto Func = [](auto a, auto b) { return (std::isnan(a) || std::isnan(b)) ? ~0ULL : 0; };

  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;
  const auto IsScalar = ElementSize == OpSize;

  if (IsScalar) {
    switch (ElementSize) {
    DO_SCALAR_COMPARE_OP(4, float, uint32_t, Func);
    DO_SCALAR_COMPARE_OP(8, double, uint64_t, Func);
    default:
      LOGMAN_MSG_A_FMT("Unsupported element size: {}", ElementSize);
      break;
    }
  }
  else {
    switch (ElementSize) {
    DO_VECTOR_COMPARE_OP(4, float, uint32_t, Func);
    DO_VECTOR_COMPARE_OP(8, double, uint64_t, Func);
    default:
      LOGMAN_MSG_A_FMT("Unsupported element size: {}", ElementSize);
      break;
    }
  }

  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VUShl) {
  const auto Op = IROp->C<IR::IROp_VUShl>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->ShiftVector);
  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;
  const auto Func = [](auto a, auto b) { return b >= (sizeof(a) * 8) ? 0 : a << b; };

  switch (ElementSize) {
    DO_VECTOR_OP(1, uint8_t,  Func)
    DO_VECTOR_OP(2, uint16_t, Func)
    DO_VECTOR_OP(4, uint32_t, Func)
    DO_VECTOR_OP(8, uint64_t, Func)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VUShr) {
  const auto Op = IROp->C<IR::IROp_VUShr>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->ShiftVector);
  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;
  const auto Func = [](auto a, auto b) { return b >= (sizeof(a) * 8) ? 0 : a >> b; };

  switch (ElementSize) {
    DO_VECTOR_OP(1, uint8_t,  Func)
    DO_VECTOR_OP(2, uint16_t, Func)
    DO_VECTOR_OP(4, uint32_t, Func)
    DO_VECTOR_OP(8, uint64_t, Func)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VSShr) {
  const auto Op = IROp->C<IR::IROp_VSShr>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->ShiftVector);
  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;
  const auto Func = [](auto a, auto b) {
    return b >= (sizeof(a) * 8) ? (a >> (sizeof(a) * 8 - 1)) : a >> b;
  };

  switch (ElementSize) {
    DO_VECTOR_OP(1, int8_t,  Func)
    DO_VECTOR_OP(2, int16_t, Func)
    DO_VECTOR_OP(4, int32_t, Func)
    DO_VECTOR_OP(8, int64_t, Func)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VUShlS) {
  const auto Op = IROp->C<IR::IROp_VUShlS>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->ShiftScalar);
  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;
  const auto Func = [](auto a, auto b) { return b >= (sizeof(a) * 8) ? 0 : a << b; };

  switch (ElementSize) {
    DO_VECTOR_SCALAR_OP(1, uint8_t, Func)
    DO_VECTOR_SCALAR_OP(2, uint16_t, Func)
    DO_VECTOR_SCALAR_OP(4, uint32_t, Func)
    DO_VECTOR_SCALAR_OP(8, uint64_t, Func)
    DO_VECTOR_SCALAR_OP(16, __uint128_t, Func)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VUShrS) {
  const auto Op = IROp->C<IR::IROp_VUShrS>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->ShiftScalar);
  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;
  const auto Func = [](auto a, auto b) { return b >= (sizeof(a) * 8) ? 0 : a >> b; };

  switch (ElementSize) {
    DO_VECTOR_SCALAR_OP(1, uint8_t, Func)
    DO_VECTOR_SCALAR_OP(2, uint16_t, Func)
    DO_VECTOR_SCALAR_OP(4, uint32_t, Func)
    DO_VECTOR_SCALAR_OP(8, uint64_t, Func)
    DO_VECTOR_SCALAR_OP(16, __uint128_t, Func)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VSShrS) {
  const auto Op = IROp->C<IR::IROp_VSShrS>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->ShiftScalar);
  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;
  const auto Func = [](auto a, auto b) {
    return b >= (sizeof(a) * 8) ? (a >> (sizeof(a) * 8 - 1)) : a >> b;
  };

  switch (ElementSize) {
    DO_VECTOR_SCALAR_OP(1, int8_t, Func)
    DO_VECTOR_SCALAR_OP(2, int16_t, Func)
    DO_VECTOR_SCALAR_OP(4, int32_t, Func)
    DO_VECTOR_SCALAR_OP(8, int64_t, Func)
    DO_VECTOR_SCALAR_OP(16, __int128_t, Func)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VUShlSWide) {
  const auto Op = IROp->C<IR::IROp_VUShlSWide>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector);
  uint64_t Src2 = *GetSrc<uint64_t*>(Data->SSAData, Op->ShiftScalar);
  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;
  const auto Func = [](auto a, uint64_t b) { return b >= (sizeof(a) * 8) ? 0 : a << b; };

  switch (ElementSize) {
    DO_VECTOR_SCALAR_WIDE_OP(1, uint8_t, Func)
    DO_VECTOR_SCALAR_WIDE_OP(2, uint16_t, Func)
    DO_VECTOR_SCALAR_WIDE_OP(4, uint32_t, Func)
    DO_VECTOR_SCALAR_WIDE_OP(8, uint64_t, Func)
    DO_VECTOR_SCALAR_WIDE_OP(16, __uint128_t, Func)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VUShrSWide) {
  const auto Op = IROp->C<IR::IROp_VUShrS>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector);
  uint64_t Src2 = *GetSrc<uint64_t*>(Data->SSAData, Op->ShiftScalar);
  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;
  const auto Func = [](auto a, uint64_t b) { return b >= (sizeof(a) * 8) ? 0 : a >> b; };

  switch (ElementSize) {
    DO_VECTOR_SCALAR_WIDE_OP(1, uint8_t, Func)
    DO_VECTOR_SCALAR_WIDE_OP(2, uint16_t, Func)
    DO_VECTOR_SCALAR_WIDE_OP(4, uint32_t, Func)
    DO_VECTOR_SCALAR_WIDE_OP(8, uint64_t, Func)
    DO_VECTOR_SCALAR_WIDE_OP(16, __uint128_t, Func)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VSShrSWide) {
  const auto Op = IROp->C<IR::IROp_VSShrS>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector);
  uint64_t Src2 = *GetSrc<uint64_t*>(Data->SSAData, Op->ShiftScalar);
  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;
  const auto Func = [](auto a, uint64_t b) {
    return b >= (sizeof(a) * 8) ? (a >> (sizeof(a) * 8 - 1)) : a >> b;
  };

  switch (ElementSize) {
    DO_VECTOR_SCALAR_WIDE_OP(1, int8_t, Func)
    DO_VECTOR_SCALAR_WIDE_OP(2, int16_t, Func)
    DO_VECTOR_SCALAR_WIDE_OP(4, int32_t, Func)
    DO_VECTOR_SCALAR_WIDE_OP(8, int64_t, Func)
    DO_VECTOR_SCALAR_WIDE_OP(16, __int128_t, Func)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VInsElement) {
  const auto Op = IROp->C<IR::IROp_VInsElement>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->DestVector);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->SrcVector);
  TempVectorDataArray Tmp;

  // Copy src1 in to dest
  memcpy(Tmp.data(), Src1, OpSize);
  switch (ElementSize) {
    case 1: {
      auto *Dst_d  = reinterpret_cast<uint8_t*>(Tmp.data());
      auto *Src2_d = reinterpret_cast<uint8_t*>(Src2);
      Dst_d[Op->DestIdx] = Src2_d[Op->SrcIdx];
      break;
    }
    case 2: {
      auto *Dst_d  = reinterpret_cast<uint16_t*>(Tmp.data());
      auto *Src2_d = reinterpret_cast<uint16_t*>(Src2);
      Dst_d[Op->DestIdx] = Src2_d[Op->SrcIdx];
      break;
    }
    case 4: {
      auto *Dst_d  = reinterpret_cast<uint32_t*>(Tmp.data());
      auto *Src2_d = reinterpret_cast<uint32_t*>(Src2);
      Dst_d[Op->DestIdx] = Src2_d[Op->SrcIdx];
      break;
    }
    case 8: {
      auto *Dst_d  = reinterpret_cast<uint64_t*>(Tmp.data());
      auto *Src2_d = reinterpret_cast<uint64_t*>(Src2);
      Dst_d[Op->DestIdx] = Src2_d[Op->SrcIdx];
      break;
    }
    case 16: {
      auto *Dst_d  = reinterpret_cast<__uint128_t*>(Tmp.data());
      auto *Src2_d = reinterpret_cast<__uint128_t*>(Src2);
      Dst_d[Op->DestIdx] = Src2_d[Op->SrcIdx];
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  };
  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VDupElement) {
  const auto Op = IROp->C<IR::IROp_VDupElement>();
  const uint8_t OpSize = IROp->Size;

  const uint64_t ElementSize = Op->Header.ElementSize;
  const uint64_t ElementSizeBits = ElementSize * 8;
  const uint8_t Elements = OpSize / ElementSize;

  constexpr auto AVXRegSize = Core::CPUState::XMM_AVX_REG_SIZE;
  constexpr auto SSERegSize = Core::CPUState::XMM_SSE_REG_SIZE;
  constexpr auto SSEBitSize = SSERegSize * 8;

  const auto Is128BitElement = ElementSizeBits == SSEBitSize;
  const auto Is256Bit = OpSize == AVXRegSize;

  LOGMAN_THROW_AA_FMT(OpSize <= AVXRegSize,
                      "OpSize is too large for VDupElement: {}", OpSize);

  if (OpSize >= SSERegSize) {
    __uint128_t SourceMask = (1ULL << ElementSizeBits) - 1;
    if (ElementSize == 8) {
      SourceMask = ~0ULL;
    }

    const auto GetResult = [&]() -> __uint128_t {
      const auto Src = *GetSrc<InterpVector256*>(Data->SSAData, Op->Vector);
      uint64_t Shift = ElementSizeBits * Op->Index;

      if (Is128BitElement) {
        if (Shift == 0) {
          return Src.Lower;
        } else {
          return Src.Upper;
        }
      } else {
        // Normalize shift to act on upper uint128_t
        if (Is256Bit && Shift >= SSEBitSize) {
          Shift -= SSEBitSize;
          return (Src.Upper >> Shift) & SourceMask;
        } else {
          return (Src.Lower >> Shift) & SourceMask;
        }
      }
    };

    const __uint128_t Result = GetResult();
    for (size_t i = 0; i < Elements; ++i) {
      auto* Dst = static_cast<uint8_t*>(GDP) + (ElementSize * i);
      memcpy(Dst, &Result, ElementSize);
    }
  } else {
    const uint64_t Shift = ElementSizeBits * Op->Index;
    uint64_t SourceMask = (1ULL << ElementSizeBits) - 1;
    if (ElementSize == 8) {
      SourceMask = ~0ULL;
    }

    const uint64_t Src = *GetSrc<uint64_t*>(Data->SSAData, Op->Vector);
    const uint64_t Result = (Src >> Shift) & SourceMask;
    for (size_t i = 0; i < Elements; ++i) {
      auto* Dst = static_cast<uint8_t*>(GDP) + (ElementSize * i);
      memcpy(Dst, &Result, ElementSize);
    }
  }
}

DEF_OP(VExtr) {
  const auto Op = IROp->C<IR::IROp_VExtr>();
  const auto OpSize = IROp->Size;
  const auto OpSizeBits = OpSize * 8;

  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;
  const auto ElementSize = Op->Header.ElementSize;
  const auto Index = Op->Index;

  if (Is256Bit) {
    const auto ByteIndex = Index * ElementSize;
    const auto IsUpperVectorZero = ByteIndex >= OpSize;
    const auto SanitizedByteIndex = IsUpperVectorZero ? ByteIndex - OpSize
                                                      : ByteIndex;

    const auto Vectors = IsUpperVectorZero
      ?
        std::array<InterpVector256, 2>{
          *GetSrc<InterpVector256*>(Data->SSAData, Op->VectorLower),
          InterpVector256{},
        }
      :
        std::array<InterpVector256, 2>{
          *GetSrc<InterpVector256*>(Data->SSAData, Op->VectorUpper),
          *GetSrc<InterpVector256*>(Data->SSAData, Op->VectorLower),
        };

    const auto* VectorsPtr = reinterpret_cast<const uint8_t*>(Vectors.data());
    const auto* SrcPtr = VectorsPtr + SanitizedByteIndex;
    const auto CopyAmount = std::max(0, int(sizeof(Vectors) - SanitizedByteIndex));

    memcpy(GDP, SrcPtr, CopyAmount);
  } else {
    uint64_t Offset = Index * ElementSize * 8;

    const auto Src1 = *GetSrc<__uint128_t*>(Data->SSAData, Op->VectorLower);
    const auto Src2 = *GetSrc<__uint128_t*>(Data->SSAData, Op->VectorUpper);

    __uint128_t Dst{};
    if (Offset >= OpSizeBits) {
      Offset -= OpSizeBits;
      Dst = Src1 >> Offset;
    } else {
      Dst = (Src1 << (OpSizeBits - Offset)) | (Src2 >> Offset);
    }

    memcpy(GDP, &Dst, OpSize);
  }
}

DEF_OP(VUShrI) {
  const auto Op = IROp->C<IR::IROp_VUShrI>();
  const uint8_t OpSize = IROp->Size;

  void *Src = GetSrc<void*>(Data->SSAData, Op->Vector);
  const uint8_t BitShift = Op->BitShift;
  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;
  const auto Func = [BitShift](auto a) {
    return BitShift >= (sizeof(a) * 8) ? 0 : a >> BitShift;
  };

  switch (ElementSize) {
    DO_VECTOR_1SRC_OP(1, uint8_t, Func)
    DO_VECTOR_1SRC_OP(2, uint16_t, Func)
    DO_VECTOR_1SRC_OP(4, uint32_t, Func)
    DO_VECTOR_1SRC_OP(8, uint64_t, Func)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VSShrI) {
  const auto Op = IROp->C<IR::IROp_VSShrI>();
  const uint8_t OpSize = IROp->Size;

  void *Src = GetSrc<void*>(Data->SSAData, Op->Vector);
  const uint8_t BitShift = Op->BitShift;
  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;
  const auto Func = [BitShift](auto a) {
    return BitShift >= (sizeof(a) * 8) ? (a >> (sizeof(a) * 8 - 1)) : a >> BitShift;
  };

  switch (ElementSize) {
    DO_VECTOR_1SRC_OP(1, int8_t, Func)
    DO_VECTOR_1SRC_OP(2, int16_t, Func)
    DO_VECTOR_1SRC_OP(4, int32_t, Func)
    DO_VECTOR_1SRC_OP(8, int64_t, Func)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VShlI) {
  const auto Op = IROp->C<IR::IROp_VShlI>();
  const uint8_t OpSize = IROp->Size;

  void *Src = GetSrc<void*>(Data->SSAData, Op->Vector);
  const uint8_t BitShift = Op->BitShift;
  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;
  const auto Func = [BitShift](auto a) {
    return BitShift >= (sizeof(a) * 8) ? 0 : (a << BitShift);
  };

  switch (ElementSize) {
    DO_VECTOR_1SRC_OP(1, uint8_t, Func)
    DO_VECTOR_1SRC_OP(2, uint16_t, Func)
    DO_VECTOR_1SRC_OP(4, uint32_t, Func)
    DO_VECTOR_1SRC_OP(8, uint64_t, Func)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VUShrNI) {
  const auto Op = IROp->C<IR::IROp_VUShrNI>();
  const uint8_t OpSize = IROp->Size;

  void *Src = GetSrc<void*>(Data->SSAData, Op->Vector);
  const uint8_t BitShift = Op->BitShift;
  TempVectorDataArray Tmp{};

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / (ElementSize << 1);
  const auto Func = [BitShift](auto a, auto min, auto max) {
    return BitShift >= (sizeof(a) * 8) ? 0 : a >> BitShift;
  };

  switch (ElementSize) {
    DO_VECTOR_1SRC_2TYPE_OP(1, uint8_t, uint16_t, Func, 0, 0)
    DO_VECTOR_1SRC_2TYPE_OP(2, uint16_t, uint32_t, Func, 0, 0)
    DO_VECTOR_1SRC_2TYPE_OP(4, uint32_t, uint64_t, Func, 0, 0)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VUShrNI2) {
  const auto Op = IROp->C<IR::IROp_VUShrNI2>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->VectorLower);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->VectorUpper);
  const uint8_t BitShift = Op->BitShift;
  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / (ElementSize << 1);
  const auto Func = [BitShift](auto a, auto min, auto max) {
    return BitShift >= (sizeof(a) * 8) ? 0 : a >> BitShift;
  };

  switch (ElementSize) {
    DO_VECTOR_1SRC_2TYPE_OP_TOP(1, uint8_t, uint16_t, Func, 0, 0)
    DO_VECTOR_1SRC_2TYPE_OP_TOP(2, uint16_t, uint32_t, Func, 0, 0)
    DO_VECTOR_1SRC_2TYPE_OP_TOP(4, uint32_t, uint64_t, Func, 0, 0)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VSXTL) {
  const auto Op = IROp->C<IR::IROp_VSXTL>();
  const uint8_t OpSize = IROp->Size;

  void *Src = GetSrc<void*>(Data->SSAData, Op->Vector);
  TempVectorDataArray Tmp{};

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;
  const auto Func = [](auto a, auto min, auto max) { return a; };

  switch (ElementSize) {
    DO_VECTOR_1SRC_2TYPE_OP(2, int16_t, int8_t, Func,  0, 0)
    DO_VECTOR_1SRC_2TYPE_OP(4, int32_t, int16_t, Func, 0, 0)
    DO_VECTOR_1SRC_2TYPE_OP(8, int64_t, int32_t, Func, 0, 0)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VSXTL2) {
  const auto Op = IROp->C<IR::IROp_VSXTL2>();
  const uint8_t OpSize = IROp->Size;

  void *Src = GetSrc<void*>(Data->SSAData, Op->Vector);
  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;
  const auto Func = [](auto a, auto min, auto max) { return a; };

  switch (ElementSize) {
    DO_VECTOR_1SRC_2TYPE_OP_TOP_SRC(2, int16_t, int8_t, Func,  0, 0)
    DO_VECTOR_1SRC_2TYPE_OP_TOP_SRC(4, int32_t, int16_t, Func, 0, 0)
    DO_VECTOR_1SRC_2TYPE_OP_TOP_SRC(8, int64_t, int32_t, Func, 0, 0)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VUXTL) {
  const auto Op = IROp->C<IR::IROp_VUXTL>();
  const uint8_t OpSize = IROp->Size;

  void *Src = GetSrc<void*>(Data->SSAData, Op->Vector);
  TempVectorDataArray Tmp{};

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;
  const auto Func = [](auto a, auto min, auto max) { return a; };

  switch (ElementSize) {
    DO_VECTOR_1SRC_2TYPE_OP(2, uint16_t, uint8_t, Func,  0, 0)
    DO_VECTOR_1SRC_2TYPE_OP(4, uint32_t, uint16_t, Func, 0, 0)
    DO_VECTOR_1SRC_2TYPE_OP(8, uint64_t, uint32_t, Func, 0, 0)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VUXTL2) {
  const auto Op = IROp->C<IR::IROp_VUXTL2>();
  const uint8_t OpSize = IROp->Size;

  void *Src = GetSrc<void*>(Data->SSAData, Op->Vector);

  TempVectorDataArray Tmp{};

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;
  const auto Func = [](auto a, auto min, auto max) { return a; };

  switch (ElementSize) {
    DO_VECTOR_1SRC_2TYPE_OP_TOP_SRC(2, uint16_t, uint8_t, Func,  0, 0)
    DO_VECTOR_1SRC_2TYPE_OP_TOP_SRC(4, uint32_t, uint16_t, Func, 0, 0)
    DO_VECTOR_1SRC_2TYPE_OP_TOP_SRC(8, uint64_t, uint32_t, Func, 0, 0)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VSQXTN) {
  const auto Op = IROp->C<IR::IROp_VSQXTN>();
  const uint8_t OpSize = IROp->Size;

  void *Src = GetSrc<void*>(Data->SSAData, Op->Vector);
  TempVectorDataArray Tmp{};

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / (ElementSize << 1);
  const auto Func = [](auto a, auto min, auto max) {
    return std::max(std::min(a, (decltype(a))max), (decltype(a))min);
  };

  switch (ElementSize) {
    DO_VECTOR_1SRC_2TYPE_OP(1, int8_t, int16_t, Func, std::numeric_limits<int8_t>::min(), std::numeric_limits<int8_t>::max())
    DO_VECTOR_1SRC_2TYPE_OP(2, int16_t, int32_t, Func, std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::max())
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VSQXTN2) {
  const auto Op = IROp->C<IR::IROp_VSQXTN2>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->VectorLower);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->VectorUpper);
  TempVectorDataArray Tmp{};

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / (ElementSize << 1);
  const auto Func = [](auto a, auto min, auto max) {
    return std::max(std::min(a, (decltype(a))max), (decltype(a))min);
  };

  switch (ElementSize) {
    DO_VECTOR_1SRC_2TYPE_OP_TOP(1, int8_t, int16_t, Func, std::numeric_limits<int8_t>::min(), std::numeric_limits<int8_t>::max())
    DO_VECTOR_1SRC_2TYPE_OP_TOP(2, int16_t, int32_t, Func, std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::max())
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VSQXTNPair) {
  const auto Op = IROp->C<IR::IROp_VSQXTNPair>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->VectorLower);
  auto Src = Src1;
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->VectorUpper);
  TempVectorDataArray Tmp{};

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / (ElementSize << 1);
  const auto Func = [](auto a, auto min, auto max) {
    return std::max(std::min(a, (decltype(a))max), (decltype(a))min);
  };

  switch (ElementSize) {
    DO_VECTOR_1SRC_2TYPE_OP(1, int8_t, int16_t, Func, std::numeric_limits<int8_t>::min(), std::numeric_limits<int8_t>::max())
    DO_VECTOR_1SRC_2TYPE_OP(2, int16_t, int32_t, Func, std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::max())
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }

  Src = Src2;
  switch (ElementSize) {
    DO_VECTOR_1SRC_2TYPE_OP_TOP_DST(1, int8_t, int16_t, Func, std::numeric_limits<int8_t>::min(), std::numeric_limits<int8_t>::max())
    DO_VECTOR_1SRC_2TYPE_OP_TOP_DST(2, int16_t, int32_t, Func, std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::max())
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VSQXTUN) {
  const auto Op = IROp->C<IR::IROp_VSQXTUN>();
  const uint8_t OpSize = IROp->Size;

  void *Src = GetSrc<void*>(Data->SSAData, Op->Vector);
  TempVectorDataArray Tmp{};

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / (ElementSize << 1);
  const auto Func = [](auto a, auto min, auto max) {
    return std::max(std::min(a, (decltype(a))max), (decltype(a))min);
  };

  switch (ElementSize) {
    DO_VECTOR_1SRC_2TYPE_OP(1, uint8_t, int16_t, Func, 0, (1 << 8) - 1)
    DO_VECTOR_1SRC_2TYPE_OP(2, uint16_t, int32_t, Func, 0, (1 << 16) - 1)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VSQXTUN2) {
  const auto Op = IROp->C<IR::IROp_VSQXTUN2>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->VectorLower);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->VectorUpper);
  TempVectorDataArray Tmp{};

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / (ElementSize << 1);
  const auto Func = [](auto a, auto min, auto max) {
    return std::max(std::min(a, (decltype(a))max), (decltype(a))min);
  };

  switch (ElementSize) {
    DO_VECTOR_1SRC_2TYPE_OP_TOP(1, uint8_t, int16_t, Func, 0, (1 << 8) - 1)
    DO_VECTOR_1SRC_2TYPE_OP_TOP(2, uint16_t, int32_t, Func, 0, (1 << 16) - 1)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VSQXTUNPair) {
  const auto Op = IROp->C<IR::IROp_VSQXTUNPair>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->VectorLower);
  auto Src = Src1;
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->VectorUpper);
  TempVectorDataArray Tmp{};

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / (ElementSize << 1);
  const auto Func = [](auto a, auto min, auto max) {
    return std::max(std::min(a, (decltype(a))max), (decltype(a))min);
  };

  switch (ElementSize) {
    DO_VECTOR_1SRC_2TYPE_OP(1, uint8_t, int16_t, Func, 0, (1 << 8) - 1)
    DO_VECTOR_1SRC_2TYPE_OP(2, uint16_t, int32_t, Func, 0, (1 << 16) - 1)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }

  Src = Src2;
  switch (ElementSize) {
    DO_VECTOR_1SRC_2TYPE_OP_TOP_DST(1, uint8_t, int16_t, Func, 0, (1 << 8) - 1)
    DO_VECTOR_1SRC_2TYPE_OP_TOP_DST(2, uint16_t, int32_t, Func, 0, (1 << 16) - 1)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VUMul) {
  const auto Op = IROp->C<IR::IROp_VUMul>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);
  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;
  const auto Func = [](auto a, auto b) { return a * b; };

  switch (ElementSize) {
    DO_VECTOR_OP(1, uint8_t,  Func)
    DO_VECTOR_OP(2, uint16_t, Func)
    DO_VECTOR_OP(4, uint32_t, Func)
    DO_VECTOR_OP(8, uint64_t, Func)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VUMull) {
  const auto Op = IROp->C<IR::IROp_VUMull>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);

  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;
  const auto Func = [](auto a, auto b) { return a * b; };

  switch (ElementSize) {
    DO_VECTOR_2SRC_2TYPE_OP(2, uint16_t, uint8_t, Func)
    DO_VECTOR_2SRC_2TYPE_OP(4, uint32_t, uint16_t, Func)
    DO_VECTOR_2SRC_2TYPE_OP(8, uint64_t, uint32_t, Func)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VSMul) {
  const auto Op = IROp->C<IR::IROp_VSMul>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);
  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;
  const auto Func = [](auto a, auto b) { return a * b; };

  switch (ElementSize) {
    DO_VECTOR_OP(1, int8_t,  Func)
    DO_VECTOR_OP(2, int16_t, Func)
    DO_VECTOR_OP(4, int32_t, Func)
    DO_VECTOR_OP(8, int64_t, Func)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VSMull) {
  const auto Op = IROp->C<IR::IROp_VSMull>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);

  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;
  const auto Func = [](auto a, auto b) { return a * b; };

  switch (ElementSize) {
    DO_VECTOR_2SRC_2TYPE_OP(2, int16_t, int8_t, Func)
    DO_VECTOR_2SRC_2TYPE_OP(4, int32_t, int16_t, Func)
    DO_VECTOR_2SRC_2TYPE_OP(8, int64_t, int32_t, Func)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VUMull2) {
  const auto Op = IROp->C<IR::IROp_VUMull2>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);

  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;
  const auto Func = [](auto a, auto b) { return a * b; };

  switch (ElementSize) {
    DO_VECTOR_2SRC_2TYPE_OP_TOP_SRC(2, uint16_t, uint8_t, Func)
    DO_VECTOR_2SRC_2TYPE_OP_TOP_SRC(4, uint32_t, uint16_t, Func)
    DO_VECTOR_2SRC_2TYPE_OP_TOP_SRC(8, uint64_t, uint32_t, Func)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VSMull2) {
  const auto Op = IROp->C<IR::IROp_VSMull2>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);

  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;
  const auto Func = [](auto a, auto b) { return a * b; };

  switch (ElementSize) {
    DO_VECTOR_2SRC_2TYPE_OP_TOP_SRC(2, int16_t, int8_t, Func)
    DO_VECTOR_2SRC_2TYPE_OP_TOP_SRC(4, int32_t, int16_t, Func)
    DO_VECTOR_2SRC_2TYPE_OP_TOP_SRC(8, int64_t, int32_t, Func)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
  memcpy(GDP, Tmp.data(), Op->Header.Size);
}

DEF_OP(VUMulH) {
  const auto Op = IROp->C<IR::IROp_VUMul>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);
  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;
  const auto Func = [](auto a, auto b) { return (a * b) >> (sizeof(a) * 8  / 2); };

  switch (ElementSize) {
    DO_VECTOR_OP_WIDE(1, uint8_t,  uint16_t, Func)
    DO_VECTOR_OP_WIDE(2, uint16_t, uint32_t, Func)
    DO_VECTOR_OP_WIDE(4, uint32_t, uint64_t, Func)
    DO_VECTOR_OP_WIDE(8, uint64_t, __uint128_t, Func)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VSMulH) {
  const auto Op = IROp->C<IR::IROp_VSMul>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);
  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;
  const auto Func = [](auto a, auto b) { return (a * b) >> (sizeof(a) * 8  / 2); };

  switch (ElementSize) {
    DO_VECTOR_OP_WIDE(1, int8_t,  int16_t, Func)
    DO_VECTOR_OP_WIDE(2, int16_t, int32_t, Func)
    DO_VECTOR_OP_WIDE(4, int32_t, int64_t, Func)
    DO_VECTOR_OP_WIDE(8, int64_t, __int128_t, Func)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VUABDL) {
  const auto Op = IROp->C<IR::IROp_VUABDL>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);

  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;

  const auto Func8 = [](auto a, auto b) { return std::abs((int16_t)a - (int16_t)b); };
  const auto Func16 = [](auto a, auto b) { return std::abs((int32_t)a - (int32_t)b); };
  const auto Func32 = [](auto a, auto b) { return std::abs((int64_t)a - (int64_t)b); };

  switch (ElementSize) {
    DO_VECTOR_2SRC_2TYPE_OP(2, uint16_t, uint8_t, Func8)
    DO_VECTOR_2SRC_2TYPE_OP(4, uint32_t, uint16_t, Func16)
    DO_VECTOR_2SRC_2TYPE_OP(8, uint64_t, uint32_t, Func32)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VUABDL2) {
  const auto Op = IROp->C<IR::IROp_VUABDL2>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);

  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;

  const auto Func8 = [](auto a, auto b) { return std::abs((int16_t)a - (int16_t)b); };
  const auto Func16 = [](auto a, auto b) { return std::abs((int32_t)a - (int32_t)b); };
  const auto Func32 = [](auto a, auto b) { return std::abs((int64_t)a - (int64_t)b); };

  switch (ElementSize) {
    DO_VECTOR_2SRC_2TYPE_OP_TOP_SRC(2, uint16_t, uint8_t, Func8)
    DO_VECTOR_2SRC_2TYPE_OP_TOP_SRC(4, uint32_t, uint16_t, Func16)
    DO_VECTOR_2SRC_2TYPE_OP_TOP_SRC(8, uint64_t, uint32_t, Func32)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VTBL1) {
  const auto Op = IROp->C<IR::IROp_VTBL1>();
  const uint8_t OpSize = IROp->Size;

  const auto *Src1 = GetSrc<uint8_t*>(Data->SSAData, Op->VectorTable);
  const auto *Src2 = GetSrc<uint8_t*>(Data->SSAData, Op->VectorIndices);

  TempVectorDataArray Tmp;

  for (size_t i = 0; i < OpSize; ++i) {
    const uint8_t Index = Src2[i];
    Tmp[i] = Index >= OpSize ? 0 : Src1[Index];
  }
  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VRev32) {
  const auto Op = IROp->C<IR::IROp_VRev32>();
  const uint8_t OpSize = IROp->Size;

  void *Src = GetSrc<void*>(Data->SSAData, Op->Vector);

  TempVectorDataArray Tmp{};

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / 8;

  // The element working size is always 32-bit
  // The defined element size in the op is the operating size of the element swapping
  const auto Func8 = [](auto a) { return BSwap32(a); };
  const auto Func16 = [](auto a) {
    return (a >> 16) | (a << 16);
  };

  switch (ElementSize) {
    DO_VECTOR_1SRC_OP(1, uint32_t, Func8)
    DO_VECTOR_1SRC_OP(2, uint32_t, Func16)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VRev64) {
  const auto Op = IROp->C<IR::IROp_VRev64>();
  const uint8_t OpSize = IROp->Size;

  void *Src = GetSrc<void*>(Data->SSAData, Op->Vector);

  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / 8;

  // The element working size is always 64-bit
  // The defined element size in the op is the operating size of the element swapping
  const auto Func8 = [](auto a) { return BSwap64(a); };
  const auto Func16 = [](auto a) {
    return (a >> 48) |                    // Element[3] -> Element[0]
      ((a >> 16) & 0xFFFF'0000U) |        // Element[2] -> Element[1]
      ((a << 16) & 0xFFFF'0000'0000ULL) | // Element[1] -> Element[2]
      (a << 48);                          // Element[0] -> Element[3]
  };
  const auto Func32 = [](auto a) {
    return (a >> 32) | (a << 32);
  };

  switch (ElementSize) {
    DO_VECTOR_1SRC_OP(1, uint64_t, Func8)
    DO_VECTOR_1SRC_OP(2, uint64_t, Func16)
    DO_VECTOR_1SRC_OP(4, uint64_t, Func32)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
  memcpy(GDP, Tmp.data(), OpSize);
}

DEF_OP(VPCMPESTRX) {
  const auto Op = IROp->C<IR::IROp_VPCMPESTRX>();
  const auto Control = Op->Control;

  const auto RAX = *GetSrc<uint64_t*>(Data->SSAData, Op->RAX);
  const auto RDX = *GetSrc<uint64_t*>(Data->SSAData, Op->RDX);
  const auto LHS = *GetSrc<__uint128_t*>(Data->SSAData, Op->LHS);
  const auto RHS = *GetSrc<__uint128_t*>(Data->SSAData, Op->RHS);

  const auto Result = OpHandlers<IR::OP_VPCMPESTRX>::handle(RAX, RDX, LHS, RHS, Control);

  memset(GDP, 0, sizeof(uint64_t));
  memcpy(GDP, &Result, sizeof(Result));
}

DEF_OP(VPCMPISTRX) {
  const auto Op = IROp->C<IR::IROp_VPCMPISTRX>();

  const auto LHS = *GetSrc<__uint128_t*>(Data->SSAData, Op->LHS);
  const auto RHS = *GetSrc<__uint128_t*>(Data->SSAData, Op->RHS);
  const auto Control = Op->Control;

  const auto Result = OpHandlers<IR::OP_VPCMPISTRX>::handle(LHS, RHS, Control);

  memset(GDP, 0, sizeof(uint64_t));
  memcpy(GDP, &Result, sizeof(Result));
}

DEF_OP(VFCADD) {
  const auto Op = IROp->C<IR::IROp_VFCADD>();
  const uint8_t OpSize = IROp->Size;

  const auto *Src1 = GetSrc<uint8_t*>(Data->SSAData, Op->Vector1);
  const auto *Src2 = GetSrc<uint8_t*>(Data->SSAData, Op->Vector2);
  const auto Rotate = Op->Rotate;
  LOGMAN_THROW_A_FMT(Rotate == 90 || Rotate == 270, "Invalid rotate!");

  TempVectorDataArray Tmp;

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;

  const auto Func = [Rotate](auto dst, auto src1, auto src2) {
    auto Element1 = src2[1];
    auto Element3 = src2[0];
    if (Rotate == 90) {
      Element1 = -Element1;
    }
    else {
      Element3 = -Element3;
    }
    dst[0] = src1[0] + Element1;
    dst[1] = src1[1] + Element3;
  };

  switch (ElementSize) {
    //DO_VECTOR_FCADD_PAIR_OP(2, float16_t, Func)
    DO_VECTOR_FCADD_PAIR_OP(4, float, Func)
    DO_VECTOR_FCADD_PAIR_OP(8, double, Func)
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
  memcpy(GDP, Tmp.data(), OpSize);
}

#undef DEF_OP

} // namespace FEXCore::CPU
