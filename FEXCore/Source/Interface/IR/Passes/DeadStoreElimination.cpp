// SPDX-License-Identifier: MIT
/*
$info$
tags: ir|opts
desc: Cross block store-after-store elimination
$end_info$
*/

#include "Interface/IR/IREmitter.h"
#include "Interface/IR/PassManager.h"

#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/IR/IR.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/Profiler.h>
#include <FEXCore/fextl/unordered_map.h>

#include <memory>
#include <stddef.h>
#include <stdint.h>

namespace FEXCore::IR {

constexpr int PropagationRounds = 5;

class DeadStoreElimination final : public FEXCore::IR::Pass {
public:
  explicit DeadStoreElimination() {}
  void Run(IREmitter* IREmit) override;

private:
  uint64_t FPRBit(RegisterClassType Class, uint32_t Reg) const {
    return (Class == FPRClass) ? (1UL << Reg) : 0;
  }
};

struct FlagInfo {
  uint64_t reads {0};
  uint64_t writes {0};
  uint64_t kill {0};
};


struct GPRInfo {
  uint32_t reads {0};
  uint32_t writes {0};
  uint32_t kill {0};
};

uint32_t GPRBit(RegisterClassType Class, uint32_t Reg) {
  return (Class == GPRClass) ? (1U << Reg) : 0;
}

struct FPRInfo {
  uint32_t reads {0};
  uint32_t writes {0};
  uint32_t kill {0};
};

struct Info {
  FlagInfo flag;
  GPRInfo gpr;
  FPRInfo fpr;
};

/**
 * @brief This is a temporary pass to detect simple multiblock dead flag/gpr/fpr stores
 *
 * First pass computes which flags/gprs/fprs are read and written per block
 *
 * Second pass computes which flags/gprs/fprs are stored, but overwritten by the next block(s).
 * It also propagates this information a few times to catch dead flags/gprs/fprs across multiple blocks.
 *
 * Third pass removes the dead stores.
 *
 */
void DeadStoreElimination::Run(IREmitter* IREmit) {
  FEXCORE_PROFILE_SCOPED("PassManager::DSE");

  fextl::unordered_map<OrderedNode*, Info> InfoMap;

  auto CurrentIR = IREmit->ViewIR();

  // Pass 1
  // Compute flags/gprs/fprs read/writes per block
  // This is conservative and doesn't try to be smart about loads after writes
  {
    for (auto [BlockNode, BlockIROp] : CurrentIR.GetBlocks()) {
      auto& BlockInfo = InfoMap[BlockNode];

      for (auto [CodeNode, IROp] : CurrentIR.GetCode(BlockNode)) {
        if (IROp->Op == OP_STOREFLAG) {
          auto Op = IROp->C<IR::IROp_StoreFlag>();

          BlockInfo.flag.writes |= 1UL << Op->Flag;
        } else if (IROp->Op == OP_INVALIDATEFLAGS) {
          auto Op = IROp->C<IR::IROp_InvalidateFlags>();

          BlockInfo.flag.writes |= Op->Flags;
        } else if (IROp->Op == OP_LOADFLAG) {
          auto Op = IROp->C<IR::IROp_LoadFlag>();

          BlockInfo.flag.reads |= 1UL << Op->Flag;
        } else if (IROp->Op == OP_LOADDF) {
          BlockInfo.flag.reads |= 1UL << X86State::RFLAG_DF_RAW_LOC;
        } else if (IROp->Op == OP_STOREREGISTER) {
          auto Op = IROp->C<IR::IROp_StoreRegister>();

          BlockInfo.gpr.writes |= GPRBit(Op->Class, Op->Reg);
          BlockInfo.fpr.writes |= FPRBit(Op->Class, Op->Reg);
        } else if (IROp->Op == OP_LOADREGISTER) {
          auto Op = IROp->C<IR::IROp_LoadRegister>();

          BlockInfo.gpr.reads |= GPRBit(Op->Class, Op->Reg);
          BlockInfo.fpr.reads |= FPRBit(Op->Class, Op->Reg);
        }
      }
    }
  }

  // Pass 2
  // Compute flags/gprs/fprs that are stored, but always ovewritten in the next blocks
  // Propagate the information a few times to eliminate more
  for (int i = 0; i < PropagationRounds; i++) {
    for (auto [BlockNode, BlockIROp] : CurrentIR.GetBlocks()) {
      auto CodeBlock = BlockIROp->C<IROp_CodeBlock>();

      auto IROp = CurrentIR.GetNode(CurrentIR.GetNode(CodeBlock->Last)->Header.Previous)->Op(CurrentIR.GetData());

      if (IROp->Op == OP_JUMP) {
        auto Op = IROp->C<IR::IROp_Jump>();
        OrderedNode* TargetNode = CurrentIR.GetNode(Op->Header.Args[0]);

        auto& BlockInfo = InfoMap[BlockNode];
        auto& TargetInfo = InfoMap[TargetNode];

        // stores to remove are written by the next block but not read
        BlockInfo.flag.kill = TargetInfo.flag.writes & ~(TargetInfo.flag.reads) & ~BlockInfo.flag.reads;
        BlockInfo.gpr.kill = TargetInfo.gpr.writes & ~(TargetInfo.gpr.reads) & ~BlockInfo.gpr.reads;
        BlockInfo.fpr.kill = TargetInfo.fpr.writes & ~(TargetInfo.fpr.reads) & ~BlockInfo.fpr.reads;

        // Flags that are written by the next block can be considered as written by this block, if not read
        BlockInfo.flag.writes |= BlockInfo.flag.kill & ~BlockInfo.flag.reads;
        BlockInfo.gpr.writes |= BlockInfo.gpr.kill & ~BlockInfo.gpr.reads;
        BlockInfo.fpr.writes |= BlockInfo.fpr.kill & ~BlockInfo.fpr.reads;
      } else if (IROp->Op == OP_CONDJUMP) {
        auto Op = IROp->C<IR::IROp_CondJump>();

        OrderedNode* TrueTargetNode = CurrentIR.GetNode(Op->TrueBlock);
        OrderedNode* FalseTargetNode = CurrentIR.GetNode(Op->FalseBlock);

        auto& BlockInfo = InfoMap[BlockNode];
        auto& TrueTargetInfo = InfoMap[TrueTargetNode];
        auto& FalseTargetInfo = InfoMap[FalseTargetNode];

        // stores to remove are written by the next blocks but not read
        BlockInfo.flag.kill = TrueTargetInfo.flag.writes & ~(TrueTargetInfo.flag.reads) & ~BlockInfo.flag.reads;
        BlockInfo.gpr.kill = TrueTargetInfo.gpr.writes & ~(TrueTargetInfo.gpr.reads) & ~BlockInfo.gpr.reads;
        BlockInfo.fpr.kill = TrueTargetInfo.fpr.writes & ~(TrueTargetInfo.fpr.reads) & ~BlockInfo.fpr.reads;

        BlockInfo.flag.kill &= FalseTargetInfo.flag.writes & ~(FalseTargetInfo.flag.reads) & ~BlockInfo.flag.reads;
        BlockInfo.gpr.kill &= FalseTargetInfo.gpr.writes & ~(FalseTargetInfo.gpr.reads) & ~BlockInfo.gpr.reads;
        BlockInfo.fpr.kill &= FalseTargetInfo.fpr.writes & ~(FalseTargetInfo.fpr.reads) & ~BlockInfo.fpr.reads;

        // Flags that are written by the next blocks can be considered as written by this block, if not read
        BlockInfo.flag.writes |= BlockInfo.flag.kill & ~BlockInfo.flag.reads;
        BlockInfo.gpr.writes |= BlockInfo.gpr.kill & ~BlockInfo.gpr.reads;
        BlockInfo.fpr.writes |= BlockInfo.fpr.kill & ~BlockInfo.fpr.reads;
      }
    }
  }

  // Pass 3
  // Remove the dead stores
  {
    for (auto [BlockNode, BlockIROp] : CurrentIR.GetBlocks()) {
      auto& BlockInfo = InfoMap[BlockNode];

      for (auto [CodeNode, IROp] : CurrentIR.GetCode(BlockNode)) {
        if (IROp->Op == OP_STOREFLAG) {
          auto Op = IROp->C<IR::IROp_StoreFlag>();

          // If this StoreFlag is never read, remove it
          if (BlockInfo.flag.kill & (1UL << Op->Flag)) {
            IREmit->Remove(CodeNode);
          }
        } else if (IROp->Op == OP_STOREREGISTER) {
          auto Op = IROp->C<IR::IROp_StoreRegister>();

          // If this OP_STOREREGISTER is never read, remove it
          if ((BlockInfo.gpr.kill & GPRBit(Op->Class, Op->Reg)) || (BlockInfo.fpr.kill & FPRBit(Op->Class, Op->Reg))) {
            IREmit->Remove(CodeNode);
          }
        }
      }
    }
  }
}

fextl::unique_ptr<FEXCore::IR::Pass> CreateDeadStoreElimination() {
  return fextl::make_unique<DeadStoreElimination>();
}

} // namespace FEXCore::IR
