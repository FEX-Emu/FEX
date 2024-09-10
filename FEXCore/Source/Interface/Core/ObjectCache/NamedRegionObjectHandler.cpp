// SPDX-License-Identifier: MIT
#include "Interface/Core/ObjectCache/ObjectCacheService.h"

#include <FEXCore/fextl/memory.h>
#include <FEXCore/fextl/string.h>

namespace FEXCore::CodeSerialize {
NamedRegionObjectHandler::NamedRegionObjectHandler(FEXCore::Context::ContextImpl* ctx) {
  DefaultSerializationConfig.Cookie = CODE_COOKIE;

  // Initialize the Arch from CPUID
  uint32_t Arch = ctx->CPUID.RunFunction(0x4000'0001, 0).eax & 0xF;
  DefaultSerializationConfig.Arch = Arch;

  DefaultSerializationConfig.MaxInstPerBlock = ctx->Config.MaxInstPerBlock;
  DefaultSerializationConfig.MultiBlock = ctx->Config.Multiblock;
  DefaultSerializationConfig.TSOEnabled = ctx->Config.TSOEnabled;
  DefaultSerializationConfig.ABILocalFlags = ctx->Config.ABILocalFlags;
  DefaultSerializationConfig.ParanoidTSO = ctx->Config.ParanoidTSO;
  DefaultSerializationConfig.Is64BitMode = ctx->Config.Is64BitMode;
  DefaultSerializationConfig.SMCChecks = ctx->Config.SMCChecks;
  DefaultSerializationConfig.x87ReducedPrecision = ctx->Config.x87ReducedPrecision;
}

void NamedRegionObjectHandler::AddNamedRegionObject(CodeRegionMapType::iterator Entry, const fextl::string& base_filename,
                                                    const fextl::string& filename, bool Executable) {
  // XXX: Add named region objects

  // XXX: Until entry loading is complete just claim it is loaded
  Entry->second->NamedJobRefCountMutex.unlock();
}

void NamedRegionObjectHandler::RemoveNamedRegionObject(uintptr_t Base, uintptr_t Size, fextl::unique_ptr<CodeRegionEntry> Entry) {
  // XXX: Remove named region objects

  // XXX: Until entry loading is complete just claim it is loaded
  Entry->NamedJobRefCountMutex.unlock();
}

void NamedRegionObjectHandler::HandleNamedRegionObjectJobs() {
  // Walk through all of our jobs sequentially until the work queue is empty
  while (NamedWorkQueueJobs.load()) {
    fextl::unique_ptr<AsyncJobHandler::NamedRegionWorkItem> WorkItem;

    {
      // Lock the work queue mutex for a short moment and grab an item from the list
      std::unique_lock lk {NamedWorkQueueMutex};
      size_t WorkItems = WorkQueue.size();
      if (WorkItems != 0) {
        WorkItem = std::move(WorkQueue.front());
        WorkQueue.pop();
      }

      // Atomically update the number of jobs
      --NamedWorkQueueJobs;
    }

    if (WorkItem) {
      if (WorkItem->GetType() == AsyncJobHandler::NamedRegionJobType::JOB_ADD_NAMED_REGION) {
        auto WorkAdd = static_cast<AsyncJobHandler::WorkItemAddNamedRegion*>(WorkItem.get());
        AddNamedRegionObject(WorkAdd->Entry, WorkAdd->BaseFilename, WorkAdd->Filename, WorkAdd->Executable);
      }

      if (WorkItem->GetType() == AsyncJobHandler::NamedRegionJobType::JOB_REMOVE_NAMED_REGION) {
        auto WorkRemove = static_cast<AsyncJobHandler::WorkItemRemoveNamedRegion*>(WorkItem.get());
        RemoveNamedRegionObject(WorkRemove->Base, WorkRemove->Size, std::move(WorkRemove->Entry));
      }
    }
  }
}
} // namespace FEXCore::CodeSerialize
