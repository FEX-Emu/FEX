#include "IRLoader/Loader.h"
#include "LogManager.h"

#include <fstream>

namespace {
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
  };
}

}

namespace FEX::IRLoader {
  std::unordered_map<std::string_view, FEXCore::IR::IROps> NameToOpMap;

  template<>
  std::pair<DecodeFailure, uint8_t> Loader::DecodeValue(std::string &Arg) {
    if (Arg.at(0) != '#') return {DecodeFailure::DECODE_INVALIDCHAR, 0};

    uint8_t Result = strtoul(&Arg.at(1), nullptr, 0);
    if (errno == ERANGE) return {DecodeFailure::DECODE_INVALIDRANGE, 0};
    return {DecodeFailure::DECODE_OKAY, Result};
	}

  template<>
  std::pair<DecodeFailure, uint16_t> Loader::DecodeValue(std::string &Arg) {
    if (Arg.at(0) != '#') return {DecodeFailure::DECODE_INVALIDCHAR, 0};

    uint16_t Result = strtoul(&Arg.at(1), nullptr, 0);
    if (errno == ERANGE) return {DecodeFailure::DECODE_INVALIDRANGE, 0};
    return {DecodeFailure::DECODE_OKAY, Result};
  }

  template<>
  std::pair<DecodeFailure, uint32_t> Loader::DecodeValue(std::string &Arg) {
    if (Arg.at(0) != '#') return {DecodeFailure::DECODE_INVALIDCHAR, 0};

    uint32_t Result = strtoul(&Arg.at(1), nullptr, 0);
    if (errno == ERANGE) return {DecodeFailure::DECODE_INVALIDRANGE, 0};
    return {DecodeFailure::DECODE_OKAY, Result};
  }

  template<>
  std::pair<DecodeFailure, uint64_t> Loader::DecodeValue(std::string &Arg) {
    if (Arg.at(0) != '#') return {DecodeFailure::DECODE_INVALIDCHAR, 0};

    uint64_t Result = strtoull(&Arg.at(1), nullptr, 0);
    if (errno == ERANGE) return {DecodeFailure::DECODE_INVALIDRANGE, 0};
    return {DecodeFailure::DECODE_OKAY, Result};
  }

  template<>
  std::pair<DecodeFailure, FEXCore::IR::RegisterClassType> Loader::DecodeValue(std::string &Arg) {
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
  std::pair<DecodeFailure, FEXCore::IR::TypeDefinition> Loader::DecodeValue(std::string &Arg) {
    uint8_t Size{}, Elements{1};
    int NumArgs = sscanf(Arg.c_str(), "i%hhdv%hhd", &Size, &Elements);

    if (NumArgs != 1 && NumArgs != 2) {
      return {DecodeFailure::DECODE_INVALID, {}};
    }

    return {DecodeFailure::DECODE_OKAY, FEXCore::IR::TypeDefinition::Create(Size / 8, Elements)};
  }

  template<>
  std::pair<DecodeFailure, FEXCore::IR::CondClassType> Loader::DecodeValue(std::string &Arg) {
    std::array<std::string, 14> CondNames = {
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
    };

    for (size_t i = 0; i < CondNames.size(); ++i) {
      if (CondNames[i] == Arg) {
        return {DecodeFailure::DECODE_OKAY, CondClassType{static_cast<uint8_t>(i)}};
      }
    }
    return {DecodeFailure::DECODE_INVALID_CONDFLAG, {}};
  }

  template<>
  std::pair<DecodeFailure, FEXCore::IR::MemOffsetType> Loader::DecodeValue(std::string &Arg) {
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
  std::pair<DecodeFailure, OrderedNode*> Loader::DecodeValue(std::string &Arg) {
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

  Loader::Loader(std::string const &Filename, std::string const &ConfigFilename) {
    Config.Init(ConfigFilename);
    std::fstream fp(Filename, std::fstream::binary | std::fstream::in);

    if (!fp.is_open()) {
      LogMan::Msg::E("Couldn't open IR file '%s'", Filename.c_str());
      return;
    }

    std::string TmpLine;
    while (!fp.eof()) {
      std::getline(fp, TmpLine);
			if (fp.eof()) {
				break;
			}
      if (fp.fail()) {
        LogMan::Msg::E("Failed to getline on line: %ld", Lines.size());
        return;
      }
      Lines.emplace_back(TmpLine);
    }

		ResetWorkingList();
    Loaded = Parse();
  }

	bool Loader::Parse() {
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

      auto Entry = DecodeValue<uint64_t>(Def.Args[0]);
      auto CodeBlockCount = DecodeValue<uint64_t>(Def.Args[2]);

      if (!CheckPrintError(Def, Entry.first)) return false;
      if (!CheckPrintError(Def, CodeBlockCount.first)) return false;

      EntryRIP = Entry.second;
      IRHeader = _IRHeader(InvalidNode, Entry.second, CodeBlockCount.second, false);
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
      }
    }
    SetWriteCursor(nullptr); // isolate the block headers too

    // Spin through all the definitions and add the ops to the basic blocks
    OrderedNode *CurrentBlock{};
    FEXCore::IR::IROp_CodeBlock *CurrentBlockOp{};
    for(size_t i = 1; i < Defs.size(); ++i) {
      auto &Def = Defs[i];
			CurrentDef = &Def;

      if (Def.OpEnum == FEXCore::IR::IROps::OP_CODEBLOCK) {
        auto DefTarget = SSANameMapper.find(Def.Definition);
        if (CurrentBlock != nullptr) {
          LogMan::Msg::E("Error on Line: %d", Def.LineNumber);
          LogMan::Msg::E("%s", Lines[Def.LineNumber].c_str());
          LogMan::Msg::E("CodeBlock being used inside of already existing codeblock!");
          return false;
        }

        CurrentBlock = DefTarget->second;
        CurrentBlockOp = CurrentBlock->Op(Data.Begin())->CW<FEXCore::IR::IROp_CodeBlock>();
      }

      if (Def.OpEnum == FEXCore::IR::IROps::OP_ENDBLOCK) {
        if (CurrentBlock == nullptr) {
          LogMan::Msg::E("Error on Line: %d", Def.LineNumber);
          LogMan::Msg::E("%s", Lines[Def.LineNumber].c_str());
          LogMan::Msg::E("EndBlock being used outside of a block!");
          return false;
        }

        auto Adjust = DecodeValue<uint64_t>(Def.Args[0]);

        if (!CheckPrintError(Def, Adjust.first)) return false;

        Def.Node = _EndBlock(CurrentBlock);
        CurrentBlockOp->Last = Def.Node->Wrapped(ListData.Begin());

        CurrentBlock = nullptr;
        CurrentBlockOp = nullptr;
      }

      if (Def.OpEnum == FEXCore::IR::IROps::OP_DUMMY) {
        auto &PrevDef = Defs[i - 1];
        if (PrevDef.OpEnum != FEXCore::IR::IROps::OP_CODEBLOCK) {
          LogMan::Msg::E("Error on Line: %d", Def.LineNumber);
          LogMan::Msg::E("%s", Lines[Def.LineNumber].c_str());
          LogMan::Msg::E("Dummy op must be first op in block");
          return false;
        }

        Def.Node = _Dummy();
        CurrentBlockOp->Begin = Def.Node->Wrapped(ListData.Begin());
        SSANameMapper[Def.Definition] = Def.Node;
      }

      switch (Def.OpEnum) {
        // Special handled
        case FEXCore::IR::IROps::OP_IRHEADER:
        case FEXCore::IR::IROps::OP_CODEBLOCK:
        case FEXCore::IR::IROps::OP_ENDBLOCK:
        case FEXCore::IR::IROps::OP_DUMMY:
          break;
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

    std::stringstream out;
    auto NewIR = ViewIR();
    FEXCore::IR::Dump(&out, &NewIR, nullptr);
    printf("IR:\n%s\n@@@@@\n", out.str().c_str());

		return true;
	}

  void InitializeStaticTables() {
    for (FEXCore::IR::IROps Op = FEXCore::IR::IROps::OP_DUMMY;
         Op <= FEXCore::IR::IROps::OP_LAST;
         Op = static_cast<FEXCore::IR::IROps>(static_cast<uint32_t>(Op) + 1)) {
      NameToOpMap[FEXCore::IR::GetName(Op)] = Op;
    }
  }
}
