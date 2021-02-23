#include "Common/Paths.h"
#include "Interface/Context/Context.h"
#include "Interface/Core/Core.h"
#include "Interface/Core/OpcodeDispatcher.h"

#include <FEXCore/Config/Config.h>
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Debug/X86Tables.h>

namespace FEXCore::Context {
  void InitializeStaticTables(OperatingMode Mode) {
    FEXCore::Paths::InitializePaths();
    X86Tables::InitializeInfoTables(Mode);
    IR::InstallOpcodeHandlers(Mode);
  }

  FEXCore::Context::Context *CreateNewContext() {
    return new FEXCore::Context::Context{};
  }

  bool InitializeContext(FEXCore::Context::Context *CTX) {
    return FEXCore::CPU::CreateCPUCore(CTX);
  }

  void DestroyContext(FEXCore::Context::Context *CTX) {
    delete CTX;
  }

  bool InitCore(FEXCore::Context::Context *CTX, FEXCore::CodeLoader *Loader) {
    return CTX->InitCore(Loader);
  }

  void SetExitHandler(FEXCore::Context::Context *CTX,
      std::function<void(uint64_t ThreadId, FEXCore::Context::ExitReason)> handler) {
    CTX->CustomExitHandler = handler;
  }

  std::function<void(uint64_t ThreadId, FEXCore::Context::ExitReason)> GetExitHandler(FEXCore::Context::Context *CTX) {
    return CTX->CustomExitHandler;
  }

  void Run(FEXCore::Context::Context *CTX) {
    CTX->Run();
  }

  void Step(FEXCore::Context::Context *CTX) {
    CTX->Step();
  }


  FEXCore::Context::ExitReason RunUntilExit(FEXCore::Context::Context *CTX) {
    return CTX->RunUntilExit();
  }

  int GetProgramStatus(FEXCore::Context::Context *CTX) {
    return CTX->GetProgramStatus();
  }

  FEXCore::Context::ExitReason GetExitReason(FEXCore::Context::Context *CTX) {
    return CTX->ParentThread->ExitReason;
  }

  bool IsDone(FEXCore::Context::Context *CTX) {
    return CTX->IsPaused();
  }

  void GetCPUState(FEXCore::Context::Context *CTX, FEXCore::Core::CPUState *State) {
    memcpy(State, &CTX->ParentThread->State.State, sizeof(FEXCore::Core::CPUState));
  }

  void SetCPUState(FEXCore::Context::Context *CTX, FEXCore::Core::CPUState *State) {
    memcpy(&CTX->ParentThread->State.State, State, sizeof(FEXCore::Core::CPUState));
  }

  void Pause(FEXCore::Context::Context *CTX) {
    CTX->Pause();
  }

  void Stop(FEXCore::Context::Context *CTX) {
    CTX->Stop(false);
  }

  void SetCustomCPUBackendFactory(FEXCore::Context::Context *CTX, CustomCPUFactoryType Factory) {
    CTX->CustomCPUFactory = std::move(Factory);
  }

  void SetFallbackCPUBackendFactory(FEXCore::Context::Context *CTX, CustomCPUFactoryType Factory) {
    CTX->FallbackCPUFactory = std::move(Factory);
  }

  bool AddVirtualMemoryMapping([[maybe_unused]] FEXCore::Context::Context *CTX, [[maybe_unused]] uint64_t VirtualAddress, [[maybe_unused]] uint64_t PhysicalAddress, [[maybe_unused]] uint64_t Size) {
    return false;
  }

  void RegisterExternalSyscallVisitor(FEXCore::Context::Context *CTX, [[maybe_unused]] uint64_t Syscall, [[maybe_unused]] FEXCore::HLE::SyscallVisitor *Visitor) {
  }

  void HandleCallback(FEXCore::Context::Context *CTX, uint64_t RIP) {
    CTX->HandleCallback(RIP);
  }

  void RegisterHostSignalHandler(FEXCore::Context::Context *CTX, int Signal, HostSignalDelegatorFunction Func) {
      CTX->RegisterHostSignalHandler(Signal, Func);
  }

  void RegisterFrontendHostSignalHandler(FEXCore::Context::Context *CTX, int Signal, HostSignalDelegatorFunction Func) {
    CTX->RegisterFrontendHostSignalHandler(Signal, Func);
  }

  FEXCore::Core::InternalThreadState* CreateThread(FEXCore::Context::Context *CTX, FEXCore::Core::CPUState *NewThreadState, uint64_t ParentTID) {
    return CTX->CreateThread(NewThreadState, ParentTID);
  }

  void InitializeThread(FEXCore::Context::Context *CTX, FEXCore::Core::InternalThreadState *Thread) {
    return CTX->InitializeThread(Thread);
  }

  void RunThread(FEXCore::Context::Context *CTX, FEXCore::Core::InternalThreadState *Thread) {
    CTX->RunThread(Thread);
  }

  void StopThread(FEXCore::Context::Context *CTX, FEXCore::Core::InternalThreadState *Thread) {
    CTX->StopThread(Thread);
  }

  void DeleteForkedThreads(FEXCore::Context::Context *CTX, FEXCore::Core::InternalThreadState *Thread) {
    // This function is called after fork
    // We need to cleanup some of the thread data that is dead
    for (auto &DeadThread : CTX->Threads) {
      if (DeadThread == Thread) {
        continue;
      }

      // Setting running to false ensures that when they are shutdown we won't send signals to kill them
      DeadThread->State.RunningEvents.Running = false;
    }

    // We now only have one thread
    CTX->IdleWaitRefCount = 1;
  }

  void SetSignalDelegator(FEXCore::Context::Context *CTX, FEXCore::SignalDelegator *SignalDelegation) {
    CTX->SignalDelegation = SignalDelegation;
  }

  void SetSyscallHandler(FEXCore::Context::Context *CTX, FEXCore::HLE::SyscallHandler *Handler) {
    CTX->SyscallHandler = Handler;
  }

  FEXCore::CPUID::FunctionResults RunCPUIDFunction(FEXCore::Context::Context *CTX, uint32_t Function, [[maybe_unused]] uint32_t Leaf) {
    return CTX->CPUID.RunFunction(Function);
  }

  void SetAOTIRLoader(FEXCore::Context::Context *CTX, std::function<std::unique_ptr<std::istream>(const std::string&)> CacheReader) {
    CTX->AOTIRLoader = CacheReader;
  }

  bool WriteAOTIR(FEXCore::Context::Context *CTX, std::function<std::unique_ptr<std::ostream>(const std::string&)> CacheWriter) {
    return CTX->WriteAOTIRCache(CacheWriter);
  }

  void AddNamedRegion(FEXCore::Context::Context *CTX, uintptr_t Base, uintptr_t Length, uintptr_t Offset, const std::string& Name) {
    return CTX->AddNamedRegion(Base, Length, Offset, Name);
  }
  void RemoveNamedRegion(FEXCore::Context::Context *CTX, uintptr_t Base, uintptr_t Length) {
    return CTX->RemoveNamedRegion(Base, Length);
  }

namespace Debug {
  void CompileRIP(FEXCore::Context::Context *CTX, uint64_t RIP) {
    CTX->CompileRIP(CTX->ParentThread, RIP);
  }
  uint64_t GetThreadCount(FEXCore::Context::Context *CTX) {
    return CTX->GetThreadCount();
  }

  FEXCore::Core::RuntimeStats *GetRuntimeStatsForThread(FEXCore::Context::Context *CTX, uint64_t Thread) {
    return CTX->GetRuntimeStatsForThread(Thread);
  }

  FEXCore::Core::CPUState GetCPUState(FEXCore::Context::Context *CTX) {
    return CTX->GetCPUState();
  }

  bool GetDebugDataForRIP(FEXCore::Context::Context *CTX, uint64_t RIP, FEXCore::Core::DebugData *Data) {
    return CTX->GetDebugDataForRIP(RIP, Data);
  }

  bool FindHostCodeForRIP(FEXCore::Context::Context *CTX, uint64_t RIP, uint8_t **Code) {
    return CTX->FindHostCodeForRIP(RIP, Code);
  }

  // XXX:
  // bool FindIRForRIP(FEXCore::Context::Context *CTX, uint64_t RIP, FEXCore::IR::IntrusiveIRList **ir) {
  //   return CTX->FindIRForRIP(RIP, ir);
  // }

  // void SetIRForRIP(FEXCore::Context::Context *CTX, uint64_t RIP, FEXCore::IR::IntrusiveIRList *const ir) {
  //   CTX->SetIRForRIP(RIP, ir);
  // }

  FEXCore::Core::ThreadState *GetThreadState(FEXCore::Context::Context *CTX) {
    return CTX->GetThreadState();
  }
}

}
