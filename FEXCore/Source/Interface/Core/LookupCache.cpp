// SPDX-License-Identifier: MIT
/*
$info$
tags: glue|block-database
desc: Stores information about blocks, and provides C++ implementations to lookup the blocks
$end_info$
*/

#include <FEXCore/Utils/Allocator.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/HLE/SyscallHandler.h>

#include "Interface/Context/Context.h"
#include "Interface/Core/LookupCache.h"

namespace FEXCore {
GuestToHostMap::GuestToHostMap()
  : BlockLinks_mbr {"FEXMem_BlockLinks"} {
  BlockLinks_pma = fextl::make_unique<std::pmr::polymorphic_allocator<std::byte>>(&BlockLinks_mbr);
  // Setup our PMR map.
  BlockLinks = BlockLinks_pma->new_object<BlockLinksMapType>();
}

LookupCache::LookupCache(FEXCore::Context::ContextImpl* CTX)
  : ctx {CTX} {

  TotalCacheSize = ctx->Config.VirtualMemSize / 4096 * 8 + CODE_SIZE + L1_SIZE;

  // Block cache ends up looking like this
  // PageMemoryMap[VirtualMemoryRegion >> 12]
  //       |
  //       v
  // PageMemory[Memory & (VIRTUAL_PAGE_SIZE - 1)]
  //       |
  //       v
  // Pointer to Code
  //
  // Allocate a region of memory that we can use to back our block pointers
  // We need one pointer per page of virtual memory
  // At 64GB of virtual memory this will allocate 128MB of virtual memory space
  PagePointer = reinterpret_cast<uintptr_t>(FEXCore::Allocator::VirtualAlloc(TotalCacheSize, false, false));
  FEXCore::Allocator::VirtualName("FEXMem_Lookup", reinterpret_cast<void*>(PagePointer), ctx->Config.VirtualMemSize / 4096 * 8 + CODE_SIZE);
  CTX->SyscallHandler->MarkOvercommitRange(PagePointer, TotalCacheSize);

  // Allocate our memory backing our pages
  // We need 32KB per guest page (One pointer per byte)
  // XXX: We can drop down to 16KB if we store 4byte offsets from the code base
  // We currently limit to 128MB of real memory for caching for the total cache size.
  // Can end up being inefficient if we compile a small number of blocks per page
  PageMemory = PagePointer + ctx->Config.VirtualMemSize / 4096 * 8;
  LOGMAN_THROW_A_FMT(PageMemory != -1ULL, "Failed to allocate page memory");

  // L1 Cache
  L1Pointer = PageMemory + CODE_SIZE;
  FEXCore::Allocator::VirtualName("FEXMem_Lookup_L1", reinterpret_cast<void*>(L1Pointer), L1_SIZE);

  LOGMAN_THROW_A_FMT(L1Pointer != -1ULL, "Failed to allocate L1Pointer");

  VirtualMemSize = ctx->Config.VirtualMemSize;
}

LookupCache::~LookupCache() {
  FEXCore::Allocator::VirtualFree(reinterpret_cast<void*>(PagePointer), TotalCacheSize);
  ctx->SyscallHandler->UnmarkOvercommitRange(PagePointer, TotalCacheSize);

  // No need to free BlockLinks map.
  // These will get freed when their memory allocators are deallocated.
}

void LookupCache::ClearL2Cache() {
  auto lk = Shared->AcquireLock();
  // Clear out the page memory
  // PagePointer and PageMemory are sequential with each other. Clear both at once.
  FEXCore::Allocator::VirtualDontNeed(reinterpret_cast<void*>(PagePointer), ctx->Config.VirtualMemSize / 4096 * 8 + CODE_SIZE, false);
  AllocateOffset = 0;
}

void LookupCache::ClearThreadLocalCaches() {
  auto lk = Shared->AcquireLock();

  // Clear L1 and L2 by clearing the full cache.
  FEXCore::Allocator::VirtualDontNeed(reinterpret_cast<void*>(PagePointer), TotalCacheSize, false);
}

void LookupCache::ClearCache() {
  auto lk = Shared->AcquireLock();

  // Clear L1 and L2 by clearing the full cache.
  FEXCore::Allocator::VirtualDontNeed(reinterpret_cast<void*>(PagePointer), TotalCacheSize, false);

  Shared->ClearCache(lk);
}

void GuestToHostMap::ClearCache(const LockToken&) {
  // Allocate a new pointer from the BlockLinks pma again.
  BlockLinks = BlockLinks_pma->new_object<BlockLinksMapType>();
  // All code is gone, clear the block list
  BlockList.clear();
}

} // namespace FEXCore
