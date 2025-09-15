// SPDX-License-Identifier: MIT
#pragma once

#include "CodeEmitter/Emitter.h"
#include "Interface/IR/IR.h"
#include "Interface/IR/IntrusiveIRList.h"

#include <FEXCore/Core/CoreState.h>
#include <FEXCore/IR/IR.h>

#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/fextl/vector.h>

#include <algorithm>
#include <stdint.h>
#include <string.h>

namespace FEXCore::IR {

class IREmitter {
public:
  IREmitter(FEXCore::Utils::IntrusivePooledAllocator& ThreadAllocator, bool SupportsTSOImm9)
    : DualListData {ThreadAllocator, 8 * 1024 * 1024}
    , SupportsTSOImm9(SupportsTSOImm9) {
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

  // These inlining helpers are used by IRDefines.inc so define first.
  Ref InlineMem(OpSize Size, Ref Offset, MemOffsetType OffsetType, uint8_t& OffsetScale, bool TSO = false) {
    uint64_t Imm {};
    if (OffsetType != MEM_OFFSET_SXTX || !IsValueConstant(WrapNode(Offset), &Imm)) {
      return Offset;
    }

    // The immediate may be scaled in the IR, we need to correct for that.
    Imm *= OffsetScale;

    // Signed immediate unscaled 9-bit range for both regular and LRCPC2 ops.
    bool IsSIMM9 = ((int64_t)Imm >= -256) && ((int64_t)Imm <= 255);
    IsSIMM9 &= (SupportsTSOImm9 || !TSO);

    // Extended offsets for regular loadstore only.
    LOGMAN_THROW_A_FMT(Size >= IR::OpSize::i8Bit && Size <= IR::OpSize::i256Bit, "Must be sized");

    bool IsExtended = (Imm & (IR::OpSizeToSize(Size) - 1)) == 0 && Imm / IR::OpSizeToSize(Size) <= 4095;
    IsExtended &= !TSO;

    if (IsSIMM9 || IsExtended) {
      OffsetScale = 1;
      return _InlineConstant(Imm);
    } else {
      return Offset;
    }
  }

#define DEF_INLINE(Type, Variable, Filter)                          \
  Ref Inline##Type(OpSize Size, Ref Source) {                       \
    uint64_t Variable;                                              \
    if (IsValueConstant(WrapNode(Source), &Variable) && (Filter)) { \
      return _InlineConstant(Variable);                             \
    } else {                                                        \
      return Source;                                                \
    }                                                               \
  }

  DEF_INLINE(Any, _, true)
  DEF_INLINE(Zero, X, X == 0)
  DEF_INLINE(AddSub, X, ARMEmitter::IsImmAddSub(X))
  DEF_INLINE(LargeAddSub, X, ARMEmitter::IsImmAddSub(X) && Size >= OpSize::i32Bit);
  DEF_INLINE(Logical, X, ARMEmitter::Emitter::IsImmLogical(X, std::max((int)IR::OpSizeAsBits(Size), 32)));

  Ref InlineSubtractZero(OpSize Size, Ref Src1, Ref Src2) {
    // Only inline a zero if we won't inline the other source.
    return IsValueConstant(WrapNode(Src2)) ? Src1 : InlineZero(Size, Src1);
  }
#undef DEF_INLINE

// These handlers add cost to the constructor and destructor
// If it becomes an issue then blow them away
// GCC also generates some pretty atrocious code around these
// Use Clang!
#define IROP_ALLOCATE_HELPERS
#define IROP_DISPATCH_HELPERS
#include <FEXCore/IR/IRDefines.inc>
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
  IRPair<IROp_Select> _Select(uint8_t Cond, Ref ssa0, Ref ssa1, Ref ssa2, Ref ssa3, IR::OpSize CompareSize = OpSize::iUnsized) {
    if (CompareSize == OpSize::iUnsized) {
      CompareSize = std::max(OpSize::i32Bit, std::max(GetOpSize(ssa0), GetOpSize(ssa1)));
    }

    return _Select(std::max(OpSize::i32Bit, std::max(GetOpSize(ssa2), GetOpSize(ssa3))), CompareSize, CondClassType {Cond}, ssa0, ssa1, ssa2, ssa3);
  }
  IRPair<IROp_LoadMem> _LoadMem(FEXCore::IR::RegisterClassType Class, IR::OpSize Size, Ref ssa0, IR::OpSize Align = OpSize::i8Bit) {
    return _LoadMem(Class, Size, ssa0, Invalid(), Align, MEM_OFFSET_SXTX, 1);
  }
  IRPair<IROp_StoreMem> _StoreMem(FEXCore::IR::RegisterClassType Class, IR::OpSize Size, Ref Addr, Ref Value, IR::OpSize Align = OpSize::i8Bit) {
    return _StoreMem(Class, Size, Value, Addr, Invalid(), Align, MEM_OFFSET_SXTX, 1);
  }

  IRPair<IROp_Select> Select01(FEXCore::IR::OpSize CompareSize, CondClassType Cond, OrderedNode* Cmp1, OrderedNode* Cmp2) {
    return _Select(OpSize::i64Bit, CompareSize, Cond, Cmp1, Cmp2, _InlineConstant(1), _InlineConstant(0));
  }

  IRPair<IROp_Select> To01(FEXCore::IR::OpSize CompareSize, OrderedNode* Cmp1) {
    return Select01(CompareSize, CondClassType {COND_NEQ}, Cmp1, Constant(0));
  }

  IRPair<IROp_NZCVSelect> _NZCVSelect01(CondClassType Cond) {
    return _NZCVSelect(OpSize::i64Bit, Cond, _InlineConstant(1), _InlineConstant(0));
  }

  Ref Addsub(IR::OpSize Size, IROps Op, IROps NegatedOp, Ref Src1, uint64_t Src2) {
    // Sign-extend the constant
    if (Size == OpSize::i32Bit) {
      Src2 = (int64_t)(int32_t)Src2;
    }

    // Negative constants need to be negated to inline.
    if (Src2 & (1ull << 63) && ARMEmitter::IsImmAddSub(-Src2)) {
      Op = NegatedOp;
      Src2 = -Src2;
    }

    auto Dest = _Add(Size, Src1, Constant(Src2));
    Dest.first->Header.Op = Op;
    return Dest;
  }

  Ref Add(IR::OpSize Size, Ref Src1, uint64_t Src2) {
    return Addsub(Size, OP_ADD, OP_SUB, Src1, Src2);
  }

  Ref Sub(IR::OpSize Size, Ref Src1, uint64_t Src2) {
    return Addsub(Size, OP_SUB, OP_ADD, Src1, Src2);
  }

  Ref AddWithFlags(IR::OpSize Size, Ref Src1, uint64_t Src2) {
    return Addsub(Size, OP_ADDWITHFLAGS, OP_SUBWITHFLAGS, Src1, Src2);
  }

  Ref SubWithFlags(IR::OpSize Size, Ref Src1, uint64_t Src2) {
    return Addsub(Size, OP_SUBWITHFLAGS, OP_ADDWITHFLAGS, Src1, Src2);
  }

#define DEF_ADDSUB(Op)                                \
  Ref Op(IR::OpSize Size, Ref Src1, Ref Src2) {       \
    uint64_t Constant;                                \
    if (IsValueConstant(WrapNode(Src2), &Constant)) { \
      return Op(Size, Src1, Constant);                \
    } else {                                          \
      return _##Op(Size, Src1, Src2);                 \
    }                                                 \
  }

  DEF_ADDSUB(Add)
  DEF_ADDSUB(Sub)
  DEF_ADDSUB(AddWithFlags)
  DEF_ADDSUB(SubWithFlags)

  int64_t Constants[32];
  Ref ConstantRefs[32];
  uint32_t NrConstants;

  Ref Constant(int64_t Value) {
    // Search for the constant in the pool.
    for (unsigned i = 0; i < std::min(NrConstants, 32u); ++i) {
      if (Constants[i] == Value) {
        return ConstantRefs[i];
      }
    }

    // Otherwise, materialize a fresh constant and pool it.
    Ref R = _Constant(Value);
    unsigned i = (NrConstants++) & 31;
    Constants[i] = Value;
    ConstantRefs[i] = R;
    return R;
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

  void ReplaceNodeArgument(Ref Node, uint8_t Arg, Ref NewArg);

  void Remove(Ref Node);
  void RemovePostRA(Ref Node);

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

    SetWriteCursor((*Before).Node);
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
  IRPair<IROp_CodeBlock> CreateCodeNode(bool EntryPoint = false, uint32_t GuestEntryOffset = 0) {
    SetWriteCursor(nullptr); // Orphan from any previous nodes

    auto ID = ViewIR().GetHeader()->BlockCount++;
    auto CodeNode = _CodeBlock(InvalidNode, InvalidNode, ID, EntryPoint, GuestEntryOffset);

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
    [[maybe_unused]] auto CurrentIROp = CodeNode->Op(DualListData.DataBegin())->CW<FEXCore::IR::IROp_CodeBlock>();
#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
    LOGMAN_THROW_A_FMT(CurrentIROp->Header.Op == IROps::OP_CODEBLOCK, "Invalid");
#endif

    CodeNode->append(DualListData.ListBegin(), Next);
  }

  IRPair<IROp_CodeBlock> CreateNewCodeBlockAtEnd() {
    return CreateNewCodeBlockAfter(nullptr);
  }
  IRPair<IROp_CodeBlock> CreateNewCodeBlockAfter(Ref insertAfter);
  void SetCurrentCodeBlock(Ref Node);

  // Helper on reduced precision to read FPSR register and set IOBit
  void CheckFPSRIOCAndSetIOBit(Ref OrWith = nullptr) {
    // Read FPSR register
    Ref FPSR = _ReadFPSR();

    // Extract IOC bit (bit 0) from FPSR
    Ref IOCBit = _And(OpSize::i32Bit, FPSR, _Constant(1));

    if (OrWith) {
      // If OrWith is provided, OR it with the IOCBit
      IOCBit = _Or(OpSize::i32Bit, IOCBit, OrWith);
    }

    // Set x87 invalid operation bit if IOC is set
    Ref CurrentIE32 = _LoadContext(OpSize::i32Bit, GPRClass, offsetof(FEXCore::Core::CPUState, flags) + FEXCore::X86State::X87FLAG_IE_LOC);
    Ref NewIE32 = _Or(OpSize::i32Bit, CurrentIE32, IOCBit);
    _StoreContext(OpSize::i32Bit, GPRClass, NewIE32, offsetof(FEXCore::Core::CPUState, flags) + FEXCore::X86State::X87FLAG_IE_LOC);
  }

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

  // MMX State can be either MMX (for 64bit) or x87 FPU (for 80bit)
  enum { MMXState_MMX, MMXState_X87 } MMXState = MMXState_MMX;

  // Overriden by dispatcher, stubbed for IR tests
  virtual void RecordX87Use() {}
  virtual void ChgStateX87_MMX() {}
  virtual void ChgStateMMX_X87() {}
  virtual void SaveNZCV(IROps Op) {}

  Ref CurrentWriteCursor = nullptr;

  // These could be combined with a little bit of work to be more efficient with memory usage. Isn't a big deal
  DualIntrusiveAllocatorThreadPool DualListData;

  Ref InvalidNode {};
  Ref CurrentCodeBlock {};
  fextl::vector<Ref> CodeBlocks;
  uint64_t Entry {};
  bool SupportsTSOImm9 {};
};

} // namespace FEXCore::IR
