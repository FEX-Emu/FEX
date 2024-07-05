%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x41424344454647a4",
    "RBX": "0x4142434445464790",
    "RCX": "0x4142434445464724",
    "RDX": "0x4142434445464790",
    "RSI": "0x4142434445468e90",
    "RDI": "0x414243444546a3a4",
    "RBP": "0x41424344454623a4",
    "RSP": "0x4142434445468e90",
    "R8":  "0x4142434445464729",
    "R9":  "0x4142434445464741",
    "R10": "0x4142434445464729",
    "R11": "0x4142434445464741",
    "R12": "0x4142434445464729",
    "R13": "0x4142434445464741",
    "R14": "0x4142434445464729",
    "R15": "0x4142434445464741"
  }
}
%endif

; FEX-Emu had a bug where 8-bit and 16-bit rotates with carry generated incorrect results when the rotate amount was larger than the data size.
; These are additional tests to capture more edge cases in the implementation.
; This is well defined in x86 semantics.

mov rax, 0x4142434445464748
mov rbx, 0x4142434445464748
mov rdx, 0x4142434445464748
mov rdi, 0x4142434445464748
mov rsi, 0x4142434445464748
mov rbp, 0x4142434445464748
mov rsp, 0x4142434445464748
mov r8, 0x4142434445464748
mov r9, 0x4142434445464748
mov r10, 0x4142434445464748
mov r11, 0x4142434445464748
mov r12, 0x4142434445464748
mov r13, 0x4142434445464748
mov r14, 0x4142434445464748
mov r15, 0x4142434445464748

mov rcx, 0x515253545556571E
jmp .test
.test:
; 8-bit cl, carry
stc
rcr r8b, cl
rcl r9b, cl

; 8-bit cl, no-carry
stc
rcr r10b, cl
rcl r11b, cl

; 16-bit cl, carry
stc
rcr r12b, cl
rcl r13b, cl

; 16-bit cl, no-carry
stc
rcr r14b, cl
rcl r15b, cl

; Fix RCX since we used it
mov rcx, 0x4142434445464748

; 8-bit const, carry
stc
rcr al, 0x21
rcl bl, 0x21

; 8-bit const, no-carry
clc
rcr cl, 0x21
rcl dl, 0x21

; 16-bit const, carry
stc
rcr di, 0x21
rcl si, 0x21

; 16-bit const, no-carry
clc
rcr bp, 0x21
rcl sp, 0x21

hlt
