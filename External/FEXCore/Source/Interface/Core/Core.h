#pragma once

namespace FEXCore {
  class CodeLoader;
}

namespace FEXCore::Context {
  class ContextImpl;
}

namespace FEXCore::CPU {

  /**
   * @brief Create the CPU core backend for the context passed in
   *
   * @param CTX
   *
   * @return true if core was able to be create
   */
  bool CreateCPUCore(FEXCore::Context::ContextImpl *CTX);

  bool LoadCode(FEXCore::Context::ContextImpl *CTX, FEXCore::CodeLoader *Loader);
}
