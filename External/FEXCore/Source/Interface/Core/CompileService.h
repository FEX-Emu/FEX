#pragma once

#include <FEXCore/Core/CPUBackend.h>
#include <FEXCore/Utils/Event.h>

#include <memory>
#include <thread>
#include <unordered_map>
#include <queue>

namespace FEXCore {
namespace Context {
  struct Context;
}
namespace Core {
  struct InternalThreadState;
}

namespace IR {
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

      // Communication
      Event ServiceWorkDone{};
      std::atomic_bool SafeToClear{};
    };

    WorkItem *CompileCode(uint64_t RIP);
    void ClearCache(FEXCore::Core::InternalThreadState *Thread);

  private:
    FEXCore::Context::Context *CTX;
    FEXCore::Core::InternalThreadState *ParentThread;

    void ExecutionThread();
    std::thread WorkerThread;
    std::unique_ptr<FEXCore::Core::InternalThreadState> CompileThreadData;

    std::mutex QueueMutex{};
    std::mutex CompileMutex{};
    std::queue<WorkItem*> WorkQueue{};
    std::vector<WorkItem*> GCArray{};
    Event StartWork{};
    std::atomic_bool ShuttingDown{false};
};
}
