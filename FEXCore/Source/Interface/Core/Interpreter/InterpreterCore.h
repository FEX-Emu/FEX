// SPDX-License-Identifier: MIT
#pragma once

#include <FEXCore/Core/CPUBackend.h>
#include <FEXCore/fextl/memory.h>

namespace FEXCore::Context {
class ContextImpl;
}

namespace FEXCore::Core {
  struct InternalThreadState;
}

namespace FEXCore::CPU {
class CPUBackend;
struct DispatcherConfig;

[[nodiscard]] fextl::unique_ptr<CPUBackend> CreateInterpreterCore(FEXCore::Context::ContextImpl *ctx,
                                                                FEXCore::Core::InternalThreadState *Thread);
void InitializeInterpreterSignalHandlers(FEXCore::Context::ContextImpl *CTX);
CPUBackendFeatures GetInterpreterBackendFeatures();

} // namespace FEXCore::CPU
