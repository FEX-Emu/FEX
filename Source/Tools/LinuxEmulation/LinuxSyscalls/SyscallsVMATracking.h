// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <tuple>

#include <FEXCore/fextl/map.h>
#include <FEXCore/fextl/memory.h>
#include <FEXCore/Utils/SignalScopeGuards.h>

namespace FEX::HLE::VMATracking {
///// VMA (Virtual Memory Area) tracking /////

namespace SpecialDev {
  static constexpr uint64_t Anon = 0x1'0000'0000; // Anonymous shared mapping, id is incrementing allocation number
  static constexpr uint64_t SHM = 0x2'0000'0000;  // sys-v shm, id is shmid
}; // namespace SpecialDev

// Memory Resource ID
// An id that can be used to identify when shared mappings actually have the same backing storage
// when dev != SpecialDev::Anon, this is unique system wide
struct MRID {
  uint64_t dev; // kernel dev_t is actually 32-bits, we use the extra bits to track SpecialDevs
  uint64_t id;

  bool operator<(const MRID& other) const {
    return std::tie(dev, id) < std::tie(other.dev, other.id);
  }
};

struct VMAEntry;

/**
 * Meta data associated to one system resource.
 *
 * Typically there is one instance of this type per ELF/PE file or special device.
 * However if an ELF/PE file is mapped multiple times at different base addresses,
 * there will be one separate MappedResource for each base address. The MRID
 * is the same in this case.
 */
struct MappedResource {
  using ContainerType = fextl::multimap<MRID, MappedResource>;

  fextl::unique_ptr<FEXCore::ExecutableFileInfo> MappedFile;
  // Pointer to lowest memory range this file is mapped to
  VMAEntry* FirstVMA;
  uint64_t Length; // 0 if not fixed size
  ContainerType::iterator Iterator;
};

union VMAProt {
  struct {
    bool Readable   : 1;
    bool Writable   : 1;
    bool Executable : 1;
  };
  uint8_t All : 3;

  static VMAProt fromProt(int Prot);
  static VMAProt fromSHM(int SHMFlg);
};

struct VMAFlags {
  bool Shared : 1;

  static VMAFlags fromFlags(int Flags);
};

struct VMAEntry {
  MappedResource* Resource;

  // these are for intrusive linked list tracking, starting from Resource->FirstVMA and ordered by address
  VMAEntry* ResourcePrevVMA;
  VMAEntry* ResourceNextVMA;

  uint64_t Base;
  uint64_t Offset;
  uint64_t Length;

  VMAFlags Flags;
  VMAProt Prot;
};

struct VMATracking {
  // Held while reading/writing this struct
  FEXCore::ForkableSharedMutex Mutex;

  // Memory ranges indexed by page aligned starting address
  fextl::map<uint64_t, VMAEntry> VMAs;

  using VMACIterator = decltype(VMAs)::const_iterator;

  // Find a VMA entry associated with the memory address.
  // Used by `mremap` and SIGSEGV handler to find previously mapped ranges, and CodeCache to find cache entries.
  // - Mutex must be at least shared_locked before calling
  VMACIterator FindVMAEntry(uint64_t GuestAddr) const;

  // Adds a new VMA Range to be tracked, along with a `MappedResource` associated with that VMA range.
  // Primarily matches `mmap` semantics, but also used by `mremap`, and `shmat`, as they all can add new VMA ranges to be tracked.
  // - Mutex must be unique_locked before calling
  void TrackVMARange(FEXCore::Context::Context* Ctx, MappedResource* MappedResource, uintptr_t Base, uintptr_t Offset, uintptr_t Length,
                     VMAFlags Flags, VMAProt Prot);

  // Deletes a VMA range provided from tracking.
  // Matches `munmap` semantics, and `mremap` with `MREMAP_DONTUNMAP` flag set.
  // Deletes internal `MappedResource` that correlates with the range **unless** it matches `PreservedMappedResource`
  // - Mutex must be unique_locked before calling
  void DeleteVMARange(FEXCore::Context::Context* Ctx, uintptr_t Base, uintptr_t Length, MappedResource* PreservedMappedResource = nullptr);

  // Changes the protections tracking for the VMA range provided.
  // Matches `mprotect` semantics.
  // - Mutex must be unique_locked before calling
  void ChangeProtectionFlags(uintptr_t Base, uintptr_t Length, VMAProt Prot);

  // Deletes the SHM region mapped at Base from tracking.
  // Matches `shmdt` semantics.
  // - Mutex must be unique_locked before calling
  // Returns the Size of the Shm or 0 if not found
  uintptr_t DeleteSHMRegion(FEXCore::Context::Context* Ctx, uintptr_t Base);

  // Adds a new `MappedResource` to track.
  inline auto InsertMappedResource(const MRID& mrid, MappedResource Resource) {
    return MappedResources.emplace(mrid, std::move(Resource));
  }

  // Returns an iterator pair spanning the range of all MappedResources matching the given MRID.
  // Typically there is only one associated resource, however sometimes the same file gets mapped
  // multiple times at different base addresses. In that case, each MappedResource will cover an
  // exclusive set of VMAEntries that refer to a consistent base mapping address.
  inline auto FindResources(const MRID& mrid) {
    return MappedResources.equal_range(mrid);
  }

private:
  MappedResource::ContainerType MappedResources;
};


} // namespace FEX::HLE::VMATracking
