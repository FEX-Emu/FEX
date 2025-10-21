// SPDX-License-Identifier: MIT
#include "Common/SHMStats.h"
#include "git_version.h"

#include <FEXCore/Debug/InternalThreadState.h>

namespace FEX::SHMStats {
void StatAllocBase::SaveHeader(FEXCore::SHMStats::AppType AppType) {
  if (!Base) {
    return;
  }

  Head = reinterpret_cast<FEXCore::SHMStats::ThreadStatsHeader*>(Base);
  Head->Size.store(CurrentSize, std::memory_order_relaxed);
  Head->Version = FEXCore::SHMStats::STATS_VERSION;
  Head->app_type = AppType;
  Head->ThreadStatsSize = sizeof(FEXCore::SHMStats::ThreadStats);

  std::string_view GitString = GIT_DESCRIBE_STRING;
  strncpy(Head->fex_version, GitString.data(), std::min(GitString.size(), sizeof(Head->fex_version)));

  Stats = reinterpret_cast<FEXCore::SHMStats::ThreadStats*>(reinterpret_cast<uint64_t>(Base) + sizeof(FEXCore::SHMStats::ThreadStatsHeader));

  RemainingSlots = TotalSlotsFromSize();
}

bool StatAllocBase::AllocateMoreSlots() {
  const auto OriginalSlotCount = TotalSlotsFromSize();

  uint32_t NewSize = FrontendAllocateSlots(CurrentSize * 2);

  if (NewSize == CurrentSize) {
    return false;
  }

  CurrentSize = NewSize;
  Head->Size.store(CurrentSize, std::memory_order_relaxed);
  RemainingSlots = TotalSlotsFromSize() - OriginalSlotCount;

  return true;
}

FEXCore::SHMStats::ThreadStats* StatAllocBase::AllocateSlot(uint32_t TID) {
  if (!RemainingSlots) {
    if (!AllocateMoreSlots()) {
      return nullptr;
    }
  }

  // Find a free slot
  store_memory_barrier();
  FEXCore::SHMStats::ThreadStats* AllocatedSlot {};
  for (size_t i = 0; i < TotalSlotsFromSize(); ++i) {
    AllocatedSlot = &Stats[i];
    if (AllocatedSlot->TID.load(std::memory_order_relaxed) == 0) {
      break;
    }
  }

  --RemainingSlots;

  // Slot might be reused, just zero it now.
  memset(AllocatedSlot, 0, sizeof(*AllocatedSlot));

  // TID != 0 means slot is allocated.
  AllocatedSlot->TID.store(TID, std::memory_order_relaxed);

  // Setup singly-linked list
  if (Head->Head.load(std::memory_order_relaxed) == 0) {
    Head->Head.store(OffsetFromStat(AllocatedSlot), std::memory_order_relaxed);
  } else {
    StatTail->Next.store(OffsetFromStat(AllocatedSlot), std::memory_order_relaxed);
  }

  // Update the tail.
  StatTail = AllocatedSlot;
  return AllocatedSlot;
}

void StatAllocBase::DeallocateSlot(FEXCore::SHMStats::ThreadStats* AllocatedSlot) {
  if (!AllocatedSlot) {
    return;
  }

  // TID == 0 will signal the reader to ignore this slot & deallocate it!
  AllocatedSlot->TID.store(0, std::memory_order_relaxed);

  store_memory_barrier();

  const auto SlotOffset = OffsetFromStat(AllocatedSlot);
  const auto AllocatedSlotNext = AllocatedSlot->Next.load(std::memory_order_relaxed);

  const bool IsTail = AllocatedSlot == StatTail;

  // Update the linked list.
  if (Head->Head == SlotOffset) {
    Head->Head.store(AllocatedSlotNext, std::memory_order_relaxed);
    if (IsTail) {
      StatTail = nullptr;
    }
  } else {
    for (size_t i = 0; i < TotalSlotsFromSize(); ++i) {
      auto Slot = &Stats[i];
      auto NextSlotOffset = Slot->Next.load(std::memory_order_relaxed);

      if (NextSlotOffset == SlotOffset) {
        Slot->Next.store(AllocatedSlotNext, std::memory_order_relaxed);

        if (IsTail) {
          // This slot is now the tail.
          StatTail = Slot;
        }
        break;
      }
    }
  }

  ++RemainingSlots;
}

} // namespace FEX::SHMStats
