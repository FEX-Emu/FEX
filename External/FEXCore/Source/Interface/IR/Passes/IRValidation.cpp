#include "Interface/IR/PassManager.h"
#include "Interface/Core/OpcodeDispatcher.h"

#include <iostream>

namespace {
  struct BlockInfo {
    bool HasExit;

    std::vector<FEXCore::IR::OrderedNode const*> Predecessors;
    std::vector<FEXCore::IR::OrderedNode const*> Successors;
  };
}

namespace FEXCore::IR::Validation {

class IRValidation final : public FEXCore::IR::Pass {
public:
  bool Run(OpDispatchBuilder *Disp) override;
};

bool IRValidation::Run(OpDispatchBuilder *Disp) {
  bool HadError = false;
  bool HadWarning = false;

  std::unordered_map<IR::OrderedNodeWrapper::NodeOffsetType, BlockInfo> OffsetToBlockMap;
  auto CurrentIR = Disp->ViewIR();
  uintptr_t ListBegin = CurrentIR.GetListData();
  uintptr_t DataBegin = CurrentIR.GetData();

  std::ostringstream Errors;
  std::ostringstream Warnings;

  auto Begin = CurrentIR.begin();
  auto Op = Begin();

  OrderedNode *RealNode = Op->GetNode(ListBegin);
  auto HeaderOp = RealNode->Op(DataBegin)->CW<FEXCore::IR::IROp_IRHeader>();
  LogMan::Throw::A(HeaderOp->Header.Op == OP_IRHEADER, "First op wasn't IRHeader");

  OrderedNode *BlockNode = HeaderOp->Blocks.GetNode(ListBegin);
  while (1) {
    auto BlockIROp = BlockNode->Op(DataBegin)->CW<FEXCore::IR::IROp_CodeBlock>();
    LogMan::Throw::A(BlockIROp->Header.Op == OP_CODEBLOCK, "IR type failed to be a code block");

    BlockInfo *CurrentBlock = &OffsetToBlockMap.try_emplace(BlockNode->Wrapped(ListBegin).ID()).first->second;

    // We grab these nodes this way so we can iterate easily
    auto CodeBegin = CurrentIR.at(BlockIROp->Begin);
    auto CodeLast = CurrentIR.at(BlockIROp->Last);

    while (1) {
      auto CodeOp = CodeBegin();
      OrderedNode *CodeNode = CodeOp->GetNode(ListBegin);
      auto IROp = CodeNode->Op(DataBegin);

      uint8_t OpSize = IROp->Size;

      if (IROp->HasDest) {
        HadError |= OpSize == 0;
        // Does the op have a destination of size 0?
        if (OpSize == 0) {
          Errors << "%ssa" << CodeOp->ID() << ": Had destination but with no size" << std::endl;
        }

        // Does the node have zero uses? Should have been DCE'd
        if (RealNode->GetUses() == 0) {
          HadWarning |= true;
          Warnings << "%ssa" << CodeOp->ID() << ": Destination created but had no uses" << std::endl;
        }
      }

      uint8_t NumArgs = IR::GetArgs(IROp->Op);
      for (uint32_t i = 0; i < NumArgs; ++i) {
        OrderedNodeWrapper Arg = IROp->Args[i];
        // Was an argument defined after this node?
        if (Arg.ID() >= CodeOp->ID()) {
          HadError |= true;
          Errors << "%ssa" << CodeOp->ID() << ": Arg[" << i << "] has definition after use at %ssa" << Arg.ID() << std::endl;
        }
      }

      switch (IROp->Op) {
        case IR::OP_EXITFUNCTION: {
          CurrentBlock->HasExit = true;
        break;
        }
        case IR::OP_CONDJUMP: {
          auto Op = IROp->C<IR::IROp_CondJump>();

          OrderedNode const *TrueTargetNode = Op->Header.Args[1].GetNode(ListBegin);
          OrderedNode const *FalseTargetNode = Op->Header.Args[2].GetNode(ListBegin);

          CurrentBlock->Successors.emplace_back(TrueTargetNode);
          CurrentBlock->Successors.emplace_back(FalseTargetNode);

          FEXCore::IR::IROp_Header const *TrueTargetOp = TrueTargetNode->Op(DataBegin);
          FEXCore::IR::IROp_Header const *FalseTargetOp = FalseTargetNode->Op(DataBegin);

          if (TrueTargetOp->Op != OP_CODEBLOCK) {
            HadError |= true;
            Errors << "CondJump %ssa" << CodeOp->ID() << ": True Target Jumps to Op that isn't the begining of a block" << std::endl;
          }
          else {
            auto Block = OffsetToBlockMap.try_emplace(Op->Header.Args[1].ID()).first;
            Block->second.Predecessors.emplace_back(BlockNode);
          }

          if (FalseTargetOp->Op != OP_CODEBLOCK) {
            HadError |= true;
            Errors << "CondJump %ssa" << CodeOp->ID() << ": False Target Jumps to Op that isn't the begining of a block" << std::endl;
          }
          else {
            auto Block = OffsetToBlockMap.try_emplace(Op->Header.Args[2].ID()).first;
            Block->second.Predecessors.emplace_back(BlockNode);
          }

          break;
        }
        case IR::OP_JUMP: {
          auto Op = IROp->C<IR::IROp_Jump>();
          OrderedNode const *TargetNode = Op->Header.Args[0].GetNode(ListBegin);
          CurrentBlock->Successors.emplace_back(TargetNode);

          FEXCore::IR::IROp_Header const *TargetOp = TargetNode->Op(DataBegin);
          if (TargetOp->Op != OP_CODEBLOCK) {
            HadError |= true;
            Errors << "Jump %ssa" << CodeOp->ID() << ": Jump to Op that isn't the begining of a block" << std::endl;
          }
          else {
            auto Block = OffsetToBlockMap.try_emplace(Op->Header.Args[0].ID()).first;
            Block->second.Predecessors.emplace_back(BlockNode);
          }
          break;
        }
        default:
          // LogMan::Msg::A("Unknown IR Op: %d(%s)", IROp->Op, FEXCore::IR::GetName(IROp->Op).data());
        break;
      }

      // CodeLast is inclusive. So we still need to dump the CodeLast op as well
      if (CodeBegin == CodeLast) {
        break;
      }
      ++CodeBegin;
    }

    // Blocks can only have zero (Exit), 1 (Unconditional branch) or 2 (Conditional) successors
    size_t NumSuccessors = CurrentBlock->Successors.size();
    if (NumSuccessors > 2) {
      HadError |= true;
      Errors << "%ssa" << BlockNode->Wrapped(ListBegin).ID() << " Has " << NumSuccessors << " successors which is too many" << std::endl;
    }

    {
      auto GetOp = [&ListBegin, &DataBegin](auto Code) {
        auto CodeOp = Code();
        OrderedNode *CodeNode = CodeOp->GetNode(ListBegin);
        auto IROp = CodeNode->Op(DataBegin);
        return IROp->Op;
      };

      auto CodeCurrent = CodeLast;

      // Last instruction in the block must be EndBlock
      {
        auto Op = GetOp(CodeCurrent);
        if (Op != IR::OP_ENDBLOCK) {
          HadError |= true;
          Errors << "%ssa" << BlockNode->Wrapped(ListBegin).ID() << " Failed to end block with EndBlock" << std::endl;
        }
      }

      --CodeCurrent;

      // Blocks need to have an instruction that leaves the block in some way before the EndBlock instruction
      {
        auto Op = GetOp(CodeCurrent);
        switch (Op) {
          case OP_EXITFUNCTION:
          case OP_JUMP:
          case OP_CONDJUMP:
          case OP_BREAK:
            break;
          default:
            HadError |= true;
            Errors << "%ssa" << BlockNode->Wrapped(ListBegin).ID() << " Didn't have an exit IR op as its last instruction" << std::endl;
        };
      }
    }

    if (BlockIROp->Next.ID() == 0) {
      break;
    } else {
      BlockNode = BlockIROp->Next.GetNode(ListBegin);
    }
  }

  std::stringstream Out;

  HadWarning = false;
  if (HadError || HadWarning) {
    FEXCore::IR::Dump(&Out, &CurrentIR);

    if (HadError) {
      Out << "Errors:" << std::endl << Errors.str() << std::endl;
    }

    if (HadWarning) {
      Out << "Warnings:" << std::endl << Warnings.str() << std::endl;
    }

    std::cerr << Out.str() << std::endl;
  }

  return false;
}

FEXCore::IR::Pass* CreateIRValidation() {
  return new IRValidation{};
}
}
