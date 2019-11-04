#pragma once

namespace FEXCore::IR {
class Pass;

FEXCore::IR::Pass* CreateConstProp();
FEXCore::IR::Pass* CreateRedundantContextLoadElimination();
FEXCore::IR::Pass* CreatePassDeadContextStoreElimination();
FEXCore::IR::Pass* CreateSyscallOptimization();
FEXCore::IR::Pass* CreateRedundantFlagCalculationEliminination();
FEXCore::IR::Pass* CreateDeadFlagCalculationEliminination();
FEXCore::IR::Pass* CreatePassDeadCodeElimination();
FEXCore::IR::Pass* CreateIRCompaction();

namespace Validation {
FEXCore::IR::Pass* CreateIRValidation();
}
}

