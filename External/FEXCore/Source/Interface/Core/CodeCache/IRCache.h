#pragma once

#include "CodeCache.h"

#include "FEXCore/IR/IntrusiveIRList.h"
#include "FEXCore/IR/RegisterAllocationData.h"


namespace FEXCore {

  class RegisterAllocationData;
  class IRListView;
  
  constexpr static uint32_t IR_CACHE_VERSION = 0x0000'00006;
  constexpr static uint64_t IR_CACHE_INDEX_COOKIE = COOKIE_VERSION("FXAI", IR_CACHE_VERSION);
  constexpr static uint64_t IR_CACHE_DATA_COOKIE = COOKIE_VERSION("FXAD", IR_CACHE_VERSION);

  struct IRCacheEntry : CacheEntry {

    auto GetRAData() const {
      return (const IR::RegisterAllocationData *)&GetRangeData()[GuestRangeCount];
    }

    auto GetRAData() {
      return (IR::RegisterAllocationData *)&GetRangeData()[GuestRangeCount];
    }

    auto GetIRData() const {
      return (const IR::IRListView *)GetRAData()->After();
    }

    auto GetIRData() {
      return (IR::IRListView *)GetRAData()->After();
    }

    static uint64_t GetInlineSize(const IR::RegisterAllocationData *RAData, const IR::IRListView *IRList) {
      return RAData->Size() + IRList->GetInlineSize();
    }

    static auto GetFiller(const IR::RegisterAllocationData *RAData, const IR::IRListView *IRList) {
      return [RAData, IRList](auto *Entry) {
        auto IREntry = (IRCacheEntry*)Entry;
        RAData->Serialize((uint8_t*)IREntry->GetRAData());
        IRList->Serialize((uint8_t*)IREntry->GetIRData());
      };
    }
  };
  
  struct IRCacheResult {
    using CacheEntryType = IRCacheEntry;

    IRCacheResult(const IRCacheEntry *const Entry) {
      Entry->toResult(this);

      RAData = Entry->GetRAData();
      IRList = Entry->GetIRData();
    }
    const std::pair<uint64_t, uint64_t> *RangeData;
    uint64_t RangeCount;
    const FEXCore::IR::IRListView *IRList;
    const FEXCore::IR::RegisterAllocationData *RAData;
  };

  template <typename FDPairType>
  auto LoadIRCache(FDPairType CacheFDs) {
    return CodeCache::LoadFile(CacheFDs->IndexFD, CacheFDs->DataFD, IR_CACHE_INDEX_COOKIE, IR_CACHE_DATA_COOKIE);
  }

}