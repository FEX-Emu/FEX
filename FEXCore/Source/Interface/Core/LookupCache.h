// SPDX-License-Identifier: MIT
#pragma once
#include "Interface/Context/Context.h"
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/fextl/map.h>
#include <FEXCore/fextl/memory_resource.h>
#include <FEXCore/fextl/robin_map.h>
#include <FEXCore/fextl/vector.h>
#include <FEXCore/fextl/memory_resource.h>

#include <cstdint>
#include <functional>
#include <stddef.h>
#include <utility>
#include <mutex>
#ifdef _M_ARM_64EC
#include <winnt.h>
#endif

namespace FEXCore {

class LookupCache {
public:
  struct LookupCacheEntry {
    uintptr_t HostCode;
    uintptr_t GuestCode;
  };

  LookupCache(FEXCore::Context::ContextImpl *CTX);
  ~LookupCache();

  uintptr_t FindBlock(uint64_t Address) {
    // Try L1, no lock needed
    auto &L1Entry = reinterpret_cast<LookupCacheEntry*>(L1Pointer)[Address & L1_ENTRIES_MASK];
    if (L1Entry.GuestCode == Address) {
      return L1Entry.HostCode;
    }

    // L2 and L3 need to be locked
    std::lock_guard<std::recursive_mutex> lk(WriteLock);

    // Try L2
    const auto PageIndex = (Address & (VirtualMemSize -1)) >> 12;
    const auto PageOffset = Address & (0x0FFF);

    const auto Pointers = reinterpret_cast<uintptr_t*>(PagePointer);
    auto LocalPagePointer = Pointers[PageIndex];

    // Do we a page pointer for this address?
    if (LocalPagePointer) {
      // Find there pointer for the address in the blocks
      auto BlockPointers = reinterpret_cast<LookupCacheEntry*>(LocalPagePointer);

      if (BlockPointers[PageOffset].GuestCode == Address)
      {
        L1Entry.GuestCode = Address;
        L1Entry.HostCode = BlockPointers[PageOffset].HostCode;
        return L1Entry.HostCode;
      }
    }

    // Try L3
    auto HostCode = BlockList.find(Address);

    if (HostCode != BlockList.end()) {
      CacheBlockMapping(Address, HostCode->second);
      return HostCode->second;
    }

    // Failed to find
    return 0;
  }

#ifdef _M_ARM_64EC
  bool CheckPageEC(uint64_t Address) {
    if (!RtlIsEcCode(Address)) {
        return false;
    }

    std::lock_guard<std::recursive_mutex> lk(WriteLock);

    // Mark L2 entry for this page as EC by setting the LSB, this can then be
    // checked by the dispatcher to see if it needs to perform a call/return to
    // EC code.
    const auto PageIndex = (Address & (VirtualMemSize -1)) >> 12;
    const auto Pointers = reinterpret_cast<uintptr_t*>(PagePointer);
    Pointers[PageIndex] |= 1;
    return true;
  }
#endif

  fextl::map<uint64_t, fextl::vector<uint64_t>> CodePages;

  // Appends Block {Address} to CodePages [Start, Start + Length)
  // Returns true if new pages are marked as containing code
  bool AddBlockExecutableRange(uint64_t Address, uint64_t Start, uint64_t Length) {
    std::lock_guard<std::recursive_mutex> lk(WriteLock);

    bool rv = false;

    for (auto CurrentPage = Start >> 12, EndPage = (Start + Length -1) >> 12; CurrentPage <= EndPage; CurrentPage++) {
      auto &CodePage = CodePages[CurrentPage];
      rv |= CodePage.size() == 0;
      CodePage.push_back(Address);
    }

    return rv;
  }

  // Adds to Guest -> Host code mapping
  void AddBlockMapping(uint64_t Address, void *HostCode) {
    std::lock_guard<std::recursive_mutex> lk(WriteLock);

    [[maybe_unused]] auto Inserted = BlockList.emplace(Address, (uintptr_t)HostCode).second;
    LOGMAN_THROW_AA_FMT(Inserted, "Duplicate block mapping added");

    // There is no need to update L1 or L2, they will get updated on first lookup
    // However, adding to L1 here increases performance
    auto &L1Entry = reinterpret_cast<LookupCacheEntry*>(L1Pointer)[Address & L1_ENTRIES_MASK];
    L1Entry.GuestCode = Address;
    L1Entry.HostCode = (uintptr_t)HostCode;
  }

  void Erase(FEXCore::Core::CpuStateFrame *Frame, uint64_t Address) {

    std::lock_guard<std::recursive_mutex> lk(WriteLock);

    // Sever any links to this block
    auto lower = BlockLinks->lower_bound({Address, nullptr});
    auto upper = BlockLinks->upper_bound({Address, reinterpret_cast<FEXCore::Context::ExitFunctionLinkData *>(UINTPTR_MAX)});
    for (auto it = lower; it != upper; it = BlockLinks->erase(it)) {
      it->second(Frame, it->first.HostLink);
    }

    // Remove from BlockList
    BlockList.erase(Address);

    // Do L1
    auto &L1Entry = reinterpret_cast<LookupCacheEntry*>(L1Pointer)[Address & L1_ENTRIES_MASK];
    if (L1Entry.GuestCode == Address) {
      L1Entry.GuestCode = 0;
      // Leave L1Entry.HostCode as is, so that concurrent lookups won't read a null pointer
      // This is a soft guarantee for cross thread invalidation, as atomics are not used
      // and it hasn't been thoroughly tested
    }

    // Do full map
    Address = Address & (VirtualMemSize -1);
    uint64_t PageOffset = Address & (0x0FFF);
    Address >>= 12;

    uintptr_t *Pointers = reinterpret_cast<uintptr_t*>(PagePointer);
    uint64_t LocalPagePointer = Pointers[Address];
    if (!LocalPagePointer) {
      // Page for this code didn't even exist, nothing to do
      return;
    }

    // Page exists, just set the offset to zero
    auto BlockPointers = reinterpret_cast<LookupCacheEntry*>(LocalPagePointer);
    BlockPointers[PageOffset].GuestCode = 0;
    BlockPointers[PageOffset].HostCode = 0;
  }

  void AddBlockLink(uint64_t GuestDestination, FEXCore::Context::ExitFunctionLinkData * HostLink, const FEXCore::Context::BlockDelinkerFunc &delinker) {
    std::lock_guard<std::recursive_mutex> lk(WriteLock);

    BlockLinks->insert({{GuestDestination, HostLink}, delinker});
  }

  void ClearCache();
  void ClearL2Cache();

  uintptr_t GetL1Pointer() const { return L1Pointer; }
  uintptr_t GetPagePointer() const { return PagePointer; }
  uintptr_t GetVirtualMemorySize() const { return VirtualMemSize; }

  constexpr static size_t L1_ENTRIES = 1 * 1024 * 1024; // Must be a power of 2
  constexpr static size_t L1_ENTRIES_MASK = L1_ENTRIES - 1;

  // This needs to be taken before reads or writes to L2, L3, CodePages,
  // and before writes to L1. Concurrent access from a thread that this LookupCache doesn't belong to
  // may only happen during cross thread invalidation (::Erase).
  // All other operations must be done from the owning thread.
  // Some care is taken so that L1 lookups can be done without locks, and even tearing is unlikely to lead to a crash.
  // This approach has not been fully vetted yet.
  // Also note that L1 lookups might be inlined in the JIT Dispatcher and/or block ends.
  std::recursive_mutex WriteLock;

private:
  void CacheBlockMapping(uint64_t Address, uintptr_t HostCode) {
    // Do L1
    auto &L1Entry = reinterpret_cast<LookupCacheEntry*>(L1Pointer)[Address & L1_ENTRIES_MASK];
    L1Entry.GuestCode = Address;
    L1Entry.HostCode = HostCode;

    // Do ful map
    auto FullAddress = Address;
    Address = Address & (VirtualMemSize -1);

    uint64_t PageOffset = Address & (0x0FFF);
    Address >>= 12;
    uintptr_t *Pointers = reinterpret_cast<uintptr_t*>(PagePointer);
    uint64_t LocalPagePointer = Pointers[Address];
    if (!LocalPagePointer) {
      // We don't have a page pointer for this address
      // Allocate one now if we can
      uintptr_t NewPageBacking = AllocateBackingForPage();
      if (!NewPageBacking) {
        // Couldn't allocate, clear L2 and retry
        ClearL2Cache();
        CacheBlockMapping(Address, HostCode);
        return;
      }
      Pointers[Address] = NewPageBacking;
      LocalPagePointer = NewPageBacking;
    }

    // Add the new pointer to the page block
    auto BlockPointers = reinterpret_cast<LookupCacheEntry*>(LocalPagePointer);

    // This silently replaces existing mappings
    BlockPointers[PageOffset].GuestCode = FullAddress;
    BlockPointers[PageOffset].HostCode = HostCode;
  }

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

  uintptr_t PagePointer;
  uintptr_t PageMemory;
  uintptr_t L1Pointer;

  struct BlockLinkTag {
    uint64_t GuestDestination;
    FEXCore::Context::ExitFunctionLinkData *HostLink;

    bool operator <(const BlockLinkTag& other) const {
      if (GuestDestination < other.GuestDestination)
        return true;
      else if (GuestDestination == other.GuestDestination)
        return HostLink < other.HostLink;
      else
        return false;
    }
  };

  // Use a monotonic buffer resource to allocate both the std::pmr::map and its members.
  // This allows us to quickly clear the block link map by clearing the monotonic allocator.
  // If we had allocated the block link map without the MBR, then clearing the map would require slowly
  // walking each block member and destructing objects.
  //
  // This makes `BlockLinks` look like a raw pointer that could memory leak, but since it is backed by the MBR, it won't.
  std::pmr::monotonic_buffer_resource BlockLinks_mbr;
  using BlockLinksMapType = std::pmr::map<BlockLinkTag, FEXCore::Context::BlockDelinkerFunc>;
  fextl::unique_ptr<std::pmr::polymorphic_allocator<std::byte>> BlockLinks_pma;
  BlockLinksMapType *BlockLinks;

  fextl::robin_map<uint64_t, uint64_t> BlockList;

  size_t TotalCacheSize;

  constexpr static size_t CODE_SIZE = 128 * 1024 * 1024;
  constexpr static size_t SIZE_PER_PAGE = 4096 * sizeof(LookupCacheEntry);
  constexpr static size_t L1_SIZE = L1_ENTRIES * sizeof(LookupCacheEntry);

  size_t AllocateOffset {};

  FEXCore::Context::ContextImpl *ctx;
  uint64_t VirtualMemSize{};
};
}
