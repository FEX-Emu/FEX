// SPDX-License-Identifier: MIT
/*
$info$
tags: ir|opts
desc: This is not used right now, possibly broken
$end_info$
*/

#include "FEXCore/Core/X86Enums.h"
#include "Interface/IR/IREmitter.h"

#include <FEXCore/IR/IR.h>
#include <FEXCore/Utils/Profiler.h>

#include "Interface/IR/PassManager.h"

#include <array>
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

  // If true, the opcode can be replaced with Replacement if its flag writes can
  // all be eliminated.
  bool CanReplace;
  IROps Replacement;
};

class DeadFlagCalculationEliminination final : public FEXCore::IR::Pass {
public:
  void Run(IREmitter* IREmit) override;

private:
  FlagInfo Classify(IROp_Header* Node);
  unsigned FlagForReg(unsigned Reg);
  unsigned FlagsForCondClassType(CondClassType Cond);
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
      .CanReplace = true,
      .Replacement = OP_AND,
    };

  case OP_ADDWITHFLAGS:
    return {
      .Write = FLAG_NZCV,
      .CanReplace = true,
      .Replacement = OP_ADD,
    };

  case OP_SUBWITHFLAGS:
    return {
      .Write = FLAG_NZCV,
      .CanReplace = true,
      .Replacement = OP_SUB,
    };

  case OP_ADCWITHFLAGS:
    return {
      .Read = FLAG_C,
      .Write = FLAG_NZCV,
      .CanReplace = true,
      .Replacement = OP_ADC,
    };

  case OP_SBBWITHFLAGS:
    return {
      .Read = FLAG_C,
      .Write = FLAG_NZCV,
      .CanReplace = true,
      .Replacement = OP_SBB,
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
  case OP_SBB: return {.Read = FLAG_C};

  case OP_ADCNZCV:
  case OP_SBBNZCV:
    return {
      .Read = FLAG_C,
      .Write = FLAG_NZCV,
      .CanEliminate = true,
    };

  case OP_NZCVSELECT: {
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

/**
 * @brief This pass removes flag calculations that will otherwise be unused INSIDE of that block
 */
void DeadFlagCalculationEliminination::Run(IREmitter* IREmit) {
  FEXCORE_PROFILE_SCOPED("PassManager::DFE");

  auto CurrentIR = IREmit->ViewIR();

  for (auto [BlockNode, BlockHeader] : CurrentIR.GetBlocks()) {
    // We model all flags as read at the end of the block, since this pass is
    // presently purely local. Optimizing this requires global anslysis.
    uint32_t FlagsRead = FLAG_ALL;

    // Reverse iteration is not yet working with the iterators
    auto BlockIROp = BlockHeader->CW<FEXCore::IR::IROp_CodeBlock>();

    // We grab these nodes this way so we can iterate easily
    auto CodeBegin = CurrentIR.at(BlockIROp->Begin);
    auto CodeLast = CurrentIR.at(BlockIROp->Last);

    // Iterate the block in reverse
    while (1) {
      auto [CodeNode, IROp] = CodeLast();

      // Optimizing flags can cause earlier flag reads to become dead but dead
      // flag reads should not impede optimiation of earlier dead flag writes.
      // We must DCE as we go to ensure we converge in a single iteration.
      //
      // TODO: This whole pass could be merged with DCE?
      bool HasSideEffects = IR::HasSideEffects(IROp->Op);
      if (!HasSideEffects && CodeNode->GetUses() == 0) {
        IREmit->Remove(CodeNode);
      } else {
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
            if (Info.CanEliminate && CodeNode->GetUses() == 0) {
              IREmit->Remove(CodeNode);
              Eliminated = true;
            } else if (Info.CanReplace) {
              IROp->Op = Info.Replacement;
            }
          } else {
            FlagsRead &= ~Info.Write;
          }

          // If we eliminated the instruction, we eliminate its read too. This
          // check is required to ensure the pass converges locally in a single
          // iteration.
          if (!Eliminated) {
            FlagsRead |= Info.Read;
          }
        }
      }

      // Iterate in reverse
      if (CodeLast == CodeBegin) {
        break;
      }
      --CodeLast;
    }
  }
}

fextl::unique_ptr<FEXCore::IR::Pass> CreateDeadFlagCalculationEliminination() {
  return fextl::make_unique<DeadFlagCalculationEliminination>();
}

} // namespace FEXCore::IR
