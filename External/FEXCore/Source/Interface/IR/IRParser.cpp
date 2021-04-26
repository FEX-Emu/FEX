/*
$info$
meta: ir|parser ~ Text -> IR
tags: ir|parser
$end_info$
*/

#include <string>
#include <vector>
#include <istream>
#include <unordered_map>

#include <FEXCore/IR/IR.h>
#include <FEXCore/IR/IntrusiveIRList.h>
#include <FEXCore/IR/IREmitter.h>



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
};

std::string ltrim(std::string String) {
	size_t pos = std::string::npos;
	if ((pos = String.find_first_not_of(" \t\n\r")) != std::string::npos) {
		String.erase(0, pos);
	}

	return String;
}

std::string rtrim(std::string String) {
	size_t pos = std::string::npos;
	if ((pos = String.find_last_not_of(" \t\n\r")) != std::string::npos) {
		String.erase(String.begin() + pos + 1, String.end());
	}

	return String;
}

std::string trim(std::string String) {
  return rtrim(ltrim(String));
}

std::string DecodeErrorToString(DecodeFailure Failure) {
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
  };
}

std::unordered_map<std::string_view, FEXCore::IR::IROps> NameToOpMap;

class IRParser: public FEXCore::IR::IREmitter {
  public:
  template<typename Type>
  std::pair<DecodeFailure, Type> DecodeValue(std::string &Arg) {
    return {DecodeFailure::DECODE_UNKNOWN_TYPE, {}};
  }


  template<>
  std::pair<DecodeFailure, uint8_t> DecodeValue(std::string &Arg) {
    if (Arg.at(0) != '#') return {DecodeFailure::DECODE_INVALIDCHAR, 0};

    uint8_t Result = strtoul(&Arg.at(1), nullptr, 0);
    if (errno == ERANGE) return {DecodeFailure::DECODE_INVALIDRANGE, 0};
    return {DecodeFailure::DECODE_OKAY, Result};
	}

  template<>
  std::pair<DecodeFailure, bool> DecodeValue(std::string &Arg) {
    if (Arg.at(0) != '#') return {DecodeFailure::DECODE_INVALIDCHAR, 0};

    uint8_t Result = strtoul(&Arg.at(1), nullptr, 0);
    if (errno == ERANGE || Result > 1) return {DecodeFailure::DECODE_INVALIDRANGE, 0};
    return {DecodeFailure::DECODE_OKAY, Result != 0};
  }

  template<>
  std::pair<DecodeFailure, uint16_t> DecodeValue(std::string &Arg) {
    if (Arg.at(0) != '#') return {DecodeFailure::DECODE_INVALIDCHAR, 0};

    uint16_t Result = strtoul(&Arg.at(1), nullptr, 0);
    if (errno == ERANGE) return {DecodeFailure::DECODE_INVALIDRANGE, 0};
    return {DecodeFailure::DECODE_OKAY, Result};
  }

  template<>
  std::pair<DecodeFailure, uint32_t> DecodeValue(std::string &Arg) {
    if (Arg.at(0) != '#') return {DecodeFailure::DECODE_INVALIDCHAR, 0};

    uint32_t Result = strtoul(&Arg.at(1), nullptr, 0);
    if (errno == ERANGE) return {DecodeFailure::DECODE_INVALIDRANGE, 0};
    return {DecodeFailure::DECODE_OKAY, Result};
  }

  template<>
  std::pair<DecodeFailure, uint64_t> DecodeValue(std::string &Arg) {
    if (Arg.at(0) != '#') return {DecodeFailure::DECODE_INVALIDCHAR, 0};

    uint64_t Result = strtoull(&Arg.at(1), nullptr, 0);
    if (errno == ERANGE) return {DecodeFailure::DECODE_INVALIDRANGE, 0};
    return {DecodeFailure::DECODE_OKAY, Result};
  }

  template<>
  std::pair<DecodeFailure, int64_t> DecodeValue(std::string &Arg) {
    if (Arg.at(0) != '#') return {DecodeFailure::DECODE_INVALIDCHAR, 0};

    int64_t Result = (int64_t)strtoull(&Arg.at(1), nullptr, 0);
    if (errno == ERANGE) return {DecodeFailure::DECODE_INVALIDRANGE, 0};
    return {DecodeFailure::DECODE_OKAY, Result};
  }

  template<>
  std::pair<DecodeFailure, IR::SHA256Sum> DecodeValue(std::string &Arg) {
    IR::SHA256Sum Result;

    if (Arg.at(0) != 's' || Arg.at(1) != 'h' || Arg.at(2) != 'a' || Arg.at(3) != '2' || Arg.at(4) != '5' || Arg.at(5) != '6' || Arg.at(6) != ':')
      return {DecodeFailure::DECODE_INVALIDCHAR, Result};

    auto GetDigit = [](const std::string &Arg, int pos, uint8_t *val) {
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
  std::pair<DecodeFailure, FEXCore::IR::RegisterClassType> DecodeValue(std::string &Arg) {
    if (Arg == "GPR") {
      return {DecodeFailure::DECODE_OKAY, FEXCore::IR::GPRClass};
    }
    else if (Arg == "FPR") {
      return {DecodeFailure::DECODE_OKAY, FEXCore::IR::FPRClass};
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
  std::pair<DecodeFailure, FEXCore::IR::TypeDefinition> DecodeValue(std::string &Arg) {
    uint8_t Size{}, Elements{1};
    int NumArgs = sscanf(Arg.c_str(), "i%hhdv%hhd", &Size, &Elements);

    if (NumArgs != 1 && NumArgs != 2) {
      return {DecodeFailure::DECODE_INVALID, {}};
    }

    return {DecodeFailure::DECODE_OKAY, FEXCore::IR::TypeDefinition::Create(Size / 8, Elements)};
  }

  template<>
  std::pair<DecodeFailure, FEXCore::IR::CondClassType> DecodeValue(std::string &Arg) {
    std::array<std::string, 22> CondNames = {
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
      "Invalid Cond",
      "Invalid Cond",
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
  std::pair<DecodeFailure, FEXCore::IR::MemOffsetType> DecodeValue(std::string &Arg) {
    std::array<std::string, 3> Names = {
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
  std::pair<DecodeFailure, FEXCore::IR::FenceType> DecodeValue(std::string &Arg) {
    std::array<std::string, 3> Names = {
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
  std::pair<DecodeFailure, OrderedNode*> DecodeValue(std::string &Arg) {
    if (Arg.at(0) != '%') return {DecodeFailure::DECODE_INVALIDCHAR, 0};

    // Strip off the type qualifier from the ssa value
    size_t ArgEnd = std::string::npos;
    std::string SSAName = trim(Arg);
    ArgEnd = SSAName.find_first_of(" ");

    if (ArgEnd != std::string::npos) {
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
    std::string Definition{};
    FEXCore::IR::TypeDefinition Size{};
    std::string IROp{};
    FEXCore::IR::IROps OpEnum;
    bool HasArgs{};
    std::vector<std::string> Args;
    OrderedNode *Node{};
  };

  std::vector<std::string> Lines;
  std::unordered_map<std::string, OrderedNode*> SSANameMapper;
  std::vector<LineDefinition> Defs;
  LineDefinition *CurrentDef{};

  IRParser(std::istream *text) {
    InitializeStaticTables();
    
    std::string TmpLine;
    while (!text->eof()) {
      std::getline(*text, TmpLine);
			if (text->eof()) {
				break;
			}
      if (text->fail()) {
        LogMan::Msg::E("Failed to getline on line: %ld", Lines.size());
        return;
      }
      Lines.emplace_back(TmpLine);
    }

		ResetWorkingList();
    Loaded = Parse();
  }

  bool Loaded = false;

#define IROP_PARSER_ALLOCATE_HELPERS
#include <FEXCore/IR/IRDefines.inc>


  bool Parse() {
    auto CheckPrintError = [&](LineDefinition &Def, DecodeFailure Failure) -> bool {
      if (Failure != DecodeFailure::DECODE_OKAY) {
        LogMan::Msg::E("Error on Line: %d", Def.LineNumber);
        LogMan::Msg::E("%s", Lines[Def.LineNumber].c_str());
        LogMan::Msg::E("Value Couldn't be decoded due to %s", DecodeErrorToString(Failure).c_str());
        return false;
      }

      return true;
    };

    // String parse every line for our definitions
		for (size_t i = 0; i < Lines.size(); ++i) {
			std::string Line = Lines[i];
      LineDefinition Def{};
			CurrentDef = &Def;
      Def.LineNumber = i;

			Line = trim(Line);

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
        size_t DefinitionEnd = std::string::npos;
        if ((DefinitionEnd = Line.find_first_of("=", CurrentPos)) != std::string::npos) {
          Def.Definition = Line.substr(0, DefinitionEnd);
          Def.Definition = trim(Def.Definition);
          Def.HasDefinition = true;
          CurrentPos = DefinitionEnd + 1; // +1 to ensure we go past then assignment
        }
        else {
          LogMan::Msg::E("Error on Line: %d", i);
          LogMan::Msg::E("%s", Lines[i].c_str());
          LogMan::Msg::E("SSA declaration without assignment");
          return false;
        }
			}

      // Check if we are pulling in some IR from the IR Printer
      // Prints (%ssa%d) at the start of lines without a definition
      if (Line[0] == '(') {
        size_t DefinitionEnd = std::string::npos;
        if ((DefinitionEnd = Line.find_first_of(")", CurrentPos)) != std::string::npos) {
          size_t SSAEnd = std::string::npos;
          if ((SSAEnd = Line.find_last_of(" ", DefinitionEnd)) != std::string::npos) {
            std::string Type = Line.substr(SSAEnd + 1, DefinitionEnd - SSAEnd - 1);
            Type = trim(Type);

            auto DefinitionSize = DecodeValue<FEXCore::IR::TypeDefinition>(Type);
            if (!CheckPrintError(Def, DefinitionSize.first)) return false;
            Def.Size = DefinitionSize.second;
          }

          Def.Definition = trim(Line.substr(1, std::min(DefinitionEnd, SSAEnd) - 1));

          CurrentPos = DefinitionEnd + 1;
        }
        else {
          LogMan::Msg::E("Error on Line: %d", i);
          LogMan::Msg::E("%s", Lines[i].c_str());
          LogMan::Msg::E("SSA value with numbered SSA provided but no closing parentheses");
          return false;
        }
      }

      if (Def.HasDefinition) {
        // Let's check if we have a size declared with this variable
        size_t NameEnd = std::string::npos;
        if ((NameEnd = Def.Definition.find_first_of(" ")) != std::string::npos) {
          std::string Type = Def.Definition.substr(NameEnd + 1);
          Type = trim(Type);
          Def.Definition = trim(Def.Definition.substr(0, NameEnd));

          auto DefinitionSize = DecodeValue<FEXCore::IR::TypeDefinition>(Type);
          if (!CheckPrintError(Def, DefinitionSize.first)) return false;
          Def.Size = DefinitionSize.second;
        }

        if (Def.Definition == "%Invalid") {
          LogMan::Msg::E("Error on Line: %d", i);
          LogMan::Msg::E("%s", Lines[i].c_str());
          LogMan::Msg::E("Definition tried to define reserved %Invalid ssa node");
          return false;
        }
      }

      // Let's get the IR op
      size_t OpNameEnd = std::string::npos;
      std::string RemainingLine = trim(Line.substr(CurrentPos));
      CurrentPos = 0;
      if ((OpNameEnd = RemainingLine.find_first_of(" \t\n\r\0", CurrentPos)) != std::string::npos) {
        Def.IROp = RemainingLine.substr(CurrentPos, OpNameEnd);
        Def.IROp = trim(Def.IROp);
        Def.HasArgs = true;
        CurrentPos = OpNameEnd;
      }
      else {
        if (RemainingLine.empty()) {
          LogMan::Msg::E("Error on Line: %d", i);
          LogMan::Msg::E("%s", Lines[i].c_str());
          LogMan::Msg::E("Line without an IROp?");
          return false;
        }

        Def.IROp = RemainingLine;
        Def.HasArgs = false;
      }

      if (Def.HasArgs) {
        RemainingLine = trim(RemainingLine.substr(CurrentPos));
        CurrentPos = 0;
        if (RemainingLine.empty()) {
          // How did we get here?
          Def.HasArgs = false;
        }
        else {
          while (!RemainingLine.empty()) {
            size_t ArgEnd = std::string::npos;
            ArgEnd = RemainingLine.find_first_of(",");

            std::string Arg = RemainingLine.substr(0, ArgEnd);
            Arg = trim(Arg);
            Def.Args.emplace_back(Arg);

            RemainingLine.erase(0, ArgEnd+1); // +1 to ensure we go past the ','
            if (ArgEnd == std::string::npos)
              break;
          }
        }
      }

      Defs.emplace_back(Def);
		}

    // Ensure all of the ops are real ops
    for(size_t i = 0; i < Defs.size(); ++i) {
      auto &Def = Defs[i];
      auto Op = NameToOpMap.find(Def.IROp);
      if (Op == NameToOpMap.end()) {
        LogMan::Msg::E("Error on Line: %d", Def.LineNumber);
        LogMan::Msg::E("%s", Lines[Def.LineNumber].c_str());
        LogMan::Msg::E("IROp '%s' doesn't exist", Def.IROp.c_str());
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
        LogMan::Msg::E("Error on Line: %d", Def.LineNumber);
        LogMan::Msg::E("%s", Lines[Def.LineNumber].c_str());
        LogMan::Msg::E("First op needs to be IRHeader. Was '%s'", Def.IROp.c_str());
        return false;
      }

      auto CodeBlockCount = DecodeValue<uint64_t>(Def.Args[1]);

      if (!CheckPrintError(Def, CodeBlockCount.first)) return false;

      IRHeader = _IRHeader(InvalidNode, CodeBlockCount.second);
    }

    SetWriteCursor(nullptr); // isolate the header from everything following

    // Initialize SSANameMapper with Invalid value
    SSANameMapper["%Invalid"] = Invalid();

    // Spin through the blocks and generate basic block ops
    for(size_t i = 0; i < Defs.size(); ++i) {
      auto &Def = Defs[i];
      if (Def.OpEnum == FEXCore::IR::IROps::OP_CODEBLOCK) {
        auto CodeBlock = _CodeBlock(InvalidNode, InvalidNode);
        SSANameMapper[Def.Definition] = CodeBlock.Node;
        Def.Node = CodeBlock.Node;

        if (i == 1) {
          // First code block is the entry block
          // Link the header to the first block
          IRHeader.first->Blocks = CodeBlock.Node->Wrapped(ListData.Begin());
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
          LogMan::Msg::E("Error on Line: %d", Def.LineNumber);
          LogMan::Msg::E("%s", Lines[Def.LineNumber].c_str());
          LogMan::Msg::E("IRHEADER used in the middle of the block!");
          return false; // only one OP_IRHEADER allowed per block

        case FEXCore::IR::IROps::OP_CODEBLOCK: {
          SetWriteCursor(nullptr); // isolate from previous block
          if (CurrentBlock != nullptr) {
            LogMan::Msg::E("Error on Line: %d", Def.LineNumber);
            LogMan::Msg::E("%s", Lines[Def.LineNumber].c_str());
            LogMan::Msg::E("CodeBlock being used inside of already existing codeblock!");
            return false;
          }

          CurrentBlock = Def.Node;
          CurrentBlockOp = CurrentBlock->Op(Data.Begin())->CW<FEXCore::IR::IROp_CodeBlock>();
          break;
        }


        case FEXCore::IR::IROps::OP_BEGINBLOCK: {
          if (CurrentBlock == nullptr) {
            LogMan::Msg::E("Error on Line: %d", Def.LineNumber);
            LogMan::Msg::E("%s", Lines[Def.LineNumber].c_str());
            LogMan::Msg::E("EndBlock being used outside of a block!");
            return false;
          }

          auto Adjust = DecodeValue<OrderedNode*>(Def.Args[0]);

          if (!CheckPrintError(Def, Adjust.first)) return false;

          Def.Node = _BeginBlock(Adjust.second);
          CurrentBlockOp->Begin = Def.Node->Wrapped(ListData.Begin());
          break;
        }

        case FEXCore::IR::IROps::OP_ENDBLOCK: {
          if (CurrentBlock == nullptr) {
            LogMan::Msg::E("Error on Line: %d", Def.LineNumber);
            LogMan::Msg::E("%s", Lines[Def.LineNumber].c_str());
            LogMan::Msg::E("EndBlock being used outside of a block!");
            return false;
          }

          auto Adjust = DecodeValue<OrderedNode*>(Def.Args[0]);

          if (!CheckPrintError(Def, Adjust.first)) return false;

          Def.Node = _EndBlock(Adjust.second);
          CurrentBlockOp->Last = Def.Node->Wrapped(ListData.Begin());

          CurrentBlock = nullptr;
          CurrentBlockOp = nullptr;

          break;
        }

        case FEXCore::IR::IROps::OP_DUMMY: {
          LogMan::Msg::E("Error on Line: %d", Def.LineNumber);
          LogMan::Msg::E("%s", Lines[Def.LineNumber].c_str());
          LogMan::Msg::E("Dummy op must not be used");

          break;
        }
#define IROP_PARSER_SWITCH_HELPERS
#include <FEXCore/IR/IRDefines.inc>
        default: {
          LogMan::Msg::E("Error on Line: %d", Def.LineNumber);
          LogMan::Msg::E("%s", Lines[Def.LineNumber].c_str());
          LogMan::Msg::E("Unhandled Op enum '%s' in parser", Def.IROp.c_str());
          return false;
          break;
        }
      }

      if (Def.HasDefinition) {
        auto IROp = Def.Node->Op(Data.Begin());
        if (Def.Size.Elements()) {
          IROp->Size = Def.Size.Bytes() * Def.Size.Elements();
          IROp->ElementSize = Def.Size.Bytes();
        }
        else {
          IROp->Size = Def.Size.Bytes();
          IROp->ElementSize = 0;
        }
        SSANameMapper[Def.Definition] = Def.Node;
      }
    }

		return true;
	}

  void InitializeStaticTables() {
    if (NameToOpMap.size() == 0) {
      for (FEXCore::IR::IROps Op = FEXCore::IR::IROps::OP_DUMMY;
         Op <= FEXCore::IR::IROps::OP_LAST;
         Op = static_cast<FEXCore::IR::IROps>(static_cast<uint32_t>(Op) + 1)) {
        NameToOpMap[FEXCore::IR::GetName(Op)] = Op;
      }
    }
  }
};

} // anon namespace

IREmitter* Parse(std::istream *in) {
    auto parser = new IRParser(in);

    if (parser->Loaded) {
      return parser;
    } else {
      delete parser;
      return nullptr;
    }
}

}
