#pragma once
#include <functional>
#include <stdint.h>
#include <string>

#include <FEXCore/Core/SignalDelegator.h>
#include <FEXCore/Core/CPUID.h>

#include <istream>
#include <ostream>
#include <memory>

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
  using CustomCPUFactoryType = std::function<FEXCore::CPU::CPUBackend* (FEXCore::Context::Context*, FEXCore::Core::InternalThreadState *Thread)>;

  /**
   * @brief This initializes internal FEXCore state that is shared between contexts and requires overhead to setup
   */
  __attribute__((visibility("default"))) void InitializeStaticTables(OperatingMode Mode = MODE_64BIT);

  /**
   * @brief [[threadsafe]] Create a new FEXCore context object
   *
   * This is necessary to do when running threaded contexts
   *
   * @return a new context object
   */
  __attribute__((visibility("default"))) FEXCore::Context::Context *CreateNewContext();

  /**
   * @brief Post creation context initialization
   * Once configurations have been set, do the post-creation initialization with that configuration
   *
   * @param CTX The context that we created
   *
   * @return true if we managed to initialize correctly
   */
  __attribute__((visibility("default"))) bool InitializeContext(FEXCore::Context::Context *CTX);

  /**
   * @brief Destroy the context object
   *
   * @param CTX
   */
  __attribute__((visibility("default"))) void DestroyContext(FEXCore::Context::Context *CTX);

  /**
   * @brief Allows setting up in memory code and other things prior to launchign code execution
   *
   * @param CTX The context that we created
   * @param Loader The loader that will be doing all the code loading
   *
   * @return true if we loaded code
   */
  __attribute__((visibility("default"))) bool InitCore(FEXCore::Context::Context *CTX, FEXCore::CodeLoader *Loader);

  __attribute__((visibility("default"))) void SetExitHandler(FEXCore::Context::Context *CTX, std::function<void(uint64_t ThreadId, FEXCore::Context::ExitReason)> handler);
  __attribute__((visibility("default"))) std::function<void(uint64_t ThreadId, FEXCore::Context::ExitReason)> GetExitHandler(FEXCore::Context::Context *CTX);

  /**
   * @brief Pauses execution on the CPU core
   *
   * Blocks until all threads have paused.
   */
  __attribute__((visibility("default"))) void Pause(FEXCore::Context::Context *CTX);

  /**
   * @brief Starts (or continues) the CPU core
   *
   * This function is async and returns immediately.
   * Use RunUntilExit() for synchonous executions
   *
   */
  __attribute__((visibility("default"))) void Run(FEXCore::Context::Context *CTX);

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
  __attribute__((visibility("default"))) ExitReason RunUntilExit(FEXCore::Context::Context *CTX);

  __attribute__((visibility("default"))) void CompileRIP(FEXCore::Context::Context *CTX, uint64_t GuestRIP);

  /**
   * @brief Gets the program exit status
   *
   *
   * @param CTX The context that we created
   *
   * @return The program exit status
   */
  __attribute__((visibility("default"))) int GetProgramStatus(FEXCore::Context::Context *CTX);

  /**
   * @brief Tells the core to shutdown
   *
   * Blocks until shutdown
   */
  __attribute__((visibility("default"))) void Stop(FEXCore::Context::Context *CTX);

  /**
   * @brief Executes one instruction
   *
   * Returns once execution is complete.
   */
  __attribute__((visibility("default"))) void Step(FEXCore::Context::Context *CTX);

  /**
   * @brief [[threadsafe]] Returns the ExitReason of the parent thread. Typically used for async result status
   *
   * @param CTX The context that we created
   *
   * @return The ExitReason for the parentthread
   */
  __attribute__((visibility("default"))) ExitReason GetExitReason(FEXCore::Context::Context *CTX);

  /**
   * @brief [[theadsafe]] Checks if the Context is either done working or paused(in the case of single stepping)
   *
   * Use this when the context is async running to determine if it is done
   *
   * @param CTX the context that we created
   *
   * @return true if the core is done or paused
   */
  __attribute__((visibility("default"))) bool IsDone(FEXCore::Context::Context *CTX);

  /**
   * @brief Gets a copy the CPUState of the parent thread
   *
   * @param CTX The context that we created
   * @param State The state object to populate
   */
  __attribute__((visibility("default"))) void GetCPUState(FEXCore::Context::Context *CTX, FEXCore::Core::CPUState *State);

  /**
   * @brief Copies the CPUState provided to the parent thread
   *
   * @param CTX The context that we created
   * @param State The satate object to copy from
   */
  __attribute__((visibility("default"))) void SetCPUState(FEXCore::Context::Context *CTX, FEXCore::Core::CPUState *State);

  /**
   * @brief Allows the frontend to pass in a custom CPUBackend creation factory
   *
   * This allows the frontend to have its own frontend. Typically for debugging
   *
   * @param CTX The context that we created
   * @param Factory The factory that the context will call if the DefaultCore config ise set to CUSTOM
   */
  __attribute__((visibility("default"))) void SetCustomCPUBackendFactory(FEXCore::Context::Context *CTX, CustomCPUFactoryType Factory);

  /**
   * @brief Sets up memory regions on the guest for mirroring within the guest's VM space
   *
   * @param VirtualAddress The address we want to set to mirror a physical memory region
   * @param PhysicalAddress The physical memory region we are mapping
   * @param Size Size of the region to mirror
   *
   * @return true when successfully mapped. false if there was an error adding
   */
  __attribute__((visibility("default"))) bool AddVirtualMemoryMapping(FEXCore::Context::Context *CTX, uint64_t VirtualAddress, uint64_t PhysicalAddress, uint64_t Size);

  /**
   * @brief Allows the frontend to set a custom syscall handler
   *
   * Useful for debugging purposes. May not work if the syscall ID exceeds the maximum number of syscalls in the lookup table
   *
   * @param Syscall Which syscall ID to install a visitor to
   * @param Visitor The Visitor to install
   */
  __attribute__((visibility("default"))) void RegisterExternalSyscallVisitor(FEXCore::Context::Context *CTX, uint64_t Syscall, FEXCore::HLE::SyscallVisitor *Visitor);

  __attribute__((visibility("default"))) void HandleCallback(FEXCore::Context::Context *CTX, uint64_t RIP);

  __attribute__((visibility("default"))) void RegisterHostSignalHandler(FEXCore::Context::Context *CTX, int Signal, HostSignalDelegatorFunction Func);
  __attribute__((visibility("default"))) void RegisterFrontendHostSignalHandler(FEXCore::Context::Context *CTX, int Signal, HostSignalDelegatorFunction Func);

  __attribute__((visibility("default"))) FEXCore::Core::InternalThreadState* CreateThread(FEXCore::Context::Context *CTX, FEXCore::Core::CPUState *NewThreadState, uint64_t ParentTID);
  __attribute__((visibility("default"))) void InitializeThread(FEXCore::Context::Context *CTX, FEXCore::Core::InternalThreadState *Thread);
  __attribute__((visibility("default"))) void RunThread(FEXCore::Context::Context *CTX, FEXCore::Core::InternalThreadState *Thread);
  __attribute__((visibility("default"))) void StopThread(FEXCore::Context::Context *CTX, FEXCore::Core::InternalThreadState *Thread);
  __attribute__((visibility("default"))) void DestroyThread(FEXCore::Context::Context *CTX, FEXCore::Core::InternalThreadState *Thread);
  __attribute__((visibility("default"))) void CleanupAfterFork(FEXCore::Context::Context *CTX, FEXCore::Core::InternalThreadState *Thread);
  __attribute__((visibility("default"))) void SetSignalDelegator(FEXCore::Context::Context *CTX, FEXCore::SignalDelegator *SignalDelegation);
  __attribute__((visibility("default"))) void SetSyscallHandler(FEXCore::Context::Context *CTX, FEXCore::HLE::SyscallHandler *Handler);
  __attribute__((visibility("default"))) FEXCore::CPUID::FunctionResults RunCPUIDFunction(FEXCore::Context::Context *CTX, uint32_t Function, uint32_t Leaf);

  __attribute__((visibility("default"))) void AddNamedRegion(FEXCore::Context::Context *CTX, uintptr_t Base, uintptr_t Length, uintptr_t Offset, const std::string& Name);
  __attribute__((visibility("default"))) void RemoveNamedRegion(FEXCore::Context::Context *CTX, uintptr_t Base, uintptr_t Length);
  __attribute__((visibility("default"))) void SetAOTIRLoader(FEXCore::Context::Context *CTX, std::function<int(const std::string&)> CacheReader);
  __attribute__((visibility("default"))) bool WriteAOTIR(FEXCore::Context::Context *CTX, std::function<std::unique_ptr<std::ostream>(const std::string&)> CacheWriter);
  __attribute__((visibility("default"))) void FlushCodeRange(FEXCore::Core::InternalThreadState *Thread, uint64_t Start, uint64_t Length);
}
