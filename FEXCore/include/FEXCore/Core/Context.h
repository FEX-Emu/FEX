// SPDX-License-Identifier: MIT
#pragma once
#include <functional>
#include <stdint.h>

#include <FEXCore/Core/SignalDelegator.h>
#include <FEXCore/Core/CPUID.h>
#include <FEXCore/IR/IR.h>
#include <FEXCore/Utils/CompilerDefs.h>
#include <FEXCore/fextl/memory.h>
#include <FEXCore/fextl/set.h>
#include <FEXCore/fextl/string.h>
#include <FEXCore/fextl/vector.h>

#include <istream>
#include <ostream>
#include <mutex>
#include <shared_mutex>

namespace FEXCore {
  class CodeLoader;
  class HostFeatures;
}

namespace FEXCore::Core {
  struct CPUState;
  struct InternalThreadState;
}

namespace FEXCore::CPU {
  class CPUBackend;
}

namespace FEXCore::HLE {
  struct SyscallArguments;
  class SyscallVisitor;
  class SyscallHandler;
}

namespace FEXCore::IR {
  struct AOTIRCacheEntry;
  class IREmitter;
}

namespace FEXCore::Context {
  class Context;
  enum ExitReason {
    EXIT_NONE,
    EXIT_WAITING,
    EXIT_ASYNC_RUN,
    EXIT_SHUTDOWN,
    EXIT_DEBUG,
    EXIT_UNKNOWNERROR,
  };

  enum OperatingMode {
    MODE_32BIT,
    MODE_64BIT,
  };

  struct CustomIRResult {
    void *Creator;
    void *Data;

    explicit operator bool() const noexcept { return !lock; }

    CustomIRResult(std::unique_lock<std::shared_mutex> &&lock, void *Creator, void *Data):
      Creator(Creator), Data(Data), lock(std::move(lock)) { }

    private:
      std::unique_lock<std::shared_mutex> lock;
  };

  /**
   * @brief IR Serialization handler class.
   */
  class AOTIRWriter {
    public:
      virtual ~AOTIRWriter() = default;
      virtual void Write(const void* Data, size_t Size) = 0;
      virtual size_t Offset() = 0;
      virtual void Close() = 0;
  };

  struct VDSOSigReturn {
    void *VDSO_kernel_sigreturn;
    void *VDSO_kernel_rt_sigreturn;
  };

  struct ThreadsState {
    FEXCore::Core::InternalThreadState* ParentThread;
    fextl::vector<FEXCore::Core::InternalThreadState*>* Threads;
  };

  using CodeRangeInvalidationFn = std::function<void(uint64_t start, uint64_t Length)>;

  using CustomCPUFactoryType = std::function<fextl::unique_ptr<CPU::CPUBackend>(Context*, Core::InternalThreadState *Thread)>;
  using CustomIREntrypointHandler = std::function<void(uintptr_t Entrypoint, IR::IREmitter *)>;

  using ExitHandler = std::function<void(uint64_t ThreadId, ExitReason)>;

  using AOTIRCodeFileWriterFn = std::function<void(const fextl::string& fileid, const fextl::string& filename)>;
  using AOTIRLoaderCBFn = std::function<int(const fextl::string&)>;
  using AOTIRRenamerCBFn = std::function<void(const fextl::string&)>;
  using AOTIRWriterCBFn = std::function<fextl::unique_ptr<AOTIRWriter>(const fextl::string&)>;

  class Context {
    public:
      virtual ~Context() = default;
      /**
       * @brief [[threadsafe]] Create a new FEXCore context object
       *
       * This is necessary to do when running threaded contexts
       *
       * @return a new context object
       */
      FEX_DEFAULT_VISIBILITY static fextl::unique_ptr<FEXCore::Context::Context> CreateNewContext();

      /**
       * @brief Post creation context initialization
       * Once configurations have been set, do the post-creation initialization with that configuration
       *
       * @param CTX The context that we created
       *
       * @return true if we managed to initialize correctly
       */
      FEX_DEFAULT_VISIBILITY virtual bool InitializeContext() = 0;

      /**
       * @brief Allows setting up in memory code and other things prior to launchign code execution
       *
       * @param CTX The context that we created
       * @param Loader The loader that will be doing all the code loading
       *
       * @return true if we loaded code
       */
      FEX_DEFAULT_VISIBILITY virtual FEXCore::Core::InternalThreadState* InitCore(uint64_t InitialRIP, uint64_t StackPointer) = 0;

      FEX_DEFAULT_VISIBILITY virtual void SetExitHandler(ExitHandler handler) = 0;
      FEX_DEFAULT_VISIBILITY virtual ExitHandler GetExitHandler() const = 0;

      /**
       * @brief Pauses execution on the CPU core
       *
       * Blocks until all threads have paused.
       */
      FEX_DEFAULT_VISIBILITY virtual void Pause() = 0;

      /**
       * @brief Waits for all threads to be idle.
       *
       * Idling can happen when the process is shutting down or the debugger has asked for all threads to pause.
       */
      FEX_DEFAULT_VISIBILITY virtual void WaitForIdle() = 0;

      /**
       * @brief When resuming from a paused state, waits for all threads to start executing before returning.
       */
      FEX_DEFAULT_VISIBILITY virtual void WaitForThreadsToRun() = 0;

      /**
       * @brief Starts (or continues) the CPU core
       *
       * This function is async and returns immediately.
       * Use RunUntilExit() for synchonous executions
       *
       */
      FEX_DEFAULT_VISIBILITY virtual void Run() = 0;

      /**
       * @brief Tells the core to shutdown
       *
       * Blocks until shutdown
       */
      FEX_DEFAULT_VISIBILITY virtual void Stop() = 0;

      /**
       * @brief Executes one instruction
       *
       * Returns once execution is complete.
       */
      FEX_DEFAULT_VISIBILITY virtual void Step() = 0;

      /**
       * @brief Runs the CPU core until it exits
       *
       * If an Exit handler has been registered, this function won't return until the core
       * has shutdown.
       *
       * @param CTX The context that we created
       *
       * @return The ExitReason for the parentthread.
       */
      FEX_DEFAULT_VISIBILITY virtual ExitReason RunUntilExit() = 0;

      /**
       * @brief Executes the supplied thread context on the current thread until a return is requested
       */
      FEX_DEFAULT_VISIBILITY virtual void ExecuteThread(FEXCore::Core::InternalThreadState *Thread) = 0;

      FEX_DEFAULT_VISIBILITY virtual void CompileRIP(FEXCore::Core::InternalThreadState *Thread, uint64_t GuestRIP) = 0;
      FEX_DEFAULT_VISIBILITY virtual void CompileRIPCount(FEXCore::Core::InternalThreadState *Thread, uint64_t GuestRIP, uint64_t MaxInst) = 0;

      /**
       * @brief Gets the program exit status
       *
       *
       * @param CTX The context that we created
       *
       * @return The program exit status
       */
      FEX_DEFAULT_VISIBILITY virtual int GetProgramStatus() const = 0;

      /**
       * @brief [[threadsafe]] Returns the ExitReason of the parent thread. Typically used for async result status
       *
       * @param CTX The context that we created
       *
       * @return The ExitReason for the parentthread
       */
      FEX_DEFAULT_VISIBILITY virtual ExitReason GetExitReason() = 0;

      /**
       * @brief [[theadsafe]] Checks if the Context is either done working or paused(in the case of single stepping)
       *
       * Use this when the context is async running to determine if it is done
       *
       * @param CTX the context that we created
       *
       * @return true if the core is done or paused
       */
      FEX_DEFAULT_VISIBILITY virtual bool IsDone() const = 0;

      /**
       * @brief Gets a copy the CPUState of the parent thread
       *
       * @param CTX The context that we created
       * @param State The state object to populate
       */
      FEX_DEFAULT_VISIBILITY virtual void GetCPUState(FEXCore::Core::CPUState *State) const = 0;

      /**
       * @brief Copies the CPUState provided to the parent thread
       *
       * @param CTX The context that we created
       * @param State The satate object to copy from
       */
      FEX_DEFAULT_VISIBILITY virtual void SetCPUState(const FEXCore::Core::CPUState *State) = 0;

      /**
       * @brief Allows the frontend to pass in a custom CPUBackend creation factory
       *
       * This allows the frontend to have its own frontend. Typically for debugging
       *
       * @param CTX The context that we created
       * @param Factory The factory that the context will call if the DefaultCore config ise set to CUSTOM
       */
      FEX_DEFAULT_VISIBILITY virtual void SetCustomCPUBackendFactory(CustomCPUFactoryType Factory) = 0;

      /**
       * @brief Retrieves a feature struct indicating certain supported aspects from
       *        the hose.
       *
       * @param CTX A valid non-null context instance.
       */
      FEX_DEFAULT_VISIBILITY virtual HostFeatures GetHostFeatures() const = 0;

      FEX_DEFAULT_VISIBILITY virtual void HandleCallback(FEXCore::Core::InternalThreadState *Thread, uint64_t RIP) = 0;

      ///< State reconstruction helpers
      ///< Reconstructs the guest RIP from the passed in thread context and related Host PC.
      FEX_DEFAULT_VISIBILITY virtual uint64_t RestoreRIPFromHostPC(FEXCore::Core::InternalThreadState *Thread, uint64_t HostPC) = 0;
      ///< Reconstructs a compacted EFLAGS from FEX's internal EFLAG representation.
      FEX_DEFAULT_VISIBILITY virtual uint32_t ReconstructCompactedEFLAGS(FEXCore::Core::InternalThreadState *Thread) = 0;
      ///< Sets FEX's internal EFLAGS representation to the passed in compacted form.
      FEX_DEFAULT_VISIBILITY virtual void SetFlagsFromCompactedEFLAGS(FEXCore::Core::InternalThreadState *Thread, uint32_t EFLAGS) = 0;

      FEX_DEFAULT_VISIBILITY virtual FEXCore::Core::InternalThreadState* CreateThread(FEXCore::Core::CPUState *NewThreadState, uint64_t ParentTID) = 0;
      FEX_DEFAULT_VISIBILITY virtual void ExecutionThread(FEXCore::Core::InternalThreadState *Thread) = 0;
      FEX_DEFAULT_VISIBILITY virtual void InitializeThread(FEXCore::Core::InternalThreadState *Thread) = 0;
      FEX_DEFAULT_VISIBILITY virtual void RunThread(FEXCore::Core::InternalThreadState *Thread) = 0;
      FEX_DEFAULT_VISIBILITY virtual void StopThread(FEXCore::Core::InternalThreadState *Thread) = 0;
      FEX_DEFAULT_VISIBILITY virtual void DestroyThread(FEXCore::Core::InternalThreadState *Thread) = 0;
#ifndef _WIN32
      FEX_DEFAULT_VISIBILITY virtual void LockBeforeFork(FEXCore::Core::InternalThreadState *Thread) {}
      FEX_DEFAULT_VISIBILITY virtual void UnlockAfterFork(FEXCore::Core::InternalThreadState *Thread, bool Child) {}
#endif
      FEX_DEFAULT_VISIBILITY virtual void SetSignalDelegator(FEXCore::SignalDelegator *SignalDelegation) = 0;
      FEX_DEFAULT_VISIBILITY virtual void SetSyscallHandler(FEXCore::HLE::SyscallHandler *Handler) = 0;

      FEX_DEFAULT_VISIBILITY virtual FEXCore::CPUID::FunctionResults RunCPUIDFunction(uint32_t Function, uint32_t Leaf) = 0;
      FEX_DEFAULT_VISIBILITY virtual FEXCore::CPUID::XCRResults RunXCRFunction(uint32_t Function) = 0;
      FEX_DEFAULT_VISIBILITY virtual FEXCore::CPUID::FunctionResults RunCPUIDFunctionName(uint32_t Function, uint32_t Leaf, uint32_t CPU) = 0;

      FEX_DEFAULT_VISIBILITY virtual FEXCore::IR::AOTIRCacheEntry *LoadAOTIRCacheEntry(const fextl::string& Name) = 0;
      FEX_DEFAULT_VISIBILITY virtual void UnloadAOTIRCacheEntry(FEXCore::IR::AOTIRCacheEntry *Entry) = 0;

      FEX_DEFAULT_VISIBILITY virtual void SetAOTIRLoader(AOTIRLoaderCBFn CacheReader) = 0;
      FEX_DEFAULT_VISIBILITY virtual void SetAOTIRWriter(AOTIRWriterCBFn CacheWriter) = 0;
      FEX_DEFAULT_VISIBILITY virtual void SetAOTIRRenamer(AOTIRRenamerCBFn CacheRenamer) = 0;

      FEX_DEFAULT_VISIBILITY virtual void FinalizeAOTIRCache() = 0;
      FEX_DEFAULT_VISIBILITY virtual void WriteFilesWithCode(AOTIRCodeFileWriterFn Writer) = 0;
      FEX_DEFAULT_VISIBILITY virtual void InvalidateGuestCodeRange(FEXCore::Core::InternalThreadState *Thread, uint64_t Start, uint64_t Length) = 0;
      FEX_DEFAULT_VISIBILITY virtual void InvalidateGuestCodeRange(FEXCore::Core::InternalThreadState *Thread, uint64_t Start, uint64_t Length, CodeRangeInvalidationFn callback) = 0;
      FEX_DEFAULT_VISIBILITY virtual void MarkMemoryShared() = 0;

      FEX_DEFAULT_VISIBILITY virtual void ConfigureAOTGen(FEXCore::Core::InternalThreadState *Thread, fextl::set<uint64_t> *ExternalBranches, uint64_t SectionMaxAddress) = 0;
      FEX_DEFAULT_VISIBILITY virtual CustomIRResult AddCustomIREntrypoint(uintptr_t Entrypoint, CustomIREntrypointHandler Handler, void *Creator = nullptr, void *Data = nullptr) = 0;

      /**
       * @brief Allows the frontend to register its own thunk handlers independent of what is controlled in the backend.
       *
       * @param CTX A valid non-null context instance.
       * @param Definitions A vector of thunk definitions that the frontend controls
       */
      FEX_DEFAULT_VISIBILITY virtual void AppendThunkDefinitions(fextl::vector<FEXCore::IR::ThunkDefinition> const& Definitions) = 0;

      FEX_DEFAULT_VISIBILITY virtual void GetVDSOSigReturn(VDSOSigReturn *VDSOPointers) = 0;

      FEX_DEFAULT_VISIBILITY virtual void IncrementIdleRefCount() = 0;

      /**
       * @brief Informs the context if hardware TSO is supported.
       * Once hardware TSO is enabled, then TSO emulation through atomics is disabled and relies on the hardware.
       *
       * @param HardwareTSOSupported If the hardware supports the TSO memory model or not.
       */
      FEX_DEFAULT_VISIBILITY virtual void SetHardwareTSOSupport(bool HardwareTSOSupported) = 0;

      /**
       * @brief Enable exiting the JIT when HLT is hit.
       *
       * This is to workaround a bug in Wine's longjump function which breaks our unittests.
       *
       */
      FEX_DEFAULT_VISIBILITY virtual void EnableExitOnHLT() = 0;

      /**
       * @brief Gets the thread data for FEX's internal tracked threads.
       *
       * @return struct containing all the thread information.
       */
      FEX_DEFAULT_VISIBILITY virtual ThreadsState GetThreads() = 0;

    private:
  };

  /**
   * @brief This initializes internal FEXCore state that is shared between contexts and requires overhead to setup
   */
  FEX_DEFAULT_VISIBILITY void InitializeStaticTables(OperatingMode Mode = MODE_64BIT);
}
