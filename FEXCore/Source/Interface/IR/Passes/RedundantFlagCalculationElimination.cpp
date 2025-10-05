// SPDX-License-Identifier: MIT
/*
$info$
tags: ir|opts
$end_info$
*/

#include "Interface/IR/IR.h"
#include "Interface/IR/IREmitter.h"
#include "Interface/IR/PassManager.h"

#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/IR/IR.h>
#include <FEXCore/Utils/CompilerDefs.h>
#include <FEXCore/Utils/MathUtils.h>
#include <FEXCore/Utils/Profiler.h>
#include <FEXCore/fextl/deque.h>
#include <FEXCore/fextl/vector.h>

// Flag bit flags
#define FLAG_V (1U << 0)
#define FLAG_C (1U << 1)
#define FLAG_Z (1U << 2)
#define FLAG_N (1U << 3)
#define FLAG_P (1U << 4)
#define FLAG_A (1U << 5)

#define FLAG_ZCV (FLAG_Z | FLAG_C | FLAG_V)
#define FLAG_NZCV (FLAG_N | FLAG_ZCV)
#define FLAG_ALL (FLAG_NZCV | FLAG_A | FLAG_P)

namespace FEXCore::IR {

struct FlagInfoUnpacked {
  // Set of flags read by the instruction.
  unsigned Read;

  // Set of flags written by the instruction. Happens AFTER the reads.
  unsigned Write;

  // If true, the instruction can be be eliminated if its flag writes can all be
  // eliminated.
  bool CanEliminate;

  // If set, the opcode can be replaced with Replacement if its flag writes can
  // all be eliminated, or ReplacementNoWrite if its register write can be
  // eliminated.
  IROps Replacement;
  IROps ReplacementNoWrite;

  // Needs speical handling
  bool Special;
};

struct FlagInfo {
  uint64_t Raw;

  static constexpr struct FlagInfo Pack(struct FlagInfoUnpacked F) {
    uint64_t R = F.Read | (F.Write << 8) | (F.CanEliminate << 16) | (((uint64_t)F.Replacement) << 32) |
                 ((uint64_t)F.ReplacementNoWrite << 48) | (F.Special ? (1ull << 63) : 0);
    return {.Raw = R};
  }

  bool Trivial() const {
    return Raw == 0;
  }

  unsigned Read() const {
    return Bits(0, 8);
  }

  unsigned Write() const {
    return Bits(8, 8);
  }

  bool CanEliminate() const {
    return Bits(16, 1);
  }

  bool Special() const {
    return Bits(63, 1);
  }

  IROps Replacement() const {
    return (IROps)Bits(32, 16);
  }

  IROps ReplacementNoWrite() const {
    return (IROps)Bits(48, 16);
  }

private:
  unsigned Bits(unsigned Start, unsigned Count) const {
    return (Raw >> Start) & ((1u << Count) - 1);
  }
};

struct BlockInfo {
  fextl::vector<uint32_t> Predecessors;
  Ref Node;
  uint8_t Flags;
  bool InWorklist;
};

struct ControlFlowGraph {
  fextl::vector<BlockInfo> BlockMap;
  IRListView& IR;

  void Init(fextl::deque<uint32_t>& Worklist, uint32_t BlockCount) {
    BlockMap.resize(BlockCount);

    for (unsigned ID = 0; ID < BlockCount; ++ID) {
      // Add the block with conservative flags and already in the worklist.
      auto Info = BlockInfo {{}, nullptr, FLAG_ALL, true};

      // Add some initial capacity
      Info.Predecessors.reserve(2);

      BlockMap[ID] = std::move(Info);
      Worklist.push_back(ID);
    }
  }

  BlockInfo* Get(uint32_t Block) {
    return &BlockMap[Block];
  }

  BlockInfo* Get(IROp_CodeBlock* Block) {
    return &BlockMap[Block->ID];
  }

  BlockInfo* Get(OrderedNodeWrapper Block) {
    return Get(IR.GetOp<IR::IROp_CodeBlock>(Block));
  }

  void RecordEdge(uint32_t From, OrderedNodeWrapper To) {
    auto Info = Get(To);
    Info->Predecessors.push_back(From);
  }

  void AddWorklist(fextl::deque<uint32_t>& Worklist, uint32_t Block) {
    auto Info = Get(Block);
    if (!Info->InWorklist) {
      Info->InWorklist = true;
      Worklist.push_front(Block);
    }
  }
};

class DeadFlagCalculationEliminination final : public FEXCore::IR::Pass {
public:
  void Run(IREmitter* IREmit) override;

private:
  FlagInfo Classify(IROp_Header* Node);
  unsigned FlagsForCondClassType(CondClass Cond);
  bool EliminateDeadCode(IREmitter* IREmit, Ref CodeNode, IROp_Header* IROp);
  void FoldBranch(IREmitter* IREmit, IRListView& CurrentIR, IROp_CondJump* Op, Ref CodeNode);
  CondClass X86ToArmFloatCond(CondClass X86);
  bool ProcessBlock(IREmitter* IREmit, IRListView& CurrentIR, Ref Block, ControlFlowGraph& CFG);
  void OptimizeParity(IREmitter* IREmit, IRListView& CurrentIR, ControlFlowGraph& CFG);
};

unsigned DeadFlagCalculationEliminination::FlagsForCondClassType(CondClass Cond) {
  switch (Cond) {
  case CondClass::AL: return 0;

  case CondClass::MI:
  case CondClass::PL: return FLAG_N;

  case CondClass::EQ:
  case CondClass::NEQ: return FLAG_Z;

  case CondClass::UGE:
  case CondClass::ULT: return FLAG_C;

  case CondClass::VS:
  case CondClass::VC:
  case CondClass::FU:
  case CondClass::FNU: return FLAG_V;

  case CondClass::UGT:
  case CondClass::ULE: return FLAG_Z | FLAG_C;

  case CondClass::SGE:
  case CondClass::SLT:
  case CondClass::FLU:
  case CondClass::FGE: return FLAG_N | FLAG_V;

  case CondClass::SGT:
  case CondClass::SLE:
  case CondClass::FLEU:
  case CondClass::FGT: return FLAG_N | FLAG_Z | FLAG_V;

  default: LOGMAN_THROW_A_FMT(false, "unknown cond class type"); return FLAG_NZCV;
  }
}

constexpr FlagInfo ClassifyConst(IROps Op) {
  switch (Op) {
  case OP_ANDWITHFLAGS:
    return FlagInfo::Pack({
      .Write = FLAG_NZCV,
      .Replacement = OP_AND,
      .ReplacementNoWrite = OP_TESTNZ,
    });

  case OP_ADDWITHFLAGS:
    return FlagInfo::Pack({
      .Write = FLAG_NZCV,
      .Replacement = OP_ADD,
      .ReplacementNoWrite = OP_ADDNZCV,
    });

  case OP_SUBWITHFLAGS:
    return FlagInfo::Pack({
      .Write = FLAG_NZCV,
      .Replacement = OP_SUB,
      .ReplacementNoWrite = OP_SUBNZCV,
    });

  case OP_ADCWITHFLAGS:
    return FlagInfo::Pack({
      .Read = FLAG_C,
      .Write = FLAG_NZCV,
      .Replacement = OP_ADC,
      .ReplacementNoWrite = OP_ADCNZCV,
    });

  case OP_ADCZEROWITHFLAGS:
    return FlagInfo::Pack({
      .Read = FLAG_C,
      .Write = FLAG_NZCV,
      .Replacement = OP_ADCZERO,
    });

  case OP_SBBWITHFLAGS:
    return FlagInfo::Pack({
      .Read = FLAG_C,
      .Write = FLAG_NZCV,
      .Replacement = OP_SBB,
      .ReplacementNoWrite = OP_SBBNZCV,
    });

  case OP_SHIFTFLAGS:
    // _ShiftFlags conditionally sets NZCV+PF, which we model here as a
    // read-modify-write. Logically, it also conditionally makes AF undefined,
    // which we model by omitting AF from both Read and Write sets (since
    // "cond ? AF : undef" may be optimized to "AF").
    return FlagInfo::Pack({
      .Read = FLAG_NZCV | FLAG_P,
      .Write = FLAG_NZCV | FLAG_P,
      .CanEliminate = true,
    });

  case OP_ROTATEFLAGS:
    // _RotateFlags conditionally sets CV, again modeled as RMW.
    return FlagInfo::Pack({
      .Read = FLAG_C | FLAG_V,
      .Write = FLAG_C | FLAG_V,
      .CanEliminate = true,
    });

  case OP_RDRAND: return FlagInfo::Pack({.Write = FLAG_NZCV});

  case OP_ADDNZCV:
  case OP_SUBNZCV:
  case OP_TESTNZ:
  case OP_FCMP:
  case OP_STORENZCV:
    return FlagInfo::Pack({
      .Write = FLAG_NZCV,
      .CanEliminate = true,
    });

  case OP_AXFLAG:
    // Per the Arm spec, axflag reads Z/V/C but not N. It writes all flags.
    return FlagInfo::Pack({
      .Read = FLAG_ZCV,
      .Write = FLAG_NZCV,
      .CanEliminate = true,
    });

  case OP_CMPPAIRZ:
    return FlagInfo::Pack({
      .Write = FLAG_Z,
      .CanEliminate = true,
    });

  case OP_CARRYINVERT:
    return FlagInfo::Pack({
      .Read = FLAG_C,
      .Write = FLAG_C,
      .CanEliminate = true,
    });

  case OP_SETSMALLNZV:
    return FlagInfo::Pack({
      .Write = FLAG_N | FLAG_Z | FLAG_V,
      .CanEliminate = true,
    });

  case OP_LOADNZCV: return FlagInfo::Pack({.Read = FLAG_NZCV});

  case OP_ADC:
  case OP_ADCZERO:
  case OP_SBB: return FlagInfo::Pack({.Read = FLAG_C});

  case OP_ADCNZCV:
  case OP_SBBNZCV:
    return FlagInfo::Pack({
      .Read = FLAG_C,
      .Write = FLAG_NZCV,
      .CanEliminate = true,
    });

  case OP_LOADPF: return FlagInfo::Pack({.Read = FLAG_P});
  case OP_LOADAF: return FlagInfo::Pack({.Read = FLAG_A});
  case OP_STOREPF: return FlagInfo::Pack({.Write = FLAG_P, .CanEliminate = true});
  case OP_STOREAF: return FlagInfo::Pack({.Write = FLAG_A, .CanEliminate = true});

  case OP_NZCVSELECT:
  case OP_NZCVSELECTV:
  case OP_NZCVSELECTINCREMENT:
  case OP_NEG:
  case OP_CONDJUMP:
  case OP_CONDSUBNZCV:
  case OP_CONDADDNZCV:
  case OP_RMIFNZCV:
  case OP_INVALIDATEFLAGS: return FlagInfo::Pack({.Special = true});
  default: return FlagInfo::Pack({});
  }
}

constexpr auto FlagInfos = std::invoke([] {
  std::array<FlagInfo, OP_LAST> ret = {};

  for (unsigned i = 0; i < OP_LAST; ++i) {
    ret[i] = ClassifyConst((IROps)i);
  }

  return ret;
});

FlagInfo DeadFlagCalculationEliminination::Classify(IROp_Header* IROp) {
  FlagInfo Info = FlagInfos[IROp->Op];
  if (!Info.Special()) {
    return Info;
  }

  switch (IROp->Op) {
  case OP_NZCVSELECT:
  case OP_NZCVSELECTINCREMENT: {
    auto Op = IROp->CW<IR::IROp_NZCVSelect>();
    return FlagInfo::Pack({.Read = FlagsForCondClassType(Op->Cond)});
  }

  case OP_NZCVSELECTV: {
    auto Op = IROp->CW<IR::IROp_NZCVSelectV>();
    return FlagInfo::Pack({.Read = FlagsForCondClassType(Op->Cond)});
  }

  case OP_NEG: {
    auto Op = IROp->CW<IR::IROp_Neg>();
    return FlagInfo::Pack({.Read = FlagsForCondClassType(Op->Cond)});
  }

  case OP_CONDJUMP: {
    auto Op = IROp->CW<IR::IROp_CondJump>();
    if (!Op->FromNZCV) {
      return FlagInfo::Pack({});
    }

    return FlagInfo::Pack({.Read = FlagsForCondClassType(Op->Cond)});
  }

  case OP_CONDSUBNZCV:
  case OP_CONDADDNZCV: {
    auto Op = IROp->CW<IR::IROp_CondAddNZCV>();
    return FlagInfo::Pack({
      .Read = FlagsForCondClassType(Op->Cond),
      .Write = FLAG_NZCV,
      .CanEliminate = true,
    });
  }

  case OP_RMIFNZCV: {
    auto Op = IROp->CW<IR::IROp_RmifNZCV>();

    static_assert(FLAG_N == (1 << 3), "rmif mask lines up with our bits");
    static_assert(FLAG_Z == (1 << 2), "rmif mask lines up with our bits");
    static_assert(FLAG_C == (1 << 1), "rmif mask lines up with our bits");
    static_assert(FLAG_V == (1 << 0), "rmif mask lines up with our bits");

    return FlagInfo::Pack({
      .Write = Op->Mask,
      .CanEliminate = true,
    });
  }

  case OP_INVALIDATEFLAGS: {
    auto Op = IROp->CW<IR::IROp_InvalidateFlags>();
    unsigned Flags = 0;

    // TODO: Make this translation less silly
    if (Op->Flags & (1u << X86State::RFLAG_SF_RAW_LOC)) {
      Flags |= FLAG_N;
    }

    if (Op->Flags & (1u << X86State::RFLAG_ZF_RAW_LOC)) {
      Flags |= FLAG_Z;
    }

    if (Op->Flags & (1u << X86State::RFLAG_CF_RAW_LOC)) {
      Flags |= FLAG_C;
    }

    if (Op->Flags & (1u << X86State::RFLAG_OF_RAW_LOC)) {
      Flags |= FLAG_V;
    }

    if (Op->Flags & (1u << X86State::RFLAG_PF_RAW_LOC)) {
      Flags |= FLAG_P;
    }

    if (Op->Flags & (1u << X86State::RFLAG_AF_RAW_LOC)) {
      Flags |= FLAG_A;
    }

    // The mental model of InvalidateFlags is writing undefined values to all
    // of the selected flags, allowing the write-after-write optimizations to
    // optimize invalidate-after-write for free.
    return FlagInfo::Pack({
      .Write = Flags,
      .CanEliminate = true,
    });
  }

  default: LOGMAN_THROW_A_FMT(false, "invalid special op"); FEX_UNREACHABLE;
  }

  FEX_UNREACHABLE;
}

// General purpose dead code elimination. Returns whether flag handling should
// be skipped (because it was removed or could not possibly affect flags).
bool DeadFlagCalculationEliminination::EliminateDeadCode(IREmitter* IREmit, Ref CodeNode, IROp_Header* IROp) {
  // Can't remove anything used or with side effects.
  if (CodeNode->GetUses() > 0 || IR::HasSideEffects(IROp->Op)) {
    return false;
  }

  IREmit->Remove(CodeNode);
  return true;
}

CondClass DeadFlagCalculationEliminination::X86ToArmFloatCond(CondClass X86) {
  // Table of x86 condition codes that map to arm64 condition codes, in the
  // sense that fcmp+axflag+branch(x86) is equivalent to fcmp+branch(arm).
  //
  // E would be "equal or unordered", no condition code.
  // G would be "greater than or less than", no condition code.
  //
  // SF/OF conditions are trivial and therefore shouldn't actually be generated
  switch (X86) {
  case CondClass::UGE /* A  */: return CondClass::FGE /* GE */;
  case CondClass::UGT /* AE */: return CondClass::FGT /* GT */;
  case CondClass::ULT /* B  */: return CondClass::SLT /* LT */;
  case CondClass::ULE /* BE */: return CondClass::SLE /* LE */;
  case CondClass::SLE /* LE */: return CondClass::SLE /* LE */;
  default: return CondClass::AL;
  }
}

void DeadFlagCalculationEliminination::FoldBranch(IREmitter* IREmit, IRListView& CurrentIR, IROp_CondJump* Op, Ref CodeNode) {
  // Skip past StoreRegisters at the end -- they don't touch flags.
  auto PrevWrap = CodeNode->Header.Previous;
  while (CurrentIR.GetOp<IR::IROp_Header>(PrevWrap)->Op == OP_STOREREGISTER ||
         CurrentIR.GetOp<IR::IROp_Header>(PrevWrap)->Op == OP_STOREPF || CurrentIR.GetOp<IR::IROp_Header>(PrevWrap)->Op == OP_STOREAF) {
    PrevWrap = CurrentIR.GetNode(PrevWrap)->Header.Previous;
  }

  auto Prev = CurrentIR.GetOp<IR::IROp_Header>(PrevWrap);
  if (Prev->Op == OP_AXFLAG) {
    // Pattern match a branch fed by AXFLAG.
    CondClass ArmCond = X86ToArmFloatCond(Op->Cond);
    if (ArmCond == CondClass::AL) {
      return;
    }

    Op->Cond = ArmCond;
  } else if (Prev->Op == OP_SUBNZCV) {
    // Pattern match a branch fed by a compare. We could also handle bit tests
    // here, but tbz/tbnz has a limited offset range which we don't have a way to
    // deal with yet. Let's hope that's not a big deal.
    if (!(Op->Cond == CondClass::NEQ || Op->Cond == CondClass::EQ) || (Prev->Size < OpSize::i32Bit)) {
      return;
    }

    auto SecondArg = CurrentIR.GetOp<IR::IROp_Header>(Prev->Args[1]);
    if (SecondArg->Op != OP_INLINECONSTANT || SecondArg->C<IR::IROp_InlineConstant>()->Constant != 0) {
      return;
    }

    // We've matched. Fold the compare into branch.
    IREmit->ReplaceNodeArgument(CodeNode, 0, CurrentIR.GetNode(Prev->Args[0]));
    IREmit->ReplaceNodeArgument(CodeNode, 1, CurrentIR.GetNode(Prev->Args[1]));
    Op->FromNZCV = false;
    Op->CompareSize = Prev->Size;
  } else {
    return;
  }

  // The compare/test/axflag sets flags but does not write registers. Flags are
  // dead after the jump. The jump does not read flags anymore.  There is no
  // intervening instruction. Therefore the compare is dead.
  IREmit->Remove(CurrentIR.GetNode(PrevWrap));
}

/**
 * @brief This pass removes dead code locally.
 */
bool DeadFlagCalculationEliminination::ProcessBlock(IREmitter* IREmit, IRListView& CurrentIR, Ref Block, ControlFlowGraph& CFG) {
  uint32_t FlagsRead = FLAG_ALL;

  // Reverse iteration is not yet working with the iterators
  auto BlockIROp = CurrentIR.GetOp<IR::IROp_CodeBlock>(Block);

  // We grab these nodes this way so we can iterate easily
  auto CodeBegin = CurrentIR.at(BlockIROp->Begin);
  auto CodeLast = CurrentIR.at(BlockIROp->Last);

  // Advance past EndBlock to get at the exit.
  --CodeLast;

  // Initialize the FlagsRead mask according to the exit instruction.
  auto [ExitNode, ExitOp] = CodeLast();
  if (ExitOp->Op == IR::OP_CONDJUMP) {
    auto Op = ExitOp->CW<IR::IROp_CondJump>();
    FlagsRead = CFG.Get(Op->TrueBlock)->Flags | CFG.Get(Op->FalseBlock)->Flags;
  } else if (ExitOp->Op == IR::OP_JUMP) {
    FlagsRead = CFG.Get(ExitOp->Args[0])->Flags;
  }

  // Iterate the block in reverse
  while (true) {
    auto [CodeNode, IROp] = CodeLast();

    // Optimizing flags can cause earlier flag reads to become dead but dead
    // flag reads should not impede optimiation of earlier dead flag writes.
    // We must DCE as we go to ensure we converge in a single iteration.
    if (!EliminateDeadCode(IREmit, CodeNode, IROp)) {
      // Optimiation algorithm: For each flag written...
      //
      //  If the flag has a later read (per FlagsRead), remove the flag from
      //  FlagsRead, since the reader is covered by this write.
      //
      //  Else, there is no later read, so remove the flag write (if we can).
      //  This is the active part of the optimization.
      //
      // Then, add each flag read to FlagsRead.
      //
      // This order is important: instructions that read-modify-write flags
      // (like adcs) first read flags, then write flags. Since we're iterating
      // the block backwards, that means we handle the write first.
      struct FlagInfo Info = Classify(IROp);

      if (!Info.Trivial()) {
        bool Eliminated = false;

        if ((FlagsRead & Info.Write()) == 0) {
          if ((Info.CanEliminate() || Info.Replacement()) && CodeNode->GetUses() == 0) {
            IREmit->Remove(CodeNode);
            Eliminated = true;
          } else if (Info.Replacement()) {
            IROp->Op = Info.Replacement();
          }
        } else if (Info.ReplacementNoWrite() && CodeNode->GetUses() == 0) {
          IROp->Op = Info.ReplacementNoWrite();
        }

        // If we don't care about the sign or carry, we can optimize testnz.
        // Carry is inverted between testz and testnz so we check that too. Note
        // this flag is outside of the if, since the TestNZ might result from
        // optimizing AndWithFlags, and we need to converge locally in a single
        // iteration.
        if (IROp->Op == OP_TESTNZ && IROp->Size < OpSize::i32Bit && !(FlagsRead & (FLAG_N | FLAG_C))) {
          IROp->Op = OP_TESTZ;
        }

        FlagsRead &= ~Info.Write();

        // If we eliminated the instruction, we eliminate its read too. This
        // check is required to ensure the pass converges locally in a single
        // iteration.
        if (!Eliminated) {
          FlagsRead |= Info.Read();
        }
      }
    }

    // Iterate in reverse
    if (CodeLast == CodeBegin) {
      break;
    }
    --CodeLast;
  }

  // For the purposes of global propagation, the content of our progress doesn't
  // matter -- only the difference in our final FlagsRead contributes to changes
  // in the predecessors.
  uint32_t OldFlagsRead = CFG.Get(BlockIROp->ID)->Flags;
  CFG.Get(BlockIROp->ID)->Flags = FlagsRead;
  return (OldFlagsRead != FlagsRead);
}

void DeadFlagCalculationEliminination::OptimizeParity(IREmitter* IREmit, IRListView& CurrentIR, ControlFlowGraph& CFG) {
  // Mapping for flags inside this pass.
  const uint8_t PARTIAL = 0;
  const uint8_t FULL = 1;

  // Initialize conservatively: all blocks need full parity. This initialization
  // matters for proper handling of backedges.
  for (auto [Block, BlockHeader] : CurrentIR.GetBlocks()) {
    auto ID = BlockHeader->C<IROp_CodeBlock>()->ID;
    CFG.Get(ID)->Flags = FULL;
  }

  for (auto [Block, BlockHeader] : CurrentIR.GetBlocks()) {
    const auto ID = BlockHeader->C<IROp_CodeBlock>()->ID;
    const auto& Predecessors = CFG.Get(ID)->Predecessors;
    bool Full = false;

    if (Predecessors.empty()) {
      // Conservatively assume there was full parity before the start block
      Full = true;
    } else {
      // If any predecessor needs full parity at the end, we need full parity.
      for (auto Pred : Predecessors) {
        Full |= (CFG.Get(Pred)->Flags == FULL);
      }
    }

    for (auto [CodeNode, IROp] : CurrentIR.GetCode(Block)) {
      if (IROp->Op == OP_STOREPF) {
        auto Op = IROp->CW<IR::IROp_StorePF>();
        auto Generator = CurrentIR.GetOp<IR::IROp_Header>(Op->Value);

        // Determine if we only write 0/1 to the parity flag.
        Full = true;
        if (Generator->Op == OP_NZCVSELECT) {
          auto C0 = CurrentIR.GetOp<IR::IROp_Header>(Generator->Args[0]);
          auto C1 = CurrentIR.GetOp<IR::IROp_Header>(Generator->Args[1]);
          if (C0->Op == C1->Op && C0->Op == OP_INLINECONSTANT) {
            auto IC0 = CurrentIR.GetOp<IR::IROp_InlineConstant>(Generator->Args[0]);
            auto IC1 = CurrentIR.GetOp<IR::IROp_InlineConstant>(Generator->Args[1]);

            // We need the full 8 if the constant has upper bits set.
            Full = (IC0->Constant | IC1->Constant) & ~1;
          }
        }
      } else if (IROp->Op == OP_PARITY && !Full) {
        // Eliminate parity calculations if it's only 1-bit.
        auto Parity = IROp->C<IROp_Parity>();
        Ref Value = CurrentIR.GetNode(Parity->Raw);

        if (Parity->Invert) {
          IREmit->SetWriteCursor(CodeNode);
          Value = IREmit->_Xor(OpSize::i32Bit, Value, IREmit->_InlineConstant(1));
        }

        IREmit->ReplaceUsesWithAfter(CodeNode, Value, CurrentIR.at(CodeNode));
        IREmit->Remove(CodeNode);
      }
    }

    // Record our final state for our successors to read.
    CFG.Get(ID)->Flags = Full ? FULL : PARTIAL;
  }
}

void DeadFlagCalculationEliminination::Run(IREmitter* IREmit) {
  FEXCORE_PROFILE_SCOPED("PassManager::DFE");

  auto CurrentIR = IREmit->ViewIR();
  fextl::deque<uint32_t> Worklist;

  // Initialize CFG
  ControlFlowGraph CFG {.IR = CurrentIR};
  CFG.Init(Worklist, CurrentIR.GetHeader()->BlockCount);

  // Gather CFG
  for (auto [BlockNode, BlockHeader] : CurrentIR.GetBlocks()) {
    auto Block = BlockHeader->C<IROp_CodeBlock>();
    auto CodeLast = CurrentIR.at(Block->Last);
    --CodeLast;
    auto [ExitNode, ExitOp] = CodeLast();
    if (ExitOp->Op == IR::OP_CONDJUMP) {
      auto Op = ExitOp->CW<IR::IROp_CondJump>();

      CFG.RecordEdge(Block->ID, Op->TrueBlock);
      CFG.RecordEdge(Block->ID, Op->FalseBlock);
    } else if (ExitOp->Op == IR::OP_JUMP) {
      CFG.RecordEdge(Block->ID, ExitOp->Args[0]);
    }

    CFG.Get(Block->ID)->Node = BlockNode;
  }

  // After processing a block, if we made progress, we must process its
  // predecessors to propagate globally. A block will be reprocessed only if
  // there is a loop backedge.
  for (; !Worklist.empty(); Worklist.pop_back()) {
    auto Block = Worklist.back();
    auto Info = CFG.Get(Block);
    Info->InWorklist = false;

    if (ProcessBlock(IREmit, CurrentIR, Info->Node, CFG)) {
      for (auto Pred : Info->Predecessors) {
        CFG.AddWorklist(Worklist, Pred);
      }
    }
  }

  // Fold compares into branches now that we're otherwise optimized. This needs
  // to run after eliminating carries etc and it needs the global flag metadata.
  // But it only needs to run once, we don't do it in the loop.
  for (auto [Block, _] : CurrentIR.GetBlocks()) {
    // Grab the jump
    auto BlockIROp = CurrentIR.GetOp<IR::IROp_CodeBlock>(Block);
    auto CodeLast = CurrentIR.at(BlockIROp->Last);
    --CodeLast;

    auto [ExitNode, ExitOp] = CodeLast();
    if (ExitOp->Op == IR::OP_CONDJUMP) {
      auto Op = ExitOp->CW<IR::IROp_CondJump>();
      uint32_t FlagsOut = CFG.Get(Op->TrueBlock)->Flags | CFG.Get(Op->FalseBlock)->Flags;

      if ((FlagsOut & FLAG_NZCV) == 0 && Op->FromNZCV) {
        FoldBranch(IREmit, CurrentIR, Op, ExitNode);
      }
    }
  }

  if (CurrentIR.GetHeader()->ReadsParity) {
    OptimizeParity(IREmit, CurrentIR, CFG);
  }
}

fextl::unique_ptr<FEXCore::IR::Pass> CreateDeadFlagCalculationEliminination() {
  return fextl::make_unique<DeadFlagCalculationEliminination>();
}

} // namespace FEXCore::IR
