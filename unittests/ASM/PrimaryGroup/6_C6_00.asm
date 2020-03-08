%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x4142434445464761"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x4142434445464748
mov [rdx + 8 * 0], rax

mov byte [rdx + 8 * 0 + 0], 0x61

mov rax, [rdx + 8 * 0]

hlt
