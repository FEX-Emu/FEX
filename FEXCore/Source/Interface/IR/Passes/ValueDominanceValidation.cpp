// SPDX-License-Identifier: MIT
/*
$info$
tags: ir|opts
desc: Sanity Checking
$end_info$
*/

#include "Interface/IR/IR.h"
#include "Interface/IR/IREmitter.h"
#include "Interface/IR/PassManager.h"

#include <FEXCore/IR/IR.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/Profiler.h>
#include <FEXCore/fextl/set.h>
#include <FEXCore/fextl/sstream.h>
#include <FEXCore/fextl/unordered_map.h>
#include <FEXCore/fextl/vector.h>

#include <functional>
#include <memory>
#include <stdint.h>
#include <utility>

namespace {
  struct BlockInfo {
    fextl::vector<FEXCore::IR::OrderedNode *> Predecessors;
    fextl::vector<FEXCore::IR::OrderedNode *> Successors;
  };
}

namespace FEXCore::IR::Validation {
class ValueDominanceValidation final : public FEXCore::IR::Pass {
public:
  bool Run(IREmitter *IREmit) override;
};

bool ValueDominanceValidation::Run(IREmitter *IREmit) {
  FEXCORE_PROFILE_SCOPED("PassManager::ValueDominanceValidation");

  bool HadError = false;
  auto CurrentIR = IREmit->ViewIR();

  fextl::ostringstream Errors;
  fextl::unordered_map<IR::NodeID, BlockInfo> OffsetToBlockMap;

  for (auto [BlockNode, BlockHeader] : CurrentIR.GetBlocks()) {

    BlockInfo *CurrentBlock = &OffsetToBlockMap.try_emplace(CurrentIR.GetID(BlockNode)).first->second;

    for (auto [CodeNode, IROp] : CurrentIR.GetCode(BlockNode)) {
      switch (IROp->Op) {
        case IR::OP_CONDJUMP: {
          auto Op = IROp->CW<IR::IROp_CondJump>();

          OrderedNode *TrueTargetNode = CurrentIR.GetNode(Op->TrueBlock);
          OrderedNode *FalseTargetNode = CurrentIR.GetNode(Op->FalseBlock);

          CurrentBlock->Successors.emplace_back(TrueTargetNode);
          CurrentBlock->Successors.emplace_back(FalseTargetNode);

          {
            auto Block = &OffsetToBlockMap.try_emplace(Op->TrueBlock.ID()).first->second;
            Block->Predecessors.emplace_back(BlockNode);
          }

          {
            auto Block = &OffsetToBlockMap.try_emplace(Op->FalseBlock.ID()).first->second;
            Block->Predecessors.emplace_back(BlockNode);
          }

          break;
        }
        case IR::OP_JUMP: {
          auto Op = IROp->CW<IR::IROp_Jump>();
          OrderedNode *TargetNode = CurrentIR.GetNode(Op->Header.Args[0]);
          CurrentBlock->Successors.emplace_back(TargetNode);

          {
            auto Block = OffsetToBlockMap.try_emplace(Op->Header.Args[0].ID()).first;
            Block->second.Predecessors.emplace_back(BlockNode);
          }
          break;
        }
        default: break;
      }
    }
  }

  for (auto [BlockNode, BlockHeader] : CurrentIR.GetBlocks()) {
    auto BlockIROp = BlockHeader->CW<FEXCore::IR::IROp_CodeBlock>();

    for (auto [CodeNode, IROp] : CurrentIR.GetCode(BlockNode)) {
      const auto CodeID = CurrentIR.GetID(CodeNode);

      const uint8_t NumArgs = IR::GetRAArgs(IROp->Op);
      for (uint32_t i = 0; i < NumArgs; ++i) {
        if (IROp->Args[i].IsInvalid()) continue;
        if (CurrentIR.GetOp<IROp_Header>(IROp->Args[i])->Op == OP_IRHEADER) continue;

        OrderedNodeWrapper Arg = IROp->Args[i];

        // We must ensure domininance of all SSA arguments
        if (Arg.ID() >= BlockIROp->Begin.ID() &&
            Arg.ID() < BlockIROp->Last.ID()) {
          // If the SSA argument is defined INSIDE this block
          // then it must only be declared prior to this instruction
          // Eg: Valid
          // CodeBlock_1:
          // %_1 = Load
          // %_2 = Load
          // %_3 = <Op> %_1, %_2
          //
          // Eg: Invalid
          // CodeBlock_1:
          // %_1 = Load
          // %_2 = <Op> %_1, %_3
          // %_3 = Load
          if (Arg.ID() > CodeID) {
            HadError |= true;
            Errors << "Inst %" << CodeID << ": Arg[" << i << "] %" << Arg.ID() << " definition does not dominate this use!" << std::endl;
          }
        }
        else if (Arg.ID() < BlockIROp->Begin.ID()) {
          // If the SSA argument is defined BEFORE this block
          // then THIS block needs to be dominated by the flow of blocks up until this point

          // Eg: Valid
          // CodeBlock_1:
          // %_1 = Load
          // %_2 = Load
          // Jump %CodeBlock_2
          //
          // CodeBlock_2:
          // %_3 = <Op> %_1, %_2
          //
          // Eg: Invalid
          // CodeBlock_1:
          // %_1 = Load
          // %_2 = Load
          // Jump %CodeBlock_3
          //
          // CodeBlock_2:
          // %_3 = <Op> %_1, %_2
          //
          // CodeBlock_3:
          // ...

          // We need to walk the predecessors to see if the value comes from there
          fextl::set<IR::OrderedNode *> Predecessors { BlockNode };
          // Recursively gather all predecessors of BlockNode
          for (auto NodeIt = Predecessors.begin(); NodeIt != Predecessors.end();) {
            auto PredBlock = &OffsetToBlockMap.try_emplace(CurrentIR.GetID(*NodeIt)).first->second;
            ++NodeIt;

            for (auto *Pred : PredBlock->Predecessors) {
              if (Predecessors.insert(Pred).second) {
                // New blocks added, so repeat from the beginning to pull in their predecessors
                NodeIt = Predecessors.begin();
              }
            }
          }

          bool FoundPredDefine = false;

          for (auto* Pred : Predecessors) {
            auto PredIROp = CurrentIR.GetOp<FEXCore::IR::IROp_CodeBlock>(Pred);

            if (Arg.ID() >= PredIROp->Begin.ID() &&
                Arg.ID() < PredIROp->Last.ID()) {
              FoundPredDefine = true;
              break;
            }
            Errors << "\tChecking Pred %" << CurrentIR.GetID(Pred) << std::endl;
          }

          if (!FoundPredDefine) {
            HadError |= true;
            Errors << "Inst %" << CodeID << ": Arg[" << i << "] %" << Arg.ID() << " definition does not dominate this use! But was defined before this block!" << std::endl;
          }
        }
        else if (Arg.ID() > BlockIROp->Last.ID()) {
          // If this SSA argument is defined AFTER this block then it is just completely broken
          // Eg: Invalid
          // CodeBlock_1:
          // %_1 = Load
          // %_2 = <Op> %_1, %_3
          // Jump %CodeBlock_2
          //
          // CodeBlock_2:
          // %_3 = Load
          HadError |= true;
          Errors << "Inst %" << CodeID << ": Arg[" << i << "] %" << Arg.ID() << " definition does not dominate this use!" << std::endl;
        }
      }
    }
  }

  if (HadError) {
    fextl::stringstream Out;
    FEXCore::IR::Dump(&Out, &CurrentIR, nullptr);
    Out << "Errors:" << std::endl << Errors.str() << std::endl;
    LogMan::Msg::EFmt("{}", Out.str());
    LOGMAN_MSG_A_FMT("Encountered IR validation Error");
  }

  return false;
}

fextl::unique_ptr<FEXCore::IR::Pass> CreateValueDominanceValidation() {
  return fextl::make_unique<ValueDominanceValidation>();
}

}
