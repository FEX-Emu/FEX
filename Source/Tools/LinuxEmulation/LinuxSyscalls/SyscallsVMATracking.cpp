// SPDX-License-Identifier: MIT
/*
$info$
category: LinuxSyscalls ~ Linux syscall emulation, marshaling and passthrough
tags: LinuxSyscalls|common
desc: VMA Tracking
$end_info$
*/

#include "LinuxSyscalls/Syscalls.h"
#include <sys/shm.h>

namespace FEX::HLE::VMATracking {
/// Helpers ///
auto VMAProt::fromProt(int Prot) -> VMAProt {
  return VMAProt {
    .Readable = (Prot & PROT_READ) != 0,
    .Writable = (Prot & PROT_WRITE) != 0,
    .Executable = (Prot & PROT_EXEC) != 0,
  };
}

auto VMAProt::fromSHM(int SHMFlg) -> VMAProt {
  return VMAProt {
    .Readable = true,
    .Writable = SHMFlg & SHM_RDONLY ? false : true,
    .Executable = SHMFlg & SHM_EXEC ? true : false,
  };
}

auto VMAFlags::fromFlags(int Flags) -> VMAFlags {
  return VMAFlags {
    .Shared = (Flags & MAP_SHARED) != 0, // also includes MAP_SHARED_VALIDATE
  };
}

/// List Operations ///
inline void VMATracking::ListCheckVMALinks(VMAEntry* VMA) {
  if (VMA) {
    LOGMAN_THROW_A_FMT(VMA->ResourceNextVMA != VMA, "VMA tracking error");
    LOGMAN_THROW_A_FMT(VMA->ResourcePrevVMA != VMA, "VMA tracking error");
  }
}

// Removes a VMA from corresponding MappedResource list
// Returns true if list is empty
bool VMATracking::ListRemove(VMAEntry* VMA) {
  LOGMAN_THROW_A_FMT(VMA->Resource != nullptr, "VMA tracking error");

  // if it has prev, make prev to next
  if (VMA->ResourcePrevVMA) {
    LOGMAN_THROW_A_FMT(VMA->ResourcePrevVMA->ResourceNextVMA == VMA, "VMA tracking error");
    VMA->ResourcePrevVMA->ResourceNextVMA = VMA->ResourceNextVMA;
  } else {
    LOGMAN_THROW_A_FMT(VMA->Resource->FirstVMA == VMA, "VMA tracking error");
  }

  // if it has next, make next to prev
  if (VMA->ResourceNextVMA) {
    LOGMAN_THROW_A_FMT(VMA->ResourceNextVMA->ResourcePrevVMA == VMA, "VMA tracking error");
    VMA->ResourceNextVMA->ResourcePrevVMA = VMA->ResourcePrevVMA;
  }

  // If it is the first in the list, make Next the first in the list
  if (VMA->Resource && VMA->Resource->FirstVMA == VMA) {
    LOGMAN_THROW_A_FMT(!VMA->ResourceNextVMA || VMA->ResourceNextVMA->ResourcePrevVMA == nullptr, "VMA tracking error");

    VMA->Resource->FirstVMA = VMA->ResourceNextVMA;
  }

  ListCheckVMALinks(VMA);
  ListCheckVMALinks(VMA->ResourceNextVMA);
  ListCheckVMALinks(VMA->ResourcePrevVMA);

  // Return true if list is empty
  return VMA->Resource->FirstVMA == nullptr;
}

// Replaces a VMA in corresponding MappedResource list
// Requires NewVMA->Resource, NewVMA->ResourcePrevVMA and NewVMA->ResourceNextVMA to be already setup
void VMATracking::ListReplace(VMAEntry* VMA, VMAEntry* NewVMA) {
  LOGMAN_THROW_A_FMT(VMA->Resource != nullptr, "VMA tracking error");

  LOGMAN_THROW_A_FMT(VMA->Resource == NewVMA->Resource, "VMA tracking error");
  LOGMAN_THROW_A_FMT(NewVMA->ResourcePrevVMA == VMA->ResourcePrevVMA, "VMA tracking error");
  LOGMAN_THROW_A_FMT(NewVMA->ResourceNextVMA == VMA->ResourceNextVMA, "VMA tracking error");

  if (VMA->ResourcePrevVMA) {
    LOGMAN_THROW_A_FMT(VMA->Resource->FirstVMA != VMA, "VMA tracking error");
    LOGMAN_THROW_A_FMT(VMA->ResourcePrevVMA->ResourceNextVMA == VMA, "VMA tracking error");
    VMA->ResourcePrevVMA->ResourceNextVMA = NewVMA;
  } else {
    LOGMAN_THROW_A_FMT(VMA->Resource->FirstVMA == VMA, "VMA tracking error");
    VMA->Resource->FirstVMA = NewVMA;
  }

  if (VMA->ResourceNextVMA) {
    LOGMAN_THROW_A_FMT(VMA->ResourceNextVMA->ResourcePrevVMA == VMA, "VMA tracking error");
    VMA->ResourceNextVMA->ResourcePrevVMA = NewVMA;
  }

  ListCheckVMALinks(VMA);
  ListCheckVMALinks(NewVMA);
  ListCheckVMALinks(VMA->ResourceNextVMA);
  ListCheckVMALinks(VMA->ResourcePrevVMA);
}

// Inserts a VMA in corresponding MappedResource list
// Requires NewVMA->Resource, NewVMA->ResourcePrevVMA and NewVMA->ResourceNextVMA to be already setup
void VMATracking::ListInsertAfter(VMAEntry* AfterVMA, VMAEntry* NewVMA) {
  LOGMAN_THROW_A_FMT(NewVMA->Resource != nullptr, "VMA tracking error");

  LOGMAN_THROW_A_FMT(AfterVMA->Resource == NewVMA->Resource, "VMA tracking error");
  LOGMAN_THROW_A_FMT(NewVMA->ResourcePrevVMA == AfterVMA, "VMA tracking error");
  LOGMAN_THROW_A_FMT(NewVMA->ResourceNextVMA == AfterVMA->ResourceNextVMA, "VMA tracking error");

  if (AfterVMA->ResourceNextVMA) {
    LOGMAN_THROW_A_FMT(AfterVMA->ResourceNextVMA->ResourcePrevVMA == AfterVMA, "VMA tracking error");
    AfterVMA->ResourceNextVMA->ResourcePrevVMA = NewVMA;
  }
  AfterVMA->ResourceNextVMA = NewVMA;

  ListCheckVMALinks(AfterVMA);
  ListCheckVMALinks(NewVMA);
  ListCheckVMALinks(AfterVMA->ResourceNextVMA);
  ListCheckVMALinks(AfterVMA->ResourcePrevVMA);
}

// Prepends a VMA
// Requires NewVMA->Resource, NewVMA->ResourcePrevVMA and NewVMA->ResourceNextVMA to be already setup
void VMATracking::ListPrepend(MappedResource* Resource, VMAEntry* NewVMA) {
  LOGMAN_THROW_A_FMT(Resource != nullptr, "VMA tracking error");

  LOGMAN_THROW_A_FMT(NewVMA->Resource == Resource, "VMA tracking error");
  LOGMAN_THROW_A_FMT(NewVMA->ResourcePrevVMA == nullptr, "VMA tracking error");
  LOGMAN_THROW_A_FMT(NewVMA->ResourceNextVMA == Resource->FirstVMA, "VMA tracking error");

  if (Resource->FirstVMA) {
    LOGMAN_THROW_A_FMT(Resource->FirstVMA->ResourcePrevVMA == nullptr, "VMA tracking error");
    Resource->FirstVMA->ResourcePrevVMA = NewVMA;
  }

  Resource->FirstVMA = NewVMA;

  ListCheckVMALinks(NewVMA);
  ListCheckVMALinks(NewVMA->ResourceNextVMA);
  ListCheckVMALinks(NewVMA->ResourcePrevVMA);
}

/// VMA tracking ///

// Lookup a VMA by address
VMATracking::VMACIterator VMATracking::FindVMAEntry(uint64_t GuestAddr) const {
  auto Entry = VMAs.upper_bound(GuestAddr);

  if (Entry != VMAs.begin()) {
    --Entry;

    if (Entry->first <= GuestAddr && (Entry->first + Entry->second.Length) > GuestAddr) {
      return Entry;
    }
  }

  return VMAs.end();
}

// Set or Replace mappings in a range with a new mapping
void VMATracking::TrackVMARange(FEXCore::Context::Context* CTX, MappedResource* MappedResource, uintptr_t Base, uintptr_t Offset,
                                uintptr_t Length, VMAFlags Flags, VMAProt Prot) {
  Mutex.check_lock_owned_by_self_as_write();

  DeleteVMARange(CTX, Base, Length, MappedResource);

  auto PrevResVMA = MappedResource ? MappedResource->FirstVMA : nullptr;
  auto NextResVMA = PrevResVMA ? PrevResVMA->ResourceNextVMA : nullptr;
  if (PrevResVMA && PrevResVMA->Base > Base) {
    NextResVMA = std::exchange(PrevResVMA, nullptr);
  }
  while (NextResVMA && NextResVMA->Base < Base) {
    PrevResVMA = NextResVMA;
    NextResVMA = PrevResVMA->ResourceNextVMA;
  }

  auto [Iter, Inserted] = VMAs.emplace(Base, VMAEntry {MappedResource, PrevResVMA, NextResVMA, Base, Offset, Length, Flags, Prot});

  LOGMAN_THROW_A_FMT(Inserted == true, "VMA Tracking corruption");

  if (MappedResource && !PrevResVMA) {
    // Insert to the front of the linked list
    ListPrepend(MappedResource, &Iter->second);
  } else if (MappedResource) {
    ListInsertAfter(PrevResVMA, &Iter->second);
  }
}

// Remove mappings in a range, possibly splitting them if needed and
// freeing their associated MappedResource unless it is equal to PreservedMappedResource
void VMATracking::DeleteVMARange(FEXCore::Context::Context* CTX, uintptr_t Base, uintptr_t Length, MappedResource* PreservedMappedResource) {
  Mutex.check_lock_owned_by_self_as_write();

  const auto Top = Base + Length;

  // find the first Mapping at or after the Range ends, or ::end()
  // Top is the address after the end
  auto CurrentIter = VMAs.lower_bound(Top);

  // Iterate backwards all mappings
  while (CurrentIter != VMAs.begin()) {
    CurrentIter--;

    const auto Current = &CurrentIter->second;
    const auto MapBase = Current->Base;
    const auto MapTop = MapBase + Current->Length;
    const auto OffsetDiff = Current->Offset - MapBase;

    if (MapTop <= Base) {
      // Mapping ends before the Range start, exit
      break;
    } else {
      const bool HasFirstPart = MapBase < Base;
      const bool HasTrailingPart = MapTop > Top;

      // (1) HasFirstPart, !HasTrailingPart -> trim
      // (2) HasFirstPart, HasTrailingPart -> trim, insert trailing, list add after first part
      // (3) !HasFirstPart, !HasTrailing part -> list remove, erase
      // (4) !HasFirstPart, HasTrailing part -> insert trailing, list replace first part, erase

      if (HasFirstPart) {
        // Handle trim for (1) & (2)
        Current->Length = Base - MapBase;
      } else if (!HasTrailingPart) {
        // Handle all of (3)
        // Mapping is included or equal to Range, delete

        // If linked to a Mapped Resource, remove from linked list and possibly delete the Mapped Resource
        if (Current->Resource) {
          if (ListRemove(Current) && Current->Resource != PreservedMappedResource) {
            if (Current->Resource->AOTIRCacheEntry) {
              CTX->UnloadAOTIRCacheEntry(Current->Resource->AOTIRCacheEntry);
            }
            MappedResources.erase(Current->Resource->Iterator);
          }
        }

        // returns next element, so -- is safe at loop
        CurrentIter = VMAs.erase(CurrentIter);
        continue; // we're done
      }

      const bool ReplaceAndErase = !HasFirstPart;

      if (HasTrailingPart) {
        // Handle insert of (2), (4)

        // insert trailing part, link it after Mapping
        auto NewOffset = OffsetDiff + Top;
        auto NewLength = MapTop - Top;

        auto [Iter, Inserted] = VMAs.emplace(Top, VMAEntry {Current->Resource, ReplaceAndErase ? Current->ResourcePrevVMA : Current,
                                                            Current->ResourceNextVMA, Top, NewOffset, NewLength, Current->Flags, Current->Prot});
        LOGMAN_THROW_A_FMT(Inserted == true, "VMA tracking error");
        auto TrailingPart = &Iter->second;
        if (Current->Resource) {
          if (ReplaceAndErase) {
            // Handle list replace of (4)
            ListReplace(Current, TrailingPart);
          } else {
            // Handle list insert (2)
            ListInsertAfter(Current, TrailingPart);
          }
        }
      }

      if (ReplaceAndErase) {
        // Handle erase of (4)
        // returns next element, so -- is safe at loop
        CurrentIter = VMAs.erase(CurrentIter);
      }
    }
  }
}

// Change flags of mappings in a range and split the mappings if needed
void VMATracking::ChangeProtectionFlags(uintptr_t Base, uintptr_t Length, VMAProt NewProt) {
  Mutex.check_lock_owned_by_self_as_write();

  // This needs to handle multiple split-merge strategies:
  // 1) Exact overlap - No Split, no Merge. Only protection tracking changes.
  // 2) Exact base overlap - Single insert, can never fail.
  // 3) Insert in middle of VMA range. 1 or 2 inserts, can never fail.
  // 4) Partial overlapping merge. The most interesting strategy.
  //    - More information below about this one.

  auto Top = Base + Length;

  // find the first Mapping at or after the Range ends, or ::end()
  // Top is the address after the end
  auto MappingIter = VMAs.lower_bound(Top);

  // Iterate backwards all mappings
  while (MappingIter != VMAs.begin()) {
    MappingIter--;

    auto Current = &MappingIter->second;

    if (Current->Base <= Base || Current->Base + Current->Length < Top) {
      break;
    }

    const auto CurrentBase = Current->Base;
    const auto CurrentTop = CurrentBase + Current->Length;
    const auto CurrentFlags = Current->Flags;
    const auto CurrentProt = Current->Prot;

    ///< Resource mapping base.
    const auto OffsetDiff = Current->Offset - CurrentBase;

    // Merge strategy 4)
    // CurrentBase range doesn't fully overlap the starting range but does overlap the tail.
    // This is the most confusing strategy as it requires splitting the protect range itself.
    //
    // if the VMA has tail data after the protection range we must first deal with that:
    // 1) Split the tail data in to new VMA range with original protections. Must not fail.
    // 2) Adjust the overlapping VMA protections to the new protections and the truncated length
    // 3) Truncate the mprotecting length and top to be that untouched range. Next loop will continue inserting.
    // [ Incoming Ranges ]
    // CurrentVMA:                            [CurrentBase ====== CurrentTop)
    // CurrentMProtectRange: [Base =============== Top)**********************
    // [ Modified Ranges ]
    // New Tail Range:                                [TailBase === Tail Top)
    // CurrentVMA Modified Range:             [=======)
    // Remaining Tracking:   [Base ==== NewTop)
    //
    // Next loop iterations will decompose the remaining mprotects in to more merge strategies.

    // Steps:
    // 1) Split VMA if Top != CurrentTop
    // 2) Change [CurrentBase, Top) protections
    // 3) Change CurrentVMA length
    // 4) Adjust searching length for [Base, CurrentBase)
    const bool HasTailData = CurrentTop > Top;

    if (HasTailData) {
      // We now need to insert another VMA entry afterwards to ensure consistency.
      // This will have the original VMA's protection flags.

      // Make new VMA with new flags, insert for length of range
      auto NewOffset = OffsetDiff + CurrentBase;
      auto NewLength = CurrentTop - Top;

      auto [Iter, Inserted] = VMAs.emplace(Top, VMAEntry {.Resource = Current->Resource,
                                                          .ResourcePrevVMA = Current,
                                                          .ResourceNextVMA = Current->ResourceNextVMA,
                                                          .Base = Top,
                                                          .Offset = NewOffset,
                                                          .Length = NewLength,
                                                          .Flags = CurrentFlags,
                                                          .Prot = CurrentProt});

      if (!Inserted) {
        // We can't recover from this.
        // Shouldn't ever happen.
        ERROR_AND_DIE_FMT("{}:{}: VMA tracking error", __func__, __LINE__);
      }

      if (Current->Resource) {
        ListInsertAfter(Current, &Iter->second);
      }
    }

    // Change CurrentVMA's protections
    Current->Prot = NewProt;

    // Change CurrentVMA's length
    Current->Length = Top - CurrentBase;

    // Adjust the protection length we're searching for.
    // Next loop will pick up the next check.
    Length = CurrentBase - Base;
    Top = Base + Length;
  }

  auto Current = &MappingIter->second;
  const auto CurrentBase = Current->Base;
  const auto CurrentTop = CurrentBase + Current->Length;
  const auto CurrentFlags = Current->Flags;
  const auto CurrentProt = Current->Prot;

  ///< Resource mapping base.
  const auto OffsetDiff = Current->Offset - CurrentBase;
  if (CurrentTop <= Base) {
    // Mapping is below what we care about
    // [CurrentBase === CurrentTop)
    //                            [Base === Top)
  } else if (CurrentBase == Base && CurrentTop == Top) {
    // Merge strategy 1)
    // Exact encompassing, quite common.
    // [CurrentBase ======================== CurrentTop)
    // [Base ====================================== Top)
    Current->Prot = NewProt;
  } else if (CurrentBase == Base && CurrentTop > Top) {
    // Merge strategy 2)
    // [CurrentBase ======================== CurrentTop)
    // [Base =============== Top)***********************
    // VMA fully encompasses with matching base.
    // VMA needs to split.

    // Steps:
    // 1) Set new permissions for this VMA
    // 2) Trim VMA->Length to match [CurrentBase, CurrentBase+Length)
    // 2) Insert new node at [CurrentBase+Length, CurrentTop)

    // 1) Set new permissions
    Current->Prot = NewProt;

    // Trim end of original mapping
    // New length for Current VMA is Top - CurrentBase
    Current->Length = Top - CurrentBase;

    // Make new VMA with original protections, insert for remaining length
    auto NewOffset = OffsetDiff + Top;
    auto NewLength = CurrentTop - Top;

    auto [Iter, Inserted] = VMAs.emplace(Top, VMAEntry {.Resource = Current->Resource,
                                                        .ResourcePrevVMA = Current,
                                                        .ResourceNextVMA = Current->ResourceNextVMA,
                                                        .Base = Top,
                                                        .Offset = NewOffset,
                                                        .Length = NewLength,
                                                        .Flags = CurrentFlags,
                                                        .Prot = CurrentProt});

    if (!Inserted) [[unlikely]] {
      // We can't recover from this.
      // Shouldn't ever happen.
      ERROR_AND_DIE_FMT("{}:{}: VMA tracking error", __func__, __LINE__);
    }

    if (Current->Resource) {
      ListInsertAfter(Current, &Iter->second);
    }
  } else if (CurrentBase < Base && CurrentTop >= Top) {
    // Merge strategy 3)
    // VMA fully encompasses, VMA needs to split.
    // Explicitly VMA base doesn't match current base.
    // [CurrentBase ======================== CurrentTop)
    // ***************[Base =============== Top)********

    // Steps:
    // 1) Split the CurrentVMA
    // 2) Set new length of CurrentVMA
    // 3) If there is tail length still, Insert another new VMA with CurrentVMA data.

    const bool HasTailData = CurrentTop > Top;

    // Trim end of original mapping
    Current->Length = Base - CurrentBase;
    {
      // Make new VMA with new flags, insert for length of range
      auto NewOffset = OffsetDiff + Base;
      auto NewLength = Top - Base;

      auto [Iter, Inserted] = VMAs.emplace(Base, VMAEntry {.Resource = Current->Resource,
                                                           .ResourcePrevVMA = Current,
                                                           .ResourceNextVMA = Current->ResourceNextVMA,
                                                           .Base = Base,
                                                           .Offset = NewOffset,
                                                           .Length = NewLength,
                                                           .Flags = CurrentFlags,
                                                           .Prot = NewProt});

      if (!Inserted) [[unlikely]] {
        // We can't recover from this.
        // Shouldn't ever happen.
        ERROR_AND_DIE_FMT("{}:{}: VMA tracking error", __func__, __LINE__);
      }

      if (Current->Resource) {
        ListInsertAfter(Current, &Iter->second);
      }
    }

    if (HasTailData) {
      // We now need to insert another VMA entry afterwards to ensure consistency.
      // This will have the original VMA's protection flags.

      // Make new VMA with new flags, insert for length of range
      auto NewOffset = OffsetDiff + Top;
      auto NewLength = CurrentTop - Top;

      auto [Iter, Inserted] = VMAs.emplace(Top, VMAEntry {.Resource = Current->Resource,
                                                          .ResourcePrevVMA = Current,
                                                          .ResourceNextVMA = Current->ResourceNextVMA,
                                                          .Base = Top,
                                                          .Offset = NewOffset,
                                                          .Length = NewLength,
                                                          .Flags = CurrentFlags,
                                                          .Prot = CurrentProt});

      if (!Inserted) {
        // We can't recover from this.
        // Shouldn't ever happen.
        ERROR_AND_DIE_FMT("{}:{}: VMA tracking error", __func__, __LINE__);
      }

      if (Current->Resource) {
        ListInsertAfter(Current, &Iter->second);
      }
    }
  } else {
    ERROR_AND_DIE_FMT("Unexpected {} Merge strategy! [0x{:x}, 0x{:x}) Versus [0x{:x}, 0x{:x})\n", __func__, CurrentBase, CurrentTop, Base, Top);
  }
}

// This matches the peculiarities algorithm used in linux ksys_shmdt (linux kernel 5.16, ipc/shm.c)
uintptr_t VMATracking::DeleteSHMRegion(FEXCore::Context::Context* CTX, uintptr_t Base) {

  // Find first VMA at or after Base
  // Iterate until first SHM VMA, with matching offset, get length
  // Then, erase any later occurrences of this SHM

  // returns first element that is greater or equal or ::end
  auto Entry = VMAs.lower_bound(Base);

  for (; Entry != VMAs.end(); ++Entry) {
    LOGMAN_THROW_A_FMT(Entry->second.Base >= Base, "VMA tracking corruption");
    if (Entry->second.Base - Base == Entry->second.Offset && Entry->second.Resource &&
        Entry->second.Resource->Iterator->first.dev == SpecialDev::SHM) {
      break;
    }
  }

  if (Entry == VMAs.end()) {
    return 0;
  }

  const auto ShmLength = Entry->second.Resource->Iterator->second.Length;
  const auto Resource = Entry->second.Resource;

  do {
    if (Entry->second.Resource == Resource) {
      if (ListRemove(&Entry->second)) {
        if (Entry->second.Resource->AOTIRCacheEntry) {
          CTX->UnloadAOTIRCacheEntry(Entry->second.Resource->AOTIRCacheEntry);
        }
        MappedResources.erase(Entry->second.Resource->Iterator);
      }
      Entry = VMAs.erase(Entry);
    } else {
      Entry++;
    }
  } while (Entry != VMAs.end() && (Entry->second.Base + Entry->second.Length - Base) <= ShmLength);

  return ShmLength;
}
} // namespace FEX::HLE::VMATracking
