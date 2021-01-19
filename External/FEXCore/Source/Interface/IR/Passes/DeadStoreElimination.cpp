#include "Interface/IR/PassManager.h"
#include "Interface/Core/OpcodeDispatcher.h"

namespace FEXCore::IR {

constexpr int PropagationRounds = 5;

class DeadStoreElimination final : public FEXCore::IR::Pass {
public:
  bool Run(IREmitter *IREmit) override;
};

struct FlagInfo {
  uint64_t reads { 0 };
  uint64_t writes { 0 };
  uint64_t kill { 0 };
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
bool DeadStoreElimination::Run(IREmitter *IREmit) {
  std::unordered_map<OrderedNode*, Info> InfoMap;

  bool Changed = false;
  auto CurrentIR = IREmit->ViewIR();

  // Pass 1
  // Compute flags/gprs/fprs read/writes per block
  // This is conservative and doesn't try to be smart about loads after writes
  {
    for (auto [BlockNode, BlockIROp] : CurrentIR.GetBlocks()) {
      for (auto [CodeNode, IROp] : CurrentIR.GetCode(BlockNode)) {

        //// Flags ////
        if (IROp->Op == OP_STOREFLAG) {
          auto Op = IROp->CW<IR::IROp_StoreFlag>();
          InfoMap[BlockNode].flag.writes |= 1UL << Op->Flag;
        }
        else if  (IROp->Op == OP_INVALIDATEFLAGS) {
          auto Op = IROp->CW<IR::IROp_InvalidateFlags>();
          InfoMap[BlockNode].flag.writes |= Op->Flags;
        }
        else if (IROp->Op == OP_LOADFLAG) {
          auto Op = IROp->CW<IR::IROp_LoadFlag>();
          InfoMap[BlockNode].flag.reads |= 1UL << Op->Flag;
        }

        //// GPR ////
        if (IROp->Op == OP_STORECONTEXT) {
          auto Op = IROp->CW<IR::IROp_StoreContext>();
          if (IsFullGPR(Op->Offset, IROp->Size))
            InfoMap[BlockNode].gpr.writes |= GPRBit(Op->Offset);
          else
            InfoMap[BlockNode].gpr.reads |= GPRBit(Op->Offset);
        }
        else if (IROp->Op == OP_STORECONTEXTINDEXED ||
               IROp->Op == OP_LOADCONTEXTINDEXED) {
          // We can't track through these
          InfoMap[BlockNode].gpr.reads = -1;
        }
        else if (IROp->Op == OP_LOADCONTEXT) {
          auto Op = IROp->CW<IR::IROp_LoadContext>();
          InfoMap[BlockNode].gpr.reads |= GPRBit(Op->Offset);
        }

        //// FPR ////
        if (IROp->Op == OP_STORECONTEXT) {
          auto Op = IROp->CW<IR::IROp_StoreContext>();
          if (IsTrackedWriteFPR(Op->Offset, IROp->Size))
            InfoMap[BlockNode].fpr.writes |= FPRBit(Op->Offset, IROp->Size);
          else
            InfoMap[BlockNode].fpr.reads |= FPRBit(Op->Offset, IROp->Size);
        }
        else if (IROp->Op == OP_STORECONTEXTINDEXED ||
               IROp->Op == OP_LOADCONTEXTINDEXED) {
          // We can't track through these
          InfoMap[BlockNode].fpr.reads = -1;
        }
        else if (IROp->Op == OP_LOADCONTEXT) {
          auto Op = IROp->CW<IR::IROp_LoadContext>();
          InfoMap[BlockNode].fpr.reads |= FPRBit(Op->Offset, IROp->Size);
        }

      }
    }
  }

  // Pass 2
  // Compute flags/gprs/fprs that are stored, but always ovewritten in the next blocks
  // Propagate the information a few times to eliminate more
  for (int i = 0; i < PropagationRounds; i++)
  {
    for (auto [BlockNode, BlockIROp] : CurrentIR.GetBlocks()) {
      for (auto [CodeNode, IROp] : CurrentIR.GetCode(BlockNode)) {

        if (IROp->Op == OP_JUMP) {
          auto Op = IROp->CW<IR::IROp_Jump>();
          OrderedNode *TargetNode = CurrentIR.GetNode(Op->Header.Args[0]);

          //// Flags ////

          // stores to remove are written by the next block but not read
          InfoMap[BlockNode].flag.kill = InfoMap[TargetNode].flag.writes & ~(InfoMap[TargetNode].flag.reads) & ~InfoMap[BlockNode].flag.reads;

          // Flags that are written by the next block can be considered as written by this block, if not read
          InfoMap[BlockNode].flag.writes |= InfoMap[BlockNode].flag.kill & ~InfoMap[BlockNode].flag.reads;
          
          
          //// GPRs ////

          // stores to remove are written by the next block but not read
          InfoMap[BlockNode].gpr.kill = InfoMap[TargetNode].gpr.writes & ~(InfoMap[TargetNode].gpr.reads) & ~InfoMap[BlockNode].gpr.reads;

          // GPRs that are written by the next block can be considered as written by this block, if not read
          InfoMap[BlockNode].gpr.writes |= InfoMap[BlockNode].gpr.kill & ~InfoMap[BlockNode].gpr.reads;


          //// FPRs ////

          // stores to remove are written by the next block but not read
          InfoMap[BlockNode].fpr.kill = InfoMap[TargetNode].fpr.writes & ~(InfoMap[TargetNode].fpr.reads) & ~InfoMap[BlockNode].fpr.reads;

          // FPRs that are written by the next block can be considered as written by this block, if not read
          InfoMap[BlockNode].fpr.writes |= InfoMap[BlockNode].fpr.kill & ~InfoMap[BlockNode].fpr.reads;

        } else if (IROp->Op == OP_CONDJUMP) {
          auto Op = IROp->CW<IR::IROp_CondJump>();

          OrderedNode *TrueTargetNode = CurrentIR.GetNode(Op->TrueBlock);
          OrderedNode *FalseTargetNode = CurrentIR.GetNode(Op->FalseBlock);

          //// Flags ////

          // stores to remove are written by the next blocks but not read
          InfoMap[BlockNode].flag.kill = InfoMap[TrueTargetNode].flag.writes & ~(InfoMap[TrueTargetNode].flag.reads) & ~InfoMap[BlockNode].flag.reads;
          InfoMap[BlockNode].flag.kill &= InfoMap[FalseTargetNode].flag.writes & ~(InfoMap[FalseTargetNode].flag.reads) & ~InfoMap[BlockNode].flag.reads;

          // Flags that are written by the next blocks can be considered as written by this block, if not read
          InfoMap[BlockNode].flag.writes |= InfoMap[BlockNode].flag.kill & ~InfoMap[BlockNode].flag.reads;

          
          //// GPRs ////

          // stores to remove are written by the next blocks but not read
          InfoMap[BlockNode].gpr.kill = InfoMap[TrueTargetNode].gpr.writes & ~(InfoMap[TrueTargetNode].gpr.reads) & ~InfoMap[BlockNode].gpr.reads;
          InfoMap[BlockNode].gpr.kill &= InfoMap[FalseTargetNode].gpr.writes & ~(InfoMap[FalseTargetNode].gpr.reads) & ~InfoMap[BlockNode].gpr.reads;

          // GPRs that are written by the next blocks can be considered as written by this block, if not read
          InfoMap[BlockNode].gpr.writes |= InfoMap[BlockNode].gpr.kill & ~InfoMap[BlockNode].gpr.reads;


          //// FPRs ////

          // stores to remove are written by the next blocks but not read
          InfoMap[BlockNode].fpr.kill = InfoMap[TrueTargetNode].fpr.writes & ~(InfoMap[TrueTargetNode].fpr.reads) & ~InfoMap[BlockNode].fpr.reads;
          InfoMap[BlockNode].fpr.kill &= InfoMap[FalseTargetNode].fpr.writes & ~(InfoMap[FalseTargetNode].fpr.reads) & ~InfoMap[BlockNode].fpr.reads;

          // FPRs that are written by the next blocks can be considered as written by this block, if not read
          InfoMap[BlockNode].fpr.writes |= InfoMap[BlockNode].fpr.kill & ~InfoMap[BlockNode].fpr.reads;
        }
      }
    }
  }

  // Pass 3
  // Remove the dead stores
  {
    for (auto [BlockNode, BlockIROp] : CurrentIR.GetBlocks()) {
      for (auto [CodeNode, IROp] : CurrentIR.GetCode(BlockNode)) {

        //// Flags ////
        if (IROp->Op == OP_STOREFLAG) {
          auto Op = IROp->CW<IR::IROp_StoreFlag>();
          // If this StoreFlag is never read, remove it
          if (InfoMap[BlockNode].flag.kill & (1UL << Op->Flag)) {
            IREmit->Remove(CodeNode);
            Changed = true;
          }
        }

        if (IROp->Op == OP_STORECONTEXT) {
          auto Op = IROp->CW<IR::IROp_StoreContext>();
          
          //// GPRs ////
          // If this OP_STORECONTEXT is never read, remove it
          if (InfoMap[BlockNode].gpr.kill & GPRBit(Op->Offset)) {
            IREmit->Remove(CodeNode);
            Changed = true;
          }

          //// FPRs ////
          // If this OP_STORECONTEXT is never read, remove it
          if ((InfoMap[BlockNode].fpr.kill & FPRBit(Op->Offset, IROp->Size)) == FPRBit(Op->Offset, IROp->Size) && (FPRBit(Op->Offset, IROp->Size) != 0)) {
            IREmit->Remove(CodeNode);
            Changed = true;
          }
        }
      }
    }
  }

  return Changed;
}

FEXCore::IR::Pass* CreateDeadStoreElimination() {
  return new DeadStoreElimination{};
}

}
