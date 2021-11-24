#pragma once

#include <FEXCore/Debug/InternalThreadState.h>
#include <FEXCore/Utils/Event.h>
#include <FEXCore/Utils/Threads.h>

#include <atomic>
#include <memory>
#include <mutex>
#include <queue>
#include <stdint.h>
#include <vector>

namespace FEXCore {
namespace Context {
  struct Context;
}
namespace IR {
  class IRListView;
  class RegisterAllocationData;
};
class CompileService final {
  public:
    CompileService(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread);
    void Initialize();
    void Shutdown();

    struct WorkItem {
      // Incoming
      uint64_t RIP{};

      // Outgoing
      void *CodePtr{};
      FEXCore::IR::IRListView *IRList{};
      FEXCore::IR::RegisterAllocationData *RAData{};
      FEXCore::Core::DebugData *DebugData{};
      uint64_t StartAddr;
      uint64_t Length;

      // Communication
      Event ServiceWorkDone{};
      std::atomic_bool SafeToClear{};
    };

    WorkItem *CompileCode(uint64_t RIP);
    void ClearCache(FEXCore::Core::InternalThreadState *Thread);

    // Public for threading
    void ExecutionThread();

    bool IsAddressInJITCode(uint64_t Address) const {
      return CompileThreadData->CPUBackend->IsAddressInJITCode(Address, false, false);
    }
  private:
    FEXCore::Context::Context *CTX;
    FEXCore::Core::InternalThreadState *ParentThread;

    std::unique_ptr<FEXCore::Threads::Thread> WorkerThread;
    std::unique_ptr<FEXCore::Core::InternalThreadState> CompileThreadData;

    std::mutex QueueMutex{};
    std::mutex CompileMutex{};
    std::queue<std::unique_ptr<WorkItem>> WorkQueue{};
    std::vector<std::unique_ptr<WorkItem>> GCArray{};
    Event StartWork{};
    std::atomic_bool ShuttingDown{false};
};
}
