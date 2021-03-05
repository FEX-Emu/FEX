#include "Interface/Core/ArchHelpers/Arm64.h"

#include <FEXCore/Utils/LogManager.h>

namespace FEXCore::ArchHelpers::Arm64 {

#ifndef _M_ARM_64
// These are stub implementations that exist only to allow instantiating the arm64 jit
// on non arm platforms.

// Obvously such a configuration can't do the actual arm64-specific stuff

bool HandleCASPAL(void *_ucontext, void *_info, uint32_t Instr) {
    ERROR_AND_DIE("HandleCASPAL Not Implemented");
}

bool HandleCASAL(void *_ucontext, void *_info, uint32_t Instr) {
    ERROR_AND_DIE("HandleCASAL Not Implemented");
}

bool HandleAtomicMemOp(void *_ucontext, void *_info, uint32_t Instr) {
    ERROR_AND_DIE("HandleAtomicMemOp Not Implemented");
}
#endif

}