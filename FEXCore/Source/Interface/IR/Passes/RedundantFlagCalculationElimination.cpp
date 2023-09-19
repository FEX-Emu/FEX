// SPDX-License-Identifier: MIT
/*
$info$
tags: ir|opts
desc: This is not used right now, possibly broken
$end_info$
*/

#include <FEXCore/IR/IR.h>
#include <FEXCore/IR/IREmitter.h>
#include <FEXCore/IR/IntrusiveIRList.h>
#include <FEXCore/Utils/Profiler.h>

#include "Interface/IR/PassManager.h"

#include <array>
#include <memory>

namespace FEXCore::IR {

class DeadFlagCalculationEliminination final : public FEXCore::IR::Pass {
public:
  bool Run(IREmitter *IREmit) override;
};

/**
 * @brief (UNSAFE) This pass removes flag calculations that will otherwise be unused INSIDE of that block
 *
 * Compilers don't really do any form of cross-block flag allocation like they do RA with GPRs.
 * This ends up with them recalculating flags across blocks regardless of if it is actually possible to reuse the flags.
 * This is an additional burden in x86 that most instructions change flags when called, so it is easier to recalculate anyway.
 *
 * This is unsafe since handwritten code can easily break this assumption.
 * This may be more interesting with full function level recompilation since flags definitely won't be used across function boundaries.
 *
 */
bool DeadFlagCalculationEliminination::Run(IREmitter *IREmit) {
  FEXCORE_PROFILE_SCOPED("PassManager::DFE");

  std::array<OrderedNode*, 32> LastValidFlagStores{};

  bool Changed = false;
  auto CurrentIR = IREmit->ViewIR();

  for (auto [BlockNode, BlockHeader] : CurrentIR.GetBlocks()) {
    for (auto [CodeNode, IROp] : CurrentIR.GetCode(BlockNode)) {
      if (IROp->Op == OP_STOREFLAG) {
        auto Op = IROp->CW<IR::IROp_StoreFlag>();
        // Set this node as the last one valid for this flag
        LastValidFlagStores[Op->Flag] = CodeNode;
      }
      else if (IROp->Op == OP_LOADFLAG) {
        auto Op = IROp->CW<IR::IROp_LoadFlag>();
        LastValidFlagStores[Op->Flag] = nullptr;
      }
    }

    // If any flags are stored but not loaded by the end of the block, then erase them
    for (auto &Flag : LastValidFlagStores) {
      if (Flag != nullptr) {
        IREmit->Remove(Flag);
        Changed = true;
      }
    }

    LastValidFlagStores.fill(nullptr);
  }

  return Changed;
}

fextl::unique_ptr<FEXCore::IR::Pass> CreateDeadFlagCalculationEliminination() {
  return fextl::make_unique<DeadFlagCalculationEliminination>();
}

}
