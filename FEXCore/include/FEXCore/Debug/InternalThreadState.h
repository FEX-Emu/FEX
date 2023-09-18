#pragma once
#include <FEXCore/Core/Context.h>
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Core/CPUBackend.h>
#include <FEXCore/Core/SignalDelegator.h>
#include <FEXCore/IR/IntrusiveIRList.h>
#include <FEXCore/IR/RegisterAllocationData.h>
#include <FEXCore/Utils/Event.h>
#include <FEXCore/Utils/InterruptableConditionVariable.h>
#include <FEXCore/Utils/Threads.h>
#include <FEXCore/fextl/memory.h>
#include <FEXCore/fextl/robin_map.h>
#include <FEXCore/fextl/vector.h>
#include <FEXHeaderUtils/TypeDefines.h>

#include <chrono>
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
  struct DebugData : public FEXCore::Allocator::FEXAllocOperators {
    uint64_t HostCodeSize; ///< The size of the code generated in the host JIT
    fextl::vector<DebugDataSubblock> Subblocks;
    fextl::vector<DebugDataGuestOpcode> GuestOpcodes;
    fextl::vector<FEXCore::CPU::Relocation> *Relocations;
  };

  struct LocalIREntry {
    uint64_t StartAddr;
    uint64_t Length;
    fextl::unique_ptr<FEXCore::IR::IRListView, FEXCore::IR::IRListViewDeleter> IR;
    FEXCore::IR::RegisterAllocationData::UniquePtr RAData;
    fextl::unique_ptr<FEXCore::Core::DebugData> DebugData;
  };

  // Buffered JIT symbol tracking.
  struct JITSymbolBuffer {
    // Maximum buffer size to ensure we are a page in size.
    constexpr static size_t BUFFER_SIZE = 4096 - (8 * 2);
    // Maximum distance until the end of the buffer to do a write.
    constexpr static size_t NEEDS_WRITE_DISTANCE = BUFFER_SIZE - 64;
    // Maximum time threshhold to wait before a buffer write occurs.
    constexpr static std::chrono::milliseconds MAXIMUM_THRESHOLD {100};

    JITSymbolBuffer()
      : LastWrite {std::chrono::steady_clock::now()} {
    }
    // stead_clock to ensure a monotonic increasing clock.
    // In highly stressed situations this can still cause >2% CPU time in vdso_clock_gettime.
    // If we need lower CPU time when JIT symbols are enabled then FEX can read the cycle counter directly.
    std::chrono::steady_clock::time_point LastWrite{};
    size_t Offset{};
    char Buffer[BUFFER_SIZE]{};
  };
  static_assert(sizeof(JITSymbolBuffer) == 4096, "Ensure this is one page in size");

  struct InternalThreadState : public FEXCore::Allocator::FEXAllocOperators {
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
    fextl::unique_ptr<JITSymbolBuffer> SymbolBuffer;

    int StatusCode{};
    FEXCore::Context::ExitReason ExitReason {FEXCore::Context::ExitReason::EXIT_WAITING};
    std::shared_ptr<FEXCore::CompileService> CompileService;

    std::shared_mutex ObjectCacheRefCounter{};
    bool DestroyedByParent{false};  // Should the parent destroy this thread, or it destory itself

    struct DeferredSignalState {
#ifndef _WIN32
      siginfo_t Info;
#endif
      int Signal;
    };

    // Queue of thread local signal frames that have been deferred.
    // Async signals aren't guaranteed to be delivered in any particular order, but FEX treats them as FILO.
    fextl::vector<DeferredSignalState> DeferredSignalFrames;

    // BaseFrameState should always be at the end, directly before the interrupt fault page
    alignas(16) FEXCore::Core::CpuStateFrame BaseFrameState{};

    // Can be reprotected as RO to trigger an interrupt at generated code block entrypoints
    alignas(FHU::FEX_PAGE_SIZE) uint8_t InterruptFaultPage[FHU::FEX_PAGE_SIZE];
  };
  static_assert((offsetof(FEXCore::Core::InternalThreadState, InterruptFaultPage) -
                 offsetof(FEXCore::Core::InternalThreadState, BaseFrameState)) < 4096,
		"Fault page is outside of immediate range from CPU state");
  // static_assert(std::is_standard_layout<InternalThreadState>::value, "This needs to be standard layout");
}


