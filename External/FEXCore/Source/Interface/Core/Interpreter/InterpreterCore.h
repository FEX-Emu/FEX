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

void InitializeInterpreterOpHandlers();

[[nodiscard]] std::unique_ptr<CPUBackend> CreateInterpreterCore(FEXCore::Context::Context *ctx,
                                                                FEXCore::Core::InternalThreadState *Thread,
                                                                bool CompileThread);

} // namespace FEXCore::CPU
