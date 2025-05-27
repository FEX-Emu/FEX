// SPDX-License-Identifier: MIT
#pragma once

#include <FEXCore/fextl/memory.h>

namespace FEXCore::CPU {
class CPUBackend;
}
namespace FEXCore::Context {
class Context;
}
namespace FEXCore::Core {
struct InternalThreadState;
struct CPUState;
} // namespace FEXCore::Core

namespace FEX::HLE {
class SignalDelegator;
}

void RunAsHost(fextl::unique_ptr<FEX::HLE::SignalDelegator>& SignalDelegation, uintptr_t InitialRip, FEXCore::Core::CPUState* OutputState);
