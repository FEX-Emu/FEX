#pragma once

#include <FEXCore/fextl/memory.h>

namespace FEXCore::CPU {
  class CPUBackend;
}
namespace FEXCore::Context{
  class Context;
}
namespace FEXCore::Core {
  struct InternalThreadState;
  struct CPUState;
}

namespace FEX::HLE {
  class SignalDelegator;
}

void RunAsHost(fextl::unique_ptr<FEX::HLE::SignalDelegator> &SignalDelegation, uintptr_t InitialRip, uintptr_t StackPointer, FEXCore::Core::CPUState *OutputState);
