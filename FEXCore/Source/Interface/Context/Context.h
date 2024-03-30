// SPDX-License-Identifier: MIT
#pragma once

#include "Common/JitSymbols.h"
#include "Interface/Core/CPUID.h"
#include "Interface/Core/X86HelperGen.h"
#include "Interface/Core/ObjectCache/ObjectCacheService.h"
#include "Interface/Core/Dispatcher/Dispatcher.h"
#include "Interface/IR/AOTIR.h"
#include <FEXCore/Config/Config.h>
#include <FEXCore/Core/Context.h>
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Core/HostFeatures.h>
#include <FEXCore/Core/SignalDelegator.h>
#include <FEXCore/Debug/InternalThreadState.h>
#include <FEXCore/Utils/CompilerDefs.h>
#include <FEXCore/Utils/Event.h>
#include <FEXCore/Utils/SignalScopeGuards.h>
#include <FEXCore/fextl/memory.h>
#include <FEXCore/fextl/set.h>
#include <FEXCore/fextl/string.h>
#include <FEXCore/fextl/unordered_map.h>
#include <FEXCore/fextl/vector.h>
#include <FEXHeaderUtils/Syscalls.h>
#include <stdint.h>

#include <atomic>
#include <condition_variable>
#include <functional>
#include <istream>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <stddef.h>
#include <queue>

namespace FEXCore {
class CodeLoader;
class ThunkHandler;

namespace CodeSerialize {
  class CodeObjectSerializeService;
}

namespace CPU {
  class Arm64JITCore;
  class Dispatcher;
}
namespace HLE {
struct SyscallArguments;
class SyscallHandler;
class SourcecodeResolver;
struct SourcecodeMap;
}
}

namespace FEXCore::IR {
  class RegisterAllocationData;
  class IRListView;
namespace Validation {
  class IRValidation;
}
}

namespace FEXCore::Context {
  enum CoreRunningMode {
    MODE_RUN        = 0,
    MODE_SINGLESTEP = 1,
  };

  struct ExitFunctionLinkData {
    uint64_t HostBranch;
    uint64_t GuestRIP;
  };

  using BlockDelinkerFunc = void(*)(FEXCore::Core::CpuStateFrame *Frame, FEXCore::Context::ExitFunctionLinkData *Record);
  constexpr uint32_t TSC_SCALE = 128;
  constexpr uint32_t TSC_SCALE_MAXIMUM = 1'000'000'000; ///< 1Ghz

  class ContextImpl final : public FEXCore::Context::Context {
    public:
      // Context base class implementation.
      bool InitCore() override;

      void SetExitHandler(ExitHandler handler) override;
      ExitHandler GetExitHandler() const override;

      ExitReason RunUntilExit(FEXCore::Core::InternalThreadState *Thread) override;

      void ExecuteThread(FEXCore::Core::InternalThreadState *Thread) override;

      void CompileRIP(FEXCore::Core::InternalThreadState *Thread, uint64_t GuestRIP) override;
      void CompileRIPCount(FEXCore::Core::InternalThreadState *Thread, uint64_t GuestRIP, uint64_t MaxInst) override;

      void SetCustomCPUBackendFactory(CustomCPUFactoryType Factory) override;

      HostFeatures GetHostFeatures() const override;

      void HandleCallback(FEXCore::Core::InternalThreadState *Thread, uint64_t RIP) override;

      uint64_t RestoreRIPFromHostPC(FEXCore::Core::InternalThreadState *Thread, uint64_t HostPC) override;
      uint32_t ReconstructCompactedEFLAGS(FEXCore::Core::InternalThreadState *Thread, bool WasInJIT, uint64_t *HostGPRs, uint64_t PSTATE) override;
      void SetFlagsFromCompactedEFLAGS(FEXCore::Core::InternalThreadState *Thread, uint32_t EFLAGS) override;

      /**
       * @brief Used to create FEX thread objects in preparation for creating a true OS thread. Does set a TID or PID.
       *
       * @param InitialRIP The starting RIP of this thread
       * @param StackPointer The starting RSP of this thread
       * @param NewThreadState The initial thread state to setup for our state, if inheriting.
       * @param ParentTID The PID that was the parent thread that created this
       *
       * @return The InternalThreadState object that tracks all of the emulated thread's state
       *
       * Usecases:
       *  Parent thread Creation:
       *    - Thread = CreateThread(InitialRIP, InitialStack, nullptr, 0);
       *    - CTX->RunUntilExit(Thread);
       *  OS thread Creation:
       *    - Thread = CreateThread(0, 0, NewState, PPID);
       *    - Thread->ExecutionThread = FEXCore::Threads::Thread::Create(ThreadHandler, Arg);
       *    - ThreadHandler calls `CTX->ExecutionThread(Thread)`
       *  OS fork (New thread created with a clone of thread state):
       *    - clone{2, 3}
       *    - Thread = CreateThread(0, 0, CopyOfThreadState, PPID);
       *    - ExecutionThread(Thread); // Starts executing without creating another host thread
       *  Thunk callback executing guest code from native host thread
       *    - Thread = CreateThread(0, 0, NewState, PPID);
       *    - InitializeThreadTLSData(Thread);
       *    - HandleCallback(Thread, RIP);
       */

      FEXCore::Core::InternalThreadState* CreateThread(uint64_t InitialRIP, uint64_t StackPointer, FEXCore::Core::CPUState *NewThreadState) override;

      // Public for threading
      void ExecutionThread(FEXCore::Core::InternalThreadState *Thread) override;

      /**
       * @brief Destroys this FEX thread object and stops tracking it internally
       *
       * @param Thread The internal FEX thread state object
       */
      void DestroyThread(FEXCore::Core::InternalThreadState *Thread, bool NeedsTLSUninstall) override;

#ifndef _WIN32
      void LockBeforeFork(FEXCore::Core::InternalThreadState *Thread) override;
      void UnlockAfterFork(FEXCore::Core::InternalThreadState *Thread, bool Child) override;
#endif
      void SetSignalDelegator(FEXCore::SignalDelegator *SignalDelegation) override;
      void SetSyscallHandler(FEXCore::HLE::SyscallHandler *Handler) override;

      FEXCore::CPUID::FunctionResults RunCPUIDFunction(uint32_t Function, uint32_t Leaf) override;
      FEXCore::CPUID::XCRResults RunXCRFunction(uint32_t Function) override;
      FEXCore::CPUID::FunctionResults RunCPUIDFunctionName(uint32_t Function, uint32_t Leaf, uint32_t CPU) override;

      FEXCore::IR::AOTIRCacheEntry *LoadAOTIRCacheEntry(const fextl::string& Name) override;
      void UnloadAOTIRCacheEntry(FEXCore::IR::AOTIRCacheEntry *Entry) override;

      void SetAOTIRLoader(AOTIRLoaderCBFn CacheReader) override {
        IRCaptureCache.SetAOTIRLoader(std::move(CacheReader));
      }
      void SetAOTIRWriter(AOTIRWriterCBFn CacheWriter) override {
        IRCaptureCache.SetAOTIRWriter(std::move(CacheWriter));
      }
      void SetAOTIRRenamer(AOTIRRenamerCBFn CacheRenamer) override {
        IRCaptureCache.SetAOTIRRenamer(std::move(CacheRenamer));
      }

      void FinalizeAOTIRCache() override {
        IRCaptureCache.FinalizeAOTIRCache();
      }
      void WriteFilesWithCode(AOTIRCodeFileWriterFn Writer) override {
        IRCaptureCache.WriteFilesWithCode(Writer);
      }

      void ClearCodeCache(FEXCore::Core::InternalThreadState *Thread) override;
      void InvalidateGuestCodeRange(FEXCore::Core::InternalThreadState *Thread, uint64_t Start, uint64_t Length) override;
      void InvalidateGuestCodeRange(FEXCore::Core::InternalThreadState *Thread, uint64_t Start, uint64_t Length, CodeRangeInvalidationFn callback) override;
      FEXCore::ForkableSharedMutex &GetCodeInvalidationMutex() override {
        return CodeInvalidationMutex;
      }

      void MarkMemoryShared(FEXCore::Core::InternalThreadState *Thread) override;

      void ConfigureAOTGen(FEXCore::Core::InternalThreadState *Thread, fextl::set<uint64_t> *ExternalBranches, uint64_t SectionMaxAddress) override;
      // returns false if a handler was already registered
      CustomIRResult AddCustomIREntrypoint(uintptr_t Entrypoint, CustomIREntrypointHandler Handler, void *Creator = nullptr, void *Data = nullptr);

      void AppendThunkDefinitions(fextl::vector<FEXCore::IR::ThunkDefinition> const& Definitions) override;

    public:
    friend class FEXCore::HLE::SyscallHandler;
  #ifdef JIT_ARM64
    friend class FEXCore::CPU::Arm64JITCore;
  #endif

    friend class FEXCore::IR::Validation::IRValidation;

    struct {
      CoreRunningMode RunningMode {CoreRunningMode::MODE_RUN};
      uint64_t VirtualMemSize{1ULL << 36};

      // this is for internal use
      bool ValidateIRarser { false };

      // Used if the JIT needs to have its interrupt fault code emitted.
      bool NeedsPendingInterruptFaultCheck { false };

      FEX_CONFIG_OPT(Multiblock, MULTIBLOCK);
      FEX_CONFIG_OPT(SingleStepConfig, SINGLESTEP);
      FEX_CONFIG_OPT(GdbServer, GDBSERVER);
      FEX_CONFIG_OPT(Is64BitMode, IS64BIT_MODE);
      FEX_CONFIG_OPT(TSOEnabled, TSOENABLED);
      FEX_CONFIG_OPT(TSOAutoMigration, TSOAUTOMIGRATION);
      FEX_CONFIG_OPT(ABILocalFlags, ABILOCALFLAGS);
      FEX_CONFIG_OPT(AOTIRCapture, AOTIRCAPTURE);
      FEX_CONFIG_OPT(AOTIRGenerate, AOTIRGENERATE);
      FEX_CONFIG_OPT(AOTIRLoad, AOTIRLOAD);
      FEX_CONFIG_OPT(SMCChecks, SMCCHECKS);
      FEX_CONFIG_OPT(Core, CORE);
      FEX_CONFIG_OPT(MaxInstPerBlock, MAXINST);
      FEX_CONFIG_OPT(RootFSPath, ROOTFS);
      FEX_CONFIG_OPT(ThunkHostLibsPath, THUNKHOSTLIBS);
      FEX_CONFIG_OPT(ThunkHostLibsPath32, THUNKHOSTLIBS32);
      FEX_CONFIG_OPT(ThunkConfigFile, THUNKCONFIG);
      FEX_CONFIG_OPT(GlobalJITNaming, GLOBALJITNAMING);
      FEX_CONFIG_OPT(LibraryJITNaming, LIBRARYJITNAMING);
      FEX_CONFIG_OPT(BlockJITNaming, BLOCKJITNAMING);
      FEX_CONFIG_OPT(GDBSymbols, GDBSYMBOLS);
      FEX_CONFIG_OPT(ParanoidTSO, PARANOIDTSO);
      FEX_CONFIG_OPT(CacheObjectCodeCompilation, CACHEOBJECTCODECOMPILATION);
      FEX_CONFIG_OPT(x87ReducedPrecision, X87REDUCEDPRECISION);
      FEX_CONFIG_OPT(DisableTelemetry, DISABLETELEMETRY);
      FEX_CONFIG_OPT(DisableVixlIndirectCalls, DISABLE_VIXL_INDIRECT_RUNTIME_CALLS);
      FEX_CONFIG_OPT(SmallTSCScale, SMALLTSCSCALE);
    } Config;


    std::atomic_bool CoreShuttingDown{false};

    FEXCore::ForkableSharedMutex CodeInvalidationMutex;

    FEXCore::HostFeatures HostFeatures;
    // CPUID depends on HostFeatures so needs to be initialized after that.
    FEXCore::CPUIDEmu CPUID;
    FEXCore::HLE::SyscallHandler *SyscallHandler{};
    FEXCore::HLE::SourcecodeResolver *SourcecodeResolver{};
    fextl::unique_ptr<FEXCore::ThunkHandler> ThunkHandler;
    fextl::unique_ptr<FEXCore::CPU::Dispatcher> Dispatcher;

    CustomCPUFactoryType CustomCPUFactory;
    FEXCore::Context::ExitHandler CustomExitHandler;

#ifdef BLOCKSTATS
    fextl::unique_ptr<FEXCore::BlockSamplingData> BlockData;
#endif

    SignalDelegator *SignalDelegation{};
    X86GeneratedCode X86CodeGen;

    ContextImpl();
    ~ContextImpl();

    static void ThreadRemoveCodeEntry(FEXCore::Core::InternalThreadState *Thread, uint64_t GuestRIP);
    static void ThreadAddBlockLink(FEXCore::Core::InternalThreadState *Thread, uint64_t GuestDestination, FEXCore::Context::ExitFunctionLinkData *HostLink, const BlockDelinkerFunc &delinker);

    template<auto Fn>
    static uint64_t ThreadExitFunctionLink(FEXCore::Core::CpuStateFrame *Frame, ExitFunctionLinkData *Record) {
      auto Thread = Frame->Thread;
      auto lk = GuardSignalDeferringSection<std::shared_lock>(static_cast<ContextImpl*>(Thread->CTX)->CodeInvalidationMutex, Thread);

      return Fn(Frame, Record);
    }

    // Wrapper which takes CpuStateFrame instead of InternalThreadState and unique_locks CodeInvalidationMutex
    // Must be called from owning thread
    static void ThreadRemoveCodeEntryFromJit(FEXCore::Core::CpuStateFrame *Frame, uint64_t GuestRIP) {
      auto Thread = Frame->Thread;

      auto lk = GuardSignalDeferringSection(static_cast<ContextImpl*>(Thread->CTX)->CodeInvalidationMutex, Thread);

      ThreadRemoveCodeEntry(Thread, GuestRIP);
    }

    void RemoveCustomIREntrypoint(uintptr_t Entrypoint);

    struct GenerateIRResult {
      FEXCore::IR::IRListView* IRList;
      FEXCore::IR::RegisterAllocationData::UniquePtr RAData;
      uint64_t TotalInstructions;
      uint64_t TotalInstructionsLength;
      uint64_t StartAddr;
      uint64_t Length;
    };
    [[nodiscard]] GenerateIRResult GenerateIR(FEXCore::Core::InternalThreadState *Thread, uint64_t GuestRIP, bool ExtendedDebugInfo, uint64_t MaxInst);

    struct CompileCodeResult {
      void* CompiledCode;
      FEXCore::IR::IRListView* IRData;
      FEXCore::Core::DebugData* DebugData;
      FEXCore::IR::RegisterAllocationData::UniquePtr RAData;
      bool GeneratedIR;
      uint64_t StartAddr;
      uint64_t Length;
    };
    [[nodiscard]] CompileCodeResult CompileCode(FEXCore::Core::InternalThreadState *Thread, uint64_t GuestRIP, uint64_t MaxInst = 0);
    uintptr_t CompileBlock(FEXCore::Core::CpuStateFrame *Frame, uint64_t GuestRIP, uint64_t MaxInst = 0);

    // Used for thread creation from syscalls
    /**
     * @brief Initializes TID, PID and TLS data for a thread
     *
     * @param Thread The internal FEX thread state object
     */
    void InitializeThreadTLSData(FEXCore::Core::InternalThreadState *Thread);

    void CopyMemoryMapping(FEXCore::Core::InternalThreadState *ParentThread, FEXCore::Core::InternalThreadState *ChildThread);

    uint8_t GetGPRSize() const { return Config.Is64BitMode ? 8 : 4; }

    FEXCore::JITSymbols Symbols;

    void GetVDSOSigReturn(VDSOSigReturn *VDSOPointers) override {
      if (VDSOPointers->VDSO_kernel_sigreturn == nullptr) {
        VDSOPointers->VDSO_kernel_sigreturn = reinterpret_cast<void*>(X86CodeGen.sigreturn_32);
      }

      if (VDSOPointers->VDSO_kernel_rt_sigreturn == nullptr) {
        VDSOPointers->VDSO_kernel_rt_sigreturn = reinterpret_cast<void*>(X86CodeGen.rt_sigreturn_32);
      }
    }

    FEXCore::Utils::PooledAllocatorVirtual OpDispatcherAllocator;
    FEXCore::Utils::PooledAllocatorVirtual FrontendAllocator;

    // If Atomic-based TSO emulation is enabled or not.
    bool IsAtomicTSOEnabled() const { return AtomicTSOEmulationEnabled; }

    void SetHardwareTSOSupport(bool HardwareTSOSupported) override {
      SupportsHardwareTSO = HardwareTSOSupported;
      UpdateAtomicTSOEmulationConfig();
    }

    // Returns if Software TSO emulation is required.
    // NOTE: This doesn't necessary return if Atomic-based TSO is currently enabled.
    // This will still return true if on a single thread and TSO is currently disabled.
    //
    // This is to ensure that if early initialization checks CPU features and TSO /could/ be enabled, that
    // we return consistent results.
    //
    // To check if Atomic TSO is currently enabled in the JIT, use `IsAtomicTSOEnabled` instead.
    bool SoftwareTSORequired() const {
      if (SupportsHardwareTSO) return false;

      return Config.TSOEnabled;
    }

    void EnableExitOnHLT() override { ExitOnHLT = true; }

    bool ExitOnHLTEnabled() const { return ExitOnHLT; }

    FEXCore::CPU::CPUBackendFeatures BackendFeatures;

  protected:
    void UpdateAtomicTSOEmulationConfig() {
      if (SupportsHardwareTSO) {
        // If the hardware supports TSO then we don't need to emulate it through atomics.
        AtomicTSOEmulationEnabled = false;
      }
      else {
        // Atomic TSO emulation only enabled if the config option is enabled.
        AtomicTSOEmulationEnabled = (IsMemoryShared || !Config.TSOAutoMigration) && Config.TSOEnabled;
      }
    }

  private:
    /**
     * @brief Initializes the JIT compilers for the thread
     *
     * @param State The internal FEX thread state object
     *
     * InitializeCompiler is called inside of CreateThread, so you likely don't need this
     */
    void InitializeCompiler(FEXCore::Core::InternalThreadState* Thread);

    void AddBlockMapping(FEXCore::Core::InternalThreadState *Thread, uint64_t Address, void *Ptr);

    IR::AOTIRCaptureCache IRCaptureCache;
    fextl::unique_ptr<FEXCore::CodeSerialize::CodeObjectSerializeService> CodeObjectCacheService;

    bool StartPaused = false;
    bool IsMemoryShared = false;
    bool SupportsHardwareTSO = false;
    bool AtomicTSOEmulationEnabled = true;
    bool ExitOnHLT = false;
    FEX_CONFIG_OPT(AppFilename, APP_FILENAME);

    std::shared_mutex CustomIRMutex;
    std::atomic<bool> HasCustomIRHandlers{};
    fextl::unordered_map<uint64_t, std::tuple<CustomIREntrypointHandler, void *, void *>> CustomIRHandlers;
  };
}
