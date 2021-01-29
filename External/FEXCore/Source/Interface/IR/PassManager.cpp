#include "Interface/IR/PassManager.h"
#include "Interface/IR/Passes.h"
#include "Interface/IR/Passes/RegisterAllocationPass.h"

#include <FEXCore/Config/Config.h>

namespace FEXCore::IR {

void PassManager::AddDefaultPasses(bool InlineConstants, bool StaticRegisterAllocation) {
  FEXCore::Config::Value<bool> DisablePasses{FEXCore::Config::CONFIG_DEBUG_DISABLE_OPTIMIZATION_PASSES, false};

  if (!DisablePasses()) {
    InsertPass(CreateContextLoadStoreElimination());
    InsertPass(CreateDeadStoreElimination());
    InsertPass(CreatePassDeadCodeElimination());
    InsertPass(CreateConstProp(InlineConstants));

    ////// InsertPass(CreateDeadFlagCalculationEliminination());

    InsertPass(CreateSyscallOptimization());
    InsertPass(CreatePassDeadCodeElimination());

    // only do SRA if enabled and JIT
    if (InlineConstants && StaticRegisterAllocation)
      InsertPass(CreateStaticRegisterAllocationPass());
  }
  else {
    // only do SRA if enabled and JIT
    if (InlineConstants && StaticRegisterAllocation)
      InsertPass(CreateStaticRegisterAllocationPass());
  }

  CompactionPass = CreateIRCompaction();
  // If the IR is compacted post-RA then the node indexing gets messed up and the backend isn't able to find the register assigned to a node
  // Compact before IR, don't worry about RA generating spills/fills
  InsertPass(CompactionPass);
}

void PassManager::AddDefaultValidationPasses() {
#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
  InsertValidationPass(Validation::CreatePhiValidation());
  InsertValidationPass(Validation::CreateIRValidation());
  InsertValidationPass(Validation::CreateValueDominanceValidation());
#endif
}

void PassManager::InsertRegisterAllocationPass(bool OptimizeSRA) {
    RAPass = IR::CreateRegisterAllocationPass(CompactionPass, OptimizeSRA);
    InsertPass(RAPass);
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
