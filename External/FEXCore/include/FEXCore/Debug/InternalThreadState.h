#pragma once
#include <FEXCore/Core/Context.h>
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Core/CPUBackend.h>
#include <FEXCore/IR/IntrusiveIRList.h>
#include <FEXCore/IR/RegisterAllocationData.h>
#include <FEXCore/Utils/Event.h>
#include <FEXCore/Utils/Threads.h>

#include <unordered_map>

namespace FEXCore {
  class LookupCache;
  class CompileService;
}

namespace FEXCore::Context {
  struct Context;
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
    uint32_t SSAId;
  };

  /**
   * @brief Contains debug data for a block of code for later debugger analysis
   *
   * Needs to remain around for as long as the code could be executed at least
   */
  struct DebugData {
    uint64_t HostCodeSize; ///< The size of the code generated in the host JIT
    uint64_t GuestCodeSize; ///< The size of the guest side code
    uint64_t GuestInstructionCount; ///< Number of guest instructions
    uint64_t TimeSpentInCode; ///< How long this code has spent time running
    uint64_t RunCount; ///< Number of times this block of code has been run
    std::vector<DebugDataSubblock> Subblocks;
  };

  enum SignalEvent {
    SIGNALEVENT_NONE, // If the guest uses our signal we need to know it was errant on our end
    SIGNALEVENT_PAUSE,
    SIGNALEVENT_STOP,
    SIGNALEVENT_RETURN,
  };

  struct LocalIREntry {
    uint64_t StartAddr;
    uint64_t Length;
    std::unique_ptr<FEXCore::IR::IRListView, FEXCore::IR::IRListViewDeleter> IR;
    std::unique_ptr<FEXCore::IR::RegisterAllocationData, FEXCore::IR::RegisterAllocationDataDeleter> RAData;
    std::unique_ptr<FEXCore::Core::DebugData> DebugData;
  };

  struct InternalThreadState {
    FEXCore::Core::CpuStateFrame* CurrentFrame = &BaseFrameState;

    struct {
      std::atomic_bool Running {false};
      std::atomic_bool WaitingToStart {false};
    } RunningEvents;

    FEXCore::Context::Context *CTX;
    std::atomic<SignalEvent> SignalReason {SignalEvent::SIGNALEVENT_NONE};

    std::unique_ptr<FEXCore::Threads::Thread> ExecutionThread;
    Event StartRunning;
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
    uint32_t CompileBlockReentrantRefCount{};
    std::shared_ptr<FEXCore::CompileService> CompileService;
    bool IsCompileService{false};
    bool DestroyedByParent{false};  // Should the parent destroy this thread, or it destory itself

    alignas(16) FEXCore::Core::CpuStateFrame BaseFrameState{};

  };
  // static_assert(std::is_standard_layout<InternalThreadState>::value, "This needs to be standard layout");
}


