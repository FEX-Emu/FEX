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

  bool AddGuestMemoryRegion(FEXCore::Context::Context *CTX, FEXCore::SHM::SHMObject *SHM) {
    CTX->MemoryMapper.SetBaseRegion(SHM);
    return true;
  }

  void SetApplicationFile(FEXCore::Context::Context *CTX, std::string const &File) {
    CTX->SyscallHandler->SetFilename(File);
    // XXX: This isn't good for debugging
    // CTX->LoadEntryList();
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

  uint64_t HandleSyscall(FEXCore::Context::Context *CTX, FEXCore::Core::ThreadState *Thread, FEXCore::HLE::SyscallArguments *Args) {
    return FEXCore::HandleSyscall(CTX->SyscallHandler.get(), reinterpret_cast<FEXCore::Core::InternalThreadState*>(Thread), Args);
  }

  bool AddVirtualMemoryMapping([[maybe_unused]] FEXCore::Context::Context *CTX, [[maybe_unused]] uint64_t VirtualAddress, [[maybe_unused]] uint64_t PhysicalAddress, [[maybe_unused]] uint64_t Size) {
    return false;
  }

  void RegisterExternalSyscallVisitor(FEXCore::Context::Context *CTX, [[maybe_unused]] uint64_t Syscall, [[maybe_unused]] FEXCore::HLE::SyscallVisitor *Visitor) {
  }

  void HandleCallback(FEXCore::Context::Context *CTX, uint64_t RIP) {
    CTX->HandleCallback(RIP);
  }

  void RegisterFrontendHostSignalHandler(FEXCore::Context::Context *CTX, int Signal, HostSignalDelegatorFunction Func) {
    CTX->RegisterFrontendHostSignalHandler(Signal, Func);
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

  void GetMemoryRegions(FEXCore::Context::Context *CTX, std::vector<FEXCore::Memory::MemRegion> *Regions) {
    return CTX->GetMemoryRegions(Regions);
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
