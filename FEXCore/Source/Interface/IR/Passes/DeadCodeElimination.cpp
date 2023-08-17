/*
$info$
tags: ir|opts
$end_info$
*/

#include "Interface/IR/PassManager.h"

#include <FEXCore/IR/IR.h>
#include <FEXCore/IR/IREmitter.h>
#include <FEXCore/IR/IntrusiveIRList.h>
#include <FEXCore/Utils/Profiler.h>

#include <memory>

namespace FEXCore::IR {

class DeadCodeElimination final : public FEXCore::IR::Pass {
  bool Run(IREmitter *IREmit) override;

private:
  void markUsed(OrderedNodeWrapper *CodeOp, IROp_Header *IROp);
};

bool DeadCodeElimination::Run(IREmitter *IREmit) {
  FEXCORE_PROFILE_SCOPED("PassManager::DCE");
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

      bool HasSideEffects = IR::HasSideEffects(IROp->Op);
      if (IROp->Op == OP_SYSCALL ||
          IROp->Op == OP_INLINESYSCALL) {
        FEXCore::IR::SyscallFlags Flags{};
        if (IROp->Op == OP_SYSCALL) {
          auto Op = IROp->C<IR::IROp_Syscall>();
          Flags = Op->Flags;
        }
        else {
          auto Op = IROp->C<IR::IROp_InlineSyscall>();
          Flags = Op->Flags;
        }

        if ((Flags & FEXCore::IR::SyscallFlags::NOSIDEEFFECTS) == FEXCore::IR::SyscallFlags::NOSIDEEFFECTS) {
          HasSideEffects = false;
        }
      }

      // Skip over anything that has side effects
      // Use count tracking can't safely remove anything with side effects
      if (!HasSideEffects) {
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

fextl::unique_ptr<FEXCore::IR::Pass> CreatePassDeadCodeElimination() {
  return fextl::make_unique<DeadCodeElimination>();
}

}
