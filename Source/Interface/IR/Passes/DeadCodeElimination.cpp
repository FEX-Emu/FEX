#include "Interface/IR/PassManager.h"
#include "Interface/Core/OpcodeDispatcher.h"

#include <array>

namespace FEXCore::IR {

class DeadCodeElimination final : public FEXCore::IR::Pass {
  bool Run(OpDispatchBuilder *Disp) override;

private:
  std::vector<int8_t> SSAStatus;
  void markUsed(OrderedNodeWrapper *CodeOp, IROp_Header *IROp);
};

enum {
  UNKNOWN,
  UNUSED,
  USED,
};


bool DeadCodeElimination::Run(OpDispatchBuilder *Disp) {
  bool Changed = false;
  auto CurrentIR = Disp->ViewIR();

  uint32_t SSACount = CurrentIR.GetSSACount();

  SSAStatus = std::vector<int8_t>(SSACount, UNKNOWN);

  uintptr_t ListBegin = CurrentIR.GetListData();
  uintptr_t DataBegin = CurrentIR.GetData();

  std::function<void(OrderedNodeWrapper)> MarkUsed = [&] (OrderedNodeWrapper CodeOp) {
    if (SSAStatus[CodeOp.ID()] == USED)
      return;
    SSAStatus[CodeOp.ID()] = USED;

    OrderedNode *CodeNode = CodeOp.GetNode(ListBegin);
    auto IROp = CodeNode->Op(DataBegin);

    uint8_t NumArgs = IR::GetArgs(IROp->Op);
    for (uint8_t i = 0; i < NumArgs; ++i) {
      MarkUsed(IROp->Args[i]);
    }
  };

  auto Begin = CurrentIR.begin();
  auto Op = Begin();

  OrderedNode *RealNode = Op->GetNode(ListBegin);
  auto HeaderOp = RealNode->Op(DataBegin)->CW<FEXCore::IR::IROp_IRHeader>();
  LogMan::Throw::A(HeaderOp->Header.Op == OP_IRHEADER, "First op wasn't IRHeader");

  OrderedNode *BlockNode = HeaderOp->Blocks.GetNode(ListBegin);

  // First, iterate over IR nodes in all blocks, marking them as used or unused

  while (1) {
    auto BlockIROp = BlockNode->Op(DataBegin)->CW<FEXCore::IR::IROp_CodeBlock>();
    LogMan::Throw::A(BlockIROp->Header.Op == OP_CODEBLOCK, "IR type failed to be a code block");

    // We grab these nodes this way so we can iterate easily
    auto CodeBegin = CurrentIR.at(BlockIROp->Begin);
    auto CodeLast = CurrentIR.at(BlockIROp->Last);

    bool controlFlowEnded = false;
    while (1) {
      auto CodeOp = CodeBegin();
      OrderedNode *CodeNode = CodeOp->GetNode(ListBegin);
      auto IROp = CodeNode->Op(DataBegin);
      auto ssa = CodeOp->ID();

      if (SSAStatus[ssa] == UNKNOWN) {
        SSAStatus[ssa] = UNUSED;
      }

      if (!controlFlowEnded) {
        switch (IROp->Op) {
        // State storage
        case OP_STORECONTEXT:
        case OP_STOREFLAG:
        case OP_STOREMEM:
        case OP_CAS:
          MarkUsed(*CodeOp);
          break;
        // IO
        case OP_SYSCALL:
          MarkUsed(*CodeOp);
          break;
        // Control flow
        case OP_BREAK:
        case OP_JUMP:
        case OP_EXITFUNCTION:
          SSAStatus[ssa] = USED;
          controlFlowEnded = true; // Kill any IR instructions after this
          break;
        case OP_CONDJUMP:
          SSAStatus[ssa] = USED;
          MarkUsed(IROp->Args[0]); // Mark the condition as used
          break;
        case OP_ENDBLOCK:
          // Only keep this when incrementing
          auto Op = IROp->C<IROp_EndBlock>();
          if (Op->RIPIncrement) {
            SSAStatus[ssa] = USED;
          }
          break;
        default:
          // leave everything else marked as unused
          break;
        }
      }

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

  // Loop over blocks
  while (1) {
    auto BlockIROp = BlockNode->Op(DataBegin)->CW<FEXCore::IR::IROp_CodeBlock>();
    LogMan::Throw::A(BlockIROp->Header.Op == OP_CODEBLOCK, "IR type failed to be a code block");

    // We grab these nodes this way so we can iterate easily
    auto CodeBegin = CurrentIR.at(BlockIROp->Begin);
    auto CodeLast = CurrentIR.at(BlockIROp->Last);

    while (SSAStatus[CodeBegin()->ID()] != USED) {
      if (CodeBegin == CodeLast) {

      }
      ++CodeBegin;


    }

    auto &prev = BlockIROp->Begin;

    while (1) {
      auto CodeOp = CodeBegin();

      if (SSAStatus[CodeOp->ID()] == USED) {
        prev = CodeOp->WrapOffset();

      }
    }

  }

  return Changed;
}

void DeadCodeElimination::markUsed(OrderedNodeWrapper *CodeOp, IROp_Header *IROp) {

}


FEXCore::IR::Pass* CreatePassDeadCodeElimination() {
  return new DeadCodeElimination{};
}


}