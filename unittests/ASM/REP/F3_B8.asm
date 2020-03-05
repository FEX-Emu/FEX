%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x6",
    "RBX": "0x10",
    "RCX": "0x1D",
    "RDX": "0x0",
    "RSI": "0x20",
    "R14": "0x10",
    "R13": "0x40"
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

popcnt ax, word [rdx + 8 * 0]
popcnt ebx, dword [rdx + 8 * 1]
popcnt rcx, qword [rdx + 8 * 2]

mov r15, 0
popcnt rdx, r15

mov r15, 0xFFFFFFFFFFFFFFFF
popcnt esi, r15d
popcnt r14w, r15w
popcnt r13, r15

hlt
