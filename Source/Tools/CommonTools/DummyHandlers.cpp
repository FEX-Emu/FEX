// SPDX-License-Identifier: MIT
#include "DummyHandlers.h"

namespace FEX::DummyHandlers {
  thread_local FEXCore::Core::InternalThreadState *TLSThread;

  void DummySignalDelegator::RegisterTLSState(FEXCore::Core::InternalThreadState *Thread) {
    TLSThread = Thread;
  }

  void DummySignalDelegator::UninstallTLSState(FEXCore::Core::InternalThreadState *Thread) {
    TLSThread = nullptr;
  }

  FEXCore::Core::InternalThreadState *DummySignalDelegator::GetTLSThread() {
    return TLSThread;
  }

fextl::unique_ptr<FEXCore::HLE::SyscallHandler> CreateSyscallHandler() {
  return fextl::make_unique<DummySyscallHandler>();
}

fextl::unique_ptr<FEX::DummyHandlers::DummySignalDelegator> CreateSignalDelegator() {
  return fextl::make_unique<DummySignalDelegator>();
}
}
