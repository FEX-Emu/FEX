// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>

namespace FEXCore::Core {
struct CPUState;
} // namespace FEXCore::Core

namespace FEX::HLE {
class SignalDelegator;
}

void RunAsHost(FEX::HLE::SignalDelegator* SignalDelegation, uintptr_t InitialRip, FEXCore::Core::CPUState* OutputState);
