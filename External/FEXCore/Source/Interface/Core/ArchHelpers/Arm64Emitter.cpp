#include "Interface/Core/ArchHelpers/Arm64Emitter.h"
#include "Interface/Core/Dispatcher/Dispatcher.h"
#include "Interface/Context/Context.h"
#include "Interface/HLE/Thunks/Thunks.h"

#include <FEXCore/Core/CoreState.h>
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
Arm64Emitter::Arm64Emitter(FEXCore::Context::Context *ctx, size_t size)
  : vixl::aarch64::Assembler(size ? (byte*)FEXCore::Allocator::mmap(nullptr, size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0) : reinterpret_cast<byte*>(~0ULL),
                             size,
                             vixl::aarch64::PositionDependentCode)
  , EmitterCTX {ctx} {
  CPU.SetUp();

#ifdef VIXL_SIMULATOR
  auto Features = vixl::CPUFeatures::All();
#else
  auto Features = vixl::CPUFeatures::InferFromOS();
  if (ctx->HostFeatures.SupportsAtomics) {
    // Hypervisor can hide this on the c630?
    Features.Combine(vixl::CPUFeatures::Feature::kLORegions);
  }
#endif

  SetCPUFeatures(Features);
}

Arm64Emitter::~Arm64Emitter() {
  auto CodeBuffer = GetBuffer();
  if (CodeBuffer->GetCapacity()) {
    FEXCore::Allocator::munmap(CodeBuffer->GetStartAddress<void*>(), CodeBuffer->GetCapacity());
  }
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
      if (EmitterCTX->HostFeatures.SupportsAVX) {
        for (size_t i = 0; i < SRAFPR.size(); i++) {
          const auto Reg = SRAFPR[i];

          if (((1U << Reg.GetCode()) & FPRSpillMask) != 0) {
            mov(TMP4, offsetof(Core::CpuStateFrame, State.xmm.avx.data[i][0]));
            st1b(Reg.Z().VnB(), PRED_TMP_32B, SVEMemOperand(STATE, TMP4));
          }
        }
      } else {
        if (GPRSpillMask && FPRSpillMask == ~0U) {
          // Optimize the common case where we can spill four registers per instruction
          auto TmpReg = SRA64[__builtin_ffs(GPRSpillMask)];
          // Load the sse offset in to the temporary register
          add(TmpReg, STATE, offsetof(FEXCore::Core::CpuStateFrame, State.xmm.sse.data[0][0]));
          for (size_t i = 0; i < SRAFPR.size(); i += 4) {
            const auto Reg1 = SRAFPR[i];
            const auto Reg2 = SRAFPR[i + 1];
            const auto Reg3 = SRAFPR[i + 2];
            const auto Reg4 = SRAFPR[i + 3];
            st1(Reg1.V2D(), Reg2.V2D(), Reg3.V2D(), Reg4.V2D(), MemOperand(TmpReg, 64, PostIndex));
          }
        }
        else {
          for (size_t i = 0; i < SRAFPR.size(); i += 2) {
            const auto Reg1 = SRAFPR[i];
            const auto Reg2 = SRAFPR[i + 1];

            if (((1U << Reg1.GetCode()) & FPRSpillMask) &&
                ((1U << Reg2.GetCode()) & FPRSpillMask)) {
              stp(Reg1.Q(), Reg2.Q(), MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, State.xmm.sse.data[i][0])));
            }
            else if (((1U << Reg1.GetCode()) & FPRSpillMask)) {
              str(Reg1.Q(), MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, State.xmm.sse.data[i][0])));
            }
            else if (((1U << Reg2.GetCode()) & FPRSpillMask)) {
              str(Reg2.Q(), MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, State.xmm.sse.data[i+1][0])));
            }
          }
        }
      }
    }
  }
}

void Arm64Emitter::FillStaticRegs(bool FPRs, uint32_t GPRFillMask, uint32_t FPRFillMask) {
  if (StaticRegisterAllocation()) {
    if (FPRs) {
      if (EmitterCTX->HostFeatures.SupportsAVX) {
        // Set up predicate registers.
        // We don't bother spilling these in SpillStaticRegs,
        // since all that matters is we restore them on a fill.
        // It's not a concern if they get trounced by something else.
        ptrue(PRED_TMP_16B.VnB(), SVE_VL16);
        ptrue(PRED_TMP_32B.VnB(), SVE_VL32);

        for (size_t i = 0; i < SRAFPR.size(); i++) {
          const auto Reg = SRAFPR[i];

          if (((1U << Reg.GetCode()) & FPRFillMask) != 0) {
            mov(TMP4, offsetof(Core::CpuStateFrame, State.xmm.avx.data[i][0]));
            ld1b(Reg.Z().VnB(), PRED_TMP_32B.Zeroing(), SVEMemOperand(STATE, TMP4));
          }
        }
      } else {
        if (GPRFillMask && FPRFillMask == ~0U) {
          // Optimize the common case where we can fill four registers per instruction.
          // Use one of the filling static registers before we fill it.
          auto TmpReg = SRA64[__builtin_ffs(GPRFillMask)];
          // Load the sse offset in to the temporary register
          add(TmpReg, STATE, offsetof(FEXCore::Core::CpuStateFrame, State.xmm.sse.data[0][0]));
          for (size_t i = 0; i < SRAFPR.size(); i += 4) {
            const auto Reg1 = SRAFPR[i];
            const auto Reg2 = SRAFPR[i + 1];
            const auto Reg3 = SRAFPR[i + 2];
            const auto Reg4 = SRAFPR[i + 3];
            ld1(Reg1.V2D(), Reg2.V2D(), Reg3.V2D(), Reg4.V2D(), MemOperand(TmpReg, 64, PostIndex));
          }
        }
        else {
          for (size_t i = 0; i < SRAFPR.size(); i += 2) {
            const auto Reg1 = SRAFPR[i];
            const auto Reg2 = SRAFPR[i + 1];

            if (((1U << Reg1.GetCode()) & FPRFillMask) &&
                ((1U << Reg2.GetCode()) & FPRFillMask)) {
              ldp(Reg1.Q(), Reg2.Q(), MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, State.xmm.sse.data[i][0])));
            }
            else if (((1U << Reg1.GetCode()) & FPRFillMask)) {
              ldr(Reg1.Q(), MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, State.xmm.sse.data[i][0])));
            }
            else if (((1U << Reg2.GetCode()) & FPRFillMask)) {
              ldr(Reg2.Q(), MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, State.xmm.sse.data[i+1][0])));
            }
          }
        }
      }
    }

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
  }
}

void Arm64Emitter::PushDynamicRegsAndLR(aarch64::Register TmpReg) {
  const auto CanUseSVE = EmitterCTX->HostFeatures.SupportsAVX;
  const auto GPRSize = 1 * Core::CPUState::GPR_REG_SIZE;
  const auto FPRRegSize = CanUseSVE ? Core::CPUState::XMM_AVX_REG_SIZE
                                    : Core::CPUState::XMM_SSE_REG_SIZE;
  const auto FPRSize = RAFPR.size() * FPRRegSize;
  const uint64_t SPOffset = AlignUp(GPRSize + FPRSize, 16);

  sub(sp, sp, SPOffset);

  // rsp capable move
  add(TmpReg, aarch64::sp, 0);

  if (CanUseSVE) {
    for (size_t i = 0; i < RAFPR.size(); i += 4) {
      const auto Reg1 = RAFPR[i];
      const auto Reg2 = RAFPR[i + 1];
      const auto Reg3 = RAFPR[i + 2];
      const auto Reg4 = RAFPR[i + 3];
      st4b(Reg1.Z().VnB(), Reg2.Z().VnB(), Reg3.Z().VnB(), Reg4.Z().VnB(), PRED_TMP_32B, SVEMemOperand(TmpReg));
      add(TmpReg, TmpReg, 32 * 4);
    }
  } else {
    for (size_t i = 0; i < RAFPR.size(); i += 4) {
      const auto Reg1 = RAFPR[i];
      const auto Reg2 = RAFPR[i + 1];
      const auto Reg3 = RAFPR[i + 2];
      const auto Reg4 = RAFPR[i + 3];
      st1(Reg1.V2D(), Reg2.V2D(), Reg3.V2D(), Reg4.V2D(), MemOperand(TmpReg, 64, PostIndex));
    }
  }

  str(aarch64::lr, MemOperand(TmpReg, 0));
}

void Arm64Emitter::PopDynamicRegsAndLR() {
  const auto CanUseSVE = EmitterCTX->HostFeatures.SupportsAVX;

  if (CanUseSVE) {
    for (size_t i = 0; i < RAFPR.size(); i += 4) {
      const auto Reg1 = RAFPR[i];
      const auto Reg2 = RAFPR[i + 1];
      const auto Reg3 = RAFPR[i + 2];
      const auto Reg4 = RAFPR[i + 3];
      ld4b(Reg1.Z().VnB(), Reg2.Z().VnB(), Reg3.Z().VnB(), Reg4.Z().VnB(), PRED_TMP_32B.Zeroing(), SVEMemOperand(aarch64::sp));
      add(aarch64::sp, aarch64::sp, 32 * 4);
    }
  } else {
    for (size_t i = 0; i < RAFPR.size(); i += 4) {
      const auto Reg1 = RAFPR[i];
      const auto Reg2 = RAFPR[i + 1];
      const auto Reg3 = RAFPR[i + 2];
      const auto Reg4 = RAFPR[i + 3];
      ld1(Reg1.V2D(), Reg2.V2D(), Reg3.V2D(), Reg4.V2D(), MemOperand(aarch64::sp, 64, PostIndex));
    }
  }

  ldr(aarch64::lr, MemOperand(aarch64::sp, 16, PostIndex));
}

void Arm64Emitter::Align16B() {
  uint64_t CurrentOffset = GetCursorAddress<uint64_t>();
  for (uint64_t i = (16 - (CurrentOffset & 0xF)); i != 0; i -= 4) {
    nop();
  }
}

}
