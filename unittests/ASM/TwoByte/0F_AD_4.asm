%ifdef CONFIG
{
  "RegData": {
    "RAX": "1",
    "RBX": "1",
    "RDX": "1",
    "RSI": "1",
    "RBP": "1",
    "RSP": "0",
    "R8":  "0",
    "r9":  "0"
  }
}
%endif

mov cl, 1
mov r15, -1
mov r14, 0xFFFFFFFFFFFF0000
mov r13, 0xFFFFFFFF00000000
mov r12, 0

mov rax, 0
mov rbx, 0
mov rdx, 0

mov rsi, 0
mov rdi, 0
mov rbp, 0

mov rsp, 0
mov r8, 0
mov r9, 0

mov r11, 1

; Sign from 0->1  should set OF

; Let's clear OF really quick
mov rax, 0
ror rax, 1

shrd r14w, r15w, cl
cmovo rax, r11

; Let's clear OF really quick
mov rax, 0
ror rax, 1

shrd r13d, r15d, cl
cmovo rbx, r11

; Let's clear OF really quick
mov rax, 0
ror rax, 1

shrd r12, r15, cl
cmovo rdx, r11

; Sign from 1->0 should set OF
mov r15, -1
mov r14, 0xFFFFFFFFFFFF8000
mov r13, 0xFFFFFFFF80000000
mov r12, 0x8000000000000000

; Let's clear OF really quick
mov rax, 0
ror rax, 1

shrd r14w, r15w, cl
cmovo rsi, r11

; Let's clear OF really quick
mov rax, 0
ror rax, 1

shrd r13d, r15d, cl
cmovo rdi, r11

; Let's clear OF really quick
mov rax, 0
ror rax, 1

shrd r12, r15, cl
cmovo rbp, r11

; Sign from 0->0 should NOT set OF
mov r15, 0xFFFFFFFFFFFFFFFE
mov r14, 0xFFFFFFFFFFFF0000
mov r13, 0xFFFFFFFF00000000
mov r12, 0x0000000000000000

; Let's clear OF really quick
mov rax, 0
ror rax, 1

shrd r14w, r15w, cl
cmovo rsp, r11

; Let's clear OF really quick
mov rax, 0
ror rax, 1

shrd r13d, r15d, cl
cmovo r8, r11

; Let's clear OF really quick
mov rax, 0
ror rax, 1

shrd r12, r15, cl
cmovo r9, r11

hlt
