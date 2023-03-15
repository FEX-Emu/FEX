#pragma once

#include "FEXCore/IR/RegisterAllocationData.h"
#include <FEXCore/Config/Config.h>
#include <FEXCore/fextl/queue.h>
#include <FEXCore/fextl/unordered_map.h>

#include <atomic>
#include <cstdint>
#include <functional>
#include <fstream>
#include <memory>
#include <map>
#include <shared_mutex>
#include <FEXCore/HLE/SourcecodeResolver.h>

namespace FEXCore::Core {
struct DebugData;
}
namespace FEXCore::Context {
  class ContextImpl;
}

namespace FEXCore::IR {
  class RegisterAllocationData;
  class IRListView;

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
  constexpr static uint32_t AOTIR_VERSION = 0x0000'00004;
  constexpr static uint64_t AOTIR_COOKIE = COOKIE_VERSION("FEXI", AOTIR_VERSION);

  struct AOTIRInlineEntry {
    uint64_t GuestHash;
    uint64_t GuestLength;

    /* RAData followed by IRData */
    uint8_t InlineData[0];

    IR::RegisterAllocationData *GetRAData();
    IR::IRListView *GetIRData();
  };

  struct AOTIRInlineIndexEntry {
    uint64_t GuestStart;
    uint64_t DataOffset;
  };

  struct AOTIRInlineIndex {
    uint64_t Count;
    uint64_t DataBase;
    AOTIRInlineIndexEntry Entries[0];

    AOTIRInlineEntry *Find(uint64_t GuestStart);
    AOTIRInlineEntry *GetInlineEntry(uint64_t DataOffset);
  };

  struct AOTIRCaptureCacheEntry {
    std::unique_ptr<std::ofstream> Stream;
    std::map<uint64_t, uint64_t> Index;

    void AppendAOTIRCaptureCache(uint64_t GuestRIP, uint64_t Start, uint64_t Length, uint64_t Hash, FEXCore::IR::IRListView *IRList, FEXCore::IR::RegisterAllocationData *RAData);
  };

  struct AOTIRCacheEntry {
    AOTIRInlineIndex *Array;
    void *FilePtr;
    size_t Size;
    std::unique_ptr<FEXCore::HLE::SourcecodeMap> SourcecodeMap;
    std::string FileId;
    std::string Filename;
    bool ContainsCode;
  };

  using AOTCacheType = fextl::unordered_map<std::string, FEXCore::IR::AOTIRCacheEntry>;

  class AOTIRCaptureCache final {
    public:

      AOTIRCaptureCache(FEXCore::Context::ContextImpl *ctx) : CTX {ctx} {}

      void FinalizeAOTIRCache();
      void AOTIRCaptureCacheWriteoutQueue_Flush();
      void AOTIRCaptureCacheWriteoutQueue_Append(const std::function<void()> &fn);
      void WriteFilesWithCode(std::function<void(const std::string& fileid, const std::string& filename)> Writer);

      struct PreGenerateIRFetchResult {
        FEXCore::IR::IRListView *IRList {};
        FEXCore::IR::RegisterAllocationData::UniquePtr RAData {};
        FEXCore::Core::DebugData *DebugData {};
        uint64_t StartAddr {};
        uint64_t Length {};
        bool GeneratedIR {};
      };
      [[nodiscard]] PreGenerateIRFetchResult PreGenerateIRFetch(uint64_t GuestRIP, FEXCore::IR::IRListView *IRList);

      bool PostCompileCode(FEXCore::Core::InternalThreadState *Thread,
        void* CodePtr,
        uint64_t GuestRIP,
        uint64_t StartAddr,
        uint64_t Length,
        FEXCore::IR::RegisterAllocationData::UniquePtr RAData,
        FEXCore::IR::IRListView *IRList,
        FEXCore::Core::DebugData *DebugData,
        bool GeneratedIR);

      AOTIRCacheEntry *LoadAOTIRCacheEntry(const std::string &filename);
      void UnloadAOTIRCacheEntry(AOTIRCacheEntry *Entry);

      // Callbacks
      void SetAOTIRLoader(std::function<int(const std::string&)> CacheReader) {
        AOTIRLoader = CacheReader;
      }

      void SetAOTIRWriter(std::function<std::unique_ptr<std::ofstream>(const std::string&)> CacheWriter) {
        AOTIRWriter = CacheWriter;
      }

      void SetAOTIRRenamer(std::function<void(const std::string&)> CacheRenamer) {
        AOTIRRenamer = CacheRenamer;
      }

    private:
      FEXCore::Context::ContextImpl *CTX;

      std::shared_mutex AOTIRCacheLock;
      std::shared_mutex AOTIRCaptureCacheWriteoutLock;
      std::atomic<bool> AOTIRCaptureCacheWriteoutFlusing;

      fextl::queue<std::function<void()>> AOTIRCaptureCacheWriteoutQueue;

      FEXCore::IR::AOTCacheType AOTIRCache;

      std::function<int(const std::string&)> AOTIRLoader;
      std::function<std::unique_ptr<std::ofstream>(const std::string&)> AOTIRWriter;
      std::function<void(const std::string&)> AOTIRRenamer;
      fextl::unordered_map<std::string, FEXCore::IR::AOTIRCaptureCacheEntry> AOTIRCaptureCacheMap;
  };
}
