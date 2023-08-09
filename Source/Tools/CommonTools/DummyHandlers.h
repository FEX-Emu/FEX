#pragma once
#include <FEXCore/Core/SignalDelegator.h>
#include <FEXCore/HLE/SyscallHandler.h>

namespace FEX::DummyHandlers {

class DummySyscallHandler: public FEXCore::HLE::SyscallHandler, public FEXCore::Allocator::FEXAllocOperators {
  public:
  uint64_t HandleSyscall(FEXCore::Core::CpuStateFrame *Frame, FEXCore::HLE::SyscallArguments *Args) override {
    // Don't do anything
    return 0;
  }

  FEXCore::HLE::SyscallABI GetSyscallABI(uint64_t Syscall) override {
    // Don't do anything
    return {0, false, 0};
  }

  // These are no-ops implementations of the SyscallHandler API
  FEXCore::HLE::AOTIRCacheEntryLookupResult LookupAOTIRCacheEntry(FEXCore::Core::InternalThreadState *Thread, uint64_t GuestAddr) override {
    return {0, 0};
  }
};

class DummySignalDelegator final : public FEXCore::SignalDelegator, public FEXCore::Allocator::FEXAllocOperators {
  public:
  void CheckXIDHandler() override {}

  void SignalThread(FEXCore::Core::InternalThreadState *Thread, FEXCore::Core::SignalEvent Event) override {}

  FEXCore::Core::InternalThreadState *GetBackingTLSThread() {
    return GetTLSThread();
  }

  protected:
    // Called from the thunk handler to handle the signal
    void HandleGuestSignal(FEXCore::Core::InternalThreadState *Thread, int Signal, void *Info, void *UContext) override {}

    void RegisterTLSState(FEXCore::Core::InternalThreadState *Thread) override;
    void UninstallTLSState(FEXCore::Core::InternalThreadState *Thread) override;

    FEXCore::Core::InternalThreadState *GetTLSThread() override;

    /**
     * @brief Registers a signal handler for the host to handle a signal
     *
     * It's a process level signal handler so one must be careful
     */
    void FrontendRegisterHostSignalHandler(int Signal, FEXCore::HostSignalDelegatorFunction Func, bool Required) override {}
    void FrontendRegisterFrontendHostSignalHandler(int Signal, FEXCore::HostSignalDelegatorFunction Func, bool Required) override {}
};

fextl::unique_ptr<FEXCore::HLE::SyscallHandler> CreateSyscallHandler();
fextl::unique_ptr<FEX::DummyHandlers::DummySignalDelegator> CreateSignalDelegator();
}
