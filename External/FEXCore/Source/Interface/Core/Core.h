#pragma once

namespace FEXCore {
  class CodeLoader;
}

namespace FEXCore::Context {
  struct Context;
}

namespace FEXCore::CPU {

  /**
   * @brief Create the CPU core backend for the context passed in
   *
   * @param CTX
   *
   * @return true if core was able to be create
   */
  bool CreateCPUCore(FEXCore::Context::Context *CTX);

  bool LoadCode(FEXCore::Context::Context *CTX, FEXCore::CodeLoader *Loader);
}
