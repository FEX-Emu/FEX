%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0x2424242424242424", "0x2424242424242424"],
    "XMM1": ["0x2424242424242424", "0x2424242424242424"],
    "XMM2": ["0x1818181818181818", "0x1818181818181818"]
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x3C3C3C3C3C3C3C3C
mov [rdx + 8 * 0], rax
mov rax, 0x3C3C3C3C3C3C3C3C
mov [rdx + 8 * 1], rax

mov rax, 0x1818181818181818
mov [rdx + 8 * 2], rax
mov rax, 0x1818181818181818
mov [rdx + 8 * 3], rax

movapd xmm0, [rdx]
pxor xmm0, [rdx + 8 * 2]

movapd xmm1, [rdx]
movapd xmm2, [rdx + 8 * 2]
pxor xmm1, xmm2

hlt
