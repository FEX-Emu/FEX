// SPDX-License-Identifier: MIT
#include "Interface/Core/ObjectCache/ObjectCacheService.h"

#include <FEXCore/Config/Config.h>
#include <FEXCore/fextl/memory.h>
#include <FEXCore/Utils/Threads.h>

namespace {
  static void* ThreadHandler(void *Arg) {
    FEXCore::CodeSerialize::CodeObjectSerializeService *This = reinterpret_cast<FEXCore::CodeSerialize::CodeObjectSerializeService*>(Arg);
    This->ExecutionThread();
    return nullptr;
  }
}

namespace FEXCore::CodeSerialize {
  CodeObjectSerializeService::CodeObjectSerializeService(FEXCore::Context::ContextImpl *ctx)
    : CTX {ctx}
    , AsyncHandler { &NamedRegionHandler , this }
    , NamedRegionHandler { ctx } {
    Initialize();
  }

  void CodeObjectSerializeService::Shutdown() {
    if (CTX->Config.CacheObjectCodeCompilation() == FEXCore::Config::ConfigObjectCodeHandler::CONFIG_NONE) {
      return;
    }

    WorkerThreadShuttingDown = true;

    // Kick the working thread
    WorkAvailable.NotifyAll();

    if (WorkerThread->joinable()) {
      // Wait for worker thread to close down
      WorkerThread->join(nullptr);
    }
  }

  void CodeObjectSerializeService::Initialize() {
    // Add a canary so we don't crash on empty map iterator handling
    auto it = AddressToEntryMap.insert_or_assign(~0ULL, fextl::make_unique<CodeRegionEntry>());
    UnrelocatedAddressToEntryMap.insert_or_assign(~0ULL, it.first->second.get());

    uint64_t OldMask = FEXCore::Threads::SetSignalMask(~0ULL);
    WorkerThread = FEXCore::Threads::Thread::Create(ThreadHandler, this);
    FEXCore::Threads::SetSignalMask(OldMask);
  }

  void CodeObjectSerializeService::DoCodeRegionClosure(uint64_t Base, CodeRegionEntry *it) {
    if (Base == ~0ULL) {
      // Don't do closure on canary
      return;
    }
    // XXX: Do code region closure
  }

  CodeObjectFileSection const *CodeObjectSerializeService::FetchCodeObjectFromCache(uint64_t GuestRIP) {
    // XXX: Actually fetch code objects from cache
    return nullptr;
  }

  void CodeObjectSerializeService::ExecutionThread() {
    // Set our thread name so we can see its relation
    FEXCore::Threads::SetThreadName("ObjectCodeSeri\0");
    while (WorkerThreadShuttingDown.load() != true) {
      // Wait for work
      WorkAvailable.Wait();

      // Handle named region async jobs first. Highest priority
      NamedRegionHandler.HandleNamedRegionObjectJobs();

      // XXX: Handle code serialization jobs second.
    }

    // Do final code region closures on thread shutdown
    for (auto &it : AddressToEntryMap) {
      DoCodeRegionClosure(it.first, it.second.get());
    }

    // Safely clear our maps now
    AddressToEntryMap.clear();
    UnrelocatedAddressToEntryMap.clear();
  }
}
