%ifdef CONFIG
{
  "RegData": {
    "MM0":  "0x0000000200000001",
    "MM1":  "0xFFFFFFFEFFFFFFFF"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x3ff0000000000000
mov [rdx + 8 * 0], rax
mov rax, 0x4000000000000000
mov [rdx + 8 * 1], rax

mov rax, 0xbff0000000000000
mov [rdx + 8 * 2], rax
mov rax, 0xc000000000000000
mov [rdx + 8 * 3], rax

mov rax, 0x4142434445464748
mov [rdx + 8 * 4], rax
mov rax, 0x5152535455565758
mov [rdx + 8 * 5], rax

movq mm0, [rdx + 8 * 4]
movq mm1, [rdx + 8 * 4]

movapd xmm2, [rdx + 8 * 0]

cvttpd2pi mm0, xmm2
cvttpd2pi mm1, [rdx + 8 * 2]

hlt
