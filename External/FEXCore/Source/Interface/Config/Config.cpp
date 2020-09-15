#include "LogManager.h"
#include "Interface/Context/Context.h"

#include <FEXCore/Config/Config.h>

namespace FEXCore::Config {
  void SetConfig(FEXCore::Context::Context *CTX, ConfigOption Option, uint64_t Config) {
    switch (Option) {
    case FEXCore::Config::CONFIG_MULTIBLOCK:
      CTX->Config.Multiblock = Config != 0;
    break;
    case FEXCore::Config::CONFIG_MAXBLOCKINST:
      CTX->Config.MaxInstPerBlock = Config;
    break;
    case FEXCore::Config::CONFIG_DEFAULTCORE:
      CTX->Config.Core = static_cast<FEXCore::Config::ConfigCore>(Config);
    break;
    case FEXCore::Config::CONFIG_VIRTUALMEMSIZE:
      CTX->Config.VirtualMemSize = Config;
    break;
    case FEXCore::Config::CONFIG_SINGLESTEP:
      CTX->Config.RunningMode = Config != 0 ? FEXCore::Context::CoreRunningMode::MODE_SINGLESTEP : FEXCore::Context::CoreRunningMode::MODE_RUN;
    break;
    case FEXCore::Config::CONFIG_GDBSERVER:
      Config != 0 ? CTX->StartGdbServer() : CTX->StopGdbServer();
    break;
    case FEXCore::Config::CONFIG_UNIFIED_MEMORY:
      CTX->Config.UnifiedMemory = Config != 0;
    break;
    case FEXCore::Config::CONFIG_IS64BIT_MODE:
      CTX->Config.Is64BitMode = Config != 0;
    break;
    case FEXCore::Config::CONFIG_EMULATED_CPU_CORES:
      CTX->Config.EmulatedCPUCores = std::max(1UL, Config);
    break;
    case FEXCore::Config::CONFIG_TSO_ENABLED:
      CTX->Config.TSOEnabled = Config != 0;
    break;
    case FEXCore::Config::CONFIG_SMC_CHECKS:
      CTX->Config.SMCChecks = Config != 0;
    break;
    default: LogMan::Msg::A("Unknown configuration option");
    }
  }

  void SetConfig(FEXCore::Context::Context *CTX, ConfigOption Option, std::string const &Config) {
    switch (Option) {
    case CONFIG_ROOTFSPATH:
      CTX->Config.RootFSPath = Config;
      break;
    case CONFIG_THUNKLIBSPATH:
      CTX->Config.ThunkLibsPath = Config;
      break;
    default: LogMan::Msg::A("Unknown configuration option");
    }
  }

  uint64_t GetConfig(FEXCore::Context::Context *CTX, ConfigOption Option) {
    switch (Option) {
    case FEXCore::Config::CONFIG_MULTIBLOCK:
      return CTX->Config.Multiblock;
    break;
    case FEXCore::Config::CONFIG_MAXBLOCKINST:
      return CTX->Config.MaxInstPerBlock;
    break;
    case FEXCore::Config::CONFIG_DEFAULTCORE:
      return CTX->Config.Core;
    break;
    case FEXCore::Config::CONFIG_VIRTUALMEMSIZE:
      return CTX->Config.VirtualMemSize;
    break;
    case FEXCore::Config::CONFIG_SINGLESTEP:
      return CTX->Config.RunningMode == FEXCore::Context::CoreRunningMode::MODE_SINGLESTEP ? 1 : 0;
    case FEXCore::Config::CONFIG_GDBSERVER:
      return CTX->GetGdbServerStatus();
    break;
    case FEXCore::Config::CONFIG_UNIFIED_MEMORY:
      return CTX->Config.UnifiedMemory;
    break;
    case FEXCore::Config::CONFIG_IS64BIT_MODE:
      return CTX->Config.Is64BitMode;
    break;
    case FEXCore::Config::CONFIG_EMULATED_CPU_CORES:
      return CTX->Config.EmulatedCPUCores;
    break;
    case FEXCore::Config::CONFIG_TSO_ENABLED:
      return CTX->Config.TSOEnabled;
    break;
    case FEXCore::Config::CONFIG_SMC_CHECKS:
      return CTX->Config.SMCChecks;
    break;
    default: LogMan::Msg::A("Unknown configuration option");
    }

    return 0;
  }
}

