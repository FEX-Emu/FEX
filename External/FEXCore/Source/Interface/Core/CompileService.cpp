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

  void CompileService::ClearCache(FEXCore::Core::InternalThreadState *Thread) {
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

      LogMan::Throw::A(CompileThreadData->IRLists.size() == 0, "Compile service must never have IRLists");
      LogMan::Throw::A(CompileThreadData->RALists.size() == 0, "Compile service must never have RALists");
      LogMan::Throw::A(CompileThreadData->DebugData.size() == 0, "Compile service must never have DebugData");

      CompileMutex.unlock();
    }

    // Clear the inverse cache of what is calling us from the Context ClearCache routine
    auto SelectedThread = Thread->IsCompileService ? ParentThread : Thread;
    SelectedThread->LookupCache->ClearCache();
    SelectedThread->CPUBackend->ClearCache();
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
          // Make sure it's not in lookup cache by accident
          LogMan::Throw::A(CompileThreadData->LookupCache->FindBlock(Item->RIP) == 0, "Compile Service must never have entries in the LookupCache");
          
          // Code isn't in cache, compile now
          // Set our thread state's RIP
          CompileThreadData->State.State.rip = Item->RIP;

          auto [CodePtr, IRList, DebugData, RAData, Generated, Min, Max] = CTX->CompileCode(CompileThreadData.get(), Item->RIP);

          LogMan::Throw::A(Generated == true, "Compile Service doesn't have IR Cache");

          if (!CodePtr) {
            // XXX: We currently have the expectation that compile service code will be significantly smaller than regular thread's code
            ERROR_AND_DIE("Couldn't compile code for thread at RIP: 0x%lx", Item->RIP);
          }

          Item->CodePtr = CodePtr;
          Item->IRList = IRList;
          Item->DebugData = DebugData;
          Item->RAData = RAData;

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
