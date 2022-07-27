#include "Utils/Allocator/HostAllocator.h"
#include <FEXCore/Utils/Allocator.h>
#include <FEXCore/Utils/CompilerDefs.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXHeaderUtils/Syscalls.h>
#include <FEXHeaderUtils/TypeDefines.h>

#include <array>
#include <cstdint>
#include <sys/mman.h>
#include <sys/user.h>
#include <vector>
#ifdef ENABLE_JEMALLOC
#include <jemalloc/jemalloc.h>
#endif
#include <errno.h>
#include <memory>
#include <stddef.h>
#include <stdint.h>

extern "C" {
  typedef void* (*mmap_hook_type)(
            void *addr, size_t length, int prot, int flags,
            int fd, off_t offset);
  typedef int (*munmap_hook_type)(void *addr, size_t length);

#ifdef ENABLE_JEMALLOC
  extern mmap_hook_type __mmap_hook;
  extern munmap_hook_type __munmap_hook;
#endif
}

namespace FEXCore::Allocator {
  MMAP_Hook mmap {::mmap};
  MUNMAP_Hook munmap {::munmap};
#ifdef ENABLE_JEMALLOC
  MALLOC_Hook malloc {::je_malloc};
  REALLOC_Hook realloc {::je_realloc};
  FREE_Hook free {::je_free};
#else
  MALLOC_Hook malloc {::malloc};
  REALLOC_Hook realloc {::realloc};
  FREE_Hook free {::free};
#endif

  using GLIBC_MALLOC_Hook = void*(*)(size_t, const void *caller);
  using GLIBC_REALLOC_Hook = void*(*)(void*, size_t, const void *caller);
  using GLIBC_FREE_Hook = void(*)(void*, const void *caller);

  std::unique_ptr<Alloc::HostAllocator> Alloc64{};

  void *FEX_mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset) {
    void *Result = Alloc64->Mmap(addr, length, prot, flags, fd, offset);
    if (Result >= (void*)-4096) {
      errno = -(uint64_t)Result;
      return (void*)-1;
    }
    return Result;
  }
  int FEX_munmap(void *addr, size_t length) {
    int Result = Alloc64->Munmap(addr, length);

    if (Result != 0) {
      errno = -Result;
      return -1;
    }
    return Result;
  }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  void SetupHooks() {
    Alloc64 = Alloc::OSAllocator::Create64BitAllocator();
#ifdef ENABLE_JEMALLOC
    //__mmap_hook   = FEX_mmap;
    //__munmap_hook = FEX_munmap;
#endif
    FEXCore::Allocator::mmap = FEX_mmap;
    FEXCore::Allocator::munmap = FEX_munmap;
  }

  void ClearHooks() {
#ifdef ENABLE_JEMALLOC
    __mmap_hook   = ::mmap;
    __munmap_hook = ::munmap;
#endif
    FEXCore::Allocator::mmap = ::mmap;
    FEXCore::Allocator::munmap = ::munmap;

    // XXX: This is currently a leak.
    // We can't work around this yet until static initializers that allocate memory are completely removed from our codebase
    // Luckily we only remove this on process shutdown, so the kernel will do the cleanup for us
    Alloc64.release();
  }
#pragma GCC diagnostic pop

  FEX_DEFAULT_VISIBILITY size_t DetermineVASize() {
    static constexpr std::array<uintptr_t, 7> TLBSizes = {
      57,
      52,
      48,
      47,
      42,
      39,
      36,
    };

    for (auto Bits : TLBSizes) {
      uintptr_t Size = 1ULL << Bits;
      // Just try allocating
      // We can't actually determine VA size on ARM safely
      auto Find = [](uintptr_t Size) -> bool {
        for (int i = 0; i < 64; ++i) {
          // Try grabbing a some of the top pages of the range
          // x86 allocates some high pages in the top end
          void *Ptr = ::mmap(reinterpret_cast<void*>(Size - FHU::FEX_PAGE_SIZE * i), FHU::FEX_PAGE_SIZE, PROT_NONE, MAP_FIXED_NOREPLACE | MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
          if (Ptr != (void*)~0ULL) {
            ::munmap(Ptr, FHU::FEX_PAGE_SIZE);
            if (Ptr == (void*)(Size - FHU::FEX_PAGE_SIZE * i)) {
              return true;
            }
          }
        }
        return false;
      };

      if (Find(Size)) {
        return Bits;
      }
    }

    LOGMAN_MSG_A_FMT("Couldn't determine host VA size");
    FEX_UNREACHABLE;
  }

  PtrCache* StealMemoryRegion(uintptr_t Begin, uintptr_t End) {
    PtrCache *Cache{};
    uint64_t CacheSize{};
    uint64_t CurrentCacheOffset = 0;
    constexpr std::array<size_t, 10> ReservedVMARegionSizes = {{
      // Anything larger than 64GB fails out
      64ULL * 1024 * 1024 * 1024,   // 64GB
      32ULL * 1024 * 1024 * 1024,   // 32GB
      16ULL * 1024 * 1024 * 1024,   // 16GB
      4ULL * 1024 * 1024 * 1024,    // 4GB
      1ULL * 1024 * 1024 * 1024, // 1GB
      512ULL * 1024 * 1024,      // 512MB
      128ULL * 1024 * 1024,      // 128MB
      32ULL * 1024 * 1024,       // 32MB
      1ULL * 1024 * 1024,        // 1MB
      4096ULL                    // One page
    }};
    constexpr size_t AllocationSizeMaxIndex = ReservedVMARegionSizes.size() - 1;
    uint64_t CurrentSizeIndex = 0;

    int PROT_FLAGS = PROT_READ | PROT_WRITE;
    for (size_t MemoryOffset = Begin; MemoryOffset < End;) {
      size_t AllocationSize = ReservedVMARegionSizes[CurrentSizeIndex];
      size_t MemoryOffsetUpper = MemoryOffset + AllocationSize;

      // If we would go above the upper bound on size then try the next size
      if (MemoryOffsetUpper > End) {
        ++CurrentSizeIndex;
        continue;
      }

      void *Ptr = ::mmap(reinterpret_cast<void*>(MemoryOffset), AllocationSize, PROT_FLAGS, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE | MAP_FIXED_NOREPLACE, -1, 0);

      // If we managed to allocate and not get the address we want then unmap it
      // This happens with kernels older than 4.17
      if (reinterpret_cast<uintptr_t>(Ptr) + AllocationSize > End) {
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
        if (!Cache) {
          Cache = reinterpret_cast<PtrCache *>(Ptr);
          CacheSize = AllocationSize;
          PROT_FLAGS = PROT_NONE;
        }
        else {
          Cache[CurrentCacheOffset] = {
            .Ptr = static_cast<uint64_t>(reinterpret_cast<uint64_t>(Ptr)),
            .Size = static_cast<uint64_t>(AllocationSize)
          };
          ++CurrentCacheOffset;
        }

        CurrentSizeIndex = 0;
        MemoryOffset += AllocationSize;
        continue;
      }

      // Couldn't allocate at this size
      // Increase and continue
      ++CurrentSizeIndex;
    }

    Cache[CurrentCacheOffset] = {
      .Ptr = static_cast<uint64_t>(reinterpret_cast<uint64_t>(Cache)),
      .Size = CacheSize,
    };
    return Cache;
  }

  PtrCache* Steal48BitVA() {
    size_t Bits = FEXCore::Allocator::DetermineVASize();
    if (Bits < 48) {
      return nullptr;
    }

    uintptr_t Begin48BitVA = 0x0'8000'0000'0000ULL;
    uintptr_t End48BitVA   = 0x1'0000'0000'0000ULL;
    return StealMemoryRegion(Begin48BitVA, End48BitVA);
  }

  PtrCache* Steal32BitVA() {
    size_t Bits = FEXCore::Allocator::DetermineVASize();
    
    struct allocc{ uintptr_t base; size_t len; };
    std::vector<allocc> alloccs;

    size_t alloc_len = 1ULL << 51;
    while (alloc_len >= FHU::FEX_PAGE_SIZE) {
      for(;;) {
        // ~0 mmap hint to reserve the entire 52-bit address space. Needed because hugetlb & mali_kbase don't respect DEFAULT_MAP_WINDOW_64.
        // Also note the lack of MAP_FIXED: let the kernel find the VM gaps on its own.
        auto result = mmap((void*)~0ULL, alloc_len, PROT_NONE, MAP_ANONYMOUS | MAP_PRIVATE | MAP_NORESERVE, -1, 0);

        // No more free VM gaps of this size.
        if (result == MAP_FAILED) {
          if (errno != ENOMEM && errno != EPERM /* when len is too big ?*/) {
            printf("Error, errno: %d\n", errno);
            return nullptr;
          } else {
            break;
          }
        }

        if ((uintptr_t)result <= UINT32_MAX) {
          alloccs.push_back({(uintptr_t)result, alloc_len});
        }
      }

      // Halve the size and continue filling in gaps.
      alloc_len >>= 1;
    }

    for(auto &alc: alloccs) {
      auto top = alc.base + alc.len;
      if (top > UINT32_MAX)
        top = UINT32_MAX;

      munmap((void*)alc.base, top - alc.base);
    }
    return nullptr;
  }


  void ReclaimMemoryRegion(PtrCache* Regions) {
    if (Regions == nullptr) {
      return;
    }

    for (size_t i = 0;; ++i) {
      void *Ptr = reinterpret_cast<void*>(Regions[i].Ptr);
      size_t Size = Regions[i].Size;
      ::munmap(Ptr, Size);
      if (Ptr == Regions) {
        break;
      }
    }
  }
}
