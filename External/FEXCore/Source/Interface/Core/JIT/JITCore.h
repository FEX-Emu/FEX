#pragma once

#include <memory>

namespace FEXCore::Context {
struct Context;
}

namespace FEXCore::Core {
struct InternalThreadState;
}

namespace FEXCore::CPU {
class CPUBackend;

[[nodiscard]] std::unique_ptr<CPUBackend> CreateX86JITCore(FEXCore::Context::Context *ctx,
                                                           FEXCore::Core::InternalThreadState *Thread,
                                                           bool CompileThread);
void InitializeX86JITSignalHandlers(FEXCore::Context::Context *CTX);

[[nodiscard]] std::unique_ptr<CPUBackend> CreateArm64JITCore(FEXCore::Context::Context *ctx,
                                                             FEXCore::Core::InternalThreadState *Thread,
                                                             bool CompileThread);
void InitializeArm64JITSignalHandlers(FEXCore::Context::Context *CTX);

[[nodiscard]] std::unique_ptr<CPUBackend> CreateRISCVJITCore(FEXCore::Context::Context *ctx,
                                                             FEXCore::Core::InternalThreadState *Thread,
                                                             bool CompileThread);
void InitializeRISCVJITSignalHandlers(FEXCore::Context::Context *CTX);

} // namespace FEXCore::CPU
