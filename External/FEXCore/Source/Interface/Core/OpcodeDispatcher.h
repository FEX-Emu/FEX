#pragma once

#include "Interface/Core/Frontend.h"

#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Debug/X86Tables.h>
#include <FEXCore/IR/IntrusiveIRList.h>
#include <FEXCore/IR/IR.h>
#include <FEXCore/IR/IREmitter.h>

#include "LogManager.h"

#include <cstdint>
#include <functional>
#include <map>
#include <set>

namespace FEXCore::IR {
class Pass;
class PassManager;

class OpDispatchBuilder final : public IREmitter {
friend class FEXCore::IR::Pass;
friend class FEXCore::IR::PassManager;
public:
  FEXCore::Context::Context *CTX{};
  bool ShouldDump {false};

  struct JumpTargetInfo {
    OrderedNode* BlockEntry;
    bool HaveEmitted;
  };

  std::map<uint64_t, JumpTargetInfo> JumpTargets;

  OrderedNode* GetNewJumpBlock(uint64_t RIP) {
    auto it = JumpTargets.find(RIP);
    LogMan::Throw::A(it != JumpTargets.end(), "Couldn't find block generated for 0x%lx", RIP);
    return it->second.BlockEntry;
  }

  void SetNewBlockIfChanged(uint64_t RIP) {
    auto it = JumpTargets.find(RIP);
    if (it == JumpTargets.end()) return;

    it->second.HaveEmitted = true;

    if (CurrentCodeBlock->Wrapped(ListData.Begin()).ID() == it->second.BlockEntry->Wrapped(ListData.Begin()).ID()) return;

    // We have hit a RIP that is a jump target
    // Thus we need to end up in a new block
    SetCurrentCodeBlock(it->second.BlockEntry);
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
    if (!BlockSetRIP) {
      auto it = JumpTargets.find(NextRIP);
      if (it == JumpTargets.end() && LastOp) {
        // If we don't have a jump target to a new block then we have to leave
        // Set the RIP to the next instruction and leave
        _StoreContext(GPRClass, 8, offsetof(FEXCore::Core::CPUState, rip), _Constant(NextRIP));
        _ExitFunction();
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
  bool HadDecodeFailure() { return DecodeFailure; }

  void BeginFunction(uint64_t RIP, std::vector<FEXCore::Frontend::Decoder::DecodedBlocks> const *Blocks);
  void ExitFunction();
  void Finalize();

  // Dispatch builder functions
#define OpcodeArgs [[maybe_unused]] FEXCore::X86Tables::DecodedOp Op
  void UnhandledOp(OpcodeArgs);
  template<uint32_t SrcIndex>
  void MOVGPROp(OpcodeArgs);
  void MOVVectorOp(OpcodeArgs);
  template<uint32_t SrcIndex>
  void ALUOp(OpcodeArgs);
  void INTOp(OpcodeArgs);
  void SyscallOp(OpcodeArgs);
  void ThunkOp(OpcodeArgs);
  void LEAOp(OpcodeArgs);
  void NOPOp(OpcodeArgs);
  void RETOp(OpcodeArgs);
  template<uint32_t SrcIndex>
  void SecondaryALUOp(OpcodeArgs);
  template<uint32_t SrcIndex>
  void ADCOp(OpcodeArgs);
  template<uint32_t SrcIndex>
  void SBBOp(OpcodeArgs);
  void PUSHOp(OpcodeArgs);
  void PUSHREGOp(OpcodeArgs);
  void POPOp(OpcodeArgs);
  void LEAVEOp(OpcodeArgs);
  void CALLOp(OpcodeArgs);
  void CALLAbsoluteOp(OpcodeArgs);
  void CondJUMPOp(OpcodeArgs);
  void CondJUMPRCXOp(OpcodeArgs);
  template<bool CheckZF, bool ZFTrue>
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

  enum Segment {
    Segment_FS,
    Segment_GS,
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
  void PMINUOp(OpcodeArgs);
  template<size_t ElementSize>
  void PMAXUOp(OpcodeArgs);
  void PMINSWOp(OpcodeArgs);
  void PMAXSWOp(OpcodeArgs);
  template<size_t ElementSize>
  void MOVMSKOp(OpcodeArgs);
  template<size_t ElementSize>
  void PUNPCKLOp(OpcodeArgs);
  template<size_t ElementSize>
  void PUNPCKHOp(OpcodeArgs);
  template<size_t ElementSize, bool HalfSize, bool Low>
  void PSHUFDOp(OpcodeArgs);
  template<size_t ElementSize>
  void PCMPEQOp(OpcodeArgs);
  template<size_t ElementSize>
  void PCMPGTOp(OpcodeArgs);
  void MOVDOp(OpcodeArgs);
  template<size_t ElementSize, bool Scalar, uint32_t SrcIndex>
  void PSRLDOp(OpcodeArgs);
  template<size_t ElementSize>
  void PSRLI(OpcodeArgs);
  template<size_t ElementSize>
  void PSLLI(OpcodeArgs);
  template<size_t ElementSize, bool Scalar, uint32_t SrcIndex>
  void PSLL(OpcodeArgs);
  template<size_t ElementSize, bool Scalar, uint32_t SrcIndex>
  void PSRAOp(OpcodeArgs);
  void PSRLDQ(OpcodeArgs);
  void PSLLDQ(OpcodeArgs);
  template<size_t ElementSize>
  void PSRAIOp(OpcodeArgs);
  template<size_t ElementSize>
  void PAVGOp(OpcodeArgs);
  void MOVDDUPOp(OpcodeArgs);
  template<size_t DstElementSize, bool Signed>
  void CVTGPR_To_FPR(OpcodeArgs);
  template<size_t SrcElementSize, bool Signed>
  void CVTFPR_To_GPR(OpcodeArgs);
  template<size_t SrcElementSize, bool Signed, bool Widen>
  void Vector_CVT_Int_To_Float(OpcodeArgs);
  template<size_t DstElementSize, size_t SrcElementSize>
  void Scalar_CVT_Float_To_Float(OpcodeArgs);
  template<size_t DstElementSize, size_t SrcElementSize>
  void Vector_CVT_Float_To_Float(OpcodeArgs);
  template<size_t SrcElementSize, bool Signed, bool Narrow>
  void Vector_CVT_Float_To_Int(OpcodeArgs);
  template<size_t SrcElementSize, bool Signed, bool Widen>
  void MMX_To_XMM_Vector_CVT_Int_To_Float(OpcodeArgs);
  void MOVBetweenGPR_FPR(OpcodeArgs);
  void TZCNT(OpcodeArgs);
  void MOVSSOp(OpcodeArgs);
  template<size_t ElementSize, bool Scalar>
  void VFCMPOp(OpcodeArgs);
  template<size_t ElementSize>
  void SHUFOp(OpcodeArgs);
  void ANDNOp(OpcodeArgs);
  template<size_t ElementSize>
  void PINSROp(OpcodeArgs);
  template<size_t ElementSize>
  void PExtrOp(OpcodeArgs);
  template<size_t ElementSize, bool Signed>
  void PMULOp(OpcodeArgs);

  // X87 Ops
  template<size_t width>
  void FLD(OpcodeArgs);
  template<uint64_t Lower, uint32_t Upper>
  void FLD_Const(OpcodeArgs);

  template<size_t width>
  void FILD(OpcodeArgs);

  template<size_t width, bool pop>
  void FST(OpcodeArgs);

  template<bool pop>
  void FST(OpcodeArgs);

  template<size_t width, bool pop>
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
  template<bool Plus1>
  void X87FYL2X(OpcodeArgs);
  void X87TAN(OpcodeArgs);
  void X87ATAN(OpcodeArgs);
  void X87LDENV(OpcodeArgs);
  void X87FNSTENV(OpcodeArgs);
  void X87FSTCW(OpcodeArgs);
  void X87LDSW(OpcodeArgs);
  void X87FNSTSW(OpcodeArgs);
  void X87FNSAVE(OpcodeArgs);
  void X87FRSTOR(OpcodeArgs);
  void X87FXAM(OpcodeArgs);
  void X87FCMOV(OpcodeArgs);

  void FXCH(OpcodeArgs);

  enum class FCOMIFlags {
    FLAGS_X87,
    FLAGS_RFLAGS,
  };
  template<size_t width, bool Integer, FCOMIFlags whichflags, bool pop>
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

  void PMADDWD(OpcodeArgs);

  template<bool Signed>
  void PMULHW(OpcodeArgs);

  void MOVBEOp(OpcodeArgs);
  template<size_t ElementSize>
  void HADDP(OpcodeArgs);
  template<size_t ElementSize>
  void HSUBP(OpcodeArgs);

  template<uint8_t FenceType>
  void FenceOp(OpcodeArgs);

  void PSADBW(OpcodeArgs);

  void UnimplementedOp(OpcodeArgs);

#undef OpcodeArgs

  void SetPackedRFLAG(bool Lower8, OrderedNode *Src);
  OrderedNode *GetPackedRFLAG(bool Lower8);

  void SetMultiblock(bool _Multiblock) { Multiblock = _Multiblock; }
  bool GetMultiblock() { return Multiblock; }

private:
  bool DecodeFailure{false};
  FEXCore::IR::IROp_IRHeader *Current_Header{};

  OrderedNode *LoadSource(FEXCore::IR::RegisterClassType Class, FEXCore::X86Tables::DecodedOp const& Op, FEXCore::X86Tables::DecodedOperand const& Operand, uint32_t Flags, int8_t Align, bool LoadData = true, bool ForceLoad = false);
  OrderedNode *LoadSource_WithOpSize(FEXCore::IR::RegisterClassType Class, FEXCore::X86Tables::DecodedOp const& Op, FEXCore::X86Tables::DecodedOperand const& Operand, uint8_t OpSize, uint32_t Flags, int8_t Align, bool LoadData = true, bool ForceLoad = false);
  void StoreResult_WithOpSize(FEXCore::IR::RegisterClassType Class, FEXCore::X86Tables::DecodedOp Op, FEXCore::X86Tables::DecodedOperand const& Operand, OrderedNode *const Src, uint8_t OpSize, int8_t Align);
  void StoreResult(FEXCore::IR::RegisterClassType Class, FEXCore::X86Tables::DecodedOp Op, FEXCore::X86Tables::DecodedOperand const& Operand, OrderedNode *const Src, int8_t Align);
  void StoreResult(FEXCore::IR::RegisterClassType Class, FEXCore::X86Tables::DecodedOp Op, OrderedNode *const Src, int8_t Align);

  uint8_t GetDstSize(FEXCore::X86Tables::DecodedOp Op);
  uint8_t GetSrcSize(FEXCore::X86Tables::DecodedOp Op);

  template<unsigned BitOffset>
  void SetRFLAG(OrderedNode *Value);
  void SetRFLAG(OrderedNode *Value, unsigned BitOffset);
  OrderedNode *GetRFLAG(unsigned BitOffset);

  void GenerateFlags_ADC(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2, OrderedNode *CF);
  void GenerateFlags_SBB(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2, OrderedNode *CF);
  void GenerateFlags_SUB(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2);
  void GenerateFlags_ADD(FEXCore::X86Tables::DecodedOp Op, OrderedNode *Res, OrderedNode *Src1, OrderedNode *Src2);
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
  void SetX87Top(OrderedNode *Value);

  bool DestIsLockedMem(FEXCore::X86Tables::DecodedOp Op) {
    return Op->Dest.TypeNone.Type !=FEXCore::X86Tables::DecodedOperand::TYPE_GPR && (Op->Flags & FEXCore::X86Tables::DecodeFlags::FLAG_LOCK);
  }

  void CreateJumpBlocks(std::vector<FEXCore::Frontend::Decoder::DecodedBlocks> const *Blocks);
  bool BlockSetRIP {false};

  bool Multiblock{};
  uint64_t Entry;
};

void InstallOpcodeHandlers(Context::OperatingMode Mode);

}

