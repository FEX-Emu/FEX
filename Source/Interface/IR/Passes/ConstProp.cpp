#include "Interface/IR/PassManager.h"
#include "Interface/Core/OpcodeDispatcher.h"

namespace FEXCore::IR {

class ConstProp final : public FEXCore::IR::Pass {
public:
  bool Run(OpDispatchBuilder *Disp) override;
};

bool ConstProp::Run(OpDispatchBuilder *Disp) {
  bool Changed = false;
  auto CurrentIR = Disp->ViewIR();
  uintptr_t ListBegin = CurrentIR.GetListData();
  uintptr_t DataBegin = CurrentIR.GetData();

  IR::NodeWrapperIterator Begin = CurrentIR.begin();
  IR::NodeWrapperIterator End = CurrentIR.end();

  while (Begin != End) {
    NodeWrapper *WrapperOp = Begin();
    OrderedNode *RealNode = reinterpret_cast<OrderedNode*>(WrapperOp->GetPtr(ListBegin));
    FEXCore::IR::IROp_Header *IROp = RealNode->Op(DataBegin);

    switch (IROp->Op) {
    case OP_ZEXT: {
      auto Op = IROp->C<IR::IROp_Zext>();
      uint64_t Constant;
      if (Disp->IsValueConstant(Op->Header.Args[0], &Constant)) {
        uint64_t NewConstant = Constant & ((1ULL << Op->SrcSize) - 1);
        auto ConstantVal = Disp->_Constant(NewConstant);
        Disp->ReplaceAllUsesWith(RealNode, ConstantVal);
        Changed = true;
      }

    break;
    }
    default: break;
    }

    ++Begin;
  }

  return Changed;
}

FEXCore::IR::Pass* CreateConstProp() {
  return new ConstProp{};
}

}
