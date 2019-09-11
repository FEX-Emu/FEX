#include "Interface/IR/PassManager.h"
#include "Interface/Core/OpcodeDispatcher.h"

#include <iostream>

namespace FEXCore::IR::Validation {

struct BlockInfo {
  IR::NodeWrapper *Begin;
  IR::NodeWrapper *End;

  bool HasExit;

  std::vector<IR::NodeWrapper*> Predecessors;
  std::vector<IR::NodeWrapper*> Successors;
};

class IRValidation final : public FEXCore::IR::Pass {
public:
  bool Run(OpDispatchBuilder *Disp) override;

private:
  std::unordered_map<IR::NodeWrapper::NodeOffsetType, BlockInfo> OffsetToBlockMap;
};

bool IRValidation::Run(OpDispatchBuilder *Disp) {
  bool HadError = false;
  auto CurrentIR = Disp->ViewIR();
  uintptr_t ListBegin = CurrentIR.GetListData();
  uintptr_t DataBegin = CurrentIR.GetData();

  IR::NodeWrapperIterator Begin = CurrentIR.begin();
  IR::NodeWrapperIterator End = CurrentIR.end();

  bool InBlock = false;
  BlockInfo *CurrentBlock {};
  std::ostringstream Errors;

  while (Begin != End) {
    NodeWrapper *WrapperOp = Begin();
    OrderedNode *RealNode = reinterpret_cast<OrderedNode*>(WrapperOp->GetPtr(ListBegin));
    FEXCore::IR::IROp_Header *IROp = RealNode->Op(DataBegin);

    uint8_t OpSize = IROp->Size;

    if (IROp->HasDest) {
      HadError |= OpSize == 0;
      if (OpSize == 0) {
        Errors << "%ssa" << WrapperOp->ID() << ": Had destination but with no size" << std::endl;
      }

      if (RealNode->GetUses() == 0) {
        HadError |= true;
        Errors << "%ssa" << WrapperOp->ID() << ": Destination created but had no uses" << std::endl;
      }
    }

    for (uint8_t i = 0; i < IROp->NumArgs; ++i) {
      NodeWrapper Arg = IROp->Args[i];
      if (Arg.ID() == 0) {
        HadError |= true;
        Errors << "Op" << WrapperOp->ID() <<": Arg[" << i << "] has invalid target of %ssa0" << std::endl;
      }
    }

    switch (IROp->Op) {
    case OP_BEGINBLOCK: {
      HadError |= InBlock;
      if (InBlock) {
        Errors << "BasicBlock " << WrapperOp->ID() << ": Begin in middle of block" << std::endl;
      }

      auto Block = OffsetToBlockMap.try_emplace(WrapperOp->ID(), BlockInfo{}).first;
      CurrentBlock = &Block->second;
      CurrentBlock->Begin = WrapperOp;
      InBlock = true;
    break;
    }

    case OP_ENDBLOCK: {
      HadError |= !InBlock;
      if (!InBlock) {
        Errors << "BasicBlock " << WrapperOp->ID() << ": End loose without a begin" << std::endl;
      }

      if (CurrentBlock) {
        // XXX: Enable once fallthrough is handled
        // HadError |= !CurrentBlock->HasExit && CurrentBlock->Successors.size() == 0;
        // if (!CurrentBlock->HasExit && CurrentBlock->Successors.size() == 0) {
        //   Errors << "BasicBlock " << WrapperOp->ID() << ": Didn't have an exit and didn't have any successors. (Fallthrough?)" << std::endl;
        // }
        CurrentBlock->End = WrapperOp;
        CurrentBlock = nullptr;
      }
      InBlock = false;
    break;
    }
    case IR::OP_EXITFUNCTION:
    case IR::OP_ENDFUNCTION: {
      if (CurrentBlock) {
        CurrentBlock->HasExit = true;
      }
    break;
    }
    case IR::OP_CONDJUMP: {
      auto Op = IROp->C<IR::IROp_CondJump>();
      auto IterLocation = NodeWrapperIterator(ListBegin, Op->Header.Args[1]);
      if (CurrentBlock) {
        CurrentBlock->Successors.emplace_back(IterLocation());
      }

      OrderedNode *TargetNode = reinterpret_cast<OrderedNode*>(IterLocation()->GetPtr(ListBegin));
      FEXCore::IR::IROp_Header *TargetOp = TargetNode->Op(DataBegin);
      HadError |= TargetOp->Op != OP_BEGINBLOCK;
      if (TargetOp->Op != OP_BEGINBLOCK) {
        Errors << "CondJump " << WrapperOp->ID() << ": CondJump to Op that isn't the begining of a block" << std::endl;
      }
      else {
        auto Block = OffsetToBlockMap.try_emplace(IterLocation()->NodeOffset, BlockInfo{}).first;
        Block->second.Predecessors.emplace_back(CurrentBlock->Begin);
      }

    break;
    }

    case IR::OP_JUMP: {
      auto Op = IROp->C<IR::IROp_Jump>();
      auto IterLocation = NodeWrapperIterator(ListBegin, Op->Header.Args[0]);
      if (CurrentBlock) {
        CurrentBlock->Successors.emplace_back(IterLocation());
      }

      OrderedNode *TargetNode = reinterpret_cast<OrderedNode*>(IterLocation()->GetPtr(ListBegin));
      FEXCore::IR::IROp_Header *TargetOp = TargetNode->Op(DataBegin);
      HadError |= TargetOp->Op != OP_BEGINBLOCK;
      if (TargetOp->Op != OP_BEGINBLOCK) {
        Errors << "Jump " << WrapperOp->ID() << ": Jump to Op that isn't the begining of a block" << std::endl;
      }
      else {
        auto Block = OffsetToBlockMap.try_emplace(IterLocation()->NodeOffset, BlockInfo{}).first;
        Block->second.Predecessors.emplace_back(CurrentBlock->Begin);
      }
    break;
    }

    default:
    //LogMan::Msg::A("Unknown IR Op: %d(%s)", IROp->Op, FEXCore::IR::GetName(IROp->Op).data());
    break;
    }

    ++Begin;
  }

  if (HadError) {
    std::stringstream Out;
    FEXCore::IR::Dump(&Out, &CurrentIR);

    std::cerr << Errors.str() << std::endl << Out.str() << std::endl;
  }
  return false;
}

FEXCore::IR::Pass* CreateIRValidation() {
  return new IRValidation{};
}
}
