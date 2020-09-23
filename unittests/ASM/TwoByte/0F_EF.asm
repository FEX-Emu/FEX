%ifdef CONFIG
{
  "RegData": {
    "MM0": "0x2424242424242424",
    "MM1": "0x2424242424242424",
    "MM2": "0x1818181818181818"
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

movq mm0, [rdx]
pxor mm0, [rdx + 8 * 2]

movq mm1, [rdx]
movq mm2, [rdx + 8 * 2]
pxor mm1, mm2

hlt
