%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x4142434445464748",
    "RBX": "0x5152535455565758",
    "RCX": "0x0",
    "RDI": "0xE0000008",
    "RSI": "0xDFFFFFF8"
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
mov rax, 0x0
mov [rdx + 8 * 2], rax
mov [rdx + 8 * 3], rax
mov [rdx + 8 * 4], rax

lea rdi, [rdx + 8 * 3]
lea rsi, [rdx + 8 * 1]

std
mov rcx, 2
rep movsq ; rdi <- rsi

mov rax, [rdx + 8 * 2]
mov rbx, [rdx + 8 * 3]
mov rcx, [rdx + 8 * 4]
hlt
