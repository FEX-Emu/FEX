#include "Utils/Allocator/FlexBitSet.h"
#include "Utils/Allocator/HostAllocator.h"
#include "Utils/Allocator/IntrusiveArenaAllocator.h"
#include <FEXCore/Utils/Allocator.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/MathUtils.h>
#include <FEXHeaderUtils/ScopedSignalMask.h>
#include <FEXHeaderUtils/Syscalls.h>
#include <FEXHeaderUtils/TypeDefines.h>

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
#include <sstream>
#include <sys/mman.h>
#include <sys/utsname.h>
#include <sys/user.h>
#include <type_traits>
#include <utility>

namespace Alloc::OSAllocator {

  class OSAllocator_64Bit final : public Alloc::HostAllocator {
    public:
      OSAllocator_64Bit();
      virtual ~OSAllocator_64Bit();
      void *AllocateSlab(size_t Size) override { return nullptr; }
      void DeallocateSlab(void *Ptr, size_t Size) override {}

      void *Mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset) override;
      int Munmap(void *addr, size_t length) override;

    private:
      // Upper bound is the maximum virtual address space of the host processor
      uintptr_t UPPER_BOUND = (1ULL << 57);

      // Lower bound is the starting of the range just past the lower 32bits
      constexpr static uintptr_t LOWER_BOUND = 0x1'0000'0000ULL;

      uintptr_t UPPER_BOUND_PAGE = UPPER_BOUND / FHU::FEX_PAGE_SIZE;
      constexpr static uintptr_t LOWER_BOUND_PAGE = LOWER_BOUND / FHU::FEX_PAGE_SIZE;

      struct ReservedVMARegion {
        uintptr_t Base;
        // Could be number of pages if we want to pack this in to 12 bytes
        uint64_t RegionSize;
      };

      bool MergeReservedRegionIfPossible(ReservedVMARegion *Region, uintptr_t NextPtr, uint64_t NextSize) {
        constexpr uint64_t MaxReservedRegionSize = 64ULL * 1024 * 1024 * 1024;   // 64GB
        uintptr_t RegionEnd = Region->Base + Region->RegionSize;
        uint64_t NewRegionSize = Region->RegionSize + NextSize;
        if (RegionEnd == NextPtr &&
            NewRegionSize <= MaxReservedRegionSize) {
          // Append the contiguous region
          Region->RegionSize = NewRegionSize;
          return true;
        }
        return false;
      }

      struct LiveVMARegion {
        ReservedVMARegion *SlabInfo;
        uint64_t FreeSpace{};
        uint32_t LastPageAllocation{};
        FEXCore::FlexBitSet<uint64_t> UsedPages;

        // This returns the size of the LiveVMARegion in addition to the flex set that tracks the used data
        // The LiveVMARegion lives at the start of the VMA region which means on initialization we need to set that
        // tracked ranged as used immediately
        static size_t GetSizeWithFlexSet(size_t Size) {
          // One element per page

          // 0x10'0000'0000 bytes
          // 0x100'0000 Pages
          // 1 bit per page for tracking means 0x20'0000 (Pages / 8) bytes of flex space
          // Which is 2MB of tracking
          uint64_t NumElements = (Size >> FHU::FEX_PAGE_SHIFT) * sizeof(uint64_t);
          return sizeof(LiveVMARegion) + FEXCore::FlexBitSet<uint64_t>::Size(NumElements);
        }

        static void InitializeVMARegionUsed(LiveVMARegion *Region, size_t AdditionalSize) {
          size_t SizeOfLiveRegion = FEXCore::AlignUp(LiveVMARegion::GetSizeWithFlexSet(Region->SlabInfo->RegionSize), FHU::FEX_PAGE_SIZE);
          size_t SizePlusManagedData = SizeOfLiveRegion + AdditionalSize;

          Region->FreeSpace = Region->SlabInfo->RegionSize - SizePlusManagedData;

          size_t NumPages = SizePlusManagedData >> FHU::FEX_PAGE_SHIFT;
          // Memset the full tracking to zero to state nothing used
          Region->UsedPages.MemSet(Region->SlabInfo->RegionSize >> FHU::FEX_PAGE_SHIFT);
          // Set our reserved pages
          for (size_t i = 0; i < NumPages; ++i) {
            // Set our used pages
            Region->UsedPages.Set(i);
          }
          Region->LastPageAllocation = NumPages;
        }
      };
      static_assert(std::is_trivially_copyable<LiveVMARegion>::value, "Needs to be trivially copyable");
      static_assert(offsetof(LiveVMARegion, UsedPages) == sizeof(LiveVMARegion), "FlexBitSet needs to be at the end");

      using ReservedRegionListType = fex_pmr::list<ReservedVMARegion*>;
      using LiveRegionListType = fex_pmr::list<LiveVMARegion*>;
      ReservedRegionListType *ReservedRegions{};
      LiveRegionListType *LiveRegions{};

      Alloc::ForwardOnlyIntrusiveArenaAllocator *ObjectAlloc{};
      std::mutex AllocationMutex{};
      void DetermineVASize();

      LiveVMARegion *MakeRegionActive(ReservedRegionListType::iterator ReservedIterator, uint64_t UsedSize) {
        ReservedVMARegion *ReservedRegion = *ReservedIterator;

        ReservedRegions->erase(ReservedIterator);
        // mprotect the new region we've allocated
        size_t SizeOfLiveRegion = FEXCore::AlignUp(LiveVMARegion::GetSizeWithFlexSet(ReservedRegion->RegionSize), FHU::FEX_PAGE_SIZE);
        size_t SizePlusManagedData = UsedSize + SizeOfLiveRegion;

        [[maybe_unused]] auto Res = mprotect(reinterpret_cast<void*>(ReservedRegion->Base), SizePlusManagedData, PROT_READ | PROT_WRITE);

        LOGMAN_THROW_AA_FMT(Res == 0, "Couldn't mprotect region: {} '{}' Likely occurs when running out of memory or Maximum VMAs", errno, strerror(errno));

        LiveVMARegion *LiveRange = new (reinterpret_cast<void*>(ReservedRegion->Base)) LiveVMARegion();

        // Copy over the reserved data
        LiveRange->SlabInfo = ReservedRegion;
        // Initialize VMA
        LiveVMARegion::InitializeVMARegionUsed(LiveRange, UsedSize);

        // Add to our active tracked ranges
        auto LiveIter = LiveRegions->emplace_back(LiveRange);

        return LiveIter;
      }

      // 32-bit old kernel workarounds
      FEXCore::Allocator::PtrCache *Steal32BitIfOldKernel();
  };

void OSAllocator_64Bit::DetermineVASize() {
  size_t Bits = FEXCore::Allocator::DetermineVASize();
  uintptr_t Size = 1ULL << Bits;
  UPPER_BOUND = Size;
  UPPER_BOUND_PAGE = UPPER_BOUND / FHU::FEX_PAGE_SIZE;
}

void *OSAllocator_64Bit::Mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset) {
  if (addr != 0 &&
      addr < reinterpret_cast<void*>(LOWER_BOUND)) {
    // If we are asked to allocate something outside of the 64-bit space
    // Then we need to just hand this to the OS
    return ::mmap(addr, length, prot, flags, fd, offset);
  }

  uint64_t Addr = reinterpret_cast<uint64_t>(addr);
  // Addr must be page aligned
  if (Addr & ~FHU::FEX_PAGE_MASK) {
    return reinterpret_cast<void*>(-EINVAL);
  }

  // If FD is provided then offset must also be page aligned
  if (fd != -1 &&
      offset & ~FHU::FEX_PAGE_MASK) {
    return reinterpret_cast<void*>(-EINVAL);
  }

  // 64bit address overflow
  if (Addr + length < Addr) {
    return reinterpret_cast<void*>(-EOVERFLOW);
  }

  bool Fixed = (flags & MAP_FIXED) || (flags & MAP_FIXED_NOREPLACE);
  length = FEXCore::AlignUp(length, FHU::FEX_PAGE_SIZE);

  uint64_t AddrEnd = Addr + length;
  size_t NumberOfPages = length / FHU::FEX_PAGE_SIZE;

  // This needs a mutex to be thread safe
  FHU::ScopedSignalCheckWithLockGuard lk(AllocationMutex);

  uint64_t AllocatedOffset{};
  LiveVMARegion *LiveRegion{};

  if (Fixed || Addr != 0) {
    // Check active slabs to see if we can fit this
    for (auto it = LiveRegions->begin(); it != LiveRegions->end(); ++it) {
      uintptr_t RegionBegin = (*it)->SlabInfo->Base;
      uintptr_t RegionEnd = RegionBegin + (*it)->SlabInfo->RegionSize;

      if (Addr >= RegionBegin &&
          Addr < RegionEnd) {
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
        ReservedVMARegion *ReservedRegion = *it;
        uintptr_t RegionEnd = ReservedRegion->Base + ReservedRegion->RegionSize;
        if (Addr >= ReservedRegion->Base &&
            AddrEnd < RegionEnd) {
          // Found one, let's make it active
          LiveRegion = MakeRegionActive(it, 0);
          break;
        }
      }
    }
  }

  again:

  auto CheckIfRangeFits = [&AllocatedOffset](LiveVMARegion *Region, uint64_t length, int prot, int flags, int fd, off_t offset, uint64_t StartingPosition = 0) -> std::pair<LiveVMARegion*, void*> {
    uint64_t AllocatedPage{};
    uint64_t NumberOfPages = length >> FHU::FEX_PAGE_SHIFT;

    if (Region->FreeSpace >= length) {
      uint64_t LastAllocation =
        StartingPosition ?
        (StartingPosition - Region->SlabInfo->Base) >> FHU::FEX_PAGE_SHIFT
        : Region->LastPageAllocation;
      size_t RegionNumberOfPages = Region->SlabInfo->RegionSize >> FHU::FEX_PAGE_SHIFT;

      // Backward scan
      // We need to do a backward scan first to fill any holes
      // Otherwise we will very quickly run out of VMA regions (65k maximum)
      for (size_t CurrentPage = LastAllocation;
           CurrentPage >= NumberOfPages;) {
        size_t Remaining = NumberOfPages;
        assert(Remaining <= CurrentPage);

        while (Remaining) {
          if (Region->UsedPages[CurrentPage - Remaining]) {
            // Has an intersecting range
            break;
          }
          --Remaining;
        }

        if (Remaining) {
          // Didn't find a slab range
          CurrentPage -= Remaining;
        }
        else {
          // We have a slab range
          CurrentPage -= NumberOfPages;

          // Keep scanning backwards to not introduce ANOTHER gap
          while (CurrentPage >= 1) {
            if (Region->UsedPages[CurrentPage - 1]) {
              // Found a used page, we can leave now
              break;
            }
            --CurrentPage;
          }
          AllocatedPage = CurrentPage;
          break;
        }
      }

      // Foward Scan
      if (AllocatedPage == 0) {
        for (size_t CurrentPage = LastAllocation;
             CurrentPage < (RegionNumberOfPages - NumberOfPages);) {
          // If we have enough free space, check if we have enough free pages that are contiguous
          size_t Remaining = NumberOfPages;

          assert((CurrentPage + Remaining - 1) < RegionNumberOfPages);
          while (Remaining) {
            if (Region->UsedPages[CurrentPage + Remaining - 1]) {
              // Has an intersecting range
              break;
            }
            --Remaining;
          }

          if (Remaining) {
            // Didn't find a slab range
            CurrentPage += Remaining;
          }
          else {
            // We have a slab range
            AllocatedPage = CurrentPage;
            break;
          }
        }
      }

      if (AllocatedPage) {
        AllocatedOffset = Region->SlabInfo->Base + AllocatedPage * FHU::FEX_PAGE_SIZE;

        // We need to setup protections for this
        void *MMapResult = ::mmap(reinterpret_cast<void*>(AllocatedOffset),
          length,
          prot,
          (flags & ~MAP_FIXED_NOREPLACE) | MAP_FIXED,
          fd, offset);

        if (MMapResult == MAP_FAILED) {
          return std::make_pair(Region, reinterpret_cast<void*>(-errno));
        }
        return std::make_pair(Region, MMapResult);
      }
    }

    return std::make_pair(nullptr, nullptr);
  };

  if (Fixed) {
    // Found a region let's allocate to it
    if (LiveRegion) {
      // Found a slab that fits this
      if (flags & MAP_FIXED_NOREPLACE) {
        auto Fits = CheckIfRangeFits(LiveRegion, length, prot, flags, fd, offset, Addr);
        if (Fits.first && Fits.second == reinterpret_cast<void*>(Addr)) {
          // We fit correctly
          AllocatedOffset = Addr;
        }
        else {
          // Intersected with something that already existed
          return reinterpret_cast<void*>(-EEXIST);
        }
      }
      else {
        // We need to mmap the file to this location
        void *MMapResult = ::mmap(reinterpret_cast<void*>(Addr),
          length,
          prot,
          (flags & ~MAP_FIXED_NOREPLACE) | MAP_FIXED,
          fd, offset);

        if (MMapResult == MAP_FAILED) {
          return reinterpret_cast<void*>(-errno);
        }

        AllocatedOffset = Addr;
      }
      // Fall through to live region tracking
    }
  }
  else {
    // Check our active slabs to see if we can fit the allocation
    // Slightly different than fixed since it doesn't need exact placement
    if (LiveRegion && Addr != 0) {
      // We found a LiveRegion that could hold this address. Let's try to place it
      // Check if this area is free
      auto Fits = CheckIfRangeFits(LiveRegion, length, prot, flags, fd, offset, Addr);
      if (Fits.first && Fits.second == reinterpret_cast<void*>(Addr)) {
        // We fit correctly
        AllocatedOffset = Addr;
      }
      else {
        // Couldn't fit
        // We can continue past this point still
        LiveRegion = nullptr;
      }
    }

    if (!LiveRegion) {
      for (auto it = LiveRegions->begin(); it != LiveRegions->end(); ++it) {
        auto Fits = CheckIfRangeFits(*it, length, prot, flags, fd, offset);
        if (Fits.first && Fits.second == reinterpret_cast<void*>(AllocatedOffset)) {
          // We fit correctly
          LiveRegion = Fits.first;
          break;
        }

        // Couldn't fit but mmap gave us an error
        if (!Fits.first && Fits.second) {
          return Fits.second;
        }

        // nullptr on both means no error and couldn't fit
      }
    }

    if (!LiveRegion) {
      // Couldn't find a fit in the live regions
      // Allocate a new reserved region
      size_t lengthOfLiveRegion = FEXCore::AlignUp(LiveVMARegion::GetSizeWithFlexSet(length), FHU::FEX_PAGE_SIZE);
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
    uintptr_t MappedBegin = (AllocatedOffset - RegionBegin) >> FHU::FEX_PAGE_SHIFT;

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

int OSAllocator_64Bit::Munmap(void *addr, size_t length) {
  if (addr < reinterpret_cast<void*>(LOWER_BOUND)) {
    // If we are asked to allocate something outside of the 64-bit space
    // Then we need to just hand this to the OS
    return ::munmap(addr, length);
  }

  uint64_t Addr = reinterpret_cast<uint64_t>(addr);

  if (Addr & ~FHU::FEX_PAGE_MASK) {
    return -EINVAL;
  }

  if (length & ~FHU::FEX_PAGE_MASK) {
    return -EINVAL;
  }

  if (Addr + length < Addr) {
    return -EOVERFLOW;
  }

  // This needs a mutex to be thread safe
  FHU::ScopedSignalCheckWithLockGuard lk(AllocationMutex);

  length = FEXCore::AlignUp(length, FHU::FEX_PAGE_SIZE);

  uintptr_t PtrBegin = reinterpret_cast<uintptr_t>(addr);
  uintptr_t PtrEnd = PtrBegin + length;
  // Walk all of the live ranges and find this slab then delete it
  for (auto it = LiveRegions->begin(); it != LiveRegions->end(); ++it) {
    uintptr_t RegionBegin = (*it)->SlabInfo->Base;
    uintptr_t RegionEnd = RegionBegin + (*it)->SlabInfo->RegionSize;

    if (RegionBegin <= PtrBegin &&
        RegionEnd > PtrEnd) {
      // Live region fully encompasses slab range

      uint64_t FreedPages{};
      uint32_t SlabPageBegin = (PtrBegin - RegionBegin) >> FHU::FEX_PAGE_SHIFT;
      uint64_t PagesToFree = length >> FHU::FEX_PAGE_SHIFT;

      for (size_t i = 0; i < PagesToFree; ++i) {
        FreedPages += (*it)->UsedPages.TestAndClear(SlabPageBegin + i) ? 1 : 0;
      }

      if (FreedPages != 0)
      {
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

      // XXX: Move region back to reserved list
      return 0;
    }
  }

  // If it didn't match at all then no error
  return 0;
}

FEXCore::Allocator::PtrCache *OSAllocator_64Bit::Steal32BitIfOldKernel() {
  // First calculate kernel version
  struct utsname buf{};
  if (uname(&buf) == -1) {
    return nullptr;
  }

  int32_t Major{};
  int32_t Minor{};
  int32_t Patch{};
  char Tmp{};
  std::istringstream ss{buf.release};
  ss >> Major;
  ss.read(&Tmp, 1);
  ss >> Minor;
  ss.read(&Tmp, 1);
  ss >> Patch;
  ss.read(&Tmp, 1);
  uint32_t Version = (Major << 24) | (Minor << 16) | Patch;

  if (Version >= ((4 << 24) | (17 << 16) | 0)) {
    // If the kernel is >= 4.17 then it supports MAP_FIXED_NOREPLACE
    return nullptr;
  }

  constexpr size_t LOWER_BOUND_32 = 0x1'0000;
  constexpr size_t UPPER_BOUND_32 = LOWER_BOUND;

  return FEXCore::Allocator::StealMemoryRegion(LOWER_BOUND_32, UPPER_BOUND_32);
}

OSAllocator_64Bit::OSAllocator_64Bit() {
  DetermineVASize();
  auto ArrayPtr = Steal32BitIfOldKernel();

  // On allocation try and steal the entire upper 64bits of address space for mapping
  constexpr std::array<size_t, 8> ReservedVMARegionSizes = {{
    // Anything larger than 64GB fails out
    64ULL * 1024 * 1024 * 1024,   // 64GB
    32ULL * 1024 * 1024 * 1024,   // 32GB
    16ULL * 1024 * 1024 * 1024,   // 16GB
    4ULL * 1024 * 1024 * 1024,    // 4GB
    1ULL * 1024 * 1024 * 1024,    // 1GB
    512ULL * 1024 * 1024,         // 512MB
    128ULL * 1024 * 1024,         // 128MB
    4096ULL                       // One page
  }};

  constexpr size_t AllocationSizeMaxIndex = ReservedVMARegionSizes.size() - 1;

  // Have the first region only be 4GB VMA
  // Avoids conflicts with some tests
  uint64_t CurrentSizeIndex = 3;
  ReservedVMARegion *PrevReserved{};
  for (size_t MemoryOffset = LOWER_BOUND; MemoryOffset < UPPER_BOUND;) {
    size_t AllocationSize = ReservedVMARegionSizes[CurrentSizeIndex];
    size_t MemoryOffsetUpper = MemoryOffset + AllocationSize;

    // If we would go above the upper bound on size then try the next size
    if (MemoryOffsetUpper > UPPER_BOUND) {
      ++CurrentSizeIndex;
      continue;
    }

    void *Ptr = ::mmap(reinterpret_cast<void*>(MemoryOffset), AllocationSize, PROT_NONE, MAP_FIXED_NOREPLACE | MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);

    // If we managed to allocate and not get the address we want then unmap it
    // This happens with kernels older than 4.17
    if (reinterpret_cast<uintptr_t>(Ptr) != MemoryOffset &&
        reinterpret_cast<uintptr_t>(Ptr) < LOWER_BOUND) {
      ::munmap(Ptr, AllocationSize);
      Ptr = reinterpret_cast<void*>(~0ULL);
    }

    // If we failed to allocate and we are on the smallest allocation size then just continue onward
    // This page was unmappable
    if (reinterpret_cast<uintptr_t>(Ptr) == ~0ULL && CurrentSizeIndex == AllocationSizeMaxIndex) {
      CurrentSizeIndex = 0;
      MemoryOffset += AllocationSize;
      continue;
    }

    // Congratulations we were able to map this bit
    // Reset and claim it was available
    if (reinterpret_cast<uintptr_t>(Ptr) != ~0ULL) {
      if (!ObjectAlloc) {
        // Steal the first allocation for an intrusive allocator
        // Will be mprotected correctly already
        mprotect(Ptr, AllocationSize, PROT_READ | PROT_WRITE);
        ObjectAlloc = new (Ptr) Alloc::ForwardOnlyIntrusiveArenaAllocator(Ptr, AllocationSize);
        ReservedRegions = ObjectAlloc->new_construct(ReservedRegions, ObjectAlloc);
        LiveRegions = ObjectAlloc->new_construct(LiveRegions, ObjectAlloc);
      }
      else {

        // If the allocation size is large than a page, then try allowing it to be a huge page
        // This enables the kernel to use transparent large pages in the allocator which can reduce memory pressure
        // Considering we are allocating the entire VA space, this is a good thing
        // If MADV_HUGEPAGE isn't support then this will fail harmlessly
        if (AllocationSize > 4096) {
          ::madvise(Ptr, AllocationSize, MADV_HUGEPAGE);
        }

        bool Merged = false;
        if (PrevReserved) {
          Merged = MergeReservedRegionIfPossible(PrevReserved, reinterpret_cast<uint64_t>(Ptr), AllocationSize);
        }

        if (!Merged) {
          ReservedVMARegion *Region = ObjectAlloc->new_construct<ReservedVMARegion>();
          Region->Base = reinterpret_cast<uint64_t>(Ptr);
          Region->RegionSize = AllocationSize;
          ReservedRegions->emplace_back(Region);
          PrevReserved = Region;
        }
      }

      CurrentSizeIndex = 0;
      MemoryOffset += AllocationSize;
      continue;
    }

    // Couldn't allocate at this size
    // Increase and continue
    ++CurrentSizeIndex;
  }

  FEXCore::Allocator::ReclaimMemoryRegion(ArrayPtr);
}

OSAllocator_64Bit::~OSAllocator_64Bit() {
  // This needs a mutex to be thread safe
  FHU::ScopedSignalCheckWithLockGuard lk(AllocationMutex);

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

std::unique_ptr<Alloc::HostAllocator> Create64BitAllocator() {
  return std::make_unique<OSAllocator_64Bit>();
}
}
