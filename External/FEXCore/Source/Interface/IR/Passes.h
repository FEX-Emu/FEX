#pragma once

#include <memory>

namespace FEXCore::Utils {
class IntrusivePooledAllocator;
}

namespace FEXCore::IR {
class Pass;
class RegisterAllocationPass;
class RegisterAllocationData;

std::unique_ptr<FEXCore::IR::Pass> CreateConstProp(bool InlineConstants, bool SupportsTSOImm9);
std::unique_ptr<FEXCore::IR::Pass> CreateContextLoadStoreElimination(bool SupportsAVX);
std::unique_ptr<FEXCore::IR::Pass> CreateSyscallOptimization();
std::unique_ptr<FEXCore::IR::Pass> CreateDeadFlagCalculationEliminination();
std::unique_ptr<FEXCore::IR::Pass> CreateDeadStoreElimination(bool SupportsAVX);
std::unique_ptr<FEXCore::IR::Pass> CreatePassDeadCodeElimination();
std::unique_ptr<FEXCore::IR::Pass> CreateIRCompaction(FEXCore::Utils::IntrusivePooledAllocator &Allocator);
std::unique_ptr<FEXCore::IR::RegisterAllocationPass> CreateRegisterAllocationPass(FEXCore::IR::Pass* CompactionPass,
                                                                                  bool OptimizeSRA,
                                                                                  bool SupportsAVX);
std::unique_ptr<FEXCore::IR::Pass> CreateLongDivideEliminationPass();

namespace Validation {
std::unique_ptr<FEXCore::IR::Pass> CreateIRValidation();
std::unique_ptr<FEXCore::IR::Pass> CreateRAValidation();
std::unique_ptr<FEXCore::IR::Pass> CreatePhiValidation();
std::unique_ptr<FEXCore::IR::Pass> CreateValueDominanceValidation();
}
}

