#include "Interface/IR/PassManager.h"
#include "Interface/Core/OpcodeDispatcher.h"

#include <array>

namespace FEXCore::IR {

class DeadCodeElimination final : public FEXCore::IR::Pass {
  bool Run(OpDispatchBuilder *Disp) override;

private:
  void markUsed(OrderedNodeWrapper *CodeOp, IROp_Header *IROp);
};

bool DeadCodeElimination::Run(OpDispatchBuilder *Disp) {
  auto CurrentIR = Disp->ViewIR();

  uintptr_t ListBegin = CurrentIR.GetListData();
  uintptr_t DataBegin = CurrentIR.GetData();

  auto Begin = CurrentIR.begin();
  auto Op = Begin();

  OrderedNode *RealNode = Op->GetNode(ListBegin);
  auto HeaderOp = RealNode->Op(DataBegin)->CW<FEXCore::IR::IROp_IRHeader>();
  LogMan::Throw::A(HeaderOp->Header.Op == OP_IRHEADER, "First op wasn't IRHeader");

  OrderedNode *BlockNode = HeaderOp->Blocks.GetNode(ListBegin);

  int NumRemoved = 0;

  while (1) {
    auto BlockIROp = BlockNode->Op(DataBegin)->CW<FEXCore::IR::IROp_CodeBlock>();
    LogMan::Throw::A(BlockIROp->Header.Op == OP_CODEBLOCK, "IR type failed to be a code block");

    // We grab these nodes this way so we can iterate easily
    auto CodeBegin = CurrentIR.at(BlockIROp->Begin);
    auto CodeLast = CurrentIR.at(BlockIROp->Last);

    while (1) {
      auto CodeOp = CodeLast();
      OrderedNode *CodeNode = CodeOp->GetNode(ListBegin);
      auto IROp = CodeNode->Op(DataBegin);

      switch (IROp->Op) {
      // State/memory storage
      case OP_STORECONTEXTINDEXED:
      case OP_STORECONTEXT:
      case OP_STORECONTEXTPAIR:
      case OP_STOREFLAG:
      case OP_STOREMEM:
      case OP_CAS:
      case OP_PRINT:
        // Keep
        break;
      // IO
      case OP_SYSCALL:
        // Keep
        break;
      // Control flow
      case OP_BREAK:
      case OP_JUMP:
      case OP_EXITFUNCTION:
      case OP_CONDJUMP:
        // Keep
        break;
      case OP_DUMMY:
      case OP_ENDBLOCK:
        // Keep, so we don't have to update block first/last
        break;
      default:
        if (CodeNode->GetUses() == 0) {
          NumRemoved++;
          Disp->Remove(CodeNode);
        }
        break;
      }

      if (CodeLast == CodeBegin) {
        break;
      }
      --CodeLast;
    }

    if (BlockIROp->Next.ID() == 0) {
      break;
    } else {
      BlockNode = BlockIROp->Next.GetNode(ListBegin);
    }
  }

  return NumRemoved != 0;
}

void DeadCodeElimination::markUsed(OrderedNodeWrapper *CodeOp, IROp_Header *IROp) {

}


FEXCore::IR::Pass* CreatePassDeadCodeElimination() {
  return new DeadCodeElimination{};
}


}
