// SPDX-License-Identifier: MIT
#pragma once

#include <optional>
#include <string_view>
#include <span>
#include "FEXCore/fextl/vector.h"

namespace FEX {
class MicroArgParser final {
public:
  class Value final {
  public:
    Value() = default;
    constexpr void Set(std::string_view value) {
      HasValue = true;
      _Value.clear();
      _Value.emplace_back(value);
    }

    constexpr void Append(std::string_view value) {
      HasValue = true;
      _Value.emplace_back(value);
    }

    template<typename T>
    T Get(std::string_view Default) const;


    const fextl::vector<std::string_view>& GetVector() const {
      return _Value;
    }

  private:
    fextl::vector<std::string_view> _Value;
    bool HasValue {};
  };

  struct ParseMember final {
    enum class Type {
      Bool,
      BoolInvert,
      String,
      Int,
      IntOptional,
      StringArray,
      Choices,
    };

    Type Type {};
    std::string_view Short {};
    std::string_view Long {};
    std::string_view Help {};
    std::string_view Default {};
    std::string_view Choices {};

    Value Value {};
  };

  MicroArgParser(std::span<ParseMember> ParseList)
    : ParseList {ParseList} {}

  void Version(std::string_view version) {
    _Version = version;
  }

  void Desc(std::string_view desc) {
    _Desc = desc;
  }

  void Parse(int argc, char** argv);

  template<typename T>
  T Get(std::string_view Arg) const;

  const fextl::vector<std::string_view>& GetVector(std::string_view Arg) const;

  int GetRemainingArgumentsIndex() const {
    return RemainingArgumentsIndex;
  }

private:
  std::span<ParseMember> ParseList;
  std::string_view _Version;
  std::string_view _Desc;
  int RemainingArgumentsIndex {};

  void PrintVersion();
  void PrintHelp(char** argv, std::span<ParseMember> ParseList);
  ParseMember* FindMember(bool Short, std::string_view Arg) const;

  bool NeedsArg(const ParseMember* Member) const {
    return Member->Type == ParseMember::Type::Int || Member->Type == ParseMember::Type::String || Member->Type == ParseMember::Type::StringArray;
  }

  bool NeedsOptionalArg(const ParseMember* Member) const {
    return Member->Type == ParseMember::Type::IntOptional;
  }

  std::optional<std::string_view> TryParseOptionalArg(int Index, int argc, char** argv);
};

} // namespace FEX
