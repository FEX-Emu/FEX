#pragma once
#include <FEXCore/Core/Context.h>
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Core/CPUBackend.h>
#include <FEXCore/IR/IntrusiveIRList.h>
#include <FEXCore/IR/RegisterAllocationData.h>
#include <FEXCore/Utils/Event.h>
#include <FEXCore/Utils/InterruptableConditionVariable.h>
#include <FEXCore/Utils/Threads.h>
#include <FEXCore/fextl/memory.h>
#include <FEXCore/fextl/robin_map.h>
#include <FEXCore/fextl/vector.h>

#include <shared_mutex>
#include <type_traits>

namespace FEXCore {
  class LookupCache;
  class CompileService;
}

namespace FEXCore::Context {
  class Context;
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
    uint32_t HostCodeOffset;
    uint32_t HostCodeSize;
  };

  struct DebugDataGuestOpcode {
    uint64_t GuestEntryOffset;
    ptrdiff_t HostEntryOffset;
  };

  /**
   * @brief Contains debug data for a block of code for later debugger analysis
   *
   * Needs to remain around for as long as the code could be executed at least
   */
  struct DebugData {
    uint64_t HostCodeSize; ///< The size of the code generated in the host JIT
    fextl::vector<DebugDataSubblock> Subblocks;
    fextl::vector<DebugDataGuestOpcode> GuestOpcodes;
    fextl::vector<FEXCore::CPU::Relocation> *Relocations;

    // Required due to raw new usage.
    void *operator new(size_t size) {
      return FEXCore::Allocator::malloc(size);
    }

    void operator delete(void *ptr) {
      return FEXCore::Allocator::free(ptr);
    }
  };

  enum class SignalEvent {
    Nothing, // If the guest uses our signal we need to know it was errant on our end
    Pause,
    Stop,
    Return,
    ReturnRT,
  };

  struct LocalIREntry {
    uint64_t StartAddr;
    uint64_t Length;
    fextl::unique_ptr<FEXCore::IR::IRListView, FEXCore::IR::IRListViewDeleter> IR;
    FEXCore::IR::RegisterAllocationData::UniquePtr RAData;
    fextl::unique_ptr<FEXCore::Core::DebugData> DebugData;
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

    fextl::unique_ptr<FEXCore::Threads::Thread> ExecutionThread;
    bool StartPaused {false};
    InterruptableConditionVariable StartRunning;
    Event ThreadWaiting;

    fextl::unique_ptr<FEXCore::IR::OpDispatchBuilder> OpDispatcher;

    fextl::unique_ptr<FEXCore::CPU::CPUBackend> CPUBackend;
    fextl::unique_ptr<FEXCore::LookupCache> LookupCache;

    fextl::robin_map<uint64_t, LocalIREntry> DebugStore;

    fextl::unique_ptr<FEXCore::Frontend::Decoder> FrontendDecoder;
    fextl::unique_ptr<FEXCore::IR::PassManager> PassManager;
    FEXCore::HLE::ThreadManagement ThreadManager;

    RuntimeStats Stats{};

    int StatusCode{};
    FEXCore::Context::ExitReason ExitReason {FEXCore::Context::ExitReason::EXIT_WAITING};
    std::shared_ptr<FEXCore::CompileService> CompileService;

    std::shared_mutex ObjectCacheRefCounter{};
    bool DestroyedByParent{false};  // Should the parent destroy this thread, or it destory itself

    // Required due to raw new usage.
    void *operator new(size_t size) {
      return FEXCore::Allocator::malloc(size);
    }

    void operator delete(void *ptr) {
      return FEXCore::Allocator::free(ptr);
    }

    alignas(16) FEXCore::Core::CpuStateFrame BaseFrameState{};

  };
  // static_assert(std::is_standard_layout<InternalThreadState>::value, "This needs to be standard layout");
}


