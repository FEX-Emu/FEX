// SPDX-License-Identifier: MIT
/*
$info$
tags: backend|arm64
$end_info$
*/

#pragma once

#include "Interface/Core/ArchHelpers/Arm64Emitter.h"
#include "Interface/Core/ArchHelpers/CodeEmitter/Emitter.h"
#include "Interface/Core/Dispatcher/Dispatcher.h"

#include <aarch64/assembler-aarch64.h>
#include <aarch64/disasm-aarch64.h>

#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Core/CPUBackend.h>
#include <FEXCore/IR/IR.h>
#include <FEXCore/IR/IntrusiveIRList.h>
#include <FEXCore/IR/RegisterAllocationData.h>
#include <FEXCore/fextl/map.h>
#include <FEXCore/fextl/string.h>
#include <FEXCore/fextl/vector.h>

#include <array>
#include <cstdint>
#include <utility>

namespace FEXCore::Core {
  struct InternalThreadState;
}

namespace FEXCore::CPU {
class Arm64JITCore final : public CPUBackend, public Arm64Emitter  {
public:
  explicit Arm64JITCore(FEXCore::Context::ContextImpl *ctx,
                        FEXCore::Core::InternalThreadState *Thread);
  ~Arm64JITCore() override;

  [[nodiscard]] fextl::string GetName() override { return "JIT"; }

  [[nodiscard]] CPUBackend::CompiledCode CompileCode(uint64_t Entry,
                                  FEXCore::IR::IRListView const *IR,
                                  FEXCore::Core::DebugData *DebugData,
                                  FEXCore::IR::RegisterAllocationData *RAData, bool GDBEnabled) override;

  [[nodiscard]] void *MapRegion(void* HostPtr, uint64_t, uint64_t) override { return HostPtr; }

  [[nodiscard]] bool NeedsOpDispatch() override { return true; }

  void ClearCache() override;

  void ClearRelocations() override { Relocations.clear(); }

private:
  FEX_CONFIG_OPT(ParanoidTSO, PARANOIDTSO);

  const bool HostSupportsSVE128{};
  const bool HostSupportsSVE256{};

  ARMEmitter::BiDirectionalLabel *PendingTargetLabel;
  FEXCore::Context::ContextImpl *CTX;
  FEXCore::IR::IRListView const *IR;
  uint64_t Entry;
  CPUBackend::CompiledCode CodeData{};

  fextl::map<IR::NodeID, ARMEmitter::BiDirectionalLabel> JumpTargets;

  [[nodiscard]] FEXCore::ARMEmitter::Register GetReg(IR::NodeID Node) const {
    const auto Reg = GetPhys(Node);

    LOGMAN_THROW_AA_FMT(Reg.Class == IR::GPRFixedClass.Val || Reg.Class == IR::GPRClass.Val, "Unexpected Class: {}", Reg.Class);

    if (Reg.Class == IR::GPRFixedClass.Val) {
      return StaticRegisters[Reg.Reg];
    } else if (Reg.Class == IR::GPRClass.Val) {
      return GeneralRegisters[Reg.Reg];
    }

    FEX_UNREACHABLE;
  }

  [[nodiscard]] FEXCore::ARMEmitter::VRegister GetVReg(IR::NodeID Node) const {
    const auto Reg = GetPhys(Node);

    LOGMAN_THROW_AA_FMT(Reg.Class == IR::FPRFixedClass.Val || Reg.Class == IR::FPRClass.Val, "Unexpected Class: {}", Reg.Class);

    if (Reg.Class == IR::FPRFixedClass.Val) {
      return StaticFPRegisters[Reg.Reg];
    } else if (Reg.Class == IR::FPRClass.Val) {
      return GeneralFPRegisters[Reg.Reg];
    }

    FEX_UNREACHABLE;
  }

  [[nodiscard]] std::pair<FEXCore::ARMEmitter::Register, FEXCore::ARMEmitter::Register> GetRegPair(IR::NodeID Node) const {
    const auto Reg = GetPhys(Node);

    LOGMAN_THROW_AA_FMT(Reg.Class == IR::GPRPairClass.Val, "Unexpected Class: {}", Reg.Class);

    return GeneralPairRegisters[Reg.Reg];
  }

  [[nodiscard]] FEXCore::IR::RegisterClassType GetRegClass(IR::NodeID Node) const;

  [[nodiscard]] IR::PhysicalRegister GetPhys(IR::NodeID Node) const {
    auto PhyReg = RAData->GetNodeRegister(Node);

    LOGMAN_THROW_A_FMT(!PhyReg.IsInvalid(), "Couldn't Allocate register for node: ssa{}. Class: {}", Node, PhyReg.Class);

    return PhyReg;
  }

  [[nodiscard]] bool IsFPR(IR::NodeID Node) const;
  [[nodiscard]] bool IsGPR(IR::NodeID Node) const;
  [[nodiscard]] bool IsGPRPair(IR::NodeID Node) const;

  [[nodiscard]] FEXCore::ARMEmitter::ExtendedMemOperand GenerateMemOperand(uint8_t AccessSize,
                                              FEXCore::ARMEmitter::Register Base,
                                              IR::OrderedNodeWrapper Offset,
                                              IR::MemOffsetType OffsetType,
                                              uint8_t OffsetScale);

  // NOTE: Will use TMP1 as a way to encode immediates that happen to fall outside
  //       the limits of the scalar plus immediate variant of SVE load/stores.
  //
  //       TMP1 is safe to use again once this memory operand is used with its
  //       equivalent loads or stores that this was called for.
  [[nodiscard]] FEXCore::ARMEmitter::SVEMemOperand GenerateSVEMemOperand(uint8_t AccessSize,
                                                    FEXCore::ARMEmitter::Register Base,
                                                    IR::OrderedNodeWrapper Offset,
                                                    IR::MemOffsetType OffsetType,
                                                    uint8_t OffsetScale);

  [[nodiscard]] bool IsInlineConstant(const IR::OrderedNodeWrapper& Node, uint64_t* Value = nullptr) const;
  [[nodiscard]] bool IsInlineEntrypointOffset(const IR::OrderedNodeWrapper& WNode, uint64_t* Value) const;

  struct LiveRange {
    uint32_t Begin;
    uint32_t End;
  };

  // This is purely a debugging aid for developers to see if they are in JIT code space when inspecting raw memory
  void EmitDetectionString();
  IR::RegisterAllocationPass *RAPass;
  IR::RegisterAllocationData *RAData;
  FEXCore::Core::DebugData *DebugData;

  void ResetStack();
  /**
   * @name Relocations
   * @{ */

    uint64_t GetNamedSymbolLiteral(FEXCore::CPU::RelocNamedSymbolLiteral::NamedSymbol Op);

    /**
     * @brief A literal pair relocation object for named symbol literals
     */
    struct NamedSymbolLiteralPair {
      ARMEmitter::ForwardLabel Loc;
      uint64_t Lit;
      Relocation MoveABI{};
    };

    /**
     * @brief Inserts a thunk relocation
     *
     * @param Reg - The GPR to move the thunk handler in to
     * @param Sum - The hash of the thunk
     */
    void InsertNamedThunkRelocation(ARMEmitter::Register Reg, const IR::SHA256Sum &Sum);

    /**
     * @brief Inserts a guest GPR move relocation
     *
     * @param Reg - The GPR to move the guest RIP in to
     * @param Constant - The guest RIP that will be relocated
     */
    void InsertGuestRIPMove(ARMEmitter::Register Reg, uint64_t Constant);

    /**
     * @brief Inserts a named symbol as a literal in memory
     *
     * Need to use `PlaceNamedSymbolLiteral` with the return value to place the literal in the desired location
     *
     * @param Op The named symbol to place
     *
     * @return A temporary `NamedSymbolLiteralPair`
     */
    NamedSymbolLiteralPair InsertNamedSymbolLiteral(FEXCore::CPU::RelocNamedSymbolLiteral::NamedSymbol Op);

    /**
     * @brief Place the named symbol literal relocation in memory
     *
     * @param Lit - Which literal to place
     */
    void PlaceNamedSymbolLiteral(NamedSymbolLiteralPair &Lit);

    fextl::vector<FEXCore::CPU::Relocation> Relocations;

    ///< Relocation code loading
    bool ApplyRelocations(uint64_t GuestEntry, uint64_t CodeEntry, uint64_t CursorEntry, size_t NumRelocations, const char* EntryRelocations);

  /**  @} */

  uint32_t SpillSlots{};
  using OpType = void (Arm64JITCore::*)(IR::IROp_Header const *IROp, IR::NodeID Node);

  // Runtime selection;
  // Load and store register style.
  OpType RT_LoadRegister;
  OpType RT_StoreRegister;
  // Load and store TSO memory style
  OpType RT_LoadMemTSO;
  OpType RT_StoreMemTSO;

#define DEF_OP(x) void Op_##x(IR::IROp_Header const *IROp, IR::NodeID Node)

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
  DEF_OP(AddNZCV);
  DEF_OP(TestNZ);
  DEF_OP(Sub);
  DEF_OP(SubNZCV);
  DEF_OP(Neg);
  DEF_OP(Abs);
  DEF_OP(Mul);
  DEF_OP(UMul);
  DEF_OP(Div);
  DEF_OP(UDiv);
  DEF_OP(Rem);
  DEF_OP(URem);
  DEF_OP(MulH);
  DEF_OP(UMulH);
  DEF_OP(Or);
  DEF_OP(Orlshl);
  DEF_OP(Orlshr);
  DEF_OP(Ornror);
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
  DEF_OP(FindTrailingZeroes);
  DEF_OP(CountLeadingZeroes);
  DEF_OP(Rev);
  DEF_OP(Bfi);
  DEF_OP(Bfxil);
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
  DEF_OP(AtomicFetchCLR);
  DEF_OP(AtomicFetchOr);
  DEF_OP(AtomicFetchXor);
  DEF_OP(AtomicFetchNeg);
  DEF_OP(TelemetrySetValue);

  ///< Branch ops
  DEF_OP(CallbackReturn);
  DEF_OP(ExitFunction);
  DEF_OP(Jump);
  DEF_OP(CondJump);
  DEF_OP(Syscall);
  DEF_OP(InlineSyscall);
  DEF_OP(Thunk);
  DEF_OP(ValidateCode);
  DEF_OP(ThreadRemoveCodeEntry);
  DEF_OP(CPUID);
  DEF_OP(XGETBV);

  ///< Conversion ops
  DEF_OP(VInsGPR);
  DEF_OP(VCastFromGPR);
  DEF_OP(VDupFromGPR);
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
  DEF_OP(LoadRegisterSRA);
  DEF_OP(StoreRegisterSRA);
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
  DEF_OP(VLoadVectorMasked);
  DEF_OP(VStoreVectorMasked);
  DEF_OP(VLoadVectorElement);
  DEF_OP(VStoreVectorElement);
  DEF_OP(VBroadcastFromMem);
  DEF_OP(Push);
  DEF_OP(MemSet);
  DEF_OP(MemCpy);
  DEF_OP(ParanoidLoadMemTSO);
  DEF_OP(ParanoidStoreMemTSO);
  DEF_OP(CacheLineClear);
  DEF_OP(CacheLineClean);
  DEF_OP(CacheLineZero);

  ///< Misc ops
  DEF_OP(GuestOpcode);
  DEF_OP(Fence);
  DEF_OP(Break);
  DEF_OP(Print);
  DEF_OP(GetRoundingMode);
  DEF_OP(SetRoundingMode);
  DEF_OP(ProcessorID);
  DEF_OP(RDRAND);
  DEF_OP(Yield);

  ///< Move ops
  DEF_OP(ExtractElementPair);
  DEF_OP(CreateElementPair);

  ///< Vector ops
  DEF_OP(VectorZero);
  DEF_OP(VectorImm);
  DEF_OP(LoadNamedVectorConstant);
  DEF_OP(LoadNamedVectorIndexedConstant);
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
  DEF_OP(VTrn);
  DEF_OP(VTrn2);
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
  DEF_OP(VUShrSWide);
  DEF_OP(VSShrSWide);
  DEF_OP(VUShlSWide);
  DEF_OP(VInsElement);
  DEF_OP(VDupElement);
  DEF_OP(VExtr);
  DEF_OP(VUShrI);
  DEF_OP(VSShrI);
  DEF_OP(VShlI);
  DEF_OP(VUShrNI);
  DEF_OP(VUShrNI2);
  DEF_OP(VSXTL);
  DEF_OP(VSXTL2);
  DEF_OP(VUXTL);
  DEF_OP(VUXTL2);
  DEF_OP(VSQXTN);
  DEF_OP(VSQXTN2);
  DEF_OP(VSQXTNPair);
  DEF_OP(VSQXTUN);
  DEF_OP(VSQXTUN2);
  DEF_OP(VSQXTUNPair);
  DEF_OP(VSRSHR);
  DEF_OP(VSQSHL);
  DEF_OP(VMul);
  DEF_OP(VUMull);
  DEF_OP(VSMull);
  DEF_OP(VUMull2);
  DEF_OP(VSMull2);
  DEF_OP(VUMulH);
  DEF_OP(VSMulH);
  DEF_OP(VUABDL);
  DEF_OP(VUABDL2);
  DEF_OP(VTBL1);
  DEF_OP(VTBL2);
  DEF_OP(VRev32);
  DEF_OP(VRev64);
  DEF_OP(VFCADD);

  ///< Encryption ops
  DEF_OP(AESImc);
  DEF_OP(AESEnc);
  DEF_OP(AESEncLast);
  DEF_OP(AESDec);
  DEF_OP(AESDecLast);
  DEF_OP(AESKeyGenAssist);
  DEF_OP(CRC32);
  DEF_OP(PCLMUL);
#undef DEF_OP
};

} // namespace FEXCore::CPU
