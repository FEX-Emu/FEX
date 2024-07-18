// SPDX-License-Identifier: MIT
#include "Common/ArgumentLoader.h"
#include <FEXCore/Config/Config.h>
#include <FEXCore/fextl/fmt.h>
#include <FEXCore/fextl/string.h>
#include <FEXCore/fextl/vector.h>

#include "cpp-optparse/OptionParser.h"
#include "git_version.h"

#include <stdint.h>

namespace FEX::ArgLoader {
void FEX::ArgLoader::ArgLoader::Load() {
  RemainingArgs.clear();
  ProgramArguments.clear();
  if (Type == LoadType::WITHOUT_FEXLOADER_PARSER) {
    LoadWithoutArguments();
    return;
  }

  optparse::OptionParser Parser {};
  Parser.version("FEX-Emu (" GIT_DESCRIBE_STRING ") ");
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
  RemainingArgs = Parser.args();
  ProgramArguments = Parser.parsed_args();
}

void FEX::ArgLoader::ArgLoader::LoadWithoutArguments() {
  // Skip argument 0, which will be the interpreter
  for (int i = 1; i < argc; ++i) {
    RemainingArgs.emplace_back(argv[i]);
  }

  // Put the interpreter in ProgramArguments
  ProgramArguments.emplace_back(argv[0]);
}

} // namespace FEX::ArgLoader
