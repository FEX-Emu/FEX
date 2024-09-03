// SPDX-License-Identifier: MIT
/*
$info$
tags: ir|opts
desc: This is not used right now, possibly broken
$end_info$
*/

#include "FEXCore/Core/X86Enums.h"
#include "FEXCore/Utils/MathUtils.h"
#include "FEXCore/fextl/deque.h"
#include "Interface/IR/IR.h"
#include "Interface/IR/IREmitter.h"

#include <FEXCore/IR/IR.h>
#include <FEXCore/Utils/Profiler.h>

#include "Interface/IR/PassManager.h"

#include <array>
#include <bit>
#include <memory>

// Flag bit flags
#define FLAG_V (1U << 0)
#define FLAG_C (1U << 1)
#define FLAG_Z (1U << 2)
#define FLAG_N (1U << 3)
#define FLAG_A (1U << 4)
#define FLAG_P (1U << 5)

#define FLAG_ZCV (FLAG_Z | FLAG_C | FLAG_V)
#define FLAG_NZCV (FLAG_N | FLAG_ZCV)
#define FLAG_ALL (FLAG_NZCV | FLAG_A | FLAG_P)

namespace FEXCore::IR {

struct FlagInfo {
  // If set, all following fields are zero, used for a quick exit.
  bool Trivial;

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
};

class DeadFlagCalculationEliminination final : public FEXCore::IR::Pass {
public:
  void Run(IREmitter* IREmit) override;

private:
  FlagInfo Classify(IROp_Header* Node);
  unsigned FlagForReg(unsigned Reg);
  unsigned FlagsForCondClassType(CondClassType Cond);
  bool EliminateDeadCode(IREmitter* IREmit, Ref CodeNode, IROp_Header* IROp);
  void FoldCompareBranch(IREmitter* IREmit, IRListView& CurrentIR, IROp_CondJump* Op, Ref CodeNode);
  void FoldAXFLAG(IREmitter* IREmit, IRListView& CurrentIR, IROp_CondJump* Op, Ref CodeNode);
  CondClassType X86ToArmFloatCond(CondClassType X86);
  bool ProcessBlock(IREmitter* IREmit, IRListView& CurrentIR, Ref Block, fextl::vector<uint8_t>& FlagsIn, bool& ReadsParity);
  void OptimizeParity(IREmitter* IREmit, IRListView& CurrentIR);
};

unsigned DeadFlagCalculationEliminination::FlagForReg(unsigned Reg) {
  return Reg == Core::CPUState::PF_AS_GREG ? FLAG_P : Reg == Core::CPUState::AF_AS_GREG ? FLAG_A : 0;
};

unsigned DeadFlagCalculationEliminination::FlagsForCondClassType(CondClassType Cond) {
  switch (Cond) {
  case COND_AL: return 0;

  case COND_MI:
  case COND_PL: return FLAG_N;

  case COND_EQ:
  case COND_NEQ: return FLAG_Z;

  case COND_UGE:
  case COND_ULT: return FLAG_C;

  case COND_VS:
  case COND_VC:
  case COND_FU:
  case COND_FNU: return FLAG_V;

  case COND_UGT:
  case COND_ULE: return FLAG_Z | FLAG_C;

  case COND_SGE:
  case COND_SLT:
  case COND_FLU:
  case COND_FGE: return FLAG_N | FLAG_V;

  case COND_SGT:
  case COND_SLE:
  case COND_FLEU:
  case COND_FGT: return FLAG_N | FLAG_Z | FLAG_V;

  default: LOGMAN_THROW_AA_FMT(false, "unknown cond class type"); return FLAG_NZCV;
  }
}

FlagInfo DeadFlagCalculationEliminination::Classify(IROp_Header* IROp) {
  switch (IROp->Op) {
  case OP_ANDWITHFLAGS:
    return {
      .Write = FLAG_NZCV,
      .Replacement = OP_AND,
      .ReplacementNoWrite = OP_TESTNZ,
    };

  case OP_ADDWITHFLAGS:
    return {
      .Write = FLAG_NZCV,
      .Replacement = OP_ADD,
      .ReplacementNoWrite = OP_ADDNZCV,
    };

  case OP_SUBWITHFLAGS:
    return {
      .Write = FLAG_NZCV,
      .Replacement = OP_SUB,
      .ReplacementNoWrite = OP_SUBNZCV,
    };

  case OP_ADCWITHFLAGS:
    return {
      .Read = FLAG_C,
      .Write = FLAG_NZCV,
      .Replacement = OP_ADC,
      .ReplacementNoWrite = OP_ADCNZCV,
    };

  case OP_ADCZEROWITHFLAGS:
    return {
      .Read = FLAG_C,
      .Write = FLAG_NZCV,
      .Replacement = OP_ADCZERO,
    };

  case OP_SBBWITHFLAGS:
    return {
      .Read = FLAG_C,
      .Write = FLAG_NZCV,
      .Replacement = OP_SBB,
      .ReplacementNoWrite = OP_SBBNZCV,
    };

  case OP_SHIFTFLAGS:
    // _ShiftFlags conditionally sets NZCV+PF, which we model here as a
    // read-modify-write. Logically, it also conditionally makes AF undefined,
    // which we model by omitting AF from both Read and Write sets (since
    // "cond ? AF : undef" may be optimized to "AF").
    return {
      .Read = FLAG_NZCV | FLAG_P,
      .Write = FLAG_NZCV | FLAG_P,
      .CanEliminate = true,
    };

  case OP_ROTATEFLAGS:
    // _RotateFlags conditionally sets CV, again modeled as RMW.
    return {
      .Read = FLAG_C | FLAG_V,
      .Write = FLAG_C | FLAG_V,
      .CanEliminate = true,
    };

  case OP_RDRAND: return {.Write = FLAG_NZCV};

  case OP_ADDNZCV:
  case OP_SUBNZCV:
  case OP_TESTNZ:
  case OP_FCMP:
  case OP_STORENZCV:
    return {
      .Write = FLAG_NZCV,
      .CanEliminate = true,
    };

  case OP_AXFLAG:
    // Per the Arm spec, axflag reads Z/V/C but not N. It writes all flags.
    return {
      .Read = FLAG_ZCV,
      .Write = FLAG_NZCV,
      .CanEliminate = true,
    };

  case OP_CMPPAIRZ:
    return {
      .Write = FLAG_Z,
      .CanEliminate = true,
    };

  case OP_CARRYINVERT:
    return {
      .Read = FLAG_C,
      .Write = FLAG_C,
      .CanEliminate = true,
    };

  case OP_SETSMALLNZV:
    return {
      .Write = FLAG_N | FLAG_Z | FLAG_V,
      .CanEliminate = true,
    };

  case OP_LOADNZCV: return {.Read = FLAG_NZCV};

  case OP_ADC:
  case OP_ADCZERO:
  case OP_SBB: return {.Read = FLAG_C};

  case OP_ADCNZCV:
  case OP_SBBNZCV:
    return {
      .Read = FLAG_C,
      .Write = FLAG_NZCV,
      .CanEliminate = true,
    };

  case OP_NZCVSELECT:
  case OP_NZCVSELECTINCREMENT: {
    auto Op = IROp->CW<IR::IROp_NZCVSelect>();
    return {.Read = FlagsForCondClassType(Op->Cond)};
  }

  case OP_NEG: {
    auto Op = IROp->CW<IR::IROp_Neg>();
    return {.Read = FlagsForCondClassType(Op->Cond)};
  }

  case OP_CONDJUMP: {
    auto Op = IROp->CW<IR::IROp_CondJump>();
    if (!Op->FromNZCV) {
      break;
    }

    return {.Read = FlagsForCondClassType(Op->Cond)};
  }

  case OP_CONDSUBNZCV:
  case OP_CONDADDNZCV: {
    auto Op = IROp->CW<IR::IROp_CondAddNZCV>();
    return {
      .Read = FlagsForCondClassType(Op->Cond),
      .Write = FLAG_NZCV,
      .CanEliminate = true,
    };
  }

  case OP_RMIFNZCV: {
    auto Op = IROp->CW<IR::IROp_RmifNZCV>();

    static_assert(FLAG_N == (1 << 3), "rmif mask lines up with our bits");
    static_assert(FLAG_Z == (1 << 2), "rmif mask lines up with our bits");
    static_assert(FLAG_C == (1 << 1), "rmif mask lines up with our bits");
    static_assert(FLAG_V == (1 << 0), "rmif mask lines up with our bits");

    return {
      .Write = Op->Mask,
      .CanEliminate = true,
    };
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
    return {
      .Write = Flags,
      .CanEliminate = true,
    };
  }

  case OP_LOADREGISTER: {
    auto Op = IROp->CW<IR::IROp_LoadRegister>();
    if (Op->Class != GPRClass) {
      break;
    }

    return {.Read = FlagForReg(Op->Reg)};
  }

  case OP_STOREREGISTER: {
    auto Op = IROp->CW<IR::IROp_StoreRegister>();
    if (Op->Class != GPRClass) {
      break;
    }

    unsigned Flag = FlagForReg(Op->Reg);

    return {
      .Write = Flag,
      .CanEliminate = Flag != 0,
    };
  }

  default: break;
  }

  return {.Trivial = true};
}

// General purpose dead code elimination. Returns whether flag handling should
// be skipped (because it was removed or could not possibly affect flags).
bool DeadFlagCalculationEliminination::EliminateDeadCode(IREmitter* IREmit, Ref CodeNode, IROp_Header* IROp) {
  // Can't remove anything used or with side effects.
  if (CodeNode->GetUses() > 0 || IR::HasSideEffects(IROp->Op)) {
    return false;
  }

  switch (IROp->Op) {
  case OP_SYSCALL: {
    auto Op = IROp->C<IR::IROp_Syscall>();
    if ((Op->Flags & IR::SyscallFlags::NOSIDEEFFECTS) != IR::SyscallFlags::NOSIDEEFFECTS) {
      return false;
    }

    break;
  }
  case OP_INLINESYSCALL: {
    auto Op = IROp->C<IR::IROp_Syscall>();
    if ((Op->Flags & IR::SyscallFlags::NOSIDEEFFECTS) != IR::SyscallFlags::NOSIDEEFFECTS) {
      return false;
    }

    break;
  }

  // If the result of the atomic fetch is completely unused, convert it to a non-fetching atomic operation.
  case OP_ATOMICFETCHADD: IROp->Op = OP_ATOMICADD; return true;
  case OP_ATOMICFETCHSUB: IROp->Op = OP_ATOMICSUB; return true;
  case OP_ATOMICFETCHAND: IROp->Op = OP_ATOMICAND; return true;
  case OP_ATOMICFETCHCLR: IROp->Op = OP_ATOMICCLR; return true;
  case OP_ATOMICFETCHOR: IROp->Op = OP_ATOMICOR; return true;
  case OP_ATOMICFETCHXOR: IROp->Op = OP_ATOMICXOR; return true;
  case OP_ATOMICFETCHNEG: IROp->Op = OP_ATOMICNEG; return true;
  default: break;
  }

  IREmit->Remove(CodeNode);
  return true;
}

void DeadFlagCalculationEliminination::FoldCompareBranch(IREmitter* IREmit, IRListView& CurrentIR, IROp_CondJump* Op, Ref CodeNode) {
  // Pattern match a branch fed by a compare. We could also handle bit tests
  // here, but tbz/tbnz has a limited offset range which we don't have a way to
  // deal with yet. Let's hope that's not a big deal.
  if (!Op->FromNZCV || !(Op->Cond == COND_NEQ || Op->Cond == COND_EQ)) {
    return;
  }

  auto Prev = CurrentIR.GetOp<IR::IROp_Header>(CodeNode->Header.Previous);
  if (Prev->Op != OP_SUBNZCV || Prev->Size < 4) {
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

  // The compare/test sets flags but does not write registers. Flags are dead
  // after the jump. The jump does not read flags anymore.  There is no
  // intervening instruction. Therefore the compare is dead.
  IREmit->Remove(CurrentIR.GetNode(CodeNode->Header.Previous));
}

CondClassType DeadFlagCalculationEliminination::X86ToArmFloatCond(CondClassType X86) {
  // Table of x86 condition codes that map to arm64 condition codes, in the
  // sense that fcmp+axflag+branch(x86) is equivalent to fcmp+branch(arm).
  //
  // E would be "equal or unordered", no condition code.
  // G would be "greater than or less than", no condition code.
  //
  // SF/OF conditions are trivial and therefore shouldn't actually be generated
  switch (X86) {
  case COND_UGE /* A  */: return {COND_FGE} /* GE */;
  case COND_UGT /* AE */: return {COND_FGT} /* GT */;
  case COND_ULT /* B  */: return {COND_SLT} /* LT */;
  case COND_ULE /* BE */: return {COND_SLE} /* LE */;
  case COND_SLE /* LE */: return {COND_SLE} /* LE */;
  default: return {COND_AL};
  }
}

void DeadFlagCalculationEliminination::FoldAXFLAG(IREmitter* IREmit, IRListView& CurrentIR, IROp_CondJump* Op, Ref CodeNode) {
  CondClassType ArmCond = X86ToArmFloatCond(Op->Cond);

  // Pattern match a branch fed by axflag.
  if (!Op->FromNZCV || ArmCond == COND_AL) {
    return;
  }

  auto Prev = CurrentIR.GetOp<IR::IROp_Header>(CodeNode->Header.Previous);
  if (Prev->Op != OP_AXFLAG) {
    return;
  }

  Op->Cond = ArmCond;
  IREmit->Remove(CurrentIR.GetNode(CodeNode->Header.Previous));
}

/**
 * @brief This pass removes dead code locally.
 */
bool DeadFlagCalculationEliminination::ProcessBlock(IREmitter* IREmit, IRListView& CurrentIR, Ref Block, fextl::vector<uint8_t>& FlagsIn,
                                                    bool& ReadsParity) {
  bool Progress = false;
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
    FlagsRead = FlagsIn[Op->TrueBlock.ID().Value] | FlagsIn[Op->FalseBlock.ID().Value];
  } else if (ExitOp->Op == IR::OP_JUMP) {
    FlagsRead = FlagsIn[ExitOp->Args[0].ID().Value];
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

      if (!Info.Trivial) {
        bool Eliminated = false;

        if ((FlagsRead & Info.Write) == 0) {
          if ((Info.CanEliminate || Info.Replacement) && CodeNode->GetUses() == 0) {
            IREmit->Remove(CodeNode);
            Eliminated = true;
            Progress = true;
          } else if (Info.Replacement) {
            IROp->Op = Info.Replacement;
            Progress = true;
          }
        } else {
          if (Info.ReplacementNoWrite && CodeNode->GetUses() == 0) {
            IROp->Op = Info.ReplacementNoWrite;
            Progress = true;
          } else if (IROp->Op == OP_TESTNZ && IROp->Size < 4 && !(FlagsRead & (FLAG_N | FLAG_C))) {
            // If we don't care about the sign or carry, we can optimize testnz.
            // Carry is inverted between testz and testnz so we check that too.
            IROp->Op = OP_TESTZ;
            Progress = true;
          }

          FlagsRead &= ~Info.Write;
        }

        // If we eliminated the instruction, we eliminate its read too. This
        // check is required to ensure the pass converges locally in a single
        // iteration.
        if (!Eliminated) {
          FlagsRead |= Info.Read;
        }
      } else if (IROp->Op == OP_PARITY) {
        ReadsParity = true;
      }
    }

    // Iterate in reverse
    if (CodeLast == CodeBegin) {
      break;
    }
    --CodeLast;
  }

  FlagsIn[CurrentIR.GetID(Block).Value] = FlagsRead;
  return Progress;
}

void DeadFlagCalculationEliminination::OptimizeParity(IREmitter* IREmit, IRListView& CurrentIR) {
  // Pass to determine if 8-bit parity is required.
  for (auto [Block, BlockHeader] : CurrentIR.GetBlocks()) {
    // TODO: This is local for now, but we should propagate this globally.
    bool Full = true;
    for (auto [CodeNode, IROp] : CurrentIR.GetCode(Block)) {
      if (IROp->Op == OP_STOREREGISTER) {
        auto Op = IROp->CW<IR::IROp_StoreRegister>();
        if (Op->Class != GPRClass || Op->Reg != Core::CPUState::PF_AS_GREG) {
          continue;
        }

        // Determine if we only write 0/1 to the parity flag.
        Full = true;
        auto Generator = CurrentIR.GetOp<IR::IROp_Header>(Op->Value);
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
  }
}

void DeadFlagCalculationEliminination::Run(IREmitter* IREmit) {
  FEXCORE_PROFILE_SCOPED("PassManager::DFE");

  auto CurrentIR = IREmit->ViewIR();
  fextl::vector<uint8_t> FlagsIn(CurrentIR.GetSSACount());
  fextl::deque<Ref> Worklist;
  bool ReadsParity = false;

  for (auto [BlockNode, BlockHeader] : CurrentIR.GetBlocks()) {
    // Initialize the map conservatiely
    FlagsIn[CurrentIR.GetID(BlockNode).Value] = FLAG_ALL;
    Worklist.push_front(BlockNode);
  }

  // Iterate until we hit a fixed point.
  //
  // XXX: This is slow. We should use the CFG to accelerate this.
  bool Progress;
  do {
    Progress = false;

    for (auto Block : Worklist) {
      Progress |= ProcessBlock(IREmit, CurrentIR, Block, FlagsIn, ReadsParity);
    }
  } while (Progress);

  // Fold compares into branches now that we're otherwise optimized. This needs
  // to run after eliminating carries etc and it needs the global flag metadata.
  // But it only needs to run once, we don't do it in the loop.
  for (auto Block : Worklist) {
    // Grab the jump
    auto BlockIROp = CurrentIR.GetOp<IR::IROp_CodeBlock>(Block);
    auto CodeLast = CurrentIR.at(BlockIROp->Last);
    --CodeLast;

    auto [ExitNode, ExitOp] = CodeLast();
    if (ExitOp->Op == IR::OP_CONDJUMP) {
      auto Op = ExitOp->CW<IR::IROp_CondJump>();
      uint32_t FlagsOut = FlagsIn[Op->TrueBlock.ID().Value] | FlagsIn[Op->FalseBlock.ID().Value];

      if ((FlagsOut & FLAG_NZCV) == 0) {
        FoldCompareBranch(IREmit, CurrentIR, Op, ExitNode);
        FoldAXFLAG(IREmit, CurrentIR, Op, ExitNode);
      }
    }
  }

  if (ReadsParity) {
    OptimizeParity(IREmit, CurrentIR);
  }
}

fextl::unique_ptr<FEXCore::IR::Pass> CreateDeadFlagCalculationEliminination() {
  return fextl::make_unique<DeadFlagCalculationEliminination>();
}

} // namespace FEXCore::IR
