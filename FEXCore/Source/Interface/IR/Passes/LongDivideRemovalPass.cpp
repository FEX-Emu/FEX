// SPDX-License-Identifier: MIT
/*
$info$
tags: ir|opts
desc: Long divide elimination pass
$end_info$
*/

#include "Interface/IR/IREmitter.h"
#include "Interface/IR/PassManager.h"
#include <FEXCore/IR/IR.h>
#include <FEXCore/Utils/Profiler.h>

#include <memory>
#include <stdint.h>

namespace FEXCore::IR {

class LongDivideEliminationPass final : public FEXCore::IR::Pass {
public:
  void Run(IREmitter* IREmit) override;
private:
  bool IsZeroOp(IREmitter* IREmit, OrderedNodeWrapper Arg);
  bool IsSextOp(IREmitter* IREmit, OrderedNodeWrapper Lower, OrderedNodeWrapper Upper);
};

bool LongDivideEliminationPass::IsZeroOp(IREmitter* IREmit, OrderedNodeWrapper Arg) {
  uint64_t Value;

  if (IREmit->IsValueConstant(Arg, &Value)) {
    // Zero constant based zero op
    return Value == 0;
  }
  return false;
}

bool LongDivideEliminationPass::IsSextOp(IREmitter* IREmit, OrderedNodeWrapper Lower, OrderedNodeWrapper Upper) {
  // We need to check if the upper source is a sext of the lower source
  auto UpperIROp = IREmit->GetOpHeader(Upper);
  if (UpperIROp->Op == OP_SBFE) {
    auto Op = UpperIROp->C<IR::IROp_Sbfe>();
    if (Op->Width == 1 && Op->lsb == 63) {
      // CQO: OrderedNode *Upper = _Sbfe(1, Size * 8 - 1, Src);
      // If the lower is the upper in this case then it can be optimized
      return Op->Header.Args[0] == Lower;
    }
  }
  return false;
}

void LongDivideEliminationPass::Run(IREmitter* IREmit) {
  FEXCORE_PROFILE_SCOPED("PassManager::LDE");

  auto CurrentIR = IREmit->ViewIR();
  auto OriginalWriteCursor = IREmit->GetWriteCursor();

  for (auto [BlockNode, BlockHeader] : CurrentIR.GetBlocks()) {
    for (auto [CodeNode, IROp] : CurrentIR.GetCode(BlockNode)) {
      if (IROp->Size == 8) {
        if (IROp->Op == OP_LDIV || IROp->Op == OP_LREM) {
          auto Op = IROp->C<IR::IROp_LDiv>();
          // Check upper Op to see if it came from a CQO
          // CQO: OrderedNode *Upper = _Sbfe(1, Size * 8 - 1, Src);
          // If it does then it we only need a 64bit SDIV
          if (IsSextOp(IREmit, Op->Lower, Op->Upper)) {
            IREmit->SetWriteCursor(CodeNode);
            OrderedNode* Lower = CurrentIR.GetNode(Op->Lower);
            OrderedNode* Divisor = CurrentIR.GetNode(Op->Divisor);
            OrderedNode* SDivOp {};
            if (IROp->Op == OP_LDIV) {
              SDivOp = IREmit->_Div(OpSize::i64Bit, Lower, Divisor);
            } else {
              SDivOp = IREmit->_Rem(OpSize::i64Bit, Lower, Divisor);
            }
            IREmit->ReplaceAllUsesWith(CodeNode, SDivOp);
          }
        } else if (IROp->Op == OP_LUDIV || IROp->Op == OP_LUREM) {
          auto Op = IROp->C<IR::IROp_LUDiv>();
          // Check upper Op to see if it came from a zeroing op
          // If it does then it we only need a 64bit UDIV
          if (IsZeroOp(IREmit, Op->Upper)) {
            IREmit->SetWriteCursor(CodeNode);
            OrderedNode* Lower = CurrentIR.GetNode(Op->Lower);
            OrderedNode* Divisor = CurrentIR.GetNode(Op->Divisor);
            OrderedNode* UDivOp {};
            if (IROp->Op == OP_LUDIV) {
              UDivOp = IREmit->_UDiv(OpSize::i64Bit, Lower, Divisor);
            } else {
              UDivOp = IREmit->_URem(OpSize::i64Bit, Lower, Divisor);
            }
            IREmit->ReplaceAllUsesWith(CodeNode, UDivOp);
          }
        }
      }
    }
  }

  IREmit->SetWriteCursor(OriginalWriteCursor);
}

fextl::unique_ptr<FEXCore::IR::Pass> CreateLongDivideEliminationPass() {
  return fextl::make_unique<LongDivideEliminationPass>();
}
} // namespace FEXCore::IR
