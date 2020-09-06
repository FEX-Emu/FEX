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

  for (auto [CodeNode, IROp] : CurrentIR.GetAllCode()) {

    // Skip over anything that has side effects
    // Use count tracking can't safely remove anything with side effects
    if (!IR::HasSideEffects(IROp->Op)) {
      if (CodeNode->GetUses() == 0) {
        NumRemoved++;
        IREmit->Remove(CodeNode);
      }
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
