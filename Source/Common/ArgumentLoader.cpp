#include "Common/ArgumentLoader.h"
#include "Common/Config.h"

#include "OptionParser.h"

namespace FEX::ArgLoader {
  std::vector<std::string> RemainingArgs;
  std::vector<std::string> ProgramArguments;

  void FEX::ArgLoader::ArgLoader::Load() {
    optparse::OptionParser Parser{};
    optparse::OptionGroup CPUGroup(Parser, "CPU Core options");
    optparse::OptionGroup EmulationGroup(Parser, "Emulation options");
    optparse::OptionGroup TestGroup(Parser, "Test Harness options");
    optparse::OptionGroup LoggingGroup(Parser, "Logging options");

    {
      CPUGroup.add_option("-c", "--core")
        .dest("Core")
        .help("Which CPU core to use")
#ifdef _M_X86_64
        .choices({"irint", "irjit", "host"})
#else
        .choices({"irint", "irjit"})
#endif
        .set_default("irjit");

      std::string BreakString = "Break";
      std::string MultiBlockString = "Multiblock";
      Parser.set_defaults(BreakString, "0");
      Parser.set_defaults(MultiBlockString, "0");

      CPUGroup.add_option("-b", "--break")
        .dest("Break")
        .action("store_true")
        .help("Break when op dispatcher doesn't understand instruction");
      CPUGroup.add_option("--no-break")
        .dest("Break")
        .action("store_false")
        .help("Break when op dispatcher doesn't understand instruction");

      CPUGroup.add_option("-s", "--single-step")
        .dest("SingleStep")
        .action("store_true")
        .help("Single Step config");

      CPUGroup.add_option("-n", "--max-inst")
        .dest("MaxInst")
        .help("Maximum number of instructions to stick in a block")
        .set_default(~0U);
      CPUGroup.add_option("-m", "--multiblock")
          .dest("Multiblock")
          .action("store_true")
          .help("Enable Multiblock code compilation");
      CPUGroup.add_option("--no-multiblock")
          .dest("Multiblock")
          .action("store_false")
          .help("Enable Multiblock code compilation");
      CPUGroup.add_option("-G", "--gdb")
          .dest("GdbServer")
          .action("store_true")
          .help("Enables the GDB server");

      CPUGroup.add_option("-T", "--Threads")
          .dest("Threads")
          .help("Number of physical hardware threads to tell the process we have")
          .set_default(1);

      CPUGroup.add_option("--smc-full-checks")
        .dest("SMCChecks")
        .action("store_true")
        .help("Checks code for modification before execution. Slow.")
        .set_default(false);

      CPUGroup.add_option("--unsafe-no-tso")
        .dest("TSOEnabled")
        .action("store_false")
        .help("Disables TSO IR ops. Highly likely to break any threaded application")
        .set_default(true);

      CPUGroup.add_option("--unsafe-local-flags")
        .dest("AbiLocalFlags")
        .action("store_true")
        .help("Assumes flags are not used across calls. Hand-written asm could violate this assumption")
        .set_default(false);

      CPUGroup.add_option("--unsafe-no-pf")
        .dest("AbiNoPF")
        .action("store_true")
        .help("Does not calculate the parity flag on integer operations")
        .set_default(false);

      Parser.add_option_group(CPUGroup);
    }
    {
      Parser.set_defaults("RootFS", "");
      Parser.set_defaults("ThunkLibs", "");

      EmulationGroup.add_option("-R", "--rootfs")
        .dest("RootFS")
        .help("Which Root filesystem prefix to use");

      EmulationGroup.add_option("-t", "--thunklibs")
        .dest("ThunkLibs")
        .help("Folder to find the host-side thunking libs");

      EmulationGroup.add_option("-E", "--env")
        .dest("Env")
        .help("Adds an environment variable")
        .action("append");

      EmulationGroup.add_option("-O")
        .dest("O0")
        .help("Disables optimization passes for debugging")
        .choices({"0"});

      EmulationGroup.add_option("--aotir-capture")
        .dest("AOTIRCapture")
        .help("Captures IR and generates an AOTIR cache for the loaded executable and libs")
        .action("store_true")
        .set_default(false);

      EmulationGroup.add_option("--aotir-load")
        .dest("AOTIRLoad")
        .help("Loads an AOTIR cache for the loaded executable")
        .action("store_true")
        .set_default(false);

      Parser.add_option_group(EmulationGroup);
    }
    {
      TestGroup.add_option("-g", "--dump-gprs")
        .dest("DumpGPRs")
        .action("store_true")
        .help("When Test Harness ends, print GPR state")
        .set_default(false);

      TestGroup.add_option("-C", "--ipc-client")
        .dest("IPCClient")
        .action("store_true")
        .help("If the lockstep runner is a client or server")
        .set_default(false);

      TestGroup.add_option("-I", "--ID")
        .dest("IPCID")
        .help("Sets an ID that is prepended to IPC names. For multiple runners")
        .set_default("0");

      TestGroup.add_option("-e", "--elf")
        .dest("ELFType")
        .action("store_true")
        .help("Lockstep runner should load argument as ELF")
        .set_default(false);

      Parser.add_option_group(TestGroup);
    }

    {
      LoggingGroup.add_option("-s", "--silent")
          .dest("SilentLog")
          .help("Disable logging")
          .action("store_true");

      LoggingGroup.add_option("-o", "--output-log")
          .dest("OutputLog")
          .help("File to write FEX output to [stdout, stderr, <Filename>]")
          .set_default("stderr");

      LoggingGroup.add_option("--dump-ir")
          .dest("DumpIR")
          .help("Folder to dump the IR [no, stdout, stderr, <Folder>]")
          .set_default("no");

      Parser.add_option_group(LoggingGroup);
    }

    optparse::Values Options = Parser.parse_args(argc, argv);

    {
      if (Options.is_set_by_user("Core")) {
        auto Core = Options["Core"];
        if (Core == "irint")
          Set(FEXCore::Config::ConfigOption::CONFIG_DEFAULTCORE, "0");
        else if (Core == "irjit")
          Set(FEXCore::Config::ConfigOption::CONFIG_DEFAULTCORE, "1");
#ifdef _M_X86_64
        else if (Core == "host")
          Set(FEXCore::Config::ConfigOption::CONFIG_DEFAULTCORE, "2");
#endif
      }

      if (Options.is_set_by_user("Break")) {
        bool Break = Options.get("Break");
        Set(FEXCore::Config::ConfigOption::CONFIG_BREAK_ON_FRONTEND, std::to_string(Break));
      }

      if (Options.is_set_by_user("SingleStep")) {
        bool SingleStep = Options.get("SingleStep");
        Set(FEXCore::Config::ConfigOption::CONFIG_SINGLESTEP, std::to_string(SingleStep));

        // Single stepping also enforces single instruction size blocks
        Set(FEXCore::Config::ConfigOption::CONFIG_MAXBLOCKINST, std::to_string(1u));
      }
      else {
        if (Options.is_set_by_user("MaxInst")) {
          uint32_t MaxInst = Options.get("MaxInst");
          Set(FEXCore::Config::ConfigOption::CONFIG_MAXBLOCKINST, std::to_string(MaxInst));
        }
      }

      if (Options.is_set_by_user("Multiblock")) {
        bool Multiblock = Options.get("Multiblock");
        Set(FEXCore::Config::ConfigOption::CONFIG_MULTIBLOCK, std::to_string(Multiblock));
      }

      if (Options.is_set_by_user("GdbServer")) {
        bool GdbServer = Options.get("GdbServer");
        Set(FEXCore::Config::ConfigOption::CONFIG_GDBSERVER, std::to_string(GdbServer));
      }

      if (Options.is_set_by_user("Threads")) {
        uint64_t Config = Options.get("Threads");
        Set(FEXCore::Config::ConfigOption::CONFIG_EMULATED_CPU_CORES, std::to_string(Config));
      }

      if (Options.is_set_by_user("TSOEnabled")) {
        bool TSOEnabled = Options.get("TSOEnabled");
        Set(FEXCore::Config::ConfigOption::CONFIG_TSO_ENABLED, std::to_string(TSOEnabled));
      }

      if (Options.is_set_by_user("SMCChecks")) {
        bool SMCChecks = Options.get("SMCChecks");
        Set(FEXCore::Config::ConfigOption::CONFIG_SMC_CHECKS, std::to_string(SMCChecks));
      }
      if (Options.is_set_by_user("AbiLocalFlags")) {
        bool AbiLocalFlags = Options.get("AbiLocalFlags");
        Set(FEXCore::Config::ConfigOption::CONFIG_ABI_LOCAL_FLAGS, std::to_string(AbiLocalFlags));
      }
      if (Options.is_set_by_user("AbiNoPF")) {
        bool AbiNoPF = Options.get("AbiNoPF");
        Set(FEXCore::Config::ConfigOption::CONFIG_ABI_NO_PF, std::to_string(AbiNoPF));
      }
    }

    {
      if (Options.is_set_by_user("RootFS")) {
        std::string Option = Options["RootFS"];
        Set(FEXCore::Config::ConfigOption::CONFIG_ROOTFSPATH, Option);
      }

      if (Options.is_set_by_user("ThunkLibs")) {
        std::string Option = Options["ThunkLibs"];
        Set(FEXCore::Config::ConfigOption::CONFIG_THUNKLIBSPATH, Option);
      }

      if (Options.is_set_by_user("Env")) {
        for (auto iter = Options.all("Env").begin(); iter != Options.all("Env").end(); ++iter) {
          Set(FEXCore::Config::ConfigOption::CONFIG_ENVIRONMENT, *iter);
        }
      }
      if (Options.is_set_by_user("O0")) {
        Set(FEXCore::Config::ConfigOption::CONFIG_DEBUG_DISABLE_OPTIMIZATION_PASSES, std::to_string(true));
      }

      if (Options.is_set_by_user("AOTIRCapture")) {
        bool AOTIRCapture = Options.get("AOTIRCapture");
        Set(FEXCore::Config::ConfigOption::CONFIG_AOTIR_GENERATE, std::to_string(AOTIRCapture));
      }

      if (Options.is_set_by_user("AOTIRLoad")) {
        bool AOTIRLoad = Options.get("AOTIRLoad");
        Set(FEXCore::Config::ConfigOption::CONFIG_AOTIR_LOAD, std::to_string(AOTIRLoad));
      }
    }

    {
      if (Options.is_set_by_user("DumpGPRs")) {
        bool DumpGPRs = Options.get("DumpGPRs");
        Set(FEXCore::Config::ConfigOption::CONFIG_DUMP_GPRS, std::to_string(DumpGPRs));
      }
    }

    {
      if (Options.is_set_by_user("SilentLog")) {
        bool SilentLog = Options.get("SilentLog");
        Set(FEXCore::Config::ConfigOption::CONFIG_SILENTLOGS, std::to_string(SilentLog));
      }

      if (Options.is_set_by_user("OutputLog")) {
        std::string OutputLog = Options["OutputLog"];
        Set(FEXCore::Config::ConfigOption::CONFIG_OUTPUTLOG, OutputLog);
      }

      if (Options.is_set_by_user("DumpIR")) {
        std::string DumpIR = Options["DumpIR"];
        Set(FEXCore::Config::ConfigOption::CONFIG_DUMPIR, DumpIR);
      }
    }

    RemainingArgs = Parser.args();
    ProgramArguments = Parser.parsed_args();
  }

  void LoadWithoutArguments(int _argc, char **_argv) {
    // Skip argument 0, which will be the interpreter
    for (int i = 1; i < _argc; ++i) {
      RemainingArgs.emplace_back(_argv[i]);
    }

    // Put the interpreter in ProgramArguments
    ProgramArguments.emplace_back(_argv[0]);
  }

  std::vector<std::string> Get() {
    return RemainingArgs;
  }
  std::vector<std::string> GetParsedArgs() {
    return ProgramArguments;
  }

}
