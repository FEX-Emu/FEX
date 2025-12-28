// SPDX-License-Identifier: MIT
namespace FEXCore::Assert {
// This function can not be inlined
[[noreturn]]
__attribute__((noinline, naked)) void ForcedAssert() {
#ifdef ARCHITECTURE_x86_64
  asm volatile("ud2");
#else
  asm volatile("hlt #1");
#endif
}
} // namespace FEXCore::Assert
