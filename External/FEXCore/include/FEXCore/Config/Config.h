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
    CONFIG_GDBSERVER,
    CONFIG_ROOTFSPATH,
    CONFIG_THUNKLIBSPATH,
    CONFIG_UNIFIED_MEMORY,
    CONFIG_IS64BIT_MODE,
    CONFIG_EMULATED_CPU_CORES,
    CONFIG_TSO_ENABLED,
    CONFIG_SMC_CHECKS
  };

  enum ConfigCore {
    CONFIG_INTERPRETER,
    CONFIG_IRJIT,
    CONFIG_CUSTOM,
  };

  void SetConfig(FEXCore::Context::Context *CTX, ConfigOption Option, uint64_t Config);
  void SetConfig(FEXCore::Context::Context *CTX, ConfigOption Option, std::string const &Config);
  uint64_t GetConfig(FEXCore::Context::Context *CTX, ConfigOption Option);
}
