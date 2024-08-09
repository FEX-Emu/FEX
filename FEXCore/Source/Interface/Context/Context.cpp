// SPDX-License-Identifier: MIT
#include "Interface/Context/Context.h"
#include "Interface/Core/OpcodeDispatcher.h"
#include "Interface/Core/X86Tables/X86Tables.h"

#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Core/Context.h>
#include <FEXCore/Core/CPUID.h>
#include <FEXCore/Core/HostFeatures.h>
#include <FEXCore/Core/SignalDelegator.h>
#include "FEXCore/Debug/InternalThreadState.h"

#include <string.h>
#include <utility>

namespace FEXCore::Context {
void InitializeStaticTables(OperatingMode Mode) {
  X86Tables::InitializeInfoTables(Mode);
  IR::InstallOpcodeHandlers(Mode);
}

fextl::unique_ptr<FEXCore::Context::Context> FEXCore::Context::Context::CreateNewContext(const FEXCore::HostFeatures& Features) {
  return fextl::make_unique<FEXCore::Context::ContextImpl>(Features);
}

void FEXCore::Context::ContextImpl::SetExitHandler(ExitHandler handler) {
  CustomExitHandler = std::move(handler);
}

ExitHandler FEXCore::Context::ContextImpl::GetExitHandler() const {
  return CustomExitHandler;
}

void FEXCore::Context::ContextImpl::CompileRIP(FEXCore::Core::InternalThreadState* Thread, uint64_t GuestRIP) {
  CompileBlock(Thread->CurrentFrame, GuestRIP);
}

void FEXCore::Context::ContextImpl::CompileRIPCount(FEXCore::Core::InternalThreadState* Thread, uint64_t GuestRIP, uint64_t MaxInst) {
  CompileBlock(Thread->CurrentFrame, GuestRIP, MaxInst);
}

void FEXCore::Context::ContextImpl::SetCustomCPUBackendFactory(CustomCPUFactoryType Factory) {
  CustomCPUFactory = std::move(Factory);
}

void FEXCore::Context::ContextImpl::SetSignalDelegator(FEXCore::SignalDelegator* _SignalDelegation) {
  SignalDelegation = _SignalDelegation;
}

void FEXCore::Context::ContextImpl::SetSyscallHandler(FEXCore::HLE::SyscallHandler* Handler) {
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

bool FEXCore::Context::ContextImpl::IsAddressInCodeBuffer(FEXCore::Core::InternalThreadState* Thread, uintptr_t Address) const {
  return Thread->CPUBackend->IsAddressInCodeBuffer(Address);
}
} // namespace FEXCore::Context
