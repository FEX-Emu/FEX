#pragma once
#include <FEXCore/Core/Context.h>
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Core/CPUBackend.h>
#include <FEXCore/IR/IntrusiveIRList.h>
#include <FEXCore/Utils/Event.h>
#include <map>
#include <thread>

namespace FEXCore {
  class BlockCache;
}

namespace FEXCore::Context {
  struct Context;
}
namespace FEXCore::IR{
  class OpDispatchBuilder;
}

namespace FEXCore::Core {

  struct RuntimeStats {
    std::atomic_uint64_t InstructionsExecuted;
    std::atomic_uint64_t BlocksCompiled;
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
  };

  struct InternalThreadState {
    FEXCore::Core::ThreadState State;

    FEXCore::Context::Context *CTX;

    std::thread ExecutionThread;
    Event StartRunning;
    Event ThreadWaiting;

    std::unique_ptr<FEXCore::IR::OpDispatchBuilder> OpDispatcher;

    std::shared_ptr<FEXCore::CPU::CPUBackend> CPUBackend;
    std::shared_ptr<FEXCore::CPU::CPUBackend> IntBackend;
    std::unique_ptr<FEXCore::CPU::CPUBackend> FallbackBackend;

    std::unique_ptr<FEXCore::BlockCache> BlockCache;

    std::map<uint64_t, std::unique_ptr<FEXCore::IR::IRListView<true>>> IRLists;
    std::map<uint64_t, FEXCore::Core::DebugData> DebugData;
    RuntimeStats Stats{};

    int StatusCode{};
    FEXCore::Context::ExitReason ExitReason {FEXCore::Context::ExitReason::EXIT_WAITING};
  };
}


