#include "Interface/IR/PassManager.h"
#include "Interface/Core/OpcodeDispatcher.h"
#include <FEXCore/Core/CoreState.h>

namespace FEXCore::IR {
class DCSE final : public FEXCore::IR::Pass {
public:
  bool Run(OpDispatchBuilder *Disp) override;
};

bool DCSE::Run(OpDispatchBuilder *Disp) {
  //printf("Doing DCSE run\n");
  return false;
}

FEXCore::IR::Pass* CreatePassDeadContextStoreElimination() {
  return new DCSE{};
}

class RCLE final : public FEXCore::IR::Pass {
public:
  bool Run(OpDispatchBuilder *Disp) override;
private:
  bool RedundantStoreLoadElimination(OpDispatchBuilder *Disp);
  bool RedundantLoadElimination(OpDispatchBuilder *Disp);
  bool RedundantStoreElimination(OpDispatchBuilder *Disp);
};

static bool IsAlignedGPR(uint8_t Size, uint32_t Offset, uint8_t *greg) {
  if (Size != 8) return false;
  if (Offset & 0b111) return false;
  if (Offset < offsetof(FEXCore::Core::CPUState, gregs[0]) || Offset > offsetof(FEXCore::Core::CPUState, gregs[15])) return false;

  *greg = (Offset - offsetof(FEXCore::Core::CPUState, gregs[0])) / 8;
  return true;
}

static bool IsGPR(uint32_t Offset, uint8_t *greg) {
  if (Offset < offsetof(FEXCore::Core::CPUState, gregs[0]) || Offset > offsetof(FEXCore::Core::CPUState, gregs[15])) return false;

  *greg = (Offset - offsetof(FEXCore::Core::CPUState, gregs[0])) / 8;
  return true;
}

static bool IsAlignedXMM(uint8_t Size, uint32_t Offset, uint8_t *greg) {
  if (Size != 16) return false;
  if (Offset & 0b1111) return false;
  if (Offset < offsetof(FEXCore::Core::CPUState, xmm[0]) || Offset > offsetof(FEXCore::Core::CPUState, xmm[15])) return false;

  *greg = (Offset - offsetof(FEXCore::Core::CPUState, xmm[0])) / 16;
  return true;
}

static bool IsXMM(uint32_t Offset, uint8_t *greg) {
  if (Offset < offsetof(FEXCore::Core::CPUState, xmm[0]) || Offset > offsetof(FEXCore::Core::CPUState, xmm[15])) return false;

  *greg = (Offset - offsetof(FEXCore::Core::CPUState, xmm[0])) / 16;
  return true;
}

/**
 * @brief This pass removes redundant pairs of storecontext and loadcontext ops
 *
 * This pass finds cases where a value is saved with StoreContext and then gets reloaded with LoadContext
 * This then eliminates the LoadContext op and uses the original IR value that was saved in the StoreContext instead
 * For XMM registers we need to be careful and do a VBitcast because the StoreContext may have stored a i32v4 and destination is expecting the i128
 * from the LoadContext.
 *
 * This removes a Store + Load dance that can occur
 *
 * eg.
 *   %ssa26 i128 = LoadMem %ssa25 i64, 0x10
 *   (%%ssa27) StoreContext %ssa26 i128, 0x10, 0xb0
 *   %ssa28 i128 = LoadContext 0x10, 0x90
 *   %ssa29 i128 = LoadContext 0x10, 0xb0
 * Converts to
 *   %ssa26 i128 = LoadMem %ssa25 i64, 0x10
 *   (%%ssa27) StoreContext %ssa26 i128, 0x10, 0xb0
 *   %ssa28 i128 = LoadContext 0x10, 0x90
 *   %ssa29 i128 = VBitcast %ssa26 i128
 */
bool RCLE::RedundantStoreLoadElimination(OpDispatchBuilder *Disp) {
  bool Changed = false;
  auto CurrentIR = Disp->ViewIR();
  std::array<OrderedNode *, 16> LastValidGPRStores{};
  std::array<OrderedNode *, 32> LastValidFLAGStores{};
  std::array<OrderedNode *, 16> LastValidXMMStores{};

  auto OriginalWriteCursor = Disp->GetWriteCursor();

  uintptr_t ListBegin = CurrentIR.GetListData();
  uintptr_t DataBegin = CurrentIR.GetData();

  auto Begin = CurrentIR.begin();
  auto Op = Begin();

  OrderedNode *RealNode = Op->GetNode(ListBegin);
  auto HeaderOp = RealNode->Op(DataBegin)->CW<FEXCore::IR::IROp_IRHeader>();
  LogMan::Throw::A(HeaderOp->Header.Op == OP_IRHEADER, "First op wasn't IRHeader");

  OrderedNode *BlockNode = HeaderOp->Blocks.GetNode(ListBegin);

  while (1) {
    auto BlockIROp = BlockNode->Op(DataBegin)->CW<FEXCore::IR::IROp_CodeBlock>();
    LogMan::Throw::A(BlockIROp->Header.Op == OP_CODEBLOCK, "IR type failed to be a code block");

    // We grab these nodes this way so we can iterate easily
    auto CodeBegin = CurrentIR.at(BlockIROp->Begin);
    auto CodeLast = CurrentIR.at(BlockIROp->Last);
    while (1) {
      auto CodeOp = CodeBegin();
      OrderedNode *CodeNode = CodeOp->GetNode(ListBegin);
      auto IROp = CodeNode->Op(DataBegin);

      if (IROp->Op == OP_STORECONTEXT) {
        auto Op = IROp->CW<IR::IROp_StoreContext>();
        // Make sure we are within GREG state
        uint8_t greg = ~0;
        if (IsAlignedGPR(Op->Size, Op->Offset, &greg)) {
          LastValidGPRStores[greg] = IROp->Args[0].GetNode(ListBegin);
        } else if (IsGPR(Op->Offset, &greg)) {
          // If we aren't overwriting the whole state then we don't want to track this value
          LastValidGPRStores[greg] = nullptr;
        }
        else if (IsAlignedXMM(Op->Size, Op->Offset, &greg)) {
          LastValidXMMStores[greg] = IROp->Args[0].GetNode(ListBegin);
        } else if (IsXMM(Op->Offset, &greg)) {
          // If we aren't overwriting the whole state then we don't want to track this value
          LastValidXMMStores[greg] = nullptr;
        }
      }
      else if (IROp->Op == OP_LOADCONTEXT) {
        auto Op = IROp->C<IR::IROp_LoadContext>();

        // Make sure we are within GREG state
        uint8_t greg = ~0;
        if (IsAlignedGPR(Op->Size, Op->Offset, &greg)) {
          if (LastValidGPRStores[greg] != nullptr) {
            // If the last store matches this load value then we can replace the loaded value with the previous valid one
            Disp->ReplaceAllUsesWithInclusive(CodeNode, LastValidGPRStores[greg], CodeBegin, CodeLast);
            if (CodeNode->GetUses() == 0)
              Disp->Remove(CodeNode);
            // Set it as invalid now
            LastValidGPRStores[greg] = nullptr;
            Changed = true;
          }
        } else if (IsGPR(Op->Offset, &greg)) {
            // If we aren't overwriting the whole state then we don't want to track this value
            LastValidGPRStores[greg] = nullptr;
        }
        else if (IsAlignedXMM(Op->Size, Op->Offset, &greg)) {
          if (LastValidXMMStores[greg] != nullptr) {
            // If the last store matches this load value then we can replace the loaded value with the previous valid one
            Disp->SetWriteCursor(CodeNode);
            auto BitCast = Disp->_VBitcast(LastValidXMMStores[greg]);
            Disp->ReplaceAllUsesWithInclusive(CodeNode, BitCast, CodeBegin, CodeLast);

            if (CodeNode->GetUses() == 0)
              Disp->Remove(CodeNode);
            // Set it as invalid now
            LastValidXMMStores[greg] = nullptr;
            Changed = true;
            // We blew away the load to point to the previous value still
          }
        } else if (IsXMM(Op->Offset, &greg)) {
            // If we aren't overwriting the whole state then we don't want to track this value
            LastValidXMMStores[greg] = nullptr;
        }
      }
      else if (IROp->Op == OP_STOREFLAG) {
        auto Op = IROp->C<IR::IROp_StoreFlag>();
        LastValidFLAGStores[Op->Flag] = IROp->Args[0].GetNode(ListBegin);
      }
      else if (IROp->Op == OP_LOADFLAG) {
        auto Op = IROp->C<IR::IROp_LoadFlag>();
        uint8_t Flag = Op->Flag;
        if (LastValidFLAGStores[Flag] != nullptr) {
          // If the last store matches this load value then we can replace the loaded value with the previous valid one
          Disp->SetWriteCursor(CodeNode);
          auto Res = Disp->_Bfe(1, 0, LastValidFLAGStores[Flag]);
          Disp->ReplaceAllUsesWithInclusive(CodeNode, Res, CodeBegin, CodeLast);
          if (CodeNode->GetUses() == 0)
            Disp->Remove(CodeNode);
          // Set it as invalid now
          LastValidFLAGStores[Flag] = nullptr;
          Changed = true;
        }
      }

      // CodeLast is inclusive. So we still need to dump the CodeLast op as well
      if (CodeBegin == CodeLast) {
        break;
      }
      ++CodeBegin;
    }

    if (BlockIROp->Next.ID() == 0) {
      break;
    } else {
      BlockNode = BlockIROp->Next.GetNode(ListBegin);
    }

    // We don't track across block boundaries
    LastValidGPRStores.fill(nullptr);
    LastValidXMMStores.fill(nullptr);
    LastValidFLAGStores.fill(nullptr);
  }

  Disp->SetWriteCursor(OriginalWriteCursor);

  return Changed;
}

/**
 * @brief This pass looks for redundant LoadContext ops and eliminates them
 *
 * This only currently looks for GPR and XMM registers that are aligned to their size
 *
 * eg.
 * 		%ssa6 i128 = LoadContext 0x10, 0x90
 *		%ssa7 i128 = LoadContext 0x10, 0x90
 *		%ssa8 i128 = VXor %ssa7 i128, %ssa6 i128
 * Converts to
 *    %ssa6 i128 = LoadContext 0x10, 0x90
 *    %ssa7 i128 = VXor %ssa6 i128, %ssa6 i128
 */
bool RCLE::RedundantLoadElimination(OpDispatchBuilder *Disp) {
  bool Changed = false;
  auto CurrentIR = Disp->ViewIR();

  auto OriginalWriteCursor = Disp->GetWriteCursor();

  uintptr_t ListBegin = CurrentIR.GetListData();
  uintptr_t DataBegin = CurrentIR.GetData();

  auto Begin = CurrentIR.begin();
  auto Op = Begin();

  OrderedNode *RealNode = Op->GetNode(ListBegin);
  auto HeaderOp = RealNode->Op(DataBegin)->CW<FEXCore::IR::IROp_IRHeader>();
  LogMan::Throw::A(HeaderOp->Header.Op == OP_IRHEADER, "First op wasn't IRHeader");

  OrderedNode *BlockNode = HeaderOp->Blocks.GetNode(ListBegin);

  std::array<OrderedNode *, 16> LastValidGPRLoads{};
  std::array<OrderedNode *, 16> LastValidXMMLoads{};

  while (1) {
    auto BlockIROp = BlockNode->Op(DataBegin)->CW<FEXCore::IR::IROp_CodeBlock>();
    LogMan::Throw::A(BlockIROp->Header.Op == OP_CODEBLOCK, "IR type failed to be a code block");

    // We grab these nodes this way so we can iterate easily
    auto CodeBegin = CurrentIR.at(BlockIROp->Begin);
    auto CodeLast = CurrentIR.at(BlockIROp->Last);
    while (1) {
      auto CodeOp = CodeBegin();
      OrderedNode *CodeNode = CodeOp->GetNode(ListBegin);
      auto IROp = CodeNode->Op(DataBegin);

      if (IROp->Op == OP_STORECONTEXT) {
        auto Op = IROp->CW<IR::IROp_StoreContext>();
        // Make sure we are within GREG state
        uint8_t greg = ~0;
        if (IsGPR(Op->Offset, &greg)) {
          LastValidGPRLoads[greg] = nullptr;
        } else if (IsXMM(Op->Offset, &greg)) {
          LastValidXMMLoads[greg] = nullptr;
        }
      }

      if (IROp->Op == OP_LOADCONTEXT) {
        auto Op = IROp->C<IR::IROp_LoadContext>();

        // Make sure we are within GREG state
        uint8_t greg = ~0;
        if (IsAlignedGPR(Op->Size, Op->Offset, &greg)) {
          if (LastValidGPRLoads[greg] != nullptr) {
            // If the last load is still valid at this point then replace all uses of the load with the previous one
            Disp->SetWriteCursor(CodeNode);
            Disp->ReplaceAllUsesWithInclusive(CodeNode, LastValidGPRLoads[greg], CodeBegin, CodeLast);
            if (CodeNode->GetUses() == 0)
              Disp->Remove(CodeNode);
            Changed = true;
          }
          else {
            LastValidGPRLoads[greg] = CodeNode;
          }
        } else if (IsGPR(Op->Offset, &greg)) {
          // If we aren't overwriting the whole state then we don't want to track this value
          LastValidGPRLoads[greg] = nullptr;
        }
        else if (IsAlignedXMM(Op->Size, Op->Offset, &greg)) {
          if (LastValidXMMLoads[greg] != nullptr) {
            // If the last load is still valid at this point then replace all uses of the load with the previous one
            Disp->SetWriteCursor(CodeNode);
            Disp->ReplaceAllUsesWithInclusive(CodeNode, LastValidXMMLoads[greg], CodeBegin, CodeLast);
            if (CodeNode->GetUses() == 0)
              Disp->Remove(CodeNode);
            Changed = true;
          }
          else {
            LastValidXMMLoads[greg] = CodeNode;
          }
        } else if (IsXMM(Op->Offset, &greg)) {
          // If we aren't overwriting the whole state then we don't want to track this value
          LastValidXMMLoads[greg] = nullptr;
        }
      }

      // CodeLast is inclusive. So we still need to dump the CodeLast op as well
      if (CodeBegin == CodeLast) {
        break;
      }
      ++CodeBegin;
    }

    if (BlockIROp->Next.ID() == 0) {
      break;
    } else {
      BlockNode = BlockIROp->Next.GetNode(ListBegin);
    }

    // We don't track across block boundaries
    LastValidGPRLoads.fill(nullptr);
    LastValidXMMLoads.fill(nullptr);
  }

  Disp->SetWriteCursor(OriginalWriteCursor);

  return Changed;
}

/**
 * @brief This pass looks for redundant StoreContext ops and eliminates them
 *
 * When multiple StoreContext ops happen to the same location without a load inbetween
 * Then the first StoreContext is redundant and we can eliminate it
 * This only currently looks for GPR and XMM registers that are aligned to their size
 *
 * eg.
 *   (%%ssa189) StoreContext %ssa188 i128, 0x10, 0xa0
 *   %ssa190 i128 = LoadContext 0x10, 0x90
 *   %ssa191 i128 = VBitcast %ssa188 i128
 *   %ssa192 i128 = VAdd %ssa191 i128, %ssa190 i128, 0x10, 0x4
 *   (%%ssa193) StoreContext %ssa192 i128, 0x10, 0xa0
 * Converts to
 *   %ssa173 i128 = LoadContext 0x10, 0x90
 *   %ssa174 i128 = VBitcast %ssa172 i128
 *   %ssa175 i128 = VAdd %ssa174 i128, %ssa173 i128, 0x10, 0x4
 *   (%%ssa176) StoreContext %ssa175 i128, 0x10, 0xa0
 */
bool RCLE::RedundantStoreElimination(OpDispatchBuilder *Disp) {
  bool Changed = false;
  auto CurrentIR = Disp->ViewIR();

  auto OriginalWriteCursor = Disp->GetWriteCursor();

  uintptr_t ListBegin = CurrentIR.GetListData();
  uintptr_t DataBegin = CurrentIR.GetData();

  auto Begin = CurrentIR.begin();
  auto Op = Begin();

  OrderedNode *RealNode = Op->GetNode(ListBegin);
  auto HeaderOp = RealNode->Op(DataBegin)->CW<FEXCore::IR::IROp_IRHeader>();
  LogMan::Throw::A(HeaderOp->Header.Op == OP_IRHEADER, "First op wasn't IRHeader");

  OrderedNode *BlockNode = HeaderOp->Blocks.GetNode(ListBegin);

  std::array<OrderedNode *, 16> LastValidGPRStores{};
  std::array<OrderedNode *, 16> LastValidXMMStores{};

  while (1) {
    auto BlockIROp = BlockNode->Op(DataBegin)->CW<FEXCore::IR::IROp_CodeBlock>();
    LogMan::Throw::A(BlockIROp->Header.Op == OP_CODEBLOCK, "IR type failed to be a code block");

    // We grab these nodes this way so we can iterate easily
    auto CodeBegin = CurrentIR.at(BlockIROp->Begin);
    auto CodeLast = CurrentIR.at(BlockIROp->Last);
    while (1) {
      auto CodeOp = CodeBegin();
      OrderedNode *CodeNode = CodeOp->GetNode(ListBegin);
      auto IROp = CodeNode->Op(DataBegin);

      if (IROp->Op == OP_STORECONTEXT) {
        auto Op = IROp->CW<IR::IROp_StoreContext>();
        // Make sure we are within GREG state
        uint8_t greg = ~0;
        if (IsAlignedGPR(Op->Size, Op->Offset, &greg)) {
          if (LastValidGPRStores[greg] != nullptr) {
            // If there was previously a store to this context location
            // and there hasn't been a load inbetween, then we can blow away the previous store
            Disp->Remove(LastValidGPRStores[greg]);
          }
          LastValidGPRStores[greg] = CodeNode;
        }
        else if (IsGPR(Op->Offset, &greg)) {
          LastValidGPRStores[greg] = nullptr;
        }
        else if (IsAlignedXMM(Op->Size, Op->Offset, &greg)) {
          if (LastValidXMMStores[greg] != nullptr) {
            // If there was previously a store to this context location
            // and there hasn't been a load inbetween, then we can blow away the previous store
            Disp->Remove(LastValidXMMStores[greg]);
          }
          // This op is now the new valid one
          LastValidXMMStores[greg] = CodeNode;
        } else if (IsXMM(Op->Offset, &greg)) {
          LastValidXMMStores[greg] = nullptr;
        }
      }

      if (IROp->Op == OP_LOADCONTEXT) {
        auto Op = IROp->C<IR::IROp_LoadContext>();

        // Make sure we are within GREG state
        uint8_t greg = ~0;
        if (IsGPR(Op->Offset, &greg)) {
          LastValidGPRStores[greg] = nullptr;
        } else if (IsXMM(Op->Offset, &greg)) {
          LastValidXMMStores[greg] = nullptr;
        }
      }

      // CodeLast is inclusive. So we still need to dump the CodeLast op as well
      if (CodeBegin == CodeLast) {
        break;
      }
      ++CodeBegin;
    }

    if (BlockIROp->Next.ID() == 0) {
      break;
    } else {
      BlockNode = BlockIROp->Next.GetNode(ListBegin);
    }

    // We don't track across block boundaries
    LastValidGPRStores.fill(nullptr);
    LastValidXMMStores.fill(nullptr);
  }

  Disp->SetWriteCursor(OriginalWriteCursor);

  return Changed;
}

bool RCLE::Run(OpDispatchBuilder *Disp) {
  bool Changed = false;
  Changed |= RedundantStoreLoadElimination(Disp);
  Changed |= RedundantLoadElimination(Disp);
  Changed |= RedundantStoreElimination(Disp);
  return Changed;
}

FEXCore::IR::Pass* CreateRedundantContextLoadElimination() {
  return new RCLE{};
}

}


