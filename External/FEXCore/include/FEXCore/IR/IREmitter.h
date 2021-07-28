#pragma once
#include <FEXCore/IR/IntrusiveIRList.h>
#include <FEXCore/IR/IR.h>

#include <FEXCore/Utils/LogManager.h>

#include <algorithm>

namespace FEXCore::IR {
class Pass;
class PassManager;

class IREmitter {
friend class FEXCore::IR::Pass;
friend class FEXCore::IR::PassManager;

  public:
    IREmitter()
      : DualListData {8 * 1024 * 1024} {
      ResetWorkingList();
    }

    IRListView ViewIR() { return IRListView(&DualListData, false); }
    IRListView *CreateIRCopy() { return new IRListView(&DualListData, true); }
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
    uint64_t Mask = ~0ULL >> (Size - 64);
    Op.first->Constant = (Constant & Mask);
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
    Op.first->Header.Args[0] = ssa0->Wrapped(DualListData.ListBegin());
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
    return _Bfe(ssa0, Width, lsb, 0);
  }
  IRPair<IROp_Bfe> _Bfe(uint8_t DestSize, int8_t Width, uint8_t lsb, OrderedNode *ssa0) {
    return _Bfe(ssa0, Width, lsb, DestSize);
  }
  IRPair<IROp_Sbfe> _Sbfe(uint8_t Width, uint8_t lsb, OrderedNode *ssa0) {
    return _Sbfe(ssa0, Width, lsb);
  }
  IRPair<IROp_Bfi> _Bfi(uint8_t DestSize, uint8_t Width, uint8_t lsb, OrderedNode *ssa0, OrderedNode *ssa1) {
    return _Bfi(ssa0, ssa1, Width, lsb, DestSize);
  }
  IRPair<IROp_StoreMem> _StoreMem(FEXCore::IR::RegisterClassType Class, uint8_t Size, OrderedNode *ssa0, OrderedNode *ssa1, uint8_t Align = 1) {
    return _StoreMem(ssa0, ssa1, Invalid(), Size, Align, Class, MEM_OFFSET_SXTX, 1);
  }
  IRPair<IROp_StoreMemTSO> _StoreMemTSO(FEXCore::IR::RegisterClassType Class, uint8_t Size, OrderedNode *ssa0, OrderedNode *ssa1, uint8_t Align = 1) {
    return _StoreMemTSO(ssa0, ssa1, Invalid(), Size, Align, Class, MEM_OFFSET_SXTX, 1);
  }
  IRPair<IROp_VStoreMemElement> _VStoreMemElement(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, OrderedNode *ssa1, uint8_t Index, uint8_t Align = 1) {
    return _VStoreMemElement(ssa0, ssa1, Index, Align, RegisterSize, ElementSize);
  }
  IRPair<IROp_LoadMem> _LoadMem(FEXCore::IR::RegisterClassType Class, uint8_t Size, OrderedNode *ssa0, uint8_t Align = 1) {
    return _LoadMem(ssa0, Invalid(), Size, Align, Class, MEM_OFFSET_SXTX, 1);
  }
  IRPair<IROp_LoadMemTSO> _LoadMemTSO(FEXCore::IR::RegisterClassType Class, uint8_t Size, OrderedNode *ssa0, uint8_t Align = 1) {
    return _LoadMemTSO(ssa0, Invalid(), Size, Align, Class, MEM_OFFSET_SXTX, 1);
  }
  IRPair<IROp_VLoadMemElement> _VLoadMemElement(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, OrderedNode *ssa1, uint8_t Index, uint8_t Align = 1) {
    return _VLoadMemElement(ssa0, ssa1, Index, Align, RegisterSize, ElementSize);
  }
  IRPair<IROp_Select> _Select(uint8_t Cond, OrderedNode *ssa0, OrderedNode *ssa1, OrderedNode *ssa2, OrderedNode *ssa3, uint8_t CompareSize = 0) {
    if (CompareSize == 0)
      CompareSize = std::max<uint8_t>(4, std::max<uint8_t>(GetOpSize(ssa0), GetOpSize(ssa1)));

    return _Select(ssa0, ssa1, ssa2, ssa3, {Cond}, CompareSize);
  }
  IRPair<IROp_Sbfe> _Sext(uint8_t SrcSize, OrderedNode *ssa0) {
    return _Sbfe(SrcSize, 0, ssa0);
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
  IRPair<IROp_VDupElement> _VDupElement(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, uint8_t Index) {
    return _VDupElement(ssa0, Index, RegisterSize, ElementSize);
  }
  IRPair<IROp_VAnd> _VAnd(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, OrderedNode *ssa1) {
    return _VAnd(ssa0, ssa1, RegisterSize, ElementSize);
  }
  IRPair<IROp_VBic> _VBic(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, OrderedNode *ssa1) {
    return _VBic(ssa0, ssa1, RegisterSize, ElementSize);
  }
  IRPair<IROp_VOr> _VOr(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, OrderedNode *ssa1) {
    return _VOr(ssa0, ssa1, RegisterSize, ElementSize);
  }
  IRPair<IROp_VXor> _VXor(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, OrderedNode *ssa1) {
    return _VXor(ssa0, ssa1, RegisterSize, ElementSize);
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
  IRPair<IROp_VAddV> _VAddV(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0) {
    return _VAddV(ssa0, RegisterSize, ElementSize);
  }
  IRPair<IROp_VUMinV> _VUMinV(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0) {
    return _VUMinV(ssa0, RegisterSize, ElementSize);
  }
  IRPair<IROp_VURAvg> _VURAvg(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, OrderedNode *ssa1) {
    return _VURAvg(ssa0, ssa1, RegisterSize, ElementSize);
  }
  IRPair<IROp_VAbs> _VAbs(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0) {
    return _VAbs(ssa0, RegisterSize, ElementSize);
  }
  IRPair<IROp_VPopcount> _VPopcount(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0) {
    return _VPopcount(ssa0, RegisterSize, ElementSize);
  }
  IRPair<IROp_VFMul> _VFMul(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, OrderedNode *ssa1) {
    return _VFMul(ssa0, ssa1, RegisterSize, ElementSize);
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
  IRPair<IROp_VUnZip> _VUnZip(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, OrderedNode *ssa1) {
    return _VUnZip(ssa0, ssa1, RegisterSize, ElementSize);
  }
  IRPair<IROp_VUnZip2> _VUnZip2(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, OrderedNode *ssa1) {
    return _VUnZip2(ssa0, ssa1, RegisterSize, ElementSize);
  }
  IRPair<IROp_VCMPEQ> _VCMPEQ(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, OrderedNode *ssa1) {
    return _VCMPEQ(ssa0, ssa1, RegisterSize, ElementSize);
  }
  IRPair<IROp_VCMPEQZ> _VCMPEQZ(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0) {
    return _VCMPEQZ(ssa0, RegisterSize, ElementSize);
  }
  IRPair<IROp_VCMPGT> _VCMPGT(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, OrderedNode *ssa1) {
    return _VCMPGT(ssa0, ssa1, RegisterSize, ElementSize);
  }
  IRPair<IROp_VCMPGTZ> _VCMPGTZ(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0) {
    return _VCMPGTZ(ssa0, RegisterSize, ElementSize);
  }
  IRPair<IROp_VCMPLTZ> _VCMPLTZ(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0) {
    return _VCMPLTZ(ssa0, RegisterSize, ElementSize);
  }
  IRPair<IROp_VFAdd> _VFAdd(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, OrderedNode *ssa1) {
    return _VFAdd(ssa0, ssa1, RegisterSize, ElementSize);
  }
  IRPair<IROp_VFAddP> _VFAddP(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, OrderedNode *ssa1) {
    return _VFAddP(ssa0, ssa1, RegisterSize, ElementSize);
  }
  IRPair<IROp_VFSub> _VFSub(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, OrderedNode *ssa1) {
    return _VFSub(ssa0, ssa1, RegisterSize, ElementSize);
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
  IRPair<IROp_Float_FromGPR_S> _Float_FromGPR_S(uint8_t DstElementSize, uint8_t SrcElementSize, OrderedNode *ssa0) {
    return _Float_FromGPR_S(ssa0, SrcElementSize, DstElementSize);
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
  IRPair<IROp_VUABDL> _VUABDL(uint8_t RegisterSize, uint8_t ElementSize, OrderedNode *ssa0, OrderedNode *ssa1) {
    return _VUABDL(ssa0, ssa1, RegisterSize, ElementSize);
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
  IRPair<IROp_VTBL1> _VTBL1(uint8_t RegisterSize, OrderedNode *ssa0, OrderedNode *ssa1) {
    return _VTBL1(ssa0, ssa1, RegisterSize);
  }
  IRPair<IROp_Jump> _Jump() {
    return _Jump(InvalidNode);
  }

  IRPair<IROp_CondJump> _CondJump(OrderedNode *ssa0, CondClassType cond = {COND_NEQ}) {
    return _CondJump(ssa0, ConstructConst(0), InvalidNode, InvalidNode, cond, GetOpSize(ssa0));
  }

  IRPair<IROp_CondJump> _CondJump(OrderedNode *ssa0, OrderedNode *ssa1, OrderedNode *ssa2, CondClassType cond = {COND_NEQ}) {
    return _CondJump(ssa0, ConstructConst(0), ssa1, ssa2, cond, GetOpSize(ssa0));
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
    auto ValueIROp = Value->Op(DualListData.DataBegin())->C<IR::IROp_PhiValue>()->Value.GetNode(DualListData.ListBegin())->Op(DualListData.DataBegin());
    Phi->Header.Size = ValueIROp->Size;
    Phi->Header.ElementSize = ValueIROp->ElementSize;

    if (!Phi->PhiBegin.ID()) {
      Phi->PhiBegin = Phi->PhiEnd = Value->Wrapped(DualListData.ListBegin());
      return;
    }
    auto PhiValueEndNode = Phi->PhiEnd.GetNode(DualListData.ListBegin());
    auto PhiValueEndOp = PhiValueEndNode->Op(DualListData.DataBegin())->CW<IR::IROp_PhiValue>();
    PhiValueEndOp->Next = Value->Wrapped(DualListData.ListBegin());
  }

  void SetJumpTarget(IR::IROp_Jump *Op, OrderedNode *Target) {
    LOGMAN_THROW_A_FMT(Target->Op(DualListData.DataBegin())->Op == OP_CODEBLOCK,
        "Tried setting Jump target to %ssa{} {}",
        Target->Wrapped(DualListData.ListBegin()).ID(),
        IR::GetName(Target->Op(DualListData.DataBegin())->Op));

    Op->Header.Args[0].NodeOffset = Target->Wrapped(DualListData.ListBegin()).NodeOffset;
  }
  void SetTrueJumpTarget(IR::IROp_CondJump *Op, OrderedNode *Target) {
    LOGMAN_THROW_A_FMT(Target->Op(DualListData.DataBegin())->Op == OP_CODEBLOCK,
        "Tried setting CondJump target to %ssa{} {}",
        Target->Wrapped(DualListData.ListBegin()).ID(),
        IR::GetName(Target->Op(DualListData.DataBegin())->Op));

    Op->TrueBlock.NodeOffset = Target->Wrapped(DualListData.ListBegin()).NodeOffset;
  }
  void SetFalseJumpTarget(IR::IROp_CondJump *Op, OrderedNode *Target) {
    LOGMAN_THROW_A_FMT(Target->Op(DualListData.DataBegin())->Op == OP_CODEBLOCK,
        "Tried setting CondJump target to %ssa{} {}",
        Target->Wrapped(DualListData.ListBegin()).ID(),
        IR::GetName(Target->Op(DualListData.DataBegin())->Op));

    Op->FalseBlock.NodeOffset = Target->Wrapped(DualListData.ListBegin()).NodeOffset;
  }

  void SetJumpTarget(IRPair<IROp_Jump> Op, OrderedNode *Target) {
    LOGMAN_THROW_A_FMT(Target->Op(DualListData.DataBegin())->Op == OP_CODEBLOCK,
        "Tried setting Jump target to %ssa{} {}",
        Target->Wrapped(DualListData.ListBegin()).ID(),
        IR::GetName(Target->Op(DualListData.DataBegin())->Op));

    Op.first->Header.Args[0].NodeOffset = Target->Wrapped(DualListData.ListBegin()).NodeOffset;
  }
  void SetTrueJumpTarget(IRPair<IROp_CondJump> Op, OrderedNode *Target) {
    LOGMAN_THROW_A_FMT(Target->Op(DualListData.DataBegin())->Op == OP_CODEBLOCK,
        "Tried setting CondJump target to %ssa{} {}",
        Target->Wrapped(DualListData.ListBegin()).ID(),
        IR::GetName(Target->Op(DualListData.DataBegin())->Op));
    Op.first->TrueBlock.NodeOffset = Target->Wrapped(DualListData.ListBegin()).NodeOffset;
  }
  void SetFalseJumpTarget(IRPair<IROp_CondJump> Op, OrderedNode *Target) {
    LOGMAN_THROW_A_FMT(Target->Op(DualListData.DataBegin())->Op == OP_CODEBLOCK,
        "Tried setting CondJump target to %ssa{} {}",
        Target->Wrapped(DualListData.ListBegin()).ID(),
        IR::GetName(Target->Op(DualListData.DataBegin())->Op));
    Op.first->FalseBlock.NodeOffset = Target->Wrapped(DualListData.ListBegin()).NodeOffset;
  }

  /**  @} */

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

  void ReplaceAllUsesWithRange(OrderedNode *Node, OrderedNode *NewNode, AllNodesIterator After, AllNodesIterator End);

  void ReplaceUsesWithAfter(OrderedNode *Node, OrderedNode *NewNode, AllNodesIterator After) {
    ReplaceAllUsesWithRange(Node, NewNode, After, AllNodesIterator(DualListData.ListBegin(), DualListData.DataBegin()));
  }

  void ReplaceUsesWithAfter(OrderedNode *Node, OrderedNode *NewNode, OrderedNode *After) {
    auto Wrapped = Node->Wrapped(DualListData.ListBegin());
    AllNodesIterator It = AllNodesIterator(DualListData.ListBegin(), DualListData.DataBegin(), Wrapped);

    ReplaceUsesWithAfter(Node, NewNode, It);
  }

  void ReplaceAllUsesWith(OrderedNode *Node, OrderedNode *NewNode) {
    auto Start = AllNodesIterator(DualListData.ListBegin(), DualListData.DataBegin(), Node->Wrapped(DualListData.ListBegin()));

    ReplaceUsesWithAfter(Node, NewNode, Start);

    LOGMAN_THROW_A_FMT(Node->NumUses == 0, "Node still used");

    // Since we have deleted ALL uses, we can safely delete the node.
    Remove(Node);
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

  void SetCurrentCodeBlock(OrderedNode *Node) {
    CurrentCodeBlock = Node;
    LOGMAN_THROW_A_FMT(Node->Op(DualListData.DataBegin())->Op == OP_CODEBLOCK, "Node wasn't codeblock. It was '{}'", IR::GetName(Node->Op(DualListData.DataBegin())->Op));
    SetWriteCursor(Node->Op(DualListData.DataBegin())->CW<IROp_CodeBlock>()->Begin.GetNode(DualListData.ListBegin()));

    // Clear our const pool which is per block
    ClearConstPool();
  }

  OrderedNode* ConstructConst(uint8_t Size, uint64_t Constant) {
    auto Pair = std::make_pair(Constant, Size);
    auto Iter = ConstPool.find(Pair);
    if (Iter == ConstPool.end()) {
      auto NewIter = ConstPool.try_emplace(Pair, _Constant(Size, Constant));
      return NewIter.first->second;
    }

    return Iter->second;
  }

  OrderedNode* ConstructConst(uint64_t Constant) {
    auto Pair = std::make_pair(Constant, 8);
    auto Iter = ConstPool.find(Pair);
    if (Iter == ConstPool.end()) {
      auto NewIter = ConstPool.try_emplace(Pair, _Constant(Constant));
      return NewIter.first->second;
    }

    return Iter->second;
  }

  void ClearConstPool() {
    ConstPool.clear();
  }

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
    DualIntrusiveAllocator DualListData;

    OrderedNode *InvalidNode;
    OrderedNode *CurrentCodeBlock{};
    std::vector<OrderedNode*> CodeBlocks;
    uint64_t Entry;

    // Per block const pool
    // Will usually end up being only a couple members in size
    struct PairHash {
      size_t operator() (const std::pair<uint64_t, uint8_t> &Pair) const {
        return std::hash<uint64_t>()(Pair.first) ^ std::hash<uint8_t>()(Pair.second);
      }
    };
    std::unordered_map<std::pair<uint64_t, uint8_t>, OrderedNode*, PairHash> ConstPool;
};

}
