// SPDX-License-Identifier: MIT
#pragma once
#include "FEXCore/Core/CodeCache.h"
#include "FEXCore/Core/Context.h"
#include "Interface/Core/JIT/Relocations.h"
#include "Interface/Core/Frontend.h"
#include "Interface/Core/CPUBackend.h"
#include "FEXCore/Config/Config.h"
#include "FEXCore/Utils/File.h"
#include "FEXCore/fextl/memory.h"
#include <FEXCore/fextl/string.h>
#include <FEXCore/fextl/unordered_set.h>
#include <FEXCore/fextl/robin_map.h>
#include <FEXCore/fextl/vector.h>
#include <stdint.h>
#include <mutex>
#include <optional>
#include <span>
#include <xxhash.h>

namespace FEXCore {

namespace Context {
  class ContextImpl;
}

namespace DiskCache {

  namespace MesaFOZ {

#define FOSSILIZE_BLOB_HASH_LENGTH 40 /* SHA1 hexadecimal string length */

    struct __attribute__((packed)) foz_payload_key {
      uint8_t bytes[FOSSILIZE_BLOB_HASH_LENGTH];
    };

    struct __attribute__((packed)) foz_payload_header {
      uint32_t payload_size;
      uint32_t format;
      uint32_t crc;
      uint32_t uncompressed_size;
    };

  } // namespace MesaFOZ

  class IndexedDB;

  struct IndexEntry {
    IndexedDB* DB;
    uint64_t Offset;
    uint32_t Size;
  };

  struct __attribute__((packed)) BlobFixedHeader {
    uint32_t GuestSize;
    uint32_t HostSize;
    uint32_t EntryPointCount;
    uint32_t SmallRelocCount;
    uint32_t ThunkRelocCount;
    uint32_t TouchedGuestPagesCount;
    XXH128_hash_t GuestHash;
  };

  struct __attribute__((packed)) BlobEntryPoint {
    uint64_t GuestRIP; // todo those have been made relative since i wrote this, can we get away with less size here?
    uint32_t HostOffset;
  };

  // packed struct for types 0, 2 and 3. type 1 is bigger and separate below
  struct __attribute__((packed)) BlobSmallRelocation {
    uint32_t Offset;
    uint8_t Type;
    union {
      struct __attribute__((packed)) {
        uint32_t Symbol;
      } Named;
      struct __attribute__((packed)) {
        uint64_t GuestRIP;
      } RIPLiteral;
      struct __attribute__((packed)) {
        uint8_t RegisterIndex;
        uint64_t GuestRIP;
      } RIPMove;
    };
  };

  // type 1, implicit
  struct __attribute__((packed)) BlobThunkRelocation {
    uint32_t Offset;
    uint8_t RegisterIndex;
    uint8_t SymbolHash[32]; // sha256sum in the real RelocNamedThunkMove
  };

  struct CodeHitData {
    fextl::vector<uint8_t> Blob;
    std::span<uint8_t> HostCode;
    std::span<const BlobEntryPoint> EntryPoints;
    fextl::vector<FEXCore::CPU::Relocation> Relocations;
    fextl::vector<uint64_t> GuestPages;

    // the spans above point to memory owned by the Blob vec, so it's important this can't be copied
    CodeHitData() = default;
    CodeHitData(CodeHitData&&) = default;
    CodeHitData& operator=(CodeHitData&&) = default;
    CodeHitData(const CodeHitData&) = delete;
    CodeHitData& operator=(const CodeHitData&) = delete;
  };

  using Index = fextl::robin_map<uint64_t, IndexEntry>;

  class FOZFile {
  public:
    bool Open(const fextl::string& CacheFileName, bool ReadOnly);
    bool Lock(uint32_t TimeoutMS) {
      if (!FD) {
        return false;
      }
      return FD->Lock(TimeoutMS);
    }
    bool Unlock() {
      if (!FD) {
        return false;
      }
      return FD->Unlock();
    }
    bool ReadNextBlob(MesaFOZ::foz_payload_key& OutKey, MesaFOZ::foz_payload_header& OutHeader, fextl::vector<uint8_t>& OutBlob);
    bool ReadBlob(uint64_t Offset, std::span<uint8_t> OutBlob);
    bool WriteBlob(const MesaFOZ::foz_payload_key& Key, std::span<const std::span<const uint8_t>> BlobChunks, uint64_t& OutBlobOffset);

  private:
    static constexpr uint32_t OPEN_LOCK_TIMEOUT_MS = 100;

    fextl::string FileName;
    fextl::unique_ptr<File::File> FD;
    bool ReadOnly = false;
  };

  class IndexedDB {
  public:
    bool Open(const fextl::string& CacheDBName, bool ReadOnly);
    void PopulateIndex(Index& CacheIndex);
    bool ReadCacheBlob(uint64_t Offset, std::span<uint8_t> OutBlob);
    bool StoreCacheBlob(const MesaFOZ::foz_payload_key& Key, std::span<const std::span<const uint8_t>> BlobChunks, Index& CacheIndex);

  private:
    // give up after 2ms of trying to store - when we have an async thread we can increase this
    static constexpr uint32_t STORE_LOCK_TIMEOUT_MS = 2;

    FOZFile CacheFOZ;
    FOZFile IndexFOZ;
    bool ReadOnly = false;
  };

  class DiskCache {
  public:
    void Init(FEXCore::Context::ContextImpl* CTX);

    std::optional<CodeHitData> Lookup(Core::InternalThreadState* Thread, const ExecutableFileSectionInfo& Region, uint64_t GuestRIP);
    bool Store(Core::InternalThreadState* Thread, const ExecutableFileSectionInfo& Region, uint64_t GuestRIP,
               std::span<const uint8_t> GuestCode, const CPU::CPUBackend::CompiledCode& CompiledCode,
               std::span<const FEXCore::CPU::Relocation> Relocations, const Frontend::Decoder::DecodedBlockInformation* DecodedBlockInfo);

    bool IsWritingDiskCache() const {
      return (bool)RWCacheDB;
    }
    bool IsReadingDiskCache() const {
      return !ROCacheDBs.empty() || RWCacheDB != nullptr;
    }

  private:
    bool OpenCacheDB(const fextl::string& CacheDBName, bool ReadOnly);

    FEXCore::Context::ContextImpl* CTX;
    fextl::vector<fextl::unique_ptr<IndexedDB>> ROCacheDBs;
    fextl::unique_ptr<IndexedDB> RWCacheDB;
    Index Index;
    std::mutex Lock;

    FEX_CONFIG_OPT(EnableDiskCache, DISKCACHE);
    FEX_CONFIG_OPT(RelocationFilter, DISKCACHERELOCATIONFILTER);
    FEX_CONFIG_OPT(BasePathOverride, DISKCACHEPATH);
    FEX_CONFIG_OPT(RODBNames, DISKCACHERODBNAMES);
  };

} // namespace DiskCache

} // namespace FEXCore