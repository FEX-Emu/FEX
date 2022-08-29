#pragma once

#include <cstdint>
#include <string.h>
#include "CodeCache.h"

#include "Interface/Context/Context.h"
#include <FEXCore/Core/CPURelocations.h>
#include <string>

namespace FEXCore {  
  constexpr static uint32_t OBJ_CACHE_VERSION = 0x0000'00002;
  constexpr static uint64_t OBJ_CACHE_INDEX_COOKIE = COOKIE_VERSION("FXOI", OBJ_CACHE_VERSION);
  constexpr static uint64_t OBJ_CACHE_DATA_COOKIE = COOKIE_VERSION("FXOD", OBJ_CACHE_VERSION);

    //const CodeSerializationData *Data;
    //const char *HostCode;
    //uint64_t NumRelocations;
    //const char *Relocations;

  struct ObjCacheFragment {
    uint64_t Bytes;
    uint8_t Code[0];
  };

  struct ObjCacheRelocations {
    size_t Bytes;
    uint8_t Relocations[0];
  };

  struct ObjCacheEntry : CacheEntry {

    auto GetObjCacheFragment() const {
      return (const ObjCacheFragment *)&GetRangeData()[GuestRangeCount];
    }

    auto GetObjCacheFragment() {
      return (ObjCacheFragment *)&GetRangeData()[GuestRangeCount];
    }

    auto GetObjCacheRelocations() const {
      auto v = GetObjCacheFragment();
      
      return (const ObjCacheRelocations *)&v->Code[v->Bytes];
    }

    auto GetObjCacheRelocations() {
      auto v = GetObjCacheFragment();
      
      return (ObjCacheRelocations *)&v->Code[v->Bytes];
    }

    static uint64_t GetInlineSize(const void *HostCode, const size_t HostCodeBytes, const ObjCacheRelocations *Relocations) {
      return HostCodeBytes + Relocations->Bytes + sizeof(*Relocations);
    }

    static auto GetFiller(const void *HostCode, const size_t HostCodeBytes, const ObjCacheRelocations *Relocations) {
      return [HostCode, HostCodeBytes, Relocations](auto *Entry) {
        auto ObjEntry = (ObjCacheEntry*)Entry;

        ObjEntry->GetObjCacheFragment()->Bytes = HostCodeBytes;
        memcpy(ObjEntry->GetObjCacheFragment()->Code, HostCode, HostCodeBytes);

        memcpy(ObjEntry->GetObjCacheRelocations(), Relocations,  sizeof(*Relocations) + Relocations->Bytes);
      };
    }
  };
  
  struct ObjCacheResult: CacheResultBase {
    using CacheEntryType = ObjCacheEntry;

    ObjCacheResult(const ObjCacheEntry *const Entry) {
      Entry->toResult(this);

      HostCode = Entry->GetObjCacheFragment();
      RelocationData = Entry->GetObjCacheRelocations();
    }
    const ObjCacheFragment *HostCode;
    const ObjCacheRelocations *RelocationData;
  };

  template <typename FDPairType>
  auto LoadObjCache(FDPairType CacheFDs) {
    return CodeCache::LoadFile(CacheFDs->IndexFD, CacheFDs->DataFD, OBJ_CACHE_INDEX_COOKIE, OBJ_CACHE_DATA_COOKIE);
  }

}