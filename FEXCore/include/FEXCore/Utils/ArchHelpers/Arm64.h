// SPDX-License-Identifier: MIT
#pragma once

#include <FEXCore/Utils/CompilerDefs.h>

#include <stdint.h>
#include <utility>

namespace FEXCore::Core {
struct InternalThreadState;
}

namespace FEXCore::ArchHelpers::Arm64 {
enum class UnalignedHandlerType {
  ///< Don't backpatch code, instead handle inside SIGBUS handler.
  Paranoid,
  ///< Backpatch unaligned access to half-barrier based atomic.
  HalfBarrier,
  ///< Backpatch unaligned access to non-atomic.
  NonAtomic,
};

/**
 * @brief On ARM64 handles an unaligned memory access that the JIT has done.
 *
 * This is an OS agnostic handler where the frontend must provide FEXCore with the information necessary to know if this is safe.
 * This does not check if the PC is within a JIT code buffer, the frontend must provide that safety with `CPUBackend::IsAddressInCodeBuffer`.
 *
 * @param ParanoidTSO If the unaligned fault needs to handled directly or can be backpatched.
 * @param ProgramCounter The location in memory for the instruction that did the access
 * @param GPRs The array of GPRs from the signal context. This will be modified and the host context needs to be updated on signal return.
 *
 * @return A pair where the first element is if the unaligned access has been handle and the second element is how many bytes to modify the host PC
 * by. FEXCore will return a positive or negative offset depending on internal handling.
 */
[[nodiscard]]
FEX_DEFAULT_VISIBILITY std::pair<bool, int32_t>
HandleUnalignedAccess(FEXCore::Core::InternalThreadState* Thread, UnalignedHandlerType HandleType, uintptr_t ProgramCounter, uint64_t* GPRs);
} // namespace FEXCore::ArchHelpers::Arm64
