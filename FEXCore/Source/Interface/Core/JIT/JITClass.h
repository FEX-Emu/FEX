// SPDX-License-Identifier: MIT
/*
$info$
tags: backend|arm64
$end_info$
*/

#pragma once

#include "Interface/Core/ArchHelpers/Arm64Emitter.h"
#include "Interface/Core/CPUBackend.h"
#include "Interface/Core/Dispatcher/Dispatcher.h"
#include "Interface/IR/IR.h"
#include "Interface/IR/IntrusiveIRList.h"
#include "Interface/IR/RegisterAllocationData.h"

#include <FEXCore/Core/CoreState.h>
#include <FEXCore/IR/IR.h>
#include <FEXCore/fextl/map.h>
#include <FEXCore/fextl/string.h>
#include <FEXCore/fextl/vector.h>

#include <CodeEmitter/Emitter.h>

#include <array>
#include <cstdint>
#include <utility>
#include <variant>

namespace FEXCore::Core {
struct InternalThreadState;
}

namespace FEXCore::CPU {
class Arm64JITCore final : public CPUBackend, public Arm64Emitter {
public:
  explicit Arm64JITCore(FEXCore::Context::ContextImpl* ctx, FEXCore::Core::InternalThreadState* Thread);
  ~Arm64JITCore() override;

  [[nodiscard]]
  CPUBackend::CompiledCode CompileCode(uint64_t Entry, uint64_t Size, bool SingleInst, const FEXCore::IR::IRListView* IR,
                                       FEXCore::Core::DebugData* DebugData, bool CheckTF) override;

  void ClearCache() override;

  void ClearRelocations() override {
    Relocations.clear();
  }

private:
  FEX_CONFIG_OPT(ParanoidTSO, PARANOIDTSO);

  const bool HostSupportsSVE128 {};
  const bool HostSupportsSVE256 {};
  const bool HostSupportsAVX256 {};
  const bool HostSupportsRPRES {};
  const bool HostSupportsAFP {};

  ARMEmitter::BiDirectionalLabel* PendingTargetLabel {};
  FEXCore::Context::ContextImpl* CTX {};
  const FEXCore::IR::IRListView* IR {};
  uint64_t Entry {};
  CPUBackend::CompiledCode CodeData {};

  fextl::map<IR::NodeID, ARMEmitter::BiDirectionalLabel> JumpTargets;

  Utils::PoolBufferWithTimedRetirement<uint8_t*, 5000, 500> TempAllocator;

  [[nodiscard]]
  ARMEmitter::Register GetReg(IR::PhysicalRegister Reg) const {
    LOGMAN_THROW_A_FMT(Reg.Class == IR::GPRFixedClass.Val || Reg.Class == IR::GPRClass.Val, "Unexpected Class: {}", Reg.Class);

    if (Reg.Class == IR::GPRFixedClass.Val) {
      return StaticRegisters[Reg.Reg];
    } else if (Reg.Class == IR::GPRClass.Val) {
      return GeneralRegisters[Reg.Reg];
    }

    FEX_UNREACHABLE;
  }

  [[nodiscard]]
  ARMEmitter::Register GetReg(IR::Ref Node) const {
    return GetReg(IR::PhysicalRegister(Node));
  }

  [[nodiscard]]
  ARMEmitter::Register GetReg(IR::OrderedNodeWrapper Wrap) const {
    return GetReg(IR::PhysicalRegister(Wrap));
  }

  [[nodiscard]]
  ARMEmitter::VRegister GetVReg(IR::PhysicalRegister Reg) const {
    LOGMAN_THROW_A_FMT(Reg.Class == IR::FPRFixedClass.Val || Reg.Class == IR::FPRClass.Val, "Unexpected Class: {}", Reg.Class);

    if (Reg.Class == IR::FPRFixedClass.Val) {
      return StaticFPRegisters[Reg.Reg];
    } else if (Reg.Class == IR::FPRClass.Val) {
      return GeneralFPRegisters[Reg.Reg];
    }

    FEX_UNREACHABLE;
  }

  [[nodiscard]]
  ARMEmitter::VRegister GetVReg(IR::Ref Node) const {
    return GetVReg(IR::PhysicalRegister(Node));
  }

  [[nodiscard]]
  ARMEmitter::VRegister GetVReg(IR::OrderedNodeWrapper Wrap) const {
    return GetVReg(IR::PhysicalRegister(Wrap));
  }

  [[nodiscard]]
  FEXCore::IR::RegisterClassType GetRegClass(IR::Ref Node) const {
    return FEXCore::IR::RegisterClassType {IR::PhysicalRegister(Node).Class};
  }

  [[nodiscard]]
  ARMEmitter::Register GetZeroableReg(IR::OrderedNodeWrapper Src) const {
    uint64_t Const;
    if (IsInlineConstant(Src, &Const)) {
      LOGMAN_THROW_A_FMT(Const == 0, "Only valid constant");
      return ARMEmitter::Reg::zr;
    } else {
      return GetReg(Src);
    }
  }

  // Converts IR-base shift type to ARMEmitter shift type.
  // Will be a no-op, only a type conversion since the two definitions match.
  [[nodiscard]]
  ARMEmitter::ShiftType ConvertIRShiftType(IR::ShiftType Shift) const {
    return Shift == IR::ShiftType::LSL ? ARMEmitter::ShiftType::LSL :
           Shift == IR::ShiftType::LSR ? ARMEmitter::ShiftType::LSR :
           Shift == IR::ShiftType::ASR ? ARMEmitter::ShiftType::ASR :
                                         ARMEmitter::ShiftType::ROR;
  }

  [[nodiscard]]
  ARMEmitter::Size ConvertSize(const IR::IROp_Header* Op) {
    return Op->Size == IR::OpSize::i64Bit ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;
  }

  [[nodiscard]]
  ARMEmitter::Size ConvertSize48(const IR::IROp_Header* Op) {
    LOGMAN_THROW_A_FMT(Op->Size == IR::OpSize::i32Bit || Op->Size == IR::OpSize::i64Bit, "Invalid size");
    return ConvertSize(Op);
  }

  [[nodiscard]]
  ARMEmitter::SubRegSize ConvertSubRegSize16(IR::OpSize ElementSize) {
    LOGMAN_THROW_A_FMT(ElementSize == IR::OpSize::i8Bit || ElementSize == IR::OpSize::i16Bit || ElementSize == IR::OpSize::i32Bit ||
                         ElementSize == IR::OpSize::i64Bit || ElementSize == IR::OpSize::i128Bit,
                       "Invalid size");
    return ElementSize == IR::OpSize::i8Bit  ? ARMEmitter::SubRegSize::i8Bit :
           ElementSize == IR::OpSize::i16Bit ? ARMEmitter::SubRegSize::i16Bit :
           ElementSize == IR::OpSize::i32Bit ? ARMEmitter::SubRegSize::i32Bit :
           ElementSize == IR::OpSize::i64Bit ? ARMEmitter::SubRegSize::i64Bit :
                                               ARMEmitter::SubRegSize::i128Bit;
  }

  [[nodiscard]]
  ARMEmitter::SubRegSize ConvertSubRegSize16(const IR::IROp_Header* Op) {
    return ConvertSubRegSize16(Op->ElementSize);
  }

  [[nodiscard]]
  ARMEmitter::SubRegSize ConvertSubRegSize8(IR::OpSize ElementSize) {
    LOGMAN_THROW_A_FMT(ElementSize != IR::OpSize::i128Bit, "Invalid size");
    return ConvertSubRegSize16(ElementSize);
  }

  [[nodiscard]]
  ARMEmitter::SubRegSize ConvertSubRegSize8(const IR::IROp_Header* Op) {
    return ConvertSubRegSize8(Op->ElementSize);
  }

  [[nodiscard]]
  ARMEmitter::SubRegSize ConvertSubRegSize4(const IR::IROp_Header* Op) {
    LOGMAN_THROW_A_FMT(Op->ElementSize != IR::OpSize::i64Bit, "Invalid size");
    return ConvertSubRegSize8(Op);
  }

  [[nodiscard]]
  ARMEmitter::SubRegSize ConvertSubRegSize248(const IR::IROp_Header* Op) {
    LOGMAN_THROW_A_FMT(Op->ElementSize != IR::OpSize::i8Bit, "Invalid size");
    return ConvertSubRegSize8(Op);
  }

  [[nodiscard]]
  ARMEmitter::VectorRegSizePair ConvertSubRegSizePair16(const IR::IROp_Header* Op) {
    return ARMEmitter::ToVectorSizePair(ConvertSubRegSize16(Op));
  }

  [[nodiscard]]
  ARMEmitter::VectorRegSizePair ConvertSubRegSizePair8(const IR::IROp_Header* Op) {
    LOGMAN_THROW_A_FMT(Op->ElementSize != IR::OpSize::i128Bit, "Invalid size");
    return ConvertSubRegSizePair16(Op);
  }

  [[nodiscard]]
  ARMEmitter::VectorRegSizePair ConvertSubRegSizePair248(const IR::IROp_Header* Op) {
    LOGMAN_THROW_A_FMT(Op->ElementSize != IR::OpSize::i8Bit, "Invalid size");
    return ConvertSubRegSizePair8(Op);
  }

  [[nodiscard]]
  ARMEmitter::Condition MapCC(IR::CondClassType Cond) {
    switch (Cond.Val) {
    case FEXCore::IR::COND_EQ: return ARMEmitter::Condition::CC_EQ;
    case FEXCore::IR::COND_NEQ: return ARMEmitter::Condition::CC_NE;
    case FEXCore::IR::COND_SGE: return ARMEmitter::Condition::CC_GE;
    case FEXCore::IR::COND_SLT: return ARMEmitter::Condition::CC_LT;
    case FEXCore::IR::COND_SGT: return ARMEmitter::Condition::CC_GT;
    case FEXCore::IR::COND_SLE: return ARMEmitter::Condition::CC_LE;
    case FEXCore::IR::COND_UGE: return ARMEmitter::Condition::CC_CS;
    case FEXCore::IR::COND_ULT: return ARMEmitter::Condition::CC_CC;
    case FEXCore::IR::COND_UGT: return ARMEmitter::Condition::CC_HI;
    case FEXCore::IR::COND_ULE: return ARMEmitter::Condition::CC_LS;
    case FEXCore::IR::COND_FLU: return ARMEmitter::Condition::CC_LT;
    case FEXCore::IR::COND_FGE: return ARMEmitter::Condition::CC_GE;
    case FEXCore::IR::COND_FLEU: return ARMEmitter::Condition::CC_LE;
    case FEXCore::IR::COND_FGT: return ARMEmitter::Condition::CC_GT;
    case FEXCore::IR::COND_FU: return ARMEmitter::Condition::CC_VS;
    case FEXCore::IR::COND_FNU: return ARMEmitter::Condition::CC_VC;
    case FEXCore::IR::COND_VS:
    case FEXCore::IR::COND_VC:
    case FEXCore::IR::COND_MI: return ARMEmitter::Condition::CC_MI;
    case FEXCore::IR::COND_PL: return ARMEmitter::Condition::CC_PL;
    default: LOGMAN_MSG_A_FMT("Unsupported compare type"); return ARMEmitter::Condition::CC_NV;
    }
  }

  [[nodiscard]]
  bool IsFPR(IR::RegisterClassType Class) const {
    return Class == IR::FPRClass || Class == IR::FPRFixedClass;
  }

  [[nodiscard]]
  bool IsGPR(IR::RegisterClassType Class) const {
    return Class == IR::GPRClass || Class == IR::GPRFixedClass;
  }

  [[nodiscard]]
  bool IsGPR(IR::Ref Node) {
    return IsGPR(GetRegClass(Node));
  }

  [[nodiscard]]
  bool IsFPR(IR::Ref Node) {
    return IsFPR(GetRegClass(Node));
  }

  [[nodiscard]]
  bool IsGPR(IR::OrderedNodeWrapper Wrap) {
    return IsGPR(IR::RegisterClassType {IR::PhysicalRegister(Wrap).Class});
  }

  [[nodiscard]]
  bool IsFPR(IR::OrderedNodeWrapper Wrap) {
    return IsFPR(IR::RegisterClassType {IR::PhysicalRegister(Wrap).Class});
  }

  [[nodiscard]]
  ARMEmitter::ExtendedMemOperand GenerateMemOperand(IR::OpSize AccessSize, ARMEmitter::Register Base, IR::OrderedNodeWrapper Offset,
                                                    IR::MemOffsetType OffsetType, uint8_t OffsetScale);

  [[nodiscard]]
  ARMEmitter::Register ApplyMemOperand(IR::OpSize AccessSize, ARMEmitter::Register Base, ARMEmitter::Register Tmp,
                                       IR::OrderedNodeWrapper Offset, IR::MemOffsetType OffsetType, uint8_t OffsetScale);

  // NOTE: Will use TMP1 as a way to encode immediates that happen to fall outside
  //       the limits of the scalar plus immediate variant of SVE load/stores.
  //
  //       TMP1 is safe to use again once this memory operand is used with its
  //       equivalent loads or stores that this was called for.
  [[nodiscard]]
  ARMEmitter::SVEMemOperand GenerateSVEMemOperand(IR::OpSize AccessSize, ARMEmitter::Register Base, IR::OrderedNodeWrapper Offset,
                                                  IR::MemOffsetType OffsetType, uint8_t OffsetScale);

  [[nodiscard]]
  bool IsInlineConstant(const IR::OrderedNodeWrapper& Node, uint64_t* Value = nullptr) const;
  [[nodiscard]]
  bool IsInlineEntrypointOffset(const IR::OrderedNodeWrapper& WNode, uint64_t* Value) const;

  struct LiveRange {
    uint32_t Begin;
    uint32_t End;
  };

  // This is purely a debugging aid for developers to see if they are in JIT code space when inspecting raw memory
  void EmitDetectionString();
  IR::RegisterAllocationPass* RAPass {};
  FEXCore::Core::DebugData* DebugData {};

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
    Relocation MoveABI {};
  };

  /**
   * @brief Inserts a thunk relocation
   *
   * @param Reg - The GPR to move the thunk handler in to
   * @param Sum - The hash of the thunk
   */
  void InsertNamedThunkRelocation(ARMEmitter::Register Reg, const IR::SHA256Sum& Sum);

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
  void PlaceNamedSymbolLiteral(NamedSymbolLiteralPair& Lit);

  fextl::vector<FEXCore::CPU::Relocation> Relocations;

  ///< Relocation code loading
  bool ApplyRelocations(uint64_t GuestEntry, uint64_t CodeEntry, uint64_t CursorEntry, size_t NumRelocations, const char* EntryRelocations);

  /**  @} */

  uint32_t SpillSlots {};
  using OpType = void (Arm64JITCore::*)(const IR::IROp_Header* IROp, IR::Ref Node);

  using ScalarFMAOpCaller =
    std::function<void(ARMEmitter::VRegister Dst, ARMEmitter::VRegister Src1, ARMEmitter::VRegister Src2, ARMEmitter::VRegister Src3)>;
  void VFScalarFMAOperation(IR::OpSize OpSize, IR::OpSize ElementSize, ScalarFMAOpCaller ScalarEmit, ARMEmitter::VRegister Dst,
                            ARMEmitter::VRegister Upper, ARMEmitter::VRegister Vector1, ARMEmitter::VRegister Vector2,
                            ARMEmitter::VRegister Addend);
  using ScalarBinaryOpCaller = std::function<void(ARMEmitter::VRegister Dst, ARMEmitter::VRegister Src1, ARMEmitter::VRegister Src2)>;
  void VFScalarOperation(IR::OpSize OpSize, IR::OpSize ElementSize, bool ZeroUpperBits, ScalarBinaryOpCaller ScalarEmit,
                         ARMEmitter::VRegister Dst, ARMEmitter::VRegister Vector1, ARMEmitter::VRegister Vector2);
  using ScalarUnaryOpCaller = std::function<void(ARMEmitter::VRegister Dst, std::variant<ARMEmitter::VRegister, ARMEmitter::Register> SrcVar)>;
  void VFScalarUnaryOperation(IR::OpSize OpSize, IR::OpSize ElementSize, bool ZeroUpperBits, ScalarUnaryOpCaller ScalarEmit,
                              ARMEmitter::VRegister Dst, ARMEmitter::VRegister Vector1,
                              std::variant<ARMEmitter::VRegister, ARMEmitter::Register> Vector2);

  void Emulate128BitGather(IR::OpSize Size, IR::OpSize ElementSize, ARMEmitter::VRegister Dst, ARMEmitter::VRegister IncomingDst,
                           std::optional<ARMEmitter::Register> BaseAddr, ARMEmitter::VRegister VectorIndexLow,
                           std::optional<ARMEmitter::VRegister> VectorIndexHigh, ARMEmitter::VRegister MaskReg, IR::OpSize VectorIndexSize,
                           size_t DataElementOffsetStart, size_t IndexElementOffsetStart, uint8_t OffsetScale);

  void EmitInterruptChecks(bool CheckTF);

  void EmitEntryPoint(ARMEmitter::BackwardLabel& HeaderLabel, bool CheckTF);

  // Runtime selection;
  // Load and store TSO memory style
  OpType RT_LoadMemTSO;
  OpType RT_StoreMemTSO;

#define DEF_OP(x) void Op_##x(IR::IROp_Header const* IROp, IR::Ref Node)

  // Dynamic Dispatcher supporting operations
  DEF_OP(ParanoidLoadMemTSO);
  DEF_OP(ParanoidStoreMemTSO);

  ///< Unhandled handler
  DEF_OP(Unhandled);

  ///< No-op Handler
  DEF_OP(NoOp);

#define IROP_DISPATCH_DEFS
#include <FEXCore/IR/IRDefines_Dispatch.inc>
#undef DEF_OP
};

#define DEF_OP(x) void Arm64JITCore::Op_##x(IR::IROp_Header const* IROp, IR::Ref Node)

[[nodiscard]]
fextl::unique_ptr<CPUBackend> CreateArm64JITCore(FEXCore::Context::ContextImpl* ctx, FEXCore::Core::InternalThreadState* Thread);

} // namespace FEXCore::CPU
