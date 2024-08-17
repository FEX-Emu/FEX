// SPDX-License-Identifier: MIT
#include "FEXHeaderUtils/Filesystem.h"
#include "Interface/Context/Context.h"
#include "Interface/IR/AOTIR.h"
#include "Interface/IR/IntrusiveIRList.h"
#include "Interface/IR/RegisterAllocationData.h"

#include <FEXCore/Utils/Allocator.h>
#include <FEXCore/HLE/SyscallHandler.h>
#include <FEXCore/fextl/fmt.h>
#include <FEXCore/fextl/string.h>

#include <Interface/Core/LookupCache.h>

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <sys/stat.h>
#include <unistd.h>
#include <xxhash.h>


namespace FEXCore::IR {
AOTIRInlineEntry* AOTIRInlineIndex::GetInlineEntry(uint64_t DataOffset) {
  uintptr_t This = (uintptr_t)this;

  return (AOTIRInlineEntry*)(This + DataBase + DataOffset);
}

AOTIRInlineEntry* AOTIRInlineIndex::Find(uint64_t GuestStart) {
  ssize_t l = 0;
  ssize_t r = Count - 1;

  while (l <= r) {
    size_t m = l + (r - l) / 2;

    if (Entries[m].GuestStart == GuestStart) {
      return GetInlineEntry(Entries[m].DataOffset);
    } else if (Entries[m].GuestStart < GuestStart) {
      l = m + 1;
    } else {
      r = m - 1;
    }
  }

  return nullptr;
}

IR::RegisterAllocationData* AOTIRInlineEntry::GetRAData() {
  return (IR::RegisterAllocationData*)InlineData;
}

IR::IRListView* AOTIRInlineEntry::GetIRData() {
  auto RAData = GetRAData();
  auto Offset = RAData->Size(RAData->MapCount);

  return (IR::IRListView*)&InlineData[Offset];
}

void AOTIRCaptureCacheEntry::AppendAOTIRCaptureCache(uint64_t GuestRIP, uint64_t Start, uint64_t Length, uint64_t Hash,
                                                     const FEXCore::IR::IRListView& IRList, const FEXCore::IR::RegisterAllocationData* RAData) {
  auto Inserted = Index.emplace(GuestRIP, Stream->Offset());

  if (Inserted.second) {
    AOTIRInlineEntry entry {
      .GuestHash = Hash,
      .GuestLength = Length,
    };
    Stream->Write((const char*)&entry, sizeof(entry));

    RAData->Serialize(*Stream);

    // IRData (inline)
    IRList.Serialize(*Stream);
  }
}

static bool readAll(int fd, void* data, size_t size) {
  int rv = read(fd, data, size);

  if (rv != size) {
    return false;
  } else {
    return true;
  }
}

static bool LoadAOTIRCache(AOTIRCacheEntry* Entry, int streamfd) {
#ifndef _WIN32
  uint64_t tag;

  if (!readAll(streamfd, (char*)&tag, sizeof(tag)) || tag != FEXCore::IR::AOTIR_COOKIE) {
    return false;
  }

  fextl::string Module;
  uint64_t ModSize;
  uint64_t IndexSize;

  lseek(streamfd, -sizeof(ModSize), SEEK_END);

  if (!readAll(streamfd, (char*)&ModSize, sizeof(ModSize))) {
    return false;
  }

  Module.resize(ModSize);

  lseek(streamfd, -sizeof(ModSize) - ModSize, SEEK_END);

  if (!readAll(streamfd, (char*)&Module[0], Module.size())) {
    return false;
  }

  if (Entry->FileId != Module) {
    return false;
  }

  lseek(streamfd, -sizeof(ModSize) - ModSize - sizeof(IndexSize), SEEK_END);

  if (!readAll(streamfd, (char*)&IndexSize, sizeof(IndexSize))) {
    return false;
  }

  struct stat fileinfo;
  if (fstat(streamfd, &fileinfo) < 0) {
    return false;
  }
  size_t Size = (fileinfo.st_size + 4095) & ~4095;

  size_t IndexOffset = fileinfo.st_size - IndexSize - sizeof(ModSize) - ModSize - sizeof(IndexSize);

  void* FilePtr = FEXCore::Allocator::mmap(nullptr, Size, PROT_READ, MAP_SHARED, streamfd, 0);

  if (FilePtr == MAP_FAILED) {
    return false;
  }

  auto Array = (AOTIRInlineIndex*)((char*)FilePtr + IndexOffset);

  LOGMAN_THROW_AA_FMT(Entry->Array == nullptr && Entry->FilePtr == nullptr, "Entry must not be initialized here");
  Entry->Array = Array;
  Entry->FilePtr = FilePtr;
  Entry->Size = Size;

  LogMan::Msg::DFmt("AOTIR: Module {} has {} functions", Module, Array->Count);

  return true;
#else
  return false;
#endif
}

void AOTIRCaptureCache::FinalizeAOTIRCache() {
  AOTIRCaptureCacheWriteoutQueue_Flush();

  std::unique_lock lk(AOTIRCacheLock);

  for (auto& [String, Entry] : AOTIRCaptureCacheMap) {
    if (!Entry.Stream) {
      continue;
    }

    const auto ModSize = String.size();
    auto& stream = Entry.Stream;

    // pad to 32 bytes
    constexpr char Zero = 0;
    while (stream->Offset() & 31) {
      stream->Write(&Zero, 1);
    }

    AOTIRInlineIndex index {
      .Count = Entry.Index.size(),
      .DataBase = -stream->Offset(),
    };
    stream->Write((const char*)&index, sizeof(index));

    for (const auto& [GuestStart, DataOffset] : Entry.Index) {
      AOTIRInlineIndexEntry entry {
        .GuestStart = GuestStart,
        .DataOffset = DataOffset,
      };

      stream->Write((const char*)&entry, sizeof(entry));
    }

    // End of file header
    const auto IndexSize = sizeof(AOTIRInlineIndex) + index.Count * sizeof(FEXCore::IR::AOTIRInlineIndexEntry);
    stream->Write((const char*)&IndexSize, sizeof(IndexSize));
    stream->Write(String.c_str(), ModSize);
    stream->Write((const char*)&ModSize, sizeof(ModSize));

    // Close the stream
    stream->Close();

    // Rename the file to atomically update the cache with the temporary file
    AOTIRRenamer(String);
  }
}

void AOTIRCaptureCache::AOTIRCaptureCacheWriteoutQueue_Flush() {
  {
    std::shared_lock lk {AOTIRCaptureCacheWriteoutLock};
    if (AOTIRCaptureCacheWriteoutQueue.size() == 0) {
      AOTIRCaptureCacheWriteoutFlusing.store(false);
      return;
    }
  }

  for (;;) {
    // This code is tricky to refactor so it doesn't allocate memory through glibc.
    // The moved std::function object deallocates memory at the end of scope.
    FEXCore::Allocator::YesIKnowImNotSupposedToUseTheGlibcAllocator glibc;

    AOTIRCaptureCacheWriteoutLock.lock();
    WriteOutFn fn = std::move(AOTIRCaptureCacheWriteoutQueue.front());
    bool MaybeEmpty = false;
    AOTIRCaptureCacheWriteoutQueue.pop();
    MaybeEmpty = AOTIRCaptureCacheWriteoutQueue.size() == 0;
    AOTIRCaptureCacheWriteoutLock.unlock();

    fn();
    if (MaybeEmpty) {
      std::shared_lock lk {AOTIRCaptureCacheWriteoutLock};
      if (AOTIRCaptureCacheWriteoutQueue.size() == 0) {
        AOTIRCaptureCacheWriteoutFlusing.store(false);
        return;
      }
    }
  }

  LOGMAN_MSG_A_FMT("Must never get here");
}

void AOTIRCaptureCache::AOTIRCaptureCacheWriteoutQueue_Append(const WriteOutFn& fn) {
  bool Flush = false;

  {
    std::unique_lock lk {AOTIRCaptureCacheWriteoutLock};
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

void AOTIRCaptureCache::WriteFilesWithCode(const Context::AOTIRCodeFileWriterFn& Writer) {
  std::shared_lock lk(AOTIRCacheLock);
  for (const auto& Entry : AOTIRCache) {
    if (Entry.second.ContainsCode) {
      Writer(Entry.second.FileId, Entry.second.Filename);
    }
  }
}

// IRStorageBase with memory owned by IR cache
class IRInlineStorage : public IRStorageBase {
  AOTIRInlineEntry& entry;
public:
  IRInlineStorage(AOTIRInlineEntry& entry)
    : entry(entry) {}
  const RegisterAllocationData* RAData() override {
    return entry.GetRAData();
  }
  IRListView GetIRView() override {
    return entry.GetIRData();
  }
};


std::optional<AOTIRCaptureCache::PreGenerateIRFetchResult>
AOTIRCaptureCache::PreGenerateIRFetch(FEXCore::Core::InternalThreadState* Thread, uint64_t GuestRIP) {
  auto AOTIRCacheEntry = CTX->SyscallHandler->LookupAOTIRCacheEntry(Thread, GuestRIP);

  PreGenerateIRFetchResult Result {};

  if (AOTIRCacheEntry.Entry) {
    AOTIRCacheEntry.Entry->ContainsCode = true;

    if (CTX->Config.AOTIRLoad()) {
      auto Mod = AOTIRCacheEntry.Entry->Array;

      if (Mod != nullptr) {
        auto AOTEntry = Mod->Find(GuestRIP - AOTIRCacheEntry.VAFileStart);

        if (AOTEntry) {
          // verify hash
          auto MappedStart = GuestRIP;
          auto hash = XXH3_64bits((void*)MappedStart, AOTEntry->GuestLength);
          if (hash == AOTEntry->GuestHash) {
            Result.IR = fextl::make_unique<IRInlineStorage>(*AOTEntry);
            // LogMan::Msg::DFmt("using {} + {:x} -> {:x}\n", file->second.fileid, AOTEntry->first, GuestRIP);
            Result.DebugData = new FEXCore::Core::DebugData();
            Result.StartAddr = MappedStart;
            Result.Length = AOTEntry->GuestLength;
            return Result;
          } else {
            LogMan::Msg::IFmt("AOTIR: hash check failed {:x}\n", MappedStart);
          }
        } else {
          // LogMan::Msg::IFmt("AOTIR: Failed to find {:x}, {:x}, {}\n", GuestRIP, GuestRIP - file->second.Start + file->second.Offset, file->second.fileid);
        }
      }
    }
  }

  return std::nullopt;
}

bool AOTIRCaptureCache::PostCompileCode(FEXCore::Core::InternalThreadState* Thread, void* CodePtr, uint64_t GuestRIP, uint64_t StartAddr,
                                        uint64_t Length, fextl::unique_ptr<FEXCore::IR::IRStorageBase> IR,
                                        FEXCore::Core::DebugData* DebugData, bool GeneratedIR) {

  // Both generated ir and LibraryJITName need a named region lookup
  if (GeneratedIR || CTX->Config.LibraryJITNaming()) {

    auto AOTIRCacheEntry = CTX->SyscallHandler->LookupAOTIRCacheEntry(Thread, GuestRIP);

    if (AOTIRCacheEntry.Entry) {
      if (DebugData && CTX->Config.LibraryJITNaming()) {
        CTX->Symbols.RegisterNamedRegion(Thread->SymbolBuffer.get(), CodePtr, DebugData->HostCodeSize, AOTIRCacheEntry.Entry->Filename);
      }

      // Add to AOT cache if aot generation is enabled
      if (GeneratedIR && IR->RAData() && (CTX->Config.AOTIRCapture() || CTX->Config.AOTIRGenerate())) {

        auto hash = XXH3_64bits((void*)StartAddr, Length);

        auto LocalRIP = GuestRIP - AOTIRCacheEntry.VAFileStart;
        auto LocalStartAddr = StartAddr - AOTIRCacheEntry.VAFileStart;
        auto FileId = AOTIRCacheEntry.Entry->FileId;

        // The lambda is converted to std::function. This is tricky to refactor so it doesn't allocate memory through glibc.
        // NOTE: unique_ptr must be passed as a raw pointer since std::function requires lambda captures to be copyable
        FEXCore::Allocator::YesIKnowImNotSupposedToUseTheGlibcAllocator glibc;
        AOTIRCaptureCacheWriteoutQueue_Append([this, LocalRIP, LocalStartAddr, Length, hash, IRRaw = IR.release(), FileId]() {
          fextl::unique_ptr<FEXCore::IR::IRStorageBase> IR(IRRaw);

          // It is guaranteed via AOTIRCaptureCacheWriteoutLock and AOTIRCaptureCacheWriteoutFlusing that this will not run concurrently
          // Memory coherency is guaranteed via AOTIRCaptureCacheWriteoutLock

          auto* AotFile = &AOTIRCaptureCacheMap[FileId];

          if (!AotFile->Stream) {
            AotFile->Stream = AOTIRWriter(FileId);
            uint64_t tag = FEXCore::IR::AOTIR_COOKIE;
            AotFile->Stream->Write(&tag, sizeof(tag));
          }
          AotFile->AppendAOTIRCaptureCache(LocalRIP, LocalStartAddr, Length, hash, IR->GetIRView(), IR->RAData());
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
      // If the IR doesn't need to be retained then we can just delete it now
      delete DebugData;
    }
  }

  return false;
}

AOTIRCacheEntry* AOTIRCaptureCache::LoadAOTIRCacheEntry(const fextl::string& filename) {
  fextl::string base_filename = FHU::Filesystem::GetFilename(filename);

  if (!base_filename.empty()) {
    auto filename_hash = XXH3_64bits(filename.c_str(), filename.size());

    auto fileid = fextl::fmt::format("{}-{}-{}{}{}", base_filename, filename_hash,
                                     (CTX->Config.SMCChecks == FEXCore::Config::CONFIG_SMC_FULL) ? 'S' : 's',
                                     CTX->Config.TSOEnabled ? 'T' : 't', CTX->Config.ABILocalFlags ? 'L' : 'l');

    std::unique_lock lk(AOTIRCacheLock);

    auto Inserted = AOTIRCache.insert({fileid, AOTIRCacheEntry {.FileId = fileid, .Filename = filename}});
    auto Entry = &(Inserted.first->second);

    LOGMAN_THROW_AA_FMT(Entry->Array == nullptr, "Duplicate LoadAOTIRCacheEntry");

    if (CTX->Config.AOTIRLoad && AOTIRLoader) {
      auto streamfd = AOTIRLoader(fileid);
      if (streamfd != -1) {
        FEXCore::IR::LoadAOTIRCache(Entry, streamfd);
        close(streamfd);
      }
    }
    return Entry;
  }

  return nullptr;
}

void AOTIRCaptureCache::UnloadAOTIRCacheEntry(AOTIRCacheEntry* Entry) {
#ifndef _WIN32
  LOGMAN_THROW_AA_FMT(Entry != nullptr, "Removing not existing entry");

  if (Entry->Array) {
    FEXCore::Allocator::munmap(Entry->FilePtr, Entry->Size);
    Entry->Array = nullptr;
    Entry->FilePtr = nullptr;
    Entry->Size = 0;
  }
#endif
}
} // namespace FEXCore::IR
