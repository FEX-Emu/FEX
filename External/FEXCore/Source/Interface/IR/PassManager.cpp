#include "Interface/IR/Passes.h"
#include "Interface/IR/Passes/RegisterAllocationPass.h"
#include "Interface/IR/PassManager.h"

namespace FEXCore::IR {

void PassManager::AddDefaultPasses(bool InlineConstants) {
  InsertPass(CreateContextLoadStoreElimination());
  InsertPass(CreateConstProp(InlineConstants));
  ////// InsertPass(CreateDeadFlagCalculationEliminination());
  InsertPass(CreateDeadFlagStoreElimination());
  InsertPass(CreateDeadGPRStoreElimination());
  InsertPass(CreateSyscallOptimization());
  InsertPass(CreatePassDeadCodeElimination());

  // If the IR is compacted post-RA then the node indexing gets messed up and the backend isn't able to find the register assigned to a node
  // Compact before IR, don't worry about RA generating spills/fills
  InsertPass(CreateIRCompaction());
}

void PassManager::AddDefaultValidationPasses() {
#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
  InsertValidationPass(Validation::CreatePhiValidation());
  InsertValidationPass(Validation::CreateIRValidation());
  InsertValidationPass(Validation::CreateValueDominanceValidation());
#endif
}

bool PassManager::Run(IREmitter *IREmit) {
  bool Changed = false;
  for (auto const &Pass : Passes) {
    Changed |= Pass->Run(IREmit);
  }

#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
  for (auto const &Pass : ValidationPasses) {
    Changed |= Pass->Run(IREmit);
  }
#endif

  return Changed;
}

}
