// SPDX-License-Identifier: MIT
#pragma once

#include "Interface/Core/CPUBackend.h"
#include <FEXCore/fextl/memory.h>

namespace FEXCore::Context {
class ContextImpl;
}

namespace FEXCore::Core {
struct InternalThreadState;
}

namespace FEXCore::CPU {
class CPUBackend;

[[nodiscard]]
fextl::unique_ptr<CPUBackend> CreateArm64JITCore(FEXCore::Context::ContextImpl* ctx, FEXCore::Core::InternalThreadState* Thread);

} // namespace FEXCore::CPU
