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
  struct ThreadState;
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
  using CustomCPUFactoryType = std::function<FEXCore::CPU::CPUBackend* (FEXCore::Context::Context*, FEXCore::Core::ThreadState *Thread)>;

  /**
   * @brief This initializes internal FEXCore state that is shared between contexts and requires overhead to setup
   */
  void InitializeStaticTables(OperatingMode Mode = MODE_64BIT);

  /**
   * @brief [[threadsafe]] Create a new FEXCore context object
   *
   * This is necessary to do when running threaded contexts
   *
   * @return a new context object
   */
  FEXCore::Context::Context *CreateNewContext();

  /**
   * @brief Post creation context initialization
   * Once configurations have been set, do the post-creation initialization with that configuration
   *
   * @param CTX The context that we created
   *
   * @return true if we managed to initialize correctly
   */
  bool InitializeContext(FEXCore::Context::Context *CTX);

  /**
   * @brief Destroy the context object
   *
   * @param CTX
   */
  void DestroyContext(FEXCore::Context::Context *CTX);

  /**
   * @brief Allows setting up in memory code and other things prior to launchign code execution
   *
   * @param CTX The context that we created
   * @param Loader The loader that will be doing all the code loading
   *
   * @return true if we loaded code
   */
  bool InitCore(FEXCore::Context::Context *CTX, FEXCore::CodeLoader *Loader);

  void SetExitHandler(FEXCore::Context::Context *CTX, std::function<void(uint64_t ThreadId, FEXCore::Context::ExitReason)> handler);
  std::function<void(uint64_t ThreadId, FEXCore::Context::ExitReason)> GetExitHandler(FEXCore::Context::Context *CTX);

  /**
   * @brief Pauses execution on the CPU core
   *
   * Blocks until all threads have paused.
   */
  void Pause(FEXCore::Context::Context *CTX);

  /**
   * @brief Starts (or continues) the CPU core
   *
   * This function is async and returns immediately.
   * Use RunUntilExit() for synchonous executions
   *
   */
  void Run(FEXCore::Context::Context *CTX);

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
  ExitReason RunUntilExit(FEXCore::Context::Context *CTX);

  /**
   * @brief Gets the program exit status
   *
   *
   * @param CTX The context that we created
   *
   * @return The program exit status
   */
  int GetProgramStatus(FEXCore::Context::Context *CTX);

  /**
   * @brief Tells the core to shutdown
   *
   * Blocks until shutdown
   */
  void Stop(FEXCore::Context::Context *CTX);

  /**
   * @brief Executes one instruction
   *
   * Returns once execution is complete.
   */
  void Step(FEXCore::Context::Context *CTX);

  /**
   * @brief [[threadsafe]] Returns the ExitReason of the parent thread. Typically used for async result status
   *
   * @param CTX The context that we created
   *
   * @return The ExitReason for the parentthread
   */
  ExitReason GetExitReason(FEXCore::Context::Context *CTX);

  /**
   * @brief [[theadsafe]] Checks if the Context is either done working or paused(in the case of single stepping)
   *
   * Use this when the context is async running to determine if it is done
   *
   * @param CTX the context that we created
   *
   * @return true if the core is done or paused
   */
  bool IsDone(FEXCore::Context::Context *CTX);

  /**
   * @brief Gets a copy the CPUState of the parent thread
   *
   * @param CTX The context that we created
   * @param State The state object to populate
   */
  void GetCPUState(FEXCore::Context::Context *CTX, FEXCore::Core::CPUState *State);

  /**
   * @brief Copies the CPUState provided to the parent thread
   *
   * @param CTX The context that we created
   * @param State The satate object to copy from
   */
  void SetCPUState(FEXCore::Context::Context *CTX, FEXCore::Core::CPUState *State);

  /**
   * @brief Allows the frontend to pass in a custom CPUBackend creation factory
   *
   * This allows the frontend to have its own frontend. Typically for debugging
   *
   * @param CTX The context that we created
   * @param Factory The factory that the context will call if the DefaultCore config ise set to CUSTOM
   */
  void SetCustomCPUBackendFactory(FEXCore::Context::Context *CTX, CustomCPUFactoryType Factory);

  /**
   * @brief Allows a custom CPUBackend creation factory for fallback routines when the main CPUBackend core can't handle an instruction
   *
   * This is only useful for debugging new instruction decodings that FEXCore doesn't understand
   * The CPUBackend that is created from this factory must have its NeedsOpDispatch function to return false
   *
   * @param CTX The context that we created
   * @param Factory The factory that the context will call on core creation
   */
  void SetFallbackCPUBackendFactory(FEXCore::Context::Context *CTX, CustomCPUFactoryType Factory);

  /**
   * @brief Sets up memory regions on the guest for mirroring within the guest's VM space
   *
   * @param VirtualAddress The address we want to set to mirror a physical memory region
   * @param PhysicalAddress The physical memory region we are mapping
   * @param Size Size of the region to mirror
   *
   * @return true when successfully mapped. false if there was an error adding
   */
  bool AddVirtualMemoryMapping(FEXCore::Context::Context *CTX, uint64_t VirtualAddress, uint64_t PhysicalAddress, uint64_t Size);

  /**
   * @brief Allows the frontend to set a custom syscall handler
   *
   * Useful for debugging purposes. May not work if the syscall ID exceeds the maximum number of syscalls in the lookup table
   *
   * @param Syscall Which syscall ID to install a visitor to
   * @param Visitor The Visitor to install
   */
  void RegisterExternalSyscallVisitor(FEXCore::Context::Context *CTX, uint64_t Syscall, FEXCore::HLE::SyscallVisitor *Visitor);

  void HandleCallback(FEXCore::Context::Context *CTX, uint64_t RIP);

  void RegisterHostSignalHandler(FEXCore::Context::Context *CTX, int Signal, HostSignalDelegatorFunction Func);
  void RegisterFrontendHostSignalHandler(FEXCore::Context::Context *CTX, int Signal, HostSignalDelegatorFunction Func);

  FEXCore::Core::InternalThreadState* CreateThread(FEXCore::Context::Context *CTX, FEXCore::Core::CPUState *NewThreadState, uint64_t ParentTID);
  void InitializeThread(FEXCore::Context::Context *CTX, FEXCore::Core::InternalThreadState *Thread);
  void RunThread(FEXCore::Context::Context *CTX, FEXCore::Core::InternalThreadState *Thread);
  void StopThread(FEXCore::Context::Context *CTX, FEXCore::Core::InternalThreadState *Thread);
  void DeleteForkedThreads(FEXCore::Context::Context *CTX, FEXCore::Core::InternalThreadState *Thread);
  void SetSignalDelegator(FEXCore::Context::Context *CTX, FEXCore::SignalDelegator *SignalDelegation);
  void SetSyscallHandler(FEXCore::Context::Context *CTX, FEXCore::HLE::SyscallHandler *Handler);
  FEXCore::CPUID::FunctionResults RunCPUIDFunction(FEXCore::Context::Context *CTX, uint32_t Function, uint32_t Leaf);

  bool ReadAOTIR(FEXCore::Context::Context *CTX, std::istream& stream);
  bool WriteAOTIR(FEXCore::Context::Context *CTX, std::function<std::unique_ptr<std::ostream>(const std::string&)> CacheWriter);
}
