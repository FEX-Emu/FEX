// SPDX-License-Identifier: MIT
#include "Common/ArgumentLoader.h"
#include <FEXCore/Config/Config.h>
#include <FEXCore/fextl/fmt.h>
#include <FEXCore/fextl/string.h>
#include <FEXCore/fextl/vector.h>

#include "git_version.h"

namespace FEX::ArgLoader {
enum class ValueType : uint32_t {
  String,
  BoolTrue,
  BoolFalse,
  StringEnum,
  StringArray,
  Max,
};

struct OptionDetails {
  std::string_view Arg;
  FEXCore::Config::ConfigOption Option;
  ValueType ParseType;
};

constexpr static OptionDetails ArgToOption[] = {
#define ARG_TO_CONFIG
#include <FEXCore/Config/ConfigOptions.inl>
};

const OptionDetails* FindOption(std::string_view Argument) {
  for (auto& Arg : ArgToOption) {
    if (Arg.Arg == Argument) {
      return &Arg;
    }
  }

  return nullptr;
}

void PrintHelp() {
  const char* Arguments[3] {};
  Arguments[0] = "man";
  Arguments[1] = "FEX";
  Arguments[2] = nullptr;

  execvp("man", (char* const*)Arguments);
}

void ExitWithError(std::string_view Error) {
  fextl::fmt::print("Error: {}\n", Error);
  _exit(1);
}

class ArgParser final {
public:
  ArgParser(FEX::ArgLoader::ArgLoader* Loader)
    : Loader {Loader} {}

  std::pair<fextl::vector<fextl::string>, fextl::vector<const char*>> Parse(int argc, char** argv);
  void Version(std::string_view version) {
    _Version = version;
  }

private:
  FEX::ArgLoader::ArgLoader* Loader;
  std::string_view _Version {};
  using ParseArgHandler = void (ArgParser::*)(std::string_view Arg, std::string_view SecondArg, const OptionDetails* Details);

  void ParseArgument_String(std::string_view Arg, std::string_view SecondArg, const OptionDetails* Details) {
    switch (Details->Option) {
#define STR_CONVERT_PARSE
#include <FEXCore/Config/ConfigOptions.inl>
    [[likely]] default:
      Loader->SetArg(Details->Option, SecondArg);
      break;
    }
  }

  void ParseArgument_BoolTrue(std::string_view Arg, std::string_view SecondArg, const OptionDetails* Details) {
    Loader->SetArg(Details->Option, "1");
  }

  void ParseArgument_BoolFalse(std::string_view Arg, std::string_view SecondArg, const OptionDetails* Details) {
    Loader->SetArg(Details->Option, "0");
  }

  void ParseArgument_StringEnum(std::string_view Arg, std::string_view SecondArg, const OptionDetails* Details) {
    switch (Details->Option) {
#define STR_ENUM_PARSE
#include <FEXCore/Config/ConfigOptions.inl>
    [[unlikely]] default:
      ExitWithError(fextl::fmt::format("Unknown strenum argument: {}", Arg));
      break;
    }
  }

  constexpr static std::array<ParseArgHandler, FEXCore::ToUnderlying(ValueType::Max)> Parser = {
    &ArgParser::ParseArgument_String,
    &ArgParser::ParseArgument_BoolTrue,
    &ArgParser::ParseArgument_BoolFalse,
    &ArgParser::ParseArgument_StringEnum,
    // Behaves the same as string but appends multiple.
    &ArgParser::ParseArgument_String,
  };

  void ParseArgument(std::string_view Arg, std::string_view SecondArg, const OptionDetails* Details) {
    (this->*Parser[FEXCore::ToUnderlying(Details->ParseType)])(Arg, SecondArg, Details);
  }
};

static bool NeedsArg(const OptionDetails* Details) {
  return Details->ParseType == ValueType::String || Details->ParseType == ValueType::StringEnum || Details->ParseType == ValueType::StringArray;
}

std::pair<fextl::vector<fextl::string>, fextl::vector<const char*>> ArgParser::Parse(int argc, char** argv) {
  fextl::vector<fextl::string> RemainingArgs {};
  fextl::vector<const char*> ProgramArguments {};

  // Skip argv[0]
  int ArgParsed = 1;
  for (; ArgParsed < argc; ++ArgParsed) {
    std::string_view Arg = argv[ArgParsed];

    // Special case version and help
    if (Arg == "--version") [[unlikely]] {
      fextl::fmt::print("{}\n", _Version);
      std::exit(0);
    }

    if (Arg == "-h" || Arg == "--help") [[unlikely]] {
      PrintHelp();
      std::exit(0);
    }

    if (Arg == "--") {
      // Special case break. Remaining arguments get passed to guest.
      ++ArgParsed;
      break;
    }

    const bool IsShort = Arg.find("--", 0, 2) == Arg.npos && Arg.find("-", 0, 1) == 0;
    const bool IsLong = Arg.find("--", 0, 2) == 0;

    std::string_view ArgFirst {};
    std::string_view ArgSecond {};

    const OptionDetails* OptionDetails {};

    if (IsShort) {
      ArgFirst = Arg;

      OptionDetails = FindOption(ArgFirst);

      if (OptionDetails == nullptr) [[unlikely]] {
        ExitWithError(fextl::fmt::format("Unsupported argument: {}\nUse --help for more information.", Arg));
      }

      if (NeedsArg(OptionDetails)) {
        ++ArgParsed;
        ArgSecond = argv[ArgParsed];
      }
    } else if (IsLong) {
      const auto Split = Arg.find_first_of('=');
      bool NeedsSplitArg {};
      if (Split == Arg.npos) {
        ArgFirst = Arg;
        OptionDetails = FindOption(Arg);

        if (OptionDetails == nullptr) [[unlikely]] {
          ExitWithError(fextl::fmt::format("Unsupported argument: {}\nUse --help for more information.", Arg));
        }

        NeedsSplitArg = NeedsArg(OptionDetails);
      } else {
        ArgFirst = Arg.substr(0, Split);
        OptionDetails = FindOption(ArgFirst);

        if (OptionDetails == nullptr) [[unlikely]] {
          ExitWithError(fextl::fmt::format("Unsupported argument: {}\nUse --help for more information.", Arg));
        }

        NeedsSplitArg = NeedsArg(OptionDetails);
      }

      if (NeedsSplitArg && Split == Arg.npos) [[unlikely]] {
        ExitWithError(fextl::fmt::format("{} needs argument", Arg));
      }

      if (!NeedsSplitArg && Split != Arg.npos) [[unlikely]] {
        ExitWithError(fextl::fmt::format("{} can't have argument", Arg));
      }

      if (NeedsSplitArg) {
        ArgSecond = Arg.substr(Split + 1, Arg.size());

        if (ArgSecond.empty()) [[unlikely]] {
          ExitWithError(fextl::fmt::format("{} needs argument", Arg));
        }
      }
    }

    if (ProgramArguments.empty() && OptionDetails == nullptr) {
      // In the special case that we hit a parse error and we haven't parsed any arguments, pass everything.
      // This handles the typical case eg: `FEXLoader /usr/bin/ls /`.
      // Some would claim that `--` should be used to split FEX arguments from sub-application arguments.
      break;
    }

    // Unsupported FEX argument. Error.
    if (ProgramArguments.empty() && OptionDetails == nullptr) [[unlikely]] {
      ExitWithError(fextl::fmt::format("Unsupported argument: {}\nUse --help for more information.", Arg));
    }

    // Save the FEX argument.
    ProgramArguments.emplace_back(argv[ArgParsed]);

    // Now parse the argument.
    ParseArgument(Arg, ArgSecond, OptionDetails);
  }

  // Pass any remaining arguments to guest application
  for (; ArgParsed < argc; ++ArgParsed) {
    RemainingArgs.emplace_back(argv[ArgParsed]);
  }

  return std::make_pair(std::move(RemainingArgs), std::move(ProgramArguments));
}

void FEX::ArgLoader::ArgLoader::Load() {
  if (Type == LoadType::WITHOUT_FEXLOADER_PARSER) {
    return;
  }

  RemainingArgs.clear();
  ProgramArguments.clear();
  ArgParser Parser(this);
  Parser.Version("FEX-Emu (" GIT_DESCRIBE_STRING ") ");
  std::tie(RemainingArgs, ProgramArguments) = Parser.Parse(argc, argv);
}

void FEX::ArgLoader::ArgLoader::SetArg(FEXCore::Config::ConfigOption Option, std::string_view Arg) {
  Set(Option, Arg);
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
