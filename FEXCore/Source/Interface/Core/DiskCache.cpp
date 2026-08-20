// SPDX-License-Identifier: MIT

#include "FEXHeaderUtils/Filesystem.h"
#include "FEXCore/Core/DiskCache.h"
#include "FEXCore/Utils/LogManager.h"
#include "Interface/Context/Context.h"
#include "FEXCore/HLE/SyscallHandler.h"
#include "FEXCore/Utils/File.h"
#include "FEXCore/fextl/memory.h"
#include <cstdint>
#include <cstring>

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

  bool FOZFile::Open(const fextl::string& FOZFileName, bool ReadOnly) {
    FileName = FOZFileName;
    this->ReadOnly = ReadOnly;

    File::FileModes Modes = File::FileModes::READ;
    if (!ReadOnly) {
      Modes = Modes | File::FileModes::WRITE | File::FileModes::CREATE;
    }
    FD = fextl::make_unique<File::File>(FileName.c_str(), Modes);
    if (!FD->IsValid()) {
      FD.reset();
      return false;
    }

    bool Valid = false;
    bool TookLock = false;
    ssize_t Size = FD->Seek(0, File::SeekOp::END);

    if (Size < FOZ_REF_MAGIC_SIZE && !ReadOnly) {
      if (!FD->Lock(OPEN_LOCK_TIMEOUT_MS)) {
        FD.reset();
        return false;
      }
      TookLock = true;
      // seek in case someone else made it while we waited above
      Size = FD->Seek(0, File::SeekOp::END);
    }

    if (Size == 0 && !ReadOnly) {
      Valid = FD->Write(MesaFOZ::stream_reference_magic_and_version, FOZ_REF_MAGIC_SIZE) == FOZ_REF_MAGIC_SIZE;
    } else {
      FD->Seek(0, File::SeekOp::BEGIN);
      uint8_t magic[FOZ_REF_MAGIC_SIZE];
      if (FD->Read(magic, FOZ_REF_MAGIC_SIZE) == FOZ_REF_MAGIC_SIZE &&
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

  bool FOZFile::ReadNextBlob(MesaFOZ::foz_payload_key& OutKey, MesaFOZ::foz_payload_header& OutHeader, fextl::vector<uint8_t>& OutBlob) {
    if (FD->Read(OutKey.bytes, sizeof(OutKey.bytes)) != sizeof(OutKey.bytes)) {
      return false;
    }
    if (FD->Read(&OutHeader, sizeof(OutHeader)) != sizeof(OutHeader)) {
      return false;
    }
    OutBlob.resize(OutHeader.payload_size);
    if (FD->Read(OutBlob.data(), OutBlob.size()) != (ssize_t)OutBlob.size()) {
      return false;
    }
    return true;
  }

  bool FOZFile::ReadBlob(uint64_t Offset, std::span<uint8_t> OutBlob) {
    ssize_t SeekRet = FD->Seek(Offset, File::SeekOp::BEGIN);
    if (SeekRet < 0) {
      return false;
    }
    if (FD->Read(OutBlob.data(), OutBlob.size()) != (ssize_t)OutBlob.size()) {
      return false;
    }

    return true;
  }

  bool FOZFile::WriteBlob(const MesaFOZ::foz_payload_key& Key, std::span<const std::span<const uint8_t>> BlobChunks, uint64_t& OutBlobOffset) {
    uint64_t WriteOffset = 0;
    ssize_t SeekRet = FD->Seek(0, File::SeekOp::END);
    if (SeekRet < 0) {
      return false;
    }
    WriteOffset = (uint64_t)SeekRet;

    if (FD->Write(Key.bytes, sizeof(Key.bytes)) != sizeof(Key.bytes)) {
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

    if (FD->Write(&ScratchHeader, sizeof(ScratchHeader)) != sizeof(ScratchHeader)) {
      return false;
    }
    WriteOffset += sizeof(ScratchHeader);

    OutBlobOffset = WriteOffset;

    for (const std::span<const uint8_t>& Chunk : BlobChunks) {
      if (Chunk.size() == 0) {
        continue;
      }
      if (FD->Write(Chunk.data(), Chunk.size()) != (ssize_t)Chunk.size()) {
        return false;
      }
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

    this->ReadOnly = ReadOnly;
    return true;
  }

  void IndexedDB::PopulateIndex(Index& CacheIndex) {
    MesaFOZ::foz_payload_key Key;
    MesaFOZ::foz_payload_header Header;
    fextl::vector<uint8_t> Blob;

    while (IndexFOZ.ReadNextBlob(Key, Header, Blob)) {
      if (Blob.size() != sizeof(MesaFOZ::mesa_index_db_file_entry)) {
        break;
      }
      MesaFOZ::mesa_index_db_file_entry* IndexEntry = (MesaFOZ::mesa_index_db_file_entry*)Blob.data();
      if (IndexEntry->hash != XXH3_64bits(Key.bytes, FOSSILIZE_BLOB_HASH_LENGTH)) {
        break;
      }
      CacheIndex.insert({IndexEntry->hash, {this, IndexEntry->cache_db_file_offset, IndexEntry->size}});
    }
    // could truncate/delete index if we don't end up perfectly at end here
  }

  bool IndexedDB::ReadCacheBlob(uint64_t Offset, std::span<uint8_t> OutBlob) {
    return CacheFOZ.ReadBlob(Offset, OutBlob);
  }

  bool IndexedDB::StoreCacheBlob(const MesaFOZ::foz_payload_key& Key, std::span<const std::span<const uint8_t>> BlobChunks, Index& Index) {
    if (ReadOnly) {
      // shouldn't happen
      return false;
    }
    uint64_t Hash = XXH3_64bits(Key.bytes, FOSSILIZE_BLOB_HASH_LENGTH);
    if (Index.contains(Hash)) {
      // shouldn't really happen.. assert or something?
      return true;
    }

    if (!CacheFOZ.Lock(STORE_LOCK_TIMEOUT_MS) || !IndexFOZ.Lock(STORE_LOCK_TIMEOUT_MS)) {
      CacheFOZ.Unlock();
      IndexFOZ.Unlock();
      return false;
    }

    // write cache side first so we get offset for index
    uint64_t BlobOffset = 0;
    if (!CacheFOZ.WriteBlob(Key, BlobChunks, BlobOffset)) {
      CacheFOZ.Unlock();
      IndexFOZ.Unlock();
      return false;
    }

    uint64_t TotalBlobSize = 0;
    for (const std::span<const uint8_t>& Chunk : BlobChunks) {
      TotalBlobSize += Chunk.size();
    }

    MesaFOZ::mesa_index_db_file_entry IndexEntry {.hash = Hash,
                                                  .size = (uint32_t)TotalBlobSize,
                                                  .last_access_time = 0, // todo..
                                                  .cache_db_file_offset = BlobOffset};

    std::span<const uint8_t> IndexBlobChunks[] = {{(const uint8_t*)&IndexEntry, sizeof(IndexEntry)}};
    uint64_t UnusedIndexBlobOffset = 0;
    if (!IndexFOZ.WriteBlob(Key, IndexBlobChunks, UnusedIndexBlobOffset)) {
      CacheFOZ.Unlock();
      IndexFOZ.Unlock();
      return false;
    }

    CacheFOZ.Unlock();
    IndexFOZ.Unlock();

    Index[Hash] = {this, BlobOffset, (uint32_t)TotalBlobSize};
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

    CurDB->PopulateIndex(Index);

    if (ReadOnly) {
      ROCacheDBs.push_back(std::move(CurDB));
    } else {
      RWCacheDB = std::move(CurDB);
    }

    return true;
  }

  void DiskCache::Init(FEXCore::Context::ContextImpl* CTX) {
    this->CTX = CTX;

    if (!EnableDiskCache) {
      return;
    }

    // todo grab all CTX options that can change compilation here + any environmental/hw things and hash into a bucket key

    fextl::string BasePath = BasePathOverride();
    if (BasePath.empty()) {
      // todo put bucket hash in that path
      BasePath = FEXCore::Config::GetCacheDirectory() + "DiskCache/";
    }
    FHU::Filesystem::CreateDirectories(BasePath);

    fextl::string RWDBBasePath = BasePath + "RWCacheDB";
    OpenCacheDB(RWDBBasePath, false);

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
  }

  std::optional<CodeHitData> DiskCache::Lookup(Core::InternalThreadState* Thread, const ExecutableFileSectionInfo& Region, uint64_t GuestRIP) {
    if (!IsReadingDiskCache()) {
      return std::nullopt;
    }
    std::lock_guard Guard(Lock);
    uint64_t ModuleOffset = GuestRIP - Region.FileStartVA;

    // todo move key making to a helper once we have options and stuff (see Store)
    MesaFOZ::foz_payload_key Key = {};
    memcpy(Key.bytes, &ModuleOffset, sizeof(ModuleOffset));

    uint64_t Hash = XXH3_64bits(Key.bytes, FOSSILIZE_BLOB_HASH_LENGTH);
    auto It = Index.find(Hash);
    if (It == Index.end()) {
      // definite miss
      return std::nullopt;
    }
    const IndexEntry& Entry = It->second;
    // found a key hash match, could still be a miss, read the blob and verify more
    CodeHitData HitData;
    HitData.Blob.resize(Entry.Size);
    if (!Entry.DB->ReadCacheBlob(Entry.Offset, HitData.Blob)) {
      return std::nullopt;
    }

    if (Entry.Size < sizeof(BlobFixedHeader)) {
      return std::nullopt;
    }
    BlobFixedHeader Header;
    memcpy(&Header, HitData.Blob.data(), sizeof(Header));

    // do we have enough room in our live code to even hash GuestSize worth?
    auto RangeInfo = CTX->SyscallHandler->QueryGuestExecutableRange(Thread, GuestRIP);
    if (RangeInfo.Size == 0 || RangeInfo.Base > GuestRIP) {
      return std::nullopt;
    }
    uint64_t Available = RangeInfo.Base + RangeInfo.Size - GuestRIP;
    if (Available < Header.GuestSize) {
      return std::nullopt;
    }

    XXH128_hash_t LiveGuestHash = XXH3_128bits(reinterpret_cast<void*>(GuestRIP), Header.GuestSize);
    if (std::memcmp(&LiveGuestHash, &Header.GuestHash, sizeof(Header.GuestHash)) != 0) {
      // LogMan::Msg::IFmt("hash mismatch! length {:d}", Header.GuestSize);
      return std::nullopt;
    }
    // LogMan::Msg::IFmt("hash ok! length {:d}", Header.GuestSize);

    // this seems to be a full hit, lastly, check the entry is big enough to have everything (except maybe GuestCode)
    uint32_t SizeNeeded = sizeof(Header) + Header.HostSize + Header.EntryPointCount * sizeof(BlobEntryPoint);
    SizeNeeded += Header.SmallRelocCount * sizeof(BlobSmallRelocation) + Header.ThunkRelocCount * sizeof(BlobThunkRelocation) +
                  Header.TouchedGuestPagesCount * sizeof(int64_t);
    if (Entry.Size < SizeNeeded) {
      return std::nullopt;
    }

    HitData.HostCode = {HitData.Blob.data() + sizeof(Header), Header.HostSize};
    HitData.EntryPoints = {reinterpret_cast<const BlobEntryPoint*>(HitData.Blob.data() + sizeof(Header) + Header.HostSize), Header.EntryPointCount};

    auto* SmallRelocs = reinterpret_cast<const BlobSmallRelocation*>(
      HitData.Blob.data() + sizeof(Header) + Header.HostSize + Header.EntryPointCount * sizeof(BlobEntryPoint));
    auto* ThunkRelocs = reinterpret_cast<const BlobThunkRelocation*>(
      reinterpret_cast<const uint8_t*>(SmallRelocs) + Header.SmallRelocCount * sizeof(BlobSmallRelocation));

    HitData.Relocations.reserve(Header.SmallRelocCount + Header.ThunkRelocCount);
    for (uint32_t i = 0; i < Header.SmallRelocCount; ++i) {
      const auto& SmallReloc = SmallRelocs[i];
      FEXCore::CPU::Relocation Reloc = FEXCore::CPU::Relocation::Default();
      Reloc.Header.Type = (CPU::RelocationTypes)SmallReloc.Type;
      Reloc.Header.Offset = SmallReloc.Offset;
      switch (SmallReloc.Type) {
      case uint8_t(CPU::RelocationTypes::RELOC_NAMED_SYMBOL_LITERAL):
        Reloc.NamedSymbolLiteral.Symbol = CPU::RelocNamedSymbolLiteral::NamedSymbol(SmallReloc.Named.Symbol);
        break;
      case uint8_t(CPU::RelocationTypes::RELOC_GUEST_RIP_LITERAL): Reloc.GuestRIP.GuestRIP = SmallReloc.RIPLiteral.GuestRIP; break;
      case uint8_t(CPU::RelocationTypes::RELOC_GUEST_RIP_MOVE):
        Reloc.GuestRIP.RegisterIndex = SmallReloc.RIPMove.RegisterIndex;
        Reloc.GuestRIP.GuestRIP = SmallReloc.RIPMove.GuestRIP;
        break;
      default: return std::nullopt;
      }
      HitData.Relocations.push_back(Reloc);
    }
    for (uint32_t i = 0; i < Header.ThunkRelocCount; ++i) {
      const auto& BigReloc = ThunkRelocs[i];
      FEXCore::CPU::Relocation Reloc = FEXCore::CPU::Relocation::Default();
      Reloc.NamedThunkMove.Header.Offset = BigReloc.Offset;
      Reloc.NamedThunkMove.Header.Type = CPU::RelocationTypes::RELOC_NAMED_THUNK_MOVE;
      Reloc.NamedThunkMove.RegisterIndex = BigReloc.RegisterIndex;
      memcpy(&Reloc.NamedThunkMove.Symbol, BigReloc.SymbolHash, sizeof(BigReloc.SymbolHash));
      HitData.Relocations.push_back(Reloc);
    }

    auto* PageOffsets =
      reinterpret_cast<const int64_t*>(reinterpret_cast<const uint8_t*>(ThunkRelocs) + Header.ThunkRelocCount * sizeof(BlobThunkRelocation));
    HitData.GuestPages.reserve(Header.TouchedGuestPagesCount);
    for (uint32_t i = 0; i < Header.TouchedGuestPagesCount; ++i) {
      HitData.GuestPages.push_back(GuestRIP + PageOffsets[i]);
    }

    return HitData;
  }

  bool DiskCache::Store(Core::InternalThreadState* Thread, const ExecutableFileSectionInfo& Region, uint64_t GuestRIP,
                        std::span<const uint8_t> GuestCode, const CPU::CPUBackend::CompiledCode& CompiledCode,
                        std::span<const FEXCore::CPU::Relocation> Relocations, const Frontend::Decoder::DecodedBlockInformation* DecodedBlockInfo) {
    if (!IsWritingDiskCache()) {
      return false;
    }
    if (!DecodedBlockInfo) {
      return false;
    }
    std::lock_guard Guard(Lock);

    // check for any reloc targets outside of our jurisdiction
    // todo what are they exactly? caching those blocks is great when it works, so need to figure this out and make finer-grained if we can
    if (RelocationFilter) {
      for (const auto& Reloc : Relocations) {
        if (Reloc.Header.Offset < CompiledCode.HostCodeOffset || Reloc.Header.Offset >= CompiledCode.HostCodeOffset + CompiledCode.Size) {
          continue;
        }
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

    // pack entrypoints to disk format
    fextl::vector<BlobEntryPoint> CacheEntryPoints;
    CacheEntryPoints.reserve(CompiledCode.EntryPoints.size());

    for (auto [GuestAddr, HostAddr] : CompiledCode.EntryPoints) {
      CacheEntryPoints.push_back({GuestAddr - Region.FileStartVA, uint32_t(HostAddr - CompiledCode.BlockBegin)});
    }

    // pack relocations to disk format
    fextl::vector<BlobSmallRelocation> SmallRelocs;
    fextl::vector<BlobThunkRelocation> ThunkRelocs;

    // todo discover sizes first and reserve vecs?

    for (const auto& Reloc : Relocations) {
      // relocs aren't cleared every time if IsGeneratingCache, so filter just in case
      if (Reloc.Header.Offset < CompiledCode.HostCodeOffset || Reloc.Header.Offset >= CompiledCode.HostCodeOffset + CompiledCode.Size) {
        continue;
      }
      // re-relocate :harold:
      uint32_t LocalOffset = uint32_t(Reloc.Header.Offset - CompiledCode.HostCodeOffset);

      switch (Reloc.Header.Type) {
      // it's important to zero-init the element completely so we don't have garbage in unused fields
      // this way, the caches stay deterministic across machines
      case CPU::RelocationTypes::RELOC_NAMED_SYMBOL_LITERAL: {
        BlobSmallRelocation SmallReloc = {};
        SmallReloc.Offset = LocalOffset;
        SmallReloc.Type = uint8_t(Reloc.Header.Type);
        SmallReloc.Named.Symbol = uint32_t(Reloc.NamedSymbolLiteral.Symbol);
        SmallRelocs.push_back(SmallReloc);
        break;
      }
      case CPU::RelocationTypes::RELOC_GUEST_RIP_LITERAL: {
        BlobSmallRelocation SmallReloc = {};
        SmallReloc.Offset = LocalOffset;
        SmallReloc.Type = uint8_t(Reloc.Header.Type);
        SmallReloc.RIPLiteral.GuestRIP = Reloc.GuestRIP.GuestRIP - GuestRIP;
        SmallRelocs.push_back(SmallReloc);
        break;
      }
      case CPU::RelocationTypes::RELOC_GUEST_RIP_MOVE: {
        BlobSmallRelocation SmallReloc = {};
        SmallReloc.Offset = LocalOffset;
        SmallReloc.Type = uint8_t(Reloc.Header.Type);
        SmallReloc.RIPMove.RegisterIndex = Reloc.GuestRIP.RegisterIndex;
        SmallReloc.RIPMove.GuestRIP = Reloc.GuestRIP.GuestRIP - GuestRIP;
        SmallRelocs.push_back(SmallReloc);
        break;
      }
      case CPU::RelocationTypes::RELOC_NAMED_THUNK_MOVE: {
        BlobThunkRelocation BigReloc = {};
        BigReloc.Offset = LocalOffset;
        BigReloc.RegisterIndex = Reloc.NamedThunkMove.RegisterIndex;
        memcpy(BigReloc.SymbolHash, &Reloc.NamedThunkMove.Symbol, sizeof(BigReloc.SymbolHash));
        ThunkRelocs.push_back(BigReloc);
        break;
      }
      }
    }

    // pack touched pages, relative to GuestRIP
    // in theory we could save some size here, unlikely we need all 64bits
    fextl::vector<int64_t> GuestPageOffsets;
    if (DecodedBlockInfo) {
      GuestPageOffsets.reserve(DecodedBlockInfo->CodePages.size());
      for (auto& GuestPage : DecodedBlockInfo->CodePages) {
        GuestPageOffsets.push_back(GuestPage - GuestRIP);
      }
    }

    uint64_t ModuleOffset = GuestRIP - Region.FileStartVA;

    // todo also copy/hash options that affect codegen into the key
    // todo should try to keep the key ascii i think?
    MesaFOZ::foz_payload_key Key = {};
    memcpy(Key.bytes, &ModuleOffset, sizeof(ModuleOffset));

    BlobFixedHeader Header {
      .GuestSize = (uint32_t)GuestCode.size(),
      .HostSize = (uint32_t)CompiledCode.Size,
      .EntryPointCount = (uint32_t)CacheEntryPoints.size(),
      .SmallRelocCount = (uint32_t)SmallRelocs.size(),
      .ThunkRelocCount = (uint32_t)ThunkRelocs.size(),
      .TouchedGuestPagesCount = (uint32_t)GuestPageOffsets.size(),
      .GuestHash = XXH3_128bits(GuestCode.data(), GuestCode.size()),
    };

    std::span<const uint8_t> BlobChunks[] = {
      {(const uint8_t*)&Header, sizeof(Header)},
      {(const uint8_t*)CompiledCode.BlockBegin, CompiledCode.Size},
      {(const uint8_t*)CacheEntryPoints.data(), CacheEntryPoints.size() * sizeof(BlobEntryPoint)},
      {(const uint8_t*)SmallRelocs.data(), SmallRelocs.size() * sizeof(BlobSmallRelocation)},
      {(const uint8_t*)ThunkRelocs.data(), ThunkRelocs.size() * sizeof(BlobThunkRelocation)},
      {(const uint8_t*)GuestPageOffsets.data(), GuestPageOffsets.size() * sizeof(int64_t)},
      GuestCode,
    };

    return RWCacheDB->StoreCacheBlob(Key, BlobChunks, Index);
  }

} // namespace DiskCache

} // namespace FEXCore