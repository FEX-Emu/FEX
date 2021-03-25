.intel_syntax noprefix

.text

.globl Dispatcher
Dispatcher:
// This isn't actually the dispatcher
// But if we label it as dispatcher, then this stack frame (which
// we faked in the real dispatcher) will show up named Dispatcher()
// in the backtrace, which is more or less the truth.

    .cfi_sections .eh_frame, .debug_frame

    .cfi_startproc
    .cfi_adjust_cfa_offset 56
    .cfi_offset rbx, -0x10
    .cfi_offset rbp, -0x18
    .cfi_offset r12, -0x20
    .cfi_offset r13, -0x28
    .cfi_offset r14, -0x30
    .cfi_offset r15, -0x38

// This nop stands in for what would theoretically be a call instruction
    nop

// The real dispatcher will set this label as it's return address
.globl DispatcherExitReturn
DispatcherExitReturn:

    add rsp, 8
.cfi_adjust_cfa_offset -8
    pop r15
.cfi_adjust_cfa_offset -8
    pop r14
.cfi_adjust_cfa_offset -8
    pop r13
.cfi_adjust_cfa_offset -8
    pop r12
.cfi_adjust_cfa_offset -8
    pop rbp
.cfi_adjust_cfa_offset -8
    pop rbx
.cfi_adjust_cfa_offset -8

    ret
.cfi_endproc