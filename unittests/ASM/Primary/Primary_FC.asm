%ifdef CONFIG
{
  "RegData": {
    "RDI": "0xE0000001"
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

lea rdi, [rdx + 8 * 0]

mov al, 0x0
cld
scasb

hlt
