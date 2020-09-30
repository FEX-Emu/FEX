#pragma once
#include "Common/JitSymbols.h"
#include "Interface/Core/CPUID.h"
#include "Interface/Core/Frontend.h"
#include "Interface/Core/HostFeatures.h"
#include "Interface/Core/InternalThreadState.h"
#include "Interface/Core/SignalDelegator.h"
#include "Interface/Core/X86HelperGen.h"
#include "Interface/HLE/Syscalls.h"
#include "Interface/Memory/MemMapper.h"
#include "Interface/IR/PassManager.h"
#include <FEXCore/Config/Config.h>
#include <FEXCore/Core/CPUBackend.h>
#include <FEXCore/Utils/Event.h>
#include <stdint.h>

#include <memory>
#include <mutex>

namespace FEXCore {
class SyscallHandler;
class ThunkHandler;
class BlockSamplingData;
class GdbServer;

namespace CPU {
  class JITCore;
}
}

namespace FEXCore::IR {
  class RegisterAllocationPass;
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
    friend class FEXCore::SyscallHandler;
    friend class FEXCore::CPU::JITCore;
    friend class FEXCore::IR::Validation::IRValidation;

    struct {
      bool Multiblock {false};
      bool BreakOnFrontendFailure {true};
      int64_t MaxInstPerBlock {-1LL};
      uint64_t VirtualMemSize {1ULL << 36};
      CoreRunningMode RunningMode {CoreRunningMode::MODE_RUN};
      FEXCore::Config::ConfigCore Core {FEXCore::Config::CONFIG_INTERPRETER};
      bool GdbServer {false};
      bool UnifiedMemory {true};
      std::string RootFSPath;
      std::string ThunkLibsPath;

      bool Is64BitMode {true};
      uint64_t EmulatedCPUCores{1};
      bool TSOEnabled {true};
      bool SMCChecks {false};
    } Config;

    FEXCore::Memory::MemMapper MemoryMapper;
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
    std::unique_ptr<FEXCore::SyscallHandler> SyscallHandler;
    std::unique_ptr<FEXCore::ThunkHandler> ThunkHandler;

    CustomCPUFactoryType CustomCPUFactory;
    CustomCPUFactoryType FallbackCPUFactory;
    std::function<void(uint64_t ThreadId, FEXCore::Context::ExitReason)> CustomExitHandler;

#ifdef BLOCKSTATS
    std::unique_ptr<FEXCore::BlockSamplingData> BlockData;
#endif

    SignalDelegator SignalDelegation;
    X86GeneratedCode X86CodeGen;

    Context();
    ~Context();

    bool InitCore(FEXCore::CodeLoader *Loader);
    FEXCore::Context::ExitReason RunUntilExit();
    int GetProgramStatus();
    bool IsPaused() const { return !Running; }
    void Pause();
    void Run();
    void WaitForThreadsToRun();
    void Step();
    void Stop(bool IgnoreCurrentThread);
    void WaitForIdle();
    void StopThread(FEXCore::Core::InternalThreadState *Thread);
    void SignalThread(FEXCore::Core::InternalThreadState *Thread, FEXCore::Core::SignalEvent Event);

    bool GetGdbServerStatus() { return (bool)DebugServer; }
    void StartGdbServer();
    void StopGdbServer();
    void HandleCallback(uint64_t RIP);
    void RegisterFrontendHostSignalHandler(int Signal, HostSignalDelegatorFunction Func);

    static void RemoveCodeEntry(FEXCore::Core::InternalThreadState *Thread, uint64_t GuestRIP);

    // Debugger interface
    void CompileRIP(FEXCore::Core::InternalThreadState *Thread, uint64_t RIP);
    uint64_t GetThreadCount() const;
    FEXCore::Core::RuntimeStats *GetRuntimeStatsForThread(uint64_t Thread);
    FEXCore::Core::CPUState GetCPUState();
    void GetMemoryRegions(std::vector<FEXCore::Memory::MemRegion> *Regions);
    bool GetDebugDataForRIP(uint64_t RIP, FEXCore::Core::DebugData *Data);
    bool FindHostCodeForRIP(uint64_t RIP, uint8_t **Code);

    // XXX:
    // bool FindIRForRIP(uint64_t RIP, FEXCore::IR::IntrusiveIRList **ir);
    // void SetIRForRIP(uint64_t RIP, FEXCore::IR::IntrusiveIRList *const ir);
    FEXCore::Core::ThreadState *GetThreadState();
    void LoadEntryList();

    uintptr_t CompileBlock(FEXCore::Core::InternalThreadState *Thread, uint64_t GuestRIP);
    uintptr_t CompileFallbackBlock(FEXCore::Core::InternalThreadState *Thread, uint64_t GuestRIP);

    FEXCore::CodeLoader *GetCodeLoader() const { return LocalLoader; }

    // Used for thread creation from syscalls
    FEXCore::Core::InternalThreadState* CreateThread(FEXCore::Core::CPUState *NewThreadState, uint64_t ParentTID);
    void InitializeThreadData(FEXCore::Core::InternalThreadState *Thread);
    void InitializeThread(FEXCore::Core::InternalThreadState *Thread);
    void CopyMemoryMapping(FEXCore::Core::InternalThreadState *ParentThread, FEXCore::Core::InternalThreadState *ChildThread);
    void RunThread(FEXCore::Core::InternalThreadState *Thread);

    std::vector<FEXCore::Core::InternalThreadState*> *const GetThreads() { return &Threads; }

  protected:
    void ClearCodeCache(FEXCore::Core::InternalThreadState *Thread, uint64_t GuestRIP);

  private:
    void WaitForIdleWithTimeout();

    void *MapRegion(FEXCore::Core::InternalThreadState *Thread, uint64_t Offset, uint64_t Size, bool Fixed = false, bool RelativeToBase = true);
    void *ShmBase();
    void MirrorRegion(FEXCore::Core::InternalThreadState *Thread, void *HostPtr, uint64_t Offset, uint64_t Size);
    void ExecutionThread(FEXCore::Core::InternalThreadState *Thread);
    void NotifyPause();

    uintptr_t AddBlockMapping(FEXCore::Core::InternalThreadState *Thread, uint64_t Address, void *Ptr);

    FEXCore::CodeLoader *LocalLoader{};

    // Entry Cache
    bool GetFilenameHash(std::string const &Filename, std::string &Hash);
    void AddThreadRIPsToEntryList(FEXCore::Core::InternalThreadState *Thread);
    void SaveEntryList();
    std::set<uint64_t> EntryList;
    std::vector<uint64_t> InitLocations;
    uint64_t StartingRIP;
    std::mutex ExitMutex;
    std::unique_ptr<GdbServer> DebugServer;

    bool StartPaused = false;
#if ENABLE_JITSYMBOLS
    FEXCore::JITSymbols Symbols;
#endif
  };
}
