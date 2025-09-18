// SPDX-License-Identifier: MIT
#pragma once
#include <FEXCore/Utils/CompilerDefs.h>

#include <cstdint>

// It's longjump without glibc fortification checks.
namespace FEXCore::LongJump {
// JumpBuf definition needs to be public because the frontend needs to understand it.
#if defined(_M_ARM_64)
struct JumpBuf {
  // All the registers that are required by AAPCS64 to save.
  // GPRs
  // X19, X20, X21, X22,
  // X23, X24, X25, X26,
  // X27, X28, X29, X30,
  //
  // Lower 64-bits:
  //  V8,  V9, V10, V11,
  // V12, V13, V14, V15,
  //
  // SP,
  uint64_t Registers[21];
};
#else
struct JumpBuf {
  // Registers to preserve
  // RBX, RSP, RBP, R12, R13, R14, R15,
  // <return address>
  uint64_t Registers[8];
};
#endif

[[nodiscard]] FEX_DEFAULT_VISIBILITY uint64_t SetJump(JumpBuf& Buffer);
[[noreturn]] FEX_DEFAULT_VISIBILITY void LongJump(JumpBuf& Buffer, uint64_t Value);
} // namespace FEXCore::LongJump
