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
  static constexpr IR::NodeID ClobberedValue {0xffff'fffd};
  static constexpr IR::NodeID StaticAssigned {0xffff'ff00};

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

  // Consume (and return) the SSA id currently in a spill slot
  IR::NodeID Unspill(uint32_t SpillSlot) {
    if (Spills.contains(SpillSlot)) {
      const auto Value = Spills[SpillSlot];
      Spills.erase(SpillSlot);
      return Value;
    }
    return UninitializedValue;
  }

  // Intersect another regstate with this one
  // Any registers/slots which contain the same SSA id will be persevered
  // Anything else will be marked as Clobbered
  //
  // Useful for merging two branches of control flow.
  // Any register that differs depending on control flow shouldn't be consumed by
  // code that follows
  void Intersect(RegState& other) {
    for (size_t i = 0; i < GPRs.size(); i++) {
      if (GPRs[i] != other.GPRs[i]) {
        GPRs[i] = ClobberedValue;
      }
    }

    for (size_t i = 0; i < GPRsFixed.size(); i++) {
      if (GPRsFixed[i] != other.GPRsFixed[i]) {
        GPRsFixed[i] = ClobberedValue;
      }
    }

    for (size_t i = 0; i < FPRs.size(); i++) {
      if (FPRs[i] != other.FPRs[i]) {
        FPRs[i] = ClobberedValue;
      }
    }

    for (size_t i = 0; i < FPRsFixed.size(); i++) {
      if (FPRsFixed[i] != other.FPRsFixed[i]) {
        FPRsFixed[i] = ClobberedValue;
      }
    }

    for (auto it = Spills.begin(); it != Spills.end(); it++) {
      auto& [SlotID, Value] = *it;
      if (!other.Spills.contains(SlotID)) {
        Spills.erase(it);
      } else if (Value != other.Spills[SlotID]) {
        Value = ClobberedValue;
      }
    }
  }

  // Filter out all registers/slots containing an SSA id larger than MaxSSA
  // Mark them as Clobbered.
  // Useful for backwards edges, where using an SSA from before the
  void Filter(IR::NodeID MaxSSA) {
    for (auto& gpr : GPRs) {
      if (gpr > MaxSSA) {
        gpr = ClobberedValue;
      }
    }

    for (auto& gpr : GPRsFixed) {
      if (gpr > MaxSSA) {
        gpr = ClobberedValue;
      }
    }

    for (auto& fpr : FPRs) {
      if (fpr > MaxSSA) {
        fpr = ClobberedValue;
      }
    }

    for (auto& fpr : FPRsFixed) {
      if (fpr > MaxSSA) {
        fpr = ClobberedValue;
      }
    }

    for (auto it = Spills.begin(); it != Spills.end(); it++) {
      auto& [SlotID, Value] = *it;
      if (Value > MaxSSA) {
        Spills.erase(it);
      }
    }
  }

private:
  std::array<IR::NodeID, 32> GPRsFixed = {};
  std::array<IR::NodeID, 32> FPRsFixed = {};
  std::array<IR::NodeID, 32> GPRs = {};
  std::array<IR::NodeID, 32> FPRs = {};

  fextl::unordered_map<uint32_t, IR::NodeID> Spills;

public:
  uint32_t Version {}; // Used to force regeneration of RegStates after following backward edges
};

class RAValidation final : public FEXCore::IR::Pass {
public:
  ~RAValidation() {}
  bool Run(IREmitter* IREmit) override;

private:
  // Holds the calculated RegState at the exit of each block
  fextl::unordered_map<IR::NodeID, RegState> BlockExitState;

  // A queue of blocks we need to visit (or revisit)
  fextl::deque<OrderedNode*> BlocksToVisit;
};


bool RAValidation::Run(IREmitter* IREmit) {
  if (!Manager->HasPass("RA")) {
    return false;
  }

  FEXCORE_PROFILE_SCOPED("PassManager::RAValidation");

  IR::RegisterAllocationData* RAData = Manager->GetPass<IR::RegisterAllocationPass>("RA")->GetAllocationData();
  BlockExitState.clear();
  // BlocksToVisit will already be empty

  // Get the control flow graph from the validation pass
  auto ValidationPass = Manager->GetPass<IRValidation>("IRValidation");
  LOGMAN_THROW_AA_FMT(ValidationPass != nullptr, "Couldn't find IRValidation pass");

  auto& OffsetToBlockMap = ValidationPass->OffsetToBlockMap;

  LOGMAN_THROW_AA_FMT(ValidationPass->EntryBlock != nullptr, "No entry point");
  BlocksToVisit.push_front(ValidationPass->EntryBlock); // Currently only a single entry point

  bool HadError = false;
  fextl::ostringstream Errors;

  auto CurrentIR = IREmit->ViewIR();
  uint32_t CurrentVersion = 1; // Incremented every backwards edge

  while (!BlocksToVisit.empty()) {
    auto BlockNode = BlocksToVisit.front();
    const auto BlockID = CurrentIR.GetID(BlockNode);
    auto& BlockInfo = OffsetToBlockMap[BlockID];

    const auto IsFowardsEdge = [&](IR::NodeID PredecessorID) {
      // Blocks are sorted in FEXes IR, so backwards edges always go to a lower (or equal) Block ID
      return PredecessorID < BlockID;
    };

    // First, make sure we have the exit state for all Predecessors that
    // get here via a forwards branch.
    bool MissingPredecessor = false;

    for (auto Predecessor : BlockInfo.Predecessors) {
      const auto PredecessorID = CurrentIR.GetID(Predecessor);
      const bool HaveState = BlockExitState.contains(PredecessorID) && BlockExitState[PredecessorID].Version == CurrentVersion;

      if (IsFowardsEdge(PredecessorID) && !HaveState) {
        // We are probably about to visit this node anyway, remove it
        auto it = std::remove(BlocksToVisit.begin(), BlocksToVisit.end(), Predecessor);
        BlocksToVisit.erase(it, BlocksToVisit.end());

        // Add the missing predecessor to start of queue
        BlocksToVisit.push_front(Predecessor);
        MissingPredecessor = true;
      }
    }

    if (MissingPredecessor) {
      // We'll have to come back to this block later
      continue;
    }

    // We have committed to processing this block
    // Remove from queue
    BlocksToVisit.pop_front();

    bool FirstVisit = !BlockExitState.contains(BlockID);

    // Second, we need to determine the register status as of Block entry
    auto BlockOp = CurrentIR.GetOp<IROp_CodeBlock>(BlockNode);
    const auto FirstSSA = BlockOp->Begin.ID();

    auto& BlockRegState = BlockExitState.try_emplace(BlockID).first->second;
    bool EmptyRegState = true;
    auto Intersect = [&](RegState& Other) {
      if (EmptyRegState) {
        BlockRegState = Other;
        EmptyRegState = false;
      } else {
        BlockRegState.Intersect(Other);
      }
    };

    for (auto Predecessor : BlockInfo.Predecessors) {
      auto PredecessorID = CurrentIR.GetID(Predecessor);
      if (BlockExitState.contains(PredecessorID)) {
        if (IsFowardsEdge(PredecessorID)) {
          Intersect(BlockExitState[PredecessorID]);
        } else {
          RegState Filtered = BlockExitState[PredecessorID];
          Filtered.Filter(FirstSSA);
          Intersect(Filtered);
        }
      }
    }

    // Third, we need to iterate over all IR ops in the block

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
        } else if (CurrentSSAAtReg == RegState::ClobberedValue) {
          HadError |= true;

          Errors << fextl::fmt::format("%{}: Arg[{}] expects reg{} to contain %{}, but contents vary depending on control flow\n", ID, i,
                                       PhyReg.Reg, ArgID);
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
        //       In the future we need to prove it contains the correct SSA value

#if 0
        if (Value == RegState::UninitializedValue) {
          HadError |= true;
          Errors << fextl::fmt::format("%{}: FillRegister expected %{} in Slot {}, but was undefined in at least one control flow path\n",
                                       ID, ExpectedValue, FillRegister->Slot);
        } else if (Value == RegState::ClobberedValue) {
          HadError |= true;
          Errors << fextl::fmt::format("%{}: FillRegister expected %{} in Slot {}, but contents vary depending on control flow\n", ID,
                                       ExpectedValue, FillRegister->Slot);
        } else if (Value != ExpectedValue) {
          HadError |= true;
          Errors << fextl::fmt::format("%{}: FillRegister expected %{} in Slot {}, but it actually contains %{}\n", ID, ExpectedValue,
                                       FillRegister->Slot, Value);
        }
#endif
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

    // Forth, Add successors to the queue of blocks to validate
    for (auto Successor : BlockInfo.Successors) {
      auto SuccessorID = CurrentIR.GetID(Successor);

      // Blocks are sorted in FEXes IR, so backwards edges always go to a lower (or equal) Block ID
      bool FowardsEdge = SuccessorID > BlockID;

      if (FowardsEdge) {
        // Always follow forwards edges, assuming it's not already on the queue
        if (std::find(BlocksToVisit.begin(), BlocksToVisit.end(), Successor) == std::end(BlocksToVisit)) {
          // Push to the back of queue so there is a higher chance all predecessors for this block are done first
          BlocksToVisit.push_back(Successor);
        }
      } else if (FirstVisit) {
        // Now that we have the block data for the backwards edge, we can visit it again and make
        // sure it (and all it's successors) are still valid.

        // But only do this the first time we encounter each backwards edge.

        // Push to the front of queue, so we get this re-checking done before examining future nodes.
        BlocksToVisit.push_front(Successor);

        // Make sure states are reprocessed
        CurrentVersion++;
      }
    }

    BlockRegState.Version = CurrentVersion;

    if (CurrentVersion > 10000) {
      Errors << "Infinite Loop\n";
      HadError |= true;

      for (auto [BlockNode, BlockHeader] : CurrentIR.GetBlocks()) {
        const auto BlockID = CurrentIR.GetID(BlockNode);
        const auto& BlockInfo = OffsetToBlockMap[BlockID];

        Errors << fextl::fmt::format("Block {}\n\tPredecessors: ", BlockID);

        for (auto Predecessor : BlockInfo.Predecessors) {
          const auto PredecessorID = CurrentIR.GetID(Predecessor);
          const bool FowardsEdge = PredecessorID < BlockID;
          if (!FowardsEdge) {
            Errors << "(Backwards): ";
          }
          Errors << fextl::fmt::format("Block {} ", PredecessorID);
        }

        Errors << "\n\tSuccessors: ";

        for (auto Successor : BlockInfo.Successors) {
          const auto SuccessorID = CurrentIR.GetID(Successor);
          const bool FowardsEdge = SuccessorID > BlockID;

          if (!FowardsEdge) {
            Errors << "(Backwards): ";
          }
          Errors << fextl::fmt::format("Block {} ", SuccessorID);
        }

        Errors << "\n\n";
      }

      break;
    }
  }

  if (HadError) {
    fextl::stringstream IrDump;
    FEXCore::IR::Dump(&IrDump, &CurrentIR, RAData);

    LogMan::Msg::EFmt("RA Validation Error\n{}\nErrors:\n{}\n", IrDump.str(), Errors.str());
    LOGMAN_MSG_A_FMT("Encountered RA validation Error");

    Errors.clear();
  }

  return false;
}

fextl::unique_ptr<FEXCore::IR::Pass> CreateRAValidation() {
  return fextl::make_unique<RAValidation>();
}
} // namespace FEXCore::IR::Validation
