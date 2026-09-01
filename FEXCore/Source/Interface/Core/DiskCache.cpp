// SPDX-License-Identifier: MIT

#define XXH_STATIC_LINKING_ONLY

#include "FEXCore/Config/Config.h"
#include "FEXCore/fextl/string.h"
#include "FEXHeaderUtils/Filesystem.h"
#include "FEXCore/Core/DiskCache.h"
#include "FEXCore/Core/DiskCacheFileMapper.h"
#include "FEXCore/Utils/LogManager.h"
#include "Interface/Context/Context.h"
#include "FEXCore/HLE/SyscallHandler.h"
#include "FEXCore/Utils/File.h"
#include "FEXCore/fextl/memory.h"
#include <cstdint>
#include <cstring>
#include <charconv>

namespace FEXCore {

namespace DiskCache {

  namespace MesaFOZ {

    enum { FOSSILIZE_COMPRESSION_NONE = 1, FOSSILIZE_COMPRESSION_DEFLATE = 2 };

    enum { FOSSILIZE_FORMAT_VERSION = 6, FOSSILIZE_FORMAT_MIN_COMPAT_VERSION = 5 };

#define FOZ_REF_MAGIC_SIZE 16

    static const uint8_t stream_reference_magic_and_version[FOZ_REF_MAGIC_SIZE] = {
      0x81, 'F', 'O', 'S', 'S', 'I', 'L', 'I', 'Z', 'E', 'D', 'B', 0, 0, 0, FOSSILIZE_FORMAT_VERSION, /* 4 bytes to use for versioning. */
    };

    struct __attribute__((packed)) mesa_index_db_file_entry {
      uint64_t hash;
      uint32_t size;
      uint64_t last_access_time;
      uint64_t cache_db_file_offset;
    };

  } // namespace MesaFOZ

  static FileMapperFunc FileMapper = nullptr;

  bool FOZFile::Open(const fextl::string& FOZFileName, bool ReadOnly) {
    FileName = FOZFileName;
    this->ReadOnly = ReadOnly;

    File::FileModes Modes = File::FileModes::READ;
    if (!ReadOnly) {
      Modes = Modes | File::FileModes::WRITE | File::FileModes::CREATE;
    }
    FD = fextl::make_unique<File::File>(FileName.c_str(), Modes, false);
    if (!FD->IsValid()) {
      FD.reset();
      return false;
    }

    bool Valid = false;
    bool TookLock = false;
    ssize_t Size = FD->Size();

    if (Size < FOZ_REF_MAGIC_SIZE && !ReadOnly) {
      if (!FD->Lock(OPEN_LOCK_TIMEOUT_MS)) {
        FD.reset();
        return false;
      }
      TookLock = true;
      // check size again in case someone else made it while we waited
      Size = FD->Size();
    }

    if (Size == 0 && !ReadOnly) {
      Valid = FD->PWrite(MesaFOZ::stream_reference_magic_and_version, FOZ_REF_MAGIC_SIZE, 0) == FOZ_REF_MAGIC_SIZE;
    } else {
      uint8_t magic[FOZ_REF_MAGIC_SIZE];
      if (FD->PRead(magic, FOZ_REF_MAGIC_SIZE, 0) == FOZ_REF_MAGIC_SIZE &&
          memcmp(magic, MesaFOZ::stream_reference_magic_and_version, FOZ_REF_MAGIC_SIZE - 1) == 0) {
        int version = magic[FOZ_REF_MAGIC_SIZE - 1];
        Valid = version <= MesaFOZ::FOSSILIZE_FORMAT_VERSION && version >= MesaFOZ::FOSSILIZE_FORMAT_MIN_COMPAT_VERSION;
      }
    }

    if (TookLock) {
      FD->Unlock();
    }

    if (!Valid) {
      FD.reset();
    }
    return Valid;
  }

  ssize_t FOZFile::Size() {
    return FD ? FD->Size() : -1;
  }

  bool FOZFile::ReadAll(fextl::vector<uint8_t>& Out) {
    ssize_t FileSize = Size();
    if (FileSize < FOZ_REF_MAGIC_SIZE) {
      return false;
    }
    Out.resize((size_t)FileSize - FOZ_REF_MAGIC_SIZE);
    return FD->PRead(Out.data(), Out.size(), FOZ_REF_MAGIC_SIZE) == (ssize_t)Out.size();
  }

  bool FOZFile::ReadBlob(uint64_t Offset, std::span<uint8_t> OutBlob) {
    if (FD->PRead(OutBlob.data(), OutBlob.size(), Offset) != (ssize_t)OutBlob.size()) {
      return false;
    }
    return true;
  }

  bool FOZFile::WriteBlob(const MesaFOZ::foz_payload_key& Key, std::span<const std::span<const uint8_t>> BlobChunks, uint64_t& OutBlobOffset) {
    ssize_t FileSize = FD->Size();
    if (FileSize < 0) {
      return false;
    }
    uint64_t WriteOffset = (uint64_t)FileSize;

    if (FD->PWrite(Key.bytes, sizeof(Key.bytes), WriteOffset) != sizeof(Key.bytes)) {
      return false;
    }
    WriteOffset += sizeof(Key.bytes);

    uint64_t TotalBlobSize = 0;
    for (const std::span<const uint8_t>& Chunk : BlobChunks) {
      TotalBlobSize += Chunk.size();
    }

    MesaFOZ::foz_payload_header ScratchHeader {.payload_size = (uint32_t)TotalBlobSize,
                                               .format = MesaFOZ::FOSSILIZE_COMPRESSION_NONE,
                                               .crc = 0, // todo? maybe
                                               .uncompressed_size = (uint32_t)TotalBlobSize};

    if (FD->PWrite(&ScratchHeader, sizeof(ScratchHeader), WriteOffset) != sizeof(ScratchHeader)) {
      return false;
    }
    WriteOffset += sizeof(ScratchHeader);

    OutBlobOffset = WriteOffset;

    for (const std::span<const uint8_t>& Chunk : BlobChunks) {
      if (Chunk.size() == 0) {
        continue;
      }
      if (FD->PWrite(Chunk.data(), Chunk.size(), WriteOffset) != (ssize_t)Chunk.size()) {
        return false;
      }
      WriteOffset += Chunk.size();
    }

    return true;
  }

  bool IndexedDB::Open(const fextl::string& CacheDBName, bool ReadOnly) {
    if (!CacheFOZ.Open(CacheDBName + ".foz", ReadOnly)) {
      return false;
    }
    if (!IndexFOZ.Open(CacheDBName + "_idx.foz", ReadOnly)) {
      return false;
    }

    File::File::FileHandleType CacheFileHandle = CacheFOZ.GetHandle();
    if (FileMapper && CacheFileHandle != (File::File::FileHandleType)-1) {
      CacheFileMapping = reinterpret_cast<uint8_t*>(FileMapper(CacheFileHandle, ReadOnly ? CacheFOZ.Size() : BIG_MAPPING_SIZE));
      CacheFileSize = CacheFOZ.Size();
    }

    this->ReadOnly = ReadOnly;
    return true;
  }

  void IndexedDB::PopulateIndex(Index& CacheIndex, bool& FoundMetadata) {
    fextl::vector<uint8_t> Data;
    if (!IndexFOZ.ReadAll(Data)) {
      return;
    }

    ssize_t CacheFOZSize = CacheFOZ.Size();
    if (CacheFOZSize < 0) {
      return;
    }

    const uint8_t* IndexDataStart = Data.data();
    const size_t IndexDataSize = Data.size();
    size_t ReadOffset = 0;
    while (ReadOffset + sizeof(MesaFOZ::foz_payload_key) + sizeof(MesaFOZ::foz_payload_header) <= IndexDataSize) {
      const auto* FOZKey = reinterpret_cast<const MesaFOZ::foz_payload_key*>(IndexDataStart + ReadOffset);
      ReadOffset += sizeof(MesaFOZ::foz_payload_key);
      const auto* FOZHeader = reinterpret_cast<const MesaFOZ::foz_payload_header*>(IndexDataStart + ReadOffset);
      ReadOffset += sizeof(MesaFOZ::foz_payload_header);

      uint64_t IndexBlobSize = sizeof(MesaFOZ::mesa_index_db_file_entry) + sizeof(IndexExtraBlobHeader);

      if (FOZHeader->payload_size < IndexBlobSize || ReadOffset + FOZHeader->payload_size > IndexDataSize) {
        break;
      }
      const auto* IndexBlobCommon = reinterpret_cast<const MesaFOZ::mesa_index_db_file_entry*>(IndexDataStart + ReadOffset);
      ReadOffset += sizeof(MesaFOZ::mesa_index_db_file_entry);
      const auto* IndexBlobExtra = reinterpret_cast<const IndexExtraBlobHeader*>(IndexDataStart + ReadOffset);
      ReadOffset += sizeof(IndexExtraBlobHeader);

      // skip corrupt (carefully) so we don't have to figure that out in the hot path later
      if (IndexBlobCommon->cache_db_file_offset > (uint64_t)CacheFOZSize ||
          IndexBlobCommon->size > (uint64_t)CacheFOZSize - IndexBlobCommon->cache_db_file_offset) {
        continue;
      }
      if (IndexBlobExtra->GuestSize + sizeof(BlobFixedHeader) > IndexBlobCommon->size) {
        continue;
      }
      IndexBlobSize += IndexBlobExtra->GuestExtentsCount * sizeof(uint32_t);
      if (FOZHeader->payload_size != IndexBlobSize) {
        break;
      }
      const auto* GuestExtents = reinterpret_cast<const uint32_t*>(IndexDataStart + ReadOffset);
      ReadOffset += IndexBlobExtra->GuestExtentsCount * sizeof(uint32_t);
      if (FOZKey->bytes[39] != 0xFF) {
        uint64_t Key;
        std::from_chars(reinterpret_cast<const char*>(FOZKey->bytes), reinterpret_cast<const char*>(&FOZKey->bytes[16]), Key, 16);
        if (IndexBlobCommon->hash != Key) {
          continue;
        }
        IndexEntry NewEntry {this, IndexBlobCommon->cache_db_file_offset, IndexBlobCommon->size, IndexBlobExtra->GuestSize,
                             IndexBlobExtra->GuestHash};
        if (IndexBlobExtra->GuestExtentsCount) {
          NewEntry.GuestExtents.resize(IndexBlobExtra->GuestExtentsCount);
          memcpy(NewEntry.GuestExtents.data(), GuestExtents, IndexBlobExtra->GuestExtentsCount * sizeof(uint32_t));
          bool ExtentsValid = true;
          for (uint32_t i = 0; i < NewEntry.GuestExtents.size(); i += 2) {
            if ((uint64_t)NewEntry.GuestExtents[i] + NewEntry.GuestExtents[i + 1] > IndexBlobExtra->GuestSize) {
              ExtentsValid = false;
              break;
            }
          }
          if (!ExtentsValid) {
            continue;
          }
        }
        CacheIndex.insert({IndexBlobCommon->hash, {std::move(NewEntry)}});
      } else {
        FoundMetadata = true;
      }
    }
    // could truncate/delete index if we don't end up perfectly at end here
  }

  bool IndexedDB::ReadCacheBlob(uint64_t Offset, std::span<uint8_t> OutBlob) {
    if (CacheFileMapping && (ReadOnly || Offset + OutBlob.size() <= BIG_MAPPING_SIZE)) {
      if (Offset + OutBlob.size() > CacheFileSize) {
        return false;
      }
      // todo could reduce copies by having a private mapping for relocs, etc
      memcpy(OutBlob.data(), CacheFileMapping + Offset, OutBlob.size());
      return true;
    } else {
      return CacheFOZ.ReadBlob(Offset, OutBlob);
    }
  }

  bool IndexedDB::StoreCacheBlob(const MesaFOZ::foz_payload_key& Key, std::span<const uint8_t> Blob, Index& Index, std::mutex& IndexMutex,
                                 std::span<const uint8_t> IndexBlob) {
    if (ReadOnly) {
      // shouldn't happen
      return false;
    }
    uint64_t Hash;
    if (Key.bytes[39] != 0xFF) {
      std::from_chars(reinterpret_cast<const char*>(Key.bytes), reinterpret_cast<const char*>(&Key.bytes[16]), Hash, 16);
    } else {
      Hash = ~0;
    }
    {
      std::lock_guard Guard(IndexMutex);
      if (Index.contains(Hash)) {
        // shouldn't really happen.. assert or something?
        return true;
      }
    }

    if (!CacheFOZ.Lock(STORE_LOCK_TIMEOUT_MS) || !IndexFOZ.Lock(STORE_LOCK_TIMEOUT_MS)) {
      CacheFOZ.Unlock();
      IndexFOZ.Unlock();
      return false;
    }

    // write cache side first so we get offset for index
    std::span<const uint8_t> BlobChunks[] = {Blob};
    uint64_t BlobOffset = 0;
    if (!CacheFOZ.WriteBlob(Key, BlobChunks, BlobOffset)) {
      CacheFOZ.Unlock();
      IndexFOZ.Unlock();
      return false;
    }

    MesaFOZ::mesa_index_db_file_entry IndexEntry {.hash = Hash,
                                                  .size = (uint32_t)Blob.size(),
                                                  .last_access_time = 0, // todo..
                                                  .cache_db_file_offset = BlobOffset};

    std::span<const uint8_t> IndexBlobChunks[] = {
      {(const uint8_t*)&IndexEntry, sizeof(IndexEntry)},
      IndexBlob,
    };
    uint64_t UnusedIndexBlobOffset = 0;
    if (!IndexFOZ.WriteBlob(Key, IndexBlobChunks, UnusedIndexBlobOffset)) {
      CacheFOZ.Unlock();
      IndexFOZ.Unlock();
      return false;
    }

    CacheFOZ.Unlock();
    IndexFOZ.Unlock();

    // publish new file size for memory-mapped reads
    if (CacheFileMapping) {
      CacheFileSize = BlobOffset + Blob.size();
    }

    const IndexExtraBlobHeader* IndexBlobHeader = reinterpret_cast<const IndexExtraBlobHeader*>(IndexBlob.data());

    struct IndexEntry NewEntry {this, BlobOffset, (uint32_t)Blob.size(), IndexBlobHeader->GuestSize, IndexBlobHeader->GuestHash};
    if (IndexBlobHeader->GuestExtentsCount) {
      NewEntry.GuestExtents.resize(IndexBlobHeader->GuestExtentsCount);
      memcpy(NewEntry.GuestExtents.data(), reinterpret_cast<const uint32_t*>(IndexBlob.data() + sizeof(IndexExtraBlobHeader)),
             IndexBlobHeader->GuestExtentsCount * sizeof(uint32_t));
    }
    std::lock_guard Guard(IndexMutex);
    Index.insert_or_assign(Hash, std::move(NewEntry));
    return true;
  }

  bool DiskCache::OpenCacheDB(const fextl::string& CacheDBName, bool ReadOnly) {
    fextl::unique_ptr<IndexedDB> CurDB;

    if (!ReadOnly && RWCacheDB) {
      // rw already opened, just support one
      return false;
    }

    CurDB = fextl::make_unique<IndexedDB>();
    if (!CurDB) {
      return false;
    }

    if (!CurDB->Open(CacheDBName, ReadOnly)) {
      CurDB.reset();
      return false;
    }

    CurDB->PopulateIndex(Index, FoundMetadata);

    if (ReadOnly) {
      ROCacheDBs.push_back(std::move(CurDB));
    } else {
      RWCacheDB = std::move(CurDB);
    }

    return true;
  }

  FEX_DEFAULT_VISIBILITY void SetFileMapper(FileMapperFunc Func) {
    FileMapper = Func;
  }

  void DiskCache::Init(FEXCore::Context::ContextImpl* CTX) {
    this->CTX = CTX;

    if (!EnableDiskCache) {
      return;
    }

    fextl::string SerializedConfig = FEXCore::Config::SerializeForCache();

    struct __attribute__((packed)) {
      uint16_t FormatVersion;
      uint8_t Is64BitMode;
      uint64_t HostFeaturesHash;
    } BucketHeader = {FormatVersion, CTX->Config.Is64BitMode, CTX->HostFeatures.HashForCaching()};

    fextl::vector<uint8_t> BucketBytes;
    BucketBytes.resize(sizeof(BucketHeader) + SerializedConfig.size());
    memcpy(BucketBytes.data(), &BucketHeader, sizeof(BucketHeader));
    memcpy(BucketBytes.data() + sizeof(BucketHeader), SerializedConfig.data(), SerializedConfig.size());
    BucketHash = XXH3_128bits(BucketBytes.data(), BucketBytes.size());

    fextl::string BasePath = BasePathOverride();
    if (BasePath.empty()) {
      BasePath = FEXCore::Config::GetCacheDirectory() + "DiskCache/";
      BasePath += fextl::fmt::format("{:016x}{:016x}", BucketHash.high64, BucketHash.low64) + "/";
    }
    FHU::Filesystem::CreateDirectories(BasePath);

    if (!MapDiskCacheFiles) {
      FileMapper = nullptr;
    }

    fextl::string RWDBBasePath = BasePath + "RWCacheDB";
    OpenCacheDB(RWDBBasePath, false);

    if (RWCacheDB && !FoundMetadata) {
      // we just opened a fresh cache, add a metadata blob
      MesaFOZ::foz_payload_key MetadataKey;
      memset(MetadataKey.bytes, 0xFF, sizeof(MetadataKey));
      IndexExtraBlobHeader MetaDataHeader = {};
      RWCacheDB->StoreCacheBlob(MetadataKey, {BucketBytes.data(), BucketBytes.size()}, Index, IndexLock,
                                {reinterpret_cast<uint8_t*>(&MetaDataHeader), sizeof(MetaDataHeader)});
      Index.clear();
    }

    std::string_view RONames = RODBNames();
    while (!RONames.empty()) {
      const auto Delim = RONames.find(',');
      const std::string_view ROName = RONames.substr(0, Delim);
      if (!ROName.empty()) {
        fextl::string RODBBasePath = BasePath;
        RODBBasePath += ROName;
        OpenCacheDB(RODBBasePath, true);
      }
      if (Delim == std::string_view::npos) {
        break;
      }
      // advance to next
      RONames.remove_prefix(Delim + 1);
    }

    if (IsWritingDiskCache()) {
      FEXCore::Threads::Flags WriterThreadFlags = {.LowPriority = true, .Internal = true};
      Writer = fextl::make_unique<WorkQueueThread>(WriterThreadFlags);
    }
  }

  uint64_t DiskCache::MakeBlobKey(const uint64_t ModuleOffset) {
    struct {
      uint64_t ModuleOffset;
      XXH128_hash_t BucketHash;
    } BlobKeyBytes = {ModuleOffset, BucketHash};

    return XXH3_64bits(&BlobKeyBytes, sizeof(BlobKeyBytes));
  }

  std::optional<CodeHitData> DiskCache::Lookup(Core::InternalThreadState* Thread, const ExecutableFileSectionInfo& Region, uint64_t GuestRIP) {
    if (!IsReadingDiskCache()) {
      return std::nullopt;
    }
    uint64_t ModuleOffset = GuestRIP - Region.FileStartVA;

    uint64_t Hash = MakeBlobKey(ModuleOffset);

    IndexEntry Entry;
    {
      std::lock_guard Guard(IndexLock);
      auto It = Index.find(Hash);
      if (It == Index.end()) {
        // definite miss
        return std::nullopt;
      }
      // we can't hold onto the iterator, the map may shift while we don't hold the lock
      Entry = It->second;
    }
    // found a key hash match, could still be a miss, check guest hash

    // do we have enough room in our live code to even hash GuestSize worth?
    auto RangeInfo = CTX->SyscallHandler->QueryGuestExecutableRange(Thread, GuestRIP);
    if (RangeInfo.Size == 0 || RangeInfo.Base > GuestRIP) {
      return std::nullopt;
    }
    uint64_t Available = RangeInfo.Base + RangeInfo.Size - GuestRIP;
    if (Available < Entry.GuestSize) {
      return std::nullopt;
    }

    XXH128_hash_t LiveGuestHash;

    // if (Entry.GuestExtents.size()) {
    //   LogMan::Msg::IFmt("lookup! length {:d}", Entry.GuestSize);
    //   for(uint32_t i = 0; i < Entry.GuestExtents.size(); i+=2 ) {
    //     LogMan::Msg::IFmt("extent {} {}", Entry.GuestExtents[i], Entry.GuestExtents[i]+Entry.GuestExtents[i+1]);
    //   }
    // }
    if (Entry.GuestExtents.size() == 0) {
      LiveGuestHash = XXH3_128bits(reinterpret_cast<void*>(GuestRIP), Entry.GuestSize);
    } else {
      XXH3_state_t HashState;
      XXH3_128bits_reset(&HashState);
      for (uint32_t i = 0; i < Entry.GuestExtents.size(); i += 2) {
        XXH3_128bits_update(&HashState, reinterpret_cast<uint8_t*>(GuestRIP) + Entry.GuestExtents[i], Entry.GuestExtents[i + 1]);
      }
      LiveGuestHash = XXH3_128bits_digest(&HashState);
    }
    if (!XXH128_isEqual(LiveGuestHash, Entry.GuestHash)) {
      // LogMan::Msg::IFmt("hash mismatch! length {:d}", Header.GuestSize);
      return std::nullopt;
    }
    // LogMan::Msg::IFmt("hash ok! length {:d}", Header.GuestSize);

    // this seems to be a full hit, pull from disk and check the entry is big enough to have everything (except GuestCode)
    CodeHitData HitData;
    uint32_t EntrySizeWithoutGuestCode = Entry.Size - Entry.GuestSize;
    HitData.Blob.resize(EntrySizeWithoutGuestCode);
    if (!Entry.DB->ReadCacheBlob(Entry.Offset, HitData.Blob)) {
      return std::nullopt;
    }

    if (EntrySizeWithoutGuestCode < sizeof(BlobFixedHeader)) {
      return std::nullopt;
    }
    BlobFixedHeader Header;
    memcpy(&Header, HitData.Blob.data(), sizeof(Header));

    uint32_t SizeNeeded = sizeof(Header) + Header.HostSize + Header.EntryPointCount * (sizeof(uint64_t) + sizeof(uint32_t));
    SizeNeeded += Header.SmallRelocCount * sizeof(BlobSmallRelocation) + Header.ThunkRelocCount * sizeof(BlobThunkRelocation) +
                  Header.TouchedGuestPagesCount * sizeof(uint64_t);
    if (EntrySizeWithoutGuestCode != SizeNeeded) {
      return std::nullopt;
    }

    if (Entry.GuestSize != Header.GuestSize || !XXH128_isEqual(Header.GuestHash, Entry.GuestHash)) {
      return std::nullopt;
    }

    uint32_t BlobOffset = sizeof(Header);

    HitData.HostCode = {HitData.Blob.data() + BlobOffset, Header.HostSize};
    BlobOffset += Header.HostSize;
    HitData.GuestPages = {reinterpret_cast<uint64_t*>(HitData.Blob.data() + BlobOffset), Header.TouchedGuestPagesCount};
    BlobOffset += Header.TouchedGuestPagesCount * sizeof(uint64_t);
    HitData.EntryPointRIPs = {reinterpret_cast<uint64_t*>(HitData.Blob.data() + BlobOffset), Header.EntryPointCount};
    BlobOffset += Header.EntryPointCount * sizeof(uint64_t);
    HitData.EntryPointHostOffsets = {reinterpret_cast<const uint32_t*>(HitData.Blob.data() + BlobOffset), Header.EntryPointCount};
    BlobOffset += Header.EntryPointCount * sizeof(uint32_t);
    HitData.SmallRelocs = {reinterpret_cast<const BlobSmallRelocation*>(HitData.Blob.data() + BlobOffset), Header.SmallRelocCount};
    BlobOffset += Header.SmallRelocCount * sizeof(BlobSmallRelocation);
    HitData.ThunkRelocs = {reinterpret_cast<const BlobThunkRelocation*>(HitData.Blob.data() + BlobOffset), Header.ThunkRelocCount};
    BlobOffset += Header.ThunkRelocCount * sizeof(BlobThunkRelocation);

    for (auto& PageOffset : HitData.GuestPages) {
      PageOffset += GuestRIP;
    }
    for (auto& EntryPointRip : HitData.EntryPointRIPs) {
      EntryPointRip += GuestRIP;
    }

    return HitData;
  }

  struct DiskCache::CacheStoreWorkItem final : WorkQueueThread::WorkItem {
    DiskCache* Self;
    IndexedDB* DB;
    MesaFOZ::foz_payload_key Key;
    fextl::vector<uint8_t> Blob;
    fextl::vector<uint8_t> IndexBlob;
    CacheStoreWorkItem(DiskCache* Self, IndexedDB* DB, const MesaFOZ::foz_payload_key& Key, fextl::vector<uint8_t>&& Blob,
                       fextl::vector<uint8_t>&& IndexBlob)
      : Self(Self)
      , DB(DB)
      , Key(Key)
      , Blob(std::move(Blob))
      , IndexBlob(std::move(IndexBlob)) {}
    void Run() override {
      DB->StoreCacheBlob(Key, Blob, Self->Index, Self->IndexLock, IndexBlob);
    }
  };

  bool DiskCache::Store(Core::InternalThreadState* Thread, const ExecutableFileSectionInfo& Region, uint64_t GuestRIP,
                        std::span<const uint8_t> GuestCode, const CPU::CPUBackend::CompiledCode& CompiledCode,
                        std::span<const FEXCore::CPU::Relocation> Relocations, const Frontend::Decoder::DecodedBlockInformation* DecodedBlockInfo) {
    if (!IsWritingDiskCache()) {
      return false;
    }
    if (!DecodedBlockInfo) {
      return false;
    }

    // check for any reloc targets outside of our jurisdiction
    // todo what are they exactly? caching those blocks is great when it works, so need to figure this out and make finer-grained if we can
    if (RelocationFilter) {
      for (const auto& Reloc : Relocations) {
        if (Reloc.Header.Type != CPU::RelocationTypes::RELOC_GUEST_RIP_LITERAL && Reloc.Header.Type != CPU::RelocationTypes::RELOC_GUEST_RIP_MOVE) {
          continue;
        }
        uint64_t Target = Reloc.GuestRIP.GuestRIP;
        if (Target >= Region.BeginVA && Target < Region.EndVA) {
          continue;
        }
        auto TargetSection = CTX->SyscallHandler->LookupExecutableFileSection(Thread, Target);
        if (!TargetSection || TargetSection->FileInfo.FileId != Region.FileInfo.FileId) {
          // we don't know where it's pointing, so we don't know how to encode the offset, so we can't cache atm
          return false;
        }
      }
    }

    uint32_t SmallRelocCount = 0;
    uint32_t ThunkRelocCount = 0;
    for (const auto& Reloc : Relocations) {
      if (Reloc.Header.Type == CPU::RelocationTypes::RELOC_NAMED_THUNK_MOVE) {
        ThunkRelocCount++;
      } else {
        SmallRelocCount++;
      }
    }

    fextl::vector<uint32_t> ExactGuestCodeExtents;
    uint64_t CurStartExtent = 0, CurEndExtent = 0;
    const Frontend::Decoder::DecodedBlocks* LastBlock = nullptr;
    for (auto& SubBlock : DecodedBlockInfo->Blocks) {
      if (!CurStartExtent) {
        CurStartExtent = SubBlock.Entry;
        CurEndExtent = SubBlock.Entry + SubBlock.Size;
      } else {
        LOGMAN_THROW_A_FMT(SubBlock.Entry >= CurEndExtent, "DecodedBlocks not sorted or overlapping?");
        if (SubBlock.Entry == CurEndExtent) {
          CurEndExtent = SubBlock.Entry + SubBlock.Size;
        } else {
          ExactGuestCodeExtents.push_back(CurStartExtent - GuestRIP);
          ExactGuestCodeExtents.push_back(CurEndExtent - CurStartExtent);
          CurStartExtent = SubBlock.Entry;
          CurEndExtent = SubBlock.Entry + SubBlock.Size;
        }
      }
      LastBlock = &SubBlock;
    }
    if (LastBlock && (CurStartExtent != GuestRIP || CurEndExtent != GuestRIP + GuestCode.size())) {
      ExactGuestCodeExtents.push_back(CurStartExtent - GuestRIP);
      ExactGuestCodeExtents.push_back(CurEndExtent - CurStartExtent);
    }

    // if (ExactGuestCodeExtents.size()) {
    //   LogMan::Msg::IFmt("store! length {:d}", GuestCode.size());
    //   for(uint32_t i = 0; i < ExactGuestCodeExtents.size(); i+=2 ) {
    //     LogMan::Msg::IFmt("extent {} {}", ExactGuestCodeExtents[i], ExactGuestCodeExtents[i]+ExactGuestCodeExtents[i+1]);
    //   }
    // }

    const uint32_t EntryPointCount = (uint32_t)CompiledCode.EntryPoints.size();
    const uint32_t TouchedGuestPagesCount = DecodedBlockInfo ? (uint32_t)DecodedBlockInfo->CodePages.size() : 0;

    const size_t HeaderOffset = 0;
    const size_t HostCodeOffset = HeaderOffset + sizeof(BlobFixedHeader);
    const size_t TouchedGuestPagesOffset = HostCodeOffset + CompiledCode.Size;
    const size_t EntryPointRIPsOffset = TouchedGuestPagesOffset + TouchedGuestPagesCount * sizeof(uint64_t);
    const size_t EntryPointHostOffsetsOffset = EntryPointRIPsOffset + EntryPointCount * sizeof(uint64_t);
    const size_t SmallRelocsOffset = EntryPointHostOffsetsOffset + EntryPointCount * sizeof(uint32_t);
    const size_t ThunkRelocsOffset = SmallRelocsOffset + SmallRelocCount * sizeof(BlobSmallRelocation);
    const size_t GuestCodeOffset = ThunkRelocsOffset + ThunkRelocCount * sizeof(BlobThunkRelocation);
    const size_t TotalSize = GuestCodeOffset + GuestCode.size();

    // we'll copy everything into here and pass it to the Writer, then return to caller quickly
    fextl::vector<uint8_t> Blob;
    Blob.resize(TotalSize);
    uint8_t* BlobData = Blob.data();

    uint64_t ModuleOffset = GuestRIP - Region.FileStartVA;

    uint64_t BlobKey = MakeBlobKey(ModuleOffset);
    MesaFOZ::foz_payload_key Key = {};
    fextl::string BlobName = fextl::fmt::format("{:016x}", BlobKey);
    memcpy(Key.bytes, BlobName.data(), BlobName.size());

    BlobFixedHeader Header {
      .GuestSize = (uint32_t)GuestCode.size(),
      .HostSize = (uint32_t)CompiledCode.Size,
      .EntryPointCount = EntryPointCount,
      .SmallRelocCount = SmallRelocCount,
      .ThunkRelocCount = ThunkRelocCount,
      .TouchedGuestPagesCount = TouchedGuestPagesCount,
    };

    if (ExactGuestCodeExtents.size() == 0) {
      Header.GuestHash = XXH3_128bits(GuestCode.data(), GuestCode.size());
    } else {
      XXH3_state_t HashState;
      XXH3_128bits_reset(&HashState);
      for (uint32_t i = 0; i < ExactGuestCodeExtents.size(); i += 2) {
        XXH3_128bits_update(&HashState, GuestCode.data() + ExactGuestCodeExtents[i], ExactGuestCodeExtents[i + 1]);
      }
      Header.GuestHash = XXH3_128bits_digest(&HashState);
    }
    memcpy(BlobData + HeaderOffset, &Header, sizeof(Header));
    memcpy(BlobData + HostCodeOffset, CompiledCode.BlockBegin, CompiledCode.Size);

    // relocate touched pages relative to GuestRIP
    auto* PageOffsets = reinterpret_cast<uint64_t*>(BlobData + TouchedGuestPagesOffset);
    uint32_t PageIdx = 0;
    for (auto GuestPage : DecodedBlockInfo->CodePages) {
      PageOffsets[PageIdx++] = GuestPage - GuestRIP;
    }

    // pack and relocate entrypoints
    auto* EntryRIPs = reinterpret_cast<uint64_t*>(BlobData + EntryPointRIPsOffset);
    auto* EntryHostOffsets = reinterpret_cast<uint32_t*>(BlobData + EntryPointHostOffsetsOffset);
    uint32_t EntryIdx = 0;
    for (auto [GuestAddr, HostAddr] : CompiledCode.EntryPoints) {
      EntryRIPs[EntryIdx] = GuestAddr - GuestRIP;
      EntryHostOffsets[EntryIdx] = uint32_t(HostAddr - CompiledCode.BlockBegin);
      EntryIdx++;
    }

    // pack relocations
    auto* SmallRelocs = reinterpret_cast<BlobSmallRelocation*>(BlobData + SmallRelocsOffset);
    auto* ThunkRelocs = reinterpret_cast<BlobThunkRelocation*>(BlobData + ThunkRelocsOffset);
    uint32_t SmallIdx = 0;
    uint32_t ThunkIdx = 0;
    for (const auto& Reloc : Relocations) {
      switch (Reloc.Header.Type) {
      // it's important to zero-init the element completely so we don't have garbage in unused fields
      // this way, the caches stay deterministic across machines
      case CPU::RelocationTypes::RELOC_NAMED_SYMBOL_LITERAL: {
        BlobSmallRelocation SmallReloc = {};
        SmallReloc.Offset = Reloc.Header.Offset;
        SmallReloc.Type = uint8_t(Reloc.Header.Type);
        SmallReloc.Named.Symbol = uint32_t(Reloc.NamedSymbolLiteral.Symbol);
        SmallRelocs[SmallIdx++] = SmallReloc;
        break;
      }
      case CPU::RelocationTypes::RELOC_GUEST_RIP_LITERAL: {
        BlobSmallRelocation SmallReloc = {};
        SmallReloc.Offset = Reloc.Header.Offset;
        SmallReloc.Type = uint8_t(Reloc.Header.Type);
        SmallReloc.RIPLiteral.GuestRIP = Reloc.GuestRIP.GuestRIP - GuestRIP;
        SmallRelocs[SmallIdx++] = SmallReloc;
        break;
      }
      case CPU::RelocationTypes::RELOC_GUEST_RIP_MOVE: {
        BlobSmallRelocation SmallReloc = {};
        SmallReloc.Offset = Reloc.Header.Offset;
        SmallReloc.Type = uint8_t(Reloc.Header.Type);
        SmallReloc.RIPMove.RegisterIndex = Reloc.GuestRIP.RegisterIndex;
        SmallReloc.RIPMove.GuestRIP = Reloc.GuestRIP.GuestRIP - GuestRIP;
        SmallRelocs[SmallIdx++] = SmallReloc;
        break;
      }
      case CPU::RelocationTypes::RELOC_NAMED_THUNK_MOVE: {
        BlobThunkRelocation BigReloc = {};
        BigReloc.Offset = Reloc.Header.Offset;
        BigReloc.RegisterIndex = Reloc.NamedThunkMove.RegisterIndex;
        memcpy(BigReloc.SymbolHash, &Reloc.NamedThunkMove.Symbol, sizeof(BigReloc.SymbolHash));
        ThunkRelocs[ThunkIdx++] = BigReloc;
        break;
      }
      }
    }

    memcpy(BlobData + GuestCodeOffset, GuestCode.data(), GuestCode.size());

    fextl::vector<uint8_t> IndexBlob;
    IndexBlob.resize(sizeof(IndexExtraBlobHeader) + ExactGuestCodeExtents.size() * sizeof(uint32_t));

    IndexExtraBlobHeader IndexBlobHeader {Header.GuestHash, Header.GuestSize, (uint32_t)ExactGuestCodeExtents.size()};
    memcpy(IndexBlob.data(), &IndexBlobHeader, sizeof(IndexExtraBlobHeader));
    memcpy(IndexBlob.data() + sizeof(IndexExtraBlobHeader), ExactGuestCodeExtents.data(), ExactGuestCodeExtents.size() * sizeof(uint32_t));

    // hand the rest off to the writer thread
    Writer->QueueWork(fextl::make_unique<CacheStoreWorkItem>(this, RWCacheDB.get(), Key, std::move(Blob), std::move(IndexBlob)));
    return true;
  }

  uint16_t GetFormatVersion() {
    return FormatVersion;
  }
} // namespace DiskCache

} // namespace FEXCore
