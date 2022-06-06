#include "Interface/Core/ArchHelpers/Arm64Emitter.h"
#include "Interface/Core/Dispatcher/Dispatcher.h"
#include "Interface/Context/Context.h"
#include "Interface/HLE/Thunks/Thunks.h"

#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/MathUtils.h>

#include "aarch64/cpu-aarch64.h"
#include "cpu-features.h"
#include "aarch64/instructions-aarch64.h"
#include "utils-vixl.h"

#include <tuple>

namespace FEXCore::CPU {
#define STATE x28

// We want vixl to not allocate a default buffer. Jit and dispatcher will manually create one.
Arm64Emitter::Arm64Emitter(FEXCore::Context::Context *ctx, size_t size)
  : vixl::aarch64::Assembler(size, vixl::aarch64::PositionDependentCode)
  , EmitterCTX {ctx} {
  CPU.SetUp();

  auto Features = vixl::CPUFeatures::InferFromOS();
  if (ctx->HostFeatures.SupportsAtomics) {
    // Hypervisor can hide this on the c630?
    Features.Combine(vixl::CPUFeatures::Feature::kLORegions);
  }

  SetCPUFeatures(Features);
}

void Arm64Emitter::LoadConstant(vixl::aarch64::Register Reg, uint64_t Constant, bool NOPPad) {
  bool Is64Bit = Reg.IsX();
  int Segments = Is64Bit ? 4 : 2;

  if (Is64Bit && ((~Constant)>> 16) == 0) {
    movn(Reg, (~Constant) & 0xFFFF);

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
        add(Reg, Reg, Constant & 0xFFF);
        NumMoves = 2;
      }
    }
  }
  else {
    movz(Reg, (Constant) & 0xFFFF, 0);
    for (int i = 1; i < Segments; ++i) {
      uint16_t Part = (Constant >> (i * 16)) & 0xFFFF;
      if (Part) {
        movk(Reg, Part, i * 16);
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
  MemOperand PairOffset(sp, -16, PreIndex);
  const std::array<std::pair<vixl::aarch64::Register, vixl::aarch64::Register>, 6> CalleeSaved = {{
    {x19, x20},
    {x21, x22},
    {x23, x24},
    {x25, x26},
    {x27, x28},
    {x29, x30},
  }};

  for (auto &RegPair : CalleeSaved) {
    stp(RegPair.first, RegPair.second, PairOffset);
  }

  // Additionally we need to store the lower 64bits of v8-v15
  // Here's a fun thing, we can use two ST4 instructions to store everything
  // We just need a single sub to sp before that
  const std::array<
    std::tuple<vixl::aarch64::VRegister,
               vixl::aarch64::VRegister,
               vixl::aarch64::VRegister,
               vixl::aarch64::VRegister>, 2> FPRs = {{
    {v8, v9, v10, v11},
    {v12, v13, v14, v15},
  }};

  uint32_t VectorSaveSize = sizeof(uint64_t) * 8;
  sub(sp, sp, VectorSaveSize);
  // SP supporting move
  // We just saved x19 so it is safe
  add(x19, sp, 0);

  MemOperand QuadOffset(x19, 32, PostIndex);
  for (auto &RegQuad : FPRs) {
    st4(std::get<0>(RegQuad).D(),
        std::get<1>(RegQuad).D(),
        std::get<2>(RegQuad).D(),
        std::get<3>(RegQuad).D(),
        0,
        QuadOffset);
  }
}

void Arm64Emitter::PopCalleeSavedRegisters() {
  const std::array<
    std::tuple<vixl::aarch64::VRegister,
               vixl::aarch64::VRegister,
               vixl::aarch64::VRegister,
               vixl::aarch64::VRegister>, 2> FPRs = {{
    {v12, v13, v14, v15},
    {v8, v9, v10, v11},
  }};

  MemOperand QuadOffset(sp, 32, PostIndex);
  for (auto &RegQuad : FPRs) {
    ld4(std::get<0>(RegQuad).D(),
        std::get<1>(RegQuad).D(),
        std::get<2>(RegQuad).D(),
        std::get<3>(RegQuad).D(),
        0,
        QuadOffset);
  }

  MemOperand PairOffset(sp, 16, PostIndex);
  const std::array<std::pair<vixl::aarch64::Register, vixl::aarch64::Register>, 6> CalleeSaved = {{
    {x29, x30},
    {x27, x28},
    {x25, x26},
    {x23, x24},
    {x21, x22},
    {x19, x20},
  }};

  for (auto &RegPair : CalleeSaved) {
    ldp(RegPair.first, RegPair.second, PairOffset);
  }
}


void Arm64Emitter::SpillStaticRegs(bool FPRs, uint32_t GPRSpillMask, uint32_t FPRSpillMask) {
  if (StaticRegisterAllocation()) {
    for (size_t i = 0; i < SRA64.size(); i+=2) {
      auto Reg1 = SRA64[i];
      auto Reg2 = SRA64[i+1];
      if (((1U << Reg1.GetCode()) & GPRSpillMask) &&
          ((1U << Reg2.GetCode()) & GPRSpillMask)) {
        stp(Reg1, Reg2, MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, State.gregs[i])));
      }
      else if (((1U << Reg1.GetCode()) & GPRSpillMask)) {
        str(Reg1, MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, State.gregs[i])));
      }
      else if (((1U << Reg2.GetCode()) & GPRSpillMask)) {
        str(Reg2, MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, State.gregs[i+1])));
      }
    }

    if (FPRs) {
      for (size_t i = 0; i < SRAFPR.size(); i+=2) {
        auto Reg1 = SRAFPR[i];
        auto Reg2 = SRAFPR[i+1];

        if (((1U << Reg1.GetCode()) & FPRSpillMask) &&
            ((1U << Reg2.GetCode()) & FPRSpillMask)) {
          stp(Reg1.Q(), Reg2.Q(), MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, State.xmm[i][0])));
        }
        else if (((1U << Reg1.GetCode()) & FPRSpillMask)) {
          str(Reg1.Q(), MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, State.xmm[i][0])));
        }
        else if (((1U << Reg2.GetCode()) & FPRSpillMask)) {
          str(Reg2.Q(), MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, State.xmm[i+1][0])));
        }
      }
    }
  }
}

void Arm64Emitter::FillStaticRegs(bool FPRs, uint32_t GPRFillMask, uint32_t FPRFillMask) {
  if (StaticRegisterAllocation()) {
    for (size_t i = 0; i < SRA64.size(); i+=2) {
      auto Reg1 = SRA64[i];
      auto Reg2 = SRA64[i+1];
      if (((1U << Reg1.GetCode()) & GPRFillMask) &&
          ((1U << Reg2.GetCode()) & GPRFillMask)) {
        ldp(Reg1, Reg2, MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, State.gregs[i])));
      }
      else if (((1U << Reg1.GetCode()) & GPRFillMask)) {
        ldr(Reg1, MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, State.gregs[i])));
      }
      else if (((1U << Reg2.GetCode()) & GPRFillMask)) {
        ldr(Reg2, MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, State.gregs[i+1])));
      }
    }

    if (FPRs) {
      for (size_t i = 0; i < SRAFPR.size(); i+=2) {
        auto Reg1 = SRAFPR[i];
        auto Reg2 = SRAFPR[i+1];

        if (((1U << Reg1.GetCode()) & FPRFillMask) &&
            ((1U << Reg2.GetCode()) & FPRFillMask)) {
          ldp(Reg1.Q(), Reg2.Q(), MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, State.xmm[i][0])));
        }
        else if (((1U << Reg1.GetCode()) & FPRFillMask)) {
          ldr(Reg1.Q(), MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, State.xmm[i][0])));
        }
        else if (((1U << Reg2.GetCode()) & FPRFillMask)) {
          ldr(Reg2.Q(), MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, State.xmm[i+1][0])));
        }
      }
    }
  }
}

void Arm64Emitter::PushDynamicRegsAndLR() {
  uint64_t SPOffset = AlignUp((RA64.size() + 1) * 8 + RAFPR.size() * 16, 16);

  sub(sp, sp, SPOffset);
  int i = 0;

  for (auto RA : RAFPR)
  {
    str(RA.Q(), MemOperand(sp, i * 8));
    i+=2;
  }

#if 0 // All GPRs should be caller saved
  for (auto RA : RA64)
  {
    str(RA, MemOperand(sp, i * 8));
    i++;
  }
#endif

  str(lr, MemOperand(sp, i * 8));
}

void Arm64Emitter::PopDynamicRegsAndLR() {
  uint64_t SPOffset = AlignUp((RA64.size() + 1) * 8 + RAFPR.size() * 16, 16);
  int i = 0;

  for (auto RA : RAFPR)
  {
    ldr(RA.Q(), MemOperand(sp, i * 8));
    i+=2;
  }

#if 0 // All GPRs should be caller saved
  for (auto RA : RA64)
  {
    ldr(RA, MemOperand(sp, i * 8));
    i++;
  }
#endif

  ldr(lr, MemOperand(sp, i * 8));

  add(sp, sp, SPOffset);
}

void Arm64Emitter::Align16B() {
  uint64_t CurrentOffset = GetCursorAddress<uint64_t>();
  for (uint64_t i = (16 - (CurrentOffset & 0xF)); i != 0; i -= 4) {
    nop();
  }
}

}
