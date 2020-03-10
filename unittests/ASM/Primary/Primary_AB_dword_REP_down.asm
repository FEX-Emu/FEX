%ifdef CONFIG
{
  "RegData": {
    "RAX": "0xF1F2F3F4F1F2F3F4",
    "RDX": "0x0",
    "RDI": "0xE000000C"
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

lea rdi, [rdx + 8 * 2 + 4]

std
mov rax, 0xF1F2F3F4
mov rcx, 2
rep stosd ; rdi <- eax

mov rax, [rdx + 8 * 2]
mov rdx, [rdx + 8 * 3]
hlt
