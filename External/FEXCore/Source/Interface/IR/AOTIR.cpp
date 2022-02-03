#include "Interface/Context/Context.h"
#include "Interface/IR/AOTIR.h"
#include "Interface/Core/LookupCache.h"

#include <FEXCore/IR/IntrusiveIRList.h>
#include <FEXCore/IR/RegisterAllocationData.h>
#include <FEXCore/Utils/Allocator.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <xxhash.h>

namespace FEXCore::IR {
  AOTIRInlineEntry *AOTIRInlineIndex::GetInlineEntry(uint64_t DataOffset) {
    uintptr_t This = (uintptr_t)this;

    return (AOTIRInlineEntry*)(This + DataBase + DataOffset);
  }

  AOTIRInlineEntry *AOTIRInlineIndex::Find(uint64_t GuestStart) {
    ssize_t l = 0;
    ssize_t r = Count - 1;

    while (l <= r) {
      size_t m = l + (r - l) / 2;

      if (Entries[m].GuestStart == GuestStart)
        return GetInlineEntry(Entries[m].DataOffset);
      else if (Entries[m].GuestStart < GuestStart)
        l = m + 1;
      else
        r = m - 1;
    }

    return nullptr;
  }

  IR::RegisterAllocationData *AOTIRInlineEntry::GetRAData() {
    return (IR::RegisterAllocationData *)InlineData;
  }

  IR::IRListView *AOTIRInlineEntry::GetIRData() {
    auto RAData = GetRAData();
    auto Offset = RAData->Size(RAData->MapCount);

    return (IR::IRListView *)&InlineData[Offset];
  }

  void AOTIRCaptureCacheEntry::AppendAOTIRCaptureCache(uint64_t GuestRIP, uint64_t Start, uint64_t Length, uint64_t Hash, FEXCore::IR::IRListView *IRList, FEXCore::IR::RegisterAllocationData *RAData) {
    auto Inserted = Index.emplace(GuestRIP, Stream->tellp());

    if (Inserted.second) {
      //GuestHash
      Stream->write((const char*)&Hash, sizeof(Hash));

      //GuestLength
      Stream->write((const char*)&Length, sizeof(Length));

      RAData->Serialize(*Stream);

      // IRData (inline)
      IRList->Serialize(*Stream);
    }
  }

  static bool readAll(int fd, void *data, size_t size) {
    int rv = read(fd, data, size);

    if (rv != size)
      return false;
    else
      return true;
  }

  bool LoadAOTIRCache(AOTCacheType *AOTIRCache, int streamfd) {
    uint64_t tag;

    if (!readAll(streamfd, (char*)&tag, sizeof(tag)) || tag != FEXCore::IR::AOTIR_COOKIE)
      return false;

    std::string Module;
    uint64_t ModSize;
    uint64_t IndexSize;

    lseek(streamfd, -sizeof(ModSize), SEEK_END);

    if (!readAll(streamfd,  (char*)&ModSize, sizeof(ModSize)))
      return false;

    Module.resize(ModSize);

    lseek(streamfd, -sizeof(ModSize) - ModSize, SEEK_END);

    if (!readAll(streamfd,  (char*)&Module[0], Module.size()))
      return false;

    lseek(streamfd, -sizeof(ModSize) - ModSize - sizeof(IndexSize), SEEK_END);

    if (!readAll(streamfd,  (char*)&IndexSize, sizeof(IndexSize)))
      return false;

    struct stat fileinfo;
    if (fstat(streamfd, &fileinfo) < 0)
      return false;
    size_t Size = (fileinfo.st_size + 4095) & ~4095;

    size_t IndexOffset = fileinfo.st_size - IndexSize -sizeof(ModSize) - ModSize - sizeof(IndexSize);

    void *FilePtr = FEXCore::Allocator::mmap(nullptr, Size, PROT_READ, MAP_SHARED, streamfd, 0);

    if (FilePtr == MAP_FAILED) {
      return false;
    }

    auto Array = (AOTIRInlineIndex *)((char*)FilePtr + IndexOffset);

    AOTIRCache->insert({Module, {Array, FilePtr, Size}});

    LogMan::Msg::DFmt("AOTIR: Module {} has {} functions", Module, Array->Count);

    return true;
  }

  AOTIRCaptureCache::~AOTIRCaptureCache() {
    for (auto &Mod: AOTIRCache) {
      FEXCore::Allocator::munmap(Mod.second.mapping, Mod.second.size);
    }
  }

  void AOTIRCaptureCache::FinalizeAOTIRCache() {
    AOTIRCaptureCacheWriteoutQueue_Flush();

    std::unique_lock lk(AOTIRCacheLock);

    for (auto& [String, Entry] : AOTIRCaptureCacheMap) {
      if (!Entry.Stream) {
        continue;
      }

      const auto ModSize = String.size();
      auto &stream = Entry.Stream;

      // pad to 32 bytes
      constexpr char Zero = 0;
      while(stream->tellp() & 31)
        stream->write(&Zero, 1);

      // AOTIRInlineIndex
      const auto FnCount = Entry.Index.size();
      const size_t DataBase = -stream->tellp();

      stream->write((const char*)&FnCount, sizeof(FnCount));
      stream->write((const char*)&DataBase, sizeof(DataBase));

      for (const auto& [GuestStart, DataOffset] : Entry.Index) {
        //AOTIRInlineIndexEntry

        // GuestStart
        stream->write((const char*)&GuestStart, sizeof(GuestStart));

        // DataOffset
        stream->write((const char*)&DataOffset, sizeof(DataOffset));
      }

      // End of file header
      const auto IndexSize = FnCount * sizeof(FEXCore::IR::AOTIRInlineIndexEntry) + sizeof(DataBase) + sizeof(FnCount);
      stream->write((const char*)&IndexSize, sizeof(IndexSize));
      stream->write(String.c_str(), ModSize);
      stream->write((const char*)&ModSize, sizeof(ModSize));

      // Close the stream
      stream->close();

      // Rename the file to atomically update the cache with the temporary file
      AOTIRRenamer(String);
    }
  }

  void AOTIRCaptureCache::AOTIRCaptureCacheWriteoutQueue_Flush() {
    {
      std::shared_lock lk{AOTIRCaptureCacheWriteoutLock};
      if (AOTIRCaptureCacheWriteoutQueue.size() == 0) {
        AOTIRCaptureCacheWriteoutFlusing.store(false);
        return;
      }
    }

    for (;;) {
      AOTIRCaptureCacheWriteoutLock.lock();
      std::function<void()> fn = std::move(AOTIRCaptureCacheWriteoutQueue.front());
      bool MaybeEmpty = false;
      AOTIRCaptureCacheWriteoutQueue.pop();
      MaybeEmpty = AOTIRCaptureCacheWriteoutQueue.size() == 0;
      AOTIRCaptureCacheWriteoutLock.unlock();

      fn();
      if (MaybeEmpty) {
        std::shared_lock lk{AOTIRCaptureCacheWriteoutLock};
        if (AOTIRCaptureCacheWriteoutQueue.size() == 0) {
          AOTIRCaptureCacheWriteoutFlusing.store(false);
          return;
        }
      }
    }

    LOGMAN_MSG_A_FMT("Must never get here");
  }

  void AOTIRCaptureCache::AOTIRCaptureCacheWriteoutQueue_Append(const std::function<void()> &fn) {
    bool Flush = false;

    {
      std::unique_lock lk{AOTIRCaptureCacheWriteoutLock};
      AOTIRCaptureCacheWriteoutQueue.push(fn);
      if (AOTIRCaptureCacheWriteoutQueue.size() > 10000) {
        Flush = true;
      }
    }

    bool test_val = false;
    if (Flush && AOTIRCaptureCacheWriteoutFlusing.compare_exchange_strong(test_val, true)) {
      AOTIRCaptureCacheWriteoutQueue_Flush();
    }
  }

  void AOTIRCaptureCache::WriteFilesWithCode(std::function<void(const std::string& fileid, const std::string& filename)> Writer) {
    std::shared_lock lk(AOTIRCacheLock);
    for( const auto &File: FilesWithCode) {
      Writer(File.first, File.second);
    }
  }

  AOTIRCaptureCache::PreGenerateIRFetchResult AOTIRCaptureCache::PreGenerateIRFetch(uint64_t GuestRIP, FEXCore::IR::IRListView *IRList) {
    {
      std::shared_lock lk(AOTIRCacheLock);
      auto file = AddrToFile.lower_bound(GuestRIP);
      if (file != AddrToFile.begin()) {
        --file;
        if (!file->second.ContainsCode) {
          file->second.ContainsCode = true;
          FilesWithCode[file->second.fileid] = file->second.filename;
        }
      }
    }

    PreGenerateIRFetchResult Result{};
    if (IRList == nullptr && CTX->Config.AOTIRLoad()) {
      std::shared_lock lk(AOTIRCacheLock);
      auto file = AddrToFile.lower_bound(GuestRIP);
      if (file != AddrToFile.begin()) {
        --file;
        auto Mod = (FEXCore::IR::AOTIRInlineIndex*)file->second.CachedFileEntry;

        if (Mod == nullptr) {
          file->second.CachedFileEntry = Mod = AOTIRCache[file->second.fileid].Array;
        }

        if (Mod != nullptr)
        {
          auto AOTEntry = Mod->Find(GuestRIP - file->second.Start + file->second.Offset);

          if (AOTEntry) {
            // verify hash
            auto MappedStart = GuestRIP;
            auto hash = XXH3_64bits((void*)MappedStart, AOTEntry->GuestLength);
            if (hash == AOTEntry->GuestHash) {
              Result.IRList = AOTEntry->GetIRData();
              //LogMan::Msg::DFmt("using {} + {:x} -> {:x}\n", file->second.fileid, AOTEntry->first, GuestRIP);

              Result.RAData = AOTEntry->GetRAData();;
              Result.DebugData = new FEXCore::Core::DebugData();
              Result.StartAddr = MappedStart;
              Result.Length = AOTEntry->GuestLength;
              Result.GeneratedIR = true;
            } else {
              LogMan::Msg::IFmt("AOTIR: hash check failed {:x}\n", MappedStart);
            }
          } else {
            //LogMan::Msg::IFmt("AOTIR: Failed to find {:x}, {:x}, {}\n", GuestRIP, GuestRIP - file->second.Start + file->second.Offset, file->second.fileid);
          }
        }
      }
    }

    return Result;
  }

  bool AOTIRCaptureCache::PostCompileCode(
    FEXCore::Core::InternalThreadState *Thread,
    void* CodePtr,
    uint64_t GuestRIP,
    uint64_t StartAddr,
    uint64_t Length,
    FEXCore::IR::RegisterAllocationData *RAData,
    FEXCore::IR::IRListView *IRList,
    FEXCore::Core::DebugData *DebugData,
    bool GeneratedIR) {
    // Both generated ir and LibraryJITName need a named region lookup
    if (GeneratedIR || CTX->Config.LibraryJITNaming()) {
      std::shared_lock lk(AOTIRCacheLock);

      auto file = FindAddrForFile(StartAddr, Length);

      // Only go down this path if we actually found a library region
      if (file != AddrToFile.end()) {
        if (DebugData && CTX->Config.LibraryJITNaming()) {
          CTX->Symbols.RegisterNamedRegion(CodePtr, DebugData->HostCodeSize, file->second.filename);
        }

        // Add to AOT cache if aot generation is enabled
        if (GeneratedIR && RAData &&
            (CTX->Config.AOTIRCapture() || CTX->Config.AOTIRGenerate())) {

          auto hash = XXH3_64bits((void*)StartAddr, Length);

          auto LocalRIP = GuestRIP - file->second.Start + file->second.Offset;
          auto LocalStartAddr = StartAddr - file->second.Start + file->second.Offset;
          auto fileid = file->second.fileid;
          auto RADataCopy = RAData->CreateCopy();
          auto IRListCopy = IRList->CreateCopy();
          AOTIRCaptureCacheWriteoutQueue_Append([this, LocalRIP, LocalStartAddr, Length, hash, IRListCopy, RADataCopy, fileid]() {
            auto *AotFile = &AOTIRCaptureCacheMap[fileid];

            if (!AotFile->Stream) {
              AotFile->Stream = AOTIRWriter(fileid);
              uint64_t tag = FEXCore::IR::AOTIR_COOKIE;
              AotFile->Stream->write((char*)&tag, sizeof(tag));
            }
            AotFile->AppendAOTIRCaptureCache(LocalRIP, LocalStartAddr, Length, hash, IRListCopy, RADataCopy);
            FEXCore::Allocator::free(RADataCopy);
            delete IRListCopy;
          });

          if (CTX->Config.AOTIRGenerate()) {
            // cleanup memory and early exit here -- we're not running the application
            Thread->CPUBackend->ClearCache();
            return true;
          }
        }
      }

      // Insert to caches if we generated IR
      if (GeneratedIR) {
        if (Thread->CPUBackend->NeedsRetainedIRCopy()) {
          // Add to thread local ir cache
          Core::LocalIREntry Entry = {StartAddr, Length, decltype(Entry.IR)(IRList), decltype(Entry.RAData)(RAData), decltype(Entry.DebugData)(DebugData)};
          
          std::lock_guard<std::recursive_mutex> lk(Thread->LookupCache->WriteLock);
          Thread->LocalIRCache.insert({GuestRIP, std::move(Entry)});
        }
        else {
          // If the IR doesn't need to be retained then we can just delete it now
          delete DebugData;
          FEXCore::Allocator::free(RAData);
          delete IRList;
        }
      }
    }

    return false;
  }

  AOTIRCaptureCache::AddrToFileMapType::iterator AOTIRCaptureCache::FindAddrForFile(uint64_t Entry, uint64_t Length) {
    // Thread safety here! We are returning an iterator to the map object
    // This needs the AOTIRCacheLock locked prior to coming in to the function
    auto file = AddrToFile.lower_bound(Entry);
    if (file != AddrToFile.begin()) {
      --file;
      if (file->second.Start <= Entry && (file->second.Start + file->second.Len) >= (Entry + Length)) {
        return file;
      }
    }
    return AddrToFile.end();
  }

  void AOTIRCaptureCache::AddNamedRegion(uintptr_t Base, uintptr_t Size, uintptr_t Offset, const std::string &filename) {
    // TODO: Support overlapping maps and region splitting
    auto base_filename = std::filesystem::path(filename).filename().string();

    if (!base_filename.empty()) {
      auto filename_hash = XXH3_64bits(filename.c_str(), filename.size());

      auto fileid = base_filename + "-" + std::to_string(filename_hash) + "-";

      // append optimization flags to the fileid
      fileid += (CTX->Config.SMCChecks == FEXCore::Config::CONFIG_SMC_FULL) ? "S" : "s";
      fileid += CTX->Config.TSOEnabled ? "T" : "t";
      fileid += CTX->Config.ABILocalFlags ? "L" : "l";
      fileid += CTX->Config.ABINoPF ? "p" : "P";

      std::unique_lock lk(AOTIRCacheLock);

      AddrToFile.insert({ Base, { Base, Size, Offset, fileid, filename, nullptr, false} });

      if (CTX->Config.AOTIRLoad && !AOTIRCache.contains(fileid) && AOTIRLoader) {
        auto streamfd = AOTIRLoader(fileid);
        if (streamfd != -1) {
          FEXCore::IR::LoadAOTIRCache(&AOTIRCache, streamfd);
          close(streamfd);
        }
      }
    }
  }

  void AOTIRCaptureCache::RemoveNamedRegion(uintptr_t Base, uintptr_t Size) {
    std::unique_lock lk(AOTIRCacheLock);
    // TODO: Support partial removing
    AddrToFile.erase(Base);
  }
}
