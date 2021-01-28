#include "Interface/Context/Context.h"
#include "Interface/Core/LookupCache.h"
#include "Interface/Core/CompileService.h"
#include "Interface/Core/InternalThreadState.h"
#include "Interface/Core/OpcodeDispatcher.h"

namespace FEXCore {
  CompileService::CompileService(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread)
    : CTX {ctx}
    , ParentThread {Thread} {

    CompileThreadData = std::make_unique<FEXCore::Core::InternalThreadState>();
    CompileThreadData->IsCompileService = true;

    // We need a compiler for this work thread
    CTX->InitializeCompiler(CompileThreadData.get(), true);
    CompileThreadData->CPUBackend->CopyNecessaryDataForCompileThread(ParentThread->CPUBackend.get());

    WorkerThread = std::thread([this]() {
      ExecutionThread();
    });
  }

  void CompileService::Initialize() {
    // Share CompileService which = this
    CompileThreadData->CompileService = ParentThread->CompileService;
  }

  void CompileService::Shutdown() {
    ShuttingDown = true;
    // Kick the working thread
    StartWork.NotifyAll();
    WorkerThread.join();
  }

  void CompileService::ClearCache(FEXCore::Core::InternalThreadState *Thread, uint64_t GuestRIP) {
    // On cache clear we need to spin down the execution thread to ensure it isn't trying to give us more work items
    if (CompileMutex.try_lock()) {
      // We can only clear these things if we pulled the compile mutex

      // Grab the work queue and clear it
      // We don't need to grab the queue mutex since this thread will no longer receive any work events
      // Threads are bounded 1:1
      while (WorkQueue.size()) {
        WorkItem *Item = WorkQueue.front();
        WorkQueue.pop();
        delete Item;
      }

      // Go through the garbage collection array and clear it
      // It's safe to clear things that aren't marked safe since we are clearing cache
      if (GCArray.size()) {
        // Clean up our GC array
        for (auto it = GCArray.begin(); it != GCArray.end();) {
          delete *it;
          it = GCArray.erase(it);
        }
      }

      if (GuestRIP == 0) {
        CompileThreadData->IRLists.clear();
      }
      else {
        auto IR = CompileThreadData->IRLists.find(GuestRIP)->second.release();
        CompileThreadData->IRLists.clear();
        CompileThreadData->IRLists.try_emplace(GuestRIP, IR);
      }

      CompileMutex.unlock();
    }

    // Clear the inverse cache of what is calling us from the Context ClearCache routine
    auto SelectedThread = Thread->IsCompileService ? ParentThread : Thread;
    SelectedThread->LookupCache->ClearCache();
    SelectedThread->CPUBackend->ClearCache();
    SelectedThread->IntBackend->ClearCache();
  }

  void CompileService::RemoveCodeEntry(uint64_t GuestRIP) {
    CompileThreadData->IRLists.erase(GuestRIP);
    CompileThreadData->DebugData.erase(GuestRIP);
    CompileThreadData->LookupCache->Erase(GuestRIP);
  }

  CompileService::WorkItem *CompileService::CompileCode(uint64_t RIP) {
    // Tell the worker thread to compile code for us
    WorkItem *Item = new WorkItem{};
    Item->RIP = RIP;

    {
      // Fill the threads work queue
      std::scoped_lock<std::mutex> lk(QueueMutex);
      WorkQueue.emplace(Item);
    }

    // Notify the thread that it has more work
    StartWork.NotifyAll();

    return Item;
  }

  void CompileService::ExecutionThread() {
    // Ignore signals coming from the guest
    CTX->SignalDelegation->MaskThreadSignals();

    // Set our thread name so we can see its relation
    char ThreadName[16]{};
    snprintf(ThreadName, 16, "%ld-CS", ParentThread->State.ThreadManager.TID.load());
    pthread_setname_np(pthread_self(), ThreadName);

    while (true) {
      // Wait for work
      StartWork.Wait();
      if (ShuttingDown.load()) {
        break;
      }
      std::scoped_lock<std::mutex> lk(CompileMutex);

      size_t WorkItems{};

      do {
        // Grab a work item
        WorkItem *Item{};
        {
          std::scoped_lock<std::mutex> lk(QueueMutex);
          WorkItems = WorkQueue.size();
          if (WorkItems) {
            Item = WorkQueue.front();
            WorkQueue.pop();
          }
        }

        // If we had a work item then work on it
        if (Item) {
          // Does the block cache already contain this RIP?
          void *CompiledCode = reinterpret_cast<void*>(CompileThreadData->LookupCache->FindBlock(Item->RIP));
          FEXCore::Core::DebugData *DebugData = nullptr;

          if (!CompiledCode) {
            // Code isn't in cache, compile now
            // Set our thread state's RIP
            CompileThreadData->State.State.rip = Item->RIP;
            auto [Code, Data] = CTX->CompileCode(CompileThreadData.get(), Item->RIP);
            CompiledCode = Code;
            DebugData = Data;
          }

          if (!CompiledCode) {
            // XXX: We currently have the expectation that compile service code will be significantly smaller than regular thread's code
            ERROR_AND_DIE("Couldn't compile code for thread at RIP: 0x%lx", Item->RIP);
          }

          auto BlockMapPtr = CompileThreadData->LookupCache->AddBlockMapping(Item->RIP, CompiledCode);
          if (BlockMapPtr == 0) {
            // XXX: We currently have the expectation that compiler service block cache will be significantly underutilized compared to regular thread
            ERROR_AND_DIE("Couldn't add code to block cache for thread at RIP: 0x%lx", Item->RIP);
          }

          Item->CodePtr = CompiledCode;
          Item->IRList = CompileThreadData->IRLists.find(Item->RIP)->second.get();
          Item->DebugData = DebugData;

          GCArray.emplace_back(Item);
          Item->ServiceWorkDone.NotifyAll();
        }
      } while (WorkItems != 0);

      if (GCArray.size()) {
        // Clean up our GC array
        for (auto it = GCArray.begin(); it != GCArray.end();) {
          if ((*it)->SafeToClear) {
            delete *it;
            it = GCArray.erase(it);
          }
          else {
            ++it;
          }
        }
      }
    }
  }
}
