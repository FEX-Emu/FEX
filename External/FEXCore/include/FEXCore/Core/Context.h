#pragma once
#include <functional>
#include <stdint.h>
#include <string>

#include <FEXCore/Core/SignalDelegator.h>
#include <FEXCore/Core/CPUID.h>
#include <FEXCore/IR/IR.h>
#include <FEXCore/Utils/CompilerDefs.h>
#include <FEXCore/fextl/vector.h>

#include <istream>
#include <ostream>
#include <memory>
#include <set>
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

  struct VDSOSigReturn {
    void *VDSO_kernel_sigreturn;
    void *VDSO_kernel_rt_sigreturn;
  };

  using CustomCPUFactoryType = std::function<std::unique_ptr<FEXCore::CPU::CPUBackend> (FEXCore::Context::Context*, FEXCore::Core::InternalThreadState *Thread)>;

  using ExitHandler = std::function<void(uint64_t ThreadId, FEXCore::Context::ExitReason)>;

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
      FEX_DEFAULT_VISIBILITY static FEXCore::Context::Context *CreateNewContext();

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
       * @brief Destroy the context object
       *
       * @param CTX
       */
      FEX_DEFAULT_VISIBILITY static void DestroyContext(FEXCore::Context::Context * CTX);
      FEX_DEFAULT_VISIBILITY virtual void DestroyContext() = 0;

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

      FEX_DEFAULT_VISIBILITY virtual void CompileRIP(FEXCore::Core::InternalThreadState *Thread, uint64_t GuestRIP) = 0;

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
       * @brief Sets up memory regions on the guest for mirroring within the guest's VM space
       *
       * @param VirtualAddress The address we want to set to mirror a physical memory region
       * @param PhysicalAddress The physical memory region we are mapping
       * @param Size Size of the region to mirror
       *
       * @return true when successfully mapped. false if there was an error adding
       */
      FEX_DEFAULT_VISIBILITY virtual bool AddVirtualMemoryMapping(uint64_t VirtualAddress, uint64_t PhysicalAddress, uint64_t Size) = 0;

      /**
       * @brief Retrieves a feature struct indicating certain supported aspects from
       *        the hose.
       *
       * @param CTX A valid non-null context instance.
       */
      FEX_DEFAULT_VISIBILITY virtual HostFeatures GetHostFeatures() const = 0;

      FEX_DEFAULT_VISIBILITY virtual void HandleCallback(FEXCore::Core::InternalThreadState *Thread, uint64_t RIP) = 0;

      FEX_DEFAULT_VISIBILITY virtual void RegisterHostSignalHandler(int Signal, HostSignalDelegatorFunction Func, bool Required) = 0;
      [[noreturn]] FEX_DEFAULT_VISIBILITY virtual void HandleSignalHandlerReturn(bool RT) = 0;
      FEX_DEFAULT_VISIBILITY virtual void RegisterFrontendHostSignalHandler(int Signal, HostSignalDelegatorFunction Func, bool Required) = 0;

      FEX_DEFAULT_VISIBILITY virtual FEXCore::Core::InternalThreadState* CreateThread(FEXCore::Core::CPUState *NewThreadState, uint64_t ParentTID) = 0;
      FEX_DEFAULT_VISIBILITY virtual void ExecutionThread(FEXCore::Core::InternalThreadState *Thread) = 0;
      FEX_DEFAULT_VISIBILITY virtual void InitializeThread(FEXCore::Core::InternalThreadState *Thread) = 0;
      FEX_DEFAULT_VISIBILITY virtual void RunThread(FEXCore::Core::InternalThreadState *Thread) = 0;
      FEX_DEFAULT_VISIBILITY virtual void StopThread(FEXCore::Core::InternalThreadState *Thread) = 0;
      FEX_DEFAULT_VISIBILITY virtual void DestroyThread(FEXCore::Core::InternalThreadState *Thread) = 0;
      FEX_DEFAULT_VISIBILITY virtual void CleanupAfterFork(FEXCore::Core::InternalThreadState *Thread) = 0;
      FEX_DEFAULT_VISIBILITY virtual void SetSignalDelegator(FEXCore::SignalDelegator *SignalDelegation) = 0;
      FEX_DEFAULT_VISIBILITY virtual void SetSyscallHandler(FEXCore::HLE::SyscallHandler *Handler) = 0;
      FEX_DEFAULT_VISIBILITY virtual FEXCore::CPUID::FunctionResults RunCPUIDFunction(uint32_t Function, uint32_t Leaf) = 0;
      FEX_DEFAULT_VISIBILITY virtual FEXCore::CPUID::FunctionResults RunCPUIDFunctionName(uint32_t Function, uint32_t Leaf, uint32_t CPU) = 0;

      FEX_DEFAULT_VISIBILITY virtual FEXCore::IR::AOTIRCacheEntry *LoadAOTIRCacheEntry(const std::string& Name) = 0;
      FEX_DEFAULT_VISIBILITY virtual void UnloadAOTIRCacheEntry(FEXCore::IR::AOTIRCacheEntry *Entry) = 0;

      FEX_DEFAULT_VISIBILITY virtual void SetAOTIRLoader(std::function<int(const std::string&)> CacheReader) = 0;
      FEX_DEFAULT_VISIBILITY virtual void SetAOTIRWriter(std::function<std::unique_ptr<std::ofstream>(const std::string&)> CacheWriter) = 0;
      FEX_DEFAULT_VISIBILITY virtual void SetAOTIRRenamer(std::function<void(const std::string&)> CacheRenamer) = 0;

      FEX_DEFAULT_VISIBILITY virtual void FinalizeAOTIRCache() = 0;
      FEX_DEFAULT_VISIBILITY virtual void WriteFilesWithCode(std::function<void(const std::string& fileid, const std::string& filename)> Writer) = 0;
      FEX_DEFAULT_VISIBILITY virtual void InvalidateGuestCodeRange(uint64_t Start, uint64_t Length) = 0;
      FEX_DEFAULT_VISIBILITY virtual void InvalidateGuestCodeRange(uint64_t Start, uint64_t Length, std::function<void(uint64_t start, uint64_t Length)> callback) = 0;
      FEX_DEFAULT_VISIBILITY virtual void MarkMemoryShared() = 0;

      FEX_DEFAULT_VISIBILITY virtual void ConfigureAOTGen(FEXCore::Core::InternalThreadState *Thread, std::set<uint64_t> *ExternalBranches, uint64_t SectionMaxAddress) = 0;
      FEX_DEFAULT_VISIBILITY virtual CustomIRResult AddCustomIREntrypoint(uintptr_t Entrypoint, std::function<void(uintptr_t Entrypoint, FEXCore::IR::IREmitter *)> Handler, void *Creator = nullptr, void *Data = nullptr) = 0;

      /**
       * @brief Allows the frontend to register its own thunk handlers independent of what is controlled in the backend.
       *
       * @param CTX A valid non-null context instance.
       * @param Definitions A vector of thunk definitions that the frontend controls
       */
      FEX_DEFAULT_VISIBILITY virtual void AppendThunkDefinitions(fextl::vector<FEXCore::IR::ThunkDefinition> const& Definitions) = 0;

      FEX_DEFAULT_VISIBILITY virtual void SetVDSOSigReturn(const VDSOSigReturn &Pointers) = 0;
    private:
  };

  /**
   * @brief This initializes internal FEXCore state that is shared between contexts and requires overhead to setup
   */
  FEX_DEFAULT_VISIBILITY void InitializeStaticTables(OperatingMode Mode = MODE_64BIT);
  FEX_DEFAULT_VISIBILITY void ShutdownStaticTables();
}
