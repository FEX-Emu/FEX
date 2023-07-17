#include "Interface/Core/ArchHelpers/Arm64Emitter.h"
#include "FEXCore/Utils/AllocatorHooks.h"
#include "Interface/Core/ArchHelpers/CodeEmitter/Emitter.h"
#include "Interface/Core/Dispatcher/Dispatcher.h"
#include "Interface/Context/Context.h"
#include "Interface/HLE/Thunks/Thunks.h"

#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Utils/BitUtils.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/MathUtils.h>

#include <aarch64/cpu-aarch64.h>
#include <aarch64/instructions-aarch64.h>
#include <cpu-features.h>
#include <utils-vixl.h>

#include <array>
#include <tuple>
#include <utility>

namespace FEXCore::CPU {
// Register x18 is unused in the current configuration.
// This is due to it being a platform register on wine platforms.
// TODO: Allow x18 register allocation in the future to gain one more register.

namespace x64 {
  // All but x19 and x29 are caller saved
  constexpr std::array<FEXCore::ARMEmitter::Register, 16> SRA = {
    FEXCore::ARMEmitter::Reg::r4, FEXCore::ARMEmitter::Reg::r5,
    FEXCore::ARMEmitter::Reg::r6, FEXCore::ARMEmitter::Reg::r7,
    FEXCore::ARMEmitter::Reg::r8, FEXCore::ARMEmitter::Reg::r9,
    FEXCore::ARMEmitter::Reg::r10, FEXCore::ARMEmitter::Reg::r11,
    FEXCore::ARMEmitter::Reg::r12, FEXCore::ARMEmitter::Reg::r13,
    FEXCore::ARMEmitter::Reg::r14, FEXCore::ARMEmitter::Reg::r15,
    FEXCore::ARMEmitter::Reg::r16, FEXCore::ARMEmitter::Reg::r17,
    FEXCore::ARMEmitter::Reg::r19, FEXCore::ARMEmitter::Reg::r29
  };

  constexpr std::array<FEXCore::ARMEmitter::Register, 9> RA = {
    // All these callee saved
    FEXCore::ARMEmitter::Reg::r20, FEXCore::ARMEmitter::Reg::r21,
    FEXCore::ARMEmitter::Reg::r22, FEXCore::ARMEmitter::Reg::r23,
    FEXCore::ARMEmitter::Reg::r24, FEXCore::ARMEmitter::Reg::r25,
    FEXCore::ARMEmitter::Reg::r26, FEXCore::ARMEmitter::Reg::r27,
    FEXCore::ARMEmitter::Reg::r30,
  };

  constexpr std::array<std::pair<FEXCore::ARMEmitter::Register, FEXCore::ARMEmitter::Register>, 4> RAPair = {{
    {FEXCore::ARMEmitter::Reg::r20, FEXCore::ARMEmitter::Reg::r21},
    {FEXCore::ARMEmitter::Reg::r22, FEXCore::ARMEmitter::Reg::r23},
    {FEXCore::ARMEmitter::Reg::r24, FEXCore::ARMEmitter::Reg::r25},
    {FEXCore::ARMEmitter::Reg::r26, FEXCore::ARMEmitter::Reg::r27},
  }};

  // All are caller saved
  constexpr std::array<FEXCore::ARMEmitter::VRegister, 16> SRAFPR = {
    FEXCore::ARMEmitter::VReg::v16, FEXCore::ARMEmitter::VReg::v17,
    FEXCore::ARMEmitter::VReg::v18, FEXCore::ARMEmitter::VReg::v19,
    FEXCore::ARMEmitter::VReg::v20, FEXCore::ARMEmitter::VReg::v21,
    FEXCore::ARMEmitter::VReg::v22, FEXCore::ARMEmitter::VReg::v23,
    FEXCore::ARMEmitter::VReg::v24, FEXCore::ARMEmitter::VReg::v25,
    FEXCore::ARMEmitter::VReg::v26, FEXCore::ARMEmitter::VReg::v27,
    FEXCore::ARMEmitter::VReg::v28, FEXCore::ARMEmitter::VReg::v29,
    FEXCore::ARMEmitter::VReg::v30, FEXCore::ARMEmitter::VReg::v31
  };

  //  v8..v15 = (lower 64bits) Callee saved
  constexpr std::array<FEXCore::ARMEmitter::VRegister, 12> RAFPR = {
    // v0 ~ v3 are used as temps.
    // FEXCore::ARMEmitter::VReg::v0, FEXCore::ARMEmitter::VReg::v1,
    // FEXCore::ARMEmitter::VReg::v2, FEXCore::ARMEmitter::VReg::v3,

    FEXCore::ARMEmitter::VReg::v4, FEXCore::ARMEmitter::VReg::v5,
    FEXCore::ARMEmitter::VReg::v6, FEXCore::ARMEmitter::VReg::v7,
    FEXCore::ARMEmitter::VReg::v8, FEXCore::ARMEmitter::VReg::v9,
    FEXCore::ARMEmitter::VReg::v10, FEXCore::ARMEmitter::VReg::v11,
    FEXCore::ARMEmitter::VReg::v12, FEXCore::ARMEmitter::VReg::v13,
    FEXCore::ARMEmitter::VReg::v14, FEXCore::ARMEmitter::VReg::v15,
  };
}

namespace x32 {
  // All but x19 and x29 are caller saved
  constexpr std::array<FEXCore::ARMEmitter::Register, 8> SRA = {
    FEXCore::ARMEmitter::Reg::r4, FEXCore::ARMEmitter::Reg::r5,
    FEXCore::ARMEmitter::Reg::r6, FEXCore::ARMEmitter::Reg::r7,
    FEXCore::ARMEmitter::Reg::r8, FEXCore::ARMEmitter::Reg::r9,
    FEXCore::ARMEmitter::Reg::r10, FEXCore::ARMEmitter::Reg::r11,
  };

  constexpr std::array<FEXCore::ARMEmitter::Register, 17> RA = {
    // All these callee saved
    FEXCore::ARMEmitter::Reg::r20, FEXCore::ARMEmitter::Reg::r21,
    FEXCore::ARMEmitter::Reg::r22, FEXCore::ARMEmitter::Reg::r23,
    FEXCore::ARMEmitter::Reg::r24, FEXCore::ARMEmitter::Reg::r25,
    FEXCore::ARMEmitter::Reg::r26, FEXCore::ARMEmitter::Reg::r27,

    // Registers only available on 32-bit
    // All these are caller saved (except for r19).
    FEXCore::ARMEmitter::Reg::r12, FEXCore::ARMEmitter::Reg::r13,
    FEXCore::ARMEmitter::Reg::r14, FEXCore::ARMEmitter::Reg::r15,
    FEXCore::ARMEmitter::Reg::r16, FEXCore::ARMEmitter::Reg::r17,
    FEXCore::ARMEmitter::Reg::r29, FEXCore::ARMEmitter::Reg::r30,

    FEXCore::ARMEmitter::Reg::r19,
  };

  constexpr std::array<std::pair<FEXCore::ARMEmitter::Register, FEXCore::ARMEmitter::Register>, 8> RAPair = {{
    {FEXCore::ARMEmitter::Reg::r20, FEXCore::ARMEmitter::Reg::r21},
    {FEXCore::ARMEmitter::Reg::r22, FEXCore::ARMEmitter::Reg::r23},
    {FEXCore::ARMEmitter::Reg::r24, FEXCore::ARMEmitter::Reg::r25},
    {FEXCore::ARMEmitter::Reg::r26, FEXCore::ARMEmitter::Reg::r27},

    {FEXCore::ARMEmitter::Reg::r12, FEXCore::ARMEmitter::Reg::r13},
    {FEXCore::ARMEmitter::Reg::r14, FEXCore::ARMEmitter::Reg::r15},
    {FEXCore::ARMEmitter::Reg::r16, FEXCore::ARMEmitter::Reg::r17},
    {FEXCore::ARMEmitter::Reg::r29, FEXCore::ARMEmitter::Reg::r30},
  }};

  // All are caller saved
  constexpr std::array<FEXCore::ARMEmitter::VRegister, 8> SRAFPR = {
    FEXCore::ARMEmitter::VReg::v16, FEXCore::ARMEmitter::VReg::v17,
    FEXCore::ARMEmitter::VReg::v18, FEXCore::ARMEmitter::VReg::v19,
    FEXCore::ARMEmitter::VReg::v20, FEXCore::ARMEmitter::VReg::v21,
    FEXCore::ARMEmitter::VReg::v22, FEXCore::ARMEmitter::VReg::v23,
  };

  //  v8..v15 = (lower 64bits) Callee saved
  constexpr std::array<FEXCore::ARMEmitter::VRegister, 20> RAFPR = {
    // v0 ~ v3 are used as temps.
    // FEXCore::ARMEmitter::VReg::v0, FEXCore::ARMEmitter::VReg::v1,
    // FEXCore::ARMEmitter::VReg::v2, FEXCore::ARMEmitter::VReg::v3,

    FEXCore::ARMEmitter::VReg::v4, FEXCore::ARMEmitter::VReg::v5,
    FEXCore::ARMEmitter::VReg::v6, FEXCore::ARMEmitter::VReg::v7,
    FEXCore::ARMEmitter::VReg::v8, FEXCore::ARMEmitter::VReg::v9,
    FEXCore::ARMEmitter::VReg::v10, FEXCore::ARMEmitter::VReg::v11,
    FEXCore::ARMEmitter::VReg::v12, FEXCore::ARMEmitter::VReg::v13,
    FEXCore::ARMEmitter::VReg::v14, FEXCore::ARMEmitter::VReg::v15,

    FEXCore::ARMEmitter::VReg::v24, FEXCore::ARMEmitter::VReg::v25,
    FEXCore::ARMEmitter::VReg::v26, FEXCore::ARMEmitter::VReg::v27,
    FEXCore::ARMEmitter::VReg::v28, FEXCore::ARMEmitter::VReg::v29,
    FEXCore::ARMEmitter::VReg::v30, FEXCore::ARMEmitter::VReg::v31
  };
}

// We want vixl to not allocate a default buffer. Jit and dispatcher will manually create one.
Arm64Emitter::Arm64Emitter(FEXCore::Context::ContextImpl *ctx, size_t size)
  : Emitter(size ? (uint8_t*)FEXCore::Allocator::VirtualAlloc(size, true) : nullptr, size)
  , EmitterCTX {ctx} {
  CPU.SetUp();

  // Number of register available is dependent on what operating mode the proccess is in.
  if (EmitterCTX->Config.Is64BitMode()) {
    StaticRegisters = x64::SRA;
    GeneralRegisters = x64::RA;
    GeneralPairRegisters = x64::RAPair;
    StaticFPRegisters = x64::SRAFPR;
    GeneralFPRegisters = x64::RAFPR;
  }
  else {
    ConfiguredDynamicRegisterBase = std::span(x32::RA.begin() + 8, 8);

    StaticRegisters = x32::SRA;
    GeneralRegisters = x32::RA;
    GeneralPairRegisters = x32::RAPair;

    StaticFPRegisters = x32::SRAFPR;
    GeneralFPRegisters = x32::RAFPR;
  }
}

Arm64Emitter::~Arm64Emitter() {
  auto BufferSize = GetBufferSize();
  if (BufferSize) {
    FEXCore::Allocator::VirtualFree(GetBufferBase(), BufferSize);
  }
}

void Arm64Emitter::LoadConstant(ARMEmitter::Size s, ARMEmitter::Register Reg, uint64_t Constant, bool NOPPad) {
  bool Is64Bit = s == ARMEmitter::Size::i64Bit;
  int Segments = Is64Bit ? 4 : 2;

  if (Is64Bit && ((~Constant)>> 16) == 0) {
    movn(s, Reg, (~Constant) & 0xFFFF);

    if (NOPPad) {
      nop(); nop(); nop();
    }
    return;
  }

  if ((Constant >> 32) == 0) {
    // If the upper 32-bits is all zero, we can now switch to a 32-bit move.
    s = ARMEmitter::Size::i32Bit;
    Is64Bit = false;
    Segments = 2;
  }

  // If this can be loaded with a mov bitmask.
  const auto IsImm = vixl::aarch64::Assembler::IsImmLogical(Constant, RegSizeInBits(s));
  if (IsImm) {
    orr(s, Reg, ARMEmitter::Reg::zr, Constant);
    if (NOPPad) {
      nop(); nop(); nop();
    }
    return;
  }

  int NumMoves = 1;
  int RequiredMoveSegments{};

  // Count the number of move segments
  // We only want to use ADRP+ADD if we have more than 1 segment
  for (size_t i = 0; i < Segments; ++i) {
    uint16_t Part = (Constant >> (i * 16)) & 0xFFFF;
    if (Part != 0) {
      ++RequiredMoveSegments;
    }
  }

  // ADRP+ADD is specifically optimized in hardware
  // Check if we can use this
  auto PC = GetCursorAddress<uint64_t>();

  // PC aligned to page
  uint64_t AlignedPC = PC & ~0xFFFULL;

  // Offset from aligned PC
  int64_t AlignedOffset = static_cast<int64_t>(Constant) - static_cast<int64_t>(AlignedPC);

  // If the aligned offset is within the 4GB window then we can use ADRP+ADD
  // and the number of move segments more than 1
  if (RequiredMoveSegments > 1 && vixl::IsInt32(AlignedOffset)) {
    // If this is 4k page aligned then we only need ADRP
    if ((AlignedOffset & 0xFFF) == 0) {
      adrp(Reg, AlignedOffset >> 12);
    }
    else {
      // If the constant is within 1MB of PC then we can still use ADR to load in a single instruction
      // 21-bit signed integer here
      int64_t SmallOffset = static_cast<int64_t>(Constant) - static_cast<int64_t>(PC);
      if (vixl::IsInt21(SmallOffset)) {
        adr(Reg, SmallOffset);
      }
      else {
        // Need to use ADRP + ADD
        adrp(Reg, AlignedOffset >> 12);
        add(s, Reg, Reg, Constant & 0xFFF);
        NumMoves = 2;
      }
    }
  }
  else {
    movz(s, Reg, (Constant) & 0xFFFF, 0);
    for (int i = 1; i < Segments; ++i) {
      uint16_t Part = (Constant >> (i * 16)) & 0xFFFF;
      if (Part) {
        movk(s, Reg, Part, i * 16);
        ++NumMoves;
      }
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
#ifdef _WIN32
    // Platform register, Just save it twice to make logic easy.
    {ARMEmitter::XReg::x18, ARMEmitter::XReg::x18},
#endif
    {ARMEmitter::XReg::x19, ARMEmitter::XReg::x20},
    {ARMEmitter::XReg::x21, ARMEmitter::XReg::x22},
    {ARMEmitter::XReg::x23, ARMEmitter::XReg::x24},
    {ARMEmitter::XReg::x25, ARMEmitter::XReg::x26},
    {ARMEmitter::XReg::x27, ARMEmitter::XReg::x28},
    {ARMEmitter::XReg::x29, ARMEmitter::XReg::x30},
  }};

  for (auto &RegPair : CalleeSaved) {
    stp<ARMEmitter::IndexType::PRE>(RegPair.first, RegPair.second, ARMEmitter::Reg::rsp, -16);
  }

  // Additionally we need to store the lower 64bits of v8-v15
  // Here's a fun thing, we can use two ST4 instructions to store everything
  // We just need a single sub to sp before that
  const std::array<
    std::tuple<ARMEmitter::DRegister,
               ARMEmitter::DRegister,
               ARMEmitter::DRegister,
               ARMEmitter::DRegister>, 2> FPRs = {{
    {ARMEmitter::DReg::d8, ARMEmitter::DReg::d9, ARMEmitter::DReg::d10, ARMEmitter::DReg::d11},
    {ARMEmitter::DReg::d12, ARMEmitter::DReg::d13, ARMEmitter::DReg::d14, ARMEmitter::DReg::d15},
  }};

  uint32_t VectorSaveSize = sizeof(uint64_t) * 8;
  sub(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::rsp, ARMEmitter::Reg::rsp, VectorSaveSize);
  // SP supporting move
  // We just saved x19 so it is safe
  add(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::r19, ARMEmitter::Reg::rsp, 0);

  for (auto &RegQuad : FPRs) {
    st4(ARMEmitter::SubRegSize::i64Bit,
        std::get<0>(RegQuad),
        std::get<1>(RegQuad),
        std::get<2>(RegQuad),
        std::get<3>(RegQuad),
        0,
        ARMEmitter::Reg::r19,
        32);
  }
}

void Arm64Emitter::PopCalleeSavedRegisters() {
  const std::array<
    std::tuple<ARMEmitter::DRegister,
               ARMEmitter::DRegister,
               ARMEmitter::DRegister,
               ARMEmitter::DRegister>, 2> FPRs = {{
    {ARMEmitter::DReg::d12, ARMEmitter::DReg::d13, ARMEmitter::DReg::d14, ARMEmitter::DReg::d15},
    {ARMEmitter::DReg::d8, ARMEmitter::DReg::d9, ARMEmitter::DReg::d10, ARMEmitter::DReg::d11},
  }};

  for (auto &RegQuad : FPRs) {
    ld4(ARMEmitter::SubRegSize::i64Bit,
        std::get<0>(RegQuad),
        std::get<1>(RegQuad),
        std::get<2>(RegQuad),
        std::get<3>(RegQuad),
        0,
        ARMEmitter::Reg::rsp,
        32);
  }

  const fextl::vector<std::pair<ARMEmitter::XRegister, ARMEmitter::XRegister>> CalleeSaved = {{
    {ARMEmitter::XReg::x29, ARMEmitter::XReg::x30},
    {ARMEmitter::XReg::x27, ARMEmitter::XReg::x28},
    {ARMEmitter::XReg::x25, ARMEmitter::XReg::x26},
    {ARMEmitter::XReg::x23, ARMEmitter::XReg::x24},
    {ARMEmitter::XReg::x21, ARMEmitter::XReg::x22},
    {ARMEmitter::XReg::x19, ARMEmitter::XReg::x20},
#ifdef _WIN32
    // Platform register.
    {ARMEmitter::XReg::x18, ARMEmitter::XReg::zr},
#endif
  }};

  for (auto &RegPair : CalleeSaved) {
    ldp<ARMEmitter::IndexType::POST>(RegPair.first, RegPair.second, ARMEmitter::Reg::rsp, 16);
  }
}

void Arm64Emitter::SpillStaticRegs(FEXCore::ARMEmitter::Register TmpReg, bool FPRs, uint32_t GPRSpillMask, uint32_t FPRSpillMask) {
  if (!StaticRegisterAllocation()) {
    return;
  }

  for (size_t i = 0; i < StaticRegisters.size(); i+=2) {
    auto Reg1 = StaticRegisters[i];
    auto Reg2 = StaticRegisters[i+1];
    if (((1U << Reg1.Idx()) & GPRSpillMask) &&
        ((1U << Reg2.Idx()) & GPRSpillMask)) {
      stp<ARMEmitter::IndexType::OFFSET>(Reg1.X(), Reg2.X(), STATE.R(), offsetof(FEXCore::Core::CpuStateFrame, State.gregs[i]));
    }
    else if (((1U << Reg1.Idx()) & GPRSpillMask)) {
      str(Reg1.X(), STATE.R(), offsetof(FEXCore::Core::CpuStateFrame, State.gregs[i]));
    }
    else if (((1U << Reg2.Idx()) & GPRSpillMask)) {
      str(Reg2.X(), STATE.R(), offsetof(FEXCore::Core::CpuStateFrame, State.gregs[i+1]));
    }
  }

  if (FPRs) {
    if (EmitterCTX->HostFeatures.SupportsAVX) {
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
      }
      else {
        for (size_t i = 0; i < StaticFPRegisters.size(); i += 2) {
          const auto Reg1 = StaticFPRegisters[i];
          const auto Reg2 = StaticFPRegisters[i + 1];

          if (((1U << Reg1.Idx()) & FPRSpillMask) &&
              ((1U << Reg2.Idx()) & FPRSpillMask)) {
            stp<ARMEmitter::IndexType::OFFSET>(Reg1.Q(), Reg2.Q(), STATE.R(), offsetof(FEXCore::Core::CpuStateFrame, State.xmm.sse.data[i][0]));
          }
          else if (((1U << Reg1.Idx()) & FPRSpillMask)) {
            str(Reg1.Q(), STATE.R(), offsetof(FEXCore::Core::CpuStateFrame, State.xmm.sse.data[i][0]));
          }
          else if (((1U << Reg2.Idx()) & FPRSpillMask)) {
            str(Reg2.Q(), STATE.R(), offsetof(FEXCore::Core::CpuStateFrame, State.xmm.sse.data[i+1][0]));
          }
        }
      }
    }
  }
}

void Arm64Emitter::FillStaticRegs(bool FPRs, uint32_t GPRFillMask, uint32_t FPRFillMask) {
  if (!StaticRegisterAllocation()) {
    return;
  }

  if (FPRs) {
    if (EmitterCTX->HostFeatures.SupportsAVX) {
      // Set up predicate registers.
      // We don't bother spilling these in SpillStaticRegs,
      // since all that matters is we restore them on a fill.
      // It's not a concern if they get trounced by something else.
      ptrue<ARMEmitter::SubRegSize::i8Bit>(PRED_TMP_16B, ARMEmitter::PredicatePattern::SVE_VL16);
      ptrue<ARMEmitter::SubRegSize::i8Bit>(PRED_TMP_32B, ARMEmitter::PredicatePattern::SVE_VL32);

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
        auto TmpReg = StaticRegisters[FindFirstSetBit(GPRFillMask)];

        // Load the sse offset in to the temporary register
        add(ARMEmitter::Size::i64Bit, TmpReg, STATE.R(), offsetof(FEXCore::Core::CpuStateFrame, State.xmm.sse.data[0][0]));
        for (size_t i = 0; i < StaticFPRegisters.size(); i += 4) {
          const auto Reg1 = StaticFPRegisters[i];
          const auto Reg2 = StaticFPRegisters[i + 1];
          const auto Reg3 = StaticFPRegisters[i + 2];
          const auto Reg4 = StaticFPRegisters[i + 3];
          ld1<ARMEmitter::SubRegSize::i64Bit>(Reg1.Q(), Reg2.Q(), Reg3.Q(), Reg4.Q(), TmpReg, 64);
        }
      }
      else {
        for (size_t i = 0; i < StaticFPRegisters.size(); i += 2) {
          const auto Reg1 = StaticFPRegisters[i];
          const auto Reg2 = StaticFPRegisters[i + 1];

          if (((1U << Reg1.Idx()) & FPRFillMask) &&
              ((1U << Reg2.Idx()) & FPRFillMask)) {
            ldp<ARMEmitter::IndexType::OFFSET>(Reg1.Q(), Reg2.Q(), STATE.R(), offsetof(FEXCore::Core::CpuStateFrame, State.xmm.sse.data[i][0]));
          }
          else if (((1U << Reg1.Idx()) & FPRFillMask)) {
            ldr(Reg1.Q(), STATE.R(), offsetof(FEXCore::Core::CpuStateFrame, State.xmm.sse.data[i][0]));
          }
          else if (((1U << Reg2.Idx()) & FPRFillMask)) {
            ldr(Reg2.Q(), STATE.R(), offsetof(FEXCore::Core::CpuStateFrame, State.xmm.sse.data[i+1][0]));
          }
        }
      }
    }
  }

  for (size_t i = 0; i < StaticRegisters.size(); i+=2) {
    auto Reg1 = StaticRegisters[i];
    auto Reg2 = StaticRegisters[i+1];
    if (((1U << Reg1.Idx()) & GPRFillMask) &&
        ((1U << Reg2.Idx()) & GPRFillMask)) {
      ldp<ARMEmitter::IndexType::OFFSET>(Reg1.X(), Reg2.X(), STATE.R(), offsetof(FEXCore::Core::CpuStateFrame, State.gregs[i]));
    }
    else if ((1U << Reg1.Idx()) & GPRFillMask) {
      ldr(Reg1.X(), STATE.R(), offsetof(FEXCore::Core::CpuStateFrame, State.gregs[i]));
    }
    else if ((1U << Reg2.Idx()) & GPRFillMask) {
      ldr(Reg2.X(), STATE.R(), offsetof(FEXCore::Core::CpuStateFrame, State.gregs[i+1]));
    }
  }
}

void Arm64Emitter::PushDynamicRegsAndLR(FEXCore::ARMEmitter::Register TmpReg) {
  const auto CanUseSVE = EmitterCTX->HostFeatures.SupportsAVX;
  const auto GPRSize = (ConfiguredDynamicRegisterBase.size() + 1) * Core::CPUState::GPR_REG_SIZE;
  const auto FPRRegSize = CanUseSVE ? Core::CPUState::XMM_AVX_REG_SIZE
                                    : Core::CPUState::XMM_SSE_REG_SIZE;
  const auto FPRSize = GeneralFPRegisters.size() * FPRRegSize;
  const uint64_t SPOffset = AlignUp(GPRSize + FPRSize, 16);

  sub(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::rsp, ARMEmitter::Reg::rsp, SPOffset);

  // rsp capable move
  add(ARMEmitter::Size::i64Bit, TmpReg, ARMEmitter::Reg::rsp, 0);

  if (CanUseSVE) {
    for (size_t i = 0; i < GeneralFPRegisters.size(); i += 4) {
      const auto Reg1 = GeneralFPRegisters[i];
      const auto Reg2 = GeneralFPRegisters[i + 1];
      const auto Reg3 = GeneralFPRegisters[i + 2];
      const auto Reg4 = GeneralFPRegisters[i + 3];
      st4b(Reg1.Z(), Reg2.Z(), Reg3.Z(), Reg4.Z(), PRED_TMP_32B, TmpReg, 0);
      add(ARMEmitter::Size::i64Bit, TmpReg, TmpReg, 32 * 4);
    }
  } else {
    LOGMAN_THROW_A_FMT(GeneralFPRegisters.size() % 4 == 0, "Needs to have multiple of 4 FPRs for RA");
    for (size_t i = 0; i < GeneralFPRegisters.size(); i += 4) {
      const auto Reg1 = GeneralFPRegisters[i];
      const auto Reg2 = GeneralFPRegisters[i + 1];
      const auto Reg3 = GeneralFPRegisters[i + 2];
      const auto Reg4 = GeneralFPRegisters[i + 3];
      st1<ARMEmitter::SubRegSize::i64Bit>(Reg1.Q(), Reg2.Q(), Reg3.Q(), Reg4.Q(), TmpReg, 64);
    }
  }

  for (size_t i = 0; i < ConfiguredDynamicRegisterBase.size(); i += 2) {
    const auto Reg1 = ConfiguredDynamicRegisterBase[i];
    const auto Reg2 = ConfiguredDynamicRegisterBase[i + 1];
    stp<ARMEmitter::IndexType::POST>(Reg1.X(), Reg2.X(), TmpReg, 16);
  }

  str(ARMEmitter::XReg::lr, TmpReg, 0);
}

void Arm64Emitter::PopDynamicRegsAndLR() {
  const auto CanUseSVE = EmitterCTX->HostFeatures.SupportsAVX;

  if (CanUseSVE) {
    for (size_t i = 0; i < GeneralFPRegisters.size(); i += 4) {
      const auto Reg1 = GeneralFPRegisters[i];
      const auto Reg2 = GeneralFPRegisters[i + 1];
      const auto Reg3 = GeneralFPRegisters[i + 2];
      const auto Reg4 = GeneralFPRegisters[i + 3];
      ld4b(Reg1.Z(), Reg2.Z(), Reg3.Z(), Reg4.Z(), PRED_TMP_32B.Zeroing(), ARMEmitter::Reg::rsp);
      add(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::rsp, ARMEmitter::Reg::rsp, 32 * 4);
    }
  } else {
    for (size_t i = 0; i < GeneralFPRegisters.size(); i += 4) {
      const auto Reg1 = GeneralFPRegisters[i];
      const auto Reg2 = GeneralFPRegisters[i + 1];
      const auto Reg3 = GeneralFPRegisters[i + 2];
      const auto Reg4 = GeneralFPRegisters[i + 3];
      ld1<ARMEmitter::SubRegSize::i64Bit>(Reg1.Q(), Reg2.Q(), Reg3.Q(), Reg4.Q(), ARMEmitter::Reg::rsp, 64);
    }
  }

  for (size_t i = 0; i < ConfiguredDynamicRegisterBase.size(); i += 2) {
    const auto Reg1 = ConfiguredDynamicRegisterBase[i];
    const auto Reg2 = ConfiguredDynamicRegisterBase[i + 1];
    ldp<ARMEmitter::IndexType::POST>(Reg1.X(), Reg2.X(), ARMEmitter::Reg::rsp, 16);
  }

  ldr<ARMEmitter::IndexType::POST>(ARMEmitter::XReg::lr, ARMEmitter::Reg::rsp, 16);
}

void Arm64Emitter::Align16B() {
  uint64_t CurrentOffset = GetCursorAddress<uint64_t>();
  for (uint64_t i = (16 - (CurrentOffset & 0xF)); i != 0; i -= 4) {
    nop();
  }
}

}
