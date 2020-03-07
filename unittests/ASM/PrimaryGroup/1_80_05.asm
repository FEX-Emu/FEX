%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x414243444546E648"
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

sub byte [rdx + 8 * 0 + 1], 0x61
mov rax, [rdx + 8 * 0]

hlt
