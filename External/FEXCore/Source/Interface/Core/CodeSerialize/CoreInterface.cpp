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
  void CodeSerializeService::Initialize() {
    // Add a canary so we don't crash on empty map iterator handling
    auto it = AddrToFile.insert_or_assign(~0ULL, std::make_unique<AddrToFileEntry>());
    UnrelocatedAddrToFile.insert_or_assign(~0ULL, it.first->second.get());

    uint64_t OldMask = FEXCore::Threads::SetSignalMask(~0ULL);
    WorkerThread = FEXCore::Threads::Thread::Create(ThreadHandler, this);
    FEXCore::Threads::SetSignalMask(OldMask);
  }

  CodeSerializeService::CodeSerializeService(FEXCore::Context::Context *ctx)
    : CTX {ctx}
    , NamedRegionHandling {ctx, this} {
    Initialize();
  }

  CodeSerializeService::~CodeSerializeService() {
  }

  void CodeSerializeService::Shutdown() {
    if (!CTX->Config.CacheCodeCompilation) {
      return;
    }

    ShuttingDown = true;
    // Kick the working thread
    StartWork.NotifyAll();
    if (WorkerThread->joinable()) {
      WorkerThread->join(nullptr);
    }
  }

  void CodeSerializeService::PrepareForExecve(FEXCore::Core::InternalThreadState *Thread) {
    // Drain this thread's work queue for the cache service
    FEXCore::Utils::RefCountLockUnique lk(Thread->AOTCodeBlockRefCounter);
  }

  void CodeSerializeService::CleanupAfterExecve(FEXCore::Core::InternalThreadState *Thread) {
  }

  void CodeSerializeService::PrepareForFork(FEXCore::Core::InternalThreadState *Thread) {
    {
      // Drain this thread's work queue for the cache service
      FEXCore::Utils::RefCountLockUnique lk(Thread->AOTCodeBlockRefCounter);
    }

    // We need to set mutexes to known state
    WorkingMutex.lock();

    // Lock named region modifying so nothing can change
    NamedRegionModifying.lock();

    // Queue mutex so no more work can get consumed
    QueueMutex.lock();

    // AOT IR cache lock
    AOTIRCacheLock.lock();

    StartWork.Lock();

    Thread->AOTCodeBlockRefCounter.lock();
  }

  constexpr static bool ServiceAfterFork = false;
  void CodeSerializeService::CleanupAfterFork(FEXCore::Core::InternalThreadState *LiveThread, bool Parent) {
    if (!Parent) {
      NamedRegionHandling.CleanupAfterFork();

      while (WorkQueue.size()) {
        WorkQueue.pop();
      }

      for (auto it = AddrToFile.begin(); it != AddrToFile.end(); ++it) {
        // Fork needs to close any file locks that got in the way
        if (it->second->CurrentSerializeFD != -1) {
          close(it->second->CurrentSerializeFD);
          it->second->CurrentSerializeFD = -1;
        }

        // We know that we don't have any jobs remaining for this object
        it->second->ObjectJobRefCountMutex.Reset();
      };

      // Clear our maps after fork to be safe
      AddrToFile.clear();
      UnrelocatedAddrToFile.clear();
    }

    LiveThread->AOTCodeBlockRefCounter.unlock();

    // Unlock working mutex
    StartWork.Unlock();

    // Unlock the mutexes we locked in prepare
    AOTIRCacheLock.unlock();

    QueueMutex.unlock();

    NamedRegionModifying.unlock();

    WorkingMutex.unlock();

    if (!Parent && ServiceAfterFork) {
      Initialize();
    }
  }

  const FileData::DataSection* CodeSerializeService::PreGenerateCodeFetch(uint64_t GuestRIP) {
    // This almost never fails because it is uncommon to remove a named region
    std::shared_lock regionlk(NamedRegionModifying);

    // Shared lock on the AOTIRCacheLock
    // XXX: wine64 hanging here
    AOTIRCacheLock.lock_shared();

    // This is a two stage lookup
    // First stage is to find the entrypoint's mapping to a named region
    // This finds a map with
    // 1) Current namespace of named regions
    // 2) Current RIP
    auto it = FindAddrForFile(GuestRIP);
    if (it != AddrToFile.end()) {
      AddrToFileEntry *Entry = it->second.get();
      // If loading the named region isn't complete then wait for it
      if (Entry->LoadingComplete.Test() == false) {
        AOTLOG("Waiting for {} code cache to load", Entry->filename);
        // We need to do a shared lock dance for the writer to insert this object
        AOTIRCacheLock.unlock_shared();
        Entry->LoadingComplete.Wait();
        AOTIRCacheLock.lock_shared();
      }

      // We've found the named region where the code lives
      // Now we need to lookup inside of its cache to find if it has a cache for this entrypoint
      // This looks in the namespace of PREVIOUS offsets
      // 1) Offset current guestRIP by this regions CURRENT base
      // 2) This gives us an offset in to the section lookup map regardless of RIP namespace
      uint64_t NamedRegionOffset = GuestRIP - it->first;

      auto CodeSectionIt = Entry->SectionLookupMap.find(NamedRegionOffset);
      if (CodeSectionIt != Entry->SectionLookupMap.end() &&
          CodeSectionIt->second->Invalid == false) {
        // Check the code hash first
        auto GuestCodeHash = XXH3_64bits((void*)GuestRIP, CodeSectionIt->second->Data->GuestCodeLength);
        [[maybe_unused]] auto HostCodeHash = XXH3_64bits(CodeSectionIt->second->HostCode, CodeSectionIt->second->Data->HostCodeLength);
        if ((
            GuestCodeHash == CodeSectionIt->second->Data->GuestCodeHash &&
            //HostCodeHash == CodeSectionIt->second->Data->HostCodeHash &&
            true) &&
            true)
        {
          AOTLOG("\tCode cache loaded");
          AOTIRCacheLock.unlock_shared();
          return CodeSectionIt->second;
        }
        else {
          if (GuestCodeHash != CodeSectionIt->second->Data->GuestCodeHash) {
            AOTLOG("0x{:x} RIP  0x{:x} != 0x{:x}. {} bytes. doesn't match what was in code cache. Not relocating. Linux relocations interfering?",
                GuestRIP,
                GuestCodeHash,
                CodeSectionIt->second->Data->GuestCodeHash,
                CodeSectionIt->second->Data->GuestCodeLength);
          }
          if (HostCodeHash != CodeSectionIt->second->Data->HostCodeHash) {
            AOTLOG("0x{:x} RIP  0x{:x} != 0x{:x}. {} bytes. Host code cache didn't match?",
                GuestRIP,
                HostCodeHash,
                CodeSectionIt->second->Data->HostCodeHash,
                CodeSectionIt->second->Data->HostCodeLength);
          }

          // Mark this as invalid so we can skip it later
          CodeSectionIt->second->Invalid = true;
          Entry->SectionLookupMap.erase(CodeSectionIt);
        }
      }
    }

    AOTIRCacheLock.unlock_shared();
    return nullptr;
  }

  uint64_t CodeSerializeService::FindRelocatedRIP(uint64_t GuestRIP) {
    std::unique_lock lk(AOTIRUnrelocatedCacheLock);

    // First step, find the addr that this relocation RIP ORIGINALLY pointed to
    auto it = FindUnrelocatedAddrForFile(GuestRIP);
    if (it != UnrelocatedAddrToFile.end()) {
      // Found the original relocated section location
      // Next Steps
      // 1) Get the offset in to the original region
      uint64_t Result = GuestRIP - it->second->CodeHeader.OriginalBase;
      // 2) Add the current offset for this named region
      Result += it->second->Base;

      // Increase the number of relocations to this region for tracking and serialization purposes
      it->second->CodeHeader.NumRelocationsTo += 1;
      return Result;
    }

    // With a hot code cache not finding an address usually happens when trying to relocate in to the .bss section of an ELF.
    // eg: libc stores a bunch of configuration options in the bss section which then of course has a relocation.
    //
    // This is a non-fatal condition and it just means we can't relocate this section.
    //
    // Ideas: With a forked glibc, we could name BSS sections so we can track these.
    return ~0ULL;
  }

  AddrToFileMapType::iterator CodeSerializeService::FindAddrForFile(uint64_t Entry) {
    // Thread safety here! We are returning an iterator to the map object
    // This needs the AOTIRCacheLock locked prior to coming in to the function
    uint64_t Length = 0;
    auto file = AddrToFile.lower_bound(Entry);
    if (file == AddrToFile.end() ||
        file->first > Entry) {
      --file;
    }

    if (file == AddrToFile.end()) {
      return AddrToFile.end();
    }

    const auto FileObject = file->second.get();

    if (FileObject->Base <= Entry && (FileObject->Base + FileObject->Len) >= (Entry + Length)) {
      return file;
    }
    // Failures happen on JIT code mostly. Not fatal
    // LOGMAN_MSG_A_FMT("Couldn't find named region for 0x{:x}", Entry);
    return AddrToFile.end();
  }

  AddrToFilePtrMapType::iterator CodeSerializeService::FindUnrelocatedAddrForFile(uint64_t Entry) {
    // Thread safety here! We are returning an iterator to the map object
    // This needs the AOTIRCacheLock locked prior to coming in to the function
    uint64_t Length = 0;
    auto file = UnrelocatedAddrToFile.lower_bound(Entry);
    if (file == UnrelocatedAddrToFile.end() ||
        file->first > Entry) {
      --file;
    }

    if (file->first <= Entry && (file->first + file->second->Len) >= (Entry + Length)) {
      //AOTLOG("\tFound a unrelocated addr to file map");
      //AOTLOG("{}{} -> Current: [0x{:x}, 0x{:x}) Original: [0x{:x}, 0x{:x})", file->second->Offset != 0 ? "\t" : "", file->second->filepath, file->second->Base, file->second->Base + file->second->Len, file->first, file->first + file->second->Len);
      return file;
    }

    return UnrelocatedAddrToFile.end();
  }

  void CodeSerializeService::ExecutionThread() {
    // Set our thread name so we can see its relation
    char ThreadName[16] = "CodeSerialize\0";
    pthread_setname_np(pthread_self(), ThreadName);

    while (true) {
      // Wait for work
      StartWork.Wait();
      std::shared_lock lk_work(WorkingMutex);

      // Named work queue jobs are higher priority than serialization
      NamedRegionHandling.HandleNamedRegionJobs();

      size_t WorkItems{};

      do {
        // Grab a work item
        std::unique_ptr<AOTData> Item{};
        {
          std::unique_lock lk(QueueMutex);
          WorkItems = WorkQueue.size();
          if (WorkItems != 0) {
            Item = std::move(WorkQueue.front());
            WorkQueue.pop();
          }
        }

        if (Item) {
          FEXCore::Utils::RefCountLockShared lk(AOTIRCacheLock);
          SerializeHandler(std::move(Item));
        }
      } while (WorkItems != 0);

      if (ShuttingDown.load()) {
        break;
      }
    }

    for (auto it = AddrToFile.begin(); it != AddrToFile.end(); ++it) {
      DoClosure(it);
    };

    UnrelocatedAddrToFile.clear();
    AddrToFile.clear();
  }
}
