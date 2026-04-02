// SPDX-License-Identifier: MIT
#pragma once
#include <cstddef>

namespace FEXCore::Core {
struct InternalThreadState;
}

namespace FEXCore::Allocator {
void InitializeAllocator(size_t PageSize);
void LockBeforeFork(FEXCore::Core::InternalThreadState* Thread);
void UnlockAfterFork(FEXCore::Core::InternalThreadState* Thread, bool Child);
} // namespace FEXCore::Allocator
