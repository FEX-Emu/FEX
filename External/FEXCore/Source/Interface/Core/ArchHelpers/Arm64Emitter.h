#pragma once

#include "Interface/Core/Dispatcher/Dispatcher.h"
#include "Interface/Core/ObjectCache/Relocations.h"

#include <aarch64/assembler-aarch64.h>
#include <aarch64/constants-aarch64.h>
#include <aarch64/cpu-aarch64.h>
#include <aarch64/operands-aarch64.h>
#include <platform-vixl.h>
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

// Contains the address to the currently available CPU state
#define STATE x28

// GPR temporaries. Only x3 can be used across spill boundaries
// so if these ever need to change, be very careful about that.
#define TMP1 x0
#define TMP2 x1
#define TMP3 x2
#define TMP4 x3

// Vector temporaries
#define VTMP1 v1
#define VTMP2 v2
#define VTMP3 v3

// Predicate register temporaries (used when AVX support is enabled)
// PRED_TMP_16B indicates a predicate register that indicates the first 16 bytes set to 1.
// PRED_TMP_32B indicates a predicate register that indicates the first 32 bytes set to 1.
#define PRED_TMP_16B p6
#define PRED_TMP_32B p7

// This class contains common emitter utility functions that can
// be used by both Arm64 JIT and ARM64 Dispatcher
class Arm64Emitter : public vixl::aarch64::Assembler {
protected:
  Arm64Emitter(FEXCore::Context::Context *ctx, size_t size);
  ~Arm64Emitter();

  FEXCore::Context::Context *EmitterCTX;
  vixl::aarch64::CPU CPU;
  void LoadConstant(vixl::aarch64::Register Reg, uint64_t Constant, bool NOPPad = false);

  // NOTE: These functions WILL clobber the register TMP4 if AVX support is enabled
  //       and FPRs are being spilled or filled. If only GPRs are spilled/filled, then
  //       TMP4 is left alone.
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
      &(Simulator::RuntimeCallStructHelper<R, P...>::Wrapper));

    uintptr_t FunctionAddress = reinterpret_cast<uintptr_t>(Function);

    hlt(kRuntimeCallOpcode);

    // Simulator wrapper address pointer.
    dc(SimulatorWrapperAddress);

    // Runtime function address to call
    dc(FunctionAddress);

    // Call type
    dc32(kCallRuntime);
  }

  template<typename R, typename... P>
  void GenerateIndirectRuntimeCall(vixl::aarch64::Register Reg) {
    uintptr_t SimulatorWrapperAddress = reinterpret_cast<uintptr_t>(
      &(Simulator::RuntimeCallStructHelper<R, P...>::Wrapper));

    hlt(kIndirectRuntimeCallOpcode);

    // Simulator wrapper address pointer.
    dc(SimulatorWrapperAddress);

    // Register that contains the function to call
    dc(Reg.GetCode());

    // Call type
    dc32(kCallRuntime);
  }

  template<>
  void GenerateIndirectRuntimeCall<float, __uint128_t>(vixl::aarch64::Register Reg) {
    uintptr_t SimulatorWrapperAddress = reinterpret_cast<uintptr_t>(
      &(Simulator::RuntimeCallStructHelper<float, __uint128_t>::Wrapper));

    hlt(kIndirectRuntimeCallOpcode);

    // Simulator wrapper address pointer.
    dc(SimulatorWrapperAddress);

    // Register that contains the function to call
    dc(Reg.GetCode());

    // Call type
    dc32(kCallRuntime);
  }

#endif

  FEX_CONFIG_OPT(StaticRegisterAllocation, SRA);
};

}
