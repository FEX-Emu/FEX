// SPDX-License-Identifier: MIT
#pragma once

#include "Interface/Core/Frontend.h"
#include "Interface/Core/X86Tables/X86Tables.h"
#include "Interface/Context/Context.h"

#include <FEXCore/Config/Config.h>
#include <FEXCore/Core/Context.h>
#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/IR/IntrusiveIRList.h>
#include <FEXCore/IR/IR.h>
#include <FEXCore/IR/IREmitter.h>

#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/MathUtils.h>
#include <FEXCore/fextl/map.h>
#include <FEXCore/fextl/vector.h>

#include <cstdint>
#include <fmt/format.h>
#include <stddef.h>
#include <utility>
#include <xxhash.h>

namespace FEXCore::IR {
class Pass;
class PassManager;

enum class MemoryAccessType {
  // Choose TSO or Non-TSO depending on access type
  DEFAULT,
  // TSO access behaviour
  TSO,
  // Non-TSO access behaviour
  NONTSO,
  // Non-temporal streaming
  STREAM,
};

struct LoadSourceOptions {
  // Alignment of the load in bytes. -1 signifies unaligned
  int8_t Align = -1;

  // Whether or not to load the data if a memory access occurs.
  // If set to false, then the address that would have been loaded from
  // will be returned instead.
  //
  // Note: If returning the address, make sure to apply the segment offset
  //       after with AppendSegmentOffset().
  //
  bool LoadData = true;

  // Use to force a load even if the underlying type isn't loadable.
  bool ForceLoad = false;

  // Specifies the access type of the load.
  MemoryAccessType AccessType = MemoryAccessType::DEFAULT;

  // Whether or not a zero extend should clear the upper bits
  // in the register (e.g. an 8-bit load would clear the upper 24 bits
  // or 56 bits depending on the operating mode).
  // If true, no zero-extension occurs.
  bool AllowUpperGarbage = false;
};

class OpDispatchBuilder final : public IREmitter {
friend class FEXCore::IR::Pass;
friend class FEXCore::IR::PassManager;

public:
  enum class FlagsGenerationType : uint8_t {
    TYPE_NONE,
    TYPE_ADC,
    TYPE_SBB,
    TYPE_SUB,
    TYPE_ADD,
    TYPE_MUL,
    TYPE_UMUL,
    TYPE_LOGICAL,
    TYPE_LSHL,
    TYPE_LSHLI,
    TYPE_LSHR,
    TYPE_LSHRI,
    TYPE_LSHRDI,
    TYPE_ASHR,
    TYPE_ASHRI,
    TYPE_ROR,
    TYPE_RORI,
    TYPE_ROL,
    TYPE_ROLI,
    TYPE_FCMP,
    TYPE_BEXTR,
    TYPE_BLSI,
    TYPE_BLSMSK,
    TYPE_BLSR,
    TYPE_POPCOUNT,
    TYPE_BZHI,
    TYPE_TZCNT,
    TYPE_LZCNT,
    TYPE_BITSELECT,
    TYPE_RDRAND,
  };

  OrderedNode* GetNewJumpBlock(uint64_t RIP) {
    auto it = JumpTargets.find(RIP);
    LOGMAN_THROW_A_FMT(it != JumpTargets.end(), "Couldn't find block generated for 0x{:x}", RIP);
    return it->second.BlockEntry;
  }

  void SetNewBlockIfChanged(uint64_t RIP) {
    auto it = JumpTargets.find(RIP);
    if (it == JumpTargets.end()) return;

    it->second.HaveEmitted = true;

    if (CurrentCodeBlock->Wrapped(DualListData.ListBegin()).ID() == it->second.BlockEntry->Wrapped(DualListData.ListBegin()).ID()) return;

    // We have hit a RIP that is a jump target
    // Thus we need to end up in a new block
    SetCurrentCodeBlock(it->second.BlockEntry);
  }

  void StartNewBlock() {
    flagsOp = SelectionFlag::Nothing;

    // If we loaded flags but didn't change them, invalidate the cached copy and move on.
    // Changes get stored out by CalculateDeferredFlags.
    CachedNZCV = nullptr;

    // New block needs to reset segment telemetry.
    SegmentsNeedReadCheck = ~0U;

    // Need to clear any named constants that were cached.
    ClearCachedNamedConstants();
  }

  IRPair<IROp_Jump> Jump() {
    CalculateDeferredFlags();
    return _Jump();
  }
  IRPair<IROp_Jump> Jump(OrderedNode *_TargetBlock) {
    CalculateDeferredFlags();
    return _Jump(_TargetBlock);
  }
  IRPair<IROp_CondJump> CondJump(OrderedNode *_Cmp1, OrderedNode *_Cmp2, OrderedNode *_TrueBlock, OrderedNode *_FalseBlock, CondClassType _Cond = {COND_NEQ}, uint8_t _CompareSize = 0) {
    CalculateDeferredFlags();
    return _CondJump(_Cmp1, _Cmp2, _TrueBlock, _FalseBlock, _Cond, _CompareSize);
  }
  IRPair<IROp_CondJump> CondJump(OrderedNode *ssa0, CondClassType cond = {COND_NEQ}) {
    CalculateDeferredFlags();
    return _CondJump(ssa0, cond);
  }
  IRPair<IROp_CondJump> CondJump(OrderedNode *ssa0, OrderedNode *ssa1, OrderedNode *ssa2, CondClassType cond = {COND_NEQ}) {
    CalculateDeferredFlags();
    return _CondJump(ssa0, ssa1, ssa2, cond);
  }

  bool FinishOp(uint64_t NextRIP, bool LastOp) {
    // If we are switching to a new block and this current block has yet to set a RIP
    // Then we need to insert an unconditional jump from the current block to the one we are going to
    // This happens most frequently when an instruction jumps backwards to another location
    // eg:
    //
    //  nop dword [rax], eax
    // .label:
    //  rdi, 0x8
    //  cmp qword [rdi-8], 0
    //  jne .label
    if (LastOp && !BlockSetRIP) {
      // Calculate flags first
      CalculateDeferredFlags();

      auto it = JumpTargets.find(NextRIP);
      if (it == JumpTargets.end()) {

        const uint8_t GPRSize = CTX->GetGPRSize();
        // If we don't have a jump target to a new block then we have to leave
        // Set the RIP to the next instruction and leave
        auto RelocatedNextRIP = _EntrypointOffset(IR::SizeToOpSize(GPRSize), NextRIP - Entry);
        _ExitFunction(RelocatedNextRIP);
      }
      else if (it != JumpTargets.end()) {
        Jump(it->second.BlockEntry);
        return true;
      }
    }

    if (LastOp) {
      LOGMAN_THROW_A_FMT(IsDeferredFlagsStored(), "FinishOp: Deferred flags weren't generated at end of block");
    }

    BlockSetRIP = false;

    return false;
  }

  static bool CanHaveSideEffects(FEXCore::X86Tables::X86InstInfo const* TableInfo, FEXCore::X86Tables::DecodedOp Op) {
    if (TableInfo && TableInfo->Flags & X86Tables::InstFlags::FLAGS_DEBUG_MEM_ACCESS) {
      // If it is marked as having memory access then always say it has a side-effect.
      // Not always true but better to be safe.
      return true;
    }

    auto CanHaveSideEffects = false;

    auto HasPotentialMemoryAccess = [](X86Tables::DecodedOperand const &Operand) -> bool {
      if (Operand.IsNone()) {
        return false;
      }

      // This isn't guaranteed that all of these types will access memory, but be safe.
      return Operand.IsGPRDirect() || Operand.IsGPRIndirect() || Operand.IsRIPRelative() || Operand.IsSIB();
    };

    CanHaveSideEffects |= HasPotentialMemoryAccess(Op->Dest);
    CanHaveSideEffects |= HasPotentialMemoryAccess(Op->Src[0]);
    CanHaveSideEffects |= HasPotentialMemoryAccess(Op->Src[1]);
    CanHaveSideEffects |= HasPotentialMemoryAccess(Op->Src[2]);
    return CanHaveSideEffects;
  }

  OpDispatchBuilder(FEXCore::Context::ContextImpl *ctx);
  OpDispatchBuilder(FEXCore::Utils::IntrusivePooledAllocator &Allocator);

  void ResetWorkingList();
  void ResetDecodeFailure() { NeedsBlockEnd = DecodeFailure = false; }
  bool HadDecodeFailure() const { return DecodeFailure; }
  bool NeedsBlockEnder() const { return NeedsBlockEnd; }

  void ResetHandledLock() { HandledLock = false; }
  bool HasHandledLock() const { return HandledLock; }

  void SetDumpIR(bool DumpIR) { ShouldDump = DumpIR; }
  bool ShouldDumpIR() const { return ShouldDump; }

  void BeginFunction(uint64_t RIP, fextl::vector<FEXCore::Frontend::Decoder::DecodedBlocks> const *Blocks, uint32_t NumInstructions);
  void Finalize();

  // Dispatch builder functions
#define OpcodeArgs [[maybe_unused]] FEXCore::X86Tables::DecodedOp Op
  void UnhandledOp(OpcodeArgs);
  template<uint32_t SrcIndex>
  void MOVGPROp(OpcodeArgs);
  void MOVGPRNTOp(OpcodeArgs);
  void MOVVectorAlignedOp(OpcodeArgs);
  void MOVVectorUnalignedOp(OpcodeArgs);
  void MOVVectorNTOp(OpcodeArgs);
  template<FEXCore::IR::IROps ALUIROp, FEXCore::IR::IROps AtomicFetchOp>
  void ALUOp(OpcodeArgs);
  void INTOp(OpcodeArgs);
  void SyscallOp(OpcodeArgs);
  void ThunkOp(OpcodeArgs);
  void LEAOp(OpcodeArgs);
  void NOPOp(OpcodeArgs);
  void RETOp(OpcodeArgs);
  void IRETOp(OpcodeArgs);
  void CallbackReturnOp(OpcodeArgs);
  void SecondaryALUOp(OpcodeArgs);
  template<uint32_t SrcIndex>
  void ADCOp(OpcodeArgs);
  template<uint32_t SrcIndex, bool SetFlags>
  void SBBOp(OpcodeArgs);
  void PUSHOp(OpcodeArgs);
  void PUSHREGOp(OpcodeArgs);
  void PUSHAOp(OpcodeArgs);
  template<uint32_t SegmentReg>
  void PUSHSegmentOp(OpcodeArgs);
  void POPOp(OpcodeArgs);
  void POPAOp(OpcodeArgs);
  template<uint32_t SegmentReg>
  void POPSegmentOp(OpcodeArgs);
  void LEAVEOp(OpcodeArgs);
  void CALLOp(OpcodeArgs);
  void CALLAbsoluteOp(OpcodeArgs);
  void CondJUMPOp(OpcodeArgs);
  void CondJUMPRCXOp(OpcodeArgs);
  void LoopOp(OpcodeArgs);
  void JUMPOp(OpcodeArgs);
  void JUMPAbsoluteOp(OpcodeArgs);
  template<uint32_t SrcIndex>
  void TESTOp(OpcodeArgs);
  void MOVSXDOp(OpcodeArgs);
  void MOVSXOp(OpcodeArgs);
  void MOVZXOp(OpcodeArgs);
  template<uint32_t SrcIndex>
  void CMPOp(OpcodeArgs);
  void SETccOp(OpcodeArgs);
  void CQOOp(OpcodeArgs);
  void CDQOp(OpcodeArgs);
  void XCHGOp(OpcodeArgs);
  void SAHFOp(OpcodeArgs);
  void LAHFOp(OpcodeArgs);
  template<bool ToSeg>
  void MOVSegOp(OpcodeArgs);
  void FLAGControlOp(OpcodeArgs);
  void MOVOffsetOp(OpcodeArgs);
  void CMOVOp(OpcodeArgs);
  void CPUIDOp(OpcodeArgs);
  void XGetBVOp(OpcodeArgs);
  template<bool SHL1Bit>
  void SHLOp(OpcodeArgs);
  void SHLImmediateOp(OpcodeArgs);
  template<bool SHR1Bit>
  void SHROp(OpcodeArgs);
  void SHRImmediateOp(OpcodeArgs);
  void SHLDOp(OpcodeArgs);
  void SHLDImmediateOp(OpcodeArgs);
  void SHRDOp(OpcodeArgs);
  void SHRDImmediateOp(OpcodeArgs);
  template<bool SHR1Bit>
  void ASHROp(OpcodeArgs);
  void ASHRImmediateOp(OpcodeArgs);
  template<bool Is1Bit>
  void ROROp(OpcodeArgs);
  void RORImmediateOp(OpcodeArgs);
  template<bool Is1Bit>
  void ROLOp(OpcodeArgs);
  void ROLImmediateOp(OpcodeArgs);
  void RCROp1Bit(OpcodeArgs);
  void RCROp8x1Bit(OpcodeArgs);
  void RCROp(OpcodeArgs);
  void RCRSmallerOp(OpcodeArgs);
  void RCLOp1Bit(OpcodeArgs);
  void RCLOp(OpcodeArgs);
  void RCLSmallerOp(OpcodeArgs);
  template<uint32_t SrcIndex>
  void BTOp(OpcodeArgs);
  template<uint32_t SrcIndex>
  void BTROp(OpcodeArgs);
  template<uint32_t SrcIndex>
  void BTSOp(OpcodeArgs);
  template<uint32_t SrcIndex>
  void BTCOp(OpcodeArgs);
  void IMUL1SrcOp(OpcodeArgs);
  void IMUL2SrcOp(OpcodeArgs);
  void IMULOp(OpcodeArgs);
  void STOSOp(OpcodeArgs);
  void MOVSOp(OpcodeArgs);
  void CMPSOp(OpcodeArgs);
  void LODSOp(OpcodeArgs);
  void SCASOp(OpcodeArgs);
  void BSWAPOp(OpcodeArgs);
  void PUSHFOp(OpcodeArgs);
  void POPFOp(OpcodeArgs);

  void RDTSCOp(OpcodeArgs);
  void INCOp(OpcodeArgs);
  void DECOp(OpcodeArgs);
  void NEGOp(OpcodeArgs);
  void DIVOp(OpcodeArgs);
  void IDIVOp(OpcodeArgs);
  void BSFOp(OpcodeArgs);
  void BSROp(OpcodeArgs);
  void CMPXCHGOp(OpcodeArgs);
  void CMPXCHGPairOp(OpcodeArgs);
  void MULOp(OpcodeArgs);
  void NOTOp(OpcodeArgs);
  void XADDOp(OpcodeArgs);
  void PopcountOp(OpcodeArgs);
  void DAAOp(OpcodeArgs);
  void DASOp(OpcodeArgs);
  void AAAOp(OpcodeArgs);
  void AASOp(OpcodeArgs);
  void AAMOp(OpcodeArgs);
  void AADOp(OpcodeArgs);
  void XLATOp(OpcodeArgs);
  template<bool Reseed>
  void RDRANDOp(OpcodeArgs);

  enum class Segment {
    FS,
    GS,
  };
  template<Segment Seg>
  void ReadSegmentReg(OpcodeArgs);
  template<Segment Seg>
  void WriteSegmentReg(OpcodeArgs);
  void EnterOp(OpcodeArgs);

  void SGDTOp(OpcodeArgs);

  // SSE
  void MOVLPOp(OpcodeArgs);
  void MOVHPDOp(OpcodeArgs);
  void MOVSDOp(OpcodeArgs);
  void MOVSSOp(OpcodeArgs);
  template<FEXCore::IR::IROps IROp, size_t ElementSize>
  void VectorALUOp(OpcodeArgs);
  template<FEXCore::IR::IROps IROp, size_t ElementSize>
  void VectorALUROp(OpcodeArgs);
  template<FEXCore::IR::IROps IROp, size_t ElementSize>
  void VectorUnaryOp(OpcodeArgs);
  template<FEXCore::IR::IROps IROp, size_t ElementSize>
  void VectorUnaryDuplicateOp(OpcodeArgs);

  void MOVQOp(OpcodeArgs);
  void MOVQMMXOp(OpcodeArgs);
  template<size_t ElementSize>
  void MOVMSKOp(OpcodeArgs);
  void MOVMSKOpOne(OpcodeArgs);
  template<size_t ElementSize>
  void PUNPCKLOp(OpcodeArgs);
  template<size_t ElementSize>
  void PUNPCKHOp(OpcodeArgs);
  void PSHUFBOp(OpcodeArgs);
  template<bool Low>
  void PSHUFWOp(OpcodeArgs);
  void PSHUFW8ByteOp(OpcodeArgs);
  void PSHUFDOp(OpcodeArgs);
  template<size_t ElementSize>
  void PSRLDOp(OpcodeArgs);
  template<size_t ElementSize>
  void PSRLI(OpcodeArgs);
  template<size_t ElementSize>
  void PSLLI(OpcodeArgs);
  template<size_t ElementSize>
  void PSLL(OpcodeArgs);
  template<size_t ElementSize>
  void PSRAOp(OpcodeArgs);
  void PSRLDQ(OpcodeArgs);
  void PSLLDQ(OpcodeArgs);
  template<size_t ElementSize>
  void PSRAIOp(OpcodeArgs);
  void MOVDDUPOp(OpcodeArgs);
  template<size_t DstElementSize>
  void CVTGPR_To_FPR(OpcodeArgs);
  template<size_t SrcElementSize, bool HostRoundingMode>
  void CVTFPR_To_GPR(OpcodeArgs);
  template<size_t SrcElementSize, bool Widen>
  void Vector_CVT_Int_To_Float(OpcodeArgs);
  template<size_t DstElementSize, size_t SrcElementSize>
  void Scalar_CVT_Float_To_Float(OpcodeArgs);
  template<size_t DstElementSize, size_t SrcElementSize>
  void Vector_CVT_Float_To_Float(OpcodeArgs);
  template<size_t SrcElementSize, bool Narrow, bool HostRoundingMode>
  void Vector_CVT_Float_To_Int(OpcodeArgs);
  void MMX_To_XMM_Vector_CVT_Int_To_Float(OpcodeArgs);
  template<size_t SrcElementSize, bool Narrow, bool HostRoundingMode>
  void XMM_To_MMX_Vector_CVT_Float_To_Int(OpcodeArgs);
  void MASKMOVOp(OpcodeArgs);
  void MOVBetweenGPR_FPR(OpcodeArgs);
  void TZCNT(OpcodeArgs);
  void LZCNT(OpcodeArgs);
  template<size_t ElementSize>
  void VFCMPOp(OpcodeArgs);
  template<size_t ElementSize>
  void SHUFOp(OpcodeArgs);
  template<size_t ElementSize>
  void PINSROp(OpcodeArgs);
  void InsertPSOp(OpcodeArgs);
  template<size_t ElementSize>
  void PExtrOp(OpcodeArgs);

  template <size_t ElementSize>
  void PSIGN(OpcodeArgs);
  template <size_t ElementSize>
  void VPSIGN(OpcodeArgs);

  // BMI1 Ops
  void ANDNBMIOp(OpcodeArgs);
  void BEXTRBMIOp(OpcodeArgs);
  void BLSIBMIOp(OpcodeArgs);
  void BLSMSKBMIOp(OpcodeArgs);
  void BLSRBMIOp(OpcodeArgs);

  // BMI2 Ops
  void BMI2Shift(OpcodeArgs);
  void BZHI(OpcodeArgs);
  void MULX(OpcodeArgs);
  void PDEP(OpcodeArgs);
  void PEXT(OpcodeArgs);
  void RORX(OpcodeArgs);

  // ADX Ops
  void ADXOp(OpcodeArgs);

  // AVX Ops
  template <IROps IROp, size_t ElementSize>
  void AVXVectorALUOp(OpcodeArgs);
  template <IROps IROp, size_t ElementSize>
  void AVXVectorUnaryOp(OpcodeArgs);

  template <size_t ElementSize>
  void AVXVectorRound(OpcodeArgs);

  template <size_t DstElementSize, size_t SrcElementSize>
  void AVXScalar_CVT_Float_To_Float(OpcodeArgs);

  template <size_t SrcElementSize, bool Narrow, bool HostRoundingMode>
  void AVXVector_CVT_Float_To_Int(OpcodeArgs);

  template <size_t SrcElementSize, bool Widen>
  void AVXVector_CVT_Int_To_Float(OpcodeArgs);

  template<FEXCore::IR::IROps IROp, size_t ElementSize>
  void VectorScalarInsertALUOp(OpcodeArgs);
  template<FEXCore::IR::IROps IROp, size_t ElementSize>
  void AVXVectorScalarInsertALUOp(OpcodeArgs);

  template<FEXCore::IR::IROps IROp, size_t ElementSize>
  void VectorScalarUnaryInsertALUOp(OpcodeArgs);
  template<FEXCore::IR::IROps IROp, size_t ElementSize>
  void AVXVectorScalarUnaryInsertALUOp(OpcodeArgs);

  template<size_t DstElementSize, size_t SrcElementSize>
  void AVXVector_CVT_Float_To_Float(OpcodeArgs);

  void InsertMMX_To_XMM_Vector_CVT_Int_To_Float(OpcodeArgs);
  template<size_t DstElementSize>
  void InsertCVTGPR_To_FPR(OpcodeArgs);
  template <size_t DstElementSize>
  void AVXInsertCVTGPR_To_FPR(OpcodeArgs);

  template<size_t DstElementSize, size_t SrcElementSize>
  void InsertScalar_CVT_Float_To_Float(OpcodeArgs);
  template<size_t DstElementSize, size_t SrcElementSize>
  void AVXInsertScalar_CVT_Float_To_Float(OpcodeArgs);

  template <size_t ElementSize>
  void InsertScalarRound(OpcodeArgs);
  template <size_t ElementSize>
  void AVXInsertScalarRound(OpcodeArgs);

  template <size_t ElementSize>
  void InsertScalarFCMPOp(OpcodeArgs);
  template <size_t ElementSize>
  void AVXInsertScalarFCMPOp(OpcodeArgs);

  template <size_t DstElementSize>
  void AVXCVTGPR_To_FPR(OpcodeArgs);

  template <size_t ElementSize>
  void AVXVFCMPOp(OpcodeArgs);

  template <size_t ElementSize>
  void VADDSUBPOp(OpcodeArgs);

  void VAESDecOp(OpcodeArgs);
  void VAESDecLastOp(OpcodeArgs);
  void VAESEncOp(OpcodeArgs);
  void VAESEncLastOp(OpcodeArgs);

  void VANDNOp(OpcodeArgs);

  void VBLENDPDOp(OpcodeArgs);
  void VPBLENDDOp(OpcodeArgs);
  void VPBLENDWOp(OpcodeArgs);

  template <size_t ElementSize>
  void VBROADCASTOp(OpcodeArgs);

  template <size_t ElementSize>
  void VDPPOp(OpcodeArgs);

  void VEXTRACT128Op(OpcodeArgs);

  template <IROps IROp, size_t ElementSize>
  void VHADDPOp(OpcodeArgs);
  template <size_t ElementSize>
  void VHSUBPOp(OpcodeArgs);

  void VINSERTOp(OpcodeArgs);
  void VINSERTPSOp(OpcodeArgs);

  template <size_t ElementSize, bool IsStore>
  void VMASKMOVOp(OpcodeArgs);

  void VMOVHPOp(OpcodeArgs);
  void VMOVLPOp(OpcodeArgs);

  void VMOVDDUPOp(OpcodeArgs);
  void VMOVSHDUPOp(OpcodeArgs);
  void VMOVSLDUPOp(OpcodeArgs);

  void VMOVSDOp(OpcodeArgs);
  void VMOVSSOp(OpcodeArgs);

  void VMOVAPS_VMOVAPDOp(OpcodeArgs);
  void VMOVUPS_VMOVUPDOp(OpcodeArgs);

  void VMPSADBWOp(OpcodeArgs);

  template <size_t ElementSize>
  void VPACKSSOp(OpcodeArgs);

  template <size_t ElementSize>
  void VPACKUSOp(OpcodeArgs);

  void VPALIGNROp(OpcodeArgs);

  void VPCMPESTRIOp(OpcodeArgs);
  void VPCMPESTRMOp(OpcodeArgs);
  void VPCMPISTRIOp(OpcodeArgs);
  void VPCMPISTRMOp(OpcodeArgs);

  void VPERM2Op(OpcodeArgs);
  void VPERMDOp(OpcodeArgs);
  void VPERMQOp(OpcodeArgs);

  template <size_t ElementSize>
  void VPERMILImmOp(OpcodeArgs);
  template <size_t ElementSize>
  void VPERMILRegOp(OpcodeArgs);

  void VPHADDSWOp(OpcodeArgs);

  template <size_t ElementSize>
  void VPHSUBOp(OpcodeArgs);
  void VPHSUBSWOp(OpcodeArgs);

  void VPINSRBOp(OpcodeArgs);
  void VPINSRDQOp(OpcodeArgs);
  void VPINSRWOp(OpcodeArgs);

  void VPMADDUBSWOp(OpcodeArgs);
  void VPMADDWDOp(OpcodeArgs);

  template <bool IsStore>
  void VPMASKMOVOp(OpcodeArgs);

  void VPMULHRSWOp(OpcodeArgs);

  template <bool Signed>
  void VPMULHWOp(OpcodeArgs);

  template <size_t ElementSize, bool Signed>
  void VPMULLOp(OpcodeArgs);

  void VPSADBWOp(OpcodeArgs);

  void VPSHUFBOp(OpcodeArgs);

  template <size_t ElementSize, bool Low>
  void VPSHUFWOp(OpcodeArgs);

  template <size_t ElementSize>
  void VPSLLOp(OpcodeArgs);
  void VPSLLDQOp(OpcodeArgs);
  template <size_t ElementSize>
  void VPSLLIOp(OpcodeArgs);
  void VPSLLVOp(OpcodeArgs);

  template <size_t ElementSize>
  void VPSRAOp(OpcodeArgs);

  template <size_t ElementSize>
  void VPSRAIOp(OpcodeArgs);

  void VPSRAVDOp(OpcodeArgs);
  void VPSRLVOp(OpcodeArgs);

  template <size_t ElementSize>
  void VPSRLDOp(OpcodeArgs);
  void VPSRLDQOp(OpcodeArgs);

  template <size_t ElementSize>
  void VPUNPCKHOp(OpcodeArgs);

  template <size_t ElementSize>
  void VPUNPCKLOp(OpcodeArgs);

  template <size_t ElementSize>
  void VPSRLIOp(OpcodeArgs);

  template <size_t ElementSize>
  void VSHUFOp(OpcodeArgs);

  template <size_t ElementSize>
  void VTESTPOp(OpcodeArgs);

  void VZEROOp(OpcodeArgs);

  // X87 Ops
  OrderedNode *ReconstructFSW();
  // Returns new x87 stack top from FSW.
  OrderedNode *ReconstructX87StateFromFSW(OrderedNode *FSW);
  template<size_t width>
  void FLD(OpcodeArgs);
  template<uint64_t Lower, uint32_t Upper>
  void FLD_Const(OpcodeArgs);

  void FBLD(OpcodeArgs);
  void FBSTP(OpcodeArgs);

  void FILD(OpcodeArgs);

  template<size_t width>
  void FST(OpcodeArgs);

  void FST(OpcodeArgs);

  template<bool Truncate>
  void FIST(OpcodeArgs);

  enum class OpResult {
    RES_ST0,
    RES_STI,
  };
  template<size_t width, bool Integer, OpResult ResInST0>
  void FADD(OpcodeArgs);
  template<size_t width, bool Integer, OpResult ResInST0>
  void FMUL(OpcodeArgs);
  template<size_t width, bool Integer, bool reverse, OpResult ResInST0>
  void FDIV(OpcodeArgs);
  template<size_t width, bool Integer, bool reverse, OpResult ResInST0>
  void FSUB(OpcodeArgs);
  void FCHS(OpcodeArgs);
  void FABS(OpcodeArgs);
  void FTST(OpcodeArgs);
  void FRNDINT(OpcodeArgs);
  void FXTRACT(OpcodeArgs);
  void FNINIT(OpcodeArgs);

  template<FEXCore::IR::IROps IROp>
  void X87UnaryOp(OpcodeArgs);
  template<FEXCore::IR::IROps IROp>
  void X87BinaryOp(OpcodeArgs);
  template<bool Inc>
  void X87ModifySTP(OpcodeArgs);
  void X87SinCos(OpcodeArgs);
  void X87FYL2X(OpcodeArgs);
  void X87TAN(OpcodeArgs);
  void X87ATAN(OpcodeArgs);
  void X87LDENV(OpcodeArgs);
  void X87FLDCW(OpcodeArgs);
  void X87FNSTENV(OpcodeArgs);
  void X87FSTCW(OpcodeArgs);
  void X87LDSW(OpcodeArgs);
  void X87FNSTSW(OpcodeArgs);
  void X87FNSAVE(OpcodeArgs);
  void X87FRSTOR(OpcodeArgs);
  void X87FXAM(OpcodeArgs);
  void X87FCMOV(OpcodeArgs);
  void X87EMMS(OpcodeArgs);
  void X87FFREE(OpcodeArgs);

  void FXCH(OpcodeArgs);

  enum class FCOMIFlags {
    FLAGS_X87,
    FLAGS_RFLAGS,
  };
  template<size_t width, bool Integer, FCOMIFlags whichflags, bool poptwice>
  void FCOMI(OpcodeArgs);

  // F64 X87 Ops
  template<size_t width>
  void FLDF64(OpcodeArgs);
  template<uint64_t num>
  void FLDF64_Const(OpcodeArgs);

  void FBLDF64(OpcodeArgs);
  void FBSTPF64(OpcodeArgs);

  void FILDF64(OpcodeArgs);

  template<size_t width>
  void FSTF64(OpcodeArgs);

  void FSTF64(OpcodeArgs);

  template<bool Truncate>
  void FISTF64(OpcodeArgs);

  template<size_t width, bool Integer, OpResult ResInST0>
  void FADDF64(OpcodeArgs);
  template<size_t width, bool Integer, OpResult ResInST0>
  void FMULF64(OpcodeArgs);
  template<size_t width, bool Integer, bool reverse, OpResult ResInST0>
  void FDIVF64(OpcodeArgs);
  template<size_t width, bool Integer, bool reverse, OpResult ResInST0>
  void FSUBF64(OpcodeArgs);
  void FCHSF64(OpcodeArgs);
  void FABSF64(OpcodeArgs);
  void FTSTF64(OpcodeArgs);
  void FRNDINTF64(OpcodeArgs);
  void FXTRACTF64(OpcodeArgs);
  void FNINITF64(OpcodeArgs);
  void FSQRTF64(OpcodeArgs);
  template<FEXCore::IR::IROps IROp>
  void X87UnaryOpF64(OpcodeArgs);
  template<FEXCore::IR::IROps IROp>
  void X87BinaryOpF64(OpcodeArgs);
  void X87SinCosF64(OpcodeArgs);
  void X87FLDCWF64(OpcodeArgs);
  void X87FYL2XF64(OpcodeArgs);
  void X87TANF64(OpcodeArgs);
  void X87ATANF64(OpcodeArgs);
  void X87FNSAVEF64(OpcodeArgs);
  void X87FRSTORF64(OpcodeArgs);
  void X87FXAMF64(OpcodeArgs);
  void X87LDENVF64(OpcodeArgs);

  template<size_t width, bool Integer, FCOMIFlags whichflags, bool poptwice>
  void FCOMIF64(OpcodeArgs);

  void FXSaveOp(OpcodeArgs);
  void FXRStoreOp(OpcodeArgs);

  void XSaveOp(OpcodeArgs);

  void PAlignrOp(OpcodeArgs);
  template<size_t ElementSize>
  void UCOMISxOp(OpcodeArgs);
  void LDMXCSR(OpcodeArgs);
  void STMXCSR(OpcodeArgs);

  template<size_t ElementSize>
  void PACKUSOp(OpcodeArgs);

  template<size_t ElementSize>
  void PACKSSOp(OpcodeArgs);

  template<size_t ElementSize, bool Signed>
  void PMULLOp(OpcodeArgs);

  template<bool ToXMM>
  void MOVQ2DQ(OpcodeArgs);

  template<size_t ElementSize>
  void ADDSUBPOp(OpcodeArgs);

  void PFNACCOp(OpcodeArgs);
  void PFPNACCOp(OpcodeArgs);
  void PSWAPDOp(OpcodeArgs);

  template<uint8_t CompType>
  void VPFCMPOp(OpcodeArgs);
  void PI2FWOp(OpcodeArgs);
  void PF2IWOp(OpcodeArgs);

  void PMULHRWOp(OpcodeArgs);

  void PMADDWD(OpcodeArgs);
  void PMADDUBSW(OpcodeArgs);

  template<bool Signed>
  void PMULHW(OpcodeArgs);

  void PMULHRSW(OpcodeArgs);

  void MOVBEOp(OpcodeArgs);
  template<size_t ElementSize>
  void HSUBP(OpcodeArgs);
  template<size_t ElementSize>
  void PHSUB(OpcodeArgs);

  void PHADDS(OpcodeArgs);
  void PHSUBS(OpcodeArgs);

  void CLWB(OpcodeArgs);
  void CLFLUSHOPT(OpcodeArgs);
  void LoadFenceOrXRSTOR(OpcodeArgs);
  void MemFenceOrXSAVEOPT(OpcodeArgs);
  void StoreFenceOrCLFlush(OpcodeArgs);
  void CLZeroOp(OpcodeArgs);
  void RDTSCPOp(OpcodeArgs);

  void PSADBW(OpcodeArgs);

  void SHA1NEXTEOp(OpcodeArgs);
  void SHA1MSG1Op(OpcodeArgs);
  void SHA1MSG2Op(OpcodeArgs);
  void SHA1RNDS4Op(OpcodeArgs);

  void SHA256MSG1Op(OpcodeArgs);
  void SHA256MSG2Op(OpcodeArgs);
  void SHA256RNDS2Op(OpcodeArgs);

  void AESImcOp(OpcodeArgs);
  void AESEncOp(OpcodeArgs);
  void AESEncLastOp(OpcodeArgs);
  void AESDecOp(OpcodeArgs);
  void AESDecLastOp(OpcodeArgs);
  void AESKeyGenAssist(OpcodeArgs);

  template<size_t ElementSize, size_t DstElementSize, bool Signed>
  void ExtendVectorElements(OpcodeArgs);
  template<size_t ElementSize>
  void VectorRound(OpcodeArgs);

  template<size_t ElementSize>
  void VectorBlend(OpcodeArgs);

  template<size_t ElementSize>
  void VectorVariableBlend(OpcodeArgs);
  void PTestOp(OpcodeArgs);
  void PHMINPOSUWOp(OpcodeArgs);
  template<size_t ElementSize>
  void DPPOp(OpcodeArgs);

  void MPSADBWOp(OpcodeArgs);
  void PCLMULQDQOp(OpcodeArgs);
  void VPCLMULQDQOp(OpcodeArgs);

  void CRC32(OpcodeArgs);

  void UnimplementedOp(OpcodeArgs);

  void InvalidOp(OpcodeArgs);

  void SetPackedRFLAG(bool Lower8, OrderedNode *Src);
  OrderedNode *GetPackedRFLAG(uint32_t FlagsMask = ~0U);

  void SetMultiblock(bool _Multiblock) { Multiblock = _Multiblock; }

  static inline constexpr unsigned IndexNZCV(unsigned BitOffset) {
    switch (BitOffset) {
      case FEXCore::X86State::RFLAG_OF_RAW_LOC: return 28;
      case FEXCore::X86State::RFLAG_CF_RAW_LOC: return 29;
      case FEXCore::X86State::RFLAG_ZF_RAW_LOC: return 30;
      case FEXCore::X86State::RFLAG_SF_RAW_LOC: return 31;
      default: FEX_UNREACHABLE;
    }
  }

protected:
  void SaveNZCV(IROps Op) override {
    /* Some opcodes are conservatively marked as clobbering flags, but in fact
     * do not clobber flags in certain conditions. Check for that here as an
     * optimization.
     */
    switch (Op) {
    case OP_VFMINSCALARINSERT:
    case OP_VFMAXSCALARINSERT:
      /* On AFP platforms, becomes fmin/fmax and preserves NZCV. Otherwise
       * becomes fcmp and clobbers.
       */
      if (CTX->HostFeatures.SupportsAFP)
        return;

      break;
    default:
      break;
    }

    // Invariant: When executing instructions that clobber NZCV, the flags must
    // be resident in a GPR, which is equivalent to CachedNZCV != nullptr. Get
    // the NZCV which fills the cache if necessary.
    if (CachedNZCV == nullptr)
      GetNZCV();

    // Assume we'll need a reload.
    NZCVDirty = true;
  }

private:
  enum class SelectionFlag {
    Nothing,  // must rely on x86 flags
    CMP,      // flags were set by a CMP between flagsOpDest/flagsOpDestSigned and flagsOpSrc/flagsOpSrcSigned with flagsOpSize size
    AND,      // flags were set by an AND/TEST, flagsOpDest contains the resulting value of flagsOpSize size
    FCMP,     // flags were set by a ucomis* / comis*
  };

  struct JumpTargetInfo {
    OrderedNode* BlockEntry;
    bool HaveEmitted;
  };

  FEXCore::Context::ContextImpl *CTX{};

  SelectionFlag flagsOp{};
  uint8_t flagsOpSize{};
  OrderedNode* flagsOpDest{};
  OrderedNode* flagsOpSrc{};
  OrderedNode* flagsOpDestSigned{};
  OrderedNode* flagsOpSrcSigned{};

  constexpr static unsigned FullNZCVMask =
    (1U << FEXCore::X86State::RFLAG_CF_RAW_LOC) |
    (1U << FEXCore::X86State::RFLAG_ZF_RAW_LOC) |
    (1U << FEXCore::X86State::RFLAG_SF_RAW_LOC) |
    (1U << FEXCore::X86State::RFLAG_OF_RAW_LOC);

  static bool ContainsNZCV(unsigned BitMask) {
    return (BitMask & FullNZCVMask) != 0;
  }

  static bool IsNZCV(unsigned BitOffset) {
    switch (BitOffset) {
      case FEXCore::X86State::RFLAG_CF_RAW_LOC:
      case FEXCore::X86State::RFLAG_ZF_RAW_LOC:
      case FEXCore::X86State::RFLAG_SF_RAW_LOC:
      case FEXCore::X86State::RFLAG_OF_RAW_LOC:
        return true;

      default:
        return false;
    }
  }

  OrderedNode* CachedNZCV{};
  bool NZCVDirty{};
  uint32_t PossiblySetNZCVBits{};

  fextl::map<uint64_t, JumpTargetInfo> JumpTargets;
  bool HandledLock{false};
  bool DecodeFailure{false};
  bool NeedsBlockEnd{false};
  // Used during new op bringup
  bool ShouldDump{false};

  void ALUOpImpl(OpcodeArgs, FEXCore::IR::IROps ALUIROp, FEXCore::IR::IROps AtomicFetchOp);

  // Opcode helpers for generalizing behavior across VEX and non-VEX variants.

  OrderedNode* ADDSUBPOpImpl(OpcodeArgs, size_t ElementSize,
                             OrderedNode *Src1, OrderedNode *Src2);

  void AVXVectorALUOpImpl(OpcodeArgs, IROps IROp, size_t ElementSize);
  void AVXVectorUnaryOpImpl(OpcodeArgs, IROps IROp, size_t ElementSize);

  template <size_t ElementSize>
  void AVXVectorVariableBlend(OpcodeArgs);

  void AVXVariableShiftImpl(OpcodeArgs, IROps IROp);

  OrderedNode* AESKeyGenAssistImpl(OpcodeArgs);

  OrderedNode* CVTGPR_To_FPRImpl(OpcodeArgs, size_t DstElementSize,
                                 const X86Tables::DecodedOperand& Src1Op,
                                 const X86Tables::DecodedOperand& Src2Op);

  OrderedNode* DPPOpImpl(OpcodeArgs, const X86Tables::DecodedOperand& Src1,
                         const X86Tables::DecodedOperand& Src2,
                         const X86Tables::DecodedOperand& Imm, size_t ElementSize);

  OrderedNode* VDPPSOpImpl(OpcodeArgs, const X86Tables::DecodedOperand& Src1,
                           const X86Tables::DecodedOperand& Src2,
                           const X86Tables::DecodedOperand& Imm);

  OrderedNode* ExtendVectorElementsImpl(OpcodeArgs, size_t ElementSize,
                                        size_t DstElementSize, bool Signed);

  OrderedNode* HSUBPOpImpl(OpcodeArgs, size_t ElementSize,
                           const X86Tables::DecodedOperand& Src1Op,
                           const X86Tables::DecodedOperand& Src2Op);

  OrderedNode* InsertPSOpImpl(OpcodeArgs, const X86Tables::DecodedOperand& Src1,
                              const X86Tables::DecodedOperand& Src2,
                              const X86Tables::DecodedOperand& Imm);

  OrderedNode* MPSADBWOpImpl(OpcodeArgs, const X86Tables::DecodedOperand& Src1Op,
                             const X86Tables::DecodedOperand& Src2Op,
                             const X86Tables::DecodedOperand& ImmOp);

  OrderedNode* PACKSSOpImpl(OpcodeArgs, size_t ElementSize,
                            OrderedNode *Src1, OrderedNode *Src2);

  OrderedNode* PACKUSOpImpl(OpcodeArgs, size_t ElementSize,
                            OrderedNode *Src1, OrderedNode *Src2);

  OrderedNode* PALIGNROpImpl(OpcodeArgs, const X86Tables::DecodedOperand& Src1,
                             const X86Tables::DecodedOperand& Src2,
                             const X86Tables::DecodedOperand& Imm,
                             bool IsAVX);

  void PCMPXSTRXOpImpl(OpcodeArgs, bool IsExplicit, bool IsMask);

  OrderedNode* PHADDSOpImpl(OpcodeArgs, const X86Tables::DecodedOperand& Src1,
                            const X86Tables::DecodedOperand& Src2);

  OrderedNode* PHMINPOSUWOpImpl(OpcodeArgs);

  OrderedNode* PHSUBOpImpl(OpcodeArgs, const X86Tables::DecodedOperand& Src1,
                           const X86Tables::DecodedOperand& Src2, size_t ElementSize);

  OrderedNode* PHSUBSOpImpl(OpcodeArgs, const X86Tables::DecodedOperand& Src1Op,
                            const X86Tables::DecodedOperand& Src2Op);

  OrderedNode* PINSROpImpl(OpcodeArgs, size_t ElementSize,
                           const X86Tables::DecodedOperand& Src1Op,
                           const X86Tables::DecodedOperand& Src2Op,
                           const X86Tables::DecodedOperand& Imm);

  OrderedNode* PMADDWDOpImpl(OpcodeArgs, const X86Tables::DecodedOperand& Src1,
                             const X86Tables::DecodedOperand& Src2);

  OrderedNode* PMADDUBSWOpImpl(OpcodeArgs, const X86Tables::DecodedOperand& Src1Op,
                               const X86Tables::DecodedOperand& Src2Op);

  OrderedNode* PMULHRSWOpImpl(OpcodeArgs, OrderedNode *Src1, OrderedNode *Src2);

  OrderedNode* PMULHWOpImpl(OpcodeArgs, bool Signed,
                            OrderedNode *Src1, OrderedNode *Src2);

  OrderedNode* PMULLOpImpl(OpcodeArgs, size_t ElementSize, bool Signed,
                           OrderedNode *Src1, OrderedNode *Src2);

  OrderedNode* PSADBWOpImpl(OpcodeArgs, const X86Tables::DecodedOperand& Src1Op,
                            const X86Tables::DecodedOperand& Src2Op);

  OrderedNode* PSHUFBOpImpl(OpcodeArgs, const X86Tables::DecodedOperand& Src1,
                            const X86Tables::DecodedOperand& Src2);

  OrderedNode* PSIGNImpl(OpcodeArgs, size_t ElementSize,
                         OrderedNode *Src1, OrderedNode *Src2);

  OrderedNode* PSLLIImpl(OpcodeArgs, size_t ElementSize,
                         OrderedNode *Src, uint64_t Shift);

  OrderedNode* PSLLImpl(OpcodeArgs, size_t ElementSize,
                        OrderedNode *Src, OrderedNode *ShiftVec);

  OrderedNode* PSRAOpImpl(OpcodeArgs, size_t ElementSize,
                          OrderedNode *Src, OrderedNode *ShiftVec);

  OrderedNode* PSRLDOpImpl(OpcodeArgs, size_t ElementSize,
                           OrderedNode *Src, OrderedNode *ShiftVec);

  OrderedNode* SHUFOpImpl(OpcodeArgs, size_t ElementSize,
                          const X86Tables::DecodedOperand& Src1,
                          const X86Tables::DecodedOperand& Src2,
                          const X86Tables::DecodedOperand& Imm);

  void VMASKMOVOpImpl(OpcodeArgs, size_t ElementSize, size_t DataSize, bool IsStore,
                      const X86Tables::DecodedOperand& MaskOp,
                      const X86Tables::DecodedOperand& DataOp);

  void MOVScalarOpImpl(OpcodeArgs, size_t ElementSize);
  void VMOVScalarOpImpl(OpcodeArgs, size_t ElementSize);

  OrderedNode* VFCMPOpImpl(OpcodeArgs, size_t ElementSize,
                           OrderedNode *Src1, OrderedNode *Src2, uint8_t CompType);

  void VTESTOpImpl(OpcodeArgs, size_t ElementSize);

  void VectorALUOpImpl(OpcodeArgs, IROps IROp, size_t ElementSize);
  void VectorALUROpImpl(OpcodeArgs, IROps IROp, size_t ElementSize);
  void VectorUnaryOpImpl(OpcodeArgs, IROps IROp, size_t ElementSize);
  void VectorUnaryDuplicateOpImpl(OpcodeArgs, IROps IROp, size_t ElementSize);

  // x86 ALU scalar operations operate in three different ways
  // - AVX512: Writemask shenanigans that we don't care about.
  // - AVX/VEX: Two source
  //   - Example 32bit VADDSS Dest, Src1, Src2
  //   - Dest[31:0] = Src1[31:0] + Src2[31:0]
  //   - Dest[127:32] = Src1[127:32]
  // - SSE: Scalar operation inserts in to the low bits, upper bits completely unaffected.
  //   - Example 32bit ADDSS Dest, Src
  //   - Dest[31:0] = Dest[31:0] + Src[31:0]
  //   - Dest[{256,128}:32] = (Unmodified)
  OrderedNode* VectorScalarInsertALUOpImpl(OpcodeArgs, IROps IROp,
                                   size_t DstSize, size_t ElementSize,
                                   const X86Tables::DecodedOperand& Src1Op,
                                   const X86Tables::DecodedOperand& Src2Op,
                                   bool ZeroUpperBits);

  OrderedNode* VectorScalarUnaryInsertALUOpImpl(OpcodeArgs, IROps IROp,
                                   size_t DstSize, size_t ElementSize,
                                   const X86Tables::DecodedOperand& Src1Op,
                                   const X86Tables::DecodedOperand& Src2Op,
                                   bool ZeroUpperBits);

  OrderedNode* InsertCVTGPR_To_FPRImpl(OpcodeArgs,
                                       size_t DstSize, size_t DstElementSize,
                                       const X86Tables::DecodedOperand& Src1Op,
                                       const X86Tables::DecodedOperand& Src2Op,
                                       bool ZeroUpperBits);

  OrderedNode* InsertScalar_CVT_Float_To_FloatImpl(OpcodeArgs,
                                                   size_t DstSize, size_t DstElementSize, size_t SrcElementSize,
                                                   const X86Tables::DecodedOperand& Src1Op,
                                                   const X86Tables::DecodedOperand& Src2Op,
                                                   bool ZeroUpperBits);
  OrderedNode* InsertScalarRoundImpl(OpcodeArgs,
                                     size_t DstSize, size_t ElementSize,
                                     const X86Tables::DecodedOperand& Src1Op,
                                     const X86Tables::DecodedOperand& Src2Op,
                                     uint64_t Mode, bool ZeroUpperBits);

  OrderedNode* InsertScalarFCMPOpImpl(OpcodeArgs,
                                      size_t DstSize, size_t ElementSize,
                                      const X86Tables::DecodedOperand& Src1Op,
                                      const X86Tables::DecodedOperand& Src2Op,
                                      uint8_t CompType, bool ZeroUpperBits);

  OrderedNode* VectorRoundImpl(OpcodeArgs, size_t ElementSize,
                               OrderedNode *Src, uint64_t Mode);

  OrderedNode* Scalar_CVT_Float_To_FloatImpl(OpcodeArgs, size_t DstElementSize, size_t SrcElementSize,
                                             const X86Tables::DecodedOperand& Src1Op,
                                             const X86Tables::DecodedOperand& Src2Op);

  void Vector_CVT_Float_To_FloatImpl(OpcodeArgs, size_t DstElementSize, size_t SrcElementSize, bool IsAVX);

  OrderedNode* Vector_CVT_Float_To_IntImpl(OpcodeArgs, size_t SrcElementSize, bool Narrow, bool HostRoundingMode);

  OrderedNode* Vector_CVT_Int_To_FloatImpl(OpcodeArgs, size_t SrcElementSize, bool Widen);

  void XSaveOpImpl(OpcodeArgs);
  void SaveX87State(OpcodeArgs, OrderedNode *MemBase);
  void SaveSSEState(OrderedNode *MemBase);
  void SaveMXCSRState(OrderedNode *MemBase);
  void SaveAVXState(OrderedNode *MemBase);

  void XRstorOpImpl(OpcodeArgs);
  void RestoreX87State(OrderedNode *MemBase);
  void RestoreSSEState(OrderedNode *MemBase);
  void RestoreMXCSRState(OrderedNode *MXCSR);
  void RestoreAVXState(OrderedNode *MemBase);
  void DefaultX87State(OpcodeArgs);
  void DefaultSSEState();
  void DefaultAVXState();

  OrderedNode *GetMXCSR();

  #undef OpcodeArgs

  OrderedNode *AppendSegmentOffset(OrderedNode *Value, uint32_t Flags, uint32_t DefaultPrefix = 0, bool Override = false);
  OrderedNode *GetSegment(uint32_t Flags, uint32_t DefaultPrefix = 0, bool Override = false);

  void UpdatePrefixFromSegment(OrderedNode *Segment, uint32_t SegmentReg);

  OrderedNode *LoadGPRRegister(uint32_t GPR, int8_t Size = -1, uint8_t Offset = 0, bool AllowUpperGarbage = false);
  OrderedNode *LoadXMMRegister(uint32_t XMM);
  void StoreGPRRegister(uint32_t GPR, OrderedNode *const Src, int8_t Size = -1, uint8_t Offset = 0);
  void StoreXMMRegister(uint32_t XMM, OrderedNode *const Src);

  OrderedNode *GetRelocatedPC(FEXCore::X86Tables::DecodedOp const& Op, int64_t Offset = 0);

  OrderedNode *LoadSource(RegisterClassType Class, X86Tables::DecodedOp const& Op, X86Tables::DecodedOperand const& Operand, uint32_t Flags, const LoadSourceOptions& Options = {});
  OrderedNode *LoadSource_WithOpSize(RegisterClassType Class, X86Tables::DecodedOp const& Op, X86Tables::DecodedOperand const& Operand, uint8_t OpSize, uint32_t Flags, const LoadSourceOptions& Options = {});
  void StoreResult_WithOpSize(FEXCore::IR::RegisterClassType Class, FEXCore::X86Tables::DecodedOp Op, FEXCore::X86Tables::DecodedOperand const& Operand, OrderedNode *const Src, uint8_t OpSize, int8_t Align, MemoryAccessType AccessType = MemoryAccessType::DEFAULT);
  void StoreResult(FEXCore::IR::RegisterClassType Class, FEXCore::X86Tables::DecodedOp Op, FEXCore::X86Tables::DecodedOperand const& Operand, OrderedNode *const Src, int8_t Align, MemoryAccessType AccessType = MemoryAccessType::DEFAULT);
  void StoreResult(FEXCore::IR::RegisterClassType Class, FEXCore::X86Tables::DecodedOp Op, OrderedNode *const Src, int8_t Align, MemoryAccessType AccessType = MemoryAccessType::DEFAULT);

  constexpr OpSize GetGuestVectorLength() const {
    return CTX->HostFeatures.SupportsAVX ? OpSize::i256Bit : OpSize::i128Bit;
  }

  [[nodiscard]] static uint32_t GPROffset(X86State::X86Reg reg) {
    LOGMAN_THROW_AA_FMT(reg <= X86State::X86Reg::REG_R15, "Invalid reg used");
    return static_cast<uint32_t>(offsetof(Core::CPUState, gregs[static_cast<size_t>(reg)]));
  }

  [[nodiscard]] static uint32_t MMBaseOffset() {
    return static_cast<uint32_t>(offsetof(Core::CPUState, mm[0][0]));
  }

  [[nodiscard]] uint8_t GetDstSize(X86Tables::DecodedOp Op) const;
  [[nodiscard]] uint8_t GetSrcSize(X86Tables::DecodedOp Op) const;
  [[nodiscard]] uint32_t GetDstBitSize(X86Tables::DecodedOp Op) const;
  [[nodiscard]] uint32_t GetSrcBitSize(X86Tables::DecodedOp Op) const;
  [[nodiscard]] IR::OpSize OpSizeFromDst(X86Tables::DecodedOp Op) const {
    return IR::SizeToOpSize(GetDstSize(Op));
  }
  [[nodiscard]] IR::OpSize OpSizeFromSrc(X86Tables::DecodedOp Op) const {
    return IR::SizeToOpSize(GetSrcSize(Op));
  }

  static inline constexpr unsigned NZCVIndexMask(unsigned BitMask) {
    unsigned NZCVMask{};
    if (BitMask & (1U << FEXCore::X86State::RFLAG_OF_RAW_LOC)) {
      NZCVMask |= 1U << IndexNZCV(FEXCore::X86State::RFLAG_OF_RAW_LOC);
    }
    if (BitMask & (1U << FEXCore::X86State::RFLAG_CF_RAW_LOC)) {
      NZCVMask |= 1U << IndexNZCV(FEXCore::X86State::RFLAG_CF_RAW_LOC);
    }
    if (BitMask & (1U << FEXCore::X86State::RFLAG_ZF_RAW_LOC)) {
      NZCVMask |= 1U << IndexNZCV(FEXCore::X86State::RFLAG_ZF_RAW_LOC);
    }
    if (BitMask & (1U << FEXCore::X86State::RFLAG_SF_RAW_LOC)) {
      NZCVMask |= 1U << IndexNZCV(FEXCore::X86State::RFLAG_SF_RAW_LOC);
    }
    return NZCVMask;
  }

  OrderedNode *GetNZCV() {
    if (!CachedNZCV) {
      CachedNZCV = _LoadNZCV();

      // We don't know what's set
      PossiblySetNZCVBits = ~0;
    }

    return CachedNZCV;
  }

  void SetNZCV(OrderedNode *Value) {
    CachedNZCV = Value;
    NZCVDirty = true;
  }

  void ZeroNZCV() {
    CachedNZCV = _Constant(0);
    PossiblySetNZCVBits = 0;
    NZCVDirty = true;
  }

  void ZeroCV() {
    // Get old NZCV before we mess with PossiblySetNZCVBits
    auto OldNZCV = GetNZCV();

    // Mask out the NZ bits, clearing CV. Even if the code sets CV after, this can end up faster
    // moves by allowing orlshl to be used instead of bfi.
    PossiblySetNZCVBits = (1u << IndexNZCV(FEXCore::X86State::RFLAG_SF_RAW_LOC)) |
                          (1u << IndexNZCV(FEXCore::X86State::RFLAG_ZF_RAW_LOC));
    SetNZCV(_And(OpSize::i32Bit, OldNZCV, _Constant(PossiblySetNZCVBits)));
  }

  void SetNZ_ZeroCV(unsigned SrcSize, OrderedNode *Res) {
    CachedNZCV = _TestNZ(SrcSize, Res);
    PossiblySetNZCVBits = (1u << 31) | (1u << 30);
    NZCVDirty = true;
  }

  OrderedNode *InsertNZCV(OrderedNode *NZCV, unsigned BitOffset, OrderedNode *Value) {
    unsigned Bit = IndexNZCV(BitOffset);

    uint32_t SetBits = PossiblySetNZCVBits;
    PossiblySetNZCVBits |= (1u << Bit);

    if (SetBits == 0)
      return _Lshl(OpSize::i64Bit, Value, _Constant(Bit));
    else if ((SetBits & (1u << Bit)) == 0)
      return _Orlshl(OpSize::i32Bit, NZCV, Value, Bit);
    else
      return _Bfi(OpSize::i32Bit, 1, Bit, NZCV, Value);
  }

  template<unsigned BitOffset>
  void SetRFLAG(OrderedNode *Value) {
    SetRFLAG(Value, BitOffset);
  }

  void SetRFLAG(OrderedNode *Value, unsigned BitOffset) {
    flagsOp = SelectionFlag::Nothing;

    if (IsNZCV(BitOffset))
      SetNZCV(InsertNZCV(PossiblySetNZCVBits ? GetNZCV() : nullptr, BitOffset, Value));
    else
      _StoreFlag(Value, BitOffset);
  }

  void SetAF(unsigned Constant) {
    // AF is stored in bit 4 of the AF flag byte, with garbage in the other
    // bits. This allows us to defer the extract in the usual case. When it is
    // read, bit 4 is extracted.  In order to write a constant value of AF, that
    // means we need to left-shift here to compensate.
    SetRFLAG<FEXCore::X86State::RFLAG_AF_RAW_LOC>(_Constant(Constant << 4));
  }

  void ZeroMultipleFlags(uint32_t BitMask);

  OrderedNode *GetRFLAG(unsigned BitOffset) {
    if (IsNZCV(BitOffset)) {
      if (!CachedNZCV || (PossiblySetNZCVBits & (1u << IndexNZCV(BitOffset))))
        return _Bfe(OpSize::i32Bit, 1, IndexNZCV(BitOffset), GetNZCV());
      else
        return _Constant(0);
    } else {
      return _LoadFlag(BitOffset);
    }
  }

  // Helper to derive Dest by a given builder-using Expression with the opcode
  // replaced with NewOp. Useful for generic building code. Not safe in general.
  // but does the right handling of ImplicitFlagClobber at least and must be
  // used instead of raw Op mutation.
#define DeriveOp(Dest, NewOp, Expr) \
  if (ImplicitFlagClobber(NewOp))   \
    SaveNZCV(NewOp);                \
  auto Dest = (Expr);               \
  Dest.first->Header.Op = (NewOp)

  // Named constant cache for the current block.
  // Different arrays for sizes 1,2,4,8,16,32.
  OrderedNode *CachedNamedVectorConstants[FEXCore::IR::NamedVectorConstant::NAMED_VECTOR_MAX][6]{};
  struct IndexNamedVectorMapKey {
    uint32_t Index{};
    FEXCore::IR::IndexNamedVectorConstant NamedIndexedConstant;
    uint8_t log2_size_in_bytes{};
    uint16_t _pad{};

    bool operator==(const IndexNamedVectorMapKey&) const = default;
  };
  struct IndexNamedVectorMapKeyHasher {
    std::size_t operator()(const IndexNamedVectorMapKey& k) const noexcept {
      return XXH3_64bits(&k, sizeof(k));
    }
  };
  fextl::unordered_map<IndexNamedVectorMapKey, OrderedNode *, IndexNamedVectorMapKeyHasher> CachedIndexedNamedVectorConstants;

  // Load and cache a named vector constant.
  OrderedNode *LoadAndCacheNamedVectorConstant(uint8_t Size, FEXCore::IR::NamedVectorConstant NamedConstant) {
    auto log2_size_bytes = FEXCore::ilog2(Size);
    if (CachedNamedVectorConstants[NamedConstant][log2_size_bytes]) {
      return CachedNamedVectorConstants[NamedConstant][log2_size_bytes];
    }

    auto Constant = _LoadNamedVectorConstant(Size, NamedConstant);
    CachedNamedVectorConstants[NamedConstant][log2_size_bytes] = Constant;
    return Constant;
  }
  OrderedNode *LoadAndCacheIndexedNamedVectorConstant(uint8_t Size, FEXCore::IR::IndexNamedVectorConstant NamedIndexedConstant, uint32_t Index) {
    IndexNamedVectorMapKey Key {
      .Index = Index,
      .NamedIndexedConstant = NamedIndexedConstant,
      .log2_size_in_bytes = FEXCore::ilog2(Size),
    };
    auto it = CachedIndexedNamedVectorConstants.find(Key);

    if (it != CachedIndexedNamedVectorConstants.end()) {
      return it->second;
    }

    auto Constant = _LoadNamedVectorIndexedConstant(Size, NamedIndexedConstant, Index);
    CachedIndexedNamedVectorConstants.insert_or_assign(Key, Constant);
    return Constant;
  }

  // Reset the named vector constants cache array.
  // These are only cached per block.
  void ClearCachedNamedConstants() {
    memset(CachedNamedVectorConstants, 0, sizeof(CachedNamedVectorConstants));
    CachedIndexedNamedVectorConstants.clear();
  }

  OrderedNode *SelectMask(OrderedNode *Cmp, uint64_t Mask, bool Invert, IR::OpSize ResultSize, OrderedNode *TrueValue, OrderedNode *FalseValue);
  OrderedNode *SelectNZCV(unsigned BitOffset, bool Invert, IR::OpSize ResultSize, OrderedNode *TrueValue, OrderedNode *FalseValue);
  OrderedNode *SelectCC(uint8_t OP, IR::OpSize ResultSize, OrderedNode *TrueValue, OrderedNode *FalseValue);

  /**
   * @name Deferred RFLAG calculation and generation.
   *
   * Only handles the six flags that ALU ops typically generate.
   * Specifically: CF, PF, AF, ZF, SF, OF
   *  These six flags are heavily generated through basic ALU ops and balloon the IR if not early eliminated.
   *  This tracking structure only tracks single blocks and requires RFLAGS calculation at block-ending ops.
   *  Some flags generating ALU ops only touch part of the registers, In these cases it will do calculation up front.
   *  This means we still need our IR passes to eliminate all redundant flags accesses but this light OpcodeDispatcher optimization
   *  doesn't take it to that level.
   * @{ */

  // Deferred flag generation tracking structure.
  // This structure is used to track RFlags from ALU ops for invalidation.
  //
  // Future ideas: Use an invalidation mask to do partial generation of flags.
  // Particularly for the instructions that don't do the full set of flags calculations.
  // These instructions currently calculate the deferred RFLAGS immediately then overwrite rflags state.
  // RCLSE IR pass will catch and remove redundant rflags stores like this currently.
  struct DeferredFlagData {
    // What type of flags to generate
    FlagsGenerationType Type {FlagsGenerationType::TYPE_NONE};

    // Source size of the op
    uint8_t SrcSize;

    // Every flag generation type has a result
    OrderedNode *Res{};

    union {
      // UMUL, BEXTR, BLSI, BLSMSK, POPCOUNT, TZCNT, LZCNT, BITSELECT, RDRAND
      struct {
      } NoSource;

      // MUL, BLSR, BZHI
      struct {
        OrderedNode *Src1;
      } OneSource;

      // Logical, LSHL, LSHR, ASHR, ROR, ROL
      struct {
        OrderedNode *Src1;
        OrderedNode *Src2;
      } TwoSource;

      // ADC, SBB
      struct {
        OrderedNode *Src1;
        OrderedNode *Src2;
        OrderedNode *Src3;
      } ThreeSource;

      // LSHLI, LSHRI, ASHRI, RORI, ROLI
      struct {
        OrderedNode *Src1;
        uint64_t Imm;
      } OneSrcImmediate;

      // ADD, SUB
      struct {
        OrderedNode *Src1;
        OrderedNode *Src2;

        bool UpdateCF;
      } TwoSrcImmediate;
    } Sources{};
  };

  DeferredFlagData CurrentDeferredFlags{};

  /**
   * @brief Takes the current deferred flag state and stores the result in to RFLAGS.
   *
   * Once executed there will no longer be any deferred flag state and RFLAGS will have the correct flags in it.
   * Necessary to do when leaving a IR block, or if an instruction is doing a partial overwrite of the flags.
   */
  void CalculateDeferredFlags(uint32_t FlagsToCalculateMask = ~0U);

  /**
   * @brief Invalidates the current deferred flags structure.
   *
   * If the emulated instruction is going to overwrite all of the flags but isn't tracked using the deferred flag system
   * then use this function to stop tracking the current active deferred flags.
   */
  void InvalidateDeferredFlags() {
    CurrentDeferredFlags.Type = FlagsGenerationType::TYPE_NONE;

    // No NZCV bits will be set, they are all invalid.
    PossiblySetNZCVBits = 0;
  }

  /**
   * @brief Checks if there is any deferred flag state active.
   *
   * @return True if RFLAGs contains the flags. False if deferred flags is tracking the data.
   */
  bool IsDeferredFlagsStored() const {
    return CurrentDeferredFlags.Type == FlagsGenerationType::TYPE_NONE;
  }

  /**
   * @name These functions are used by the deferred flag handling while it is calculating and storing flags in to RFLAGs.
   * @{ */
  OrderedNode *LoadPFRaw();
  OrderedNode *LoadAF();
  void FixupAF();
  void CalculatePF(OrderedNode *Res, OrderedNode *condition = nullptr);
  void CalculateAF(OpSize OpSize, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2);

  void CalculateOF(uint8_t SrcSize, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2, bool Sub);
  void CalculateFlags_ADC(uint8_t SrcSize, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2, OrderedNode *CF);
  void CalculateFlags_SBB(uint8_t SrcSize, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2, OrderedNode *CF);
  void CalculateFlags_SUB(uint8_t SrcSize, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2, bool UpdateCF = true);
  void CalculateFlags_ADD(uint8_t SrcSize, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2, bool UpdateCF = true);
  void CalculateFlags_MUL(uint8_t SrcSize, OrderedNode *Res, OrderedNode *High);
  void CalculateFlags_UMUL(OrderedNode *High);
  void CalculateFlags_Logical(uint8_t SrcSize, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2);
  void CalculateFlags_ShiftLeft(uint8_t SrcSize, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2);
  void CalculateFlags_ShiftLeftImmediate(uint8_t SrcSize, OrderedNode *Res, OrderedNode *Src1, uint64_t Shift);
  void CalculateFlags_ShiftRight(uint8_t SrcSize, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2);
  void CalculateFlags_ShiftRightImmediate(uint8_t SrcSize, OrderedNode *Res, OrderedNode *Src1, uint64_t Shift);
  void CalculateFlags_ShiftRightDoubleImmediate(uint8_t SrcSize, OrderedNode *Res, OrderedNode *Src1, uint64_t Shift);
  void CalculateFlags_ShiftRightImmediateCommon(uint8_t SrcSize, OrderedNode *Res, OrderedNode *Src1, uint64_t Shift);
  void CalculateFlags_SignShiftRight(uint8_t SrcSize, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2);
  void CalculateFlags_SignShiftRightImmediate(uint8_t SrcSize, OrderedNode *Res, OrderedNode *Src1, uint64_t Shift);
  void CalculateFlags_RotateRight(uint8_t SrcSize, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2);
  void CalculateFlags_RotateLeft(uint8_t SrcSize, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2);
  void CalculateFlags_RotateRightImmediate(uint8_t SrcSize, OrderedNode *Res, OrderedNode *Src1, uint64_t Shift);
  void CalculateFlags_RotateLeftImmediate(uint8_t SrcSize, OrderedNode *Res, OrderedNode *Src1, uint64_t Shift);
  void CalculateFlags_FCMP(uint8_t SrcSize, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2);
  void CalculateFlags_BEXTR(OrderedNode *Src);
  void CalculateFlags_BLSI(uint8_t SrcSize, OrderedNode *Src);
  void CalculateFlags_BLSMSK(OrderedNode *Src);
  void CalculateFlags_BLSR(uint8_t SrcSize, OrderedNode *Res, OrderedNode *Src);
  void CalculateFlags_POPCOUNT(OrderedNode *Src);
  void CalculateFlags_BZHI(uint8_t SrcSize, OrderedNode *Result, OrderedNode *Src);
  void CalculateFlags_TZCNT(OrderedNode *Src);
  void CalculateFlags_LZCNT(uint8_t SrcSize, OrderedNode *Src);
  void CalculateFlags_BITSELECT(OrderedNode *Src);
  void CalculateFlags_RDRAND(OrderedNode *Src);
  /**  @} */

  /**
   * @name These functions generated deferred RFLAGs tracking.
   *
   * Depending on the operation it may force a RFLAGs calculation before storing the new deferred state.
   * @{ */
  void GenerateFlags_ADC(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2, OrderedNode *CF) {
    CurrentDeferredFlags = DeferredFlagData {
      .Type = FlagsGenerationType::TYPE_ADC,
      .SrcSize = GetSrcSize(Op),
      .Res = Res,
      .Sources = {
        .ThreeSource = {
          .Src1 = Src1,
          .Src2 = Src2,
          .Src3 = CF,
        },
      },
    };
  }

  void GenerateFlags_SBB(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2, OrderedNode *CF) {
    CurrentDeferredFlags = DeferredFlagData {
      .Type = FlagsGenerationType::TYPE_SBB,
      .SrcSize = GetSrcSize(Op),
      .Res = Res,
      .Sources = {
        .ThreeSource = {
          .Src1 = Src1,
          .Src2 = Src2,
          .Src3 = CF,
        },
      },
    };
  }

  void GenerateFlags_SUB(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2, bool UpdateCF = true) {
    if (!UpdateCF) {
      // If we aren't updating CF then we need to calculate flags. Invalidation mask would make this not required.
      CalculateDeferredFlags();
    }
    CurrentDeferredFlags = DeferredFlagData {
      .Type = FlagsGenerationType::TYPE_SUB,
      .SrcSize = GetSrcSize(Op),
      .Res = Res,
      .Sources = {
        .TwoSrcImmediate = {
          .Src1 = Src1,
          .Src2 = Src2,
          .UpdateCF = UpdateCF,
        },
      },
    };
  }

  void GenerateFlags_ADD(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2, bool UpdateCF = true) {
    if (!UpdateCF) {
      // If we aren't updating CF then we need to calculate flags. Invalidation mask would make this not required.
      CalculateDeferredFlags();
    }
    CurrentDeferredFlags = DeferredFlagData {
      .Type = FlagsGenerationType::TYPE_ADD,
      .SrcSize = GetSrcSize(Op),
      .Res = Res,
      .Sources = {
        .TwoSrcImmediate = {
          .Src1 = Src1,
          .Src2 = Src2,
          .UpdateCF = UpdateCF,
        },
      },
    };
  }

  void GenerateFlags_MUL(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Res, OrderedNode *High) {
    CurrentDeferredFlags = DeferredFlagData {
      .Type = FlagsGenerationType::TYPE_MUL,
      .SrcSize = GetSrcSize(Op),
      .Res = Res,
      .Sources = {
        .OneSource = {
          .Src1 = High,
        },
      },
    };
  }

  void GenerateFlags_UMUL(FEXCore::X86Tables::DecodedOp Op, OrderedNode *High) {
    CurrentDeferredFlags = DeferredFlagData {
      .Type = FlagsGenerationType::TYPE_UMUL,
      .SrcSize = GetSrcSize(Op),
      .Res = High,
    };
  }

  void GenerateFlags_Logical(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2) {
    CurrentDeferredFlags = DeferredFlagData {
      .Type = FlagsGenerationType::TYPE_LOGICAL,
      .SrcSize = GetSrcSize(Op),
      .Res = Res,
      .Sources = {
        .TwoSource = {
          .Src1 = Src1,
          .Src2 = Src2,
        },
      },
    };
  }

  void GenerateFlags_ShiftLeft(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2) {
    // Flags need to be used, generate incoming flags first.
    CalculateDeferredFlags();

    CurrentDeferredFlags = DeferredFlagData {
      .Type = FlagsGenerationType::TYPE_LSHL,
      .SrcSize = GetSrcSize(Op),
      .Res = Res,
      .Sources = {
        .TwoSource = {
          .Src1 = Src1,
          .Src2 = Src2,
        },
      },
    };
  }

  void GenerateFlags_ShiftRight(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2) {
    // Flags need to be used, generate incoming flags first.
    CalculateDeferredFlags();

    CurrentDeferredFlags = DeferredFlagData {
      .Type = FlagsGenerationType::TYPE_LSHR,
      .SrcSize = GetSrcSize(Op),
      .Res = Res,
      .Sources = {
        .TwoSource = {
          .Src1 = Src1,
          .Src2 = Src2,
        },
      },
    };
  }

  void GenerateFlags_SignShiftRight(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2) {
    // Flags need to be used, generate incoming flags first.
    CalculateDeferredFlags();

    CurrentDeferredFlags = DeferredFlagData {
      .Type = FlagsGenerationType::TYPE_ASHR,
      .SrcSize = GetSrcSize(Op),
      .Res = Res,
      .Sources = {
        .TwoSource = {
          .Src1 = Src1,
          .Src2 = Src2,
        },
      },
    };
  }

  void GenerateFlags_ShiftLeftImmediate(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Res, OrderedNode *Src1, uint64_t Shift) {
    // No flags changed if shift is zero.
    if (Shift == 0) return;

    CurrentDeferredFlags = DeferredFlagData {
      .Type = FlagsGenerationType::TYPE_LSHLI,
      .SrcSize = GetSrcSize(Op),
      .Res = Res,
      .Sources = {
        .OneSrcImmediate = {
          .Src1 = Src1,
          .Imm = Shift,
        },
      },
    };
  }

  void GenerateFlags_SignShiftRightImmediate(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Res, OrderedNode *Src1, uint64_t Shift) {
    // No flags changed if shift is zero.
    if (Shift == 0) return;

    CurrentDeferredFlags = DeferredFlagData {
      .Type = FlagsGenerationType::TYPE_ASHRI,
      .SrcSize = GetSrcSize(Op),
      .Res = Res,
      .Sources = {
        .OneSrcImmediate = {
          .Src1 = Src1,
          .Imm = Shift,
        },
      },
    };
  }

  void GenerateFlags_ShiftRightImmediate(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Res, OrderedNode *Src1, uint64_t Shift) {
    // No flags changed if shift is zero.
    if (Shift == 0) return;

    CurrentDeferredFlags = DeferredFlagData {
      .Type = FlagsGenerationType::TYPE_LSHRI,
      .SrcSize = GetSrcSize(Op),
      .Res = Res,
      .Sources = {
        .OneSrcImmediate = {
          .Src1 = Src1,
          .Imm = Shift,
        },
      },
    };
  }

  void GenerateFlags_ShiftRightDoubleImmediate(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Res, OrderedNode *Src1, uint64_t Shift) {
    // No flags changed if shift is zero.
    if (Shift == 0) return;

    CurrentDeferredFlags = DeferredFlagData {
      .Type = FlagsGenerationType::TYPE_LSHRDI,
      .SrcSize = GetSrcSize(Op),
      .Res = Res,
      .Sources = {
        .OneSrcImmediate = {
          .Src1 = Src1,
          .Imm = Shift,
        },
      },
    };
  }

  void GenerateFlags_RotateRight(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2) {
    // Doesn't set all the flags, needs to calculate.
    CalculateDeferredFlags();

    CurrentDeferredFlags = DeferredFlagData {
      .Type = FlagsGenerationType::TYPE_ROR,
      .SrcSize = GetSrcSize(Op),
      .Res = Res,
      .Sources = {
        .TwoSource = {
          .Src1 = Src1,
          .Src2 = Src2,
        },
      },
    };
  }

  void GenerateFlags_RotateLeft(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2) {
    // Doesn't set all the flags, needs to calculate.
    CalculateDeferredFlags();

    CurrentDeferredFlags = DeferredFlagData {
      .Type = FlagsGenerationType::TYPE_ROL,
      .SrcSize = GetSrcSize(Op),
      .Res = Res,
      .Sources = {
        .TwoSource = {
          .Src1 = Src1,
          .Src2 = Src2,
        },
      },
    };
  }

  void GenerateFlags_RotateRightImmediate(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Res, OrderedNode *Src1, uint64_t Shift) {
    if (Shift == 0) return;

    // Doesn't set all the flags, needs to calculate.
    CalculateDeferredFlags();

    CurrentDeferredFlags = DeferredFlagData {
      .Type = FlagsGenerationType::TYPE_RORI,
      .SrcSize = GetSrcSize(Op),
      .Res = Res,
      .Sources = {
        .OneSrcImmediate = {
          .Src1 = Src1,
          .Imm = Shift,
        },
      },
    };
  }

  void GenerateFlags_RotateLeftImmediate(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Res, OrderedNode *Src1, uint64_t Shift) {
    if (Shift == 0) return;

    // Doesn't set all the flags, needs to calculate.
    CalculateDeferredFlags();

    CurrentDeferredFlags = DeferredFlagData {
      .Type = FlagsGenerationType::TYPE_ROLI,
      .SrcSize = GetSrcSize(Op),
      .Res = Res,
      .Sources = {
        .OneSrcImmediate = {
          .Src1 = Src1,
          .Imm = Shift,
        },
      }
    };
  }

  void GenerateFlags_FCMP(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2) {
    CurrentDeferredFlags = DeferredFlagData {
      .Type = FlagsGenerationType::TYPE_FCMP,
      .SrcSize = GetSrcSize(Op),
      .Res = Res,
      .Sources = {
        .TwoSource = {
          .Src1 = Src1,
          .Src2 = Src2,
        },
      }
    };
  }

  void GenerateFlags_BEXTR(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Src) {
    CurrentDeferredFlags = DeferredFlagData {
      .Type = FlagsGenerationType::TYPE_BEXTR,
      .SrcSize = GetSrcSize(Op),
      .Res = Src,
    };
  }

  void GenerateFlags_BLSI(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Src) {
    CurrentDeferredFlags = DeferredFlagData {
      .Type = FlagsGenerationType::TYPE_BLSI,
      .SrcSize = GetSrcSize(Op),
      .Res = Src,
    };
  }

  void GenerateFlags_BLSMSK(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Src) {
    CurrentDeferredFlags = DeferredFlagData {
      .Type = FlagsGenerationType::TYPE_BLSMSK,
      .SrcSize = GetSrcSize(Op),
      .Res = Src,
    };
  }

  void GenerateFlags_BLSR(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Res, OrderedNode *Src) {
    CurrentDeferredFlags = DeferredFlagData {
      .Type = FlagsGenerationType::TYPE_BLSR,
      .SrcSize = GetSrcSize(Op),
      .Res = Res,
      .Sources = {
        .OneSource = {
          .Src1 = Src,
        },
      },
    };
  }

  void GenerateFlags_POPCOUNT(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Src) {
    CurrentDeferredFlags = DeferredFlagData {
      .Type = FlagsGenerationType::TYPE_POPCOUNT,
      .SrcSize = GetSrcSize(Op),
      .Res = Src,
    };
  }

  void GenerateFlags_BZHI(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Result, OrderedNode *Src) {
    CurrentDeferredFlags = DeferredFlagData {
      .Type = FlagsGenerationType::TYPE_BZHI,
      .SrcSize = GetSrcSize(Op),
      .Res = Result,
      .Sources = {
        .OneSource = {
          .Src1 = Src,
        },
      },
    };
  }

  void GenerateFlags_TZCNT(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Src) {
    CurrentDeferredFlags = DeferredFlagData {
      .Type = FlagsGenerationType::TYPE_TZCNT,
      .SrcSize = GetSrcSize(Op),
      .Res = Src,
    };
  }

  void GenerateFlags_LZCNT(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Src) {
    CurrentDeferredFlags = DeferredFlagData {
      .Type = FlagsGenerationType::TYPE_LZCNT,
      .SrcSize = GetSrcSize(Op),
      .Res = Src,
    };
  }

  void GenerateFlags_BITSELECT(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Src) {
    CurrentDeferredFlags = DeferredFlagData {
      .Type = FlagsGenerationType::TYPE_BITSELECT,
      .SrcSize = GetSrcSize(Op),
      .Res = Src,
    };
  }

  void GenerateFlags_RDRAND(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Src) {
    CurrentDeferredFlags = DeferredFlagData {
      .Type = FlagsGenerationType::TYPE_RDRAND,
      .SrcSize = GetSrcSize(Op),
      .Res = Src,
    };
  }

  /**  @} */
  /**  @} */

  OrderedNode * GetX87Top();
  void SetX87ValidTag(OrderedNode *Value, bool Valid);
  OrderedNode *GetX87ValidTag(OrderedNode *Value);
  OrderedNode *GetX87Tag(OrderedNode *Value, OrderedNode *AbridgedFTW);
  OrderedNode *GetX87Tag(OrderedNode *Value);
  void SetX87FTW(OrderedNode *FTW);
  OrderedNode *GetX87FTW();
  void SetX87Top(OrderedNode *Value);

  bool DestIsLockedMem(FEXCore::X86Tables::DecodedOp Op) const {
    return DestIsMem(Op) && (Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_LOCK) != 0;
  }

  bool DestIsMem(FEXCore::X86Tables::DecodedOp Op) const {
    return !Op->Dest.IsGPR();
  }

  void CreateJumpBlocks(fextl::vector<FEXCore::Frontend::Decoder::DecodedBlocks> const *Blocks);
  bool BlockSetRIP {false};

  bool Multiblock{};
  uint64_t Entry;

  OrderedNode* _StoreMemAutoTSO(FEXCore::IR::RegisterClassType Class, uint8_t Size, OrderedNode *Addr, OrderedNode *Value, uint8_t Align = 1) {
    if (CTX->IsAtomicTSOEnabled())
      return _StoreMemTSO(Class, Size, Value, Addr, Invalid(), Align, MEM_OFFSET_SXTX, 1);
    else
      return _StoreMem(Class, Size, Value, Addr, Invalid(), Align, MEM_OFFSET_SXTX, 1);
  }

  OrderedNode* _LoadMemAutoTSO(FEXCore::IR::RegisterClassType Class, uint8_t Size, OrderedNode *ssa0, uint8_t Align = 1) {
    if (CTX->IsAtomicTSOEnabled())
      return _LoadMemTSO(Class, Size, ssa0, Invalid(), Align, MEM_OFFSET_SXTX, 1);
    else
      return _LoadMem(Class, Size, ssa0, Invalid(), Align, MEM_OFFSET_SXTX, 1);
  }

  void InstallHostSpecificOpcodeHandlers();

  ///< Segment telemetry tracking
  uint32_t SegmentsNeedReadCheck{~0U};
  void CheckLegacySegmentWrite(OrderedNode *NewNode, uint32_t SegmentReg);
  void CheckLegacySegmentRead(OrderedNode *NewNode, uint32_t SegmentReg);
};

void InstallOpcodeHandlers(Context::OperatingMode Mode);

}
template <>
struct fmt::formatter<FEXCore::IR::OpDispatchBuilder::FlagsGenerationType> : fmt::formatter<int> {
  using Base = fmt::formatter<int>;

  // Pass-through the underlying value, so IDs can
  // be formatted like any integral value.
  template <typename FormatContext>
  auto format(const FEXCore::IR::OpDispatchBuilder::FlagsGenerationType& ID, FormatContext& ctx) {
    return Base::format(static_cast<int>(ID), ctx);
  }
};



