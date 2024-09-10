// SPDX-License-Identifier: MIT
#include "Interface/Core/ObjectCache/ObjectCacheService.h"

#include <FEXCore/Config/Config.h>
#include <FEXCore/fextl/memory.h>
#include <FEXCore/fextl/string.h>
#include <FEXHeaderUtils/Filesystem.h>

#include <fcntl.h>
#include <xxhash.h>

namespace FEXCore::CodeSerialize {
void AsyncJobHandler::AsyncAddNamedRegionJob(uintptr_t Base, uintptr_t Size, uintptr_t Offset, const fextl::string& filename) {
#ifndef _WIN32
  // This function adds a named region *JOB* to our named region handler
  // This needs to be as fast as possible to keep out of the way of the JIT

  const fextl::string BaseFilename = FHU::Filesystem::GetFilename(filename);

  if (!BaseFilename.empty()) {
    // Create a new entry that once set up will be put in to our section object map
    auto Entry = fextl::make_unique<CodeRegionEntry>(Base, Size, Offset, filename, NamedRegionHandler->DefaultCodeHeader(Base, Offset));

    // Lock the job ref counter so we can block anything attempting to use the entry before it is loaded
    Entry->NamedJobRefCountMutex.lock();

    CodeRegionMapType::iterator EntryIterator;
    {
      std::unique_lock lk {CodeObjectCacheService->GetEntryMapMutex()};

      auto& EntryMap = CodeObjectCacheService->GetEntryMap();

      auto it = EntryMap.emplace(Base, std::move(Entry));
      if (!it.second) {
        // This happens when an application overwrites a previous region without unmapping what was there

        // Lock this entry's Named job reference counter.
        // Once this passes then we know that this section has been loaded.
        it.first->second->NamedJobRefCountMutex.lock();

        // Finalize anything the region needs to do first.
        CodeObjectCacheService->DoCodeRegionClosure(it.first->second->Base, it.first->second.get());

        // munmap the file that was mapped
        FEXCore::Allocator::munmap(it.first->second->CodeData, it.first->second->FileSize);

        // Remove this entry from the unrelocated map as well
        {
          std::unique_lock lk2 {CodeObjectCacheService->GetUnrelocatedEntryMapMutex()};
          CodeObjectCacheService->GetUnrelocatedEntryMap().erase(it.first->second->EntryHeader.OriginalBase);
        }

        // Now overwrite the entry in the map
        it = EntryMap.insert_or_assign(Base, std::move(Entry));
        EntryIterator = it.first;
      } else {
        // No overwrite, just insert
        EntryIterator = it.first;
      }
    }

    // Now that this entry has been added to the map, we can insert a load job using the entry iterator.
    // This allows us to quickly unblock the JIT thread when it is loading multiple regions and have the async thread
    // do the loading for us.
    //
    // Create the async work queue job now so it can load
    NamedRegionHandler->AsyncAddNamedRegionWorkItem(BaseFilename, filename, true, EntryIterator);

    // Tell the async thread that it has work to do
    CodeObjectCacheService->NotifyWork();
  }
#endif
}

void AsyncJobHandler::AsyncRemoveNamedRegionJob(uintptr_t Base, uintptr_t Size) {
#ifndef _WIN32
  // Removing a named region through the job system
  // We need to find the entry that we are deleting first
  fextl::unique_ptr<CodeRegionEntry> EntryPointer;
  {
    std::unique_lock lk {CodeObjectCacheService->GetEntryMapMutex()};

    auto& EntryMap = CodeObjectCacheService->GetEntryMap();
    auto it = EntryMap.find(Base);
    if (it != EntryMap.end()) {
      // Lock the job ref counter since we are erasing it
      // Once this passes it will have been loaded
      it->second->NamedJobRefCountMutex.lock();

      // Take the pointer from the map
      EntryPointer = std::move(it->second);

      // We can now unmap the file data
      FEXCore::Allocator::munmap(EntryPointer->CodeData, EntryPointer->FileSize);

      // Remove this from the entry map
      EntryMap.erase(it);

      // Remove this entry from the unrelocated map as well
      {
        std::unique_lock lk2 {CodeObjectCacheService->GetUnrelocatedEntryMapMutex()};
        CodeObjectCacheService->GetUnrelocatedEntryMap().erase(EntryPointer->EntryHeader.OriginalBase);
      }
    } else {
      // Tried to remove something that wasn't in our code object tracking
      return;
    }

    // Create the async work queue job now so it can finalize what it needs to do
    NamedRegionHandler->AsyncRemoveNamedRegionWorkItem(Base, Size, std::move(EntryPointer));

    // Tell the async thread that it has work to do
    CodeObjectCacheService->NotifyWork();
  }
#endif
}

void AsyncJobHandler::AsyncAddSerializationJob(fextl::unique_ptr<SerializationJobData> Data) {
  // XXX: Actually add serialization job
}
} // namespace FEXCore::CodeSerialize
