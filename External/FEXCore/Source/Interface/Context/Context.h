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
#include <FEXCore/Utils/Event.h>
#include <stdint.h>

#include <memory>
#include <map>
#include <set>
#include <mutex>
#include <istream>
#include <ostream>

namespace FEXCore {
class ThunkHandler;
class BlockSamplingData;
class GdbServer;
class SiganlDelegator;

namespace CPU {
  class JITCore;
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

  struct Context {
    friend class FEXCore::HLE::SyscallHandler;
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
      std::string RootFSPath;
      std::string ThunkLibsPath;

      bool Is64BitMode {true};
      bool TSOEnabled {true};
      bool SMCChecks {false};
      bool ABILocalFlags {false};
      bool ABINoPF {false};

      bool AOTIRGenerate {false};
      bool AOTIRLoad {false};

      std::string DumpIR;

      // this is for internal use
      bool ValidateIRarser { false };

    } Config;

    using IntCallbackReturn =  __attribute__((naked)) void(*)(FEXCore::Core::InternalThreadState *Thread, volatile void *Host_RSP);
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
    CustomCPUFactoryType FallbackCPUFactory;
    std::function<void(uint64_t ThreadId, FEXCore::Context::ExitReason)> CustomExitHandler;

    struct AOTCacheEntry {
      uint64_t start;
      uint64_t len;
      uint64_t crc;
      IR::IRListView *IR;
      IR::RegisterAllocationData *RAData;
    };
    std::map<uint64_t, AOTCacheEntry> AOTCache;
#ifdef BLOCKSTATS
    std::unique_ptr<FEXCore::BlockSamplingData> BlockData;
#endif

    SignalDelegator *SignalDelegation{};
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
    void RegisterHostSignalHandler(int Signal, HostSignalDelegatorFunction Func);
    void RegisterFrontendHostSignalHandler(int Signal, HostSignalDelegatorFunction Func);

    static void RemoveCodeEntry(FEXCore::Core::InternalThreadState *Thread, uint64_t GuestRIP);

    // Debugger interface
    void CompileRIP(FEXCore::Core::InternalThreadState *Thread, uint64_t RIP);
    uint64_t GetThreadCount() const;
    FEXCore::Core::RuntimeStats *GetRuntimeStatsForThread(uint64_t Thread);
    FEXCore::Core::CPUState GetCPUState();
    bool GetDebugDataForRIP(uint64_t RIP, FEXCore::Core::DebugData *Data);
    bool FindHostCodeForRIP(uint64_t RIP, uint8_t **Code);

    // XXX:
    // bool FindIRForRIP(uint64_t RIP, FEXCore::IR::IntrusiveIRList **ir);
    // void SetIRForRIP(uint64_t RIP, FEXCore::IR::IntrusiveIRList *const ir);
    FEXCore::Core::ThreadState *GetThreadState();
    void LoadEntryList();

    std::tuple<FEXCore::IR::IRListView *, FEXCore::IR::RegisterAllocationData *, uint64_t, uint64_t, uint64_t, uint64_t> GenerateIR(FEXCore::Core::InternalThreadState *Thread, uint64_t GuestRIP);

    std::tuple<void *, FEXCore::IR::IRListView *, FEXCore::Core::DebugData *, FEXCore::IR::RegisterAllocationData *, bool, uint64_t, uint64_t> CompileCode(FEXCore::Core::InternalThreadState *Thread, uint64_t GuestRIP);
    uintptr_t CompileBlock(FEXCore::Core::InternalThreadState *Thread, uint64_t GuestRIP);

    bool LoadAOTIRCache(std::istream &stream);
    void WriteAOTIRCache(std::ostream &stream);
    // Used for thread creation from syscalls
    void InitializeCompiler(FEXCore::Core::InternalThreadState* State, bool CompileThread);
    FEXCore::Core::InternalThreadState* CreateThread(FEXCore::Core::CPUState *NewThreadState, uint64_t ParentTID);
    void InitializeThreadData(FEXCore::Core::InternalThreadState *Thread);
    void InitializeThread(FEXCore::Core::InternalThreadState *Thread);
    void CopyMemoryMapping(FEXCore::Core::InternalThreadState *ParentThread, FEXCore::Core::InternalThreadState *ChildThread);
    void RunThread(FEXCore::Core::InternalThreadState *Thread);

    std::vector<FEXCore::Core::InternalThreadState*> *const GetThreads() { return &Threads; }

#if ENABLE_JITSYMBOLS
    FEXCore::JITSymbols Symbols;
#endif

  protected:
    void ClearCodeCache(FEXCore::Core::InternalThreadState *Thread, bool AlsoClearIRCache);

  private:
    void WaitForIdleWithTimeout();

    void ExecutionThread(FEXCore::Core::InternalThreadState *Thread);
    void NotifyPause();

    void AddBlockMapping(FEXCore::Core::InternalThreadState *Thread, uint64_t Address, void *Ptr);

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
    FEXCore::Config::Value<std::string> AppFilename{FEXCore::Config::CONFIG_APP_FILENAME, ""};
  };

  uint64_t HandleSyscall(FEXCore::HLE::SyscallHandler *Handler, FEXCore::Core::InternalThreadState *Thread, FEXCore::HLE::SyscallArguments *Args);
}
