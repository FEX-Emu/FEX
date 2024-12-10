// SPDX-License-Identifier: MIT
#pragma once

#include "IntervalList.h"
#include <mutex>
#include <unordered_map>

namespace FEXCore::Core {
struct InternalThreadState;
}

namespace FEXCore::Context {
class Context;
}

namespace FEX::Windows {
/**
 * @brief Handles SMC and regular code invalidation
 */
class InvalidationTracker {
public:
  InvalidationTracker(FEXCore::Context::Context& CTX, const std::unordered_map<DWORD, FEXCore::Core::InternalThreadState*>& Threads);
  void HandleMemoryProtectionNotification(uint64_t Address, uint64_t Size, ULONG Prot);
  void HandleImageMap(uint64_t Address);
  void InvalidateContainingSection(uint64_t Address, bool Free);
  void InvalidateAlignedInterval(uint64_t Address, uint64_t Size, bool Free);
  void ReprotectRWXIntervals(uint64_t Address, uint64_t Size);
  bool HandleRWXAccessViolation(uint64_t FaultAddress);

private:
  IntervalList<uint64_t> RWXIntervals;
  std::mutex RWXIntervalsLock;
  FEXCore::Context::Context& CTX;
  const std::unordered_map<DWORD, FEXCore::Core::InternalThreadState*>& Threads;
};
} // namespace FEX::Windows
