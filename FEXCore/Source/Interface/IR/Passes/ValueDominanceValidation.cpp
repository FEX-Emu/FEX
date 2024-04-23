// SPDX-License-Identifier: MIT
/*
$info$
tags: ir|opts
desc: Sanity Checking
$end_info$
*/

#include "Interface/IR/IR.h"
#include "Interface/IR/IREmitter.h"
#include "Interface/IR/PassManager.h"

#include <FEXCore/IR/IR.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/Profiler.h>
#include <FEXCore/fextl/set.h>
#include <FEXCore/fextl/sstream.h>
#include <FEXCore/fextl/unordered_map.h>
#include <FEXCore/fextl/vector.h>

#include <functional>
#include <memory>
#include <stdint.h>
#include <utility>

namespace FEXCore::IR::Validation {
class ValueDominanceValidation final : public FEXCore::IR::Pass {
public:
  bool Run(IREmitter* IREmit) override;
};

bool ValueDominanceValidation::Run(IREmitter* IREmit) {
  FEXCORE_PROFILE_SCOPED("PassManager::ValueDominanceValidation");

  bool HadError = false;
  auto CurrentIR = IREmit->ViewIR();

  fextl::ostringstream Errors;

  for (auto [BlockNode, BlockHeader] : CurrentIR.GetBlocks()) {
    auto BlockIROp = BlockHeader->CW<FEXCore::IR::IROp_CodeBlock>();

    for (auto [CodeNode, IROp] : CurrentIR.GetCode(BlockNode)) {
      const auto CodeID = CurrentIR.GetID(CodeNode);

      const uint8_t NumArgs = IR::GetRAArgs(IROp->Op);
      for (uint32_t i = 0; i < NumArgs; ++i) {
        if (IROp->Args[i].IsInvalid()) {
          continue;
        }

        // We do not validate the location of inline constants because it's
        // irrelevant, they're ignored by RA and always inlined to where they
        // need to be. This lets us pool inline constants globally.
        IROps Op = CurrentIR.GetOp<IROp_Header>(IROp->Args[i])->Op;
        if (Op == OP_IRHEADER || Op == OP_INLINECONSTANT) {
          continue;
        }

        OrderedNodeWrapper Arg = IROp->Args[i];

        // If the SSA argument is not defined INSIDE the block, we have
        // cross-block liveness, which we forbid in the IR to simplify RA.
        if (!(Arg.ID() >= BlockIROp->Begin.ID() && Arg.ID() < BlockIROp->Last.ID())) {
          // HadError |= true;
          // Errors << "Inst %" << CodeID << ": Arg[" << i << "] %" << Arg.ID() << " definition not local!" << std::endl;
          continue;
        }

        // The SSA argument is defined INSIDE this block.
        // It must only be declared prior to this instruction
        // Eg: Valid
        // CodeBlock_1:
        // %_1 = Load
        // %_2 = Load
        // %_3 = <Op> %_1, %_2
        //
        // Eg: Invalid
        // CodeBlock_1:
        // %_1 = Load
        // %_2 = <Op> %_1, %_3
        // %_3 = Load
        if (Arg.ID() > CodeID) {
          HadError |= true;
          Errors << "Inst %" << CodeID << ": Arg[" << i << "] %" << Arg.ID() << " definition does not dominate this use!" << std::endl;
        }
      }
    }
  }

  if (HadError) {
    fextl::stringstream Out;
    FEXCore::IR::Dump(&Out, &CurrentIR, nullptr);
    Out << "Errors:" << std::endl << Errors.str() << std::endl;
    LogMan::Msg::EFmt("{}", Out.str());
    LOGMAN_MSG_A_FMT("Encountered IR validation Error");
  }

  return false;
}

fextl::unique_ptr<FEXCore::IR::Pass> CreateValueDominanceValidation() {
  return fextl::make_unique<ValueDominanceValidation>();
}

} // namespace FEXCore::IR::Validation
