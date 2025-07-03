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
#include "Interface/Core/CPUID.h"
#include <FEXCore/IR/IR.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/Profiler.h>
#include <FEXCore/fextl/vector.h>
#include <bit>
#include <cstdint>

using namespace FEXCore;

namespace FEXCore::IR {
namespace {
  struct RegisterClass {
    uint32_t Available;
    uint32_t Count;

    // If bit R of Available is 0, then RegToSSA[R] is the node currently
    // allocated to R. Else, RegToSSA[R] is UNDEFINED, no need to clear this
    // when freeing registers.
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
  explicit ConstrainedRAPass(const FEXCore::CPUIDEmu* CPUID)
    : CPUID {CPUID} {}
  void Run(IREmitter* IREmit) override;
  void AddRegisters(IR::RegisterClassType Class, uint32_t RegisterCount) override;
  bool TryPostRAMerge(Ref LastNode, Ref CodeNode, IROp_Header* IROp);

private:
  RegisterClass Classes[IR::NumClasses];

  IREmitter* IREmit;
  IRListView* IR;
  const FEXCore::CPUIDEmu* CPUID;

  // Map of nodes to their preferred register, to coalesce load/store reg.
  fextl::vector<PhysicalRegister> PreferredReg;

  // Map of assigned registers. Does not grow beyond the initial set.
  fextl::vector<PhysicalRegister> SSAToReg;

  // Maps defs to their assigned spill slot + 1, or 0 if not spilled.
  fextl::vector<unsigned> SpillSlots;

  bool Rematerializable(IROp_Header* IROp) {
    return IROp->Op == OP_CONSTANT;
  }

  Ref InsertFill(Ref Node) {
    IROp_Header* IROp = IR->GetOp<IROp_Header>(Node);

    // Remat if we can
    if (Rematerializable(IROp)) {
      uint64_t Const = IROp->C<IR::IROp_Constant>()->Constant;
      return IREmit->_Constant(Const);
    }

    // Otherwise fill from stack
    uint32_t SlotPlusOne = SpillSlots[IR->GetID(Node).Value];
    LOGMAN_THROW_A_FMT(SlotPlusOne >= 1, "Node must have been spilled");

    RegisterClassType RegClass = GetRegClassFromNode(IR, IROp);
    return IREmit->_FillRegister(IROp->Size, IROp->ElementSize, SlotPlusOne - 1, RegClass);
  };

  // IP of next-use of each source. IPs are measured from the end of the
  // block, so we don't need to size the block up-front.
  fextl::vector<uint32_t> NextUses;

  bool AnySpilled;

  bool IsValidArg(OrderedNodeWrapper Arg) {
    if (Arg.IsInvalid()) {
      return false;
    }

    auto Op = IR->GetOp<IROp_Header>(Arg)->Op;
    return Op != OP_INLINECONSTANT && Op != OP_INLINEENTRYPOINTOFFSET;
  };

  RegisterClass* GetClass(PhysicalRegister Reg) {
    return &Classes[Reg.Class];
  };

  uint32_t GetRegBits(PhysicalRegister Reg) {
    return 1 << Reg.Reg;
  };

  bool IsInRegisterFile(Ref Node) {
    auto ID = IR->GetID(Node).Value;
    LOGMAN_THROW_A_FMT(ID < SSAToReg.size(), "Only old nodes looked up");

    PhysicalRegister Reg = SSAToReg[ID];
    RegisterClass* Class = GetClass(Reg);

    return (Class->Available & GetRegBits(Reg)) == 0 && Class->RegToSSA[Reg.Reg] == Node;
  };

  void FreeReg(PhysicalRegister Reg) {
    RegisterClass* Class = GetClass(Reg);
    uint32_t RegBits = GetRegBits(Reg);

    LOGMAN_THROW_A_FMT(!(Class->Available & RegBits), "Register double-free");

    Class->Available |= RegBits;
  };

  bool HasSource(IROp_Header* I, PhysicalRegister Reg) {
    for (auto s = 0; s < IR::GetRAArgs(I->Op); ++s) {
      if (I->Args[s].IsImmediate() && PhysicalRegister(I->Args[s]) == Reg) {
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
    uint8_t FlagOffset = Classes[GPRFixedClass.Val].Count - 2;

    if (IROp->Op == OP_STOREREGISTER) {
      return PhysicalRegister(Node);
    } else if (IROp->Op == OP_LOADPF || IROp->Op == OP_STOREPF) {
      return PhysicalRegister {GPRFixedClass, FlagOffset};
    } else if (IROp->Op == OP_LOADAF || IROp->Op == OP_STOREAF) {
      return PhysicalRegister {GPRFixedClass, (uint8_t)(FlagOffset + 1)};
    } else {
      const IROp_LoadRegister* Op = IROp->C<IR::IROp_LoadRegister>();

      LOGMAN_THROW_A_FMT(Op->Class == GPRClass || Op->Class == FPRClass, "SRA classes");
      if (Op->Class == FPRClass) {
        return PhysicalRegister {FPRFixedClass, (uint8_t)Op->Reg};
      } else {
        return PhysicalRegister {GPRFixedClass, (uint8_t)Op->Reg};
      }
    }
  };

  bool IsTrivial(Ref Node, const IROp_Header* Header) {
    switch (Header->Op) {
    case OP_ALLOCATEGPR: return true;
    case OP_ALLOCATEGPRAFTER: return true;
    case OP_ALLOCATEFPR: return true;
    case OP_RMWHANDLE: return PhysicalRegister(Node) == PhysicalRegister(Header->Args[0]);
    case OP_LOADREGISTER: return PhysicalRegister(Node) == DecodeSRAReg(Header, Node);
    case OP_STOREREGISTER: return PhysicalRegister(Header->Args[0]) == DecodeSRAReg(Header, Node);
    default: return false;
    }
  }

  // Helper macro to walk the set bits b in a 32-bit word x, using ffs to get
  // the next set bit and then clearing on each iteration.
#define foreach_bit(b, x) for (uint32_t __x = (x), b; ((b) = __builtin_ffs(__x) - 1, __x); __x &= ~(1 << (b)))

  void SpillReg(RegisterClass* Class, IROp_Header* Exclude) {
    // Find the best node to spill according to the "furthest-first" heuristic.
    // Since we defined IPs relative to the end of the block, the furthest
    // next-use has the /smallest/ unsigned IP.
    Ref Candidate = nullptr;
    uint32_t BestDistance = UINT32_MAX;
    [[maybe_unused]] uint8_t BestReg = ~0;
    uint32_t Allocated = ((1u << Class->Count) - 1) & ~Class->Available;

    foreach_bit(i, Allocated) {
      Ref Node = Class->RegToSSA[i];
      auto Reg = SSAToReg[IR->GetID(Node).Value];

      LOGMAN_THROW_A_FMT(Node != nullptr, "Invariant3");
      LOGMAN_THROW_A_FMT(Reg.Reg == i, "Invariant4");

      // Skip any source used by the current instruction, it is unspillable.
      if (!HasSource(Exclude, Reg)) {
        uint32_t NextUse = NextUses[IR->GetID(Node).Value];

        // Prioritize remat over spilling. It is typically cheaper to remat a
        // constant multiple times than to spill a single value.
        if (!Rematerializable(IR->GetOp<IROp_Header>(Node))) {
          NextUse += 100000;
        }

        if (NextUse < BestDistance) {
          BestDistance = NextUse;
          BestReg = i;
          Candidate = Node;
        }
      }
    }

    LOGMAN_THROW_A_FMT(Candidate != nullptr, "must've found something..");

    PhysicalRegister Reg = SSAToReg[IR->GetID(Candidate).Value];
    LOGMAN_THROW_A_FMT(Reg.Reg == BestReg, "Invariant6");

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
      uint32_t Slot = IR->GetHeader()->SpillSlots++;

      // We must map here in case we're spilling something we shuffled.
      auto SpillOp = IREmit->_SpillRegister(OrderedNodeWrapper::FromImmediate(Reg.Raw), Slot, RegisterClassType {Reg.Class});
      SpillOp.first->Header.Size = Header->Size;
      SpillOp.first->Header.ElementSize = Header->ElementSize;
      SpillSlots[Value] = Slot + 1;
    }

    // Now that we've spilled the value, take it out of the register file
    FreeReg(Reg);
    AnySpilled = true;
  };

  void RemapReg(Ref Node, PhysicalRegister Reg) {
    RegisterClass* Class = GetClass(Reg);
    Class->RegToSSA[Reg.Reg] = Node;

    uint32_t Index = IR->GetID(Node).Value;
    if (Index < SSAToReg.size()) {
      SSAToReg[Index] = Reg;
    }
  };

  // Record a given assignment of register Reg to Node.
  void SetReg(Ref Node, PhysicalRegister Reg) {
    RegisterClass* Class = GetClass(Reg);
    uint32_t RegBits = GetRegBits(Reg);

    LOGMAN_THROW_A_FMT((Class->Available & RegBits) == RegBits, "Precondition");

    Class->Available &= ~RegBits;

    RemapReg(Node, Reg);
    Node->Reg = Reg.Raw;
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
      auto Reg = PhysicalRegister(IROp->Args[TiedIdx]);
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
      auto After = PhysicalRegister(IROp->Args[0]);
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
    LOGMAN_THROW_A_FMT(Class->Available != 0, "Post-condition of spilling");
    unsigned Reg = std::countr_zero(Class->Available);
    SetReg(CodeNode, PhysicalRegister(ClassType, Reg));
  };
};

void ConstrainedRAPass::AddRegisters(IR::RegisterClassType Class, uint32_t RegisterCount) {
  LOGMAN_THROW_A_FMT(RegisterCount <= 31, "Up to 31 regs supported");

  Classes[Class].Count = RegisterCount;
}

inline bool KillMove(IROp_Header* LastOp, IROp_Header* IROp, Ref LastNode, Ref CodeNode) {
  // 32-bit moves in x86_64 are represented as a Bfe, detect them.
  if (LastOp->Op == OP_BFE && LastOp->C<IR::IROp_Bfe>()->lsb == 0 && LastOp->C<IR::IROp_Bfe>()->Width == 32) {
    auto Op = IROp->Op;

    if (Op == OP_AND) {
      // Rewrite "mov wA, wB; and xA, xA, xC" into "and wA, wB, wC", since
      // ((b & 0xffffffff) & c) == (b & c) & 0xffffffff.
      IROp->Size = OpSize::i32Bit;
      return true;
    } else if (IROp->Size == OpSize::i32Bit) {
      return Op == OP_OR || Op == OP_XOR || Op == OP_AND || Op == OP_SUB || Op == OP_LSHL || Op == OP_LSHR || Op == OP_ASHR;
    }
  }

  return LastOp->Op == OP_STOREREGISTER;
}

inline bool IsSignext(const IROp_Header* IROp, OrderedNodeWrapper Src, OpSize Size) {
  if (IROp->Op == OP_SBFE) {
    auto Sbfe = IROp->C<IR::IROp_Sbfe>();
    return Sbfe->Width == 1 && Sbfe->lsb == (IR::OpSizeAsBits(Size) - 1) && Sbfe->Src == Src;
  } else {
    return false;
  }
}

inline bool IsZero(const IROp_Header* IROp) {
  return IROp->Op == OP_CONSTANT && IROp->C<IROp_Constant>()->Constant == 0;
}

bool ConstrainedRAPass::TryPostRAMerge(Ref LastNode, Ref CodeNode, IROp_Header* IROp) {
  auto LastOp = IR->GetOp<IROp_Header>(LastNode);

  if (IROp->Op == OP_PUSH && LastOp->Op == OP_PUSH) {
    auto SP = PhysicalRegister(CodeNode);
    auto Push = IR->GetOp<IROp_Push>(CodeNode);
    auto LastPush = IR->GetOp<IROp_Push>(LastNode);

    if (LastOp->Size == IROp->Size && LastPush->ValueSize == Push->ValueSize && SP == PhysicalRegister(LastNode) &&
        SP == PhysicalRegister(IROp->Args[1]) && SP == PhysicalRegister(LastOp->Args[1]) && SP != PhysicalRegister(IROp->Args[0]) &&
        SP != PhysicalRegister(LastOp->Args[0]) && Push->ValueSize >= OpSize::i32Bit) {

      IREmit->SetWriteCursorBefore(LastNode);
      IREmit->_PushTwo(IROp->Size, Push->ValueSize, IROp->Args[0], LastOp->Args[0], IROp->Args[1]);
      IREmit->RemovePostRA(CodeNode);
      return true;
    }
  } else if (IROp->Op == OP_POP) {
    auto SP = PhysicalRegister(IROp->Args[0]);

    if (LastOp->Op == OP_POP && LastOp->Size == IROp->Size && IROp->Size >= OpSize::i32Bit && SP == PhysicalRegister(LastOp->Args[0])) {
      IREmit->SetWriteCursorBefore(LastNode);
      IREmit->_PopTwo(IROp->Size, IROp->Args[0], LastOp->Args[1], IROp->Args[1]);
      IREmit->RemovePostRA(CodeNode);
      return true;
    }
  } else if ((IROp->Op == OP_DIV || IROp->Op == OP_UDIV) && IROp->Size >= OpSize::i32Bit) {
    // If Upper came from a sign/zero extension, we only need a 64-bit division.
    auto Op = IROp->CW<IR::IROp_Div>();
    if (!Op->Upper.IsInvalid() && PhysicalRegister(Op->Upper) == PhysicalRegister(LastNode)) {
      if (IROp->Op == OP_DIV ? IsSignext(LastOp, Op->Lower, IROp->Size) : IsZero(LastOp)) {
        Op->Upper.SetInvalid();
        return PhysicalRegister(LastNode) == PhysicalRegister(Op->OutRemainder);
      }
    }
  } else if (IROp->Op == OP_XGETBV && PhysicalRegister(IROp->Args[0]) == PhysicalRegister(LastNode) && LastOp->Op == OP_CONSTANT) {
    // Try to constant fold
    uint64_t ConstantFunction = LastOp->C<IROp_Constant>()->Constant;
    auto Op = IROp->CW<IR::IROp_XGetBV>();
    if (CPUID->DoesXCRFunctionReportConstantData(ConstantFunction)) {
      const auto Result = CPUID->RunXCRFunction(ConstantFunction);
      IREmit->SetWriteCursorBefore(CodeNode);
      IREmit->_Constant(Result.eax).Node->Reg = PhysicalRegister(Op->OutEAX).Raw;
      IREmit->_Constant(Result.edx).Node->Reg = PhysicalRegister(Op->OutEDX).Raw;
      IREmit->RemovePostRA(CodeNode);
      return false;
    }
  } else if (IROp->Op == OP_CPUID && PhysicalRegister(IROp->Args[0]) == PhysicalRegister(LastNode) && LastOp->Op == OP_CONSTANT) {
    // Try to constant fold. As a limitation of merging only 2 instructions, we
    // can only handle constant functions, not constant leafs. This could be
    // lifted if we generalized at a (significant) complexity cost.
    uint64_t ConstantFunction = LastOp->C<IROp_Constant>()->Constant;
    auto Op = IROp->CW<IR::IROp_CPUID>();

    const auto SupportsConstant = CPUID->DoesFunctionReportConstantData(ConstantFunction);
    if (SupportsConstant.SupportsConstantFunction == CPUIDEmu::SupportsConstant::CONSTANT &&
        SupportsConstant.NeedsLeaf != CPUIDEmu::NeedsLeafConstant::NEEDSLEAFCONSTANT) {
      const auto Result = CPUID->RunFunction(ConstantFunction, 0 /* leaf */);

      IREmit->SetWriteCursorBefore(CodeNode);
      IREmit->_Fence({FEXCore::IR::Fence_Inst});
      IREmit->_Constant(Result.eax).Node->Reg = PhysicalRegister(Op->OutEAX).Raw;
      IREmit->_Constant(Result.ebx).Node->Reg = PhysicalRegister(Op->OutEBX).Raw;
      IREmit->_Constant(Result.ecx).Node->Reg = PhysicalRegister(Op->OutECX).Raw;
      IREmit->_Constant(Result.edx).Node->Reg = PhysicalRegister(Op->OutEDX).Raw;
      IREmit->RemovePostRA(CodeNode);
      return false;
    }
  }

  // Merge moves that are immediately consumed.
  //
  // x86 code inserts such moves to workaround x86's 2-address code. Because
  // arm64 is 3-address code, we can optimize these out.
  //
  // Note we rely on the short-circuiting here.
  if (PhysicalRegister(LastNode) == PhysicalRegister(CodeNode) && KillMove(LastOp, IROp, LastNode, CodeNode)) {
    LOGMAN_THROW_A_FMT(!PhysicalRegister(CodeNode).IsInvalid(), "invariant");

    for (auto s = 0; s < IR::GetRAArgs(IROp->Op); ++s) {
      if (IROp->Args[s].IsImmediate() && PhysicalRegister(IROp->Args[s]) == PhysicalRegister(LastNode)) {
        IROp->Args[s].SetImmediate(PhysicalRegister(LastOp->Args[0]).Raw);
      }
    }

    return true;
  }

  return false;
}

void ConstrainedRAPass::Run(IREmitter* IREmit_) {
  FEXCORE_PROFILE_SCOPED("PassManager::RA");

  IREmit = IREmit_;
  auto IR_ = IREmit->ViewIR();
  IR = &IR_;

  PreferredReg.resize(IR->GetSSACount(), PhysicalRegister::Invalid());
  SSAToReg.resize(IR->GetSSACount(), PhysicalRegister::Invalid());
  NextUses.resize(IR->GetSSACount(), 0);
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
        for (int i = NumArgs - 1; i >= 0; --i) {
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
          auto Reg = DecodeSRAReg(IROp, CodeNode);

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
    int64_t SourceIndex = SourcesNextUses.size();

    // Last nontrivial instruction, for merging as we go.
    Ref LastNode = nullptr;

    // Forward pass: Assign registers, spilling & optimizing as we go.
    for (auto [CodeNode, IROp] : IR->GetCode(BlockNode)) {
      // These do not read or write registers, and must be skipped for merging.
      // Since we'd be doing this check anyway for merging, do the check now so
      // we can skip the rest of the logic too.
      if (IROp->Op == OP_GUESTOPCODE || IROp->Op == OP_INLINECONSTANT) {
        continue;
      }

      // Static registers must be consistent at SRA load/store. Evict to ensure.
      if (auto Node = DecodeSRANode(IROp, CodeNode); Node != nullptr) {
        auto Reg = DecodeSRAReg(IROp, CodeNode);
        RegisterClass* Class = &Classes[Reg.Class];

        if (!(Class->Available & (1u << Reg.Reg))) {
          Ref Old = Class->RegToSSA[Reg.Reg];

          if (Old != Node) {
            // Before inserting instructions, we need to set the cursor and
            // reset LastNode so we don't merge across an inserted copy.
            // Otherwise, we would erroneously miss the copy when determining if
            // we can merge, and end up unsoundly merging a mov+xchg sequence.
            IREmit->SetWriteCursorBefore(CodeNode);
            LastNode = nullptr;

            Ref Copy;

            if (Reg.Class == FPRFixedClass) {
              IROp_Header* Header = IR->GetOp<IROp_Header>(Old);
              Copy = IREmit->_VMov(Header->Size, OrderedNodeWrapper::FromImmediate(Reg.Raw));
            } else {
              Copy = IREmit->_Copy(OrderedNodeWrapper::FromImmediate(Reg.Raw));
            }

            FreeReg(Reg);
            AssignReg(IR->GetOp<IROp_Header>(Copy), Copy, IROp);
            RemapReg(Old, PhysicalRegister(Copy));
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

          if (!IsInRegisterFile(Old)) {
            IREmit->SetWriteCursorBefore(CodeNode);
            LastNode = nullptr;

            Ref Fill = InsertFill(Old);

            AssignReg(IR->GetOp<IROp_Header>(Fill), Fill, IROp);
            RemapReg(Old, PhysicalRegister(Fill));
          }
        }
      }

      for (auto s = 0; s < IR::GetRAArgs(IROp->Op); ++s) {
        if (IROp->Args[s].IsInvalid()) {
          continue;
        }

        Ref Node = IR->GetNode(IROp->Args[s]);
        auto ID = IR->GetID(Node).Value;
        auto Reg = SSAToReg[ID];

        SourceIndex--;
        LOGMAN_THROW_A_FMT(SourceIndex >= 0, "Consistent source count");

        if (!Reg.IsInvalid()) {
          IROp->Args[s].SetImmediate(Reg.Raw);

          if (!SourcesNextUses[SourceIndex]) {
            LOGMAN_THROW_A_FMT(IsInRegisterFile(Node), "sources in file");
            FreeReg(Reg);
          }
        }

        NextUses[ID] = SourcesNextUses[SourceIndex];
      }

      // Assign destinations.
      if (GetHasDest(IROp->Op) && PhysicalRegister(CodeNode).IsInvalid()) {
        AssignReg(IROp, CodeNode, IROp);
      }

      if (IsTrivial(CodeNode, IROp)) {
        // Delete instructions that only exist for RA
        IREmit->RemovePostRA(CodeNode);
      } else if (LastNode && TryPostRAMerge(LastNode, CodeNode, IROp)) {
        // Merge adjacent instructions
        IREmit->RemovePostRA(LastNode);
        LastNode = nullptr;
      } else {
        LastNode = CodeNode;
      }
    }

    LOGMAN_THROW_A_FMT(SourceIndex == 0, "Consistent source count in block");
  }

  PreferredReg.clear();
  SSAToReg.clear();
  SpillSlots.clear();
  NextUses.clear();

  IR->GetHeader()->PostRA = true;
}

fextl::unique_ptr<IR::RegisterAllocationPass> CreateRegisterAllocationPass(const FEXCore::CPUIDEmu* CPUID) {
  return fextl::make_unique<ConstrainedRAPass>(CPUID);
}
} // namespace FEXCore::IR
