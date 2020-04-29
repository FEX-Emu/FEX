%ifdef CONFIG
{
  "RegData": {
    "XMM0":  ["0x2020202000000000", "0x0"],
    "XMM1":  ["0x2020202000000000", "0x0"]
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x6162636445464748
mov [rdx + 8 * 0], rax
mov rax, 0x5152535455565758
mov [rdx + 8 * 1], rax

mov rax, 0x4142434465666768
mov [rdx + 8 * 2], rax
mov rax, 0x7172737475767778
mov [rdx + 8 * 3], rax

movaps xmm0, [rdx + 8 * 0]
movaps xmm1, [rdx + 8 * 0]
movaps xmm2, [rdx + 8 * 2]

psubusb xmm0, xmm2
psubusb xmm1, [rdx + 8 * 2]

hlt
