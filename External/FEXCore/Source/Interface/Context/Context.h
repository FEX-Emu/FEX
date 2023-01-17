#pragma once

#include "Common/JitSymbols.h"
#include "FEXHeaderUtils/ScopedSignalMask.h"
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
#include <FEXHeaderUtils/Syscalls.h>
#include <stdint.h>


#include <atomic>
#include <condition_variable>
#include <functional>
#include <istream>
#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <stddef.h>
#include <string>
#include <unordered_map>
#include <queue>
#include <vector>

namespace FEXCore {
class CodeLoader;
class ThunkHandler;
class GdbServer;

namespace CodeSerialize {
  class CodeObjectSerializeService;
}

namespace CPU {
  class Arm64JITCore;
  class X86JITCore;
  class InterpreterCore;
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

  struct Context {
    friend class FEXCore::HLE::SyscallHandler;
  #ifdef JIT_ARM64
    friend class FEXCore::CPU::Arm64JITCore;
  #endif
  #ifdef JIT_X86_64
    friend class FEXCore::CPU::X86JITCore;
  #endif

    friend class FEXCore::CPU::InterpreterCore;
    friend class FEXCore::IR::Validation::IRValidation;

    struct {
      CoreRunningMode RunningMode {CoreRunningMode::MODE_RUN};
      uint64_t VirtualMemSize{1ULL << 36};

      // this is for internal use
      bool ValidateIRarser { false };

      FEX_CONFIG_OPT(Multiblock, MULTIBLOCK);
      FEX_CONFIG_OPT(SingleStepConfig, SINGLESTEP);
      FEX_CONFIG_OPT(GdbServer, GDBSERVER);
      FEX_CONFIG_OPT(Is64BitMode, IS64BIT_MODE);
      FEX_CONFIG_OPT(TSOEnabled, TSOENABLED);
      FEX_CONFIG_OPT(TSOAutoMigration, TSOAUTOMIGRATION);
      FEX_CONFIG_OPT(ABILocalFlags, ABILOCALFLAGS);
      FEX_CONFIG_OPT(ABINoPF, ABINOPF);
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
      FEX_CONFIG_OPT(DumpIR, DUMPIR);
      FEX_CONFIG_OPT(StaticRegisterAllocation, SRA);
      FEX_CONFIG_OPT(GlobalJITNaming, GLOBALJITNAMING);
      FEX_CONFIG_OPT(LibraryJITNaming, LIBRARYJITNAMING);
      FEX_CONFIG_OPT(BlockJITNaming, BLOCKJITNAMING);
      FEX_CONFIG_OPT(GDBSymbols, GDBSYMBOLS);
      FEX_CONFIG_OPT(ParanoidTSO, PARANOIDTSO);
      FEX_CONFIG_OPT(CacheObjectCodeCompilation, CACHEOBJECTCODECOMPILATION);
      FEX_CONFIG_OPT(x87ReducedPrecision, X87REDUCEDPRECISION);
      FEX_CONFIG_OPT(x86dec_SynchronizeRIPOnAllBlocks, X86DEC_SYNCHRONIZERIPONALLBLOCKS);
      FEX_CONFIG_OPT(EnableAVX, ENABLEAVX);
    } Config;

    FEXCore::HostFeatures HostFeatures;

    std::mutex ThreadCreationMutex;
    FEXCore::Core::InternalThreadState* ParentThread{};
    std::vector<FEXCore::Core::InternalThreadState*> Threads;
    std::atomic_bool CoreShuttingDown{false};
    bool NeedToCheckXID{true};

    std::mutex IdleWaitMutex;
    std::condition_variable IdleWaitCV;
    std::atomic<uint32_t> IdleWaitRefCount{};

    Event PauseWait;
    bool Running{};

    std::shared_mutex CodeInvalidationMutex;

    FEXCore::CPUIDEmu CPUID;
    FEXCore::HLE::SyscallHandler *SyscallHandler{};
    FEXCore::HLE::SourcecodeResolver *SourcecodeResolver{};
    std::unique_ptr<FEXCore::ThunkHandler> ThunkHandler;
    std::unique_ptr<FEXCore::CPU::Dispatcher> Dispatcher;

    CustomCPUFactoryType CustomCPUFactory;
    FEXCore::Context::ExitHandler CustomExitHandler;

#ifdef BLOCKSTATS
    std::unique_ptr<FEXCore::BlockSamplingData> BlockData;
#endif

    SignalDelegator *SignalDelegation{};
    X86GeneratedCode X86CodeGen;

    Context();
    ~Context();

    FEXCore::Core::InternalThreadState* InitCore(uint64_t InitialRIP, uint64_t StackPointer);
    FEXCore::Context::ExitReason RunUntilExit();
    int GetProgramStatus() const;
    bool IsPaused() const { return !Running; }
    void Pause();
    void Run();
    void WaitForThreadsToRun();
    void Step();
    void Stop(bool IgnoreCurrentThread);
    void WaitForIdle();
    void StopThread(FEXCore::Core::InternalThreadState *Thread);
    void SignalThread(FEXCore::Core::InternalThreadState *Thread, FEXCore::Core::SignalEvent Event);

    bool GetGdbServerStatus() const { return DebugServer != nullptr; }
    void StartGdbServer();
    void StopGdbServer();
    void HandleCallback(FEXCore::Core::InternalThreadState *Thread, uint64_t RIP);
    [[noreturn]] void HandleSignalHandlerReturn(bool RT);
    void RegisterHostSignalHandler(int Signal, HostSignalDelegatorFunction Func, bool Required);
    void RegisterFrontendHostSignalHandler(int Signal, HostSignalDelegatorFunction Func, bool Required);

    static void ThreadRemoveCodeEntry(FEXCore::Core::InternalThreadState *Thread, uint64_t GuestRIP);
    static void ThreadAddBlockLink(FEXCore::Core::InternalThreadState *Thread, uint64_t GuestDestination, uintptr_t HostLink, const std::function<void()> &delinker);

    template<auto Fn>
    static uint64_t ThreadExitFunctionLink(FEXCore::Core::CpuStateFrame *Frame, uint64_t *record) {
      FHU::ScopedSignalMaskWithSharedLock lk(Frame->Thread->CTX->CodeInvalidationMutex);

      return Fn(Frame, record);
    }

    // Wrapper which takes CpuStateFrame instead of InternalThreadState and unique_locks CodeInvalidationMutex
    // Must be called from owning thread
    static void ThreadRemoveCodeEntryFromJit(FEXCore::Core::CpuStateFrame *Frame, uint64_t GuestRIP) {
      auto Thread = Frame->Thread;
      
      LogMan::Throw::AFmt(Thread->ThreadManager.GetTID() == FHU::Syscalls::gettid(), "Must be called from owning thread {}, not {}", Thread->ThreadManager.GetTID(), FHU::Syscalls::gettid());

      FHU::ScopedSignalMaskWithUniqueLock lk(Thread->CTX->CodeInvalidationMutex);

      ThreadRemoveCodeEntry(Thread, GuestRIP);
    }

    // returns false if a handler was already registered
    CustomIRResult AddCustomIREntrypoint(uintptr_t Entrypoint, std::function<void(uintptr_t Entrypoint, FEXCore::IR::IREmitter *)> Handler, void *Creator, void *Data);

    void RemoveCustomIREntrypoint(uintptr_t Entrypoint);

    // Debugger interface
    void CompileRIP(FEXCore::Core::InternalThreadState *Thread, uint64_t RIP);
    uint64_t GetThreadCount() const;
    FEXCore::Core::RuntimeStats *GetRuntimeStatsForThread(uint64_t Thread);
    bool GetDebugDataForRIP(uint64_t RIP, FEXCore::Core::DebugData *Data);
    bool FindHostCodeForRIP(uint64_t RIP, uint8_t **Code);

    struct GenerateIRResult {
      FEXCore::IR::IRListView* IRList;
      FEXCore::IR::RegisterAllocationData::UniquePtr RAData;
      uint64_t TotalInstructions;
      uint64_t TotalInstructionsLength;
      uint64_t StartAddr;
      uint64_t Length;
    };
    [[nodiscard]] GenerateIRResult GenerateIR(FEXCore::Core::InternalThreadState *Thread, uint64_t GuestRIP, bool ExtendedDebugInfo);

    struct CompileCodeResult {
      void* CompiledCode;
      FEXCore::IR::IRListView* IRData;
      FEXCore::Core::DebugData* DebugData;
      FEXCore::IR::RegisterAllocationData::UniquePtr RAData;
      bool GeneratedIR;
      uint64_t StartAddr;
      uint64_t Length;
    };
    [[nodiscard]] CompileCodeResult CompileCode(FEXCore::Core::InternalThreadState *Thread, uint64_t GuestRIP);
    uintptr_t CompileBlock(FEXCore::Core::CpuStateFrame *Frame, uint64_t GuestRIP);

    // same as CompileBlock, but aborts on failure
    void CompileBlockJit(FEXCore::Core::CpuStateFrame *Frame, uint64_t GuestRIP);

    // Used for thread creation from syscalls
    /**
     * @brief Used to create FEX thread objects in preparation for creating a true OS thread. Does set a TID or PID.
     *
     * @param NewThreadState The initial thread state to setup for our state
     * @param ParentTID The PID that was the parent thread that created this
     *
     * @return The InternalThreadState object that tracks all of the emulated thread's state
     *
     * Usecases:
     *  OS thread Creation:
     *    - Thread = CreateThread(NewState, PPID);
     *    - InitializeThread(Thread);
     *  OS fork (New thread created with a clone of thread state):
     *    - clone{2, 3}
     *    - Thread = CreateThread(CopyOfThreadState, PPID);
     *    - ExecutionThread(Thread); // Starts executing without creating another host thread
     *  Thunk callback executing guest code from native host thread
     *    - Thread = CreateThread(NewState, PPID);
     *    - InitializeThreadTLSData(Thread);
     *    - HandleCallback(Thread, RIP);
     */
    FEXCore::Core::InternalThreadState* CreateThread(FEXCore::Core::CPUState *NewThreadState, uint64_t ParentTID);

    /**
     * @brief Initializes TID, PID and TLS data for a thread
     *
     * @param Thread The internal FEX thread state object
     */
    void InitializeThreadTLSData(FEXCore::Core::InternalThreadState *Thread);

    /**
     * @brief Initializes the OS thread object and prepares to start executing on that new OS thread
     *
     * @param Thread The internal FEX thread state object
     *
     * The OS thread will wait until RunThread is executed
     */
    void InitializeThread(FEXCore::Core::InternalThreadState *Thread);

    /**
     * @brief Starts the OS thread object to start executing guest code
     *
     * @param Thread The internal FEX thread state object
     */
    void RunThread(FEXCore::Core::InternalThreadState *Thread);

    /**
     * @brief Destroys this FEX thread object and stops tracking it internally
     *
     * @param Thread The internal FEX thread state object
     */
    void DestroyThread(FEXCore::Core::InternalThreadState *Thread);
    void CopyMemoryMapping(FEXCore::Core::InternalThreadState *ParentThread, FEXCore::Core::InternalThreadState *ChildThread);

    void CleanupAfterFork(FEXCore::Core::InternalThreadState *ExceptForThread);

    std::vector<FEXCore::Core::InternalThreadState*>* GetThreads() { return &Threads; }

    uint8_t GetGPRSize() const { return Config.Is64BitMode ? 8 : 4; }

    IR::AOTIRCacheEntry *LoadAOTIRCacheEntry(const std::string &filename);
    void UnloadAOTIRCacheEntry(IR::AOTIRCacheEntry *Entry);

    FEXCore::JITSymbols Symbols;

    // Public for threading
    void ExecutionThread(FEXCore::Core::InternalThreadState *Thread);

    void FinalizeAOTIRCache() {
      IRCaptureCache.FinalizeAOTIRCache();
    }

    void WriteFilesWithCode(std::function<void(const std::string& fileid, const std::string& filename)> Writer) {
      IRCaptureCache.WriteFilesWithCode(Writer);
    }

    void SetAOTIRLoader(std::function<int(const std::string&)> CacheReader) {
      IRCaptureCache.SetAOTIRLoader(CacheReader);
    }

    void SetAOTIRWriter(std::function<std::unique_ptr<std::ofstream>(const std::string&)> CacheWriter) {
      IRCaptureCache.SetAOTIRWriter(CacheWriter);
    }

    void SetAOTIRRenamer(std::function<void(const std::string&)> CacheRenamer) {
      IRCaptureCache.SetAOTIRRenamer(CacheRenamer);
    }

    void AppendThunkDefinitions(std::vector<FEXCore::IR::ThunkDefinition> const& Definitions);

    FEXCore::Utils::PooledAllocatorMMap OpDispatcherAllocator;
    FEXCore::Utils::PooledAllocatorMMap FrontendAllocator;

    void MarkMemoryShared();

    bool IsTSOEnabled() { return (IsMemoryShared || !Config.TSOAutoMigration) && Config.TSOEnabled; }

  protected:
    void ClearCodeCache(FEXCore::Core::InternalThreadState *Thread);

  private:
    /**
     * @brief Does some final thread initialization
     *
     * @param Thread The internal FEX thread state object
     *
     * InitCore and CreateThread both call this to finish up thread object initialization
     */
    void InitializeThreadData(FEXCore::Core::InternalThreadState *Thread);

    /**
     * @brief Initializes the JIT compilers for the thread
     *
     * @param State The internal FEX thread state object
     *
     * InitializeCompiler is called inside of CreateThread, so you likely don't need this
     */
    void InitializeCompiler(FEXCore::Core::InternalThreadState* Thread);

    void WaitForIdleWithTimeout();

    void NotifyPause();

    void AddBlockMapping(FEXCore::Core::InternalThreadState *Thread, uint64_t Address, void *Ptr);

    // Entry Cache
    std::mutex ExitMutex;
    std::unique_ptr<GdbServer> DebugServer;

    IR::AOTIRCaptureCache IRCaptureCache;
    std::unique_ptr<FEXCore::CodeSerialize::CodeObjectSerializeService> CodeObjectCacheService;

    bool StartPaused = false;
    bool IsMemoryShared = false;
    FEX_CONFIG_OPT(AppFilename, APP_FILENAME);

    std::shared_mutex CustomIRMutex;
    std::unordered_map<uint64_t, std::tuple<std::function<void(uintptr_t Entrypoint, FEXCore::IR::IREmitter *)>, void *, void *>> CustomIRHandlers;
    FEXCore::CPU::CPUBackendFeatures BackendFeatures;
    FEXCore::CPU::DispatcherConfig DispatcherConfig;
  };

  uint64_t HandleSyscall(FEXCore::HLE::SyscallHandler *Handler, FEXCore::Core::CpuStateFrame *Frame, FEXCore::HLE::SyscallArguments *Args);
}
