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

// Return a bit representing a single GPR or FPR.
static inline uint64_t RegBit(RegisterClassType Class, uint32_t Reg) {
  uint32_t AdjustedReg = (Class == FPRClass) ? (32 + Reg) : Reg;

  return 1UL << AdjustedReg;
}

class DeadStoreElimination final : public FEXCore::IR::Pass {
public:
  void Run(IREmitter* IREmit) override;
};

struct ReadWriteKill {
  uint64_t reads {0};
  uint64_t writes {0};
  uint64_t kill {0};
};

struct Info {
  ReadWriteKill flag;
  ReadWriteKill reg;
};

/**
 * @brief This is a temporary pass to detect simple multiblock dead flag/reg stores
 *
 * First pass computes which flags/regs are read and written per block
 *
 * Second pass computes which flags/regs are stored, but overwritten by the next block(s).
 * It also propagates this information a few times to catch dead flags/regs across multiple blocks.
 *
 * Third pass removes the dead stores.
 *
 */
void DeadStoreElimination::Run(IREmitter* IREmit) {
  FEXCORE_PROFILE_SCOPED("PassManager::DSE");

  fextl::unordered_map<OrderedNode*, Info> InfoMap;

  auto CurrentIR = IREmit->ViewIR();

  // Pass 1
  // Compute flags/regs read/writes per block
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
          BlockInfo.reg.writes |= RegBit(Op->Class, Op->Reg);
        } else if (IROp->Op == OP_LOADREGISTER) {
          auto Op = IROp->C<IR::IROp_LoadRegister>();
          BlockInfo.reg.reads |= RegBit(Op->Class, Op->Reg);
        }
      }
    }
  }

  // Pass 2
  // Compute flags/registers that are stored, but always ovewritten in the next blocks
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
        BlockInfo.reg.kill = TargetInfo.reg.writes & ~(TargetInfo.reg.reads) & ~BlockInfo.reg.reads;

        // Flags that are written by the next block can be considered as written by this block, if not read
        BlockInfo.flag.writes |= BlockInfo.flag.kill & ~BlockInfo.flag.reads;
        BlockInfo.reg.writes |= BlockInfo.reg.kill & ~BlockInfo.reg.reads;
      } else if (IROp->Op == OP_CONDJUMP) {
        auto Op = IROp->C<IR::IROp_CondJump>();

        OrderedNode* TrueTargetNode = CurrentIR.GetNode(Op->TrueBlock);
        OrderedNode* FalseTargetNode = CurrentIR.GetNode(Op->FalseBlock);

        auto& BlockInfo = InfoMap[BlockNode];
        auto& TrueTargetInfo = InfoMap[TrueTargetNode];
        auto& FalseTargetInfo = InfoMap[FalseTargetNode];

        // stores to remove are written by the next blocks but not read
        BlockInfo.flag.kill = TrueTargetInfo.flag.writes & ~(TrueTargetInfo.flag.reads) & ~BlockInfo.flag.reads;
        BlockInfo.reg.kill = TrueTargetInfo.reg.writes & ~(TrueTargetInfo.reg.reads) & ~BlockInfo.reg.reads;

        BlockInfo.flag.kill &= FalseTargetInfo.flag.writes & ~(FalseTargetInfo.flag.reads) & ~BlockInfo.flag.reads;
        BlockInfo.reg.kill &= FalseTargetInfo.reg.writes & ~(FalseTargetInfo.reg.reads) & ~BlockInfo.reg.reads;

        // Flags that are written by the next blocks can be considered as written by this block, if not read
        BlockInfo.flag.writes |= BlockInfo.flag.kill & ~BlockInfo.flag.reads;
        BlockInfo.reg.writes |= BlockInfo.reg.kill & ~BlockInfo.reg.reads;
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
          if (BlockInfo.reg.kill & RegBit(Op->Class, Op->Reg)) {
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
