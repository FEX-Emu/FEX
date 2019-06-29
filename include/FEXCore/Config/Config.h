#pragma once
#include <FEXCore/Core/Context.h>

#include <stdint.h>

namespace FEXCore::Config {
  enum ConfigOption {
    CONFIG_MULTIBLOCK,
    CONFIG_MAXBLOCKINST,
    CONFIG_DEFAULTCORE,
    CONFIG_VIRTUALMEMSIZE,
    CONFIG_SINGLESTEP,
  };

  enum ConfigCore {
    CONFIG_INTERPRETER,
    CONFIG_IRJIT,
    CONFIG_LLVMJIT,
    CONFIG_CUSTOM,
  };

  void SetConfig(FEXCore::Context::Context *CTX, ConfigOption Option, uint64_t Config);
  uint64_t GetConfig(FEXCore::Context::Context *CTX, ConfigOption Option);
}
