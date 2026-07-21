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
  if (PassManagerDumpIR() & FEXCore::Config::PassManagerDumpIR::BEFOREOPT) {
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

void PassManager::AddDefaultPasses(Context::ContextImpl* ctx) {
  FEX_CONFIG_OPT(DisablePasses, O0);

  // We only specifically disable optimization passes if desired, as IR output should
  // still be well-formed regardless of the modifications made to it.
  if (!DisablePasses()) {
    InsertPass(CreateX87StackOptimizationPass(ctx->HostFeatures, ctx->Config.Is64BitMode ? IR::OpSize::i64Bit : IR::OpSize::i32Bit));
    InsertPass(CreateDeadFlagCalculationEliminination());
  }

  InsertPass(IR::CreateRegisterAllocationPass(&ctx->CPUID), "RA");

#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
  InsertValidationPass(Validation::CreateIRValidation(), "IRValidation");
#endif
}

Pass* PassManager::InsertPass(fextl::unique_ptr<Pass> Pass, fextl::string Name) {
  auto PassPtr = InsertAt(Passes.end(), std::move(Pass))->get();

  if (!Name.empty()) {
    NameToPassMaping[Name] = PassPtr;
  }
  return PassPtr;
}

PassManager::PassArrayType::iterator PassManager::InsertAt(PassArrayType::iterator pos, fextl::unique_ptr<Pass> Pass) {
  Pass->RegisterPassManager(this);
  return Passes.insert(pos, std::move(Pass));
}

#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
void PassManager::InsertValidationPass(fextl::unique_ptr<Pass> Pass, fextl::string Name) {
  Pass->RegisterPassManager(this);
  auto PassPtr = ValidationPasses.emplace_back(std::move(Pass)).get();

  if (!Name.empty()) {
    NameToPassMaping[Name] = PassPtr;
  }
}
#endif

void PassManager::Run(IREmitter* IREmit) {
  FEXCORE_PROFILE_SCOPED("PassManager::Run");

  for (const auto& Pass : Passes) {
    Pass->Run(IREmit);
  }

#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
  for (const auto& Pass : ValidationPasses) {
    Pass->Run(IREmit);
  }
#endif
}
} // namespace FEXCore::IR
