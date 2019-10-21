#include "Interface/IR/PassManager.h"
#include "Interface/Core/OpcodeDispatcher.h"

namespace FEXCore::IR {

class RedundantFlagCalculationEliminination final : public FEXCore::IR::Pass {
public:
  bool Run(OpDispatchBuilder *Disp) override;
};

bool RedundantFlagCalculationEliminination::Run(OpDispatchBuilder *Disp) {
  std::array<OrderedNode*, 32> LastValidFlagStores{};

  bool Changed = false;
  auto CurrentIR = Disp->ViewIR();
  uintptr_t ListBegin = CurrentIR.GetListData();
  uintptr_t DataBegin = CurrentIR.GetData();

  auto Begin = CurrentIR.begin();
  auto Op = Begin();

  OrderedNode *RealNode = Op->GetNode(ListBegin);
  auto HeaderOp = RealNode->Op(DataBegin)->CW<FEXCore::IR::IROp_IRHeader>();
  LogMan::Throw::A(HeaderOp->Header.Op == OP_IRHEADER, "First op wasn't IRHeader");

  OrderedNode *BlockNode = HeaderOp->Blocks.GetNode(ListBegin);

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

      if (IROp->Op == OP_STOREFLAG) {
        auto Op = IROp->CW<IR::IROp_StoreFlag>();

        // If we have had a valid flag store previously and it hasn't been touched until this new store
        // Then just delete the old one and let DCE to take care of the rest
        if (LastValidFlagStores[Op->Flag] != nullptr) {
          Disp->Remove(LastValidFlagStores[Op->Flag]);
          Changed = true;
        }

        // Set this node as the last one valid for this flag
        LastValidFlagStores[Op->Flag] = CodeNode;
      }
      else if (IROp->Op == OP_LOADFLAG) {
        auto Op = IROp->CW<IR::IROp_LoadFlag>();

        // If we loaded a flag then we can't track past this
        LastValidFlagStores[Op->Flag] = nullptr;
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

    // We don't track across block boundaries
    LastValidFlagStores.fill(nullptr);
  }

  return Changed;
}

FEXCore::IR::Pass* CreateRedundantFlagCalculationEliminination() {
  return new RedundantFlagCalculationEliminination{};
}

}
