%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0x4142434445464748", "0x5152535455565758"],
    "XMM1": ["0x4142434445464748", "0x5152535455565758"]
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0
mov [rdx + 8 * 0], rax
mov [rdx + 8 * 1], rax

mov rax, 0x4142434445464748
mov [rdx + 8 * 2], rax
mov rax, 0x5152535455565758
mov [rdx + 8 * 3], rax

movupd xmm1, [rdx + 8 * 2]
movupd [rdx + 8 * 0], xmm1
movupd xmm0, [rdx + 8 * 0]

hlt
