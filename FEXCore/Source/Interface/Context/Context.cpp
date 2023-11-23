// SPDX-License-Identifier: MIT
#include "Interface/Context/Context.h"
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
    X86Tables::InitializeInfoTables(Mode);
    IR::InstallOpcodeHandlers(Mode);
  }

  fextl::unique_ptr<FEXCore::Context::Context> FEXCore::Context::Context::CreateNewContext() {
    return fextl::make_unique<FEXCore::Context::ContextImpl>();
  }

  bool FEXCore::Context::ContextImpl::InitializeContext() {
    // This should be used for generating things that are shared between threads
    CPUID.Init(this);
    return true;
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

  void FEXCore::Context::ContextImpl::CompileRIPCount(FEXCore::Core::InternalThreadState *Thread, uint64_t GuestRIP, uint64_t MaxInst) {
    CompileBlock(Thread->CurrentFrame, GuestRIP, MaxInst);
  }

  bool FEXCore::Context::ContextImpl::IsDone() const {
    return IsPaused();
  }

  void FEXCore::Context::ContextImpl::GetCPUState(FEXCore::Core::InternalThreadState *Thread, FEXCore::Core::CPUState *State) const {
    memcpy(State, Thread->CurrentFrame, sizeof(FEXCore::Core::CPUState));
  }

  void FEXCore::Context::ContextImpl::SetCPUState(FEXCore::Core::InternalThreadState *Thread, const FEXCore::Core::CPUState *State) {
    memcpy(Thread->CurrentFrame, State, sizeof(FEXCore::Core::CPUState));
  }

  void FEXCore::Context::ContextImpl::SetCustomCPUBackendFactory(CustomCPUFactoryType Factory) {
    CustomCPUFactory = std::move(Factory);
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

  FEXCore::CPUID::XCRResults FEXCore::Context::ContextImpl::RunXCRFunction(uint32_t Function) {
    return CPUID.RunXCRFunction(Function);
  }

  FEXCore::CPUID::FunctionResults FEXCore::Context::ContextImpl::RunCPUIDFunctionName(uint32_t Function, uint32_t Leaf, uint32_t CPU) {
    return CPUID.RunFunctionName(Function, Leaf, CPU);
  }
}
