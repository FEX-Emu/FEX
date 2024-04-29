// SPDX-License-Identifier: MIT
/*
$info$
tags: ir|opts
$end_info$
*/

#include "Interface/IR/Passes/RegisterAllocationPass.h"
#include "Interface/IR/IR.h"
#include "Interface/IR/IREmitter.h"
#include "Interface/IR/RegisterAllocationData.h"
#include "Interface/IR/Passes.h"
#include <FEXCore/IR/IR.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/Profiler.h>
#include <FEXCore/fextl/vector.h>
#include <bit>
#include <cstdint>

namespace FEXCore::IR {
namespace {
  constexpr uint32_t INVALID_REG = FEXCore::IR::InvalidReg;
  constexpr uint32_t INVALID_CLASS = FEXCore::IR::InvalidClass.Val;
  constexpr uint32_t EVEN_BITS = 0x55555555;

  struct RegisterClass {
    uint32_t Available;
    uint32_t Count;

    // If bit R of Available is 0, then RegToSSA[R] is the node
    // currently allocated to R.
    //
    // This is the Old version of the node. (TODO: Consider relaxing this for
    // performance?)
    //
    // Else, RegToSSA[R] is UNDEFINED. This means we don't need to clear this
    // when clearing bits from Available.
    OrderedNode* RegToSSA[32];
  };

  struct RegisterSet {
    fextl::vector<RegisterClass> Classes;
    uint32_t ClassCount;
  };

  struct RegisterGraph : public FEXCore::Allocator::FEXAllocOperators {
    IR::RegisterAllocationData::UniquePtr AllocData;
    RegisterSet Set;
  };

  RegisterGraph* AllocateRegisterGraph(uint32_t ClassCount) {
    RegisterGraph* Graph = new RegisterGraph {};

    // Allocate the register set
    Graph->Set.ClassCount = ClassCount;
    Graph->Set.Classes.resize(ClassCount);

    return Graph;
  }

  FEXCore::IR::RegisterClassType GetRegClassFromNode(FEXCore::IR::IRListView* IR, FEXCore::IR::IROp_Header* IROp) {
    using namespace FEXCore;

    FEXCore::IR::RegisterClassType Class = IR::GetRegClass(IROp->Op);
    if (Class != FEXCore::IR::ComplexClass) {
      return Class;
    }

    // Complex register class handling
    switch (IROp->Op) {
    case IR::OP_LOADCONTEXT: return IROp->C<IR::IROp_LoadContext>()->Class;
    case IR::OP_LOADREGISTER: return IROp->C<IR::IROp_LoadRegister>()->Class;
    case IR::OP_LOADCONTEXTINDEXED: return IROp->C<IR::IROp_LoadContextIndexed>()->Class;
    case IR::OP_LOADMEM:
    case IR::OP_LOADMEMTSO: return IROp->C<IR::IROp_LoadMem>()->Class;
    case IR::OP_FILLREGISTER: return IROp->C<IR::IROp_FillRegister>()->Class;
    default: return FEXCore::IR::InvalidClass;
    }
  };
} // Anonymous namespace

class ConstrainedRAPass final : public RegisterAllocationPass {
public:
  ConstrainedRAPass(bool SupportsAVX);
  ~ConstrainedRAPass();
  bool Run(IREmitter* IREmit) override;

  void AllocateRegisterSet(uint32_t ClassCount) override;
  void AddRegisters(FEXCore::IR::RegisterClassType Class, uint32_t RegisterCount) override;

  RegisterAllocationData* GetAllocationData() override;
  RegisterAllocationData::UniquePtr PullAllocationData() override;

private:
  RegisterGraph* Graph;
  bool SupportsAVX;
};

ConstrainedRAPass::ConstrainedRAPass(bool _SupportsAVX)
  : SupportsAVX {_SupportsAVX} {}

ConstrainedRAPass::~ConstrainedRAPass() {
  delete Graph;
}

void ConstrainedRAPass::AllocateRegisterSet(uint32_t ClassCount) {
  LOGMAN_THROW_AA_FMT(ClassCount <= INVALID_CLASS, "Up to {} classes supported", INVALID_CLASS);

  Graph = AllocateRegisterGraph(ClassCount);
}

void ConstrainedRAPass::AddRegisters(FEXCore::IR::RegisterClassType Class, uint32_t RegisterCount) {
  LOGMAN_THROW_AA_FMT(RegisterCount <= INVALID_REG, "Up to {} regs supported", INVALID_REG);

  Graph->Set.Classes[Class].Count = RegisterCount;
}

RegisterAllocationData* ConstrainedRAPass::GetAllocationData() {
  return Graph->AllocData.get();
}

RegisterAllocationData::UniquePtr ConstrainedRAPass::PullAllocationData() {
  return std::move(Graph->AllocData);
}


#define foreach_arg(IROp, index, arg) \
  for (auto [index, Arg] = std::tuple(0, IROp->Args[0]); index < IR::GetRAArgs(IROp->Op); Arg = IROp->Args[++index]) \

#define foreach_valid_arg(IROp, index, arg) \
  for (auto [index, Arg] = std::tuple(0, IROp->Args[0]); index < IR::GetRAArgs(IROp->Op); Arg = IROp->Args[++index]) \
    if (IsValidArg(Arg))

bool ConstrainedRAPass::Run(IREmitter* IREmit) {
  FEXCORE_PROFILE_SCOPED("PassManager::RA");

  using namespace FEXCore;

  auto IR = IREmit->ViewIR();

  // SSA def remapping. FEX's original RA could only assign a single register to
  // a given def for its entire live range, and this limitation is baked deep
  // into the IR. However, we need to be able to split live ranges to
  // implement register pairs and spilling properly.
  //
  // To reconcile these two worlds, we'll generate new SSA nodes when we split
  // live ranges, and remap SSA sources accordingly. This is a bit annoying,
  // because it means SSAToReg can grow, but it avoids disturbing the rest of
  // FEX (for now).
  //
  // We define "Old" nodes as nodes present in the original IR, and "New" nodes
  // as nodes added to split live ranges. Helpful properties:
  //
  // - A node is Old <===> it is not New
  // - A node is Old <===> its ID < IR.GetSSACount() at the start
  // - All sources are Old before remapping an instruction
  //
  // Down the road, we might want to optimize this but I'm not prepared to
  // bulldoze the IR for this.
  //
  // This data structure tracks the current remapping. nullptr indicates no
  // remapping.
  //
  // Unlike SSAToReg, this data structure does not grow, since it only tracks
  // SSA defs that were there before we started splitting live ranges.
  fextl::vector<OrderedNode*> SSAToNewSSA(IR.GetSSACount(), nullptr);

  // Inverse map of SSAToNewSSA. Along with SSAToReg, this is the only data
  // struture that grows. The other maps all track the old SSA names. This helps
  // water down the wacky.
  fextl::vector<OrderedNode*> NewSSAToSSA(IR.GetSSACount(), nullptr);

  auto IsOld = [&IR, &SSAToNewSSA](OrderedNode* Node) {
    return IR.GetID(Node).Value < SSAToNewSSA.size();
  };

  // Map of assigned registers. Grows.
  PhysicalRegister InvalidPhysReg = PhysicalRegister(InvalidClass, InvalidReg);
  fextl::vector<PhysicalRegister> SSAToReg(IR.GetSSACount(), InvalidPhysReg);

  // Return the New node (if it exists) for an Old node, else the Old node.
  auto Map = [&IR, &SSAToNewSSA, &IsOld](OrderedNode* Old) {
    LOGMAN_THROW_AA_FMT(IsOld(Old), "Pre-condition");
    auto Index = IR.GetID(Old).Value;

    return SSAToNewSSA.at(Index) ?: Old;
  };

  // Return the Old node for a possibly-remapped node.
  auto Unmap = [&IR, &NewSSAToSSA](OrderedNode* Node) {
    auto Index = IR.GetID(Node).Value;
    return NewSSAToSSA.at(Index) ?: Node;
  };

  // Record a remapping of Old to New.
  auto Remap = [&IR, &SSAToNewSSA, &NewSSAToSSA, &Map, &Unmap, &IsOld](OrderedNode* Old, OrderedNode* New) {
    LOGMAN_THROW_AA_FMT(IsOld(Old), "Pre-condition");
    LOGMAN_THROW_AA_FMT(!IsOld(New), "Pre-condition");

    auto OldID = IR.GetID(Old).Value;
    auto NewID = IR.GetID(New).Value;

    LOGMAN_THROW_AA_FMT(NewID >= NewSSAToSSA.size(), "Brand new SSA def");
    NewSSAToSSA.resize(NewID + 1, 0);

    SSAToNewSSA.at(OldID) = New;
    NewSSAToSSA.at(NewID) = Old;

    LOGMAN_THROW_AA_FMT(Map(Old) == New, "Post-condition");
    LOGMAN_THROW_AA_FMT(Unmap(New) == Old, "Post-condition");
    LOGMAN_THROW_AA_FMT(Unmap(Old) == Old, "Invariant");
  };

  // Mapping of spilled SSA defs to their assigned slot + 1, or 0 for defs that
  // have not been spilled. Persisting this mapping avoids repeated spills of
  // the same long-lived SSA value.
  fextl::vector<unsigned> SpillSlots(IR.GetSSACount(), 0);

  auto InsertFill = [&IR, &IREmit, &SpillSlots, &IsOld](OrderedNode* Old) {
    LOGMAN_THROW_AA_FMT(IsOld(Old), "Precondition");

    auto SlotPlusOne = SpillSlots.at(IR.GetID(Old).Value);
    LOGMAN_THROW_AA_FMT(SlotPlusOne >= 1, "Old must have been spilled");

    auto Header = IR.GetOp<IROp_Header>(Old);
    auto RegClass = GetRegClassFromNode(&IR, Header);

    auto Fill = IREmit->_FillRegister(Old, SlotPlusOne - 1, RegClass);
    Fill.first->Header.Size = Header->Size;
    Fill.first->Header.ElementSize = Header->ElementSize;

    return Fill;
  };

  // IP of next-use of each SSA source. IPs are measured from the end of the
  // block, so we don't need to size the block up-front.
  fextl::vector<uint32_t> NextUses(IR.GetSSACount(), 0);

  unsigned SpillSlotCount = 0;

  auto IsValidArg = [&IR](auto Arg) {
    if (Arg.IsInvalid()) {
      return false;
    }

    switch (IR.GetOp<IROp_Header>(Arg)->Op) {
    case OP_INLINECONSTANT:
    case OP_INLINEENTRYPOINTOFFSET:
    case OP_IRHEADER: return false;

    case OP_SPILLREGISTER: LOGMAN_MSG_A_FMT("should not be seen"); return false;

    default: return true;
    }
  };

  auto ClassSize = [](RegisterClassType T) {
    return (T == GPRPairClass) ? 2 : 1;
  };

  auto IsInRegisterFile = [this, &IR, &Map, &IsOld, &SSAToReg](auto Old) {
    LOGMAN_THROW_AA_FMT(IsOld(Old), "Precondition");

    auto Reg = SSAToReg.at(IR.GetID(Map(Old)).Value);
    auto Class = &Graph->Set.Classes[Reg.Class == GPRPairClass ? GPRClass : Reg.Class];

    return (!(Class->Available & (1 << Reg.Reg))) && Class->RegToSSA[Reg.Reg] == Old;
  };

  auto FreeReg = [this](auto Reg) {
    bool Pair = Reg.Class == GPRPairClass;
    auto ClassType = Pair ? GPRClass : Reg.Class;
    auto RegBits = (Pair ? 0x3 : 0x1) << Reg.Reg;

    auto Class = &Graph->Set.Classes[ClassType];
    LOGMAN_THROW_AA_FMT(!(Class->Available & RegBits), "Register double-free");
    Class->Available |= RegBits;
  };

  auto SpillReg = [&IR, &IREmit, &SpillSlotCount, &SpillSlots, &SSAToReg, &FreeReg, &Map, &Unmap, &IsOld, &NextUses](auto Class, IROp_Header *Exclude) {
    // First, find the best node to spill. We use the well-known
    // "furthest-first" heuristic, spilling the node whose next-use is the
    // farthest in the future.
    //
    // Since we defined IPs relative to the end of the block, the furthest
    // next-use has the /smallest/ unsigned IP.
    //
    // TODO: Remat.
    OrderedNode* Candidate = nullptr;
    uint32_t BestDistance = UINT32_MAX;
    uint8_t BestReg = ~0;

    for (int i = 0; i < Class->Count; ++i) {
      if (!(Class->Available & (1 << i))) {
        OrderedNode* Old = Class->RegToSSA[i];

        LOGMAN_THROW_AA_FMT(Old != nullptr, "Invariant");
        LOGMAN_THROW_AA_FMT(IsOld(Old), "Invariant");
        LOGMAN_THROW_AA_FMT(SSAToReg.at(IR.GetID(Map(Old)).Value).Reg == i, "Invariant'");

        bool Excluded = false;
        foreach_arg(Exclude, _, Arg) {
          if (Unmap(IR.GetNode(Arg)) == Old)
            Excluded = true;
        }

        if (Excluded)
          continue;

        uint32_t NextUse = NextUses.at(IR.GetID(Old).Value);
        if (NextUse < BestDistance) {
          BestDistance = NextUse;
          BestReg = i;
          Candidate = Old;
        }

        // TODO: Cleaner solution?
        auto Header = IR.GetOp<IROp_Header>(Old);
        if (GetRegClassFromNode(&IR, Header) == GPRPairClass) {
          ++i;
        }
      }
    }

    LOGMAN_THROW_AA_FMT(Candidate != nullptr, "must've found something..");
    LOGMAN_THROW_AA_FMT(IsOld(Candidate), "Invariant");

    auto Reg = SSAToReg.at(IR.GetID(Map(Candidate)).Value);
    LOGMAN_THROW_AA_FMT(Reg.Reg == BestReg, "Invariant");

    auto Header = IR.GetOp<IROp_Header>(Candidate);
    auto Value = IR.GetID(Candidate).Value;
    auto Spilled = SpillSlots.at(Value) != 0;
    printf("spilling %d = %u\n", IR.GetID(Candidate).Value, IR.GetID(Map(Candidate)).Value);

    // If we already spilled the Candidate, we don't need to spill again.
    if (!Spilled) {
      printf("   spill\n");
      LOGMAN_THROW_AA_FMT(Reg.Class == GetRegClassFromNode(&IR, Header), "Consistent");

      // TODO: we should colour spill slots
      auto Slot = SpillSlotCount++;

      // We must map here in case we're spilling something we shuffled.
      auto SpillOp = IREmit->_SpillRegister(Map(Candidate), Slot, RegisterClassType {Reg.Class});
      SpillOp.first->Header.Size = Header->Size;
      SpillOp.first->Header.ElementSize = Header->ElementSize;
      SpillSlots.at(Value) = Slot + 1;
    }

    // Now that we've spilled the value, take it out of the register file
    FreeReg(Reg);
  };

  // Record a given assignment of register Reg to Node.
  auto SetReg = [&IR, &Unmap, &SSAToReg, InvalidPhysReg](OrderedNode* Node, struct RegisterClass* Class, PhysicalRegister Reg) {
    uint32_t Index = IR.GetID(Node).Value;
    uint32_t SizeMask = (Reg.Class == GPRPairClass) ? 0b11 : 0b1;
    uint32_t RegBits = SizeMask << Reg.Reg;

    LOGMAN_THROW_AA_FMT((Class->Available & RegBits) == RegBits, "Precondition");
    Class->Available &= ~RegBits;
    Class->RegToSSA[Reg.Reg] = Unmap(Node);

    if (Index >= SSAToReg.size()) {
      SSAToReg.resize(Index + 1, InvalidPhysReg);
    }

    SSAToReg.at(Index) = Reg;
  };

  // Get the mask of avaiable registers for a given register class
  auto AvailableMask = [](auto Class, bool Pair) {
    uint32_t Available = Class->Available;

    // Limit Available to only valid base registers for pairs
    if (Pair) {
      // Only choose register R if R and R + 1 are both free.
      Available &= (Available >> 1);

      // Only consider aligned registers
      Available &= EVEN_BITS;
    }

    return Available;
  };

  // Assign a register for a given Node, spilling if necessary.
  auto AssignReg = [this, &IR, IREmit, &SpillReg, &ClassSize, &Remap, &FreeReg, &SetReg, &Map, &AvailableMask](OrderedNode* CodeNode, IROp_Header *Pivot) {
    const auto Node = IR.GetID(CodeNode);
    const auto IROp = IR.GetOp<IROp_Header>(CodeNode);

    LOGMAN_THROW_AA_FMT(Node.IsValid(), "Node must be valid");

    auto OrigClassType = GetRegClassFromNode(&IR, IROp);
    bool Pair = OrigClassType == GPRPairClass;
    auto ClassType = Pair ? GPRClass : OrigClassType;
    auto Class = &Graph->Set.Classes[ClassType];

    // First, we need to limit the register file to ensure space, spilling if
    // necessary. This is based only on the number of bits set in Available, not
    // their order. At this point, free registers need not be contiguous, even
    // if we're allocating a pair. We'll worry about shuffle code later.
    //
    // TODO: Maybe specialize this function for pairs vs not-pairs?
    while (std::popcount(Class->Available) < ClassSize(OrigClassType)) {
      IREmit->SetWriteCursorBefore(CodeNode);
      SpillReg(Class, Pivot);
    }

    // Now that we've spilled, there are enough registers. But the register file
    // may be fragmented. In that case, ppick a scalar that is blocking a pair,
    // and evict it to make room for the pair.
    uint32_t Available = AvailableMask(Class, Pair);
    if (!Available) {
      LOGMAN_THROW_AA_FMT(OrigClassType == GPRPairClass, "Already spilled");

      // First, find a free scalar. There must be at least 2.
      Available = Class->Available;
      unsigned Hole = std::countr_zero(Available);
      LOGMAN_THROW_AA_FMT(Class->Available & (1 << Hole), "Definition");

      // Its neighbour is blocking the pair.
      unsigned Blocked = Hole ^ 1;
      LOGMAN_THROW_AA_FMT(!(Class->Available & (1 << Blocked)), "Invariant");

      // Find another free scalar to evict the neighbour
      uint32_t AvailableAfter = Available & ~(1 << Hole);
      unsigned NewReg = std::countr_zero(AvailableAfter);
      LOGMAN_THROW_AA_FMT(Class->Available & (1 << NewReg), "Ensured space");

      // Now just evict.
      IREmit->SetWriteCursorBefore(CodeNode);
      auto Old = Class->RegToSSA[Blocked];

      LOGMAN_THROW_AA_FMT(GetRegClassFromNode(&IR, IR.GetOp<IROp_Header>(Old)) == GPRClass, "Must be a scalar due to alignment and free "
                                                                                            "neighbour");
      auto Copy = IREmit->_Copy(Map(Old));
      printf("copy %d -> %d -> %d\n", IR.GetID(Old).Value, IR.GetID(Map(Old)).Value, IR.GetID(Copy).Value),

        Remap(Old, Copy);
      FreeReg(PhysicalRegister(GPRClass, Blocked));
      SetReg(Copy, Class, PhysicalRegister(GPRClass, NewReg));

      Available = AvailableMask(Class, Pair);
    }

    LOGMAN_THROW_AA_FMT(Available != 0, "Post-condition of spill and shuffle");

    // Assign a free register in the appropriate class
    // Now that we have split live ranges, this must succeed.
    unsigned Reg = std::countr_zero(Available);
    printf("assigning %u <-- %u\n", IR.GetID(CodeNode).Value, Reg);
    SetReg(CodeNode, Class, PhysicalRegister(OrigClassType, Reg));
  };

  for (auto [BlockNode, BlockHeader] : IR.GetBlocks()) {
    for (auto& Class : Graph->Set.Classes) {
      // At the start of each block, all registers are available. Initialize the
      // available bit set. This is a bit set.
      Class.Available = (1 << Class.Count) - 1;
    }

    // Stream of sources in the block, backwards. (First element is the last
    // source in the block.)
    //
    // Contains the next-use distance (relative to the end of the block) of the
    // source following this instruction.
    fextl::vector<uint32_t> SourcesNextUses;

    // IP relative to the end of the block.
    uint32_t IP = 1;

    // Backwards pass:
    //  - analyze kill bits, next-use distances, and affinities (TODO).
    //  - insert moves for tied operands (TODO)
    {
      // Reverse iteration is not yet working with the iterators
      auto BlockIROp = BlockHeader->CW<FEXCore::IR::IROp_CodeBlock>();

      // We grab these nodes this way so we can iterate easily
      auto CodeBegin = IR.at(BlockIROp->Begin);
      auto CodeLast = IR.at(BlockIROp->Last);

      while (1) {
        auto [CodeNode, IROp] = CodeLast();
        // End of iteration gunk

        // We iterate sources backwards, since we're walking backwards and this
        // will ensure the order of SourcesKilled is consistent. This means the
        // forward pass can iterate forwards and just flip the order.
        const uint8_t NumArgs = IR::GetRAArgs(IROp->Op);
        for (int8_t i = NumArgs - 1; i >= 0; --i) {
          const auto& Arg = IROp->Args[i];
          if (!IsValidArg(Arg)) {
            continue;
          }

          const auto Index = Arg.ID().Value;

          SourcesNextUses.push_back(NextUses[Index]);
          NextUses[Index] = IP;
        }

        // IP is relative to end of the block and we're iterating backwards, so
        // increment here.
        ++IP;

        // Rest is iteration gunk
        if (CodeLast == CodeBegin) {
          break;
        }
        --CodeLast;
      }
    }

    // After the backwards pass, NextUses contains with the distances of
    // the first use in each block, which is exactly what we need to initialize
    // at the start of the forward pass. So there's no need for explicit
    // initialization here.

    // SourcesNextUses is read backwards, this tracks the index
    unsigned SourceIndex = SourcesNextUses.size();

    // Forward pass: Assign registers, spilling as we go.
    for (auto [CodeNode, IROp] : IR.GetCode(BlockNode)) {
      LOGMAN_THROW_AA_FMT(IROp->Op != OP_SPILLREGISTER && IROp->Op != OP_FILLREGISTER, "Spills/fills inserted before the node,"
                                                                                       "so we don't see them iterating forward");

      // Fill all sources read that are not already in the register file.
      //
      // This happens before processing kill bits, since we need all sources in
      // the register file at the same time.
      foreach_valid_arg(IROp, _, Arg) {
        auto Old = IR.GetNode(Arg);
        LOGMAN_THROW_AA_FMT(IsOld(Old), "before remapping");

        if (!IsInRegisterFile(Old)) {
          printf("not in the file %d\n", IR.GetID(Old).Value);
          LOGMAN_THROW_AA_FMT(SpillSlots.at(IR.GetID(Old).Value), "Must have been spilled");
          IREmit->SetWriteCursorBefore(CodeNode);
          auto Fill = InsertFill(Old);

          Remap(Old, Fill);
          AssignReg(Fill, IROp);
        }
      }

      // Remap sources, in case we split any live ranges.
      // Then process killed/next-use info.
      foreach_valid_arg(IROp, i, Arg) {
        LOGMAN_THROW_AA_FMT(IsInRegisterFile(IR.GetNode(Arg)), "Filled");

        const auto Remapped = SSAToNewSSA.at(Arg.ID().Value);
        if (Remapped != nullptr) {
          IREmit->ReplaceNodeArgument(CodeNode, i, Remapped);
        }
      }

      // Assign destinations
      if (GetHasDest(IROp->Op)) {
        AssignReg(CodeNode, IROp);
      }

      // XXX TODO: Move up so we can share a reg with killed dest
      foreach_valid_arg(IROp, i, Arg) {
        SourceIndex--;
        LOGMAN_THROW_AA_FMT(SourceIndex >= 0, "Consistent source count");

        // TODO: Would rather not remap twice? but needed if AssignReg shuffles
        // a source.
        const auto Remapped = Map(Unmap(IR.GetNode(Arg)));
        if (Remapped != IR.GetNode(Arg)) {
          IREmit->ReplaceNodeArgument(CodeNode, i, Remapped);
        }

        // TODO: special handling for clobbered killed sources?

        auto New = IR.GetNode(IROp->Args[i]);
        auto Old = Unmap(New);

        printf("source %d (was %d)\n", IR.GetID(Old).Value, IsInRegisterFile(Old));
        LOGMAN_THROW_AA_FMT(IsInRegisterFile(Old), "instructions sources in file");
        if (!SourcesNextUses[SourceIndex]) {
          printf("killing %d (was %d)\n", IR.GetID(New).Value, IR.GetID(Old).Value);
          FreeReg(SSAToReg.at(IR.GetID(New).Value));
        }

        NextUses.at(IR.GetID(Old).Value) = SourcesNextUses[SourceIndex];
      }

      LOGMAN_THROW_AA_FMT(IP >= 1, "IP relative to end of block, iterating forward");
      --IP;
    }

    LOGMAN_THROW_AA_FMT(SourceIndex == 0, "Consistent source count in block");
  }

  /* Now that we're done growing things, we can finalize our results.
   *
   * TODO: Rework RegisterAllocationData to remove this memcpy, it's pointless.
   */
  Graph->AllocData = RegisterAllocationData::Create(SSAToReg.size());
  Graph->AllocData->SpillSlotCount = SpillSlotCount;
  memcpy(Graph->AllocData->Map, SSAToReg.data(), sizeof(PhysicalRegister) * SSAToReg.size());

  /* No point tracking this finely, RA is always one-shot */
  return true;
}

fextl::unique_ptr<FEXCore::IR::RegisterAllocationPass> CreateRegisterAllocationPass(FEXCore::IR::Pass* CompactionPass, bool SupportsAVX) {
  return fextl::make_unique<ConstrainedRAPass>(SupportsAVX);
}
} // namespace FEXCore::IR
