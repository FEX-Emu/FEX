%ifdef CONFIG
{
  "RegData": {
    "MM0": "0x4851525354555657",
    "MM2": "0x0061626364656667",
    "MM3": "0x0"
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

mov rax, 0x6162636465666768
mov [rdx + 8 * 2], rax
mov rax, 0x7172737475767778
mov [rdx + 8 * 3], rax

movq mm0, [rdx]
movq mm1, [rdx + 8 * 1]

palignr mm0, mm1, 1

movq mm2, [rdx + 8 * 2]
movq mm3, [rdx + 8 * 3]

palignr mm2, mm3, 9

movq mm3, [rdx + 8 * 2]
movq mm4, [rdx + 8 * 3]

palignr mm3, mm4, 16

hlt
