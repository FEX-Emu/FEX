#include "Interface/IR/PassManager.h"
#include "Interface/Core/OpcodeDispatcher.h"

namespace FEXCore::IR {

class DeadGPRStoreElimination final : public FEXCore::IR::Pass {
public:
  bool Run(IREmitter *IREmit) override;
};

struct GPRInfo {
  uint32_t reads { 0 };
  uint32_t writes { 0 };
  uint32_t kill { 0 };
};

bool IsFullGPR(uint32_t Offset, uint8_t Size) {
  if (Size != 8)
    return false;
  if (Offset & 7)
    return false;
  
  if (Offset < 8 || Offset >= (17 * 8))
    return false;

  return true;
}

bool IsGPR(uint32_t Offset) {
  
  if (Offset < 8 || Offset >= (17 * 8))
    return false;

  return true;
}

uint32_t GPRBit(uint32_t Offset) {
  if (!IsGPR(Offset)) {
    return 0;
  }

  return  1 << ((Offset - 8)/8); 
}

/**
 * @brief This is a temporary pass to detect simple multiblock dead GPR stores
 *
 * First pass computes which GPRs are read and written per block
 *
 * Second pass computes which GPRs are stored, but overwritten by the next block(s).
 * It also propagates this information a few times to catch dead GPRs across multiple blocks.
 *
 * Third pass removes the dead stores.
 *
 */
bool DeadGPRStoreElimination::Run(IREmitter *IREmit) {
  std::map<OrderedNode*, GPRInfo> GPRMap;

  bool Changed = false;
  auto CurrentIR = IREmit->ViewIR();

  // Pass 1
  // Compute GPRs read/writes per block
  // This is conservative and doesn't try to be smart about loads after writes
  {
    for (auto [BlockNode, BlockIROp] : CurrentIR.GetBlocks()) {
      for (auto [CodeNode, IROp] : CurrentIR.GetCode(BlockNode)) {

        if (IROp->Op == OP_STORECONTEXT) {
          auto Op = IROp->CW<IR::IROp_StoreContext>();
          if (IsFullGPR(Op->Offset, IROp->Size))
            GPRMap[BlockNode].writes |= GPRBit(Op->Offset);
          else
            GPRMap[BlockNode].reads |= GPRBit(Op->Offset);
        }
        else if (IROp->Op == OP_STORECONTEXTINDEXED ||
               IROp->Op == OP_LOADCONTEXTINDEXED ||
               IROp->Op == OP_LOADCONTEXTPAIR ||
               IROp->Op == OP_STORECONTEXTPAIR) {
          // We can't track through these
          GPRMap[BlockNode].reads = -1;
        }
        else if (IROp->Op == OP_LOADCONTEXT) {
          auto Op = IROp->CW<IR::IROp_LoadContext>();
          GPRMap[BlockNode].reads |= GPRBit(Op->Offset);
        }

      }
    }
  }

  // Pass 2
  // Compute GPRs that are stored, but always ovewritten in the next blocks
  // Propagate the information a few times to eliminate more
  for (int i = 0; i < 5; i++)
  {
    for (auto [BlockNode, BlockIROp] : CurrentIR.GetBlocks()) {
      for (auto [CodeNode, IROp] : CurrentIR.GetCode(BlockNode)) {

        if (IROp->Op == OP_JUMP) {
          auto Op = IROp->CW<IR::IROp_Jump>();
          OrderedNode *TargetNode = CurrentIR.GetNode(Op->Header.Args[0]);

          // stores to remove are written by the next block but not read
          GPRMap[BlockNode].kill = GPRMap[TargetNode].writes & ~(GPRMap[TargetNode].reads) & ~GPRMap[BlockNode].reads;

          // GPRs that are written by the next block can be considered as written by this block, if not read
          GPRMap[BlockNode].writes |= GPRMap[BlockNode].kill & ~GPRMap[BlockNode].reads;
        }
        else if (IROp->Op == OP_CONDJUMP) {
          auto Op = IROp->CW<IR::IROp_CondJump>();

          OrderedNode *TrueTargetNode = CurrentIR.GetNode(Op->TrueBlock);
          OrderedNode *FalseTargetNode = CurrentIR.GetNode(Op->FalseBlock);

          // stores to remove are written by the next blocks but not read
          GPRMap[BlockNode].kill = GPRMap[TrueTargetNode].writes & ~(GPRMap[TrueTargetNode].reads) & ~GPRMap[BlockNode].reads;
          GPRMap[BlockNode].kill &= GPRMap[FalseTargetNode].writes & ~(GPRMap[FalseTargetNode].reads) & ~GPRMap[BlockNode].reads;

          // GPRs that are written by the next blocks can be considered as written by this block, if not read
          GPRMap[BlockNode].writes |= GPRMap[BlockNode].kill & ~GPRMap[BlockNode].reads;
        }
      }
    }
  }

  // Pass 3
  // Remove the dead stores
  {
    for (auto [BlockNode, BlockIROp] : CurrentIR.GetBlocks()) {
      for (auto [CodeNode, IROp] : CurrentIR.GetCode(BlockNode)) {

        if (IROp->Op == OP_STORECONTEXT) {
          auto Op = IROp->CW<IR::IROp_StoreContext>();
          // If this OP_STORECONTEXT is never read, remove it
          if (GPRMap[BlockNode].kill & GPRBit(Op->Offset)) {
            IREmit->Remove(CodeNode);
            //printf("Removed dead store %d, %d\n", Op->Offset, IROp->Size);  
            Changed = true;
          }
        }

      }
    }
  }

  return Changed;
}

FEXCore::IR::Pass* CreateDeadGPRStoreElimination() {
  return new DeadGPRStoreElimination{};
}

}