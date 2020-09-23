namespace FEXCore::CPU {
  class CPUBackend;
}
namespace FEXCore::Context{
  struct Context;
}
namespace FEXCore::Core {
  struct ThreadState;
}

namespace HostCPUFactory {
  FEXCore::CPU::CPUBackend *HostCPUCreationFactory(FEXCore::Context::Context* CTX, FEXCore::Core::ThreadState *Thread);
}

namespace VMFactory {
  FEXCore::CPU::CPUBackend *CPUCreationFactory(FEXCore::Context::Context* CTX, FEXCore::Core::ThreadState *Thread);
  FEXCore::CPU::CPUBackend *CPUCreationFactoryFallback(FEXCore::Context::Context* CTX, FEXCore::Core::ThreadState *Thread);
}
