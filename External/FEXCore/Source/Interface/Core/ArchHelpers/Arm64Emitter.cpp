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

// We want vixl to not allocate a default buffer. Jit and dispatcher will manually create one.
Arm64Emitter::Arm64Emitter(FEXCore::Context::ContextImpl *ctx, size_t size)
  : Emitter(size ? (uint8_t*)FEXCore::Allocator::VirtualAlloc(size, true) : nullptr, size)
  , EmitterCTX {ctx} {
  CPU.SetUp();

  // Number of register available is dependent on what operating mode the proccess is in.
  if (EmitterCTX->Config.Is64BitMode()) {
    ConfiguredGPRs = NumGPRs64;
    ConfiguredSRAGPRs = NumSRAGPRs64;
    ConfiguredGPRPairs = NumGPRPairs64;
    ConfiguredFPRs = NumFPRs64;
    ConfiguredSRAFPRs = NumSRAFPRs64;
    ConfiguredDynamicGPRs = NumGPRs64 - NumGPRs64; // Will be zero, just to be consistent with 32-bit side
    ConfiguredDynamicRegisterBase = nullptr;
  }
  else {
    ConfiguredGPRs = NumGPRs32;
    ConfiguredSRAGPRs = NumSRAGPRs32;
    ConfiguredGPRPairs = NumGPRPairs32;
    ConfiguredFPRs = NumFPRs32;
    ConfiguredSRAFPRs = NumSRAFPRs32;
    ConfiguredDynamicGPRs = NumGPRs32 - NumGPRs64; // Will be 8
    ConfiguredDynamicRegisterBase = &RA64[9];
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
  const std::array<std::pair<ARMEmitter::XRegister, ARMEmitter::XRegister>, 6> CalleeSaved = {{
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

  const std::array<std::pair<ARMEmitter::XRegister, ARMEmitter::XRegister>, 6> CalleeSaved = {{
    {ARMEmitter::XReg::x29, ARMEmitter::XReg::x30},
    {ARMEmitter::XReg::x27, ARMEmitter::XReg::x28},
    {ARMEmitter::XReg::x25, ARMEmitter::XReg::x26},
    {ARMEmitter::XReg::x23, ARMEmitter::XReg::x24},
    {ARMEmitter::XReg::x21, ARMEmitter::XReg::x22},
    {ARMEmitter::XReg::x19, ARMEmitter::XReg::x20},
  }};

  for (auto &RegPair : CalleeSaved) {
    ldp<ARMEmitter::IndexType::POST>(RegPair.first, RegPair.second, ARMEmitter::Reg::rsp, 16);
  }
}

void Arm64Emitter::SpillStaticRegs(bool FPRs, uint32_t GPRSpillMask, uint32_t FPRSpillMask) {
  if (!StaticRegisterAllocation()) {
    return;
  }

  for (size_t i = 0; i < ConfiguredSRAGPRs; i+=2) {
    auto Reg1 = SRA64[i];
    auto Reg2 = SRA64[i+1];
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
      for (size_t i = 0; i < ConfiguredSRAFPRs; i++) {
        const auto Reg = SRAFPR[i];

        if (((1U << Reg.Idx()) & FPRSpillMask) != 0) {
          mov(ARMEmitter::Size::i64Bit, TMP4.R(), offsetof(Core::CpuStateFrame, State.xmm.avx.data[i][0]));
          st1b<ARMEmitter::SubRegSize::i8Bit>(Reg.Z(), PRED_TMP_32B, STATE.R(), TMP4.R());
        }
      }
    } else {
      if (GPRSpillMask && FPRSpillMask == ~0U) {
        // Optimize the common case where we can spill four registers per instruction
        auto TmpReg = SRA64[FindFirstSetBit(GPRSpillMask)];

        // Load the sse offset in to the temporary register
        add(ARMEmitter::Size::i64Bit, TmpReg, STATE.R(), offsetof(FEXCore::Core::CpuStateFrame, State.xmm.sse.data[0][0]));
        for (size_t i = 0; i < ConfiguredSRAFPRs; i += 4) {
          const auto Reg1 = SRAFPR[i];
          const auto Reg2 = SRAFPR[i + 1];
          const auto Reg3 = SRAFPR[i + 2];
          const auto Reg4 = SRAFPR[i + 3];
          st1<ARMEmitter::SubRegSize::i64Bit>(Reg1.Q(), Reg2.Q(), Reg3.Q(), Reg4.Q(), TmpReg, 64);
        }
      }
      else {
        for (size_t i = 0; i < ConfiguredSRAFPRs; i += 2) {
          const auto Reg1 = SRAFPR[i];
          const auto Reg2 = SRAFPR[i + 1];

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

      for (size_t i = 0; i < ConfiguredSRAFPRs; i++) {
        const auto Reg = SRAFPR[i];
        if (((1U << Reg.Idx()) & FPRFillMask) != 0) {
          mov(ARMEmitter::Size::i64Bit, TMP4.R(), offsetof(Core::CpuStateFrame, State.xmm.avx.data[i][0]));
          ld1b<ARMEmitter::SubRegSize::i8Bit>(Reg.Z(), PRED_TMP_32B.Zeroing(), STATE.R(), TMP4.R());
        }
      }
    } else {
      if (GPRFillMask && FPRFillMask == ~0U) {
        // Optimize the common case where we can fill four registers per instruction.
        // Use one of the filling static registers before we fill it.
        auto TmpReg = SRA64[FindFirstSetBit(GPRFillMask)];

        // Load the sse offset in to the temporary register
        add(ARMEmitter::Size::i64Bit, TmpReg, STATE.R(), offsetof(FEXCore::Core::CpuStateFrame, State.xmm.sse.data[0][0]));
        for (size_t i = 0; i < ConfiguredSRAFPRs; i += 4) {
          const auto Reg1 = SRAFPR[i];
          const auto Reg2 = SRAFPR[i + 1];
          const auto Reg3 = SRAFPR[i + 2];
          const auto Reg4 = SRAFPR[i + 3];
          ld1<ARMEmitter::SubRegSize::i64Bit>(Reg1.Q(), Reg2.Q(), Reg3.Q(), Reg4.Q(), TmpReg, 64);
        }
      }
      else {
        for (size_t i = 0; i < ConfiguredSRAFPRs; i += 2) {
          const auto Reg1 = SRAFPR[i];
          const auto Reg2 = SRAFPR[i + 1];

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

  for (size_t i = 0; i < ConfiguredSRAGPRs; i+=2) {
    auto Reg1 = SRA64[i];
    auto Reg2 = SRA64[i+1];
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
  const auto GPRSize = (ConfiguredDynamicGPRs + 1) * Core::CPUState::GPR_REG_SIZE;
  const auto FPRRegSize = CanUseSVE ? Core::CPUState::XMM_AVX_REG_SIZE
                                    : Core::CPUState::XMM_SSE_REG_SIZE;
  const auto FPRSize = ConfiguredFPRs * FPRRegSize;
  const uint64_t SPOffset = AlignUp(GPRSize + FPRSize, 16);

  sub(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::rsp, ARMEmitter::Reg::rsp, SPOffset);

  // rsp capable move
  add(ARMEmitter::Size::i64Bit, TmpReg, ARMEmitter::Reg::rsp, 0);

  if (CanUseSVE) {
    for (size_t i = 0; i < ConfiguredFPRs; i += 4) {
      const auto Reg1 = RAFPR[i];
      const auto Reg2 = RAFPR[i + 1];
      const auto Reg3 = RAFPR[i + 2];
      const auto Reg4 = RAFPR[i + 3];
      st4b(Reg1.Z(), Reg2.Z(), Reg3.Z(), Reg4.Z(), PRED_TMP_32B, TmpReg, 0);
      add(ARMEmitter::Size::i64Bit, TmpReg, TmpReg, 32 * 4);
    }
  } else {
    LOGMAN_THROW_AA_FMT(ConfiguredFPRs % 4 == 0, "Needs to have multiple of 4 FPRs for RA");
    for (size_t i = 0; i < ConfiguredFPRs; i += 4) {
      const auto Reg1 = RAFPR[i];
      const auto Reg2 = RAFPR[i + 1];
      const auto Reg3 = RAFPR[i + 2];
      const auto Reg4 = RAFPR[i + 3];
      st1<ARMEmitter::SubRegSize::i64Bit>(Reg1.Q(), Reg2.Q(), Reg3.Q(), Reg4.Q(), TmpReg, 64);
    }
  }

  if (ConfiguredDynamicRegisterBase) {
    for (size_t i = 0; i < ConfiguredDynamicGPRs; i += 2) {
      const auto Reg1 = ConfiguredDynamicRegisterBase[i];
      const auto Reg2 = ConfiguredDynamicRegisterBase[i + 1];
      stp<ARMEmitter::IndexType::POST>(Reg1.X(), Reg2.X(), TmpReg, 16);
    }
  }

  str(ARMEmitter::XReg::lr, TmpReg, 0);
}

void Arm64Emitter::PopDynamicRegsAndLR() {
  const auto CanUseSVE = EmitterCTX->HostFeatures.SupportsAVX;

  if (CanUseSVE) {
    for (size_t i = 0; i < ConfiguredFPRs; i += 4) {
      const auto Reg1 = RAFPR[i];
      const auto Reg2 = RAFPR[i + 1];
      const auto Reg3 = RAFPR[i + 2];
      const auto Reg4 = RAFPR[i + 3];
      ld4b(Reg1.Z(), Reg2.Z(), Reg3.Z(), Reg4.Z(), PRED_TMP_32B.Zeroing(), ARMEmitter::Reg::rsp);
      add(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::rsp, ARMEmitter::Reg::rsp, 32 * 4);
    }
  } else {
    for (size_t i = 0; i < ConfiguredFPRs; i += 4) {
      const auto Reg1 = RAFPR[i];
      const auto Reg2 = RAFPR[i + 1];
      const auto Reg3 = RAFPR[i + 2];
      const auto Reg4 = RAFPR[i + 3];
      ld1<ARMEmitter::SubRegSize::i64Bit>(Reg1.Q(), Reg2.Q(), Reg3.Q(), Reg4.Q(), ARMEmitter::Reg::rsp, 64);
    }
  }

  if (ConfiguredDynamicRegisterBase) {
    for (size_t i = 0; i < ConfiguredDynamicGPRs; i += 2) {
      const auto Reg1 = ConfiguredDynamicRegisterBase[i];
      const auto Reg2 = ConfiguredDynamicRegisterBase[i + 1];
      ldp<ARMEmitter::IndexType::POST>(Reg1.X(), Reg2.X(), ARMEmitter::Reg::rsp, 16);
    }
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
