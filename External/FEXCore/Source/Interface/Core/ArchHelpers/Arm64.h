#pragma once

#include <stdint.h>

namespace FEXCore::ArchHelpers::Arm64 {
  constexpr uint32_t CASPAL_MASK = 0xBF'E0'FC'00;
  constexpr uint32_t CASPAL_INST = 0x08'60'FC'00;

  constexpr uint32_t CASAL_MASK = 0x3F'E0'FC'00;
  constexpr uint32_t CASAL_INST = 0x08'E0'FC'00;

  bool HandleCASPAL(void *_mcontext, void *_info, uint32_t Instr);
  bool HandleCASAL(void *_mcontext, void *_info, uint32_t Instr);
}
