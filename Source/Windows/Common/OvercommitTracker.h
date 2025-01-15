// SPDX-License-Identifier: MIT
#pragma once

#include "IntervalList.h"
#include <thread>
#include <shared_mutex>


namespace FEX::Windows {
/**
 * @brief Emulates memory overcommit of reserved regions with exceptions
 */
class OvercommitTracker {
private:
  bool IsWine;
  IntervalList<uint64_t> OvercommitIntervals;
  std::shared_mutex OvercommitIntervalsMutex;

public:
  OvercommitTracker(bool IsWine)
    : IsWine {IsWine} {}

  void MarkRange(uint64_t Start, uint64_t Length) {
    std::unique_lock Lock {OvercommitIntervalsMutex};
    OvercommitIntervals.Insert({Start, Start + Length});
  }

  void UnmarkRange(uint64_t Start, uint64_t Length) {
    std::unique_lock Lock {OvercommitIntervalsMutex};
    OvercommitIntervals.Remove({Start, Start + Length});
  }

  bool HandleAccessViolation(uint64_t FaultAddress) {
    std::shared_lock Lock {OvercommitIntervalsMutex};
    auto Query = OvercommitIntervals.Query(FaultAddress);

    if (Query.Enclosed) {
      if (IsWine) {
        MEMORY_BASIC_INFORMATION Info;
        NtQueryVirtualMemory(NtCurrentProcess(), reinterpret_cast<void*>(FaultAddress), MemoryBasicInformation, &Info, sizeof(Info), nullptr);
        const auto CommitSize = reinterpret_cast<SIZE_T>(Info.BaseAddress) + Info.RegionSize - reinterpret_cast<SIZE_T>(Info.AllocationBase);
        VirtualAlloc(reinterpret_cast<void*>(Info.AllocationBase), CommitSize, MEM_COMMIT, PAGE_READWRITE);
      } else {
        static constexpr size_t MaxFaultCommitSize = 1024 * 64;
        const auto AlignedFaultAddress = reinterpret_cast<void*>(FaultAddress & FEXCore::Utils::FEX_PAGE_MASK);
        VirtualAlloc(AlignedFaultAddress, std::min(Query.Size, MaxFaultCommitSize), MEM_COMMIT, PAGE_READWRITE);
      }
      return true;
    }
    return false;
  }
};
} // namespace FEX::Windows
