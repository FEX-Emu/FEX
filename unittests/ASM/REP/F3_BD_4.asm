%ifdef CONFIG
{
  "RegData": {
    "RAX": "0xFFFFFFFFFFFF0010",
    "RBX": "0xFFFFFFFFFFFF0003",
    "RCX": "0xFFFFFFFFFFFF0007",
    "RDX": "0xFFFFFFFFFFFF000B",
    "RSI": "0xFFFFFFFFFFFF000F",
    "RDI": "0xFFFFFFFFFFFF0000",
    "RBP": "0xFFFFFFFFFFFF0004",
    "RSP": "0xFFFFFFFFFFFF0008",
    "R8":  "0xFFFFFFFFFFFF000F",
    "R9":  "0xFFFFFFFFFFFF0000",
    "R10": "0xFFFFFFFFFFFF0002",
    "R11": "0xFFFFFFFFFFFF0006",
    "R12": "0xFFFFFFFFFFFF000A",
    "R13": "0xFFFFFFFFFFFF000E",
    "R14": "0xFFFFFFFFFFFF000D",
    "R15": "0x0000000000000008"
  }
}
%endif

lea r15, [rel .data]

mov rax, -1
mov rbx, -1
mov rcx, -1
mov rdx, -1
mov rsi, -1
mov rdi, -1
mov rbp, -1
mov rsp, -1
mov r8,  -1
mov r9,  -1
mov r10, -1
mov r11, -1
mov r12, -1
mov r13, -1
mov r14, -1

; We only care about results here
lzcnt ax,  word [r15 + 2 * 0]
lzcnt bx,  word [r15 + 2 * 1]
lzcnt cx,  word [r15 + 2 * 2]
lzcnt dx,  word [r15 + 2 * 3]
lzcnt si,  word [r15 + 2 * 4]
lzcnt di,  word [r15 + 2 * 5]
lzcnt bp,  word [r15 + 2 * 6]
lzcnt sp,  word [r15 + 2 * 7]
lzcnt r8w,  word [r15 + 2 * 4]
lzcnt r9w,  word [r15 + 2 * 9]
lzcnt r10w, word [r15 + 2 * 10]
lzcnt r11w, word [r15 + 2 * 11]
lzcnt r12w, word [r15 + 2 * 12]
lzcnt r13w, word [r15 + 2 * 13]
lzcnt r14w, word [r15 + 2 * 14]
lzcnt r15w, word [r15 + 2 * 15]
movzx r15d, r15w

hlt

.data:
dw 0x0000
dw 0x1FFF
dw 0x01FF
dw 0x001F
dw 0x0001
dw 0x8000
dw 0x0800
dw 0x0080
dw 0x0008
dw 0xFFFF
dw 0x2000
dw 0x0200
dw 0x0020
dw 0x0002
dw 0x0004
