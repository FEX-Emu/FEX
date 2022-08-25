#include "FEXCore/Utils/LogManager.h"
#include "FEXCore/Utils/MathUtils.h"
#include "FEXHeaderUtils/TypeDefines.h"
#include "Interface/Context/Context.h"
#include "Interface/Core/CodeCache/CodeCache.h"
#include "Interface/Core/CodeCache/IRCache.h"

#include <FEXCore/IR/IntrusiveIRList.h>
#include <FEXCore/IR/RegisterAllocationData.h>
#include <FEXCore/Utils/Allocator.h>
#include <FEXCore/HLE/SyscallHandler.h>
#include <Interface/Core/LookupCache.h>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <xxhash.h>

#define VERBOSE_LOG(...) // LogMan::Msg::DFmt
#define VERBOSE_LOG2(...) //LogMan::Msg::DFmt

static void LockFD(int fd) {
  struct flock fl;

  fl.l_type   = F_WRLCK;
  fl.l_whence = SEEK_SET;
  fl.l_start  = 0;
  fl.l_len    = 0;
  fl.l_pid    = getpid();

  fcntl(fd, F_SETLKW, &fl);
}

static void UnlockFD(int fd) {
  struct flock fl;

  fl.l_type   = F_UNLCK;
  fl.l_whence = SEEK_SET;
  fl.l_start  = 0;
  fl.l_len    = 0;
  fl.l_pid    = getpid();

  fcntl(fd, F_SETLKW, &fl);
}

// Last 16 bits are used for time
static constexpr uint64_t DATA_OFFSET_TIME_BASE = UINT64_MAX - 65535;


namespace FEXCore {
  std::unique_ptr<CodeCache> CodeCache::LoadFile(int IndexFD, int DataFD, const uint64_t IndexCookie, const uint64_t DataCookie) {

    auto rv = std::unique_ptr<CodeCache>(new CodeCache());

    rv->IndexFD = IndexFD;
    rv->DataFD = DataFD;

    struct stat IndexStat;
    fstat(IndexFD, &IndexStat);

    rv->IndexFileSize = AlignUp(sizeof(*rv->Index), FHU::FEX_PAGE_SIZE);
    
    if (IndexStat.st_size > rv->IndexFileSize) {
      rv->IndexFileSize = IndexStat.st_size;
    }

    fallocate(IndexFD, 0, 0, rv->IndexFileSize);
    rv->Index = (decltype(rv->Index))FEXCore::Allocator::mmap(nullptr, rv->IndexFileSize, PROT_READ | PROT_WRITE, MAP_SHARED, IndexFD, 0);

    LOGMAN_THROW_A_FMT(rv->Index != MAP_FAILED, "initial Index mmap failed {} {} {}", (void*)rv->Index, rv->IndexFileSize.load(), IndexFD);

    auto DataMapSize = AlignUp(sizeof(*rv->Data), FHU::FEX_PAGE_SIZE);
    fallocate(DataFD, 0, 0, DataMapSize);
    rv->Data = (decltype(rv->Data))FEXCore::Allocator::mmap(nullptr, DataMapSize, PROT_READ | PROT_WRITE, MAP_SHARED, DataFD, 0);

    LOGMAN_THROW_A_FMT(rv->Data != MAP_FAILED, "initial Data mmap failed {} {} {}", (void*)rv->Index, DataMapSize, DataFD);

    LockFD(IndexFD);

    if (rv->Index->Tag != IndexCookie || rv->Data->Tag != DataCookie) {
      // regenerate files 

      // Index file
      rv->Index->Tag = IndexCookie;
      rv->Index->FileSize = rv->IndexFileSize.load();
      rv->Index->Count = 0;

      // Data file
      rv->Data->Tag = DataCookie;
      rv->Data->ChunksUsed = 0;
      rv->Data->CurrentChunkFree = 0;
      rv->Data->WritePointer = AlignUp(DataMapSize, FHU::FEX_PAGE_SIZE);
    }

    UnlockFD(IndexFD);

    auto NChunks = rv->Data->ChunksUsed.load();
    
    rv->MappedDataChunks.resize(NChunks);
    
    for (decltype(NChunks) i = 0; i < NChunks; i++) {
      rv->MappedDataChunks[i] = (uint8_t*) FEXCore::Allocator::mmap(nullptr, CHUNK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, rv->DataFD, rv->Data->ChunkOffsets[i]);
    }

    return rv;
  }

  CodeCache::~CodeCache() {
    // unmap
    FEXCore::Allocator::munmap(Index, IndexFileSize);
    FEXCore::Allocator::munmap(Data, sizeof(*Data));

    for (const auto Ptr: MappedDataChunks) {
      if (Ptr) {
        FEXCore::Allocator::munmap(Ptr, CHUNK_SIZE);
      }
    }

    // close files
    close(IndexFD);
    close(DataFD);
  }

  void CodeCache::MapDataChunkUnsafe(uint64_t ChunkNum) {
    if (MappedDataChunks.size() <= ChunkNum || !MappedDataChunks[ChunkNum]) {
      MappedDataChunks.resize(Data->ChunksUsed.load());

      auto v = (uint8_t *)FEXCore::Allocator::mmap(nullptr, CHUNK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, DataFD, Data->ChunkOffsets[ChunkNum]);
      LOGMAN_THROW_A_FMT(v != MAP_FAILED, "mmap failed {} {} {}", (void *)v, CHUNK_SIZE, Data->ChunkOffsets[ChunkNum]);

      MappedDataChunks[ChunkNum] = v;
    }
  }

  CacheEntry *CodeCache::Find(uint64_t OffsetRIP, uint64_t GuestRIP) {

    CacheEntry *CacheEntry = nullptr;

    // Read the count before any remap
    auto Count = Index->Count.load();

    if (Index->FileSize > IndexFileSize) {
      std::lock_guard lk {Mutex};
      auto OldSize = IndexFileSize.load();
      IndexFileSize = Index->FileSize.load();
      // FEXCore::Allocator:: is missing this one
      //Index = (decltype(Index))FEXCore::Allocator::mremap(Index, OldSize, IndexFileSize, MREMAP_MAYMOVE);
      FEXCore::Allocator::munmap(Index, OldSize);
      Index = (decltype(Index))FEXCore::Allocator::mmap(nullptr, IndexFileSize, PROT_READ | PROT_WRITE, MAP_SHARED, IndexFD, 0);
      
      LogMan::Msg::DFmt("remapped Index: {}, OldSize: {}, IndexFileSize: {}", (void*)Index, OldSize, IndexFileSize.load());
      LOGMAN_THROW_A_FMT(Index != MAP_FAILED, "mremap failed {} {} {}", (void*)Index, OldSize, IndexFileSize.load());
    }

    {
      std::shared_lock lk{Mutex};

      size_t m = 0;

      while (m < Count) {
        VERBOSE_LOG2("Looking {} {:x} l:{} r:{}", m, Index->Entries[m].GuestStart, Index->Entries[m].Left, Index->Entries[m].Right);
        if (Index->Entries[m].GuestStart == OffsetRIP) {
          auto DataOffset = Index->Entries[m].DataOffset.load();

          if (DataOffset >= DATA_OFFSET_TIME_BASE) {
            // This entry is not fully inserted yet
            return nullptr;
          }

          auto ChunkNum = DataOffset / CHUNK_SIZE;
          auto ChunkOffs = DataOffset % CHUNK_SIZE;

          if (MappedDataChunks.size() <= ChunkNum || !MappedDataChunks[ChunkNum]) {
            // Upgrade to a unique lock here and re-test
            lk.unlock();
            std::lock_guard ulk{Mutex};
            MapDataChunkUnsafe(ChunkNum);
          }

          CacheEntry = (decltype(CacheEntry))(MappedDataChunks[ChunkNum] + ChunkOffs);

          VERBOSE_LOG("Found {:x} {:x} in index {} {}", GuestRIP, OffsetRIP, (void*)CacheEntry, CacheEntry->GuestRangeCount);
          break;
        } else if (Index->Entries[m].GuestStart < OffsetRIP) {
          m = Index->Entries[m].Left;
        } else {
          m = Index->Entries[m].Right;
        }
      }
    }

    if (!CacheEntry) {
      return nullptr;
    }

    // verify hash
    uint64_t hash = 0;
    auto Ranges = CacheEntry->GetRangeData();
    for (size_t i = 0; i < CacheEntry->GuestRangeCount; i++){ 
      hash = XXH3_64bits_withSeed((void*)(GuestRIP + Ranges[i].start), Ranges[i].length, hash);
    }

    if (hash != CacheEntry->GuestHash) {
      LogMan::Msg::IFmt("CodeCache: hash check failed {:x}\n", GuestRIP);
      return nullptr;
    }

    VERBOSE_LOG("Found {:x} {:x} in cache", GuestRIP, OffsetRIP);
    return CacheEntry;
  }

  void CodeCache::Insert(uint64_t OffsetRIP, uint64_t GuestRIP, uint64_t InlineSize, const std::function<void(CacheEntry *CacheEntry)> &Fill) {
    
    uint32_t NewEntry;
    uint64_t NewEntryValue;

    // See if it exists in the index, and make a placeholder entry if it does
    {
      // process wide lock
      std::lock_guard lk {Mutex};

      // Index File lock
      LockFD(IndexFD);

      // Resize Index file if needed
      {
        auto NewFileSize = sizeof(CacheIndex) + (Index->Count + 1) * sizeof(CacheIndexEntry);

        if (NewFileSize > Index->FileSize) {
          LogMan::Msg::DFmt("resize Index: NewSize: {}, Index->FileSize: {}", Index->FileSize + INDEX_CHUNK_SIZE, Index->FileSize.load());
          fallocate(IndexFD, 0, Index->FileSize.load(), INDEX_CHUNK_SIZE);
          Index->FileSize += INDEX_CHUNK_SIZE;
        }
      }
      
      // Make sure we have all of the index mapped
      if (Index->FileSize > IndexFileSize) {
        auto OldSize = IndexFileSize.load();
        IndexFileSize = Index->FileSize.load();

        // FEXCore::Allocator:: is missing this one
        //Index = (decltype(Index))FEXCore::Allocator::mremap(Index, OldSize, IndexFileSize, MREMAP_MAYMOVE);
        FEXCore::Allocator::munmap(Index, OldSize);
        Index = (decltype(Index))FEXCore::Allocator::mmap(nullptr, IndexFileSize, PROT_READ | PROT_WRITE, MAP_SHARED, IndexFD, 0);

        LogMan::Msg::DFmt("remapped Index: {}, OldSize: {}, IndexFileSize: {}", (void*)Index, OldSize, IndexFileSize.load());
        LOGMAN_THROW_A_FMT(Index != MAP_FAILED, "mremap failed {:x} {} {}", (void*)Index, OldSize, IndexFileSize.load());
      }

      // Make sure entry doesn't exist & find insert point
      size_t InsertPoint = SIZE_MAX;

      NewEntry = Index->Count.load();

      {
        size_t m = 0;
        auto Count = Index->Count.load();

        while (m < Count) {
          InsertPoint = m;
          VERBOSE_LOG2("Insert Looking {} {} {:x} l:{} r:{}", (uint8_t*)(&Index->Entries[m]) - (uint8_t*)Index, m, Index->Entries[m].GuestStart, Index->Entries[m].Left, Index->Entries[m].Right);

          if (Index->Entries[m].GuestStart == OffsetRIP) {

            auto DataOffset = Index->Entries[m].DataOffset.load();

            if (DataOffset >= DATA_OFFSET_TIME_BASE) {
              auto now = std::time(nullptr) | DATA_OFFSET_TIME_BASE;
              auto then = DataOffset;
              
              if (now - then >= 2 && Index->Entries[m].DataOffset.compare_exchange_strong(DataOffset, now)) {
                NewEntryValue = now;
                NewEntry = m;
                // This entry is now considered adopted
                break;
              }
            }
          
            UnlockFD(IndexFD);
            // some other process got here already. Abort.
            return;
          } else if (Index->Entries[m].GuestStart < OffsetRIP) {
            m = Index->Entries[m].Left;
          } else {
            m = Index->Entries[m].Right;
          }
        }
      }
      

      // Is this a new entry? Adopted ones don't have to update the index further
      if (InsertPoint != NewEntry) {

        // These don't have to be atomic if a barrier follows before Index Count update
        Index->Entries[NewEntry].GuestStart = OffsetRIP;
        Index->Entries[NewEntry].Left = UINT32_MAX;
        Index->Entries[NewEntry].Right = UINT32_MAX;
        Index->Entries[NewEntry].DataOffset = NewEntryValue = std::time(nullptr) | DATA_OFFSET_TIME_BASE;
      
        if (NewEntry != 0) {
          // Link the index
          if (Index->Entries[InsertPoint].GuestStart < OffsetRIP) {
            Index->Entries[InsertPoint].Left = NewEntry;
          } else {
            Index->Entries[InsertPoint].Right = NewEntry;
          }

          VERBOSE_LOG("Inserted {} after {} left {} right {}", NewEntry, InsertPoint, Index->Entries[InsertPoint].Left, Index->Entries[InsertPoint].Right);

        }

        // Increase Index Count.
        Index->Count++;
      }

      UnlockFD(IndexFD);
    }

    uint32_t ChunkNum;
    uint32_t ChunkOffset;

    // Allocate space on Data File
    {
      // process wide lock
      std::lock_guard lk {Mutex};

      // Data File lock
      LockFD(DataFD);

      uint64_t WriteOffset = 0;

      // Resize Data file if needed
      auto DataSize = InlineSize + sizeof(CacheEntry);
      auto DataSizeAligned = AlignUp(DataSize, 32);

      ChunkNum = Data->ChunksUsed - 1;
      ChunkOffset = CHUNK_SIZE - Data->CurrentChunkFree;

      if (DataSizeAligned > Data->CurrentChunkFree) {
        LOGMAN_THROW_A_FMT(DataSizeAligned < CHUNK_SIZE, "IR {} size > {}", DataSizeAligned, CHUNK_SIZE);
        WriteOffset = Data->ChunkOffsets[Data->ChunksUsed] = AlignUp(Data->WritePointer, FHU::FEX_PAGE_SIZE);


        fallocate(DataFD, 0, WriteOffset, CHUNK_SIZE);

        ChunkNum = Data->ChunksUsed;
        ChunkOffset = 0;
      }

      // Make sure changes are flushed
      std::atomic_thread_fence(std::memory_order_seq_cst);

      // Update Data file
      // If process crashes after here, the file might contain some junk, but won't be corrupted
      const auto NewChunk = WriteOffset != 0;

      if (NewChunk) {
        Data->WritePointer = Data->ChunkOffsets[ChunkNum];
        if (Data->ChunksUsed == MAX_CHUNKS - 1) {
          ERROR_AND_DIE_FMT("CodeCache: ChunksUsed Overflow, ChunksUsed: {}, MAX_CHUNKS: {}", Data->ChunksUsed, MAX_CHUNKS);
        }
        Data->ChunksUsed++;
        Data->CurrentChunkFree = CHUNK_SIZE;
      }

      Data->CurrentChunkFree -= DataSizeAligned;
      Data->WritePointer += DataSizeAligned;

      UnlockFD(DataFD);

      // Since we have the lock, do the map here
      if (NewChunk) {
        MapDataChunkUnsafe(ChunkNum);
      }
    }

    // Make sure the required chunk is mapped
    if (MappedDataChunks.size() <= ChunkNum || !MappedDataChunks[ChunkNum]) {
      std::lock_guard ulk{Mutex};
      MapDataChunkUnsafe(ChunkNum);
    }
    
    // Fill in the CacheEntry
    CacheEntry *Entry = (CacheEntry *)(MappedDataChunks[ChunkNum] + ChunkOffset);
    
    // Fill the data
    Fill(Entry);

    // Calculate the hash
    uint64_t &hash = Entry->GuestHash;
    hash = 0;

    auto Ranges = Entry->GetRangeData();
    for (size_t i = 0; i < Entry->GuestRangeCount; i++){ 
      hash = XXH3_64bits_withSeed((void*)(GuestRIP + Ranges[i].start), Ranges[i].length, hash);
    }

    [[maybe_unused]] bool Wasted;

    // Update the index
    {
      // process wide lock
      std::lock_guard lk {Mutex};

      // Index File lock
      LockFD(IndexFD);

      // Make sure changes are flushed
      std::atomic_thread_fence(std::memory_order_seq_cst);

      // Atomically update
      Wasted = !Index->Entries[NewEntry].DataOffset.compare_exchange_strong(NewEntryValue, ChunkNum * CHUNK_SIZE + ChunkOffset);

      // All done, unlock file
      UnlockFD(IndexFD);
    }  

    if (Wasted) {
      LogMan::Msg::IFmt("Adopted entry was not abandoned");
    } else {
      VERBOSE_LOG("Inserted {:x} {:x} to cache", GuestRIP, OffsetRIP);
    }
  }
}