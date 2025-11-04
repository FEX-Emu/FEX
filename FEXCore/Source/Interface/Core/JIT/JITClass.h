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
#include "Interface/Core/JIT/Relocations.h"
#include "Interface/IR/IR.h"
#include "Interface/IR/IntrusiveIRList.h"
#include "Interface/IR/RegisterAllocationData.h"

#include <FEXCore/Config/Config.h>
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/IR/IR.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/fextl/map.h>
#include <FEXCore/fextl/memory.h>
#include <FEXCore/fextl/string.h>
#include <FEXCore/fextl/vector.h>
#include <FEXCore/Utils/LongJump.h>

#include <CodeEmitter/Emitter.h>

#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <utility>
#include <variant>

namespace FEXCore::Core {
struct InternalThreadState;
}
namespace FEXCore::Context {
struct ExitFunctionLinkData;
}
namespace FEXCore::IR {
class RegisterAllocationPass;
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
  const bool HostSupportsSVE128 {};
  const bool HostSupportsSVE256 {};
  const bool HostSupportsAVX256 {};
  const bool HostSupportsRPRES {};
  const bool HostSupportsAFP {};

  struct RestartOptions {
    FEXCore::UncheckedLongJump::JumpBuf RestartJump;
    enum class Control : uint64_t {
      Incoming = 0,
      EnableFarARM64Jumps = 1,
    };
  };

  // FEXCore makes assumptions in the JIT about certain conditions being true.
  // In the rare case when those assumptions are broken, FEX needs to safely restart the JIT.
  RestartOptions RestartControl {};
  bool RequiresFarARM64Jumps {};

  ARMEmitter::BiDirectionalLabel* PendingTargetLabel {};
  ARMEmitter::BiDirectionalLabel* PendingCallReturnTargetLabel {};
  FEXCore::Context::ContextImpl* CTX {};
  const FEXCore::IR::IRListView* IR {};
  uint64_t Entry {};
  CPUBackend::CompiledCode CodeData {};

  fextl::vector<ARMEmitter::BiDirectionalLabel> JumpTargets;

  ARMEmitter::BiDirectionalLabel* JumpTarget(IR::OrderedNodeWrapper Node) {
    auto Block = IR->GetOp<IR::IROp_CodeBlock>(Node);
    return &JumpTargets[Block->ID];
  }

  fextl::map<IR::NodeID, ARMEmitter::BiDirectionalLabel> CallReturnTargets;

  struct PendingJumpThunk {
    uint64_t CallerAddress;
    uint64_t GuestRIP;
    ARMEmitter::ForwardLabel Label;
  };
  fextl::vector<PendingJumpThunk> PendingJumpThunks;

  Utils::PoolBufferWithTimedRetirement<uint8_t*, 5000, 500> TempAllocator;

  static uint64_t ExitFunctionLink(FEXCore::Core::CpuStateFrame* Frame, FEXCore::Context::ExitFunctionLinkData* Record);

  [[nodiscard]]
  ARMEmitter::Register GetReg(IR::PhysicalRegister Reg) const {
    const auto RegClass = Reg.AsRegClass();

    LOGMAN_THROW_A_FMT(RegClass == IR::RegClass::GPRFixed || RegClass == IR::RegClass::GPR, "Unexpected Class: {}", Reg.Class);

    if (RegClass == IR::RegClass::GPRFixed) {
      return StaticRegisters[Reg.Reg];
    } else if (RegClass == IR::RegClass::GPR) {
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
    const auto RegClass = Reg.AsRegClass();

    LOGMAN_THROW_A_FMT(RegClass == IR::RegClass::FPRFixed || RegClass == IR::RegClass::FPR, "Unexpected Class: {}", Reg.Class);

    if (RegClass == IR::RegClass::FPRFixed) {
      return StaticFPRegisters[Reg.Reg];
    } else if (RegClass == IR::RegClass::FPR) {
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
  static IR::RegClass GetRegClass(IR::Ref Node) {
    return IR::PhysicalRegister(Node).AsRegClass();
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
  static ARMEmitter::ShiftType ConvertIRShiftType(IR::ShiftType Shift) {
    return Shift == IR::ShiftType::LSL ? ARMEmitter::ShiftType::LSL :
           Shift == IR::ShiftType::LSR ? ARMEmitter::ShiftType::LSR :
           Shift == IR::ShiftType::ASR ? ARMEmitter::ShiftType::ASR :
                                         ARMEmitter::ShiftType::ROR;
  }

  [[nodiscard]]
  static ARMEmitter::Size ConvertSize(const IR::IROp_Header* Op) {
    return Op->Size == IR::OpSize::i64Bit ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;
  }

  [[nodiscard]]
  static ARMEmitter::Size ConvertSize48(const IR::IROp_Header* Op) {
    LOGMAN_THROW_A_FMT(Op->Size == IR::OpSize::i32Bit || Op->Size == IR::OpSize::i64Bit, "Invalid size");
    return ConvertSize(Op);
  }

  [[nodiscard]]
  static ARMEmitter::Size ConvertSize(IR::OpSize Size) {
    return Size == IR::OpSize::i64Bit ? ARMEmitter::Size::i64Bit : ARMEmitter::Size::i32Bit;
  }

  [[nodiscard]]
  static ARMEmitter::SubRegSize ConvertSubRegSize16(IR::OpSize ElementSize) {
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
  static ARMEmitter::SubRegSize ConvertSubRegSize16(const IR::IROp_Header* Op) {
    return ConvertSubRegSize16(Op->ElementSize);
  }

  [[nodiscard]]
  static ARMEmitter::SubRegSize ConvertSubRegSize8(IR::OpSize ElementSize) {
    LOGMAN_THROW_A_FMT(ElementSize != IR::OpSize::i128Bit, "Invalid size");
    return ConvertSubRegSize16(ElementSize);
  }

  [[nodiscard]]
  static ARMEmitter::SubRegSize ConvertSubRegSize8(const IR::IROp_Header* Op) {
    return ConvertSubRegSize8(Op->ElementSize);
  }

  [[nodiscard]]
  static ARMEmitter::SubRegSize ConvertSubRegSize4(const IR::IROp_Header* Op) {
    LOGMAN_THROW_A_FMT(Op->ElementSize != IR::OpSize::i64Bit, "Invalid size");
    return ConvertSubRegSize8(Op);
  }

  [[nodiscard]]
  static ARMEmitter::SubRegSize ConvertSubRegSize248(const IR::IROp_Header* Op) {
    LOGMAN_THROW_A_FMT(Op->ElementSize != IR::OpSize::i8Bit, "Invalid size");
    return ConvertSubRegSize8(Op);
  }

  [[nodiscard]]
  static ARMEmitter::VectorRegSizePair ConvertSubRegSizePair16(const IR::IROp_Header* Op) {
    return ARMEmitter::ToVectorSizePair(ConvertSubRegSize16(Op));
  }

  [[nodiscard]]
  static ARMEmitter::VectorRegSizePair ConvertSubRegSizePair8(const IR::IROp_Header* Op) {
    LOGMAN_THROW_A_FMT(Op->ElementSize != IR::OpSize::i128Bit, "Invalid size");
    return ConvertSubRegSizePair16(Op);
  }

  [[nodiscard]]
  static ARMEmitter::VectorRegSizePair ConvertSubRegSizePair248(const IR::IROp_Header* Op) {
    LOGMAN_THROW_A_FMT(Op->ElementSize != IR::OpSize::i8Bit, "Invalid size");
    return ConvertSubRegSizePair8(Op);
  }

  [[nodiscard]]
  static ARMEmitter::Condition MapCC(IR::CondClass Cond) {
    switch (Cond) {
    case IR::CondClass::EQ: return ARMEmitter::Condition::CC_EQ;
    case IR::CondClass::NEQ: return ARMEmitter::Condition::CC_NE;
    case IR::CondClass::SGE: return ARMEmitter::Condition::CC_GE;
    case IR::CondClass::SLT: return ARMEmitter::Condition::CC_LT;
    case IR::CondClass::SGT: return ARMEmitter::Condition::CC_GT;
    case IR::CondClass::SLE: return ARMEmitter::Condition::CC_LE;
    case IR::CondClass::UGE: return ARMEmitter::Condition::CC_CS;
    case IR::CondClass::ULT: return ARMEmitter::Condition::CC_CC;
    case IR::CondClass::UGT: return ARMEmitter::Condition::CC_HI;
    case IR::CondClass::ULE: return ARMEmitter::Condition::CC_LS;
    case IR::CondClass::FLU: return ARMEmitter::Condition::CC_LT;
    case IR::CondClass::FGE: return ARMEmitter::Condition::CC_GE;
    case IR::CondClass::FLEU: return ARMEmitter::Condition::CC_LE;
    case IR::CondClass::FGT: return ARMEmitter::Condition::CC_GT;
    case IR::CondClass::FU:
    case IR::CondClass::VS: return ARMEmitter::Condition::CC_VS;
    case IR::CondClass::FNU:
    case IR::CondClass::VC: return ARMEmitter::Condition::CC_VC;
    case IR::CondClass::MI: return ARMEmitter::Condition::CC_MI;
    case IR::CondClass::PL: return ARMEmitter::Condition::CC_PL;
    default: LOGMAN_MSG_A_FMT("Unsupported compare type"); return ARMEmitter::Condition::CC_NV;
    }
  }

  [[nodiscard]]
  static bool IsFPR(IR::RegClass Class) {
    return Class == IR::RegClass::FPR || Class == IR::RegClass::FPRFixed;
  }

  [[nodiscard]]
  static bool IsGPR(IR::RegClass Class) {
    return Class == IR::RegClass::GPR || Class == IR::RegClass::GPRFixed;
  }

  [[nodiscard]]
  static bool IsGPR(IR::Ref Node) {
    return IsGPR(GetRegClass(Node));
  }

  [[nodiscard]]
  static bool IsFPR(IR::Ref Node) {
    return IsFPR(GetRegClass(Node));
  }

  [[nodiscard]]
  static bool IsGPR(IR::OrderedNodeWrapper Wrap) {
    return IsGPR(IR::PhysicalRegister(Wrap).AsRegClass());
  }

  [[nodiscard]]
  static bool IsFPR(IR::OrderedNodeWrapper Wrap) {
    return IsFPR(IR::PhysicalRegister(Wrap).AsRegClass());
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

  void EmitLinkedBranch(uint64_t GuestRIP, bool Call) {
    PendingJumpThunks.push_back({GetCursorAddress<uint64_t>(), GuestRIP, {}});
    auto& Thunk = PendingJumpThunks.back();
    BindOrRestart(&Thunk.Label);
    if (Call) {
      bl_OrRestart(&Thunk.Label);
    } else {
      b_OrRestart(&Thunk.Label);
    }
  }

  // Restart helpers
  template<ARMEmitter::IsLabel T>
  void bl_OrRestart(T* Label) {
    if (bl(Label) == ARMEmitter::BranchEncodeSucceeded::Success) {
      return;
    }

    // We can support this but currently unnecessary.
    ERROR_AND_DIE_FMT("Tried to branch larger than 128MB away!");
    FEXCore::UncheckedLongJump::LongJump(RestartControl.RestartJump, FEXCore::ToUnderlying(RestartOptions::Control::EnableFarARM64Jumps));
  }

  template<ARMEmitter::IsLabel T>
  void b_OrRestart(T* Label) {
    if (b(Label) == ARMEmitter::BranchEncodeSucceeded::Success) {
      return;
    }

    // We can support this but currently unnecessary.
    ERROR_AND_DIE_FMT("Tried to branch larger than 128MB away!");
    FEXCore::UncheckedLongJump::LongJump(RestartControl.RestartJump, FEXCore::ToUnderlying(RestartOptions::Control::EnableFarARM64Jumps));
  }

  template<ARMEmitter::IsLabel T>
  void b_OrRestart(ARMEmitter::Condition Cond, T* Label) {
    if (RequiresFarARM64Jumps) {
      ARMEmitter::ForwardLabel Skip {};
      // Wrap a manual Cond check around an unconditional branch; this can encode larger offsets
      (void)b(InvertCondition(Cond), &Skip);
      if (b(Label) == ARMEmitter::BranchEncodeSucceeded::Failure) {
        ERROR_AND_DIE_FMT("Tried to branch larger than 128MB away!");
      }

      (void)Bind(&Skip);
      return;
    }

    if (b(Cond, Label) == ARMEmitter::BranchEncodeSucceeded::Success) {
      return;
    }

    FEXCore::UncheckedLongJump::LongJump(RestartControl.RestartJump, FEXCore::ToUnderlying(RestartOptions::Control::EnableFarARM64Jumps));
  }

  template<ARMEmitter::IsLabel T>
  void cbz_OrRestart(ARMEmitter::Size s, ARMEmitter::Register rt, T* Label) {
    if (RequiresFarARM64Jumps) {
      ARMEmitter::ForwardLabel Skip {};
      // Wrap a manual Cond check around an unconditional branch; this can encode larger offsets
      (void)cbnz(s, rt, &Skip);
      if (b(Label) == ARMEmitter::BranchEncodeSucceeded::Failure) {
        ERROR_AND_DIE_FMT("Tried to branch larger than 128MB away!");
      }

      (void)Bind(&Skip);
      return;
    }

    if (cbz(s, rt, Label) == ARMEmitter::BranchEncodeSucceeded::Success) {
      return;
    }

    FEXCore::UncheckedLongJump::LongJump(RestartControl.RestartJump, FEXCore::ToUnderlying(RestartOptions::Control::EnableFarARM64Jumps));
  }

  template<ARMEmitter::IsLabel T>
  void cbnz_OrRestart(ARMEmitter::Size s, ARMEmitter::Register rt, T* Label) {
    if (RequiresFarARM64Jumps) {
      ARMEmitter::ForwardLabel Skip {};
      // Wrap a manual Cond check around an unconditional branch; this can encode larger offsets
      (void)cbz(s, rt, &Skip);
      if (b(Label) == ARMEmitter::BranchEncodeSucceeded::Failure) {
        ERROR_AND_DIE_FMT("Tried to branch larger than 128MB away!");
      }

      (void)Bind(&Skip);
      return;
    }

    if (cbnz(s, rt, Label) == ARMEmitter::BranchEncodeSucceeded::Success) {
      return;
    }

    FEXCore::UncheckedLongJump::LongJump(RestartControl.RestartJump, FEXCore::ToUnderlying(RestartOptions::Control::EnableFarARM64Jumps));
  }

  template<ARMEmitter::IsLabel T>
  void tbz_OrRestart(ARMEmitter::Register rt, uint32_t Bit, T* Label) {
    if (RequiresFarARM64Jumps) {
      ARMEmitter::ForwardLabel Skip {};
      // Wrap a manual Cond check around an unconditional branch; this can encode larger offsets
      (void)tbnz(rt, Bit, &Skip);
      if (b(Label) == ARMEmitter::BranchEncodeSucceeded::Failure) {
        ERROR_AND_DIE_FMT("Tried to branch larger than 128MB away!");
      }

      (void)Bind(&Skip);
      return;
    }

    if (tbz(rt, Bit, Label) == ARMEmitter::BranchEncodeSucceeded::Success) {
      return;
    }

    FEXCore::UncheckedLongJump::LongJump(RestartControl.RestartJump, FEXCore::ToUnderlying(RestartOptions::Control::EnableFarARM64Jumps));
  }

  template<ARMEmitter::IsLabel T>
  void tbnz_OrRestart(ARMEmitter::Register rt, uint32_t Bit, T* Label) {
    if (RequiresFarARM64Jumps) {
      ARMEmitter::ForwardLabel Skip {};
      // Wrap a manual Cond check around an unconditional branch; this can encode larger offsets
      (void)tbz(rt, Bit, &Skip);
      if (b(Label) == ARMEmitter::BranchEncodeSucceeded::Failure) {
        ERROR_AND_DIE_FMT("Tried to branch larger than 128MB away!");
      }

      (void)Bind(&Skip);
      return;
    }

    if (tbnz(rt, Bit, Label) == ARMEmitter::BranchEncodeSucceeded::Success) {
      return;
    }

    FEXCore::UncheckedLongJump::LongJump(RestartControl.RestartJump, FEXCore::ToUnderlying(RestartOptions::Control::EnableFarARM64Jumps));
  }

  template<ARMEmitter::IsLabel T>
  void adr_OrRestart(ARMEmitter::Register rd, T* Label) {
    if (adr(rd, Label) == ARMEmitter::BranchEncodeSucceeded::Success) {
      return;
    }

    // We can support this but currently unnecessary.
    ERROR_AND_DIE_FMT("Long ADR currently unsupported!");
    FEXCore::UncheckedLongJump::LongJump(RestartControl.RestartJump, FEXCore::ToUnderlying(RestartOptions::Control::EnableFarARM64Jumps));
  }

  template<ARMEmitter::IsLabel T>
  void adrp_OrRestart(ARMEmitter::Register rd, T* Label) {
    if (adrp(rd, Label) == ARMEmitter::BranchEncodeSucceeded::Success) {
      return;
    }

    // We can support this but currently unnecessary.
    ERROR_AND_DIE_FMT("Long ADRP currently unsupported!");
    FEXCore::UncheckedLongJump::LongJump(RestartControl.RestartJump, FEXCore::ToUnderlying(RestartOptions::Control::EnableFarARM64Jumps));
  }

  template<ARMEmitter::IsLabel T>
  void BindOrRestart(T* Label) {
    if (Bind(Label)) {
      return;
    }

    if (RequiresFarARM64Jumps) {
      // This should have been caught before this point.
      ERROR_AND_DIE_FMT("Unhandled long bind");
      return;
    }

    FEXCore::UncheckedLongJump::LongJump(RestartControl.RestartJump, FEXCore::ToUnderlying(RestartOptions::Control::EnableFarARM64Jumps));
  }

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
  bool ApplyRelocations(uint64_t GuestEntry, std::span<std::byte> Code, std::span<const FEXCore::CPU::Relocation>);

  fextl::vector<FEXCore::CPU::Relocation> TakeRelocations() override;

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
                           size_t DataElementOffsetStart, size_t IndexElementOffsetStart, uint8_t OffsetScale, IR::OpSize AddrSize);

  void EmitTFCheck();

  void EmitSuspendInterruptCheck();

  void EmitEntryPoint(ARMEmitter::BackwardLabel& HeaderLabel, bool CheckTF);

#define DEF_OP(x) void Op_##x(IR::IROp_Header const* IROp, IR::Ref Node)

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
