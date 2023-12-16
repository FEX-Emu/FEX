// SPDX-License-Identifier: MIT
/*
$info$
category: LinuxSyscalls ~ Linux syscall emulation, marshaling and passthrough
tags: LinuxSyscalls|common
desc: VMA Tracking
$end_info$
*/

#include "LinuxSyscalls/Syscalls.h"

namespace FEX::HLE {
/// List Operations ///

inline void SyscallHandler::VMATracking::ListCheckVMALinks(VMAEntry *VMA) {
  if (VMA) {
    LOGMAN_THROW_A_FMT(VMA->ResourceNextVMA != VMA, "VMA tracking error");
    LOGMAN_THROW_A_FMT(VMA->ResourcePrevVMA != VMA, "VMA tracking error");
  }
}

// Removes a VMA from corresponding MappedResource list
// Returns true if list is empty
bool SyscallHandler::VMATracking::ListRemove(VMAEntry *VMA) {
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
void SyscallHandler::VMATracking::ListReplace(VMAEntry *VMA, VMAEntry *NewVMA) {
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
void SyscallHandler::VMATracking::ListInsertAfter(VMAEntry *AfterVMA, VMAEntry *NewVMA) {
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
void SyscallHandler::VMATracking::ListPrepend(MappedResource *Resource, VMAEntry *NewVMA) {
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
SyscallHandler::VMATracking::VMACIterator SyscallHandler::VMATracking::LookupVMAUnsafe(uint64_t GuestAddr) const {
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
void SyscallHandler::VMATracking::SetUnsafe(FEXCore::Context::Context *CTX, MappedResource *MappedResource, uintptr_t Base,
                                            uintptr_t Offset, uintptr_t Length, VMAFlags Flags, VMAProt Prot) {
  ClearUnsafe(CTX, Base, Length, MappedResource);

  auto [Iter, Inserted] = VMAs.emplace(
      Base, VMAEntry{MappedResource, nullptr, MappedResource ? MappedResource->FirstVMA : nullptr, Base, Offset, Length, Flags, Prot});
      
  LOGMAN_THROW_A_FMT(Inserted == true, "VMA Tracking corruption");

  if (MappedResource) {
    // Insert to the front of the linked list
    ListPrepend(MappedResource, &Iter->second);
  }
}

// Remove mappings in a range, possibly splitting them if needed and 
// freeing their associated MappedResource unless it is equal to PreservedMappedResource
void SyscallHandler::VMATracking::ClearUnsafe(FEXCore::Context::Context *CTX, uintptr_t Base, uintptr_t Length,
                                              MappedResource *PreservedMappedResource) {
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

        auto [Iter, Inserted] = VMAs.emplace(
            Top, VMAEntry{Current->Resource, ReplaceAndErase ?  Current->ResourcePrevVMA : Current, Current->ResourceNextVMA, Top, NewOffset, NewLength, Current->Flags, Current->Prot});
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
void SyscallHandler::VMATracking::ChangeUnsafe(uintptr_t Base, uintptr_t Length, VMAProt NewProt) {
  const auto Top = Base + Length;

  // find the first Mapping at or after the Range ends, or ::end()
  // Top is the address after the end
  auto MappingIter = VMAs.lower_bound(Top);

  // Iterate backwards all mappings
  while (MappingIter != VMAs.begin()) {
    MappingIter--;

    auto Current = &MappingIter->second;
    const auto MapBase = Current->Base;
    const auto MapTop = MapBase + Current->Length;
    const auto MapFlags = Current->Flags;
    const auto MapProt = Current->Prot;

    const auto OffsetDiff = Current->Offset - MapBase;

    if (MapTop <= Base) {
      // Mapping ends before the Range start, exit
      break;
    } else if (MapProt.All == NewProt.All) {
      // Mapping already has the needed prots
      continue;
    } else {
      const bool HasFirstPart = MapBase < Base;
      const bool HasTrailingPart = MapTop > Top;

      if (HasFirstPart) {
        // Mapping starts before range, split first part

        // Trim end of original mapping
        Current->Length = Base - MapBase;

        // Make new VMA with new flags, insert for length of range
        auto NewOffset = OffsetDiff + Base;
        auto NewLength = Top - Base;

        auto [Iter, Inserted] =
            VMAs.emplace(Base, VMAEntry{Current->Resource, Current, Current->ResourceNextVMA, Base, NewOffset, NewLength, MapFlags, NewProt});
        LOGMAN_THROW_A_FMT(Inserted == true, "VMA tracking error");
        auto RestOfMapping = &Iter->second;

        if (Current->Resource) {
          ListInsertAfter(Current, RestOfMapping);
        }

        Current = RestOfMapping;
      } else {
        // Mapping starts in range, just change Prot
        Current->Prot = NewProt;
      }

      if (HasTrailingPart) {
        // ends after Range, split last part and insert with original flags

        // Trim the mapping (possibly already trimmed)
        Current->Length = Top - Current->Base;

        // prot has already been changed

        // Make new VMA with original flags, insert for remaining length
        auto NewOffset = OffsetDiff + Top;
        auto NewLength = MapTop - Top;

        auto [Iter, Inserted] =
            VMAs.emplace(Top, VMAEntry{Current->Resource, Current, Current->ResourceNextVMA, Top, NewOffset, NewLength, MapFlags, MapProt});
        LOGMAN_THROW_A_FMT(Inserted == true, "VMA tracking error");
        auto TrailingMapping = &Iter->second;

        if (Current->Resource) {
          ListInsertAfter(Current, TrailingMapping);
        }
      }
    }
  }
}

// This matches the peculiarities algorithm used in linux ksys_shmdt (linux kernel 5.16, ipc/shm.c)
uintptr_t SyscallHandler::VMATracking::ClearShmUnsafe(FEXCore::Context::Context *CTX, uintptr_t Base) {

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
}
