#include "Interface/IR/PassManager.h"
#include "Interface/Core/OpcodeDispatcher.h"

#include <iostream>
#include <map>
#include <list>
#include <unordered_map>

namespace {
  struct BlockInfo {
    std::vector<FEXCore::IR::OrderedNode *> Predecessors;
    std::vector<FEXCore::IR::OrderedNode *> Successors;
  };
}

namespace FEXCore::IR::Validation {
class ValueDominanceValidation final : public FEXCore::IR::Pass {
public:
  bool Run(OpDispatchBuilder *Disp) override;
};

bool ValueDominanceValidation::Run(OpDispatchBuilder *Disp) {
  bool HadError = false;
  bool HadWarning = false;
  auto CurrentIR = Disp->ViewIR();
  uintptr_t ListBegin = CurrentIR.GetListData();
  uintptr_t DataBegin = CurrentIR.GetData();
  std::ostringstream Errors;
  std::ostringstream Warnings;

  std::unordered_map<IR::OrderedNodeWrapper::NodeOffsetType, BlockInfo> OffsetToBlockMap;

  auto Begin = CurrentIR.begin();
  auto Op = Begin();

  OrderedNode *RealNode = Op->GetNode(ListBegin);
  auto HeaderOp = RealNode->Op(DataBegin)->CW<FEXCore::IR::IROp_IRHeader>();
  LogMan::Throw::A(HeaderOp->Header.Op == OP_IRHEADER, "First op wasn't IRHeader");

  // Walk the list and calculate the control flow
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

      switch (IROp->Op) {
        case IR::OP_CONDJUMP: {
          auto Op = IROp->CW<IR::IROp_CondJump>();

          OrderedNode *TrueTargetNode = Op->Header.Args[1].GetNode(ListBegin);
          OrderedNode *FalseTargetNode = Op->Header.Args[2].GetNode(ListBegin);

          CurrentBlock->Successors.emplace_back(TrueTargetNode);
          CurrentBlock->Successors.emplace_back(FalseTargetNode);

          {
            auto Block = &OffsetToBlockMap.try_emplace(Op->Header.Args[1].ID()).first->second;
            Block->Predecessors.emplace_back(BlockNode);
          }

          {
            auto Block = &OffsetToBlockMap.try_emplace(Op->Header.Args[2].ID()).first->second;
            Block->Predecessors.emplace_back(BlockNode);
          }

          break;
        }
        case IR::OP_JUMP: {
          auto Op = IROp->CW<IR::IROp_Jump>();
          OrderedNode *TargetNode = Op->Header.Args[0].GetNode(ListBegin);
          CurrentBlock->Successors.emplace_back(TargetNode);

          {
            auto Block = OffsetToBlockMap.try_emplace(Op->Header.Args[0].ID()).first;
            Block->second.Predecessors.emplace_back(BlockNode);
          }
          break;
        }
        default: break;
      }

      // CodeLast is inclusive. So we still need to dump the CodeLast op as well
      if (CodeBegin == CodeLast) {
        break;
      }
      ++CodeBegin;
    }

    if (BlockIROp->Next.ID() == 0) {
      break;
    } else {
      BlockNode = BlockIROp->Next.GetNode(ListBegin);
    }
  }

  BlockNode = HeaderOp->Blocks.GetNode(ListBegin);
  while (1) {
    auto BlockIROp = BlockNode->Op(DataBegin)->CW<FEXCore::IR::IROp_CodeBlock>();
    LogMan::Throw::A(BlockIROp->Header.Op == OP_CODEBLOCK, "IR type failed to be a code block");

    // We grab these nodes this way so we can iterate easily
    auto CodeBegin = CurrentIR.at(BlockIROp->Begin);
    auto CodeLast = CurrentIR.at(BlockIROp->Last);

    while (1) {
      auto CodeOp = CodeBegin();
      OrderedNode *CodeNode = CodeOp->GetNode(ListBegin);
      auto IROp = CodeNode->Op(DataBegin);

      uint8_t NumArgs = IR::GetArgs(IROp->Op);
      for (uint32_t i = 0; i < NumArgs; ++i) {
        if (IROp->Args[i].IsInvalid()) continue;
        OrderedNodeWrapper Arg = IROp->Args[i];

        // We must ensure domininance of all SSA arguments
        if (Arg.ID() >= BlockIROp->Begin.ID() &&
            Arg.ID() < BlockIROp->Last.ID()) {
          // If the SSA argument is defined INSIDE this block
          // then it must only be declared prior to this instruction
          // Eg: Valid
          // CodeBlock_1:
          // %ssa_1 = Load
          // %ssa_2 = Load
          // %ssa_3 = <Op> %ssa_1, %ssa_2
          //
          // Eg: Invalid
          // CodeBlock_1:
          // %ssa_1 = Load
          // %ssa_2 = <Op> %ssa_1, %ssa_3
          // %ssa_3 = Load
          if (Arg.ID() > CodeOp->ID()) {
            HadError |= true;
            Errors << "Inst %ssa" << CodeOp->ID() << ": Arg[" << i << "] %ssa" << Arg.ID() << " definition does not dominate this use!" << std::endl;
          }
        }
        else if (Arg.ID() < BlockIROp->Begin.ID()) {
          // If the SSA argument is defined BEFORE this block
          // then THIS block needs to be dominated by the flow of blocks up until this point

          // Eg: Valid
          // CodeBlock_1:
          // %ssa_1 = Load
          // %ssa_2 = Load
          // Jump %CodeBlock_2
          //
          // CodeBlock_2:
          // %ssa_3 = <Op> %ssa_1, %ssa_2
          //
          // Eg: Invalid
          // CodeBlock_1:
          // %ssa_1 = Load
          // %ssa_2 = Load
          // Jump %CodeBlock_3
          //
          // CodeBlock_2:
          // %ssa_3 = <Op> %ssa_1, %ssa_2
          //
          // CodeBlock_3:
          // ...

          // We need to walk the predecessors to see if the value comes from there
          std::set<IR::OrderedNode *> Predecessors;

          std::function<void(IR::OrderedNode*)> AddPredecessors = [&] (IR::OrderedNode *Node) {
            auto PredBlock = &OffsetToBlockMap.try_emplace(Node->Wrapped(ListBegin).ID()).first->second;

            // Current Block will always be in set
            // Walk each predecessor, adding their predecessors
            // Leave if all the predecessors are already in the set
            for (auto &Pred : PredBlock->Predecessors) {
              if (Predecessors.insert(Pred).second) {
                // If this block didn't exist then walk its predecessors as well
                AddPredecessors(Pred);
              }
            }
          };

          AddPredecessors(BlockNode);

          bool FoundPredDefine = false;

          for (auto it = Predecessors.begin(); it != Predecessors.end(); ++it) {
            IR::OrderedNode *Pred = *it;
            auto PredIROp = Pred->Op(DataBegin)->CW<FEXCore::IR::IROp_CodeBlock>();

            if (Arg.ID() >= PredIROp->Begin.ID() &&
                Arg.ID() < PredIROp->Last.ID()) {
              FoundPredDefine = true;
              break;
            }
            Errors << "\tChecking Pred %ssa" << Pred->Wrapped(ListBegin).ID() << std::endl;
          }

          if (!FoundPredDefine) {
            HadError |= true;
            Errors << "Inst %ssa" << CodeOp->ID() << ": Arg[" << i << "] %ssa" << Arg.ID() << " definition does not dominate this use! But was defined before this block!" << std::endl;
          }
        }
        else if (Arg.ID() > BlockIROp->Last.ID()) {
          // If this SSA argument is defined AFTER this block then it is just completely broken
          // Eg: Invalid
          // CodeBlock_1:
          // %ssa_1 = Load
          // %ssa_2 = <Op> %ssa_1, %ssa_3
          // Jump %CodeBlock_2
          //
          // CodeBlock_2:
          // %ssa_3 = Load
          HadError |= true;
          Errors << "Inst %ssa" << CodeOp->ID() << ": Arg[" << i << "] %ssa" << Arg.ID() << " definition does not dominate this use!" << std::endl;
        }
      }

      // CodeLast is inclusive. So we still need to dump the CodeLast op as well
      if (CodeBegin == CodeLast) {
        break;
      }
      ++CodeBegin;
    }

    if (BlockIROp->Next.ID() == 0) {
      break;
    } else {
      BlockNode = BlockIROp->Next.GetNode(ListBegin);
    }
  }

  std::stringstream Out;

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

FEXCore::IR::Pass* CreateValueDominanceValidation() {
  return new ValueDominanceValidation{};
}

}
