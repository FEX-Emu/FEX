#include "Interface/IR/PassManager.h"
#include "Interface/Core/OpcodeDispatcher.h"

namespace FEXCore::IR {

class RedundantFlagCalculationEliminination final : public FEXCore::IR::Pass {
public:
  bool Run(OpDispatchBuilder *Disp) override;
};

bool RedundantFlagCalculationEliminination::Run(OpDispatchBuilder *Disp) {
  bool Changed = false;
  auto CurrentIR = Disp->ViewIR();
  uintptr_t ListBegin = CurrentIR.GetListData();
  uintptr_t DataBegin = CurrentIR.GetData();

  IR::NodeWrapperIterator Begin = CurrentIR.begin();
  IR::NodeWrapperIterator End = CurrentIR.end();

  std::array<OrderedNode*, 32> LastValidFlagStores{};

  while (Begin != End) {
    NodeWrapper *WrapperOp = Begin();
    OrderedNode *RealNode = reinterpret_cast<OrderedNode*>(WrapperOp->GetPtr(ListBegin));
    FEXCore::IR::IROp_Header *IROp = RealNode->Op(DataBegin);

    if (IROp->Op == OP_BEGINBLOCK ||
        IROp->Op == OP_ENDBLOCK ||
        IROp->Op == OP_JUMP ||
        IROp->Op == OP_CONDJUMP ||
        IROp->Op == OP_EXITFUNCTION) {
      // We don't track across block boundaries
      LastValidFlagStores.fill(nullptr);
    }

    if (IROp->Op == OP_STOREFLAG) {
      auto Op = IROp->CW<IR::IROp_StoreFlag>();

      // If we have had a valid flag store previously and it hasn't been touched until this new store
      // Then just delete the old one and let DCE to take care of the rest
      if (LastValidFlagStores[Op->Flag] != nullptr) {
        Disp->Unlink(LastValidFlagStores[Op->Flag]);
        Changed = true;
      }

      // Set this node as the last one valid for this flag
      LastValidFlagStores[Op->Flag] = RealNode;
    }
    else if (IROp->Op == OP_LOADFLAG) {
      auto Op = IROp->CW<IR::IROp_LoadFlag>();

      // If we loaded a flag then we can't track past this
      LastValidFlagStores[Op->Flag] = nullptr;
    }

    ++Begin;
  }

  return Changed;
}

FEXCore::IR::Pass* CreateRedundantFlagCalculationEliminination() {
  return new RedundantFlagCalculationEliminination{};
}

}
