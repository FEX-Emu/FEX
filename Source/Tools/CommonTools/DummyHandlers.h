// SPDX-License-Identifier: MIT
#pragma once
#include <FEXCore/Core/SignalDelegator.h>
#include <FEXCore/HLE/SyscallHandler.h>

namespace FEX::DummyHandlers {

class DummySyscallHandler : public FEXCore::HLE::SyscallHandler, public FEXCore::Allocator::FEXAllocOperators {
public:
  uint64_t HandleSyscall(FEXCore::Core::CpuStateFrame* Frame, FEXCore::HLE::SyscallArguments* Args) override {
    // Don't do anything
    return 0;
  }

  FEXCore::HLE::SyscallABI GetSyscallABI(uint64_t Syscall) override {
    // Don't do anything
    return {0, false, 0};
  }

  // These are no-ops implementations of the SyscallHandler API
  FEXCore::HLE::AOTIRCacheEntryLookupResult LookupAOTIRCacheEntry(FEXCore::Core::InternalThreadState* Thread, uint64_t GuestAddr) override {
    return {0, 0};
  }

  FEXCore::HLE::ExecutableRangeInfo QueryGuestExecutableRange(FEXCore::Core::InternalThreadState* Thread, uint64_t Address) override {
    return {0, UINT64_MAX, true};
  }
};

class DummySignalDelegator final : public FEXCore::SignalDelegator, public FEXCore::Allocator::FEXAllocOperators {
public:
  FEXCore::Core::InternalThreadState* GetBackingTLSThread() {
    return GetTLSThread();
  }

protected:
  void RegisterTLSState(FEXCore::Core::InternalThreadState* Thread);
  void UninstallTLSState(FEXCore::Core::InternalThreadState* Thread);

private:
  FEXCore::Core::InternalThreadState* GetTLSThread();
};

fextl::unique_ptr<FEXCore::HLE::SyscallHandler> CreateSyscallHandler();
fextl::unique_ptr<FEX::DummyHandlers::DummySignalDelegator> CreateSignalDelegator();
} // namespace FEX::DummyHandlers
