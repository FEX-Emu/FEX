#pragma once

#include "Interface/Core/Frontend.h"
#include "Interface/Context/Context.h"

#include <FEXCore/Config/Config.h>
#include <FEXCore/Core/Context.h>
#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/Debug/X86Tables.h>
#include <FEXCore/IR/IntrusiveIRList.h>
#include <FEXCore/IR/IR.h>
#include <FEXCore/IR/IREmitter.h>

#include <FEXCore/Utils/LogManager.h>

#include <cstdint>
#include <map>
#include <stddef.h>
#include <utility>
#include <vector>

namespace FEXCore::IR {
class Pass;
class PassManager;

class OpDispatchBuilder final : public IREmitter {
friend class FEXCore::IR::Pass;
friend class FEXCore::IR::PassManager;

enum class SelectionFlag {
  Nothing,  // must rely on x86 flags
  CMP,      // flags were set by a CMP between flagsOpDest/flagsOpDestSigned and flagsOpSrc/flagsOpSrcSigned with flagsOpSize size
  AND,      // flags were set by an AND/TEST, flagsOpDest contains the resulting value of flagsOpSize size
  FCMP,     // flags were set by a ucomis* / comis*
};

public:
  SelectionFlag flagsOp{};
  uint8_t flagsOpSize{};
  OrderedNode* flagsOpDest{};
  OrderedNode* flagsOpSrc{};
  OrderedNode* flagsOpDestSigned{};
  OrderedNode* flagsOpSrcSigned{};

  FEXCore::Context::Context *CTX{};
  bool ShouldDump {false};

  struct JumpTargetInfo {
    OrderedNode* BlockEntry;
    bool HaveEmitted;
  };

  std::map<uint64_t, JumpTargetInfo> JumpTargets;

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
        auto RelocatedNextRIP = _EntrypointOffset(NextRIP - Entry, GPRSize);
        _ExitFunction(RelocatedNextRIP);
      }
      else if (it != JumpTargets.end()) {
        _Jump(it->second.BlockEntry);
        return true;
      }
    }
    BlockSetRIP = false;

    return false;
  }

  OpDispatchBuilder(FEXCore::Context::Context *ctx);

  void ResetWorkingList();
  void ResetDecodeFailure() { DecodeFailure = false; }
  bool HadDecodeFailure() const { return DecodeFailure; }

  void BeginFunction(uint64_t RIP, std::vector<FEXCore::Frontend::Decoder::DecodedBlocks> const *Blocks);
  void Finalize();

  // Dispatch builder functions
#define OpcodeArgs [[maybe_unused]] FEXCore::X86Tables::DecodedOp Op
  void UnhandledOp(OpcodeArgs);
  template<uint32_t SrcIndex>
  void MOVGPROp(OpcodeArgs);
  void MOVVectorOp(OpcodeArgs);
  void ALUOp(OpcodeArgs);
  void INTOp(OpcodeArgs);
  void SyscallOp(OpcodeArgs);
  void ThunkOp(OpcodeArgs);
  void LEAOp(OpcodeArgs);
  void NOPOp(OpcodeArgs);
  void RETOp(OpcodeArgs);
  void IRETOp(OpcodeArgs);
  void SIGRETOp(OpcodeArgs);
  void CallbackReturnOp(OpcodeArgs);
  void SecondaryALUOp(OpcodeArgs);
  template<uint32_t SrcIndex>
  void ADCOp(OpcodeArgs);
  template<uint32_t SrcIndex>
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
  void XLATOp(OpcodeArgs);

  enum class Segment {
    FS,
    GS,
  };
  template<Segment Seg>
  void ReadSegmentReg(OpcodeArgs);
  template<Segment Seg>
  void WriteSegmentReg(OpcodeArgs);
  void EnterOp(OpcodeArgs);

  // SSE
  void MOVAPSOp(OpcodeArgs);
  void MOVUPSOp(OpcodeArgs);
  void MOVLHPSOp(OpcodeArgs);
  void MOVLPOp(OpcodeArgs);
  void MOVSHDUPOp(OpcodeArgs);
  void MOVSLDUPOp(OpcodeArgs);
  void MOVHPDOp(OpcodeArgs);
  void MOVSDOp(OpcodeArgs);
  template<FEXCore::IR::IROps IROp, size_t ElementSize>
  void VectorALUOp(OpcodeArgs);
  template<FEXCore::IR::IROps IROp, size_t ElementSize>
  void VectorScalarALUOp(OpcodeArgs);
  template<FEXCore::IR::IROps IROp, size_t ElementSize, bool Scalar>
  void VectorUnaryOp(OpcodeArgs);
  void MOVQOp(OpcodeArgs);
  template<size_t ElementSize>
  void PADDQOp(OpcodeArgs);
  template<size_t ElementSize>
  void PSUBQOp(OpcodeArgs);
  template<size_t ElementSize>
  void MOVMSKOp(OpcodeArgs);
  void MOVMSKOpOne(OpcodeArgs);
  template<size_t ElementSize>
  void PUNPCKLOp(OpcodeArgs);
  template<size_t ElementSize>
  void PUNPCKHOp(OpcodeArgs);
  void PSHUFBOp(OpcodeArgs);
  template<size_t ElementSize, bool HalfSize, bool Low>
  void PSHUFDOp(OpcodeArgs);
  void MOVDOp(OpcodeArgs);
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
  template<size_t ElementSize>
  void PAVGOp(OpcodeArgs);
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
  template<size_t SrcElementSize, bool Widen>
  void MMX_To_XMM_Vector_CVT_Int_To_Float(OpcodeArgs);
  template<size_t SrcElementSize, bool HostRoundingMode>
  void XMM_To_MMX_Vector_CVT_Float_To_Int(OpcodeArgs);
  void MASKMOVOp(OpcodeArgs);
  void MOVBetweenGPR_FPR(OpcodeArgs);
  void TZCNT(OpcodeArgs);
  void LZCNT(OpcodeArgs);
  void MOVSSOp(OpcodeArgs);
  template<size_t ElementSize, bool Scalar>
  void VFCMPOp(OpcodeArgs);
  template<size_t ElementSize>
  void SHUFOp(OpcodeArgs);
  void ANDNOp(OpcodeArgs);
  template<size_t ElementSize>
  void PINSROp(OpcodeArgs);
  void InsertPSOp(OpcodeArgs);
  template<size_t ElementSize>
  void PExtrOp(OpcodeArgs);

  template<size_t ElementSize>
  void PSIGN(OpcodeArgs);

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
  void PEXT(OpcodeArgs);
  void RORX(OpcodeArgs);

  // ADX Ops
  void ADXOp(OpcodeArgs);

  // X87 Ops
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

  void FXSaveOp(OpcodeArgs);
  void FXRStoreOp(OpcodeArgs);

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

  template<size_t ElementSize, bool Signed>
  void PADDSOp(OpcodeArgs);

  template<size_t ElementSize, bool Signed>
  void PSUBSOp(OpcodeArgs);

  template<size_t ElementSize>
  void ADDSUBPOp(OpcodeArgs);

  void PMADDWD(OpcodeArgs);
  void PMADDUBSW(OpcodeArgs);

  template<bool Signed>
  void PMULHW(OpcodeArgs);

  void PMULHRSW(OpcodeArgs);

  void MOVBEOp(OpcodeArgs);
  template<size_t ElementSize>
  void HADDP(OpcodeArgs);
  template<size_t ElementSize>
  void HSUBP(OpcodeArgs);

  template<size_t ElementSize>
  void PHADD(OpcodeArgs);
  template<size_t ElementSize>
  void PHSUB(OpcodeArgs);

  void PHADDS(OpcodeArgs);
  void PHSUBS(OpcodeArgs);

  template<uint8_t FenceType>
  void FenceOp(OpcodeArgs);

  void StoreFenceOrCLFlush(OpcodeArgs);

  void PSADBW(OpcodeArgs);

  void AESImcOp(OpcodeArgs);
  void AESEncOp(OpcodeArgs);
  void AESEncLastOp(OpcodeArgs);
  void AESDecOp(OpcodeArgs);
  void AESDecLastOp(OpcodeArgs);
  void AESKeyGenAssist(OpcodeArgs);

  template<size_t ElementSize, size_t DstElementSize, bool Signed>
  void ExtendVectorElements(OpcodeArgs);
  template<size_t ElementSize, bool Scalar>
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

  void UnimplementedOp(OpcodeArgs);

  void InvalidOp(OpcodeArgs);

#undef OpcodeArgs

  void SetPackedRFLAG(bool Lower8, OrderedNode *Src);
  OrderedNode *GetPackedRFLAG(bool Lower8);

  void SetMultiblock(bool _Multiblock) { Multiblock = _Multiblock; }

  bool HandledLock = false;
private:
  bool DecodeFailure{false};
  FEXCore::IR::IROp_IRHeader *Current_Header{};
  OrderedNode *Current_HeaderNode{};

  OrderedNode *AppendSegmentOffset(OrderedNode *Value, uint32_t Flags, uint32_t DefaultPrefix = 0, bool Override = false);

  OrderedNode *GetDynamicPC(FEXCore::X86Tables::DecodedOp const& Op, int64_t Offset = 0);
  OrderedNode *LoadSource(FEXCore::IR::RegisterClassType Class, FEXCore::X86Tables::DecodedOp const& Op, FEXCore::X86Tables::DecodedOperand const& Operand, uint32_t Flags, int8_t Align, bool LoadData = true, bool ForceLoad = false);
  OrderedNode *LoadSource_WithOpSize(FEXCore::IR::RegisterClassType Class, FEXCore::X86Tables::DecodedOp const& Op, FEXCore::X86Tables::DecodedOperand const& Operand, uint8_t OpSize, uint32_t Flags, int8_t Align, bool LoadData = true, bool ForceLoad = false);
  void StoreResult_WithOpSize(FEXCore::IR::RegisterClassType Class, FEXCore::X86Tables::DecodedOp Op, FEXCore::X86Tables::DecodedOperand const& Operand, OrderedNode *const Src, uint8_t OpSize, int8_t Align);
  void StoreResult(FEXCore::IR::RegisterClassType Class, FEXCore::X86Tables::DecodedOp Op, FEXCore::X86Tables::DecodedOperand const& Operand, OrderedNode *const Src, int8_t Align);
  void StoreResult(FEXCore::IR::RegisterClassType Class, FEXCore::X86Tables::DecodedOp Op, OrderedNode *const Src, int8_t Align);

  [[nodiscard]] static uint32_t GPROffset(X86State::X86Reg reg) {
    LOGMAN_THROW_A_FMT(reg <= X86State::X86Reg::REG_R15, "Invalid reg used");
    return static_cast<uint32_t>(offsetof(Core::CPUState, gregs[static_cast<size_t>(reg)]));
  }

  [[nodiscard]] static uint32_t MMBaseOffset() {
    return static_cast<uint32_t>(offsetof(Core::CPUState, mm[0][0]));
  }

  [[nodiscard]] uint8_t GetDstSize(X86Tables::DecodedOp Op) const;
  [[nodiscard]] uint8_t GetSrcSize(X86Tables::DecodedOp Op) const;
  [[nodiscard]] uint32_t GetDstBitSize(X86Tables::DecodedOp Op) const;
  [[nodiscard]] uint32_t GetSrcBitSize(X86Tables::DecodedOp Op) const;

  template<unsigned BitOffset>
  void SetRFLAG(OrderedNode *Value) {
    flagsOp = SelectionFlag::Nothing;
    _StoreFlag(_Bfe(1, 0, Value), BitOffset);
  }

  void SetRFLAG(OrderedNode *Value, unsigned BitOffset) {
    flagsOp = SelectionFlag::Nothing;
    _StoreFlag(_Bfe(1, 0, Value), BitOffset);
  }

  OrderedNode *GetRFLAG(unsigned BitOffset) {
    return _LoadFlag(BitOffset);
  }

  OrderedNode *SelectCC(uint8_t OP, OrderedNode *TrueValue, OrderedNode *FalseValue);

  void GenerateFlags_ADC(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2, OrderedNode *CF);
  void GenerateFlags_SBB(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2, OrderedNode *CF);
  void GenerateFlags_SUB(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2, bool UpdateCF = true);
  void GenerateFlags_ADD(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2, bool UpdateCF = true);
  void GenerateFlags_MUL(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Res, OrderedNode *High);
  void GenerateFlags_UMUL(FEXCore::X86Tables::DecodedOp Op, OrderedNode *High);
  void GenerateFlags_Logical(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2);
  void GenerateFlags_ShiftLeft(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2);
  void GenerateFlags_ShiftLeftImmediate(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Res, OrderedNode *Src1, uint64_t Shift);
  void GenerateFlags_ShiftRight(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2);
  void GenerateFlags_ShiftRightImmediate(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Res, OrderedNode *Src1, uint64_t Shift);
  void GenerateFlags_SignShiftRight(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2);
  void GenerateFlags_SignShiftRightImmediate(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Res, OrderedNode *Src1, uint64_t Shift);
  void GenerateFlags_RotateRight(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2);
  void GenerateFlags_RotateLeft(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2);
  void GenerateFlags_RotateRightImmediate(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Res, OrderedNode *Src1, uint64_t Shift);
  void GenerateFlags_RotateLeftImmediate(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Res, OrderedNode *Src1, uint64_t Shift);

  OrderedNode * GetX87Top();
  enum class X87Tag {
    Valid   = 0b00,
    Zero    = 0b01,
    Special = 0b10,
    Empty   = 0b11
  };
  void SetX87TopTag(OrderedNode *Value, X87Tag Tag);
  OrderedNode *GetX87FTW(OrderedNode *Value);
  void SetX87Top(OrderedNode *Value);

  bool DestIsLockedMem(FEXCore::X86Tables::DecodedOp Op) const {
    return DestIsMem(Op) && (Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_LOCK) != 0;
  }

  bool DestIsMem(FEXCore::X86Tables::DecodedOp Op) const {
    return !Op->Dest.IsGPR();
  }

  void CreateJumpBlocks(std::vector<FEXCore::Frontend::Decoder::DecodedBlocks> const *Blocks);
  bool BlockSetRIP {false};

  bool Multiblock{};
  uint64_t Entry;

  OrderedNode* _StoreMemAutoTSO(FEXCore::IR::RegisterClassType Class, uint8_t Size, OrderedNode *ssa0, OrderedNode *ssa1, uint8_t Align = 1) {
    if (CTX->Config.TSOEnabled)
    	return _StoreMemTSO(ssa0, ssa1, Invalid(), Align, Class, MEM_OFFSET_SXTX, 1, Size);
    else
      return _StoreMem(ssa0, ssa1, Invalid(), Align, Class, MEM_OFFSET_SXTX, 1, Size);
  }

  OrderedNode* _LoadMemAutoTSO(FEXCore::IR::RegisterClassType Class, uint8_t Size, OrderedNode *ssa0, uint8_t Align = 1) {
    if (CTX->Config.TSOEnabled)
      return _LoadMemTSO(ssa0, Invalid(), Align, Class, MEM_OFFSET_SXTX, 1, Size);
    else
      return _LoadMem(ssa0, Invalid(), Align, Class, MEM_OFFSET_SXTX, 1, Size);
  }


};

void InstallOpcodeHandlers(Context::OperatingMode Mode);

}

