#pragma once

#include <FEXCore/Utils/CompilerDefs.h>

#include <cstdint>
#include <functional>
#include <sys/types.h>

namespace FEXCore::Allocator {
  using MMAP_Hook = void*(*)(void*, size_t, int, int, int, off_t);
  using MUNMAP_Hook = int(*)(void*, size_t);

  using MALLOC_Hook = void*(*)(size_t);
  using REALLOC_Hook = void*(*)(void*, size_t);
  using FREE_Hook = void(*)(void*);

  FEX_DEFAULT_VISIBILITY extern MMAP_Hook mmap;
  FEX_DEFAULT_VISIBILITY extern MUNMAP_Hook munmap;
  FEX_DEFAULT_VISIBILITY extern MALLOC_Hook malloc;
  FEX_DEFAULT_VISIBILITY extern REALLOC_Hook realloc;
  FEX_DEFAULT_VISIBILITY extern FREE_Hook free;

  FEX_DEFAULT_VISIBILITY void SetupHooks();
  FEX_DEFAULT_VISIBILITY void ClearHooks();

  FEX_DEFAULT_VISIBILITY size_t DetermineVASize();
  // 48-bit VA handling
  struct PtrCache {
    uint64_t Ptr;
    uint64_t Size;
  };

  FEX_DEFAULT_VISIBILITY PtrCache* StealMemoryRegion(uintptr_t Begin, uintptr_t End);
  FEX_DEFAULT_VISIBILITY void ReclaimMemoryRegion(PtrCache* Regions);
  // When running a 64-bit executable on ARM then userspace guest only gets 47 bits of VA
  // This is a feature of x86-64 where the kernel gets a full 128TB of VA space
  // x86-64 canonical addresses with bit 48 set will sign extend the address (Ignoring LA57)
  // AArch64 canonical addresses are only up to bits 48/52 with the remainder being other things
  // Use this to reserve the top 128TB of VA so the guest never see it
  // Returns nullptr on host VA < 48bits
  FEX_DEFAULT_VISIBILITY PtrCache* Steal48BitVA();
}
