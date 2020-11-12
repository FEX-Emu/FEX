#pragma once
#include <FEXCore/IR/IR.h>
#include <FEXCore/IR/IntrusiveIRList.h>
#include <FEXCore/IR/IREmitter.h>

#include "HarnessHelpers.h"

#include "LogManager.h"

#include <string>
#include <vector>

using namespace FEXCore::IR;

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
};

}

namespace FEX::IRLoader {
  class Loader final : public FEXCore::IR::IREmitter {
    public:
      Loader(std::string const &Filename, std::string const &ConfigFilename);

			bool IsValid() const { return Loaded; }
      uint64_t GetEntryRIP() const { return EntryRIP; }

      bool CompareStates(FEXCore::Core::CPUState const* State) {
        return Config.CompareStates(State, nullptr);
      }

      void LoadMemory(uint64_t MemoryBase, FEXCore::CodeLoader::MemoryWriter Writer) {
        Config.LoadMemory(MemoryBase, Writer);
      }

      void MapRegions(std::function<void*(uint64_t, uint64_t, bool, bool)> Mapper) {
        for (auto& [region, size] : Config.GetMemoryRegions()) {
          Mapper(region, size, true, true);
        }
      }

#define IROP_PARSER_ALLOCATE_HELPERS
#include <FEXCore/IR/IRDefines.inc>
    private:
			bool Parse();

      uint64_t EntryRIP{};
      std::vector<std::string> Lines;
      bool Loaded{};

			std::unordered_map<std::string, OrderedNode*> SSANameMapper;

      template<typename Type>
      std::pair<DecodeFailure, Type> DecodeValue(std::string &Arg) {
        return {DecodeFailure::DECODE_UNKNOWN_TYPE, {}};
      }

      template<>
      std::pair<DecodeFailure, uint8_t>
      DecodeValue(std::string &Arg);

      template<>
      std::pair<DecodeFailure, uint16_t>
      DecodeValue(std::string &Arg);

      template<>
      std::pair<DecodeFailure, uint32_t>
      DecodeValue(std::string &Arg);

      template<>
      std::pair<DecodeFailure, uint64_t>
      DecodeValue(std::string &Arg);

      template<>
      std::pair<DecodeFailure, FEXCore::IR::RegisterClassType>
      DecodeValue(std::string &Arg);

      template<>
      std::pair<DecodeFailure, FEXCore::IR::TypeDefinition>
      DecodeValue(std::string &Arg);

      template<>
      std::pair<DecodeFailure, FEXCore::IR::CondClassType>
      DecodeValue(std::string &Arg);

      template<>
      std::pair<DecodeFailure, FEXCore::IR::MemOffsetType>
      DecodeValue(std::string &Arg);

      template<>
      std::pair<DecodeFailure, OrderedNode*>
      DecodeValue(std::string &Arg);

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

      std::vector<LineDefinition> Defs;
			LineDefinition *CurrentDef{};
      FEX::HarnessHelper::ConfigLoader Config;
  };

  void InitializeStaticTables();
}
