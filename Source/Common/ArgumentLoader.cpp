#include "Common/ArgumentLoader.h"
#include <FEXCore/Config/Config.h>

#include "OptionParser.h"
#include "git_version.h"

#include <stdint.h>

namespace FEX::ArgLoader {
  std::vector<std::string> RemainingArgs;
  std::vector<std::string> ProgramArguments;

  static std::string Version = "FEX-Emu (" GIT_DESCRIBE_STRING ") ";
  void FEX::ArgLoader::ArgLoader::Load() {
    optparse::OptionParser Parser{};
    Parser.version(Version);
    optparse::OptionGroup CPUGroup(Parser, "CPU Core options");
    optparse::OptionGroup EmulationGroup(Parser, "Emulation options");
    optparse::OptionGroup DebugGroup(Parser, "Debug options");
    optparse::OptionGroup HacksGroup(Parser, "Hacks options");
    optparse::OptionGroup MiscGroup(Parser, "Miscellaneous options");
    optparse::OptionGroup LoggingGroup(Parser, "Logging options");
    optparse::OptionGroup FEXServerGroup(Parser, "FEXServer options");

#define BEFORE_PARSE
#include <FEXCore/Config/ConfigOptions.inl>

    Parser.add_option_group(CPUGroup);
    Parser.add_option_group(EmulationGroup);
    Parser.add_option_group(DebugGroup);
    Parser.add_option_group(HacksGroup);
    Parser.add_option_group(MiscGroup);
    Parser.add_option_group(LoggingGroup);

    optparse::Values Options = Parser.parse_args(argc, argv);

    using int32 = int32_t;
    using uint32 = uint32_t;
#define AFTER_PARSE
#include <FEXCore/Config/ConfigOptions.inl>
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
