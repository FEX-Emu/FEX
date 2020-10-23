#include "Interface/IR/PassManager.h"
#include "Interface/Core/OpcodeDispatcher.h"

namespace FEXCore::IR {

class DeadFlagStoreElimination final : public FEXCore::IR::Pass {
public:
  bool Run(IREmitter *IREmit) override;
};

struct FlagInfo {
  uint32_t reads { 0 };
  uint32_t writes { 0 };
  uint32_t kill { 0 };
};

/**
 * @brief This is a temporary pass to detect simple multiblock dead flag stores
 *
 * First pass computes which flags are read and written per block
 *
 * Second pass computes which flags are stored, but overwritten by the next block(s).
 * It also propagates this information a few times to catch dead flags across multiple blocks.
 *
 * Third pass removes the dead stores.
 *
 */
bool DeadFlagStoreElimination::Run(IREmitter *IREmit) {
  std::map<OrderedNode*, FlagInfo> FlagMap;

  bool Changed = false;
  auto CurrentIR = IREmit->ViewIR();

  // Pass 1
  // Compute flags read/writes per block
  // This is conservative and doesn't try to be smart about loads after writes
  {
    for (auto [BlockNode, BlockIROp] : CurrentIR.GetBlocks()) {
      for (auto [CodeNode, IROp] : CurrentIR.GetCode(BlockNode)) {

        if (IROp->Op == OP_STOREFLAG) {
          auto Op = IROp->CW<IR::IROp_StoreFlag>();
          FlagMap[BlockNode].writes |= 1 << Op->Flag;
        }
        else if  (IROp->Op == OP_INVALIDATEFLAGS) {
          FlagMap[BlockNode].writes = -1;
        }
        else if (IROp->Op == OP_LOADFLAG) {
          auto Op = IROp->CW<IR::IROp_LoadFlag>();
          FlagMap[BlockNode].reads |= 1 << Op->Flag;
        }

      }
    }
  }

  // Pass 2
  // Compute flags that are stored, but always ovewritten in the next blocks
  // Propagate the information a few times to eliminate more
  for (int i = 0; i < 5; i++)
  {
    for (auto [BlockNode, BlockIROp] : CurrentIR.GetBlocks()) {
      for (auto [CodeNode, IROp] : CurrentIR.GetCode(BlockNode)) {

        if (IROp->Op == OP_JUMP) {
          auto Op = IROp->CW<IR::IROp_Jump>();
          OrderedNode *TargetNode = CurrentIR.GetNode(Op->Header.Args[0]);

          // stores to remove are written by the next block but not read
          FlagMap[BlockNode].kill = FlagMap[TargetNode].writes & ~(FlagMap[TargetNode].reads);

          // Flags that are written by the next block can be considered as written by this block, if not read
          FlagMap[BlockNode].writes |= FlagMap[BlockNode].kill & ~FlagMap[BlockNode].reads;
        }
        else if (IROp->Op == OP_CONDJUMP) {
          auto Op = IROp->CW<IR::IROp_CondJump>();

          OrderedNode *TrueTargetNode = CurrentIR.GetNode(Op->Header.Args[1]);
          OrderedNode *FalseTargetNode = CurrentIR.GetNode(Op->Header.Args[2]);

          // stores to remove are written by the next blocks but not read
          FlagMap[BlockNode].kill = FlagMap[TrueTargetNode].writes & ~(FlagMap[TrueTargetNode].reads);
          FlagMap[BlockNode].kill &= FlagMap[FalseTargetNode].writes & ~(FlagMap[FalseTargetNode].reads);

          // Flags that are written by the next blocks can be considered as written by this block, if not read
          FlagMap[BlockNode].writes |= FlagMap[BlockNode].kill & ~FlagMap[BlockNode].reads;
        }
      }
    }
  }

  // Pass 3
  // Remove the dead stores
  {
    for (auto [BlockNode, BlockIROp] : CurrentIR.GetBlocks()) {
      for (auto [CodeNode, IROp] : CurrentIR.GetCode(BlockNode)) {

        if (IROp->Op == OP_STOREFLAG) {
          auto Op = IROp->CW<IR::IROp_StoreFlag>();
          // If this StoreFlag is never read, remove it
          if (FlagMap[BlockNode].kill & (1 << Op->Flag)) {
            IREmit->Remove(CodeNode);
            Changed = true;
          }
        }

      }
    }
  }

  return Changed;
}

FEXCore::IR::Pass* CreateDeadFlagStoreElimination() {
  return new DeadFlagStoreElimination{};
}

}
