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
  uintptr_t ListBegin = CurrentIR.GetListData();
  uintptr_t DataBegin = CurrentIR.GetData();

  IR::NodeWrapperIterator Begin = CurrentIR.begin();
  IR::NodeWrapperIterator End = CurrentIR.end();

  std::array<NodeWrapper*, 16> LastValidGPRStores{};

  while (Begin != End) {
    NodeWrapper *WrapperOp = Begin();
    OrderedNode *RealNode = reinterpret_cast<OrderedNode*>(WrapperOp->GetPtr(ListBegin));
    FEXCore::IR::IROp_Header *IROp = RealNode->Op(DataBegin);

    if (IROp->Op == OP_BEGINBLOCK ||
        IROp->Op == OP_ENDBLOCK ||
        IROp->Op == OP_JUMP ||
        IROp->Op == OP_CONDJUMP ||
        IROp->Op == OP_EXITFUNCTION) {
      // We don't track across block boundaries
      LastValidGPRStores.fill(nullptr);
    }

    if (IROp->Op == OP_STORECONTEXT) {
      auto Op = IROp->CW<IR::IROp_StoreContext>();
      // Make sure we are within GREG state
      uint8_t greg = ~0;
      if (IsAlignedGPR(Op->Size, Op->Offset, &greg)) {
        FEXCore::IR::IROp_Header *ArgOp = reinterpret_cast<OrderedNode*>(Op->Header.Args[0].GetPtr(ListBegin))->Op(DataBegin);
        // Ensure we aren't doing a mismatched store
        // XXX: We should really catch this in IR validation
        if (ArgOp->Size == 8) {
          LastValidGPRStores[greg] = &Op->Header.Args[0];
        }
        else {
          LastValidGPRStores[greg] = nullptr;
        }
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
          auto MovVal = Disp->_Mov(reinterpret_cast<OrderedNode*>(LastValidGPRStores[greg]->GetPtr(ListBegin)));
          Disp->ReplaceAllUsesWith(RealNode, MovVal);
          Changed = true;
        }
      } else if (IsGPR(Op->Offset, &greg)) {
          // If we aren't overwriting the whole state then we don't want to track this value
          LastValidGPRStores[greg] = nullptr; // 0 is invalid
      }
    }
    ++Begin;
  }

  return Changed;
}

FEXCore::IR::Pass* CreateRedundantContextLoadElimination() {
  return new RCLE{};
}

}


