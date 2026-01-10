// SPDX-License-Identifier: MIT
#pragma once

namespace FEX::SBRKAllocations {
// Disable allocations through glibc's sbrk allocation method.
// Returns a pointer at the end of the sbrk region.
void* DisableSBRKAllocations();

// Allow sbrk again. Pass in the pointer returned by `DisableSBRKAllocations`
void ReenableSBRKAllocations(void* Ptr);
} // namespace FEX::SBRKAllocations
