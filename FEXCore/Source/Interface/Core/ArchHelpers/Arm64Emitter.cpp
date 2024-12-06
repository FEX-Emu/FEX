// SPDX-License-Identifier: MIT
#include "Interface/Core/ArchHelpers/Arm64Emitter.h"
#include "FEXCore/Core/X86Enums.h"
#include "FEXCore/Utils/AllocatorHooks.h"
#include "Interface/Core/Dispatcher/Dispatcher.h"
#include "Interface/Context/Context.h"

#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/MathUtils.h>

#include <FEXHeaderUtils/BitUtils.h>
#include <CodeEmitter/Emitter.h>
#include <CodeEmitter/Registers.h>

#ifdef VIXL_DISASSEMBLER
#include <aarch64/cpu-aarch64.h>
#include <aarch64/instructions-aarch64.h>
#include <cpu-features.h>
#include <utils-vixl.h>
#endif

#include <array>
#include <tuple>
#include <utility>

namespace FEXCore::CPU {
namespace x64 {
#ifndef _M_ARM_64EC
  // All but x19 and x29 are caller saved
  // Note that rax/rdx are rearranged here so we can coalesce cmpxchg.
  constexpr std::array<ARMEmitter::Register, 18> SRA = {
    ARMEmitter::Reg::r4,
    ARMEmitter::Reg::r7,
    ARMEmitter::Reg::r5,
    ARMEmitter::Reg::r6,
    ARMEmitter::Reg::r8,
    ARMEmitter::Reg::r9,
    ARMEmitter::Reg::r10,
    ARMEmitter::Reg::r11,
    ARMEmitter::Reg::r12,
    ARMEmitter::Reg::r13,
    ARMEmitter::Reg::r14,
    ARMEmitter::Reg::r15,
    ARMEmitter::Reg::r16,
    ARMEmitter::Reg::r17,
    ARMEmitter::Reg::r19,
    ARMEmitter::Reg::r29,
    // PF/AF must be last.
    REG_PF,
    REG_AF,
  };

  constexpr std::array<ARMEmitter::Register, 8> RA = {
    // All these callee saved
    ARMEmitter::Reg::r20, ARMEmitter::Reg::r21, ARMEmitter::Reg::r22, ARMEmitter::Reg::r23,
    ARMEmitter::Reg::r24, ARMEmitter::Reg::r25, ARMEmitter::Reg::r30, ARMEmitter::Reg::r18,
  };

  // p6 and p7 registers are used as temporaries no not added here for RA
  // See PREF_TMP_16B and PREF_TMP_32B
  // p0-p1 are also used in the jit as temps.
  // Also p8-p15 cannot be used can only encode p0-p7, so we're left with p2-p5.
  constexpr std::array<ARMEmitter::PRegister, 4> PR = {ARMEmitter::PReg::p2, ARMEmitter::PReg::p3, ARMEmitter::PReg::p4, ARMEmitter::PReg::p5};

  constexpr unsigned RAPairs = 6;

  // All are caller saved
  constexpr std::array<ARMEmitter::VRegister, 16> SRAFPR = {
    ARMEmitter::VReg::v16, ARMEmitter::VReg::v17, ARMEmitter::VReg::v18, ARMEmitter::VReg::v19,
    ARMEmitter::VReg::v20, ARMEmitter::VReg::v21, ARMEmitter::VReg::v22, ARMEmitter::VReg::v23,
    ARMEmitter::VReg::v24, ARMEmitter::VReg::v25, ARMEmitter::VReg::v26, ARMEmitter::VReg::v27,
    ARMEmitter::VReg::v28, ARMEmitter::VReg::v29, ARMEmitter::VReg::v30, ARMEmitter::VReg::v31};

  //  v8..v15 = (lower 64bits) Callee saved
  constexpr std::array<ARMEmitter::VRegister, 14> RAFPR = {
    // v0 ~ v1 are used as temps.
    // ARMEmitter::VReg::v0, ARMEmitter::VReg::v1,

    ARMEmitter::VReg::v2,  ARMEmitter::VReg::v3,  ARMEmitter::VReg::v4,  ARMEmitter::VReg::v5,  ARMEmitter::VReg::v6,
    ARMEmitter::VReg::v7,  ARMEmitter::VReg::v8,  ARMEmitter::VReg::v9,  ARMEmitter::VReg::v10, ARMEmitter::VReg::v11,
    ARMEmitter::VReg::v12, ARMEmitter::VReg::v13, ARMEmitter::VReg::v14, ARMEmitter::VReg::v15,
  };
#else
  constexpr std::array<ARMEmitter::Register, 18> SRA = {
    ARMEmitter::Reg::r8,
    ARMEmitter::Reg::r0,
    ARMEmitter::Reg::r1,
    ARMEmitter::Reg::r27,
    // SP's register location isn't specified by the ARM64EC ABI, we choose to use r23
    ARMEmitter::Reg::r23,
    ARMEmitter::Reg::r29,
    ARMEmitter::Reg::r25,
    ARMEmitter::Reg::r26,
    ARMEmitter::Reg::r2,
    ARMEmitter::Reg::r3,
    ARMEmitter::Reg::r4,
    ARMEmitter::Reg::r5,
    ARMEmitter::Reg::r19,
    ARMEmitter::Reg::r20,
    ARMEmitter::Reg::r21,
    ARMEmitter::Reg::r22,
    REG_PF,
    REG_AF,
  };

  constexpr std::array<ARMEmitter::Register, 7> RA = {
    ARMEmitter::Reg::r6,  ARMEmitter::Reg::r7,  ARMEmitter::Reg::r14, ARMEmitter::Reg::r15,
    ARMEmitter::Reg::r16, ARMEmitter::Reg::r17, ARMEmitter::Reg::r30,
  };

  // p6 and p7 registers are used as temporaries no not added here for RA
  // See PREF_TMP_16B and PREF_TMP_32B
  // p0-p1 are also used in the jit as temps.
  // Also p8-p15 cannot be used can only encode p0-p7, so we're left with p2-p5.
  constexpr std::array<ARMEmitter::PRegister, 4> PR = {ARMEmitter::PReg::p2, ARMEmitter::PReg::p3, ARMEmitter::PReg::p4, ARMEmitter::PReg::p5};

  constexpr unsigned RAPairs = 6;

  constexpr std::array<ARMEmitter::VRegister, 16> SRAFPR = {
    ARMEmitter::VReg::v0,  ARMEmitter::VReg::v1,  ARMEmitter::VReg::v2,  ARMEmitter::VReg::v3,
    ARMEmitter::VReg::v4,  ARMEmitter::VReg::v5,  ARMEmitter::VReg::v6,  ARMEmitter::VReg::v7,
    ARMEmitter::VReg::v8,  ARMEmitter::VReg::v9,  ARMEmitter::VReg::v10, ARMEmitter::VReg::v11,
    ARMEmitter::VReg::v12, ARMEmitter::VReg::v13, ARMEmitter::VReg::v14, ARMEmitter::VReg::v15,
  };

  constexpr std::array<ARMEmitter::VRegister, 14> RAFPR = {
    ARMEmitter::VReg::v18, ARMEmitter::VReg::v19, ARMEmitter::VReg::v20, ARMEmitter::VReg::v21, ARMEmitter::VReg::v22,
    ARMEmitter::VReg::v23, ARMEmitter::VReg::v24, ARMEmitter::VReg::v25, ARMEmitter::VReg::v26, ARMEmitter::VReg::v27,
    ARMEmitter::VReg::v28, ARMEmitter::VReg::v29, ARMEmitter::VReg::v30, ARMEmitter::VReg::v31};
#endif

  // I wish this could get constexpr generated from SRA's definition but impossible until libstdc++12, libc++15.
  // SRA GPRs that need to be spilled when calling a function with `preserve_all` ABI.
  constexpr std::array<ARMEmitter::Register, 7> PreserveAll_SRA = {
    ARMEmitter::Reg::r4, ARMEmitter::Reg::r5,  ARMEmitter::Reg::r6,  ARMEmitter::Reg::r7,
    ARMEmitter::Reg::r8, ARMEmitter::Reg::r16, ARMEmitter::Reg::r17,
  };

  constexpr uint32_t PreserveAll_SRAMask = {[]() -> uint32_t {
    uint32_t Mask {};
    for (auto Reg : PreserveAll_SRA) {
      switch (Reg.Idx()) {
      case 0:
      case 1:
      case 2:
      case 3:
      case 4:
      case 5:
      case 6:
      case 7:
      case 8:
      case 16:
      case 17: Mask |= (1U << Reg.Idx()); break;
      default: break;
      }
    }

    return Mask;
  }()};

  // Dynamic GPRs
  constexpr std::array<ARMEmitter::Register, 1> PreserveAll_Dynamic = {
    // Only LR needs to get saved.
    ARMEmitter::Reg::r30};

  // SRA FPRs that need to be spilled when calling a function with `preserve_all` ABI.
  constexpr std::array<ARMEmitter::Register, 0> PreserveAll_SRAFPR = {
    // None.
  };

  constexpr uint32_t PreserveAll_SRAFPRMask = {[]() -> uint32_t {
    uint32_t Mask {};
    for (auto Reg : PreserveAll_SRAFPR) {
      Mask |= (1U << Reg.Idx());
    }
    return Mask;
  }()};

  // Dynamic FPRs
  // - v0-v7
  constexpr std::array<ARMEmitter::VRegister, 6> PreserveAll_DynamicFPR = {
    // v0 ~ v1 are temps
    ARMEmitter::VReg::v2, ARMEmitter::VReg::v3, ARMEmitter::VReg::v4, ARMEmitter::VReg::v5, ARMEmitter::VReg::v6, ARMEmitter::VReg::v7,
  };

  // SRA FPRs that need to be spilled when the host supports SVE-256bit with `preserve_all` ABI.
  // This is /all/ of the SRA registers
  constexpr std::array<ARMEmitter::VRegister, 16> PreserveAll_SRAFPRSVE = SRAFPR;

  constexpr uint32_t PreserveAll_SRAFPRSVEMask = {[]() -> uint32_t {
    uint32_t Mask {};
    for (auto Reg : PreserveAll_SRAFPRSVE) {
      Mask |= (1U << Reg.Idx());
    }
    return Mask;
  }()};

  // Dynamic FPRs when the host supports SVE-256bit.
  constexpr std::array<ARMEmitter::VRegister, 14> PreserveAll_DynamicFPRSVE = {
    // v0 ~ v1 are used as temps.
    ARMEmitter::VReg::v2,  ARMEmitter::VReg::v3,  ARMEmitter::VReg::v4,  ARMEmitter::VReg::v5,  ARMEmitter::VReg::v6,
    ARMEmitter::VReg::v7,  ARMEmitter::VReg::v8,  ARMEmitter::VReg::v9,  ARMEmitter::VReg::v10, ARMEmitter::VReg::v11,
    ARMEmitter::VReg::v12, ARMEmitter::VReg::v13, ARMEmitter::VReg::v14, ARMEmitter::VReg::v15,
  };
} // namespace x64

namespace x32 {
  // All but x19 and x29 are caller saved. eax/edx rearranged for cmpxchg.
  constexpr std::array<ARMEmitter::Register, 10> SRA = {
    ARMEmitter::Reg::r4,
    ARMEmitter::Reg::r7,
    ARMEmitter::Reg::r5,
    ARMEmitter::Reg::r6,
    ARMEmitter::Reg::r8,
    ARMEmitter::Reg::r9,
    ARMEmitter::Reg::r10,
    ARMEmitter::Reg::r11,
    // PF/AF must be last.
    REG_PF,
    REG_AF,
  };

  constexpr std::array<ARMEmitter::Register, 15> RA = {
    // All these callee saved
    ARMEmitter::Reg::r20,
    ARMEmitter::Reg::r21,
    ARMEmitter::Reg::r22,
    ARMEmitter::Reg::r23,
    ARMEmitter::Reg::r24,
    ARMEmitter::Reg::r25,

    // Registers only available on 32-bit
    // All these are caller saved (except for r19).
    ARMEmitter::Reg::r12,
    ARMEmitter::Reg::r13,
    ARMEmitter::Reg::r14,
    ARMEmitter::Reg::r15,
    ARMEmitter::Reg::r16,
    ARMEmitter::Reg::r17,
    ARMEmitter::Reg::r29,
    ARMEmitter::Reg::r30,

    ARMEmitter::Reg::r19,
  };

  constexpr unsigned RAPairs = 12;

  // p6 and p7 registers are used as temporaries no not added here for RA
  // See PREF_TMP_16B and PREF_TMP_32B
  // p0-p1 are also used in the jit as temps.
  // Also p8-p15 cannot be used can only encode p0-p7, so we're left with p2-p5.
  constexpr std::array<ARMEmitter::PRegister, 4> PR = {ARMEmitter::PReg::p2, ARMEmitter::PReg::p3, ARMEmitter::PReg::p4, ARMEmitter::PReg::p5};

  // All are caller saved
  constexpr std::array<ARMEmitter::VRegister, 8> SRAFPR = {
    ARMEmitter::VReg::v16, ARMEmitter::VReg::v17, ARMEmitter::VReg::v18, ARMEmitter::VReg::v19,
    ARMEmitter::VReg::v20, ARMEmitter::VReg::v21, ARMEmitter::VReg::v22, ARMEmitter::VReg::v23,
  };

  //  v8..v15 = (lower 64bits) Callee saved
  constexpr std::array<ARMEmitter::VRegister, 22> RAFPR = {
    // v0 ~ v1 are used as temps.
    // ARMEmitter::VReg::v0, ARMEmitter::VReg::v1,

    ARMEmitter::VReg::v2,  ARMEmitter::VReg::v3,  ARMEmitter::VReg::v4,  ARMEmitter::VReg::v5,  ARMEmitter::VReg::v6,
    ARMEmitter::VReg::v7,  ARMEmitter::VReg::v8,  ARMEmitter::VReg::v9,  ARMEmitter::VReg::v10, ARMEmitter::VReg::v11,
    ARMEmitter::VReg::v12, ARMEmitter::VReg::v13, ARMEmitter::VReg::v14, ARMEmitter::VReg::v15,

    ARMEmitter::VReg::v24, ARMEmitter::VReg::v25, ARMEmitter::VReg::v26, ARMEmitter::VReg::v27, ARMEmitter::VReg::v28,
    ARMEmitter::VReg::v29, ARMEmitter::VReg::v30, ARMEmitter::VReg::v31};

  // I wish this could get constexpr generated from SRA's definition but impossible until libstdc++12, libc++15.
  // SRA GPRs that need to be spilled when calling a function with `preserve_all` ABI.
  constexpr std::array<ARMEmitter::Register, 5> PreserveAll_SRA = {
    ARMEmitter::Reg::r4, ARMEmitter::Reg::r5, ARMEmitter::Reg::r6, ARMEmitter::Reg::r7, ARMEmitter::Reg::r8,
  };

  constexpr uint32_t PreserveAll_SRAMask = {[]() -> uint32_t {
    uint32_t Mask {};
    for (auto Reg : PreserveAll_SRA) {
      switch (Reg.Idx()) {
      case 0:
      case 1:
      case 2:
      case 3:
      case 4:
      case 5:
      case 6:
      case 7:
      case 8:
      case 16:
      case 17: Mask |= (1U << Reg.Idx()); break;
      default: break;
      }
    }

    return Mask;
  }()};

  // Dynamic GPRs
  constexpr std::array<ARMEmitter::Register, 3> PreserveAll_Dynamic = {ARMEmitter::Reg::r16, ARMEmitter::Reg::r17, ARMEmitter::Reg::r30};

  // SRA FPRs that need to be spilled when calling a function with `preserve_all` ABI.
  constexpr std::array<ARMEmitter::Register, 0> PreserveAll_SRAFPR = {
    // None.
  };

  constexpr uint32_t PreserveAll_SRAFPRMask = {[]() -> uint32_t {
    uint32_t Mask {};
    for (auto Reg : PreserveAll_SRAFPR) {
      Mask |= (1U << Reg.Idx());
    }
    return Mask;
  }()};

  // Dynamic FPRs
  // - v0-v7
  constexpr std::array<ARMEmitter::VRegister, 6> PreserveAll_DynamicFPR = {
    // v0 ~ v1 are temps
    ARMEmitter::VReg::v2, ARMEmitter::VReg::v3, ARMEmitter::VReg::v4, ARMEmitter::VReg::v5, ARMEmitter::VReg::v6, ARMEmitter::VReg::v7,
  };

  // SRA FPRs that need to be spilled when the host supports SVE-256bit with `preserve_all` ABI.
  // This is /all/ of the SRA registers
  constexpr std::array<ARMEmitter::VRegister, 8> PreserveAll_SRAFPRSVE = SRAFPR;

  constexpr uint32_t PreserveAll_SRAFPRSVEMask = {[]() -> uint32_t {
    uint32_t Mask {};
    for (auto Reg : PreserveAll_SRAFPRSVE) {
      Mask |= (1U << Reg.Idx());
    }
    return Mask;
  }()};

  // Dynamic FPRs when the host supports SVE-256bit.
  constexpr std::array<ARMEmitter::VRegister, 22> PreserveAll_DynamicFPRSVE = {
    // v0 ~ v1 are used as temps.
    ARMEmitter::VReg::v2,  ARMEmitter::VReg::v3,  ARMEmitter::VReg::v4,  ARMEmitter::VReg::v5,  ARMEmitter::VReg::v6,
    ARMEmitter::VReg::v7,  ARMEmitter::VReg::v8,  ARMEmitter::VReg::v9,  ARMEmitter::VReg::v10, ARMEmitter::VReg::v11,
    ARMEmitter::VReg::v12, ARMEmitter::VReg::v13, ARMEmitter::VReg::v14, ARMEmitter::VReg::v15,

    ARMEmitter::VReg::v24, ARMEmitter::VReg::v25, ARMEmitter::VReg::v26, ARMEmitter::VReg::v27, ARMEmitter::VReg::v28,
    ARMEmitter::VReg::v29, ARMEmitter::VReg::v30, ARMEmitter::VReg::v31};
} // namespace x32

// We want vixl to not allocate a default buffer. Jit and dispatcher will manually create one.
Arm64Emitter::Arm64Emitter(FEXCore::Context::ContextImpl* ctx, void* EmissionPtr, size_t size)
  : Emitter(static_cast<uint8_t*>(EmissionPtr), size)
  , EmitterCTX {ctx}
#ifdef VIXL_SIMULATOR
  , Simulator {&SimDecoder, stdout, vixl::aarch64::SimStack(SimulatorStackSize).Allocate()}
#endif
{
#ifdef VIXL_SIMULATOR
  FEX_CONFIG_OPT(ForceSVEWidth, FORCESVEWIDTH);
  // Hardcode a 256-bit vector width if we are running in the simulator.
  // Allow the user to override this.
  Simulator.SetVectorLengthInBits(ForceSVEWidth() ? ForceSVEWidth() : 256);
#endif
#ifdef VIXL_DISASSEMBLER
  // Only setup the disassembler if enabled.
  // vixl's decoder is expensive to setup.
  if (Disassemble()) {
    DisasmBuffer.resize(DISASM_BUFFER_SIZE);
    Disasm = fextl::make_unique<vixl::aarch64::Disassembler>(DisasmBuffer.data(), DISASM_BUFFER_SIZE);
    DisasmDecoder = fextl::make_unique<vixl::aarch64::Decoder>();
    DisasmDecoder->AppendVisitor(Disasm.get());
  }
#endif

  // Number of register available is dependent on what operating mode the proccess is in.
  if (EmitterCTX->Config.Is64BitMode()) {
    StaticRegisters = x64::SRA;
    GeneralRegisters = x64::RA;
    StaticFPRegisters = x64::SRAFPR;
    GeneralFPRegisters = x64::RAFPR;
    PredicateRegisters = x64::PR;
    PairRegisters = x64::RAPairs;
#ifdef _M_ARM_64EC
    ConfiguredDynamicRegisterBase = std::span(x64::RA.begin(), 7);
#endif
  } else {
    ConfiguredDynamicRegisterBase = std::span(x32::RA.begin() + 6, 8);
    PairRegisters = x32::RAPairs;

    StaticRegisters = x32::SRA;
    GeneralRegisters = x32::RA;

    StaticFPRegisters = x32::SRAFPR;
    GeneralFPRegisters = x32::RAFPR;

    PredicateRegisters = x32::PR;
  }
}

FEXCore::X86State::X86Reg Arm64Emitter::GetX86RegRelationToARMReg(ARMEmitter::Register Reg) {
  for (size_t i = 0; i < StaticRegisters.size(); ++i) {
    const auto& RegI = StaticRegisters[i];
    if (RegI == Reg) {
      // X86 Registers are mapped linerally from the StaticRegisters span.
      // Directly correlating Enum index to span index.
      return static_cast<FEXCore::X86State::X86Reg>(FEXCore::ToUnderlying(FEXCore::X86State::X86Reg::REG_RAX) + i);
    }
  }

  // Unmapped register.
  return FEXCore::X86State::X86Reg::REG_INVALID;
}

void Arm64Emitter::LoadConstant(ARMEmitter::Size s, ARMEmitter::Register Reg, uint64_t Constant, bool NOPPad) {
  bool Is64Bit = s == ARMEmitter::Size::i64Bit;
  int Segments = Is64Bit ? 4 : 2;

  if (Is64Bit && ((~Constant) >> 16) == 0) {
    movn(s, Reg, (~Constant) & 0xFFFF);

    if (NOPPad) {
      nop();
      nop();
      nop();
    }
    return;
  }

  if ((Constant >> 32) == 0) {
    // If the upper 32-bits is all zero, we can now switch to a 32-bit move.
    s = ARMEmitter::Size::i32Bit;
    Is64Bit = false;
    Segments = 2;
  }

  if (!Is64Bit && ((~Constant) & 0xFFFF0000) == 0) {
    movn(s, Reg.W(), (~Constant) & 0xFFFF);

    if (NOPPad) {
      nop();
      nop();
      nop();
    }
    return;
  }

  int RequiredMoveSegments {};

  // Count the number of move segments
  // We only want to use ADRP+ADD if we have more than 1 segment
  for (size_t i = 0; i < Segments; ++i) {
    uint16_t Part = (Constant >> (i * 16)) & 0xFFFF;
    if (Part != 0) {
      ++RequiredMoveSegments;
    }
  }

  // If this can be loaded with a mov bitmask.
  if (RequiredMoveSegments > 1) {
    // Only try to use this path if the number of segments is > 1.
    // `movz` is better than `orr` since hardware will rename or merge if possible when `movz` is used.
    const auto IsImm = ARMEmitter::Emitter::IsImmLogical(Constant, RegSizeInBits(s));
    if (IsImm) {
      orr(s, Reg, ARMEmitter::Reg::zr, Constant);
      if (NOPPad) {
        nop();
        nop();
        nop();
      }
      return;
    }
  }

  // If we can't handle negatives with the orr, try with movn+movk
  if (Is64Bit && ((~Constant) >> 32) == 0) {
    movn(s, Reg, (~Constant) & 0xFFFF);
    movk(s, Reg, (Constant >> 16) & 0xFFFF, 16);
    if (NOPPad) {
      nop();
      nop();
    }
    return;
  }

  // ADRP+ADD is specifically optimized in hardware
  // Check if we can use this
  auto PC = GetCursorAddress<uint64_t>();

  // PC aligned to page
  uint64_t AlignedPC = PC & ~0xFFFULL;

  // Offset from aligned PC
  int64_t AlignedOffset = static_cast<int64_t>(Constant) - static_cast<int64_t>(AlignedPC);

  int NumMoves = 0;

  // If the aligned offset is within the 4GB window then we can use ADRP+ADD
  // and the number of move segments more than 1
  if (RequiredMoveSegments > 1 && ARMEmitter::Emitter::IsInt32(AlignedOffset)) {
    // If this is 4k page aligned then we only need ADRP
    if ((AlignedOffset & 0xFFF) == 0) {
      adrp(Reg, AlignedOffset >> 12);
    } else {
      // If the constant is within 1MB of PC then we can still use ADR to load in a single instruction
      // 21-bit signed integer here
      int64_t SmallOffset = static_cast<int64_t>(Constant) - static_cast<int64_t>(PC);
      if (ARMEmitter::Emitter::IsInt21(SmallOffset)) {
        adr(Reg, SmallOffset);
      } else {
        // Need to use ADRP + ADD
        adrp(Reg, AlignedOffset >> 12);
        add(s, Reg, Reg, Constant & 0xFFF);
        NumMoves = 2;
      }
    }
  } else {
    int CurrentSegment = 0;
    for (; CurrentSegment < Segments; ++CurrentSegment) {
      uint16_t Part = (Constant >> (CurrentSegment * 16)) & 0xFFFF;
      if (Part) {
        movz(s, Reg, Part, CurrentSegment * 16);
        ++CurrentSegment;
        ++NumMoves;
        break;
      }
    }

    for (; CurrentSegment < Segments; ++CurrentSegment) {
      uint16_t Part = (Constant >> (CurrentSegment * 16)) & 0xFFFF;
      if (Part) {
        movk(s, Reg, Part, CurrentSegment * 16);
        ++NumMoves;
      }
    }

    if (NumMoves == 0) {
      // If we didn't move anything that means this is a zero move. Special case this.
      movz(s, Reg, 0);
      ++NumMoves;
    }
  }

  if (NOPPad) {
    for (int i = NumMoves; i < Segments; ++i) {
      nop();
    }
  }
}

void Arm64Emitter::PushCalleeSavedRegisters() {
  // We need to save pairs of registers
  // We save r19-r30
  const fextl::vector<std::pair<ARMEmitter::XRegister, ARMEmitter::XRegister>> CalleeSaved = {{
    {ARMEmitter::XReg::x19, ARMEmitter::XReg::x20},
    {ARMEmitter::XReg::x21, ARMEmitter::XReg::x22},
    {ARMEmitter::XReg::x23, ARMEmitter::XReg::x24},
    {ARMEmitter::XReg::x25, ARMEmitter::XReg::x26},
    {ARMEmitter::XReg::x27, ARMEmitter::XReg::x28},
    {ARMEmitter::XReg::x29, ARMEmitter::XReg::x30},
  }};

  for (auto& RegPair : CalleeSaved) {
    stp<ARMEmitter::IndexType::PRE>(RegPair.first, RegPair.second, ARMEmitter::Reg::rsp, -16);
  }

  // Additionally we need to store the lower 64bits of v8-v15
  // Here's a fun thing, we can use two ST4 instructions to store everything
  // We just need a single sub to sp before that
  const std::array< std::tuple<ARMEmitter::DRegister, ARMEmitter::DRegister, ARMEmitter::DRegister, ARMEmitter::DRegister>, 2> FPRs = {{
    {ARMEmitter::DReg::d8, ARMEmitter::DReg::d9, ARMEmitter::DReg::d10, ARMEmitter::DReg::d11},
    {ARMEmitter::DReg::d12, ARMEmitter::DReg::d13, ARMEmitter::DReg::d14, ARMEmitter::DReg::d15},
  }};

  uint32_t VectorSaveSize = sizeof(uint64_t) * 8;
  sub(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::rsp, ARMEmitter::Reg::rsp, VectorSaveSize);
  // SP supporting move
  // We just saved x19 so it is safe
  add(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::r19, ARMEmitter::Reg::rsp, 0);

  for (auto& RegQuad : FPRs) {
    st4(ARMEmitter::SubRegSize::i64Bit, std::get<0>(RegQuad), std::get<1>(RegQuad), std::get<2>(RegQuad), std::get<3>(RegQuad), 0,
        ARMEmitter::Reg::r19, 32);
  }
}

void Arm64Emitter::PopCalleeSavedRegisters() {
  const std::array< std::tuple<ARMEmitter::DRegister, ARMEmitter::DRegister, ARMEmitter::DRegister, ARMEmitter::DRegister>, 2> FPRs = {{
    {ARMEmitter::DReg::d12, ARMEmitter::DReg::d13, ARMEmitter::DReg::d14, ARMEmitter::DReg::d15},
    {ARMEmitter::DReg::d8, ARMEmitter::DReg::d9, ARMEmitter::DReg::d10, ARMEmitter::DReg::d11},
  }};

  for (auto& RegQuad : FPRs) {
    ld4(ARMEmitter::SubRegSize::i64Bit, std::get<0>(RegQuad), std::get<1>(RegQuad), std::get<2>(RegQuad), std::get<3>(RegQuad), 0,
        ARMEmitter::Reg::rsp, 32);
  }

  const fextl::vector<std::pair<ARMEmitter::XRegister, ARMEmitter::XRegister>> CalleeSaved = {{
    {ARMEmitter::XReg::x29, ARMEmitter::XReg::x30},
    {ARMEmitter::XReg::x27, ARMEmitter::XReg::x28},
    {ARMEmitter::XReg::x25, ARMEmitter::XReg::x26},
    {ARMEmitter::XReg::x23, ARMEmitter::XReg::x24},
    {ARMEmitter::XReg::x21, ARMEmitter::XReg::x22},
    {ARMEmitter::XReg::x19, ARMEmitter::XReg::x20},
  }};

  for (auto& RegPair : CalleeSaved) {
    ldp<ARMEmitter::IndexType::POST>(RegPair.first, RegPair.second, ARMEmitter::Reg::rsp, 16);
  }
}

void Arm64Emitter::FillSpecialRegs(ARMEmitter::Register TmpReg, ARMEmitter::Register TmpReg2, bool SetFIZ, bool SetPredRegs) {
#ifndef VIXL_SIMULATOR
  if (EmitterCTX->HostFeatures.SupportsAFP) {
    // Enable AFP features when filling JIT state.
    mrs(TmpReg, ARMEmitter::SystemRegister::FPCR);

    // Enable FPCR.NEP and FPCR.AH
    // NEP(2): Changes ASIMD scalar instructions to insert in to the lower bits of the destination.
    // AH(1):  Changes NaN behaviour in some instructions. Specifically fmin, fmax.
    //
    // Additional interesting AFP bits:
    // FIZ(0): Flush Inputs to Zero
    orr(ARMEmitter::Size::i64Bit, TmpReg, TmpReg,
        (1U << 2) |   // NEP
          (1U << 1)); // AH

    if (SetFIZ) {
      // Insert MXCSR.DAZ in to FIZ
      ldr(TmpReg2.W(), STATE.R(), offsetof(FEXCore::Core::CPUState, mxcsr));
      bfxil(ARMEmitter::Size::i64Bit, TmpReg, TmpReg2, 6, 1);
    }

    msr(ARMEmitter::SystemRegister::FPCR, TmpReg);
  }
#endif

  if (SetPredRegs) {
    // Set up predicate registers.
    // We don't bother spilling these in SpillStaticRegs,
    // since all that matters is we restore them on a fill.
    // It's not a concern if they get trounced by something else.
    if (EmitterCTX->HostFeatures.SupportsSVE256) {
      ptrue(ARMEmitter::SubRegSize::i8Bit, PRED_TMP_32B, ARMEmitter::PredicatePattern::SVE_VL32);
    }

    if (EmitterCTX->HostFeatures.SupportsSVE128) {
      ptrue(ARMEmitter::SubRegSize::i8Bit, PRED_TMP_16B, ARMEmitter::PredicatePattern::SVE_VL16);
    }
  }
}

void Arm64Emitter::SpillStaticRegs(ARMEmitter::Register TmpReg, bool FPRs, uint32_t GPRSpillMask, uint32_t FPRSpillMask) {
#ifndef VIXL_SIMULATOR
  if (EmitterCTX->HostFeatures.SupportsAFP) {
    // Disable AFP features when spilling registers.
    //
    // Disable FPCR.NEP and FPCR.AH and FPCR.FIZ
    // NEP(2): Changes ASIMD scalar instructions to insert in to the lower bits of the destination.
    // AH(1):  Changes NaN behaviour in some instructions. Specifically fmin, fmax.
    //         Also interacts with RPRES to change reciprocal/rsqrt precision from 8-bit mantissa to 12-bit.
    //
    // Additional interesting AFP bits:
    // FIZ(0): Flush Inputs to Zero
    mrs(TmpReg, ARMEmitter::SystemRegister::FPCR);
    bic(ARMEmitter::Size::i64Bit, TmpReg, TmpReg,
        (1U << 2) |   // NEP
          (1U << 1) | // AH
          (1U << 0)); // FIZ
    msr(ARMEmitter::SystemRegister::FPCR, TmpReg);
  }
#endif

  // Regardless of what GPRs/FPRs we're spilling, we need to spill NZCV since it
  // is always static and almost certainly clobbered by the subsequent code.
  //
  // TODO: Can we prove that NZCV is not used across a call in some cases and
  // omit this? Might help x87 perf? Future idea.
  mrs(TmpReg, ARMEmitter::SystemRegister::NZCV);
  str(TmpReg.W(), STATE.R(), offsetof(FEXCore::Core::CpuStateFrame, State.flags[24]));

  // PF/AF are special, remove them from the mask
  uint32_t PFAFMask = ((1u << REG_PF.Idx()) | ((1u << REG_AF.Idx())));
  unsigned PFAFSpillMask = GPRSpillMask & PFAFMask;
  GPRSpillMask &= ~PFAFSpillMask;

  for (size_t i = 0; i < StaticRegisters.size(); i += 2) {
    auto Reg1 = StaticRegisters[i];
    auto Reg2 = StaticRegisters[i + 1];
    if (((1U << Reg1.Idx()) & GPRSpillMask) && ((1U << Reg2.Idx()) & GPRSpillMask)) {
      stp<ARMEmitter::IndexType::OFFSET>(Reg1.X(), Reg2.X(), STATE.R(), offsetof(FEXCore::Core::CpuStateFrame, State.gregs[i]));
    } else if (((1U << Reg1.Idx()) & GPRSpillMask)) {
      str(Reg1.X(), STATE.R(), offsetof(FEXCore::Core::CpuStateFrame, State.gregs[i]));
    } else if (((1U << Reg2.Idx()) & GPRSpillMask)) {
      str(Reg2.X(), STATE.R(), offsetof(FEXCore::Core::CpuStateFrame, State.gregs[i + 1]));
    }
  }

  // Now handle PF/AF
  if (PFAFSpillMask) {
    LOGMAN_THROW_A_FMT(PFAFSpillMask == PFAFMask, "PF/AF not spilled together");

    str(REG_PF.X(), STATE.R(), offsetof(FEXCore::Core::CpuStateFrame, State.pf_raw));
    str(REG_AF.X(), STATE.R(), offsetof(FEXCore::Core::CpuStateFrame, State.af_raw));
  }

  if (FPRs) {
    if (EmitterCTX->HostFeatures.SupportsAVX && EmitterCTX->HostFeatures.SupportsSVE256) {
      for (size_t i = 0; i < StaticFPRegisters.size(); i++) {
        const auto Reg = StaticFPRegisters[i];

        if (((1U << Reg.Idx()) & FPRSpillMask) != 0) {
          mov(ARMEmitter::Size::i64Bit, TMP4.R(), offsetof(Core::CpuStateFrame, State.xmm.avx.data[i][0]));
          st1b<ARMEmitter::SubRegSize::i8Bit>(Reg.Z(), PRED_TMP_32B, STATE.R(), TMP4.R());
        }
      }
    } else {
      if (GPRSpillMask && FPRSpillMask == ~0U) {
        // Optimize the common case where we can spill four registers per instruction
        // Load the sse offset in to the temporary register
        add(ARMEmitter::Size::i64Bit, TmpReg, STATE.R(), offsetof(FEXCore::Core::CpuStateFrame, State.xmm.sse.data[0][0]));
        for (size_t i = 0; i < StaticFPRegisters.size(); i += 4) {
          const auto Reg1 = StaticFPRegisters[i];
          const auto Reg2 = StaticFPRegisters[i + 1];
          const auto Reg3 = StaticFPRegisters[i + 2];
          const auto Reg4 = StaticFPRegisters[i + 3];
          st1<ARMEmitter::SubRegSize::i64Bit>(Reg1.Q(), Reg2.Q(), Reg3.Q(), Reg4.Q(), TmpReg, 64);
        }
      } else {
        for (size_t i = 0; i < StaticFPRegisters.size(); i += 2) {
          const auto Reg1 = StaticFPRegisters[i];
          const auto Reg2 = StaticFPRegisters[i + 1];

          if (((1U << Reg1.Idx()) & FPRSpillMask) && ((1U << Reg2.Idx()) & FPRSpillMask)) {
            stp<ARMEmitter::IndexType::OFFSET>(Reg1.Q(), Reg2.Q(), STATE.R(), offsetof(FEXCore::Core::CpuStateFrame, State.xmm.sse.data[i][0]));
          } else if (((1U << Reg1.Idx()) & FPRSpillMask)) {
            str(Reg1.Q(), STATE.R(), offsetof(FEXCore::Core::CpuStateFrame, State.xmm.sse.data[i][0]));
          } else if (((1U << Reg2.Idx()) & FPRSpillMask)) {
            str(Reg2.Q(), STATE.R(), offsetof(FEXCore::Core::CpuStateFrame, State.xmm.sse.data[i + 1][0]));
          }
        }
      }
    }
  }
}

void Arm64Emitter::FillStaticRegs(bool FPRs, uint32_t GPRFillMask, uint32_t FPRFillMask, std::optional<ARMEmitter::Register> OptionalReg,
                                  std::optional<ARMEmitter::Register> OptionalReg2) {
  auto FindTempReg = [this](uint32_t* GPRFillMask) -> std::optional<ARMEmitter::Register> {
    for (auto Reg : StaticRegisters) {
      if (((1U << Reg.Idx()) & *GPRFillMask)) {
        *GPRFillMask &= ~(1U << Reg.Idx());
        return std::make_optional(Reg);
      }
    }
    return std::nullopt;
  };

  LOGMAN_THROW_A_FMT(GPRFillMask != 0, "Must fill at least 2 GPRs for a temp");
  uint32_t TempGPRFillMask = GPRFillMask;
  if (!OptionalReg.has_value()) {
    OptionalReg = FindTempReg(&TempGPRFillMask);
  }

  if (!OptionalReg2.has_value()) {
    OptionalReg2 = FindTempReg(&TempGPRFillMask);
  }
  LOGMAN_THROW_A_FMT(OptionalReg.has_value() && OptionalReg2.has_value(), "Didn't have an SRA register to use as a temporary while "
                                                                          "spilling!");

  auto TmpReg = *OptionalReg;
  auto TmpReg2 = *OptionalReg2;

#ifdef _M_ARM_64EC
  // Load STATE in from the CPU area as x28 is not callee saved in the ARM64EC ABI.
  ldr(TmpReg.X(), ARMEmitter::Reg::r18, TEB_CPU_AREA_OFFSET);
  ldr(STATE, TmpReg, CPU_AREA_EMULATOR_DATA_OFFSET);
#endif

  // Regardless of what GPRs/FPRs we're filling, we need to fill NZCV since it
  // is always static and was almost certainly clobbered.
  //
  // TODO: Can we prove that NZCV is not used across a call in some cases and
  // omit this? Might help x87 perf? Future idea.
  ldr(TmpReg.W(), STATE.R(), offsetof(FEXCore::Core::CpuStateFrame, State.flags[24]));
  msr(ARMEmitter::SystemRegister::NZCV, TmpReg);

  FillSpecialRegs(TmpReg, TmpReg2, true, FPRs);

  if (FPRs) {
    if (EmitterCTX->HostFeatures.SupportsAVX && EmitterCTX->HostFeatures.SupportsSVE256) {
      for (size_t i = 0; i < StaticFPRegisters.size(); i++) {
        const auto Reg = StaticFPRegisters[i];
        if (((1U << Reg.Idx()) & FPRFillMask) != 0) {
          mov(ARMEmitter::Size::i64Bit, TMP4.R(), offsetof(Core::CpuStateFrame, State.xmm.avx.data[i][0]));
          ld1b<ARMEmitter::SubRegSize::i8Bit>(Reg.Z(), PRED_TMP_32B.Zeroing(), STATE.R(), TMP4.R());
        }
      }
    } else {
      if (GPRFillMask && FPRFillMask == ~0U) {
        // Optimize the common case where we can fill four registers per instruction.
        // Use one of the filling static registers before we fill it.
        // Load the sse offset in to the temporary register
        add(ARMEmitter::Size::i64Bit, TmpReg, STATE.R(), offsetof(FEXCore::Core::CpuStateFrame, State.xmm.sse.data[0][0]));
        for (size_t i = 0; i < StaticFPRegisters.size(); i += 4) {
          const auto Reg1 = StaticFPRegisters[i];
          const auto Reg2 = StaticFPRegisters[i + 1];
          const auto Reg3 = StaticFPRegisters[i + 2];
          const auto Reg4 = StaticFPRegisters[i + 3];
          ld1<ARMEmitter::SubRegSize::i64Bit>(Reg1.Q(), Reg2.Q(), Reg3.Q(), Reg4.Q(), TmpReg, 64);
        }
      } else {
        for (size_t i = 0; i < StaticFPRegisters.size(); i += 2) {
          const auto Reg1 = StaticFPRegisters[i];
          const auto Reg2 = StaticFPRegisters[i + 1];

          if (((1U << Reg1.Idx()) & FPRFillMask) && ((1U << Reg2.Idx()) & FPRFillMask)) {
            ldp<ARMEmitter::IndexType::OFFSET>(Reg1.Q(), Reg2.Q(), STATE.R(), offsetof(FEXCore::Core::CpuStateFrame, State.xmm.sse.data[i][0]));
          } else if (((1U << Reg1.Idx()) & FPRFillMask)) {
            ldr(Reg1.Q(), STATE.R(), offsetof(FEXCore::Core::CpuStateFrame, State.xmm.sse.data[i][0]));
          } else if (((1U << Reg2.Idx()) & FPRFillMask)) {
            ldr(Reg2.Q(), STATE.R(), offsetof(FEXCore::Core::CpuStateFrame, State.xmm.sse.data[i + 1][0]));
          }
        }
      }
    }
  }

  // PF/AF are special, remove them from the mask
  uint32_t PFAFMask = ((1u << REG_PF.Idx()) | ((1u << REG_AF.Idx())));
  uint32_t PFAFFillMask = GPRFillMask & PFAFMask;
  GPRFillMask &= ~PFAFMask;

  for (size_t i = 0; i < StaticRegisters.size(); i += 2) {
    auto Reg1 = StaticRegisters[i];
    auto Reg2 = StaticRegisters[i + 1];
    if (((1U << Reg1.Idx()) & GPRFillMask) && ((1U << Reg2.Idx()) & GPRFillMask)) {
      ldp<ARMEmitter::IndexType::OFFSET>(Reg1.X(), Reg2.X(), STATE.R(), offsetof(FEXCore::Core::CpuStateFrame, State.gregs[i]));
    } else if ((1U << Reg1.Idx()) & GPRFillMask) {
      ldr(Reg1.X(), STATE.R(), offsetof(FEXCore::Core::CpuStateFrame, State.gregs[i]));
    } else if ((1U << Reg2.Idx()) & GPRFillMask) {
      ldr(Reg2.X(), STATE.R(), offsetof(FEXCore::Core::CpuStateFrame, State.gregs[i + 1]));
    }
  }

  // Now handle PF/AF
  if (PFAFFillMask) {
    LOGMAN_THROW_A_FMT(PFAFFillMask == PFAFMask, "PF/AF not filled together");

    ldr(REG_PF.X(), STATE.R(), offsetof(FEXCore::Core::CpuStateFrame, State.pf_raw));
    ldr(REG_AF.X(), STATE.R(), offsetof(FEXCore::Core::CpuStateFrame, State.af_raw));
  }
}

void Arm64Emitter::PushVectorRegisters(ARMEmitter::Register TmpReg, bool SVE256Regs, std::span<const ARMEmitter::VRegister> VRegs) {
  if (SVE256Regs) {
    size_t i = 0;

    for (; i < (VRegs.size() % 4); i += 2) {
      const auto Reg1 = VRegs[i];
      const auto Reg2 = VRegs[i + 1];
      st2b(Reg1.Z(), Reg2.Z(), PRED_TMP_32B, TmpReg, 0);
      add(ARMEmitter::Size::i64Bit, TmpReg, TmpReg, 32 * 2);
    }

    for (; i < VRegs.size(); i += 4) {
      const auto Reg1 = VRegs[i];
      const auto Reg2 = VRegs[i + 1];
      const auto Reg3 = VRegs[i + 2];
      const auto Reg4 = VRegs[i + 3];
      st4b(Reg1.Z(), Reg2.Z(), Reg3.Z(), Reg4.Z(), PRED_TMP_32B, TmpReg, 0);
      add(ARMEmitter::Size::i64Bit, TmpReg, TmpReg, 32 * 4);
    }
  } else {
    size_t i = 0;
    for (; i < (VRegs.size() % 4); i += 2) {
      const auto Reg1 = VRegs[i];
      const auto Reg2 = VRegs[i + 1];
      st1<ARMEmitter::SubRegSize::i64Bit>(Reg1.Q(), Reg2.Q(), TmpReg, 32);
    }

    for (; i < VRegs.size(); i += 4) {
      const auto Reg1 = VRegs[i];
      const auto Reg2 = VRegs[i + 1];
      const auto Reg3 = VRegs[i + 2];
      const auto Reg4 = VRegs[i + 3];
      st1<ARMEmitter::SubRegSize::i64Bit>(Reg1.Q(), Reg2.Q(), Reg3.Q(), Reg4.Q(), TmpReg, 64);
    }
  }
}

void Arm64Emitter::PushGeneralRegisters(ARMEmitter::Register TmpReg, std::span<const ARMEmitter::Register> Regs) {
  size_t i = 0;
  for (; i < (Regs.size() % 2); ++i) {
    const auto Reg1 = Regs[i];
    str<ARMEmitter::IndexType::POST>(Reg1.X(), TmpReg, 16);
  }

  for (; i < Regs.size(); i += 2) {
    const auto Reg1 = Regs[i];
    const auto Reg2 = Regs[i + 1];
    stp<ARMEmitter::IndexType::POST>(Reg1.X(), Reg2.X(), TmpReg, 16);
  }
}

void Arm64Emitter::PopVectorRegisters(bool SVE256Regs, std::span<const ARMEmitter::VRegister> VRegs) {
  if (SVE256Regs) {
    size_t i = 0;
    for (; i < (VRegs.size() % 4); i += 2) {
      const auto Reg1 = VRegs[i];
      const auto Reg2 = VRegs[i + 1];
      ld2b(Reg1.Z(), Reg2.Z(), PRED_TMP_32B.Zeroing(), ARMEmitter::Reg::rsp);
      add(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::rsp, ARMEmitter::Reg::rsp, 32 * 2);
    }

    for (; i < VRegs.size(); i += 4) {
      const auto Reg1 = VRegs[i];
      const auto Reg2 = VRegs[i + 1];
      const auto Reg3 = VRegs[i + 2];
      const auto Reg4 = VRegs[i + 3];
      ld4b(Reg1.Z(), Reg2.Z(), Reg3.Z(), Reg4.Z(), PRED_TMP_32B.Zeroing(), ARMEmitter::Reg::rsp);
      add(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::rsp, ARMEmitter::Reg::rsp, 32 * 4);
    }
  } else {
    size_t i = 0;
    for (; i < (VRegs.size() % 4); i += 2) {
      const auto Reg1 = VRegs[i];
      const auto Reg2 = VRegs[i + 1];
      ld1<ARMEmitter::SubRegSize::i64Bit>(Reg1.Q(), Reg2.Q(), ARMEmitter::Reg::rsp, 32);
    }

    for (; i < VRegs.size(); i += 4) {
      const auto Reg1 = VRegs[i];
      const auto Reg2 = VRegs[i + 1];
      const auto Reg3 = VRegs[i + 2];
      const auto Reg4 = VRegs[i + 3];
      ld1<ARMEmitter::SubRegSize::i64Bit>(Reg1.Q(), Reg2.Q(), Reg3.Q(), Reg4.Q(), ARMEmitter::Reg::rsp, 64);
    }
  }
}

void Arm64Emitter::PopGeneralRegisters(std::span<const ARMEmitter::Register> Regs) {
  size_t i = 0;
  for (; i < (Regs.size() % 2); ++i) {
    const auto Reg1 = Regs[i];
    ldr<ARMEmitter::IndexType::POST>(Reg1.X(), ARMEmitter::Reg::rsp, 16);
  }
  for (; i < Regs.size(); i += 2) {
    const auto Reg1 = Regs[i];
    const auto Reg2 = Regs[i + 1];
    ldp<ARMEmitter::IndexType::POST>(Reg1.X(), Reg2.X(), ARMEmitter::Reg::rsp, 16);
  }
}

void Arm64Emitter::PushDynamicRegsAndLR(ARMEmitter::Register TmpReg) {
  const auto CanUseSVE256 = EmitterCTX->HostFeatures.SupportsSVE256;
  const auto GPRSize = (ConfiguredDynamicRegisterBase.size() + 1) * Core::CPUState::GPR_REG_SIZE;
  const auto FPRRegSize = CanUseSVE256 ? 32 : 16;
  const auto FPRSize = GeneralFPRegisters.size() * FPRRegSize;
  const uint64_t SPOffset = AlignUp(GPRSize + FPRSize, 16);

  sub(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::rsp, ARMEmitter::Reg::rsp, SPOffset);

  // rsp capable move
  add(ARMEmitter::Size::i64Bit, TmpReg, ARMEmitter::Reg::rsp, 0);

  LOGMAN_THROW_A_FMT(GeneralFPRegisters.size() % 2 == 0, "Needs to have multiple of 2 FPRs for RA");

  // Push the vector registers
  PushVectorRegisters(TmpReg, CanUseSVE256, GeneralFPRegisters);

  // Push the general registers.
  PushGeneralRegisters(TmpReg, ConfiguredDynamicRegisterBase);

#ifndef _M_ARM_64EC
  str(ARMEmitter::XReg::lr, TmpReg, 0);
#endif
}

void Arm64Emitter::PopDynamicRegsAndLR() {
  const auto CanUseSVE256 = EmitterCTX->HostFeatures.SupportsSVE256;

  // Pop vectors first
  PopVectorRegisters(CanUseSVE256, GeneralFPRegisters);

  // Pop GPRs second
  PopGeneralRegisters(ConfiguredDynamicRegisterBase);

#ifndef _M_ARM_64EC
  ldr<ARMEmitter::IndexType::POST>(ARMEmitter::XReg::lr, ARMEmitter::Reg::rsp, 16);
#endif
}

void Arm64Emitter::SpillForPreserveAllABICall(ARMEmitter::Register TmpReg, bool FPRs) {
  const auto CanUseSVE256 = EmitterCTX->HostFeatures.SupportsSVE256;
  const auto FPRRegSize = CanUseSVE256 ? 32 : 16;

  std::span<const ARMEmitter::Register> DynamicGPRs {};
  std::span<const ARMEmitter::VRegister> DynamicFPRs {};
  uint32_t PreserveSRAMask {};
  uint32_t PreserveSRAFPRMask {};
  if (EmitterCTX->Config.Is64BitMode()) {
    DynamicGPRs = x64::PreserveAll_Dynamic;
    DynamicFPRs = x64::PreserveAll_DynamicFPR;
    PreserveSRAMask = x64::PreserveAll_SRAMask;
    PreserveSRAFPRMask = x64::PreserveAll_SRAFPRMask;

    if (CanUseSVE256) {
      DynamicFPRs = x64::PreserveAll_DynamicFPRSVE;
      PreserveSRAFPRMask = x64::PreserveAll_SRAFPRSVEMask;
    }
  } else {
    DynamicGPRs = x32::PreserveAll_Dynamic;
    DynamicFPRs = x32::PreserveAll_DynamicFPR;
    PreserveSRAMask = x32::PreserveAll_SRAMask;
    PreserveSRAFPRMask = x32::PreserveAll_SRAFPRMask;

    if (CanUseSVE256) {
      DynamicFPRs = x32::PreserveAll_DynamicFPRSVE;
      PreserveSRAFPRMask = x32::PreserveAll_SRAFPRSVEMask;
    }
  }

  const auto GPRSize = AlignUp(DynamicGPRs.size(), 2) * Core::CPUState::GPR_REG_SIZE;
  const auto FPRSize = DynamicFPRs.size() * FPRRegSize;
  const uint64_t SPOffset = AlignUp(GPRSize + FPRSize, 16);

  // Spill the static registers.
  SpillStaticRegs(TmpReg, true, PreserveSRAMask, PreserveSRAFPRMask);

  sub(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::rsp, ARMEmitter::Reg::rsp, SPOffset);

  // rsp capable move
  add(ARMEmitter::Size::i64Bit, TmpReg, ARMEmitter::Reg::rsp, 0);

  // Push the vector registers.
  PushVectorRegisters(TmpReg, CanUseSVE256, DynamicFPRs);

  // Push the general registers.
  PushGeneralRegisters(TmpReg, DynamicGPRs);
}

void Arm64Emitter::FillForPreserveAllABICall(bool FPRs) {
  const auto CanUseSVE256 = EmitterCTX->HostFeatures.SupportsSVE256;

  std::span<const ARMEmitter::Register> DynamicGPRs {};
  std::span<const ARMEmitter::VRegister> DynamicFPRs {};
  uint32_t PreserveSRAMask {};
  uint32_t PreserveSRAFPRMask {};

  if (EmitterCTX->Config.Is64BitMode()) {
    DynamicGPRs = x64::PreserveAll_Dynamic;
    DynamicFPRs = x64::PreserveAll_DynamicFPR;
    PreserveSRAMask = x64::PreserveAll_SRAMask;
    PreserveSRAFPRMask = x64::PreserveAll_SRAFPRMask;

    if (CanUseSVE256) {
      DynamicFPRs = x64::PreserveAll_DynamicFPRSVE;
      PreserveSRAFPRMask = x64::PreserveAll_SRAFPRSVEMask;
    }
  } else {
    DynamicGPRs = x32::PreserveAll_Dynamic;
    DynamicFPRs = x32::PreserveAll_DynamicFPR;
    PreserveSRAMask = x32::PreserveAll_SRAMask;
    PreserveSRAFPRMask = x32::PreserveAll_SRAFPRMask;

    if (CanUseSVE256) {
      DynamicFPRs = x32::PreserveAll_DynamicFPRSVE;
      PreserveSRAFPRMask = x32::PreserveAll_SRAFPRSVEMask;
    }
  }

  // Fill the static registers.
  FillStaticRegs(true, PreserveSRAMask, PreserveSRAFPRMask);

  // Pop the vector registers.
  PopVectorRegisters(CanUseSVE256, DynamicFPRs);

  // Pop the general registers.
  PopGeneralRegisters(DynamicGPRs);
}

void Arm64Emitter::Align16B() {
  uint64_t CurrentOffset = GetCursorAddress<uint64_t>();
  for (uint64_t i = (16 - (CurrentOffset & 0xF)); i != 0; i -= 4) {
    nop();
  }
}

} // namespace FEXCore::CPU
