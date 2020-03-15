%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x3",
    "RBX": "0x3",
    "RCX": "0x3",
    "RDX": "0x40",
    "RSI": "0x0",
    "R14": "0x0",
    "R13": "0x0",
    "R12": "0x20",
    "R11": "0x10"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x4142434445464748
mov [rdx + 8 * 0], rax
mov rax, 0x5152535455565758
mov [rdx + 8 * 1], rax
mov rax, 0x6162636465666768
mov [rdx + 8 * 2], rax

mov rax, 0
mov [rdx + 8 * 3], rax

tzcnt ax, word [rdx + 8 * 0]
tzcnt ebx, dword [rdx + 8 * 1]
tzcnt rcx, qword [rdx + 8 * 2]

mov r15, 0
mov r12, 0
mov r11, 0
tzcnt rdx, r15
tzcnt r12d, r15d
tzcnt r11w, r15w

mov r15, 0xFFFFFFFFFFFFFFFF
tzcnt esi, r15d
tzcnt r14w, r15w
tzcnt r13, r15

hlt
