#include "Interface/Core/ArchHelpers/Arm64Emitter.h"

#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Core/CoreState.h>

#include "aarch64/cpu-aarch64.h"
#include "cpu-features.h"
#include "aarch64/instructions-aarch64.h"
#include "utils-vixl.h"

#include <tuple>

namespace FEXCore::CPU {
#define STATE x28

// We want vixl to not allocate a default buffer. Jit and dispatcher will manually create one.
Arm64Emitter::Arm64Emitter(size_t size) : vixl::aarch64::Assembler(size, vixl::aarch64::PositionDependentCode) {
  CPU.SetUp();

  auto Features = vixl::CPUFeatures::InferFromOS();
  SupportsAtomics = Features.Has(vixl::CPUFeatures::Feature::kAtomics);
  // RCPC is bugged on Snapdragon 865
  // Causes glibc cond16 test to immediately throw assert
  // __pthread_mutex_cond_lock: Assertion `mutex->__data.__owner == 0'
  SupportsRCPC = false; //Features.Has(vixl::CPUFeatures::Feature::kRCpc);

  if (SupportsAtomics) {
    // Hypervisor can hide this on the c630?
    Features.Combine(vixl::CPUFeatures::Feature::kLORegions);
  }

  SetCPUFeatures(Features);

  if (!SupportsAtomics) {
    WARN_ONCE("Host CPU doesn't support atomics. Expect bad performance");
  }

#ifdef _M_ARM_64
  // We need to get the CPU's cache line size
  // We expect sane targets that have correct cacheline sizes across clusters
  uint64_t CTR;
  __asm volatile ("mrs %[ctr], ctr_el0"
    : [ctr] "=r"(CTR));

  DCacheLineSize = 4 << ((CTR >> 16) & 0xF);
  ICacheLineSize = 4 << (CTR & 0xF);
#endif
}

void Arm64Emitter::LoadConstant(vixl::aarch64::Register Reg, uint64_t Constant) {
  bool Is64Bit = Reg.IsX();
  int Segments = Is64Bit ? 4 : 2;

  if (Is64Bit && ((~Constant)>> 16) == 0) {
    movn(Reg, (~Constant) & 0xFFFF);
    return;
  }

  movz(Reg, (Constant) & 0xFFFF, 0);
  for (int i = 1; i < Segments; ++i) {
    uint16_t Part = (Constant >> (i * 16)) & 0xFFFF;
    if (Part) {
      movk(Reg, Part, i * 16);
    }
  }
}

void Arm64Emitter::PushCalleeSavedRegisters() {
  // We need to save pairs of registers
  // We save r19-r30
  MemOperand PairOffset(sp, -16, PreIndex);
  const std::array<std::pair<vixl::aarch64::XRegister, vixl::aarch64::XRegister>, 6> CalleeSaved = {{
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
  const std::array<std::pair<vixl::aarch64::XRegister, vixl::aarch64::XRegister>, 6> CalleeSaved = {{
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


void Arm64Emitter::SpillStaticRegs() {
  if (StaticRegisterAllocation()) {
    for (size_t i = 0; i < SRA64.size(); i+=2) {
        stp(SRA64[i], SRA64[i+1], MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, State.gregs[i])));
    }

    for (size_t i = 0; i < SRAFPR.size(); i+=2) {
      stp(SRAFPR[i].Q(), SRAFPR[i+1].Q(), MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, State.xmm[i][0])));
    }
  }
}

void Arm64Emitter::FillStaticRegs() {
  if (StaticRegisterAllocation()) {
    for (size_t i = 0; i < SRA64.size(); i+=2) {
      ldp(SRA64[i], SRA64[i+1], MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, State.gregs[i])));
    }

    for (size_t i = 0; i < SRAFPR.size(); i+=2) {
      ldp(SRAFPR[i].Q(), SRAFPR[i+1].Q(), MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, State.xmm[i][0])));
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

void Arm64Emitter::ResetStack() {
  if (SpillSlots == 0)
    return;

  if (IsImmAddSub(SpillSlots * 16)) {
    add(sp, sp, SpillSlots * 16);
  } else {
   // Too big to fit in a 12bit immediate
   LoadConstant(x0, SpillSlots * 16);
   add(sp, sp, x0);
  }
}

void Arm64Emitter::Align16B() {
  uint64_t CurrentOffset = GetCursorAddress<uint64_t>();
  for (uint64_t i = (16 - (CurrentOffset & 0xF)); i != 0; i -= 4) {
      nop();
  }
}

}
