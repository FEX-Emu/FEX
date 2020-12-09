#include "Interface/IR/PassManager.h"
#include "Interface/Core/OpcodeDispatcher.h"

// Higher values might result in more stores getting eliminated but will make the optimization take more time
constexpr int PropagationRounds = 5;

namespace FEXCore::IR {

class DeadFPRStoreElimination final : public FEXCore::IR::Pass {
public:
  bool Run(IREmitter *IREmit) override;
};

struct FPRInfo {
  uint64_t reads { 0 };
  uint64_t writes { 0 };
  uint64_t kill { 0 };
};

bool IsFPR(uint32_t Offset) {
  
  auto begin = offsetof(FEXCore::Core::ThreadState, State.xmm[0][0]);
  auto end = offsetof(FEXCore::Core::ThreadState, State.xmm[17][0]);

  if (Offset < begin || Offset >= end)
    return false;

  return true;
}

bool IsTrackedWriteFPR(uint32_t Offset, uint8_t Size) {
  if (Size != 16 && Size != 8 && Size != 4)
    return false;
  if (Offset & 15)
    return false;

  return IsFPR(Offset);
}



uint64_t FPRBit(uint32_t Offset, uint32_t Size) {
  if (!IsFPR(Offset)) {
    return 0;
  }

  auto begin = offsetof(FEXCore::Core::ThreadState, State.xmm[0][0]);

  auto regn = (Offset - begin)/16;
  auto bitn = regn * 3;

  if (!IsTrackedWriteFPR(Offset, Size))
    return 7UL << (bitn);

  if (Size == 16)
    return  7UL << (bitn);
  else if (Size == 8)
    return  3UL << (bitn);
  else if (Size == 4)
    return  1UL << (bitn);
  else
    LogMan::Throw::A(false, "Unexpected FPR size %d", Size);
}

/**
 * @brief This is a temporary pass to detect simple multiblock dead FPR stores
 *
 * First pass computes which FPRs are read and written per block
 *
 * Second pass computes which FPRs are stored, but overwritten by the next block(s).
 * It also propagates this information a few times to catch dead FPRs across multiple blocks.
 *
 * Third pass removes the dead stores.
 *
 */
bool DeadFPRStoreElimination::Run(IREmitter *IREmit) {
  std::map<OrderedNode*, FPRInfo> FPRMap;

  bool Changed = false;
  auto CurrentIR = IREmit->ViewIR();

  // Pass 1
  // Compute FPRs read/writes per block
  // This is conservative and doesn't try to be smart about loads after writes
  {
    for (auto [BlockNode, BlockIROp] : CurrentIR.GetBlocks()) {
      for (auto [CodeNode, IROp] : CurrentIR.GetCode(BlockNode)) {
        if (IROp->Op == OP_STORECONTEXT) {
          auto Op = IROp->CW<IR::IROp_StoreContext>();
          if (IsTrackedWriteFPR(Op->Offset, IROp->Size))
            FPRMap[BlockNode].writes |= FPRBit(Op->Offset, IROp->Size);
          else
            FPRMap[BlockNode].reads |= FPRBit(Op->Offset, IROp->Size);
        }
        else if (IROp->Op == OP_STORECONTEXTINDEXED ||
               IROp->Op == OP_LOADCONTEXTINDEXED) {
          // We can't track through these
          FPRMap[BlockNode].reads = -1;
        }
        else if (IROp->Op == OP_LOADCONTEXT) {
          auto Op = IROp->CW<IR::IROp_LoadContext>();
          FPRMap[BlockNode].reads |= FPRBit(Op->Offset, IROp->Size);
        }

      }
    }
  }

  // Pass 2
  // Compute FPRs that are stored, but always ovewritten in the next blocks
  // Propagate the information a few times to eliminate more
  for (int i = 0; i < PropagationRounds; i++)
  {
    for (auto [BlockNode, BlockIROp] : CurrentIR.GetBlocks()) {
      for (auto [CodeNode, IROp] : CurrentIR.GetCode(BlockNode)) {

        if (IROp->Op == OP_JUMP) {
          auto Op = IROp->CW<IR::IROp_Jump>();
          OrderedNode *TargetNode = CurrentIR.GetNode(Op->Header.Args[0]);

          // stores to remove are written by the next block but not read
          FPRMap[BlockNode].kill = FPRMap[TargetNode].writes & ~(FPRMap[TargetNode].reads) & ~FPRMap[BlockNode].reads;

          // FPRs that are written by the next block can be considered as written by this block, if not read
          FPRMap[BlockNode].writes |= FPRMap[BlockNode].kill & ~FPRMap[BlockNode].reads;
        }
        else if (IROp->Op == OP_CONDJUMP) {
          auto Op = IROp->CW<IR::IROp_CondJump>();

          OrderedNode *TrueTargetNode = CurrentIR.GetNode(Op->TrueBlock);
          OrderedNode *FalseTargetNode = CurrentIR.GetNode(Op->FalseBlock);

          // stores to remove are written by the next blocks but not read
          FPRMap[BlockNode].kill = FPRMap[TrueTargetNode].writes & ~(FPRMap[TrueTargetNode].reads) & ~FPRMap[BlockNode].reads;
          FPRMap[BlockNode].kill &= FPRMap[FalseTargetNode].writes & ~(FPRMap[FalseTargetNode].reads) & ~FPRMap[BlockNode].reads;

          // FPRs that are written by the next blocks can be considered as written by this block, if not read
          FPRMap[BlockNode].writes |= FPRMap[BlockNode].kill & ~FPRMap[BlockNode].reads;
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
          if ((FPRMap[BlockNode].kill & FPRBit(Op->Offset, IROp->Size)) == FPRBit(Op->Offset, IROp->Size) && (FPRBit(Op->Offset, IROp->Size) != 0)) {
            IREmit->Remove(CodeNode);
            Changed = true;
          }
        }

      }
    }
  }

  return Changed;
}

FEXCore::IR::Pass* CreateDeadFPRStoreElimination() {
  return new DeadFPRStoreElimination{};
}

}