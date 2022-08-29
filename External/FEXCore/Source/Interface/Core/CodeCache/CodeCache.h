#pragma once

#include "FEXCore/Utils/LogManager.h"
#include <cstdint>

#include <atomic>
#include <functional>
#include <memory>
#include <optional>
#include <shared_mutex>
namespace FEXCore {

  constexpr auto COOKIE_VERSION = [](const char CookieText[4], uint32_t Version) {
    uint64_t Cookie = Version;
    Cookie <<= 32;

    // Make the cookie text be the lower bits
    Cookie |= CookieText[3];
    Cookie <<= 8;
    Cookie |= CookieText[2];
    Cookie <<= 8;
    Cookie |= CookieText[1];
    Cookie <<= 8;
    Cookie |= CookieText[0];
    return Cookie;

  };

  constexpr static uint64_t CHUNK_SIZE = 16 * 1024 * 1024;
  constexpr static uint64_t MAX_CHUNKS = 1024;

  constexpr static uint64_t INDEX_CHUNK_SIZE = 64 * 1024;

  struct GuestCodeRange;

  struct CacheEntry {
    uint64_t GuestHash;
    uint64_t GuestRangeCount;

    uint8_t InlineData[0];

    const GuestCodeRange *GetRangeData() const {
      return (const GuestCodeRange *)(InlineData);
    }

    GuestCodeRange *GetRangeData() {
      return (GuestCodeRange *)(InlineData);
    }

    template<typename RangesType>
    CacheEntry *fromValue(const RangesType &Ranges) {
      GuestRangeCount = Ranges.size();
      memcpy(InlineData, Ranges.data(), Ranges.size() * sizeof(typename RangesType::value_type));
      return this;
    }

    template <typename T>
    void toResult(T *v) const {
      v->RangeCount = GuestRangeCount;
      v->RangeData = GetRangeData();
    }
  };

  struct CacheIndexEntry {
    std::atomic<uint64_t> GuestStart;
    std::atomic<uint32_t> Left;
    std::atomic<uint32_t> Right;
    std::atomic<uint64_t> DataOffset;
  };

  struct CacheIndex {
    uint64_t Tag;
    std::atomic<uint64_t> FileSize;
    std::atomic<uint32_t> Count;
    CacheIndexEntry Entries[0];
  };

  struct CacheData {
    uint64_t Tag;
    uint64_t ChunkOffsets[MAX_CHUNKS];

    std::atomic<uint64_t> ChunksUsed;
    std::atomic<uint64_t> CurrentChunkFree;
    std::atomic<uint64_t> WritePointer;
  };

  class CodeCache {

    public:
      static std::unique_ptr<CodeCache> LoadFile(int IndexFD, int DataFD, const uint64_t IndexCookie, const uint64_t DataCookie);
      ~CodeCache();

      CacheEntry *Find(uint64_t OffsetRIP, uint64_t GuestRIP);

      template<typename CacheResult>
      inline std::optional<CacheResult> Find(uint64_t OffsetRIP, uint64_t GuestRIP) {
        auto CacheEntry = Find(OffsetRIP, GuestRIP);

        if (CacheEntry) {
          LogMan::Throw::AFmt(CacheEntry->GuestRangeCount > 0, "Invalid CacheEntry->GuestRangeCount");
          return CacheResult((const typename CacheResult::CacheEntryType *)CacheEntry);
        }

        return std::nullopt;
      }

      void Insert(uint64_t OffsetRIP, uint64_t GuestRIP, const GuestCodeRange *Ranges, size_t RangeCount, uint64_t InlineSize, const std::function<void(CacheEntry *CacheEntry)> &Fill);

      template<typename EntryType, typename RangesType, typename... Args>
      inline void Insert(uint64_t OffsetRIP, uint64_t GuestRIP, const RangesType& Ranges, Args... args) {
        Insert(
          OffsetRIP,
          GuestRIP,
          &Ranges[0],
          Ranges.size(),
          EntryType::GetInlineSize(args...),
          EntryType::GetFiller(args...)
        );
      }

    private:
      std::shared_mutex Mutex;

      CacheIndex *Index;
      std::atomic<uint64_t> IndexFileSize;

      int IndexFD;

      CacheData *Data;
      int DataFD;

      std::vector<uint8_t *> MappedDataChunks;

      CodeCache() {}

      void MapDataChunkUnsafe(uint64_t ChunkNum);
  };

  struct CacheResultBase {
    const GuestCodeRange *RangeData;
    uint64_t RangeCount;
  };
} // namespace FEXCore