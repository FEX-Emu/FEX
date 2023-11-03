// SPDX-License-Identifier: MIT
/*
$info$
meta: ir|parser ~ Text -> IR
tags: ir|parser
$end_info$
*/

#include <FEXCore/IR/IR.h>
#include <FEXCore/IR/IntrusiveIRList.h>
#include <FEXCore/IR/IREmitter.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/StringUtils.h>
#include <FEXCore/fextl/sstream.h>
#include <FEXCore/fextl/string.h>
#include <FEXCore/fextl/unordered_map.h>
#include <FEXCore/fextl/vector.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <errno.h>
#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <string_view>
#include <utility>
#include <istream>
#include <unordered_map>

namespace FEXCore::IR {
namespace {

enum class DecodeFailure {
  DECODE_OKAY,
  DECODE_UNKNOWN_TYPE,
  DECODE_INVALID,
  DECODE_INVALIDCHAR,
  DECODE_INVALIDRANGE,
  DECODE_INVALIDREGISTERCLASS,
  DECODE_UNKNOWN_SSA,
  DECODE_INVALID_CONDFLAG,
  DECODE_INVALID_MEMOFFSETTYPE,
  DECODE_INVALID_FENCETYPE,
  DECODE_INVALID_BREAKTYPE,
  DECODE_INVALID_OPSIZE,
};

fextl::string DecodeErrorToString(DecodeFailure Failure) {
  switch (Failure) {
    case DecodeFailure::DECODE_OKAY: return "Okay";
    case DecodeFailure::DECODE_UNKNOWN_TYPE: return "Unknown Type";
    case DecodeFailure::DECODE_INVALID: return "Invalid";
    case DecodeFailure::DECODE_INVALIDCHAR: return "Invalid starting char";
    case DecodeFailure::DECODE_INVALIDRANGE: return "Invalid integer range";
    case DecodeFailure::DECODE_INVALIDREGISTERCLASS: return "Invalid register class";
    case DecodeFailure::DECODE_UNKNOWN_SSA: return "Unknown SSA value";
    case DecodeFailure::DECODE_INVALID_CONDFLAG: return "Invalid Conditional name";
    case DecodeFailure::DECODE_INVALID_MEMOFFSETTYPE: return "Invalid Memory Offset Type";
    case DecodeFailure::DECODE_INVALID_FENCETYPE: return "Invalid Fence Type";
    case DecodeFailure::DECODE_INVALID_BREAKTYPE: return "Invalid Break Reason Type";
    case DecodeFailure::DECODE_INVALID_OPSIZE: return "Invalid Operation size name";
  }
  return "Unknown Error";
}

class IRParser: public FEXCore::IR::IREmitter {
  public:
  template<typename Type>
  std::pair<DecodeFailure, Type> DecodeValue(const fextl::string &Arg) {
    return {DecodeFailure::DECODE_UNKNOWN_TYPE, {}};
  }

  template<>
  std::pair<DecodeFailure, uint8_t> DecodeValue(const fextl::string &Arg) {
    if (Arg.at(0) != '#') return {DecodeFailure::DECODE_INVALIDCHAR, 0};

    uint8_t Result = strtoul(&Arg.at(1), nullptr, 0);
    if (errno == ERANGE) return {DecodeFailure::DECODE_INVALIDRANGE, 0};
    return {DecodeFailure::DECODE_OKAY, Result};
  }

  template<>
  std::pair<DecodeFailure, bool> DecodeValue(const fextl::string &Arg) {
    if (Arg.at(0) != '#') return {DecodeFailure::DECODE_INVALIDCHAR, 0};

    uint8_t Result = strtoul(&Arg.at(1), nullptr, 0);
    if (errno == ERANGE || Result > 1) return {DecodeFailure::DECODE_INVALIDRANGE, 0};
    return {DecodeFailure::DECODE_OKAY, Result != 0};
  }

  template<>
  std::pair<DecodeFailure, uint16_t> DecodeValue(const fextl::string &Arg) {
    if (Arg.at(0) != '#') return {DecodeFailure::DECODE_INVALIDCHAR, 0};

    uint16_t Result = strtoul(&Arg.at(1), nullptr, 0);
    if (errno == ERANGE) return {DecodeFailure::DECODE_INVALIDRANGE, 0};
    return {DecodeFailure::DECODE_OKAY, Result};
  }

  template<>
  std::pair<DecodeFailure, uint32_t> DecodeValue(const fextl::string &Arg) {
    if (Arg.at(0) != '#') return {DecodeFailure::DECODE_INVALIDCHAR, 0};

    uint32_t Result = strtoul(&Arg.at(1), nullptr, 0);
    if (errno == ERANGE) return {DecodeFailure::DECODE_INVALIDRANGE, 0};
    return {DecodeFailure::DECODE_OKAY, Result};
  }

  template<>
  std::pair<DecodeFailure, uint64_t> DecodeValue(const fextl::string &Arg) {
    if (Arg.at(0) != '#') return {DecodeFailure::DECODE_INVALIDCHAR, 0};

    uint64_t Result = strtoull(&Arg.at(1), nullptr, 0);
    if (errno == ERANGE) return {DecodeFailure::DECODE_INVALIDRANGE, 0};
    return {DecodeFailure::DECODE_OKAY, Result};
  }

  template<>
  std::pair<DecodeFailure, int64_t> DecodeValue(const fextl::string &Arg) {
    if (Arg.at(0) != '#') return {DecodeFailure::DECODE_INVALIDCHAR, 0};

    int64_t Result = (int64_t)strtoull(&Arg.at(1), nullptr, 0);
    if (errno == ERANGE) return {DecodeFailure::DECODE_INVALIDRANGE, 0};
    return {DecodeFailure::DECODE_OKAY, Result};
  }

  template<>
  std::pair<DecodeFailure, IR::SHA256Sum> DecodeValue(const fextl::string &Arg) {
    IR::SHA256Sum Result;

    if (Arg.at(0) != 's' || Arg.at(1) != 'h' || Arg.at(2) != 'a' || Arg.at(3) != '2' || Arg.at(4) != '5' || Arg.at(5) != '6' || Arg.at(6) != ':')
      return {DecodeFailure::DECODE_INVALIDCHAR, Result};

    auto GetDigit = [](const fextl::string &Arg, int pos, uint8_t *val) {
      auto chr = Arg.at(pos);
      if (chr >= '0' && chr <= '9') {
        *val = chr - '0';
        return true;
      } else if (chr >= 'a' && chr <= 'f') {
        *val = 10 + chr - 'a';
        return true;
      } else {
        return false;
      }
    };

    for (size_t i = 0; i < sizeof(Result.data); i++) {
      uint8_t high, low;
      if (!GetDigit(Arg, 7 + 2 * i + 0, &high) || !GetDigit(Arg, 7 + 2 * i + 1, &low)) {
        return {DecodeFailure::DECODE_INVALIDRANGE, Result};
      }
      Result.data[i] = high * 16 + low;
    }

    return {DecodeFailure::DECODE_OKAY, Result};
  }

  template<>
  std::pair<DecodeFailure, FEXCore::IR::RegisterClassType> DecodeValue(const fextl::string &Arg) {
    if (Arg == "GPR") {
      return {DecodeFailure::DECODE_OKAY, FEXCore::IR::GPRClass};
    }
    else if (Arg == "FPR") {
      return {DecodeFailure::DECODE_OKAY, FEXCore::IR::FPRClass};
    }
    else if (Arg == "GPRFixed") {
      return {DecodeFailure::DECODE_OKAY, FEXCore::IR::GPRFixedClass};
    }
    else if (Arg == "FPRFixed") {
      return {DecodeFailure::DECODE_OKAY, FEXCore::IR::FPRFixedClass};
    }
    else if (Arg == "GPRPair") {
      return {DecodeFailure::DECODE_OKAY, FEXCore::IR::GPRPairClass};
    }
    else if (Arg == "Complex") {
      return {DecodeFailure::DECODE_OKAY, FEXCore::IR::ComplexClass};
    }

    return {DecodeFailure::DECODE_INVALIDREGISTERCLASS, FEXCore::IR::InvalidClass};
  }

  template<>
  std::pair<DecodeFailure, FEXCore::IR::TypeDefinition> DecodeValue(const fextl::string &Arg) {
    uint8_t Size{}, Elements{1};
    int NumArgs = sscanf(Arg.c_str(), "i%hhdv%hhd", &Size, &Elements);

    if (NumArgs != 1 && NumArgs != 2) {
      return {DecodeFailure::DECODE_INVALID, {}};
    }

    return {DecodeFailure::DECODE_OKAY, FEXCore::IR::TypeDefinition::Create(Size / 8, Elements)};
  }

  template<>
  std::pair<DecodeFailure, FEXCore::IR::CondClassType> DecodeValue(const fextl::string &Arg) {
    static constexpr std::array<std::string_view, 22> CondNames = {
      "EQ",
      "NEQ",
      "UGE",
      "ULT",
      "MI",
      "PL",
      "VS",
      "VC",
      "UGT",
      "ULE",
      "SGE",
      "SLT",
      "SGT",
      "SLE",
      "ANDZ",
      "ANDNZ",
      "FLU",
      "FGE",
      "FLEU",
      "FGT",
      "FU",
      "FNU"
    };

    for (size_t i = 0; i < CondNames.size(); ++i) {
      if (CondNames[i] == Arg) {
        return {DecodeFailure::DECODE_OKAY, CondClassType{static_cast<uint8_t>(i)}};
      }
    }
    return {DecodeFailure::DECODE_INVALID_CONDFLAG, {}};
  }

  template<>
  std::pair<DecodeFailure, FEXCore::IR::MemOffsetType> DecodeValue(const fextl::string &Arg) {
    static constexpr std::array<std::string_view, 3> Names = {
      "SXTX",
      "UXTW",
      "SXTW",
    };

    for (size_t i = 0; i < Names.size(); ++i) {
      if (Names[i] == Arg) {
        return {DecodeFailure::DECODE_OKAY, MemOffsetType{static_cast<uint8_t>(i)}};
      }
    }
    return {DecodeFailure::DECODE_INVALID_MEMOFFSETTYPE, {}};
  }

  template<>
  std::pair<DecodeFailure, FEXCore::IR::FenceType> DecodeValue(const fextl::string &Arg) {
    static constexpr std::array<std::string_view, 3> Names = {
      "Loads",
      "Stores",
      "LoadStores",
    };

    for (size_t i = 0; i < Names.size(); ++i) {
      if (Names[i] == Arg) {
        return {DecodeFailure::DECODE_OKAY, FenceType{static_cast<uint8_t>(i)}};
      }
    }
    return {DecodeFailure::DECODE_INVALID_FENCETYPE, {}};
  }

  template<>
  std::pair<DecodeFailure, FEXCore::IR::BreakDefinition> DecodeValue(const fextl::string &Arg) {
    uint32_t tmp{};
    fextl::stringstream ss{Arg};
    BreakDefinition Reason{};

    // Seek past '{'
    ss.seekg(1, std::ios::cur);
    ss >> Reason.ErrorRegister;

    // Seek past '.'
    ss.seekg(1, std::ios::cur);
    ss >> tmp;
    Reason.Signal = tmp;

    // Seek past '.'
    ss.seekg(1, std::ios::cur);
    ss >> tmp;
    Reason.TrapNumber = tmp;

    // Seek past '.'
    ss.seekg(1, std::ios::cur);
    ss >> tmp;
    Reason.si_code = tmp;

    if (ss.fail()) {
      return {DecodeFailure::DECODE_INVALIDCHAR, {}};
    }
    else {
      return {DecodeFailure::DECODE_OKAY, Reason};
    }
  }

  template<>
  std::pair<DecodeFailure, FEXCore::IR::OpSize> DecodeValue(const fextl::string &Arg) {
    static constexpr std::array<std::pair<std::string_view, FEXCore::IR::OpSize>, 6> Names = {{
      { "i8", OpSize::i8Bit },
      { "i16", OpSize::i16Bit },
      { "i32", OpSize::i32Bit },
      { "i64", OpSize::i64Bit },
      { "i128", OpSize::i128Bit },
      { "i256", OpSize::i256Bit },
    }};

    for (size_t i = 0; i < Names.size(); ++i) {
      if (Names[i].first == Arg) {
        return {DecodeFailure::DECODE_OKAY, Names[i].second};
      }
    }
    return {DecodeFailure::DECODE_INVALID_OPSIZE, {}};
  }

  template<>
  std::pair<DecodeFailure, OrderedNode*> DecodeValue(const fextl::string &Arg) {
    if (Arg.at(0) != '%') return {DecodeFailure::DECODE_INVALIDCHAR, 0};

    // Strip off the type qualifier from the ssa value
    fextl::string SSAName = FEXCore::StringUtils::Trim(Arg);
    const size_t ArgEnd = SSAName.find_first_of(' ');

    if (ArgEnd != fextl::string::npos) {
      SSAName = SSAName.substr(0, ArgEnd);
    }

    // Forward declarations may make this not succed
    auto Op = SSANameMapper.find(SSAName);
    if (Op == SSANameMapper.end()) {
      return {DecodeFailure::DECODE_UNKNOWN_SSA, nullptr};
    }

    return {DecodeFailure::DECODE_OKAY, Op->second};
  }

  struct LineDefinition {
    size_t LineNumber;
    bool HasDefinition{};
    fextl::string Definition{};
    FEXCore::IR::TypeDefinition Size{};
    fextl::string IROp{};
    FEXCore::IR::IROps OpEnum;
    bool HasArgs{};
    fextl::vector<fextl::string> Args;
    OrderedNode *Node{};
  };

  fextl::vector<fextl::string> Lines;
  fextl::unordered_map<fextl::string, OrderedNode*> SSANameMapper;
  fextl::vector<LineDefinition> Defs;
  LineDefinition *CurrentDef{};
  fextl::unordered_map<std::string_view, FEXCore::IR::IROps> NameToOpMap;

  IRParser(FEXCore::Utils::IntrusivePooledAllocator &ThreadAllocator, fextl::stringstream &MapsStream)
    : IREmitter {ThreadAllocator} {
    InitializeNameMap();

    fextl::string Line;
    while (std::getline(MapsStream, Line)) {
      if (MapsStream.eof()) break;
      if (MapsStream.fail()) {
        LogMan::Msg::EFmt("Failed to getline on line: {}", Lines.size());
        return;
      }
      Lines.emplace_back(Line);
    }

    ResetWorkingList();
    Loaded = Parse();
  }

  bool Loaded = false;

  bool Parse() {
    const auto CheckPrintError = [&](const LineDefinition &Def, DecodeFailure Failure) -> bool {
      if (Failure != DecodeFailure::DECODE_OKAY) {
        LogMan::Msg::EFmt("Error on Line: {}", Def.LineNumber);
        LogMan::Msg::EFmt("{}", Lines[Def.LineNumber]);
        LogMan::Msg::EFmt("Value Couldn't be decoded due to {}", DecodeErrorToString(Failure));
        return false;
      }

      return true;
    };

    const auto CheckPrintErrorArg = [&](const LineDefinition &Def, DecodeFailure Failure, size_t Arg) -> bool {
      if (Failure != DecodeFailure::DECODE_OKAY) {
        LogMan::Msg::EFmt("Error on Line: {}", Def.LineNumber);
        LogMan::Msg::EFmt("{}", Lines[Def.LineNumber]);
        LogMan::Msg::EFmt("Argument Number {}: {}", Arg + 1, Def.Args[Arg]);
        LogMan::Msg::EFmt("Value Couldn't be decoded due to {}", DecodeErrorToString(Failure));
        return false;
      }

      return true;
    };

    // String parse every line for our definitions
    for (size_t i = 0; i < Lines.size(); ++i) {
      fextl::string Line = Lines[i];
      LineDefinition Def{};
      CurrentDef = &Def;
      Def.LineNumber = i;

      Line = FEXCore::StringUtils::Trim(Line);

      // Skip empty lines
      if (Line.empty()) {
        continue;
      }

      if (Line[0] == ';') {
        // This is a comment line
        // Skip it
        continue;
      }

      size_t CurrentPos{};
      // Let's see if this node is assigning something first
      if (Line[0] == '%') {
        size_t DefinitionEnd = fextl::string::npos;
        if ((DefinitionEnd = Line.find_first_of('=', CurrentPos)) != fextl::string::npos) {
          Def.Definition = Line.substr(0, DefinitionEnd);
          Def.Definition = FEXCore::StringUtils::Trim(Def.Definition);
          Def.HasDefinition = true;
          CurrentPos = DefinitionEnd + 1; // +1 to ensure we go past then assignment
        }
        else {
          LogMan::Msg::EFmt("Error on Line: {}", i);
          LogMan::Msg::EFmt("{}", Lines[i]);
          LogMan::Msg::EFmt("SSA declaration without assignment");
          return false;
        }
      }

      // Check if we are pulling in some IR from the IR Printer
      // Prints (%%d) at the start of lines without a definition
      if (Line[0] == '(') {
        size_t DefinitionEnd = fextl::string::npos;
        if ((DefinitionEnd = Line.find_first_of(')', CurrentPos)) != fextl::string::npos) {
          size_t SSAEnd = fextl::string::npos;
          if ((SSAEnd = Line.find_last_of(' ', DefinitionEnd)) != fextl::string::npos) {
            fextl::string Type = Line.substr(SSAEnd + 1, DefinitionEnd - SSAEnd - 1);
            Type = FEXCore::StringUtils::Trim(Type);

            auto DefinitionSize = DecodeValue<FEXCore::IR::TypeDefinition>(Type);
            if (!CheckPrintError(Def, DefinitionSize.first)) {
              return false;
            }
            Def.Size = DefinitionSize.second;
          }

          Def.Definition = FEXCore::StringUtils::Trim(Line.substr(1, std::min(DefinitionEnd, SSAEnd) - 1));

          CurrentPos = DefinitionEnd + 1;
        }
        else {
          LogMan::Msg::EFmt("Error on Line: {}", i);
          LogMan::Msg::EFmt("{}", Lines[i]);
          LogMan::Msg::EFmt("SSA value with numbered SSA provided but no closing parentheses");
          return false;
        }
      }

      if (Def.HasDefinition) {
        // Let's check if we have a size declared with this variable
        size_t NameEnd = fextl::string::npos;
        if ((NameEnd = Def.Definition.find_first_of(' ')) != fextl::string::npos) {
          fextl::string Type = Def.Definition.substr(NameEnd + 1);
          Type = FEXCore::StringUtils::Trim(Type);
          Def.Definition = FEXCore::StringUtils::Trim(Def.Definition.substr(0, NameEnd));

          auto DefinitionSize = DecodeValue<FEXCore::IR::TypeDefinition>(Type);
          if (!CheckPrintError(Def, DefinitionSize.first)) return false;
          Def.Size = DefinitionSize.second;
        }

        if (Def.Definition == "%Invalid") {
          LogMan::Msg::EFmt("Error on Line: {}", i);
          LogMan::Msg::EFmt("{}", Lines[i]);
          LogMan::Msg::EFmt("Definition tried to define reserved %Invalid ssa node");
          return false;
        }
      }

      // Let's get the IR op
      size_t OpNameEnd = fextl::string::npos;
      fextl::string RemainingLine = FEXCore::StringUtils::Trim(Line.substr(CurrentPos));
      CurrentPos = 0;
      if ((OpNameEnd = RemainingLine.find_first_of(" \t\n\r\0", CurrentPos)) != fextl::string::npos) {
        Def.IROp = RemainingLine.substr(CurrentPos, OpNameEnd);
        Def.IROp = FEXCore::StringUtils::Trim(Def.IROp);
        Def.HasArgs = true;
        CurrentPos = OpNameEnd;
      }
      else {
        if (RemainingLine.empty()) {
          LogMan::Msg::EFmt("Error on Line: {}", i);
          LogMan::Msg::EFmt("{}", Lines[i]);
          LogMan::Msg::EFmt("Line without an IROp?");
          return false;
        }

        Def.IROp = RemainingLine;
        Def.HasArgs = false;
      }

      if (Def.HasArgs) {
        RemainingLine = FEXCore::StringUtils::Trim(RemainingLine.substr(CurrentPos));
        CurrentPos = 0;
        if (RemainingLine.empty()) {
          // How did we get here?
          Def.HasArgs = false;
        }
        else {
          while (!RemainingLine.empty()) {
            const size_t ArgEnd = RemainingLine.find(',');
            fextl::string Arg = FEXCore::StringUtils::Trim(RemainingLine.substr(0, ArgEnd));

            Def.Args.emplace_back(std::move(Arg));

            RemainingLine.erase(0, ArgEnd+1); // +1 to ensure we go past the ','
            if (ArgEnd == fextl::string::npos)
              break;
          }
        }
      }

      CurrentDef = &Defs.emplace_back(std::move(Def));
    }

    // Ensure all of the ops are real ops
    for(size_t i = 0; i < Defs.size(); ++i) {
      auto &Def = Defs[i];
      auto Op = NameToOpMap.find(Def.IROp);
      if (Op == NameToOpMap.end()) {
        LogMan::Msg::EFmt("Error on Line: {}", Def.LineNumber);
        LogMan::Msg::EFmt("{}", Lines[Def.LineNumber]);
        LogMan::Msg::EFmt("IROp '{}' doesn't exist", Def.IROp);
        return false;
      }
      Def.OpEnum = Op->second;
    }

    // Emit the header op
    IRPair<IROp_IRHeader> IRHeader;
    {
      auto &Def = Defs[0];
      CurrentDef = &Def;
      if (Def.OpEnum != FEXCore::IR::IROps::OP_IRHEADER) {
        LogMan::Msg::EFmt("Error on Line: {}", Def.LineNumber);
        LogMan::Msg::EFmt("{}", Lines[Def.LineNumber]);
        LogMan::Msg::EFmt("First op needs to be IRHeader. Was '{}'", Def.IROp);
        return false;
      }

      auto OriginalRIP = DecodeValue<uint64_t>(Def.Args[1]);

      if (!CheckPrintError(Def, OriginalRIP.first)) return false;

      auto CodeBlockCount = DecodeValue<uint64_t>(Def.Args[2]);

      if (!CheckPrintError(Def, CodeBlockCount.first)) return false;

      auto InstructionCount = DecodeValue<uint64_t>(Def.Args[3]);

      if (!CheckPrintError(Def, InstructionCount.first)) return false;

      IRHeader = _IRHeader(InvalidNode, OriginalRIP.second, CodeBlockCount.second, InstructionCount.second);
    }

    SetWriteCursor(nullptr); // isolate the header from everything following

    // Initialize SSANameMapper with Invalid value
    SSANameMapper.insert_or_assign("%Invalid", Invalid());

    // Spin through the blocks and generate basic block ops
    for(size_t i = 0; i < Defs.size(); ++i) {
      auto &Def = Defs[i];
      if (Def.OpEnum == FEXCore::IR::IROps::OP_CODEBLOCK) {
        auto CodeBlock = _CodeBlock(InvalidNode, InvalidNode);
        SSANameMapper.insert_or_assign(Def.Definition, CodeBlock.Node);
        Def.Node = CodeBlock.Node;

        if (i == 1) {
          // First code block is the entry block
          // Link the header to the first block
          IRHeader.first->Blocks = CodeBlock.Node->Wrapped(DualListData.ListBegin());
        }
        CodeBlocks.emplace_back(CodeBlock.Node);
      }
    }
    SetWriteCursor(nullptr); // isolate the block headers too

    // Spin through all the definitions and add the ops to the basic blocks
    OrderedNode *CurrentBlock{};
    FEXCore::IR::IROp_CodeBlock *CurrentBlockOp{};
    for(size_t i = 1; i < Defs.size(); ++i) {
      auto &Def = Defs[i];
      CurrentDef = &Def;

      switch (Def.OpEnum) {
        // Special handled
        case FEXCore::IR::IROps::OP_IRHEADER:
          LogMan::Msg::EFmt("Error on Line: {}", Def.LineNumber);
          LogMan::Msg::EFmt("{}", Lines[Def.LineNumber]);
          LogMan::Msg::EFmt("IRHEADER used in the middle of the block!");
          return false; // only one OP_IRHEADER allowed per block

        case FEXCore::IR::IROps::OP_CODEBLOCK: {
          SetWriteCursor(nullptr); // isolate from previous block
          if (CurrentBlock != nullptr) {
            LogMan::Msg::EFmt("Error on Line: {}", Def.LineNumber);
            LogMan::Msg::EFmt("{}", Lines[Def.LineNumber]);
            LogMan::Msg::EFmt("CodeBlock being used inside of already existing codeblock!");
            return false;
          }

          CurrentBlock = Def.Node;
          CurrentBlockOp = CurrentBlock->Op(DualListData.DataBegin())->CW<FEXCore::IR::IROp_CodeBlock>();
          break;
        }


        case FEXCore::IR::IROps::OP_BEGINBLOCK: {
          if (CurrentBlock == nullptr) {
            LogMan::Msg::EFmt("Error on Line: {}", Def.LineNumber);
            LogMan::Msg::EFmt("{}", Lines[Def.LineNumber]);
            LogMan::Msg::EFmt("EndBlock being used outside of a block!");
            return false;
          }

          auto Adjust = DecodeValue<OrderedNode*>(Def.Args[0]);
          if (!CheckPrintError(Def, Adjust.first)) {
            return false;
          }

          Def.Node = _BeginBlock(Adjust.second);
          CurrentBlockOp->Begin = Def.Node->Wrapped(DualListData.ListBegin());
          break;
        }

        case FEXCore::IR::IROps::OP_ENDBLOCK: {
          if (CurrentBlock == nullptr) {
            LogMan::Msg::EFmt("Error on Line: {}", Def.LineNumber);
            LogMan::Msg::EFmt("{}", Lines[Def.LineNumber]);
            LogMan::Msg::EFmt("EndBlock being used outside of a block!");
            return false;
          }

          auto Adjust = DecodeValue<OrderedNode*>(Def.Args[0]);
          if (!CheckPrintError(Def, Adjust.first)) {
            return false;
          }

          Def.Node = _EndBlock(Adjust.second);
          CurrentBlockOp->Last = Def.Node->Wrapped(DualListData.ListBegin());

          CurrentBlock = nullptr;
          CurrentBlockOp = nullptr;

          break;
        }

        case FEXCore::IR::IROps::OP_DUMMY: {
          LogMan::Msg::EFmt("Error on Line: {}", Def.LineNumber);
          LogMan::Msg::EFmt("{}", Lines[Def.LineNumber]);
          LogMan::Msg::EFmt("Dummy op must not be used");
          break;
        }
#define IROP_PARSER_SWITCH_HELPERS
#include <FEXCore/IR/IRDefines.inc>
        default: {
          LogMan::Msg::EFmt("Error on Line: {}", Def.LineNumber);
          LogMan::Msg::EFmt("{}", Lines[Def.LineNumber]);
          LogMan::Msg::EFmt("Unhandled Op enum '{}' in parser", Def.IROp);
          return false;
        }
      }

      if (Def.HasDefinition) {
        auto IROp = Def.Node->Op(DualListData.DataBegin());
        if (Def.Size.Elements()) {
          IROp->Size = Def.Size.Bytes() * Def.Size.Elements();
          IROp->ElementSize = Def.Size.Bytes();
        }
        else {
          IROp->Size = Def.Size.Bytes();
          IROp->ElementSize = 0;
        }
        SSANameMapper.insert_or_assign(Def.Definition, Def.Node);
      }
    }

		return true;
	}

  void InitializeNameMap() {
    if (NameToOpMap.empty()) {
      for (FEXCore::IR::IROps Op = FEXCore::IR::IROps::OP_DUMMY;
         Op <= FEXCore::IR::IROps::OP_LAST;
         Op = static_cast<FEXCore::IR::IROps>(static_cast<uint32_t>(Op) + 1)) {
        NameToOpMap.insert_or_assign(FEXCore::IR::GetName(Op), Op);
      }
    }
  }
};

} // anon namespace

fextl::unique_ptr<IREmitter> Parse(FEXCore::Utils::IntrusivePooledAllocator &ThreadAllocator, fextl::stringstream &MapsStream) {
    auto parser = fextl::make_unique<IRParser>(ThreadAllocator, MapsStream);

    if (parser->Loaded) {
      return parser;
    } else {
      return nullptr;
    }
}

}
