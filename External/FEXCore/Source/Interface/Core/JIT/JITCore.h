#pragma once

#include <memory>

namespace FEXCore::Context {
class ContextImpl;
}

namespace FEXCore::Core {
struct InternalThreadState;
}

namespace FEXCore::CPU {
class CPUBackend;

[[nodiscard]] std::unique_ptr<CPUBackend> CreateX86JITCore(FEXCore::Context::ContextImpl *ctx,
                                                           FEXCore::Core::InternalThreadState *Thread);
void InitializeX86JITSignalHandlers(FEXCore::Context::ContextImpl *CTX);
CPUBackendFeatures GetX86JITBackendFeatures();

[[nodiscard]] std::unique_ptr<CPUBackend> CreateArm64JITCore(FEXCore::Context::ContextImpl *ctx,
                                                             FEXCore::Core::InternalThreadState *Thread);
void InitializeArm64JITSignalHandlers(FEXCore::Context::ContextImpl *CTX);
CPUBackendFeatures GetArm64JITBackendFeatures();

} // namespace FEXCore::CPU
