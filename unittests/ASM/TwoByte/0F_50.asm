%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x0",
    "RBX": "0x3"
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

mov rax, 0x8000000080000000
mov [rdx + 8 * 2], rax
mov rax, 0x7000000070000000
mov [rdx + 8 * 3], rax

movaps xmm0, [rdx + 8 * 0]
movaps xmm1, [rdx + 8 * 2]
movmskps rax, xmm0
movmskps rbx, xmm1

hlt
