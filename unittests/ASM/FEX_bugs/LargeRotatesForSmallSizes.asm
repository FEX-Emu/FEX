%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x4142434445464748",
    "RBX": "0x4142434445464748",
    "RCX": "0x4142434445464748",
    "RDX": "0x4142434445464748",
    "RSI": "0x4142434445464714",
    "RDI": "0x414243444546478a",
    "RBP": "0x4142434445464704",
    "RSP": "0x414243444546478a",
    "R8":  "0x4142434445461d22",
    "R9":  "0x41424344454651d2",
    "R10": "0x4142434445461d20",
    "R11": "0x41424344454651d2",
    "R12": "0x4142434445463a45",
    "R13": "0x41424344454608e9",
    "R14": "0x4142434445463a41",
    "R15": "0x41424344454608e9"
  }
}
%endif

; FEX-Emu had a bug where 8-bit and 16-bit rotates with carry generated incorrect results when the rotate amount was larger than the data size.
; This is well defined in x86 semantics.

mov cl, 0x9
stc
jmp .test
.test:
; 8-bit: Test 1-bit past data size, plus carry
rcr byte [rel .data + (0 * 8)], cl
rcl byte [rel .data + (1 * 8)], cl

mov cl, 0x9
clc
jmp .test2
.test2:
; 8-bit: Test 1-bit past data size, no carry
rcr byte [rel .data + (2 * 8)], cl
rcl byte [rel .data + (3 * 8)], cl

mov cl, 0x1f
stc
jmp .test3
.test3:
; 8-bit: Test maximum 32-bit rotate, plus carry
rcr byte [rel .data + (4 * 8)], cl
rcl byte [rel .data + (5 * 8)], cl

mov cl, 0x1f
clc
jmp .test4
.test4:
; 8-bit: Test maximum 32-bit rotate, plus carry
rcr byte [rel .data + (6 * 8)], cl
rcl byte [rel .data + (7 * 8)], cl

mov cl, 0xF
stc
jmp .test5
.test5:
; 16-bit: Test 1-bit past data size, plus carry
rcr word [rel .data + (8 * 8)], cl
rcl word [rel .data + (9 * 8)], cl

mov cl, 0xF
clc
jmp .test6
.test6:
; 16-bit: Test 1-bit past data size, no carry
rcr word [rel .data + (10 * 8)], cl
rcl word [rel .data + (11 * 8)], cl

mov cl, 0x1f
stc
jmp .test7
.test7:
; 16-bit: Test maximum 32-bit rotate, plus carry
rcr word [rel .data + (12 * 8)], cl
rcl word [rel .data + (13 * 8)], cl

mov cl, 0x1f
clc
jmp .test8
.test8:
; 16-bit: Test maximum 32-bit rotate, plus carry
rcr word [rel .data + (14 * 8)], cl
rcl word [rel .data + (15 * 8)], cl

jmp .end
.end:

; Load all the results in order
mov rax, [rel .data + (0 * 8)]
mov rbx, [rel .data + (1 * 8)]
mov rcx, [rel .data + (2 * 8)]
mov rdx, [rel .data + (3 * 8)]
mov rsi, [rel .data + (4 * 8)]
mov rdi, [rel .data + (5 * 8)]
mov rbp, [rel .data + (6 * 8)]
mov rsp, [rel .data + (7 * 8)]
mov r8,  [rel .data + (8 * 8)]
mov r9,  [rel .data + (9 * 8)]
mov r10, [rel .data + (10 * 8)]
mov r11, [rel .data + (11 * 8)]
mov r12, [rel .data + (12 * 8)]
mov r13, [rel .data + (13 * 8)]
mov r14, [rel .data + (14 * 8)]
mov r15, [rel .data + (15 * 8)]

hlt

.data:
times 16 dq 0x4142434445464748
