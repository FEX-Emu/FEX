#pragma once
#include <FEXCore/IR/IR.h>
#include <FEXCore/IR/IntrusiveIRList.h>

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
};

}

namespace FEX::IRLoader {
  class Loader final {
    public:
      Loader(std::string const &Filename);

      IRListView<false> ViewIR() { return IRListView<false>(&Data, &ListData); }
			IRListView<true> *CreateIRCopy() { return new IRListView<true>(&Data, &ListData); }

			bool IsValid() const { return Loaded; }
      uint64_t GetEntryRIP() const { return EntryRIP; }

#define IROP_ALLOCATE_HELPERS
#define IROP_PARSER_ALLOCATE_HELPERS
#define IROP_DISPATCH_HELPERS
#include <FEXCore/IR/IRDefines.inc>
    private:
			bool Parse();

      uint64_t EntryRIP{};
      std::vector<std::string> Lines;
      bool Loaded{};

			// IR definition requirements
			void ResetWorkingList();

			OrderedNode *CreateNode(IROp_Header *Op) {
				uintptr_t ListBegin = ListData.Begin();
				size_t Size = sizeof(OrderedNode);
				void *Ptr = ListData.Allocate(Size);
				OrderedNode *Node = new (Ptr) OrderedNode();
				Node->Header.Value.SetOffset(Data.Begin(), reinterpret_cast<uintptr_t>(Op));

				if (CurrentWriteCursor) {
					CurrentWriteCursor->append(ListBegin, Node);
				}
				CurrentWriteCursor = Node;
				return Node;
			}

			OrderedNode *GetNode(uint32_t SSANode) {
				uintptr_t ListBegin = ListData.Begin();
				OrderedNode *Node = reinterpret_cast<OrderedNode *>(ListBegin + SSANode * sizeof(OrderedNode));
				return Node;
			}

			void SetWriteCursor(OrderedNode *Node) {
				CurrentWriteCursor = Node;
			}

			OrderedNode *GetWriteCursor() {
				return CurrentWriteCursor;
			}

			IntrusiveAllocator Data;
			IntrusiveAllocator ListData;

			OrderedNode *InvalidNode;
			OrderedNode *CurrentCodeBlock{};
			OrderedNode *CurrentWriteCursor = nullptr;

			std::vector<OrderedNode*> CodeBlocks;
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
  };

  void InitializeStaticTables();
}
