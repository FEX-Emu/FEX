#pragma once
#include <FEXCore/Core/Context.h>
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Core/CPUBackend.h>
#include <FEXCore/IR/IntrusiveIRList.h>
#include <FEXCore/IR/RegisterAllocationData.h>
#include <FEXCore/Utils/Event.h>
#include <FEXCore/Utils/InterruptableConditionVariable.h>
#include <FEXCore/Utils/Threads.h>

#include <unordered_map>
#include <shared_mutex>

namespace FEXCore {
  class LookupCache;
  class CompileService;
}

namespace FEXCore::Context {
  struct Context;
}

namespace FEXCore::CPU {
  union Relocation;
}

namespace FEXCore::Frontend {
  class Decoder;
}

namespace FEXCore::IR{
  class OpDispatchBuilder;
  class PassManager;
}

namespace FEXCore::Core {

  struct RuntimeStats {
    std::atomic_uint64_t InstructionsExecuted;
    std::atomic_uint64_t BlocksCompiled;
  };

  struct DebugDataSubblock {
    uintptr_t HostCodeStart;
    uint32_t HostCodeSize;
  };

  /**
   * @brief Contains debug data for a block of code for later debugger analysis
   *
   * Needs to remain around for as long as the code could be executed at least
   */
  struct DebugData {
    uint64_t HostCodeSize; ///< The size of the code generated in the host JIT
    std::vector<DebugDataSubblock> Subblocks;
    std::vector<FEXCore::CPU::Relocation> *Relocations;
  };

  enum class SignalEvent {
    Nothing, // If the guest uses our signal we need to know it was errant on our end
    Pause,
    Stop,
    Return,
  };

  struct LocalIREntry {
    uint64_t StartAddr;
    uint64_t Length;
    std::unique_ptr<FEXCore::IR::IRListView, FEXCore::IR::IRListViewDeleter> IR;
    std::unique_ptr<FEXCore::IR::RegisterAllocationData, FEXCore::IR::RegisterAllocationDataDeleter> RAData;
    std::unique_ptr<FEXCore::Core::DebugData> DebugData;
  };

  struct InternalThreadState {
    FEXCore::Core::CpuStateFrame* const CurrentFrame = &BaseFrameState;

    struct {
      std::atomic_bool Running {false};
      std::atomic_bool WaitingToStart {true};
      std::atomic_bool EarlyExit {false};
      std::atomic_bool ThreadSleeping {false};
    } RunningEvents;

    FEXCore::Context::Context *CTX;
    std::atomic<SignalEvent> SignalReason{SignalEvent::Nothing};

    std::unique_ptr<FEXCore::Threads::Thread> ExecutionThread;
    InterruptableConditionVariable StartRunning;
    Event ThreadWaiting;

    std::unique_ptr<FEXCore::IR::OpDispatchBuilder> OpDispatcher;

    std::unique_ptr<FEXCore::CPU::CPUBackend> CPUBackend;
    std::unique_ptr<FEXCore::LookupCache> LookupCache;

    std::unordered_map<uint64_t, LocalIREntry> LocalIRCache;

    std::unique_ptr<FEXCore::Frontend::Decoder> FrontendDecoder;
    std::unique_ptr<FEXCore::IR::PassManager> PassManager;
    FEXCore::HLE::ThreadManagement ThreadManager;

    RuntimeStats Stats{};

    int StatusCode{};
    FEXCore::Context::ExitReason ExitReason {FEXCore::Context::ExitReason::EXIT_WAITING};
    std::shared_ptr<FEXCore::CompileService> CompileService;

    std::shared_mutex ObjectCacheRefCounter{};
    bool DestroyedByParent{false};  // Should the parent destroy this thread, or it destory itself

    alignas(16) FEXCore::Core::CpuStateFrame BaseFrameState{};

  };
  // static_assert(std::is_standard_layout<InternalThreadState>::value, "This needs to be standard layout");
}


