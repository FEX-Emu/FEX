// SPDX-License-Identifier: MIT
/*
$info$
meta: ir|emitter ~ C++ Functions to generate IR. See IR.json for spec.
tags: ir|emitter
$end_info$
*/

#include "Interface/IR/IREmitter.h"

#include <FEXCore/IR/IR.h>
#include <FEXCore/Utils/EnumUtils.h>
#include <FEXCore/Utils/LogManager.h>

#include <array>
#include <stdint.h>
#include <string.h>

namespace FEXCore::IR {

bool IsFragmentExit(FEXCore::IR::IROps Op) {
  switch (Op) {
  case OP_EXITFUNCTION:
  case OP_BREAK: return true;
  default: return false;
  }
}

bool IsBlockExit(FEXCore::IR::IROps Op) {
  switch (Op) {
  case OP_JUMP:
  case OP_CONDJUMP: return true;
  default: return IsFragmentExit(Op);
  }
}

FEXCore::IR::RegisterClassType IREmitter::WalkFindRegClass(Ref Node) {
  auto Class = GetOpRegClass(Node);
  switch (Class) {
  case GPRClass:
  case FPRClass:
  case GPRFixedClass:
  case FPRFixedClass:
  case InvalidClass: return Class;
  default: break;
  }

  // Complex case, needs to be handled on an op by op basis
  uintptr_t DataBegin = DualListData.DataBegin();

  FEXCore::IR::IROp_Header* IROp = Node->Op(DataBegin);

  switch (IROp->Op) {
  case IROps::OP_LOADREGISTER: {
    auto Op = IROp->C<IROp_LoadRegister>();
    return Op->Class;
    break;
  }
  case IROps::OP_LOADCONTEXT: {
    auto Op = IROp->C<IROp_LoadContext>();
    return Op->Class;
    break;
  }
  case IROps::OP_LOADCONTEXTINDEXED: {
    auto Op = IROp->C<IROp_LoadContextIndexed>();
    return Op->Class;
    break;
  }
  case IROps::OP_FILLREGISTER: {
    auto Op = IROp->C<IROp_FillRegister>();
    return Op->Class;
    break;
  }
  case IROps::OP_LOADMEM: {
    auto Op = IROp->C<IROp_LoadMem>();
    return Op->Class;
    break;
  }
  case IROps::OP_LOADMEMTSO: {
    auto Op = IROp->C<IROp_LoadMemTSO>();
    return Op->Class;
    break;
  }
  default: LOGMAN_MSG_A_FMT("Unhandled op type: {} {} in argument class validation", ToUnderlying(IROp->Op), GetOpName(Node)); break;
  }
  return InvalidClass;
}

void IREmitter::ResetWorkingList() {
  DualListData.Reset();
  CodeBlocks.clear();
  CurrentWriteCursor = nullptr;
  // This is necessary since we do "null" pointer checks
  InvalidNode = reinterpret_cast<Ref>(DualListData.ListAllocate(sizeof(OrderedNode)));
  memset(InvalidNode, 0, sizeof(OrderedNode));
  CurrentCodeBlock = nullptr;
}

void IREmitter::ReplaceAllUsesWithRange(Ref Node, Ref NewNode, AllNodesIterator Begin, AllNodesIterator End) {
  uintptr_t ListBegin = DualListData.ListBegin();
  auto NodeId = Node->Wrapped(ListBegin).ID();

  while (Begin != End) {
    auto [RealNode, IROp] = Begin();

    const uint8_t NumArgs = IR::GetArgs(IROp->Op);
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

    ++Begin;
  }
}

void IREmitter::ReplaceNodeArgument(Ref Node, uint8_t Arg, Ref NewArg) {
  uintptr_t ListBegin = DualListData.ListBegin();
  uintptr_t DataBegin = DualListData.DataBegin();

  FEXCore::IR::IROp_Header* IROp = Node->Op(DataBegin);
  OrderedNodeWrapper OldArgWrapper = IROp->Args[Arg];
  Ref OldArg = OldArgWrapper.GetNode(ListBegin);
  OldArg->RemoveUse();
  NewArg->AddUse();
  IROp->Args[Arg].NodeOffset = NewArg->Wrapped(ListBegin).NodeOffset;
}

void IREmitter::RemoveArgUses(Ref Node) {
  uintptr_t ListBegin = DualListData.ListBegin();
  uintptr_t DataBegin = DualListData.DataBegin();

  FEXCore::IR::IROp_Header* IROp = Node->Op(DataBegin);

  const uint8_t NumArgs = IR::GetArgs(IROp->Op);
  for (uint8_t i = 0; i < NumArgs; ++i) {
    auto ArgNode = IROp->Args[i].GetNode(ListBegin);
    ArgNode->RemoveUse();
  }
}

void IREmitter::RemovePostRA(Ref Node) {
  Node->Unlink(DualListData.ListBegin());
}

void IREmitter::Remove(Ref Node) {
  RemoveArgUses(Node);

  Node->Unlink(DualListData.ListBegin());
}

IREmitter::IRPair<IROp_CodeBlock> IREmitter::CreateNewCodeBlockAfter(Ref insertAfter) {
  auto OldCursor = GetWriteCursor();

  auto CodeNode = CreateCodeNode();

  if (insertAfter) {
    LinkCodeBlocks(insertAfter, CodeNode);
  } else {
    LOGMAN_THROW_A_FMT(CurrentCodeBlock != nullptr, "CurrentCodeBlock must not be null here");

    // Find last block
    auto LastBlock = CurrentCodeBlock;

    while (LastBlock->Header.Next.GetNode(DualListData.ListBegin()) != InvalidNode) {
      LastBlock = LastBlock->Header.Next.GetNode(DualListData.ListBegin());
    }

    // Append it after the last block
    LinkCodeBlocks(LastBlock, CodeNode);
  }

  SetWriteCursor(OldCursor);

  return CodeNode;
}

void IREmitter::SetCurrentCodeBlock(Ref Node) {
  CurrentCodeBlock = Node;
  LOGMAN_THROW_A_FMT(Node->Op(DualListData.DataBegin())->Op == OP_CODEBLOCK, "Node wasn't codeblock. It was '{}'",
                     IR::GetName(Node->Op(DualListData.DataBegin())->Op));
  SetWriteCursor(Node->Op(DualListData.DataBegin())->CW<IROp_CodeBlock>()->Begin.GetNode(DualListData.ListBegin()));

  // Constants are pooled only within a single block.
  ConstantPool.clear();
}

} // namespace FEXCore::IR
