#pragma once

#include "Common/JitSymbols.h"
#include "Interface/Core/CPUID.h"
#include "Interface/Core/Frontend.h"
#include "Interface/Core/HostFeatures.h"
#include "Interface/Core/InternalThreadState.h"
#include "Interface/Core/X86HelperGen.h"
#include "Interface/IR/PassManager.h"
#include "Interface/IR/Passes/RegisterAllocationPass.h"
#include <FEXCore/Config/Config.h>
#include <FEXCore/Core/CPUBackend.h>
#include <FEXCore/Utils/CompilerDefs.h>
#include <FEXCore/Utils/Event.h>
#include <stdint.h>

#include <functional>
#include <istream>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <ostream>
#include <set>
#include <shared_mutex>
#include <unordered_map>
#include <queue>

namespace FEXCore {
class ThunkHandler;
class BlockSamplingData;
class GdbServer;
class SiganlDelegator;

namespace CPU {
  class Arm64JITCore;
  class X86JITCore;
}
namespace HLE {
class SyscallHandler;
}
}

namespace FEXCore::IR {
  class RegisterAllocationPass;
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
      FEX_CONFIG_OPT(DumpIR, DUMPIR);
      FEX_CONFIG_OPT(StaticRegisterAllocation, SRA);
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

    std::map<uint64_t, AddrToFileEntry> AddrToFile;
    std::map<std::string, std::string> FilesWithCode;

#ifdef BLOCKSTATS
    std::unique_ptr<FEXCore::BlockSamplingData> BlockData;
#endif

    SignalDelegator *SignalDelegation{};
    X86GeneratedCode X86CodeGen;

    Context();
    ~Context();

    bool InitCore(FEXCore::CodeLoader *Loader);
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
    void HandleCallback(uint64_t RIP);
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
    
    // Used for thread creation from syscalls
    void InitializeCompiler(FEXCore::Core::InternalThreadState* State, bool CompileThread);
    FEXCore::Core::InternalThreadState* CreateThread(FEXCore::Core::CPUState *NewThreadState, uint64_t ParentTID);
    void InitializeThreadData(FEXCore::Core::InternalThreadState *Thread);
    void InitializeThread(FEXCore::Core::InternalThreadState *Thread);
    void CopyMemoryMapping(FEXCore::Core::InternalThreadState *ParentThread, FEXCore::Core::InternalThreadState *ChildThread);
    void RunThread(FEXCore::Core::InternalThreadState *Thread);

    void DestroyThread(FEXCore::Core::InternalThreadState *Thread);
    void CleanupAfterFork(FEXCore::Core::InternalThreadState *ExceptForThread);

    std::vector<FEXCore::Core::InternalThreadState*>* GetThreads() { return &Threads; }

    uint8_t GetGPRSize() const { return Config.Is64BitMode ? 8 : 4; }

    void AddNamedRegion(uintptr_t Base, uintptr_t Size, uintptr_t Offset, const std::string &filename);
    void RemoveNamedRegion(uintptr_t Base, uintptr_t Size);

#if ENABLE_JITSYMBOLS
    FEXCore::JITSymbols Symbols;
#endif

    // Public for threading
    void ExecutionThread(FEXCore::Core::InternalThreadState *Thread);

  protected:
    void ClearCodeCache(FEXCore::Core::InternalThreadState *Thread, bool AlsoClearIRCache);

  private:
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
