%ifdef CONFIG
{
  "RegData": {
    "RBP": "0xFFFFFFFFFFFFFFFF",
    "RSI": "0xFFFFFFFFFFFFFFFF",

    "RDI": "0xFFFFFFFF",
    "RSP": "0xFFFFFFFF",

    "R8": "0xFFFF",
    "R9": "0xFFFF",

    "R10": "0x01",
    "R11": "0x0",

    "R12": "0x01",
    "R13": "0x0",

    "R14": "0x01",
    "R15": "0xFFFFFFFFFFFF0000"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

; Positive 16bit
mov rax, 1
mov rdx, -1
cwd

mov r14, rax
mov r15, rdx

; Positive 32bit

mov rax, 1
mov rdx, -1
cdq

mov r12, rax
mov r13, rdx

; Positive 64bit

mov rax, 1
mov rdx, -1
cqo

mov r10, rax
mov r11, rdx

; Negative 16bit
mov rax, 0xFFFF
mov rdx, 0
cwd

mov r8, rax
mov r9, rdx

; Negative 32bit
mov rax, 0xFFFFFFFF
mov rdx, 0
cdq

mov rdi, rax
mov rsp, rdx

; Negative 64bit
mov rax, -1
mov rdx, 0
cqo

mov rbp, rax
mov rsi, rdx


hlt
