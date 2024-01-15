// SPDX-License-Identifier: MIT

#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Core/Context.h>
#include <FEXCore/Debug/InternalThreadState.h>
#include "InvalidationTracker.h"
#include <windef.h>
#include <winternl.h>

namespace FEX::Windows {
void InvalidationTracker::HandleMemoryProtectionNotification(FEXCore::Core::InternalThreadState *Thread, uint64_t Address, uint64_t Size, ULONG Prot) {
  const auto AlignedBase = Address & FHU::FEX_PAGE_MASK;
  const auto AlignedSize = (Address - AlignedBase + Size + FHU::FEX_PAGE_SIZE - 1) & FHU::FEX_PAGE_MASK;

  if (Prot & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE)) {
    Thread->CTX->InvalidateGuestCodeRange(Thread, AlignedBase, AlignedSize);
  }

  if (Prot & PAGE_EXECUTE_READWRITE) {
    LogMan::Msg::DFmt("Add SMC interval: {:X} - {:X}", AlignedBase, AlignedBase + AlignedSize);
    std::scoped_lock Lock(RWXIntervalsLock);
    RWXIntervals.Insert({AlignedBase, AlignedBase + AlignedSize});
  } else {
    std::scoped_lock Lock(RWXIntervalsLock);
    RWXIntervals.Remove({AlignedBase, AlignedBase + AlignedSize});
  }
}

void InvalidationTracker::InvalidateContainingSection(FEXCore::Core::InternalThreadState *Thread, uint64_t Address, bool Free) {
  MEMORY_BASIC_INFORMATION Info;
  if (NtQueryVirtualMemory(NtCurrentProcess(), reinterpret_cast<void *>(Address), MemoryBasicInformation, &Info, sizeof(Info), nullptr))
    return;

  const auto SectionBase = reinterpret_cast<uint64_t>(Info.AllocationBase);
  const auto SectionSize = reinterpret_cast<uint64_t>(Info.BaseAddress) + Info.RegionSize
                           - reinterpret_cast<uint64_t>(Info.AllocationBase);
  Thread->CTX->InvalidateGuestCodeRange(Thread, SectionBase, SectionSize);

  if (Free) {
    std::scoped_lock Lock(RWXIntervalsLock);
    RWXIntervals.Remove({SectionBase, SectionBase + SectionSize});
  }
}

void InvalidationTracker::InvalidateAlignedInterval(FEXCore::Core::InternalThreadState *Thread, uint64_t Address, uint64_t Size, bool Free) {
  const auto AlignedBase = Address & FHU::FEX_PAGE_MASK;
  const auto AlignedSize = (Address - AlignedBase + Size + FHU::FEX_PAGE_SIZE - 1) & FHU::FEX_PAGE_MASK;
  Thread->CTX->InvalidateGuestCodeRange(Thread, AlignedBase, AlignedSize);

  if (Free) {
    std::scoped_lock Lock(RWXIntervalsLock);
    RWXIntervals.Remove({AlignedBase, AlignedBase + AlignedSize});
  }
}

void InvalidationTracker::ReprotectRWXIntervals(uint64_t Address, uint64_t Size) {
  const auto End = Address + Size;
  std::scoped_lock Lock(RWXIntervalsLock);

  do {
    const auto Query = RWXIntervals.Query(Address);
    if (Query.Enclosed) {
      void *TmpAddress = reinterpret_cast<void *>(Address);
      SIZE_T TmpSize = static_cast<SIZE_T>(std::min(End, Address + Query.Size) - Address);
      ULONG TmpProt;
      NtProtectVirtualMemory(NtCurrentProcess(), &TmpAddress, &TmpSize, PAGE_EXECUTE_READ, &TmpProt);
    } else if (!Query.Size) {
      // No more regions past `Address` in the interval list
      break;
    }

    Address += Query.Size;
  } while (Address < End);
}

bool InvalidationTracker::HandleRWXAccessViolation(FEXCore::Core::InternalThreadState *Thread, uint64_t FaultAddress) {
  const bool NeedsInvalidate = [&](uint64_t Address) {
    std::unique_lock Lock(RWXIntervalsLock);
    const bool Enclosed = RWXIntervals.Query(Address).Enclosed;
    // Invalidate just the single faulting page
    if (!Enclosed)
      return false;

    ULONG TmpProt;
    void *TmpAddress = reinterpret_cast<void *>(Address);
    SIZE_T TmpSize = 1;
    NtProtectVirtualMemory(NtCurrentProcess(), &TmpAddress, &TmpSize, PAGE_EXECUTE_READWRITE, &TmpProt);
    return true;
  }(FaultAddress);

  if (NeedsInvalidate) {
    // RWXIntervalsLock cannot be held during invalidation
    Thread->CTX->InvalidateGuestCodeRange(Thread, FaultAddress & FHU::FEX_PAGE_MASK, FHU::FEX_PAGE_SIZE);
    return true;
  }
  return false;
}
}
