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
  NamedRegionHandling::NamedRegionHandling(FEXCore::Context::Context *ctx, CodeSerializeService *service)
    : CodeService {service} {
    if (!ctx->Config.CacheCodeCompilation) {
      return;
    }

    DefaultSerializationConfig.Cookie = CODE_COOKIE;

    // Initialize the Arch from CPUID
    uint32_t Arch = ctx->CPUID.RunFunction(0x4000'0001, 0).eax & 0xF;
    DefaultSerializationConfig.Arch = Arch;

    DefaultSerializationConfig.MaxInstPerBlock = ctx->Config.MaxInstPerBlock;
    DefaultSerializationConfig.MultiBlock = ctx->Config.Multiblock;
    DefaultSerializationConfig.TSOEnabled = ctx->Config.TSOEnabled;
    DefaultSerializationConfig.ABILocalFlags = ctx->Config.ABILocalFlags;
    DefaultSerializationConfig.ABINoPF = ctx->Config.ABINoPF;
    DefaultSerializationConfig.SRA = ctx->Config.StaticRegisterAllocation;
    DefaultSerializationConfig.ParanoidTSO = ctx->Config.ParanoidTSO;
    DefaultSerializationConfig.Is64BitMode = ctx->Config.Is64BitMode;
    DefaultSerializationConfig.SMCChecks = ctx->Config.SMCChecks;
  }

  void NamedRegionHandling::AddNamedRegionJob(uintptr_t Base, uintptr_t Size, uintptr_t Offset, const std::string &filename, bool Executable) {
    auto base_filename = std::filesystem::path(filename).filename().string();

    if (!base_filename.empty()) {
      std::unique_ptr<AddrToFileEntry> Entry = std::make_unique<AddrToFileEntry>(
        Base,
        Size,
        Offset,
        filename,
        DefaultCodeHeader(Base, Offset)
      );

      // Increment the job ref counter
      Entry->NamedJobRefCountMutex.lock_shared();

      AOTLOG("Adding region job for: {} -> 0x{:x}", base_filename, Base);
      AddrToFileMapType::iterator EntryIterator;
      {
        // Pull a small lock to insert the entry
        FEXCore::Utils::RefCountLockUnique lk(CodeService->GetFileMapLock());
        auto &FileMap = CodeService->GetFileMap();

        auto it = FileMap.emplace(Base, std::move(Entry));
        if (!it.second){
          // This is fun when an application is overwriting a previous region without unmapping what was already there
          if (it.first->second->LoadingComplete.Test()) {
            // If loading for this was already complete then we can safely overwrite
            //if (!it.first->second->NamedJobRefCountMutex.try_lock()) {
            //  // No named job references here
            //}
            //else {
            //  // XXX: Had remaining jobs, drain the queue so we can safely remove what was previously there
            //  //std::unique_lock lk(NamedQueueMutex);
            //  //DrainAOTNameQueue();
            //}

            AOTLOG("Removing overwritten named region: {}", it.first->second->SourceCodePath);
            {
              // Wait for lock to unlock if it was being loaded still
              std::unique_lock lk(it.first->second->NamedJobRefCountMutex);
            }

            CodeService->DoClosure(it.first);

            FEXCore::Allocator::munmap(it.first->second->Data.CodeData, it.first->second->Data.FileSize);

            // Hold all the cache locks to modify this
            std::unique_lock lk2(CodeService->GetUnrelocatedFileMapLock());

            CodeService->GetUnrelocatedFileMap().erase(it.first->second->CodeHeader.OriginalBase);

            // Overwrite
            it = FileMap.insert_or_assign(Base, std::move(Entry));
            EntryIterator = it.first;
          }
          else {
            LOGMAN_MSG_A_FMT("Tried to overwrite a region with a new region! 0x{:x} {} overwritten by {}", Base, it.first->second->SourceCodePath, base_filename);
            return;
          }
        }
        else {
          // No overwrite
          EntryIterator = it.first;
        }
      }

      // Preallocate the tracking data so if the JIT tries running a section before load, it waits for load to complete.
      // Otherwise we spin a bunch of JIT code that just slows down the whole process.
      {
        // Add the named object to load to the queue
        std::unique_lock lk(NamedQueueMutex);
        NamedWorkQueue.emplace(std::make_unique<WorkItemAdd> (
          base_filename,
          filename,
          Executable,
          EntryIterator
        ));
        ++NamedWorkQueueJobs;
      }

      // Notify the thread that it has more work
      CodeService->NotifyWork();
    }
  }

  void NamedRegionHandling::RemoveNamedRegionJob(uintptr_t Base, uintptr_t Size) {
    {
      CodeService->RemoveNamedRegion(Base, Size);
      return;
    }
    // Removing a named region through the job system
    // We need to find the entry that we are deleting first
    AddrToFileMapType::iterator EntryIterator;
    {
      // Pull a shared lock so we can read the entry
      FEXCore::Utils::RefCountLockShared lk(CodeService->GetFileMapLock());

      auto &FileMap = CodeService->GetFileMap();
      auto it = FileMap.find(Base);
      if (it != FileMap.end()) {
        // Increment the job ref counter
        // This lets us know if we are trying to erase while overwriting
        //it->second->NamedJobRefCountMutex.lock_shared();
        EntryIterator = it;
      }
      else {
        // Trying to erase a named region that doesn't exist
        return;
      }
    }

    {
      // Add the named object to load to the queue
      std::unique_lock lk(NamedQueueMutex);
      NamedWorkQueue.emplace(std::make_unique<WorkItemRemove> (
        Base,
        Size,
        EntryIterator
      ));
      ++NamedWorkQueueJobs;
    }

    // Notify the thread that it has more work
    AOTLOG("Adding a removing named region job");
    CodeService->NotifyWork();
  }

  void NamedRegionHandling::HandleNamedRegionJobs() {
    while (NamedWorkQueueJobs) {
      std::unique_ptr<NamedWorkItem> Item{};
      {
        std::unique_lock lk(NamedQueueMutex);
        size_t WorkItems = NamedWorkQueue.size();
        if (WorkItems != 0) {
          Item = std::move(NamedWorkQueue.front());
          NamedWorkQueue.pop();
        }
        --NamedWorkQueueJobs;
      }

      if (Item) {
        if (Item->GetType() == NamedWorkItem::QueueWorkType::TYPE_ADD) {
          WorkItemAdd *WorkAdd = static_cast<WorkItemAdd*>(Item.get());

          CodeService->AddNamedRegion(
            WorkAdd->Entry,
            WorkAdd->BaseFilename,
            WorkAdd->Filename,
            WorkAdd->Executable);
        }
        else {
          if (Item->GetType() == NamedWorkItem::QueueWorkType::TYPE_REMOVE) {
            WorkItemRemove *WorkRemove = static_cast<WorkItemRemove*>(Item.get());

            CodeService->RemoveNamedRegion(
              WorkRemove->Base,
              WorkRemove->Size
            );
          }
        }
      }
    }
  }

  void NamedRegionHandling::DrainNameJobQueue() {
    // Lock the named queue up front and consume all named region work
    std::unique_lock lk(NamedQueueMutex);

    while (NamedWorkQueue.size()) {
      std::unique_ptr<NamedWorkItem> Item{};
      Item = std::move(NamedWorkQueue.front());
      NamedWorkQueue.pop();
      --NamedWorkQueueJobs;
      // Work the queue
      if (Item->GetType() == NamedWorkItem::QueueWorkType::TYPE_ADD) {
        WorkItemAdd *WorkAdd = static_cast<WorkItemAdd*>(Item.get());

        CodeService->AddNamedRegion(
          WorkAdd->Entry,
          WorkAdd->BaseFilename,
          WorkAdd->Filename,
          WorkAdd->Executable);
      }
      else {
        if (Item->GetType() == NamedWorkItem::QueueWorkType::TYPE_REMOVE) {
          WorkItemRemove *WorkRemove = static_cast<WorkItemRemove*>(Item.get());
          CodeService->RemoveNamedRegion(
            WorkRemove->Base,
            WorkRemove->Size
          );
        }
      }
    }
  }

  void NamedRegionHandling::CleanupAfterFork() {
    // Remove current jobs. They no longer exist in this fork
    while (NamedWorkQueue.size()) {
      NamedWorkQueue.pop();
    }

    // We have deleted all named work queue jobs
    NamedWorkQueueJobs = 0;
  }

  void CodeSerializeService::DrainAOTJobQueue() {
    // Thread warning here!
    // Ensure that the Work Queue mutex is locked before coming in to this!
    while (WorkQueue.size()) {
      // Grab a work item
      std::unique_ptr<AOTData> Item{};
      Item = std::move(WorkQueue.front());
      WorkQueue.pop();
      SerializeHandler(std::move(Item));
    }
  }

  void CodeSerializeService::AddSerializeJob(std::unique_ptr<AOTData> Data) {
    // Hash the host code before sending it over to the helper thread
    // Prevents code changing problems due to backpatching
    if (Data->HostCodeHash == 0) {
      Data->HostCodeHash = XXH3_64bits(Data->HostCodeBegin, Data->HostCodeLength);
    }

    // This almost never fails because it is uncommon to remove a named region
    std::shared_lock regionlk(NamedRegionModifying);

    // Lock necessary for FindAddrForFile
    // Causing deadlock between AOTIRCacheLock and QueueMutex in RemoveNamedRegion
    FEXCore::Utils::RefCountLockShared lk(AOTIRCacheLock);

    // Find in the named region lookup this location
    auto it = FindAddrForFile(Data->GuestRIP);
    if (it != AddrToFile.end() && it->second->StillSerializing) {
      // Increment the ref counter so it is tracked that it has a job
      // If it fails to get the shared lock then the region is likely getting deleted
      // Throw away this job if it fails

      // XXX: This has a nasty edge case where if there is a write lock pending then this
      // will still increment the shared counter. Thus making the write lock take even LONGER
      // before it gains an exclusive lock.
      //
      // We can fix this with a exclusive priority mutex
      if (it->second->ObjectJobRefCountMutex.try_lock_shared()) {
        // Increment the share counter
        // Guaranteed to not exclusive block
        Data->ThreadRefCountAOT->lock_shared();

        // Get the pointer to the named region's ref counter
        Data->ObjectJobRefCountMutexPtr = &it->second->ObjectJobRefCountMutex;

        // Pass over the iterator through the AOT data to remove another lookup
        Data->MapIter = it;

        {
          // Fill the threads work queue
          std::unique_lock lk(QueueMutex);
          WorkQueue.emplace(std::move(Data));
        }

        // Notify the thread that it has more work
        StartWork.NotifyAll();
      }
    }
  }

}

