/*
$info$
tags: backend|interpreter
$end_info$
*/

#include "Interface/Core/Interpreter/InterpreterClass.h"
#include "Interface/Core/Interpreter/InterpreterOps.h"
#include "Interface/Core/Interpreter/InterpreterDefines.h"
#include <FEXCore/Utils/BitUtils.h>

#include <bit>
#include <cstdint>

namespace FEXCore::CPU {
#define DEF_OP(x) void InterpreterOps::Op_##x(IR::IROp_Header *IROp, IROpData *Data, IR::NodeID Node)
DEF_OP(VectorZero) {
  uint8_t OpSize = IROp->Size;
  memset(GDP, 0, OpSize);
}

DEF_OP(VectorImm) {
  auto Op = IROp->C<IR::IROp_VectorImm>();
  uint8_t OpSize = IROp->Size;

  uint8_t Tmp[32];

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
  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(SplatVector) {
  auto Op = IROp->C<IR::IROp_SplatVector2>();
  uint8_t OpSize = IROp->Size;

  LOGMAN_THROW_AA_FMT(OpSize <= 16, "Can't handle a vector of size: {}", OpSize);
  void *Src = GetSrc<void*>(Data->SSAData, Op->Scalar);
  uint8_t Tmp[16];
  uint8_t Elements = 0;

  switch (Op->Header.Op) {
    case IR::OP_SPLATVECTOR4: Elements = 4; break;
    case IR::OP_SPLATVECTOR2: Elements = 2; break;
    default: LOGMAN_MSG_A_FMT("Uknown Splat size"); break;
  }

  #define CREATE_VECTOR(elementsize, type) \
    case elementsize: { \
      auto *Dst_d = reinterpret_cast<type*>(Tmp); \
      auto *Src_d = reinterpret_cast<type*>(Src); \
      for (uint8_t i = 0; i < Elements; ++i) \
        Dst_d[i] = *Src_d;\
      break; \
    }
  uint8_t ElementSize = OpSize / Elements;
  switch (ElementSize) {
    CREATE_VECTOR(1, uint8_t)
    CREATE_VECTOR(2, uint16_t)
    CREATE_VECTOR(4, uint32_t)
    CREATE_VECTOR(8, uint64_t)
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.Size); break;
  }
  #undef CREATE_VECTOR
  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VMov) {
  auto Op = IROp->C<IR::IROp_VMov>();
  const uint8_t OpSize = IROp->Size;

  const auto Src = *GetSrc<__uint128_t*>(Data->SSAData, Op->Source);

  memcpy(GDP, &Src, OpSize);
}

DEF_OP(VAnd) {
  auto Op = IROp->C<IR::IROp_VAnd>();
  const auto Src1 = *GetSrc<__uint128_t*>(Data->SSAData, Op->Vector1);
  const auto Src2 = *GetSrc<__uint128_t*>(Data->SSAData, Op->Vector2);

  const auto Dst = Src1 & Src2;
  memcpy(GDP, &Dst, 16);
}

DEF_OP(VBic) {
  auto Op = IROp->C<IR::IROp_VBic>();
  const auto Src1 = *GetSrc<__uint128_t*>(Data->SSAData, Op->Vector1);
  const auto Src2 = *GetSrc<__uint128_t*>(Data->SSAData, Op->Vector2);

  const auto Dst = Src1 & ~Src2;
  memcpy(GDP, &Dst, 16);
}

DEF_OP(VOr) {
  auto Op = IROp->C<IR::IROp_VOr>();
  const auto Src1 = *GetSrc<__uint128_t*>(Data->SSAData, Op->Vector1);
  const auto Src2 = *GetSrc<__uint128_t*>(Data->SSAData, Op->Vector2);

  const auto Dst = Src1 | Src2;
  memcpy(GDP, &Dst, 16);
}

DEF_OP(VXor) {
  auto Op = IROp->C<IR::IROp_VXor>();
  const auto Src1 = *GetSrc<__uint128_t*>(Data->SSAData, Op->Vector1);
  const auto Src2 = *GetSrc<__uint128_t*>(Data->SSAData, Op->Vector2);

  const auto Dst = Src1 ^ Src2;
  memcpy(GDP, &Dst, 16);
}

DEF_OP(VAdd) {
  auto Op = IROp->C<IR::IROp_VAdd>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);
  uint8_t Tmp[16];

  const uint8_t Elements = OpSize / Op->Header.ElementSize;

  const auto Func = [](auto a, auto b) { return a + b; };
  switch (Op->Header.ElementSize) {
    DO_VECTOR_OP(1, uint8_t,  Func)
    DO_VECTOR_OP(2, uint16_t, Func)
    DO_VECTOR_OP(4, uint32_t, Func)
    DO_VECTOR_OP(8, uint64_t, Func)
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VSub) {
  auto Op = IROp->C<IR::IROp_VSub>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);
  uint8_t Tmp[16];

  const uint8_t Elements = OpSize / Op->Header.ElementSize;

  const auto Func = [](auto a, auto b) { return a - b; };
  switch (Op->Header.ElementSize) {
    DO_VECTOR_OP(1, uint8_t,  Func)
    DO_VECTOR_OP(2, uint16_t, Func)
    DO_VECTOR_OP(4, uint32_t, Func)
    DO_VECTOR_OP(8, uint64_t, Func)
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VUQAdd) {
  auto Op = IROp->C<IR::IROp_VUQAdd>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);
  uint8_t Tmp[16];

  const uint8_t Elements = OpSize / Op->Header.ElementSize;

  const auto Func = [](auto a, auto b) {
    decltype(a) res = a + b;
    return res < a ? ~0U : res;
  };
  switch (Op->Header.ElementSize) {
    DO_VECTOR_OP(1, uint8_t,  Func)
    DO_VECTOR_OP(2, uint16_t, Func)
    DO_VECTOR_OP(4, uint32_t, Func)
    DO_VECTOR_OP(8, uint64_t, Func)
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VUQSub) {
  auto Op = IROp->C<IR::IROp_VUQSub>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);
  uint8_t Tmp[16];

  const uint8_t Elements = OpSize / Op->Header.ElementSize;

  const auto Func = [](auto a, auto b) {
    decltype(a) res = a - b;
    return res > a ? 0U : res;
  };
  switch (Op->Header.ElementSize) {
    DO_VECTOR_OP(1, uint8_t,  Func)
    DO_VECTOR_OP(2, uint16_t, Func)
    DO_VECTOR_OP(4, uint32_t, Func)
    DO_VECTOR_OP(8, uint64_t, Func)
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VSQAdd) {
  auto Op = IROp->C<IR::IROp_VSQAdd>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);
  uint8_t Tmp[16];

  const uint8_t Elements = OpSize / Op->Header.ElementSize;

  const auto Func = [](auto a, auto b) {
    decltype(a) res = a + b;

    if (a > 0) {
      if (b > (std::numeric_limits<decltype(a)>::max() - a)) {
        return std::numeric_limits<decltype(a)>::max();
      }
    }
    else if (b < (std::numeric_limits<decltype(a)>::min() - a)) {
      return std::numeric_limits<decltype(a)>::min();
    }

    return res;
  };
  switch (Op->Header.ElementSize) {
    DO_VECTOR_OP(1, int8_t,  Func)
    DO_VECTOR_OP(2, int16_t, Func)
    DO_VECTOR_OP(4, int32_t, Func)
    DO_VECTOR_OP(8, int64_t, Func)
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VSQSub) {
  auto Op = IROp->C<IR::IROp_VSQSub>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);
  uint8_t Tmp[16];

  const uint8_t Elements = OpSize / Op->Header.ElementSize;

  const auto Func = [](auto a, auto b) {
    __int128_t res = a - b;
    if (res < std::numeric_limits<decltype(a)>::min())
      return std::numeric_limits<decltype(a)>::min();

    if (res > std::numeric_limits<decltype(a)>::max())
      return std::numeric_limits<decltype(a)>::max();
    return (decltype(a))res;
  };

  switch (Op->Header.ElementSize) {
    DO_VECTOR_OP(1, int8_t,  Func)
    DO_VECTOR_OP(2, int16_t, Func)
    DO_VECTOR_OP(4, int32_t, Func)
    DO_VECTOR_OP(8, int64_t, Func)
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VAddP) {
  auto Op = IROp->C<IR::IROp_VAddP>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->VectorLower);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->VectorUpper);
  uint8_t Tmp[16];

  const uint8_t Elements = (OpSize / Op->Header.ElementSize) / 2;

  const auto Func = [](auto a, auto b) { return a + b; };
  switch (Op->Header.ElementSize) {
    DO_VECTOR_PAIR_OP(1, uint8_t,  Func)
    DO_VECTOR_PAIR_OP(2, uint16_t, Func)
    DO_VECTOR_PAIR_OP(4, uint32_t, Func)
    DO_VECTOR_PAIR_OP(8, uint64_t, Func)
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VAddV) {
  auto Op = IROp->C<IR::IROp_VAddV>();
  const uint8_t OpSize = IROp->Size;

  void *Src = GetSrc<void*>(Data->SSAData, Op->Vector);
  uint8_t Tmp[16];

  const uint8_t Elements = OpSize / Op->Header.ElementSize;

  const auto Func = [](auto current, auto a) { return current + a; };
  switch (Op->Header.ElementSize) {
    DO_VECTOR_REDUCE_1SRC_OP(1, int8_t, Func, 0)
    DO_VECTOR_REDUCE_1SRC_OP(2, int16_t, Func, 0)
    DO_VECTOR_REDUCE_1SRC_OP(4, int32_t, Func, 0)
    DO_VECTOR_REDUCE_1SRC_OP(8, int64_t, Func, 0)
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
  memcpy(GDP, Tmp, Op->Header.ElementSize);
}

DEF_OP(VUMinV) {
  auto Op = IROp->C<IR::IROp_VUMinV>();
  const int8_t OpSize = IROp->Size;

  void *Src = GetSrc<void*>(Data->SSAData, Op->Vector);
  uint8_t Tmp[16];

  const uint8_t Elements = OpSize / Op->Header.ElementSize;

  const auto Func = [](auto current, auto a) { return std::min(current, a); };
  switch (Op->Header.ElementSize) {
    DO_VECTOR_REDUCE_1SRC_OP(1, uint8_t, Func, ~0)
    DO_VECTOR_REDUCE_1SRC_OP(2, uint16_t, Func, ~0)
    DO_VECTOR_REDUCE_1SRC_OP(4, uint32_t, Func, ~0U)
    DO_VECTOR_REDUCE_1SRC_OP(8, uint64_t, Func, ~0ULL)
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
  memcpy(GDP, Tmp, Op->Header.ElementSize);
}

DEF_OP(VURAvg) {
  auto Op = IROp->C<IR::IROp_VURAvg>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);
  uint8_t Tmp[16];

  const uint8_t Elements = OpSize / Op->Header.ElementSize;

  const auto Func = [](auto a, auto b) { return (a + b + 1) >> 1; };
  switch (Op->Header.ElementSize) {
    DO_VECTOR_OP(1, uint8_t,  Func)
    DO_VECTOR_OP(2, uint16_t, Func)
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VAbs) {
  auto Op = IROp->C<IR::IROp_VAbs>();
  const uint8_t OpSize = IROp->Size;

  void *Src = GetSrc<void*>(Data->SSAData, Op->Vector);
  uint8_t Tmp[16];

  const uint8_t Elements = OpSize / Op->Header.ElementSize;

  const auto Func = [](auto a) { return std::abs(a); };
  switch (Op->Header.ElementSize) {
    DO_VECTOR_1SRC_OP(1, int8_t, Func)
    DO_VECTOR_1SRC_OP(2, int16_t, Func)
    DO_VECTOR_1SRC_OP(4, int32_t, Func)
    DO_VECTOR_1SRC_OP(8, int64_t, Func)
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VPopcount) {
  auto Op = IROp->C<IR::IROp_VPopcount>();
  const uint8_t OpSize = IROp->Size;

  void *Src = GetSrc<void*>(Data->SSAData, Op->Vector);
  uint8_t Tmp[16];

  const uint8_t Elements = OpSize / Op->Header.ElementSize;

  const auto Func = [](auto a) { return std::popcount(a); };
  switch (Op->Header.ElementSize) {
    DO_VECTOR_1SRC_OP(1, uint8_t, Func)
    DO_VECTOR_1SRC_OP(2, uint16_t, Func)
    DO_VECTOR_1SRC_OP(4, uint32_t, Func)
    DO_VECTOR_1SRC_OP(8, uint64_t, Func)
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VFAdd) {
  auto Op = IROp->C<IR::IROp_VFAdd>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);
  uint8_t Tmp[16];

  const uint8_t Elements = OpSize / Op->Header.ElementSize;

  const auto Func = [](auto a, auto b) { return a + b; };
  switch (Op->Header.ElementSize) {
    DO_VECTOR_OP(4, float, Func)
    DO_VECTOR_OP(8, double, Func)
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VFAddP) {
  auto Op = IROp->C<IR::IROp_VFAddP>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->VectorLower);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->VectorUpper);
  uint8_t Tmp[16];

  const uint8_t Elements = (OpSize / Op->Header.ElementSize) / 2;

  const auto Func = [](auto a, auto b) { return a + b; };
  switch (Op->Header.ElementSize) {
    DO_VECTOR_PAIR_OP(4, float, Func)
    DO_VECTOR_PAIR_OP(8, double, Func)
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VFSub) {
  auto Op = IROp->C<IR::IROp_VFSub>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);
  uint8_t Tmp[16];

  const uint8_t Elements = OpSize / Op->Header.ElementSize;

  const auto Func = [](auto a, auto b) { return a - b; };
  switch (Op->Header.ElementSize) {
    DO_VECTOR_OP(4, float, Func)
    DO_VECTOR_OP(8, double, Func)
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VFMul) {
  auto Op = IROp->C<IR::IROp_VFMul>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);
  uint8_t Tmp[16];

  const uint8_t Elements = OpSize / Op->Header.ElementSize;

  const auto Func = [](auto a, auto b) { return a * b; };
  switch (Op->Header.ElementSize) {
    DO_VECTOR_OP(4, float, Func)
    DO_VECTOR_OP(8, double, Func)
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VFDiv) {
  auto Op = IROp->C<IR::IROp_VFDiv>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);
  uint8_t Tmp[16];

  const uint8_t Elements = OpSize / Op->Header.ElementSize;

  const auto Func = [](auto a, auto b) { return a / b; };
  switch (Op->Header.ElementSize) {
    DO_VECTOR_OP(4, float, Func)
    DO_VECTOR_OP(8, double, Func)
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VFMin) {
  auto Op = IROp->C<IR::IROp_VFMin>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);
  uint8_t Tmp[16];

  const uint8_t Elements = OpSize / Op->Header.ElementSize;

  const auto Func = [](auto a, auto b) { return std::min(a, b); };
  switch (Op->Header.ElementSize) {
    DO_VECTOR_OP(4, float, Func)
    DO_VECTOR_OP(8, double, Func)
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VFMax) {
  auto Op = IROp->C<IR::IROp_VFMax>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);
  uint8_t Tmp[16];

  const uint8_t Elements = OpSize / Op->Header.ElementSize;

  const auto Func = [](auto a, auto b) { return std::max(a, b); };
  switch (Op->Header.ElementSize) {
    DO_VECTOR_OP(4, float, Func)
    DO_VECTOR_OP(8, double, Func)
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VFRecp) {
  auto Op = IROp->C<IR::IROp_VFRecp>();
  const uint8_t OpSize = IROp->Size;

  void *Src = GetSrc<void*>(Data->SSAData, Op->Vector);
  uint8_t Tmp[16];

  const uint8_t Elements = OpSize / Op->Header.ElementSize;

  const auto Func = [](auto a) { return 1.0 / a; };
  switch (Op->Header.ElementSize) {
    DO_VECTOR_1SRC_OP(4, float, Func)
    DO_VECTOR_1SRC_OP(8, double, Func)
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VFSqrt) {
  auto Op = IROp->C<IR::IROp_VFSqrt>();
  const uint8_t OpSize = IROp->Size;

  void *Src = GetSrc<void*>(Data->SSAData, Op->Vector);
  uint8_t Tmp[16];

  const uint8_t Elements = OpSize / Op->Header.ElementSize;

  const auto Func = [](auto a) { return std::sqrt(a); };
  switch (Op->Header.ElementSize) {
    DO_VECTOR_1SRC_OP(4, float, Func)
    DO_VECTOR_1SRC_OP(8, double, Func)
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VFRSqrt) {
  auto Op = IROp->C<IR::IROp_VFRSqrt>();
  const uint8_t OpSize = IROp->Size;

  void *Src = GetSrc<void*>(Data->SSAData, Op->Vector);
  uint8_t Tmp[16];

  const uint8_t Elements = OpSize / Op->Header.ElementSize;

  const auto Func = [](auto a) { return 1.0 / std::sqrt(a); };
  switch (Op->Header.ElementSize) {
    DO_VECTOR_1SRC_OP(4, float, Func)
    DO_VECTOR_1SRC_OP(8, double, Func)
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VNeg) {
  auto Op = IROp->C<IR::IROp_VNeg>();
  const uint8_t OpSize = IROp->Size;

  void *Src = GetSrc<void*>(Data->SSAData, Op->Vector);
  uint8_t Tmp[16];

  const uint8_t Elements = OpSize / Op->Header.ElementSize;

  const auto Func = [](auto a) { return -a; };
  switch (Op->Header.ElementSize) {
    DO_VECTOR_1SRC_OP(1, int8_t, Func)
    DO_VECTOR_1SRC_OP(2, int16_t, Func)
    DO_VECTOR_1SRC_OP(4, int32_t, Func)
    DO_VECTOR_1SRC_OP(8, int64_t, Func)
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VFNeg) {
  auto Op = IROp->C<IR::IROp_VFNeg>();
  const uint8_t OpSize = IROp->Size;

  void *Src = GetSrc<void*>(Data->SSAData, Op->Vector);
  uint8_t Tmp[16];

  const uint8_t Elements = OpSize / Op->Header.ElementSize;

  const auto Func = [](auto a) { return -a; };
  switch (Op->Header.ElementSize) {
    DO_VECTOR_1SRC_OP(4, float, Func)
    DO_VECTOR_1SRC_OP(8, double, Func)
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VNot) {
  auto Op = IROp->C<IR::IROp_VNot>();
  const auto Src1 = *GetSrc<__uint128_t*>(Data->SSAData, Op->Vector);

  const auto Dst = ~Src1;
  memcpy(GDP, &Dst, 16);
}

DEF_OP(VUMin) {
  auto Op = IROp->C<IR::IROp_VUMin>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);
  uint8_t Tmp[16];

  const uint8_t Elements = OpSize / Op->Header.ElementSize;
  const auto Func = [](auto a, auto b) { return std::min(a, b); };

  switch (Op->Header.ElementSize) {
    DO_VECTOR_OP(1, uint8_t,  Func)
    DO_VECTOR_OP(2, uint16_t, Func)
    DO_VECTOR_OP(4, uint32_t, Func)
    DO_VECTOR_OP(8, uint64_t, Func)
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VSMin) {
  auto Op = IROp->C<IR::IROp_VSMin>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);
  uint8_t Tmp[16];

  const uint8_t Elements = OpSize / Op->Header.ElementSize;
  const auto Func = [](auto a, auto b) { return std::min(a, b); };

  switch (Op->Header.ElementSize) {
    DO_VECTOR_OP(1, int8_t,  Func)
    DO_VECTOR_OP(2, int16_t, Func)
    DO_VECTOR_OP(4, int32_t, Func)
    DO_VECTOR_OP(8, int64_t, Func)
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VUMax) {
  auto Op = IROp->C<IR::IROp_VUMax>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);
  uint8_t Tmp[16];

  const uint8_t Elements = OpSize / Op->Header.ElementSize;
  const auto Func = [](auto a, auto b) { return std::max(a, b); };

  switch (Op->Header.ElementSize) {
    DO_VECTOR_OP(1, uint8_t,  Func)
    DO_VECTOR_OP(2, uint16_t, Func)
    DO_VECTOR_OP(4, uint32_t, Func)
    DO_VECTOR_OP(8, uint64_t, Func)
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VSMax) {
  auto Op = IROp->C<IR::IROp_VSMax>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);
  uint8_t Tmp[16];

  const uint8_t Elements = OpSize / Op->Header.ElementSize;
  const auto Func = [](auto a, auto b) { return std::max(a, b); };

  switch (Op->Header.ElementSize) {
    DO_VECTOR_OP(1, int8_t,  Func)
    DO_VECTOR_OP(2, int16_t, Func)
    DO_VECTOR_OP(4, int32_t, Func)
    DO_VECTOR_OP(8, int64_t, Func)
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VZip) {
  auto Op = IROp->C<IR::IROp_VZip>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->VectorLower);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->VectorUpper);
  uint8_t Tmp[16];
  uint8_t Elements = OpSize / Op->Header.ElementSize;
  uint8_t BaseOffset = IROp->Op == IR::OP_VZIP2 ? (Elements / 2) : 0;
  Elements >>= 1;

  switch (Op->Header.ElementSize) {
    case 1: {
      auto *Dst_d  = reinterpret_cast<uint8_t*>(Tmp);
      auto *Src1_d = reinterpret_cast<uint8_t*>(Src1);
      auto *Src2_d = reinterpret_cast<uint8_t*>(Src2);
      for (unsigned i = 0; i < Elements; ++i) {
        Dst_d[i*2] = Src1_d[BaseOffset + i];
        Dst_d[i*2+1] = Src2_d[BaseOffset + i];
      }
      break;
    }
    case 2: {
      auto *Dst_d  = reinterpret_cast<uint16_t*>(Tmp);
      auto *Src1_d = reinterpret_cast<uint16_t*>(Src1);
      auto *Src2_d = reinterpret_cast<uint16_t*>(Src2);
      for (unsigned i = 0; i < Elements; ++i) {
        Dst_d[i*2] = Src1_d[BaseOffset + i];
        Dst_d[i*2+1] = Src2_d[BaseOffset + i];
      }
      break;
    }
    case 4: {
      auto *Dst_d  = reinterpret_cast<uint32_t*>(Tmp);
      auto *Src1_d = reinterpret_cast<uint32_t*>(Src1);
      auto *Src2_d = reinterpret_cast<uint32_t*>(Src2);
      for (unsigned i = 0; i < Elements; ++i) {
        Dst_d[i*2] = Src1_d[BaseOffset + i];
        Dst_d[i*2+1] = Src2_d[BaseOffset + i];
      }
      break;
    }
    case 8: {
      auto *Dst_d  = reinterpret_cast<uint64_t*>(Tmp);
      auto *Src1_d = reinterpret_cast<uint64_t*>(Src1);
      auto *Src2_d = reinterpret_cast<uint64_t*>(Src2);
      for (unsigned i = 0; i < Elements; ++i) {
        Dst_d[i*2] = Src1_d[BaseOffset + i];
        Dst_d[i*2+1] = Src2_d[BaseOffset + i];
      }
      break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }

  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VUnZip) {
  auto Op = IROp->C<IR::IROp_VUnZip>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->VectorLower);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->VectorUpper);
  uint8_t Tmp[16];
  uint8_t Elements = OpSize / Op->Header.ElementSize;
  unsigned Start = IROp->Op == IR::OP_VUNZIP ? 0 : 1;
  Elements >>= 1;

  switch (Op->Header.ElementSize) {
    case 1: {
      auto *Dst_d  = reinterpret_cast<uint8_t*>(Tmp);
      auto *Src1_d = reinterpret_cast<uint8_t*>(Src1);
      auto *Src2_d = reinterpret_cast<uint8_t*>(Src2);
      for (unsigned i = 0; i < Elements; ++i) {
        Dst_d[i] = Src1_d[Start + (i * 2)];
        Dst_d[Elements+i] = Src2_d[Start + (i * 2)];
      }
      break;
    }
    case 2: {
      auto *Dst_d  = reinterpret_cast<uint16_t*>(Tmp);
      auto *Src1_d = reinterpret_cast<uint16_t*>(Src1);
      auto *Src2_d = reinterpret_cast<uint16_t*>(Src2);
      for (unsigned i = 0; i < Elements; ++i) {
        Dst_d[i] = Src1_d[Start + (i * 2)];
        Dst_d[Elements+i] = Src2_d[Start + (i * 2)];
      }
      break;
    }
    case 4: {
      auto *Dst_d  = reinterpret_cast<uint32_t*>(Tmp);
      auto *Src1_d = reinterpret_cast<uint32_t*>(Src1);
      auto *Src2_d = reinterpret_cast<uint32_t*>(Src2);
      for (unsigned i = 0; i < Elements; ++i) {
        Dst_d[i] = Src1_d[Start + (i * 2)];
        Dst_d[Elements+i] = Src2_d[Start + (i * 2)];
      }
      break;
    }
    case 8: {
      auto *Dst_d  = reinterpret_cast<uint64_t*>(Tmp);
      auto *Src1_d = reinterpret_cast<uint64_t*>(Src1);
      auto *Src2_d = reinterpret_cast<uint64_t*>(Src2);
      for (unsigned i = 0; i < Elements; ++i) {
        Dst_d[i] = Src1_d[Start + (i * 2)];
        Dst_d[Elements+i] = Src2_d[Start + (i * 2)];
      }
      break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }

  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VBSL) {
  auto Op = IROp->C<IR::IROp_VBSL>();
  const auto Src1 = *GetSrc<__uint128_t*>(Data->SSAData, Op->VectorMask);
  const auto Src2 = *GetSrc<__uint128_t*>(Data->SSAData, Op->VectorTrue);
  const auto Src3 = *GetSrc<__uint128_t*>(Data->SSAData, Op->VectorFalse);

  __uint128_t Tmp{};
  Tmp = Src2 & Src1;
  Tmp |= Src3 & ~Src1;

  memcpy(GDP, &Tmp, 16);
}

DEF_OP(VCMPEQ) {
  auto Op = IROp->C<IR::IROp_VCMPEQ>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);
  uint8_t Tmp[16];

  const uint8_t Elements = OpSize / Op->Header.ElementSize;
  const auto Func = [](auto a, auto b) { return a == b ? ~0ULL : 0; };

  switch (Op->Header.ElementSize) {
    DO_VECTOR_OP(1, uint8_t,   Func)
    DO_VECTOR_OP(2, uint16_t,  Func)
    DO_VECTOR_OP(4, uint32_t,  Func)
    DO_VECTOR_OP(8, uint64_t,  Func)
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }

  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VCMPEQZ) {
  auto Op = IROp->C<IR::IROp_VCMPEQZ>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector);
  uint8_t Src2[16]{};
  uint8_t Tmp[16];

  const uint8_t Elements = OpSize / Op->Header.ElementSize;
  const auto Func = [](auto a, auto b) { return a == b ? ~0ULL : 0; };

  switch (Op->Header.ElementSize) {
    DO_VECTOR_OP(1, uint8_t,   Func)
    DO_VECTOR_OP(2, uint16_t,  Func)
    DO_VECTOR_OP(4, uint32_t,  Func)
    DO_VECTOR_OP(8, uint64_t,  Func)
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }

  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VCMPGT) {
  auto Op = IROp->C<IR::IROp_VCMPGT>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);
  uint8_t Tmp[16];

  const uint8_t Elements = OpSize / Op->Header.ElementSize;
  const auto Func = [](auto a, auto b) { return a > b ? ~0ULL : 0; };

  switch (Op->Header.ElementSize) {
    DO_VECTOR_OP(1, int8_t,   Func)
    DO_VECTOR_OP(2, int16_t,  Func)
    DO_VECTOR_OP(4, int32_t,  Func)
    DO_VECTOR_OP(8, int64_t,  Func)
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }

  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VCMPGTZ) {
  auto Op = IROp->C<IR::IROp_VCMPGTZ>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector);
  uint8_t Src2[16]{};
  uint8_t Tmp[16];

  const uint8_t Elements = OpSize / Op->Header.ElementSize;
  const auto Func = [](auto a, auto b) { return a > b ? ~0ULL : 0; };

  switch (Op->Header.ElementSize) {
    DO_VECTOR_OP(1, int8_t,   Func)
    DO_VECTOR_OP(2, int16_t,  Func)
    DO_VECTOR_OP(4, int32_t,  Func)
    DO_VECTOR_OP(8, int64_t,  Func)
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }

  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VCMPLTZ) {
  auto Op = IROp->C<IR::IROp_VCMPLTZ>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector);
  uint8_t Src2[16]{};
  uint8_t Tmp[16];

  const uint8_t Elements = OpSize / Op->Header.ElementSize;
  const auto Func = [](auto a, auto b) { return a < b ? ~0ULL : 0; };

  switch (Op->Header.ElementSize) {
    DO_VECTOR_OP(1, int8_t,   Func)
    DO_VECTOR_OP(2, int16_t,  Func)
    DO_VECTOR_OP(4, int32_t,  Func)
    DO_VECTOR_OP(8, int64_t,  Func)
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }

  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VFCMPEQ) {
  auto Op = IROp->C<IR::IROp_VFCMPEQ>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);

  const auto Func = [](auto a, auto b) { return a == b ? ~0ULL : 0; };

  uint8_t Tmp[16];
  const uint8_t Elements = OpSize / Op->Header.ElementSize;

  if (Op->Header.ElementSize == OpSize) {
    switch (Op->Header.ElementSize) {
    DO_SCALAR_COMPARE_OP(4, float, uint32_t, Func);
    DO_SCALAR_COMPARE_OP(8, double, uint64_t, Func);
    default: LOGMAN_MSG_A_FMT("Unsupported elementSize: {}", Op->Header.ElementSize);
    }
  }
  else {
    switch (Op->Header.ElementSize) {
    DO_VECTOR_COMPARE_OP(4, float, uint32_t, Func);
    DO_VECTOR_COMPARE_OP(8, double, uint64_t, Func);
    default: LOGMAN_MSG_A_FMT("Unsupported elementSize: {}", Op->Header.ElementSize);
    }
  }

  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VFCMPNEQ) {
  auto Op = IROp->C<IR::IROp_VFCMPNEQ>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);

  const auto Func = [](auto a, auto b) { return a != b ? ~0ULL : 0; };

  uint8_t Tmp[16];
  const uint8_t Elements = OpSize / Op->Header.ElementSize;

  if (Op->Header.ElementSize == OpSize) {
    switch (Op->Header.ElementSize) {
    DO_SCALAR_COMPARE_OP(4, float, uint32_t, Func);
    DO_SCALAR_COMPARE_OP(8, double, uint64_t, Func);
    default: LOGMAN_MSG_A_FMT("Unsupported elementSize: {}", Op->Header.ElementSize);
    }
  }
  else {
    switch (Op->Header.ElementSize) {
    DO_VECTOR_COMPARE_OP(4, float, uint32_t, Func);
    DO_VECTOR_COMPARE_OP(8, double, uint64_t, Func);
    default: LOGMAN_MSG_A_FMT("Unsupported elementSize: {}", Op->Header.ElementSize);
    }
  }

  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VFCMPLT) {
  auto Op = IROp->C<IR::IROp_VFCMPLT>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);

  const auto Func = [](auto a, auto b) { return a < b ? ~0ULL : 0; };

  uint8_t Tmp[16];
  const uint8_t Elements = OpSize / Op->Header.ElementSize;

  if (Op->Header.ElementSize == OpSize) {
    switch (Op->Header.ElementSize) {
    DO_SCALAR_COMPARE_OP(4, float, uint32_t, Func);
    DO_SCALAR_COMPARE_OP(8, double, uint64_t, Func);
    default: LOGMAN_MSG_A_FMT("Unsupported elementSize: {}", Op->Header.ElementSize);
    }
  }
  else {
    switch (Op->Header.ElementSize) {
    DO_VECTOR_COMPARE_OP(4, float, uint32_t, Func);
    DO_VECTOR_COMPARE_OP(8, double, uint64_t, Func);
    default: LOGMAN_MSG_A_FMT("Unsupported elementSize: {}", Op->Header.ElementSize);
    }
  }

  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VFCMPGT) {
  auto Op = IROp->C<IR::IROp_VFCMPLT>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);

  const auto Func = [](auto a, auto b) { return a > b ? ~0ULL : 0; };

  uint8_t Tmp[16];
  const uint8_t Elements = OpSize / Op->Header.ElementSize;

  if (Op->Header.ElementSize == OpSize) {
    switch (Op->Header.ElementSize) {
    DO_SCALAR_COMPARE_OP(4, float, uint32_t, Func);
    DO_SCALAR_COMPARE_OP(8, double, uint64_t, Func);
    default: LOGMAN_MSG_A_FMT("Unsupported elementSize: {}", Op->Header.ElementSize);
    }
  }
  else {
    switch (Op->Header.ElementSize) {
    DO_VECTOR_COMPARE_OP(4, float, uint32_t, Func);
    DO_VECTOR_COMPARE_OP(8, double, uint64_t, Func);
    default: LOGMAN_MSG_A_FMT("Unsupported elementSize: {}", Op->Header.ElementSize);
    }
  }

  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VFCMPLE) {
  auto Op = IROp->C<IR::IROp_VFCMPLE>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);

  const auto Func = [](auto a, auto b) { return a <= b ? ~0ULL : 0; };

  uint8_t Tmp[16];
  const uint8_t Elements = OpSize / Op->Header.ElementSize;

  if (Op->Header.ElementSize == OpSize) {
    switch (Op->Header.ElementSize) {
    DO_SCALAR_COMPARE_OP(4, float, uint32_t, Func);
    DO_SCALAR_COMPARE_OP(8, double, uint64_t, Func);
    default: LOGMAN_MSG_A_FMT("Unsupported elementSize: {}", Op->Header.ElementSize);
    }
  }
  else {
    switch (Op->Header.ElementSize) {
    DO_VECTOR_COMPARE_OP(4, float, uint32_t, Func);
    DO_VECTOR_COMPARE_OP(8, double, uint64_t, Func);
    default: LOGMAN_MSG_A_FMT("Unsupported elementSize: {}", Op->Header.ElementSize);
    }
  }

  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VFCMPORD) {
  auto Op = IROp->C<IR::IROp_VFCMPORD>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);

  const auto Func = [](auto a, auto b) { return (!std::isnan(a) && !std::isnan(b)) ? ~0ULL : 0; };

  uint8_t Tmp[16];
  const uint8_t Elements = OpSize / Op->Header.ElementSize;

  if (Op->Header.ElementSize == OpSize) {
    switch (Op->Header.ElementSize) {
    DO_SCALAR_COMPARE_OP(4, float, uint32_t, Func);
    DO_SCALAR_COMPARE_OP(8, double, uint64_t, Func);
    default: LOGMAN_MSG_A_FMT("Unsupported elementSize: {}", Op->Header.ElementSize);
    }
  }
  else {
    switch (Op->Header.ElementSize) {
    DO_VECTOR_COMPARE_OP(4, float, uint32_t, Func);
    DO_VECTOR_COMPARE_OP(8, double, uint64_t, Func);
    default: LOGMAN_MSG_A_FMT("Unsupported elementSize: {}", Op->Header.ElementSize);
    }
  }

  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VFCMPUNO) {
  auto Op = IROp->C<IR::IROp_VFCMPUNO>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);

  const auto Func = [](auto a, auto b) { return (std::isnan(a) || std::isnan(b)) ? ~0ULL : 0; };

  uint8_t Tmp[16];
  const uint8_t Elements = OpSize / Op->Header.ElementSize;

  if (Op->Header.ElementSize == OpSize) {
    switch (Op->Header.ElementSize) {
    DO_SCALAR_COMPARE_OP(4, float, uint32_t, Func);
    DO_SCALAR_COMPARE_OP(8, double, uint64_t, Func);
    default: LOGMAN_MSG_A_FMT("Unsupported elementSize: {}", Op->Header.ElementSize);
    }
  }
  else {
    switch (Op->Header.ElementSize) {
    DO_VECTOR_COMPARE_OP(4, float, uint32_t, Func);
    DO_VECTOR_COMPARE_OP(8, double, uint64_t, Func);
    default: LOGMAN_MSG_A_FMT("Unsupported elementSize: {}", Op->Header.ElementSize);
    }
  }

  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VUShl) {
  auto Op = IROp->C<IR::IROp_VUShl>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->ShiftVector);
  uint8_t Tmp[16];

  const uint8_t Elements = OpSize / Op->Header.ElementSize;
  const auto Func = [](auto a, auto b) { return b >= (sizeof(a) * 8) ? 0 : a << b; };

  switch (Op->Header.ElementSize) {
    DO_VECTOR_OP(1, uint8_t,  Func)
    DO_VECTOR_OP(2, uint16_t, Func)
    DO_VECTOR_OP(4, uint32_t, Func)
    DO_VECTOR_OP(8, uint64_t, Func)
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VUShr) {
  auto Op = IROp->C<IR::IROp_VUShr>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->ShiftVector);
  uint8_t Tmp[16];

  const uint8_t Elements = OpSize / Op->Header.ElementSize;
  const auto Func = [](auto a, auto b) { return b >= (sizeof(a) * 8) ? 0 : a >> b; };

  switch (Op->Header.ElementSize) {
    DO_VECTOR_OP(1, uint8_t,  Func)
    DO_VECTOR_OP(2, uint16_t, Func)
    DO_VECTOR_OP(4, uint32_t, Func)
    DO_VECTOR_OP(8, uint64_t, Func)
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VSShr) {
  auto Op = IROp->C<IR::IROp_VSShr>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->ShiftVector);
  uint8_t Tmp[16];

  const uint8_t Elements = OpSize / Op->Header.ElementSize;
  const auto Func = [](auto a, auto b) {
    return b >= (sizeof(a) * 8) ? (a >> (sizeof(a) * 8 - 1)) : a >> b;
  };

  switch (Op->Header.ElementSize) {
    DO_VECTOR_OP(1, int8_t,  Func)
    DO_VECTOR_OP(2, int16_t, Func)
    DO_VECTOR_OP(4, int32_t, Func)
    DO_VECTOR_OP(8, int64_t, Func)
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VUShlS) {
  auto Op = IROp->C<IR::IROp_VUShlS>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->ShiftScalar);
  uint8_t Tmp[16];

  const uint8_t Elements = OpSize / Op->Header.ElementSize;
  const auto Func = [](auto a, auto b) { return b >= (sizeof(a) * 8) ? 0 : a << b; };

  switch (Op->Header.ElementSize) {
    DO_VECTOR_SCALAR_OP(1, uint8_t, Func)
    DO_VECTOR_SCALAR_OP(2, uint16_t, Func)
    DO_VECTOR_SCALAR_OP(4, uint32_t, Func)
    DO_VECTOR_SCALAR_OP(8, uint64_t, Func)
    DO_VECTOR_SCALAR_OP(16, __uint128_t, Func)
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VUShrS) {
  auto Op = IROp->C<IR::IROp_VUShrS>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->ShiftScalar);
  uint8_t Tmp[16];

  const uint8_t Elements = OpSize / Op->Header.ElementSize;
  const auto Func = [](auto a, auto b) { return b >= (sizeof(a) * 8) ? 0 : a >> b; };

  switch (Op->Header.ElementSize) {
    DO_VECTOR_SCALAR_OP(1, uint8_t, Func)
    DO_VECTOR_SCALAR_OP(2, uint16_t, Func)
    DO_VECTOR_SCALAR_OP(4, uint32_t, Func)
    DO_VECTOR_SCALAR_OP(8, uint64_t, Func)
    DO_VECTOR_SCALAR_OP(16, __uint128_t, Func)
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VSShrS) {
  auto Op = IROp->C<IR::IROp_VSShrS>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->ShiftScalar);
  uint8_t Tmp[16];

  const uint8_t Elements = OpSize / Op->Header.ElementSize;
  const auto Func = [](auto a, auto b) {
    return b >= (sizeof(a) * 8) ? (a >> (sizeof(a) * 8 - 1)) : a >> b;
  };

  switch (Op->Header.ElementSize) {
    DO_VECTOR_SCALAR_OP(1, int8_t, Func)
    DO_VECTOR_SCALAR_OP(2, int16_t, Func)
    DO_VECTOR_SCALAR_OP(4, int32_t, Func)
    DO_VECTOR_SCALAR_OP(8, int64_t, Func)
    DO_VECTOR_SCALAR_OP(16, __int128_t, Func)
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VInsElement) {
  auto Op = IROp->C<IR::IROp_VInsElement>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->DestVector);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->SrcVector);
  uint8_t Tmp[16];

  // Copy src1 in to dest
  memcpy(Tmp, Src1, OpSize);
  switch (Op->Header.ElementSize) {
    case 1: {
      auto *Dst_d  = reinterpret_cast<uint8_t*>(Tmp);
      auto *Src2_d = reinterpret_cast<uint8_t*>(Src2);
      Dst_d[Op->DestIdx] = Src2_d[Op->SrcIdx];
      break;
    }
    case 2: {
      auto *Dst_d  = reinterpret_cast<uint16_t*>(Tmp);
      auto *Src2_d = reinterpret_cast<uint16_t*>(Src2);
      Dst_d[Op->DestIdx] = Src2_d[Op->SrcIdx];
      break;
    }
    case 4: {
      auto *Dst_d  = reinterpret_cast<uint32_t*>(Tmp);
      auto *Src2_d = reinterpret_cast<uint32_t*>(Src2);
      Dst_d[Op->DestIdx] = Src2_d[Op->SrcIdx];
      break;
    }
    case 8: {
      auto *Dst_d  = reinterpret_cast<uint64_t*>(Tmp);
      auto *Src2_d = reinterpret_cast<uint64_t*>(Src2);
      Dst_d[Op->DestIdx] = Src2_d[Op->SrcIdx];
      break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  };
  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VInsScalarElement) {
  auto Op = IROp->C<IR::IROp_VInsScalarElement>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->DestVector);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->SrcScalar);
  uint8_t Tmp[16];

  // Copy src1 in to dest
  memcpy(Tmp, Src1, OpSize);
  switch (Op->Header.ElementSize) {
    case 1: {
      auto *Dst_d  = reinterpret_cast<uint8_t*>(Tmp);
      auto Src2_d = *reinterpret_cast<uint8_t*>(Src2);
      Dst_d[Op->DestIdx] = Src2_d;
      break;
    }
    case 2: {
      auto *Dst_d  = reinterpret_cast<uint16_t*>(Tmp);
      auto Src2_d = *reinterpret_cast<uint16_t*>(Src2);
      Dst_d[Op->DestIdx] = Src2_d;
      break;
    }
    case 4: {
      auto *Dst_d  = reinterpret_cast<uint32_t*>(Tmp);
      auto Src2_d = *reinterpret_cast<uint32_t*>(Src2);
      Dst_d[Op->DestIdx] = Src2_d;
      break;
    }
    case 8: {
      auto *Dst_d  = reinterpret_cast<uint64_t*>(Tmp);
      auto Src2_d = *reinterpret_cast<uint64_t*>(Src2);
      Dst_d[Op->DestIdx] = Src2_d;
      break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  };
  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VExtractElement) {
  auto Op = IROp->C<IR::IROp_VExtractElement>();

  const uint32_t SourceSize = GetOpSize(Data->CurrentIR, Op->Vector);
  LOGMAN_THROW_AA_FMT(IROp->Size <= 16, "OpSize is too large for VExtractElement: {}", IROp->Size);
  if (SourceSize == 16) {
    __uint128_t SourceMask = (1ULL << (Op->Header.ElementSize * 8)) - 1;
    uint64_t Shift = Op->Header.ElementSize * Op->Index * 8;
    if (Op->Header.ElementSize == 8)
      SourceMask = ~0ULL;

    __uint128_t Src = *GetSrc<__uint128_t*>(Data->SSAData, Op->Vector);
    Src >>= Shift;
    Src &= SourceMask;
    memcpy(GDP, &Src, Op->Header.ElementSize);
  }
  else {
    uint64_t SourceMask = (1ULL << (Op->Header.ElementSize * 8)) - 1;
    uint64_t Shift = Op->Header.ElementSize * Op->Index * 8;
    if (Op->Header.ElementSize == 8)
      SourceMask = ~0ULL;

    uint64_t Src = *GetSrc<uint64_t*>(Data->SSAData, Op->Vector);
    Src >>= Shift;
    Src &= SourceMask;
    GD = Src;
  }
}

DEF_OP(VDupElement) {
  auto Op = IROp->C<IR::IROp_VDupElement>();
  const uint8_t OpSize = IROp->Size;
  const uint8_t Elements = OpSize / Op->Header.ElementSize;

  LOGMAN_THROW_AA_FMT(OpSize <= 16, "OpSize is too large for VDupElement: {}", OpSize);
  if (OpSize == 16) {
    __uint128_t SourceMask = (1ULL << (Op->Header.ElementSize * 8)) - 1;
    uint64_t Shift = Op->Header.ElementSize * Op->Index * 8;
    if (Op->Header.ElementSize == 8)
      SourceMask = ~0ULL;

    __uint128_t Src = *GetSrc<__uint128_t*>(Data->SSAData, Op->Vector);
    Src >>= Shift;
    Src &= SourceMask;
    for (size_t i = 0; i < Elements; ++i) {
      memcpy(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(GDP) + (Op->Header.ElementSize * i)),
        &Src, Op->Header.ElementSize);
    }
  }
  else {
    uint64_t SourceMask = (1ULL << (Op->Header.ElementSize * 8)) - 1;
    uint64_t Shift = Op->Header.ElementSize * Op->Index * 8;
    if (Op->Header.ElementSize == 8)
      SourceMask = ~0ULL;

    uint64_t Src = *GetSrc<uint64_t*>(Data->SSAData, Op->Vector);
    Src >>= Shift;
    Src &= SourceMask;
    for (size_t i = 0; i < Elements; ++i) {
      memcpy(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(GDP) + (Op->Header.ElementSize * i)),
        &Src, Op->Header.ElementSize);
    }
  }
}

DEF_OP(VExtr) {
  auto Op = IROp->C<IR::IROp_VExtr>();
  const uint8_t OpSize = IROp->Size;

  const auto Src1 = *GetSrc<__uint128_t*>(Data->SSAData, Op->VectorLower);
  const auto Src2 = *GetSrc<__uint128_t*>(Data->SSAData, Op->VectorUpper);

  uint64_t Offset = Op->Index * Op->Header.ElementSize * 8;
  __uint128_t Dst{};
  if (Offset >= (OpSize * 8)) {
    Offset -= OpSize * 8;
    Dst = Src1 >> Offset;
  }
  else {
    Dst = (Src1 << (OpSize * 8 - Offset)) | (Src2 >> Offset);
  }

  memcpy(GDP, &Dst, OpSize);
}

DEF_OP(VSLI) {
  auto Op = IROp->C<IR::IROp_VSLI>();
  const __uint128_t Src1 = *GetSrc<__uint128_t*>(Data->SSAData, Op->Vector);
  const __uint128_t Src2 = Op->ByteShift * 8;

  const __uint128_t Dst = Op->ByteShift >= sizeof(__uint128_t) ? 0 : Src1 << Src2;
  memcpy(GDP, &Dst, 16);
}

DEF_OP(VSRI) {
  auto Op = IROp->C<IR::IROp_VSRI>();
  const __uint128_t Src1 = *GetSrc<__uint128_t*>(Data->SSAData, Op->Vector);
  const __uint128_t Src2 = Op->ByteShift * 8;

  const __uint128_t Dst = Op->ByteShift >= sizeof(__uint128_t) ? 0 : Src1 >> Src2;
  memcpy(GDP, &Dst, 16);
}

DEF_OP(VUShrI) {
  auto Op = IROp->C<IR::IROp_VUShrI>();
  const uint8_t OpSize = IROp->Size;

  void *Src = GetSrc<void*>(Data->SSAData, Op->Vector);
  const uint8_t BitShift = Op->BitShift;
  uint8_t Tmp[16];

  const uint8_t Elements = OpSize / Op->Header.ElementSize;
  const auto Func = [BitShift](auto a) {
    return BitShift >= (sizeof(a) * 8) ? 0 : a >> BitShift;
  };

  switch (Op->Header.ElementSize) {
    DO_VECTOR_1SRC_OP(1, uint8_t, Func)
    DO_VECTOR_1SRC_OP(2, uint16_t, Func)
    DO_VECTOR_1SRC_OP(4, uint32_t, Func)
    DO_VECTOR_1SRC_OP(8, uint64_t, Func)
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VSShrI) {
  auto Op = IROp->C<IR::IROp_VSShrI>();
  const uint8_t OpSize = IROp->Size;

  void *Src = GetSrc<void*>(Data->SSAData, Op->Vector);
  uint8_t BitShift = Op->BitShift;
  uint8_t Tmp[16];

  const uint8_t Elements = OpSize / Op->Header.ElementSize;
  const auto Func = [BitShift](auto a) {
    return BitShift >= (sizeof(a) * 8) ? (a >> (sizeof(a) * 8 - 1)) : a >> BitShift;
  };

  switch (Op->Header.ElementSize) {
    DO_VECTOR_1SRC_OP(1, int8_t, Func)
    DO_VECTOR_1SRC_OP(2, int16_t, Func)
    DO_VECTOR_1SRC_OP(4, int32_t, Func)
    DO_VECTOR_1SRC_OP(8, int64_t, Func)
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VShlI) {
  auto Op = IROp->C<IR::IROp_VShlI>();
  const uint8_t OpSize = IROp->Size;

  void *Src = GetSrc<void*>(Data->SSAData, Op->Vector);
  uint8_t BitShift = Op->BitShift;
  uint8_t Tmp[16];

  const uint8_t Elements = OpSize / Op->Header.ElementSize;
  const auto Func = [BitShift](auto a) {
    return BitShift >= (sizeof(a) * 8) ? 0 : (a << BitShift);
  };

  switch (Op->Header.ElementSize) {
    DO_VECTOR_1SRC_OP(1, uint8_t, Func)
    DO_VECTOR_1SRC_OP(2, uint16_t, Func)
    DO_VECTOR_1SRC_OP(4, uint32_t, Func)
    DO_VECTOR_1SRC_OP(8, uint64_t, Func)
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VUShrNI) {
  auto Op = IROp->C<IR::IROp_VUShrNI>();
  const uint8_t OpSize = IROp->Size;

  void *Src = GetSrc<void*>(Data->SSAData, Op->Vector);
  uint8_t BitShift = Op->BitShift;
  uint8_t Tmp[16]{};

  const uint8_t Elements = OpSize / (Op->Header.ElementSize << 1);
  const auto Func = [BitShift](auto a, auto min, auto max) {
    return BitShift >= (sizeof(a) * 8) ? 0 : a >> BitShift;
  };

  switch (Op->Header.ElementSize) {
    DO_VECTOR_1SRC_2TYPE_OP(1, uint8_t, uint16_t, Func, 0, 0)
    DO_VECTOR_1SRC_2TYPE_OP(2, uint16_t, uint32_t, Func, 0, 0)
    DO_VECTOR_1SRC_2TYPE_OP(4, uint32_t, uint64_t, Func, 0, 0)
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VUShrNI2) {
  auto Op = IROp->C<IR::IROp_VUShrNI2>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->VectorLower);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->VectorUpper);
  uint8_t BitShift = Op->BitShift;
  uint8_t Tmp[16];

  const uint8_t Elements = OpSize / (Op->Header.ElementSize << 1);
  const auto Func = [BitShift](auto a, auto min, auto max) {
    return BitShift >= (sizeof(a) * 8) ? 0 : a >> BitShift;
  };

  switch (Op->Header.ElementSize) {
    DO_VECTOR_1SRC_2TYPE_OP_TOP(1, uint8_t, uint16_t, Func, 0, 0)
    DO_VECTOR_1SRC_2TYPE_OP_TOP(2, uint16_t, uint32_t, Func, 0, 0)
    DO_VECTOR_1SRC_2TYPE_OP_TOP(4, uint32_t, uint64_t, Func, 0, 0)
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VBitcast) {
  auto Op = IROp->C<IR::IROp_VBitcast>();
  memcpy(GDP, GetSrc<void*>(Data->SSAData, Op->Source), 16);
}

DEF_OP(VSXTL) {
  auto Op = IROp->C<IR::IROp_VSXTL>();
  const uint8_t OpSize = IROp->Size;

  void *Src = GetSrc<void*>(Data->SSAData, Op->Vector);
  uint8_t Tmp[16]{};

  const uint8_t Elements = OpSize / Op->Header.ElementSize;
  const auto Func = [](auto a, auto min, auto max) { return a; };

  switch (Op->Header.ElementSize) {
    DO_VECTOR_1SRC_2TYPE_OP(2, int16_t, int8_t, Func,  0, 0)
    DO_VECTOR_1SRC_2TYPE_OP(4, int32_t, int16_t, Func, 0, 0)
    DO_VECTOR_1SRC_2TYPE_OP(8, int64_t, int32_t, Func, 0, 0)
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VSXTL2) {
  auto Op = IROp->C<IR::IROp_VSXTL2>();
  const uint8_t OpSize = IROp->Size;

  void *Src = GetSrc<void*>(Data->SSAData, Op->Vector);
  uint8_t Tmp[16];

  const uint8_t Elements = OpSize / Op->Header.ElementSize;
  const auto Func = [](auto a, auto min, auto max) { return a; };

  switch (Op->Header.ElementSize) {
    DO_VECTOR_1SRC_2TYPE_OP_TOP_SRC(2, int16_t, int8_t, Func,  0, 0)
    DO_VECTOR_1SRC_2TYPE_OP_TOP_SRC(4, int32_t, int16_t, Func, 0, 0)
    DO_VECTOR_1SRC_2TYPE_OP_TOP_SRC(8, int64_t, int32_t, Func, 0, 0)
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VUXTL) {
  auto Op = IROp->C<IR::IROp_VUXTL>();
  const uint8_t OpSize = IROp->Size;

  void *Src = GetSrc<void*>(Data->SSAData, Op->Vector);
  uint8_t Tmp[16]{};

  const uint8_t Elements = OpSize / Op->Header.ElementSize;
  const auto Func = [](auto a, auto min, auto max) { return a; };

  switch (Op->Header.ElementSize) {
    DO_VECTOR_1SRC_2TYPE_OP(2, uint16_t, uint8_t, Func,  0, 0)
    DO_VECTOR_1SRC_2TYPE_OP(4, uint32_t, uint16_t, Func, 0, 0)
    DO_VECTOR_1SRC_2TYPE_OP(8, uint64_t, uint32_t, Func, 0, 0)
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VUXTL2) {
  auto Op = IROp->C<IR::IROp_VUXTL2>();
  const uint8_t OpSize = IROp->Size;

  void *Src = GetSrc<void*>(Data->SSAData, Op->Vector);

  uint8_t Tmp[16];

  const uint8_t Elements = OpSize / Op->Header.ElementSize;
  const auto Func = [](auto a, auto min, auto max) { return a; };

  switch (Op->Header.ElementSize) {
    DO_VECTOR_1SRC_2TYPE_OP_TOP_SRC(2, uint16_t, uint8_t, Func,  0, 0)
    DO_VECTOR_1SRC_2TYPE_OP_TOP_SRC(4, uint32_t, uint16_t, Func, 0, 0)
    DO_VECTOR_1SRC_2TYPE_OP_TOP_SRC(8, uint64_t, uint32_t, Func, 0, 0)
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VSQXTN) {
  auto Op = IROp->C<IR::IROp_VSQXTN>();
  const uint8_t OpSize = IROp->Size;

  void *Src = GetSrc<void*>(Data->SSAData, Op->Vector);
  uint8_t Tmp[16]{};

  const uint8_t Elements = OpSize / (Op->Header.ElementSize << 1);
  const auto Func = [](auto a, auto min, auto max) {
    return std::max(std::min(a, (decltype(a))max), (decltype(a))min);
  };

  switch (Op->Header.ElementSize) {
    DO_VECTOR_1SRC_2TYPE_OP(1, int8_t, int16_t, Func, std::numeric_limits<int8_t>::min(), std::numeric_limits<int8_t>::max())
    DO_VECTOR_1SRC_2TYPE_OP(2, int16_t, int32_t, Func, std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::max())
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VSQXTN2) {
  auto Op = IROp->C<IR::IROp_VSQXTN2>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->VectorLower);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->VectorUpper);
  uint8_t Tmp[16]{};

  const uint8_t Elements = OpSize / (Op->Header.ElementSize << 1);
  const auto Func = [](auto a, auto min, auto max) {
    return std::max(std::min(a, (decltype(a))max), (decltype(a))min);
  };

  switch (Op->Header.ElementSize) {
    DO_VECTOR_1SRC_2TYPE_OP_TOP(1, int8_t, int16_t, Func, std::numeric_limits<int8_t>::min(), std::numeric_limits<int8_t>::max())
    DO_VECTOR_1SRC_2TYPE_OP_TOP(2, int16_t, int32_t, Func, std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::max())
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VSQXTUN) {
  auto Op = IROp->C<IR::IROp_VSQXTUN>();
  const uint8_t OpSize = IROp->Size;

  void *Src = GetSrc<void*>(Data->SSAData, Op->Vector);
  uint8_t Tmp[16]{};

  const uint8_t Elements = OpSize / (Op->Header.ElementSize << 1);
  const auto Func = [](auto a, auto min, auto max) {
    return std::max(std::min(a, (decltype(a))max), (decltype(a))min);
  };

  switch (Op->Header.ElementSize) {
    DO_VECTOR_1SRC_2TYPE_OP(1, uint8_t, int16_t, Func, 0, (1 << 8) - 1)
    DO_VECTOR_1SRC_2TYPE_OP(2, uint16_t, int32_t, Func, 0, (1 << 16) - 1)
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VSQXTUN2) {
  auto Op = IROp->C<IR::IROp_VSQXTUN2>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->VectorLower);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->VectorUpper);
  uint8_t Tmp[16]{};

  const uint8_t Elements = OpSize / (Op->Header.ElementSize << 1);
  const auto Func = [](auto a, auto min, auto max) {
    return std::max(std::min(a, (decltype(a))max), (decltype(a))min);
  };

  switch (Op->Header.ElementSize) {
    DO_VECTOR_1SRC_2TYPE_OP_TOP(1, uint8_t, int16_t, Func, 0, (1 << 8) - 1)
    DO_VECTOR_1SRC_2TYPE_OP_TOP(2, uint16_t, int32_t, Func, 0, (1 << 16) - 1)
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VUMul) {
  auto Op = IROp->C<IR::IROp_VUMul>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);
  uint8_t Tmp[16];

  const uint8_t Elements = OpSize / Op->Header.ElementSize;
  const auto Func = [](auto a, auto b) { return a * b; };

  switch (Op->Header.ElementSize) {
    DO_VECTOR_OP(1, uint8_t,  Func)
    DO_VECTOR_OP(2, uint16_t, Func)
    DO_VECTOR_OP(4, uint32_t, Func)
    DO_VECTOR_OP(8, uint64_t, Func)
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VUMull) {
  auto Op = IROp->C<IR::IROp_VUMull>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);

  uint8_t Tmp[16];

  const uint8_t Elements = OpSize / Op->Header.ElementSize;
  const auto Func = [](auto a, auto b) { return a * b; };

  switch (Op->Header.ElementSize) {
    DO_VECTOR_2SRC_2TYPE_OP(2, uint16_t, uint8_t, Func)
    DO_VECTOR_2SRC_2TYPE_OP(4, uint32_t, uint16_t, Func)
    DO_VECTOR_2SRC_2TYPE_OP(8, uint64_t, uint32_t, Func)
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VSMul) {
  auto Op = IROp->C<IR::IROp_VSMul>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);
  uint8_t Tmp[16];

  const uint8_t Elements = OpSize / Op->Header.ElementSize;
  const auto Func = [](auto a, auto b) { return a * b; };

  switch (Op->Header.ElementSize) {
    DO_VECTOR_OP(1, int8_t,  Func)
    DO_VECTOR_OP(2, int16_t, Func)
    DO_VECTOR_OP(4, int32_t, Func)
    DO_VECTOR_OP(8, int64_t, Func)
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VSMull) {
  auto Op = IROp->C<IR::IROp_VSMull>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);

  uint8_t Tmp[16];

  const uint8_t Elements = OpSize / Op->Header.ElementSize;
  const auto Func = [](auto a, auto b) { return a * b; };

  switch (Op->Header.ElementSize) {
    DO_VECTOR_2SRC_2TYPE_OP(2, int16_t, int8_t, Func)
    DO_VECTOR_2SRC_2TYPE_OP(4, int32_t, int16_t, Func)
    DO_VECTOR_2SRC_2TYPE_OP(8, int64_t, int32_t, Func)
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VUMull2) {
  auto Op = IROp->C<IR::IROp_VUMull2>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);

  uint8_t Tmp[16];

  const uint8_t Elements = OpSize / Op->Header.ElementSize;
  const auto Func = [](auto a, auto b) { return a * b; };

  switch (Op->Header.ElementSize) {
    DO_VECTOR_2SRC_2TYPE_OP_TOP_SRC(2, uint16_t, uint8_t, Func)
    DO_VECTOR_2SRC_2TYPE_OP_TOP_SRC(4, uint32_t, uint16_t, Func)
    DO_VECTOR_2SRC_2TYPE_OP_TOP_SRC(8, uint64_t, uint32_t, Func)
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VSMull2) {
  auto Op = IROp->C<IR::IROp_VSMull2>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);

  uint8_t Tmp[16];

  const uint8_t Elements = OpSize / Op->Header.ElementSize;
  const auto Func = [](auto a, auto b) { return a * b; };

  switch (Op->Header.ElementSize) {
    DO_VECTOR_2SRC_2TYPE_OP_TOP_SRC(2, int16_t, int8_t, Func)
    DO_VECTOR_2SRC_2TYPE_OP_TOP_SRC(4, int32_t, int16_t, Func)
    DO_VECTOR_2SRC_2TYPE_OP_TOP_SRC(8, int64_t, int32_t, Func)
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
  memcpy(GDP, Tmp, Op->Header.Size);
}

DEF_OP(VUABDL) {
  auto Op = IROp->C<IR::IROp_VUABDL>();
  const uint8_t OpSize = IROp->Size;

  void *Src1 = GetSrc<void*>(Data->SSAData, Op->Vector1);
  void *Src2 = GetSrc<void*>(Data->SSAData, Op->Vector2);

  uint8_t Tmp[16];

  const uint8_t Elements = OpSize / Op->Header.ElementSize;

  const auto Func8 = [](auto a, auto b) { return std::abs((int16_t)a - (int16_t)b); };
  const auto Func16 = [](auto a, auto b) { return std::abs((int32_t)a - (int32_t)b); };
  const auto Func32 = [](auto a, auto b) { return std::abs((int64_t)a - (int64_t)b); };

  switch (Op->Header.ElementSize) {
    DO_VECTOR_2SRC_2TYPE_OP(2, uint16_t, uint8_t, Func8)
    DO_VECTOR_2SRC_2TYPE_OP(4, uint32_t, uint16_t, Func16)
    DO_VECTOR_2SRC_2TYPE_OP(8, uint64_t, uint32_t, Func32)
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VTBL1) {
  auto Op = IROp->C<IR::IROp_VTBL1>();
  const uint8_t OpSize = IROp->Size;

  const auto *Src1 = GetSrc<uint8_t*>(Data->SSAData, Op->VectorTable);
  const auto *Src2 = GetSrc<uint8_t*>(Data->SSAData, Op->VectorIndices);

  uint8_t Tmp[16];

  for (size_t i = 0; i < OpSize; ++i) {
    const uint8_t Index = Src2[i];
    Tmp[i] = Index >= OpSize ? 0 : Src1[Index];
  }
  memcpy(GDP, Tmp, OpSize);
}

DEF_OP(VRev64) {
  auto Op = IROp->C<IR::IROp_VRev64>();
  const uint8_t OpSize = IROp->Size;

  void *Src = GetSrc<void*>(Data->SSAData, Op->Vector);

  uint8_t Tmp[16];

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

  switch (Op->Header.ElementSize) {
    DO_VECTOR_1SRC_OP(1, uint64_t, Func8)
    DO_VECTOR_1SRC_OP(2, uint64_t, Func16)
    DO_VECTOR_1SRC_OP(4, uint64_t, Func32)

    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
  memcpy(GDP, Tmp, Op->Header.Size);
}

#undef DEF_OP

} // namespace FEXCore::CPU
