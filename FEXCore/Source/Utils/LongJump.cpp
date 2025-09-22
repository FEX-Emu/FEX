// SPDX-License-Identifier: MIT
#include <FEXCore/Utils/LongJump.h>

namespace FEXCore::LongJump {
#if defined(_M_ARM_64)
[[nodiscard]]
FEX_DEFAULT_VISIBILITY FEX_NAKED uint64_t SetJump(JumpBuf& Buffer) {
  __asm volatile(R"(
      // x0 contains the jumpbuffer
      stp x19, x20, [x0, #( 0 * 8)];
      stp x21, x22, [x0, #( 2 * 8)];
      stp x23, x24, [x0, #( 4 * 8)];
      stp x25, x26, [x0, #( 6 * 8)];
      stp x27, x28, [x0, #( 8 * 8)];
      stp x29, x30, [x0, #(10 * 8)];

      // FPRs
      stp d8,   d9, [x0, #(12 * 8)];
      stp d10, d11, [x0, #(14 * 8)];
      stp d12, d13, [x0, #(16 * 8)];
      stp d14, d15, [x0, #(18 * 8)];

      // Move SP in to a temporary to store.
      mov x1, sp;
      str x1,  [x0, #(20 * 8)];

      // Return zero to signify this is the SetJump.
      mov x0, #0;
      ret;
    )" ::
                   : "memory");
}

[[noreturn]]
FEX_DEFAULT_VISIBILITY FEX_NAKED void LongJump(JumpBuf& Buffer, uint64_t Value) {
  __asm volatile(R"(
      // x0 contains the jumpbuffer
      ldp x19, x20, [x0, #( 0 * 8)];
      ldp x21, x22, [x0, #( 2 * 8)];
      ldp x23, x24, [x0, #( 4 * 8)];
      ldp x25, x26, [x0, #( 6 * 8)];
      ldp x27, x28, [x0, #( 8 * 8)];
      ldp x29, x30, [x0, #(10 * 8)];

      // FPRs
      ldp d8,   d9, [x0, #(12 * 8)];
      ldp d10, d11, [x0, #(14 * 8)];
      ldp d12, d13, [x0, #(16 * 8)];
      ldp d14, d15, [x0, #(18 * 8)];

      // Load SP in to temporary then move
      ldr x0,  [x0, #(20 * 8)];
      mov sp, x0;

      // Move value in to result register
      mov x0, x1;
      ret;
    )" ::
                   : "memory");
}
#else
[[nodiscard]]
FEX_DEFAULT_VISIBILITY FEX_NAKED uint64_t SetJump(JumpBuf& Buffer) {
  __asm volatile(R"(
    .intel_syntax noprefix;
    // rdi contains the jumpbuffer
    mov [rdi + (0 * 8)], rbx;
    mov [rdi + (1 * 8)], rsp;
    mov [rdi + (2 * 8)], rbp;
    mov [rdi + (3 * 8)], r12;
    mov [rdi + (4 * 8)], r13;
    mov [rdi + (5 * 8)], r14;
    mov [rdi + (6 * 8)], r15;

    // Return address is on the stack, load it and store
    mov rsi, [rsp];
    mov [rdi + (7 * 8)], rsi;

    // Return zero to signify this is the SetJump.
    mov rax, 0;
    ret;

    .att_syntax prefix;
    )" ::
                   : "memory");
}

[[noreturn]]
FEX_DEFAULT_VISIBILITY FEX_NAKED void LongJump(JumpBuf& Buffer, uint64_t Value) {
  __asm volatile(R"(
    .intel_syntax noprefix;
    // rdi contains the jumpbuffer
    mov rbx, [rdi + (0 * 8)];
    mov rsp, [rdi + (1 * 8)];
    mov rbp, [rdi + (2 * 8)];
    mov r12, [rdi + (3 * 8)];
    mov r13, [rdi + (4 * 8)];
    mov r14, [rdi + (5 * 8)];
    mov r15, [rdi + (6 * 8)];

    // Move value in to result register
    mov rax, rsi;

    // Pop the dead return address off the stack
    pop rsi;

    // Load the original return address from the jumpbuffer
    mov rsi, [rdi + (7 * 8)];

    // Return using a jump
    jmp rsi;

    .att_syntax prefix;
    )" ::
                   : "memory");
}

#endif
} // namespace FEXCore::LongJump
