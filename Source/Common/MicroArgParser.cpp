#include "Common/MicroArgParser.h"

#include <FEXCore/fextl/fmt.h>
#include <algorithm>
#include <charconv>
#include <ranges>

namespace FEX {
[[noreturn]]
inline void ExitWithError(std::string_view Error) {
  fextl::fmt::print(stderr, "Error: {}\n", Error);
  std::exit(1);
}

template<typename T>
T MicroArgParser::Value::Get(std::string_view Default) const {
  auto ParseValue = HasValue ? _Value.front() : Default;

  if constexpr (std::is_same_v<T, bool>) {
    using namespace std::literals;
    return ParseValue == "0"sv ? false : true;
  } else if constexpr (std::is_same_v<T, std::string_view>) {
    return ParseValue;
  } else if constexpr (std::is_integral_v<T>) {
    T Result {};
    auto Results = std::from_chars(ParseValue.begin(), ParseValue.end(), Result, 10);
    if (Results.ec == std::errc()) {
      return Result;
    }
    ExitWithError(fextl::fmt::format("Failed to convert '{}' to integer\n", ParseValue));
  } else if constexpr (std::is_same_v<T, std::string_view>) {
    return ParseValue;
  } else {
    ExitWithError("Unknown type");
  }
}

template bool MicroArgParser::Value::Get<bool>(std::string_view Default) const;
template uint32_t MicroArgParser::Value::Get<uint32_t>(std::string_view Default) const;
template std::string_view MicroArgParser::Value::Get<std::string_view>(std::string_view Default) const;

void MicroArgParser::PrintVersion() {
  fextl::fmt::print("{}\n", _Version);
  std::exit(0);
}

void MicroArgParser::PrintHelp(char** argv, std::span<ParseMember> ParseList) {
  fextl::fmt::print("Usage: {} [options]\n", argv[0]);

  if (!_Version.empty()) {
    fextl::fmt::print("{}\n", _Version);
  }

  if (!_Desc.empty()) {
    fextl::fmt::print("\n{}\n\n", _Desc);
  }

  fextl::fmt::print("Options:\n");
  for (auto& Arg : ParseList) {
    fextl::string ShortArg;
    fextl::string LongArg;
    auto CombineType = [Arg](auto& ArgName, auto Type, auto Delim) {
      if (Type == MicroArgParser::ParseMember::Type::String) {
        return fextl::fmt::format("{}{}VALUE", ArgName, Delim);
      } else if (Type == MicroArgParser::ParseMember::Type::Int) {
        return fextl::fmt::format("{}{}N", ArgName, Delim);
      } else if (Type == MicroArgParser::ParseMember::Type::IntOptional) {
        return fextl::fmt::format("{}{}[N]", ArgName, Delim);
      } else if (Type == MicroArgParser::ParseMember::Type::Choices) {
        return fextl::fmt::format("{}{}[{}]", ArgName, Delim, Arg.Choices);
      }

      return fextl::string(ArgName);
    };

    if (!Arg.Short.empty()) {
      ShortArg = CombineType(Arg.Short, Arg.Type, " ");
    }

    if (!Arg.Long.empty()) {
      LongArg = CombineType(Arg.Long, Arg.Type, "=");
    }

    const bool Combined = !Arg.Short.empty() && !Arg.Long.empty();

    fextl::string Args;
    if (Combined) {
      Args = fextl::fmt::format("{}, {}", ShortArg, LongArg);
    } else {
      Args = Arg.Short.empty() ? LongArg : ShortArg;
    }

    fextl::fmt::print("  {:<24} {}\n", Args, Arg.Help);
  }

  std::exit(0);
}

MicroArgParser::ParseMember* MicroArgParser::FindMember(bool Short, std::string_view Arg) const {
  for (auto& ParseArg : ParseList) {
    if (Short) {
      if (!ParseArg.Short.empty() && ParseArg.Short == Arg) {
        return &ParseArg;
      }
    } else {
      if (!ParseArg.Long.empty() && ParseArg.Long == Arg) {
        return &ParseArg;
      }
    }
  }
  return nullptr;
}

template<typename T>
T MicroArgParser::Get(std::string_view Arg) const {
  auto Member = FindMember(true, Arg);
  if (Member) {
    return Member->Value.Get<T>(Member->Default);
  }

  // Try again with long argument
  Member = FindMember(false, Arg);
  if (Member) {
    return Member->Value.Get<T>(Member->Default);
  }

  ExitWithError(fextl::fmt::format("Unknown argument: {}", Arg));
}

template bool MicroArgParser::Get<bool>(std::string_view Arg) const;
template uint32_t MicroArgParser::Get<uint32_t>(std::string_view Arg) const;
template std::string_view MicroArgParser::Get<std::string_view>(std::string_view Arg) const;

const fextl::vector<std::string_view>& MicroArgParser::GetVector(std::string_view Arg) const {
  auto Member = FindMember(true, Arg);
  if (Member) {
    return Member->Value.GetVector();
  }

  // Try again with long argument
  Member = FindMember(false, Arg);
  if (Member) {
    return Member->Value.GetVector();
  }

  ExitWithError(fextl::fmt::format("Unknown argument: {}", Arg));
}

std::optional<std::string_view> MicroArgParser::TryParseOptionalArg(int Index, int argc, char** argv) {
  if (Index >= argc) {
    return std::nullopt;
  }

  auto NextArg = std::string_view(argv[Index]);

  const bool NextIsShort = NextArg.find("--", 0, 2) == NextArg.npos && NextArg.find("-", 0, 1) == 0;
  const bool NextIsLong = NextArg.find("--", 0, 2) == 0;

  if (!NextIsShort && !NextIsLong) {
    // If the next argument is not an argument then use it.
    return NextArg;
  }

  return std::nullopt;
}

void ValidateArgInChoices(MicroArgParser::ParseMember* Member, std::string_view Arg) {
  using namespace std::literals;
  // Ideally we could use `std::ranges::split_view` to walk the Choices list.
  // Clang-15 doesn't work with the libstdc++ implementation, Would need to bump minimum spec to 16.
  // Reimplement by walking the string_view manually.
  auto Begin = Member->Choices.begin();
  auto ArgEnd = Begin;
  auto End = Member->Choices.end();
  const auto Delim = ","sv;
  for (; ArgEnd != End && Begin != End; Begin = ArgEnd + 1) {
    // Find the end of the choice.
    ArgEnd = std::find_first_of(Begin, End, Delim.begin(), Delim.end());

    if (Begin != ArgEnd) {
      const auto View = std::string_view(Begin, ArgEnd - Begin);
      if (View == Arg) {
        return;
      }
    }
  }

  ExitWithError(fextl::fmt::format("'{}' Not in list of choices '{}'\n", Arg, Member->Choices));
}

void MicroArgParser::Parse(int argc, char** argv) {
  for (RemainingArgumentsIndex = 1; RemainingArgumentsIndex < argc; ++RemainingArgumentsIndex) {
    auto Arg = std::string_view(argv[RemainingArgumentsIndex]);

    // Special case version and help.
    if (Arg == "-v" || Arg == "--version") {
      PrintVersion();
    }

    if (Arg == "-h" || Arg == "--help") {
      PrintHelp(argv, ParseList);
    }

    const bool IsShort = Arg.find("--", 0, 2) == Arg.npos && Arg.find("-", 0, 1) == 0;
    const bool IsLong = Arg.find("--", 0, 2) == 0;

    std::string_view ArgFirst {};
    std::string_view ArgSecond {};
    MicroArgParser::ParseMember* ParseMember {};

    if (IsShort) {
      ArgFirst = Arg;
      ParseMember = FindMember(IsShort, ArgFirst);

      if (!ParseMember) {
        ExitWithError(fextl::fmt::format("Unknown argument: {}\n", Arg));
      }

      if (NeedsArg(ParseMember)) {
        // Always needs an argument.
        if ((RemainingArgumentsIndex + 1) >= argc) {
          ExitWithError(fextl::fmt::format("Argument needs valid input: {}\n", Arg));
        }

        ++RemainingArgumentsIndex;
        ArgSecond = argv[RemainingArgumentsIndex];
      } else if (NeedsOptionalArg(ParseMember)) {
        auto NextIndex = RemainingArgumentsIndex + 1;
        auto NextArg = TryParseOptionalArg(NextIndex, argc, argv);
        if (NextArg.has_value()) {
          ArgSecond = *NextArg;
          ++RemainingArgumentsIndex;
        }
      }
    } else if (IsLong) {
      const auto Split = Arg.find_first_of('=');
      bool NeedsSplitArg {};
      if (Split == Arg.npos) {
        ArgFirst = Arg;
      } else {
        ArgFirst = Arg.substr(0, Split);
        NeedsSplitArg = true;
        ArgSecond = Arg.substr(Split + 1, Arg.size());
      }

      ParseMember = FindMember(!IsLong, ArgFirst);

      if (!ParseMember) {
        ExitWithError(fextl::fmt::format("Unknown argument: {}\n", Arg));
      }

      if (NeedsArg(ParseMember) && !NeedsSplitArg) {
        // Long arguments always need an = split if they need an argument.
        ExitWithError(fextl::fmt::format("Argument not provided to: {}\n", Arg));
      }
    } else {
      // Let any remaining arguments pass through.
      break;
    }

    switch (ParseMember->Type) {
    case ParseMember::Type::String:
    case ParseMember::Type::Int: {
      ParseMember->Value.Set(ArgSecond);
      break;
    }
    case ParseMember::Type::StringArray: {
      ParseMember->Value.Append(ArgSecond);
      break;
    }
    case ParseMember::Type::Choices: {
      ValidateArgInChoices(ParseMember, ArgSecond);
      ParseMember->Value.Set(ArgSecond);
      break;
    }
    case ParseMember::Type::Bool: {
      using namespace std::literals;
      // Bool always set to true.
      ParseMember->Value.Set("1"sv);
      break;
    }
    case ParseMember::Type::BoolInvert: {
      using namespace std::literals;
      // BoolInvert always set to false.
      ParseMember->Value.Set("0"sv);
      break;
    }
    case ParseMember::Type::IntOptional: {
      if (ArgSecond.empty()) {
        // If the argument is empty then it gets set to default.
        ParseMember->Value.Set(ParseMember->Default);
      } else {
        ParseMember->Value.Set(ArgSecond);
      }
      break;
    }

    default: ExitWithError("Unknown parsemember type\n");
    }
  }
}

} // namespace FEX
