/*
$info$
tags: backend|interpreter
$end_info$
*/

#include "Interface/Core/Interpreter/InterpreterClass.h"
#include "Interface/Core/Interpreter/InterpreterOps.h"
#include "Interface/Core/Interpreter/InterpreterDefines.h"

#include <FEXCore/Utils/BitUtils.h>

#include <cstdint>

namespace FEXCore::CPU {

#ifdef _M_X86_64
uint8_t AtomicFetchNeg(uint8_t *Addr) {
  using Type = uint8_t;
  std::atomic<Type> *MemData = reinterpret_cast<std::atomic<Type>*>(Addr);
  Type Expected = MemData->load();
  Type Desired = -Expected;
  do {
    Desired = -Expected;
  } while (!MemData->compare_exchange_strong(Expected, Desired, std::memory_order_seq_cst));

  return Expected;
}

uint16_t AtomicFetchNeg(uint16_t *Addr) {
  using Type = uint16_t;
  std::atomic<Type> *MemData = reinterpret_cast<std::atomic<Type>*>(Addr);
  Type Expected = MemData->load();
  Type Desired = -Expected;
  do {
    Desired = -Expected;
  } while (!MemData->compare_exchange_strong(Expected, Desired, std::memory_order_seq_cst));

  return Expected;
}

uint32_t AtomicFetchNeg(uint32_t *Addr) {
  using Type = uint32_t;
  std::atomic<Type> *MemData = reinterpret_cast<std::atomic<Type>*>(Addr);
  Type Expected = MemData->load();
  Type Desired = -Expected;
  do {
    Desired = -Expected;
  } while (!MemData->compare_exchange_strong(Expected, Desired, std::memory_order_seq_cst));

  return Expected;
}

uint64_t AtomicFetchNeg(uint64_t *Addr) {
  using Type = uint64_t;
  std::atomic<Type> *MemData = reinterpret_cast<std::atomic<Type>*>(Addr);
  Type Expected = MemData->load();
  Type Desired = -Expected;
  do {
    Desired = -Expected;
  } while (!MemData->compare_exchange_strong(Expected, Desired, std::memory_order_seq_cst));

  return Expected;
}

template<typename T>
T AtomicCompareAndSwap(T expected, T desired, T *addr)
{
  std::atomic<T> *MemData = reinterpret_cast<std::atomic<T>*>(addr);

  T Src1 = expected;
  T Src2 = desired;

  T Expected = Src1;
  bool Result = MemData->compare_exchange_strong(Expected, Src2);

  return Result ? Src1 : Expected;
}

template uint8_t AtomicCompareAndSwap<uint8_t>(uint8_t expected, uint8_t desired, uint8_t *addr);
template uint16_t AtomicCompareAndSwap<uint16_t>(uint16_t expected, uint16_t desired, uint16_t *addr);
template uint32_t AtomicCompareAndSwap<uint32_t>(uint32_t expected, uint32_t desired, uint32_t *addr);
template uint64_t AtomicCompareAndSwap<uint64_t>(uint64_t expected, uint64_t desired, uint64_t *addr);

#else
// Needs to match what the AArch64 JIT and unaligned signal handler expects
uint8_t AtomicFetchNeg(uint8_t *Addr) {
  using Type = uint8_t;
  Type Result{};
  Type Tmp{};
  Type TmpStatus{};

  __asm__ volatile(
  R"(
  1:
    ldaxrb %w[Result], [%[Memory]];
    neg %w[Tmp], %w[Result];
    stlxrb %w[TmpStatus], %w[Tmp], [%[Memory]];
    cbnz %w[TmpStatus], 1b;
  )"
  : [Result] "=r" (Result)
  , [Tmp] "=r" (Tmp)
  , [TmpStatus] "=r" (TmpStatus)
  , [Memory] "+r" (Addr)
  :: "memory"
  );
  return Result;
}

uint16_t AtomicFetchNeg(uint16_t *Addr) {
  using Type = uint16_t;
  Type Result{};
  Type Tmp{};
  Type TmpStatus{};

  __asm__ volatile(
  R"(
  1:
    ldaxrh %w[Result], [%[Memory]];
    neg %w[Tmp], %w[Result];
    stlxrh %w[TmpStatus], %w[Tmp], [%[Memory]];
    cbnz %w[TmpStatus], 1b;
  )"
  : [Result] "=r" (Result)
  , [Tmp] "=r" (Tmp)
  , [TmpStatus] "=r" (TmpStatus)
  , [Memory] "+r" (Addr)
  :: "memory"
  );
  return Result;
}

uint32_t AtomicFetchNeg(uint32_t *Addr) {
  using Type = uint32_t;
  Type Result{};
  Type Tmp{};
  Type TmpStatus{};

  __asm__ volatile(
  R"(
  1:
    ldaxr %w[Result], [%[Memory]];
    neg %w[Tmp], %w[Result];
    stlxr %w[TmpStatus], %w[Tmp], [%[Memory]];
    cbnz %w[TmpStatus], 1b;
  )"
  : [Result] "=r" (Result)
  , [Tmp] "=r" (Tmp)
  , [TmpStatus] "=r" (TmpStatus)
  , [Memory] "+r" (Addr)
  :: "memory"
  );
  return Result;
}

uint64_t AtomicFetchNeg(uint64_t *Addr) {
  using Type = uint64_t;
  Type Result{};
  Type Tmp{};
  Type TmpStatus{};

  __asm__ volatile(
  R"(
  1:
    ldaxr %[Result], [%[Memory]];
    neg %[Tmp], %[Result];
    stlxr %w[TmpStatus], %[Tmp], [%[Memory]];
    cbnz %w[TmpStatus], 1b;
  )"
  : [Result] "=r" (Result)
  , [Tmp] "=r" (Tmp)
  , [TmpStatus] "=r" (TmpStatus)
  , [Memory] "+r" (Addr)
  :: "memory"
  );
  return Result;
}

template<>
uint8_t AtomicCompareAndSwap(uint8_t expected, uint8_t desired, uint8_t *addr) {
  using Type = uint8_t;
  //force Result to r9 (scratch register) or clang spills to stack
  register Type Result asm("r9"){};
  Type Tmp{};
  Type Tmp2{};
  __asm__ volatile(
  R"(
  1:
    ldaxrb %w[Tmp], [%[Memory]];
    cmp %w[Tmp], %w[Expected], uxtb;
    b.ne 2f;
    stlxrb %w[Tmp2], %w[Desired], [%[Memory]];
    cbnz %w[Tmp2], 1b;
    mov %w[Result], %w[Expected];
    b 3f;
  2:
    mov %w[Result], %w[Tmp];
    clrex;
  3:
  )"
  : [Tmp] "=r" (Tmp)
  , [Tmp2] "=r" (Tmp2)
  , [Desired] "+r" (desired)
  , [Expected] "+r" (expected)
  , [Result] "=r" (Result)
  , [Memory] "+r" (addr)
  :: "memory"
  );
  return Result;
}

template<>
uint16_t AtomicCompareAndSwap(uint16_t expected, uint16_t desired, uint16_t *addr) {
  using Type = uint16_t;
  //force Result to r9 (scratch register) or clang spills to stack
  register Type Result asm("r9"){};
  Type Tmp{};
  Type Tmp2{};
  __asm__ volatile(
  R"(
  1:
    ldaxrh %w[Tmp], [%[Memory]];
    cmp %w[Tmp], %w[Expected], uxth;
    b.ne 2f;
    stlxrh %w[Tmp2], %w[Desired], [%[Memory]];
    cbnz %w[Tmp2], 1b;
    mov %w[Result], %w[Expected];
    b 3f;
  2:
    mov %w[Result], %w[Tmp];
    clrex;
  3:
  )"
  : [Tmp] "=r" (Tmp)
  , [Tmp2] "=r" (Tmp2)
  , [Desired] "+r" (desired)
  , [Expected] "+r" (expected)
  , [Result] "=r" (Result)
  , [Memory] "+r" (addr)
  :: "memory"
  );
  return Result;
}

template<>
uint32_t AtomicCompareAndSwap(uint32_t expected, uint32_t desired, uint32_t *addr) {
  using Type = uint32_t;
  //force Result to r9 (scratch register) or clang spills to stack
  register Type Result asm("r9"){};
  Type Tmp{};
  Type Tmp2{};
  __asm__ volatile(
  R"(
  1:
    ldaxr %w[Tmp], [%[Memory]];
    cmp %w[Tmp], %w[Expected];
    b.ne 2f;
    stlxr %w[Tmp2], %w[Desired], [%[Memory]];
    cbnz %w[Tmp2], 1b;
    mov %w[Result], %w[Expected];
    b 3f;
  2:
    mov %w[Result], %w[Tmp];
    clrex;
  3:
  )"
  : [Tmp] "=r" (Tmp)
  , [Tmp2] "=r" (Tmp2)
  , [Desired] "+r" (desired)
  , [Expected] "+r" (expected)
  , [Result] "=r" (Result)
  , [Memory] "+r" (addr)
  :: "memory"
  );
  return Result;
}

template<>
uint64_t AtomicCompareAndSwap(uint64_t expected, uint64_t desired, uint64_t *addr) {
  using Type = uint64_t;
  //force Result to r9 (scratch register) or clang spills to stack
  register Type Result asm("r9"){};
  Type Tmp{};
  Type Tmp2{};
  __asm__ volatile(
  R"(
  1:
    ldaxr %[Tmp], [%[Memory]];
    cmp %[Tmp], %[Expected];
    b.ne 2f;
    stlxr %w[Tmp2], %[Desired], [%[Memory]];
    cbnz %w[Tmp2], 1b;
    mov %[Result], %[Expected];
    b 3f;
  2:
    mov %[Result], %[Tmp];
    clrex;
  3:
  )"
  : [Tmp] "=r" (Tmp)
  , [Tmp2] "=r" (Tmp2)
  , [Desired] "+r" (desired)
  , [Expected] "+r" (expected)
  , [Result] "=r" (Result)
  , [Memory] "+r" (addr)
  :: "memory"
  );
  return Result;
}

#endif

#define DEF_OP(x) void InterpreterOps::Op_##x(IR::IROp_Header *IROp, IROpData *Data, IR::NodeID Node)
DEF_OP(CASPair) {
  auto Op = IROp->C<IR::IROp_CASPair>();

  // Size is the size of each pair element
  switch (IROp->ElementSize) {
    case 4: {
      GD = AtomicCompareAndSwap(
        *GetSrc<uint64_t*>(Data->SSAData, Op->Expected),
        *GetSrc<uint64_t*>(Data->SSAData, Op->Desired),
        *GetSrc<uint64_t**>(Data->SSAData, Op->Addr)
      );
      break;
    }
    case 8: {
      std::atomic<__uint128_t> *MemData = *GetSrc<std::atomic<__uint128_t> **>(Data->SSAData, Op->Addr);

      __uint128_t Src1 = *GetSrc<__uint128_t*>(Data->SSAData, Op->Expected);
      __uint128_t Src2 = *GetSrc<__uint128_t*>(Data->SSAData, Op->Desired);

      __uint128_t Expected = Src1;
      bool Result = MemData->compare_exchange_strong(Expected, Src2);
      memcpy(GDP, Result ? &Src1 : &Expected, 16);
      break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown CAS size: {}", IROp->ElementSize); break;
  }
}

DEF_OP(CAS) {
  auto Op = IROp->C<IR::IROp_CAS>();
  uint8_t OpSize = IROp->Size;

  switch (OpSize) {
    case 1: {
      GD = AtomicCompareAndSwap(
        *GetSrc<uint8_t*>(Data->SSAData, Op->Expected),
        *GetSrc<uint8_t*>(Data->SSAData, Op->Desired),
        *GetSrc<uint8_t**>(Data->SSAData, Op->Addr)
      );
      break;
    }
    case 2: {
      GD = AtomicCompareAndSwap(
        *GetSrc<uint16_t*>(Data->SSAData, Op->Expected),
        *GetSrc<uint16_t*>(Data->SSAData, Op->Desired),
        *GetSrc<uint16_t**>(Data->SSAData, Op->Addr)
      );
      break;
    }
    case 4: {
      GD = AtomicCompareAndSwap(
        *GetSrc<uint32_t*>(Data->SSAData, Op->Expected),
        *GetSrc<uint32_t*>(Data->SSAData, Op->Desired),
        *GetSrc<uint32_t**>(Data->SSAData, Op->Addr)
      );
      break;
    }
    case 8: {
      GD = AtomicCompareAndSwap(
        *GetSrc<uint64_t*>(Data->SSAData, Op->Expected),
        *GetSrc<uint64_t*>(Data->SSAData, Op->Desired),
        *GetSrc<uint64_t**>(Data->SSAData, Op->Addr)
      );
      break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown CAS size: {}", OpSize); break;
  }
}

DEF_OP(AtomicAdd) {
  auto Op = IROp->C<IR::IROp_AtomicAdd>();
  switch (IROp->Size) {
    case 1: {
      std::atomic<uint8_t> *MemData = *GetSrc<std::atomic<uint8_t> **>(Data->SSAData, Op->Addr);
      uint8_t Src = *GetSrc<uint8_t*>(Data->SSAData, Op->Value);
      *MemData += Src;
      break;
    }
    case 2: {
      std::atomic<uint16_t> *MemData = *GetSrc<std::atomic<uint16_t> **>(Data->SSAData, Op->Addr);
      uint16_t Src = *GetSrc<uint16_t*>(Data->SSAData, Op->Value);
      *MemData += Src;
      break;
    }
    case 4: {
      std::atomic<uint32_t> *MemData = *GetSrc<std::atomic<uint32_t> **>(Data->SSAData, Op->Addr);
      uint32_t Src = *GetSrc<uint32_t*>(Data->SSAData, Op->Value);
      *MemData += Src;
      break;
    }
    case 8: {
      std::atomic<uint64_t> *MemData = *GetSrc<std::atomic<uint64_t> **>(Data->SSAData, Op->Addr);
      uint64_t Src = *GetSrc<uint64_t*>(Data->SSAData, Op->Value);
      *MemData += Src;
      break;
    }
    default:  LOGMAN_MSG_A_FMT("Unhandled Atomic size: {}", IROp->Size);
  }
}

DEF_OP(AtomicSub) {
  auto Op = IROp->C<IR::IROp_AtomicSub>();
  switch (IROp->Size) {
    case 1: {
      std::atomic<uint8_t> *MemData = *GetSrc<std::atomic<uint8_t> **>(Data->SSAData, Op->Addr);
      uint8_t Src = *GetSrc<uint8_t*>(Data->SSAData, Op->Value);
      *MemData -= Src;
      break;
    }
    case 2: {
      std::atomic<uint16_t> *MemData = *GetSrc<std::atomic<uint16_t> **>(Data->SSAData, Op->Addr);
      uint16_t Src = *GetSrc<uint16_t*>(Data->SSAData, Op->Value);
      *MemData -= Src;
      break;
    }
    case 4: {
      std::atomic<uint32_t> *MemData = *GetSrc<std::atomic<uint32_t> **>(Data->SSAData, Op->Addr);
      uint32_t Src = *GetSrc<uint32_t*>(Data->SSAData, Op->Value);
      *MemData -= Src;
      break;
    }
    case 8: {
      std::atomic<uint64_t> *MemData = *GetSrc<std::atomic<uint64_t> **>(Data->SSAData, Op->Addr);
      uint64_t Src = *GetSrc<uint64_t*>(Data->SSAData, Op->Value);
      *MemData -= Src;
      break;
    }
    default:  LOGMAN_MSG_A_FMT("Unhandled Atomic size: {}", IROp->Size);
  }
}

DEF_OP(AtomicAnd) {
  auto Op = IROp->C<IR::IROp_AtomicAnd>();
  switch (IROp->Size) {
    case 1: {
      std::atomic<uint8_t> *MemData = *GetSrc<std::atomic<uint8_t> **>(Data->SSAData, Op->Addr);
      uint8_t Src = *GetSrc<uint8_t*>(Data->SSAData, Op->Value);
      *MemData &= Src;
      break;
    }
    case 2: {
      std::atomic<uint16_t> *MemData = *GetSrc<std::atomic<uint16_t> **>(Data->SSAData, Op->Addr);
      uint16_t Src = *GetSrc<uint16_t*>(Data->SSAData, Op->Value);
      *MemData &= Src;
      break;
    }
    case 4: {
      std::atomic<uint32_t> *MemData = *GetSrc<std::atomic<uint32_t> **>(Data->SSAData, Op->Addr);
      uint32_t Src = *GetSrc<uint32_t*>(Data->SSAData, Op->Value);
      *MemData &= Src;
      break;
    }
    case 8: {
      std::atomic<uint64_t> *MemData = *GetSrc<std::atomic<uint64_t> **>(Data->SSAData, Op->Addr);
      uint64_t Src = *GetSrc<uint64_t*>(Data->SSAData, Op->Value);
      *MemData &= Src;
      break;
    }
    default:  LOGMAN_MSG_A_FMT("Unhandled Atomic size: {}", IROp->Size);
  }
}

DEF_OP(AtomicOr) {
  auto Op = IROp->C<IR::IROp_AtomicOr>();
  switch (IROp->Size) {
    case 1: {
      std::atomic<uint8_t> *MemData = *GetSrc<std::atomic<uint8_t> **>(Data->SSAData, Op->Addr);
      uint8_t Src = *GetSrc<uint8_t*>(Data->SSAData, Op->Value);
      *MemData |= Src;
      break;
    }
    case 2: {
      std::atomic<uint16_t> *MemData = *GetSrc<std::atomic<uint16_t> **>(Data->SSAData, Op->Addr);
      uint16_t Src = *GetSrc<uint16_t*>(Data->SSAData, Op->Value);
      *MemData |= Src;
      break;
    }
    case 4: {
      std::atomic<uint32_t> *MemData = *GetSrc<std::atomic<uint32_t> **>(Data->SSAData, Op->Addr);
      uint32_t Src = *GetSrc<uint32_t*>(Data->SSAData, Op->Value);
      *MemData |= Src;
      break;
    }
    case 8: {
      std::atomic<uint64_t> *MemData = *GetSrc<std::atomic<uint64_t> **>(Data->SSAData, Op->Addr);
      uint64_t Src = *GetSrc<uint64_t*>(Data->SSAData, Op->Value);
      *MemData |= Src;
      break;
    }
    default:  LOGMAN_MSG_A_FMT("Unhandled Atomic size: {}", IROp->Size);
  }
}

DEF_OP(AtomicXor) {
  auto Op = IROp->C<IR::IROp_AtomicXor>();
  switch (IROp->Size) {
    case 1: {
      std::atomic<uint8_t> *MemData = *GetSrc<std::atomic<uint8_t> **>(Data->SSAData, Op->Addr);
      uint8_t Src = *GetSrc<uint8_t*>(Data->SSAData, Op->Value);
      *MemData ^= Src;
      break;
    }
    case 2: {
      std::atomic<uint16_t> *MemData = *GetSrc<std::atomic<uint16_t> **>(Data->SSAData, Op->Addr);
      uint16_t Src = *GetSrc<uint16_t*>(Data->SSAData, Op->Value);
      *MemData ^= Src;
      break;
    }
    case 4: {
      std::atomic<uint32_t> *MemData = *GetSrc<std::atomic<uint32_t> **>(Data->SSAData, Op->Addr);
      uint32_t Src = *GetSrc<uint32_t*>(Data->SSAData, Op->Value);
      *MemData ^= Src;
      break;
    }
    case 8: {
      std::atomic<uint64_t> *MemData = *GetSrc<std::atomic<uint64_t> **>(Data->SSAData, Op->Addr);
      uint64_t Src = *GetSrc<uint64_t*>(Data->SSAData, Op->Value);
      *MemData ^= Src;
      break;
    }
    default:  LOGMAN_MSG_A_FMT("Unhandled Atomic size: {}", IROp->Size);
  }
}

DEF_OP(AtomicSwap) {
  auto Op = IROp->C<IR::IROp_AtomicSwap>();
  switch (IROp->Size) {
    case 1: {
      std::atomic<uint8_t> *MemData = *GetSrc<std::atomic<uint8_t> **>(Data->SSAData, Op->Addr);
      uint8_t Src = *GetSrc<uint8_t*>(Data->SSAData, Op->Value);
      uint8_t Previous = MemData->exchange(Src);
      GD = Previous;
      break;
    }
    case 2: {
      std::atomic<uint16_t> *MemData = *GetSrc<std::atomic<uint16_t> **>(Data->SSAData, Op->Addr);
      uint16_t Src = *GetSrc<uint16_t*>(Data->SSAData, Op->Value);
      uint16_t Previous = MemData->exchange(Src);
      GD = Previous;
      break;
    }
    case 4: {
      std::atomic<uint32_t> *MemData = *GetSrc<std::atomic<uint32_t> **>(Data->SSAData, Op->Addr);
      uint32_t Src = *GetSrc<uint32_t*>(Data->SSAData, Op->Value);
      uint32_t Previous = MemData->exchange(Src);
      GD = Previous;
      break;
    }
    case 8: {
      std::atomic<uint64_t> *MemData = *GetSrc<std::atomic<uint64_t> **>(Data->SSAData, Op->Addr);
      uint64_t Src = *GetSrc<uint64_t*>(Data->SSAData, Op->Value);
      uint64_t Previous = MemData->exchange(Src);
      GD = Previous;
      break;
    }
    default:  LOGMAN_MSG_A_FMT("Unhandled Atomic size: {}", IROp->Size);
  }
}

DEF_OP(AtomicFetchAdd) {
  auto Op = IROp->C<IR::IROp_AtomicFetchAdd>();
  switch (IROp->Size) {
    case 1: {
      std::atomic<uint8_t> *MemData = *GetSrc<std::atomic<uint8_t> **>(Data->SSAData, Op->Addr);
      uint8_t Src = *GetSrc<uint8_t*>(Data->SSAData, Op->Value);
      uint8_t Previous = MemData->fetch_add(Src);
      GD = Previous;
      break;
    }
    case 2: {
      std::atomic<uint16_t> *MemData = *GetSrc<std::atomic<uint16_t> **>(Data->SSAData, Op->Addr);
      uint16_t Src = *GetSrc<uint16_t*>(Data->SSAData, Op->Value);
      uint16_t Previous = MemData->fetch_add(Src);
      GD = Previous;
      break;
    }
    case 4: {
      std::atomic<uint32_t> *MemData = *GetSrc<std::atomic<uint32_t> **>(Data->SSAData, Op->Addr);
      uint32_t Src = *GetSrc<uint32_t*>(Data->SSAData, Op->Value);
      uint32_t Previous = MemData->fetch_add(Src);
      GD = Previous;
      break;
    }
    case 8: {
      std::atomic<uint64_t> *MemData = *GetSrc<std::atomic<uint64_t> **>(Data->SSAData, Op->Addr);
      uint64_t Src = *GetSrc<uint64_t*>(Data->SSAData, Op->Value);
      uint64_t Previous = MemData->fetch_add(Src);
      GD = Previous;
      break;
    }
    default:  LOGMAN_MSG_A_FMT("Unhandled Atomic size: {}", IROp->Size);
  }
}

DEF_OP(AtomicFetchSub) {
  auto Op = IROp->C<IR::IROp_AtomicFetchSub>();
  switch (IROp->Size) {
    case 1: {
      std::atomic<uint8_t> *MemData = *GetSrc<std::atomic<uint8_t> **>(Data->SSAData, Op->Addr);
      uint8_t Src = *GetSrc<uint8_t*>(Data->SSAData, Op->Value);
      uint8_t Previous = MemData->fetch_sub(Src);
      GD = Previous;
      break;
    }
    case 2: {
      std::atomic<uint16_t> *MemData = *GetSrc<std::atomic<uint16_t> **>(Data->SSAData, Op->Addr);
      uint16_t Src = *GetSrc<uint16_t*>(Data->SSAData, Op->Value);
      uint16_t Previous = MemData->fetch_sub(Src);
      GD = Previous;
      break;
    }
    case 4: {
      std::atomic<uint32_t> *MemData = *GetSrc<std::atomic<uint32_t> **>(Data->SSAData, Op->Addr);
      uint32_t Src = *GetSrc<uint32_t*>(Data->SSAData, Op->Value);
      uint32_t Previous = MemData->fetch_sub(Src);
      GD = Previous;
      break;
    }
    case 8: {
      std::atomic<uint64_t> *MemData = *GetSrc<std::atomic<uint64_t> **>(Data->SSAData, Op->Addr);
      uint64_t Src = *GetSrc<uint64_t*>(Data->SSAData, Op->Value);
      uint64_t Previous = MemData->fetch_sub(Src);
      GD = Previous;
      break;
    }
    default:  LOGMAN_MSG_A_FMT("Unhandled Atomic size: {}", IROp->Size);
  }
}

DEF_OP(AtomicFetchAnd) {
  auto Op = IROp->C<IR::IROp_AtomicFetchAnd>();
  switch (IROp->Size) {
    case 1: {
      std::atomic<uint8_t> *MemData = *GetSrc<std::atomic<uint8_t> **>(Data->SSAData, Op->Addr);
      uint8_t Src = *GetSrc<uint8_t*>(Data->SSAData, Op->Value);
      uint8_t Previous = MemData->fetch_and(Src);
      GD = Previous;
      break;
    }
    case 2: {
      std::atomic<uint16_t> *MemData = *GetSrc<std::atomic<uint16_t> **>(Data->SSAData, Op->Addr);
      uint16_t Src = *GetSrc<uint16_t*>(Data->SSAData, Op->Value);
      uint16_t Previous = MemData->fetch_and(Src);
      GD = Previous;
      break;
    }
    case 4: {
      std::atomic<uint32_t> *MemData = *GetSrc<std::atomic<uint32_t> **>(Data->SSAData, Op->Addr);
      uint32_t Src = *GetSrc<uint32_t*>(Data->SSAData, Op->Value);
      uint32_t Previous = MemData->fetch_and(Src);
      GD = Previous;
      break;
    }
    case 8: {
      std::atomic<uint64_t> *MemData = *GetSrc<std::atomic<uint64_t> **>(Data->SSAData, Op->Addr);
      uint64_t Src = *GetSrc<uint64_t*>(Data->SSAData, Op->Value);
      uint64_t Previous = MemData->fetch_and(Src);
      GD = Previous;
      break;
    }
    default:  LOGMAN_MSG_A_FMT("Unhandled Atomic size: {}", IROp->Size);
  }
}

DEF_OP(AtomicFetchOr) {
  auto Op = IROp->C<IR::IROp_AtomicFetchOr>();
  switch (IROp->Size) {
    case 1: {
      std::atomic<uint8_t> *MemData = *GetSrc<std::atomic<uint8_t> **>(Data->SSAData, Op->Addr);
      uint8_t Src = *GetSrc<uint8_t*>(Data->SSAData, Op->Value);
      uint8_t Previous = MemData->fetch_or(Src);
      GD = Previous;
      break;
    }
    case 2: {
      std::atomic<uint16_t> *MemData = *GetSrc<std::atomic<uint16_t> **>(Data->SSAData, Op->Addr);
      uint16_t Src = *GetSrc<uint16_t*>(Data->SSAData, Op->Value);
      uint16_t Previous = MemData->fetch_or(Src);
      GD = Previous;
      break;
    }
    case 4: {
      std::atomic<uint32_t> *MemData = *GetSrc<std::atomic<uint32_t> **>(Data->SSAData, Op->Addr);
      uint32_t Src = *GetSrc<uint32_t*>(Data->SSAData, Op->Value);
      uint32_t Previous = MemData->fetch_or(Src);
      GD = Previous;
      break;
    }
    case 8: {
      std::atomic<uint64_t> *MemData = *GetSrc<std::atomic<uint64_t> **>(Data->SSAData, Op->Addr);
      uint64_t Src = *GetSrc<uint64_t*>(Data->SSAData, Op->Value);
      uint64_t Previous = MemData->fetch_or(Src);
      GD = Previous;
      break;
    }
    default:  LOGMAN_MSG_A_FMT("Unhandled Atomic size: {}", IROp->Size);
  }
}

DEF_OP(AtomicFetchXor) {
  auto Op = IROp->C<IR::IROp_AtomicFetchXor>();
  switch (IROp->Size) {
    case 1: {
      std::atomic<uint8_t> *MemData = *GetSrc<std::atomic<uint8_t> **>(Data->SSAData, Op->Addr);
      uint8_t Src = *GetSrc<uint8_t*>(Data->SSAData, Op->Value);
      uint8_t Previous = MemData->fetch_xor(Src);
      GD = Previous;
      break;
    }
    case 2: {
      std::atomic<uint16_t> *MemData = *GetSrc<std::atomic<uint16_t> **>(Data->SSAData, Op->Addr);
      uint16_t Src = *GetSrc<uint16_t*>(Data->SSAData, Op->Value);
      uint16_t Previous = MemData->fetch_xor(Src);
      GD = Previous;
      break;
    }
    case 4: {
      std::atomic<uint32_t> *MemData = *GetSrc<std::atomic<uint32_t> **>(Data->SSAData, Op->Addr);
      uint32_t Src = *GetSrc<uint32_t*>(Data->SSAData, Op->Value);
      uint32_t Previous = MemData->fetch_xor(Src);
      GD = Previous;
      break;
    }
    case 8: {
      std::atomic<uint64_t> *MemData = *GetSrc<std::atomic<uint64_t> **>(Data->SSAData, Op->Addr);
      uint64_t Src = *GetSrc<uint64_t*>(Data->SSAData, Op->Value);
      uint64_t Previous = MemData->fetch_xor(Src);
      GD = Previous;
      break;
    }
    default:  LOGMAN_MSG_A_FMT("Unhandled Atomic size: {}", IROp->Size);
  }
}

DEF_OP(AtomicFetchNeg) {
  auto Op = IROp->C<IR::IROp_AtomicFetchNeg>();
  switch (IROp->Size) {
    case 1: {
      using Type = uint8_t;
      GD = AtomicFetchNeg(*GetSrc<Type**>(Data->SSAData, Op->Addr));
      break;
    }
    case 2: {
      using Type = uint16_t;
      GD = AtomicFetchNeg(*GetSrc<Type**>(Data->SSAData, Op->Addr));
      break;
    }
    case 4: {
      using Type = uint32_t;
      GD = AtomicFetchNeg(*GetSrc<Type**>(Data->SSAData, Op->Addr));
      break;
    }
    case 8: {
      using Type = uint64_t;
      GD = AtomicFetchNeg(*GetSrc<Type**>(Data->SSAData, Op->Addr));
      break;
    }
    default:  LOGMAN_MSG_A_FMT("Unhandled Atomic size: {}", IROp->Size);
  }
}
DEF_OP(TelemetrySetValue) {
#ifndef FEX_DISABLE_TELEMETRY
  auto Op = IROp->C<IR::IROp_TelemetrySetValue>();
  uint64_t Src = *GetSrc<uint64_t*>(Data->SSAData, Op->Value);

  auto TelemetryPtr = reinterpret_cast<std::atomic<uint64_t>*>(Data->State->CurrentFrame->Pointers.Common.TelemetryValueAddresses[Op->TelemetryValueIndex]);
  uint64_t Set{};
  if (Src != 0) {
    Set = 1;
  }

  *TelemetryPtr |= Set;
#endif
}

#undef DEF_OP

} // namespace FEXCore::CPU
