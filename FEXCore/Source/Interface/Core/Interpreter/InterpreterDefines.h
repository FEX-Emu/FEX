#pragma once

#include <FEXCore/IR/IR.h>

#define GD *GetDest<uint64_t*>(Data->SSAData, Node)
#define GDP GetDest<void*>(Data->SSAData, Node)

#define DO_OP(size, type, func)              \
  case size: {                                      \
               auto *Dst_d  = reinterpret_cast<type*>(GDP);  \
               auto *Src1_d = reinterpret_cast<type*>(Src1); \
               auto *Src2_d = reinterpret_cast<type*>(Src2); \
               *Dst_d = func(*Src1_d, *Src2_d);          \
               break;                                            \
             }
#define DO_SCALAR_COMPARE_OP(size, type, type2, func)              \
  case size: {                                      \
  auto *Dst_d  = reinterpret_cast<type2*>(Tmp);  \
  auto *Src1_d = reinterpret_cast<type*>(Src1); \
  auto *Src2_d = reinterpret_cast<type*>(Src2); \
  Dst_d[0] = func(Src1_d[0], Src2_d[0]);          \
  break;                                            \
  }

#define DO_VECTOR_COMPARE_OP(size, type, type2, func)              \
  case size: {                                      \
  auto *Dst_d  = reinterpret_cast<type2*>(Tmp);  \
  auto *Src1_d = reinterpret_cast<type*>(Src1); \
  auto *Src2_d = reinterpret_cast<type*>(Src2); \
  for (uint8_t i = 0; i < Elements; ++i) {          \
    Dst_d[i] = func(Src1_d[i], Src2_d[i]);          \
  }                                                 \
  break;                                            \
  }
#define DO_VECTOR_OP(size, type, func)              \
  case size: {                                      \
  auto *Dst_d  = reinterpret_cast<type*>(Tmp);  \
  auto *Src1_d = reinterpret_cast<type*>(Src1); \
  auto *Src2_d = reinterpret_cast<type*>(Src2); \
  for (uint8_t i = 0; i < Elements; ++i) {          \
    Dst_d[i] = func(Src1_d[i], Src2_d[i]);          \
  }                                                 \
  break;                                            \
  }
#define DO_VECTOR_PAIR_OP(size, type, func)              \
  case size: {                                      \
  auto *Dst_d  = reinterpret_cast<type*>(Tmp);  \
  auto *Src1_d = reinterpret_cast<type*>(Src1); \
  auto *Src2_d = reinterpret_cast<type*>(Src2); \
  for (uint8_t i = 0; i < Elements; ++i) {          \
    Dst_d[i] = func(Src1_d[i*2], Src1_d[i*2 + 1]);          \
    Dst_d[i+Elements] = func(Src2_d[i*2], Src2_d[i*2 + 1]);          \
  }                                                 \
  break;                                            \
  }
#define DO_VECTOR_SCALAR_OP(size, type, func)\
  case size: {                                      \
  auto *Dst_d  = reinterpret_cast<type*>(Tmp);  \
  auto *Src1_d = reinterpret_cast<type*>(Src1); \
  auto *Src2_d = reinterpret_cast<type*>(Src2); \
  for (uint8_t i = 0; i < Elements; ++i) {          \
    Dst_d[i] = func(Src1_d[i], *Src2_d);          \
  }                                                 \
  break;                                            \
  }
#define DO_VECTOR_0SRC_OP(size, type, func)              \
  case size: {                                      \
  auto *Dst_d  = reinterpret_cast<type*>(Tmp);  \
  for (uint8_t i = 0; i < Elements; ++i) {          \
    Dst_d[i] = func();          \
  }                                                 \
  break;                                            \
  }
#define DO_VECTOR_1SRC_OP(size, type, func)              \
  case size: {                                      \
  auto *Dst_d  = reinterpret_cast<type*>(Tmp);  \
  auto *Src_d = reinterpret_cast<type*>(Src); \
  for (uint8_t i = 0; i < Elements; ++i) {          \
    Dst_d[i] = func(Src_d[i]);          \
  }                                                 \
  break;                                            \
  }
#define DO_VECTOR_REDUCE_1SRC_OP(size, type, func, start_val)              \
  case size: {                                      \
  auto *Dst_d  = reinterpret_cast<type*>(Tmp);  \
  auto *Src_d = reinterpret_cast<type*>(Src); \
  type begin = start_val;                           \
  for (uint8_t i = 0; i < Elements; ++i) {          \
    begin = func(begin, Src_d[i]);          \
  }                                                 \
  Dst_d[0] = begin;                                 \
  break;                                            \
  }
#define DO_VECTOR_SAT_OP(size, type, func, min, max)              \
  case size: {                                      \
  auto *Dst_d  = reinterpret_cast<type*>(Tmp);  \
  auto *Src1_d = reinterpret_cast<type*>(Src1); \
  auto *Src2_d = reinterpret_cast<type*>(Src2); \
  for (uint8_t i = 0; i < Elements; ++i) {          \
    Dst_d[i] = func(Src1_d[i], Src2_d[i], min, max);          \
  }                                                 \
  break;                                            \
  }

#define DO_VECTOR_1SRC_2TYPE_OP(size, type, type2, func, min, max)              \
  case size: {                                      \
  auto *Dst_d  = reinterpret_cast<type*>(Tmp);  \
  auto *Src_d = reinterpret_cast<type2*>(Src); \
  for (uint8_t i = 0; i < Elements; ++i) {          \
    Dst_d[i] = (type)func(Src_d[i], min, max);          \
  }                                                 \
  break;                                            \
  }

#define DO_VECTOR_1SRC_2TYPE_OP_NOSIZE(type, type2, func, min, max)              \
  auto *Dst_d  = reinterpret_cast<type*>(Tmp);  \
  auto *Src_d = reinterpret_cast<type2*>(Src); \
  for (uint8_t i = 0; i < Elements; ++i) {          \
    Dst_d[i] = (type)func(Src_d[i], min, max);          \
  }
#define DO_VECTOR_1SRC_2TYPE_OP_TOP(size, type, type2, func, min, max)              \
  case size: {                                      \
  auto *Dst_d  = reinterpret_cast<type*>(Tmp);  \
  auto *Src_d = reinterpret_cast<type2*>(Src2); \
  memcpy(Dst_d, Src1, Elements * sizeof(type2));\
  for (uint8_t i = 0; i < Elements; ++i) {          \
    Dst_d[i+Elements] = (type)func(Src_d[i], min, max);          \
  }                                                 \
  break;                                            \
  }

#define DO_VECTOR_1SRC_2TYPE_OP_TOP_SRC(size, type, type2, func, min, max)              \
  case size: {                                      \
  auto *Dst_d  = reinterpret_cast<type*>(Tmp);  \
  auto *Src_d = reinterpret_cast<type2*>(Src); \
  for (uint8_t i = 0; i < Elements; ++i) {          \
    Dst_d[i] = (type)func(Src_d[i+Elements], min, max);          \
  }                                                 \
  break;                                            \
  }
#define DO_VECTOR_2SRC_2TYPE_OP(size, type, type2, func)              \
  case size: {                                      \
  auto *Dst_d  = reinterpret_cast<type*>(Tmp);  \
  auto *Src1_d = reinterpret_cast<type2*>(Src1); \
  auto *Src2_d = reinterpret_cast<type2*>(Src2); \
  for (uint8_t i = 0; i < Elements; ++i) {          \
    Dst_d[i] = (type)func((type)Src1_d[i], (type)Src2_d[i]);          \
  }                                                 \
  break;                                            \
  }
#define DO_VECTOR_2SRC_2TYPE_OP_TOP_SRC(size, type, type2, func)              \
  case size: {                                      \
  auto *Dst_d  = reinterpret_cast<type*>(Tmp);  \
  auto *Src1_d = reinterpret_cast<type2*>(Src1); \
  auto *Src2_d = reinterpret_cast<type2*>(Src2); \
  for (uint8_t i = 0; i < Elements; ++i) {          \
    Dst_d[i] = (type)func((type)Src1_d[i+Elements], (type)Src2_d[i+Elements]);          \
  }                                                 \
  break;                                            \
  }

struct InterpVector256 {
  __uint128_t Lower;
  __uint128_t Upper;
};

template<typename Res>
Res GetDest(void* SSAData, FEXCore::IR::OrderedNodeWrapper Op) {
  auto DstPtr = &reinterpret_cast<InterpVector256*>(SSAData)[Op.ID().Value];
  return reinterpret_cast<Res>(DstPtr);
}

template<typename Res>
Res GetDest(void* SSAData, FEXCore::IR::NodeID Op) {
  auto DstPtr = &reinterpret_cast<InterpVector256*>(SSAData)[Op.Value];
  return reinterpret_cast<Res>(DstPtr);
}


template<typename Res>
Res GetSrc(void* SSAData, FEXCore::IR::OrderedNodeWrapper Src) {
  auto DstPtr = &reinterpret_cast<InterpVector256*>(SSAData)[Src.ID().Value];
  return reinterpret_cast<Res>(DstPtr);
}
