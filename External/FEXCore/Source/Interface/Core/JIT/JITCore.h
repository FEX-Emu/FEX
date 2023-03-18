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

[[nodiscard]] fextl::unique_ptr<CPUBackend> CreateX86JITCore(FEXCore::Context::ContextImpl *ctx,
                                                           FEXCore::Core::InternalThreadState *Thread);
void InitializeX86JITSignalHandlers(FEXCore::Context::ContextImpl *CTX);
CPUBackendFeatures GetX86JITBackendFeatures();

[[nodiscard]] fextl::unique_ptr<CPUBackend> CreateArm64JITCore(FEXCore::Context::ContextImpl *ctx,
                                                             FEXCore::Core::InternalThreadState *Thread);
void InitializeArm64JITSignalHandlers(FEXCore::Context::ContextImpl *CTX);
CPUBackendFeatures GetArm64JITBackendFeatures();

} // namespace FEXCore::CPU
