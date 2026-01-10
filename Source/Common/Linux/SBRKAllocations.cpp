// SPDX-License-Identifier: MIT
#include <FEXCore/fextl/fmt.h>
#include <FEXCore/Utils/CompilerDefs.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/MathUtils.h>
#include <FEXCore/Utils/TypeDefines.h>

#include <sys/mman.h>

namespace FEX::SBRKAllocations {
// This function disables glibc's ability to allocate memory through the `sbrk` interface.
// This is run early in the lifecycle of FEX in order to make sure no 64-bit pointers can make it to the guest 32-bit application.
//
// How this works is that this allocates a single page at the current sbrk pointer (aligned upward to page size). This makes it
// so that when the sbrk syscall is used to allocate more memory, it fails with an ENOMEM since it runs in to the allocated guard page.
//
// glibc notices the sbrk failure and falls back to regular mmap based allocations when this occurs. Ensuring that memory can still be allocated.
void* DisableSBRKAllocations() {
  void* INVALID_PTR = reinterpret_cast<void*>(~0ULL);
  // Get the starting sbrk pointer.
  void* StartingSBRK = sbrk(0);
  if (StartingSBRK == INVALID_PTR) {
    // If sbrk is already returning invalid pointers then nothing to do here.
    return INVALID_PTR;
  }

  // Now allocate the next page after the sbrk address to ensure it can't grow.
  // In most cases at the start of `main` this will already be page aligned, which means subsequent `sbrk`
  // calls won't allocate any memory through that.
  void* AlignedBRK = reinterpret_cast<void*>(FEXCore::AlignUp(reinterpret_cast<uintptr_t>(StartingSBRK), FEXCore::Utils::FEX_PAGE_SIZE));
  void* AfterBRK =
    ::mmap(AlignedBRK, FEXCore::Utils::FEX_PAGE_SIZE, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE | MAP_NORESERVE, -1, 0);
  if (AfterBRK == INVALID_PTR) {
    // Couldn't allocate the page after the aligned brk? This should never happen.
    // FEXCore::LogMan isn't configured yet so we just need to print the message.
    fextl::fmt::print("Couldn't allocate page after SBRK.\n");
    FEX_TRAP_EXECUTION;
    return INVALID_PTR;
  }

  // Now that the page after sbrk is allocated, FEX needs to consume the remaining sbrk space.
  // This will be anywhere from [0, 4096) bytes.
  // Start allocating from 1024 byte increments just to make any steps a bit faster.
  intptr_t IncrementAmount = 1024;
  for (; IncrementAmount != 0; IncrementAmount >>= 1) {
    while (sbrk(IncrementAmount) != INVALID_PTR)
      ;
  }
  return AlignedBRK;
}

void ReenableSBRKAllocations(void* Ptr) {
  const void* INVALID_PTR = reinterpret_cast<void*>(~0ULL);
  if (Ptr != INVALID_PTR) {
    munmap(Ptr, FEXCore::Utils::FEX_PAGE_SIZE);
  }
}
} // namespace FEX::SBRKAllocations
