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

  struct RegisterClass {
    uint32_t Available;
    uint32_t Count;

    // If bit R of Available is 0, then RegToSSA[R] is the Old node
    // currently allocated to R. Else, RegToSSA[R] is UNDEFINED, no need to
    // clear this when freeing registers.
    Ref RegToSSA[32];
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
  void Run(IREmitter* IREmit) override;
  void AddRegisters(IR::RegisterClassType Class, uint32_t RegisterCount) override;

  RegisterAllocationData* GetAllocationData() override;
  RegisterAllocationData::UniquePtr PullAllocationData() override;

private:
  IR::RegisterAllocationData::UniquePtr AllocData;
  RegisterClass Classes[INVALID_CLASS];

  IREmitter* IREmit;
  IRListView* IR;

  // Map of Old nodes to their preferred register, to coalesce load/store reg.
  fextl::vector<PhysicalRegister> PreferredReg;

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
  // Since its indexed by Old nodes, SSAToNewSSA does not grow after allocation.
  fextl::vector<Ref> SSAToNewSSA;

  // Inverse of SSAToNewSSA. Since it's indexed by new nodes, it grows.
  fextl::vector<Ref> NewSSAToSSA;

  // Map of assigned registers. Grows.
  fextl::vector<PhysicalRegister> SSAToReg;

  bool IsOld(Ref Node) {
    return IR->GetID(Node).Value < PreferredReg.size();
  };

  // Return the New node (if it exists) for an Old node, else the Old node.
  Ref Map(Ref Old) {
    LOGMAN_THROW_A_FMT(IsOld(Old), "Pre-condition");

    if (SSAToNewSSA.empty()) {
      return Old;
    } else {
      return SSAToNewSSA[IR->GetID(Old).Value] ?: Old;
    }
  };

  // Return the Old node for a possibly-remapped node.
  Ref Unmap(Ref Node) {
    if (NewSSAToSSA.empty()) {
      return Node;
    } else {
      return NewSSAToSSA[IR->GetID(Node).Value] ?: Node;
    }
  };

  // Record a remapping of Old to New.
  void Remap(Ref Old, Ref New) {
    LOGMAN_THROW_A_FMT(IsOld(Old) && !IsOld(New), "Pre-condition");

    uint32_t OldID = IR->GetID(Old).Value;
    uint32_t NewID = IR->GetID(New).Value;

    LOGMAN_THROW_A_FMT(NewID >= NewSSAToSSA.size(), "Brand new SSA def");
    NewSSAToSSA.resize(NewID + 1, 0);

    if (SSAToNewSSA.empty()) {
      SSAToNewSSA.resize(PreferredReg.size(), nullptr);
    }

    SSAToNewSSA[OldID] = New;
    NewSSAToSSA[NewID] = Old;

    LOGMAN_THROW_A_FMT(Map(Old) == New && Unmap(New) == Old, "Post-condition");
    LOGMAN_THROW_A_FMT(Unmap(Old) == Old, "Invariant1");
  };

  // Maps Old defs to their assigned spill slot + 1, or 0 if not spilled.
  fextl::vector<unsigned> SpillSlots;

  bool Rematerializable(IROp_Header* IROp) {
    return IROp->Op == OP_CONSTANT;
  }

  Ref InsertFill(Ref Old) {
    LOGMAN_THROW_A_FMT(IsOld(Old), "Precondition");
    IROp_Header* IROp = IR->GetOp<IROp_Header>(Old);

    // Remat if we can
    if (Rematerializable(IROp)) {
      uint64_t Const = IROp->C<IR::IROp_Constant>()->Constant;
      return IREmit->_Constant(Const);
    }

    // Otherwise fill from stack
    uint32_t SlotPlusOne = SpillSlots[IR->GetID(Old).Value];
    LOGMAN_THROW_AA_FMT(SlotPlusOne >= 1, "Old must have been spilled");

    RegisterClassType RegClass = GetRegClassFromNode(IR, IROp);

    auto Fill = IREmit->_FillRegister(Old, SlotPlusOne - 1, RegClass);
    Fill.first->Header.Size = IROp->Size;
    Fill.first->Header.ElementSize = IROp->ElementSize;
    return Fill;
  };

  // IP of next-use of each Old source. IPs are measured from the end of the
  // block, so we don't need to size the block up-front.
  fextl::vector<uint32_t> NextUses;

  unsigned SpillSlotCount;
  bool AnySpilled;

  bool IsValidArg(OrderedNodeWrapper Arg) {
    if (Arg.IsInvalid()) {
      return false;
    }

    switch (IR->GetOp<IROp_Header>(Arg)->Op) {
    case OP_INLINECONSTANT:
    case OP_INLINEENTRYPOINTOFFSET:
    case OP_IRHEADER: return false;

    case OP_SPILLREGISTER: LOGMAN_MSG_A_FMT("should not be seen"); return false;

    default: return true;
    }
  };

  RegisterClass* GetClass(PhysicalRegister Reg) {
    return &Classes[Reg.Class];
  };

  uint32_t GetRegBits(PhysicalRegister Reg) {
    return 1 << Reg.Reg;
  };

  bool IsInRegisterFile(Ref Old) {
    LOGMAN_THROW_A_FMT(IsOld(Old), "Precondition");

    PhysicalRegister Reg = SSAToReg[IR->GetID(Map(Old)).Value];
    RegisterClass* Class = GetClass(Reg);

    return (Class->Available & GetRegBits(Reg)) == 0 && Class->RegToSSA[Reg.Reg] == Old;
  };

  void FreeReg(PhysicalRegister Reg) {
    RegisterClass* Class = GetClass(Reg);
    uint32_t RegBits = GetRegBits(Reg);

    LOGMAN_THROW_AA_FMT(!(Class->Available & RegBits), "Register double-free");

    Class->Available |= RegBits;
  };

  bool HasSource(IROp_Header* I, Ref Old) {
    LOGMAN_THROW_A_FMT(IsOld(Old), "Invariant2");

    for (auto s = 0; s < IR::GetRAArgs(I->Op); ++s) {
      Ref Node = IR->GetNode(I->Args[s]);
      LOGMAN_THROW_A_FMT(IsOld(Node), "not yet mapped");

      if (Node == Old) {
        return true;
      }
    }

    return false;
  };

  Ref DecodeSRANode(const IROp_Header* IROp, Ref Node) {
    if (IROp->Op == OP_LOADREGISTER || IROp->Op == OP_LOADPF || IROp->Op == OP_LOADAF) {
      return Node;
    } else if (IROp->Op == OP_STOREREGISTER) {
      const IROp_StoreRegister* Op = IROp->C<IR::IROp_StoreRegister>();
      return IR->GetNode(Op->Value);
    } else if (IROp->Op == OP_STOREPF || IROp->Op == OP_STOREAF) {
      const IROp_StorePF* Op = IROp->C<IR::IROp_StorePF>();
      return IR->GetNode(Op->Value);
    }

    return nullptr;
  };

  PhysicalRegister DecodeSRAReg(const IROp_Header* IROp, Ref Node) {
    RegisterClassType Class;
    uint8_t Reg;

    uint8_t FlagOffset = Classes[GPRFixedClass.Val].Count - 2;

    if (IROp->Op == OP_LOADREGISTER) {
      const IROp_LoadRegister* Op = IROp->C<IR::IROp_LoadRegister>();

      Class = Op->Class;
      Reg = Op->Reg;
    } else if (IROp->Op == OP_STOREREGISTER) {
      LOGMAN_THROW_AA_FMT(IROp->Op == OP_STOREREGISTER, "node is SRA");
      const IROp_StoreRegister* Op = IROp->C<IR::IROp_StoreRegister>();

      Class = Op->Class;
      Reg = Op->Reg;
    } else if (IROp->Op == OP_LOADPF || IROp->Op == OP_STOREPF) {
      return PhysicalRegister {GPRFixedClass, FlagOffset};
    } else if (IROp->Op == OP_LOADAF || IROp->Op == OP_STOREAF) {
      return PhysicalRegister {GPRFixedClass, (uint8_t)(FlagOffset + 1)};
    }

    LOGMAN_THROW_A_FMT(Class == GPRClass || Class == FPRClass, "SRA classes");
    if (Class == FPRClass) {
      return PhysicalRegister {FPRFixedClass, Reg};
    } else {
      return PhysicalRegister {GPRFixedClass, Reg};
    }
  };

  // Helper macro to walk the set bits b in a 32-bit word x, using ffs to get
  // the next set bit and then clearing on each iteration.
#define foreach_bit(b, x) for (uint32_t __x = (x), b; ((b) = __builtin_ffs(__x) - 1, __x); __x &= ~(1 << (b)))

  void SpillReg(RegisterClass* Class, IROp_Header* Exclude) {
    // Find the best node to spill according to the "furthest-first" heuristic.
    // Since we defined IPs relative to the end of the block, the furthest
    // next-use has the /smallest/ unsigned IP.
    Ref Candidate = nullptr;
    uint32_t BestDistance = UINT32_MAX;
    uint8_t BestReg = ~0;
    uint32_t Allocated = ((1u << Class->Count) - 1) & ~Class->Available;

    foreach_bit(i, Allocated) {
      Ref Old = Class->RegToSSA[i];

      LOGMAN_THROW_AA_FMT(Old != nullptr, "Invariant3");
      LOGMAN_THROW_A_FMT(SSAToReg[IR->GetID(Map(Old)).Value].Reg == i, "Invariant4");

      // Skip any source used by the current instruction, it is unspillable.
      if (!HasSource(Exclude, Old)) {
        uint32_t NextUse = NextUses[IR->GetID(Old).Value];

        // Prioritize remat over spilling. It is typically cheaper to remat a
        // constant multiple times than to spill a single value.
        if (!Rematerializable(IR->GetOp<IROp_Header>(Old))) {
          NextUse += 100000;
        }

        if (NextUse < BestDistance) {
          BestDistance = NextUse;
          BestReg = i;
          Candidate = Old;
        }
      }
    }

    LOGMAN_THROW_AA_FMT(Candidate != nullptr, "must've found something..");
    LOGMAN_THROW_A_FMT(IsOld(Candidate), "Invariant5");

    PhysicalRegister Reg = SSAToReg[IR->GetID(Map(Candidate)).Value];
    LOGMAN_THROW_AA_FMT(Reg.Reg == BestReg, "Invariant6");

    IROp_Header* Header = IR->GetOp<IROp_Header>(Candidate);
    uint32_t Value = IR->GetID(Candidate).Value;
    bool Spilled = !SpillSlots.empty() && SpillSlots[Value] != 0;

    // If we already spilled the Candidate, we don't need to spill again.
    // Similarly, if we can rematerialize the instruction, we don't spill it.
    if (!Spilled && Header->Op != OP_CONSTANT) {
      LOGMAN_THROW_A_FMT(Reg.Class == GetRegClassFromNode(IR, Header), "Consistent");

      // SpillSlots allocation is deferred.
      if (SpillSlots.empty()) {
        SpillSlots.resize(IR->GetSSACount(), 0);
      }

      // TODO: we should colour spill slots
      uint32_t Slot = SpillSlotCount++;

      // We must map here in case we're spilling something we shuffled.
      auto SpillOp = IREmit->_SpillRegister(Map(Candidate), Slot, RegisterClassType {Reg.Class});
      SpillOp.first->Header.Size = Header->Size;
      SpillOp.first->Header.ElementSize = Header->ElementSize;
      SpillSlots[Value] = Slot + 1;
    }

    // Now that we've spilled the value, take it out of the register file
    FreeReg(Reg);
    AnySpilled = true;
  };

  // Record a given assignment of register Reg to Node.
  void SetReg(Ref Node, PhysicalRegister Reg) {
    uint32_t Index = IR->GetID(Node).Value;
    RegisterClass* Class = GetClass(Reg);
    uint32_t RegBits = GetRegBits(Reg);

    LOGMAN_THROW_AA_FMT((Class->Available & RegBits) == RegBits, "Precondition");

    Class->Available &= ~RegBits;
    Class->RegToSSA[Reg.Reg] = Unmap(Node);

    if (Index >= SSAToReg.size()) {
      SSAToReg.resize(Index + 1, PhysicalRegister::Invalid());
    }

    SSAToReg[Index] = Reg;
  };

  // Assign a register for a given Node, spilling if necessary.
  void AssignReg(IROp_Header* IROp, Ref CodeNode, IROp_Header* Pivot) {
    const uint32_t Node = IR->GetID(CodeNode).Value;

    // Prioritize preferred registers.
    if (Node < PreferredReg.size()) {
      if (PhysicalRegister Reg = PreferredReg[Node]; !Reg.IsInvalid()) {
        RegisterClass* Class = GetClass(Reg);
        uint32_t RegBits = GetRegBits(Reg);

        if ((Class->Available & RegBits) == RegBits) {
          SetReg(CodeNode, Reg);
          return;
        }
      }
    }

    // Try to handle tied registers. This can fail, the JIT will insert moves.
    if (int TiedIdx = IR::TiedSource(IROp->Op); TiedIdx >= 0) {
      PhysicalRegister Reg = SSAToReg[IROp->Args[TiedIdx].ID().Value];
      RegisterClass* Class = GetClass(Reg);
      uint32_t RegBits = GetRegBits(Reg);

      if (Reg.Class != GPRFixedClass && Reg.Class != FPRFixedClass && (Class->Available & RegBits) == RegBits) {
        SetReg(CodeNode, Reg);
        return;
      }
    }

    // Try to coalesce reserved pairs. Just a heuristic to remove some moves.
    if (IROp->Op == OP_ALLOCATEGPR) {
      if (IROp->C<IROp_AllocateGPR>()->ForPair) {
        uint32_t Available = Classes[GPRClass].Available;

        // Only choose base register R if R and R + 1 are both free
        Available &= (Available >> 1);

        // Only consider aligned registers in the pair region
        constexpr uint32_t EVEN_BITS = 0x55555555;
        Available &= (EVEN_BITS & ((1u << PairRegs) - 1));

        if (Available) {
          unsigned Reg = std::countr_zero(Available);
          SetReg(CodeNode, PhysicalRegister(GPRClass, Reg));
          return;
        }
      }
    } else if (IROp->Op == OP_ALLOCATEGPRAFTER) {
      uint32_t Available = Classes[GPRClass].Available;
      auto After = SSAToReg[IR->GetID(IR->GetNode(IROp->Args[0])).Value];
      if ((After.Reg & 1) == 0 && Available & (1ull << (After.Reg + 1))) {
        SetReg(CodeNode, PhysicalRegister(GPRClass, After.Reg + 1));
        return;
      }
    }

    RegisterClassType ClassType = GetRegClassFromNode(IR, IROp);
    RegisterClass* Class = &Classes[ClassType];

    // Spill to make room in the register file.
    if (!Class->Available) {
      IREmit->SetWriteCursorBefore(CodeNode);
      SpillReg(Class, Pivot);
    }

    // Assign a free register in the appropriate class.
    LOGMAN_THROW_AA_FMT(Class->Available != 0, "Post-condition of spilling");
    unsigned Reg = std::countr_zero(Class->Available);
    SetReg(CodeNode, PhysicalRegister(ClassType, Reg));
  };

  bool IsRAOp(IROps Op) {
    return Op == OP_SPILLREGISTER || Op == OP_FILLREGISTER || Op == OP_COPY;
  };
};

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

void ConstrainedRAPass::Run(IREmitter* IREmit_) {
  FEXCORE_PROFILE_SCOPED("PassManager::RA");

  IREmit = IREmit_;
  auto IR_ = IREmit->ViewIR();
  IR = &IR_;

  // SSAToNewSSA, NewSSAToSSA allocated on first-use
  PreferredReg.resize(IR->GetSSACount(), PhysicalRegister::Invalid());
  SSAToReg.resize(IR->GetSSACount(), PhysicalRegister::Invalid());
  NextUses.resize(IR->GetSSACount(), 0);
  SpillSlotCount = 0;
  AnySpilled = false;

  // Next-use distance relative to the block end of each source, last first.
  fextl::vector<uint32_t> SourcesNextUses;

  for (auto [BlockNode, BlockHeader] : IR->GetBlocks()) {
    // At the start of each block, all registers are available.
    for (auto& Class : Classes) {
      Class.Available = (1u << Class.Count) - 1;
    }

    SourcesNextUses.clear();

    // IP relative to the end of the block.
    uint32_t IP = 1;

    // Backwards pass:
    //  - analyze kill bits, next-use distances, and affinities
    //  - insert moves for tied operands (TODO)
    {
      // Reverse iteration is not yet working with the iterators
      auto BlockIROp = BlockHeader->CW<IR::IROp_CodeBlock>();

      // We grab these nodes this way so we can iterate easily
      auto CodeBegin = IR->at(BlockIROp->Begin);
      auto CodeLast = IR->at(BlockIROp->Last);

      while (1) {
        auto [CodeNode, IROp] = CodeLast();
        // End of iteration gunk

        // Iterate sources backwards, since we walk backwards. Ensures the order
        // of SourcesNextUses is consistent. The forward pass can then iterate
        // forwards and just flip the order.
        const uint8_t NumArgs = IR::GetRAArgs(IROp->Op);
        for (int8_t i = NumArgs - 1; i >= 0; --i) {
          const auto& Arg = IROp->Args[i];
          if (!Arg.IsInvalid()) {
            const uint32_t Index = Arg.ID().Value;

            SourcesNextUses.push_back(NextUses[Index]);
            NextUses[Index] = IP;
          }
        }

        // Record preferred registers for SRA. We also record the Node accessing
        // each register, used below. Since we initialized Class->Available,
        // RegToSSA is otherwise undefined so we can stash our temps there.
        if (auto Node = DecodeSRANode(IROp, CodeNode); Node != nullptr) {
          auto Reg = DecodeSRAReg(IROp, Node);

          PreferredReg[IR->GetID(Node).Value] = Reg;
          GetClass(Reg)->RegToSSA[Reg.Reg] = CodeNode;
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
        if (auto Reg = PreferredReg[IR->GetID(CodeNode).Value]; !Reg.IsInvalid()) {
          auto Node = GetClass(Reg)->RegToSSA[Reg.Reg];
          IROp_Header* Header = IR->GetOp<IROp_Header>(Node);

          if (CodeNode != DecodeSRANode(Header, Node)) {
            PreferredReg[IR->GetID(CodeNode).Value] = PhysicalRegister::Invalid();
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
    for (auto [CodeNode, IROp] : IR->GetCode(BlockNode)) {
      LOGMAN_THROW_A_FMT(!IsRAOp(IROp->Op), "RA ops inserted before, so not seen iterating forward");

      // Static registers must be consistent at SRA load/store. Evict to ensure.
      if (auto Node = DecodeSRANode(IROp, CodeNode); Node != nullptr) {
        auto Reg = DecodeSRAReg(IROp, Node);
        RegisterClass* Class = &Classes[Reg.Class];

        if (!(Class->Available & (1u << Reg.Reg))) {
          Ref Old = Class->RegToSSA[Reg.Reg];

          LOGMAN_THROW_A_FMT(IsOld(Old), "RegToSSA invariant");
          LOGMAN_THROW_A_FMT(IsOld(Node), "Haven't remapped this instruction");

          if (Old != Node) {
            IREmit->SetWriteCursorBefore(CodeNode);
            Ref Copy;

            if (Reg.Class == FPRFixedClass) {
              IROp_Header* Header = IR->GetOp<IROp_Header>(Old);
              Copy = IREmit->_VMov(Header->Size, Map(Old));
            } else {
              Copy = IREmit->_Copy(Map(Old));
            }

            Remap(Old, Copy);
            FreeReg(Reg);
            AssignReg(IR->GetOp<IROp_Header>(Copy), Copy, IROp);
          }
        }
      }

      // Fill all sources that are not already in the register file.
      //
      // This happens before freeing killed sources, since we need all sources in
      // the register file simultaneously.
      if (AnySpilled) {
        for (auto s = 0; s < IR::GetRAArgs(IROp->Op); ++s) {
          if (!IsValidArg(IROp->Args[s])) {
            continue;
          }

          Ref Old = IR->GetNode(IROp->Args[s]);
          LOGMAN_THROW_A_FMT(IsOld(Old), "before remapping");

          if (!IsInRegisterFile(Old)) {
            IREmit->SetWriteCursorBefore(CodeNode);
            Ref Fill = InsertFill(Old);

            Remap(Old, Fill);
            AssignReg(IR->GetOp<IROp_Header>(Fill), Fill, IROp);
          }
        }
      }

      for (auto s = 0; s < IR::GetRAArgs(IROp->Op); ++s) {
        if (IROp->Args[s].IsInvalid()) {
          continue;
        }

        SourceIndex--;
        LOGMAN_THROW_AA_FMT(SourceIndex >= 0, "Consistent source count");

        Ref Old = IR->GetNode(IROp->Args[s]);

        if (!SourcesNextUses[SourceIndex]) {
          auto Reg = SSAToReg[IR->GetID(Map(Old)).Value];
          if (!Reg.IsInvalid()) {
            LOGMAN_THROW_A_FMT(IsInRegisterFile(Old), "sources in file");
            FreeReg(Reg);
          }
        }

        NextUses[IR->GetID(Old).Value] = SourcesNextUses[SourceIndex];
      }

      // Assign destinations.
      if (GetHasDest(IROp->Op)) {
        AssignReg(IROp, CodeNode, IROp);
      }

      // Remap sources last, since AssignReg can shuffle.
      if (!SSAToNewSSA.empty()) {
        for (auto s = 0; s < IR::GetRAArgs(IROp->Op); ++s) {
          Ref Remapped = SSAToNewSSA[IROp->Args[s].ID().Value];

          if (Remapped != nullptr) {
            IREmit->ReplaceNodeArgument(CodeNode, s, Remapped);
          }
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

  PreferredReg.clear();
  SSAToNewSSA.clear();
  NewSSAToSSA.clear();
  SSAToReg.clear();
  SpillSlots.clear();
  NextUses.clear();
}

fextl::unique_ptr<IR::RegisterAllocationPass> CreateRegisterAllocationPass() {
  return fextl::make_unique<ConstrainedRAPass>();
}
} // namespace FEXCore::IR
