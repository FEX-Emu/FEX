// SPDX-License-Identifier: MIT
/*
$info$
tags: backend|interpreter
$end_info$
*/

#include "Interface/Context/Context.h"

#include "Interface/Core/Interpreter/InterpreterClass.h"
#include "Interface/Core/Interpreter/InterpreterOps.h"
#include "Interface/Core/Interpreter/InterpreterDefines.h"

#include <FEXHeaderUtils/Syscalls.h>

#include <cstdint>
#ifdef _M_X86_64
#include <xmmintrin.h>
#endif
#include <sys/random.h>

namespace FEXCore::CPU {

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

  Data->State->CurrentFrame->SynchronousFaultData.FaultToTopAndGeneratedException = 1;
  Data->State->CurrentFrame->SynchronousFaultData.Signal = Op->Reason.Signal;
  Data->State->CurrentFrame->SynchronousFaultData.TrapNo = Op->Reason.TrapNumber;
  Data->State->CurrentFrame->SynchronousFaultData.err_code = Op->Reason.ErrorRegister;
  Data->State->CurrentFrame->SynchronousFaultData.si_code = Op->Reason.si_code;

  switch (Op->Reason.Signal) {
  case SIGILL:
    FHU::Syscalls::tgkill(Data->State->ThreadManager.PID, Data->State->ThreadManager.TID, SIGILL);
    break;
  case SIGTRAP:
    FHU::Syscalls::tgkill(Data->State->ThreadManager.PID, Data->State->ThreadManager.TID, SIGTRAP);
    break;
  case SIGSEGV:
    FHU::Syscalls::tgkill(Data->State->ThreadManager.PID, Data->State->ThreadManager.TID, SIGSEGV);
    break;
  default:
    FHU::Syscalls::tgkill(Data->State->ThreadManager.PID, Data->State->ThreadManager.TID, SIGTRAP);
    break;
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
  const auto GuestRounding = *GetSrc<uint8_t*>(Data->SSAData, Op->RoundMode);
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
  const uint8_t OpSize = IROp->Size;

  if (OpSize <= 8) {
    const auto Src = *GetSrc<uint64_t*>(Data->SSAData, Op->Value);
    LogMan::Msg::IFmt(">>>> Value in Arg: 0x{:x}, {}", Src, Src);
  }
  else if (OpSize == 16) {
    const auto Src = *GetSrc<__uint128_t*>(Data->SSAData, Op->Value);
    const uint64_t Src0 = Src;
    const uint64_t Src1 = Src >> 64;
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

DEF_OP(RDRAND) {
  // We are ignoring Op->GetReseeded in the interpreter
  uint64_t *DstPtr = GetDest<uint64_t*>(Data->SSAData, Node);
  ssize_t Result = ::getrandom(&DstPtr[0], 8, 0);

  // Second result is if we managed to read a valid random number or not
  DstPtr[1] = Result == 8 ? 1 : 0;
}

DEF_OP(Yield) {
  // Nop implementation
}

#undef DEF_OP

} // namespace FEXCore::CPU
