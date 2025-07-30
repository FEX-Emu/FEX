// SPDX-License-Identifier: MIT
/*
$info$
tags: Common|SHMStats
desc: Frontend profiler common code
$end_info$
*/
#pragma once
#include <FEXCore/Utils/SHMStats.h>

namespace FEXCore::Core {
struct InternalThreadState;
}

#ifdef _M_ARM_64
static inline void store_memory_barrier() {
  asm volatile("dmb ishst;" ::: "memory");
}

#else
static inline void store_memory_barrier() {
  // Intentionally empty.
  // x86 is strongly memory ordered with regular loadstores. No need for barrier.
}
#endif

namespace FEX::SHMStats {
class StatAllocBase {
protected:
  FEXCore::SHMStats::ThreadStats* AllocateSlot(uint32_t TID);
  void DeallocateSlot(FEXCore::SHMStats::ThreadStats* AllocatedSlot);

  uint32_t OffsetFromStat(FEXCore::SHMStats::ThreadStats* Stat) const {
    return reinterpret_cast<uint64_t>(Stat) - reinterpret_cast<uint64_t>(Base);
  }
  uint32_t TotalSlotsFromSize() const {
    return (CurrentSize - sizeof(FEXCore::SHMStats::ThreadStatsHeader)) / sizeof(FEXCore::SHMStats::ThreadStats) - 1;
  }
  static uint32_t TotalSlotsFromSize(uint32_t Size) {
    return (Size - sizeof(FEXCore::SHMStats::ThreadStatsHeader)) / sizeof(FEXCore::SHMStats::ThreadStats) - 1;
  }

  static uint32_t SlotIndexFromOffset(uint32_t Offset) {
    return (Offset - sizeof(FEXCore::SHMStats::ThreadStatsHeader)) / sizeof(FEXCore::SHMStats::ThreadStats);
  }

  void SaveHeader(FEXCore::SHMStats::AppType AppType);

  void* Base {};
  uint32_t CurrentSize {};
  FEXCore::SHMStats::ThreadStatsHeader* Head {};
  FEXCore::SHMStats::ThreadStats* Stats {};
  FEXCore::SHMStats::ThreadStats* StatTail {};
  uint32_t RemainingSlots {};

  // Limited to 4MB which should be a few hundred threads of tracking capability.
  // I (Sonicadvance1) wanted to reserve 128MB of VA space because it's cheap, but ran in to a bug when running WINE.
  // WINE allocates [0x7fff'fe00'0000, 0x7fff'ffff'0000) which /consistently/ overlaps with FEX's sigaltstack.
  // This only occurs when this stat allocation size is large as the top-down allocation pushes the alt-stack further.
  // Additionally, only occurs on 48-bit VA systems, as mmap on lesser VA will fail regardless.
  // TODO: Bump allocation size up once FEXCore's allocator can first use the 128TB of blocked VA space on 48-bit systems.
  constexpr static uint32_t MAX_STATS_SIZE = 4 * 1024 * 1024;

private:
  virtual uint32_t FrontendAllocateSlots(uint32_t NewSize) = 0;
  bool AllocateMoreSlots();
};

} // namespace FEX::SHMStats
