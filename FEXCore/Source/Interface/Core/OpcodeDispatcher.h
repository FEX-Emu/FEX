// SPDX-License-Identifier: MIT
#pragma once

#include "Interface/Core/Frontend.h"
#include "Interface/Core/X86Tables/X86Tables.h"
#include "Interface/Core/Addressing.h"
#include "Interface/Context/Context.h"
#include "Interface/IR/IR.h"
#include "Interface/IR/IREmitter.h"
#include "Interface/IR/RegisterAllocationData.h"

#include <FEXCore/Config/Config.h>
#include <FEXCore/Core/Context.h>
#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/IR/IR.h>

#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/MathUtils.h>
#include <FEXCore/fextl/map.h>
#include <FEXCore/fextl/vector.h>

#include <bit>
#include <cstdint>
#include <fmt/format.h>
#include <stddef.h>
#include <utility>
#include <xxhash.h>

namespace FEXCore::IR {

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

enum class ForceTSOMode {
  NoOverride,
  ForceDisabled,
  ForceEnabled,
};

struct LoadSourceOptions {
  // Alignment of the load in bytes. iInvalid signifies opsize aligned.
  IR::OpSize Align = OpSize::iInvalid;

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

struct DispatchTableEntry {
  uint16_t Op;
  uint8_t Count;
  X86Tables::OpDispatchPtr Ptr;
};

class OpDispatchBuilder final : public IREmitter {
public:
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
    CFInverted = CFInvertedABI;

    FlushRegisterCache();

    // Start block in X87 state.
    // This is important to ensure that blocks always start with the same state independently of predecessors
    // which allows independent compilation of blocks.
    // Starting in the X87 state is better than starting in MMX state because
    // MMX state is more work to initialize.
    MMXState = MMXState_X87;

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
  IRPair<IROp_CondJump> CondJump(Ref _Cmp1, Ref _Cmp2, Ref _TrueBlock, Ref _FalseBlock, CondClass _Cond = CondClass::NEQ,
                                 IR::OpSize _CompareSize = OpSize::iInvalid) {
    FlushRegisterCache();
    return _CondJump(_Cmp1, _Cmp2, _TrueBlock, _FalseBlock, _Cond, _CompareSize);
  }
  IRPair<IROp_CondJump> CondJump(Ref ssa0, CondClass cond = CondClass::NEQ) {
    FlushRegisterCache();
    return _CondJump(ssa0, cond);
  }
  IRPair<IROp_CondJump> CondJump(Ref ssa0, Ref ssa1, Ref ssa2, CondClass cond = CondClass::NEQ) {
    FlushRegisterCache();
    return _CondJump(ssa0, ssa1, ssa2, cond);
  }
  IRPair<IROp_CondJump> CondJumpNZCV(CondClass Cond) {
    FlushRegisterCache();
    return _CondJump(InvalidNode, InvalidNode, InvalidNode, InvalidNode, Cond, OpSize::iInvalid, true);
  }
  IRPair<IROp_CondJump> CondJumpBit(Ref Src, unsigned Bit, bool Set) {
    FlushRegisterCache();
    auto InlineConst = _InlineConstant(Bit);
    auto Cond = Set ? CondClass::TSTNZ : CondClass::TSTZ;
    return _CondJump(Src, InlineConst, InvalidNode, InvalidNode, Cond, OpSize::iInvalid, false);
  }
  IRPair<IROp_ExitFunction> ExitFunction(Ref NewRIP, BranchHint Hint = BranchHint::None) {
    FlushRegisterCache();
    return _ExitFunction(GetOpSize(NewRIP), NewRIP, Hint, InvalidNode, InvalidNode);
  }
  IRPair<IROp_ExitFunction> ExitFunction(Ref NewRIP, BranchHint Hint, Ref CallReturnAddress, Ref CallReturnBlock) {
    FlushRegisterCache();
    return _ExitFunction(GetOpSize(NewRIP), NewRIP, Hint, CallReturnAddress, CallReturnBlock);
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

        const auto GPRSize = GetGPROpSize();
        // If we don't have a jump target to a new block then we have to leave
        // Set the RIP to the next instruction and leave
        ExitFunction(_InlineEntrypointOffset(GPRSize, NextRIP - Entry));
      } else if (it != JumpTargets.end()) {
        Jump(it->second.BlockEntry);
        return true;
      }
    }

    BlockSetRIP = false;

    return false;
  }

  static bool CanHaveSideEffects(const FEXCore::X86Tables::X86InstInfo* TableInfo, FEXCore::X86Tables::DecodedOp Op) {
    if (TableInfo) {
      if (TableInfo->Flags & X86Tables::InstFlags::FLAGS_DEBUG_MEM_ACCESS) {
        // If it is marked as having memory access then always say it has a side-effect.
        // Not always true but better to be safe.
        return true;
      }

      if (TableInfo->Flags & (X86Tables::InstFlags::FLAGS_SETS_RIP | X86Tables::InstFlags::FLAGS_BLOCK_END)) {
        // Cooperative suspend interrupts can be triggered at any back-edge, the RIP must be reconstructed correctly in such cases
        return true;
      }
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
    auto Zero = Constant(0);

    // If the shift is zero, do not touch the flags.
    auto ForwardBlock = CreateNewCodeBlockAfter(GetCurrentBlock());
    auto BackwardBlock = CreateNewCodeBlockAfter(ForwardBlock);
    auto ExitBlock = CreateNewCodeBlockAfter(BackwardBlock);

    auto DF = GetRFLAG(X86State::RFLAG_DF_RAW_LOC);
    CondJump(DF, Zero, ForwardBlock, BackwardBlock, CondClass::EQ);

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

  void SetForceTSO(ForceTSOMode Mode) {
    ForceTSO = Mode;
  }
  ForceTSOMode GetForceTSO() const {
    return ForceTSO;
  }

  void SetDumpIR(bool DumpIR) {
    ShouldDump = DumpIR;
  }
  bool ShouldDumpIR() const {
    return ShouldDump;
  }

  void BeginFunction(uint64_t RIP, const fextl::vector<FEXCore::Frontend::Decoder::DecodedBlocks>* Blocks, uint32_t NumInstructions,
                     bool Is64BitMode, bool MonoBackpatcherBlock);
  void Finalize();

  // Dispatch builder functions
#define OpcodeArgs [[maybe_unused]] FEXCore::X86Tables::DecodedOp Op

  /**
   * Binds a sequence of compile-time constants as arguments to another member function.
   * This allows to construct OpDispatchPtrs that are specialized for the given set of arguments.
   */
  template<auto Fn, auto... Args>
  void Bind(OpcodeArgs) {
    [[clang::noinline]] (this->*Fn)(Op, Args...);
  };

  void UnhandledOp(OpcodeArgs);
  void MOVGPROp(OpcodeArgs, uint32_t SrcIndex);
  void MOVGPRNTOp(OpcodeArgs);
  void MOVVectorAlignedOp(OpcodeArgs);
  void MOVVectorUnalignedOp(OpcodeArgs);
  void MOVVectorNTOp(OpcodeArgs);
  void ALUOp(OpcodeArgs, FEXCore::IR::IROps ALUIROp, FEXCore::IR::IROps AtomicFetchOp, unsigned SrcIdx);
  void LSLOp(OpcodeArgs);
  void INTOp(OpcodeArgs);
  void SyscallOp(OpcodeArgs, bool IsSyscallInst);
  void ThunkOp(OpcodeArgs);
  void LEAOp(OpcodeArgs);
  void NOPOp(OpcodeArgs);
  void RETOp(OpcodeArgs);
  void IRETOp(OpcodeArgs);
  void CallbackReturnOp(OpcodeArgs);
  void SecondaryALUOp(OpcodeArgs);
  void ADCOp(OpcodeArgs, uint32_t SrcIndex);
  void SBBOp(OpcodeArgs, uint32_t SrcIndex);
  void SALCOp(OpcodeArgs);
  void PUSHOp(OpcodeArgs);
  void PUSHREGOp(OpcodeArgs);
  void PUSHAOp(OpcodeArgs);
  void PUSHSegmentOp(OpcodeArgs, uint32_t SegmentReg);
  void POPOp(OpcodeArgs);
  void POPAOp(OpcodeArgs);
  void POPSegmentOp(OpcodeArgs, uint32_t SegmentReg);
  void LEAVEOp(OpcodeArgs);
  void CALLOp(OpcodeArgs);
  void CALLAbsoluteOp(OpcodeArgs);
  void CondJUMPOp(OpcodeArgs);
  void CondJUMPRCXOp(OpcodeArgs);
  void LoopOp(OpcodeArgs);
  void JUMPOp(OpcodeArgs);
  void JUMPAbsoluteOp(OpcodeArgs);
  void JUMPFARIndirectOp(OpcodeArgs);
  void CALLFARIndirectOp(OpcodeArgs);
  void RETFARIndirectOp(OpcodeArgs);
  void TESTOp(OpcodeArgs, uint32_t SrcIndex);
  void ARPLOp(OpcodeArgs);
  void MOVSXDOp(OpcodeArgs);
  void MOVSXOp(OpcodeArgs);
  void MOVZXOp(OpcodeArgs);
  void CMPOp(OpcodeArgs, uint32_t SrcIndex);
  void SETccOp(OpcodeArgs);
  void CQOOp(OpcodeArgs);
  void CDQOp(OpcodeArgs);
  void XCHGOp(OpcodeArgs);
  void SAHFOp(OpcodeArgs);
  void LAHFOp(OpcodeArgs);
  void MOVSegOp(OpcodeArgs, bool ToSeg);
  void FLAGControlOp(OpcodeArgs);
  void MOVOffsetOp(OpcodeArgs);
  void CMOVOp(OpcodeArgs);
  void CPUIDOp(OpcodeArgs);
  void XGetBVOp(OpcodeArgs);
  uint32_t GetConstantShift(X86Tables::DecodedOp Op, bool Is1Bit);
  void SHLOp(OpcodeArgs);
  void SHLImmediateOp(OpcodeArgs, bool SHL1Bit);
  void SHROp(OpcodeArgs);
  void SHRImmediateOp(OpcodeArgs, bool SHR1Bit);
  void SHLDOp(OpcodeArgs);
  void SHLDImmediateOp(OpcodeArgs);
  void SHRDOp(OpcodeArgs);
  void SHRDImmediateOp(OpcodeArgs);
  void ASHROp(OpcodeArgs, bool IsImmediate, bool Is1Bit);
  void RotateOp(OpcodeArgs, bool Left, bool IsImmediate, bool Is1Bit);
  void RCROp1Bit(OpcodeArgs);
  void RCROp8x1Bit(OpcodeArgs);
  void RCROp(OpcodeArgs);
  void RCRSmallerOp(OpcodeArgs);
  void RCLOp1Bit(OpcodeArgs);
  void RCLOp(OpcodeArgs);
  void RCLSmallerOp(OpcodeArgs);

  void BTOp(OpcodeArgs, uint32_t SrcIndex, enum BTAction Action);

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
  CycleCounterPair CycleCounter(bool SelfSynchronizingLoads);
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
  void ReadSegmentReg(OpcodeArgs, Segment Seg);
  void WriteSegmentReg(OpcodeArgs, Segment Seg);
  void EnterOp(OpcodeArgs);

  void SGDTOp(OpcodeArgs);
  void SIDTOp(OpcodeArgs);
  void SMSWOp(OpcodeArgs);

  enum class VectorOpType {
    MMX,
    SSE,
    AVX,
  };
  // SSE
  void MOVLPOp(OpcodeArgs);
  void MOVHPDOp(OpcodeArgs);
  void MOVSDOp(OpcodeArgs);
  void MOVSSOp(OpcodeArgs);
  void VectorALUOp(OpcodeArgs, IROps IROp, IR::OpSize ElementSize);
  void VectorXOROp(OpcodeArgs);

  void VectorALUROp(OpcodeArgs, IROps IROp, IR::OpSize ElementSize);
  void VectorUnaryOp(OpcodeArgs, IROps IROp, IR::OpSize ElementSize);
  void RSqrt3DNowOp(OpcodeArgs, bool Duplicate);
  template<FEXCore::IR::IROps IROp, IR::OpSize ElementSize>
  void VectorUnaryDuplicateOp(OpcodeArgs);

  void MOVQOp(OpcodeArgs, VectorOpType VectorType);
  void MOVQMMXOp(OpcodeArgs);
  void MOVMSKOp(OpcodeArgs, IR::OpSize ElementSize);
  void MOVMSKOpOne(OpcodeArgs);
  void PUNPCKLOp(OpcodeArgs, IR::OpSize ElementSize);
  void PUNPCKHOp(OpcodeArgs, IR::OpSize ElementSize);
  void PSHUFBOp(OpcodeArgs);
  Ref PShufWLane(IR::OpSize Size, FEXCore::IR::IndexNamedVectorConstant IndexConstant, bool LowLane, Ref IncomingLane, uint8_t Shuffle);
  void PSHUFWOp(OpcodeArgs, bool Low);
  void PSHUFW8ByteOp(OpcodeArgs);
  void PSHUFDOp(OpcodeArgs);
  void PSRLDOp(OpcodeArgs, IR::OpSize ElementSize);
  void PSRLI(OpcodeArgs, IR::OpSize ElementSize);
  void PSLLI(OpcodeArgs, IR::OpSize ElementSize);
  void PSLL(OpcodeArgs, IR::OpSize ElementSize);
  void PSRAOp(OpcodeArgs, IR::OpSize ElementSize);
  void PSRLDQ(OpcodeArgs);
  void PSLLDQ(OpcodeArgs);
  void PSRAIOp(OpcodeArgs, IR::OpSize ElementSize);
  void MOVDDUPOp(OpcodeArgs);
  template<IR::OpSize DstElementSize>
  void CVTGPR_To_FPR(OpcodeArgs);
  template<IR::OpSize SrcElementSize, bool HostRoundingMode>
  void CVTFPR_To_GPR(OpcodeArgs);
  template<IR::OpSize SrcElementSize, bool Widen>
  void Vector_CVT_Int_To_Float(OpcodeArgs);
  template<IR::OpSize DstElementSize, IR::OpSize SrcElementSize>
  void Scalar_CVT_Float_To_Float(OpcodeArgs);
  void Vector_CVT_Float_To_Float(OpcodeArgs, IR::OpSize DstElementSize, IR::OpSize SrcElementSize, bool IsAVX);
  template<IR::OpSize SrcElementSize, bool HostRoundingMode>
  void Vector_CVT_Float_To_Int(OpcodeArgs);
  void MMX_To_XMM_Vector_CVT_Int_To_Float(OpcodeArgs);
  template<IR::OpSize SrcElementSize, bool HostRoundingMode>
  void XMM_To_MMX_Vector_CVT_Float_To_Int(OpcodeArgs);
  void MASKMOVOp(OpcodeArgs);
  void MOVBetweenGPR_FPR(OpcodeArgs, VectorOpType VectorType);
  void TZCNT(OpcodeArgs);
  void LZCNT(OpcodeArgs);
  template<IR::OpSize ElementSize>
  void VFCMPOp(OpcodeArgs);
  void SHUFOp(OpcodeArgs, IR::OpSize ElementSize);
  template<IR::OpSize ElementSize>
  void PINSROp(OpcodeArgs);
  void InsertPSOp(OpcodeArgs);
  void PExtrOp(OpcodeArgs, IR::OpSize ElementSize);

  template<IR::OpSize ElementSize>
  void PSIGN(OpcodeArgs);
  template<IR::OpSize ElementSize>
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
  void AVXVectorXOROp(OpcodeArgs);

  template<IR::OpSize ElementSize>
  void AVXVectorRound(OpcodeArgs);

  template<IR::OpSize DstElementSize, IR::OpSize SrcElementSize>
  void AVXScalar_CVT_Float_To_Float(OpcodeArgs);

  template<FEXCore::IR::IROps IROp, IR::OpSize ElementSize>
  void VectorScalarInsertALUOp(OpcodeArgs);
  template<FEXCore::IR::IROps IROp, IR::OpSize ElementSize>
  void AVXVectorScalarInsertALUOp(OpcodeArgs);

  template<FEXCore::IR::IROps IROp, IR::OpSize ElementSize>
  void VectorScalarUnaryInsertALUOp(OpcodeArgs);
  template<FEXCore::IR::IROps IROp, IR::OpSize ElementSize>
  void AVXVectorScalarUnaryInsertALUOp(OpcodeArgs);

  void InsertMMX_To_XMM_Vector_CVT_Int_To_Float(OpcodeArgs);
  template<IR::OpSize DstElementSize>
  void InsertCVTGPR_To_FPR(OpcodeArgs);
  template<IR::OpSize DstElementSize>
  void AVXInsertCVTGPR_To_FPR(OpcodeArgs);

  template<IR::OpSize DstElementSize, IR::OpSize SrcElementSize>
  void InsertScalar_CVT_Float_To_Float(OpcodeArgs);
  template<IR::OpSize DstElementSize, IR::OpSize SrcElementSize>
  void AVXInsertScalar_CVT_Float_To_Float(OpcodeArgs);

  RoundMode TranslateRoundType(uint8_t Mode);

  template<IR::OpSize ElementSize>
  void InsertScalarRound(OpcodeArgs);
  template<IR::OpSize ElementSize>
  void AVXInsertScalarRound(OpcodeArgs);

  template<IR::OpSize ElementSize>
  void InsertScalarFCMPOp(OpcodeArgs);
  template<IR::OpSize ElementSize>
  void AVXInsertScalarFCMPOp(OpcodeArgs);

  template<IR::OpSize DstElementSize>
  void AVXCVTGPR_To_FPR(OpcodeArgs);

  template<IR::OpSize ElementSize>
  void AVXVFCMPOp(OpcodeArgs);

  template<IR::OpSize ElementSize>
  void VADDSUBPOp(OpcodeArgs);

  void VAESDecOp(OpcodeArgs);
  void VAESDecLastOp(OpcodeArgs);
  void VAESEncOp(OpcodeArgs);
  void VAESEncLastOp(OpcodeArgs);

  void VANDNOp(OpcodeArgs);

  Ref VBLENDOpImpl(IR::OpSize VecSize, IR::OpSize ElementSize, Ref Src1, Ref Src2, Ref ZeroRegister, uint64_t Selector);
  void VBLENDPDOp(OpcodeArgs);
  void VPBLENDDOp(OpcodeArgs);
  void VPBLENDWOp(OpcodeArgs);

  void VBROADCASTOp(OpcodeArgs, IR::OpSize ElementSize);

  template<IR::OpSize ElementSize>
  void VDPPOp(OpcodeArgs);

  void VEXTRACT128Op(OpcodeArgs);

  template<IROps IROp, IR::OpSize ElementSize>
  void VHADDPOp(OpcodeArgs);
  void VHSUBPOp(OpcodeArgs, IR::OpSize ElementSize);

  void VINSERTOp(OpcodeArgs);
  void VINSERTPSOp(OpcodeArgs);

  template<IR::OpSize ElementSize, bool IsStore>
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

  void VPACKSSOp(OpcodeArgs, IR::OpSize ElementSize);

  void VPACKUSOp(OpcodeArgs, IR::OpSize ElementSize);

  void VPALIGNROp(OpcodeArgs);

  void VPCMPESTRIOp(OpcodeArgs);
  void VPCMPESTRMOp(OpcodeArgs);
  void VPCMPISTRIOp(OpcodeArgs);
  void VPCMPISTRMOp(OpcodeArgs);

  void VCVTPH2PSOp(OpcodeArgs);
  void VCVTPS2PHOp(OpcodeArgs);

  Ref VPERMDIndices(OpSize DstSize, Ref Indices, Ref IndexMask, Ref Repeating3210);
  void VPERM2Op(OpcodeArgs);
  void VPERMDOp(OpcodeArgs);
  void VPERMQOp(OpcodeArgs);

  void VPERMILImmOp(OpcodeArgs, IR::OpSize ElementSize);

  Ref VPERMILRegOpImpl(OpSize DstSize, IR::OpSize ElementSize, Ref Src, Ref Indices);
  template<IR::OpSize ElementSize>
  void VPERMILRegOp(OpcodeArgs);

  void VPHADDSWOp(OpcodeArgs);

  void VPHSUBOp(OpcodeArgs, IR::OpSize ElementSize);
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

  template<IR::OpSize ElementSize, bool Signed>
  void VPMULLOp(OpcodeArgs);

  void VPSADBWOp(OpcodeArgs);

  void VPSHUFBOp(OpcodeArgs);

  void VPSHUFWOp(OpcodeArgs, IR::OpSize ElementSize, bool Low);

  void VPSLLOp(OpcodeArgs, IR::OpSize ElementSize);
  void VPSLLDQOp(OpcodeArgs);
  void VPSLLIOp(OpcodeArgs, IR::OpSize ElementSize);
  void VPSLLVOp(OpcodeArgs);

  void VPSRAOp(OpcodeArgs, IR::OpSize ElementSize);

  void VPSRAIOp(OpcodeArgs, IR::OpSize ElementSize);

  void VPSRAVDOp(OpcodeArgs);
  void VPSRLVOp(OpcodeArgs);

  void VPSRLDOp(OpcodeArgs, IR::OpSize ElementSize);
  void VPSRLDQOp(OpcodeArgs);

  void VPUNPCKHOp(OpcodeArgs, IR::OpSize ElementSize);

  void VPUNPCKLOp(OpcodeArgs, IR::OpSize ElementSize);

  void VPSRLIOp(OpcodeArgs, IR::OpSize ElementSize);

  void VSHUFOp(OpcodeArgs, IR::OpSize ElementSize);

  template<IR::OpSize ElementSize>
  void VTESTPOp(OpcodeArgs);

  void VZEROOp(OpcodeArgs);

  // X87 Ops
  Ref ReconstructFSW_Helper(Ref T = nullptr);
  // Returns new x87 stack top from FSW.
  Ref ReconstructX87StateFromFSW_Helper(Ref FSW);
  void FLD(OpcodeArgs, IR::OpSize Width);
  void FLDFromStack(OpcodeArgs);
  void FLD_Const(OpcodeArgs, NamedVectorConstant K);

  void FBLD(OpcodeArgs);
  void FBSTP(OpcodeArgs);

  void FILD(OpcodeArgs);

  void FST(OpcodeArgs, IR::OpSize Width);
  void FSTToStack(OpcodeArgs);

  void FIST(OpcodeArgs, bool Truncate);

  // OpResult is used for Stack operations,
  // describes if the result of the operation is stored in ST(0) or ST(i),
  // where ST(i) is one of the arguments to the operation.
  enum class OpResult {
    RES_ST0,
    RES_STI,
  };

  void FADD(OpcodeArgs, IR::OpSize Width, bool Integer, OpResult ResInST0);
  void FDIV(OpcodeArgs, IR::OpSize Width, bool Integer, bool Reverse, OpResult ResInST0);
  void FMUL(OpcodeArgs, IR::OpSize Width, bool Integer, OpResult ResInST0);
  void FNCLEX(OpcodeArgs);
  void FNINIT(OpcodeArgs);
  void FSUB(OpcodeArgs, IR::OpSize Width, bool Integer, bool Reverse, OpResult ResInST0);
  void FTST(OpcodeArgs);
  void FXCH(OpcodeArgs);
  void X87EMMS(OpcodeArgs);
  void X87FCMOV(OpcodeArgs);
  void X87FFREE(OpcodeArgs);
  void X87FLDCW(OpcodeArgs);
  void X87FNSAVE(OpcodeArgs);
  void X87FNSTENV(OpcodeArgs);
  void X87FNSTSW(OpcodeArgs);
  void X87FRSTOR(OpcodeArgs);
  void X87FSTCW(OpcodeArgs);
  void X87FXAM(OpcodeArgs);
  void X87FXTRACT(OpcodeArgs);
  void X87FYL2X(OpcodeArgs, bool IsFYL2XP1);
  void X87LDENV(OpcodeArgs);
  void X87ModifySTP(OpcodeArgs, bool Inc);
  void X87OpHelper(OpcodeArgs, FEXCore::IR::IROps IROp, bool ZeroC2);

  enum class FCOMIFlags {
    FLAGS_X87,
    FLAGS_RFLAGS,
  };
  void FCOMI(OpcodeArgs, IR::OpSize Width, bool Integer, FCOMIFlags WhichFlags, bool PopTwice);

  // F64 X87 Ops
  void FADDF64(OpcodeArgs, IR::OpSize Width, bool Integer, OpResult ResInST0);
  void FBLDF64(OpcodeArgs);
  void FBSTPF64(OpcodeArgs);
  void FCOMIF64(OpcodeArgs, IR::OpSize width, bool Integer, FCOMIFlags whichflags, bool poptwice);
  void FDIVF64(OpcodeArgs, IR::OpSize Width, bool Integer, bool Reverse, OpResult ResInST0);
  void FILDF64(OpcodeArgs);
  void FISTF64(OpcodeArgs, bool Truncate);
  void FLDF64_Const(OpcodeArgs, uint64_t Num);
  void FLDF64(OpcodeArgs, IR::OpSize Width);
  void FMULF64(OpcodeArgs, IR::OpSize Width, bool Integer, OpResult ResInST0);
  void FSUBF64(OpcodeArgs, IR::OpSize Width, bool Integer, bool Reverse, OpResult ResInST0);
  void FTSTF64(OpcodeArgs);
  void X87FLDCWF64(OpcodeArgs);
  void X87FXTRACTF64(OpcodeArgs);
  void X87LDENVF64(OpcodeArgs);

  void FXSaveOp(OpcodeArgs);
  void FXRStoreOp(OpcodeArgs);

  Ref XSaveBase(X86Tables::DecodedOp Op);
  void XSaveOp(OpcodeArgs);

  void PAlignrOp(OpcodeArgs);
  template<IR::OpSize ElementSize>
  void UCOMISxOp(OpcodeArgs);
  void LDMXCSR(OpcodeArgs);
  void STMXCSR(OpcodeArgs);

  template<IR::OpSize ElementSize>
  void PACKUSOp(OpcodeArgs);

  template<IR::OpSize ElementSize>
  void PACKSSOp(OpcodeArgs);

  template<IR::OpSize ElementSize, bool Signed>
  void PMULLOp(OpcodeArgs);

  template<bool ToXMM>
  void MOVQ2DQ(OpcodeArgs);

  template<IR::OpSize ElementSize>
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
  template<IR::OpSize ElementSize>
  void HSUBP(OpcodeArgs);
  template<IR::OpSize ElementSize>
  void PHSUB(OpcodeArgs);

  void PHADDS(OpcodeArgs);
  void PHSUBS(OpcodeArgs);

  void CLWBOrTPause(OpcodeArgs);
  void CLFLUSHOPT(OpcodeArgs);
  void LoadFenceOrXRSTOR(OpcodeArgs);
  void MemFenceOrXSAVEOPT(OpcodeArgs);
  void StoreFenceOrCLFlush(OpcodeArgs);
  void UMonitorOrCLRSSBSY(OpcodeArgs);
  void UMWaitOp(OpcodeArgs);
  void CLZeroOp(OpcodeArgs);
  void RDTSCPOp(OpcodeArgs);
  void RDPIDOp(OpcodeArgs);

  void Prefetch(OpcodeArgs, bool ForStore, bool Stream, uint8_t Level);

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

  void VFMAImpl(OpcodeArgs, IROps IROp, bool Scalar, uint8_t Src1Idx, uint8_t Src2Idx, uint8_t AddendIdx);
  void VFMAddSubImpl(OpcodeArgs, bool AddSub, uint8_t Src1Idx, uint8_t Src2Idx, uint8_t AddendIdx);

  struct RefVSIB {
    Ref Low, High;
    Ref BaseAddr;
    int32_t Displacement;
    uint8_t Scale;
  };

  RefVSIB LoadVSIB(const X86Tables::DecodedOp& Op, const X86Tables::DecodedOperand& Operand, uint32_t Flags);
  template<OpSize AddrElementSize>
  void VPGATHER(OpcodeArgs);

  template<IR::OpSize ElementSize, IR::OpSize DstElementSize, bool Signed>
  void ExtendVectorElements(OpcodeArgs);
  template<IR::OpSize ElementSize>
  void VectorRound(OpcodeArgs);

  Ref VectorBlend(OpSize Size, IR::OpSize ElementSize, Ref Src1, Ref Src2, uint8_t Selector);

  template<IR::OpSize ElementSize>
  void VectorBlend(OpcodeArgs);

  void VectorVariableBlend(OpcodeArgs, IR::OpSize ElementSize);
  void PTestOpImpl(OpSize Size, Ref Dest, Ref Src);
  void PTestOp(OpcodeArgs);
  void PHMINPOSUWOp(OpcodeArgs);
  template<IR::OpSize ElementSize>
  void DPPOp(OpcodeArgs);

  void MPSADBWOp(OpcodeArgs);
  void PCLMULQDQOp(OpcodeArgs);
  void VPCLMULQDQOp(OpcodeArgs);

  void CRC32(OpcodeArgs);
  void Extrq_imm(OpcodeArgs);
  void Insertq_imm(OpcodeArgs);
  void Extrq(OpcodeArgs);
  void Insertq(OpcodeArgs);

  void BreakOp(OpcodeArgs, FEXCore::IR::BreakDefinition BreakDefinition);
  void UnimplementedOp(OpcodeArgs);
  void PermissionRestrictedOp(OpcodeArgs);

  ///< Helper for PSHUD and VPERMILPS(imm) since they are the same instruction
  Ref Single128Bit4ByteVectorShuffle(Ref Src, uint8_t Shuffle);
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

  Ref SHADataShuffle(Ref Src) {
    // SHA data shuffle matches PSHUFD shuffle where elements are inverted.
    // Because this shuffle mask gets reused multiple times per instruction, it's always a win to load the mask once and reuse it.
    const uint32_t Shuffle = 0b00'01'10'11;
    auto LookupIndexes =
      LoadAndCacheIndexedNamedVectorConstant(OpSize::i128Bit, FEXCore::IR::IndexNamedVectorConstant::INDEXED_NAMED_VECTOR_PSHUFD, Shuffle * 16);
    return _VTBL1(OpSize::i128Bit, Src, LookupIndexes);
  }

  RefPair AVX128_LoadSource_WithOpSize(const X86Tables::DecodedOp& Op, const X86Tables::DecodedOperand& Operand, uint32_t Flags,
                                       bool NeedsHigh, MemoryAccessType AccessType = MemoryAccessType::DEFAULT);

  RefVSIB AVX128_LoadVSIB(const X86Tables::DecodedOp& Op, const X86Tables::DecodedOperand& Operand, uint32_t Flags, bool NeedsHigh);
  void AVX128_StoreResult_WithOpSize(FEXCore::X86Tables::DecodedOp Op, const FEXCore::X86Tables::DecodedOperand& Operand, const RefPair Src,
                                     MemoryAccessType AccessType = MemoryAccessType::DEFAULT);
  void AVX128_VMOVScalarImpl(OpcodeArgs, IR::OpSize ElementSize);
  void AVX128_VectorALU(OpcodeArgs, IROps IROp, IR::OpSize ElementSize);
  void AVX128_VectorUnary(OpcodeArgs, IROps IROp, IR::OpSize ElementSize);
  void AVX128_VectorUnaryImpl(OpcodeArgs, IR::OpSize SrcSize, IR::OpSize ElementSize, std::function<Ref(IR::OpSize ElementSize, Ref Src)> Helper);
  void AVX128_VectorBinaryImpl(OpcodeArgs, IR::OpSize SrcSize, IR::OpSize ElementSize,
                               std::function<Ref(IR::OpSize ElementSize, Ref Src1, Ref Src2)> Helper);
  void AVX128_VectorShiftWideImpl(OpcodeArgs, IR::OpSize ElementSize, IROps IROp);
  void AVX128_VectorShiftImmImpl(OpcodeArgs, IR::OpSize ElementSize, IROps IROp);
  void AVX128_VectorTrinaryImpl(OpcodeArgs, IR::OpSize SrcSize, IR::OpSize ElementSize, Ref Src3,
                                std::function<Ref(IR::OpSize ElementSize, Ref Src1, Ref Src2, Ref Src3)> Helper);

  enum class ShiftDirection { RIGHT, LEFT };
  void AVX128_ShiftDoubleImm(OpcodeArgs, ShiftDirection Dir);

  void AVX128_VMOVAPS(OpcodeArgs);
  void AVX128_VMOVSD(OpcodeArgs);
  void AVX128_VMOVSS(OpcodeArgs);

  void AVX128_VectorXOR(OpcodeArgs);

  void AVX128_VZERO(OpcodeArgs);
  void AVX128_MOVVectorNT(OpcodeArgs);
  void AVX128_MOVQ(OpcodeArgs);
  void AVX128_VMOVLP(OpcodeArgs);
  void AVX128_VMOVHP(OpcodeArgs);
  void AVX128_VMOVDDUP(OpcodeArgs);
  void AVX128_VMOVSLDUP(OpcodeArgs);
  void AVX128_VMOVSHDUP(OpcodeArgs);
  void AVX128_VBROADCAST(OpcodeArgs, IR::OpSize ElementSize);
  void AVX128_VPUNPCKL(OpcodeArgs, IR::OpSize ElementSize);
  void AVX128_VPUNPCKH(OpcodeArgs, IR::OpSize ElementSize);
  void AVX128_MOVVectorUnaligned(OpcodeArgs);
  void AVX128_InsertCVTGPR_To_FPR(OpcodeArgs, IR::OpSize DstElementSize);
  void AVX128_CVTFPR_To_GPR(OpcodeArgs, IR::OpSize SrcElementSize, bool HostRoundingMode);
  void AVX128_VANDN(OpcodeArgs);
  void AVX128_VPACKSS(OpcodeArgs, IR::OpSize ElementSize);
  void AVX128_VPACKUS(OpcodeArgs, IR::OpSize ElementSize);
  Ref AVX128_PSIGNImpl(IR::OpSize ElementSize, Ref Src1, Ref Src2);
  void AVX128_VPSIGN(OpcodeArgs, IR::OpSize ElementSize);
  void AVX128_UCOMISx(OpcodeArgs, IR::OpSize ElementSize);
  void AVX128_VectorScalarInsertALU(OpcodeArgs, FEXCore::IR::IROps IROp, IR::OpSize ElementSize);
  void AVX128_VFCMP(OpcodeArgs, IR::OpSize ElementSize);
  void AVX128_InsertScalarFCMP(OpcodeArgs, IR::OpSize ElementSize);
  void AVX128_MOVBetweenGPR_FPR(OpcodeArgs);
  void AVX128_PExtr(OpcodeArgs, IR::OpSize ElementSize);
  void AVX128_ExtendVectorElements(OpcodeArgs, IR::OpSize ElementSize, IR::OpSize DstElementSize, bool Signed);
  void AVX128_MOVMSK(OpcodeArgs, IR::OpSize ElementSize);
  void AVX128_MOVMSKB(OpcodeArgs);
  void AVX128_PINSRImpl(OpcodeArgs, IR::OpSize ElementSize, const X86Tables::DecodedOperand& Src1Op,
                        const X86Tables::DecodedOperand& Src2Op, const X86Tables::DecodedOperand& Imm);
  void AVX128_VPINSRB(OpcodeArgs);
  void AVX128_VPINSRW(OpcodeArgs);
  void AVX128_VPINSRDQ(OpcodeArgs);

  void AVX128_VariableShiftImpl(OpcodeArgs, IROps IROp);

  void AVX128_VINSERT(OpcodeArgs);
  void AVX128_VINSERTPS(OpcodeArgs);

  void AVX128_VPHSUB(OpcodeArgs, IR::OpSize ElementSize);
  void AVX128_VPHSUBSW(OpcodeArgs);

  void AVX128_VADDSUBP(OpcodeArgs, IR::OpSize ElementSize);

  void AVX128_VPMULL(OpcodeArgs, IR::OpSize ElementSize, bool Signed);

  void AVX128_VPMULHRSW(OpcodeArgs);

  void AVX128_VPMULHW(OpcodeArgs, bool Signed);

  void AVX128_InsertScalar_CVT_Float_To_Float(OpcodeArgs, IR::OpSize DstElementSize, IR::OpSize SrcElementSize);

  void AVX128_Vector_CVT_Float_To_Float(OpcodeArgs, IR::OpSize DstElementSize, IR::OpSize SrcElementSize);

  void AVX128_Vector_CVT_Float_To_Int(OpcodeArgs, IR::OpSize SrcElementSize, bool HostRoundingMode);

  void AVX128_Vector_CVT_Int_To_Float(OpcodeArgs, IR::OpSize SrcElementSize, bool Widen);

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

  void AVX128_VectorRound(OpcodeArgs, IR::OpSize ElementSize);
  void AVX128_InsertScalarRound(OpcodeArgs, IR::OpSize ElementSize);

  void AVX128_VDPP(OpcodeArgs, IR::OpSize ElementSize);
  void AVX128_VPERMQ(OpcodeArgs);

  void AVX128_VPSHUFW(OpcodeArgs, bool Low);

  void AVX128_VSHUF(OpcodeArgs, IR::OpSize ElementSize);

  void AVX128_VPERMILImm(OpcodeArgs, IR::OpSize ElementSize);

  void AVX128_VHADDP(OpcodeArgs, IROps IROp, IR::OpSize ElementSize);

  void AVX128_VPHADDSW(OpcodeArgs);

  void AVX128_VPMADDUBSW(OpcodeArgs);
  void AVX128_VPMADDWD(OpcodeArgs);

  void AVX128_VBLEND(OpcodeArgs, IR::OpSize ElementSize);

  void AVX128_VHSUBP(OpcodeArgs, IR::OpSize ElementSize);

  void AVX128_VPSHUFB(OpcodeArgs);
  void AVX128_VPSADBW(OpcodeArgs);

  void AVX128_VMPSADBW(OpcodeArgs);
  void AVX128_VPALIGNR(OpcodeArgs);

  void AVX128_VMASKMOVImpl(OpcodeArgs, IR::OpSize ElementSize, IR::OpSize DstSize, bool IsStore, const X86Tables::DecodedOperand& MaskOp,
                           const X86Tables::DecodedOperand& DataOp);

  void AVX128_VPMASKMOV(OpcodeArgs, bool IsStore);

  void AVX128_VMASKMOV(OpcodeArgs, IR::OpSize ElementSize, bool IsStore);

  void AVX128_MASKMOV(OpcodeArgs);

  void AVX128_VectorVariableBlend(OpcodeArgs, IR::OpSize ElementSize);

  void AVX128_SaveAVXState(Ref MemBase);
  void AVX128_RestoreAVXState(Ref MemBase);
  void AVX128_DefaultAVXState();

  void AVX128_VPERM2(OpcodeArgs);
  void AVX128_VTESTP(OpcodeArgs, IR::OpSize ElementSize);
  void AVX128_PTest(OpcodeArgs);

  void AVX128_VPERMILReg(OpcodeArgs, IR::OpSize ElementSize);

  void AVX128_VPERMD(OpcodeArgs);

  void AVX128_VPCLMULQDQ(OpcodeArgs);

  void AVX128_VFMAImpl(OpcodeArgs, IROps IROp, uint8_t Src1Idx, uint8_t Src2Idx, uint8_t AddendIdx);
  void AVX128_VFMAScalarImpl(OpcodeArgs, IROps IROp, uint8_t Src1Idx, uint8_t Src2Idx, uint8_t AddendIdx);
  void AVX128_VFMAddSubImpl(OpcodeArgs, bool AddSub, uint8_t Src1Idx, uint8_t Src2Idx, uint8_t AddendIdx);

  RefPair AVX128_VPGatherQPSImpl(OpcodeArgs, Ref Dest, Ref Mask, RefVSIB VSIB);
  RefPair AVX128_VPGatherImpl(OpcodeArgs, OpSize Size, OpSize ElementLoadSize, OpSize AddrElementSize, RefPair Dest, RefPair Mask, RefVSIB VSIB);

  void AVX128_VPGATHER(OpcodeArgs, OpSize AddrElementSize);

  void AVX128_VCVTPH2PS(OpcodeArgs);
  void AVX128_VCVTPS2PH(OpcodeArgs);

  // End of AVX 128-bit implementation

  // AVX 256-bit operations
  void StoreResult_WithAVXInsert(VectorOpType Type, RegClass Class, FEXCore::X86Tables::DecodedOp Op, Ref Value,
                                 IR::OpSize Align = IR::OpSize::iInvalid, MemoryAccessType AccessType = MemoryAccessType::DEFAULT) {
    if (Op->Dest.IsGPR() && Op->Dest.Data.GPR.GPR >= X86State::REG_XMM_0 && Op->Dest.Data.GPR.GPR <= X86State::REG_XMM_15 &&
        GetGuestVectorLength() == OpSize::i256Bit && Type == VectorOpType::SSE) {
      const auto gpr = Op->Dest.Data.GPR.GPR;
      const auto gprIndex = gpr - X86State::REG_XMM_0;
      auto DestVector = LoadXMMRegister(gprIndex);
      Value = _VInsElement(GetGuestVectorLength(), OpSize::i128Bit, 0, 0, DestVector, Value);
      StoreXMMRegister(gprIndex, Value);
      return;
    }

    StoreResult(Class, Op, Value, Align, AccessType);
  }

  void StoreXMMRegister_WithAVXInsert(VectorOpType Type, uint32_t XMM, Ref Value) {
    if (GetGuestVectorLength() == OpSize::i256Bit && Type == VectorOpType::SSE) {
      ///< SSE vector stores need to insert in the low 128-bit lane of the 256-bit register.
      auto DestVector = LoadXMMRegister(XMM);
      Value = _VInsElement(GetGuestVectorLength(), OpSize::i128Bit, 0, 0, DestVector, Value);
      StoreXMMRegister(XMM, Value);
      return;
    }
    StoreXMMRegister(XMM, Value);
  }

  void AVXVectorALUOp(OpcodeArgs, IROps IROp, IR::OpSize ElementSize);
  void AVXVectorUnaryOp(OpcodeArgs, IROps IROp, IR::OpSize ElementSize);
  void AVXVectorVariableBlend(OpcodeArgs, IR::OpSize ElementSize);

  // End of AVX 256-bit implementation

  void InvalidOp(OpcodeArgs);
  void NoExecOp(OpcodeArgs);

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

  void StoreContextHelper(IR::OpSize Size, RegClass Class, Ref Value, uint32_t Offset) {
    // For i128Bit, we won't see a normal Constant to inline, but as a special
    // case we can replace with a 2x64-bit store which can use inline zeroes.
    if (Size == OpSize::i128Bit) {
      auto Header = GetOpHeader(WrapNode(Value));
      const auto MAX_STP_OFFSET = (252 * 4);

      if (Offset <= MAX_STP_OFFSET && Header->Op == OP_LOADNAMEDVECTORCONSTANT) {
        auto Const = Header->C<IR::IROp_LoadNamedVectorConstant>();

        if (Const->Constant == IR::NamedVectorConstant::NAMED_VECTOR_ZERO) {
          Ref Zero = _Constant(0);
          Ref STP = _StoreContextPair(IR::OpSize::i64Bit, RegClass::GPR, Zero, Zero, Offset);

          // XXX: This works around InlineConstant not having an associated
          // register class, else we'd just do InlineConstant above.
          Ref InlineZero = _InlineConstant(0);
          ReplaceNodeArgument(STP, 0, InlineZero);
          ReplaceNodeArgument(STP, 1, InlineZero);
          return;
        }
      }
    }

    _StoreContext(Size, Class, Value, Offset);
  }

  void FlushRegisterCache(bool SRAOnly = false, bool MMXOnly = false) {
    // At block boundaries, fix up the carry flag.
    if (!SRAOnly) {
      RectifyCarryInvert(CFInvertedABI);
    }

    if (!MMXOnly) {
      CalculateDeferredFlags();
    }

    const auto GPRSize = GetGPROpSize();
    const auto VectorSize = GetGuestVectorLength();

    // Write backwards. This is a heuristic to improve coalescing, since we
    // often copy from (low) fixed GPRs to (high) PF/AF for celebrity
    // instructions like "add rax, 1". This hack will go away with clauses.
    uint64_t Bits = RegCache.Written;

    // We have an SRA only mode that exists as a hack to make register caching
    // less aggressive. We should get rid of this once RA can take it.
    uint64_t Mask = ~0ULL;

    if (SRAOnly) {
      const uint64_t GPRMask = ((1ull << (AFIndex - GPR0Index + 1)) - 1) << GPR0Index;
      const uint64_t FPRMask = ((1ull << (FPR15Index - FPR0Index + 1)) - 1) << FPR0Index;

      Mask &= (GPRMask | FPRMask);
      Bits &= Mask;
    }

    if (MMXOnly) {
      Mask &= ((1ull << (MM7Index - MM0Index + 1)) - 1) << MM0Index;
      Bits &= Mask;
    }

    while (Bits != 0) {
      uint32_t Index = 63 - std::countl_zero(Bits);
      Ref Value = RegCache.Value[Index];

      if (Index >= GPR0Index && Index <= GPR15Index) {
        Ref R = _StoreRegister(Value, GPRSize);
        R->Reg = PhysicalRegister(RegClass::GPRFixed, Index - GPR0Index).Raw;
      } else if (Index == PFIndex) {
        _StorePF(Value, GPRSize);
      } else if (Index == AFIndex) {
        _StoreAF(Value, GPRSize);
      } else if (Index >= FPR0Index && Index <= FPR15Index) {
        Ref R = _StoreRegister(Value, VectorSize);
        R->Reg = PhysicalRegister(RegClass::FPRFixed, Index - FPR0Index).Raw;
      } else if (Index == DFIndex) {
        _StoreContextGPR(OpSize::i8Bit, Value, offsetof(Core::CPUState, flags[X86State::RFLAG_DF_RAW_LOC]));
      } else {
        bool Partial = RegCache.Partial & (1ull << Index);
        auto Size = Partial ? OpSize::i64Bit : CacheIndexToOpSize(Index);
        uint64_t NextBit = (1ull << (Index - 1));
        uint32_t Offset = CacheIndexToContextOffset(Index);
        auto Class = CacheIndexClass(Index);
        LOGMAN_THROW_A_FMT(Offset != ~0U, "Invalid offset");

        // Use stp where possible to store multiple values at a time. This accelerates AVX.
        // TODO: this is all really confusing because of backwards iteration,
        // can we peel back that hack?
        const auto SizeInt = IR::OpSizeToSize(Size);
        if ((Bits & NextBit) && !Partial && Size >= OpSize::i32Bit && CacheIndexToContextOffset(Index - 1) == Offset - SizeInt &&
            (Offset - SizeInt) / SizeInt < 64) {
          LOGMAN_THROW_A_FMT(CacheIndexClass(Index - 1) == Class, "construction");
          LOGMAN_THROW_A_FMT((Offset % SizeInt) == 0, "construction");
          Ref ValueNext = RegCache.Value[Index - 1];

          _StoreContextPair(Size, Class, ValueNext, Value, Offset - SizeInt);
          Bits &= ~NextBit;
        } else {
          StoreContextHelper(Size, Class, Value, Offset);
          // If Partial and MMX register, then we need to store all 1s in bits 64-80
          if (Partial && Index >= MM0Index && Index <= MM7Index) {
            _StoreContextGPR(OpSize::i16Bit, Constant(0xFFFF), Offset + 8);
          }
        }
      }

      Bits &= ~(1ull << Index);
    }

    RegCache.Written &= ~Mask;
    RegCache.Cached &= ~Mask;
    RegCache.Partial &= ~Mask;
  }

  IR::OpSize GetGPROpSize() const {
    return Is64BitMode ? IR::OpSize::i64Bit : IR::OpSize::i32Bit;
  }

protected:
  void RecordX87Use() override {
    CurrentHeader->HasX87 = true;
  }

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

    case OP_VLOADVECTORMASKED:
    case OP_VLOADVECTORGATHERMASKED:
    case OP_VLOADVECTORGATHERMASKEDQPS:
    case OP_VSTOREVECTORMASKED:
      /* On ASIMD platforms, the emulation happens to preserve NZCV, unlike the
       * more optimal SVE implementation that clobbers.
       */
      if (!CTX->HostFeatures.SupportsSVE128 && !CTX->HostFeatures.SupportsSVE256) {
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
  FEX_CONFIG_OPT(ReducedPrecisionMode, X87REDUCEDPRECISION);

  struct JumpTargetInfo {
    Ref BlockEntry;
    bool HaveEmitted;
    bool IsEntryPoint;
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

  // Set if the host carry is inverted from the guest carry. This is set after
  // subtraction, because arm64 and x86 have inverted borrow flags, but clear
  // after addition.
  //
  // All CF access needs to maintain this flag. cfinv may be inserted at the end
  // of a block to rectify to the FEX convention (current convention: NOT
  // INVERTED).
  bool CFInverted {};

  // FEX convention for CF at the end of blocks: INVERTED.
  const bool CFInvertedABI {true};

  fextl::map<uint64_t, JumpTargetInfo> JumpTargets;
  bool HandledLock {false};
  bool DecodeFailure {false};
  bool NeedsBlockEnd {false};
  ForceTSOMode ForceTSO {ForceTSOMode::NoOverride};
  // Used during new op bringup
  bool ShouldDump {false};

  using SaveStoreAVXStatePtr = void (OpDispatchBuilder::*)(Ref MemBase);
  using DefaultAVXStatePtr = void (OpDispatchBuilder::*)();
  SaveStoreAVXStatePtr SaveAVXStateFunc {&OpDispatchBuilder::SaveAVXState};
  SaveStoreAVXStatePtr RestoreAVXStateFunc {&OpDispatchBuilder::RestoreAVXState};
  DefaultAVXStatePtr DefaultAVXStateFunc {&OpDispatchBuilder::DefaultAVXState};

  // Opcode helpers for generalizing behavior across VEX and non-VEX variants.

  Ref ADDSUBPOpImpl(OpSize Size, IR::OpSize ElementSize, Ref Src1, Ref Src2);

  void AVXVariableShiftImpl(OpcodeArgs, IROps IROp);

  Ref AESKeyGenAssistImpl(OpcodeArgs);

  Ref CVTGPR_To_FPRImpl(OpcodeArgs, IR::OpSize DstElementSize, const X86Tables::DecodedOperand& Src1Op, const X86Tables::DecodedOperand& Src2Op);

  Ref DPPOpImpl(IR::OpSize DstSize, Ref Src1, Ref Src2, uint8_t Mask, IR::OpSize ElementSize);

  Ref VDPPSOpImpl(OpcodeArgs, const X86Tables::DecodedOperand& Src1, const X86Tables::DecodedOperand& Src2, const X86Tables::DecodedOperand& Imm);

  Ref ExtendVectorElementsImpl(OpcodeArgs, IR::OpSize ElementSize, IR::OpSize DstElementSize, bool Signed);

  Ref HSUBPOpImpl(OpSize Size, IR::OpSize ElementSize, Ref Src1, Ref Src2);

  Ref InsertPSOpImpl(OpcodeArgs, const X86Tables::DecodedOperand& Src1, const X86Tables::DecodedOperand& Src2,
                     const X86Tables::DecodedOperand& Imm);

  Ref MPSADBWOpImpl(IR::OpSize SrcSize, Ref Src1, Ref Src2, uint8_t Select);

  Ref PALIGNROpImpl(OpcodeArgs, const X86Tables::DecodedOperand& Src1, const X86Tables::DecodedOperand& Src2,
                    const X86Tables::DecodedOperand& Imm, bool IsAVX);

  void PCMPXSTRXOpImpl(OpcodeArgs, bool IsExplicit, bool IsMask);

  Ref PHADDSOpImpl(OpSize Size, Ref Src1, Ref Src2);

  Ref PHMINPOSUWOpImpl(OpcodeArgs);

  Ref PHSUBOpImpl(OpSize Size, Ref Src1, Ref Src2, IR::OpSize ElementSize);

  Ref PHSUBSOpImpl(OpSize Size, Ref Src1, Ref Src2);

  Ref PINSROpImpl(OpcodeArgs, IR::OpSize ElementSize, const X86Tables::DecodedOperand& Src1Op, const X86Tables::DecodedOperand& Src2Op,
                  const X86Tables::DecodedOperand& Imm);

  Ref PMADDWDOpImpl(IR::OpSize Size, Ref Src1, Ref Src2);

  Ref PMADDUBSWOpImpl(IR::OpSize Size, Ref Src1, Ref Src2);

  Ref PMULHRSWOpImpl(OpSize Size, Ref Src1, Ref Src2);

  Ref PMULHWOpImpl(OpcodeArgs, bool Signed, Ref Src1, Ref Src2);

  Ref PMULLOpImpl(OpSize Size, IR::OpSize ElementSize, bool Signed, Ref Src1, Ref Src2);

  Ref PSADBWOpImpl(IR::OpSize Size, Ref Src1, Ref Src2);

  Ref GeneratePSHUFBMask(IR::OpSize SrcSize);
  Ref PSHUFBOpImpl(IR::OpSize SrcSize, Ref Src1, Ref Src2, Ref MaskVector);

  Ref PSIGNImpl(OpcodeArgs, IR::OpSize ElementSize, Ref Src1, Ref Src2);

  Ref PSLLIImpl(OpcodeArgs, IR::OpSize ElementSize, Ref Src, uint64_t Shift);

  Ref PSLLImpl(OpcodeArgs, IR::OpSize ElementSize, Ref Src, Ref ShiftVec);

  Ref PSRAOpImpl(OpcodeArgs, IR::OpSize ElementSize, Ref Src, Ref ShiftVec);

  Ref PSRLDOpImpl(OpcodeArgs, IR::OpSize ElementSize, Ref Src, Ref ShiftVec);

  Ref SHUFOpImpl(OpcodeArgs, IR::OpSize DstSize, IR::OpSize ElementSize, Ref Src1, Ref Src2, uint8_t Shuffle);

  void VMASKMOVOpImpl(OpcodeArgs, IR::OpSize ElementSize, IR::OpSize DataSize, bool IsStore, const X86Tables::DecodedOperand& MaskOp,
                      const X86Tables::DecodedOperand& DataOp);

  void MOVScalarOpImpl(OpcodeArgs, IR::OpSize ElementSize);
  void VMOVScalarOpImpl(OpcodeArgs, IR::OpSize ElementSize);

  Ref VFCMPOpImpl(OpSize Size, IR::OpSize ElementSize, Ref Src1, Ref Src2, uint8_t CompType);

  void VTESTOpImpl(OpSize SrcSize, IR::OpSize ElementSize, Ref Src1, Ref Src2);

  void VectorUnaryDuplicateOpImpl(OpcodeArgs, IROps IROp, IR::OpSize ElementSize);

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
  Ref VectorScalarInsertALUOpImpl(OpcodeArgs, IROps IROp, IR::OpSize DstSize, IR::OpSize ElementSize,
                                  const X86Tables::DecodedOperand& Src1Op, const X86Tables::DecodedOperand& Src2Op, bool ZeroUpperBits);

  Ref VectorScalarUnaryInsertALUOpImpl(OpcodeArgs, IROps IROp, IR::OpSize DstSize, IR::OpSize ElementSize,
                                       const X86Tables::DecodedOperand& Src1Op, const X86Tables::DecodedOperand& Src2Op, bool ZeroUpperBits);

  Ref InsertCVTGPR_To_FPRImpl(OpcodeArgs, IR::OpSize DstSize, IR::OpSize DstElementSize, const X86Tables::DecodedOperand& Src1Op,
                              const X86Tables::DecodedOperand& Src2Op, bool ZeroUpperBits);

  Ref InsertScalar_CVT_Float_To_FloatImpl(OpcodeArgs, IR::OpSize DstSize, IR::OpSize DstElementSize, IR::OpSize SrcElementSize,
                                          const X86Tables::DecodedOperand& Src1Op, const X86Tables::DecodedOperand& Src2Op, bool ZeroUpperBits);
  Ref InsertScalarRoundImpl(OpcodeArgs, IR::OpSize DstSize, IR::OpSize ElementSize, const X86Tables::DecodedOperand& Src1Op,
                            const X86Tables::DecodedOperand& Src2Op, uint64_t Mode, bool ZeroUpperBits);

  Ref InsertScalarFCMPOpImpl(OpSize Size, IR::OpSize OpDstSize, IR::OpSize ElementSize, Ref Src1, Ref Src2, uint8_t CompType, bool ZeroUpperBits);

  Ref VectorRoundImpl(OpSize Size, IR::OpSize ElementSize, Ref Src, uint64_t Mode);

  Ref Scalar_CVT_Float_To_FloatImpl(OpcodeArgs, IR::OpSize DstElementSize, IR::OpSize SrcElementSize,
                                    const X86Tables::DecodedOperand& Src1Op, const X86Tables::DecodedOperand& Src2Op);

  Ref CVTFPR_To_GPRImpl(OpcodeArgs, Ref Src, IR::OpSize SrcElementSize, bool HostRoundingMode);

  Ref Vector_CVT_Float_To_Int32Impl(OpcodeArgs, IR::OpSize DstSize, Ref Src, IR::OpSize SrcSize, IR::OpSize SrcElementSize,
                                    bool HostRoundingMode, bool ZeroUpperHalf);

  Ref Vector_CVT_Int_To_FloatImpl(OpcodeArgs, IR::OpSize SrcElementSize, bool Widen);

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
  Ref GetSegment(uint32_t Flags, uint32_t DefaultPrefix = FEXCore::X86Tables::DecodeFlags::FLAG_NO_PREFIX, bool Override = false);

  void UpdatePrefixFromSegment(Ref Segment, uint32_t SegmentReg);

  Ref LoadGPRRegister(uint32_t GPR, IR::OpSize Size = OpSize::iInvalid, uint8_t Offset = 0, bool AllowUpperGarbage = false);
  void StoreGPRRegister(uint32_t GPR, const Ref Src, IR::OpSize Size = OpSize::iInvalid, uint8_t Offset = 0);
  void StoreXMMRegister(uint32_t XMM, const Ref Src);

  Ref _GetRelocatedPC(const FEXCore::X86Tables::DecodedOp& Op, int64_t Offset, bool Inline) {
    const auto GPRSize = GetGPROpSize();
    const auto Offs = Op->PC + Op->InstSize + Offset - Entry;
    return Inline ? _InlineEntrypointOffset(GPRSize, Offs) : _EntrypointOffset(GPRSize, Offs);
  }

  Ref GetRelocatedPC(const FEXCore::X86Tables::DecodedOp& Op, int64_t Offset = 0) {
    return _GetRelocatedPC(Op, Offset, false);
  }

  void ExitRelocatedPC(const FEXCore::X86Tables::DecodedOp& Op, int64_t Offset = 0) {
    ExitFunction(_GetRelocatedPC(Op, Offset, true /* Inline */));
  }

  void ExitRelocatedPC(const FEXCore::X86Tables::DecodedOp& Op, int64_t Offset, BranchHint Hint, Ref CallReturnAddress, Ref CallReturnBlock) {
    ExitFunction(_GetRelocatedPC(Op, Offset, true /* Inline */), Hint, CallReturnAddress, CallReturnBlock);
  }

  [[nodiscard]]
  static bool IsOperandMem(const X86Tables::DecodedOperand& Operand, bool Load) {
    // Literals are immediates as sources but memory addresses as destinations.
    return !(Load && (Operand.IsLiteral() || Operand.IsLiteralRelocation())) && !Operand.IsGPR();
  }

  [[nodiscard]]
  static bool IsNonTSOReg(MemoryAccessType Access, uint8_t Reg) {
    return Access == MemoryAccessType::DEFAULT && Reg == X86State::REG_RSP;
  }

  AddressMode DecodeAddress(const X86Tables::DecodedOp& Op, const X86Tables::DecodedOperand& Operand, MemoryAccessType AccessType, bool IsLoad);

  Ref LoadSource(RegClass Class, const X86Tables::DecodedOp& Op, const X86Tables::DecodedOperand& Operand, uint32_t Flags,
                 const LoadSourceOptions& Options = {});
  Ref LoadSourceGPR(const X86Tables::DecodedOp& Op, const X86Tables::DecodedOperand& Operand, uint32_t Flags,
                    const LoadSourceOptions& Options = {}) {
    return LoadSource(RegClass::GPR, Op, Operand, Flags, Options);
  }
  Ref LoadSourceFPR(const X86Tables::DecodedOp& Op, const X86Tables::DecodedOperand& Operand, uint32_t Flags,
                    const LoadSourceOptions& Options = {}) {
    return LoadSource(RegClass::FPR, Op, Operand, Flags, Options);
  }

  Ref LoadSource_WithOpSize(RegClass Class, const X86Tables::DecodedOp& Op, const X86Tables::DecodedOperand& Operand, IR::OpSize OpSize,
                            uint32_t Flags, const LoadSourceOptions& Options = {});
  Ref LoadSourceGPR_WithOpSize(const X86Tables::DecodedOp& Op, const X86Tables::DecodedOperand& Operand, IR::OpSize OpSize, uint32_t Flags,
                               const LoadSourceOptions& Options = {}) {
    return LoadSource_WithOpSize(RegClass::GPR, Op, Operand, OpSize, Flags, Options);
  }
  Ref LoadSourceFPR_WithOpSize(const X86Tables::DecodedOp& Op, const X86Tables::DecodedOperand& Operand, IR::OpSize OpSize, uint32_t Flags,
                               const LoadSourceOptions& Options = {}) {
    return LoadSource_WithOpSize(RegClass::FPR, Op, Operand, OpSize, Flags, Options);
  }

  void StoreResult_WithOpSize(RegClass Class, X86Tables::DecodedOp Op, const X86Tables::DecodedOperand& Operand, Ref Src, IR::OpSize OpSize,
                              IR::OpSize Align, MemoryAccessType AccessType = MemoryAccessType::DEFAULT);
  void StoreResultGPR_WithOpSize(X86Tables::DecodedOp Op, const X86Tables::DecodedOperand& Operand, Ref Src, IR::OpSize OpSize,
                                 IR::OpSize Align = IR::OpSize::iInvalid, MemoryAccessType AccessType = MemoryAccessType::DEFAULT) {
    StoreResult_WithOpSize(RegClass::GPR, Op, Operand, Src, OpSize, Align, AccessType);
  }
  void StoreResultFPR_WithOpSize(X86Tables::DecodedOp Op, const X86Tables::DecodedOperand& Operand, Ref Src, IR::OpSize OpSize,
                                 IR::OpSize Align = IR::OpSize::iInvalid, MemoryAccessType AccessType = MemoryAccessType::DEFAULT) {
    StoreResult_WithOpSize(RegClass::FPR, Op, Operand, Src, OpSize, Align, AccessType);
  }

  void StoreResult(RegClass Class, X86Tables::DecodedOp Op, const X86Tables::DecodedOperand& Operand, Ref Src, OpSize Align,
                   MemoryAccessType AccessType = MemoryAccessType::DEFAULT);
  void StoreResultGPR(X86Tables::DecodedOp Op, const X86Tables::DecodedOperand& Operand, Ref Src, OpSize Align = OpSize::iInvalid,
                      MemoryAccessType AccessType = MemoryAccessType::DEFAULT) {
    StoreResult(RegClass::GPR, Op, Operand, Src, Align, AccessType);
  }
  void StoreResultFPR(X86Tables::DecodedOp Op, const X86Tables::DecodedOperand& Operand, Ref Src, OpSize Align = OpSize::iInvalid,
                      MemoryAccessType AccessType = MemoryAccessType::DEFAULT) {
    StoreResult(RegClass::FPR, Op, Operand, Src, Align, AccessType);
  }

  void StoreResult(RegClass Class, X86Tables::DecodedOp Op, Ref Src, OpSize Align, MemoryAccessType AccessType = MemoryAccessType::DEFAULT);
  void StoreResultGPR(X86Tables::DecodedOp Op, Ref Src, OpSize Align = OpSize::iInvalid, MemoryAccessType AccessType = MemoryAccessType::DEFAULT) {
    StoreResult(RegClass::GPR, Op, Src, Align, AccessType);
  }
  void StoreResultFPR(X86Tables::DecodedOp Op, Ref Src, OpSize Align = OpSize::iInvalid, MemoryAccessType AccessType = MemoryAccessType::DEFAULT) {
    StoreResult(RegClass::FPR, Op, Src, Align, AccessType);
  }

  // In several instances, it's desirable to get a base address with the segment offset
  // applied to it. This pulls all the common-case appending into a single set of functions.
  [[nodiscard]]
  Ref MakeSegmentAddress(const X86Tables::DecodedOp& Op, const X86Tables::DecodedOperand& Operand, IR::OpSize OpSize) {
    Ref Mem = LoadSourceGPR_WithOpSize(Op, Operand, OpSize, Op->Flags, {.LoadData = false});
    return AppendSegmentOffset(Mem, Op->Flags);
  }
  [[nodiscard]]
  Ref MakeSegmentAddress(const X86Tables::DecodedOp& Op, const X86Tables::DecodedOperand& Operand) {
    return MakeSegmentAddress(Op, Operand, OpSizeFromSrc(Op));
  }
  [[nodiscard]]
  Ref MakeSegmentAddress(X86State::X86Reg Reg, uint32_t Flags, uint32_t DefaultPrefix = 0, bool Override = false) {
    Ref Address = LoadGPRRegister(Reg);
    return AppendSegmentOffset(Address, Flags, DefaultPrefix, Override);
  }

  constexpr OpSize GetGuestVectorLength() const {
    return (CTX->HostFeatures.SupportsSVE256 && CTX->HostFeatures.SupportsAVX) ? OpSize::i256Bit : OpSize::i128Bit;
  }

  [[nodiscard]]
  static uint32_t GPROffset(X86State::X86Reg reg) {
    LOGMAN_THROW_A_FMT(reg <= X86State::X86Reg::REG_R15, "Invalid reg used");
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

  [[nodiscard]]
  IR::OpSize GetStringOpSize(X86Tables::DecodedOp Op) const;

  // Set flag tracking to prepare for an operation that directly writes NZCV.
  void HandleNZCVWrite() {
    CachedNZCV = nullptr;
    NZCVDirty = false;
  }

  // Set flag tracking to prepare for a read-modify-write operation on NZCV.
  void HandleNZCV_RMW() {
    CalculateDeferredFlags();

    if (NZCVDirty && CachedNZCV) {
      _StoreNZCV(CachedNZCV);
    }

    HandleNZCVWrite();
  }

  // Special case of the above where we are known to zero C/V
  void HandleNZ00Write() {
    HandleNZCVWrite();

    // Host carry will be implicitly zeroed, and we want guest carry zeroed as
    // well. So do not invert.
    CFInverted = false;
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
    CachedNZCV = Constant(0);
    NZCVDirty = true;
  }

  void SetNZ_ZeroCV(IR::OpSize SrcSize, Ref Res, bool SetPF = false) {
    HandleNZ00Write();

    // x - 0 = x. NZ set according to Res. C always set. V always unset. This
    // matches what we want since we want carry inverted.
    //
    // This is currently worse for 8/16-bit, but that should be optimized. TODO
    if (SrcSize >= OpSize::i32Bit) {
      if (SetPF) {
        CalculatePF(SubWithFlags(SrcSize, Res, (uint64_t)0));
      } else {
        _SubNZCV(SrcSize, Res, Constant(0));
      }

      CFInverted = true;
    } else {
      _TestNZ(SrcSize, Res, Res);
      CFInverted = false;

      if (SetPF) {
        CalculatePF(Res);
      }
    }
  }

  void SetNZP_ZeroCV(IR::OpSize SrcSize, Ref Res) {
    SetNZ_ZeroCV(SrcSize, Res, true);
  }

  void InsertNZCV(unsigned BitOffset, Ref Value, signed FlagOffset, bool MustMask) {
    signed Bit = IndexNZCV(BitOffset);

    // Heuristic to choose rmif vs msr.
    bool PreferRmif = !NZCVDirty || FlagOffset || MustMask;

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

      SetNZCV(_Bfi(OpSize::i32Bit, 1, Bit, GetNZCV(), Value));
    }
  }

  // If we don't care about N/C/V and just need Z, we can test with a simple
  // mask without any shifting.
  void SetZ_InvalidateNCV(IR::OpSize Size, Ref Src) {
    HandleNZCVWrite();
    CFInverted = true;

    if (Size < OpSize::i32Bit) {
      _TestNZ(OpSize::i32Bit, Src, _InlineConstant((1u << (IR::OpSizeAsBits(Size))) - 1));
    } else {
      _TestNZ(Size, Src, Src);
    }
  }

  // Ensure the carry invert flag matches the desired form. Used before an
  // operation reading carry or at the end of a block.
  void RectifyCarryInvert(bool RequiredInvert) {
    if (CFInverted != RequiredInvert) {
      if (CTX->HostFeatures.SupportsFlagM && !NZCVDirty) {
        // Invert as NZCV.
        _CarryInvert();
        CachedNZCV = nullptr;
      } else {
        // Invert as a GPR
        unsigned Bit = IndexNZCV(FEXCore::X86State::RFLAG_CF_RAW_LOC);
        SetNZCV(_Xor(OpSize::i32Bit, GetNZCV(), Constant(1u << Bit)));
        CalculateDeferredFlags();
      }

      CFInverted ^= true;
    }

    LOGMAN_THROW_A_FMT(CFInverted == RequiredInvert, "post condition");
  }

  void CarryInvert() {
    CFInverted ^= true;
  }

  template<unsigned BitOffset>
  void SetRFLAG(Ref Value, unsigned ValueOffset = 0, bool MustMask = false) {
    SetRFLAG(Value, BitOffset, ValueOffset, MustMask);
  }

  void SetCFDirect(Ref Value, unsigned ValueOffset = 0, bool MustMask = false) {
    Value = _Xor(OpSize::i64Bit, Value, _InlineConstant(1ull << ValueOffset));
    SetRFLAG(Value, X86State::RFLAG_CF_RAW_LOC, ValueOffset, MustMask);
    CFInverted = true;
  }

  // Set CF directly to the given 0/1 value. This needs to respect the
  // invert. We use a subtraction:
  //
  //     0 - x = 0 + (~x) + 1.
  //
  // If x = 0, then 0 + (~0) + 1 = 0x100000000 so hardware C is set.
  // If x = 1, then 0 + (~1) + 1 = 0x0ffffffff so hardware C is not set.
  void SetCFDirect_InvalidateNZV(Ref Value, unsigned ValueOffset = 0, bool MustMask = false) {
    if (ValueOffset || MustMask) {
      Value = _Bfe(OpSize::i64Bit, 1, ValueOffset, Value);
    }

    HandleNZCVWrite();
    _SubNZCV(OpSize::i32Bit, Constant(0), Value);
    CFInverted = true;
  }

  void SetCFInverted(Ref Value, unsigned ValueOffset = 0, bool MustMask = false) {
    SetRFLAG(Value, X86State::RFLAG_CF_RAW_LOC, ValueOffset, MustMask);
    CFInverted = true;
  }

  void SetRFLAG(Ref Value, unsigned BitOffset, unsigned ValueOffset = 0, bool MustMask = false) {
    if (IsNZCV(BitOffset)) {
      InsertNZCV(BitOffset, Value, ValueOffset, MustMask);
      return;
    }

    if (ValueOffset || MustMask) {
      Value = _Bfe(OpSize::i32Bit, 1, ValueOffset, Value);
    }

    if (BitOffset == FEXCore::X86State::RFLAG_PF_RAW_LOC) {
      StoreRegister(Core::CPUState::PF_AS_GREG, false, Value);
    } else if (BitOffset == FEXCore::X86State::RFLAG_AF_RAW_LOC) {
      StoreRegister(Core::CPUState::AF_AS_GREG, false, Value);
    } else if (BitOffset == FEXCore::X86State::RFLAG_DF_RAW_LOC) {
      // For DF, we need to transform 0/1 into 1/-1
      StoreDF(_SubShift(OpSize::i64Bit, Constant(1), Value, ShiftType::LSL, 1));
    } else if (BitOffset == FEXCore::X86State::RFLAG_TF_RAW_LOC) {
      auto PackedTF = _LoadContextGPR(OpSize::i8Bit, offsetof(FEXCore::Core::CPUState, flags[BitOffset]));
      // An exception should still be raised after an instruction that unsets TF, leave the unblocked bit set but unset
      // the TF bit to cause such behaviour. The handling code at the start of the next block will then unset the
      // unblocked bit before raising the exception.
      auto NewPackedTF =
        _Select(OpSize::i64Bit, OpSize::i64Bit, CondClass::EQ, Value, Constant(0), _And(OpSize::i32Bit, PackedTF, Constant(~1)), Constant(1));
      _StoreContextGPR(OpSize::i8Bit, NewPackedTF, offsetof(FEXCore::Core::CPUState, flags[BitOffset]));
    } else {
      _StoreContextGPR(OpSize::i8Bit, Value, offsetof(FEXCore::Core::CPUState, flags[BitOffset]));
    }
  }

  void SetAF(unsigned K) {
    // AF is stored in bit 4 of the AF flag byte, with garbage in the other
    // bits. This allows us to defer the extract in the usual case. When it is
    // read, bit 4 is extracted.  In order to write a constant value of AF, that
    // means we need to left-shift here to compensate.
    SetRFLAG<FEXCore::X86State::RFLAG_AF_RAW_LOC>(Constant(K << 4));
  }

  void ZeroPF_AF();

  void InvalidateAF() {
    _InvalidateFlags((1u << X86State::RFLAG_AF_RAW_LOC));
    InvalidateReg(Core::CPUState::AF_AS_GREG);
  }

  void InvalidatePF_AF() {
    _InvalidateFlags((1u << X86State::RFLAG_PF_RAW_LOC) | (1u << X86State::RFLAG_AF_RAW_LOC));
    InvalidateReg(Core::CPUState::PF_AS_GREG);
    InvalidateReg(Core::CPUState::AF_AS_GREG);
  }

  [[nodiscard]]
  static CondClass CondForNZCVBit(unsigned BitOffset, bool Invert) {
    switch (BitOffset) {
    case X86State::RFLAG_SF_RAW_LOC: return Invert ? CondClass::PL : CondClass::MI;
    case X86State::RFLAG_ZF_RAW_LOC: return Invert ? CondClass::NEQ : CondClass::EQ;
    case X86State::RFLAG_CF_RAW_LOC: return Invert ? CondClass::ULT : CondClass::UGE;
    case X86State::RFLAG_OF_RAW_LOC: return Invert ? CondClass::FNU : CondClass::FU;
    default: FEX_UNREACHABLE;
    }
  }

  /* Layout of cache indices. We use a single 64-bit bitmask for the cache */
  static const int GPR0Index = 0;
  static const int GPR15Index = 15;
  static const int PFIndex = 16;
  static const int AFIndex = 17;
  /* Gap 18..19 */
  /* Note this range is only valid if MMXState = MMXState_MMX */
  static const int MM0Index = 20;
  static const int MM7Index = 27;
  /* Gap 28..30 */
  static const int DFIndex = 31;
  static const int FPR0Index = 32;
  static const int FPR15Index = 47;
  static const int AVXHigh0Index = 48;
  static const int AVXHigh15Index = 63;

  [[nodiscard]]
  static uint32_t CacheIndexToContextOffset(int Index) {
    switch (Index) {
    case MM0Index ... MM7Index: return offsetof(FEXCore::Core::CPUState, mm[Index - MM0Index]);
    case AVXHigh0Index ... AVXHigh15Index: return offsetof(FEXCore::Core::CPUState, avx_high[Index - AVXHigh0Index][0]);
    default: return ~0U;
    }
  }

  [[nodiscard]]
  static RegClass CacheIndexClass(int Index) {
    if ((Index >= MM0Index && Index <= MM7Index) || Index >= FPR0Index) {
      return RegClass::FPR;
    } else {
      return RegClass::GPR;
    }
  }

  [[nodiscard]]
  static IR::OpSize CacheIndexToOpSize(int Index) {
    // MMX registers are rounded up to 128-bit since they are shared with 80-bit
    // x87 registers, even though MMX is logically only 64-bit.
    if (Index >= AVXHigh0Index || ((Index >= MM0Index && Index <= MM7Index))) {
      return OpSize::i128Bit;
    } else {
      return OpSize::i8Bit;
    }
  }

  struct {
    uint64_t Cached;
    uint64_t Written;

    // Indicates that Value contains only the lower 64-bit of the full 80-bit
    // register. Used for MMX/x87 optimization.
    uint64_t Partial;

    Ref Value[64];
  } RegCache {};

  void InvalidateReg(uint8_t Index) {
    uint64_t Bit = (1ull << (uint64_t)Index);
    RegCache.Cached &= ~Bit;
    RegCache.Written &= ~Bit;
  }

  Ref LoadRegCache(uint64_t Offset, uint8_t Index, RegClass Class, IR::OpSize Size) {
    LOGMAN_THROW_A_FMT(Index < 64, "valid index");
    uint64_t Bit = (1ull << (uint64_t)Index);

    if (Size == OpSize::i128Bit && (RegCache.Partial & Bit)) {
      // We need to load the full register extend if we previously did a partial access.
      Ref Value = RegCache.Value[Index];
      Ref Full = _LoadContext(Size, Class, Offset);

      // If we did a partial store, we're inserting into the full register
      if (RegCache.Written & Bit) {
        Full = _VInsElement(OpSize::i128Bit, OpSize::i64Bit, 0, 0, Full, Value);
      }

      RegCache.Value[Index] = Full;
    }

    if (!(RegCache.Cached & Bit)) {
      if (Index == DFIndex) {
        RegCache.Value[Index] = _LoadDF();
      } else if ((Index >= MM0Index && Index <= MM7Index) || Index >= AVXHigh0Index) {
        RegCache.Value[Index] = _LoadContext(Size, Class, Offset);

        // We may have done a partial load, this requires special handling.
        if (Size == OpSize::i64Bit) {
          RegCache.Partial |= Bit;
        }
      } else if (Index == PFIndex) {
        RegCache.Value[Index] = _LoadPF(Size);
      } else if (Index == AFIndex) {
        RegCache.Value[Index] = _LoadAF(Size);
      } else {
        RegCache.Value[Index] = _LoadRegister(Offset, Class, Size);
      }

      RegCache.Cached |= Bit;
    }

    return RegCache.Value[Index];
  }

  RefPair AllocatePair(RegClass Class, IR::OpSize Size) {
    if (Class == RegClass::FPR) {
      return {_AllocateFPR(Size, Size), _AllocateFPR(Size, Size)};
    } else {
      return {_AllocateGPR(false), _AllocateGPR(false)};
    }
  }

  RefPair LoadContextPair_Uncached(RegClass Class, IR::OpSize Size, unsigned Offset) {
    RefPair Values = AllocatePair(Class, Size);
    _LoadContextPair(Size, Class, Offset, Values.Low, Values.High);
    return Values;
  }

  RefPair LoadRegCachePair(uint64_t Offset, uint8_t Index, RegClass Class, IR::OpSize Size) {
    LOGMAN_THROW_A_FMT(Index != DFIndex, "must be pairable");
    LOGMAN_THROW_A_FMT(Size != IR::OpSize::iUnsized, "Invalid size!");

    // Try to load a pair into the cache
    uint64_t Bits = (3ull << (uint64_t)Index);
    const auto SizeInt = IR::OpSizeToSize(Size);
    if (((RegCache.Partial | RegCache.Cached) & Bits) == 0 && ((Offset / SizeInt) < 64)) {
      auto Values = LoadContextPair_Uncached(Class, Size, Offset);
      RegCache.Value[Index] = Values.Low;
      RegCache.Value[Index + 1] = Values.High;
      RegCache.Cached |= Bits;
      if (Size == OpSize::i64Bit) {
        RegCache.Partial |= Bits;
      }
      return Values;
    }

    // Fallback on a pair of loads
    return {
      .Low = LoadRegCache(Offset, Index, Class, Size),
      .High = LoadRegCache(Offset + SizeInt, Index + 1, Class, Size),
    };
  }

  Ref LoadGPR(uint8_t Reg) {
    return LoadRegCache(Reg, GPR0Index + Reg, RegClass::GPR, GetGPROpSize());
  }

  Ref LoadContext(IR::OpSize Size, uint8_t Index) {
    return LoadRegCache(CacheIndexToContextOffset(Index), Index, CacheIndexClass(Index), Size);
  }

  RefPair LoadContextPair(IR::OpSize Size, uint8_t Index) {
    return LoadRegCachePair(CacheIndexToContextOffset(Index), Index, CacheIndexClass(Index), Size);
  }

  Ref LoadContext(uint8_t Index) {
    return LoadContext(CacheIndexToOpSize(Index), Index);
  }

  Ref LoadXMMRegister(uint8_t Reg) {
    return LoadRegCache(Reg, FPR0Index + Reg, RegClass::FPR, GetGuestVectorLength());
  }

  Ref LoadDF() {
    return LoadGPR(DFIndex);
  }

  void StoreContext(uint8_t Index, Ref Value) {
    LOGMAN_THROW_A_FMT(Index < 64, "valid index");
    LOGMAN_THROW_A_FMT(Value != InvalidNode, "storing valid");

    uint64_t Bit = (1ull << (uint64_t)Index);

    RegCache.Value[Index] = Value;
    RegCache.Cached |= Bit;
    RegCache.Written |= Bit;
  }

  void InvalidateHighAVXRegisters() {
    for (size_t i = 0; i < 16; ++i) {
      InvalidateReg(AVXHigh0Index + i);
    }
  }

  void StoreRegister(uint8_t Reg, bool FPR, Ref Value) {
    StoreContext(Reg + (FPR ? FPR0Index : GPR0Index), Value);
  }

  void StoreDF(Ref Value) {
    StoreContext(DFIndex, Value);
  }

  Ref GetRFLAG(unsigned BitOffset, bool Invert = false) {
    if (IsNZCV(BitOffset)) {
      // Handle the CFInverted state internally so GetRFLAG is safe regardless
      // of the invert state. This simplifies the call sites.
      if (BitOffset == X86State::RFLAG_CF_RAW_LOC) {
        Invert ^= CFInverted;
      }

      if (NZCVDirty) {
        auto Value = _Bfe(OpSize::i32Bit, 1, IndexNZCV(BitOffset), GetNZCV());

        if (Invert) {
          return _Xor(OpSize::i32Bit, Value, Constant(1));
        } else {
          return Value;
        }
      } else {
        // Because we explicitly inverted for CF above, we use the unsafe
        // _NZCVSelect rather than the safe CF-aware version.
        return _NZCVSelect01(CondForNZCVBit(BitOffset, Invert));
      }
    } else if (BitOffset == FEXCore::X86State::RFLAG_PF_RAW_LOC) {
      return LoadGPR(Core::CPUState::PF_AS_GREG);
    } else if (BitOffset == FEXCore::X86State::RFLAG_AF_RAW_LOC) {
      return LoadGPR(Core::CPUState::AF_AS_GREG);
    } else if (BitOffset == FEXCore::X86State::RFLAG_DF_RAW_LOC) {
      // Recover the sign bit, it is the logical DF value
      return _Lshr(OpSize::i64Bit, LoadDF(), Constant(63));
    } else {
      return _LoadContextGPR(OpSize::i8Bit, offsetof(Core::CPUState, flags[BitOffset]));
    }
  }

  // Returns (DF ? -Size : Size)
  Ref LoadDir(const unsigned Size) {
    return ARef(LoadDF()).Lshl(FEXCore::ilog2(Size)).Ref();
  }

  // Returns DF ? (X - Size) : (X + Size)
  Ref OffsetByDir(Ref X, const unsigned Size) {
    auto Shift = FEXCore::ilog2(Size);

    return _AddShift(OpSize::i64Bit, X, LoadDF(), ShiftType::LSL, Shift);
  }

  // Safe version of NZCVSelect that handles inverted carries automatically.
  Ref NZCVSelect(OpSize OpSize, CondClass Cond, Ref TrueV, Ref FalseV, bool CarryIsInverted = false) {
    switch (Cond) {
    case CondClass::UGE: /* cs */
    case CondClass::ULT: /* cc */
      // Invert the condition to match our expectations.
      if (CarryIsInverted != CFInverted) {
        Cond = (Cond == CondClass::UGE) ? CondClass::ULT : CondClass::UGE;
      }
      break;

    case CondClass::UGT: /* hi */
    case CondClass::ULE: /* ls */
      // No clever optimization we can do here, rectify carry itself.
      RectifyCarryInvert(CarryIsInverted);
      break;

    default:
      // No other condition codes read carry so no need to rectify.
      break;
    }

    return _NZCVSelect(OpSize, Cond, TrueV, FalseV);
  }

  // Compares two floats and sets flags for a COMISS instruction
  void Comiss(IR::OpSize ElementSize, Ref Src1, Ref Src2, bool InvalidateAF = false) {
    // First, set flags according to Arm FCMP.
    HandleNZCVWrite();
    _FCmp(ElementSize, Src1, Src2);
    CFInverted = false;
    ComissFlags(InvalidateAF);
  }

  // Sets flags for a COMISS instruction
  void ComissFlags(bool InvalidateAF = false) {
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
    Ref V_inv = GetRFLAG(FEXCore::X86State::RFLAG_OF_RAW_LOC, true);
    SetRFLAG<FEXCore::X86State::RFLAG_PF_RAW_LOC>(V_inv);

    if (!InvalidateAF) {
      // Zero AF. Note that the comparison sets the raw PF to 0/1 above, so
      // PF[4] is 0 so the XOR with PF will have no effect, so setting the AF
      // byte to zero will indeed zero AF as intended.
      SetRFLAG<FEXCore::X86State::RFLAG_AF_RAW_LOC>(Constant(0));
    }

    // Convert NZCV from the Arm representation to an eXternal representation
    // that's totally not a euphemism for x86, nuh-uh. But maps to exactly we
    // need, what a coincidence!
    //
    // Our AXFlag emulation on FlagM2-less systems needs V_inv passed.
    _AXFlag(CTX->HostFeatures.SupportsFlagM2 ? Invalid() : V_inv);
    CFInverted = true;
  }

  // Set x87 comparison flags based on the result set by Arm FCMP. Clobbers
  // NZCV on flagm2 platforms.
  void ConvertNZCVToX87() {
    LOGMAN_THROW_A_FMT(NZCVDirty && CachedNZCV, "NZCV must be saved");

    Ref V = _NZCVSelect01(CondForNZCVBit(FEXCore::X86State::RFLAG_OF_RAW_LOC, false));

    if (CTX->HostFeatures.SupportsFlagM2) {
      // Convert to x86 flags, saves us from or'ing after.
      _AXFlag(Invalid());
    }

    // CF is inverted after FCMP
    Ref C = _NZCVSelect01(CondForNZCVBit(FEXCore::X86State::RFLAG_CF_RAW_LOC, true));
    Ref Z = _NZCVSelect01(CondForNZCVBit(FEXCore::X86State::RFLAG_ZF_RAW_LOC, false));

    if (!CTX->HostFeatures.SupportsFlagM2) {
      C = _Or(OpSize::i32Bit, C, V);
      Z = _Or(OpSize::i32Bit, Z, V);
    }

    SetRFLAG<FEXCore::X86State::X87FLAG_C0_LOC>(C);
    SetRFLAG<FEXCore::X86State::X87FLAG_C1_LOC>(Constant(0));
    SetRFLAG<FEXCore::X86State::X87FLAG_C2_LOC>(V);
    SetRFLAG<FEXCore::X86State::X87FLAG_C3_LOC>(Z);
  }

  // Helper to store a variable shift and calculate its flags for a variable
  // shift, with correct PF handling.
  void HandleShift(X86Tables::DecodedOp Op, Ref Result, Ref Dest, ShiftType Shift, Ref Src) {

    auto OldPF = GetRFLAG(X86State::RFLAG_PF_RAW_LOC);

    HandleNZCV_RMW();
    CalculatePF(_ShiftFlags(OpSizeFromSrc(Op), Result, Dest, Shift, Src, OldPF, CFInverted));
    StoreResultGPR(Op, Result);
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
  Ref LoadAndCacheNamedVectorConstant(IR::OpSize Size, FEXCore::IR::NamedVectorConstant NamedConstant) {
    auto log2_size_bytes = FEXCore::ilog2(IR::OpSizeToSize(Size));
    if (CachedNamedVectorConstants[NamedConstant][log2_size_bytes]) {
      return CachedNamedVectorConstants[NamedConstant][log2_size_bytes];
    }

    auto K = _LoadNamedVectorConstant(Size, NamedConstant);
    CachedNamedVectorConstants[NamedConstant][log2_size_bytes] = K;
    return K;
  }
  Ref LoadAndCacheIndexedNamedVectorConstant(IR::OpSize Size, FEXCore::IR::IndexNamedVectorConstant NamedIndexedConstant, uint32_t Index) {
    IndexNamedVectorMapKey Key {
      .Index = Index,
      .NamedIndexedConstant = NamedIndexedConstant,
      .log2_size_in_bytes = FEXCore::ilog2(IR::OpSizeToSize(Size)),
    };
    auto it = CachedIndexedNamedVectorConstants.find(Key);

    if (it != CachedIndexedNamedVectorConstants.end()) {
      return it->second;
    }

    auto K = _LoadNamedVectorIndexedConstant(Size, NamedIndexedConstant, Index);
    CachedIndexedNamedVectorConstants.insert_or_assign(Key, K);
    return K;
  }

  Ref LoadUncachedZeroVector(IR::OpSize Size) {
    return _LoadNamedVectorConstant(Size, IR::NamedVectorConstant::NAMED_VECTOR_ZERO);
  }

  Ref LoadZeroVector(IR::OpSize Size) {
    return LoadAndCacheNamedVectorConstant(Size, IR::NamedVectorConstant::NAMED_VECTOR_ZERO);
  }

  // Reset the named vector constants cache array.
  // These are only cached per block.
  void ClearCachedNamedConstants() {
    memset(CachedNamedVectorConstants, 0, sizeof(CachedNamedVectorConstants));
    CachedIndexedNamedVectorConstants.clear();
  }

  std::optional<CondClass> DecodeNZCVCondition(uint8_t OP);
  Ref SelectCC0All1(uint8_t OP);

  /**
   * @brief Flushes NZCV. Mostly vestigial.
   */
  void CalculateDeferredFlags();

  void ZeroShiftResult(FEXCore::X86Tables::DecodedOp Op) {
    // In the case of zero-rotate, we need to store the destination still to deal with 32-bit semantics.
    const auto Size = OpSizeFromSrc(Op);
    if (Size != OpSize::i32Bit) {
      return;
    }
    auto Dest = LoadSourceGPR(Op, Op->Dest, Op->Flags);
    StoreResultGPR(Op, Dest);
  }

  using ZeroShiftFunctionPtr = void (OpDispatchBuilder::*)(FEXCore::X86Tables::DecodedOp Op);

  template<typename F>
  void Calculate_ShiftVariable(FEXCore::X86Tables::DecodedOp Op, Ref Shift, F&& Calculate,
                               std::optional<ZeroShiftFunctionPtr> ZeroShiftResult = std::nullopt) {
    // RCR can call this with constants, so handle that without branching.
    uint64_t Const;
    if (IsValueConstant(WrapNode(Shift), &Const)) {
      if (Const) {
        Calculate();
      } else if (ZeroShiftResult) {
        (this->*(*ZeroShiftResult))(Op);
      }

      return;
    }

    // Otherwise, prepare to branch.
    auto Zero = Constant(0);

    // If the shift is zero, do not touch the flags.
    auto SetBlock = CreateNewCodeBlockAfter(GetCurrentBlock());
    IRPair<IROp_CodeBlock> NextBlock = SetBlock;
    IRPair<IROp_CodeBlock> ZeroShiftBlock;
    if (ZeroShiftResult) {
      ZeroShiftBlock = CreateNewCodeBlockAfter(NextBlock);
      NextBlock = ZeroShiftBlock;
    }
    auto EndBlock = CreateNewCodeBlockAfter(NextBlock);

    ///< Jump to zeroshift block or end block depending on if it was provided.
    IRPair<IROp_CodeBlock> TailHandling = ZeroShiftResult ? ZeroShiftBlock : EndBlock;
    CondJump(Shift, Zero, TailHandling, SetBlock, CondClass::EQ);

    SetCurrentCodeBlock(SetBlock);
    StartNewBlock();
    {
      Calculate();
      Jump(EndBlock);
    }

    if (ZeroShiftResult) {
      SetCurrentCodeBlock(ZeroShiftBlock);
      StartNewBlock();
      {
        (this->*(*ZeroShiftResult))(Op);
        Jump(EndBlock);
      }
    }

    SetCurrentCodeBlock(EndBlock);
    StartNewBlock();
  }

  /**
   * @name These functions are used by the deferred flag handling while it is calculating and storing flags in to RFLAGs.
   * @{ */
  Ref LoadPFRaw(bool Mask, bool Invert);
  Ref LoadAF();
  void FixupAF();
  void SetAFAndFixup(Ref AF);
  Ref CalculateAFForDecimal(Ref A);
  void CalculatePF(Ref Res);
  void CalculateAF(Ref Src1, Ref Src2);

  Ref IncrementByCarry(OpSize OpSize, Ref Src);

  void CalculateOF(IR::OpSize SrcSize, Ref Res, Ref Src1, Ref Src2, bool Sub);
  Ref CalculateFlags_ADC(IR::OpSize SrcSize, Ref Src1, Ref Src2);
  Ref CalculateFlags_SBB(IR::OpSize SrcSize, Ref Src1, Ref Src2);
  Ref CalculateFlags_SUB(IR::OpSize SrcSize, Ref Src1, Ref Src2, bool UpdateCF = true);
  Ref CalculateFlags_ADD(IR::OpSize SrcSize, Ref Src1, Ref Src2, bool UpdateCF = true);
  void CalculateFlags_MUL(IR::OpSize SrcSize, Ref Res, Ref High);
  void CalculateFlags_UMUL(Ref High);
  void CalculateFlags_Logical(IR::OpSize SrcSize, Ref Res);
  void CalculateFlags_ShiftLeftImmediate(IR::OpSize SrcSize, Ref Res, Ref Src1, uint64_t Shift);
  void CalculateFlags_ShiftRightImmediate(IR::OpSize SrcSize, Ref Res, Ref Src1, uint64_t Shift);
  void CalculateFlags_ShiftRightDoubleImmediate(IR::OpSize SrcSize, Ref Res, Ref Src1, uint64_t Shift);
  void CalculateFlags_ShiftRightImmediateCommon(IR::OpSize SrcSize, Ref Res, Ref Src1, uint64_t Shift);
  void CalculateFlags_SignShiftRightImmediate(IR::OpSize SrcSize, Ref Res, Ref Src1, uint64_t Shift);
  void CalculateFlags_ZCNT(IR::OpSize SrcSize, Ref Result);
  /**  @} */

  Ref GetX87Top();
  void SetX87FTW(Ref FTW);
  Ref GetX87FTW_Helper();
  void SetX87Top(Ref Value);

  void ChgStateX87_MMX() override {
    LOGMAN_THROW_A_FMT(MMXState == MMXState_X87, "Expected state to be x87");
    _StackForceSlow();
    SetX87Top(Constant(0)); // top reset to zero
    _StoreContextGPR(OpSize::i8Bit, Constant(0xFFFFUL), offsetof(FEXCore::Core::CPUState, AbridgedFTW));
    MMXState = MMXState_MMX;
  }

  void ChgStateMMX_X87() override {
    LOGMAN_THROW_A_FMT(MMXState == MMXState_MMX, "Expected state to be MMX");
    // The opcode dispatcher register cache is used for MMX, but the x87 pass register cache is used for x87, spill to
    // context to ensure coherence.
    FlushRegisterCache(false, true);
    // We explicitly initialize to x87 state in StartNewBlock.
    // So if we ever change this to do something else, we need to
    // make sure that we consider if we need to explicitly set it there.
    MMXState = MMXState_X87;
  }

  bool DestIsLockedMem(FEXCore::X86Tables::DecodedOp Op) const {
    return DestIsMem(Op) && (Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_LOCK) != 0;
  }

  bool DestIsMem(FEXCore::X86Tables::DecodedOp Op) const {
    return !Op->Dest.IsGPR();
  }

  void CreateJumpBlocks(const fextl::vector<FEXCore::Frontend::Decoder::DecodedBlocks>* Blocks);
  bool BlockSetRIP {false};

  bool Multiblock {};
  bool Is64BitMode {};
  uint64_t Entry {};

  // Set if mono hacks are enabled and the current block is the mono callsite backpatcher, in which case the
  // XCHG ops that would patch code are replaced with a hook that performs the write and manually invalidates
  // the target address.
  bool IsMonoBackpatcherBlock {false};
  IROp_IRHeader* CurrentHeader {};

  [[nodiscard]]
  bool IsTSOEnabled(RegClass Class) const {
    if (ForceTSO == ForceTSOMode::ForceEnabled) {
      return true;
    } else if (ForceTSO == ForceTSOMode::ForceDisabled) {
      return false;
    } else if (Class == RegClass::FPR) {
      return CTX->IsVectorAtomicTSOEnabled();
    } else {
      return CTX->IsAtomicTSOEnabled();
    }
  }

  Ref _StoreMemAutoTSO(RegClass Class, OpSize Size, Ref Addr, Ref Value, OpSize Align = OpSize::i8Bit) {
    if (IsTSOEnabled(Class)) {
      return _StoreMemTSO(Class, Size, Value, Addr, Invalid(), Align, MemOffsetType::SXTX, 1);
    } else {
      return _StoreMem(Class, Size, Value, Addr, Invalid(), Align, MemOffsetType::SXTX, 1);
    }
  }
  Ref _StoreMemGPRAutoTSO(OpSize Size, Ref Addr, Ref Value, OpSize Align = OpSize::i8Bit) {
    return _StoreMemAutoTSO(RegClass::GPR, Size, Addr, Value, Align);
  }
  Ref _StoreMemFPRAutoTSO(OpSize Size, Ref Addr, Ref Value, OpSize Align = OpSize::i8Bit) {
    return _StoreMemAutoTSO(RegClass::FPR, Size, Addr, Value, Align);
  }

  Ref _LoadMemAutoTSO(RegClass Class, OpSize Size, Ref ssa0, OpSize Align = OpSize::i8Bit) {
    if (IsTSOEnabled(Class)) {
      return _LoadMemTSO(Class, Size, ssa0, Invalid(), Align, MemOffsetType::SXTX, 1);
    } else {
      return _LoadMem(Class, Size, ssa0, Invalid(), Align, MemOffsetType::SXTX, 1);
    }
  }
  Ref _LoadMemGPRAutoTSO(OpSize Size, Ref ssa0, OpSize Align = OpSize::i8Bit) {
    return _LoadMemAutoTSO(RegClass::GPR, Size, ssa0, Align);
  }
  Ref _LoadMemFPRAutoTSO(OpSize Size, Ref ssa0, OpSize Align = OpSize::i8Bit) {
    return _LoadMemAutoTSO(RegClass::FPR, Size, ssa0, Align);
  }

  Ref _LoadMemAutoTSO(RegClass Class, OpSize Size, const AddressMode& A, OpSize Align = OpSize::i8Bit) {
    const bool AtomicTSO = IsTSOEnabled(Class) && !A.NonTSO;
    const auto B = SelectAddressMode(this, A, GetGPROpSize(), CTX->HostFeatures.SupportsTSOImm9, AtomicTSO, Class != RegClass::GPR, Size);

    if (AtomicTSO) {
      return _LoadMemTSO(Class, Size, B.Base, B.Index, Align, B.IndexType, B.IndexScale);
    } else {
      return _LoadMem(Class, Size, B.Base, B.Index, Align, B.IndexType, B.IndexScale);
    }
  }
  Ref _LoadMemGPRAutoTSO(OpSize Size, const AddressMode& A, OpSize Align = OpSize::i8Bit) {
    return _LoadMemAutoTSO(RegClass::GPR, Size, A, Align);
  }
  Ref _LoadMemFPRAutoTSO(OpSize Size, const AddressMode& A, OpSize Align = OpSize::i8Bit) {
    return _LoadMemAutoTSO(RegClass::FPR, Size, A, Align);
  }

  AddressMode SelectPairAddressMode(AddressMode A, IR::OpSize Size) {
    LOGMAN_THROW_A_FMT(Size != IR::OpSize::iUnsized, "Invalid size!");
    const auto SizeInt = IR::OpSizeToSize(Size);
    AddressMode Out {};

    signed OffsetEl = A.Offset / SizeInt;
    if ((A.Offset % SizeInt) == 0 && OffsetEl >= -64 && OffsetEl < 64) {
      Out.Offset = A.Offset;
      A.Offset = 0;
    }

    Out.Base = LoadEffectiveAddress(this, A, GetGPROpSize(), true, false);
    return Out;
  }


  RefPair LoadMemPair(RegClass Class, OpSize Size, Ref Base, uint32_t Offset) {
    RefPair Values = AllocatePair(Class, Size);
    _LoadMemPair(Class, Size, Base, Offset, Values.Low, Values.High);
    return Values;
  }
  RefPair LoadMemPairFPR(OpSize Size, Ref Base, uint32_t Offset) {
    return LoadMemPair(RegClass::FPR, Size, Base, Offset);
  }

  RefPair _LoadMemPairAutoTSO(RegClass Class, OpSize Size, const AddressMode& A, OpSize Align = OpSize::i8Bit) {
    const bool AtomicTSO = IsTSOEnabled(Class) && !A.NonTSO;

    // Use ldp if possible, otherwise fallback on two loads.
    if (!AtomicTSO && !A.Segment && Size >= OpSize::i32Bit && Size <= OpSize::i128Bit) {
      const auto B = SelectPairAddressMode(A, Size);
      return LoadMemPair(Class, Size, B.Base, B.Offset);
    }

    AddressMode HighA = A;
    HighA.Offset += 16;

    return {
      .Low = _LoadMemAutoTSO(Class, Size, A, Align),
      .High = _LoadMemAutoTSO(Class, Size, HighA, Align),
    };
  }
  RefPair _LoadMemPairFPRAutoTSO(OpSize Size, const AddressMode& A, OpSize Align = OpSize::i8Bit) {
    return _LoadMemPairAutoTSO(RegClass::FPR, Size, A, Align);
  }

  Ref _StoreMemAutoTSO(RegClass Class, OpSize Size, const AddressMode& A, Ref Value, OpSize Align = OpSize::i8Bit) {
    const bool AtomicTSO = IsTSOEnabled(Class) && !A.NonTSO;
    const auto B = SelectAddressMode(this, A, GetGPROpSize(), CTX->HostFeatures.SupportsTSOImm9, AtomicTSO, Class != RegClass::GPR, Size);

    if (AtomicTSO) {
      return _StoreMemTSO(Class, Size, Value, B.Base, B.Index, Align, B.IndexType, B.IndexScale);
    } else {
      return _StoreMem(Class, Size, Value, B.Base, B.Index, Align, B.IndexType, B.IndexScale);
    }
  }
  Ref _StoreMemGPRAutoTSO(OpSize Size, const AddressMode& A, Ref Value, OpSize Align = OpSize::i8Bit) {
    return _StoreMemAutoTSO(RegClass::GPR, Size, A, Value, Align);
  }
  Ref _StoreMemFPRAutoTSO(OpSize Size, const AddressMode& A, Ref Value, OpSize Align = OpSize::i8Bit) {
    return _StoreMemAutoTSO(RegClass::FPR, Size, A, Value, Align);
  }

  void _StoreMemPairAutoTSO(RegClass Class, OpSize Size, const AddressMode& A, Ref Value1, Ref Value2, OpSize Align = OpSize::i8Bit) {
    const auto SizeInt = IR::OpSizeToSize(Size);
    const bool AtomicTSO = IsTSOEnabled(Class) && !A.NonTSO;

    // Use stp if possible, otherwise fallback on two stores.
    if (!AtomicTSO && !A.Segment && Size >= OpSize::i32Bit && Size <= OpSize::i128Bit) {
      const auto B = SelectPairAddressMode(A, Size);
      _StoreMemPair(Class, Size, Value1, Value2, B.Base, B.Offset);
    } else {
      auto B = A;

      _StoreMemAutoTSO(Class, Size, B, Value1, OpSize::i8Bit);
      B.Offset += SizeInt;
      _StoreMemAutoTSO(Class, Size, B, Value2, OpSize::i8Bit);
    }
  }
  void _StoreMemPairFPRAutoTSO(OpSize Size, const AddressMode& A, Ref Value1, Ref Value2, OpSize Align = OpSize::i8Bit) {
    return _StoreMemPairAutoTSO(RegClass::FPR, Size, A, Value1, Value2, Align);
  }

  Ref Pop(IR::OpSize Size, Ref SP_RMW) {
    Ref Value = _AllocateGPR(false);
    _Pop(Size, SP_RMW, Value);
    return Value;
  }

  Ref Pop(IR::OpSize Size) {
    Ref SP = _RMWHandle(LoadGPRRegister(X86State::REG_RSP));
    Ref Value = _AllocateGPR(false);

    _Pop(Size, SP, Value);

    // Store the new stack pointer
    StoreGPRRegister(X86State::REG_RSP, SP);
    return Value;
  }

  Ref VZeroExtendOperand(OpSize Size, X86Tables::DecodedOperand Op, Ref Value) {
    bool IsMMX = Op.IsGPR() && Op.Data.GPR.GPR >= X86State::REG_MM_0;
    bool AlreadyExtended = Op.IsGPRDirect() || Op.IsGPRIndirect() || IsMMX;

    return AlreadyExtended ? Value : _VMov(Size, Value);
  }

  void Push(IR::OpSize Size, Ref Value) {
    auto OldSP = LoadGPRRegister(X86State::REG_RSP);
    auto NewSP = _Push(GetGPROpSize(), Size, Value, OldSP);
    StoreGPRRegister(X86State::REG_RSP, NewSP);
    FlushRegisterCache();
  }

  struct ArithRef {
    IREmitter* E {};
    bool IsConstant {};
    union {
      Ref R {};
      uint64_t C;
    };

    ArithRef() {}

    ArithRef(IREmitter* IREmit, Ref Reference)
      : E(IREmit)
      , IsConstant(false)
      , R(Reference) {}

    ArithRef(IREmitter* IREmit, uint64_t K)
      : E(IREmit)
      , IsConstant(true)
      , C(K) {}

    ArithRef Neg() {
      return IsConstant ? ArithRef(E, -C) : ArithRef(E, E->_Neg(OpSize::i64Bit, R));
    }

    ArithRef And(uint64_t K) {
      return IsConstant ? ArithRef(E, C & K) : ArithRef(E, E->_And(OpSize::i64Bit, R, E->Constant(K)));
    }

    ArithRef Presub(uint64_t K) {
      return IsConstant ? ArithRef(E, K - C) : ArithRef(E, E->Sub(OpSize::i64Bit, E->Constant(K), R));
    }

    ArithRef Lshl(uint64_t Shift) {
      if (Shift == 0) {
        return *this;
      } else if (IsConstant) {
        return ArithRef(E, C << Shift);
      } else {
        return ArithRef(E, E->_Lshl(OpSize::i64Bit, R, E->Constant(Shift)));
      }
    }

    ArithRef Bfe(unsigned Start, unsigned Size) {
      if (IsConstant) {
        return ArithRef(E, (C >> Start) & ((1ull << Size) - 1));
      } else {
        return ArithRef(E, E->_Bfe(OpSize::i64Bit, Size, Start, R));
      }
    }

    ArithRef Sbfe(unsigned Start, unsigned Size) {
      if (IsConstant) {
        uint64_t SourceMask = Size == 64 ? ~0ULL : ((1ULL << Size) - 1);
        SourceMask <<= Start;

        int64_t NewConstant = (C & SourceMask) >> Start;
        NewConstant <<= 64 - Size;
        NewConstant >>= 64 - Size;

        return ArithRef(E, NewConstant);
      } else {
        return ArithRef(E, E->_Sbfe(OpSize::i64Bit, Size, Start, R));
      }
    }

    Ref BfiInto(Ref Bitfield, unsigned Start, unsigned Size) {
      if (IsConstant && (Size > 0 && Size < 64)) {
        uint64_t SourceMask = (1ULL << Size) - 1;
        uint64_t SourceMaskShifted = SourceMask << Start;

        if (C == 0) {
          return E->_And(OpSize::i64Bit, Bitfield, E->_InlineConstant(~SourceMaskShifted));
        } else if (C == SourceMask) {
          return E->_Or(OpSize::i64Bit, Bitfield, E->_InlineConstant(SourceMaskShifted));
        }
      }

      if (IsConstant) {
        return E->_Bfi(OpSize::i64Bit, Size, Start, Bitfield, E->Constant(C));
      } else {
        return E->_Bfi(OpSize::i64Bit, Size, Start, Bitfield, R);
      }
    }

    ArithRef MaskBit(OpSize Size) {
      if (IsConstant) {
        uint64_t ShiftMask = Size == OpSize::i64Bit ? 63 : 31;
        uint64_t Result = 1ull << (C & ShiftMask);
        if (ShiftMask == 31) {
          Result &= ((1ull << 32) - 1);
        }

        return ArithRef(E, Result);
      } else {
        return ArithRef(E, E->_Lshl(Size, E->Constant(1), R));
      }
    }

    Ref Ref() {
      return IsConstant ? E->Constant(C) : R;
    }

    bool IsDefinitelyZero() const {
      return IsConstant && C == 0;
    }
  };

  ArithRef ARef(Ref R) {
    uint64_t C;

    if (IsValueConstant(WrapNode(R), &C)) {
      return ARef(C);
    } else {
      return ArithRef(this, R);
    }
  }

  ArithRef ARef(uint64_t K) {
    return ArithRef(this, K);
  }

  ///< Segment telemetry tracking
  uint32_t SegmentsNeedReadCheck {~0U};
  void CheckLegacySegmentWrite(Ref NewNode, uint32_t SegmentReg);
  void CheckLegacySegmentRead(Ref NewNode, uint32_t SegmentReg);
};

constexpr inline void InstallToTable(auto& FinalTable, const auto& LocalTable) {
  for (const auto& Op : LocalTable) {
    auto OpNum = Op.Op;
    auto Dispatcher = Op.Ptr;
    for (uint8_t i = 0; i < Op.Count; ++i) {
      auto& TableOp = FinalTable[OpNum + i];
#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
      if (TableOp.OpcodeDispatcher.OpDispatch) {
        ERROR_AND_DIE_FMT("Duplicate Entry {}", TableOp.Name);
      }
#endif

      TableOp.OpcodeDispatcher.OpDispatch = Dispatcher;
    }
  }
}

} // namespace FEXCore::IR
