#pragma once

#include "Interface/Core/CodeBuffer.h"

#include "aarch64/assembler-aarch64.h"
#include "aarch64/cpu-aarch64.h"

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
  Arm64Emitter(size_t size);

  CodeBuffer *CurrentCodeBuffer{};

  void SetCodeBuffer(CodeBuffer &Buffer) {
    vixl::CodeBuffer* CurrentVixlBuffer = GetBuffer();
    vixl::CodeBuffer NewVixelBuffer(Buffer.Ptr, Buffer.Size);
    *CurrentVixlBuffer = std::move(NewVixelBuffer);
    CurrentCodeBuffer = &Buffer;
  }

  vixl::aarch64::CPU CPU;
  bool SupportsAtomics{};
  bool SupportsRCPC{};

  void LoadConstant(vixl::aarch64::Register Reg, uint64_t Constant);
  void SpillStaticRegs();
  void FillStaticRegs();

  void PushDynamicRegsAndLR();
  void PopDynamicRegsAndLR();

  void PushCalleeSavedRegisters();
  void PopCalleeSavedRegisters();

  void ResetStack();
  void Align16B();

  uint32_t SpillSlots{};

private:
  CodeBuffer InternalCodeBuffer{};
};

}
