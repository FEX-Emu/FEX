// SPDX-License-Identifier: MIT
#pragma once

namespace FEXCore::Core {
struct InternalThreadState;
}

namespace FEXCore::Allocator {
  void LockBeforeFork(FEXCore::Core::InternalThreadState *Thread);
  void UnlockAfterFork(FEXCore::Core::InternalThreadState *Thread, bool Child);
}
