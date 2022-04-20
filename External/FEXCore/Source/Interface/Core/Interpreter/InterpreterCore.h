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

[[nodiscard]] std::unique_ptr<CPUBackend> CreateInterpreterCore(FEXCore::Context::Context *ctx,
                                                                FEXCore::Core::InternalThreadState *Thread);

void InitializeInterpreterSignalHandlers(FEXCore::Context::Context *CTX);

} // namespace FEXCore::CPU
