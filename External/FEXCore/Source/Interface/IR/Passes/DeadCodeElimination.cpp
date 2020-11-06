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
  int NumRemoved = 0;

  for (auto [BlockNode, BlockHeader] : CurrentIR.GetBlocks()) {

    // Reverse iteration is not yet working with the iterators
    auto BlockIROp = BlockHeader->CW<FEXCore::IR::IROp_CodeBlock>();

    // We grab these nodes this way so we can iterate easily
    auto CodeBegin = CurrentIR.at(BlockIROp->Begin);
    auto CodeLast = CurrentIR.at(BlockIROp->Last);

    while (1) {
      auto [CodeNode, IROp] = CodeLast();

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
