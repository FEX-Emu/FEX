// SPDX-License-Identifier: MIT
/*
$info$
meta: ir|opts ~ IR to IR Optimization
tags: ir|opts
desc: Defines which passes are run, and runs them
$end_info$
*/

#include "Interface/Context/Context.h"
#include "Interface/IR/PassManager.h"
#include "Interface/IR/Passes.h"
#include "Interface/IR/Passes/RegisterAllocationPass.h"

#include <FEXCore/Config/Config.h>
#include <FEXCore/Utils/Profiler.h>

namespace FEXCore::IR {
class IREmitter;

void PassManager::Finalize() {
  if (!PassManagerDumpIR()) {
    // Not configured to dump any IR, just return.
    return;
  }

  auto it = Passes.begin();
  // Walk the passes and add them where asked.
  if (true || PassManagerDumpIR() & FEXCore::Config::PassManagerDumpIR::BEFOREOPT) {
    // Insert at the start.
    it = InsertAt(it, Debug::CreateIRDumper());
    ++it; // Skip what we inserted.
  }

  if ((PassManagerDumpIR() & FEXCore::Config::PassManagerDumpIR::BEFOREPASS) ||
      (PassManagerDumpIR() & FEXCore::Config::PassManagerDumpIR::AFTERPASS)) {

    bool SkipFirstBefore = PassManagerDumpIR() & FEXCore::Config::PassManagerDumpIR::BEFOREOPT;
    for (; it != Passes.end();) {
      if (PassManagerDumpIR() & FEXCore::Config::PassManagerDumpIR::BEFOREPASS) {
        if (SkipFirstBefore) {
          // If we need to skip the first one, then continue.
          SkipFirstBefore = false;
          ++it;
          continue;
        }

        // Insert before
        it = InsertAt(it, Debug::CreateIRDumper());
        ++it; // Skip what we inserted.
      }

      ++it; // Skip current pass.
      if (PassManagerDumpIR() & FEXCore::Config::PassManagerDumpIR::AFTERPASS) {
        // Insert after
        it = InsertAt(it, Debug::CreateIRDumper());
        ++it; // Skip what we inserted.
      }
    }
  }
  if (PassManagerDumpIR() & FEXCore::Config::PassManagerDumpIR::AFTEROPT) {
    if (!(PassManagerDumpIR() & FEXCore::Config::PassManagerDumpIR::AFTERPASS)) {
      // Insert final IRDumper.
      InsertAt(Passes.end(), Debug::CreateIRDumper());
    }
  }
}

void PassManager::AddDefaultPasses(FEXCore::Context::ContextImpl* ctx, bool InlineConstants) {
  FEX_CONFIG_OPT(DisablePasses, O0);

  if (!DisablePasses()) {
    InsertPass(CreateContextLoadStoreElimination(ctx->HostFeatures.SupportsAVX));

    if (Is64BitMode()) {
      // This needs to run after RCLSE
      // This only matters for 64-bit code since these instructions don't exist in 32-bit
      InsertPass(CreateLongDivideEliminationPass());
    }

    InsertPass(CreateDeadStoreElimination(ctx->HostFeatures.SupportsAVX));
    InsertPass(CreatePassDeadCodeElimination());
    InsertPass(CreateConstProp(InlineConstants, ctx->HostFeatures.SupportsTSOImm9, Is64BitMode()));

    InsertPass(CreateDeadFlagCalculationEliminination());

    InsertPass(CreateInlineCallOptimization(&ctx->CPUID));
    InsertPass(CreatePassDeadCodeElimination());
  }

  // If the IR is compacted post-RA then the node indexing gets messed up and the backend isn't able to find the register assigned to a node
  // Compact before IR, don't worry about RA generating spills/fills
  InsertPass(CreateIRCompaction(ctx->OpDispatcherAllocator), "Compaction");
}

void PassManager::AddDefaultValidationPasses() {
#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
  InsertValidationPass(Validation::CreateIRValidation(), "IRValidation");
  InsertValidationPass(Validation::CreateRAValidation());
  InsertValidationPass(Validation::CreateValueDominanceValidation());
#endif
}

void PassManager::InsertRegisterAllocationPass(bool SupportsAVX) {
  InsertPass(IR::CreateRegisterAllocationPass(GetPass("Compaction"), SupportsAVX), "RA");
}

bool PassManager::Run(IREmitter* IREmit) {
  FEXCORE_PROFILE_SCOPED("PassManager::Run");

  bool Changed = false;
  for (const auto& Pass : Passes) {
    Changed |= Pass->Run(IREmit);
  }

#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
  for (const auto& Pass : ValidationPasses) {
    Changed |= Pass->Run(IREmit);
  }
#endif

  return Changed;
}
} // namespace FEXCore::IR
