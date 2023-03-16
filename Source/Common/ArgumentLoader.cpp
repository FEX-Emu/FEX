#include "Common/ArgumentLoader.h"
#include <FEXCore/Config/Config.h>
#include <FEXCore/fextl/string.h>
#include <FEXCore/fextl/vector.h>

#include "OptionParser.h"
#include "git_version.h"

#include <stdint.h>

namespace FEX::ArgLoader {
  fextl::vector<fextl::string> RemainingArgs;
  fextl::vector<fextl::string> ProgramArguments;

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
    // TODO: Convert cpp-optparse over to fextl::vector
    auto ParserArgs = Parser.args();
    auto ParsedArgs = Parser.parsed_args();

    RemainingArgs.insert(RemainingArgs.begin(), ParserArgs.begin(), ParserArgs.end());
    ProgramArguments.insert(ProgramArguments.begin(), ParsedArgs.begin(), ParsedArgs.end());
  }

  void LoadWithoutArguments(int _argc, char **_argv) {
    // Skip argument 0, which will be the interpreter
    for (int i = 1; i < _argc; ++i) {
      RemainingArgs.emplace_back(_argv[i]);
    }

    // Put the interpreter in ProgramArguments
    ProgramArguments.emplace_back(_argv[0]);
  }

  fextl::vector<fextl::string> Get() {
    return RemainingArgs;
  }
  fextl::vector<fextl::string> GetParsedArgs() {
    return ProgramArguments;
  }

}
