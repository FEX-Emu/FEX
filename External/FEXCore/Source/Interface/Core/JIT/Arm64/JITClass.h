#pragma once

#include "Interface/Core/BlockCache.h"
#include "Interface/Core/InternalThreadState.h"

#if _M_X86_64
#define VIXL_INCLUDE_SIMULATOR_AARCH64
#include "aarch64/simulator-aarch64.h"
#endif
#include "aarch64/assembler-aarch64.h"
#include "aarch64/cpu-aarch64.h"
#include "aarch64/disasm-aarch64.h"
#include "aarch64/macro-assembler-aarch64.h"

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

namespace FEXCore::CPU {
using namespace vixl;
using namespace vixl::aarch64;

const std::array<aarch64::Register, 22> RA64 = {
  x4, x5, x6, x7, x8, x9,
  x10, x11, x12, x13, x14, x15,
  /*x16, x17,*/ // We can't use these until we move away from the MacroAssembler
  x18, x19, x20, x21, x22, x23,
  x24, x25, x26, x27};

const std::array<std::pair<aarch64::Register, aarch64::Register>, 11> RA64Pair = {{
  {x4, x5},
  {x6, x7},
  {x8, x9},
  {x10, x11},
  {x12, x13},
  {x14, x15},
  /* {x16, x17}, */
  {x18, x19},
  {x20, x21},
  {x22, x23},
  {x24, x25},
  {x26, x27},
}};

const std::array<std::pair<aarch64::Register, aarch64::Register>, 11> RA32Pair = {{
  {w4, w5},
  {w6, w7},
  {w8, w9},
  {w10, w11},
  {w12, w13},
  {w14, w15},
  /* {w16, w17}, */
  {w18, w19},
  {w20, w21},
  {w22, w23},
  {w24, w25},
  {w26, w27},
}};

//  v8..v15 = (lower 64bits) Callee saved
const std::array<aarch64::VRegister, 22> RAFPR = {
  v3, v4, v5, v6, v7, v8, v16,
  v17, v18, v19, v20, v21, v22,
  v23, v24, v25, v26, v27, v28,
  v29, v30, v31};

// XXX: Switch from MacroAssembler to Assembler once we drop the simulator
class JITCore final : public CPUBackend, public vixl::aarch64::MacroAssembler  {
public:
  explicit JITCore(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread);
  ~JITCore() override;
  std::string GetName() override { return "JIT"; }
  void *CompileCode(FEXCore::IR::IRListView<true> const *IR, FEXCore::Core::DebugData *DebugData) override;

  void *MapRegion(void* HostPtr, uint64_t, uint64_t) override { return HostPtr; }

  bool NeedsOpDispatch() override { return true; }

#if _M_X86_64
  void SimulationExecution(FEXCore::Core::InternalThreadState *Thread);
#endif

  bool HasCustomDispatch() const override { return CustomDispatchGenerated; }

#if _M_X86_64
  void ExecuteCustomDispatch(FEXCore::Core::ThreadState *Thread) override;
#else
  void ExecuteCustomDispatch(FEXCore::Core::ThreadState *Thread) override {
    DispatchPtr(reinterpret_cast<FEXCore::Core::InternalThreadState*>(Thread));
  }
#endif

  void ClearCache() override;

  bool HandleSIGBUS(int Signal, void *info, void *ucontext);
  bool HandleSIGSEGV(int Signal, void *info, void *ucontext);

private:
  FEXCore::Context::Context *CTX;
  FEXCore::Core::InternalThreadState *State;
  FEXCore::IR::IRListView<true> const *CurrentIR;

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
  constexpr static uint32_t RegisterClasses = 3;

  constexpr static uint64_t GPRBase = (0ULL << 32);
  constexpr static uint64_t FPRBase = (1ULL << 32);
  constexpr static uint64_t GPRPairBase = (2ULL << 32);

  /**  @} */

  constexpr static uint8_t RA_32 = 0;
  constexpr static uint8_t RA_64 = 1;
  constexpr static uint8_t RA_FPR = 2;

  static constexpr uint32_t MAX_CODE_SIZE = 1024 * 1024 * 128;

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

  struct LiveRange {
    uint32_t Begin;
    uint32_t End;
  };

#if DEBUG || _M_X86_64
  vixl::aarch64::Decoder Decoder;
#endif
  vixl::aarch64::CPU CPU;
  bool SupportsAtomics{};

#if DEBUG
  vixl::aarch64::Disassembler Disasm;
#endif

#if _M_X86_64
  vixl::aarch64::Simulator Sim;
  std::unordered_map<uint64_t, std::pair<uint64_t, uint64_t>> HostToGuest;
#endif
  void LoadConstant(vixl::aarch64::Register Reg, uint64_t Constant);

  void CreateCustomDispatch(FEXCore::Core::InternalThreadState *Thread);
  void GenerateDispatchHelpers();
  ptrdiff_t ConstantCodeCacheOffset{};
  /**
   * @name Dispatch Helper functions
   * @{ */
  /**  @} */

  bool CustomDispatchGenerated {false};
  using CustomDispatch = void(*)(FEXCore::Core::InternalThreadState *Thread);
  CustomDispatch DispatchPtr{};
  IR::RegisterAllocationPass *RAPass;

#if _M_X86_64
  uint64_t CustomDispatchEnd;
#endif
  uint32_t SpillSlots{};

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
#define DEF_OP(x) void Op_##x(FEXCore::IR::IROp_Header *IROp, uint32_t Node)

  ///< Unhandled handler
  DEF_OP(Unhandled);

  ///< No-op Handler
  DEF_OP(NoOp);

  ///< ALU Ops
  DEF_OP(TruncElementPair);
  DEF_OP(Constant);
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
  DEF_OP(LDiv);
  DEF_OP(LUDiv);
  DEF_OP(LRem);
  DEF_OP(LURem);
  DEF_OP(Zext);
  DEF_OP(Sext);
  DEF_OP(Not);
  DEF_OP(Popcount);
  DEF_OP(FindLSB);
  DEF_OP(FindMSB);
  DEF_OP(FindTrailingZeros);
  DEF_OP(Rev);
  DEF_OP(Bfi);
  DEF_OP(Bfe);
  DEF_OP(Sbfe);
  DEF_OP(Select);
  DEF_OP(VExtractToGPR);
  DEF_OP(Float_ToGPR_ZU);
  DEF_OP(Float_ToGPR_ZS);
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
  DEF_OP(ExitFunction);
  DEF_OP(Jump);
  DEF_OP(CondJump);
  DEF_OP(Syscall);
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
  DEF_OP(Vector_FToF);

  ///< Flag ops
  DEF_OP(GetHostFlag);

  ///< Memory ops
  DEF_OP(LoadContextPair);
  DEF_OP(StoreContextPair);
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

  ///< Move ops
  DEF_OP(ExtractElementPair);
  DEF_OP(CreateElementPair);
  DEF_OP(Mov);

  ///< Vector ops
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
  DEF_OP(VCMPGT);
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
#undef DEF_OP
};


}

