// SPDX-License-Identifier: MIT
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/ArchHelpers/Arm64.h>
#include <stdint.h>

namespace FEXCore::ArchHelpers::Arm64 {

#ifndef _M_ARM_64
// These are stub implementations that exist only to allow instantiating the arm64 jit
// on non arm platforms.

// Obvously such a configuration can't do the actual arm64-specific stuff

bool HandleCASPAL(void *_ucontext, void *_info, uint32_t Instr) {
    ERROR_AND_DIE_FMT("HandleCASPAL Not Implemented");
}

bool HandleCASAL(void *_ucontext, void *_info, uint32_t Instr) {
    ERROR_AND_DIE_FMT("HandleCASAL Not Implemented");
}

bool HandleAtomicMemOp(void *_ucontext, void *_info, uint32_t Instr) {
    ERROR_AND_DIE_FMT("HandleAtomicMemOp Not Implemented");
}

std::pair<bool, int32_t> HandleUnalignedAccess(bool ParanoidTSO, uintptr_t ProgramCounter, uint64_t *GPRs) {
  ERROR_AND_DIE_FMT("HandleAtomicMemOp Not Implemented");
}

#endif

}
