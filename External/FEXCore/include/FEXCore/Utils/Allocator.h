#pragma once

#include <FEXCore/Utils/CompilerDefs.h>

#include <cstdint>
#include <functional>
#include <sys/types.h>
#include <vector>

namespace FEXCore::Allocator {
  using MMAP_Hook = void*(*)(void*, size_t, int, int, int, off_t);
  using MUNMAP_Hook = int(*)(void*, size_t);

  using MALLOC_Hook = void *(*)(size_t);
  using CALLOC_Hook = void *(*)(size_t, size_t);
  using MEMALIGN_Hook = void *(*)(size_t, size_t);
  using VALLOC_Hook = void *(*)(size_t);
  using POSIX_MEMALIGN_Hook = int (*)(void**, size_t, size_t);
  using REALLOC_Hook = void *(*)(void*, size_t);
  using FREE_Hook = void(*)(void*);
  using MALLOC_USABLE_SIZE_Hook = size_t (*)(void*);
  using ALIGNED_ALLOC_Hook = void *(*)(size_t, size_t);

  FEX_DEFAULT_VISIBILITY extern MMAP_Hook mmap;
  FEX_DEFAULT_VISIBILITY extern MUNMAP_Hook munmap;
  FEX_DEFAULT_VISIBILITY extern MALLOC_Hook malloc;
  FEX_DEFAULT_VISIBILITY extern CALLOC_Hook calloc;
  FEX_DEFAULT_VISIBILITY extern MEMALIGN_Hook memalign;
  FEX_DEFAULT_VISIBILITY extern VALLOC_Hook valloc;
  FEX_DEFAULT_VISIBILITY extern POSIX_MEMALIGN_Hook posix_memalign;
  FEX_DEFAULT_VISIBILITY extern REALLOC_Hook realloc;
  FEX_DEFAULT_VISIBILITY extern FREE_Hook free;
  FEX_DEFAULT_VISIBILITY extern MALLOC_USABLE_SIZE_Hook malloc_usable_size;
  FEX_DEFAULT_VISIBILITY extern ALIGNED_ALLOC_Hook aligned_alloc;

  FEX_DEFAULT_VISIBILITY void SetupHooks();
  FEX_DEFAULT_VISIBILITY void ClearHooks();

  FEX_DEFAULT_VISIBILITY size_t DetermineVASize();

  struct MemoryRegion {
    void *Ptr;
    size_t Size;
  };

  FEX_DEFAULT_VISIBILITY std::vector<MemoryRegion> StealMemoryRegion(uintptr_t Begin, uintptr_t End);
  FEX_DEFAULT_VISIBILITY void ReclaimMemoryRegion(const std::vector<MemoryRegion> & Regions);
  // When running a 64-bit executable on ARM then userspace guest only gets 47 bits of VA
  // This is a feature of x86-64 where the kernel gets a full 128TB of VA space
  // x86-64 canonical addresses with bit 48 set will sign extend the address (Ignoring LA57)
  // AArch64 canonical addresses are only up to bits 48/52 with the remainder being other things
  // Use this to reserve the top 128TB of VA so the guest never see it
  // Returns nullptr on host VA < 48bits
  FEX_DEFAULT_VISIBILITY std::vector<MemoryRegion> Steal48BitVA();
}
