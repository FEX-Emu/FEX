/*
$info$
tags: backend|riscv64
$end_info$
*/

#pragma once

#include "Interface/Core/ArchHelpers/RISCVEmitter.h"
#include "Interface/Core/Dispatcher/Dispatcher.h"

#include <FEXCore/Core/CPUBackend.h>
#include <FEXCore/IR/IR.h>
#include <FEXCore/IR/IntrusiveIRList.h>

#define STATE x27

namespace FEXCore::Core {
  struct InternalThreadState;
}

namespace FEXCore::CPU {
class RISCVJITCore final : public CPUBackend, public RISCVEmitter  {
public:
  struct CodeBuffer {
    uint8_t *Ptr;
    size_t Size;
  };

  explicit RISCVJITCore(FEXCore::Context::Context *ctx,
                        FEXCore::Core::InternalThreadState *Thread,
                        bool CompileThread);
  ~RISCVJITCore() override;

  [[nodiscard]] std::string GetName() override { return "JIT"; }

  [[nodiscard]] void *CompileCode(uint64_t Entry,
                                  FEXCore::IR::IRListView const *IR,
                                  FEXCore::Core::DebugData *DebugData,
                                  FEXCore::IR::RegisterAllocationData *RAData) override;

  [[nodiscard]] void *MapRegion(void* HostPtr, uint64_t, uint64_t) override { return HostPtr; }

  [[nodiscard]] bool NeedsOpDispatch() override { return true; }

  void ClearCache() override;

  static constexpr size_t INITIAL_CODE_SIZE = 1024 * 1024 * 16;
  [[nodiscard]] CodeBuffer AllocateNewCodeBuffer(size_t Size);

  void CopyNecessaryDataForCompileThread(CPUBackend *Original) override;
  bool IsAddressInJITCode(uint64_t Address, bool IncludeDispatcher = true, bool IncludeCompileService = true) const override {
    return Dispatcher->IsAddressInJITCode(Address, IncludeDispatcher, IncludeCompileService);
  }

  static void InitializeSignalHandlers(FEXCore::Context::Context *CTX);

  [[nodiscard]] IR::PhysicalRegister GetPhys(IR::NodeID Node) const;
  [[nodiscard]] biscuit::GPR GetReg(IR::NodeID Node) const;
  [[nodiscard]] biscuit::Vec GetVReg(IR::NodeID Node) const;
  [[nodiscard]] uint32_t GetVIndex(IR::NodeID Node) const;

  [[nodiscard]] std::pair<biscuit::GPR, biscuit::GPR> GetSrcPair(IR::NodeID Node) const;

private:
  std::unique_ptr<FEXCore::CPU::Dispatcher> Dispatcher;
  FEXCore::Context::Context *CTX;
  FEXCore::Core::InternalThreadState *ThreadState;
  FEXCore::IR::IRListView const *IR;
  Label *PendingTargetLabel;

  uint64_t Entry;

  std::map<IR::NodeID, biscuit::Label> JumpTargets;

  constexpr static std::array<biscuit::SEW, 8> SEWBySize = {
    biscuit::SEW::E8,
    biscuit::SEW::E16,
    biscuit::SEW::E1024, // Invalid
    biscuit::SEW::E32,
    biscuit::SEW::E1024, // Invalid
    biscuit::SEW::E1024, // Invalid
    biscuit::SEW::E1024, // Invalid
    biscuit::SEW::E64,
  };

  /**
   * @name Register Allocation
   * @{ */
  constexpr static uint32_t RegisterClasses = 6;

  constexpr static uint64_t GPRBase = (0ULL << 32);
  constexpr static uint64_t FPRBase = (1ULL << 32);
  constexpr static uint64_t GPRPairBase = (2ULL << 32);

  /**  @} */

  IR::RegisterAllocationPass *RAPass;
  IR::RegisterAllocationData *RAData;

  // This is purely a debugging aid for developers to see if they are in JIT code space when inspecting raw memory
  void EmitDetectionString();

  void EmplaceNewCodeBuffer(CodeBuffer Buffer) {
    CurrentCodeBuffer = &CodeBuffers.emplace_back(Buffer);
  }

  void FreeCodeBuffer(CodeBuffer Buffer);

  [[nodiscard]] GPR GenerateMemOperand(uint8_t AccessSize,
                                              GPR Base,
                                              IR::OrderedNodeWrapper Offset,
                                              IR::MemOffsetType OffsetType,
                                              uint8_t OffsetScale);

  [[nodiscard]] bool IsInlineConstant(const IR::OrderedNodeWrapper& Node, uint64_t* Value = nullptr) const;
  [[nodiscard]] bool IsInlineEntrypointOffset(const IR::OrderedNodeWrapper& WNode, uint64_t* Value) const;

  // This is the initial code buffer that we will fall back to
  // In a program without signals and code clearing, we will typically
  // only have this code buffer
  CodeBuffer InitialCodeBuffer{};
  // This is the array of /additional/ code buffers that we may need to allocate
  // Allocation only occurs when we've hit signals and need to clear code cache
  // For code safety we can't delete code buffers until outside of all signals
  std::vector<CodeBuffer> CodeBuffers{};

  // This is the current code buffer that we are tracking
  CodeBuffer *CurrentCodeBuffer{};

  // We don't want to mvoe above 128MB atm because that means we will have to encode longer jumps
  static constexpr size_t MAX_CODE_SIZE = 1024 * 1024 * 128;
  static constexpr size_t MAX_DISPATCHER_CODE_SIZE = 4096 * 2;

  using OpHandler = void (RISCVJITCore::*)(IR::IROp_Header *IROp, IR::NodeID Node);
  std::array<OpHandler, IR::IROps::OP_LAST + 1> OpHandlers {};
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
#define DEF_OP(x) void Op_##x(IR::IROp_Header *IROp, IR::NodeID Node)

  ///< Unhandled handler
  DEF_OP(Unhandled);

  ///< No-op Handler
  DEF_OP(NoOp);

  ///< ALU Ops
  DEF_OP(TruncElementPair);
  DEF_OP(Constant);
  DEF_OP(EntrypointOffset);
  DEF_OP(InlineConstant);
  DEF_OP(InlineEntrypointOffset);
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
  DEF_OP(Andn);
  DEF_OP(Xor);
  DEF_OP(Lshl);
  DEF_OP(Lshr);
  DEF_OP(Ashr);
  DEF_OP(Rol);
  DEF_OP(Ror);
  DEF_OP(Extr);
  DEF_OP(PDep);
  DEF_OP(PExt);
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
  DEF_OP(AtomicFetchNeg);

  ///< Branch ops
  DEF_OP(GuestCallDirect);
  DEF_OP(GuestCallIndirect);
  DEF_OP(SignalReturn);
  DEF_OP(CallbackReturn);
  DEF_OP(ExitFunction);
  DEF_OP(Jump);
  DEF_OP(CondJump);
  DEF_OP(Syscall);
  DEF_OP(InlineSyscall);
  DEF_OP(Thunk);
  DEF_OP(ValidateCode);
  DEF_OP(RemoveCodeEntry);
  DEF_OP(CPUID);

  ///< Conversion ops
  DEF_OP(VInsGPR);
  DEF_OP(VCastFromGPR);
  DEF_OP(Float_FromGPR_S);
  DEF_OP(Float_FToF);
  DEF_OP(Vector_SToF);
  DEF_OP(Vector_FToZS);
  DEF_OP(Vector_FToS);
  DEF_OP(Vector_FToF);
  DEF_OP(Vector_FToI);

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
  DEF_OP(ParanoidLoadMemTSO);
  DEF_OP(ParanoidStoreMemTSO);
  DEF_OP(VLoadMemElement);
  DEF_OP(VStoreMemElement);
  DEF_OP(CacheLineClear);
  DEF_OP(CacheLineZero);

  ///< Misc ops
  DEF_OP(EndBlock);
  DEF_OP(Fence);
  DEF_OP(Break);
  DEF_OP(Phi);
  DEF_OP(PhiValue);
  DEF_OP(Print);
  DEF_OP(GetRoundingMode);
  DEF_OP(SetRoundingMode);
  DEF_OP(ProcessorID);
  DEF_OP(RDRAND);

  ///< Move ops
  DEF_OP(ExtractElementPair);
  DEF_OP(CreateElementPair);
  DEF_OP(Mov);

  ///< Vector ops
  DEF_OP(VectorZero);
  DEF_OP(VectorImm);
  DEF_OP(SplatVector2);
  DEF_OP(SplatVector4);
  DEF_OP(VMov);
  DEF_OP(VAnd);
  DEF_OP(VBic);
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
  DEF_OP(VUMinV);
  DEF_OP(VURAvg);
  DEF_OP(VAbs);
  DEF_OP(VPopcount);
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
  DEF_OP(VUnZip);
  DEF_OP(VUnZip2);
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
  DEF_OP(VDupElement);
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
  DEF_OP(VUMul);
  DEF_OP(VSMul);
  DEF_OP(VUMull);
  DEF_OP(VSMull);
  DEF_OP(VUMull2);
  DEF_OP(VSMull2);
  DEF_OP(VUABDL);
  DEF_OP(VTBL1);
  DEF_OP(VRev64);

  ///< Encryption ops
  DEF_OP(AESImc);
  DEF_OP(AESEnc);
  DEF_OP(AESEncLast);
  DEF_OP(AESDec);
  DEF_OP(AESDecLast);
  DEF_OP(AESKeyGenAssist);
  DEF_OP(CRC32);
#undef DEF_OP
};
}
