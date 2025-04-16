// SPDX-License-Identifier: MIT

#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/TypeDefines.h>
#include <FEXCore/Utils/SignalScopeGuards.h>
#include <FEXCore/Core/Context.h>
#include <FEXCore/Debug/InternalThreadState.h>
#include "InvalidationTracker.h"
#include <windef.h>
#include <winternl.h>

namespace FEX::Windows {
InvalidationTracker::InvalidationTracker(FEXCore::Context::Context& CTX, const std::unordered_map<DWORD, FEXCore::Core::InternalThreadState*>& Threads)
  : CTX {CTX}
  , Threads {Threads} {
  MEMORY_BASIC_INFORMATION Info;
  uint64_t Address = 0;

  while (VirtualQuery(reinterpret_cast<LPCVOID>(Address), &Info, sizeof(Info))) {
    uint64_t BaseAddress = reinterpret_cast<uint64_t>(Info.BaseAddress);
    if (Info.State == MEM_COMMIT) {
      HandleMemoryProtectionNotification(BaseAddress, Info.RegionSize, Info.Protect);
    }

    Address = BaseAddress + Info.RegionSize;
  }
}

void InvalidationTracker::HandleMemoryProtectionNotification(uint64_t Address, uint64_t Size, ULONG Prot) {
  const auto AlignedBase = Address & FEXCore::Utils::FEX_PAGE_MASK;
  const auto AlignedSize = (Address - AlignedBase + Size + FEXCore::Utils::FEX_PAGE_SIZE - 1) & FEXCore::Utils::FEX_PAGE_MASK;

  if (Prot & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)) {
    std::scoped_lock Lock(CTX.GetCodeInvalidationMutex());
    for (auto Thread : Threads) {
      CTX.InvalidateGuestCodeRange(Thread.second, AlignedBase, AlignedSize);
    }
  }

  if (Prot & (PAGE_EXECUTE_WRITECOPY | PAGE_EXECUTE_READWRITE)) {
    LogMan::Msg::DFmt("Add SMC interval: {:X} - {:X}", AlignedBase, AlignedBase + AlignedSize);
    std::scoped_lock Lock(RWXIntervalsLock);
    RWXIntervals.Insert({AlignedBase, AlignedBase + AlignedSize});
  } else {
    std::scoped_lock Lock(RWXIntervalsLock);
    RWXIntervals.Remove({AlignedBase, AlignedBase + AlignedSize});
  }
}

void InvalidationTracker::HandleImageMap(uint64_t Address) {
  auto* Nt = RtlImageNtHeader(reinterpret_cast<HMODULE>(Address));
  auto* SectionsBegin = IMAGE_FIRST_SECTION(Nt);
  auto* SectionsEnd = SectionsBegin + Nt->FileHeader.NumberOfSections;

  for (auto* Section = SectionsBegin; Section != SectionsEnd; Section++) {
    if ((Section->Characteristics & IMAGE_SCN_MEM_EXECUTE) && (Section->Characteristics & IMAGE_SCN_MEM_WRITE)) {
      uint64_t SectionBase = Address + Section->VirtualAddress;
      LogMan::Msg::DFmt("Add image SMC interval: {:X} - {:X}", SectionBase, SectionBase + Section->Misc.VirtualSize);
      std::scoped_lock Lock(RWXIntervalsLock);
      RWXIntervals.Insert({SectionBase, SectionBase + Section->Misc.VirtualSize});
    }
  }
}

InvalidationTracker::InvalidateContainingSectionResult InvalidationTracker::InvalidateContainingSection(uint64_t Address, bool Free) {
  MEMORY_BASIC_INFORMATION Info;
  if (NtQueryVirtualMemory(NtCurrentProcess(), reinterpret_cast<void*>(Address), MemoryBasicInformation, &Info, sizeof(Info), nullptr)) {
    return {Address, 0};
  }

  const auto SectionBase = reinterpret_cast<uint64_t>(Info.AllocationBase);
  auto SectionSize = reinterpret_cast<uint64_t>(Info.BaseAddress) + Info.RegionSize - SectionBase;

  while (!NtQueryVirtualMemory(NtCurrentProcess(), reinterpret_cast<void*>(SectionBase + SectionSize), MemoryBasicInformation, &Info,
                               sizeof(Info), nullptr) &&
         reinterpret_cast<uint64_t>(Info.AllocationBase) == SectionBase) {
    SectionSize += Info.RegionSize;
  }
  {
    std::scoped_lock Lock(CTX.GetCodeInvalidationMutex());
    for (auto Thread : Threads) {
      CTX.InvalidateGuestCodeRange(Thread.second, SectionBase, SectionSize);
    }
  }

  if (Free) {
    std::scoped_lock Lock(RWXIntervalsLock);
    RWXIntervals.Remove({SectionBase, SectionBase + SectionSize});
  }

  return {SectionBase, SectionSize};
}

void InvalidationTracker::InvalidateAlignedInterval(uint64_t Address, uint64_t Size, bool Free) {
  if (!Address) {
    // Match the Windows behaviour when passed a NULL base address.
    Size = std::numeric_limits<uint64_t>::max();
  }

  const auto AlignedBase = Address & FEXCore::Utils::FEX_PAGE_MASK;
  const auto AlignedSize = std::max(Size, (Address - AlignedBase + Size + FEXCore::Utils::FEX_PAGE_SIZE - 1) & FEXCore::Utils::FEX_PAGE_MASK);

  {
    std::scoped_lock Lock(CTX.GetCodeInvalidationMutex());
    for (auto Thread : Threads) {
      CTX.InvalidateGuestCodeRange(Thread.second, AlignedBase, AlignedSize);
    }
  }

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
      void* TmpAddress = reinterpret_cast<void*>(Address);
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

bool InvalidationTracker::HandleRWXAccessViolation(uint64_t FaultAddress) {
  const bool NeedsInvalidate = [&](uint64_t Address) {
    std::unique_lock Lock(RWXIntervalsLock);
    const bool Enclosed = RWXIntervals.Query(Address).Enclosed;
    // Invalidate just the single faulting page
    if (!Enclosed) {
      return false;
    }

    ULONG TmpProt;
    void* TmpAddress = reinterpret_cast<void*>(Address);
    SIZE_T TmpSize = 1;
    NtProtectVirtualMemory(NtCurrentProcess(), &TmpAddress, &TmpSize, PAGE_EXECUTE_READWRITE, &TmpProt);
    return true;
  }(FaultAddress);

  if (NeedsInvalidate) {
    // RWXIntervalsLock cannot be held during invalidation
    std::scoped_lock Lock(CTX.GetCodeInvalidationMutex());
    for (auto Thread : Threads) {
      CTX.InvalidateGuestCodeRange(Thread.second, FaultAddress & FEXCore::Utils::FEX_PAGE_MASK, FEXCore::Utils::FEX_PAGE_SIZE);
    }
    return true;
  }
  return false;
}
} // namespace FEX::Windows
