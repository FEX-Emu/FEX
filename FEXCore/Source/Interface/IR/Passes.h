// SPDX-License-Identifier: MIT
#pragma once

#include <FEXCore/fextl/memory.h>

namespace FEXCore {
class CPUIDEmu;
}

namespace FEXCore::Utils {
class IntrusivePooledAllocator;
}

namespace FEXCore::IR {
class Pass;
class RegisterAllocationPass;
class RegisterAllocationData;

fextl::unique_ptr<FEXCore::IR::Pass> CreateConstProp(bool InlineConstants, bool SupportsTSOImm9);
fextl::unique_ptr<FEXCore::IR::Pass> CreateContextLoadStoreElimination(bool SupportsAVX);
fextl::unique_ptr<FEXCore::IR::Pass> CreateInlineCallOptimization(const FEXCore::CPUIDEmu* CPUID);
fextl::unique_ptr<FEXCore::IR::Pass> CreateDeadFlagCalculationEliminination();
fextl::unique_ptr<FEXCore::IR::Pass> CreateDeadStoreElimination();
fextl::unique_ptr<FEXCore::IR::RegisterAllocationPass> CreateRegisterAllocationPass();
fextl::unique_ptr<FEXCore::IR::Pass> CreateLongDivideEliminationPass();

namespace Validation {
  fextl::unique_ptr<FEXCore::IR::Pass> CreateIRValidation();
  fextl::unique_ptr<FEXCore::IR::Pass> CreateRAValidation();
} // namespace Validation

namespace Debug {
  fextl::unique_ptr<FEXCore::IR::Pass> CreateIRDumper();
}
} // namespace FEXCore::IR
