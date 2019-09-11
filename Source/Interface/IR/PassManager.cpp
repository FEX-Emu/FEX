#include "Interface/IR/Passes.h"
#include "Interface/IR/PassManager.h"

namespace FEXCore::IR {
void PassManager::AddDefaultPasses() {
  Passes.emplace_back(std::unique_ptr<FEXCore::IR::Pass>(CreateConstProp()));
  Passes.emplace_back(std::unique_ptr<FEXCore::IR::Pass>(CreateRedundantContextLoadElimination()));
  Passes.emplace_back(std::unique_ptr<FEXCore::IR::Pass>(CreateRedundantFlagCalculationEliminination()));
  Passes.emplace_back(std::unique_ptr<FEXCore::IR::Pass>(CreateSyscallOptimization()));
  Passes.emplace_back(std::unique_ptr<FEXCore::IR::Pass>(CreatePassDeadContextStoreElimination()));

  Passes.emplace_back(std::unique_ptr<FEXCore::IR::Pass>(CreateIRCompaction()));
}

void PassManager::AddDefaultValidationPasses() {
  // Passes.emplace_back(std::unique_ptr<FEXCore::IR::Pass>(Validation::CreateIRValidation()));
}

bool PassManager::Run(OpDispatchBuilder *Disp) {
  bool Changed = false;
  for (auto const &Pass : Passes) {
    Changed |= Pass->Run(Disp);
  }
  return Changed;
}

}
