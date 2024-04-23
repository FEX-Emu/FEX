// SPDX-License-Identifier: MIT
/*
$info$
tags: backend|arm64
$end_info$
*/

#pragma once

#include "Interface/Core/ArchHelpers/Arm64Emitter.h"
#include "Interface/Core/ArchHelpers/CodeEmitter/Emitter.h"
#include "Interface/Core/CPUBackend.h"
#include "Interface/Core/Dispatcher/Dispatcher.h"
#include "Interface/IR/IR.h"
#include "Interface/IR/IntrusiveIRList.h"
#include "Interface/IR/RegisterAllocationData.h"

#include <aarch64/assembler-aarch64.h>
#include <aarch64/disasm-aarch64.h>

#include <FEXCore/Core/CoreState.h>
#include <FEXCore/IR/IR.h>
#include <FEXCore/fextl/map.h>
#include <FEXCore/fextl/string.h>
#include <FEXCore/fextl/vector.h>

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
  fextl::string GetName() override {
    return "JIT";
  }

  [[nodiscard]]
  CPUBackend::CompiledCode CompileCode(uint64_t Entry, const FEXCore::IR::IRListView* IR, FEXCore::Core::DebugData* DebugData,
                                       FEXCore::IR::RegisterAllocationData* RAData) override;

  [[nodiscard]]
  void* MapRegion(void* HostPtr, uint64_t, uint64_t) override {
    return HostPtr;
  }

  [[nodiscard]]
  bool NeedsOpDispatch() override {
    return true;
  }

  void ClearCache() override;

  void ClearRelocations() override {
    Relocations.clear();
  }

private:
  FEX_CONFIG_OPT(ParanoidTSO, PARANOIDTSO);
  FEX_CONFIG_OPT(VectorTSOEnabled, VECTORTSOENABLED);
  FEX_CONFIG_OPT(MemcpySetTSOEnabled, MEMCPYSETTSOENABLED);

  const bool HostSupportsSVE128 {};
  const bool HostSupportsSVE256 {};
  const bool HostSupportsRPRES {};
  const bool HostSupportsAFP {};

  ARMEmitter::BiDirectionalLabel* PendingTargetLabel;
  FEXCore::Context::ContextImpl* CTX;
  const FEXCore::IR::IRListView* IR;
  uint64_t Entry;
  CPUBackend::CompiledCode CodeData {};

  fextl::map<IR::NodeID, ARMEmitter::BiDirectionalLabel> JumpTargets;

  [[nodiscard]]
  FEXCore::ARMEmitter::Register GetReg(IR::NodeID Node) const {
    const auto Reg = GetPhys(Node);

    LOGMAN_THROW_AA_FMT(Reg.Class == IR::GPRFixedClass.Val || Reg.Class == IR::GPRClass.Val, "Unexpected Class: {}", Reg.Class);

    if (Reg.Class == IR::GPRFixedClass.Val) {
      return StaticRegisters[Reg.Reg];
    } else if (Reg.Class == IR::GPRClass.Val) {
      return GeneralRegisters[Reg.Reg];
    }

    FEX_UNREACHABLE;
  }

  [[nodiscard]]
  FEXCore::ARMEmitter::VRegister GetVReg(IR::NodeID Node) const {
    const auto Reg = GetPhys(Node);

    LOGMAN_THROW_AA_FMT(Reg.Class == IR::FPRFixedClass.Val || Reg.Class == IR::FPRClass.Val, "Unexpected Class: {}", Reg.Class);

    if (Reg.Class == IR::FPRFixedClass.Val) {
      return StaticFPRegisters[Reg.Reg];
    } else if (Reg.Class == IR::FPRClass.Val) {
      return GeneralFPRegisters[Reg.Reg];
    }

    FEX_UNREACHABLE;
  }

  [[nodiscard]]
  std::pair<FEXCore::ARMEmitter::Register, FEXCore::ARMEmitter::Register> GetRegPair(IR::NodeID Node) const {
    const auto Reg = GetPhys(Node);

    LOGMAN_THROW_AA_FMT(Reg.Class == IR::GPRPairClass.Val, "Unexpected Class: {}", Reg.Class);

    return std::make_pair(GeneralRegisters[Reg.Reg], GeneralRegisters[Reg.Reg + 1]);
  }

  [[nodiscard]]
  FEXCore::IR::RegisterClassType GetRegClass(IR::NodeID Node) const;

  [[nodiscard]]
  IR::PhysicalRegister GetPhys(IR::NodeID Node) const {
    auto PhyReg = RAData->GetNodeRegister(Node);

    LOGMAN_THROW_A_FMT(!PhyReg.IsInvalid(), "Couldn't Allocate register for node: ssa{}. Class: {}", Node, PhyReg.Class);

    return PhyReg;
  }

  [[nodiscard]]
  FEXCore::ARMEmitter::Register GetZeroableReg(IR::OrderedNodeWrapper Src) const {
    uint64_t Const;
    if (IsInlineConstant(Src, &Const)) {
      LOGMAN_THROW_AA_FMT(Const == 0, "Only valid constant");
      return ARMEmitter::Reg::zr;
    } else {
      return GetReg(Src.ID());
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
  bool IsFPR(IR::NodeID Node) const;
  [[nodiscard]]
  bool IsGPR(IR::NodeID Node) const;
  [[nodiscard]]
  bool IsGPRPair(IR::NodeID Node) const;

  [[nodiscard]]
  FEXCore::ARMEmitter::ExtendedMemOperand GenerateMemOperand(
    uint8_t AccessSize, FEXCore::ARMEmitter::Register Base, IR::OrderedNodeWrapper Offset, IR::MemOffsetType OffsetType, uint8_t OffsetScale);

  // NOTE: Will use TMP1 as a way to encode immediates that happen to fall outside
  //       the limits of the scalar plus immediate variant of SVE load/stores.
  //
  //       TMP1 is safe to use again once this memory operand is used with its
  //       equivalent loads or stores that this was called for.
  [[nodiscard]]
  FEXCore::ARMEmitter::SVEMemOperand GenerateSVEMemOperand(uint8_t AccessSize, FEXCore::ARMEmitter::Register Base,
                                                           IR::OrderedNodeWrapper Offset, IR::MemOffsetType OffsetType, uint8_t OffsetScale);

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
  IR::RegisterAllocationPass* RAPass;
  IR::RegisterAllocationData* RAData;
  FEXCore::Core::DebugData* DebugData;

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
  using OpType = void (Arm64JITCore::*)(const IR::IROp_Header* IROp, IR::NodeID Node);

  using ScalarBinaryOpCaller = std::function<void(ARMEmitter::VRegister Dst, ARMEmitter::VRegister Src1, ARMEmitter::VRegister Src2)>;
  void VFScalarOperation(uint8_t OpSize, uint8_t ElementSize, bool ZeroUpperBits, ScalarBinaryOpCaller ScalarEmit,
                         ARMEmitter::VRegister Dst, ARMEmitter::VRegister Vector1, ARMEmitter::VRegister Vector2);
  using ScalarUnaryOpCaller = std::function<void(ARMEmitter::VRegister Dst, std::variant<ARMEmitter::VRegister, ARMEmitter::Register> SrcVar)>;
  void VFScalarUnaryOperation(uint8_t OpSize, uint8_t ElementSize, bool ZeroUpperBits, ScalarUnaryOpCaller ScalarEmit, ARMEmitter::VRegister Dst,
                              ARMEmitter::VRegister Vector1, std::variant<ARMEmitter::VRegister, ARMEmitter::Register> Vector2);

  // Runtime selection;
  // Load and store TSO memory style
  OpType RT_LoadMemTSO;
  OpType RT_StoreMemTSO;

#define DEF_OP(x) void Op_##x(IR::IROp_Header const* IROp, IR::NodeID Node)

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

} // namespace FEXCore::CPU
