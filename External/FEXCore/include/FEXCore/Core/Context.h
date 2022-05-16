#pragma once
#include <functional>
#include <stdint.h>
#include <string>

#include <FEXCore/Core/SignalDelegator.h>
#include <FEXCore/Core/CPUID.h>
#include <FEXCore/Utils/CompilerDefs.h>

#include <istream>
#include <ostream>
#include <memory>
#include <set>

namespace FEXCore {
  class CodeLoader;
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
}

namespace FEXCore::Context {
  struct Context;
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

  using CustomCPUFactoryType = std::function<std::unique_ptr<FEXCore::CPU::CPUBackend> (FEXCore::Context::Context*, FEXCore::Core::InternalThreadState *Thread)>;

  using ExitHandler = std::function<void(uint64_t ThreadId, FEXCore::Context::ExitReason)>;

  /**
   * @brief This initializes internal FEXCore state that is shared between contexts and requires overhead to setup
   */
  FEX_DEFAULT_VISIBILITY void InitializeStaticTables(OperatingMode Mode = MODE_64BIT);
  FEX_DEFAULT_VISIBILITY void ShutdownStaticTables();

  /**
   * @brief [[threadsafe]] Create a new FEXCore context object
   *
   * This is necessary to do when running threaded contexts
   *
   * @return a new context object
   */
  FEX_DEFAULT_VISIBILITY FEXCore::Context::Context *CreateNewContext();

  /**
   * @brief Post creation context initialization
   * Once configurations have been set, do the post-creation initialization with that configuration
   *
   * @param CTX The context that we created
   *
   * @return true if we managed to initialize correctly
   */
  FEX_DEFAULT_VISIBILITY bool InitializeContext(FEXCore::Context::Context *CTX);

  /**
   * @brief Destroy the context object
   *
   * @param CTX
   */
  FEX_DEFAULT_VISIBILITY void DestroyContext(FEXCore::Context::Context *CTX);

  /**
   * @brief Allows setting up in memory code and other things prior to launchign code execution
   *
   * @param CTX The context that we created
   * @param Loader The loader that will be doing all the code loading
   *
   * @return true if we loaded code
   */
  FEX_DEFAULT_VISIBILITY FEXCore::Core::InternalThreadState* InitCore(FEXCore::Context::Context *CTX, FEXCore::CodeLoader *Loader);

  FEX_DEFAULT_VISIBILITY void SetExitHandler(FEXCore::Context::Context *CTX, ExitHandler handler);
  FEX_DEFAULT_VISIBILITY ExitHandler GetExitHandler(const FEXCore::Context::Context *CTX);

  /**
   * @brief Pauses execution on the CPU core
   *
   * Blocks until all threads have paused.
   */
  FEX_DEFAULT_VISIBILITY void Pause(FEXCore::Context::Context *CTX);

  /**
   * @brief Starts (or continues) the CPU core
   *
   * This function is async and returns immediately.
   * Use RunUntilExit() for synchonous executions
   *
   */
  FEX_DEFAULT_VISIBILITY void Run(FEXCore::Context::Context *CTX);

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
  FEX_DEFAULT_VISIBILITY ExitReason RunUntilExit(FEXCore::Context::Context *CTX);

  FEX_DEFAULT_VISIBILITY void CompileRIP(FEXCore::Core::InternalThreadState *Thread, uint64_t GuestRIP);

  /**
   * @brief Gets the program exit status
   *
   *
   * @param CTX The context that we created
   *
   * @return The program exit status
   */
  FEX_DEFAULT_VISIBILITY int GetProgramStatus(const FEXCore::Context::Context *CTX);

  /**
   * @brief Tells the core to shutdown
   *
   * Blocks until shutdown
   */
  FEX_DEFAULT_VISIBILITY void Stop(FEXCore::Context::Context *CTX);

  /**
   * @brief Executes one instruction
   *
   * Returns once execution is complete.
   */
  FEX_DEFAULT_VISIBILITY void Step(FEXCore::Context::Context *CTX);

  /**
   * @brief [[threadsafe]] Returns the ExitReason of the parent thread. Typically used for async result status
   *
   * @param CTX The context that we created
   *
   * @return The ExitReason for the parentthread
   */
  FEX_DEFAULT_VISIBILITY ExitReason GetExitReason(const FEXCore::Context::Context *CTX);

  /**
   * @brief [[theadsafe]] Checks if the Context is either done working or paused(in the case of single stepping)
   *
   * Use this when the context is async running to determine if it is done
   *
   * @param CTX the context that we created
   *
   * @return true if the core is done or paused
   */
  FEX_DEFAULT_VISIBILITY bool IsDone(const FEXCore::Context::Context *CTX);

  /**
   * @brief Gets a copy the CPUState of the parent thread
   *
   * @param CTX The context that we created
   * @param State The state object to populate
   */
  FEX_DEFAULT_VISIBILITY void GetCPUState(const FEXCore::Context::Context *CTX,
                                          FEXCore::Core::CPUState *State);

  /**
   * @brief Copies the CPUState provided to the parent thread
   *
   * @param CTX The context that we created
   * @param State The satate object to copy from
   */
  FEX_DEFAULT_VISIBILITY void SetCPUState(FEXCore::Context::Context *CTX,
                                          const FEXCore::Core::CPUState *State);

  /**
   * @brief Allows the frontend to pass in a custom CPUBackend creation factory
   *
   * This allows the frontend to have its own frontend. Typically for debugging
   *
   * @param CTX The context that we created
   * @param Factory The factory that the context will call if the DefaultCore config ise set to CUSTOM
   */
  FEX_DEFAULT_VISIBILITY void SetCustomCPUBackendFactory(FEXCore::Context::Context *CTX, CustomCPUFactoryType Factory);

  /**
   * @brief Sets up memory regions on the guest for mirroring within the guest's VM space
   *
   * @param VirtualAddress The address we want to set to mirror a physical memory region
   * @param PhysicalAddress The physical memory region we are mapping
   * @param Size Size of the region to mirror
   *
   * @return true when successfully mapped. false if there was an error adding
   */
  FEX_DEFAULT_VISIBILITY bool AddVirtualMemoryMapping(FEXCore::Context::Context *CTX, uint64_t VirtualAddress, uint64_t PhysicalAddress, uint64_t Size);

  /**
   * @brief Allows the frontend to set a custom syscall handler
   *
   * Useful for debugging purposes. May not work if the syscall ID exceeds the maximum number of syscalls in the lookup table
   *
   * @param Syscall Which syscall ID to install a visitor to
   * @param Visitor The Visitor to install
   */
  FEX_DEFAULT_VISIBILITY void RegisterExternalSyscallVisitor(FEXCore::Context::Context *CTX, uint64_t Syscall, FEXCore::HLE::SyscallVisitor *Visitor);

  FEX_DEFAULT_VISIBILITY void HandleCallback(FEXCore::Context::Context *CTX, FEXCore::Core::InternalThreadState *Thread, uint64_t RIP);

  FEX_DEFAULT_VISIBILITY void RegisterHostSignalHandler(FEXCore::Context::Context *CTX, int Signal, HostSignalDelegatorFunction Func, bool Required);
  FEX_DEFAULT_VISIBILITY void RegisterFrontendHostSignalHandler(FEXCore::Context::Context *CTX, int Signal, HostSignalDelegatorFunction Func, bool Required);

  FEX_DEFAULT_VISIBILITY FEXCore::Core::InternalThreadState* CreateThread(FEXCore::Context::Context *CTX, FEXCore::Core::CPUState *NewThreadState, uint64_t ParentTID);
  FEX_DEFAULT_VISIBILITY void ExecutionThread(FEXCore::Context::Context *CTX, FEXCore::Core::InternalThreadState *Thread);
  FEX_DEFAULT_VISIBILITY void InitializeThread(FEXCore::Context::Context *CTX, FEXCore::Core::InternalThreadState *Thread);
  FEX_DEFAULT_VISIBILITY void RunThread(FEXCore::Context::Context *CTX, FEXCore::Core::InternalThreadState *Thread);
  FEX_DEFAULT_VISIBILITY void StopThread(FEXCore::Context::Context *CTX, FEXCore::Core::InternalThreadState *Thread);
  FEX_DEFAULT_VISIBILITY void DestroyThread(FEXCore::Context::Context *CTX, FEXCore::Core::InternalThreadState *Thread);
  FEX_DEFAULT_VISIBILITY void CleanupAfterFork(FEXCore::Context::Context *CTX, FEXCore::Core::InternalThreadState *Thread);
  FEX_DEFAULT_VISIBILITY void SetSignalDelegator(FEXCore::Context::Context *CTX, FEXCore::SignalDelegator *SignalDelegation);
  FEX_DEFAULT_VISIBILITY void SetSyscallHandler(FEXCore::Context::Context *CTX, FEXCore::HLE::SyscallHandler *Handler);
  FEX_DEFAULT_VISIBILITY FEXCore::CPUID::FunctionResults RunCPUIDFunction(FEXCore::Context::Context *CTX, uint32_t Function, uint32_t Leaf);
  FEX_DEFAULT_VISIBILITY FEXCore::CPUID::FunctionResults RunCPUIDFunctionName(FEXCore::Context::Context *CTX, uint32_t Function, uint32_t Leaf, uint32_t CPU);

  FEX_DEFAULT_VISIBILITY FEXCore::IR::AOTIRCacheEntry *LoadAOTIRCacheEntry(FEXCore::Context::Context *CTX, const std::string& Name);
  FEX_DEFAULT_VISIBILITY void UnloadAOTIRCacheEntry(FEXCore::Context::Context *CTX, FEXCore::IR::AOTIRCacheEntry *Entry);

  FEX_DEFAULT_VISIBILITY void SetAOTIRLoader(FEXCore::Context::Context *CTX, std::function<int(const std::string&)> CacheReader);
  FEX_DEFAULT_VISIBILITY void SetAOTIRWriter(FEXCore::Context::Context *CTX, std::function<std::unique_ptr<std::ofstream>(const std::string&)> CacheWriter);
  FEX_DEFAULT_VISIBILITY void SetAOTIRRenamer(FEXCore::Context::Context *CTX, std::function<void(const std::string&)> CacheRenamer);

  FEX_DEFAULT_VISIBILITY void FinalizeAOTIRCache(FEXCore::Context::Context *CTX);
  FEX_DEFAULT_VISIBILITY void WriteFilesWithCode(FEXCore::Context::Context *CTX, std::function<void(const std::string& fileid, const std::string& filename)> Writer);
  FEX_DEFAULT_VISIBILITY void InvalidateGuestCodeRange(FEXCore::Context::Context *CTX, uint64_t Start, uint64_t Length);

  FEX_DEFAULT_VISIBILITY void ConfigureAOTGen(FEXCore::Core::InternalThreadState *Thread, std::set<uint64_t> *ExternalBranches, uint64_t SectionMaxAddress);
}
