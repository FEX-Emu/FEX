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

  FEXCore::Context::Context *FEXCore::Context::Context::CreateNewContext() {
    return new FEXCore::Context::ContextImpl{};
  }

  void FEXCore::Context::Context::DestroyContext(FEXCore::Context::Context *CTX) {
    CTX->DestroyContext();
    delete CTX;
  }

  bool FEXCore::Context::ContextImpl::InitializeContext() {
    return FEXCore::CPU::CreateCPUCore(this);
  }

  void FEXCore::Context::ContextImpl::DestroyContext() {
    if (ParentThread) {
      DestroyThread(ParentThread);
    }
  }

  void FEXCore::Context::ContextImpl::SetExitHandler(ExitHandler handler) {
    CustomExitHandler = std::move(handler);
  }

  ExitHandler FEXCore::Context::ContextImpl::GetExitHandler() const {
    return CustomExitHandler;
  }

  void FEXCore::Context::ContextImpl::Stop() {
    Stop(false);
  }

  void FEXCore::Context::ContextImpl::CompileRIP(FEXCore::Core::InternalThreadState *Thread, uint64_t GuestRIP) {
    CompileBlock(Thread->CurrentFrame, GuestRIP);
  }

  FEXCore::Context::ExitReason FEXCore::Context::ContextImpl::GetExitReason() {
    return ParentThread->ExitReason;
  }

  bool FEXCore::Context::ContextImpl::IsDone() const {
    return IsPaused();
  }

  void FEXCore::Context::ContextImpl::GetCPUState(FEXCore::Core::CPUState *State) const {
    memcpy(State, ParentThread->CurrentFrame, sizeof(FEXCore::Core::CPUState));
  }

  void FEXCore::Context::ContextImpl::SetCPUState(const FEXCore::Core::CPUState *State) {
    memcpy(ParentThread->CurrentFrame, State, sizeof(FEXCore::Core::CPUState));
  }

  void FEXCore::Context::ContextImpl::SetCustomCPUBackendFactory(CustomCPUFactoryType Factory) {
    CustomCPUFactory = std::move(Factory);
  }

  bool FEXCore::Context::ContextImpl::AddVirtualMemoryMapping([[maybe_unused]] uint64_t VirtualAddress, [[maybe_unused]] uint64_t PhysicalAddress, [[maybe_unused]] uint64_t Size) {
    return false;
  }

  HostFeatures FEXCore::Context::ContextImpl::GetHostFeatures() const {
    return HostFeatures;
  }

  void FEXCore::Context::ContextImpl::SetSignalDelegator(FEXCore::SignalDelegator *_SignalDelegation) {
    SignalDelegation = _SignalDelegation;
  }

  void FEXCore::Context::ContextImpl::SetSyscallHandler(FEXCore::HLE::SyscallHandler *Handler) {
    SyscallHandler = Handler;
    SourcecodeResolver = Handler->GetSourcecodeResolver();
  }

  FEXCore::CPUID::FunctionResults FEXCore::Context::ContextImpl::RunCPUIDFunction(uint32_t Function, uint32_t Leaf) {
    return CPUID.RunFunction(Function, Leaf);
  }

  FEXCore::CPUID::FunctionResults FEXCore::Context::ContextImpl::RunCPUIDFunctionName(uint32_t Function, uint32_t Leaf, uint32_t CPU) {
    return CPUID.RunFunctionName(Function, Leaf, CPU);
  }
  void SetVDSOSigReturn(FEXCore::Context::Context *CTX, const VDSOSigReturn &Pointers) {
    CTX->SetVDSOSigReturn(Pointers);
  }

namespace Debug {
  //void CompileRIP(FEXCore::Context::Context *CTX, uint64_t RIP) {
  //  CTX->CompileRIP(CTX->ParentThread, RIP);
  //}
  //uint64_t GetThreadCount(FEXCore::Context::Context *CTX) {
  //  return CTX->GetThreadCount();
  //}

  //FEXCore::Core::RuntimeStats *GetRuntimeStatsForThread(FEXCore::Context::Context *CTX, uint64_t Thread) {
  //  return CTX->GetRuntimeStatsForThread(Thread);
  //}

  //bool GetDebugDataForRIP(FEXCore::Context::Context *CTX, uint64_t RIP, FEXCore::Core::DebugData *Data) {
  //  return CTX->GetDebugDataForRIP(RIP, Data);
  //}

  //bool FindHostCodeForRIP(FEXCore::Context::Context *CTX, uint64_t RIP, uint8_t **Code) {
  //  return CTX->FindHostCodeForRIP(RIP, Code);
  //}

  // XXX:
  // bool FindIRForRIP(FEXCore::Context::Context *CTX, uint64_t RIP, FEXCore::IR::IntrusiveIRList **ir) {
  //   return CTX->FindIRForRIP(RIP, ir);
  // }

  // void SetIRForRIP(FEXCore::Context::Context *CTX, uint64_t RIP, FEXCore::IR::IntrusiveIRList *const ir) {
  //   CTX->SetIRForRIP(RIP, ir);
  // }
}

}
