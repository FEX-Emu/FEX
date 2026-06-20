// SPDX-License-Identifier: MIT
#pragma once

#include <FEXCore/fextl/memory.h>

namespace FEXCore {
class CPUIDEmu;
struct HostFeatures;
} // namespace FEXCore

namespace FEXCore::IR {
class Pass;

fextl::unique_ptr<Pass> CreateDeadFlagCalculationEliminination();
fextl::unique_ptr<Pass> CreateRegisterAllocationPass(const CPUIDEmu* CPUID);
fextl::unique_ptr<Pass> CreateX87StackOptimizationPass(const HostFeatures&, OpSize GPROpSize);

namespace Validation {
  fextl::unique_ptr<Pass> CreateIRValidation();
} // namespace Validation

namespace Debug {
  fextl::unique_ptr<Pass> CreateIRDumper();
}
} // namespace FEXCore::IR
