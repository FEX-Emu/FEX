#include "Common/Config.h"

#include "OptionParser.h"

namespace FEX::ArgLoader {
  std::vector<std::string> RemainingArgs;
  std::vector<std::string> ProgramArguments;

  void Load(int argc, char **argv) {

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
        .set_default("irint");

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

      CPUGroup.add_option("--disable-tso")
        .dest("TSOEnabled")
        .action("store_false")
        .help("Disables TSO IR ops. Highly likely to break any threaded application")
        .set_default(true);

      CPUGroup.add_option("--smc-full-checks")
        .dest("SMCChecks")
        .action("store_true")
        .help("Checks code for modification before execution. Slow.")
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

      EmulationGroup.add_option("-U", "--unified-memory")
        .dest("UnifiedMemory")
        .action("store_true")
        .help("Enable unified memory for the emulator");

      EmulationGroup.add_option("-E", "--env")
        .dest("Env")
        .help("Adds an environment variable")
        .action("append");

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

      Parser.add_option_group(LoggingGroup);
    }

    optparse::Values Options = Parser.parse_args(argc, argv);

    {
      if (Options.is_set_by_user("Core")) {
        auto Core = Options["Core"];
        if (Core == "irint")
          Config::Add("Core", "0");
        else if (Core == "irjit")
          Config::Add("Core", "1");
#ifdef _M_X86_64
        else if (Core == "host")
          Config::Add("Core", "2");
#endif
      }

      if (Options.is_set_by_user("Break")) {
        bool Break = Options.get("Break");
        Config::Add("Break", std::to_string(Break));
      }

      if (Options.is_set_by_user("SingleStep")) {
        bool SingleStep = Options.get("SingleStep");
        Config::Add("SingleStep", std::to_string(SingleStep));

        // Single stepping also enforces single instruction size blocks
        Config::Add("MaxInst", std::to_string(1u));
      }
      else {
        if (Options.is_set_by_user("MaxInst")) {
          uint32_t MaxInst = Options.get("MaxInst");
          Config::Add("MaxInst", std::to_string(MaxInst));
        }
      }

      if (Options.is_set_by_user("Multiblock")) {
        bool Multiblock = Options.get("Multiblock");
        Config::Add("Multiblock", std::to_string(Multiblock));
      }

      if (Options.is_set_by_user("GdbServer")) {
        bool GdbServer = Options.get("GdbServer");
        Config::Add("GdbServer", std::to_string(GdbServer));
      }

      if (Options.is_set_by_user("Threads")) {
        uint64_t Config = Options.get("Threads");
        Config::Add("Threads", std::to_string(Config));
      }

      if (Options.is_set_by_user("TSOEnabled")) {
        bool TSOEnabled = Options.get("TSOEnabled");
        Config::Add("TSOEnabled", std::to_string(TSOEnabled));
      }
      
      if (Options.is_set_by_user("SMCChecks")) {
        bool SMCChecks = Options.get("SMCChecks");
        Config::Add("SMCChecks", std::to_string(SMCChecks));
      }
    }

    {
      if (Options.is_set_by_user("RootFS")) {
        std::string Option = Options["RootFS"];
        Config::Add("RootFS", Option);
      }

      if (Options.is_set_by_user("ThunkLibs")) {
        std::string Option = Options["ThunkLibs"];
        Config::Add("ThunkLibs", Option);
      }

      if (Options.is_set_by_user("UnifiedMemory")) {
        bool Option = Options.get("UnifiedMemory");
        Config::Add("UnifiedMemory", std::to_string(Option));
      }

      if (Options.is_set_by_user("Env")) {
        for (auto iter = Options.all("Env").begin(); iter != Options.all("Env").end(); ++iter) {
          Config::Append("Env", *iter);
        }
      }
    }

    {
      if (Options.is_set_by_user("DumpGPRs")) {
        bool DumpGPRs = Options.get("DumpGPRs");
        Config::Add("DumpGPRs", std::to_string(DumpGPRs));
      }

      if (Options.is_set_by_user("IPCClient")) {
        bool IPCClient = Options.get("IPCClient");
        Config::Add("IPCClient", std::to_string(IPCClient));
      }

      if (Options.is_set_by_user("ELFType")) {
        bool ELFType = Options.get("ELFType");
        Config::Add("ELFType", std::to_string(ELFType));
      }
      if (Options.is_set_by_user("IPCID")) {
        const char* Value = Options.get("IPCID");
        Config::Add("IPCID", Value);
      }
    }

    {
      if (Options.is_set_by_user("SilentLog")) {
        bool SilentLog = Options.get("SilentLog");
        Config::Add("SilentLog", std::to_string(SilentLog));
      }

      if (Options.is_set_by_user("OutputLog")) {
        std::string OutputLog = Options["OutputLog"];
        Config::Add("OutputLog", OutputLog);
      }
    }

    RemainingArgs = Parser.args();
    ProgramArguments = Parser.parsed_args();
  }

  std::vector<std::string> Get() {
    return RemainingArgs;
  }
  std::vector<std::string> GetParsedArgs() {
    return ProgramArguments;
  }

}
