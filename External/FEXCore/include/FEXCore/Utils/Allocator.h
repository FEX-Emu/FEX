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
