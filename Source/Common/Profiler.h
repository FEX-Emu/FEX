// SPDX-License-Identifier: MIT
/*
$info$
tags: Common|Profiler
desc: Frontend profiler common code
$end_info$
*/
#pragma once
#include <FEXCore/Utils/Profiler.h>

namespace FEXCore::Core {
struct InternalThreadState;
}

#ifdef _M_ARM_64
static inline void memory_barrier() {
  asm volatile("dmb ishst;" ::: "memory");
}

#else
static inline void memory_barrier() {
  // Intentionally empty.
}
#endif

namespace FEX::Profiler {
class StatAllocBase {
public:
  virtual ~StatAllocBase() = default;

protected:
  FEXCore::Profiler::ThreadStats* AllocateBaseSlot(uint32_t TID);
  void DeallocateBaseSlot(FEXCore::Profiler::ThreadStats* AllocatedSlot);

  uint32_t OffsetFromStat(FEXCore::Profiler::ThreadStats* Stat) const {
    return reinterpret_cast<uint64_t>(Stat) - reinterpret_cast<uint64_t>(Base);
  }
  size_t TotalSlotsFromSize() const {
    return (CurrentSize - sizeof(FEXCore::Profiler::ThreadStatsHeader)) / sizeof(FEXCore::Profiler::ThreadStats) - 1;
  }
  size_t SlotIndexFromOffset(uint32_t Offset) {
    return (Offset - sizeof(FEXCore::Profiler::ThreadStatsHeader)) / sizeof(FEXCore::Profiler::ThreadStats);
  }

  void SaveHeader(FEXCore::Profiler::AppType AppType);

  void* Base;
  size_t CurrentSize {};
  FEXCore::Profiler::ThreadStatsHeader* Head {};
  FEXCore::Profiler::ThreadStats* Stats;
  FEXCore::Profiler::ThreadStats* StatTail {};
  uint64_t RemainingSlots;

  // Limited to 4MB which should be a few hundred threads of tracking capability.
  // I (Sonicadvance1) wanted to reserve 128MB of VA space because it's cheap, but ran in to a bug when running WINE.
  // WINE allocates [0x7fff'fe00'0000, 0x7fff'ffff'0000) which /consistently/ overlaps with FEX's sigaltstack.
  // This only occurs when this stat allocation size is large as the top-down allocation pushes the alt-stack further.
  // Additionally, only occurs on 48-bit VA systems, as mmap on lesser VA will fail regardless.
  // TODO: Bump allocation size up once FEXCore's allocator can first use the 128TB of blocked VA space on 48-bit systems.
  constexpr static size_t MAX_STATS_SIZE = 4 * 1024 * 1024;

private:
  virtual uint64_t AllocateMoreSlots(uint64_t NewSize) = 0;
  bool AllocateMoreSlots();
};

} // namespace FEX::Profiler
