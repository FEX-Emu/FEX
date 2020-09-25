%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0x3ff0000000000000", "0x0"],
    "XMM1": ["0xc000000000000000", "0xbff0000000000000"]
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x0000000000000001
mov [rdx + 8 * 0], rax
mov rax, 0xFFFFFFFFFFFFFFFE
mov [rdx + 8 * 1], rax

movq mm0, [rdx]
cvtpi2pd xmm0, mm0
cvtpi2pd xmm1, [rdx + 8 * 1]

hlt
