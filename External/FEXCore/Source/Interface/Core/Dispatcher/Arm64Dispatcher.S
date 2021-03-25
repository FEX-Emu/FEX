
.text
.globl Dispatcher
Dispatcher:

// This isn't actually the dispatcher
// But if we label it as dispatcher, then this stack frame (which
// we faked in the real dispatcher) will show up named Dispatcher()
// in the backtrace, which is more or less the truth.

    .cfi_sections .eh_frame, .debug_frame
    .cfi_startproc


    stp x19, x20, [sp, -16]!
.cfi_adjust_cfa_offset 16
.cfi_offset x20, -0x08
.cfi_offset x19, -0x10

    stp x21, x22, [sp, -16]!
.cfi_adjust_cfa_offset 16
.cfi_offset x22, -0x18
.cfi_offset x21, -0x20

    stp x23, x24, [sp, -16]!
.cfi_adjust_cfa_offset 16
.cfi_offset x24, -0x28
.cfi_offset x23, -0x30

    stp x25, x26, [sp, -16]!
.cfi_adjust_cfa_offset 16
.cfi_offset x26, -0x38
.cfi_offset x25, -0x40

    stp x27, x28, [sp, -16]!
.cfi_adjust_cfa_offset 16
.cfi_offset x28, -0x48
.cfi_offset x27, -0x50

    stp x29, x30, [sp, -16]!
.cfi_adjust_cfa_offset 16
.cfi_offset x30, -0x58
.cfi_offset x29, -0x60

    sub sp, sp, 64
.cfi_adjust_cfa_offset 64


// Additionally we need to store the lower 64bits of v8-v15
// Here's a fun thing, we can use two ST4 instructions to store everything
// We just need a single sub to sp before that

// SP supporting move
// We just saved x19 so it is safe
    add x19, sp, 0

    st4 {v8.D, v9.D, v10.D, v11.D}[0], [x19], #32
.cfi_offset d11, -0x88
.cfi_offset d10, -0x90
.cfi_offset d9,  -0x98
.cfi_offset d8,  -0xa0

    st4 {v12.d, v13.d, v14.d, v15.d}[0], [x19], #32
.cfi_offset d15, -0x68
.cfi_offset d14, -0x70
.cfi_offset d13, -0x78
.cfi_offset d12, -0x80

    nop

.globl DispatcherExitReturn
DispatcherExitReturn:

    ld4  {v8.D,  v9.D, v10.D, v11.D}[0], [sp], #32
.cfi_adjust_cfa_offset -32
    ld4 {v12.D, v13.D, v14.D, v15.D}[0], [sp], #32
.cfi_adjust_cfa_offset -32


    ldp x29, x30, [sp], 16
.cfi_adjust_cfa_offset -16

    ldp x27, x28, [sp], 16
.cfi_adjust_cfa_offset -16

    ldp x25, x26, [sp], 16
.cfi_adjust_cfa_offset -16

    ldp x23, x24, [sp], 16
.cfi_adjust_cfa_offset -16

    ldp x21, x22, [sp], 16
.cfi_adjust_cfa_offset -16

    ldp x19, x20, [sp], 16
.cfi_adjust_cfa_offset -16

    br x30
.cfi_endproc