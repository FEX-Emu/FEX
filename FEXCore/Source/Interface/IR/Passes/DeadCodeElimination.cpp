// SPDX-License-Identifier: MIT
/*
$info$
tags: ir|opts
$end_info$
*/

#include "Interface/IR/IREmitter.h"
#include "Interface/IR/PassManager.h"

#include <FEXCore/IR/IR.h>
#include <FEXCore/Utils/Profiler.h>

#include <memory>

namespace FEXCore::IR {

class DeadCodeElimination final : public FEXCore::IR::Pass {
  void Run(IREmitter* IREmit) override;

private:
  void markUsed(OrderedNodeWrapper* CodeOp, IROp_Header* IROp);
};

void DeadCodeElimination::Run(IREmitter* IREmit) {
  FEXCORE_PROFILE_SCOPED("PassManager::DCE");
  auto CurrentIR = IREmit->ViewIR();

  for (auto [BlockNode, BlockHeader] : CurrentIR.GetBlocks()) {

    // Reverse iteration is not yet working with the iterators
    auto BlockIROp = BlockHeader->CW<FEXCore::IR::IROp_CodeBlock>();

    // We grab these nodes this way so we can iterate easily
    auto CodeBegin = CurrentIR.at(BlockIROp->Begin);
    auto CodeLast = CurrentIR.at(BlockIROp->Last);

    while (1) {
      auto [CodeNode, IROp] = CodeLast();

      bool HasSideEffects = IR::HasSideEffects(IROp->Op);

      switch (IROp->Op) {
      case OP_SYSCALL:
      case OP_INLINESYSCALL: {
        FEXCore::IR::SyscallFlags Flags {};
        if (IROp->Op == OP_SYSCALL) {
          auto Op = IROp->C<IR::IROp_Syscall>();
          Flags = Op->Flags;
        } else {
          auto Op = IROp->C<IR::IROp_InlineSyscall>();
          Flags = Op->Flags;
        }

        if ((Flags & FEXCore::IR::SyscallFlags::NOSIDEEFFECTS) == FEXCore::IR::SyscallFlags::NOSIDEEFFECTS) {
          HasSideEffects = false;
        }

        break;
      }
      case OP_ATOMICFETCHADD:
      case OP_ATOMICFETCHSUB:
      case OP_ATOMICFETCHAND:
      case OP_ATOMICFETCHCLR:
      case OP_ATOMICFETCHOR:
      case OP_ATOMICFETCHXOR:
      case OP_ATOMICFETCHNEG: {
        // If the result of the atomic fetch is completely unused, convert it to a non-fetching atomic operation.
        if (CodeNode->GetUses() == 0) {
          switch (IROp->Op) {
          case OP_ATOMICFETCHADD: IROp->Op = OP_ATOMICADD; break;
          case OP_ATOMICFETCHSUB: IROp->Op = OP_ATOMICSUB; break;
          case OP_ATOMICFETCHAND: IROp->Op = OP_ATOMICAND; break;
          case OP_ATOMICFETCHCLR: IROp->Op = OP_ATOMICCLR; break;
          case OP_ATOMICFETCHOR: IROp->Op = OP_ATOMICOR; break;
          case OP_ATOMICFETCHXOR: IROp->Op = OP_ATOMICXOR; break;
          case OP_ATOMICFETCHNEG: IROp->Op = OP_ATOMICNEG; break;
          default: FEX_UNREACHABLE;
          }
        }
        break;
      }
      default: break;
      }

      // Skip over anything that has side effects
      // Use count tracking can't safely remove anything with side effects
      if (!HasSideEffects) {
        if (CodeNode->GetUses() == 0) {
          IREmit->Remove(CodeNode);
        }
      }

      if (CodeLast == CodeBegin) {
        break;
      }
      --CodeLast;
    }
  }
}

void DeadCodeElimination::markUsed(OrderedNodeWrapper* CodeOp, IROp_Header* IROp) {}

fextl::unique_ptr<FEXCore::IR::Pass> CreatePassDeadCodeElimination() {
  return fextl::make_unique<DeadCodeElimination>();
}

} // namespace FEXCore::IR
