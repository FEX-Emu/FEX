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

using namespace FEXCore;

namespace FEXCore::IR {
namespace {
  constexpr uint32_t INVALID_REG = IR::InvalidReg;
  constexpr uint32_t INVALID_CLASS = IR::InvalidClass.Val;
  constexpr uint32_t EVEN_BITS = 0x55555555;

  struct RegisterClass {
    uint32_t Available;
    uint32_t Count;

    // If bit R of Allocated is 1, then RegToSSA[R] is the Old node
    // currently allocated to R. Else, RegToSSA[R] is UNDEFINED, no need to
    // clear this when freeing registers.
    OrderedNode* RegToSSA[32];

    // Allocated base registers. Similar to ~Available except for pairs.
    uint32_t Allocated;
  };

  IR::RegisterClassType GetRegClassFromNode(IR::IRListView* IR, IR::IROp_Header* IROp) {
    IR::RegisterClassType Class = IR::GetRegClass(IROp->Op);
    if (Class != IR::ComplexClass) {
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
    default: return IR::InvalidClass;
    }
  };
} // Anonymous namespace

class ConstrainedRAPass final : public RegisterAllocationPass {
public:
  bool Run(IREmitter* IREmit) override;

  void AllocateRegisterSet(uint32_t ClassCount) override;
  void AddRegisters(IR::RegisterClassType Class, uint32_t RegisterCount) override;

  RegisterAllocationData* GetAllocationData() override;
  RegisterAllocationData::UniquePtr PullAllocationData() override;

private:
  IR::RegisterAllocationData::UniquePtr AllocData;
  fextl::vector<RegisterClass> Classes;
};

void ConstrainedRAPass::AllocateRegisterSet(uint32_t ClassCount) {
  LOGMAN_THROW_AA_FMT(ClassCount <= INVALID_CLASS, "Up to {} classes supported", INVALID_CLASS);

  Classes.resize(ClassCount);
}

void ConstrainedRAPass::AddRegisters(IR::RegisterClassType Class, uint32_t RegisterCount) {
  LOGMAN_THROW_AA_FMT(RegisterCount <= INVALID_REG, "Up to {} regs supported", INVALID_REG);

  Classes[Class].Count = RegisterCount;
}

RegisterAllocationData* ConstrainedRAPass::GetAllocationData() {
  return AllocData.get();
}

RegisterAllocationData::UniquePtr ConstrainedRAPass::PullAllocationData() {
  return std::move(AllocData);
}

#define foreach_arg(IROp, index, arg) \
  for (auto [index, Arg] = std::tuple(0, IROp->Args[0]); index < IR::GetRAArgs(IROp->Op); Arg = IROp->Args[++index])

#define foreach_valid_arg(IROp, index, arg) foreach_arg(IROp, index, arg) if (IsValidArg(Arg))

bool ConstrainedRAPass::Run(IREmitter* IREmit) {
  FEXCORE_PROFILE_SCOPED("PassManager::RA");

  auto IR = IREmit->ViewIR();

  // Map of Old nodes to their preferred register, to coalesce load/store reg.
  fextl::vector<PhysicalRegister> PreferredReg(IR.GetSSACount(), PhysicalRegister::Invalid());

  // FEX's original RA could only assign a single register to a given def for
  // its entire live range, and this limitation is baked deep into the IR.
  // However, we split live ranges to implement register pairs and spilling.
  //
  // To reconcile, we generate new SSA nodes when we split live ranges, and
  // remap SSA sources accordingly. This means SSAToReg can grow.
  //
  // We define "Old" nodes as nodes present in the original IR, and "New" nodes
  // as nodes added to split live ranges. Helpful properties:
  //
  // - A node is Old <===> it is not New
  // - A node is Old <===> its ID < IR.GetSSACount() at the start
  // - All sources are Old before remapping an instruction
  //
  // SSAToNewSSA tracks the current remapping. nullptr indicates no remapping.
  //
  // Since its indexed by Old nodes, SSAToNewSSA does not grow.
  fextl::vector<OrderedNode*> SSAToNewSSA(IR.GetSSACount(), nullptr);

  // Inverse of SSAToNewSSA. Since it's indexed by new nodes, it grows.
  fextl::vector<OrderedNode*> NewSSAToSSA(IR.GetSSACount(), nullptr);

  // Map of assigned registers. Grows.
  PhysicalRegister InvalidPhysReg = PhysicalRegister(InvalidClass, InvalidReg);
  fextl::vector<PhysicalRegister> SSAToReg(IR.GetSSACount(), InvalidPhysReg);

  auto IsOld = [&IR, &SSAToNewSSA](OrderedNode* Node) {
    return IR.GetID(Node).Value < SSAToNewSSA.size();
  };

  // Return the New node (if it exists) for an Old node, else the Old node.
  auto Map = [&IR, &SSAToNewSSA, &IsOld](OrderedNode* Old) {
    LOGMAN_THROW_AA_FMT(IsOld(Old), "Pre-condition");

    return SSAToNewSSA[IR.GetID(Old).Value] ?: Old;
  };

  // Return the Old node for a possibly-remapped node.
  auto Unmap = [&IR, &NewSSAToSSA](OrderedNode* Node) {
    return NewSSAToSSA[IR.GetID(Node).Value] ?: Node;
  };

  // Record a remapping of Old to New.
  auto Remap = [&IR, &SSAToNewSSA, &NewSSAToSSA, &Map, &Unmap, &IsOld](OrderedNode* Old, OrderedNode* New) {
    LOGMAN_THROW_AA_FMT(IsOld(Old) && !IsOld(New), "Pre-condition");

    auto OldID = IR.GetID(Old).Value;
    auto NewID = IR.GetID(New).Value;

    LOGMAN_THROW_AA_FMT(NewID >= NewSSAToSSA.size(), "Brand new SSA def");
    NewSSAToSSA.resize(NewID + 1, 0);

    SSAToNewSSA[OldID] = New;
    NewSSAToSSA[NewID] = Old;

    LOGMAN_THROW_AA_FMT(Map(Old) == New && Unmap(New) == Old, "Post-condition");
    LOGMAN_THROW_AA_FMT(Unmap(Old) == Old, "Invariant1");
  };

  // Maps Old defs to their assigned spill slot + 1, or 0 if not spilled.
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

  // IP of next-use of each Old source. IPs are measured from the end of the
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

  auto DecodeReg = [this](PhysicalRegister Reg) -> std::pair<struct RegisterClass*, uint32_t> {
    auto Pair = (Reg.Class == GPRPairClass);

    return std::make_pair(&Classes[Pair ? GPRClass : Reg.Class], (Pair ? 0b11 : 0b1) << Reg.Reg);
  };

  auto IsInRegisterFile = [&DecodeReg, &IR, &Map, &IsOld, &SSAToReg](auto Old) {
    LOGMAN_THROW_AA_FMT(IsOld(Old), "Precondition");

    auto Reg = SSAToReg[IR.GetID(Map(Old)).Value];
    auto [Class, RegBit] = DecodeReg(Reg);

    return (Class->Allocated & RegBit) && Class->RegToSSA[Reg.Reg] == Old;
  };

  auto FreeReg = [&DecodeReg](auto Reg) {
    auto [Class, RegBits] = DecodeReg(Reg);

    LOGMAN_THROW_AA_FMT(!(Class->Available & RegBits), "Register double-free");
    LOGMAN_THROW_AA_FMT((Class->Allocated & RegBits), "Register double-free");

    Class->Available |= RegBits;
    Class->Allocated &= ~RegBits;
  };

  auto HasSource = [&IR, &IsOld](auto Header, auto Old) {
    LOGMAN_THROW_AA_FMT(IsOld(Old), "Invariant2");

    foreach_arg(Header, _, Arg) {
      auto Node = IR.GetNode(Arg);
      LOGMAN_THROW_AA_FMT(IsOld(Node), "not yet mapped");

      if (Node == Old) {
        return true;
      }
    }

    return false;
  };

  auto GetFixedReg = [this](RegisterClassType Class, uint8_t Reg) -> PhysicalRegister {
    LOGMAN_THROW_AA_FMT(Class == GPRClass || Class == FPRClass, "SRA classes");
    uint8_t FlagOffset = Classes[GPRFixedClass.Val].Count - 2;

    if (Class == FPRClass) {
      return PhysicalRegister {FPRFixedClass, Reg};
    } else if (Reg == Core::CPUState::PF_AS_GREG) {
      return PhysicalRegister {GPRFixedClass, FlagOffset};
    } else if (Reg == Core::CPUState::AF_AS_GREG) {
      return PhysicalRegister {GPRFixedClass, (uint8_t)(FlagOffset + 1)};
    } else {
      return PhysicalRegister {GPRFixedClass, Reg};
    }
  };

  auto DecodeSRA = [&IR, &GetFixedReg](OrderedNode* Node) -> std::pair<OrderedNode*, PhysicalRegister> {
    auto Header = IR.GetOp<IROp_Header>(Node);

    if (Header->Op == OP_LOADREGISTER) {
      auto Op = Header->C<IR::IROp_LoadRegister>();
      return std::make_pair(Node, GetFixedReg(Op->Class, Op->Reg));
    } else if (Header->Op == OP_STOREREGISTER) {
      auto Op = Header->C<IR::IROp_StoreRegister>();
      return std::make_pair(IR.GetNode(Op->Value), GetFixedReg(Op->Class, Op->Reg));
    }

    return std::make_pair(nullptr, PhysicalRegister::Invalid());
  };

  auto SpillReg = [&IR, &IREmit, &SpillSlotCount, &SpillSlots, &SSAToReg, &FreeReg, &Map, &IsOld, &NextUses,
                   &HasSource](auto Class, IROp_Header* Exclude, bool Pair) {
    // Find the best node to spill according to the "furthest-first" heuristic.
    // Since we defined IPs relative to the end of the block, the furthest
    // next-use has the /smallest/ unsigned IP.
    //
    // TODO: Remat.
    OrderedNode* Candidate = nullptr;
    uint32_t BestDistance = UINT32_MAX;
    uint8_t BestReg = ~0;

    for (int i = 0; i < Class->Count; ++i) {
      // We have to prioritize the pair region if we're allocating for a Pair.
      // See the comment at the call site in AssignReg.
      if (Pair && Candidate != nullptr && i >= PAIR_COUNT) {
        break;
      }

      if (Class->Allocated & (1u << i)) {
        OrderedNode* Old = Class->RegToSSA[i];

        LOGMAN_THROW_AA_FMT(Old != nullptr, "Invariant3");
        LOGMAN_THROW_AA_FMT(SSAToReg.at(IR.GetID(Map(Old)).Value).Reg == i, "Invariant4");

        // Skip any source used by the current instruction, it is unspillable.
        if (!HasSource(Exclude, Old)) {
          uint32_t NextUse = NextUses.at(IR.GetID(Old).Value);
          if (NextUse < BestDistance) {
            BestDistance = NextUse;
            BestReg = i;
            Candidate = Old;
          }
        }
      }
    }

    LOGMAN_THROW_AA_FMT(Candidate != nullptr, "must've found something..");
    LOGMAN_THROW_AA_FMT(IsOld(Candidate), "Invariant5");

    auto Reg = SSAToReg.at(IR.GetID(Map(Candidate)).Value);
    LOGMAN_THROW_AA_FMT(Reg.Reg == BestReg, "Invariant6");

    auto Header = IR.GetOp<IROp_Header>(Candidate);
    auto Value = IR.GetID(Candidate).Value;
    auto Spilled = SpillSlots.at(Value) != 0;

    // If we already spilled the Candidate, we don't need to spill again.
    if (!Spilled) {
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
  auto SetReg = [&IR, &Unmap, &SSAToReg, &DecodeReg, InvalidPhysReg](OrderedNode* Node, PhysicalRegister Reg) {
    uint32_t Index = IR.GetID(Node).Value;
    auto [Class, RegBits] = DecodeReg(Reg);

    LOGMAN_THROW_AA_FMT((Class->Available & RegBits) == RegBits, "Precondition");
    LOGMAN_THROW_AA_FMT(!(Class->Allocated & RegBits), "Precondition");

    Class->Available &= ~RegBits;
    Class->Allocated |= (1u << Reg.Reg);
    Class->RegToSSA[Reg.Reg] = Unmap(Node);

    if (Index >= SSAToReg.size()) {
      SSAToReg.resize(Index + 1, InvalidPhysReg);
    }

    SSAToReg.at(Index) = Reg;
  };

  // Get the mask of avaiable registers for a given register class
  auto AvailableMask = [](auto Class, bool Pair) {
    uint32_t Available = Class->Available;

    if (Pair) {
      // Only choose base register R if R and R + 1 are both free
      Available &= (Available >> 1);

      // Only consider aligned registers in the pair region
      Available &= (EVEN_BITS & ((1u << PairRegs) - 1));
    }

    return Available;
  };

  // Assign a register for a given Node, spilling if necessary.
  auto AssignReg = [this, &IR, IREmit, &SpillReg, &Remap, &FreeReg, &SetReg, &Map, &AvailableMask, &PreferredReg, &SSAToReg, &DecodeReg,
                    &IsOld](OrderedNode* CodeNode, IROp_Header* Pivot) {
    const auto Node = IR.GetID(CodeNode);
    LOGMAN_THROW_AA_FMT(Node.IsValid(), "Node must be valid");

    // Prioritize preferred registers.
    if (Node.Value < PreferredReg.size()) {
      auto Reg = PreferredReg[Node.Value];
      auto [Class, RegBits] = DecodeReg(Reg);

      if (!Reg.IsInvalid() && (Class->Available & RegBits) == RegBits) {
        SetReg(CodeNode, Reg);
        return;
      }
    }

    const auto IROp = IR.GetOp<IROp_Header>(CodeNode);
    auto OrigClassType = GetRegClassFromNode(&IR, IROp);
    bool Pair = OrigClassType == GPRPairClass;
    auto ClassType = Pair ? GPRClass : OrigClassType;
    auto Class = &Classes[ClassType];

    // Spill to make room in the register file. Free registers need not be
    // contiguous, we'll shuffle later.
    //
    // There is one subtlety: when allocating a pair, we need at least 1 free
    // register in the pair region. Else, we could end up trying to allocate a
    // pair when the only free 2 regs are outside the pair region, and the pair
    // region is made of all pairs (so nothing to shuffle). With 1 free
    // register in the pair region, we'll be able to shuffle.
    //
    // When spilling for pairs, SpillReg prioritizes spilling the pair region
    // which ensures this loop is well-behaved.
    while (std::popcount(Class->Available) < (Pair ? 2 : 1) || (Pair && !(Class->Available & ((1u << PairRegs) - 1)))) {
      IREmit->SetWriteCursorBefore(CodeNode);
      SpillReg(Class, Pivot, Pair);
    }

    // There are now enough free registers, but they may be fragmented.
    // Pick a scalar blocking a pair and shuffle to make room.
    uint32_t Available = AvailableMask(Class, Pair);
    if (!Available) {
      LOGMAN_THROW_AA_FMT(OrigClassType == GPRPairClass, "Already spilled");

      // Find the first free scalar. There are at least 2.
      unsigned Hole = std::countr_zero(Class->Available);
      LOGMAN_THROW_AA_FMT(Class->Available & (1u << Hole), "Definition");

      // Its neighbour is blocking the pair.
      unsigned Blocked = Hole ^ 1;
      LOGMAN_THROW_AA_FMT(!(Class->Available & (1u << Blocked)), "Invariant7");
      LOGMAN_THROW_AA_FMT(Hole < PairRegs, "Pairable register");

      // Find another free scalar to evict the neighbour
      unsigned NewReg = std::countr_zero(Class->Available & ~(1u << Hole));
      LOGMAN_THROW_AA_FMT(Class->Available & (1u << NewReg), "Ensured space");

      IREmit->SetWriteCursorBefore(CodeNode);
      auto Old = Class->RegToSSA[Blocked];
      LOGMAN_THROW_AA_FMT(GetRegClassFromNode(&IR, IR.GetOp<IROp_Header>(Old)) == GPRClass, "Only scalars have free neighbours");
      FreeReg(PhysicalRegister(GPRClass, Blocked));

      bool Clobbered = false;

      // If that scalar is free because it is killed by this instruction, it
      // needs to be shuffled too, since the copy would clobber it.
      foreach_arg(Pivot, _, Arg) {
        const auto Clobber = IR.GetNode(Arg);
        const auto ClobberReg = SSAToReg[IR.GetID(Clobber).Value];
        if (ClobberReg.Class == GPRClass && ClobberReg.Reg == NewReg) {
          // Swap the registers instead of copying.
          LOGMAN_THROW_AA_FMT(IsOld(Clobber), "Not yet mapped");
          Clobbered = true;

          auto ClobberNew = IREmit->_Swap1(Map(Clobber), Map(Old));
          Remap(Clobber, ClobberNew);

          auto New = IREmit->_Swap2();
          Remap(Old, New);

          SetReg(New, PhysicalRegister(GPRClass, NewReg));
          SetReg(ClobberNew, PhysicalRegister(GPRClass, Blocked));
          FreeReg(PhysicalRegister(GPRClass, Blocked));
          break;
        }
      }

      // Otherwise, simply copy.
      if (!Clobbered) {
        auto Copy = IREmit->_Copy(Map(Old));

        Remap(Old, Copy);
        SetReg(Copy, PhysicalRegister(GPRClass, NewReg));
      }

      Available = AvailableMask(Class, Pair);
    }

    LOGMAN_THROW_AA_FMT(Available != 0, "Post-condition of spill and shuffle");

    // Assign a free register in the appropriate class.
    unsigned Reg = std::countr_zero(Available);
    SetReg(CodeNode, PhysicalRegister(OrigClassType, Reg));
  };

  auto IsRAOp = [](auto Op) -> bool {
    return Op == OP_SPILLREGISTER || Op == OP_FILLREGISTER || Op == OP_COPY;
  };

  for (auto [BlockNode, BlockHeader] : IR.GetBlocks()) {
    // At the start of each block, all registers are available.
    for (auto& Class : Classes) {
      Class.Available = (1u << Class.Count) - 1;
      Class.Allocated = 0;
    }

    // Next-use distance relative to the block end of each source, last first.
    fextl::vector<uint32_t> SourcesNextUses;

    // IP relative to the end of the block.
    uint32_t IP = 1;

    // Backwards pass:
    //  - analyze kill bits, next-use distances, and affinities
    //  - insert moves for tied operands (TODO)
    {
      // Reverse iteration is not yet working with the iterators
      auto BlockIROp = BlockHeader->CW<IR::IROp_CodeBlock>();

      // We grab these nodes this way so we can iterate easily
      auto CodeBegin = IR.at(BlockIROp->Begin);
      auto CodeLast = IR.at(BlockIROp->Last);

      while (1) {
        auto [CodeNode, IROp] = CodeLast();
        // End of iteration gunk

        // Iterate sources backwards, since we walk backwards. Ensures the order
        // of SourcesNextUses is consistent. The forward pass can then iterate
        // forwards and just flip the order.
        const uint8_t NumArgs = IR::GetRAArgs(IROp->Op);
        for (int8_t i = NumArgs - 1; i >= 0; --i) {
          const auto& Arg = IROp->Args[i];
          if (IsValidArg(Arg)) {
            const auto Index = Arg.ID().Value;

            SourcesNextUses.push_back(NextUses[Index]);
            NextUses[Index] = IP;
          }
        }

        // Record preferred registers for SRA. We also record the Node accessing
        // each register, used below. Since we initialized Class->Allocated = 0,
        // RegToSSA is otherwise undefined so we can stash our temps there.
        if (auto [Node, Reg] = DecodeSRA(CodeNode); Node != nullptr) {
          auto [Class, _] = DecodeReg(Reg);

          PreferredReg[IR.GetID(Node).Value] = Reg;
          Class->RegToSSA[Reg.Reg] = CodeNode;
        }

        // Coalescing an SRA store is equivalent to hoisting the store,
        // implying write-after-write and read-after-write hazards. We can only
        // coalesce if there is no intervening load/store.
        //
        // Since we're walking backwards, RegToSSA tracks
        // the first load/store after CodeNode. That first instruction is the
        // store in question iff there is no intervening load/store.
        //
        // Reset PreferredReg if that is not the case, ensuring SRA correctness.
        if (auto Reg = PreferredReg[IR.GetID(CodeNode).Value]; !Reg.IsInvalid()) {
          auto [Class, _] = DecodeReg(Reg);
          auto [SRA, __] = DecodeSRA(Class->RegToSSA[Reg.Reg]);

          if (CodeNode != SRA) {
            PreferredReg[IR.GetID(CodeNode).Value] = PhysicalRegister::Invalid();
          }
        }

        // IP is relative to block end and we iterate backwards, so increment.
        ++IP;

        // Rest is iteration gunk
        if (CodeLast == CodeBegin) {
          break;
        }
        --CodeLast;
      }
    }

    // NextUses currently contains first use distances, the exact initialization
    // assumed by the forward pass. Do not reset it.

    // SourcesNextUses is read backwards, this tracks the index
    unsigned SourceIndex = SourcesNextUses.size();

    // Forward pass: Assign registers, spilling as we go.
    for (auto [CodeNode, IROp] : IR.GetCode(BlockNode)) {
      LOGMAN_THROW_AA_FMT(!IsRAOp(IROp->Op), "RA ops inserted before, so not seen iterating forward");

      // Static registers must be consistent at SRA load/store. Evict to ensure.
      if (auto [Node, Reg] = DecodeSRA(CodeNode); Node != nullptr) {
        auto [Class, RegBit] = DecodeReg(Reg);

        if (Class->Allocated & RegBit) {
          auto Old = Class->RegToSSA[Reg.Reg];

          LOGMAN_THROW_AA_FMT(IsOld(Old), "RegToSSA invariant");
          LOGMAN_THROW_AA_FMT(IsOld(Node), "Haven't remapped this instruction");

          if (Old != Node) {
            IREmit->SetWriteCursorBefore(CodeNode);
            OrderedNode* Copy;

            if (Reg.Class == FPRFixedClass) {
              auto Header = IR.GetOp<IROp_Header>(Old);
              Copy = IREmit->_VMov(Header->Size, Map(Old));
            } else {
              Copy = IREmit->_Copy(Map(Old));
            }

            Remap(Old, Copy);
            FreeReg(Reg);
            AssignReg(Copy, IROp);
          }
        }
      }

      // Fill all sources that are not already in the register file.
      //
      // This happens before freeing killed sources, since we need all sources in
      // the register file simultaneously.
      foreach_valid_arg(IROp, _, Arg) {
        auto Old = IR.GetNode(Arg);
        LOGMAN_THROW_AA_FMT(IsOld(Old), "before remapping");

        if (!IsInRegisterFile(Old)) {
          LOGMAN_THROW_AA_FMT(SpillSlots.at(IR.GetID(Old).Value), "Must have been spilled");
          IREmit->SetWriteCursorBefore(CodeNode);
          auto Fill = InsertFill(Old);

          Remap(Old, Fill);
          AssignReg(Fill, IROp);
        }
      }

      foreach_valid_arg(IROp, i, Arg) {
        SourceIndex--;
        LOGMAN_THROW_AA_FMT(SourceIndex >= 0, "Consistent source count");

        auto Old = IR.GetNode(Arg);
        LOGMAN_THROW_AA_FMT(IsInRegisterFile(Old), "sources in file");

        if (!SourcesNextUses[SourceIndex]) {
          FreeReg(SSAToReg.at(IR.GetID(Map(Old)).Value));
        }

        NextUses.at(IR.GetID(Old).Value) = SourcesNextUses[SourceIndex];
      }

      // Assign destinations.
      if (GetHasDest(IROp->Op)) {
        AssignReg(CodeNode, IROp);
      }

      // Remap sources last, since AssignReg can shuffle.
      foreach_valid_arg(IROp, i, Arg) {
        auto Remapped = SSAToNewSSA.at(IR.GetID(IR.GetNode(Arg)).Value);

        if (Remapped != nullptr) {
          IREmit->ReplaceNodeArgument(CodeNode, i, Remapped);
        }
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
  AllocData = RegisterAllocationData::Create(SSAToReg.size());
  AllocData->SpillSlotCount = SpillSlotCount;
  memcpy(AllocData->Map, SSAToReg.data(), sizeof(PhysicalRegister) * SSAToReg.size());

  /* No point tracking this finely, RA is always one-shot */
  return true;
}

fextl::unique_ptr<IR::RegisterAllocationPass> CreateRegisterAllocationPass(IR::Pass* CompactionPass) {
  return fextl::make_unique<ConstrainedRAPass>();
}
} // namespace FEXCore::IR
