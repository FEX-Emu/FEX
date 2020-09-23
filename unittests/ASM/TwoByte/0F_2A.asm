%ifdef CONFIG
{
  "RegData": {
    "MM0":  "0x0000000100000002",
    "XMM0": ["0x3f80000040000000", "0x7172737475767778"]
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x0000000100000002
mov [rdx + 8 * 0], rax
mov rax, 0x5152535455565758
mov [rdx + 8 * 1], rax

mov rax, 0x6162636465666768
mov [rdx + 8 * 2], rax
mov rax, 0x7172737475767778
mov [rdx + 8 * 3], rax

movq mm0, [rdx + 8 * 0]
movaps xmm0, [rdx + 8 * 2]

cvtpi2ps xmm0, mm0

hlt
