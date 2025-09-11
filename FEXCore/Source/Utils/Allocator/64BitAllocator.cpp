// SPDX-License-Identifier: MIT
#include "Utils/Allocator/FlexBitSet.h"
#include "Utils/Allocator/HostAllocator.h"
#include "Utils/Allocator/IntrusiveArenaAllocator.h"
#include <FEXCore/Utils/Allocator.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/MathUtils.h>
#include <FEXCore/Utils/SignalScopeGuards.h>
#include <FEXCore/Utils/TypeDefines.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/MathUtils.h>
#include <FEXCore/fextl/sstream.h>
#include <FEXHeaderUtils/Syscalls.h>
#include <FEXCore/fextl/memory.h>
#include <FEXCore/fextl/vector.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <list>
#include <memory>
#include <mutex>
#include <new>
#include <sys/mman.h>
#include <sys/utsname.h>
#include <sys/user.h>
#include <type_traits>
#include <utility>

namespace Alloc::OSAllocator {

thread_local FEXCore::Core::InternalThreadState* TLSThread {};

class OSAllocator_64Bit final : public Alloc::HostAllocator {
public:
  OSAllocator_64Bit();
  OSAllocator_64Bit(fextl::vector<FEXCore::Allocator::MemoryRegion>& Regions);

  virtual ~OSAllocator_64Bit();
  void* AllocateSlab(size_t Size) override {
    return nullptr;
  }
  void DeallocateSlab(void* Ptr, size_t Size) override {}

  void* Mmap(void* addr, size_t length, int prot, int flags, int fd, off_t offset) override;
  int Munmap(void* addr, size_t length) override;

  void LockBeforeFork(FEXCore::Core::InternalThreadState* Thread) override {
    AllocationMutex.lock();
  }

  void UnlockAfterFork(FEXCore::Core::InternalThreadState* Thread, bool Child) override {
    if (Child) {
      AllocationMutex.StealAndDropActiveLocks();
    } else {
      AllocationMutex.unlock();
    }
  }

private:
  // Upper bound is the maximum virtual address space of the host processor
  uintptr_t UPPER_BOUND = (1ULL << 57);

  // Lower bound is the starting of the range just past the lower 32bits
  constexpr static uintptr_t LOWER_BOUND = 0x1'0000'0000ULL;

  uintptr_t UPPER_BOUND_PAGE = UPPER_BOUND / FEXCore::Utils::FEX_PAGE_SIZE;
  constexpr static uintptr_t LOWER_BOUND_PAGE = LOWER_BOUND / FEXCore::Utils::FEX_PAGE_SIZE;

  struct ReservedVMARegion {
    uintptr_t Base;
    // Could be number of pages if we want to pack this in to 12 bytes
    uint64_t RegionSize;
  };

  bool MergeReservedRegionIfPossible(ReservedVMARegion* Region, uintptr_t NextPtr, uint64_t NextSize) {
    constexpr uint64_t MaxReservedRegionSize = 64ULL * 1024 * 1024 * 1024; // 64GB
    uintptr_t RegionEnd = Region->Base + Region->RegionSize;
    uint64_t NewRegionSize = Region->RegionSize + NextSize;
    if (RegionEnd == NextPtr && NewRegionSize <= MaxReservedRegionSize) {
      // Append the contiguous region
      Region->RegionSize = NewRegionSize;
      return true;
    }
    return false;
  }

  struct LiveVMARegion {
    ReservedVMARegion* SlabInfo;
    uint64_t FreeSpace {};
    uint64_t NumManagedPages {};
    uint32_t LastPageAllocation {};
    bool HadMunmap {};

    // Align UsedPages so it pads to the next page.
    // Necessary to take advantage of madvise zero page pooling.
    using FlexBitElementType = uint64_t;
    alignas(4096) FEXCore::FlexBitSet<FlexBitElementType> UsedPages;

    // This returns the size of the LiveVMARegion in addition to the flex set that tracks the used data
    // The LiveVMARegion lives at the start of the VMA region which means on initialization we need to set that
    // tracked ranged as used immediately
    static size_t GetFEXManagedVMARegionSize(size_t Size) {
      // One element per page

      // 0x10'0000'0000 bytes
      // 0x100'0000 Pages
      // 1 bit per page for tracking means 0x20'0000 (Pages / 8) bytes of flex space
      // Which is 2MB of tracking
      const uint64_t NumElements = Size >> FEXCore::Utils::FEX_PAGE_SHIFT;
      return sizeof(LiveVMARegion) + FEXCore::FlexBitSet<FlexBitElementType>::SizeInBytes(NumElements);
    }

    static void InitializeVMARegionUsed(LiveVMARegion* Region, size_t AdditionalSize) {
      size_t SizeOfLiveRegion =
        FEXCore::AlignUp(LiveVMARegion::GetFEXManagedVMARegionSize(Region->SlabInfo->RegionSize), FEXCore::Utils::FEX_PAGE_SIZE);
      size_t SizePlusManagedData = SizeOfLiveRegion + AdditionalSize;

      Region->FreeSpace = Region->SlabInfo->RegionSize - SizePlusManagedData;

      size_t NumManagedPages = SizePlusManagedData >> FEXCore::Utils::FEX_PAGE_SHIFT;
      size_t ManagedSize = NumManagedPages << FEXCore::Utils::FEX_PAGE_SHIFT;

      // Use madvise to set the full tracking region to zero.
      // This ensures unused pages are zero, while not having the backing pages consuming memory.
      ::madvise(Region->UsedPages.Memory + ManagedSize, (Region->SlabInfo->RegionSize >> FEXCore::Utils::FEX_PAGE_SHIFT) - ManagedSize,
                MADV_DONTNEED);

      // Use madvise to claim WILLNEED on the beginning pages for initial state tracking.
      // Improves performance of the following MemClear by not doing a page level fault dance for data necessary to track >170TB of used pages.
      ::madvise(Region->UsedPages.Memory, ManagedSize, MADV_WILLNEED);

      // Set our reserved pages
      Region->UsedPages.MemSet(NumManagedPages);
      Region->LastPageAllocation = NumManagedPages;
      Region->NumManagedPages = NumManagedPages;
    }
  };

  static_assert(sizeof(LiveVMARegion) == 4096, "Needs to be the size of a page");

  static_assert(std::is_trivially_copyable<LiveVMARegion>::value, "Needs to be trivially copyable");
  static_assert(offsetof(LiveVMARegion, UsedPages) == sizeof(LiveVMARegion), "FlexBitSet needs to be at the end");

  using ReservedRegionListType = fex_pmr::list<ReservedVMARegion*>;
  using LiveRegionListType = fex_pmr::list<LiveVMARegion*>;
  ReservedRegionListType* ReservedRegions {};
  LiveRegionListType* LiveRegions {};

  Alloc::ForwardOnlyIntrusiveArenaAllocator* ObjectAlloc {};
  FEXCore::ForkableUniqueMutex AllocationMutex;
  void DetermineVASize();

  LiveVMARegion* MakeRegionActive(ReservedRegionListType::iterator ReservedIterator, uint64_t UsedSize) {
    ReservedVMARegion* ReservedRegion = *ReservedIterator;

    ReservedRegions->erase(ReservedIterator);

    // mprotect the new region we've allocated
    size_t SizeOfLiveRegion =
      FEXCore::AlignUp(LiveVMARegion::GetFEXManagedVMARegionSize(ReservedRegion->RegionSize), FEXCore::Utils::FEX_PAGE_SIZE);
    size_t SizePlusManagedData = UsedSize + SizeOfLiveRegion;

    auto Res = mprotect(reinterpret_cast<void*>(ReservedRegion->Base), SizePlusManagedData, PROT_READ | PROT_WRITE);
    LOGMAN_THROW_A_FMT(Res != -1, "Couldn't mprotect region: {} '{}' Likely occurs when running out of memory or Maximum VMAs", errno,
                       strerror(errno));

    LiveVMARegion* LiveRange = new (reinterpret_cast<void*>(ReservedRegion->Base)) LiveVMARegion();

    // Copy over the reserved data
    LiveRange->SlabInfo = ReservedRegion;

    // Initialize VMA
    LiveVMARegion::InitializeVMARegionUsed(LiveRange, UsedSize);

    // Add to our active tracked ranges
    auto LiveIter = LiveRegions->emplace_back(LiveRange);
    return LiveIter;
  }

  void AllocateMemoryRegions(fextl::vector<FEXCore::Allocator::MemoryRegion>& Ranges);
  LiveVMARegion* FindLiveRegionForAddress(uintptr_t Addr, uintptr_t AddrEnd);
};

void OSAllocator_64Bit::DetermineVASize() {
  size_t Bits = FEXCore::Allocator::DetermineVASize();
  uintptr_t Size = 1ULL << Bits;

  UPPER_BOUND = Size;

#if _M_X86_64 // Last page cannot be allocated on x86
  UPPER_BOUND -= FEXCore::Utils::FEX_PAGE_SIZE;
#endif

  UPPER_BOUND_PAGE = UPPER_BOUND / FEXCore::Utils::FEX_PAGE_SIZE;
}

OSAllocator_64Bit::LiveVMARegion* OSAllocator_64Bit::FindLiveRegionForAddress(uintptr_t Addr, uintptr_t AddrEnd) {
  LiveVMARegion* LiveRegion {};

  // Check active slabs to see if we can fit this
  for (auto it = LiveRegions->begin(); it != LiveRegions->end(); ++it) {
    uintptr_t RegionBegin = (*it)->SlabInfo->Base;
    uintptr_t RegionEnd = RegionBegin + (*it)->SlabInfo->RegionSize;

    if (Addr >= RegionBegin && Addr < RegionEnd) {
      LiveRegion = *it;
      // Leave our loop
      break;
    }
  }

  // Couldn't find an active region that fit
  // Check reserved regions
  if (!LiveRegion) {
    // Didn't have a slab that fit this range
    // Check our reserved regions to see if we have one that fits
    for (auto it = ReservedRegions->begin(); it != ReservedRegions->end(); ++it) {
      ReservedVMARegion* ReservedRegion = *it;
      uintptr_t RegionEnd = ReservedRegion->Base + ReservedRegion->RegionSize;
      if (Addr >= ReservedRegion->Base && AddrEnd < RegionEnd) {
        // Found one, let's make it active
        LiveRegion = MakeRegionActive(it, 0);
        break;
      }
    }
  }

  return LiveRegion;
}

void* OSAllocator_64Bit::Mmap(void* addr, size_t length, int prot, int flags, int fd, off_t offset) {
  if (addr != 0 && addr < reinterpret_cast<void*>(LOWER_BOUND)) {
    // If we are asked to allocate something outside of the 64-bit space
    // Then we need to just hand this to the OS
    return ::mmap(addr, length, prot, flags, fd, offset);
  }

  uint64_t Addr = reinterpret_cast<uint64_t>(addr);
  // Addr must be page aligned
  if (Addr & ~FEXCore::Utils::FEX_PAGE_MASK) {
    return reinterpret_cast<void*>(-EINVAL);
  }

  // If FD is provided then offset must also be page aligned
  if (fd != -1 && offset & ~FEXCore::Utils::FEX_PAGE_MASK) {
    return reinterpret_cast<void*>(-EINVAL);
  }

  // 64bit address overflow
  if (Addr + length < Addr) {
    return reinterpret_cast<void*>(-EOVERFLOW);
  }

  bool Fixed = (flags & MAP_FIXED) || (flags & MAP_FIXED_NOREPLACE);
  length = FEXCore::AlignUp(length, FEXCore::Utils::FEX_PAGE_SIZE);

  uint64_t AddrEnd = Addr + length;
  size_t NumberOfPages = length / FEXCore::Utils::FEX_PAGE_SIZE;

  // This needs a mutex to be thread safe
  auto lk = FEXCore::GuardSignalDeferringSectionWithFallback(AllocationMutex, TLSThread);

  uint64_t AllocatedOffset {};
  LiveVMARegion* LiveRegion {};

  if (Fixed || Addr != 0) {
    LiveRegion = FindLiveRegionForAddress(Addr, AddrEnd);
  }

again:

  struct RangeResult final {
    LiveVMARegion* RegionInsertedInto;
    void* Ptr;
  };

  auto CheckIfRangeFits = [&AllocatedOffset](LiveVMARegion* Region, uint64_t length, int prot, int flags, int fd, off_t offset,
                                             uint64_t StartingPosition = 0) -> RangeResult {
    uint64_t AllocatedPage {~0ULL};
    uint64_t NumberOfPages = length >> FEXCore::Utils::FEX_PAGE_SHIFT;

    if (Region->FreeSpace >= length) {
      uint64_t LastAllocation =
        StartingPosition ? (StartingPosition - Region->SlabInfo->Base) >> FEXCore::Utils::FEX_PAGE_SHIFT : Region->LastPageAllocation;
      size_t RegionNumberOfPages = Region->SlabInfo->RegionSize >> FEXCore::Utils::FEX_PAGE_SHIFT;


      if (Region->HadMunmap) {
        // Backward scan
        // We need to do a backward scan first to fill any holes
        // Otherwise we will very quickly run out of VMA regions (65k maximum)
        auto SearchResult = Region->UsedPages.BackwardScanForRange<true>(LastAllocation, NumberOfPages, Region->NumManagedPages);

        AllocatedPage = SearchResult.FoundElement;

        // If we didn't even have a one page free in the backward search, then unclaim HadMunmap.
        // Switching over to default forward search.
        if (SearchResult.FoundElement == ~0ULL && !SearchResult.FoundHole) {
          Region->HadMunmap = false;
        }
      }

      // Foward Scan
      if (AllocatedPage == ~0ULL) {
        auto SearchResult = Region->UsedPages.ForwardScanForRange<true>(LastAllocation, NumberOfPages, RegionNumberOfPages);
        AllocatedPage = SearchResult.FoundElement;
      }

      if (AllocatedPage != ~0ULL) {
        AllocatedOffset = Region->SlabInfo->Base + AllocatedPage * FEXCore::Utils::FEX_PAGE_SIZE;

        // We need to setup protections for this
        void* MMapResult = ::mmap(reinterpret_cast<void*>(AllocatedOffset), length, prot, (flags & ~MAP_FIXED_NOREPLACE) | MAP_FIXED, fd, offset);

        if (MMapResult == MAP_FAILED) {
          return RangeResult {Region, reinterpret_cast<void*>(-errno)};
        }
        return RangeResult {Region, MMapResult};
      }
    }

    return {};
  };

  if (Fixed) {
    // Found a region let's allocate to it
    if (LiveRegion) {
      // Found a slab that fits this
      if (flags & MAP_FIXED_NOREPLACE) {
        auto Fits = CheckIfRangeFits(LiveRegion, length, prot, flags, fd, offset, Addr);
        if (Fits.RegionInsertedInto && Fits.Ptr == reinterpret_cast<void*>(Addr)) {
          // We fit correctly
          AllocatedOffset = Addr;
        } else {
          // Intersected with something that already existed
          return reinterpret_cast<void*>(-EEXIST);
        }
      } else {
        // We need to mmap the file to this location
        void* MMapResult = ::mmap(reinterpret_cast<void*>(Addr), length, prot, (flags & ~MAP_FIXED_NOREPLACE) | MAP_FIXED, fd, offset);

        if (MMapResult == MAP_FAILED) {
          return reinterpret_cast<void*>(-errno);
        }

        AllocatedOffset = Addr;
      }
      // Fall through to live region tracking
    }
  } else {
    // Check our active slabs to see if we can fit the allocation
    // Slightly different than fixed since it doesn't need exact placement
    if (LiveRegion && Addr != 0) {
      // We found a LiveRegion that could hold this address. Let's try to place it
      // Check if this area is free
      auto Fits = CheckIfRangeFits(LiveRegion, length, prot, flags, fd, offset, Addr);
      if (Fits.RegionInsertedInto && Fits.Ptr == reinterpret_cast<void*>(Addr)) {
        // We fit correctly
        AllocatedOffset = Addr;
      } else {
        // Couldn't fit
        // We can continue past this point still
        LiveRegion = nullptr;
      }
    }

    if (!LiveRegion) {
      for (auto it = LiveRegions->begin(); it != LiveRegions->end(); ++it) {
        auto Fits = CheckIfRangeFits(*it, length, prot, flags, fd, offset);
        if (Fits.RegionInsertedInto && Fits.Ptr == reinterpret_cast<void*>(AllocatedOffset)) {
          // We fit correctly
          LiveRegion = Fits.RegionInsertedInto;
          break;
        }

        // Couldn't fit but mmap gave us an error
        if (!Fits.RegionInsertedInto && Fits.Ptr) {
          return Fits.Ptr;
        }

        // nullptr on both means no error and couldn't fit
      }
    }

    if (!LiveRegion) {
      // Couldn't find a fit in the live regions
      // Allocate a new reserved region
      size_t lengthOfLiveRegion = FEXCore::AlignUp(LiveVMARegion::GetFEXManagedVMARegionSize(length), FEXCore::Utils::FEX_PAGE_SIZE);
      size_t lengthPlusManagedData = length + lengthOfLiveRegion;
      for (auto it = ReservedRegions->begin(); it != ReservedRegions->end(); ++it) {
        if ((*it)->RegionSize >= lengthPlusManagedData) {
          MakeRegionActive(it, 0);
          goto again;
        }
      }
    }
  }

  if (LiveRegion) {
    // Mark the pages as used
    uintptr_t RegionBegin = LiveRegion->SlabInfo->Base;
    uintptr_t MappedBegin = (AllocatedOffset - RegionBegin) >> FEXCore::Utils::FEX_PAGE_SHIFT;

    for (size_t i = 0; i < NumberOfPages; ++i) {
      LiveRegion->UsedPages.Set(MappedBegin + i);
    }

    // Change our last allocation region
    LiveRegion->LastPageAllocation = MappedBegin + NumberOfPages;
    LiveRegion->FreeSpace -= length;
  }

  if (!AllocatedOffset) {
    AllocatedOffset = -ENOMEM;
  }
  return reinterpret_cast<void*>(AllocatedOffset);
}

int OSAllocator_64Bit::Munmap(void* addr, size_t length) {
  if (addr < reinterpret_cast<void*>(LOWER_BOUND)) {
    // If we are asked to allocate something outside of the 64-bit space
    // Then we need to just hand this to the OS
    return ::munmap(addr, length);
  }

  uint64_t Addr = reinterpret_cast<uint64_t>(addr);

  if (Addr & ~FEXCore::Utils::FEX_PAGE_MASK) {
    return -EINVAL;
  }

  if (length & ~FEXCore::Utils::FEX_PAGE_MASK) {
    return -EINVAL;
  }

  if (Addr + length < Addr) {
    return -EOVERFLOW;
  }

  // This needs a mutex to be thread safe
  auto lk = FEXCore::GuardSignalDeferringSectionWithFallback(AllocationMutex, TLSThread);

  length = FEXCore::AlignUp(length, FEXCore::Utils::FEX_PAGE_SIZE);

  uintptr_t PtrBegin = reinterpret_cast<uintptr_t>(addr);
  uintptr_t PtrEnd = PtrBegin + length;
  // Walk all of the live ranges and find this slab then delete it
  for (auto it = LiveRegions->begin(); it != LiveRegions->end(); ++it) {
    uintptr_t RegionBegin = (*it)->SlabInfo->Base;
    uintptr_t RegionEnd = RegionBegin + (*it)->SlabInfo->RegionSize;

    if (RegionBegin <= PtrBegin && RegionEnd > PtrEnd) {
      // Live region fully encompasses slab range

      uint64_t FreedPages {};
      uint32_t SlabPageBegin = (PtrBegin - RegionBegin) >> FEXCore::Utils::FEX_PAGE_SHIFT;
      uint64_t PagesToFree = length >> FEXCore::Utils::FEX_PAGE_SHIFT;

      for (size_t i = 0; i < PagesToFree; ++i) {
        FreedPages += (*it)->UsedPages.TestAndClear(SlabPageBegin + i) ? 1 : 0;
      }

      if (FreedPages != 0) {
        // If we were contiuous freeing then make sure to give back the physical address space
        // If the region was locked then madvise won't remove the physical backing
        // This woul be a bug in the frontend application
        // So be careful with mlock/munlock
        ::madvise(addr, length, MADV_DONTNEED);
        ::mmap(addr, length, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
      }

      (*it)->FreeSpace += FreedPages * 4096;

      // Set the last allocated page to the minimum of last page allocation or this slab
      // This will let us more quickly fill holes
      (*it)->LastPageAllocation = std::min((*it)->LastPageAllocation, SlabPageBegin);

      (*it)->HadMunmap = true;

      // XXX: Move region back to reserved list
      return 0;
    }
  }

  // If it didn't match at all then no error
  return 0;
}

void OSAllocator_64Bit::AllocateMemoryRegions(fextl::vector<FEXCore::Allocator::MemoryRegion>& Ranges) {
  // Need to allocate the ObjectAlloc up front. Find a region that is larger than our minimum size first.
  const size_t ObjectAllocSize = 64 * 1024 * 1024;

  for (auto& it : Ranges) {
    if (ObjectAllocSize > it.Size) {
      continue;
    }

    // Allocate up to 64 MiB the first allocation for an intrusive allocator
    mprotect(it.Ptr, ObjectAllocSize, PROT_READ | PROT_WRITE);

    // This enables the kernel to use transparent large pages in the allocator which can reduce memory pressure
    ::madvise(it.Ptr, ObjectAllocSize, MADV_HUGEPAGE);

    ObjectAlloc = new (it.Ptr) Alloc::ForwardOnlyIntrusiveArenaAllocator(it.Ptr, ObjectAllocSize);
    ReservedRegions = ObjectAlloc->new_construct(ReservedRegions, ObjectAlloc);
    LiveRegions = ObjectAlloc->new_construct(LiveRegions, ObjectAlloc);

    if (it.Size >= ObjectAllocSize) {
      // Modify region size
      it.Size -= ObjectAllocSize;
      (uint8_t*&)it.Ptr += ObjectAllocSize;
    }

    break;
  }

  if (!ObjectAlloc) {
    ERROR_AND_DIE_FMT("Couldn't allocate object allocator!");
  }

  for (auto [Ptr, AllocationSize] : Ranges) {
    // Skip using any regions that are <= two pages. FEX's VMA allocator requires two pages
    // for tracking data. So three pages are minimum for a single page VMA allocation.
    if (AllocationSize <= (FEXCore::Utils::FEX_PAGE_SIZE * 2)) {
      continue;
    }

    ReservedVMARegion* Region = ObjectAlloc->new_construct<ReservedVMARegion>();
    Region->Base = reinterpret_cast<uint64_t>(Ptr);
    Region->RegionSize = AllocationSize;
    ReservedRegions->emplace_back(Region);
  }
}


OSAllocator_64Bit::OSAllocator_64Bit() {
  DetermineVASize();

  auto Ranges = FEXCore::Allocator::StealMemoryRegion(LOWER_BOUND, UPPER_BOUND);

  AllocateMemoryRegions(Ranges);
}

OSAllocator_64Bit::OSAllocator_64Bit(fextl::vector<FEXCore::Allocator::MemoryRegion>& Regions) {
  AllocateMemoryRegions(Regions);
}

OSAllocator_64Bit::~OSAllocator_64Bit() {
  // This needs a mutex to be thread safe
  auto lk = FEXCore::GuardSignalDeferringSectionWithFallback(AllocationMutex, TLSThread);

  // Walk the pages and deallocate
  // First walk the live regions
  for (auto it = LiveRegions->begin(); it != LiveRegions->end(); ++it) {
    ::munmap(reinterpret_cast<void*>((*it)->SlabInfo->Base), (*it)->SlabInfo->RegionSize);
  }

  // Now walk the reserved regions
  for (auto it = ReservedRegions->begin(); it != ReservedRegions->end(); ++it) {
    ::munmap(reinterpret_cast<void*>((*it)->Base), (*it)->RegionSize);
  }
}

fextl::unique_ptr<Alloc::HostAllocator> Create64BitAllocator() {
  return fextl::make_unique<OSAllocator_64Bit>();
}

template<class T>
struct alloc_delete : public std::default_delete<T> {
  void operator()(T* ptr) const {
    if (ptr) {
      const auto size = sizeof(T);
      const auto MinPage = FEXCore::AlignUp(size, FEXCore::Utils::FEX_PAGE_SIZE);

      std::destroy_at(ptr);
      ::munmap(ptr, MinPage);
    }
  }

  template<typename U>
  requires (std::is_base_of_v<U, T>)
  operator fextl::default_delete<U>() {
    return fextl::default_delete<U>();
  }
};

template<class T, class... Args>
requires (!std::is_array_v<T>)
fextl::unique_ptr<T> make_alloc_unique(FEXCore::Allocator::MemoryRegion& Base, Args&&... args) {
  const auto size = sizeof(T);
  const auto MinPage = FEXCore::AlignUp(size, FEXCore::Utils::FEX_PAGE_SIZE);
  if (Base.Size < size || MinPage != FEXCore::Utils::FEX_PAGE_SIZE) {
    ERROR_AND_DIE_FMT("Couldn't fit allocator in to page!");
  }

  auto ptr = ::mmap(Base.Ptr, MinPage, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
  if (ptr == MAP_FAILED) {
    ERROR_AND_DIE_FMT("Couldn't allocate memory region");
  }

  // Remove the page from the base region.
  // Could be zero after this.
  Base.Size -= MinPage;
  Base.Ptr = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(Base.Ptr) + MinPage);

  auto Result = ::new (ptr) T(std::forward<Args>(args)...);
  return fextl::unique_ptr<T, alloc_delete<T>>(Result);
}

fextl::unique_ptr<Alloc::HostAllocator> Create64BitAllocatorWithRegions(fextl::vector<FEXCore::Allocator::MemoryRegion>& Regions) {
  // This is a bit tricky as we can't allocate memory safely except from the Regions provided. Otherwise we might overwrite memory pages we
  // don't own. Scan the memory regions and find the smallest one.
  FEXCore::Allocator::MemoryRegion& Smallest = Regions[0];
  for (auto& it : Regions) {
    if (it.Size <= Smallest.Size) {
      Smallest = it;
    }
  }

  return make_alloc_unique<OSAllocator_64Bit>(Smallest, Regions);
}

} // namespace Alloc::OSAllocator

namespace FEXCore::Allocator {
void RegisterTLSData(FEXCore::Core::InternalThreadState* Thread) {
  Alloc::OSAllocator::TLSThread = Thread;
}

void UninstallTLSData(FEXCore::Core::InternalThreadState* Thread) {
  Alloc::OSAllocator::TLSThread = nullptr;
}
} // namespace FEXCore::Allocator
