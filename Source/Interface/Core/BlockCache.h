#pragma once
#include <FEXCore/Core/Context.h>
#include "LogManager.h"

namespace FEXCore {
class BlockCache {
public:

  BlockCache(FEXCore::Context::Context *CTX);
  ~BlockCache();

  using BlockCacheIter = uintptr_t;
  uintptr_t End() { return 0; }

  uintptr_t FindBlock(uint64_t Address) {
    return FindCodePointerForAddress(Address);
  }

  void Erase(uint64_t Address) {
    uint64_t PageOffset = Address & (0x0FFF);
    Address >>= 12;

    uintptr_t *Pointers = reinterpret_cast<uintptr_t*>(PagePointer);
    uint64_t LocalPagePointer = Pointers[Address];
    if (!LocalPagePointer) {
      // Page for this code didn't even exist, nothing to do
      return;
    }

    // Page exists, just set the offset to zero
    uintptr_t *BlockPointers = reinterpret_cast<uintptr_t*>(LocalPagePointer);
    BlockPointers[PageOffset] = 0;
  }

  uintptr_t AddBlockMapping(uint64_t Address, void *Ptr) {
    uint64_t PageOffset = Address & (0x0FFF);
    Address >>= 12;
    uintptr_t *Pointers = reinterpret_cast<uintptr_t*>(PagePointer);
    uint64_t LocalPagePointer = Pointers[Address];
    if (!LocalPagePointer) {
      // We don't have a page pointer for this address
      // Allocate one now if we can
      uintptr_t NewPageBacking = AllocateBackingForPage();
      if (!NewPageBacking) {
        // Couldn't allocate, return so the frontend can recover from this
        return 0;
      }
      Pointers[Address] = NewPageBacking;
      LocalPagePointer = NewPageBacking;
    }

    // Add the new pointer to the page block
    uintptr_t *BlockPointers = reinterpret_cast<uintptr_t*>(LocalPagePointer);
    uintptr_t CastPtr = reinterpret_cast<uintptr_t>(Ptr);
    BlockPointers[PageOffset] = CastPtr;

    return CastPtr;
  }

  void ClearCache();

  void HintUsedRange(uint64_t Address, uint64_t Size);

  uintptr_t GetPagePointer() { return PagePointer; }

private:
  uintptr_t AllocateBackingForPage() {
    uintptr_t NewBase = AllocateOffset;
    uintptr_t NewEnd = AllocateOffset + SIZE_PER_PAGE;

    if (NewEnd >= CODE_SIZE) {
      // We ran out of block backing space. Need to clear the block cache and tell the JIT cores to clear their caches as well
      // Tell whatever is calling this that it needs to do it.
      return 0;
    }

    AllocateOffset = NewEnd;
    return PageMemory + NewBase;
  }

  uintptr_t FindCodePointerForAddress(uint64_t Address) {
    uint64_t PageOffset = Address & (0x0FFF);
    Address >>= 12;
    uintptr_t *Pointers = reinterpret_cast<uintptr_t*>(PagePointer);
    uint64_t LocalPagePointer = Pointers[Address];
    if (!LocalPagePointer) {
      // We don't have a page pointer for this address
      return 0;
    }

    // Find there pointer for the address in the blocks
    uintptr_t *BlockPointers = reinterpret_cast<uintptr_t*>(LocalPagePointer);
    return BlockPointers[PageOffset];
  }

  uintptr_t PagePointer;
  uintptr_t PageMemory;

  constexpr static size_t CODE_SIZE = 128 * 1024 * 1024;
  constexpr static size_t SIZE_PER_PAGE = 4096 * 8;
  size_t AllocateOffset {};

  FEXCore::Context::Context *ctx;

};
}
