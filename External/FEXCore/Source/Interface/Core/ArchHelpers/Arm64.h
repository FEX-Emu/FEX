#pragma once

#include <stdint.h>

namespace FEXCore::ArchHelpers::Arm64 {
  constexpr uint32_t CASPAL_MASK = 0xBF'E0'FC'00;
  constexpr uint32_t CASPAL_INST = 0x08'60'FC'00;

  constexpr uint32_t CASAL_MASK = 0x3F'E0'FC'00;
  constexpr uint32_t CASAL_INST = 0x08'E0'FC'00;

  constexpr uint32_t ATOMIC_MEM_MASK = 0x3B200C00;
  constexpr uint32_t ATOMIC_MEM_INST = 0x38200000;

  // Load ops are 4 bits
  // Acquire and release bits are independent on the instruction
  constexpr uint32_t ATOMIC_ADD_OP  = 0b0000;
  constexpr uint32_t ATOMIC_CLR_OP  = 0b0001;
  constexpr uint32_t ATOMIC_EOR_OP  = 0b0010;
  constexpr uint32_t ATOMIC_SET_OP  = 0b0011;
  constexpr uint32_t ATOMIC_SMAX_OP = 0b0100;
  constexpr uint32_t ATOMIC_SMIN_OP = 0b0101;
  constexpr uint32_t ATOMIC_UMAX_OP = 0b0110;
  constexpr uint32_t ATOMIC_UMIN_OP = 0b0111;
  constexpr uint32_t ATOMIC_SWAP_OP = 0b1000;

  bool HandleCASPAL(void *_ucontext, void *_info, uint32_t Instr);
  bool HandleCASAL(void *_ucontext, void *_info, uint32_t Instr);
  bool HandleAtomicMemOp(void *_ucontext, void *_info, uint32_t Instr);
}
