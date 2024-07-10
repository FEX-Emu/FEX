// SPDX-License-Identifier: MIT
#pragma once

#include "Interface/IR/RegisterAllocationData.h"
#include "Interface/IR/IntrusiveIRList.h"

#include <FEXCore/Config/Config.h>
#include <FEXCore/fextl/map.h>
#include <FEXCore/fextl/string.h>
#include <FEXCore/fextl/queue.h>
#include <FEXCore/fextl/unordered_map.h>

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
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

  IR::RegisterAllocationData* GetRAData();
  IR::IRListView* GetIRData();
};

struct AOTIRInlineIndexEntry {
  uint64_t GuestStart;
  uint64_t DataOffset;
};

struct AOTIRInlineIndex {
  uint64_t Count;
  uint64_t DataBase;
  AOTIRInlineIndexEntry Entries[0];

  AOTIRInlineEntry* Find(uint64_t GuestStart);
  AOTIRInlineEntry* GetInlineEntry(uint64_t DataOffset);
};

struct AOTIRCaptureCacheEntry {
  fextl::unique_ptr<FEXCore::Context::AOTIRWriter> Stream;
  fextl::map<uint64_t, uint64_t> Index;

  void AppendAOTIRCaptureCache(uint64_t GuestRIP, uint64_t Start, uint64_t Length, uint64_t Hash, const FEXCore::IR::IRListView& IRList,
                               const FEXCore::IR::RegisterAllocationData* RAData);
};

struct AOTIRCacheEntry {
  AOTIRInlineIndex* Array;
  void* FilePtr;
  size_t Size;
  fextl::unique_ptr<FEXCore::HLE::SourcecodeMap> SourcecodeMap;
  fextl::string FileId;
  fextl::string Filename;
  bool ContainsCode;
};

using AOTCacheType = fextl::unordered_map<fextl::string, FEXCore::IR::AOTIRCacheEntry>;

class AOTIRCaptureCache final {
public:
  using WriteOutFn = std::function<void()>;

  AOTIRCaptureCache(FEXCore::Context::ContextImpl* ctx)
    : CTX {ctx} {}

  void FinalizeAOTIRCache();
  void AOTIRCaptureCacheWriteoutQueue_Flush();
  void AOTIRCaptureCacheWriteoutQueue_Append(const WriteOutFn& fn);
  void WriteFilesWithCode(const Context::AOTIRCodeFileWriterFn& Writer);

  struct PreGenerateIRFetchResult {
    fextl::unique_ptr<IRStorageBase> IR;
    FEXCore::Core::DebugData* DebugData {};
    uint64_t StartAddr {};
    uint64_t Length {};
  };
  [[nodiscard]]
  std::optional<PreGenerateIRFetchResult> PreGenerateIRFetch(FEXCore::Core::InternalThreadState* Thread, uint64_t GuestRIP);

  bool PostCompileCode(FEXCore::Core::InternalThreadState* Thread, void* CodePtr, uint64_t GuestRIP, uint64_t StartAddr, uint64_t Length,
                       fextl::unique_ptr<FEXCore::IR::IRStorageBase> IR, FEXCore::Core::DebugData* DebugData, bool GeneratedIR);

  AOTIRCacheEntry* LoadAOTIRCacheEntry(const fextl::string& filename);
  void UnloadAOTIRCacheEntry(AOTIRCacheEntry* Entry);

  // Callbacks
  void SetAOTIRLoader(Context::AOTIRLoaderCBFn CacheReader) {
    AOTIRLoader = std::move(CacheReader);
  }

  void SetAOTIRWriter(Context::AOTIRWriterCBFn CacheWriter) {
    AOTIRWriter = std::move(CacheWriter);
  }

  void SetAOTIRRenamer(Context::AOTIRRenamerCBFn CacheRenamer) {
    AOTIRRenamer = std::move(CacheRenamer);
  }

private:
  FEXCore::Context::ContextImpl* CTX;

  std::shared_mutex AOTIRCacheLock;
  std::shared_mutex AOTIRCaptureCacheWriteoutLock;
  std::atomic<bool> AOTIRCaptureCacheWriteoutFlusing;

  fextl::queue<WriteOutFn> AOTIRCaptureCacheWriteoutQueue;

  FEXCore::IR::AOTCacheType AOTIRCache;

  Context::AOTIRLoaderCBFn AOTIRLoader;
  Context::AOTIRWriterCBFn AOTIRWriter;
  Context::AOTIRRenamerCBFn AOTIRRenamer;
  fextl::unordered_map<fextl::string, FEXCore::IR::AOTIRCaptureCacheEntry> AOTIRCaptureCacheMap;
};
} // namespace FEXCore::IR
