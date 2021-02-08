
.intel_syntax noprefix

.text
.globl DispatcherExit

DispatcherExit:

.cfi_startproc simple
.cfi_def_cfa rsp, 8*8

.cfi_offset r15, 0x30
.cfi_offset r14, 0x28
.cfi_offset r13, 0x20
.cfi_offset r12, 0x18
.cfi_offset rbp, 0x10
.cfi_offset rbx, 0x08
.cfi_offset rip, 0x00

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