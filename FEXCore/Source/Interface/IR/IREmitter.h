// SPDX-License-Identifier: MIT
#pragma once

#include "Interface/IR/IR.h"
#include "Interface/IR/IntrusiveIRList.h"

#include <FEXCore/Core/CoreState.h>
#include <FEXCore/IR/IR.h>

#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/fextl/vector.h>

#include <algorithm>
#include <new>
#include <stdint.h>
#include <string.h>

namespace FEXCore::IR {
class Pass;
class PassManager;

class IREmitter {
  friend class FEXCore::IR::Pass;
  friend class FEXCore::IR::PassManager;

public:
  IREmitter(FEXCore::Utils::IntrusivePooledAllocator& ThreadAllocator)
    : DualListData {ThreadAllocator, 8 * 1024 * 1024} {
    ReownOrClaimBuffer();
    ResetWorkingList();
  }

  virtual ~IREmitter() = default;

  void ReownOrClaimBuffer() {
    DualListData.ReownOrClaimBuffer();
  }

  void DelayedDisownBuffer() {
    DualListData.DelayedDisownBuffer();
  }

  IRListView ViewIR() {
    return IRListView(&DualListData);
  }
  void ResetWorkingList();

  /**
   * @name IR allocation routines
   *
   * @{ */

  FEXCore::IR::RegisterClassType WalkFindRegClass(Ref Node);

// These handlers add cost to the constructor and destructor
// If it becomes an issue then blow them away
// GCC also generates some pretty atrocious code around these
// Use Clang!
#define IROP_ALLOCATE_HELPERS
#define IROP_DISPATCH_HELPERS
#include <FEXCore/IR/IRDefines.inc>
  IRPair<IROp_Constant> _Constant(uint8_t Size, uint64_t Constant) {
    auto Op = AllocateOp<IROp_Constant, IROps::OP_CONSTANT>();
    uint64_t Mask = ~0ULL >> (64 - Size);
    Op.first->Constant = (Constant & Mask);
    Op.first->Header.Size = Size / 8;
    Op.first->Header.ElementSize = Size / 8;
    return Op;
  }
  IRPair<IROp_Jump> _Jump() {
    return _Jump(InvalidNode);
  }
  IRPair<IROp_CondJump> _CondJump(Ref ssa0, CondClassType cond = {COND_NEQ}) {
    return _CondJump(ssa0, _Constant(0), InvalidNode, InvalidNode, cond, GetOpSize(ssa0));
  }
  IRPair<IROp_CondJump> _CondJump(Ref ssa0, Ref ssa1, Ref ssa2, CondClassType cond = {COND_NEQ}) {
    return _CondJump(ssa0, _Constant(0), ssa1, ssa2, cond, GetOpSize(ssa0));
  }
  // TODO: Work to remove this implicit sized Select implementation.
  IRPair<IROp_Select> _Select(uint8_t Cond, Ref ssa0, Ref ssa1, Ref ssa2, Ref ssa3, uint8_t CompareSize = 0) {
    if (CompareSize == 0) {
      CompareSize = std::max<uint8_t>(4, std::max<uint8_t>(GetOpSize(ssa0), GetOpSize(ssa1)));
    }

    return _Select(IR::SizeToOpSize(std::max<uint8_t>(4, std::max<uint8_t>(GetOpSize(ssa2), GetOpSize(ssa3)))),
                   IR::SizeToOpSize(CompareSize), CondClassType {Cond}, ssa0, ssa1, ssa2, ssa3);
  }
  IRPair<IROp_LoadMem> _LoadMem(FEXCore::IR::RegisterClassType Class, uint8_t Size, Ref ssa0, uint8_t Align = 1) {
    return _LoadMem(Class, Size, ssa0, Invalid(), Align, MEM_OFFSET_SXTX, 1);
  }
  IRPair<IROp_LoadMemTSO> _LoadMemTSO(FEXCore::IR::RegisterClassType Class, uint8_t Size, Ref ssa0, uint8_t Align = 1) {
    return _LoadMemTSO(Class, Size, ssa0, Invalid(), Align, MEM_OFFSET_SXTX, 1);
  }
  IRPair<IROp_StoreMem> _StoreMem(FEXCore::IR::RegisterClassType Class, uint8_t Size, Ref Addr, Ref Value, uint8_t Align = 1) {
    return _StoreMem(Class, Size, Value, Addr, Invalid(), Align, MEM_OFFSET_SXTX, 1);
  }
  IRPair<IROp_StoreMemTSO> _StoreMemTSO(FEXCore::IR::RegisterClassType Class, uint8_t Size, Ref Addr, Ref Value, uint8_t Align = 1) {
    return _StoreMemTSO(Class, Size, Value, Addr, Invalid(), Align, MEM_OFFSET_SXTX, 1);
  }
  Ref Invalid() {
    return InvalidNode;
  }

  void SetJumpTarget(IR::IROp_Jump* Op, Ref Target) {
    LOGMAN_THROW_A_FMT(Target->Op(DualListData.DataBegin())->Op == OP_CODEBLOCK, "Tried setting Jump target to %{} {}",
                       Target->Wrapped(DualListData.ListBegin()).ID(), IR::GetName(Target->Op(DualListData.DataBegin())->Op));

    Op->Header.Args[0].NodeOffset = Target->Wrapped(DualListData.ListBegin()).NodeOffset;
  }
  void SetTrueJumpTarget(IR::IROp_CondJump* Op, Ref Target) {
    LOGMAN_THROW_A_FMT(Target->Op(DualListData.DataBegin())->Op == OP_CODEBLOCK, "Tried setting CondJump target to %{} {}",
                       Target->Wrapped(DualListData.ListBegin()).ID(), IR::GetName(Target->Op(DualListData.DataBegin())->Op));

    Op->TrueBlock.NodeOffset = Target->Wrapped(DualListData.ListBegin()).NodeOffset;
  }
  void SetFalseJumpTarget(IR::IROp_CondJump* Op, Ref Target) {
    LOGMAN_THROW_A_FMT(Target->Op(DualListData.DataBegin())->Op == OP_CODEBLOCK, "Tried setting CondJump target to %{} {}",
                       Target->Wrapped(DualListData.ListBegin()).ID(), IR::GetName(Target->Op(DualListData.DataBegin())->Op));

    Op->FalseBlock.NodeOffset = Target->Wrapped(DualListData.ListBegin()).NodeOffset;
  }

  void SetJumpTarget(IRPair<IROp_Jump> Op, Ref Target) {
    LOGMAN_THROW_A_FMT(Target->Op(DualListData.DataBegin())->Op == OP_CODEBLOCK, "Tried setting Jump target to %{} {}",
                       Target->Wrapped(DualListData.ListBegin()).ID(), IR::GetName(Target->Op(DualListData.DataBegin())->Op));

    Op.first->Header.Args[0].NodeOffset = Target->Wrapped(DualListData.ListBegin()).NodeOffset;
  }
  void SetTrueJumpTarget(IRPair<IROp_CondJump> Op, Ref Target) {
    LOGMAN_THROW_A_FMT(Target->Op(DualListData.DataBegin())->Op == OP_CODEBLOCK, "Tried setting CondJump target to %{} {}",
                       Target->Wrapped(DualListData.ListBegin()).ID(), IR::GetName(Target->Op(DualListData.DataBegin())->Op));
    Op.first->TrueBlock.NodeOffset = Target->Wrapped(DualListData.ListBegin()).NodeOffset;
  }
  void SetFalseJumpTarget(IRPair<IROp_CondJump> Op, Ref Target) {
    LOGMAN_THROW_A_FMT(Target->Op(DualListData.DataBegin())->Op == OP_CODEBLOCK, "Tried setting CondJump target to %{} {}",
                       Target->Wrapped(DualListData.ListBegin()).ID(), IR::GetName(Target->Op(DualListData.DataBegin())->Op));
    Op.first->FalseBlock.NodeOffset = Target->Wrapped(DualListData.ListBegin()).NodeOffset;
  }

  /**  @} */
  FEXCore::IR::RegisterClassType WalkFindRegClass(OrderedNodeWrapper ssa) {
    Ref RealNode = ssa.GetNode(DualListData.ListBegin());
    return WalkFindRegClass(RealNode);
  }

  bool IsValueConstant(OrderedNodeWrapper ssa, uint64_t* Constant = nullptr) {
    Ref RealNode = ssa.GetNode(DualListData.ListBegin());
    FEXCore::IR::IROp_Header* IROp = RealNode->Op(DualListData.DataBegin());
    if (IROp->Op == OP_CONSTANT) {
      auto Op = IROp->C<IR::IROp_Constant>();
      if (Constant) {
        *Constant = Op->Constant;
      }
      return true;
    }
    return false;
  }

  bool IsValueInlineConstant(OrderedNodeWrapper ssa) {
    Ref RealNode = ssa.GetNode(DualListData.ListBegin());
    FEXCore::IR::IROp_Header* IROp = RealNode->Op(DualListData.DataBegin());
    if (IROp->Op == OP_INLINECONSTANT) {
      return true;
    }
    return false;
  }

  FEXCore::IR::IROp_Header* GetOpHeader(OrderedNodeWrapper ssa) {
    Ref RealNode = ssa.GetNode(DualListData.ListBegin());
    return RealNode->Op(DualListData.DataBegin());
  }

  Ref UnwrapNode(OrderedNodeWrapper ssa) {
    return ssa.GetNode(DualListData.ListBegin());
  }

  OrderedNodeWrapper WrapNode(Ref node) {
    return node->Wrapped(DualListData.ListBegin());
  }

  NodeIterator GetIterator(OrderedNodeWrapper wrapper) {
    return NodeIterator(DualListData.ListBegin(), DualListData.DataBegin(), wrapper);
  }

  // Overwrite a node with a constant
  // Depending on what node has been overwritten, there might be some unallocated space around the node
  // Because we are overwriting the node, we don't have to worry about update all the arguments which use it
  void ReplaceWithConstant(Ref Node, uint64_t Value);

  void ReplaceAllUsesWithRange(Ref Node, Ref NewNode, AllNodesIterator Begin, AllNodesIterator End);

  void ReplaceUsesWithAfter(Ref Node, Ref NewNode, AllNodesIterator After) {
    ++After;
    ReplaceAllUsesWithRange(Node, NewNode, After, AllNodesIterator(DualListData.ListBegin(), DualListData.DataBegin()));
  }

  void ReplaceUsesWithAfter(Ref Node, Ref NewNode, Ref After) {
    auto Wrapped = After->Wrapped(DualListData.ListBegin());
    AllNodesIterator It = AllNodesIterator(DualListData.ListBegin(), DualListData.DataBegin(), Wrapped);

    ReplaceUsesWithAfter(Node, NewNode, It);
  }

  void ReplaceAllUsesWith(Ref Node, Ref NewNode) {
    auto Start = AllNodesIterator(DualListData.ListBegin(), DualListData.DataBegin(), Node->Wrapped(DualListData.ListBegin()));

    ReplaceAllUsesWithRange(Node, NewNode, Start, AllNodesIterator(DualListData.ListBegin(), DualListData.DataBegin()));

    LOGMAN_THROW_AA_FMT(Node->NumUses == 0, "Node still used");

    auto IROp = Node->Op(DualListData.DataBegin())->CW<FEXCore::IR::IROp_Header>();
    // We can not remove the op if there are side-effects
    if (!IR::HasSideEffects(IROp->Op)) {
      // Since we have deleted ALL uses, we can safely delete the node.
      Remove(Node);
    }
  }

  void ReplaceNodeArgument(Ref Node, uint8_t Arg, Ref NewArg);

  void Remove(Ref Node);

  void SetPackedRFLAG(bool Lower8, Ref Src);
  Ref GetPackedRFLAG(bool Lower8);

  void CopyData(const IREmitter& rhs) {
    LOGMAN_THROW_A_FMT(rhs.DualListData.DataBackingSize() <= DualListData.DataBackingSize(), "Trying to take ownership of data that is too "
                                                                                             "large");
    LOGMAN_THROW_A_FMT(rhs.DualListData.ListBackingSize() <= DualListData.ListBackingSize(), "Trying to take ownership of data that is too "
                                                                                             "large");
    DualListData.CopyData(rhs.DualListData);
    InvalidNode = rhs.InvalidNode->Wrapped(rhs.DualListData.ListBegin()).GetNode(DualListData.ListBegin());
    CurrentWriteCursor = rhs.CurrentWriteCursor;
    CodeBlocks = rhs.CodeBlocks;
    for (auto& CodeBlock : CodeBlocks) {
      CodeBlock = CodeBlock->Wrapped(rhs.DualListData.ListBegin()).GetNode(DualListData.ListBegin());
    }
  }

  void SetWriteCursor(Ref Node) {
    CurrentWriteCursor = Node;
  }

  // Set cursor to write before Node
  void SetWriteCursorBefore(Ref Node) {
    auto IR = ViewIR();
    auto Before = IR.at(Node);
    --Before;

    SetWriteCursor(std::get<0>(*Before));
  }

  Ref GetWriteCursor() {
    return CurrentWriteCursor;
  }

  Ref GetCurrentBlock() {
    return CurrentCodeBlock;
  }

  /**
   * @brief This creates an orphaned code node
   * The IROp backing is in the correct list but the OrderedNode lives outside of the list
   *
   * XXX: This is because we don't want code blocks to interleave with current instruction IR ops currently
   * We can change this behaviour once we remove the old BeginBlock/EndBlock types
   *
   * @return OrderedNode
   */
  IRPair<IROp_CodeBlock> CreateCodeNode() {
    SetWriteCursor(nullptr); // Orphan from any previous nodes

    auto CodeNode = _CodeBlock(InvalidNode, InvalidNode);

    CodeBlocks.emplace_back(CodeNode);

    SetWriteCursor(nullptr); // Orphan from any future nodes

    auto Begin = _BeginBlock(CodeNode);
    CodeNode.first->Begin = Begin.Node->Wrapped(DualListData.ListBegin());

    auto EndBlock = _EndBlock(CodeNode);
    CodeNode.first->Last = EndBlock.Node->Wrapped(DualListData.ListBegin());

    return CodeNode;
  }

  /**
   * @name Links codeblocks together
   * Codeblocks are singly linked so we need to walk the list forward if the linked block isn't isn't the last
   *
   * eq.
   * CodeNode->Next -> Next
   * to
   * CodeNode->Next -> New -> Next
   *
   * @{ */
  /**  @} */
  void LinkCodeBlocks(Ref CodeNode, Ref Next) {
#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
    FEXCore::IR::IROp_CodeBlock* CurrentIROp =
#endif
      CodeNode->Op(DualListData.DataBegin())->CW<FEXCore::IR::IROp_CodeBlock>();

    LOGMAN_THROW_A_FMT(CurrentIROp->Header.Op == IROps::OP_CODEBLOCK, "Invalid");

    CodeNode->append(DualListData.ListBegin(), Next);
  }

  IRPair<IROp_CodeBlock> CreateNewCodeBlockAtEnd() {
    return CreateNewCodeBlockAfter(nullptr);
  }
  IRPair<IROp_CodeBlock> CreateNewCodeBlockAfter(Ref insertAfter);
  void SetCurrentCodeBlock(Ref Node);

protected:
  void RemoveArgUses(Ref Node);

  Ref CreateNode(IROp_Header* Op) {
    uintptr_t ListBegin = DualListData.ListBegin();
    size_t Size = sizeof(OrderedNode);
    void* Ptr = DualListData.ListAllocate(Size);
    Ref Node = new (Ptr) OrderedNode();
    Node->Header.Value.SetOffset(DualListData.DataBegin(), reinterpret_cast<uintptr_t>(Op));

    if (CurrentWriteCursor) {
      CurrentWriteCursor->append(ListBegin, Node);
    }
    CurrentWriteCursor = Node;
    return Node;
  }

  Ref GetNode(uint32_t SSANode) {
    uintptr_t ListBegin = DualListData.ListBegin();
    Ref Node = reinterpret_cast<Ref>(ListBegin + SSANode * sizeof(OrderedNode));
    return Node;
  }

  Ref EmplaceOrphanedNode(Ref OldNode) {
    size_t Size = sizeof(OrderedNode);
    Ref Ptr = reinterpret_cast<Ref>(DualListData.ListAllocate(Size));
    memcpy(Ptr, OldNode, Size);
    return Ptr;
  }

  // Overriden by dispatcher, stubbed for IR tests
  virtual void RecordX87Use() {}
  virtual void SaveNZCV(IROps Op) {}

  Ref CurrentWriteCursor = nullptr;

  // These could be combined with a little bit of work to be more efficient with memory usage. Isn't a big deal
  DualIntrusiveAllocatorThreadPool DualListData;

  Ref InvalidNode;
  Ref CurrentCodeBlock {};
  fextl::vector<Ref> CodeBlocks;
  uint64_t Entry;
};

} // namespace FEXCore::IR
