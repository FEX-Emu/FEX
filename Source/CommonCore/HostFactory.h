#pragma once

#include <memory>

namespace FEXCore::CPU {
  class CPUBackend;
}
namespace FEXCore::Context{
  struct Context;
}
namespace FEXCore::Core {
  struct ThreadState;
}

namespace HostFactory {
  std::unique_ptr<FEXCore::CPU::CPUBackend> CPUCreationFactory(FEXCore::Context::Context* CTX, FEXCore::Core::InternalThreadState *Thread);
}
