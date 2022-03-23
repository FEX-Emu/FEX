#pragma once

#include <stdint.h>

namespace FEXCore::ArchHelpers::RISCV {
  constexpr uint32_t AMO_OP_MASK    = 0xFE00007FU;
  constexpr uint32_t AMO_OP_LR_AQRL = 0x1600002FU;
  constexpr uint32_t AMO_OP_SC_AQRL = 0x1E00002FU;

  constexpr uint32_t IMM_OP_MASK     = 0xFE00707F;
  constexpr uint32_t IMM_SLLI64_MASK = 0x00001013;
  constexpr uint32_t IMM_SRLI64_MASK = 0x00005013;

  enum class AtomicOperation {
    OP_NONE,
    OP_FETCHADD,
  };
  bool HandleAtomicStore(void *_ucontext, void *_info, uint32_t Instr);
  bool HandleAtomicLoad(void *_ucontext, void *_info, uint32_t Instr);

  AtomicOperation FindAtomicOperationType(uint32_t *PC);
  [[nodiscard]] bool HandleSIGBUS(bool ParanoidTSO, int Signal, void *info, void *ucontext);
}
