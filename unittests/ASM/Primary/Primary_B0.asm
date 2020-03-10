%ifdef CONFIG
{
  "RegData": {
    "RAX": "0xFFFFFFFFFFFFFF41",
    "RBX": "0xFFFFFFFFFFFFFF41",
    "RCX": "0xFFFFFFFFFFFFFF41",
    "RDX": "0xFFFFFFFFFFFFFF41",
    "RBP": "0xFFFFFFFFFFFFFF41",
    "RSI": "0xFFFFFFFFFFFFFF41",
    "RDI": "0xFFFFFFFFFFFFFF41",
    "RSP": "0xFFFFFFFFFFFFFF41",
    "R8":  "0xFFFFFFFFFFFFFF41",
    "R9":  "0xFFFFFFFFFFFFFF41",
    "R10": "0xFFFFFFFFFFFFFF41",
    "R11": "0xFFFFFFFFFFFFFF41",
    "R12": "0xFFFFFFFFFFFFFF41",
    "R13": "0xFFFFFFFFFFFFFF41",
    "R14": "0xFFFFFFFFFFFFFF41",
    "R15": "0xFFFFFFFFFFFFFF41"
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


mov al, 0x41
mov bl, 0x41
mov cl, 0x41
mov dl, 0x41
mov bpl, 0x41
mov sil, 0x41
mov dil, 0x41
mov spl, 0x41
mov r8b, 0x41
mov r9b, 0x41
mov r10b, 0x41
mov r11b, 0x41
mov r12b, 0x41
mov r13b, 0x41
mov r14b, 0x41
mov r15b, 0x41

hlt
