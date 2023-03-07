/*
$info$
tags: backend|interpreter
$end_info$
*/

#include "Interface/Core/CPUID.h"
#include "Interface/Core/Interpreter/InterpreterClass.h"
#include "Interface/Core/Interpreter/InterpreterOps.h"
#include "Interface/Core/Interpreter/InterpreterDefines.h"

#include <cstdint>

namespace FEXCore::CPU {
static inline void CacheLineFlush(char *Addr) {
#ifdef _M_X86_64
  __asm volatile (
    "clflush (%[Addr]);"
    :: [Addr] "r" (Addr)
    : "memory");
#else
  __builtin___clear_cache(Addr, Addr+64);
#endif
}

static inline void CacheLineClean(char *Addr) {
#ifdef _M_X86_64
  __asm volatile (
    "clwb (%[Addr]);"
    :: [Addr] "r" (Addr)
    : "memory");
#elif _M_ARM_64
  __asm volatile (
    "dc cvac, %[Addr]"
    :: [Addr] "r" (Addr)
    : "memory");
#else
    LOGMAN_THROW_A_FMT("Unsupported architecture with cacheline clean");
#endif
}

#define DEF_OP(x) void InterpreterOps::Op_##x(IR::IROp_Header *IROp, IROpData *Data, IR::NodeID Node)
DEF_OP(LoadContext) {
  const auto Op = IROp->C<IR::IROp_LoadContext>();
  const auto OpSize = IROp->Size;

  const auto ContextPtr = reinterpret_cast<uintptr_t>(Data->State->CurrentFrame);
  const auto Src = ContextPtr + Op->Offset;

  #define LOAD_CTX(x, y) \
    case x: { \
      y const *MemData = reinterpret_cast<y const*>(Src); \
      GD = *MemData; \
      break; \
    }

  switch (OpSize) {
    LOAD_CTX(1, uint8_t)
    LOAD_CTX(2, uint16_t)
    LOAD_CTX(4, uint32_t)
    LOAD_CTX(8, uint64_t)
    case 16:
    case 32: {
      void const *MemData = reinterpret_cast<void const*>(Src);
      memcpy(GDP, MemData, OpSize);
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unhandled LoadContext size: {}", OpSize);
      break;
  }
  #undef LOAD_CTX
}

DEF_OP(StoreContext) {
  const auto Op = IROp->C<IR::IROp_StoreContext>();
  const auto OpSize = IROp->Size;

  const auto ContextPtr = reinterpret_cast<uintptr_t>(Data->State->CurrentFrame);
  const auto Dst = ContextPtr + Op->Offset;

  void *MemData = reinterpret_cast<void*>(Dst);
  void *Src = GetSrc<void*>(Data->SSAData, Op->Value);
  memcpy(MemData, Src, OpSize);
}

DEF_OP(LoadRegister) {
  const auto Op = IROp->C<IR::IROp_LoadRegister>();
  const auto OpSize = IROp->Size;

  const auto ContextPtr = reinterpret_cast<uintptr_t>(Data->State->CurrentFrame);
  const auto Src = ContextPtr + Op->Offset;

  #define LOAD_CTX(x, y) \
    case x: { \
      y const *MemData = reinterpret_cast<y const*>(Src); \
      GD = *MemData; \
      break; \
    }

  switch (OpSize) {
    LOAD_CTX(1, uint8_t)
    LOAD_CTX(2, uint16_t)
    LOAD_CTX(4, uint32_t)
    LOAD_CTX(8, uint64_t)
    case 16:
    case 32: {
      void const *MemData = reinterpret_cast<void const*>(Src);
      memcpy(GDP, MemData, OpSize);
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unhandled LoadContext size: {}", OpSize);
      break;
  }
  #undef LOAD_CTX
}

DEF_OP(StoreRegister) {
  const auto Op = IROp->C<IR::IROp_StoreRegister>();
  const auto OpSize = IROp->Size;

  const auto ContextPtr = reinterpret_cast<uintptr_t>(Data->State->CurrentFrame);
  const auto Dst = ContextPtr + Op->Offset;

  void *MemData = reinterpret_cast<void*>(Dst);
  void *Src = GetSrc<void*>(Data->SSAData, Op->Value);
  memcpy(MemData, Src, OpSize);
}

DEF_OP(LoadContextIndexed) {
  const auto Op = IROp->C<IR::IROp_LoadContextIndexed>();
  const auto OpSize = IROp->Size;

  const auto Index = *GetSrc<uint64_t*>(Data->SSAData, Op->Index);

  const auto ContextPtr = reinterpret_cast<uintptr_t>(Data->State->CurrentFrame);
  const auto Src = ContextPtr + Op->BaseOffset + (Index * Op->Stride);

  #define LOAD_CTX(x, y) \
    case x: { \
      y const *MemData = reinterpret_cast<y const*>(Src); \
      GD = *MemData; \
      break; \
    }

  switch (OpSize) {
    LOAD_CTX(1, uint8_t)
    LOAD_CTX(2, uint16_t)
    LOAD_CTX(4, uint32_t)
    LOAD_CTX(8, uint64_t)
    case 16:
    case 32: {
      void const *MemData = reinterpret_cast<void const*>(Src);
      memcpy(GDP, MemData, OpSize);
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unhandled LoadContextIndexed size: {}", OpSize);
      break;
  }
  #undef LOAD_CTX
}

DEF_OP(StoreContextIndexed) {
  const auto Op = IROp->C<IR::IROp_StoreContextIndexed>();
  const auto OpSize = IROp->Size;

  const auto Index = *GetSrc<uint64_t*>(Data->SSAData, Op->Index);

  const auto ContextPtr = reinterpret_cast<uintptr_t>(Data->State->CurrentFrame);
  const auto Dst = ContextPtr + Op->BaseOffset + (Index * Op->Stride);

  void *MemData = reinterpret_cast<void*>(Dst);
  void *Src = GetSrc<void*>(Data->SSAData, Op->Value);
  memcpy(MemData, Src, OpSize);
}

DEF_OP(SpillRegister) {
  LOGMAN_MSG_A_FMT("Unimplemented");
}

DEF_OP(FillRegister) {
  LOGMAN_MSG_A_FMT("Unimplemented");
}

DEF_OP(LoadFlag) {
  auto Op = IROp->C<IR::IROp_LoadFlag>();

  uintptr_t ContextPtr = reinterpret_cast<uintptr_t>(Data->State->CurrentFrame);
  ContextPtr += offsetof(FEXCore::Core::CPUState, flags[0]);
  ContextPtr += Op->Flag;
  uint8_t const *MemData = reinterpret_cast<uint8_t const*>(ContextPtr);
  GD = *MemData;
}

DEF_OP(StoreFlag) {
  auto Op = IROp->C<IR::IROp_StoreFlag>();
  uint8_t Arg = *GetSrc<uint8_t*>(Data->SSAData, Op->Value);

  uintptr_t ContextPtr = reinterpret_cast<uintptr_t>(Data->State->CurrentFrame);
  ContextPtr += offsetof(FEXCore::Core::CPUState, flags[0]);
  ContextPtr += Op->Flag;
  uint8_t *MemData = reinterpret_cast<uint8_t*>(ContextPtr);
  *MemData = Arg;
}

DEF_OP(LoadMem) {
  const auto Op = IROp->C<IR::IROp_LoadMem>();
  const auto OpSize = IROp->Size;

  uint8_t const *MemData = *GetSrc<uint8_t const**>(Data->SSAData, Op->Addr);

  if (!Op->Offset.IsInvalid()) {
    auto Offset = *GetSrc<uintptr_t const*>(Data->SSAData, Op->Offset) * Op->OffsetScale;

    switch(Op->OffsetType.Val) {
      case IR::MEM_OFFSET_SXTX.Val: MemData +=  Offset; break;
      case IR::MEM_OFFSET_UXTW.Val: MemData += (uint32_t)Offset; break;
      case IR::MEM_OFFSET_SXTW.Val: MemData += (int32_t)Offset; break;
    }
  }

  memset(GDP, 0, Core::CPUState::XMM_AVX_REG_SIZE);
  switch (OpSize) {
    case 1: {
      auto D = reinterpret_cast<const std::atomic<uint8_t>*>(MemData);
      GD = D->load();
      break;
    }
    case 2: {
      auto D = reinterpret_cast<const std::atomic<uint16_t>*>(MemData);
      GD = D->load();
      break;
    }
    case 4: {
      auto D = reinterpret_cast<const std::atomic<uint32_t>*>(MemData);
      GD = D->load();
      break;
    }
    case 8: {
      auto D = reinterpret_cast<const std::atomic<uint64_t>*>(MemData);
      GD = D->load();
      break;
    }
    default:
      memcpy(GDP, MemData, OpSize);
      break;
  }
}

DEF_OP(StoreMem) {
  const auto Op = IROp->C<IR::IROp_StoreMem>();
  const auto OpSize = IROp->Size;

  uint8_t *MemData = *GetSrc<uint8_t **>(Data->SSAData, Op->Addr);

  if (!Op->Offset.IsInvalid()) {
    auto Offset = *GetSrc<uintptr_t const*>(Data->SSAData, Op->Offset) * Op->OffsetScale;

    switch(Op->OffsetType.Val) {
      case IR::MEM_OFFSET_SXTX.Val: MemData +=  Offset; break;
      case IR::MEM_OFFSET_UXTW.Val: MemData += (uint32_t)Offset; break;
      case IR::MEM_OFFSET_SXTW.Val: MemData += (int32_t)Offset; break;
    }
  }
  switch (OpSize) {
    case 1: {
      reinterpret_cast<std::atomic<uint8_t>*>(MemData)->store(*GetSrc<uint8_t*>(Data->SSAData, Op->Value));
      break;
    }
    case 2: {
      reinterpret_cast<std::atomic<uint16_t>*>(MemData)->store(*GetSrc<uint16_t*>(Data->SSAData, Op->Value));
      break;
    }
    case 4: {
      reinterpret_cast<std::atomic<uint32_t>*>(MemData)->store(*GetSrc<uint32_t*>(Data->SSAData, Op->Value));
      break;
    }
    case 8: {
      reinterpret_cast<std::atomic<uint64_t>*>(MemData)->store(*GetSrc<uint64_t*>(Data->SSAData, Op->Value));
      break;
    }

    default:
      memcpy(MemData, GetSrc<void*>(Data->SSAData, Op->Value), OpSize);
      break;
  }
}

DEF_OP(MemSet) {
  const auto Op = IROp->C<IR::IROp_MemSet>();
  const int32_t Size = Op->Size;

  char *MemData = *GetSrc<char **>(Data->SSAData, Op->Addr);
  uint64_t MemPrefix{};
  if (!Op->Prefix.IsInvalid()) {
    MemPrefix = *GetSrc<uint64_t*>(Data->SSAData, Op->Prefix);
  }

  const auto Value = *GetSrc<uint64_t*>(Data->SSAData, Op->Value);
  const auto Length = *GetSrc<uint64_t*>(Data->SSAData, Op->Length);
  const auto Direction = *GetSrc<uint8_t*>(Data->SSAData, Op->Direction);

  auto MemSetElements = [](auto* Memory, uint64_t Value, size_t Length) {
    for (size_t i = 0; i < Length; ++i) {
      Memory[i] = Value;
    }
  };

  auto MemSetElementsInverse = [](auto* Memory, uint64_t Value, size_t Length) {
    for (size_t i = 0; i < Length; ++i) {
      Memory[-i] = Value;
    }
  };

  if (Direction == 0) { // Forward
    if (Op->IsAtomic) {
      switch (Size) {
        case 1:
          MemSetElements(reinterpret_cast<std::atomic<uint8_t>*>(MemData + MemPrefix), Value, Length);
          break;
        case 2:
          MemSetElements(reinterpret_cast<std::atomic<uint16_t>*>(MemData + MemPrefix), Value, Length);
          break;
        case 4:
          MemSetElements(reinterpret_cast<std::atomic<uint32_t>*>(MemData + MemPrefix), Value, Length);
          break;
        case 8:
          MemSetElements(reinterpret_cast<std::atomic<uint64_t>*>(MemData + MemPrefix), Value, Length);
          break;
        default:
          LOGMAN_MSG_A_FMT("Unhandled {} size: {}", __func__, Size);
          break;
      }
    }
    else {
      switch (Size) {
        case 1:
          MemSetElements(reinterpret_cast<uint8_t*>(MemData + MemPrefix), Value, Length);
          break;
        case 2:
          MemSetElements(reinterpret_cast<uint16_t*>(MemData + MemPrefix), Value, Length);
          break;
        case 4:
          MemSetElements(reinterpret_cast<uint32_t*>(MemData + MemPrefix), Value, Length);
          break;
        case 8:
          MemSetElements(reinterpret_cast<uint64_t*>(MemData + MemPrefix), Value, Length);
          break;
        default:
          LOGMAN_MSG_A_FMT("Unhandled {} size: {}", __func__, Size);
          break;
      }
    }
    GD = reinterpret_cast<uint64_t>(MemData + (Length * Size));
  }
  else { // Backward
    if (Op->IsAtomic) {
      switch (Size) {
        case 1:
          MemSetElementsInverse(reinterpret_cast<std::atomic<uint8_t>*>(MemData + MemPrefix), Value, Length);
          break;
        case 2:
          MemSetElementsInverse(reinterpret_cast<std::atomic<uint16_t>*>(MemData + MemPrefix), Value, Length);
          break;
        case 4:
          MemSetElementsInverse(reinterpret_cast<std::atomic<uint32_t>*>(MemData + MemPrefix), Value, Length);
          break;
        case 8:
          MemSetElementsInverse(reinterpret_cast<std::atomic<uint64_t>*>(MemData + MemPrefix), Value, Length);
          break;
        default:
          LOGMAN_MSG_A_FMT("Unhandled {} size: {}", __func__, Size);
          break;
      }
    }
    else {
      switch (Size) {
        case 1:
          MemSetElementsInverse(reinterpret_cast<uint8_t*>(MemData + MemPrefix), Value, Length);
          break;
        case 2:
          MemSetElementsInverse(reinterpret_cast<uint16_t*>(MemData + MemPrefix), Value, Length);
          break;
        case 4:
          MemSetElementsInverse(reinterpret_cast<uint32_t*>(MemData + MemPrefix), Value, Length);
          break;
        case 8:
          MemSetElementsInverse(reinterpret_cast<uint64_t*>(MemData + MemPrefix), Value, Length);
          break;
        default:
          LOGMAN_MSG_A_FMT("Unhandled {} size: {}", __func__, Size);
          break;
      }
    }
    GD = reinterpret_cast<uint64_t>(MemData - (Length * Size));
  }
}

DEF_OP(MemCpy) {
  const auto Op = IROp->C<IR::IROp_MemCpy>();
  const int32_t Size = Op->Size;

  uint64_t *DstPtr = GetDest<uint64_t*>(Data->SSAData, Node);

  char *MemDataDest = *GetSrc<char **>(Data->SSAData, Op->AddrDest);
  char *MemDataSrc = *GetSrc<char **>(Data->SSAData, Op->AddrSrc);

  uint64_t DestPrefix{};
  uint64_t SrcPrefix{};
  if (!Op->PrefixDest.IsInvalid()) {
    DestPrefix = *GetSrc<uint64_t*>(Data->SSAData, Op->PrefixDest);

  }
  if (!Op->PrefixSrc.IsInvalid()) {
    SrcPrefix = *GetSrc<uint64_t*>(Data->SSAData, Op->PrefixSrc);
  }

  const auto Length = *GetSrc<uint64_t*>(Data->SSAData, Op->Length);
  const auto Direction = *GetSrc<uint8_t*>(Data->SSAData, Op->Direction);

  auto MemSetElementsAtomic = [](auto* MemDst, auto* MemSrc, size_t Length) {
    for (size_t i = 0; i < Length; ++i) {
      MemDst[i].store(MemSrc[i].load());
    }
  };

  auto MemSetElementsAtomicInverse = [](auto* MemDst, auto* MemSrc, size_t Length) {
    for (size_t i = 0; i < Length; ++i) {
      MemDst[-i].store(MemSrc[-i].load());
    }
  };

  auto MemSetElements = [](auto* MemDst, auto* MemSrc, size_t Length) {
    for (size_t i = 0; i < Length; ++i) {
      MemDst[i] = MemSrc[i];
    }
  };

  auto MemSetElementsInverse = [](auto* MemDst, auto* MemSrc, size_t Length) {
    for (size_t i = 0; i < Length; ++i) {
      MemDst[-i] = MemSrc[-i];
    }
  };

  if (Direction == 0) { // Forward
    if (Op->IsAtomic) {
      switch (Size) {
        case 1:
          MemSetElementsAtomic(reinterpret_cast<std::atomic<uint8_t>*>(MemDataDest + DestPrefix), reinterpret_cast<std::atomic<uint8_t>*>(MemDataSrc + SrcPrefix), Length);
          break;
        case 2:
          MemSetElementsAtomic(reinterpret_cast<std::atomic<uint16_t>*>(MemDataDest + DestPrefix), reinterpret_cast<std::atomic<uint16_t>*>(MemDataSrc + SrcPrefix), Length);
          break;
        case 4:
          MemSetElementsAtomic(reinterpret_cast<std::atomic<uint32_t>*>(MemDataDest + DestPrefix), reinterpret_cast<std::atomic<uint32_t>*>(MemDataSrc + SrcPrefix), Length);
          break;
        case 8:
          MemSetElementsAtomic(reinterpret_cast<std::atomic<uint64_t>*>(MemDataDest + DestPrefix), reinterpret_cast<std::atomic<uint64_t>*>(MemDataSrc + SrcPrefix), Length);
          break;
        default:
          LOGMAN_MSG_A_FMT("Unhandled {} size: {}", __func__, Size);
          break;
      }
    }
    else {
      switch (Size) {
        case 1:
          MemSetElements(reinterpret_cast<uint8_t*>(MemDataDest + DestPrefix), reinterpret_cast<uint8_t*>(MemDataSrc + SrcPrefix), Length);
          break;
        case 2:
          MemSetElements(reinterpret_cast<uint16_t*>(MemDataDest + DestPrefix), reinterpret_cast<uint16_t*>(MemDataSrc + SrcPrefix), Length);
          break;
        case 4:
          MemSetElements(reinterpret_cast<uint32_t*>(MemDataDest + DestPrefix), reinterpret_cast<uint32_t*>(MemDataSrc + SrcPrefix), Length);
          break;
        case 8:
          MemSetElements(reinterpret_cast<uint64_t*>(MemDataDest + DestPrefix), reinterpret_cast<uint64_t*>(MemDataSrc + SrcPrefix), Length);
          break;
        default:
          LOGMAN_MSG_A_FMT("Unhandled {} size: {}", __func__, Size);
          break;
      }
    }
    DstPtr[0] = reinterpret_cast<uint64_t>(MemDataDest + (Length * Size));
    DstPtr[1] = reinterpret_cast<uint64_t>(MemDataSrc + (Length * Size));
  }
  else { // Backward
    if (Op->IsAtomic) {
      switch (Size) {
        case 1:
          MemSetElementsAtomicInverse(reinterpret_cast<std::atomic<uint8_t>*>(MemDataDest + DestPrefix), reinterpret_cast<std::atomic<uint8_t>*>(MemDataSrc + SrcPrefix), Length);
          break;
        case 2:
          MemSetElementsAtomicInverse(reinterpret_cast<std::atomic<uint16_t>*>(MemDataDest + DestPrefix), reinterpret_cast<std::atomic<uint16_t>*>(MemDataSrc + SrcPrefix), Length);
          break;
        case 4:
          MemSetElementsAtomicInverse(reinterpret_cast<std::atomic<uint32_t>*>(MemDataDest + DestPrefix), reinterpret_cast<std::atomic<uint32_t>*>(MemDataSrc + SrcPrefix), Length);
          break;
        case 8:
          MemSetElementsAtomicInverse(reinterpret_cast<std::atomic<uint64_t>*>(MemDataDest + DestPrefix), reinterpret_cast<std::atomic<uint64_t>*>(MemDataSrc + SrcPrefix), Length);
          break;
        default:
          LOGMAN_MSG_A_FMT("Unhandled {} size: {}", __func__, Size);
          break;
      }
    }
    else {
      switch (Size) {
        case 1:
          MemSetElementsInverse(reinterpret_cast<uint8_t*>(MemDataDest + DestPrefix), reinterpret_cast<uint8_t*>(MemDataSrc + SrcPrefix), Length);
          break;
        case 2:
          MemSetElementsInverse(reinterpret_cast<uint16_t*>(MemDataDest + DestPrefix), reinterpret_cast<uint16_t*>(MemDataSrc + SrcPrefix), Length);
          break;
        case 4:
          MemSetElementsInverse(reinterpret_cast<uint32_t*>(MemDataDest + DestPrefix), reinterpret_cast<uint32_t*>(MemDataSrc + SrcPrefix), Length);
          break;
        case 8:
          MemSetElementsInverse(reinterpret_cast<uint64_t*>(MemDataDest + DestPrefix), reinterpret_cast<uint64_t*>(MemDataSrc + SrcPrefix), Length);
          break;
        default:
          LOGMAN_MSG_A_FMT("Unhandled {} size: {}", __func__, Size);
          break;
      }
    }
    DstPtr[0] = reinterpret_cast<uint64_t>(MemDataDest - (Length * Size));
    DstPtr[1] = reinterpret_cast<uint64_t>(MemDataSrc - (Length * Size));
  }
}

DEF_OP(CacheLineClear) {
  auto Op = IROp->C<IR::IROp_CacheLineClear>();

  char *MemData = *GetSrc<char **>(Data->SSAData, Op->Addr);

  // 64-byte cache line clear
  CacheLineFlush(MemData);
}

DEF_OP(CacheLineClean) {
  auto Op = IROp->C<IR::IROp_CacheLineClean>();

  char *MemData = *GetSrc<char **>(Data->SSAData, Op->Addr);

  // 64-byte cache line clear
  CacheLineClean(MemData);
}

DEF_OP(CacheLineZero) {
  auto Op = IROp->C<IR::IROp_CacheLineZero>();

  uintptr_t MemData = *GetSrc<uintptr_t*>(Data->SSAData, Op->Addr);

  // Force cacheline alignment
  MemData = MemData & ~(CPUIDEmu::CACHELINE_SIZE - 1);

  using DataType = uint64_t;
  DataType *MemData64 = reinterpret_cast<DataType*>(MemData);

  // 64-byte cache line zero
  for (size_t i = 0; i < (CPUIDEmu::CACHELINE_SIZE / sizeof(DataType)); ++i) {
    MemData64[i] = 0;
  }
}

#undef DEF_OP
} // namespace FEXCore::CPU
