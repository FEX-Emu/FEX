%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x4142434445468E48"
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

shl byte [rdx + 8 * 0 + 1], 1

mov rax, [rdx + 8 * 0]

hlt
