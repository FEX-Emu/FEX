#pragma once

#include "Interface/Core/Dispatcher/Dispatcher.h"
#include "Interface/Core/ObjectCache/Relocations.h"

#include "aarch64/assembler-aarch64.h"
#include "aarch64/constants-aarch64.h"
#include "aarch64/cpu-aarch64.h"
#include "aarch64/operands-aarch64.h"
#include "platform-vixl.h"
#include "FEXCore/Config/Config.h"

#include <array>
#include <stddef.h>
#include <stdint.h>
#include <utility>

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

// This class contains common emitter utility functions that can
// be used by both Arm64 JIT and ARM64 Dispatcher
class Arm64Emitter : public vixl::aarch64::Assembler {
protected:
  Arm64Emitter(FEXCore::Context::Context *ctx, size_t size);

  std::unique_ptr<FEXCore::CPU::Dispatcher> Dispatcher;

  FEXCore::Context::Context *EmitterCTX;
  vixl::aarch64::CPU CPU;
  void LoadConstant(vixl::aarch64::Register Reg, uint64_t Constant, bool NOPPad = false);
  void SpillStaticRegs(bool FPRs = true, uint32_t GPRSpillMask = ~0U, uint32_t FPRSpillMask = ~0U);
  void FillStaticRegs(bool FPRs = true, uint32_t GPRFillMask = ~0U, uint32_t FPRFillMask = ~0U);

  static constexpr uint32_t CALLER_GPR_MASK = 0b0011'1111'1111'1111'1111;

  // This isn't technically true because the lower 64-bits of v8..v15 are callee saved
  // We can't guarantee only the lower 64bits are used so flush everything
  static constexpr uint32_t CALLER_FPR_MASK = ~0U;

  void PushDynamicRegsAndLR();
  void PopDynamicRegsAndLR();

  void PushCalleeSavedRegisters();
  void PopCalleeSavedRegisters();

  void ResetStack();
  void Align16B();
  /**
   * @name Relocations
   * @{ */

    uint64_t GetNamedSymbolLiteral(FEXCore::CPU::RelocNamedSymbolLiteral::NamedSymbol Op);

    /**
     * @brief A literal pair relocation object for named symbol literals
     */
    struct NamedSymbolLiteralPair {
      Literal<uint64_t> Lit;
      Relocation MoveABI{};
    };

    /**
     * @brief Inserts a thunk relocation
     *
     * @param Reg - The GPR to move the thunk handler in to
     * @param Sum - The hash of the thunk
     */
    void InsertNamedThunkRelocation(vixl::aarch64::Register Reg, const IR::SHA256Sum &Sum);

    /**
     * @brief Inserts a guest GPR move relocation
     *
     * @param Reg - The GPR to move the guest RIP in to
     * @param Constant - The guest RIP that will be relocated
     */
    void InsertGuestRIPMove(vixl::aarch64::Register Reg, uint64_t Constant);

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

    std::vector<FEXCore::CPU::Relocation> Relocations;

    ///< Relocation code loading
    bool ApplyRelocations(uint64_t GuestEntry, uint64_t CodeEntry, uint64_t CursorEntry, size_t NumRelocations, const char* EntryRelocations);

  /**  @} */

  uint32_t SpillSlots{};
  /**
  * @brief Current guest RIP entrypoint
  */
  uint64_t GuestEntry{};

  FEX_CONFIG_OPT(StaticRegisterAllocation, SRA);
};

}
