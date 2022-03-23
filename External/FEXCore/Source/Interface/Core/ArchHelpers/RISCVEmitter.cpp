#include "Interface/Core/ArchHelpers/RISCVEmitter.h"
#include "Interface/Context/Context.h"

#include <biscuit/assembler.hpp>
#include <FEXCore/Utils/MathUtils.h>

namespace FEXCore::CPU {
using namespace biscuit;
RISCVEmitter::RISCVEmitter(FEXCore::Context::Context *ctx, uint8_t* Buffer, size_t size)
  : biscuit::Assembler(Buffer, size) {
}

RISCVEmitter::RISCVEmitter(FEXCore::Context::Context *ctx, size_t size)
  : biscuit::Assembler(size) {
}

void RISCVEmitter::LoadConstant(biscuit::GPR Reg, uint64_t Constant, bool NOPPad) {
  // XXX: This can be done way better
  // 8 bits at a time
  uint64_t Mask = 0b1111'1111ULL;
  MV(Reg, zero);
  bool Pushed = false;
  for (size_t i = 0; i < 64; i += 8) {
    uint8_t Tmp = (Constant >> (56 - i)) & Mask;
    if (Tmp) {
      ORI(Reg, Reg, Tmp);
      Pushed = true;
    }
    if (i != 56 && Pushed) {
      SLLI64(Reg, Reg, 8);
    }
  }
}

void RISCVEmitter::SpillStaticRegs(bool FPRs, uint32_t GPRSpillMask, uint32_t FPRSpillMask) {
  // XXX: SRA isn't enabled
  //if (StaticRegisterAllocation()) {
  //  for (size_t i = 0; i < SRA64.size(); ++i) {
  //    auto Reg1 = SRA64[i];
  //    if (((1U << Reg1.Index()) & GPRSpillMask)) {
  //      SD(Reg1, offsetof(FEXCore::Core::CpuStateFrame, State.gregs[i]), STATE);
  //    }
  //  }

  //  // XXX: FPRs
  //}
}

void RISCVEmitter::FillStaticRegs(bool FPRs, uint32_t GPRFillMask, uint32_t FPRFillMask) {
  // XXX: SRA isn't enabled
  //if (StaticRegisterAllocation()) {
  //  for (size_t i = 0; i < SRA64.size(); ++i) {
  //    auto Reg1 = SRA64[i];
  //    if (((1U << Reg1.Index()) & GPRFillMask)) {
  //      LD(Reg1, offsetof(FEXCore::Core::CpuStateFrame, State.gregs[i]), STATE);
  //    }
  //  }

  //  // XXX: FPRs
  //}
}

void RISCVEmitter::PushDynamicRegsAndLR() {
}

void RISCVEmitter::PopDynamicRegsAndLR() {
}

void RISCVEmitter::PushCalleeSavedRegisters() {
  const std::vector<biscuit::GPR> GPRs = {
    x1, // Not technically calleee saved
    x8, x9,
    x18, x19, x20, x21, x22, x23, x24, x25, x26, x27
  };
  int32_t SpaceRequired = GPRs.size() * 8;
  SpaceRequired = FEXCore::AlignUp(SpaceRequired, 16);

  ADDI(sp, sp, -SpaceRequired);
  size_t Offset = 0;
  for (auto Reg : GPRs) {
    SD(Reg, Offset, sp);
    Offset += 8;
  }
}

void RISCVEmitter::PopCalleeSavedRegisters() {
  const std::vector<biscuit::GPR> GPRs = {
    x1, // Not technically calleee saved
    x8, x9,
    x18, x19, x20, x21, x22, x23, x24, x25, x26, x27
  };
  int32_t SpaceRequired = GPRs.size() * 8;
  SpaceRequired = FEXCore::AlignUp(SpaceRequired, 16);

  size_t Offset = 0;
  for (auto Reg : GPRs) {
    LD(Reg, Offset, sp);
    Offset += 8;
  }

  ADDI(sp, sp, SpaceRequired);
}


void RISCVEmitter::ResetStack() {
  if (SpillSlots == 0)
    return;

  ADDI(sp, sp, SpillSlots * 16);
}

}
