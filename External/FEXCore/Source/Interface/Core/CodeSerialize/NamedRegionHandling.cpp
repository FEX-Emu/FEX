#include "Interface/Context/Context.h"
#include "Interface/Core/CodeSerialize/CodeSerialize.h"

#include <FEXCore/Config/Config.h>

#include <fcntl.h>
#include <filesystem>
#include <memory>
#include <string>
#include <sys/uio.h>
#include <sys/mman.h>
#include <xxhash.h>

namespace FEXCore::CodeSerialize {
  void CodeSerializeService::AddNamedRegion(AddrToFileMapType::iterator Entry, const std::string &base_filename, const std::string &filename, bool Executable){
    AOTIRCacheRemoveLock.lock_shared();

    // We need to have relocation data for each section
    // If we store only executable sections then some relocations aren't found
    auto DataDir = FEXCore::Config::GetDataDirectory();
    const auto filepath = fmt::format("{}/aotcode/", DataDir);
    std::error_code ec;

    // Just create it, don't care about failure atm
    std::filesystem::create_directory(filepath, ec);

    auto EntryPtr = Entry->second.get();

    AOTLOG("Adding named region: {}", filename);
    const auto filepath_entry = fmt::format("{}{}-{}-{}.code", filepath, base_filename, CodeSerializationConfig::GetHash(NamedRegionHandling.GetDefaultSerializationConfig()), EntryPtr->Offset);

    // Copy over the remaining elements
    EntryPtr->SourceCodePath = filepath_entry;

    size_t CodeEntries{};
    // Load the incoming code
    int stream_code = open(EntryPtr->SourceCodePath.c_str(), O_RDWR, (S_IRUSR | S_IWUSR) | (S_IRGRP | S_IWGRP) | (S_IROTH | S_IWOTH));
    {
      if (stream_code != -1) {
        AOTLOG("Loaded code cache for {}", EntryPtr->SourceCodePath);

        EntryPtr->Data.FileSize = lseek(stream_code, 0, SEEK_END);
        lseek(stream_code, 0, SEEK_SET);

        // Copy the code header
        // Read it to ensure it's atomic
        ReadHeader(stream_code, &EntryPtr->CodeHeader);

        if (EntryPtr->CodeHeader.Config == NamedRegionHandling.GetDefaultSerializationConfig() &&
            EntryPtr->CodeHeader.TotalCodeSize != 0) {

          // mmap the file as shared
          // MAP_POPULATE helps with an application blocking JIT due to slow IO
          EntryPtr->Data.CodeData = (char*)FEXCore::Allocator::mmap(nullptr, EntryPtr->Data.FileSize, PROT_READ, MAP_SHARED | MAP_POPULATE, stream_code, 0);

          EntryPtr->Data.FileCodeSections.reserve(EntryPtr->CodeHeader.NumCodeEntries);
          EntryPtr->SectionLookupMap.reserve(EntryPtr->CodeHeader.NumCodeEntries);

          for (size_t DataIndex = sizeof(EntryPtr->CodeHeader); DataIndex < EntryPtr->Data.FileSize;) {
            const CodeSerializationData *CodeSerializeData = reinterpret_cast<const CodeSerializationData*>(&EntryPtr->Data.CodeData[DataIndex]);
            DataIndex += sizeof(CodeSerializationData);

            // Get the code offset
            const char *HostCodeOffset = EntryPtr->Data.CodeData + DataIndex;
            // Add to our index
            DataIndex += CodeSerializeData->HostCodeLength;

            if (CodeSerializeData->HostCodeLength > EntryPtr->CodeHeader.TotalCodeSize) {
              // Invalid code length means this cache is corrupted
              // Clear it out to avoid crashes
              CodeEntries = 0;
              EntryPtr->Data.FileCodeSections.clear();
              EntryPtr->SectionLookupMap.clear();

              AOTLOG("Offset: {} {} {}", lseek(stream_code, 0, SEEK_CUR), errno, strerror(errno));
              AOTLOG("Oops, invalid code length?  0x{:x} > 0x{:x}",
                CodeSerializeData->HostCodeLength,
                EntryPtr->CodeHeader.TotalCodeSize);

              FEXCore::Allocator::munmap(EntryPtr->Data.CodeData, EntryPtr->Data.FileSize);

              // Tell the rest of the machine to stop serializing to this file
              EntryPtr->StillSerializing = false;
              break;
            }
            else {
              size_t RelocationOffset = DataIndex;
              DataIndex += CodeSerializeData->RelocationSize;

              // Add the entry to the code tracking
              auto &it = EntryPtr->Data.FileCodeSections.emplace_back(FileData::DataSection {
                true, // Since we copied the file, this is already serialized
                false,
                CodeSerializeData,
                HostCodeOffset,
                CodeSerializeData->NumRelocations,
                &EntryPtr->Data.CodeData[RelocationOffset]
              });
              EntryPtr->SectionLookupMap.emplace(it.Data->RIPOffset, &it);
              ++CodeEntries;
            }
          }

          // Track the number of sections we need to reserialize
          AOTLOG("Loaded {} entries from file", CodeEntries);
          close (stream_code);
          stream_code = -1;
        }
        else {
          FEXCore::Allocator::munmap(EntryPtr->Data.CodeData, EntryPtr->Data.FileSize);
        }
      }
      else {
        // Only create a file if we are in read/write mode
        if (CTX->Config.CacheCodeCompilation == 2) {
          // Try to create the file with O_EXCL.
          // This allows us to exclusively create the file.
          // If another process managed to create the file just before us then it is safe to continue.
          auto in = open(EntryPtr->SourceCodePath.c_str(), O_CREAT | O_EXCL | O_RDWR, (S_IRUSR | S_IWUSR) | (S_IRGRP | S_IWGRP) | (S_IROTH | S_IWOTH));

          if (in == -1 && errno != EEXIST) {
            // Unhandled error
            LOGMAN_MSG_A_FMT("Couldn't open file {} {} {} {}", EntryPtr->SourceCodePath, in, errno, strerror(errno));
          }
          else if (in != -1) {
            // Managed to create the file. Set up a lock and write the code header
            GetFDLock(in, F_WRLCK);

            int Result = pwrite(in, &EntryPtr->CodeHeader, sizeof(EntryPtr->CodeHeader), 0);

            if (Result == -1) {
              LOGMAN_MSG_A_FMT("Had an error serializing Original source {}. {} {}", EntryPtr->SourceCodePath, errno, strerror(errno));
            }

            close(in);
          }
        }
      }
    }

    std::unique_lock lk(AOTIRUnrelocatedCacheLock);
    AOTIRCacheRemoveLock.unlock_shared();

    // Entry was already insert in to the AddrToFile map
    // Now we just need to insert it in to the unrelocated map
    UnrelocatedAddrToFile.insert_or_assign(EntryPtr->CodeHeader.OriginalBase, EntryPtr);

    // Decrement the named job counter
    EntryPtr->NamedJobRefCountMutex.unlock_shared();

    // Let any threads know that loading is complete
    EntryPtr->LoadingComplete.NotifyAll();
  }

  void CodeSerializeService::RemoveNamedRegionByIterator(AddrToFileMapType::iterator Entry) {
    // Hold the region lock so no named regions will be coming in
    std::unique_lock regionlk(NamedRegionModifying);

    // Hold the Serialization queue mutex so no more jobs can be added.
    std::unique_lock aotlk(QueueMutex);

    // Removal lock
    // Theoretically this can be moved lower in code but there is a danger of deadlocks
    // Do this lock right now while investigations occur to remove those deadlocks
    AOTIRCacheRemoveLock.lock();

    // Hold the AOTIRCache lock so no threads can change the mapping.
    AOTIRCacheLock.lock_shared();

    AddrToFile.erase(Entry);

    // Unlock the shared mutex
    AOTIRCacheLock.unlock_shared();

    AOTIRCacheRemoveLock.unlock();
  }

  void CodeSerializeService::RemoveNamedRegion(uintptr_t Base, uintptr_t Size) {
    // Hold the region lock so no named regions will be coming in
    std::unique_lock regionlk(NamedRegionModifying);

    // Hold the Serialization queue mutex so no more jobs can be added.
    std::unique_lock aotlk(QueueMutex);

    // Removal lock
    // Theoretically this can be moved lower in code but there is a danger of deadlocks
    // Do this lock right now while investigations occur to remove those deadlocks
    AOTIRCacheRemoveLock.lock();

    // Hold the AOTIRCache lock so no threads can change the mapping.
    AOTIRCacheLock.lock_shared();

    // TODO: Support partial removing
    auto it = AddrToFile.find(Base);
    if (it != AddrToFile.end()) {
      AOTLOG("Removing Named Region: {}", it->second->filename);

      // Decrement ref counter for this region
      //it->second->NamedJobRefCountMutex.unlock_shared();

      // Now try to lock it
      if (!it->second->NamedJobRefCountMutex.try_lock()) {
        AOTIRCacheRemoveLock.unlock();
        // Wait for the named region to load so...we can immediately remove it.
        {
          // We need to do this here because the serialize thread is likely blocked on doing code serialization
          // And we own the QueueMutex right now
          NamedRegionHandling.DrainNameJobQueue();
        }

        {
          // In case the serialization thread was processing named regions already. Ensure we have the lock
          std::unique_lock lk(it->second->NamedJobRefCountMutex);
        }
        AOTIRCacheRemoveLock.lock();
      }

      if (!it->second->ObjectJobRefCountMutex.try_lock()) {
        // Unlock the shared mutex
        AOTIRCacheLock.unlock_shared();

        // If we could not get the object lock, then this means we have jobs in the queue for this region
        // We must drain the queue now
        // Drain the job queue before removing.
        DrainAOTJobQueue();
      }
      else {
        // Unlock the shared mutex
        AOTIRCacheLock.unlock_shared();
      }

      // If we try pulling a unique lock on AOTIRCacheLock here then
      // something could be already holding the AOTIRCacheLock and waiting on QueueMutex to unlock
      AOTLOG("Removing named region: {}", it->second->SourceCodePath);
      DoClosure(it);
      FEXCore::Allocator::munmap(it->second->Data.CodeData, it->second->Data.FileSize);

      // Hold all the cache locks to modify this
      FEXCore::Utils::RefCountLockUnique lk(AOTIRCacheLock);
      std::unique_lock lk2(AOTIRUnrelocatedCacheLock);

      UnrelocatedAddrToFile.erase(it->second->CodeHeader.OriginalBase);
      AddrToFile.erase(it);
    }
    else {
      // Unlock the shared mutex
      AOTIRCacheLock.unlock_shared();
    }

    AOTIRCacheRemoveLock.unlock();
  }
}
