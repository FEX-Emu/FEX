#include "Interface/IR/PassManager.h"
#include "Interface/Core/OpcodeDispatcher.h"

#include <iostream>

namespace FEXCore::IR::Validation {

class PhiValidation final : public FEXCore::IR::Pass {
public:
  bool Run(IREmitter *IREmit) override;
};

bool PhiValidation::Run(IREmitter *IREmit) {
  bool HadError = false;
  auto CurrentIR = IREmit->ViewIR();
  uintptr_t ListBegin = CurrentIR.GetListData();
  uintptr_t DataBegin = CurrentIR.GetData();

  std::ostringstream Errors;

  // Walk the list and calculate the control flow
  for (auto [BlockNode, BlockHeader] : CurrentIR.getBlocks()) {
    auto BlockIROp = BlockHeader->CW<FEXCore::IR::IROp_CodeBlock>();
    LogMan::Throw::A(BlockIROp->Header.Op == OP_CODEBLOCK, "IR type failed to be a code block");

    // We grab these nodes this way so we can iterate easily
    auto CodeBegin = CurrentIR.at(BlockIROp->Begin);
    auto CodeLast = CurrentIR.at(BlockIROp->Last);

    bool FoundNonPhi{};

    while (1) {
      auto CodeOp = CodeBegin();
      OrderedNode *CodeNode = CodeOp->GetNode(ListBegin);
      auto IROp = CodeNode->Op(DataBegin);

      switch (IROp->Op) {
        // DUMMY doesn't matter for us
        case IR::OP_DUMMY: break;
        case IR::OP_PHIVALUE:
        case IR::OP_PHI: {
          if (FoundNonPhi) {
            // If we have found a non-phi IR op and then had a Phi or PhiValue value then this is a programming mistake
            // PHI values MUST be defined at the top of the block only
            HadError |= true;
            Errors << "Phi %ssa" << CodeOp->ID() << ": Was defined after non-phi operations. Which is invalid!" << std::endl;
          }

          // Check all the phi values to ensure they have the same type
          break;
        }
        default:
          FoundNonPhi = true;
          break;
      }

      // CodeLast is inclusive. So we still need to dump the CodeLast op as well
      if (CodeBegin == CodeLast) {
        break;
      }
      ++CodeBegin;
    }
  }

  std::stringstream Out;

  if (HadError) {
    FEXCore::IR::Dump(&Out, &CurrentIR, nullptr);

    Out << "Errors:" << std::endl << Errors.str() << std::endl;

    std::cerr << Out.str() << std::endl;
  }


  return false;
}

FEXCore::IR::Pass* CreatePhiValidation() {
  return new PhiValidation{};
}

}
