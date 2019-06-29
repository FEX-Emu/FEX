#include "Interface/IR/PassManager.h"
#include "Interface/Core/OpcodeDispatcher.h"

#include "LogManager.h"

namespace FEXCore::IR {

class SyscallOptimization final : public FEXCore::IR::Pass {
public:
  bool Run(OpDispatchBuilder *Disp) override;
};

bool SyscallOptimization::Run(OpDispatchBuilder *Disp) {
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

    if (IROp->Op == FEXCore::IR::OP_SYSCALL) {
      // Is the first argument a constant?
      uint64_t Constant;
      if (Disp->IsValueConstant(IROp->Args[0], &Constant)) {
      //  LogMan::Msg::A("Whoa. Syscall argument is constant: %ld", Constant);
        Changed = true;
      }

    }
    ++Begin;
  }

  return Changed;
}

FEXCore::IR::Pass* CreateSyscallOptimization() {
  return new SyscallOptimization{};
}

}
