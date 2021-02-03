#pragma once

#include "Interface/Core/LookupCache.h"

#include "aarch64/assembler-aarch64.h"
#include "aarch64/cpu-aarch64.h"
#include "aarch64/disasm-aarch64.h"
#include "aarch64/assembler-aarch64.h"

#include <FEXCore/Core/CPUBackend.h>
#include <FEXCore/IR/IR.h>
#include <FEXCore/IR/IntrusiveIRList.h>

#define STATE x28
#define TMP1 x0
#define TMP2 x1
#define TMP3 x2
#define TMP4 x3

#define VTMP1 v1
#define VTMP2 v2
#define VTMP3 v3

namespace FEXCore::Core {
  struct InternalThreadState;
}

namespace FEXCore::CPU {
using namespace vixl;
using namespace vixl::aarch64;

// All but x29 are caller saved
const std::array<aarch64::Register, 16> SRA64 = {
  x4, x5, x6, x7, x8, x9, x10, x11,
  x12, x18, x17, x16, x15, x14, x13, x29
};

// All are callee saved
const std::array<aarch64::Register, 9> RA64 = {
  x20, x21, x22, x23, x24, x25, x26, x27,
  x19
};

const std::array<std::pair<aarch64::Register, aarch64::Register>, 4>  RA64Pair = {{
  {x20, x21},
  {x22, x23},
  {x24, x25},
  {x26, x27},
}};

const std::array<std::pair<aarch64::Register, aarch64::Register>, 4> RA32Pair = {{
  {w20, w21},
  {w22, w23},
  {w24, w25},
  {w26, w27},
}};

// All are caller saved
const std::array<aarch64::VRegister, 16> SRAFPR = {
  v16, v17, v18, v19, v20, v21, v22, v23,
  v24, v25, v26, v27, v28, v29, v30, v31
};

//  v8..v15 = (lower 64bits) Callee saved
const std::array<aarch64::VRegister, 12> RAFPR = {
/*v0,  v1,  v2,  v3,*/v4,  v5,  v6,  v7,  // v0 ~ v3 are used as temps
  v8,  v9,  v10, v11, v12, v13, v14, v15
};

class JITCore final : public CPUBackend, public vixl::aarch64::Assembler  {
public:
  struct CodeBuffer {
    uint8_t *Ptr;
    size_t Size;
  };

  explicit JITCore(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread, CodeBuffer Buffer, bool CompileThread);

  ~JITCore() override;
  std::string GetName() override { return "JIT"; }
  void *CompileCode(FEXCore::IR::IRListView const *IR, FEXCore::Core::DebugData *DebugData, FEXCore::IR::RegisterAllocationData *RAData) override;

  void *MapRegion(void* HostPtr, uint64_t, uint64_t) override { return HostPtr; }

  bool NeedsOpDispatch() override { return true; }

  void ClearCache() override;

  bool HandleSIGILL(int Signal, void *info, void *ucontext);
  bool HandleSIGBUS(int Signal, void *info, void *ucontext);
  bool HandleSignalPause(int Signal, void *info, void *ucontext);
  bool HandleGuestSignal(int Signal, void *info, void *ucontext, GuestSigAction *GuestAction, stack_t *GuestStack);

  static constexpr size_t INITIAL_CODE_SIZE = 1024 * 1024 * 16;
  static CodeBuffer AllocateNewCodeBuffer(size_t Size);

  void CopyNecessaryDataForCompileThread(CPUBackend *Original) override;

private:
  Label *PendingTargetLabel;
  FEXCore::Context::Context *CTX;
  FEXCore::Core::InternalThreadState *State;
  FEXCore::IR::IRListView const *IR;

  std::map<IR::OrderedNodeWrapper::NodeOffsetType, aarch64::Label> JumpTargets;

  /**
   * @name Register Allocation
   * @{ */
  constexpr static uint32_t NumGPRs = RA64.size();
  constexpr static uint32_t NumFPRs = RAFPR.size();
  constexpr static uint32_t NumGPRPairs = RA64Pair.size();
  constexpr static uint32_t NumCalleeGPRs = 10;
  constexpr static uint32_t NumCalleeGPRPairs = 5;
  constexpr static uint32_t RegisterCount = NumGPRs + NumFPRs + NumGPRPairs;
  constexpr static uint32_t RegisterClasses = 6;

  constexpr static uint64_t GPRBase = (0ULL << 32);
  constexpr static uint64_t FPRBase = (1ULL << 32);
  constexpr static uint64_t GPRPairBase = (2ULL << 32);

  /**  @} */

  constexpr static uint8_t RA_32 = 0;
  constexpr static uint8_t RA_64 = 1;
  constexpr static uint8_t RA_FPR = 2;

  template<uint8_t RAType>
  aarch64::Register GetReg(uint32_t Node);

  template<>
  aarch64::Register GetReg<RA_32>(uint32_t Node);
  template<>
  aarch64::Register GetReg<RA_64>(uint32_t Node);

  template<uint8_t RAType>
  std::pair<aarch64::Register, aarch64::Register> GetSrcPair(uint32_t Node);

  template<>
  std::pair<aarch64::Register, aarch64::Register> GetSrcPair<RA_32>(uint32_t Node);
  template<>
  std::pair<aarch64::Register, aarch64::Register> GetSrcPair<RA_64>(uint32_t Node);

  aarch64::VRegister GetSrc(uint32_t Node);
  aarch64::VRegister GetDst(uint32_t Node);

  FEXCore::IR::RegisterClassType GetRegClass(uint32_t Node);

  bool IsFPR(uint32_t Node);
  bool IsGPR(uint32_t Node);

  MemOperand GenerateMemOperand(uint8_t AccessSize, aarch64::Register Base, IR::OrderedNodeWrapper Offset, IR::MemOffsetType OffsetType, uint8_t OffsetScale);

  bool IsInlineConstant(const IR::OrderedNodeWrapper& Node, uint64_t* Value = nullptr);

  struct LiveRange {
    uint32_t Begin;
    uint32_t End;
  };

#if DEBUG
  vixl::aarch64::Decoder Decoder;
#endif
  vixl::aarch64::CPU CPU;
  bool SupportsAtomics{};
  bool SupportsRCPC{};

  void EmplaceNewCodeBuffer(CodeBuffer Buffer) {
    CurrentCodeBuffer = &CodeBuffers.emplace_back(Buffer);
  }

  void FreeCodeBuffer(CodeBuffer Buffer);

  // This is the initial code buffer that we will fall back to
  // In a program without signals and code clearing, we will typically
  // only have this code buffer
  CodeBuffer InitialCodeBuffer{};
  // This is the array of /additional/ code buffers that we may need to allocate
  // Allocation only occurs when we've hit signals and need to clear code cache
  // For code safety we can't delete code buffers until outside of all signals
  std::vector<CodeBuffer> CodeBuffers{};

  // This is the codebuffer that our dispatcher lives in
  CodeBuffer DispatcherCodeBuffer{};
  // This is the current code buffer that we are tracking
  CodeBuffer *CurrentCodeBuffer{};

  // We don't want to mvoe above 128MB atm because that means we will have to encode longer jumps
  static constexpr size_t MAX_CODE_SIZE = 1024 * 1024 * 128;
  static constexpr size_t MAX_DISPATCHER_CODE_SIZE = 4096 * 2;

  bool IsAddressInJITCode(uint64_t Address, bool IncludeDispatcher = true);

#if DEBUG
  vixl::aarch64::Disassembler Disasm;
#endif

  void LoadConstant(vixl::aarch64::Register Reg, uint64_t Constant);

  void CreateCustomDispatch(FEXCore::Core::InternalThreadState *Thread);
  void PushCalleeSavedRegisters();
  void PopCalleeSavedRegisters();

  static uint64_t ExitFunctionLink(JITCore *core, FEXCore::Core::InternalThreadState *Thread, uint64_t *record);

  /**
   * @name Dispatch Helper functions
   * @{ */
  uint64_t AbsoluteLoopTopAddressFillSRA{};
  uint64_t AbsoluteLoopTopAddress{};
  uint64_t ThreadPauseHandlerAddressSpillSRA{};
  uint64_t ExitFunctionLinkerAddress{};
  uint64_t ThreadPauseHandlerAddress{};

  uint64_t ThreadStopHandlerAddressSpillSRA{};
  uint64_t ThreadStopHandlerAddress{};
  uint64_t PauseReturnInstruction{};

  uint32_t SignalHandlerRefCounter{};

  void StoreThreadState(int Signal, void *ucontext);
  void RestoreThreadState(void *ucontext);
  /**  @} */

  struct CompilerSharedData {
    uint64_t SignalReturnInstruction{};

    uint32_t *SignalHandlerRefCounterPtr{};
  };

  CompilerSharedData ThreadSharedData;
  IR::RegisterAllocationPass *RAPass;
  IR::RegisterAllocationData *RAData;

  uint32_t SpillSlots{};

  void SpillStaticRegs();
  void FillStaticRegs();

  void PushDynamicRegsAndLR();
  void PopDynamicRegsAndLR();

  void ResetStack();

  using OpHandler = void (JITCore::*)(FEXCore::IR::IROp_Header *IROp, uint32_t Node);
  std::array<OpHandler, FEXCore::IR::IROps::OP_LAST + 1> OpHandlers {};
  void RegisterALUHandlers();
  void RegisterAtomicHandlers();
  void RegisterBranchHandlers();
  void RegisterConversionHandlers();
  void RegisterFlagHandlers();
  void RegisterMemoryHandlers();
  void RegisterMiscHandlers();
  void RegisterMoveHandlers();
  void RegisterVectorHandlers();
  void RegisterEncryptionHandlers();
#define DEF_OP(x) void Op_##x(FEXCore::IR::IROp_Header *IROp, uint32_t Node)

  ///< Unhandled handler
  DEF_OP(Unhandled);

  ///< No-op Handler
  DEF_OP(NoOp);

  ///< ALU Ops
  DEF_OP(TruncElementPair);
  DEF_OP(Constant);
  DEF_OP(EntrypointOffset);
  DEF_OP(InlineConstant);
  DEF_OP(CycleCounter);
  DEF_OP(Add);
  DEF_OP(Sub);
  DEF_OP(Neg);
  DEF_OP(Mul);
  DEF_OP(UMul);
  DEF_OP(Div);
  DEF_OP(UDiv);
  DEF_OP(Rem);
  DEF_OP(URem);
  DEF_OP(MulH);
  DEF_OP(UMulH);
  DEF_OP(Or);
  DEF_OP(And);
  DEF_OP(Xor);
  DEF_OP(Lshl);
  DEF_OP(Lshr);
  DEF_OP(Ashr);
  DEF_OP(Rol);
  DEF_OP(Ror);
  DEF_OP(Extr);
  DEF_OP(LDiv);
  DEF_OP(LUDiv);
  DEF_OP(LRem);
  DEF_OP(LURem);
  DEF_OP(Zext);
  DEF_OP(Not);
  DEF_OP(Popcount);
  DEF_OP(FindLSB);
  DEF_OP(FindMSB);
  DEF_OP(FindTrailingZeros);
  DEF_OP(CountLeadingZeroes);
  DEF_OP(Rev);
  DEF_OP(Bfi);
  DEF_OP(Bfe);
  DEF_OP(Sbfe);
  DEF_OP(Select);
  DEF_OP(VExtractToGPR);
  DEF_OP(Float_ToGPR_ZU);
  DEF_OP(Float_ToGPR_ZS);
  DEF_OP(Float_ToGPR_U);
  DEF_OP(Float_ToGPR_S);
  DEF_OP(FCmp);

  ///< Atomic ops
  DEF_OP(CASPair);
  DEF_OP(CAS);
  DEF_OP(AtomicAdd);
  DEF_OP(AtomicSub);
  DEF_OP(AtomicAnd);
  DEF_OP(AtomicOr);
  DEF_OP(AtomicXor);
  DEF_OP(AtomicSwap);
  DEF_OP(AtomicFetchAdd);
  DEF_OP(AtomicFetchSub);
  DEF_OP(AtomicFetchAnd);
  DEF_OP(AtomicFetchOr);
  DEF_OP(AtomicFetchXor);

  ///< Branch ops
  DEF_OP(GuestCallDirect);
  DEF_OP(GuestCallIndirect);
  DEF_OP(GuestReturn);
  DEF_OP(SignalReturn);
  DEF_OP(CallbackReturn);
  DEF_OP(ExitFunction);
  DEF_OP(Jump);
  DEF_OP(CondJump);
  DEF_OP(Syscall);
  DEF_OP(Thunk);
  DEF_OP(ValidateCode);
  DEF_OP(RemoveCodeEntry);
  DEF_OP(CPUID);

  ///< Conversion ops
  DEF_OP(VInsGPR);
  DEF_OP(VCastFromGPR);
  DEF_OP(Float_FromGPR_U);
  DEF_OP(Float_FromGPR_S);
  DEF_OP(Float_FToF);
  DEF_OP(Vector_UToF);
  DEF_OP(Vector_SToF);
  DEF_OP(Vector_FToZU);
  DEF_OP(Vector_FToZS);
  DEF_OP(Vector_FToU);
  DEF_OP(Vector_FToS);
  DEF_OP(Vector_FToF);

  ///< Flag ops
  DEF_OP(GetHostFlag);

  ///< Memory ops
    DEF_OP(LoadContext);
  DEF_OP(StoreContext);
  DEF_OP(LoadRegister);
  DEF_OP(StoreRegister);
  DEF_OP(LoadContextIndexed);
  DEF_OP(StoreContextIndexed);
  DEF_OP(SpillRegister);
  DEF_OP(FillRegister);
  DEF_OP(LoadFlag);
  DEF_OP(StoreFlag);
  DEF_OP(LoadMem);
  DEF_OP(StoreMem);
  DEF_OP(LoadMemTSO);
  DEF_OP(StoreMemTSO);
  DEF_OP(VLoadMemElement);
  DEF_OP(VStoreMemElement);

  ///< Misc ops
  DEF_OP(EndBlock);
  DEF_OP(Fence);
  DEF_OP(Break);
  DEF_OP(Phi);
  DEF_OP(PhiValue);
  DEF_OP(Print);
  DEF_OP(GetRoundingMode);
  DEF_OP(SetRoundingMode);

  ///< Move ops
  DEF_OP(ExtractElementPair);
  DEF_OP(CreateElementPair);
  DEF_OP(Mov);

  ///< Vector ops
  DEF_OP(VectorZero);
  DEF_OP(VectorImm);
  DEF_OP(CreateVector2);
  DEF_OP(CreateVector4);
  DEF_OP(SplatVector2);
  DEF_OP(SplatVector4);
  DEF_OP(VMov);
  DEF_OP(VAnd);
  DEF_OP(VOr);
  DEF_OP(VXor);
  DEF_OP(VAdd);
  DEF_OP(VSub);
  DEF_OP(VUQAdd);
  DEF_OP(VUQSub);
  DEF_OP(VSQAdd);
  DEF_OP(VSQSub);
  DEF_OP(VAddP);
  DEF_OP(VAddV);
  DEF_OP(VURAvg);
  DEF_OP(VAbs);
  DEF_OP(VFAdd);
  DEF_OP(VFAddP);
  DEF_OP(VFSub);
  DEF_OP(VFMul);
  DEF_OP(VFDiv);
  DEF_OP(VFMin);
  DEF_OP(VFMax);
  DEF_OP(VFRecp);
  DEF_OP(VFSqrt);
  DEF_OP(VFRSqrt);
  DEF_OP(VNeg);
  DEF_OP(VFNeg);
  DEF_OP(VNot);
  DEF_OP(VUMin);
  DEF_OP(VSMin);
  DEF_OP(VUMax);
  DEF_OP(VSMax);
  DEF_OP(VZip);
  DEF_OP(VZip2);
  DEF_OP(VBSL);
  DEF_OP(VCMPEQ);
  DEF_OP(VCMPEQZ);
  DEF_OP(VCMPGT);
  DEF_OP(VCMPGTZ);
  DEF_OP(VCMPLTZ);
  DEF_OP(VFCMPEQ);
  DEF_OP(VFCMPNEQ);
  DEF_OP(VFCMPLT);
  DEF_OP(VFCMPGT);
  DEF_OP(VFCMPLE);
  DEF_OP(VFCMPORD);
  DEF_OP(VFCMPUNO);
  DEF_OP(VUShl);
  DEF_OP(VUShr);
  DEF_OP(VSShr);
  DEF_OP(VUShlS);
  DEF_OP(VUShrS);
  DEF_OP(VSShrS);
  DEF_OP(VInsElement);
  DEF_OP(VInsScalarElement);
  DEF_OP(VExtractElement);
  DEF_OP(VExtr);
  DEF_OP(VSLI);
  DEF_OP(VSRI);
  DEF_OP(VUShrI);
  DEF_OP(VSShrI);
  DEF_OP(VShlI);
  DEF_OP(VUShrNI);
  DEF_OP(VUShrNI2);
  DEF_OP(VBitcast);
  DEF_OP(VSXTL);
  DEF_OP(VSXTL2);
  DEF_OP(VUXTL);
  DEF_OP(VUXTL2);
  DEF_OP(VSQXTN);
  DEF_OP(VSQXTN2);
  DEF_OP(VSQXTUN);
  DEF_OP(VSQXTUN2);
  DEF_OP(VMul);
  DEF_OP(VUMull);
  DEF_OP(VSMull);
  DEF_OP(VUMull2);
  DEF_OP(VSMull2);
  DEF_OP(VTBL1);

  ///< Encryption ops
  DEF_OP(AESImc);
  DEF_OP(AESEnc);
  DEF_OP(AESEncLast);
  DEF_OP(AESDec);
  DEF_OP(AESDecLast);
  DEF_OP(AESKeyGenAssist);
#undef DEF_OP
};


}

