/*
$info$
tags: backend|interpreter
$end_info$
*/

#include "Interface/Core/Interpreter/InterpreterClass.h"
#include "Interface/Core/Interpreter/InterpreterOps.h"
#include "Interface/Core/Interpreter/InterpreterDefines.h"

#include <FEXHeaderUtils/Syscalls.h>

#include <cstdint>
#ifdef _M_X86_64
#include <xmmintrin.h>
#endif

namespace FEXCore::CPU {
[[noreturn]]
static void StopThread(FEXCore::Core::InternalThreadState *Thread) {
  Thread->CTX->StopThread(Thread);

  LOGMAN_MSG_A_FMT("unreachable");
  FEX_UNREACHABLE;
}

#define DEF_OP(x) void InterpreterOps::Op_##x(IR::IROp_Header *IROp, IROpData *Data, IR::NodeID Node)
DEF_OP(Fence) {
  auto Op = IROp->C<IR::IROp_Fence>();
  switch (Op->Fence) {
    case IR::Fence_Load.Val:
      std::atomic_thread_fence(std::memory_order_acquire);
      break;
    case IR::Fence_LoadStore.Val:
      std::atomic_thread_fence(std::memory_order_seq_cst);
      break;
    case IR::Fence_Store.Val:
      std::atomic_thread_fence(std::memory_order_release);
      break;
    default: LOGMAN_MSG_A_FMT("Unknown Fence: {}", Op->Fence); break;
  }
}

DEF_OP(Break) {
  auto Op = IROp->C<IR::IROp_Break>();
  switch (Op->Reason) {
    case FEXCore::IR::Break_Halt: // HLT
      StopThread(Data->State);
    break;
    case FEXCore::IR::Break_InvalidInstruction:
      FHU::Syscalls::tgkill(Data->State->ThreadManager.PID, Data->State->ThreadManager.TID, SIGILL);
    break;
  default: LOGMAN_MSG_A_FMT("Unknown Break Reason: {}", Op->Reason); break;
  }
}

DEF_OP(GetRoundingMode) {
  uint32_t GuestRounding{};
#ifdef _M_ARM_64
  uint64_t Tmp{};
  __asm(R"(
    mrs %[Tmp], FPCR;
  )"
  : [Tmp] "=r" (Tmp));
  // Extract the rounding
  // On ARM the ordering is different than on x86
  GuestRounding |= ((Tmp >> 24) & 1) ? IR::ROUND_MODE_FLUSH_TO_ZERO : 0;
  uint8_t RoundingMode = (Tmp >> 22) & 0b11;
  if (RoundingMode == 0)
    GuestRounding |= IR::ROUND_MODE_NEAREST;
  else if (RoundingMode == 1)
    GuestRounding |= IR::ROUND_MODE_POSITIVE_INFINITY;
  else if (RoundingMode == 2)
    GuestRounding |= IR::ROUND_MODE_NEGATIVE_INFINITY;
  else if (RoundingMode == 3)
    GuestRounding |= IR::ROUND_MODE_TOWARDS_ZERO;
#else
  GuestRounding = _mm_getcsr();

  // Extract the rounding
  GuestRounding = (GuestRounding >> 13) & 0b111;
#endif
  memcpy(GDP, &GuestRounding, sizeof(GuestRounding));
}

DEF_OP(SetRoundingMode) {
  auto Op = IROp->C<IR::IROp_SetRoundingMode>();
  uint8_t GuestRounding = *GetSrc<uint8_t*>(Data->SSAData, Op->Header.Args[0]);
#ifdef _M_ARM_64
  uint64_t HostRounding{};
  __asm volatile(R"(
    mrs %[Tmp], FPCR;
  )"
  : [Tmp] "=r" (HostRounding));
  // Mask out the rounding
  HostRounding &= ~(0b111 << 22);

  HostRounding |= (GuestRounding & IR::ROUND_MODE_FLUSH_TO_ZERO) ? (1U << 24) : 0;

  uint8_t RoundingMode = GuestRounding & 0b11;
  if (RoundingMode == IR::ROUND_MODE_NEAREST)
    HostRounding |= (0b00U << 22);
  else if (RoundingMode == IR::ROUND_MODE_POSITIVE_INFINITY)
    HostRounding |= (0b01U << 22);
  else if (RoundingMode == IR::ROUND_MODE_NEGATIVE_INFINITY)
    HostRounding |= (0b10U << 22);
  else if (RoundingMode == IR::ROUND_MODE_TOWARDS_ZERO)
    HostRounding |= (0b11U << 22);

  __asm volatile(R"(
    msr FPCR, %[Tmp];
  )"
  :: [Tmp] "r" (HostRounding));
#else
  uint32_t HostRounding = _mm_getcsr();

  // Cut out the host rounding mode
  HostRounding &= ~(0b111 << 13);

  // Insert our new rounding mode
  HostRounding |= GuestRounding << 13;
  _mm_setcsr(HostRounding);
#endif
}

DEF_OP(Print) {
  auto Op = IROp->C<IR::IROp_Print>();
  uint8_t OpSize = IROp->Size;

  if (OpSize <= 8) {
    uint64_t Src = *GetSrc<uint64_t*>(Data->SSAData, Op->Header.Args[0]);
    LogMan::Msg::IFmt(">>>> Value in Arg: 0x{:x}, {}", Src, Src);
  }
  else if (OpSize == 16) {
    __uint128_t Src = *GetSrc<__uint128_t*>(Data->SSAData, Op->Header.Args[0]);
    uint64_t Src0 = Src;
    uint64_t Src1 = Src >> 64;
    LogMan::Msg::IFmt(">>>> Value[0] in Arg: 0x{:x}, {}", Src0, Src0);
    LogMan::Msg::IFmt("     Value[1] in Arg: 0x{:x}, {}", Src1, Src1);
  }
  else
    LOGMAN_MSG_A_FMT("Unknown value size: {}", OpSize);
}

DEF_OP(ProcessorID) {
  uint32_t CPU, CPUNode;
  FHU::Syscalls::getcpu(&CPU, &CPUNode);
  GD = (CPUNode << 12) | CPU;
}

#undef DEF_OP

} // namespace FEXCore::CPU
