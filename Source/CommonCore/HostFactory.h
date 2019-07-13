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
  FEXCore::CPU::CPUBackend *CPUCreationFactory(FEXCore::Context::Context* CTX, FEXCore::Core::ThreadState *Thread);
  FEXCore::CPU::CPUBackend *CPUCreationFactoryFallback(FEXCore::Context::Context* CTX, FEXCore::Core::ThreadState *Thread);
}
