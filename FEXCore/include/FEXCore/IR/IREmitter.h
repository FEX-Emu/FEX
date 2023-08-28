#pragma once
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/IR/IntrusiveIRList.h>
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
    IREmitter(FEXCore::Utils::IntrusivePooledAllocator &ThreadAllocator)
      : DualListData {ThreadAllocator, 8 * 1024 * 1024} {
      ReownOrClaimBuffer();
      ResetWorkingList();
    }

    void ReownOrClaimBuffer() {
      DualListData.ReownOrClaimBuffer();
    }

    void DelayedDisownBuffer() {
      DualListData.DelayedDisownBuffer();
    }

    IRListView ViewIR() { return IRListView(&DualListData, false); }
    IRListView *CreateIRCopy() { return new IRListView(&DualListData, true); }
    void ResetWorkingList();

  /**
   * @name IR allocation routines
   *
   * @{ */

    FEXCore::IR::RegisterClassType WalkFindRegClass(OrderedNode *Node);

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
  IRPair<IROp_Sbfe> _Sext(uint8_t SrcSize, OrderedNode *ssa0) {
    return _Sbfe(OpSize::i64Bit, SrcSize, 0, ssa0);
  }
  IRPair<IROp_Jump> _Jump() {
    return _Jump(InvalidNode);
  }
  IRPair<IROp_CondJump> _CondJump(OrderedNode *ssa0, CondClassType cond = {COND_NEQ}) {
    return _CondJump(ssa0, _Constant(0), InvalidNode, InvalidNode, cond, GetOpSize(ssa0));
  }
  IRPair<IROp_CondJump> _CondJump(OrderedNode *ssa0, OrderedNode *ssa1, OrderedNode *ssa2, CondClassType cond = {COND_NEQ}) {
    return _CondJump(ssa0, _Constant(0), ssa1, ssa2, cond, GetOpSize(ssa0));
  }
  IRPair<IROp_Select> _Select(uint8_t Cond, OrderedNode *ssa0, OrderedNode *ssa1, OrderedNode *ssa2, OrderedNode *ssa3, uint8_t CompareSize = 0) {
    if (CompareSize == 0)
      CompareSize = std::max<uint8_t>(4, std::max<uint8_t>(GetOpSize(ssa0), GetOpSize(ssa1)));

    return _Select(CondClassType{Cond}, ssa0, ssa1, ssa2, ssa3, CompareSize);
  }
  IRPair<IROp_LoadMem> _LoadMem(FEXCore::IR::RegisterClassType Class, uint8_t Size, OrderedNode *ssa0, uint8_t Align = 1) {
    return _LoadMem(Class, Size, ssa0, Invalid(), Align, MEM_OFFSET_SXTX, 1);
  }
  IRPair<IROp_LoadMemTSO> _LoadMemTSO(FEXCore::IR::RegisterClassType Class, uint8_t Size, OrderedNode *ssa0, uint8_t Align = 1) {
    return _LoadMemTSO(Class, Size, ssa0, Invalid(), Align, MEM_OFFSET_SXTX, 1);
  }
  IRPair<IROp_StoreMem> _StoreMem(FEXCore::IR::RegisterClassType Class, uint8_t Size, OrderedNode *Addr, OrderedNode *Value, uint8_t Align = 1) {
    return _StoreMem(Class, Size, Value, Addr, Invalid(), Align, MEM_OFFSET_SXTX, 1);
  }
  IRPair<IROp_StoreMemTSO> _StoreMemTSO(FEXCore::IR::RegisterClassType Class, uint8_t Size, OrderedNode *Addr, OrderedNode *Value, uint8_t Align = 1) {
    return _StoreMemTSO(Class, Size, Value, Addr, Invalid(), Align, MEM_OFFSET_SXTX, 1);
  }
  IRPair<IROp_Lshl> _Lshl(OrderedNode *Src1, OrderedNode *Src2) {
    return _Lshl(std::max<uint8_t>(4, GetOpSize(Src1)), Src1, Src2);
  }
  IRPair<IROp_Lshr> _Lshr(OrderedNode *Src1, OrderedNode *Src2) {
    return _Lshr(std::max<uint8_t>(4, GetOpSize(Src1)), Src1, Src2);
  }
  OrderedNode *Invalid() {
    return InvalidNode;
  }

  // Temporary naughty implicit IR operation handlers
  ///< Moves
  IRPair<IROp_ExtractElementPair> _ExtractElementPair(OrderedNode *_Pair, uint8_t _Element) {
    return _ExtractElementPair(static_cast<OpSize>(GetOpSize(_Pair) >> 1), _Pair, _Element);
  }
  IRPair<IROp_CreateElementPair> _CreateElementPair(OrderedNode *_Lower, OrderedNode *_Upper) {
    return _CreateElementPair(static_cast<OpSize>(GetOpSize(_Lower) * 2), _Lower, _Upper);
  }
  ///< Atomics
  IRPair<IROp_CAS> _CAS(OrderedNode *_Expected, OrderedNode *_Desired, OrderedNode *_Addr) {
    return _CAS(static_cast<OpSize>(GetOpSize(_Expected)), _Expected, _Desired, _Addr);
  }
  IRPair<IROp_CASPair> _CASPair(OrderedNode *_Expected, OrderedNode *_Desired, OrderedNode *_Addr) {
    return _CASPair(static_cast<OpSize>(GetOpSize(_Expected)), _Expected, _Desired, _Addr);
  }
  IRPair<IROp_AtomicAdd> _AtomicAdd(uint8_t Size, OrderedNode *_Value, OrderedNode *_Addr) {
    return _AtomicAdd(static_cast<OpSize>(Size), _Value, _Addr);
  }
  IRPair<IROp_AtomicSub> _AtomicSub(uint8_t Size, OrderedNode *_Value, OrderedNode *_Addr) {
    return _AtomicSub(static_cast<OpSize>(Size), _Value, _Addr);
  }
  IRPair<IROp_AtomicAnd> _AtomicAnd(uint8_t Size, OrderedNode *_Value, OrderedNode *_Addr) {
    return _AtomicAnd(static_cast<OpSize>(Size), _Value, _Addr);
  }
  IRPair<IROp_AtomicOr> _AtomicOr(uint8_t Size, OrderedNode *_Value, OrderedNode *_Addr) {
    return _AtomicOr(static_cast<OpSize>(Size), _Value, _Addr);
  }
  IRPair<IROp_AtomicXor> _AtomicXor(uint8_t Size, OrderedNode *_Value, OrderedNode *_Addr) {
    return _AtomicXor(static_cast<OpSize>(Size), _Value, _Addr);
  }
  IRPair<IROp_AtomicSwap> _AtomicSwap(uint8_t Size, OrderedNode *_Value, OrderedNode *_Addr) {
    return _AtomicSwap(static_cast<OpSize>(Size), _Value, _Addr);
  }
  IRPair<IROp_AtomicFetchAdd> _AtomicFetchAdd(uint8_t Size, OrderedNode *_Value, OrderedNode *_Addr) {
    return _AtomicFetchAdd(static_cast<OpSize>(Size), _Value, _Addr);
  }
  IRPair<IROp_AtomicFetchSub> _AtomicFetchSub(uint8_t Size, OrderedNode *_Value, OrderedNode *_Addr) {
    return _AtomicFetchSub(static_cast<OpSize>(Size), _Value, _Addr);
  }
  IRPair<IROp_AtomicFetchAnd> _AtomicFetchAnd(uint8_t Size, OrderedNode *_Value, OrderedNode *_Addr) {
    return _AtomicFetchAnd(static_cast<OpSize>(Size), _Value, _Addr);
  }
  IRPair<IROp_AtomicFetchOr> _AtomicFetchOr(uint8_t Size, OrderedNode *_Value, OrderedNode *_Addr) {
    return _AtomicFetchOr(static_cast<OpSize>(Size), _Value, _Addr);
  }
  IRPair<IROp_AtomicFetchXor> _AtomicFetchXor(uint8_t Size, OrderedNode *_Value, OrderedNode *_Addr) {
    return _AtomicFetchXor(static_cast<OpSize>(Size), _Value, _Addr);
  }
  IRPair<IROp_AtomicFetchNeg> _AtomicFetchNeg(uint8_t Size, OrderedNode *_Addr) {
    return _AtomicFetchNeg(static_cast<OpSize>(Size), _Addr);
  }
  ///< ALU
  IRPair<IROp_Add> _Add(OrderedNode *_Src1, OrderedNode *_Src2) {
    return _Add(static_cast<OpSize>(std::max<uint8_t>(4, std::max(GetOpSize(_Src1), GetOpSize(_Src2)))), _Src1, _Src2);
  }
  IRPair<IROp_Sub> _Sub(OrderedNode *_Src1, OrderedNode *_Src2) {
    return _Sub(static_cast<OpSize>(std::max<uint8_t>(4, std::max(GetOpSize(_Src1), GetOpSize(_Src2)))), _Src1, _Src2);
  }
  IRPair<IROp_Or> _Or(OrderedNode *_Src1, OrderedNode *_Src2) {
    return _Or(static_cast<OpSize>(std::max(GetOpSize(_Src1), GetOpSize(_Src2))), _Src1, _Src2);
  }
  IRPair<IROp_Xor> _Xor(OrderedNode *_Src1, OrderedNode *_Src2) {
    return _Xor(static_cast<OpSize>(std::max<uint8_t>(4, std::max(GetOpSize(_Src1), GetOpSize(_Src2)))), _Src1, _Src2);
  }
  IRPair<IROp_And> _And(OrderedNode *_Src1, OrderedNode *_Src2) {
    return _And(static_cast<OpSize>(std::max<uint8_t>(4, std::max(GetOpSize(_Src1), GetOpSize(_Src2)))), _Src1, _Src2);
  }
  IRPair<IROp_Andn> _Andn(OrderedNode *_Src1, OrderedNode *_Src2) {
    return _Andn(static_cast<OpSize>(std::max<uint8_t>(4, std::max(GetOpSize(_Src1), GetOpSize(_Src2)))), _Src1, _Src2);
  }
  IRPair<IROp_Lshl> _Lshl(uint8_t Size, OrderedNode *_Src1, OrderedNode *_Src2) {
    return _Lshl(static_cast<OpSize>(Size), _Src1, _Src2);
  }
  IRPair<IROp_Lshr> _Lshr(uint8_t Size, OrderedNode *_Src1, OrderedNode *_Src2) {
    return _Lshr(static_cast<OpSize>(Size), _Src1, _Src2);
  }
  IRPair<IROp_Mul> _Mul(OrderedNode *_Src1, OrderedNode *_Src2) {
    return _Mul(static_cast<OpSize>(std::max<uint8_t>(4, std::max(GetOpSize(_Src1), GetOpSize(_Src2)))), _Src1, _Src2);
  }
  IRPair<IROp_UMul> _UMul(OrderedNode *_Src1, OrderedNode *_Src2) {
    return _UMul(static_cast<OpSize>(std::max<uint8_t>(4, std::max(GetOpSize(_Src1), GetOpSize(_Src2)))), _Src1, _Src2);
  }
  IRPair<IROp_MulH> _MulH(OrderedNode *_Src1, OrderedNode *_Src2) {
    return _MulH(static_cast<OpSize>(std::max<uint8_t>(4, std::max(GetOpSize(_Src1), GetOpSize(_Src2)))), _Src1, _Src2);
  }
  IRPair<IROp_UMulH> _UMulH(OrderedNode *_Src1, OrderedNode *_Src2) {
    return _UMulH(static_cast<OpSize>(std::max<uint8_t>(4, std::max(GetOpSize(_Src1), GetOpSize(_Src2)))), _Src1, _Src2);
  }
  IRPair<IROp_Bfi> _Bfi(uint8_t DestSize, uint8_t _Width, uint8_t _lsb, OrderedNode *_Dest, OrderedNode *_Src) {
    return _Bfi(static_cast<OpSize>(DestSize), _Width, _lsb, _Dest, _Src);
  }
  IRPair<IROp_Bfe> _Bfe(uint8_t DestSize, uint8_t _Width, uint8_t _lsb, OrderedNode *_Src) {
    return _Bfe(static_cast<OpSize>(DestSize != 0 ? DestSize : GetOpSize(_Src)), _Width, _lsb, _Src);
  }
  IRPair<IROp_Bfe> _Bfe(uint8_t Width, uint8_t lsb, OrderedNode *ssa0) {
    return _Bfe(0, Width, lsb, ssa0);
  }
  // End of Temporary naughty implicit IR operation handlers

  void AddPhiValue(IR::IROp_Phi *Phi, OrderedNode *Value) {
    // Got to do some bookkeeping first
    Value->AddUse();
    auto ValueIROp = Value->Op(DualListData.DataBegin())->C<IR::IROp_PhiValue>()->Value.GetNode(DualListData.ListBegin())->Op(DualListData.DataBegin());
    Phi->Header.Size = ValueIROp->Size;
    Phi->Header.ElementSize = ValueIROp->ElementSize;

    if (Phi->PhiBegin.ID().IsInvalid()) {
      Phi->PhiBegin = Phi->PhiEnd = Value->Wrapped(DualListData.ListBegin());
      return;
    }
    auto PhiValueEndNode = Phi->PhiEnd.GetNode(DualListData.ListBegin());
    auto PhiValueEndOp = PhiValueEndNode->Op(DualListData.DataBegin())->CW<IR::IROp_PhiValue>();
    PhiValueEndOp->Next = Value->Wrapped(DualListData.ListBegin());
  }

  void SetJumpTarget(IR::IROp_Jump *Op, OrderedNode *Target) {
    LOGMAN_THROW_A_FMT(Target->Op(DualListData.DataBegin())->Op == OP_CODEBLOCK,
        "Tried setting Jump target to %{} {}",
        Target->Wrapped(DualListData.ListBegin()).ID(),
        IR::GetName(Target->Op(DualListData.DataBegin())->Op));

    Op->Header.Args[0].NodeOffset = Target->Wrapped(DualListData.ListBegin()).NodeOffset;
  }
  void SetTrueJumpTarget(IR::IROp_CondJump *Op, OrderedNode *Target) {
    LOGMAN_THROW_A_FMT(Target->Op(DualListData.DataBegin())->Op == OP_CODEBLOCK,
        "Tried setting CondJump target to %{} {}",
        Target->Wrapped(DualListData.ListBegin()).ID(),
        IR::GetName(Target->Op(DualListData.DataBegin())->Op));

    Op->TrueBlock.NodeOffset = Target->Wrapped(DualListData.ListBegin()).NodeOffset;
  }
  void SetFalseJumpTarget(IR::IROp_CondJump *Op, OrderedNode *Target) {
    LOGMAN_THROW_A_FMT(Target->Op(DualListData.DataBegin())->Op == OP_CODEBLOCK,
        "Tried setting CondJump target to %{} {}",
        Target->Wrapped(DualListData.ListBegin()).ID(),
        IR::GetName(Target->Op(DualListData.DataBegin())->Op));

    Op->FalseBlock.NodeOffset = Target->Wrapped(DualListData.ListBegin()).NodeOffset;
  }

  void SetJumpTarget(IRPair<IROp_Jump> Op, OrderedNode *Target) {
    LOGMAN_THROW_A_FMT(Target->Op(DualListData.DataBegin())->Op == OP_CODEBLOCK,
        "Tried setting Jump target to %{} {}",
        Target->Wrapped(DualListData.ListBegin()).ID(),
        IR::GetName(Target->Op(DualListData.DataBegin())->Op));

    Op.first->Header.Args[0].NodeOffset = Target->Wrapped(DualListData.ListBegin()).NodeOffset;
  }
  void SetTrueJumpTarget(IRPair<IROp_CondJump> Op, OrderedNode *Target) {
    LOGMAN_THROW_A_FMT(Target->Op(DualListData.DataBegin())->Op == OP_CODEBLOCK,
        "Tried setting CondJump target to %{} {}",
        Target->Wrapped(DualListData.ListBegin()).ID(),
        IR::GetName(Target->Op(DualListData.DataBegin())->Op));
    Op.first->TrueBlock.NodeOffset = Target->Wrapped(DualListData.ListBegin()).NodeOffset;
  }
  void SetFalseJumpTarget(IRPair<IROp_CondJump> Op, OrderedNode *Target) {
    LOGMAN_THROW_A_FMT(Target->Op(DualListData.DataBegin())->Op == OP_CODEBLOCK,
        "Tried setting CondJump target to %{} {}",
        Target->Wrapped(DualListData.ListBegin()).ID(),
        IR::GetName(Target->Op(DualListData.DataBegin())->Op));
    Op.first->FalseBlock.NodeOffset = Target->Wrapped(DualListData.ListBegin()).NodeOffset;
  }

  /**  @} */
  FEXCore::IR::RegisterClassType WalkFindRegClass(OrderedNodeWrapper ssa) {
     OrderedNode *RealNode = ssa.GetNode(DualListData.ListBegin());
     return WalkFindRegClass(RealNode);
  }

  bool IsValueConstant(OrderedNodeWrapper ssa, uint64_t *Constant = nullptr) {
     OrderedNode *RealNode = ssa.GetNode(DualListData.ListBegin());
     FEXCore::IR::IROp_Header *IROp = RealNode->Op(DualListData.DataBegin());
     if (IROp->Op == OP_CONSTANT) {
       auto Op = IROp->C<IR::IROp_Constant>();
       if (Constant) *Constant = Op->Constant;
       return true;
     }
     return false;
  }

  bool IsValueInlineConstant(OrderedNodeWrapper ssa) {
     OrderedNode *RealNode = ssa.GetNode(DualListData.ListBegin());
     FEXCore::IR::IROp_Header *IROp = RealNode->Op(DualListData.DataBegin());
     if (IROp->Op == OP_INLINECONSTANT) {
       return true;
     }
     return false;
  }

  FEXCore::IR::IROp_Header *GetOpHeader(OrderedNodeWrapper ssa) {
    OrderedNode *RealNode = ssa.GetNode(DualListData.ListBegin());
    return RealNode->Op(DualListData.DataBegin());
  }

  OrderedNode *UnwrapNode(OrderedNodeWrapper ssa) {
    return ssa.GetNode(DualListData.ListBegin());
  }

  OrderedNodeWrapper WrapNode(OrderedNode *node) {
    return node->Wrapped(DualListData.ListBegin());
  }

  NodeIterator GetIterator(OrderedNodeWrapper wrapper) {
    return NodeIterator(DualListData.ListBegin(), DualListData.DataBegin(), wrapper);
  }

  // Overwrite a node with a constant
  // Depending on what node has been overwritten, there might be some unallocated space around the node
  // Because we are overwriting the node, we don't have to worry about update all the arguments which use it
  void ReplaceWithConstant(OrderedNode *Node, uint64_t Value);

  void ReplaceAllUsesWithRange(OrderedNode *Node, OrderedNode *NewNode, AllNodesIterator Begin, AllNodesIterator End);

  void ReplaceUsesWithAfter(OrderedNode *Node, OrderedNode *NewNode, AllNodesIterator After) {
    ++After;
    ReplaceAllUsesWithRange(Node, NewNode, After, AllNodesIterator(DualListData.ListBegin(), DualListData.DataBegin()));
  }

  void ReplaceUsesWithAfter(OrderedNode *Node, OrderedNode *NewNode, OrderedNode *After) {
    auto Wrapped = After->Wrapped(DualListData.ListBegin());
    AllNodesIterator It = AllNodesIterator(DualListData.ListBegin(), DualListData.DataBegin(), Wrapped);

    ReplaceUsesWithAfter(Node, NewNode, It);
  }

  void ReplaceAllUsesWith(OrderedNode *Node, OrderedNode *NewNode) {
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

  void ReplaceNodeArgument(OrderedNode *Node, uint8_t Arg, OrderedNode *NewArg);

  void Remove(OrderedNode *Node);

  void SetPackedRFLAG(bool Lower8, OrderedNode *Src);
  OrderedNode *GetPackedRFLAG(bool Lower8);

  void CopyData(IREmitter const &rhs) {
    LOGMAN_THROW_A_FMT(rhs.DualListData.DataBackingSize() <= DualListData.DataBackingSize(), "Trying to take ownership of data that is too large");
    LOGMAN_THROW_A_FMT(rhs.DualListData.ListBackingSize() <= DualListData.ListBackingSize(), "Trying to take ownership of data that is too large");
    DualListData.CopyData(rhs.DualListData);
    InvalidNode = rhs.InvalidNode->Wrapped(rhs.DualListData.ListBegin()).GetNode(DualListData.ListBegin());
    CurrentWriteCursor = rhs.CurrentWriteCursor;
    CodeBlocks = rhs.CodeBlocks;
    for (auto& CodeBlock: CodeBlocks) {
      CodeBlock = CodeBlock->Wrapped(rhs.DualListData.ListBegin()).GetNode(DualListData.ListBegin());
    }
  }

  void SetWriteCursor(OrderedNode *Node) {
    CurrentWriteCursor = Node;
  }

  OrderedNode *GetWriteCursor() {
    return CurrentWriteCursor;
  }

  OrderedNode *GetCurrentBlock() {
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

    SetWriteCursor(nullptr);// Orphan from any future nodes

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
  void LinkCodeBlocks(OrderedNode *CodeNode, OrderedNode *Next) {
#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
    FEXCore::IR::IROp_CodeBlock *CurrentIROp =
#endif
    CodeNode->Op(DualListData.DataBegin())->CW<FEXCore::IR::IROp_CodeBlock>();

    LOGMAN_THROW_A_FMT(CurrentIROp->Header.Op == IROps::OP_CODEBLOCK, "Invalid");

    CodeNode->append(DualListData.ListBegin(), Next);
  }

  IRPair<IROp_CodeBlock> CreateNewCodeBlockAtEnd() { return CreateNewCodeBlockAfter(nullptr); }
  IRPair<IROp_CodeBlock> CreateNewCodeBlockAfter(OrderedNode* insertAfter);
  void SetCurrentCodeBlock(OrderedNode *Node);

  protected:
    void RemoveArgUses(OrderedNode *Node);

    OrderedNode *CreateNode(IROp_Header *Op) {
      uintptr_t ListBegin = DualListData.ListBegin();
      size_t Size = sizeof(OrderedNode);
      void *Ptr = DualListData.ListAllocate(Size);
      OrderedNode *Node = new (Ptr) OrderedNode();
      Node->Header.Value.SetOffset(DualListData.DataBegin(), reinterpret_cast<uintptr_t>(Op));

      if (CurrentWriteCursor) {
        CurrentWriteCursor->append(ListBegin, Node);
      }
      CurrentWriteCursor = Node;
      return Node;
    }

    OrderedNode *GetNode(uint32_t SSANode) {
      uintptr_t ListBegin = DualListData.ListBegin();
      OrderedNode *Node = reinterpret_cast<OrderedNode *>(ListBegin + SSANode * sizeof(OrderedNode));
      return Node;
    }

    OrderedNode *EmplaceOrphanedNode(OrderedNode *OldNode) {
      size_t Size = sizeof(OrderedNode);
      OrderedNode *Ptr = reinterpret_cast<OrderedNode*>(DualListData.ListAllocate(Size));
      memcpy(Ptr, OldNode, Size);
      return Ptr;
    }

    OrderedNode *CurrentWriteCursor = nullptr;

    // These could be combined with a little bit of work to be more efficient with memory usage. Isn't a big deal
    DualIntrusiveAllocatorThreadPool DualListData;

    OrderedNode *InvalidNode;
    OrderedNode *CurrentCodeBlock{};
    fextl::vector<OrderedNode*> CodeBlocks;
    uint64_t Entry;
};

}
