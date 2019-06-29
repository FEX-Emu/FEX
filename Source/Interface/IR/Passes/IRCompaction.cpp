#include "Interface/IR/PassManager.h"
#include "Interface/Core/OpcodeDispatcher.h"

#include <map>

namespace FEXCore::IR {

class IRCompaction final : public FEXCore::IR::Pass {
public:
  IRCompaction();
  bool Run(OpDispatchBuilder *Disp) override;

private:
  OpDispatchBuilder LocalBuilder;
  std::vector<IR::NodeWrapper::NodeOffsetType> NodeLocationRemapper;
};

IRCompaction::IRCompaction() {
  NodeLocationRemapper.resize(9000);
}

bool IRCompaction::Run(OpDispatchBuilder *Disp) {
  auto CurrentIR = Disp->ViewIR();
  auto LocalIR = LocalBuilder.ViewIR();
  uint32_t NodeCount = LocalIR.GetListSize() / sizeof(OrderedNode);

  // Reset our local working list
  LocalBuilder.ResetWorkingList();
  if (NodeLocationRemapper.size() < NodeCount) {
    NodeLocationRemapper.resize(NodeCount);
  }
  memset(&NodeLocationRemapper.at(0), 0xFF, NodeCount * sizeof(IR::NodeWrapper::NodeOffsetType));

  uintptr_t LocalListBegin = LocalIR.GetListData();
  uintptr_t LocalDataBegin = LocalIR.GetData();

  uintptr_t ListBegin = CurrentIR.GetListData();
  uintptr_t DataBegin = CurrentIR.GetData();

  IR::NodeWrapperIterator Begin = CurrentIR.begin();
  IR::NodeWrapperIterator End = CurrentIR.end();

  // This compaction pass is something that we need to ensure correct ordering and distances between IROps\
  // Later on we assume that an IROp's SSA value live range is its Node locations
  //
  // RA distance calculation is calculated purely on the Node locations
  // So we just need to reorder those
  //
  // Additionally there may be some dead ops hanging out in the IR list that are orphaned.
  // These can also be dropped during this pass

  while (Begin != End) {
    NodeWrapper *WrapperOp = Begin();
    OrderedNode *RealNode = reinterpret_cast<OrderedNode*>(WrapperOp->GetPtr(ListBegin));
    FEXCore::IR::IROp_Header *IROp = RealNode->Op(DataBegin);
    size_t OpSize = FEXCore::IR::GetSize(IROp->Op);

    // Allocate the ops locally for our local dispatch
    auto LocalPair = LocalBuilder.AllocateRawOp(OpSize);
    IR::NodeWrapper LocalNodeWrapper = LocalPair.Node->Wrapped(LocalListBegin);

    // Copy over the op
    memcpy(LocalPair.first, IROp, OpSize);

    // Set our map remapper to map the new location
    // Even nodes that don't have a destination need to be in this map
    // Need to be able to remap branch targets any other bits
    NodeLocationRemapper[WrapperOp->ID()] = LocalNodeWrapper.ID();
    ++Begin;
  }

  Begin = CurrentIR.begin();
  while (Begin != End) {
    NodeWrapper *WrapperOp = Begin();
    OrderedNode *RealNode = reinterpret_cast<OrderedNode*>(WrapperOp->GetPtr(ListBegin));
    FEXCore::IR::IROp_Header *IROp = RealNode->Op(DataBegin);

    NodeWrapper LocalNodeWrapper = NodeWrapper::WrapOffset(NodeLocationRemapper[WrapperOp->ID()] * sizeof(OrderedNode));
    OrderedNode *LocalNode = reinterpret_cast<OrderedNode*>(LocalNodeWrapper.GetPtr(LocalListBegin));
    FEXCore::IR::IROp_Header *LocalIROp = LocalNode->Op(LocalDataBegin);

    // Now that we have the op copied over, we need to modify SSA values to point to the new correct locations
    for (uint8_t i = 0; i < IROp->NumArgs; ++i) {
      NodeWrapper OldArg = IROp->Args[i];
      LogMan::Throw::A(NodeLocationRemapper[OldArg.ID()] != ~0U, "Tried remapping unfound node");
      LocalIROp->Args[i].NodeOffset = NodeLocationRemapper[OldArg.ID()] * sizeof(OrderedNode);
    }
    ++Begin;
  }

//  uintptr_t OldListSize = CurrentIR.GetListSize();
//  uintptr_t OldDataSize = CurrentIR.GetDataSize();
//
//  uintptr_t NewListSize = LocalIR.GetListSize();
//  uintptr_t NewDataSize = LocalIR.GetDataSize();
//
//  if (NewListSize < OldListSize ||
//      NewDataSize < OldDataSize) {
//    if (NewListSize < OldListSize) {
//      LogMan::Msg::D("Shaved %ld bytes off the list size", OldListSize - NewListSize);
//    }
//    if (NewDataSize < OldDataSize) {
//      LogMan::Msg::D("Shaved %ld bytes off the data size", OldDataSize - NewDataSize);
//    }
//  }

//  if (NewListSize > OldListSize ||
//      NewDataSize > OldDataSize) {
//    LogMan::Msg::A("Whoa. Compaction made the IR a different size when it shouldn't have. 0x%lx > 0x%lx or 0x%lx > 0x%lx",NewListSize, OldListSize, NewDataSize, OldDataSize);
//  }

  Disp->CopyData(LocalBuilder);

  return true;
}

FEXCore::IR::Pass* CreateIRCompaction() {
  return new IRCompaction{};
}

}
