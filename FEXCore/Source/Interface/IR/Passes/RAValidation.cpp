// SPDX-License-Identifier: MIT

#include "Interface/IR/IR.h"
#include "Interface/IR/IREmitter.h"
#include "Interface/IR/PassManager.h"
#include "Interface/IR/RegisterAllocationData.h"
#include "Interface/IR/Passes/IRValidation.h"
#include "Interface/IR/Passes/RegisterAllocationPass.h"

#include <FEXCore/IR/IR.h>
#include <FEXCore/Utils/Profiler.h>
#include <FEXCore/fextl/deque.h>
#include <FEXCore/fextl/fmt.h>
#include <FEXCore/fextl/sstream.h>
#include <FEXCore/fextl/unordered_map.h>

#include <algorithm>

namespace FEXCore::IR::Validation {

// Hold the mapping of physical registers to the SSA id it holds at any given point in the IR
struct RegState {
  static constexpr IR::NodeID UninitializedValue {0};
  static constexpr IR::NodeID InvalidReg {0xffff'ffff};
  static constexpr IR::NodeID CorruptedPair {0xffff'fffe};

  // This class makes some assumptions about how the host registers are arranged and mapped to virtual registers:
  // 1. There will be less than 32 GPRs and 32 FPRs
  // 2. If the GPRFixed class is used, there will be 16 GPRs and 16 FixedGPRs max
  // 3. Same with FPRFixed
  // 4. If the GPRPairClass is used, it is assumed each GPRPair N will map onto GPRs N and N + 1

  // These assumptions were all true for the state of the arm64 and x86 jits at the time this was written

  // Mark a physical register as containing a SSA id
  bool Set(PhysicalRegister Reg, IR::NodeID ssa) {
    LOGMAN_THROW_A_FMT(ssa.IsValid(), "RegState assumes ssa0 will be the block header and never assigned to a register");

    // PhysicalRegisters aren't fully mapped until assembly emission
    // We need to apply a generic mapping here to catch any aliasing
    switch (Reg.Class) {
    case GPRClass: GPRs[Reg.Reg] = ssa; return true;
    case GPRFixedClass:
      // On arm64, there are 16 Fixed and 9 normal
      GPRsFixed[Reg.Reg] = ssa;
      return true;
    case FPRClass: FPRs[Reg.Reg] = ssa; return true;
    case FPRFixedClass:
      // On arm64, there are 16 Fixed and 12 normal
      FPRsFixed[Reg.Reg] = ssa;
      return true;
    case GPRPairClass:
      // Alias paired registers onto both
      GPRs[Reg.Reg] = ssa;
      GPRs[Reg.Reg + 1] = ssa;
      return true;
    }
    return false;
  }

  // Get the current SSA id
  // Or an error value there isn't a (sane) SSA id
  IR::NodeID Get(PhysicalRegister Reg) const {
    switch (Reg.Class) {
    case GPRClass: return GPRs[Reg.Reg];
    case GPRFixedClass: return GPRsFixed[Reg.Reg];
    case FPRClass: return FPRs[Reg.Reg];
    case FPRFixedClass: return FPRsFixed[Reg.Reg];
    case GPRPairClass:
      // Make sure both halves of the Pair contain the same SSA
      if (GPRs[Reg.Reg] == GPRs[Reg.Reg + 1]) {
        return GPRs[Reg.Reg];
      }
      return CorruptedPair;
    }
    return InvalidReg;
  }

  // Mark a spill slot as containing a SSA id
  void Spill(uint32_t SpillSlot, IR::NodeID ssa) {
    Spills[SpillSlot] = ssa;
  }

  // Return the SSA id currently in a spill slot
  IR::NodeID Unspill(uint32_t SpillSlot) {
    if (Spills.contains(SpillSlot)) {
      return Spills[SpillSlot];
    } else {
      return UninitializedValue;
    }
  }

private:
  std::array<IR::NodeID, 32> GPRsFixed = {};
  std::array<IR::NodeID, 32> FPRsFixed = {};
  std::array<IR::NodeID, 32> GPRs = {};
  std::array<IR::NodeID, 32> FPRs = {};

  fextl::unordered_map<uint32_t, IR::NodeID> Spills;
};

class RAValidation final : public FEXCore::IR::Pass {
public:
  ~RAValidation() {}
  void Run(IREmitter* IREmit) override;
};


void RAValidation::Run(IREmitter* IREmit) {
  if (!Manager->HasPass("RA")) {
    return;
  }

  FEXCORE_PROFILE_SCOPED("PassManager::RAValidation");

  IR::RegisterAllocationData* RAData = Manager->GetPass<IR::RegisterAllocationPass>("RA")->GetAllocationData();

  bool HadError = false;
  fextl::ostringstream Errors;

  auto CurrentIR = IREmit->ViewIR();

  for (auto [BlockNode, BlockIROp] : CurrentIR.GetBlocks()) {
    // We only allocate registers locally, so state is reset each block
    struct RegState BlockRegState = {};

    for (auto [CodeNode, IROp] : CurrentIR.GetCode(BlockNode)) {
      const auto ID = CurrentIR.GetID(CodeNode);

      const auto CheckArg = [&](uint32_t i, OrderedNodeWrapper Arg) {
        const auto ArgID = Arg.ID();
        const auto PhyReg = RAData->GetNodeRegister(ArgID);

        if (PhyReg.IsInvalid()) {
          return;
        }

        auto CurrentSSAAtReg = BlockRegState.Get(PhyReg);
        if (CurrentSSAAtReg == RegState::InvalidReg) {
          HadError |= true;
          Errors << fextl::fmt::format("%{}: Arg[{}] unknown Reg: {}, class: {}\n", ID, i, PhyReg.Reg, PhyReg.Class);
        } else if (CurrentSSAAtReg == RegState::CorruptedPair) {
          HadError |= true;

          auto Lower = BlockRegState.Get(PhysicalRegister(GPRClass, uint8_t(PhyReg.Reg * 2) + 1));
          auto Upper = BlockRegState.Get(PhysicalRegister(GPRClass, PhyReg.Reg * 2 + 1));

          Errors << fextl::fmt::format("%{}: Arg[{}] expects paired reg{} to contain %{}, but it actually contains {{%{}, %{}}}\n", ID, i,
                                       PhyReg.Reg, ArgID, Lower, Upper);
        } else if (CurrentSSAAtReg == RegState::UninitializedValue) {
          HadError |= true;

          Errors << fextl::fmt::format("%{}: Arg[{}] expects reg{} to contain %{}, but it is uninitialized\n", ID, i, PhyReg.Reg, ArgID);
        } else if (CurrentSSAAtReg != ArgID) {
          HadError |= true;
          Errors << fextl::fmt::format("%{}: Arg[{}] expects reg{} to contain %{}, but it actually contains %{}\n", ID, i, PhyReg.Reg,
                                       ArgID, CurrentSSAAtReg);
        }
      };

      switch (IROp->Op) {
      case OP_SPILLREGISTER: {
        auto SpillRegister = IROp->C<IROp_SpillRegister>();
        CheckArg(0, SpillRegister->Value);

        BlockRegState.Spill(SpillRegister->Slot, SpillRegister->Value.ID());
        break;
      }

      case OP_FILLREGISTER: {
        auto FillRegister = IROp->C<IROp_FillRegister>();
        const auto ExpectedValue = FillRegister->OriginalValue.ID();
        const auto Value = BlockRegState.Unspill(FillRegister->Slot);

        // TODO: This only proves that the Spill has a consistent SSA value
        // In the future we need to prove it contains the correct SSA value. For
        // this we need to analyze copies/swaps properly. As a hot fix, don't
        // compare Value with ExpectedValue.

        if (Value == RegState::UninitializedValue) {
          HadError |= true;
          Errors << fextl::fmt::format("%{}: FillRegister expected %{} in Slot {}, but was undefined\n", ID, ExpectedValue, FillRegister->Slot);
        }
        break;
      }

      default: {
        // And check that all args point at the correct SSA
        uint8_t NumArgs = IR::GetArgs(IROp->Op);
        for (uint32_t i = 0; i < NumArgs; ++i) {
          CheckArg(i, IROp->Args[i]);
        }
        break;
      }
      }

      // Update BlockState map
      if (IROp->Op != OP_SPILLREGISTER) {
        BlockRegState.Set(RAData->GetNodeRegister(ID), ID);
      }
    }
  }

  if (HadError) {
    fextl::stringstream IrDump;
    FEXCore::IR::Dump(&IrDump, &CurrentIR, RAData);

    LogMan::Msg::EFmt("RA Validation Error\n{}\nErrors:\n{}\n", IrDump.str(), Errors.str());
    LOGMAN_MSG_A_FMT("Encountered RA validation Error");

    Errors.clear();
  }
}

fextl::unique_ptr<FEXCore::IR::Pass> CreateRAValidation() {
  return fextl::make_unique<RAValidation>();
}
} // namespace FEXCore::IR::Validation
