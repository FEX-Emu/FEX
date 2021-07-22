/*
$info$
meta: ir|emitter ~ C++ Functions to generate IR. See IR.json for spec.
tags: ir|emitter
$end_info$
*/

#include <FEXCore/IR/IREmitter.h>

namespace FEXCore::IR {
void IREmitter::ResetWorkingList() {
  DualListData.Reset();
  CodeBlocks.clear();
  CurrentWriteCursor = nullptr;
  // This is necessary since we do "null" pointer checks
  InvalidNode = reinterpret_cast<OrderedNode*>(DualListData.ListAllocate(sizeof(OrderedNode)));
  memset(InvalidNode, 0, sizeof(OrderedNode));
  CurrentCodeBlock = nullptr;
}

void IREmitter::ReplaceAllUsesWithRange(OrderedNode *Node, OrderedNode *NewNode, AllNodesIterator After, AllNodesIterator End) {
  uintptr_t ListBegin = DualListData.ListBegin();
  auto NodeId = Node->Wrapped(ListBegin).ID();

  while (After != End) {
    auto [RealNode, IROp] = After();

    uint8_t NumArgs = IR::GetArgs(IROp->Op);
    for (uint8_t i = 0; i < NumArgs; ++i) {
      if (IROp->Args[i].ID() == NodeId) {
        Node->RemoveUse();
        NewNode->AddUse();
        IROp->Args[i].NodeOffset = NewNode->Wrapped(ListBegin).NodeOffset;

        // We can stop searching once all uses of the node are gone.
        if (Node->NumUses == 0) {
          return;
        }
      }
    }

    ++After;
  }
}

void IREmitter::ReplaceNodeArgument(OrderedNode *Node, uint8_t Arg, OrderedNode *NewArg) {
  uintptr_t ListBegin = DualListData.ListBegin();
  uintptr_t DataBegin = DualListData.DataBegin();

  FEXCore::IR::IROp_Header *IROp = Node->Op(DataBegin);
  OrderedNodeWrapper OldArgWrapper = IROp->Args[Arg];
  OrderedNode *OldArg = OldArgWrapper.GetNode(ListBegin);
  OldArg->RemoveUse();
  NewArg->AddUse();
  IROp->Args[Arg].NodeOffset = NewArg->Wrapped(ListBegin).NodeOffset;
}

void IREmitter::RemoveArgUses(OrderedNode *Node) {
  uintptr_t ListBegin = DualListData.ListBegin();
  uintptr_t DataBegin = DualListData.DataBegin();

  FEXCore::IR::IROp_Header *IROp = Node->Op(DataBegin);

  uint8_t NumArgs = IR::GetArgs(IROp->Op);
  for (uint8_t i = 0; i < NumArgs; ++i) {
    auto ArgNode = IROp->Args[i].GetNode(ListBegin);
    ArgNode->RemoveUse();
  }
}

void IREmitter::Remove(OrderedNode *Node) {
  RemoveArgUses(Node);

  Node->Unlink(DualListData.ListBegin());
}

IREmitter::IRPair<IROp_CodeBlock> IREmitter::CreateNewCodeBlockAfter(OrderedNode* insertAfter) {
  auto OldCursor = GetWriteCursor();

  auto CodeNode = CreateCodeNode();

  if (insertAfter) {
    LinkCodeBlocks(insertAfter, CodeNode);
  } else {
    LOGMAN_THROW_A_FMT(CurrentCodeBlock != nullptr, "CurrentCodeBlock must not be null here");

    // Find last block
    auto LastBlock = CurrentCodeBlock;

    while (LastBlock->Header.Next.GetNode(DualListData.ListBegin()) != InvalidNode)
      LastBlock = LastBlock->Header.Next.GetNode(DualListData.ListBegin());

    // Append it after the last block
    LinkCodeBlocks(LastBlock, CodeNode);
  }

  SetWriteCursor(OldCursor);

  return CodeNode;
}

void IREmitter::ReplaceWithConstant(OrderedNode *Node, uint64_t Value) {
    auto Header = Node->Op(DualListData.DataBegin());

    if (IRSizes[Header->Op] >= sizeof(IROp_Constant)) {
      // Unlink any arguments the node currently has
      RemoveArgUses(Node);

      // Overwrite data with the new constant op
      Header->Op = OP_CONSTANT;
      Header->NumArgs = 0;
      auto Const = Header->CW<IROp_Constant>();
      Const->Constant = Value;
    } else {
      // Fallback path for when the node to overwrite is too small
      auto cursor = GetWriteCursor();
      SetWriteCursor(Node);

      auto NewNode = _Constant(Value);
      ReplaceAllUsesWith(Node, NewNode);

      SetWriteCursor(cursor);
    }
  }

}

