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

  std::ostringstream Errors;

  // Walk the list and calculate the control flow
  for (auto [BlockNode, BlockHeader] : CurrentIR.GetBlocks()) {

    bool FoundNonPhi{};

    for (auto [CodeNode, IROp] : CurrentIR.GetCode(BlockNode)) {

      switch (IROp->Op) {
        // BEGINBLOCK doesn't matter for us
        case IR::OP_BEGINBLOCK: break;
        case IR::OP_PHIVALUE:
        case IR::OP_PHI: {
          if (FoundNonPhi) {
            // If we have found a non-phi IR op and then had a Phi or PhiValue value then this is a programming mistake
            // PHI values MUST be defined at the top of the block only
            HadError |= true;
            Errors << "Phi %ssa" << CurrentIR.GetID(CodeNode) << ": Was defined after non-phi operations. Which is invalid!" << std::endl;
          }

          // Check all the phi values to ensure they have the same type
          break;
        }
        default:
          FoundNonPhi = true;
          break;
      }
    }
  }

  std::stringstream Out;

  if (HadError) {
    FEXCore::IR::Dump(&Out, &CurrentIR, nullptr);

    Out << "Errors:" << std::endl << Errors.str() << std::endl;

    LogMan::Msg::E(Out.str().c_str());
  }


  return false;
}

FEXCore::IR::Pass* CreatePhiValidation() {
  return new PhiValidation{};
}

}
