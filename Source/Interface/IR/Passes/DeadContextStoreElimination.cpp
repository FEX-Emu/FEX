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

bool RCLE::Run(OpDispatchBuilder *Disp) {
  bool Changed = false;
  auto CurrentIR = Disp->ViewIR();
  std::array<OrderedNode *, 16> LastValidGPRStores{};
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
      }

      if (IROp->Op == OP_LOADCONTEXT) {
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
  }

  Disp->SetWriteCursor(OriginalWriteCursor);

  return Changed;
}

FEXCore::IR::Pass* CreateRedundantContextLoadElimination() {
  return new RCLE{};
}

}


