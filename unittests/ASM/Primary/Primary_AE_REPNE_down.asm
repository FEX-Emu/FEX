%ifdef CONFIG
{
  "RegData": {
    "RCX": "5",
    "RDI": "0xE000000D"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x4142434445464748
mov [rdx + 8 * 0], rax
mov rax, 0x5161535455565758
mov [rdx + 8 * 1], rax
mov rax, 0x0
mov [rdx + 8 * 2], rax

lea rdi, [rdx + 8 * 2]

std
mov rax, 0x61
mov rcx, 8
cmp rax, 0

repne scasb

hlt
