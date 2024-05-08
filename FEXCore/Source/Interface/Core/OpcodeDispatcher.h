// SPDX-License-Identifier: MIT
#pragma once

#include "Interface/Core/Frontend.h"
#include "Interface/Core/X86Tables/X86Tables.h"
#include "Interface/Context/Context.h"
#include "Interface/IR/IREmitter.h"

#include <FEXCore/Config/Config.h>
#include <FEXCore/Core/Context.h>
#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/IR/IR.h>

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

enum class BTAction {
  BTNone,
  BTClear,
  BTSet,
  BTComplement,
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

struct AddressMode {
  Ref Base {nullptr};
  Ref Index {nullptr};
  MemOffsetType IndexType = MEM_OFFSET_SXTX;
  uint8_t IndexScale = 1;
  int64_t Offset = 0;

  // Size in bytes for the address calculation. 8 for an arm64 hardware mode.
  uint8_t AddrSize;
  bool NonTSO;
};

class OpDispatchBuilder final : public IREmitter {
  friend class FEXCore::IR::Pass;
  friend class FEXCore::IR::PassManager;

public:
  enum class FlagsGenerationType : uint8_t {
    TYPE_NONE,
    TYPE_SUB,
    TYPE_MUL,
    TYPE_UMUL,
    TYPE_LOGICAL,
    TYPE_LSHLI,
    TYPE_LSHRI,
    TYPE_LSHRDI,
    TYPE_ASHRI,
    TYPE_BEXTR,
    TYPE_BLSI,
    TYPE_BLSMSK,
    TYPE_BLSR,
    TYPE_POPCOUNT,
    TYPE_BZHI,
    TYPE_ZCNT,
    TYPE_RDRAND,
  };

  Ref GetNewJumpBlock(uint64_t RIP) {
    auto it = JumpTargets.find(RIP);
    LOGMAN_THROW_A_FMT(it != JumpTargets.end(), "Couldn't find block generated for 0x{:x}", RIP);
    return it->second.BlockEntry;
  }

  void SetNewBlockIfChanged(uint64_t RIP) {
    auto it = JumpTargets.find(RIP);
    if (it == JumpTargets.end()) {
      return;
    }

    it->second.HaveEmitted = true;

    if (CurrentCodeBlock->Wrapped(DualListData.ListBegin()).ID() == it->second.BlockEntry->Wrapped(DualListData.ListBegin()).ID()) {
      return;
    }

    // We have hit a RIP that is a jump target
    // Thus we need to end up in a new block
    SetCurrentCodeBlock(it->second.BlockEntry);
  }

  void StartNewBlock() {
    // If we loaded flags but didn't change them, invalidate the cached copy and move on.
    // Changes get stored out by CalculateDeferredFlags.
    CachedNZCV = nullptr;
    PossiblySetNZCVBits = ~0U;
    FlushRegisterCache();

    // New block needs to reset segment telemetry.
    SegmentsNeedReadCheck = ~0U;

    // Need to clear any named constants that were cached.
    ClearCachedNamedConstants();
  }

  IRPair<IROp_Jump> Jump() {
    FlushRegisterCache();
    return _Jump();
  }
  IRPair<IROp_Jump> Jump(Ref _TargetBlock) {
    FlushRegisterCache();
    return _Jump(_TargetBlock);
  }
  IRPair<IROp_CondJump>
  CondJump(Ref _Cmp1, Ref _Cmp2, Ref _TrueBlock, Ref _FalseBlock, CondClassType _Cond = {COND_NEQ}, uint8_t _CompareSize = 0) {
    FlushRegisterCache();
    return _CondJump(_Cmp1, _Cmp2, _TrueBlock, _FalseBlock, _Cond, _CompareSize);
  }
  IRPair<IROp_CondJump> CondJump(Ref ssa0, CondClassType cond = {COND_NEQ}) {
    FlushRegisterCache();
    return _CondJump(ssa0, cond);
  }
  IRPair<IROp_CondJump> CondJump(Ref ssa0, Ref ssa1, Ref ssa2, CondClassType cond = {COND_NEQ}) {
    FlushRegisterCache();
    return _CondJump(ssa0, ssa1, ssa2, cond);
  }
  IRPair<IROp_CondJump> CondJumpNZCV(CondClassType Cond) {
    FlushRegisterCache();

    // The jump will ignore the sources, so it doesn't matter what we put here.
    // Put an inline constant so RA+codegen will ignore altogether.
    auto Placeholder = _InlineConstant(0);
    return _CondJump(Placeholder, Placeholder, InvalidNode, InvalidNode, Cond, 0, true);
  }
  IRPair<IROp_ExitFunction> ExitFunction(Ref NewRIP) {
    FlushRegisterCache();
    return _ExitFunction(NewRIP);
  }
  IRPair<IROp_Break> Break(BreakDefinition Reason) {
    FlushRegisterCache();
    return _Break(Reason);
  }
  IRPair<IROp_Thunk> Thunk(Ref ArgPtr, SHA256Sum ThunkNameHash) {
    FlushRegisterCache();
    return _Thunk(ArgPtr, ThunkNameHash);
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
      auto it = JumpTargets.find(NextRIP);
      if (it == JumpTargets.end()) {

        const uint8_t GPRSize = CTX->GetGPRSize();
        // If we don't have a jump target to a new block then we have to leave
        // Set the RIP to the next instruction and leave
        auto RelocatedNextRIP = _EntrypointOffset(IR::SizeToOpSize(GPRSize), NextRIP - Entry);
        ExitFunction(RelocatedNextRIP);
      } else if (it != JumpTargets.end()) {
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

  static bool CanHaveSideEffects(const FEXCore::X86Tables::X86InstInfo* TableInfo, FEXCore::X86Tables::DecodedOp Op) {
    if (TableInfo && TableInfo->Flags & X86Tables::InstFlags::FLAGS_DEBUG_MEM_ACCESS) {
      // If it is marked as having memory access then always say it has a side-effect.
      // Not always true but better to be safe.
      return true;
    }

    auto CanHaveSideEffects = false;

    auto HasPotentialMemoryAccess = [](const X86Tables::DecodedOperand& Operand) -> bool {
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

  template<typename F>
  void ForeachDirection(F&& Routine) {
    // Otherwise, prepare to branch.
    auto Zero = _Constant(0);

    // If the shift is zero, do not touch the flags.
    auto ForwardBlock = CreateNewCodeBlockAfter(GetCurrentBlock());
    auto BackwardBlock = CreateNewCodeBlockAfter(ForwardBlock);
    auto ExitBlock = CreateNewCodeBlockAfter(BackwardBlock);

    auto DF = GetRFLAG(X86State::RFLAG_DF_RAW_LOC);
    CondJump(DF, Zero, ForwardBlock, BackwardBlock, {COND_EQ});

    for (auto D = 0; D < 2; ++D) {
      SetCurrentCodeBlock(D ? BackwardBlock : ForwardBlock);
      StartNewBlock();
      {
        Routine(D ? -1 : 1);
        Jump(ExitBlock);
      }
    }

    SetCurrentCodeBlock(ExitBlock);
    StartNewBlock();
  }

  OpDispatchBuilder(FEXCore::Context::ContextImpl* ctx);
  OpDispatchBuilder(FEXCore::Utils::IntrusivePooledAllocator& Allocator);

  void ResetWorkingList();
  void ResetDecodeFailure() {
    NeedsBlockEnd = DecodeFailure = false;
  }
  bool HadDecodeFailure() const {
    return DecodeFailure;
  }
  bool NeedsBlockEnder() const {
    return NeedsBlockEnd;
  }

  void ResetHandledLock() {
    HandledLock = false;
  }
  bool HasHandledLock() const {
    return HandledLock;
  }

  void SetDumpIR(bool DumpIR) {
    ShouldDump = DumpIR;
  }
  bool ShouldDumpIR() const {
    return ShouldDump;
  }

  void BeginFunction(uint64_t RIP, const fextl::vector<FEXCore::Frontend::Decoder::DecodedBlocks>* Blocks, uint32_t NumInstructions);
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
  template<bool IsSyscallInst>
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
  template<uint32_t SrcIndex>
  void SBBOp(OpcodeArgs);
  void SALCOp(OpcodeArgs);
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
  uint32_t LoadConstantShift(X86Tables::DecodedOp Op, bool Is1Bit);
  void SHLOp(OpcodeArgs);
  template<bool SHL1Bit>
  void SHLImmediateOp(OpcodeArgs);
  void SHROp(OpcodeArgs);
  template<bool SHR1Bit>
  void SHRImmediateOp(OpcodeArgs);
  void SHLDOp(OpcodeArgs);
  void SHLDImmediateOp(OpcodeArgs);
  void SHRDOp(OpcodeArgs);
  void SHRDImmediateOp(OpcodeArgs);
  template<bool IsImmediate, bool Is1Bit>
  void ASHROp(OpcodeArgs);
  template<bool Left, bool IsImmediate, bool Is1Bit>
  void RotateOp(OpcodeArgs);
  void RCROp1Bit(OpcodeArgs);
  void RCROp8x1Bit(OpcodeArgs);
  void RCROp(OpcodeArgs);
  void RCRSmallerOp(OpcodeArgs);
  void RCLOp1Bit(OpcodeArgs);
  void RCLOp(OpcodeArgs);
  void RCLSmallerOp(OpcodeArgs);

  template<uint32_t SrcIndex, enum BTAction Action>
  void BTOp(OpcodeArgs);

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

  struct CycleCounterPair {
    Ref CounterLow;
    Ref CounterHigh;
  };
  CycleCounterPair CycleCounter();
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
  void SMSWOp(OpcodeArgs);

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

  template<size_t ElementSize>
  void PSIGN(OpcodeArgs);
  template<size_t ElementSize>
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
  template<IROps IROp, size_t ElementSize>
  void AVXVectorALUOp(OpcodeArgs);
  template<IROps IROp, size_t ElementSize>
  void AVXVectorUnaryOp(OpcodeArgs);

  template<size_t ElementSize>
  void AVXVectorRound(OpcodeArgs);

  template<size_t DstElementSize, size_t SrcElementSize>
  void AVXScalar_CVT_Float_To_Float(OpcodeArgs);

  template<size_t SrcElementSize, bool Narrow, bool HostRoundingMode>
  void AVXVector_CVT_Float_To_Int(OpcodeArgs);

  template<size_t SrcElementSize, bool Widen>
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
  template<size_t DstElementSize>
  void AVXInsertCVTGPR_To_FPR(OpcodeArgs);

  template<size_t DstElementSize, size_t SrcElementSize>
  void InsertScalar_CVT_Float_To_Float(OpcodeArgs);
  template<size_t DstElementSize, size_t SrcElementSize>
  void AVXInsertScalar_CVT_Float_To_Float(OpcodeArgs);

  RoundType TranslateRoundType(uint8_t Mode);

  template<size_t ElementSize>
  void InsertScalarRound(OpcodeArgs);
  template<size_t ElementSize>
  void AVXInsertScalarRound(OpcodeArgs);

  template<size_t ElementSize>
  void InsertScalarFCMPOp(OpcodeArgs);
  template<size_t ElementSize>
  void AVXInsertScalarFCMPOp(OpcodeArgs);

  template<size_t DstElementSize>
  void AVXCVTGPR_To_FPR(OpcodeArgs);

  template<size_t ElementSize>
  void AVXVFCMPOp(OpcodeArgs);

  template<size_t ElementSize>
  void VADDSUBPOp(OpcodeArgs);

  void VAESDecOp(OpcodeArgs);
  void VAESDecLastOp(OpcodeArgs);
  void VAESEncOp(OpcodeArgs);
  void VAESEncLastOp(OpcodeArgs);

  void VANDNOp(OpcodeArgs);

  Ref VBLENDOpImpl(uint32_t VecSize, uint32_t ElementSize, Ref Src1, Ref Src2, Ref ZeroRegister, uint64_t Selector);
  void VBLENDPDOp(OpcodeArgs);
  void VPBLENDDOp(OpcodeArgs);
  void VPBLENDWOp(OpcodeArgs);

  template<size_t ElementSize>
  void VBROADCASTOp(OpcodeArgs);

  template<size_t ElementSize>
  void VDPPOp(OpcodeArgs);

  void VEXTRACT128Op(OpcodeArgs);

  template<IROps IROp, size_t ElementSize>
  void VHADDPOp(OpcodeArgs);
  template<size_t ElementSize>
  void VHSUBPOp(OpcodeArgs);

  void VINSERTOp(OpcodeArgs);
  void VINSERTPSOp(OpcodeArgs);

  template<size_t ElementSize, bool IsStore>
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

  template<size_t ElementSize>
  void VPACKSSOp(OpcodeArgs);

  template<size_t ElementSize>
  void VPACKUSOp(OpcodeArgs);

  void VPALIGNROp(OpcodeArgs);

  void VPCMPESTRIOp(OpcodeArgs);
  void VPCMPESTRMOp(OpcodeArgs);
  void VPCMPISTRIOp(OpcodeArgs);
  void VPCMPISTRMOp(OpcodeArgs);

  void VCVTPH2PSOp(OpcodeArgs);

  Ref VPERMDIndices(OpSize DstSize, Ref Indices, Ref IndexMask, Ref Repeating3210);
  void VPERM2Op(OpcodeArgs);
  void VPERMDOp(OpcodeArgs);
  void VPERMQOp(OpcodeArgs);

  template<size_t ElementSize>
  void VPERMILImmOp(OpcodeArgs);

  Ref VPERMILRegOpImpl(OpSize DstSize, size_t ElementSize, Ref Src, Ref Indices);
  template<size_t ElementSize>
  void VPERMILRegOp(OpcodeArgs);

  void VPHADDSWOp(OpcodeArgs);

  template<size_t ElementSize>
  void VPHSUBOp(OpcodeArgs);
  void VPHSUBSWOp(OpcodeArgs);

  void VPINSRBOp(OpcodeArgs);
  void VPINSRDQOp(OpcodeArgs);
  void VPINSRWOp(OpcodeArgs);

  void VPMADDUBSWOp(OpcodeArgs);
  void VPMADDWDOp(OpcodeArgs);

  template<bool IsStore>
  void VPMASKMOVOp(OpcodeArgs);

  void VPMULHRSWOp(OpcodeArgs);

  template<bool Signed>
  void VPMULHWOp(OpcodeArgs);

  template<size_t ElementSize, bool Signed>
  void VPMULLOp(OpcodeArgs);

  void VPSADBWOp(OpcodeArgs);

  void VPSHUFBOp(OpcodeArgs);

  template<size_t ElementSize, bool Low>
  void VPSHUFWOp(OpcodeArgs);

  template<size_t ElementSize>
  void VPSLLOp(OpcodeArgs);
  void VPSLLDQOp(OpcodeArgs);
  template<size_t ElementSize>
  void VPSLLIOp(OpcodeArgs);
  void VPSLLVOp(OpcodeArgs);

  template<size_t ElementSize>
  void VPSRAOp(OpcodeArgs);

  template<size_t ElementSize>
  void VPSRAIOp(OpcodeArgs);

  void VPSRAVDOp(OpcodeArgs);
  void VPSRLVOp(OpcodeArgs);

  template<size_t ElementSize>
  void VPSRLDOp(OpcodeArgs);
  void VPSRLDQOp(OpcodeArgs);

  template<size_t ElementSize>
  void VPUNPCKHOp(OpcodeArgs);

  template<size_t ElementSize>
  void VPUNPCKLOp(OpcodeArgs);

  template<size_t ElementSize>
  void VPSRLIOp(OpcodeArgs);

  template<size_t ElementSize>
  void VSHUFOp(OpcodeArgs);

  template<size_t ElementSize>
  void VTESTPOp(OpcodeArgs);

  void VZEROOp(OpcodeArgs);

  // X87 Ops
  Ref ReconstructFSW();
  // Returns new x87 stack top from FSW.
  Ref ReconstructX87StateFromFSW(Ref FSW);
  template<size_t width>
  void FLD(OpcodeArgs);
  template<NamedVectorConstant constant>
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

  Ref XSaveBase(X86Tables::DecodedOp Op);
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
  void RDPIDOp(OpcodeArgs);

  template<bool ForStore, bool Stream, uint8_t Level>
  void Prefetch(OpcodeArgs);

  void PSADBW(OpcodeArgs);

  Ref BitwiseAtLeastTwo(Ref A, Ref B, Ref C);

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
  void PTestOpImpl(OpSize Size, Ref Dest, Ref Src);
  void PTestOp(OpcodeArgs);
  void PHMINPOSUWOp(OpcodeArgs);
  template<size_t ElementSize>
  void DPPOp(OpcodeArgs);

  void MPSADBWOp(OpcodeArgs);
  void PCLMULQDQOp(OpcodeArgs);
  void VPCLMULQDQOp(OpcodeArgs);

  void CRC32(OpcodeArgs);

  void BreakOp(OpcodeArgs, FEXCore::IR::BreakDefinition BreakDefinition);
  void UnimplementedOp(OpcodeArgs);
  void PermissionRestrictedOp(OpcodeArgs);

  // AVX 128-bit operations
  Ref AVX128_LoadXMMRegister(uint32_t XMM, bool High);
  void AVX128_StoreXMMRegister(uint32_t XMM, const Ref Src, bool High);

  struct RefPair {
    Ref Low, High;
  };

  RefPair AVX128_Zext(Ref R) {
    RefPair Pair;
    Pair.Low = R;
    Pair.High = LoadZeroVector(OpSize::i128Bit);
    return Pair;
  }

  RefPair AVX128_LoadSource_WithOpSize(const X86Tables::DecodedOp& Op, const X86Tables::DecodedOperand& Operand, uint32_t Flags,
                                       bool NeedsHigh, MemoryAccessType AccessType = MemoryAccessType::DEFAULT);

  void AVX128_StoreResult_WithOpSize(FEXCore::X86Tables::DecodedOp Op, const FEXCore::X86Tables::DecodedOperand& Operand, const RefPair Src,
                                     MemoryAccessType AccessType = MemoryAccessType::DEFAULT);
  void InstallAVX128Handlers();
  void AVX128_VMOVScalarImpl(OpcodeArgs, size_t ElementSize);
  void AVX128_VectorALUImpl(OpcodeArgs, IROps IROp, size_t ElementSize);
  void AVX128_VectorUnaryImpl(OpcodeArgs, IROps IROp, size_t ElementSize);
  void AVX128_VectorUnaryImpl(OpcodeArgs, size_t SrcSize, size_t ElementSize, std::function<Ref(size_t ElementSize, Ref Src)> Helper);
  void AVX128_VectorBinaryImpl(OpcodeArgs, size_t SrcSize, size_t ElementSize, std::function<Ref(size_t ElementSize, Ref Src1, Ref Src2)> Helper);
  void AVX128_VectorShiftWideImpl(OpcodeArgs, size_t ElementSize, IROps IROp);
  void AVX128_VectorShiftImmImpl(OpcodeArgs, size_t ElementSize, IROps IROp);
  void AVX128_VectorTrinaryImpl(OpcodeArgs, size_t SrcSize, size_t ElementSize, Ref Src3,
                                std::function<Ref(size_t ElementSize, Ref Src1, Ref Src2, Ref Src3)> Helper);

  enum class ShiftDirection { RIGHT, LEFT };
  void AVX128_ShiftDoubleImm(OpcodeArgs, ShiftDirection Dir);

  void AVX128_VMOVAPS(OpcodeArgs);
  void AVX128_VMOVSD(OpcodeArgs);
  void AVX128_VMOVSS(OpcodeArgs);

  template<IROps IROp, size_t ElementSize>
  void AVX128_VectorALU(OpcodeArgs);
  template<IROps IROp, size_t ElementSize>
  void AVX128_VectorUnary(OpcodeArgs);

  void AVX128_VZERO(OpcodeArgs);
  void AVX128_MOVVectorNT(OpcodeArgs);
  void AVX128_MOVQ(OpcodeArgs);
  void AVX128_VMOVLP(OpcodeArgs);
  void AVX128_VMOVHP(OpcodeArgs);
  void AVX128_VMOVDDUP(OpcodeArgs);
  void AVX128_VMOVSLDUP(OpcodeArgs);
  void AVX128_VMOVSHDUP(OpcodeArgs);
  template<size_t ElementSize>
  void AVX128_VBROADCAST(OpcodeArgs);
  template<size_t ElementSize>
  void AVX128_VPUNPCKL(OpcodeArgs);
  template<size_t ElementSize>
  void AVX128_VPUNPCKH(OpcodeArgs);
  void AVX128_MOVVectorUnaligned(OpcodeArgs);
  template<size_t DstElementSize>
  void AVX128_InsertCVTGPR_To_FPR(OpcodeArgs);
  template<size_t SrcElementSize, bool HostRoundingMode>
  void AVX128_CVTFPR_To_GPR(OpcodeArgs);
  void AVX128_VANDN(OpcodeArgs);
  template<size_t ElementSize>
  void AVX128_VPACKSS(OpcodeArgs);
  template<size_t ElementSize>
  void AVX128_VPACKUS(OpcodeArgs);
  Ref AVX128_PSIGNImpl(size_t ElementSize, Ref Src1, Ref Src2);
  template<size_t ElementSize>
  void AVX128_VPSIGN(OpcodeArgs);
  template<size_t ElementSize>
  void AVX128_UCOMISx(OpcodeArgs);
  template<FEXCore::IR::IROps IROp, size_t ElementSize>
  void AVX128_VectorScalarInsertALU(OpcodeArgs);
  Ref AVX128_VFCMPImpl(size_t ElementSize, Ref Src1, Ref Src2, uint8_t CompType);
  template<size_t ElementSize>
  void AVX128_VFCMP(OpcodeArgs);
  template<size_t ElementSize>
  void AVX128_InsertScalarFCMP(OpcodeArgs);
  void AVX128_MOVBetweenGPR_FPR(OpcodeArgs);
  template<size_t ElementSize>
  void AVX128_PExtr(OpcodeArgs);
  template<size_t ElementSize, size_t DstElementSize, bool Signed>
  void AVX128_ExtendVectorElements(OpcodeArgs);
  template<size_t ElementSize>
  void AVX128_MOVMSK(OpcodeArgs);
  void AVX128_MOVMSKB(OpcodeArgs);
  void AVX128_PINSRImpl(OpcodeArgs, size_t ElementSize, const X86Tables::DecodedOperand& Src1Op, const X86Tables::DecodedOperand& Src2Op,
                        const X86Tables::DecodedOperand& Imm);
  void AVX128_VPINSRB(OpcodeArgs);
  void AVX128_VPINSRW(OpcodeArgs);
  void AVX128_VPINSRDQ(OpcodeArgs);
  template<size_t ElementSize>
  void AVX128_VPSRA(OpcodeArgs);
  template<size_t ElementSize>
  void AVX128_VPSLL(OpcodeArgs);
  template<size_t ElementSize>
  void AVX128_VPSRL(OpcodeArgs);

  void AVX128_VariableShiftImpl(OpcodeArgs, IROps IROp);
  void AVX128_VPSLLV(OpcodeArgs);
  void AVX128_VPSRAVD(OpcodeArgs);
  void AVX128_VPSRLV(OpcodeArgs);

  template<size_t ElementSize>
  void AVX128_VPSRLI(OpcodeArgs);
  template<size_t ElementSize>
  void AVX128_VPSLLI(OpcodeArgs);
  template<size_t ElementSize>
  void AVX128_VPSRAI(OpcodeArgs);

  void AVX128_VPSRLDQ(OpcodeArgs);
  void AVX128_VPSLLDQ(OpcodeArgs);

  void AVX128_VINSERT(OpcodeArgs);
  void AVX128_VINSERTPS(OpcodeArgs);

  Ref AVX128_PHSUBImpl(Ref Src1, Ref Src2, size_t ElementSize);
  template<size_t ElementSize>
  void AVX128_VPHSUB(OpcodeArgs);

  void AVX128_VPHSUBSW(OpcodeArgs);

  template<size_t ElementSize>
  void AVX128_VADDSUBP(OpcodeArgs);

  template<size_t ElementSize, bool Signed>
  void AVX128_VPMULL(OpcodeArgs);

  void AVX128_VPMULHRSW(OpcodeArgs);

  template<bool Signed>
  void AVX128_VPMULHW(OpcodeArgs);

  template<size_t DstElementSize, size_t SrcElementSize>
  void AVX128_InsertScalar_CVT_Float_To_Float(OpcodeArgs);

  template<size_t DstElementSize, size_t SrcElementSize>
  void AVX128_Vector_CVT_Float_To_Float(OpcodeArgs);

  template<size_t SrcElementSize, bool Narrow, bool HostRoundingMode>
  void AVX128_Vector_CVT_Float_To_Int(OpcodeArgs);

  template<size_t SrcElementSize, bool Widen>
  void AVX128_Vector_CVT_Int_To_Float(OpcodeArgs);

  void AVX128_VEXTRACT128(OpcodeArgs);
  void AVX128_VAESImc(OpcodeArgs);
  void AVX128_VAESEnc(OpcodeArgs);
  void AVX128_VAESEncLast(OpcodeArgs);
  void AVX128_VAESDec(OpcodeArgs);
  void AVX128_VAESDecLast(OpcodeArgs);
  void AVX128_VAESKeyGenAssist(OpcodeArgs);

  void AVX128_VPCMPESTRI(OpcodeArgs);
  void AVX128_VPCMPESTRM(OpcodeArgs);
  void AVX128_VPCMPISTRI(OpcodeArgs);
  void AVX128_VPCMPISTRM(OpcodeArgs);

  void AVX128_PHMINPOSUW(OpcodeArgs);

  template<size_t ElementSize>
  void AVX128_VectorRound(OpcodeArgs);
  template<size_t ElementSize>
  void AVX128_InsertScalarRound(OpcodeArgs);

  template<size_t ElementSize>
  void AVX128_VDPP(OpcodeArgs);
  void AVX128_VPERMQ(OpcodeArgs);

  template<size_t ElementSize, bool Low>
  void AVX128_VPSHUF(OpcodeArgs);

  template<size_t ElementSize>
  void AVX128_VSHUF(OpcodeArgs);

  template<size_t ElementSize>
  void AVX128_VPERMILImm(OpcodeArgs);

  template<IROps IROp, size_t ElementSize>
  void AVX128_VHADDP(OpcodeArgs);

  void AVX128_VPHADDSW(OpcodeArgs);

  void AVX128_VPMADDUBSW(OpcodeArgs);
  void AVX128_VPMADDWD(OpcodeArgs);

  template<size_t ElementSize>
  void AVX128_VBLEND(OpcodeArgs);

  template<size_t ElementSize>
  void AVX128_VHSUBP(OpcodeArgs);

  void AVX128_VPSHUFB(OpcodeArgs);
  void AVX128_VPSADBW(OpcodeArgs);

  void AVX128_VMPSADBW(OpcodeArgs);
  void AVX128_VPALIGNR(OpcodeArgs);

  void AVX128_VMASKMOVImpl(OpcodeArgs, size_t ElementSize, size_t DstSize, bool IsStore, const X86Tables::DecodedOperand& MaskOp,
                           const X86Tables::DecodedOperand& DataOp);

  template<bool IsStore>
  void AVX128_VPMASKMOV(OpcodeArgs);

  template<size_t ElementSize, bool IsStore>
  void AVX128_VMASKMOV(OpcodeArgs);

  void AVX128_MASKMOV(OpcodeArgs);

  template<size_t ElementSize>
  void AVX128_VectorVariableBlend(OpcodeArgs);

  void AVX128_SaveAVXState(Ref MemBase);
  void AVX128_RestoreAVXState(Ref MemBase);
  void AVX128_DefaultAVXState();

  void AVX128_VPERM2(OpcodeArgs);
  template<size_t ElementSize>
  void AVX128_VTESTP(OpcodeArgs);
  void AVX128_PTest(OpcodeArgs);

  template<size_t ElementSize>
  void AVX128_VPERMILReg(OpcodeArgs);

  void AVX128_VPERMD(OpcodeArgs);

  void AVX128_VPCLMULQDQ(OpcodeArgs);

  // End of AVX 128-bit implementation
  void InvalidOp(OpcodeArgs);

  void SetPackedRFLAG(bool Lower8, Ref Src);
  Ref GetPackedRFLAG(uint32_t FlagsMask = ~0U);

  void SetMultiblock(bool _Multiblock) {
    Multiblock = _Multiblock;
  }

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
  void SaveNZCV(IROps Op = OP_DUMMY) override {
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
      if (CTX->HostFeatures.SupportsAFP) {
        return;
      }

      break;
    default: break;
    }

    // Invariant: When executing instructions that clobber NZCV, the flags must
    // be resident in a GPR, which is equivalent to CachedNZCV != nullptr. Get
    // the NZCV which fills the cache if necessary.
    if (CachedNZCV == nullptr) {
      GetNZCV();
    }

    // Assume we'll need a reload.
    NZCVDirty = true;
  }

private:
  struct JumpTargetInfo {
    Ref BlockEntry;
    bool HaveEmitted;
  };

  FEXCore::Context::ContextImpl* CTX {};

  constexpr static unsigned FullNZCVMask = (1U << FEXCore::X86State::RFLAG_CF_RAW_LOC) | (1U << FEXCore::X86State::RFLAG_ZF_RAW_LOC) |
                                           (1U << FEXCore::X86State::RFLAG_SF_RAW_LOC) | (1U << FEXCore::X86State::RFLAG_OF_RAW_LOC);

  static bool ContainsNZCV(unsigned BitMask) {
    return (BitMask & FullNZCVMask) != 0;
  }

  static bool IsNZCV(unsigned BitOffset) {
    return BitOffset < 32 && ContainsNZCV(1U << BitOffset);
  }

  Ref CachedNZCV {};
  bool NZCVDirty {};
  uint32_t PossiblySetNZCVBits {};

  fextl::map<uint64_t, JumpTargetInfo> JumpTargets;
  bool HandledLock {false};
  bool DecodeFailure {false};
  bool NeedsBlockEnd {false};
  // Used during new op bringup
  bool ShouldDump {false};

  using SaveStoreAVXStatePtr = void (OpDispatchBuilder::*)(Ref MemBase);
  using DefaultAVXStatePtr = void (OpDispatchBuilder::*)();
  SaveStoreAVXStatePtr SaveAVXStateFunc {&OpDispatchBuilder::SaveAVXState};
  SaveStoreAVXStatePtr RestoreAVXStateFunc {&OpDispatchBuilder::RestoreAVXState};
  DefaultAVXStatePtr DefaultAVXStateFunc {&OpDispatchBuilder::DefaultAVXState};

  void ALUOpImpl(OpcodeArgs, FEXCore::IR::IROps ALUIROp, FEXCore::IR::IROps AtomicFetchOp, unsigned SrcIdx);

  // Opcode helpers for generalizing behavior across VEX and non-VEX variants.

  Ref ADDSUBPOpImpl(OpSize Size, size_t ElementSize, Ref Src1, Ref Src2);

  void AVXVectorALUOpImpl(OpcodeArgs, IROps IROp, size_t ElementSize);
  void AVXVectorUnaryOpImpl(OpcodeArgs, IROps IROp, size_t ElementSize);

  template<size_t ElementSize>
  void AVXVectorVariableBlend(OpcodeArgs);

  void AVXVariableShiftImpl(OpcodeArgs, IROps IROp);

  Ref AESKeyGenAssistImpl(OpcodeArgs);

  Ref CVTGPR_To_FPRImpl(OpcodeArgs, size_t DstElementSize, const X86Tables::DecodedOperand& Src1Op, const X86Tables::DecodedOperand& Src2Op);

  Ref DPPOpImpl(size_t DstSize, Ref Src1, Ref Src2, uint8_t Mask, size_t ElementSize);

  Ref VDPPSOpImpl(OpcodeArgs, const X86Tables::DecodedOperand& Src1, const X86Tables::DecodedOperand& Src2, const X86Tables::DecodedOperand& Imm);

  Ref ExtendVectorElementsImpl(OpcodeArgs, size_t ElementSize, size_t DstElementSize, bool Signed);

  Ref HSUBPOpImpl(OpSize Size, size_t ElementSize, Ref Src1, Ref Src2);

  Ref InsertPSOpImpl(OpcodeArgs, const X86Tables::DecodedOperand& Src1, const X86Tables::DecodedOperand& Src2,
                     const X86Tables::DecodedOperand& Imm);

  Ref MPSADBWOpImpl(size_t SrcSize, Ref Src1, Ref Src2, uint8_t Select);

  Ref PALIGNROpImpl(OpcodeArgs, const X86Tables::DecodedOperand& Src1, const X86Tables::DecodedOperand& Src2,
                    const X86Tables::DecodedOperand& Imm, bool IsAVX);

  void PCMPXSTRXOpImpl(OpcodeArgs, bool IsExplicit, bool IsMask);

  Ref PHADDSOpImpl(OpSize Size, Ref Src1, Ref Src2);

  Ref PHMINPOSUWOpImpl(OpcodeArgs);

  Ref PHSUBOpImpl(OpSize Size, Ref Src1, Ref Src2, size_t ElementSize);

  Ref PHSUBSOpImpl(OpSize Size, Ref Src1, Ref Src2);

  Ref PINSROpImpl(OpcodeArgs, size_t ElementSize, const X86Tables::DecodedOperand& Src1Op, const X86Tables::DecodedOperand& Src2Op,
                  const X86Tables::DecodedOperand& Imm);

  Ref PMADDWDOpImpl(size_t Size, Ref Src1, Ref Src2);

  Ref PMADDUBSWOpImpl(size_t Size, Ref Src1, Ref Src2);

  Ref PMULHRSWOpImpl(OpSize Size, Ref Src1, Ref Src2);

  Ref PMULHWOpImpl(OpcodeArgs, bool Signed, Ref Src1, Ref Src2);

  Ref PMULLOpImpl(OpSize Size, size_t ElementSize, bool Signed, Ref Src1, Ref Src2);

  Ref PSADBWOpImpl(size_t Size, Ref Src1, Ref Src2);

  Ref PSHUFBOpImpl(uint8_t SrcSize, Ref Src1, Ref Src2);

  Ref PSIGNImpl(OpcodeArgs, size_t ElementSize, Ref Src1, Ref Src2);

  Ref PSLLIImpl(OpcodeArgs, size_t ElementSize, Ref Src, uint64_t Shift);

  Ref PSLLImpl(OpcodeArgs, size_t ElementSize, Ref Src, Ref ShiftVec);

  Ref PSRAOpImpl(OpcodeArgs, size_t ElementSize, Ref Src, Ref ShiftVec);

  Ref PSRLDOpImpl(OpcodeArgs, size_t ElementSize, Ref Src, Ref ShiftVec);

  Ref SHUFOpImpl(OpcodeArgs, size_t DstSize, size_t ElementSize, Ref Src1, Ref Src2, uint8_t Shuffle);

  void VMASKMOVOpImpl(OpcodeArgs, size_t ElementSize, size_t DataSize, bool IsStore, const X86Tables::DecodedOperand& MaskOp,
                      const X86Tables::DecodedOperand& DataOp);

  void MOVScalarOpImpl(OpcodeArgs, size_t ElementSize);
  void VMOVScalarOpImpl(OpcodeArgs, size_t ElementSize);

  Ref VFCMPOpImpl(OpcodeArgs, size_t ElementSize, Ref Src1, Ref Src2, uint8_t CompType);

  void VTESTOpImpl(OpSize SrcSize, size_t ElementSize, Ref Src1, Ref Src2);

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
  Ref VectorScalarInsertALUOpImpl(OpcodeArgs, IROps IROp, size_t DstSize, size_t ElementSize, const X86Tables::DecodedOperand& Src1Op,
                                  const X86Tables::DecodedOperand& Src2Op, bool ZeroUpperBits);

  Ref VectorScalarUnaryInsertALUOpImpl(OpcodeArgs, IROps IROp, size_t DstSize, size_t ElementSize, const X86Tables::DecodedOperand& Src1Op,
                                       const X86Tables::DecodedOperand& Src2Op, bool ZeroUpperBits);

  Ref InsertCVTGPR_To_FPRImpl(OpcodeArgs, size_t DstSize, size_t DstElementSize, const X86Tables::DecodedOperand& Src1Op,
                              const X86Tables::DecodedOperand& Src2Op, bool ZeroUpperBits);

  Ref InsertScalar_CVT_Float_To_FloatImpl(OpcodeArgs, size_t DstSize, size_t DstElementSize, size_t SrcElementSize,
                                          const X86Tables::DecodedOperand& Src1Op, const X86Tables::DecodedOperand& Src2Op, bool ZeroUpperBits);
  Ref InsertScalarRoundImpl(OpcodeArgs, size_t DstSize, size_t ElementSize, const X86Tables::DecodedOperand& Src1Op,
                            const X86Tables::DecodedOperand& Src2Op, uint64_t Mode, bool ZeroUpperBits);

  Ref InsertScalarFCMPOpImpl(OpSize Size, uint8_t OpDstSize, size_t ElementSize, Ref Src1, Ref Src2, uint8_t CompType, bool ZeroUpperBits);

  Ref VectorRoundImpl(OpSize Size, size_t ElementSize, Ref Src, uint64_t Mode);

  Ref Scalar_CVT_Float_To_FloatImpl(OpcodeArgs, size_t DstElementSize, size_t SrcElementSize, const X86Tables::DecodedOperand& Src1Op,
                                    const X86Tables::DecodedOperand& Src2Op);

  void Vector_CVT_Float_To_FloatImpl(OpcodeArgs, size_t DstElementSize, size_t SrcElementSize, bool IsAVX);

  Ref Vector_CVT_Float_To_IntImpl(OpcodeArgs, size_t SrcElementSize, bool Narrow, bool HostRoundingMode);

  Ref Vector_CVT_Int_To_FloatImpl(OpcodeArgs, size_t SrcElementSize, bool Widen);

  void XSaveOpImpl(OpcodeArgs);
  void SaveX87State(OpcodeArgs, Ref MemBase);
  void SaveSSEState(Ref MemBase);
  void SaveMXCSRState(Ref MemBase);
  void SaveAVXState(Ref MemBase);

  void XRstorOpImpl(OpcodeArgs);
  void RestoreX87State(Ref MemBase);
  void RestoreSSEState(Ref MemBase);
  void RestoreMXCSRState(Ref MXCSR);
  void RestoreAVXState(Ref MemBase);
  void DefaultX87State(OpcodeArgs);
  void DefaultSSEState();
  void DefaultAVXState();

  Ref GetMXCSR();

#undef OpcodeArgs

  Ref AppendSegmentOffset(Ref Value, uint32_t Flags, uint32_t DefaultPrefix = 0, bool Override = false);
  Ref GetSegment(uint32_t Flags, uint32_t DefaultPrefix = 0, bool Override = false);

  void UpdatePrefixFromSegment(Ref Segment, uint32_t SegmentReg);

  Ref LoadGPRRegister(uint32_t GPR, int8_t Size = -1, uint8_t Offset = 0, bool AllowUpperGarbage = false);
  Ref LoadXMMRegister(uint32_t XMM);
  void StoreGPRRegister(uint32_t GPR, const Ref Src, int8_t Size = -1, uint8_t Offset = 0);
  void StoreXMMRegister(uint32_t XMM, const Ref Src);

  Ref GetRelocatedPC(const FEXCore::X86Tables::DecodedOp& Op, int64_t Offset = 0);

  AddressMode AddSegmentToAddress(AddressMode A, uint32_t Flags);
  Ref LoadEffectiveAddress(AddressMode A, bool AllowUpperGarbage = false);
  AddressMode SelectAddressMode(AddressMode A, bool AtomicTSO, bool Vector, unsigned AccessSize);

  bool IsOperandMem(const X86Tables::DecodedOperand& Operand, bool Load) {
    // Literals are immediates as sources but memory addresses as destinations.
    return !(Load && Operand.IsLiteral()) && !Operand.IsGPR();
  }

  bool IsNonTSOReg(MemoryAccessType Access, uint8_t Reg) {
    return Access == MemoryAccessType::DEFAULT && Reg == X86State::REG_RSP;
  }

  AddressMode DecodeAddress(const X86Tables::DecodedOp& Op, const X86Tables::DecodedOperand& Operand, MemoryAccessType AccessType, bool IsLoad);

  Ref LoadSource(RegisterClassType Class, const X86Tables::DecodedOp& Op, const X86Tables::DecodedOperand& Operand, uint32_t Flags,
                 const LoadSourceOptions& Options = {});
  Ref LoadSource_WithOpSize(RegisterClassType Class, const X86Tables::DecodedOp& Op, const X86Tables::DecodedOperand& Operand,
                            uint8_t OpSize, uint32_t Flags, const LoadSourceOptions& Options = {});
  void StoreResult_WithOpSize(FEXCore::IR::RegisterClassType Class, FEXCore::X86Tables::DecodedOp Op,
                              const FEXCore::X86Tables::DecodedOperand& Operand, const Ref Src, uint8_t OpSize, int8_t Align,
                              MemoryAccessType AccessType = MemoryAccessType::DEFAULT);
  void StoreResult(FEXCore::IR::RegisterClassType Class, FEXCore::X86Tables::DecodedOp Op, const FEXCore::X86Tables::DecodedOperand& Operand,
                   const Ref Src, int8_t Align, MemoryAccessType AccessType = MemoryAccessType::DEFAULT);
  void StoreResult(FEXCore::IR::RegisterClassType Class, FEXCore::X86Tables::DecodedOp Op, const Ref Src, int8_t Align,
                   MemoryAccessType AccessType = MemoryAccessType::DEFAULT);

  // In several instances, it's desirable to get a base address with the segment offset
  // applied to it. This pulls all the common-case appending into a single set of functions.
  [[nodiscard]]
  Ref MakeSegmentAddress(const X86Tables::DecodedOp& Op, const X86Tables::DecodedOperand& Operand, uint8_t OpSize) {
    Ref Mem = LoadSource_WithOpSize(GPRClass, Op, Operand, OpSize, Op->Flags, {.LoadData = false});
    return AppendSegmentOffset(Mem, Op->Flags);
  }
  [[nodiscard]]
  Ref MakeSegmentAddress(const X86Tables::DecodedOp& Op, const X86Tables::DecodedOperand& Operand) {
    return MakeSegmentAddress(Op, Operand, GetSrcSize(Op));
  }
  [[nodiscard]]
  Ref MakeSegmentAddress(X86State::X86Reg Reg, uint32_t Flags, uint32_t DefaultPrefix = 0, bool Override = false) {
    Ref Address = LoadGPRRegister(Reg);
    return AppendSegmentOffset(Address, Flags, DefaultPrefix, Override);
  }

  constexpr OpSize GetGuestVectorLength() const {
    return CTX->HostFeatures.SupportsSVE256 ? OpSize::i256Bit : OpSize::i128Bit;
  }

  [[nodiscard]]
  static uint32_t GPROffset(X86State::X86Reg reg) {
    LOGMAN_THROW_AA_FMT(reg <= X86State::X86Reg::REG_R15, "Invalid reg used");
    return static_cast<uint32_t>(offsetof(Core::CPUState, gregs[static_cast<size_t>(reg)]));
  }

  [[nodiscard]]
  static uint32_t MMBaseOffset() {
    return static_cast<uint32_t>(offsetof(Core::CPUState, mm[0][0]));
  }

  [[nodiscard]]
  uint8_t GetDstSize(X86Tables::DecodedOp Op) const;
  [[nodiscard]]
  uint8_t GetSrcSize(X86Tables::DecodedOp Op) const;
  [[nodiscard]]
  uint32_t GetDstBitSize(X86Tables::DecodedOp Op) const;
  [[nodiscard]]
  uint32_t GetSrcBitSize(X86Tables::DecodedOp Op) const;
  [[nodiscard]]
  IR::OpSize OpSizeFromDst(X86Tables::DecodedOp Op) const {
    return IR::SizeToOpSize(GetDstSize(Op));
  }
  [[nodiscard]]
  IR::OpSize OpSizeFromSrc(X86Tables::DecodedOp Op) const {
    return IR::SizeToOpSize(GetSrcSize(Op));
  }

  static inline constexpr unsigned NZCVIndexMask(unsigned BitMask) {
    unsigned NZCVMask {};
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

  // Set flag tracking to prepare for an operation that directly writes NZCV. If
  // some bits are known to be zeroed, the PossiblySetNZCVBits mask can be
  // passed. Otherwise, it defaults to assuming all bits may be set after
  // (this is conservative).
  void HandleNZCVWrite(uint32_t _PossiblySetNZCVBits = ~0) {
    InvalidateDeferredFlags();
    CachedNZCV = nullptr;
    PossiblySetNZCVBits = _PossiblySetNZCVBits;
    NZCVDirty = false;
  }

  // Set flag tracking to prepare for a read-modify-write operation on NZCV.
  void HandleNZCV_RMW(uint32_t _PossiblySetNZCVBits = ~0) {
    CalculateDeferredFlags();

    if (NZCVDirty && CachedNZCV) {
      _StoreNZCV(CachedNZCV);
    }

    HandleNZCVWrite(_PossiblySetNZCVBits);
  }

  // Special case of the above where we are known to zero C/V
  void HandleNZ00Write() {
    HandleNZCVWrite((1u << 31) | (1u << 30));
  }

  Ref GetNZCV() {
    if (!CachedNZCV) {
      CachedNZCV = _LoadNZCV();
    }

    return CachedNZCV;
  }

  void SetNZCV(Ref Value) {
    CachedNZCV = Value;
    NZCVDirty = true;
  }

  void ZeroNZCV() {
    CachedNZCV = _Constant(0);
    PossiblySetNZCVBits = 0;
    NZCVDirty = true;
  }

  void SetNZ_ZeroCV(unsigned SrcSize, Ref Res) {
    HandleNZ00Write();
    _TestNZ(IR::SizeToOpSize(SrcSize), Res, Res);
  }

  void InsertNZCV(unsigned BitOffset, Ref Value, signed FlagOffset, bool MustMask) {
    signed Bit = IndexNZCV(BitOffset);

    // If NZCV is not dirty, we always want to use rmif, it's 1 instruction to
    // implement this. But if NZCV is dirty, it might still be cheaper to copy
    // the GPR flags to NZCV and rmif. This is a heuristic for cases where we
    // expect that 2 instruction sequence to be a win (versus something like
    // bfe+mov+bfi+mov which can happen with our RA..). It's not totally
    // conservative but it's pretty good in practice.
    bool PreferRmif = !NZCVDirty || FlagOffset || MustMask || (PossiblySetNZCVBits & (1u << Bit));

    if (CTX->HostFeatures.SupportsFlagM && PreferRmif) {
      // Update NZCV
      if (NZCVDirty && CachedNZCV) {
        _StoreNZCV(CachedNZCV);
      }

      CachedNZCV = nullptr;
      NZCVDirty = false;

      // Insert as NZCV.
      signed RmifBit = Bit - 28;
      _RmifNZCV(Value, (64 + FlagOffset - RmifBit) % 64, 1u << RmifBit);
      CachedNZCV = nullptr;
    } else {
      // Insert as GPR
      if (FlagOffset || MustMask) {
        Value = _Bfe(OpSize::i64Bit, 1, FlagOffset, Value);
      }

      if (PossiblySetNZCVBits == 0) {
        SetNZCV(_Lshl(OpSize::i64Bit, Value, _Constant(Bit)));
      } else if ((PossiblySetNZCVBits & (1u << Bit)) == 0) {
        SetNZCV(_Orlshl(OpSize::i32Bit, GetNZCV(), Value, Bit));
      } else {
        SetNZCV(_Bfi(OpSize::i32Bit, 1, Bit, GetNZCV(), Value));
      }
    }

    PossiblySetNZCVBits |= (1u << Bit);
  }

  void CarryInvert() {
    unsigned Bit = IndexNZCV(FEXCore::X86State::RFLAG_CF_RAW_LOC);

    if (CTX->HostFeatures.SupportsFlagM && !NZCVDirty) {
      // Invert as NZCV.
      _CarryInvert();
      CachedNZCV = nullptr;
    } else {
      // Invert as a GPR
      SetNZCV(_Xor(OpSize::i32Bit, GetNZCV(), _Constant(1u << Bit)));
    }

    PossiblySetNZCVBits |= 1u << Bit;
  }

  template<unsigned BitOffset>
  void SetRFLAG(Ref Value, unsigned ValueOffset = 0, bool MustMask = false) {
    SetRFLAG(Value, BitOffset, ValueOffset, MustMask);
  }

  void SetRFLAG(Ref Value, unsigned BitOffset, unsigned ValueOffset = 0, bool MustMask = false) {
    if (IsNZCV(BitOffset)) {
      InsertNZCV(BitOffset, Value, ValueOffset, MustMask);
    } else if (BitOffset == FEXCore::X86State::RFLAG_PF_RAW_LOC) {
      _StoreRegister(Value, Core::CPUState::PF_AS_GREG, GPRClass, CTX->GetGPRSize());
    } else if (BitOffset == FEXCore::X86State::RFLAG_AF_RAW_LOC) {
      _StoreRegister(Value, Core::CPUState::AF_AS_GREG, GPRClass, CTX->GetGPRSize());
    } else {
      if (ValueOffset || MustMask) {
        Value = _Bfe(OpSize::i32Bit, 1, ValueOffset, Value);
      }

      // For DF, we need to transform 0/1 into 1/-1
      if (BitOffset == FEXCore::X86State::RFLAG_DF_RAW_LOC) {
        Value = _SubShift(OpSize::i64Bit, _Constant(1), Value, ShiftType::LSL, 1);
      }

      _StoreFlag(Value, BitOffset);
    }
  }

  void SetAF(unsigned Constant) {
    // AF is stored in bit 4 of the AF flag byte, with garbage in the other
    // bits. This allows us to defer the extract in the usual case. When it is
    // read, bit 4 is extracted.  In order to write a constant value of AF, that
    // means we need to left-shift here to compensate.
    SetRFLAG<FEXCore::X86State::RFLAG_AF_RAW_LOC>(_Constant(Constant << 4));
  }

  void ZeroPF_AF();

  void InvalidateAF() {
    _InvalidateFlags((1u << X86State::RFLAG_AF_RAW_LOC));
  }

  void InvalidatePF_AF() {
    _InvalidateFlags((1u << X86State::RFLAG_PF_RAW_LOC) | (1u << X86State::RFLAG_AF_RAW_LOC));
  }

  CondClassType CondForNZCVBit(unsigned BitOffset, bool Invert) {
    switch (BitOffset) {
    case FEXCore::X86State::RFLAG_SF_RAW_LOC: return Invert ? CondClassType {COND_PL} : CondClassType {COND_MI};

    case FEXCore::X86State::RFLAG_ZF_RAW_LOC: return Invert ? CondClassType {COND_NEQ} : CondClassType {COND_EQ};

    case FEXCore::X86State::RFLAG_CF_RAW_LOC: return Invert ? CondClassType {COND_ULT} : CondClassType {COND_UGE};

    case FEXCore::X86State::RFLAG_OF_RAW_LOC: return Invert ? CondClassType {COND_FNU} : CondClassType {COND_FU};

    default: FEX_UNREACHABLE;
    }
  }

  void FlushRegisterCache() {
    CalculateDeferredFlags();
  }

  Ref GetRFLAG(unsigned BitOffset, bool Invert = false) {
    if (IsNZCV(BitOffset)) {
      if (!(PossiblySetNZCVBits & (1u << IndexNZCV(BitOffset)))) {
        return _Constant(Invert ? 1 : 0);
      } else if (NZCVDirty) {
        auto Value = _Bfe(OpSize::i32Bit, 1, IndexNZCV(BitOffset), GetNZCV());

        if (Invert) {
          return _Xor(OpSize::i32Bit, Value, _Constant(1));
        } else {
          return Value;
        }
      } else {
        return _NZCVSelect(OpSize::i32Bit, CondForNZCVBit(BitOffset, Invert), _Constant(1), _Constant(0));
      }
    } else if (BitOffset == FEXCore::X86State::RFLAG_PF_RAW_LOC) {
      return _LoadRegister(Core::CPUState::PF_AS_GREG, GPRClass, CTX->GetGPRSize());
    } else if (BitOffset == FEXCore::X86State::RFLAG_AF_RAW_LOC) {
      return _LoadRegister(Core::CPUState::AF_AS_GREG, GPRClass, CTX->GetGPRSize());
    } else if (BitOffset == FEXCore::X86State::RFLAG_DF_RAW_LOC) {
      // Recover the sign bit, it is the logical DF value
      return _Lshr(OpSize::i64Bit, _LoadDF(), _Constant(63));
    } else {
      return _LoadFlag(BitOffset);
    }
  }

  // Returns (DF ? -Size : Size)
  Ref LoadDir(const unsigned Size) {
    auto Dir = _LoadDF();
    auto Shift = FEXCore::ilog2(Size);

    if (Shift) {
      return _Lshl(IR::SizeToOpSize(CTX->GetGPRSize()), Dir, _Constant(Shift));
    } else {
      return Dir;
    }
  }

  // Returns DF ? (X - Size) : (X + Size)
  Ref OffsetByDir(Ref X, const unsigned Size) {
    auto Shift = FEXCore::ilog2(Size);

    return _AddShift(OpSize::i64Bit, X, _LoadDF(), ShiftType::LSL, Shift);
  }

  // Compares two floats and sets flags for a COMISS instruction
  void Comiss(size_t ElementSize, Ref Src1, Ref Src2, bool InvalidateAF = false) {
    // First, set flags according to Arm FCMP.
    HandleNZCVWrite();
    _FCmp(ElementSize, Src1, Src2);

    // Now set COMISS flags by converts NZCV from the Arm representation to an
    // eXternal representation that's totally not a euphemism for x86, nuh-uh.
    if (CTX->HostFeatures.SupportsFlagM2) {
      LOGMAN_THROW_A_FMT(!NZCVDirty, "only expected after fcmp");

      // We need to set PF according to the unordered flag. We'd rather do this
      // after axflag, since some impls fuse fcmp+axflag, so we want to do this
      // after. We can recover "unordered" after axflag as (Z && !C), but
      // there's no condition code for this so it would take 2 instructions
      // instead of one, which seems worse than doing 1 op before and breaking
      // the fusion.
      //
      // We set PF to unordered (V), but our PF representation is inverted so we
      // actually set to !V. This is one instruction with the VC cond code.
      Ref PFInvert = _NZCVSelect(OpSize::i32Bit, CondClassType {COND_FNU}, _Constant(1), _Constant(0));

      SetRFLAG<FEXCore::X86State::RFLAG_PF_RAW_LOC>(PFInvert);

      // For the rest, this one weird a64 instruction maps exactly to what x86
      // needs. What a coincidence!
      _AXFlag();
      PossiblySetNZCVBits = ~0;

      // It does assume we invert CF internally, which is still TODO for us. For
      // now, add a cfinv to deal. Hopefully we delete this later.
      CarryInvert();
    } else {
      Ref Z = GetRFLAG(FEXCore::X86State::RFLAG_ZF_RAW_LOC);
      Ref C_inv = GetRFLAG(FEXCore::X86State::RFLAG_CF_RAW_LOC, true);
      Ref V = GetRFLAG(FEXCore::X86State::RFLAG_OF_RAW_LOC);

      // We want to zero SF/OF, and then set CF/ZF. Zeroing up front lets us do
      // this all with shifted-or's on non-flagm platforms.
      ZeroNZCV();

      SetRFLAG<FEXCore::X86State::RFLAG_CF_RAW_LOC>(_Or(OpSize::i32Bit, C_inv, V));
      SetRFLAG<FEXCore::X86State::RFLAG_ZF_RAW_LOC>(_Or(OpSize::i32Bit, Z, V));

      // Note that we store PF inverted.
      SetRFLAG<FEXCore::X86State::RFLAG_PF_RAW_LOC>(_Xor(OpSize::i32Bit, V, _Constant(1)));
    }

    if (!InvalidateAF) {
      // Zero AF. Note that the comparison sets the raw PF to 0/1 above, so
      // PF[4] is 0 so the XOR with PF will have no effect, so setting the AF
      // byte to zero will indeed zero AF as intended.
      SetRFLAG<FEXCore::X86State::RFLAG_AF_RAW_LOC>(_Constant(0));
    }
  }

  // Set x87 comparison flags based on the result set by Arm FCMP. Clobbers
  // NZCV on flagm2 platforms.
  void ConvertNZCVToX87() {
    Ref V = GetRFLAG(FEXCore::X86State::RFLAG_OF_RAW_LOC);

    if (CTX->HostFeatures.SupportsFlagM2) {
      LOGMAN_THROW_A_FMT(!NZCVDirty, "only expected after fcmp");

      // Convert to x86 flags, saves us from or'ing after.
      _AXFlag();
      PossiblySetNZCVBits = ~0;

      // Copy the values. CF is inverted from the axflag result, ZF is as-is.
      SetRFLAG<FEXCore::X86State::X87FLAG_C0_LOC>(GetRFLAG(FEXCore::X86State::RFLAG_CF_RAW_LOC, true));
      SetRFLAG<FEXCore::X86State::X87FLAG_C3_LOC>(GetRFLAG(FEXCore::X86State::RFLAG_ZF_RAW_LOC));
    } else {
      Ref Z = GetRFLAG(FEXCore::X86State::RFLAG_ZF_RAW_LOC);
      Ref N = GetRFLAG(FEXCore::X86State::RFLAG_SF_RAW_LOC);

      SetRFLAG<FEXCore::X86State::X87FLAG_C0_LOC>(_Or(OpSize::i32Bit, N, V));
      SetRFLAG<FEXCore::X86State::X87FLAG_C3_LOC>(_Or(OpSize::i32Bit, Z, V));
    }

    SetRFLAG<FEXCore::X86State::X87FLAG_C1_LOC>(_Constant(0));
    SetRFLAG<FEXCore::X86State::X87FLAG_C2_LOC>(V);
  }

  // Helper to store a variable shift and calculate its flags for a variable
  // shift, with correct PF handling.
  void HandleShift(X86Tables::DecodedOp Op, Ref Result, Ref Dest, ShiftType Shift, Ref Src) {

    auto OldPF = GetRFLAG(X86State::RFLAG_PF_RAW_LOC);

    HandleNZCV_RMW();
    CalculatePF(_ShiftFlags(OpSizeFromSrc(Op), Result, Dest, Shift, Src, OldPF));
    StoreResult(GPRClass, Op, Result, -1);
  }

  std::pair<Ref, Ref> ExtractPair(OpSize Size, Ref Pair) {
    // Extract high first. This is a hack to improve coalescing.
    Ref Hi = _ExtractElementPair(Size, Pair, 1);
    Ref Lo = _ExtractElementPair(Size, Pair, 0);

    return std::make_pair(Lo, Hi);
  }

  // Helper to derive Dest by a given builder-using Expression with the opcode
  // replaced with NewOp. Useful for generic building code. Not safe in general.
  // but does the right handling of ImplicitFlagClobber at least and must be
  // used instead of raw Op mutation.
#define DeriveOp(Dest, NewOp, Expr)                \
  if (ImplicitFlagClobber(NewOp)) SaveNZCV(NewOp); \
  auto Dest = (Expr);                              \
  Dest.first->Header.Op = (NewOp)

  // Named constant cache for the current block.
  // Different arrays for sizes 1,2,4,8,16,32.
  Ref CachedNamedVectorConstants[FEXCore::IR::NamedVectorConstant::NAMED_VECTOR_MAX][6] {};
  struct IndexNamedVectorMapKey {
    uint32_t Index {};
    FEXCore::IR::IndexNamedVectorConstant NamedIndexedConstant;
    uint8_t log2_size_in_bytes {};
    uint16_t _pad {};

    bool operator==(const IndexNamedVectorMapKey&) const = default;
  };
  struct IndexNamedVectorMapKeyHasher {
    std::size_t operator()(const IndexNamedVectorMapKey& k) const noexcept {
      return XXH3_64bits(&k, sizeof(k));
    }
  };
  fextl::unordered_map<IndexNamedVectorMapKey, Ref, IndexNamedVectorMapKeyHasher> CachedIndexedNamedVectorConstants;

  // Load and cache a named vector constant.
  Ref LoadAndCacheNamedVectorConstant(uint8_t Size, FEXCore::IR::NamedVectorConstant NamedConstant) {
    auto log2_size_bytes = FEXCore::ilog2(Size);
    if (CachedNamedVectorConstants[NamedConstant][log2_size_bytes]) {
      return CachedNamedVectorConstants[NamedConstant][log2_size_bytes];
    }

    auto Constant = _LoadNamedVectorConstant(Size, NamedConstant);
    CachedNamedVectorConstants[NamedConstant][log2_size_bytes] = Constant;
    return Constant;
  }
  Ref LoadAndCacheIndexedNamedVectorConstant(uint8_t Size, FEXCore::IR::IndexNamedVectorConstant NamedIndexedConstant, uint32_t Index) {
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

  Ref LoadUncachedZeroVector(uint8_t Size) {
    return _LoadNamedVectorConstant(Size, IR::NamedVectorConstant::NAMED_VECTOR_ZERO);
  }

  Ref LoadZeroVector(uint8_t Size) {
    return LoadAndCacheNamedVectorConstant(Size, IR::NamedVectorConstant::NAMED_VECTOR_ZERO);
  }

  // Reset the named vector constants cache array.
  // These are only cached per block.
  void ClearCachedNamedConstants() {
    memset(CachedNamedVectorConstants, 0, sizeof(CachedNamedVectorConstants));
    CachedIndexedNamedVectorConstants.clear();
  }

  std::pair<bool, CondClassType> DecodeNZCVCondition(uint8_t OP) const;
  Ref SelectBit(Ref Cmp, IR::OpSize ResultSize, Ref TrueValue, Ref FalseValue);
  Ref SelectCC(uint8_t OP, IR::OpSize ResultSize, Ref TrueValue, Ref FalseValue);

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
    Ref Res {};

    union {
      // UMUL, BEXTR, BLSI, POPCOUNT, ZCNT, RDRAND
      struct {
      } NoSource;

      // MUL, BLSR, BLSMSKB, BZHI
      struct {
        Ref Src1;
      } OneSource;

      // Logical
      struct {
        Ref Src1;
        Ref Src2;
      } TwoSource;

      // LSHLI, LSHRI, ASHRI
      struct {
        Ref Src1;
        uint64_t Imm;
      } OneSrcImmediate;

      // ADD, SUB
      struct {
        Ref Src1;
        Ref Src2;

        bool UpdateCF;
      } TwoSrcImmediate;
    } Sources {};
  };

  DeferredFlagData CurrentDeferredFlags {};

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

  template<typename F>
  void Calculate_ShiftVariable(Ref Shift, F&& Calculate) {
    // RCR can call this with constants, so handle that without branching.
    uint64_t Const;
    if (IsValueConstant(WrapNode(Shift), &Const)) {
      if (Const) {
        Calculate();
      }

      return;
    }

    // Otherwise, prepare to branch.
    uint32_t OldSetNZCVBits = PossiblySetNZCVBits;
    auto Zero = _Constant(0);

    // If the shift is zero, do not touch the flags.
    auto SetBlock = CreateNewCodeBlockAfter(GetCurrentBlock());
    auto EndBlock = CreateNewCodeBlockAfter(SetBlock);
    CondJump(Shift, Zero, EndBlock, SetBlock, {COND_EQ});

    SetCurrentCodeBlock(SetBlock);
    StartNewBlock();
    {
      Calculate();
      Jump(EndBlock);
    }

    SetCurrentCodeBlock(EndBlock);
    StartNewBlock();
    PossiblySetNZCVBits |= OldSetNZCVBits;
  }

  /**
   * @name These functions are used by the deferred flag handling while it is calculating and storing flags in to RFLAGs.
   * @{ */
  Ref LoadPFRaw(bool Invert);
  Ref LoadAF();
  void FixupAF();
  void SetAFAndFixup(Ref AF);
  Ref CalculateAFForDecimal(Ref A);
  void CalculatePF(Ref Res);
  void CalculateAF(Ref Src1, Ref Src2);

  void CalculateOF(uint8_t SrcSize, Ref Res, Ref Src1, Ref Src2, bool Sub);
  Ref CalculateFlags_ADC(uint8_t SrcSize, Ref Src1, Ref Src2);
  Ref CalculateFlags_SBB(uint8_t SrcSize, Ref Src1, Ref Src2);
  Ref CalculateFlags_SUB(uint8_t SrcSize, Ref Src1, Ref Src2, bool UpdateCF = true);
  Ref CalculateFlags_ADD(uint8_t SrcSize, Ref Src1, Ref Src2, bool UpdateCF = true);
  void CalculateFlags_MUL(uint8_t SrcSize, Ref Res, Ref High);
  void CalculateFlags_UMUL(Ref High);
  void CalculateFlags_Logical(uint8_t SrcSize, Ref Res, Ref Src1, Ref Src2);
  void CalculateFlags_ShiftLeft(uint8_t SrcSize, Ref Res, Ref Src1, Ref Src2);
  void CalculateFlags_ShiftLeftImmediate(uint8_t SrcSize, Ref Res, Ref Src1, uint64_t Shift);
  void CalculateFlags_ShiftRight(uint8_t SrcSize, Ref Res, Ref Src1, Ref Src2);
  void CalculateFlags_ShiftRightImmediate(uint8_t SrcSize, Ref Res, Ref Src1, uint64_t Shift);
  void CalculateFlags_ShiftRightDoubleImmediate(uint8_t SrcSize, Ref Res, Ref Src1, uint64_t Shift);
  void CalculateFlags_ShiftRightImmediateCommon(uint8_t SrcSize, Ref Res, Ref Src1, uint64_t Shift);
  void CalculateFlags_SignShiftRightImmediate(uint8_t SrcSize, Ref Res, Ref Src1, uint64_t Shift);
  void CalculateFlags_BEXTR(Ref Src);
  void CalculateFlags_BLSI(uint8_t SrcSize, Ref Src);
  void CalculateFlags_BLSMSK(uint8_t SrcSize, Ref Res, Ref Src);
  void CalculateFlags_BLSR(uint8_t SrcSize, Ref Res, Ref Src);
  void CalculateFlags_POPCOUNT(Ref Src);
  void CalculateFlags_BZHI(uint8_t SrcSize, Ref Result, Ref Src);
  void CalculateFlags_ZCNT(uint8_t SrcSize, Ref Result);
  void CalculateFlags_RDRAND(Ref Src);
  /**  @} */

  /**
   * @name These functions generated deferred RFLAGs tracking.
   *
   * Depending on the operation it may force a RFLAGs calculation before storing the new deferred state.
   * @{ */
  void GenerateFlags_SUB(FEXCore::X86Tables::DecodedOp Op, Ref Src1, Ref Src2, bool UpdateCF = true) {
    if (!UpdateCF) {
      // If we aren't updating CF then we need to calculate flags. Invalidation mask would make this not required.
      CalculateDeferredFlags();
    }
    CurrentDeferredFlags = DeferredFlagData {
      .Type = FlagsGenerationType::TYPE_SUB,
      .SrcSize = GetSrcSize(Op),
      .Sources =
        {
          .TwoSrcImmediate =
            {
              .Src1 = Src1,
              .Src2 = Src2,
              .UpdateCF = UpdateCF,
            },
        },
    };
  }

  void GenerateFlags_MUL(FEXCore::X86Tables::DecodedOp Op, Ref Res, Ref High) {
    CurrentDeferredFlags = DeferredFlagData {
      .Type = FlagsGenerationType::TYPE_MUL,
      .SrcSize = GetSrcSize(Op),
      .Res = Res,
      .Sources =
        {
          .OneSource =
            {
              .Src1 = High,
            },
        },
    };
  }

  void GenerateFlags_UMUL(FEXCore::X86Tables::DecodedOp Op, Ref High) {
    CurrentDeferredFlags = DeferredFlagData {
      .Type = FlagsGenerationType::TYPE_UMUL,
      .SrcSize = GetSrcSize(Op),
      .Res = High,
    };
  }

  void GenerateFlags_Logical(FEXCore::X86Tables::DecodedOp Op, Ref Res, Ref Src1, Ref Src2) {
    CurrentDeferredFlags = DeferredFlagData {
      .Type = FlagsGenerationType::TYPE_LOGICAL,
      .SrcSize = GetSrcSize(Op),
      .Res = Res,
      .Sources =
        {
          .TwoSource =
            {
              .Src1 = Src1,
              .Src2 = Src2,
            },
        },
    };
  }

  void GenerateFlags_ShiftLeftImmediate(FEXCore::X86Tables::DecodedOp Op, Ref Res, Ref Src1, uint64_t Shift) {
    // No flags changed if shift is zero.
    if (Shift == 0) {
      return;
    }

    CurrentDeferredFlags = DeferredFlagData {
      .Type = FlagsGenerationType::TYPE_LSHLI,
      .SrcSize = GetSrcSize(Op),
      .Res = Res,
      .Sources =
        {
          .OneSrcImmediate =
            {
              .Src1 = Src1,
              .Imm = Shift,
            },
        },
    };
  }

  void GenerateFlags_SignShiftRightImmediate(FEXCore::X86Tables::DecodedOp Op, Ref Res, Ref Src1, uint64_t Shift) {
    // No flags changed if shift is zero.
    if (Shift == 0) {
      return;
    }

    CurrentDeferredFlags = DeferredFlagData {
      .Type = FlagsGenerationType::TYPE_ASHRI,
      .SrcSize = GetSrcSize(Op),
      .Res = Res,
      .Sources =
        {
          .OneSrcImmediate =
            {
              .Src1 = Src1,
              .Imm = Shift,
            },
        },
    };
  }

  void GenerateFlags_ShiftRightImmediate(FEXCore::X86Tables::DecodedOp Op, Ref Res, Ref Src1, uint64_t Shift) {
    // No flags changed if shift is zero.
    if (Shift == 0) {
      return;
    }

    CurrentDeferredFlags = DeferredFlagData {
      .Type = FlagsGenerationType::TYPE_LSHRI,
      .SrcSize = GetSrcSize(Op),
      .Res = Res,
      .Sources =
        {
          .OneSrcImmediate =
            {
              .Src1 = Src1,
              .Imm = Shift,
            },
        },
    };
  }

  void GenerateFlags_ShiftRightDoubleImmediate(FEXCore::X86Tables::DecodedOp Op, Ref Res, Ref Src1, uint64_t Shift) {
    // No flags changed if shift is zero.
    if (Shift == 0) {
      return;
    }

    CurrentDeferredFlags = DeferredFlagData {
      .Type = FlagsGenerationType::TYPE_LSHRDI,
      .SrcSize = GetSrcSize(Op),
      .Res = Res,
      .Sources =
        {
          .OneSrcImmediate =
            {
              .Src1 = Src1,
              .Imm = Shift,
            },
        },
    };
  }

  void GenerateFlags_BEXTR(FEXCore::X86Tables::DecodedOp Op, Ref Src) {
    CurrentDeferredFlags = DeferredFlagData {
      .Type = FlagsGenerationType::TYPE_BEXTR,
      .SrcSize = GetSrcSize(Op),
      .Res = Src,
    };
  }

  void GenerateFlags_BLSI(FEXCore::X86Tables::DecodedOp Op, Ref Src) {
    CurrentDeferredFlags = DeferredFlagData {
      .Type = FlagsGenerationType::TYPE_BLSI,
      .SrcSize = GetSrcSize(Op),
      .Res = Src,
    };
  }

  void GenerateFlags_BLSMSK(FEXCore::X86Tables::DecodedOp Op, Ref Res, Ref Src) {
    CurrentDeferredFlags = DeferredFlagData {
      .Type = FlagsGenerationType::TYPE_BLSMSK,
      .SrcSize = GetSrcSize(Op),
      .Res = Res,
      .Sources =
        {
          .OneSource =
            {
              .Src1 = Src,
            },
        },
    };
  }

  void GenerateFlags_BLSR(FEXCore::X86Tables::DecodedOp Op, Ref Res, Ref Src) {
    CurrentDeferredFlags = DeferredFlagData {
      .Type = FlagsGenerationType::TYPE_BLSR,
      .SrcSize = GetSrcSize(Op),
      .Res = Res,
      .Sources =
        {
          .OneSource =
            {
              .Src1 = Src,
            },
        },
    };
  }

  void GenerateFlags_POPCOUNT(FEXCore::X86Tables::DecodedOp Op, Ref Src) {
    CurrentDeferredFlags = DeferredFlagData {
      .Type = FlagsGenerationType::TYPE_POPCOUNT,
      .SrcSize = GetSrcSize(Op),
      .Res = Src,
    };
  }

  void GenerateFlags_BZHI(FEXCore::X86Tables::DecodedOp Op, Ref Result, Ref Src) {
    CurrentDeferredFlags = DeferredFlagData {
      .Type = FlagsGenerationType::TYPE_BZHI,
      .SrcSize = GetSrcSize(Op),
      .Res = Result,
      .Sources =
        {
          .OneSource =
            {
              .Src1 = Src,
            },
        },
    };
  }

  void GenerateFlags_ZCNT(FEXCore::X86Tables::DecodedOp Op, Ref Src) {
    CurrentDeferredFlags = DeferredFlagData {
      .Type = FlagsGenerationType::TYPE_ZCNT,
      .SrcSize = GetSrcSize(Op),
      .Res = Src,
    };
  }

  void GenerateFlags_RDRAND(FEXCore::X86Tables::DecodedOp Op, Ref Src) {
    CurrentDeferredFlags = DeferredFlagData {
      .Type = FlagsGenerationType::TYPE_RDRAND,
      .SrcSize = GetSrcSize(Op),
      .Res = Src,
    };
  }

  Ref AndConst(FEXCore::IR::OpSize Size, Ref Node, uint64_t Const) {
    uint64_t NodeConst;

    if (IsValueConstant(WrapNode(Node), &NodeConst)) {
      return _Constant(NodeConst & Const);
    } else {
      return _And(Size, Node, _Constant(Const));
    }
  }

  /**  @} */
  /**  @} */

  Ref GetX87Top();
  void SetX87ValidTag(Ref Value, bool Valid);
  Ref GetX87ValidTag(Ref Value);
  Ref GetX87Tag(Ref Value, Ref AbridgedFTW);
  Ref GetX87Tag(Ref Value);
  void SetX87FTW(Ref FTW);
  Ref GetX87FTW();
  void SetX87Top(Ref Value);

  bool DestIsLockedMem(FEXCore::X86Tables::DecodedOp Op) const {
    return DestIsMem(Op) && (Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_LOCK) != 0;
  }

  bool DestIsMem(FEXCore::X86Tables::DecodedOp Op) const {
    return !Op->Dest.IsGPR();
  }

  void CreateJumpBlocks(const fextl::vector<FEXCore::Frontend::Decoder::DecodedBlocks>* Blocks);
  bool BlockSetRIP {false};

  bool Multiblock {};
  uint64_t Entry;

  Ref _StoreMemAutoTSO(FEXCore::IR::RegisterClassType Class, uint8_t Size, Ref Addr, Ref Value, uint8_t Align = 1) {
    if (CTX->IsAtomicTSOEnabled()) {
      return _StoreMemTSO(Class, Size, Value, Addr, Invalid(), Align, MEM_OFFSET_SXTX, 1);
    } else {
      return _StoreMem(Class, Size, Value, Addr, Invalid(), Align, MEM_OFFSET_SXTX, 1);
    }
  }

  Ref _LoadMemAutoTSO(FEXCore::IR::RegisterClassType Class, uint8_t Size, Ref ssa0, uint8_t Align = 1) {
    if (CTX->IsAtomicTSOEnabled()) {
      return _LoadMemTSO(Class, Size, ssa0, Invalid(), Align, MEM_OFFSET_SXTX, 1);
    } else {
      return _LoadMem(Class, Size, ssa0, Invalid(), Align, MEM_OFFSET_SXTX, 1);
    }
  }

  Ref _LoadMemAutoTSO(FEXCore::IR::RegisterClassType Class, uint8_t Size, AddressMode A, uint8_t Align = 1) {
    bool AtomicTSO = CTX->IsAtomicTSOEnabled() && !A.NonTSO;
    A = SelectAddressMode(A, AtomicTSO, Class != GPRClass, Size);

    if (AtomicTSO) {
      return _LoadMemTSO(Class, Size, A.Base, A.Index, Align, A.IndexType, A.IndexScale);
    } else {
      return _LoadMem(Class, Size, A.Base, A.Index, Align, A.IndexType, A.IndexScale);
    }
  }

  Ref _StoreMemAutoTSO(FEXCore::IR::RegisterClassType Class, uint8_t Size, AddressMode A, Ref Value, uint8_t Align = 1) {
    bool AtomicTSO = CTX->IsAtomicTSOEnabled() && !A.NonTSO;
    A = SelectAddressMode(A, AtomicTSO, Class != GPRClass, Size);

    if (AtomicTSO) {
      return _StoreMemTSO(Class, Size, Value, A.Base, A.Index, Align, A.IndexType, A.IndexScale);
    } else {
      return _StoreMem(Class, Size, Value, A.Base, A.Index, Align, A.IndexType, A.IndexScale);
    }
  }

  Ref Prefetch(bool ForStore, bool Stream, uint8_t CacheLevel, Ref ssa0) {
    return _Prefetch(ForStore, Stream, CacheLevel, ssa0, Invalid(), MEM_OFFSET_SXTX, 1);
  }

  void InstallHostSpecificOpcodeHandlers();

  ///< Segment telemetry tracking
  uint32_t SegmentsNeedReadCheck {~0U};
  void CheckLegacySegmentWrite(Ref NewNode, uint32_t SegmentReg);
  void CheckLegacySegmentRead(Ref NewNode, uint32_t SegmentReg);
};

void InstallOpcodeHandlers(Context::OperatingMode Mode);

} // namespace FEXCore::IR
template<>
struct fmt::formatter<FEXCore::IR::OpDispatchBuilder::FlagsGenerationType> : fmt::formatter<int> {
  using Base = fmt::formatter<int>;

  // Pass-through the underlying value, so IDs can
  // be formatted like any integral value.
  template<typename FormatContext>
  auto format(const FEXCore::IR::OpDispatchBuilder::FlagsGenerationType& ID, FormatContext& ctx) {
    return Base::format(static_cast<int>(ID), ctx);
  }
};
