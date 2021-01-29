#pragma once

#include "Interface/Core/LookupCache.h"
#include "Interface/Core/BlockSamplingData.h"

#include "Interface/Core/JIT/x86_64/JIT.h"
#include "Common/MathUtils.h"

#include <xbyak/xbyak.h>
#include <xbyak/xbyak_util.h>

using namespace Xbyak;

#include <FEXCore/Core/CPUBackend.h>
#include <FEXCore/IR/IR.h>
#include <FEXCore/IR/IntrusiveIRList.h>
#include "Interface/IR/Passes/RegisterAllocationPass.h"

#include <tuple>

namespace FEXCore::CPU {
struct CodeBuffer {
  uint8_t *Ptr;
  size_t Size;
};

CodeBuffer AllocateNewCodeBuffer(size_t Size);
void FreeCodeBuffer(CodeBuffer Buffer);

}

namespace FEXCore::CPU {


// Temp registers
// rax, rcx, rdx, rsi, r8, r9,
// r10, r11
//
// Callee Saved
// rbx, rbp, r12, r13, r14, r15
//
// 1St Argument: rdi <ThreadState>
// XMM:
// All temp
#define STATE r14
#define TMP1 rax
#define TMP2 rcx
#define TMP3 rdx
#define TMP4 rdi
#define TMP5 rbx
using namespace Xbyak::util;
const std::array<Xbyak::Reg, 9> RA64 = { rsi, r8, r9, r10, r11, rbp, r12, r13, r15 };
const std::array<std::pair<Xbyak::Reg, Xbyak::Reg>, 4> RA64Pair = {{ {rsi, r8}, {r9, r10}, {r11, rbp}, {r12, r13} }};
const std::array<Xbyak::Reg, 11> RAXMM = { xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm8, xmm9, xmm10, xmm11};
const std::array<Xbyak::Xmm, 11> RAXMM_x = {  xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm8, xmm9, xmm10, xmm11};

class JITCore final : public CPUBackend, public Xbyak::CodeGenerator {
public:
  explicit JITCore(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread, CodeBuffer Buffer, bool CompileThread);
  ~JITCore() override;
  std::string GetName() override { return "JIT"; }
  void *CompileCode(FEXCore::IR::IRListView const *IR, FEXCore::Core::DebugData *DebugData, FEXCore::IR::RegisterAllocationData *RAData) override;

  void *MapRegion(void* HostPtr, uint64_t, uint64_t) override { return HostPtr; }

  bool NeedsOpDispatch() override { return true; }

  void ClearCache() override;

  static constexpr size_t INITIAL_CODE_SIZE = 1024 * 1024 * 16;
  static constexpr size_t MAX_CODE_SIZE = 1024 * 1024 * 256;

  bool HandleSIGILL(int Signal, void *info, void *ucontext);
  bool HandleSignalPause(int Signal, void *info, void *ucontext);
  bool HandleGuestSignal(int Signal, void *info, void *ucontext, GuestSigAction *GuestAction, stack_t *GuestStack);
  void CopyNecessaryDataForCompileThread(CPUBackend *Original) override;

private:
  Label* PendingTargetLabel{};
  FEXCore::Context::Context *CTX;
  FEXCore::Core::InternalThreadState *ThreadState;
  FEXCore::IR::IRListView const *IR;

  std::unordered_map<IR::OrderedNodeWrapper::NodeOffsetType, Label> JumpTargets;
  Xbyak::util::Cpu Features{};

  bool MemoryDebug = false;

  /**
   * @name Register Allocation
   * @{ */
  constexpr static uint32_t NumGPRs = RA64.size(); // 4 is the minimum required for GPR ops
  constexpr static uint32_t NumXMMs = RAXMM.size();
  constexpr static uint32_t NumGPRPairs = RA64Pair.size();
  constexpr static uint32_t RegisterCount = NumGPRs + NumXMMs + NumGPRPairs;
  constexpr static uint32_t RegisterClasses = 6;

  constexpr static uint64_t GPRBase = (0ULL << 32);
  constexpr static uint64_t XMMBase = (1ULL << 32);
  constexpr static uint64_t GPRPairBase = (2ULL << 32);

  /**  @} */

  constexpr static uint8_t RA_8 = 0;
  constexpr static uint8_t RA_16 = 1;
  constexpr static uint8_t RA_32 = 2;
  constexpr static uint8_t RA_64 = 3;
  constexpr static uint8_t RA_XMM = 4;

  IR::PhysicalRegister GetPhys(uint32_t Node);

  bool IsFPR(uint32_t Node);
  bool IsGPR(uint32_t Node);

  template<uint8_t RAType>
  Xbyak::Reg GetSrc(uint32_t Node);
  template<uint8_t RAType>
  std::pair<Xbyak::Reg, Xbyak::Reg> GetSrcPair(uint32_t Node);

  template<uint8_t RAType>
  Xbyak::Reg GetDst(uint32_t Node);

  Xbyak::Xmm GetSrc(uint32_t Node);
  Xbyak::Xmm GetDst(uint32_t Node);

  Xbyak::RegExp GenerateModRM(Xbyak::Reg Base, IR::OrderedNodeWrapper Offset, IR::MemOffsetType OffsetType, uint8_t OffsetScale);

  bool IsInlineConstant(const IR::OrderedNodeWrapper& Node, uint64_t* Value = nullptr);

  void CreateCustomDispatch(FEXCore::Core::InternalThreadState *Thread);
  IR::RegisterAllocationPass *RAPass;
  FEXCore::IR::RegisterAllocationData *RAData;

#ifdef BLOCKSTATS
  bool GetSamplingData {true};
#endif

  static constexpr size_t MAX_DISPATCHER_CODE_SIZE = 4096 * 1;

  void EmplaceNewCodeBuffer(CodeBuffer Buffer) {
    CurrentCodeBuffer = &CodeBuffers.emplace_back(Buffer);
  }

  static uint64_t ExitFunctionLink(JITCore* code, FEXCore::Core::InternalThreadState *Thread, uint64_t *record);

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

  uint64_t AbsoluteLoopTopAddress{};
  uint64_t ExitFunctionLinkerAddress{};
  uint64_t ThreadStopHandlerAddress{};
  uint64_t ThreadPauseHandlerAddress{};

  uint64_t PauseReturnInstruction{};

  uint32_t SignalHandlerRefCounter{};

  struct CompilerSharedData {
    uint64_t SignalHandlerReturnAddress{};

    uint32_t *SignalHandlerRefCounterPtr{};
  };

  CompilerSharedData ThreadSharedData;

  void StoreThreadState(int Signal, void *ucontext);
  void RestoreThreadState(void *ucontext);
  std::stack<uint64_t> SignalFrames;
  uint32_t SpillSlots{};
  using SetCC = void (JITCore::*)(const Operand& op);
  using CMovCC = void (JITCore::*)(const Reg& reg, const Operand& op);
  using JCC = void (JITCore::*)(const Label& label, LabelType type);

  std::tuple<SetCC, CMovCC, JCC> GetCC(IR::CondClassType cond);

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

  void PushRegs();
  void PopRegs();
#define DEF_OP(x) void Op_##x(FEXCore::IR::IROp_Header *IROp, uint32_t Node)

  ///< Unhandled handler
  DEF_OP(Unhandled);

  ///< No-op Handler
  DEF_OP(NoOp);

  ///< ALU Ops
  DEF_OP(TruncElementPair);
  DEF_OP(Constant);
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
  DEF_OP(F80Cmp);

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
  DEF_OP(LoadContextIndexed);
  DEF_OP(StoreContextIndexed);
  DEF_OP(SpillRegister);
  DEF_OP(FillRegister);
  DEF_OP(LoadFlag);
  DEF_OP(StoreFlag);
  DEF_OP(LoadMem);
  DEF_OP(StoreMem);
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
  DEF_OP(SplatVector);
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
