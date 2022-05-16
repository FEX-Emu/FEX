#include "Common/Paths.h"
#include "Interface/Context/Context.h"
#include "Interface/Core/Core.h"
#include "Interface/Core/OpcodeDispatcher.h"
#include "Interface/Core/X86Tables/X86Tables.h"

#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Core/Context.h>
#include <FEXCore/Core/CPUID.h>
#include <FEXCore/Core/SignalDelegator.h>
#include "FEXCore/Debug/InternalThreadState.h"

#include <string.h>
#include <utility>

namespace FEXCore::HLE {
  class SyscallVisitor;
}

namespace FEXCore::Context {
  void InitializeStaticTables(OperatingMode Mode) {
    FEXCore::Paths::InitializePaths();
    X86Tables::InitializeInfoTables(Mode);
    IR::InstallOpcodeHandlers(Mode);
  }

  void ShutdownStaticTables() {
    FEXCore::Paths::ShutdownPaths();
  }

  FEXCore::Context::Context *CreateNewContext() {
    return new FEXCore::Context::Context{};
  }

  bool InitializeContext(FEXCore::Context::Context *CTX) {
    return FEXCore::CPU::CreateCPUCore(CTX);
  }

  void DestroyContext(FEXCore::Context::Context *CTX) {
    if (CTX->ParentThread) {
      CTX->DestroyThread(CTX->ParentThread);
    }
    delete CTX;
  }

  FEXCore::Core::InternalThreadState* InitCore(FEXCore::Context::Context *CTX, FEXCore::CodeLoader *Loader) {
    return CTX->InitCore(Loader);
  }

  void SetExitHandler(FEXCore::Context::Context *CTX, ExitHandler handler) {
    CTX->CustomExitHandler = std::move(handler);
  }

  ExitHandler GetExitHandler(const FEXCore::Context::Context *CTX) {
    return CTX->CustomExitHandler;
  }

  void Run(FEXCore::Context::Context *CTX) {
    CTX->Run();
  }

  void Step(FEXCore::Context::Context *CTX) {
    CTX->Step();
  }

  void CompileRIP(FEXCore::Core::InternalThreadState *Thread, uint64_t GuestRIP) {
    Thread->CTX->CompileBlock(Thread->CurrentFrame, GuestRIP);
  }

  FEXCore::Context::ExitReason RunUntilExit(FEXCore::Context::Context *CTX) {
    return CTX->RunUntilExit();
  }

  int GetProgramStatus(const FEXCore::Context::Context *CTX) {
    return CTX->GetProgramStatus();
  }

  FEXCore::Context::ExitReason GetExitReason(const FEXCore::Context::Context *CTX) {
    return CTX->ParentThread->ExitReason;
  }

  bool IsDone(const FEXCore::Context::Context *CTX) {
    return CTX->IsPaused();
  }

  void GetCPUState(const FEXCore::Context::Context *CTX, FEXCore::Core::CPUState *State) {
    memcpy(State, CTX->ParentThread->CurrentFrame, sizeof(FEXCore::Core::CPUState));
  }

  void SetCPUState(FEXCore::Context::Context *CTX, const FEXCore::Core::CPUState *State) {
    memcpy(CTX->ParentThread->CurrentFrame, State, sizeof(FEXCore::Core::CPUState));
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

  bool AddVirtualMemoryMapping([[maybe_unused]] FEXCore::Context::Context *CTX, [[maybe_unused]] uint64_t VirtualAddress, [[maybe_unused]] uint64_t PhysicalAddress, [[maybe_unused]] uint64_t Size) {
    return false;
  }

  void RegisterExternalSyscallVisitor(FEXCore::Context::Context *CTX, [[maybe_unused]] uint64_t Syscall, [[maybe_unused]] FEXCore::HLE::SyscallVisitor *Visitor) {
  }

  void HandleCallback(FEXCore::Context::Context *CTX, FEXCore::Core::InternalThreadState *Thread, uint64_t RIP) {
    CTX->HandleCallback(Thread, RIP);
  }

  void RegisterHostSignalHandler(FEXCore::Context::Context *CTX, int Signal, HostSignalDelegatorFunction Func, bool Required) {
      CTX->RegisterHostSignalHandler(Signal, std::move(Func), Required);
  }

  void RegisterFrontendHostSignalHandler(FEXCore::Context::Context *CTX, int Signal, HostSignalDelegatorFunction Func, bool Required) {
    CTX->RegisterFrontendHostSignalHandler(Signal, std::move(Func), Required);
  }

  FEXCore::Core::InternalThreadState* CreateThread(FEXCore::Context::Context *CTX, FEXCore::Core::CPUState *NewThreadState, uint64_t ParentTID) {
    return CTX->CreateThread(NewThreadState, ParentTID);
  }

  void ExecutionThread(FEXCore::Context::Context *CTX, FEXCore::Core::InternalThreadState *Thread) {
    return CTX->ExecutionThread(Thread);
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

  void DestroyThread(FEXCore::Context::Context *CTX, FEXCore::Core::InternalThreadState *Thread) {
    CTX->DestroyThread(Thread);
  }

  void CleanupAfterFork(FEXCore::Context::Context *CTX, FEXCore::Core::InternalThreadState *Thread) {
    CTX->CleanupAfterFork(Thread);
  }
  
  void SetSignalDelegator(FEXCore::Context::Context *CTX, FEXCore::SignalDelegator *SignalDelegation) {
    CTX->SignalDelegation = SignalDelegation;
  }

  void SetSyscallHandler(FEXCore::Context::Context *CTX, FEXCore::HLE::SyscallHandler *Handler) {
    CTX->SyscallHandler = Handler;
  }

  FEXCore::CPUID::FunctionResults RunCPUIDFunction(FEXCore::Context::Context *CTX, uint32_t Function, uint32_t Leaf) {
    return CTX->CPUID.RunFunction(Function, Leaf);
  }

  FEX_DEFAULT_VISIBILITY FEXCore::CPUID::FunctionResults RunCPUIDFunctionName(FEXCore::Context::Context *CTX, uint32_t Function, uint32_t Leaf, uint32_t CPU) {
    return CTX->CPUID.RunFunctionName(Function, Leaf, CPU);
  }

  void SetAOTIRLoader(FEXCore::Context::Context *CTX, std::function<int(const std::string&)> CacheReader) {
    CTX->SetAOTIRLoader(CacheReader);
  }

  void SetAOTIRWriter(FEXCore::Context::Context *CTX, std::function<std::unique_ptr<std::ofstream>(const std::string&)> CacheWriter) {
    CTX->SetAOTIRWriter(CacheWriter);
  }

  void SetAOTIRRenamer(FEXCore::Context::Context *CTX, std::function<void(const std::string&)> CacheRenamer) {
    CTX->SetAOTIRRenamer(CacheRenamer);
  }

  void FinalizeAOTIRCache(FEXCore::Context::Context *CTX) {
    CTX->FinalizeAOTIRCache();
  }

  void WriteFilesWithCode(FEXCore::Context::Context *CTX, std::function<void(const std::string& fileid, const std::string& filename)> Writer) {
    CTX->WriteFilesWithCode(Writer);
  }

  IR::AOTIRCacheEntry *LoadAOTIRCacheEntry(FEXCore::Context::Context *CTX, const std::string &Name) {
    return CTX->LoadAOTIRCacheEntry(Name);
  }
  void UnloadAOTIRCacheEntry(FEXCore::Context::Context *CTX, IR::AOTIRCacheEntry *Entry) {
    return CTX->UnloadAOTIRCacheEntry(Entry);
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
}

}
