// SPDX-License-Identifier: MIT
// FIXME TODO put in cpp
#pragma once

#include "IntervalList.h"
#include <mutex>

namespace FEXCore::Core {
  struct InternalThreadState;
}

namespace FEX::Windows {
/**
 * @brief Handles SMC and regular code invalidation
 */
class InvalidationTracker {
public:
  void HandleMemoryProtectionNotification(FEXCore::Core::InternalThreadState *Thread, uint64_t Address, uint64_t Size, ULONG Prot);
  void InvalidateContainingSection(FEXCore::Core::InternalThreadState *Thread, uint64_t Address, bool Free);
  void InvalidateAlignedInterval(FEXCore::Core::InternalThreadState *Thread, uint64_t Address, uint64_t Size, bool Free);
  void ReprotectRWXIntervals(uint64_t Address, uint64_t Size);
  bool HandleRWXAccessViolation(FEXCore::Core::InternalThreadState *Thread, uint64_t FaultAddress);

private:
  IntervalList<uint64_t> RWXIntervals;
  std::mutex RWXIntervalsLock;
};
}
