#pragma once

namespace FEXCore::IR {
class Pass;
class RegisterAllocationPass;

FEXCore::IR::Pass* CreateConstProp(bool InlineConstants);
FEXCore::IR::Pass* CreateContextLoadStoreElimination();
FEXCore::IR::Pass* CreateSyscallOptimization();
FEXCore::IR::Pass* CreateDeadFlagCalculationEliminination();
FEXCore::IR::Pass* CreateDeadFlagStoreElimination();
FEXCore::IR::Pass* CreateDeadGPRStoreElimination();
FEXCore::IR::Pass* CreatePassDeadCodeElimination();
FEXCore::IR::Pass* CreateIRCompaction();
FEXCore::IR::RegisterAllocationPass* CreateRegisterAllocationPass();

namespace Validation {
FEXCore::IR::Pass* CreateIRValidation();
FEXCore::IR::Pass* CreatePhiValidation();
FEXCore::IR::Pass* CreateValueDominanceValidation();
}
}

