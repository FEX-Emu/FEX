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

[[nodiscard]] std::unique_ptr<CPUBackend> CreateArm64JITCore(FEXCore::Context::Context *ctx,
                                                             FEXCore::Core::InternalThreadState *Thread,
                                                             bool CompileThread);
} // namespace FEXCore::CPU
