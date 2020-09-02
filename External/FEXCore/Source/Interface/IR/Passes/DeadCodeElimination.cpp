#include "Interface/IR/PassManager.h"
#include "Interface/Core/OpcodeDispatcher.h"

#include <FEXCore/IR/IR.h>

#include <array>

namespace FEXCore::IR {

class DeadCodeElimination final : public FEXCore::IR::Pass {
  bool Run(IREmitter *IREmit) override;

private:
  void markUsed(OrderedNodeWrapper *CodeOp, IROp_Header *IROp);
};

bool DeadCodeElimination::Run(IREmitter *IREmit) {
  auto CurrentIR = IREmit->ViewIR();

  uintptr_t ListBegin = CurrentIR.GetListData();
  uintptr_t DataBegin = CurrentIR.GetData();

  int NumRemoved = 0;

  for (auto Block : CurrentIR.getBlocks()) {
    auto BlockIROp = Block.GetNode(ListBegin)->Op(DataBegin)->CW<FEXCore::IR::IROp_CodeBlock>();
    LogMan::Throw::A(BlockIROp->Header.Op == OP_CODEBLOCK, "IR type failed to be a code block");

    // We grab these nodes this way so we can iterate easily
    auto CodeBegin = CurrentIR.at(BlockIROp->Begin);
    auto CodeLast = CurrentIR.at(BlockIROp->Last);

    while (1) {
      auto CodeOp = CodeLast();
      OrderedNode *CodeNode = CodeOp->GetNode(ListBegin);
      auto IROp = CodeNode->Op(DataBegin);

      // Skip over anything that has side effects
      // Use count tracking can't safely remove anything with side effects
      if (!IR::HasSideEffects(IROp->Op)) {
        if (CodeNode->GetUses() == 0) {
          NumRemoved++;
          IREmit->Remove(CodeNode);
        }
      }

      if (CodeLast == CodeBegin) {
        break;
      }
      --CodeLast;
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
