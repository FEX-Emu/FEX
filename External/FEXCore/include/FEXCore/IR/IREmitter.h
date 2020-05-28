#pragma once
#include <FEXCore/IR/IntrusiveIRList.h>
#include <FEXCore/IR/IR.h>

#include "LogManager.h"

namespace FEXCore::IR {
class Pass;
class PassManager;

class IREmitter {
friend class FEXCore::IR::Pass;
friend class FEXCore::IR::PassManager;

  public:
    IREmitter()
      : Data {8 * 1024 * 1024}
      , ListData {8 * 1024 * 1024} {
      ResetWorkingList();
    }

    IRListView<false> ViewIR() { return IRListView<false>(&Data, &ListData); }
    IRListView<true> *CreateIRCopy() { return new IRListView<true>(&Data, &ListData); }
    void ResetWorkingList();

  /**
   * @name IR allocation routines
   *
   * @{ */

// These handlers add cost to the constructor and destructor
// If it becomes an issue then blow them away
// GCC also generates some pretty atrocious code around these
// Use Clang!
#define IROP_ALLOCATE_HELPERS
#define IROP_DISPATCH_HELPERS
#include <FEXCore/IR/IRDefines.inc>

  IRPair<IROp_Constant> _Constant(uint8_t Size, uint64_t Constant) {
    auto Op = AllocateOp<IROp_Constant, IROps::OP_CONSTANT>();
    Op.first->Constant = Constant;
    Op.first->Header.Size = Size / 8;
    Op.first->Header.ElementSize = Size / 8;
    Op.first->Header.NumArgs = 0;
    Op.first->Header.HasDest = true;
    return Op;
  }
  IRPair<IROp_VBitcast> _VBitcast(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0) {
    auto Op = AllocateOp<IROp_VBitcast, IROps::OP_VBITCAST>();
    Op.first->Header.Size = RegisterSize / 8;
    Op.first->Header.ElementSize = ElementSize;
    Op.first->Header.NumArgs = 1;
    Op.first->Header.HasDest = true;
    Op.first->Header.Args[0] = ssa0->Wrapped(ListData.Begin());
    ssa0->AddUse();
    return Op;
  }
  IRPair<IROp_LoadContext> _LoadContext(uint8_t Size, uint32_t Offset, RegisterClassType Class) {
    return _LoadContext(Offset, Class, Size);
  }
  IRPair<IROp_StoreContext> _StoreContext(RegisterClassType Class, uint8_t Size, uint32_t Offset, OrderedNode *ssa0) {
    return _StoreContext(ssa0, Offset, Class, Size);
  }
  IRPair<IROp_Bfe> _Bfe(uint8_t Width, uint8_t lsb, OrderedNode *ssa0) {
    return _Bfe(ssa0, Width, lsb);
  }
  IRPair<IROp_Sbfe> _Sbfe(uint8_t Width, uint8_t lsb, OrderedNode *ssa0) {
    return _Sbfe(ssa0, Width, lsb);
  }
  IRPair<IROp_Bfi> _Bfi(uint8_t Width, uint8_t lsb, OrderedNode *ssa0, OrderedNode *ssa1) {
    return _Bfi(ssa0, ssa1, Width, lsb);
  }
  IRPair<IROp_StoreMem> _StoreMem(FEXCore::IR::RegisterClassType Class, uint8_t Size, OrderedNode *ssa0, OrderedNode *ssa1, uint8_t Align = 1) {
    return _StoreMem(ssa0, ssa1, Size, Align, Class);
  }
  IRPair<IROp_VStoreMemElement> _VStoreMemElement(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, OrderedNode *ssa1, uint8_t Index, uint8_t Align = 1) {
    return _VStoreMemElement(ssa0, ssa1, Index, Align, RegisterSize, ElementSize);
  }
  IRPair<IROp_LoadMem> _LoadMem(FEXCore::IR::RegisterClassType Class, uint8_t Size, OrderedNode *ssa0, uint8_t Align = 1) {
    return _LoadMem(ssa0, Size, Align, Class);
  }
  IRPair<IROp_VLoadMemElement> _VLoadMemElement(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, OrderedNode *ssa1, uint8_t Index, uint8_t Align = 1) {
    return _VLoadMemElement(ssa0, ssa1, Index, Align, RegisterSize, ElementSize);
  }
  IRPair<IROp_Select> _Select(uint8_t Cond, OrderedNode *ssa0, OrderedNode *ssa1, OrderedNode *ssa2, OrderedNode *ssa3) {
    return _Select(ssa0, ssa1, ssa2, ssa3, {Cond});
  }
  IRPair<IROp_Sext> _Sext(uint8_t SrcSize, OrderedNode *ssa0) {
    return _Sext(ssa0, SrcSize);
  }
  IRPair<IROp_Zext> _Zext(uint8_t SrcSize, OrderedNode *ssa0) {
    return _Zext(ssa0, SrcSize);
  }
  IRPair<IROp_VInsElement> _VInsElement(uint8_t RegisterSize, uint8_t ElementSize, uint8_t DestIdx, uint8_t SrcIdx, OrderedNode *ssa0, OrderedNode *ssa1) {
    return _VInsElement(ssa0, ssa1, DestIdx, SrcIdx, RegisterSize, ElementSize);
  }
  IRPair<IROp_VInsScalarElement> _VInsScalarElement(uint8_t RegisterSize, uint8_t ElementSize, uint8_t DestIdx, OrderedNode *ssa0, OrderedNode *ssa1) {
    return _VInsScalarElement(ssa0, ssa1, DestIdx, RegisterSize, ElementSize);
  }
  IRPair<IROp_VExtractElement> _VExtractElement(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, uint8_t Index) {
    return _VExtractElement(ssa0, Index, RegisterSize, ElementSize);
  }
  IRPair<IROp_VAnd> _VAnd(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, OrderedNode *ssa1) {
    return _VAnd(ssa0, ssa1, RegisterSize, ElementSize);
  }
  IRPair<IROp_VAdd> _VAdd(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, OrderedNode *ssa1) {
    return _VAdd(ssa0, ssa1, RegisterSize, ElementSize);
  }
  IRPair<IROp_VSub> _VSub(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, OrderedNode *ssa1) {
    return _VSub(ssa0, ssa1, RegisterSize, ElementSize);
  }
  IRPair<IROp_VUQAdd> _VUQAdd(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, OrderedNode *ssa1) {
    return _VUQAdd(ssa0, ssa1, RegisterSize, ElementSize);
  }
  IRPair<IROp_VUQSub> _VUQSub(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, OrderedNode *ssa1) {
    return _VUQSub(ssa0, ssa1, RegisterSize, ElementSize);
  }
  IRPair<IROp_VSQAdd> _VSQAdd(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, OrderedNode *ssa1) {
    return _VSQAdd(ssa0, ssa1, RegisterSize, ElementSize);
  }
  IRPair<IROp_VSQSub> _VSQSub(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, OrderedNode *ssa1) {
    return _VSQSub(ssa0, ssa1, RegisterSize, ElementSize);
  }
  IRPair<IROp_VAddP> _VAddP(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, OrderedNode *ssa1) {
    return _VAddP(ssa0, ssa1, RegisterSize, ElementSize);
  }
  IRPair<IROp_VUMin> _VUMin(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, OrderedNode *ssa1) {
    return _VUMin(ssa0, ssa1, RegisterSize, ElementSize);
  }
  IRPair<IROp_VSMin> _VSMin(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, OrderedNode *ssa1) {
    return _VSMin(ssa0, ssa1, RegisterSize, ElementSize);
  }
  IRPair<IROp_VUMax> _VUMax(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, OrderedNode *ssa1) {
    return _VUMax(ssa0, ssa1, RegisterSize, ElementSize);
  }
  IRPair<IROp_VSMax> _VSMax(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, OrderedNode *ssa1) {
    return _VSMax(ssa0, ssa1, RegisterSize, ElementSize);
  }
  IRPair<IROp_VZip> _VZip(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, OrderedNode *ssa1) {
    return _VZip(ssa0, ssa1, RegisterSize, ElementSize);
  }
  IRPair<IROp_VZip2> _VZip2(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, OrderedNode *ssa1) {
    return _VZip2(ssa0, ssa1, RegisterSize, ElementSize);
  }
  IRPair<IROp_VCMPEQ> _VCMPEQ(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, OrderedNode *ssa1) {
    return _VCMPEQ(ssa0, ssa1, RegisterSize, ElementSize);
  }
  IRPair<IROp_VCMPGT> _VCMPGT(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, OrderedNode *ssa1) {
    return _VCMPGT(ssa0, ssa1, RegisterSize, ElementSize);
  }
  IRPair<IROp_VFAdd> _VFAdd(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, OrderedNode *ssa1) {
    return _VFAdd(ssa0, ssa1, RegisterSize, ElementSize);
  }
  IRPair<IROp_VFAddP> _VFAddP(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, OrderedNode *ssa1) {
    return _VFAddP(ssa0, ssa1, RegisterSize, ElementSize);
  }
  IRPair<IROp_VFCMPEQ> _VFCMPEQ(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, OrderedNode *ssa1) {
    return _VFCMPEQ(ssa0, ssa1, RegisterSize, ElementSize);
  }
  IRPair<IROp_VFCMPNEQ> _VFCMPNEQ(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, OrderedNode *ssa1) {
    return _VFCMPNEQ(ssa0, ssa1, RegisterSize, ElementSize);
  }
  IRPair<IROp_VFCMPLT> _VFCMPLT(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, OrderedNode *ssa1) {
    return _VFCMPLT(ssa0, ssa1, RegisterSize, ElementSize);
  }
  IRPair<IROp_VFCMPGT> _VFCMPGT(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, OrderedNode *ssa1) {
    return _VFCMPGT(ssa0, ssa1, RegisterSize, ElementSize);
  }
  IRPair<IROp_VFCMPLE> _VFCMPLE(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, OrderedNode *ssa1) {
    return _VFCMPLE(ssa0, ssa1, RegisterSize, ElementSize);
  }
  IRPair<IROp_VFCMPUNO> _VFCMPUNO(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, OrderedNode *ssa1) {
    return _VFCMPUNO(ssa0, ssa1, RegisterSize, ElementSize);
  }
  IRPair<IROp_VFCMPORD> _VFCMPORD(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, OrderedNode *ssa1) {
    return _VFCMPORD(ssa0, ssa1, RegisterSize, ElementSize);
  }
  IRPair<IROp_VUShl> _VUShl(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, OrderedNode *ssa1) {
    return _VUShl(ssa0, ssa1, RegisterSize, ElementSize);
  }
  IRPair<IROp_VUShlS> _VUShlS(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, OrderedNode *ssa1) {
    return _VUShlS(ssa0, ssa1, RegisterSize, ElementSize);
  }
  IRPair<IROp_VUShrS> _VUShrS(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, OrderedNode *ssa1) {
    return _VUShrS(ssa0, ssa1, RegisterSize, ElementSize);
  }
  IRPair<IROp_VSShrS> _VSShrS(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, OrderedNode *ssa1) {
    return _VSShrS(ssa0, ssa1, RegisterSize, ElementSize);
  }
  IRPair<IROp_VUShr> _VUShr(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, OrderedNode *ssa1) {
    return _VUShr(ssa0, ssa1, RegisterSize, ElementSize);
  }
  IRPair<IROp_VSShr> _VSShr(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, OrderedNode *ssa1) {
    return _VSShr(ssa0, ssa1, RegisterSize, ElementSize);
  }
  IRPair<IROp_VExtr> _VExtr(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, OrderedNode *ssa1, uint8_t Index) {
    return _VExtr(ssa0, ssa1, Index, RegisterSize, ElementSize);
  }
  IRPair<IROp_VSLI> _VSLI(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, uint8_t ByteShift) {
    return _VSLI(ssa0, ByteShift, RegisterSize, ElementSize);
  }
  IRPair<IROp_VSRI> _VSRI(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, uint8_t ByteShift) {
    return _VSRI(ssa0, ByteShift, RegisterSize, ElementSize);
  }
  IRPair<IROp_VUShrI> _VUShrI(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, uint8_t BitShift) {
    return _VUShrI(ssa0, BitShift, RegisterSize, ElementSize);
  }
  IRPair<IROp_VSShrI> _VSShrI(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, uint8_t BitShift) {
    return _VSShrI(ssa0, BitShift, RegisterSize, ElementSize);
  }
  IRPair<IROp_VUShrNI> _VUShrNI(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, uint8_t BitShift) {
    return _VUShrNI(ssa0, BitShift, RegisterSize, ElementSize);
  }
  IRPair<IROp_VUShrNI2> _VUShrNI2(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, OrderedNode *ssa1, uint8_t BitShift) {
    return _VUShrNI2(ssa0, ssa1, BitShift, RegisterSize, ElementSize);
  }
  IRPair<IROp_VShlI> _VShlI(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, uint8_t BitShift) {
    return _VShlI(ssa0, BitShift, RegisterSize, ElementSize);
  }
  IRPair<IROp_VFSqrt> _VFSqrt(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0) {
    return _VFSqrt(ssa0, RegisterSize, ElementSize);
  }
  IRPair<IROp_VFRSqrt> _VFRSqrt(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0) {
    return _VFRSqrt(ssa0, RegisterSize, ElementSize);
  }
  IRPair<IROp_VNeg> _VNeg(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0) {
    return _VNeg(ssa0, RegisterSize, ElementSize);
  }
  IRPair<IROp_VFNeg> _VFNeg(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0) {
    return _VFNeg(ssa0, RegisterSize, ElementSize);
  }
  IRPair<IROp_VNot> _VNot(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0) {
    return _VNot(ssa0, RegisterSize, ElementSize);
  }
  IRPair<IROp_VSQXTN> _VSQXTN(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0) {
    return _VSQXTN(ssa0, RegisterSize, ElementSize);
  }
  IRPair<IROp_VSQXTN2> _VSQXTN2(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, OrderedNode *ssa1) {
    return _VSQXTN2(ssa0, ssa1, RegisterSize, ElementSize);
  }
  IRPair<IROp_VSQXTUN> _VSQXTUN(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0) {
    return _VSQXTUN(ssa0, RegisterSize, ElementSize);
  }
  IRPair<IROp_VSQXTUN2> _VSQXTUN2(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, OrderedNode *ssa1) {
    return _VSQXTUN2(ssa0, ssa1, RegisterSize, ElementSize);
  }
  IRPair<IROp_VCastFromGPR> _VCastFromGPR(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0) {
    return _VCastFromGPR(ssa0, RegisterSize, ElementSize);
  }
  IRPair<IROp_VExtractToGPR> _VExtractToGPR(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, uint8_t Index) {
    return _VExtractToGPR(ssa0, Index, RegisterSize, ElementSize);
  }
  IRPair<IROp_VInsGPR> _VInsGPR(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, OrderedNode *ssa1, uint8_t Index) {
    return _VInsGPR(ssa0, ssa1, Index, RegisterSize, ElementSize);
  }
  IRPair<IROp_Vector_FToF> _Vector_FToF(uint8_t RegisterSize, uint8_t DstElementSize, uint8_t SrcElementSize, OrderedNode *ssa0) {
    return _Vector_FToF(ssa0, SrcElementSize, RegisterSize, DstElementSize);
  }
  IRPair<IROp_Float_FToF> _Float_FToF(uint8_t DstElementSize, uint8_t SrcElementSize, OrderedNode *ssa0) {
    return _Float_FToF(ssa0, SrcElementSize, DstElementSize);
  }
  IRPair<IROp_VUMul> _VUMul(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, OrderedNode *ssa1) {
    return _VUMul(ssa0, ssa1, RegisterSize, ElementSize);
  }
  IRPair<IROp_VSMul> _VSMul(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, OrderedNode *ssa1) {
    return _VSMul(ssa0, ssa1, RegisterSize, ElementSize);
  }
  IRPair<IROp_VUMull> _VUMull(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, OrderedNode *ssa1) {
    return _VUMull(ssa0, ssa1, RegisterSize, ElementSize);
  }
  IRPair<IROp_VSMull> _VSMull(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, OrderedNode *ssa1) {
    return _VSMull(ssa0, ssa1, RegisterSize, ElementSize);
  }
  IRPair<IROp_VUMull2> _VUMull2(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, OrderedNode *ssa1) {
    return _VUMull2(ssa0, ssa1, RegisterSize, ElementSize);
  }
  IRPair<IROp_VSMull2> _VSMull2(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, OrderedNode *ssa1) {
    return _VSMull2(ssa0, ssa1, RegisterSize, ElementSize);
  }
  IRPair<IROp_VSXTL> _VSXTL(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0) {
    return _VSXTL(ssa0, RegisterSize, ElementSize);
  }
  IRPair<IROp_VSXTL2> _VSXTL2(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0) {
    return _VSXTL2(ssa0, RegisterSize, ElementSize);
  }
  IRPair<IROp_VUXTL> _VUXTL(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0) {
    return _VUXTL(ssa0, RegisterSize, ElementSize);
  }
  IRPair<IROp_VUXTL2> _VUXTL2(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0) {
    return _VUXTL2(ssa0, RegisterSize, ElementSize);
  }
  IRPair<IROp_Jump> _Jump() {
    return _Jump(InvalidNode);
  }
  IRPair<IROp_CondJump> _CondJump(OrderedNode *ssa0) {
    return _CondJump(ssa0, InvalidNode, InvalidNode);
  }

  IRPair<IROp_Phi> _Phi() {
    return _Phi(InvalidNode, InvalidNode, 0);
  }

  IRPair<IROp_PhiValue> _PhiValue(OrderedNode *Value, OrderedNode *Block) {
    return _PhiValue(Value, Block, InvalidNode);
  }

  OrderedNode *Invalid() {
    return InvalidNode;
  }

  void AddPhiValue(IR::IROp_Phi *Phi, OrderedNode *Value) {
    // Got to do some bookkeeping first
    Value->AddUse();
    auto ValueIROp = Value->Op(Data.Begin())->C<IR::IROp_PhiValue>()->Value.GetNode(ListData.Begin())->Op(Data.Begin());
    Phi->Header.Size = ValueIROp->Size;
    Phi->Header.ElementSize = ValueIROp->ElementSize;

    if (!Phi->PhiBegin.ID()) {
      Phi->PhiBegin = Phi->PhiEnd = Value->Wrapped(ListData.Begin());
      return;
    }
    auto PhiValueEndNode = Phi->PhiEnd.GetNode(ListData.Begin());
    auto PhiValueEndOp = PhiValueEndNode->Op(Data.Begin())->CW<IR::IROp_PhiValue>();
    PhiValueEndOp->Next = Value->Wrapped(ListData.Begin());
  }

  void SetJumpTarget(IR::IROp_Jump *Op, OrderedNode *Target) {
    LogMan::Throw::A(Target->Op(Data.Begin())->Op == OP_CODEBLOCK,
        "Tried setting Jump target to %%ssa%d %s",
        Target->Wrapped(ListData.Begin()).ID(),
        std::string(IR::GetName(Target->Op(Data.Begin())->Op)).c_str());

    Op->Header.Args[0].NodeOffset = Target->Wrapped(ListData.Begin()).NodeOffset;
  }
  void SetTrueJumpTarget(IR::IROp_CondJump *Op, OrderedNode *Target) {
    LogMan::Throw::A(Target->Op(Data.Begin())->Op == OP_CODEBLOCK,
        "Tried setting CondJump target to %%ssa%d %s",
        Target->Wrapped(ListData.Begin()).ID(),
        std::string(IR::GetName(Target->Op(Data.Begin())->Op)).c_str());

    Op->Header.Args[1].NodeOffset = Target->Wrapped(ListData.Begin()).NodeOffset;
  }
  void SetFalseJumpTarget(IR::IROp_CondJump *Op, OrderedNode *Target) {
    LogMan::Throw::A(Target->Op(Data.Begin())->Op == OP_CODEBLOCK,
        "Tried setting CondJump target to %%ssa%d %s",
        Target->Wrapped(ListData.Begin()).ID(),
        std::string(IR::GetName(Target->Op(Data.Begin())->Op)).c_str());

    Op->Header.Args[2].NodeOffset = Target->Wrapped(ListData.Begin()).NodeOffset;
  }

  void SetJumpTarget(IRPair<IROp_Jump> Op, OrderedNode *Target) {
    LogMan::Throw::A(Target->Op(Data.Begin())->Op == OP_CODEBLOCK,
        "Tried setting Jump target to %%ssa%d %s",
        Target->Wrapped(ListData.Begin()).ID(),
        std::string(IR::GetName(Target->Op(Data.Begin())->Op)).c_str());

    Op.first->Header.Args[0].NodeOffset = Target->Wrapped(ListData.Begin()).NodeOffset;
  }
  void SetTrueJumpTarget(IRPair<IROp_CondJump> Op, OrderedNode *Target) {
    LogMan::Throw::A(Target->Op(Data.Begin())->Op == OP_CODEBLOCK,
        "Tried setting CondJump target to %%ssa%d %s",
        Target->Wrapped(ListData.Begin()).ID(),
        std::string(IR::GetName(Target->Op(Data.Begin())->Op)).c_str());
    Op.first->Header.Args[1].NodeOffset = Target->Wrapped(ListData.Begin()).NodeOffset;
  }
  void SetFalseJumpTarget(IRPair<IROp_CondJump> Op, OrderedNode *Target) {
    LogMan::Throw::A(Target->Op(Data.Begin())->Op == OP_CODEBLOCK,
        "Tried setting CondJump target to %%ssa%d %s",
        Target->Wrapped(ListData.Begin()).ID(),
        std::string(IR::GetName(Target->Op(Data.Begin())->Op)).c_str());
    Op.first->Header.Args[2].NodeOffset = Target->Wrapped(ListData.Begin()).NodeOffset;
  }

  /**  @} */

  bool IsValueConstant(OrderedNodeWrapper ssa, uint64_t *Constant) {
     OrderedNode *RealNode = ssa.GetNode(ListData.Begin());
     FEXCore::IR::IROp_Header *IROp = RealNode->Op(Data.Begin());
     if (IROp->Op == OP_CONSTANT) {
       auto Op = IROp->C<IR::IROp_Constant>();
       *Constant = Op->Constant;
       return true;
     }
     return false;
  }

  // This is fairly special in how it operates
  // Since the node is decoupled from the backing op then we can swap out the backing op without much overhead
  // This can potentially cause problems where multiple nodes are pointing to the same IROp
  OrderedNode *ReplaceAllUsesWith(OrderedNode *Node, IROp_Header *Op) {
    RemoveArgUses(Node);
    Node->Header.Value.SetOffset(Data.Begin(), reinterpret_cast<uintptr_t>(Op));
    return Node;
  }

  // This is similar to the previous op except that we pass in a node
  // This takes the op backing in the new node and replaces the node in the other node
  // Again can cause problems where things are pointing to NewNode and haven't been decoupled
  OrderedNode *ReplaceAllUsesWith(OrderedNode *Node, OrderedNode *NewNode) {
    RemoveArgUses(Node);
    Node->Header.Value.NodeOffset = NewNode->Header.Value.NodeOffset;
    return Node;
  }

  void ReplaceAllUsesWithInclusive(OrderedNode *Node, OrderedNode *NewNode, IR::NodeWrapperIterator After, IR::NodeWrapperIterator End);
  void ReplaceNodeArgument(OrderedNode *Node, uint8_t Arg, OrderedNode *NewArg);

  void Remove(OrderedNode *Node);

  void SetPackedRFLAG(bool Lower8, OrderedNode *Src);
  OrderedNode *GetPackedRFLAG(bool Lower8);

  void CopyData(IREmitter const &rhs) {
    LogMan::Throw::A(rhs.Data.BackingSize() <= Data.BackingSize(), "Trying to take ownership of data that is too large");
    LogMan::Throw::A(rhs.ListData.BackingSize() <= ListData.BackingSize(), "Trying to take ownership of data that is too large");
    Data.CopyData(rhs.Data);
    ListData.CopyData(rhs.ListData);
    InvalidNode = rhs.InvalidNode;
    CurrentWriteCursor = rhs.CurrentWriteCursor;
    CodeBlocks = rhs.CodeBlocks;
  }

  void SetWriteCursor(OrderedNode *Node) {
    CurrentWriteCursor = Node;
  }

  OrderedNode *GetWriteCursor() {
    return CurrentWriteCursor;
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
    auto CodeNode = _CodeBlock(InvalidNode, InvalidNode, InvalidNode);
    CodeBlocks.emplace_back(CodeNode);
    return CodeNode;
  }

  void SetCodeNodeBegin(OrderedNode *CodeNode, OrderedNode *Begin) {
     FEXCore::IR::IROp_CodeBlock *IROp = CodeNode->Op(Data.Begin())->CW<FEXCore::IR::IROp_CodeBlock>();
     LogMan::Throw::A(IROp->Header.Op == IROps::OP_CODEBLOCK, "Invalid");
     IROp->Begin = Begin->Wrapped(ListData.Begin());
  }

  void SetCodeNodeLast(OrderedNode *CodeNode, OrderedNode *Last) {
     FEXCore::IR::IROp_CodeBlock *IROp = CodeNode->Op(Data.Begin())->CW<FEXCore::IR::IROp_CodeBlock>();
     LogMan::Throw::A(IROp->Header.Op == IROps::OP_CODEBLOCK, "Invalid");
     IROp->Last = Last->Wrapped(ListData.Begin());
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
     FEXCore::IR::IROp_CodeBlock *CurrentIROp = CodeNode->Op(Data.Begin())->CW<FEXCore::IR::IROp_CodeBlock>();
     LogMan::Throw::A(CurrentIROp->Header.Op == IROps::OP_CODEBLOCK, "Invalid");

     OrderedNodeWrapper OldNext = CurrentIROp->Next;
     // First thing is to assign CodeNode->Next to the incoming node
     {
       CurrentIROp->Next = Next->Wrapped(ListData.Begin());
     }

     // Second thing is to assign the incoming node's Next to what was in CodeNode->Next
     {
       FEXCore::IR::IROp_CodeBlock *NextIROp = Next->Op(Data.Begin())->CW<FEXCore::IR::IROp_CodeBlock>();
       auto NewOldNext = NextIROp->Next;
       NextIROp->Next = OldNext;
       OldNext = NewOldNext;
     }
  }

  IRPair<IROp_CodeBlock> CreateNewCodeBlock();
  void SetCurrentCodeBlock(OrderedNode *Node);

  protected:
    void RemoveArgUses(OrderedNode *Node);

    OrderedNode *CreateNode(IROp_Header *Op) {
      uintptr_t ListBegin = ListData.Begin();
      size_t Size = sizeof(OrderedNode);
      void *Ptr = ListData.Allocate(Size);
      OrderedNode *Node = new (Ptr) OrderedNode();
      Node->Header.Value.SetOffset(Data.Begin(), reinterpret_cast<uintptr_t>(Op));

      if (CurrentWriteCursor) {
        CurrentWriteCursor->append(ListBegin, Node);
      }
      CurrentWriteCursor = Node;
      return Node;
    }

    OrderedNode *GetNode(uint32_t SSANode) {
      uintptr_t ListBegin = ListData.Begin();
      OrderedNode *Node = reinterpret_cast<OrderedNode *>(ListBegin + SSANode * sizeof(OrderedNode));
      return Node;
    }

    OrderedNode *EmplaceOrphanedNode(OrderedNode *OldNode) {
      size_t Size = sizeof(OrderedNode);
      OrderedNode *Ptr = reinterpret_cast<OrderedNode*>(ListData.Allocate(Size));
      memcpy(Ptr, OldNode, Size);
      return Ptr;
    }

    OrderedNode *CurrentWriteCursor = nullptr;

    // These could be combined with a little bit of work to be more efficient with memory usage. Isn't a big deal
    IntrusiveAllocator Data;
    IntrusiveAllocator ListData;

    OrderedNode *InvalidNode;
    OrderedNode *CurrentCodeBlock{};
    std::vector<OrderedNode*> CodeBlocks;
    uint64_t Entry;
};

}
