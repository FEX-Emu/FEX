// SPDX-License-Identifier: MIT
#pragma once
#include <functional>
#include <stdint.h>

#include <FEXCore/Core/SignalDelegator.h>
#include <FEXCore/Core/CPUID.h>
#include <FEXCore/IR/IR.h>
#include <FEXCore/Utils/CompilerDefs.h>
#include <FEXCore/Utils/IntervalList.h>
#include <FEXCore/fextl/memory.h>
#include <FEXCore/fextl/set.h>
#include <FEXCore/fextl/string.h>
#include <FEXCore/fextl/vector.h>

#include <istream>
#include <ostream>
#include <span>

namespace FEXCore {
class CodeLoader;
struct HostFeatures;
class ForkableSharedMutex;
class ThunkHandler;
} // namespace FEXCore

namespace FEXCore::Core {
struct CPUState;
struct InternalThreadState;
} // namespace FEXCore::Core

namespace FEXCore::CPU {
class CPUBackend;
}

namespace FEXCore::HLE {
struct SyscallArguments;
class SyscallHandler;
} // namespace FEXCore::HLE

namespace FEXCore::IR {
struct AOTIRCacheEntry;
class IREmitter;
} // namespace FEXCore::IR

namespace FEXCore::Context {
class Context;

enum OperatingMode {
  MODE_32BIT,
  MODE_64BIT,
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

using CodeRangeInvalidationFn = std::function<void(uint64_t start, uint64_t Length)>;

// Nested vector of guest block entrypoints
using InvalidatedEntryAccumulator = fextl::vector<fextl::vector<uint64_t>>;

using CustomIREntrypointHandler = std::function<void(uintptr_t Entrypoint, IR::IREmitter*)>;

using ExitHandler = std::function<void(Core::InternalThreadState* Thread)>;

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
  FEX_DEFAULT_VISIBILITY static fextl::unique_ptr<FEXCore::Context::Context> CreateNewContext(const FEXCore::HostFeatures& Features);

  /**
   * @brief Allows setting up in memory code and other things prior to launchign code execution
   *
   * @param CTX The context that we created
   * @param Loader The loader that will be doing all the code loading
   *
   * @return true if we loaded code
   */
  FEX_DEFAULT_VISIBILITY virtual bool InitCore() = 0;

  /**
   * @brief Executes the supplied thread context on the current thread until a return is requested
   */
  FEX_DEFAULT_VISIBILITY virtual void ExecuteThread(FEXCore::Core::InternalThreadState* Thread) = 0;

  FEX_DEFAULT_VISIBILITY virtual void CompileRIP(FEXCore::Core::InternalThreadState* Thread, uint64_t GuestRIP) = 0;
  FEX_DEFAULT_VISIBILITY virtual void CompileRIPCount(FEXCore::Core::InternalThreadState* Thread, uint64_t GuestRIP, uint64_t MaxInst) = 0;

  FEX_DEFAULT_VISIBILITY virtual void HandleCallback(FEXCore::Core::InternalThreadState* Thread, uint64_t RIP) = 0;

  FEX_DEFAULT_VISIBILITY virtual bool IsAddressInCurrentBlock(FEXCore::Core::InternalThreadState* Thread, uint64_t Address, uint64_t Size) = 0;
  FEX_DEFAULT_VISIBILITY virtual bool IsCurrentBlockSingleInst(FEXCore::Core::InternalThreadState* Thread) = 0;
  FEX_DEFAULT_VISIBILITY virtual uint64_t GetGuestBlockEntry(FEXCore::Core::InternalThreadState* Thread) = 0;

  ///< State reconstruction helpers
  ///< Reconstructs the guest RIP from the passed in thread context and related Host PC.
  FEX_DEFAULT_VISIBILITY virtual uint64_t RestoreRIPFromHostPC(FEXCore::Core::InternalThreadState* Thread, uint64_t HostPC) = 0;
  /**
   * @brief Reconstructs a compacted EFLAGS from FEX's internal EFLAG representation.
   *
   * @param Thread The thread getting the state reconstructed
   * @param WasInJIT If the code was in the JIT at the time.
   * @param HostGPRs The host Arm64 GPRs at the point of state inside the JIT.
   * @param PSTATE The Arm64 PState value.
   *
   * If WasInJIT is false then HostGPRs and PSTATE is ignored, with the assumption that the FEX JIT has already stored all state in to the
   * ThreadState object.
   *
   * @return x86 EFLAGS reconstructed
   */
  FEX_DEFAULT_VISIBILITY virtual uint32_t
  ReconstructCompactedEFLAGS(FEXCore::Core::InternalThreadState* Thread, bool WasInJIT, const uint64_t* HostGPRs, uint64_t PSTATE) = 0;
  ///< Sets FEX's internal EFLAGS representation to the passed in compacted form.
  FEX_DEFAULT_VISIBILITY virtual void SetFlagsFromCompactedEFLAGS(FEXCore::Core::InternalThreadState* Thread, uint32_t EFLAGS) = 0;

  FEX_DEFAULT_VISIBILITY virtual void
  ReconstructXMMRegisters(const FEXCore::Core::InternalThreadState* Thread, __uint128_t* XMM_Low, __uint128_t* YMM_High) = 0;
  FEX_DEFAULT_VISIBILITY virtual void
  SetXMMRegistersFromState(FEXCore::Core::InternalThreadState* Thread, const __uint128_t* XMM_Low, const __uint128_t* YMM_High) = 0;

  /**
   * @brief Create a new thread object that doesn't inherit any state.
   * Used to create FEX thread objects in preparation for creating a true OS thread.
   *
   * @param InitialRIP The starting RIP of this thread
   * @param StackPointer The starting RSP of this thread
   * @param NewThreadState The thread state to inherit from if not nullptr.
   *
   * @return A new InternalThreadState object for using with a new guest thread.
   */

  FEX_DEFAULT_VISIBILITY virtual FEXCore::Core::InternalThreadState*
  CreateThread(uint64_t InitialRIP, uint64_t StackPointer, const FEXCore::Core::CPUState* NewThreadState = nullptr) = 0;

  FEX_DEFAULT_VISIBILITY virtual void DestroyThread(FEXCore::Core::InternalThreadState* Thread) = 0;
#ifndef _WIN32
  FEX_DEFAULT_VISIBILITY virtual void LockBeforeFork(FEXCore::Core::InternalThreadState* Thread) {}
  FEX_DEFAULT_VISIBILITY virtual void UnlockAfterFork(FEXCore::Core::InternalThreadState* Thread, bool Child) {}
#endif
  FEX_DEFAULT_VISIBILITY virtual void SetSignalDelegator(FEXCore::SignalDelegator* SignalDelegation) = 0;
  FEX_DEFAULT_VISIBILITY virtual void SetSyscallHandler(FEXCore::HLE::SyscallHandler* Handler) = 0;
  FEX_DEFAULT_VISIBILITY virtual void SetThunkHandler(FEXCore::ThunkHandler* Handler) = 0;

  FEX_DEFAULT_VISIBILITY virtual FEXCore::CPUID::FunctionResults RunCPUIDFunction(uint32_t Function, uint32_t Leaf) = 0;
  FEX_DEFAULT_VISIBILITY virtual FEXCore::CPUID::XCRResults RunXCRFunction(uint32_t Function) = 0;
  FEX_DEFAULT_VISIBILITY virtual FEXCore::CPUID::FunctionResults RunCPUIDFunctionName(uint32_t Function, uint32_t Leaf, uint32_t CPU) = 0;

  FEX_DEFAULT_VISIBILITY virtual FEXCore::IR::AOTIRCacheEntry* LoadAOTIRCacheEntry(const fextl::string& Name) = 0;
  FEX_DEFAULT_VISIBILITY virtual void UnloadAOTIRCacheEntry(FEXCore::IR::AOTIRCacheEntry* Entry) = 0;

  FEX_DEFAULT_VISIBILITY virtual void SetAOTIRLoader(AOTIRLoaderCBFn CacheReader) = 0;
  FEX_DEFAULT_VISIBILITY virtual void SetAOTIRWriter(AOTIRWriterCBFn CacheWriter) = 0;
  FEX_DEFAULT_VISIBILITY virtual void SetAOTIRRenamer(AOTIRRenamerCBFn CacheRenamer) = 0;

  FEX_DEFAULT_VISIBILITY virtual void FinalizeAOTIRCache() = 0;
  FEX_DEFAULT_VISIBILITY virtual void WriteFilesWithCode(AOTIRCodeFileWriterFn Writer) = 0;

  FEX_DEFAULT_VISIBILITY virtual void ClearCodeCache(FEXCore::Core::InternalThreadState* Thread, bool NewCodeBuffer = true) = 0;
  FEX_DEFAULT_VISIBILITY virtual void InvalidateGuestCodeRange(
    FEXCore::Core::InternalThreadState* Thread, InvalidatedEntryAccumulator& Accumulator, uint64_t Start, uint64_t Length) = 0;
  FEX_DEFAULT_VISIBILITY virtual FEXCore::ForkableSharedMutex& GetCodeInvalidationMutex() = 0;

  FEX_DEFAULT_VISIBILITY virtual void MarkMemoryShared(FEXCore::Core::InternalThreadState* Thread) = 0;

  FEX_DEFAULT_VISIBILITY virtual void
  ConfigureAOTGen(FEXCore::Core::InternalThreadState* Thread, fextl::set<uint64_t>* ExternalBranches, uint64_t SectionMaxAddress) = 0;

  /**
   * @brief Checks if a PC is inside any code buffer used by the thread's JIT.
   *
   * @param Thread Which thread's code buffers to check inside of.
   * @param Address The PC to check against.
   *
   * @return true if PC is inside the thread's code buffers.
   */
  FEX_DEFAULT_VISIBILITY virtual bool IsAddressInCodeBuffer(FEXCore::Core::InternalThreadState* Thread, uintptr_t Address) const = 0;

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
   * @brief Adds a new Thunk trampoline handler
   *
   * @param Entrypoint The guest PC that the custom thunk trampoline IR handler will be installed at.
   * @param GuestThunkEntrypoint The thunk entrypoint that the IR handler will redirect to.
   */
  FEX_DEFAULT_VISIBILITY virtual void AddThunkTrampolineIRHandler(uintptr_t Entrypoint, uintptr_t GuestThunkEntrypoint) = 0;

  /**
   * @brief Adds additional per-instruction granularity TSO enable/disable information for the given range.
   *
   * @param ValidRanges The set of address ranges covered by this information
   * @param Instructions The set of instruction addresses within the given ranges for which TSO should be enabled
   */
  FEX_DEFAULT_VISIBILITY virtual void AddForceTSOInformation(const IntervalList<uint64_t>& ValidRanges, fextl::set<uint64_t>&& Instructions) = 0;

  FEX_DEFAULT_VISIBILITY virtual void RemoveForceTSOInformation(uint64_t Address, uint64_t Size) = 0;

  FEX_DEFAULT_VISIBILITY virtual void MarkMonoDetected() = 0;

  FEX_DEFAULT_VISIBILITY virtual void MarkMonoBackpatcherBlock(uint64_t BlockEntry) = 0;
private:
};

/**
 * @brief This initializes internal FEXCore state that is shared between contexts and requires overhead to setup
 */
FEX_DEFAULT_VISIBILITY void InitializeStaticTables(OperatingMode Mode = MODE_64BIT);
} // namespace FEXCore::Context
