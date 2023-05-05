#pragma once

#include "FEXCore/Utils/EnumUtils.h"
#include "Interface/Core/ArchHelpers/CodeEmitter/Emitter.h"
#include "Interface/Core/ArchHelpers/CodeEmitter/Registers.h"

#include "Interface/Core/Dispatcher/Dispatcher.h"
#include "Interface/Core/ObjectCache/Relocations.h"

#include <aarch64/assembler-aarch64.h>
#include <aarch64/constants-aarch64.h>
#include <aarch64/cpu-aarch64.h>
#include <aarch64/operands-aarch64.h>
#include <platform-vixl.h>
#ifdef VIXL_DISASSEMBLER
#include <aarch64/disasm-aarch64.h>
#endif
#ifdef VIXL_SIMULATOR
#include <aarch64/simulator-aarch64.h>
#include <aarch64/simulator-constants-aarch64.h>
#endif

#include <FEXCore/Config/Config.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>

namespace FEXCore::CPU {
// Register x18 is unused in the current configuration.
// This is due to it being a platform register on wine platforms.
// TODO: Allow x18 register allocation in the future to gain one more register.

// All but x19 and x29 are caller saved
constexpr std::array<FEXCore::ARMEmitter::Register, 16> SRA64 = {
  FEXCore::ARMEmitter::Reg::r4, FEXCore::ARMEmitter::Reg::r5,
  FEXCore::ARMEmitter::Reg::r6, FEXCore::ARMEmitter::Reg::r7,
  FEXCore::ARMEmitter::Reg::r8, FEXCore::ARMEmitter::Reg::r9,
  FEXCore::ARMEmitter::Reg::r10, FEXCore::ARMEmitter::Reg::r11,
  // Registers that don't exist on 32-bit
  FEXCore::ARMEmitter::Reg::r12, FEXCore::ARMEmitter::Reg::r13,
  FEXCore::ARMEmitter::Reg::r14, FEXCore::ARMEmitter::Reg::r15,
  FEXCore::ARMEmitter::Reg::r16, FEXCore::ARMEmitter::Reg::r17,
  FEXCore::ARMEmitter::Reg::r19, FEXCore::ARMEmitter::Reg::r29
};

constexpr std::array<FEXCore::ARMEmitter::Register, 9 + 8> RA64 = {
  // All these callee saved
  FEXCore::ARMEmitter::Reg::r20, FEXCore::ARMEmitter::Reg::r21,
  FEXCore::ARMEmitter::Reg::r22, FEXCore::ARMEmitter::Reg::r23,
  FEXCore::ARMEmitter::Reg::r24, FEXCore::ARMEmitter::Reg::r25,
  FEXCore::ARMEmitter::Reg::r26, FEXCore::ARMEmitter::Reg::r27,
  FEXCore::ARMEmitter::Reg::r30,

  // Registers only available on 32-bit
  // All these are caller saved (except for r19).
  FEXCore::ARMEmitter::Reg::r12, FEXCore::ARMEmitter::Reg::r13,
  FEXCore::ARMEmitter::Reg::r14, FEXCore::ARMEmitter::Reg::r15,
  FEXCore::ARMEmitter::Reg::r16, FEXCore::ARMEmitter::Reg::r17,
  FEXCore::ARMEmitter::Reg::r19, FEXCore::ARMEmitter::Reg::r29
};

constexpr std::array<std::pair<FEXCore::ARMEmitter::Register, FEXCore::ARMEmitter::Register>, 4 + 3> RA64Pair = {{
  {FEXCore::ARMEmitter::Reg::r20, FEXCore::ARMEmitter::Reg::r21},
  {FEXCore::ARMEmitter::Reg::r22, FEXCore::ARMEmitter::Reg::r23},
  {FEXCore::ARMEmitter::Reg::r24, FEXCore::ARMEmitter::Reg::r25},
  {FEXCore::ARMEmitter::Reg::r26, FEXCore::ARMEmitter::Reg::r27},

  // Registers only available on 32-bit
  {FEXCore::ARMEmitter::Reg::r12, FEXCore::ARMEmitter::Reg::r13},
  {FEXCore::ARMEmitter::Reg::r14, FEXCore::ARMEmitter::Reg::r15},
  {FEXCore::ARMEmitter::Reg::r16, FEXCore::ARMEmitter::Reg::r17}
}};

// All are caller saved
constexpr std::array<FEXCore::ARMEmitter::VRegister, 16> SRAFPR = {
  FEXCore::ARMEmitter::VReg::v16, FEXCore::ARMEmitter::VReg::v17,
  FEXCore::ARMEmitter::VReg::v18, FEXCore::ARMEmitter::VReg::v19,
  FEXCore::ARMEmitter::VReg::v20, FEXCore::ARMEmitter::VReg::v21,
  FEXCore::ARMEmitter::VReg::v22, FEXCore::ARMEmitter::VReg::v23,

  // Registers that don't exist on 32-bit
  FEXCore::ARMEmitter::VReg::v24, FEXCore::ARMEmitter::VReg::v25,
  FEXCore::ARMEmitter::VReg::v26, FEXCore::ARMEmitter::VReg::v27,
  FEXCore::ARMEmitter::VReg::v28, FEXCore::ARMEmitter::VReg::v29,
  FEXCore::ARMEmitter::VReg::v30, FEXCore::ARMEmitter::VReg::v31
};

//  v8..v15 = (lower 64bits) Callee saved
constexpr std::array<FEXCore::ARMEmitter::VRegister, 12 + 8> RAFPR = {
  // v0 ~ v3 are used as temps.
  // FEXCore::ARMEmitter::VReg::v0, FEXCore::ARMEmitter::VReg::v1,
  // FEXCore::ARMEmitter::VReg::v2, FEXCore::ARMEmitter::VReg::v3,

  FEXCore::ARMEmitter::VReg::v4, FEXCore::ARMEmitter::VReg::v5,
  FEXCore::ARMEmitter::VReg::v6, FEXCore::ARMEmitter::VReg::v7,
  FEXCore::ARMEmitter::VReg::v8, FEXCore::ARMEmitter::VReg::v9,
  FEXCore::ARMEmitter::VReg::v10, FEXCore::ARMEmitter::VReg::v11,
  FEXCore::ARMEmitter::VReg::v12, FEXCore::ARMEmitter::VReg::v13,
  FEXCore::ARMEmitter::VReg::v14, FEXCore::ARMEmitter::VReg::v15,

  // Registers only available on 32-bit
  FEXCore::ARMEmitter::VReg::v24, FEXCore::ARMEmitter::VReg::v25,
  FEXCore::ARMEmitter::VReg::v26, FEXCore::ARMEmitter::VReg::v27,
  FEXCore::ARMEmitter::VReg::v28, FEXCore::ARMEmitter::VReg::v29,
  FEXCore::ARMEmitter::VReg::v30, FEXCore::ARMEmitter::VReg::v31
};

// Contains the address to the currently available CPU state
constexpr auto STATE = FEXCore::ARMEmitter::XReg::x28;

// GPR temporaries. Only x3 can be used across spill boundaries
// so if these ever need to change, be very careful about that.
constexpr auto TMP1 = FEXCore::ARMEmitter::XReg::x0;
constexpr auto TMP2 = FEXCore::ARMEmitter::XReg::x1;
constexpr auto TMP3 = FEXCore::ARMEmitter::XReg::x2;
constexpr auto TMP4 = FEXCore::ARMEmitter::XReg::x3;

// Vector temporaries
constexpr auto VTMP1 = FEXCore::ARMEmitter::VReg::v0;
constexpr auto VTMP2 = FEXCore::ARMEmitter::VReg::v1;
constexpr auto VTMP3 = FEXCore::ARMEmitter::VReg::v2;
constexpr auto VTMP4 = FEXCore::ARMEmitter::VReg::v3;

// Predicate register temporaries (used when AVX support is enabled)
// PRED_TMP_16B indicates a predicate register that indicates the first 16 bytes set to 1.
// PRED_TMP_32B indicates a predicate register that indicates the first 32 bytes set to 1.
constexpr FEXCore::ARMEmitter::PRegister PRED_TMP_16B = FEXCore::ARMEmitter::PReg::p6;
constexpr FEXCore::ARMEmitter::PRegister PRED_TMP_32B = FEXCore::ARMEmitter::PReg::p7;

// This class contains common emitter utility functions that can
// be used by both Arm64 JIT and ARM64 Dispatcher
class Arm64Emitter : public FEXCore::ARMEmitter::Emitter {
protected:
  Arm64Emitter(FEXCore::Context::ContextImpl *ctx, size_t size);
  ~Arm64Emitter();

  FEXCore::Context::ContextImpl *EmitterCTX;
  vixl::aarch64::CPU CPU;

  uint32_t ConfiguredGPRs;
  uint32_t ConfiguredSRAGPRs;
  uint32_t ConfiguredGPRPairs;
  uint32_t ConfiguredFPRs;
  uint32_t ConfiguredSRAFPRs;
  uint32_t ConfiguredDynamicGPRs;
  const FEXCore::ARMEmitter::Register *ConfiguredDynamicRegisterBase{};

  /**
   * @name Register Allocation
   * @{ */
  // 64-bit gets removal of additional pairs
  constexpr static uint32_t NumGPRs64       = RA64.size() - 8;
  constexpr static uint32_t NumSRAGPRs64    = SRA64.size();
  constexpr static uint32_t NumFPRs64       = RAFPR.size() - 8;
  constexpr static uint32_t NumSRAFPRs64    = SRAFPR.size();
  constexpr static uint32_t NumGPRPairs64   = RA64Pair.size() - 3;

  // 32-bit gets full array of GPR registers
  // SRA registers remove the additional 8
  constexpr static uint32_t NumGPRs32       = RA64.size();
  constexpr static uint32_t NumSRAGPRs32    = SRA64.size() - 8;
  constexpr static uint32_t NumFPRs32       = RAFPR.size();
  constexpr static uint32_t NumSRAFPRs32    = SRAFPR.size() - 8;
  constexpr static uint32_t NumGPRPairs32   = RA64Pair.size();

  constexpr static uint32_t RegisterClasses = 6;

  constexpr static uint64_t GPRBase = (0ULL << 32);
  constexpr static uint64_t FPRBase = (1ULL << 32);
  constexpr static uint64_t GPRPairBase = (2ULL << 32);

  /**  @} */

  constexpr static uint8_t RA_32 = 0;
  constexpr static uint8_t RA_64 = 1;
  constexpr static uint8_t RA_FPR = 2;

  void LoadConstant(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register Reg, uint64_t Constant, bool NOPPad = false);


  // NOTE: These functions WILL clobber the register TMP4 if AVX support is enabled
  //       and FPRs are being spilled or filled. If only GPRs are spilled/filled, then
  //       TMP4 is left alone.
  void SpillStaticRegs(bool FPRs = true, uint32_t GPRSpillMask = ~0U, uint32_t FPRSpillMask = ~0U);
  void FillStaticRegs(bool FPRs = true, uint32_t GPRFillMask = ~0U, uint32_t FPRFillMask = ~0U);

  // Register 0-18 + 29 + 30 are caller saved
  static constexpr uint32_t CALLER_GPR_MASK = 0b0110'0000'0000'0111'1111'1111'1111'1111U;

  // This isn't technically true because the lower 64-bits of v8..v15 are callee saved
  // We can't guarantee only the lower 64bits are used so flush everything
  static constexpr uint32_t CALLER_FPR_MASK = ~0U;

  void PushDynamicRegsAndLR(FEXCore::ARMEmitter::Register TmpReg);
  void PopDynamicRegsAndLR();

  void PushCalleeSavedRegisters();
  void PopCalleeSavedRegisters();

  void Align16B();

#ifdef VIXL_SIMULATOR
  // Generates a vixl simulator runtime call.
  //
  // This matches behaviour of vixl's macro assembler, but we need to reimplement it since we aren't using the macro assembler.
  // This isn't too complex with how vixl emits this.
  //
  // Emit:
  // 1) hlt(kRuntimeCallOpcode)
  // 2) Simulator wrapper handler
  // 3) Function to call
  // 4) Style of the function call (Call versus tail-call)

  template<typename R, typename... P>
  void GenerateRuntimeCall(R (*Function)(P...)) {
    uintptr_t SimulatorWrapperAddress = reinterpret_cast<uintptr_t>(
      &(vixl::aarch64::Simulator::RuntimeCallStructHelper<R, P...>::Wrapper));

    uintptr_t FunctionAddress = reinterpret_cast<uintptr_t>(Function);

    hlt(vixl::aarch64::kRuntimeCallOpcode);

    // Simulator wrapper address pointer.
    dc64(SimulatorWrapperAddress);

    // Runtime function address to call
    dc64(FunctionAddress);

    // Call type
    dc32(vixl::aarch64::kCallRuntime);
  }

  template<typename R, typename... P>
  void GenerateIndirectRuntimeCall(ARMEmitter::Register Reg) {
    uintptr_t SimulatorWrapperAddress = reinterpret_cast<uintptr_t>(
      &(vixl::aarch64::Simulator::RuntimeCallStructHelper<R, P...>::Wrapper));

    hlt(vixl::aarch64::kIndirectRuntimeCallOpcode);

    // Simulator wrapper address pointer.
    dc64(SimulatorWrapperAddress);

    // Register that contains the function to call
    dc32(Reg.Idx());

    // Call type
    dc32(vixl::aarch64::kCallRuntime);
  }

  template<>
  void GenerateIndirectRuntimeCall<float, __uint128_t>(ARMEmitter::Register Reg) {
    uintptr_t SimulatorWrapperAddress = reinterpret_cast<uintptr_t>(
      &(vixl::aarch64::Simulator::RuntimeCallStructHelper<float, __uint128_t>::Wrapper));

    hlt(vixl::aarch64::kIndirectRuntimeCallOpcode);

    // Simulator wrapper address pointer.
    dc64(SimulatorWrapperAddress);

    // Register that contains the function to call
    dc32(Reg.Idx());

    // Call type
    dc32(vixl::aarch64::kCallRuntime);
  }

#endif
#ifdef VIXL_DISASSEMBLER
  vixl::aarch64::PrintDisassembler Disasm {stderr};
#endif
  FEX_CONFIG_OPT(StaticRegisterAllocation, SRA);
};

}
