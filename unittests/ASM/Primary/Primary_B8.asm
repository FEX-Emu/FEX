%ifdef CONFIG
{
  "RegData": {
    "RAX": "0xFFFFFFFFFFFF4241",
    "RBX": "0xFFFFFFFFFFFF4241",
    "RCX": "0xFFFFFFFFFFFF4241",
    "RDX": "0xFFFFFFFFFFFF4241",
    "RBP": "0xFFFFFFFFFFFF4241",
    "RSI": "0xFFFFFFFFFFFF4241",
    "RDI": "0xFFFFFFFFFFFF4241",
    "RSP": "0xFFFFFFFFFFFF4241",
    "R8":  "0xFFFFFFFFFFFF4241",
    "R9":  "0xFFFFFFFFFFFF4241",
    "R10": "0xFFFFFFFFFFFF4241",
    "R11": "0xFFFFFFFFFFFF4241",
    "R12": "0xFFFFFFFFFFFF4241",
    "R13": "0xFFFFFFFFFFFF4241",
    "R14": "0xFFFFFFFFFFFF4241",
    "R15": "0xFFFFFFFFFFFF4241"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rax, -1
mov rbx, -1
mov rcx, -1
mov rdx, -1
mov rbp, -1
mov rsi, -1
mov rdi, -1
mov rsp, -1
mov r8, -1
mov r9, -1
mov r10, -1
mov r11, -1
mov r12, -1
mov r13, -1
mov r14, -1
mov r15, -1


mov ax, 0x4241
mov bx, 0x4241
mov cx, 0x4241
mov dx, 0x4241
mov bp, 0x4241
mov si, 0x4241
mov di, 0x4241
mov sp, 0x4241
mov r8w, 0x4241
mov r9w, 0x4241
mov r10w, 0x4241
mov r11w, 0x4241
mov r12w, 0x4241
mov r13w, 0x4241
mov r14w, 0x4241
mov r15w, 0x4241

hlt
