// SPDX-License-Identifier: MIT
#pragma once

#include <FEXCore/Utils/CompilerDefs.h>
#include <FEXCore/fextl/vector.h>

#include <cstdint>
#include <functional>
#include <sys/types.h>

namespace FEXCore::Allocator {
  FEX_DEFAULT_VISIBILITY void SetupHooks();
  FEX_DEFAULT_VISIBILITY void ClearHooks();

  FEX_DEFAULT_VISIBILITY size_t DetermineVASize();

#ifdef GLIBC_ALLOCATOR_FAULT
  // Glibc hooks should only fault once we are in main.
  // Required since glibc allocator hooking will catch things before FEX has control.
  FEX_DEFAULT_VISIBILITY void SetupFaultEvaluate();
  // Glibc hook faulting needs to be disabled when leaving main.
  // Required since glibc does some state teardown after main.
  FEX_DEFAULT_VISIBILITY void ClearFaultEvaluate();

  class FEX_DEFAULT_VISIBILITY YesIKnowImNotSupposedToUseTheGlibcAllocator final {
    public:
      FEX_DEFAULT_VISIBILITY YesIKnowImNotSupposedToUseTheGlibcAllocator();
      FEX_DEFAULT_VISIBILITY ~YesIKnowImNotSupposedToUseTheGlibcAllocator();
      FEX_DEFAULT_VISIBILITY static void HardDisable();
  };

  class FEX_DEFAULT_VISIBILITY GLIBCScopedFault final {
    public:
      GLIBCScopedFault() {
        FEXCore::Allocator::SetupFaultEvaluate();
      }
      ~GLIBCScopedFault() {
        FEXCore::Allocator::ClearFaultEvaluate();
      }
  };
#else
  FEX_DEFAULT_VISIBILITY inline void SetupFaultEvaluate() {}
  FEX_DEFAULT_VISIBILITY inline void ClearFaultEvaluate() {}

  class FEX_DEFAULT_VISIBILITY YesIKnowImNotSupposedToUseTheGlibcAllocator final {
    public:
    FEX_DEFAULT_VISIBILITY YesIKnowImNotSupposedToUseTheGlibcAllocator() {}
    FEX_DEFAULT_VISIBILITY ~YesIKnowImNotSupposedToUseTheGlibcAllocator() {}
    FEX_DEFAULT_VISIBILITY static inline void HardDisable() {}
  };

  class FEX_DEFAULT_VISIBILITY GLIBCScopedFault final {
    public:
      GLIBCScopedFault() {
        // nop
      }
      ~GLIBCScopedFault() {
        // nop
      }
  };
#endif

  // Disable allocations through glibc's sbrk allocation method.
  // Returns a pointer at the end of the sbrk region.
  extern "C" FEX_DEFAULT_VISIBILITY void *DisableSBRKAllocations();

  // Allow sbrk again. Pass in the pointer returned by `DisableSBRKAllocations`
  FEX_DEFAULT_VISIBILITY void ReenableSBRKAllocations(void* Ptr);

  struct MemoryRegion {
    void *Ptr;
    size_t Size;
  };

  FEX_DEFAULT_VISIBILITY fextl::vector<MemoryRegion> StealMemoryRegion(uintptr_t Begin, uintptr_t End);
  FEX_DEFAULT_VISIBILITY void ReclaimMemoryRegion(const fextl::vector<MemoryRegion> & Regions);
  // When running a 64-bit executable on ARM then userspace guest only gets 47 bits of VA
  // This is a feature of x86-64 where the kernel gets a full 128TB of VA space
  // x86-64 canonical addresses with bit 48 set will sign extend the address (Ignoring LA57)
  // AArch64 canonical addresses are only up to bits 48/52 with the remainder being other things
  // Use this to reserve the top 128TB of VA so the guest never see it
  // Returns nullptr on host VA < 48bits
  FEX_DEFAULT_VISIBILITY fextl::vector<MemoryRegion> Steal48BitVA();
}
