#pragma once

#include "Common/JitSymbols.h"
#include "Interface/Core/CPUID.h"
#include "Interface/Core/HostFeatures.h"
#include "Interface/Core/X86HelperGen.h"
#include <FEXCore/Config/Config.h>
#include <FEXCore/Core/Context.h>
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Core/SignalDelegator.h>
#include <FEXCore/Debug/InternalThreadState.h>
#include <FEXCore/Utils/CompilerDefs.h>
#include <FEXCore/Utils/Event.h>
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

namespace CPU {
  class Arm64JITCore;
  class X86JITCore;
}
namespace HLE {
struct SyscallArguments;
class SyscallHandler;
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

  struct AOTIRInlineEntry {
    uint64_t GuestHash;
    uint64_t GuestLength;

    /* RAData followed by IRData */
    uint8_t InlineData[0];

    IR::RegisterAllocationData *GetRAData();
    IR::IRListView *GetIRData();
  };

  struct AOTIRInlineIndexEntry {
    uint64_t GuestStart;
    uint64_t DataOffset;
  };

  struct AOTIRInlineIndex {
    uint64_t Count;
    uint64_t DataBase;
    AOTIRInlineIndexEntry Entries[0];

    AOTIRInlineEntry *Find(uint64_t GuestStart);
    AOTIRInlineEntry *GetInlineEntry(uint64_t DataOffset);
  };

  struct AOTIRCaptureCacheEntry {
    std::unique_ptr<std::ostream> Stream;
    std::map<uint64_t, uint64_t> Index;

    void AppendAOTIRCaptureCache(uint64_t GuestRIP, uint64_t Start, uint64_t Length, uint64_t Hash, FEXCore::IR::IRListView *IRList, FEXCore::IR::RegisterAllocationData *RAData);
  };

  struct Context {
    friend class FEXCore::HLE::SyscallHandler;
  #ifdef JIT_ARM64
    friend class FEXCore::CPU::Arm64JITCore;
  #endif
  #ifdef JIT_X86_64
    friend class FEXCore::CPU::X86JITCore;
  #endif

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
      FEX_CONFIG_OPT(ThunkConfigFile, THUNKCONFIG);
      FEX_CONFIG_OPT(DumpIR, DUMPIR);
      FEX_CONFIG_OPT(StaticRegisterAllocation, SRA);
      FEX_CONFIG_OPT(GlobalJITNaming, GLOBALJITNAMING);
      FEX_CONFIG_OPT(LibraryJITNaming, LIBRARYJITNAMING);
      FEX_CONFIG_OPT(BlockJITNaming, BLOCKJITNAMING);
      FEX_CONFIG_OPT(ParanoidTSO, PARANOIDTSO);
    } Config;

    using IntCallbackReturn =  FEX_NAKED void(*)(FEXCore::Core::InternalThreadState *Thread, volatile void *Host_RSP);
    IntCallbackReturn InterpreterCallbackReturn;

    FEXCore::HostFeatures HostFeatures;

    std::mutex ThreadCreationMutex;
    uint64_t ThreadID{};
    FEXCore::Core::InternalThreadState* ParentThread;
    std::vector<FEXCore::Core::InternalThreadState*> Threads;
    std::atomic_bool CoreShuttingDown{false};

    std::mutex IdleWaitMutex;
    std::condition_variable IdleWaitCV;
    std::atomic<uint32_t> IdleWaitRefCount{};

    Event PauseWait;
    bool Running{};

    FEXCore::CPUIDEmu CPUID;
    FEXCore::HLE::SyscallHandler *SyscallHandler{};
    std::unique_ptr<FEXCore::ThunkHandler> ThunkHandler;

    CustomCPUFactoryType CustomCPUFactory;
    FEXCore::Context::ExitHandler CustomExitHandler;

    struct AOTIRCacheEntry {
      AOTIRInlineIndex *Array;
      void *mapping;
      size_t size;
    };

    std::unordered_map<std::string, AOTIRCacheEntry> AOTIRCache;
    std::function<int(const std::string&)> AOTIRLoader;
    std::function<std::unique_ptr<std::ostream>(const std::string&)> AOTIRWriter;
    std::unordered_map<std::string, AOTIRCaptureCacheEntry> AOTIRCaptureCache;

    struct AddrToFileEntry {
      uint64_t Start;
      uint64_t Len;
      uint64_t Offset;
      std::string fileid;
      std::string filename;
      void *CachedFileEntry;
      bool ContainsCode;
    };

    using AddrToFileMapType = std::map<uint64_t, AddrToFileEntry>;
    AddrToFileMapType AddrToFile;
    std::map<std::string, std::string> FilesWithCode;

    AddrToFileMapType::iterator FindAddrForFile(uint64_t Entry, uint64_t Length);
#ifdef BLOCKSTATS
    std::unique_ptr<FEXCore::BlockSamplingData> BlockData;
#endif

    SignalDelegator *SignalDelegation{};
    X86GeneratedCode X86CodeGen;

    Context();
    ~Context();

    FEXCore::Core::InternalThreadState* InitCore(FEXCore::CodeLoader *Loader);
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
    void RegisterHostSignalHandler(int Signal, HostSignalDelegatorFunction Func, bool Required);
    void RegisterFrontendHostSignalHandler(int Signal, HostSignalDelegatorFunction Func, bool Required);

    static void RemoveCodeEntry(FEXCore::Core::InternalThreadState *Thread, uint64_t GuestRIP);

    // Wrapper which takes CpuStateFrame instead of InternalThreadState
    static void RemoveCodeEntryFromJit(FEXCore::Core::CpuStateFrame *Frame, uint64_t GuestRIP) {
      RemoveCodeEntry(Frame->Thread, GuestRIP);
    }

    // Debugger interface
    void CompileRIP(FEXCore::Core::InternalThreadState *Thread, uint64_t RIP);
    uint64_t GetThreadCount() const;
    FEXCore::Core::RuntimeStats *GetRuntimeStatsForThread(uint64_t Thread);
    bool GetDebugDataForRIP(uint64_t RIP, FEXCore::Core::DebugData *Data);
    bool FindHostCodeForRIP(uint64_t RIP, uint8_t **Code);

    struct GenerateIRResult {
      FEXCore::IR::IRListView* IRList;
      // User's responsibility to deallocate this.
      FEXCore::IR::RegisterAllocationData* RAData;
      uint64_t TotalInstructions;
      uint64_t TotalInstructionsLength;
      uint64_t StartAddr;
      uint64_t Length;
    };
    [[nodiscard]] GenerateIRResult GenerateIR(FEXCore::Core::InternalThreadState *Thread, uint64_t GuestRIP);

    struct CompileCodeResult {
      void* CompiledCode;
      FEXCore::IR::IRListView* IRData;
      FEXCore::Core::DebugData* DebugData;
      // User's responsibility to deallocate this.
      FEXCore::IR::RegisterAllocationData* RAData;
      bool GeneratedIR;
      uint64_t StartAddr;
      uint64_t Length;
    };
    [[nodiscard]] CompileCodeResult CompileCode(FEXCore::Core::InternalThreadState *Thread, uint64_t GuestRIP);
    uintptr_t CompileBlock(FEXCore::Core::CpuStateFrame *Frame, uint64_t GuestRIP);

    // same as CompileBlock, but aborts on failure
    void CompileBlockJit(FEXCore::Core::CpuStateFrame *Frame, uint64_t GuestRIP);

    bool LoadAOTIRCache(int streamfd);
    void FinalizeAOTIRCache();
    void WriteFilesWithCode(std::function<void(const std::string& fileid, const std::string& filename)> Writer);
    /**
     * @brief Initializes the JIT compilers for the thread
     *
     * @param State The internal FEX thread state object
     * @param CompileThread Is this for the compile service or not?
     *
     * InitializeCompiler is called inside of CreateThread, so you likely don't need this
     * This is exposed because the CompileService needs to initialize compilers while copying data from
     * the paired InternalThreadState that it is compiling code for
     */
    void InitializeCompiler(FEXCore::Core::InternalThreadState* State, bool CompileThread);

    // Used for thread creation from syscalls
    /**
     * @brief Used to create FEX thread objects in preparation for creating a true OS thread
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
     * @brief Initializes the TLS data for a thread
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

    void AddNamedRegion(uintptr_t Base, uintptr_t Size, uintptr_t Offset, const std::string &filename);
    void RemoveNamedRegion(uintptr_t Base, uintptr_t Size);

    FEXCore::JITSymbols Symbols;

    // Public for threading
    void ExecutionThread(FEXCore::Core::InternalThreadState *Thread);

  protected:
    void ClearCodeCache(FEXCore::Core::InternalThreadState *Thread, bool AlsoClearIRCache);

  private:
    /**
     * @brief Does some final thread initialization
     *
     * @param Thread The internal FEX thread state object
     *
     * InitCore and CreateThread both call this to finish up thread object initialization
     */
    void InitializeThreadData(FEXCore::Core::InternalThreadState *Thread);

    void WaitForIdleWithTimeout();

    void NotifyPause();

    void AddBlockMapping(FEXCore::Core::InternalThreadState *Thread, uint64_t Address, void *Ptr, uint64_t Start, uint64_t Length);
    FEXCore::CodeLoader *LocalLoader{};

    // Entry Cache
    uint64_t StartingRIP;
    std::mutex ExitMutex;
    std::unique_ptr<GdbServer> DebugServer;

    std::shared_mutex AOTIRCacheLock;
    std::shared_mutex AOTIRCaptureCacheWriteoutLock;
    std::atomic<bool> AOTIRCaptureCacheWriteoutFlusing;

    std::queue<std::function<void()>> AOTIRCaptureCacheWriteoutQueue;
    void AOTIRCaptureCacheWriteoutQueue_Flush();
    void AOTIRCaptureCacheWriteoutQueue_Append(const std::function<void()> &fn);

    bool StartPaused = false;
    FEX_CONFIG_OPT(AppFilename, APP_FILENAME);
  };

  uint64_t HandleSyscall(FEXCore::HLE::SyscallHandler *Handler, FEXCore::Core::CpuStateFrame *Frame, FEXCore::HLE::SyscallArguments *Args);
}
