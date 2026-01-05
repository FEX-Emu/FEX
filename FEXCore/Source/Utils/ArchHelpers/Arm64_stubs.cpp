// SPDX-License-Identifier: MIT
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/ArchHelpers/Arm64.h>
#include <stdint.h>

namespace FEXCore::ArchHelpers::Arm64 {

#ifndef ARCHITECTURE_arm64
// These are stub implementations that exist only to allow instantiating the arm64 jit
// on non arm platforms.

// Obvously such a configuration can't do the actual arm64-specific stuff

std::optional<int32_t>
HandleUnalignedAccess(FEXCore::Core::InternalThreadState* Thread, UnalignedHandlerType HandleType, uintptr_t ProgramCounter, uint64_t* GPRs) {
  ERROR_AND_DIE_FMT("HandleAtomicMemOp Not Implemented");
}

#endif

} // namespace FEXCore::ArchHelpers::Arm64
